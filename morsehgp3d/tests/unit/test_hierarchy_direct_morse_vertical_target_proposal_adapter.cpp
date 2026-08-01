#include "morsehgp3d/hierarchy/direct_morse_forest_carrier_cut_replay_session.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <span>
#include <string>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

constexpr std::uint64_t authority_id = UINT64_C(0xC0175E5510);
constexpr std::uint64_t invocation_replay_token = UINT64_C(999);
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
  budget.maximum_component_handle_count = 32U;
  budget.maximum_committed_binding_count = 32U;
  budget.maximum_committed_key_point_count = 128U;
  budget.maximum_committed_union_count = 32U;
  budget.maximum_committed_batch_count = 32U;
  budget.maximum_batch_query_count = 32U;
  budget.maximum_batch_union_count = 32U;
  budget.maximum_batch_binding_count = 32U;
  budget.maximum_batch_key_point_count = 128U;
  budget.maximum_table_slot_count = 128U;
  budget.maximum_batch_scratch_slot_count = 128U;
  return budget;
}

[[nodiscard]] ExactDirectMorseForestBudget forest_budget() {
  ExactDirectMorseForestBudget budget;
  budget.maximum_source_role_scan_count = 128U;
  budget.maximum_source_batch_scan_count = 128U;
  budget.maximum_source_family_scan_count = 128U;
  budget.maximum_source_arm_seed_scan_count = 128U;
  budget.maximum_birth_record_count = 128U;
  budget.maximum_arm_root_binding_count = 128U;
  budget.maximum_saddle_record_count = 128U;
  budget.maximum_atomic_group_count = 128U;
  budget.maximum_child_reference_count = 128U;
  budget.maximum_batch_record_count = 128U;
  budget.maximum_node_count = 128U;
  budget.maximum_final_root_count = 128U;
  budget.maximum_batch_distinct_arm_count = 128U;
  budget.maximum_logical_output_entry_count = 1024U;
  budget.maximum_aggregate_closure_node_count = 128U;
  budget.maximum_aggregate_closure_step_call_count = 128U;
  budget.locator_budget = locator_budget();
  return budget;
}

[[nodiscard]] ExactDirectMorseForestCarrierCutIndexBudget index_budget() {
  ExactDirectMorseForestCarrierCutIndexBudget budget;
  budget.maximum_forest_birth_record_scan_count = 128U;
  budget.maximum_forest_node_scan_count = 128U;
  budget.maximum_forest_batch_scan_count = 128U;
  budget.maximum_forest_atomic_group_scan_count = 128U;
  budget.maximum_forest_saddle_scan_count = 128U;
  budget.maximum_forest_arm_binding_scan_count = 128U;
  budget.maximum_forest_child_reference_scan_count = 128U;
  budget.maximum_forest_final_root_scan_count = 128U;
  budget.maximum_component_state_count = 128U;
  budget.maximum_node_marker_state_count = 128U;
  budget.maximum_index_entry_count = 128U;
  budget.maximum_group_carrier_scratch_count = 128U;
  budget.maximum_group_prior_root_scratch_count = 128U;
  budget.maximum_parent_hop_count = 4096U;
  budget.maximum_exact_level_comparison_count = 4096U;
  budget.maximum_single_exact_level_integer_bit_count = 64U;
  budget.maximum_logical_output_entry_count = 1024U;
  return budget;
}

[[nodiscard]] ExactDirectMorseForestCarrierCutReplaySessionBudget
session_budget() {
  ExactDirectMorseForestCarrierCutReplaySessionBudget budget;
  budget.carrier_state_budget = index_budget();
  budget.locator_budget = locator_budget();
  budget.maximum_replayed_global_batch_count = 32U;
  budget.maximum_replayed_locator_union_count = 32U;
  budget.maximum_replayed_locator_binding_count = 32U;
  budget.maximum_batch_group_plan_count = 16U;
  budget.maximum_batch_group_carrier_reference_count = 32U;
  budget.maximum_batch_group_prior_root_reference_count = 32U;
  budget.maximum_batch_locator_union_scratch_count = 32U;
  budget.maximum_batch_locator_binding_scratch_count = 32U;
  return budget;
}

[[nodiscard]] ExactDirectSparseFacetDescentClosureBudget closure_budget() {
  ExactDirectSparseFacetDescentStepBudget step_budget;
  step_budget.source_locator_probe = {128U, 32U};
  step_budget.top_k_query = ExactLbvhTopKBudget{
      1024U, 1024U, 1024U, 1024U, 64U, 10U, 10U};
  step_budget.successor_locator_probe = {128U, 32U};
  return {32U, 32U, 32U, 65U, step_budget};
}

