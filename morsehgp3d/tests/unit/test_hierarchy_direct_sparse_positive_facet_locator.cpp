#include "morsehgp3d/hierarchy/direct_sparse_positive_facet_locator.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::contract::CanonicalId;
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

[[nodiscard]]
ExactDirectSparsePositiveFacetLocatorStructuralVerificationBudget
generous_structural_verification_budget() {
  return {
      128U,
      1024U,
      16U,
      64U,
      64U,
      64U,
      1024U,
      128U,
      16U,
      8192U,
      8192U,
      8192U,
      8192U,
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

[[nodiscard]]
ExactDirectSparsePositiveFacetLocatorPrefixStampSweepBudget
generous_prefix_stamp_sweep_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      64U,
      128U,
      128U,
      64U,
      64U,
      64U,
      640U,
      maximum,
  };
}

[[nodiscard]]
ExactDirectSparsePositiveFacetLocatorPrefixStampSweepBudget
exact_prefix_stamp_sweep_budget() {
  return {
      9U,
      12U,
      65U,
      3U,
      4U,
      3U,
      12U,
      3U * sizeof(std::size_t),
  };
}

struct PrefixStampSixCommitFixture {
  ExactDirectSparsePositiveFacetLocator locator;
  std::array<ExactDirectSparsePositiveFacetLocatorSnapshotStamp, 7U>
      live_snapshot_stamps{};
};

[[nodiscard]] PrefixStampSixCommitFixture
make_prefix_stamp_six_commit_locator() {
  PrefixStampSixCommitFixture fixture{make_locator(0U), {}};
  ExactDirectSparsePositiveFacetLocator& locator = fixture.locator;
  fixture.live_snapshot_stamps[0U] = locator.snapshot_stamp();
  const ExactDirectSparseFacetKey facet_a = key({1U, 3U, 5U, 7U});
  const ExactDirectSparseFacetKey facet_b = key({2U, 4U, 6U, 8U});
  const ExactDirectSparseFacetKey facet_c = key({9U, 10U, 11U, 12U});

  const std::array<ExactDirectSparseFacetBinding, 1U> first_binding{{
      {0U, facet_a, 5U, witness(300U)},
  }};
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              first_binding)
          .certified_committed_batch(),
      "prefix-stamp batch 0 inserts A");
  fixture.live_snapshot_stamps[1U] = locator.snapshot_stamp();
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch(),
      "prefix-stamp batch 1 is empty");
  fixture.live_snapshot_stamps[2U] = locator.snapshot_stamp();

  const std::array<ExactDirectSparseComponentUnion, 1U> first_union{{
      {0U, 5U, 4U, witness(301U)},
  }};
  const std::array<ExactDirectSparseFacetBinding, 1U> second_binding{{
      {0U, facet_b, 2U, witness(302U)},
  }};
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              first_union,
              second_binding)
          .certified_committed_batch(),
      "prefix-stamp batch 2 unions A and inserts B");
  fixture.live_snapshot_stamps[3U] = locator.snapshot_stamp();

  const std::array<ExactDirectSparseComponentUnion, 1U> second_union{{
      {0U, 4U, 1U, witness(303U)},
  }};
  const std::array<ExactDirectSparseFacetBinding, 1U>
      compatible_duplicate{{
          {0U, facet_a, 1U, witness(304U)},
      }};
  const auto duplicate = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      second_union,
      compatible_duplicate);
  check(
      duplicate.certified_committed_batch() &&
          duplicate.counters.inserted_binding_count == 0U &&
          duplicate.counters.compatible_duplicate_binding_count == 1U,
      "prefix-stamp batch 3 commits one compatible duplicate");
  fixture.live_snapshot_stamps[4U] = locator.snapshot_stamp();

  const std::array<ExactDirectSparseComponentUnion, 1U> redundant_union{{
      {0U, 5U, 1U, witness(305U)},
  }};
  const std::array<ExactDirectSparseFacetBinding, 1U> third_binding{{
      {0U, facet_c, 3U, witness(306U)},
  }};
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              redundant_union,
              third_binding)
          .certified_committed_batch(),
      "prefix-stamp batch 4 records a redundant union and inserts C");
  fixture.live_snapshot_stamps[5U] = locator.snapshot_stamp();

  const std::array<ExactDirectSparseComponentUnion, 1U> final_union{{
      {0U, 2U, 1U, witness(307U)},
  }};
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              final_union,
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch(),
      "prefix-stamp batch 5 unions B into the final root");
  fixture.live_snapshot_stamps[6U] = locator.snapshot_stamp();
  check(
      locator.committed_batches().size() == 6U &&
          locator.committed_unions().size() == 4U &&
          locator.counters().inserted_binding_count == 3U &&
          locator.counters().binding_request_count == 4U,
      "the prefix-stamp fixture has six commits, four unions, three keys and four binding requests");
  return fixture;
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocatorStructuralVerification
verify_structure(
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparsePositiveFacetLocatorStateView& observed,
    const ExactDirectSparsePositiveFacetLocatorStructuralVerificationBudget&
        budget = generous_structural_verification_budget()) {
  return verify_exact_direct_sparse_positive_facet_locator_structure(
      locator.component_parents().size(),
      locator.budget(),
      locator.config(),
      budget,
      observed);
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

void check_snapshot_stamp_tracks_only_committed_state() {
  static_assert(
      std::is_trivially_copyable_v<
          ExactDirectSparsePositiveFacetLocatorSnapshotStamp>);

  auto locator = make_locator();
  const auto initial = locator.snapshot_stamp();
  const CanonicalId initial_digest = initial.committed_history_digest;
  check(
      initial == ExactDirectSparsePositiveFacetLocatorSnapshotStamp{
                     direct_sparse_positive_facet_locator_schema_version,
                     authority_id,
                     0U,
                     0U,
                     0U,
                     0U,
                     initial_digest} &&
          locator.state_view().committed_history_digest == initial_digest,
      "the initial snapshot stamp exposes its schema, authority and canonical history digest with zero committed work");

  const ExactDirectSparseFacetKey facet = key({1U, 4U, 9U});
  const std::array<ExactDirectSparseFacetBinding, 1U> invalid_bindings{{
      {0U, facet, 2U, {authority_id, 0U}},
  }};
  const auto invalid = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      invalid_bindings);
  check(
      invalid.decision ==
              ExactDirectSparsePositiveFacetBatchDecision::
                  no_positive_locator_external_witness_rejected &&
          locator.snapshot_stamp() == initial &&
          locator.snapshot_stamp().committed_history_digest == initial_digest,
      "a rejected batch leaves every snapshot field and its history digest unchanged");

  const auto empty_commit = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      std::span<const ExactDirectSparseFacetBinding>{});
  const auto after_empty = locator.snapshot_stamp();
  check(
      empty_commit.certified_committed_batch() &&
          after_empty ==
              ExactDirectSparsePositiveFacetLocatorSnapshotStamp{
                  direct_sparse_positive_facet_locator_schema_version,
                  authority_id,
                  1U,
                  0U,
                  0U,
                  0U,
                  after_empty.committed_history_digest} &&
          after_empty.committed_history_digest != initial_digest &&
          after_empty.committed_history_digest ==
              locator.state_view().committed_history_digest &&
          !(after_empty == initial),
      "a committed empty batch advances both the snapshot commit clock and its canonical history digest");

  const std::array<ExactDirectSparseFacetBinding, 1U> first_binding{{
      {0U, facet, 2U, witness(100U)},
  }};
  const auto inserted = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      first_binding);
  const auto after_insert = locator.snapshot_stamp();
  check(
      inserted.certified_committed_batch() &&
          after_insert.committed_batch_count == 2U &&
          after_insert.inserted_key_count == 1U &&
          after_insert.component_union_count == 0U &&
          after_insert.binding_count == 1U &&
          after_insert.committed_history_digest !=
              after_empty.committed_history_digest,
      "a new durable facet advances its counts and canonical history digest");

  const std::array<ExactDirectSparseComponentUnion, 1U> component_union{{
      {0U, 0U, 1U, witness(101U)},
  }};
  const auto united = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      component_union,
      std::span<const ExactDirectSparseFacetBinding>{});
  const auto after_union = locator.snapshot_stamp();
  check(
      united.certified_committed_batch() &&
          after_union.committed_batch_count == 3U &&
          after_union.inserted_key_count == 1U &&
          after_union.component_union_count == 1U &&
          after_union.binding_count == 1U &&
          after_union.committed_history_digest !=
              after_insert.committed_history_digest,
      "a committed component union advances its operation count, commit clock and canonical history digest");

  const std::array<ExactDirectSparseFacetBinding, 1U>
      compatible_duplicate{{
          {0U, facet, 2U, witness(102U)},
      }};
  const auto duplicate = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      compatible_duplicate);
  const auto after_duplicate = locator.snapshot_stamp();
  check(
      duplicate.certified_committed_batch() &&
          duplicate.counters.compatible_duplicate_binding_count == 1U &&
          after_duplicate.committed_batch_count == 4U &&
          after_duplicate.inserted_key_count == 1U &&
          after_duplicate.component_union_count == 1U &&
          after_duplicate.binding_count == 2U &&
          after_duplicate.committed_history_digest !=
              after_union.committed_history_digest,
      "a compatible duplicate advances the binding-request count and history digest without inventing a second key");

  const std::array<ExactDirectSparseFacetBinding, 1U>
      incompatible_duplicate{{
          {0U, facet, 3U, witness(103U)},
      }};
  const auto contradiction = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      incompatible_duplicate);
  check(
      contradiction.decision ==
              ExactDirectSparsePositiveFacetBatchDecision::
                  contradiction_incompatible_exact_facet_binding &&
          locator.snapshot_stamp() == after_duplicate &&
          locator.snapshot_stamp().committed_history_digest ==
              after_duplicate.committed_history_digest,
      "a rejected contradiction cannot advance any snapshot stamp field or its digest");
}

