#pragma once

#include "../spindle/q2_prepared_bounds.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <type_traits>

namespace mhgp8 {

// Exact extrema of 4H on A x B x Z. Only the two query boxes are prepared;
// the value owns arithmetic constants, not sites, counts or continuations.
class Q2JointPreparedBounds final {
 public:
  Q2JointPreparedBounds(const Box3& a, const Box3& b) {
    require_valid_box(a);
    require_valid_box(b);
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const std::array<i64, 2> a_ends{a.low[axis], a.high[axis]};
      const std::array<i64, 2> b_ends{b.low[axis], b.high[axis]};
      for (std::size_t ai = 0; ai < 2; ++ai) {
        for (std::size_t bi = 0; bi < 2; ++bi) {
          const i64 difference = b_ends[bi] - a_ends[ai];
          constants_[axis][2 * ai + bi] = {
              static_cast<std::uint64_t>(a_ends[ai] + b_ends[bi]),
              static_cast<std::uint64_t>(difference * difference)};
        }
      }
    }
  }

  [[nodiscard]] Q2Bounds bounds(const Box3& z) const {
    require_valid_box(z);
    return bounds_unchecked(z);
  }

 private:
  struct Constants {
    std::uint64_t center_twice{};
    std::uint64_t distance_squared{};
  };
  std::array<std::array<Constants, 4>, 3> constants_{};

  friend struct Q2CensusEngine;
  // Only the census may create an inert value before installing prepared
  // constants. Public callers cannot accidentally query an unprepared box.
  Q2JointPreparedBounds() = default;

  [[nodiscard]] Q2Bounds bounds_unchecked(const Box3& z) const {
    Q2Bounds result;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const i64 low_twice = 2 * static_cast<i64>(z.low[axis]);
      const i64 high_twice = 2 * static_cast<i64>(z.high[axis]);
      i64 minimum4 = std::numeric_limits<i64>::max();
      i64 maximum4 = std::numeric_limits<i64>::min();
      for (const auto& constant : constants_[axis]) {
        const i64 center = static_cast<i64>(constant.center_twice);
        const i64 distance = static_cast<i64>(constant.distance_squared);
        const i64 low_delta = low_twice - center;
        const i64 high_delta = high_twice - center;
        const i64 low_squared = low_delta * low_delta;
        const i64 high_squared = high_delta * high_delta;
        const i64 nearest_squared = low_delta > 0 ? low_squared
                                        : high_delta < 0 ? high_squared : 0;
        minimum4 = std::min(minimum4, distance - std::max(low_squared, high_squared));
        maximum4 = std::max(maximum4, distance - nearest_squared);
      }
      result.minimum4 += minimum4;
      result.maximum4 += maximum4;
    }
    return result;
  }
};

// H is separately affine in a and b, concave in z and separable by axis.
// Its extrema therefore use the four endpoint pairs (a,b) per axis. For
// each pair, C=a+b and D=(b-a)^2 give 4H=D-(2z-C)^2. The farthest endpoint
// of 2Z gives the minimum; its nearest point to C gives the maximum. This
// includes half-integral summits without rounding and uses all four pairs,
// not only matching low/low and high/high endpoints.
//
// With M=262143 (18 bits): C<=2M<2^19, D<=M^2<2^36, |2z-C|<=2M. Stored C,D
// are u64 (D no longer fits u32 beyond 16 bits), while every subtraction and
// product above is performed in i64. Summed bounds satisfy
// -12*M^2<=4H<=3*M^2 (<2^40). The 192-byte representation does not qualify
// a task-count bound, a GPU kernel or an HGP FULL tower.
static_assert(std::is_trivially_copyable_v<Q2JointPreparedBounds>);
static_assert(sizeof(Q2JointPreparedBounds) == 24 * sizeof(std::uint64_t));

}  // namespace mhgp8
