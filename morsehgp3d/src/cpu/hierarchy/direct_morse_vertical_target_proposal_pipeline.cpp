#include "morsehgp3d/hierarchy/direct_morse_vertical_target_proposal_pipeline.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

using PipelineBudget =
    ExactDirectMorseVerticalTargetProposalPipelineBudget;
using PipelineCounters =
    ExactDirectMorseVerticalTargetProposalPipelineCounters;
using PipelineDecision =
    ExactDirectMorseVerticalTargetProposalPipelineDecision;
using PipelineGroupAudit =
    ExactDirectMorseVerticalTargetProposalPipelineGroupAudit;
using PipelineResult =
    ExactDirectMorseVerticalTargetProposalPipelineResult;
using PipelineScope = ExactDirectMorseVerticalTargetProposalPipelineScope;
using PipelineSessionAudit =
    ExactDirectMorseVerticalTargetProposalPipelineSessionAudit;

[[nodiscard]] bool checked_add(
    std::size_t left,
    std::size_t right,
    std::size_t& output) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  output = left + right;
  return true;
}

[[nodiscard]] bool checked_multiply(
    std::size_t left,
    std::size_t right,
    std::size_t& output) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  output = left * right;
  return true;
}

[[nodiscard]] bool checked_query_token(
    std::size_t source_atomic_group_index,
    std::uint64_t& token) noexcept {
  constexpr std::uint64_t maximum =
      std::numeric_limits<std::uint64_t>::max();
  if constexpr (
      std::numeric_limits<std::size_t>::max() >
      std::numeric_limits<std::uint64_t>::max()) {
    if (source_atomic_group_index >
        static_cast<std::size_t>(maximum)) {
      return false;
    }
  }
  const auto group =
      static_cast<std::uint64_t>(source_atomic_group_index);
  if (group > maximum / UINT64_C(3) - UINT64_C(1)) {
    return false;
  }
  token = UINT64_C(3) * (group + UINT64_C(1));
  return token != 0U;
}

[[nodiscard]] bool closure_decision_complete(
    ExactDirectMorseForestCarrierCutClosureAdapterDecision decision)
    noexcept {
  using Decision =
      ExactDirectMorseForestCarrierCutClosureAdapterDecision;
  return decision == Decision::complete_empty_key_set ||
         decision == Decision::complete_all_positive_terminal_summaries ||
         decision == Decision::complete_with_unresolved_terminal_summaries;
}

[[nodiscard]] bool adapter_decision_complete(
    ExactDirectMorseVerticalTargetProposalAdapterDecision decision)
    noexcept {
  using Decision = ExactDirectMorseVerticalTargetProposalAdapterDecision;
  return decision ==
             Decision::
                 complete_all_resolved_group_local_vertical_target_proposals ||
         decision ==
             Decision::
                 complete_with_unresolved_group_local_vertical_target_proposals;
}

[[nodiscard]] bool initialization_decision_complete(
    ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision
        decision) noexcept {
  return decision ==
         ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
             complete_certified_replay_session_ready;
}

[[nodiscard]] bool pipeline_non_scope_honest(
    const PipelineResult& result) noexcept {
  return result.matching_canonical_point_namespace_required &&
         !result.forest_to_cloud_namespace_identity_certified &&
         result.forest_relative_only &&
         !result.external_target_authority_replayed &&
         !result.vertical_maps_complete &&
         !result.global_facet_coface_incidence_cell_gamma_or_delaunay_materialized &&
         !result.higher_order_delaunay_materialized &&
         !result.public_status_claimed;
}

void fail(PipelineResult& result, PipelineDecision decision) noexcept {
  result.proposals.clear();
  result.session_audits.clear();
  result.group_audits.clear();
  result.one_session_per_referenced_target_order = false;
  result.source_groups_processed_in_batch_then_group_order = false;
  result.each_session_advanced_monotonically = false;
  result.query_tokens_follow_three_times_group_index_plus_one = false;
  result.query_tokens_disjoint_from_birth_and_union_domains = false;
  result.every_group_plan_certified = false;
  result.every_closure_summary_certified = false;
  result.every_group_adapter_certified = false;
  result.every_advance_post_stamp_certified_before_publication = false;
  result.output_preallocated_after_aggregate_preflight = false;
  result.proposals_sorted_unique_by_binding_index = false;
  result.source_binding_partition_proved_global_output_order = false;
  result.only_final_proposals_and_scalar_audits_retained = false;
  result.no_plan_closure_locator_or_session_reference_retained = false;
  result.no_partial_scientific_payload_published_on_failure = true;
  result.decision = decision;
  result.scope = PipelineScope::unspecified;
}

[[nodiscard]] PipelineDecision map_plan_failure(
    ExactDirectMorseVerticalTargetFacetPlanDecision decision) noexcept {
  using Decision = ExactDirectMorseVerticalTargetFacetPlanDecision;
  switch (decision) {
    case Decision::no_plan_capacity_overflow:
      return PipelineDecision::no_pipeline_capacity_overflow;
    case Decision::no_plan_budget_exhausted:
      return PipelineDecision::no_pipeline_budget_exhausted;
    case Decision::no_plan_allocation_failed:
      return PipelineDecision::no_pipeline_allocation_failed;
    default:
      return PipelineDecision::no_pipeline_facet_plan_rejected;
  }
}

struct GroupDescriptor {
  std::size_t source_atomic_group_index{};
  std::size_t source_batch_index{};
  std::size_t target_order{};
  std::size_t session_index{};
  std::uint64_t query_token{};
};

struct WorkingSession {
  std::size_t target_order{};
  std::unique_ptr<ExactDirectMorseForestCarrierCutReplaySession> session;
  PipelineSessionAudit audit{};
  std::optional<exact::ExactLevel> last_closed_squared_level;
};

struct GroupWork {
  PipelineGroupAudit audit{};
  std::vector<ExactDirectMorseVerticalTargetProposal> proposals;
  bool closure_rejected{false};
  bool adapter_rejected{false};
};

[[nodiscard]] bool add_requirement(
    std::size_t value,
    std::size_t maximum,
    std::size_t& aggregate,
    PipelineResult& result) noexcept {
  if (!checked_add(aggregate, value, aggregate)) {
    fail(result, PipelineDecision::no_pipeline_capacity_overflow);
    return false;
  }
  if (aggregate > maximum) {
    fail(result, PipelineDecision::no_pipeline_budget_exhausted);
    return false;
  }
  return true;
}

[[nodiscard]] bool proposals_strictly_increasing(
    const std::vector<ExactDirectMorseVerticalTargetProposal>& proposals)
    noexcept {
  return std::adjacent_find(
             proposals.begin(),
             proposals.end(),
             [](const auto& left, const auto& right) {
               return left.representative_arm_root_binding_index >=
                      right.representative_arm_root_binding_index;
             }) == proposals.end();
}

