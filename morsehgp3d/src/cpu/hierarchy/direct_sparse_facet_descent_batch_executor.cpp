#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_batch_executor.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::size_t maximum_lane_count_per_exact_batch = 3U;
constexpr std::size_t maximum_selected_key_point_reference_count =
    direct_sparse_facet_descent_closure_maximum_seed_count *
    direct_sparse_positive_facet_maximum_point_count;

enum class BuildFailure : std::uint8_t {
  source_join_inconsistent,
  capacity_overflow,
  batch_budget_exhausted,
  allocation_failed,
  shared_closure_budget_exhausted,
  shared_closure_unresolved,
  shared_closure_contradiction,
  shared_closure_rejected,
};

struct BatchCursor {
  std::size_t source_batch_index{};
  std::size_t source_chunk_index{};
  std::size_t source_lane_index{};
  std::size_t source_family_index{};
  std::size_t source_arm_seed_index{};
};

struct SelectedArm {
  std::size_t arm_seed_index{};
  std::size_t family_index{};
  std::size_t lane_index{};
  ExactDirectSparseFacetKey source_facet_key{};
};

struct CompactClosureProjection {
  ExactDirectSparseFacetDescentBatchClosureSummary summary{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_snapshot_stamp{};
  std::vector<ExactDirectSparseFacetDescentBatchResolvedKey> resolved_keys;
  std::size_t closure_build_count{};
  std::optional<BuildFailure> failure;
};

struct ProposalPreparationEvidence {
  std::optional<
      ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit>
      consumption_audit;
  ExactDirectSparseFacetDescentClosureTopKProposalConsumptionDecision
      consumption_decision{
          ExactDirectSparseFacetDescentClosureTopKProposalConsumptionDecision::
              not_validated};
  bool consumption_attempted{false};
  bool validation_completed_before_closure{false};
  bool exact_consumption_certified{false};
  bool atomic_rejection_certified{false};
  bool no_closure_constructed_on_rejection{false};
  bool scientific_closure_separated_from_consumption_audit{false};
  bool no_transcript_or_top_k_payload_persisted_in_closure{false};
};

[[nodiscard]] std::optional<std::size_t> checked_add(
    std::size_t left,
    std::size_t right) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return std::nullopt;
  }
  return left + right;
}

[[nodiscard]] std::optional<std::size_t> checked_multiply(
    std::size_t left,
    std::size_t right) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return std::nullopt;
  }
  return left * right;
}

[[nodiscard]] bool known_atomic_transcript_rejection_decision(
    ExactDirectSparseFacetDescentClosureTopKProposalConsumptionDecision
        decision) noexcept {
  using Decision =
      ExactDirectSparseFacetDescentClosureTopKProposalConsumptionDecision;
  switch (decision) {
    case Decision::no_closure_transcript_not_complete:
    case Decision::no_closure_metadata_mismatch:
    case Decision::no_closure_canonical_seed_shape_rejected:
    case Decision::no_closure_record_key_not_in_seed_domain:
    case Decision::no_closure_candidate_point_domain_rejected:
      return true;
    case Decision::not_validated:
    case Decision::complete_exact_closure_with_proposal_consumption_audit:
      return false;
  }
  return false;
}

[[nodiscard]] bool proposal_rejection_audit_has_no_exact_work(
    const ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit&
        audit) noexcept {
  const bool no_stop_reason_count = std::all_of(
      audit.top_k_stop_reason_counts.begin(),
      audit.top_k_stop_reason_counts.end(),
      [](std::size_t count) { return count == 0U; });
  return audit.closure_build_count == 0U &&
         audit.top_k_query_count == 0U &&
         audit.nonempty_proposal_hit_query_count == 0U &&
         audit.missing_initial_record_fallback_query_count == 0U &&
         audit.explicit_empty_record_fallback_query_count == 0U &&
         audit.dynamic_successor_fallback_query_count == 0U &&
         audit.baseline_facet_point_reference_count == 0U &&
         audit.proposal_point_reference_count == 0U &&
         audit.union_point_reference_count == 0U &&
         audit.deduplicated_point_reference_count == 0U &&
         audit.exact_seed_distance_evaluation_count == 0U &&
         audit.exact_point_distance_evaluation_count == 0U &&
         audit.node_visit_count == 0U &&
         audit.internal_node_expansion_count == 0U &&
         audit.exact_aabb_bound_evaluation_count == 0U &&
         audit.pruned_subtree_count == 0U &&
         audit.pruned_eligible_point_count == 0U &&
         audit.traversal_complete_query_count == 0U &&
         audit.complete_query_count == 0U &&
         audit.full_scan_query_count == 0U &&
         audit.strict_pruning_query_count == 0U &&
         audit.exhausted_query_count == 0U &&
         no_stop_reason_count;
}

[[nodiscard]] bool proposal_consumption_audit_consistent_with_delta(
    const ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit&
        audit,
    const ExactDirectSparseFacetDescentBatchExecutionResult& delta) noexcept {
  std::optional<std::size_t> classified_query_count = checked_add(
      audit.nonempty_proposal_hit_query_count,
      audit.missing_initial_record_fallback_query_count);
  if (!classified_query_count.has_value()) {
    return false;
  }
  classified_query_count = checked_add(
      *classified_query_count,
      audit.explicit_empty_record_fallback_query_count);
  if (!classified_query_count.has_value()) {
    return false;
  }
  classified_query_count = checked_add(
      *classified_query_count,
      audit.dynamic_successor_fallback_query_count);
  const std::optional<std::size_t> combined_pool_reference_count =
      checked_add(
          audit.baseline_facet_point_reference_count,
          audit.proposal_point_reference_count);
  const std::optional<std::size_t> complete_classification_count =
      checked_add(
          audit.full_scan_query_count,
          audit.strict_pruning_query_count);
  const std::optional<std::size_t> terminal_query_count =
      checked_add(
          audit.complete_query_count,
          audit.exhausted_query_count);
  const std::optional<std::size_t> expected_baseline_reference_count =
      checked_multiply(
          audit.top_k_query_count,
          delta.source_facet_cardinality);
  const std::optional<std::size_t> maximum_transcript_candidate_count =
      checked_multiply(
          audit.transcript_record_count,
          delta.source_facet_cardinality);
  const auto& top_k_budget =
      delta.requested_closure_budget.step_budget.top_k_query;
  const std::optional<std::size_t> maximum_node_visit_count =
      checked_multiply(
          audit.top_k_query_count,
          top_k_budget.maximum_node_visit_count);
  const std::optional<std::size_t>
      maximum_internal_node_expansion_count = checked_multiply(
          audit.top_k_query_count,
          top_k_budget.maximum_internal_node_expansion_count);
  const std::optional<std::size_t>
      maximum_exact_aabb_bound_evaluation_count = checked_multiply(
          audit.top_k_query_count,
          top_k_budget.maximum_exact_aabb_bound_evaluation_count);
  const std::optional<std::size_t>
      maximum_exact_point_distance_evaluation_count = checked_multiply(
          audit.top_k_query_count,
          top_k_budget.maximum_exact_point_distance_evaluation_count);
  if (!classified_query_count.has_value() ||
      !combined_pool_reference_count.has_value() ||
      !complete_classification_count.has_value() ||
      !terminal_query_count.has_value() ||
      !expected_baseline_reference_count.has_value() ||
      !maximum_transcript_candidate_count.has_value() ||
      !maximum_node_visit_count.has_value() ||
      !maximum_internal_node_expansion_count.has_value() ||
      !maximum_exact_aabb_bound_evaluation_count.has_value() ||
      !maximum_exact_point_distance_evaluation_count.has_value() ||
      audit.deduplicated_point_reference_count >
          *combined_pool_reference_count) {
    return false;
  }

  std::size_t stop_reason_count = 0U;
  for (const std::size_t count : audit.top_k_stop_reason_counts) {
    const std::optional<std::size_t> next =
        checked_add(stop_reason_count, count);
    if (!next.has_value()) {
      return false;
    }
    stop_reason_count = *next;
  }
  const std::size_t no_stop_reason_index = static_cast<std::size_t>(
      spatial::ExactLbvhTopKStopReason::none);
  if (no_stop_reason_index >=
      audit.top_k_stop_reason_counts.size()) {
    return false;
  }

  const std::size_t expected_union_point_reference_count =
      *combined_pool_reference_count -
      audit.deduplicated_point_reference_count;
  return audit.canonical_seed_key_count ==
             delta.required_resolved_key_count &&
         audit.transcript_record_count <=
             audit.canonical_seed_key_count &&
         audit.transcript_candidate_point_reference_count <=
             *maximum_transcript_candidate_count &&
         audit.nonempty_proposal_hit_query_count <=
             audit.transcript_record_count &&
         audit.proposal_point_reference_count <=
             audit.transcript_candidate_point_reference_count &&
         *classified_query_count == audit.top_k_query_count &&
         stop_reason_count == audit.top_k_query_count &&
         *terminal_query_count == audit.top_k_query_count &&
         audit.top_k_stop_reason_counts[no_stop_reason_index] ==
             audit.complete_query_count &&
         *complete_classification_count == audit.complete_query_count &&
         audit.traversal_complete_query_count <=
             audit.top_k_query_count &&
         audit.union_point_reference_count ==
             expected_union_point_reference_count &&
         audit.baseline_facet_point_reference_count ==
             *expected_baseline_reference_count &&
         audit.exact_seed_distance_evaluation_count <=
             audit.union_point_reference_count &&
         audit.exact_point_distance_evaluation_count >=
             audit.exact_seed_distance_evaluation_count &&
         audit.node_visit_count <= *maximum_node_visit_count &&
         audit.internal_node_expansion_count <=
             *maximum_internal_node_expansion_count &&
         audit.exact_aabb_bound_evaluation_count <=
             *maximum_exact_aabb_bound_evaluation_count &&
         audit.exact_point_distance_evaluation_count <=
             *maximum_exact_point_distance_evaluation_count;
}

