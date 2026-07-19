#include "morsehgp3d/hierarchy/miniball.hpp"

#include "morsehgp3d/exact/support.hpp"
#include "morsehgp3d/spatial/brute_force.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
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

[[nodiscard]] std::size_t checked_distance_total(
    std::size_t left, std::size_t right) {
  if (left > std::numeric_limits<std::size_t>::max() - right) {
    throw std::length_error(
        "exact descent-precondition distance counters overflow size_t");
  }
  return left + right;
}

[[nodiscard]] bool same_point_ids(
    std::span<const PointId> left,
    std::span<const PointId> right) {
  return left.size() == right.size() &&
         std::equal(left.begin(), left.end(), right.begin());
}

[[nodiscard]] bool contains_point_id(
    std::span<const PointId> sorted_ids, PointId point_id) {
  return std::binary_search(
      sorted_ids.begin(), sorted_ids.end(), point_id);
}

[[nodiscard]] bool all_point_ids_in(
    std::span<const PointId> ids,
    std::span<const PointId> sorted_superset) {
  return std::all_of(
      ids.begin(), ids.end(),
      [sorted_superset](PointId point_id) {
        return contains_point_id(sorted_superset, point_id);
      });
}

[[nodiscard]] std::vector<PointId> sorted_strict_top_k_ids(
    const spatial::TopKPartition& partition) {
  std::vector<PointId> ids;
  ids.reserve(partition.strict_below().size());
  for (const spatial::ExactNeighbor& neighbor : partition.strict_below()) {
    ids.push_back(neighbor.point_id);
  }
  std::sort(ids.begin(), ids.end());
  if (std::adjacent_find(ids.begin(), ids.end()) != ids.end()) {
    throw std::logic_error(
        "an exact top-k strict partition repeats a PointId");
  }
  return ids;
}

[[nodiscard]] bool same_exact_neighbors(
    std::span<const spatial::ExactNeighbor> left,
    std::span<const spatial::ExactNeighbor> right) {
  return left.size() == right.size() &&
         std::equal(
             left.begin(), left.end(), right.begin(), right.end(),
             [](const spatial::ExactNeighbor& left_neighbor,
                const spatial::ExactNeighbor& right_neighbor) {
               return left_neighbor.point_id == right_neighbor.point_id &&
                      left_neighbor.squared_distance ==
                          right_neighbor.squared_distance;
             });
}

[[nodiscard]] bool same_closed_ball_partition(
    const spatial::CanonicalPointCloud& cloud,
    const spatial::ClosedBallPartition& observed,
    const spatial::ClosedBallPartition& expected) {
  if (!observed.partition_complete() ||
      !expected.partition_complete() ||
      !observed.validated_for(cloud) ||
      !expected.validated_for(cloud)) {
    return false;
  }
  return observed.squared_radius() == expected.squared_radius() &&
         same_point_ids(observed.interior_ids(), expected.interior_ids()) &&
         same_point_ids(observed.shell_ids(), expected.shell_ids()) &&
         same_point_ids(observed.exterior_ids(), expected.exterior_ids()) &&
         observed.closed_rank() == expected.closed_rank() &&
         observed.evaluation_count() == expected.evaluation_count() &&
         observed.query_counters() == expected.query_counters();
}

[[nodiscard]] bool same_top_k_partition(
    const spatial::CanonicalPointCloud& cloud,
    const spatial::TopKPartition& observed,
    const spatial::TopKPartition& expected) {
  if (!observed.shell_complete() || !expected.shell_complete() ||
      !observed.validated_for(cloud) || !expected.validated_for(cloud)) {
    return false;
  }
  return observed.requested_rank() == expected.requested_rank() &&
         observed.cutoff_squared_distance() ==
             expected.cutoff_squared_distance() &&
         same_exact_neighbors(
             observed.strict_below(), expected.strict_below()) &&
         same_point_ids(
             observed.cutoff_shell_ids(), expected.cutoff_shell_ids()) &&
         same_point_ids(
             observed.canonical_choice_ids(),
             expected.canonical_choice_ids()) &&
         observed.eligible_point_count() == expected.eligible_point_count() &&
         observed.query_counters() == expected.query_counters();
}

