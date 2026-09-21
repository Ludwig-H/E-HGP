#include "core/float32_q3_block.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace mhgp8 {
namespace {
using float32_predicate_detail::count;
using float32_predicate_detail::as_double;
using float32_predicate_detail::down;
using float32_predicate_detail::up;
using Interval = float32_predicate_detail::Interval;
using Vector = std::array<Interval, 3>;

struct Operations {
  Float32Q3BlockWork& work;
  Interval add(Interval a, Interval b) {
    count(work.interval_additions);
    return float32_predicate_detail::add(a, b);
  }
  Interval sub(Interval a, Interval b) { return add(a, {-b.high, -b.low}); }
  Interval mul(Interval a, Interval b) {
    count(work.interval_products);
    return float32_predicate_detail::multiply(a, b);
  }
  Interval square(Interval a) {
    count(work.interval_products);
    const double largest = std::max(std::abs(a.low), std::abs(a.high));
    const double smallest = a.low <= 0 && a.high >= 0 ? 0 : std::min(std::abs(a.low), std::abs(a.high));
    // Nonnegativity is algebraic. Widening around zero must not accidentally
    // make the lower Gram bound positive under directed rounding or FTZ.
    return {std::max(0.0, down(smallest * smallest)), up(largest * largest)};
  }
  Interval divide_positive(Interval numerator, Interval denominator) {
    count(work.interval_divisions);
    count(work.scalar_divisions, 2);
    // denominator.low>0 is established by the caller. With positive
    // denominator endpoints these are the two exact extremal quotients.
    const double low = numerator.low / (numerator.low < 0 ? denominator.low : denominator.high);
    const double high = numerator.high / (numerator.high < 0 ? denominator.high : denominator.low);
    // A quotient can overflow even though each polynomial is finite. Signed
    // infinities are conservative here; intersect with the finite geometric
    // hull before storing any centre or evaluating any power. down/up also
    // enclose a quotient underflow flushed to zero (normal endpoint widening).
    return {down(low), up(high)};
  }
};

Vector difference(Operations& op, const Vector& a, const Vector& b) {
  return {op.sub(a[0], b[0]), op.sub(a[1], b[1]), op.sub(a[2], b[2])};
}
Interval dot(Operations& op, const Vector& a, const Vector& b) {
  return op.add(op.add(op.mul(a[0], b[0]), op.mul(a[1], b[1])), op.mul(a[2], b[2]));
}
Interval norm2(Operations& op, const Vector& value) {
  auto result = op.add(op.add(op.square(value[0]), op.square(value[1])), op.square(value[2]));
  result.low = std::max(0.0, result.low);
  return result;
}
Vector cross(Operations& op, const Vector& a, const Vector& b) {
  return {op.sub(op.mul(a[1], b[2]), op.mul(a[2], b[1])),
          op.sub(op.mul(a[2], b[0]), op.mul(a[0], b[2])),
          op.sub(op.mul(a[0], b[1]), op.mul(a[1], b[0]))};
}
Interval value(double x) { return {x, x}; }

Interval power_axis(Operations& op, double z, double a, double center) {
  count(op.work.power_evaluations);
  // (z-a)*(z+a-2c), written as differences to avoid needlessly cancelling
  // large globally translated coordinates. No square of a centre coefficient.
  return op.mul(op.sub(value(z), value(a)),
                op.add(op.sub(value(z), value(center)), op.sub(value(a), value(center))));
}

}  // namespace

