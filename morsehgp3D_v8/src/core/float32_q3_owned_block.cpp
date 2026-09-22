#include "core/float32_q3_owned_block.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace mhgp8 {
namespace {
using float32_predicate_detail::as_double;
using float32_predicate_detail::count;
using float32_predicate_detail::down;
using float32_predicate_detail::up;
using Interval = float32_predicate_detail::Interval;
using Vector = std::array<Interval, 3>;

// Explicit port of Float32Q3Block's outward operations and continuous
// parabolas. Only the centre envelope changes; the older free-acute contract
// is untouched. No binary32 subtraction or epsilon enters this path.
struct Operations {
  Float32Q3OwnedBlockWork& work;
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
    return {std::max(0.0, down(smallest * smallest)), up(largest * largest)};
  }
  Interval divide_positive(Interval a, Interval b) {
    count(work.interval_divisions);
    count(work.scalar_divisions, 2);
    return {down(a.low / (a.low < 0 ? b.low : b.high)),
            up(a.high / (a.high < 0 ? b.high : b.low))};
  }
};
Interval value(double x) { return {x, x}; }
Vector difference(Operations& op, const Vector& a, const Vector& b) {
  return {op.sub(a[0], b[0]), op.sub(a[1], b[1]), op.sub(a[2], b[2])};
}
Interval dot(Operations& op, const Vector& a, const Vector& b) {
  return op.add(op.add(op.mul(a[0], b[0]), op.mul(a[1], b[1])), op.mul(a[2], b[2]));
}
Interval norm2(Operations& op, const Vector& a) {
  auto result = op.add(op.add(op.square(a[0]), op.square(a[1])), op.square(a[2]));
  result.low = std::max(0.0, result.low);
  return result;
}
Vector cross(Operations& op, const Vector& a, const Vector& b) {
  return {op.sub(op.mul(a[1], b[2]), op.mul(a[2], b[1])),
          op.sub(op.mul(a[2], b[0]), op.mul(a[0], b[2])),
          op.sub(op.mul(a[0], b[1]), op.mul(a[1], b[0]))};
}
Interval power_axis(Operations& op, double z, double a, double center) {
  count(op.work.power_evaluations);
  return op.mul(op.sub(value(z), value(a)),
                op.add(op.sub(value(z), value(center)), op.sub(value(a), value(center))));
}
}  // namespace

Float32Q3OwnedBlock::PreparedEdge Float32Q3OwnedBlock::prepare_edge(
    const Float32Point3& a, const Float32Point3& b, Float32Q3OwnedBlockWork& work) {
  count(work.edge_preparations);
  Operations op{work};
  std::array<double, 3> origin{}, other{};
  Vector av{}, bv{}, midpoint{};
  for (std::size_t axis = 0; axis != 3; ++axis) {
    origin[axis] = as_double(a.bits()[axis]);
    other[axis] = as_double(b.bits()[axis]);
    av[axis] = value(origin[axis]);
    bv[axis] = value(other[axis]);
    midpoint[axis] = op.mul(op.add(av[axis], bv[axis]), value(0.5));
  }
  const auto delta = difference(op, bv, av);
  const auto diameter = norm2(op, delta);
  std::array<Vector, 3> projection{};
  const bool ready = diameter.low > 0;
  if (ready) {
    count(work.edge_squared_positive);
    for (std::size_t row = 0; row != 3; ++row)
      for (std::size_t col = 0; col != 3; ++col)
        projection[row][col] = op.sub(value(row == col ? 1 : 0),
            op.divide_positive(op.mul(delta[row], delta[col]), diameter));
  } else {
    count(work.edge_squared_unresolved);
  }
  return PreparedEdge(origin, other, delta, midpoint, diameter, projection, ready);
}

Float32Q3OwnedBlock Float32Q3OwnedBlock::make(
    const Float32Point3& a, const Float32Point3& b, const Float32Box3& x,
    Float32Q3OwnedBlockWork& work) {
  const auto edge = prepare_edge(a, b, work);
  return make(edge, x, work);
}

