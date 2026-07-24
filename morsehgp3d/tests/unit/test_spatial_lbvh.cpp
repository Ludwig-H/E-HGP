#include "morsehgp3d/spatial/brute_force.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ClosedBallPartition;
using morsehgp3d::spatial::ExactNeighbor;
using morsehgp3d::spatial::ExactBudgetedLbvhTopKResult;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::ExactLbvhTopKAudit;
using morsehgp3d::spatial::ExactLbvhTopKStatus;
using morsehgp3d::spatial::ExactLbvhTopKStopReason;
using morsehgp3d::spatial::ExclusionSet;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::MortonLbvhSnapshot;
using morsehgp3d::spatial::MortonLbvhSnapshotNode;
using morsehgp3d::spatial::MortonLeafRecord;
using morsehgp3d::spatial::PointId;
using morsehgp3d::spatial::SpatialQueryMethod;
using morsehgp3d::spatial::TopKPartition;
using morsehgp3d::spatial::brute_force_closed_ball;
using morsehgp3d::spatial::brute_force_nearest;
using morsehgp3d::spatial::brute_force_top_k;
using morsehgp3d::spatial::lbvh_closed_ball;
using morsehgp3d::spatial::lbvh_nearest;
using morsehgp3d::spatial::lbvh_top_k;
using morsehgp3d::spatial::lbvh_top_k_budgeted;

static_assert(!std::is_copy_constructible_v<MortonLbvhIndex>);
static_assert(!std::is_copy_assignable_v<MortonLbvhIndex>);
static_assert(std::is_nothrow_move_constructible_v<MortonLbvhIndex>);
static_assert(std::is_nothrow_move_assignable_v<MortonLbvhIndex>);
static_assert(
    std::is_nothrow_move_constructible_v<ExactBudgetedLbvhTopKResult>);
static_assert(
    std::is_nothrow_move_assignable_v<ExactBudgetedLbvhTopKResult>);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Exception, typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    function();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message << " (unexpected exception: "
              << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

