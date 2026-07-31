#include "morsehgp3d/hierarchy/direct_morse_terminal_forest_reduction.hpp"

#include "morsehgp3d/hierarchy/direct_sparse_facet_top_k_proposal_transcript.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

using Clock = std::chrono::steady_clock;

[[nodiscard]] std::uint64_t elapsed_nanoseconds(
    Clock::time_point begin,
    Clock::time_point end) noexcept {
  const auto elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin)
          .count();
  if (elapsed <= 0) {
    return 0U;
  }
  return static_cast<std::uint64_t>(elapsed);
}

[[nodiscard]] ExactDirectSparseFacetTopKProposalTranscriptResult
empty_proposal_transcript(
    std::size_t source_batch_index,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparsePositiveFacetLocator& locator) {
  const std::array<ExactDirectSparseFacetTopKProposalRecord, 0U> records{};
  return build_exact_direct_sparse_facet_top_k_proposal_transcript(
      {source_batch_index,
       closed_batch_squared_level,
       locator.snapshot_stamp()},
      records,
      {0U, 0U, 0U, 0U, 0U});
}

void finish_timing(
    ExactDirectMorseTerminalForestReductionAudit& audit,
    Clock::time_point total_begin) noexcept {
  audit.timings.total_wall_nanoseconds =
      elapsed_nanoseconds(total_begin, Clock::now());
}

}  // namespace

bool ExactDirectMorseTerminalForestReductionAudit::certified_reduction()
    const noexcept {
  return schema_version ==
             direct_morse_terminal_forest_reduction_schema_version &&
      point_count != 0U && requested_maximum_order != 0U &&
      effective_maximum_order != 0U && source_batch_count != 0U &&
      prepared_ticket_count == source_batch_count &&
      committed_batch_count == source_batch_count &&
      source_bridge_certified && source_manifest_certified &&
      provider_constructor_used && source_plan_complete &&
      executor_and_reducer_shared_one_locator && every_ticket_prepared &&
      every_live_commit_certified && executor_complete && reducer_complete &&
      forest_materialized && forest_conditional_h0_candidate_certified &&
      no_source_window_or_batch_delta_retained &&
      !forbidden_global_structure_materialized &&
      hierarchy_reduction_performed && !public_status_claimed &&
      decision == ExactDirectMorseTerminalForestReductionDecision::
                      complete_conditional_h0_forest;
}

bool ExactDirectMorseTerminalForestReductionResult::certified_reduction()
    const noexcept {
  return audit.certified_reduction() && forest.has_value() &&
      forest->certified_conditional_h0_candidate() &&
      !forest->forbidden_global_structure_materialized &&
      !forest->gamma_cells_or_global_cofaces_materialized &&
      !forest->higher_order_delaunay_materialized &&
      !forest->public_status_claimed;
}

