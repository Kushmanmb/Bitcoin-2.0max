// src/main.cpp
//
// Bitcoin 2.0max daemon entry point.

#include "api/blocks_api.h"
#include "api/http_server.h"
#include "api/status_api.h"
#include "config/config.h"
#include "node/node.h"
#include "platform/socket_compat.h"

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

static bitcoin2max::Node* g_node = nullptr;

static void handleSignal(int signal) {
    std::cout
        << "\n[main] Caught signal "
        << signal
        << ", shutting down...\n";

    if (g_node) {
        g_node->stop();
    }
}

int main(int argc, char* argv[]) {
    std::string confPath;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (
            (arg == "--conf" || arg == "-conf") &&
            i + 1 < argc
        ) {
            confPath = argv[++i];
        }
        else if (
            arg == "--help" ||
            arg == "-help" ||
            arg == "-h"
        ) {
            std::cout
                << "Bitcoin 2.0max daemon\n\n"
                << "  --conf <path>  Path to configuration file\n"
                << "  --help         Print this help message\n";

            return EXIT_SUCCESS;
        }
    }

    if (confPath.empty()) {
        const char* home = std::getenv("HOME");

#ifdef _WIN32
        if (!home) {
            home = std::getenv("USERPROFILE");
        }
#endif

        if (home) {
            confPath =
                std::string(home) +
                "/.bitcoin2max/bitcoin2max.conf";
        }
    }

    if (!bitcoin2max::initializeSockets()) {
        std::cerr
            << "[main] Failed to initialize networking.\n";

        return EXIT_FAILURE;
    }

    const bitcoin2max::Config cfg =
        bitcoin2max::loadConfig(confPath);

    bitcoin2max::printConfig(cfg);

    bitcoin2max::Node node(cfg);
    g_node = &node;

    bitcoin2max::StatusApi statusApi(node);
    bitcoin2max::BlocksApi blocksApi(node);

    bitcoin2max::HttpServer httpServer(
        statusApi,
        blocksApi,
        8080
    );

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    if (!httpServer.start()) {
        std::cerr
            << "[main] Failed to start HTTP status server.\n";

        g_node = nullptr;
        bitcoin2max::cleanupSockets();

        return EXIT_FAILURE;
    }

    node.start();
    node.join();

    httpServer.stop();

    g_node = nullptr;
    bitcoin2max::cleanupSockets();

    std::cout
        << "[main] Exited cleanly.\n";

    return EXIT_SUCCESS;
}
