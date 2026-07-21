#pragma once

#include "morsehgp3d/exact/binary64.hpp"

#include <cstdint>
#include <optional>
#include <stdexcept>

namespace morsehgp3d::exact {

// Return the immediately adjacent canonical finite binary64 word in numeric
// order.  No floating arithmetic is performed, so these helpers are
// independent of the active rounding mode and of FTZ/DAZ.  The null result is
// reserved for the two finite extrema whose outward neighbour is an infinity.
[[nodiscard]] inline std::optional<std::uint64_t>
previous_canonical_finite_binary64_bits(std::uint64_t bits) {
  const std::uint64_t canonical_bits = canonicalize_binary64_bits(bits);
  if (canonical_bits != bits) {
    throw std::invalid_argument(
        "a finite binary64 predecessor requires a canonical input word");
  }
  if (canonical_bits == 0U) {
    return binary64_sign_mask | std::uint64_t{1};
  }
  const std::uint64_t candidate =
      (canonical_bits & binary64_sign_mask) != 0U
          ? canonical_bits + std::uint64_t{1}
          : canonical_bits - std::uint64_t{1};
  if (!is_finite_binary64_bits(candidate)) {
    return std::nullopt;
  }
  return candidate;
}

[[nodiscard]] inline std::optional<std::uint64_t>
next_canonical_finite_binary64_bits(std::uint64_t bits) {
  const std::uint64_t canonical_bits = canonicalize_binary64_bits(bits);
  if (canonical_bits != bits) {
    throw std::invalid_argument(
        "a finite binary64 successor requires a canonical input word");
  }
  std::uint64_t candidate =
      (canonical_bits & binary64_sign_mask) != 0U
          ? canonical_bits - std::uint64_t{1}
          : canonical_bits + std::uint64_t{1};
  if (candidate == binary64_negative_zero) {
    candidate = 0U;
  }
  if (!is_finite_binary64_bits(candidate)) {
    return std::nullopt;
  }
  return candidate;
}

}  // namespace morsehgp3d::exact
