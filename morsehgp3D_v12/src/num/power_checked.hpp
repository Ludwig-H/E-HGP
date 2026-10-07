// Essai interne entier signe : aucune valeur tronquee n'est publiee apres un debordement.
#pragma once

#include <array>
#include <optional>

#include "num/budgets.hpp"

namespace mhgp12::num::detail {

// Les entrees du produit viennent de Sphere ; cette fonction interne teste aussi les limites dans le harnais.
// Les builtins calculent sans UB, meme lorsque leur resultat n'est pas representable en i128. Dans ce cas,
// ignorer la valeur ecrite et reprendre toute l'expression en Wide depuis ses coefficients initiaux.
// Tous les operandes sont deja en i128 (norme |v|^2 < 3*2^66 et facteurs -2v_j < 2^34 a toute etendue de repere) :
// aucune conversion retrecissante ne precede un builtin, qui ne certifierait alors rien (CONTRAT_NUMERIQUE.md,
// paragraphe 3, types reconstruits).
inline std::optional<i128> checked_power_sum(i128 denominator, const std::array<i128, 3>& numerator,
                                             i128 norm, const std::array<i128, 3>& factors) noexcept {
  i128 total = 0;
  if (__builtin_mul_overflow(denominator, norm, &total)) return std::nullopt;
  for (int j = 0; j < 3; ++j) {
    i128 term = 0, next = 0;
    if (__builtin_mul_overflow(numerator[j], factors[j], &term)) return std::nullopt;
    if (__builtin_add_overflow(total, term, &next)) return std::nullopt;
    total = next;
  }
  return total;
}
// Meme essai pour des termes d'etendue au plus 30 (norme et facteurs en i64) : elargissement exact, jamais l'inverse.
inline std::optional<i128> checked_power_sum(i128 denominator, const std::array<i128, 3>& numerator,
                                             i64 norm, const std::array<i64, 3>& factors) noexcept {
  return checked_power_sum(denominator, numerator, i128{norm}, std::array<i128, 3>{factors[0], factors[1], factors[2]});
}

}  // namespace mhgp12::num::detail
