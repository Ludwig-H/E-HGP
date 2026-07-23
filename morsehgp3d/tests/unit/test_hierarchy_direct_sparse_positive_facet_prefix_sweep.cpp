#include "morsehgp3d/hierarchy/direct_sparse_positive_facet_prefix_sweep.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
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

[[nodiscard]] ExactDirectSparsePositiveFacetLocatorBudget locator_budget() {
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

[[nodiscard]] ExactDirectSparsePositiveFacetLocator make_empty_locator() {
  return build_exact_direct_sparse_positive_facet_locator(
      8U, locator_budget(), {authority_id, 0U});
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocator make_hostile_locator() {
  auto locator = make_empty_locator();
  const ExactDirectSparseFacetKey facet_a = key({1U, 3U, 5U, 7U});
  const ExactDirectSparseFacetKey facet_b = key({2U, 4U, 6U, 8U});
  const ExactDirectSparseFacetKey facet_c = key({9U, 10U, 11U, 12U});

  const std::array<ExactDirectSparseFacetBinding, 1U> first_binding{{
      {0U, facet_a, 5U, witness(100U)},
  }};
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              first_binding)
          .certified_committed_batch(),
      "hostile batch 0 inserts A on handle 5");

  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch(),
      "hostile batch 1 is an accepted empty commit");

  const std::array<ExactDirectSparseComponentUnion, 1U> first_union{{
      {0U, 5U, 4U, witness(101U)},
  }};
  const std::array<ExactDirectSparseFacetBinding, 1U> second_binding{{
      {0U, facet_b, 2U, witness(102U)},
  }};
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              first_union,
              second_binding)
          .certified_committed_batch(),
      "hostile batch 2 redirects A to 4 and inserts B");

  const std::array<ExactDirectSparseComponentUnion, 1U> second_union{{
      {0U, 4U, 1U, witness(103U)},
  }};
  const std::array<ExactDirectSparseFacetBinding, 1U>
      compatible_duplicate{{
          {0U, facet_a, 1U, witness(104U)},
      }};
  const auto duplicate_commit = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      second_union,
      compatible_duplicate);
  check(
      duplicate_commit.certified_committed_batch() &&
          duplicate_commit.counters.inserted_binding_count == 0U &&
          duplicate_commit.counters.compatible_duplicate_binding_count == 1U,
      "hostile batch 3 applies its union before accepting A as a duplicate");

  const std::array<ExactDirectSparseComponentUnion, 1U> redundant_union{{
      {0U, 5U, 1U, witness(105U)},
  }};
  const std::array<ExactDirectSparseFacetBinding, 1U> third_binding{{
      {0U, facet_c, 3U, witness(106U)},
  }};
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              redundant_union,
              third_binding)
          .certified_committed_batch(),
      "hostile batch 4 records a redundant path traversal and inserts C");

  const std::array<ExactDirectSparseComponentUnion, 1U> final_union{{
      {0U, 2U, 1U, witness(107U)},
  }};
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              final_union,
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch(),
      "hostile batch 5 redirects B to 1");

  check(
      locator.committed_batches().size() == 6U &&
          locator.committed_unions().size() == 4U &&
          locator.counters().inserted_binding_count == 3U,
      "the hostile locator has exactly six batches, four union records and three unique bindings");
  return locator;
}

[[nodiscard]] std::vector<ExactDirectSparsePositiveFacetPrefixQuery>
hostile_queries() {
  const ExactDirectSparseFacetKey facet_a = key({1U, 3U, 5U, 7U});
  const ExactDirectSparseFacetKey facet_b = key({2U, 4U, 6U, 8U});
  const ExactDirectSparseFacetKey facet_c = key({9U, 10U, 11U, 12U});
  const ExactDirectSparseFacetKey missing = key({13U, 14U, 15U, 16U});
  return {
      {0U, 0U, facet_a},
      {1U, 1U, facet_a},
      {2U, 1U, facet_b},
      {3U, 2U, facet_a},
      {4U, 2U, facet_b},
      {5U, 3U, facet_a},
      {6U, 3U, facet_b},
      {7U, 3U, facet_c},
      {8U, 4U, facet_a},
      {9U, 5U, facet_c},
      {10U, 5U, missing},
      {11U, 6U, facet_b},
  };
}