[[nodiscard]] bool session_audit_within_budget(
    const PipelineSessionAudit& audit,
    const ExactDirectMorseForestCarrierCutReplaySessionBudget& budget)
    noexcept {
  const auto& counters = audit.final_cumulative_counters;
  const auto& carrier = budget.carrier_state_budget;
  return counters.forest_birth_record_scan_count <=
             carrier.maximum_forest_birth_record_scan_count &&
         counters.forest_node_scan_count <=
             carrier.maximum_forest_node_scan_count &&
         counters.forest_batch_preflight_scan_count <=
             carrier.maximum_forest_batch_scan_count &&
         counters.replayed_global_batch_count <=
             budget.maximum_replayed_global_batch_count &&
         counters.replayed_target_batch_count <=
             counters.replayed_global_batch_count &&
         counters.replayed_atomic_group_count <=
             carrier.maximum_forest_atomic_group_scan_count &&
         counters.replayed_saddle_count <=
             carrier.maximum_forest_saddle_scan_count &&
         counters.replayed_arm_binding_count <=
             carrier.maximum_forest_arm_binding_scan_count &&
         counters.replayed_child_reference_count <=
             carrier.maximum_forest_child_reference_scan_count &&
         counters.replayed_locator_union_count <=
             budget.maximum_replayed_locator_union_count &&
         counters.replayed_locator_binding_count <=
             budget.maximum_replayed_locator_binding_count &&
         counters.component_parent_hop_count <=
             carrier.maximum_parent_hop_count &&
         counters.exact_level_comparison_count <=
             carrier.maximum_exact_level_comparison_count &&
         counters.maximum_observed_exact_level_integer_bit_count <=
             carrier.maximum_single_exact_level_integer_bit_count &&
         counters.target_carrier_count <=
             carrier.maximum_index_entry_count &&
         counters.target_carrier_count <=
             carrier.maximum_logical_output_entry_count &&
         counters.synchronous_view_visit_count ==
             audit.processed_group_count &&
         counters.replayed_global_batch_count ==
             audit.final_committed_global_batch_prefix_count;
}

[[nodiscard]] bool group_audit_within_stage_budgets(
    const PipelineGroupAudit& audit,
    const PipelineBudget& budget) noexcept {
  const auto& plan = budget.facet_plan_budget;
  const auto& plan_counters = audit.facet_plan_counters;
  if (plan_counters.group_saddle_scan_count >
          plan.maximum_group_saddle_scan_count ||
      plan_counters.group_arm_binding_scan_count >
          plan.maximum_group_arm_binding_scan_count ||
      plan_counters.group_arm_binding_scan_count >
          plan.maximum_binding_sort_scratch_count ||
      plan_counters.generated_target_facet_reference_count !=
          audit.projected_target_facet_reference_count ||
      plan_counters.sort_comparison_count >
          plan.maximum_sort_comparison_count ||
      plan_counters.target_facet_lookup_comparison_count >
          plan.maximum_target_facet_lookup_comparison_count ||
      audit.representative_count >
          plan.maximum_representative_binding_count ||
      audit.projected_target_facet_reference_count >
          plan.maximum_projected_target_facet_reference_count ||
      audit.distinct_target_facet_count >
          plan.maximum_distinct_target_facet_count ||
      audit.retained_key_point_reference_count >
          plan.maximum_retained_key_point_reference_count ||
      audit.plan_logical_output_entry_count >
          plan.maximum_logical_output_entry_count) {
    return false;
  }

  const auto& closure = budget.closure_budget;
  if (audit.closure_audit.canonical_key_scan_count !=
          audit.distinct_target_facet_count ||
      audit.closure_audit.canonical_key_scan_count >
          closure.maximum_canonical_key_scan_count ||
      audit.closure_terminal_summary_count >
          closure.maximum_terminal_summary_count ||
      audit.closure_required_terminal_key_point_reference_count >
          closure.maximum_terminal_key_point_reference_count ||
      audit.closure_audit.carrier_cut_lookup_count >
          closure.maximum_carrier_cut_lookup_count ||
      audit.closure_required_logical_output_entry_count >
          closure.maximum_logical_output_entry_count ||
      audit.closure_counters.input_seed_reference_count !=
          audit.distinct_target_facet_count ||
      audit.closure_counters.processed_seed_reference_count !=
          audit.distinct_target_facet_count ||
      audit.closure_counters.distinct_seed_key_count !=
          audit.distinct_target_facet_count ||
      audit.closure_counters.duplicate_seed_key_reference_count != 0U ||
      audit.closure_audit.transient_node_count !=
          audit.closure_counters.interned_node_count ||
      audit.closure_audit.transient_edge_count !=
          audit.closure_counters.strict_edge_count ||
      audit.closure_audit.transient_seed_projection_count !=
          audit.distinct_target_facet_count ||
      audit.closure_audit.transient_node_count >
          closure.closure_budget.maximum_node_count ||
      audit.closure_audit.transient_seed_projection_count >
          closure.closure_budget.maximum_seed_count ||
      audit.closure_counters.evaluated_step_source_count >
          closure.closure_budget.maximum_step_call_count ||
      audit.closure_required_memo_slot_count >
          closure.closure_budget.maximum_memo_slot_count ||
      audit.closure_audit.terminal_summary_count !=
          audit.closure_terminal_summary_count ||
      audit.closure_audit.retained_terminal_key_point_reference_count !=
          audit.closure_required_terminal_key_point_reference_count ||
      audit.closure_audit.retained_logical_output_entry_count !=
          audit.closure_required_logical_output_entry_count ||
      !audit.closure_audit.transient_closure_graph_destroyed_before_return ||
      audit.closure_audit.closure_nodes_edges_or_projections_persisted ||
      audit.closure_audit.locator_state_mutated ||
      audit.closure_audit.locator_batch_committed ||
      audit.closure_audit.forest_to_cloud_namespace_identity_certified ||
      audit.closure_audit.facet_coface_cell_gamma_or_delaunay_materialized ||
      audit.closure_audit.public_status_claimed) {
    return false;
  }

  const auto& adapter = budget.proposal_budget;
  const auto& adapter_counters = audit.adapter_counters;
  return adapter_counters.source_saddle_revalidation_count <=
             adapter.maximum_source_saddle_revalidation_count &&
         adapter_counters.source_binding_revalidation_count <=
             adapter.maximum_source_binding_revalidation_count &&
         adapter_counters.source_key_lookup_comparison_count <=
             adapter.maximum_source_key_lookup_comparison_count &&
         adapter_counters.closure_summary_scan_count ==
             audit.distinct_target_facet_count &&
         adapter_counters.closure_summary_scan_count <=
             adapter.maximum_closure_summary_scan_count &&
         adapter_counters.projected_target_facet_aggregation_count ==
             audit.projected_target_facet_reference_count &&
         adapter_counters.positive_terminal_probe_count ==
             audit.adapter_required_carrier_entry_revalidation_count &&
         adapter_counters.positive_terminal_probe_count <=
             adapter.maximum_positive_terminal_probe_count &&
         adapter_counters.positive_terminal_probe_slot_visit_count <=
             adapter.maximum_positive_terminal_probe_slot_visit_count &&
         adapter_counters.positive_terminal_probe_parent_hop_count <=
             adapter.maximum_positive_terminal_probe_parent_hop_count &&
         adapter_counters.positive_terminal_probe_full_key_comparison_count >=
             adapter_counters.positive_terminal_probe_count &&
         adapter_counters.carrier_entry_revalidation_count ==
             audit.adapter_required_carrier_entry_revalidation_count &&
         adapter_counters.carrier_entry_revalidation_count <=
             adapter.maximum_carrier_entry_revalidation_count &&
         adapter_counters.target_node_lookup_count <=
             adapter.maximum_target_node_lookup_count &&
         adapter_counters.exact_level_comparison_count <=
             adapter.maximum_exact_level_comparison_count &&
         adapter_counters.maximum_observed_exact_level_integer_bit_count <=
             adapter.maximum_single_exact_level_integer_bit_count &&
         adapter_counters.unresolved_proposal_count ==
             audit.unresolved_proposal_count &&
         adapter_counters.resolved_proposal_count ==
             audit.resolved_proposal_count &&
         audit.proposal_count <= adapter.maximum_proposal_count &&
         audit.adapter_required_logical_output_entry_count ==
             audit.proposal_count &&
         audit.adapter_required_logical_output_entry_count <=
             adapter.maximum_logical_output_entry_count;
}

