# Bitcoin2.0Max + Ethereum — Project Roadmap

Last updated: September 30, 2026

Our goal is to build useful blockchain tools, a personal
workspace, and independently verifiable native-chain infrastructure.

Milestones represent development priorities, not release dates.
A working interface does not establish production readiness.

## Current Position

- The public website is hosted on GitHub Pages.
- Bitcoin market data and explorer features are implemented.
- Ethereum network monitoring and address lookup are implemented.
- The personal dashboard includes interfaces for accounts,
  portfolios, saved addresses, transactions, notes, and chat.
- Public account-service deployment remains unfinished.
- The C++ node includes startup, configuration, incoming peer
  connections, an Electrum client, and local status endpoints.
- Native-chain synchronization and consensus remain incomplete.

Implemented features still require ongoing testing.
Completed milestones must include evidence of their exit criteria.

---

## 1. Website and Network Dashboard

Status: In progress

- [ ] Verify consistent Bitcoin2.0Max + Ethereum branding.
- [ ] Verify matching Bitcoin and Ethereum price cards.
- [ ] Verify both price charts and their data sources.
- [ ] Test dropdown navigation on desktop and mobile.
- [ ] Test dark, light, and seasonal themes.
- [ ] Preserve keyboard navigation and reduced-motion support.
- [ ] Clearly label Bitcoin, Ethereum, and Bitcoin2.0Max data.
- [ ] Show loading, offline, and stale-data states.
- [ ] Publish the updated manifesto and roadmap.
- [ ] Check links, assets, layout, and browser errors.

Exit criteria:
All primary pages work on desktop and mobile, with readable
content, functioning navigation, and honest data-source labels.

## 2. Accounts and Personal Workspace

Status: Interface and backend implementation available;
public deployment pending

- [ ] Review the account server before public deployment.
- [ ] Deploy the backend with HTTPS and persistent storage.
- [ ] Connect the dashboard to its deployed account API.
- [ ] Configure permitted origins and secure session handling.
- [ ] Verify registration, sign-in, sign-out, and session expiry.
- [ ] Add email verification and password recovery.
- [ ] Verify users can access only their own saved records.
- [ ] Test portfolios, address books, and transaction bookmarks.
- [ ] Test watchlists, goals, and research notes.
- [ ] Add account export and deletion.
- [ ] Test database backups and restoration.
- [ ] Document privacy practices and retention.

Exit criteria:
A user can register, return later, and retrieve their own data.
Authorization tests pass, and backups survive a recovery drill.

## 3. Community Chat

Status: Implementation available; deployment and review pending

- [ ] Verify room access and message persistence.
- [ ] Enforce authentication and message-length limits.
- [ ] Add rate limits and spam controls.
- [ ] Add reporting and moderator tools.
- [ ] Publish community rules.
- [ ] Test safe rendering of user-generated content.
- [ ] Define message retention and deletion behavior.
- [ ] Clearly discourage sharing keys and recovery phrases.

Exit criteria:
Members can communicate reliably, while moderators can
respond to abuse and unauthorized access is rejected.

## 4. Node Reliability and Monitoring

Status: Development daemon implemented

- [ ] Verify clean builds and automated tests.
- [ ] Fix thread ownership and graceful shutdown.
- [ ] Test startup failures and occupied ports.
- [ ] Improve Electrum connection and reconnection handling.
- [ ] Report chain height and hash from verified state.
- [ ] Distinguish process health from synchronization status.
- [ ] Harden HTTP request handling and connection timeouts.
- [ ] Add service supervision, logs, and restart instructions.
- [ ] Connect the website to a protected HTTPS monitoring API.
- [ ] Separate public monitoring from administrative endpoints.

Exit criteria:
The daemon starts, stops, and restarts reliably.
Its monitoring endpoints accurately describe its actual state.

## 5. Native Chain Specification and Local Testing

Status: Planned; implementation must be verified

- [ ] Document consensus rules and network identity.
- [ ] Define genesis block and network-specific identifiers.
- [ ] Specify proof of work and difficulty adjustment.
- [ ] Specify issuance, subsidy, maturity, and supply rules.
- [ ] Define address formats and transaction rules.
- [ ] Establish separate local-test, testnet, and mainnet data.
- [ ] Implement deterministic local block generation.
- [ ] Test valid and invalid transactions.
- [ ] Persist blocks, chain state, and the UTXO set.
- [ ] Verify state after restarting the node.

