#include "morsehgp3d/hierarchy/direct_morse_forest_carrier_cut_index.hpp"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::PointId;

constexpr std::uint64_t authority_id = UINT64_C(0xC017);
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

[[nodiscard]] ExactDirectSparsePositiveFacetLocatorSnapshotStamp stamp(
    std::size_t committed_batch_count) {
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp result;
  result.schema_version =
      direct_sparse_positive_facet_locator_schema_version;
  result.external_authority_id = authority_id;
  result.committed_batch_count = committed_batch_count;
  return result;
}

[[nodiscard]] ExactDirectMorseForestBirthRecord birth(
    std::size_t birth_index,
    std::size_t source_batch_index,
    std::size_t order,
    ExactDirectSparseFacetKey facet_key) {
  return {
      birth_index,
      birth_index,
      source_batch_index,
      order,
      std::move(facet_key),
      birth_index,
      std::nullopt,
      {authority_id, static_cast<std::uint64_t>(birth_index + 1U)}};
}

[[nodiscard]] ExactDirectMorseForestNode node(
    ExactDirectMorseForestNodeId node_id,
    std::size_t order,
    std::int64_t squared_level,
    ExactDirectMorseForestNodeKind kind,
    std::size_t child_offset,
    std::size_t child_count,
    std::size_t group_index) {
  return {
      node_id,
      order,
      level(squared_level),
      kind,
      child_offset,
      child_count,
      std::nullopt,
      group_index};
}

[[nodiscard]] ExactDirectMorseForestArmRootBinding binding(
    std::size_t binding_index,
    std::size_t family_index,
    ExactDirectSparseFacetKey strict_arm_key,
    ExactDirectSparseComponentHandle component_handle,
    std::optional<ExactDirectMorseForestNodeId> prior_root) {
  return {
      binding_index,
      binding_index,
      family_index,
      std::move(strict_arm_key),
      component_handle,
      prior_root};
}

[[nodiscard]] ExactDirectMorseForestSaddleRecord saddle(
    std::size_t saddle_index,
    std::size_t source_batch_index,
    std::size_t binding_offset,
    std::size_t binding_count,
    std::size_t distinct_carrier_count,
    std::size_t distinct_latent_count,
    std::size_t distinct_root_count,
    std::size_t group_index) {
  return {
      saddle_index,
      saddle_index,
      saddle_index,
      source_batch_index,
      binding_offset,
      binding_count,
      distinct_carrier_count,
      distinct_latent_count,
      distinct_root_count,
      group_index};
}

