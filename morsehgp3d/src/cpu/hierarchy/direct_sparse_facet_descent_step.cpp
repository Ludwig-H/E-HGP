#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_step.hpp"

#include "direct_sparse_facet_descent_step_detail.hpp"

#include "morsehgp3d/hierarchy/facet_miniball.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

void require_valid_traversal_order(
    spatial::LbvhTraversalOrder traversal_order) {
  switch (traversal_order) {
    case spatial::LbvhTraversalOrder::near_first:
    case spatial::LbvhTraversalOrder::far_first:
      return;
  }
  throw std::invalid_argument("an LBVH traversal order is invalid");
}

[[nodiscard]] exact::ExactLevel exact_squared_distance(
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

[[nodiscard]] ExactDirectSparseFacetKey canonical_facet_key(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> point_ids) {
  if (point_ids.empty() ||
      point_ids.size() > direct_sparse_positive_facet_maximum_point_count) {
    throw std::invalid_argument(
        "a direct sparse descent facet must contain between one and ten points");
  }
  ExactDirectSparseFacetKey key;
  key.point_count = point_ids.size();
  for (std::size_t index = 0U; index < point_ids.size(); ++index) {
    const PointId point_id = point_ids[index];
    if (point_id >= static_cast<PointId>(cloud.size())) {
      throw std::out_of_range(
          "a direct sparse descent facet contains an unknown PointId");
    }
    if (index != 0U && point_ids[index - 1U] >= point_id) {
      throw std::invalid_argument(
          "a direct sparse descent facet key must be strictly increasing");
    }
    key.point_ids[index] = point_id;
  }
  return key;
}

[[nodiscard]] std::span<const PointId> used_point_ids(
    const ExactDirectSparseFacetKey& key) {
  return std::span<const PointId>{key.point_ids}.first(key.point_count);
}

[[nodiscard]] bool key_matches_ids(
    const ExactDirectSparseFacetKey& key,
    std::span<const PointId> point_ids) {
  return key.point_count == point_ids.size() &&
         std::equal(
             point_ids.begin(),
             point_ids.end(),
             key.point_ids.begin());
}

[[nodiscard]] bool certified_local_miniball(
    const spatial::CanonicalPointCloud& cloud,
    const ExactFacetMiniballResult& miniball,
    const ExactDirectSparseFacetKey& key) {
  if (miniball.status !=
          ExactFacetMiniballStatus::exact_facet_miniball_certified ||
      miniball.scope !=
          ExactFacetMiniballScope::local_facet_miniball_only ||
      !key_matches_ids(key, miniball.facet_point_ids) ||
      miniball.counters.facet_point_count != key.point_count ||
      miniball.counters.enumerated_support_count >
          ExactFacetMiniballResult::maximum_enumerated_support_count) {
    return false;
  }

  const std::span<const PointId> point_ids = used_point_ids(key);
  exact::ExactLevel maximum_level = exact_squared_distance(
      miniball.center, cloud.point(point_ids.front()).exact());
  for (std::size_t index = 1U; index < point_ids.size(); ++index) {
    const exact::ExactLevel point_level = exact_squared_distance(
        miniball.center, cloud.point(point_ids[index]).exact());
    if (point_level > maximum_level) {
      maximum_level = point_level;
    }
  }
  return maximum_level == miniball.squared_radius;
}

[[nodiscard]] bool source_miniball_acquisition_certified(
    const ExactDirectSparseFacetDescentStepResult& result) noexcept {
  const bool exactly_one_acquisition =
      (result.counters.source_miniball_build_count == 1U &&
       result.counters.source_miniball_reuse_count == 0U) ||
      (result.counters.source_miniball_build_count == 0U &&
       result.counters.source_miniball_reuse_count == 1U);
  return exactly_one_acquisition &&
         result.source_miniball_freshly_certified ==
             (result.counters.source_miniball_build_count == 1U) &&
         result.source_miniball_reused_from_certified_input ==
             (result.counters.source_miniball_reuse_count == 1U);
}

[[nodiscard]] bool successor_miniball_acquisition_certified(
    const ExactDirectSparseFacetDescentStepResult& result) noexcept {
  const bool exactly_one_acquisition =
      (result.counters.successor_miniball_build_count == 1U &&
       result.counters.successor_miniball_reuse_count == 0U) ||
      (result.counters.successor_miniball_build_count == 0U &&
       result.counters.successor_miniball_reuse_count == 1U);
  return exactly_one_acquisition &&
         result.successor_miniball_freshly_certified ==
             (result.counters.successor_miniball_build_count == 1U) &&
         result.successor_miniball_reused_from_certified_lookup ==
             (result.counters.successor_miniball_reuse_count == 1U);
}

[[nodiscard]] bool no_source_miniball_acquisition(
    const ExactDirectSparseFacetDescentStepResult& result) noexcept {
  return result.counters.source_miniball_build_count == 0U &&
         result.counters.source_miniball_reuse_count == 0U &&
         !result.source_miniball_freshly_certified &&
         !result.source_miniball_reused_from_certified_input;
}

[[nodiscard]] bool no_successor_miniball_acquisition(
    const ExactDirectSparseFacetDescentStepResult& result) noexcept {
  return result.counters.successor_miniball_build_count == 0U &&
         result.counters.successor_miniball_reuse_count == 0U &&
         !result.successor_miniball_freshly_certified &&
         !result.successor_miniball_reused_from_certified_lookup;
}

void initialize_closed_scope(
    ExactDirectSparseFacetDescentStepResult& result) {
  result.locator_state_mutated = false;
  result.locator_batch_committed = false;
  result.external_binding_authority_replayed = false;
  result.missing_facet_means_isolated = false;
  result.singleton_component_created = false;
  result.global_closed_ball_materialized = false;
  result.forbidden_global_structure_materialized = false;
  result.hierarchy_attachment_published = false;
  result.public_status_claimed = false;
  result.partial_refinement_only = true;
  result.scope = ExactDirectSparseFacetDescentStepScope::
      single_strict_top_k_step_and_caller_authority_relative_positive_hit_only;
}

[[nodiscard]] bool common_result_facts_certified(
    const ExactDirectSparseFacetDescentStepResult& result) noexcept {
  return result.schema_version ==
             direct_sparse_facet_descent_step_schema_version &&
         result.input_shape_certified &&
         result.source_probe_used_const_pre_call_locator &&
         result.counters.source_locator_probe_count == 1U &&
         result.source_locator_probe.query_key == result.source_facet_key &&
         result.source_locator_probe.query_witness ==
             result.locator_query_witness &&
         result.source_locator_probe.budget ==
             result.requested_budget.source_locator_probe &&
         !result.locator_state_mutated &&
         !result.locator_batch_committed &&
         !result.external_binding_authority_replayed &&
         !result.missing_facet_means_isolated &&
         !result.singleton_component_created &&
         !result.global_closed_ball_materialized &&
         !result.forbidden_global_structure_materialized &&
         !result.hierarchy_attachment_published &&
         !result.public_status_claimed &&
         result.partial_refinement_only &&
         result.scope == ExactDirectSparseFacetDescentStepScope::
             single_strict_top_k_step_and_caller_authority_relative_positive_hit_only;
}

[[nodiscard]] bool strict_witness_certified(
    const ExactDirectSparseFacetDescentStepResult& result) noexcept {
  if (!result.strict_step_witness.has_value()) {
    return false;
  }
  const ExactDirectSparseFacetDescentStepWitness& witness =
      *result.strict_step_witness;
  return result.successor_locator_probe.has_value() &&
         result.successor_locator_probe->query_key ==
             witness.successor_facet_key &&
         result.successor_locator_probe->query_witness ==
             result.locator_query_witness &&
         result.successor_locator_probe->budget ==
             result.requested_budget.successor_locator_probe &&
         witness.source_facet_key == result.source_facet_key &&
         witness.successor_facet_key.point_count ==
             witness.source_facet_key.point_count &&
         witness.successor_facet_key != witness.source_facet_key &&
         witness.successor_at_source_squared_level ==
             witness.top_k_cutoff_squared_level &&
         witness.top_k_cutoff_squared_level <=
             witness.source_facet_squared_level &&
         witness.successor_facet_squared_level <=
             witness.top_k_cutoff_squared_level &&
         witness.successor_facet_squared_level <
             witness.source_facet_squared_level &&
         witness.source_facet_squared_level <=
             result.closed_batch_squared_level &&
         witness.successor_is_complete_canonical_top_k_choice &&
         witness.successor_differs_from_source &&
         witness.successor_at_source_equals_top_k_cutoff &&
         witness.top_k_cutoff_at_most_source_level &&
         witness.successor_level_at_most_top_k_cutoff &&
         witness.strict_miniball_level_decrease &&
         witness.exact_squared_distance_chord_identity_applies &&
         witness.source_facet_at_or_below_closed_batch_level &&
         witness.source_open_target_closed_segment_strict_below_source_level &&
         witness.source_open_target_closed_segment_strict_below_closed_batch_level &&
         witness.closed_segment_strict_below_closed_batch_level ==
             (witness.top_k_cutoff_squared_level <
              result.closed_batch_squared_level) &&
         source_miniball_acquisition_certified(result) &&
         result.complete_top_k_partition_certified &&
         result.complete_top_k_shell_consumed_transiently &&
         successor_miniball_acquisition_certified(result) &&
         result.exact_level_relations_certified &&
         result.strict_half_open_segment_certified &&
         result.complete_top_k_query_counters.has_value() &&
         result.top_k_audit.traversal_complete &&
         result.top_k_stop_reason == spatial::ExactLbvhTopKStopReason::none &&
         result.counters.top_k_query_count == 1U &&
         result.counters.canonical_successor_selection_count == 1U &&
         result.counters.successor_source_distance_evaluation_count ==
             witness.source_facet_key.point_count &&
         result.counters.successor_source_maximum_comparison_count + 1U ==
             witness.source_facet_key.point_count &&
         result.counters.center_displacement_evaluation_count == 1U &&
         result.counters.exact_level_relation_count == 6U &&
         result.counters.convex_segment_certification_count == 1U;
}

[[nodiscard]] bool no_resolved_payload(
    const ExactDirectSparseFacetDescentStepResult& result) noexcept {
  return !result.resolved_component_handle.has_value() &&
         !result.resolved_binding_witness.has_value();
}

}  // namespace

