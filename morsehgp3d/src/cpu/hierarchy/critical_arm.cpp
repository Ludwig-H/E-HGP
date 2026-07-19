#include "morsehgp3d/hierarchy/critical_arm.hpp"

#include "morsehgp3d/spatial/brute_force.hpp"

#include <algorithm>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

[[nodiscard]] bool same_point_ids(
    std::span<const PointId> left,
    std::span<const PointId> right) {
  return left.size() == right.size() &&
         std::equal(left.begin(), left.end(), right.begin());
}

[[nodiscard]] std::vector<PointId> canonical_critical_shell(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> critical_shell_point_ids,
    PointId removed_shell_point_id) {
  if (critical_shell_point_ids.size() <
          ExactCriticalArmInitialSegmentResult::
              minimum_critical_shell_point_count ||
      critical_shell_point_ids.size() >
          ExactCriticalArmInitialSegmentResult::
              maximum_critical_shell_point_count) {
    throw std::invalid_argument(
        "a critical arm requires a shell of two to four points");
  }
  std::vector<PointId> shell(
      critical_shell_point_ids.begin(), critical_shell_point_ids.end());
  std::sort(shell.begin(), shell.end());
  if (std::adjacent_find(shell.begin(), shell.end()) != shell.end()) {
    throw std::invalid_argument(
        "a critical shell cannot repeat a PointId");
  }
  for (const PointId point_id : shell) {
    static_cast<void>(cloud.point(point_id));
  }
  if (!std::binary_search(
          shell.begin(), shell.end(), removed_shell_point_id)) {
    throw std::invalid_argument(
        "the removed point must belong to the critical shell");
  }
  return shell;
}

[[nodiscard]] exact::ExactRational linear_distance_coefficient(
    const exact::ExactRational3& source,
    const exact::ExactRational3& target,
    const exact::ExactRational3& point) {
  exact::ExactRational dot_product;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational source_to_point =
        source.coordinate(axis) - point.coordinate(axis);
    const exact::ExactRational displacement =
        target.coordinate(axis) - source.coordinate(axis);
    dot_product = dot_product + source_to_point * displacement;
  }
  return exact::ExactRational{exact::BigInt{2}} * dot_product;
}

[[nodiscard]] exact::ExactLevel exact_squared_distance(
    const exact::ExactRational3& left,
    const exact::ExactRational3& right) {
  exact::ExactRational squared_distance;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational difference =
        left.coordinate(axis) - right.coordinate(axis);
    squared_distance =
        squared_distance + difference * difference;
  }
  return exact::ExactLevel{std::move(squared_distance)};
}

[[nodiscard]] bool same_closed_ball_partition(
    const spatial::CanonicalPointCloud& cloud,
    const spatial::ClosedBallPartition& observed,
    const spatial::ClosedBallPartition& expected) {
  return observed.partition_complete() &&
         expected.partition_complete() &&
         observed.validated_for(cloud) &&
         expected.validated_for(cloud) &&
         observed.squared_radius() == expected.squared_radius() &&
         same_point_ids(
             observed.interior_ids(), expected.interior_ids()) &&
         same_point_ids(observed.shell_ids(), expected.shell_ids()) &&
         same_point_ids(
             observed.exterior_ids(), expected.exterior_ids()) &&
         observed.closed_rank() == expected.closed_rank() &&
         observed.evaluation_count() == expected.evaluation_count() &&
         observed.query_counters() == expected.query_counters();
}

