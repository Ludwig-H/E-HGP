#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_stable_facet_descent_closure.hpp"
#include "morsehgp3d/hierarchy/facet_miniball.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::contract::CanonicalId;
using morsehgp3d::contract::CanonicalSha256Builder;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKAudit;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::ExclusionSet;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

static_assert(
    direct_sparse_stable_facet_descent_closure_schema_version == 1U);
static_assert(
    ExactDirectSparseStableFacetDescentClosureResult::backend ==
        std::string_view{"reference_cpu"});
static_assert(
    ExactDirectSparseStableFacetDescentClosureResult::profile ==
        std::string_view{"hgp_reduced"});
static_assert(
    ExactDirectSparseStableFacetDescentClosureResult::mode ==
        std::string_view{
            "stable_forest_full_key_strict_facet_descent_closure_v1"});
static_assert(
    !std::is_aggregate_v<
        ExactDirectSparseStableFacetDescentClosureResult>);

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

[[nodiscard]] CertifiedPoint3 point(
    double x,
    double y = 0.0,
    double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] ExactDirectSparseFacetKey key(
    std::initializer_list<PointId> point_ids) {
  ExactDirectSparseFacetKey result;
  result.point_count = point_ids.size();
  std::copy(
      point_ids.begin(), point_ids.end(), result.point_ids.begin());
  return result;
}

[[nodiscard]] CanonicalPointCloud ac_de_cloud() {
  const std::array<CertifiedPoint3, 5U> input{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0),
  };
  return CanonicalPointCloud::rejecting_duplicates(input);
}

[[nodiscard]] ExactDirectSparseStableFacetForestBudget forest_budget() {
  return {
      16U,
      64U,
      128U,
      16U,
      16U,
      16U,
      64U,
      128U,
      64U,
      4U,
  };
}

[[nodiscard]] ExactDirectSparseStableFacetForest make_bound_forest(
    const ExactDirectSparseFacetKey& root_key,
    const ExactDirectSparseFacetKey& bound_key,
    std::uint64_t fingerprint_mask) {
  auto initialization = initialize_exact_direct_sparse_stable_facet_forest(
      {128U, digest("stable-descent-closure-fixture"), fingerprint_mask},
      forest_budget());
  check(
      initialization.certified_initialized() &&
          initialization.forest.has_value(),
      "the stable closure fixture initializes a sparse forest");
  auto forest = std::move(*initialization.forest);
  const std::array insertions{
      ExactDirectSparseStableFacetInsertion{2U, root_key},
      ExactDirectSparseStableFacetInsertion{3U, bound_key},
  };
  const std::array unions{
      ExactDirectSparseStableFacetUnion{2U, 3U},
  };
  auto prepared = forest.prepare_batch(insertions, unions);
  check(
      prepared.certified_prepared() && prepared.ticket.has_value(),
      "the stable closure fixture prepares its positive DE component");
  if (prepared.ticket.has_value()) {
    const auto committed = forest.commit(std::move(*prepared.ticket));
    check(
        committed.certified_commit(),
        "the stable closure fixture commits its positive DE component");
  }
  return forest;
}

[[nodiscard]] ExactDirectSparseStableFacetForest make_positive_forest(
    std::uint64_t fingerprint_mask = 0U) {
  return make_bound_forest(
      key({0U, 1U}), key({0U, 4U}), fingerprint_mask);
}

[[nodiscard]] ExactDirectSparseStableFacetForest make_empty_forest() {
  auto initialization = initialize_exact_direct_sparse_stable_facet_forest(
      {128U, digest("stable-descent-empty-fixture"), 0U},
      forest_budget());
  check(
      initialization.certified_initialized() &&
          initialization.forest.has_value(),
      "the empty stable closure fixture initializes");
  return std::move(*initialization.forest);
}

[[nodiscard]] ExactLbvhTopKBudget generous_top_k_budget() {
  return {1024U, 1024U, 1024U, 1024U, 64U, 16U, 16U};
}

[[nodiscard]] ExactDirectSparseStableFacetPositiveProbeBudget
generous_probe_budget() {
  return {64U, 64U, 64U, 64U};
}

[[nodiscard]] std::size_t probe_budget_sum(
    const ExactDirectSparseStableFacetPositiveProbeBudget& budget) {
  return budget.maximum_positive_key_slot_visit_count +
         budget.maximum_full_key_comparison_count +
         budget.maximum_handle_index_slot_visit_count +
         budget.maximum_parent_hop_count;
}

[[nodiscard]] ExactDirectSparseStableFacetDescentClosureBudget
generous_closure_budget() {
  const auto probe = generous_probe_budget();
  return {
      16U,
      16U,
      16U,
      33U,
      16U * probe_budget_sum(probe),
      probe,
      generous_top_k_budget(),
  };
}

[[nodiscard]] ExactDirectSparseStableFacetDescentClosureResult build_at_level(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::span<const ExactDirectSparseStableFacetDescentClosureSeed> seeds,
    const ExactLevel& closed_level,
    const ExactDirectSparseStableFacetForest& forest,
    const ExactDirectSparseStableFacetDescentClosureBudget& budget,
    LbvhTraversalOrder order = LbvhTraversalOrder::near_first,
    const ExactDirectSparseStableFacetDescentClosureConfig& config = {}) {
  return build_exact_direct_sparse_stable_facet_descent_closure(
      index,
      cloud,
      seeds,
      closed_level,
      forest,
      budget,
      config,
      order);
}