[[nodiscard]] bool key_less(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right) noexcept {
  const std::size_t shared_count =
      std::min(left.point_count, right.point_count);
  for (std::size_t index = 0U; index < shared_count; ++index) {
    if (left.point_ids[index] != right.point_ids[index]) {
      return left.point_ids[index] < right.point_ids[index];
    }
  }
  return left.point_count < right.point_count;
}

[[nodiscard]] bool valid_key(
    const ExactDirectSparseFacetKey& key,
    std::size_t point_count,
    std::size_t expected_cardinality) noexcept {
  if (key.point_count != expected_cardinality ||
      key.point_count == 0U ||
      key.point_count > key.point_ids.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (key.point_ids[index] >= point_count ||
        (index != 0U &&
         key.point_ids[index - 1U] >= key.point_ids[index])) {
      return false;
    }
  }
  for (std::size_t index = key.point_count;
       index < key.point_ids.size();
       ++index) {
    if (key.point_ids[index] != 0U) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] ExactDirectSparseFacetKey facet_key_from(
    const ExactDirectSaddleArmFacet& facet,
    std::size_t point_count,
    std::size_t expected_cardinality) {
  ExactDirectSparseFacetKey key;
  key.point_count = facet.point_count;
  key.point_ids = facet.point_ids;
  if (!valid_key(key, point_count, expected_cardinality)) {
    throw std::logic_error(
        "a selected direct saddle arm did not reconstruct one canonical "
        "order-k key");
  }
  return key;
}

void require_valid_traversal_order(
    spatial::LbvhTraversalOrder traversal_order) {
  switch (traversal_order) {
    case spatial::LbvhTraversalOrder::near_first:
    case spatial::LbvhTraversalOrder::far_first:
      return;
  }
  throw std::invalid_argument("an LBVH traversal order is invalid");
}

void require_execution_budget_within_confidence(
    const ExactDirectSparseFacetDescentBatchExecutionBudget& budget) {
  if (budget.maximum_selected_lane_count >
          maximum_lane_count_per_exact_batch ||
      budget.maximum_selected_family_count >
          direct_sparse_facet_descent_closure_maximum_seed_count ||
      budget.maximum_selected_arm_seed_count >
          direct_sparse_facet_descent_closure_maximum_seed_count ||
      budget.maximum_selected_key_point_reference_count >
          maximum_selected_key_point_reference_count ||
      budget.maximum_resolved_key_count >
          direct_sparse_facet_descent_closure_maximum_seed_count) {
    throw std::invalid_argument(
        "a direct batch-execution budget exceeds its confidence cap");
  }
}

void require_closure_budget_within_confidence(
    const ExactDirectSparseFacetDescentClosureBudget& budget) {
  if (budget.maximum_seed_count >
          direct_sparse_facet_descent_closure_maximum_seed_count ||
      budget.maximum_node_count >
          direct_sparse_facet_descent_closure_maximum_node_count ||
      budget.maximum_step_call_count >
          direct_sparse_facet_descent_closure_maximum_step_call_count ||
      budget.maximum_memo_slot_count >
          direct_sparse_facet_descent_closure_maximum_memo_slot_count) {
    throw std::invalid_argument(
        "a direct batch-execution closure budget exceeds its confidence cap");
  }
}

void initialize_result_scope(
    ExactDirectSparseFacetDescentBatchExecutionResult& result) {
  result.source_plan_verified_once_at_session_open = true;
  result.source_plan_rebuilt_for_this_batch = false;
  result.counters.anchored_source_plan_verification_count = 1U;
  result.counters.batch_local_source_plan_rebuild_count = 0U;
  result.scientific_commit_performed = false;
  result.locator_state_mutated = false;
  result.hierarchy_reduction_or_attachment_published = false;
  result.partial_closure_prefix_published_as_delta = false;
  result.facet_coface_cell_gamma_or_delaunay_materialized = false;
  result.gpu_execution_qualified = false;
  result.sub_second_latency_claimed = false;
  result.ten_million_point_capacity_claimed = false;
  result.public_status_claimed = false;
  result.closure_graph_persisted = false;
  result.scope =
      ExactDirectSparseFacetDescentBatchExecutionScope::
          one_exact_14c_batch_compact_relative_positive_delta_before_hierarchy_commit_only;
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchExecutionResult fail(
    ExactDirectSparseFacetDescentBatchExecutionResult result,
    BuildFailure failure) {
  result.resolved_keys.clear();
  result.arm_joins.clear();
  result.batch_delta_ready_before_commit = false;
  result.every_arm_joined_by_identity_and_full_key = false;
  result.partial_closure_prefix_published_as_delta = false;
  result.closure_graph_persisted = false;
  result.transient_closure_released_before_delta_publication = true;
  switch (failure) {
    case BuildFailure::source_join_inconsistent:
      result.decision =
          ExactDirectSparseFacetDescentBatchExecutionDecision::
              no_execution_source_join_inconsistent;
      break;
    case BuildFailure::capacity_overflow:
      result.decision =
          ExactDirectSparseFacetDescentBatchExecutionDecision::
              no_execution_capacity_overflow;
      break;
    case BuildFailure::batch_budget_exhausted:
      result.decision =
          ExactDirectSparseFacetDescentBatchExecutionDecision::
              no_execution_batch_budget_exhausted;
      break;
    case BuildFailure::allocation_failed:
      result.decision =
          ExactDirectSparseFacetDescentBatchExecutionDecision::
              no_execution_allocation_failed;
      break;
    case BuildFailure::shared_closure_budget_exhausted:
      result.decision =
          ExactDirectSparseFacetDescentBatchExecutionDecision::
              no_execution_shared_closure_budget_exhausted;
      break;
    case BuildFailure::shared_closure_unresolved:
      result.decision =
          ExactDirectSparseFacetDescentBatchExecutionDecision::
              no_execution_shared_closure_unresolved;
      break;
    case BuildFailure::shared_closure_contradiction:
      result.decision =
          ExactDirectSparseFacetDescentBatchExecutionDecision::
              no_execution_shared_closure_contradiction;
      break;
    case BuildFailure::shared_closure_rejected:
      result.decision =
          ExactDirectSparseFacetDescentBatchExecutionDecision::
              no_execution_shared_closure_rejected;
      break;
  }
  return result;
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchClosureSummary
summarize_closure(
    const ExactDirectSparseFacetDescentClosureResult& closure) {
  ExactDirectSparseFacetDescentBatchClosureSummary summary;
  summary.transient_node_count = closure.nodes.size();
  summary.transient_edge_count = closure.edges.size();
  summary.transient_seed_projection_count =
      closure.seed_projections.size();
  summary.required_memo_slot_count = closure.required_memo_slot_count;
  summary.counters = closure.counters;
  summary.disposition = closure.disposition;
  summary.decision = closure.decision;
  summary.complete_relative_positive =
      closure.certified_complete_relative_positive_closure();
  summary.graph_payload_persisted = false;
  return summary;
}

[[nodiscard]] CompactClosureProjection build_compact_closure_projection(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const std::vector<ExactDirectSparseFacetKey>& distinct_keys,
    std::size_t source_batch_index,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentClosureBudget& closure_budget,
    const ExactDirectSparseFacetDescentClosureConfig& closure_config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseFacetTopKProposalTranscriptResult*
        proposal_transcript,
    ProposalPreparationEvidence* proposal_evidence) {
  // The closure object is scoped inside this lambda.  Only the compact
  // projection and, on the proposal path, a key-free scalar audit survive its
  // destruction.
  return [&]() {
    CompactClosureProjection projection;
    ExactDirectSparseFacetDescentClosureResult closure;
    if (proposal_transcript == nullptr) {
      closure =
          build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys(
              index,
              cloud,
              std::span<const ExactDirectSparseFacetKey>{distinct_keys},
              closed_batch_squared_level,
              locator_query_witness,
              locator,
              closure_budget,
              closure_config,
              traversal_order);
      projection.closure_build_count = 1U;
    } else {
      if (proposal_evidence == nullptr) {
        throw std::logic_error(
            "proposal consumption requires a separate preparation audit");
      }
      proposal_evidence->consumption_attempted = true;
      ExactDirectSparseFacetDescentClosureTopKProposalConsumptionResult
          wrapped =
              build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys_with_top_k_proposal_transcript(
                  index,
                  cloud,
                  std::span<const ExactDirectSparseFacetKey>{distinct_keys},
                  source_batch_index,
                  closed_batch_squared_level,
                  locator_query_witness,
                  locator,
                  *proposal_transcript,
                  closure_budget,
                  closure_config,
                  traversal_order);
      proposal_evidence->consumption_audit =
          wrapped.consumption_audit;
      proposal_evidence->consumption_decision = wrapped.decision;
      proposal_evidence->validation_completed_before_closure =
          wrapped.validation_completed_before_closure;
      proposal_evidence->exact_consumption_certified =
          wrapped.certified_exact_consumption_outcome();
      proposal_evidence->atomic_rejection_certified =
          wrapped.certified_atomic_rejection();
      proposal_evidence->no_closure_constructed_on_rejection =
          wrapped.no_closure_constructed_on_rejection;
      proposal_evidence
          ->scientific_closure_separated_from_consumption_audit =
          wrapped.scientific_closure_separated_from_consumption_audit;
      proposal_evidence
          ->no_transcript_or_top_k_payload_persisted_in_closure =
          wrapped.no_transcript_or_top_k_payload_persisted_in_closure;
      projection.locator_snapshot_stamp =
          wrapped.consumption_audit.locator_snapshot_stamp;
      projection.closure_build_count =
          wrapped.consumption_audit.closure_build_count;
      if (wrapped.certified_atomic_rejection()) {
        projection.failure = BuildFailure::shared_closure_rejected;
        return projection;
      }
      if (!wrapped.certified_exact_consumption_outcome() ||
          !wrapped.scientific_closure.has_value()) {
        throw std::logic_error(
            "proposal consumption produced no certified exact closure");
      }
      closure = std::move(*wrapped.scientific_closure);
    }
    projection.summary = summarize_closure(closure);
    projection.locator_snapshot_stamp = closure.locator_snapshot_stamp;

    if (!closure.certified_complete_relative_positive_closure()) {
      if (closure.certified_budget_exhaustion()) {
        projection.failure =
            BuildFailure::shared_closure_budget_exhausted;
      } else if (
          closure.certified_complete_with_unresolved_terminals()) {
        projection.failure = BuildFailure::shared_closure_unresolved;
      } else if (closure.certified_fail_closed_contradiction()) {
        projection.failure = BuildFailure::shared_closure_contradiction;
      } else {
        projection.failure = BuildFailure::shared_closure_rejected;
      }
      return projection;
    }
    if (closure.seed_projections.size() != distinct_keys.size()) {
      projection.failure = BuildFailure::shared_closure_contradiction;
      return projection;
    }

    projection.resolved_keys.reserve(distinct_keys.size());
    for (std::size_t key_index = 0U;
         key_index < distinct_keys.size();
         ++key_index) {
      const ExactDirectSparseFacetDescentSeedProjection& seed_projection =
          closure.seed_projections[key_index];
      if (seed_projection.seed_index != key_index ||
          seed_projection.source_facet_key != distinct_keys[key_index] ||
          seed_projection.closure_disposition !=
              ExactDirectSparseFacetDescentClosureDisposition::
                  relative_positive ||
          seed_projection.terminal_node_index >= closure.nodes.size()) {
        projection.failure = BuildFailure::shared_closure_contradiction;
        projection.resolved_keys.clear();
        return projection;
      }
      const ExactDirectSparseFacetDescentNode& terminal =
          closure.nodes[seed_projection.terminal_node_index];
      if (!terminal.resolved_component_handle.has_value() ||
          !terminal.resolved_binding_witness.has_value() ||
          terminal.closure_disposition !=
              ExactDirectSparseFacetDescentClosureDisposition::
                  relative_positive ||
          terminal.resolved_binding_witness->external_authority_id !=
              locator.config().external_authority_id ||
          terminal.resolved_binding_witness->replay_token == 0U) {
        projection.failure = BuildFailure::shared_closure_contradiction;
        projection.resolved_keys.clear();
        return projection;
      }
      projection.resolved_keys.push_back(
          {key_index,
           distinct_keys[key_index],
           *terminal.resolved_component_handle,
           *terminal.resolved_binding_witness,
           seed_projection.closure_disposition,
           true});
    }
    return projection;
  }();
}

[[nodiscard]] bool observed_storage_within_budget(
    const ExactDirectSparseFacetDescentBatchExecutionResult& observed,
    const ExactDirectSparseFacetDescentBatchExecutionBudget& budget) noexcept {
  if (observed.resolved_keys.size() >
          budget.maximum_resolved_key_count ||
      observed.arm_joins.size() >
          budget.maximum_selected_arm_seed_count) {
    return false;
  }
  std::size_t key_point_reference_count = 0U;
  for (const auto& resolved : observed.resolved_keys) {
    const std::optional<std::size_t> next_key_point_reference_count =
        checked_add(
            key_point_reference_count,
            resolved.source_facet_key.point_count);
    if (resolved.source_facet_key.point_count >
            resolved.source_facet_key.point_ids.size() ||
        !next_key_point_reference_count.has_value() ||
        *next_key_point_reference_count >
            budget.maximum_selected_key_point_reference_count) {
      return false;
    }
    key_point_reference_count = *next_key_point_reference_count;
  }
  return true;
}

[[nodiscard]] bool architecture_execution_payload_shape_is_consistent(
    const ExactDirectSparseFacetDescentBatchExecutionResult& result) noexcept {
  if (!result.source_chunk_index.has_value() ||
      result.source_facet_cardinality == 0U ||
      result.source_facet_cardinality >
          direct_sparse_positive_facet_maximum_point_count ||
      result.source_lane_end_index < result.source_lane_begin_index ||
      result.source_family_end_index < result.source_family_begin_index ||
      result.source_arm_seed_end_index <
          result.source_arm_seed_begin_index ||
      result.source_lane_end_index - result.source_lane_begin_index !=
          result.required_selected_lane_count ||
      result.source_family_end_index - result.source_family_begin_index !=
          result.required_selected_family_count ||
      result.source_arm_seed_end_index -
              result.source_arm_seed_begin_index !=
          result.required_selected_arm_seed_count ||
      result.required_selected_lane_count >
          result.requested_budget.maximum_selected_lane_count ||
      result.required_selected_family_count >
          result.requested_budget.maximum_selected_family_count ||
      result.required_selected_arm_seed_count >
          result.requested_budget.maximum_selected_arm_seed_count ||
      result.required_selected_key_point_reference_count >
          result.requested_budget
              .maximum_selected_key_point_reference_count ||
      result.required_resolved_key_count >
          result.requested_budget.maximum_resolved_key_count ||
      result.required_resolved_key_count >
          result.required_selected_arm_seed_count ||
      result.resolved_keys.size() != result.required_resolved_key_count ||
      result.arm_joins.size() !=
          result.required_selected_arm_seed_count ||
      result.locator_query_witness.external_authority_id == 0U ||
      result.locator_query_witness.replay_token == 0U) {
    return false;
  }

  const std::optional<std::size_t>
      expected_selected_key_point_reference_count = checked_multiply(
          result.required_selected_arm_seed_count,
          result.source_facet_cardinality);
  const std::optional<std::size_t>
      expected_resolved_key_point_reference_count = checked_multiply(
          result.required_resolved_key_count,
          result.source_facet_cardinality);
  const std::optional<std::size_t> projected_arm_reference_count =
      checked_add(
          result.counters.duplicate_arm_key_reference_count,
          result.counters.distinct_closure_seed_count);
  if (!expected_selected_key_point_reference_count.has_value() ||
      !expected_resolved_key_point_reference_count.has_value() ||
      !projected_arm_reference_count.has_value() ||
      result.required_selected_key_point_reference_count !=
          *expected_selected_key_point_reference_count ||
      *expected_resolved_key_point_reference_count >
          result.requested_budget
              .maximum_selected_key_point_reference_count ||
      result.counters.distinct_closure_seed_count !=
          result.required_resolved_key_count ||
      *projected_arm_reference_count !=
          result.required_selected_arm_seed_count) {
    return false;
  }

  for (std::size_t resolved_index = 0U;
       resolved_index < result.resolved_keys.size();
       ++resolved_index) {
    const ExactDirectSparseFacetDescentBatchResolvedKey& resolved =
        result.resolved_keys[resolved_index];
    if (resolved.resolved_key_index != resolved_index ||
        !valid_key(
            resolved.source_facet_key,
            std::numeric_limits<std::size_t>::max(),
            result.source_facet_cardinality) ||
        resolved.resolved_binding_witness.external_authority_id !=
            result.locator_query_witness.external_authority_id ||
        resolved.resolved_binding_witness.replay_token == 0U ||
        resolved.closure_disposition !=
            ExactDirectSparseFacetDescentClosureDisposition::
                relative_positive ||
        !resolved.source_projection_and_terminal_certified ||
        (resolved_index != 0U &&
         !key_less(
             result.resolved_keys[resolved_index - 1U].source_facet_key,
             resolved.source_facet_key))) {
      return false;
    }
  }

  for (std::size_t join_index = 0U;
       join_index < result.arm_joins.size();
       ++join_index) {
    const ExactDirectSparseFacetDescentBatchArmJoin& join =
        result.arm_joins[join_index];
    const std::optional<std::size_t> expected_arm_seed_index =
        checked_add(result.source_arm_seed_begin_index, join_index);
    if (!expected_arm_seed_index.has_value() ||
        join.arm_seed_index != *expected_arm_seed_index ||
        join.family_index < result.source_family_begin_index ||
        join.family_index >= result.source_family_end_index ||
        join.lane_index < result.source_lane_begin_index ||
        join.lane_index >= result.source_lane_end_index ||
        join.resolved_key_index >= result.resolved_keys.size() ||
        !join.arm_identity_and_full_key_joined ||
        (join_index != 0U &&
         join.family_index <
             result.arm_joins[join_index - 1U].family_index)) {
      return false;
    }
  }

  const bool empty_batch =
      result.required_selected_arm_seed_count == 0U;
  if (empty_batch) {
    return result.required_selected_lane_count == 0U &&
           result.required_selected_family_count == 0U &&
           result.required_resolved_key_count == 0U &&
           result.closure_summary.transient_node_count == 0U &&
           result.closure_summary.transient_edge_count == 0U &&
           result.closure_summary.transient_seed_projection_count == 0U &&
           result.closure_summary.counters ==
               ExactDirectSparseFacetDescentClosureCounters{} &&
           result.closure_summary.disposition ==
               ExactDirectSparseFacetDescentClosureDisposition::
                   relative_positive &&
           result.closure_summary.decision ==
               ExactDirectSparseFacetDescentClosureDecision::
                   complete_empty_seed_set;
  }

  const std::optional<std::size_t> expected_locator_snapshot_check_count =
      checked_add(
          result.closure_summary.counters.evaluated_step_source_count,
          2U);
  return result.required_selected_lane_count != 0U &&
         result.required_selected_family_count != 0U &&
         result.required_resolved_key_count != 0U &&
         result.required_resolved_key_count <=
             result.requested_closure_budget.maximum_seed_count &&
         result.closure_summary.transient_node_count >=
             result.required_resolved_key_count &&
         result.closure_summary.transient_node_count <=
             result.requested_closure_budget.maximum_node_count &&
         result.closure_summary.transient_edge_count <
             result.closure_summary.transient_node_count &&
         result.closure_summary.transient_seed_projection_count ==
             result.required_resolved_key_count &&
         result.closure_summary.required_memo_slot_count <=
             result.requested_closure_budget.maximum_memo_slot_count &&
         result.closure_summary.counters.input_seed_reference_count ==
             result.required_resolved_key_count &&
         result.closure_summary.counters.processed_seed_reference_count ==
             result.required_resolved_key_count &&
         result.closure_summary.counters.distinct_seed_key_count ==
             result.required_resolved_key_count &&
         result.closure_summary.counters.duplicate_seed_key_reference_count ==
             0U &&
         result.closure_summary.counters.interned_node_count ==
             result.closure_summary.transient_node_count &&
         result.closure_summary.counters.strict_edge_count ==
             result.closure_summary.transient_edge_count &&
         result.closure_summary.counters.unresolved_terminal_count == 0U &&
         result.closure_summary.counters.budget_terminal_count == 0U &&
         expected_locator_snapshot_check_count.has_value() &&
         result.closure_summary.counters.locator_snapshot_check_count ==
             *expected_locator_snapshot_check_count &&
         result.closure_summary.disposition ==
             ExactDirectSparseFacetDescentClosureDisposition::
                 relative_positive &&
         result.closure_summary.decision ==
             ExactDirectSparseFacetDescentClosureDecision::
                 complete_all_seeds_relative_positive;
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchExecutionResult
build_batch_execution(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_seed_journal,
    const ExactDirectSparseFacetDescentBatchPlanResult& source_plan,
    const BatchCursor& cursor,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentBatchExecutionBudget& execution_budget,
    const ExactDirectSparseFacetDescentClosureBudget& closure_budget,
    const ExactDirectSparseFacetDescentClosureConfig& closure_config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseFacetTopKProposalTranscriptResult*
        proposal_transcript,
    ProposalPreparationEvidence* proposal_evidence) {
  if ((proposal_transcript == nullptr) !=
      (proposal_evidence == nullptr)) {
    throw std::invalid_argument(
        "a proposal transcript and its separate audit sink must be supplied together");
  }
  if (proposal_evidence != nullptr) {
    *proposal_evidence = ProposalPreparationEvidence{};
  }
  require_execution_budget_within_confidence(execution_budget);
  require_closure_budget_within_confidence(closure_budget);
  require_valid_traversal_order(traversal_order);
  if (locator_query_witness.external_authority_id == 0U ||
      locator_query_witness.external_authority_id !=
          locator.config().external_authority_id ||
      locator_query_witness.replay_token == 0U) {
    throw std::invalid_argument(
        "a direct batch execution requires one matching locator witness");
  }

  ExactDirectSparseFacetDescentBatchExecutionResult result;
  result.requested_budget = execution_budget;
  result.requested_closure_budget = closure_budget;
  result.traversal_order = traversal_order;
  result.source_batch_index = cursor.source_batch_index;
  result.locator_query_witness = locator_query_witness;
  result.locator_snapshot_stamp = locator.snapshot_stamp();
  initialize_result_scope(result);

  if (cursor.source_batch_index >= source_event_journal.batches.size() ||
      cursor.source_chunk_index >=
          source_plan.source_industrial_plan.chunks.size() ||
      cursor.source_lane_index > source_plan.lanes.size() ||
      cursor.source_family_index > source_arm_seed_journal.families.size() ||
      cursor.source_arm_seed_index >
          source_arm_seed_journal.arm_seeds.size()) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }

  const ExactDirectMorseH0Batch& source_batch =
      source_event_journal.batches[cursor.source_batch_index];
  const ExactDirectMorseIndustrialChunk& source_chunk =
      source_plan.source_industrial_plan.chunks[cursor.source_chunk_index];
  if (source_batch.batch_index != cursor.source_batch_index ||
      source_batch.order == 0U ||
      source_batch.order >
          direct_sparse_positive_facet_maximum_point_count ||
      source_chunk.chunk_index != cursor.source_chunk_index ||
      cursor.source_batch_index <
          source_chunk.source_batch_begin_index ||
      cursor.source_batch_index >= source_chunk.source_batch_end_index) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }

  result.source_chunk_index = cursor.source_chunk_index;
  result.source_facet_cardinality = source_batch.order;
  result.closed_batch_squared_level = source_batch.squared_level;
  result.source_lane_begin_index = cursor.source_lane_index;
  result.source_family_begin_index = cursor.source_family_index;
  result.source_arm_seed_begin_index = cursor.source_arm_seed_index;

  std::size_t lane_end = cursor.source_lane_index;
  while (lane_end < source_plan.lanes.size() &&
         source_plan.lanes[lane_end].source_batch_index ==
             cursor.source_batch_index) {
    ++lane_end;
  }
  if (lane_end < source_plan.lanes.size() &&
      source_plan.lanes[lane_end].source_batch_index <
          cursor.source_batch_index) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }
  result.source_lane_end_index = lane_end;

  const std::optional<std::size_t> family_end = checked_add(
      cursor.source_family_index, source_batch.saddle_role_count);
  if (!family_end.has_value() ||
      *family_end > source_arm_seed_journal.families.size()) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  }
  result.source_family_end_index = *family_end;

  std::size_t required_arm_seed_count = 0U;
  std::size_t required_family_count_from_lanes = 0U;
  std::array<std::optional<std::size_t>, 5U> lane_by_support_cardinality{};
  for (std::size_t lane_index = cursor.source_lane_index;
       lane_index < lane_end;
       ++lane_index) {
    const ExactDirectSparseFacetDescentBatchLane& lane =
        source_plan.lanes[lane_index];
    if (lane.lane_index != lane_index ||
        lane.source_chunk_index != cursor.source_chunk_index ||
        lane.source_batch_index != cursor.source_batch_index ||
        lane.candidate_family_begin_index != cursor.source_family_index ||
        lane.candidate_family_end_index != *family_end ||
        lane.candidate_arm_seed_begin_index !=
            cursor.source_arm_seed_index ||
        lane.facet_cardinality != source_batch.order ||
        lane.source_support_cardinality < 2U ||
        lane.source_support_cardinality > 4U ||
        lane.source_support_cardinality > source_batch.order + 1U ||
        lane.source_interior_cardinality !=
            source_batch.order + 1U -
                lane.source_support_cardinality ||
        lane_by_support_cardinality[lane.source_support_cardinality]
            .has_value()) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    lane_by_support_cardinality[lane.source_support_cardinality] =
        lane_index;
    const std::optional<std::size_t> next_family_count = checked_add(
        required_family_count_from_lanes, lane.matching_family_count);
    const std::optional<std::size_t> next_arm_count = checked_add(
        required_arm_seed_count, lane.matching_arm_seed_count);
    if (!next_family_count.has_value() || !next_arm_count.has_value()) {
      return fail(std::move(result), BuildFailure::capacity_overflow);
    }
    required_family_count_from_lanes = *next_family_count;
    required_arm_seed_count = *next_arm_count;
  }
  if (required_family_count_from_lanes !=
          source_batch.saddle_role_count ||
      (source_batch.saddle_role_count == 0U) !=
          (lane_end == cursor.source_lane_index)) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }

  const std::optional<std::size_t> arm_seed_end =
      checked_add(cursor.source_arm_seed_index, required_arm_seed_count);
  const std::optional<std::size_t> key_point_reference_count =
      checked_multiply(required_arm_seed_count, source_batch.order);
  if (!arm_seed_end.has_value() ||
      !key_point_reference_count.has_value()) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  }
  if (*arm_seed_end > source_arm_seed_journal.arm_seeds.size()) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }
  result.source_arm_seed_end_index = *arm_seed_end;
  result.required_selected_lane_count =
      lane_end - cursor.source_lane_index;
  result.required_selected_family_count =
      source_batch.saddle_role_count;
  result.required_selected_arm_seed_count = required_arm_seed_count;
  result.required_selected_key_point_reference_count =
      *key_point_reference_count;
  result.batch_budget_preflight_completed = true;

  if (result.required_selected_lane_count >
          execution_budget.maximum_selected_lane_count ||
      result.required_selected_family_count >
          execution_budget.maximum_selected_family_count ||
      result.required_selected_arm_seed_count >
          execution_budget.maximum_selected_arm_seed_count ||
      result.required_selected_key_point_reference_count >
          execution_budget.maximum_selected_key_point_reference_count) {
    return fail(
        std::move(result), BuildFailure::batch_budget_exhausted);
  }
  result.source_exact_batch_and_chunk_joined = true;
  result.source_lanes_share_one_exact_batch = true;
  result.counters.selected_lane_reference_count =
      result.required_selected_lane_count;

  try {
    std::vector<SelectedArm> selected_arms;
    selected_arms.reserve(required_arm_seed_count);
    std::array<std::size_t, 5U> actual_family_counts{};
    std::array<std::size_t, 5U> actual_arm_counts{};
    std::size_t arm_seed_cursor = cursor.source_arm_seed_index;
    for (std::size_t family_index = cursor.source_family_index;
         family_index < *family_end;
         ++family_index) {
      const ExactDirectSaddleArmFamilyRecord& family =
          source_arm_seed_journal.families[family_index];
      if (family.family_index != family_index ||
          family.journal_batch_index != cursor.source_batch_index ||
          family.order != source_batch.order ||
          family.critical_squared_level != source_batch.squared_level ||
          family.arm_seed_count < 2U ||
          family.arm_seed_count > 4U ||
          family.arm_seed_offset != arm_seed_cursor ||
          !lane_by_support_cardinality[family.arm_seed_count].has_value()) {
        return fail(
            std::move(result), BuildFailure::source_join_inconsistent);
      }
      const std::size_t lane_index =
          *lane_by_support_cardinality[family.arm_seed_count];
      ++actual_family_counts[family.arm_seed_count];
      for (std::size_t local = 0U;
           local < family.arm_seed_count;
           ++local) {
        if (arm_seed_cursor >= *arm_seed_end) {
          return fail(
              std::move(result), BuildFailure::source_join_inconsistent);
        }
        const ExactDirectSaddleArmSeedRecord& arm_seed =
            source_arm_seed_journal.arm_seeds[arm_seed_cursor];
        if (arm_seed.arm_seed_index != arm_seed_cursor ||
            arm_seed.family_index != family_index) {
          return fail(
              std::move(result), BuildFailure::source_join_inconsistent);
        }
        const ExactDirectSaddleArmFacet facet =
            reconstruct_exact_direct_saddle_arm_facet(
                source_facade,
                source_arm_seed_journal,
                arm_seed_cursor);
        selected_arms.push_back(
            {arm_seed_cursor,
             family_index,
             lane_index,
             facet_key_from(
                 facet, cloud.size(), source_batch.order)});
        ++actual_arm_counts[family.arm_seed_count];
        ++arm_seed_cursor;
      }
    }
    if (arm_seed_cursor != *arm_seed_end ||
        selected_arms.size() != required_arm_seed_count) {
      return fail(
          std::move(result), BuildFailure::source_join_inconsistent);
    }
    for (std::size_t lane_index = cursor.source_lane_index;
         lane_index < lane_end;
         ++lane_index) {
      const ExactDirectSparseFacetDescentBatchLane& lane =
          source_plan.lanes[lane_index];
      if (actual_family_counts[lane.source_support_cardinality] !=
              lane.matching_family_count ||
          actual_arm_counts[lane.source_support_cardinality] !=
              lane.matching_arm_seed_count ||
          lane.candidate_arm_seed_end_index != *arm_seed_end) {
        return fail(
            std::move(result), BuildFailure::source_join_inconsistent);
      }
    }

    result.counters.selected_family_scan_count =
        result.required_selected_family_count;
    result.counters.selected_arm_seed_scan_count =
        result.required_selected_arm_seed_count;
    result.counters.reconstructed_facet_count =
        result.required_selected_arm_seed_count;
    result.selected_families_scanned_once = true;
    result.every_arm_seed_selected_once_in_source_order = true;
    result.facets_reconstructed_on_demand_only = true;

    std::vector<ExactDirectSparseFacetKey> distinct_keys;
    distinct_keys.reserve(selected_arms.size());
    for (const SelectedArm& arm : selected_arms) {
      distinct_keys.push_back(arm.source_facet_key);
    }
    std::sort(distinct_keys.begin(), distinct_keys.end(), key_less);
    distinct_keys.erase(
        std::unique(distinct_keys.begin(), distinct_keys.end()),
        distinct_keys.end());
    result.required_resolved_key_count = distinct_keys.size();
    result.counters.distinct_closure_seed_count = distinct_keys.size();
    result.counters.duplicate_arm_key_reference_count =
        selected_arms.size() - distinct_keys.size();
    result.distinct_full_keys_canonicalized = true;
    if (distinct_keys.size() >
        execution_budget.maximum_resolved_key_count) {
      return fail(
          std::move(result), BuildFailure::batch_budget_exhausted);
    }
    result.batch_budget_preflight_satisfied = true;

    if (distinct_keys.empty()) {
      if (proposal_transcript != nullptr) {
        CompactClosureProjection closure_projection =
            build_compact_closure_projection(
                index,
                cloud,
                distinct_keys,
                cursor.source_batch_index,
                source_batch.squared_level,
                locator_query_witness,
                locator,
                closure_budget,
                closure_config,
                traversal_order,
                proposal_transcript,
                proposal_evidence);
        result.locator_snapshot_stamp =
            closure_projection.locator_snapshot_stamp;
        result.transient_closure_released_before_delta_publication = true;
        if (closure_projection.failure.has_value()) {
          return fail(
              std::move(result), *closure_projection.failure);
        }
        if (!closure_projection.resolved_keys.empty()) {
          return fail(
              std::move(result),
              BuildFailure::shared_closure_contradiction);
        }
      }
      result.closure_summary.disposition =
          ExactDirectSparseFacetDescentClosureDisposition::
              relative_positive;
      result.closure_summary.decision =
          ExactDirectSparseFacetDescentClosureDecision::
              complete_empty_seed_set;
      result.closure_summary.complete_relative_positive = true;
      result.closure_summary.graph_payload_persisted = false;
      if (proposal_transcript == nullptr) {
        result.locator_snapshot_stamp = locator.snapshot_stamp();
      }
      result.one_shared_closure_and_memo_built_or_empty_batch = true;
      result.common_frozen_locator_snapshot_certified =
          locator.snapshot_stamp() == result.locator_snapshot_stamp;
      result.shared_closure_complete_relative_positive = true;
      result.transient_closure_released_before_delta_publication = true;
      if (!result.common_frozen_locator_snapshot_certified) {
        throw std::runtime_error(
            "the locator changed after empty proposal consumption");
      }
    } else {
      CompactClosureProjection closure_projection =
          build_compact_closure_projection(
              index,
              cloud,
              distinct_keys,
              cursor.source_batch_index,
              source_batch.squared_level,
              locator_query_witness,
              locator,
              closure_budget,
              closure_config,
              traversal_order,
              proposal_transcript,
              proposal_evidence);
      result.closure_summary = closure_projection.summary;
      result.locator_snapshot_stamp =
          closure_projection.locator_snapshot_stamp;
      result.counters.shared_closure_build_count =
          closure_projection.closure_build_count;
      result.transient_closure_released_before_delta_publication = true;
      if (closure_projection.failure.has_value()) {
        return fail(
            std::move(result), *closure_projection.failure);
      }
      result.resolved_keys =
          std::move(closure_projection.resolved_keys);
      std::vector<ExactDirectSparseFacetKey>{}.swap(distinct_keys);
      result.counters.resolved_key_projection_count =
          result.resolved_keys.size();
      result.one_shared_closure_and_memo_built_or_empty_batch = true;
      result.common_frozen_locator_snapshot_certified =
          locator.snapshot_stamp() == result.locator_snapshot_stamp;
      result.shared_closure_complete_relative_positive =
          result.closure_summary.complete_relative_positive;
      if (!result.common_frozen_locator_snapshot_certified) {
        throw std::runtime_error(
            "the locator changed after compact closure projection");
      }
    }

    result.arm_joins.reserve(selected_arms.size());
    for (const SelectedArm& arm : selected_arms) {
      const auto found = std::lower_bound(
          result.resolved_keys.begin(),
          result.resolved_keys.end(),
          arm.source_facet_key,
          [](const ExactDirectSparseFacetDescentBatchResolvedKey& resolved,
             const ExactDirectSparseFacetKey& key) {
            return key_less(resolved.source_facet_key, key);
          });
      if (found == result.resolved_keys.end() ||
          found->source_facet_key != arm.source_facet_key) {
        return fail(
            std::move(result), BuildFailure::shared_closure_contradiction);
      }
      result.arm_joins.push_back(
          {arm.arm_seed_index,
           arm.family_index,
           arm.lane_index,
           found->resolved_key_index,
           true});
    }
    result.counters.arm_join_count = result.arm_joins.size();
    result.every_arm_joined_by_identity_and_full_key = true;
    result.batch_delta_ready_before_commit = true;
    result.decision =
        ExactDirectSparseFacetDescentBatchExecutionDecision::
            complete_architecture_only_relative_positive_batch_delta;
    return result;
  } catch (const std::bad_alloc&) {
    return fail(std::move(result), BuildFailure::allocation_failed);
  }
}

