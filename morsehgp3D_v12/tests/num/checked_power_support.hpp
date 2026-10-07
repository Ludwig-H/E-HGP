// Outils du harnais : termes entiers pour observer l'essai, reference TOUJOURS Wide pour juger le resultat.
#pragma once

#include <algorithm>

#include "num/num.hpp"
#include "num/power_checked.hpp"
#include "power_reference.hpp"

namespace checked_test {
using namespace mhgp12;
using namespace mhgp12::num;

struct Terms { i64 norm = 0; std::array<i64, 3> factors{}; };
inline Terms query_terms(const Sphere& sphere, Point point) {
  Terms out;
  for (int j = 0; j < 3; ++j) {
    const i64 v = i64{point.coordinates()[j]} - sphere.anchor().coordinates()[j];
    out.norm += v * v;
    out.factors[j] = -2 * v;
  }
  return out;
}
inline std::array<Terms, 2> box_terms(const Sphere& sphere, const Box& box) {
  std::array<Terms, 2> out{};
  for (int j = 0; j < 3; ++j) {
    const i64 lo = i64{box.lo().coordinates()[j]} - sphere.anchor().coordinates()[j];
    const i64 hi = i64{box.hi().coordinates()[j]} - sphere.anchor().coordinates()[j];
    const i64 near = lo > 0 ? lo : hi < 0 ? hi : 0;
    out[0].norm += near * near;
    out[1].norm += std::max(lo * lo, hi * hi);
    const bool positive = sphere.numerator()[j] >= 0;
    out[0].factors[j] = -2 * (positive ? hi : lo);
    out[1].factors[j] = -2 * (positive ? lo : hi);
  }
  return out;
}
inline std::optional<i128> attempt(const Sphere& sphere, const Terms& terms) {
  return mhgp12::num::detail::checked_power_sum(sphere.denominator(), sphere.numerator(), terms.norm, terms.factors);
}
inline Wide<4> wide_sum(const Sphere& sphere, const Terms& terms) {
  auto total = multiply(to_wide(sphere.denominator()), to_wide(i128{terms.norm}));
  for (int j = 0; j < 3; ++j) {
    const auto term = multiply(to_wide(sphere.numerator()[j]), to_wide(i128{terms.factors[j]}));
    Wide<4> next;
    if (!add(total, term, next)) throw std::runtime_error("reference checked : largeur insuffisante");
    total = next;
  }
  return total;
}
}  // namespace checked_test
