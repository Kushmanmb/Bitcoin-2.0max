#include <catch2/catch_test_macros.hpp>

#include "bitcoin2max/subsidy.h"
#include "consensus/pow.h"
#include "consensus/regtest_chain.h"

#include <limits>
#include <sstream>

using namespace bitcoin2max;

TEST_CASE("Native subsidy halves, terminates and stays below the cap", "[native][subsidy]") {
    REQUIRE(consensus::blockSubsidy(0) == 0);
    REQUIRE(consensus::blockSubsidy(1) == 50 * consensus::COIN);
    REQUIRE(consensus::blockSubsidy(209999) == 50 * consensus::COIN);
    REQUIRE(consensus::blockSubsidy(210000) == 25 * consensus::COIN);
    REQUIRE(consensus::blockSubsidy(420000) == 1250000000ULL);
    REQUIRE(consensus::blockSubsidy(33 * consensus::HALVING_INTERVAL) == 0);
    REQUIRE(consensus::blockSubsidy(64 * consensus::HALVING_INTERVAL) == 0);
    REQUIRE(consensus::blockSubsidy(std::numeric_limits<uint64_t>::max()) == 0);
    uint64_t sum = 0;
    for (uint64_t epoch = 0; epoch < 64; ++epoch) {
        const auto height = epoch * consensus::HALVING_INTERVAL;
        const auto count = consensus::HALVING_INTERVAL - (epoch == 0 ? 1 : 0);
        sum += count * consensus::blockSubsidy(height == 0 ? 1 : height);
    }
    REQUIRE(sum == 2099994997690000ULL);
    REQUIRE(sum < consensus::MAX_MONEY_SATOSHIS);
}

TEST_CASE("Coinbase reward bounds reject excess money and overflow", "[native][subsidy]") {
    constexpr auto reward = consensus::INITIAL_SUBSIDY;
    REQUIRE(consensus::validCoinbaseReward(1, reward, 0, 0));
    REQUIRE(consensus::validCoinbaseReward(1, reward + 100, 100, 0));
    REQUIRE_FALSE(consensus::validCoinbaseReward(1, reward + 1, 0, 0));
    REQUIRE_FALSE(consensus::validCoinbaseReward(1, reward, 0, consensus::MAX_MONEY_SATOSHIS));
    REQUIRE_FALSE(consensus::validCoinbaseReward(1, reward, 0, std::numeric_limits<uint64_t>::max()));
    REQUIRE_FALSE(consensus::validCoinbaseReward(1, reward, std::numeric_limits<uint64_t>::max(), 0));
    REQUIRE_FALSE(consensus::validCoinbaseReward(1, std::numeric_limits<uint64_t>::max(), 0, 0));
}

TEST_CASE("Proof of work rejects invalid targets and honors little-endian boundary", "[native][pow]") {
    crypto::Hash256 zero{};
    REQUIRE(consensus::checkProofOfWork(zero, 0x207fffff, 0x207fffff));
    for (const auto bad : {0u, 0x20800001u, 0x23000001u, 0x22000100u,
                           0x21010000u, 0x01000001u, 0x2100ffffu}) {
        REQUIRE_FALSE(consensus::checkProofOfWork(zero, bad, 0x207fffff));
    }
    crypto::Hash256 target{};
    target[29] = 0xff;
    target[30] = 0xff;
    target[31] = 0x7f;
    REQUIRE(consensus::checkProofOfWork(target, 0x207fffff, 0x207fffff));
    target[0] = 1;
    REQUIRE_FALSE(consensus::checkProofOfWork(target, 0x207fffff, 0x207fffff));
    target.fill(0xff);
    REQUIRE_FALSE(consensus::checkProofOfWork(target, 0x207fffff, 0x207fffff));
}

TEST_CASE("Bitcoin genesis proof of work uses the raw digest in correct order", "[native][pow]") {
    // Conventional displayed genesis hash, reversed into raw digest order.
    const std::string shown = "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f";
    crypto::Hash256 raw{};
    for (size_t i = 0; i < 32; ++i)
        raw[31 - i] = static_cast<uint8_t>(std::stoul(shown.substr(2 * i, 2), nullptr, 16));
    REQUIRE(consensus::checkProofOfWork(raw, 0x1d00ffff, 0x1d00ffff));
    REQUIRE_FALSE(consensus::checkProofOfWork(raw, 0x03000001, 0x1d00ffff));
}