void account_transient_work(
    const ExactDirectSparseFacetDescentBatchExecutionResult& result,
    ExactDirectSparseFacetDescentBatchExecutionSessionAudit& audit) {
  const std::optional<std::size_t> next_closure_build_count =
      checked_add(
          audit.transient_closure_build_count,
          result.counters.shared_closure_build_count);
  if (!next_closure_build_count.has_value()) {
    throw std::overflow_error(
        "an anchored batch-execution audit counter overflows size_t");
  }
  audit.transient_closure_build_count = *next_closure_build_count;
  audit.maximum_transient_closure_node_count =
      std::max(
          audit.maximum_transient_closure_node_count,
          result.closure_summary.transient_node_count);
  audit.retained_closure_node_count = 0U;
  audit.closure_graph_retained_between_batches = false;
}

void increment_audit_counter(std::size_t& counter) {
  const std::optional<std::size_t> next = checked_add(counter, 1U);
  if (!next.has_value()) {
    throw std::overflow_error(
        "an anchored batch-execution audit counter overflows size_t");
  }
  counter = *next;
}

}  // namespace

bool ExactDirectSparseFacetDescentBatchExecutionResult::
complete_architecture_execution() const noexcept {
  const bool empty_batch =
      required_selected_arm_seed_count == 0U;
  const bool closure_shape =
      one_shared_closure_and_memo_built_or_empty_batch &&
      common_frozen_locator_snapshot_certified &&
      shared_closure_complete_relative_positive &&
      closure_summary.complete_relative_positive &&
      !closure_summary.graph_payload_persisted &&
      counters.shared_closure_build_count == (empty_batch ? 0U : 1U) &&
      closure_summary.transient_seed_projection_count ==
          required_resolved_key_count;
  return schema_version ==
             direct_sparse_facet_descent_batch_executor_schema_version &&
         source_plan_verified_once_at_session_open &&
         !source_plan_rebuilt_for_this_batch &&
         source_exact_batch_and_chunk_joined &&
         source_lanes_share_one_exact_batch &&
         batch_budget_preflight_completed &&
         batch_budget_preflight_satisfied &&
         selected_families_scanned_once &&
         every_arm_seed_selected_once_in_source_order &&
         facets_reconstructed_on_demand_only &&
         distinct_full_keys_canonicalized &&
         closure_shape &&
         every_arm_joined_by_identity_and_full_key &&
         transient_closure_released_before_delta_publication &&
         !closure_graph_persisted &&
         batch_delta_ready_before_commit &&
         !scientific_commit_performed && !locator_state_mutated &&
         !hierarchy_reduction_or_attachment_published &&
         !partial_closure_prefix_published_as_delta &&
         !facet_coface_cell_gamma_or_delaunay_materialized &&
         !gpu_execution_qualified && !sub_second_latency_claimed &&
         !ten_million_point_capacity_claimed &&
         !public_status_claimed &&
         resolved_keys.size() == required_resolved_key_count &&
         arm_joins.size() == required_selected_arm_seed_count &&
         counters.anchored_source_plan_verification_count == 1U &&
         counters.batch_local_source_plan_rebuild_count == 0U &&
         counters.selected_lane_reference_count ==
             required_selected_lane_count &&
         counters.selected_family_scan_count ==
             required_selected_family_count &&
         counters.selected_arm_seed_scan_count ==
             required_selected_arm_seed_count &&
         counters.reconstructed_facet_count ==
             required_selected_arm_seed_count &&
         counters.distinct_closure_seed_count ==
             required_resolved_key_count &&
         counters.resolved_key_projection_count ==
             required_resolved_key_count &&
         counters.arm_join_count == required_selected_arm_seed_count &&
         decision ==
             ExactDirectSparseFacetDescentBatchExecutionDecision::
                 complete_architecture_only_relative_positive_batch_delta &&
         scope ==
             ExactDirectSparseFacetDescentBatchExecutionScope::
                 one_exact_14c_batch_compact_relative_positive_delta_before_hierarchy_commit_only &&
         architecture_execution_payload_shape_is_consistent(*this);
}

