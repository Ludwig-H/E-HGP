// Centres R2 865f5e6 : q2 milieu, q3 double produit vectoriel, q4 Cramer ; D normalise positif.
// Level q3 emploie le produit des trois carres de longueurs / (4|u x v|^2), pour eviter le degre 10 de |N3|^2.
#include "num/geometry_internal.hpp"

namespace mhgp11::num {

Result<Point> Point::make(i64 x, i64 y, i64 z) noexcept {
  if (x < 0 || y < 0 || z < 0 || x > kCoordMax || y > kCoordMax || z > kCoordMax)
    return fail(Reason::coordinate_out_of_domain);
  return Point({static_cast<u32>(x), static_cast<u32>(y), static_cast<u32>(z)});
}

Sphere Sphere::point(Point a) noexcept { return Sphere(a, {}, 1, Level{}); }

Result<std::optional<Sphere>> Sphere::through(Point a, Point b) noexcept {
  if (a == b) return std::optional<Sphere>{};
  const auto u = detail::difference(b, a);
  auto level = detail::checked_level(to_wide(detail::dot(u, u)), to_wide(i64{4}));
  if (!level.ok()) return level.outcome();
  return std::optional<Sphere>{Sphere(a, {u[0], u[1], u[2]}, 2, level.value())};
}

Result<std::optional<Sphere>> Sphere::through(Point a, Point b, Point c) noexcept {
  const auto u = detail::difference(b, a), v = detail::difference(c, a);
  const auto w = detail::cross(u, v);
  const i128 g = i128{w[0]} * w[0] + i128{w[1]} * w[1] + i128{w[2]} * w[2];
  if (g == 0) return std::optional<Sphere>{};
  const auto uu = detail::dot(u, u), vv = detail::dot(v, v);
  std::array<i128, 3> t{};
  for (int j = 0; j < 3; ++j) t[j] = i128{uu} * v[j] - i128{vv} * u[j];
  static_assert(Budget::numerator3 <= 127 && Budget::denominator3 <= 127);
  const std::array<CenterInt, 3> n = {t[1] * w[2] - t[2] * w[1], t[2] * w[0] - t[0] * w[2],
                                      t[0] * w[1] - t[1] * w[0]};
  const auto bc = detail::difference(c, b);
  const auto numerator = multiply(to_wide(i128{uu} * vv), to_wide(detail::dot(bc, bc)));
  auto level = detail::checked_level(numerator, to_wide(4 * g));
  if (!level.ok()) return level.outcome();
  return std::optional<Sphere>{Sphere(a, n, 2 * g, level.value())};
}

Result<std::optional<Sphere>> Sphere::through(Point a, Point b, Point c, Point d) noexcept {
  const auto u = detail::difference(b, a), v = detail::difference(c, a), s = detail::difference(d, a);
  const auto vs = detail::cross(v, s), su = detail::cross(s, u), uv = detail::cross(u, v);
  const i128 det = i128{u[0]} * vs[0] + i128{u[1]} * vs[1] + i128{u[2]} * vs[2];
  if (det == 0) return std::optional<Sphere>{};
  const auto uu = detail::dot(u, u), vv = detail::dot(v, v), ss = detail::dot(s, s);
  static_assert(Budget::numerator4 <= 127 && Budget::denominator4 <= 127);
  std::array<CenterInt, 3> n{};
  for (int j = 0; j < 3; ++j) n[j] = i128{uu} * vs[j] + i128{vv} * su[j] + i128{ss} * uv[j];
  CenterDen denominator = 2 * det;
  if (denominator < 0) {
    denominator = -denominator;
    for (auto& coordinate : n) coordinate = -coordinate;
  }
  constexpr int words = (Budget::level_numerator + 63) / 64;
  Wide<words> numerator;
  for (const auto coordinate : n) {
    auto square = detail::product<words>(coordinate, coordinate);
    if (!square.ok()) return square.outcome();
    auto sum = detail::require_add(numerator, square.value());
    if (!sum.ok()) return sum.outcome();
    numerator = sum.value();
  }
  auto level = detail::checked_level(numerator, multiply(to_wide(denominator), to_wide(denominator)));
  if (!level.ok()) return level.outcome();
  return std::optional<Sphere>{Sphere(a, n, denominator, level.value())};
}

}  // namespace mhgp11::num
