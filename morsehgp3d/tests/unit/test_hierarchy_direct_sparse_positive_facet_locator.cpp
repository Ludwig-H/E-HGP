#include "morsehgp3d/hierarchy/direct_sparse_positive_facet_locator.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using PointId = morsehgp3d::spatial::PointId;

constexpr std::uint64_t authority_id = UINT64_C(0x51a7);
int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] ExactDirectSparseFacetKey key(
    std::initializer_list<PointId> point_ids) {
  ExactDirectSparseFacetKey result;
  result.point_count = point_ids.size();
  std::size_t index = 0U;
  for (const PointId point_id : point_ids) {
    result.point_ids[index] = point_id;
    ++index;
  }
  return result;
}

[[nodiscard]] ExactDirectSparseFacetWitness witness(
    std::uint64_t replay_token) {
  return {authority_id, replay_token};
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocatorBudget generous_budget() {
  return {
      8U,
      32U,
      320U,
      32U,
      32U,
      32U,
      32U,
      32U,
      640U,
      65U,
      65U,
  };
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocator make_locator(
    std::uint64_t fingerprint_mask =
        std::numeric_limits<std::uint64_t>::max()) {
  return build_exact_direct_sparse_positive_facet_locator(
      8U,
      generous_budget(),
      {authority_id, fingerprint_mask});
}

void check_initialization_budget_and_external_scope() {
  const auto locator = make_locator();
  check(
      locator.certified_positive_locator() &&
          locator.required_table_slot_capacity() == 65U &&
          locator.required_batch_scratch_slot_capacity() == 65U &&
          locator.slots().size() == 65U &&
          locator.key_point_arena().empty() &&
          locator.component_parents() ==
              std::vector<ExactDirectSparseComponentHandle>(
                  {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U}) &&
          locator.scope() == ExactDirectSparsePositiveFacetLocatorScope::
                                 positive_bindings_relative_to_caller_asserted_external_authority_only,
      "initialization creates only dense handles, a flat empty table and an explicit external-authority scope");

  auto insufficient = generous_budget();
  insufficient.maximum_table_slot_count = 64U;
  const auto rejected = build_exact_direct_sparse_positive_facet_locator(
      8U, insufficient, {authority_id, ~std::uint64_t{0U}});
  check(
      !rejected.certified_positive_locator() && rejected.slots().empty() &&
          rejected.component_parents().empty() &&
          rejected.initialization_decision() ==
              ExactDirectSparsePositiveFacetLocatorInitializationDecision::
                  no_locator_budget_exhausted,
      "table capacity is preflighted before any durable arena is allocated");

  const auto no_authority =
      build_exact_direct_sparse_positive_facet_locator(
          8U, generous_budget(), {0U, ~std::uint64_t{0U}});
  check(
      !no_authority.certified_positive_locator() &&
          no_authority.initialization_decision() ==
              ExactDirectSparsePositiveFacetLocatorInitializationDecision::
                  no_locator_external_authority_rejected,
      "a locator without an external replay authority is rejected");
}

void check_strict_snapshot_and_unresolved_miss() {
  auto locator = make_locator();
  const ExactDirectSparseFacetKey facet = key({1U, 4U, 9U});
  const std::array<ExactDirectSparseFacetQuery, 1U> first_queries{{
      {0U, facet, witness(10U)},
  }};
  const std::array<ExactDirectSparseFacetBinding, 1U> first_bindings{{
      {0U, facet, 3U, witness(20U)},
  }};
  const auto first = locator.apply_batch(
      first_queries,
      std::span<const ExactDirectSparseComponentUnion>{},
      first_bindings);
  check(
      first.certified_committed_batch() && first.lookups.size() == 1U &&
          first.lookups[0].disposition ==
              ExactDirectSparseFacetLookupDisposition::unresolved &&
          !first.lookups[0].component_handle_present &&
          !first.lookups[0].source_binding_witness_present &&
          !first.missing_facet_means_isolated &&
          first.current_batch_bindings_hidden_from_lookups,
      "a binding staged in the current batch remains invisible and its miss is unresolved");

  const std::array<ExactDirectSparseFacetQuery, 2U> second_queries{{
      {0U, facet, witness(11U)},
      {1U, key({2U, 4U, 9U}), witness(12U)},
  }};
  const auto second = locator.apply_batch(
      second_queries,
      std::span<const ExactDirectSparseComponentUnion>{},
      std::span<const ExactDirectSparseFacetBinding>{});
  check(
      second.certified_committed_batch() && second.lookups.size() == 2U &&
          second.lookups[0].disposition ==
              ExactDirectSparseFacetLookupDisposition::positive &&
          second.lookups[0].pre_batch_component_handle == 3U &&
          second.lookups[0].query_witness == witness(11U) &&
          second.lookups[0].source_binding_witness == witness(20U) &&
          second.lookups[1].disposition ==
              ExactDirectSparseFacetLookupDisposition::unresolved &&
          second.counters.positive_lookup_count == 1U &&
          second.counters.unresolved_lookup_count == 1U,
      "the following batch sees the positive binding with both replay witnesses while an absent key stays unresolved");
}

void check_forced_fingerprint_collision_and_full_rank_keys() {
  auto locator = make_locator(0U);
  const ExactDirectSparseFacetKey first_key =
      key({0U, 2U, 4U, 6U, 8U, 10U, 12U, 14U, 16U, 18U});
  const ExactDirectSparseFacetKey second_key =
      key({1U, 3U, 5U, 7U, 9U, 11U, 13U, 15U, 17U, 19U});
  const std::array<ExactDirectSparseFacetBinding, 2U> bindings{{
      {0U, first_key, 2U, witness(30U)},
      {1U, second_key, 5U, witness(31U)},
  }};
  const auto inserted = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      bindings);
  check(
      inserted.certified_committed_batch() &&
          inserted.counters.inserted_binding_count == 2U &&
          locator.key_point_arena().size() == 20U,
      "two complete ten-PointId keys are packed in the durable flat arena");

  const std::array<ExactDirectSparseFacetQuery, 2U> queries{{
      {0U, second_key, witness(32U)},
      {1U, first_key, witness(33U)},
  }};
  const auto found = locator.apply_batch(
      queries,
      std::span<const ExactDirectSparseComponentUnion>{},
      std::span<const ExactDirectSparseFacetBinding>{});
  check(
      found.certified_committed_batch() &&
          found.lookups[0].pre_batch_component_handle == 5U &&
          found.lookups[1].pre_batch_component_handle == 2U &&
          found.counters.full_key_comparison_count >= 2U &&
          found.counters.equal_fingerprint_distinct_key_count >= 1U,
      "a zero test mask forces equal fingerprints but complete key comparison preserves both answers");
}

void check_union_indirection_without_key_rewrite() {
  auto locator = make_locator();
  const ExactDirectSparseFacetKey facet = key({7U, 8U, 12U, 40U});
  const std::array<ExactDirectSparseFacetBinding, 1U> binding{{
      {0U, facet, 3U, witness(40U)},
  }};
  const auto seeded = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      binding);
  check(seeded.certified_committed_batch(), "the union fixture is seeded");

  std::size_t occupied_index = locator.slots().size();
  for (std::size_t index = 0U; index < locator.slots().size(); ++index) {
    if (locator.slots()[index].occupied) {
      occupied_index = index;
      break;
    }
  }
  check(
      occupied_index < locator.slots().size(),
      "the seeded facet has one occupied flat slot");
  if (occupied_index >= locator.slots().size()) {
    return;
  }
  const ExactDirectSparsePositiveFacetSlot slot_before =
      locator.slots()[occupied_index];
  const std::vector<PointId> arena_before =
      locator.key_point_arena();

  const std::array<ExactDirectSparseFacetQuery, 1U> query_before_union{{
      {0U, facet, witness(41U)},
  }};
  const std::array<ExactDirectSparseComponentUnion, 1U> unions{{
      {0U, 3U, 1U, witness(42U)},
  }};
  const auto merged = locator.apply_batch(
      query_before_union,
      unions,
      std::span<const ExactDirectSparseFacetBinding>{});
  check(
      merged.certified_committed_batch() &&
          merged.lookups[0].pre_batch_component_handle == 3U &&
          locator.slots()[occupied_index] == slot_before &&
          locator.key_point_arena() == arena_before &&
          locator.component_parents()[3U] == 1U &&
          locator.committed_unions().size() == 1U &&
          locator.committed_unions()[0].witness == witness(42U),
      "same-batch lookup uses the old handle while the committed union changes only the indirect DSU arena");

  const auto after = locator.apply_batch(
      query_before_union,
      std::span<const ExactDirectSparseComponentUnion>{},
      std::span<const ExactDirectSparseFacetBinding>{});
  check(
      after.certified_committed_batch() &&
          after.lookups[0].pre_batch_component_handle == 1U &&
          locator.slots()[occupied_index].component_handle == 3U,
      "the next snapshot follows the union without rewriting the slot's original dense handle");
}

