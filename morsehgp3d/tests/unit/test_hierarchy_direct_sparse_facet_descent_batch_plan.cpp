#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_batch_plan.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <utility>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
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
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
  };
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
      maximum,
  };
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget source_seed_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

struct Scenario {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedJournalResult seed_journal;
};

[[nodiscard]] Scenario make_scenario(
    CanonicalPointCloud cloud,
    std::size_t maximum_order) {
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSupportTerminalBudget terminal_budget{
      source_pair_budget(), source_higher_budget()};
  const auto pair = build_exact_pair_support_stream(
      index, cloud, maximum_order, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, maximum_order, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, maximum_order, terminal_budget, pair, higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  auto seed_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, source_seed_budget());
  return {
      std::move(cloud),
      std::move(index),
      std::move(facade),
      std::move(event_journal),
      std::move(seed_journal),
  };
}

[[nodiscard]] Scenario regular_tetrahedron_scenario() {
  const std::array<CertifiedPoint3, 4U> points{
      point(1.0, 1.0, 1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, 1.0, -1.0),
      point(1.0, -1.0, -1.0),
  };
  return make_scenario(canonical_cloud(points), 3U);
}

[[nodiscard]] Scenario singleton_scenario() {
  const std::array<CertifiedPoint3, 1U> points{
      point(0.0, 0.0, 0.0),
  };
  return make_scenario(canonical_cloud(points), 1U);
}

[[nodiscard]] Scenario mixed_support_same_batch_scenario() {
  // Two radius-five clusters: one diameter support with one strict interior
  // point and one three-point positive support.  Both are order-two saddles
  // at the exact squared level 25, while their closed balls stay disjoint.
  const std::array<CertifiedPoint3, 6U> points{
      point(-5.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(5.0, 0.0, 0.0),
      point(97.0, -4.0, 0.0),
      point(97.0, 4.0, 0.0),
      point(105.0, 0.0, 0.0),
  };
  return make_scenario(canonical_cloud(points), 5U);
}

[[nodiscard]] Scenario collinear_k10_scenario() {
  const std::array<CertifiedPoint3, 11U> points{
      point(-5.0, 0.0, 0.0),
      point(-4.0, 0.0, 0.0),
      point(-3.0, 0.0, 0.0),
      point(-2.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(3.0, 0.0, 0.0),
      point(4.0, 0.0, 0.0),
      point(5.0, 0.0, 0.0),
  };
  return make_scenario(canonical_cloud(points), 10U);
}

[[nodiscard]] ExactDirectMorseIndustrialPlanConfig plan_config(
    ExactDirectMorseIndustrialPolicy policy,
    std::uint64_t maximum_batch_count) {
  ExactDirectMorseIndustrialPlanConfig config;
  config.policy = policy;
  config.memory_model = {
      64U,
      16U,
      16U,
      8U,
      16U,
      16U,
      16U,
      4U,
      16U,
      8U,
      16U,
      2U,
  };
  config.chunk_budget = {
      1'000'000U,
      maximum_batch_count,
      4096U,
      4096U,
      4096U,
      4096U,
  };
  return config;
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchPlanBudget
batch_plan_budget() {
  return {
      16U,
      64U,
      11U,
      28U,
      3U,
      7U,
      304U,
      5U,
  };
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchPlanBudget
generous_batch_plan_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      8U,
  };
}

void test_regular_tetrahedron_has_three_exact_structural_lanes() {
  const Scenario scenario = regular_tetrahedron_scenario();
  const auto config = plan_config(
      ExactDirectMorseIndustrialPolicy::interactive_resident_50k,
      64U);
  const auto budget = batch_plan_budget();
  const auto result =
      build_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          source_seed_budget(),
          scenario.seed_journal,
          config,
          budget);

  check(
      result.complete_architecture_plan() &&
          result.lanes.size() == 3U &&
          result.standalone_shape_check_only &&
          result.fresh_reconstruction_required_before_execution &&
          result.required_initial_seed_launch_count == 7U &&
          result
                  .required_initial_seed_standalone_step_support_examination_count ==
              304U,
      "the regular tetrahedron produces the exact support-2, support-3 and support-4 architecture lanes");

  const std::array<std::size_t, 3U> expected_families{6U, 4U, 1U};
  const std::array<std::size_t, 3U> expected_arms{12U, 12U, 4U};
  const std::array<std::size_t, 3U> expected_launches{3U, 3U, 1U};
  const std::array<std::size_t, 3U> expected_examinations{
      48U, 144U, 112U};
  for (std::size_t lane_index = 0U;
       lane_index < result.lanes.size();
       ++lane_index) {
    const auto& lane = result.lanes[lane_index];
    check(
        lane.lane_index == lane_index &&
            lane.source_chunk_index == 0U &&
            lane.facet_cardinality == lane_index + 1U &&
            lane.source_support_cardinality == lane_index + 2U &&
            lane.source_interior_cardinality == 0U &&
            lane.matching_family_count ==
                expected_families[lane_index] &&
            lane.matching_arm_seed_count == expected_arms[lane_index] &&
            lane.initial_seed_work_item_count ==
                expected_arms[lane_index] &&
            lane.initial_seed_launch_count ==
                expected_launches[lane_index] &&
            lane
                    .initial_seed_standalone_step_support_examination_count_upper_bound ==
                expected_examinations[lane_index],
        "each regular-tetrahedron lane keeps its exact structural class and bounded initial tiling");
  }

  const auto verification =
      verify_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          source_seed_budget(),
          scenario.seed_journal,
          config,
          budget,
          result);
  check(
      verification.result_certified &&
          verification.exact_result_equality_certified &&
          verification
              .no_geometric_difficulty_or_gpu_qualification_claimed,
      "the architecture-only lane plan is freshly reconstructed without a performance or scientific promotion");

  auto falsified = result;
  ++falsified.lanes[1U].matching_family_count;
  const auto rejected =
      verify_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          source_seed_budget(),
          scenario.seed_journal,
          config,
          budget,
          falsified);
  check(
      rejected.structural_lanes_freshly_rebuilt &&
          !rejected.exact_result_equality_certified &&
          !rejected.result_certified,
      "one falsified lane count is rejected by the fresh reconstruction");
}

