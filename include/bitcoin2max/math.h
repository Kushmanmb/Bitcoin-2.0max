#pragma once
// include/bitcoin2max/math.h
//
// Bitcoin 2.0max — mathematical utility functions.

#include <cstdint>
#include <stdexcept>

namespace bitcoin2max {
namespace utils {

/// Compute the n-th Fibonacci number recursively.
///
/// Uses the recurrence:
///   fibonacci(0) = 0
///   fibonacci(1) = 1
///   fibonacci(n) = fibonacci(n - 1) + fibonacci(n - 2)  for n > 1
///
/// This is a straightforward recursive implementation as specified.
/// For large n (n > ~40) consider an iterative or memoized variant to
/// avoid exponential call overhead.
///
/// \param n  Non-negative index into the Fibonacci sequence.
/// \returns  The n-th Fibonacci number as a 64-bit unsigned integer.
/// \throws   std::invalid_argument if \p n is negative.
inline uint64_t fibonacci(int n) {
    if (n < 0)
        throw std::invalid_argument("fibonacci: n must be non-negative");
    if (n <= 1)
        return static_cast<uint64_t>(n);
    return fibonacci(n - 1) + fibonacci(n - 2);
}

} // namespace utils
} // namespace bitcoin2max
