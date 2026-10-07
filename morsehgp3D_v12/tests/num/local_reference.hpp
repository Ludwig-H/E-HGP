// References de test du repere local : puissance et orientation toujours larges (384 bits), quel que soit le profil et
// quel que soit le stockage des coefficients ; aucune selection de voie. Les points et ecarts sont des i64.
#pragma once

#include <array>
#include <stdexcept>

#include "num/num.hpp"

namespace local_test {
using namespace mhgp12;
using namespace mhgp12::num;
using W = Wide<6>;

template <class T>
W widen(const T& value) {
  W out;
  if (!resize(to_wide(value), out)) throw std::runtime_error("reference : valeur trop large");
  return out;
}
inline W product(const W& a, const W& b) {
  W out;
  if (!multiply_into(a, b, out)) throw std::runtime_error("reference : produit trop large");
  return out;
}
inline W sum(const W& a, const W& b) {
  W out;
  if (!add(a, b, out)) throw std::runtime_error("reference : somme trop large");
  return out;
}

// D |v|^2 - 2 N.v pour l'ecart v a l'ancre.
template <class Ball>
W power(const Ball& ball, const std::array<i64, 3>& v) {
  W norm;
  for (const i64 x : v) norm = sum(norm, product(widen(i128{x}), widen(i128{x})));
  W total = product(widen(ball.denominator()), norm);
  for (int j = 0; j < 3; ++j) total = sum(total, product(widen(ball.numerator()[j]), widen(-2 * i128{v[j]})));
  return total;
}
template <class Ball>
std::array<i64, 3> offset(const Ball& ball, const std::array<i64, 3>& point) {
  const auto a = ball.anchor().coordinates();
  return {point[0] - a[0], point[1] - a[1], point[2] - a[2]};
}
// Signe de cross(b-a, c-a).(N + D (o - a)), le predicat orientation(a, b, c, centre).
template <class Ball>
int orientation(Point a, Point b, Point c, const Ball& ball) {
  std::array<i128, 3> u{}, v{}, o{};
  for (int j = 0; j < 3; ++j) {
    u[j] = i128{b.coordinates()[j]} - a.coordinates()[j];
    v[j] = i128{c.coordinates()[j]} - a.coordinates()[j];
    o[j] = i128{ball.anchor().coordinates()[j]} - a.coordinates()[j];
  }
  const std::array<i128, 3> normal{u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
  W total;
  for (int j = 0; j < 3; ++j) {
    const W coordinate = sum(widen(ball.numerator()[j]), product(widen(ball.denominator()), widen(o[j])));
    total = sum(total, product(coordinate, widen(normal[j])));
  }
  return total.sign();
}
inline Point point(i64 x, i64 y, i64 z) {
  const auto made = Point::make(x, y, z);
  if (!made.ok()) throw std::runtime_error("point de test hors domaine");
  return made.value();
}
template <class... P>
Sphere sphere(P... points) {
  const auto made = Sphere::through(points...);
  if (!made.ok() || !made.value()) throw std::runtime_error("sphere de test degeneree");
  return *made.value();
}
// Triangle aigu d'etendue s : (0,0,0), (h,h,0), (h,0,h), h = 2^s - 1 (temoin WIT-S21 de la v11 et de l'auditeur).
inline std::array<Point, 3> acute_corner(int s, i64 base = 0) {
  const i64 h = (i64{1} << s) - 1;
  return {point(base, base, base), point(base + h, base + h, base), point(base + h, base, base + h)};
}

}  // namespace local_test