void check_snapshot_digest_determinism_and_state_divergence() {
  auto canonical = make_locator();
  auto replayed = make_locator();
  auto different_key = make_locator();
  auto different_witness = make_locator();

  check(
      canonical.snapshot_stamp() == replayed.snapshot_stamp(),
      "identically configured empty locators start with the same history digest");

  const std::array<ExactDirectSparseFacetBinding, 1U> canonical_binding{{
      {0U, key({1U, 4U, 9U}), 2U, witness(110U)},
  }};
  const std::array<ExactDirectSparseFacetBinding, 1U> different_key_binding{{
      {0U, key({1U, 4U, 10U}), 2U, witness(110U)},
  }};
  const std::array<ExactDirectSparseFacetBinding, 1U>
      different_witness_binding{{
          {0U, key({1U, 4U, 9U}), 2U, witness(111U)},
      }};
  const auto canonical_commit = canonical.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      canonical_binding);
  const auto replayed_commit = replayed.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      canonical_binding);
  const auto different_key_commit = different_key.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      different_key_binding);
  const auto different_witness_commit = different_witness.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      different_witness_binding);
  const auto canonical_stamp = canonical.snapshot_stamp();
  const auto replayed_stamp = replayed.snapshot_stamp();
  const auto different_key_stamp = different_key.snapshot_stamp();
  const auto different_witness_stamp = different_witness.snapshot_stamp();
  check(
      canonical_commit.certified_committed_batch() &&
          replayed_commit.certified_committed_batch() &&
          different_key_commit.certified_committed_batch() &&
          different_witness_commit.certified_committed_batch() &&
          canonical_stamp == replayed_stamp,
      "replaying the same canonical batch on equal locators produces the same complete snapshot stamp");
  check(
      canonical_stamp.committed_batch_count ==
              different_key_stamp.committed_batch_count &&
          canonical_stamp.inserted_key_count ==
              different_key_stamp.inserted_key_count &&
          canonical_stamp.component_union_count ==
              different_key_stamp.component_union_count &&
          canonical_stamp.binding_count == different_key_stamp.binding_count &&
          canonical_stamp.committed_history_digest !=
              different_key_stamp.committed_history_digest &&
          canonical_stamp.committed_history_digest !=
              different_witness_stamp.committed_history_digest,
      "equal authorities and counters cannot alias snapshots with different committed keys or witnesses");

  auto first_union = make_locator();
  auto different_union = make_locator();
  const std::array<ExactDirectSparseComponentUnion, 1U> canonical_union{{
      {0U, 0U, 1U, witness(112U)},
  }};
  const std::array<ExactDirectSparseComponentUnion, 1U> other_union{{
      {0U, 0U, 2U, witness(112U)},
  }};
  const auto first_union_commit = first_union.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      canonical_union,
      std::span<const ExactDirectSparseFacetBinding>{});
  const auto different_union_commit = different_union.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      other_union,
      std::span<const ExactDirectSparseFacetBinding>{});
  const auto first_union_stamp = first_union.snapshot_stamp();
  const auto different_union_stamp = different_union.snapshot_stamp();
  check(
      first_union_commit.certified_committed_batch() &&
          different_union_commit.certified_committed_batch() &&
          first_union_stamp.committed_batch_count ==
              different_union_stamp.committed_batch_count &&
          first_union_stamp.component_union_count ==
              different_union_stamp.component_union_count &&
          first_union_stamp.committed_history_digest !=
              different_union_stamp.committed_history_digest,
      "equal snapshot counters cannot alias different committed union histories");
}

