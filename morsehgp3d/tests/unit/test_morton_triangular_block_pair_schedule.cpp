#include "morsehgp3d/hierarchy/morton_triangular_block_pair_schedule.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::ExactMortonTriangularBlockPairDecision;
using morsehgp3d::hierarchy::ExactMortonTriangularBlockPairResponseKind;
using morsehgp3d::hierarchy::ExactMortonTriangularBlockPairScheduleAudit;
using morsehgp3d::hierarchy::ExactMortonTriangularBlockPairScheduleBudget;
using morsehgp3d::hierarchy::ExactMortonTriangularBlockPairScheduleConfig;
using morsehgp3d::hierarchy::ExactMortonTriangularBlockPairScheduleContext;
using morsehgp3d::hierarchy::ExactMortonTriangularBlockPairScheduleStepKind;
using morsehgp3d::hierarchy::exact_morton_triangular_block_pair_maximum_pending_count;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] CanonicalPointCloud make_permuted_morton_cloud() {
  const std::array<std::array<double, 3>, 8> coordinates{{
      {0.0, 4.0, 2.0},
      {1.0, 2.0, 3.0},
      {1.0, 3.0, 1.0},
      {4.0, 3.0, 7.0},
      {5.0, 4.0, 6.0},
      {5.0, 6.0, 7.0},
      {7.0, 0.0, 1.0},
      {7.0, 4.0, 5.0},
  }};
  std::vector<CertifiedPoint3> points;
  points.reserve(coordinates.size());
  for (const auto& coordinate : coordinates) {
    points.push_back(CertifiedPoint3::from_binary64(
        coordinate[0], coordinate[1], coordinate[2]));
  }
  return CanonicalPointCloud::rejecting_duplicates(points);
}

using UnorderedPair = std::array<PointId, 2>;

struct SemanticRecord {
  std::size_t kind{};
  std::size_t anchor_begin{};
  std::size_t anchor_end{};
  std::size_t query_begin{};
  std::size_t query_end{};
  PointId point_id{};
  std::size_t pair_count{};

  friend bool operator==(const SemanticRecord&, const SemanticRecord&) =
      default;
};

struct Run {
  std::vector<SemanticRecord> records;
  std::set<PointId> self_point_ids;
  std::set<UnorderedPair> unordered_pairs;
  ExactMortonTriangularBlockPairScheduleAudit audit;
};

void insert_range_pairs(
    Run& run,
    const MortonLbvhIndex& index,
    std::size_t anchor_begin,
    std::size_t anchor_end,
    std::size_t query_begin,
    std::size_t query_end) {
  for (std::size_t anchor = anchor_begin; anchor < anchor_end; ++anchor) {
    for (std::size_t query = query_begin; query < query_end; ++query) {
      const PointId first = index.leaves()[anchor].point_id;
      const PointId second = index.leaves()[query].point_id;
      require(first != second, "a cross block contains a diagonal pair");
      const UnorderedPair pair{
          std::min(first, second), std::max(first, second)};
      require(
          run.unordered_pairs.insert(pair).second,
          "the triangular partition emitted a duplicate pair");
    }
  }
}

