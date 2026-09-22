#pragma once

#include "../spindle/q2_prepared_bounds.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

namespace mhgp9::gen {

struct Q34XiBounds {
  i128 low{}, high{};
  bool operator==(const Q34XiBounds&) const = default;
};

// Fixed-endpoint value: owns only arithmetic constants, with no site views,
// allocation, witness credit or mutable query state. a==b is allowed and
// gives no strict H>0 witness. Both query methods validate their box first.
// H bounds are exact extrema of 4H over the CONTINUOUS box. Xi bounds sum
// interval-square bounds of three exact affine cross components: component
// extrema need not occur together, so only the singleton Xi is always exact.
class PreparedPairCitronBounds final {
 public:
  explicit PreparedPairCitronBounds(Point3 a, Point3 b) {
    require_valid_point(a);
    require_valid_point(b);
    for (std::size_t axis = 0; axis < 3; ++axis) {
      difference_[axis] = static_cast<i64>(b[axis]) - a[axis];
      center_twice_[axis] = static_cast<i64>(a[axis]) + b[axis];
      diameter_squared_ += difference_[axis] * difference_[axis];
    }
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto j = (axis + 1) % 3, k = (axis + 2) % 3;
      cross_constant_[axis] = difference_[k] * static_cast<i64>(a[j]) -
                              difference_[j] * static_cast<i64>(a[k]);
    }
  }

  [[nodiscard]] Q2Bounds h_bounds(const Box3& z) const {
    require_valid_box(z);
    return h_bounds_unchecked(z);
  }

  [[nodiscard]] Q34XiBounds xi_bounds(const Box3& z) const {
    require_valid_box(z);
    return xi_bounds_unchecked(z);
  }

 private:
  // The friend only passes immutable index boxes, already range certified.
  friend struct Q34WitnessSearchBoundsAccess;
  [[nodiscard]] Q2Bounds h_bounds_unchecked(const Box3& z) const {
    Q2Bounds result{diameter_squared_, diameter_squared_};
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const i64 low = 2 * static_cast<i64>(z.low[axis]) - center_twice_[axis];
      const i64 high = 2 * static_cast<i64>(z.high[axis]) - center_twice_[axis];
      const i64 low_squared = low * low, high_squared = high * high;
      result.minimum4 -= std::max(low_squared, high_squared);
      result.maximum4 -= low > 0 ? low_squared : high < 0 ? high_squared : 0;
    }
    // 4H=|b-a|^2-|2z-a-b|^2. With M=262143 (18 bits), each coordinate
    // difference is <=M, each doubled displacement <=2M, all these i64
    // intermediates fit. In particular -12M^2<=4H<=3M^2. The nearest
    // point is continuous, retaining half-integral maxima without rounding.
    return result;
  }

  [[nodiscard]] Q34XiBounds xi_bounds_unchecked(const Box3& z) const {
    Q34XiBounds result{};
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto j = (axis + 1) % 3, k = (axis + 2) % 3;
      const i64 first = difference_[j], second = -difference_[k];
      const i64 low = cross_constant_[axis] +
          first * static_cast<i64>(first < 0 ? z.high[k] : z.low[k]) +
          second * static_cast<i64>(second < 0 ? z.high[j] : z.low[j]);
      const i64 high = cross_constant_[axis] +
          first * static_cast<i64>(first < 0 ? z.low[k] : z.high[k]) +
          second * static_cast<i64>(second < 0 ? z.low[j] : z.high[j]);
      const i128 left = static_cast<i128>(low) * low;
      const i128 right = static_cast<i128>(high) * high;
      result.low += low <= 0 && high >= 0 ? 0 : std::min(left, right);
      result.high += std::max(left, right);
    }
    // Xi=|d x (z-a)|^2. Component intervals are EXACT, since each has
    // two independent affine coordinates. |component|<=2M^2; even the
    // separate constant/evaluation intermediates are <=4M^2 and fit i64.
    // Promote BEFORE squaring: Xi_high<=12M^4<2^76. Summing individual
    // square minima/maxima remains conservative despite shared coordinates.
    return result;
  }

  std::array<i64, 3> difference_{}, center_twice_{}, cross_constant_{};
  i64 diameter_squared_{};
};

}  // namespace mhgp9::gen
