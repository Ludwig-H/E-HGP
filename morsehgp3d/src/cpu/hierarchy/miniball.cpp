#include "morsehgp3d/hierarchy/miniball.hpp"

#include "morsehgp3d/exact/support.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

struct EnclosingSupportCandidate {
  std::vector<PointId> support_point_ids;
  exact::ExactCenter3 center;
  exact::ExactLevel squared_radius;
};

[[nodiscard]] std::vector<PointId> canonical_facet(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet_point_ids) {
  if (facet_point_ids.empty() ||
      facet_point_ids.size() >
          ExactFacetMiniballResult::maximum_facet_point_count) {
    throw std::invalid_argument(
        "an exact facet miniball requires between one and ten points");
  }
  std::vector<PointId> facet{
      facet_point_ids.begin(), facet_point_ids.end()};
  std::sort(facet.begin(), facet.end());
  if (std::adjacent_find(facet.begin(), facet.end()) != facet.end()) {
    throw std::invalid_argument(
        "an exact facet miniball requires distinct PointIds");
  }
  for (const PointId point_id : facet) {
    if (point_id >= static_cast<PointId>(cloud.size())) {
      throw std::out_of_range(
          "an exact facet miniball PointId is outside the cloud");
    }
  }
  return facet;
}

template <std::size_t SupportSize>
void evaluate_support(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet,
    const std::array<std::size_t, SupportSize>& positions,
    ExactFacetMiniballCounters& counters,
    std::vector<EnclosingSupportCandidate>& enclosing_candidates) {
  static_assert(SupportSize >= 1U && SupportSize <= 4U);
  ++counters.enumerated_support_count_by_size[SupportSize - 1U];
  ++counters.enumerated_support_count;

  std::array<exact::ExactRational3, SupportSize> support;
  std::vector<PointId> support_point_ids;
  support_point_ids.reserve(SupportSize);
  for (std::size_t index = 0U; index < SupportSize; ++index) {
    if (positions[index] >= facet.size()) {
      throw std::logic_error(
          "an exact miniball support position is outside its facet");
    }
    const PointId point_id = facet[positions[index]];
    support[index] = cloud.point(point_id).exact();
    support_point_ids.push_back(point_id);
  }

  const exact::CircumcenterSupportAnalysis analysis =
      exact::analyze_circumcenter_support(support);
  switch (analysis.status()) {
    case exact::CircumcenterSupportStatus::affinely_dependent:
      ++counters.affinely_dependent_support_count;
      return;
    case exact::CircumcenterSupportStatus::boundary_reduced:
      ++counters.boundary_reduced_support_count;
      return;
    case exact::CircumcenterSupportStatus::exterior_circumcenter:
      ++counters.exterior_circumcenter_support_count;
      return;
    case exact::CircumcenterSupportStatus::minimal:
      ++counters.minimal_support_candidate_count;
      break;
  }

  const exact::CircumcenterResult& sphere = analysis.circumcenter_result();
  if (sphere.kind() != exact::CircumcenterKind::unique ||
      !sphere.center().has_value() ||
      !sphere.squared_level().has_value()) {
    throw std::logic_error(
        "a minimal exact support omitted its unique sphere witnesses");
  }

  bool encloses_facet = true;
  for (const PointId point_id : facet) {
    const exact::SpherePointClassification classification =
        exact::classify_sphere_point(
            *sphere.center(),
            *sphere.squared_level(),
            cloud.point(point_id));
    ++counters.candidate_point_classification_count;
    switch (classification.location()) {
      case exact::SpherePointLocation::strictly_inside:
        ++counters.candidate_strictly_inside_classification_count;
        break;
      case exact::SpherePointLocation::boundary:
        ++counters.candidate_boundary_classification_count;
        break;
      case exact::SpherePointLocation::outside:
        ++counters.candidate_outside_classification_count;
        encloses_facet = false;
        break;
    }
  }
  if (!encloses_facet) {
    return;
  }
  ++counters.enclosing_support_count;
  enclosing_candidates.push_back(EnclosingSupportCandidate{
      std::move(support_point_ids),
      *sphere.center(),
      *sphere.squared_level()});
}

[[nodiscard]] bool canonical_support_less(
    const EnclosingSupportCandidate& left,
    const EnclosingSupportCandidate& right) {
  if (left.support_point_ids.size() != right.support_point_ids.size()) {
    return left.support_point_ids.size() < right.support_point_ids.size();
  }
  return std::lexicographical_compare(
      left.support_point_ids.begin(),
      left.support_point_ids.end(),
      right.support_point_ids.begin(),
      right.support_point_ids.end());
}