[[nodiscard]] ExactDirectSparsePositiveFacetPrefixSweepBudget exact_budget() {
  return {
      12U,
      48U,
      8U,
      12U,
      4U,
      2U,
      23U,
      4U,
      12U,
      {4U, 2U},
  };
}

[[nodiscard]] ExactDirectSparsePositiveFacetPrefixSweepBudget
generous_sweep_budget() {
  return {
      64U,
      640U,
      8U,
      64U,
      32U,
      64U,
      512U,
      256U,
      64U,
      {65U, 8U},
  };
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocatorStructuralVerificationBudget
exact_locator_verification_budget(
    const ExactDirectSparsePositiveFacetLocator& locator) {
  ExactDirectSparsePositiveFacetLocatorStructuralVerificationBudget budget{
      locator.slots().size(),
      locator.key_point_arena().size(),
      locator.component_parents().size(),
      locator.committed_unions().size(),
      locator.committed_batches().size(),
      locator.counters().inserted_binding_count,
      locator.key_point_arena().size(),
      locator.slots().size(),
      locator.component_parents().size(),
      std::numeric_limits<std::size_t>::max(),
      std::numeric_limits<std::size_t>::max(),
      std::numeric_limits<std::size_t>::max(),
      std::numeric_limits<std::size_t>::max(),
  };
  const auto accounted =
      verify_exact_direct_sparse_positive_facet_locator_structure(
          locator.component_parents().size(),
          locator.budget(),
          locator.config(),
          budget,
          locator.state_view());
  check(
      accounted.result_certified,
      "the fixture locator admits one finite structural-verification accounting pass");
  budget.maximum_table_slot_count = accounted.required_table_slot_count;
  budget.maximum_key_point_count = accounted.required_key_point_count;
  budget.maximum_component_parent_count =
      accounted.required_component_parent_count;
  budget.maximum_union_record_count = accounted.required_union_record_count;
  budget.maximum_batch_record_count = accounted.required_batch_record_count;
  budget.maximum_binding_scratch_entry_count =
      accounted.required_binding_scratch_entry_count;
  budget.maximum_key_point_scratch_entry_count =
      accounted.required_key_point_scratch_entry_count;
  budget.maximum_table_slot_scratch_entry_count =
      accounted.required_table_slot_scratch_entry_count;
  budget.maximum_component_parent_scratch_entry_count =
      accounted.required_component_parent_scratch_entry_count;
  budget.maximum_temporary_scratch_byte_count =
      accounted.required_temporary_scratch_byte_count;
  budget.maximum_fingerprint_search_slot_visit_count =
      accounted.fingerprint_search_slot_visit_count;
  budget.maximum_insertion_chronology_slot_visit_count =
      accounted.insertion_chronology_slot_visit_count;
  budget.maximum_union_parent_hop_count =
      accounted.union_parent_hop_count;
  return budget;
}

void check_latent(
    const ExactDirectSparsePositiveFacetPrefixSweepResult& result,
    std::size_t index,
    std::size_t prefix,
    const std::string& message) {
  const auto& resolution = result.resolutions[index];
  check(
      resolution.query_index == index &&
          resolution.committed_batch_prefix_count == prefix &&
          resolution.disposition ==
              ExactDirectSparsePositiveFacetPrefixDisposition::
                  latent_unresolved &&
          !resolution.component_handle_present &&
          resolution.component_handle == 0U &&
          !resolution.source_binding_witness_present &&
          resolution.source_binding_witness ==
              ExactDirectSparseFacetWitness{},
      message);
}

void check_positive(
    const ExactDirectSparsePositiveFacetPrefixSweepResult& result,
    std::size_t index,
    std::size_t prefix,
    ExactDirectSparseComponentHandle root,
    const ExactDirectSparseFacetWitness& source_witness,
    const std::string& message) {
  const auto& resolution = result.resolutions[index];
  check(
      resolution.query_index == index &&
          resolution.committed_batch_prefix_count == prefix &&
          resolution.disposition ==
              ExactDirectSparsePositiveFacetPrefixDisposition::
                  relative_positive &&
          resolution.component_handle_present &&
          resolution.component_handle == root &&
          resolution.source_binding_witness_present &&
          resolution.source_binding_witness == source_witness,
      message);
}

void check_hostile_prefix_oracles_and_read_only_state() {
  auto locator = make_hostile_locator();
  const auto queries = hostile_queries();
  const auto budget = exact_budget();
  const auto durable_before = locator;
  const auto stamp_before = locator.snapshot_stamp();
  const auto counters_before = locator.counters();
  const auto* const slots_data_before = locator.slots().data();
  const auto* const keys_data_before = locator.key_point_arena().data();
  const auto* const parents_data_before = locator.component_parents().data();
  const auto* const unions_data_before = locator.committed_unions().data();
  const auto* const batches_data_before = locator.committed_batches().data();
  const std::size_t slots_capacity_before = locator.slots().capacity();
  const std::size_t keys_capacity_before =
      locator.key_point_arena().capacity();
  const std::size_t parents_capacity_before =
      locator.component_parents().capacity();
  const std::size_t unions_capacity_before =
      locator.committed_unions().capacity();
  const std::size_t batches_capacity_before =
      locator.committed_batches().capacity();

  const auto result = build_exact_direct_sparse_positive_facet_prefix_sweep(
      queries, witness(900U), locator, budget);
  check(
      result.certified_partial_refinement() && result.certified_outcome() &&
          result.decision ==
              ExactDirectSparsePositiveFacetPrefixSweepDecision::
                  complete_certified_positive_facet_prefix_sweep &&
          result.resolutions.size() == queries.size(),
      "the hostile monotone prefix sweep is a complete certified relative result");

  check_latent(
      result,
      0U,
      0U,
      "A is hidden at prefix zero even though its final slot is occupied");
  check_positive(result, 1U, 1U, 5U, witness(100U), "A appears at prefix one on root 5");
  check_latent(
      result,
      2U,
      1U,
      "the future B slot terminates its historical miss at prefix one");
  check_positive(
      result,
      3U,
      2U,
      5U,
      witness(100U),
      "the empty second batch leaves A on root 5");
  check_latent(
      result,
      4U,
      2U,
      "the empty second batch leaves B latent");
  check_positive(
      result,
      5U,
      3U,
      4U,
      witness(100U),
      "the first historical union redirects A to root 4");
  check_positive(result, 6U, 3U, 2U, witness(102U), "B appears at prefix three on root 2");
  check_latent(
      result,
      7U,
      3U,
      "the future C slot terminates its historical miss at prefix three");
  check_positive(
      result,
      8U,
      4U,
      1U,
      witness(100U),
      "A follows 5 to 4 to 1 while retaining its original binding witness");
  check_positive(result, 9U, 5U, 3U, witness(106U), "C appears at prefix five on root 3");
  check_latent(
      result,
      10U,
      5U,
      "a truly missing key terminates only on the actual empty slot");
  check_positive(
      result,
      11U,
      6U,
      1U,
      witness(102U),
      "the final historical union redirects B to root 1");

  check(
      result.required_query_key_point_count == 48U &&
          result.required_component_handle_scratch_count == 8U &&
          result.required_committed_batch_prefix_count == 6U &&
          result.required_batch_record_scan_count == 12U &&
          result.required_active_binding_prefix_count == 3U &&
          result.required_union_record_replay_count == 4U &&
          result.logical_output_entry_count == 12U,
      "the hostile sweep publishes the exact bounded preflight requirements");
  check(
      result.counters.query_scan_count == 12U &&
          result.counters.query_key_point_count == 48U &&
          result.counters.component_handle_initialization_count == 8U &&
          result.counters.batch_record_scan_count == 12U &&
          result.counters.union_record_replay_count == 4U &&
          result.counters.union_replay_parent_hop_count == 2U &&
          result.counters.query_resolution_count == 12U &&
          result.counters.slot_visit_count == 23U &&
          result.counters.query_parent_hop_count == 4U &&
          result.counters.full_key_comparison_count == 18U &&
          result.counters.equal_fingerprint_distinct_key_count == 11U &&
          result.counters.future_binding_terminator_count == 4U &&
          result.counters.relative_positive_count == 7U &&
          result.counters.latent_unresolved_count == 5U &&
          result.counters.maximum_single_query_slot_visit_count == 4U &&
          result.counters.maximum_single_query_parent_hop_count == 2U &&
          result.counters.locator_snapshot_check_count == 20U,
      "the hostile sweep accounts exactly for collisions, future terminators and both parent-hop domains");
  check(
      result.future_binding_slots_are_historical_terminators &&
          result.every_positive_has_historical_root_and_original_witness &&
          result.common_frozen_locator_snapshot_certified &&
          !result.source_batch_alignment_claimed &&
          !result.external_binding_authority_replayed &&
          !result.missing_facet_means_isolated &&
          !result.locator_state_mutated && !result.locator_batch_committed &&
          result.partial_refinement_only && !result.public_status_claimed,
      "the result stays locator-prefix-relative and never invents 10.7 temporal alignment");

  const auto verified =
      verify_exact_direct_sparse_positive_facet_prefix_sweep(
          queries,
          witness(900U),
          locator,
          budget,
          exact_locator_verification_budget(locator),
          result);
  check(
      verified.trusted_live_locator_and_witness_certified &&
          verified.observed_storage_within_budget &&
          verified.locator_snapshot_matches_observed_build &&
          verified.locator_verification_budget_preflight_certified &&
          verified.locator_verification_budget_respected &&
          verified.locator_structural_verification.result_certified &&
          verified.locator_durable_structure_freshly_verified &&
          verified.committed_slot_insertion_chronology_freshly_replayed &&
          verified.queries_and_prefixes_freshly_replayed &&
          verified.union_prefixes_and_historical_roots_freshly_replayed &&
          verified.historical_slot_probes_freshly_replayed &&
          verified.counters_and_result_facts_freshly_replayed &&
          !verified.external_binding_authority_replayed &&
          !verified.source_batch_alignment_replayed &&
          verified.result_certified,
      "the fresh verifier reconstructs every prefix without claiming a 10.7 clock mapping");

  check(
      locator == durable_before && locator.snapshot_stamp() == stamp_before &&
          locator.counters() == counters_before &&
          locator.slots().data() == slots_data_before &&
          locator.key_point_arena().data() == keys_data_before &&
          locator.component_parents().data() == parents_data_before &&
          locator.committed_unions().data() == unions_data_before &&
          locator.committed_batches().data() == batches_data_before &&
          locator.slots().capacity() == slots_capacity_before &&
          locator.key_point_arena().capacity() == keys_capacity_before &&
          locator.component_parents().capacity() == parents_capacity_before &&
          locator.committed_unions().capacity() == unions_capacity_before &&
          locator.committed_batches().capacity() == batches_capacity_before,
      "build and fresh verification leave every durable locator value and arena identity unchanged");
}

void check_empty_and_k10_boundaries() {
  const auto hostile = make_hostile_locator();
  const ExactDirectSparsePositiveFacetPrefixSweepBudget empty_budget{
      0U, 0U, 8U, 0U, 0U, 0U, 0U, 0U, 0U, {0U, 0U}};
  const auto empty = build_exact_direct_sparse_positive_facet_prefix_sweep(
      std::span<const ExactDirectSparsePositiveFacetPrefixQuery>{},
      witness(901U),
      hostile,
      empty_budget);
  check(
      empty.certified_partial_refinement() && empty.resolutions.empty() &&
          empty.required_component_handle_scratch_count == 8U &&
          empty.counters.component_handle_initialization_count == 8U &&
          empty.counters.locator_snapshot_check_count == 2U &&
          empty.required_committed_batch_prefix_count == 0U,
      "an empty query sweep succeeds with one bounded identity scratch and two stamp checks");
  check(
      verify_exact_direct_sparse_positive_facet_prefix_sweep(
          std::span<const ExactDirectSparsePositiveFacetPrefixQuery>{},
          witness(901U),
          hostile,
          empty_budget,
          exact_locator_verification_budget(hostile),
          empty)
          .result_certified,
      "the fresh verifier accepts the empty sweep");
  const auto structural_budget_rejected =
      verify_exact_direct_sparse_positive_facet_prefix_sweep(
          std::span<const ExactDirectSparsePositiveFacetPrefixQuery>{},
          witness(901U),
          hostile,
          empty_budget,
          ExactDirectSparsePositiveFacetLocatorStructuralVerificationBudget{},
          empty);
  check(
      !structural_budget_rejected
           .locator_verification_budget_preflight_certified &&
          !structural_budget_rejected.locator_verification_budget_respected &&
          structural_budget_rejected.locator_structural_verification
              .budget_exhausted &&
          !structural_budget_rejected
               .locator_durable_structure_freshly_verified &&
          !structural_budget_rejected.fresh_replay_certified &&
          !structural_budget_rejected.result_certified,
      "an empty sweep budget cannot silently authorize an unbounded full-locator verification");

  auto locator = make_empty_locator();
  const ExactDirectSparseFacetKey k10 =
      key({20U, 21U, 22U, 23U, 24U, 25U, 26U, 27U, 28U, 29U});
  const std::array<ExactDirectSparseFacetBinding, 1U> binding{{
      {0U, k10, 7U, witness(200U)},
  }};
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              binding)
          .certified_committed_batch(),
      "the K=10 boundary binding is committed");
  const std::array<ExactDirectSparsePositiveFacetPrefixQuery, 2U> queries{{
      {0U, 0U, k10},
      {1U, 1U, k10},
  }};
  const ExactDirectSparsePositiveFacetPrefixSweepBudget budget{
      2U, 20U, 8U, 2U, 0U, 0U, 2U, 0U, 2U, {1U, 0U}};
  const auto result = build_exact_direct_sparse_positive_facet_prefix_sweep(
      queries, witness(902U), locator, budget);
  check(
      result.certified_partial_refinement() &&
          result.counters.future_binding_terminator_count == 1U &&
          result.counters.slot_visit_count == 2U &&
          result.counters.full_key_comparison_count == 1U,
      "the full K=10 key is hidden before its batch and matched afterward");
  check_latent(result, 0U, 0U, "the K=10 final slot is a future terminator at prefix zero");
  check_positive(result, 1U, 1U, 7U, witness(200U), "the complete K=10 key resolves after its commit");
}

