#include "morsehgp3d/hierarchy/anchored_pair_witness_bank.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::hierarchy::ExactAnchoredPairCandidateStatus;
using morsehgp3d::hierarchy::ExactAnchoredPairCandidateStopReason;
using morsehgp3d::hierarchy::ExactAnchoredPairWitnessBankBudget;
using morsehgp3d::hierarchy::ExactAnchoredPairWitnessBankResult;
using morsehgp3d::hierarchy::
    build_exact_anchored_pair_witness_bank_candidates;
using morsehgp3d::hierarchy::
    exact_anchored_pair_witness_aabb_minimum_sign;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactDyadicAabb3;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] CanonicalPointCloud make_cloud(
    std::span<const std::array<double, 3>> coordinates) {
  std::vector<CertifiedPoint3> points;
  points.reserve(coordinates.size());
  for (const std::array<double, 3>& coordinate : coordinates) {
    points.push_back(CertifiedPoint3::from_binary64(
        coordinate[0], coordinate[1], coordinate[2]));
  }
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] CanonicalPointCloud make_line_cloud(std::size_t point_count) {
  std::vector<std::array<double, 3>> coordinates;
  coordinates.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    coordinates.push_back({static_cast<double>(index), 0.0, 0.0});
  }
  return make_cloud(coordinates);
}

[[nodiscard]] ExactRational diametral_phi(
    const CanonicalPointCloud& cloud,
    PointId query_point_id,
    PointId first_support_id,
    PointId second_support_id) {
  ExactRational value{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    value = value +
        (cloud.point(query_point_id).coordinate(axis) -
         cloud.point(first_support_id).coordinate(axis)) *
            (cloud.point(query_point_id).coordinate(axis) -
             cloud.point(second_support_id).coordinate(axis));
  }
  return value;
}

[[nodiscard]] std::size_t exhaustive_closed_rank(
    const CanonicalPointCloud& cloud,
    PointId first_support_id,
    PointId second_support_id) {
  std::size_t rank = 0U;
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    if (diametral_phi(
            cloud,
            static_cast<PointId>(index),
            first_support_id,
            second_support_id)
            .sign() <= 0) {
      ++rank;
    }
  }
  return rank;
}

[[nodiscard]] ExactLbvhTopKBudget complete_top_k_budget(
    std::size_t point_count,
    std::size_t rank) {
  ExactLbvhTopKBudget budget;
  const std::size_t roomy = 8U * point_count + 32U;
  budget.maximum_node_visit_count = roomy;
  budget.maximum_internal_node_expansion_count = roomy;
  budget.maximum_exact_aabb_bound_evaluation_count = roomy;
  budget.maximum_exact_point_distance_evaluation_count = roomy;
  budget.maximum_frontier_entry_count = roomy;
  budget.maximum_best_neighbor_entry_count = rank;
  budget.maximum_cutoff_shell_entry_count = point_count;
  return budget;
}

[[nodiscard]] ExactAnchoredPairWitnessBankBudget complete_budget(
    std::size_t point_count,
    std::size_t witness_bank_size) {
  ExactAnchoredPairWitnessBankBudget budget;
  budget.proposed_witness_bank_size = witness_bank_size;
  budget.witness_search_budget =
      complete_top_k_budget(point_count, witness_bank_size);
  budget.maximum_node_visit_count = 4U * point_count + 8U;
  budget.maximum_internal_node_expansion_count =
      2U * point_count + 8U;
  budget.maximum_traversal_stack_entry_count =
      2U * point_count + 8U;
  budget.maximum_witness_node_predicate_count =
      (4U * point_count + 8U) * (witness_bank_size + 1U);
  budget.maximum_candidate_entry_count = point_count;
  budget.maximum_prune_record_count = 2U * point_count + 8U;
  return budget;
}

[[nodiscard]] bool contains_id(
    const std::vector<PointId>& ids,
    PointId point_id) {
  return std::find(ids.begin(), ids.end(), point_id) != ids.end();
}

