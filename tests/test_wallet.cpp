// tests/test_wallet.cpp — validate BitcoinMaxWallet behaviour

#include <catch2/catch_test_macros.hpp>
#include "bitcoin2max/wallet.h"

#include <limits>

using namespace bitcoin2max;

TEST_CASE("Wallet: initial balance is 10 000 BTC", "[wallet]") {
    BitcoinMaxWallet w;
    REQUIRE(w.balanceBTC() == 10'000ULL);
    REQUIRE(w.balanceSatoshis() == 10'000ULL * SATOSHIS_PER_BTC);
}

TEST_CASE("Wallet: addBTC increases balance correctly", "[wallet]") {
    BitcoinMaxWallet w;
    w.addBTC(500);
    REQUIRE(w.balanceBTC() == 10'500ULL);
}

TEST_CASE("Wallet: addSatoshis increases balance correctly", "[wallet]") {
    BitcoinMaxWallet w;
    w.addSatoshis(1);
    REQUIRE(w.balanceSatoshis() == 10'000ULL * SATOSHIS_PER_BTC + 1ULL);
}

TEST_CASE("Wallet: spendSatoshis decreases balance correctly", "[wallet]") {
    BitcoinMaxWallet w;
    w.spendSatoshis(SATOSHIS_PER_BTC); // spend 1 BTC
    REQUIRE(w.balanceBTC() == 9'999ULL);
}

TEST_CASE("Wallet: spending more than balance throws", "[wallet]") {
    BitcoinMaxWallet w;
    uint64_t tooMuch = w.balanceSatoshis() + 1;
    REQUIRE_THROWS_AS(w.spendSatoshis(tooMuch), std::runtime_error);
}

TEST_CASE("Wallet: adding zero satoshis throws", "[wallet]") {
    BitcoinMaxWallet w;
    REQUIRE_THROWS_AS(w.addSatoshis(0), std::invalid_argument);
}

TEST_CASE("Wallet: adding zero BTC throws", "[wallet]") {
    BitcoinMaxWallet w;
    REQUIRE_THROWS_AS(w.addBTC(0), std::invalid_argument);
}

TEST_CASE("Wallet: spending zero satoshis throws", "[wallet]") {
    BitcoinMaxWallet w;
    REQUIRE_THROWS_AS(w.spendSatoshis(0), std::invalid_argument);
}

TEST_CASE("Wallet: default label is 'BitcoinMaxWallet'", "[wallet]") {
    BitcoinMaxWallet w;
    REQUIRE(w.label() == "BitcoinMaxWallet");
}

TEST_CASE("Wallet: label can be changed", "[wallet]") {
    BitcoinMaxWallet w;
    w.setLabel("MyWallet");
    REQUIRE(w.label() == "MyWallet");
}

TEST_CASE("Wallet: addSatoshis overflow throws", "[wallet]") {
    BitcoinMaxWallet w;
    uint64_t huge = std::numeric_limits<uint64_t>::max();
    REQUIRE_THROWS_AS(w.addSatoshis(huge), std::overflow_error);
}

TEST_CASE("Wallet: addBTC overflow throws", "[wallet]") {
    BitcoinMaxWallet w;
    uint64_t huge = std::numeric_limits<uint64_t>::max() / SATOSHIS_PER_BTC + 1;
    REQUIRE_THROWS_AS(w.addBTC(huge), std::overflow_error);
}

TEST_CASE("Wallet: WALLET_INITIAL_BTC constant equals 10 000", "[wallet]") {
    REQUIRE(WALLET_INITIAL_BTC == 10'000ULL);
}