bool ExactDirectSparseFacetDescentStepResult::
certified_relative_positive_resolution() const noexcept {
  if (!common_result_facts_certified(*this) ||
      disposition !=
          ExactDirectSparseFacetDescentStepDisposition::relative_positive ||
      !resolved_component_handle.has_value() ||
      !resolved_binding_witness.has_value() ||
      resolved_binding_witness->external_authority_id !=
          locator_query_witness.external_authority_id ||
      resolved_binding_witness->replay_token == 0U) {
    return false;
  }
  if (decision == ExactDirectSparseFacetDescentStepDecision::
                      complete_relative_source_positive_hit) {
    return source_locator_probe.certified_positive_hit() &&
           *resolved_component_handle ==
               source_locator_probe.component_handle &&
           *resolved_binding_witness ==
               source_locator_probe.source_binding_witness &&
           !successor_locator_probe.has_value() &&
           !strict_step_witness.has_value() &&
           !complete_top_k_query_counters.has_value() &&
           no_source_miniball_acquisition(*this) &&
           counters.top_k_query_count == 0U &&
           counters.canonical_successor_selection_count == 0U &&
           no_successor_miniball_acquisition(*this) &&
           counters.successor_locator_probe_count == 0U;
  }
  if (decision == ExactDirectSparseFacetDescentStepDecision::
                      complete_relative_strict_successor_positive_hit) {
    return source_locator_probe.certified_unresolved_miss() &&
           strict_witness_certified(*this) &&
           successor_locator_probe.has_value() &&
           successor_locator_probe->certified_positive_hit() &&
           successor_probe_used_same_const_pre_call_locator &&
           counters.successor_locator_probe_count == 1U &&
           *resolved_component_handle ==
               successor_locator_probe->component_handle &&
           *resolved_binding_witness ==
               successor_locator_probe->source_binding_witness;
  }
  return false;
}

