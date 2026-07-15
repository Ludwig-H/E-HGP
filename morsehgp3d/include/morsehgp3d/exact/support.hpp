#pragma once

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/predicate.hpp"
#include "morsehgp3d/exact/support_polynomial.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

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
class CircumcenterSupportAnalysis;

template <std::size_t SupportSize>
[[nodiscard]] CircumcenterSupportAnalysis analyze_circumcenter_support(
    const std::array<CertifiedPoint3, SupportSize>& support,
    PredicateCounters* counters = nullptr,
    PredicateFilterPolicy filter_policy = PredicateFilterPolicy::allow_adaptive);

template <std::size_t SupportSize>
[[nodiscard]] BarycentricCoordinates barycentric_coordinates(
    const ExactRational3& query,
    const std::array<ExactRational3, SupportSize>& support,
    PredicateCounters* counters = nullptr);

template <std::size_t SupportSize>
[[nodiscard]] BarycentricCoordinates barycentric_coordinates(
    const CertifiedPoint3& query,
    const std::array<CertifiedPoint3, SupportSize>& support,
    PredicateCounters* counters = nullptr,
    PredicateFilterPolicy filter_policy = PredicateFilterPolicy::allow_adaptive);

// Exact barycentric coordinates in an affinely independent support for a
// query in its affine hull. A query outside that affine hull is rejected
// instead of being projected or mislabeled as merely outside the convex hull.
// All signs are certified by one collective pass. If one coordinate remains
// uncertain, the complete vector falls through to the next stage so its
// location witness never mixes authorities.
class BarycentricCoordinates {
 private:
  [[nodiscard]] static BarycentricCoordinates certified(
      std::size_t support_size,
      std::array<ExactRational, 4> coordinates,
      CertificationStage certification_stage,
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
    for (std::size_t index = 0; index < support_size; ++index) {
      static_cast<void>(PredicateDecision{signs[index], certification_stage});
    }
    const ConvexHullLocation location =
        has_negative
            ? ConvexHullLocation::exterior
            : (has_zero ? ConvexHullLocation::relative_boundary
                        : ConvexHullLocation::relative_interior);
    if (counters != nullptr) {
      for (std::size_t index = 0; index < support_size; ++index) {
        counters->record_certification(PredicateDecision{
            signs[index], certification_stage});
      }
    }
    return BarycentricCoordinates{
        support_size,
        std::move(coordinates),
        signs,
        certification_stage,
        location};
  }

  template <std::size_t SupportSize>
  friend BarycentricCoordinates barycentric_coordinates(
      const ExactRational3& query,
      const std::array<ExactRational3, SupportSize>& support,
      PredicateCounters* counters);

  template <std::size_t SupportSize>
  friend BarycentricCoordinates barycentric_coordinates(
      const CertifiedPoint3& query,
      const std::array<CertifiedPoint3, SupportSize>& support,
      PredicateCounters* counters,
      PredicateFilterPolicy filter_policy);

  template <std::size_t SupportSize>
  friend CircumcenterSupportAnalysis analyze_circumcenter_support(
      const std::array<CertifiedPoint3, SupportSize>& support,
      PredicateCounters* counters,
      PredicateFilterPolicy filter_policy);

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

  [[nodiscard]] CertificationStage certification_stage() const noexcept {
    return certification_stage_;
  }

  [[nodiscard]] ConvexHullLocation location() const noexcept {
    return location_;
  }

  friend bool operator==(
      const BarycentricCoordinates& left,
      const BarycentricCoordinates& right) noexcept {
    return left.support_size_ == right.support_size_ &&
           left.coordinates_ == right.coordinates_ &&
           left.signs_ == right.signs_ && left.location_ == right.location_;
  }

 private:
  BarycentricCoordinates(
      std::size_t support_size,
      std::array<ExactRational, 4> coordinates,
      std::array<PredicateSign, 4> signs,
      CertificationStage certification_stage,
      ConvexHullLocation location)
      : support_size_(support_size),
        coordinates_(std::move(coordinates)),
        signs_(signs),
        certification_stage_(certification_stage),
        location_(location) {}

  void require_index(std::size_t index) const {
    if (index >= support_size_) {
      throw std::out_of_range("barycentric coordinate index is outside its support");
    }
  }

