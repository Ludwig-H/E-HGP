#pragma once

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/predicate.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace morsehgp3d::exact {

enum class ConvexHullLocation {
  relative_interior,
  relative_boundary,
  exterior,
};

[[nodiscard]] inline std::string_view to_string(ConvexHullLocation location) {
  switch (location) {
    case ConvexHullLocation::relative_interior:
      return "relative_interior";
    case ConvexHullLocation::relative_boundary:
      return "relative_boundary";
    case ConvexHullLocation::exterior:
      return "exterior";
  }
  throw std::invalid_argument("convex-hull location is invalid");
}

class BarycentricCoordinates;

template <std::size_t SupportSize>
[[nodiscard]] BarycentricCoordinates barycentric_coordinates(
    const ExactRational3& query,
    const std::array<ExactRational3, SupportSize>& support,
    PredicateCounters* counters = nullptr);

// Exact barycentric coordinates in an affinely independent support for a
// query in its affine hull. A query outside that affine hull is rejected
// instead of being projected or mislabeled as merely outside the convex hull.
// All signs are currently certified by the same CPU-multiprecision pass;
// later filters may refine that collective stage without changing witnesses.
class BarycentricCoordinates {
 private:
  [[nodiscard]] static BarycentricCoordinates certified(
      std::size_t support_size,
      std::array<ExactRational, 4> coordinates,
      PredicateCounters* counters = nullptr) {
    if (support_size < 1U || support_size > coordinates.size()) {
      throw std::invalid_argument(
          "barycentric coordinates require a support of size one to four");
    }
    ExactRational sum;
    std::array<PredicateSign, 4> signs{
        PredicateSign::zero,
        PredicateSign::zero,
        PredicateSign::zero,
        PredicateSign::zero};
    bool has_zero = false;
    bool has_negative = false;
    for (std::size_t index = 0; index < support_size; ++index) {
      sum = sum + coordinates[index];
      signs[index] = predicate_sign(coordinates[index].sign());
      has_zero = has_zero || signs[index] == PredicateSign::zero;
      has_negative = has_negative || signs[index] == PredicateSign::negative;
    }
    for (std::size_t index = support_size; index < coordinates.size(); ++index) {
      if (!coordinates[index].is_zero()) {
        throw std::invalid_argument(
            "unused barycentric coordinate slots must be exact zeroes");
      }
    }
    if (sum != ExactRational{BigInt{1}}) {
      throw std::invalid_argument("barycentric coordinates must sum exactly to one");
    }
    const ConvexHullLocation location =
        has_negative
            ? ConvexHullLocation::exterior
            : (has_zero ? ConvexHullLocation::relative_boundary
                        : ConvexHullLocation::relative_interior);
    if (counters != nullptr) {
      for (std::size_t index = 0; index < support_size; ++index) {
        counters->record_certification(PredicateDecision{
            signs[index], CertificationStage::cpu_multiprecision});
      }
    }
    return BarycentricCoordinates{
        support_size, std::move(coordinates), signs, location};
  }

  template <std::size_t SupportSize>
  friend BarycentricCoordinates barycentric_coordinates(
      const ExactRational3& query,
      const std::array<ExactRational3, SupportSize>& support,
      PredicateCounters* counters);

 public:

  [[nodiscard]] std::size_t support_size() const noexcept {
    return support_size_;
  }

  [[nodiscard]] const ExactRational& coordinate(std::size_t index) const {
    require_index(index);
    return coordinates_[index];
  }

  [[nodiscard]] PredicateSign sign(std::size_t index) const {
    require_index(index);
    return signs_[index];
  }

  [[nodiscard]] static constexpr CertificationStage certification_stage() noexcept {
    return CertificationStage::cpu_multiprecision;
  }

  [[nodiscard]] ConvexHullLocation location() const noexcept {
    return location_;
  }

  friend bool operator==(
      const BarycentricCoordinates&,
      const BarycentricCoordinates&) noexcept = default;

 private:
  BarycentricCoordinates(
      std::size_t support_size,
      std::array<ExactRational, 4> coordinates,
      std::array<PredicateSign, 4> signs,
      ConvexHullLocation location)
      : support_size_(support_size),
        coordinates_(std::move(coordinates)),
        signs_(signs),
        location_(location) {}

  void require_index(std::size_t index) const {
    if (index >= support_size_) {
      throw std::out_of_range("barycentric coordinate index is outside its support");
    }
  }

  std::size_t support_size_;
  std::array<ExactRational, 4> coordinates_{};
  std::array<PredicateSign, 4> signs_{};
  ConvexHullLocation location_;
};

namespace support_detail {

using ExactVector3 = std::array<ExactRational, 3>;

[[nodiscard]] inline ExactVector3 difference(
    const ExactRational3& left, const ExactRational3& right) {
  ExactVector3 result{};
  for (std::size_t axis = 0; axis < result.size(); ++axis) {
    result[axis] = left.coordinate(axis) - right.coordinate(axis);
  }
  return result;
}

[[nodiscard]] inline ExactRational dot_product(
    const ExactVector3& left, const ExactVector3& right) {
  ExactRational result;
  for (std::size_t axis = 0; axis < left.size(); ++axis) {
    result = result + left[axis] * right[axis];
  }
  return result;
}

[[nodiscard]] inline ExactVector3 cross_product(
    const ExactVector3& left, const ExactVector3& right) {
  return {
      left[1] * right[2] - left[2] * right[1],
      left[2] * right[0] - left[0] * right[2],
      left[0] * right[1] - left[1] * right[0]};
}

[[nodiscard]] inline ExactRational triple_product(
    const ExactVector3& first,
    const ExactVector3& second,
    const ExactVector3& third) {
  return dot_product(first, cross_product(second, third));
}

template <std::size_t SupportSize>
inline void require_affine_reconstruction(
    const ExactRational3& query,
    const std::array<ExactRational3, SupportSize>& support,
    const std::array<ExactRational, 4>& coordinates) {
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    ExactRational reconstructed;
    for (std::size_t index = 0; index < support.size(); ++index) {
      reconstructed = reconstructed +
                      coordinates[index] * support[index].coordinate(axis);
    }
    if (reconstructed != query.coordinate(axis)) {
      throw std::invalid_argument(
          "the query point is outside the support affine hull");
    }
  }
}

template <std::size_t SupportSize>
[[nodiscard]] inline CircumcenterResult circumcenter_of(
    const std::array<ExactRational3, SupportSize>& support) {
  static_assert(SupportSize >= 1U && SupportSize <= 4U);
  if constexpr (SupportSize == 1U) {
    return circumcenter(support[0]);
  } else if constexpr (SupportSize == 2U) {
    return circumcenter(support[0], support[1]);
  } else if constexpr (SupportSize == 3U) {
    return circumcenter(support[0], support[1], support[2]);
  } else {
    return circumcenter(support[0], support[1], support[2], support[3]);
  }
}

template <std::size_t SupportSize>
[[nodiscard]] inline std::array<ExactRational3, SupportSize> exact_support(
    const std::array<CertifiedPoint3, SupportSize>& support) {
  std::array<ExactRational3, SupportSize> result{};
  for (std::size_t index = 0; index < support.size(); ++index) {
    result[index] = support[index].exact();
  }
  return result;
}

}  // namespace support_detail

