// tests/test_signature.cpp
//
// Unit tests for the Bitcoin 2.0max compact ECDSA signature validation module.
//
// Test coverage:
//   1. Base-64 parsing of a well-known 65-byte signature
//   2. Header decoding (compressed flag, recovery ID)
//   3. Rejection of malformed / wrong-length inputs
//   4. Round-trip sign → validateMessageSignature for a deterministic key pair

#include <catch2/catch_test_macros.hpp>

#include "crypto/signature.h"

// The low-level EC_KEY API is deprecated in OpenSSL 3.x but remains available;
// suppress the deprecation warnings in this test-only signing helper.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// OpenSSL for the signing helper used in the round-trip test
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/sha.h>

#include <array>
#include <cstring>
#include <string>
#include <vector>

using namespace bitcoin2max::crypto;

// ── Helpers used only in these tests ─────────────────────────────────────────

// The well-known compact signature from the issue tracker (65 bytes / 88 b64 chars).
static const std::string kIssueSig =
    "H23uozTk0qkJmqJz8Lj4SNBjNKpqOpqZnFO+OYI9yzRi"
    "ExFmfDMdbGVngFCOILNPxu6jtsnoJ+4cYW0PA+6EuMA=";

// Base64 encode (test helper — inverse of the production decoder)
static const char* kB64 =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64Encode(const std::vector<uint8_t>& in) {
    std::string out;
    out.reserve(((in.size() + 2) / 3) * 4);
    for (size_t i = 0; i < in.size(); i += 3) {
        uint32_t b = static_cast<uint32_t>(in[i]) << 16;
        if (i + 1 < in.size()) b |= static_cast<uint32_t>(in[i + 1]) << 8;
        if (i + 2 < in.size()) b |= static_cast<uint32_t>(in[i + 2]);
        out += kB64[(b >> 18) & 0x3F];
        out += kB64[(b >> 12) & 0x3F];
        out += (i + 1 < in.size()) ? kB64[(b >> 6) & 0x3F] : '=';
        out += (i + 2 < in.size()) ? kB64[b & 0x3F]        : '=';
    }
    return out;
}

// double-SHA256 (mirrors production code, used to build the message hash here)
static std::array<uint8_t, 32> dsha256(const uint8_t* d, size_t n) {
    std::array<uint8_t, 32> h1, h2;
    SHA256(d, n, h1.data());
    SHA256(h1.data(), 32, h2.data());
    return h2;
}

static std::array<uint8_t, 32> bitcoinMessageHash(const std::string& msg) {
    static const std::string kMagic = "Bitcoin Signed Message:\n";
    std::vector<uint8_t> buf;
    buf.push_back(static_cast<uint8_t>(kMagic.size()));
    buf.insert(buf.end(), kMagic.begin(), kMagic.end());
    buf.push_back(static_cast<uint8_t>(msg.size()));
    buf.insert(buf.end(), msg.begin(), msg.end());
    return dsha256(buf.data(), buf.size());
}

