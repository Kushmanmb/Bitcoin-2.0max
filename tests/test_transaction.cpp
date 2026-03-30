// tests/test_transaction.cpp
//
// Unit tests for the Bitcoin 2.0max transaction validation module.
//
// Test coverage:
//   1.  validateTransaction: accepts valid version-1 and version-2 transactions
//   2.  validateTransaction: rejects version 0 and version 3
//   3.  validateTransaction: rejects transaction with no inputs
//   4.  validateTransaction: rejects transaction with no outputs
//   5.  validateTransaction: rejects single output exceeding MAX_MONEY_SATOSHIS
//   6.  validateTransaction: rejects total output exceeding MAX_MONEY_SATOSHIS
//   7.  validateTransaction: accepts total output equal to MAX_MONEY_SATOSHIS
//   8.  loadTransactions: successfully loads transaction_data.dat
//   9.  loadTransactions: returns 3 transactions with correct fields
//   10. loadTransactions: returns error for nonexistent file
//   11. loadTransactions: returns error for invalid version
//   12. loadTransactions: returns error for IN before TX
//   13. loadTransactions: returns error for OUT before TX
//   14. loadTransactions: returns error for unknown keyword

#include <catch2/catch_test_macros.hpp>

#include "bitcoin2max/transaction.h"
#include "bitcoin2max/params.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

using namespace bitcoin2max;

// Path to the sample .dat file installed alongside the test binary.
// The CMake test runner sets the working directory to the build tree, so we
// use the source path via a compile-time constant injected by CMakeLists.txt.
#ifndef TRANSACTION_DAT_PATH
#  define TRANSACTION_DAT_PATH "transaction_data.dat"
#endif

// ── Helpers ───────────────────────────────────────────────────────────────────

static Transaction makeMinimalTx(uint32_t version = 1) {
    Transaction tx;
    tx.version  = version;
    tx.locktime = 0;

    TxInput inp;
    inp.prevTxid.fill(0);
    inp.prevIndex = 0;
    inp.scriptSig = {0x04, 0xde, 0xad, 0xbe, 0xef};
    inp.sequence  = 0xFFFFFFFF;
    tx.inputs.push_back(inp);

    TxOutput out;
    out.valueSatoshis = 1'000'000;
    out.scriptPubKey  = {0x76, 0xa9, 0x14};
    tx.outputs.push_back(out);

    return tx;
}

// Write a small .dat file to a temp path and return that path.
static std::string writeTempDat(const std::string& content) {
    std::string path = "/tmp/test_tx_" +
                       std::to_string(reinterpret_cast<uintptr_t>(&content)) +
                       ".dat";
    std::ofstream f(path);
    f << content;
    return path;
}

// ── validateTransaction tests ─────────────────────────────────────────────────

TEST_CASE("validateTransaction: accepts valid version-1 transaction", "[transaction]") {
    auto tx = makeMinimalTx(1);
    auto res = validateTransaction(tx);
    REQUIRE(res.valid);
    REQUIRE(res.error.empty());
}

TEST_CASE("validateTransaction: accepts valid version-2 transaction", "[transaction]") {
    auto tx = makeMinimalTx(2);
    auto res = validateTransaction(tx);
    REQUIRE(res.valid);
    REQUIRE(res.error.empty());
}

TEST_CASE("validateTransaction: rejects version 0", "[transaction]") {
    auto tx = makeMinimalTx(0);
    auto res = validateTransaction(tx);
    REQUIRE_FALSE(res.valid);
    REQUIRE_FALSE(res.error.empty());
}

TEST_CASE("validateTransaction: rejects version 3", "[transaction]") {
    auto tx = makeMinimalTx(3);
    auto res = validateTransaction(tx);
    REQUIRE_FALSE(res.valid);
    REQUIRE_FALSE(res.error.empty());
}

TEST_CASE("validateTransaction: rejects transaction with no inputs", "[transaction]") {
    auto tx = makeMinimalTx();
    tx.inputs.clear();
    auto res = validateTransaction(tx);
    REQUIRE_FALSE(res.valid);
    REQUIRE_FALSE(res.error.empty());
}

TEST_CASE("validateTransaction: rejects transaction with no outputs", "[transaction]") {
    auto tx = makeMinimalTx();
    tx.outputs.clear();
    auto res = validateTransaction(tx);
    REQUIRE_FALSE(res.valid);
    REQUIRE_FALSE(res.error.empty());
}

