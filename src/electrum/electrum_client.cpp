// src/electrum/electrum_client.cpp
//
// Synchronous Electrum JSON-RPC client using POSIX sockets.
// Connects to 127.0.0.1:9050 by default (local Electrum server).

#include "electrum_client.h"
#include "../platform/socket_compat.h"

#ifndef _WIN32
#include <netdb.h>
#endif

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace bitcoin2max {

// ── ctor/dtor
// ─────────────────────────────────────────────────────────────────

ElectrumClient::ElectrumClient(const Config &cfg) : cfg_(cfg) {}

ElectrumClient::~ElectrumClient() { disconnect(); }

ElectrumClient::ElectrumClient(ElectrumClient &&o) noexcept
    : cfg_(o.cfg_), fd_(o.fd_), nextId_(o.nextId_) {
  o.fd_ = INVALID_SOCKET_HANDLE;
}

ElectrumClient &ElectrumClient::operator=(ElectrumClient &&o) noexcept {
  if (this != &o) {
    disconnect();
    fd_ = o.fd_;
    nextId_ = o.nextId_;
    o.fd_ = INVALID_SOCKET_HANDLE;
  }
  return *this;
}

// ── connect / disconnect
// ──────────────────────────────────────────────────────

bool ElectrumClient::connect() {
  if (socketValid(fd_))
    return true; // already connected

  const std::string &host = cfg_.electrum_host;
  uint16_t port = cfg_.electrum_port;

  struct addrinfo hints {};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *res = nullptr;
  std::string portStr = std::to_string(port);

  int rc = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res);
  if (rc != 0) {
    std::cerr << "[Electrum] getaddrinfo(" << host << ":" << port
              << "): " << gai_strerror(rc) << "\n";
    return false;
  }

  SocketHandle sock = INVALID_SOCKET_HANDLE;
  for (auto *p = res; p != nullptr; p = p->ai_next) {
    sock = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (!socketValid(sock))
      continue;

    if (::connect(sock, p->ai_addr, p->ai_addrlen) == 0)
      break;

    closeSocket(sock);
    sock = INVALID_SOCKET_HANDLE;
  }
  freeaddrinfo(res);

  if (!socketValid(sock)) {
    std::cerr << "[Electrum] Failed to connect to " << host << ":" << port
              << " — " << std::strerror(errno) << "\n";
    return false;
  }

  fd_ = sock;
  std::cout << "[Electrum] Connected to " << host << ":" << port << "\n";
  return true;
}

void ElectrumClient::disconnect() {
  if (socketValid(fd_)) {
    closeSocket(fd_);
    fd_ = INVALID_SOCKET_HANDLE;
    std::cout << "[Electrum] Disconnected.\n";
  }
}

// ── low-level send/receive
// ────────────────────────────────────────────────────

std::string ElectrumClient::sendRequest(const std::string &json) {
  if (!socketValid(fd_)) {
    std::cerr << "[Electrum] sendRequest: not connected.\n";
    return {};
  }

  // Electrum protocol: each message terminated by a newline.
  std::string msg = json + "\n";
  int sent = ::send(fd_, msg.c_str(), static_cast<int>(msg.size()), SOCKET_SEND_FLAGS);
  if (sent < 0) {
    std::cerr << "[Electrum] send error: " << std::strerror(errno) << "\n";
    return {};
  }

  // Read response (newline-terminated).
  std::string response;
  char buf[4096];
  while (true) {
    int n = ::recv(fd_, buf, static_cast<int>(sizeof(buf) - 1), 0);
    if (n <= 0) {
      if (n < 0)
        std::cerr << "[Electrum] recv error: " << std::strerror(errno) << "\n";
      break;
    }
    buf[n] = '\0';
    response += buf;
    if (response.find('\n') != std::string::npos)
      break;
  }

  return response;
}

std::string ElectrumClient::buildRequest(const std::string &method,
                                         const std::string &params) {
  std::ostringstream oss;
  oss << "{\"jsonrpc\":\"2.0\"," << "\"id\":" << nextId_++ << ","
      << "\"method\":\"" << method << "\"," << "\"params\":" << params << "}";
  return oss.str();
}