bool ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult::
    complete_architecture_preparation() const noexcept {
  if (!scientific_delta.has_value() ||
      batch_diagnostic.has_value() ||
      !proposal_consumption_audit.has_value()) {
    return false;
  }
  const ExactDirectSparseFacetDescentBatchExecutionResult& delta =
      *scientific_delta;
  const ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit&
      proposal_audit = *proposal_consumption_audit;
  return schema_version ==
             direct_sparse_facet_descent_batch_top_k_proposal_preparation_schema_version &&
         source_batch_index == delta.source_batch_index &&
         source_batch_index == proposal_audit.source_batch_index &&
         closed_batch_squared_level ==
             delta.closed_batch_squared_level &&
         closed_batch_squared_level ==
             proposal_audit.closed_batch_squared_level &&
         locator_snapshot_stamp == delta.locator_snapshot_stamp &&
         locator_snapshot_stamp ==
             proposal_audit.locator_snapshot_stamp &&
         batch_execution_decision == delta.decision &&
         batch_execution_decision ==
             ExactDirectSparseFacetDescentBatchExecutionDecision::
                 complete_architecture_only_relative_positive_batch_delta &&
         proposal_consumption_decision ==
             ExactDirectSparseFacetDescentClosureTopKProposalConsumptionDecision::
                 complete_exact_closure_with_proposal_consumption_audit &&
         delta.complete_architecture_execution() &&
         proposal_audit.schema_version ==
             direct_sparse_facet_descent_closure_top_k_proposal_consumption_schema_version &&
         proposal_audit.closed_batch_squared_level ==
             delta.closed_batch_squared_level &&
         proposal_audit.locator_snapshot_stamp ==
             delta.locator_snapshot_stamp &&
         proposal_audit.canonical_seed_key_count ==
             delta.required_resolved_key_count &&
         proposal_audit.closure_build_count == 1U &&
         proposal_audit.top_k_query_count ==
             delta.closure_summary.counters.aggregate_step_counters
                 .top_k_query_count &&
         proposal_consumption_audit_consistent_with_delta(
             proposal_audit, delta) &&
         proposal_audit.transcript_complete_revalidated &&
         proposal_audit
             .metadata_matches_requested_batch_level_and_live_locator &&
         proposal_audit.canonical_seed_keys_revalidated &&
         proposal_audit.record_keys_are_canonical_seed_subset &&
         proposal_audit.candidate_point_domains_revalidated &&
         proposal_audit.locator_snapshot_stable_during_atomic_validation &&
         proposal_audit
             .every_top_k_query_used_exact_source_facet_baseline &&
         proposal_audit.nonempty_records_passed_only_as_proposals &&
         proposal_audit
             .empty_missing_and_dynamic_fallbacks_used_empty_proposal_pool &&
         proposal_audit.exact_pool_and_spatial_work_accounted &&
         !proposal_audit.transcript_payload_persisted &&
         !proposal_audit.top_k_partition_or_shell_persisted &&
         !proposal_audit.scientific_decision_taken_from_proposal &&
         !proposal_audit.locator_state_mutated &&
         !proposal_audit.public_status_claimed &&
         transcript_consumption_requested &&
         batch_preflight_completed_before_proposal_consumption &&
         transcript_validation_completed_synchronously &&
         exact_proposal_consumption_certified &&
         !atomic_transcript_rejection_certified &&
         !no_closure_constructed_on_transcript_rejection &&
         scientific_delta_separated_from_proposal_audit &&
         no_transcript_record_candidate_partition_or_shell_persisted &&
         no_proposal_payload_or_audit_retained_by_session &&
         preparation_left_session_cursor_unchanged &&
         standard_commit_replays_unseeded_exact_path &&
         operational_audit_has_no_scientific_authority &&
         !unseeded_commit_readiness_claimed &&
         !scientific_commit_performed && !locator_state_mutated &&
         !hierarchy_reduction_or_attachment_published &&
         !gpu_execution_qualified && !sub_second_latency_claimed &&
         !ten_million_point_capacity_claimed &&
         !public_status_claimed &&
         decision ==
             ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision::
                 complete_architecture_only_scientific_delta_with_separate_proposal_audit &&
         scope ==
             ExactDirectSparseFacetDescentBatchTopKProposalPreparationScope::
                 one_synchronous_non_authoritative_proposal_preparation_before_unseeded_exact_commit_replay_only;
}

