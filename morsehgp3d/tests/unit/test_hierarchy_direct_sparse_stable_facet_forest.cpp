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

[[nodiscard]] ExactDirectSparseStableFacetForestInitialization initialize(
    const ExactDirectSparseStableFacetForestBudget& budget =
        generous_budget()) {
  return initialize_exact_direct_sparse_stable_facet_forest(
      {1'000'000'001U, digest("normalized-structural-source")}, budget);
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
    check(
        expected_key != nullptr && observed.certified_observed() &&
            observed.root_handle == oracle.root_handle(handle) &&
            observed.component_size == oracle.component_size(handle) &&
            observed.facet_key == *expected_key,
        "flat-index lookup agrees with the bounded stable-handle oracle");
  }
  check(
      forest.materialized_handle_index_slot_count() >=
              2U * observed_entries.size() &&
          forest.materialized_handle_index_slot_count() < 1'000U,
      "the handle index scales with observed rows rather than the billion-token namespace");
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
          forest.outstanding_ticket_count() == 2U,
      "a sibling may grow row and index capacity without semantic mutation");
  allocation_probe::begin();
  const auto committed_small = forest.commit(std::move(*small.ticket));
  const std::size_t commit_allocation_count = allocation_probe::finish();
  check(
      committed_small.certified_commit() &&
          committed_small.inserted_handle_count == 1U &&
          forest.lookup(600U).certified_observed() &&
          commit_allocation_count == 0U,
      "a ticket prepared before a sibling rehash commits without retained slot addresses");
  const auto stamp_after_small = forest.current_stamp();
  const auto stale_large = forest.commit(std::move(*large.ticket));
  check(
      stale_large.ticket_consumed && !stale_large.state_mutated &&
          stale_large.decision ==
              ExactDirectSparseStableFacetForestCommitDecision::
                  no_stale_or_sibling_ticket_rejected &&
          forest.current_stamp() == stamp_after_small &&
          forest.observed_entries().size() == 1U &&
          forest.lookup(600U).certified_observed(),
      "stale sibling rejection leaves append rows and the rehashed index intact");
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
          initialization.forest->materialized_handle_index_slot_count() == 0U,
      "a billion-token namespace initializes with zero rows and zero index slots");
  auto forest = std::move(*initialization.forest);
  const auto initial_stamp = forest.current_stamp();
  const auto missing = forest.lookup(handle_a);
  check(
      missing.certified_unobserved() &&
          !missing.missing_handle_means_source_absent &&
          !missing.source_exactness_claimed,
      "an unobserved sparse handle is not promoted to source absence");
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
          tight_forest.observed_entries().empty(),
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
  test_sibling_ticket_survives_physical_rehash();
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