void check_incompatible_duplicate_is_atomic_contradiction() {
  auto locator = make_locator();
  const ExactDirectSparseFacetKey facet = key({3U, 11U});
  const std::array<ExactDirectSparseFacetBinding, 1U> seed{{
      {0U, facet, 0U, witness(50U)},
  }};
  const auto seeded = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      seed);
  check(seeded.certified_committed_batch(), "the contradiction fixture is seeded");
  const auto before = locator;

  const std::array<ExactDirectSparseComponentUnion, 1U> unrelated_union{{
      {0U, 2U, 3U, witness(51U)},
  }};
  const std::array<ExactDirectSparseFacetBinding, 1U> conflict{{
      {0U, facet, 1U, witness(52U)},
  }};
  const auto rejected = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      unrelated_union,
      conflict);
  check(
      rejected.decision == ExactDirectSparsePositiveFacetBatchDecision::
                               contradiction_incompatible_exact_facet_binding &&
          rejected.contradiction_detected &&
          !rejected.atomic_commit_performed &&
          !rejected.locator_state_mutated && locator == before,
      "an exact duplicate bound to incompatible post-union handles rejects the whole batch without committing even unrelated unions");
}

void check_explicit_union_makes_duplicate_compatible() {
  auto locator = make_locator();
  const ExactDirectSparseFacetKey facet = key({4U, 6U, 22U});
  const std::array<ExactDirectSparseFacetBinding, 1U> seed{{
      {0U, facet, 0U, witness(60U)},
  }};
  const auto seeded = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      seed);
  check(seeded.certified_committed_batch(), "the compatible fixture is seeded");
  const std::vector<PointId> arena_before =
      locator.key_point_arena();

  const std::array<ExactDirectSparseFacetQuery, 1U> queries{{
      {0U, facet, witness(61U)},
  }};
  const std::array<ExactDirectSparseComponentUnion, 1U> unions{{
      {0U, 0U, 1U, witness(62U)},
  }};
  const std::array<ExactDirectSparseFacetBinding, 1U> duplicate{{
      {0U, facet, 1U, witness(63U)},
  }};
  const auto accepted = locator.apply_batch(queries, unions, duplicate);
  check(
      accepted.certified_committed_batch() &&
          accepted.lookups[0].pre_batch_component_handle == 0U &&
          accepted.counters.inserted_binding_count == 0U &&
          accepted.counters.compatible_duplicate_binding_count == 1U &&
          accepted.explicit_unions_applied_before_binding_compatibility &&
          locator.key_point_arena() == arena_before &&
          locator.counters().inserted_binding_count == 1U &&
          locator.counters().compatible_duplicate_binding_count == 1U,
      "an explicit same-batch union makes an exact duplicate compatible without duplicating its key arena slice");
}