[[nodiscard]] ExactDirectMorseForestCarrierCutClosureAdapterBudget
closure_adapter_budget() {
  return {32U, 32U, 128U, 32U, 256U, closure_budget()};
}

[[nodiscard]] ExactDirectMorseVerticalTargetFacetPlanBudget plan_budget() {
  return {32U, 32U, 32U, 32U, 64U, 32U, 1024U, 1024U, 256U, 512U};
}

[[nodiscard]] ExactDirectMorseVerticalTargetProposalAdapterBudget
proposal_budget() {
  ExactDirectMorseVerticalTargetProposalAdapterBudget budget;
  budget.maximum_source_saddle_revalidation_count = 32U;
  budget.maximum_source_binding_revalidation_count = 32U;
  budget.maximum_source_key_lookup_comparison_count = 1024U;
  budget.maximum_closure_summary_scan_count = 32U;
  budget.maximum_positive_terminal_probe_count = 64U;
  budget.maximum_positive_terminal_probe_slot_visit_count = 4096U;
  budget.maximum_positive_terminal_probe_parent_hop_count = 4096U;
  budget.positive_terminal_probe_budget = {128U, 32U};
  budget.maximum_carrier_entry_revalidation_count = 64U;
  budget.maximum_target_node_lookup_count = 32U;
  budget.maximum_exact_level_comparison_count = 128U;
  budget.maximum_single_exact_level_integer_bit_count = 64U;
  budget.maximum_proposal_count = 32U;
  budget.maximum_logical_output_entry_count = 32U;
  return budget;
}

[[nodiscard]] ExactDirectMorseVerticalBudget vertical_budget() {
  ExactDirectMorseVerticalBudget budget;
  budget.maximum_forest_node_scan_count = 128U;
  budget.maximum_child_reference_scan_count = 128U;
  budget.maximum_birth_record_scan_count = 128U;
  budget.maximum_batch_scan_count = 128U;
  budget.maximum_atomic_group_scan_count = 128U;
  budget.maximum_saddle_scan_count = 128U;
  budget.maximum_arm_binding_scan_count = 128U;
  budget.maximum_proposal_count = 128U;
  budget.maximum_label_resolution_count = 128U;
  budget.maximum_group_check_count = 128U;
  budget.maximum_checkpoint_count = 128U;
  budget.maximum_adjacent_family_count = 8U;
  budget.maximum_group_sort_scratch_count = 128U;
  budget.maximum_group_sort_comparison_count = 1024U;
  budget.maximum_target_parent_hop_count = 1024U;
  budget.maximum_exact_level_comparison_count = 1024U;
  budget.maximum_single_exact_level_integer_bit_count = 64U;
  budget.maximum_logical_output_entry_count = 1024U;
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

void populate_locator_stamps(ExactDirectMorseForestJournalResult& forest) {
  auto locator = build_exact_direct_sparse_positive_facet_locator(
      5U, forest.requested_budget.locator_budget, forest.config.locator_config);
  check(locator.certified_positive_locator(), "fixture locator initializes");

  forest.batches[0U].strict_pre_batch_stamp = locator.snapshot_stamp();
  check(
      locator.apply_canonical_singleton_identity_batch(3U)
          .certified_committed_identity_batch(),
      "singleton prefix commits");
  forest.batches[0U].committed_batch_stamp = locator.snapshot_stamp();

  forest.batches[1U].strict_pre_batch_stamp = locator.snapshot_stamp();
  const ExactDirectSparseComponentUnion order_one_unions[] = {
      component_union(0U, 0U, 0U, 1U),
      component_union(1U, 1U, 0U, 2U),
  };
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              order_one_unions,
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch(),
      "lower-order quotient commits");
  forest.batches[1U].committed_batch_stamp = locator.snapshot_stamp();

  forest.batches[2U].strict_pre_batch_stamp = locator.snapshot_stamp();
  const ExactDirectSparseFacetBinding births[] = {
      {0U, key({0U, 1U}), 3U, {authority_id, 10U}},
      {1U, key({1U, 2U}), 4U, {authority_id, 13U}},
  };
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              births)
          .certified_committed_batch(),
      "target births commit");
  forest.batches[2U].committed_batch_stamp = locator.snapshot_stamp();

  forest.batches[3U].strict_pre_batch_stamp = locator.snapshot_stamp();
  const ExactDirectSparseComponentUnion target_union[] = {
      component_union(0U, 2U, 3U, 4U),
  };
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              target_union,
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch(),
      "target reduced-birth quotient commits");
  forest.batches[3U].committed_batch_stamp = locator.snapshot_stamp();
  forest.final_locator_stamp = locator.snapshot_stamp();
}