  std::size_t support_size_;
  std::array<ExactRational, 4> coordinates_{};
  std::array<PredicateSign, 4> signs_{};
  CertificationStage certification_stage_;
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

template <std::size_t SupportSize>
[[nodiscard]] inline bool verify_collective_attempt(
    const detail::support_polynomial::SignVectorAttempt<SupportSize>& attempt,
    const detail::support_polynomial::CanonicalSupport<SupportSize>& support,
    const std::array<PredicateSign, SupportSize>& exact_signs,
    const char* contradiction_message) {
  if (attempt.state() != FilterState::certified) {
    return false;
  }
  if (!attempt.signs().has_value()) {
    throw std::runtime_error(
        "a certified barycentric attempt omitted its sign vector");
  }
  for (std::size_t canonical_index = 0;
       canonical_index < SupportSize;
       ++canonical_index) {
    const std::size_t original_index =
        support.original_indices[canonical_index];
    if ((*attempt.signs())[canonical_index] != exact_signs[original_index]) {
      throw std::runtime_error(contradiction_message);
    }
  }
  return true;
}

template <std::size_t SupportSize>
[[nodiscard]] inline CertificationStage query_barycentric_stage(
    const CertifiedPoint3& query,
    const std::array<CertifiedPoint3, SupportSize>& support,
    const std::array<PredicateSign, SupportSize>& exact_signs,
    PredicateFilterPolicy filter_policy) {
  const auto canonical_support =
      detail::support_polynomial::canonicalize_support(support);
  if (detail::policy_allows_fp64(filter_policy) &&
      verify_collective_attempt(
          detail::support_polynomial::filter_query_barycentric_signs(
              query, canonical_support),
          canonical_support,
          exact_signs,
          "the FP64 barycentric filter contradicted its exact witness")) {
    return CertificationStage::fp64_filtered;
  }
  if (detail::policy_allows_expansion(filter_policy) &&
      verify_collective_attempt(
          detail::support_polynomial::expansion_query_barycentric_signs(
              query, canonical_support),
          canonical_support,
          exact_signs,
          "the barycentric expansion contradicted its exact witness")) {
    return CertificationStage::expansion;
  }
  return CertificationStage::cpu_multiprecision;
}

template <std::size_t SupportSize>
[[nodiscard]] inline CertificationStage circumcenter_barycentric_stage(
    const std::array<CertifiedPoint3, SupportSize>& support,
    const std::array<PredicateSign, SupportSize>& exact_signs,
    PredicateFilterPolicy filter_policy) {
  const auto canonical_support =
      detail::support_polynomial::canonicalize_support(support);
  if (detail::policy_allows_fp64(filter_policy) &&
      verify_collective_attempt(
          detail::support_polynomial::filter_circumcenter_barycentric_signs(
              canonical_support),
          canonical_support,
          exact_signs,
          "the FP64 circumcenter-barycentric filter contradicted its exact witness")) {
    return CertificationStage::fp64_filtered;
  }
  if (detail::policy_allows_expansion(filter_policy) &&
      verify_collective_attempt(
          detail::support_polynomial::expansion_circumcenter_barycentric_signs(
              canonical_support),
          canonical_support,
          exact_signs,
          "the circumcenter-barycentric expansion contradicted its exact witness")) {
    return CertificationStage::expansion;
  }
  return CertificationStage::cpu_multiprecision;
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
      SupportSize,
      std::move(coordinates),
      CertificationStage::cpu_multiprecision,
      counters);
}