void test_exhaustive_relevant_pair_differential() {
  constexpr std::size_t point_count = 12U;
  constexpr std::size_t maximum_closed_rank = 4U;
  CanonicalPointCloud cloud = make_line_cloud(point_count);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  bool saw_prune = false;

  for (std::size_t anchor_index = 0U;
       anchor_index < point_count;
       ++anchor_index) {
    const PointId anchor_point_id = static_cast<PointId>(anchor_index);
    const ExactAnchoredPairWitnessBankResult result =
        build_exact_anchored_pair_witness_bank_candidates(
            index,
            cloud,
            anchor_point_id,
            maximum_closed_rank,
            complete_budget(point_count, point_count - 1U));
    require(result.complete(), "the exhaustive differential traversal stopped");
    require(
        result.audit.traversal_complete,
        "a complete anchored traversal lacks its completion audit");
    require(
        result.audit.candidate_entry_count ==
            result.candidate_point_ids.size(),
        "candidate payload and audit disagree");
    require(
        result.audit.witness_node_predicate_count ==
            result.audit.fp64_interval_positive_count +
                result.audit.fp64_interval_negative_count +
                result.audit.exact_witness_node_predicate_count,
        "filtered and exact predicate counters do not partition the work");
    saw_prune = saw_prune || !result.prune_records.empty();

    std::vector<unsigned char> covered(point_count, 0U);
    for (const PointId candidate_point_id : result.candidate_point_ids) {
      require(
          candidate_point_id > anchor_point_id,
          "the anchored stream emitted a reversed or diagonal pair");
      covered[static_cast<std::size_t>(candidate_point_id)] = 1U;
    }
    for (const auto& record : result.prune_records) {
      require(
          record.witness_count == maximum_closed_rank - 1U,
          "a prune record has the wrong strict witness threshold");
      for (std::size_t left = 0U; left < record.witness_count; ++left) {
        require(
            record.witness_point_ids[left] != anchor_point_id,
            "a prune record reused its anchor as a witness");
        for (std::size_t right = 0U; right < left; ++right) {
          require(
              record.witness_point_ids[left] !=
                  record.witness_point_ids[right],
              "a prune record repeated a witness");
        }
      }
      require(
          record.leaf_begin < record.leaf_end &&
              record.leaf_end <= index.leaves().size(),
          "a prune record has an invalid Morton range");
      for (std::size_t position = record.leaf_begin;
           position < record.leaf_end;
           ++position) {
        const PointId omitted_id = index.leaves()[position].point_id;
        if (omitted_id > anchor_point_id) {
          require(
              covered[static_cast<std::size_t>(omitted_id)] == 0U,
              "candidate and prune ranges overlap");
          covered[static_cast<std::size_t>(omitted_id)] = 2U;
        }
        for (std::size_t witness_index = 0U;
             witness_index < record.witness_count;
             ++witness_index) {
          require(
              diametral_phi(
                  cloud,
                  record.witness_point_ids[witness_index],
                  anchor_point_id,
                  omitted_id)
                      .sign() < 0,
              "a recorded witness is not strictly inside an omitted pair");
        }
      }
    }

    for (std::size_t other_index = anchor_index + 1U;
         other_index < point_count;
         ++other_index) {
      const PointId other_point_id = static_cast<PointId>(other_index);
      require(
          covered[other_index] != 0U,
          "the anchored stream neither emitted nor certified a pair");
      const std::size_t closed_rank = exhaustive_closed_rank(
          cloud, anchor_point_id, other_point_id);
      if (closed_rank <= maximum_closed_rank) {
        require(
            covered[other_index] == 1U,
            "an exhaustive rank-relevant pair was incorrectly pruned");
      }
      if (covered[other_index] == 2U) {
        require(
            closed_rank > maximum_closed_rank,
            "a certified omission has relevant closed rank");
      }
    }
  }
  require(saw_prune, "the differential fixture exercised no exact prune");
}

