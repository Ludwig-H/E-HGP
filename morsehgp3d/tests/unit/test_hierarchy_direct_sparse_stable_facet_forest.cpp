#include "morsehgp3d/hierarchy/direct_sparse_stable_facet_forest.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

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
          initialization.forest->observed_entries().empty(),
      "a billion-token namespace initializes with zero durable rows");
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

  auto committed = forest.commit(std::move(*prepared_a.ticket));
  check(
      committed.certified_commit() && committed.inserted_handle_count == 3U &&
          committed.effective_union_count == 2U &&
          committed.compatible_repeat_count == 1U &&
          committed.post_stamp.observed_handle_count == 3U &&
          committed.post_stamp.component_count == 1U &&
          forest.observed_entries().size() == 3U &&
          forest.outstanding_ticket_count() == 1U,
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