bool ExactDirectSparseFacetDescentStepResult::
certified_unresolved_without_isolation() const noexcept {
  if (!common_result_facts_certified(*this) ||
      disposition != ExactDirectSparseFacetDescentStepDisposition::unresolved ||
      !no_resolved_payload(*this) ||
      !source_locator_probe.certified_unresolved_miss()) {
    return false;
  }
  switch (decision) {
    case ExactDirectSparseFacetDescentStepDecision::
        complete_unresolved_source_above_closed_batch_level:
      return source_miniball_acquisition_certified(*this) &&
             counters.top_k_query_count == 0U &&
             top_k_stop_reason == spatial::ExactLbvhTopKStopReason::none &&
             !complete_top_k_query_counters.has_value() &&
             !strict_step_witness.has_value() &&
             !successor_locator_probe.has_value() &&
             no_successor_miniball_acquisition(*this);
    case ExactDirectSparseFacetDescentStepDecision::
        complete_unresolved_source_is_canonical_top_k_choice:
      return source_miniball_acquisition_certified(*this) &&
             complete_top_k_partition_certified &&
             complete_top_k_shell_consumed_transiently &&
             counters.top_k_query_count == 1U &&
             counters.canonical_successor_selection_count == 1U &&
             no_successor_miniball_acquisition(*this) &&
             complete_top_k_query_counters.has_value() &&
             top_k_audit.traversal_complete &&
             top_k_stop_reason == spatial::ExactLbvhTopKStopReason::none &&
             !strict_step_witness.has_value() &&
             !successor_locator_probe.has_value();
    case ExactDirectSparseFacetDescentStepDecision::
        complete_unresolved_non_strict_canonical_successor:
      return source_miniball_acquisition_certified(*this) &&
             complete_top_k_partition_certified &&
             complete_top_k_shell_consumed_transiently &&
             successor_miniball_acquisition_certified(*this) &&
             counters.top_k_query_count == 1U &&
             counters.canonical_successor_selection_count == 1U &&
             complete_top_k_query_counters.has_value() &&
             top_k_audit.traversal_complete &&
             top_k_stop_reason == spatial::ExactLbvhTopKStopReason::none &&
             !strict_step_witness.has_value() &&
             !successor_locator_probe.has_value();
    case ExactDirectSparseFacetDescentStepDecision::
        complete_unresolved_strict_successor_not_bound:
      return strict_witness_certified(*this) &&
             successor_locator_probe.has_value() &&
             successor_locator_probe->certified_unresolved_miss() &&
             successor_probe_used_same_const_pre_call_locator &&
             counters.successor_locator_probe_count == 1U;
    default:
      return false;
  }
}