void test_strict_equality_is_fail_open() {
  const std::array<std::array<double, 3>, 3> coordinates{{
      {-1.0, 0.0, 0.0},
      {0.0, 1.0, 0.0},
      {1.0, 0.0, 0.0},
  }};
  CanonicalPointCloud cloud = make_cloud(coordinates);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  ExactAnchoredPairWitnessBankBudget budget =
      complete_budget(cloud.size(), 1U);
  const ExactAnchoredPairWitnessBankResult result =
      build_exact_anchored_pair_witness_bank_candidates(
          index, cloud, PointId{0}, 2U, budget);
  require(result.complete(), "the equality fixture stopped");
  require(
      result.witness_bank_point_ids == std::vector<PointId>{PointId{1}},
      "the equality fixture selected the wrong exact witness bank");
  require(
      contains_id(result.candidate_point_ids, PointId{2}),
      "a shell equality incorrectly pruned the diametral pair");

  const std::array<std::uint64_t, 3> query_bits =
      cloud.point(PointId{2}).canonical_input_bits();
  const ExactDyadicAabb3 singleton_bounds{query_bits, query_bits};
  require(
      exact_anchored_pair_witness_aabb_minimum_sign(
          cloud, PointId{0}, PointId{1}, singleton_bounds) == 0,
      "the exact AABB predicate did not preserve equality");
}

void test_incomplete_and_small_banks_are_fail_open() {
  constexpr std::size_t point_count = 8U;
  CanonicalPointCloud cloud = make_line_cloud(point_count);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);

  ExactAnchoredPairWitnessBankBudget exhausted =
      complete_budget(point_count, point_count - 1U);
  exhausted.witness_search_budget = ExactLbvhTopKBudget{};
  const ExactAnchoredPairWitnessBankResult exhausted_result =
      build_exact_anchored_pair_witness_bank_candidates(
          index, cloud, PointId{0}, 4U, exhausted);
  require(exhausted_result.complete(), "bank failure stopped pair traversal");
  require(
      exhausted_result.audit.witness_search_attempted &&
          !exhausted_result.audit.witness_search_complete &&
          exhausted_result.witness_bank_point_ids.empty(),
      "an exhausted top-k search published a witness bank");
  require(
      exhausted_result.candidate_point_ids.size() == point_count - 1U &&
          exhausted_result.prune_records.empty(),
      "an incomplete witness bank did not fail open");

  ExactAnchoredPairWitnessBankBudget too_small =
      complete_budget(point_count, 2U);
  const ExactAnchoredPairWitnessBankResult too_small_result =
      build_exact_anchored_pair_witness_bank_candidates(
          index, cloud, PointId{0}, 4U, too_small);
  require(too_small_result.complete(), "the small-bank traversal stopped");
  require(
      too_small_result.audit.witness_search_complete &&
          !too_small_result.audit.witness_bank_sufficient_for_rank_pruning,
      "the small bank was incorrectly marked rank-sufficient");
  require(
      too_small_result.candidate_point_ids.size() == point_count - 1U &&
          too_small_result.prune_records.empty(),
      "a bank below the witness threshold pruned a pair");
}

