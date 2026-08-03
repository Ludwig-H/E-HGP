#include "morsehgp3d/hierarchy/direct_sparse_root_ledger.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace allocation_probe {

bool active = false;
std::size_t count = 0U;

void record() noexcept {
  if (active) {
    ++count;
  }
}

void begin() noexcept {
  count = 0U;
  active = true;
}

[[nodiscard]] std::size_t finish() noexcept {
  active = false;
  return count;
}

}  // namespace allocation_probe

void* operator new(std::size_t size) {
  allocation_probe::record();
  if (void* memory = std::malloc(size == 0U ? 1U : size); memory != nullptr) {
    return memory;
  }
  throw std::bad_alloc{};
}

void* operator new[](std::size_t size) {
  return ::operator new(size);
}

void operator delete(void* memory) noexcept {
  std::free(memory);
}

void operator delete[](void* memory) noexcept {
  ::operator delete(memory);
}

void operator delete(void* memory, std::size_t) noexcept {
  ::operator delete(memory);
}

void operator delete[](void* memory, std::size_t) noexcept {
  ::operator delete(memory);
}

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::contract::CanonicalId;
using morsehgp3d::contract::CanonicalSha256Builder;
using morsehgp3d::spatial::PointId;

template <typename T>
concept HasTraversalOrderMember = requires(const T& value) {
  value.traversal_order;
};

static_assert(direct_sparse_root_ledger_schema_version == 1U);
static_assert(direct_sparse_root_ledger_storage_schema_version == 1U);
static_assert(
    !std::is_copy_constructible_v<ExactDirectSparseRootLedger> &&
    std::is_nothrow_move_constructible_v<ExactDirectSparseRootLedger>);
static_assert(
    !std::is_copy_constructible_v<ExactDirectSparseRootLedgerPreparedBatch> &&
    std::is_nothrow_move_constructible_v<
        ExactDirectSparseRootLedgerPreparedBatch>);
static_assert(
    !std::is_aggregate_v<ExactDirectSparseRootLedgerLookupResult>);
static_assert(
    !std::is_aggregate_v<
        ExactDirectSparseRootLedgerActiveRootProbeResult>);
static_assert(
    !std::is_aggregate_v<
        ExactDirectSparseRootCoverageFromActiveRootProofsResult>);
static_assert(std::is_same_v<
              decltype(std::declval<const
                       ExactDirectSparseRootCoverageFromActiveRootProofsResult&>()
                           .records()),
              std::span<
                  const ExactDirectSparseRootCoverageMaterializationRecord>>);
static_assert(std::is_same_v<
              decltype(std::declval<const
                       ExactDirectSparseRootCoverageFromActiveRootProofsResult&>()
                           .point_ids()),
              std::span<const PointId>>);
static_assert(!HasTraversalOrderMember<
              ExactDirectSparseRootCoverageFromActiveRootProofsBudget>);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CanonicalId digest(std::string_view text) {
  CanonicalSha256Builder builder;
  builder.update(text);
  return builder.finalize();
}

[[nodiscard]] ExactDirectSparseFacetKey facet(PointId id) {
  ExactDirectSparseFacetKey key;
  key.point_count = 2U;
  key.point_ids[0U] = id;
  key.point_ids[1U] = std::numeric_limits<PointId>::max();
  return key;
}

[[nodiscard]] ExactDirectSparseStableFacetForestBudget forest_budget() {
  ExactDirectSparseStableFacetForestBudget budget;
  budget.maximum_observed_handle_count = 256U;
  budget.maximum_committed_key_point_count = 1024U;
  budget.maximum_committed_operation_count = 2048U;
  budget.maximum_committed_batch_count = 128U;
  budget.maximum_batch_insertion_request_count = 64U;
  budget.maximum_batch_union_request_count = 64U;
  budget.maximum_batch_key_point_count = 512U;
  budget.maximum_batch_operation_count = 256U;
  budget.maximum_staging_entry_count = 512U;
  budget.maximum_outstanding_ticket_count = 16U;
  return budget;
}

[[nodiscard]] ExactDirectSparseStableFacetForestPreparedPreviewBudget
preview_budget() {
  return {128U, 512U, 256U, 640U};
}

[[nodiscard]] ExactDirectSparseRootLedgerBudget ledger_budget() {
  ExactDirectSparseRootLedgerBudget budget;
  budget.maximum_active_root_count = 128U;
  budget.maximum_history_entry_count = 512U;
  budget.maximum_coverage_node_count = 512U;
  budget.maximum_coverage_parent_reference_count = 1024U;
  budget.maximum_coverage_point_delta_count = 2048U;
  budget.maximum_prior_root_parent_reference_count = 1024U;
  budget.maximum_committed_batch_count = 128U;
  budget.maximum_group_count = 64U;
  budget.maximum_standalone_birth_count = 64U;
  budget.maximum_parent_root_reference_count = 256U;
  budget.maximum_point_delta_count = 512U;
  budget.maximum_preview_record_count = 256U;
  budget.maximum_staging_entry_count = 2048U;
  budget.maximum_outstanding_ticket_count = 8U;
  return budget;
}

struct Context {
  ExactDirectSparseStableFacetForest forest;
  ExactDirectSparseRootLedger ledger;
};

[[nodiscard]] Context make_context(
    std::size_t namespace_size = 1'000U,
    ExactDirectSparseRootId first_root_id = 1U,
    std::uint64_t fingerprint_mask = ~std::uint64_t{0U},
    const ExactDirectSparseRootLedgerBudget& root_budget = ledger_budget()) {
  auto forest_initialization =
      initialize_exact_direct_sparse_stable_facet_forest(
          {namespace_size, digest("sparse-root-ledger-source"), 0U},
          forest_budget());
  check(
      forest_initialization.certified_initialized(),
      "forest initialization must certify");
  auto ledger_initialization = initialize_exact_direct_sparse_root_ledger(
      {first_root_id, fingerprint_mask, fingerprint_mask},
      root_budget,
      *forest_initialization.forest);
  check(
      ledger_initialization.certified_initialized(),
      "ledger initialization must certify");
  return {
      std::move(*forest_initialization.forest),
      std::move(*ledger_initialization.ledger)};
}

struct PreparedForest {
  ExactDirectSparseStableFacetForestPreparedBatch ticket;
  ExactDirectSparseStableFacetForestPreparedPreview preview;
};

[[nodiscard]] PreparedForest prepare_forest(
    ExactDirectSparseStableFacetForest& forest,
    std::span<const ExactDirectSparseStableFacetInsertion> insertions,
    std::span<const ExactDirectSparseStableFacetUnion> unions,
    std::span<const ExactDirectSparseStableFacetHandle> requested_handles) {
  auto preparation = forest.prepare_batch(insertions, unions);
  check(preparation.certified_prepared(), "forest preparation must certify");
  auto preview = forest.preview_prepared_batch(
      *preparation.ticket, requested_handles, preview_budget());
  check(
      preview.certified_prepared_preview(),
      "forest sparse shadow preview must certify");
  return {
      std::move(*preparation.ticket), std::move(*preview.preview)};
}

[[nodiscard]] ExactDirectSparseRootLedgerCommitResult commit_births(
    Context& context,
    std::span<const ExactDirectSparseStableFacetHandle> handles,
    std::span<const ExactDirectSparseRootLedgerStandaloneBirth> births,
    std::span<const PointId> points,
    bool probe_commit = false) {
  std::vector<ExactDirectSparseStableFacetInsertion> insertions;
  insertions.reserve(handles.size());
  for (const auto handle : handles) {
    insertions.push_back({handle, facet(static_cast<PointId>(handle))});
  }
  auto prepared_forest = prepare_forest(
      context.forest, insertions, {}, handles);
  auto prepared_ledger = context.ledger.prepare_transition(
      context.forest,
      std::move(prepared_forest.ticket),
      std::move(prepared_forest.preview),
      {},
      {},
      {},
      births,
      points);
  check(
      prepared_ledger.certified_prepared(),
      "standalone-birth ledger preparation must certify");
  if (probe_commit) {
    allocation_probe::begin();
  }
  auto commit = context.ledger.commit(
      std::move(*prepared_ledger.ticket), context.forest);
  const std::size_t allocations = probe_commit ? allocation_probe::finish() : 0U;
  check(commit.certified_commit(), "standalone-birth commit must certify");
  if (probe_commit) {
    check(allocations == 0U, "commit must perform no allocation");
  }
  return commit;
}

[[nodiscard]] ExactDirectSparseRootLedgerCommitResult commit_group(
    Context& context,
    std::span<const ExactDirectSparseStableFacetInsertion> insertions,
    std::span<const ExactDirectSparseStableFacetUnion> unions,
    std::span<const ExactDirectSparseStableFacetHandle> preview_handles,
    const ExactDirectSparseRootLedgerGroupTransition& group,
    std::span<const ExactDirectSparseStableFacetHandle> parents,
    std::span<const PointId> point_delta,
    bool probe_commit = false) {
  auto prepared_forest = prepare_forest(
      context.forest, insertions, unions, preview_handles);
  const std::array groups{group};
  auto prepared_ledger = context.ledger.prepare_transition(
      context.forest,
      std::move(prepared_forest.ticket),
      std::move(prepared_forest.preview),
      groups,
      parents,
      point_delta,
      {},
      {});
  check(
      prepared_ledger.certified_prepared(),
      "group ledger preparation must certify");
  if (probe_commit) {
    allocation_probe::begin();
  }
  auto commit = context.ledger.commit(
      std::move(*prepared_ledger.ticket), context.forest);
  const std::size_t allocations = probe_commit ? allocation_probe::finish() : 0U;
  check(commit.certified_commit(), "group commit must certify");
  if (probe_commit) {
    check(allocations == 0U, "group commit must perform no allocation");
  }
  return commit;
}