Float32Q3Block Float32Q3Block::make(const Float32Point3& a, const Float32Point3& b,
                                  const Float32Box3& x, Float32Q3BlockWork& work) {
  count(work.preparations);
  Operations op{work};
  std::array<double, 3> origin{};
  Vector av{}, bv{}, xv{}, hull{};
  for (std::size_t axis = 0; axis != 3; ++axis) {
    origin[axis] = as_double(a.bits()[axis]);
    av[axis] = value(origin[axis]);
    bv[axis] = value(as_double(b.bits()[axis]));
    xv[axis] = {as_double(x.low()[axis]), as_double(x.high()[axis])};
    hull[axis] = {std::min({av[axis].low, bv[axis].low, xv[axis].low}),
                  std::max({av[axis].high, bv[axis].high, xv[axis].high})};
  }
  const auto d = difference(op, bv, av), u = difference(op, xv, av);
  // |d cross u|^2=G. This avoids subtracting two large overlapping Gram
  // intervals and preserves the useful fact G>=0 for every point of X.
  const auto gram = norm2(op, cross(op, d, u));
  if (gram.low <= 0) {
    count(work.gram_unresolved);
    return Float32Q3Block(origin, hull);
  }
  count(work.gram_positive);
  const auto dd = norm2(op, d), uu = norm2(op, u), du = dot(op, d, u);
  const auto along_d = op.mul(uu, op.sub(dd, du));
  const auto along_u = op.mul(dd, op.sub(uu, du));
  // Positive interval endpoints are normal doubles (down/up widen tinies),
  // hence doubling and one outward ulp keep this denominator strictly >0.
  // Physical G is bounded well below double overflow.
  const auto denominator = op.add(gram, gram);
  Vector tightened{};
  bool empty = false;
  for (std::size_t axis = 0; axis != 3; ++axis) {
    const auto linear = op.add(op.mul(along_d, d[axis]), op.mul(along_u, u[axis]));
    const auto candidate = op.add(av[axis], op.divide_positive(linear, denominator));
    tightened[axis] = {std::max(hull[axis].low, candidate.low), std::min(hull[axis].high, candidate.high)};
    empty = empty || tightened[axis].low > tightened[axis].high;
  }
  if (empty) {
    count(work.center_intersection_fallbacks);
    return Float32Q3Block(origin, hull);
  }
  for (std::size_t axis = 0; axis != 3; ++axis)
    if (tightened[axis].low > hull[axis].low || tightened[axis].high < hull[axis].high)
      count(work.center_axes_tightened);
  return Float32Q3Block(origin, tightened);
}

Float32Q3Block::Interval Float32Q3Block::bounds(const Float32Box3& z, Float32Q3BlockWork& work) const {
  count(work.bound_queries);
  Operations op{work};
  Interval total{};
  for (std::size_t axis = 0; axis != 3; ++axis) {
    const double low = as_double(z.low()[axis]), high = as_double(z.high()[axis]);
    Interval contribution{};
    bool first = true;
    for (const double center : {centers_[axis].low, centers_[axis].high}) {
      count(work.axis_parabolas);
      count(work.vertex_clamps);
      // At fixed z the expression is affine in c, so BOTH centre endpoints
      // suffice. At fixed c it is convex in real z: minimum at clamp(c,Z),
      // maximum at either endpoint. Do not import integer-grid floor/ceil.
      const double vertex = std::clamp(center, low, high);
      const auto at_vertex = power_axis(op, vertex, origin_[axis], center);
      const auto at_low = power_axis(op, low, origin_[axis], center);
      const auto at_high = power_axis(op, high, origin_[axis], center);
      const double maximum = std::max(at_low.high, at_high.high);
      if (first) {
        contribution = {at_vertex.low, maximum};
        first = false;
      } else {
        contribution.low = std::min(contribution.low, at_vertex.low);
        contribution.high = std::max(contribution.high, maximum);
      }
    }
    total = op.add(total, contribution);
  }
  // Published centres are in hull of finite binary32 inputs. Differences
  // have magnitude <2^129, factors below2^130, sum below2^261 including
  // outward rounding. No coefficient quotient/infinity reaches this stage.
  if (total.high < 0) count(work.inside_certificates);
  else if (total.low > 0) count(work.outside_certificates);
  else count(work.unknown_bounds);
  return total;
}

int Float32Q3Block::classify(const Float32Box3& z, Float32Q3BlockWork& work) const {
  const auto result = bounds(z, work);
  return result.high < 0 ? -1 : result.low > 0 ? 1 : 0;
}

}  // namespace mhgp8
