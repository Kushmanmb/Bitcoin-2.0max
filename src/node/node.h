#pragma once
// src/node/node.h
//
// Bitcoin 2.0max node — manages the event loop, P2P networking, block
// validation, and optional Electrum server connectivity.

#include "bitcoin2max/config.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace bitcoin2max {

class ElectrumClient;
namespace net { class Peer; }

class Node {
public:
    explicit Node(const Config& cfg);
    ~Node();

    // Non-copyable, non-movable
    Node(const Node&)            = delete;
    Node& operator=(const Node&) = delete;

    /// Start the node (returns immediately; runs in background threads).
    void start();

    /// Request a graceful shutdown.
    void stop();

    /// Block the calling thread until the node has stopped.
    void join();

    // ── Status accessors ─────────────────────────────────────────────────────
    uint64_t bestHeight()    const { return bestHeight_.load(); }
    bool     isRunning()     const { return running_.load(); }

    /// Number of peers that have completed the opening handshake.
    size_t peerCount() const;

private:
    const Config& cfg_;
    std::atomic<bool>     running_{false};
    std::atomic<uint64_t> bestHeight_{0};

    std::unique_ptr<ElectrumClient> electrum_;

    // ── P2P peer management ───────────────────────────────────────────────────
    int                                         listenFd_{-1};
    std::vector<std::unique_ptr<net::Peer>>     peers_;
    mutable std::mutex                          peersMutex_;

    bool startListening();
    void acceptLoop();

    void mainLoop();
    void connectToElectrum();
    void logBanner() const;
};

} // namespace bitcoin2max
