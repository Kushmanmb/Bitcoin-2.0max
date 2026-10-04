#include "peer.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

namespace bitcoin2max {
namespace net {

Peer::Peer(SocketHandle fd, bool outbound)
    : fd_(fd),
      outbound_(outbound) {}

Peer::~Peer() {
    if (socketValid(fd_)) {
        shutdownSocket(fd_);
        closeSocket(fd_);
        fd_ = INVALID_SOCKET_HANDLE;
    }
}

bool Peer::doHandshake(int32_t bestHeight, uint16_t ourPort) {
    state_ = HandshakeState::IDLE;

    if (outbound_) {
        if (!sendMsg(buildVersionMsg(bestHeight, ourPort))) {
            state_ = HandshakeState::FAILED;
            return false;
        }

        state_ = HandshakeState::VERSION_SENT;

        NetMessage msg;

        if (!recvMsg(msg) || msg.header.commandStr() != CMD_VERSION) {
            std::cerr
                << "[Peer] Outbound: expected 'version', got '"
                << (msg.header.magic
                        ? msg.header.commandStr()
                        : "(recv error)")
                << "'\n";

            state_ = HandshakeState::FAILED;
            return false;
        }

        if (!parseVersionPayload(msg.payload)) {
            std::cerr << "[Peer] Invalid version payload\n";
            state_ = HandshakeState::FAILED;
            return false;
        }

        state_ = HandshakeState::VERSION_RECEIVED;

        if (!sendMsg(buildVerackMsg())) {
            state_ = HandshakeState::FAILED;
            return false;
        }

        if (!recvMsg(msg) ||
            msg.header.commandStr() != CMD_VERACK) {

            std::cerr
                << "[Peer] Outbound: expected 'verack', got '"
                << (msg.header.magic
                        ? msg.header.commandStr()
                        : "(recv error)")
                << "'\n";

            state_ = HandshakeState::FAILED;
            return false;
        }
    }
    else {
        NetMessage msg;

        if (!recvMsg(msg) ||
            msg.header.commandStr() != CMD_VERSION) {

            std::cerr
                << "[Peer] Inbound: expected 'version', got '"
                << (msg.header.magic
                        ? msg.header.commandStr()
                        : "(recv error)")
                << "'\n";

            state_ = HandshakeState::FAILED;
            return false;
        }

        if (!parseVersionPayload(msg.payload)) {
            std::cerr << "[Peer] Invalid version payload\n";
            state_ = HandshakeState::FAILED;
            return false;
        }

        state_ = HandshakeState::VERSION_RECEIVED;

        if (!sendMsg(buildVersionMsg(bestHeight, ourPort))) {
            state_ = HandshakeState::FAILED;
            return false;
        }

        state_ = HandshakeState::VERSION_SENT;

        if (!sendMsg(buildVerackMsg())) {
            state_ = HandshakeState::FAILED;
            return false;
        }

        if (!recvMsg(msg) ||
            msg.header.commandStr() != CMD_VERACK) {

            std::cerr
                << "[Peer] Inbound: expected 'verack', got '"
                << (msg.header.magic
                        ? msg.header.commandStr()
                        : "(recv error)")
                << "'\n";

            state_ = HandshakeState::FAILED;
            return false;
        }
    }

    state_ = HandshakeState::COMPLETE;

    std::cout
        << "[Peer] Handshake complete — remote: "
        << remoteUserAgent_
        << "  height="
        << remoteHeight_
        << "\n";

    return true;
}

bool Peer::sendMsg(const NetMessage& msg) {
    const auto wire = msg.serialise();

    size_t offset = 0;

    while (offset < wire.size()) {
        const size_t remaining = wire.size() - offset;

        const int amount = static_cast<int>(
            remaining > static_cast<size_t>(std::numeric_limits<int>::max())
                ? std::numeric_limits<int>::max()
                : remaining
        );

#ifdef _WIN32
        const int n = ::send(
            fd_,
            reinterpret_cast<const char*>(wire.data() + offset),
            amount,
            SOCKET_SEND_FLAGS
        );
#else
        const ssize_t n = ::send(
            fd_,
            wire.data() + offset,
            remaining,
            SOCKET_SEND_FLAGS
        );
#endif

        if (n <= 0) {
            if (n < 0) {
                std::cerr
                    << "[Peer] send error: "
                    << socketLastError()
                    << "\n";
            }

            return false;
        }

        offset += static_cast<size_t>(n);
    }

    return true;
}

bool Peer::recvMsg(NetMessage& msg) {
    std::array<uint8_t, MessageHeader::WIRE_SIZE> hdrBuf{};

    size_t received = 0;

    while (received < MessageHeader::WIRE_SIZE) {
        const size_t remaining =
            MessageHeader::WIRE_SIZE - received;

#ifdef _WIN32
        const int n = ::recv(
            fd_,
            reinterpret_cast<char*>(hdrBuf.data() + received),
            static_cast<int>(remaining),
            0
        );
#else
        const ssize_t n = ::recv(
            fd_,
            hdrBuf.data() + received,
            remaining,
            0
        );
#endif

        if (n <= 0) {
            if (n < 0) {
                std::cerr
                    << "[Peer] recv header error: "
                    << socketLastError()
                    << "\n";
            }

            return false;
        }

        received += static_cast<size_t>(n);
    }

    auto hdr =
        MessageHeader::deserialise(
            hdrBuf.data(),
            hdrBuf.size()
        );

    if (hdr.magic != network::MAGIC) {
        std::cerr
            << "[Peer] Invalid magic: 0x"
            << std::hex
            << hdr.magic
            << std::dec
            << "\n";

        return false;
    }

    if (hdr.length > network::MAX_MESSAGE_PAYLOAD) {
        std::cerr
            << "[Peer] Oversized payload: "
            << hdr.length
            << " bytes\n";

        return false;
    }

    std::vector<uint8_t> payload(hdr.length);

    received = 0;

    while (received < hdr.length) {
        const size_t remaining =
            static_cast<size_t>(hdr.length) - received;

#ifdef _WIN32
        const int n = ::recv(
            fd_,
            reinterpret_cast<char*>(payload.data() + received),
            static_cast<int>(remaining),
            0
        );
#else
        const ssize_t n = ::recv(
            fd_,
            payload.data() + received,
            remaining,
            0
        );
#endif

        if (n <= 0) {
            if (n < 0) {
                std::cerr
                    << "[Peer] recv payload error: "
                    << socketLastError()
                    << "\n";
            }

            return false;
        }

        received += static_cast<size_t>(n);
    }

    if (computeChecksum(payload) != hdr.checksum) {
        std::cerr
            << "[Peer] Checksum mismatch on '"
            << hdr.commandStr()
            << "'\n";

        return false;
    }

    msg.header = hdr;
    msg.payload = std::move(payload);

    return true;
}

bool Peer::parseVersionPayload(
    const std::vector<uint8_t>& payload
) {
    size_t offset = 0;

    auto remaining = [&]() {
        return payload.size() - offset;
    };

    auto readU32 = [&]() -> uint32_t {
        uint32_t v =
            static_cast<uint32_t>(payload[offset]) |
            (static_cast<uint32_t>(payload[offset + 1]) << 8) |
            (static_cast<uint32_t>(payload[offset + 2]) << 16) |
            (static_cast<uint32_t>(payload[offset + 3]) << 24);

        offset += 4;
        return v;
    };

    auto readU64 = [&]() -> uint64_t {
        uint64_t v = 0;

        for (int i = 0; i < 8; ++i) {
            v |= (
                static_cast<uint64_t>(payload[offset + i])
                << (8 * i)
            );
        }

        offset += 8;
        return v;
    };

    if (remaining() < 4) return false;

    remoteVersion_ =
        static_cast<int32_t>(readU32());

    if (remaining() < 8) return false;
    readU64();

    if (remaining() < 8) return false;
    readU64();

    if (remaining() < 26) return false;
    offset += 26;

    if (remoteVersion_ >= 106) {
        if (remaining() < 26) return false;
        offset += 26;
    }

    if (remaining() < 8) return false;
    offset += 8;

    if (remaining() < 1) return false;

    uint64_t uaLen = payload[offset++];

    if (uaLen == 0xFD) {
        if (remaining() < 2) return false;

        uaLen =
            static_cast<uint64_t>(payload[offset]) |
            (static_cast<uint64_t>(payload[offset + 1]) << 8);

        offset += 2;
    }
    else if (uaLen == 0xFE) {
        if (remaining() < 4) return false;

        uaLen =
            static_cast<uint64_t>(payload[offset]) |
            (static_cast<uint64_t>(payload[offset + 1]) << 8) |
            (static_cast<uint64_t>(payload[offset + 2]) << 16) |
            (static_cast<uint64_t>(payload[offset + 3]) << 24);

        offset += 4;
    }
    else if (uaLen == 0xFF) {
        if (remaining() < 8) return false;

        uaLen = readU64();
    }

    if (uaLen > remaining()) return false;

    remoteUserAgent_.assign(
        reinterpret_cast<const char*>(
            payload.data() + offset
        ),
        static_cast<size_t>(uaLen)
    );

    offset += static_cast<size_t>(uaLen);

    if (remaining() < 4) return false;

    remoteHeight_ =
        static_cast<int32_t>(readU32());

    return true;
}

} // namespace net
} // namespace bitcoin2max
