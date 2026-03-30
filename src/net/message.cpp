// src/net/message.cpp
//
// Bitcoin 2.0max P2P message serialisation / deserialisation.
//
// SHA256 is used via the direct OpenSSL API, consistent with the rest of the
// codebase.  The deprecation warning is suppressed as in signature.cpp.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <openssl/sha.h>
#pragma GCC diagnostic pop

#include "message.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <random>

namespace bitcoin2max {
namespace net {

// ── Little-endian serialisation helpers ──────────────────────────────────────

static void writeU16LE(std::vector<uint8_t>& buf, uint16_t v) {
    buf.push_back(static_cast<uint8_t>(v));
    buf.push_back(static_cast<uint8_t>(v >> 8));
}

static void writeU32LE(std::vector<uint8_t>& buf, uint32_t v) {
    buf.push_back(static_cast<uint8_t>(v));
    buf.push_back(static_cast<uint8_t>(v >>  8));
    buf.push_back(static_cast<uint8_t>(v >> 16));
    buf.push_back(static_cast<uint8_t>(v >> 24));
}

static void writeU64LE(std::vector<uint8_t>& buf, uint64_t v) {
    for (int i = 0; i < 8; ++i)
        buf.push_back(static_cast<uint8_t>(v >> (8 * i)));
}

static void writeI32LE(std::vector<uint8_t>& buf, int32_t  v) { writeU32LE(buf, static_cast<uint32_t>(v)); }
static void writeI64LE(std::vector<uint8_t>& buf, int64_t  v) { writeU64LE(buf, static_cast<uint64_t>(v)); }

static uint16_t readU16LE(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) |
          (static_cast<uint16_t>(p[1]) << 8);
}

static uint32_t readU32LE(const uint8_t* p) {
    return  static_cast<uint32_t>(p[0])        |
           (static_cast<uint32_t>(p[1]) <<  8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

/// Write a Bitcoin variable-length string (compact-size length prefix + bytes).
static void writeVarStr(std::vector<uint8_t>& buf, const std::string& s) {
    uint64_t len = static_cast<uint64_t>(s.size());
    if (len < 0xFD) {
        buf.push_back(static_cast<uint8_t>(len));
    } else if (len <= 0xFFFF) {
        buf.push_back(0xFD);
        writeU16LE(buf, static_cast<uint16_t>(len));
    } else {
        buf.push_back(0xFE);
        writeU32LE(buf, static_cast<uint32_t>(len));
    }
    buf.insert(buf.end(), s.begin(), s.end());
}

// ── computeChecksum ───────────────────────────────────────────────────────────

std::array<uint8_t, 4> computeChecksum(const std::vector<uint8_t>& data) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    uint8_t h1[SHA256_DIGEST_LENGTH];
    uint8_t h2[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), h1);
    SHA256(h1, SHA256_DIGEST_LENGTH, h2);
#pragma GCC diagnostic pop

    std::array<uint8_t, 4> cs{};
    std::copy(h2, h2 + 4, cs.begin());
    return cs;
}

// ── MessageHeader ─────────────────────────────────────────────────────────────

std::string MessageHeader::commandStr() const {
    const char* begin = command.data();
    const char* end   = begin + command.size();
    const char* nul   = std::find(begin, end, '\0');
    return std::string(begin, nul);
}

std::vector<uint8_t> MessageHeader::serialise() const {
    std::vector<uint8_t> buf;
    buf.reserve(WIRE_SIZE);
    writeU32LE(buf, magic);
    buf.insert(buf.end(), command.begin(), command.end());
    writeU32LE(buf, length);
    buf.insert(buf.end(), checksum.begin(), checksum.end());
    assert(buf.size() == WIRE_SIZE);
    return buf;
}

MessageHeader MessageHeader::deserialise(const uint8_t* data, size_t len) {
    if (len < WIRE_SIZE) return {};
    MessageHeader hdr;
    hdr.magic  = readU32LE(data);
    std::copy(data + 4, data + 16, hdr.command.begin());
    hdr.length = readU32LE(data + 16);
    std::copy(data + 20, data + 24, hdr.checksum.begin());
    return hdr;
}

// ── NetMessage ────────────────────────────────────────────────────────────────

std::vector<uint8_t> NetMessage::serialise() const {
    auto hdrBytes = header.serialise();
    std::vector<uint8_t> out;
    out.reserve(hdrBytes.size() + payload.size());
    out.insert(out.end(), hdrBytes.begin(), hdrBytes.end());
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

NetMessage NetMessage::deserialise(const uint8_t* data, size_t len) {
    NetMessage msg;
    msg.header = MessageHeader::deserialise(data, len);
    if (len > MessageHeader::WIRE_SIZE) {
        const uint8_t* payloadPtr = data + MessageHeader::WIRE_SIZE;
        size_t payloadLen = std::min<size_t>(
            msg.header.length, len - MessageHeader::WIRE_SIZE);
        msg.payload.assign(payloadPtr, payloadPtr + payloadLen);
    }
    return msg;
}

// ── buildVersionMsg ───────────────────────────────────────────────────────────

NetMessage buildVersionMsg(int32_t bestHeight, uint16_t ourPort) {
    std::vector<uint8_t> payload;
    payload.reserve(128);

    // version (int32)
    writeI32LE(payload, network::PROTOCOL_VERSION);

    // services (uint64)
    writeU64LE(payload, network::NODE_NETWORK);

    // timestamp (int64, Unix seconds)
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    writeI64LE(payload, static_cast<int64_t>(now));

    // addr_recv: services(8) + IPv4-mapped IPv6 address(16) + port(2, big-endian)
    writeU64LE(payload, network::NODE_NETWORK);
    static const uint8_t kIPv4Mapped[16] = {
        0,0,0,0, 0,0,0,0, 0,0, 0xFF,0xFF, 0,0,0,0
    };
    payload.insert(payload.end(), std::begin(kIPv4Mapped), std::end(kIPv4Mapped));
    payload.push_back(static_cast<uint8_t>(ourPort >> 8));
    payload.push_back(static_cast<uint8_t>(ourPort));

    // addr_from: same structure (our own address)
    writeU64LE(payload, network::NODE_NETWORK);
    payload.insert(payload.end(), std::begin(kIPv4Mapped), std::end(kIPv4Mapped));
    payload.push_back(static_cast<uint8_t>(ourPort >> 8));
    payload.push_back(static_cast<uint8_t>(ourPort));

    // nonce (random uint64 — used to detect self-connections)
    std::random_device rd;
    std::mt19937_64 gen(rd());
    writeU64LE(payload, gen());

    // user_agent (var_str)
    writeVarStr(payload, network::USER_AGENT);

    // start_height (int32)
    writeI32LE(payload, bestHeight);

    // relay (1 byte, true — we want to receive relayed transactions)
    payload.push_back(1);

    // Build the header
    MessageHeader hdr;
    hdr.magic   = network::MAGIC;
    hdr.command = {};
    std::copy(CMD_VERSION, CMD_VERSION + std::strlen(CMD_VERSION),
              hdr.command.begin());
    hdr.length   = static_cast<uint32_t>(payload.size());
    hdr.checksum = computeChecksum(payload);

    return NetMessage{hdr, std::move(payload)};
}

// ── buildVerackMsg ────────────────────────────────────────────────────────────

NetMessage buildVerackMsg() {
    std::vector<uint8_t> emptyPayload;

    MessageHeader hdr;
    hdr.magic   = network::MAGIC;
    hdr.command = {};
    std::copy(CMD_VERACK, CMD_VERACK + std::strlen(CMD_VERACK),
              hdr.command.begin());
    hdr.length   = 0;
    hdr.checksum = computeChecksum(emptyPayload);

    return NetMessage{hdr, emptyPayload};
}

} // namespace net
} // namespace bitcoin2max
