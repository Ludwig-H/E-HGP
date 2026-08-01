#include "morsehgp3d/hierarchy/direct_morse_forest_carrier_cut_replay_session.hpp"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <utility>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::PointId;

constexpr std::uint64_t authority_id = UINT64_C(0xC0175E5510);
int failures = 0;

static_assert(
    !std::is_copy_constructible_v<
        ExactDirectMorseForestCarrierCutReplaySession>);
static_assert(
    !std::is_copy_assignable_v<
        ExactDirectMorseForestCarrierCutReplaySession>);
static_assert(
    !std::is_move_constructible_v<
        ExactDirectMorseForestCarrierCutReplaySession>);
static_assert(
    !std::is_move_assignable_v<
        ExactDirectMorseForestCarrierCutReplaySession>);
static_assert(
    !std::is_copy_constructible_v<
        ExactDirectMorseForestCarrierCutReplayView>);
static_assert(
    !std::is_move_constructible_v<
        ExactDirectMorseForestCarrierCutReplayView>);

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

[[nodiscard]] ExactDirectMorseForestBirthRecord birth(
    std::size_t birth_index,
    std::size_t batch_index,
    ExactDirectSparseFacetKey facet_key) {
  return {
      birth_index,
      birth_index,
      batch_index,
      2U,
      std::move(facet_key),
      birth_index,
      std::nullopt,
      {authority_id,
       static_cast<std::uint64_t>(3U * birth_index + 1U)}};
}

[[nodiscard]] ExactDirectMorseForestArmRootBinding binding(
    std::size_t binding_index,
    std::size_t family_index,
    ExactDirectSparseFacetKey strict_arm_key,
    ExactDirectSparseComponentHandle carrier,
    std::optional<ExactDirectMorseForestNodeId> prior_root) {
  return {
      binding_index,
      binding_index,
      family_index,
      std::move(strict_arm_key),
      carrier,
      prior_root};
}

[[nodiscard]] ExactDirectMorseForestSaddleRecord saddle(
    std::size_t saddle_index,
    std::size_t family_index,
    std::size_t batch_index,
    std::size_t binding_offset,
    std::size_t binding_count,
    std::size_t carrier_count,
    std::size_t latent_count,
    std::size_t root_count,
    std::size_t group_index) {
  return {
      saddle_index,
      family_index,
      family_index,
      batch_index,
      binding_offset,
      binding_count,
      carrier_count,
      latent_count,
      root_count,
      group_index};
}

[[nodiscard]] ExactDirectMorseForestAtomicGroup group(
    std::size_t group_index,
    std::size_t batch_index,
    std::size_t saddle_offset,
    std::size_t carrier_count,
    std::size_t latent_count,
    std::size_t root_count,
    std::size_t child_offset,
    std::size_t child_count,
    std::optional<ExactDirectMorseForestNodeId> created_node,
    ExactDirectMorseForestNodeId resulting_root,
    ExactDirectMorseForestAtomicGroupKind kind) {
  return {
      group_index,
      batch_index,
      saddle_offset,
      1U,
      carrier_count,
      latent_count,
      root_count,
      child_offset,
      child_count,
      created_node,
      resulting_root,
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

void populate_real_locator_stamps(
    ExactDirectMorseForestJournalResult& forest) {
  auto locator = build_exact_direct_sparse_positive_facet_locator(
      6U, forest.requested_budget.locator_budget, forest.config.locator_config);
  check(locator.certified_positive_locator(), "fixture locator initializes");

  forest.batches[0U].strict_pre_batch_stamp = locator.snapshot_stamp();
  const auto singleton = locator.apply_canonical_singleton_identity_batch(3U);
  check(
      singleton.certified_committed_identity_batch(),
      "fixture singleton prefix commits");
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
      "fixture lower-order multifusion locator batch commits");
  forest.batches[1U].committed_batch_stamp = locator.snapshot_stamp();

  forest.batches[2U].strict_pre_batch_stamp = locator.snapshot_stamp();
  const ExactDirectSparseFacetBinding first_order_two_births[] = {
      {0U, key({0U, 1U}), 3U, {authority_id, 10U}},
      {1U, key({1U, 2U}), 4U, {authority_id, 13U}},
  };
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              first_order_two_births)
          .certified_committed_batch(),
      "fixture order-two latent births commit");
  forest.batches[2U].committed_batch_stamp = locator.snapshot_stamp();

  forest.batches[3U].strict_pre_batch_stamp = locator.snapshot_stamp();
  const ExactDirectSparseComponentUnion order_two_union[] = {
      component_union(0U, 2U, 3U, 4U),
  };
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              order_two_union,
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch(),
      "fixture order-two reduced-birth union commits");
  forest.batches[3U].committed_batch_stamp = locator.snapshot_stamp();

  forest.batches[4U].strict_pre_batch_stamp = locator.snapshot_stamp();
  const ExactDirectSparseFacetBinding same_level_future_birth[] = {
      {0U, key({0U, 2U}), 5U, {authority_id, 16U}},
  };
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              same_level_future_birth)
          .certified_committed_batch(),
      "fixture continuation then current birth batch commits");
  forest.batches[4U].committed_batch_stamp = locator.snapshot_stamp();
  forest.final_locator_stamp = locator.snapshot_stamp();
}