template <std::size_t SupportSize>
[[nodiscard]] BarycentricCoordinates barycentric_coordinates(
    const ExactRational3& query,
    const std::array<ExactRational3, SupportSize>& support,
    PredicateCounters* counters) {
  static_assert(SupportSize >= 1U && SupportSize <= 4U);
  std::array<ExactRational, 4> coordinates{};
  if constexpr (SupportSize == 1U) {
    if (query != support[0]) {
      throw std::invalid_argument(
          "a singleton affine hull contains only its support point");
    }
    coordinates[0] = ExactRational{BigInt{1}};
  } else if constexpr (SupportSize == 2U) {
    if (center_detail::affine_dimension(support[0], support[1]) != 1U) {
      throw std::invalid_argument(
          "barycentric coordinates require an affinely independent pair");
    }
    const auto direction = support_detail::difference(support[1], support[0]);
    const auto query_delta = support_detail::difference(query, support[0]);
    const ExactRational parameter =
        support_detail::dot_product(query_delta, direction) /
        support_detail::dot_product(direction, direction);
    coordinates[0] = ExactRational{BigInt{1}} - parameter;
    coordinates[1] = parameter;
  } else if constexpr (SupportSize == 3U) {
    if (center_detail::affine_dimension(
            support[0], support[1], support[2]) != 2U) {
      throw std::invalid_argument(
          "barycentric coordinates require an affinely independent triangle");
    }
    const auto first = support_detail::difference(support[1], support[0]);
    const auto second = support_detail::difference(support[2], support[0]);
    const auto query_delta = support_detail::difference(query, support[0]);
    const ExactRational first_first =
        support_detail::dot_product(first, first);
    const ExactRational first_second =
        support_detail::dot_product(first, second);
    const ExactRational second_second =
        support_detail::dot_product(second, second);
    const ExactRational first_query =
        support_detail::dot_product(first, query_delta);
    const ExactRational second_query =
        support_detail::dot_product(second, query_delta);
    const ExactRational determinant =
        first_first * second_second - first_second * first_second;
    coordinates[1] =
        (first_query * second_second - second_query * first_second) /
        determinant;
    coordinates[2] =
        (first_first * second_query - first_second * first_query) /
        determinant;
    coordinates[0] = ExactRational{BigInt{1}} - coordinates[1] - coordinates[2];
  } else {
    if (center_detail::affine_dimension(
            support[0], support[1], support[2], support[3]) != 3U) {
      throw std::invalid_argument(
          "barycentric coordinates require an affinely independent tetrahedron");
    }
    const auto first = support_detail::difference(support[1], support[0]);
    const auto second = support_detail::difference(support[2], support[0]);
    const auto third = support_detail::difference(support[3], support[0]);
    const auto query_delta = support_detail::difference(query, support[0]);
    const ExactRational determinant =
        support_detail::triple_product(first, second, third);
    coordinates[1] =
        support_detail::triple_product(query_delta, second, third) /
        determinant;
    coordinates[2] =
        support_detail::triple_product(first, query_delta, third) /
        determinant;
    coordinates[3] =
        support_detail::triple_product(first, second, query_delta) /
        determinant;
    coordinates[0] = ExactRational{BigInt{1}} - coordinates[1] -
                     coordinates[2] - coordinates[3];
  }
  support_detail::require_affine_reconstruction(query, support, coordinates);
  return BarycentricCoordinates::certified(
      SupportSize, std::move(coordinates), counters);
}