[[nodiscard]] bool facet_is_top_k_member(
    std::span<const PointId> facet,
    std::span<const PointId> sorted_strict_ids,
    std::span<const PointId> sorted_cutoff_shell_ids) {
  if (!all_point_ids_in(sorted_strict_ids, facet)) {
    return false;
  }
  return std::all_of(
      facet.begin(), facet.end(),
      [sorted_strict_ids, sorted_cutoff_shell_ids](PointId point_id) {
        return contains_point_id(sorted_strict_ids, point_id) ||
               contains_point_id(sorted_cutoff_shell_ids, point_id);
      });
}

[[nodiscard]] bool local_global_intersections_close(
    const ExactFacetMiniballResult& miniball,
    const spatial::ClosedBallPartition& global_ball) {
  std::vector<PointId> facet_global_interior;
  std::vector<PointId> facet_global_shell;
  std::vector<PointId> facet_global_exterior;
  facet_global_interior.reserve(miniball.facet_point_ids.size());
  facet_global_shell.reserve(miniball.facet_point_ids.size());
  facet_global_exterior.reserve(miniball.facet_point_ids.size());
  for (const PointId point_id : miniball.facet_point_ids) {
    if (contains_point_id(global_ball.interior_ids(), point_id)) {
      facet_global_interior.push_back(point_id);
    } else if (contains_point_id(global_ball.shell_ids(), point_id)) {
      facet_global_shell.push_back(point_id);
    } else if (contains_point_id(global_ball.exterior_ids(), point_id)) {
      facet_global_exterior.push_back(point_id);
    } else {
      return false;
    }
  }
  return same_point_ids(
             facet_global_interior,
             miniball.strictly_inside_point_ids) &&
         same_point_ids(
             facet_global_shell, miniball.boundary_point_ids) &&
         facet_global_exterior.empty();
}

[[nodiscard]] bool cutoff_partition_closes_against_global_ball(
    const spatial::TopKPartition& top_k,
    const spatial::ClosedBallPartition& global_ball,
    const exact::ExactLevel& facet_squared_radius,
    std::span<const PointId> sorted_strict_ids) {
  if (top_k.cutoff_squared_distance() > facet_squared_radius) {
    return false;
  }
  if (top_k.cutoff_squared_distance() == facet_squared_radius) {
    return same_point_ids(sorted_strict_ids, global_ball.interior_ids()) &&
           same_point_ids(
               top_k.cutoff_shell_ids(), global_ball.shell_ids());
  }
  return all_point_ids_in(sorted_strict_ids, global_ball.interior_ids()) &&
         all_point_ids_in(
             top_k.cutoff_shell_ids(), global_ball.interior_ids());
}

[[nodiscard]] ExactFacetDescentPreconditionDecision expected_decision(
    const ExactFacetDescentPreconditionResult& result) {
  const bool unique_positive_support =
      result.facet_miniball.counters.optimal_support_count == 1U;
  if (!result.local_boundary_equals_support ||
      !unique_positive_support ||
      !result.global_shell_equals_local_boundary) {
    return ExactFacetDescentPreconditionDecision::unsupported_degeneracy;
  }
  return result.facet_is_exact_top_k_member
             ? ExactFacetDescentPreconditionDecision::
                   already_active_at_own_center
             : ExactFacetDescentPreconditionDecision::
                   strict_descent_admissible;
}

