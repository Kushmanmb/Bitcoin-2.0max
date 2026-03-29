// src/config/config.cpp
// Loads configuration from a .conf file and/or environment variables.

#include "bitcoin2max/config.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace bitcoin2max {

// ── helpers ───────────────────────────────────────────────────────────────────

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static bool parseBool(const std::string& v) {
    std::string lv = v;
    std::transform(lv.begin(), lv.end(), lv.begin(), ::tolower);
    return lv == "1" || lv == "true" || lv == "yes";
}

// ── public API ────────────────────────────────────────────────────────────────

Config loadConfig(const std::string& path) {
    Config cfg;

    if (path.empty()) return cfg;

    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        // Config file is optional; return defaults if not found.
        return cfg;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        line = trim(line);

        // Skip blank lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key   = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));

        // Strip inline comments
        auto hash = value.find('#');
        if (hash != std::string::npos) value = trim(value.substr(0, hash));

        if (key == "p2pport")              cfg.p2p_port             = static_cast<uint16_t>(std::stoul(value));
        else if (key == "rpcport")         cfg.rpc_port             = static_cast<uint16_t>(std::stoul(value));
        else if (key == "rpcuser")         cfg.rpc_user             = value;
        else if (key == "rpcpassword")     cfg.rpc_password         = value;
        else if (key == "targetblocktime") cfg.target_block_time    = static_cast<uint32_t>(std::stoul(value));
        else if (key == "maxblocksize")    cfg.max_block_size       = static_cast<uint32_t>(std::stoul(value));
        else if (key == "electrum")        cfg.electrum_enabled     = parseBool(value);
        else if (key == "electrumhost")    cfg.electrum_host        = value;
        else if (key == "electrumport")    cfg.electrum_port        = static_cast<uint16_t>(std::stoul(value));
        else if (key == "electrumusessl")  cfg.electrum_use_ssl     = parseBool(value);
        else if (key == "tor")             cfg.tor_enabled          = parseBool(value);
        else if (key == "torproxy")        cfg.tor_proxy            = value;
        else if (key == "datadir")         cfg.datadir              = value;
        else if (key == "loglevel")        cfg.log_level            = value;
        else if (key == "logfile")         cfg.log_file             = value;
    }

    return cfg;
}

void printConfig(const Config& cfg) {
    std::cout << "[Config] p2p_port          = " << cfg.p2p_port          << "\n"
              << "[Config] rpc_port          = " << cfg.rpc_port          << "\n"
              << "[Config] target_block_time = " << cfg.target_block_time << "s\n"
              << "[Config] max_block_size    = " << cfg.max_block_size / (1024*1024) << " MiB\n"
              << "[Config] electrum_enabled  = " << (cfg.electrum_enabled ? "yes" : "no") << "\n"
              << "[Config] electrum_host     = " << cfg.electrum_host     << "\n"
              << "[Config] electrum_port     = " << cfg.electrum_port     << "\n"
              << "[Config] electrum_use_ssl  = " << (cfg.electrum_use_ssl ? "yes" : "no") << "\n"
              << "[Config] tor_enabled       = " << (cfg.tor_enabled      ? "yes" : "no") << "\n"
              << "[Config] datadir           = " << cfg.datadir           << "\n"
              << "[Config] log_level         = " << cfg.log_level         << "\n";
}

} // namespace bitcoin2max