template <std::size_t SupportSize>
[[nodiscard]] BarycentricCoordinates barycentric_coordinates(
    const CertifiedPoint3& query,
    const std::array<CertifiedPoint3, SupportSize>& support,
    PredicateCounters* counters,
    PredicateFilterPolicy filter_policy) {
  detail::require_filter_policy(filter_policy);
  const std::array<ExactRational3, SupportSize> exact_support =
      support_detail::exact_support(support);
  const BarycentricCoordinates exact_coordinates = barycentric_coordinates(
      query.exact(), exact_support, nullptr);
  std::array<ExactRational, 4> coordinates{};
  std::array<PredicateSign, SupportSize> exact_signs{};
  for (std::size_t index = 0; index < SupportSize; ++index) {
    coordinates[index] = exact_coordinates.coordinate(index);
    exact_signs[index] = exact_coordinates.sign(index);
  }
  const CertificationStage certification_stage =
      support_detail::query_barycentric_stage(
          query, support, exact_signs, filter_policy);
  return BarycentricCoordinates::certified(
      SupportSize,
      std::move(coordinates),
      certification_stage,
      counters);
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
      CircumcenterResult circumcenter_result,
      std::vector<CertifiedPoint3> binary64_support = {}) {
    if (circumcenter_result.kind() != CircumcenterKind::affinely_dependent) {
      throw std::invalid_argument(
          "a dependent support analysis requires a dependent circumcenter result");
    }
    return CircumcenterSupportAnalysis{
        std::move(circumcenter_result),
        std::nullopt,
        CircumcenterSupportStatus::affinely_dependent,
        std::nullopt,
        std::move(binary64_support)};
  }

  [[nodiscard]] static CircumcenterSupportAnalysis independent(
      CircumcenterResult circumcenter_result,
      BarycentricCoordinates barycentric,
      std::vector<CertifiedPoint3> binary64_support = {}) {
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
        reduced_mask,
        std::move(binary64_support)};
  }

  template <std::size_t SupportSize>
  friend CircumcenterSupportAnalysis analyze_circumcenter_support(
      const std::array<ExactRational3, SupportSize>& support,
      PredicateCounters* counters);

  template <std::size_t SupportSize>
  friend CircumcenterSupportAnalysis analyze_circumcenter_support(
      const std::array<CertifiedPoint3, SupportSize>& support,
      PredicateCounters* counters,
      PredicateFilterPolicy filter_policy);

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

  // Positional binary64 provenance. It is empty for arbitrary rational input
  // and deliberately excluded from the scientific equality of an analysis.
  [[nodiscard]] std::span<const CertifiedPoint3> binary64_support()
      const noexcept {
    return binary64_support_;
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
      const CircumcenterSupportAnalysis& left,
      const CircumcenterSupportAnalysis& right) noexcept {
    return left.circumcenter_result_ == right.circumcenter_result_ &&
           left.barycentric_ == right.barycentric_ &&
           left.status_ == right.status_ &&
           left.reduced_support_mask_ == right.reduced_support_mask_;
  }

 private:
  CircumcenterSupportAnalysis(
      CircumcenterResult circumcenter_result,
      std::optional<BarycentricCoordinates> barycentric,
      CircumcenterSupportStatus status,
      std::optional<std::uint8_t> reduced_support_mask,
      std::vector<CertifiedPoint3> binary64_support)
      : circumcenter_result_(std::move(circumcenter_result)),
        barycentric_(std::move(barycentric)),
        status_(status),
        reduced_support_mask_(reduced_support_mask),
        binary64_support_(std::move(binary64_support)) {
    if (!binary64_support_.empty() &&
        binary64_support_.size() != circumcenter_result_.support_size()) {
      throw std::invalid_argument(
          "binary64 support provenance must match the analyzed support");
    }
  }

  CircumcenterResult circumcenter_result_;
  std::optional<BarycentricCoordinates> barycentric_;
  CircumcenterSupportStatus status_;
  std::optional<std::uint8_t> reduced_support_mask_;
  std::vector<CertifiedPoint3> binary64_support_;
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
    PredicateCounters* counters,
    PredicateFilterPolicy filter_policy) {
  detail::require_filter_policy(filter_policy);
  const std::array<ExactRational3, SupportSize> exact_support =
      support_detail::exact_support(support);
  CircumcenterResult center = support_detail::circumcenter_of(exact_support);
  std::vector<CertifiedPoint3> binary64_support{
      support.begin(), support.end()};
  if (center.kind() == CircumcenterKind::affinely_dependent) {
    return CircumcenterSupportAnalysis::dependent(
        std::move(center), std::move(binary64_support));
  }
  if (!center.center().has_value()) {
    throw std::logic_error("a unique circumcenter omitted its center witness");
  }
  const BarycentricCoordinates exact_barycentric =
      barycentric_coordinates(*center.center(), exact_support, nullptr);
  std::array<ExactRational, 4> coordinates{};
  std::array<PredicateSign, SupportSize> exact_signs{};
  for (std::size_t index = 0; index < SupportSize; ++index) {
    coordinates[index] = exact_barycentric.coordinate(index);
    exact_signs[index] = exact_barycentric.sign(index);
  }
  const CertificationStage certification_stage =
      support_detail::circumcenter_barycentric_stage(
          support, exact_signs, filter_policy);
  BarycentricCoordinates barycentric = BarycentricCoordinates::certified(
      SupportSize,
      std::move(coordinates),
      certification_stage,
      counters);
  CircumcenterSupportAnalysis analysis =
      CircumcenterSupportAnalysis::independent(
          std::move(center),
          std::move(barycentric),
          std::move(binary64_support));
  support_detail::verify_boundary_reduction(analysis, exact_support);
  return analysis;
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

class SpherePointClassification;

// Classifies against the circumsphere of this affinely independent support.
// This low-level overload does not assert that the support is a minimal
// enclosing-ball support for any ambient point set.
template <std::size_t SupportSize>
[[nodiscard]] SpherePointClassification classify_sphere_point(
    const std::array<CertifiedPoint3, SupportSize>& support,
    const CertifiedPoint3& point,
    PredicateCounters* counters = nullptr,
    PredicateFilterPolicy filter_policy = PredicateFilterPolicy::allow_adaptive);

class SpherePointClassification {
 public:
  [[nodiscard]] static SpherePointClassification certified(
      ExactLevel point_squared_distance,
      const ExactLevel& sphere_squared_level,
      PredicateCounters* counters = nullptr) {
    return certified_at_stage(
        std::move(point_squared_distance),
        sphere_squared_level,
        CertificationStage::cpu_multiprecision,
        counters);
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
  template <std::size_t SupportSize>
  friend SpherePointClassification classify_sphere_point(
      const std::array<CertifiedPoint3, SupportSize>& support,
      const CertifiedPoint3& point,
      PredicateCounters* counters,
      PredicateFilterPolicy filter_policy);

  [[nodiscard]] static SpherePointClassification certified_at_stage(
      ExactLevel point_squared_distance,
      const ExactLevel& sphere_squared_level,
      CertificationStage certification_stage,
      PredicateCounters* counters) {
    ExactRational signed_power =
        point_squared_distance.rational() - sphere_squared_level.rational();
    const PredicateDecision decision{
        predicate_sign(signed_power.sign()),
        certification_stage};
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

template <std::size_t SupportSize>
[[nodiscard]] SpherePointClassification classify_sphere_point(
    const std::array<CertifiedPoint3, SupportSize>& support,
    const CertifiedPoint3& point,
    PredicateCounters* counters,
    PredicateFilterPolicy filter_policy) {
  static_assert(SupportSize >= 1U && SupportSize <= 4U);
  detail::require_filter_policy(filter_policy);
  const std::array<ExactRational3, SupportSize> exact_support =
      support_detail::exact_support(support);
  const CircumcenterResult sphere = support_detail::circumcenter_of(exact_support);
  if (sphere.kind() != CircumcenterKind::unique ||
      !sphere.center().has_value() || !sphere.squared_level().has_value()) {
    throw std::invalid_argument(
        "sphere-point classification requires an affinely independent support");
  }
  const SpherePointClassification exact_classification = classify_sphere_point(
      *sphere.center(), *sphere.squared_level(), point.exact(), nullptr);
  const auto canonical_support =
      detail::support_polynomial::canonicalize_support(support);
  FilterResult filtered = FilterResult::uncertain();
  if (detail::policy_allows_fp64(filter_policy)) {
    filtered = detail::support_polynomial::filter_sphere_side(
        point, canonical_support);
  }
  ExpansionResult expanded = ExpansionResult::uncertain();
  if (detail::policy_allows_expansion(filter_policy) &&
      filtered.state() != FilterState::certified) {
    expanded = detail::support_polynomial::expansion_sphere_side(
        point, canonical_support);
  }
  const PredicateDecision decision = detail::certify_materialized_sign(
      filtered,
      expanded,
      exact_classification.decision().sign(),
      filter_policy,
      nullptr);
  return SpherePointClassification::certified_at_stage(
      exact_classification.point_squared_distance(),
      *sphere.squared_level(),
      decision.certification_stage(),
      counters);
}

}  // namespace morsehgp3d::exact
