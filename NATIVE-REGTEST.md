# Bitcoin2.0Max native coin: private regtest foundation

This is the first executable milestone toward an independent native coin.
`bitcoin2max_regtest` mines and validates a local, coinbase-only chain. It is
separate from `bitcoin2maxd`, Electrum, Bitcoin mainnet, and the website.
The existing daemon is still a networking/status framework, not a complete
native-coin consensus node.

## Build and run

Use the repository's normal C++17, CMake and OpenSSL prerequisites:

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j2
ctest --test-dir build --output-on-failure

# Create a private test chain and mine three blocks.
./build/bitcoin2max_regtest /tmp/bitcoin2max-regtest.chain --mine 3

# Restart: load and independently validate every saved block.
./build/bitcoin2max_regtest /tmp/bitcoin2max-regtest.chain

# Mine another 98 blocks: the first reward is now 100 blocks behind the tip.
./build/bitcoin2max_regtest /tmp/bitcoin2max-regtest.chain --mine 98
```

A second process can validate a copy of the chain independently:

```bash
cp /tmp/bitcoin2max-regtest.chain /tmp/bitcoin2max-regtest-copy.chain
./build/bitcoin2max_regtest /tmp/bitcoin2max-regtest-copy.chain
```

Both copies must report the same height, tip and issued amount. This proves
deterministic offline replay; it does **not** implement P2P synchronization.
Use a fresh path to reset the test chain. Corrupt or incomplete files fail
validation instead of being silently reset. A file lock prevents concurrent
instances from appending to the same path.

## Provisional issuance rules

| Rule | Private test chain |
|---|---|
| Units | 100,000,000 satoshis per coin |
| Genesis allocation | Zero; deterministic private-network anchor |
| Initial reward | 50 coins at height 1 |
| First halving | Height 210,000; reward becomes 25 coins |
| Later halvings | Every 210,000 blocks, rounded down to whole satoshis |
| Supply bound | 21 million coins |
| Reward age | 100 blocks behind the current tip for the mature-reward counter |
| Proof of work | SHA256d; fixed, deliberately easy target `0x207fffff` |
| Header timestamps | Genesis time plus 60 seconds per height |
| Mining speed | On demand; no real-time pacing |
| Harness limit | 100,000 blocks; at most 10,000 per command |

The complete provisional schedule issues 20,999,949.9769 coins: the genesis
reward is zero and each halving rounds down. The 21 million constant is a
ceiling, not a promise that precisely 21 million coins will be issued.
With actual one-minute blocks, 210,000 blocks would take approximately 146
days. These monetary rules are a development proposal, not a launched network.

Each test block contains one deterministic coinbase transaction. Its height is
committed in the input script; its output uses the test-only `OP_TRUE` script.
The header commits to that transaction and its parent hash. Validation checks
the expected reward, height commitment, payout script, Merkle root, header
parameters, proof of work and issuance bound before accepting the block.

The on-disk format stores heights, nonces and displayed block hashes. Replay
reconstructs the deterministic headers and coinbases and validates them. It is
not a general block database or a compatible Bitcoin `blk*.dat` file.

## What this milestone does not do

- No spendable wallet, private keys, signed transfers or UTXO database.
- No script interpreter, fees, mempool, forks, chainwork selection or reorgs.
- No difficulty retargeting or production timestamp rules.
- No P2P block relay, peer discovery or public testnet/mainnet deployment.
- No Ethereum bridge or backing by Bitcoin or ETH.

Issued and mature-reward counters are test-chain accounting, not wallet
balances. The existing `BitcoinMaxWallet` starts with a hard-coded 10,000-coin
demo balance; it is not connected to this chain and does not represent mined
coins. No public coin has been launched by this change.

## Remaining milestones

1. Implement persistent UTXOs, signed transactions, script validation and
   coinbase maturity enforcement at spend time.
2. Add a wallet whose balance comes entirely from validated UTXOs.
3. Implement peer block/transaction relay, cumulative-work chain selection,
   rollback and replay across reorgs.
4. Test two independent nodes mining and transferring matured rewards, including
   invalid signatures, double spends, excess rewards, restarts and forks.
5. Define separate public testnet/mainnet genesis blocks and network parameters,
   implement difficulty adjustment, and review consensus/security before launch.

Proof-of-work checks follow the target range and digest byte-order conventions
used by [Bitcoin Core](https://github.com/bitcoin/bitcoin/blob/master/src/pow.cpp).
This small harness is not a fork of Bitcoin Core's complete consensus engine.