void test_no_saddle_produces_a_complete_empty_lane_plan() {
  const Scenario scenario = singleton_scenario();
  const auto config = plan_config(
      ExactDirectMorseIndustrialPolicy::interactive_resident_50k,
      64U);
  const auto budget = batch_plan_budget();
  const auto result =
      build_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          source_seed_budget(),
          scenario.seed_journal,
          config,
          budget);
  check(
      result.complete_architecture_plan() &&
          result.required_lane_count == 0U &&
          result.required_initial_seed_launch_count == 0U &&
          result
                  .required_initial_seed_standalone_step_support_examination_count ==
              0U &&
          result.lanes.empty(),
      "a source with no saddle produces a complete empty architecture plan");
}

void test_mixed_support_lanes_share_one_exact_batch_barrier() {
  const Scenario scenario = mixed_support_same_batch_scenario();
  const auto config = plan_config(
      ExactDirectMorseIndustrialPolicy::interactive_resident_50k,
      256U);
  const auto budget = generous_batch_plan_budget();
  const auto result =
      build_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          source_seed_budget(),
          scenario.seed_journal,
          config,
          budget);

  const ExactDirectSparseFacetDescentBatchLane* support_two = nullptr;
  const ExactDirectSparseFacetDescentBatchLane* support_three = nullptr;
  for (const auto& lane : result.lanes) {
    if (lane.source_batch_index >=
        scenario.event_journal.batches.size()) {
      continue;
    }
    const auto& batch =
        scenario.event_journal.batches[lane.source_batch_index];
    if (batch.order != 2U ||
        batch.squared_level != ExactLevel{BigInt{25}, BigInt{1}}) {
      continue;
    }
    if (lane.source_support_cardinality == 2U) {
      support_two = &lane;
    } else if (lane.source_support_cardinality == 3U) {
      support_three = &lane;
    }
  }
  check(
      result.complete_architecture_plan() &&
          support_two != nullptr && support_three != nullptr &&
          support_two->source_batch_index ==
              support_three->source_batch_index &&
          support_two->source_chunk_index ==
              support_three->source_chunk_index &&
          support_two->facet_cardinality == 2U &&
          support_three->facet_cardinality == 2U &&
          support_two->source_interior_cardinality == 1U &&
          support_three->source_interior_cardinality == 0U &&
          support_two->matching_family_count == 1U &&
          support_three->matching_family_count == 1U &&
          support_two->matching_arm_seed_count == 2U &&
          support_three->matching_arm_seed_count == 3U &&
          result.common_frozen_pre_batch_locator_snapshot_required &&
          result.one_shared_closure_and_memo_required_per_exact_batch,
      "support-two and support-three lanes at level 25 remain behind one exact-batch snapshot and shared-closure barrier");
}

