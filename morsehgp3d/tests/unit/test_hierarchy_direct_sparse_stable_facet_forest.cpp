#include "morsehgp3d/hierarchy/direct_sparse_stable_facet_forest.hpp"

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

static_assert(
    !std::is_copy_constructible_v<
        ExactDirectSparseStableFacetForestPreparedBatch> &&
    std::is_nothrow_move_constructible_v<
        ExactDirectSparseStableFacetForestPreparedBatch>);
static_assert(
    !std::is_copy_constructible_v<ExactDirectSparseStableFacetForest> &&
    std::is_nothrow_move_constructible_v<
        ExactDirectSparseStableFacetForest>);
static_assert(direct_sparse_stable_facet_forest_schema_version == 1U);
static_assert(direct_sparse_stable_facet_forest_storage_schema_version == 2U);
static_assert(
    std::is_aggregate_v<
        ExactDirectSparseStableFacetPositiveLookupReceipt> &&
    !std::is_aggregate_v<
        ExactDirectSparseStableFacetPositiveLookupResult> &&
    std::is_nothrow_copy_constructible_v<
        ExactDirectSparseStableFacetPositiveLookupResult>);
static_assert(
    !std::is_copy_constructible_v<
        ExactDirectSparseStableFacetForestPreparedPreview> &&
    std::is_nothrow_move_constructible_v<
        ExactDirectSparseStableFacetForestPreparedPreview>);

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

template <std::size_t Size>
[[nodiscard]] ExactDirectSparseFacetKey facet(
    const std::array<PointId, Size>& ids) {
  ExactDirectSparseFacetKey key;
  key.point_count = Size;
  std::copy(ids.begin(), ids.end(), key.point_ids.begin());
  return key;
}

[[nodiscard]] ExactDirectSparseStableFacetForestBudget generous_budget() {
  return {
      16U,
      64U,
      100U,
      16U,
      16U,
      16U,
      128U,
      32U,
      32U,
      4U,
  };
}

[[nodiscard]]
ExactDirectSparseStableFacetForestPreparedPreviewBudget
generous_preview_budget() {
  return {
      64U,
      256U,
      128U,
      320U,
  };
}