void check_snapshot_digest_golden_vectors() {
  auto locator = make_locator();
  check(
      locator.snapshot_stamp().committed_history_digest.to_lower_hex() ==
          "d9b8a4e4b287a77411799dccdeea986565af78a80b26c8e36179852bcb5151a8",
      "the initial snapshot digest matches the independent Python SHA-256 v1 vector");

  const auto empty_commit = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      std::span<const ExactDirectSparseFacetBinding>{});
  check(
      empty_commit.certified_committed_batch() &&
          locator.snapshot_stamp()
                  .committed_history_digest.to_lower_hex() ==
              "d920fbef1978e49e8bb78b0c4ead161fd2b4b963ed91b19389d8a634a67b9929",
      "the committed-empty snapshot digest matches the independent Python SHA-256 v1 vector");

  const std::array<ExactDirectSparseComponentUnion, 1U> component_union{{
      {0U, 1U, 2U, witness(200U)},
  }};
  const std::array<ExactDirectSparseFacetBinding, 1U> binding{{
      {0U, key({1U, 4U, 9U}), 2U, witness(201U)},
  }};
  const auto mixed_commit = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      component_union,
      binding);
  check(
      mixed_commit.certified_committed_batch() &&
          locator.snapshot_stamp()
                  .committed_history_digest.to_lower_hex() ==
              "7976b55326f90d08889f09071245c0853e703a5e54c326b061f490cccf1b2a9f",
      "the empty-union-binding digest chain matches the independent Python SHA-256 v1 vector");
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

void check_public_fingerprint_is_bounded_and_canonical() {
  static_assert(noexcept(fingerprint_exact_direct_sparse_facet_key(
      ExactDirectSparseFacetKey{}, std::uint64_t{})));

  auto locator = make_locator();
  const ExactDirectSparseFacetKey canonical_key = key({1U, 4U, 9U});
  const std::array<ExactDirectSparseFacetBinding, 1U> binding{{
      {0U, canonical_key, 2U, witness(34U)},
  }};
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              binding)
          .certified_committed_batch(),
      "the public-fingerprint fixture stores one canonical key");

  const std::uint64_t complete_fingerprint =
      fingerprint_exact_direct_sparse_facet_key(
          canonical_key, std::numeric_limits<std::uint64_t>::max());
  constexpr std::uint64_t narrow_mask = UINT64_C(0x0000ffff00ffff00);
  bool stored_fingerprint_matches = false;
  for (const ExactDirectSparsePositiveFacetSlot& slot : locator.slots()) {
    if (slot.occupied) {
      stored_fingerprint_matches =
          slot.fingerprint == complete_fingerprint;
      break;
    }
  }

  ExactDirectSparseFacetKey malformed_key = canonical_key;
  malformed_key.point_count = std::numeric_limits<std::size_t>::max();
  const std::uint64_t malformed_fingerprint =
      fingerprint_exact_direct_sparse_facet_key(
          malformed_key, std::numeric_limits<std::uint64_t>::max());
  check(
      stored_fingerprint_matches &&
          fingerprint_exact_direct_sparse_facet_key(
              canonical_key, narrow_mask) ==
              (complete_fingerprint & narrow_mask) &&
          malformed_fingerprint ==
              fingerprint_exact_direct_sparse_facet_key(
                  malformed_key,
                  std::numeric_limits<std::uint64_t>::max()),
      "the public fingerprint preserves canonical table identity, applies the mask last and bounds reads for an invalid point count");
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

