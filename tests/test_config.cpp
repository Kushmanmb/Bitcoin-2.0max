// tests/test_config.cpp — validate Config loading from .conf files

#include <catch2/catch_test_macros.hpp>
#include "config/config.h"
#include "bitcoin2max/config.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>  // getpid()

// Helper: write a temporary conf file and return its path.
// Uses the process PID so parallel CTest invocations don't overwrite each
// other's files (each test case runs as a separate process with its own
// static counter starting at 0).
static std::string writeTempConf(const std::string& content) {
    static int counter = 0;
    std::string path = "/tmp/test_bitcoin2max_" + std::to_string(getpid())
                       + "_" + std::to_string(counter++) + ".conf";
    std::ofstream f(path);
    f << content;
    return path;
}

TEST_CASE("Config: defaults without a file", "[config]") {
    bitcoin2max::Config cfg = bitcoin2max::loadConfig("");
    REQUIRE(cfg.p2p_port             == 8333);
    REQUIRE(cfg.rpc_port             == 8332);
    REQUIRE(cfg.electrum_enabled     == true);
    REQUIRE(cfg.electrum_host        == "127.0.0.1");
    REQUIRE(cfg.electrum_port        == 9050);
    REQUIRE(cfg.target_block_time    == 60u);
    REQUIRE(cfg.max_block_size       == 32u * 1024u * 1024u);
}

TEST_CASE("Config: missing file returns defaults", "[config]") {
    bitcoin2max::Config cfg = bitcoin2max::loadConfig("/nonexistent/path/bitcoin2max.conf");
    REQUIRE(cfg.electrum_host  == "127.0.0.1");
    REQUIRE(cfg.electrum_port  == 9050);
}

TEST_CASE("Config: values are loaded from file", "[config]") {
    std::string path = writeTempConf(
        "electrumhost=192.168.1.100\n"
        "electrumport=50001\n"
        "electrum=0\n"
        "targetblocktime=30\n"
        "maxblocksize=67108864\n"  // 64 MiB
        "rpcuser=alice\n"
        "rpcpassword=secret\n"
        "loglevel=debug\n"
    );

    bitcoin2max::Config cfg = bitcoin2max::loadConfig(path);

    REQUIRE(cfg.electrum_host      == "192.168.1.100");
    REQUIRE(cfg.electrum_port      == 50001);
    REQUIRE(cfg.electrum_enabled   == false);
    REQUIRE(cfg.target_block_time  == 30u);
    REQUIRE(cfg.max_block_size     == 67108864u);
    REQUIRE(cfg.rpc_user           == "alice");
    REQUIRE(cfg.rpc_password       == "secret");
    REQUIRE(cfg.log_level          == "debug");

    std::remove(path.c_str());
}

TEST_CASE("Config: comments and blank lines are ignored", "[config]") {
    std::string path = writeTempConf(
        "# This is a comment\n"
        "\n"
        "; Another comment\n"
        "loglevel=warn  # inline comment\n"
    );

    bitcoin2max::Config cfg = bitcoin2max::loadConfig(path);
    REQUIRE(cfg.log_level == "warn");

    std::remove(path.c_str());
}
