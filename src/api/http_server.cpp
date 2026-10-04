#include "http_server.h"
#include "status_api.h"
#include "blocks_api.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

namespace bitcoin2max {

HttpServer::HttpServer(
    const StatusApi& statusApi,
    const BlocksApi& blocksApi,
    uint16_t port
)
    : statusApi_(statusApi),
      blocksApi_(blocksApi),
      port_(port) {}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start() {
    if (running_) return true;
serverFd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (!socketValid(serverFd_)) {
        std::cerr << "[HTTP] Unable to create socket\n";
        return false;
    }

    int reuse = 1;

#ifdef _WIN32
    ::setsockopt(
        serverFd_,
        SOL_SOCKET,
        SO_REUSEADDR,
        reinterpret_cast<const char*>(&reuse),
        sizeof(reuse)
    );
#else
    ::setsockopt(
        serverFd_,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse,
        sizeof(reuse)
    );
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port_);

    if (::bind(
            serverFd_,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)
        ) != 0) {
        std::cerr << "[HTTP] Unable to bind to 127.0.0.1:"
                  << port_ << "\n";

        closeSocket(serverFd_);
        serverFd_ = INVALID_SOCKET_HANDLE;
        return false;
    }

    if (::listen(serverFd_, 16) != 0) {
        std::cerr << "[HTTP] Unable to listen\n";

        closeSocket(serverFd_);
        serverFd_ = INVALID_SOCKET_HANDLE;
        return false;
    }

    running_ = true;
    thread_ = std::thread(&HttpServer::run, this);

    std::cout
        << "[HTTP] Status API listening on http://127.0.0.1:"
        << port_
        << "/status\n";

    return true;
}

void HttpServer::stop() {
    if (!running_.exchange(false)) {
        if (thread_.joinable()) {
            thread_.join();
        }
        return;
    }

    if (socketValid(serverFd_)) {
        shutdownSocket(serverFd_);
        closeSocket(serverFd_);
        serverFd_ = INVALID_SOCKET_HANDLE;
    }

    if (thread_.joinable()) {
        thread_.join();
    }
}

bool HttpServer::isRunning() const {
    return running_.load();
}

void HttpServer::run() {
    while (running_) {
        sockaddr_in clientAddress{};

#ifdef _WIN32
        int clientLength = sizeof(clientAddress);
#else
        socklen_t clientLength = sizeof(clientAddress);
#endif

        SocketHandle client = ::accept(
            serverFd_,
            reinterpret_cast<sockaddr*>(&clientAddress),
            &clientLength
        );

        if (!socketValid(client)) {
            if (running_) {
                std::cerr << "[HTTP] accept failed\n";
            }
            continue;
        }

        char request[4096]{};

#ifdef _WIN32
        const int received = ::recv(
            client,
            request,
            static_cast<int>(sizeof(request) - 1),
            0
        );
#else
        const ssize_t received = ::recv(
            client,
            request,
            sizeof(request) - 1,
            0
        );
#endif

        if (received > 0) {
            request[received] = '\0';

            const std::string req(request);

            std::string body;
            std::string status;

            if (req.rfind("GET /status ", 0) == 0) {
                body = statusApi_.getStatusJson();
                status = "200 OK";
            }
            else if (req.rfind("GET /blocks ", 0) == 0) {
                body = blocksApi_.getBlocksJson();
                status = "200 OK";
            }
            else {
                body = "{\"error\":\"not found\"}";
                status = "404 Not Found";
            }

            const std::string response =
                "HTTP/1.1 " + status + "\r\n"
                "Content-Type: application/json\r\n"
                "Cache-Control: no-store\r\n"
                "Connection: close\r\n"
                "Content-Length: " +
                std::to_string(body.size()) +
                "\r\n\r\n" +
                body;

#ifdef _WIN32
            ::send(
                client,
                response.data(),
                static_cast<int>(response.size()),
                0
            );
#else
            ::send(
                client,
                response.data(),
                response.size(),
                0
            );
#endif
        }

        shutdownSocket(client);
        closeSocket(client);
    }
}

} // namespace bitcoin2max
