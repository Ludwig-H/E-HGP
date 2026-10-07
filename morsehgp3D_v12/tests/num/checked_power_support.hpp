// Outils du harnais : termes entiers pour observer l'essai, reference TOUJOURS Wide pour juger le resultat.
#pragma once

#include <algorithm>

#include "num/num.hpp"
#include "num/power_checked.hpp"
#include "power_reference.hpp"

namespace checked_test {
using namespace mhgp12;
using namespace mhgp12::num;

// Termes en i128 : norme < 3*2^66 et facteurs < 2^34 a toute etendue (profil 32 compris).
struct Terms { i128 norm = 0; std::array<i128, 3> factors{}; };
inline Terms query_terms(const Sphere& sphere, Point point) {
  Terms out;
  for (int j = 0; j < 3; ++j) {
    const i64 v = i64{point.coordinates()[j]} - sphere.anchor().coordinates()[j];
    out.norm += i128{v} * v;
    out.factors[j] = -2 * i128{v};
  }
  return out;
}
inline std::array<Terms, 2> box_terms(const Sphere& sphere, const Box& box) {
  std::array<Terms, 2> out{};
  for (int j = 0; j < 3; ++j) {
    const i64 lo = i64{box.lo().coordinates()[j]} - sphere.anchor().coordinates()[j];
    const i64 hi = i64{box.hi().coordinates()[j]} - sphere.anchor().coordinates()[j];
    const i64 near = lo > 0 ? lo : hi < 0 ? hi : 0;
    out[0].norm += i128{near} * near;
    out[1].norm += std::max(i128{lo} * lo, i128{hi} * hi);
    const bool positive = to_wide(sphere.numerator()[j]).sign() >= 0;
    out[0].factors[j] = -2 * i128{positive ? hi : lo};
    out[1].factors[j] = -2 * i128{positive ? lo : hi};
  }
  return out;
}
// Essai controle du produit, possible seulement si les coefficients tiennent en i128 (sinon rien, comme le produit
// qui passe alors directement a la voie large).
inline std::optional<i128> attempt(const Sphere& sphere, const Terms& terms) {
  const auto d = narrow<127>(to_wide(sphere.denominator()));
  std::array<i128, 3> n{};
  for (int j = 0; j < 3; ++j) {
    const auto value = narrow<127>(to_wide(sphere.numerator()[j]));
    if (!d || !value) return std::nullopt;
    n[j] = *value;
  }
  return mhgp12::num::detail::checked_power_sum(*d, n, terms.norm, terms.factors);
}
inline Wide<4> wide_sum(const Sphere& sphere, const Terms& terms) {
  Wide<4> total;
  if (!multiply_into(to_wide(sphere.denominator()), to_wide(terms.norm), total))
    throw std::runtime_error("reference checked : largeur insuffisante");
  for (int j = 0; j < 3; ++j) {
    Wide<4> term, next;
    if (!multiply_into(to_wide(sphere.numerator()[j]), to_wide(terms.factors[j]), term) || !add(total, term, next))
      throw std::runtime_error("reference checked : largeur insuffisante");
    total = next;
  }
  return total;
}
}  // namespace checked_test