[[nodiscard]] ExactDirectMorseForestJournalResult forest_fixture() {
  ExactDirectMorseForestJournalResult forest;
  forest.requested_budget = forest_budget();
  forest.config.locator_config.external_authority_id = authority_id;
  forest.point_count = 3U;
  forest.effective_maximum_order = 2U;
  forest.implicit_order_one_prefix_count = 3U;

  forest.birth_records = {
      birth(3U, 2U, key({0U, 1U})),
      birth(4U, 2U, key({1U, 2U})),
      birth(5U, 4U, key({0U, 2U})),
  };
  forest.arm_root_bindings = {
      binding(0U, 0U, key({0U}), 0U, 0U),
      binding(1U, 0U, key({1U}), 1U, 1U),
      binding(2U, 0U, key({2U}), 2U, 2U),
      // Deliberately reverse the arm encounter order: locator union history
      // must still follow the quotient's sorted carrier slice (3, 4).
      binding(3U, 1U, key({1U, 2U}), 4U, std::nullopt),
      binding(4U, 1U, key({0U, 1U}), 3U, std::nullopt),
      binding(5U, 2U, key({0U, 1U}), 3U, 4U),
  };
  forest.saddle_records = {
      saddle(0U, 0U, 1U, 0U, 3U, 3U, 0U, 3U, 0U),
      saddle(1U, 1U, 3U, 3U, 2U, 2U, 2U, 0U, 1U),
      saddle(2U, 2U, 4U, 5U, 1U, 1U, 0U, 1U, 2U),
  };
  forest.atomic_groups = {
      group(
          0U,
          1U,
          0U,
          3U,
          0U,
          3U,
          0U,
          3U,
          3U,
          3U,
          ExactDirectMorseForestAtomicGroupKind::multifusion),
      group(
          1U,
          3U,
          1U,
          2U,
          2U,
          0U,
          3U,
          0U,
          4U,
          4U,
          ExactDirectMorseForestAtomicGroupKind::reduced_birth),
      group(
          2U,
          4U,
          2U,
          1U,
          0U,
          1U,
          3U,
          0U,
          std::nullopt,
          4U,
          ExactDirectMorseForestAtomicGroupKind::continuation),
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
       level(2),
       ExactDirectMorseForestNodeKind::reduced_birth,
       3U,
       0U,
       std::nullopt,
       1U},
  };
  forest.batches = {
      batch(0U, 1U, 0, 0U, 3U, 0U, 0U, 0U, 0U, 0U, 0U, 3U, 3U),
      batch(1U, 1U, 1, 3U, 0U, 0U, 1U, 0U, 1U, 3U, 3U, 1U, 1U),
      batch(2U, 2U, 1, 3U, 2U, 1U, 0U, 1U, 0U, 0U, 0U, 2U, 0U),
      batch(3U, 2U, 2, 5U, 0U, 1U, 1U, 1U, 1U, 2U, 0U, 1U, 1U),
      batch(4U, 2U, 3, 5U, 1U, 2U, 1U, 2U, 1U, 1U, 1U, 2U, 1U),
  };
  forest.final_roots = {
      {0U, 1U, 0U, 3U},
      {1U, 2U, 3U, 4U},
  };

  forest.counters.birth_record_count = 6U;
  forest.counters.latent_higher_order_birth_count = 3U;
  forest.counters.order_one_birth_node_count = 3U;
  forest.counters.arm_root_binding_count = 6U;
  forest.counters.saddle_record_count = 3U;
  forest.counters.atomic_group_count = 3U;
  forest.counters.reduced_birth_group_count = 1U;
  forest.counters.continuation_group_count = 1U;
  forest.counters.multifusion_group_count = 1U;
  forest.counters.child_reference_count = 3U;
  forest.counters.batch_record_count = 5U;
  forest.counters.node_count = 5U;
  forest.counters.final_root_count = 2U;
  forest.counters.locator_union_count = 3U;
  forest.counters.quotient_call_count = 3U;
  forest.counters.distinct_strict_arm_count = 6U;
  forest.counters.maximum_batch_arm_count = 3U;
  forest.counters.maximum_batch_carrier_arity = 3U;
  forest.counters.maximum_batch_merge_arity = 3U;
  forest.logical_output_entry_count = 40U;
  populate_real_locator_stamps(forest);
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
  budget.maximum_parent_hop_count = 4096U;
  budget.maximum_exact_level_comparison_count = 4096U;
  budget.maximum_single_exact_level_integer_bit_count = 64U;
  budget.maximum_logical_output_entry_count = 128U;
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

