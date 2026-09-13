#pragma once

#include "../core/types.hpp"

#include <algorithm>
#include <array>

namespace mhgp8 {

// A, B and Z below are continuous boxes, not just the stored sites in them.
// This decision concerns witness credit, never the death of an anchor.
// Credit: every z in Z witnesses every (a,b) in A x B.
// NoCredit: every (a,z) in A x Z fails universality over the BOX B. A failing
// box corner need not itself be a stored site: this does not refute a weaker
// certificate concerning only the discrete sites of B.
// Uncertain: neither statement is certified; it is not a rejection.
enum class BlockDecision : std::uint8_t { Credit, NoCredit, Uncertain };

namespace spindle_detail {

struct Interval {
  i64 low;
  i64 high;
};

[[nodiscard]] inline Interval coordinate(const Box3& box, std::size_t axis) {
  return {static_cast<i64>(box.low[axis]), static_cast<i64>(box.high[axis])};
}

[[nodiscard]] inline Interval subtract(Interval left, Interval right) {
  return {left.low - right.high, left.high - right.low};
}

[[nodiscard]] inline Interval multiply(Interval left, Interval right) {
  const std::array<i64, 4> products{
      left.low * right.low, left.low * right.high,
      left.high * right.low, left.high * right.high};
  const auto bounds = std::minmax_element(products.begin(), products.end());
  return {*bounds.first, *bounds.second};
}

[[nodiscard]] inline i128 square(i64 value) {
  return static_cast<i128>(value) * value;
}

struct SquaredBounds {
  i128 low;
  i128 high;
};

[[nodiscard]] inline SquaredBounds square_bounds(Interval value) {
  const i128 left = square(value.low);
  const i128 right = square(value.high);
  return {value.low <= 0 && value.high >= 0 ? 0 : std::min(left, right),
          std::max(left, right)};
}

// M=65535. Coordinate differences have magnitude <=M, products <=M^2,
// cross components <=2M^2, hence Xi<=12M^4<2^68. Dependencies between
// intervals are discarded only outward: both Xi bounds remain certified.
[[nodiscard]] inline SquaredBounds xi_bounds(const Box3& a, const Box3& b,
                                           const Box3& z) {
  std::array<Interval, 3> u{};
  std::array<Interval, 3> w{};
  for (std::size_t axis = 0; axis < 3; ++axis) {
    u[axis] = subtract(coordinate(z, axis), coordinate(a, axis));
    w[axis] = subtract(coordinate(b, axis), coordinate(z, axis));
  }
  SquaredBounds result{0, 0};
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const std::size_t j = (axis + 1) % 3;
    const std::size_t k = (axis + 2) % 3;
    const auto component = square_bounds(
        subtract(multiply(u[j], w[k]), multiply(u[k], w[j])));
    result.low += component.low;
    result.high += component.high;
  }
  return result;
}

// H=(z-a).(b-z) is separable by coordinate, affine separately in a and b,
// and concave in z. Its minimum over three boxes is therefore attained at
// coordinate endpoints. The largest partial sum has magnitude <=3M^2.
[[nodiscard]] inline i64 h_minimum(const Box3& a, const Box3& b,
                                 const Box3& z) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const Interval ac = coordinate(a, axis);
    const Interval bc = coordinate(b, axis);
    const Interval zc = coordinate(z, axis);
    const auto at_z = [ac, bc](i64 value) {
      return multiply({value - ac.high, value - ac.low},
                      {bc.low - value, bc.high - value}).low;
    };
    result += std::min(at_z(zc.low), at_z(zc.high));
  }
  return result;
}

// Four times the exact maximum of H over A x {b} x Z. For each coordinate,
// maximize over a at an endpoint, then over z at clamp((a+b)/2,Z).
// Doubling the vertex and multiplying H by four avoids division/rounding.
// Each term is in [-4M^2,M^2], so the sum is in [-12M^2,3M^2].
[[nodiscard]] inline i64 h_maximum_times_four(const Box3& a, const Point3& b,
                                            const Box3& z) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 bc = static_cast<i64>(b[axis]);
    const i64 low_twice = 2 * static_cast<i64>(z.low[axis]);
    const i64 high_twice = 2 * static_cast<i64>(z.high[axis]);
    const auto at_a = [bc, low_twice, high_twice](i64 ac) {
      const i64 sum = ac + bc;
      const i64 closest = std::clamp(sum, low_twice, high_twice);
      const i64 delta = closest - sum;
      const i64 distance = bc - ac;
      return distance * distance - delta * delta;
    };
    result += std::max(at_a(static_cast<i64>(a.low[axis])),
                       at_a(static_cast<i64>(a.high[axis])));
  }
  return result;
}

