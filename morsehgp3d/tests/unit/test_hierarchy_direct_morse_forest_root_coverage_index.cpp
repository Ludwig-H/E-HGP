#include "morsehgp3d/hierarchy/direct_morse_forest_root_coverage_index.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
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
      {authority_id, static_cast<std::uint64_t>(3U * birth_index + 1U)}};
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


// 8c8fed7 provenance migration: seal the source-event projection join, the
// arm identity digests and the terminal-birth provenance that the adjacent
// event propagation certification now requires of a self-contained fixture.
// Every materialized birth shares its projection index with its source
// saddle event; each binding's terminal birth is its frozen carrier.
void finalize_forest_event_provenance(
    ExactDirectMorseForestJournalResult& forest) {
  forest.source_event_projection_count =
      forest.point_count + forest.saddle_records.size();
  for (auto& saddle_record : forest.saddle_records) {
    saddle_record.journal_event_projection_index =
        forest.point_count + saddle_record.source_event_index;
    saddle_record.source_event_arm_identity_digest =
        morsehgp3d::contract::CanonicalId::from_lower_hex(
            std::string(64U, '2'));
  }
  for (auto& arm_binding : forest.arm_root_bindings) {
    const std::size_t carrier =
        arm_binding.frozen_carrier_component_handle;
    arm_binding.terminal_birth_record_index = carrier;
    if (carrier < forest.implicit_order_one_prefix_count) {
      ExactDirectSparseFacetKey singleton_key;
      singleton_key.point_count = 1U;
      singleton_key.point_ids[0U] = static_cast<PointId>(carrier);
      arm_binding.terminal_birth_facet_key = singleton_key;
      arm_binding.terminal_birth_binding_witness = {
          forest.config.locator_config.external_authority_id,
          static_cast<std::uint64_t>(3U * carrier + 1U)};
      arm_binding.terminal_birth_exact_squared_level =
          forest.batches.front().squared_level;
    } else {
      const auto& terminal_birth = forest.birth_records[
          carrier - forest.implicit_order_one_prefix_count];
      arm_binding.terminal_birth_facet_key = terminal_birth.facet_key;
      arm_binding.terminal_birth_binding_witness =
          terminal_birth.binding_witness;
      arm_binding.terminal_birth_exact_squared_level =
          forest.batches[terminal_birth.source_journal_batch_index]
              .squared_level;
    }
    for (PointId candidate = 0U;
         static_cast<std::size_t>(candidate) < forest.point_count;
         ++candidate) {
      bool candidate_in_arm_key = false;
      for (std::size_t local = 0U;
           local < arm_binding.strict_arm_key.point_count;
           ++local) {
        candidate_in_arm_key =
            candidate_in_arm_key ||
            arm_binding.strict_arm_key.point_ids[local] == candidate;
      }
      if (!candidate_in_arm_key) {
        arm_binding.removed_support_point_id = candidate;
        break;
      }
    }
  }
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
  finalize_forest_event_provenance(forest);
  return forest;
}

[[nodiscard]] ExactDirectMorseForestCarrierCutIndexBudget cut_budget() {
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

[[nodiscard]] ExactDirectMorseForestRootCoverageIndexBudget
coverage_budget() {
  ExactDirectMorseForestRootCoverageIndexBudget budget;
  budget.maximum_cut_entry_scan_count = 128U;
  budget.maximum_forest_birth_record_scan_count = 256U;
  budget.maximum_forest_node_scan_count = 128U;
  budget.maximum_forest_atomic_group_scan_count = 128U;
  budget.maximum_forest_batch_scan_count = 128U;
  budget.maximum_child_edge_scan_count = 128U;
  budget.maximum_facet_key_point_scan_count = 4096U;
  budget.maximum_point_reference_scan_count = 512U;
  budget.maximum_node_state_count = 128U;
  budget.maximum_depth_first_scratch_count = 128U;
  budget.maximum_resolved_carrier_scratch_count = 128U;
  budget.maximum_point_reference_scratch_count = 512U;
  budget.maximum_referenced_root_count = 128U;
  budget.maximum_carrier_root_binding_count = 128U;
  budget.maximum_coverage_point_count = 512U;
  budget.maximum_logical_output_entry_count = 1024U;
  return budget;
}

void check_points(
    const ExactDirectMorseForestRootCoverageIndexResult& result,
    ExactDirectMorseForestNodeId root_node_id,
    std::initializer_list<PointId> expected,
    const std::string& path) {
  const auto* root = result.find_root(root_node_id);
  if (root == nullptr) {
    check(false, path + ": root exists");
    return;
  }
  const std::span<const PointId> observed =
      result.points_for_root(root->root_coverage_index);
  check(
      observed.size() == expected.size() &&
          std::equal(observed.begin(), observed.end(), expected.begin()),
      path);
}

void check_binding(
    const ExactDirectMorseForestRootCoverageIndexResult& result,
    ExactDirectSparseComponentHandle component_handle,
    ExactDirectMorseForestNodeId root_node_id,
    const std::string& path) {
  const auto* binding = result.find_binding(component_handle);
  check(
      binding != nullptr &&
          binding->root_coverage_index < result.roots.size() &&
          result.roots[binding->root_coverage_index]
                  .reduced_root_node_id == root_node_id,
      path);
}

}  // namespace