void check_shape_witness_and_durable_budget_rejections() {
  auto locator = make_locator();
  ExactDirectSparseFacetKey malformed = key({1U, 2U});
  malformed.point_ids[8U] = 99U;
  const std::array<ExactDirectSparseFacetQuery, 1U> malformed_query{{
      {0U, malformed, witness(70U)},
  }};
  const auto before_shape = locator;
  const auto shape_rejected = locator.apply_batch(
      malformed_query,
      std::span<const ExactDirectSparseComponentUnion>{},
      std::span<const ExactDirectSparseFacetBinding>{});
  check(
      shape_rejected.decision ==
              ExactDirectSparsePositiveFacetBatchDecision::
                  no_positive_locator_input_shape_rejected &&
          locator == before_shape,
      "non-zero unused key tail data is rejected as non-canonical without mutation");

  const std::array<ExactDirectSparseFacetQuery, 1U> null_witness_query{{
      {0U, key({1U}), {authority_id, 0U}},
  }};
  const auto witness_rejected = locator.apply_batch(
      null_witness_query,
      std::span<const ExactDirectSparseComponentUnion>{},
      std::span<const ExactDirectSparseFacetBinding>{});
  check(
      witness_rejected.decision ==
              ExactDirectSparsePositiveFacetBatchDecision::
                  no_positive_locator_external_witness_rejected &&
          locator == before_shape,
      "a null replay token is rejected without mutation");

  auto tight_budget = generous_budget();
  tight_budget.maximum_committed_binding_count = 2U;
  tight_budget.maximum_committed_key_point_count = 3U;
  tight_budget.maximum_table_slot_count = 5U;
  auto tight = build_exact_direct_sparse_positive_facet_locator(
      8U, tight_budget, {authority_id, ~std::uint64_t{0U}});
  const std::array<ExactDirectSparseFacetBinding, 1U> first{{
      {0U, key({1U, 2U}), 0U, witness(71U)},
  }};
  check(
      tight.apply_batch(
               std::span<const ExactDirectSparseFacetQuery>{},
               std::span<const ExactDirectSparseComponentUnion>{},
               first)
          .certified_committed_batch(),
      "the tight durable arena accepts its first canonical key");
  const auto before_budget = tight;
  const std::array<ExactDirectSparseFacetBinding, 1U> second{{
      {0U, key({3U, 4U}), 1U, witness(72U)},
  }};
  const auto budget_rejected = tight.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      second);
  check(
      budget_rejected.decision ==
              ExactDirectSparsePositiveFacetBatchDecision::
                  no_positive_locator_budget_exhausted &&
          tight == before_budget && tight.key_point_arena().size() == 2U,
      "durable key-point capacity is checked after exact duplicate analysis and before atomic commit");
}

