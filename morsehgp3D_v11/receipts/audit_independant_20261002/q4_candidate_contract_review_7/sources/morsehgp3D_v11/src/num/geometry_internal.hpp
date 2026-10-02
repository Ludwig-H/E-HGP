// Outils internes des formules geometriques : differences certifiees par Point, produits natifs bornes.
#pragma once

#include "num/geometry.hpp"

namespace mhgp11::num::detail {

using Vec = std::array<i64, 3>;
inline Vec difference(Point a, Point b) noexcept {
  return {i64{a.x()} - b.x(), i64{a.y()} - b.y(), i64{a.z()} - b.z()};
}
inline DotInt dot(const Vec& a, const Vec& b) noexcept {
  static_assert(Budget::dot <= 63);
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
inline Vec cross(const Vec& a, const Vec& b) noexcept {
  static_assert(Budget::cross <= 63);
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
inline int sign(i128 value) noexcept { return (value > 0) - (value < 0); }

template <int N, int D>
Result<Level> checked_level(const Wide<N>& numerator, const Wide<D>& denominator) noexcept {
  auto out = Level::make(numerator, denominator);
  if (!out.ok()) return fail(Reason::arithmetic_invariant);
  return out.value();
}

// Trois produits au plus : les entrees sont issues des bornes de Point/Sphere, jamais d'entiers utilisateur nus.
// Chaque produit est forme a largeur totale avant de le ramener a la largeur demontree ; aucune troncature muette.
template <int Words>
Result<Wide<Words>> product(i128 a, i128 b) noexcept {
  Wide<Words> out;
  if (!resize(multiply(to_wide(a), to_wide(b)), out)) return fail(Reason::arithmetic_invariant);
  return out;
}

}  // namespace mhgp11::num::detail