[[nodiscard]] Run run_schedule(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t visit_budget) {
  auto schedule = ExactMortonTriangularBlockPairScheduleContext::start(
      index, cloud, ExactMortonTriangularBlockPairScheduleConfig{2U});
  Run run;
  std::size_t call_count = 0U;
  while (!schedule.complete()) {
    require(call_count < 1024U, "the triangular cursor did not progress");
    ++call_count;
    const auto step = schedule.advance(
        index,
        cloud,
        ExactMortonTriangularBlockPairScheduleBudget{visit_budget});
    if (step.kind() ==
        ExactMortonTriangularBlockPairScheduleStepKind::budget_exhausted) {
      require(
          step.work().block_pair_visit_count == visit_budget,
          "a segmented resume did not charge its exact visit budget");
      continue;
    }
    if (step.kind() ==
        ExactMortonTriangularBlockPairScheduleStepKind::diagonal_self) {
      require(
          step.self_point_id().has_value() &&
              step.anchor_leaf_begin().has_value() &&
              step.anchor_leaf_end().has_value() &&
              *step.anchor_leaf_end() == *step.anchor_leaf_begin() + 1U,
          "a diagonal self step lost its singleton authority");
      require(
          run.self_point_ids.insert(*step.self_point_id()).second,
          "the triangular partition emitted a duplicate self point");
      run.records.push_back(SemanticRecord{
          0U,
          *step.anchor_leaf_begin(),
          *step.anchor_leaf_end(),
          0U,
          0U,
          *step.self_point_id(),
          0U});
      continue;
    }
    require(
        step.kind() ==
            ExactMortonTriangularBlockPairScheduleStepKind::cross_block &&
            step.anchor_leaf_begin().has_value() &&
            step.anchor_leaf_end().has_value() &&
            step.query_leaf_begin().has_value() &&
            step.query_leaf_end().has_value() &&
            *step.anchor_leaf_end() <= *step.query_leaf_begin() &&
            step.anchor_point_ids().size() <= 2U &&
            std::is_sorted(
                step.anchor_point_ids().begin(),
                step.anchor_point_ids().end()),
        "a cross step lost its bounded ordered native authority");

    const std::size_t anchor_count =
        *step.anchor_leaf_end() - *step.anchor_leaf_begin();
    const ExactMortonTriangularBlockPairDecision decision =
        anchor_count == 2U && *step.anchor_leaf_begin() % 4U == 0U
        ? ExactMortonTriangularBlockPairDecision::certified
        : ExactMortonTriangularBlockPairDecision::inconclusive;
    const auto response =
        schedule.respond_to_cross_block(index, cloud, decision);
    const std::size_t response_kind =
        response.kind() ==
            ExactMortonTriangularBlockPairResponseKind::certified_closed
        ? 1U
        : (response.kind() ==
                   ExactMortonTriangularBlockPairResponseKind::anchor_split
               ? 2U
               : 3U);
    run.records.push_back(SemanticRecord{
        response_kind,
        response.anchor_leaf_begin(),
        response.anchor_leaf_end(),
        response.query_leaf_begin(),
        response.query_leaf_end(),
        response.terminal_anchor_point_id().value_or(PointId{}),
        response.certified_unordered_pair_count() +
            response.opened_unordered_pair_count()});
    if (response.kind() !=
        ExactMortonTriangularBlockPairResponseKind::anchor_split) {
      insert_range_pairs(
          run,
          index,
          response.anchor_leaf_begin(),
          response.anchor_leaf_end(),
          response.query_leaf_begin(),
          response.query_leaf_end());
    }
  }
  const auto complete_step = schedule.advance(
      index, cloud, ExactMortonTriangularBlockPairScheduleBudget{0U});
  require(
      complete_step.kind() ==
          ExactMortonTriangularBlockPairScheduleStepKind::complete &&
          complete_step.schedule_complete_after_step(),
      "a completed triangular cursor did not remain complete");
  run.audit = schedule.audit();
  return run;
}

void test_permuted_eight_point_partition_and_resume() {
  const CanonicalPointCloud cloud = make_permuted_morton_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 8> expected_morton_order{
      1U, 0U, 2U, 6U, 3U, 4U, 5U, 7U};
  require(index.leaves().size() == expected_morton_order.size(),
          "the fixture LBVH has the wrong size");
  for (std::size_t leaf = 0U; leaf < expected_morton_order.size(); ++leaf) {
    require(
        index.leaves()[leaf].point_id == expected_morton_order[leaf],
        "the fixture Morton order differs at leaf " +
            std::to_string(leaf) + ": observed " +
            std::to_string(index.leaves()[leaf].point_id));
  }

  const Run roomy = run_schedule(index, cloud, 1024U);
  const Run segmented = run_schedule(index, cloud, 1U);
  require(roomy.records == segmented.records,
          "unit budgets changed the triangular semantic schedule");
  require(roomy.self_point_ids == segmented.self_point_ids &&
              roomy.unordered_pairs == segmented.unordered_pairs,
          "unit budgets changed the triangular partition");
  require(roomy.self_point_ids.size() == 8U &&
              roomy.unordered_pairs.size() == 28U,
          "the fixture did not cover its full diagonal and C(8,2)");
  for (PointId first = 0U; first < 8U; ++first) {
    for (PointId second = first + 1U; second < 8U; ++second) {
      require(
          roomy.unordered_pairs.contains({first, second}),
          "the fixture omitted an unordered point pair");
    }
  }

  const auto& audit = roomy.audit;
  require(audit.complete && audit.triangular_partition_complete &&
              audit.emitted_diagonal_self_count == 8U &&
              audit.certified_unordered_pair_count +
                      audit.opened_unordered_pair_count ==
                  28U &&
              audit.certified_cross_block_count > 0U &&
              audit.consumer_anchor_split_count > 0U &&
              audit.opened_singleton_cross_block_count > 0U &&
              audit.maximum_pending_block_pair_count <=
                  exact_morton_triangular_block_pair_maximum_pending_count &&
              audit.no_dynamic_dual_tree_or_pair_arena_materialized,
          "the roomy triangular audit lost an exact bounded invariant");
  require(
      roomy.audit.block_pair_visit_count ==
              segmented.audit.block_pair_visit_count &&
          roomy.audit.diagonal_split_count ==
              segmented.audit.diagonal_split_count &&
          roomy.audit.oversized_anchor_split_count ==
              segmented.audit.oversized_anchor_split_count &&
          roomy.audit.certified_unordered_pair_count ==
              segmented.audit.certified_unordered_pair_count &&
          roomy.audit.opened_unordered_pair_count ==
              segmented.audit.opened_unordered_pair_count &&
          roomy.audit.budget_exhaustion_count == 0U &&
          segmented.audit.budget_exhaustion_count > 0U,
      "segmentation changed cumulative work or failed to expose exhaustion");
}

}  // namespace

int main() {
  try {
    test_permuted_eight_point_partition_and_resume();
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  return 0;
}