bool ExactDirectSparseFacetDescentStepResult::certified_budget_exhaustion()
    const noexcept {
  if (!common_result_facts_certified(*this) ||
      disposition !=
          ExactDirectSparseFacetDescentStepDisposition::budget_exhausted ||
      !no_resolved_payload(*this)) {
    return false;
  }
  switch (decision) {
    case ExactDirectSparseFacetDescentStepDecision::
        no_resolution_source_locator_probe_budget_exhausted:
      return source_locator_probe.certified_budget_exhaustion() &&
             no_source_miniball_acquisition(*this) &&
             counters.top_k_query_count == 0U &&
             !complete_top_k_query_counters.has_value() &&
             !strict_step_witness.has_value() &&
             !successor_locator_probe.has_value() &&
             no_successor_miniball_acquisition(*this);
    case ExactDirectSparseFacetDescentStepDecision::
        no_resolution_top_k_budget_exhausted:
      return source_locator_probe.certified_unresolved_miss() &&
             source_miniball_acquisition_certified(*this) &&
             counters.top_k_query_count == 1U &&
             top_k_stop_reason != spatial::ExactLbvhTopKStopReason::none &&
             ((top_k_stop_reason == spatial::ExactLbvhTopKStopReason::
                                         cutoff_shell_entry_limit) ==
              top_k_audit.traversal_complete) &&
             !complete_top_k_query_counters.has_value() &&
             !strict_step_witness.has_value() &&
             !successor_locator_probe.has_value() &&
             no_successor_miniball_acquisition(*this);
    case ExactDirectSparseFacetDescentStepDecision::
        no_resolution_successor_locator_probe_budget_exhausted:
      return source_locator_probe.certified_unresolved_miss() &&
             strict_witness_certified(*this) &&
             successor_locator_probe.has_value() &&
             successor_locator_probe->certified_budget_exhaustion() &&
             successor_probe_used_same_const_pre_call_locator &&
             counters.successor_locator_probe_count == 1U;
    default:
      return false;
  }
}

bool ExactDirectSparseFacetDescentStepResult::
certified_fail_closed_contradiction() const noexcept {
  if (!common_result_facts_certified(*this) ||
      disposition !=
          ExactDirectSparseFacetDescentStepDisposition::contradiction ||
      !no_resolved_payload(*this) ||
      !source_locator_probe.certified_unresolved_miss() ||
      strict_step_witness.has_value() ||
      successor_locator_probe.has_value()) {
    return false;
  }
  switch (decision) {
    case ExactDirectSparseFacetDescentStepDecision::
        contradiction_top_k_cutoff_above_source_miniball:
      return source_miniball_acquisition_certified(*this) &&
             complete_top_k_partition_certified &&
             no_successor_miniball_acquisition(*this);
    case ExactDirectSparseFacetDescentStepDecision::
        contradiction_successor_miniball_above_top_k_cutoff:
      return source_miniball_acquisition_certified(*this) &&
             complete_top_k_partition_certified &&
             successor_miniball_acquisition_certified(*this);
    case ExactDirectSparseFacetDescentStepDecision::
        contradiction_successor_source_atom_cutoff_mismatch:
      return source_miniball_acquisition_certified(*this) &&
             complete_top_k_partition_certified &&
             successor_miniball_acquisition_certified(*this) &&
             counters.successor_source_distance_evaluation_count ==
                 source_facet_key.point_count;
    default:
      return false;
  }
}