[[nodiscard]] std::array<std::size_t, 4> expected_support_counts(
    std::size_t point_count) {
  std::array<std::size_t, 4> counts{};
  for (std::size_t support_size = 1U;
       support_size <= counts.size();
       ++support_size) {
    if (support_size > point_count) {
      continue;
    }
    std::size_t value = 1U;
    for (std::size_t factor = 0U; factor < support_size; ++factor) {
      value *= point_count - factor;
      value /= factor + 1U;
    }
    counts[support_size - 1U] = value;
  }
  return counts;
}

[[nodiscard]] std::size_t support_count_sum(
    const std::array<std::size_t, 4>& counts) {
  std::size_t sum = 0U;
  for (const std::size_t count : counts) {
    sum += count;
  }
  return sum;
}

[[nodiscard]] ExactFacetMiniballResult compute_exact_facet_miniball(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> input_facet) {
  ExactFacetMiniballResult result;
  result.facet_point_ids = canonical_facet(cloud, input_facet);
  result.counters.facet_point_count = result.facet_point_ids.size();

  std::vector<EnclosingSupportCandidate> enclosing_candidates;
  enclosing_candidates.reserve(
      ExactFacetMiniballResult::maximum_enumerated_support_count);
  const std::span<const PointId> facet{result.facet_point_ids};
  for (std::size_t first = 0U; first < facet.size(); ++first) {
    evaluate_support<1U>(
        cloud,
        facet,
        std::array<std::size_t, 1>{first},
        result.counters,
        enclosing_candidates);
  }
  for (std::size_t first = 0U; first < facet.size(); ++first) {
    for (std::size_t second = first + 1U;
         second < facet.size();
         ++second) {
      evaluate_support<2U>(
          cloud,
          facet,
          std::array<std::size_t, 2>{first, second},
          result.counters,
          enclosing_candidates);
    }
  }
  for (std::size_t first = 0U; first < facet.size(); ++first) {
    for (std::size_t second = first + 1U;
         second < facet.size();
         ++second) {
      for (std::size_t third = second + 1U;
           third < facet.size();
           ++third) {
        evaluate_support<3U>(
            cloud,
            facet,
            std::array<std::size_t, 3>{first, second, third},
            result.counters,
            enclosing_candidates);
      }
    }
  }
  for (std::size_t first = 0U; first < facet.size(); ++first) {
    for (std::size_t second = first + 1U;
         second < facet.size();
         ++second) {
      for (std::size_t third = second + 1U;
           third < facet.size();
           ++third) {
        for (std::size_t fourth = third + 1U;
             fourth < facet.size();
             ++fourth) {
          evaluate_support<4U>(
              cloud,
              facet,
              std::array<std::size_t, 4>{
                  first, second, third, fourth},
              result.counters,
              enclosing_candidates);
        }
      }
    }
  }

  const std::array<std::size_t, 4> expected_counts =
      expected_support_counts(facet.size());
  if (result.counters.enumerated_support_count_by_size != expected_counts ||
      result.counters.enumerated_support_count !=
          support_count_sum(expected_counts) ||
      result.counters.enumerated_support_count >
          ExactFacetMiniballResult::maximum_enumerated_support_count ||
      result.counters.affinely_dependent_support_count +
              result.counters.boundary_reduced_support_count +
              result.counters.exterior_circumcenter_support_count +
              result.counters.minimal_support_candidate_count !=
          result.counters.enumerated_support_count ||
      result.counters.candidate_point_classification_count !=
          result.counters.minimal_support_candidate_count * facet.size() ||
      result.counters.candidate_strictly_inside_classification_count +
              result.counters.candidate_boundary_classification_count +
              result.counters.candidate_outside_classification_count !=
          result.counters.candidate_point_classification_count ||
      enclosing_candidates.empty() ||
      result.counters.enclosing_support_count !=
          enclosing_candidates.size()) {
    throw std::logic_error(
        "the exhaustive exact facet-miniball enumeration did not close");
  }

  const auto minimum = std::min_element(
      enclosing_candidates.begin(),
      enclosing_candidates.end(),
      [](const EnclosingSupportCandidate& left,
         const EnclosingSupportCandidate& right) {
        return left.squared_radius < right.squared_radius;
      });
  if (minimum == enclosing_candidates.end()) {
    throw std::logic_error("an exact facet miniball has no enclosing support");
  }
  const exact::ExactLevel minimum_radius = minimum->squared_radius;
  std::optional<EnclosingSupportCandidate> selected;
  for (const EnclosingSupportCandidate& candidate : enclosing_candidates) {
    if (candidate.squared_radius != minimum_radius) {
      continue;
    }
    ++result.counters.optimal_support_count;
    if (candidate.center != minimum->center) {
      throw std::logic_error(
          "minimum enclosing supports disagree on the unique exact center");
    }
    if (!selected.has_value() || canonical_support_less(candidate, *selected)) {
      selected = candidate;
    }
  }
  if (!selected.has_value() || result.counters.optimal_support_count == 0U) {
    throw std::logic_error(
        "the exact facet miniball omitted its canonical optimal support");
  }

  result.support_point_ids = selected->support_point_ids;
  result.center = selected->center;
  result.squared_radius = selected->squared_radius;
  result.counters.selected_support_size = result.support_point_ids.size();
  for (const PointId point_id : facet) {
    const exact::SpherePointClassification classification =
        exact::classify_sphere_point(
            result.center,
            result.squared_radius,
            cloud.point(point_id));
    switch (classification.location()) {
      case exact::SpherePointLocation::strictly_inside:
        result.strictly_inside_point_ids.push_back(point_id);
        break;
      case exact::SpherePointLocation::boundary:
        result.boundary_point_ids.push_back(point_id);
        break;
      case exact::SpherePointLocation::outside:
        throw std::logic_error(
            "the selected exact facet miniball excludes a facet point");
    }
  }
  if (result.support_point_ids.empty() ||
      result.support_point_ids.size() >
          ExactFacetMiniballResult::maximum_support_point_count ||
      result.strictly_inside_point_ids.size() +
              result.boundary_point_ids.size() !=
          facet.size() ||
      !std::includes(
          result.boundary_point_ids.begin(),
          result.boundary_point_ids.end(),
          result.support_point_ids.begin(),
          result.support_point_ids.end())) {
    throw std::logic_error(
        "the selected exact facet-miniball partition did not close");
  }
  return result;
}

