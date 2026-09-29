#include <catch2/catch_test_macros.hpp>

#include "api/blocks_api.h"
#include "config/config.h"
#include "node/node.h"

#include <string>

using namespace bitcoin2max;

TEST_CASE("BlocksApi returns empty array when node has no chain tip",
          "[blocks_api]") {
    Config cfg;
    Node node(cfg);
    BlocksApi api(node);

    REQUIRE(api.getBlocksJson() == "[]");
}

TEST_CASE("BlocksApi respects zero limit",
          "[blocks_api]") {
    Config cfg;
    Node node(cfg);

    node.setBestHeight(100);
    node.setBestBlockHash("test-hash");

    BlocksApi api(node);

    REQUIRE(api.getBlocksJson(0) == "[]");
}
