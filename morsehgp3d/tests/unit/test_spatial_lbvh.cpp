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
using morsehgp3d::spatial::ExclusionSet;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
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

static_assert(!std::is_copy_constructible_v<MortonLbvhIndex>);
static_assert(!std::is_copy_assignable_v<MortonLbvhIndex>);
static_assert(std::is_nothrow_move_constructible_v<MortonLbvhIndex>);
static_assert(std::is_nothrow_move_assignable_v<MortonLbvhIndex>);

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
  test_namespace_binding_and_move_fail_closed();

  if (failures != 0) {
    std::cerr << failures << " spatial Morton-LBVH test(s) failed\n";
    return 1;
  }
  std::cout << "MorseHGP3D spatial Morton-LBVH tests passed\n";
  return 0;
}