[[nodiscard]] ExactDirectSparseStableFacetDescentClosureResult build(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::span<const ExactDirectSparseStableFacetDescentClosureSeed> seeds,
    const ExactDirectSparseStableFacetForest& forest,
    const ExactDirectSparseStableFacetDescentClosureBudget& budget,
    LbvhTraversalOrder order = LbvhTraversalOrder::near_first,
    const ExactDirectSparseStableFacetDescentClosureConfig& config = {}) {
  return build_at_level(
      index,
      cloud,
      seeds,
      level(33, 2),
      forest,
      budget,
      order,
      config);
}

[[nodiscard]] const ExactDirectSparseStableFacetDescentNode* find_node(
    const ExactDirectSparseStableFacetDescentClosureResult& result,
    const ExactDirectSparseFacetKey& facet_key) {
  const auto found = std::find_if(
      result.nodes().begin(),
      result.nodes().end(),
      [&facet_key](const auto& node) {
        return node.facet_key == facet_key;
      });
  return found == result.nodes().end() ? nullptr : &*found;
}

void check_empty_graph(
    const ExactDirectSparseStableFacetDescentClosureResult& result,
    std::string_view label) {
  check(
      result.nodes().empty() && result.edges().empty() &&
          result.seed_projections().empty(),
      std::string{label} + " publishes no partial graph");
}

void test_ac_to_de_exact_and_traversal_invariance() {
  const CanonicalPointCloud cloud = ac_de_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  auto forest = make_positive_forest();
  const ExactDirectSparseFacetKey ac = key({1U, 3U});
  const ExactDirectSparseFacetKey de = key({0U, 4U});
  const std::array seeds{
      ExactDirectSparseStableFacetDescentClosureSeed{0U, ac},
  };
  const auto stamp = forest.current_stamp();
  const std::vector<ExactDirectSparseStableFacetForestEntry> entries_before{
      forest.observed_entries().begin(), forest.observed_entries().end()};

  const auto near = build(
      index, cloud, seeds, forest, generous_closure_budget());
  const auto far = build(
      index,
      cloud,
      seeds,
      forest,
      generous_closure_budget(),
      LbvhTraversalOrder::far_first);
  check(
      near.certified_complete_relative_positive_closure() &&
          near.certified_for(stamp) &&
          far.certified_complete_relative_positive_closure() &&
          far.certified_for(stamp),
      "near-first and far-first certify the exact AC-to-DE closure");
  check(
      near.nodes().size() == 2U && near.edges().size() == 1U &&
          near.seed_projections().size() == 1U &&
          near.counters().evaluated_step_source_count == 1U &&
          near.counters().terminal_node_count == 1U &&
          near.counters().relative_positive_terminal_count == 1U &&
          near.counters().unresolved_terminal_count == 0U &&
          near.counters().top_k_query_count == 1U &&
          near.counters().source_miniball_build_count == 1U &&
          near.counters().successor_miniball_build_count == 1U,
      "AC reaches one stable positive DE terminal in one strict step");

  const auto* ac_node = find_node(near, ac);
  const auto* de_node = find_node(near, de);
  check(
      ac_node != nullptr && de_node != nullptr,
      "AC and DE are both interned by complete key");
  if (ac_node != nullptr && de_node != nullptr) {
    check(
        ac_node->step_evaluated &&
            ac_node->exact_squared_level == level(33, 2) &&
            ac_node->outgoing_edge_index.has_value() &&
            de_node->exact_squared_level == level(9, 2) &&
            !de_node->step_evaluated &&
            de_node->stable_positive_attachment ==
                std::optional<ExactDirectSparseStableFacetPositiveAttachment>{
                    ExactDirectSparseStableFacetPositiveAttachment{
                        3U, 2U, 2U}} &&
            ac_node->terminal_node_index == de_node->node_index &&
            near.seed_projections().front().root_node_index ==
                ac_node->node_index &&
            near.seed_projections().front().terminal_node_index ==
                de_node->node_index,
        "the terminal keeps distinct bound/root handles and exact levels");
    if (ac_node->outgoing_edge_index.has_value()) {
      const auto& edge = near.edges()[*ac_node->outgoing_edge_index];
      check(
          edge.source_node_index == ac_node->node_index &&
              edge.target_node_index == de_node->node_index &&
              edge.strict_step_witness.source_facet_squared_level ==
                  level(33, 2) &&
              edge.strict_step_witness.top_k_cutoff_squared_level ==
                  level(31, 2) &&
              edge.strict_step_witness.successor_facet_squared_level ==
                  level(9, 2),
          "the strict edge retains the exact 33/2, 31/2 and 9/2 seam");
    }
  }

  check(
      far.nodes().size() == near.nodes().size() &&
          far.edges().size() == near.edges().size() &&
          far.edges().front().strict_step_witness ==
              near.edges().front().strict_step_witness,
      "traversal order changes no exact scientific witness");
  const auto verification =
      verify_exact_direct_sparse_stable_facet_descent_closure(
          index,
          cloud,
          seeds,
          level(33, 2),
          forest,
          generous_closure_budget(),
          {},
          LbvhTraversalOrder::near_first,
          near);
  check(
      verification.result_certified &&
          verification.fresh_replay_equal_to_observed,
      "the fresh verifier reproduces the complete private result");
  check(
      forest.current_stamp() == stamp &&
          std::equal(
              forest.observed_entries().begin(),
              forest.observed_entries().end(),
              entries_before.begin(),
              entries_before.end()),
      "building and verifying the closure do not mutate the stable forest");
}