[[nodiscard]] bool success_decision(PipelineDecision decision) noexcept {
  return decision == PipelineDecision::complete_empty_adjacent_group_set ||
         decision ==
             PipelineDecision::
                 complete_all_resolved_multiorder_target_proposals ||
         decision ==
             PipelineDecision::
                 complete_with_unresolved_multiorder_target_proposals;
}

}  // namespace

ExactDirectMorseVerticalTargetProposalPipelineResult
build_exact_direct_morse_vertical_target_proposal_pipeline(
    const ExactDirectMorseForestJournalResult& source_forest,
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectMorseVerticalTargetProposalPipelineBudget& budget) {
  PipelineResult result;
  result.requested_budget = budget;
  result.point_count = source_forest.point_count;
  result.effective_maximum_order = source_forest.effective_maximum_order;
  result.external_target_authority_id =
      source_forest.config.locator_config.external_authority_id;

  if (!source_forest.certified_conditional_h0_candidate()) {
    fail(result, PipelineDecision::no_pipeline_source_forest_rejected);
    return result;
  }
  result.source_forest_certified = true;
  if (!index.validated_for(cloud) ||
      cloud.size() != source_forest.point_count ||
      source_forest.point_count == 0U ||
      result.external_target_authority_id == 0U) {
    fail(result, PipelineDecision::no_pipeline_point_namespace_rejected);
    return result;
  }
  // The LBVH/cloud identity is owned by the spatial layer.  The forest does
  // not retain a coordinate digest: preserving its canonical PointId
  // namespace is an explicit caller precondition, never a pipeline claim.
  result.lbvh_validated_for_cloud_and_point_count_matches = true;

  std::vector<GroupDescriptor> descriptors;
  std::vector<std::size_t> target_orders;
  try {
    for (std::size_t batch_index = 0U;
         batch_index < source_forest.batches.size();
         ++batch_index) {
      if (result.counters.source_batch_scan_count >=
          budget.maximum_source_batch_scan_count) {
        fail(result, PipelineDecision::no_pipeline_budget_exhausted);
        return result;
      }
      ++result.counters.source_batch_scan_count;
      const auto& batch = source_forest.batches[batch_index];
      if (batch.batch_index != batch_index || batch.order == 0U ||
          batch.order > source_forest.effective_maximum_order ||
          batch.atomic_group_offset > source_forest.atomic_groups.size() ||
          batch.atomic_group_count >
              source_forest.atomic_groups.size() -
                  batch.atomic_group_offset) {
        fail(result, PipelineDecision::no_pipeline_source_shape_rejected);
        return result;
      }
      if (batch.order < 2U) {
        continue;
      }
      for (std::size_t local_group = 0U;
           local_group < batch.atomic_group_count;
           ++local_group) {
        if (result.counters.source_atomic_group_scan_count >=
            budget.maximum_source_atomic_group_scan_count) {
          fail(result, PipelineDecision::no_pipeline_budget_exhausted);
          return result;
        }
        ++result.counters.source_atomic_group_scan_count;
        const std::size_t group_index =
            batch.atomic_group_offset + local_group;
        const auto& group = source_forest.atomic_groups[group_index];
        if (group.atomic_group_index != group_index ||
            group.batch_index != batch_index) {
          fail(result, PipelineDecision::no_pipeline_source_shape_rejected);
          return result;
        }
        if (descriptors.size() >= budget.maximum_group_audit_count ||
            result.counters.preflight_facet_plan_count >=
                budget.maximum_preflight_facet_plan_count) {
          fail(result, PipelineDecision::no_pipeline_budget_exhausted);
          return result;
        }
        std::uint64_t query_token = 0U;
        if (!checked_query_token(group_index, query_token)) {
          result.rejected_source_atomic_group_index = group_index;
          fail(result, PipelineDecision::no_pipeline_token_overflow);
          return result;
        }

        const std::size_t target_order = batch.order - 1U;
        bool target_order_seen = false;
        for (const std::size_t prior_target_order : target_orders) {
          if (result.counters.target_order_lookup_count >=
              budget.maximum_target_order_lookup_count) {
            fail(result, PipelineDecision::no_pipeline_budget_exhausted);
            return result;
          }
          ++result.counters.target_order_lookup_count;
          if (prior_target_order == target_order) {
            target_order_seen = true;
            break;
          }
        }
        if (!target_order_seen) {
          if (target_orders.size() >=
              budget.maximum_referenced_target_order_count) {
            fail(result, PipelineDecision::no_pipeline_budget_exhausted);
            return result;
          }
          target_orders.push_back(target_order);
        }

        ++result.counters.preflight_facet_plan_count;
        const auto plan =
            build_exact_direct_morse_vertical_target_facet_plan(
                source_forest,
                group_index,
                budget.facet_plan_budget);
        if (!plan.certified_group_local_target_facet_plan()) {
          result.rejected_source_atomic_group_index = group_index;
          result.rejected_facet_plan_decision = plan.decision;
          fail(result, map_plan_failure(plan.decision));
          return result;
        }
        if (!add_requirement(
                plan.required_representative_binding_count,
                budget.maximum_aggregate_representative_count,
                result.required_representative_count,
                result) ||
            !add_requirement(
                plan.required_projected_target_facet_reference_count,
                budget
                    .maximum_aggregate_projected_target_facet_reference_count,
                result.required_projected_target_facet_reference_count,
                result) ||
            !add_requirement(
                plan.required_distinct_target_facet_count,
                budget.maximum_aggregate_distinct_target_facet_count,
                result.required_distinct_target_facet_count,
                result) ||
            !add_requirement(
                plan.required_retained_key_point_reference_count,
                budget.maximum_aggregate_retained_key_point_reference_count,
                result.required_retained_key_point_reference_count,
                result) ||
            !add_requirement(
                plan.required_logical_output_entry_count,
                budget
                    .maximum_aggregate_plan_logical_output_entry_count,
                result.required_plan_logical_output_entry_count,
                result) ||
            !add_requirement(
                plan.required_distinct_target_facet_count,
                budget.maximum_aggregate_closure_terminal_summary_count,
                result.required_closure_terminal_summary_count,
                result) ||
            !add_requirement(
                plan.required_representative_binding_count,
                budget.maximum_aggregate_proposal_count,
                result.required_proposal_count,
                result)) {
          return result;
        }
        descriptors.push_back(
            {group_index, batch_index, target_order, 0U, query_token});
      }
    }

    result.required_group_count = descriptors.size();
    result.required_referenced_target_order_count = target_orders.size();
    result.counters.referenced_target_order_count = target_orders.size();
    if (target_orders.size() > budget.maximum_session_audit_count ||
        descriptors.size() > budget.maximum_executed_facet_plan_count ||
        descriptors.size() > budget.maximum_replay_advance_count ||
        descriptors.size() > budget.maximum_closure_build_count ||
        descriptors.size() > budget.maximum_proposal_adapter_count) {
      fail(result, PipelineDecision::no_pipeline_budget_exhausted);
      return result;
    }

    if (!checked_add(
            result.required_proposal_count,
            result.required_group_count,
            result.required_logical_output_entry_count) ||
        !checked_add(
            result.required_logical_output_entry_count,
            result.required_referenced_target_order_count,
            result.required_logical_output_entry_count)) {
      fail(result, PipelineDecision::no_pipeline_capacity_overflow);
      return result;
    }
    if (result.required_logical_output_entry_count >
        budget.maximum_logical_output_entry_count) {
      fail(result, PipelineDecision::no_pipeline_budget_exhausted);
      return result;
    }

    for (auto& descriptor : descriptors) {
      bool found = false;
      for (std::size_t session_index = 0U;
           session_index < target_orders.size();
           ++session_index) {
        if (result.counters.target_order_lookup_count >=
            budget.maximum_target_order_lookup_count) {
          fail(result, PipelineDecision::no_pipeline_budget_exhausted);
          return result;
        }
        ++result.counters.target_order_lookup_count;
        if (target_orders[session_index] == descriptor.target_order) {
          descriptor.session_index = session_index;
          found = true;
          break;
        }
      }
      if (!found) {
        fail(result, PipelineDecision::no_pipeline_source_shape_rejected);
        return result;
      }
    }
    result.aggregate_budget_preflight_certified = true;

    std::vector<WorkingSession> working_sessions;
    std::vector<ExactDirectMorseVerticalTargetProposal> final_proposals;
    std::vector<PipelineGroupAudit> final_group_audits;
    std::vector<PipelineSessionAudit> final_session_audits;
    working_sessions.reserve(target_orders.size());
    final_proposals.reserve(result.required_proposal_count);
    final_group_audits.reserve(result.required_group_count);
    final_session_audits.reserve(
        result.required_referenced_target_order_count);
    result.output_preallocated_after_aggregate_preflight = true;

    for (std::size_t session_index = 0U;
         session_index < target_orders.size();
         ++session_index) {
      const std::size_t target_order = target_orders[session_index];
      auto initialized =
          build_exact_direct_morse_forest_carrier_cut_replay_session(
              source_forest, target_order, budget.session_budget);
      if (!initialized.certified_ready_session() ||
          !initialized.session) {
        result.rejected_target_order = target_order;
        result.rejected_session_initialization_decision =
            initialized.decision;
        fail(
            result,
            PipelineDecision::no_pipeline_session_initialization_rejected);
        return result;
      }
      WorkingSession working;
      working.target_order = target_order;
      working.audit.session_audit_index = session_index;
      working.audit.target_order = target_order;
      working.audit.initialization_decision = initialized.decision;
      working.audit.ready_before_first_advance = initialized.session->ready();
      working.session = std::move(initialized.session);
      working_sessions.push_back(std::move(working));
      ++result.counters.initialized_session_count;
    }

    for (std::size_t pipeline_group_index = 0U;
         pipeline_group_index < descriptors.size();
         ++pipeline_group_index) {
      const auto descriptor = descriptors[pipeline_group_index];
      if (descriptor.session_index >= working_sessions.size() ||
          descriptor.source_batch_index >= source_forest.batches.size()) {
        fail(result, PipelineDecision::no_pipeline_source_shape_rejected);
        return result;
      }
      auto& working_session = working_sessions[descriptor.session_index];
      const auto& batch =
          source_forest.batches[descriptor.source_batch_index];
      if (working_session.target_order != descriptor.target_order ||
          batch.order != descriptor.target_order + 1U) {
        fail(result, PipelineDecision::no_pipeline_source_shape_rejected);
        return result;
      }

      GroupWork group_work;
      {
        ++result.counters.executed_facet_plan_count;
        const auto plan =
            build_exact_direct_morse_vertical_target_facet_plan(
                source_forest,
                descriptor.source_atomic_group_index,
                budget.facet_plan_budget);
        if (!plan.certified_group_local_target_facet_plan()) {
          result.rejected_source_atomic_group_index =
              descriptor.source_atomic_group_index;
          result.rejected_facet_plan_decision = plan.decision;
          fail(result, map_plan_failure(plan.decision));
          return result;
        }

        auto& audit = group_work.audit;
        audit.group_audit_index = pipeline_group_index;
        audit.source_atomic_group_index =
            descriptor.source_atomic_group_index;
        audit.source_batch_index = descriptor.source_batch_index;
        audit.source_order = batch.order;
        audit.target_order = descriptor.target_order;
        audit.source_batch_squared_level = batch.squared_level;
        audit.invocation_replay_token = descriptor.query_token;
        audit.source_binding_scan_count =
            plan.counters.group_arm_binding_scan_count;
        audit.representative_count =
            plan.required_representative_binding_count;
        audit.projected_target_facet_reference_count =
            plan.required_projected_target_facet_reference_count;
        audit.distinct_target_facet_count =
            plan.required_distinct_target_facet_count;
        audit.retained_key_point_reference_count =
            plan.required_retained_key_point_reference_count;
        audit.plan_logical_output_entry_count =
            plan.required_logical_output_entry_count;
        audit.facet_plan_counters = plan.counters;
        audit.facet_plan_decision = plan.decision;
        audit.exact_cut_equals_source_batch_level = true;

        ++result.counters.replay_advance_count;
        const auto advanced =
            working_session.session->advance_to_closed_cut(
                batch.squared_level,
                [&](const ExactDirectMorseForestCarrierCutReplayView& view) {
                  ++result.counters.closure_build_count;
                  const ExactDirectSparseFacetWitness query_witness{
                      result.external_target_authority_id,
                      descriptor.query_token};
                  const auto closure =
                      view.build_closure_summary_from_canonical_distinct_keys(
                          index,
                          cloud,
                          plan.canonical_distinct_target_facet_keys,
                          query_witness,
                          budget.closure_budget,
                          source_forest.config.closure_config,
                          source_forest.traversal_order);
                  audit.closure_decision = closure.decision;
                  audit.closure_terminal_summary_count =
                      closure.terminal_summaries.size();
                  audit.closure_required_terminal_key_point_reference_count =
                      closure.required_terminal_key_point_reference_count;
                  audit.closure_required_logical_output_entry_count =
                      closure.required_logical_output_entry_count;
                  audit.closure_required_memo_slot_count =
                      closure.required_memo_slot_count;
                  audit.closure_counters = closure.closure_counters;
                  audit.closure_audit = closure.audit;
                  if (!closure.certified_compact_closure_summary()) {
                    group_work.closure_rejected = true;
                    return;
                  }
                  for (const auto& summary : closure.terminal_summaries) {
                    using Disposition =
                        ExactDirectMorseForestCarrierCutClosureTerminalDisposition;
                    switch (summary.disposition) {
                      case Disposition::closure_unresolved:
                        ++audit.closure_unresolved_terminal_count;
                        break;
                      case Disposition::positive_active_latent:
                        ++audit.closure_active_latent_terminal_count;
                        break;
                      case Disposition::positive_resolved_reduced_root:
                        ++audit.closure_resolved_terminal_count;
                        break;
                      case Disposition::not_certified:
                        group_work.closure_rejected = true;
                        return;
                    }
                  }

                  ++result.counters.proposal_adapter_count;
                  auto proposals =
                      view.build_vertical_target_proposals_from_group_plan(
                          source_forest,
                          plan,
                          closure,
                          budget.proposal_budget);
                  audit.adapter_decision = proposals.decision;
                  audit.unresolved_proposal_count =
                      proposals.counters.unresolved_proposal_count;
                  audit.resolved_proposal_count =
                      proposals.counters.resolved_proposal_count;
                  audit.adapter_required_carrier_entry_revalidation_count =
                      proposals.required_carrier_entry_revalidation_count;
                  audit.adapter_required_logical_output_entry_count =
                      proposals.required_logical_output_entry_count;
                  audit.adapter_counters = proposals.counters;
                  if (!proposals
                           .certified_group_local_vertical_target_proposals()) {
                    group_work.adapter_rejected = true;
                    return;
                  }
                  group_work.proposals = std::move(proposals.proposals);
                });
        audit.advance_decision = advanced.decision;
        audit.committed_global_batch_prefix_count =
            advanced.committed_global_batch_prefix_count;

        if (!advanced.certified_forest_relative_closed_cut()) {
          result.rejected_source_atomic_group_index =
              descriptor.source_atomic_group_index;
          result.rejected_advance_decision = advanced.decision;
          fail(result, PipelineDecision::no_pipeline_replay_advance_rejected);
          return result;
        }
        if (group_work.closure_rejected) {
          result.rejected_source_atomic_group_index =
              descriptor.source_atomic_group_index;
          result.rejected_closure_decision = audit.closure_decision;
          fail(result, PipelineDecision::no_pipeline_closure_rejected);
          return result;
        }
        if (group_work.adapter_rejected) {
          result.rejected_source_atomic_group_index =
              descriptor.source_atomic_group_index;
          result.rejected_adapter_decision = audit.adapter_decision;
          fail(
              result,
              PipelineDecision::no_pipeline_proposal_adapter_rejected);
          return result;
        }
        if (group_work.proposals.size() != audit.representative_count ||
            audit.closure_terminal_summary_count !=
                audit.distinct_target_facet_count) {
          fail(result, PipelineDecision::no_pipeline_source_shape_rejected);
          return result;
        }
        audit.advance_post_stamp_certified_before_publication = true;
        working_session.audit.final_cumulative_counters =
            advanced.cumulative_counters;
      }

      group_work.audit.plan_closure_and_locator_payload_destroyed = true;
      group_work.audit.proposal_offset = final_proposals.size();
      group_work.audit.proposal_count = group_work.proposals.size();
      if (working_session.last_closed_squared_level.has_value() &&
          *working_session.last_closed_squared_level ==
              group_work.audit.source_batch_squared_level) {
        ++result.counters.equal_level_same_target_order_group_count;
      }
      working_session.last_closed_squared_level =
          group_work.audit.source_batch_squared_level;
      ++working_session.audit.processed_group_count;
      working_session.audit.final_committed_global_batch_prefix_count =
          group_work.audit.committed_global_batch_prefix_count;

      final_proposals.insert(
          final_proposals.end(),
          std::make_move_iterator(group_work.proposals.begin()),
          std::make_move_iterator(group_work.proposals.end()));
      final_group_audits.push_back(std::move(group_work.audit));
    }

    if (final_proposals.size() != result.required_proposal_count ||
        final_group_audits.size() != result.required_group_count ||
        !proposals_strictly_increasing(final_proposals)) {
      fail(result, PipelineDecision::no_pipeline_source_shape_rejected);
      return result;
    }

    for (auto& working_session : working_sessions) {
      working_session.audit.remained_unpoisoned =
          working_session.session && !working_session.session->poisoned();
      if (!working_session.audit.ready_before_first_advance ||
          !working_session.audit.remained_unpoisoned) {
        result.rejected_target_order = working_session.target_order;
        fail(result, PipelineDecision::no_pipeline_replay_advance_rejected);
        return result;
      }
      final_session_audits.push_back(working_session.audit);
    }

    for (const auto& audit : final_group_audits) {
      if (!checked_add(
              result.counters.representative_count,
              audit.representative_count,
              result.counters.representative_count) ||
          !checked_add(
              result.counters.projected_target_facet_reference_count,
              audit.projected_target_facet_reference_count,
              result.counters.projected_target_facet_reference_count) ||
          !checked_add(
              result.counters.distinct_target_facet_count,
              audit.distinct_target_facet_count,
              result.counters.distinct_target_facet_count) ||
          !checked_add(
              result.counters.closure_terminal_summary_count,
              audit.closure_terminal_summary_count,
              result.counters.closure_terminal_summary_count) ||
          !checked_add(
              result.counters.closure_unresolved_terminal_count,
              audit.closure_unresolved_terminal_count,
              result.counters.closure_unresolved_terminal_count) ||
          !checked_add(
              result.counters.closure_active_latent_terminal_count,
              audit.closure_active_latent_terminal_count,
              result.counters.closure_active_latent_terminal_count) ||
          !checked_add(
              result.counters.closure_resolved_terminal_count,
              audit.closure_resolved_terminal_count,
              result.counters.closure_resolved_terminal_count) ||
          !checked_add(
              result.counters.unresolved_proposal_count,
              audit.unresolved_proposal_count,
              result.counters.unresolved_proposal_count) ||
          !checked_add(
              result.counters.resolved_proposal_count,
              audit.resolved_proposal_count,
              result.counters.resolved_proposal_count)) {
        fail(result, PipelineDecision::no_pipeline_capacity_overflow);
        return result;
      }
    }
    result.counters.retained_key_point_reference_count =
        result.required_retained_key_point_reference_count;
    result.counters.plan_logical_output_entry_count =
        result.required_plan_logical_output_entry_count;

    if (result.counters.representative_count !=
            result.required_representative_count ||
        result.counters.projected_target_facet_reference_count !=
            result.required_projected_target_facet_reference_count ||
        result.counters.distinct_target_facet_count !=
            result.required_distinct_target_facet_count ||
        result.counters.closure_terminal_summary_count !=
            result.required_closure_terminal_summary_count ||
        result.counters.unresolved_proposal_count +
                result.counters.resolved_proposal_count !=
            result.required_proposal_count) {
      fail(result, PipelineDecision::no_pipeline_source_shape_rejected);
      return result;
    }

    result.counters.referenced_target_order_count = target_orders.size();
    result.counters.initialized_session_count = working_sessions.size();
    working_sessions.clear();
    descriptors.clear();
    target_orders.clear();

    result.proposals = std::move(final_proposals);
    result.group_audits = std::move(final_group_audits);
    result.session_audits = std::move(final_session_audits);
    result.one_session_per_referenced_target_order = true;
    result.source_groups_processed_in_batch_then_group_order = true;
    result.each_session_advanced_monotonically = true;
    result.query_tokens_follow_three_times_group_index_plus_one = true;
    result.query_tokens_disjoint_from_birth_and_union_domains = true;
    result.every_group_plan_certified = true;
    result.every_closure_summary_certified = true;
    result.every_group_adapter_certified = true;
    result.every_advance_post_stamp_certified_before_publication = true;
    result.proposals_sorted_unique_by_binding_index = true;
    result.source_binding_partition_proved_global_output_order = true;
    result.only_final_proposals_and_scalar_audits_retained = true;
    result.no_plan_closure_locator_or_session_reference_retained = true;
    result.scope = PipelineScope::
        all_source_orders_k_at_least_two_atomic_group_strict_arms_to_compact_adjacent_k_minus_one_target_proposals_only;
    if (result.required_group_count == 0U) {
      result.decision =
          PipelineDecision::complete_empty_adjacent_group_set;
    } else if (result.counters.unresolved_proposal_count == 0U) {
      result.decision = PipelineDecision::
          complete_all_resolved_multiorder_target_proposals;
    } else {
      result.decision = PipelineDecision::
          complete_with_unresolved_multiorder_target_proposals;
    }
    return result;
  } catch (const std::bad_alloc&) {
    fail(result, PipelineDecision::no_pipeline_allocation_failed);
  } catch (const std::length_error&) {
    fail(result, PipelineDecision::no_pipeline_capacity_overflow);
  } catch (const std::exception&) {
    fail(result, PipelineDecision::no_pipeline_source_shape_rejected);
  }
  return result;
}