template <class Callable>
void expect_invalid_argument(Callable&& callable, const std::string& message) {
  bool rejected = false;
  try {
    callable();
  } catch (const std::invalid_argument&) {
    rejected = true;
  } catch (...) {
  }
  check(rejected, message);
}

void check_invalid_inputs_and_witnesses() {
  const auto locator = make_hostile_locator();
  const auto budget = generous_sweep_budget();
  const ExactDirectSparseFacetKey facet_a = key({1U, 3U, 5U, 7U});

  expect_invalid_argument(
      [&]() {
        const ExactDirectSparsePositiveFacetLocator uninitialized;
        static_cast<void>(
            build_exact_direct_sparse_positive_facet_prefix_sweep(
                std::span<const ExactDirectSparsePositiveFacetPrefixQuery>{},
                witness(910U),
                uninitialized,
                budget));
      },
      "an uninitialized locator is rejected before sweep construction");
  for (const ExactDirectSparseFacetWitness invalid_witness :
       std::array<ExactDirectSparseFacetWitness, 3U>{{
           {0U, 1U},
           {authority_id, 0U},
           {authority_id + 1U, 1U},
       }}) {
    expect_invalid_argument(
        [&]() {
          static_cast<void>(
              build_exact_direct_sparse_positive_facet_prefix_sweep(
                  std::span<const ExactDirectSparsePositiveFacetPrefixQuery>{},
                  invalid_witness,
                  locator,
                  budget));
        },
        "a null, tokenless or foreign locator witness is rejected");
  }

  std::vector<std::vector<ExactDirectSparsePositiveFacetPrefixQuery>>
      malformed_queries;
  malformed_queries.push_back({{1U, 0U, facet_a}});
  malformed_queries.push_back({
      {0U, 2U, facet_a},
      {1U, 1U, facet_a},
  });
  malformed_queries.push_back({{0U, 7U, facet_a}});
  malformed_queries.push_back({{0U, 0U, ExactDirectSparseFacetKey{}}});
  ExactDirectSparseFacetKey oversized = facet_a;
  oversized.point_count = 11U;
  malformed_queries.push_back({{0U, 0U, oversized}});
  malformed_queries.push_back({{0U, 0U, key({1U, 1U})}});
  ExactDirectSparseFacetKey nonzero_tail = key({1U, 2U, 3U});
  nonzero_tail.point_ids[9U] = 99U;
  malformed_queries.push_back({{0U, 0U, nonzero_tail}});

  for (const auto& malformed : malformed_queries) {
    const auto rejected =
        build_exact_direct_sparse_positive_facet_prefix_sweep(
            std::span<const ExactDirectSparsePositiveFacetPrefixQuery>{
                malformed},
            witness(911U),
            locator,
            budget);
    check(
        rejected.certified_atomic_failure() &&
            rejected.decision ==
                ExactDirectSparsePositiveFacetPrefixSweepDecision::
                    no_prefix_sweep_input_shape_rejected &&
            rejected.resolutions.empty() &&
            rejected.logical_output_entry_count == 0U,
        "a non-dense index, decreasing/out-of-range prefix or malformed key fails atomically");
  }
}

