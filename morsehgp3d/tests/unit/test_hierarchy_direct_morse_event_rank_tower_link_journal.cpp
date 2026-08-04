#include "morsehgp3d/hierarchy/direct_morse_event_rank_tower_link_journal.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
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

constexpr std::uint64_t authority_id = UINT64_C(0xE701);
int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] ExactLevel level(std::int64_t numerator) {
  return {BigInt{numerator}, BigInt{1}};
}

[[nodiscard]] ExactCenter3 center(std::int64_t x) {
  return {BigInt{x}, BigInt{0}, BigInt{0}, BigInt{1}};
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactPairSupportStreamBudget source_pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget source_higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget source_seed_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectSparseFacetDescentStepBudget step_budget() {
  return {
      ExactDirectSparsePositiveFacetProbeBudget{129U, 64U},
      ExactLbvhTopKBudget{256U, 256U, 512U, 256U, 64U, 16U, 16U},
      ExactDirectSparsePositiveFacetProbeBudget{129U, 64U}};
}

[[nodiscard]] morsehgp3d::contract::CanonicalId event_digest(
    char digit = 'a') {
  return morsehgp3d::contract::CanonicalId::from_lower_hex(
      std::string(64U, digit));
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
  result.schema_version =
      direct_sparse_positive_facet_locator_schema_version;
  result.external_authority_id = authority_id;
  result.committed_batch_count = batch_count;
  return result;
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
  budget.maximum_batch_distinct_arm_count = 16U;
  budget.maximum_logical_output_entry_count = capacity;
  budget.maximum_aggregate_closure_node_count = capacity;
  budget.maximum_aggregate_closure_step_call_count = capacity;
  budget.locator_budget = {
      64U,
      64U,
      640U,
      64U,
      64U,
      64U,
      64U,
      64U,
      640U,
      129U,
      129U};
  budget.closure_budget = {16U, 64U, 64U, 129U, step_budget()};
  budget.quotient_budget = {64U, 64U, 64U, 64U, 256U};
  return budget;
}

[[nodiscard]] ExactDirectMorseForestConfig forest_config() {
  ExactDirectMorseForestConfig config;
  config.locator_config.external_authority_id = authority_id;
  return config;
}

struct EndToEndScenario {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedJournalResult seed_journal;
  ExactDirectMorseForestJournalResult forest;
};

[[nodiscard]] EndToEndScenario end_to_end_scenario() {
  const std::array<CertifiedPoint3, 3U> points{
      point(0.0, 0.0), point(4.0, 0.0), point(1.0, 3.0)};
  auto cloud = canonical_cloud(points);
  auto index = MortonLbvhIndex::build(cloud);
  const ExactDirectSupportTerminalBudget terminal_budget{
      source_pair_budget(), source_higher_budget()};
  const auto pair = build_exact_pair_support_stream(
      index, cloud, 2U, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, 2U, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, 2U, terminal_budget, pair, higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  auto seed_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, source_seed_budget());
  auto forest = build_exact_direct_morse_forest_journal(
      index,
      cloud,
      facade,
      event_journal,
      source_seed_budget(),
      seed_journal,
      forest_budget(),
      forest_config(),
      LbvhTraversalOrder::near_first);
  return {
      std::move(cloud),
      std::move(index),
      std::move(facade),
      std::move(event_journal),
      std::move(seed_journal),
      std::move(forest)};
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

[[nodiscard]] ExactDirectMorseForestJournalResult terminal_fixture() {
  ExactDirectMorseForestJournalResult forest;
  forest.requested_budget = forest_budget();
  forest.config.locator_config.external_authority_id = authority_id;
  forest.point_count = 2U;
  forest.effective_maximum_order = 2U;
  forest.source_higher_canonical_cloud_digest =
      morsehgp3d::contract::CanonicalId::from_lower_hex(
          std::string(64U, '7'));
  forest.implicit_order_one_prefix_count = 2U;

  forest.birth_records = {
      {2U,
       2U,
       2U,
       2U,
       key({0U, 1U}),
       2U,
       std::nullopt,
       witness(2U)}};
  forest.arm_root_bindings = {
      {0U,
       0U,
       0U,
       key({0U}),
       0U,
       0U,
       0U,
       key({0U}),
       witness(0U),
       1U,
       center(0),
       level(0)},
      {1U,
       1U,
       0U,
       key({1U}),
       1U,
       1U,
       1U,
       key({1U}),
       witness(1U),
       0U,
       center(1),
       level(0)}};
  forest.saddle_records = {
      {0U,
       0U,
       0U,
       1U,
       0U,
       2U,
       2U,
       0U,
       2U,
       0U,
       2U,
       event_digest()}};
  forest.atomic_groups = {
      {0U,
       1U,
       0U,
       1U,
       2U,
       0U,
       2U,
       0U,
       2U,
       2U,
       2U,
       ExactDirectMorseForestAtomicGroupKind::multifusion}};
  forest.child_node_ids = {0U, 1U};
  forest.nodes = {
      {2U,
       1U,
       level(1),
       ExactDirectMorseForestNodeKind::multifusion,
       0U,
       2U,
       std::nullopt,
       0U}};
  forest.batches = {
      batch(0U, 1U, 0, 0U, 2U, 0U, 0U, 0U, 0U, 0U, 0U, 2U, 2U),
      batch(1U, 1U, 1, 2U, 0U, 0U, 1U, 0U, 1U, 2U, 2U, 1U, 1U),
      batch(2U, 2U, 1, 2U, 1U, 1U, 0U, 1U, 0U, 0U, 0U, 1U, 0U)};
  forest.final_roots = {{0U, 1U, 0U, 2U}};

  forest.counters.birth_record_count = 3U;
  forest.counters.latent_higher_order_birth_count = 1U;
  forest.counters.order_one_birth_node_count = 2U;
  forest.counters.arm_root_binding_count = 2U;
  forest.counters.saddle_record_count = 1U;
  forest.counters.atomic_group_count = 1U;
  forest.counters.multifusion_group_count = 1U;
  forest.counters.child_reference_count = 2U;
  forest.counters.batch_record_count = 3U;
  forest.counters.node_count = 3U;
  forest.counters.final_root_count = 1U;
  forest.counters.locator_union_count = 1U;
  forest.counters.closure_call_count = 1U;
  forest.counters.quotient_call_count = 1U;
  forest.counters.distinct_strict_arm_count = 2U;
  forest.counters.maximum_batch_arm_count = 2U;
  forest.counters.maximum_batch_carrier_arity = 2U;
  forest.counters.maximum_batch_merge_arity = 2U;
  forest.logical_output_entry_count = 24U;
  forest.final_locator_stamp = stamp(3U);
  set_success_flags(forest);
  forest.decision = ExactDirectMorseForestDecision::
      complete_conditional_exact_direct_morse_forest;
  forest.scope = ExactDirectMorseForestScope::
      all_orders_direct_minimum_carriers_strict_arms_recursive_positive_terminals_and_atomic_full_component_saddle_quotients_with_reduced_qr_only;
  forest.source_event_projection_count = 3U;
  return forest;
}

[[nodiscard]] ExactDirectMorseForestJournalResult singleton_fixture() {
  ExactDirectMorseForestJournalResult forest;
  forest.requested_budget = forest_budget();
  forest.config.locator_config.external_authority_id = authority_id;
  forest.point_count = 1U;
  forest.effective_maximum_order = 1U;
  forest.source_higher_canonical_cloud_digest =
      morsehgp3d::contract::CanonicalId::from_lower_hex(
          std::string(64U, '8'));
  forest.implicit_order_one_prefix_count = 1U;
  forest.batches = {
      batch(0U, 1U, 0, 0U, 1U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 1U)};
  forest.final_roots = {{0U, 1U, 0U, 0U}};
  forest.counters.birth_record_count = 1U;
  forest.counters.order_one_birth_node_count = 1U;
  forest.counters.batch_record_count = 1U;
  forest.counters.node_count = 1U;
  forest.counters.final_root_count = 1U;
  forest.logical_output_entry_count = 5U;
  forest.final_locator_stamp = stamp(1U);
  set_success_flags(forest);
  forest.decision = ExactDirectMorseForestDecision::
      complete_conditional_exact_direct_morse_forest;
  forest.scope = ExactDirectMorseForestScope::
      all_orders_direct_minimum_carriers_strict_arms_recursive_positive_terminals_and_atomic_full_component_saddle_quotients_with_reduced_qr_only;
  forest.source_event_projection_count = 1U;
  return forest;
}

[[nodiscard]] ExactDirectMorseEventRankTowerLinkBudget budget() {
  return {
      32U,
      32U,
      32U,
      32U,
      32U,
      32U,
      32U,
      32U,
      128U};
}

void test_terminal_rank_link_and_fresh_verification() {
  const auto forest = terminal_fixture();
  check(
      forest.certified_conditional_h0_candidate(),
      "the terminal-rank source forest is conditionally certified");
  const auto result =
      build_exact_direct_morse_event_rank_tower_link_journal(
          forest, budget());
  check(
      result.certified_conditional_event_rank_tower_links(),
      "the same-event adjacent-rank link journal is certified");
  check(
      result.links.size() == 1U &&
          result.arm_terminals.size() == 2U &&
          result.links[0U].source_birth_record_index == 2U &&
          result.links[0U].source_event_projection_index == 2U &&
          result.links[0U].source_order == 2U &&
          result.links[0U].target_order == 1U &&
          result.links[0U].saddle_record_index == 0U &&
          result.links[0U].lower_batch_index == 1U &&
          result.links[0U].atomic_group_index == 0U &&
          result.links[0U].target_resulting_root_node_id == 2U &&
          result.links[0U].arm_terminal_offset == 0U &&
          result.links[0U].arm_terminal_count == 2U &&
          result.links[0U].lower_strict_pre_batch_stamp == stamp(1U) &&
          result.links[0U].lower_committed_post_batch_stamp == stamp(2U) &&
          result.links[0U].source_event_arm_identity_digest ==
              event_digest() &&
          result.arm_terminals[0U].removed_support_point_id == 1U &&
          result.arm_terminals[0U].terminal_birth_facet_key == key({0U}) &&
          result.arm_terminals[0U].terminal_birth_binding_witness ==
              witness(0U) &&
          result.arm_terminals[0U].terminal_birth_exact_center == center(0) &&
          result.arm_terminals[0U].terminal_birth_exact_squared_level ==
              level(0),
      "the terminal r=n birth is anchored by its order-(n-1) saddle group");
  check(
      result.counters.terminal_maximum_rank_birth_count == 1U &&
          result.terminal_maximum_rank_link_required &&
          result.all_present_terminal_maximum_rank_births_included &&
          result.lower_rank_links_are_composition_only &&
          !result.public_status_claimed &&
          !result.gamma_cells_or_global_cofaces_materialized &&
          !result.higher_order_delaunay_materialized,
      "the terminal link is explicit without a source-order group or global complex");
  const auto verification =
      verify_exact_direct_morse_event_rank_tower_link_journal(
          forest, budget(), result);
  check(
      verification.result_certified &&
          verification.expected_journal_freshly_rebuilt &&
          verification.observed_recursively_equal,
      "fresh forest-relative reconstruction accepts the unmodified journal");

  auto mutated = result;
  mutated.links[0U].target_resulting_root_node_id = 0U;
  check(
      !verify_exact_direct_morse_event_rank_tower_link_journal(
           forest, budget(), mutated)
           .result_certified,
      "fresh reconstruction rejects an observed target-root mutation");
}

void test_singleton_terminal_has_no_order_zero_link() {
  const auto forest = singleton_fixture();
  check(
      forest.certified_conditional_h0_candidate(),
      "the singleton source forest is conditionally certified");
  const auto result =
      build_exact_direct_morse_event_rank_tower_link_journal(
          forest, budget());
  check(
      result.certified_conditional_event_rank_tower_links() &&
          result.links.empty() && result.arm_terminals.empty() &&
          !result.terminal_maximum_rank_link_required &&
          result.counters.terminal_maximum_rank_birth_count == 0U,
      "K_eff=n=1 retains T1 without inventing an order-zero link");
  check(
      verify_exact_direct_morse_event_rank_tower_link_journal(
          forest, budget(), result)
          .result_certified,
      "fresh replay accepts the empty singleton adjacent-link journal");
}

void test_fresh_upstream_replay_rejects_coordinated_identity_substitutions() {
  const auto scenario = end_to_end_scenario();
  const auto honest_link =
      build_exact_direct_morse_event_rank_tower_link_journal(
          scenario.forest, budget());
  const auto honest_verification =
      verify_exact_direct_morse_event_rank_tower_link_journal_from_fresh_forest(
          scenario.index,
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          source_seed_budget(),
          scenario.seed_journal,
          forest_budget(),
          forest_config(),
          LbvhTraversalOrder::near_first,
          scenario.forest,
          budget(),
          honest_link);
  check(
      scenario.forest.certified_conditional_h0_candidate() &&
          honest_link.certified_conditional_event_rank_tower_links() &&
          honest_verification.source_forest_freshly_reconstructed &&
          honest_verification.source_forest_recursively_equal &&
          honest_verification.observed_link_recursively_equal &&
          honest_verification.result_certified,
      "fresh upstream forest and adjacent-link replay accept the honest end-to-end payload");

  bool duplicate_projection_rejected =
      scenario.forest.birth_records.size() >= 2U;
  if (duplicate_projection_rejected) {
    auto duplicate = scenario.forest;
    const std::size_t first_projection =
        duplicate.birth_records[0U].source_event_projection_index;
    const std::size_t second_projection =
        duplicate.birth_records[1U].source_event_projection_index;
    auto first_saddle = std::find_if(
        duplicate.saddle_records.begin(),
        duplicate.saddle_records.end(),
        [first_projection](const auto& saddle) {
          return saddle.journal_event_projection_index == first_projection;
        });
    auto second_saddle = std::find_if(
        duplicate.saddle_records.begin(),
        duplicate.saddle_records.end(),
        [second_projection](const auto& saddle) {
          return saddle.journal_event_projection_index == second_projection;
        });
    duplicate_projection_rejected =
        first_saddle != duplicate.saddle_records.end() &&
        second_saddle != duplicate.saddle_records.end() &&
        first_saddle != second_saddle;
    if (duplicate_projection_rejected) {
      duplicate.birth_records[1U].source_event_projection_index =
          first_projection;
      second_saddle->source_event_index = first_saddle->source_event_index;
      second_saddle->journal_event_projection_index = first_projection;
      second_saddle->source_event_arm_identity_digest =
          first_saddle->source_event_arm_identity_digest;
      const auto duplicate_link =
          build_exact_direct_morse_event_rank_tower_link_journal(
              duplicate, budget());
      const auto duplicate_verification =
          verify_exact_direct_morse_event_rank_tower_link_journal_from_fresh_forest(
              scenario.index,
              scenario.cloud,
              scenario.facade,
              scenario.event_journal,
              source_seed_budget(),
              scenario.seed_journal,
              forest_budget(),
              forest_config(),
              LbvhTraversalOrder::near_first,
              duplicate,
              budget(),
              duplicate_link);
      duplicate_projection_rejected =
          duplicate_link.decision ==
              ExactDirectMorseEventRankTowerLinkDecision::
                  no_link_duplicate_adjacent_saddle_role &&
          duplicate_link.certified_atomic_failure() &&
          !duplicate.certified_conditional_h0_candidate() &&
          duplicate_verification.source_forest_freshly_reconstructed &&
          !duplicate_verification.source_forest_recursively_equal &&
          !duplicate_verification.result_certified;
    }
  }
  check(
      duplicate_projection_rejected,
      "a coordinated duplicate birth/saddle projection and digest is rejected before publication and by fresh upstream replay");

  bool coordinated_terminal_substitution_rejected = false;
  for (std::size_t source_index = 0U;
       source_index < scenario.forest.arm_root_bindings.size() &&
       !coordinated_terminal_substitution_rejected;
       ++source_index) {
    for (std::size_t replacement_index = 0U;
         replacement_index < scenario.forest.arm_root_bindings.size();
         ++replacement_index) {
      const auto& source =
          scenario.forest.arm_root_bindings[source_index];
      const auto& replacement =
          scenario.forest.arm_root_bindings[replacement_index];
      if (source.terminal_birth_record_index ==
          replacement.terminal_birth_record_index) {
        continue;
      }
      const ExactDirectMorseForestJournalView view{scenario.forest};
      if (view.birth_record_at(source.terminal_birth_record_index).order !=
          view.birth_record_at(replacement.terminal_birth_record_index)
              .order) {
        continue;
      }
      auto substituted = scenario.forest;
      auto& mutated = substituted.arm_root_bindings[source_index];
      mutated.terminal_birth_record_index =
          replacement.terminal_birth_record_index;
      mutated.terminal_birth_facet_key =
          replacement.terminal_birth_facet_key;
      mutated.terminal_birth_binding_witness =
          replacement.terminal_birth_binding_witness;
      mutated.terminal_birth_exact_center =
          replacement.terminal_birth_exact_center;
      mutated.terminal_birth_exact_squared_level =
          replacement.terminal_birth_exact_squared_level;
      const auto substituted_link =
          build_exact_direct_morse_event_rank_tower_link_journal(
              substituted, budget());
      const auto substituted_verification =
          verify_exact_direct_morse_event_rank_tower_link_journal_from_fresh_forest(
              scenario.index,
              scenario.cloud,
              scenario.facade,
              scenario.event_journal,
              source_seed_budget(),
              scenario.seed_journal,
              forest_budget(),
              forest_config(),
              LbvhTraversalOrder::near_first,
              substituted,
              budget(),
              substituted_link);
      coordinated_terminal_substitution_rejected =
          substituted.certified_conditional_h0_candidate() &&
          substituted_link.certified_conditional_event_rank_tower_links() &&
          substituted_verification.source_forest_freshly_reconstructed &&
          !substituted_verification.source_forest_recursively_equal &&
          !substituted_verification.result_certified;
      if (coordinated_terminal_substitution_rejected) {
        break;
      }
    }
  }
  check(
      coordinated_terminal_substitution_rejected,
      "fresh upstream replay rejects a locally coherent coordinated terminal birth/key/witness/center/level substitution");
}

void test_fail_closed_role_level_terminal_and_group_mutations() {
  const auto expected_failure = [](
                                    const ExactDirectMorseForestJournalResult&
                                        forest,
                                    ExactDirectMorseEventRankTowerLinkDecision
                                        decision,
                                    const std::string& message) {
    const auto result =
        build_exact_direct_morse_event_rank_tower_link_journal(
            forest, budget());
    check(
        result.decision == decision && result.certified_atomic_failure() &&
            result.links.empty() && result.arm_terminals.empty(),
        message);
  };

  auto missing = terminal_fixture();
  missing.source_event_projection_count = 4U;
  missing.saddle_records[0U].source_event_index = 1U;
  missing.saddle_records[0U].journal_event_projection_index = 3U;
  expected_failure(
      missing,
      ExactDirectMorseEventRankTowerLinkDecision::
          no_link_missing_adjacent_saddle_role,
      "a missing same-event saddle role fails atomically");

  auto wrong_level = terminal_fixture();
  wrong_level.batches[1U].squared_level = level(2);
  wrong_level.nodes[0U].squared_level = level(2);
  expected_failure(
      wrong_level,
      ExactDirectMorseEventRankTowerLinkDecision::
          no_link_exact_level_mismatch,
      "a birth/saddle exact-level mismatch fails atomically");

  auto wrong_terminal = terminal_fixture();
  wrong_terminal.arm_root_bindings[0U].terminal_birth_record_index = 2U;
  expected_failure(
      wrong_terminal,
      ExactDirectMorseEventRankTowerLinkDecision::
          no_link_arm_terminal_inconsistent,
      "a non-lower or non-strict arm terminal fails atomically");

  auto wrong_group = terminal_fixture();
  wrong_group.saddle_records[0U].atomic_group_index = 1U;
  expected_failure(
      wrong_group,
      ExactDirectMorseEventRankTowerLinkDecision::
          no_link_atomic_group_inconsistent,
      "a saddle outside its atomic post-batch group fails atomically");

  auto too_small = budget();
  too_small.maximum_link_count = 0U;
  const auto exhausted =
      build_exact_direct_morse_event_rank_tower_link_journal(
          terminal_fixture(), too_small);
  check(
      exhausted.decision ==
              ExactDirectMorseEventRankTowerLinkDecision::
                  no_link_budget_exhausted &&
          exhausted.certified_atomic_failure(),
      "budget preflight rejects output before publishing a partial link");
}

}  // namespace

int main() {
  test_terminal_rank_link_and_fresh_verification();
  test_singleton_terminal_has_no_order_zero_link();
  test_fresh_upstream_replay_rejects_coordinated_identity_substitutions();
  test_fail_closed_role_level_terminal_and_group_mutations();
  return failures == 0 ? 0 : 1;
}