[[nodiscard]] ExactFacetDescentPreconditionResult
compute_exact_facet_descent_preconditions(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet_point_ids) {
  ExactFacetDescentPreconditionResult result;
  result.facet_miniball =
      build_exact_facet_miniball(cloud, facet_point_ids);
  const ExactFacetMiniballResult& miniball = result.facet_miniball;
  const std::span<const PointId> facet{miniball.facet_point_ids};
  const std::size_t point_count = cloud.size();

  spatial::ClosedBallPartition global_ball =
      spatial::brute_force_closed_ball(
          cloud, miniball.center, miniball.squared_radius);
  if (!global_ball.partition_complete() ||
      !global_ball.validated_for(cloud) ||
      global_ball.squared_radius() != miniball.squared_radius ||
      global_ball.evaluation_count() != point_count ||
      global_ball.distance_evaluation_count() != point_count ||
      global_ball.query_counters().method !=
          spatial::SpatialQueryMethod::brute_force ||
      global_ball.closed_rank() !=
          global_ball.interior_ids().size() + global_ball.shell_ids().size() ||
      global_ball.closed_rank() < facet.size() ||
      !local_global_intersections_close(miniball, global_ball)) {
    throw std::logic_error(
        "the exact global facet-miniball partition did not close");
  }

  const spatial::ExclusionSet empty_exclusions =
      spatial::ExclusionSet::from_ids(
          std::span<const PointId>{}, cloud, 0U);
  spatial::TopKPartition top_k = spatial::brute_force_top_k(
      cloud, miniball.center, facet.size(), empty_exclusions);
  if (!top_k.shell_complete() || !top_k.validated_for(cloud) ||
      top_k.requested_rank() != facet.size() ||
      top_k.eligible_point_count() != point_count ||
      top_k.distance_evaluation_count() != point_count ||
      top_k.query_counters().method !=
          spatial::SpatialQueryMethod::brute_force ||
      top_k.query_counters().excluded_point_count != 0U ||
      top_k.cutoff_squared_distance() > miniball.squared_radius) {
    throw std::logic_error(
        "the exact global facet top-k partition did not close");
  }

  const std::vector<PointId> strict_ids =
      sorted_strict_top_k_ids(top_k);
  if (!cutoff_partition_closes_against_global_ball(
          top_k, global_ball, miniball.squared_radius, strict_ids)) {
    throw std::logic_error(
        "the exact top-k cutoff disagrees with the global facet-miniball partition");
  }

  result.local_boundary_equals_support = same_point_ids(
      miniball.boundary_point_ids, miniball.support_point_ids);
  result.global_shell_equals_local_boundary = same_point_ids(
      global_ball.shell_ids(), miniball.boundary_point_ids);
  result.facet_is_exact_top_k_member = facet_is_top_k_member(
      facet, strict_ids, top_k.cutoff_shell_ids());

  const bool no_foreign_interior =
      all_point_ids_in(global_ball.interior_ids(), facet);
  if (result.facet_is_exact_top_k_member != no_foreign_interior) {
    throw std::logic_error(
        "exact top-k facet membership disagrees with the global interior");
  }

  const bool regular = result.local_boundary_equals_support &&
      miniball.counters.optimal_support_count == 1U &&
      result.global_shell_equals_local_boundary;
  const bool canonical_choice_equals_facet = same_point_ids(
      top_k.canonical_choice_ids(), facet);
  if (regular && result.facet_is_exact_top_k_member &&
      (global_ball.closed_rank() != facet.size() ||
       !canonical_choice_equals_facet)) {
    throw std::logic_error(
        "a regular active facet did not close at rank k");
  }
  if (regular && !result.facet_is_exact_top_k_member &&
      canonical_choice_equals_facet) {
    throw std::logic_error(
        "a regular strict descent retained its source facet as top-k choice");
  }

  result.counters.global_closed_ball_query_count = 1U;
  result.counters.global_closed_ball_distance_evaluation_count =
      global_ball.distance_evaluation_count();
  result.counters.exact_top_k_query_count = 1U;
  result.counters.exact_top_k_distance_evaluation_count =
      top_k.distance_evaluation_count();
  result.counters.total_exact_point_distance_evaluation_count =
      checked_distance_total(
          result.counters.global_closed_ball_distance_evaluation_count,
          result.counters.exact_top_k_distance_evaluation_count);
  if (result.counters.global_closed_ball_distance_evaluation_count !=
          point_count ||
      result.counters.exact_top_k_distance_evaluation_count != point_count) {
    throw std::logic_error(
        "exact descent-precondition distance counters did not close");
  }

  result.global_closed_ball.emplace(std::move(global_ball));
  result.exact_top_k.emplace(std::move(top_k));
  return result;
}