void check_budget_failure(
    const std::vector<ExactDirectSparsePositiveFacetPrefixQuery>& queries,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparsePositiveFacetPrefixSweepBudget& budget,
    ExactDirectSparsePositiveFacetPrefixSweepDecision expected_decision,
    const std::string& message) {
  const auto result = build_exact_direct_sparse_positive_facet_prefix_sweep(
      queries, witness(920U), locator, budget);
  const auto verified =
      verify_exact_direct_sparse_positive_facet_prefix_sweep(
          queries,
          witness(920U),
          locator,
          budget,
          exact_locator_verification_budget(locator),
          result);
  check(
      result.certified_atomic_failure() &&
          result.decision == expected_decision &&
          result.resolutions.empty() &&
          result.logical_output_entry_count == 0U &&
          verified.result_certified,
      message);
}

void check_every_budget_exact_and_minus_one() {
  const auto locator = make_hostile_locator();
  const auto queries = hostile_queries();
  const auto exact = exact_budget();
  check(
      build_exact_direct_sparse_positive_facet_prefix_sweep(
          queries, witness(920U), locator, exact)
          .certified_partial_refinement(),
      "all eleven prefix-sweep caps pass at their exact values");

  auto reduced = exact;
  --reduced.maximum_query_count;
  check_budget_failure(
      queries,
      locator,
      reduced,
      ExactDirectSparsePositiveFacetPrefixSweepDecision::
          no_prefix_sweep_budget_exhausted,
      "the query-count cap fails atomically at exact minus one");
  reduced = exact;
  --reduced.maximum_query_key_point_count;
  check_budget_failure(
      queries,
      locator,
      reduced,
      ExactDirectSparsePositiveFacetPrefixSweepDecision::
          no_prefix_sweep_budget_exhausted,
      "the query-key PointId cap fails atomically at exact minus one");
  reduced = exact;
  --reduced.maximum_component_handle_scratch_count;
  check_budget_failure(
      queries,
      locator,
      reduced,
      ExactDirectSparsePositiveFacetPrefixSweepDecision::
          no_prefix_sweep_budget_exhausted,
      "the identity-DSU scratch cap fails atomically at exact minus one");
  reduced = exact;
  --reduced.maximum_batch_record_scan_count;
  check_budget_failure(
      queries,
      locator,
      reduced,
      ExactDirectSparsePositiveFacetPrefixSweepDecision::
          no_prefix_sweep_budget_exhausted,
      "the two-pass batch-record cap fails atomically at exact minus one");
  reduced = exact;
  --reduced.maximum_union_record_replay_count;
  check_budget_failure(
      queries,
      locator,
      reduced,
      ExactDirectSparsePositiveFacetPrefixSweepDecision::
          no_prefix_sweep_budget_exhausted,
      "the union-record cap fails atomically at exact minus one");
  reduced = exact;
  --reduced.maximum_union_replay_parent_hop_count;
  check_budget_failure(
      queries,
      locator,
      reduced,
      ExactDirectSparsePositiveFacetPrefixSweepDecision::
          no_prefix_sweep_budget_exhausted,
      "the redundant-union parent-hop cap fails at exact minus one");
  reduced = exact;
  --reduced.maximum_aggregate_slot_visit_count;
  check_budget_failure(
      queries,
      locator,
      reduced,
      ExactDirectSparsePositiveFacetPrefixSweepDecision::
          no_prefix_sweep_budget_exhausted,
      "the aggregate slot cap fails atomically at exact minus one");
  reduced = exact;
  --reduced.maximum_aggregate_query_parent_hop_count;
  check_budget_failure(
      queries,
      locator,
      reduced,
      ExactDirectSparsePositiveFacetPrefixSweepDecision::
          no_prefix_sweep_budget_exhausted,
      "the aggregate query-parent cap fails atomically at exact minus one");
  reduced = exact;
  --reduced.maximum_logical_output_entry_count;
  check_budget_failure(
      queries,
      locator,
      reduced,
      ExactDirectSparsePositiveFacetPrefixSweepDecision::
          no_prefix_sweep_budget_exhausted,
      "the logical-output cap fails atomically at exact minus one");
  reduced = exact;
  --reduced.facet_probe_budget.maximum_slot_visit_count;
  check_budget_failure(
      queries,
      locator,
      reduced,
      ExactDirectSparsePositiveFacetPrefixSweepDecision::
          no_prefix_sweep_probe_budget_exhausted,
      "the per-query slot cap fails distinctly at exact minus one");
  reduced = exact;
  --reduced.facet_probe_budget.maximum_component_parent_hop_count;
  check_budget_failure(
      queries,
      locator,
      reduced,
      ExactDirectSparsePositiveFacetPrefixSweepDecision::
          no_prefix_sweep_probe_budget_exhausted,
      "the per-query parent-hop cap fails distinctly at exact minus one");
}

