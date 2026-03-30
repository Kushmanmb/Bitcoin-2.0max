# Bitcoin-2.0max

[![CI](https://github.com/Kushmanmb/Bitcoin-2.0max/actions/workflows/ci.yml/badge.svg)](https://github.com/Kushmanmb/Bitcoin-2.0max/actions/workflows/ci.yml)
[![Release](https://github.com/Kushmanmb/Bitcoin-2.0max/actions/workflows/release.yml/badge.svg)](https://github.com/Kushmanmb/Bitcoin-2.0max/actions/workflows/release.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Website](https://img.shields.io/badge/website-Kushmanmb.eth-f7931a)](https://kushmanmb.eth.limo)

**Bitcoin supercharged — faster blocks, larger capacity, independent wallet relay via Electrum.**

🌐 **[Kushmanmb.eth](https://kushmanmb.eth.limo)** — project website

## Key Features

| Feature | Value |
|---|---|
| Target block time | **60 seconds** (10× faster than legacy Bitcoin) |
| Max block size | **32 MiB** (32× larger than legacy Bitcoin) |
| Wallet relay | Electrum server at `127.0.0.1:9050` (no BOLDwallet dependency) |
| Build system | CMake ≥ 3.16 |
| Language | C++17 |

## Building

### Prerequisites

```
cmake >= 3.16
g++ / clang++ with C++17 support
OpenSSL development headers  (libssl-dev on Debian/Ubuntu)
```

### Configure & Build

```bash
git clone https://github.com/Kushmanmb/Bitcoin-2.0max.git
cd Bitcoin-2.0max

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

The resulting binary is `build/bitcoin2maxd`.

### Build Options

| Option | Default | Description |
|---|---|---|
| `BUILD_TESTS` | `ON` | Build unit tests (requires internet for Catch2 fetch) |
| `ENABLE_ELECTRUM` | `ON` | Compile in the Electrum client |
| `ENABLE_TOR` | `OFF` | Route Electrum traffic over Tor |

```bash
# Example: build without tests
cmake -B build -DBUILD_TESTS=OFF
cmake --build build
```

### Run Tests

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

## Configuration

Copy the default configuration file to your home directory:

```bash
mkdir -p ~/.bitcoin2max
cp conf/bitcoin2max.conf ~/.bitcoin2max/bitcoin2max.conf
# Edit the file as needed, then start the node:
./build/bitcoin2maxd
```

### Critical settings

```ini
# Electrum server (independent of BOLDwallet)
electrum=1
electrumhost=127.0.0.1
electrumport=9050

# Supercharged block parameters
targetblocktime=60        # seconds
maxblocksize=33554432     # 32 MiB
```

## Running

```bash
# Use default config at ~/.bitcoin2max/bitcoin2max.conf
./build/bitcoin2maxd

# Specify a custom config file
./build/bitcoin2maxd --conf /path/to/bitcoin2max.conf

# Print help
./build/bitcoin2maxd --help
```

## Architecture

```
bitcoin2maxd
├── src/
│   ├── main.cpp                  Entry point & signal handling
│   ├── config/
│   │   ├── config.h              Config loader declarations
│   │   └── config.cpp            .conf file parser
│   ├── electrum/
│   │   ├── electrum_client.h     Electrum JSON-RPC client
│   │   └── electrum_client.cpp   POSIX socket implementation
│   └── node/
│       ├── node.h                Node declarations
│       └── node.cpp              Main loop, P2P, block validation
├── include/bitcoin2max/
│   ├── params.h                  Compile-time consensus constants
│   └── config.h                  Config struct
├── conf/
│   └── bitcoin2max.conf          Default configuration
├── tests/
│   ├── test_params.cpp           Consensus parameter tests
│   └── test_config.cpp           Config loader tests
└── CMakeLists.txt
```

## Independence from BOLDwallet

Bitcoin 2.0max does **not** depend on BOLDwallet.  Wallet functionality is
delegated entirely to an Electrum server running at `127.0.0.1:9050`.  Any
standard Electrum server (e.g. Electrum Personal Server, Fulcrum, Electrs)
can be used.  The node will start and sync even if the Electrum server is
temporarily unavailable, and will reconnect automatically.

## Docker

```bash
# Build the image locally
docker build -t bitcoin2maxd .

# Run with a host-mounted config directory
docker run -d \
  -v ~/.bitcoin2max:/var/lib/bitcoin2max \
  -p 8333:8333 \
  bitcoin2maxd
```

Pre-built images are published to the GitHub Container Registry on every
tagged release:

```bash
docker pull ghcr.io/kushmanmb/bitcoin-2.0max:latest
```

## Project Roadmap

See [ROADMAP.md](ROADMAP.md) for planned milestones and upcoming features.

## Contributing

1. Fork the repository and create your branch from `main`.
2. Make your changes, add or update tests, and verify the build passes.
3. Open a pull request — the [PULL_REQUEST_TEMPLATE](.github/PULL_REQUEST_TEMPLATE.md) will guide you.
4. A code owner will review and merge your PR.

See [OWNERS.md](OWNERS.md) for the list of maintainers and area owners.