bool ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult::
    certified_atomic_transcript_rejection() const noexcept {
  if (scientific_delta.has_value() ||
      batch_diagnostic.has_value() ||
      !proposal_consumption_audit.has_value()) {
    return false;
  }
  const ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit&
      proposal_audit = *proposal_consumption_audit;
  return schema_version ==
             direct_sparse_facet_descent_batch_top_k_proposal_preparation_schema_version &&
         source_batch_index == proposal_audit.source_batch_index &&
         closed_batch_squared_level ==
             proposal_audit.closed_batch_squared_level &&
         locator_snapshot_stamp ==
             proposal_audit.locator_snapshot_stamp &&
         batch_execution_decision ==
             ExactDirectSparseFacetDescentBatchExecutionDecision::
                 no_execution_shared_closure_rejected &&
         known_atomic_transcript_rejection_decision(
             proposal_consumption_decision) &&
         proposal_audit.schema_version ==
             direct_sparse_facet_descent_closure_top_k_proposal_consumption_schema_version &&
         proposal_rejection_audit_has_no_exact_work(proposal_audit) &&
         !proposal_audit.transcript_payload_persisted &&
         !proposal_audit.top_k_partition_or_shell_persisted &&
         !proposal_audit.scientific_decision_taken_from_proposal &&
         !proposal_audit.locator_state_mutated &&
         !proposal_audit.public_status_claimed &&
         transcript_consumption_requested &&
         batch_preflight_completed_before_proposal_consumption &&
         transcript_validation_completed_synchronously &&
         !exact_proposal_consumption_certified &&
         atomic_transcript_rejection_certified &&
         no_closure_constructed_on_transcript_rejection &&
         !scientific_delta_separated_from_proposal_audit &&
         no_transcript_record_candidate_partition_or_shell_persisted &&
         no_proposal_payload_or_audit_retained_by_session &&
         preparation_left_session_cursor_unchanged &&
         standard_commit_replays_unseeded_exact_path &&
         operational_audit_has_no_scientific_authority &&
         !unseeded_commit_readiness_claimed &&
         !scientific_commit_performed && !locator_state_mutated &&
         !hierarchy_reduction_or_attachment_published &&
         !gpu_execution_qualified && !sub_second_latency_claimed &&
         !ten_million_point_capacity_claimed &&
         !public_status_claimed &&
         decision ==
             ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision::
                 no_preparation_transcript_rejected_before_closure &&
         scope ==
             ExactDirectSparseFacetDescentBatchTopKProposalPreparationScope::
                 one_synchronous_non_authoritative_proposal_preparation_before_unseeded_exact_commit_replay_only;
}