void check_fresh_verifier_has_distinct_bounded_accounting() {
  const auto locator = make_hostile_locator();
  const auto queries = hostile_queries();
  const auto sweep_budget = exact_budget();
  const auto result = build_exact_direct_sparse_positive_facet_prefix_sweep(
      queries, witness(925U), locator, sweep_budget);
  const auto exact_structural_budget =
      exact_locator_verification_budget(locator);
  const auto exact = verify_exact_direct_sparse_positive_facet_prefix_sweep(
      queries,
      witness(925U),
      locator,
      sweep_budget,
      exact_structural_budget,
      result);
  check(
      exact.locator_verification_budget_preflight_certified &&
          exact.locator_verification_budget_respected &&
          exact.locator_structural_verification.requested_budget ==
              exact_structural_budget &&
          exact.result_certified,
      "the fresh verifier succeeds when every distinct structural cap is exact");

  auto reduced_structural_budget = exact_structural_budget;
  check(
      reduced_structural_budget
              .maximum_fingerprint_search_slot_visit_count != 0U,
      "the forced-collision fixture performs a nonzero structural fingerprint search");
  --reduced_structural_budget.maximum_fingerprint_search_slot_visit_count;
  const auto exhausted =
      verify_exact_direct_sparse_positive_facet_prefix_sweep(
          queries,
          witness(925U),
          locator,
          sweep_budget,
          reduced_structural_budget,
          result);
  check(
      exhausted.locator_verification_budget_preflight_certified &&
          !exhausted.locator_verification_budget_respected &&
          exhausted.locator_structural_verification
              .fingerprint_search_budget_exhausted &&
          exhausted.locator_structural_verification.decision ==
              ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
                  no_verification_fingerprint_search_budget_exhausted &&
          !exhausted.observed_outcome_well_formed &&
          !exhausted.fresh_replay_certified && !exhausted.result_certified,
      "a structural collision cap at exact minus one stops before reconstructing the sweep and is not reported as malformed state");
}