void compare_view_to_static_index(
    const ExactDirectMorseForestCarrierCutReplayView& view,
    const ExactDirectMorseForestCarrierCutIndexResult& expected,
    const std::string& path) {
  check(
      view.target_order() == expected.target_order &&
          view.closed_squared_level() == expected.closed_squared_level &&
          view.target_carrier_count() == expected.entries.size(),
      path + ": synchronous cut metadata matches");
  for (std::size_t index = 0U; index < expected.entries.size(); ++index) {
    check(
        view.entry_at(index) == expected.entries[index],
        path + ": entry_at matches the static cut index");
    check(
        view.find_entry(expected.entries[index].component_handle) ==
            std::optional<ExactDirectMorseForestCarrierCutEntry>{
                expected.entries[index]},
        path + ": find_entry matches the static cut index");
  }
  check(
      !view.find_entry(99U).has_value(),
      path + ": a foreign handle is absent");
}

[[nodiscard]] ExactDirectSparsePositiveFacetProbeResult probe(
    const ExactDirectMorseForestCarrierCutReplayView& view,
    ExactDirectSparseFacetKey facet_key) {
  return view.probe_positive_facet(
      facet_key,
      {authority_id, UINT64_C(999)},
      {128U, 128U});
}

}  // namespace

int main() {
  const auto forest = forest_fixture();
  check(
      forest.certified_conditional_h0_candidate(),
      "the compact real-stamp forest fixture is certified");
  const auto cut_budget = index_budget();
  auto initialized =
      build_exact_direct_morse_forest_carrier_cut_replay_session(
          forest, 2U, session_budget());
  check(
      initialized.certified_ready_session(),
      "the monotone replay session initializes from the forest journal");
  if (!initialized.session) {
    return 1;
  }
  auto& session = *initialized.session;

  for (const std::int64_t cut : {0, 1, 2, 3}) {
    const auto expected =
        build_exact_direct_morse_forest_carrier_cut_index(
            forest, 2U, level(cut), cut_budget);
    check(
        expected.certified_forest_relative_closed_cut_index(),
        "the static comparison index is certified at every test cut");
    bool visited = false;
    const auto advanced = session.advance_to_closed_cut(
        level(cut),
        [&](const ExactDirectMorseForestCarrierCutReplayView& view) {
          visited = true;
          compare_view_to_static_index(
              view, expected, "closed cut " + std::to_string(cut));
          const auto stamp_before_probe = view.locator_snapshot_stamp();
          const auto wrong_order_probe = probe(view, key({0U}));
          check(
              wrong_order_probe.decision ==
                      ExactDirectSparsePositiveFacetProbeDecision::
                          no_positive_locator_input_shape_rejected &&
                  !wrong_order_probe.certified_positive_hit() &&
                  !wrong_order_probe.certified_unresolved_miss() &&
                  !wrong_order_probe.certified_budget_exhaustion(),
              "the public cut probe rejects a key outside the target order before consulting the mixed-order locator");
          const auto first_birth = probe(view, key({0U, 1U}));
          if (cut == 0) {
            check(
                first_birth.certified_unresolved_miss(),
                "a target-order future birth is absent after the complete lower-order prefix");
            check(
                view.committed_global_batch_prefix_count() == 2U,
                "the cut before the first target birth still replays every lower-order batch");
          } else {
            check(
                first_birth.certified_positive_hit() &&
                    first_birth.component_handle_present &&
                    first_birth.component_handle == 3U,
                "the first target birth is positive after its historical batch");
          }
          const auto last_birth = probe(view, key({0U, 2U}));
          check(
              cut < 3 ? last_birth.certified_unresolved_miss()
                      : last_birth.certified_positive_hit() &&
                            last_birth.component_handle == 5U,
              "the same-level current birth appears only after groups have replayed");
          check(
              view.locator_snapshot_stamp() == stamp_before_probe,
              "const probes do not mutate the frozen locator stamp");
        });
    check(
        visited && advanced.certified_forest_relative_closed_cut(),
        "every monotone advance publishes exactly one synchronous certified view");
    if (cut == 0) {
      auto forged_schema = advanced;
      ++forged_schema.frozen_locator_stamp.schema_version;
      auto forged_authority = advanced;
      forged_authority.frozen_locator_stamp.external_authority_id = 0U;
      check(
          !forged_schema.certified_forest_relative_closed_cut() &&
              !forged_authority.certified_forest_relative_closed_cut(),
          "the ephemeral advance receipt rejects a forged locator schema or null authority");
    }
  }

  bool nonmonotone_visited = false;
  const auto nonmonotone = session.advance_to_closed_cut(
      level(2),
      [&](const ExactDirectMorseForestCarrierCutReplayView&) {
        nonmonotone_visited = true;
      });
  check(
      !nonmonotone_visited &&
          nonmonotone.decision ==
              ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
                  no_replay_nonmonotone_closed_cut &&
          nonmonotone.certified_nonmutating_rejection() &&
          session.ready(),
      "a backwards cut is rejected without mutation or poisoning");

  bool equal_cut_visited = false;
  const auto equal_cut = session.advance_to_closed_cut(
      level(3),
      [&](const ExactDirectMorseForestCarrierCutReplayView& view) {
        equal_cut_visited = true;
        check(
            view.committed_global_batch_prefix_count() == 5U,
            "an equal cut reuses the same exact global prefix");
      });
  check(
      equal_cut_visited && equal_cut.certified_forest_relative_closed_cut() &&
          !equal_cut.locator_mutated_during_advance,
      "an equal closed cut republishes a synchronous immutable view");

  auto tampered_forest = forest;
  ++tampered_forest.batches[2U].committed_batch_stamp.binding_count;
  check(
      tampered_forest.certified_conditional_h0_candidate(),
      "the fixture isolates a complete stamp field not replayed by the forest shape predicate");
  auto tampered_session =
      build_exact_direct_morse_forest_carrier_cut_replay_session(
          tampered_forest, 2U, session_budget());
  check(
      tampered_session.certified_ready_session(),
      "a later historical stamp is checked lazily at its exact cut");
  bool tampered_view_visited = false;
  const auto tampered_advance =
      tampered_session.session->advance_to_closed_cut(
          level(1),
          [&](const ExactDirectMorseForestCarrierCutReplayView&) {
            tampered_view_visited = true;
          });
  check(
      !tampered_view_visited && tampered_advance.certified_atomic_failure() &&
          tampered_advance.decision ==
              ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
                  no_replay_locator_stamp_contradiction &&
          tampered_advance.locator_mutated_during_advance &&
          tampered_session.session->poisoned(),
      "a complete post-stamp mismatch exposes no cut and poisons the irreversible session");

  auto understated_group_forest = forest;
  understated_group_forest.atomic_groups[1U].frozen_carrier_count = 1U;
  understated_group_forest.atomic_groups[1U].latent_carrier_count = 1U;
  check(
      understated_group_forest.certified_conditional_h0_candidate(),
      "the fixture isolates group arity fields not replayed by the forest shape predicate");
  auto understated_group_session =
      build_exact_direct_morse_forest_carrier_cut_replay_session(
          understated_group_forest, 2U, session_budget());
  check(
      understated_group_session.certified_ready_session(),
      "an understated group arity is checked lazily during reconstruction");
  bool understated_group_view_visited = false;
  const auto understated_group_advance =
      understated_group_session.session->advance_to_closed_cut(
          level(2),
          [&](const ExactDirectMorseForestCarrierCutReplayView&) {
            understated_group_view_visited = true;
          });
  check(
      !understated_group_view_visited &&
          understated_group_advance.certified_atomic_failure() &&
          understated_group_advance.decision ==
              ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
                  no_replay_source_forest_contradiction &&
          understated_group_session.session->poisoned(),
      "an understated group cannot outgrow preflight scratch or publish a partial view");

  if (failures != 0) {
    std::cerr << failures
              << " direct Morse carrier-cut replay session checks failed\n";
    return 1;
  }
  std::cout << "direct Morse forest carrier-cut replay session checks passed\n";
  return 0;
}
