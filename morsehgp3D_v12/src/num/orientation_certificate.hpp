// Certificat GLOBAL pour normale de trois Point du meme cube, jamais de deux Vec arbitraires.
#pragma once
#include <array>
#include "num/budgets.hpp"

namespace mhgp12::num::detail {

inline constexpr bool global_orientation_i128(CenterDen denominator,
                                              const std::array<CenterInt, 3>& numerator) noexcept {
  constexpr i128 d_limit = i128{1} << (124 - 3 * kCoordBits);
  constexpr i128 n_limit = i128{1} << (124 - 2 * kCoordBits);
  static_assert(124 - 3 * kCoordBits > 0 && 124 - 2 * kCoordBits < 127);
  if (denominator <= 0 || denominator >= d_limit) return false;
  for (const auto coordinate : numerator)
    if (coordinate <= -n_limit || coordinate >= n_limit) return false;
  return true;
}

}  // namespace mhgp12::num::detail