TEST_CASE("Regtest genesis is deterministic and allocates no coins", "[native][regtest]") {
    const auto genesis = regtest::mineBlock(0, {});
    REQUIRE(genesis.coinbase.outputs[0].valueSatoshis == 0);
    REQUIRE(consensus::checkProofOfWork(crypto::computeBlockHash(genesis.header),
                                      genesis.header.bits, regtest::POW_BITS));
    regtest::Chain first, second;
    REQUIRE(first.tip() == second.tip());
    REQUIRE(regtest::displayHash(first.tip()) == regtest::GENESIS_HASH);
    REQUIRE(first.issued() == 0);
}

TEST_CASE("Regtest rejects altered blocks without mutating chain state", "[native][regtest]") {
    regtest::Chain chain;
    const auto oldTip = chain.tip();
    const auto good = regtest::mineBlock(1, chain.tip());
    SECTION("inflated reward") {
        auto bad = good;
        ++bad.coinbase.outputs[0].valueSatoshis;
        bad.header.merkleRoot = computeContentHash(bad.coinbase);
        REQUIRE_FALSE(chain.append(bad).valid);
    }
    SECTION("wrong parent") {
        auto bad = good;
        bad.header.prevHash[0] ^= 1;
        REQUIRE_FALSE(chain.append(bad).valid);
    }
    SECTION("wrong Merkle root") {
        auto bad = good;
        bad.header.merkleRoot[0] ^= 1;
        REQUIRE_FALSE(chain.append(bad).valid);
    }
    SECTION("different difficulty") {
        auto bad = good;
        bad.header.bits = 0x207ffffe;
        REQUIRE_FALSE(chain.append(bad).valid);
    }
    SECTION("invalid nonce") {
        auto bad = good;
        while (consensus::checkProofOfWork(crypto::computeBlockHash(bad.header),
                                          bad.header.bits, regtest::POW_BITS)) ++bad.header.nonce;
        REQUIRE_FALSE(chain.append(bad).valid);
    }
    REQUIRE(chain.tip() == oldTip);
    REQUIRE(chain.height() == 0);
    REQUIRE(chain.issued() == 0);
    REQUIRE(chain.append(good).valid);
    REQUIRE_FALSE(chain.append(good).valid);
}

TEST_CASE("Saved regtest chain replays independently with reward maturity", "[native][regtest]") {
    regtest::Chain chain;
    std::stringstream file;
    file << "B2MX-REGTEST-V1\n";
    for (int i = 0; i < 101; ++i) {
        const auto block = regtest::mineBlock(chain.height() + 1, chain.tip());
        REQUIRE(chain.append(block).valid);
        regtest::Chain::writeRecord(file, chain.height(), block);
        if (chain.height() == 100) REQUIRE(chain.matureRewards() == 0);
    }
    REQUIRE(chain.matureRewards() == consensus::INITIAL_SUBSIDY);
    const auto restored = regtest::Chain::load(file);
    REQUIRE(restored.height() == chain.height());
    REQUIRE(restored.tip() == chain.tip());
    REQUIRE(restored.issued() == 101 * consensus::INITIAL_SUBSIDY);
    REQUIRE(restored.matureRewards() == chain.matureRewards());
}

TEST_CASE("Regtest replay rejects corrupt and incomplete files", "[native][regtest]") {
    regtest::Chain chain;
    const auto block = regtest::mineBlock(1, chain.tip());
    std::ostringstream good;
    good << "B2MX-REGTEST-V1\n";
    regtest::Chain::writeRecord(good, 1, block);
    std::string text = good.str();
    SECTION("wrong network") { text.replace(0, 4, "BTC!"); }
    SECTION("truncated final record") { text.pop_back(); }
    SECTION("truncated identifier") { text = "B2MX-REGTEST-V1"; }
    SECTION("modified stored hash") { text[text.size() - 2] = 'z'; }
    SECTION("negative height") { text.insert(text.find('\n') + 1, "-"); }
    SECTION("duplicate height") { text += text.substr(text.find('\n') + 1); }
    SECTION("oversized record") { text += std::string(129, '1') + '\n'; }
    SECTION("nonce overflow") { text = "B2MX-REGTEST-V1\n1 4294967296 bad\n"; }
    std::istringstream input(text);
    REQUIRE_THROWS(regtest::Chain::load(input));
}
