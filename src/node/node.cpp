// src/node/node.cpp
//
// Bitcoin 2.0max node implementation.
// Runs independently — does NOT depend on BOLDwallet.  Wallet functionality
// is provided externally via the Electrum protocol on 127.0.0.1:9050.

#include "node.h"

#include "bitcoin2max/params.h"
#include "../electrum/electrum_client.h"
#include "../net/peer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

namespace bitcoin2max {

// ── ctor / dtor ───────────────────────────────────────────────────────────────

Node::Node(const Config& cfg) : cfg_(cfg) {}

Node::~Node() {
    stop();
    join();
}

// ── public interface ──────────────────────────────────────────────────────────

void Node::start() {
    if (running_.load()) return;

    logBanner();
    running_.store(true);

    if (cfg_.electrum_enabled) {
        connectToElectrum();
    }

    // Start the P2P listen socket and accept loop.
    if (startListening()) {
        std::thread([this] { acceptLoop(); }).detach();
    }

    // Start the main loop in a background thread.
    std::thread([this] { mainLoop(); }).detach();
}

void Node::stop() {
    running_.store(false);
    if (electrum_) electrum_->disconnect();
    if (listenFd_ >= 0) {
        ::close(listenFd_);
        listenFd_ = -1;
    }
}

void Node::join() {
    // Poll until main loop exits.  A production implementation would use
    // condition variables; this is kept simple for clarity.
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// ── private helpers ───────────────────────────────────────────────────────────

void Node::logBanner() const {
    std::cout << "\n"
              << "  ██████╗ ██╗████████╗ ██████╗ ██████╗ ██╗███╗  ██╗   ██████╗  ██████╗ \n"
              << "  ██╔══██╗██║╚══██╔══╝██╔════╝██╔═══██╗██║████╗ ██║  ╚════██╗ ██╔═████╗\n"
              << "  ██████╔╝██║   ██║   ██║     ██║   ██║██║██╔██╗██║   █████╔╝ ██║██╔██║\n"
              << "  ██╔══██╗██║   ██║   ██║     ██║   ██║██║██║╚████║  ██╔═══╝  ████╔╝██║\n"
              << "  ██████╔╝██║   ██║   ╚██████╗╚██████╔╝██║██║ ╚███║  ███████╗ ╚██████╔╝\n"
              << "  ╚═════╝ ╚═╝   ╚═╝    ╚═════╝ ╚═════╝ ╚═╝╚═╝  ╚══╝  ╚══════╝  ╚═════╝ \n"
              << "  Bitcoin 2.0max  —  supercharged block speed & scalability\n"
              << "  Block target  : " << cfg_.target_block_time << " seconds\n"
              << "  Max block size: " << cfg_.max_block_size / (1024*1024) << " MiB\n"
              << "  P2P port      : " << cfg_.p2p_port << "\n"
              << "  RPC port      : " << cfg_.rpc_port << "\n"
              << "  Electrum      : "
              << (cfg_.electrum_enabled
                      ? cfg_.electrum_host + ":" + std::to_string(cfg_.electrum_port)
                      : "disabled")
              << "\n\n";
}

void Node::connectToElectrum() {
    electrum_ = std::make_unique<ElectrumClient>(cfg_);

    uint32_t attempts = 0;
    while (attempts < electrum::MAX_RECONNECT_ATTEMPTS) {
        if (electrum_->connect()) {
            auto ver = electrum_->serverVersion();
            std::cout << "[Node] Electrum server version: " << ver << "\n";
            break;
        }
        ++attempts;
        std::cerr << "[Node] Electrum connect attempt " << attempts
                  << " failed; retrying in 5 s…\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    if (!electrum_->isConnected()) {
        std::cerr << "[Node] WARNING: Could not connect to Electrum server at "
                  << cfg_.electrum_host << ":" << cfg_.electrum_port
                  << ".  Running without wallet relay.\n";
    }
}

void Node::mainLoop() {
    std::cout << "[Node] Main loop started.\n";

    while (running_.load()) {
        // In a full implementation this loop drives:
        //  - P2P message dispatch
        //  - Block download & validation
        //  - Mempool management
        //  - Electrum subscription updates

        if (electrum_ && electrum_->isConnected()) {
            auto header = electrum_->getBestBlockHeader();
            if (!header.empty()) {
                bestHeight_.fetch_add(0); // placeholder — parse real height
                // std::cout << "[Node] Best header: " << header << "\n";
            }
        }

        std::this_thread::sleep_for(
            std::chrono::seconds(cfg_.target_block_time));
    }

    std::cout << "[Node] Main loop stopped.\n";
    running_.store(false);
}

// ── peerCount ─────────────────────────────────────────────────────────────────

size_t Node::peerCount() const {
    std::lock_guard<std::mutex> lk(peersMutex_);
    return peers_.size();
}

// ── startListening ────────────────────────────────────────────────────────────

bool Node::startListening() {
    listenFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        std::cerr << "[Node] socket(): " << std::strerror(errno) << "\n";
        return false;
    }

    int opt = 1;
    ::setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(cfg_.p2p_port);

    if (::bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[Node] bind() on port " << cfg_.p2p_port
                  << ": " << std::strerror(errno) << "\n";
        ::close(listenFd_);
        listenFd_ = -1;
        return false;
    }

    if (::listen(listenFd_, network::LISTEN_BACKLOG) < 0) {
        std::cerr << "[Node] listen(): " << std::strerror(errno) << "\n";
        ::close(listenFd_);
        listenFd_ = -1;
        return false;
    }

    std::cout << "[Node] Listening for peers on port " << cfg_.p2p_port << "\n";
    return true;
}

// ── acceptLoop ────────────────────────────────────────────────────────────────

void Node::acceptLoop() {
    while (running_.load()) {
        if (listenFd_ < 0) break;

        // Use select() with a 1-second timeout so we can check running_ and
        // react to stop() closing listenFd_.
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(listenFd_, &rfds);
        timeval tv{1, 0};

        int r = ::select(listenFd_ + 1, &rfds, nullptr, nullptr, &tv);
        if (r <= 0) continue;

        sockaddr_in peerAddr{};
        socklen_t   addrLen = sizeof(peerAddr);
        int peerFd = ::accept(listenFd_,
                              reinterpret_cast<sockaddr*>(&peerAddr),
                              &addrLen);
        if (peerFd < 0) {
            if (running_.load())
                std::cerr << "[Node] accept(): " << std::strerror(errno) << "\n";
            continue;
        }

        char ipStr[INET_ADDRSTRLEN] = {};
        ::inet_ntop(AF_INET, &peerAddr.sin_addr, ipStr, sizeof(ipStr));
        std::cout << "[Node] Accepted connection from " << ipStr
                  << ":" << ntohs(peerAddr.sin_port) << "\n";

        // Perform the handshake in a detached thread so the accept loop keeps
        // running while we wait for the remote's messages.
        int32_t  height  = static_cast<int32_t>(bestHeight_.load());
        uint16_t ourPort = cfg_.p2p_port;
        std::thread([this, peerFd, height, ourPort]() {
            auto peer = std::make_unique<net::Peer>(peerFd, /*outbound=*/false);
            if (peer->doHandshake(height, ourPort)) {
                std::lock_guard<std::mutex> lk(peersMutex_);
                peers_.push_back(std::move(peer));
            }
        }).detach();
    }

    std::cout << "[Node] Accept loop stopped.\n";
}

} // namespace bitcoin2max
