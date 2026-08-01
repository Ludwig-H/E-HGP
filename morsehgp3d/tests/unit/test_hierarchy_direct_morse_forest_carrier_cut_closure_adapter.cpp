#include "morsehgp3d/hierarchy/direct_morse_forest_carrier_cut_replay_session.hpp"

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
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

constexpr std::uint64_t authority_id = UINT64_C(0xC0175E5510);
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
  return {16U, 16U, 16U, 33U, step_budget};
}

[[nodiscard]] ExactDirectMorseForestCarrierCutClosureAdapterBudget
adapter_budget() {
  return {16U, 16U, 128U, 16U, 160U, closure_budget()};
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

[[nodiscard]] ExactDirectSparsePositiveFacetLocator locator_at_cut(
    std::int64_t cut) {
  auto locator = build_exact_direct_sparse_positive_facet_locator(
      5U,
      locator_budget(),
      ExactDirectSparsePositiveFacetLocatorConfig{authority_id});
  check(
      locator.apply_canonical_singleton_identity_batch(3U)
          .certified_committed_identity_batch(),
      "direct comparison singleton commits");
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
      "direct comparison lower prefix commits");
  if (cut >= 52) {
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
        "direct comparison target births commit");
  }
  if (cut >= 53) {
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
        "direct comparison target quotient commits");
  }
  return locator;
}

[[nodiscard]] CanonicalPointCloud cloud_fixture() {
  const std::array<CertifiedPoint3, 3U> points{
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(10.0, 0.0, 0.0),
  };
  return CanonicalPointCloud::rejecting_duplicates(points);
}

void compare_compact_to_direct(
    const ExactDirectMorseForestCarrierCutClosureAdapterResult& compact,
    const ExactDirectSparseFacetDescentClosureResult& direct,
    const ExactDirectMorseForestCarrierCutReplayView& view,
    const std::string& context) {
  check(
      compact.closure_counters == direct.counters &&
          compact.closure_disposition == direct.disposition &&
          compact.closure_decision == direct.decision &&
          compact.required_memo_slot_count ==
              direct.required_memo_slot_count &&
          compact.locator_snapshot_stamp == direct.locator_snapshot_stamp &&
          compact.audit.transient_node_count == direct.nodes.size() &&
          compact.audit.transient_edge_count == direct.edges.size() &&
          compact.audit.transient_seed_projection_count ==
              direct.seed_projections.size(),
      context + ": scalar closure audit exactly matches direct 10.5c");
  check(
      compact.terminal_summaries.size() == direct.seed_projections.size(),
      context + ": one compact summary survives per direct seed");
  for (std::size_t index = 0U;
       index < compact.terminal_summaries.size();
       ++index) {
    const auto& summary = compact.terminal_summaries[index];
    const auto& projection = direct.seed_projections[index];
    const auto& terminal = direct.nodes[projection.terminal_node_index];
    check(
        summary.seed_index == projection.seed_index &&
            summary.source_facet_key == projection.source_facet_key &&
            summary.terminal_facet_key == terminal.facet_key &&
            summary.closure_disposition ==
                projection.closure_disposition &&
            summary.canonical_component_handle ==
                terminal.resolved_component_handle &&
            summary.binding_witness == terminal.resolved_binding_witness,
        context + ": compact terminal projection matches direct 10.5c");
    if (terminal.resolved_component_handle.has_value()) {
      check(
          summary.carrier_cut_entry ==
              view.find_entry(*terminal.resolved_component_handle),
          context + ": positive terminal joins the live carrier cut");
    }
  }
}

}  // namespace

