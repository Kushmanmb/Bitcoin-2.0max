// src/electrum/electrum_client.cpp
//
// Synchronous Electrum JSON-RPC client using POSIX sockets.
// Connects to 127.0.0.1:9050 by default (local Electrum server).

#include "electrum_client.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace bitcoin2max {

// ── ctor/dtor ─────────────────────────────────────────────────────────────────

ElectrumClient::ElectrumClient(const Config& cfg) : cfg_(cfg) {}

ElectrumClient::~ElectrumClient() {
    disconnect();
}

ElectrumClient::ElectrumClient(ElectrumClient&& o) noexcept
    : cfg_(o.cfg_), fd_(o.fd_), nextId_(o.nextId_) {
    o.fd_ = -1;
}

ElectrumClient& ElectrumClient::operator=(ElectrumClient&& o) noexcept {
    if (this != &o) {
        disconnect();
        fd_      = o.fd_;
        nextId_  = o.nextId_;
        o.fd_    = -1;
    }
    return *this;
}

// ── connect / disconnect ──────────────────────────────────────────────────────

bool ElectrumClient::connect() {
    if (fd_ >= 0) return true;  // already connected

    const std::string& host = cfg_.electrum_host;
    uint16_t           port = cfg_.electrum_port;

    struct addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = nullptr;
    std::string portStr  = std::to_string(port);

    int rc = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res);
    if (rc != 0) {
        std::cerr << "[Electrum] getaddrinfo(" << host << ":" << port
                  << "): " << gai_strerror(rc) << "\n";
        return false;
    }

    int sock = -1;
    for (auto* p = res; p != nullptr; p = p->ai_next) {
        sock = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock < 0) continue;

        if (::connect(sock, p->ai_addr, p->ai_addrlen) == 0) break;

        ::close(sock);
        sock = -1;
    }
    freeaddrinfo(res);

    if (sock < 0) {
        std::cerr << "[Electrum] Failed to connect to " << host
                  << ":" << port << " — " << std::strerror(errno) << "\n";
        return false;
    }

    fd_ = sock;
    std::cout << "[Electrum] Connected to " << host << ":" << port << "\n";
    return true;
}

void ElectrumClient::disconnect() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
        std::cout << "[Electrum] Disconnected.\n";
    }
}

// ── low-level send/receive ────────────────────────────────────────────────────

std::string ElectrumClient::sendRequest(const std::string& json) {
    if (fd_ < 0) {
        std::cerr << "[Electrum] sendRequest: not connected.\n";
        return {};
    }

    // Electrum protocol: each message terminated by a newline.
    std::string msg = json + "\n";
    ssize_t sent = ::send(fd_, msg.c_str(), msg.size(), 0);
    if (sent < 0) {
        std::cerr << "[Electrum] send error: " << std::strerror(errno) << "\n";
        return {};
    }

    // Read response (newline-terminated).
    std::string response;
    char buf[4096];
    while (true) {
        ssize_t n = ::recv(fd_, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            if (n < 0) std::cerr << "[Electrum] recv error: "
                                  << std::strerror(errno) << "\n";
            break;
        }
        buf[n] = '\0';
        response += buf;
        if (response.find('\n') != std::string::npos) break;
    }

    return response;
}

std::string ElectrumClient::buildRequest(const std::string& method,
                                          const std::string& params) {
    std::ostringstream oss;
    oss << "{\"jsonrpc\":\"2.0\","
        << "\"id\":"     << nextId_++ << ","
        << "\"method\":\"" << method << "\","
        << "\"params\":"  << params  << "}";
    return oss.str();
}

// ── Electrum protocol methods ─────────────────────────────────────────────────

std::string ElectrumClient::serverVersion() {
    std::string req = buildRequest("server.version",
                                   "[\"Bitcoin2Max/2.0\", \"1.4\"]");
    return sendRequest(req);
}

std::string ElectrumClient::getBestBlockHeader() {
    std::string req = buildRequest("blockchain.headers.subscribe");
    return sendRequest(req);
}

std::string ElectrumClient::getTransaction(const std::string& txid) {
    std::string req = buildRequest("blockchain.transaction.get",
                                   "[\"" + txid + "\", false]");
    return sendRequest(req);
}

std::string ElectrumClient::getScriptHashBalance(const std::string& scriptHash) {
    std::string req = buildRequest("blockchain.scripthash.get_balance",
                                   "[\"" + scriptHash + "\"]");
    return sendRequest(req);
}

std::string ElectrumClient::broadcastTransaction(const std::string& rawTxHex) {
    std::string req = buildRequest("blockchain.transaction.broadcast",
                                   "[\"" + rawTxHex + "\"]");
    return sendRequest(req);
}

} // namespace bitcoin2max