void test_caps_fail_before_lane_publication() {
  const Scenario scenario = regular_tetrahedron_scenario();
  const auto config = plan_config(
      ExactDirectMorseIndustrialPolicy::interactive_resident_50k,
      64U);

  auto lane_short = batch_plan_budget();
  --lane_short.maximum_lane_count;
  const auto lane_failure =
      build_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          source_seed_budget(),
          scenario.seed_journal,
          config,
          lane_short);
  check(
      lane_failure.decision ==
              ExactDirectSparseFacetDescentBatchPlanDecision::
                  no_plan_budget_exhausted &&
          lane_failure.required_lane_count == 3U &&
          lane_failure.lanes.empty() &&
          !lane_failure.budget_preflight_satisfied,
      "a lane cap one short publishes requirements but no partial lane");

  auto launch_short = batch_plan_budget();
  --launch_short.maximum_initial_seed_launch_count;
  const auto launch_failure =
      build_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          source_seed_budget(),
          scenario.seed_journal,
          config,
          launch_short);
  check(
      launch_failure.decision ==
              ExactDirectSparseFacetDescentBatchPlanDecision::
                  no_plan_budget_exhausted &&
          launch_failure.required_initial_seed_launch_count == 7U &&
          launch_failure.lanes.empty(),
      "a launch cap one short fails before lane allocation");

  auto examination_short = batch_plan_budget();
  --examination_short
        .maximum_initial_seed_standalone_step_support_examination_count;
  const auto examination_failure =
      build_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          source_seed_budget(),
          scenario.seed_journal,
          config,
          examination_short);
  check(
      examination_failure.decision ==
              ExactDirectSparseFacetDescentBatchPlanDecision::
                  no_plan_budget_exhausted &&
          examination_failure
                  .required_initial_seed_standalone_step_support_examination_count ==
              304U &&
          examination_failure.lanes.empty(),
      "a standalone-step examination cap one short fails atomically");

  auto zero_tile = batch_plan_budget();
  zero_tile.maximum_initial_seed_work_item_count_per_launch = 0U;
  const auto invalid =
      build_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          source_seed_budget(),
          scenario.seed_journal,
          config,
          zero_tile);
  check(
      invalid.decision ==
              ExactDirectSparseFacetDescentBatchPlanDecision::
                  no_plan_invalid_budget &&
          !invalid.source_industrial_plan_freshly_built &&
          invalid.lanes.empty(),
      "a zero work-item tile is rejected before the source plan is built");
}