[[nodiscard]] bool same_computed_descent_preconditions(
    const spatial::CanonicalPointCloud& cloud,
    const ExactFacetDescentPreconditionResult& observed,
    const ExactFacetDescentPreconditionResult& expected) {
  if (!observed.global_closed_ball.has_value() ||
      !expected.global_closed_ball.has_value() ||
      !observed.exact_top_k.has_value() ||
      !expected.exact_top_k.has_value()) {
    return false;
  }
  return same_computed_miniball(
             observed.facet_miniball, expected.facet_miniball) &&
         observed.facet_miniball.status == expected.facet_miniball.status &&
         observed.facet_miniball.scope == expected.facet_miniball.scope &&
         same_closed_ball_partition(
             cloud,
             *observed.global_closed_ball,
             *expected.global_closed_ball) &&
         same_top_k_partition(
             cloud, *observed.exact_top_k, *expected.exact_top_k) &&
         observed.local_boundary_equals_support ==
             expected.local_boundary_equals_support &&
         observed.global_shell_equals_local_boundary ==
             expected.global_shell_equals_local_boundary &&
         observed.facet_is_exact_top_k_member ==
             expected.facet_is_exact_top_k_member &&
         observed.counters == expected.counters;
}

[[nodiscard]] ExactFacetDescentArcDecision expected_arc_decision(
    const ExactFacetDescentPreconditionResult& source_preconditions) {
  switch (source_preconditions.decision) {
    case ExactFacetDescentPreconditionDecision::strict_descent_admissible:
      return ExactFacetDescentArcDecision::strict_descent_arc_certified;
    case ExactFacetDescentPreconditionDecision::
        already_active_at_own_center:
      return ExactFacetDescentArcDecision::
          no_arc_already_active_at_own_center;
    case ExactFacetDescentPreconditionDecision::unsupported_degeneracy:
      return ExactFacetDescentArcDecision::
          no_arc_unsupported_degeneracy;
    case ExactFacetDescentPreconditionDecision::not_certified:
      break;
  }
  throw std::logic_error(
      "an exact descent arc requires certified source preconditions");
}

