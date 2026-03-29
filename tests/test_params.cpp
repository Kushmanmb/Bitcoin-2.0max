// tests/test_params.cpp — validate compile-time consensus parameters

#include <catch2/catch_test_macros.hpp>
#include "bitcoin2max/params.h"

using namespace bitcoin2max;

TEST_CASE("Consensus: supercharged block time is 60 seconds", "[params]") {
    REQUIRE(consensus::TARGET_BLOCK_TIME_SECONDS == 60u);
    // Must be faster than legacy Bitcoin's 600-second target
    REQUIRE(consensus::TARGET_BLOCK_TIME_SECONDS < 600u);
}

TEST_CASE("Consensus: max block size is 32 MiB", "[params]") {
    REQUIRE(consensus::MAX_BLOCK_SIZE_BYTES == 32u * 1024u * 1024u);
    // Must be larger than legacy Bitcoin's 1 MiB limit
    REQUIRE(consensus::MAX_BLOCK_SIZE_BYTES > 1u * 1024u * 1024u);
}

TEST_CASE("Electrum: default host and port match spec", "[params]") {
    REQUIRE(std::string(electrum::DEFAULT_HOST) == "127.0.0.1");
    REQUIRE(electrum::DEFAULT_PORT == 9050u);
}

TEST_CASE("Network: default P2P port is 8333", "[params]") {
    REQUIRE(network::DEFAULT_P2P_PORT == 8333u);
}