Float32Q3OwnedBlock Float32Q3OwnedBlock::make(
    const PreparedEdge& edge, const Float32Box3& x, Float32Q3OwnedBlockWork& work) {
  count(work.block_preparations);
  Operations op{work};
  Vector hull{}, relative{};
  for (std::size_t axis = 0; axis != 3; ++axis) {
    const Interval coordinate{as_double(x.low()[axis]), as_double(x.high()[axis])};
    hull[axis] = {std::min({edge.a_[axis], edge.b_[axis], coordinate.low}),
                  std::max({edge.a_[axis], edge.b_[axis], coordinate.high})};
    relative[axis] = op.sub(coordinate, value(edge.a_[axis]));
  }
  if (!edge.ready_) return Float32Q3OwnedBlock(edge.a_, hull);
  count(work.projected_envelopes);
  Vector height{};
  for (std::size_t axis = 0; axis != 3; ++axis)
    height[axis] = dot(op, edge.projection_[axis], relative);

  // Let d=b-a, u=x-a, D=d.d, E=u.u, F=d.u, G=|d cross u|^2.
  // c=(a+b)/2+xi*(I-dd^t/D)u and xi=D(E-F)/(2G). Strict acuteness
  // and ab maximal imply 0<xi<=1/3. Ties attain 1/3 and stay included.
  // This is CONDITIONAL on ownership/acuteness, never evidence that X
  // contains a valid seed. Other X sites remain global census witnesses.
  const auto third = op.divide_positive(value(1), value(3));
  Interval xi{0, third.high};
  const auto gram = norm2(op, cross(op, edge.delta_, relative));
  if (gram.low > 0) {
    count(work.gram_positive);
    const auto numerator = op.mul(edge.diameter_,
        op.sub(norm2(op, relative), dot(op, edge.delta_, relative)));
    const auto ratio = op.divide_positive(numerator, op.add(gram, gram));
    const Interval tightened{std::max(xi.low, ratio.low), std::min(xi.high, ratio.high)};
    if (tightened.low > tightened.high) {
      count(work.xi_intersection_fallbacks);
    } else {
      if (tightened.low > xi.low || tightened.high < xi.high) count(work.xi_tightened);
      xi = tightened;
    }
  } else {
    count(work.gram_unresolved);
  }
  Vector centers{};
  bool empty = false;
  for (std::size_t axis = 0; axis != 3; ++axis) {
    const auto projected = op.add(edge.midpoint_[axis], op.mul(xi, height[axis]));
    centers[axis] = {std::max(hull[axis].low, projected.low),
                     std::min(hull[axis].high, projected.high)};
    empty = empty || centers[axis].low > centers[axis].high;
  }
  // An empty intersection may simply mean all points of X violate the
  // conditional contract. Never turn it into a seed rejection here.
  if (empty) {
    count(work.center_intersection_fallbacks);
    return Float32Q3OwnedBlock(edge.a_, hull);
  }
  for (std::size_t axis = 0; axis != 3; ++axis)
    if (centers[axis].low > hull[axis].low || centers[axis].high < hull[axis].high)
      count(work.center_axes_tightened);
  // Every polynomial above has degree <=4 in physical input coordinates:
  // differences <2^129, G and D(E-F) <2^524. Quotients may temporarily
  // overflow but are clipped to finite xi/hull BEFORE power evaluation.
  // No degree-9 operation: exact point fallback remains Float32Ball's
  // degree-6, <2^1677 integer bound in the common unit 2^-149.
  return Float32Q3OwnedBlock(edge.a_, centers);
}

Float32Q3OwnedBlock::Interval Float32Q3OwnedBlock::bounds(
    const Float32Box3& z, Float32Q3OwnedBlockWork& work) const {
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
  // Finite hull centres ensure products/sums <2^261. Continuous minima
  // occur at the clamped vertex, NOT only at the eight box corners.
  if (total.high < 0) count(work.inside_certificates);
  else if (total.low > 0) count(work.outside_certificates);
  else count(work.unknown_bounds);
  return total;
}

int Float32Q3OwnedBlock::classify(const Float32Box3& z, Float32Q3OwnedBlockWork& work) const {
  const auto result = bounds(z, work);
  return result.high < 0 ? -1 : result.low > 0 ? 1 : 0;
}
}  // namespace mhgp8