[[nodiscard]] ExactDirectMorseForestAtomicGroup group(
    std::size_t group_index,
    std::size_t batch_index,
    std::size_t saddle_offset,
    std::size_t frozen_carrier_count,
    std::size_t latent_carrier_count,
    std::size_t prior_root_count,
    std::size_t child_offset,
    std::size_t child_count,
    std::optional<ExactDirectMorseForestNodeId> created_node_id,
    ExactDirectMorseForestNodeId resulting_root_node_id,
    ExactDirectMorseForestAtomicGroupKind kind) {
  return {
      group_index,
      batch_index,
      saddle_offset,
      1U,
      frozen_carrier_count,
      latent_carrier_count,
      prior_root_count,
      child_offset,
      child_count,
      created_node_id,
      resulting_root_node_id,
      kind};
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
  return {
      batch_index,
      batch_index,
      order,
      level(squared_level),
      birth_offset,
      birth_count,
      saddle_offset,
      saddle_count,
      group_offset,
      group_count,
      strict_carrier_count,
      strict_root_count,
      closed_carrier_count,
      closed_root_count,
      stamp(batch_index),
      stamp(batch_index + 1U),
      true,
      true,
      true};
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

[[nodiscard]] ExactDirectMorseForestJournalResult forest_fixture() {
  ExactDirectMorseForestJournalResult forest;
  forest.source_higher_canonical_cloud_digest =
      morsehgp3d::contract::CanonicalId::from_lower_hex(
          std::string(64U, '1'));
  forest.requested_budget = forest_budget();
  forest.config.locator_config.external_authority_id = authority_id;
  forest.point_count = 4U;
  forest.effective_maximum_order = 3U;
  forest.implicit_order_one_prefix_count = 4U;

  forest.birth_records = {
      birth(4U, 2U, 2U, key({0U, 1U})),
      birth(5U, 2U, 2U, key({2U, 3U})),
      birth(6U, 4U, 2U, key({0U, 3U})),
      birth(7U, 7U, 3U, key({0U, 1U, 2U})),
  };

  forest.child_node_ids = {0U, 1U, 2U, 3U, 5U, 6U};
  forest.nodes = {
      node(
          4U,
          1U,
          1,
          ExactDirectMorseForestNodeKind::multifusion,
          0U,
          4U,
          0U),
      node(
          5U,
          2U,
          2,
          ExactDirectMorseForestNodeKind::reduced_birth,
          4U,
          0U,
          1U),
      node(
          6U,
          2U,
          4,
          ExactDirectMorseForestNodeKind::reduced_birth,
          4U,
          0U,
          3U),
      node(
          7U,
          2U,
          5,
          ExactDirectMorseForestNodeKind::multifusion,
          4U,
          2U,
          4U),
      node(
          8U,
          3U,
          2,
          ExactDirectMorseForestNodeKind::reduced_birth,
          6U,
          0U,
          5U),
  };

  forest.arm_root_bindings = {
      binding(0U, 0U, key({0U}), 0U, 0U),
      binding(1U, 0U, key({1U}), 1U, 1U),
      binding(2U, 0U, key({2U}), 2U, 2U),
      binding(3U, 0U, key({3U}), 3U, 3U),
      binding(4U, 1U, key({0U, 1U}), 4U, std::nullopt),
      binding(5U, 1U, key({2U, 3U}), 5U, std::nullopt),
      binding(6U, 2U, key({0U, 2U}), 4U, 5U),
      binding(7U, 3U, key({0U, 3U}), 6U, std::nullopt),
      binding(8U, 4U, key({0U, 2U}), 4U, 5U),
      binding(9U, 4U, key({1U, 3U}), 6U, 6U),
      binding(10U, 5U, key({0U, 1U, 2U}), 7U, std::nullopt),
  };

  forest.saddle_records = {
      saddle(0U, 1U, 0U, 4U, 4U, 0U, 4U, 0U),
      saddle(1U, 3U, 4U, 2U, 2U, 2U, 0U, 1U),
      saddle(2U, 4U, 6U, 1U, 1U, 0U, 1U, 2U),
      saddle(3U, 5U, 7U, 1U, 1U, 1U, 0U, 3U),
      saddle(4U, 6U, 8U, 2U, 2U, 0U, 2U, 4U),
      saddle(5U, 8U, 10U, 1U, 1U, 1U, 0U, 5U),
  };

  forest.atomic_groups = {
      group(
          0U,
          1U,
          0U,
          4U,
          0U,
          4U,
          0U,
          4U,
          4U,
          4U,
          ExactDirectMorseForestAtomicGroupKind::multifusion),
      group(
          1U,
          3U,
          1U,
          2U,
          2U,
          0U,
          4U,
          0U,
          5U,
          5U,
          ExactDirectMorseForestAtomicGroupKind::reduced_birth),
      group(
          2U,
          4U,
          2U,
          1U,
          0U,
          1U,
          4U,
          0U,
          std::nullopt,
          5U,
          ExactDirectMorseForestAtomicGroupKind::continuation),
      group(
          3U,
          5U,
          3U,
          1U,
          1U,
          0U,
          4U,
          0U,
          6U,
          6U,
          ExactDirectMorseForestAtomicGroupKind::reduced_birth),
      group(
          4U,
          6U,
          4U,
          2U,
          0U,
          2U,
          4U,
          2U,
          7U,
          7U,
          ExactDirectMorseForestAtomicGroupKind::multifusion),
      group(
          5U,
          8U,
          5U,
          1U,
          1U,
          0U,
          6U,
          0U,
          8U,
          8U,
          ExactDirectMorseForestAtomicGroupKind::reduced_birth),
  };

  forest.batches = {
      batch(0U, 1U, 0, 0U, 4U, 0U, 0U, 0U, 0U, 0U, 0U, 4U, 4U),
      batch(1U, 1U, 1, 4U, 0U, 0U, 1U, 0U, 1U, 4U, 4U, 1U, 1U),
      batch(2U, 2U, 1, 4U, 2U, 1U, 0U, 1U, 0U, 0U, 0U, 2U, 0U),
      batch(3U, 2U, 2, 6U, 0U, 1U, 1U, 1U, 1U, 2U, 0U, 1U, 1U),
      batch(4U, 2U, 3, 6U, 1U, 2U, 1U, 2U, 1U, 1U, 1U, 2U, 1U),
      batch(5U, 2U, 4, 7U, 0U, 3U, 1U, 3U, 1U, 2U, 1U, 2U, 2U),
      batch(6U, 2U, 5, 7U, 0U, 4U, 1U, 4U, 1U, 2U, 2U, 1U, 1U),
      batch(7U, 3U, 1, 7U, 1U, 5U, 0U, 5U, 0U, 0U, 0U, 1U, 0U),
      batch(8U, 3U, 2, 8U, 0U, 5U, 1U, 5U, 1U, 1U, 0U, 1U, 1U),
  };

  forest.final_roots = {
      {0U, 1U, 0U, 4U},
      {1U, 2U, 4U, 7U},
      {2U, 3U, 7U, 8U},
  };

  forest.counters.birth_record_count = 8U;
  forest.counters.latent_higher_order_birth_count = 4U;
  forest.counters.order_one_birth_node_count = 4U;
  forest.counters.arm_root_binding_count = 11U;
  forest.counters.saddle_record_count = 6U;
  forest.counters.atomic_group_count = 6U;
  forest.counters.reduced_birth_group_count = 3U;
  forest.counters.continuation_group_count = 1U;
  forest.counters.multifusion_group_count = 2U;
  forest.counters.child_reference_count = 6U;
  forest.counters.batch_record_count = 9U;
  forest.counters.node_count = 9U;
  forest.counters.final_root_count = 3U;
  forest.counters.locator_union_count = 5U;
  forest.counters.quotient_call_count = 6U;
  forest.counters.distinct_strict_arm_count = 11U;
  forest.counters.maximum_batch_arm_count = 4U;
  forest.counters.maximum_batch_carrier_arity = 4U;
  forest.counters.maximum_batch_merge_arity = 4U;
  forest.logical_output_entry_count = 64U;
  forest.final_locator_stamp = stamp(forest.batches.size());
  set_success_flags(forest);
  forest.decision = ExactDirectMorseForestDecision::
      complete_conditional_exact_direct_morse_forest;
  forest.scope = ExactDirectMorseForestScope::
      all_orders_direct_minimum_carriers_strict_arms_recursive_positive_terminals_and_atomic_full_component_saddle_quotients_with_reduced_qr_only;
  return forest;
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
  budget.maximum_group_carrier_scratch_count = 8U;
  budget.maximum_group_prior_root_scratch_count = 8U;
  budget.maximum_parent_hop_count = 1024U;
  budget.maximum_exact_level_comparison_count = 1024U;
  budget.maximum_single_exact_level_integer_bit_count = 64U;
  budget.maximum_logical_output_entry_count = 128U;
  return budget;
}

[[nodiscard]] const ExactDirectMorseForestCarrierCutEntry& entry(
    const ExactDirectMorseForestCarrierCutIndexResult& result,
    ExactDirectSparseComponentHandle handle) {
  const auto* found = result.find_entry(handle);
  if (found == nullptr) {
    static const ExactDirectMorseForestCarrierCutEntry missing{};
    check(false, "the requested carrier is present in the cut index");
    return missing;
  }
  return *found;
}

void check_disposition(
    const ExactDirectMorseForestCarrierCutIndexResult& result,
    ExactDirectSparseComponentHandle handle,
    ExactDirectMorseForestCarrierCutDisposition disposition,
    std::optional<ExactDirectMorseForestNodeId> root,
    const std::string& path) {
  const auto& observed = entry(result, handle);
  check(
      observed.disposition == disposition &&
          observed.reduced_root_node_id == root,
      path);
}

}  // namespace