ExactDirectMorseTerminalForestReductionResult
reduce_exact_direct_morse_terminal_source_to_forest(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    ExactDirectMorseTerminalReducerSourceBridge& source_bridge,
    const ExactDirectMorseTerminalForestReductionConfig& config) {
  const Clock::time_point total_begin = Clock::now();
  ExactDirectMorseTerminalForestReductionResult result;
  auto& audit = result.audit;
  audit.point_count = cloud.size();
  audit.requested_maximum_order =
      source_bridge.audit().requested_maximum_order;
  audit.effective_maximum_order =
      source_bridge.audit().effective_maximum_order;
  audit.source_batch_count = source_bridge.source_manifest().batch_count;
  audit.source_bridge_certified = source_bridge.audit().certified();
  audit.source_manifest_certified =
      source_bridge.source_manifest().certified();
  if (!audit.source_bridge_certified || !audit.source_manifest_certified) {
    audit.decision = ExactDirectMorseTerminalForestReductionDecision::
        no_reduction_bridge_not_certified;
    finish_timing(audit, total_begin);
    return result;
  }

  const Clock::time_point plan_begin = Clock::now();
  const ExactDirectSparseFacetDescentBatchPlanResult plan =
      build_exact_direct_sparse_facet_descent_batch_plan(
          cloud,
          source_bridge.direct_support_facade(),
          source_bridge.event_journal(),
          source_bridge.trusted_seed_budget(),
          source_bridge.saddle_arm_seed_journal(),
          config.industrial_config,
          config.plan_budget);
  const Clock::time_point plan_end = Clock::now();
  audit.timings.plan_wall_nanoseconds =
      elapsed_nanoseconds(plan_begin, plan_end);
  audit.plan_decision = plan.decision;
  audit.plan_lane_count = plan.lanes.size();
  audit.source_plan_complete = plan.complete_architecture_plan();
  audit.forbidden_global_structure_materialized =
      plan.forbidden_global_structure_materialized;
  if (!audit.source_plan_complete) {
    audit.decision = ExactDirectMorseTerminalForestReductionDecision::
        no_reduction_source_plan_rejected;
    finish_timing(audit, total_begin);
    return result;
  }

  const Clock::time_point setup_begin = Clock::now();
  auto provider = source_bridge.source_provider();
  ExactDirectMorseForestReducer reducer(
      source_bridge.source_manifest(),
      provider,
      config.forest_budget,
      config.forest_config,
      config.traversal_order);
  audit.provider_constructor_used = true;
  ExactDirectSparseFacetDescentAnchoredBatchExecutor executor(
      index,
      cloud,
      source_bridge.direct_support_facade(),
      source_bridge.event_journal(),
      source_bridge.trusted_seed_budget(),
      source_bridge.saddle_arm_seed_journal(),
      config.industrial_config,
      config.plan_budget,
      plan,
      reducer.strict_locator(),
      config.forest_config.closure_config,
      config.traversal_order);
  const Clock::time_point setup_end = Clock::now();
  audit.timings.reducer_setup_wall_nanoseconds =
      elapsed_nanoseconds(setup_begin, setup_end);
  audit.executor_and_reducer_shared_one_locator = true;
  audit.every_ticket_prepared = true;
  audit.every_live_commit_certified = true;

  const Clock::time_point stream_begin = Clock::now();
  while (!executor.complete()) {
    const std::size_t batch_index = executor.next_source_batch_index();
    if (batch_index >= source_bridge.event_journal().batches.size()) {
      audit.every_ticket_prepared = false;
      audit.every_live_commit_certified = false;
      audit.decision = ExactDirectMorseTerminalForestReductionDecision::
          no_reduction_executor_cursor_out_of_range;
      audit.timings.reducer_stream_wall_nanoseconds =
          elapsed_nanoseconds(stream_begin, Clock::now());
      finish_timing(audit, total_begin);
      return result;
    }
    if (batch_index >
        std::numeric_limits<std::uint64_t>::max() / 3U - 1U) {
      audit.every_ticket_prepared = false;
      audit.every_live_commit_certified = false;
      audit.decision = ExactDirectMorseTerminalForestReductionDecision::
          no_reduction_batch_token_overflow;
      audit.timings.reducer_stream_wall_nanoseconds =
          elapsed_nanoseconds(stream_begin, Clock::now());
      finish_timing(audit, total_begin);
      return result;
    }
    const ExactDirectSparseFacetWitness witness{
        config.forest_config.locator_config.external_authority_id,
        (static_cast<std::uint64_t>(batch_index) + 1U) * 3U};
    const auto transcript = empty_proposal_transcript(
        batch_index,
        source_bridge.event_journal().batches[batch_index].squared_level,
        reducer.strict_locator());
    auto ticket =
        executor.prepare_next_sealed_with_top_k_proposal_transcript(
            witness,
            config.execution_budget,
            config.forest_budget.closure_budget,
            transcript);
    audit.last_preparation_decision = ticket.preparation().decision;
    audit.last_batch_execution_decision =
        ticket.preparation().batch_execution_decision;
    if (!ticket.prepared()) {
      audit.every_ticket_prepared = false;
      audit.every_live_commit_certified = false;
      audit.decision = ExactDirectMorseTerminalForestReductionDecision::
          no_reduction_ticket_preparation_rejected;
      audit.timings.reducer_stream_wall_nanoseconds =
          elapsed_nanoseconds(stream_begin, Clock::now());
      finish_timing(audit, total_begin);
      return result;
    }
    ++audit.prepared_ticket_count;
    const ExactDirectMorseForestLiveCommitResult committed =
        reducer.fold_prepared_ticket(executor, std::move(ticket));
    audit.last_live_commit_decision = committed.decision;
    audit.last_reducer_fold_decision = committed.reducer_fold.decision;
    audit.forbidden_global_structure_materialized =
        audit.forbidden_global_structure_materialized ||
        committed.forbidden_global_structure_materialized;
    if (!committed.certified_live_commit() ||
        !committed.scientific_delta.has_value()) {
      audit.every_live_commit_certified = false;
      audit.decision = ExactDirectMorseTerminalForestReductionDecision::
          no_reduction_live_commit_rejected;
      audit.timings.reducer_stream_wall_nanoseconds =
          elapsed_nanoseconds(stream_begin, Clock::now());
      finish_timing(audit, total_begin);
      return result;
    }
    ++audit.committed_batch_count;
    audit.aggregate_resolved_key_count +=
        committed.scientific_delta->resolved_keys.size();
    audit.aggregate_arm_join_count +=
        committed.scientific_delta->arm_joins.size();
    audit.maximum_transient_closure_node_count = std::max(
        audit.maximum_transient_closure_node_count,
        committed.scientific_delta->closure_summary.transient_node_count);
    // committed and its compact delta die at the end of this iteration; the
    // reducer retains only locator/carrier/forest state.
  }
  const Clock::time_point stream_end = Clock::now();
  audit.timings.reducer_stream_wall_nanoseconds =
      elapsed_nanoseconds(stream_begin, stream_end);
  audit.executor_complete = executor.complete();
  audit.reducer_complete = reducer.complete();
  audit.no_source_window_or_batch_delta_retained =
      audit.executor_complete && audit.reducer_complete;
  if (!audit.executor_complete || !audit.reducer_complete) {
    audit.decision = ExactDirectMorseTerminalForestReductionDecision::
        no_reduction_stream_incomplete;
    finish_timing(audit, total_begin);
    return result;
  }

  const Clock::time_point finish_begin = Clock::now();
  try {
    result.forest.emplace(reducer.finish());
  } catch (const std::logic_error&) {
    audit.timings.forest_finish_wall_nanoseconds =
        elapsed_nanoseconds(finish_begin, Clock::now());
    audit.decision = ExactDirectMorseTerminalForestReductionDecision::
        no_reduction_forest_finish_rejected;
    finish_timing(audit, total_begin);
    return result;
  }
  const Clock::time_point finish_end = Clock::now();
  audit.timings.forest_finish_wall_nanoseconds =
      elapsed_nanoseconds(finish_begin, finish_end);
  const auto& forest = *result.forest;
  audit.forest_materialized = true;
  audit.forest_conditional_h0_candidate_certified =
      forest.certified_conditional_h0_candidate();
  audit.forbidden_global_structure_materialized =
      audit.forbidden_global_structure_materialized ||
      forest.forbidden_global_structure_materialized ||
      forest.gamma_cells_or_global_cofaces_materialized ||
      forest.higher_order_delaunay_materialized;
  audit.forest_birth_record_count = forest.counters.birth_record_count;
  audit.forest_saddle_record_count = forest.counters.saddle_record_count;
  audit.forest_atomic_group_count = forest.counters.atomic_group_count;
  audit.forest_node_count = forest.counters.node_count;
  audit.forest_final_root_count = forest.counters.final_root_count;
  audit.forest_logical_output_entry_count =
      forest.logical_output_entry_count;
  audit.hierarchy_reduction_performed = true;
  audit.decision = ExactDirectMorseTerminalForestReductionDecision::
      complete_conditional_h0_forest;
  finish_timing(audit, total_begin);
  return result;
}

}  // namespace morsehgp3d::hierarchy
