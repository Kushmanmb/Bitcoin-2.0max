// src/crypto/signature.cpp
//
// Bitcoin 2.0max — compact ECDSA signature validation using OpenSSL.
//
// Implements:
//   • Base64 decoding
//   • Bitcoin signed-message hash  (double-SHA256 of magic-prefixed message)
//   • secp256k1 public-key recovery from (r, s, recoveryId)
//   • P2PKH address derivation    (SHA256 → RIPEMD-160 → Base58Check)
//   • Full validateMessageSignature() pipeline

#include "signature.h"

// The low-level EC_KEY API (deprecated in OpenSSL 3.x) is the only portable
// way to handle secp256k1 key recovery and serialization across 1.x and 3.x.
// Suppress the deprecation noise so the build stays warning-clean.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/sha.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <memory>
#include <vector>

namespace bitcoin2max {
namespace crypto {

// ── Base64 ────────────────────────────────────────────────────────────────────

static const std::string kBase64Chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static std::vector<uint8_t> base64Decode(const std::string& encoded) {
    std::vector<uint8_t> out;
    out.reserve(encoded.size() * 3 / 4);

    int val  = 0;
    int valb = -8;
    for (unsigned char c : encoded) {
        if (c == '=') break;
        auto pos = kBase64Chars.find(static_cast<char>(c));
        if (pos == std::string::npos) return {}; // invalid character
        val   = (val << 6) + static_cast<int>(pos);
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

// ── Hashing helpers ───────────────────────────────────────────────────────────

static std::array<uint8_t, 32> sha256Once(const uint8_t* data, size_t len) {
    std::array<uint8_t, 32> digest;
    SHA256(data, len, digest.data());
    return digest;
}

static std::array<uint8_t, 32> doubleSHA256(const uint8_t* data, size_t len) {
    auto h1 = sha256Once(data, len);
    return sha256Once(h1.data(), 32);
}

// RIPEMD-160 via EVP (compatible with both OpenSSL 1.1.x and 3.x).
static std::array<uint8_t, 20> ripemd160(const uint8_t* data, size_t len) {
    std::array<uint8_t, 20> out{};
    unsigned int outLen = static_cast<unsigned int>(out.size());

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_get_digestbyname("RIPEMD160");
    if (ctx && md) {
        EVP_DigestInit_ex(ctx, md, nullptr);
        EVP_DigestUpdate(ctx, data, len);
        EVP_DigestFinal_ex(ctx, out.data(), &outLen);
    }
    EVP_MD_CTX_free(ctx);
    return out;
}

// ── Varint (Bitcoin compact size) ─────────────────────────────────────────────

static void appendVarint(std::vector<uint8_t>& buf, size_t n) {
    if (n < 0xFD) {
        buf.push_back(static_cast<uint8_t>(n));
    } else if (n <= 0xFFFF) {
        buf.push_back(0xFD);
        buf.push_back(static_cast<uint8_t>(n & 0xFF));
        buf.push_back(static_cast<uint8_t>((n >> 8) & 0xFF));
    } else if (n <= 0xFFFFFFFF) {
        buf.push_back(0xFE);
        for (int i = 0; i < 4; ++i)
            buf.push_back(static_cast<uint8_t>((n >> (8 * i)) & 0xFF));
    } else {
        buf.push_back(0xFF);
        for (int i = 0; i < 8; ++i)
            buf.push_back(static_cast<uint8_t>((n >> (8 * i)) & 0xFF));
    }
}

// ── Bitcoin signed-message hash ───────────────────────────────────────────────

static std::array<uint8_t, 32> bitcoinMessageHash(const std::string& message) {
    static const std::string kMagic = "Bitcoin Signed Message:\n";
    std::vector<uint8_t> data;
    data.reserve(2 + kMagic.size() + 4 + message.size());

    appendVarint(data, kMagic.size());
    data.insert(data.end(), kMagic.begin(), kMagic.end());
    appendVarint(data, message.size());
    data.insert(data.end(), message.begin(), message.end());

    return doubleSHA256(data.data(), data.size());
}

// ── ECDSA public-key recovery (secp256k1) ────────────────────────────────────

// Recover the EC public key from (r, s, recoveryId, hash).
// Returns nullptr on any failure; caller owns the returned EC_KEY*.
static EC_KEY* recoverPublicKey(const BIGNUM* r,
                                const BIGNUM* s,
                                int           recId,
                                const uint8_t* hash32,
                                bool           compressed) {
    if (recId < 0 || recId > 3) return nullptr;

    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    if (!group) return nullptr;

    BN_CTX* ctx = BN_CTX_new();
    if (!ctx) { EC_GROUP_free(group); return nullptr; }

    const BIGNUM* order = EC_GROUP_get0_order(group);

    // Retrieve field prime p
    BIGNUM* p  = BN_new();
    BIGNUM* a  = BN_new();
    BIGNUM* bn = BN_new();
    EC_GROUP_get_curve(group, p, a, bn, ctx);
    BN_free(a);
    BN_free(bn);

    // x = r + (recId / 2) * order
    BIGNUM* x = BN_dup(r);
    if (recId >> 1) BN_add(x, x, order);

    // x must be in [0, p)
    if (BN_cmp(x, p) >= 0) {
        BN_free(x); BN_free(p);
        BN_CTX_free(ctx); EC_GROUP_free(group);
        return nullptr;
    }
    BN_free(p);

    // Decompress x into curve point R with parity (recId & 1)
    EC_POINT* R = EC_POINT_new(group);
    if (!EC_POINT_set_compressed_coordinates(group, R, x, recId & 1, ctx) ||
        !EC_POINT_is_on_curve(group, R, ctx)) {
        EC_POINT_free(R); BN_free(x);
        BN_CTX_free(ctx); EC_GROUP_free(group);
        return nullptr;
    }
    BN_free(x);

    // z = message hash as bignum
    BIGNUM* z = BN_bin2bn(hash32, 32, nullptr);

    // r_inv = r^-1 mod order
    BIGNUM* r_inv = BN_new();
    BN_mod_inverse(r_inv, r, order, ctx);

    // u1 = (-z * r_inv) mod order  →  negate z first
    BIGNUM* neg_z = BN_new();
    BN_mod(neg_z, z, order, ctx);
    if (!BN_is_zero(neg_z)) BN_sub(neg_z, order, neg_z);

    BIGNUM* u1 = BN_new();
    BN_mod_mul(u1, neg_z, r_inv, order, ctx);

    // u2 = (s * r_inv) mod order
    BIGNUM* u2 = BN_new();
    BN_mod_mul(u2, s, r_inv, order, ctx);

    // Q = u1*G + u2*R
    EC_POINT* Q = EC_POINT_new(group);
    bool ok = (EC_POINT_mul(group, Q, u1, R, u2, ctx) == 1) &&
              (!EC_POINT_is_at_infinity(group, Q));

    EC_POINT_free(R);
    BN_free(z); BN_free(r_inv); BN_free(neg_z); BN_free(u1); BN_free(u2);
    BN_CTX_free(ctx);

    if (!ok) {
        EC_POINT_free(Q);
        EC_GROUP_free(group);
        return nullptr;
    }

    EC_KEY* key = EC_KEY_new();
    EC_KEY_set_group(key, group);
    EC_KEY_set_public_key(key, Q);
    EC_KEY_set_conv_form(key, compressed ? POINT_CONVERSION_COMPRESSED
                                         : POINT_CONVERSION_UNCOMPRESSED);

    EC_POINT_free(Q);
    EC_GROUP_free(group);
    return key;
}

// ── Base58 encoding ───────────────────────────────────────────────────────────

static const char* kBase58Chars =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

static std::string base58Encode(const std::vector<uint8_t>& data) {
    // Count leading zero bytes → each maps to '1' in Base58
    int zeros = 0;
    for (uint8_t b : data) {
        if (b != 0) break;
        ++zeros;
    }

    BIGNUM*  bn   = BN_bin2bn(data.data(), static_cast<int>(data.size()), nullptr);
    BIGNUM*  div  = BN_new();
    BIGNUM*  rem  = BN_new();
    BIGNUM*  base = BN_new();
    BN_CTX*  ctx  = BN_CTX_new();
    BN_set_word(base, 58);

    std::string rev;
    while (!BN_is_zero(bn)) {
        BN_div(div, rem, bn, base, ctx);
        rev += kBase58Chars[BN_get_word(rem)];
        BN_copy(bn, div);
    }

    BN_free(bn); BN_free(div); BN_free(rem); BN_free(base); BN_CTX_free(ctx);

    for (int i = 0; i < zeros; ++i) rev += '1';
    std::reverse(rev.begin(), rev.end());
    return rev;
}

// ── P2PKH address from EC public key ─────────────────────────────────────────

static std::string pubkeyToAddress(EC_KEY* key) {
    // 1. Serialise public key (respects POINT_CONVERSION_* form set on key)
    size_t   len = static_cast<size_t>(i2o_ECPublicKey(key, nullptr));
    std::vector<uint8_t> pub(len);
    uint8_t* ptr = pub.data();
    i2o_ECPublicKey(key, &ptr);

    // 2. hash160 = RIPEMD160(SHA256(pubkey))
    auto sha     = sha256Once(pub.data(), pub.size());
    auto h160    = ripemd160(sha.data(), 32);

    // 3. Version payload: 0x00 || hash160
    std::vector<uint8_t> payload;
    payload.push_back(0x00);
    payload.insert(payload.end(), h160.begin(), h160.end());

    // 4. Checksum = first 4 bytes of double-SHA256(payload)
    auto checksum = doubleSHA256(payload.data(), payload.size());
    payload.insert(payload.end(), checksum.begin(), checksum.begin() + 4);

    return base58Encode(payload);
}

// ── Public API ────────────────────────────────────────────────────────────────

SignatureInfo parseSignature(const std::string& sigBase64) {
    SignatureInfo info{};

    auto bytes = base64Decode(sigBase64);
    if (bytes.size() != 65) return info; // valid stays false

    uint8_t header = bytes[0];
    if (header < 27 || header > 34) return info;

    info.valid      = true;
    info.compressed = (header >= 31);
    info.recoveryId = info.compressed ? (header - 31) : (header - 27);
    info.r.assign(bytes.begin() + 1,  bytes.begin() + 33);
    info.s.assign(bytes.begin() + 33, bytes.end());
    return info;
}

bool validateMessageSignature(const std::string& address,
                              const std::string& message,
                              const std::string& sigBase64) {
    auto info = parseSignature(sigBase64);
    if (!info.valid) return false;

    auto hash = bitcoinMessageHash(message);

    BIGNUM* r = BN_bin2bn(info.r.data(), 32, nullptr);
    BIGNUM* s = BN_bin2bn(info.s.data(), 32, nullptr);

    EC_KEY* key = recoverPublicKey(r, s, info.recoveryId,
                                   hash.data(), info.compressed);
    BN_free(r);
    BN_free(s);

    if (!key) return false;

    std::string recovered = pubkeyToAddress(key);
    EC_KEY_free(key);

    return recovered == address;
}

} // namespace crypto
} // namespace bitcoin2max

#pragma GCC diagnostic pop