void check_const_budgeted_probe() {
  auto locator = make_locator(0U);
  const ExactDirectSparseFacetKey collision_key =
      key({1U, 3U, 5U, 7U});
  const ExactDirectSparseFacetKey target_key =
      key({2U, 4U, 6U, 8U});
  const ExactDirectSparseFacetKey missing_key =
      key({9U, 10U, 11U, 12U});
  const std::array<ExactDirectSparseFacetBinding, 2U> bindings{{
      {0U, collision_key, 2U, witness(90U)},
      {1U, target_key, 5U, witness(91U)},
  }};
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              bindings)
          .certified_committed_batch(),
      "the const-probe fixture stores two deliberately colliding keys");
  const std::array<ExactDirectSparseComponentUnion, 1U> unions{{
      {0U, 5U, 1U, witness(92U)},
  }};
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              unions,
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch(),
      "the const-probe fixture redirects the target handle through one DSU parent edge");

  const ExactDirectSparsePositiveFacetLocator durable_state_before = locator;
  const ExactDirectSparsePositiveFacetLocatorCounters counters_before =
      locator.counters();
  const std::size_t committed_batch_count_before =
      locator.committed_batches().size();
  const auto* const slots_data_before = locator.slots().data();
  const auto* const key_arena_data_before =
      locator.key_point_arena().data();
  const auto* const parents_data_before =
      locator.component_parents().data();
  const auto* const unions_data_before = locator.committed_unions().data();
  const auto* const batches_data_before = locator.committed_batches().data();
  const std::size_t slots_capacity_before = locator.slots().capacity();
  const std::size_t key_arena_capacity_before =
      locator.key_point_arena().capacity();
  const std::size_t parents_capacity_before =
      locator.component_parents().capacity();
  const std::size_t unions_capacity_before =
      locator.committed_unions().capacity();
  const std::size_t batches_capacity_before =
      locator.committed_batches().capacity();
  const auto& read_only_locator =
      static_cast<const ExactDirectSparsePositiveFacetLocator&>(locator);

  const auto hit = read_only_locator.probe_positive_facet(
      target_key, witness(93U), {2U, 1U});
  check(
      hit.certified_positive_hit() &&
          hit.query_key == target_key &&
          hit.disposition ==
              ExactDirectSparsePositiveFacetProbeDisposition::positive &&
          hit.component_handle_present && hit.component_handle == 1U &&
          hit.source_binding_witness_present &&
          hit.source_binding_witness == witness(91U) &&
          hit.slot_visit_count == 2U &&
          hit.component_parent_hop_count == 1U &&
          hit.full_key_comparison_count == 2U &&
          hit.equal_fingerprint_distinct_key_count == 1U &&
          !hit.locator_state_mutated && !hit.batch_committed,
      "a const probe compares complete colliding keys and follows the committed union under separate exact budgets");
  const auto hit_verified =
      verify_exact_direct_sparse_positive_facet_probe(
          read_only_locator,
          target_key,
          witness(93U),
          {2U, 1U},
          hit);
  check(
      hit_verified.locator_certified_at_entry &&
          hit_verified.query_key_bound_to_observed_result &&
          hit_verified.query_witness_bound_to_observed_result &&
          hit_verified.budget_bound_to_observed_result &&
          hit_verified.outcome_contract_certified &&
          hit_verified.exact_fresh_probe_replay_certified &&
          hit_verified.no_locator_mutation_or_batch_commit &&
          !hit_verified.external_authority_replayed_by_locator &&
          hit_verified.relative_external_authority_scope_preserved &&
          hit_verified.result_certified,
      "the public fresh verifier binds a positive result to the exact locator, key, witness and budget without claiming external-authority replay");

  const auto slot_exhausted = read_only_locator.probe_positive_facet(
      target_key, witness(94U), {1U, 1U});
  check(
      slot_exhausted.certified_budget_exhaustion() &&
          slot_exhausted.query_key == target_key &&
          slot_exhausted.slot_visit_budget_exhausted &&
          !slot_exhausted.component_parent_hop_budget_exhausted &&
          slot_exhausted.slot_visit_count == 1U &&
          slot_exhausted.full_key_comparison_count == 1U &&
          slot_exhausted.equal_fingerprint_distinct_key_count == 1U &&
          !slot_exhausted.slot_search_completed &&
          slot_exhausted.disposition ==
              ExactDirectSparsePositiveFacetProbeDisposition::
                  budget_exhausted &&
          !slot_exhausted.component_handle_present &&
          slot_exhausted.component_handle == 0U &&
          slot_exhausted.source_binding_witness ==
              ExactDirectSparseFacetWitness{} &&
          !slot_exhausted.missing_facet_means_isolated,
      "one slot visit is just insufficient for the second colliding key and is reported as budget exhaustion, never as absence");

  const auto parent_exhausted = read_only_locator.probe_positive_facet(
      target_key, witness(95U), {2U, 0U});
  check(
      parent_exhausted.certified_budget_exhaustion() &&
          parent_exhausted.query_key == target_key &&
          !parent_exhausted.slot_visit_budget_exhausted &&
          parent_exhausted.component_parent_hop_budget_exhausted &&
          parent_exhausted.slot_search_completed &&
          !parent_exhausted.component_find_completed &&
          parent_exhausted.component_parent_hop_count == 0U &&
          parent_exhausted.full_key_comparison_count == 2U &&
          parent_exhausted.equal_fingerprint_distinct_key_count == 1U &&
          parent_exhausted.disposition ==
              ExactDirectSparsePositiveFacetProbeDisposition::
                  budget_exhausted &&
          !parent_exhausted.component_handle_present &&
          parent_exhausted.component_handle == 0U &&
          !parent_exhausted.source_binding_witness_present &&
          parent_exhausted.source_binding_witness ==
              ExactDirectSparseFacetWitness{},
      "zero parent hops is just insufficient after the full key hit and cannot produce a partial positive answer");

  const auto miss = read_only_locator.probe_positive_facet(
      missing_key, witness(96U), {3U, 0U});
  check(
      miss.certified_unresolved_miss() &&
          miss.query_key == missing_key &&
          miss.disposition ==
              ExactDirectSparsePositiveFacetProbeDisposition::unresolved &&
          miss.slot_visit_count == 3U &&
          miss.full_key_comparison_count == 2U &&
          miss.equal_fingerprint_distinct_key_count == 2U &&
          miss.slot_search_completed && !miss.component_find_completed &&
          !miss.component_handle_present &&
          !miss.missing_facet_means_isolated &&
          !miss.total_facet_authority_claimed,
      "only the visited empty terminator certifies a complete unresolved miss across forced collisions");

  const auto miss_verified =
      verify_exact_direct_sparse_positive_facet_probe(
          read_only_locator,
          missing_key,
          witness(96U),
          {3U, 0U},
          miss);
  const auto slot_exhausted_verified =
      verify_exact_direct_sparse_positive_facet_probe(
          read_only_locator,
          target_key,
          witness(94U),
          {1U, 1U},
          slot_exhausted);
  const auto parent_exhausted_verified =
      verify_exact_direct_sparse_positive_facet_probe(
          read_only_locator,
          target_key,
          witness(95U),
          {2U, 0U},
          parent_exhausted);
  check(
      miss_verified.result_certified &&
          slot_exhausted_verified.result_certified &&
          parent_exhausted_verified.result_certified &&
          !miss_verified.external_authority_replayed_by_locator &&
          !slot_exhausted_verified.external_authority_replayed_by_locator &&
          !parent_exhausted_verified.external_authority_replayed_by_locator,
      "fresh replay certifies complete misses and both fail-closed exhaustion modes only inside the relative locator domain");

  auto mutated_key_result = hit;
  mutated_key_result.query_key = collision_key;
  const auto mutated_key_rejected =
      verify_exact_direct_sparse_positive_facet_probe(
          read_only_locator,
          target_key,
          witness(93U),
          {2U, 1U},
          mutated_key_result);
  check(
      mutated_key_result.certified_positive_hit() &&
          !mutated_key_rejected.query_key_bound_to_observed_result &&
          !mutated_key_rejected.exact_fresh_probe_replay_certified &&
          !mutated_key_rejected.result_certified,
      "a canonical but substituted observed query key requires and fails the locator-bound fresh replay");

  auto mutated_audit_result = hit;
  ++mutated_audit_result.full_key_comparison_count;
  const auto mutated_audit_rejected =
      verify_exact_direct_sparse_positive_facet_probe(
          read_only_locator,
          target_key,
          witness(93U),
          {2U, 1U},
          mutated_audit_result);
  check(
      !mutated_audit_result.certified_positive_hit() &&
          !mutated_audit_rejected.outcome_contract_certified &&
          !mutated_audit_rejected.exact_fresh_probe_replay_certified &&
          !mutated_audit_rejected.result_certified,
      "the exact hit relation full comparisons equals collision mismatches plus one rejects a mutated audit");

  auto mutated_miss_audit_result = miss;
  ++mutated_miss_audit_result.full_key_comparison_count;
  const auto mutated_miss_audit_rejected =
      verify_exact_direct_sparse_positive_facet_probe(
          read_only_locator,
          missing_key,
          witness(96U),
          {3U, 0U},
          mutated_miss_audit_result);
  check(
      !mutated_miss_audit_result.certified_unresolved_miss() &&
          !mutated_miss_audit_rejected.outcome_contract_certified &&
          !mutated_miss_audit_rejected.result_certified,
      "the exact complete-miss relation full comparisons equals collision mismatches rejects a mutated audit");

  auto mutated_exhaustion_audit_result = slot_exhausted;
  ++mutated_exhaustion_audit_result.full_key_comparison_count;
  const auto mutated_exhaustion_audit_rejected =
      verify_exact_direct_sparse_positive_facet_probe(
          read_only_locator,
          target_key,
          witness(94U),
          {1U, 1U},
          mutated_exhaustion_audit_result);
  check(
      !mutated_exhaustion_audit_result.certified_budget_exhaustion() &&
          !mutated_exhaustion_audit_rejected.outcome_contract_certified &&
          !mutated_exhaustion_audit_rejected.result_certified,
      "a slot-budget exhaustion keeps the exact miss-prefix counter relation and rejects a mutated audit");

  auto mutated_handle_result = hit;
  mutated_handle_result.component_handle = 7U;
  const auto mutated_handle_rejected =
      verify_exact_direct_sparse_positive_facet_probe(
          read_only_locator,
          target_key,
          witness(93U),
          {2U, 1U},
          mutated_handle_result);
  check(
      mutated_handle_result.certified_positive_hit() &&
          !mutated_handle_rejected.exact_fresh_probe_replay_certified &&
          !mutated_handle_rejected.result_certified,
      "a well-shaped but substituted component handle is rejected by fresh replay against the locator DSU");

  auto leaked_exhaustion_result = parent_exhausted;
  leaked_exhaustion_result.component_handle = 7U;
  const auto leaked_exhaustion_rejected =
      verify_exact_direct_sparse_positive_facet_probe(
          read_only_locator,
          target_key,
          witness(95U),
          {2U, 0U},
          leaked_exhaustion_result);
  check(
      !leaked_exhaustion_result.certified_budget_exhaustion() &&
          !leaked_exhaustion_rejected.outcome_contract_certified &&
          !leaked_exhaustion_rejected.result_certified,
      "an exhausted parent traversal rejects even a hidden component-handle payload");

  ExactDirectSparseFacetKey malformed = target_key;
  malformed.point_ids[9U] = 99U;
  const auto malformed_rejected = read_only_locator.probe_positive_facet(
      malformed, witness(97U), {3U, 1U});
  const auto witness_rejected = read_only_locator.probe_positive_facet(
      target_key, {authority_id, 0U}, {3U, 1U});
  check(
      malformed_rejected.decision ==
              ExactDirectSparsePositiveFacetProbeDecision::
                  no_positive_locator_input_shape_rejected &&
          malformed_rejected.slot_visit_count == 0U &&
          witness_rejected.decision ==
              ExactDirectSparsePositiveFacetProbeDecision::
                  no_positive_locator_external_witness_rejected &&
          witness_rejected.slot_visit_count == 0U,
      "the const probe validates its canonical key and external witness before spending either work budget");

  check(
      locator == durable_state_before &&
          locator.counters() == counters_before &&
          locator.committed_batches().size() ==
              committed_batch_count_before &&
          locator.slots().data() == slots_data_before &&
          locator.key_point_arena().data() == key_arena_data_before &&
          locator.component_parents().data() == parents_data_before &&
          locator.committed_unions().data() == unions_data_before &&
          locator.committed_batches().data() == batches_data_before &&
          locator.slots().capacity() == slots_capacity_before &&
          locator.key_point_arena().capacity() ==
              key_arena_capacity_before &&
          locator.component_parents().capacity() ==
              parents_capacity_before &&
          locator.committed_unions().capacity() ==
              unions_capacity_before &&
          locator.committed_batches().capacity() ==
              batches_capacity_before,
      "all successful, missing, rejected and exhausted const probes leave every durable value, counter, batch and arena identity unchanged");
}

