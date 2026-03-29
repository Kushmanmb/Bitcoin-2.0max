// src/net/peer.cpp
//
// Bitcoin 2.0max peer handshake implementation.
//
// Handshake protocol (both directions use the same version + verack exchange):
//
//   Outbound:
//     1. Send our `version`
//     2. Receive their `version`  → send our `verack`
//     3. Receive their `verack`
//
//   Inbound:
//     1. Receive their `version`  → send our `version` + our `verack`
//     2. Receive their `verack`

#include "peer.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

namespace bitcoin2max {
namespace net {

// ── ctor / dtor ───────────────────────────────────────────────────────────────

Peer::Peer(int fd, bool outbound) : fd_(fd), outbound_(outbound) {}

Peer::~Peer() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

// ── doHandshake ───────────────────────────────────────────────────────────────

bool Peer::doHandshake(int32_t bestHeight, uint16_t ourPort) {
    state_ = HandshakeState::IDLE;

    if (outbound_) {
        // ── Outbound flow ──────────────────────────────────────────────────────
        // 1. Send our version.
        if (!sendMsg(buildVersionMsg(bestHeight, ourPort))) {
            state_ = HandshakeState::FAILED;
            return false;
        }
        state_ = HandshakeState::VERSION_SENT;

        // 2. Receive their version.
        NetMessage msg;
        if (!recvMsg(msg) || msg.header.commandStr() != CMD_VERSION) {
            std::cerr << "[Peer] Outbound: expected 'version', got '"
                      << (msg.header.magic ? msg.header.commandStr() : "(recv error)")
                      << "'\n";
            state_ = HandshakeState::FAILED;
            return false;
        }
        parseVersionPayload(msg.payload);
        state_ = HandshakeState::VERSION_RECEIVED;

        // 3. Send our verack.
        if (!sendMsg(buildVerackMsg())) {
            state_ = HandshakeState::FAILED;
            return false;
        }

        // 4. Receive their verack.
        if (!recvMsg(msg) || msg.header.commandStr() != CMD_VERACK) {
            std::cerr << "[Peer] Outbound: expected 'verack', got '"
                      << (msg.header.magic ? msg.header.commandStr() : "(recv error)")
                      << "'\n";
            state_ = HandshakeState::FAILED;
            return false;
        }

    } else {
        // ── Inbound flow ───────────────────────────────────────────────────────
        // 1. Receive their version.
        NetMessage msg;
        if (!recvMsg(msg) || msg.header.commandStr() != CMD_VERSION) {
            std::cerr << "[Peer] Inbound: expected 'version', got '"
                      << (msg.header.magic ? msg.header.commandStr() : "(recv error)")
                      << "'\n";
            state_ = HandshakeState::FAILED;
            return false;
        }
        parseVersionPayload(msg.payload);
        state_ = HandshakeState::VERSION_RECEIVED;

        // 2. Send our version.
        if (!sendMsg(buildVersionMsg(bestHeight, ourPort))) {
            state_ = HandshakeState::FAILED;
            return false;
        }
        state_ = HandshakeState::VERSION_SENT;

        // 3. Send our verack.
        if (!sendMsg(buildVerackMsg())) {
            state_ = HandshakeState::FAILED;
            return false;
        }

        // 4. Receive their verack.
        if (!recvMsg(msg) || msg.header.commandStr() != CMD_VERACK) {
            std::cerr << "[Peer] Inbound: expected 'verack', got '"
                      << (msg.header.magic ? msg.header.commandStr() : "(recv error)")
                      << "'\n";
            state_ = HandshakeState::FAILED;
            return false;
        }
    }

    state_ = HandshakeState::COMPLETE;
    std::cout << "[Peer] Handshake complete — remote: " << remoteUserAgent_
              << "  height=" << remoteHeight_ << "\n";
    return true;
}

// ── sendMsg ───────────────────────────────────────────────────────────────────

bool Peer::sendMsg(const NetMessage& msg) {
    auto wire = msg.serialise();
    size_t offset = 0;
    while (offset < wire.size()) {
        ssize_t n = ::send(fd_,
                           wire.data() + offset,
                           wire.size() - offset,
                           MSG_NOSIGNAL);
        if (n <= 0) {
            if (n < 0)
                std::cerr << "[Peer] send error: " << std::strerror(errno) << "\n";
            return false;
        }
        offset += static_cast<size_t>(n);
    }
    return true;
}

// ── recvMsg ───────────────────────────────────────────────────────────────────

bool Peer::recvMsg(NetMessage& msg) {
    // 1. Read the 24-byte fixed header.
    std::array<uint8_t, MessageHeader::WIRE_SIZE> hdrBuf{};
    size_t received = 0;
    while (received < MessageHeader::WIRE_SIZE) {
        ssize_t n = ::recv(fd_,
                           hdrBuf.data() + received,
                           MessageHeader::WIRE_SIZE - received,
                           0);
        if (n <= 0) {
            if (n < 0)
                std::cerr << "[Peer] recv header error: " << std::strerror(errno) << "\n";
            return false;
        }
        received += static_cast<size_t>(n);
    }

    auto hdr = MessageHeader::deserialise(hdrBuf.data(), hdrBuf.size());

    // Validate magic bytes.
    if (hdr.magic != network::MAGIC) {
        std::cerr << "[Peer] Invalid magic: 0x" << std::hex << hdr.magic << std::dec << "\n";
        return false;
    }

    // Guard against unreasonably large payloads.
    if (hdr.length > network::MAX_MESSAGE_PAYLOAD) {
        std::cerr << "[Peer] Oversized payload: " << hdr.length << " bytes\n";
        return false;
    }

    // 2. Read the payload.
    std::vector<uint8_t> payload(hdr.length);
    received = 0;
    while (received < hdr.length) {
        ssize_t n = ::recv(fd_,
                           payload.data() + received,
                           hdr.length - received,
                           0);
        if (n <= 0) {
            if (n < 0)
                std::cerr << "[Peer] recv payload error: " << std::strerror(errno) << "\n";
            return false;
        }
        received += static_cast<size_t>(n);
    }

    // 3. Verify checksum.
    if (computeChecksum(payload) != hdr.checksum) {
        std::cerr << "[Peer] Checksum mismatch on '" << hdr.commandStr() << "'\n";
        return false;
    }

    msg.header  = hdr;
    msg.payload = std::move(payload);
    return true;
}

// ── parseVersionPayload ───────────────────────────────────────────────────────

bool Peer::parseVersionPayload(const std::vector<uint8_t>& payload) {
    size_t offset = 0;

    auto remaining = [&]() { return payload.size() - offset; };

    auto readU32 = [&]() -> uint32_t {
        uint32_t v = static_cast<uint32_t>(payload[offset    ])        |
                    (static_cast<uint32_t>(payload[offset + 1]) <<  8) |
                    (static_cast<uint32_t>(payload[offset + 2]) << 16) |
                    (static_cast<uint32_t>(payload[offset + 3]) << 24);
        offset += 4;
        return v;
    };

    auto readU64 = [&]() -> uint64_t {
        uint64_t v = 0;
        for (int i = 0; i < 8; ++i)
            v |= (static_cast<uint64_t>(payload[offset + i]) << (8 * i));
        offset += 8;
        return v;
    };

    // version (int32)
    if (remaining() < 4) return false;
    remoteVersion_ = static_cast<int32_t>(readU32());

    // services (uint64) — skip
    if (remaining() < 8) return false;
    readU64();

    // timestamp (int64) — skip
    if (remaining() < 8) return false;
    readU64();

    // addr_recv (26 bytes) — skip
    if (remaining() < 26) return false;
    offset += 26;

    // addr_from (26 bytes) — present when version >= 106
    if (remoteVersion_ >= 106) {
        if (remaining() < 26) return false;
        offset += 26;
    }

    // nonce (uint64) — skip
    if (remaining() < 8) return false;
    offset += 8;

    // user_agent (var_str)
    if (remaining() < 1) return false;
    uint64_t uaLen = payload[offset++];
    if (uaLen == 0xFD) {
        if (remaining() < 2) return false;
        uaLen = static_cast<uint64_t>(payload[offset    ])        |
               (static_cast<uint64_t>(payload[offset + 1]) << 8);
        offset += 2;
    } else if (uaLen == 0xFE) {
        if (remaining() < 4) return false;
        uaLen = static_cast<uint64_t>(payload[offset    ])         |
               (static_cast<uint64_t>(payload[offset + 1]) <<  8) |
               (static_cast<uint64_t>(payload[offset + 2]) << 16) |
               (static_cast<uint64_t>(payload[offset + 3]) << 24);
        offset += 4;
    }
    if (remaining() < uaLen) return false;
    remoteUserAgent_.assign(
        reinterpret_cast<const char*>(payload.data() + offset), uaLen);
    offset += uaLen;

    // start_height (int32)
    if (remaining() < 4) return false;
    remoteHeight_ = static_cast<int32_t>(readU32());

    return true;
}

} // namespace net
} // namespace bitcoin2max
