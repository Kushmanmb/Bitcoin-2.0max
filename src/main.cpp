// src/main.cpp
//
// Bitcoin 2.0max daemon entry point.
// Usage:
//   bitcoin2maxd [--conf <path>] [--datadir <path>] [--help]

#include "config/config.h"
#include "node/node.h"

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

// ── signal handling ───────────────────────────────────────────────────────────

static bitcoin2max::Node* g_node = nullptr;

static void handleSignal(int sig) {
    std::cout << "\n[main] Caught signal " << sig << ", shutting down…\n";
    if (g_node) g_node->stop();
}

// ── main ──────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    std::string confPath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--conf" || arg == "-conf") && i + 1 < argc) {
            confPath = argv[++i];
        } else if ((arg == "--help" || arg == "-help" || arg == "-h")) {
            std::cout << "Bitcoin 2.0max daemon\n\n"
                      << "  --conf   <path>   Path to configuration file\n"
                      << "                    (default: ~/.bitcoin2max/bitcoin2max.conf)\n"
                      << "  --help            Print this help message\n";
            return EXIT_SUCCESS;
        }
    }

    // Default config path
    if (confPath.empty()) {
        const char* home = std::getenv("HOME");
        if (home) confPath = std::string(home) + "/.bitcoin2max/bitcoin2max.conf";
    }

    bitcoin2max::Config cfg = bitcoin2max::loadConfig(confPath);
    bitcoin2max::printConfig(cfg);

    bitcoin2max::Node node(cfg);
    g_node = &node;

    std::signal(SIGINT,  handleSignal);
    std::signal(SIGTERM, handleSignal);

    node.start();
    node.join();

    std::cout << "[main] Exited cleanly.\n";
    return EXIT_SUCCESS;
}