[[nodiscard]] ExactDirectMorseForestJournalResult forest_fixture() {
  ExactDirectMorseForestJournalResult forest;
  forest.source_higher_canonical_cloud_digest =
      morsehgp3d::contract::CanonicalId::from_lower_hex(
          std::string(64U, '1'));
  forest.requested_budget = forest_budget();
  forest.config.locator_config.external_authority_id = authority_id;
  forest.point_count = 3U;
  forest.effective_maximum_order = 2U;
  forest.implicit_order_one_prefix_count = 3U;
  forest.birth_records = {
      {3U,
       3U,
       2U,
       2U,
       key({0U, 1U}),
       3U,
       std::nullopt,
       {authority_id, 10U}},
      {4U,
       4U,
       2U,
       2U,
       key({1U, 2U}),
       4U,
       std::nullopt,
       {authority_id, 13U}},
  };
  forest.arm_root_bindings = {
      {0U, 0U, 0U, key({0U}), 0U, 0U},
      {1U, 1U, 0U, key({1U}), 1U, 1U},
      {2U, 2U, 0U, key({2U}), 2U, 2U},
      {3U, 3U, 1U, key({1U, 2U}), 4U, std::nullopt},
      {4U, 4U, 1U, key({0U, 1U}), 3U, std::nullopt},
  };
  forest.saddle_records = {
      {0U, 0U, 0U, 1U, 0U, 3U, 3U, 0U, 3U, 0U},
      {1U, 1U, 1U, 3U, 3U, 2U, 2U, 2U, 0U, 1U},
  };
  forest.atomic_groups = {
      {0U,
       1U,
       0U,
       1U,
       3U,
       0U,
       3U,
       0U,
       3U,
       3U,
       3U,
       ExactDirectMorseForestAtomicGroupKind::multifusion},
      {1U,
       3U,
       1U,
       1U,
       2U,
       2U,
       0U,
       3U,
       0U,
       4U,
       4U,
       ExactDirectMorseForestAtomicGroupKind::reduced_birth},
  };
  forest.child_node_ids = {0U, 1U, 2U};
  forest.nodes = {
      {3U,
       1U,
       level(1),
       ExactDirectMorseForestNodeKind::multifusion,
       0U,
       3U,
       std::nullopt,
       0U},
      {4U,
       2U,
       level(53),
       ExactDirectMorseForestNodeKind::reduced_birth,
       3U,
       0U,
       std::nullopt,
       1U},
  };
  forest.batches = {
      batch(0U, 1U, 0, 0U, 3U, 0U, 0U, 0U, 0U, 0U, 0U, 3U, 3U),
      batch(1U, 1U, 1, 3U, 0U, 0U, 1U, 0U, 1U, 3U, 3U, 1U, 1U),
      batch(2U, 2U, 52, 3U, 2U, 1U, 0U, 1U, 0U, 0U, 0U, 2U, 0U),
      batch(3U, 2U, 53, 5U, 0U, 1U, 1U, 1U, 1U, 2U, 0U, 1U, 1U),
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
  forest.logical_output_entry_count = 32U;
  populate_locator_stamps(forest);
  set_success_flags(forest);
  forest.decision = ExactDirectMorseForestDecision::
      complete_conditional_exact_direct_morse_forest;
  forest.scope = ExactDirectMorseForestScope::
      all_orders_direct_minimum_carriers_strict_arms_recursive_positive_terminals_and_atomic_full_component_saddle_quotients_with_reduced_qr_only;
  return forest;
}

[[nodiscard]] CanonicalPointCloud cloud_fixture() {
  const std::array<CertifiedPoint3, 3U> points{
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(10.0, 0.0, 0.0),
  };
  return CanonicalPointCloud::rejecting_duplicates(points);
}

void check_plan_budget_rejection(
    const ExactDirectMorseForestJournalResult& forest,
    const ExactDirectMorseVerticalTargetFacetPlanBudget& budget,
    const std::string& context) {
  const auto rejected = build_exact_direct_morse_vertical_target_facet_plan(
      forest, 1U, budget);
  check(
      rejected.decision ==
              ExactDirectMorseVerticalTargetFacetPlanDecision::
                  no_plan_budget_exhausted &&
          rejected.certified_atomic_failure() &&
          rejected.representatives.empty() &&
          rejected.target_facet_indices.empty() &&
          rejected.canonical_distinct_target_facet_keys.empty(),
      context);
}

void test_plan_and_plan_budgets(
    const ExactDirectMorseForestJournalResult& forest,
    const ExactDirectMorseVerticalTargetFacetPlanResult& plan) {
  check(
      plan.certified_group_local_target_facet_plan() &&
          plan.source_atomic_group_index == 1U &&
          plan.source_batch_index == 3U && plan.source_order == 2U &&
          plan.target_order == 1U &&
          plan.source_batch_squared_level == level(53),
      "the K=2 group produces a certified adjacent-order plan");
  check(
      plan.representatives.size() == 2U &&
          plan.target_facet_indices.size() == 4U &&
          plan.canonical_distinct_target_facet_keys.size() == 3U &&
          plan.required_representative_binding_count == 2U &&
          plan.required_projected_target_facet_reference_count == 4U &&
          plan.required_distinct_target_facet_count == 3U,
      "two Q representatives project I=4 incidences onto D=3 facets");
  check(
      plan.canonical_distinct_target_facet_keys[0U] == key({0U}) &&
          plan.canonical_distinct_target_facet_keys[1U] == key({1U}) &&
          plan.canonical_distinct_target_facet_keys[2U] == key({2U}) &&
          plan.representatives[0U].source_strict_arm_key == key({0U, 1U}) &&
          plan.representatives[1U].source_strict_arm_key == key({1U, 2U}) &&
          plan.target_facet_indices ==
              std::vector<std::size_t>{1U, 0U, 2U, 1U} &&
          plan.representative_indices_in_binding_order ==
              std::vector<std::size_t>{1U, 0U},
      "the shared {1} facet remains one D entry referenced twice, while output order follows bindings");
  check(
      std::count(
          plan.target_facet_indices.begin(),
          plan.target_facet_indices.end(),
          1U) == 2,
      "the shared-facet incidence is not mistaken for a summary bijection");

  auto forged_mapping = plan;
  forged_mapping.target_facet_indices[3U] = 0U;
  auto forged_required_size = plan;
  --forged_required_size.required_projected_target_facet_reference_count;
  auto forged_binding_order = plan;
  std::swap(
      forged_binding_order.representative_indices_in_binding_order[0U],
      forged_binding_order.representative_indices_in_binding_order[1U]);
  auto forged_stored_budget = plan;
  forged_stored_budget.requested_budget.maximum_distinct_target_facet_count =
      2U;
  auto forged_superfluous_target = plan;
  forged_superfluous_target.point_count = 4U;
  forged_superfluous_target.canonical_distinct_target_facet_keys.push_back(
      key({3U}));
  ++forged_superfluous_target.required_distinct_target_facet_count;
  ++forged_superfluous_target.required_retained_key_point_reference_count;
  forged_superfluous_target.required_logical_output_entry_count += 2U;
  check(
      !forged_mapping.certified_group_local_target_facet_plan() &&
          !forged_required_size.certified_group_local_target_facet_plan() &&
          !forged_binding_order.certified_group_local_target_facet_plan() &&
          !forged_stored_budget.certified_group_local_target_facet_plan() &&
          !forged_superfluous_target
               .certified_group_local_target_facet_plan(),
      "plan certification rejects a false shared-facet map, forged sizes, order, stored budget or unreferenced D key");

  auto budget = plan_budget();
  budget.maximum_group_saddle_scan_count =
      plan.counters.group_saddle_scan_count - 1U;
  check_plan_budget_rejection(forest, budget, "one-less saddle plan budget fails closed");
  budget = plan_budget();
  budget.maximum_group_arm_binding_scan_count =
      plan.counters.group_arm_binding_scan_count - 1U;
  check_plan_budget_rejection(forest, budget, "one-less binding plan budget fails closed");
  budget = plan_budget();
  budget.maximum_binding_sort_scratch_count =
      plan.counters.group_arm_binding_scan_count - 1U;
  check_plan_budget_rejection(forest, budget, "one-less binding scratch plan budget fails closed");
  budget = plan_budget();
  budget.maximum_representative_binding_count =
      plan.required_representative_binding_count - 1U;
  check_plan_budget_rejection(forest, budget, "one-less representative plan budget fails closed");
  budget = plan_budget();
  budget.maximum_projected_target_facet_reference_count =
      plan.required_projected_target_facet_reference_count - 1U;
  check_plan_budget_rejection(forest, budget, "one-less incidence plan budget fails closed");
  budget = plan_budget();
  budget.maximum_distinct_target_facet_count =
      plan.required_distinct_target_facet_count - 1U;
  check_plan_budget_rejection(forest, budget, "one-less distinct-facet plan budget fails closed");
  budget = plan_budget();
  budget.maximum_sort_comparison_count =
      plan.counters.sort_comparison_count - 1U;
  check_plan_budget_rejection(forest, budget, "one-less comparison plan budget fails closed");
  budget = plan_budget();
  budget.maximum_target_facet_lookup_comparison_count =
      plan.counters.target_facet_lookup_comparison_count - 1U;
  check_plan_budget_rejection(forest, budget, "one-less lookup plan budget fails closed");
  budget = plan_budget();
  budget.maximum_retained_key_point_reference_count =
      plan.required_retained_key_point_reference_count - 1U;
  check_plan_budget_rejection(forest, budget, "one-less retained-key plan budget fails closed");
  budget = plan_budget();
  budget.maximum_logical_output_entry_count =
      plan.required_logical_output_entry_count - 1U;
  check_plan_budget_rejection(forest, budget, "one-less logical-output plan budget fails closed");
}

void check_proposal_budget_rejection(
    const ExactDirectMorseForestCarrierCutReplayView& view,
    const ExactDirectMorseForestJournalResult& forest,
    const ExactDirectMorseVerticalTargetFacetPlanResult& plan,
    const ExactDirectMorseForestCarrierCutClosureAdapterResult& closure,
    const ExactDirectMorseVerticalTargetProposalAdapterBudget& budget,
    const std::string& context) {
  const auto rejected = view.build_vertical_target_proposals_from_group_plan(
      forest, plan, closure, budget);
  check(
      rejected.decision ==
              ExactDirectMorseVerticalTargetProposalAdapterDecision::
                  no_adapter_budget_exhausted &&
          rejected.certified_atomic_rejection() &&
          rejected.proposals.empty(),
      context);
}

void test_proposals_at_exact_cut(
    const ExactDirectMorseForestJournalResult& forest,
    const ExactDirectMorseVerticalTargetFacetPlanResult& plan) {
  auto initialized =
      build_exact_direct_morse_forest_carrier_cut_replay_session(
          forest, 1U, session_budget());
  check(
      initialized.certified_ready_session(),
      "the order-one carrier-cut replay session initializes");
  if (!initialized.session) {
    return;
  }

  const CanonicalPointCloud cloud = cloud_fixture();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparseFacetWitness query_witness{
      authority_id, invocation_replay_token};
  bool visited = false;
  const auto advanced = initialized.session->advance_to_closed_cut(
      level(53),
      [&](const ExactDirectMorseForestCarrierCutReplayView& view) {
        visited = true;
        const auto adapter_entry_stamp = view.locator_snapshot_stamp();
        const auto closure =
            view.build_closure_summary_from_canonical_distinct_keys(
                index,
                cloud,
                plan.canonical_distinct_target_facet_keys,
                query_witness,
                closure_adapter_budget());
        check(
            closure.certified_compact_closure_summary() &&
                closure.target_order == 1U &&
                closure.closed_squared_level == level(53) &&
                closure.terminal_summaries.size() == 3U,
            "10.5c closes exactly the three D singleton targets at the source cut");

        std::array<std::uint64_t, 3U> binding_tokens{};
        bool all_singletons_resolve_to_order_one_root = true;
        for (std::size_t index = 0U;
             index < closure.terminal_summaries.size();
             ++index) {
          const auto& summary = closure.terminal_summaries[index];
          if (summary.disposition !=
                  ExactDirectMorseForestCarrierCutClosureTerminalDisposition::
                      positive_resolved_reduced_root ||
              !summary.binding_witness.has_value() ||
              !summary.carrier_cut_entry.has_value() ||
              summary.carrier_cut_entry->reduced_root_node_id !=
                  std::optional<ExactDirectMorseForestNodeId>{3U}) {
            all_singletons_resolve_to_order_one_root = false;
            continue;
          }
          binding_tokens[index] = summary.binding_witness->replay_token;
        }
        check(
            all_singletons_resolve_to_order_one_root,
            "all singleton terminals resolve to the closed order-one root");
        check(
            binding_tokens[0U] != 0U && binding_tokens[1U] != 0U &&
                binding_tokens[2U] != 0U &&
                binding_tokens[0U] != binding_tokens[1U] &&
                binding_tokens[0U] != binding_tokens[2U] &&
                binding_tokens[1U] != binding_tokens[2U],
            "the three resolved closure bindings retain distinct source tokens");

        auto forged_live_entry_closure = closure;
        std::optional<ExactDirectMorseForestCarrierCutEntry>
            alternate_live_entry;
        for (std::size_t entry_index = 0U;
             entry_index < view.target_carrier_count();
             ++entry_index) {
          const auto candidate = view.entry_at(entry_index);
          if (forged_live_entry_closure.terminal_summaries[0U]
                      .canonical_component_handle !=
                  std::optional<ExactDirectSparseComponentHandle>{
                      candidate.component_handle} &&
              candidate.disposition ==
                  ExactDirectMorseForestCarrierCutDisposition::
                      resolved_reduced_root) {
            alternate_live_entry = candidate;
            break;
          }
        }
        check(
            alternate_live_entry.has_value(),
            "the hostile fixture has another live resolved carrier entry");
        if (alternate_live_entry.has_value()) {
          auto& forged_summary =
              forged_live_entry_closure.terminal_summaries[0U];
          forged_summary.canonical_component_handle =
              alternate_live_entry->component_handle;
          forged_summary.carrier_cut_entry = alternate_live_entry;
          check(
              forged_live_entry_closure
                  .certified_compact_closure_summary(),
              "the compact closure predicate alone permits a different coherent live entry payload");
          const auto rejected_live_entry_substitution =
              view.build_vertical_target_proposals_from_group_plan(
                  forest,
                  plan,
                  forged_live_entry_closure,
                  proposal_budget());
          check(
              rejected_live_entry_substitution.certified_atomic_rejection() &&
                  rejected_live_entry_substitution.decision ==
                      ExactDirectMorseVerticalTargetProposalAdapterDecision::
                          no_adapter_target_seed_rejected,
              "a fresh terminal-key plus binding probe rejects substitution by another live carrier entry");
        }

        const auto proposals =
            view.build_vertical_target_proposals_from_group_plan(
                forest, plan, closure, proposal_budget());
        const bool proposals_certified =
            proposals.certified_group_local_vertical_target_proposals();
        check(
            proposals_certified &&
                proposals.decision ==
                    ExactDirectMorseVerticalTargetProposalAdapterDecision::
                        complete_all_resolved_group_local_vertical_target_proposals &&
                proposals.source_order == 2U &&
                proposals.target_order == 1U &&
                proposals.source_batch_squared_level == level(53) &&
                proposals.input_representative_count == 2U &&
                proposals.input_distinct_target_facet_count == 3U &&
                proposals.input_projected_target_facet_reference_count ==
                    4U,
            "the exact-cut adapter certifies one compact proposal per Q representative");
        if (!proposals_certified || proposals.proposals.size() != 2U) {
          std::cerr
              << "proposal adapter diagnostic: decision="
              << static_cast<unsigned int>(proposals.decision)
              << ", payload=" << proposals.proposals.size()
              << ", saddle_revalidation="
              << proposals.counters.source_saddle_revalidation_count
              << ", binding_revalidation="
              << proposals.counters.source_binding_revalidation_count
              << ", key_lookup="
              << proposals.counters.source_key_lookup_comparison_count
              << ", summary_scan="
              << proposals.counters.closure_summary_scan_count
              << ", incidence_aggregation="
              << proposals.counters.projected_target_facet_aggregation_count
              << '\n';
          return;
        }
        check(
            proposals.proposals.size() == 2U &&
                proposals.proposals[0U]
                        .representative_arm_root_binding_index ==
                    3U &&
                proposals.proposals[1U]
                        .representative_arm_root_binding_index ==
                    4U &&
                proposals.proposals[0U].target_seed_node_id ==
                    std::optional<ExactDirectMorseForestNodeId>{3U} &&
                proposals.proposals[1U].target_seed_node_id ==
                    std::optional<ExactDirectMorseForestNodeId>{3U} &&
                proposals.proposals[0U].disposition ==
                    ExactDirectMorseVerticalProposalDisposition::
                        resolved_target_seed &&
                proposals.proposals[1U].disposition ==
                    ExactDirectMorseVerticalProposalDisposition::
                        resolved_target_seed &&
                proposals.proposals[0U].replay_token ==
                    invocation_replay_token &&
                proposals.proposals[1U].replay_token ==
                    invocation_replay_token,
            "resolved proposals use the one invocation token despite distinct closure binding tokens");
        check(
            proposals.counters.projected_target_facet_aggregation_count ==
                    4U &&
                proposals.counters.positive_terminal_probe_count == 4U &&
                proposals.counters
                        .positive_terminal_probe_full_key_comparison_count >=
                    4U &&
                proposals.counters.closure_summary_scan_count == 3U &&
                proposals.counters.resolved_proposal_count == 2U &&
                proposals.counters.unresolved_proposal_count == 0U &&
                proposals.required_carrier_entry_revalidation_count == 4U &&
                proposals.audit.closure_sources_biject_canonical_target_facets &&
                proposals.audit
                    .every_positive_terminal_key_binding_reprobed &&
                proposals.audit.every_projected_target_facet_aggregated &&
                !proposals.audit.plan_or_summary_payload_retained &&
                !proposals.audit.external_vertical_authority_replayed &&
                !proposals.audit.vertical_maps_complete &&
                !proposals.audit.public_status_claimed,
            "the adapter audits D=3 summary scans and I=4 group-local aggregations without authority promotion");

        const auto vertical = build_exact_direct_morse_vertical_journal(
            forest,
            proposals.proposals,
            vertical_budget(),
            ExactDirectMorseVerticalConfig{authority_id});
        check(
            vertical.certified_conditional_vertical_candidate() &&
                vertical.decision ==
                    ExactDirectMorseVerticalDecision::
                        complete_conditional_total_relative_vertical_journal &&
                vertical.adjacent_families.size() == 1U &&
                vertical.label_resolutions.size() == 2U &&
                vertical.group_checks.size() == 1U &&
                vertical.checkpoints.size() == 1U &&
                vertical.counters.resolved_label_count == 2U &&
                vertical.counters.missing_label_count == 0U &&
                vertical.counters.unresolved_label_count == 0U &&
                !vertical.external_target_authority_replayed &&
                !vertical.vertical_maps_complete,
            "the compact proposals are consumed directly by the conditional vertical journal");

        auto forged_plan = plan;
        forged_plan.target_facet_indices[3U] = 0U;
        const auto rejected_forged_plan =
            view.build_vertical_target_proposals_from_group_plan(
                forest, forged_plan, closure, proposal_budget());
        check(
            rejected_forged_plan.certified_atomic_rejection() &&
                rejected_forged_plan.decision ==
                    ExactDirectMorseVerticalTargetProposalAdapterDecision::
                        no_adapter_plan_rejected,
            "the adapter rejects a false I-to-D plan before proposal publication");

        auto falsified_budget = proposals;
        falsified_budget.requested_budget.maximum_proposal_count = 1U;
        auto falsified_common_token = proposals;
        falsified_common_token.proposals[0U].replay_token = binding_tokens[0U];
        auto falsified_seed = proposals;
        falsified_seed.proposals[0U].target_seed_node_id.reset();
        auto falsified_required_count = proposals;
        --falsified_required_count.required_logical_output_entry_count;
        auto falsified_incidence_count = proposals;
        --falsified_incidence_count.input_projected_target_facet_reference_count;
        auto falsified_probe_count = proposals;
        --falsified_probe_count.counters.positive_terminal_probe_count;
        auto falsified_reprobe_audit = proposals;
        falsified_reprobe_audit.audit
            .every_positive_terminal_key_binding_reprobed = false;
        check(
            !falsified_budget
                 .certified_group_local_vertical_target_proposals() &&
                !falsified_common_token
                     .certified_group_local_vertical_target_proposals() &&
                !falsified_seed
                     .certified_group_local_vertical_target_proposals() &&
                !falsified_required_count
                     .certified_group_local_vertical_target_proposals() &&
                !falsified_incidence_count
                     .certified_group_local_vertical_target_proposals() &&
                !falsified_probe_count
                     .certified_group_local_vertical_target_proposals() &&
                !falsified_reprobe_audit
                     .certified_group_local_vertical_target_proposals(),
            "proposal certification rejects narrowed storage, mixed tokens, missing seeds, forged Q/I sizes or fresh-probe evidence");

        auto budget = proposal_budget();
        budget.maximum_source_saddle_revalidation_count =
            proposals.counters.source_saddle_revalidation_count - 1U;
        check_proposal_budget_rejection(
            view, forest, plan, closure, budget,
            "one-less source-saddle adapter budget fails closed");
        budget = proposal_budget();
        budget.maximum_source_binding_revalidation_count =
            proposals.counters.source_binding_revalidation_count - 1U;
        check_proposal_budget_rejection(
            view, forest, plan, closure, budget,
            "one-less source-binding adapter budget fails closed");
        budget = proposal_budget();
        budget.maximum_source_key_lookup_comparison_count =
            proposals.counters.source_key_lookup_comparison_count - 1U;
        check_proposal_budget_rejection(
            view, forest, plan, closure, budget,
            "one-less source-key lookup adapter budget fails closed");
        budget = proposal_budget();
        budget.maximum_closure_summary_scan_count =
            proposals.counters.closure_summary_scan_count - 1U;
        check_proposal_budget_rejection(
            view, forest, plan, closure, budget,
            "one-less closure-summary adapter budget fails closed");
        budget = proposal_budget();
        budget.maximum_positive_terminal_probe_count =
            proposals.counters.positive_terminal_probe_count - 1U;
        check_proposal_budget_rejection(
            view, forest, plan, closure, budget,
            "one-less positive-terminal probe budget fails closed");
        if (proposals.counters.positive_terminal_probe_parent_hop_count !=
            0U) {
          budget = proposal_budget();
          budget.maximum_positive_terminal_probe_parent_hop_count =
              proposals.counters.positive_terminal_probe_parent_hop_count -
              1U;
          check_proposal_budget_rejection(
              view, forest, plan, closure, budget,
              "one-less aggregate positive-terminal parent-hop budget fails closed");
        }
        budget = proposal_budget();
        budget.maximum_carrier_entry_revalidation_count =
            proposals.counters.carrier_entry_revalidation_count - 1U;
        check_proposal_budget_rejection(
            view, forest, plan, closure, budget,
            "one-less carrier-entry adapter budget fails closed");
        budget = proposal_budget();
        budget.maximum_target_node_lookup_count =
            proposals.counters.target_node_lookup_count - 1U;
        check_proposal_budget_rejection(
            view, forest, plan, closure, budget,
            "one-less target-node adapter budget fails closed");
        budget = proposal_budget();
        budget.maximum_exact_level_comparison_count =
            proposals.counters.exact_level_comparison_count - 1U;
        check_proposal_budget_rejection(
            view, forest, plan, closure, budget,
            "one-less exact-comparison adapter budget fails closed");
        budget = proposal_budget();
        budget.maximum_single_exact_level_integer_bit_count =
            proposals.counters.maximum_observed_exact_level_integer_bit_count -
            1U;
        check_proposal_budget_rejection(
            view, forest, plan, closure, budget,
            "one-less exact-integer adapter budget fails closed");
        budget = proposal_budget();
        budget.maximum_proposal_count = proposals.proposals.size() - 1U;
        check_proposal_budget_rejection(
            view, forest, plan, closure, budget,
            "one-less proposal adapter budget fails closed");
        budget = proposal_budget();
        budget.maximum_logical_output_entry_count =
            proposals.required_logical_output_entry_count - 1U;
        check_proposal_budget_rejection(
            view, forest, plan, closure, budget,
            "one-less logical-output adapter budget fails closed");
        check(
            view.locator_snapshot_stamp() == adapter_entry_stamp,
            "proposal planning and every hostile rejection leave the live carrier cut unchanged");
      });
  check(
      visited && advanced.certified_forest_relative_closed_cut(),
      "the proposal campaign returns through one certified synchronous exact-cut receipt");
}

}  // namespace

int main() {
  const auto forest = forest_fixture();
  check(
      forest.certified_conditional_h0_candidate(),
      "the three-point proposal fixture is a certified source forest");
  const auto plan = build_exact_direct_morse_vertical_target_facet_plan(
      forest, 1U, plan_budget());
  test_plan_and_plan_budgets(forest, plan);
  if (plan.certified_group_local_target_facet_plan()) {
    test_proposals_at_exact_cut(forest, plan);
  }

  if (failures != 0) {
    std::cerr << failures
              << " direct Morse vertical target proposal adapter check(s) failed\n";
    return 1;
  }
  std::cout << "direct Morse vertical target proposal adapter checks passed\n";
  return 0;
}