void test_streaming_lanes_stay_inside_indivisible_source_batches() {
  const Scenario scenario = regular_tetrahedron_scenario();
  const auto config = plan_config(
      ExactDirectMorseIndustrialPolicy::massive_external_streaming,
      2U);
  const auto budget = batch_plan_budget();
  const auto result =
      build_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          source_seed_budget(),
          scenario.seed_journal,
          config,
          budget);

  bool every_lane_is_inside_its_chunk = true;
  for (const auto& lane : result.lanes) {
    const auto& chunk =
        result.source_industrial_plan.chunks[lane.source_chunk_index];
    every_lane_is_inside_its_chunk =
        every_lane_is_inside_its_chunk &&
        lane.source_batch_index >= chunk.source_batch_begin_index &&
        lane.source_batch_index < chunk.source_batch_end_index;
  }
  check(
      result.complete_architecture_plan() &&
          result.source_industrial_plan.chunks.size() > 1U &&
          result.lanes.size() == 3U &&
          every_lane_is_inside_its_chunk &&
          result.lanes_never_cross_chunk_or_exact_batch &&
          result.common_frozen_pre_batch_locator_snapshot_required &&
          result.one_shared_closure_and_memo_required_per_exact_batch &&
          !result.complete_shared_closure_work_bounded,
      "streaming changes only chunk ownership while every lane preserves its exact-batch closure barrier");

  auto one_chunk_only = budget;
  one_chunk_only.maximum_source_chunk_count = 1U;
  const auto rejected =
      build_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          source_seed_budget(),
          scenario.seed_journal,
          config,
          one_chunk_only);
  check(
      rejected.decision ==
              ExactDirectSparseFacetDescentBatchPlanDecision::
                  no_plan_budget_exhausted &&
          rejected.source_industrial_plan.decision ==
              ExactDirectMorseIndustrialPlanDecision::
                  no_plan_chunk_count_budget_exhausted &&
          rejected.source_industrial_plan.chunks.empty() &&
          rejected.lanes.empty(),
      "a one-chunk cap rejects the streaming source plan before retaining any partial chunk or lane");
}

void test_order_ten_pair_support_lane_has_a_finite_initial_bound() {
  const Scenario scenario = collinear_k10_scenario();
  const auto config = plan_config(
      ExactDirectMorseIndustrialPolicy::interactive_resident_50k,
      256U);
  const auto budget = generous_batch_plan_budget();
  const auto result =
      build_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          source_seed_budget(),
          scenario.seed_journal,
          config,
          budget);
  const auto lane = std::find_if(
      result.lanes.begin(),
      result.lanes.end(),
      [](const ExactDirectSparseFacetDescentBatchLane& candidate) {
        return candidate.facet_cardinality == 10U &&
               candidate.source_support_cardinality == 2U;
      });
  check(
      result.complete_architecture_plan() &&
          lane != result.lanes.end() &&
          lane->source_interior_cardinality == 9U &&
          lane->matching_family_count == 1U &&
          lane->matching_arm_seed_count == 2U &&
          lane->initial_seed_launch_count == 1U &&
          lane
                  ->initial_seed_standalone_step_support_examination_count_upper_bound ==
              3080U,
      "the K=10 boundary keeps one support-two lane with two arms and a finite 4x385-per-arm initial bound");

  const auto verification =
      verify_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          source_seed_budget(),
          scenario.seed_journal,
          config,
          budget,
          result);
  check(
      verification.result_certified,
      "the K=10 structural lane is accepted only after fresh reconstruction");
}

}  // namespace

int main() {
  test_no_saddle_produces_a_complete_empty_lane_plan();
  test_mixed_support_lanes_share_one_exact_batch_barrier();
  test_regular_tetrahedron_has_three_exact_structural_lanes();
  test_caps_fail_before_lane_publication();
  test_streaming_lanes_stay_inside_indivisible_source_batches();
  test_order_ten_pair_support_lane_has_a_finite_initial_bound();

  if (failures != 0) {
    std::cerr << failures
              << " direct sparse facet descent batch-plan test(s) failed\n";
    return 1;
  }
  return 0;
}
