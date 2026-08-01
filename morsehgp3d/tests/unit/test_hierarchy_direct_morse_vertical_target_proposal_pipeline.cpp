#include "morsehgp3d/hierarchy/direct_morse_vertical_target_proposal_pipeline.hpp"
#include "morsehgp3d/hierarchy/direct_morse_vertical_journal.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

constexpr std::uint64_t authority_id = UINT64_C(0x51A7E1E55);
int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] ExactLevel level(std::int64_t numerator) {
  return ExactLevel{BigInt{numerator}, BigInt{1}};
}

[[nodiscard]] ExactDirectSparseFacetKey key(
    std::initializer_list<PointId> points) {
  ExactDirectSparseFacetKey result;
  result.point_count = points.size();
  std::size_t index = 0U;
  for (const PointId point : points) {
    result.point_ids[index] = point;
    ++index;
  }
  return result;
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocatorBudget locator_budget() {
  ExactDirectSparsePositiveFacetLocatorBudget budget;
  budget.maximum_component_handle_count = 64U;
  budget.maximum_committed_binding_count = 64U;
  budget.maximum_committed_key_point_count = 256U;
  budget.maximum_committed_union_count = 64U;
  budget.maximum_committed_batch_count = 64U;
  budget.maximum_batch_query_count = 64U;
  budget.maximum_batch_union_count = 64U;
  budget.maximum_batch_binding_count = 64U;
  budget.maximum_batch_key_point_count = 256U;
  budget.maximum_table_slot_count = 256U;
  budget.maximum_batch_scratch_slot_count = 256U;
  return budget;
}

[[nodiscard]] ExactDirectMorseForestBudget forest_budget() {
  ExactDirectMorseForestBudget budget;
  budget.maximum_source_role_scan_count = 512U;
  budget.maximum_source_batch_scan_count = 512U;
  budget.maximum_source_family_scan_count = 512U;
  budget.maximum_source_arm_seed_scan_count = 512U;
  budget.maximum_birth_record_count = 512U;
  budget.maximum_arm_root_binding_count = 512U;
  budget.maximum_saddle_record_count = 512U;
  budget.maximum_atomic_group_count = 512U;
  budget.maximum_child_reference_count = 512U;
  budget.maximum_batch_record_count = 512U;
  budget.maximum_node_count = 512U;
  budget.maximum_final_root_count = 512U;
  budget.maximum_batch_distinct_arm_count = 512U;
  budget.maximum_logical_output_entry_count = 4096U;
  budget.maximum_aggregate_closure_node_count = 4096U;
  budget.maximum_aggregate_closure_step_call_count = 4096U;
  budget.locator_budget = locator_budget();
  return budget;
}

[[nodiscard]] ExactDirectMorseForestCarrierCutIndexBudget index_budget() {
  ExactDirectMorseForestCarrierCutIndexBudget budget;
  budget.maximum_forest_birth_record_scan_count = 512U;
  budget.maximum_forest_node_scan_count = 512U;
  budget.maximum_forest_batch_scan_count = 512U;
  budget.maximum_forest_atomic_group_scan_count = 512U;
  budget.maximum_forest_saddle_scan_count = 512U;
  budget.maximum_forest_arm_binding_scan_count = 512U;
  budget.maximum_forest_child_reference_scan_count = 512U;
  budget.maximum_forest_final_root_scan_count = 512U;
  budget.maximum_component_state_count = 512U;
  budget.maximum_node_marker_state_count = 512U;
  budget.maximum_index_entry_count = 512U;
  budget.maximum_group_carrier_scratch_count = 512U;
  budget.maximum_group_prior_root_scratch_count = 512U;
  budget.maximum_parent_hop_count = 32768U;
  budget.maximum_exact_level_comparison_count = 32768U;
  budget.maximum_single_exact_level_integer_bit_count = 64U;
  budget.maximum_logical_output_entry_count = 4096U;
  return budget;
}

[[nodiscard]] ExactDirectMorseForestCarrierCutReplaySessionBudget
session_budget() {
  ExactDirectMorseForestCarrierCutReplaySessionBudget budget;
  budget.carrier_state_budget = index_budget();
  budget.locator_budget = locator_budget();
  budget.maximum_replayed_global_batch_count = 512U;
  budget.maximum_replayed_locator_union_count = 512U;
  budget.maximum_replayed_locator_binding_count = 512U;
  budget.maximum_batch_group_plan_count = 128U;
  budget.maximum_batch_group_carrier_reference_count = 512U;
  budget.maximum_batch_group_prior_root_reference_count = 512U;
  budget.maximum_batch_locator_union_scratch_count = 512U;
  budget.maximum_batch_locator_binding_scratch_count = 512U;
  return budget;
}

[[nodiscard]] ExactDirectSparseFacetDescentClosureBudget
descent_closure_budget() {
  ExactDirectSparseFacetDescentStepBudget step;
  step.source_locator_probe = {512U, 64U};
  step.top_k_query = ExactLbvhTopKBudget{
      4096U, 4096U, 4096U, 4096U, 256U, 16U, 16U};
  step.successor_locator_probe = {512U, 64U};
  return {128U, 128U, 128U, 257U, step};
}

[[nodiscard]] ExactDirectMorseForestCarrierCutClosureAdapterBudget
closure_adapter_budget() {
  return {128U, 128U, 512U, 128U, 1024U, descent_closure_budget()};
}

[[nodiscard]] ExactDirectMorseVerticalTargetFacetPlanBudget plan_budget() {
  return {
      128U,
      128U,
      128U,
      128U,
      512U,
      256U,
      32768U,
      32768U,
      2048U,
      4096U};
}

[[nodiscard]] ExactDirectMorseVerticalTargetProposalAdapterBudget
proposal_adapter_budget() {
  ExactDirectMorseVerticalTargetProposalAdapterBudget budget;
  budget.maximum_source_saddle_revalidation_count = 128U;
  budget.maximum_source_binding_revalidation_count = 128U;
  budget.maximum_source_key_lookup_comparison_count = 32768U;
  budget.maximum_closure_summary_scan_count = 256U;
  budget.maximum_positive_terminal_probe_count = 512U;
  budget.maximum_positive_terminal_probe_slot_visit_count = 32768U;
  budget.maximum_positive_terminal_probe_parent_hop_count = 32768U;
  budget.positive_terminal_probe_budget = {512U, 64U};
  budget.maximum_carrier_entry_revalidation_count = 512U;
  budget.maximum_target_node_lookup_count = 512U;
  budget.maximum_exact_level_comparison_count = 4096U;
  budget.maximum_single_exact_level_integer_bit_count = 64U;
  budget.maximum_proposal_count = 128U;
  budget.maximum_logical_output_entry_count = 128U;
  return budget;
}

[[nodiscard]] ExactDirectMorseVerticalTargetProposalPipelineBudget
pipeline_budget() {
  ExactDirectMorseVerticalTargetProposalPipelineBudget budget;
  budget.session_budget = session_budget();
  budget.facet_plan_budget = plan_budget();
  budget.closure_budget = closure_adapter_budget();
  budget.proposal_budget = proposal_adapter_budget();
  budget.maximum_source_batch_scan_count = 128U;
  budget.maximum_source_atomic_group_scan_count = 128U;
  budget.maximum_referenced_target_order_count = 16U;
  budget.maximum_target_order_lookup_count = 1024U;
  budget.maximum_preflight_facet_plan_count = 128U;
  budget.maximum_executed_facet_plan_count = 128U;
  budget.maximum_replay_advance_count = 128U;
  budget.maximum_closure_build_count = 128U;
  budget.maximum_proposal_adapter_count = 128U;
  budget.maximum_aggregate_representative_count = 512U;
  budget.maximum_aggregate_projected_target_facet_reference_count = 2048U;
  budget.maximum_aggregate_distinct_target_facet_count = 2048U;
  budget.maximum_aggregate_retained_key_point_reference_count = 4096U;
  budget.maximum_aggregate_plan_logical_output_entry_count = 8192U;
  budget.maximum_aggregate_closure_terminal_summary_count = 2048U;
  budget.maximum_aggregate_proposal_count = 512U;
  budget.maximum_session_audit_count = 16U;
  budget.maximum_group_audit_count = 128U;
  budget.maximum_logical_output_entry_count = 1024U;
  return budget;
}

[[nodiscard]] ExactDirectMorseVerticalBudget vertical_budget() {
  ExactDirectMorseVerticalBudget budget;
  budget.maximum_forest_node_scan_count = 512U;
  budget.maximum_child_reference_scan_count = 512U;
  budget.maximum_birth_record_scan_count = 512U;
  budget.maximum_batch_scan_count = 512U;
  budget.maximum_atomic_group_scan_count = 512U;
  budget.maximum_saddle_scan_count = 512U;
  budget.maximum_arm_binding_scan_count = 512U;
  budget.maximum_proposal_count = 512U;
  budget.maximum_label_resolution_count = 512U;
  budget.maximum_group_check_count = 512U;
  budget.maximum_checkpoint_count = 512U;
  budget.maximum_adjacent_family_count = 16U;
  budget.maximum_group_sort_scratch_count = 512U;
  budget.maximum_group_sort_comparison_count = 32768U;
  budget.maximum_target_parent_hop_count = 32768U;
  budget.maximum_exact_level_comparison_count = 32768U;
  budget.maximum_single_exact_level_integer_bit_count = 64U;
  budget.maximum_logical_output_entry_count = 4096U;
  return budget;
}

void set_success_flags(ExactDirectMorseForestJournalResult& forest) {
  forest.budget_preflight_certified = true;
  forest.source_event_journal_freshly_replayed = true;
  forest.source_strict_arm_journal_freshly_replayed = true;
  forest.every_birth_key_reconstructed_from_closed_direct_event = true;
  forest.deterministic_disjoint_birth_union_and_query_tokens = true;
  forest.batches_processed_in_strict_order_level_order = true;
  forest.cardinality_isolates_orders_in_shared_locator = true;
  forest.current_level_births_hidden_from_arm_descent = true;
  forest.higher_order_direct_births_are_latent_carriers = true;
  forest.one_10_5c_call_per_nonempty_strict_arm_batch = true;
  forest.every_strict_arm_has_positive_terminal = true;
  forest.all_catalogued_saddle_families_consumed_once = true;
  forest.carrier_to_optional_reduced_root_authority_maintained = true;
  forest.every_saddle_has_positive_carrier = true;
  forest.typed_root_or_latent_carrier_hyperedges_closed_transitively = true;
  forest.q_r_counts_only_distinct_prior_reduced_roots = true;
  forest.all_equal_level_saddles_quotiented_before_mutation = true;
  forest.saddle_records_grouped_with_source_family_provenance = true;
  forest.q_zero_groups_create_one_reduced_birth = true;
  forest.q_one_continuations_create_no_node = true;
  forest.q_at_least_two_groups_create_one_multifusion = true;
  forest.current_batch_birth_nodes_never_same_batch_children = true;
  forest.all_group_carriers_attached_to_resulting_root_atomically = true;
  forest.locator_commits_unions_before_current_birth_bindings = true;
  forest.final_roots_cover_exactly_nonterminal_reduced_orders = true;
  forest.order_one_birth_and_node_prefix_implicit_and_unmaterialized = true;
  forest.no_partial_scientific_payload_published = true;
}

[[nodiscard]] ExactDirectMorseForestBatch batch(
    std::size_t batch_index,
    std::size_t order,
    std::int64_t squared_level,
    std::size_t birth_offset,
    std::size_t birth_count,
    std::size_t saddle_offset,
    std::size_t saddle_count,
    std::size_t group_offset,
    std::size_t group_count,
    std::size_t strict_carrier_count,
    std::size_t strict_root_count,
    std::size_t closed_carrier_count,
    std::size_t closed_root_count) {
  ExactDirectMorseForestBatch result;
  result.batch_index = batch_index;
  result.source_journal_batch_index = batch_index;
  result.order = order;
  result.squared_level = level(squared_level);
  result.birth_record_offset = birth_offset;
  result.birth_record_count = birth_count;
  result.saddle_record_offset = saddle_offset;
  result.saddle_record_count = saddle_count;
  result.atomic_group_offset = group_offset;
  result.atomic_group_count = group_count;
  result.strict_pre_batch_carrier_count = strict_carrier_count;
  result.strict_pre_batch_reduced_root_count = strict_root_count;
  result.closed_post_batch_carrier_count = closed_carrier_count;
  result.closed_post_batch_reduced_root_count = closed_root_count;
  result.strict_arms_resolved_before_mutation = true;
  result.quotient_resolved_before_mutation = true;
  result.unions_then_births_committed_atomically = true;
  return result;
}

[[nodiscard]] ExactDirectSparseComponentUnion component_union(
    std::size_t local_index,
    std::size_t global_index,
    ExactDirectSparseComponentHandle left,
    ExactDirectSparseComponentHandle right) {
  return {
      local_index,
      left,
      right,
      {authority_id,
       static_cast<std::uint64_t>(3U * global_index + 2U)}};
}

[[nodiscard]] ExactDirectSparseFacetBinding birth_binding(
    std::size_t local_index,
    std::size_t logical_birth_index,
    const ExactDirectSparseFacetKey& facet_key) {
  return {
      local_index,
      facet_key,
      logical_birth_index,
      {authority_id,
       static_cast<std::uint64_t>(3U * logical_birth_index + 1U)}};
}

void commit_batch(
    ExactDirectSparsePositiveFacetLocator& locator,
    ExactDirectMorseForestBatch& target_batch,
    std::span<const ExactDirectSparseComponentUnion> unions,
    std::span<const ExactDirectSparseFacetBinding> bindings,
    const std::string& context) {
  target_batch.strict_pre_batch_stamp = locator.snapshot_stamp();
  const auto committed = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{}, unions, bindings);
  check(committed.certified_committed_batch(), context);
  target_batch.committed_batch_stamp = locator.snapshot_stamp();
}