[[nodiscard]] bool same_computed_miniball(
    const ExactFacetMiniballResult& observed,
    const ExactFacetMiniballResult& expected) {
  return observed.facet_point_ids == expected.facet_point_ids &&
         observed.support_point_ids == expected.support_point_ids &&
         observed.strictly_inside_point_ids ==
             expected.strictly_inside_point_ids &&
         observed.boundary_point_ids == expected.boundary_point_ids &&
         observed.center == expected.center &&
         observed.squared_radius == expected.squared_radius &&
         observed.counters == expected.counters;
}

}  // namespace

ExactFacetMiniballVerification verify_exact_facet_miniball(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet_point_ids,
    const ExactFacetMiniballResult& result) {
  const ExactFacetMiniballResult expected =
      compute_exact_facet_miniball(cloud, facet_point_ids);
  ExactFacetMiniballVerification verification;
  verification.facet_identity_certified =
      result.facet_point_ids == expected.facet_point_ids;
  verification.exhaustive_support_enumeration_certified =
      result.counters.enumerated_support_count_by_size ==
          expected.counters.enumerated_support_count_by_size &&
      result.counters.enumerated_support_count ==
          expected.counters.enumerated_support_count &&
      result.counters.enumerated_support_count <=
          ExactFacetMiniballResult::maximum_enumerated_support_count;
  verification.exact_center_and_radius_certified =
      result.center == expected.center &&
      result.squared_radius == expected.squared_radius;
  verification.enclosing_partition_certified =
      result.strictly_inside_point_ids ==
          expected.strictly_inside_point_ids &&
      result.boundary_point_ids == expected.boundary_point_ids;
  verification.canonical_support_certified =
      result.support_point_ids == expected.support_point_ids;
  verification.counters_certified = result.counters == expected.counters;
  verification.status_certified =
      result.status ==
      ExactFacetMiniballStatus::exact_facet_miniball_certified;
  verification.local_scope_certified =
      result.scope == ExactFacetMiniballScope::local_facet_miniball_only;
  verification.fresh_replay_certified = same_computed_miniball(
      result, expected);
  verification.local_exact_facet_miniball_certified =
      verification.facet_identity_certified &&
      verification.exhaustive_support_enumeration_certified &&
      verification.exact_center_and_radius_certified &&
      verification.enclosing_partition_certified &&
      verification.canonical_support_certified &&
      verification.counters_certified &&
      verification.status_certified &&
      verification.local_scope_certified &&
      verification.fresh_replay_certified;
  return verification;
}

ExactFacetMiniballResult build_exact_facet_miniball(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet_point_ids) {
  ExactFacetMiniballResult result =
      compute_exact_facet_miniball(cloud, facet_point_ids);
  result.status =
      ExactFacetMiniballStatus::exact_facet_miniball_certified;
  result.scope = ExactFacetMiniballScope::local_facet_miniball_only;
  const ExactFacetMiniballVerification verification =
      verify_exact_facet_miniball(cloud, facet_point_ids, result);
  if (!verification.local_exact_facet_miniball_certified) {
    throw std::logic_error(
        "the exact facet miniball failed its fresh exhaustive replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