bool ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult::
    certified_outcome() const noexcept {
  return complete_architecture_preparation() ||
         certified_atomic_transcript_rejection();
}

ExactDirectSparseFacetDescentAnchoredBatchExecutor::
    ExactDirectSparseFacetDescentAnchoredBatchExecutor(
        const spatial::MortonLbvhIndex& index,
        const spatial::CanonicalPointCloud& cloud,
        const ExactDirectSupportTerminalFacade& source_facade,
        const ExactDirectMorseEventJournalResult& source_event_journal,
        const ExactDirectSaddleArmSeedBudget& trusted_arm_seed_budget,
        const ExactDirectSaddleArmSeedJournalResult&
            source_arm_seed_journal,
        const ExactDirectMorseIndustrialPlanConfig& industrial_config,
        const ExactDirectSparseFacetDescentBatchPlanBudget& plan_budget,
        const ExactDirectSparseFacetDescentBatchPlanResult& observed_plan,
        const ExactDirectSparsePositiveFacetLocator& locator,
        const ExactDirectSparseFacetDescentClosureConfig& closure_config,
        spatial::LbvhTraversalOrder traversal_order)
    : index_(&index),
      cloud_(&cloud),
      source_facade_(&source_facade),
      source_event_journal_(&source_event_journal),
      trusted_arm_seed_budget_(trusted_arm_seed_budget),
      source_arm_seed_journal_(&source_arm_seed_journal),
      industrial_config_(industrial_config),
      plan_budget_(plan_budget),
      locator_(&locator),
      closure_config_(closure_config),
      traversal_order_(traversal_order) {
  require_valid_traversal_order(traversal_order_);
  if (!index_->validated_for(*cloud_)) {
    throw std::invalid_argument(
        "an anchored batch executor requires a matching LBVH authority");
  }
  if (!locator_->certified_positive_locator()) {
    throw std::invalid_argument(
        "an anchored batch executor requires a certified locator");
  }
  source_plan_ = build_exact_direct_sparse_facet_descent_batch_plan(
      *cloud_,
      *source_facade_,
      *source_event_journal_,
      trusted_arm_seed_budget_,
      *source_arm_seed_journal_,
      industrial_config_,
      plan_budget_);
  if (!source_plan_.complete_architecture_plan() ||
      source_plan_ != observed_plan) {
    throw std::invalid_argument(
        "an anchored batch executor requires one freshly matching 14C plan");
  }
  audit_.source_plan_verification_count = 1U;
  audit_.source_plan_owned_by_session = true;
  audit_.full_source_plan_replayed_per_batch = false;
  audit_.closure_graph_retained_between_batches = false;
  audit_.proposal_payload_or_audit_retained_between_calls = false;
}

