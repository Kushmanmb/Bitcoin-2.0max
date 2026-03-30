#pragma once
// src/net/message.h
//
// Bitcoin 2.0max P2P network message framing.
//
// Wire format (24-byte header + variable payload):
//   [magic:4][command:12][length:4][checksum:4][payload:N]
//
// All multi-byte integer fields are little-endian unless noted otherwise.

#include "bitcoin2max/params.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace bitcoin2max {
namespace net {

// ── Command name constants ────────────────────────────────────────────────────

static constexpr const char* CMD_VERSION = "version";
static constexpr const char* CMD_VERACK  = "verack";

// ── Message header (24 bytes on the wire) ────────────────────────────────────

/// Fixed-length header that precedes every P2P message.
struct MessageHeader {
    uint32_t               magic{network::MAGIC};
    std::array<char, 12>   command{};   ///< Null-padded ASCII command name
    uint32_t               length{0};   ///< Payload length in bytes
    std::array<uint8_t, 4> checksum{};  ///< SHA256d(payload)[0..3]

    static constexpr size_t WIRE_SIZE = 24;

    /// Return the command as a null-trimmed std::string.
    std::string commandStr() const;

    /// Serialise to 24 little-endian bytes.
    std::vector<uint8_t> serialise() const;

    /// Deserialise from raw bytes.  Returns a default-constructed header on
    /// error (e.g. insufficient data).
    static MessageHeader deserialise(const uint8_t* data, size_t len);
};

// ── Full network message ──────────────────────────────────────────────────────

struct NetMessage {
    MessageHeader         header;
    std::vector<uint8_t>  payload;

    /// Serialise header + payload into a contiguous byte vector.
    std::vector<uint8_t> serialise() const;

    /// Deserialise a complete message (header + payload) from raw bytes.
    static NetMessage deserialise(const uint8_t* data, size_t len);
};

// ── Helpers ───────────────────────────────────────────────────────────────────

/// Compute SHA256d(data)[0..3] — the 4-byte message checksum.
std::array<uint8_t, 4> computeChecksum(const std::vector<uint8_t>& data);

/// Build a `version` message.
/// @param bestHeight  Our current best chain height (sent as start_height).
/// @param ourPort     Our P2P listening port (written into addr_from).
NetMessage buildVersionMsg(int32_t bestHeight, uint16_t ourPort);

/// Build a zero-payload `verack` message.
NetMessage buildVerackMsg();

} // namespace net
} // namespace bitcoin2max