// Build a compact 65-byte Bitcoin signature from a private key + message.
// Tries recovery IDs 0–3 until the recovered key matches the signing key.
// Returns base64(header || R || S) or "" on failure.
static std::string signMessage(const std::string& privKeyHex,
                               const std::string& message) {
    // Parse private key
    BIGNUM* priv = nullptr;
    if (!BN_hex2bn(&priv, privKeyHex.c_str()) || !priv) return "";

    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    if (!group) { BN_free(priv); return ""; }

    EC_KEY* key = EC_KEY_new();
    if (!key) { EC_GROUP_free(group); BN_free(priv); return ""; }

    EC_KEY_set_group(key, group);
    EC_KEY_set_private_key(key, priv);

    // Derive compressed public key
    BN_CTX* ctx = BN_CTX_new();
    if (!ctx) { EC_KEY_free(key); EC_GROUP_free(group); BN_free(priv); return ""; }

    EC_POINT* pub = EC_POINT_new(group);
    if (!pub) { BN_CTX_free(ctx); EC_KEY_free(key); EC_GROUP_free(group); BN_free(priv); return ""; }

    EC_POINT_mul(group, pub, priv, nullptr, nullptr, ctx);
    EC_KEY_set_public_key(key, pub);
    EC_KEY_set_conv_form(key, POINT_CONVERSION_COMPRESSED);

    auto hash = bitcoinMessageHash(message);

    // Sign
    ECDSA_SIG* sig = ECDSA_do_sign(hash.data(), 32, key);
    if (!sig) {
        EC_POINT_free(pub); BN_CTX_free(ctx);
        EC_KEY_free(key); EC_GROUP_free(group); BN_free(priv);
        return "";
    }

    const BIGNUM* r;
    const BIGNUM* s;
    ECDSA_SIG_get0(sig, &r, &s);

    // Serialise R and S as 32-byte big-endian
    std::vector<uint8_t> rBytes(32, 0), sBytes(32, 0);
    BN_bn2binpad(r, rBytes.data(), 32);
    BN_bn2binpad(s, sBytes.data(), 32);

    // Determine recovery ID: find recId such that the recovered pub key matches
    // the serialised compressed public key of our signing key.
    size_t pubLen = static_cast<size_t>(i2o_ECPublicKey(key, nullptr));
    std::vector<uint8_t> expectedPub(pubLen);
    uint8_t* ptr = expectedPub.data();
    i2o_ECPublicKey(key, &ptr);

    std::string result;
    for (int recId = 0; recId <= 3; ++recId) {
        uint8_t header = static_cast<uint8_t>(31 + recId); // compressed

        std::vector<uint8_t> compact(65);
        compact[0] = header;
        std::memcpy(compact.data() + 1,  rBytes.data(), 32);
        std::memcpy(compact.data() + 33, sBytes.data(), 32);

        auto candidate = parseSignature(base64Encode(compact));
        if (!candidate.valid) continue;

        // Use validateMessageSignature indirectly: recover key and check pubkey
        BIGNUM* rb = BN_bin2bn(rBytes.data(), 32, nullptr);
        BIGNUM* sb = BN_bin2bn(sBytes.data(), 32, nullptr);

        EC_GROUP* g2  = EC_GROUP_new_by_curve_name(NID_secp256k1);
        BIGNUM*   p2  = BN_new(), *a2 = BN_new(), *bn2 = BN_new();
        BN_CTX*   c2  = BN_CTX_new();
        EC_GROUP_get_curve(g2, p2, a2, bn2, c2);

        const BIGNUM* ord = EC_GROUP_get0_order(g2);
        BIGNUM* x2 = BN_dup(rb);
        if (recId >> 1) BN_add(x2, x2, ord);
        bool xOk = BN_cmp(x2, p2) < 0;

        EC_POINT* R2 = EC_POINT_new(g2);
        bool pointOk = xOk &&
            EC_POINT_set_compressed_coordinates(g2, R2, x2, recId & 1, c2) &&
            EC_POINT_is_on_curve(g2, R2, c2);

        std::vector<uint8_t> recPub;
        if (pointOk) {
            BIGNUM* z2    = BN_bin2bn(hash.data(), 32, nullptr);
            BIGNUM* rinv  = BN_new();
            BN_mod_inverse(rinv, rb, ord, c2);
            BIGNUM* negz  = BN_new();
            BN_mod(negz, z2, ord, c2);
            if (!BN_is_zero(negz)) BN_sub(negz, ord, negz);
            BIGNUM* u1 = BN_new(), *u2 = BN_new();
            BN_mod_mul(u1, negz, rinv, ord, c2);
            BN_mod_mul(u2, sb,   rinv, ord, c2);
            EC_POINT* Q2 = EC_POINT_new(g2);
            if (EC_POINT_mul(g2, Q2, u1, R2, u2, c2) &&
                !EC_POINT_is_at_infinity(g2, Q2)) {
                EC_KEY* rk = EC_KEY_new();
                EC_KEY_set_group(rk, g2);
                EC_KEY_set_public_key(rk, Q2);
                EC_KEY_set_conv_form(rk, POINT_CONVERSION_COMPRESSED);
                size_t rl = static_cast<size_t>(i2o_ECPublicKey(rk, nullptr));
                recPub.resize(rl);
                uint8_t* rptr = recPub.data();
                i2o_ECPublicKey(rk, &rptr);
                EC_KEY_free(rk);
            }
            EC_POINT_free(Q2);
            BN_free(z2); BN_free(rinv); BN_free(negz); BN_free(u1); BN_free(u2);
        }

        EC_POINT_free(R2);
        BN_free(x2); BN_free(p2); BN_free(a2); BN_free(bn2);
        BN_CTX_free(c2); EC_GROUP_free(g2);
        BN_free(rb); BN_free(sb);

        if (recPub == expectedPub) {
            result = base64Encode(compact);
            break;
        }
    }

    ECDSA_SIG_free(sig);
    EC_POINT_free(pub);
    BN_CTX_free(ctx);
    EC_KEY_free(key);
    EC_GROUP_free(group);
    BN_free(priv);
    return result;
}

