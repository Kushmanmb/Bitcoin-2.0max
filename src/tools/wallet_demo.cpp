// src/tools/wallet_demo.cpp
//
// Wallet demo tool: creates a BitcoinMaxWallet, adds Bitcoin, and reports the
// resulting balance.  Used by the wallet CI pipeline to verify that the
// add-Bitcoin-to-wallet path builds and runs correctly end-to-end.

#include "bitcoin2max/wallet.h"

#include <cstdlib>
#include <iostream>

int main() {
    using namespace bitcoin2max;

    BitcoinMaxWallet wallet;
    wallet.setLabel("CI-demo-wallet");

    std::cout << "[wallet_demo] Initial balance : "
              << wallet.balanceBTC() << " BTC ("
              << wallet.balanceSatoshis() << " sat)\n";

    // Add 1 000 BTC to the wallet — the core operation exercised by this tool.
    constexpr uint64_t DEMO_BTC_AMOUNT = 1'000ULL;
    wallet.addBTC(DEMO_BTC_AMOUNT);

    std::cout << "[wallet_demo] Added           : " << DEMO_BTC_AMOUNT << " BTC\n";
    std::cout << "[wallet_demo] Balance after   : "
              << wallet.balanceBTC() << " BTC ("
              << wallet.balanceSatoshis() << " sat)\n";

    const uint64_t expected = (WALLET_INITIAL_BTC + DEMO_BTC_AMOUNT) * SATOSHIS_PER_BTC;
    if (wallet.balanceSatoshis() != expected) {
        std::cerr << "[wallet_demo] ERROR: expected " << expected
                  << " sat but got " << wallet.balanceSatoshis() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "[wallet_demo] OK — Bitcoin successfully added to wallet.\n";
    return EXIT_SUCCESS;
}