[[nodiscard]] bool same_segment_coefficients(
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

[[nodiscard]] bool critical_shell_is_positive_minimal(
    const ExactFacetMiniballResult& miniball,
    std::span<const PointId> critical_shell_point_ids) {
  return miniball.status ==
             ExactFacetMiniballStatus::exact_facet_miniball_certified &&
         miniball.scope == ExactFacetMiniballScope::
                               local_facet_miniball_only &&
         same_point_ids(
             miniball.facet_point_ids, critical_shell_point_ids) &&
         same_point_ids(
             miniball.support_point_ids, critical_shell_point_ids) &&
         miniball.strictly_inside_point_ids.empty() &&
         same_point_ids(
             miniball.boundary_point_ids, critical_shell_point_ids) &&
         miniball.counters.optimal_support_count == 1U;
}

void finish_unsupported_source(
    ExactCriticalArmInitialSegmentResult& result,
    ExactCriticalArmSourceDecision source_decision) {
  result.source_decision = source_decision;
  result.decision = ExactCriticalArmInitialSegmentDecision::
      no_segment_unsupported_critical_source;
  result.scope = ExactCriticalArmInitialSegmentScope::
      single_index_one_critical_arm_initial_germ_segment_only;
}

[[nodiscard]] ExactCriticalArmInitialSegmentResult
compute_exact_critical_arm_initial_segment(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> critical_shell_point_ids,
    PointId removed_shell_point_id) {
  ExactCriticalArmInitialSegmentResult result;
  result.critical_shell_point_ids = canonical_critical_shell(
      cloud, critical_shell_point_ids, removed_shell_point_id);
  result.removed_shell_point_id = removed_shell_point_id;

  result.critical_shell_miniball = build_exact_facet_miniball(
      cloud, result.critical_shell_point_ids);
  result.counters.critical_shell_miniball_build_count = 1U;
  result.counters.critical_shell_support_check_count = 1U;
  result.critical_shell_is_positive_minimal_support =
      critical_shell_is_positive_minimal(
          result.critical_shell_miniball,
          result.critical_shell_point_ids);
  if (!result.critical_shell_is_positive_minimal_support) {
    finish_unsupported_source(
        result,
        ExactCriticalArmSourceDecision::
            unsupported_nonminimal_or_nonpositive_shell);
    return result;
  }

  result.global_closed_ball.emplace(spatial::brute_force_closed_ball(
      cloud,
      result.critical_shell_miniball.center,
      result.critical_shell_miniball.squared_radius));
  result.counters.global_closed_ball_query_count = 1U;
  result.counters.global_closed_ball_distance_evaluation_count =
      result.global_closed_ball->distance_evaluation_count();
  result.counters.global_shell_identity_check_count = 1U;
  result.global_shell_matches_critical_shell = same_point_ids(
      result.global_closed_ball->shell_ids(),
      result.critical_shell_point_ids);
  result.closed_rank = result.global_closed_ball->closed_rank();
  result.order = result.closed_rank == 0U
                     ? 0U
                     : result.closed_rank - 1U;
  if (!result.global_shell_matches_critical_shell) {
    finish_unsupported_source(
        result,
        ExactCriticalArmSourceDecision::
            unsupported_incomplete_global_shell);
    return result;
  }

  result.counters.closed_rank_order_check_count = 1U;
  result.closed_rank_and_order_supported =
      result.closed_rank >= 2U &&
      result.closed_rank <=
          ExactCriticalArmInitialSegmentResult::
              maximum_supported_closed_rank &&
      result.order >= 1U &&
      result.order <=
          ExactCriticalArmInitialSegmentResult::maximum_supported_order &&
      result.order + 1U == result.closed_rank;
  if (!result.closed_rank_and_order_supported) {
    finish_unsupported_source(
        result,
        ExactCriticalArmSourceDecision::
            unsupported_closed_rank_or_order);
    return result;
  }

  result.critical_source_certified = true;
  result.source_decision =
      ExactCriticalArmSourceDecision::critical_source_certified;

  result.arm_facet_point_ids.reserve(result.order);
  result.arm_facet_point_ids.insert(
      result.arm_facet_point_ids.end(),
      result.global_closed_ball->interior_ids().begin(),
      result.global_closed_ball->interior_ids().end());
  result.arm_facet_point_ids.insert(
      result.arm_facet_point_ids.end(),
      result.global_closed_ball->shell_ids().begin(),
      result.global_closed_ball->shell_ids().end());
  std::sort(
      result.arm_facet_point_ids.begin(),
      result.arm_facet_point_ids.end());
  const auto removed = std::lower_bound(
      result.arm_facet_point_ids.begin(),
      result.arm_facet_point_ids.end(),
      removed_shell_point_id);
  if (removed == result.arm_facet_point_ids.end() ||
      *removed != removed_shell_point_id) {
    throw std::logic_error(
        "a certified critical source lost its removed shell point");
  }
  result.arm_facet_point_ids.erase(removed);
  result.counters.arm_facet_construction_count = 1U;
  result.arm_facet_cardinality_certified =
      result.arm_facet_point_ids.size() == result.order;
  if (!result.arm_facet_cardinality_certified) {
    throw std::logic_error(
        "an index-one arm facet has the wrong cardinality");
  }

  result.arm_miniball.emplace(build_exact_facet_miniball(
      cloud, result.arm_facet_point_ids));
  result.counters.arm_miniball_build_count = 1U;

  const exact::ExactRational3& critical_center =
      result.critical_shell_miniball.center;
  const exact::ExactLevel& critical_level =
      result.critical_shell_miniball.squared_radius;
  const exact::ExactRational3& arm_center =
      result.arm_miniball->center;
  const exact::ExactLevel& arm_level =
      result.arm_miniball->squared_radius;

  exact::ExactLevel source_atom_level = exact_squared_distance(
      critical_center,
      cloud.point(result.arm_facet_point_ids.front()).exact());
  result.counters.arm_source_distance_evaluation_count = 1U;
  for (std::size_t index = 1U;
       index < result.arm_facet_point_ids.size();
       ++index) {
    const exact::ExactLevel point_level = exact_squared_distance(
        critical_center,
        cloud.point(result.arm_facet_point_ids[index]).exact());
    ++result.counters.arm_source_distance_evaluation_count;
    if (point_level > source_atom_level) {
      source_atom_level = point_level;
    }
  }

  const exact::ExactLevel center_displacement =
      exact_squared_distance(critical_center, arm_center);
  result.counters.center_displacement_evaluation_count = 1U;
  result.counters.exact_level_relation_count = 3U;
  if (source_atom_level != critical_level) {
    throw std::logic_error(
        "an index-one arm does not meet the critical level at its source");
  }
  result.arm_miniball_strict_decrease_certified =
      arm_level < critical_level;
  result.positive_center_displacement_certified =
      center_displacement > exact::ExactLevel{};
  if (!result.arm_miniball_strict_decrease_certified ||
      !result.positive_center_displacement_certified) {
    throw std::logic_error(
        "a certified critical arm did not strictly lower its miniball");
  }

  ExactFacetDescentSegmentWitness coefficients;
  coefficients.source_atom_level = source_atom_level;
  coefficients.successor_atom_level = arm_level;
  coefficients.center_squared_displacement = center_displacement;
  coefficients.centers_equal = false;
  coefficients.source_endpoint_strict_sublevel = false;
  coefficients.quadratic_max_upper_bound_certified = true;
  coefficients.closed_segment_nonstrict_sublevel = true;
  coefficients.half_open_segment_strict_sublevel = true;
  result.initial_segment_coefficients.emplace(std::move(coefficients));
  result.counters.convex_identity_certification_count = 1U;
  result.closed_initial_segment_nonstrict_critical_sublevel = true;
  result.half_open_initial_segment_strict_critical_sublevel = true;

  result.removed_point_target_squared_distance.emplace(
      exact_squared_distance(
          arm_center, cloud.point(removed_shell_point_id).exact()));
  result.counters.removed_point_target_distance_evaluation_count = 1U;
  result.removed_point_target_outside_arm_ball_certified =
      *result.removed_point_target_squared_distance > arm_level;
  ++result.counters.exact_level_relation_count;

  result.removed_point_outgoing_linear_coefficient.emplace(
      linear_distance_coefficient(
          critical_center,
          arm_center,
          cloud.point(removed_shell_point_id).exact()));
  result.counters.
      removed_point_directional_coefficient_evaluation_count = 1U;
  result.removed_point_outgoing_direction_certified =
      result.removed_point_outgoing_linear_coefficient->sign() > 0;
  if (!result.removed_point_target_outside_arm_ball_certified ||
      !result.removed_point_outgoing_direction_certified) {
    throw std::logic_error(
        "a certified critical arm did not leave through its removed point");
  }

  exact::ExactRational parameter_bound{exact::BigInt{1}};
  result.counters.parameter_bound_candidate_count = 1U;
  for (const PointId point_id :
       result.global_closed_ball->exterior_ids()) {
    const exact::ExactLevel point_level = exact_squared_distance(
        critical_center, cloud.point(point_id).exact());
    ++result.counters.exterior_point_clearance_evaluation_count;
    if (point_level <= critical_level) {
      throw std::logic_error(
          "a globally exterior point lacks strict source clearance");
    }
    const exact::ExactLevel clearance{
        point_level.rational() - critical_level.rational()};
    const exact::ExactRational linear_coefficient =
        linear_distance_coefficient(
            critical_center,
            arm_center,
            cloud.point(point_id).exact());
    ++result.counters.
        exterior_point_directional_dot_product_evaluation_count;
    if (linear_coefficient.sign() < 0) {
      const exact::ExactRational denominator =
          exact::ExactRational{exact::BigInt{-2}} *
          linear_coefficient;
      const exact::ExactRational candidate =
          clearance.rational() / denominator;
      if (candidate.sign() <= 0) {
        throw std::logic_error(
            "an exterior germ constraint has a nonpositive bound");
      }
      result.negative_exterior_direction_constraints.push_back(
          ExactCriticalArmExteriorConstraintWitness{
              point_id,
              clearance,
              linear_coefficient,
              candidate});
      ++result.counters.negative_exterior_direction_constraint_count;
      ++result.counters.parameter_bound_candidate_count;
      ++result.counters.parameter_bound_minimum_comparison_count;
      if (candidate < parameter_bound) {
        parameter_bound = candidate;
      }
    }
  }
  result.strict_local_parameter_upper_bound.emplace(
      std::move(parameter_bound));
  result.exterior_prefix_bound_certified =
      result.strict_local_parameter_upper_bound->sign() > 0 &&
      *result.strict_local_parameter_upper_bound <=
          exact::ExactRational{exact::BigInt{1}};
  if (!result.exterior_prefix_bound_certified) {
    throw std::logic_error(
        "an exact critical arm has no positive local germ prefix");
  }

  result.decision = ExactCriticalArmInitialSegmentDecision::
      strict_initial_arm_segment_certified;
  result.scope = ExactCriticalArmInitialSegmentScope::
      single_index_one_critical_arm_initial_germ_segment_only;
  return result;
}

[[nodiscard]] bool same_optional_segment_coefficients(
    const std::optional<ExactFacetDescentSegmentWitness>& observed,
    const std::optional<ExactFacetDescentSegmentWitness>& expected) {
  return observed.has_value() == expected.has_value() &&
         (!expected.has_value() ||
          same_segment_coefficients(*observed, *expected));
}

[[nodiscard]] bool same_initial_segment_result(
    const spatial::CanonicalPointCloud& cloud,
    const ExactCriticalArmInitialSegmentResult& observed,
    const ExactCriticalArmInitialSegmentResult& expected) {
  if (observed.critical_shell_point_ids !=
          expected.critical_shell_point_ids ||
      observed.removed_shell_point_id !=
          expected.removed_shell_point_id ||
      observed.global_closed_ball.has_value() !=
          expected.global_closed_ball.has_value() ||
      observed.closed_rank != expected.closed_rank ||
      observed.order != expected.order ||
      observed.arm_facet_point_ids != expected.arm_facet_point_ids ||
      observed.arm_miniball.has_value() !=
          expected.arm_miniball.has_value() ||
      !same_optional_segment_coefficients(
          observed.initial_segment_coefficients,
          expected.initial_segment_coefficients) ||
      observed.removed_point_target_squared_distance !=
          expected.removed_point_target_squared_distance ||
      observed.removed_point_outgoing_linear_coefficient !=
          expected.removed_point_outgoing_linear_coefficient ||
      observed.negative_exterior_direction_constraints !=
          expected.negative_exterior_direction_constraints ||
      observed.strict_local_parameter_upper_bound !=
          expected.strict_local_parameter_upper_bound ||
      observed.critical_shell_is_positive_minimal_support !=
          expected.critical_shell_is_positive_minimal_support ||
      observed.global_shell_matches_critical_shell !=
          expected.global_shell_matches_critical_shell ||
      observed.closed_rank_and_order_supported !=
          expected.closed_rank_and_order_supported ||
      observed.critical_source_certified !=
          expected.critical_source_certified ||
      observed.arm_facet_cardinality_certified !=
          expected.arm_facet_cardinality_certified ||
      observed.arm_miniball_strict_decrease_certified !=
          expected.arm_miniball_strict_decrease_certified ||
      observed.positive_center_displacement_certified !=
          expected.positive_center_displacement_certified ||
      observed.removed_point_outgoing_direction_certified !=
          expected.removed_point_outgoing_direction_certified ||
      observed.removed_point_target_outside_arm_ball_certified !=
          expected.removed_point_target_outside_arm_ball_certified ||
      observed.exterior_prefix_bound_certified !=
          expected.exterior_prefix_bound_certified ||
      observed.closed_initial_segment_nonstrict_critical_sublevel !=
          expected.closed_initial_segment_nonstrict_critical_sublevel ||
      observed.half_open_initial_segment_strict_critical_sublevel !=
          expected.half_open_initial_segment_strict_critical_sublevel ||
      observed.counters != expected.counters ||
      observed.source_decision != expected.source_decision ||
      observed.decision != expected.decision ||
      observed.scope != expected.scope) {
    return false;
  }
  if (expected.global_closed_ball.has_value() &&
      !same_closed_ball_partition(
          cloud,
          *observed.global_closed_ball,
          *expected.global_closed_ball)) {
    return false;
  }
  const auto critical_miniball_verification =
      verify_exact_facet_miniball(
          cloud,
          expected.critical_shell_point_ids,
          observed.critical_shell_miniball);
  if (!critical_miniball_verification.
          local_exact_facet_miniball_certified) {
    return false;
  }
  if (expected.arm_miniball.has_value()) {
    const auto arm_miniball_verification =
        verify_exact_facet_miniball(
            cloud,
            expected.arm_facet_point_ids,
            *observed.arm_miniball);
    if (!arm_miniball_verification.
            local_exact_facet_miniball_certified) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] ExactCriticalArmDescentResult
compute_exact_critical_arm_descent(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> critical_shell_point_ids,
    PointId removed_shell_point_id,
    ExactFacetDescentChainBudget chain_budget) {
  ExactCriticalArmDescentResult result;
  result.requested_chain_budget = chain_budget;
  if (chain_budget.maximum_committed_strict_segment_count >
      ExactFacetDescentChainBudget::
          maximum_supported_committed_strict_segment_count) {
    throw std::invalid_argument(
        "an exact critical-arm chain budget exceeds its reference cap");
  }
  result.initial_segment =
      compute_exact_critical_arm_initial_segment(
          cloud,
          critical_shell_point_ids,
          removed_shell_point_id);
  result.counters.initial_segment_probe_count = 1U;

  if (result.initial_segment.decision !=
      ExactCriticalArmInitialSegmentDecision::
          strict_initial_arm_segment_certified) {
    result.counters.source_unsupported_terminal_count = 1U;
    result.decision = ExactCriticalArmDescentDecision::
        no_descent_unsupported_critical_source;
    result.scope = ExactCriticalArmDescentScope::
        single_index_one_critical_arm_plus_canonical_strict_chain_only;
    return result;
  }

  result.counters.certified_initial_segment_count = 1U;
  result.initial_segment_excluded_from_chain_budget = true;
  result.facet_descent_chain.emplace(
      build_exact_facet_descent_chain(
          cloud,
          result.initial_segment.arm_facet_point_ids,
          chain_budget));
  result.counters.facet_chain_build_count = 1U;
  result.counters.initial_to_chain_seam_replay_count = 1U;
  if (!result.initial_segment.arm_miniball.has_value() ||
      result.facet_descent_chain->nodes.empty()) {
    throw std::logic_error(
        "a certified critical arm lacks its chain seam payload");
  }
  const ExactFacetDescentChainNodeWitness& first_node =
      result.facet_descent_chain->nodes.front();
  result.exact_initial_to_chain_seam_certified =
      first_node.facet_point_ids ==
          result.initial_segment.arm_facet_point_ids &&
      first_node.center ==
          result.initial_segment.arm_miniball->center &&
      first_node.squared_level ==
          result.initial_segment.arm_miniball->squared_radius;
  result.source_open_composite_path_strict_critical_sublevel =
      result.exact_initial_to_chain_seam_certified &&
      result.initial_segment.
          half_open_initial_segment_strict_critical_sublevel &&
      result.facet_descent_chain->
          closed_polyline_nonstrict_initial_sublevel &&
      result.initial_segment.arm_miniball->squared_radius <
          result.initial_segment.
              critical_shell_miniball.squared_radius;
  if (!result.exact_initial_to_chain_seam_certified ||
      !result.source_open_composite_path_strict_critical_sublevel) {
    throw std::logic_error(
        "a certified critical arm failed its exact 6.5 seam");
  }

  result.counters.committed_chain_strict_segment_count =
      result.facet_descent_chain->
          committed_segment_witnesses.size();
  result.counters.committed_composite_path_segment_count =
      result.counters.committed_chain_strict_segment_count + 1U;
  switch (result.facet_descent_chain->decision) {
    case ExactFacetDescentChainDecision::
        complete_at_regular_active_facet:
      result.counters.active_terminal_count = 1U;
      result.decision = ExactCriticalArmDescentDecision::
          complete_at_regular_active_facet;
      break;
    case ExactFacetDescentChainDecision::
        certified_prefix_blocked_unsupported_degeneracy:
      result.counters.unsupported_chain_terminal_count = 1U;
      result.decision = ExactCriticalArmDescentDecision::
          certified_prefix_blocked_unsupported_degeneracy;
      break;
    case ExactFacetDescentChainDecision::
        certified_prefix_strict_segment_budget_exhausted:
      result.counters.chain_budget_terminal_count = 1U;
      result.decision = ExactCriticalArmDescentDecision::
          certified_prefix_strict_segment_budget_exhausted;
      break;
    case ExactFacetDescentChainDecision::not_certified:
      throw std::logic_error(
          "a certified critical arm received an uncertified 6.5 chain");
  }
  result.scope = ExactCriticalArmDescentScope::
      single_index_one_critical_arm_plus_canonical_strict_chain_only;
  return result;
}

}  // namespace

ExactCriticalArmInitialSegmentVerification
verify_exact_critical_arm_initial_segment(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> critical_shell_point_ids,
    PointId removed_shell_point_id,
    const ExactCriticalArmInitialSegmentResult& result) {
  const ExactCriticalArmInitialSegmentResult expected =
      compute_exact_critical_arm_initial_segment(
          cloud,
          critical_shell_point_ids,
          removed_shell_point_id);
  ExactCriticalArmInitialSegmentVerification verification;
  verification.input_shell_identity_certified =
      result.critical_shell_point_ids ==
          expected.critical_shell_point_ids;
  verification.removed_shell_point_identity_certified =
      result.removed_shell_point_id == removed_shell_point_id &&
      result.removed_shell_point_id ==
          expected.removed_shell_point_id;
  const auto critical_miniball_verification =
      verify_exact_facet_miniball(
          cloud,
          expected.critical_shell_point_ids,
          result.critical_shell_miniball);
  verification.critical_shell_miniball_certified =
      critical_miniball_verification.
          local_exact_facet_miniball_certified;
  verification.global_closed_ball_presence_certified =
      result.global_closed_ball.has_value() ==
          expected.global_closed_ball.has_value();
  verification.global_closed_ball_certified =
      verification.global_closed_ball_presence_certified &&
      (!expected.global_closed_ball.has_value() ||
       same_closed_ball_partition(
           cloud,
           *result.global_closed_ball,
           *expected.global_closed_ball));
  verification.source_facts_certified =
      result.closed_rank == expected.closed_rank &&
      result.order == expected.order &&
      result.critical_shell_is_positive_minimal_support ==
          expected.critical_shell_is_positive_minimal_support &&
      result.global_shell_matches_critical_shell ==
          expected.global_shell_matches_critical_shell &&
      result.closed_rank_and_order_supported ==
          expected.closed_rank_and_order_supported &&
      result.critical_source_certified ==
          expected.critical_source_certified;
  verification.source_decision_certified =
      result.source_decision == expected.source_decision;

  verification.arm_payload_presence_certified =
      result.arm_miniball.has_value() ==
          expected.arm_miniball.has_value() &&
      result.initial_segment_coefficients.has_value() ==
          expected.initial_segment_coefficients.has_value() &&
      result.removed_point_target_squared_distance.has_value() ==
          expected.removed_point_target_squared_distance.has_value() &&
      result.removed_point_outgoing_linear_coefficient.has_value() ==
          expected.removed_point_outgoing_linear_coefficient.has_value() &&
      result.strict_local_parameter_upper_bound.has_value() ==
          expected.strict_local_parameter_upper_bound.has_value();
  verification.arm_facet_certified =
      result.arm_facet_point_ids == expected.arm_facet_point_ids;
  verification.arm_miniball_certified =
      result.arm_miniball.has_value() ==
          expected.arm_miniball.has_value();
  if (verification.arm_miniball_certified &&
      expected.arm_miniball.has_value()) {
    const auto arm_miniball_verification =
        verify_exact_facet_miniball(
            cloud,
            expected.arm_facet_point_ids,
            *result.arm_miniball);
    verification.arm_miniball_certified =
        arm_miniball_verification.
            local_exact_facet_miniball_certified;
  }
  verification.analytic_segment_coefficients_certified =
      same_optional_segment_coefficients(
          result.initial_segment_coefficients,
          expected.initial_segment_coefficients);
  verification.removed_point_witnesses_certified =
      result.removed_point_target_squared_distance ==
          expected.removed_point_target_squared_distance &&
      result.removed_point_outgoing_linear_coefficient ==
          expected.removed_point_outgoing_linear_coefficient;
  verification.exterior_constraint_witnesses_certified =
      result.negative_exterior_direction_constraints ==
          expected.negative_exterior_direction_constraints;
  verification.strict_local_parameter_bound_certified =
      result.strict_local_parameter_upper_bound ==
          expected.strict_local_parameter_upper_bound;
  verification.strict_arm_consequences_certified =
      result.arm_facet_cardinality_certified ==
          expected.arm_facet_cardinality_certified &&
      result.arm_miniball_strict_decrease_certified ==
          expected.arm_miniball_strict_decrease_certified &&
      result.positive_center_displacement_certified ==
          expected.positive_center_displacement_certified &&
      result.removed_point_outgoing_direction_certified ==
          expected.removed_point_outgoing_direction_certified &&
      result.removed_point_target_outside_arm_ball_certified ==
          expected.removed_point_target_outside_arm_ball_certified &&
      result.exterior_prefix_bound_certified ==
          expected.exterior_prefix_bound_certified &&
      result.closed_initial_segment_nonstrict_critical_sublevel ==
          expected.closed_initial_segment_nonstrict_critical_sublevel &&
      result.half_open_initial_segment_strict_critical_sublevel ==
          expected.half_open_initial_segment_strict_critical_sublevel;
  verification.counters_certified =
      result.counters == expected.counters;
  verification.decision_certified =
      result.decision == expected.decision;
  verification.scope_certified =
      result.scope == ExactCriticalArmInitialSegmentScope::
                          single_index_one_critical_arm_initial_germ_segment_only;
  verification.fresh_replay_certified =
      same_initial_segment_result(cloud, result, expected);
  verification.exact_critical_arm_initial_segment_decision_certified =
      verification.input_shell_identity_certified &&
      verification.removed_shell_point_identity_certified &&
      verification.critical_shell_miniball_certified &&
      verification.global_closed_ball_presence_certified &&
      verification.global_closed_ball_certified &&
      verification.source_facts_certified &&
      verification.source_decision_certified &&
      verification.arm_payload_presence_certified &&
      verification.arm_facet_certified &&
      verification.arm_miniball_certified &&
      verification.analytic_segment_coefficients_certified &&
      verification.removed_point_witnesses_certified &&
      verification.exterior_constraint_witnesses_certified &&
      verification.strict_local_parameter_bound_certified &&
      verification.strict_arm_consequences_certified &&
      verification.counters_certified &&
      verification.decision_certified &&
      verification.scope_certified &&
      verification.fresh_replay_certified;
  return verification;
}

ExactCriticalArmInitialSegmentResult
build_exact_critical_arm_initial_segment(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> critical_shell_point_ids,
    PointId removed_shell_point_id) {
  ExactCriticalArmInitialSegmentResult result =
      compute_exact_critical_arm_initial_segment(
          cloud,
          critical_shell_point_ids,
          removed_shell_point_id);
  const auto verification =
      verify_exact_critical_arm_initial_segment(
          cloud,
          critical_shell_point_ids,
          removed_shell_point_id,
          result);
  if (!verification.
          exact_critical_arm_initial_segment_decision_certified) {
    throw std::logic_error(
        "the exact critical-arm initial segment failed its fresh replay");
  }
  return result;
}

ExactCriticalArmDescentVerification verify_exact_critical_arm_descent(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> critical_shell_point_ids,
    PointId removed_shell_point_id,
    ExactFacetDescentChainBudget chain_budget,
    const ExactCriticalArmDescentResult& result) {
  const ExactCriticalArmDescentResult expected =
      compute_exact_critical_arm_descent(
          cloud,
          critical_shell_point_ids,
          removed_shell_point_id,
          chain_budget);
  ExactCriticalArmDescentVerification verification;
  verification.requested_chain_budget_certified =
      result.requested_chain_budget == chain_budget &&
      result.requested_chain_budget ==
          expected.requested_chain_budget;
  const auto initial_verification =
      verify_exact_critical_arm_initial_segment(
          cloud,
          critical_shell_point_ids,
          removed_shell_point_id,
          result.initial_segment);
  verification.initial_segment_certified =
      initial_verification.
          exact_critical_arm_initial_segment_decision_certified;
  verification.facet_chain_presence_certified =
      result.facet_descent_chain.has_value() ==
          expected.facet_descent_chain.has_value();
  verification.facet_chain_certified =
      verification.facet_chain_presence_certified;
  if (verification.facet_chain_certified &&
      expected.facet_descent_chain.has_value()) {
    const auto chain_verification =
        verify_exact_facet_descent_chain(
            cloud,
            expected.initial_segment.arm_facet_point_ids,
            chain_budget,
            *result.facet_descent_chain);
    verification.facet_chain_certified =
        chain_verification.
            exact_descent_chain_decision_certified;
  }
  verification.initial_segment_budget_separation_certified =
      result.initial_segment_excluded_from_chain_budget ==
          expected.initial_segment_excluded_from_chain_budget;
  verification.exact_initial_to_chain_seam_certified =
      result.exact_initial_to_chain_seam_certified ==
          expected.exact_initial_to_chain_seam_certified;
  verification.source_open_composite_path_certified =
      result.source_open_composite_path_strict_critical_sublevel ==
          expected.source_open_composite_path_strict_critical_sublevel;
  verification.counters_certified =
      result.counters == expected.counters;
  verification.decision_mapping_certified =
      result.decision == expected.decision;
  verification.scope_certified =
      result.scope == ExactCriticalArmDescentScope::
                          single_index_one_critical_arm_plus_canonical_strict_chain_only;
  verification.fresh_replay_certified =
      result.requested_chain_budget ==
          expected.requested_chain_budget &&
      same_initial_segment_result(
          cloud, result.initial_segment, expected.initial_segment) &&
      verification.facet_chain_certified &&
      verification.initial_segment_budget_separation_certified &&
      verification.exact_initial_to_chain_seam_certified &&
      verification.source_open_composite_path_certified &&
      verification.counters_certified &&
      verification.decision_mapping_certified &&
      result.scope == expected.scope;
  verification.exact_critical_arm_descent_decision_certified =
      verification.requested_chain_budget_certified &&
      verification.initial_segment_certified &&
      verification.facet_chain_presence_certified &&
      verification.facet_chain_certified &&
      verification.initial_segment_budget_separation_certified &&
      verification.exact_initial_to_chain_seam_certified &&
      verification.source_open_composite_path_certified &&
      verification.counters_certified &&
      verification.decision_mapping_certified &&
      verification.scope_certified &&
      verification.fresh_replay_certified;
  return verification;
}

ExactCriticalArmDescentResult build_exact_critical_arm_descent(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> critical_shell_point_ids,
    PointId removed_shell_point_id,
    ExactFacetDescentChainBudget chain_budget) {
  ExactCriticalArmDescentResult result =
      compute_exact_critical_arm_descent(
          cloud,
          critical_shell_point_ids,
          removed_shell_point_id,
          chain_budget);
  const auto verification = verify_exact_critical_arm_descent(
      cloud,
      critical_shell_point_ids,
      removed_shell_point_id,
      chain_budget,
      result);
  if (!verification.exact_critical_arm_descent_decision_certified) {
    throw std::logic_error(
        "the exact critical-arm descent failed its fresh replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
