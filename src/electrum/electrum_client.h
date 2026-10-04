#pragma once

// Lightweight Electrum JSON-RPC client.

#include "bitcoin2max/config.h"
#include "../platform/socket_compat.h"

#include <cstdint>
#include <string>

namespace bitcoin2max {

class ElectrumClient {
public:
    explicit ElectrumClient(const Config& cfg);
    ~ElectrumClient();

    ElectrumClient(const ElectrumClient&) = delete;
    ElectrumClient& operator=(const ElectrumClient&) = delete;

    ElectrumClient(ElectrumClient&&) noexcept;
    ElectrumClient& operator=(ElectrumClient&&) noexcept;

    bool connect();
    void disconnect();

    bool isConnected() const {
        return socketValid(fd_);
    }

    std::string serverVersion();

    std::string getBestBlockHeader();

    uint64_t getBestBlockHeight();

    std::string getBlockHeaderHex(
        uint64_t height
    );

    std::string getBestBlockHeaderHex();

    std::string getTransaction(
        const std::string& txid
    );

    std::string getScriptHashBalance(
        const std::string& scriptHash
    );

    std::string broadcastTransaction(
        const std::string& rawTxHex
    );

private:
    const Config& cfg_;

    SocketHandle
        fd_{INVALID_SOCKET_HANDLE};

    uint64_t nextId_{1};

    static std::string extractStringResult(
        const std::string& response
    );

    std::string sendRequest(
        const std::string& json
    );

    std::string buildRequest(
        const std::string& method,
        const std::string& params = "[]"
    );
};

} // namespace bitcoin2max
