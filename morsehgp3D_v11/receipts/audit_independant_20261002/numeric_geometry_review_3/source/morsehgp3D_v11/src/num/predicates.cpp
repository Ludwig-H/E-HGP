// Predicats R2 portes avec budgets par expression. Pas de conversion flottante, pas de filtre heuristique.
#include "num/geometry_internal.hpp"

namespace mhgp11::num {

Result<SideInt> power(const Sphere& sphere, Point point) noexcept {
  constexpr int words = (Budget::side + 63) / 64;
  const auto v = detail::difference(point, sphere.anchor());
  auto first = detail::product<words>(sphere.denominator(), detail::dot(v, v));
  if (!first.ok()) return first.outcome();
  auto total = first.value();
  for (int j = 0; j < 3; ++j) {
    auto term = detail::product<words>(sphere.numerator()[j], -2 * i128{v[j]});
    if (!term.ok()) return term.outcome();
    auto sum = detail::require_add(total, term.value());
    if (!sum.ok()) return sum.outcome();
    total = sum.value();
  }
  return detail::require_fit<Budget::side>(total);
}

Result<int> side(const Sphere& sphere, Point point) noexcept {
  auto value = power(sphere, point);
  if (!value.ok()) return value.outcome();
  return to_wide(value.value()).sign();
}

DeterminantInt orientation(Point a, Point b, Point c, Point d) noexcept {
  const auto u = detail::difference(b, a), v = detail::difference(c, a), w = detail::difference(d, a);
  const auto normal = detail::cross(u, v);
  static_assert(Budget::determinant <= 127);
  // La somme a six monomes est < 6 M^3 < 2^Budget::determinant ; la conversion en i64 du profil 18 est exacte.
  return static_cast<DeterminantInt>(i128{normal[0]} * w[0] + i128{normal[1]} * w[1] + i128{normal[2]} * w[2]);
}

Result<int> orientation(Point a, Point b, Point c, const Sphere& center) noexcept {
  constexpr int words = (Budget::center_orientation + 63) / 64;
  const auto normal = detail::cross(detail::difference(b, a), detail::difference(c, a));
  const auto offset = detail::difference(center.anchor(), a);
  Wide<words> total;
  for (int j = 0; j < 3; ++j) {
    // N + D*(anchor-a) < 48 M^5, donc < 2^(5B+6) <= 2^126 : i128 reste exact.
    static_assert(5 * kCoordBits + 6 <= 127);
    const i128 coordinate = center.numerator()[j] + center.denominator() * offset[j];
    auto term = detail::product<words>(coordinate, normal[j]);
    if (!term.ok()) return term.outcome();
    auto sum = detail::require_add(total, term.value());
    if (!sum.ok()) return sum.outcome();
    total = sum.value();
  }
  if (total.bit_length() > Budget::center_orientation) return fail(Reason::arithmetic_invariant);
  return total.sign();
}

bool strictly_acute(Point a, Point b, Point c) noexcept {
  return detail::dot(detail::difference(b, a), detail::difference(c, a)) > 0 &&
         detail::dot(detail::difference(a, b), detail::difference(c, b)) > 0 &&
         detail::dot(detail::difference(a, c), detail::difference(b, c)) > 0;
}

Result<bool> strictly_inside(const Sphere& center, Point a, Point b, Point c, Point d) noexcept {
  const std::array<Point, 4> points{a, b, c, d};
  for (int opposite = 0; opposite < 4; ++opposite) {
    std::array<Point, 3> face{};
    int j = 0;
    for (int i = 0; i < 4; ++i)
      if (i != opposite) face[j++] = points[i];
    const int vertex_sign = detail::sign(orientation(face[0], face[1], face[2], points[opposite]));
    if (vertex_sign == 0) return false;
    auto center_sign = orientation(face[0], face[1], face[2], center);
    if (!center_sign.ok()) return center_sign.outcome();
    if (center_sign.value() != vertex_sign) return false;
  }
  return true;
}

bool is_midpoint(const Sphere& center, Point a, Point b) noexcept {
  // Chaque cote < 96 M^5 < 2^(5B+7) <= 2^127 ; les intermediaires signes restent representables.
  static_assert(5 * kCoordBits + 7 <= 127);
  const auto anchor = center.anchor().coordinates(), ac = a.coordinates(), bc = b.coordinates();
  for (int j = 0; j < 3; ++j)
    if (2 * (center.denominator() * anchor[j] + center.numerator()[j]) !=
        center.denominator() * (i128{ac[j]} + bc[j])) return false;
  return true;
}

}  // namespace mhgp11::num
