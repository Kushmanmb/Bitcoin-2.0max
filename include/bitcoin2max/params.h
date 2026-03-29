#pragma once

#include <cstdint>

// ── Bitcoin 2.0max consensus parameters ──────────────────────────────────────
// All values can be overridden at runtime via bitcoin2max.conf.

namespace bitcoin2max {
namespace consensus {

/// Target time between blocks (seconds).  1 minute for supercharged speed.
static constexpr uint32_t TARGET_BLOCK_TIME_SECONDS = 60;

/// Maximum serialised block size in bytes (32 MiB for high scalability).
static constexpr uint32_t MAX_BLOCK_SIZE_BYTES = 32 * 1024 * 1024;

/// Difficulty retarget window (number of blocks).
static constexpr uint32_t RETARGET_WINDOW_BLOCKS = 2016;

/// Coinbase maturity depth (blocks before a coinbase can be spent).
static constexpr uint32_t COINBASE_MATURITY = 100;

/// Maximum money supply (satoshis).
static constexpr uint64_t MAX_MONEY_SATOSHIS = 21'000'000ULL * 100'000'000ULL;

} // namespace consensus

namespace network {

/// Default port for the P2P network.
static constexpr uint16_t DEFAULT_P2P_PORT = 8333;

/// Default port for the RPC interface.
static constexpr uint16_t DEFAULT_RPC_PORT = 8332;

} // namespace network

namespace electrum {

/// Default Electrum server host.
static constexpr const char* DEFAULT_HOST = "127.0.0.1";

/// Default Electrum server port (local Electrum server).
static constexpr uint16_t DEFAULT_PORT = 9050;

/// Connection timeout in seconds.
static constexpr uint32_t CONNECT_TIMEOUT_SECONDS = 10;

/// Maximum number of reconnect attempts before giving up.
static constexpr uint32_t MAX_RECONNECT_ATTEMPTS = 5;

} // namespace electrum
} // namespace bitcoin2max
