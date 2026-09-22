#include "lanes/exact_ball.hpp"

#include <cstddef>

namespace mhgp9::gen {
namespace {

using SmallVector = std::array<i64, 3>;
using WideVector = std::array<i128, 3>;

SmallVector coordinates(Point3 point) noexcept {
  return {point.x, point.y, point.z};
}

SmallVector difference(Point3 point, Point3 origin) noexcept {
  return {static_cast<i64>(point.x) - origin.x,
          static_cast<i64>(point.y) - origin.y,
          static_cast<i64>(point.z) - origin.z};
}

i64 dot_small(const SmallVector& a, const SmallVector& b) noexcept {
  // Used only for coordinates/differences, each of magnitude <=M=262143.
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

SmallVector cross(const SmallVector& a, const SmallVector& b) noexcept {
  // Each product <=M^2 and each component <=2*M^2, safely within i64.
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
          a[0] * b[1] - a[1] * b[0]};
}

i128 dot_wide(const WideVector& a, const SmallVector& b) noexcept {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

i128 magnitude(i128 value) noexcept {
  // All callers have |value|<2^117; negation cannot encounter INT128_MIN.
  return value < 0 ? -value : value;
}

i128 gcd(i128 a, i128 b) noexcept {
  while (b != 0) {
    const auto remainder = a % b;
    a = b;
    b = remainder;
  }
  return a;
}

std::array<i128, 5> primitive(std::array<i128, 5> value) noexcept {
  // Only factories call this helper, always with A>0. A bounds the final
  // positive divisor, including when any/all other coefficients are zero.
  auto divisor = value[0];
  for (std::size_t i = 1; i < value.size(); ++i)
    divisor = gcd(divisor, magnitude(value[i]));
  for (auto& coefficient : value) coefficient /= divisor;
  return value;
}

std::array<i128, 5> translated(i128 scale, const WideVector& linear, Point3 origin) noexcept {
  // scale*|z-origin|^2-linear.(z-origin), expanded in GLOBAL coordinates.
  const auto a = coordinates(origin);
  std::array<i128, 5> value{scale, 0, 0, 0,
      scale * dot_small(a, a) + dot_wide(linear, a)};
  for (std::size_t axis = 0; axis < 3; ++axis)
    value[axis + 1] = -2 * scale * a[axis] - linear[axis];
  return primitive(value);
}

}  // namespace

std::optional<ExactBall> ExactBall::make_q2(Point3 a, Point3 b) {
  require_valid_point(a);
  require_valid_point(b);
  if (a == b) return std::nullopt;
  // (z-a).(z-b): A=1 already forces the common gcd to one.
  // |B_i|<=2*M, |C|<=3*M^2, and all arithmetic fits i64 before widening.
  const auto av = coordinates(a);
  const auto bv = coordinates(b);
  return ExactBall({1, -static_cast<i128>(av[0] + bv[0]),
      -static_cast<i128>(av[1] + bv[1]), -static_cast<i128>(av[2] + bv[2]),
      static_cast<i128>(dot_small(av, bv))});
}

std::optional<ExactBall> ExactBall::make_q3(std::array<Point3, 3> points) {
  for (const auto point : points) require_valid_point(point);
  const auto d = difference(points[1], points[0]);
  const auto u = difference(points[2], points[0]);
  const i64 dd = dot_small(d, d);
  const i64 uu = dot_small(u, u);
  const i64 du = dot_small(d, u);
  if (du <= 0 || dd - du <= 0 || uu - du <= 0) return std::nullopt;
  const i128 gram = static_cast<i128>(dd) * uu - static_cast<i128>(du) * du;
  if (gram <= 0) return std::nullopt;
  const i128 along_d = static_cast<i128>(uu) * (dd - du);
  const i128 along_u = static_cast<i128>(dd) * (uu - du);
  WideVector linear{};
  for (std::size_t axis = 0; axis < 3; ++axis)
    linear[axis] = along_d * d[axis] + along_u * u[axis];
  // Relative power G*|z-a|^2-W.(z-a); W=E(D-F)d+D(E-F)u.
  // Conservative bounds (M=262143): G<=12*M^4, |W_i|<=36*M^5. After
  // translation, A<=12*M^4, |B_i|<=60*M^5, |C|<=144*M^6. Every intermediate
  // and the global power are bounded in magnitude by 360*M^6<2^117; gcd only divides.
  return ExactBall(translated(gram, linear, points[0]));
}

std::optional<ExactBall> ExactBall::make_q4(std::array<Point3, 4> points) {
  for (const auto point : points) require_valid_point(point);
  const std::array<SmallVector, 3> d{
      difference(points[1], points[0]), difference(points[2], points[0]),
      difference(points[3], points[0])};
  const std::array<SmallVector, 3> cofactors{
      cross(d[1], d[2]), cross(d[2], d[0]), cross(d[0], d[1])};
  // This mixed dot can reach 6*M^3; promotion precedes all products.
  i128 determinant = 0;
  for (std::size_t axis = 0; axis < 3; ++axis)
    determinant += static_cast<i128>(d[0][axis]) * cofactors[0][axis];
  if (determinant == 0) return std::nullopt;

  WideVector numerator{};
  for (std::size_t row = 0; row < 3; ++row) {
    const auto squared_length = dot_small(d[row], d[row]);
    for (std::size_t axis = 0; axis < 3; ++axis)
      numerator[axis] += static_cast<i128>(squared_length) * cofactors[row][axis];
  }
  // Cramer: centre-a = numerator/(2*det). Barycentric weights of the three
  // non-origin vertices have common POSITIVE denominator 2*det^2, not 2*det.
  // Test them and the origin's remaining weight BEFORE normalizing orientation.
  const i128 denominator = 2 * determinant * determinant;
  i128 remaining = denominator;
  for (const auto& cofactor : cofactors) {
    const auto weight = dot_wide(numerator, cofactor);
    if (weight <= 0) return std::nullopt;
    remaining -= weight;
  }
  if (remaining <= 0) return std::nullopt;

  // |det|<=6*M^3, |numerator_i|<=18*M^4; each barycentric numerator is
  // <=108*M^6 in magnitude, and all four-weight intermediates are <2^117.
  // Normalize orientation only now: A=|det| and lin=sign(det)*numerator.
  if (determinant < 0) {
    determinant = -determinant;
    for (auto& coefficient : numerator) coefficient = -coefficient;
  }
  // Relative power A*|z-a|^2-lin.(z-a). Global |B_i|<=30*M^4,
  // |C|<=72*M^5; the global power is <=180*M^5<2^98 before gcd reduction.
  return ExactBall(translated(determinant, numerator, points[0]));
}

i128 ExactBall::power(Point3 z) const noexcept {
  const auto point = coordinates(z);
  // The q3 bound (<2^117) dominates q2/q4, including all partial sums.
  i128 value = coefficients_[0] * dot_small(point, point);
  for (std::size_t axis = 0; axis < 3; ++axis)
    value += coefficients_[axis + 1] * point[axis];
  return value + coefficients_[4];
}

}  // namespace mhgp9::gen