bool ExactDirectSparseFacetDescentStepResult::
certified_partial_refinement_outcome() const noexcept {
  return certified_relative_positive_resolution() ||
         certified_unresolved_without_isolation() ||
         certified_budget_exhaustion() ||
         certified_fail_closed_contradiction();
}

detail::ExactDirectSparseFacetDescentStepTransient
detail::build_exact_direct_sparse_facet_descent_step_transient(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> source_facet_point_ids,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentStepBudget& budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactFacetMiniballResult* certified_source_miniball,
    ExactDirectSparseCertifiedFacetMiniballLookup
        certified_miniball_lookup) {
  require_valid_traversal_order(traversal_order);
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "a direct sparse descent step requires a matching LBVH authority");
  }
  if (!locator.certified_positive_locator()) {
    throw std::invalid_argument(
        "a direct sparse descent step requires a certified positive locator");
  }
  if (locator_query_witness.external_authority_id == 0U ||
      locator_query_witness.external_authority_id !=
          locator.config().external_authority_id ||
      locator_query_witness.replay_token == 0U) {
    throw std::invalid_argument(
        "a direct sparse descent step requires a matching non-null locator witness");
  }

  ExactDirectSparseFacetDescentStepTransient transient;
  ExactDirectSparseFacetDescentStepResult& result = transient.result;
  result.requested_budget = budget;
  result.traversal_order = traversal_order;
  result.closed_batch_squared_level = closed_batch_squared_level;
  result.source_facet_key =
      canonical_facet_key(cloud, source_facet_point_ids);
  result.locator_query_witness = locator_query_witness;
  result.input_shape_certified = true;
  initialize_closed_scope(result);

  result.source_locator_probe = locator.probe_positive_facet(
      result.source_facet_key,
      locator_query_witness,
      budget.source_locator_probe);
  result.counters.source_locator_probe_count = 1U;
  result.source_probe_used_const_pre_call_locator = true;
  if (result.source_locator_probe.certified_budget_exhaustion()) {
    result.disposition =
        ExactDirectSparseFacetDescentStepDisposition::budget_exhausted;
    result.decision = ExactDirectSparseFacetDescentStepDecision::
        no_resolution_source_locator_probe_budget_exhausted;
    return transient;
  }
  if (result.source_locator_probe.certified_positive_hit()) {
    result.resolved_component_handle =
        result.source_locator_probe.component_handle;
    result.resolved_binding_witness =
        result.source_locator_probe.source_binding_witness;
    result.disposition =
        ExactDirectSparseFacetDescentStepDisposition::relative_positive;
    result.decision = ExactDirectSparseFacetDescentStepDecision::
        complete_relative_source_positive_hit;
    return transient;
  }
  if (!result.source_locator_probe.certified_unresolved_miss()) {
    throw std::logic_error(
        "a valid const source probe returned no certified disposition");
  }

  const ExactFacetMiniballResult* source_miniball_pointer =
      certified_source_miniball;
  if (source_miniball_pointer == nullptr) {
    transient.newly_built_source_miniball.emplace(
        build_exact_facet_miniball(
            cloud, used_point_ids(result.source_facet_key)));
    source_miniball_pointer =
        &*transient.newly_built_source_miniball;
    result.counters.source_miniball_build_count = 1U;
    result.source_miniball_freshly_certified = certified_local_miniball(
        cloud, *source_miniball_pointer, result.source_facet_key);
    if (!result.source_miniball_freshly_certified) {
      throw std::logic_error(
          "the exact source-facet miniball did not close locally");
    }
  } else {
    if (!certified_local_miniball(
            cloud, *source_miniball_pointer, result.source_facet_key)) {
      throw std::invalid_argument(
          "a reused source miniball does not match its full key, exact center and exact level");
    }
    result.counters.source_miniball_reuse_count = 1U;
    result.source_miniball_reused_from_certified_input = true;
  }
  const ExactFacetMiniballResult& source_miniball =
      *source_miniball_pointer;
  ++result.counters.exact_level_relation_count;
  if (source_miniball.squared_radius > closed_batch_squared_level) {
    result.disposition =
        ExactDirectSparseFacetDescentStepDisposition::unresolved;
    result.decision = ExactDirectSparseFacetDescentStepDecision::
        complete_unresolved_source_above_closed_batch_level;
    return transient;
  }

  const spatial::ExclusionSet empty_exclusions =
      spatial::ExclusionSet::from_ids(
          std::span<const PointId>{}, cloud, 0U);
  spatial::ExactBudgetedLbvhTopKResult top_k_result =
      spatial::lbvh_top_k_budgeted(
          index,
          cloud,
          source_miniball.center,
          result.source_facet_key.point_count,
          empty_exclusions,
          budget.top_k_query,
          traversal_order);
  result.counters.top_k_query_count = 1U;
  result.top_k_audit = top_k_result.audit();
  result.top_k_stop_reason = top_k_result.stop_reason();
  if (!top_k_result.complete()) {
    result.disposition =
        ExactDirectSparseFacetDescentStepDisposition::budget_exhausted;
    result.decision = ExactDirectSparseFacetDescentStepDecision::
        no_resolution_top_k_budget_exhausted;
    return transient;
  }

  const spatial::TopKPartition& top_k = top_k_result.partition();
  if (!top_k.validated_for(cloud) || !top_k.shell_complete() ||
      top_k.requested_rank() != result.source_facet_key.point_count ||
      top_k.eligible_point_count() != cloud.size() ||
      top_k.query_counters().method !=
          spatial::SpatialQueryMethod::morton_lbvh ||
      top_k.query_counters().excluded_point_count != 0U ||
      !result.top_k_audit.traversal_complete) {
    throw std::logic_error(
        "a complete budgeted top-k query did not close its exact partition");
  }
  result.complete_top_k_query_counters = top_k.query_counters();
  result.complete_top_k_partition_certified = true;
  result.complete_top_k_shell_consumed_transiently = true;
  ++result.counters.exact_level_relation_count;
  if (top_k.cutoff_squared_distance() > source_miniball.squared_radius) {
    result.disposition =
        ExactDirectSparseFacetDescentStepDisposition::contradiction;
    result.decision = ExactDirectSparseFacetDescentStepDecision::
        contradiction_top_k_cutoff_above_source_miniball;
    return transient;
  }

  const ExactDirectSparseFacetKey successor_key =
      canonical_facet_key(cloud, top_k.canonical_choice_ids());
  result.counters.canonical_successor_selection_count = 1U;
  if (successor_key == result.source_facet_key) {
    result.disposition =
        ExactDirectSparseFacetDescentStepDisposition::unresolved;
    result.decision = ExactDirectSparseFacetDescentStepDecision::
        complete_unresolved_source_is_canonical_top_k_choice;
    return transient;
  }

  const ExactFacetMiniballResult* successor_miniball_pointer =
      certified_miniball_lookup(successor_key);
  if (successor_miniball_pointer == nullptr) {
    transient.newly_built_successor_miniball.emplace(
        build_exact_facet_miniball(cloud, used_point_ids(successor_key)));
    successor_miniball_pointer =
        &*transient.newly_built_successor_miniball;
    result.counters.successor_miniball_build_count = 1U;
    result.successor_miniball_freshly_certified = certified_local_miniball(
        cloud, *successor_miniball_pointer, successor_key);
    if (!result.successor_miniball_freshly_certified) {
      throw std::logic_error(
          "the exact successor-facet miniball did not close locally");
    }
  } else {
    if (!certified_local_miniball(
            cloud, *successor_miniball_pointer, successor_key)) {
      throw std::logic_error(
          "the certified miniball lookup returned a mismatched full key, exact center or exact level");
    }
    result.counters.successor_miniball_reuse_count = 1U;
    result.successor_miniball_reused_from_certified_lookup = true;
  }
  const ExactFacetMiniballResult& successor_miniball =
      *successor_miniball_pointer;
  result.counters.exact_level_relation_count += 2U;
  if (successor_miniball.squared_radius >
      top_k.cutoff_squared_distance()) {
    result.disposition =
        ExactDirectSparseFacetDescentStepDisposition::contradiction;
    result.decision = ExactDirectSparseFacetDescentStepDecision::
        contradiction_successor_miniball_above_top_k_cutoff;
    return transient;
  }
  if (successor_miniball.squared_radius >=
      source_miniball.squared_radius) {
    result.disposition =
        ExactDirectSparseFacetDescentStepDisposition::unresolved;
    result.decision = ExactDirectSparseFacetDescentStepDecision::
        complete_unresolved_non_strict_canonical_successor;
    return transient;
  }

  const std::span<const PointId> successor_ids = used_point_ids(successor_key);
  exact::ExactLevel successor_at_source = exact_squared_distance(
      source_miniball.center,
      cloud.point(successor_ids.front()).exact());
  result.counters.successor_source_distance_evaluation_count = 1U;
  for (std::size_t point_index = 1U;
       point_index < successor_ids.size();
       ++point_index) {
    const exact::ExactLevel point_level = exact_squared_distance(
        source_miniball.center,
        cloud.point(successor_ids[point_index]).exact());
    ++result.counters.successor_source_distance_evaluation_count;
    ++result.counters.successor_source_maximum_comparison_count;
    if (point_level > successor_at_source) {
      successor_at_source = point_level;
    }
  }
  ++result.counters.exact_level_relation_count;
  if (successor_at_source != top_k.cutoff_squared_distance()) {
    result.disposition =
        ExactDirectSparseFacetDescentStepDisposition::contradiction;
    result.decision = ExactDirectSparseFacetDescentStepDecision::
        contradiction_successor_source_atom_cutoff_mismatch;
    return transient;
  }

  ExactDirectSparseFacetDescentStepWitness strict_witness;
  strict_witness.source_facet_key = result.source_facet_key;
  strict_witness.successor_facet_key = successor_key;
  strict_witness.source_center = source_miniball.center;
  strict_witness.successor_center = successor_miniball.center;
  strict_witness.source_facet_squared_level =
      source_miniball.squared_radius;
  strict_witness.top_k_cutoff_squared_level =
      top_k.cutoff_squared_distance();
  strict_witness.successor_at_source_squared_level =
      std::move(successor_at_source);
  strict_witness.successor_facet_squared_level =
      successor_miniball.squared_radius;
  strict_witness.center_squared_displacement = exact_squared_distance(
      source_miniball.center, successor_miniball.center);
  result.counters.center_displacement_evaluation_count = 1U;
  strict_witness.successor_is_complete_canonical_top_k_choice = true;
  strict_witness.successor_differs_from_source = true;
  strict_witness.successor_at_source_equals_top_k_cutoff = true;
  strict_witness.top_k_cutoff_at_most_source_level = true;
  strict_witness.successor_level_at_most_top_k_cutoff = true;
  strict_witness.strict_miniball_level_decrease = true;
  strict_witness.exact_squared_distance_chord_identity_applies = true;
  strict_witness.source_facet_at_or_below_closed_batch_level = true;
  strict_witness.source_open_target_closed_segment_strict_below_source_level =
      true;
  strict_witness.source_open_target_closed_segment_strict_below_closed_batch_level =
      true;
  strict_witness.closed_segment_strict_below_closed_batch_level =
      top_k.cutoff_squared_distance() < closed_batch_squared_level;
  ++result.counters.exact_level_relation_count;
  result.counters.convex_segment_certification_count = 1U;
  result.exact_level_relations_certified = true;
  result.strict_half_open_segment_certified = true;
  result.strict_step_witness.emplace(std::move(strict_witness));

  result.successor_locator_probe.emplace(locator.probe_positive_facet(
      successor_key,
      locator_query_witness,
      budget.successor_locator_probe));
  result.counters.successor_locator_probe_count = 1U;
  result.successor_probe_used_same_const_pre_call_locator = true;
  if (result.successor_locator_probe->certified_budget_exhaustion()) {
    result.disposition =
        ExactDirectSparseFacetDescentStepDisposition::budget_exhausted;
    result.decision = ExactDirectSparseFacetDescentStepDecision::
        no_resolution_successor_locator_probe_budget_exhausted;
    return transient;
  }
  if (result.successor_locator_probe->certified_unresolved_miss()) {
    result.disposition =
        ExactDirectSparseFacetDescentStepDisposition::unresolved;
    result.decision = ExactDirectSparseFacetDescentStepDecision::
        complete_unresolved_strict_successor_not_bound;
    return transient;
  }
  if (!result.successor_locator_probe->certified_positive_hit()) {
    throw std::logic_error(
        "a valid const successor probe returned no certified disposition");
  }
  result.resolved_component_handle =
      result.successor_locator_probe->component_handle;
  result.resolved_binding_witness =
      result.successor_locator_probe->source_binding_witness;
  result.disposition =
      ExactDirectSparseFacetDescentStepDisposition::relative_positive;
  result.decision = ExactDirectSparseFacetDescentStepDecision::
      complete_relative_strict_successor_positive_hit;
  return transient;
}