void populate_multiorder_locator_stamps(
    ExactDirectMorseForestJournalResult& forest) {
  auto locator = build_exact_direct_sparse_positive_facet_locator(
      11U, forest.requested_budget.locator_budget, forest.config.locator_config);
  check(locator.certified_positive_locator(), "multiorder locator initializes");

  forest.batches[0U].strict_pre_batch_stamp = locator.snapshot_stamp();
  check(
      locator.apply_canonical_singleton_identity_batch(4U)
          .certified_committed_identity_batch(),
      "multiorder singleton prefix commits");
  forest.batches[0U].committed_batch_stamp = locator.snapshot_stamp();

  const std::array<ExactDirectSparseComponentUnion, 3U> order_one_unions{
      component_union(0U, 0U, 0U, 1U),
      component_union(1U, 1U, 0U, 2U),
      component_union(2U, 2U, 0U, 3U),
  };
  commit_batch(
      locator,
      forest.batches[1U],
      order_one_unions,
      {},
      "multiorder order-one quotient commits");

  const std::array<ExactDirectSparseFacetBinding, 5U> order_two_births{
      birth_binding(0U, 4U, key({0U, 1U})),
      birth_binding(1U, 5U, key({0U, 2U})),
      birth_binding(2U, 6U, key({1U, 2U})),
      birth_binding(3U, 7U, key({1U, 3U})),
      birth_binding(4U, 8U, key({2U, 3U})),
  };
  commit_batch(
      locator,
      forest.batches[2U],
      {},
      order_two_births,
      "multiorder order-two births commit");

  const std::array<ExactDirectSparseComponentUnion, 4U> order_two_unions{
      component_union(0U, 3U, 4U, 5U),
      component_union(1U, 4U, 4U, 6U),
      component_union(2U, 5U, 4U, 7U),
      component_union(3U, 6U, 4U, 8U),
  };
  commit_batch(
      locator,
      forest.batches[3U],
      order_two_unions,
      {},
      "multiorder order-two quotient commits");

  const std::array<ExactDirectSparseFacetBinding, 2U> order_three_births{
      birth_binding(0U, 9U, key({0U, 1U, 2U})),
      birth_binding(1U, 10U, key({1U, 2U, 3U})),
  };
  commit_batch(
      locator,
      forest.batches[4U],
      {},
      order_three_births,
      "multiorder order-three births commit");
  commit_batch(
      locator,
      forest.batches[5U],
      {},
      {},
      "multiorder equal-level reduced births commit an empty locator batch");

  const std::array<ExactDirectSparseComponentUnion, 1U> order_three_union{
      component_union(0U, 7U, 9U, 10U),
  };
  commit_batch(
      locator,
      forest.batches[6U],
      order_three_union,
      {},
      "multiorder order-three multifusion commits");
  commit_batch(
      locator,
      forest.batches[7U],
      {},
      {},
      "multiorder order-three continuation commits an empty locator batch");
  forest.final_locator_stamp = locator.snapshot_stamp();
}

