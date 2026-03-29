#pragma once
// include/bitcoin2max/wallet.h
//
// BitcoinMaxWallet — simple in-process wallet that tracks a satoshi balance.
// The wallet is initialised with 10 000 BTC (1 000 000 000 000 satoshis) as
// required by the project specification.

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace bitcoin2max {

/// Satoshis in one Bitcoin.
static constexpr uint64_t SATOSHIS_PER_BTC = 100'000'000ULL;

/// Initial endowment placed in every new BitcoinMaxWallet (10 000 BTC).
static constexpr uint64_t WALLET_INITIAL_BTC = 10'000ULL;

class BitcoinMaxWallet {
public:
    /// Construct a wallet pre-loaded with WALLET_INITIAL_BTC bitcoin.
    BitcoinMaxWallet()
        : balance_satoshis_(WALLET_INITIAL_BTC * SATOSHIS_PER_BTC) {}

    // ── Balance accessors ─────────────────────────────────────────────────────

    /// Current balance in satoshis.
    uint64_t balanceSatoshis() const { return balance_satoshis_; }

    /// Current balance in whole BTC (truncated).
    uint64_t balanceBTC() const { return balance_satoshis_ / SATOSHIS_PER_BTC; }

    // ── Mutations ─────────────────────────────────────────────────────────────

    /// Add `amount` satoshis to the wallet.
    /// @throws std::invalid_argument if amount is zero.
    /// @throws std::overflow_error   if the addition would overflow.
    void addSatoshis(uint64_t amount) {
        if (amount == 0) {
            throw std::invalid_argument("Cannot add zero satoshis");
        }
        if (amount > std::numeric_limits<uint64_t>::max() - balance_satoshis_) {
            throw std::overflow_error("Balance overflow");
        }
        balance_satoshis_ += amount;
    }

    /// Add `btc` whole bitcoin to the wallet.
    /// @throws std::invalid_argument if btc is zero.
    /// @throws std::overflow_error   if the multiplication or addition would overflow.
    void addBTC(uint64_t btc) {
        if (btc == 0) {
            throw std::invalid_argument("Cannot add zero BTC");
        }
        if (btc > std::numeric_limits<uint64_t>::max() / SATOSHIS_PER_BTC) {
            throw std::overflow_error("BTC amount too large: satoshi conversion would overflow");
        }
        addSatoshis(btc * SATOSHIS_PER_BTC);
    }

    /// Deduct `amount` satoshis from the wallet.
    /// @throws std::invalid_argument if amount is zero.
    /// @throws std::runtime_error    if insufficient funds.
    void spendSatoshis(uint64_t amount) {
        if (amount == 0) {
            throw std::invalid_argument("Cannot spend zero satoshis");
        }
        if (amount > balance_satoshis_) {
            throw std::runtime_error("Insufficient funds");
        }
        balance_satoshis_ -= amount;
    }

    /// Human-readable label shown in logs and UI.
    std::string label() const { return label_; }
    void        setLabel(const std::string& lbl) { label_ = lbl; }

private:
    uint64_t    balance_satoshis_;
    std::string label_{"BitcoinMaxWallet"};
};

} // namespace bitcoin2max
