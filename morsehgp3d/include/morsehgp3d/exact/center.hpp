#pragma once

#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/plane.hpp"
#include "morsehgp3d/exact/point.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace morsehgp3d::exact {

// ExactRational3 already is the canonical homogeneous coordinate contract:
// one strictly positive common denominator and a common gcd of one.
using ExactCenter3 = ExactRational3;

enum class CircumcenterKind {
  unique,
  affinely_dependent,
};

[[nodiscard]] inline std::string_view to_string(CircumcenterKind kind) {
  switch (kind) {
    case CircumcenterKind::unique:
      return "unique";
    case CircumcenterKind::affinely_dependent:
      return "affinely_dependent";
  }
  throw std::invalid_argument("circumcenter kind is invalid");
}

// A unique result couples its homogeneous center and squared level. A
// dependent support exposes only its exact affine dimension; dimensional
// reduction to a smaller minimal support belongs to a later certified lot.
class CircumcenterResult {
 public:
  [[nodiscard]] static CircumcenterResult unique(
      std::size_t support_size,
      ExactCenter3 center,
      ExactLevel squared_level) {
    if (support_size < 1U || support_size > 4U) {
      throw std::invalid_argument(
          "a circumcenter support must contain between one and four points");
    }
    const bool zero_level = squared_level.numerator() == 0;
    if ((support_size == 1U) != zero_level) {
      throw std::invalid_argument(
          "a unique circumcenter has a zero level exactly for a singleton support");
    }
    return CircumcenterResult{
        CircumcenterKind::unique,
        support_size,
        support_size - 1U,
        std::move(center),
        std::move(squared_level)};
  }

  [[nodiscard]] static CircumcenterResult affinely_dependent(
      std::size_t support_size, std::size_t affine_dimension) {
    return CircumcenterResult{
        CircumcenterKind::affinely_dependent,
        support_size,
        affine_dimension,
        std::nullopt,
        std::nullopt};
  }

  [[nodiscard]] CircumcenterKind kind() const noexcept { return kind_; }

  [[nodiscard]] std::size_t support_size() const noexcept {
    return support_size_;
  }

  [[nodiscard]] std::size_t affine_dimension() const noexcept {
    return affine_dimension_;
  }

  [[nodiscard]] const std::optional<ExactCenter3>& center() const noexcept {
    return center_;
  }

  [[nodiscard]] const std::optional<ExactLevel>& squared_level() const noexcept {
    return squared_level_;
  }

  friend bool operator==(
      const CircumcenterResult&, const CircumcenterResult&) noexcept = default;

 private:
  CircumcenterResult(
      CircumcenterKind kind,
      std::size_t support_size,
      std::size_t affine_dimension,
      std::optional<ExactCenter3> center,
      std::optional<ExactLevel> squared_level)
      : kind_(kind),
        support_size_(support_size),
        affine_dimension_(affine_dimension),
        center_(std::move(center)),
        squared_level_(std::move(squared_level)) {
    if (support_size_ < 1U || support_size_ > 4U) {
      throw std::invalid_argument(
          "a circumcenter support must contain between one and four points");
    }
    if (affine_dimension_ >= support_size_ || affine_dimension_ > 3U) {
      throw std::invalid_argument(
          "a circumcenter support has an invalid affine dimension");
    }
    const bool has_witnesses = center_.has_value() && squared_level_.has_value();
    if (kind_ == CircumcenterKind::unique &&
        (!has_witnesses || affine_dimension_ + 1U != support_size_)) {
      throw std::invalid_argument(
          "a unique circumcenter requires an affinely independent support and both witnesses");
    }
    if (kind_ == CircumcenterKind::affinely_dependent &&
        (center_.has_value() || squared_level_.has_value() ||
         affine_dimension_ + 1U >= support_size_)) {
      throw std::invalid_argument(
          "an affinely dependent support cannot expose circumcenter witnesses");
    }
  }

