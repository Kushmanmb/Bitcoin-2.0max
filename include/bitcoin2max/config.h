#pragma once

#include <cstdint>
#include <string>

namespace bitcoin2max {

/// Runtime configuration loaded from bitcoin2max.conf (or CLI arguments).
struct Config {
    // ── Network ──────────────────────────────────────────────────────────────
    uint16_t    p2p_port{8333};
    uint16_t    rpc_port{8332};
    std::string rpc_user;
    std::string rpc_password;

    // ── Consensus (may diverge from compile-time defaults) ───────────────────
    uint32_t    target_block_time{60};   ///< seconds
    uint32_t    max_block_size{32 * 1024 * 1024}; ///< bytes

    // ── Electrum server ───────────────────────────────────────────────────────
    /// Whether to connect to a local Electrum server instead of BOLDwallet.
    bool        electrum_enabled{true};
    std::string electrum_host{"127.0.0.1"};
    uint16_t    electrum_port{9050};
    bool        electrum_use_ssl{false};

    // ── Tor ───────────────────────────────────────────────────────────────────
    bool        tor_enabled{false};
    /// Tor SOCKS5 proxy address.  Uses port 9051 to avoid collision with the
    /// Electrum server which defaults to 9050.
    std::string tor_proxy{"127.0.0.1:9051"};

    // ── Storage ───────────────────────────────────────────────────────────────
    std::string datadir{"~/.bitcoin2max"};

    // ── Logging ───────────────────────────────────────────────────────────────
    std::string log_level{"info"};
    std::string log_file;  ///< empty → stderr
};

} // namespace bitcoin2max