[[nodiscard]] ExactDirectMorseForestJournalResult multiorder_forest_fixture() {
  ExactDirectMorseForestJournalResult forest;
  forest.requested_budget = forest_budget();
  forest.config.locator_config.external_authority_id = authority_id;
  forest.point_count = 4U;
  forest.effective_maximum_order = 3U;
  forest.implicit_order_one_prefix_count = 4U;
  forest.birth_records = {
      {4U, 4U, 2U, 2U, key({0U, 1U}), 4U, std::nullopt, {authority_id, 13U}},
      {5U, 5U, 2U, 2U, key({0U, 2U}), 5U, std::nullopt, {authority_id, 16U}},
      {6U, 6U, 2U, 2U, key({1U, 2U}), 6U, std::nullopt, {authority_id, 19U}},
      {7U, 7U, 2U, 2U, key({1U, 3U}), 7U, std::nullopt, {authority_id, 22U}},
      {8U, 8U, 2U, 2U, key({2U, 3U}), 8U, std::nullopt, {authority_id, 25U}},
      {9U, 9U, 4U, 3U, key({0U, 1U, 2U}), 9U, std::nullopt, {authority_id, 28U}},
      {10U, 10U, 4U, 3U, key({1U, 2U, 3U}), 10U, std::nullopt, {authority_id, 31U}},
  };
  forest.arm_root_bindings = {
      {0U, 0U, 0U, key({0U}), 0U, 0U},
      {1U, 1U, 0U, key({1U}), 1U, 1U},
      {2U, 2U, 0U, key({2U}), 2U, 2U},
      {3U, 3U, 0U, key({3U}), 3U, 3U},
      {4U, 4U, 1U, key({0U, 1U}), 4U, std::nullopt},
      {5U, 5U, 1U, key({0U, 2U}), 5U, std::nullopt},
      {6U, 6U, 1U, key({1U, 2U}), 6U, std::nullopt},
      {7U, 7U, 2U, key({1U, 2U}), 6U, std::nullopt},
      {8U, 8U, 2U, key({1U, 3U}), 7U, std::nullopt},
      {9U, 9U, 2U, key({2U, 3U}), 8U, std::nullopt},
      {10U, 10U, 3U, key({0U, 1U, 2U}), 9U, std::nullopt},
      {11U, 11U, 4U, key({1U, 2U, 3U}), 10U, std::nullopt},
      {12U, 12U, 5U, key({0U, 1U, 2U}), 9U, 6U},
      {13U, 13U, 5U, key({0U, 1U, 2U}), 9U, 6U},
      {14U, 14U, 5U, key({1U, 2U, 3U}), 10U, 7U},
      {15U, 15U, 6U, key({0U, 1U, 2U}), 9U, 8U},
      {16U, 16U, 6U, key({1U, 2U, 3U}), 9U, 8U},
  };
  forest.saddle_records = {
      {0U, 0U, 0U, 1U, 0U, 4U, 4U, 0U, 4U, 0U},
      {1U, 1U, 1U, 3U, 4U, 3U, 3U, 3U, 0U, 1U},
      {2U, 2U, 2U, 3U, 7U, 3U, 3U, 3U, 0U, 1U},
      {3U, 3U, 3U, 5U, 10U, 1U, 1U, 1U, 0U, 2U},
      {4U, 4U, 4U, 5U, 11U, 1U, 1U, 1U, 0U, 3U},
      {5U, 5U, 5U, 6U, 12U, 3U, 2U, 0U, 2U, 4U},
      {6U, 6U, 6U, 7U, 15U, 2U, 1U, 0U, 1U, 5U},
  };
  forest.atomic_groups = {
      {0U, 1U, 0U, 1U, 4U, 0U, 4U, 0U, 4U, 4U, 4U,
       ExactDirectMorseForestAtomicGroupKind::multifusion},
      {1U, 3U, 1U, 2U, 5U, 5U, 0U, 4U, 0U, 5U, 5U,
       ExactDirectMorseForestAtomicGroupKind::reduced_birth},
      {2U, 5U, 3U, 1U, 1U, 1U, 0U, 4U, 0U, 6U, 6U,
       ExactDirectMorseForestAtomicGroupKind::reduced_birth},
      {3U, 5U, 4U, 1U, 1U, 1U, 0U, 4U, 0U, 7U, 7U,
       ExactDirectMorseForestAtomicGroupKind::reduced_birth},
      {4U, 6U, 5U, 1U, 2U, 0U, 2U, 4U, 2U, 8U, 8U,
       ExactDirectMorseForestAtomicGroupKind::multifusion},
      {5U, 7U, 6U, 1U, 1U, 0U, 1U, 6U, 0U, std::nullopt, 8U,
       ExactDirectMorseForestAtomicGroupKind::continuation},
  };
  forest.child_node_ids = {0U, 1U, 2U, 3U, 6U, 7U};
  forest.nodes = {
      {4U, 1U, level(1), ExactDirectMorseForestNodeKind::multifusion,
       0U, 4U, std::nullopt, 0U},
      {5U, 2U, level(53), ExactDirectMorseForestNodeKind::reduced_birth,
       4U, 0U, std::nullopt, 1U},
      {6U, 3U, level(51), ExactDirectMorseForestNodeKind::reduced_birth,
       4U, 0U, std::nullopt, 2U},
      {7U, 3U, level(51), ExactDirectMorseForestNodeKind::reduced_birth,
       4U, 0U, std::nullopt, 3U},
      {8U, 3U, level(52), ExactDirectMorseForestNodeKind::multifusion,
       4U, 2U, std::nullopt, 4U},
  };
  forest.batches = {
      batch(0U, 1U, 0, 0U, 4U, 0U, 0U, 0U, 0U, 0U, 0U, 4U, 4U),
      batch(1U, 1U, 1, 4U, 0U, 0U, 1U, 0U, 1U, 4U, 4U, 1U, 1U),
      batch(2U, 2U, 52, 4U, 5U, 1U, 0U, 1U, 0U, 0U, 0U, 5U, 0U),
      batch(3U, 2U, 53, 9U, 0U, 1U, 2U, 1U, 1U, 5U, 0U, 1U, 1U),
      batch(4U, 3U, 50, 9U, 2U, 3U, 0U, 2U, 0U, 0U, 0U, 2U, 0U),
      batch(5U, 3U, 51, 11U, 0U, 3U, 2U, 2U, 2U, 2U, 0U, 2U, 2U),
      batch(6U, 3U, 52, 11U, 0U, 5U, 1U, 4U, 1U, 2U, 2U, 1U, 1U),
      batch(7U, 3U, 53, 11U, 0U, 6U, 1U, 5U, 1U, 1U, 1U, 1U, 1U),
  };
  forest.final_roots = {
      {0U, 1U, 0U, 4U},
      {1U, 2U, 4U, 5U},
      {2U, 3U, 9U, 8U},
  };
  forest.counters.birth_record_count = 11U;
  forest.counters.latent_higher_order_birth_count = 7U;
  forest.counters.order_one_birth_node_count = 4U;
  forest.counters.arm_root_binding_count = 17U;
  forest.counters.saddle_record_count = 7U;
  forest.counters.atomic_group_count = 6U;
  forest.counters.reduced_birth_group_count = 3U;
  forest.counters.continuation_group_count = 1U;
  forest.counters.multifusion_group_count = 2U;
  forest.counters.child_reference_count = 6U;
  forest.counters.batch_record_count = 8U;
  forest.counters.node_count = 9U;
  forest.counters.final_root_count = 3U;
  forest.counters.locator_union_count = 8U;
  forest.counters.closure_call_count = 5U;
  forest.counters.quotient_call_count = 5U;
  forest.counters.distinct_strict_arm_count = 15U;
  forest.counters.duplicate_strict_arm_reference_count = 2U;
  forest.counters.maximum_batch_arm_count = 6U;
  forest.counters.maximum_batch_carrier_arity = 5U;
  forest.counters.maximum_batch_merge_arity = 4U;
  forest.logical_output_entry_count = 256U;
  populate_multiorder_locator_stamps(forest);
  set_success_flags(forest);
  forest.decision = ExactDirectMorseForestDecision::
      complete_conditional_exact_direct_morse_forest;
  forest.scope = ExactDirectMorseForestScope::
      all_orders_direct_minimum_carriers_strict_arms_recursive_positive_terminals_and_atomic_full_component_saddle_quotients_with_reduced_qr_only;
  return forest;
}