  CircumcenterKind kind_;
  std::size_t support_size_;
  std::size_t affine_dimension_;
  std::optional<ExactCenter3> center_;
  std::optional<ExactLevel> squared_level_;
};

namespace center_detail {

using ExactVector3 = std::array<ExactRational, 3>;

[[nodiscard]] inline ExactVector3 subtract(
    const ExactRational3& left, const ExactRational3& right) {
  ExactVector3 result{};
  for (std::size_t axis = 0; axis < result.size(); ++axis) {
    result[axis] = left.coordinate(axis) - right.coordinate(axis);
  }
  return result;
}

[[nodiscard]] inline ExactVector3 cross(
    const ExactVector3& left, const ExactVector3& right) {
  return {
      left[1] * right[2] - left[2] * right[1],
      left[2] * right[0] - left[0] * right[2],
      left[0] * right[1] - left[1] * right[0]};
}

[[nodiscard]] inline ExactRational dot(
    const ExactVector3& left, const ExactVector3& right) {
  ExactRational result;
  for (std::size_t axis = 0; axis < left.size(); ++axis) {
    result = result + left[axis] * right[axis];
  }
  return result;
}

[[nodiscard]] inline bool is_zero(const ExactVector3& vector) noexcept {
  return vector[0].is_zero() && vector[1].is_zero() && vector[2].is_zero();
}

[[nodiscard]] inline std::size_t affine_dimension(
    const ExactRational3& a, const ExactRational3& b) {
  return a == b ? 0U : 1U;
}

[[nodiscard]] inline std::size_t affine_dimension(
    const ExactRational3& a,
    const ExactRational3& b,
    const ExactRational3& c) {
  const ExactVector3 u = subtract(b, a);
  const ExactVector3 v = subtract(c, a);
  if (!is_zero(cross(u, v))) {
    return 2U;
  }
  return is_zero(u) && is_zero(v) ? 0U : 1U;
}

[[nodiscard]] inline std::size_t affine_dimension(
    const ExactRational3& a,
    const ExactRational3& b,
    const ExactRational3& c,
    const ExactRational3& d) {
  const ExactVector3 u = subtract(b, a);
  const ExactVector3 v = subtract(c, a);
  const ExactVector3 w = subtract(d, a);
  if (!dot(u, cross(v, w)).is_zero()) {
    return 3U;
  }
  if (!is_zero(cross(u, v)) || !is_zero(cross(u, w)) ||
      !is_zero(cross(v, w))) {
    return 2U;
  }
  return is_zero(u) && is_zero(v) && is_zero(w) ? 0U : 1U;
}

[[nodiscard]] inline ExactPlane3 equidistance_plane(
    const ExactRational3& left, const ExactRational3& right) {
  const ExactVector3 delta = subtract(right, left);
  if (is_zero(delta)) {
    throw std::invalid_argument(
        "two identical points do not define an equidistance plane");
  }
  ExactVector3 sum{};
  for (std::size_t axis = 0; axis < sum.size(); ++axis) {
    sum[axis] = left.coordinate(axis) + right.coordinate(axis);
  }
  const ExactRational two{BigInt{2}};
  return ExactPlane3::from_rational_coefficients({
      two * delta[0],
      two * delta[1],
      two * delta[2],
      -dot(delta, sum)});
}

[[nodiscard]] inline ExactLevel squared_distance(
    const ExactRational3& left, const ExactRational3& right) {
  ExactRational result;
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    const ExactRational delta = left.coordinate(axis) - right.coordinate(axis);
    result = result + delta * delta;
  }
  return ExactLevel{std::move(result)};
}

template <std::size_t SupportSize>
[[nodiscard]] inline CircumcenterResult verified_unique_result(
    ExactCenter3 center,
    const std::array<ExactRational3, SupportSize>& support) {
  static_assert(SupportSize >= 2U && SupportSize <= 4U);
  ExactLevel squared_level = squared_distance(center, support[0]);
  for (std::size_t index = 1U; index < support.size(); ++index) {
    if (squared_distance(center, support[index]) != squared_level) {
      throw std::logic_error(
          "an exact circumcenter candidate is not equidistant from its support");
    }
  }
  return CircumcenterResult::unique(
      SupportSize, std::move(center), std::move(squared_level));
}

