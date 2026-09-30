#include <catch2/catch_test_macros.hpp>

#include "crypto/block_hash.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

using namespace bitcoin2max;

TEST_CASE("Bitcoin genesis block header produces correct hash", "[block_hash]") {
    const std::string headerHex =
        "01000000"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "3ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a"
        "29ab5f49"
        "ffff001d"
        "1dac2b7c";

    std::vector<uint8_t> header;

    for (std::size_t i = 0; i < headerHex.size(); i += 2) {
        header.push_back(
            static_cast<uint8_t>(
                std::stoul(headerHex.substr(i, 2), nullptr, 16)
            )
        );
    }

    auto hash = crypto::sha256d(header);

    std::reverse(hash.begin(), hash.end());

    REQUIRE(
        crypto::toHex(hash) ==
        "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"
    );
}