[[nodiscard]] ExactFacetDescentArcResult compute_exact_facet_descent_arc(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet_point_ids) {
  ExactFacetDescentArcResult result;
  result.source_preconditions =
      build_exact_facet_descent_preconditions(cloud, facet_point_ids);
  result.counters.precondition_classification_count = 1U;

  switch (result.source_preconditions.decision) {
    case ExactFacetDescentPreconditionDecision::strict_descent_admissible: {
      if (!result.source_preconditions.exact_top_k.has_value() ||
          !result.source_preconditions.exact_top_k->shell_complete() ||
          !result.source_preconditions.exact_top_k->validated_for(cloud)) {
        throw std::logic_error(
            "strict descent preconditions omitted their exact top-k partition");
      }
      const spatial::TopKPartition& source_top_k =
          *result.source_preconditions.exact_top_k;
      const std::span<const PointId> source_facet{
          result.source_preconditions.facet_miniball.facet_point_ids};
      std::vector<PointId> successor_facet{
          source_top_k.canonical_choice_ids().begin(),
          source_top_k.canonical_choice_ids().end()};
      if (successor_facet.size() != source_facet.size()) {
        throw std::logic_error(
            "the canonical top-k successor changed the facet cardinality");
      }

      result.successor_is_canonical_top_k_choice = same_point_ids(
          successor_facet, source_top_k.canonical_choice_ids());
      const std::vector<PointId> strict_ids =
          sorted_strict_top_k_ids(source_top_k);
      result.successor_is_exact_top_k_member = facet_is_top_k_member(
          successor_facet, strict_ids, source_top_k.cutoff_shell_ids());
      result.successor_differs_from_source =
          !same_point_ids(successor_facet, source_facet);
      result.counters.canonical_top_k_selection_count = 1U;

      ExactFacetMiniballResult successor_miniball =
          build_exact_facet_miniball(cloud, successor_facet);
      result.counters.successor_miniball_build_count = 1U;
      const bool successor_miniball_matches_choice = same_point_ids(
          successor_miniball.facet_point_ids, successor_facet);
      result.successor_level_within_top_k_cutoff =
          successor_miniball.squared_radius <=
          source_top_k.cutoff_squared_distance();
      result.strict_level_decrease =
          successor_miniball.squared_radius <
          result.source_preconditions.facet_miniball.squared_radius;
      result.counters.exact_level_comparison_count = 2U;

      if (!result.successor_is_canonical_top_k_choice ||
          !result.successor_is_exact_top_k_member ||
          !result.successor_differs_from_source ||
          !successor_miniball_matches_choice ||
          !result.successor_level_within_top_k_cutoff ||
          !result.strict_level_decrease) {
        throw std::logic_error(
            "strict descent preconditions contradicted their canonical exact successor");
      }
      result.successor_facet_point_ids.emplace(
          std::move(successor_facet));
      result.successor_miniball.emplace(
          std::move(successor_miniball));
      break;
    }
    case ExactFacetDescentPreconditionDecision::
        already_active_at_own_center:
    case ExactFacetDescentPreconditionDecision::unsupported_degeneracy:
      break;
    case ExactFacetDescentPreconditionDecision::not_certified:
      throw std::logic_error(
          "an exact descent arc received uncertified source preconditions");
  }

  const bool strict_branch =
      result.source_preconditions.decision ==
      ExactFacetDescentPreconditionDecision::strict_descent_admissible;
  if (result.counters.precondition_classification_count != 1U ||
      result.counters.canonical_top_k_selection_count !=
          (strict_branch ? 1U : 0U) ||
      result.counters.successor_miniball_build_count !=
          (strict_branch ? 1U : 0U) ||
      result.counters.exact_level_comparison_count !=
          (strict_branch ? 2U : 0U) ||
      result.successor_facet_point_ids.has_value() != strict_branch ||
      result.successor_miniball.has_value() != strict_branch) {
    throw std::logic_error(
        "the exact descent-arc branch counters did not close");
  }
  return result;
}

