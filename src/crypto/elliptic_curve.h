#pragma once
// src/crypto/elliptic_curve.h
//
// Bitcoin 2.0max — elliptic curve parameter parsing.
//
// Supports named standard curves:
//   "secp256k1"  – the curve used by Bitcoin (y² = x³ + 7  over F_p)
//   "secp256r1"  – NIST P-256              (y² = x³ − 3x + b  over F_p)
//
// Parameters follow the short Weierstrass form: y² = x³ + a·x + b (mod p).

#include <cstdint>
#include <string>
#include <vector>

namespace bitcoin2max {
namespace crypto {

/// Parsed representation of an elliptic curve over a prime field.
///
/// Curve equation: y² = x³ + a·x + b (mod p).
struct EllipticCurveParams {
    bool        valid;    ///< false if parsing / lookup failed
    std::string name;     ///< Canonical curve name (e.g. "secp256k1")

    std::vector<uint8_t> p;      ///< Field prime p (big-endian bytes)
    std::vector<uint8_t> a;      ///< Curve coefficient a (big-endian bytes)
    std::vector<uint8_t> b;      ///< Curve coefficient b (big-endian bytes)
    std::vector<uint8_t> gx;     ///< Generator G x-coordinate (big-endian bytes)
    std::vector<uint8_t> gy;     ///< Generator G y-coordinate (big-endian bytes)
    std::vector<uint8_t> order;  ///< Group order n (big-endian bytes)
    uint32_t             cofactor; ///< Cofactor h (almost always 1)
};

/// Parse a named elliptic curve and extract its domain parameters.
///
/// Supported names (case-sensitive):
///   - "secp256k1"
///   - "secp256r1"  (alias: "P-256")
///
/// \returns EllipticCurveParams; valid == false if the name is unsupported or
///          if OpenSSL fails to load the curve.
EllipticCurveParams parseEllipticCurve(const std::string& name);

} // namespace crypto
} // namespace bitcoin2max
