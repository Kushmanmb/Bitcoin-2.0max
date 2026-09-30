#pragma once
// src/crypto/block_hash.h
//
// Bitcoin 2.0max — Bitcoin block header serialization and hashing.
//
// Bitcoin block headers are serialized as:
//   version(4) | prev_block_hash(32) | merkle_root(32) | time(4) | bits(4) |
//   nonce(4)
//
// The canonical block ID is the double-SHA256 hash of this 80-byte header.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>
#include <string>
#include <vector>

namespace bitcoin2max {
namespace crypto {

using Hash256 = std::array<uint8_t, 32>;
using BlockHash = std::array<uint8_t, 32>;

/// Bitcoin block header in canonical wire format.
struct BlockHeader {
    uint32_t version{1};
    std::array<uint8_t, 32> prevHash{};
    std::array<uint8_t, 32> merkleRoot{};
    uint32_t time{0};
    uint32_t bits{0};
    uint32_t nonce{0};
};

/// Convert a block header to its 80-byte Bitcoin wire representation.
inline std::vector<uint8_t> serializeBlockHeader(const BlockHeader& header) {
    std::vector<uint8_t> raw;
    raw.reserve(80);

    auto appendU32 = [&raw](uint32_t value) {
        for (int i = 0; i < 4; ++i)
            raw.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFFu));
    };

    appendU32(header.version);
    raw.insert(raw.end(), header.prevHash.begin(), header.prevHash.end());
    raw.insert(raw.end(), header.merkleRoot.begin(), header.merkleRoot.end());
    appendU32(header.time);
    appendU32(header.bits);
    appendU32(header.nonce);

    return raw;
}

/// Compute SHA256d (double-SHA256) over arbitrary input bytes.
inline Hash256 sha256d(const uint8_t* data, size_t len) {
    uint8_t h1[SHA256_DIGEST_LENGTH];
    uint8_t h2[SHA256_DIGEST_LENGTH];

    SHA256(data, len, h1);
    SHA256(h1, SHA256_DIGEST_LENGTH, h2);

    Hash256 result{};
    std::copy(h2, h2 + SHA256_DIGEST_LENGTH, result.begin());
    return result;
}

/// Compute SHA256d over a byte vector.
inline Hash256 sha256d(const std::vector<uint8_t>& data) {
    return data.empty() ? Hash256{} : sha256d(data.data(), data.size());
}

/// Compute the canonical 256-bit hash of a block header.
inline BlockHash computeBlockHash(const BlockHeader& header) {
    return sha256d(serializeBlockHeader(header));
}

/// Compute the canonical 256-bit hash of arbitrary raw bytes.
inline Hash256 hash256(const uint8_t* data, size_t len) {
    return sha256d(data, len);
}

/// Compute the canonical 256-bit hash of a byte vector.
inline Hash256 hash256(const std::vector<uint8_t>& data) {
    return sha256d(data);
}

/// Encode a 32-byte hash as lowercase hexadecimal.
inline std::string toHex(const std::array<uint8_t, 32>& hash) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (const auto byte : hash)
        oss << std::setw(2) << static_cast<unsigned>(byte);
    return oss.str();
}

/// Return the block header hash as lowercase hexadecimal.
inline std::string blockHashHex(const BlockHeader& header) {
    return toHex(computeBlockHash(header));
}

/// Return the block header hash as lowercase hexadecimal for a raw serialized header.
inline std::string blockHashHex(const std::vector<uint8_t>& rawHeader) {
    return toHex(sha256d(rawHeader));
}

} // namespace crypto
} // namespace bitcoin2max