[[nodiscard]] CanonicalPointCloud cloud_fixture() {
  const std::array<CertifiedPoint3, 4U> points{
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(10.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(11.0, 0.0, 0.0),
  };
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] CanonicalPointCloud foreign_cloud_fixture() {
  const std::array<CertifiedPoint3, 4U> points{
      CertifiedPoint3::from_binary64(100.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(101.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(110.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(111.0, 0.0, 0.0),
  };
  return CanonicalPointCloud::rejecting_duplicates(points);
}

void bind_forest_to_cloud(
    ExactDirectMorseForestJournalResult& forest,
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud) {
  forest.source_higher_canonical_cloud_digest =
      make_exact_higher_support_checkpoint_manifest(
          index, cloud, forest.effective_maximum_order)
          .canonical_cloud_digest;
}

[[nodiscard]] ExactDirectMorseForestJournalResult conflict_forest_fixture() {
  ExactDirectMorseForestJournalResult forest;
  forest.requested_budget = forest_budget();
  forest.config.locator_config.external_authority_id = authority_id;
  forest.point_count = 3U;
  forest.effective_maximum_order = 2U;
  forest.implicit_order_one_prefix_count = 3U;
  forest.birth_records = {
      {3U, 3U, 2U, 2U, key({0U, 1U}), 3U, std::nullopt, {authority_id, 10U}},
      {4U, 4U, 2U, 2U, key({1U, 2U}), 4U, std::nullopt, {authority_id, 13U}},
  };
  forest.arm_root_bindings = {
      {0U, 0U, 0U, key({0U}), 0U, 0U},
      {1U, 1U, 0U, key({1U}), 1U, 1U},
      {2U, 2U, 0U, key({2U}), 2U, 2U},
      {3U, 3U, 1U, key({0U, 1U}), 3U, std::nullopt},
      {4U, 4U, 1U, key({1U, 2U}), 4U, std::nullopt},
  };
  forest.saddle_records = {
      {0U, 0U, 0U, 1U, 0U, 3U, 3U, 0U, 3U, 0U},
      {1U, 1U, 1U, 3U, 3U, 2U, 2U, 2U, 0U, 1U},
  };
  forest.atomic_groups = {
      {0U, 1U, 0U, 1U, 3U, 0U, 3U, 0U, 3U, 3U, 3U,
       ExactDirectMorseForestAtomicGroupKind::multifusion},
      {1U, 3U, 1U, 1U, 2U, 2U, 0U, 3U, 0U, 4U, 4U,
       ExactDirectMorseForestAtomicGroupKind::reduced_birth},
  };
  forest.child_node_ids = {0U, 1U, 2U};
  forest.nodes = {
      {3U, 1U, level(100), ExactDirectMorseForestNodeKind::multifusion,
       0U, 3U, std::nullopt, 0U},
      {4U, 2U, level(50), ExactDirectMorseForestNodeKind::reduced_birth,
       3U, 0U, std::nullopt, 1U},
  };
  forest.batches = {
      batch(0U, 1U, 0, 0U, 3U, 0U, 0U, 0U, 0U, 0U, 0U, 3U, 3U),
      batch(1U, 1U, 100, 3U, 0U, 0U, 1U, 0U, 1U, 3U, 3U, 1U, 1U),
      batch(2U, 2U, 40, 3U, 2U, 1U, 0U, 1U, 0U, 0U, 0U, 2U, 0U),
      batch(3U, 2U, 50, 5U, 0U, 1U, 1U, 1U, 1U, 2U, 0U, 1U, 1U),
  };
  forest.final_roots = {
      {0U, 1U, 0U, 3U},
      {1U, 2U, 3U, 4U},
  };
  forest.counters.birth_record_count = 5U;
  forest.counters.latent_higher_order_birth_count = 2U;
  forest.counters.order_one_birth_node_count = 3U;
  forest.counters.arm_root_binding_count = 5U;
  forest.counters.saddle_record_count = 2U;
  forest.counters.atomic_group_count = 2U;
  forest.counters.reduced_birth_group_count = 1U;
  forest.counters.multifusion_group_count = 1U;
  forest.counters.child_reference_count = 3U;
  forest.counters.batch_record_count = 4U;
  forest.counters.node_count = 5U;
  forest.counters.final_root_count = 2U;
  forest.counters.locator_union_count = 3U;
  forest.counters.quotient_call_count = 2U;
  forest.counters.distinct_strict_arm_count = 5U;
  forest.counters.maximum_batch_arm_count = 3U;
  forest.counters.maximum_batch_carrier_arity = 3U;
  forest.counters.maximum_batch_merge_arity = 3U;
  forest.logical_output_entry_count = 64U;

  auto locator = build_exact_direct_sparse_positive_facet_locator(
      5U, forest.requested_budget.locator_budget, forest.config.locator_config);
  forest.batches[0U].strict_pre_batch_stamp = locator.snapshot_stamp();
  check(
      locator.apply_canonical_singleton_identity_batch(3U)
          .certified_committed_identity_batch(),
      "conflict singleton prefix commits");
  forest.batches[0U].committed_batch_stamp = locator.snapshot_stamp();
  const std::array<ExactDirectSparseComponentUnion, 2U> singleton_unions{
      component_union(0U, 0U, 0U, 1U),
      component_union(1U, 1U, 0U, 2U),
  };
  commit_batch(
      locator,
      forest.batches[1U],
      singleton_unions,
      {},
      "conflict future singleton quotient commits");
  const std::array<ExactDirectSparseFacetBinding, 2U> pair_births{
      birth_binding(0U, 3U, key({0U, 1U})),
      birth_binding(1U, 4U, key({1U, 2U})),
  };
  commit_batch(
      locator,
      forest.batches[2U],
      {},
      pair_births,
      "conflict pair births commit");
  const std::array<ExactDirectSparseComponentUnion, 1U> pair_union{
      component_union(0U, 2U, 3U, 4U),
  };
  commit_batch(
      locator,
      forest.batches[3U],
      pair_union,
      {},
      "conflict pair quotient commits");
  forest.final_locator_stamp = locator.snapshot_stamp();
  set_success_flags(forest);
  forest.decision = ExactDirectMorseForestDecision::
      complete_conditional_exact_direct_morse_forest;
  forest.scope = ExactDirectMorseForestScope::
      all_orders_direct_minimum_carriers_strict_arms_recursive_positive_terminals_and_atomic_full_component_saddle_quotients_with_reduced_qr_only;
  return forest;
}

[[nodiscard]] CanonicalPointCloud conflict_cloud_fixture() {
  const std::array<CertifiedPoint3, 3U> points{
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(2.0, 0.0, 0.0),
  };
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] std::vector<ExactDirectMorseVerticalTargetProposal>
manual_composition(
    const ExactDirectMorseForestJournalResult& forest,
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud) {
  auto target_one =
      build_exact_direct_morse_forest_carrier_cut_replay_session(
          forest, 1U, session_budget());
  auto target_two =
      build_exact_direct_morse_forest_carrier_cut_replay_session(
          forest, 2U, session_budget());
  check(
      target_one.certified_ready_session() &&
          target_two.certified_ready_session(),
      "manual comparison sessions initialize");
  std::vector<ExactDirectMorseVerticalTargetProposal> proposals;
  if (!target_one.session || !target_two.session) {
    return proposals;
  }
  for (const auto& batch_record : forest.batches) {
    if (batch_record.order < 2U) {
      continue;
    }
    for (std::size_t local_group = 0U;
         local_group < batch_record.atomic_group_count;
         ++local_group) {
      const std::size_t group_index =
          batch_record.atomic_group_offset + local_group;
      const auto plan = build_exact_direct_morse_vertical_target_facet_plan(
          forest, group_index, plan_budget());
      check(
          plan.certified_group_local_target_facet_plan(),
          "manual group plan certifies");
      auto& session = batch_record.order == 2U
                          ? *target_one.session
                          : *target_two.session;
      const std::uint64_t token =
          static_cast<std::uint64_t>(3U * (group_index + 1U));
      bool group_certified = false;
      std::vector<ExactDirectMorseVerticalTargetProposal> group_proposals;
      const auto advanced = session.advance_to_closed_cut(
          batch_record.squared_level,
          [&](const ExactDirectMorseForestCarrierCutReplayView& view) {
            const auto closure =
                view.build_closure_summary_from_canonical_distinct_keys(
                    index,
                    cloud,
                    plan.canonical_distinct_target_facet_keys,
                    {authority_id, token},
                    closure_adapter_budget(),
                    forest.config.closure_config,
                    forest.traversal_order);
            if (!closure.certified_compact_closure_summary()) {
              return;
            }
            auto adapted =
                view.build_vertical_target_proposals_from_group_plan(
                    forest, plan, closure, proposal_adapter_budget());
            group_certified =
                adapted.certified_group_local_vertical_target_proposals();
            group_proposals = std::move(adapted.proposals);
          });
      check(
          group_certified && advanced.certified_forest_relative_closed_cut(),
          "manual group composition certifies after the replay receipt");
      proposals.insert(
          proposals.end(),
          group_proposals.begin(),
          group_proposals.end());
    }
  }
  return proposals;
}

void test_complete_multiorder_pipeline() {
  auto forest = multiorder_forest_fixture();
  const auto cloud = cloud_fixture();
  const auto index = MortonLbvhIndex::build(cloud);
  bind_forest_to_cloud(forest, index, cloud);
  check(
      forest.certified_conditional_h0_candidate(),
      "the multiorder forest fixture is certified");
  const auto result =
      build_exact_direct_morse_vertical_target_proposal_pipeline(
          forest, index, cloud, pipeline_budget());
  check(
      result.certified_multiorder_target_proposals() &&
          result.decision ==
              ExactDirectMorseVerticalTargetProposalPipelineDecision::
                  complete_with_unresolved_multiorder_target_proposals,
      "the full group-local multiorder proposal pipeline certifies");
  check(
      result.session_audits.size() == 2U &&
          result.group_audits.size() == 5U &&
          result.proposals.size() == 11U &&
          result.required_representative_count == 11U &&
          result.required_projected_target_facet_reference_count == 28U &&
          result.required_distinct_target_facet_count == 20U &&
          result.counters.equal_level_same_target_order_group_count == 1U,
      "two sessions cover five groups, including one repeated exact cut");
  check(
      result.group_audits[0U].source_binding_scan_count == 6U &&
          result.group_audits[0U].representative_count == 5U &&
          result.group_audits[0U].projected_target_facet_reference_count ==
              10U &&
          result.group_audits[0U].distinct_target_facet_count == 4U &&
          result.group_audits[3U].source_binding_scan_count == 3U &&
          result.group_audits[3U].representative_count == 2U &&
          result.group_audits[3U].projected_target_facet_reference_count ==
              6U &&
          result.group_audits[3U].distinct_target_facet_count == 5U,
      "duplicates collapse to Q representatives while shared lower shadows remain D<I");
  check(
      result.group_audits[1U].closure_unresolved_terminal_count == 3U &&
          result.group_audits[2U].closure_unresolved_terminal_count == 3U &&
          result.group_audits[3U].closure_active_latent_terminal_count == 5U &&
          result.group_audits[4U].closure_resolved_terminal_count == 5U &&
          result.counters.unresolved_proposal_count == 4U &&
          result.counters.resolved_proposal_count == 7U,
      "historical cuts distinguish unresolved, active-latent and resolved targets");
  check(
      result.group_audits[0U].invocation_replay_token == 6U &&
          result.group_audits[1U].invocation_replay_token == 9U &&
          result.group_audits[4U].invocation_replay_token == 18U &&
          std::all_of(
              result.proposals.begin(),
              result.proposals.end(),
              [](const auto& proposal) {
                return proposal.replay_token % 3U == 0U;
              }),
      "group invocation tokens occupy only the non-birth/non-union residue");
  check(
      result.lbvh_validated_for_cloud_and_point_count_matches &&
          result.matching_canonical_point_namespace_required &&
          result.forest_to_cloud_namespace_identity_certified &&
          result.source_forest_canonical_cloud_digest ==
              forest.source_higher_canonical_cloud_digest &&
          result.replayed_canonical_cloud_digest ==
              forest.source_higher_canonical_cloud_digest &&
          !result.external_target_authority_replayed &&
          !result.vertical_maps_complete && !result.public_status_claimed &&
          !result.global_facet_coface_incidence_cell_gamma_or_delaunay_materialized &&
          result.no_plan_closure_locator_or_session_reference_retained,
      "the pipeline binds the source forest to the freshly replayed cloud without public promotion");

  const auto foreign_cloud = foreign_cloud_fixture();
  const auto foreign_index = MortonLbvhIndex::build(foreign_cloud);
  const auto foreign =
      build_exact_direct_morse_vertical_target_proposal_pipeline(
          forest, foreign_index, foreign_cloud, pipeline_budget());
  check(
      foreign.decision ==
              ExactDirectMorseVerticalTargetProposalPipelineDecision::
                  no_pipeline_point_namespace_rejected &&
          foreign.certified_atomic_failure() && foreign.proposals.empty() &&
          foreign.group_audits.empty() && foreign.session_audits.empty() &&
          !foreign.forest_to_cloud_namespace_identity_certified,
      "a fresh same-cardinality LBVH over a foreign cloud is rejected atomically");

  auto mutated_digest_forest = forest;
  mutated_digest_forest.source_higher_canonical_cloud_digest =
      make_exact_higher_support_checkpoint_manifest(
          foreign_index,
          foreign_cloud,
          mutated_digest_forest.effective_maximum_order)
          .canonical_cloud_digest;
  const auto mutated_digest =
      build_exact_direct_morse_vertical_target_proposal_pipeline(
          mutated_digest_forest, index, cloud, pipeline_budget());
  check(
      mutated_digest.decision ==
              ExactDirectMorseVerticalTargetProposalPipelineDecision::
                  no_pipeline_point_namespace_rejected &&
          mutated_digest.certified_atomic_failure() &&
          mutated_digest.proposals.empty() &&
          mutated_digest.group_audits.empty() &&
          mutated_digest.session_audits.empty() &&
          !mutated_digest.forest_to_cloud_namespace_identity_certified,
      "a certified forest carrying a different nonzero cloud digest is rejected atomically");

  const auto manual = manual_composition(forest, index, cloud);
  check(
      manual == result.proposals,
      "the reusable pipeline equals direct manual group composition");

  const auto vertical = build_exact_direct_morse_vertical_journal(
      forest,
      result.proposals,
      vertical_budget(),
      ExactDirectMorseVerticalConfig{authority_id});
  check(
      vertical.certified_conditional_vertical_candidate() &&
          vertical.counters.expected_label_count == result.proposals.size() &&
          vertical.counters.missing_label_count == 0U &&
          vertical.counters.unresolved_label_count == 4U &&
          vertical.counters.resolved_label_count == 7U &&
          !vertical.external_target_authority_replayed &&
          !vertical.vertical_maps_complete,
      "the downstream journal consumes every proposal with zero missing labels");

  auto exact_budget = pipeline_budget();
  exact_budget.maximum_source_batch_scan_count =
      result.counters.source_batch_scan_count;
  exact_budget.maximum_source_atomic_group_scan_count =
      result.counters.source_atomic_group_scan_count;
  exact_budget.maximum_referenced_target_order_count =
      result.required_referenced_target_order_count;
  exact_budget.maximum_target_order_lookup_count =
      result.counters.target_order_lookup_count;
  exact_budget.maximum_preflight_facet_plan_count =
      result.counters.preflight_facet_plan_count;
  exact_budget.maximum_executed_facet_plan_count =
      result.counters.executed_facet_plan_count;
  exact_budget.maximum_replay_advance_count =
      result.counters.replay_advance_count;
  exact_budget.maximum_closure_build_count =
      result.counters.closure_build_count;
  exact_budget.maximum_proposal_adapter_count =
      result.counters.proposal_adapter_count;
  exact_budget.maximum_aggregate_representative_count =
      result.required_representative_count;
  exact_budget.maximum_aggregate_projected_target_facet_reference_count =
      result.required_projected_target_facet_reference_count;
  exact_budget.maximum_aggregate_distinct_target_facet_count =
      result.required_distinct_target_facet_count;
  exact_budget.maximum_aggregate_retained_key_point_reference_count =
      result.required_retained_key_point_reference_count;
  exact_budget.maximum_aggregate_plan_logical_output_entry_count =
      result.required_plan_logical_output_entry_count;
  exact_budget.maximum_aggregate_closure_terminal_summary_count =
      result.required_closure_terminal_summary_count;
  exact_budget.maximum_aggregate_proposal_count =
      result.required_proposal_count;
  exact_budget.maximum_session_audit_count = result.session_audits.size();
  exact_budget.maximum_group_audit_count = result.group_audits.size();
  exact_budget.maximum_logical_output_entry_count =
      result.required_logical_output_entry_count;
  const auto exact =
      build_exact_direct_morse_vertical_target_proposal_pipeline(
          forest, index, cloud, exact_budget);
  check(
      exact.certified_multiorder_target_proposals() &&
          exact.proposals == result.proposals,
      "every aggregate pipeline cap accepts its exact observed requirement");

  auto one_less = exact_budget;
  one_less.maximum_aggregate_proposal_count =
      result.required_proposal_count - 1U;
  const auto rejected =
      build_exact_direct_morse_vertical_target_proposal_pipeline(
          forest, index, cloud, one_less);
  check(
      rejected.decision ==
              ExactDirectMorseVerticalTargetProposalPipelineDecision::
                  no_pipeline_budget_exhausted &&
          rejected.certified_atomic_failure() && rejected.proposals.empty() &&
          rejected.group_audits.empty() && rejected.session_audits.empty(),
      "one-less aggregate proposal capacity fails before session publication");

  auto late_budget = pipeline_budget();
  late_budget.closure_budget.maximum_terminal_summary_count = 4U;
  const auto late =
      build_exact_direct_morse_vertical_target_proposal_pipeline(
          forest, index, cloud, late_budget);
  check(
      late.decision ==
              ExactDirectMorseVerticalTargetProposalPipelineDecision::
                  no_pipeline_closure_rejected &&
          late.rejected_source_atomic_group_index ==
              std::optional<std::size_t>{4U} &&
          late.certified_atomic_failure() && late.proposals.empty() &&
          late.group_audits.empty() && late.session_audits.empty(),
      "a late fourth-group closure failure erases all prior scientific payload");

  auto forged_level = result;
  forged_level.group_audits[2U].source_batch_squared_level = level(50);
  auto forged_prefix = result;
  forged_prefix.group_audits[2U].committed_global_batch_prefix_count = 1U;
  auto forged_equal_count = result;
  ++forged_equal_count.counters.equal_level_same_target_order_group_count;
  auto forged_namespace = result;
  forged_namespace.forest_to_cloud_namespace_identity_certified = false;
  auto forged_replayed_digest = result;
  forged_replayed_digest.replayed_canonical_cloud_digest =
      make_exact_higher_support_checkpoint_manifest(
          foreign_index, foreign_cloud, forest.effective_maximum_order)
          .canonical_cloud_digest;
  auto forged_plan_budget = result;
  forged_plan_budget.requested_budget.facet_plan_budget
      .maximum_distinct_target_facet_count = 4U;
  auto forged_session_budget = result;
  forged_session_budget.requested_budget.session_budget
      .maximum_replayed_global_batch_count = 3U;
  check(
      !forged_level.certified_multiorder_target_proposals() &&
          !forged_prefix.certified_multiorder_target_proposals() &&
          !forged_equal_count.certified_multiorder_target_proposals() &&
          !forged_namespace.certified_multiorder_target_proposals() &&
          !forged_replayed_digest.certified_multiorder_target_proposals() &&
          !forged_plan_budget.certified_multiorder_target_proposals() &&
          !forged_session_budget.certified_multiorder_target_proposals(),
      "the certificate reconstructs monotonicity, equal cuts, stage budgets and namespace identity");
}

void test_known_target_root_conflict() {
  auto forest = conflict_forest_fixture();
  const auto cloud = conflict_cloud_fixture();
  const auto index = MortonLbvhIndex::build(cloud);
  bind_forest_to_cloud(forest, index, cloud);
  check(
      forest.certified_conditional_h0_candidate(),
      "the historical conflict forest fixture is certified");
  const auto result =
      build_exact_direct_morse_vertical_target_proposal_pipeline(
          forest, index, cloud, pipeline_budget());
  check(
      result.decision ==
              ExactDirectMorseVerticalTargetProposalPipelineDecision::
                  no_pipeline_proposal_adapter_rejected &&
          result.rejected_source_atomic_group_index ==
              std::optional<std::size_t>{1U} &&
          result.rejected_adapter_decision ==
              ExactDirectMorseVerticalTargetProposalAdapterDecision::
                  no_adapter_known_target_root_contradiction &&
          result.certified_atomic_failure() && result.proposals.empty() &&
          result.group_audits.empty() && result.session_audits.empty(),
      "divergent known singleton roots reject the complete pipeline atomically");
}

}  // namespace

int main() {
  test_complete_multiorder_pipeline();
  test_known_target_root_conflict();
  if (failures != 0) {
    std::cerr << failures
              << " direct Morse vertical target proposal pipeline check(s) failed\n";
    return 1;
  }
  std::cout
      << "direct Morse vertical target proposal pipeline checks passed\n";
  return 0;
}
