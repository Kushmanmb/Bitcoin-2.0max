#pragma once

#include "../platform/socket_compat.h"

#include <atomic>
#include <cstdint>
#include <thread>

namespace bitcoin2max {

class StatusApi;
class BlocksApi;

class HttpServer {
public:
    HttpServer(
        const StatusApi& statusApi,
        const BlocksApi& blocksApi,
        uint16_t port = 8080
    );

    ~HttpServer();

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    bool start();
    void stop();
    bool isRunning() const;

private:
    void run();

    const StatusApi& statusApi_;
    const BlocksApi& blocksApi_;

    uint16_t port_;
    std::atomic<bool> running_{false};
    SocketHandle serverFd_{INVALID_SOCKET_HANDLE};
    std::thread thread_;
};

} // namespace bitcoin2max
