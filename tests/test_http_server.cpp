#include "api/http_server.h"
#include "api/status_api.h"
#include "config/config.h"
#include "node/node.h"

#include <arpa/inet.h>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

TEST_CASE("HttpServer serves status endpoint") {
    bitcoin2max::Config config;
    bitcoin2max::Node node(config);
    bitcoin2max::StatusApi statusApi(node);

    // Use a test-only port so we don't interfere with the normal API.
    bitcoin2max::HttpServer server(statusApi, 18080);

    REQUIRE(server.start());

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(fd >= 0);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(18080);
    REQUIRE(::inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) == 1);

    REQUIRE(::connect(
        fd,
        reinterpret_cast<sockaddr*>(&address),
        sizeof(address)) == 0);

    const std::string request =
        "GET /status HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: close\r\n\r\n";

    REQUIRE(::send(fd, request.data(), request.size(), 0) >= 0);

    std::string response;
    char buffer[4096];

    ssize_t count;
    while ((count = ::recv(fd, buffer, sizeof(buffer), 0)) > 0) {
        response.append(buffer, static_cast<std::size_t>(count));
    }

    ::close(fd);
    server.stop();

    REQUIRE(response.find("HTTP/1.1 200 OK") != std::string::npos);
    REQUIRE(response.find("application/json") != std::string::npos);
    REQUIRE(response.find("\"status\":\"offline\"") != std::string::npos);
    REQUIRE(response.find("\"blockHeight\":0") != std::string::npos);
}