// Base58Check → address (lightweight: just checks it is non-empty here)
// The full derivation lives in signature.cpp; we rely on validateMessageSignature.

// ── Tests ─────────────────────────────────────────────────────────────────────

TEST_CASE("parseSignature: issue-tracker signature decodes to 65 bytes", "[signature]") {
    auto info = parseSignature(kIssueSig);
    REQUIRE(info.valid);
    REQUIRE(info.r.size() == 32u);
    REQUIRE(info.s.size() == 32u);
}

TEST_CASE("parseSignature: header encodes compressed flag and recovery ID", "[signature]") {
    auto info = parseSignature(kIssueSig);
    REQUIRE(info.valid);
    // Header byte 0x1F = 31 → compressed key, recovery ID 0
    REQUIRE(info.compressed == true);
    REQUIRE(info.recoveryId == 0);
}

TEST_CASE("parseSignature: rejects empty string", "[signature]") {
    REQUIRE_FALSE(parseSignature("").valid);
}

TEST_CASE("parseSignature: rejects wrong-length input", "[signature]") {
    // A base64 string that decodes to fewer than 65 bytes
    std::string wrongLengthSig = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
    REQUIRE_FALSE(parseSignature(wrongLengthSig).valid);
}

TEST_CASE("parseSignature: rejects invalid header byte", "[signature]") {
    // Build a syntactically correct 65-byte blob but with header 0x00 (< 27)
    std::vector<uint8_t> blob(65, 0xAA);
    blob[0] = 0x00;
    REQUIRE_FALSE(parseSignature(base64Encode(blob)).valid);
}

TEST_CASE("validateMessageSignature: round-trip with deterministic key", "[signature]") {
    // Private key: 1  (well-known test scalar for secp256k1)
    const std::string privKeyHex =
        "0000000000000000000000000000000000000000000000000000000000000001";
    const std::string message = "Bitcoin 2.0max test message";

    // P2PKH address for privKey = 1 (secp256k1 compressed public key = generator G):
    //   pubkey  = 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
    //   address = 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
    const std::string expectedAddress = "1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH";

    std::string sig = signMessage(privKeyHex, message);
    REQUIRE_FALSE(sig.empty()); // signing must succeed

    bool ok = validateMessageSignature(expectedAddress, message, sig);
    REQUIRE(ok);
}

TEST_CASE("validateMessageSignature: wrong address returns false", "[signature]") {
    const std::string privKeyHex =
        "0000000000000000000000000000000000000000000000000000000000000001";
    const std::string message    = "Bitcoin 2.0max test message";
    const std::string wrongAddr  = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa"; // genesis coinbase

    std::string sig = signMessage(privKeyHex, message);
    REQUIRE_FALSE(sig.empty());

    REQUIRE_FALSE(validateMessageSignature(wrongAddr, message, sig));
}

TEST_CASE("validateMessageSignature: wrong message returns false", "[signature]") {
    const std::string privKeyHex =
        "0000000000000000000000000000000000000000000000000000000000000001";
    const std::string message        = "Bitcoin 2.0max test message";
    const std::string differentMsg   = "A completely different message";
    const std::string expectedAddress = "1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH";

    std::string sig = signMessage(privKeyHex, message);
    REQUIRE_FALSE(sig.empty());

    REQUIRE_FALSE(validateMessageSignature(expectedAddress, differentMsg, sig));
}

TEST_CASE("validateMessageSignature: rejects empty signature", "[signature]") {
    REQUIRE_FALSE(validateMessageSignature("1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH", "test", ""));
}

TEST_CASE("validateMessageSignature: rejects invalid base64 signature", "[signature]") {
    REQUIRE_FALSE(validateMessageSignature("1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH", "test", "!!!not_valid_base64!!!"));
}

TEST_CASE("validateMessageSignature: rejects empty address", "[signature]") {
    REQUIRE_FALSE(validateMessageSignature("", "test", kIssueSig));
}

TEST_CASE("invalid signatures rejected consistently by parseSignature and validateMessageSignature", "[signature]") {
    // Demonstrate that parseSignature.valid must be true before calling
    // validateMessageSignature, and that bad input is consistently rejected.
    auto infoEmpty = parseSignature("");
    REQUIRE_FALSE(infoEmpty.valid);
    REQUIRE_FALSE(validateMessageSignature("1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH", "test", ""));

    auto infoShort = parseSignature("AAAA");
    REQUIRE_FALSE(infoShort.valid);
    REQUIRE_FALSE(validateMessageSignature("1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH", "test", "AAAA"));
}

#pragma GCC diagnostic pop
