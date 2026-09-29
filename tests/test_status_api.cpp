#include "api/status_api.h"
#include "config/config.h"
#include "node/node.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("StatusApi returns node status JSON") {
    bitcoin2max::Config config;
    bitcoin2max::Node node(config);
    bitcoin2max::StatusApi api(node);

    const std::string json = api.getStatusJson();

    REQUIRE(json.find("\"status\":\"offline\"") != std::string::npos);
    REQUIRE(json.find("\"running\":false") != std::string::npos);
    REQUIRE(json.find("\"blockHeight\":0") != std::string::npos);
    REQUIRE(json.find("\"peers\":0") != std::string::npos);
    REQUIRE(json.find("\"mempool\":0") != std::string::npos);
    REQUIRE(json.find("\"electrumConnected\":false") != std::string::npos);
}
