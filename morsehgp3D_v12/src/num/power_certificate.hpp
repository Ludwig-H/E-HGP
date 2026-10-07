// Certificat global suffisant, calcule une seule fois par la fabrique q3. Aucun coefficient public forgeable.
#pragma once
#include <array>
#include "num/budgets.hpp"

namespace mhgp12::num::detail {

inline constexpr bool q3_global_power_i128(CenterDen denominator, const std::array<CenterInt, 3>& numerator) noexcept {
  constexpr i128 d_limit = i128{1} << (123 - 2 * kCoordBits);
  constexpr i128 n_limit = i128{1} << (124 - kCoordBits);
  static_assert(123 - 2 * kCoordBits > 0 && 124 - kCoordBits < 127);
  if (denominator <= 0 || denominator >= d_limit) return false;
  for (const auto coordinate : numerator)
    if (coordinate <= -n_limit || coordinate >= n_limit) return false;
  return true;
}

}  // namespace mhgp12::num::detail
