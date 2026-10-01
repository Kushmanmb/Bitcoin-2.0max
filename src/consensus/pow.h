#pragma once

#include "crypto/block_hash.h"

namespace bitcoin2max::consensus {

// Hash256 stores raw digest bytes. Bitcoin interprets these as a little-endian
// integer for proof of work. The supplied limit is a compact target too.
bool checkProofOfWork(const crypto::Hash256& hash, uint32_t bits,
                      uint32_t limitBits);

} // namespace bitcoin2max::consensus