void check_verifier_mutations_and_stale_stamp() {
  auto locator = make_hostile_locator();
  const auto queries = hostile_queries();
  const auto budget = exact_budget();
  const auto result = build_exact_direct_sparse_positive_facet_prefix_sweep(
      queries, witness(930U), locator, budget);
  const auto rejected = [&](
                            const ExactDirectSparsePositiveFacetPrefixSweepResult&
                                observed,
                            const std::string& message) {
    check(
        !verify_exact_direct_sparse_positive_facet_prefix_sweep(
             queries,
             witness(930U),
             locator,
             budget,
             exact_locator_verification_budget(locator),
             observed)
             .result_certified,
        message);
  };

  auto mutated = result;
  mutated.resolutions[8U].committed_batch_prefix_count = 5U;
  rejected(mutated, "the verifier rejects a substituted historical prefix");
  mutated = result;
  mutated.resolutions[8U].component_handle = 7U;
  rejected(mutated, "the verifier rejects a substituted historical root");
  mutated = result;
  mutated.resolutions[8U].component_handle =
      result.required_component_handle_scratch_count;
  check(
      !mutated.certified_partial_refinement(),
      "the self-contained predicate rejects a positive handle outside its declared dense scratch");
  rejected(mutated, "the verifier rejects an out-of-range historical root");
  mutated = result;
  ++mutated.resolutions[8U].source_binding_witness.replay_token;
  rejected(mutated, "the verifier rejects a substituted original binding witness");
  mutated = result;
  ++mutated.resolutions[8U].source_binding_witness.external_authority_id;
  check(
      !mutated.certified_partial_refinement(),
      "the self-contained predicate rejects a positive witness from another authority");
  rejected(mutated, "the verifier rejects a foreign source-binding authority");
  mutated = result;
  --mutated.counters.relative_positive_count;
  ++mutated.counters.latent_unresolved_count;
  check(
      !mutated.certified_partial_refinement(),
      "the self-contained predicate recounts positive and latent dispositions");
  rejected(mutated, "the verifier rejects substituted disposition counters");
  mutated = result;
  ++mutated.counters.future_binding_terminator_count;
  rejected(mutated, "the verifier rejects a mutated future-slot audit");
  mutated = result;
  mutated.source_batch_alignment_claimed = true;
  rejected(mutated, "the verifier rejects an invented 10.7 source-batch alignment");
  mutated = result;
  ++mutated.locator_snapshot_stamp.committed_batch_count;
  rejected(mutated, "the verifier rejects a substituted locator lineage stamp");
  mutated = result;
  mutated.resolutions.push_back(mutated.resolutions.back());
  mutated.logical_output_entry_count = mutated.resolutions.size();
  rejected(mutated, "oversized hostile output is rejected before fresh replay");

  const auto stale_result = result;
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch(),
      "an empty commit advances the live locator stamp after the sweep");
  const auto stale_verification =
      verify_exact_direct_sparse_positive_facet_prefix_sweep(
          queries,
          witness(930U),
          locator,
          budget,
          exact_locator_verification_budget(locator),
          stale_result);
  check(
      !stale_verification.locator_snapshot_matches_observed_build &&
          !stale_verification.fresh_replay_certified &&
          !stale_verification.result_certified,
      "an otherwise unchanged historical prefix result is stale after a later empty commit");
}

}  // namespace

int main() {
  check_hostile_prefix_oracles_and_read_only_state();
  check_empty_and_k10_boundaries();
  check_invalid_inputs_and_witnesses();
  check_every_budget_exact_and_minus_one();
  check_fresh_verifier_has_distinct_bounded_accounting();
  check_verifier_mutations_and_stale_stamp();

  if (failures != 0) {
    std::cerr << failures
              << " direct sparse positive-facet prefix-sweep checks failed\n";
    return 1;
  }
  std::cout <<
      "direct sparse positive-facet prefix-sweep checks passed\n";
  return 0;
}
