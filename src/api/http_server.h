#pragma once

#include <atomic>
#include <cstdint>
#include <thread>

namespace bitcoin2max {

class StatusApi;

class HttpServer {
public:
    HttpServer(const StatusApi& statusApi, uint16_t port = 8080);
    ~HttpServer();

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    bool start();
    void stop();
    bool isRunning() const;

private:
    void run();

    const StatusApi& statusApi_;
    uint16_t port_;
    std::atomic<bool> running_{false};
    int serverFd_{-1};
    std::thread thread_;
};

} // namespace bitcoin2max
