#pragma once
// src/crypto/signature.h
//
// Bitcoin 2.0max — compact ECDSA signature validation.
//
// Supports the Bitcoin-standard signed-message format:
//   header (1 byte) | R (32 bytes) | S (32 bytes)  → base64-encoded (65 bytes / 88 chars)
//
// The header byte encodes the recovery ID and whether the signing key was
// compressed:
//   27-30 : uncompressed key, recovery ID = header - 27
//   31-34 : compressed key,   recovery ID = header - 31

#include <cstdint>
#include <string>
#include <vector>

namespace bitcoin2max {
namespace crypto {

/// Parsed representation of a compact Bitcoin signature.
struct SignatureInfo {
    bool                 valid;       ///< false if decoding / header check failed
    int                  recoveryId;  ///< 0–3
    bool                 compressed;  ///< true when signing key was compressed
    std::vector<uint8_t> r;           ///< 32-byte R component
    std::vector<uint8_t> s;           ///< 32-byte S component
};

/// Decode and inspect a base64-encoded compact Bitcoin signature.
/// \returns SignatureInfo; valid == false on any decoding error.
SignatureInfo parseSignature(const std::string& sigBase64);

/// Verify a Bitcoin signed-message signature.
///
/// Re-derives the signer's public key via ECDSA recovery and checks that the
/// resulting P2PKH address matches \p address.
///
/// \param address   Base58Check-encoded mainnet P2PKH address (e.g. "1A1z…")
/// \param message   UTF-8 plaintext that was signed
/// \param sigBase64 Base64-encoded 65-byte compact ECDSA signature
/// \returns true if and only if the recovered address equals \p address.
bool validateMessageSignature(const std::string& address,
                              const std::string& message,
                              const std::string& sigBase64);

} // namespace crypto
} // namespace bitcoin2max