void check_fresh_durable_structure_verifier_and_mutations() {
  auto locator = make_locator(0U);
  const ExactDirectSparseFacetKey first = key({1U, 5U, 9U});
  const ExactDirectSparseFacetKey second = key({2U, 6U, 10U});
  const std::array<ExactDirectSparseFacetBinding, 2U> bindings{{
      {0U, first, 3U, witness(80U)},
      {1U, second, 4U, witness(81U)},
  }};
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              bindings)
          .certified_committed_batch(),
      "the structural-verifier fixture stores two colliding keys");
  const std::array<ExactDirectSparseFacetQuery, 1U> queries{{
      {0U, first, witness(82U)},
  }};
  const std::array<ExactDirectSparseComponentUnion, 1U> unions{{
      {0U, 3U, 1U, witness(83U)},
  }};
  check(
      locator
          .apply_batch(
              queries,
              unions,
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch(),
      "the structural-verifier fixture stores one witnessed union and query batch");

  const auto verified =
      verify_exact_direct_sparse_positive_facet_locator_structure(
          8U, locator.budget(), locator.config(), locator.state_view());
  check(
      verified.trusted_construction_parameters_certified &&
          verified.capacity_requirements_certified &&
          verified.flat_table_and_key_arena_certified &&
          verified.every_fingerprint_recomputed_and_full_key_located &&
          verified.dense_handle_dsu_replay_certified &&
          verified.union_witness_structure_certified &&
          verified
              .historical_batch_assertions_and_counters_well_formed &&
          verified.internal_fact_fields_match_contract &&
          verified.decision_and_scope_certified &&
          !verified.external_authority_replayed_by_locator &&
          verified
              .bounded_temporary_scratch_without_second_durable_output &&
          verified.fresh_durable_structure_verification_certified &&
          verified.result_certified &&
          verified.table_slot_scan_count == locator.slots().size() &&
          verified.key_point_scan_count ==
              locator.key_point_arena().size(),
      "fresh structural verification replays the DSU and audits every flat key while leaving caller authority unreplayed");

  std::vector<ExactDirectSparsePositiveFacetSlot> mutated_slots =
      locator.slots();
  for (ExactDirectSparsePositiveFacetSlot& slot : mutated_slots) {
    if (slot.occupied) {
      slot.fingerprint ^= UINT64_C(1);
      break;
    }
  }
  auto mutated_slot_view = locator.state_view();
  mutated_slot_view.slots =
      std::span<const ExactDirectSparsePositiveFacetSlot>{mutated_slots};
  const auto slot_rejected =
      verify_exact_direct_sparse_positive_facet_locator_structure(
          8U, locator.budget(), locator.config(), mutated_slot_view);
  check(
      !slot_rejected.every_fingerprint_recomputed_and_full_key_located &&
          !slot_rejected.fresh_durable_structure_verification_certified &&
          !slot_rejected.result_certified,
      "fresh structural verification rejects a mutated stored fingerprint");

  std::vector<ExactDirectSparseComponentHandle> mutated_parents =
      locator.component_parents();
  mutated_parents[3U] = 3U;
  auto mutated_parent_view = locator.state_view();
  mutated_parent_view.component_parents =
      std::span<const ExactDirectSparseComponentHandle>{mutated_parents};
  const auto parent_rejected =
      verify_exact_direct_sparse_positive_facet_locator_structure(
          8U, locator.budget(), locator.config(), mutated_parent_view);
  check(
      !parent_rejected.dense_handle_dsu_replay_certified &&
          !parent_rejected.result_certified,
      "fresh structural verification rejects a parent arena inconsistent with union replay");

  std::vector<ExactDirectSparseCommittedBatchRecord> mutated_batches =
      locator.committed_batches();
  mutated_batches[0U].counters.binding_request_count =
      locator.budget().maximum_batch_binding_count + 1U;
  auto mutated_batch_view = locator.state_view();
  mutated_batch_view.committed_batches =
      std::span<const ExactDirectSparseCommittedBatchRecord>{mutated_batches};
  const auto batch_rejected =
      verify_exact_direct_sparse_positive_facet_locator_structure(
          8U, locator.budget(), locator.config(), mutated_batch_view);
  check(
      !batch_rejected
           .historical_batch_assertions_and_counters_well_formed &&
          !batch_rejected.result_certified,
      "structural verification rejects an historical counter beyond its trusted per-batch cap");

  std::vector<ExactDirectSparsePositiveFacetSlot> oversized_slots(
      locator.slots().size() + 1U);
  auto oversized_view = locator.state_view();
  oversized_view.slots =
      std::span<const ExactDirectSparsePositiveFacetSlot>{oversized_slots};
  const auto oversized_rejected =
      verify_exact_direct_sparse_positive_facet_locator_structure(
          8U, locator.budget(), locator.config(), oversized_view);
  check(
      !oversized_rejected.capacity_requirements_certified &&
          oversized_rejected.table_slot_scan_count == 0U &&
          !oversized_rejected.result_certified,
      "untrusted span sizes fail before scans or scratch allocations derived from them");

  auto empty_domain = make_locator();
  const std::array<ExactDirectSparseFacetQuery, 1U> empty_query{{
      {0U, key({50U, 51U}), witness(84U)},
  }};
  check(
      empty_domain
          .apply_batch(
              empty_query,
              std::span<const ExactDirectSparseComponentUnion>{},
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch(),
      "the prefix-invariant fixture commits one unresolved empty-domain query");
  std::vector<ExactDirectSparseCommittedBatchRecord> forged_hit_batches =
      empty_domain.committed_batches();
  forged_hit_batches[0U].counters.positive_lookup_count = 1U;
  forged_hit_batches[0U].counters.unresolved_lookup_count = 0U;
  auto forged_hit_view = empty_domain.state_view();
  forged_hit_view.committed_batches =
      std::span<const ExactDirectSparseCommittedBatchRecord>{
          forged_hit_batches};
  forged_hit_view.counters.positive_lookup_count = 1U;
  forged_hit_view.counters.unresolved_lookup_count = 0U;
  const auto forged_hit_rejected =
      verify_exact_direct_sparse_positive_facet_locator_structure(
          8U,
          empty_domain.budget(),
          empty_domain.config(),
          forged_hit_view);
  check(
      !forged_hit_rejected
           .historical_batch_assertions_and_counters_well_formed &&
          !forged_hit_rejected.result_certified,
      "a structurally impossible positive lookup before the first binding is rejected even when aggregate counters agree");

  std::vector<ExactDirectSparseCommittedBatchRecord>
      forged_duplicate_batches = empty_domain.committed_batches();
  forged_duplicate_batches[0U].counters.binding_request_count = 1U;
  forged_duplicate_batches[0U].counters.compatible_duplicate_binding_count =
      1U;
  forged_duplicate_batches[0U].counters.full_key_comparison_count = 1U;
  auto forged_duplicate_view = empty_domain.state_view();
  forged_duplicate_view.committed_batches =
      std::span<const ExactDirectSparseCommittedBatchRecord>{
          forged_duplicate_batches};
  forged_duplicate_view.counters.binding_request_count = 1U;
  forged_duplicate_view.counters.compatible_duplicate_binding_count = 1U;
  forged_duplicate_view.counters.full_key_comparison_count = 1U;
  const auto forged_duplicate_rejected =
      verify_exact_direct_sparse_positive_facet_locator_structure(
          8U,
          empty_domain.budget(),
          empty_domain.config(),
          forged_duplicate_view);
  check(
      !forged_duplicate_rejected
           .historical_batch_assertions_and_counters_well_formed &&
          !forged_duplicate_rejected.result_certified,
      "a compatible duplicate without a prior or current binding is rejected even when aggregate counters agree");

  auto comparison_lower_bound = make_locator();
  const ExactDirectSparseFacetKey comparison_key = key({60U, 61U});
  const std::array<ExactDirectSparseFacetBinding, 1U> comparison_binding{{
      {0U, comparison_key, 0U, witness(85U)},
  }};
  check(
      comparison_lower_bound
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              comparison_binding)
          .certified_committed_batch(),
      "the comparison-lower-bound fixture stores one key");
  const std::array<ExactDirectSparseFacetQuery, 1U> comparison_query{{
      {0U, comparison_key, witness(86U)},
  }};
  check(
      comparison_lower_bound
          .apply_batch(
              comparison_query,
              std::span<const ExactDirectSparseComponentUnion>{},
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch(),
      "the comparison-lower-bound fixture records one positive lookup");
  std::vector<ExactDirectSparseCommittedBatchRecord>
      forged_comparison_batches = comparison_lower_bound.committed_batches();
  forged_comparison_batches[1U].counters.full_key_comparison_count = 0U;
  auto forged_comparison_view = comparison_lower_bound.state_view();
  forged_comparison_view.committed_batches =
      std::span<const ExactDirectSparseCommittedBatchRecord>{
          forged_comparison_batches};
  forged_comparison_view.counters.full_key_comparison_count = 0U;
  const auto forged_comparison_rejected =
      verify_exact_direct_sparse_positive_facet_locator_structure(
          8U,
          comparison_lower_bound.budget(),
          comparison_lower_bound.config(),
          forged_comparison_view);
  check(
      !forged_comparison_rejected
           .historical_batch_assertions_and_counters_well_formed &&
          !forged_comparison_rejected.result_certified,
      "a positive lookup without its mandatory full-key comparison is rejected");
}

}  // namespace

int main() {
  check_initialization_budget_and_external_scope();
  check_strict_snapshot_and_unresolved_miss();
  check_forced_fingerprint_collision_and_full_rank_keys();
  check_union_indirection_without_key_rewrite();
  check_incompatible_duplicate_is_atomic_contradiction();
  check_explicit_union_makes_duplicate_compatible();
  check_shape_witness_and_durable_budget_rejections();
  check_const_budgeted_probe();
  check_fresh_durable_structure_verifier_and_mutations();
  if (failures != 0) {
    std::cerr << failures << " direct sparse positive facet test(s) failed\n";
    return 1;
  }
  std::cout << "direct sparse positive facet locator tests passed\n";
  return 0;
}