[[nodiscard]] ExactDirectSparseStableFacetForestInitialization initialize(
    const ExactDirectSparseStableFacetForestBudget& budget =
        generous_budget()) {
  return initialize_exact_direct_sparse_stable_facet_forest(
      {1'000'000'001U, digest("normalized-structural-source")}, budget);
}

[[nodiscard]] ExactDirectSparseStableFacetForestInitialization
initialize_with_positive_fingerprint_mask(
    std::uint64_t fingerprint_mask,
    const ExactDirectSparseStableFacetForestBudget& budget =
        generous_budget()) {
  return initialize_exact_direct_sparse_stable_facet_forest(
      {1'000'000'001U,
       digest("normalized-structural-source"),
       fingerprint_mask},
      budget);
}

struct BoundedOracleRow {
  ExactDirectSparseStableFacetHandle handle{};
  ExactDirectSparseFacetKey key{};
  ExactDirectSparseStableFacetHandle parent{};
  std::size_t component_size{};
};

class BoundedStableFacetOracle {
 public:
  void insert(
      ExactDirectSparseStableFacetHandle handle,
      const ExactDirectSparseFacetKey& key) {
    if (row_index(handle) == no_row) {
      rows_.push_back({handle, key, handle, 1U});
    }
  }

  void unite(
      ExactDirectSparseStableFacetHandle left,
      ExactDirectSparseStableFacetHandle right) {
    const std::size_t left_root = root_row(left);
    const std::size_t right_root = root_row(right);
    if (left_root == no_row || right_root == no_row ||
        left_root == right_root) {
      return;
    }
    auto& left_entry = rows_[left_root];
    auto& right_entry = rows_[right_root];
    const bool left_wins =
        left_entry.component_size > right_entry.component_size ||
        (left_entry.component_size == right_entry.component_size &&
         left_entry.handle < right_entry.handle);
    auto& root = left_wins ? left_entry : right_entry;
    auto& child = left_wins ? right_entry : left_entry;
    root.component_size += child.component_size;
    child.component_size = 0U;
    child.parent = root.handle;
  }

  [[nodiscard]] ExactDirectSparseStableFacetHandle root_handle(
      ExactDirectSparseStableFacetHandle handle) const {
    const std::size_t root = root_row(handle);
    return root == no_row ? ExactDirectSparseStableFacetHandle{}
                          : rows_[root].handle;
  }

  [[nodiscard]] std::size_t component_size(
      ExactDirectSparseStableFacetHandle handle) const {
    const std::size_t root = root_row(handle);
    return root == no_row ? 0U : rows_[root].component_size;
  }

  [[nodiscard]] const ExactDirectSparseFacetKey* key(
      ExactDirectSparseStableFacetHandle handle) const {
    const std::size_t row = row_index(handle);
    return row == no_row ? nullptr : &rows_[row].key;
  }

 private:
  static constexpr std::size_t no_row =
      std::numeric_limits<std::size_t>::max();

  [[nodiscard]] std::size_t row_index(
      ExactDirectSparseStableFacetHandle handle) const {
    const auto found = std::find_if(
        rows_.begin(), rows_.end(), [handle](const BoundedOracleRow& row) {
          return row.handle == handle;
        });
    return found == rows_.end()
               ? no_row
               : static_cast<std::size_t>(found - rows_.begin());
  }

  [[nodiscard]] std::size_t root_row(
      ExactDirectSparseStableFacetHandle handle) const {
    std::size_t row = row_index(handle);
    std::size_t hops = 0U;
    while (row != no_row && rows_[row].parent != rows_[row].handle &&
           hops <= rows_.size()) {
      row = row_index(rows_[row].parent);
      ++hops;
    }
    return hops <= rows_.size() ? row : no_row;
  }

  std::vector<BoundedOracleRow> rows_;
};

void apply_oracle_batch(
    BoundedStableFacetOracle& oracle,
    std::span<const ExactDirectSparseStableFacetInsertion> input_insertions,
    std::span<const ExactDirectSparseStableFacetUnion> input_unions) {
  std::vector<ExactDirectSparseStableFacetInsertion> insertions(
      input_insertions.begin(), input_insertions.end());
  std::vector<ExactDirectSparseStableFacetUnion> unions(
      input_unions.begin(), input_unions.end());
  std::sort(
      insertions.begin(),
      insertions.end(),
      [](const auto& left, const auto& right) {
        return left.stable_source_facet_token_index <
               right.stable_source_facet_token_index;
      });
  for (auto& operation : unions) {
    if (operation.right_handle < operation.left_handle) {
      std::swap(operation.left_handle, operation.right_handle);
    }
  }
  std::sort(
      unions.begin(), unions.end(), [](const auto& left, const auto& right) {
        return left.left_handle < right.left_handle ||
               (left.left_handle == right.left_handle &&
                left.right_handle < right.right_handle);
      });
  for (const auto& insertion : insertions) {
    oracle.insert(
        insertion.stable_source_facet_token_index, insertion.facet_key);
  }
  for (const auto& operation : unions) {
    oracle.unite(operation.left_handle, operation.right_handle);
  }
}

[[nodiscard]] ExactDirectSparseFacetKey key_for_handle(std::size_t handle) {
  return facet(std::array<PointId, 2U>{
      static_cast<PointId>(handle + 1U),
      static_cast<PointId>(handle + 1'000'001U)});
}

void check_positive_lookup_receipt_tamper_round_trip(
    ExactDirectSparseStableFacetPositiveLookupResult genuine,
    bool expected_observed,
    std::string_view label) {
  using Receipt = ExactDirectSparseStableFacetPositiveLookupReceipt;
  const Receipt original = static_cast<const Receipt&>(genuine);
  const auto certified = [&]() {
    return expected_observed ? genuine.certified_observed()
                             : genuine.certified_unobserved();
  };
  const auto verify_mutation_and_restore = [&](std::string_view category) {
    check(
        !certified(),
        std::string{label} + " rejects public " + std::string{category} +
            " mutation");
    static_cast<Receipt&>(genuine) = original;
    check(
        certified(),
        std::string{label} + " recertifies after exact " +
            std::string{category} + " restoration");
  };

  check(certified(), std::string{label} + " starts privately certified");
  genuine.requested_key.point_ids[0U] += 1U;
  verify_mutation_and_restore("requested-key");
  genuine.bound_handle ^= 1U;
  verify_mutation_and_restore("bound-handle");
  genuine.root_handle ^= 1U;
  verify_mutation_and_restore("root-handle");
  genuine.component_size ^= 1U;
  verify_mutation_and_restore("component-size");
  genuine.forest_stamp.epoch ^= 1U;
  verify_mutation_and_restore("forest-stamp");
  genuine.forest_stamp.source_identity_digest =
      digest("tampered-positive-lookup-source-digest");
  verify_mutation_and_restore("source-digest");
  genuine.forest_stamp.semantic_history_digest =
      digest("tampered-positive-lookup-history-digest");
  verify_mutation_and_restore("history-digest");
  genuine.slot_visit_count ^= 1U;
  verify_mutation_and_restore("slot-visit-count");
  genuine.full_key_comparison_count ^= 1U;
  verify_mutation_and_restore("full-key-comparison-count");

  constexpr std::array receipt_flags{
      &Receipt::forest_certified_at_entry,
      &Receipt::requested_key_canonical,
      &Receipt::lookup_completed_without_mutation,
      &Receipt::fingerprint_used_only_as_accelerator,
      &Receipt::complete_key_comparison_authoritative,
      &Receipt::immutable_key_handle_binding_observed,
      &Receipt::root_resolved_without_mutation,
      &Receipt::source_exactness_claimed,
      &Receipt::public_status_claimed,
  };
  for (const auto flag : receipt_flags) {
    genuine.*flag = !(genuine.*flag);
    verify_mutation_and_restore("flag");
  }
  genuine.disposition =
      ExactDirectSparseStableFacetPositiveLookupDisposition::not_certified;
  verify_mutation_and_restore("disposition");
}

void commit_oracle_batch(
    ExactDirectSparseStableFacetForest& forest,
    BoundedStableFacetOracle& oracle,
    std::span<const ExactDirectSparseStableFacetInsertion> insertions,
    std::span<const ExactDirectSparseStableFacetUnion> unions,
    std::string_view label) {
  auto prepared = forest.prepare_batch(insertions, unions);
  if (!prepared.certified_prepared()) {
    check(false, std::string{label} + " prepares exactly");
    return;
  }
  allocation_probe::begin();
  const auto committed = forest.commit(std::move(*prepared.ticket));
  const std::size_t commit_allocation_count = allocation_probe::finish();
  if (!committed.certified_commit() || commit_allocation_count != 0U) {
    check(
        false,
        std::string{label} + " commits exactly without allocation");
    return;
  }
  apply_oracle_batch(oracle, insertions, unions);
}

void test_append_only_multibatch_index() {
  auto initialization = initialize();
  auto forest = std::move(*initialization.forest);
  BoundedStableFacetOracle oracle;

  const std::array first_insertions{
      ExactDirectSparseStableFacetInsertion{800U, key_for_handle(800U)},
      ExactDirectSparseStableFacetInsertion{900U, key_for_handle(900U)},
      ExactDirectSparseStableFacetInsertion{1'000U, key_for_handle(1'000U)},
  };
  const std::array first_unions{
      ExactDirectSparseStableFacetUnion{1'000U, 900U},
      ExactDirectSparseStableFacetUnion{800U, 900U},
  };
  commit_oracle_batch(
      forest, oracle, first_insertions, first_unions, "ascending handle batch");

  const std::array second_insertions{
      ExactDirectSparseStableFacetInsertion{700U, key_for_handle(700U)},
      ExactDirectSparseStableFacetInsertion{600U, key_for_handle(600U)},
      ExactDirectSparseStableFacetInsertion{500U, key_for_handle(500U)},
  };
  const std::array second_unions{
      ExactDirectSparseStableFacetUnion{700U, 600U},
      ExactDirectSparseStableFacetUnion{600U, 500U},
      ExactDirectSparseStableFacetUnion{500U, 800U},
  };
  commit_oracle_batch(
      forest,
      oracle,
      second_insertions,
      second_unions,
      "descending handle batch");

  const std::array third_insertions{
      ExactDirectSparseStableFacetInsertion{850U, key_for_handle(850U)},
      ExactDirectSparseStableFacetInsertion{550U, key_for_handle(550U)},
      ExactDirectSparseStableFacetInsertion{950U, key_for_handle(950U)},
  };
  const std::array third_unions{
      ExactDirectSparseStableFacetUnion{850U, 950U},
      ExactDirectSparseStableFacetUnion{550U, 850U},
      ExactDirectSparseStableFacetUnion{550U, 1'000U},
  };
  commit_oracle_batch(
      forest,
      oracle,
      third_insertions,
      third_unions,
      "interleaved handle batch");

  constexpr std::array expected_append_order{
      800U, 900U, 1'000U, 500U, 600U, 700U, 550U, 850U, 950U};
  const auto observed_entries = forest.observed_entries();
  bool append_order_exact =
      observed_entries.size() == expected_append_order.size();
  for (std::size_t index = 0U;
       append_order_exact && index < observed_entries.size();
       ++index) {
    append_order_exact =
        observed_entries[index].stable_source_facet_token_index ==
        expected_append_order[index];
  }
  check(
      append_order_exact,
      "durable rows append canonically within each batch without global resort");

  for (const auto handle : expected_append_order) {
    const auto observed = forest.lookup(handle);
    const auto* expected_key = oracle.key(handle);
    const auto positive = expected_key == nullptr
                              ? ExactDirectSparseStableFacetPositiveLookupResult{}
                              : forest.lookup_positive_full_key(*expected_key);
    check(
        expected_key != nullptr && observed.certified_observed() &&
            observed.root_handle == oracle.root_handle(handle) &&
            observed.component_size == oracle.component_size(handle) &&
            observed.facet_key == *expected_key &&
            positive.certified_observed() &&
            positive.bound_handle == handle &&
            positive.root_handle == oracle.root_handle(handle) &&
            positive.component_size == oracle.component_size(handle),
        "both sparse indexes agree with the bounded stable-handle oracle");
  }
  check(
      forest.materialized_handle_index_slot_count() >=
              2U * observed_entries.size() &&
          forest.materialized_handle_index_slot_count() < 1'000U,
      "the handle index scales with observed rows rather than the billion-token namespace");
  check(
      forest.materialized_positive_key_index_slot_count() >=
              2U * observed_entries.size() &&
          forest.materialized_positive_key_index_slot_count() < 1'000U,
      "the full-key index scales with observed rows rather than the billion-token namespace");
}

void test_flat_index_collision_chain() {
  auto initialization = initialize();
  auto forest = std::move(*initialization.forest);
  BoundedStableFacetOracle oracle;
  // Under the production 64-bit mixer, these four handles all start in bucket
  // seven of the eight-slot table required by four rows.  Exact handle
  // comparison must therefore survive one complete linear-probing cluster.
  constexpr std::array collision_handles{0U, 7U, 13U, 16U};
  const std::array insertions{
      ExactDirectSparseStableFacetInsertion{16U, key_for_handle(16U)},
      ExactDirectSparseStableFacetInsertion{0U, key_for_handle(0U)},
      ExactDirectSparseStableFacetInsertion{13U, key_for_handle(13U)},
      ExactDirectSparseStableFacetInsertion{7U, key_for_handle(7U)},
  };
  const std::array unions{
      ExactDirectSparseStableFacetUnion{0U, 7U},
      ExactDirectSparseStableFacetUnion{13U, 16U},
      ExactDirectSparseStableFacetUnion{7U, 13U},
  };
  commit_oracle_batch(
      forest, oracle, insertions, unions, "colliding handle-index batch");
  check(
      forest.materialized_handle_index_slot_count() == 8U,
      "four observed handles materialize only the required eight index slots");
  for (const auto handle : collision_handles) {
    const auto observed = forest.lookup(handle);
    check(
        observed.certified_observed() &&
            observed.root_handle == oracle.root_handle(handle) &&
            observed.component_size == oracle.component_size(handle),
        "linear-probing collisions retain exact DSU lookup semantics");
  }
}

void test_positive_full_key_index_collision_and_injective_binding() {
  auto collided_initialization =
      initialize_with_positive_fingerprint_mask(0U);
  auto regular_initialization = initialize();
  auto collided = std::move(*collided_initialization.forest);
  auto regular = std::move(*regular_initialization.forest);
  const auto expected_v1_initial_digest = CanonicalId::from_lower_hex(
      "c6f8f6721497a92c93740e8c3316509756d0bcbabf86f0d7ed32ffba7621d418");
  const auto expected_v1_batch_digest = CanonicalId::from_lower_hex(
      "39316233b5579f8c1aab7319c976b1cbd55242d5e5814d80c8610fb3ea5ee5c4");
  const auto expected_v1_post_history_digest = CanonicalId::from_lower_hex(
      "6a395f021581fc7743545c57d8ba3ef2a5940980dc959a78e88235de6823dc75");
  check(
      collided.current_stamp().semantic_history_digest ==
              regular.current_stamp().semantic_history_digest &&
          collided.current_stamp().semantic_history_digest ==
              expected_v1_initial_digest,
      "the physical fingerprint mask preserves the frozen v1 logical initial digest");

  const std::array insertions{
      ExactDirectSparseStableFacetInsertion{10U, key_for_handle(10U)},
      ExactDirectSparseStableFacetInsertion{11U, key_for_handle(11U)},
      ExactDirectSparseStableFacetInsertion{12U, key_for_handle(12U)},
      ExactDirectSparseStableFacetInsertion{13U, key_for_handle(13U)},
  };
  const std::array unions{
      ExactDirectSparseStableFacetUnion{10U, 11U},
      ExactDirectSparseStableFacetUnion{12U, 13U},
      ExactDirectSparseStableFacetUnion{11U, 12U},
  };
  auto collided_prepared = collided.prepare_batch(insertions, unions);
  auto regular_prepared = regular.prepare_batch(insertions, unions);
  check(
      collided_prepared.certified_prepared() &&
          regular_prepared.certified_prepared() &&
          collided_prepared.ticket->canonical_batch_digest() ==
              regular_prepared.ticket->canonical_batch_digest() &&
          collided_prepared.ticket->canonical_batch_digest() ==
              expected_v1_batch_digest &&
          collided.materialized_handle_index_slot_count() == 8U &&
          collided.materialized_positive_key_index_slot_count() == 8U,
      "preparation reserves rows and both sparse indexes before either ticket commits");

  allocation_probe::begin();
  const auto collided_commit =
      collided.commit(std::move(*collided_prepared.ticket));
  const std::size_t collided_commit_allocations = allocation_probe::finish();
  allocation_probe::begin();
  const auto regular_commit = regular.commit(std::move(*regular_prepared.ticket));
  const std::size_t regular_commit_allocations = allocation_probe::finish();
  check(
      collided_commit.certified_commit() && regular_commit.certified_commit() &&
          collided_commit_allocations == 0U &&
          regular_commit_allocations == 0U &&
          collided.certified_structure_only_forest() &&
          regular.certified_structure_only_forest() &&
          collided.current_stamp().semantic_history_digest ==
              regular.current_stamp().semantic_history_digest &&
          collided.current_stamp().semantic_history_digest ==
              expected_v1_post_history_digest,
      "both row+1 indexes commit allocation-free with the frozen v1 history digest");

  std::array<ExactDirectSparseStableFacetPositiveLookupResult, 4U> hits{};
  allocation_probe::begin();
  for (std::size_t index = 0U; index < hits.size(); ++index) {
    hits[index] = collided.lookup_positive_full_key(
        key_for_handle(10U + index));
  }
  const auto miss =
      collided.lookup_positive_full_key(key_for_handle(99U));
  const std::size_t lookup_allocations = allocation_probe::finish();
  bool every_hit_exact = lookup_allocations == 0U;
  for (std::size_t index = 0U; index < hits.size(); ++index) {
    every_hit_exact =
        every_hit_exact && hits[index].certified_observed() &&
        hits[index].bound_handle == 10U + index &&
        hits[index].root_handle == 10U &&
        hits[index].component_size == hits.size() &&
        hits[index].forest_stamp == collided.current_stamp();
  }
  check(
      every_hit_exact && hits.back().slot_visit_count == hits.size() &&
          hits.back().full_key_comparison_count == hits.size(),
      "forced equal fingerprints still return the authoritative full-key binding and DSU root");
  check(
      miss.certified_unobserved() && miss.slot_visit_count == 5U &&
          miss.full_key_comparison_count == hits.size() &&
          miss.forest_stamp == collided.current_stamp(),
      "a collision-chain miss compares every occupied complete key before the empty terminator");

  ExactDirectSparseStableFacetPositiveLookupResult forged_hit;
  static_cast<ExactDirectSparseStableFacetPositiveLookupReceipt&>(forged_hit) =
      static_cast<const ExactDirectSparseStableFacetPositiveLookupReceipt&>(
          hits.front());
  ExactDirectSparseStableFacetPositiveLookupResult forged_miss;
  static_cast<ExactDirectSparseStableFacetPositiveLookupReceipt&>(forged_miss) =
      static_cast<const ExactDirectSparseStableFacetPositiveLookupReceipt&>(
          miss);
  check(
      !forged_hit.certified_observed() &&
          !forged_hit.certified_unobserved() &&
          !forged_miss.certified_observed() &&
          !forged_miss.certified_unobserved(),
      "complete public hit and miss receipts cannot forge the private mint");
  check_positive_lookup_receipt_tamper_round_trip(
      hits.front(), true, "a minted positive hit");
  check_positive_lookup_receipt_tamper_round_trip(
      miss, false, "a minted positive miss");

  const auto before_cross_batch_collision = collided.current_stamp();
  const std::array cross_batch_collision{
      ExactDirectSparseStableFacetInsertion{77U, key_for_handle(10U)}};
  const auto rejected_cross_batch =
      collided.prepare_batch(cross_batch_collision, {});
  check(
      !rejected_cross_batch.ticket.has_value() &&
          rejected_cross_batch.decision ==
              ExactDirectSparseStableFacetForestPreparationDecision::
                  contradiction_stable_handle_key_collision &&
          collided.current_stamp() == before_cross_batch_collision &&
          collided.lookup_positive_full_key(key_for_handle(10U))
                  .bound_handle == 10U &&
          collided.lookup(77U).certified_unobserved(),
      "an existing complete key cannot be rebound to a second stable handle");

  auto fresh_initialization = initialize_with_positive_fingerprint_mask(0U);
  auto fresh = std::move(*fresh_initialization.forest);
  const auto fresh_stamp = fresh.current_stamp();
  const std::array within_batch_collision{
      ExactDirectSparseStableFacetInsertion{20U, key_for_handle(20U)},
      ExactDirectSparseStableFacetInsertion{21U, key_for_handle(20U)},
  };
  const auto rejected_within_batch =
      fresh.prepare_batch(within_batch_collision, {});
  check(
      !rejected_within_batch.ticket.has_value() &&
          rejected_within_batch.decision ==
              ExactDirectSparseStableFacetForestPreparationDecision::
                  contradiction_stable_handle_key_collision &&
          fresh.current_stamp() == fresh_stamp &&
          fresh.observed_entries().empty() &&
          fresh.materialized_handle_index_slot_count() == 0U &&
          fresh.materialized_positive_key_index_slot_count() == 0U,
      "an intra-batch complete-key rebinding is rejected before physical reservation or mutation");
}

[[nodiscard]] bool any_positive_probe_certification(
    const ExactDirectSparseStableFacetPositiveProbeResult& result) {
  return result.certified_observed() || result.certified_unobserved() ||
         result.certified_budget_exhaustion() ||
         result.certified_fail_closed_contradiction();
}

void test_bounded_positive_full_key_probe_caps_collision_and_stamp() {
  auto initialization = initialize_with_positive_fingerprint_mask(0U);
  auto forest = std::move(*initialization.forest);
  const std::array insertions{
      ExactDirectSparseStableFacetInsertion{10U, key_for_handle(10U)},
      ExactDirectSparseStableFacetInsertion{11U, key_for_handle(11U)},
      ExactDirectSparseStableFacetInsertion{12U, key_for_handle(12U)},
      ExactDirectSparseStableFacetInsertion{13U, key_for_handle(13U)},
  };
  const std::array unions{
      ExactDirectSparseStableFacetUnion{10U, 11U},
      ExactDirectSparseStableFacetUnion{12U, 13U},
      ExactDirectSparseStableFacetUnion{11U, 12U},
  };
  auto prepared = forest.prepare_batch(insertions, unions);
  const auto committed = forest.commit(std::move(*prepared.ticket));
  check(committed.certified_commit(), "bounded-probe forest commits");

  constexpr std::size_t maximum =
      std::numeric_limits<std::size_t>::max();
  const ExactDirectSparseStableFacetPositiveProbeBudget generous{
      maximum, maximum, maximum, maximum};
  const auto pre_stamp = forest.current_stamp();
  const auto entries_before = forest.observed_entries();
  const std::vector<ExactDirectSparseStableFacetForestEntry> entry_copy{
      entries_before.begin(), entries_before.end()};
  allocation_probe::begin();
  const auto exact =
      forest.probe_positive_full_key(key_for_handle(13U), generous);
  const auto miss =
      forest.probe_positive_full_key(key_for_handle(99U), generous);
  const std::size_t exact_probe_allocations = allocation_probe::finish();
  check(
      exact.certified_observed_for(pre_stamp) &&
          exact.bound_handle == 13U && exact.root_handle == 10U &&
          exact.component_size == 4U &&
          exact.positive_key_slot_visit_count == 4U &&
          exact.full_key_comparison_count == 4U &&
          exact.handle_index_slot_visit_count != 0U &&
          exact.parent_hop_count == 1U &&
          miss.certified_unobserved_for(pre_stamp) &&
          miss.positive_key_slot_visit_count == 5U &&
          miss.full_key_comparison_count == 4U &&
          exact_probe_allocations == 0U &&
          forest.current_stamp() == pre_stamp &&
          std::equal(
              forest.observed_entries().begin(),
              forest.observed_entries().end(),
              entry_copy.begin(),
              entry_copy.end()),
      "mask-zero bounded hit and miss compare full keys, resolve the root, allocate nothing and mutate nothing");

  const auto check_cap = [&](auto budget,
                             auto expected_decision,
                             std::string_view label) {
    allocation_probe::begin();
    const auto exhausted =
        forest.probe_positive_full_key(key_for_handle(13U), budget);
    const std::size_t allocations = allocation_probe::finish();
    check(
        exhausted.certified_budget_exhaustion() &&
            exhausted.certified_for(pre_stamp) &&
            exhausted.decision == expected_decision &&
            exhausted.bound_handle == 0U && exhausted.root_handle == 0U &&
            exhausted.component_size == 0U && allocations == 0U &&
            forest.current_stamp() == pre_stamp,
        std::string{label});
    return exhausted;
  };

  auto positive_slot_exact = generous;
  positive_slot_exact.maximum_positive_key_slot_visit_count =
      exact.positive_key_slot_visit_count;
  check(
      forest.probe_positive_full_key(
                key_for_handle(13U), positive_slot_exact)
          .certified_observed_for(pre_stamp),
      "the exact positive-slot cap succeeds");
  --positive_slot_exact.maximum_positive_key_slot_visit_count;
  const auto positive_slot_exhausted = check_cap(
      positive_slot_exact,
      ExactDirectSparseStableFacetPositiveProbeDecision::
          no_positive_key_slot_budget_exhausted,
      "the positive-slot cap minus one fails closed");

  auto full_key_exact = generous;
  full_key_exact.maximum_full_key_comparison_count =
      exact.full_key_comparison_count;
  check(
      forest.probe_positive_full_key(key_for_handle(13U), full_key_exact)
          .certified_observed_for(pre_stamp),
      "the exact full-key comparison cap succeeds");
  --full_key_exact.maximum_full_key_comparison_count;
  const auto full_key_exhausted = check_cap(
      full_key_exact,
      ExactDirectSparseStableFacetPositiveProbeDecision::
          no_full_key_comparison_budget_exhausted,
      "the full-key comparison cap minus one fails separately from slots");

  auto handle_slot_exact = generous;
  handle_slot_exact.maximum_handle_index_slot_visit_count =
      exact.handle_index_slot_visit_count;
  check(
      forest.probe_positive_full_key(key_for_handle(13U), handle_slot_exact)
          .certified_observed_for(pre_stamp),
      "the exact handle-index slot cap succeeds");
  --handle_slot_exact.maximum_handle_index_slot_visit_count;
  const auto handle_slot_exhausted = check_cap(
      handle_slot_exact,
      ExactDirectSparseStableFacetPositiveProbeDecision::
          no_handle_index_slot_budget_exhausted,
      "the handle-index slot cap minus one fails closed");

  auto parent_hop_exact = generous;
  parent_hop_exact.maximum_parent_hop_count = exact.parent_hop_count;
  check(
      forest.probe_positive_full_key(key_for_handle(13U), parent_hop_exact)
          .certified_observed_for(pre_stamp),
      "the exact parent-hop cap succeeds");
  --parent_hop_exact.maximum_parent_hop_count;
  const auto parent_hop_exhausted = check_cap(
      parent_hop_exact,
      ExactDirectSparseStableFacetPositiveProbeDecision::
          no_parent_hop_budget_exhausted,
      "the parent-hop cap minus one fails closed");

  const ExactDirectSparseStableFacetPositiveProbeBudget zero{};
  const auto root_with_zero_dsu_caps = forest.probe_positive_full_key(
      key_for_handle(10U),
      {exact.positive_key_slot_visit_count,
       exact.full_key_comparison_count,
       0U,
       0U});
  auto empty_initialization = initialize_with_positive_fingerprint_mask(0U);
  auto empty = std::move(*empty_initialization.forest);
  const auto empty_zero =
      empty.probe_positive_full_key(key_for_handle(10U), zero);
  check(
      root_with_zero_dsu_caps.certified_observed_for(pre_stamp) &&
          root_with_zero_dsu_caps.parent_hop_count == 0U &&
          root_with_zero_dsu_caps.handle_index_slot_visit_count == 0U &&
          empty_zero.certified_unobserved_for(empty.current_stamp()) &&
          empty_zero.positive_key_slot_visit_count == 0U,
      "zero caps suffice for an empty index and for DSU resolution of an already-root binding");

  using ProbeReceipt = ExactDirectSparseStableFacetPositiveProbeReceipt;
  const auto check_forged = [&](const auto& genuine,
                                std::string_view label) {
    ExactDirectSparseStableFacetPositiveProbeResult forged;
    static_cast<ProbeReceipt&>(forged) =
        static_cast<const ProbeReceipt&>(genuine);
    check(
        !any_positive_probe_certification(forged),
        std::string{label});
  };
  check_forged(exact, "a copied observed receipt cannot forge its mint");
  check_forged(miss, "a copied unobserved receipt cannot forge its mint");
  check_forged(
      positive_slot_exhausted,
      "a copied positive-slot exhaustion cannot forge its mint");
  auto tampered_budget = parent_hop_exhausted;
  ++tampered_budget.parent_hop_count;
  check(
      !any_positive_probe_certification(tampered_budget),
      "tampering a minted cap counter invalidates every certification");
  auto tampered_observed = exact;
  ++tampered_observed.root_handle;
  auto tampered_unobserved = miss;
  tampered_unobserved.disposition =
      ExactDirectSparseStableFacetPositiveProbeDisposition::contradiction;
  auto tampered_full_key_budget = full_key_exhausted;
  tampered_full_key_budget.decision =
      ExactDirectSparseStableFacetPositiveProbeDecision::
          contradiction_positive_key_index;
  check(
      !any_positive_probe_certification(tampered_observed) &&
          !any_positive_probe_certification(tampered_unobserved) &&
          !any_positive_probe_certification(tampered_full_key_budget) &&
          handle_slot_exhausted.certified_budget_exhaustion(),
      "observed, unobserved and budget public receipt mutations are rejected");
  ProbeReceipt forged_contradiction_receipt =
      static_cast<const ProbeReceipt&>(positive_slot_exhausted);
  forged_contradiction_receipt.disposition =
      ExactDirectSparseStableFacetPositiveProbeDisposition::contradiction;
  forged_contradiction_receipt.decision =
      ExactDirectSparseStableFacetPositiveProbeDecision::
          contradiction_positive_key_index;
  ExactDirectSparseStableFacetPositiveProbeResult forged_contradiction;
  static_cast<ProbeReceipt&>(forged_contradiction) =
      forged_contradiction_receipt;
  check(
      !forged_contradiction.certified_fail_closed_contradiction(),
      "a complete public contradiction receipt cannot forge a private mint");

  const std::array later_insertion{
      ExactDirectSparseStableFacetInsertion{20U, key_for_handle(20U)}};
  auto later = forest.prepare_batch(later_insertion, {});
  const auto later_commit = forest.commit(std::move(*later.ticket));
  check(
      later_commit.certified_commit() && exact.certified_for(pre_stamp) &&
          !exact.certified_for(forest.current_stamp()),
      "a genuine probe remains historical and fails certification for a later forest stamp");
}

void test_positive_index_budget_preflight() {
  auto maximum_namespace_initialization =
      initialize_exact_direct_sparse_stable_facet_forest(
          {std::numeric_limits<std::size_t>::max(),
           digest("maximum-scalar-namespace")},
          generous_budget());
  check(
      maximum_namespace_initialization.certified_initialized() &&
          maximum_namespace_initialization.forest->observed_entries().empty() &&
          maximum_namespace_initialization.forest->
                  materialized_handle_index_slot_count() == 0U &&
          maximum_namespace_initialization.forest->
                  materialized_positive_key_index_slot_count() == 0U,
      "even a SIZE_MAX scalar namespace materializes neither sparse index");

  auto one_row_budget = generous_budget();
  one_row_budget.maximum_observed_handle_count = 1U;
  auto initialization = initialize(one_row_budget);
  auto forest = std::move(*initialization.forest);
  const auto pre_stamp = forest.current_stamp();
  const std::array insertions{
      ExactDirectSparseStableFacetInsertion{1U, key_for_handle(1U)},
      ExactDirectSparseStableFacetInsertion{2U, key_for_handle(2U)},
  };
  const auto rejected = forest.prepare_batch(insertions, {});
  check(
      !rejected.ticket.has_value() &&
          rejected.decision ==
              ExactDirectSparseStableFacetForestPreparationDecision::
                  no_budget_exhausted &&
          forest.current_stamp() == pre_stamp &&
          forest.observed_entries().empty() &&
          forest.materialized_handle_index_slot_count() == 0U &&
          forest.materialized_positive_key_index_slot_count() == 0U &&
          forest.certified_structure_only_forest(),
      "the durable row budget rejects before reserving either sparse index");
}

void test_positive_bijection_sibling_adversaries_and_idempotence() {
  const auto shared_key = key_for_handle(300U);
  const std::array key_to_first_handle{
      ExactDirectSparseStableFacetInsertion{301U, shared_key}};
  const std::array key_to_second_handle{
      ExactDirectSparseStableFacetInsertion{302U, shared_key}};
  auto key_sibling_initialization = initialize();
  auto key_sibling_forest = std::move(*key_sibling_initialization.forest);
  const auto key_sibling_pre_stamp = key_sibling_forest.current_stamp();
  auto key_to_first =
      key_sibling_forest.prepare_batch(key_to_first_handle, {});
  auto key_to_second =
      key_sibling_forest.prepare_batch(key_to_second_handle, {});
  check(
      key_to_first.certified_prepared() && key_to_second.certified_prepared() &&
          key_to_first.pre_stamp == key_sibling_pre_stamp &&
          key_to_second.pre_stamp == key_sibling_pre_stamp,
      "K-to-H1 and K-to-H2 may stage only as siblings of one frozen pre-state");
  const auto first_commit =
      key_sibling_forest.commit(std::move(*key_to_first.ticket));
  const auto after_first_commit = key_sibling_forest.current_stamp();
  const auto stale_second =
      key_sibling_forest.commit(std::move(*key_to_second.ticket));
  const auto key_binding_after_stale =
      key_sibling_forest.lookup_positive_full_key(shared_key);
  const auto absent_key_receipt =
      key_sibling_forest.lookup_positive_full_key(key_for_handle(303U));
  check(
      first_commit.certified_commit() && stale_second.ticket_consumed &&
          !stale_second.state_mutated &&
          stale_second.decision ==
              ExactDirectSparseStableFacetForestCommitDecision::
                  no_stale_or_sibling_ticket_rejected &&
          key_sibling_forest.current_stamp() == after_first_commit &&
          key_binding_after_stale.certified_observed_for(after_first_commit) &&
          absent_key_receipt.certified_unobserved_for(after_first_commit) &&
          key_binding_after_stale.bound_handle == 301U &&
          key_sibling_forest.lookup(302U).certified_unobserved(),
      "committing K-to-H1 makes sibling K-to-H2 stale without rebinding the key");

  const auto key_slot_count =
      key_sibling_forest.materialized_positive_key_index_slot_count();
  auto idempotent =
      key_sibling_forest.prepare_batch(key_to_first_handle, {});
  const auto idempotent_commit =
      key_sibling_forest.commit(std::move(*idempotent.ticket));
  const auto after_idempotent = key_sibling_forest.current_stamp();
  const auto key_binding_after_idempotent =
      key_sibling_forest.lookup_positive_full_key(shared_key);
  check(
      idempotent_commit.certified_commit() &&
          idempotent_commit.inserted_handle_count == 0U &&
          idempotent_commit.compatible_repeat_count == 1U &&
          idempotent_commit.post_stamp.observed_handle_count == 1U &&
          idempotent_commit.post_stamp.component_count == 1U &&
          key_sibling_forest.materialized_positive_key_index_slot_count() ==
              key_slot_count &&
          key_binding_after_idempotent.certified_observed_for(
              after_idempotent) &&
          key_binding_after_idempotent.bound_handle == 301U &&
          key_binding_after_idempotent.root_handle == 301U &&
          key_binding_after_idempotent.component_size == 1U,
      "a cross-batch identical key-handle repeat advances history without duplicating index state");
  check(
      key_binding_after_stale.certified_observed_for(after_first_commit) &&
          !key_binding_after_stale.certified_observed_for(after_idempotent) &&
          absent_key_receipt.certified_unobserved_for(after_first_commit) &&
          !absent_key_receipt.certified_unobserved_for(after_idempotent),
      "genuine old hit and miss receipts are rejected against the post-commit forest stamp");

  const auto before_cross_batch_rebind = key_sibling_forest.current_stamp();
  const auto cross_batch_rebind =
      key_sibling_forest.prepare_batch(key_to_second_handle, {});
  check(
      !cross_batch_rebind.ticket.has_value() &&
          cross_batch_rebind.decision ==
              ExactDirectSparseStableFacetForestPreparationDecision::
                  contradiction_stable_handle_key_collision &&
          key_sibling_forest.current_stamp() == before_cross_batch_rebind &&
          key_sibling_forest.lookup_positive_full_key(shared_key)
                  .bound_handle == 301U,
      "a later batch cannot rebind an idempotently retained complete key");

  const auto first_key = key_for_handle(400U);
  const auto second_key = key_for_handle(401U);
  const std::array handle_to_first_key{
      ExactDirectSparseStableFacetInsertion{410U, first_key}};
  const std::array handle_to_second_key{
      ExactDirectSparseStableFacetInsertion{410U, second_key}};
  auto handle_sibling_initialization = initialize();
  auto handle_sibling_forest = std::move(*handle_sibling_initialization.forest);
  auto handle_to_first =
      handle_sibling_forest.prepare_batch(handle_to_first_key, {});
  auto handle_to_second =
      handle_sibling_forest.prepare_batch(handle_to_second_key, {});
  check(
      handle_to_first.certified_prepared() &&
          handle_to_second.certified_prepared(),
      "H-to-K1 and H-to-K2 may stage only as siblings of one frozen pre-state");
  const auto handle_first_commit =
      handle_sibling_forest.commit(std::move(*handle_to_first.ticket));
  const auto handle_post_stamp = handle_sibling_forest.current_stamp();
  const auto handle_stale_second =
      handle_sibling_forest.commit(std::move(*handle_to_second.ticket));
  check(
      handle_first_commit.certified_commit() &&
          handle_stale_second.ticket_consumed &&
          !handle_stale_second.state_mutated &&
          handle_sibling_forest.current_stamp() == handle_post_stamp &&
          handle_sibling_forest.lookup_positive_full_key(first_key)
                  .bound_handle == 410U &&
          handle_sibling_forest.lookup_positive_full_key(second_key)
                  .certified_unobserved() &&
          handle_sibling_forest.lookup(410U).facet_key == first_key,
      "committing H-to-K1 makes sibling H-to-K2 stale without rewriting the handle");

  auto within_batch_initialization = initialize();
  auto within_batch_forest = std::move(*within_batch_initialization.forest);
  const auto within_batch_stamp = within_batch_forest.current_stamp();
  const std::array one_handle_two_keys{
      ExactDirectSparseStableFacetInsertion{510U, first_key},
      ExactDirectSparseStableFacetInsertion{510U, second_key},
  };
  const auto within_batch_rejected =
      within_batch_forest.prepare_batch(one_handle_two_keys, {});
  check(
      !within_batch_rejected.ticket.has_value() &&
          within_batch_rejected.decision ==
              ExactDirectSparseStableFacetForestPreparationDecision::
                  contradiction_stable_handle_key_collision &&
          within_batch_forest.current_stamp() == within_batch_stamp &&
          within_batch_forest.observed_entries().empty() &&
          within_batch_forest.materialized_handle_index_slot_count() == 0U &&
          within_batch_forest.materialized_positive_key_index_slot_count() ==
              0U,
      "one intra-batch handle cannot name two complete keys before reservation");
}

void test_sibling_ticket_survives_physical_rehash() {
  auto initialization = initialize();
  auto forest = std::move(*initialization.forest);
  const auto initial_stamp = forest.current_stamp();
  const std::array small_insertions{
      ExactDirectSparseStableFacetInsertion{600U, key_for_handle(600U)}};
  const std::array large_insertions{
      ExactDirectSparseStableFacetInsertion{100U, key_for_handle(100U)},
      ExactDirectSparseStableFacetInsertion{101U, key_for_handle(101U)},
      ExactDirectSparseStableFacetInsertion{102U, key_for_handle(102U)},
      ExactDirectSparseStableFacetInsertion{103U, key_for_handle(103U)},
      ExactDirectSparseStableFacetInsertion{104U, key_for_handle(104U)},
      ExactDirectSparseStableFacetInsertion{105U, key_for_handle(105U)},
      ExactDirectSparseStableFacetInsertion{106U, key_for_handle(106U)},
      ExactDirectSparseStableFacetInsertion{107U, key_for_handle(107U)},
  };
  auto small = forest.prepare_batch(small_insertions, {});
  auto large = forest.prepare_batch(large_insertions, {});
  check(
      small.certified_prepared() && large.certified_prepared() &&
          forest.current_stamp() == initial_stamp &&
          forest.observed_entries().empty() &&
          forest.materialized_handle_index_slot_count() >= 16U &&
          forest.materialized_positive_key_index_slot_count() >= 16U &&
          forest.outstanding_ticket_count() == 2U,
      "a sibling may grow row and both index capacities without semantic mutation");
  allocation_probe::begin();
  const auto committed_small = forest.commit(std::move(*small.ticket));
  const std::size_t commit_allocation_count = allocation_probe::finish();
  check(
      committed_small.certified_commit() &&
          committed_small.inserted_handle_count == 1U &&
          forest.lookup(600U).certified_observed() &&
          forest.lookup_positive_full_key(key_for_handle(600U))
                  .certified_observed() &&
          commit_allocation_count == 0U,
      "a ticket prepared before sibling rehashes commits without retained slot addresses");
  const auto stamp_after_small = forest.current_stamp();
  const auto stale_large = forest.commit(std::move(*large.ticket));
  check(
      stale_large.ticket_consumed && !stale_large.state_mutated &&
          stale_large.decision ==
              ExactDirectSparseStableFacetForestCommitDecision::
                  no_stale_or_sibling_ticket_rejected &&
          forest.current_stamp() == stamp_after_small &&
          forest.observed_entries().size() == 1U &&
          forest.lookup(600U).certified_observed() &&
          forest.lookup_positive_full_key(key_for_handle(600U))
                  .certified_observed() &&
          forest.lookup_positive_full_key(key_for_handle(100U))
                  .certified_unobserved(),
      "stale sibling rejection leaves append rows and both rehashed indexes intact");
}

void test_prepared_sparse_shadow_preview_q_like_replay() {
  auto initialization = initialize();
  auto forest = std::move(*initialization.forest);
  BoundedStableFacetOracle oracle;
  const std::array initial_insertions{
      ExactDirectSparseStableFacetInsertion{10U, key_for_handle(10U)},
      ExactDirectSparseStableFacetInsertion{20U, key_for_handle(20U)},
      ExactDirectSparseStableFacetInsertion{30U, key_for_handle(30U)},
      ExactDirectSparseStableFacetInsertion{40U, key_for_handle(40U)},
  };
  const std::array initial_unions{
      ExactDirectSparseStableFacetUnion{10U, 20U},
      ExactDirectSparseStableFacetUnion{30U, 40U},
  };
  commit_oracle_batch(
      forest,
      oracle,
      initial_insertions,
      initial_unions,
      "preview pre-forest");

  const std::array insertions{
      ExactDirectSparseStableFacetInsertion{20U, key_for_handle(20U)},
      ExactDirectSparseStableFacetInsertion{50U, key_for_handle(50U)},
      ExactDirectSparseStableFacetInsertion{60U, key_for_handle(60U)},
      ExactDirectSparseStableFacetInsertion{70U, key_for_handle(70U)},
      ExactDirectSparseStableFacetInsertion{80U, key_for_handle(80U)},
  };
  const std::array unions{
      ExactDirectSparseStableFacetUnion{60U, 70U},
      ExactDirectSparseStableFacetUnion{50U, 60U},
      ExactDirectSparseStableFacetUnion{20U, 50U},
      ExactDirectSparseStableFacetUnion{50U, 50U},
      ExactDirectSparseStableFacetUnion{70U, 30U},
      ExactDirectSparseStableFacetUnion{10U, 50U},
      ExactDirectSparseStableFacetUnion{70U, 60U},
  };
  constexpr std::array<ExactDirectSparseStableFacetHandle, 8U> requested{
      10U, 20U, 30U, 40U, 50U, 60U, 70U, 80U};
  auto prepared = forest.prepare_batch(insertions, unions);
  const auto pre_stamp = forest.current_stamp();
  auto preview = forest.preview_prepared_batch(
      *prepared.ticket, requested, generous_preview_budget());
  check(
      prepared.certified_prepared() &&
          prepared.compatible_repeat_count == 1U &&
          preview.certified_prepared_preview() &&
          preview.preview->certified_for(*prepared.ticket, pre_stamp) &&
          forest.current_stamp() == pre_stamp,
      "q-like preparation mints one ticket-bound mutation-free sparse preview");

  const auto& receipt = preview.preview->receipt();
  bool expected_records = receipt.records.size() == requested.size();
  for (std::size_t index = 0U;
       expected_records && index + 1U < receipt.records.size();
       ++index) {
    const auto& record = receipt.records[index];
    expected_records =
        record.requested_handle == requested[index] &&
        record.post_root_handle == 10U && record.post_component_size == 7U;
  }
  if (expected_records && !receipt.records.empty()) {
    const auto& singleton = receipt.records.back();
    expected_records = singleton.requested_handle == 80U &&
                       singleton.post_root_handle == 80U &&
                       singleton.post_component_size == 1U;
  }
  check(
      expected_records && receipt.shadow_node_count == 6U &&
          receipt.shadow_node_upper_bound == 26U &&
          receipt.total_entry_upper_bound == 34U &&
          receipt.union_request_count == unions.size() &&
          receipt.expected_effective_union_count == 4U &&
          receipt.sparse_pre_roots_and_new_handles_only &&
          receipt.requested_handles_cover_all_ticket_touched_handles &&
          receipt.canonical_union_replay_exact,
      "repeat, new singleton, chain, multifusion, duplicate and self unions replay exactly over six sparse roots");

  constexpr std::array<ExactDirectSparseStableFacetHandle, 1U>
      partial_requested{10U};
  const auto partial_preview = forest.preview_prepared_batch(
      *prepared.ticket, partial_requested, generous_preview_budget());
  check(
      partial_preview.certified_prepared_preview() &&
          !partial_preview.preview->receipt()
               .requested_handles_cover_all_ticket_touched_handles,
      "a sealed subset preview stays exact but cannot claim exhaustive ticket-handle coverage");

  auto tampered = receipt;
  ++tampered.expected_effective_union_count;
  check(
      !preview.preview->certifies_receipt(tampered),
      "an edited effective-union count cannot pass the private preview mint");
  tampered = receipt;
  tampered.records.front().post_root_handle = 30U;
  check(
      !preview.preview->certifies_receipt(tampered),
      "an edited post-root record cannot pass the private preview mint");
  tampered = receipt;
  tampered.canonical_union_replay_exact = false;
  check(
      !preview.preview->certifies_receipt(tampered) &&
          preview.preview->certifies_receipt(receipt),
      "a copied public receipt certifies only at exact equality with the immutable sealed receipt");
  ExactDirectSparseStableFacetForestPreparedPreview forged;
  check(
      !forged.certified_sparse_shadow_preview(),
      "default construction cannot forge a sealed prepared preview");

  const auto expected_post_records = receipt.records;
  const std::size_t expected_effective_union_count =
      receipt.expected_effective_union_count;
  allocation_probe::begin();
  const auto committed = forest.commit(std::move(*prepared.ticket));
  const std::size_t commit_allocations = allocation_probe::finish();
  bool post_lookup_equality = committed.certified_commit() &&
                              commit_allocations == 0U &&
                              committed.effective_union_count ==
                                  expected_effective_union_count;
  for (const auto& expected : expected_post_records) {
    const auto observed = forest.lookup(expected.requested_handle);
    post_lookup_equality =
        post_lookup_equality && observed.certified_observed() &&
        observed.root_handle == expected.post_root_handle &&
        observed.component_size == expected.post_component_size;
  }
  check(
      post_lookup_equality &&
          preview.preview->certified_sparse_shadow_preview() &&
          !preview.preview->certified_for(*prepared.ticket, pre_stamp),
      "every sealed post-root and size equals the allocation-free post-commit forest lookup");
}

void test_prepared_sparse_shadow_preview_ticket_identity() {
  auto initialization = initialize();
  auto foreign_initialization = initialize();
  auto forest = std::move(*initialization.forest);
  auto foreign_forest = std::move(*foreign_initialization.forest);
  const std::array insertions{
      ExactDirectSparseStableFacetInsertion{21U, key_for_handle(21U)}};
  constexpr std::array<ExactDirectSparseStableFacetHandle, 1U> requested{21U};
  auto sibling_a = forest.prepare_batch(insertions, {});
  auto sibling_b = forest.prepare_batch(insertions, {});
  const auto pre_stamp = forest.current_stamp();
  auto preview_a = forest.preview_prepared_batch(
      *sibling_a.ticket, requested, generous_preview_budget());
  auto preview_b = forest.preview_prepared_batch(
      *sibling_b.ticket, requested, generous_preview_budget());
  check(
      sibling_a.ticket->canonical_batch_digest() ==
              sibling_b.ticket->canonical_batch_digest() &&
          preview_a.certified_prepared_preview() &&
          preview_b.certified_prepared_preview() &&
          preview_a.preview->certified_for(*sibling_a.ticket, pre_stamp) &&
          !preview_a.preview->certified_for(*sibling_b.ticket, pre_stamp) &&
          preview_b.preview->certified_for(*sibling_b.ticket, pre_stamp) &&
          !preview_b.preview->certified_for(*sibling_a.ticket, pre_stamp),
      "private shared identities distinguish siblings with identical stamps and batch digests");

  ExactDirectSparseStableFacetForestPreparedBatch moved_ticket =
      std::move(*sibling_a.ticket);
  ExactDirectSparseStableFacetForestPreparedPreview moved_preview =
      std::move(*preview_a.preview);
  check(
      moved_preview.certified_for(moved_ticket, pre_stamp) &&
          !moved_preview.certified_for(*sibling_a.ticket, pre_stamp) &&
          !preview_a.preview->certified_sparse_shadow_preview(),
      "ticket and preview moves preserve one identity without minting an ABA sibling");

  const auto foreign_attempt = foreign_forest.preview_prepared_batch(
      moved_ticket, requested, generous_preview_budget());
  check(
      !foreign_attempt.preview.has_value() &&
          foreign_attempt.decision ==
              ExactDirectSparseStableFacetForestPreparedPreviewDecision::
                  no_foreign_ticket_rejected &&
          moved_ticket.valid(),
      "a foreign forest rejects the ticket without consuming its identity");

  const auto committed = forest.commit(std::move(moved_ticket));
  const auto stale_attempt = forest.preview_prepared_batch(
      *sibling_b.ticket, requested, generous_preview_budget());
  check(
      committed.certified_commit() &&
          moved_preview.certified_sparse_shadow_preview() &&
          !moved_preview.certified_for(moved_ticket, pre_stamp) &&
          preview_b.preview->certified_for(*sibling_b.ticket, pre_stamp) &&
          !preview_b.preview->certified_for(
              *sibling_b.ticket, forest.current_stamp()) &&
          !stale_attempt.preview.has_value() &&
          stale_attempt.decision ==
              ExactDirectSparseStableFacetForestPreparedPreviewDecision::
                  no_stale_or_sibling_ticket_rejected,
      "a sibling seal remains historical but fails against the freshly read post-commit stamp");

  ExactDirectSparseStableFacetForestPreparedPreview orphaned_preview;
  {
    auto lifetime_initialization = initialize();
    auto lifetime_forest = std::move(*lifetime_initialization.forest);
    auto lifetime_ticket = lifetime_forest.prepare_batch(insertions, {});
    auto lifetime_preview = lifetime_forest.preview_prepared_batch(
        *lifetime_ticket.ticket, requested, generous_preview_budget());
    orphaned_preview = std::move(*lifetime_preview.preview);
  }
  check(
      orphaned_preview.certified_sparse_shadow_preview(),
      "the sealed receipt retains its unique identity safely after ticket destruction without a raw-pointer ABA seam");
}

void test_prepared_sparse_shadow_preview_caps_and_size_max() {
  auto initialization = initialize();
  auto forest = std::move(*initialization.forest);
  const std::array insertions{
      ExactDirectSparseStableFacetInsertion{1U, key_for_handle(1U)},
      ExactDirectSparseStableFacetInsertion{2U, key_for_handle(2U)},
  };
  const std::array unions{ExactDirectSparseStableFacetUnion{1U, 2U}};
  constexpr std::array<ExactDirectSparseStableFacetHandle, 2U> requested{1U,
                                                                        2U};
  auto prepared = forest.prepare_batch(insertions, unions);
  const auto pre_stamp = forest.current_stamp();
  const ExactDirectSparseStableFacetForestPreparedPreviewBudget exact_budget{
      2U, 6U, 1U, 8U};
  const auto exact = forest.preview_prepared_batch(
      *prepared.ticket, requested, exact_budget);
  check(
      exact.certified_prepared_preview() &&
          exact.preview->receipt().shadow_node_upper_bound == 6U &&
          exact.preview->receipt().total_entry_upper_bound == 8U,
      "the exact conservative preview caps admit the two-root q-like batch");

  const auto check_cap_minus_one = [&](auto budget, std::string_view label) {
    allocation_probe::begin();
    const auto rejected = forest.preview_prepared_batch(
        *prepared.ticket, requested, budget);
    const std::size_t allocation_count = allocation_probe::finish();
    check(
        !rejected.preview.has_value() &&
            rejected.decision ==
                ExactDirectSparseStableFacetForestPreparedPreviewDecision::
                    no_budget_exhausted &&
            allocation_count == 0U &&
            !rejected.forest_logical_state_mutated &&
            forest.current_stamp() == pre_stamp,
        std::string{label});
  };
  auto requested_cap_minus_one = exact_budget;
  requested_cap_minus_one.maximum_requested_handle_count = 1U;
  check_cap_minus_one(
      requested_cap_minus_one,
      "requested-record cap minus one rejects before preview allocation");
  auto shadow_cap_minus_one = exact_budget;
  shadow_cap_minus_one.maximum_shadow_node_count = 5U;
  check_cap_minus_one(
      shadow_cap_minus_one,
      "shadow-node cap minus one rejects before preview allocation");
  auto union_cap_minus_one = exact_budget;
  union_cap_minus_one.maximum_union_replay_count = 0U;
  check_cap_minus_one(
      union_cap_minus_one,
      "union-replay cap minus one rejects before preview allocation");
  auto total_cap_minus_one = exact_budget;
  total_cap_minus_one.maximum_total_entry_count = 7U;
  check_cap_minus_one(
      total_cap_minus_one,
      "total-entry cap minus one rejects before preview allocation");

  constexpr std::array<ExactDirectSparseStableFacetHandle, 2U>
      duplicate_requested{1U, 1U};
  allocation_probe::begin();
  const auto duplicate_rejected = forest.preview_prepared_batch(
      *prepared.ticket, duplicate_requested, generous_preview_budget());
  const std::size_t duplicate_rejection_allocations =
      allocation_probe::finish();
  constexpr std::array<ExactDirectSparseStableFacetHandle, 1U>
      unknown_requested{3U};
  allocation_probe::begin();
  const auto unknown_rejected = forest.preview_prepared_batch(
      *prepared.ticket, unknown_requested, generous_preview_budget());
  const std::size_t unknown_rejection_allocations =
      allocation_probe::finish();
  check(
      duplicate_rejected.decision ==
              ExactDirectSparseStableFacetForestPreparedPreviewDecision::
                  no_requested_handle_shape_rejected &&
          unknown_rejected.decision ==
              ExactDirectSparseStableFacetForestPreparedPreviewDecision::
                  no_requested_handle_unknown_rejected &&
          duplicate_rejection_allocations == 0U &&
          unknown_rejection_allocations == 0U &&
          forest.current_stamp() == pre_stamp,
      "noncanonical or unavailable requested handles fail before shadow allocation");

  auto maximum_namespace_initialization =
      initialize_exact_direct_sparse_stable_facet_forest(
          {std::numeric_limits<std::size_t>::max(),
           digest("preview-maximum-scalar-namespace")},
          generous_budget());
  auto maximum_namespace_forest =
      std::move(*maximum_namespace_initialization.forest);
  constexpr auto maximum_valid_handle =
      std::numeric_limits<std::size_t>::max() - 1U;
  const std::array maximum_insertion{
      ExactDirectSparseStableFacetInsertion{
          maximum_valid_handle,
          facet(std::array<PointId, 2U>{7U, 11U})}};
  auto maximum_ticket =
      maximum_namespace_forest.prepare_batch(maximum_insertion, {});
  const std::array maximum_requested{maximum_valid_handle};
  const ExactDirectSparseStableFacetForestPreparedPreviewBudget
      maximum_preview_budget{
          std::numeric_limits<std::size_t>::max(),
          std::numeric_limits<std::size_t>::max(),
          std::numeric_limits<std::size_t>::max(),
          std::numeric_limits<std::size_t>::max()};
  const auto maximum_preview = maximum_namespace_forest.preview_prepared_batch(
      *maximum_ticket.ticket,
      maximum_requested,
      maximum_preview_budget);
  const std::array outside_namespace_requested{
      std::numeric_limits<std::size_t>::max()};
  allocation_probe::begin();
  const auto outside_namespace =
      maximum_namespace_forest.preview_prepared_batch(
          *maximum_ticket.ticket,
          outside_namespace_requested,
          maximum_preview_budget);
  const std::size_t outside_namespace_allocations =
      allocation_probe::finish();
  check(
      maximum_preview.certified_prepared_preview() &&
          maximum_preview.preview->records().size() == 1U &&
          maximum_preview.preview->records().front().requested_handle ==
              maximum_valid_handle &&
          maximum_namespace_forest.observed_entries().empty() &&
          outside_namespace.decision ==
              ExactDirectSparseStableFacetForestPreparedPreviewDecision::
                  no_requested_handle_shape_rejected &&
          outside_namespace_allocations == 0U,
      "SIZE_MAX caps and namespace remain scalar while SIZE_MAX itself is rejected as the exclusive bound");
}

void run_tests() {
  constexpr std::size_t handle_a = 900'000'000U;
  constexpr std::size_t handle_b = 2U;
  constexpr std::size_t handle_c = 500'000U;
  const auto key_a = facet(std::array<PointId, 2U>{1U, 7U});
  const auto key_b = facet(std::array<PointId, 2U>{2U, 9U});
  const auto key_c = facet(std::array<PointId, 3U>{3U, 4U, 8U});

  auto initialization = initialize();
  check(
      initialization.certified_initialized() &&
          initialization.namespace_capacity_not_materialized &&
          initialization.forest->observed_entries().empty() &&
          initialization.forest->materialized_handle_index_slot_count() == 0U &&
          initialization.forest->
                  materialized_positive_key_index_slot_count() == 0U,
      "a billion-token namespace initializes with zero rows and zero slots in both indexes");
  auto forest = std::move(*initialization.forest);
  const auto initial_stamp = forest.current_stamp();
  const auto missing = forest.lookup(handle_a);
  check(
      missing.certified_unobserved() &&
          !missing.missing_handle_means_source_absent &&
          !missing.source_exactness_claimed,
      "an unobserved sparse handle is not promoted to source absence");
  allocation_probe::begin();
  const auto missing_positive = forest.lookup_positive_full_key(key_a);
  const std::size_t empty_positive_lookup_allocations =
      allocation_probe::finish();
  check(
      missing_positive.certified_unobserved() &&
          missing_positive.forest_stamp == initial_stamp &&
          missing_positive.slot_visit_count == 0U &&
          empty_positive_lookup_allocations == 0U,
      "an empty full-key index certifies an allocation-free unobserved result at its forest stamp");
  auto invalid_positive_key = key_a;
  invalid_positive_key.point_count = 1U;
  const auto invalid_positive =
      forest.lookup_positive_full_key(invalid_positive_key);
  check(
      invalid_positive.disposition ==
              ExactDirectSparseStableFacetPositiveLookupDisposition::
                  input_key_rejected &&
          !invalid_positive.certified_observed() &&
          !invalid_positive.certified_unobserved() &&
          invalid_positive.forest_stamp == initial_stamp,
      "the positive lookup rejects a noncanonical requested key explicitly");
  check(
      forest.lookup(1'000'000'001U).disposition ==
          ExactDirectSparseStableFacetLookupDisposition::
              handle_out_of_namespace,
      "a handle outside the scalar namespace is rejected");

  auto point_count_eleven = key_a;
  point_count_eleven.point_count = point_count_eleven.point_ids.size() + 1U;
  auto point_count_size_max = key_a;
  point_count_size_max.point_count =
      std::numeric_limits<std::size_t>::max();
  const std::array malformed_point_count_eleven{
      ExactDirectSparseStableFacetInsertion{
          handle_a, point_count_eleven}};
  const std::array malformed_point_count_size_max{
      ExactDirectSparseStableFacetInsertion{
          handle_a, point_count_size_max}};
  const std::array malformed_same_handle_pair{
      ExactDirectSparseStableFacetInsertion{
          handle_a, point_count_size_max},
      ExactDirectSparseStableFacetInsertion{
          handle_a, point_count_eleven},
  };
  const auto check_malformed_batch_rejected =
      [&](std::span<const ExactDirectSparseStableFacetInsertion> malformed,
          std::string_view label) {
        const auto result = forest.prepare_batch(malformed, {});
        check(
            !result.ticket.has_value() &&
                result.decision ==
                    ExactDirectSparseStableFacetForestPreparationDecision::
                        no_input_shape_rejected &&
                !result.forest_logical_state_mutated &&
                forest.current_stamp() == initial_stamp &&
                forest.observed_entries().empty() &&
                forest.outstanding_ticket_count() == 0U,
            std::string{label});
      };
  check_malformed_batch_rejected(
      malformed_point_count_eleven,
      "point_count above fixed key capacity is rejected before staging");
  check_malformed_batch_rejected(
      malformed_point_count_size_max,
      "SIZE_MAX point_count is rejected before staging");
  check_malformed_batch_rejected(
      malformed_same_handle_pair,
      "two malformed insertions sharing one handle never reach sorting");

  const std::array insertions_a{
      ExactDirectSparseStableFacetInsertion{handle_a, key_a},
      ExactDirectSparseStableFacetInsertion{handle_b, key_b},
      ExactDirectSparseStableFacetInsertion{handle_a, key_a},
      ExactDirectSparseStableFacetInsertion{handle_c, key_c},
  };
  const std::array insertions_b{
      ExactDirectSparseStableFacetInsertion{handle_c, key_c},
      ExactDirectSparseStableFacetInsertion{handle_a, key_a},
      ExactDirectSparseStableFacetInsertion{handle_b, key_b},
      ExactDirectSparseStableFacetInsertion{handle_a, key_a},
  };
  const std::array unions_a{
      ExactDirectSparseStableFacetUnion{handle_a, handle_b},
      ExactDirectSparseStableFacetUnion{handle_c, handle_b},
      ExactDirectSparseStableFacetUnion{handle_b, handle_a},
  };
  const std::array unions_b{
      ExactDirectSparseStableFacetUnion{handle_b, handle_a},
      ExactDirectSparseStableFacetUnion{handle_b, handle_c},
      ExactDirectSparseStableFacetUnion{handle_a, handle_b},
  };
  auto prepared_a = forest.prepare_batch(insertions_a, unions_a);
  auto prepared_b = forest.prepare_batch(insertions_b, unions_b);
  check(
      prepared_a.certified_prepared() && prepared_b.certified_prepared() &&
          prepared_a.ticket->canonical_batch_digest() ==
              prepared_b.ticket->canonical_batch_digest() &&
          prepared_a.compatible_repeat_count == 1U &&
          prepared_a.duplicate_union_request_count == 1U &&
          forest.current_stamp() == initial_stamp &&
          forest.outstanding_ticket_count() == 2U,
      "canonical staging is independent of request order and mutation-free");

  allocation_probe::begin();
  auto committed = forest.commit(std::move(*prepared_a.ticket));
  const std::size_t first_commit_allocation_count = allocation_probe::finish();
  check(
      committed.certified_commit() && committed.inserted_handle_count == 3U &&
          committed.effective_union_count == 2U &&
          committed.compatible_repeat_count == 1U &&
          committed.post_stamp.observed_handle_count == 3U &&
          committed.post_stamp.component_count == 1U &&
          forest.observed_entries().size() == 3U &&
          forest.outstanding_ticket_count() == 1U &&
          first_commit_allocation_count == 0U,
      "one allocation-free commit inserts only observed handles and joins them");
  const auto stamp_after_commit = forest.current_stamp();
  auto stale = forest.commit(std::move(*prepared_b.ticket));
  check(
      stale.ticket_consumed && !stale.state_mutated &&
          stale.decision == ExactDirectSparseStableFacetForestCommitDecision::
                                no_stale_or_sibling_ticket_rejected &&
          forest.current_stamp() == stamp_after_commit &&
          forest.outstanding_ticket_count() == 0U,
      "the committed sibling makes every other sibling stale without mutation");

  const auto observed_a = forest.lookup(handle_a);
  const auto observed_b = forest.lookup(handle_b);
  const auto observed_c = forest.lookup(handle_c);
  check(
      observed_a.certified_observed() && observed_b.certified_observed() &&
          observed_c.certified_observed() &&
          observed_a.root_handle == handle_b &&
          observed_b.root_handle == handle_b &&
          observed_c.root_handle == handle_b &&
          observed_a.component_size == 3U && observed_a.facet_key == key_a,
      "deterministic union-by-size resolves the canonical stable root");
  const auto positive_a = forest.lookup_positive_full_key(key_a);
  const auto positive_b = forest.lookup_positive_full_key(key_b);
  const auto positive_c = forest.lookup_positive_full_key(key_c);
  check(
      positive_a.certified_observed() &&
          positive_b.certified_observed() &&
          positive_c.certified_observed() &&
          positive_a.bound_handle == handle_a &&
          positive_b.bound_handle == handle_b &&
          positive_c.bound_handle == handle_c &&
          positive_a.root_handle == handle_b &&
          positive_a.component_size == 3U &&
          positive_a.forest_stamp == forest.current_stamp(),
      "full-key positive lookup returns the immutable stable binding and current DSU root");

  const std::array repeated{
      ExactDirectSparseStableFacetInsertion{handle_b, key_b}};
  auto repeat_preparation = forest.prepare_batch(repeated, {});
  check(
      repeat_preparation.certified_prepared() &&
          repeat_preparation.new_handle_count == 0U &&
          repeat_preparation.compatible_repeat_count == 1U,
      "an identical stable-id to key repetition is accepted lazily");
  const auto repeat_commit =
      forest.commit(std::move(*repeat_preparation.ticket));
  check(
      repeat_commit.certified_commit() &&
          repeat_commit.inserted_handle_count == 0U &&
          forest.lookup(handle_b).facet_key == key_b,
      "a compatible repeat cannot rewrite the immutable key");

  const auto before_collision = forest.current_stamp();
  const std::array collision{
      ExactDirectSparseStableFacetInsertion{
          handle_b, facet(std::array<PointId, 2U>{2U, 10U})}};
  const auto collision_result = forest.prepare_batch(collision, {});
  check(
      !collision_result.ticket.has_value() &&
          collision_result.decision ==
              ExactDirectSparseStableFacetForestPreparationDecision::
                  contradiction_stable_handle_key_collision &&
          forest.current_stamp() == before_collision,
      "one stable id can never be rebound to a different exact key");

  const std::array unknown_union{
      ExactDirectSparseStableFacetUnion{handle_b, 123'456U}};
  const auto unknown_result = forest.prepare_batch({}, unknown_union);
  check(
      !unknown_result.ticket.has_value() &&
          unknown_result.decision ==
              ExactDirectSparseStableFacetForestPreparationDecision::
                  no_union_unknown_handle_rejected &&
          forest.current_stamp() == before_collision,
      "a union cannot manufacture an unobserved DSU handle");

  auto second_initialization = initialize();
  auto second_forest = std::move(*second_initialization.forest);
  auto foreign_preparation = second_forest.prepare_batch({}, {});
  const auto target_before_foreign = forest.current_stamp();
  const auto foreign =
      forest.commit(std::move(*foreign_preparation.ticket));
  check(
      foreign.ticket_consumed && !foreign.state_mutated &&
          foreign.decision ==
              ExactDirectSparseStableFacetForestCommitDecision::
                  no_foreign_ticket_rejected &&
          forest.current_stamp() == target_before_foreign &&
          second_forest.current_stamp().epoch == 0U &&
          second_forest.outstanding_ticket_count() == 0U,
      "a foreign ticket is consumed and rejected without mutating either forest");

  {
    auto discarded = forest.prepare_batch({}, {});
    check(
        discarded.certified_prepared() &&
            forest.outstanding_ticket_count() == 1U,
        "an empty semantic ticket occupies one bounded sibling slot");
  }
  check(
      forest.outstanding_ticket_count() == 0U,
      "destroying an unused ticket releases its sibling budget");

  auto tight = generous_budget();
  tight.maximum_batch_insertion_request_count = 1U;
  auto tight_initialization = initialize(tight);
  auto tight_forest = std::move(*tight_initialization.forest);
  const std::array two_insertions{
      ExactDirectSparseStableFacetInsertion{handle_a, key_a},
      ExactDirectSparseStableFacetInsertion{handle_b, key_b},
  };
  const auto budget_rejection =
      tight_forest.prepare_batch(two_insertions, {});
  check(
      !budget_rejection.ticket.has_value() &&
          budget_rejection.decision ==
              ExactDirectSparseStableFacetForestPreparationDecision::
                  no_budget_exhausted &&
          tight_forest.observed_entries().empty() &&
          tight_forest.materialized_handle_index_slot_count() == 0U &&
          tight_forest.materialized_positive_key_index_slot_count() == 0U,
      "strict request caps reject before staging or logical mutation");

  const auto checkpoint = forest.checkpoint();
  check(
      checkpoint.certified_honest_nonrestartable() &&
          checkpoint.stamp == forest.current_stamp() &&
          !checkpoint.sparse_entries_serialized &&
          !checkpoint.dsu_state_serialized && !checkpoint.restartable &&
          !checkpoint.checkpoint_restore_supported &&
          !checkpoint.source_exactness_claimed &&
          !checkpoint.vertical_maps_complete &&
          !checkpoint.public_status_claimed,
      "the semantic checkpoint honestly omits all restart state and claims");

  test_append_only_multibatch_index();
  test_flat_index_collision_chain();
  test_positive_full_key_index_collision_and_injective_binding();
  test_bounded_positive_full_key_probe_caps_collision_and_stamp();
  test_positive_index_budget_preflight();
  test_positive_bijection_sibling_adversaries_and_idempotence();
  test_sibling_ticket_survives_physical_rehash();
  test_prepared_sparse_shadow_preview_q_like_replay();
  test_prepared_sparse_shadow_preview_ticket_identity();
  test_prepared_sparse_shadow_preview_caps_and_size_max();
}

}  // namespace

int main() {
  run_tests();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "direct sparse stable facet forest tests passed\n";
  return 0;
}