template <std::size_t SupportSize>
[[nodiscard]] BarycentricCoordinates barycentric_coordinates(
    const CertifiedPoint3& query,
    const std::array<CertifiedPoint3, SupportSize>& support,
    PredicateCounters* counters = nullptr) {
  return barycentric_coordinates(
      query.exact(), support_detail::exact_support(support), counters);
}

enum class CircumcenterSupportStatus {
  minimal,
  boundary_reduced,
  exterior_circumcenter,
  affinely_dependent,
};

[[nodiscard]] inline std::string_view to_string(
    CircumcenterSupportStatus status) {
  switch (status) {
    case CircumcenterSupportStatus::minimal:
      return "minimal";
    case CircumcenterSupportStatus::boundary_reduced:
      return "boundary_reduced";
    case CircumcenterSupportStatus::exterior_circumcenter:
      return "exterior_circumcenter";
    case CircumcenterSupportStatus::affinely_dependent:
      return "affinely_dependent";
  }
  throw std::invalid_argument("circumcenter support status is invalid");
}

class CircumcenterSupportAnalysis;

template <std::size_t SupportSize>
[[nodiscard]] CircumcenterSupportAnalysis analyze_circumcenter_support(
    const std::array<ExactRational3, SupportSize>& support,
    PredicateCounters* counters = nullptr);

