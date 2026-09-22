#pragma once

#include "../core/types.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <type_traits>

namespace mhgp9::gen {

struct Q2Bounds {
  i64 minimum4{};
  i64 maximum4{};
};

// Exact extrema of 4H(a,b,z) on {a} x B x Z, with a fixed and B prepared
// once. This constant-size value contains no point views, dynamic storage,
// witness counts or task history. Its trivial representation does not itself
// qualify a parallel/GPU execution or a bound on the number of tasks.
class Q2PreparedBounds final {
 public:
  Q2PreparedBounds(const Point3& a, const Box3& b) {
    require_valid_point(a);
    require_valid_box(b);
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const i64 anchor = a[axis];
      const std::array<i64, 2> endpoints{b.low[axis], b.high[axis]};
      for (std::size_t side = 0; side < 2; ++side) {
        const i64 difference = endpoints[side] - anchor;
        constants_[axis][side] = {
            static_cast<std::uint64_t>(anchor + endpoints[side]),
            static_cast<std::uint64_t>(difference * difference)};
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
  std::array<std::array<Constants, 2>, 3> constants_{};

  // The census uses only boxes constructed inside its validated immutable
  // index. Other callers use the checked public entry point above.
  friend struct Q2CensusEngine;
  // The singleton path never queries this inert value. It avoids an optional
  // discriminator; a nonsingleton task installs all prepared constants once.
  Q2PreparedBounds() = default;
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

// For each endpoint e of one coordinate of B, C=a+e and D=(e-a)^2
// give 4H=D-(2z-C)^2. H is affine in b and concave in z, so minimizing
// uses the farthest endpoint of 2Z, while maximizing uses its nearest point.
// The two endpoint squares are reused for both extrema, with nearest square
// zero when C lies inside 2Z (also for half-integral geometric centers).
// With M=262143 (18 bits): C<=2M<2^19, D<=M^2<2^36, |2z-C|<=2M, each
// square<=4M^2<2^38, and -12M^2<=4H<=3M^2 (<2^40). Promotion precedes every
// product; i64 is ample. The stored constants are u64: D no longer fits u32
// (65535^2 did, 262143^2=68718952449 does not), and a u32 store would have
// truncated silently. They are promoted back to i64 before differences.
static_assert(std::is_trivially_copyable_v<Q2PreparedBounds>);
static_assert(sizeof(Q2PreparedBounds) == 12 * sizeof(std::uint64_t));

}  // namespace mhgp9::gen