template <typename Value>
void self_move_assign(Value& value) {
  Value* alias = &value;
  value = std::move(*alias);
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactRational3 origin() {
  return ExactRational3{};
}

[[nodiscard]] CanonicalPointCloud canonical_cloud(
    std::span<const CertifiedPoint3> points) {
  return CanonicalPointCloud::rejecting_duplicates(points);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return canonical_cloud(std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::vector<CertifiedPoint3>& points) {
  return canonical_cloud(std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExclusionSet empty_exclusions(
    const CanonicalPointCloud& cloud) {
  const std::array<PointId, 0> ids{};
  return ExclusionSet::from_ids(
      std::span<const PointId>{ids}, cloud, 0U);
}

template <typename Range>
[[nodiscard]] std::vector<PointId> materialize_ids(const Range& range) {
  return std::vector<PointId>{range.begin(), range.end()};
}

[[nodiscard]] std::vector<MortonLeafRecord> materialize_leaves(
    const MortonLbvhIndex& index) {
  return std::vector<MortonLeafRecord>{
      index.leaves().begin(), index.leaves().end()};
}

[[nodiscard]] PointId snapshot_lower_witness(
    const CanonicalPointCloud& cloud,
    PointId left,
    PointId right,
    std::size_t axis) {
  const auto left_coordinate =
      cloud.point(left).exact().coordinate(axis);
  const auto right_coordinate =
      cloud.point(right).exact().coordinate(axis);
  if (right_coordinate < left_coordinate ||
      (right_coordinate == left_coordinate && right < left)) {
    return right;
  }
  return left;
}

[[nodiscard]] PointId snapshot_upper_witness(
    const CanonicalPointCloud& cloud,
    PointId left,
    PointId right,
    std::size_t axis) {
  const auto left_coordinate =
      cloud.point(left).exact().coordinate(axis);
  const auto right_coordinate =
      cloud.point(right).exact().coordinate(axis);
  if (right_coordinate > left_coordinate ||
      (right_coordinate == left_coordinate && right < left)) {
    return right;
  }
  return left;
}

[[nodiscard]] std::size_t snapshot_find_split(
    std::span<const MortonLeafRecord> leaves,
    std::size_t begin,
    std::size_t end) {
  if (begin + 1U >= end || end > leaves.size()) {
    throw std::logic_error(
        "a snapshot test split requires at least two leaves");
  }
  const std::uint64_t first_code = leaves[begin].morton_code;
  const std::uint64_t last_code = leaves[end - 1U].morton_code;
  if (first_code == last_code) {
    return begin + (end - begin) / 2U;
  }
  const std::uint64_t difference = first_code ^ last_code;
  const unsigned int highest_bit =
      static_cast<unsigned int>(std::bit_width(difference) - 1);
  const std::uint64_t mask = std::uint64_t{1} << highest_bit;
  std::size_t low = begin + 1U;
  std::size_t high = end;
  while (low < high) {
    const std::size_t middle = low + (high - low) / 2U;
    if ((leaves[middle].morton_code & mask) == 0U) {
      low = middle + 1U;
    } else {
      high = middle;
    }
  }
  return low;
}

[[nodiscard]] std::size_t append_snapshot_node(
    const CanonicalPointCloud& cloud,
    std::span<const MortonLeafRecord> leaves,
    std::size_t begin,
    std::size_t end,
    std::vector<MortonLbvhSnapshotNode>& nodes) {
  MortonLbvhSnapshotNode node;
  node.leaf_begin = static_cast<std::uint64_t>(begin);
  node.leaf_end = static_cast<std::uint64_t>(end);
  if (end - begin == 1U) {
    node.lower_point_ids.fill(leaves[begin].point_id);
    node.upper_point_ids.fill(leaves[begin].point_id);
    nodes.push_back(node);
    return nodes.size() - 1U;
  }
  const std::size_t split =
      snapshot_find_split(leaves, begin, end);
  const std::size_t left_child = append_snapshot_node(
      cloud, leaves, begin, split, nodes);
  const std::size_t right_child = append_snapshot_node(
      cloud, leaves, split, end, nodes);
  node.left_child = static_cast<std::uint64_t>(left_child);
  node.right_child = static_cast<std::uint64_t>(right_child);
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    node.lower_point_ids[axis] = snapshot_lower_witness(
        cloud,
        nodes[left_child].lower_point_ids[axis],
        nodes[right_child].lower_point_ids[axis],
        axis);
    node.upper_point_ids[axis] = snapshot_upper_witness(
        cloud,
        nodes[left_child].upper_point_ids[axis],
        nodes[right_child].upper_point_ids[axis],
        axis);
  }
  nodes.push_back(node);
  return nodes.size() - 1U;
}

[[nodiscard]] MortonLbvhSnapshot snapshot_from_index(
    const CanonicalPointCloud& cloud,
    const MortonLbvhIndex& index) {
  MortonLbvhSnapshot snapshot;
  snapshot.point_count = static_cast<std::uint64_t>(cloud.size());
  snapshot.root_aabb = index.root_aabb();
  const auto& counters = index.build_counters();
  snapshot.proposed_counters = {
      static_cast<std::uint64_t>(counters.point_count),
      static_cast<std::uint64_t>(counters.node_count),
      static_cast<std::uint64_t>(counters.maximum_depth),
      static_cast<std::uint64_t>(
          counters.morton_collision_group_count),
      static_cast<std::uint64_t>(
          counters.maximum_morton_collision_size)};
  snapshot.leaves = materialize_leaves(index);
  snapshot.nodes.reserve(counters.node_count);
  snapshot.root_node_index = static_cast<std::uint64_t>(
      append_snapshot_node(
          cloud,
          snapshot.leaves,
          0U,
          snapshot.leaves.size(),
          snapshot.nodes));
  return snapshot;
}

[[nodiscard]] bool same_neighbors(
    std::span<const ExactNeighbor> left,
    std::span<const ExactNeighbor> right) {
  if (left.size() != right.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < left.size(); ++index) {
    if (left[index].point_id != right[index].point_id ||
        left[index].squared_distance != right[index].squared_distance) {
      return false;
    }
  }
  return true;
}

void check_top_matches(
    const TopKPartition& actual,
    const TopKPartition& reference,
    const std::string& message) {
  check(actual.shell_complete(), message + " is complete");
  check(
      actual.requested_rank() == reference.requested_rank(),
      message + " preserves the requested rank");
  check(
      actual.cutoff_squared_distance() ==
          reference.cutoff_squared_distance(),
      message + " has the exact reference cutoff");
  check(
      same_neighbors(actual.strict_below(), reference.strict_below()),
      message + " has the exact ordered strict set");
  check(
      materialize_ids(actual.cutoff_shell_ids()) ==
          materialize_ids(reference.cutoff_shell_ids()),
      message + " has the complete reference shell");
  check(
      materialize_ids(actual.canonical_choice_ids()) ==
          materialize_ids(reference.canonical_choice_ids()),
      message + " has the reference canonical choice");
  check(
      actual.eligible_point_count() == reference.eligible_point_count(),
      message + " preserves the eligible population");
}

void check_budgeted_top_matches(
    const ExactBudgetedLbvhTopKResult& actual,
    const TopKPartition& reference,
    const ExactLbvhTopKBudget& budget,
    const std::string& message) {
  check(actual.complete(), message + " is complete");
  check(
      actual.status() == ExactLbvhTopKStatus::complete &&
          actual.stop_reason() == ExactLbvhTopKStopReason::none,
      message + " reports no exhaustion");
  check(
      actual.requested_budget() == budget,
      message + " preserves the requested budget");
  const auto& audit = actual.audit();
  check(audit.traversal_complete, message + " completed its traversal");
  check(
      audit.node_visit_count <= budget.maximum_node_visit_count &&
          audit.internal_node_expansion_count <=
              budget.maximum_internal_node_expansion_count &&
          audit.exact_aabb_bound_evaluation_count <=
              budget.maximum_exact_aabb_bound_evaluation_count &&
          audit.exact_point_distance_evaluation_count <=
              budget.maximum_exact_point_distance_evaluation_count &&
          audit.peak_frontier_entry_count <=
              budget.maximum_frontier_entry_count &&
          audit.peak_best_neighbor_entry_count <=
              budget.maximum_best_neighbor_entry_count &&
          audit.peak_retained_cutoff_shell_entry_count <=
              budget.maximum_cutoff_shell_entry_count,
      message + " respects every deterministic budget");
  check(
      audit.pruned_subtree_count ==
              actual.partition().query_counters().pruned_subtree_count &&
          audit.pruned_eligible_point_count ==
              actual.partition().query_counters()
                  .pruned_eligible_point_count,
      message + " preserves pruning work in its operational audit");
  check_top_matches(actual.partition(), reference, message + " partition");
}

void check_budgeted_top_exhaustion(
    const ExactBudgetedLbvhTopKResult& actual,
    const ExactLbvhTopKBudget& budget,
    ExactLbvhTopKStopReason expected_stop_reason,
    const std::string& message) {
  check(
      !actual.complete() &&
          actual.status() == ExactLbvhTopKStatus::budget_exhausted &&
          actual.stop_reason() == expected_stop_reason,
      message + " reports the expected fail-closed exhaustion");
  check(
      actual.requested_budget() == budget,
      message + " preserves the exhausted budget");
  const ExactLbvhTopKAudit& audit = actual.audit();
  check(
      audit.node_visit_count <= budget.maximum_node_visit_count &&
          audit.internal_node_expansion_count <=
              budget.maximum_internal_node_expansion_count &&
          audit.exact_aabb_bound_evaluation_count <=
              budget.maximum_exact_aabb_bound_evaluation_count &&
          audit.exact_point_distance_evaluation_count <=
              budget.maximum_exact_point_distance_evaluation_count &&
          audit.peak_frontier_entry_count <=
              budget.maximum_frontier_entry_count &&
          audit.peak_best_neighbor_entry_count <=
              budget.maximum_best_neighbor_entry_count &&
          audit.peak_retained_cutoff_shell_entry_count <=
              budget.maximum_cutoff_shell_entry_count,
      message + " never crosses any requested cap");
  check_throws<std::logic_error>(
      [&actual] { static_cast<void>(actual.partition()); },
      message + " publishes no partial top-k partition");
}

void check_ball_matches(
    const ClosedBallPartition& actual,
    const ClosedBallPartition& reference,
    const std::string& message) {
  check(actual.partition_complete(), message + " is complete");
  check(
      actual.squared_radius() == reference.squared_radius(),
      message + " preserves the exact radius");
  check(
      materialize_ids(actual.interior_ids()) ==
          materialize_ids(reference.interior_ids()),
      message + " has the reference interior");
  check(
      materialize_ids(actual.shell_ids()) ==
          materialize_ids(reference.shell_ids()),
      message + " has the complete reference shell");
  check(
      materialize_ids(actual.exterior_ids()) ==
          materialize_ids(reference.exterior_ids()),
      message + " has the reference exterior");
  check(
      actual.closed_rank() == reference.closed_rank() &&
          actual.evaluation_count() == reference.evaluation_count(),
      message + " preserves the global closed rank and population");
}

void check_top_counters(
    const TopKPartition& partition,
    std::size_t expected_excluded_count,
    const std::string& message) {
  const auto& counters = partition.query_counters();
  check(
      counters.method == SpatialQueryMethod::morton_lbvh,
      message + " identifies the Morton-LBVH method");
  check(
      counters.excluded_point_count == expected_excluded_count,
      message + " records every configured exclusion");
  check(
      counters.exact_point_distance_evaluation_count ==
          partition.distance_evaluation_count(),
      message + " exposes one exact-distance counter");
  check(
      counters.exact_point_distance_evaluation_count +
              counters.pruned_eligible_point_count ==
          partition.eligible_point_count(),
      message + " partitions all eligible leaves into evaluated and pruned");
  check(
      counters.node_visit_count ==
              counters.exact_aabb_bound_evaluation_count &&
          counters.node_visit_count ==
              1U + 2U * counters.internal_node_expansion_count,
      message + " accounts for every enqueued exact AABB bound");
  check(
      counters.bulk_interior_subtree_count == 0U &&
          counters.bulk_interior_point_count == 0U,
      message + " never uses ball-only bulk-interior accounting");
  const bool has_strict_pruning = counters.pruned_subtree_count != 0U;
  check(
      counters.minimum_strict_pruning_margin.has_value() ==
          has_strict_pruning,
      message + " binds a margin exactly to strict pruning");
  if (counters.minimum_strict_pruning_margin.has_value()) {
    check(
        *counters.minimum_strict_pruning_margin > ExactLevel{BigInt{0}},
        message + " records a strictly positive pruning margin");
  }
}

void check_ball_counters(
    const ClosedBallPartition& partition,
    const std::string& message) {
  const auto& counters = partition.query_counters();
  check(
      counters.method == SpatialQueryMethod::morton_lbvh,
      message + " identifies the Morton-LBVH method");
  check(
      counters.excluded_point_count == 0U,
      message + " remains a global, unfiltered partition");
  check(
      counters.exact_point_distance_evaluation_count ==
          partition.distance_evaluation_count(),
      message + " exposes one exact-distance counter");
  check(
      counters.exact_point_distance_evaluation_count +
              counters.pruned_eligible_point_count +
              counters.bulk_interior_point_count ==
          partition.evaluation_count(),
      message + " partitions the complete cloud accounting");
  check(
      counters.node_visit_count ==
          1U + 2U * counters.internal_node_expansion_count,
      message + " visits exactly the children of expanded internal nodes");
  check(
      counters.exact_aabb_bound_evaluation_count >=
              counters.node_visit_count &&
          counters.exact_aabb_bound_evaluation_count <=
              2U * counters.node_visit_count,
      message + " evaluates one or two exact bounds per visited node");
  const bool has_strict_classification =
      counters.pruned_subtree_count != 0U ||
      counters.bulk_interior_subtree_count != 0U;
  check(
      counters.minimum_strict_pruning_margin.has_value() ==
          has_strict_classification,
      message + " binds a margin exactly to strict subtree classification");
  if (counters.minimum_strict_pruning_margin.has_value()) {
    check(
        *counters.minimum_strict_pruning_margin > ExactLevel{BigInt{0}},
        message + " records a strictly positive classification margin");
  }
}

void check_import_matches_build(
    const CanonicalPointCloud& cloud,
    const std::string& message) {
  const MortonLbvhIndex built = MortonLbvhIndex::build(cloud);
  const MortonLbvhSnapshot snapshot =
      snapshot_from_index(cloud, built);
  const MortonLbvhIndex imported =
      MortonLbvhIndex::import_certified_snapshot(cloud, snapshot);
  check(
      imported.ready() && imported.validated_for(cloud),
      message + " publishes a namespace-bound imported index");
  check(
      materialize_leaves(imported) == materialize_leaves(built) &&
          imported.build_counters() == built.build_counters() &&
          imported.root_aabb() == built.root_aabb(),
      message + " exactly matches every public build artifact");

  const ExclusionSet exclusions = empty_exclusions(cloud);
  const std::size_t rank = std::min<std::size_t>(3U, cloud.size());
  const TopKPartition built_top = lbvh_top_k(
      built,
      cloud,
      origin(),
      rank,
      exclusions,
      LbvhTraversalOrder::near_first);
  const TopKPartition imported_top = lbvh_top_k(
      imported,
      cloud,
      origin(),
      rank,
      exclusions,
      LbvhTraversalOrder::near_first);
  check_top_matches(
      imported_top, built_top, message + " top-k");
  check(
      imported_top.query_counters() ==
          built_top.query_counters(),
      message + " reproduces the complete build traversal");
  const ClosedBallPartition built_ball = lbvh_closed_ball(
      built,
      cloud,
      origin(),
      built_top.cutoff_squared_distance(),
      LbvhTraversalOrder::far_first);
  const ClosedBallPartition imported_ball = lbvh_closed_ball(
      imported,
      cloud,
      origin(),
      built_top.cutoff_squared_distance(),
      LbvhTraversalOrder::far_first);
  check_ball_matches(
      imported_ball, built_ball, message + " closed ball");
  check(
      imported_ball.query_counters() ==
          built_ball.query_counters(),
      message + " reproduces the complete ball traversal");
}

void check_leaf_permutation_and_order(
    const MortonLbvhIndex& index,
    std::size_t point_count,
    const std::string& message) {
  const auto leaves = index.leaves();
  check(leaves.size() == point_count, message + " has one leaf per point");
  std::vector<PointId> ids;
  ids.reserve(leaves.size());
  for (std::size_t position = 0U; position < leaves.size(); ++position) {
    ids.push_back(leaves[position].point_id);
    check(
        leaves[position].morton_code <=
            (std::numeric_limits<std::uint64_t>::max() >> 1U),
        message + " uses at most 63 Morton bits");
    if (position != 0U) {
      const MortonLeafRecord& previous = leaves[position - 1U];
      const MortonLeafRecord& current = leaves[position];
      check(
          previous.morton_code < current.morton_code ||
              (previous.morton_code == current.morton_code &&
               previous.point_id < current.point_id),
          message + " sorts leaves by (Morton code, PointId)");
    }
  }
  std::sort(ids.begin(), ids.end());
  for (std::size_t position = 0U; position < ids.size(); ++position) {
    check(
        ids[position] == static_cast<PointId>(position),
        message + " is a permutation of the canonical PointId namespace");
  }
}

void test_singleton_build_and_boundary_queries() {
  const std::array<CertifiedPoint3, 1> input{
      point(-0.0, 2.0, -3.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExclusionSet exclusions = empty_exclusions(cloud);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);

  check(index.ready() && index.validated_for(cloud),
        "a singleton LBVH is complete and namespace-bound");
  check_leaf_permutation_and_order(index, 1U, "the singleton LBVH");
  check(
      index.leaves().front() == MortonLeafRecord{0U, PointId{0}},
      "a singleton has the zero Morton code and canonical PointId zero");
  const auto& build = index.build_counters();
  check(
      build.point_count == 1U && build.node_count == 1U &&
          build.maximum_depth == 0U &&
          build.morton_collision_group_count == 0U &&
          build.maximum_morton_collision_size == 0U,
      "a singleton build reports its exact one-node topology");
  const auto point_bits = cloud.point(PointId{0}).canonical_input_bits();
  check(
      index.root_aabb().lower_binary64_bits == point_bits &&
          index.root_aabb().upper_binary64_bits == point_bits &&
          point_bits[0] == 0U,
      "a singleton root AABB is its exact canonical input dyadic");

  const auto brute_top = brute_force_nearest(cloud, origin(), exclusions);
  const auto near_top = lbvh_nearest(
      index, cloud, origin(), exclusions, LbvhTraversalOrder::near_first);
  const auto far_top = lbvh_nearest(
      index, cloud, origin(), exclusions, LbvhTraversalOrder::far_first);
  check_top_matches(near_top, brute_top, "the singleton near-first query");
  check_top_matches(far_top, brute_top, "the singleton far-first query");
  check_top_counters(near_top, 0U, "the singleton top-k counters");
  check(
      near_top.distance_evaluation_count() == 1U &&
          near_top.query_counters().pruned_subtree_count == 0U,
      "a singleton top-k query evaluates its only point without pruning");
  check(near_top.validated_for(cloud),
        "a singleton top-k certificate retains its PointId namespace");

  const ExactLevel radius = brute_top.cutoff_squared_distance();
  const auto brute_ball = brute_force_closed_ball(cloud, origin(), radius);
  const auto near_ball = lbvh_closed_ball(
      index, cloud, origin(), radius, LbvhTraversalOrder::near_first);
  const auto far_ball = lbvh_closed_ball(
      index, cloud, origin(), radius, LbvhTraversalOrder::far_first);
  check_ball_matches(near_ball, brute_ball, "the singleton near-first ball");
  check_ball_matches(far_ball, brute_ball, "the singleton far-first ball");
  check_ball_counters(near_ball, "the singleton ball counters");
  check(
      near_ball.distance_evaluation_count() == 1U &&
          near_ball.query_counters().pruned_subtree_count == 0U &&
          near_ball.query_counters().bulk_interior_subtree_count == 0U,
      "equality at a singleton AABB is evaluated rather than pruned");
  check(near_ball.validated_for(cloud),
        "a singleton ball certificate retains its PointId namespace");
}

void test_morton_collisions_and_permutation_determinism() {
  const double tiny = std::numeric_limits<double>::denorm_min();
  const double twice_tiny = std::ldexp(tiny, 1);
  const std::array<CertifiedPoint3, 4> first_input{
      point(1.0, 1.0, 1.0),
      point(twice_tiny, 0.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(tiny, 0.0, 0.0)};
  const std::array<CertifiedPoint3, 4> second_input{
      point(tiny, 0.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(1.0, 1.0, 1.0),
      point(twice_tiny, 0.0, 0.0)};
  const CanonicalPointCloud first_cloud = canonical_cloud(first_input);
  const CanonicalPointCloud second_cloud = canonical_cloud(second_input);
  const MortonLbvhIndex first = MortonLbvhIndex::build(first_cloud);
  const MortonLbvhIndex second = MortonLbvhIndex::build(second_cloud);

  check_leaf_permutation_and_order(first, 4U, "the collision LBVH");
  check(
      materialize_leaves(first) == materialize_leaves(second),
      "input permutation cannot change canonical Morton leaves");
  check(
      first.build_counters() == second.build_counters(),
      "input permutation cannot change LBVH build counters");
  check(
      first.root_aabb() == second.root_aabb(),
      "input permutation cannot change the exact root AABB");
  const auto leaves = first.leaves();
  check(
      leaves[0] == MortonLeafRecord{0U, PointId{0}} &&
          leaves[1] == MortonLeafRecord{0U, PointId{1}} &&
          leaves[2] == MortonLeafRecord{0U, PointId{2}} &&
          leaves[3] == MortonLeafRecord{
              std::numeric_limits<std::uint64_t>::max() >> 1U,
              PointId{3}},
      "an exact three-way Morton collision is resolved by increasing PointId");
  const auto& build = first.build_counters();
  check(
      build.point_count == 4U && build.node_count == 7U &&
          build.maximum_depth >= 2U && build.maximum_depth <= 3U &&
          build.morton_collision_group_count == 1U &&
          build.maximum_morton_collision_size == 3U,
      "the collision build reports its group size and full binary topology");
  const std::array<std::uint64_t, 3> zero_bits{0U, 0U, 0U};
  const std::uint64_t one_bits = std::bit_cast<std::uint64_t>(1.0);
  const std::array<std::uint64_t, 3> unit_bits{
      one_bits, one_bits, one_bits};
  check(
      first.root_aabb().lower_binary64_bits == zero_bits &&
          first.root_aabb().upper_binary64_bits == unit_bits,
      "the collision root AABB selects exact endpoint input dyadics");
}

void test_finite_extrema_remain_exact() {
  const double maximum = std::numeric_limits<double>::max();
  const double tiny = std::numeric_limits<double>::denorm_min();
  const std::array<CertifiedPoint3, 4> input{
      point(-maximum, -tiny, 0.0),
      point(tiny, maximum, -maximum),
      point(maximum, 0.0, tiny),
      point(0.0, -maximum, maximum)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExclusionSet exclusions = empty_exclusions(cloud);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  check_leaf_permutation_and_order(index, 4U, "the finite-extrema LBVH");

  const std::uint64_t negative_maximum_bits =
      std::bit_cast<std::uint64_t>(-maximum);
  const std::uint64_t maximum_bits = std::bit_cast<std::uint64_t>(maximum);
  const std::array<std::uint64_t, 3> expected_lower{
      negative_maximum_bits,
      negative_maximum_bits,
      negative_maximum_bits};
  const std::array<std::uint64_t, 3> expected_upper{
      maximum_bits, maximum_bits, maximum_bits};
  check(
      index.root_aabb().lower_binary64_bits == expected_lower &&
          index.root_aabb().upper_binary64_bits == expected_upper,
      "the root AABB encloses finite binary64 extrema without nextafter overflow");

  const ExactRational3 query{
      BigInt{1}, BigInt{-2}, BigInt{7}, BigInt{11}};
  const auto brute_top = brute_force_top_k(cloud, query, 3U, exclusions);
  const auto near_top = lbvh_top_k(
      index, cloud, query, 3U, exclusions, LbvhTraversalOrder::near_first);
  const auto far_top = lbvh_top_k(
      index, cloud, query, 3U, exclusions, LbvhTraversalOrder::far_first);
  check_top_matches(near_top, brute_top, "the finite-extrema near-first top-k");
  check_top_matches(far_top, brute_top, "the finite-extrema far-first top-k");
  check_top_counters(near_top, 0U, "the finite-extrema top-k counters");

  const ExactLevel radius = brute_top.cutoff_squared_distance();
  const auto brute_ball = brute_force_closed_ball(cloud, query, radius);
  const auto near_ball = lbvh_closed_ball(
      index, cloud, query, radius, LbvhTraversalOrder::near_first);
  const auto far_ball = lbvh_closed_ball(
      index, cloud, query, radius, LbvhTraversalOrder::far_first);
  check_ball_matches(
      near_ball, brute_ball, "the finite-extrema near-first ball");
  check_ball_matches(
      far_ball, brute_ball, "the finite-extrema far-first ball");
  check_ball_counters(near_ball, "the finite-extrema ball counters");
}

void test_bound_equality_never_prunes_complete_shells() {
  const std::array<CertifiedPoint3, 6> input{
      point(-1.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, -1.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.0, 0.0, -1.0),
      point(0.0, 0.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExclusionSet exclusions = empty_exclusions(cloud);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto brute_top = brute_force_top_k(cloud, origin(), 3U, exclusions);

  for (const LbvhTraversalOrder order : {
           LbvhTraversalOrder::near_first,
           LbvhTraversalOrder::far_first}) {
    const auto top = lbvh_top_k(index, cloud, origin(), 3U, exclusions, order);
    check_top_matches(top, brute_top, "the six-way equality top-k");
    check_top_counters(top, 0U, "the six-way equality top-k counters");
    check(
        top.cutoff_squared_distance() == ExactLevel{BigInt{1}} &&
            top.strict_below().empty() &&
            top.cutoff_shell_ids().size() == 6U &&
            top.distance_evaluation_count() == 6U &&
            top.query_counters().pruned_subtree_count == 0U &&
            !top.query_counters().minimum_strict_pruning_margin.has_value(),
        "AABB lower-bound equality preserves all six cutoff co-minimizers");
  }

  const ExactLevel unit_radius{BigInt{1}};
  const auto brute_ball =
      brute_force_closed_ball(cloud, origin(), unit_radius);
  for (const LbvhTraversalOrder order : {
           LbvhTraversalOrder::near_first,
           LbvhTraversalOrder::far_first}) {
    const auto ball =
        lbvh_closed_ball(index, cloud, origin(), unit_radius, order);
    check_ball_matches(ball, brute_ball, "the six-way equality ball");
    check_ball_counters(ball, "the six-way equality ball counters");
    check(
        ball.interior_ids().empty() && ball.shell_ids().size() == 6U &&
            ball.exterior_ids().empty() &&
            ball.distance_evaluation_count() == 6U &&
            ball.query_counters().pruned_subtree_count == 0U &&
            ball.query_counters().bulk_interior_subtree_count == 0U &&
            !ball.query_counters().minimum_strict_pruning_margin.has_value(),
        "AABB min/max equality descends to all six exact shell points");
  }
}

[[nodiscard]] CanonicalPointCloud nonnegative_line_cloud(
    std::size_t point_count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    points.push_back(point(static_cast<double>(index), 0.0, 0.0));
  }
  return canonical_cloud(points);
}

void test_strict_pruning_bulk_classification_and_exclusions() {
  constexpr std::size_t point_count = 64U;
  const CanonicalPointCloud cloud = nonnegative_line_cloud(point_count);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExclusionSet no_exclusions = empty_exclusions(cloud);
  const auto brute_nearest =
      brute_force_nearest(cloud, origin(), no_exclusions);
  const auto near_nearest = lbvh_nearest(
      index,
      cloud,
      origin(),
      no_exclusions,
      LbvhTraversalOrder::near_first);
  const auto far_nearest = lbvh_nearest(
      index,
      cloud,
      origin(),
      no_exclusions,
      LbvhTraversalOrder::far_first);
  check_top_matches(
      near_nearest, brute_nearest, "the strictly pruned near-first nearest");
  check_top_matches(
      far_nearest, brute_nearest, "the far-first nearest");
  check_top_counters(
      near_nearest, 0U, "the strictly pruned near-first counters");
  check_top_counters(far_nearest, 0U, "the far-first nearest counters");
  check(
      near_nearest.distance_evaluation_count() == 1U &&
          near_nearest.query_counters().pruned_eligible_point_count ==
              point_count - 1U &&
          near_nearest.query_counters().pruned_subtree_count != 0U,
      "near-first rank one certifies all farther points through strict bounds");
  check(
      near_nearest.distance_evaluation_count() <
          far_nearest.distance_evaluation_count(),
      "near-first ordering performs fewer exact point evaluations on a line");

  const ExactLevel radius{BigInt{100}};
  const auto brute_ball = brute_force_closed_ball(cloud, origin(), radius);
  const auto near_ball = lbvh_closed_ball(
      index, cloud, origin(), radius, LbvhTraversalOrder::near_first);
  const auto far_ball = lbvh_closed_ball(
      index, cloud, origin(), radius, LbvhTraversalOrder::far_first);
  check_ball_matches(near_ball, brute_ball, "the strictly classified ball");
  check_ball_matches(
      far_ball, brute_ball, "the far-first strictly classified ball");
  check_ball_counters(near_ball, "the strictly classified ball counters");
  check_ball_counters(far_ball, "the far-first ball counters");
  check(
      near_ball.interior_ids().size() == 10U &&
          near_ball.shell_ids().size() == 1U &&
          near_ball.exterior_ids().size() == point_count - 11U &&
          near_ball.distance_evaluation_count() == 1U &&
          near_ball.query_counters().bulk_interior_point_count == 10U &&
          near_ball.query_counters().pruned_eligible_point_count ==
              point_count - 11U &&
          near_ball.query_counters().bulk_interior_subtree_count != 0U &&
          near_ball.query_counters().pruned_subtree_count != 0U,
      "strict AABB bounds bulk-classify every non-shell point exactly");

  const std::array<PointId, 3> excluded_ids{
      PointId{10}, PointId{0}, PointId{1}};
  const ExclusionSet exclusions = ExclusionSet::from_ids(
      std::span<const PointId>{excluded_ids}, cloud, 3U);
  const auto brute_top = brute_force_top_k(cloud, origin(), 2U, exclusions);
  const auto near_top = lbvh_top_k(
      index,
      cloud,
      origin(),
      2U,
      exclusions,
      LbvhTraversalOrder::near_first);
  const auto far_top = lbvh_top_k(
      index,
      cloud,
      origin(),
      2U,
      exclusions,
      LbvhTraversalOrder::far_first);
  check_top_matches(near_top, brute_top, "the excluded near-first top-2");
  check_top_matches(far_top, brute_top, "the excluded far-first top-2");
  check_top_counters(near_top, 3U, "the excluded near-first counters");
  check_top_counters(far_top, 3U, "the excluded far-first counters");
  check(
      near_top.cutoff_squared_distance() == ExactLevel{BigInt{9}} &&
          near_top.distance_evaluation_count() == 2U &&
          near_top.query_counters().pruned_eligible_point_count ==
              point_count - excluded_ids.size() - 2U,
      "excluded leaves are neither evaluated nor charged as pruned eligible points");
  for (const PointId excluded_id : excluded_ids) {
    check(
        std::find(
            near_top.canonical_choice_ids().begin(),
            near_top.canonical_choice_ids().end(),
            excluded_id) == near_top.canonical_choice_ids().end(),
        "an excluded PointId cannot enter the canonical top-k choice");
  }

  check_throws<std::out_of_range>(
      [&index, &cloud, &exclusions] {
        static_cast<void>(
            lbvh_top_k(index, cloud, origin(), 0U, exclusions));
      },
      "LBVH top-k rejects rank zero");
  check_throws<std::out_of_range>(
      [&index, &cloud, &exclusions] {
        static_cast<void>(lbvh_top_k(
            index,
            cloud,
            origin(),
            cloud.size() - exclusions.ids().size() + 1U,
            exclusions));
      },
      "LBVH top-k rejects a rank beyond the eligible population");

  const auto invalid_order = static_cast<LbvhTraversalOrder>(255U);
  check_throws<std::invalid_argument>(
      [&index, &cloud, &no_exclusions] {
        static_cast<void>(lbvh_nearest(
            index, cloud, origin(), no_exclusions, invalid_order));
      },
      "LBVH top-k rejects an invalid traversal-order value");
  check_throws<std::invalid_argument>(
      [&index, &cloud] {
        static_cast<void>(lbvh_closed_ball(
            index,
            cloud,
            origin(),
            ExactLevel{BigInt{1}},
            invalid_order));
      },
      "LBVH closed-ball rejects an invalid traversal-order value");
}

void test_budgeted_top_k_exactness_and_fail_closed_limits() {
  const std::array<CertifiedPoint3, 6> equality_input{
      point(-1.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, -1.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.0, 0.0, -1.0),
      point(0.0, 0.0, 1.0)};
  const CanonicalPointCloud equality_cloud = canonical_cloud(equality_input);
  const ExclusionSet equality_exclusions =
      empty_exclusions(equality_cloud);
  const MortonLbvhIndex equality_index =
      MortonLbvhIndex::build(equality_cloud);
  const TopKPartition equality_reference = brute_force_top_k(
      equality_cloud, origin(), 3U, equality_exclusions);
  const ExactLbvhTopKBudget generous_budget{
      4096U,
      4096U,
      4096U,
      4096U,
      128U,
      3U,
      6U};

  for (const LbvhTraversalOrder order : {
           LbvhTraversalOrder::near_first,
           LbvhTraversalOrder::far_first}) {
    const TopKPartition legacy = lbvh_top_k(
        equality_index,
        equality_cloud,
        origin(),
        3U,
        equality_exclusions,
        order);
    const ExactBudgetedLbvhTopKResult first = lbvh_top_k_budgeted(
        equality_index,
        equality_cloud,
        origin(),
        3U,
        equality_exclusions,
        generous_budget,
        order);
    const ExactBudgetedLbvhTopKResult repeated = lbvh_top_k_budgeted(
        equality_index,
        equality_cloud,
        origin(),
        3U,
        equality_exclusions,
        generous_budget,
        order);
    check_budgeted_top_matches(
        first, equality_reference, generous_budget,
        "the budgeted six-way equality query");
    check_top_matches(
        first.partition(), legacy,
        "the budgeted/legacy six-way equality query");
    check(
        first.audit() == repeated.audit(),
        "identical budgeted traversals have deterministic audits");
    check(
        first.audit().supplied_incumbent_point_count == 0U &&
            first.audit().exact_incumbent_distance_evaluation_count == 0U,
        "the historical budgeted path records no supplied incumbents");
    check(
        first.partition().distance_evaluation_count() == 6U &&
            first.partition().query_counters().pruned_subtree_count == 0U,
        "budgeted top-k never prunes an AABB bound equal to the cutoff");
    ExactBudgetedLbvhTopKResult move_source = first;
    ExactBudgetedLbvhTopKResult move_target = std::move(move_source);
    check_top_matches(
        move_target.partition(), equality_reference,
        "the moved budgeted equality query");
    check(
        !move_source.complete() &&
            move_source.status() ==
                ExactLbvhTopKStatus::not_certified &&
            move_source.stop_reason() == ExactLbvhTopKStopReason::none &&
            !move_source.audit().traversal_complete,
        "a moved-from budgeted result is explicitly not certified");
    check_throws<std::logic_error>(
        [&move_source] { static_cast<void>(move_source.partition()); },
        "a moved-from budgeted result exposes no stale partition");
    self_move_assign(move_target);
    check_top_matches(
        move_target.partition(), equality_reference,
        "a self-moved budgeted equality query");

    const ExactLbvhTopKAudit& complete_audit = first.audit();
    check(
        complete_audit.node_visit_count != 0U &&
            complete_audit.internal_node_expansion_count != 0U &&
            complete_audit.exact_aabb_bound_evaluation_count != 0U &&
            complete_audit.exact_point_distance_evaluation_count != 0U &&
            complete_audit.peak_frontier_entry_count != 0U &&
            complete_audit.peak_best_neighbor_entry_count != 0U &&
            complete_audit.peak_retained_cutoff_shell_entry_count != 0U,
        "the six-way equality fixture exercises every top-k budget dimension");
    const ExactLbvhTopKBudget exact_budget{
        complete_audit.node_visit_count,
        complete_audit.internal_node_expansion_count,
        complete_audit.exact_aabb_bound_evaluation_count,
        complete_audit.exact_point_distance_evaluation_count,
        complete_audit.peak_frontier_entry_count,
        complete_audit.peak_best_neighbor_entry_count,
        complete_audit.peak_retained_cutoff_shell_entry_count};
    const ExactBudgetedLbvhTopKResult exact = lbvh_top_k_budgeted(
        equality_index,
        equality_cloud,
        origin(),
        3U,
        equality_exclusions,
        exact_budget,
        order);
    check_budgeted_top_matches(
        exact, equality_reference, exact_budget,
        "the all-seven-exact-boundary budgeted equality query");

    ExactLbvhTopKBudget short_node_visit_budget = exact_budget;
    --short_node_visit_budget.maximum_node_visit_count;
    const ExactBudgetedLbvhTopKResult short_node_visit = lbvh_top_k_budgeted(
        equality_index,
        equality_cloud,
        origin(),
        3U,
        equality_exclusions,
        short_node_visit_budget,
        order);
    check_budgeted_top_exhaustion(
        short_node_visit,
        short_node_visit_budget,
        ExactLbvhTopKStopReason::node_visit_limit,
        "the one-short node-visit budget");
    check(
        !short_node_visit.audit().traversal_complete &&
            short_node_visit.audit().node_visit_count ==
                short_node_visit_budget.maximum_node_visit_count,
        "one missing node visit exhausts before the final frontier pop");

    ExactLbvhTopKBudget short_expansion_budget = exact_budget;
    --short_expansion_budget.maximum_internal_node_expansion_count;
    const ExactBudgetedLbvhTopKResult short_expansion = lbvh_top_k_budgeted(
        equality_index,
        equality_cloud,
        origin(),
        3U,
        equality_exclusions,
        short_expansion_budget,
        order);
    check_budgeted_top_exhaustion(
        short_expansion,
        short_expansion_budget,
        ExactLbvhTopKStopReason::internal_node_expansion_limit,
        "the one-short internal-expansion budget");
    check(
        !short_expansion.audit().traversal_complete &&
            short_expansion.audit().internal_node_expansion_count ==
                short_expansion_budget.maximum_internal_node_expansion_count,
        "one missing internal expansion exhausts before child publication");

    ExactLbvhTopKBudget short_aabb_budget = exact_budget;
    --short_aabb_budget.maximum_exact_aabb_bound_evaluation_count;
    const ExactBudgetedLbvhTopKResult short_aabb = lbvh_top_k_budgeted(
        equality_index,
        equality_cloud,
        origin(),
        3U,
        equality_exclusions,
        short_aabb_budget,
        order);
    check_budgeted_top_exhaustion(
        short_aabb,
        short_aabb_budget,
        ExactLbvhTopKStopReason::exact_aabb_bound_evaluation_limit,
        "the one-short exact-AABB budget");
    check(
        !short_aabb.audit().traversal_complete,
        "one missing exact AABB evaluation exhausts before a child-bound pair");

    ExactLbvhTopKBudget short_distance_budget = exact_budget;
    --short_distance_budget.maximum_exact_point_distance_evaluation_count;
    const ExactBudgetedLbvhTopKResult short_distance = lbvh_top_k_budgeted(
        equality_index,
        equality_cloud,
        origin(),
        3U,
        equality_exclusions,
        short_distance_budget,
        order);
    check_budgeted_top_exhaustion(
        short_distance,
        short_distance_budget,
        ExactLbvhTopKStopReason::exact_point_distance_evaluation_limit,
        "the one-short exact-distance budget");
    check(
        !short_distance.audit().traversal_complete &&
            short_distance.audit().exact_point_distance_evaluation_count ==
                short_distance_budget
                    .maximum_exact_point_distance_evaluation_count,
        "one missing exact point distance exhausts before leaf evaluation");

    ExactLbvhTopKBudget short_frontier_budget = exact_budget;
    --short_frontier_budget.maximum_frontier_entry_count;
    const ExactBudgetedLbvhTopKResult short_frontier =
        lbvh_top_k_budgeted(
            equality_index,
            equality_cloud,
            origin(),
            3U,
            equality_exclusions,
            short_frontier_budget,
            order);
    check_budgeted_top_exhaustion(
        short_frontier,
        short_frontier_budget,
        ExactLbvhTopKStopReason::frontier_entry_limit,
        "the one-short frontier budget");
    check(
        !short_frontier.audit().traversal_complete &&
            short_frontier.audit().peak_frontier_entry_count <=
                short_frontier_budget.maximum_frontier_entry_count,
        "one missing frontier entry exhausts before queue growth");

    ExactLbvhTopKBudget short_best_neighbor_budget = exact_budget;
    --short_best_neighbor_budget.maximum_best_neighbor_entry_count;
    const ExactBudgetedLbvhTopKResult short_best_neighbor =
        lbvh_top_k_budgeted(
            equality_index,
            equality_cloud,
            origin(),
            3U,
            equality_exclusions,
            short_best_neighbor_budget,
            order);
    check_budgeted_top_exhaustion(
        short_best_neighbor,
        short_best_neighbor_budget,
        ExactLbvhTopKStopReason::best_neighbor_entry_limit,
        "the one-short best-neighbor budget");
    check(
        !short_best_neighbor.audit().traversal_complete &&
            short_best_neighbor.audit().node_visit_count == 0U &&
            short_best_neighbor.audit().peak_best_neighbor_entry_count == 0U,
        "one missing best-neighbor slot exhausts before traversal allocation");

    ExactLbvhTopKBudget short_shell_budget = exact_budget;
    --short_shell_budget.maximum_cutoff_shell_entry_count;
    const ExactBudgetedLbvhTopKResult short_shell = lbvh_top_k_budgeted(
        equality_index,
        equality_cloud,
        origin(),
        3U,
        equality_exclusions,
        short_shell_budget,
        order);
    check_budgeted_top_exhaustion(
        short_shell,
        short_shell_budget,
        ExactLbvhTopKStopReason::cutoff_shell_entry_limit,
        "the one-short cutoff-shell budget");
    check(
        short_shell.audit().traversal_complete &&
            short_shell.audit().peak_retained_cutoff_shell_entry_count ==
                short_shell_budget.maximum_cutoff_shell_entry_count &&
            short_shell.audit().provisional_cutoff_shell_overflow_count != 0U,
        "a final shell overflow exhausts only after the complete traversal");
  }

  const ExactLbvhTopKBudget zero_budget{};
  const ExactBudgetedLbvhTopKResult zero = lbvh_top_k_budgeted(
      equality_index,
      equality_cloud,
      origin(),
      3U,
      equality_exclusions,
      zero_budget);
  check_budgeted_top_exhaustion(
      zero,
      zero_budget,
      ExactLbvhTopKStopReason::best_neighbor_entry_limit,
      "the all-zero budget");
  check(
      zero.audit().node_visit_count == 0U &&
          zero.audit().internal_node_expansion_count == 0U &&
          zero.audit().exact_aabb_bound_evaluation_count == 0U &&
          zero.audit().exact_point_distance_evaluation_count == 0U &&
          zero.audit().peak_frontier_entry_count == 0U &&
          zero.audit().peak_best_neighbor_entry_count == 0U &&
          zero.audit().peak_retained_cutoff_shell_entry_count == 0U &&
          !zero.audit().traversal_complete,
      "a zero budget fails closed before any bounded query operation");

  const std::array<CertifiedPoint3, 13> temporary_overflow_input{
      point(-3.0, 0.0, 0.0),
      point(3.0, 0.0, 0.0),
      point(0.0, -3.0, 0.0),
      point(0.0, 3.0, 0.0),
      point(0.0, 0.0, -3.0),
      point(0.0, 0.0, 3.0),
      point(-2.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(0.0, -2.0, 0.0),
      point(0.0, 2.0, 0.0),
      point(0.0, 0.0, -2.0),
      point(0.0, 0.0, 2.0),
      point(0.0, 0.0, 0.0)};
  const CanonicalPointCloud temporary_overflow_cloud =
      canonical_cloud(temporary_overflow_input);
  const ExclusionSet temporary_overflow_exclusions =
      empty_exclusions(temporary_overflow_cloud);
  const MortonLbvhIndex temporary_overflow_index =
      MortonLbvhIndex::build(temporary_overflow_cloud);
  const TopKPartition temporary_overflow_reference = brute_force_top_k(
      temporary_overflow_cloud,
      origin(),
      1U,
      temporary_overflow_exclusions);
  const ExactLbvhTopKBudget temporary_overflow_budget{
      4096U,
      4096U,
      4096U,
      4096U,
      128U,
      1U,
      1U};
  const ExactBudgetedLbvhTopKResult temporary_overflow =
      lbvh_top_k_budgeted(
          temporary_overflow_index,
          temporary_overflow_cloud,
          origin(),
          1U,
          temporary_overflow_exclusions,
          temporary_overflow_budget,
          LbvhTraversalOrder::far_first);
  check_budgeted_top_matches(
      temporary_overflow,
      temporary_overflow_reference,
      temporary_overflow_budget,
      "the temporary-shell-overflow query");
  check(
      temporary_overflow.audit().provisional_cutoff_decrease_count >= 2U &&
          temporary_overflow.audit()
                  .provisional_cutoff_shell_overflow_count !=
              0U,
      "successive cutoff decreases cancel earlier provisional shell overflows");

  constexpr std::size_t line_point_count = 64U;
  const CanonicalPointCloud line_cloud =
      nonnegative_line_cloud(line_point_count);
  const MortonLbvhIndex line_index = MortonLbvhIndex::build(line_cloud);
  const std::array<PointId, 3> excluded_ids{
      PointId{10}, PointId{0}, PointId{1}};
  const ExclusionSet line_exclusions = ExclusionSet::from_ids(
      std::span<const PointId>{excluded_ids}, line_cloud, 3U);
  const TopKPartition line_reference =
      brute_force_top_k(line_cloud, origin(), 2U, line_exclusions);
  const ExactLbvhTopKBudget line_budget{
      4096U,
      4096U,
      4096U,
      4096U,
      128U,
      2U,
      1U};
  const ExactBudgetedLbvhTopKResult line = lbvh_top_k_budgeted(
      line_index,
      line_cloud,
      origin(),
      2U,
      line_exclusions,
      line_budget,
      LbvhTraversalOrder::near_first);
  check_budgeted_top_matches(
      line, line_reference, line_budget,
      "the budgeted excluded and strictly pruned line query");
  check(
      line.partition().query_counters().pruned_subtree_count != 0U &&
          line.partition().distance_evaluation_count() +
                  line.partition().query_counters().pruned_eligible_point_count ==
              line.partition().eligible_point_count(),
      "a complete budgeted query accounts for every eligible leaf");

  std::vector<CertifiedPoint3> pruned_overflow_points{
      point(-1.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0)};
  for (std::size_t point_index = 0U; point_index < 32U; ++point_index) {
    pruned_overflow_points.push_back(
        point(32.0 + static_cast<double>(point_index), 0.0, 0.0));
  }
  const CanonicalPointCloud pruned_overflow_cloud =
      canonical_cloud(pruned_overflow_points);
  const MortonLbvhIndex pruned_overflow_index =
      MortonLbvhIndex::build(pruned_overflow_cloud);
  const ExclusionSet pruned_overflow_exclusions =
      empty_exclusions(pruned_overflow_cloud);
  const ExactLbvhTopKBudget pruned_overflow_budget{
      4096U,
      4096U,
      4096U,
      4096U,
      128U,
      1U,
      1U};
  const ExactBudgetedLbvhTopKResult pruned_overflow =
      lbvh_top_k_budgeted(
          pruned_overflow_index,
          pruned_overflow_cloud,
          origin(),
          1U,
          pruned_overflow_exclusions,
          pruned_overflow_budget,
          LbvhTraversalOrder::near_first);
  check_budgeted_top_exhaustion(
      pruned_overflow,
      pruned_overflow_budget,
      ExactLbvhTopKStopReason::cutoff_shell_entry_limit,
      "the strictly pruned final-shell-overflow query");
  check(
      pruned_overflow.audit().traversal_complete &&
          pruned_overflow.audit().pruned_subtree_count != 0U &&
          pruned_overflow.audit().pruned_eligible_point_count != 0U &&
          pruned_overflow.audit()
                  .provisional_cutoff_shell_overflow_count !=
              0U &&
          pruned_overflow.audit().exact_point_distance_evaluation_count +
                  pruned_overflow.audit().pruned_eligible_point_count ==
              pruned_overflow_cloud.size(),
      "an exhausted shell result preserves its strict-pruning audit "
      "without publishing a partition");

  check_throws<std::out_of_range>(
      [&equality_index, &equality_cloud, &equality_exclusions,
       &generous_budget] {
        static_cast<void>(lbvh_top_k_budgeted(
            equality_index,
            equality_cloud,
            origin(),
            0U,
            equality_exclusions,
            generous_budget));
      },
      "budgeted LBVH top-k rejects rank zero before traversal");
}

void test_budgeted_top_k_exact_incumbent_seeding() {
  constexpr std::size_t line_point_count = 64U;
  const CanonicalPointCloud line_cloud =
      nonnegative_line_cloud(line_point_count);
  const MortonLbvhIndex line_index =
      MortonLbvhIndex::build(line_cloud);
  const ExclusionSet no_exclusions =
      empty_exclusions(line_cloud);
  const TopKPartition line_reference = brute_force_top_k(
      line_cloud, origin(), 2U, no_exclusions);
  const ExactLbvhTopKBudget generous_budget{
      4096U,
      4096U,
      4096U,
      4096U,
      128U,
      2U,
      2U};

  const ExactBudgetedLbvhTopKResult unseeded =
      lbvh_top_k_budgeted(
          line_index,
          line_cloud,
          origin(),
          2U,
          no_exclusions,
          generous_budget,
          LbvhTraversalOrder::far_first);
  const std::array<PointId, 2> exact_incumbents{
      PointId{0}, PointId{1}};
  const ExactBudgetedLbvhTopKResult well_seeded =
      lbvh_top_k_budgeted(
          line_index,
          line_cloud,
          origin(),
          2U,
          no_exclusions,
          std::span<const PointId>{exact_incumbents},
          generous_budget,
          LbvhTraversalOrder::far_first);
  check_budgeted_top_matches(
      well_seeded,
      line_reference,
      generous_budget,
      "the exactly seeded line query");
  check(
      well_seeded.audit().supplied_incumbent_point_count == 2U &&
          well_seeded.audit()
                  .exact_incumbent_distance_evaluation_count ==
              2U &&
          well_seeded.audit().exact_point_distance_evaluation_count == 2U &&
          well_seeded.partition().query_counters()
                  .pruned_eligible_point_count ==
              line_point_count - 2U,
      "good incumbents are re-evaluated once and initialize exact pruning");
  check(
      well_seeded.audit().node_visit_count <
          unseeded.audit().node_visit_count,
      "good incumbents reduce far-first traversal without changing its certificate");

  ExactLbvhTopKBudget short_incumbent_budget = generous_budget;
  short_incumbent_budget.maximum_exact_point_distance_evaluation_count = 1U;
  const ExactBudgetedLbvhTopKResult short_incumbent =
      lbvh_top_k_budgeted(
          line_index,
          line_cloud,
          origin(),
          2U,
          no_exclusions,
          std::span<const PointId>{exact_incumbents},
          short_incumbent_budget,
          LbvhTraversalOrder::far_first);
  check_budgeted_top_exhaustion(
      short_incumbent,
      short_incumbent_budget,
      ExactLbvhTopKStopReason::exact_point_distance_evaluation_limit,
      "the one-short exact incumbent-distance budget");
  check(
      short_incumbent.audit().supplied_incumbent_point_count == 2U &&
          short_incumbent.audit()
                  .exact_incumbent_distance_evaluation_count ==
              0U &&
          short_incumbent.audit().exact_point_distance_evaluation_count == 0U &&
          short_incumbent.audit().node_visit_count == 0U,
      "an insufficient incumbent-distance cap fails before partial replay");

  const std::array<PointId, 2> adversarial_incumbents{
      PointId{62}, PointId{63}};
  const ExactBudgetedLbvhTopKResult adversarially_seeded =
      lbvh_top_k_budgeted(
          line_index,
          line_cloud,
          origin(),
          2U,
          no_exclusions,
          std::span<const PointId>{adversarial_incumbents},
          generous_budget,
          LbvhTraversalOrder::far_first);
  check_budgeted_top_matches(
      adversarially_seeded,
      line_reference,
      generous_budget,
      "the adversarially seeded line query");
  check(
      adversarially_seeded.audit().supplied_incumbent_point_count == 2U &&
          adversarially_seeded.audit()
                  .exact_incumbent_distance_evaluation_count ==
              2U &&
          adversarially_seeded.audit().provisional_cutoff_decrease_count !=
              0U,
      "adversarial incumbents remain non-authoritative exact heap seeds");

  const std::array<PointId, 1> excluded_ids{PointId{0}};
  const ExclusionSet exclusions = ExclusionSet::from_ids(
      std::span<const PointId>{excluded_ids}, line_cloud, 1U);
  const std::array<PointId, 3> too_many{
      PointId{0}, PointId{1}, PointId{2}};
  const std::array<PointId, 2> repeated{
      PointId{1}, PointId{1}};
  const std::array<PointId, 1> outside{
      static_cast<PointId>(line_point_count)};
  check_throws<std::invalid_argument>(
      [&line_index, &line_cloud, &no_exclusions, &too_many,
       &generous_budget] {
        static_cast<void>(lbvh_top_k_budgeted(
            line_index,
            line_cloud,
            origin(),
            2U,
            no_exclusions,
            std::span<const PointId>{too_many},
            generous_budget));
      },
      "a seeded query rejects more incumbents than its rank");
  check_throws<std::invalid_argument>(
      [&line_index, &line_cloud, &no_exclusions, &repeated,
       &generous_budget] {
        static_cast<void>(lbvh_top_k_budgeted(
            line_index,
            line_cloud,
            origin(),
            2U,
            no_exclusions,
            std::span<const PointId>{repeated},
            generous_budget));
      },
      "a seeded query rejects repeated incumbents");
  check_throws<std::out_of_range>(
      [&line_index, &line_cloud, &no_exclusions, &outside,
       &generous_budget] {
        static_cast<void>(lbvh_top_k_budgeted(
            line_index,
            line_cloud,
            origin(),
            2U,
            no_exclusions,
            std::span<const PointId>{outside},
            generous_budget));
      },
      "a seeded query rejects an incumbent outside the PointId domain");
  check_throws<std::invalid_argument>(
      [&line_index, &line_cloud, &exclusions, &excluded_ids,
       &generous_budget] {
        static_cast<void>(lbvh_top_k_budgeted(
            line_index,
            line_cloud,
            origin(),
            2U,
            exclusions,
            std::span<const PointId>{excluded_ids},
            generous_budget));
      },
      "a seeded query rejects an excluded incumbent");

  const std::array<CertifiedPoint3, 6> equality_input{
      point(-1.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, -1.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.0, 0.0, -1.0),
      point(0.0, 0.0, 1.0)};
  const CanonicalPointCloud equality_cloud =
      canonical_cloud(equality_input);
  const MortonLbvhIndex equality_index =
      MortonLbvhIndex::build(equality_cloud);
  const ExclusionSet equality_exclusions =
      empty_exclusions(equality_cloud);
  const TopKPartition equality_reference = brute_force_top_k(
      equality_cloud, origin(), 3U, equality_exclusions);
  const std::array<PointId, 3> equality_incumbents{
      PointId{0}, PointId{2}, PointId{5}};
  const ExactLbvhTopKBudget equality_budget{
      4096U,
      4096U,
      4096U,
      4096U,
      128U,
      3U,
      6U};
  const ExactBudgetedLbvhTopKResult equality =
      lbvh_top_k_budgeted(
          equality_index,
          equality_cloud,
          origin(),
          3U,
          equality_exclusions,
          std::span<const PointId>{equality_incumbents},
          equality_budget,
          LbvhTraversalOrder::far_first);
  check_budgeted_top_matches(
      equality,
      equality_reference,
      equality_budget,
      "the seeded six-way equality query");
  const std::array<PointId, 1> partial_equality_incumbent{PointId{5}};
  const ExactBudgetedLbvhTopKResult partial_equality =
      lbvh_top_k_budgeted(
          equality_index,
          equality_cloud,
          origin(),
          3U,
          equality_exclusions,
          std::span<const PointId>{partial_equality_incumbent},
          equality_budget,
          LbvhTraversalOrder::far_first);
  check_budgeted_top_matches(
      partial_equality,
      equality_reference,
      equality_budget,
      "the partially seeded six-way equality query");
  check(
      equality.audit().exact_incumbent_distance_evaluation_count == 3U &&
          equality.audit().exact_point_distance_evaluation_count == 6U &&
          equality.partition().cutoff_shell_ids().size() == 6U &&
          equality.partition().query_counters().pruned_subtree_count == 0U &&
          partial_equality.audit().supplied_incumbent_point_count == 1U &&
          partial_equality.audit()
                  .exact_incumbent_distance_evaluation_count ==
              1U &&
          partial_equality.audit().exact_point_distance_evaluation_count ==
              6U &&
          partial_equality.partition().query_counters()
                  .pruned_eligible_point_count ==
              0U,
      "full and partial incumbent heaps preserve every member of an equal cutoff shell");
}

void test_budgeted_top_k_baseline_proposal_union() {
  constexpr std::size_t line_point_count = 64U;
  const CanonicalPointCloud line_cloud =
      nonnegative_line_cloud(line_point_count);
  const MortonLbvhIndex line_index =
      MortonLbvhIndex::build(line_cloud);
  const ExclusionSet no_exclusions =
      empty_exclusions(line_cloud);
  const TopKPartition line_reference = brute_force_top_k(
      line_cloud, origin(), 2U, no_exclusions);
  const ExactLbvhTopKBudget line_budget{
      4096U,
      4096U,
      4096U,
      4096U,
      128U,
      2U,
      2U};
  const std::array<PointId, 2> exact_baseline{
      PointId{0}, PointId{1}};
  const std::array<PointId, 0> empty_proposal{};
  const std::array<PointId, 2> adversarial_proposal{
      PointId{62}, PointId{63}};

  const ExactBudgetedLbvhTopKResult baseline_only =
      lbvh_top_k_budgeted(
          line_index,
          line_cloud,
          origin(),
          2U,
          no_exclusions,
          std::span<const PointId>{exact_baseline},
          std::span<const PointId>{empty_proposal},
          line_budget,
          LbvhTraversalOrder::far_first);
  const ExactBudgetedLbvhTopKResult adversarial_union =
      lbvh_top_k_budgeted(
          line_index,
          line_cloud,
          origin(),
          2U,
          no_exclusions,
          std::span<const PointId>{exact_baseline},
          std::span<const PointId>{adversarial_proposal},
          line_budget,
          LbvhTraversalOrder::far_first);
  check_budgeted_top_matches(
      baseline_only,
      line_reference,
      line_budget,
      "the exact baseline-only query");
  check_budgeted_top_matches(
      adversarial_union,
      line_reference,
      line_budget,
      "the exact baseline plus adversarial proposal query");
  check(
      baseline_only.audit().node_visit_count ==
              adversarial_union.audit().node_visit_count &&
          baseline_only.audit().internal_node_expansion_count ==
              adversarial_union.audit()
                  .internal_node_expansion_count &&
          baseline_only.audit().exact_aabb_bound_evaluation_count ==
              adversarial_union.audit()
                  .exact_aabb_bound_evaluation_count &&
          adversarial_union.audit().supplied_incumbent_point_count == 4U &&
          adversarial_union.audit()
                  .exact_incumbent_distance_evaluation_count ==
              4U &&
          adversarial_union.audit().peak_best_neighbor_entry_count == 2U &&
          adversarial_union.partition().query_counters()
                  .pruned_eligible_point_count ==
              line_point_count - 4U,
      "an adversarial proposal cannot worsen the exact baseline cutoff "
      "or grow the K-neighbor heap");

  const std::array<PointId, 2> overlapping_proposal{
      PointId{1}, PointId{62}};
  const ExactBudgetedLbvhTopKResult overlapping_union =
      lbvh_top_k_budgeted(
          line_index,
          line_cloud,
          origin(),
          2U,
          no_exclusions,
          std::span<const PointId>{exact_baseline},
          std::span<const PointId>{overlapping_proposal},
          line_budget,
          LbvhTraversalOrder::far_first);
  check_budgeted_top_matches(
      overlapping_union,
      line_reference,
      line_budget,
      "the overlapping baseline/proposal union");
  check(
      overlapping_union.audit().supplied_incumbent_point_count == 3U &&
          overlapping_union.audit()
                  .exact_incumbent_distance_evaluation_count ==
              3U,
      "an F/P overlap is evaluated exactly once");

  ExactLbvhTopKBudget short_union_budget = line_budget;
  short_union_budget.maximum_exact_point_distance_evaluation_count = 3U;
  const ExactBudgetedLbvhTopKResult short_union =
      lbvh_top_k_budgeted(
          line_index,
          line_cloud,
          origin(),
          2U,
          no_exclusions,
          std::span<const PointId>{exact_baseline},
          std::span<const PointId>{adversarial_proposal},
          short_union_budget,
          LbvhTraversalOrder::far_first);
  check_budgeted_top_exhaustion(
      short_union,
      short_union_budget,
      ExactLbvhTopKStopReason::exact_point_distance_evaluation_limit,
      "the one-short F-union-P exact-distance budget");
  check(
      short_union.audit().supplied_incumbent_point_count == 4U &&
          short_union.audit()
                  .exact_incumbent_distance_evaluation_count ==
              0U &&
          short_union.audit().exact_point_distance_evaluation_count == 0U &&
          short_union.audit().node_visit_count == 0U,
      "an F-union-P cap below its distinct cardinality fails before "
      "partial evaluation");

  const std::array<PointId, 1> short_baseline{PointId{0}};
  const std::array<PointId, 2> repeated_baseline{
      PointId{0}, PointId{0}};
  const std::array<PointId, 2> repeated_proposal{
      PointId{62}, PointId{62}};
  const std::array<PointId, 3> oversized_proposal{
      PointId{2}, PointId{3}, PointId{4}};
  check_throws<std::invalid_argument>(
      [&line_index, &line_cloud, &no_exclusions, &short_baseline,
       &empty_proposal, &line_budget] {
        static_cast<void>(lbvh_top_k_budgeted(
            line_index,
            line_cloud,
            origin(),
            2U,
            no_exclusions,
            std::span<const PointId>{short_baseline},
            std::span<const PointId>{empty_proposal},
            line_budget));
      },
      "an exact baseline shorter than K is rejected");
  check_throws<std::invalid_argument>(
      [&line_index, &line_cloud, &no_exclusions, &repeated_baseline,
       &empty_proposal, &line_budget] {
        static_cast<void>(lbvh_top_k_budgeted(
            line_index,
            line_cloud,
            origin(),
            2U,
            no_exclusions,
            std::span<const PointId>{repeated_baseline},
            std::span<const PointId>{empty_proposal},
            line_budget));
      },
      "a repeated exact-baseline PointId is rejected");
  check_throws<std::invalid_argument>(
      [&line_index, &line_cloud, &no_exclusions, &exact_baseline,
       &repeated_proposal, &line_budget] {
        static_cast<void>(lbvh_top_k_budgeted(
            line_index,
            line_cloud,
            origin(),
            2U,
            no_exclusions,
            std::span<const PointId>{exact_baseline},
            std::span<const PointId>{repeated_proposal},
            line_budget));
      },
      "a repeated proposal PointId is rejected");
  check_throws<std::invalid_argument>(
      [&line_index, &line_cloud, &no_exclusions, &exact_baseline,
       &oversized_proposal, &line_budget] {
        static_cast<void>(lbvh_top_k_budgeted(
            line_index,
            line_cloud,
            origin(),
            2U,
            no_exclusions,
            std::span<const PointId>{exact_baseline},
            std::span<const PointId>{oversized_proposal},
            line_budget));
      },
      "a proposal longer than K is rejected");

  const std::array<CertifiedPoint3, 6> equality_input{
      point(-1.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, -1.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.0, 0.0, -1.0),
      point(0.0, 0.0, 1.0)};
  const CanonicalPointCloud equality_cloud =
      canonical_cloud(equality_input);
  const MortonLbvhIndex equality_index =
      MortonLbvhIndex::build(equality_cloud);
  const ExclusionSet equality_exclusions =
      empty_exclusions(equality_cloud);
  const TopKPartition equality_reference = brute_force_top_k(
      equality_cloud, origin(), 3U, equality_exclusions);
  const std::array<PointId, 3> equality_baseline{
      PointId{0}, PointId{1}, PointId{2}};
  const std::array<PointId, 3> equality_proposal{
      PointId{3}, PointId{4}, PointId{5}};
  const ExactLbvhTopKBudget equality_budget{
      4096U,
      4096U,
      4096U,
      4096U,
      128U,
      3U,
      6U};
  const ExactBudgetedLbvhTopKResult equality_union =
      lbvh_top_k_budgeted(
          equality_index,
          equality_cloud,
          origin(),
          3U,
          equality_exclusions,
          std::span<const PointId>{equality_baseline},
          std::span<const PointId>{equality_proposal},
          equality_budget,
          LbvhTraversalOrder::far_first);
  check_budgeted_top_matches(
      equality_union,
      equality_reference,
      equality_budget,
      "the six-way F-union-P equality query");
  check(
      equality_union.audit().supplied_incumbent_point_count == 6U &&
          equality_union.audit()
                  .exact_incumbent_distance_evaluation_count ==
              6U &&
          equality_union.audit().peak_best_neighbor_entry_count == 3U &&
          equality_union.partition().cutoff_shell_ids().size() == 6U &&
          equality_union.partition().query_counters()
                  .pruned_subtree_count ==
              0U,
      "the initial F-union-P heap retains every member of its exact cutoff shell");
}

void test_certified_snapshot_import_and_hostile_rejections() {
  static_assert(sizeof(MortonLeafRecord) == 16U);
  static_assert(sizeof(MortonLbvhSnapshotNode) == 80U);

  const std::array<CertifiedPoint3, 1> singleton_input{
      point(-0.0, 2.0, -3.0)};
  const CanonicalPointCloud singleton =
      canonical_cloud(singleton_input);
  check_import_matches_build(
      singleton, "the singleton snapshot import");

  const double tiny = std::numeric_limits<double>::denorm_min();
  const double maximum = std::numeric_limits<double>::max();
  const std::array<CertifiedPoint3, 6> extreme_input{
      point(-maximum, 2.0, -3.0),
      point(-1.0, 2.0, -3.0),
      point(-tiny, 2.0, -3.0),
      point(0.0, 2.0, -3.0),
      point(tiny, 2.0, -3.0),
      point(maximum, 2.0, -3.0)};
  const CanonicalPointCloud cloud = canonical_cloud(extreme_input);
  check_import_matches_build(
      cloud, "the extreme collision snapshot import");

  const MortonLbvhIndex reference = MortonLbvhIndex::build(cloud);
  const MortonLbvhSnapshot valid =
      snapshot_from_index(cloud, reference);
  const auto reject =
      [&cloud](
          MortonLbvhSnapshot corrupted,
          const std::string& message) {
        check_throws<std::invalid_argument>(
            [&cloud, &corrupted] {
              static_cast<void>(
                  MortonLbvhIndex::import_certified_snapshot(
                      cloud, corrupted));
            },
            message);
      };

  MortonLbvhSnapshot corrupted = valid;
  ++corrupted.schema_version;
  reject(
      std::move(corrupted),
      "snapshot import rejects another schema");

  corrupted = valid;
  --corrupted.morton_bits_per_axis;
  reject(
      std::move(corrupted),
      "snapshot import rejects another Morton grid");

  corrupted = valid;
  --corrupted.point_count;
  reject(
      std::move(corrupted),
      "snapshot import rejects a wrong point count");

  corrupted = valid;
  corrupted.leaves.pop_back();
  reject(
      std::move(corrupted),
      "snapshot import rejects a missing leaf");

  corrupted = valid;
  corrupted.nodes.pop_back();
  reject(
      std::move(corrupted),
      "snapshot import rejects a missing node");

  corrupted = valid;
  corrupted.leaves.front().morton_code ^= 1U;
  reject(
      std::move(corrupted),
      "snapshot import rejects one corrupt exact Morton bin");

  corrupted = valid;
  corrupted.leaves[1U].point_id =
      corrupted.leaves.front().point_id;
  reject(
      std::move(corrupted),
      "snapshot import rejects a repeated PointId");

  corrupted = valid;
  std::swap(corrupted.leaves[0U], corrupted.leaves[1U]);
  reject(
      std::move(corrupted),
      "snapshot import rejects noncanonical leaf order");

  const std::size_t root_index =
      static_cast<std::size_t>(valid.root_node_index);
  const std::size_t root_left = static_cast<std::size_t>(
      valid.nodes[root_index].left_child);

  corrupted = valid;
  ++corrupted.nodes[root_left].leaf_end;
  reject(
      std::move(corrupted),
      "snapshot import rejects a corrupt derived split range");

  corrupted = valid;
  corrupted.nodes[root_index].left_child =
      corrupted.nodes[root_index].right_child;
  reject(
      std::move(corrupted),
      "snapshot import rejects a repeated child");

  corrupted = valid;
  corrupted.nodes[root_index].left_child =
      corrupted.root_node_index;
  reject(
      std::move(corrupted),
      "snapshot import rejects a cyclic child");

  corrupted = valid;
  corrupted.nodes[root_index].lower_point_ids[0U] =
      corrupted.nodes[root_index].upper_point_ids[0U];
  reject(
      std::move(corrupted),
      "snapshot import rejects a corrupt exact AABB witness");

  const auto leaf_position = std::find_if(
      valid.nodes.begin(),
      valid.nodes.end(),
      [](const MortonLbvhSnapshotNode& node) {
        return node.is_leaf();
      });
  check(
      leaf_position != valid.nodes.end(),
      "the hostile snapshot fixture contains a leaf node");
  if (leaf_position != valid.nodes.end()) {
    corrupted = valid;
    const std::size_t leaf_index = static_cast<std::size_t>(
        leaf_position - valid.nodes.begin());
    corrupted.nodes[leaf_index].left_child = 0U;
    reject(
        std::move(corrupted),
        "snapshot import rejects a child on a leaf");
  }

  corrupted = valid;
  std::swap(corrupted.nodes[0U], corrupted.nodes[1U]);
  reject(
      std::move(corrupted),
      "snapshot import rejects non-postorder node numbering");

  corrupted = valid;
  --corrupted.root_node_index;
  reject(
      std::move(corrupted),
      "snapshot import rejects a nonterminal root");

  corrupted = valid;
  corrupted.root_aabb.lower_binary64_bits[0U] ^= 1U;
  reject(
      std::move(corrupted),
      "snapshot import rejects a corrupt root AABB");

  corrupted = valid;
  ++corrupted.proposed_counters.maximum_depth;
  reject(
      std::move(corrupted),
      "snapshot import rejects a corrupt proposed depth");

  corrupted = valid;
  ++corrupted.proposed_counters.morton_collision_group_count;
  reject(
      std::move(corrupted),
      "snapshot import rejects corrupt collision counters");

  std::array<CertifiedPoint3, 6> changed_input = extreme_input;
  changed_input[5U] = point(maximum / 2.0, 2.0, -3.0);
  const CanonicalPointCloud changed_cloud =
      canonical_cloud(changed_input);
  check_throws<std::invalid_argument>(
      [&changed_cloud, &valid] {
        static_cast<void>(
            MortonLbvhIndex::import_certified_snapshot(
                changed_cloud, valid));
      },
      "snapshot import rejects another same-size cloud identity");

  const ExclusionSet exclusions = empty_exclusions(cloud);
  const TopKPartition after_rejections = lbvh_top_k(
      reference,
      cloud,
      origin(),
      3U,
      exclusions,
      LbvhTraversalOrder::near_first);
  const TopKPartition fresh = lbvh_top_k(
      MortonLbvhIndex::import_certified_snapshot(cloud, valid),
      cloud,
      origin(),
      3U,
      exclusions,
      LbvhTraversalOrder::near_first);
  check_top_matches(
      after_rejections,
      fresh,
      "hostile imports leave the reference index atomic");
}

void test_namespace_binding_and_move_fail_closed() {
  const std::array<CertifiedPoint3, 4> input{
      point(-2.0, 0.0, 1.0),
      point(-1.0, 1.0, 0.0),
      point(1.0, -1.0, 0.0),
      point(2.0, 0.0, -1.0)};
  CanonicalPointCloud cloud = canonical_cloud(input);
  const CanonicalPointCloud copied_cloud = cloud;
  const CanonicalPointCloud unrelated_cloud = canonical_cloud(input);
  const ExclusionSet exclusions = empty_exclusions(cloud);
  const ExclusionSet unrelated_exclusions =
      empty_exclusions(unrelated_cloud);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);

  check(index.validated_for(copied_cloud),
        "a copied cloud preserves the LBVH PointId namespace token");
  check(!index.validated_for(unrelated_cloud),
        "equal geometry does not merge independently built namespaces");
  const auto copied_result =
      lbvh_nearest(index, copied_cloud, origin(), exclusions);
  check(
      copied_result.validated_for(cloud) &&
          copied_result.validated_for(copied_cloud) &&
          !copied_result.validated_for(unrelated_cloud),
      "an LBVH query certificate is bound to the shared cloud namespace");
  check_throws<std::invalid_argument>(
      [&index, &unrelated_cloud, &unrelated_exclusions] {
        static_cast<void>(lbvh_nearest(
            index,
            unrelated_cloud,
            origin(),
            unrelated_exclusions));
      },
      "an LBVH cannot be replayed against equal geometry in another namespace");
  check_throws<std::invalid_argument>(
      [&index, &cloud, &unrelated_exclusions] {
        static_cast<void>(lbvh_nearest(
            index, cloud, origin(), unrelated_exclusions));
      },
      "an unrelated exclusion namespace cannot alter an LBVH query");

  MortonLbvhIndex moved_index = std::move(index);
  check(
      !index.ready() && !index.validated_for(cloud) && index.leaves().empty(),
      "a moved-from LBVH is explicitly invalid and exposes no leaves");
  check_throws<std::logic_error>(
      [&index] { static_cast<void>(index.build_counters()); },
      "a moved-from LBVH exposes no stale build counters");
  check_throws<std::logic_error>(
      [&index] { static_cast<void>(index.root_aabb()); },
      "a moved-from LBVH exposes no stale root AABB");
  check_throws<std::invalid_argument>(
      [&index, &cloud, &exclusions] {
        static_cast<void>(
            lbvh_nearest(index, cloud, origin(), exclusions));
      },
      "a moved-from LBVH cannot publish a query certificate");
  check(moved_index.ready() && moved_index.validated_for(cloud),
        "the LBVH move target retains its complete namespace-bound tree");
  self_move_assign(moved_index);
  check(moved_index.ready() && moved_index.validated_for(cloud),
        "LBVH self-move assignment preserves a valid index");

  MortonLbvhIndex assigned_index =
      MortonLbvhIndex::build(unrelated_cloud);
  assigned_index = std::move(moved_index);
  check(
      assigned_index.ready() && assigned_index.validated_for(cloud) &&
          !moved_index.ready(),
      "LBVH move assignment transactionally replaces the target tree");

  CanonicalPointCloud moved_cloud = std::move(cloud);
  check(
      cloud.size() == 0U && !assigned_index.validated_for(cloud) &&
          assigned_index.validated_for(moved_cloud),
      "moving a cloud transfers the namespace recognized by its LBVH");
  const auto moved_cloud_result =
      lbvh_nearest(assigned_index, moved_cloud, origin(), exclusions);
  check(moved_cloud_result.validated_for(moved_cloud),
        "the transferred cloud and pre-move exclusions remain queryable");
  check_throws<std::invalid_argument>(
      [&assigned_index, &cloud, &exclusions] {
        static_cast<void>(lbvh_nearest(
            assigned_index, cloud, origin(), exclusions));
      },
      "a moved-from cloud cannot be queried through its former LBVH");
  check_throws<std::invalid_argument>(
      [&cloud] {
        static_cast<void>(MortonLbvhIndex::build(cloud));
      },
      "a moved-from cloud cannot build an empty LBVH");
}

}  // namespace

int main() {
  test_singleton_build_and_boundary_queries();
  test_morton_collisions_and_permutation_determinism();
  test_finite_extrema_remain_exact();
  test_bound_equality_never_prunes_complete_shells();
  test_strict_pruning_bulk_classification_and_exclusions();
  test_budgeted_top_k_exactness_and_fail_closed_limits();
  test_budgeted_top_k_exact_incumbent_seeding();
  test_budgeted_top_k_baseline_proposal_union();
  test_certified_snapshot_import_and_hostile_rejections();
  test_namespace_binding_and_move_fail_closed();

  if (failures != 0) {
    std::cerr << failures << " spatial Morton-LBVH test(s) failed\n";
    return 1;
  }
  std::cout << "MorseHGP3D spatial Morton-LBVH tests passed\n";
  return 0;
}
