// Essai interne entier signe : aucune valeur tronquee n'est publiee apres un debordement.
#pragma once

#include <array>
#include <optional>

#include "num/budgets.hpp"

namespace mhgp11::num::detail {

// Les entrees du produit viennent de Sphere ; cette fonction interne teste aussi les limites dans le harnais.
// Les builtins calculent sans UB, meme lorsque leur resultat n'est pas representable en i128. Dans ce cas,
// ignorer la valeur ecrite et reprendre toute l'expression en Wide depuis ses coefficients initiaux.
inline std::optional<i128> checked_power_sum(i128 denominator, const std::array<i128, 3>& numerator,
                                             i64 norm, const std::array<i64, 3>& factors) noexcept {
  i128 total = 0;
  if (__builtin_mul_overflow(denominator, i128{norm}, &total)) return std::nullopt;
  for (int j = 0; j < 3; ++j) {
    i128 term = 0, next = 0;
    if (__builtin_mul_overflow(numerator[j], i128{factors[j]}, &term)) return std::nullopt;
    if (__builtin_add_overflow(total, term, &next)) return std::nullopt;
    total = next;
  }
  return total;
}

}  // namespace mhgp11::num::detail