int main() {
  const auto forest = forest_fixture();
  const auto budget = index_budget();
  check(
      forest.certified_conditional_h0_candidate(),
      "the compact fixture is a certified self-contained forest outcome");

  const auto before_birth =
      build_exact_direct_morse_forest_carrier_cut_index(
          forest, 2U, level(0), budget);
  check(
      before_birth.certified_forest_relative_closed_cut_index(),
      "the before-birth closed cut is certified forest-relative");
  check_disposition(
      before_birth,
      4U,
      ExactDirectMorseForestCarrierCutDisposition::inactive_at_closed_cut,
      std::nullopt,
      "a future order-two carrier is inactive before its birth level");
  check_disposition(
      before_birth,
      6U,
      ExactDirectMorseForestCarrierCutDisposition::inactive_at_closed_cut,
      std::nullopt,
      "all later carriers are indexed explicitly as inactive");

  const auto at_birth = build_exact_direct_morse_forest_carrier_cut_index(
      forest, 2U, level(1), budget);
  check(
      at_birth.certified_forest_relative_closed_cut_index(),
      "the birth-level closed cut is certified");
  check_disposition(
      at_birth,
      4U,
      ExactDirectMorseForestCarrierCutDisposition::
          active_latent_without_reduced_root,
      std::nullopt,
      "closed-cut equality includes a higher-order latent birth");
  check_disposition(
      at_birth,
      5U,
      ExactDirectMorseForestCarrierCutDisposition::
          active_latent_without_reduced_root,
      std::nullopt,
      "both same-level births are active and latent");
  check_disposition(
      at_birth,
      6U,
      ExactDirectMorseForestCarrierCutDisposition::inactive_at_closed_cut,
      std::nullopt,
      "a later birth stays inactive at the earlier closed cut");

  const auto at_reduced_birth =
      build_exact_direct_morse_forest_carrier_cut_index(
          forest, 2U, level(2), budget);
  check_disposition(
      at_reduced_birth,
      4U,
      ExactDirectMorseForestCarrierCutDisposition::resolved_reduced_root,
      5U,
      "a q_R=0 group creates the reduced root at its closed level");
  check_disposition(
      at_reduced_birth,
      5U,
      ExactDirectMorseForestCarrierCutDisposition::resolved_reduced_root,
      5U,
      "all carriers unioned by the reduced birth resolve to one root");

  const auto after_continuation =
      build_exact_direct_morse_forest_carrier_cut_index(
          forest, 2U, level(3), budget);
  check_disposition(
      after_continuation,
      4U,
      ExactDirectMorseForestCarrierCutDisposition::resolved_reduced_root,
      5U,
      "a q_R=1 continuation preserves the pre-existing reduced root");
  check_disposition(
      after_continuation,
      6U,
      ExactDirectMorseForestCarrierCutDisposition::
          active_latent_without_reduced_root,
      std::nullopt,
      "groups are replayed before a same-batch current-level birth");

  const auto before_multifusion =
      build_exact_direct_morse_forest_carrier_cut_index(
          forest, 2U, level(4), budget);
  check_disposition(
      before_multifusion,
      6U,
      ExactDirectMorseForestCarrierCutDisposition::resolved_reduced_root,
      6U,
      "the second q_R=0 group produces an independent reduced root");

  const auto at_multifusion =
      build_exact_direct_morse_forest_carrier_cut_index(
          forest, 2U, level(5), budget);
  check(
      at_multifusion.certified_forest_relative_closed_cut_index() &&
          at_multifusion.entries.size() == 3U &&
          at_multifusion.find_entry(99U) == nullptr,
      "the multifusion cut publishes one sorted entry per target carrier");
  for (const std::size_t handle : {4U, 5U, 6U}) {
    check_disposition(
        at_multifusion,
        handle,
        ExactDirectMorseForestCarrierCutDisposition::resolved_reduced_root,
        7U,
        "the q_R>=2 multifusion resolves every attached carrier to node 7");
  }
  check(
      !at_multifusion.gamma_cells_or_global_cofaces_materialized &&
          !at_multifusion.higher_order_delaunay_materialized &&
          !at_multifusion.forbidden_global_structure_materialized &&
          !at_multifusion.public_status_claimed &&
          at_multifusion.forest_relative_only,
      "the index remains a forest-relative architecture prerequisite only");

  const auto invalid_order =
      build_exact_direct_morse_forest_carrier_cut_index(
          forest, 0U, level(5), budget);
  check(
      invalid_order.decision ==
              ExactDirectMorseForestCarrierCutIndexDecision::
                  no_index_invalid_target_order &&
          invalid_order.certified_atomic_failure(),
      "an invalid target order fails atomically");

  auto insufficient_budget = budget;
  insufficient_budget.maximum_index_entry_count = 2U;
  const auto exhausted = build_exact_direct_morse_forest_carrier_cut_index(
      forest, 2U, level(5), insufficient_budget);
  check(
      exhausted.decision ==
              ExactDirectMorseForestCarrierCutIndexDecision::
                  no_index_budget_exhausted &&
          exhausted.certified_atomic_failure() && exhausted.entries.empty(),
      "an insufficient output budget clears the complete scientific payload");

  const auto verification =
      verify_exact_direct_morse_forest_carrier_cut_index(
          forest, 2U, level(5), budget, at_multifusion);
  check(
      verification.result_certified &&
          verification.expected_index_freshly_reconstructed &&
          verification.observed_recursively_equal &&
          verification.conditional_on_caller_fresh_source_forest_replay &&
          !verification.external_locator_authority_replayed &&
          !verification.original_geometry_replayed &&
          !verification.global_morse_obligation_replayed,
      "the fresh verifier reconstructs the same bounded forest-relative index");
  auto forged = at_multifusion;
  forged.entries[0U].reduced_root_node_id = 5U;
  check(
      !verify_exact_direct_morse_forest_carrier_cut_index(
           forest, 2U, level(5), budget, forged)
           .result_certified,
      "the fresh verifier rejects a recursively unequal root mapping");

  auto contradictory_forest = forest;
  contradictory_forest.batches[3U].strict_pre_batch_carrier_count = 1U;
  check(
      contradictory_forest.certified_conditional_h0_candidate(),
      "the minimal fixture isolates a semantic pre-count not covered by the source shape predicate");
  const auto contradiction =
      build_exact_direct_morse_forest_carrier_cut_index(
          contradictory_forest, 2U, level(2), budget);
  check(
      contradiction.decision ==
              ExactDirectMorseForestCarrierCutIndexDecision::
                  no_index_replay_contradiction &&
          contradiction.certified_atomic_failure(),
      "fresh exact-cut replay rejects the minimal carrier-count contradiction atomically");

  auto understated_group_forest = forest;
  understated_group_forest.atomic_groups[1U].frozen_carrier_count = 1U;
  understated_group_forest.atomic_groups[1U].latent_carrier_count = 1U;
  check(
      understated_group_forest.certified_conditional_h0_candidate(),
      "the fixture isolates an understated group arity while the trusted scratch cap remains sufficient");
  const auto understated_group =
      build_exact_direct_morse_forest_carrier_cut_index(
          understated_group_forest, 2U, level(2), budget);
  check(
      understated_group.decision ==
              ExactDirectMorseForestCarrierCutIndexDecision::
                  no_index_replay_contradiction &&
          understated_group.certified_atomic_failure() &&
          understated_group.entries.empty(),
      "a second distinct carrier is rejected before push can outgrow the source-declared group arity");

  if (failures != 0) {
    std::cerr << failures << " direct Morse carrier-cut index checks failed\n";
    return 1;
  }
  std::cout << "direct Morse forest carrier-cut index checks passed\n";
  return 0;
}