void test_source_hit_duplicates_and_empty_seed_set() {
  const CanonicalPointCloud cloud = ac_de_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  auto forest = make_positive_forest();
  const ExactDirectSparseFacetKey de = key({0U, 4U});
  const std::array source_hit_seed{
      ExactDirectSparseStableFacetDescentClosureSeed{0U, de},
  };
  const auto hit = build(
      index, cloud, source_hit_seed, forest, generous_closure_budget());
  check(
      hit.certified_complete_relative_positive_closure() &&
          hit.nodes().size() == 1U && hit.edges().empty() &&
          hit.counters().stable_forest_probe_count == 1U &&
          hit.counters().top_k_query_count == 0U &&
          hit.nodes().front().stable_positive_attachment ==
              std::optional<ExactDirectSparseStableFacetPositiveAttachment>{
                  ExactDirectSparseStableFacetPositiveAttachment{
                      3U, 2U, 2U}},
      "a positive source terminates immediately without geometry or top-k");

  const std::array duplicate_seeds{
      ExactDirectSparseStableFacetDescentClosureSeed{1U, de},
      ExactDirectSparseStableFacetDescentClosureSeed{0U, de},
  };
  auto duplicate_budget = generous_closure_budget();
  duplicate_budget.maximum_node_count = 1U;
  duplicate_budget.maximum_memo_slot_count = 3U;
  duplicate_budget.maximum_step_call_count = 1U;
  const auto duplicate = build(
      index, cloud, duplicate_seeds, forest, duplicate_budget);
  check(
      duplicate.certified_complete_relative_positive_closure() &&
          duplicate.nodes().size() == 1U &&
          duplicate.seed_projections().size() == 2U &&
          duplicate.seed_projections()[0U].seed_index == 0U &&
          duplicate.seed_projections()[1U].seed_index == 1U &&
          !duplicate.seed_projections()[0U].reused_existing_node &&
          duplicate.seed_projections()[1U].reused_existing_node &&
          duplicate.counters().input_seed_reference_count == 2U &&
          duplicate.counters().processed_seed_reference_count == 2U &&
          duplicate.counters().distinct_seed_key_count == 1U &&
          duplicate.counters().duplicate_seed_key_reference_count == 1U &&
          duplicate.counters().memoized_seed_reuse_count == 1U &&
          duplicate.counters().memoized_suffix_reuse_count == 0U &&
          duplicate.counters().evaluated_step_source_count == 1U,
      "physical permutation and duplicate keys reuse a node, not a suffix");

  const std::array distinct_seeds{
      ExactDirectSparseStableFacetDescentClosureSeed{0U, de},
      ExactDirectSparseStableFacetDescentClosureSeed{1U, key({1U, 3U})},
  };
  const auto distinct_over_cap = build(
      index, cloud, distinct_seeds, forest, duplicate_budget);
  check(
      distinct_over_cap.certified_budget_exhaustion() &&
          distinct_over_cap.decision() ==
              ExactDirectSparseStableFacetDescentClosureDecision::
                  no_node_budget_exhausted &&
          distinct_over_cap.counters().stable_forest_probe_count == 0U,
      "two distinct roots reject a one-node cap before any scientific probe");
  check_empty_graph(distinct_over_cap, "distinct-root node preflight");

  const std::span<const ExactDirectSparseStableFacetDescentClosureSeed> empty;
  auto empty_budget = generous_closure_budget();
  empty_budget.maximum_seed_count = 0U;
  empty_budget.maximum_node_count = 0U;
  empty_budget.maximum_step_call_count = 0U;
  empty_budget.maximum_memo_slot_count = 0U;
  empty_budget.maximum_lookup_work_count = 0U;
  empty_budget.positive_probe = {};
  empty_budget.top_k_query = {};
  const auto empty_result = build(
      index, cloud, empty, forest, empty_budget);
  check(
      empty_result.certified_complete_relative_positive_closure() &&
          empty_result.decision() ==
              ExactDirectSparseStableFacetDescentClosureDecision::
                  complete_empty_seed_set &&
          empty_result.required_memo_slot_count() == 0U &&
          empty_result.nodes().empty() &&
          empty_result.seed_projections().empty(),
      "an empty seed set requires no memo, probe, geometry or top-k budget");
}

[[nodiscard]] CanonicalPointCloud chain_cloud() {
  const std::array<CertifiedPoint3, 6U> input{
      point(-5.0, -6.0),
      point(-5.0, 5.0),
      point(-3.0, 3.0),
      point(3.0, 1.0),
      point(3.0, 6.0),
      point(4.0, 5.0),
  };
  return CanonicalPointCloud::rejecting_duplicates(input);
}