TEST_CASE("validateTransaction: rejects output exceeding MAX_MONEY_SATOSHIS", "[transaction]") {
    auto tx = makeMinimalTx();
    tx.outputs[0].valueSatoshis = consensus::MAX_MONEY_SATOSHIS + 1;
    auto res = validateTransaction(tx);
    REQUIRE_FALSE(res.valid);
    REQUIRE_FALSE(res.error.empty());
}

TEST_CASE("validateTransaction: rejects total output exceeding MAX_MONEY_SATOSHIS", "[transaction]") {
    auto tx = makeMinimalTx();
    // Two outputs that individually fit but overflow together
    tx.outputs[0].valueSatoshis = consensus::MAX_MONEY_SATOSHIS;
    TxOutput out2;
    out2.valueSatoshis = 1;
    out2.scriptPubKey  = {0x76};
    tx.outputs.push_back(out2);
    auto res = validateTransaction(tx);
    REQUIRE_FALSE(res.valid);
    REQUIRE_FALSE(res.error.empty());
}

TEST_CASE("validateTransaction: accepts total output equal to MAX_MONEY_SATOSHIS", "[transaction]") {
    auto tx = makeMinimalTx();
    tx.outputs[0].valueSatoshis = consensus::MAX_MONEY_SATOSHIS;
    auto res = validateTransaction(tx);
    REQUIRE(res.valid);
}

// ── loadTransactions tests ────────────────────────────────────────────────────

TEST_CASE("loadTransactions: loads transaction_data.dat successfully", "[transaction]") {
    std::string err;
    auto txs = loadTransactions(TRANSACTION_DAT_PATH, err);
    REQUIRE(err.empty());
    REQUIRE_FALSE(txs.empty());
}

TEST_CASE("loadTransactions: transaction_data.dat contains 3 transactions", "[transaction]") {
    std::string err;
    auto txs = loadTransactions(TRANSACTION_DAT_PATH, err);
    REQUIRE(err.empty());
    REQUIRE(txs.size() == 3u);
}

