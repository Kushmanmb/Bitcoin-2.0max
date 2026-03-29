// src/transaction/transaction.cpp
//
// Bitcoin 2.0max — transaction data structures and validation.
//
// Implements:
//   • validateTransaction()  — consensus rule checks
//   • loadTransactions()     — plain-text .dat file parser

#include "bitcoin2max/transaction.h"
#include "bitcoin2max/params.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace bitcoin2max {

// ── Hex decode helper ─────────────────────────────────────────────────────────

static int hexCharValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/// Decode a hex string into bytes.  Returns an empty vector on any error.
static std::vector<uint8_t> hexDecode(const std::string& hex) {
    if (hex.size() % 2 != 0) return {};
    std::vector<uint8_t> out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        int hi = hexCharValue(hex[i]);
        int lo = hexCharValue(hex[i + 1]);
        if (hi < 0 || lo < 0) return {};
        out.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }
    return out;
}

// ── Validation ────────────────────────────────────────────────────────────────

ValidationResult validateTransaction(const Transaction& tx) {
    if (tx.version < 1 || tx.version > 2) {
        return {false, "invalid version: must be 1 or 2"};
    }
    if (tx.inputs.empty()) {
        return {false, "transaction has no inputs"};
    }
    if (tx.outputs.empty()) {
        return {false, "transaction has no outputs"};
    }

    uint64_t totalOut = 0;
    for (const auto& out : tx.outputs) {
        if (out.valueSatoshis > consensus::MAX_MONEY_SATOSHIS) {
            return {false, "output value exceeds MAX_MONEY_SATOSHIS"};
        }
        if (totalOut > consensus::MAX_MONEY_SATOSHIS - out.valueSatoshis) {
            return {false, "total output value exceeds MAX_MONEY_SATOSHIS"};
        }
        totalOut += out.valueSatoshis;
    }

    return {true, ""};
}

// ── .dat file parser ──────────────────────────────────────────────────────────

std::vector<Transaction> loadTransactions(const std::string& path,
                                          std::string&       error) {
    error.clear();

    std::ifstream file(path);
    if (!file.is_open()) {
        error = "cannot open file: " + path;
        return {};
    }

    std::vector<Transaction> result;
    bool        hasCurrent = false;
    Transaction current;
    std::string line;
    int         lineNum = 0;

    auto finaliseCurrent = [&](int atLine) -> bool {
        auto res = validateTransaction(current);
        if (!res.valid) {
            error = "transaction validation failed before line " +
                    std::to_string(atLine) + ": " + res.error;
            return false;
        }
        result.push_back(std::move(current));
        current    = Transaction{};
        hasCurrent = false;
        return true;
    };

    while (std::getline(file, line)) {
        ++lineNum;

        // Strip trailing CR (Windows line endings)
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // Skip blank lines and comments
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string        keyword;
        iss >> keyword;

        if (keyword == "TX") {
            // Finalise the previous transaction (if any) before starting a new one
            if (hasCurrent && !finaliseCurrent(lineNum)) return {};

            current    = Transaction{};
            hasCurrent = true;

            if (!(iss >> current.version >> current.locktime)) {
                error = "malformed TX line at line " + std::to_string(lineNum);
                return {};
            }

        } else if (keyword == "IN") {
            if (!hasCurrent) {
                error = "IN record before any TX at line " + std::to_string(lineNum);
                return {};
            }

            TxInput     inp;
            std::string prevHex;
            std::string scriptHex;
            std::string seqHex;

            if (!(iss >> prevHex >> inp.prevIndex >> scriptHex >> seqHex)) {
                error = "malformed IN line at line " + std::to_string(lineNum);
                return {};
            }

            if (prevHex.size() != 64) {
                error = "prev_txid must be exactly 64 hex characters at line " +
                        std::to_string(lineNum);
                return {};
            }

            auto prevBytes = hexDecode(prevHex);
            if (prevBytes.size() != 32) {
                error = "invalid hex in prev_txid at line " + std::to_string(lineNum);
                return {};
            }
            std::copy(prevBytes.begin(), prevBytes.end(), inp.prevTxid.begin());

            inp.scriptSig = hexDecode(scriptHex);
            // An empty scriptSig is valid (e.g. coinbase inputs use arbitrary data,
            // and native SegWit inputs carry no scriptSig at all).  Only reject the
            // case where the hex field was non-empty but failed to decode — that
            // indicates malformed data in the .dat file.
            if (inp.scriptSig.empty() && !scriptHex.empty()) {
                error = "invalid hex in scriptSig at line " + std::to_string(lineNum);
                return {};
            }

            try {
                inp.sequence = static_cast<uint32_t>(std::stoul(seqHex, nullptr, 16));
            } catch (...) {
                error = "invalid sequence value at line " + std::to_string(lineNum);
                return {};
            }

            current.inputs.push_back(std::move(inp));

        } else if (keyword == "OUT") {
            if (!hasCurrent) {
                error = "OUT record before any TX at line " + std::to_string(lineNum);
                return {};
            }

            TxOutput    out;
            std::string scriptHex;

            if (!(iss >> out.valueSatoshis >> scriptHex)) {
                error = "malformed OUT line at line " + std::to_string(lineNum);
                return {};
            }

            out.scriptPubKey = hexDecode(scriptHex);
            if (out.scriptPubKey.empty() && !scriptHex.empty()) {
                error = "invalid hex in scriptPubKey at line " + std::to_string(lineNum);
                return {};
            }

            current.outputs.push_back(std::move(out));

        } else {
            error = "unknown keyword '" + keyword + "' at line " +
                    std::to_string(lineNum);
            return {};
        }
    }

    // Finalise the last transaction
    if (hasCurrent && !finaliseCurrent(lineNum + 1)) return {};

    return result;
}

} // namespace bitcoin2max