[[nodiscard]] inline ExactCenter3 require_unique_intersection(
    const ExactPlane3& first,
    const ExactPlane3& second,
    const ExactPlane3& third) {
  const ThreePlaneIntersection intersection =
      intersect_three_planes(first, second, third);
  if (intersection.kind() != ThreePlaneIntersectionKind::unique ||
      !intersection.point().has_value()) {
    throw std::logic_error(
        "an affinely independent support produced a nonunique circumcenter");
  }
  return *intersection.point();
}

}  // namespace center_detail

[[nodiscard]] inline CircumcenterResult circumcenter(
    const ExactRational3& a) {
  return CircumcenterResult::unique(1U, a, ExactLevel{});
}

[[nodiscard]] inline CircumcenterResult circumcenter(
    const ExactRational3& a, const ExactRational3& b) {
  const std::size_t dimension = center_detail::affine_dimension(a, b);
  if (dimension != 1U) {
    return CircumcenterResult::affinely_dependent(2U, dimension);
  }
  std::array<ExactRational, 3> coordinates{};
  const ExactRational two{BigInt{2}};
  for (std::size_t axis = 0; axis < coordinates.size(); ++axis) {
    coordinates[axis] = (a.coordinate(axis) + b.coordinate(axis)) / two;
  }
  return center_detail::verified_unique_result(
      ExactCenter3{coordinates}, std::array<ExactRational3, 2>{a, b});
}

[[nodiscard]] inline CircumcenterResult circumcenter(
    const ExactRational3& a,
    const ExactRational3& b,
    const ExactRational3& c) {
  const std::size_t dimension = center_detail::affine_dimension(a, b, c);
  if (dimension != 2U) {
    return CircumcenterResult::affinely_dependent(3U, dimension);
  }
  const ExactCenter3 center = center_detail::require_unique_intersection(
      ExactPlane3::through_points(a, b, c),
      center_detail::equidistance_plane(a, b),
      center_detail::equidistance_plane(a, c));
  return center_detail::verified_unique_result(
      center, std::array<ExactRational3, 3>{a, b, c});
}

[[nodiscard]] inline CircumcenterResult circumcenter(
    const ExactRational3& a,
    const ExactRational3& b,
    const ExactRational3& c,
    const ExactRational3& d) {
  const std::size_t dimension = center_detail::affine_dimension(a, b, c, d);
  if (dimension != 3U) {
    return CircumcenterResult::affinely_dependent(4U, dimension);
  }
  const ExactCenter3 center = center_detail::require_unique_intersection(
      center_detail::equidistance_plane(a, b),
      center_detail::equidistance_plane(a, c),
      center_detail::equidistance_plane(a, d));
  return center_detail::verified_unique_result(
      center, std::array<ExactRational3, 4>{a, b, c, d});
}

[[nodiscard]] inline CircumcenterResult circumcenter(
    const CertifiedPoint3& a) {
  return circumcenter(a.exact());
}

[[nodiscard]] inline CircumcenterResult circumcenter(
    const CertifiedPoint3& a, const CertifiedPoint3& b) {
  return circumcenter(a.exact(), b.exact());
}

[[nodiscard]] inline CircumcenterResult circumcenter(
    const CertifiedPoint3& a,
    const CertifiedPoint3& b,
    const CertifiedPoint3& c) {
  return circumcenter(a.exact(), b.exact(), c.exact());
}

[[nodiscard]] inline CircumcenterResult circumcenter(
    const CertifiedPoint3& a,
    const CertifiedPoint3& b,
    const CertifiedPoint3& c,
    const CertifiedPoint3& d) {
  return circumcenter(a.exact(), b.exact(), c.exact(), d.exact());
}

}  // namespace morsehgp3d::exact