void check_prefix_stamp_sweep_and_exact_budgets() {
  static_assert(
      ExactDirectSparsePositiveFacetLocatorPrefixStampSweepResult::backend ==
          "reference_cpu" &&
      ExactDirectSparsePositiveFacetLocatorPrefixStampSweepResult::profile ==
          "hgp_reduced" &&
      ExactDirectSparsePositiveFacetLocatorPrefixStampSweepResult::mode ==
          "certified" &&
      ExactDirectSparsePositiveFacetLocatorPrefixStampSweepResult::
              refinement_status ==
          "partial_refinement" &&
      ExactDirectSparsePositiveFacetLocatorPrefixStampSweepResult::
              public_status ==
          "not_claimed");

  auto fixture = make_prefix_stamp_six_commit_locator();
  auto& locator = fixture.locator;
  const std::array<std::size_t, 9U> repeated_prefixes{
      0U, 0U, 1U, 2U, 2U, 3U, 4U, 5U, 6U};
  const auto exact =
      build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
          repeated_prefixes, locator, exact_prefix_stamp_sweep_budget());
  check(
      exact.certified_partial_refinement() &&
          exact.prefix_stamps.size() == repeated_prefixes.size() &&
          exact.required_committed_batch_prefix_count == 6U &&
          exact.required_batch_record_scan_count == 12U &&
          exact.required_table_slot_scan_count == locator.slots().size() &&
          exact.required_active_binding_prefix_count == 3U &&
          exact.required_union_record_replay_count == 4U &&
          exact.required_key_point_replay_count == 12U &&
          exact.required_temporary_scratch_byte_count ==
              3U * sizeof(std::size_t) &&
          exact.counters.batch_record_scan_count == 12U &&
          exact.counters.table_slot_scan_count == locator.slots().size() &&
          exact.counters.union_record_replay_count == 4U &&
          exact.counters.binding_record_replay_count == 3U &&
          exact.counters.key_point_replay_count == 12U &&
          exact.counters.emitted_stamp_count == repeated_prefixes.size() &&
          exact.counters.locator_snapshot_check_count == 2U,
      "the prefix-stamp sweep performs exactly two batch scans, one conditional table scan and one replay of each active durable delta");
  check(
      exact.prefix_stamps[0U] == exact.prefix_stamps[1U] &&
          exact.prefix_stamps[3U] == exact.prefix_stamps[4U] &&
          exact.prefix_stamps[0U].committed_batch_count == 0U &&
          exact.prefix_stamps[2U].committed_batch_count == 1U &&
          exact.prefix_stamps[3U].committed_batch_count == 2U &&
          exact.prefix_stamps[5U].committed_batch_count == 3U &&
          exact.prefix_stamps[6U].committed_batch_count == 4U &&
          exact.prefix_stamps[7U].committed_batch_count == 5U &&
          exact.prefix_stamps[8U].committed_batch_count == 6U,
      "repeated nondecreasing prefix requests preserve their exact input order in the output");
  bool historical_stamps_match_live_commits = true;
  for (std::size_t request_index = 0U;
       request_index < repeated_prefixes.size();
       ++request_index) {
    historical_stamps_match_live_commits =
        historical_stamps_match_live_commits &&
        exact.prefix_stamps[request_index] ==
            fixture.live_snapshot_stamps[repeated_prefixes[request_index]];
  }
  check(
      historical_stamps_match_live_commits,
      "every reconstructed prefix stamp equals the independent live snapshot "
      "captured at that commit boundary");
  check(
      exact.prefix_stamps[2U].inserted_key_count == 1U &&
          exact.prefix_stamps[2U].component_union_count == 0U &&
          exact.prefix_stamps[2U].binding_count == 1U &&
          exact.prefix_stamps[3U].inserted_key_count == 1U &&
          exact.prefix_stamps[3U].component_union_count == 0U &&
          exact.prefix_stamps[3U].binding_count == 1U &&
          exact.prefix_stamps[2U].committed_history_digest !=
              exact.prefix_stamps[3U].committed_history_digest,
      "an empty committed batch preserves semantic counters but advances the exact history stamp");
  check(
      exact.prefix_stamps[5U].inserted_key_count == 2U &&
          exact.prefix_stamps[5U].component_union_count == 1U &&
          exact.prefix_stamps[5U].binding_count == 2U &&
          exact.prefix_stamps[6U].inserted_key_count == 2U &&
          exact.prefix_stamps[6U].component_union_count == 2U &&
          exact.prefix_stamps[6U].binding_count == 3U &&
          exact.prefix_stamps[5U].committed_history_digest !=
              exact.prefix_stamps[6U].committed_history_digest,
      "a compatible duplicate advances the binding-request count and digest without inserting another key");
  check(
      exact.locator_snapshot_stamp == locator.snapshot_stamp() &&
          exact.prefix_stamps.back() == exact.locator_snapshot_stamp &&
          exact.prefix_stamps.back() == locator.snapshot_stamp() &&
          exact.final_prefix_matches_live_locator_when_requested &&
          exact.common_frozen_locator_snapshot_certified,
      "a B=T sweep compares its final reconstructed stamp exactly with both the entry and exit live snapshots");
  check(
      verify_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
          repeated_prefixes,
          locator,
          exact_prefix_stamp_sweep_budget(),
          exact)
          .result_certified,
      "fresh replay certifies the exact six-commit prefix-stamp sweep");

  const auto check_budget_failure = [&](
                                        const auto& reduced_budget,
                                        const std::string& context) {
    const auto failed =
        build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
            repeated_prefixes, locator, reduced_budget);
    check(
        failed.certified_atomic_failure() && failed.prefix_stamps.empty() &&
            failed.no_partial_scientific_payload_published &&
            failed.decision ==
                ExactDirectSparsePositiveFacetLocatorPrefixStampSweepDecision::
                    no_prefix_stamp_budget_exhausted,
        context);
  };
  auto prefix_request_less_one = exact_prefix_stamp_sweep_budget();
  --prefix_request_less_one.maximum_prefix_request_count;
  check_budget_failure(
      prefix_request_less_one,
      "the prefix-request cap fails atomically at exact minus one");
  auto batch_scan_less_one = exact_prefix_stamp_sweep_budget();
  --batch_scan_less_one.maximum_batch_record_scan_count;
  check_budget_failure(
      batch_scan_less_one,
      "the exact 2B batch-scan cap fails atomically at minus one");
  auto table_scan_less_one = exact_prefix_stamp_sweep_budget();
  --table_scan_less_one.maximum_table_slot_scan_count;
  check_budget_failure(
      table_scan_less_one,
      "the conditional table-scan cap fails atomically at exact minus one");
  auto binding_scratch_less_one = exact_prefix_stamp_sweep_budget();
  --binding_scratch_less_one.maximum_binding_slot_index_scratch_count;
  check_budget_failure(
      binding_scratch_less_one,
      "the unique binding-index scratch cap fails atomically at minus one");
  auto union_replay_less_one = exact_prefix_stamp_sweep_budget();
  --union_replay_less_one.maximum_union_record_replay_count;
  check_budget_failure(
      union_replay_less_one,
      "the union replay cap fails atomically at exact minus one");
  auto binding_replay_less_one = exact_prefix_stamp_sweep_budget();
  --binding_replay_less_one.maximum_binding_record_replay_count;
  check_budget_failure(
      binding_replay_less_one,
      "the binding replay cap fails atomically at exact minus one");
  auto key_point_replay_less_one = exact_prefix_stamp_sweep_budget();
  --key_point_replay_less_one.maximum_key_point_replay_count;
  check_budget_failure(
      key_point_replay_less_one,
      "the key-point replay cap fails atomically at exact minus one");
  auto scratch_bytes_less_one = exact_prefix_stamp_sweep_budget();
  --scratch_bytes_less_one.maximum_temporary_scratch_byte_count;
  check_budget_failure(
      scratch_bytes_less_one,
      "the exact I_B*sizeof(size_t) scratch-byte cap fails atomically at minus one");
}