void test_six_point_shared_suffix_memo_collisions_and_miniball_cache() {
  const CanonicalPointCloud cloud = chain_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparseFacetKey f0 = key({0U, 1U, 2U, 4U});
  const ExactDirectSparseFacetKey f1 = key({1U, 2U, 3U, 5U});
  const ExactDirectSparseFacetKey f2 = key({1U, 2U, 3U, 4U});
  auto forest = make_bound_forest(key({0U, 1U, 2U, 3U}), f2, 0U);
  const std::array seeds{
      ExactDirectSparseStableFacetDescentClosureSeed{0U, f0},
      ExactDirectSparseStableFacetDescentClosureSeed{1U, f1},
  };
  const auto result = build_at_level(
      index,
      cloud,
      seeds,
      level(52),
      forest,
      generous_closure_budget(),
      LbvhTraversalOrder::near_first,
      ExactDirectSparseStableFacetDescentClosureConfig{0U});
  check(
      result.certified_complete_relative_positive_closure() &&
          result.required_memo_slot_count() == 31U &&
          result.nodes().size() == 3U && result.edges().size() == 2U &&
          result.counters().terminal_node_count == 1U &&
          result.counters().evaluated_step_source_count == 2U &&
          result.counters().memoized_seed_reuse_count == 1U &&
          result.counters().memoized_suffix_reuse_count >= 1U &&
          result.counters().equal_fingerprint_distinct_key_count != 0U,
      "the six-point chain shares its closed suffix under forced memo collisions");
  check(
      result.counters().source_miniball_build_count == 1U &&
          result.counters().source_miniball_reuse_count == 1U &&
          result.counters().successor_miniball_build_count == 2U &&
          result.counters().successor_miniball_reuse_count == 0U &&
          result.counters().distinct_cached_miniball_count == 3U,
      "the six-point chain builds only F0, F1 and F2 exact miniballs");
  const auto* node_f0 = find_node(result, f0);
  const auto* node_f1 = find_node(result, f1);
  const auto* node_f2 = find_node(result, f2);
  check(
      node_f0 != nullptr && node_f1 != nullptr && node_f2 != nullptr,
      "the six-point chain interns all three complete keys");
  if (node_f0 != nullptr && node_f1 != nullptr && node_f2 != nullptr) {
    check(
        node_f0->exact_squared_level == level(52) &&
            node_f1->exact_squared_level == level(85, 4) &&
            node_f2->exact_squared_level == level(325, 16) &&
            node_f0->terminal_node_index == node_f2->node_index &&
            node_f1->terminal_node_index == node_f2->node_index &&
            node_f2->stable_positive_attachment ==
                std::optional<ExactDirectSparseStableFacetPositiveAttachment>{
                    ExactDirectSparseStableFacetPositiveAttachment{
                        3U, 2U, 2U}},
        "the shared chain preserves its three exact levels and stable terminal");
  }
  check(
      result.seed_projections()[0U].terminal_node_index ==
              result.seed_projections()[1U].terminal_node_index &&
          !result.seed_projections()[0U].reused_existing_node &&
          result.seed_projections()[1U].reused_existing_node,
      "the F1 seed reuses the suffix discovered from F0");

  const std::array<CertifiedPoint3, 4U> diamond_input{
      point(-1.0, 0.0, 0.0),
      point(0.0, -1.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(1.0, 0.0, 0.0),
  };
  const CanonicalPointCloud diamond =
      CanonicalPointCloud::rejecting_duplicates(diamond_input);
  const MortonLbvhIndex diamond_index = MortonLbvhIndex::build(diamond);
  auto empty = make_empty_forest();
  const std::array plateau_seeds{
      ExactDirectSparseStableFacetDescentClosureSeed{
          0U, key({0U, 2U, 3U})},
      ExactDirectSparseStableFacetDescentClosureSeed{
          1U, key({1U, 2U, 3U})},
  };
  const auto plateau = build_at_level(
      diamond_index,
      diamond,
      plateau_seeds,
      level(1),
      empty,
      generous_closure_budget(),
      LbvhTraversalOrder::near_first,
      ExactDirectSparseStableFacetDescentClosureConfig{0U});
  check(
      plateau.certified_complete_with_unresolved_terminals() &&
          plateau.nodes().size() == 2U && plateau.edges().empty() &&
          plateau.counters().source_miniball_build_count == 2U &&
          plateau.counters().successor_miniball_build_count == 1U &&
          plateau.counters().successor_miniball_reuse_count == 1U &&
          plateau.counters().distinct_cached_miniball_count == 3U &&
          plateau.counters().equal_fingerprint_distinct_key_count != 0U &&
          std::all_of(
              plateau.nodes().begin(),
              plateau.nodes().end(),
              [](const auto& node) {
                return node.local_decision ==
                       ExactDirectSparseStableFacetDescentLocalDecision::
                           complete_unresolved_non_strict_canonical_successor;
              }),
      "two non-strict sources reuse one uninterned full-key successor miniball");

  const CanonicalPointCloud ac_cloud = ac_de_cloud();
  const MortonLbvhIndex ac_index = MortonLbvhIndex::build(ac_cloud);
  auto ac_empty = make_empty_forest();
  const std::array ac_seed{
      ExactDirectSparseStableFacetDescentClosureSeed{
          0U, key({1U, 3U})},
  };
  const auto above = build_at_level(
      ac_index,
      ac_cloud,
      ac_seed,
      level(16),
      ac_empty,
      generous_closure_budget());
  check(
      above.certified_complete_with_unresolved_terminals() &&
          above.nodes().size() == 1U && above.edges().empty() &&
          above.counters().source_miniball_build_count == 1U &&
          above.counters().top_k_query_count == 0U &&
          above.nodes().front().exact_squared_level == level(33, 2) &&
          above.nodes().front().local_decision ==
              ExactDirectSparseStableFacetDescentLocalDecision::
                  complete_unresolved_source_above_closed_batch_level,
      "AC above closed level 16 terminates before top-k without invention");
}

void test_lookup_aggregate_and_four_probe_caps() {
  const CanonicalPointCloud cloud = ac_de_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  auto forest = make_positive_forest();
  const ExactDirectSparseFacetKey ac = key({1U, 3U});
  const ExactDirectSparseFacetKey de = key({0U, 4U});
  const std::array ac_seed{
      ExactDirectSparseStableFacetDescentClosureSeed{0U, ac},
  };
  const std::array de_seed{
      ExactDirectSparseStableFacetDescentClosureSeed{0U, de},
  };

  auto one_probe_budget = generous_closure_budget();
  const std::size_t one_probe_reservation =
      probe_budget_sum(one_probe_budget.positive_probe);
  one_probe_budget.maximum_lookup_work_count = one_probe_reservation;
  const auto stopped_before_successor = build(
      index, cloud, ac_seed, forest, one_probe_budget);
  check(
      stopped_before_successor.certified_budget_exhaustion() &&
          stopped_before_successor.decision() ==
              ExactDirectSparseStableFacetDescentClosureDecision::
                  no_forest_probe_budget_exhausted &&
          stopped_before_successor.counters().stable_forest_probe_count == 1U,
      "the aggregate lookup cap stops before the unreserved successor probe");
  check_empty_graph(
      stopped_before_successor,
      "aggregate lookup exhaustion");
  const std::size_t completed_source_work =
      stopped_before_successor.counters().aggregate_lookup_work_count;
  check(
      completed_source_work != 0U,
      "the collision-chain source miss records positive lookup work");

  auto exact_second_reservation = generous_closure_budget();
  exact_second_reservation.maximum_lookup_work_count =
      completed_source_work + one_probe_reservation;
  const auto exact = build(
      index, cloud, ac_seed, forest, exact_second_reservation);
  check(
      exact.certified_complete_relative_positive_closure(),
      "the exact aggregate reservation for the second probe succeeds");
  --exact_second_reservation.maximum_lookup_work_count;
  const auto cap_minus_one = build(
      index, cloud, ac_seed, forest, exact_second_reservation);
  check(
      cap_minus_one.certified_budget_exhaustion() &&
          cap_minus_one.counters().stable_forest_probe_count == 1U &&
          cap_minus_one.counters().aggregate_lookup_work_count ==
              completed_source_work,
      "the aggregate cap minus one fails before the second probe");
  check_empty_graph(cap_minus_one, "aggregate cap minus one");

  const auto direct = forest.probe_positive_full_key(
      de, generous_probe_budget());
  check(
      direct.certified_observed() &&
          direct.positive_key_slot_visit_count != 0U &&
          direct.full_key_comparison_count != 0U &&
          direct.handle_index_slot_visit_count != 0U &&
          direct.parent_hop_count != 0U,
      "the forced-collision DE probe exercises all four work dimensions");

  using ProbeBudget = ExactDirectSparseStableFacetPositiveProbeBudget;
  using ClosureCounters =
      ExactDirectSparseStableFacetDescentClosureCounters;
  struct CapCase {
    std::size_t ProbeBudget::*budget_member;
    std::size_t ClosureCounters::*counter_member;
    std::size_t exact_value;
    std::string_view label;
  };
  const std::array cap_cases{
      CapCase{
          &ProbeBudget::maximum_positive_key_slot_visit_count,
          &ClosureCounters::positive_key_slot_visit_count,
          direct.positive_key_slot_visit_count,
          "positive-key slots"},
      CapCase{
          &ProbeBudget::maximum_full_key_comparison_count,
          &ClosureCounters::full_key_comparison_count,
          direct.full_key_comparison_count,
          "full-key comparisons"},
      CapCase{
          &ProbeBudget::maximum_handle_index_slot_visit_count,
          &ClosureCounters::handle_index_slot_visit_count,
          direct.handle_index_slot_visit_count,
          "handle-index slots"},
      CapCase{
          &ProbeBudget::maximum_parent_hop_count,
          &ClosureCounters::parent_hop_count,
          direct.parent_hop_count,
          "DSU parent hops"},
  };
  for (const auto& cap_case : cap_cases) {
    auto exact_budget = generous_closure_budget();
    exact_budget.positive_probe.*(cap_case.budget_member) =
        cap_case.exact_value;
    exact_budget.maximum_lookup_work_count =
        probe_budget_sum(exact_budget.positive_probe);
    const auto exact_cap = build(
        index, cloud, de_seed, forest, exact_budget);
    check(
        exact_cap.certified_complete_relative_positive_closure(),
        std::string{cap_case.label} + " exact cap succeeds");

    --(exact_budget.positive_probe.*(cap_case.budget_member));
    exact_budget.maximum_lookup_work_count =
        probe_budget_sum(exact_budget.positive_probe);
    const auto exhausted = build(
        index, cloud, de_seed, forest, exact_budget);
    check(
        exhausted.certified_budget_exhaustion() &&
            exhausted.decision() ==
                ExactDirectSparseStableFacetDescentClosureDecision::
                    no_forest_probe_budget_exhausted &&
            exhausted.counters().*(cap_case.counter_member) ==
                cap_case.exact_value - 1U,
        std::string{cap_case.label} +
            " cap minus one fails at its exact counter");
    check_empty_graph(exhausted, cap_case.label);
  }
}

struct SuccessorProbeCapFixture {
  ExactDirectSparseStableFacetForest forest;
  ExactDirectSparseStableFacetPositiveProbeResult source_probe;
  ExactDirectSparseStableFacetPositiveProbeResult successor_probe;
};

[[nodiscard]] std::optional<SuccessorProbeCapFixture>
find_successor_probe_cap_fixture(
    const ExactDirectSparseFacetKey& source_key,
    const ExactDirectSparseFacetKey& successor_key) {
  constexpr std::array<std::uint64_t, 10U> masks{
      std::numeric_limits<std::uint64_t>::max(),
      UINT64_C(0xff),
      UINT64_C(0x7f),
      UINT64_C(0x3f),
      UINT64_C(0x1f),
      UINT64_C(0x0f),
      UINT64_C(0x07),
      UINT64_C(0x03),
      UINT64_C(0x01),
      UINT64_C(0x00),
  };
  const std::array root_keys{
      key({0U, 1U}),
      key({0U, 2U}),
      key({0U, 3U}),
      key({1U, 2U}),
      key({1U, 4U}),
      key({2U, 3U}),
      key({2U, 4U}),
      key({3U, 4U}),
  };
  for (const std::uint64_t mask : masks) {
    for (const auto& root_key : root_keys) {
      if (root_key == source_key || root_key == successor_key) {
        continue;
      }
      auto forest = make_bound_forest(root_key, successor_key, mask);
      const auto source = forest.probe_positive_full_key(
          source_key, generous_probe_budget());
      const auto successor = forest.probe_positive_full_key(
          successor_key, generous_probe_budget());
      if (source.certified_unobserved() && successor.certified_observed() &&
          source.positive_key_slot_visit_count <
              successor.positive_key_slot_visit_count &&
          source.full_key_comparison_count <
              successor.full_key_comparison_count) {
        return SuccessorProbeCapFixture{
            std::move(forest), source, successor};
      }
    }
  }
  return std::nullopt;
}

void test_four_probe_caps_at_strict_successor_position() {
  const CanonicalPointCloud cloud = ac_de_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparseFacetKey ac = key({1U, 3U});
  const ExactDirectSparseFacetKey de = key({0U, 4U});
  auto fixture = find_successor_probe_cap_fixture(ac, de);
  check(
      fixture.has_value(),
      "an exact sparse-index layout separates source-miss and successor-hit cap boundaries");
  if (!fixture.has_value()) {
    return;
  }
  const std::array seed{
      ExactDirectSparseStableFacetDescentClosureSeed{0U, ac},
  };
  using ProbeBudget = ExactDirectSparseStableFacetPositiveProbeBudget;
  using ClosureCounters =
      ExactDirectSparseStableFacetDescentClosureCounters;
  struct CapCase {
    std::size_t ProbeBudget::*budget_member;
    std::size_t ClosureCounters::*counter_member;
    std::size_t source_value;
    std::size_t successor_value;
    std::string_view label;
  };
  const auto& source = fixture->source_probe;
  const auto& successor = fixture->successor_probe;
  const std::array cap_cases{
      CapCase{
          &ProbeBudget::maximum_positive_key_slot_visit_count,
          &ClosureCounters::positive_key_slot_visit_count,
          source.positive_key_slot_visit_count,
          successor.positive_key_slot_visit_count,
          "successor positive-key slots"},
      CapCase{
          &ProbeBudget::maximum_full_key_comparison_count,
          &ClosureCounters::full_key_comparison_count,
          source.full_key_comparison_count,
          successor.full_key_comparison_count,
          "successor full-key comparisons"},
      CapCase{
          &ProbeBudget::maximum_handle_index_slot_visit_count,
          &ClosureCounters::handle_index_slot_visit_count,
          source.handle_index_slot_visit_count,
          successor.handle_index_slot_visit_count,
          "successor handle-index slots"},
      CapCase{
          &ProbeBudget::maximum_parent_hop_count,
          &ClosureCounters::parent_hop_count,
          source.parent_hop_count,
          successor.parent_hop_count,
          "successor DSU parent hops"},
  };
  for (const auto& cap_case : cap_cases) {
    check(
        cap_case.source_value < cap_case.successor_value,
        std::string{cap_case.label} +
            " has a strict source/successor work separation");
    if (cap_case.source_value >= cap_case.successor_value) {
      continue;
    }
    auto exact_budget = generous_closure_budget();
    exact_budget.positive_probe.*(cap_case.budget_member) =
        cap_case.successor_value;
    const auto exact = build(
        index, cloud, seed, fixture->forest, exact_budget);
    check(
        exact.certified_complete_relative_positive_closure() &&
            exact.counters().stable_forest_probe_count == 2U,
        std::string{cap_case.label} + " exact cap succeeds");

    --(exact_budget.positive_probe.*(cap_case.budget_member));
    const auto exhausted = build(
        index, cloud, seed, fixture->forest, exact_budget);
    check(
        exhausted.certified_budget_exhaustion() &&
            exhausted.decision() ==
                ExactDirectSparseStableFacetDescentClosureDecision::
                    no_forest_probe_budget_exhausted &&
            exhausted.counters().stable_forest_probe_count == 2U &&
            exhausted.counters().*(cap_case.counter_member) ==
                cap_case.source_value + cap_case.successor_value - 1U,
        std::string{cap_case.label} +
            " cap minus one exhausts only at the successor probe");
    check_empty_graph(exhausted, cap_case.label);
  }
}

struct ExactAcTopKFixture {
  ExactLbvhTopKAudit audit;
  std::size_t required_cutoff_shell_entry_count{};
};

[[nodiscard]] ExactAcTopKFixture exact_ac_top_k_fixture(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& ac) {
  const auto ac_ids =
      std::span<const PointId>{ac.point_ids}.first(ac.point_count);
  const auto miniball = build_exact_facet_miniball(cloud, ac_ids);
  check(
      miniball.status ==
          ExactFacetMiniballStatus::exact_facet_miniball_certified &&
          miniball.squared_radius == level(33, 2),
      "the top-k cap fixture rebuilds the exact AC miniball");
  const ExclusionSet exclusions = ExclusionSet::from_ids(
      std::span<const PointId>{}, cloud, 0U);
  const auto query = morsehgp3d::spatial::lbvh_top_k_budgeted(
      index,
      cloud,
      miniball.center,
      ac.point_count,
      exclusions,
      ac_ids,
      std::span<const PointId>{},
      generous_top_k_budget(),
      LbvhTraversalOrder::near_first);
  check(
      query.complete() && query.audit().traversal_complete,
      "the direct AC top-k audit is complete");
  return {
      query.audit(), query.partition().cutoff_shell_ids().size()};
}

void test_all_top_k_caps_and_preflight_caps() {
  const CanonicalPointCloud cloud = ac_de_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  auto forest = make_positive_forest();
  const ExactDirectSparseFacetKey ac = key({1U, 3U});
  const std::array seed{
      ExactDirectSparseStableFacetDescentClosureSeed{0U, ac},
  };
  const ExactAcTopKFixture top_k =
      exact_ac_top_k_fixture(index, cloud, ac);
  const ExactLbvhTopKAudit& audit = top_k.audit;

  using Budget = ExactLbvhTopKBudget;
  struct TopKCapCase {
    std::size_t Budget::*member;
    std::size_t exact_value;
    std::string_view label;
  };
  const std::array cap_cases{
      TopKCapCase{
          &Budget::maximum_node_visit_count,
          audit.node_visit_count,
          "top-k node visits"},
      TopKCapCase{
          &Budget::maximum_internal_node_expansion_count,
          audit.internal_node_expansion_count,
          "top-k internal expansions"},
      TopKCapCase{
          &Budget::maximum_exact_aabb_bound_evaluation_count,
          audit.exact_aabb_bound_evaluation_count,
          "top-k exact AABB bounds"},
      TopKCapCase{
          &Budget::maximum_exact_point_distance_evaluation_count,
          audit.exact_point_distance_evaluation_count,
          "top-k exact point distances"},
      TopKCapCase{
          &Budget::maximum_frontier_entry_count,
          audit.peak_frontier_entry_count,
          "top-k frontier entries"},
      TopKCapCase{
          &Budget::maximum_best_neighbor_entry_count,
          audit.peak_best_neighbor_entry_count,
          "top-k best-neighbor entries"},
      TopKCapCase{
          &Budget::maximum_cutoff_shell_entry_count,
          top_k.required_cutoff_shell_entry_count,
          "top-k cutoff-shell entries"},
  };
  for (const auto& cap_case : cap_cases) {
    check(
        cap_case.exact_value != 0U,
        std::string{cap_case.label} + " fixture has a nonzero boundary");
    if (cap_case.exact_value == 0U) {
      continue;
    }
    auto exact_budget = generous_closure_budget();
    exact_budget.top_k_query.*(cap_case.member) = cap_case.exact_value;
    const auto exact = build(index, cloud, seed, forest, exact_budget);
    check(
        exact.certified_complete_relative_positive_closure(),
        std::string{cap_case.label} + " exact cap succeeds");

    --(exact_budget.top_k_query.*(cap_case.member));
    const auto exhausted = build(index, cloud, seed, forest, exact_budget);
    check(
        exhausted.certified_budget_exhaustion() &&
            exhausted.decision() ==
                ExactDirectSparseStableFacetDescentClosureDecision::
                    no_top_k_budget_exhausted,
        std::string{cap_case.label} + " cap minus one fails closed");
    check_empty_graph(exhausted, cap_case.label);
  }

  auto seed_cap = generous_closure_budget();
  seed_cap.maximum_seed_count = 0U;
  const auto seed_exhausted = build(index, cloud, seed, forest, seed_cap);
  check(
      seed_exhausted.certified_budget_exhaustion() &&
          seed_exhausted.decision() ==
              ExactDirectSparseStableFacetDescentClosureDecision::
                  no_preflight_budget_exhausted,
      "seed cap minus one fails in preflight");
  check_empty_graph(seed_exhausted, "seed preflight exhaustion");

  auto memo_exact = generous_closure_budget();
  memo_exact.maximum_node_count = 2U;
  memo_exact.maximum_memo_slot_count = 5U;
  check(
      build(index, cloud, seed, forest, memo_exact)
          .certified_complete_relative_positive_closure(),
      "the exact five-slot memo bound for two nodes succeeds");
  --memo_exact.maximum_memo_slot_count;
  const auto memo_exhausted = build(index, cloud, seed, forest, memo_exact);
  check(
      memo_exhausted.certified_budget_exhaustion() &&
          memo_exhausted.decision() ==
              ExactDirectSparseStableFacetDescentClosureDecision::
                  no_preflight_budget_exhausted,
      "the memo bound cap minus one fails before graph publication");
  check_empty_graph(memo_exhausted, "memo preflight exhaustion");

  auto node_cap = generous_closure_budget();
  node_cap.maximum_node_count = 1U;
  node_cap.maximum_memo_slot_count = 3U;
  const auto node_exhausted = build(index, cloud, seed, forest, node_cap);
  check(
      node_exhausted.certified_budget_exhaustion() &&
          node_exhausted.decision() ==
              ExactDirectSparseStableFacetDescentClosureDecision::
                  no_node_budget_exhausted,
      "a strict successor fails closed when the node cap is one");
  check_empty_graph(node_exhausted, "node exhaustion");

  auto step_cap = generous_closure_budget();
  step_cap.maximum_step_call_count = 0U;
  const auto step_exhausted = build(index, cloud, seed, forest, step_cap);
  check(
      step_exhausted.certified_budget_exhaustion() &&
          step_exhausted.decision() ==
              ExactDirectSparseStableFacetDescentClosureDecision::
                  no_step_call_budget_exhausted,
      "a zero step cap fails before any forest probe");
  check_empty_graph(step_exhausted, "step exhaustion");

  auto overflow = generous_closure_budget();
  overflow.positive_probe.maximum_positive_key_slot_visit_count =
      std::numeric_limits<std::size_t>::max();
  overflow.positive_probe.maximum_full_key_comparison_count = 1U;
  overflow.maximum_lookup_work_count =
      std::numeric_limits<std::size_t>::max();
  const auto overflow_exhausted = build(index, cloud, seed, forest, overflow);
  check(
      overflow_exhausted.certified_budget_exhaustion() &&
          overflow_exhausted.decision() ==
              ExactDirectSparseStableFacetDescentClosureDecision::
                  no_preflight_budget_exhausted,
      "an unrepresentable per-probe work sum fails preflight");
  check_empty_graph(overflow_exhausted, "probe-sum overflow");
}

void test_unresolved_misses_private_replay_and_stale_stamp() {
  const CanonicalPointCloud cloud = ac_de_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  auto empty_forest = make_empty_forest();
  const ExactDirectSparseFacetKey ac = key({1U, 3U});
  const std::array seed{
      ExactDirectSparseStableFacetDescentClosureSeed{0U, ac},
  };
  const auto unresolved = build(
      index, cloud, seed, empty_forest, generous_closure_budget());
  check(
      unresolved.certified_complete_with_unresolved_terminals() &&
          unresolved.counters().unresolved_terminal_count == 1U &&
          unresolved.counters().relative_positive_terminal_count == 0U &&
          unresolved.nodes().size() >= 2U,
      "a strict successor miss remains unresolved rather than inventing a birth");
  const auto unresolved_verification =
      verify_exact_direct_sparse_stable_facet_descent_closure(
          index,
          cloud,
          seed,
          level(33, 2),
          empty_forest,
          generous_closure_budget(),
          {},
          LbvhTraversalOrder::near_first,
          unresolved);
  check(
      unresolved_verification.result_certified,
      "the unresolved graph is also reproducible from trusted authorities");

  auto positive_forest = make_positive_forest();
  const auto observed = build(
      index, cloud, seed, positive_forest, generous_closure_budget());
  const auto authentic_copy = observed;
  ExactDirectSparseStableFacetDescentClosureResult default_result;
  check(
      authentic_copy.certified_for(positive_forest) &&
          !default_result.certified_outcome(),
      "an authentic immutable copy retains its mint and a default result has none");

  auto changed_budget = generous_closure_budget();
  --changed_budget.maximum_lookup_work_count;
  const auto wrong_budget_verification =
      verify_exact_direct_sparse_stable_facet_descent_closure(
          index,
          cloud,
          seed,
          level(33, 2),
          positive_forest,
          changed_budget,
          {},
          LbvhTraversalOrder::near_first,
          observed);
  const auto wrong_order_verification =
      verify_exact_direct_sparse_stable_facet_descent_closure(
          index,
          cloud,
          seed,
          level(33, 2),
          positive_forest,
          generous_closure_budget(),
          {},
          LbvhTraversalOrder::far_first,
          observed);
  check(
      !wrong_budget_verification.result_certified &&
          !wrong_order_verification.result_certified,
      "an observed result cannot steer replay under another budget or order");

  const auto old_stamp = positive_forest.current_stamp();
  const std::array later_insertion{
      ExactDirectSparseStableFacetInsertion{4U, key({1U, 2U})},
  };
  auto prepared = positive_forest.prepare_batch(later_insertion, {});
  check(
      prepared.certified_prepared() && prepared.ticket.has_value(),
      "the stale-stamp fixture prepares a later forest mutation");
  if (prepared.ticket.has_value()) {
    const auto committed = positive_forest.commit(std::move(*prepared.ticket));
    check(
        committed.certified_commit(),
        "the stale-stamp fixture commits a later forest mutation");
  }
  check(
      observed.certified_for(old_stamp) &&
          !observed.certified_for(positive_forest),
      "a genuine result certifies only for its captured forest stamp");
  const auto stale_verification =
      verify_exact_direct_sparse_stable_facet_descent_closure(
          index,
          cloud,
          seed,
          level(33, 2),
          positive_forest,
          generous_closure_budget(),
          {},
          LbvhTraversalOrder::near_first,
          observed);
  check(
      !stale_verification.result_certified &&
          !stale_verification.observed_stamp_matches_forest &&
          !stale_verification.fresh_replay_completed,
      "verification rejects a stale observed stamp before scientific replay");

  auto foreign = make_positive_forest();
  const auto foreign_verification =
      verify_exact_direct_sparse_stable_facet_descent_closure(
          index,
          cloud,
          seed,
          level(33, 2),
          foreign,
          generous_closure_budget(),
          {},
          LbvhTraversalOrder::near_first,
          observed);
  check(
      !foreign_verification.result_certified &&
          !observed.certified_for(foreign),
      "another logically similar forest session cannot reuse the result mint");
}

}  // namespace

int main() {
  test_ac_to_de_exact_and_traversal_invariance();
  test_source_hit_duplicates_and_empty_seed_set();
  test_six_point_shared_suffix_memo_collisions_and_miniball_cache();
  test_lookup_aggregate_and_four_probe_caps();
  test_four_probe_caps_at_strict_successor_position();
  test_all_top_k_caps_and_preflight_caps();
  test_unresolved_misses_private_replay_and_stale_stamp();
  if (failures != 0) {
    std::cerr << failures
              << " direct sparse stable-facet descent-closure checks failed\n";
    return 1;
  }
  std::cout
      << "direct sparse stable-facet descent-closure checks passed\n";
  return 0;
}