bool ExactDirectSparseFacetDescentAnchoredBatchExecutor::complete()
    const noexcept {
  return next_source_batch_index_ ==
         source_event_journal_->batches.size();
}

ExactDirectSparseFacetDescentBatchExecutionResult
ExactDirectSparseFacetDescentAnchoredBatchExecutor::prepare_next(
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparseFacetDescentBatchExecutionBudget&
        execution_budget,
    const ExactDirectSparseFacetDescentClosureBudget& closure_budget) {
  if (complete()) {
    throw std::logic_error(
        "a complete anchored batch executor has no next batch");
  }
  const BatchCursor cursor{
      next_source_batch_index_,
      next_source_chunk_index_,
      next_source_lane_index_,
      next_source_family_index_,
      next_source_arm_seed_index_};
  ExactDirectSparseFacetDescentBatchExecutionResult result =
      build_batch_execution(
          *index_,
          *cloud_,
          *source_facade_,
          *source_event_journal_,
          *source_arm_seed_journal_,
          source_plan_,
          cursor,
          locator_query_witness,
          *locator_,
          execution_budget,
          closure_budget,
          closure_config_,
          traversal_order_,
          nullptr,
          nullptr);
  increment_audit_counter(audit_.prepare_attempt_count);
  account_transient_work(result, audit_);
  return result;
}

ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult
ExactDirectSparseFacetDescentAnchoredBatchExecutor::
    prepare_next_with_top_k_proposal_transcript(
        const ExactDirectSparseFacetWitness& locator_query_witness,
        const ExactDirectSparseFacetDescentBatchExecutionBudget&
            execution_budget,
        const ExactDirectSparseFacetDescentClosureBudget& closure_budget,
        const ExactDirectSparseFacetTopKProposalTranscriptResult&
            transcript) {
  if (complete()) {
    throw std::logic_error(
        "a complete anchored batch executor has no next batch");
  }
  const BatchCursor cursor{
      next_source_batch_index_,
      next_source_chunk_index_,
      next_source_lane_index_,
      next_source_family_index_,
      next_source_arm_seed_index_};
  ProposalPreparationEvidence proposal_evidence;
  ExactDirectSparseFacetDescentBatchExecutionResult batch_execution =
      build_batch_execution(
          *index_,
          *cloud_,
          *source_facade_,
          *source_event_journal_,
          *source_arm_seed_journal_,
          source_plan_,
          cursor,
          locator_query_witness,
          *locator_,
          execution_budget,
          closure_budget,
          closure_config_,
          traversal_order_,
          &transcript,
          &proposal_evidence);
  increment_audit_counter(audit_.prepare_attempt_count);
  increment_audit_counter(audit_.proposal_prepare_attempt_count);
  account_transient_work(batch_execution, audit_);

  ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult result;
  result.source_batch_index = cursor.source_batch_index;
  result.closed_batch_squared_level =
      batch_execution.closed_batch_squared_level;
  result.locator_snapshot_stamp =
      batch_execution.locator_snapshot_stamp;
  result.batch_execution_decision = batch_execution.decision;
  result.transcript_consumption_requested = true;
  result.batch_preflight_completed_before_proposal_consumption =
      proposal_evidence.consumption_attempted &&
      batch_execution.batch_budget_preflight_completed &&
      batch_execution.batch_budget_preflight_satisfied;
  result.transcript_validation_completed_synchronously =
      proposal_evidence.validation_completed_before_closure;
  result.exact_proposal_consumption_certified =
      proposal_evidence.exact_consumption_certified;
  result.atomic_transcript_rejection_certified =
      proposal_evidence.atomic_rejection_certified;
  result.no_closure_constructed_on_transcript_rejection =
      proposal_evidence.no_closure_constructed_on_rejection;
  result.scientific_delta_separated_from_proposal_audit =
      proposal_evidence
          .scientific_closure_separated_from_consumption_audit;
  result.no_transcript_record_candidate_partition_or_shell_persisted =
      proposal_evidence
          .no_transcript_or_top_k_payload_persisted_in_closure &&
      (!proposal_evidence.consumption_audit.has_value() ||
       (!proposal_evidence.consumption_audit
             ->transcript_payload_persisted &&
        !proposal_evidence.consumption_audit
             ->top_k_partition_or_shell_persisted));
  result.no_proposal_payload_or_audit_retained_by_session = true;
  result.standard_commit_replays_unseeded_exact_path = true;
  result.operational_audit_has_no_scientific_authority = true;
  result.unseeded_commit_readiness_claimed = false;
  result.scientific_commit_performed = false;
  result.locator_state_mutated = false;
  result.hierarchy_reduction_or_attachment_published = false;
  result.gpu_execution_qualified = false;
  result.sub_second_latency_claimed = false;
  result.ten_million_point_capacity_claimed = false;
  result.public_status_claimed = false;
  result.scope =
      ExactDirectSparseFacetDescentBatchTopKProposalPreparationScope::
          one_synchronous_non_authoritative_proposal_preparation_before_unseeded_exact_commit_replay_only;

  if (proposal_evidence.consumption_attempted) {
    increment_audit_counter(audit_.proposal_consumption_attempt_count);
    if (proposal_evidence.consumption_audit.has_value()) {
      const std::optional<std::size_t> next_proposal_closure_call_count =
          checked_add(
              audit_.proposal_exact_closure_call_count,
              proposal_evidence.consumption_audit
                  ->closure_build_count);
      if (!next_proposal_closure_call_count.has_value()) {
        throw std::overflow_error(
            "an anchored proposal-closure audit counter overflows size_t");
      }
      audit_.proposal_exact_closure_call_count =
          *next_proposal_closure_call_count;
    }
    result.proposal_consumption_decision =
        proposal_evidence.consumption_decision;
    result.proposal_consumption_audit =
        std::move(proposal_evidence.consumption_audit);
    if (proposal_evidence.atomic_rejection_certified) {
      increment_audit_counter(audit_.proposal_transcript_rejection_count);
      result.decision =
          ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision::
              no_preparation_transcript_rejected_before_closure;
    } else if (proposal_evidence.exact_consumption_certified &&
               batch_execution.complete_architecture_execution()) {
      result.scientific_delta.emplace(std::move(batch_execution));
      result.decision =
          ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision::
              complete_architecture_only_scientific_delta_with_separate_proposal_audit;
    } else if (proposal_evidence.exact_consumption_certified) {
      result.batch_diagnostic.emplace(std::move(batch_execution));
      result.decision =
          ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision::
              no_preparation_batch_diagnostic_after_exact_proposal_consumption;
    } else {
      result.batch_diagnostic.emplace(std::move(batch_execution));
      result.decision =
          ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision::
              no_preparation_batch_diagnostic_during_proposal_consumption;
    }
  } else {
    result.batch_diagnostic.emplace(std::move(batch_execution));
    result.decision =
        ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision::
            no_preparation_batch_diagnostic_before_proposal_consumption;
  }

  audit_.retained_proposal_record_count = 0U;
  audit_.proposal_payload_or_audit_retained_between_calls = false;
  result.preparation_left_session_cursor_unchanged =
      next_source_batch_index_ == cursor.source_batch_index &&
      next_source_chunk_index_ == cursor.source_chunk_index &&
      next_source_lane_index_ == cursor.source_lane_index &&
      next_source_family_index_ == cursor.source_family_index &&
      next_source_arm_seed_index_ == cursor.source_arm_seed_index;
  return result;
}

