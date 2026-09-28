#pragma once

// Bitcoin 2.0max node
// Central runtime state for networking, chain status,
// transaction pool statistics and Electrum connectivity.

#include "bitcoin2max/config.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace bitcoin2max {

class ElectrumClient;

namespace net {
class Peer;
}

class Node {
public:
    explicit Node(const Config& cfg);
    ~Node();

    // Node cannot be copied or moved.
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    // Start Bitcoin 2.0max.
    void start();

    // Request graceful shutdown.
    void stop();

    // Wait until the node stops.
    void join();


    // =====================================================
    // NODE STATUS
    // =====================================================

    bool isRunning() const {
        return running_.load();
    }

    uint64_t bestHeight() const {
        return bestHeight_.load();
    }

    uint64_t mempoolSize() const {
        return mempoolSize_.load();
    }

    bool electrumConnected() const;

    size_t peerCount() const;


    // =====================================================
    // CHAIN STATE
    // =====================================================

    void setBestHeight(uint64_t height);

    void setBestBlockHash(
        const std::string& hash
    );

    std::string bestBlockHash() const;


    // =====================================================
    // MEMPOOL STATE
    // =====================================================

    void setMempoolSize(
        uint64_t size
    );


private:

    const Config& cfg_;


    // =====================================================
    // RUNTIME STATE
    // =====================================================

    std::atomic<bool>
        running_{false};

    std::atomic<uint64_t>
        bestHeight_{0};

    std::atomic<uint64_t>
        mempoolSize_{0};


    // =====================================================
    // CHAIN TIP
    // =====================================================

    mutable std::mutex
        chainMutex_;

    std::string
        bestBlockHash_;


    // =====================================================
    // ELECTRUM
    // =====================================================

    std::unique_ptr<ElectrumClient>
        electrum_;


    // =====================================================
    // P2P NETWORK
    // =====================================================

    int listenFd_{-1};

    std::vector<
        std::unique_ptr<net::Peer>
    > peers_;

    mutable std::mutex
        peersMutex_;


    // =====================================================
    // INTERNAL NODE FUNCTIONS
    // =====================================================

    bool startListening();

    void acceptLoop();

    void mainLoop();

    void connectToElectrum();

    void updateChainState();

    void logBanner() const;
};

} // namespace bitcoin2max