int main() {
  const auto forest = forest_fixture();
  check(
      forest.certified_conditional_h0_candidate(),
      "the compact adapter forest fixture is certified");
  auto initialized =
      build_exact_direct_morse_forest_carrier_cut_replay_session(
          forest, 2U, session_budget());
  check(
      initialized.certified_ready_session(),
      "the carrier-cut replay session initializes for the adapter");
  if (!initialized.session) {
    return 1;
  }

  const CanonicalPointCloud cloud = cloud_fixture();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<ExactDirectSparseFacetKey, 1U> keys{
      key({0U, 1U}),
  };
  const ExactDirectSparseFacetWitness query_witness{authority_id, 999U};

  for (const std::int64_t cut : {51, 52, 53}) {
    bool visited = false;
    const auto advanced = initialized.session->advance_to_closed_cut(
        level(cut),
        [&](const ExactDirectMorseForestCarrierCutReplayView& view) {
          visited = true;
          auto direct_locator = locator_at_cut(cut);
          const auto direct =
              build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys(
                  index,
                  cloud,
                  keys,
                  level(cut),
                  query_witness,
                  direct_locator,
                  closure_budget());
          const auto compact =
              view.build_closure_summary_from_canonical_distinct_keys(
                  index,
                  cloud,
                  keys,
                  query_witness,
                  adapter_budget());
          check(
              compact.certified_compact_closure_summary() &&
                  compact.audit
                      .transient_closure_graph_destroyed_before_return &&
                  !compact.audit
                       .closure_nodes_edges_or_projections_persisted &&
                  compact.closed_squared_level == level(cut),
              "the adapter returns one certified graph-free compact summary");
          compare_compact_to_direct(
              compact,
              direct,
              view,
              "closed cut " + std::to_string(cut));
          if (cut == 51) {
            check(
                compact.decision ==
                        ExactDirectMorseForestCarrierCutClosureAdapterDecision::
                            complete_with_unresolved_terminal_summaries &&
                    compact.terminal_summaries[0U].disposition ==
                        ExactDirectMorseForestCarrierCutClosureTerminalDisposition::
                            closure_unresolved,
                "a future target birth remains unresolved before its batch");
          } else if (cut == 52) {
            check(
                compact.terminal_summaries[0U].disposition ==
                    ExactDirectMorseForestCarrierCutClosureTerminalDisposition::
                        positive_active_latent,
                "the new positive carrier is latent at its birth cut");
            if (compact.terminal_summaries[0U].binding_witness.has_value() &&
                compact.terminal_summaries[0U]
                    .carrier_cut_entry.has_value()) {
              auto falsified_budget = compact;
              falsified_budget.requested_budget
                  .maximum_terminal_summary_count = 0U;
              auto falsified_witness = compact;
              falsified_witness.terminal_summaries[0U]
                  .binding_witness->replay_token = 0U;
              auto falsified_sizes = compact;
              falsified_sizes.required_terminal_key_point_reference_count =
                  0U;
              falsified_sizes.audit
                  .retained_terminal_key_point_reference_count = 0U;
              auto falsified_entry = compact;
              falsified_entry.terminal_summaries[0U]
                  .carrier_cut_entry->reduced_root_node_id = 4U;
              check(
                  !falsified_budget.certified_compact_closure_summary() &&
                      !falsified_witness
                           .certified_compact_closure_summary() &&
                      !falsified_sizes
                           .certified_compact_closure_summary() &&
                      !falsified_entry.certified_compact_closure_summary(),
                  "the compact certificate rejects narrowed storage, forged sizes or witness and an incoherent latent entry");
            } else {
              check(false, "the latent summary exposes its compact authority");
            }
          } else {
            check(
                compact.terminal_summaries[0U].disposition ==
                        ExactDirectMorseForestCarrierCutClosureTerminalDisposition::
                            positive_resolved_reduced_root &&
                    compact.terminal_summaries[0U]
                        .carrier_cut_entry.has_value() &&
                    compact.terminal_summaries[0U]
                            .carrier_cut_entry->reduced_root_node_id ==
                        std::optional<ExactDirectMorseForestNodeId>{4U},
                "the same positive carrier resolves after its quotient group");
            const auto repeated =
                view.build_closure_summary_from_canonical_distinct_keys(
                    index,
                    cloud,
                    keys,
                    query_witness,
                    adapter_budget());
            check(
                repeated == compact,
                "two adapters in one frozen cut epoch are deterministic and commit nothing");
          }
        });
    check(
        visited && advanced.certified_forest_relative_closed_cut(),
        "the monotone carrier cut remains certified after adapter use");
  }

  const auto equal_cut = initialized.session->advance_to_closed_cut(
      level(53),
      [&](const ExactDirectMorseForestCarrierCutReplayView& view) {
        const std::array<ExactDirectSparseFacetKey, 0U> empty_keys{};
        const auto empty =
            view.build_closure_summary_from_canonical_distinct_keys(
                index,
                cloud,
                empty_keys,
                query_witness,
                adapter_budget());
        check(
            empty.certified_compact_closure_summary() &&
                empty.decision ==
                    ExactDirectMorseForestCarrierCutClosureAdapterDecision::
                        complete_empty_key_set &&
                empty.terminal_summaries.empty(),
            "the empty canonical key set builds and destroys one empty closure");

        auto falsified_empty_order = empty;
        falsified_empty_order.target_order = empty.point_count + 1U;
        auto falsified_empty_stamp = empty;
        ++falsified_empty_stamp.locator_snapshot_stamp.schema_version;
        check(
            !falsified_empty_order.certified_compact_closure_summary() &&
                !falsified_empty_stamp.certified_compact_closure_summary(),
            "empty summaries reject impossible order and forged locator schema");

        const std::array<ExactDirectSparseFacetKey, 1U> wrong_order{
            key({0U}),
        };
        const auto rejected_shape =
            view.build_closure_summary_from_canonical_distinct_keys(
                index,
                cloud,
                wrong_order,
                query_witness,
                adapter_budget());
        check(
            rejected_shape.certified_atomic_rejection() &&
                rejected_shape.decision ==
                    ExactDirectMorseForestCarrierCutClosureAdapterDecision::
                        no_adapter_input_shape_rejected &&
                rejected_shape.audit.closure_build_attempt_count == 0U,
            "wrong target cardinality rejects before closure construction");

        const std::array<ExactDirectSparseFacetKey, 2U> duplicate_keys{
            keys[0U], keys[0U]};
        const auto rejected_duplicate =
            view.build_closure_summary_from_canonical_distinct_keys(
                index,
                cloud,
                duplicate_keys,
                query_witness,
                adapter_budget());
        check(
            rejected_duplicate.certified_atomic_rejection() &&
                rejected_duplicate.decision ==
                    ExactDirectMorseForestCarrierCutClosureAdapterDecision::
                        no_adapter_input_shape_rejected &&
                rejected_duplicate.audit.closure_build_attempt_count == 0U,
            "duplicate canonical keys reject before closure construction");

        auto narrow_adapter_budget = adapter_budget();
        narrow_adapter_budget.maximum_terminal_summary_count = 0U;
        const auto rejected_adapter_budget =
            view.build_closure_summary_from_canonical_distinct_keys(
                index,
                cloud,
                keys,
                query_witness,
                narrow_adapter_budget);
        check(
            rejected_adapter_budget.certified_atomic_rejection() &&
                rejected_adapter_budget.decision ==
                    ExactDirectMorseForestCarrierCutClosureAdapterDecision::
                        no_adapter_budget_exhausted &&
                rejected_adapter_budget.audit.closure_build_attempt_count ==
                    0U,
            "compact output capacity rejects before closure construction");

        auto narrow_closure_budget = adapter_budget();
        narrow_closure_budget.closure_budget.maximum_node_count = 0U;
        const auto rejected_closure_budget =
            view.build_closure_summary_from_canonical_distinct_keys(
                index,
                cloud,
                keys,
                query_witness,
                narrow_closure_budget);
        check(
            rejected_closure_budget.certified_atomic_rejection() &&
                rejected_closure_budget.decision ==
                    ExactDirectMorseForestCarrierCutClosureAdapterDecision::
                        no_adapter_closure_budget_exhausted &&
                rejected_closure_budget.audit.closure_build_attempt_count ==
                    1U &&
                rejected_closure_budget.audit
                    .transient_closure_graph_destroyed_before_return &&
                rejected_closure_budget.terminal_summaries.empty(),
            "a 10.5c node preflight exhaustion publishes no partial summary");
      });
  check(
      equal_cut.certified_forest_relative_closed_cut() &&
          !equal_cut.locator_mutated_during_advance,
      "adapter rejection paths leave the equal cut and locator unchanged");

  if (failures != 0) {
    std::cerr << failures
              << " carrier-cut closure adapter checks failed\n";
    return 1;
  }
  std::cout << "carrier-cut closure adapter checks passed\n";
  return 0;
}
