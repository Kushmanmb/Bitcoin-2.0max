#include "pow.h"

#include <memory>
#include <openssl/bn.h>

namespace bitcoin2max::consensus {
namespace {
using BigNum = std::unique_ptr<BIGNUM, decltype(&BN_free)>;

BigNum decodeTarget(uint32_t compact) {
    BigNum target(BN_new(), BN_free);
    if (!target) return target;
    const uint32_t size = compact >> 24;
    uint32_t word = compact & 0x007fffffu;
    if (size <= 3) word >>= 8 * (3 - size);
    const bool negative = word != 0 && (compact & 0x00800000u);
    const bool overflow = word != 0 &&
        (size > 34 || (word > 0xff && size > 33) ||
         (word > 0xffff && size > 32));
    if (negative || overflow || word == 0 || BN_set_word(target.get(), word) != 1)
        return BigNum(nullptr, BN_free);
    if (size > 3 && BN_lshift(target.get(), target.get(), 8 * (size - 3)) != 1)
        return BigNum(nullptr, BN_free);
    return target;
}
} // namespace

bool checkProofOfWork(const crypto::Hash256& hash, uint32_t bits,
                      uint32_t limitBits) {
    const auto target = decodeTarget(bits);
    const auto limit = decodeTarget(limitBits);
    if (!target || !limit || BN_cmp(target.get(), limit.get()) > 0) return false;
    BigNum value(BN_lebin2bn(hash.data(), hash.size(), nullptr), BN_free);
    return value && BN_cmp(value.get(), target.get()) <= 0;
}
} // namespace bitcoin2max::consensus