bool ExactDirectMorseVerticalTargetProposalPipelineResult::
    certified_multiorder_target_proposals() const noexcept {
  if (schema_version !=
          direct_morse_vertical_target_proposal_pipeline_schema_version ||
      !success_decision(decision) || point_count == 0U ||
      effective_maximum_order == 0U || external_target_authority_id == 0U ||
      required_referenced_target_order_count != session_audits.size() ||
      required_group_count != group_audits.size() ||
      required_representative_count != proposals.size() ||
      required_proposal_count != proposals.size() ||
      required_closure_terminal_summary_count !=
          required_distinct_target_facet_count ||
      counters.source_batch_scan_count >
          requested_budget.maximum_source_batch_scan_count ||
      counters.source_atomic_group_scan_count >
          requested_budget.maximum_source_atomic_group_scan_count ||
      counters.target_order_lookup_count >
          requested_budget.maximum_target_order_lookup_count ||
      counters.referenced_target_order_count != session_audits.size() ||
      counters.initialized_session_count != session_audits.size() ||
      counters.preflight_facet_plan_count != group_audits.size() ||
      counters.preflight_facet_plan_count >
          requested_budget.maximum_preflight_facet_plan_count ||
      counters.executed_facet_plan_count != group_audits.size() ||
      counters.executed_facet_plan_count >
          requested_budget.maximum_executed_facet_plan_count ||
      counters.replay_advance_count != group_audits.size() ||
      counters.replay_advance_count >
          requested_budget.maximum_replay_advance_count ||
      counters.closure_build_count != group_audits.size() ||
      counters.closure_build_count >
          requested_budget.maximum_closure_build_count ||
      counters.proposal_adapter_count != group_audits.size() ||
      counters.proposal_adapter_count >
          requested_budget.maximum_proposal_adapter_count ||
      counters.representative_count != proposals.size() ||
      counters.representative_count != required_representative_count ||
      counters.projected_target_facet_reference_count !=
          required_projected_target_facet_reference_count ||
      counters.distinct_target_facet_count !=
          required_distinct_target_facet_count ||
      counters.retained_key_point_reference_count !=
          required_retained_key_point_reference_count ||
      counters.plan_logical_output_entry_count !=
          required_plan_logical_output_entry_count ||
      counters.closure_terminal_summary_count !=
          required_closure_terminal_summary_count ||
      counters.unresolved_proposal_count +
              counters.resolved_proposal_count !=
          proposals.size() ||
      required_referenced_target_order_count >
          requested_budget.maximum_referenced_target_order_count ||
      required_referenced_target_order_count >
          requested_budget.maximum_session_audit_count ||
      required_group_count > requested_budget.maximum_group_audit_count ||
      required_representative_count >
          requested_budget.maximum_aggregate_representative_count ||
      required_projected_target_facet_reference_count >
          requested_budget
              .maximum_aggregate_projected_target_facet_reference_count ||
      required_distinct_target_facet_count >
          requested_budget.maximum_aggregate_distinct_target_facet_count ||
      required_retained_key_point_reference_count >
          requested_budget
              .maximum_aggregate_retained_key_point_reference_count ||
      required_plan_logical_output_entry_count >
          requested_budget
              .maximum_aggregate_plan_logical_output_entry_count ||
      required_closure_terminal_summary_count >
          requested_budget
              .maximum_aggregate_closure_terminal_summary_count ||
      required_proposal_count >
          requested_budget.maximum_aggregate_proposal_count ||
      required_logical_output_entry_count >
          requested_budget.maximum_logical_output_entry_count ||
      !source_forest_certified ||
      !lbvh_validated_for_cloud_and_point_count_matches ||
      !matching_canonical_point_namespace_required ||
      forest_to_cloud_namespace_identity_certified ||
      !aggregate_budget_preflight_certified ||
      !one_session_per_referenced_target_order ||
      !source_groups_processed_in_batch_then_group_order ||
      !each_session_advanced_monotonically ||
      !query_tokens_follow_three_times_group_index_plus_one ||
      !query_tokens_disjoint_from_birth_and_union_domains ||
      !every_group_plan_certified ||
      !every_closure_summary_certified ||
      !every_group_adapter_certified ||
      !every_advance_post_stamp_certified_before_publication ||
      !output_preallocated_after_aggregate_preflight ||
      !proposals_sorted_unique_by_binding_index ||
      !source_binding_partition_proved_global_output_order ||
      !only_final_proposals_and_scalar_audits_retained ||
      !no_plan_closure_locator_or_session_reference_retained ||
      no_partial_scientific_payload_published_on_failure ||
      scope != PipelineScope::
                   all_source_orders_k_at_least_two_atomic_group_strict_arms_to_compact_adjacent_k_minus_one_target_proposals_only ||
      !pipeline_non_scope_honest(*this) ||
      !proposals_strictly_increasing(proposals)) {
    return false;
  }

  std::size_t logical = 0U;
  if (!checked_add(proposals.size(), group_audits.size(), logical) ||
      !checked_add(logical, session_audits.size(), logical) ||
      logical != required_logical_output_entry_count) {
    return false;
  }
  const bool empty = group_audits.empty();
  if (empty !=
          (decision == PipelineDecision::complete_empty_adjacent_group_set) ||
      (empty && (!proposals.empty() || !session_audits.empty())) ||
      (!empty && decision ==
                     PipelineDecision::
                         complete_all_resolved_multiorder_target_proposals &&
       counters.unresolved_proposal_count != 0U) ||
      (!empty && decision ==
                     PipelineDecision::
                         complete_with_unresolved_multiorder_target_proposals &&
       counters.unresolved_proposal_count == 0U)) {
    return false;
  }

  std::size_t proposal_cursor = 0U;
  std::size_t representative_total = 0U;
  std::size_t projected_total = 0U;
  std::size_t distinct_total = 0U;
  std::size_t retained_key_reference_total = 0U;
  std::size_t plan_logical_output_total = 0U;
  std::size_t closure_total = 0U;
  std::size_t unresolved_terminal_total = 0U;
  std::size_t latent_terminal_total = 0U;
  std::size_t resolved_terminal_total = 0U;
  std::size_t unresolved_proposal_total = 0U;
  std::size_t resolved_proposal_total = 0U;
  std::optional<std::size_t> previous_group_index;
  for (std::size_t audit_index = 0U;
       audit_index < group_audits.size();
       ++audit_index) {
    const auto& audit = group_audits[audit_index];
    std::uint64_t expected_token = 0U;
    std::size_t expected_projected = 0U;
    std::size_t expected_source_key_references = 0U;
    std::size_t expected_target_key_references = 0U;
    std::size_t expected_retained_key_references = 0U;
    std::size_t expected_plan_logical_output = 0U;
    if (audit.group_audit_index != audit_index ||
        (previous_group_index.has_value() &&
         *previous_group_index >= audit.source_atomic_group_index) ||
        audit.source_order < 2U ||
        audit.target_order + 1U != audit.source_order ||
        audit.representative_count == 0U ||
        audit.source_binding_scan_count < audit.representative_count ||
        !checked_multiply(
            audit.representative_count,
            audit.source_order,
            expected_projected) ||
        expected_projected !=
            audit.projected_target_facet_reference_count ||
        audit.distinct_target_facet_count == 0U ||
        audit.distinct_target_facet_count >
            audit.projected_target_facet_reference_count ||
        !checked_multiply(
            audit.representative_count,
            audit.source_order,
            expected_source_key_references) ||
        !checked_multiply(
            audit.distinct_target_facet_count,
            audit.target_order,
            expected_target_key_references) ||
        !checked_add(
            expected_source_key_references,
            expected_target_key_references,
            expected_retained_key_references) ||
        expected_retained_key_references !=
            audit.retained_key_point_reference_count ||
        !checked_add(
            audit.representative_count,
            audit.projected_target_facet_reference_count,
            expected_plan_logical_output) ||
        !checked_add(
            expected_plan_logical_output,
            audit.distinct_target_facet_count,
            expected_plan_logical_output) ||
        !checked_add(
            expected_plan_logical_output,
            audit.representative_count,
            expected_plan_logical_output) ||
        !checked_add(
            expected_plan_logical_output,
            expected_retained_key_references,
            expected_plan_logical_output) ||
        expected_plan_logical_output !=
            audit.plan_logical_output_entry_count ||
        audit.closure_terminal_summary_count !=
            audit.distinct_target_facet_count ||
        audit.closure_unresolved_terminal_count +
                audit.closure_active_latent_terminal_count +
                audit.closure_resolved_terminal_count !=
            audit.closure_terminal_summary_count ||
        audit.proposal_count != audit.representative_count ||
        audit.unresolved_proposal_count +
                audit.resolved_proposal_count !=
            audit.proposal_count ||
        !group_audit_within_stage_budgets(audit, requested_budget) ||
        audit.proposal_offset != proposal_cursor ||
        audit.proposal_count > proposals.size() - proposal_cursor ||
        !checked_query_token(
            audit.source_atomic_group_index, expected_token) ||
        audit.invocation_replay_token != expected_token ||
        audit.facet_plan_decision !=
            ExactDirectMorseVerticalTargetFacetPlanDecision::
                complete_group_local_canonical_target_facet_plan ||
        audit.advance_decision !=
            ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
                complete_certified_forest_relative_closed_cut ||
        !closure_decision_complete(audit.closure_decision) ||
        !adapter_decision_complete(audit.adapter_decision) ||
        !audit.exact_cut_equals_source_batch_level ||
        !audit.advance_post_stamp_certified_before_publication ||
        !audit.plan_closure_and_locator_payload_destroyed) {
      return false;
    }
    for (std::size_t local = 0U; local < audit.proposal_count; ++local) {
      const auto& proposal = proposals[proposal_cursor + local];
      if (proposal.replay_token != audit.invocation_replay_token ||
          (proposal.disposition ==
               ExactDirectMorseVerticalProposalDisposition::unresolved &&
           proposal.target_seed_node_id.has_value()) ||
          (proposal.disposition ==
               ExactDirectMorseVerticalProposalDisposition::
                   resolved_target_seed &&
           !proposal.target_seed_node_id.has_value())) {
        return false;
      }
    }
    previous_group_index = audit.source_atomic_group_index;
    proposal_cursor += audit.proposal_count;
    if (!checked_add(
            representative_total,
            audit.representative_count,
            representative_total) ||
        !checked_add(
            projected_total,
            audit.projected_target_facet_reference_count,
            projected_total) ||
        !checked_add(
            distinct_total,
            audit.distinct_target_facet_count,
            distinct_total) ||
        !checked_add(
            retained_key_reference_total,
            audit.retained_key_point_reference_count,
            retained_key_reference_total) ||
        !checked_add(
            plan_logical_output_total,
            audit.plan_logical_output_entry_count,
            plan_logical_output_total) ||
        !checked_add(
            closure_total,
            audit.closure_terminal_summary_count,
            closure_total) ||
        !checked_add(
            unresolved_terminal_total,
            audit.closure_unresolved_terminal_count,
            unresolved_terminal_total) ||
        !checked_add(
            latent_terminal_total,
            audit.closure_active_latent_terminal_count,
            latent_terminal_total) ||
        !checked_add(
            resolved_terminal_total,
            audit.closure_resolved_terminal_count,
            resolved_terminal_total) ||
        !checked_add(
            unresolved_proposal_total,
            audit.unresolved_proposal_count,
            unresolved_proposal_total) ||
        !checked_add(
            resolved_proposal_total,
            audit.resolved_proposal_count,
            resolved_proposal_total)) {
      return false;
    }
  }
  if (proposal_cursor != proposals.size() ||
      representative_total != counters.representative_count ||
      projected_total != counters.projected_target_facet_reference_count ||
      distinct_total != counters.distinct_target_facet_count ||
      retained_key_reference_total !=
          counters.retained_key_point_reference_count ||
      plan_logical_output_total !=
          counters.plan_logical_output_entry_count ||
      closure_total != counters.closure_terminal_summary_count ||
      unresolved_terminal_total !=
          counters.closure_unresolved_terminal_count ||
      latent_terminal_total !=
          counters.closure_active_latent_terminal_count ||
      resolved_terminal_total !=
          counters.closure_resolved_terminal_count ||
      unresolved_proposal_total != counters.unresolved_proposal_count ||
      resolved_proposal_total != counters.resolved_proposal_count) {
    return false;
  }

  std::size_t session_group_total = 0U;
  std::size_t observed_equal_level_same_target_order_group_count = 0U;
  for (std::size_t audit_index = 0U;
       audit_index < session_audits.size();
       ++audit_index) {
    const auto& audit = session_audits[audit_index];
    bool duplicate_target_order = false;
    for (std::size_t prior_index = 0U;
         prior_index < audit_index;
         ++prior_index) {
      duplicate_target_order =
          duplicate_target_order ||
          session_audits[prior_index].target_order == audit.target_order;
    }
    if (audit.session_audit_index != audit_index || audit.target_order == 0U ||
        duplicate_target_order ||
        !initialization_decision_complete(audit.initialization_decision) ||
        !audit.ready_before_first_advance || !audit.remained_unpoisoned ||
        audit.processed_group_count == 0U ||
        !session_audit_within_budget(
            audit, requested_budget.session_budget)) {
      return false;
    }
    std::size_t observed_group_count = 0U;
    std::size_t maximum_prefix = 0U;
    std::optional<exact::ExactLevel> previous_level;
    std::optional<std::size_t> previous_prefix;
    std::optional<std::size_t> previous_source_batch_index;
    for (const auto& group_audit : group_audits) {
      if (group_audit.target_order == audit.target_order) {
        if ((previous_level.has_value() &&
             group_audit.source_batch_squared_level < *previous_level) ||
            (previous_prefix.has_value() &&
             group_audit.committed_global_batch_prefix_count <
                 *previous_prefix) ||
            (previous_source_batch_index.has_value() &&
             group_audit.source_batch_index <
                 *previous_source_batch_index)) {
          return false;
        }
        if (previous_level.has_value() &&
            group_audit.source_batch_squared_level == *previous_level) {
          ++observed_equal_level_same_target_order_group_count;
        }
        previous_level = group_audit.source_batch_squared_level;
        previous_prefix =
            group_audit.committed_global_batch_prefix_count;
        previous_source_batch_index = group_audit.source_batch_index;
        ++observed_group_count;
        maximum_prefix = std::max(
            maximum_prefix,
            group_audit.committed_global_batch_prefix_count);
      }
    }
    if (observed_group_count != audit.processed_group_count ||
        maximum_prefix !=
            audit.final_committed_global_batch_prefix_count ||
        !checked_add(
            session_group_total,
            observed_group_count,
            session_group_total)) {
      return false;
    }
  }
  return session_group_total == group_audits.size() &&
         observed_equal_level_same_target_order_group_count ==
             counters.equal_level_same_target_order_group_count;
}

bool ExactDirectMorseVerticalTargetProposalPipelineResult::
    certified_atomic_failure() const noexcept {
  return schema_version ==
             direct_morse_vertical_target_proposal_pipeline_schema_version &&
         decision != PipelineDecision::not_certified &&
         !success_decision(decision) && proposals.empty() &&
         session_audits.empty() && group_audits.empty() &&
         no_partial_scientific_payload_published_on_failure &&
         scope == PipelineScope::unspecified &&
         pipeline_non_scope_honest(*this);
}

bool ExactDirectMorseVerticalTargetProposalPipelineResult::
    certified_outcome() const noexcept {
  return certified_multiorder_target_proposals() ||
         certified_atomic_failure();
}

}  // namespace morsehgp3d::hierarchy