void check_prefix_stamp_sweep_empty_invalid_stale_and_mutations() {
  auto fixture = make_prefix_stamp_six_commit_locator();
  auto& locator = fixture.locator;
  const auto generous = generous_prefix_stamp_sweep_budget();
  const auto locator_before_read_only_sweeps = locator;
  const auto* slots_data_before = locator.slots().data();
  const auto* key_arena_data_before = locator.key_point_arena().data();
  const auto* parents_data_before = locator.component_parents().data();
  const auto* unions_data_before = locator.committed_unions().data();
  const auto* batches_data_before = locator.committed_batches().data();
  const std::size_t slots_capacity_before = locator.slots().capacity();
  const std::size_t key_arena_capacity_before =
      locator.key_point_arena().capacity();
  const std::size_t parents_capacity_before =
      locator.component_parents().capacity();
  const std::size_t unions_capacity_before =
      locator.committed_unions().capacity();
  const std::size_t batches_capacity_before =
      locator.committed_batches().capacity();
  const auto empty =
      build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
          std::span<const std::size_t>{}, locator, generous);
  check(
      empty.certified_partial_refinement() && empty.prefix_stamps.empty() &&
          empty.required_committed_batch_prefix_count == 0U &&
          empty.required_batch_record_scan_count == 0U &&
          empty.required_table_slot_scan_count == 0U &&
          empty.required_active_binding_prefix_count == 0U &&
          empty.required_temporary_scratch_byte_count == 0U &&
          empty.counters.batch_record_scan_count == 0U &&
          empty.counters.table_slot_scan_count == 0U &&
          empty.counters.locator_snapshot_check_count == 2U,
      "an empty request succeeds without a history, table or binding-scratch scan");

  const std::array<std::size_t, 3U> zero_prefixes{0U, 0U, 0U};
  const auto zero =
      build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
          zero_prefixes, locator, generous);
  check(
      zero.certified_partial_refinement() && zero.prefix_stamps.size() == 3U &&
          zero.prefix_stamps[0U] == zero.prefix_stamps[1U] &&
          zero.prefix_stamps[1U] == zero.prefix_stamps[2U] &&
          zero.required_table_slot_scan_count == 0U &&
          zero.counters.table_slot_scan_count == 0U &&
          zero.required_temporary_scratch_byte_count == 0U,
      "repeated zero prefixes emit ordered initial stamps without scanning the populated final table");

  auto empty_locator = make_locator(0U);
  const std::array<std::size_t, 1U> initial_prefix{0U};
  const auto initial =
      build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
          initial_prefix, empty_locator, generous);
  check(
      initial.certified_partial_refinement() &&
          initial.prefix_stamps.size() == 1U &&
          initial.prefix_stamps[0U] == empty_locator.snapshot_stamp() &&
          initial.required_table_slot_scan_count == 0U,
      "the initial prefix of an empty locator is its exact current snapshot without a table scan");

  const std::array<std::size_t, 3U> decreasing{0U, 2U, 1U};
  const auto decreasing_rejected =
      build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
          decreasing, locator, generous);
  check(
      decreasing_rejected.certified_atomic_failure() &&
          decreasing_rejected.decision ==
              ExactDirectSparsePositiveFacetLocatorPrefixStampSweepDecision::
                  no_prefix_stamp_input_shape_rejected &&
          decreasing_rejected.prefix_stamps.empty(),
      "a decreasing prefix sequence is rejected atomically");
  const std::array<std::size_t, 1U> beyond_history{7U};
  const auto beyond_rejected =
      build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
          beyond_history, locator, generous);
  check(
      beyond_rejected.certified_atomic_failure() &&
          beyond_rejected.decision ==
              ExactDirectSparsePositiveFacetLocatorPrefixStampSweepDecision::
                  no_prefix_stamp_input_shape_rejected &&
          beyond_rejected.prefix_stamps.empty(),
      "a prefix beyond the durable commit history is rejected atomically");

  const ExactDirectSparsePositiveFacetLocator uninitialized_locator;
  const auto uninitialized_rejected =
      build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
          initial_prefix, uninitialized_locator, generous);
  check(
      uninitialized_rejected.certified_atomic_failure() &&
          uninitialized_rejected.decision ==
              ExactDirectSparsePositiveFacetLocatorPrefixStampSweepDecision::
                  no_prefix_stamp_locator_not_certified &&
          uninitialized_rejected.prefix_stamps.empty(),
      "an uninitialized locator is rejected before any prefix history is read");

  auto overflow_locator = locator;
  auto& overflow_batches =
      const_cast<std::vector<ExactDirectSparseCommittedBatchRecord>&>(
          overflow_locator.committed_batches());
  overflow_batches[0U].counters.inserted_binding_count =
      std::numeric_limits<std::size_t>::max();
  overflow_batches[1U].counters.inserted_binding_count = 1U;
  const std::array<std::size_t, 1U> overflow_prefix{2U};
  const auto overflow_rejected =
      build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
          overflow_prefix, overflow_locator, generous);
  check(
      overflow_rejected.certified_atomic_failure() &&
          overflow_rejected.decision ==
              ExactDirectSparsePositiveFacetLocatorPrefixStampSweepDecision::
                  no_prefix_stamp_capacity_overflow &&
          overflow_rejected.prefix_stamps.empty(),
      "prefix requirement arithmetic overflow stays distinct from malformed history and publishes no stamp");

  auto non_dense_locator = locator;
  auto& non_dense_batches =
      const_cast<std::vector<ExactDirectSparseCommittedBatchRecord>&>(
          non_dense_locator.committed_batches());
  non_dense_batches[1U].committed_batch_index = 2U;
  const auto non_dense_rejected =
      build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
          overflow_prefix, non_dense_locator, generous);
  check(
      non_dense_rejected.certified_atomic_failure() &&
          non_dense_rejected.decision ==
              ExactDirectSparsePositiveFacetLocatorPrefixStampSweepDecision::
                  no_prefix_stamp_locator_history_rejected &&
          non_dense_rejected.prefix_stamps.empty(),
      "a non-dense durable batch index is rejected as malformed history");

  auto malformed_locator = locator;
  auto& malformed_batches =
      const_cast<std::vector<ExactDirectSparseCommittedBatchRecord>&>(
          malformed_locator.committed_batches());
  malformed_batches[2U].counters.inserted_key_point_count = 3U;
  const std::array<std::size_t, 1U> malformed_prefix{3U};
  const auto malformed_rejected =
      build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
          malformed_prefix, malformed_locator, generous);
  check(
      malformed_rejected.certified_atomic_failure() &&
          malformed_rejected.decision ==
              ExactDirectSparsePositiveFacetLocatorPrefixStampSweepDecision::
                  no_prefix_stamp_locator_history_rejected &&
          malformed_rejected.prefix_stamps.empty(),
      "a malformed durable batch history is rejected without publishing a prefix stamp");

  auto extra_final_slot_locator = locator;
  auto& extra_final_slots =
      const_cast<std::vector<ExactDirectSparsePositiveFacetSlot>&>(
          extra_final_slot_locator.slots());
  const auto free_slot = std::find_if(
      extra_final_slots.begin(),
      extra_final_slots.end(),
      [](const ExactDirectSparsePositiveFacetSlot& slot) {
        return !slot.occupied;
      });
  check(
      free_slot != extra_final_slots.end(),
      "the final-history falsification has one free physical slot");
  if (free_slot != extra_final_slots.end()) {
    *free_slot = {
        0U,
        3U,
        0U,
        4U,
        0U,
        witness(399U),
        true,
    };
  }
  const std::array<std::size_t, 1U> final_prefix{6U};
  const auto extra_final_slot_rejected =
      build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
          final_prefix, extra_final_slot_locator, generous);
  check(
      extra_final_slot_rejected.certified_atomic_failure() &&
          extra_final_slot_rejected.decision ==
              ExactDirectSparsePositiveFacetLocatorPrefixStampSweepDecision::
                  no_prefix_stamp_locator_history_rejected &&
          extra_final_slot_rejected.prefix_stamps.empty(),
      "a B=T replay rejects an occupied slot beyond the exact active binding prefix");

  const std::array<std::size_t, 4U> observed_prefixes{0U, 2U, 4U, 6U};
  const auto observed =
      build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
          observed_prefixes, locator, generous);
  auto mutated_stamp = observed;
  ++mutated_stamp.prefix_stamps[1U].inserted_key_count;
  check(
      !verify_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
           observed_prefixes, locator, generous, mutated_stamp)
           .result_certified,
      "fresh replay rejects a mutated historical snapshot stamp");
  auto mutated_counter = observed;
  ++mutated_counter.counters.batch_record_scan_count;
  check(
      !verify_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
           observed_prefixes, locator, generous, mutated_counter)
           .result_certified,
      "fresh replay rejects a mutated sweep counter");
  auto invented_global = observed;
  invented_global.forbidden_global_structure_materialized = true;
  check(
      !verify_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
           observed_prefixes, locator, generous, invented_global)
           .result_certified,
      "fresh replay rejects an invented forbidden-global-structure claim");

  check(
      locator == locator_before_read_only_sweeps &&
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
      "all successful, rejected, exhausted and freshly verified prefix-stamp sweeps preserve every durable locator value, address and capacity");

  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch(),
      "the stale prefix-stamp fixture appends one empty commit");
  const auto stale_verification =
      verify_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
          observed_prefixes, locator, generous, observed);
  check(
      !stale_verification.locator_snapshot_matches_observed_build &&
          !stale_verification.result_certified,
      "an appended empty commit makes the previously observed prefix-stamp sweep stale even when earlier logical states are unchanged");
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
  const std::array<ExactDirectSparseComponentUnion, 2U> unions{{
      {0U, 3U, 1U, witness(83U)},
      {1U, 3U, 0U, witness(89U)},
  }};
  check(
      locator
          .apply_batch(
              queries,
              unions,
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch(),
      "the structural-verifier fixture stores one witnessed union and query batch");

  const auto verified = verify_structure(locator, locator.state_view());
  check(
      verified.trusted_construction_parameters_certified &&
          verified.capacity_requirements_certified &&
          verified.scratch_requirement_arithmetic_certified &&
          verified.budget_preflight_certified &&
          !verified.budget_exhausted &&
          !verified.structure_contract_rejected &&
          verified.flat_table_and_key_arena_certified &&
          verified.every_fingerprint_recomputed_and_full_key_located &&
          verified
              .committed_slot_insertion_chronology_freshly_replayed &&
          verified.dense_handle_dsu_replay_certified &&
          verified.union_witness_structure_certified &&
          verified
              .historical_batch_assertions_and_counters_well_formed &&
          verified.committed_history_digest_freshly_replayed &&
          verified.internal_fact_fields_match_contract &&
          verified.decision_and_scope_certified &&
          !verified.external_authority_replayed_by_locator &&
          verified
              .bounded_temporary_scratch_without_second_durable_output &&
          verified.fresh_durable_structure_verification_certified &&
          verified.result_certified &&
          verified.decision ==
              ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
                  complete_certified_durable_structure_verification &&
          verified.table_slot_scan_count == locator.slots().size() &&
          verified.key_point_scan_count ==
              locator.key_point_arena().size() &&
          verified.union_record_scan_count == unions.size() &&
          verified.batch_record_scan_count ==
              locator.committed_batches().size() &&
          verified.fingerprint_search_slot_visit_count > 0U &&
          verified.insertion_chronology_slot_visit_count > 0U &&
          verified.union_parent_hop_count > 0U,
      "fresh structural verification replays the DSU and audits every flat key while leaving caller authority unreplayed");

  auto exact_verification_budget = generous_structural_verification_budget();
  exact_verification_budget.maximum_table_slot_count =
      verified.required_table_slot_count;
  exact_verification_budget.maximum_key_point_count =
      verified.required_key_point_count;
  exact_verification_budget.maximum_component_parent_count =
      verified.required_component_parent_count;
  exact_verification_budget.maximum_union_record_count =
      verified.required_union_record_count;
  exact_verification_budget.maximum_batch_record_count =
      verified.required_batch_record_count;
  exact_verification_budget.maximum_binding_scratch_entry_count =
      verified.required_binding_scratch_entry_count;
  exact_verification_budget.maximum_key_point_scratch_entry_count =
      verified.required_key_point_scratch_entry_count;
  exact_verification_budget.maximum_table_slot_scratch_entry_count =
      verified.required_table_slot_scratch_entry_count;
  exact_verification_budget.maximum_component_parent_scratch_entry_count =
      verified.required_component_parent_scratch_entry_count;
  exact_verification_budget.maximum_temporary_scratch_byte_count =
      verified.required_temporary_scratch_byte_count;
  exact_verification_budget.maximum_fingerprint_search_slot_visit_count =
      verified.fingerprint_search_slot_visit_count;
  exact_verification_budget
      .maximum_insertion_chronology_slot_visit_count =
      verified.insertion_chronology_slot_visit_count;
  exact_verification_budget.maximum_union_parent_hop_count =
      verified.union_parent_hop_count;
  const auto exactly_budgeted =
      verify_structure(locator, locator.state_view(), exact_verification_budget);
  check(
      exactly_budgeted.result_certified &&
          exactly_budgeted.requested_budget == exact_verification_budget &&
          exactly_budgeted.required_temporary_scratch_byte_count ==
              verified.required_temporary_scratch_byte_count &&
          exactly_budgeted.fingerprint_search_slot_visit_count ==
              verified.fingerprint_search_slot_visit_count &&
          exactly_budgeted.insertion_chronology_slot_visit_count ==
              verified.insertion_chronology_slot_visit_count &&
          exactly_budgeted.union_parent_hop_count ==
              verified.union_parent_hop_count,
      "the exact structural size, scratch and three variable-work caps certify without slack");

  auto fingerprint_less_one = exact_verification_budget;
  --fingerprint_less_one.maximum_fingerprint_search_slot_visit_count;
  const auto fingerprint_exhausted =
      verify_structure(locator, locator.state_view(), fingerprint_less_one);
  check(
      fingerprint_exhausted.budget_preflight_certified &&
          fingerprint_exhausted.budget_exhausted &&
          fingerprint_exhausted.fingerprint_search_budget_exhausted &&
          !fingerprint_exhausted.insertion_chronology_budget_exhausted &&
          !fingerprint_exhausted.union_parent_hop_budget_exhausted &&
          !fingerprint_exhausted.structure_contract_rejected &&
          fingerprint_exhausted.fingerprint_search_slot_visit_count ==
              fingerprint_less_one
                  .maximum_fingerprint_search_slot_visit_count &&
          fingerprint_exhausted.insertion_chronology_slot_visit_count == 0U &&
          fingerprint_exhausted.union_parent_hop_count == 0U &&
          fingerprint_exhausted.decision ==
              ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
                  no_verification_fingerprint_search_budget_exhausted &&
          !fingerprint_exhausted.result_certified,
      "one fewer cumulative fingerprint-search visit stops verification before chronology");

  auto chronology_less_one = exact_verification_budget;
  --chronology_less_one.maximum_insertion_chronology_slot_visit_count;
  const auto chronology_exhausted =
      verify_structure(locator, locator.state_view(), chronology_less_one);
  check(
      chronology_exhausted.budget_exhausted &&
          !chronology_exhausted.fingerprint_search_budget_exhausted &&
          chronology_exhausted.insertion_chronology_budget_exhausted &&
          !chronology_exhausted.union_parent_hop_budget_exhausted &&
          chronology_exhausted
              .every_fingerprint_recomputed_and_full_key_located &&
          chronology_exhausted.insertion_chronology_slot_visit_count ==
              chronology_less_one
                  .maximum_insertion_chronology_slot_visit_count &&
          chronology_exhausted.union_parent_hop_count == 0U &&
          chronology_exhausted.decision ==
              ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
                  no_verification_insertion_chronology_budget_exhausted &&
          !chronology_exhausted.result_certified,
      "one fewer cumulative chronology visit stops verification before DSU replay");

  auto union_hop_less_one = exact_verification_budget;
  --union_hop_less_one.maximum_union_parent_hop_count;
  const auto union_hop_exhausted =
      verify_structure(locator, locator.state_view(), union_hop_less_one);
  check(
      union_hop_exhausted.budget_exhausted &&
          !union_hop_exhausted.fingerprint_search_budget_exhausted &&
          !union_hop_exhausted.insertion_chronology_budget_exhausted &&
          union_hop_exhausted.union_parent_hop_budget_exhausted &&
          union_hop_exhausted
              .committed_slot_insertion_chronology_freshly_replayed &&
          union_hop_exhausted.union_parent_hop_count ==
              union_hop_less_one.maximum_union_parent_hop_count &&
          union_hop_exhausted.decision ==
              ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
                  no_verification_union_parent_hop_budget_exhausted &&
          !union_hop_exhausted.result_certified,
      "one fewer cumulative union-parent hop stops verification during DSU replay");

  auto table_size_less_one = exact_verification_budget;
  --table_size_less_one.maximum_table_slot_count;
  const auto table_size_exhausted =
      verify_structure(locator, locator.state_view(), table_size_less_one);
  check(
      table_size_exhausted.scratch_requirement_arithmetic_certified &&
          !table_size_exhausted.budget_preflight_certified &&
          table_size_exhausted.budget_exhausted &&
          !table_size_exhausted.structure_contract_rejected &&
          table_size_exhausted.table_slot_scan_count == 0U &&
          table_size_exhausted.fingerprint_search_slot_visit_count == 0U &&
          table_size_exhausted.decision ==
              ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
                  no_verification_budget_preflight_exhausted,
      "an insufficient durable-size cap rejects before table scans or scratch allocation");

  auto scratch_population_less_one = exact_verification_budget;
  --scratch_population_less_one.maximum_binding_scratch_entry_count;
  const auto scratch_population_exhausted = verify_structure(
      locator, locator.state_view(), scratch_population_less_one);
  check(
      !scratch_population_exhausted.budget_preflight_certified &&
          scratch_population_exhausted.budget_exhausted &&
          scratch_population_exhausted.table_slot_scan_count == 0U &&
          scratch_population_exhausted.key_point_scan_count == 0U &&
          !scratch_population_exhausted
               .bounded_temporary_scratch_without_second_durable_output &&
          scratch_population_exhausted.decision ==
              ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
                  no_verification_budget_preflight_exhausted,
      "an insufficient scratch-population cap rejects before every variable scan and allocation");

  auto scratch_bytes_less_one = exact_verification_budget;
  --scratch_bytes_less_one.maximum_temporary_scratch_byte_count;
  const auto scratch_bytes_exhausted =
      verify_structure(locator, locator.state_view(), scratch_bytes_less_one);
  check(
      !scratch_bytes_exhausted.budget_preflight_certified &&
          scratch_bytes_exhausted.budget_exhausted &&
          scratch_bytes_exhausted.required_temporary_scratch_byte_count ==
              verified.required_temporary_scratch_byte_count &&
          scratch_bytes_exhausted.table_slot_scan_count == 0U &&
          scratch_bytes_exhausted.decision ==
              ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
                  no_verification_budget_preflight_exhausted,
      "one fewer exact scratch payload byte rejects before allocation");

  auto chronology_locator = make_locator(0U);
  const ExactDirectSparseFacetKey chronology_first = key({20U, 21U, 22U});
  const ExactDirectSparseFacetKey chronology_second = key({30U, 31U, 32U});
  const std::array<ExactDirectSparseFacetBinding, 2U>
      chronological_bindings{{
          {0U, chronology_first, 5U, witness(87U)},
          {1U, chronology_second, 6U, witness(88U)},
      }};
  check(
      chronology_locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              chronological_bindings)
          .certified_committed_batch(),
      "the hostile chronology fixture stores an ordered forced-collision cluster");
  std::vector<ExactDirectSparsePositiveFacetSlot> chronology_slots =
      chronology_locator.slots();
  check(
      chronology_slots.size() >= 2U && chronology_slots[0U].occupied &&
          chronology_slots[1U].occupied,
      "the zero fingerprint mask places the hostile fixture in the first two physical slots");
  if (chronology_slots.size() >= 2U) {
    std::swap(chronology_slots[0U], chronology_slots[1U]);
  }
  auto chronology_view = chronology_locator.state_view();
  chronology_view.slots =
      std::span<const ExactDirectSparsePositiveFacetSlot>{chronology_slots};
  const auto chronology_rejected =
      verify_structure(chronology_locator, chronology_view);
  check(
      chronology_rejected.trusted_construction_parameters_certified &&
          chronology_rejected.capacity_requirements_certified &&
          chronology_rejected.flat_table_and_key_arena_certified &&
          chronology_rejected
              .every_fingerprint_recomputed_and_full_key_located &&
          !chronology_rejected
               .committed_slot_insertion_chronology_freshly_replayed &&
          chronology_rejected.dense_handle_dsu_replay_certified &&
          chronology_rejected.union_witness_structure_certified &&
          chronology_rejected
              .historical_batch_assertions_and_counters_well_formed &&
          chronology_rejected.committed_history_digest_freshly_replayed &&
          chronology_rejected.internal_fact_fields_match_contract &&
          chronology_rejected.decision_and_scope_certified &&
          !chronology_rejected.external_authority_replayed_by_locator &&
          chronology_rejected
              .bounded_temporary_scratch_without_second_durable_output &&
          !chronology_rejected
               .fresh_durable_structure_verification_certified &&
          !chronology_rejected.result_certified,
      "a digest-consistent forced-collision cluster with swapped physical insertion chronology is rejected by the new fact alone");

  auto corrupted_digest_view = locator.state_view();
  auto corrupted_digest_bytes =
      corrupted_digest_view.committed_history_digest.bytes();
  corrupted_digest_bytes[0U] ^= UINT8_C(1);
  corrupted_digest_view.committed_history_digest =
      CanonicalId{corrupted_digest_bytes};
  const auto digest_rejected =
      verify_structure(locator, corrupted_digest_view);
  check(
      digest_rejected
          .historical_batch_assertions_and_counters_well_formed &&
          !digest_rejected.committed_history_digest_freshly_replayed &&
          !digest_rejected.fresh_durable_structure_verification_certified &&
          !digest_rejected.result_certified,
      "fresh structural verification rejects a corrupted state-view history digest after replaying otherwise valid history");

  std::vector<ExactDirectSparsePositiveFacetSlot> mutated_witness_slots =
      locator.slots();
  for (ExactDirectSparsePositiveFacetSlot& slot : mutated_witness_slots) {
    if (slot.occupied) {
      ++slot.binding_witness.replay_token;
      break;
    }
  }
  auto mutated_witness_view = locator.state_view();
  mutated_witness_view.slots =
      std::span<const ExactDirectSparsePositiveFacetSlot>{
          mutated_witness_slots};
  const auto witness_rejected =
      verify_structure(locator, mutated_witness_view);
  check(
      witness_rejected
          .historical_batch_assertions_and_counters_well_formed &&
          !witness_rejected.committed_history_digest_freshly_replayed &&
          !witness_rejected.fresh_durable_structure_verification_certified &&
          !witness_rejected.result_certified,
      "fresh digest replay rejects a structurally valid but mutated durable binding witness");

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
  const auto slot_rejected = verify_structure(locator, mutated_slot_view);
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
      verify_structure(locator, mutated_parent_view);
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
      verify_structure(locator, mutated_batch_view);
  check(
      !batch_rejected
           .historical_batch_assertions_and_counters_well_formed &&
          !batch_rejected.result_certified,
      "structural verification rejects an historical counter beyond its trusted per-batch cap");

  std::vector<ExactDirectSparseCommittedBatchRecord> hostile_batches(
      locator.budget().maximum_committed_batch_count,
      locator.committed_batches().front());
  for (std::size_t index = 0U; index < hostile_batches.size(); ++index) {
    hostile_batches[index].committed_batch_index = index;
    hostile_batches[index].input_shape_certified = false;
  }
  auto hostile_batch_view = locator.state_view();
  hostile_batch_view.committed_batches =
      std::span<const ExactDirectSparseCommittedBatchRecord>{hostile_batches};
  const auto hostile_batch_rejected =
      verify_structure(locator, hostile_batch_view);
  check(
      hostile_batch_rejected.structure_contract_rejected &&
          !hostile_batch_rejected.budget_exhausted &&
          hostile_batch_rejected.batch_record_scan_count == 1U &&
          hostile_batch_rejected.decision ==
              ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
                  no_verification_durable_structure_rejected &&
          !hostile_batch_rejected.result_certified,
      "the first malformed batch record fails before any later record can rescan the same binding prefix");

  std::vector<ExactDirectSparsePositiveFacetSlot> oversized_slots(
      locator.slots().size() + 1U);
  auto oversized_view = locator.state_view();
  oversized_view.slots =
      std::span<const ExactDirectSparsePositiveFacetSlot>{oversized_slots};
  const auto oversized_rejected = verify_structure(locator, oversized_view);
  check(
      !oversized_rejected.capacity_requirements_certified &&
          oversized_rejected.table_slot_scan_count == 0U &&
          !oversized_rejected.result_certified,
      "untrusted span sizes fail before scans or scratch allocations derived from them");

  auto oversized_binding_counter_view = locator.state_view();
  oversized_binding_counter_view.counters.inserted_binding_count =
      locator.budget().maximum_committed_binding_count + 1U;
  const auto oversized_binding_counter_rejected =
      verify_structure(locator, oversized_binding_counter_view);
  check(
      !oversized_binding_counter_rejected.capacity_requirements_certified &&
          oversized_binding_counter_rejected.structure_contract_rejected &&
          !oversized_binding_counter_rejected.budget_exhausted &&
          oversized_binding_counter_rejected.table_slot_scan_count == 0U &&
          oversized_binding_counter_rejected.decision ==
              ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
                  no_verification_capacity_requirements_rejected &&
          !oversized_binding_counter_rejected.result_certified,
      "an untrusted binding counter above the construction cap fails before sizing or allocating its scratch");

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
      verify_structure(empty_domain, forged_hit_view);
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
      verify_structure(empty_domain, forged_duplicate_view);
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
      verify_structure(comparison_lower_bound, forged_comparison_view);
  check(
      !forged_comparison_rejected
           .historical_batch_assertions_and_counters_well_formed &&
          !forged_comparison_rejected.result_certified,
      "a positive lookup without its mandatory full-key comparison is rejected");
}

}  // namespace

int main() {
  check_initialization_budget_and_external_scope();
  check_snapshot_stamp_tracks_only_committed_state();
  check_snapshot_digest_determinism_and_state_divergence();
  check_snapshot_digest_golden_vectors();
  check_strict_snapshot_and_unresolved_miss();
  check_forced_fingerprint_collision_and_full_rank_keys();
  check_public_fingerprint_is_bounded_and_canonical();
  check_union_indirection_without_key_rewrite();
  check_incompatible_duplicate_is_atomic_contradiction();
  check_explicit_union_makes_duplicate_compatible();
  check_shape_witness_and_durable_budget_rejections();
  check_const_budgeted_probe();
  check_prefix_stamp_sweep_and_exact_budgets();
  check_prefix_stamp_sweep_empty_invalid_stale_and_mutations();
  check_fresh_durable_structure_verifier_and_mutations();
  if (failures != 0) {
    std::cerr << failures << " direct sparse positive facet test(s) failed\n";
    return 1;
  }
  std::cout << "direct sparse positive facet locator tests passed\n";
  return 0;
}
