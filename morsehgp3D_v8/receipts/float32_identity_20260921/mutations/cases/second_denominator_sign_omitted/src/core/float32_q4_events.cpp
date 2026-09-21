#include "core/float32_q4_events.hpp"

#include "core/fixed_signed.hpp"

#include <cstddef>
#include <stdexcept>

namespace mhgp8 {
namespace {
using float32_predicate_detail::count;
using Interval = float32_predicate_detail::Interval;
using Integer = float32_ball_detail::FixedSigned;
template<class Number> using Vector = std::array<Number, 3>;
template<class Number> using Lifted = std::array<Number, 4>;

struct ExactOps {
  using Number = Integer;
  Float32Q4EventsWork& work;
  Number add(const Number& a, const Number& b) { count(work.exact_additions); return a + b; }
  Number sub(const Number& a, const Number& b) { count(work.exact_additions); return a - b; }
  Number mul(const Number& a, const Number& b) { count(work.exact_products); return a * b; }
  Vector<Number> point(const Float32Point3& p) {
    count(work.exact_point_decodes);
    return {Number::from_bits(p.bits()[0]), Number::from_bits(p.bits()[1]), Number::from_bits(p.bits()[2])};
  }
};

struct IntervalOps {
  using Number = Interval;
  Float32Q4EventsWork& work;
  Number add(Number a, Number b) {
    count(work.interval_additions); return float32_predicate_detail::add(a, b);
  }
  Number sub(Number a, Number b) { return add(a, {-b.high, -b.low}); }
  Number mul(Number a, Number b) {
    count(work.interval_products); return float32_predicate_detail::multiply(a, b);
  }
  Vector<Number> point(const Float32Point3& p) {
    Vector<Number> result{};
    for (std::size_t i = 0; i != 3; ++i) {
      const double x = float32_predicate_detail::as_double(p.bits()[i]);
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
template<class Ops> auto lifted(Ops& op, const Vector<typename Ops::Number>& v) {
  return Lifted<typename Ops::Number>{v[0], v[1], v[2], dot(op, v, v)};
}
template<class Ops> auto minors(Ops& op, const Lifted<typename Ops::Number>& a,
                               const Lifted<typename Ops::Number>& b) {
  const auto minor = [&](std::size_t i, std::size_t j) {
    return op.sub(op.mul(a[i], b[j]), op.mul(a[j], b[i]));
  };
  return std::array<typename Ops::Number, 6>{minor(0, 1), minor(0, 2), minor(0, 3),
                                           minor(1, 2), minor(1, 3), minor(2, 3)};
}
template<class Ops> auto determinant(Ops& op, const std::array<typename Ops::Number, 6>& prepared,
                                    const std::array<typename Ops::Number, 6>& query) {
  // Explicit port of the Laplace identity in lanes/q4_family.cpp, with
  // separate interval and fixed-integer arithmetic. Each product has degree
  // five: one minor has degree two and its complement degree three.
  auto result = op.sub(op.mul(prepared[0], query[5]), op.mul(prepared[1], query[4]));
  result = op.add(result, op.mul(prepared[2], query[3]));
  result = op.add(result, op.mul(prepared[3], query[2]));
  result = op.sub(result, op.mul(prepared[4], query[1]));
  return op.add(result, op.mul(prepared[5], query[0]));
}

int known_sign(Interval value) { return value.low > 0 ? 1 : value.high < 0 ? -1 : 0; }

}  // namespace

std::optional<Float32Q4Events> Float32Q4Events::make(std::array<Float32Point3, 3> points,
    Float32PredicateMode mode, Float32Q4EventsWork& work) {
  if (mode != Float32PredicateMode::Filtered && mode != Float32PredicateMode::ExactOnly)
    throw std::invalid_argument("mhgp8 invalid binary32 q4 event mode");
  count(work.preparations);
  if (!Float32Ball::make_q3(points, mode, work.seed)) {
    count(work.rejected_seeds);
    return std::nullopt;
  }
  std::array<Interval, 6> prepared{};
  Vector<Interval> normal{};
  if (mode == Float32PredicateMode::Filtered) {
    count(work.interval_preparations);
    IntervalOps op{work};
    const auto origin = op.point(points[0]);
    const auto d = difference(op, op.point(points[1]), origin);
    const auto u = difference(op, op.point(points[2]), origin);
    prepared = minors(op, lifted(op, d), lifted(op, u));
    normal = {prepared[3], {-prepared[1].high, -prepared[1].low}, prepared[0]};
  }
  count(work.accepted_seeds);
  return Float32Q4Events(points, mode, prepared, normal);
}

int Float32Q4Events::side(const Float32Point3& z, Float32Q4EventsWork& work) const {
  count(work.side_queries);
  if (mode_ == Float32PredicateMode::Filtered) {
    count(work.side_filter_attempts);
    IntervalOps op{work};
    const auto v = difference(op, op.point(z), op.point(points_[0]));
    const int sign = known_sign(dot(op, normal_, v));
    if (sign != 0) { count(work.side_filter_accepts); return sign; }
    count(work.side_exact_fallbacks);
  }
  count(work.side_exact_evaluations);
  ExactOps op{work};
  const auto origin = op.point(points_[0]);
  const auto d = difference(op, op.point(points_[1]), origin);
  const auto u = difference(op, op.point(points_[2]), origin);
  const auto v = difference(op, op.point(z), origin);
  return dot(op, cross(op, d, u), v).sign();
}

int Float32Q4Events::compare_roots(const Float32Point3& z1, const Float32Point3& z2,
                                  Float32Q4EventsWork& work) const {
  count(work.root_queries);
  const int b1 = side(z1, work), b2 = side(z2, work);
  if (b1 == 0 || b2 == 0) {
    count(work.root_coplanar_rejections);
    throw std::invalid_argument("mhgp8 binary32 q4 roots require two noncoplanar sites");
  }
  if (mode_ == Float32PredicateMode::Filtered) {
    count(work.root_filter_attempts);
    IntervalOps op{work};
    const auto origin = op.point(points_[0]);
    const auto v1 = difference(op, op.point(z1), origin);
    const auto v2 = difference(op, op.point(z2), origin);
    const auto query = minors(op, lifted(op, v1), lifted(op, v2));
    const int sign = known_sign(determinant(op, minors_, query));
    if (sign != 0) { count(work.root_filter_accepts); return -sign * b1; }
    count(work.root_exact_fallbacks);
  }
  count(work.root_exact_evaluations);
  ExactOps op{work};
  const auto origin = op.point(points_[0]);
  const auto d = difference(op, op.point(points_[1]), origin);
  const auto u = difference(op, op.point(points_[2]), origin);
  const auto v1 = difference(op, op.point(z1), origin);
  const auto v2 = difference(op, op.point(z2), origin);
  const auto prepared = minors(op, lifted(op, d), lifted(op, u));
  const auto query = minors(op, lifted(op, v1), lifted(op, v2));
  // Differences are <D=2^278 in dyadic integer units. Each determinant
  // partial sum is <72*D^5<2^1397; side partial sums are <6*D^3<2^837.
  // The 1728-bit core therefore covers every intermediate, unlike a direct
  // degree-nine cross-product of rational roots. Physical-unit interval
  // partial sums are <72*2^(5*129)<2^652, below double overflow.
  const int sign = determinant(op, prepared, query).sign();
  if (sign == 0) count(work.root_equalities);
  // G*delta=P2*B1-P1*B2, G>0. BOTH denominator signs are essential.
  return -sign * b1;
}

}  // namespace mhgp8
