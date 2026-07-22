#include "morsehgp3d/hierarchy/miniball.hpp"

#include "morsehgp3d/spatial/brute_force.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

[[nodiscard]] std::vector<PointId> canonical_facet_point_ids(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet_point_ids) {
  if (facet_point_ids.empty() ||
      facet_point_ids.size() >
          ExactFacetMiniballResult::maximum_facet_point_count) {
    throw std::invalid_argument(
        "an exact facet descent requires between one and ten points");
  }
  std::vector<PointId> facet{
      facet_point_ids.begin(), facet_point_ids.end()};
  std::sort(facet.begin(), facet.end());
  if (std::adjacent_find(facet.begin(), facet.end()) != facet.end()) {
    throw std::invalid_argument(
        "an exact facet descent requires distinct PointIds");
  }
  for (const PointId point_id : facet) {
    if (point_id >= static_cast<PointId>(cloud.size())) {
      throw std::out_of_range(
          "an exact facet descent PointId is outside the cloud");
    }
  }
  return facet;
}

[[nodiscard]] bool same_exact_facet_miniball_payload(
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

[[nodiscard]] std::size_t checked_chain_counter_add(
    std::size_t left, std::size_t right) {
  if (left > std::numeric_limits<std::size_t>::max() - right) {
    throw std::length_error(
        "exact descent-chain counters overflow size_t");
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_chain_counter_multiply(
    std::size_t left, std::size_t right) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::length_error(
        "exact descent-chain counters overflow size_t");
  }
  return left * right;
}

[[nodiscard]] std::size_t capped_binomial_coefficient(
    std::size_t n, std::size_t k, std::size_t cap) {
  if (k > n || cap == 0U) {
    throw std::invalid_argument(
        "a capped facet binomial requires k <= n and a positive cap");
  }
  if (cap == 1U) {
    return cap;
  }

  k = std::min(k, n - k);
  std::size_t value = 1U;
  for (std::size_t index = 1U; index <= k; ++index) {
    std::size_t numerator = n - k + index;
    std::size_t denominator = index;
    const std::size_t numerator_gcd =
        std::gcd(numerator, denominator);
    numerator /= numerator_gcd;
    denominator /= numerator_gcd;
    const std::size_t value_gcd = std::gcd(value, denominator);
    value /= value_gcd;
    denominator /= value_gcd;
    if (denominator != 1U) {
      throw std::logic_error(
          "a binomial recurrence did not divide exactly");
    }
    if (value != 0U && numerator > (cap - 1U) / value) {
      return cap;
    }
    value *= numerator;
    if (value >= cap) {
      return cap;
    }
  }
  return value;
}

[[nodiscard]] std::size_t effective_chain_segment_budget(
    std::size_t point_count,
    std::size_t facet_size,
    ExactFacetDescentChainBudget budget) {
  if (budget.maximum_committed_strict_segment_count >
      ExactFacetDescentChainBudget::
          maximum_supported_committed_strict_segment_count) {
    throw std::invalid_argument(
        "an exact descent-chain budget exceeds its reference cap");
  }
  const std::size_t binomial_cap = checked_chain_counter_add(
      budget.maximum_committed_strict_segment_count, 2U);
  const std::size_t capped_facet_count = capped_binomial_coefficient(
      point_count, facet_size, binomial_cap);
  if (capped_facet_count == 0U) {
    throw std::logic_error(
        "an exact descent chain has no admissible source facet");
  }
  return std::min(
      budget.maximum_committed_strict_segment_count,
      capped_facet_count - 1U);
}

void accumulate_segment_counters(
    ExactFacetDescentSegmentCounters& aggregate,
    const ExactFacetDescentSegmentCounters& contribution) {
  aggregate.source_arc_classification_count = checked_chain_counter_add(
      aggregate.source_arc_classification_count,
      contribution.source_arc_classification_count);
  aggregate.source_atom_distance_evaluation_count =
      checked_chain_counter_add(
          aggregate.source_atom_distance_evaluation_count,
          contribution.source_atom_distance_evaluation_count);
  aggregate.source_atom_maximum_comparison_count =
      checked_chain_counter_add(
          aggregate.source_atom_maximum_comparison_count,
          contribution.source_atom_maximum_comparison_count);
  aggregate.center_displacement_evaluation_count =
      checked_chain_counter_add(
          aggregate.center_displacement_evaluation_count,
          contribution.center_displacement_evaluation_count);
  aggregate.exact_level_relation_count = checked_chain_counter_add(
      aggregate.exact_level_relation_count,
      contribution.exact_level_relation_count);
  aggregate.convex_identity_certification_count =
      checked_chain_counter_add(
          aggregate.convex_identity_certification_count,
          contribution.convex_identity_certification_count);
}

[[nodiscard]] exact::ExactLevel exact_center_squared_distance(
    const exact::ExactRational3& left,
    const exact::ExactRational3& right) {
  exact::ExactRational squared_distance;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational delta =
        left.coordinate(axis) - right.coordinate(axis);
    squared_distance = squared_distance + delta * delta;
  }
  return exact::ExactLevel{std::move(squared_distance)};
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
  return same_exact_facet_miniball_payload(
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
      (!same_exact_facet_miniball_payload(
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

[[nodiscard]] ExactFacetDescentSegmentDecision expected_segment_decision(
    const ExactFacetDescentArcResult& source_arc) {
  switch (source_arc.decision) {
    case ExactFacetDescentArcDecision::strict_descent_arc_certified:
      return ExactFacetDescentSegmentDecision::
          strict_half_open_segment_certified;
    case ExactFacetDescentArcDecision::
        no_arc_already_active_at_own_center:
      return ExactFacetDescentSegmentDecision::
          no_segment_already_active_at_own_center;
    case ExactFacetDescentArcDecision::no_arc_unsupported_degeneracy:
      return ExactFacetDescentSegmentDecision::
          no_segment_unsupported_degeneracy;
    case ExactFacetDescentArcDecision::not_certified:
      break;
  }
  throw std::logic_error(
      "an exact descent segment requires a certified source arc decision");
}

[[nodiscard]] bool same_segment_witness(
    const ExactFacetDescentSegmentWitness& observed,
    const ExactFacetDescentSegmentWitness& expected) {
  return observed.source_atom_level == expected.source_atom_level &&
         observed.successor_atom_level == expected.successor_atom_level &&
         observed.center_squared_displacement ==
             expected.center_squared_displacement &&
         observed.centers_equal == expected.centers_equal &&
         observed.source_endpoint_strict_sublevel ==
             expected.source_endpoint_strict_sublevel &&
         observed.quadratic_max_upper_bound_certified ==
             expected.quadratic_max_upper_bound_certified &&
         observed.closed_segment_nonstrict_sublevel ==
             expected.closed_segment_nonstrict_sublevel &&
         observed.half_open_segment_strict_sublevel ==
             expected.half_open_segment_strict_sublevel;
}

[[nodiscard]] ExactFacetDescentSegmentResult
compute_exact_facet_descent_segment(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet_point_ids) {
  ExactFacetDescentSegmentResult result;
  result.source_arc = build_exact_facet_descent_arc(
      cloud, facet_point_ids);
  result.counters.source_arc_classification_count = 1U;

  switch (result.source_arc.decision) {
    case ExactFacetDescentArcDecision::strict_descent_arc_certified: {
      if (!result.source_arc.successor_facet_point_ids.has_value() ||
          !result.source_arc.successor_miniball.has_value() ||
          !result.source_arc.source_preconditions.exact_top_k.has_value()) {
        throw std::logic_error(
            "a strict exact arc omitted data required by its segment witness");
      }
      const std::vector<PointId>& successor_facet =
          *result.source_arc.successor_facet_point_ids;
      if (successor_facet.empty()) {
        throw std::logic_error(
            "a strict exact segment requires a nonempty successor facet");
      }
      const ExactFacetMiniballResult& source_miniball =
          result.source_arc.source_preconditions.facet_miniball;
      const ExactFacetMiniballResult& successor_miniball =
          *result.source_arc.successor_miniball;
      const spatial::TopKPartition& source_top_k =
          *result.source_arc.source_preconditions.exact_top_k;

      exact::ExactLevel source_atom_level = exact_center_squared_distance(
          source_miniball.center,
          cloud.point(successor_facet.front()).exact());
      result.counters.source_atom_distance_evaluation_count = 1U;
      for (std::size_t index = 1U;
           index < successor_facet.size();
           ++index) {
        const exact::ExactLevel point_level = exact_center_squared_distance(
            source_miniball.center,
            cloud.point(successor_facet[index]).exact());
        ++result.counters.source_atom_distance_evaluation_count;
        ++result.counters.source_atom_maximum_comparison_count;
        if (point_level > source_atom_level) {
          source_atom_level = point_level;
        }
      }

      const exact::ExactLevel successor_atom_level =
          successor_miniball.squared_radius;
      const exact::ExactLevel center_squared_displacement =
          exact_center_squared_distance(
              source_miniball.center, successor_miniball.center);
      result.counters.center_displacement_evaluation_count = 1U;
      result.counters.exact_level_relation_count = 4U;

      const exact::ExactLevel& top_k_cutoff =
          source_top_k.cutoff_squared_distance();
      const exact::ExactLevel& source_level =
          source_miniball.squared_radius;
      const bool centers_equal =
          source_miniball.center == successor_miniball.center;
      const bool zero_displacement =
          center_squared_displacement == exact::ExactLevel{};
      if (centers_equal != zero_displacement ||
          (centers_equal &&
           source_atom_level != successor_atom_level)) {
        throw std::logic_error(
            "exact descent-segment centers disagree with their displacement or atom levels");
      }

      ExactFacetDescentSegmentWitness witness;
      witness.source_atom_level = source_atom_level;
      witness.successor_atom_level = successor_atom_level;
      witness.center_squared_displacement = center_squared_displacement;
      witness.centers_equal = centers_equal;
      witness.source_endpoint_strict_sublevel =
          source_atom_level < source_level;
      witness.quadratic_max_upper_bound_certified =
          source_atom_level == top_k_cutoff &&
          top_k_cutoff <= source_level;
      witness.closed_segment_nonstrict_sublevel =
          source_atom_level <= source_level &&
          successor_atom_level <= source_level;
      witness.half_open_segment_strict_sublevel =
          source_atom_level <= source_level &&
          successor_atom_level < source_level;
      result.counters.convex_identity_certification_count = 1U;

      if (source_atom_level != top_k_cutoff ||
          top_k_cutoff > source_level ||
          successor_atom_level >= source_level ||
          !witness.quadratic_max_upper_bound_certified ||
          !witness.closed_segment_nonstrict_sublevel ||
          !witness.half_open_segment_strict_sublevel) {
        throw std::logic_error(
            "a strict exact arc contradicted its half-open sublevel segment identity");
      }
      result.segment_witness.emplace(std::move(witness));
      break;
    }
    case ExactFacetDescentArcDecision::
        no_arc_already_active_at_own_center:
    case ExactFacetDescentArcDecision::no_arc_unsupported_degeneracy:
      break;
    case ExactFacetDescentArcDecision::not_certified:
      throw std::logic_error(
          "an exact descent segment received an uncertified arc decision");
  }

  const bool strict_branch =
      result.source_arc.decision ==
      ExactFacetDescentArcDecision::strict_descent_arc_certified;
  const std::size_t successor_size = strict_branch
      ? result.source_arc.successor_facet_point_ids->size()
      : 0U;
  const std::size_t expected_maximum_comparison_count =
      successor_size == 0U ? 0U : successor_size - 1U;
  if (result.counters.source_arc_classification_count != 1U ||
      result.counters.source_atom_distance_evaluation_count !=
          successor_size ||
      result.counters.source_atom_maximum_comparison_count !=
          expected_maximum_comparison_count ||
      result.counters.center_displacement_evaluation_count !=
          (strict_branch ? 1U : 0U) ||
      result.counters.exact_level_relation_count !=
          (strict_branch ? 4U : 0U) ||
      result.counters.convex_identity_certification_count !=
          (strict_branch ? 1U : 0U) ||
      result.segment_witness.has_value() != strict_branch) {
    throw std::logic_error(
        "the exact descent-segment branch counters did not close");
  }
  return result;
}

[[nodiscard]] bool same_computed_descent_segment(
    const spatial::CanonicalPointCloud& cloud,
    const ExactFacetDescentSegmentResult& observed,
    const ExactFacetDescentSegmentResult& expected) {
  if (!same_computed_descent_arc(
          cloud, observed.source_arc, expected.source_arc) ||
      observed.source_arc.decision != expected.source_arc.decision ||
      observed.source_arc.scope != expected.source_arc.scope ||
      observed.segment_witness.has_value() !=
          expected.segment_witness.has_value()) {
    return false;
  }
  if (expected.segment_witness.has_value() &&
      !same_segment_witness(
          *observed.segment_witness, *expected.segment_witness)) {
    return false;
  }
  return observed.counters == expected.counters;
}

[[nodiscard]] ExactFacetDescentSegmentResult
compute_stamped_exact_facet_descent_segment(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet_point_ids) {
  ExactFacetDescentSegmentResult result =
      compute_exact_facet_descent_segment(cloud, facet_point_ids);
  result.decision = expected_segment_decision(result.source_arc);
  result.scope = ExactFacetDescentSegmentScope::
      canonical_strict_arc_half_open_sublevel_segment_only;
  return result;
}

[[nodiscard]] ExactFacetDescentChainNodeWitness chain_node_from_miniball(
    const ExactFacetMiniballResult& miniball) {
  ExactFacetDescentChainNodeWitness node;
  node.facet_point_ids = miniball.facet_point_ids;
  node.center = miniball.center;
  node.squared_level = miniball.squared_radius;
  return node;
}

[[nodiscard]] bool same_segment_witnesses(
    std::span<const ExactFacetDescentSegmentWitness> observed,
    std::span<const ExactFacetDescentSegmentWitness> expected) {
  if (observed.size() != expected.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < observed.size(); ++index) {
    if (!same_segment_witness(observed[index], expected[index])) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] ExactFacetDescentChainResult
compute_exact_facet_descent_chain(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet_point_ids,
    ExactFacetDescentChainBudget budget) {
  ExactFacetDescentChainResult result;
  result.requested_budget = budget;

  std::vector<PointId> current_facet =
      canonical_facet_point_ids(cloud, facet_point_ids);
  const std::size_t facet_size = current_facet.size();
  result.effective_maximum_committed_strict_segment_count =
      effective_chain_segment_budget(
          cloud.size(), facet_size, budget);
  if (result.effective_maximum_committed_strict_segment_count >
          result.committed_segment_witnesses.max_size() ||
      result.effective_maximum_committed_strict_segment_count >=
          result.nodes.max_size()) {
    throw std::length_error(
        "an exact descent-chain budget exceeds its vector capacities");
  }
  result.committed_segment_witnesses.reserve(
      result.effective_maximum_committed_strict_segment_count);
  result.nodes.reserve(
      result.effective_maximum_committed_strict_segment_count + 1U);

  std::map<std::vector<PointId>, std::size_t> visited_facets;
  visited_facets.emplace(current_facet, 0U);

  while (!result.stopping_probe.has_value()) {
    ExactFacetDescentSegmentResult probe =
        compute_stamped_exact_facet_descent_segment(
            cloud, current_facet);
    result.counters.facet_probe_count = checked_chain_counter_add(
        result.counters.facet_probe_count, 1U);
    accumulate_segment_counters(
        result.counters.accumulated_probe_counters,
        probe.counters);

    const ExactFacetMiniballResult& source_miniball =
        probe.source_arc.source_preconditions.facet_miniball;
    if (source_miniball.facet_point_ids != current_facet) {
      throw std::logic_error(
          "an exact descent-chain probe changed its source facet");
    }
    const ExactFacetDescentChainNodeWitness source_node =
        chain_node_from_miniball(source_miniball);
    if (result.nodes.empty()) {
      result.nodes.push_back(source_node);
    } else {
      // A seam identifies the preceding target miniball with the freshly
      // rebuilt source miniball. It deliberately does not equate the next
      // source atom level a with the preceding successor atom level b.
      if (result.nodes.back() != source_node) {
        throw std::logic_error(
            "two exact descent-chain segments disagree at their seam");
      }
      result.counters.inter_step_seam_replay_count =
          checked_chain_counter_add(
              result.counters.inter_step_seam_replay_count, 1U);
    }

    switch (probe.decision) {
      case ExactFacetDescentSegmentDecision::
          no_segment_already_active_at_own_center:
        result.counters.active_terminal_count =
            checked_chain_counter_add(
                result.counters.active_terminal_count, 1U);
        result.decision = ExactFacetDescentChainDecision::
            complete_at_regular_active_facet;
        result.stopping_probe.emplace(std::move(probe));
        break;

      case ExactFacetDescentSegmentDecision::
          no_segment_unsupported_degeneracy:
        result.counters.unsupported_terminal_count =
            checked_chain_counter_add(
                result.counters.unsupported_terminal_count, 1U);
        result.decision = ExactFacetDescentChainDecision::
            certified_prefix_blocked_unsupported_degeneracy;
        result.stopping_probe.emplace(std::move(probe));
        break;

      case ExactFacetDescentSegmentDecision::
          strict_half_open_segment_certified: {
        result.counters.strict_segment_probe_count =
            checked_chain_counter_add(
                result.counters.strict_segment_probe_count, 1U);
        result.counters.successor_cycle_lookup_count =
            checked_chain_counter_add(
                result.counters.successor_cycle_lookup_count, 1U);
        if (!probe.segment_witness.has_value() ||
            !probe.source_arc.successor_facet_point_ids.has_value() ||
            !probe.source_arc.successor_miniball.has_value()) {
          throw std::logic_error(
              "a strict exact descent-chain probe omitted its payload");
        }

        const ExactFacetDescentChainNodeWitness target_node =
            chain_node_from_miniball(
                *probe.source_arc.successor_miniball);
        if (target_node.facet_point_ids !=
                *probe.source_arc.successor_facet_point_ids ||
            target_node.facet_point_ids.size() != facet_size ||
            target_node.squared_level >= source_node.squared_level ||
            probe.segment_witness->successor_atom_level !=
                target_node.squared_level ||
            !probe.segment_witness->quadratic_max_upper_bound_certified ||
            !probe.segment_witness->closed_segment_nonstrict_sublevel ||
            !probe.segment_witness->half_open_segment_strict_sublevel) {
          throw std::logic_error(
              "a strict exact descent-chain probe contradicted its compact transition");
        }

        if (visited_facets.find(target_node.facet_point_ids) !=
            visited_facets.end()) {
          throw std::logic_error(
              "a strict exact descent-chain facet potential repeated a facet");
        }

        if (result.committed_segment_witnesses.size() >=
            result.effective_maximum_committed_strict_segment_count) {
          if (result.committed_segment_witnesses.size() !=
              result.effective_maximum_committed_strict_segment_count) {
            throw std::logic_error(
                "an exact descent chain exceeded its effective budget");
          }
          result.counters.structural_budget_stop_count =
              checked_chain_counter_add(
                  result.counters.structural_budget_stop_count, 1U);
          result.decision = ExactFacetDescentChainDecision::
              certified_prefix_strict_segment_budget_exhausted;
          result.stopping_probe.emplace(std::move(probe));
          break;
        }

        const std::size_t target_index = result.nodes.size();
        const auto insertion = visited_facets.emplace(
            target_node.facet_point_ids, target_index);
        if (!insertion.second) {
          throw std::logic_error(
              "an exact descent-chain cycle check changed during insertion");
        }
        result.committed_segment_witnesses.push_back(
            *probe.segment_witness);
        result.nodes.push_back(target_node);
        result.counters.committed_strict_segment_count =
            checked_chain_counter_add(
                result.counters.committed_strict_segment_count, 1U);
        current_facet = result.nodes.back().facet_point_ids;
        break;
      }

      case ExactFacetDescentSegmentDecision::not_certified:
        throw std::logic_error(
            "an exact descent chain received an uncertified segment probe");
    }
  }

  const std::size_t committed_count =
      result.committed_segment_witnesses.size();
  const std::size_t expected_probe_count =
      checked_chain_counter_add(committed_count, 1U);
  const bool budget_stop =
      result.decision == ExactFacetDescentChainDecision::
          certified_prefix_strict_segment_budget_exhausted;
  const std::size_t expected_strict_probe_count =
      checked_chain_counter_add(committed_count, budget_stop ? 1U : 0U);
  const std::size_t expected_node_count =
      checked_chain_counter_add(committed_count, 1U);
  const std::size_t expected_atom_distance_count =
      checked_chain_counter_multiply(
          facet_size, expected_strict_probe_count);
  const std::size_t expected_atom_comparison_count =
      checked_chain_counter_multiply(
          facet_size - 1U, expected_strict_probe_count);
  const std::size_t expected_level_relation_count =
      checked_chain_counter_multiply(
          4U, expected_strict_probe_count);
  const ExactFacetDescentSegmentCounters& aggregate =
      result.counters.accumulated_probe_counters;
  result.counters.visited_facet_count = result.nodes.size();

  bool outcome_counters_close = false;
  if (result.stopping_probe.has_value()) {
    switch (result.decision) {
      case ExactFacetDescentChainDecision::
          complete_at_regular_active_facet:
        outcome_counters_close =
            result.counters.active_terminal_count == 1U &&
            result.counters.unsupported_terminal_count == 0U &&
            result.counters.structural_budget_stop_count == 0U &&
            result.stopping_probe->decision ==
                ExactFacetDescentSegmentDecision::
                    no_segment_already_active_at_own_center;
        break;
      case ExactFacetDescentChainDecision::
          certified_prefix_blocked_unsupported_degeneracy:
        outcome_counters_close =
            result.counters.active_terminal_count == 0U &&
            result.counters.unsupported_terminal_count == 1U &&
            result.counters.structural_budget_stop_count == 0U &&
            result.stopping_probe->decision ==
                ExactFacetDescentSegmentDecision::
                    no_segment_unsupported_degeneracy;
        break;
      case ExactFacetDescentChainDecision::
          certified_prefix_strict_segment_budget_exhausted:
        outcome_counters_close =
            result.counters.active_terminal_count == 0U &&
            result.counters.unsupported_terminal_count == 0U &&
            result.counters.structural_budget_stop_count == 1U &&
            committed_count ==
                result.effective_maximum_committed_strict_segment_count &&
            result.stopping_probe->decision ==
                ExactFacetDescentSegmentDecision::
                    strict_half_open_segment_certified;
        break;
      case ExactFacetDescentChainDecision::not_certified:
        break;
    }
  }

  if (result.nodes.size() != expected_node_count ||
      visited_facets.size() != expected_node_count ||
      result.counters.visited_facet_count != expected_node_count ||
      committed_count >
          result.effective_maximum_committed_strict_segment_count ||
      result.counters.facet_probe_count != expected_probe_count ||
      result.counters.strict_segment_probe_count !=
          expected_strict_probe_count ||
      result.counters.committed_strict_segment_count != committed_count ||
      result.counters.inter_step_seam_replay_count != committed_count ||
      result.counters.successor_cycle_lookup_count !=
          expected_strict_probe_count ||
      !outcome_counters_close ||
      aggregate.source_arc_classification_count != expected_probe_count ||
      aggregate.source_atom_distance_evaluation_count !=
          expected_atom_distance_count ||
      aggregate.source_atom_maximum_comparison_count !=
          expected_atom_comparison_count ||
      aggregate.center_displacement_evaluation_count !=
          expected_strict_probe_count ||
      aggregate.exact_level_relation_count !=
          expected_level_relation_count ||
      aggregate.convex_identity_certification_count !=
          expected_strict_probe_count) {
    throw std::logic_error(
        "the exact descent-chain compact structure or counters did not close");
  }

  result.exact_seams_certified = true;
  result.strict_facet_potential_certified = true;
  result.finite_strict_facet_orbit_theorem_certified = true;
  result.closed_polyline_nonstrict_initial_sublevel = true;
  result.source_open_polyline_strict_initial_sublevel = true;
  return result;
}

[[nodiscard]] bool same_computed_descent_chain(
    const spatial::CanonicalPointCloud& cloud,
    const ExactFacetDescentChainResult& observed,
    const ExactFacetDescentChainResult& expected) {
  if (observed.requested_budget != expected.requested_budget ||
      observed.effective_maximum_committed_strict_segment_count !=
          expected.effective_maximum_committed_strict_segment_count ||
      observed.nodes != expected.nodes ||
      !same_segment_witnesses(
          observed.committed_segment_witnesses,
          expected.committed_segment_witnesses) ||
      observed.stopping_probe.has_value() !=
          expected.stopping_probe.has_value() ||
      observed.exact_seams_certified !=
          expected.exact_seams_certified ||
      observed.strict_facet_potential_certified !=
          expected.strict_facet_potential_certified ||
      observed.finite_strict_facet_orbit_theorem_certified !=
          expected.finite_strict_facet_orbit_theorem_certified ||
      observed.closed_polyline_nonstrict_initial_sublevel !=
          expected.closed_polyline_nonstrict_initial_sublevel ||
      observed.source_open_polyline_strict_initial_sublevel !=
          expected.source_open_polyline_strict_initial_sublevel ||
      observed.counters != expected.counters ||
      observed.decision != expected.decision) {
    return false;
  }
  if (expected.stopping_probe.has_value() &&
      (!same_computed_descent_segment(
           cloud,
           *observed.stopping_probe,
           *expected.stopping_probe) ||
       observed.stopping_probe->decision !=
           expected.stopping_probe->decision ||
       observed.stopping_probe->scope != expected.stopping_probe->scope)) {
    return false;
  }
  return true;
}

}  // namespace

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
      same_exact_facet_miniball_payload(
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
        same_exact_facet_miniball_payload(
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

ExactFacetDescentSegmentVerification verify_exact_facet_descent_segment(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet_point_ids,
    const ExactFacetDescentSegmentResult& result) {
  const ExactFacetDescentSegmentResult expected =
      compute_exact_facet_descent_segment(cloud, facet_point_ids);
  ExactFacetDescentSegmentVerification verification;

  const ExactFacetDescentArcVerification source_verification =
      verify_exact_facet_descent_arc(
          cloud, facet_point_ids, result.source_arc);
  verification.source_arc_certified =
      source_verification.exact_descent_arc_decision_certified;
  verification.segment_witness_presence_certified =
      result.segment_witness.has_value() ==
      expected.segment_witness.has_value();

  if (expected.segment_witness.has_value() &&
      result.segment_witness.has_value()) {
    const ExactFacetDescentSegmentWitness& observed_witness =
        *result.segment_witness;
    const ExactFacetDescentSegmentWitness& expected_witness =
        *expected.segment_witness;
    verification.source_atom_level_certified =
        observed_witness.source_atom_level ==
        expected_witness.source_atom_level;
    verification.successor_atom_level_certified =
        observed_witness.successor_atom_level ==
        expected_witness.successor_atom_level;
    verification.center_squared_displacement_certified =
        observed_witness.center_squared_displacement ==
        expected_witness.center_squared_displacement;
    verification.centers_equal_certified =
        observed_witness.centers_equal == expected_witness.centers_equal;
    verification.source_endpoint_strict_sublevel_certified =
        observed_witness.source_endpoint_strict_sublevel ==
        expected_witness.source_endpoint_strict_sublevel;
    verification.quadratic_max_upper_bound_certified =
        observed_witness.quadratic_max_upper_bound_certified ==
        expected_witness.quadratic_max_upper_bound_certified;
    verification.closed_segment_nonstrict_sublevel_certified =
        observed_witness.closed_segment_nonstrict_sublevel ==
        expected_witness.closed_segment_nonstrict_sublevel;
    verification.half_open_segment_strict_sublevel_certified =
        observed_witness.half_open_segment_strict_sublevel ==
        expected_witness.half_open_segment_strict_sublevel;

    if (!expected.source_arc.source_preconditions.exact_top_k.has_value() ||
        !expected.source_arc.successor_miniball.has_value()) {
      throw std::logic_error(
          "a fresh strict segment omitted its expected arc witnesses");
    }
    const ExactFacetMiniballResult& expected_source_miniball =
        expected.source_arc.source_preconditions.facet_miniball;
    const ExactFacetMiniballResult& expected_successor_miniball =
        *expected.source_arc.successor_miniball;
    const exact::ExactLevel& expected_cutoff =
        expected.source_arc.source_preconditions.exact_top_k->
            cutoff_squared_distance();
    const bool expected_centers_equal =
        expected_source_miniball.center ==
        expected_successor_miniball.center;
    const exact::ExactLevel expected_displacement =
        exact_center_squared_distance(
            expected_source_miniball.center,
            expected_successor_miniball.center);
    verification.exact_level_relations_certified =
        observed_witness.source_atom_level == expected_cutoff &&
        expected_cutoff <= expected_source_miniball.squared_radius &&
        observed_witness.successor_atom_level ==
            expected_successor_miniball.squared_radius &&
        observed_witness.successor_atom_level <
            expected_source_miniball.squared_radius &&
        observed_witness.center_squared_displacement ==
            expected_displacement &&
        observed_witness.centers_equal == expected_centers_equal &&
        (observed_witness.center_squared_displacement ==
             exact::ExactLevel{}) == observed_witness.centers_equal &&
        (!observed_witness.centers_equal ||
         observed_witness.source_atom_level ==
             observed_witness.successor_atom_level) &&
        observed_witness.source_endpoint_strict_sublevel ==
            (observed_witness.source_atom_level <
             expected_source_miniball.squared_radius) &&
        observed_witness.quadratic_max_upper_bound_certified &&
        observed_witness.closed_segment_nonstrict_sublevel &&
        observed_witness.half_open_segment_strict_sublevel;
  } else {
    const bool both_absent =
        !expected.segment_witness.has_value() &&
        !result.segment_witness.has_value();
    verification.source_atom_level_certified = both_absent;
    verification.successor_atom_level_certified = both_absent;
    verification.center_squared_displacement_certified = both_absent;
    verification.centers_equal_certified = both_absent;
    verification.source_endpoint_strict_sublevel_certified = both_absent;
    verification.quadratic_max_upper_bound_certified = both_absent;
    verification.closed_segment_nonstrict_sublevel_certified = both_absent;
    verification.half_open_segment_strict_sublevel_certified = both_absent;
    verification.exact_level_relations_certified = both_absent;
  }

  verification.counters_certified = result.counters == expected.counters;
  verification.decision_certified =
      result.decision == expected_segment_decision(expected.source_arc);
  verification.scope_certified =
      result.scope == ExactFacetDescentSegmentScope::
                          canonical_strict_arc_half_open_sublevel_segment_only;
  verification.fresh_replay_certified =
      same_computed_descent_segment(cloud, result, expected);
  verification.exact_descent_segment_decision_certified =
      verification.source_arc_certified &&
      verification.segment_witness_presence_certified &&
      verification.source_atom_level_certified &&
      verification.successor_atom_level_certified &&
      verification.center_squared_displacement_certified &&
      verification.centers_equal_certified &&
      verification.source_endpoint_strict_sublevel_certified &&
      verification.quadratic_max_upper_bound_certified &&
      verification.closed_segment_nonstrict_sublevel_certified &&
      verification.half_open_segment_strict_sublevel_certified &&
      verification.exact_level_relations_certified &&
      verification.counters_certified &&
      verification.decision_certified &&
      verification.scope_certified &&
      verification.fresh_replay_certified;
  return verification;
}

ExactFacetDescentSegmentResult build_exact_facet_descent_segment(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet_point_ids) {
  ExactFacetDescentSegmentResult result =
      compute_stamped_exact_facet_descent_segment(
          cloud, facet_point_ids);
  const ExactFacetDescentSegmentVerification verification =
      verify_exact_facet_descent_segment(cloud, facet_point_ids, result);
  if (!verification.exact_descent_segment_decision_certified) {
    throw std::logic_error(
        "the exact facet descent segment failed its fresh replay");
  }
  return result;
}

ExactFacetDescentChainVerification verify_exact_facet_descent_chain(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet_point_ids,
    ExactFacetDescentChainBudget budget,
    const ExactFacetDescentChainResult& result) {
  const ExactFacetDescentChainResult expected =
      compute_exact_facet_descent_chain(
          cloud, facet_point_ids, budget);
  ExactFacetDescentChainVerification verification;

  verification.requested_budget_certified =
      result.requested_budget == budget &&
      result.requested_budget == expected.requested_budget;
  verification.effective_budget_certified =
      result.effective_maximum_committed_strict_segment_count ==
      expected.effective_maximum_committed_strict_segment_count;

  const bool observed_path_bounded =
      result.committed_segment_witnesses.size() <=
          ExactFacetDescentChainBudget::
              maximum_supported_committed_strict_segment_count &&
      result.nodes.size() <=
          ExactFacetDescentChainBudget::
                  maximum_supported_committed_strict_segment_count +
              1U;
  verification.compact_path_shape_certified =
      observed_path_bounded && !result.nodes.empty() &&
      result.nodes.size() ==
          result.committed_segment_witnesses.size() + 1U &&
      result.committed_segment_witnesses.size() <=
          result.effective_maximum_committed_strict_segment_count;
  verification.initial_facet_identity_certified =
      !result.nodes.empty() && !expected.nodes.empty() &&
      result.nodes.front().facet_point_ids ==
          expected.nodes.front().facet_point_ids;
  verification.compact_nodes_certified =
      result.nodes == expected.nodes;
  verification.committed_segment_witnesses_certified =
      same_segment_witnesses(
          result.committed_segment_witnesses,
          expected.committed_segment_witnesses);
  verification.stopping_probe_presence_certified =
      result.stopping_probe.has_value() &&
      expected.stopping_probe.has_value();

  if (verification.stopping_probe_presence_certified &&
      !expected.nodes.empty()) {
    const ExactFacetDescentSegmentVerification stopping_verification =
        verify_exact_facet_descent_segment(
            cloud,
            expected.nodes.back().facet_point_ids,
            *result.stopping_probe);
    verification.stopping_probe_certified =
        stopping_verification.exact_descent_segment_decision_certified &&
        same_computed_descent_segment(
            cloud,
            *result.stopping_probe,
            *expected.stopping_probe) &&
        result.stopping_probe->decision ==
            expected.stopping_probe->decision &&
        result.stopping_probe->scope == expected.stopping_probe->scope;
  }

  verification.exact_seams_certified =
      result.exact_seams_certified &&
      result.exact_seams_certified ==
          expected.exact_seams_certified;
  verification.strict_facet_potential_certified =
      result.strict_facet_potential_certified &&
      result.strict_facet_potential_certified ==
          expected.strict_facet_potential_certified;
  verification.finite_strict_facet_orbit_theorem_certified =
      result.finite_strict_facet_orbit_theorem_certified &&
      result.finite_strict_facet_orbit_theorem_certified ==
          expected.finite_strict_facet_orbit_theorem_certified;
  verification.closed_polyline_nonstrict_initial_sublevel_certified =
      result.closed_polyline_nonstrict_initial_sublevel &&
      result.closed_polyline_nonstrict_initial_sublevel ==
          expected.closed_polyline_nonstrict_initial_sublevel;
  verification.source_open_polyline_strict_initial_sublevel_certified =
      result.source_open_polyline_strict_initial_sublevel &&
      result.source_open_polyline_strict_initial_sublevel ==
          expected.source_open_polyline_strict_initial_sublevel;
  verification.counters_certified =
      result.counters == expected.counters;
  verification.decision_certified =
      result.decision == expected.decision;
  verification.scope_certified =
      result.scope == ExactFacetDescentChainScope::
                          single_source_canonical_strict_descent_chain_only;
  verification.fresh_replay_certified =
      same_computed_descent_chain(cloud, result, expected);
  verification.exact_descent_chain_decision_certified =
      verification.requested_budget_certified &&
      verification.effective_budget_certified &&
      verification.compact_path_shape_certified &&
      verification.initial_facet_identity_certified &&
      verification.compact_nodes_certified &&
      verification.committed_segment_witnesses_certified &&
      verification.stopping_probe_presence_certified &&
      verification.stopping_probe_certified &&
      verification.exact_seams_certified &&
      verification.strict_facet_potential_certified &&
      verification.finite_strict_facet_orbit_theorem_certified &&
      verification.closed_polyline_nonstrict_initial_sublevel_certified &&
      verification.source_open_polyline_strict_initial_sublevel_certified &&
      verification.counters_certified &&
      verification.decision_certified &&
      verification.scope_certified &&
      verification.fresh_replay_certified;
  return verification;
}

ExactFacetDescentChainResult build_exact_facet_descent_chain(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> facet_point_ids,
    ExactFacetDescentChainBudget budget) {
  ExactFacetDescentChainResult result =
      compute_exact_facet_descent_chain(
          cloud, facet_point_ids, budget);
  result.scope = ExactFacetDescentChainScope::
      single_source_canonical_strict_descent_chain_only;
  const ExactFacetDescentChainVerification verification =
      verify_exact_facet_descent_chain(
          cloud, facet_point_ids, budget, result);
  if (!verification.exact_descent_chain_decision_certified) {
    throw std::logic_error(
        "the exact facet descent chain failed its fresh replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