The 60-second block target and 32 MiB block limit are
development parameters, not proof of achieved throughput.

Exit criteria:
A repeatable local test creates blocks, validates transactions,
rejects invalid state transitions, and survives a restart.

## 6. Peer Networking and Synchronization

Status: Basic incoming connections implemented;
full networking incomplete

- [ ] Complete and test peer handshakes.
- [ ] Add outbound connections and peer discovery.
- [ ] Maintain a persistent peer address book.
- [ ] Enforce connection limits and protocol bounds.
- [ ] Implement header and block synchronization.
- [ ] Validate received blocks before accepting them.
- [ ] Select the chain with the most valid cumulative work.
- [ ] Handle missing parents and chain reorganizations.
- [ ] Add peer penalties and resource-abuse protection.
- [ ] Evaluate asynchronous I/O and optional Tor support.

Exit criteria:
Multiple independent nodes synchronize, reject invalid peers
and blocks, and converge after a controlled chain reorganization.

## 7. Transaction Validation, Mempool, and Wallet Support

Status: Incomplete

- [ ] Complete script and signature validation.
- [ ] Verify Merkle roots, transaction inputs, and outputs.
- [ ] Reject double spending and invalid coin creation.
- [ ] Implement bounded mempool admission and eviction.
- [ ] Relay validated transactions.
- [ ] Define replacement and fee policies.
- [ ] Add fee estimation when sufficient data exists.
- [ ] Implement address history and balance indexing.
- [ ] Provide documented wallet integration and broadcast APIs.
- [ ] Test wallet recovery and network compatibility.

Exit criteria:
Compatible wallets can create transactions that independent
nodes validate, relay, confirm, and retain across restarts.

## 8. Ethereum Integration

Status: Read-only integration in progress

- [ ] Verify Ethereum Mainnet chain ID before accepting RPC data.
- [ ] Test block, gas, transaction-count, and balance displays.
- [ ] Verify ETH price updates and stale-data handling.
- [ ] Clearly distinguish native ETH from token balances.
- [ ] Add provider failure handling and request limits.
- [ ] Support saved Ethereum addresses in the personal dashboard.
- [ ] Evaluate optional wallet connection with explicit permissions.
- [ ] Review any transaction-signing feature separately.

A shared interface does not merge Bitcoin, Ethereum, and
Bitcoin2.0Max or make their assets interchangeable.

Exit criteria:
Ethereum information is accurate, clearly attributed, and
usable without requesting private keys or recovery phrases.

## 9. Security, Performance, and Release Readiness

Status: Ongoing across every milestone

- [ ] Audit dependencies and review exposed services.
- [ ] Fuzz network messages, transactions, and parsers.
- [ ] Run memory and undefined-behavior sanitizers.
- [ ] Test malformed input and resource exhaustion.
- [ ] Establish responsible vulnerability disclosure.
- [ ] Publish reproducible build instructions.
- [ ] Benchmark on documented hardware.
- [ ] Measure bandwidth, storage, and synchronization costs.
- [ ] Obtain independent review of consensus-critical code.
- [ ] Run a public testnet with documented operating procedures.
- [ ] Publish known limitations and release evidence.

Exit criteria:
Release claims are supported by tests and review.
Critical findings are resolved before production use.

---

## Future Ideas — Not Scheduled

- Human-readable address names.
- Additional read-only network integrations.
- Mobile-friendly light-client tools.
- Optional wallet interfaces.
- Lightning-related research.
- Schnorr and Taproot compatibility research.
- Cross-chain functionality with explicit trust assumptions.

These ideas require separate specifications and review.
They are not promises of availability.

## Immediate Priorities

1. Verify node startup and local status output.
2. Complete public account-server deployment.
3. Validate website price cards, navigation, and themes.
4. Establish reproducible native-chain local tests.
5. Complete persistent chain state and synchronization.

## Contributing

Open an issue describing the problem, proposed behavior,
and how success can be tested.

Keep changes focused. Include relevant tests and documentation.
Mark milestones complete only when their exit criteria are met.

**Build openly. Verify carefully. Keep moving forward.**