int main() {
  const auto forest = forest_fixture();
  const auto trusted_cut_budget = cut_budget();
  const auto budget = coverage_budget();
  check(
      forest.certified_conditional_h0_candidate(),
      "the existing compact direct-forest fixture is accepted");

  const auto latent_cut =
      build_exact_direct_morse_forest_carrier_cut_index(
          forest, 2U, level(1), trusted_cut_budget);
  const auto latent_coverage =
      build_exact_direct_morse_forest_root_coverage_index(
          forest,
          2U,
          level(1),
          trusted_cut_budget,
          latent_cut,
          budget);
  check(
      latent_coverage.certified_forest_relative_root_coverage_index() &&
          latent_coverage.roots.empty() &&
          latent_coverage.point_ids.empty() &&
          latent_coverage.carrier_root_bindings.empty() &&
          latent_coverage.counters.facet_key_point_scan_count != 0U,
      "an all-latent closed cut has a certified empty root coverage index");

  const auto order_one_cut =
      build_exact_direct_morse_forest_carrier_cut_index(
          forest, 1U, level(1), trusted_cut_budget);
  const auto order_one_coverage =
      build_exact_direct_morse_forest_root_coverage_index(
          forest,
          1U,
          level(1),
          trusted_cut_budget,
          order_one_cut,
          budget);
  check(
      order_one_coverage.certified_forest_relative_root_coverage_index() &&
          order_one_coverage.roots.size() == 1U &&
          order_one_coverage.carrier_root_bindings.size() == 4U &&
          order_one_coverage.counters.forest_node_scan_count == 5U &&
          order_one_coverage.counters.child_edge_scan_count == 4U,
      "the implicit order-one prefix is recovered through its multifusion tree");
  check_points(
      order_one_coverage,
      4U,
      {0U, 1U, 2U, 3U},
      "the implicit singleton births cover all four canonical points");
  for (std::size_t handle = 0U; handle < 4U; ++handle) {
    check_binding(
        order_one_coverage,
        handle,
        4U,
        "every resolved implicit carrier is bound to root 4");
  }

  const auto split_cut =
      build_exact_direct_morse_forest_carrier_cut_index(
          forest, 2U, level(4), trusted_cut_budget);
  const auto split_coverage =
      build_exact_direct_morse_forest_root_coverage_index(
          forest,
          2U,
          level(4),
          trusted_cut_budget,
          split_cut,
          budget);
  check(
      split_coverage.certified_forest_relative_root_coverage_index() &&
          split_coverage.roots.size() == 2U &&
          split_coverage.roots[0U].reduced_root_node_id == 5U &&
          split_coverage.roots[0U].carrier_binding_count == 2U &&
          split_coverage.roots[1U].reduced_root_node_id == 6U &&
          split_coverage.roots[1U].carrier_binding_count == 1U,
      "roots are strictly sorted in the split order-two cut");
  check_points(
      split_coverage,
      5U,
      {0U, 1U, 2U, 3U},
      "q0 plus q1 coverage is the union of both carrier births");
  check_points(
      split_coverage,
      6U,
      {0U, 3U},
      "the second q0 root covers its physical higher-order birth");
  check_binding(
      split_coverage, 4U, 5U, "carrier 4 is exhaustively bound");
  check_binding(
      split_coverage, 5U, 5U, "carrier 5 is exhaustively bound");
  check_binding(
      split_coverage, 6U, 6U, "carrier 6 is exhaustively bound");

  const auto fused_cut =
      build_exact_direct_morse_forest_carrier_cut_index(
          forest, 2U, level(5), trusted_cut_budget);
  const auto fused_coverage =
      build_exact_direct_morse_forest_root_coverage_index(
          forest,
          2U,
          level(5),
          trusted_cut_budget,
          fused_cut,
          budget);
  check(
      fused_coverage.certified_forest_relative_root_coverage_index() &&
          fused_coverage.roots.size() == 1U &&
          fused_coverage.carrier_root_bindings.size() == 3U &&
          fused_coverage.counters.forest_node_scan_count == 3U &&
          fused_coverage.counters.child_edge_scan_count == 2U,
      "the order-two multifusion has one audited reachable root tree");
  check_points(
      fused_coverage,
      7U,
      {0U, 1U, 2U, 3U},
      "multifusion coverage is canonical and deduplicated");
  check(
      !fused_coverage.strict_pre_batch_alignment_replayed &&
          !fused_coverage.strict_pre_batch_alignment_claimed &&
          fused_coverage.forest_relative_only &&
          !fused_coverage.gamma_cells_or_global_cofaces_materialized &&
          !fused_coverage.higher_order_delaunay_materialized &&
          !fused_coverage.public_status_claimed,
      "the component makes no strict-prebatch or public exactness claim");

  const auto order_three_cut =
      build_exact_direct_morse_forest_carrier_cut_index(
          forest, 3U, level(2), trusted_cut_budget);
  const auto order_three_coverage =
      build_exact_direct_morse_forest_root_coverage_index(
          forest,
          3U,
          level(2),
          trusted_cut_budget,
          order_three_cut,
          budget);
  check_points(
      order_three_coverage,
      8U,
      {0U, 1U, 2U},
      "the physical order-three suffix birth is recovered exactly");

  const auto verification =
      verify_exact_direct_morse_forest_root_coverage_index(
          forest,
          2U,
          level(5),
          trusted_cut_budget,
          fused_cut,
          budget,
          fused_coverage);
  check(
      verification.result_certified &&
          verification.trusted_source_carrier_cut_index_freshly_verified &&
          verification.expected_index_freshly_reconstructed &&
          verification.observed_recursively_equal &&
          !verification.strict_pre_batch_alignment_replayed,
      "the fresh verifier reconstructs the complete bounded index");
  auto forged_coverage = fused_coverage;
  forged_coverage.point_ids[0U] = 3U;
  check(
      !verify_exact_direct_morse_forest_root_coverage_index(
           forest,
           2U,
           level(5),
           trusted_cut_budget,
           fused_cut,
           budget,
           forged_coverage)
           .result_certified,
      "the fresh verifier rejects a forged CSR point slice");

  auto forged_binding_partition = split_coverage;
  forged_binding_partition.carrier_root_bindings.back()
      .root_coverage_index = 0U;
  check(
      !forged_binding_partition
           .certified_forest_relative_root_coverage_index(),
      "the self-predicate rejects a binding reassigned across root partitions");

  auto forged_cut = fused_cut;
  forged_cut.entries[0U].reduced_root_node_id = 5U;
  const auto rejected_cut =
      build_exact_direct_morse_forest_root_coverage_index(
          forest,
          2U,
          level(5),
          trusted_cut_budget,
          forged_cut,
          budget);
  check(
      rejected_cut.decision ==
              ExactDirectMorseForestRootCoverageIndexDecision::
                  no_coverage_source_cut_rejected &&
          rejected_cut.certified_atomic_failure(),
      "a carrier-root mutation is rejected by fresh cut replay atomically");

  const auto mismatched_cut =
      build_exact_direct_morse_forest_root_coverage_index(
          forest,
          2U,
          level(4),
          trusted_cut_budget,
          fused_cut,
          budget);
  check(
      mismatched_cut.decision ==
              ExactDirectMorseForestRootCoverageIndexDecision::
                  no_coverage_order_or_cut_mismatch &&
          mismatched_cut.certified_atomic_failure(),
      "target order and exact cut metadata must match explicitly");

  auto point_cap = budget;
  point_cap.maximum_point_reference_scan_count = 5U;
  const auto point_exhausted =
      build_exact_direct_morse_forest_root_coverage_index(
          forest,
          2U,
          level(5),
          trusted_cut_budget,
          fused_cut,
          point_cap);
  check(
      point_exhausted.decision ==
              ExactDirectMorseForestRootCoverageIndexDecision::
                  no_coverage_budget_exhausted &&
          point_exhausted.certified_atomic_failure(),
      "the point-reference scan cap is enforced before publication");

  auto key_scan_cap = budget;
  key_scan_cap.maximum_facet_key_point_scan_count = 0U;
  const auto latent_key_scan_exhausted =
      build_exact_direct_morse_forest_root_coverage_index(
          forest,
          2U,
          level(1),
          trusted_cut_budget,
          latent_cut,
          key_scan_cap);
  check(
      latent_key_scan_exhausted.decision ==
              ExactDirectMorseForestRootCoverageIndexDecision::
                  no_coverage_budget_exhausted &&
          latent_key_scan_exhausted.certified_atomic_failure(),
      "the facet-key PointId scan cap also bounds an all-latent cut");

  auto edge_cap = budget;
  edge_cap.maximum_child_edge_scan_count = 3U;
  const auto edge_exhausted =
      build_exact_direct_morse_forest_root_coverage_index(
          forest,
          1U,
          level(1),
          trusted_cut_budget,
          order_one_cut,
          edge_cap);
  check(
      edge_exhausted.decision ==
              ExactDirectMorseForestRootCoverageIndexDecision::
                  no_coverage_budget_exhausted &&
          edge_exhausted.certified_atomic_failure(),
      "the reachable-child edge cap fails atomically");

  auto depth_cap = budget;
  depth_cap.maximum_depth_first_scratch_count = 1U;
  const auto depth_exhausted =
      build_exact_direct_morse_forest_root_coverage_index(
          forest,
          1U,
          level(1),
          trusted_cut_budget,
          order_one_cut,
          depth_cap);
  check(
      depth_exhausted.decision ==
              ExactDirectMorseForestRootCoverageIndexDecision::
                  no_coverage_budget_exhausted &&
          depth_exhausted.certified_atomic_failure(),
      "the DFS scratch cap bounds multifusion depth");

  auto empty_node_state_cap = budget;
  empty_node_state_cap.maximum_node_state_count = 0U;
  const auto empty_without_node_state =
      build_exact_direct_morse_forest_root_coverage_index(
          forest,
          2U,
          level(1),
          trusted_cut_budget,
          latent_cut,
          empty_node_state_cap);
  check(
      empty_without_node_state
              .certified_forest_relative_root_coverage_index() &&
          empty_without_node_state.counters.node_state_count == 0U,
      "an empty cut allocates no forest-node ownership state");

  auto reachable_node_state_cap = budget;
  reachable_node_state_cap.maximum_node_state_count = 2U;
  const auto reachable_node_state_exhausted =
      build_exact_direct_morse_forest_root_coverage_index(
          forest,
          2U,
          level(5),
          trusted_cut_budget,
          fused_cut,
          reachable_node_state_cap);
  check(
      reachable_node_state_exhausted.decision ==
              ExactDirectMorseForestRootCoverageIndexDecision::
                  no_coverage_budget_exhausted &&
          reachable_node_state_exhausted.certified_atomic_failure(),
      "the sparse ownership cap counts only the three reachable nodes");

  auto cyclic_forest = forest;
  cyclic_forest.child_node_ids[3U] = 4U;
  const auto cyclic_rejected =
      build_exact_direct_morse_forest_root_coverage_index(
          cyclic_forest,
          1U,
          level(1),
          trusted_cut_budget,
          order_one_cut,
          budget);
  check(
      !cyclic_forest.certified_conditional_h0_candidate() &&
          cyclic_rejected.decision ==
              ExactDirectMorseForestRootCoverageIndexDecision::
                  no_coverage_source_forest_rejected &&
          cyclic_rejected.certified_atomic_failure(),
      "a hostile self-cycle index is rejected before traversal");

  auto hostile_forest = forest;
  hostile_forest.nodes[3U].child_offset =
      std::numeric_limits<std::size_t>::max();
  const auto hostile_rejected =
      build_exact_direct_morse_forest_root_coverage_index(
          hostile_forest,
          2U,
          level(5),
          trusted_cut_budget,
          fused_cut,
          budget);
  check(
      hostile_rejected.decision ==
              ExactDirectMorseForestRootCoverageIndexDecision::
                  no_coverage_source_forest_rejected &&
          hostile_rejected.certified_atomic_failure(),
      "a hostile child slice is rejected without indexing it");

  if (failures != 0) {
    std::cerr << failures
              << " direct Morse forest root-coverage checks failed\n";
    return 1;
  }
  std::cout << "direct Morse forest root-coverage checks passed\n";
  return 0;
}
