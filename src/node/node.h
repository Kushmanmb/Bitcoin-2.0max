#pragma once
// src/node/node.h
//
// Bitcoin 2.0max node — manages the event loop, P2P networking, block
// validation, and optional Electrum server connectivity.

#include "bitcoin2max/config.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace bitcoin2max {

class ElectrumClient;

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

private:
    const Config& cfg_;
    std::atomic<bool>     running_{false};
    std::atomic<uint64_t> bestHeight_{0};

    std::unique_ptr<ElectrumClient> electrum_;

    void mainLoop();
    void connectToElectrum();
    void logBanner() const;
};

} // namespace bitcoin2max
