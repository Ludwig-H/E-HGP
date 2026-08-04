#include "morsehgp3d/hierarchy/direct_morse_event_vertical_propagation_journal.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <initializer_list>
#include <limits>
#include <span>
#include <string>
#include <utility>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactCenter3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

constexpr std::uint64_t authority_id = UINT64_C(0xE702);
int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(
    double x,
    double y,
    double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactLevel level(std::int64_t numerator) {
  return {BigInt{numerator}, BigInt{1}};
}

[[nodiscard]] ExactCenter3 center(std::int64_t x) {
  return {BigInt{x}, BigInt{0}, BigInt{0}, BigInt{1}};
}

[[nodiscard]] ExactDirectSparseFacetKey key(
    std::initializer_list<PointId> ids) {
  ExactDirectSparseFacetKey result;
  result.point_count = ids.size();
  std::size_t position = 0U;
  for (const PointId id : ids) {
    result.point_ids[position] = id;
    ++position;
  }
  return result;
}

[[nodiscard]] ExactDirectSparseFacetWitness witness(
    std::size_t birth_record_index) {
  return {
      authority_id,
      static_cast<std::uint64_t>(birth_record_index) * UINT64_C(3) +
          UINT64_C(1)};
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocatorSnapshotStamp stamp(
    std::size_t batch_count) {
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp result;
  result.schema_version = direct_sparse_positive_facet_locator_schema_version;
  result.external_authority_id = authority_id;
  result.committed_batch_count = batch_count;
  return result;
}

[[nodiscard]] morsehgp3d::contract::CanonicalId event_digest(char digit) {
  return morsehgp3d::contract::CanonicalId::from_lower_hex(
      std::string(64U, digit));
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactPairSupportStreamBudget pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum,
          maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget seed_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectSparseFacetDescentStepBudget step_budget() {
  return {
      ExactDirectSparsePositiveFacetProbeBudget{129U, 64U},
      ExactLbvhTopKBudget{256U, 256U, 512U, 256U, 64U, 16U, 16U},
      ExactDirectSparsePositiveFacetProbeBudget{129U, 64U},
  };
}

[[nodiscard]] ExactDirectMorseForestBudget forest_budget() {
  constexpr std::size_t capacity = 4096U;
  ExactDirectMorseForestBudget budget;
  budget.maximum_source_role_scan_count = capacity;
  budget.maximum_source_batch_scan_count = capacity;
  budget.maximum_source_family_scan_count = capacity;
  budget.maximum_source_arm_seed_scan_count = capacity;
  budget.maximum_birth_record_count = capacity;
  budget.maximum_arm_root_binding_count = capacity;
  budget.maximum_saddle_record_count = capacity;
  budget.maximum_atomic_group_count = capacity;
  budget.maximum_child_reference_count = capacity;
  budget.maximum_batch_record_count = capacity;
  budget.maximum_node_count = capacity;
  budget.maximum_final_root_count = capacity;
  budget.maximum_batch_distinct_arm_count = 32U;
  budget.maximum_logical_output_entry_count = 16U * capacity;
  budget.maximum_aggregate_closure_node_count = capacity;
  budget.maximum_aggregate_closure_step_call_count = capacity;
  budget.locator_budget = {
      128U, 128U, 2048U, 128U, 128U, 128U,
      128U, 128U, 2048U, 257U, 257U};
  budget.closure_budget = {32U, 128U, 128U, 257U, step_budget()};
  budget.quotient_budget = {128U, 128U, 128U, 128U, 1024U};
  return budget;
}

[[nodiscard]] ExactDirectMorseForestConfig forest_config() {
  ExactDirectMorseForestConfig config;
  config.locator_config.external_authority_id = authority_id;
  return config;
}

[[nodiscard]] ExactDirectMorseEventRankTowerLinkBudget link_budget() {
  constexpr std::size_t capacity = 4096U;
  ExactDirectMorseEventRankTowerLinkBudget budget;
  budget.maximum_forest_birth_record_scan_count = capacity;
  budget.maximum_forest_saddle_record_scan_count = capacity;
  budget.maximum_forest_arm_binding_scan_count = capacity;
  budget.maximum_forest_atomic_group_scan_count = capacity;
  budget.maximum_forest_batch_scan_count = capacity;
  budget.maximum_forest_node_scan_count = capacity;
  budget.maximum_link_count = capacity;
  budget.maximum_arm_terminal_reference_count = capacity;
  budget.maximum_logical_output_entry_count = 4U * capacity;
  return budget;
}

[[nodiscard]] ExactDirectMorseEventVerticalPropagationBudget
propagation_budget() {
  constexpr std::size_t capacity = 4096U;
  ExactDirectMorseEventVerticalPropagationBudget budget;
  budget.maximum_source_link_scan_count = capacity;
  budget.maximum_source_group_scan_count = capacity;
  budget.maximum_source_arm_binding_scan_count = capacity;
  budget.maximum_source_node_scan_count = capacity;
  budget.maximum_source_child_reference_scan_count = capacity;
  budget.maximum_source_final_root_scan_count = capacity;
  budget.maximum_carrier_anchor_count = capacity;
  budget.maximum_group_anchor_count = capacity;
  budget.maximum_group_input_reference_count = 2U * capacity;
  budget.maximum_final_root_anchor_count = capacity;
  budget.maximum_parent_activation_count = capacity;
  budget.maximum_parent_find_step_count = 16U * capacity;
  budget.maximum_logical_output_entry_count = 8U * capacity;
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

[[nodiscard]] ExactDirectMorseForestBatch forest_batch(
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
  result.strict_pre_batch_stamp = stamp(batch_index);
  result.committed_batch_stamp = stamp(batch_index + 1U);
  result.strict_arms_resolved_before_mutation = true;
  result.quotient_resolved_before_mutation = true;
  result.unions_then_births_committed_atomically = true;
  return result;
}

[[nodiscard]] ExactDirectMorseForestBirthRecord higher_birth(
    std::size_t index,
    std::size_t batch_index,
    ExactDirectSparseFacetKey facet_key) {
  ExactDirectMorseForestBirthRecord result;
  result.birth_record_index = index;
  result.source_event_projection_index = index;
  result.source_journal_batch_index = batch_index;
  result.order = 2U;
  result.facet_key = facet_key;
  result.component_handle = index;
  result.binding_witness = witness(index);
  return result;
}

[[nodiscard]] ExactDirectMorseForestArmRootBinding arm_binding(
    std::size_t index,
    std::size_t family_index,
    ExactDirectSparseFacetKey strict_arm_key,
    ExactDirectSparseComponentHandle carrier,
    std::optional<ExactDirectMorseForestNodeId> prior_root,
    std::size_t terminal_birth_index,
    ExactDirectSparseFacetKey terminal_key,
    PointId removed_point,
    std::int64_t terminal_center,
    std::int64_t terminal_level) {
  ExactDirectMorseForestArmRootBinding result;
  result.binding_index = index;
  result.source_arm_seed_index = index;
  result.source_family_index = family_index;
  result.strict_arm_key = strict_arm_key;
  result.frozen_carrier_component_handle = carrier;
  result.prior_reduced_root_node_id = prior_root;
  result.terminal_birth_record_index = terminal_birth_index;
  result.terminal_birth_facet_key = terminal_key;
  result.terminal_birth_binding_witness = witness(terminal_birth_index);
  result.removed_support_point_id = removed_point;
  result.terminal_birth_exact_center = center(terminal_center);
  result.terminal_birth_exact_squared_level = level(terminal_level);
  return result;
}

[[nodiscard]] ExactDirectMorseForestSaddleRecord saddle_record(
    std::size_t index,
    std::size_t batch_index,
    std::size_t arm_offset,
    std::size_t latent_count,
    std::size_t prior_root_count,
    std::size_t group_index,
    char digest_digit) {
  ExactDirectMorseForestSaddleRecord result;
  result.saddle_record_index = index;
  result.source_family_index = index;
  result.source_event_index = index;
  result.source_journal_batch_index = batch_index;
  result.arm_binding_offset = arm_offset;
  result.arm_binding_count = 2U;
  result.distinct_frozen_carrier_count = 2U;
  result.distinct_latent_carrier_count = latent_count;
  result.distinct_prior_reduced_root_count = prior_root_count;
  result.atomic_group_index = group_index;
  result.journal_event_projection_index = 4U + index;
  result.source_event_arm_identity_digest = event_digest(digest_digit);
  return result;
}

[[nodiscard]] ExactDirectMorseForestAtomicGroup atomic_group(
    std::size_t index,
    std::size_t batch_index,
    std::size_t latent_count,
    std::size_t prior_root_count,
    std::size_t child_offset,
    std::size_t child_count,
    std::optional<ExactDirectMorseForestNodeId> created_node,
    ExactDirectMorseForestNodeId resulting_root,
    ExactDirectMorseForestAtomicGroupKind kind) {
  ExactDirectMorseForestAtomicGroup result;
  result.atomic_group_index = index;
  result.batch_index = batch_index;
  result.saddle_record_offset = index;
  result.saddle_record_count = 1U;
  result.frozen_carrier_count = 2U;
  result.latent_carrier_count = latent_count;
  result.prior_reduced_root_count = prior_root_count;
  result.child_offset = child_offset;
  result.child_count = child_count;
  result.created_node_id = created_node;
  result.resulting_root_node_id = resulting_root;
  result.kind = kind;
  return result;
}

struct Scenario {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedJournalResult seed_journal;
  ExactDirectMorseForestJournalResult forest;
  ExactDirectMorseEventRankTowerLinkJournalResult links;
  ExactDirectMorseEventVerticalPropagationJournalResult propagation;
};

[[nodiscard]] Scenario make_scenario(
    CanonicalPointCloud cloud,
    std::size_t maximum_order) {
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  ExactDirectSupportTerminalBudget terminal_budget;
  terminal_budget.pair = pair_budget();
  terminal_budget.higher = higher_budget();
  auto pair = build_exact_pair_support_stream(
      index, cloud, maximum_order, terminal_budget.pair);
  auto higher = build_exact_higher_support_stream(
      index, cloud, maximum_order, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, maximum_order, terminal_budget, pair, higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  auto seed_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, seed_budget());
  auto forest = build_exact_direct_morse_forest_journal(
      index, cloud, facade, event_journal, seed_budget(), seed_journal,
      forest_budget(), forest_config(), LbvhTraversalOrder::near_first);
  auto links = build_exact_direct_morse_event_rank_tower_link_journal(
      forest, link_budget());
  auto propagation =
      build_exact_direct_morse_event_vertical_propagation_journal(
          forest, links, link_budget(), propagation_budget());
  return {std::move(cloud), std::move(index), std::move(facade),
          std::move(event_journal), std::move(seed_journal),
          std::move(forest), std::move(links), std::move(propagation)};
}

// Forest-relative transaction fixture.  At source order 2 and level 2, the
// two carrier anchors initially target distinct order-1 roots 4 and 5.  The
// lower multifusion 4,5 -> 6 has exactly the same level and must therefore be
// activated before the closed source-group convergence is evaluated.
[[nodiscard]] ExactDirectMorseForestJournalResult
equal_level_lower_fusion_forest() {
  ExactDirectMorseForestJournalResult forest;
  forest.requested_budget = forest_budget();
  forest.config = forest_config();
  forest.point_count = 4U;
  forest.effective_maximum_order = 2U;
  forest.source_higher_canonical_cloud_digest = event_digest('9');
  forest.source_event_projection_count = 9U;
  forest.implicit_order_one_prefix_count = 4U;
  forest.birth_records = {
      higher_birth(4U, 3U, key({0U, 1U})),
      higher_birth(5U, 3U, key({2U, 3U})),
      higher_birth(6U, 4U, key({0U, 2U})),
  };
  forest.arm_root_bindings = {
      arm_binding(0U, 0U, key({0U}), 0U, 0U, 0U, key({0U}),
                  1U, 0, 0),
      arm_binding(1U, 0U, key({1U}), 1U, 1U, 1U, key({1U}),
                  0U, 1, 0),
      arm_binding(2U, 1U, key({2U}), 2U, 2U, 2U, key({2U}),
                  3U, 2, 0),
      arm_binding(3U, 1U, key({3U}), 3U, 3U, 3U, key({3U}),
                  2U, 3, 0),
      arm_binding(4U, 2U, key({0U}), 0U, 4U, 0U, key({0U}), 2U, 0, 0),
      arm_binding(5U, 2U, key({2U}), 2U, 5U, 2U, key({2U}), 0U, 2, 0),
      arm_binding(6U, 3U, key({0U, 1U}), 4U, std::nullopt, 4U,
                  key({0U, 1U}), 2U, 4, 1),
      arm_binding(7U, 3U, key({1U, 2U}), 5U, std::nullopt, 5U,
                  key({2U, 3U}), 0U, 5, 1),
      arm_binding(8U, 4U, key({0U, 2U}), 4U, 7U, 4U,
                  key({0U, 1U}), 3U, 4, 1),
      arm_binding(9U, 4U, key({2U, 3U}), 6U, std::nullopt, 6U,
                  key({0U, 2U}), 0U, 6, 2),
  };
  forest.saddle_records = {
      saddle_record(0U, 1U, 0U, 0U, 2U, 0U, 'a'),
      saddle_record(1U, 1U, 2U, 0U, 2U, 1U, 'b'),
      saddle_record(2U, 2U, 4U, 0U, 2U, 2U, 'c'),
      saddle_record(3U, 4U, 6U, 2U, 0U, 3U, 'd'),
      saddle_record(4U, 5U, 8U, 1U, 1U, 4U, 'e'),
  };
  forest.atomic_groups = {
      atomic_group(0U, 1U, 0U, 2U, 0U, 2U, 4U, 4U,
                   ExactDirectMorseForestAtomicGroupKind::multifusion),
      atomic_group(1U, 1U, 0U, 2U, 2U, 2U, 5U, 5U,
                   ExactDirectMorseForestAtomicGroupKind::multifusion),
      atomic_group(2U, 2U, 0U, 2U, 4U, 2U, 6U, 6U,
                   ExactDirectMorseForestAtomicGroupKind::multifusion),
      atomic_group(3U, 4U, 2U, 0U, 6U, 0U, 7U, 7U,
                   ExactDirectMorseForestAtomicGroupKind::reduced_birth),
      atomic_group(4U, 5U, 1U, 1U, 6U, 0U, std::nullopt, 7U,
                   ExactDirectMorseForestAtomicGroupKind::continuation),
  };
  forest.child_node_ids = {0U, 1U, 2U, 3U, 4U, 5U};
  forest.nodes = {
      {4U, 1U, level(1), ExactDirectMorseForestNodeKind::multifusion,
       0U, 2U, std::nullopt, 0U},
      {5U, 1U, level(1), ExactDirectMorseForestNodeKind::multifusion,
       2U, 2U, std::nullopt, 1U},
      {6U, 1U, level(2), ExactDirectMorseForestNodeKind::multifusion,
       4U, 2U, std::nullopt, 2U},
      {7U, 2U, level(2), ExactDirectMorseForestNodeKind::reduced_birth,
       6U, 0U, std::nullopt, 3U},
  };
  forest.batches = {
      forest_batch(0U, 1U, 0, 0U, 4U, 0U, 0U, 0U, 0U, 0U, 0U,
                   4U, 4U),
      forest_batch(1U, 1U, 1, 4U, 0U, 0U, 2U, 0U, 2U, 4U, 4U,
                   2U, 2U),
      forest_batch(2U, 1U, 2, 4U, 0U, 2U, 1U, 2U, 1U, 2U, 2U,
                   1U, 1U),
      forest_batch(3U, 2U, 1, 4U, 2U, 3U, 0U, 3U, 0U, 0U, 0U,
                   2U, 0U),
      forest_batch(4U, 2U, 2, 6U, 1U, 3U, 1U, 3U, 1U, 2U, 0U,
                   2U, 1U),
      forest_batch(5U, 2U, 3, 7U, 0U, 4U, 1U, 4U, 1U, 2U, 1U,
                   1U, 1U),
  };
  forest.final_roots = {
      {0U, 1U, 0U, 6U},
      {1U, 2U, 4U, 7U},
  };
  forest.counters.birth_record_count = 7U;
  forest.counters.latent_higher_order_birth_count = 3U;
  forest.counters.order_one_birth_node_count = 4U;
  forest.counters.arm_root_binding_count = 10U;
  forest.counters.saddle_record_count = 5U;
  forest.counters.atomic_group_count = 5U;
  forest.counters.reduced_birth_group_count = 1U;
  forest.counters.continuation_group_count = 1U;
  forest.counters.multifusion_group_count = 3U;
  forest.counters.child_reference_count = 6U;
  forest.counters.batch_record_count = 6U;
  forest.counters.node_count = 8U;
  forest.counters.final_root_count = 2U;
  forest.counters.locator_union_count = 5U;
  forest.counters.closure_call_count = 4U;
  forest.counters.quotient_call_count = 4U;
  forest.counters.distinct_strict_arm_count = 10U;
  forest.counters.maximum_batch_arm_count = 4U;
  forest.counters.maximum_batch_carrier_arity = 2U;
  forest.counters.maximum_batch_merge_arity = 2U;
  forest.logical_output_entry_count = 73U;
  forest.final_locator_stamp = stamp(6U);
  set_success_flags(forest);
  forest.decision = ExactDirectMorseForestDecision::
      complete_conditional_exact_direct_morse_forest;
  forest.scope = ExactDirectMorseForestScope::
      all_orders_direct_minimum_carriers_strict_arms_recursive_positive_terminals_and_atomic_full_component_saddle_quotients_with_reduced_qr_only;
  return forest;
}

void test_singleton_has_no_vertical_arrow() {
  const std::array<CertifiedPoint3, 1U> points{
      point(2.0, -1.0, 4.0),
  };
  const Scenario scenario = make_scenario(canonical_cloud(points), 1U);
  check(
      scenario.forest.certified_conditional_h0_candidate() &&
          scenario.links.certified_conditional_event_rank_tower_links() &&
          scenario.propagation
              .certified_conditional_event_vertical_propagation() &&
          scenario.propagation.carrier_anchors.empty() &&
          scenario.propagation.group_anchors.empty() &&
          scenario.propagation.final_root_anchors.empty() &&
          !scenario.propagation
               .terminal_maximum_rank_latent_carrier_required &&
          !scenario.propagation.m1_reconstruction_claimed,
      "n=K_eff=1 keeps the singleton birth without inventing an order-zero vertical arrow");
}

void test_multiorder_and_terminal_latent_carrier() {
  const std::array<CertifiedPoint3, 3U> points{
      point(0.0, 0.0, 0.0),
      point(4.0, 1.0, 0.0),
      point(1.0, 3.0, 1.0),
  };
  const Scenario scenario = make_scenario(canonical_cloud(points), 3U);
  if (!scenario.propagation
           .certified_conditional_event_vertical_propagation()) {
    std::cerr << "diagnostic: forest="
              << static_cast<unsigned>(scenario.forest.decision)
              << " links="
              << static_cast<unsigned>(scenario.links.decision)
              << " propagation="
              << static_cast<unsigned>(scenario.propagation.decision)
              << '\n';
  }
  check(
      scenario.forest.certified_conditional_h0_candidate() &&
          scenario.links.certified_conditional_event_rank_tower_links() &&
          scenario.propagation
              .certified_conditional_event_vertical_propagation(),
      "the exact all-order n=3 forest yields a multiorder adjacent vertical certificate");
  check(
      scenario.propagation.carrier_anchors.size() ==
              scenario.links.links.size() &&
          scenario.propagation.counters
                  .terminal_maximum_rank_latent_carrier_anchor_count ==
              1U &&
          std::count_if(
              scenario.propagation.carrier_anchors.begin(),
              scenario.propagation.carrier_anchors.end(),
              [](const ExactDirectMorseEventVerticalCarrierAnchor& anchor) {
                return anchor.terminal_maximum_rank_latent_carrier &&
                       anchor.remains_latent_without_source_group;
              }) == 1,
      "the unique r=n carrier remains explicit even without a source-order group");
  check(
      std::any_of(
          scenario.propagation.carrier_anchors.begin(),
          scenario.propagation.carrier_anchors.end(),
          [](const ExactDirectMorseEventVerticalCarrierAnchor& anchor) {
            return anchor.source_order == 2U;
          }) &&
          std::any_of(
              scenario.propagation.carrier_anchors.begin(),
              scenario.propagation.carrier_anchors.end(),
              [](const ExactDirectMorseEventVerticalCarrierAnchor& anchor) {
                return anchor.source_order == 3U;
              }) &&
          !scenario.propagation.final_root_anchors.empty() &&
          std::all_of(
              scenario.propagation.group_input_references.begin(),
              scenario.propagation.group_input_references.end(),
              [&scenario](
                  const ExactDirectMorseEventVerticalGroupInputReference&
                      input) {
                return input.target_root_at_group_level ==
                       scenario.propagation.group_anchors[
                           input.group_anchor_index]
                           .target_resulting_root_node_id;
              }),
      "the order-3 and order-2 carrier links propagate through every exact group to the lower final root");
  const auto strong =
      verify_exact_direct_morse_event_vertical_propagation_journal_from_fresh_forest(
          scenario.index, scenario.cloud, scenario.facade,
          scenario.event_journal, seed_budget(), scenario.seed_journal,
          forest_budget(), forest_config(), LbvhTraversalOrder::near_first,
          scenario.forest, link_budget(), scenario.links,
          propagation_budget(), scenario.propagation);
  check(
      strong.result_certified &&
          strong.source_link_fresh_forest_verification
              .source_forest_freshly_reconstructed &&
          strong.propagation_verification.observed_recursively_equal,
      "the strong verifier freshly reconstructs forest, links and propagation");

  auto coordinated_forest = scenario.forest;
  auto coordinated_links = scenario.links;
  auto coordinated_propagation = scenario.propagation;
  const auto forged_digest =
      morsehgp3d::contract::CanonicalId::from_lower_hex(
          std::string(64U, 'a'));
  coordinated_forest.source_higher_canonical_cloud_digest = forged_digest;
  coordinated_links.source_higher_canonical_cloud_digest = forged_digest;
  coordinated_propagation.source_higher_canonical_cloud_digest =
      forged_digest;
  const auto forest_relative =
      verify_exact_direct_morse_event_vertical_propagation_journal(
          coordinated_forest, coordinated_links, link_budget(),
          propagation_budget(), coordinated_propagation);
  const auto upstream_replay =
      verify_exact_direct_morse_event_vertical_propagation_journal_from_fresh_forest(
          scenario.index, scenario.cloud, scenario.facade,
          scenario.event_journal, seed_budget(), scenario.seed_journal,
          forest_budget(), forest_config(), LbvhTraversalOrder::near_first,
          coordinated_forest, link_budget(), coordinated_links,
          propagation_budget(), coordinated_propagation);
  check(
      forest_relative.result_certified && !upstream_replay.result_certified &&
          !upstream_replay.source_link_fresh_forest_verification
               .source_forest_recursively_equal,
      "the forest-relative verifier names its premise, while the strong wrapper rejects a coordinated cloud-identity forgery");

  if (!scenario.propagation.carrier_anchors.empty()) {
    auto mutated = scenario.propagation;
    mutated.carrier_anchors.back().referenced_by_source_atomic_group = true;
    check(
        !verify_exact_direct_morse_event_vertical_propagation_journal(
             scenario.forest, scenario.links, link_budget(),
             propagation_budget(), mutated)
             .result_certified,
        "fresh reconstruction rejects a mutation that consumes the terminal latent carrier");
  }
}

void test_simultaneous_group_inputs() {
  const std::array<CertifiedPoint3, 4U> points{
      point(-2.0, 0.0),
      point(0.0, -3.0),
      point(0.0, 3.0),
      point(2.0, 0.0),
  };
  const Scenario scenario = make_scenario(canonical_cloud(points), 2U);
  if (!scenario.propagation
           .certified_conditional_event_vertical_propagation()) {
    std::cerr << "diagnostic simultaneous: forest="
              << static_cast<unsigned>(scenario.forest.decision)
              << " links=" << static_cast<unsigned>(scenario.links.decision)
              << " propagation="
              << static_cast<unsigned>(scenario.propagation.decision)
              << '\n';
  }
  const auto simultaneous = std::find_if(
      scenario.forest.atomic_groups.begin(),
      scenario.forest.atomic_groups.end(),
      [&scenario](const ExactDirectMorseForestAtomicGroup& group) {
        return scenario.forest.batches[group.batch_index].order == 2U &&
               group.saddle_record_count >= 2U;
      });
  const auto propagated =
      simultaneous == scenario.forest.atomic_groups.end()
          ? scenario.propagation.group_anchors.end()
          : std::find_if(
                scenario.propagation.group_anchors.begin(),
                scenario.propagation.group_anchors.end(),
                [simultaneous](
                    const ExactDirectMorseEventVerticalGroupAnchor& anchor) {
                  return anchor.source_atomic_group_index ==
                         simultaneous->atomic_group_index;
                });
  check(
      scenario.propagation
              .certified_conditional_event_vertical_propagation() &&
          !scenario.propagation
               .terminal_maximum_rank_latent_carrier_required &&
          scenario.propagation.counters
                  .unconsumed_latent_carrier_anchor_count ==
              0U &&
          simultaneous != scenario.forest.atomic_groups.end() &&
          propagated != scenario.propagation.group_anchors.end() &&
          propagated->input_reference_count >=
              simultaneous->frozen_carrier_count,
      "one simultaneous lower-order quotient propagates every distinct carrier through one CSR group anchor");
}

void test_equal_level_lower_fusion_is_closed_before_source_group() {
  const auto forest = equal_level_lower_fusion_forest();
  const auto links =
      build_exact_direct_morse_event_rank_tower_link_journal(
          forest, link_budget());
  const auto propagation =
      build_exact_direct_morse_event_vertical_propagation_journal(
          forest, links, link_budget(), propagation_budget());
  const auto source_group = std::find_if(
      propagation.group_anchors.begin(), propagation.group_anchors.end(),
      [](const ExactDirectMorseEventVerticalGroupAnchor& anchor) {
        return anchor.source_order == 2U &&
               anchor.squared_level == level(2);
      });
  bool distinct_before_common_after = false;
  if (source_group != propagation.group_anchors.end() &&
      source_group->input_reference_count >= 2U) {
    const auto& first = propagation.group_input_references[
        source_group->input_reference_offset];
    const auto& second = propagation.group_input_references[
        source_group->input_reference_offset + 1U];
    distinct_before_common_after =
        first.target_root_before_advance !=
            second.target_root_before_advance &&
        first.target_root_at_group_level == 6U &&
        second.target_root_at_group_level == 6U &&
        source_group->target_resulting_root_node_id == 6U;
  }
  if (!distinct_before_common_after) {
    std::cerr << "diagnostic equal-level: forest="
              << static_cast<unsigned>(forest.decision)
              << " links=" << static_cast<unsigned>(links.decision)
              << " propagation="
              << static_cast<unsigned>(propagation.decision)
              << " groups=" << propagation.group_anchors.size() << '\n';
  }
  check(
      forest.certified_conditional_h0_candidate() &&
          links.certified_conditional_event_rank_tower_links() &&
          propagation.certified_conditional_event_vertical_propagation() &&
          distinct_before_common_after,
      "the lower order-1 multifusion at the exact source level is activated before closed order-2 convergence");
}

void test_same_batch_birth_cannot_forge_a_strict_carrier() {
  const auto honest_forest = equal_level_lower_fusion_forest();
  const auto honest_links =
      build_exact_direct_morse_event_rank_tower_link_journal(
          honest_forest, link_budget());
  auto forged_forest = honest_forest;

  const auto batch = std::find_if(
      forged_forest.batches.begin(), forged_forest.batches.end(),
      [](const ExactDirectMorseForestBatch& candidate) {
        return candidate.order == 2U && candidate.squared_level == level(2) &&
               candidate.birth_record_count != 0U &&
               candidate.atomic_group_count != 0U;
      });
  bool fixture_ready = batch != forged_forest.batches.end();
  if (fixture_ready) {
    const auto& group = forged_forest.atomic_groups[
        batch->atomic_group_offset];
    const auto& saddle = forged_forest.saddle_records[
        group.saddle_record_offset];
    auto& binding = forged_forest.arm_root_bindings[
        saddle.arm_binding_offset];
    binding.frozen_carrier_component_handle = batch->birth_record_offset;
  }

  const auto rejected_links =
      build_exact_direct_morse_event_rank_tower_link_journal(
          forged_forest, link_budget());
  const auto rejected_propagation =
      build_exact_direct_morse_event_vertical_propagation_journal(
          forged_forest, honest_links, link_budget(), propagation_budget());
  check(
      fixture_ready &&
          honest_forest.certified_conditional_h0_candidate() &&
          honest_links.certified_conditional_event_rank_tower_links() &&
          !forged_forest.certified_conditional_h0_candidate() &&
          rejected_links.certified_atomic_failure() &&
          rejected_links.decision ==
              ExactDirectMorseEventRankTowerLinkDecision::
                  no_link_arm_terminal_inconsistent &&
          rejected_propagation.certified_atomic_failure() &&
          rejected_propagation.decision ==
              ExactDirectMorseEventVerticalPropagationDecision::
                  no_propagation_source_forest_rejected,
      "a birth introduced in the current batch cannot be forged into the frozen strict carrier set");
}

void test_multifusion_and_fail_closed_budget() {
  const std::array<CertifiedPoint3, 5U> points{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0),
  };
  const Scenario scenario = make_scenario(canonical_cloud(points), 2U);
  if (!scenario.propagation
           .certified_conditional_event_vertical_propagation()) {
    std::cerr << "diagnostic multifusion: forest="
              << static_cast<unsigned>(scenario.forest.decision)
              << " links=" << static_cast<unsigned>(scenario.links.decision)
              << " propagation="
              << static_cast<unsigned>(scenario.propagation.decision)
              << '\n';
  }
  const auto multifusion = std::find_if(
      scenario.forest.atomic_groups.begin(),
      scenario.forest.atomic_groups.end(),
      [&scenario](const ExactDirectMorseForestAtomicGroup& group) {
        return scenario.forest.batches[group.batch_index].order == 2U &&
               group.kind ==
                   ExactDirectMorseForestAtomicGroupKind::multifusion;
      });
  const auto propagated =
      multifusion == scenario.forest.atomic_groups.end()
          ? scenario.propagation.group_anchors.end()
          : std::find_if(
                scenario.propagation.group_anchors.begin(),
                scenario.propagation.group_anchors.end(),
                [multifusion](
                    const ExactDirectMorseEventVerticalGroupAnchor& anchor) {
                  return anchor.source_atomic_group_index ==
                         multifusion->atomic_group_index;
                });
  check(
      scenario.propagation
              .certified_conditional_event_vertical_propagation() &&
          multifusion != scenario.forest.atomic_groups.end() &&
          multifusion->prior_reduced_root_count == 2U &&
          propagated != scenario.propagation.group_anchors.end() &&
          propagated->input_reference_count >= 2U,
      "the binary source multifusion advances both prior-root anchors before accepting their common lower target");

  auto insufficient = propagation_budget();
  insufficient.maximum_group_input_reference_count = 0U;
  const auto rejected =
      build_exact_direct_morse_event_vertical_propagation_journal(
          scenario.forest, scenario.links, link_budget(), insufficient);
  check(
      rejected.certified_atomic_failure() &&
          rejected.carrier_anchors.empty() &&
          rejected.group_anchors.empty() &&
          rejected.group_input_references.empty() &&
          rejected.final_root_anchors.empty(),
      "a preflight output-capacity failure publishes no partial scientific payload");
}

}  // namespace

int main() {
  test_singleton_has_no_vertical_arrow();
  test_multiorder_and_terminal_latent_carrier();
  test_simultaneous_group_inputs();
  test_equal_level_lower_fusion_is_closed_before_source_group();
  test_same_batch_birth_cannot_forge_a_strict_carrier();
  test_multifusion_and_fail_closed_budget();
  return failures == 0 ? 0 : 1;
}