ExactDirectSparseFacetDescentBatchExecutionVerification
ExactDirectSparseFacetDescentAnchoredBatchExecutor::commit_prepared(
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparseFacetDescentBatchExecutionBudget&
        execution_budget,
    const ExactDirectSparseFacetDescentClosureBudget& closure_budget,
    const ExactDirectSparseFacetDescentBatchExecutionResult& observed) {
  ExactDirectSparseFacetDescentBatchExecutionVerification verification;
  require_execution_budget_within_confidence(execution_budget);
  require_closure_budget_within_confidence(closure_budget);
  verification.observed_storage_within_budget =
      observed_storage_within_budget(observed, execution_budget);
  verification.anchored_source_plan_reused_without_full_replay =
      audit_.source_plan_verification_count == 1U &&
      !audit_.full_source_plan_replayed_per_batch &&
      audit_.source_plan_owned_by_session;
  if (!verification.observed_storage_within_budget || complete()) {
    increment_audit_counter(audit_.rejected_batch_replay_count);
    return verification;
  }

  const BatchCursor cursor{
      next_source_batch_index_,
      next_source_chunk_index_,
      next_source_lane_index_,
      next_source_family_index_,
      next_source_arm_seed_index_};
  const ExactDirectSparseFacetDescentBatchExecutionResult expected =
      build_batch_execution(
          *index_,
          *cloud_,
          *source_facade_,
          *source_event_journal_,
          *source_arm_seed_journal_,
          source_plan_,
          cursor,
          locator_query_witness,
          *locator_,
          execution_budget,
          closure_budget,
          closure_config_,
          traversal_order_,
          nullptr,
          nullptr);
  increment_audit_counter(audit_.fresh_batch_replay_count);
  account_transient_work(expected, audit_);

  verification.locator_snapshot_matches_observed_build =
      observed.locator_snapshot_stamp ==
          expected.locator_snapshot_stamp &&
      locator_->snapshot_stamp() == expected.locator_snapshot_stamp;
  verification.exact_batch_execution_freshly_replayed = true;
  verification.exact_result_equality_certified = observed == expected;
  verification.transient_closure_released_before_comparison =
      expected.transient_closure_released_before_delta_publication &&
      !expected.closure_graph_persisted &&
      audit_.retained_closure_node_count == 0U;
  verification.no_partial_delta_or_external_mutation =
      !expected.partial_closure_prefix_published_as_delta &&
      !expected.locator_state_mutated &&
      !expected.scientific_commit_performed &&
      !expected.hierarchy_reduction_or_attachment_published;
  verification.no_forbidden_global_structure_materialized =
      !expected.facet_coface_cell_gamma_or_delaunay_materialized;
  verification.no_scale_or_public_status_claimed =
      !expected.gpu_execution_qualified &&
      !expected.sub_second_latency_claimed &&
      !expected.ten_million_point_capacity_claimed &&
      !expected.public_status_claimed;
  verification.diagnostic_result_certified =
      verification.anchored_source_plan_reused_without_full_replay &&
      verification.locator_snapshot_matches_observed_build &&
      verification.exact_batch_execution_freshly_replayed &&
      verification.exact_result_equality_certified &&
      verification.transient_closure_released_before_comparison &&
      verification.no_partial_delta_or_external_mutation &&
      verification.no_forbidden_global_structure_materialized &&
      verification.no_scale_or_public_status_claimed;
  verification.result_certified =
      verification.diagnostic_result_certified;

  if (!verification.result_certified ||
      !expected.complete_architecture_execution()) {
    increment_audit_counter(audit_.rejected_batch_replay_count);
    return verification;
  }

  const std::size_t advanced_batch_index =
      next_source_batch_index_ + 1U;
  std::size_t advanced_chunk_index = next_source_chunk_index_;
  while (advanced_chunk_index <
             source_plan_.source_industrial_plan.chunks.size() &&
         advanced_batch_index >=
             source_plan_.source_industrial_plan
                 .chunks[advanced_chunk_index]
                 .source_batch_end_index) {
    ++advanced_chunk_index;
  }
  const bool terminal = advanced_batch_index ==
                        source_event_journal_->batches.size();
  if ((terminal &&
       (expected.source_lane_end_index != source_plan_.lanes.size() ||
        expected.source_family_end_index !=
            source_arm_seed_journal_->families.size() ||
        expected.source_arm_seed_end_index !=
            source_arm_seed_journal_->arm_seeds.size() ||
        advanced_chunk_index !=
            source_plan_.source_industrial_plan.chunks.size())) ||
      (!terminal &&
       advanced_chunk_index >=
           source_plan_.source_industrial_plan.chunks.size())) {
    increment_audit_counter(audit_.rejected_batch_replay_count);
    verification.result_certified = false;
    return verification;
  }

  next_source_batch_index_ = advanced_batch_index;
  next_source_chunk_index_ = advanced_chunk_index;
  next_source_lane_index_ = expected.source_lane_end_index;
  next_source_family_index_ = expected.source_family_end_index;
  next_source_arm_seed_index_ = expected.source_arm_seed_end_index;
  increment_audit_counter(audit_.accepted_batch_count);
  verification.session_advanced = true;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
