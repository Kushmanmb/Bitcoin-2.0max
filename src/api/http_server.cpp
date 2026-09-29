#include "http_server.h"
#include "status_api.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace bitcoin2max {

HttpServer::HttpServer(const StatusApi& statusApi, uint16_t port)
    : statusApi_(statusApi), port_(port) {}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start() {
    if (running_) return true;

    serverFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd_ < 0) {
        std::cerr << "[HTTP] Unable to create socket: "
                  << std::strerror(errno) << "\n";
        return false;
    }

    int reuse = 1;
    ::setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR,
                 &reuse, sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port_);

    if (::bind(serverFd_,
               reinterpret_cast<sockaddr*>(&address),
               sizeof(address)) < 0) {
        std::cerr << "[HTTP] Unable to bind to 127.0.0.1:"
                  << port_ << ": " << std::strerror(errno) << "\n";
        ::close(serverFd_);
        serverFd_ = -1;
        return false;
    }

    if (::listen(serverFd_, 16) < 0) {
        std::cerr << "[HTTP] Unable to listen: "
                  << std::strerror(errno) << "\n";
        ::close(serverFd_);
        serverFd_ = -1;
        return false;
    }

    running_ = true;
    thread_ = std::thread(&HttpServer::run, this);

    std::cout << "[HTTP] Status API listening on http://127.0.0.1:"
              << port_ << "/status\n";

    return true;
}

void HttpServer::stop() {
    if (!running_.exchange(false)) {
        if (thread_.joinable()) thread_.join();
        return;
    }

    if (serverFd_ >= 0) {
        ::shutdown(serverFd_, SHUT_RDWR);
        ::close(serverFd_);
        serverFd_ = -1;
    }

    if (thread_.joinable()) thread_.join();
}

bool HttpServer::isRunning() const {
    return running_.load();
}

void HttpServer::run() {
    while (running_) {
        sockaddr_in clientAddress{};
        socklen_t clientLength = sizeof(clientAddress);

        int client = ::accept(
            serverFd_,
            reinterpret_cast<sockaddr*>(&clientAddress),
            &clientLength
        );

        if (client < 0) {
            if (running_) {
                std::cerr << "[HTTP] accept failed: "
                          << std::strerror(errno) << "\n";
            }
            continue;
        }

        char request[4096]{};
        const ssize_t received =
            ::recv(client, request, sizeof(request) - 1, 0);

        if (received > 0) {
            request[received] = '\0';

            const std::string req(request);

            if (req.rfind("GET /status ", 0) == 0) {
                const std::string body = statusApi_.getStatusJson();

                const std::string response =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/json\r\n"
                    "Cache-Control: no-store\r\n"
                    "Connection: close\r\n"
                    "Content-Length: " +
                    std::to_string(body.size()) +
                    "\r\n\r\n" + body;

                ::send(client, response.data(), response.size(), 0);
            } else {
                const std::string body =
                    "{\"error\":\"not found\"}";

                const std::string response =
                    "HTTP/1.1 404 Not Found\r\n"
                    "Content-Type: application/json\r\n"
                    "Connection: close\r\n"
                    "Content-Length: " +
                    std::to_string(body.size()) +
                    "\r\n\r\n" + body;

                ::send(client, response.data(), response.size(), 0);
            }
        }

        ::shutdown(client, SHUT_RDWR);
        ::close(client);
    }
}

} // namespace bitcoin2max