ExactDirectSparseFacetDescentStepResult
build_exact_direct_sparse_facet_descent_step(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> source_facet_point_ids,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentStepBudget& budget,
    spatial::LbvhTraversalOrder traversal_order) {
  detail::ExactDirectSparseFacetDescentStepTransient transient =
      detail::build_exact_direct_sparse_facet_descent_step_transient(
          index,
          cloud,
          source_facet_point_ids,
          closed_batch_squared_level,
          locator_query_witness,
          locator,
          budget,
          traversal_order);
  return std::move(transient.result);
}

ExactDirectSparseFacetDescentStepVerification
verify_exact_direct_sparse_facet_descent_step(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> source_facet_point_ids,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentStepBudget& budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseFacetDescentStepResult& observed) {
  ExactDirectSparseFacetDescentStepVerification verification;
  verification.trusted_inputs_certified =
      index.validated_for(cloud) && locator.certified_positive_locator() &&
      locator_query_witness.external_authority_id != 0U &&
      locator_query_witness.external_authority_id ==
          locator.config().external_authority_id &&
      locator_query_witness.replay_token != 0U;
  if (!verification.trusted_inputs_certified) {
    return verification;
  }

  const ExactDirectSparseFacetKey expected_source_key =
      canonical_facet_key(cloud, source_facet_point_ids);
  const ExactDirectSparseFacetDescentStepResult expected =
      build_exact_direct_sparse_facet_descent_step(
          index,
          cloud,
          source_facet_point_ids,
          closed_batch_squared_level,
          locator_query_witness,
          locator,
          budget,
          traversal_order);

  verification.observed_outcome_well_formed =
      observed.certified_partial_refinement_outcome();
  verification.source_key_freshly_reconstructed =
      observed.source_facet_key == expected_source_key &&
      expected.source_facet_key == expected_source_key;
  verification.bounded_top_k_freshly_replayed =
      observed.counters.top_k_query_count ==
          expected.counters.top_k_query_count &&
      observed.top_k_audit == expected.top_k_audit &&
      observed.complete_top_k_query_counters ==
          expected.complete_top_k_query_counters;
  verification.strict_witness_freshly_replayed =
      observed.strict_step_witness == expected.strict_step_witness;
  verification.locator_probes_freshly_replayed =
      observed.source_locator_probe == expected.source_locator_probe &&
      observed.successor_locator_probe ==
          expected.successor_locator_probe;
  verification.no_partial_top_k_partition_persisted = true;
  verification.no_locator_mutation_or_batch_commit =
      !observed.locator_state_mutated &&
      !observed.locator_batch_committed &&
      !expected.locator_state_mutated &&
      !expected.locator_batch_committed;
  verification.no_isolation_singleton_or_attachment_invented =
      !observed.missing_facet_means_isolated &&
      !observed.singleton_component_created &&
      !observed.hierarchy_attachment_published &&
      !expected.missing_facet_means_isolated &&
      !expected.singleton_component_created &&
      !expected.hierarchy_attachment_published;
  verification.external_binding_authority_replayed = false;
  verification.no_forbidden_global_structure_materialized =
      !observed.global_closed_ball_materialized &&
      !observed.forbidden_global_structure_materialized &&
      !expected.global_closed_ball_materialized &&
      !expected.forbidden_global_structure_materialized;
  verification.fresh_replay_certified = observed == expected;
  verification.result_certified =
      verification.observed_outcome_well_formed &&
      verification.source_key_freshly_reconstructed &&
      verification.bounded_top_k_freshly_replayed &&
      verification.strict_witness_freshly_replayed &&
      verification.locator_probes_freshly_replayed &&
      verification.no_partial_top_k_partition_persisted &&
      verification.no_locator_mutation_or_batch_commit &&
      verification.no_isolation_singleton_or_attachment_invented &&
      !verification.external_binding_authority_replayed &&
      verification.no_forbidden_global_structure_materialized &&
      verification.fresh_replay_certified &&
      expected.certified_partial_refinement_outcome();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
