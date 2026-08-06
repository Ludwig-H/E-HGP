#pragma once

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/fp64_interval.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/exact/support.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
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

namespace homogeneous_sphere_detail {

// One integer as a binary64 interval that provably contains it: the backend
// conversion rounds, and one outward ulp on each side covers that rounding.
// An integer too large for the format yields an invalid interval, which the
// filter reads as "undecided" and answers exactly.
// Above this magnitude the backend conversion itself is out of range for the
// format, so the value is refused before it is converted rather than after.
inline constexpr unsigned int integer_interval_maximum_bit_index = 1000U;

[[nodiscard]] inline detail::Binary64Interval integer_interval(
    const BigInt& value) {
  if (value == 0) {
    return detail::point_binary64_interval(0.0);
  }
  if (boost::multiprecision::msb(magnitude(value)) >=
      integer_interval_maximum_bit_index) {
    return detail::invalid_binary64_interval();
  }
  const double converted = static_cast<double>(value);
  if (!std::isfinite(converted)) {
    return detail::invalid_binary64_interval();
  }
  return detail::outward_interval(converted, converted);
}

[[nodiscard]] inline detail::Binary64Interval fraction_interval(
    const BigInt& numerator,
    const BigInt& denominator) {
  return detail::divide_binary64_intervals(
      integer_interval(numerator), integer_interval(denominator));
}

}  // namespace homogeneous_sphere_detail

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
    // The filter is an accelerator, never an authority: it answers only when
    // its enclosures are disjoint, and every other case falls to the integer
    // decision below.  It is armed only under the strict floating-point
    // environment the exact target exports.
    if (fp64_filter_environment_supported()) {
      for (std::size_t axis = 0U; axis < 3U; ++axis) {
        center_interval_[axis] = homogeneous_sphere_detail::fraction_interval(
            center_numerator_[axis], center_denominator_);
        filter_armed_ = filter_armed_ && center_interval_[axis].valid;
      }
      level_interval_ = homogeneous_sphere_detail::fraction_interval(
          level_numerator_, level_denominator_);
      filter_armed_ = filter_armed_ && level_interval_.valid;
    } else {
      filter_armed_ = false;
    }
  }

  // Cloud points carry their binary64 origin exactly, so the filter reads the
  // query coordinates with no conversion and no widening at all -- this is
  // the form the closed-ball traversal calls once per visited leaf.
  [[nodiscard]] SpherePointLocation classify_point(
      const CertifiedPoint3& point) const {
    if (filter_armed_) {
      detail::Binary64Interval squared_distance =
          detail::point_binary64_interval(0.0);
      bool usable = true;
      for (std::size_t axis = 0U; axis < 3U; ++axis) {
        const detail::Binary64Interval coordinate =
            detail::point_binary64_interval(point.binary64_coordinate(axis));
        usable = usable && coordinate.valid;
        squared_distance = detail::add_binary64_intervals(
            squared_distance,
            detail::square_binary64_interval(
                detail::subtract_binary64_intervals(
                    coordinate, center_interval_[axis])));
      }
      if (usable && squared_distance.valid) {
        if (squared_distance.upper < level_interval_.lower) {
          return SpherePointLocation::strictly_inside;
        }
        if (squared_distance.lower > level_interval_.upper) {
          return SpherePointLocation::outside;
        }
      }
    }
    return classify_point(point.exact());
  }

  [[nodiscard]] SpherePointLocation classify_point(
      const ExactRational3& point) const {
    if (filter_armed_) {
      const std::optional<SpherePointLocation> filtered =
          filtered_point_location(point);
      if (filtered.has_value()) {
        return *filtered;
      }
    }
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

  // The same two box decisions on a box whose per-axis extremes are cloud
  // points: the filter reads their exact binary64 coordinates, the integer
  // path stays the authority.
  [[nodiscard]] bool box_minimum_squared_distance_exceeds_level(
      const std::array<const CertifiedPoint3*, 3>& lower,
      const std::array<const CertifiedPoint3*, 3>& upper) const {
    const std::optional<bool> filtered =
        filtered_binary64_box_comparison(lower, upper, false);
    if (filtered.has_value()) {
      return *filtered;
    }
    return box_minimum_squared_distance_exceeds_level(
        exact_box_bounds(lower), exact_box_bounds(upper));
  }

  [[nodiscard]] bool box_maximum_squared_distance_is_below_level(
      const std::array<const CertifiedPoint3*, 3>& lower,
      const std::array<const CertifiedPoint3*, 3>& upper) const {
    const std::optional<bool> filtered =
        filtered_binary64_box_comparison(lower, upper, true);
    if (filtered.has_value()) {
      return *filtered;
    }
    return box_maximum_squared_distance_is_below_level(
        exact_box_bounds(lower), exact_box_bounds(upper));
  }

  // `minimum squared distance to the box > level`: no point of the box can
  // lie in the closed ball.
  [[nodiscard]] bool box_minimum_squared_distance_exceeds_level(
      const std::array<ExactAxisCoordinate, 3>& lower,
      const std::array<ExactAxisCoordinate, 3>& upper) const {
    if (filter_armed_) {
      const std::optional<bool> filtered =
          filtered_box_comparison(lower, upper, /*maximum_distance=*/false);
      if (filtered.has_value()) {
        return *filtered;
      }
    }
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
    if (filter_armed_) {
      const std::optional<bool> filtered =
          filtered_box_comparison(lower, upper, /*maximum_distance=*/true);
      if (filtered.has_value()) {
        return *filtered;
      }
    }
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
  [[nodiscard]] static std::array<ExactAxisCoordinate, 3> exact_box_bounds(
      const std::array<const CertifiedPoint3*, 3>& bound) {
    return {
        exact_axis_coordinate(bound[0]->exact(), 0U),
        exact_axis_coordinate(bound[1]->exact(), 1U),
        exact_axis_coordinate(bound[2]->exact(), 2U)};
  }

  [[nodiscard]] std::optional<bool> filtered_binary64_box_comparison(
      const std::array<const CertifiedPoint3*, 3>& lower,
      const std::array<const CertifiedPoint3*, 3>& upper,
      bool maximum_distance) const {
    if (!filter_armed_) {
      return std::nullopt;
    }
    detail::Binary64Interval squared_distance =
        detail::point_binary64_interval(0.0);
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const detail::Binary64Interval lower_coordinate =
          detail::point_binary64_interval(
              lower[axis]->binary64_coordinate(axis));
      const detail::Binary64Interval upper_coordinate =
          detail::point_binary64_interval(
              upper[axis]->binary64_coordinate(axis));
      if (!lower_coordinate.valid || !upper_coordinate.valid) {
        return std::nullopt;
      }
      squared_distance = detail::add_binary64_intervals(
          squared_distance,
          detail::square_binary64_interval(axis_distance_interval(
              axis, lower_coordinate, upper_coordinate, maximum_distance)));
    }
    return decide_box_comparison(squared_distance, maximum_distance);
  }

  // The per-axis distance from the centre to the box, as an enclosure: the
  // farthest corner, or the nearest point of the interval (which is zero when
  // the centre is inside it).
  [[nodiscard]] detail::Binary64Interval axis_distance_interval(
      std::size_t axis,
      const detail::Binary64Interval& lower_coordinate,
      const detail::Binary64Interval& upper_coordinate,
      bool maximum_distance) const {
    const detail::Binary64Interval to_lower =
        detail::subtract_binary64_intervals(
            center_interval_[axis], lower_coordinate);
    const detail::Binary64Interval to_upper =
        detail::subtract_binary64_intervals(
            center_interval_[axis], upper_coordinate);
    if (maximum_distance) {
      return detail::maximum_binary64_intervals(
          detail::magnitude_binary64_interval(to_lower),
          detail::magnitude_binary64_interval(to_upper));
    }
    return detail::maximum_binary64_intervals(
        detail::maximum_binary64_intervals(
            detail::subtract_binary64_intervals(
                lower_coordinate, center_interval_[axis]),
            to_upper),
        detail::point_binary64_interval(0.0));
  }

  [[nodiscard]] std::optional<bool> decide_box_comparison(
      const detail::Binary64Interval& squared_distance,
      bool maximum_distance) const {
    if (!squared_distance.valid) {
      return std::nullopt;
    }
    if (maximum_distance) {
      if (squared_distance.upper < level_interval_.lower) {
        return true;
      }
      if (squared_distance.lower > level_interval_.upper) {
        return false;
      }
      return std::nullopt;
    }
    if (squared_distance.lower > level_interval_.upper) {
      return true;
    }
    if (squared_distance.upper < level_interval_.lower) {
      return false;
    }
    return std::nullopt;
  }

  // The filtered point decision: an enclosure of the squared distance and an
  // enclosure of the level, answered only when they are disjoint.
  [[nodiscard]] std::optional<SpherePointLocation> filtered_point_location(
      const ExactRational3& point) const {
    detail::Binary64Interval squared_distance =
        detail::point_binary64_interval(0.0);
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const detail::Binary64Interval coordinate =
          homogeneous_sphere_detail::fraction_interval(
              point.numerator(axis), point.denominator());
      if (!coordinate.valid) {
        return std::nullopt;
      }
      squared_distance = detail::add_binary64_intervals(
          squared_distance,
          detail::square_binary64_interval(detail::subtract_binary64_intervals(
              coordinate, center_interval_[axis])));
    }
    if (!squared_distance.valid) {
      return std::nullopt;
    }
    if (squared_distance.upper < level_interval_.lower) {
      return SpherePointLocation::strictly_inside;
    }
    if (squared_distance.lower > level_interval_.upper) {
      return SpherePointLocation::outside;
    }
    return std::nullopt;
  }

  // The filtered box decisions.  `maximum_distance` selects the farthest
  // corner comparison (`maximum squared distance < level`) instead of the
  // nearest point comparison (`minimum squared distance > level`).
  [[nodiscard]] std::optional<bool> filtered_box_comparison(
      const std::array<ExactAxisCoordinate, 3>& lower,
      const std::array<ExactAxisCoordinate, 3>& upper,
      bool maximum_distance) const {
    detail::Binary64Interval squared_distance =
        detail::point_binary64_interval(0.0);
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const detail::Binary64Interval lower_coordinate =
          homogeneous_sphere_detail::fraction_interval(
              *lower[axis].numerator, *lower[axis].denominator);
      const detail::Binary64Interval upper_coordinate =
          homogeneous_sphere_detail::fraction_interval(
              *upper[axis].numerator, *upper[axis].denominator);
      if (!lower_coordinate.valid || !upper_coordinate.valid) {
        return std::nullopt;
      }
      squared_distance = detail::add_binary64_intervals(
          squared_distance,
          detail::square_binary64_interval(axis_distance_interval(
              axis, lower_coordinate, upper_coordinate, maximum_distance)));
    }
    return decide_box_comparison(squared_distance, maximum_distance);
  }

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
  std::array<detail::Binary64Interval, 3> center_interval_{};
  detail::Binary64Interval level_interval_{};
  bool filter_armed_{true};
};

}  // namespace morsehgp3d::exact