[[nodiscard]] bool same_computed_descent_arc(
    const spatial::CanonicalPointCloud& cloud,
    const ExactFacetDescentArcResult& observed,
    const ExactFacetDescentArcResult& expected) {
  if (!same_computed_descent_preconditions(
          cloud,
          observed.source_preconditions,
          expected.source_preconditions) ||
      observed.source_preconditions.decision !=
          expected.source_preconditions.decision ||
      observed.source_preconditions.scope !=
          expected.source_preconditions.scope ||
      observed.successor_facet_point_ids.has_value() !=
          expected.successor_facet_point_ids.has_value() ||
      observed.successor_miniball.has_value() !=
          expected.successor_miniball.has_value()) {
    return false;
  }
  if (expected.successor_facet_point_ids.has_value() &&
      !same_point_ids(
          *observed.successor_facet_point_ids,
          *expected.successor_facet_point_ids)) {
    return false;
  }
  if (expected.successor_miniball.has_value() &&
      (!same_computed_miniball(
           *observed.successor_miniball,
           *expected.successor_miniball) ||
       observed.successor_miniball->status !=
           expected.successor_miniball->status ||
       observed.successor_miniball->scope !=
           expected.successor_miniball->scope)) {
    return false;
  }
  return observed.successor_is_canonical_top_k_choice ==
             expected.successor_is_canonical_top_k_choice &&
         observed.successor_is_exact_top_k_member ==
             expected.successor_is_exact_top_k_member &&
         observed.successor_differs_from_source ==
             expected.successor_differs_from_source &&
         observed.successor_level_within_top_k_cutoff ==
             expected.successor_level_within_top_k_cutoff &&
         observed.strict_level_decrease ==
             expected.strict_level_decrease &&
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

ExactFacetDescentPreconditionVerification
verify_exact_facet_descent_preconditions(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet_point_ids,
    const ExactFacetDescentPreconditionResult& result) {
  const ExactFacetDescentPreconditionResult expected =
      compute_exact_facet_descent_preconditions(cloud, facet_point_ids);
  ExactFacetDescentPreconditionVerification verification;

  const ExactFacetMiniballVerification miniball_verification =
      verify_exact_facet_miniball(
          cloud, facet_point_ids, result.facet_miniball);
  verification.facet_miniball_certified =
      miniball_verification.local_exact_facet_miniball_certified &&
      same_computed_miniball(
          result.facet_miniball, expected.facet_miniball) &&
      result.facet_miniball.status == expected.facet_miniball.status &&
      result.facet_miniball.scope == expected.facet_miniball.scope;

  verification.global_closed_ball_identity_certified =
      result.global_closed_ball.has_value() &&
      result.global_closed_ball->partition_complete() &&
      result.global_closed_ball->validated_for(cloud);
  verification.global_closed_ball_partition_certified =
      verification.global_closed_ball_identity_certified &&
      expected.global_closed_ball.has_value() &&
      same_closed_ball_partition(
          cloud,
          *result.global_closed_ball,
          *expected.global_closed_ball);

  verification.exact_top_k_identity_certified =
      result.exact_top_k.has_value() &&
      result.exact_top_k->shell_complete() &&
      result.exact_top_k->validated_for(cloud);
  verification.exact_top_k_partition_certified =
      verification.exact_top_k_identity_certified &&
      expected.exact_top_k.has_value() &&
      same_top_k_partition(
          cloud, *result.exact_top_k, *expected.exact_top_k);
  verification.top_k_cutoff_bound_certified =
      verification.exact_top_k_partition_certified &&
      result.exact_top_k->cutoff_squared_distance() <=
          expected.facet_miniball.squared_radius;

  verification.local_boundary_decision_certified =
      result.local_boundary_equals_support ==
      expected.local_boundary_equals_support;
  verification.global_shell_decision_certified =
      result.global_shell_equals_local_boundary ==
      expected.global_shell_equals_local_boundary;
  verification.facet_top_k_membership_decision_certified =
      result.facet_is_exact_top_k_member ==
      expected.facet_is_exact_top_k_member;
  verification.counters_certified = result.counters == expected.counters;
  verification.decision_certified =
      result.decision == expected_decision(expected);
  verification.scope_certified =
      result.scope == ExactFacetDescentPreconditionScope::
                          global_shell_and_top_k_preconditions_only;
  verification.fresh_replay_certified =
      same_computed_descent_preconditions(cloud, result, expected);
  verification.exact_descent_preconditions_certified =
      verification.facet_miniball_certified &&
      verification.global_closed_ball_identity_certified &&
      verification.global_closed_ball_partition_certified &&
      verification.exact_top_k_identity_certified &&
      verification.exact_top_k_partition_certified &&
      verification.top_k_cutoff_bound_certified &&
      verification.local_boundary_decision_certified &&
      verification.global_shell_decision_certified &&
      verification.facet_top_k_membership_decision_certified &&
      verification.counters_certified &&
      verification.decision_certified &&
      verification.scope_certified &&
      verification.fresh_replay_certified;
  return verification;
}

ExactFacetDescentPreconditionResult
build_exact_facet_descent_preconditions(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet_point_ids) {
  ExactFacetDescentPreconditionResult result =
      compute_exact_facet_descent_preconditions(cloud, facet_point_ids);
  result.decision = expected_decision(result);
  result.scope = ExactFacetDescentPreconditionScope::
      global_shell_and_top_k_preconditions_only;
  const ExactFacetDescentPreconditionVerification verification =
      verify_exact_facet_descent_preconditions(
          cloud, facet_point_ids, result);
  if (!verification.exact_descent_preconditions_certified) {
    throw std::logic_error(
        "the exact facet descent preconditions failed their fresh replay");
  }
  return result;
}

ExactFacetDescentArcVerification verify_exact_facet_descent_arc(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet_point_ids,
    const ExactFacetDescentArcResult& result) {
  const ExactFacetDescentArcResult expected =
      compute_exact_facet_descent_arc(cloud, facet_point_ids);
  ExactFacetDescentArcVerification verification;

  const ExactFacetDescentPreconditionVerification source_verification =
      verify_exact_facet_descent_preconditions(
          cloud, facet_point_ids, result.source_preconditions);
  verification.source_preconditions_certified =
      source_verification.exact_descent_preconditions_certified;

  verification.successor_payload_presence_certified =
      result.successor_facet_point_ids.has_value() ==
          expected.successor_facet_point_ids.has_value() &&
      result.successor_miniball.has_value() ==
          expected.successor_miniball.has_value() &&
      result.successor_facet_point_ids.has_value() ==
          result.successor_miniball.has_value();

  if (expected.successor_facet_point_ids.has_value()) {
    verification.successor_facet_certified =
        result.successor_facet_point_ids.has_value() &&
        same_point_ids(
            *result.successor_facet_point_ids,
            *expected.successor_facet_point_ids);
  } else {
    verification.successor_facet_certified =
        !result.successor_facet_point_ids.has_value();
  }

  if (expected.successor_miniball.has_value() &&
      expected.successor_facet_point_ids.has_value() &&
      result.successor_miniball.has_value()) {
    const ExactFacetMiniballVerification successor_verification =
        verify_exact_facet_miniball(
            cloud,
            *expected.successor_facet_point_ids,
            *result.successor_miniball);
    verification.successor_miniball_certified =
        successor_verification.local_exact_facet_miniball_certified &&
        same_computed_miniball(
            *result.successor_miniball,
            *expected.successor_miniball) &&
        result.successor_miniball->status ==
            expected.successor_miniball->status &&
        result.successor_miniball->scope ==
            expected.successor_miniball->scope;
  } else {
    verification.successor_miniball_certified =
        !expected.successor_miniball.has_value() &&
        !result.successor_miniball.has_value();
  }

  verification.successor_is_canonical_top_k_choice_certified =
      result.successor_is_canonical_top_k_choice ==
      expected.successor_is_canonical_top_k_choice;
  verification.successor_is_exact_top_k_member_certified =
      result.successor_is_exact_top_k_member ==
      expected.successor_is_exact_top_k_member;
  verification.successor_differs_from_source_certified =
      result.successor_differs_from_source ==
      expected.successor_differs_from_source;
  verification.successor_level_within_top_k_cutoff_certified =
      result.successor_level_within_top_k_cutoff ==
      expected.successor_level_within_top_k_cutoff;
  verification.strict_level_decrease_certified =
      result.strict_level_decrease == expected.strict_level_decrease;
  verification.counters_certified = result.counters == expected.counters;
  verification.decision_certified =
      result.decision == expected_arc_decision(
                             expected.source_preconditions);
  verification.scope_certified =
      result.scope == ExactFacetDescentArcScope::
                          canonical_top_k_selected_strict_level_arc_only;
  verification.fresh_replay_certified =
      same_computed_descent_arc(cloud, result, expected);
  verification.exact_descent_arc_decision_certified =
      verification.source_preconditions_certified &&
      verification.successor_payload_presence_certified &&
      verification.successor_facet_certified &&
      verification.successor_miniball_certified &&
      verification.successor_is_canonical_top_k_choice_certified &&
      verification.successor_is_exact_top_k_member_certified &&
      verification.successor_differs_from_source_certified &&
      verification.successor_level_within_top_k_cutoff_certified &&
      verification.strict_level_decrease_certified &&
      verification.counters_certified &&
      verification.decision_certified &&
      verification.scope_certified &&
      verification.fresh_replay_certified;
  return verification;
}

ExactFacetDescentArcResult build_exact_facet_descent_arc(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet_point_ids) {
  ExactFacetDescentArcResult result =
      compute_exact_facet_descent_arc(cloud, facet_point_ids);
  result.decision = expected_arc_decision(result.source_preconditions);
  result.scope = ExactFacetDescentArcScope::
      canonical_top_k_selected_strict_level_arc_only;
  const ExactFacetDescentArcVerification verification =
      verify_exact_facet_descent_arc(cloud, facet_point_ids, result);
  if (!verification.exact_descent_arc_decision_certified) {
    throw std::logic_error(
        "the exact facet descent arc failed its fresh replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
