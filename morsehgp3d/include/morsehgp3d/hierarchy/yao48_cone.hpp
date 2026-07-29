#pragma once

#include "morsehgp3d/exact/point.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace morsehgp3d::hierarchy {

inline constexpr std::size_t exact_yao48_cone_count = 48U;

// The 48 half-open chambers are the eight sign choices times the six strict
// total orders of |dx|, |dy| and |dz|.  Zero is assigned the positive sign and
// equal absolute deltas are ordered by increasing axis.  The resulting key is
// therefore a partition, not an overlapping proposal convention.
struct ExactYao48ConeKey {
  std::uint8_t negative_axis_mask{};
  std::array<std::uint8_t, 3> axes_by_descending_absolute_delta{};
  std::size_t cone_index{};

  friend bool operator==(const ExactYao48ConeKey&, const ExactYao48ConeKey&) =
      default;
};

// Exact classification of target-source.  Equal points are rejected because
// the zero vector has no direction and canonical point clouds disallow them.
[[nodiscard]] ExactYao48ConeKey classify_exact_yao48_cone(
    const exact::CertifiedPoint3& source,
    const exact::CertifiedPoint3& target);

}  // namespace morsehgp3d::hierarchy
