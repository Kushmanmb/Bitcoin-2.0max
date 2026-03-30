// tests/test_elliptic_curve.cpp
//
// Unit tests for Bitcoin 2.0max elliptic curve parsing and Fibonacci utility.
//
// Test coverage:
//   Fibonacci:
//     1. Base cases (n = 0, n = 1) return n
//     2. Small recursive values (n = 2 … 10)
//     3. Negative input throws std::invalid_argument
//
//   parseEllipticCurve:
//     4. Unsupported / empty name returns invalid result
//     5. "secp256k1" parse succeeds with correct 32-byte field parameters
//     6. "secp256r1" / "P-256" alias parse succeeds and produces the same params
//     7. Generator and order lengths match field prime length
//     8. Cofactor is 1 for both standard curves

#include <catch2/catch_test_macros.hpp>

#include "bitcoin2max/math.h"
#include "crypto/elliptic_curve.h"

#include <stdexcept>

using namespace bitcoin2max;
using namespace bitcoin2max::crypto;

// ── Fibonacci tests ───────────────────────────────────────────────────────────

TEST_CASE("fibonacci: base cases return n", "[fibonacci]") {
    REQUIRE(utils::fibonacci(0) == 0u);
    REQUIRE(utils::fibonacci(1) == 1u);
}

TEST_CASE("fibonacci: small values follow the recurrence", "[fibonacci]") {
    REQUIRE(utils::fibonacci(2)  == 1u);
    REQUIRE(utils::fibonacci(3)  == 2u);
    REQUIRE(utils::fibonacci(4)  == 3u);
    REQUIRE(utils::fibonacci(5)  == 5u);
    REQUIRE(utils::fibonacci(6)  == 8u);
    REQUIRE(utils::fibonacci(7)  == 13u);
    REQUIRE(utils::fibonacci(8)  == 21u);
    REQUIRE(utils::fibonacci(9)  == 34u);
    REQUIRE(utils::fibonacci(10) == 55u);
}

TEST_CASE("fibonacci: negative input throws invalid_argument", "[fibonacci]") {
    REQUIRE_THROWS_AS(utils::fibonacci(-1), std::invalid_argument);
    REQUIRE_THROWS_AS(utils::fibonacci(-100), std::invalid_argument);
}

// ── parseEllipticCurve tests ──────────────────────────────────────────────────

TEST_CASE("parseEllipticCurve: unknown name returns invalid", "[elliptic_curve]") {
    auto r = parseEllipticCurve("notacurve");
    REQUIRE_FALSE(r.valid);
}

TEST_CASE("parseEllipticCurve: empty string returns invalid", "[elliptic_curve]") {
    auto r = parseEllipticCurve("");
    REQUIRE_FALSE(r.valid);
}

TEST_CASE("parseEllipticCurve: secp256k1 parse succeeds", "[elliptic_curve]") {
    auto r = parseEllipticCurve("secp256k1");
    REQUIRE(r.valid);
    REQUIRE(r.name == "secp256k1");
}

TEST_CASE("parseEllipticCurve: secp256k1 has 32-byte (256-bit) field parameters", "[elliptic_curve]") {
    auto r = parseEllipticCurve("secp256k1");
    REQUIRE(r.valid);
    // secp256k1 prime is a 256-bit number → 32 bytes
    REQUIRE(r.p.size()  == 32u);
    REQUIRE(r.a.size()  == 32u);
    REQUIRE(r.b.size()  == 32u);
    REQUIRE(r.gx.size() == 32u);
    REQUIRE(r.gy.size() == 32u);
}

TEST_CASE("parseEllipticCurve: secp256k1 order is 32 bytes", "[elliptic_curve]") {
    auto r = parseEllipticCurve("secp256k1");
    REQUIRE(r.valid);
    REQUIRE(r.order.size() == 32u);
}

TEST_CASE("parseEllipticCurve: secp256k1 cofactor is 1", "[elliptic_curve]") {
    auto r = parseEllipticCurve("secp256k1");
    REQUIRE(r.valid);
    REQUIRE(r.cofactor == 1u);
}

TEST_CASE("parseEllipticCurve: secp256k1 coefficient a is zero (y²=x³+7)", "[elliptic_curve]") {
    auto r = parseEllipticCurve("secp256k1");
    REQUIRE(r.valid);
    // secp256k1: a = 0
    for (uint8_t byte : r.a)
        REQUIRE(byte == 0u);
    // secp256k1: b = 7 (last byte of big-endian 32-byte vector)
    REQUIRE(r.b.back() == 7u);
}

TEST_CASE("parseEllipticCurve: secp256r1 parse succeeds", "[elliptic_curve]") {
    auto r = parseEllipticCurve("secp256r1");
    REQUIRE(r.valid);
    REQUIRE(r.name == "secp256r1");
}

TEST_CASE("parseEllipticCurve: P-256 alias returns same parameters as secp256r1", "[elliptic_curve]") {
    auto r1 = parseEllipticCurve("secp256r1");
    auto r2 = parseEllipticCurve("P-256");
    REQUIRE(r1.valid);
    REQUIRE(r2.valid);
    REQUIRE(r1.p     == r2.p);
    REQUIRE(r1.a     == r2.a);
    REQUIRE(r1.b     == r2.b);
    REQUIRE(r1.gx    == r2.gx);
    REQUIRE(r1.gy    == r2.gy);
    REQUIRE(r1.order == r2.order);
    REQUIRE(r1.cofactor == r2.cofactor);
}

TEST_CASE("parseEllipticCurve: secp256r1 cofactor is 1", "[elliptic_curve]") {
    auto r = parseEllipticCurve("secp256r1");
    REQUIRE(r.valid);
    REQUIRE(r.cofactor == 1u);
}
