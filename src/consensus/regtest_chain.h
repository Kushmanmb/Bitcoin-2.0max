#pragma once

#include "bitcoin2max/transaction.h"
#include "crypto/block_hash.h"
#include <iosfwd>

namespace bitcoin2max::regtest {

// These parameters never enter bitcoin2maxd or its public P2P protocol.
inline constexpr uint32_t POW_BITS = 0x207fffffu;
inline constexpr uint32_t GENESIS_TIME = 1790812800u;
inline constexpr const char* GENESIS_HASH =
    "7dbd3b9ed2b7d9f74a4113c7e9af353a2d2ba226d3e23daea312275b42b11863";
inline constexpr uint64_t MAX_TEST_HEIGHT = 100'000;

struct Block {
    crypto::BlockHeader header;
    Transaction coinbase;
};

Block blockTemplate(uint64_t height, const crypto::Hash256& previous);
Block mineBlock(uint64_t height, const crypto::Hash256& previous);
std::string displayHash(const crypto::Hash256& raw);

// A linear, coinbase-only test chain. No spends, forks, fees, wallet, script
// interpreter or network synchronization are implemented by this harness.
class Chain {
public:
    Chain();
    ValidationResult append(const Block& block);
    uint64_t height() const { return height_; }
    uint64_t issued() const { return issued_; }
    const crypto::Hash256& tip() const { return tip_; }
    uint64_t matureRewards() const;
    static Chain load(std::istream& input);
    static void writeRecord(std::ostream& output, uint64_t height,
                            const Block& block);
private:
    uint64_t height_{0};
    uint64_t issued_{0};
    crypto::Hash256 tip_{};
};

} // namespace bitcoin2max::regtest