class CircumcenterSupportAnalysis {
 private:
  [[nodiscard]] static CircumcenterSupportAnalysis dependent(
      CircumcenterResult circumcenter_result) {
    if (circumcenter_result.kind() != CircumcenterKind::affinely_dependent) {
      throw std::invalid_argument(
          "a dependent support analysis requires a dependent circumcenter result");
    }
    return CircumcenterSupportAnalysis{
        std::move(circumcenter_result),
        std::nullopt,
        CircumcenterSupportStatus::affinely_dependent,
        std::nullopt};
  }

  [[nodiscard]] static CircumcenterSupportAnalysis independent(
      CircumcenterResult circumcenter_result,
      BarycentricCoordinates barycentric) {
    if (circumcenter_result.kind() != CircumcenterKind::unique ||
        circumcenter_result.support_size() != barycentric.support_size()) {
      throw std::invalid_argument(
          "an independent support analysis requires matching exact witnesses");
    }
    CircumcenterSupportStatus status =
        CircumcenterSupportStatus::exterior_circumcenter;
    std::optional<std::uint8_t> reduced_mask;
    if (barycentric.location() == ConvexHullLocation::relative_interior) {
      status = CircumcenterSupportStatus::minimal;
      std::uint8_t mask = 0U;
      for (std::size_t index = 0; index < barycentric.support_size(); ++index) {
        mask = static_cast<std::uint8_t>(
            mask | static_cast<std::uint8_t>(1U << index));
      }
      reduced_mask = mask;
    } else if (
        barycentric.location() == ConvexHullLocation::relative_boundary) {
      status = CircumcenterSupportStatus::boundary_reduced;
      std::uint8_t mask = 0U;
      for (std::size_t index = 0; index < barycentric.support_size(); ++index) {
        if (barycentric.sign(index) == PredicateSign::positive) {
          mask = static_cast<std::uint8_t>(
              mask | static_cast<std::uint8_t>(1U << index));
        }
      }
      if (mask == 0U) {
        throw std::logic_error(
            "a boundary barycentric witness has no positive coordinate");
      }
      reduced_mask = mask;
    }
    return CircumcenterSupportAnalysis{
        std::move(circumcenter_result),
        std::move(barycentric),
        status,
        reduced_mask};
  }

  template <std::size_t SupportSize>
  friend CircumcenterSupportAnalysis analyze_circumcenter_support(
      const std::array<ExactRational3, SupportSize>& support,
      PredicateCounters* counters);

 public:

  [[nodiscard]] const CircumcenterResult& circumcenter_result() const noexcept {
    return circumcenter_result_;
  }

  [[nodiscard]] const std::optional<BarycentricCoordinates>& barycentric()
      const noexcept {
    return barycentric_;
  }

  [[nodiscard]] CircumcenterSupportStatus status() const noexcept {
    return status_;
  }

  [[nodiscard]] const std::optional<std::uint8_t>& reduced_support_mask()
      const noexcept {
    return reduced_support_mask_;
  }

  [[nodiscard]] std::optional<std::size_t> reduced_support_size() const noexcept {
    if (!reduced_support_mask_.has_value()) {
      return std::nullopt;
    }
    std::size_t count = 0U;
    for (std::size_t index = 0; index < circumcenter_result_.support_size(); ++index) {
      const auto bit = static_cast<std::uint8_t>(1U << index);
      if ((*reduced_support_mask_ & bit) != 0U) {
        ++count;
      }
    }
    return count;
  }

  [[nodiscard]] bool reduced_support_contains(std::size_t index) const {
    if (index >= circumcenter_result_.support_size()) {
      throw std::out_of_range("reduced support index is outside the input support");
    }
    return reduced_support_mask_.has_value() &&
           ((*reduced_support_mask_ & static_cast<std::uint8_t>(1U << index)) !=
            0U);
  }

  friend bool operator==(
      const CircumcenterSupportAnalysis&,
      const CircumcenterSupportAnalysis&) noexcept = default;

 private:
  CircumcenterSupportAnalysis(
      CircumcenterResult circumcenter_result,
      std::optional<BarycentricCoordinates> barycentric,
      CircumcenterSupportStatus status,
      std::optional<std::uint8_t> reduced_support_mask)
      : circumcenter_result_(std::move(circumcenter_result)),
        barycentric_(std::move(barycentric)),
        status_(status),
        reduced_support_mask_(reduced_support_mask) {}