std::string ElectrumClient::extractStringResult(const std::string &response) {
  const std::string key = "\"result\"";
  const std::size_t keyPos = response.find(key);
  if (keyPos == std::string::npos) {
    return {};
  }

  const std::size_t colonPos = response.find(':', keyPos + key.size());
  if (colonPos == std::string::npos) {
    return {};
  }

  const std::size_t valueStart =
      response.find_first_not_of(" \t\n\r", colonPos + 1);
  if (valueStart == std::string::npos || response[valueStart] != '"') {
    return {};
  }

  bool escaped = false;
  for (std::size_t i = valueStart + 1; i < response.size(); ++i) {
    if (escaped) {
      escaped = false;
    } else if (response[i] == '\\') {
      escaped = true;
    } else if (response[i] == '"') {
      return response.substr(valueStart + 1, i - valueStart - 1);
    }
  }

  return {};
}

// ── Electrum protocol methods
// ─────────────────────────────────────────────────

std::string ElectrumClient::serverVersion() {
  std::string req =
      buildRequest("server.version", "[\"Bitcoin2Max/2.0\", \"1.4\"]");
  return sendRequest(req);
}

std::string ElectrumClient::getBestBlockHeader() {
  std::string req = buildRequest("blockchain.headers.subscribe");
  return sendRequest(req);
}
uint64_t ElectrumClient::getBestBlockHeight() {
  const std::string response = getBestBlockHeader();

  if (response.empty()) {
    return 0;
  }

  const std::string key = "\"height\"";
  const std::size_t keyPos = response.find(key);

  if (keyPos == std::string::npos) {
    std::cerr << "[Electrum] Header response has no height.\n";
    return 0;
  }

  const std::size_t colonPos = response.find(':', keyPos + key.size());

  if (colonPos == std::string::npos) {
    return 0;
  }

  std::size_t start = response.find_first_of("0123456789", colonPos + 1);

  if (start == std::string::npos) {
    return 0;
  }

  const std::size_t end = response.find_first_not_of("0123456789", start);

  try {
    return std::stoull(response.substr(start, end - start));
  } catch (const std::exception &e) {
    std::cerr << "[Electrum] Invalid block height: " << e.what() << "\n";

    return 0;
  }
}

std::string ElectrumClient::getBlockHeaderHex(uint64_t height) {
  const std::string req = buildRequest("blockchain.block.header",
                                       "[" + std::to_string(height) + "]");

  const std::string response = sendRequest(req);

  if (response.empty()) {
    return {};
  }

  const std::string result = extractStringResult(response);
  if (result.empty()) {
    std::cerr << "[Electrum] Block header response has no result.\n";
    return {};
  }

  return result;
}
std::string ElectrumClient::getBestBlockHeaderHex() {
  const std::string response = getBestBlockHeader();

  if (response.empty()) {
    return {};
  }

  const std::string key = "\"hex\"";
  const std::size_t keyPos = response.find(key);

  if (keyPos == std::string::npos) {
    std::cerr << "[Electrum] Header response has no hex field.\n";

    return {};
  }

  const std::size_t colonPos = response.find(':', keyPos + key.size());

  if (colonPos == std::string::npos) {
    return {};
  }

  const std::size_t quoteStart = response.find('"', colonPos + 1);

  if (quoteStart == std::string::npos) {
    return {};
  }

  const std::size_t quoteEnd = response.find('"', quoteStart + 1);

  if (quoteEnd == std::string::npos) {
    return {};
  }

  return response.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
}

std::string ElectrumClient::getTransaction(const std::string &txid) {
  std::string req =
      buildRequest("blockchain.transaction.get", "[\"" + txid + "\", false]");
  return sendRequest(req);
}

std::string
ElectrumClient::getScriptHashBalance(const std::string &scriptHash) {
  std::string req = buildRequest("blockchain.scripthash.get_balance",
                                 "[\"" + scriptHash + "\"]");
  return sendRequest(req);
}

std::string ElectrumClient::broadcastTransaction(const std::string &rawTxHex) {
  std::string req = buildRequest("blockchain.transaction.broadcast",
                                 "[\"" + rawTxHex + "\"]");
  return sendRequest(req);
}

} // namespace bitcoin2max

