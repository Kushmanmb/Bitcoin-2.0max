#include "regtest_chain.h"
#include "bitcoin2max/subsidy.h"
#include "pow.h"

#include <algorithm>
#include <istream>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace bitcoin2max::regtest {
namespace {
std::vector<uint8_t> heightScript(uint64_t height) {
    std::vector<uint8_t> number;
    while (height) {
        number.push_back(static_cast<uint8_t>(height));
        height >>= 8;
    }
    if (!number.empty() && (number.back() & 0x80)) number.push_back(0);
    std::vector<uint8_t> script{static_cast<uint8_t>(number.size())};
    script.insert(script.end(), number.begin(), number.end());
    const std::string marker = "B2MX regtest only";
    script.push_back(static_cast<uint8_t>(marker.size()));
    script.insert(script.end(), marker.begin(), marker.end());
    return script;
}

uint64_t parseDecimal(const std::string& value) {
    if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos)
        throw std::runtime_error("Non-decimal chain record field");
    size_t used = 0;
    const auto parsed = std::stoull(value, &used);
    if (used != value.size()) throw std::runtime_error("Invalid chain record field");
    return parsed;
}
} // namespace

std::string displayHash(const crypto::Hash256& raw) {
    auto reversed = raw;
    std::reverse(reversed.begin(), reversed.end());
    return crypto::toHex(reversed);
}

Block blockTemplate(uint64_t height, const crypto::Hash256& previous) {
    if (height > MAX_TEST_HEIGHT) throw std::runtime_error("Regtest height limit exceeded");
    Block block;
    TxInput input;
    input.prevIndex = std::numeric_limits<uint32_t>::max();
    input.scriptSig = heightScript(height);
    block.coinbase.inputs.push_back(input);
    // OP_TRUE is a test-only payout script; there is no spend engine yet.
    block.coinbase.outputs.push_back({consensus::blockSubsidy(height), {0x51}});
    block.header.prevHash = previous;
    block.header.merkleRoot = computeContentHash(block.coinbase);
    block.header.time = GENESIS_TIME + static_cast<uint32_t>(height * 60);
    block.header.bits = POW_BITS;
    return block;
}

Block mineBlock(uint64_t height, const crypto::Hash256& previous) {
    auto block = blockTemplate(height, previous);
    for (uint64_t nonce = 0; nonce <= std::numeric_limits<uint32_t>::max(); ++nonce) {
        block.header.nonce = static_cast<uint32_t>(nonce);
        if (consensus::checkProofOfWork(crypto::computeBlockHash(block.header),
                                      block.header.bits, POW_BITS)) return block;
    }
    throw std::runtime_error("Nonce space exhausted");
}

Chain::Chain() : tip_(crypto::computeBlockHash(blockTemplate(0, {}).header)) {
    if (displayHash(tip_) != GENESIS_HASH ||
        !consensus::checkProofOfWork(tip_, POW_BITS, POW_BITS))
        throw std::runtime_error("Regtest genesis does not match the pinned network anchor");
}

ValidationResult Chain::append(const Block& block) {
    if (height_ >= MAX_TEST_HEIGHT) return {false, "Regtest height limit exceeded"};
    const auto next = height_ + 1;
    const auto expected = blockTemplate(next, tip_);
    if (block.header.prevHash != tip_) return {false, "Wrong previous block"};
    if (block.header.version != expected.header.version ||
        block.header.time != expected.header.time || block.header.bits != POW_BITS)
        return {false, "Unexpected regtest header parameters"};
    const auto txResult = validateTransaction(block.coinbase);
    if (!txResult.valid) return txResult;
    if (serializeTransaction(block.coinbase) != serializeTransaction(expected.coinbase))
        return {false, "Unexpected coinbase: height, reward or payout script"};
    if (block.header.merkleRoot != computeContentHash(block.coinbase))
        return {false, "Wrong coinbase Merkle root"};
    const auto reward = block.coinbase.outputs[0].valueSatoshis;
    if (!consensus::validCoinbaseReward(next, reward, 0, issued_))
        return {false, "Coinbase reward exceeds issuance limit"};
    const auto hash = crypto::computeBlockHash(block.header);
    if (!consensus::checkProofOfWork(hash, block.header.bits, POW_BITS))
        return {false, "Invalid proof of work"};
    tip_ = hash;
    height_ = next;
    issued_ += reward;
    return {true, {}};
}

uint64_t Chain::matureRewards() const {
    uint64_t total = 0;
    if (height_ < consensus::COINBASE_MATURITY) return total;
    for (uint64_t h = 1; h <= height_ - consensus::COINBASE_MATURITY; ++h)
        total += consensus::blockSubsidy(h);
    return total;
}

Chain Chain::load(std::istream& input) {
    Chain chain;
    std::string line;
    if (!std::getline(input, line) || input.eof() || line != "B2MX-REGTEST-V1")
        throw std::runtime_error("Missing regtest file identifier");
    while (std::getline(input, line)) {
        // Require newline-terminated records; fail closed on partial writes.
        if (input.eof() || line.size() > 128)
            throw std::runtime_error("Truncated or oversized chain record");
        std::istringstream record(line);
        std::string heightText, nonceText, hashText, extra;
        if (!(record >> heightText >> nonceText >> hashText) || (record >> extra))
            throw std::runtime_error("Malformed chain record");
        const auto height = parseDecimal(heightText);
        const auto nonce = parseDecimal(nonceText);
        if (height != chain.height() + 1 || nonce > std::numeric_limits<uint32_t>::max())
            throw std::runtime_error("Invalid chain record height or nonce");
        auto block = blockTemplate(height, chain.tip());
        block.header.nonce = static_cast<uint32_t>(nonce);
        const auto result = chain.append(block);
        if (!result.valid) throw std::runtime_error(result.error);
        if (hashText != displayHash(chain.tip()))
            throw std::runtime_error("Stored block hash mismatch");
    }
    if (input.bad()) throw std::runtime_error("Failed to read chain file");
    return chain;
}

void Chain::writeRecord(std::ostream& output, uint64_t height, const Block& block) {
    output << height << ' ' << block.header.nonce << ' '
           << displayHash(crypto::computeBlockHash(block.header)) << '\n';
    if (!output) throw std::runtime_error("Failed to write chain record");
}
} // namespace bitcoin2max::regtest
