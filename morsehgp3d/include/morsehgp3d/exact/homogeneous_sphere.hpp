#pragma once

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/exact/support.hpp"

#include <array>
#include <cstddef>
#include <stdexcept>

namespace morsehgp3d::exact {

// One axis coordinate borrowed from an ExactRational3 in its stored
// homogeneous form.  Reading a coordinate through ExactRational3::coordinate
// builds a reduced ExactRational, which costs a greatest common divisor; the
// decisions below never need the reduced form, so they borrow instead.  The
// referenced value must outlive the view.
struct ExactAxisCoordinate {
  const BigInt* numerator{};
  const BigInt* denominator{};
};

[[nodiscard]] inline ExactAxisCoordinate exact_axis_coordinate(
    const ExactRational3& point,
    std::size_t axis) {
  return ExactAxisCoordinate{&point.numerator(axis), &point.denominator()};
}

// One sphere in homogeneous integer form: centre (x, y, z) / w with w > 0 and
// squared level ln / ld with ld > 0.
//
// Every decision here is the same exact decision the normalized rational
// predicates take -- three-way against the same squared distance -- but it is
// taken with integer arithmetic only.  No intermediate value is reduced, so
// no greatest common divisor runs on the hot path.  That is not a micro
// detail: on the ~220-bit operands an exact circumcenter really produces,
// reducing every intermediate cost about ninety microseconds per classified
// point, and the closed-ball traversal classifies one point per visited leaf.
//
// The exactness argument is elementary.  With centre x_a / w and query
// coordinate p_a / q (one denominator per point, as ExactRational3 stores
// it), the squared distance is sum_a (p_a w - x_a q)^2 / (q w)^2, so
// comparing it against ln / ld is comparing ld * sum_a (p_a w - x_a q)^2
// against ln * w^2 * q^2 -- both sides positive integers, both denominators
// cleared, no rounding anywhere.
class ExactHomogeneousSphere3 {
 public:
  ExactHomogeneousSphere3(
      const ExactRational3& center,
      const ExactLevel& squared_level)
      : center_numerator_{
            center.numerator(0), center.numerator(1), center.numerator(2)},
        center_denominator_(center.denominator()),
        level_numerator_(squared_level.numerator()),
        level_denominator_(squared_level.denominator()),
        level_numerator_times_center_denominator_squared_(
            squared_level.numerator() * center.denominator() *
            center.denominator()) {
    if (center_denominator_ <= 0 || level_denominator_ <= 0) {
      throw std::invalid_argument(
          "a homogeneous sphere requires positive denominators");
    }
  }

  [[nodiscard]] SpherePointLocation classify_point(
      const ExactRational3& point) const {
    const BigInt& point_denominator = point.denominator();
    BigInt squared_sum{0};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const BigInt delta = point.numerator(axis) * center_denominator_ -
          center_numerator_[axis] * point_denominator;
      squared_sum += delta * delta;
    }
    const BigInt left = squared_sum * level_denominator_;
    const BigInt right = level_numerator_times_center_denominator_squared_ *
        point_denominator * point_denominator;
    if (left < right) {
      return SpherePointLocation::strictly_inside;
    }
    if (left > right) {
      return SpherePointLocation::outside;
    }
    return SpherePointLocation::boundary;
  }

  // `minimum squared distance to the box > level`: no point of the box can
  // lie in the closed ball.
  [[nodiscard]] bool box_minimum_squared_distance_exceeds_level(
      const std::array<ExactAxisCoordinate, 3>& lower,
      const std::array<ExactAxisCoordinate, 3>& upper) const {
    std::array<BigInt, 3> numerator{};
    std::array<const BigInt*, 3> denominator{};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const BigInt lower_scaled = *lower[axis].numerator * center_denominator_;
      const BigInt center_scaled_to_lower =
          center_numerator_[axis] * *lower[axis].denominator;
      if (center_scaled_to_lower < lower_scaled) {
        numerator[axis] = lower_scaled - center_scaled_to_lower;
        denominator[axis] = lower[axis].denominator;
        continue;
      }
      const BigInt upper_scaled = *upper[axis].numerator * center_denominator_;
      const BigInt center_scaled_to_upper =
          center_numerator_[axis] * *upper[axis].denominator;
      if (center_scaled_to_upper > upper_scaled) {
        numerator[axis] = center_scaled_to_upper - upper_scaled;
        denominator[axis] = upper[axis].denominator;
        continue;
      }
      numerator[axis] = 0;
      denominator[axis] = &one_;
    }
    return compare_axis_fractions_with_level(numerator, denominator) > 0;
  }

  // `maximum squared distance to the box < level`: every point of the box
  // lies in the open ball.
  [[nodiscard]] bool box_maximum_squared_distance_is_below_level(
      const std::array<ExactAxisCoordinate, 3>& lower,
      const std::array<ExactAxisCoordinate, 3>& upper) const {
    std::array<BigInt, 3> numerator{};
    std::array<const BigInt*, 3> denominator{};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const BigInt to_lower = center_numerator_[axis] *
              *lower[axis].denominator -
          *lower[axis].numerator * center_denominator_;
      const BigInt to_upper = center_numerator_[axis] *
              *upper[axis].denominator -
          *upper[axis].numerator * center_denominator_;
      // |to_lower| / lower_denominator against |to_upper| / upper_denominator.
      const BigInt lower_magnitude = magnitude(to_lower);
      const BigInt upper_magnitude = magnitude(to_upper);
      if (lower_magnitude * *upper[axis].denominator >=
          upper_magnitude * *lower[axis].denominator) {
        numerator[axis] = lower_magnitude;
        denominator[axis] = lower[axis].denominator;
      } else {
        numerator[axis] = upper_magnitude;
        denominator[axis] = upper[axis].denominator;
      }
    }
    return compare_axis_fractions_with_level(numerator, denominator) < 0;
  }

 private:
  // Three-way comparison of sum_a (numerator_a / (denominator_a * w))^2
  // against the sphere level.  Every denominator is positive.
  [[nodiscard]] int compare_axis_fractions_with_level(
      const std::array<BigInt, 3>& numerator,
      const std::array<const BigInt*, 3>& denominator) const {
    const BigInt denominator_product =
        *denominator[0] * *denominator[1] * *denominator[2];
    BigInt squared_sum{0};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const BigInt scaled =
          numerator[axis] * (denominator_product / *denominator[axis]);
      squared_sum += scaled * scaled;
    }
    const BigInt left = squared_sum * level_denominator_;
    const BigInt right = level_numerator_times_center_denominator_squared_ *
        denominator_product * denominator_product;
    if (left < right) {
      return -1;
    }
    return left > right ? 1 : 0;
  }

  std::array<BigInt, 3> center_numerator_;
  BigInt center_denominator_;
  BigInt level_numerator_;
  BigInt level_denominator_;
  BigInt level_numerator_times_center_denominator_squared_;
  BigInt one_{1};
};

}  // namespace morsehgp3d::exact
