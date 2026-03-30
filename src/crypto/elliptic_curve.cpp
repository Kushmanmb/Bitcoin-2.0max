// src/crypto/elliptic_curve.cpp
//
// Bitcoin 2.0max — elliptic curve parameter parsing using OpenSSL.
//
// Implements parseEllipticCurve():
//   • Maps a human-readable curve name to the corresponding OpenSSL NID.
//   • Loads the EC_GROUP for that NID.
//   • Extracts the prime field parameters (p, a, b), the generator coordinates
//     (Gx, Gy), the group order (n), and the cofactor (h).
//   • Returns all values as big-endian byte vectors inside EllipticCurveParams.

#include "elliptic_curve.h"

// The low-level EC_GROUP / EC_POINT API is deprecated in OpenSSL 3.x but is
// still available and is the portable path for parameter extraction across
// OpenSSL 1.x and 3.x.  Suppress the deprecation noise to keep the build
// warning-clean.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>

#include <cstddef>
#include <map>

namespace bitcoin2max {
namespace crypto {

// ── Internal helpers ──────────────────────────────────────────────────────────

/// Convert a BIGNUM to a big-endian byte vector of exactly \p byteLen bytes.
static std::vector<uint8_t> bnToBytes(const BIGNUM* bn, int byteLen) {
    std::vector<uint8_t> out(static_cast<size_t>(byteLen), 0u);
    BN_bn2binpad(bn, out.data(), byteLen);
    return out;
}

// ── Name → NID table ─────────────────────────────────────────────────────────

static const std::map<std::string, int> kCurveNids = {
    {"secp256k1", NID_secp256k1},
    {"secp256r1", NID_X9_62_prime256v1},
    {"P-256",     NID_X9_62_prime256v1},
};

// ── Public API ────────────────────────────────────────────────────────────────

EllipticCurveParams parseEllipticCurve(const std::string& name) {
    EllipticCurveParams out{};

    // Look up the NID for the requested curve name.
    auto it = kCurveNids.find(name);
    if (it == kCurveNids.end())
        return out; // valid stays false

    int nid = it->second;

    // Load the EC_GROUP.
    EC_GROUP* group = EC_GROUP_new_by_curve_name(nid);
    if (!group)
        return out;

    BN_CTX* ctx = BN_CTX_new();
    if (!ctx) {
        EC_GROUP_free(group);
        return out;
    }

    // Extract curve parameters: p, a, b.
    BIGNUM* p  = BN_new();
    BIGNUM* a  = BN_new();
    BIGNUM* b  = BN_new();
    if (!EC_GROUP_get_curve(group, p, a, b, ctx)) {
        BN_free(p); BN_free(a); BN_free(b);
        BN_CTX_free(ctx);
        EC_GROUP_free(group);
        return out;
    }

    // Extract generator point G.
    const EC_POINT* G = EC_GROUP_get0_generator(group);
    BIGNUM* gx = BN_new();
    BIGNUM* gy = BN_new();
    if (!G || !EC_POINT_get_affine_coordinates(group, G, gx, gy, ctx)) {
        BN_free(gx); BN_free(gy);
        BN_free(p);  BN_free(a); BN_free(b);
        BN_CTX_free(ctx);
        EC_GROUP_free(group);
        return out;
    }

    // Extract group order n.
    const BIGNUM* order = EC_GROUP_get0_order(group);

    // Extract cofactor h.
    const BIGNUM* cofactorBn = EC_GROUP_get0_cofactor(group);
    BN_ULONG cofactorWord = cofactorBn ? BN_get_word(cofactorBn) : 1UL;
    // BN_get_word returns ULONG_MAX on overflow; standard curves have cofactor 1.
    uint32_t cofactor = (cofactorWord <= UINT32_MAX)
                            ? static_cast<uint32_t>(cofactorWord)
                            : 1u;

    // Determine byte length from the field prime.
    int byteLen = BN_num_bytes(p);

    // Populate output struct.
    out.valid    = true;
    out.name     = name;
    out.p        = bnToBytes(p,     byteLen);
    out.a        = bnToBytes(a,     byteLen);
    out.b        = bnToBytes(b,     byteLen);
    out.gx       = bnToBytes(gx,    byteLen);
    out.gy       = bnToBytes(gy,    byteLen);
    out.order    = bnToBytes(order, BN_num_bytes(order));
    out.cofactor = cofactor;

    BN_free(gx); BN_free(gy);
    BN_free(p);  BN_free(a); BN_free(b);
    BN_CTX_free(ctx);
    EC_GROUP_free(group);

    return out;
}

} // namespace crypto
} // namespace bitcoin2max

#pragma GCC diagnostic pop
