#pragma once
// src/electrum/electrum_client.h
//
// Lightweight Electrum JSON-RPC client.
// Connects to an Electrum server over a TCP socket (plain or TLS) and exposes
// the most commonly used Electrum protocol methods.

#include "bitcoin2max/config.h"

#include <cstdint>
#include <functional>
#include <string>

namespace bitcoin2max {

/// Simple synchronous Electrum client.
/// Thread safety: NOT thread-safe; wrap in a mutex if shared across threads.
class ElectrumClient {
public:
    explicit ElectrumClient(const Config& cfg);
    ~ElectrumClient();

    // Non-copyable, movable
    ElectrumClient(const ElectrumClient&)            = delete;
    ElectrumClient& operator=(const ElectrumClient&) = delete;
    ElectrumClient(ElectrumClient&&)                 noexcept;
    ElectrumClient& operator=(ElectrumClient&&)      noexcept;

    /// Open the connection to the configured Electrum server.
    /// Returns true on success.
    bool connect();

    /// Close the connection gracefully.
    void disconnect();

    /// Returns true if the socket is currently connected.
    bool isConnected() const { return fd_ >= 0; }

    // ── Electrum protocol methods ─────────────────────────────────────────────

    /// server.version — negotiate protocol version.
    /// Returns the server's reported version string, or empty on error.
    std::string serverVersion();

    /// blockchain.headers.subscribe — get the current best-block header.
    /// Returns the raw hex header, or empty on error.
    std::string getBestBlockHeader();

    /// blockchain.transaction.get — fetch a raw transaction by txid.
    /// Returns raw hex, or empty on error.
    std::string getTransaction(const std::string& txid);

    /// blockchain.scripthash.get_balance — get confirmed + unconfirmed balance
    /// for the given script hash (sha256-reversed of the output script).
    /// Returns the JSON response string, or empty on error.
    std::string getScriptHashBalance(const std::string& scriptHash);

    /// blockchain.transaction.broadcast — broadcast a raw transaction.
    /// Returns the txid on success, or empty on error.
    std::string broadcastTransaction(const std::string& rawTxHex);

private:
    const Config& cfg_;
    int           fd_{-1};
    uint64_t      nextId_{1};

    /// Send a raw JSON-RPC request and return the full response string.
    std::string sendRequest(const std::string& json);

    /// Build a JSON-RPC 2.0 request string.
    std::string buildRequest(const std::string& method,
                             const std::string& params = "[]");
};

} // namespace bitcoin2max