  CircumcenterResult circumcenter_result_;
  std::optional<BarycentricCoordinates> barycentric_;
  CircumcenterSupportStatus status_;
  std::optional<std::uint8_t> reduced_support_mask_;
};

namespace support_detail {

template <std::size_t SupportSize>
inline void verify_boundary_reduction(
    const CircumcenterSupportAnalysis& analysis,
    const std::array<ExactRational3, SupportSize>& support) {
  if (analysis.status() != CircumcenterSupportStatus::boundary_reduced) {
    return;
  }
  std::array<ExactRational3, 4> reduced{};
  std::size_t reduced_size = 0U;
  for (std::size_t index = 0; index < support.size(); ++index) {
    if (analysis.reduced_support_contains(index)) {
      reduced[reduced_size] = support[index];
      ++reduced_size;
    }
  }
  CircumcenterResult reduced_center = [&reduced, reduced_size] {
    if (reduced_size == 1U) {
      return circumcenter(reduced[0]);
    }
    if (reduced_size == 2U) {
      return circumcenter(reduced[0], reduced[1]);
    }
    if (reduced_size == 3U) {
      return circumcenter(reduced[0], reduced[1], reduced[2]);
    }
    throw std::logic_error(
        "a boundary reduction must remove at least one support point");
  }();
  const CircumcenterResult& original = analysis.circumcenter_result();
  if (reduced_center.kind() != CircumcenterKind::unique ||
      !reduced_center.center().has_value() ||
      !reduced_center.squared_level().has_value() ||
      reduced_center.center() != original.center() ||
      reduced_center.squared_level() != original.squared_level()) {
    throw std::logic_error(
        "a barycentric boundary reduction changed the exact sphere");
  }
  const ExactRational3& reduced_center_point = *reduced_center.center();
  BarycentricCoordinates reduced_barycentric =
      [&reduced, reduced_size, &reduced_center_point] {
        if (reduced_size == 1U) {
          return barycentric_coordinates(
              reduced_center_point,
              std::array<ExactRational3, 1>{reduced[0]});
        }
        if (reduced_size == 2U) {
          return barycentric_coordinates(
              reduced_center_point,
              std::array<ExactRational3, 2>{reduced[0], reduced[1]});
        }
        return barycentric_coordinates(
            reduced_center_point,
            std::array<ExactRational3, 3>{
                reduced[0], reduced[1], reduced[2]});
      }();
  if (reduced_barycentric.location() !=
      ConvexHullLocation::relative_interior) {
    throw std::logic_error(
        "a reduced boundary support is not relatively interior");
  }
}

}  // namespace support_detail

template <std::size_t SupportSize>
[[nodiscard]] CircumcenterSupportAnalysis analyze_circumcenter_support(
    const std::array<ExactRational3, SupportSize>& support,
    PredicateCounters* counters) {
  static_assert(SupportSize >= 1U && SupportSize <= 4U);
  CircumcenterResult center = support_detail::circumcenter_of(support);
  if (center.kind() == CircumcenterKind::affinely_dependent) {
    return CircumcenterSupportAnalysis::dependent(std::move(center));
  }
  if (!center.center().has_value()) {
    throw std::logic_error("a unique circumcenter omitted its center witness");
  }
  BarycentricCoordinates barycentric =
      barycentric_coordinates(*center.center(), support, counters);
  CircumcenterSupportAnalysis analysis =
      CircumcenterSupportAnalysis::independent(
          std::move(center), std::move(barycentric));
  support_detail::verify_boundary_reduction(analysis, support);
  return analysis;
}

template <std::size_t SupportSize>
[[nodiscard]] CircumcenterSupportAnalysis analyze_circumcenter_support(
    const std::array<CertifiedPoint3, SupportSize>& support,
    PredicateCounters* counters = nullptr) {
  return analyze_circumcenter_support(
      support_detail::exact_support(support), counters);
}

enum class SpherePointLocation {
  strictly_inside,
  boundary,
  outside,
};

