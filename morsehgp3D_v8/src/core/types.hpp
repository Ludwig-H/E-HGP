#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace mhgp8 {

using u64 = std::uint64_t;
using i64 = std::int64_t;

// Quantized integer coordinates on a grid of 2^coordinate_bits positions per
// axis (18 bits: 1 mm over +-131 m). Storage is a signed 32-bit integer so
// that sums and differences of two coordinates promote exactly as the
// historical u16 fields did (to int); prepare_cloud refuses any value outside
// [0, coordinate_limit], so every bound below is proved with M=coordinate_limit.
// The 16-bit inputs of the 2 cm profile remain valid and produce bit-identical
// outputs; each factor M of a bound gains two bits relative to M=65535.
using Coordinate = std::int32_t;
inline constexpr unsigned coordinate_bits = 18;
inline constexpr Coordinate coordinate_limit = (Coordinate{1} << coordinate_bits) - 1;
// Midpoint splits halve a positive integer extent at most coordinate_bits
// times per axis: an index path has at most max_index_depth splits and a
// binary DFS at most index_stack_frames pending frames. Proved bounds, never
// exploration caps.
inline constexpr std::size_t max_index_depth = 3 * coordinate_bits;
inline constexpr std::size_t index_stack_frames = max_index_depth + 1;
static_assert(coordinate_limit == 262143 && max_index_depth == 54);

// The local-credit geometry (including tube bounds) needs at most 83 signed
// bits at 18 bits (16Q<=768*M^4<2^82). Later lane primitives document their
// own wider bounds; the widest (q3 balls) stays below 2^117.
// This CPU implementation requires the compiler's native integer extension; it does
// not silently replace exact predicates with floating-point arithmetic.
#if defined(__SIZEOF_INT128__)
__extension__ typedef signed __int128 i128;
#else
#error "mhgp8 CPU predicates require a compiler with signed 128-bit integers"
#endif

struct Point3 {
  Coordinate x{};
  Coordinate y{};
  Coordinate z{};

  [[nodiscard]] constexpr Coordinate operator[](std::size_t axis) const {
    switch (axis) {
      case 0: return x;
      case 1: return y;
      case 2: return z;
      default: throw std::out_of_range("mhgp8 point axis must be 0, 1 or 2");
    }
  }

  bool operator==(const Point3&) const = default;
};

struct Box3 {
  Point3 low{};
  Point3 high{};
};

enum class Lane : std::uint8_t { Q2 = 2, Q3 = 3, Q4 = 4 };

[[nodiscard]] constexpr bool valid_lane(Lane lane) noexcept {
  return lane == Lane::Q2 || lane == Lane::Q3 || lane == Lane::Q4;
}

inline void require_valid_lane(Lane lane) {
  if (!valid_lane(lane)) {
    throw std::invalid_argument("mhgp8 lane must be Q2, Q3 or Q4");
  }
}

[[nodiscard]] inline unsigned arity(Lane lane) {
  require_valid_lane(lane);
  return static_cast<unsigned>(lane);
}

[[nodiscard]] constexpr bool valid_box(const Box3& box) noexcept {
  return box.low.x <= box.high.x && box.low.y <= box.high.y &&
         box.low.z <= box.high.z;
}

inline void require_valid_box(const Box3& box) {
  if (!valid_box(box)) {
    throw std::invalid_argument("mhgp8 box has inverted bounds");
  }
}

[[nodiscard]] constexpr Box3 singleton_box(const Point3& point) noexcept {
  return {point, point};
}

[[nodiscard]] inline Point3 box_corner(const Box3& box, unsigned corner) {
  require_valid_box(box);
  if (corner >= 8) {
    throw std::out_of_range("mhgp8 box corner must be in [0, 8)");
  }
  return {(corner & 1U) != 0U ? box.high.x : box.low.x,
          (corner & 2U) != 0U ? box.high.y : box.low.y,
          (corner & 4U) != 0U ? box.high.z : box.low.z};
}

struct PredicateWork {
  u64 point_tests{};
  u64 universal_queries{};
  u64 q2_axis_terms{};
  u64 corner_tests{};
  u64 block_bound_tests{};
  u64 negative_probes{};
};

inline void counter_add(u64& value, u64 increment = 1) {
  if (increment > std::numeric_limits<u64>::max() - value) {
    throw std::overflow_error("mhgp8 work counter overflow");
  }
  value += increment;
}

}  // namespace mhgp8
