#pragma once

#include "bitcoin2max/params.h"
#include <cstdint>

namespace bitcoin2max::consensus {

// Provisional native-coin monetary policy, used only by the isolated regtest
// harness until a complete production consensus engine exists.
inline constexpr uint64_t COIN = 100'000'000ULL;
inline constexpr uint64_t INITIAL_SUBSIDY = 50ULL * COIN;
inline constexpr uint64_t HALVING_INTERVAL = 210'000ULL;

// Height zero is an unspendable, zero-allocation genesis anchor.
constexpr uint64_t blockSubsidy(uint64_t height) {
    if (height == 0) return 0;
    const auto halvings = height / HALVING_INTERVAL;
    return halvings >= 64 ? 0 : INITIAL_SUBSIDY >> halvings;
}

// Fees recycle existing money; only the subsidy counts as new issuance.
constexpr bool validCoinbaseReward(uint64_t height, uint64_t reward,
                                   uint64_t fees, uint64_t issued) {
    if (issued > MAX_MONEY_SATOSHIS || fees > MAX_MONEY_SATOSHIS ||
        reward > MAX_MONEY_SATOSHIS) return false;
    const auto subsidy = blockSubsidy(height);
    if (subsidy > MAX_MONEY_SATOSHIS - issued) return false;
    // Both operands are bounded above by MAX_MONEY, well below UINT64_MAX.
    return reward <= subsidy + fees;
}

} // namespace bitcoin2max::consensus