void test_qr_transitions_coverage_and_migration() {
  auto context = make_context(1'000U, 1U, 0U);
  check(
      context.ledger.materialized_active_handle_index_slot_count() == 0U &&
          context.ledger.materialized_active_root_id_index_slot_count() == 0U,
      "empty ledger must materialize neither sparse index");

  const std::array<ExactDirectSparseStableFacetHandle, 3U> birth_handles{
      10U, 30U, 90U};
  const std::array births{
      ExactDirectSparseRootLedgerStandaloneBirth{10U, 0U, 2U, 1U},
      ExactDirectSparseRootLedgerStandaloneBirth{30U, 2U, 2U, 1U},
      ExactDirectSparseRootLedgerStandaloneBirth{90U, 4U, 1U, 1U},
  };
  const std::array<PointId, 5U> birth_points{1U, 2U, 4U, 7U, 99U};
  const auto births_commit = commit_births(
      context, birth_handles, births, birth_points, true);
  check(
      births_commit.latent_birth_count == 3U &&
          births_commit.allocated_root_id_count == 0U,
      "standalone births must stay latent and allocate no root id");
  for (const auto handle : birth_handles) {
    const auto lookup = context.ledger.lookup_active_root(handle);
    check(
        lookup.certified_observed() && !lookup.entry.prior_root_id.has_value(),
        "each standalone birth must be an active latent binding");
  }

  const std::array q0_insertions{
      ExactDirectSparseStableFacetInsertion{11U, facet(11U)}};
  const std::array q0_unions{ExactDirectSparseStableFacetUnion{10U, 11U}};
  const std::array<ExactDirectSparseStableFacetHandle, 2U> q0_preview{
      10U, 11U};
  const std::array<ExactDirectSparseStableFacetHandle, 1U> q0_parents{10U};
  const std::array<PointId, 2U> q0_delta{2U, 3U};
  const auto q0_commit = commit_group(
      context,
      q0_insertions,
      q0_unions,
      q0_preview,
      {0U, 11U, 0U, 1U, 0U, 2U, 1U},
      q0_parents,
      q0_delta,
      true);
  check(
      q0_commit.allocated_root_id_count == 1U &&
          context.ledger.lookup_active_root(10U).entry.prior_root_id == 1U,
      "q_R=0 over a latent carrier must allocate root id 1");

  // The two new equal-side handles first form a size-two component.  The
  // stable forest's union-by-size/tie-smallest rule then migrates the old
  // rooted component from handle 10 to handle 1.  The ledger must follow the
  // sealed preview and preserve id 1, not force the old/min-parent handle.
  const std::array q1_insertions{
      ExactDirectSparseStableFacetInsertion{1U, facet(1U)},
      ExactDirectSparseStableFacetInsertion{2U, facet(2U)},
  };
  const std::array q1_unions{
      ExactDirectSparseStableFacetUnion{1U, 2U},
      ExactDirectSparseStableFacetUnion{1U, 10U},
  };
  const std::array<ExactDirectSparseStableFacetHandle, 3U> q1_preview{
      1U, 2U, 10U};
  const std::array<ExactDirectSparseStableFacetHandle, 1U> q1_parents{10U};
  const auto q1_commit = commit_group(
      context,
      q1_insertions,
      q1_unions,
      q1_preview,
      {0U, 1U, 0U, 1U, 0U, 0U, 2U},
      q1_parents,
      {},
      true);
  const auto migrated = context.ledger.lookup_active_root(1U);
  check(
      q1_commit.preserved_root_id_count == 1U &&
          migrated.certified_observed() && migrated.entry.prior_root_id == 1U &&
          migrated.entry.coverage_reused &&
          migrated.entry.facet_delta_count == 2U &&
          migrated.entry.point_delta_count == 0U &&
          context.ledger.lookup_active_root(10U).certified_unobserved(),
      "q_R=1 must preserve id, migrate to preview root and retain an action "
      "record when point delta is empty");

  const std::array q0b_insertions{
      ExactDirectSparseStableFacetInsertion{31U, facet(31U)}};
  const std::array q0b_unions{ExactDirectSparseStableFacetUnion{30U, 31U}};
  const std::array<ExactDirectSparseStableFacetHandle, 2U> q0b_preview{
      30U, 31U};
  const std::array<ExactDirectSparseStableFacetHandle, 1U> q0b_parents{30U};
  const std::array<PointId, 1U> q0b_delta{5U};
  (void)commit_group(
      context,
      q0b_insertions,
      q0b_unions,
      q0b_preview,
      {0U, 31U, 0U, 1U, 0U, 1U, 1U},
      q0b_parents,
      q0b_delta);
  check(
      context.ledger.lookup_active_root(30U).entry.prior_root_id == 2U,
      "second q_R=0 transition must allocate monotone id 2");

  const std::array multi_insertions{
      ExactDirectSparseStableFacetInsertion{50U, facet(50U)}};
  const std::array multi_unions{
      ExactDirectSparseStableFacetUnion{1U, 30U},
      ExactDirectSparseStableFacetUnion{1U, 50U},
  };
  const std::array<ExactDirectSparseStableFacetHandle, 3U> multi_preview{
      1U, 30U, 50U};
  const std::array<ExactDirectSparseStableFacetHandle, 2U> multi_parents{
      1U, 30U};
  const auto multi_commit = commit_group(
      context,
      multi_insertions,
      multi_unions,
      multi_preview,
      {0U, 50U, 0U, 2U, 0U, 0U, 1U},
      multi_parents,
      {},
      true);
  const auto final_root = context.ledger.lookup_active_root(1U);
  check(
      multi_commit.allocated_root_id_count == 1U &&
          final_root.certified_observed() &&
          final_root.entry.prior_root_id == 3U && final_root.entry.q_r == 2U &&
          context.ledger.lookup_active_root_id(1U).certified_unobserved() &&
          context.ledger.lookup_active_root_id(2U).certified_unobserved() &&
          context.ledger.lookup_active_root_id(3U).entry.forest_root_handle ==
              1U,
      "q_R>1 must retire both active ids, allocate id 3 and preserve sparse "
      "handle/id bijections");

  check(
      context.ledger.history_entries().size() == 7U &&
          context.ledger.coverage_nodes().size() == 6U &&
          context.ledger.coverage_parent_references().size() == 4U &&
          context.ledger.prior_root_parent_references().size() == 3U &&
          context.ledger.prior_root_parent_references()[1U] == 1U &&
          context.ledger.prior_root_parent_references()[2U] == 2U,
      "history must retain every action while q1 reuses one coverage version");

  const std::array<ExactDirectSparseStableFacetHandle, 1U> requested{1U};
  const auto materialized = context.ledger.materialize_active_coverages(
      requested, {1U, 16U, 16U, 16U, 16U, 64U});
  const std::array<PointId, 6U> expected_points{1U, 2U, 3U, 4U, 5U, 7U};
  check(
      materialized.certified_requested_only_materialization() &&
          materialized.records.size() == 1U &&
          std::equal(
              materialized.point_ids.begin(),
              materialized.point_ids.end(),
              expected_points.begin(),
              expected_points.end()) &&
          materialized.reachable_coverage_node_count == 5U &&
          materialized.point_delta_scan_count == 7U &&
          std::find(
              materialized.point_ids.begin(),
              materialized.point_ids.end(),
              PointId{99U}) == materialized.point_ids.end(),
      "bounded materialization must match the explicit DAG oracle and skip "
      "the unrelated latent coverage");

  auto tampered_lookup = final_root;
  tampered_lookup.entry.forest_root_handle = 999U;
  check(
      !tampered_lookup.certified_observed(),
      "public lookup edits must invalidate the private inline snapshot");
  ExactDirectSparseRootLedgerLookupResult forged;
  forged.disposition =
      ExactDirectSparseRootLedgerLookupDisposition::observed_rooted;
  forged.requested_key_kind =
      ExactDirectSparseRootLedgerLookupKeyKind::prior_root_id;
  forged.requested_prior_root_id = 3U;
  forged.entry.prior_root_id = 3U;
  forged.entry.coverage_id = 1U;
  forged.ledger_certified_at_entry = true;
  forged.sparse_active_index_authoritative = true;
  forged.lookup_completed_without_mutation = true;
  check(!forged.certified_observed(), "default lookup must be unforgeable");

  const auto missing_handle = context.ledger.lookup_active_root(777U);
  const auto missing_id = context.ledger.lookup_active_root_id(777U);
  check(
      missing_handle.certified_unobserved() &&
          missing_handle.requested_key_kind ==
              ExactDirectSparseRootLedgerLookupKeyKind::forest_root_handle &&
          missing_handle.requested_forest_root_handle == 777U &&
          !missing_handle.requested_prior_root_id.has_value() &&
          missing_id.certified_unobserved() &&
          missing_id.requested_key_kind ==
              ExactDirectSparseRootLedgerLookupKeyKind::prior_root_id &&
          missing_id.requested_prior_root_id == 777U,
      "handle and PriorRootId misses must retain distinct exact query keys");
  auto tampered_id_hit = context.ledger.lookup_active_root_id(3U);
  tampered_id_hit.requested_prior_root_id = 4U;
  check(
      !tampered_id_hit.certified_observed(),
      "PriorRootId hit certification must bind the requested id");

  const auto checkpoint = context.ledger.checkpoint();
  check(
      checkpoint.certified_honest_nonrestartable() &&
          !checkpoint.active_indexes_serialized &&
          !checkpoint.history_entries_serialized &&
          !checkpoint.coverage_dag_serialized,
      "checkpoint must be explicit semantic metadata, never a restart image");
}

void test_bounded_active_root_probe() {
  // A zero fingerprint mask forces every active root into the same physical
  // linear-probing cluster while exact handle equality remains authoritative.
  auto context = make_context(1'000U, 1U, 0U);
  const std::array<ExactDirectSparseStableFacetHandle, 2U> birth_handles{
      10U, 30U};
  const std::array births{
      ExactDirectSparseRootLedgerStandaloneBirth{10U, 0U, 1U, 1U},
      ExactDirectSparseRootLedgerStandaloneBirth{30U, 1U, 1U, 1U},
  };
  const std::array<PointId, 2U> birth_points{1U, 2U};
  (void)commit_births(context, birth_handles, births, birth_points);

  const std::array rooted_insertions{
      ExactDirectSparseStableFacetInsertion{11U, facet(11U)}};
  const std::array rooted_unions{
      ExactDirectSparseStableFacetUnion{10U, 11U}};
  const std::array<ExactDirectSparseStableFacetHandle, 2U> rooted_preview{
      10U, 11U};
  const std::array<ExactDirectSparseStableFacetHandle, 1U> rooted_parent{10U};
  (void)commit_group(
      context,
      rooted_insertions,
      rooted_unions,
      rooted_preview,
      {0U, 11U, 0U, 1U, 0U, 0U, 1U},
      rooted_parent,
      {});

  check(
      context.ledger.materialized_active_handle_index_slot_count() == 8U,
      "collision probe fixture must retain the minimum physical root index");
  const auto pre_stamp = context.ledger.current_stamp();
  const std::vector<ExactDirectSparseRootLedgerEntry> history_before{
      context.ledger.history_entries().begin(),
      context.ledger.history_entries().end()};
  const std::vector<ExactDirectSparseRootCoverageNode> coverage_before{
      context.ledger.coverage_nodes().begin(),
      context.ledger.coverage_nodes().end()};
  const std::vector<ExactDirectSparseRootCoverageId>
      coverage_parents_before{
          context.ledger.coverage_parent_references().begin(),
          context.ledger.coverage_parent_references().end()};
  const std::vector<PointId> coverage_deltas_before{
      context.ledger.coverage_point_deltas().begin(),
      context.ledger.coverage_point_deltas().end()};
  const std::vector<ExactDirectSparseRootId> prior_root_parents_before{
      context.ledger.prior_root_parent_references().begin(),
      context.ledger.prior_root_parent_references().end()};

  allocation_probe::begin();
  const auto rooted = context.ledger.probe_active_root(
      10U, ExactDirectSparseRootLedgerActiveRootProbeBudget{1U});
  const std::size_t rooted_allocations = allocation_probe::finish();
  check(
      rooted_allocations == 0U && rooted.certified_observed() &&
          rooted.certified_observed_rooted() &&
          !rooted.certified_observed_latent() &&
          rooted.certified_for(pre_stamp) &&
          rooted.entry.prior_root_id == 1U &&
          rooted.root_index_slot_visit_count == 1U &&
          rooted.exact_key_comparison_count == 1U,
      "bounded root probe must certify an allocation-free rooted hit at its "
      "exact cap");

  allocation_probe::begin();
  const auto latent = context.ledger.probe_active_root(
      30U, ExactDirectSparseRootLedgerActiveRootProbeBudget{2U});
  const std::size_t latent_allocations = allocation_probe::finish();
  check(
      latent_allocations == 0U && latent.certified_observed() &&
          latent.certified_observed_latent() &&
          !latent.certified_observed_rooted() &&
          latent.certified_for(pre_stamp) &&
          !latent.entry.prior_root_id.has_value() &&
          latent.root_index_slot_visit_count == 2U &&
          latent.exact_key_comparison_count == 2U,
      "forced collisions must retain exact latent semantics at cap");

  allocation_probe::begin();
  const auto missing = context.ledger.probe_active_root(
      777U, ExactDirectSparseRootLedgerActiveRootProbeBudget{3U});
  const std::size_t missing_allocations = allocation_probe::finish();
  check(
      missing_allocations == 0U && missing.certified_unobserved() &&
          missing.certified_for(pre_stamp) &&
          missing.entry == ExactDirectSparseRootLedgerEntry{} &&
          missing.root_index_slot_visit_count == 3U &&
          missing.exact_key_comparison_count == 2U,
      "a bounded collision-chain miss must stop only at the authoritative "
      "empty slot");

  allocation_probe::begin();
  const auto rooted_zero = context.ledger.probe_active_root(
      10U, ExactDirectSparseRootLedgerActiveRootProbeBudget{0U});
  const auto latent_cap_minus_one = context.ledger.probe_active_root(
      30U, ExactDirectSparseRootLedgerActiveRootProbeBudget{1U});
  const auto missing_cap_minus_one = context.ledger.probe_active_root(
      777U, ExactDirectSparseRootLedgerActiveRootProbeBudget{2U});
  const auto maximal_missing = context.ledger.probe_active_root(
      std::numeric_limits<ExactDirectSparseStableFacetHandle>::max(),
      ExactDirectSparseRootLedgerActiveRootProbeBudget{
          std::numeric_limits<std::size_t>::max()});
  const std::size_t remaining_probe_allocations = allocation_probe::finish();
  check(
      remaining_probe_allocations == 0U &&
          rooted_zero.certified_budget_exhaustion() &&
          rooted_zero.certified_for(pre_stamp) &&
          rooted_zero.root_index_slot_visit_count == 0U &&
          rooted_zero.entry == ExactDirectSparseRootLedgerEntry{} &&
          latent_cap_minus_one.certified_budget_exhaustion() &&
          latent_cap_minus_one.root_index_slot_visit_count == 1U &&
          latent_cap_minus_one.entry == ExactDirectSparseRootLedgerEntry{} &&
          missing_cap_minus_one.certified_budget_exhaustion() &&
          missing_cap_minus_one.root_index_slot_visit_count == 2U &&
          missing_cap_minus_one.entry ==
              ExactDirectSparseRootLedgerEntry{} &&
          maximal_missing.certified_unobserved() &&
          maximal_missing.requested_forest_root_handle ==
              std::numeric_limits<
                  ExactDirectSparseStableFacetHandle>::max() &&
          maximal_missing.requested_budget
                  .maximum_root_index_slot_visit_count ==
              std::numeric_limits<std::size_t>::max(),
      "zero and cap-minus-one probes must fail closed at the exact slot cap "
      "without payload; every disposition and SIZE_MAX stay allocation-free");

  const auto unchanged = [](const auto& before, const auto current) {
    return before.size() == current.size() &&
           std::equal(
               before.begin(), before.end(), current.begin(), current.end());
  };
  check(
      context.ledger.current_stamp() == pre_stamp &&
          unchanged(history_before, context.ledger.history_entries()) &&
          unchanged(coverage_before, context.ledger.coverage_nodes()) &&
          unchanged(
              coverage_parents_before,
              context.ledger.coverage_parent_references()) &&
          unchanged(
              coverage_deltas_before,
              context.ledger.coverage_point_deltas()) &&
          unchanged(
              prior_root_parents_before,
              context.ledger.prior_root_parent_references()) &&
          context.ledger.lookup_active_root(10U).certified_observed() &&
          context.ledger.lookup_active_root(30U).certified_observed() &&
          context.ledger.lookup_active_root(777U).certified_unobserved(),
      "bounded probes must not mutate the ledger and historical lookup "
      "behavior must remain intact");

  auto tampered = rooted;
  tampered.entry.prior_root_id.reset();
  auto comparison_tamper = rooted;
  comparison_tamper.exact_key_comparison_count = 0U;
  auto payload_tamper = rooted_zero;
  payload_tamper.entry = rooted.entry;
  ExactDirectSparseRootLedgerActiveRootProbeResult forged;
  forged.ledger_stamp = pre_stamp;
  forged.requested_forest_root_handle = 10U;
  forged.requested_budget = {1U};
  forged.entry = rooted.entry;
  forged.root_index_slot_visit_count = 1U;
  forged.exact_key_comparison_count = 1U;
  forged.ledger_certified_at_entry = true;
  forged.sparse_active_root_index_authoritative = true;
  forged.fingerprint_used_only_as_accelerator = true;
  forged.exact_forest_root_handle_comparison_authoritative = true;
  forged.lookup_completed_without_mutation = true;
  forged.disposition =
      ExactDirectSparseRootLedgerActiveRootProbeDisposition::observed_rooted;
  forged.decision =
      ExactDirectSparseRootLedgerActiveRootProbeDecision::
          complete_observed_rooted;
  check(
      !tampered.certified_observed() &&
          !comparison_tamper.certified_observed() &&
          !payload_tamper.certified_budget_exhaustion() &&
          !forged.certified_observed(),
      "private probe minting must reject public tamper, injected payload and "
      "forgery");

  auto foreign = make_context(1'000U, 1U, 0U);
  check(
      !rooted.certified_for(foreign.ledger.current_stamp()),
      "a privately minted probe must reject a foreign ledger stamp");

  const std::array<ExactDirectSparseStableFacetHandle, 1U> later_handle{50U};
  const std::array later_birth{
      ExactDirectSparseRootLedgerStandaloneBirth{50U, 0U, 1U, 1U}};
  const std::array<PointId, 1U> later_point{3U};
  (void)commit_births(context, later_handle, later_birth, later_point);
  check(
      rooted.certified_for(pre_stamp) &&
          !rooted.certified_for(context.ledger.current_stamp()),
      "a privately minted probe must not certify for a stale successor stamp");
}

void test_proof_bound_active_root_materialization() {
  const auto make_fixture = []() {
    // The zero fingerprint mask makes requested proof work observable: the
    // rooted handle occupies the first slot and the latent handle the second.
    auto context = make_context(1'000U, 1U, 0U);
    const std::array<ExactDirectSparseStableFacetHandle, 2U> birth_handles{
        10U, 30U};
    const std::array births{
        ExactDirectSparseRootLedgerStandaloneBirth{10U, 0U, 2U, 1U},
        ExactDirectSparseRootLedgerStandaloneBirth{30U, 2U, 2U, 1U},
    };
    const std::array<PointId, 4U> birth_points{1U, 2U, 4U, 7U};
    (void)commit_births(context, birth_handles, births, birth_points);

    const std::array insertions{
        ExactDirectSparseStableFacetInsertion{11U, facet(11U)}};
    const std::array unions{
        ExactDirectSparseStableFacetUnion{10U, 11U}};
    const std::array<ExactDirectSparseStableFacetHandle, 2U> preview_handles{
        10U, 11U};
    const std::array<ExactDirectSparseStableFacetHandle, 1U> parents{10U};
    const std::array<PointId, 2U> point_delta{2U, 3U};
    (void)commit_group(
        context,
        insertions,
        unions,
        preview_handles,
        {0U, 11U, 0U, 1U, 0U, 2U, 1U},
        parents,
        point_delta);
    return context;
  };

  auto context = make_fixture();
  const auto pre_stamp = context.ledger.current_stamp();
  const std::vector<ExactDirectSparseRootLedgerEntry> history_before{
      context.ledger.history_entries().begin(),
      context.ledger.history_entries().end()};
  const std::vector<ExactDirectSparseRootCoverageNode> coverage_before{
      context.ledger.coverage_nodes().begin(),
      context.ledger.coverage_nodes().end()};
  const std::vector<ExactDirectSparseRootCoverageId>
      coverage_parents_before{
          context.ledger.coverage_parent_references().begin(),
          context.ledger.coverage_parent_references().end()};
  const std::vector<PointId> coverage_deltas_before{
      context.ledger.coverage_point_deltas().begin(),
      context.ledger.coverage_point_deltas().end()};
  const std::vector<ExactDirectSparseRootId> prior_root_parents_before{
      context.ledger.prior_root_parent_references().begin(),
      context.ledger.prior_root_parent_references().end()};

  const auto rooted = context.ledger.probe_active_root(
      10U, ExactDirectSparseRootLedgerActiveRootProbeBudget{1U});
  const auto latent = context.ledger.probe_active_root(
      30U, ExactDirectSparseRootLedgerActiveRootProbeBudget{2U});
  const auto missing = context.ledger.probe_active_root(
      777U, ExactDirectSparseRootLedgerActiveRootProbeBudget{3U});
  const auto exhausted = context.ledger.probe_active_root(
      30U, ExactDirectSparseRootLedgerActiveRootProbeBudget{1U});
  check(
      rooted.certified_observed_rooted() &&
          rooted.root_index_slot_visit_count == 1U &&
          rooted.exact_key_comparison_count == 1U &&
          latent.certified_observed_latent() &&
          latent.root_index_slot_visit_count == 2U &&
          latent.exact_key_comparison_count == 2U &&
          missing.certified_unobserved() &&
          exhausted.certified_budget_exhaustion(),
      "proof materialization fixture must expose rooted, latent, miss and "
      "forced-collision exhaustion");

  const std::array<ExactDirectSparseStableFacetHandle, 2U> requested_handles{
      10U, 30U};
  const ExactDirectSparseRootCoverageMaterializationBudget coverage_budget{
      2U, 3U, 1U, 6U, 5U, 6U};
  const auto historical = context.ledger.materialize_active_coverages(
      requested_handles, coverage_budget);
  const std::array<PointId, 5U> expected_points{1U, 2U, 3U, 4U, 7U};
  check(
      historical.certified_requested_only_materialization() &&
          historical.records.size() == 2U &&
          historical.records[0U].forest_root_handle == 10U &&
          historical.records[0U].prior_root_id == 1U &&
          historical.records[0U].coverage_id == 3U &&
          historical.records[0U].point_offset == 0U &&
          historical.records[0U].point_count == 3U &&
          historical.records[1U].forest_root_handle == 30U &&
          !historical.records[1U].prior_root_id.has_value() &&
          historical.records[1U].coverage_id == 2U &&
          historical.records[1U].point_offset == 3U &&
          historical.records[1U].point_count == 2U &&
          historical.requested_root_count == 2U &&
          historical.reachable_coverage_node_count == 3U &&
          historical.parent_reference_scan_count == 1U &&
          historical.point_delta_scan_count == 6U &&
          historical.scratch_entry_count == 6U &&
          std::equal(
              historical.point_ids.begin(),
              historical.point_ids.end(),
              expected_points.begin(),
              expected_points.end()),
      "historical v1 materialization must expose the exact two-root oracle "
      "and per-root traversal counters");

  const auto rooted_copy = rooted;
  const std::array proofs{rooted_copy, latent};
  const auto proofs_before = proofs;
  const ExactDirectSparseRootCoverageFromActiveRootProofsBudget exact_budget{
      2U, 3U, 3U, 3U, 2U, coverage_budget};
  const ExactDirectSparseRootCoverageFromActiveRootProofsCounters
      exact_counters{2U, 3U, 3U, 3U, 2U};
  const auto materialized =
      context.ledger.materialize_active_coverages_from_active_root_proofs(
          proofs, exact_budget);
  check(
      rooted_copy.certified_observed_rooted() &&
          materialized.certified_requested_only_materialization() &&
          materialized.certified_outcome() &&
          materialized.certified_for(pre_stamp) &&
          materialized.disposition() ==
              ExactDirectSparseRootCoverageFromActiveRootProofsDisposition::
                  complete &&
          materialized.decision() ==
              ExactDirectSparseRootCoverageFromActiveRootProofsDecision::
                  complete_requested_roots_only &&
          materialized.coverage_decision() ==
              ExactDirectSparseRootCoverageMaterializationDecision::
                  complete_requested_roots_only &&
          materialized.requested_budget() == exact_budget &&
          materialized.counters() == exact_counters &&
          materialized.no_historical_active_root_lookup() &&
          materialized.records().size() == historical.records.size() &&
          std::equal(
              materialized.records().begin(),
              materialized.records().end(),
              historical.records.begin(),
              historical.records.end()) &&
          materialized.point_ids().size() == historical.point_ids.size() &&
          std::equal(
              materialized.point_ids().begin(),
              materialized.point_ids().end(),
              historical.point_ids.begin(),
              historical.point_ids.end()),
      "canonical rooted and latent proofs must reproduce v1 records, "
      "offsets, points and exact aggregate work without a second lookup");
  check(
      proofs == proofs_before,
      "proof-bound materialization must not modify authentic input proofs");

  const auto copied_materialization = materialized;
  ExactDirectSparseRootCoverageFromActiveRootProofsResult assigned;
  check(
      !assigned.certified_outcome() && assigned.records().empty() &&
          assigned.point_ids().empty(),
      "default proof-bound materialization cannot mint a certified payload");
  assigned = copied_materialization;
  check(
      copied_materialization.certified_requested_only_materialization() &&
          assigned.certified_requested_only_materialization() &&
          assigned.certified_for(pre_stamp) &&
          assigned.records().size() == materialized.records().size() &&
          assigned.point_ids().size() == materialized.point_ids().size(),
      "copy construction and assignment must preserve an authentic private "
      "materialization capability");

  const ExactDirectSparseRootCoverageFromActiveRootProofsBudget zero_budget{};
  const auto empty =
      context.ledger.materialize_active_coverages_from_active_root_proofs(
          {}, zero_budget);
  check(
      empty.certified_requested_only_materialization() &&
          empty.certified_for(pre_stamp) &&
          empty.requested_budget() == zero_budget &&
          empty.counters() ==
              ExactDirectSparseRootCoverageFromActiveRootProofsCounters{} &&
          empty.records().empty() && empty.point_ids().empty() &&
          empty.no_historical_active_root_lookup(),
      "an empty canonical proof set must complete under all-zero caps");

  const auto payloadless = [](const auto& result) {
    return result.records().empty() && result.point_ids().empty() &&
           result.no_historical_active_root_lookup();
  };
  const auto expect_proof_budget_exhaustion =
      [&](const ExactDirectSparseRootCoverageFromActiveRootProofsBudget& budget,
          ExactDirectSparseRootCoverageFromActiveRootProofsDecision decision,
          const ExactDirectSparseRootCoverageFromActiveRootProofsCounters&
              expected_counters,
          const std::string& message) {
        const auto result =
            context.ledger.materialize_active_coverages_from_active_root_proofs(
                proofs, budget);
        check(
            result.certified_budget_exhaustion() &&
                result.certified_outcome() && result.certified_for(pre_stamp) &&
                result.disposition() ==
                    ExactDirectSparseRootCoverageFromActiveRootProofsDisposition::
                        budget_exhausted &&
                result.decision() == decision &&
                result.counters() == expected_counters &&
                payloadless(result),
            message);
      };

  auto reduced = exact_budget;
  --reduced.maximum_proof_count;
  expect_proof_budget_exhaustion(
      reduced,
      ExactDirectSparseRootCoverageFromActiveRootProofsDecision::
          no_proof_count_budget_exhausted,
      {1U, 0U, 0U, 0U, 0U},
      "proof-count cap minus one must reject without scientific payload");
  reduced = exact_budget;
  --reduced.maximum_aggregate_requested_root_index_slot_visit_count;
  expect_proof_budget_exhaustion(
      reduced,
      ExactDirectSparseRootCoverageFromActiveRootProofsDecision::
          no_aggregate_requested_slot_budget_exhausted,
      {2U, 2U, 0U, 0U, 0U},
      "aggregate requested-slot cap minus one must reject without payload");
  reduced = exact_budget;
  --reduced.maximum_aggregate_root_index_slot_visit_count;
  expect_proof_budget_exhaustion(
      reduced,
      ExactDirectSparseRootCoverageFromActiveRootProofsDecision::
          no_aggregate_slot_visit_budget_exhausted,
      {2U, 3U, 2U, 0U, 0U},
      "aggregate actual-visit cap minus one must reject without payload");
  reduced = exact_budget;
  --reduced.maximum_aggregate_exact_key_comparison_count;
  expect_proof_budget_exhaustion(
      reduced,
      ExactDirectSparseRootCoverageFromActiveRootProofsDecision::
          no_aggregate_exact_comparison_budget_exhausted,
      {2U, 3U, 3U, 2U, 0U},
      "aggregate exact-comparison cap minus one must reject without payload");
  reduced = exact_budget;
  --reduced.maximum_direct_history_entry_access_count;
  expect_proof_budget_exhaustion(
      reduced,
      ExactDirectSparseRootCoverageFromActiveRootProofsDecision::
          no_history_access_budget_exhausted,
      {2U, 3U, 3U, 3U, 1U},
      "direct-history-access cap minus one must reject without payload");

  const auto expect_coverage_budget_exhaustion =
      [&](const ExactDirectSparseRootCoverageFromActiveRootProofsBudget& budget,
          const std::string& message) {
        const auto result =
            context.ledger.materialize_active_coverages_from_active_root_proofs(
                proofs, budget);
        check(
            result.certified_budget_exhaustion() &&
                result.certified_outcome() && result.certified_for(pre_stamp) &&
                result.decision() ==
                    ExactDirectSparseRootCoverageFromActiveRootProofsDecision::
                        no_coverage_budget_exhausted &&
                result.coverage_decision() ==
                    ExactDirectSparseRootCoverageMaterializationDecision::
                        no_budget_exhausted &&
                result.counters() == exact_counters &&
                payloadless(result),
            message);
      };
  reduced = exact_budget;
  --reduced.coverage.maximum_requested_root_count;
  expect_coverage_budget_exhaustion(
      reduced, "v1 requested-root cap minus one must remain exact");
  reduced = exact_budget;
  --reduced.coverage.maximum_reachable_coverage_node_count;
  expect_coverage_budget_exhaustion(
      reduced, "v1 reachable-node cap minus one must remain exact");
  reduced = exact_budget;
  --reduced.coverage.maximum_parent_reference_scan_count;
  expect_coverage_budget_exhaustion(
      reduced, "v1 parent-reference cap minus one must remain exact");
  reduced = exact_budget;
  --reduced.coverage.maximum_point_delta_scan_count;
  expect_coverage_budget_exhaustion(
      reduced, "v1 point-delta cap minus one must remain exact");
  reduced = exact_budget;
  --reduced.coverage.maximum_output_point_count;
  expect_coverage_budget_exhaustion(
      reduced, "v1 output-point cap minus one must remain exact");
  reduced = exact_budget;
  --reduced.coverage.maximum_scratch_entry_count;
  expect_coverage_budget_exhaustion(
      reduced, "v1 scratch cap minus one must remain exact");

  const auto expect_proof_rejection =
      [&](std::span<const ExactDirectSparseRootLedgerActiveRootProbeResult>
              rejected_proofs,
          ExactDirectSparseRootCoverageFromActiveRootProofsDecision decision,
          const std::string& message) {
        const auto result =
            context.ledger.materialize_active_coverages_from_active_root_proofs(
                rejected_proofs, exact_budget);
        check(
            result.certified_rejection() && result.certified_outcome() &&
                result.certified_for(pre_stamp) &&
                result.disposition() ==
                    ExactDirectSparseRootCoverageFromActiveRootProofsDisposition::
                        rejected &&
                result.decision() == decision && payloadless(result),
            message);
      };
  const std::array reversed{latent, rooted};
  const std::array duplicate{rooted, rooted_copy};
  const std::array missing_proof{missing};
  const std::array exhausted_proof{exhausted};
  ExactDirectSparseRootLedgerActiveRootProbeResult default_proof;
  const std::array default_proofs{default_proof};
  auto tampered = rooted;
  tampered.exact_key_comparison_count = 0U;
  const std::array tampered_proof{tampered};
  expect_proof_rejection(
      reversed,
      ExactDirectSparseRootCoverageFromActiveRootProofsDecision::
          no_proof_shape_rejected,
      "descending proofs must be rejected without payload");
  expect_proof_rejection(
      duplicate,
      ExactDirectSparseRootCoverageFromActiveRootProofsDecision::
          no_proof_shape_rejected,
      "duplicate proofs must be rejected without payload");
  expect_proof_rejection(
      missing_proof,
      ExactDirectSparseRootCoverageFromActiveRootProofsDecision::
          no_proof_rejected,
      "a certified miss must not authorize coverage materialization");
  expect_proof_rejection(
      exhausted_proof,
      ExactDirectSparseRootCoverageFromActiveRootProofsDecision::
          no_proof_rejected,
      "a certified exhausted probe must not authorize materialization");
  expect_proof_rejection(
      default_proofs,
      ExactDirectSparseRootCoverageFromActiveRootProofsDecision::
          no_proof_rejected,
      "a default probe cannot authorize materialization");
  expect_proof_rejection(
      tampered_proof,
      ExactDirectSparseRootCoverageFromActiveRootProofsDecision::
          no_proof_rejected,
      "a publicly tampered probe must be rejected without payload");

  auto foreign = make_fixture();
  const auto foreign_rooted = foreign.ledger.probe_active_root(
      10U, ExactDirectSparseRootLedgerActiveRootProbeBudget{1U});
  const std::array foreign_proof{foreign_rooted};
  expect_proof_rejection(
      foreign_proof,
      ExactDirectSparseRootCoverageFromActiveRootProofsDecision::
          no_proof_rejected,
      "an authentic foreign-ledger probe must be rejected without payload");

  const auto maximum = std::numeric_limits<std::size_t>::max();
  const auto rooted_maximum = context.ledger.probe_active_root(
      10U, ExactDirectSparseRootLedgerActiveRootProbeBudget{maximum});
  const auto latent_maximum = context.ledger.probe_active_root(
      30U, ExactDirectSparseRootLedgerActiveRootProbeBudget{maximum});
  const std::array overflow_proofs{rooted_maximum, latent_maximum};
  auto overflow_budget = exact_budget;
  overflow_budget.maximum_aggregate_requested_root_index_slot_visit_count =
      maximum;
  const auto overflow =
      context.ledger.materialize_active_coverages_from_active_root_proofs(
          overflow_proofs, overflow_budget);
  check(
      overflow.certified_rejection() &&
          overflow.certified_outcome() && overflow.certified_for(pre_stamp) &&
          overflow.disposition() ==
              ExactDirectSparseRootCoverageFromActiveRootProofsDisposition::
                  rejected &&
          overflow.decision() ==
              ExactDirectSparseRootCoverageFromActiveRootProofsDecision::
                  no_counter_capacity_overflow &&
          payloadless(overflow),
      "SIZE_MAX requested-slot aggregation must fail closed on overflow");

  const auto unchanged = [](const auto& before, const auto current) {
    return before.size() == current.size() &&
           std::equal(
               before.begin(), before.end(), current.begin(), current.end());
  };
  check(
      context.ledger.current_stamp() == pre_stamp &&
          unchanged(history_before, context.ledger.history_entries()) &&
          unchanged(coverage_before, context.ledger.coverage_nodes()) &&
          unchanged(
              coverage_parents_before,
              context.ledger.coverage_parent_references()) &&
          unchanged(
              coverage_deltas_before,
              context.ledger.coverage_point_deltas()) &&
          unchanged(
              prior_root_parents_before,
              context.ledger.prior_root_parent_references()),
      "all proof-bound outcomes must leave the stamp and five ledger arenas "
      "unchanged");

  const std::array<ExactDirectSparseStableFacetHandle, 1U> later_handle{50U};
  const std::array later_birth{
      ExactDirectSparseRootLedgerStandaloneBirth{50U, 0U, 1U, 1U}};
  const std::array<PointId, 1U> later_point{9U};
  (void)commit_births(context, later_handle, later_birth, later_point);
  const std::array stale_proof{rooted};
  const auto stale =
      context.ledger.materialize_active_coverages_from_active_root_proofs(
          stale_proof, exact_budget);
  check(
      stale.certified_rejection() && stale.certified_outcome() &&
          stale.certified_for(context.ledger.current_stamp()) &&
          stale.decision() ==
              ExactDirectSparseRootCoverageFromActiveRootProofsDecision::
                  no_proof_rejected &&
          payloadless(stale),
      "a stale authentic probe must be rejected against the successor stamp");
}

void test_caps_siblings_foreign_and_tamper() {
  auto tight_budget = ledger_budget();
  tight_budget.maximum_point_delta_count = 1U;
  auto tight = make_context(1'000U, 1U, ~std::uint64_t{0U}, tight_budget);
  const std::array<ExactDirectSparseStableFacetHandle, 1U> handles{10U};
  const std::array insertions{
      ExactDirectSparseStableFacetInsertion{10U, facet(10U)}};
  auto prepared_forest = prepare_forest(tight.forest, insertions, {}, handles);
  const std::array births{
      ExactDirectSparseRootLedgerStandaloneBirth{10U, 0U, 2U, 1U}};
  const std::array<PointId, 2U> points{1U, 2U};
  const auto capped = tight.ledger.prepare_transition(
      tight.forest,
      std::move(prepared_forest.ticket),
      std::move(prepared_forest.preview),
      {},
      {},
      {},
      births,
      points);
  check(
      capped.decision == ExactDirectSparseRootLedgerPreparationDecision::
                             no_budget_exhausted &&
          !capped.ticket.has_value() &&
          tight.forest.current_stamp().epoch == 0U &&
          tight.ledger.current_stamp().epoch == 0U,
      "cap-minus-one must reject atomically before either logical commit");

  auto sibling = make_context();
  auto forest_a = prepare_forest(sibling.forest, insertions, {}, handles);
  auto forest_b = prepare_forest(sibling.forest, insertions, {}, handles);
  const std::array one_birth{
      ExactDirectSparseRootLedgerStandaloneBirth{10U, 0U, 1U, 1U}};
  const std::array<PointId, 1U> one_point{1U};
  auto ledger_a = sibling.ledger.prepare_transition(
      sibling.forest,
      std::move(forest_a.ticket),
      std::move(forest_a.preview),
      {},
      {},
      {},
      one_birth,
      one_point);
  auto ledger_b = sibling.ledger.prepare_transition(
      sibling.forest,
      std::move(forest_b.ticket),
      std::move(forest_b.preview),
      {},
      {},
      {},
      one_birth,
      one_point);
  check(
      ledger_a.certified_prepared() && ledger_b.certified_prepared() &&
          sibling.ledger.outstanding_ticket_count() == 2U,
      "two siblings over one immutable epoch must prepare independently");
  auto public_tamper = std::move(ledger_a);
  ++public_tamper.group_count;
  check(
      !public_tamper.certified_prepared(),
      "public preparation-receipt tamper must not certify");
  --public_tamper.group_count;
  const auto first = sibling.ledger.commit(
      std::move(*public_tamper.ticket), sibling.forest);
  const auto stale = sibling.ledger.commit(
      std::move(*ledger_b.ticket), sibling.forest);
  check(
      first.certified_commit() &&
          stale.decision == ExactDirectSparseRootLedgerCommitDecision::
                                no_stale_or_sibling_ticket_rejected &&
          stale.ticket_consumed && ledger_b.ticket->consumed() &&
          sibling.ledger.outstanding_ticket_count() == 0U &&
          sibling.forest.outstanding_ticket_count() == 0U &&
          !stale.state_mutated,
      "committed sibling must make the second ledger/forest ticket stale and release both ticket budgets");

  auto foreign_forest_initialization =
      initialize_exact_direct_sparse_stable_facet_forest(
          {1'000U, digest("foreign-shared-source"), 0U}, forest_budget());
  auto ledger_a_initialization = initialize_exact_direct_sparse_root_ledger(
      {1U, 0U, 0U}, ledger_budget(), *foreign_forest_initialization.forest);
  auto ledger_b_initialization = initialize_exact_direct_sparse_root_ledger(
      {1U, 0U, 0U}, ledger_budget(), *foreign_forest_initialization.forest);
  auto& foreign_forest = *foreign_forest_initialization.forest;
  auto& owner = *ledger_a_initialization.ledger;
  auto& nonowner = *ledger_b_initialization.ledger;
  auto foreign_prepared_forest =
      prepare_forest(foreign_forest, insertions, {}, handles);
  auto owner_ticket = owner.prepare_transition(
      foreign_forest,
      std::move(foreign_prepared_forest.ticket),
      std::move(foreign_prepared_forest.preview),
      {},
      {},
      {},
      one_birth,
      one_point);
  const auto rejected =
      nonowner.commit(std::move(*owner_ticket.ticket), foreign_forest);
  check(
      rejected.decision == ExactDirectSparseRootLedgerCommitDecision::
                               no_foreign_ticket_rejected &&
          owner_ticket.ticket->valid(),
      "foreign ledger must reject without consuming the owner ticket");
  const auto owned_commit =
      owner.commit(std::move(*owner_ticket.ticket), foreign_forest);
  check(owned_commit.certified_commit(), "owner must still commit its ticket");
}

void test_root_id_max_namespace_size_max_and_digest() {
  auto maximum = make_context(
      1'000U, std::numeric_limits<ExactDirectSparseRootId>::max(), 0U);
  const std::array<ExactDirectSparseStableFacetHandle, 2U> handles{100U, 200U};
  const std::array births{
      ExactDirectSparseRootLedgerStandaloneBirth{100U, 0U, 1U, 1U},
      ExactDirectSparseRootLedgerStandaloneBirth{200U, 1U, 1U, 1U},
  };
  const std::array<PointId, 2U> points{10U, 20U};
  (void)commit_births(maximum, handles, births, points);

  const std::array insertions{
      ExactDirectSparseStableFacetInsertion{101U, facet(101U)}};
  const std::array unions{ExactDirectSparseStableFacetUnion{100U, 101U}};
  const std::array<ExactDirectSparseStableFacetHandle, 2U> preview_handles{
      100U, 101U};
  const std::array<ExactDirectSparseStableFacetHandle, 1U> parents{100U};
  (void)commit_group(
      maximum,
      insertions,
      unions,
      preview_handles,
      {0U, 101U, 0U, 1U, 0U, 0U, 1U},
      parents,
      {});
  const auto max_lookup = maximum.ledger.lookup_active_root(100U);
  check(
      max_lookup.entry.prior_root_id ==
              std::numeric_limits<ExactDirectSparseRootId>::max() &&
          !maximum.ledger.current_stamp().next_allocatable_root_id.has_value(),
      "the maximum root id must allocate exactly once without wrapping");

  const std::array max_repeat_insertions{
      ExactDirectSparseStableFacetInsertion{100U, facet(100U)}};
  const std::array<ExactDirectSparseStableFacetHandle, 1U> max_repeat_preview{
      100U};
  const std::array<ExactDirectSparseStableFacetHandle, 1U> max_repeat_parent{
      100U};
  const auto max_preserved = commit_group(
      maximum,
      max_repeat_insertions,
      {},
      max_repeat_preview,
      {0U, 100U, 0U, 1U, 0U, 0U, 0U},
      max_repeat_parent,
      {});
  check(
      max_preserved.preserved_root_id_count == 1U &&
          max_preserved.allocated_root_id_count == 0U &&
          maximum.ledger.lookup_active_root(100U).entry.prior_root_id ==
              std::numeric_limits<ExactDirectSparseRootId>::max() &&
          maximum.ledger.history_entries().back().coverage_reused,
      "q_R=1 compatible repeat must preserve UINT64_MAX and still append an "
      "action record after allocation exhaustion");

  const std::array exhausted_insertions{
      ExactDirectSparseStableFacetInsertion{201U, facet(201U)}};
  const std::array exhausted_unions{
      ExactDirectSparseStableFacetUnion{200U, 201U}};
  const std::array<ExactDirectSparseStableFacetHandle, 2U> exhausted_preview{
      200U, 201U};
  auto exhausted_forest = prepare_forest(
      maximum.forest,
      exhausted_insertions,
      exhausted_unions,
      exhausted_preview);
  const std::array exhausted_group{
      ExactDirectSparseRootLedgerGroupTransition{
          0U, 201U, 0U, 1U, 0U, 0U, 1U}};
  const std::array<ExactDirectSparseStableFacetHandle, 1U> exhausted_parents{
      200U};
  const auto exhausted = maximum.ledger.prepare_transition(
      maximum.forest,
      std::move(exhausted_forest.ticket),
      std::move(exhausted_forest.preview),
      exhausted_group,
      exhausted_parents,
      {},
      {},
      {});
  check(
      exhausted.decision == ExactDirectSparseRootLedgerPreparationDecision::
                                contradiction_root_id_namespace_exhausted &&
          maximum.ledger.lookup_active_root(200U).certified_observed(),
      "a second allocating transition after UINT64_MAX must fail closed");

  auto huge = make_context(std::numeric_limits<std::size_t>::max(), 1U, 0U);
  const ExactDirectSparseStableFacetHandle huge_handle =
      std::numeric_limits<std::size_t>::max() - 1U;
  const std::array huge_handles{huge_handle};
  const std::array huge_births{
      ExactDirectSparseRootLedgerStandaloneBirth{huge_handle, 0U, 1U, 1U}};
  const std::array<PointId, 1U> huge_points{42U};
  (void)commit_births(huge, huge_handles, huge_births, huge_points);
  check(
      huge.ledger.lookup_active_root(huge_handle).certified_observed() &&
          huge.ledger.materialized_active_handle_index_slot_count() == 8U,
      "SIZE_MAX forest namespace must remain scalar while one sparse row uses "
      "only the minimum index table");

  const auto initial_digest =
      make_context().ledger.current_stamp().semantic_history_digest;
  // This fixed oracle locks the endian-stable v1 logical initialization
  // projection.  It intentionally excludes both physical fingerprint masks.
  check(
      initial_digest.to_lower_hex() ==
          "350d3ff6e3d7d5e4aeb9fcd2bd55d1cfcf36981354efa636347605003f0391d6",
      "v1 initial semantic digest oracle (observed " +
          initial_digest.to_lower_hex() + ")");

  auto digest_left = make_context(1'000U, 1U, 0U);
  auto digest_right =
      make_context(1'000U, 1U, ~std::uint64_t{0U});
  const std::array<ExactDirectSparseStableFacetHandle, 1U> digest_handles{10U};
  const std::array digest_births{
      ExactDirectSparseRootLedgerStandaloneBirth{10U, 0U, 1U, 1U}};
  const std::array<PointId, 1U> digest_points{7U};
  (void)commit_births(
      digest_left, digest_handles, digest_births, digest_points);
  (void)commit_births(
      digest_right, digest_handles, digest_births, digest_points);
  check(
      digest_left.ledger.current_stamp().session_instance_id !=
              digest_right.ledger.current_stamp().session_instance_id &&
          digest_left.ledger.current_stamp().semantic_history_digest ==
              digest_right.ledger.current_stamp().semantic_history_digest,
      "semantic history must be invariant across process-local sessions and "
      "physical fingerprint masks");
}

void test_csr_partitions_duplicate_parents_and_empty_forest_gate() {
  auto context = make_context();
  const std::array<ExactDirectSparseStableFacetHandle, 3U> birth_handles{
      10U, 20U, 25U};
  const std::array births{
      ExactDirectSparseRootLedgerStandaloneBirth{10U, 0U, 1U, 1U},
      ExactDirectSparseRootLedgerStandaloneBirth{20U, 1U, 1U, 1U},
      ExactDirectSparseRootLedgerStandaloneBirth{25U, 2U, 1U, 1U},
  };
  const std::array<PointId, 3U> birth_points{1U, 2U, 3U};
  (void)commit_births(context, birth_handles, births, birth_points);

  const std::array insertions{
      ExactDirectSparseStableFacetInsertion{30U, facet(30U)},
      ExactDirectSparseStableFacetInsertion{40U, facet(40U)},
  };
  const std::array<ExactDirectSparseStableFacetHandle, 2U> preview_handles{
      30U, 40U};
  auto forest_ticket =
      prepare_forest(context.forest, insertions, {}, preview_handles);
  const std::array<ExactDirectSparseRootLedgerGroupTransition, 2U> overlap{
      ExactDirectSparseRootLedgerGroupTransition{
          0U, 30U, 0U, 1U, 0U, 1U, 1U},
      ExactDirectSparseRootLedgerGroupTransition{
          1U, 40U, 0U, 1U, 0U, 1U, 1U},
  };
  const std::array<ExactDirectSparseStableFacetHandle, 2U> overlap_parents{
      10U, 20U};
  const std::array<PointId, 2U> overlap_points{4U, 5U};
  const auto overlap_rejected = context.ledger.prepare_transition(
      context.forest,
      std::move(forest_ticket.ticket),
      std::move(forest_ticket.preview),
      overlap,
      overlap_parents,
      overlap_points,
      {},
      {});
  check(
      overlap_rejected.decision ==
              ExactDirectSparseRootLedgerPreparationDecision::
                  no_input_shape_rejected &&
          forest_ticket.ticket.valid() &&
          forest_ticket.preview.certified_for(
              forest_ticket.ticket, context.forest.current_stamp()),
      "overlapping CSR slices must reject without consuming forest capabilities");

  const std::array<ExactDirectSparseRootLedgerGroupTransition, 2U> gap{
      ExactDirectSparseRootLedgerGroupTransition{
          0U, 30U, 0U, 1U, 0U, 1U, 1U},
      ExactDirectSparseRootLedgerGroupTransition{
          1U, 40U, 2U, 1U, 2U, 1U, 1U},
  };
  const std::array<ExactDirectSparseStableFacetHandle, 3U> three_parents{
      10U, 20U, 25U};
  const std::array<PointId, 3U> three_points{4U, 5U, 6U};
  const auto gap_rejected = context.ledger.prepare_transition(
      context.forest,
      std::move(forest_ticket.ticket),
      std::move(forest_ticket.preview),
      gap,
      three_parents,
      three_points,
      {},
      {});
  check(
      gap_rejected.decision == ExactDirectSparseRootLedgerPreparationDecision::
                                   no_input_shape_rejected,
      "gapped parent and point CSRs must reject");

  const std::array<ExactDirectSparseRootLedgerGroupTransition, 2U> trailing{
      ExactDirectSparseRootLedgerGroupTransition{
          0U, 30U, 0U, 1U, 0U, 1U, 1U},
      ExactDirectSparseRootLedgerGroupTransition{
          1U, 40U, 1U, 1U, 1U, 1U, 1U},
  };
  const auto trailing_rejected = context.ledger.prepare_transition(
      context.forest,
      std::move(forest_ticket.ticket),
      std::move(forest_ticket.preview),
      trailing,
      three_parents,
      three_points,
      {},
      {});
  check(
      trailing_rejected.decision ==
          ExactDirectSparseRootLedgerPreparationDecision::
              no_input_shape_rejected,
      "trailing unreferenced parent/point entries must reject");

  const std::array<ExactDirectSparseRootLedgerStandaloneBirth, 2U>
      overlapping_births{
          ExactDirectSparseRootLedgerStandaloneBirth{30U, 0U, 1U, 1U},
          ExactDirectSparseRootLedgerStandaloneBirth{40U, 0U, 1U, 1U},
      };
  const auto birth_overlap_rejected = context.ledger.prepare_transition(
      context.forest,
      std::move(forest_ticket.ticket),
      std::move(forest_ticket.preview),
      {},
      {},
      {},
      overlapping_births,
      overlap_points);
  check(
      birth_overlap_rejected.decision ==
          ExactDirectSparseRootLedgerPreparationDecision::
              no_input_shape_rejected,
      "overlapping standalone-birth point slices must reject");

  auto duplicate_context = make_context();
  const std::array<ExactDirectSparseStableFacetHandle, 1U> one_handle{10U};
  const std::array one_birth{
      ExactDirectSparseRootLedgerStandaloneBirth{10U, 0U, 1U, 1U}};
  const std::array<PointId, 1U> one_point{1U};
  (void)commit_births(
      duplicate_context, one_handle, one_birth, one_point);
  const std::array duplicate_insertions{
      ExactDirectSparseStableFacetInsertion{30U, facet(30U)},
      ExactDirectSparseStableFacetInsertion{40U, facet(40U)},
  };
  const std::array duplicate_unions{
      ExactDirectSparseStableFacetUnion{10U, 30U},
      ExactDirectSparseStableFacetUnion{10U, 40U},
  };
  const std::array<ExactDirectSparseStableFacetHandle, 3U> duplicate_preview{
      10U, 30U, 40U};
  auto duplicate_forest = prepare_forest(
      duplicate_context.forest,
      duplicate_insertions,
      duplicate_unions,
      duplicate_preview);
  const std::array<ExactDirectSparseRootLedgerGroupTransition, 2U>
      duplicate_groups{
          ExactDirectSparseRootLedgerGroupTransition{
              0U, 30U, 0U, 1U, 0U, 0U, 1U},
          ExactDirectSparseRootLedgerGroupTransition{
              1U, 40U, 1U, 1U, 0U, 0U, 1U},
      };
  const std::array<ExactDirectSparseStableFacetHandle, 2U> duplicate_parents{
      10U, 10U};
  const auto duplicate_rejected = duplicate_context.ledger.prepare_transition(
      duplicate_context.forest,
      std::move(duplicate_forest.ticket),
      std::move(duplicate_forest.preview),
      duplicate_groups,
      duplicate_parents,
      {},
      {},
      {});
  check(
      duplicate_rejected.decision ==
          ExactDirectSparseRootLedgerPreparationDecision::
              contradiction_unknown_or_duplicate_parent_root,
      "one active parent consumed by two groups must reject after O(P log P) "
      "canonical duplicate detection");

  auto inverse_context = make_context();
  const std::array<ExactDirectSparseStableFacetHandle, 2U> inverse_birth_handles{
      100U, 200U};
  const std::array inverse_births{
      ExactDirectSparseRootLedgerStandaloneBirth{100U, 0U, 1U, 1U},
      ExactDirectSparseRootLedgerStandaloneBirth{200U, 1U, 1U, 1U},
  };
  const std::array<PointId, 2U> inverse_birth_points{1U, 2U};
  (void)commit_births(
      inverse_context,
      inverse_birth_handles,
      inverse_births,
      inverse_birth_points);
  const std::array inverse_insertions{
      ExactDirectSparseStableFacetInsertion{10U, facet(10U)},
      ExactDirectSparseStableFacetInsertion{90U, facet(90U)},
  };
  const std::array inverse_unions{
      ExactDirectSparseStableFacetUnion{10U, 200U},
      ExactDirectSparseStableFacetUnion{90U, 100U},
  };
  const std::array<ExactDirectSparseStableFacetHandle, 4U> inverse_preview{
      10U, 90U, 100U, 200U};
  auto inverse_forest = prepare_forest(
      inverse_context.forest,
      inverse_insertions,
      inverse_unions,
      inverse_preview);
  // Canonical frozen group order is 0 then 1 even though representatives are
  // 90 then 10.  Root-id allocation must follow source_group_index.
  const std::array<ExactDirectSparseRootLedgerGroupTransition, 2U>
      inverse_groups{
          ExactDirectSparseRootLedgerGroupTransition{
              0U, 90U, 0U, 1U, 0U, 0U, 1U},
          ExactDirectSparseRootLedgerGroupTransition{
              1U, 10U, 1U, 1U, 0U, 0U, 1U},
      };
  const std::array<ExactDirectSparseStableFacetHandle, 2U> inverse_parents{
      100U, 200U};
  auto inverse_ledger = inverse_context.ledger.prepare_transition(
      inverse_context.forest,
      std::move(inverse_forest.ticket),
      std::move(inverse_forest.preview),
      inverse_groups,
      inverse_parents,
      {},
      {},
      {});
  check(
      inverse_ledger.certified_prepared(),
      "descending representatives must prepare in dense source group order");
  const auto inverse_commit = inverse_context.ledger.commit(
      std::move(*inverse_ledger.ticket), inverse_context.forest);
  check(
      inverse_commit.certified_commit() &&
          inverse_context.ledger.lookup_active_root(90U).entry.prior_root_id ==
              1U &&
          inverse_context.ledger.lookup_active_root(10U).entry.prior_root_id ==
              2U &&
          inverse_context.ledger.history_entries()[2U].source_group_index ==
              0U &&
          inverse_context.ledger.history_entries()[3U].source_group_index ==
              1U,
      "root ids and history must follow group_index, never representative "
      "handle order");

  auto hidden_context = make_context();
  const std::array<ExactDirectSparseStableFacetHandle, 2U> hidden_birth_handles{
      10U, 20U};
  const std::array hidden_births{
      ExactDirectSparseRootLedgerStandaloneBirth{10U, 0U, 1U, 1U},
      ExactDirectSparseRootLedgerStandaloneBirth{20U, 1U, 1U, 1U},
  };
  const std::array<PointId, 2U> hidden_birth_points{1U, 2U};
  (void)commit_births(
      hidden_context,
      hidden_birth_handles,
      hidden_births,
      hidden_birth_points);
  const std::array hidden_singleton_insertions{
      ExactDirectSparseStableFacetInsertion{11U, facet(11U)},
      ExactDirectSparseStableFacetInsertion{30U, facet(30U)},
  };
  const std::array hidden_singleton_unions{
      ExactDirectSparseStableFacetUnion{10U, 11U}};
  const std::array<ExactDirectSparseStableFacetHandle, 3U>
      hidden_singleton_preview{10U, 11U, 30U};
  auto hidden_singleton_forest = prepare_forest(
      hidden_context.forest,
      hidden_singleton_insertions,
      hidden_singleton_unions,
      hidden_singleton_preview);
  const std::array hidden_group{
      ExactDirectSparseRootLedgerGroupTransition{
          0U, 11U, 0U, 1U, 0U, 0U, 1U}};
  const std::array<ExactDirectSparseStableFacetHandle, 1U> hidden_parent{10U};
  const auto hidden_singleton_rejected =
      hidden_context.ledger.prepare_transition(
          hidden_context.forest,
          std::move(hidden_singleton_forest.ticket),
          std::move(hidden_singleton_forest.preview),
          hidden_group,
          hidden_parent,
          {},
          {},
          {});
  check(
      hidden_singleton_rejected.decision ==
          ExactDirectSparseRootLedgerPreparationDecision::
              no_preview_exhaustiveness_rejected,
      "an exhaustively previewed but unbound singleton insertion must reject");

  const std::array hidden_merge_insertions{
      ExactDirectSparseStableFacetInsertion{11U, facet(11U)}};
  const std::array hidden_merge_unions{
      ExactDirectSparseStableFacetUnion{10U, 11U},
      ExactDirectSparseStableFacetUnion{10U, 20U},
  };
  const std::array<ExactDirectSparseStableFacetHandle, 3U> hidden_merge_preview{
      10U, 11U, 20U};
  auto hidden_merge_forest = prepare_forest(
      hidden_context.forest,
      hidden_merge_insertions,
      hidden_merge_unions,
      hidden_merge_preview);
  const auto hidden_merge_rejected = hidden_context.ledger.prepare_transition(
      hidden_context.forest,
      std::move(hidden_merge_forest.ticket),
      std::move(hidden_merge_forest.preview),
      hidden_group,
      hidden_parent,
      {},
      {},
      {});
  check(
      hidden_merge_rejected.decision ==
          ExactDirectSparseRootLedgerPreparationDecision::
              no_preview_exhaustiveness_rejected,
      "a touched active root omitted from group parents must reject");

  auto nonempty_forest = initialize_exact_direct_sparse_stable_facet_forest(
      {1'000U, digest("nonempty-ledger-init"), 0U}, forest_budget());
  const std::array nonempty_insertions{
      ExactDirectSparseStableFacetInsertion{10U, facet(10U)}};
  auto nonempty_ticket = nonempty_forest.forest->prepare_batch(
      nonempty_insertions, {});
  const auto nonempty_commit = nonempty_forest.forest->commit(
      std::move(*nonempty_ticket.ticket));
  check(nonempty_commit.certified_commit(), "nonempty forest setup must commit");
  const auto nonempty_rejected = initialize_exact_direct_sparse_root_ledger(
      {1U, 0U, 0U}, ledger_budget(), *nonempty_forest.forest);
  check(
      nonempty_rejected.decision ==
          ExactDirectSparseRootLedgerInitializationDecision::no_forest_rejected,
      "ledger initialization must reject an already nonempty forest");

  auto outstanding_forest = initialize_exact_direct_sparse_stable_facet_forest(
      {1'000U, digest("outstanding-ledger-init"), 0U}, forest_budget());
  auto outstanding_ticket = outstanding_forest.forest->prepare_batch(
      nonempty_insertions, {});
  check(
      outstanding_ticket.certified_prepared(),
      "outstanding forest ticket setup must certify");
  const auto outstanding_rejected = initialize_exact_direct_sparse_root_ledger(
      {1U, 0U, 0U}, ledger_budget(), *outstanding_forest.forest);
  check(
      outstanding_rejected.decision ==
          ExactDirectSparseRootLedgerInitializationDecision::no_forest_rejected,
      "ledger initialization must reject a forest with an outstanding ticket");
}

}  // namespace

int main() {
  test_qr_transitions_coverage_and_migration();
  test_bounded_active_root_probe();
  test_proof_bound_active_root_materialization();
  test_caps_siblings_foreign_and_tamper();
  test_root_id_max_namespace_size_max_and_digest();
  test_csr_partitions_duplicate_parents_and_empty_forest_gate();
  if (failures != 0) {
    std::cerr << failures << " direct sparse root ledger checks failed\n";
    return 1;
  }
  std::cout << "direct sparse root ledger checks passed\n";
  return 0;
}
