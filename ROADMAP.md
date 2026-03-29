# Bitcoin 2.0max — Project Roadmap

This document tracks the planned milestones and features for the Bitcoin 2.0max project.
Items are organised by release milestone.  Completed items are checked.

---

## Milestone 1 — Foundation (v2.0.0) ✅

Core infrastructure required to compile, run, and validate the node.

- [x] CMake-based build system (C++17)
- [x] Configurable block parameters (60-second target, 32 MiB max block size)
- [x] Electrum JSON-RPC client (independent of BOLDwallet)
- [x] Config file parser (`~/.bitcoin2max/bitcoin2max.conf`)
- [x] Basic P2P node loop
- [x] Unit tests for consensus parameters and config loading
- [x] CI workflow (build + test on Ubuntu)
- [x] Release & package workflow (binary tarballs, Docker image)
- [x] CODEOWNERS and GitHub label definitions

---

## Milestone 2 — Network Layer (v2.1.0)

Robust peer-to-peer networking with enhanced peer discovery and sync.

- [ ] Implement full peer handshake protocol
- [ ] Persistent peer address book
- [ ] Configurable maximum peer connections
- [ ] Asynchronous I/O via Boost.Asio (replace blocking socket calls)
- [ ] Tor integration (route all traffic via SOCKS5 proxy)
- [ ] DNS seed bootstrap for initial peer discovery
- [ ] Peer ban / penalty scoring

---

## Milestone 3 — Block Validation (v2.2.0)

Full script validation and consensus rule enforcement.

- [ ] Script interpreter (OP codes)
- [ ] Transaction input/output validation
- [ ] Merkle tree verification
- [ ] Chain-tip selection (most cumulative work)
- [ ] Orphan block handling
- [ ] Checkpoint database
- [ ] Block-height reorganisation (reorg) logic

---

## Milestone 4 — Mempool & Relay (v2.3.0)

Transaction mempool, fee estimation, and transaction relay.

- [ ] In-memory transaction pool
- [ ] Fee-rate ordering and eviction policy
- [ ] RBF (Replace-By-Fee) support
- [ ] Transaction relay to peers
- [ ] Mempool persistence across restarts
- [ ] Fee estimation API

---

## Milestone 5 — Wallet API (v2.4.0)

Electrum-compatible wallet API served over the Electrum protocol.

- [ ] Electrum server-side protocol (TCP JSON-RPC)
- [ ] Address-to-UTXO index
- [ ] Transaction history per address
- [ ] Balance query
- [ ] Raw transaction broadcast endpoint
- [ ] SSL/TLS support for remote wallet connections

---

## Milestone 6 — Performance & Scalability (v3.0.0)

Optimisations to fully exploit the 32 MiB block capacity.

- [ ] UTXO set (LevelDB or RocksDB backend)
- [ ] Block and UTXO cache tuning
- [ ] Parallel script validation (thread pool)
- [ ] SIMD-accelerated hash routines (SHA-256, RIPEMD-160)
- [ ] Block index stored on disk (fast restart)
- [ ] Benchmarking harness and regression CI job
- [ ] Profile-guided optimisation (PGO) build option

---

## Milestone 7 — Security & Hardening (ongoing)

Continuous security improvements.

- [ ] Fuzz testing (libFuzzer / AFL++) for parser inputs
- [ ] CodeQL code-scanning in CI
- [ ] Address Sanitizer / UBSan CI job
- [ ] Responsible disclosure policy (SECURITY.md)
- [ ] Regular dependency audits
- [ ] Binary reproducible builds

---

## Milestone 8 — Developer Experience (ongoing)

Make contributing easy and enjoyable.

- [ ] Contribution guide (CONTRIBUTING.md)
- [ ] Developer documentation (docs/ directory)
- [ ] Pre-commit hooks (clang-format, clang-tidy)
- [ ] `docker-compose` development environment
- [ ] VSCode devcontainer configuration
- [ ] Changelog automation (conventional commits)

---

## Future / Under Discussion

Ideas that are not yet scheduled for a specific milestone.

- Lightning Network channel support
- REST/gRPC control API
- BIP-340 Schnorr signature support
- Taproot / Tapscript
- Mobile-friendly light client mode (SPV)
- GUI wallet front-end

---

*Last updated: 2026-03-29*

> To propose changes to this roadmap, open a GitHub issue with the
> **roadmap** label.