[[nodiscard]] inline unsigned multiplier(Lane lane) {
  return lane == Lane::Q3 ? 3U : 2U;
}

}  // namespace spindle_detail

[[nodiscard]] inline bool point_witness(Lane lane, const Point3& a,
                                       const Point3& b, const Point3& z,
                                       PredicateWork& work) {
  require_valid_lane(lane);
  counter_add(work.point_tests);
  std::array<i64, 3> u{};
  std::array<i64, 3> w{};
  i64 h = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    u[axis] = static_cast<i64>(z[axis]) - static_cast<i64>(a[axis]);
    w[axis] = static_cast<i64>(b[axis]) - static_cast<i64>(z[axis]);
    h += u[axis] * w[axis];
  }
  if (h <= 0) {
    return false;
  }
  if (lane == Lane::Q2) {
    return true;
  }
  i128 xi = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const std::size_t j = (axis + 1) % 3;
    const std::size_t k = (axis + 2) % 3;
    const i64 cross = u[j] * w[k] - u[k] * w[j];
    xi += spindle_detail::square(cross);
  }
  // 3H^2<=27M^4<2^69. Promotion occurs before either multiplication.
  return static_cast<i128>(spindle_detail::multiplier(lane)) *
             spindle_detail::square(h) > xi;
}

[[nodiscard]] inline bool universal_witness(Lane lane, const Point3& a,
                                           const Box3& b, const Point3& z,
                                           PredicateWork& work) {
  require_valid_lane(lane);
  require_valid_box(b);
  counter_add(work.universal_queries);
  if (lane == Lane::Q2) {
    // H is affine in b: choose the minimizing bound independently per axis.
    // This is the exact continuous-box minimum, not a heuristic proposal.
    i64 h = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const i64 delta = static_cast<i64>(z[axis]) - static_cast<i64>(a[axis]);
      const i64 bound =
          static_cast<i64>(delta >= 0 ? b.low[axis] : b.high[axis]);
      h += delta * (bound - static_cast<i64>(z[axis]));
      counter_add(work.q2_axis_terms);
    }
    return h > 0;
  }
  // For fixed a,z, H>0 and sqrt(t)H>|(z-a)x(b-z)| is an open convex
  // cone in b. All eight corners are necessary and sufficient for the box.
  for (unsigned corner = 0; corner < 8; ++corner) {
    counter_add(work.corner_tests);
    if (!point_witness(lane, a, box_corner(b, corner), z, work)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline bool box_witness(Lane lane, const Box3& a,
                                     const Box3& b, const Point3& z,
                                     PredicateWork& work) {
  require_valid_lane(lane);
  require_valid_box(a);
  require_valid_box(b);
  // Convexity is separate in a and b, not assumed joint. Applying the
  // corner certificate successively proves the entire Cartesian product.
  for (unsigned corner = 0; corner < 8; ++corner) {
    if (!universal_witness(lane, box_corner(a, corner), b, z, work)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline BlockDecision classify_witness_block(
    Lane lane, const Box3& a, const Box3& b, const Box3& z,
    PredicateWork& work) {
  require_valid_lane(lane);
  require_valid_box(a);
  require_valid_box(b);
  require_valid_box(z);
  counter_add(work.block_bound_tests);

  const i64 h_min = spindle_detail::h_minimum(a, b, z);
  if (h_min > 0) {
    if (lane == Lane::Q2) {
      return BlockDecision::Credit;
    }
    const auto xi = spindle_detail::xi_bounds(a, b, z);
    if (static_cast<i128>(spindle_detail::multiplier(lane)) *
            spindle_detail::square(h_min) > xi.high) {
      return BlockDecision::Credit;
    }
  }

  // A single fixed b0 in B which fails for ALL a in A and z in Z proves
  // that no (a,z) can supply a universal-over-B credit. This does NOT
  // prove that those anchors or their candidate supports may be dropped.
  for (unsigned corner = 0; corner < 8; ++corner) {
    counter_add(work.negative_probes);
    const Point3 b0 = box_corner(b, corner);
    const i64 h_max4 = spindle_detail::h_maximum_times_four(a, b0, z);
    if (h_max4 <= 0) {
      return BlockDecision::NoCredit;
    }
    if (lane != Lane::Q2) {
      const auto xi = spindle_detail::xi_bounds(a, singleton_box(b0), z);
      // H_max = h_max4/4; equality fails the required strict witness test.
      // 16Xi<=192M^4<2^72; all products are promoted to i128 first.
      if (static_cast<i128>(spindle_detail::multiplier(lane)) *
              spindle_detail::square(h_max4) <= 16 * xi.low) {
        return BlockDecision::NoCredit;
      }
    }
  }
  return BlockDecision::Uncertain;
}

}  // namespace mhgp8
