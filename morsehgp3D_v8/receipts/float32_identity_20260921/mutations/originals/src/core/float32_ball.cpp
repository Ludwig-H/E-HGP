#include "core/float32_ball.hpp"

#include "core/fixed_signed.hpp"

#include <stdexcept>

namespace mhgp8 {
namespace {

using float32_predicate_detail::count;
using Interval = float32_predicate_detail::Interval;
using Integer = float32_ball_detail::FixedSigned;
template<class Number> using Vector = std::array<Number, 3>;

struct ExactOps {
  using Number = Integer;
  Float32BallWork& work;
  Number add(const Number& a, const Number& b) { count(work.exact_additions); return a + b; }
  Number sub(const Number& a, const Number& b) { count(work.exact_additions); return a - b; }
  Number mul(const Number& a, const Number& b) { count(work.exact_products); return a * b; }
  Number neg(const Number& a) { return -a; }
  Vector<Number> point(const Float32Point3& p) {
    count(work.exact_point_decodes);
    return {Number::from_bits(p.bits()[0]), Number::from_bits(p.bits()[1]),
            Number::from_bits(p.bits()[2])};
  }
};

struct IntervalOps {
  using Number = Interval;
  Float32BallWork& work;
  Number add(Number a, Number b) {
    count(work.interval_additions); return float32_predicate_detail::add(a, b);
  }
  Number neg(Number a) { return {-a.high, -a.low}; }
  Number sub(Number a, Number b) { return add(a, neg(b)); }
  Number mul(Number a, Number b) {
    count(work.interval_products); return float32_predicate_detail::multiply(a, b);
  }
  Vector<Number> point(const Float32Point3& p) {
    Vector<Number> result{};
    for (std::size_t i = 0; i != 3; ++i) {
      const auto x = float32_predicate_detail::as_double(p.bits()[i]);
      result[i] = {x, x};
    }
    return result;
  }
};

template<class Ops> auto difference(Ops& op, const Vector<typename Ops::Number>& a,
                                  const Vector<typename Ops::Number>& b) {
  return Vector<typename Ops::Number>{op.sub(a[0], b[0]), op.sub(a[1], b[1]), op.sub(a[2], b[2])};
}
template<class Ops> auto dot(Ops& op, const Vector<typename Ops::Number>& a,
                           const Vector<typename Ops::Number>& b) {
  return op.add(op.add(op.mul(a[0], b[0]), op.mul(a[1], b[1])), op.mul(a[2], b[2]));
}
template<class Ops> auto cross(Ops& op, const Vector<typename Ops::Number>& a,
                             const Vector<typename Ops::Number>& b) {
  return Vector<typename Ops::Number>{op.sub(op.mul(a[1], b[2]), op.mul(a[2], b[1])),
      op.sub(op.mul(a[2], b[0]), op.mul(a[0], b[2])),
      op.sub(op.mul(a[0], b[1]), op.mul(a[1], b[0]))};
}

template<class Number> struct Coefficients {
  Number scale{};
  Vector<Number> linear{};
  std::array<Number, 4> positive{};
  Vector<Number> origin{};
};

template<class Ops> auto coefficients(Ops& op, const std::array<Float32Point3, 4>& points,
                                     std::size_t arity, bool validate) {
  using Number = typename Ops::Number;
  Coefficients<Number> result;
  result.origin = op.point(points[0]);
  const auto& origin = result.origin;
  const auto d = difference(op, op.point(points[1]), origin);
  const auto u = difference(op, op.point(points[2]), origin);
  const auto dd = dot(op, d, d), uu = dot(op, u, u);
  if (arity == 3) {
    const auto du = dot(op, d, u);
    const auto dd_du = op.sub(dd, du), uu_du = op.sub(uu, du);
    result.scale = op.sub(op.mul(dd, uu), op.mul(du, du));
    if (validate) result.positive = {du, dd_du, uu_du, result.scale};
    const auto along_d = op.mul(uu, dd_du), along_u = op.mul(dd, uu_du);
    for (std::size_t i = 0; i != 3; ++i)
      result.linear[i] = op.add(op.mul(along_d, d[i]), op.mul(along_u, u[i]));
  } else {
    const auto v = difference(op, op.point(points[3]), origin);
    const auto vv = dot(op, v, v);
    const std::array<Vector<Number>, 3> cofactor{cross(op, u, v), cross(op, v, d), cross(op, d, u)};
    result.scale = dot(op, d, cofactor[0]);
    for (std::size_t i = 0; i != 3; ++i)
      result.linear[i] = op.add(op.add(op.mul(dd, cofactor[0][i]), op.mul(uu, cofactor[1][i])),
                                op.mul(vv, cofactor[2][i]));
    // Barycentric denominator is POSITIVE 2*det^2, not 2*det.
    // Do NOT normalize orientation before all four weights are tested.
    if (validate) {
      const auto square = op.mul(result.scale, result.scale);
      auto remaining = op.add(square, square);
      for (std::size_t i = 0; i != 3; ++i) {
        result.positive[i] = dot(op, result.linear, cofactor[i]);
        remaining = op.sub(remaining, result.positive[i]);
      }
      result.positive[3] = remaining;
    }
  }
  return result;
}

int known_sign(Interval x) {
  return x.low > 0 ? 1 : x.high < 0 ? -1 : 0;
}

// +1 all strict conditions proven; -1 at least one disproven; 0 unknown.
int eligibility(const Coefficients<Interval>& c) {
  bool all_positive = true;
  for (const auto x : c.positive) {
    const auto sign = known_sign(x);
    if (sign < 0) return -1;
    all_positive = all_positive && sign > 0;
  }
  return all_positive && known_sign(c.scale) != 0 ? 1 : 0;
}
bool eligible(const Coefficients<Integer>& c) {
  if (c.scale.sign() == 0) return false;
  for (const auto& x : c.positive) if (x.sign() <= 0) return false;
  return true;
}

template<class Ops> auto power(Ops& op, const typename Ops::Number& scale,
                               const Vector<typename Ops::Number>& linear,
                               const Vector<typename Ops::Number>& displacement) {
  return op.sub(op.mul(scale, dot(op, displacement, displacement)), dot(op, linear, displacement));
}

}  // namespace

std::optional<Float32Ball> Float32Ball::make_q3(std::array<Float32Point3, 3> points,
    Float32PredicateMode mode, Float32BallWork& work) {
  return make({points[0], points[1], points[2], points[0]}, 3, mode, work);
}
std::optional<Float32Ball> Float32Ball::make_q4(std::array<Float32Point3, 4> points,
    Float32PredicateMode mode, Float32BallWork& work) {
  return make(points, 4, mode, work);
}
std::optional<Float32Ball> Float32Ball::make(std::array<Float32Point3, 4> points,
    std::size_t arity, Float32PredicateMode mode, Float32BallWork& work) {
  if (mode != Float32PredicateMode::ExactOnly && mode != Float32PredicateMode::Filtered)
    throw std::invalid_argument("mhgp8 invalid binary32 ball mode");
  count(arity == 3 ? work.q3_preparations : work.q4_preparations);
  Coefficients<Interval> bounds;
  int decision = 0, orientation = 0;
  if (mode == Float32PredicateMode::Filtered) {
    count(work.preparation_filter_attempts);
    IntervalOps op{work};
    bounds = coefficients(op, points, arity, true);
    decision = eligibility(bounds);
    if (decision != 0) {
      count(work.preparation_filter_accepts);
      orientation = known_sign(bounds.scale);
    } else count(work.preparation_exact_fallbacks);
  }
  if (decision == 0) {
    count(work.preparation_exact_evaluations);
    ExactOps op{work};
    const auto exact = coefficients(op, points, arity, true);
    decision = eligible(exact) ? 1 : -1;
    orientation = exact.scale.sign();
  }
  if (decision < 0) { count(work.rejected_supports); return std::nullopt; }
  if (mode == Float32PredicateMode::Filtered && orientation < 0) {
    bounds.scale = {-bounds.scale.high, -bounds.scale.low};
    for (auto& x : bounds.linear) x = {-x.high, -x.low};
  }
  count(work.accepted_supports);
  return Float32Ball(points, arity, mode, bounds.scale, bounds.linear);
}

int Float32Ball::power_sign(const Float32Point3& z, Float32BallWork& work) const {
  count(work.power_queries);
  if (mode_ == Float32PredicateMode::Filtered) {
    count(work.power_filter_attempts);
    IntervalOps op{work};
    const auto v = difference(op, op.point(z), op.point(points_[0]));
    const auto sign = known_sign(power(op, scale_, linear_, v));
    if (sign != 0) { count(work.power_filter_accepts); return sign; }
    count(work.power_exact_fallbacks);
  }
  count(work.power_exact_evaluations);
  ExactOps op{work};
  // Exact coefficients are temporary, not a large per-candidate or shared
  // mutable cache. All values use the common dyadic unit 2^-149.
  const auto exact = coefficients(op, points_, arity_, false);
  const auto v = difference(op, op.point(z), exact.origin);
  const auto sign = power(op, exact.scale, exact.linear, v).sign();
  return sign * exact.scale.sign();
}

std::array<Integer, 5> Float32Ball::global_coefficients(Float32BallWork& work) const {
  ExactOps op{work};
  auto exact = coefficients(op, points_, arity_, false);
  if (exact.scale.sign() < 0) {
    exact.scale = -exact.scale;
    for (auto& x : exact.linear) x = -x;
  }
  // A*|Q-a|^2-W.(Q-a), expanded in the single GLOBAL unit 2^-149.
  // q3: A<=12M^4, |B_i|<=60M^5, |C|<=144M^6, M=2^278.
  // q4: A<=6M^3, |B_i|<=30M^4, |C|<=72M^5. Degree<=6 fits 1728 bits.
  std::array<Integer, 5> result{};
  result[0] = exact.scale;
  for (std::size_t i = 0; i != 3; ++i) {
    const auto translated = op.mul(exact.scale, exact.origin[i]);
    result[i + 1] = op.sub(-op.add(translated, translated), exact.linear[i]);
  }
  result[4] = op.add(op.mul(exact.scale, dot(op, exact.origin, exact.origin)),
                     dot(op, exact.linear, exact.origin));
  return result;
}

}  // namespace mhgp8
