#pragma once
// include/bitcoin2max/transaction.h
//
// Bitcoin 2.0max — transaction data structures and validation.
//
// Supports loading transactions from a plain-text .dat file and validating
// each transaction against the Bitcoin 2.0max consensus rules.
//
// .dat file format:
//   # comment lines and blank lines are ignored
//   TX <version> <locktime>
//   IN <prev_txid_hex64> <prev_index> <script_hex> <sequence_hex>
//   OUT <value_satoshis> <script_hex>
//   (repeat TX/IN/OUT blocks for additional transactions)

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace bitcoin2max {

/// A single transaction input.
struct TxInput {
    std::array<uint8_t, 32> prevTxid{};          ///< Previous transaction ID (32 bytes)
    uint32_t                prevIndex{0};         ///< Output index in previous transaction
    std::vector<uint8_t>    scriptSig;             ///< Input script bytes
    uint32_t                sequence{0xFFFFFFFF};  ///< Sequence number
};

/// A single transaction output.
struct TxOutput {
    uint64_t             valueSatoshis{0};  ///< Value in satoshis
    std::vector<uint8_t> scriptPubKey;      ///< Output script bytes
};

/// A Bitcoin transaction.
struct Transaction {
    uint32_t              version{1};  ///< Transaction version (1 or 2)
    std::vector<TxInput>  inputs;      ///< List of inputs
    std::vector<TxOutput> outputs;     ///< List of outputs
    uint32_t              locktime{0}; ///< Locktime
};

/// Result of transaction validation.
struct ValidationResult {
    bool        valid{false};
    std::string error;  ///< Human-readable description when !valid
};

/// Validate a single transaction against Bitcoin 2.0max consensus rules:
///   - version must be 1 or 2
///   - at least one input
///   - at least one output
///   - no individual output value exceeds MAX_MONEY_SATOSHIS
///   - total output value does not exceed MAX_MONEY_SATOSHIS
///
/// \returns ValidationResult with valid==true on success.
ValidationResult validateTransaction(const Transaction& tx);

/// Load and validate all transactions from a .dat file.
///
/// Parses the text-based .dat format, validates each transaction, and returns
/// the complete list.  On the first parse or validation error, \p error is
/// populated and an empty vector is returned.
///
/// \param path   Path to the transaction_data.dat file.
/// \param error  Populated with a description of the first error encountered.
/// \returns vector of validated Transaction objects; empty on any error.
std::vector<Transaction> loadTransactions(const std::string& path,
                                          std::string&       error);

// ── Content hash ─────────────────────────────────────────────────────────────

/// Serialize a transaction to its canonical Bitcoin wire format.
///
/// Wire format: version(4) | vin_count(varint) | inputs... |
///              vout_count(varint) | outputs... | locktime(4)
///
/// \returns The raw serialized bytes.
std::vector<uint8_t> serializeTransaction(const Transaction& tx);

/// Compute the double-SHA256 content hash of a transaction.
///
/// This is the standard Bitcoin transaction ID (TXID) computed as
/// SHA256(SHA256(serialize(tx))).
///
/// \returns A 32-byte hash array.
std::array<uint8_t, 32> computeContentHash(const Transaction& tx);

/// Return the content hash of a transaction as a lowercase hex string.
///
/// The bytes are reversed before hex-encoding to match the conventional
/// big-endian display used by Bitcoin block explorers.
///
/// \returns A 64-character lowercase hex string.
std::string contentHashHex(const Transaction& tx);

} // namespace bitcoin2max