[[nodiscard]] inline std::string_view to_string(SpherePointLocation location) {
  switch (location) {
    case SpherePointLocation::strictly_inside:
      return "strictly_inside";
    case SpherePointLocation::boundary:
      return "boundary";
    case SpherePointLocation::outside:
      return "outside";
  }
  throw std::invalid_argument("sphere point location is invalid");
}

class SpherePointClassification {
 public:
  [[nodiscard]] static SpherePointClassification certified(
      ExactLevel point_squared_distance,
      const ExactLevel& sphere_squared_level,
      PredicateCounters* counters = nullptr) {
    ExactRational signed_power =
        point_squared_distance.rational() - sphere_squared_level.rational();
    const PredicateDecision decision{
        predicate_sign(signed_power.sign()),
        CertificationStage::cpu_multiprecision};
    if (counters != nullptr) {
      counters->record_certification(decision);
    }
    const SpherePointLocation location =
        decision.sign() == PredicateSign::negative
            ? SpherePointLocation::strictly_inside
            : (decision.sign() == PredicateSign::zero
                   ? SpherePointLocation::boundary
                   : SpherePointLocation::outside);
    return SpherePointClassification{
        decision,
        std::move(point_squared_distance),
        std::move(signed_power),
        location};
  }

  [[nodiscard]] const PredicateDecision& decision() const noexcept {
    return decision_;
  }

  [[nodiscard]] const ExactLevel& point_squared_distance() const noexcept {
    return point_squared_distance_;
  }

  [[nodiscard]] const ExactRational& signed_power() const noexcept {
    return signed_power_;
  }

  [[nodiscard]] SpherePointLocation location() const noexcept {
    return location_;
  }

  friend bool operator==(
      const SpherePointClassification& left,
      const SpherePointClassification& right) noexcept {
    return left.decision_.sign() == right.decision_.sign() &&
           left.decision_.certification_stage() ==
               right.decision_.certification_stage() &&
           left.point_squared_distance_ == right.point_squared_distance_ &&
           left.signed_power_ == right.signed_power_ &&
           left.location_ == right.location_;
  }

 private:
  SpherePointClassification(
      PredicateDecision decision,
      ExactLevel point_squared_distance,
      ExactRational signed_power,
      SpherePointLocation location)
      : decision_(decision),
        point_squared_distance_(std::move(point_squared_distance)),
        signed_power_(std::move(signed_power)),
        location_(location) {}

  PredicateDecision decision_;
  ExactLevel point_squared_distance_;
  ExactRational signed_power_;
  SpherePointLocation location_;
};

[[nodiscard]] inline SpherePointClassification classify_sphere_point(
    const ExactRational3& center,
    const ExactLevel& squared_level,
    const ExactRational3& point,
    PredicateCounters* counters = nullptr) {
  ExactRational squared_distance;
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    const ExactRational delta =
        point.coordinate(axis) - center.coordinate(axis);
    squared_distance = squared_distance + delta * delta;
  }
  return SpherePointClassification::certified(
      ExactLevel{std::move(squared_distance)}, squared_level, counters);
}

[[nodiscard]] inline SpherePointClassification classify_sphere_point(
    const ExactRational3& center,
    const ExactLevel& squared_level,
    const CertifiedPoint3& point,
    PredicateCounters* counters = nullptr) {
  return classify_sphere_point(center, squared_level, point.exact(), counters);
}

[[nodiscard]] inline SpherePointClassification classify_sphere_point(
    const CircumcenterResult& sphere,
    const ExactRational3& point,
    PredicateCounters* counters = nullptr) {
  if (sphere.kind() != CircumcenterKind::unique ||
      !sphere.center().has_value() || !sphere.squared_level().has_value()) {
    throw std::invalid_argument(
        "sphere-point classification requires a unique circumcenter sphere");
  }
  return classify_sphere_point(
      *sphere.center(), *sphere.squared_level(), point, counters);
}

[[nodiscard]] inline SpherePointClassification classify_sphere_point(
    const CircumcenterResult& sphere,
    const CertifiedPoint3& point,
    PredicateCounters* counters = nullptr) {
  return classify_sphere_point(sphere, point.exact(), counters);
}

}  // namespace morsehgp3d::exact