void test_k10_requires_ten_strict_witnesses() {
  constexpr std::size_t point_count = 12U;
  CanonicalPointCloud cloud = make_line_cloud(point_count);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);

  const ExactAnchoredPairWitnessBankResult nine_witness_result =
      build_exact_anchored_pair_witness_bank_candidates(
          index,
          cloud,
          PointId{0},
          11U,
          complete_budget(point_count, 9U));
  require(nine_witness_result.complete(), "the K=10 nine-bank run stopped");
  require(
      !nine_witness_result.audit.witness_bank_sufficient_for_rank_pruning &&
          nine_witness_result.candidate_point_ids.size() == point_count - 1U,
      "nine witnesses incorrectly pruned a K=10 pair");

  const ExactAnchoredPairWitnessBankResult ten_witness_result =
      build_exact_anchored_pair_witness_bank_candidates(
          index,
          cloud,
          PointId{0},
          11U,
          complete_budget(point_count, 10U));
  require(ten_witness_result.complete(), "the K=10 ten-bank run stopped");
  require(
      ten_witness_result.audit.witness_bank_sufficient_for_rank_pruning,
      "ten witnesses were not recognized at the K=10 threshold");
  require(
      contains_id(ten_witness_result.candidate_point_ids, PointId{10}) &&
          !contains_id(ten_witness_result.candidate_point_ids, PointId{11}),
      "the K=10 threshold did not separate closed ranks eleven and twelve");
  require(
      exhaustive_closed_rank(cloud, PointId{0}, PointId{10}) == 11U &&
          exhaustive_closed_rank(cloud, PointId{0}, PointId{11}) == 12U,
      "the K=10 line fixture has the wrong exhaustive closed ranks");
  require(
      std::any_of(
          ten_witness_result.prune_records.begin(),
          ten_witness_result.prune_records.end(),
          [](const auto& record) { return record.witness_count == 10U; }),
      "the K=10 prune did not retain its ten strict witnesses");
}

void test_bounded_outputs_and_fail_open_prune_records() {
  constexpr std::size_t point_count = 8U;
  CanonicalPointCloud cloud = make_line_cloud(point_count);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);

  ExactAnchoredPairWitnessBankBudget no_records =
      complete_budget(point_count, point_count - 1U);
  no_records.maximum_prune_record_count = 0U;
  const ExactAnchoredPairWitnessBankResult no_record_result =
      build_exact_anchored_pair_witness_bank_candidates(
          index, cloud, PointId{0}, 4U, no_records);
  require(no_record_result.complete(), "zero prune-record capacity stopped");
  require(
      no_record_result.audit.prune_record_capacity_fail_open_count > 0U &&
          no_record_result.prune_records.empty() &&
          no_record_result.candidate_point_ids.size() == point_count - 1U,
      "zero prune-record capacity did not descend fail-open");

  ExactAnchoredPairWitnessBankBudget candidate_limited =
      complete_budget(point_count, 0U);
  candidate_limited.maximum_candidate_entry_count = 2U;
  const ExactAnchoredPairWitnessBankResult candidate_limited_result =
      build_exact_anchored_pair_witness_bank_candidates(
          index, cloud, PointId{0}, 4U, candidate_limited);
  require(
      candidate_limited_result.status ==
              ExactAnchoredPairCandidateStatus::budget_exhausted &&
          candidate_limited_result.stop_reason ==
              ExactAnchoredPairCandidateStopReason::candidate_entry_limit &&
          candidate_limited_result.candidate_point_ids.size() == 2U,
      "the candidate cap was not enforced exactly");

  ExactAnchoredPairWitnessBankBudget stack_limited =
      complete_budget(point_count, 0U);
  stack_limited.maximum_traversal_stack_entry_count = 1U;
  const ExactAnchoredPairWitnessBankResult stack_limited_result =
      build_exact_anchored_pair_witness_bank_candidates(
          index, cloud, PointId{0}, 4U, stack_limited);
  require(
      stack_limited_result.status ==
              ExactAnchoredPairCandidateStatus::budget_exhausted &&
          stack_limited_result.stop_reason ==
              ExactAnchoredPairCandidateStopReason::
                  traversal_stack_entry_limit &&
          stack_limited_result.candidate_point_ids.empty(),
      "the traversal-stack cap was not enforced before expansion");
}

}  // namespace

int main() {
  try {
    test_exhaustive_relevant_pair_differential();
    test_strict_equality_is_fail_open();
    test_incomplete_and_small_banks_are_fail_open();
    test_k10_requires_ten_strict_witnesses();
    test_bounded_outputs_and_fail_open_prune_records();
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  return 0;
}