TEST_CASE("loadTransactions: first transaction is a coinbase", "[transaction]") {
    std::string err;
    auto txs = loadTransactions(TRANSACTION_DAT_PATH, err);
    REQUIRE(err.empty());
    REQUIRE(txs.size() >= 1u);

    const auto& coinbase = txs[0];
    REQUIRE(coinbase.version == 1u);
    REQUIRE(coinbase.locktime == 0u);
    REQUIRE(coinbase.inputs.size() == 1u);
    REQUIRE(coinbase.outputs.size() == 1u);

    // Coinbase: prev_txid all zeros, prev_index = 0xFFFFFFFF
    for (auto b : coinbase.inputs[0].prevTxid) REQUIRE(b == 0);
    REQUIRE(coinbase.inputs[0].prevIndex == 0xFFFFFFFFu);

    // Block reward = 50 BTC = 5 000 000 000 satoshis
    REQUIRE(coinbase.outputs[0].valueSatoshis == 5'000'000'000ULL);
}

TEST_CASE("loadTransactions: second transaction has two outputs", "[transaction]") {
    std::string err;
    auto txs = loadTransactions(TRANSACTION_DAT_PATH, err);
    REQUIRE(err.empty());
    REQUIRE(txs.size() >= 2u);

    const auto& tx = txs[1];
    REQUIRE(tx.version == 1u);
    REQUIRE(tx.inputs.size() == 1u);
    REQUIRE(tx.outputs.size() == 2u);
    REQUIRE(tx.outputs[0].valueSatoshis == 100'000'000ULL);
    REQUIRE(tx.outputs[1].valueSatoshis == 399'900'000ULL);
}

TEST_CASE("loadTransactions: third transaction is version 2", "[transaction]") {
    std::string err;
    auto txs = loadTransactions(TRANSACTION_DAT_PATH, err);
    REQUIRE(err.empty());
    REQUIRE(txs.size() >= 3u);

    const auto& tx = txs[2];
    REQUIRE(tx.version == 2u);
    REQUIRE(tx.locktime == 500'000u);
    REQUIRE(tx.inputs[0].sequence == 0xFDFFFFFFu);
}

TEST_CASE("loadTransactions: returns error for nonexistent file", "[transaction]") {
    std::string err;
    auto txs = loadTransactions("/nonexistent/path/transaction_data.dat", err);
    REQUIRE_FALSE(err.empty());
    REQUIRE(txs.empty());
}

TEST_CASE("loadTransactions: returns error for invalid transaction version", "[transaction]") {
    const std::string dat =
        "TX 0 0\n"
        "IN 0000000000000000000000000000000000000000000000000000000000000000 0 04dead ffffffff\n"
        "OUT 1000 76a914\n";
    std::string path = writeTempDat(dat);
    std::string err;
    auto txs = loadTransactions(path, err);
    REQUIRE_FALSE(err.empty());
    REQUIRE(txs.empty());
}

TEST_CASE("loadTransactions: returns error for IN before TX", "[transaction]") {
    const std::string dat =
        "IN 0000000000000000000000000000000000000000000000000000000000000000 0 04dead ffffffff\n";
    std::string path = writeTempDat(dat);
    std::string err;
    auto txs = loadTransactions(path, err);
    REQUIRE_FALSE(err.empty());
    REQUIRE(txs.empty());
}

TEST_CASE("loadTransactions: returns error for OUT before TX", "[transaction]") {
    const std::string dat = "OUT 50000 76a914\n";
    std::string path = writeTempDat(dat);
    std::string err;
    auto txs = loadTransactions(path, err);
    REQUIRE_FALSE(err.empty());
    REQUIRE(txs.empty());
}

TEST_CASE("loadTransactions: returns error for unknown keyword", "[transaction]") {
    const std::string dat =
        "TX 1 0\n"
        "IN 0000000000000000000000000000000000000000000000000000000000000000 0 04dead ffffffff\n"
        "OUT 50000 76a914\n"
        "BOGUS whatever\n";
    std::string path = writeTempDat(dat);
    std::string err;
    auto txs = loadTransactions(path, err);
    REQUIRE_FALSE(err.empty());
    REQUIRE(txs.empty());
}

TEST_CASE("loadTransactions: returns error for malformed TX line", "[transaction]") {
    const std::string dat = "TX\n";   // missing version and locktime
    std::string path = writeTempDat(dat);
    std::string err;
    auto txs = loadTransactions(path, err);
    REQUIRE_FALSE(err.empty());
    REQUIRE(txs.empty());
}

TEST_CASE("loadTransactions: returns error for prev_txid not 64 hex chars", "[transaction]") {
    const std::string dat =
        "TX 1 0\n"
        "IN 0000 0 04dead ffffffff\n"  // too short
        "OUT 50000 76a914\n";
    std::string path = writeTempDat(dat);
    std::string err;
    auto txs = loadTransactions(path, err);
    REQUIRE_FALSE(err.empty());
    REQUIRE(txs.empty());
}

TEST_CASE("loadTransactions: skips comment and blank lines", "[transaction]") {
    const std::string dat =
        "# This is a comment\n"
        "\n"
        "TX 1 0\n"
        "# Another comment\n"
        "IN 0000000000000000000000000000000000000000000000000000000000000000 4294967295 04ff ffffffff\n"
        "\n"
        "OUT 1000000 76a914\n";
    std::string path = writeTempDat(dat);
    std::string err;
    auto txs = loadTransactions(path, err);
    REQUIRE(err.empty());
    REQUIRE(txs.size() == 1u);
}

// ── serializeTransaction tests ────────────────────────────────────────────────

TEST_CASE("serializeTransaction: non-empty for minimal transaction", "[transaction]") {
    auto tx  = makeMinimalTx(1);
    auto raw = serializeTransaction(tx);
    REQUIRE_FALSE(raw.empty());
}

TEST_CASE("serializeTransaction: begins with version bytes (little-endian)", "[transaction]") {
    auto tx  = makeMinimalTx(1);
    auto raw = serializeTransaction(tx);
    REQUIRE(raw.size() >= 4u);
    // version 1 in little-endian: 01 00 00 00
    REQUIRE(raw[0] == 0x01);
    REQUIRE(raw[1] == 0x00);
    REQUIRE(raw[2] == 0x00);
    REQUIRE(raw[3] == 0x00);
}

TEST_CASE("serializeTransaction: ends with locktime bytes (little-endian)", "[transaction]") {
    auto tx      = makeMinimalTx(1);
    tx.locktime  = 500000u;  // 0x0007A120
    auto raw     = serializeTransaction(tx);
    REQUIRE(raw.size() >= 4u);
    // locktime 500000 = 0x0007A120 in little-endian: 20 A1 07 00
    REQUIRE(raw[raw.size() - 4] == 0x20);
    REQUIRE(raw[raw.size() - 3] == 0xA1);
    REQUIRE(raw[raw.size() - 2] == 0x07);
    REQUIRE(raw[raw.size() - 1] == 0x00);
}

TEST_CASE("serializeTransaction: two transactions with same content produce identical bytes",
          "[transaction]") {
    auto tx1 = makeMinimalTx(1);
    auto tx2 = makeMinimalTx(1);
    REQUIRE(serializeTransaction(tx1) == serializeTransaction(tx2));
}

TEST_CASE("serializeTransaction: differing versions produce different bytes", "[transaction]") {
    auto tx1 = makeMinimalTx(1);
    auto tx2 = makeMinimalTx(2);
    REQUIRE(serializeTransaction(tx1) != serializeTransaction(tx2));
}

// ── computeContentHash tests ──────────────────────────────────────────────────

TEST_CASE("computeContentHash: returns 32-byte hash", "[transaction]") {
    auto tx   = makeMinimalTx(1);
    auto hash = computeContentHash(tx);
    REQUIRE(hash.size() == 32u);
}

TEST_CASE("computeContentHash: identical transactions produce identical hashes",
          "[transaction]") {
    auto tx1 = makeMinimalTx(1);
    auto tx2 = makeMinimalTx(1);
    REQUIRE(computeContentHash(tx1) == computeContentHash(tx2));
}

TEST_CASE("computeContentHash: different transactions produce different hashes",
          "[transaction]") {
    auto tx1 = makeMinimalTx(1);
    auto tx2 = makeMinimalTx(2);
    REQUIRE(computeContentHash(tx1) != computeContentHash(tx2));
}

TEST_CASE("computeContentHash: hash is not all-zero for non-empty transaction",
          "[transaction]") {
    auto tx   = makeMinimalTx(1);
    auto hash = computeContentHash(tx);
    bool allZero = true;
    for (auto b : hash) {
        if (b != 0) { allZero = false; break; }
    }
    REQUIRE_FALSE(allZero);
}

// ── contentHashHex tests ──────────────────────────────────────────────────────

TEST_CASE("contentHashHex: returns 64-character string", "[transaction]") {
    auto tx  = makeMinimalTx(1);
    auto hex = contentHashHex(tx);
    REQUIRE(hex.size() == 64u);
}

TEST_CASE("contentHashHex: contains only lowercase hex characters", "[transaction]") {
    auto tx  = makeMinimalTx(1);
    auto hex = contentHashHex(tx);
    for (char c : hex) {
        bool valid = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        REQUIRE(valid);
    }
}

TEST_CASE("contentHashHex: identical transactions produce identical hex strings",
          "[transaction]") {
    auto tx1 = makeMinimalTx(1);
    auto tx2 = makeMinimalTx(1);
    REQUIRE(contentHashHex(tx1) == contentHashHex(tx2));
}

TEST_CASE("contentHashHex: different transactions produce different hex strings",
          "[transaction]") {
    auto tx1 = makeMinimalTx(1);
    auto tx2 = makeMinimalTx(2);
    REQUIRE(contentHashHex(tx1) != contentHashHex(tx2));
}

TEST_CASE("contentHashHex: hex is reversed byte order of computeContentHash",
          "[transaction]") {
    auto tx   = makeMinimalTx(1);
    auto hash = computeContentHash(tx);
    auto hex  = contentHashHex(tx);

    // The hex string should represent the hash bytes in reverse order.
    // hash[31] should be the first two hex chars, hash[0] the last two.
    auto byteToHex = [](uint8_t b) -> std::string {
        const char* digits = "0123456789abcdef";
        return std::string{digits[b >> 4], digits[b & 0x0F]};
    };
    REQUIRE(hex.substr(0, 2) == byteToHex(hash[31]));
    REQUIRE(hex.substr(62, 2) == byteToHex(hash[0]));
}
