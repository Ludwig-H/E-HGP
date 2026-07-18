#include "fake_gpu_spatial_bounds_launchers.hpp"

#include "morsehgp3d/gpu/spatial_lbvh.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <initializer_list>
#include <iostream>
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
using morsehgp3d::gpu::DirectedEnclosureStatus;
using morsehgp3d::gpu::SpatialLbvhAudit;
using morsehgp3d::gpu::SpatialLbvhClosedBallResult;
using morsehgp3d::gpu::SpatialLbvhContext;
using morsehgp3d::gpu::SpatialLbvhTopKResult;
using morsehgp3d::gpu::test_support::
    FakeSpatialBoundsProposalConfiguration;
using morsehgp3d::gpu::test_support::
    FakeSpatialBoundsProposalCorruption;
using morsehgp3d::gpu::test_support::
    FakeSpatialBoundsProposalPermutation;
using morsehgp3d::gpu::test_support::FakeSpatialBoundsProposalValues;
using morsehgp3d::gpu::test_support::configure_fake_gpu_spatial_bounds;
using morsehgp3d::gpu::test_support::fake_gpu_spatial_bounds_last_box_count;
using morsehgp3d::gpu::test_support::
    fake_gpu_spatial_bounds_last_cutoff_bits;
using morsehgp3d::gpu::test_support::
    fake_gpu_spatial_bounds_last_query_lower_bits;
using morsehgp3d::gpu::test_support::
    fake_gpu_spatial_bounds_last_query_upper_bits;
using morsehgp3d::gpu::test_support::fake_gpu_spatial_bounds_launch_count;
using morsehgp3d::gpu::test_support::reset_fake_gpu_spatial_bounds;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ClosedBallPartition;
using morsehgp3d::spatial::ExclusionSet;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;
using morsehgp3d::spatial::SpatialQueryMethod;
using morsehgp3d::spatial::TopKPartition;

static_assert(!std::is_copy_constructible_v<SpatialLbvhContext>);
static_assert(!std::is_copy_assignable_v<SpatialLbvhContext>);
static_assert(std::is_nothrow_move_constructible_v<SpatialLbvhContext>);
static_assert(std::is_nothrow_move_assignable_v<SpatialLbvhContext>);

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
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message
              << " (unexpected exception: " << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CertifiedPoint3 point(
    double x, double y = 0.0, double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactRational3 origin() {
  return ExactRational3{};
}

[[nodiscard]] ExactRational3 unit_x() {
  return ExactRational3{BigInt{1}, BigInt{0}, BigInt{0}, BigInt{1}};
}

template <std::size_t Size>
[[nodiscard]] ExclusionSet exclusion_set(
    const CanonicalPointCloud& cloud,
    const std::array<PointId, Size>& ids,
    std::size_t run_m_star = Size) {
  return ExclusionSet::from_ids(
      std::span<const PointId>{ids}, cloud, run_m_star);
}

[[nodiscard]] ExclusionSet empty_exclusions(
    const CanonicalPointCloud& cloud) {
  return exclusion_set(cloud, std::array<PointId, 0>{});
}

template <typename Range>
[[nodiscard]] std::vector<PointId> materialize_ids(const Range& range) {
  return std::vector<PointId>{range.begin(), range.end()};
}

[[nodiscard]] std::vector<PointId> materialize_neighbor_ids(
    std::span<const morsehgp3d::spatial::ExactNeighbor> neighbors) {
  std::vector<PointId> ids;
  ids.reserve(neighbors.size());
  for (const morsehgp3d::spatial::ExactNeighbor& neighbor : neighbors) {
    ids.push_back(neighbor.point_id);
  }
  return ids;
}

template <typename Range>
void check_ids(
    const Range& actual,
    std::initializer_list<PointId> expected,
    const std::string& message) {
  check(
      materialize_ids(actual) == std::vector<PointId>{expected},
      message);
}

void check_unit_ball_partition(
    const ClosedBallPartition& partition,
    const CanonicalPointCloud& cloud,
    const std::string& label) {
  check(
      partition.validated_for(cloud) && partition.partition_complete(),
      label + " is a complete certificate in the expected namespace");
  check(
      partition.squared_radius() == ExactLevel{BigInt{1}},
      label + " preserves the exact squared radius");
  check_ids(
      partition.interior_ids(), {PointId{0}},
      label + " classifies x=0 in the strict interior");
  check_ids(
      partition.shell_ids(), {PointId{1}},
      label + " preserves x=1 on the closed shell");
  check_ids(
      partition.exterior_ids(), {PointId{2}},
      label + " classifies x=2 in the strict exterior");
  check(
      partition.closed_rank() == 2U && partition.evaluation_count() == 3U,
      label + " reports the global closed rank and population");
}

void check_supported_audit(
    const SpatialLbvhAudit& audit,
    std::size_t point_count,
    std::size_t candidate_count,
    std::size_t exact_candidate_distance_count,
    std::size_t pruned_point_count,
    std::size_t pruned_subtree_count,
    std::uint64_t epoch,
    const std::string& label) {
  const std::size_t node_count = 2U * point_count - 1U;
  check(
      audit.resident_node_count == node_count &&
          audit.resident_point_count == point_count &&
          audit.gpu_output_capacity == node_count,
      label + " preserves the resident tree extents");
  check(
      audit.gpu_output_cover_record_count ==
              audit.gpu_prune_proposal_count +
                  audit.gpu_candidate_leaf_count &&
          audit.gpu_candidate_leaf_count == candidate_count &&
          audit.gpu_prune_proposal_count == pruned_subtree_count,
      label + " closes the terminal GPU cover records");
  check(
      audit.gpu_launch_count == 1U && audit.buffer_epoch == epoch &&
          audit.unsupported_range_fallback_count == 0U,
      label + " reports one supported resident traversal");
  check(
      audit.gpu_kernel_launch_count == audit.gpu_traversal_round_count &&
          audit.gpu_kernel_launch_count > 0U &&
          audit.gpu_processed_node_count == audit.traversed_node_count,
      label + " accounts for every frontier round and processed node");
  check(
      audit.gpu_peak_frontier_count > 0U &&
          audit.gpu_peak_frontier_count <=
              audit.gpu_processed_node_count &&
          audit.gpu_parallel_round_count <=
              audit.gpu_traversal_round_count &&
          ((audit.gpu_peak_frontier_count > 1U) ==
           (audit.gpu_parallel_round_count > 0U)),
      label + " reports coherent bounded parallel-frontier metadata");
  check(
      audit.traversed_node_count ==
              1U + 2U * audit.internal_node_expansion_count &&
          audit.cpu_exact_aabb_bound_evaluation_count ==
              audit.traversed_node_count,
      label + " rebuilds one full binary host cover");
  check(
      audit.cpu_exact_prune_recertification_count ==
              pruned_subtree_count &&
          audit.certified_pruned_subtree_count == pruned_subtree_count &&
          audit.certified_pruned_point_count == pruned_point_count,
      label + " recertifies every retained prune exactly");
  check(
      audit.candidate_point_count == candidate_count &&
          audit.cpu_exact_candidate_distance_evaluation_count ==
              exact_candidate_distance_count &&
          candidate_count + pruned_point_count == point_count,
      label + " partitions exact point work into candidates and prunes");
  check(
      audit.cover_antichain_complete && audit.point_partition_complete &&
          audit.cpu_exact_recertification_complete &&
          audit.exact_partition_complete,
      label + " publishes only a fully closed exact partition");
  check(
      audit.minimum_certified_strict_margin.has_value() ==
          (pruned_subtree_count != 0U),
      label + " binds a strict margin exactly to certified pruning");
  if (audit.minimum_certified_strict_margin.has_value()) {
    check(
        *audit.minimum_certified_strict_margin > ExactLevel{},
        label + " exposes a positive exact pruning margin");
  }
  check(
      audit.query_enclosure ==
              std::array<DirectedEnclosureStatus, 3>{
                  DirectedEnclosureStatus::exact,
                  DirectedEnclosureStatus::exact,
                  DirectedEnclosureStatus::exact} &&
          audit.cutoff_enclosure == DirectedEnclosureStatus::exact,
      label + " uses exact binary64 enclosures for the unit query");
}

void check_top_k_audit_closure(
    const SpatialLbvhTopKResult& result,
    const std::string& label) {
  const SpatialLbvhAudit& audit = result.audit;
  const TopKPartition& partition = result.exact_partition;
  check(
      audit.cpu_exact_seed_distance_evaluation_count <=
          audit.cpu_exact_candidate_distance_evaluation_count,
      label + " treats exact seeds as a subset of exact candidates");
  check(
      audit.cpu_exact_candidate_distance_evaluation_count +
              audit.excluded_candidate_point_count ==
          audit.candidate_point_count,
      label + " splits structural candidates into exact and excluded IDs");
  check(
      audit.cpu_exact_candidate_distance_evaluation_count +
              audit.certified_pruned_eligible_point_count ==
          partition.eligible_point_count(),
      label + " partitions every eligible ID into exact or pruned work");
  check(
      audit.cpu_exact_candidate_distance_evaluation_count ==
              partition.distance_evaluation_count() &&
          audit.certified_pruned_eligible_point_count ==
              partition.query_counters().pruned_eligible_point_count,
      label + " agrees with the published scientific counters");
}

void test_singleton_context_and_exact_partition() {
  reset_fake_gpu_spatial_bounds();
  const std::array<CertifiedPoint3, 1> points{point(0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  SpatialLbvhContext context{index, cloud};

  check(
      context.node_count() == 1U && context.point_count() == 1U,
      "a singleton context snapshots its one resident leaf");
  const SpatialLbvhClosedBallResult result =
      context.closed_ball(cloud, origin(), ExactLevel{BigInt{1}});
  check_ids(
      result.exact_partition.interior_ids(), {PointId{0}},
      "the singleton origin lies inside the exact unit ball");
  check_ids(
      result.exact_partition.shell_ids(), {},
      "the singleton unit ball has an empty shell");
  check_ids(
      result.exact_partition.exterior_ids(), {},
      "the singleton unit ball has an empty exterior");
  check(
      result.exact_partition.validated_for(cloud) &&
          result.exact_partition.closed_rank() == 1U &&
          result.exact_partition.evaluation_count() == 1U &&
          result.exact_partition.distance_evaluation_count() == 1U,
      "the singleton result is one complete exact evaluation");
  check_supported_audit(
      result.audit, 1U, 1U, 1U, 0U, 0U, 1U,
      "the singleton spatial-LBVH audit");
  check(
      result.exact_partition.query_counters().method ==
              SpatialQueryMethod::morton_lbvh &&
          fake_gpu_spatial_bounds_launch_count() == 1U &&
          fake_gpu_spatial_bounds_last_box_count() == 1U,
      "the singleton query executes one resident LBVH launch");
}

void test_unit_ball_pruning_permutation_and_unknown_fallback() {
  reset_fake_gpu_spatial_bounds();
  const std::array<CertifiedPoint3, 3> points{
      point(2.0), point(0.0), point(1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  SpatialLbvhContext context{index, cloud};
  const ExactLevel radius{BigInt{1}};

  const SpatialLbvhClosedBallResult canonical =
      context.closed_ball(cloud, origin(), radius);
  check_unit_ball_partition(
      canonical.exact_partition, cloud, "the canonical GPU cover");
  check_supported_audit(
      canonical.audit, 3U, 2U, 2U, 1U, 1U, 1U,
      "the canonical GPU cover audit");
  check(
      canonical.audit.minimum_certified_strict_margin ==
          ExactLevel{BigInt{3}},
      "the x=2 leaf is pruned with exact squared margin three");
  check(
      canonical.exact_partition.distance_evaluation_count() == 2U &&
          canonical.exact_partition.query_counters()
                  .pruned_eligible_point_count == 1U,
      "the canonical cover evaluates {0,1} and certifies x=2 outside");

  configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{
      FakeSpatialBoundsProposalPermutation::reversed,
      FakeSpatialBoundsProposalValues::actual_interval_recipe,
      FakeSpatialBoundsProposalCorruption::none});
  const SpatialLbvhClosedBallResult reversed =
      context.closed_ball(cloud, origin(), radius);
  check_unit_ball_partition(
      reversed.exact_partition, cloud, "the reversed GPU cover");
  check_supported_audit(
      reversed.audit, 3U, 2U, 2U, 1U, 1U, 2U,
      "the reversed GPU cover audit");
  check(
      reversed.audit.proposal_digest_fnv1a ==
          canonical.audit.proposal_digest_fnv1a,
      "reversing terminal copy-back order preserves the canonical digest");

  configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{
      FakeSpatialBoundsProposalPermutation::canonical,
      FakeSpatialBoundsProposalValues::all_unknown,
      FakeSpatialBoundsProposalCorruption::none});
  const SpatialLbvhClosedBallResult unknown =
      context.closed_ball(cloud, origin(), radius);
  check_unit_ball_partition(
      unknown.exact_partition, cloud, "the all-unknown GPU cover");
  check_supported_audit(
      unknown.audit, 3U, 3U, 3U, 0U, 0U, 3U,
      "the all-unknown GPU cover audit");
  check(
      unknown.exact_partition.distance_evaluation_count() == 3U &&
          unknown.exact_partition.query_counters()
                  .pruned_eligible_point_count == 0U,
      "unknown proposals descend to and evaluate every leaf exactly");
  check(
      fake_gpu_spatial_bounds_launch_count() == 3U &&
          fake_gpu_spatial_bounds_last_box_count() == 5U &&
          fake_gpu_spatial_bounds_last_query_lower_bits() ==
              std::array<std::uint64_t, 3>{0U, 0U, 0U} &&
          fake_gpu_spatial_bounds_last_query_upper_bits() ==
              std::array<std::uint64_t, 3>{0U, 0U, 0U} &&
          fake_gpu_spatial_bounds_last_cutoff_bits() ==
              std::array<std::uint64_t, 2>{
                  std::bit_cast<std::uint64_t>(1.0),
                  std::bit_cast<std::uint64_t>(1.0)},
      "all supported variants reuse one five-node resident snapshot");
}

void test_top_k_seed_shell_and_exclusion_accounting() {
  reset_fake_gpu_spatial_bounds();
  const std::array<CertifiedPoint3, 3> points{
      point(0.0), point(1.0), point(2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExclusionSet no_exclusions = empty_exclusions(cloud);
  SpatialLbvhContext context{index, cloud};

  const SpatialLbvhTopKResult pruned_rank_two =
      context.top_k(cloud, origin(), 2U, no_exclusions);
  const TopKPartition& pruned_partition = pruned_rank_two.exact_partition;
  check(
      pruned_partition.validated_for(cloud) &&
          pruned_partition.shell_complete() &&
          pruned_partition.requested_rank() == 2U &&
          pruned_partition.eligible_point_count() == 3U,
      "rank two publishes a complete exact partition in its namespace");
  check(
      pruned_partition.cutoff_squared_distance() ==
          ExactLevel{BigInt{1}},
      "rank two at the origin has exact squared cutoff one");
  check(
      materialize_neighbor_ids(pruned_partition.strict_below()) ==
              std::vector<PointId>{PointId{0}} &&
          pruned_partition.strict_below().front().squared_distance ==
              ExactLevel{},
      "rank two keeps x=0 strictly below its cutoff");
  check_ids(
      pruned_partition.cutoff_shell_ids(), {PointId{1}},
      "rank two retains the complete cutoff shell at x=1");
  check_ids(
      pruned_partition.canonical_choice_ids(),
      {PointId{0}, PointId{1}},
      "rank two exposes the canonical two-point choice");
  check_supported_audit(
      pruned_rank_two.audit, 3U, 2U, 2U, 1U, 1U, 1U,
      "the seeded rank-two audit");
  check(
      pruned_rank_two.audit.cpu_exact_seed_distance_evaluation_count == 2U &&
          pruned_rank_two.audit.top_k_seed_squared_cutoff ==
              ExactLevel{BigInt{1}} &&
          pruned_rank_two.audit.excluded_candidate_point_count == 0U &&
          pruned_rank_two.audit.certified_pruned_eligible_point_count == 1U,
      "rank two audits both exact seeds and its eligible strict prune");
  check_top_k_audit_closure(
      pruned_rank_two, "the seeded rank-two audit");
  const auto& pruned_counters = pruned_partition.query_counters();
  check(
      pruned_counters.method == SpatialQueryMethod::morton_lbvh &&
          pruned_counters.excluded_point_count == 0U &&
          pruned_counters.exact_point_distance_evaluation_count == 2U &&
          pruned_counters.pruned_eligible_point_count == 1U &&
          pruned_counters.exact_point_distance_evaluation_count +
                  pruned_counters.pruned_eligible_point_count ==
              pruned_partition.eligible_point_count(),
      "rank-two counters partition every eligible point exactly once");

  const SpatialLbvhTopKResult tied_rank_two =
      context.top_k(cloud, unit_x(), 2U, no_exclusions);
  const TopKPartition& tied_partition = tied_rank_two.exact_partition;
  check(
      tied_partition.cutoff_squared_distance() == ExactLevel{BigInt{1}} &&
          materialize_neighbor_ids(tied_partition.strict_below()) ==
              std::vector<PointId>{PointId{1}} &&
          tied_partition.strict_below().front().squared_distance ==
              ExactLevel{},
      "the centered rank-two query keeps only x=1 strictly below");
  check_ids(
      tied_partition.cutoff_shell_ids(), {PointId{0}, PointId{2}},
      "equality descends and returns both members of the final shell");
  check_ids(
      tied_partition.canonical_choice_ids(),
      {PointId{0}, PointId{1}},
      "the complete tie shell yields a deterministic canonical choice");
  check_supported_audit(
      tied_rank_two.audit, 3U, 3U, 3U, 0U, 0U, 2U,
      "the tied rank-two audit");
  check(
      tied_rank_two.audit.cpu_exact_seed_distance_evaluation_count == 2U &&
          tied_rank_two.audit.certified_pruned_eligible_point_count == 0U &&
          tied_partition.distance_evaluation_count() == 3U,
      "shell equality forces exact evaluation of the third non-seed leaf");
  check_top_k_audit_closure(
      tied_rank_two, "the tied rank-two audit");

  const std::array<PointId, 1> excluded_ids{PointId{0}};
  const ExclusionSet exclusions = exclusion_set(cloud, excluded_ids);
  const SpatialLbvhTopKResult excluded_nearest =
      context.nearest(cloud, origin(), exclusions);
  const TopKPartition& excluded_partition =
      excluded_nearest.exact_partition;
  check(
      excluded_partition.requested_rank() == 1U &&
          excluded_partition.eligible_point_count() == 2U &&
          excluded_partition.cutoff_squared_distance() ==
              ExactLevel{BigInt{1}} &&
          excluded_partition.strict_below().empty(),
      "nearest skips excluded x=0 when selecting its exact seed");
  check_ids(
      excluded_partition.cutoff_shell_ids(), {PointId{1}},
      "nearest returns the complete eligible shell at x=1");
  check_ids(
      excluded_partition.canonical_choice_ids(), {PointId{1}},
      "nearest chooses the only eligible cutoff-shell point");
  check_supported_audit(
      excluded_nearest.audit, 3U, 2U, 1U, 1U, 1U, 3U,
      "the excluded-nearest audit");
  check(
      excluded_nearest.audit.cpu_exact_seed_distance_evaluation_count == 1U &&
          excluded_nearest.audit.top_k_seed_squared_cutoff ==
              ExactLevel{BigInt{1}} &&
          excluded_nearest.audit.excluded_candidate_point_count == 1U &&
          excluded_nearest.audit.certified_pruned_eligible_point_count == 1U,
      "nearest separates its excluded candidate from its eligible prune");
  check_top_k_audit_closure(
      excluded_nearest, "the excluded-nearest audit");
  const auto& excluded_counters = excluded_partition.query_counters();
  check(
      excluded_counters.excluded_point_count == 1U &&
          excluded_counters.exact_point_distance_evaluation_count == 1U &&
          excluded_counters.pruned_eligible_point_count == 1U &&
          excluded_counters.exact_point_distance_evaluation_count +
                  excluded_counters.pruned_eligible_point_count ==
              excluded_partition.eligible_point_count(),
      "nearest counters omit the exclusion and close the eligible population");
  const SpatialLbvhTopKResult hierarchical_nearest =
      context.nearest(cloud, origin(), no_exclusions);
  const TopKPartition& hierarchical_partition =
      hierarchical_nearest.exact_partition;
  check(
      hierarchical_partition.requested_rank() == 1U &&
          hierarchical_partition.eligible_point_count() == 3U &&
          hierarchical_partition.cutoff_squared_distance() == ExactLevel{} &&
          hierarchical_partition.strict_below().empty(),
      "nearest at the origin uses x=0 as its exact zero-distance seed");
  check_ids(
      hierarchical_partition.cutoff_shell_ids(), {PointId{0}},
      "nearest at the origin publishes the complete zero-distance shell");
  check_ids(
      hierarchical_partition.canonical_choice_ids(), {PointId{0}},
      "nearest at the origin canonically chooses x=0");
  check_supported_audit(
      hierarchical_nearest.audit, 3U, 1U, 1U, 2U, 1U, 4U,
      "the hierarchical nearest audit");
  check(
      hierarchical_nearest.audit.gpu_output_cover_record_count == 2U &&
          hierarchical_nearest.audit.traversed_node_count == 3U &&
          hierarchical_nearest.audit.internal_node_expansion_count == 1U &&
          hierarchical_nearest.audit.certified_pruned_subtree_count == 1U &&
          hierarchical_nearest.audit.certified_pruned_point_count == 2U &&
          hierarchical_nearest.audit.minimum_certified_strict_margin ==
              ExactLevel{BigInt{1}},
      "one internal [1,2] subtree prune replaces two leaf candidates");
  check(
      hierarchical_nearest.audit.cpu_exact_seed_distance_evaluation_count ==
              1U &&
          hierarchical_nearest.audit.top_k_seed_squared_cutoff ==
              ExactLevel{} &&
          hierarchical_nearest.audit.excluded_candidate_point_count == 0U &&
          hierarchical_nearest.audit.certified_pruned_eligible_point_count ==
              2U,
      "hierarchical nearest closes one seed against two eligible pruned IDs");
  check_top_k_audit_closure(
      hierarchical_nearest, "the hierarchical nearest audit");
  check(
      fake_gpu_spatial_bounds_launch_count() == 4U,
      "all top-k variants reuse one resident context through epoch four");
}

void test_unsupported_range_falls_back_without_launch_or_poisoning() {
  reset_fake_gpu_spatial_bounds();
  const std::array<CertifiedPoint3, 3> points{
      point(0.0), point(1.0), point(2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  SpatialLbvhContext context{index, cloud};
  const ExactRational3 unsupported_query{
      BigInt{1} << 1024U, BigInt{0}, BigInt{0}, BigInt{1}};

  const SpatialLbvhClosedBallResult unsupported = context.closed_ball(
      cloud, unsupported_query, ExactLevel{BigInt{1}});
  check_ids(
      unsupported.exact_partition.interior_ids(), {},
      "the out-of-range query has no unit-ball interior point");
  check_ids(
      unsupported.exact_partition.shell_ids(), {},
      "the out-of-range query has no unit-ball shell point");
  check_ids(
      unsupported.exact_partition.exterior_ids(),
      {PointId{0}, PointId{1}, PointId{2}},
      "the exact host fallback classifies all finite points outside");
  check(
      unsupported.exact_partition.query_counters().method ==
              SpatialQueryMethod::brute_force &&
          unsupported.exact_partition.distance_evaluation_count() == 3U,
      "the unsupported enclosure publishes truthful brute-force counters");
  check(
      unsupported.audit.resident_node_count == 5U &&
          unsupported.audit.resident_point_count == 3U &&
          unsupported.audit.gpu_output_capacity == 5U &&
          unsupported.audit.gpu_output_cover_record_count == 0U &&
          unsupported.audit.gpu_launch_count == 0U &&
          unsupported.audit.gpu_kernel_launch_count == 0U &&
          unsupported.audit.gpu_traversal_round_count == 0U &&
          unsupported.audit.gpu_parallel_round_count == 0U &&
          unsupported.audit.gpu_peak_frontier_count == 0U &&
          unsupported.audit.gpu_processed_node_count == 0U &&
          unsupported.audit.traversed_node_count == 0U &&
          unsupported.audit.cpu_exact_aabb_bound_evaluation_count == 0U,
      "the unsupported enclosure never enters resident GPU traversal");
  check(
      unsupported.audit.candidate_point_count == 3U &&
          unsupported.audit.cpu_exact_candidate_distance_evaluation_count ==
              3U &&
          unsupported.audit.unsupported_range_fallback_count == 3U &&
          unsupported.audit.query_enclosure[0] ==
              DirectedEnclosureStatus::unsupported_range,
      "the unsupported audit exposes its complete exact host fallback");
  check(
      unsupported.audit.cover_antichain_complete &&
          unsupported.audit.point_partition_complete &&
          unsupported.audit.cpu_exact_recertification_complete &&
          unsupported.audit.exact_partition_complete &&
          fake_gpu_spatial_bounds_launch_count() == 0U,
      "a pre-GPU range fallback is complete without poisoning the context");

  const SpatialLbvhClosedBallResult supported = context.closed_ball(
      cloud, origin(), ExactLevel{BigInt{1}});
  check_unit_ball_partition(
      supported.exact_partition, cloud,
      "the supported query after an out-of-range fallback");
  check(
      supported.audit.buffer_epoch == 1U &&
          fake_gpu_spatial_bounds_launch_count() == 1U,
      "the first supported query launches at epoch one after fallback");
}

void test_move_and_point_namespace_contract() {
  reset_fake_gpu_spatial_bounds();
  const std::array<CertifiedPoint3, 3> points{
      point(0.0), point(1.0), point(2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const CanonicalPointCloud namespace_alias = cloud;
  const CanonicalPointCloud independent_cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const MortonLbvhIndex independent_index =
      MortonLbvhIndex::build(independent_cloud);

  check_throws<std::invalid_argument>(
      [&] {
        SpatialLbvhContext invalid{index, independent_cloud};
        static_cast<void>(invalid);
      },
      "an LBVH snapshot cannot be rebound to an equal but alien cloud");
  check(
      fake_gpu_spatial_bounds_launch_count() == 0U,
      "namespace rejection during construction has no GPU side effect");

  SpatialLbvhContext original{index, cloud};
  SpatialLbvhContext moved{std::move(original)};
  check(
      original.node_count() == 0U && original.point_count() == 0U,
      "a moved-from spatial-LBVH context exposes empty extents");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(original.closed_ball(
            cloud, origin(), ExactLevel{BigInt{1}}));
      },
      "a moved-from spatial-LBVH context rejects queries");

  const SpatialLbvhClosedBallResult alias_result = moved.closed_ball(
      namespace_alias, origin(), ExactLevel{BigInt{1}});
  check_unit_ball_partition(
      alias_result.exact_partition,
      namespace_alias,
      "a copied-cloud namespace alias");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(moved.closed_ball(
            independent_cloud, origin(), ExactLevel{BigInt{1}}));
      },
      "equal coordinates from an independent canonicalization are rejected");

  SpatialLbvhContext assigned{independent_index, independent_cloud};
  assigned = std::move(moved);
  check(
      moved.node_count() == 0U && moved.point_count() == 0U &&
          assigned.node_count() == 5U && assigned.point_count() == 3U,
      "move assignment transfers the resident namespace and extents");
  const SpatialLbvhClosedBallResult reassigned_result = assigned.closed_ball(
      cloud, origin(), ExactLevel{BigInt{1}});
  check_unit_ball_partition(
      reassigned_result.exact_partition,
      cloud,
      "the move-assigned spatial-LBVH context");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(assigned.closed_ball(
            independent_cloud, origin(), ExactLevel{BigInt{1}}));
      },
      "move assignment discards the destination's former namespace");
  check(
      fake_gpu_spatial_bounds_launch_count() == 2U &&
          reassigned_result.audit.buffer_epoch == 2U,
      "namespace failures are pre-GPU and do not poison moved state");
}

void test_false_prune_poisons_only_its_context() {
  reset_fake_gpu_spatial_bounds();
  const std::array<CertifiedPoint3, 3> points{
      point(0.0), point(1.0), point(2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  SpatialLbvhContext poisoned{index, cloud};
  configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{
      FakeSpatialBoundsProposalPermutation::canonical,
      FakeSpatialBoundsProposalValues::actual_interval_recipe,
      FakeSpatialBoundsProposalCorruption::false_prune});

  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(poisoned.closed_ball(
            cloud, origin(), ExactLevel{BigInt{1}}));
      },
      "an FP64 strict-prune claim that excludes x=0 is rejected exactly");
  check(
      fake_gpu_spatial_bounds_launch_count() == 1U,
      "the rejected false prune is a post-launch failure");

  configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{});
  const ExactRational3 unsupported_query{
      BigInt{1} << 1024U, BigInt{0}, BigInt{0}, BigInt{1}};
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(poisoned.closed_ball(
            cloud, unsupported_query, ExactLevel{BigInt{1}}));
      },
      "an unsupported-range fallback cannot bypass a poisoned context");
  check(
      fake_gpu_spatial_bounds_launch_count() == 1U,
      "fail-closed range fallback does not relaunch the poisoned context");
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(poisoned.closed_ball(
            cloud, origin(), ExactLevel{BigInt{1}}));
      },
      "a false prune poisons its resident traversal context");
  check(
      fake_gpu_spatial_bounds_launch_count() == 1U,
      "a poisoned context cannot launch a second traversal");

  SpatialLbvhContext fresh{index, cloud};
  const SpatialLbvhClosedBallResult fresh_result = fresh.closed_ball(
      cloud, origin(), ExactLevel{BigInt{1}});
  check_unit_ball_partition(
      fresh_result.exact_partition,
      cloud,
      "a fresh context after an isolated false prune");
  check(
      fresh_result.audit.cpu_exact_recertification_complete &&
          fresh_result.audit.exact_partition_complete &&
          fresh_result.audit.buffer_epoch == 1U &&
          fake_gpu_spatial_bounds_launch_count() == 2U,
      "poisoning remains isolated and a fresh context starts at epoch one");
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(poisoned.closed_ball(
            cloud, origin(), ExactLevel{BigInt{1}}));
      },
      "the original context stays poisoned after an independent success");
  check(
      fake_gpu_spatial_bounds_launch_count() == 2U,
      "retrying the poisoned context cannot affect the fresh one");
}

void test_frontier_overflow_poisons_only_its_context() {
  reset_fake_gpu_spatial_bounds();
  const std::array<CertifiedPoint3, 3> points{
      point(0.0), point(1.0), point(2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  SpatialLbvhContext poisoned{index, cloud};
  configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{
      FakeSpatialBoundsProposalPermutation::canonical,
      FakeSpatialBoundsProposalValues::actual_interval_recipe,
      FakeSpatialBoundsProposalCorruption::frontier_overflow});

  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(poisoned.closed_ball(
            cloud, origin(), ExactLevel{BigInt{1}}));
      },
      "frontier metadata above node capacity fails closed");
  check(
      fake_gpu_spatial_bounds_launch_count() == 1U,
      "invalid frontier metadata is rejected after one proposal call");

  configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{});
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(poisoned.closed_ball(
            cloud, origin(), ExactLevel{BigInt{1}}));
      },
      "frontier overflow poisons only its resident context");
  check(
      fake_gpu_spatial_bounds_launch_count() == 1U,
      "a poisoned frontier context cannot launch again");

  SpatialLbvhContext fresh{index, cloud};
  const SpatialLbvhClosedBallResult fresh_result = fresh.closed_ball(
      cloud, origin(), ExactLevel{BigInt{1}});
  check_unit_ball_partition(
      fresh_result.exact_partition,
      cloud,
      "a fresh context after invalid frontier metadata");
  check_supported_audit(
      fresh_result.audit, 3U, 2U, 2U, 1U, 1U, 1U,
      "the fresh context after frontier overflow");
  check(
      fake_gpu_spatial_bounds_launch_count() == 2U,
      "frontier overflow poisoning remains isolated from a fresh context");
}

void test_malformed_covers_and_launch_failure_poison_fail_closed() {
  const std::array<CertifiedPoint3, 3> points{
      point(0.0), point(1.0), point(2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<FakeSpatialBoundsProposalCorruption, 4> corruptions{
      FakeSpatialBoundsProposalCorruption::missing_record,
      FakeSpatialBoundsProposalCorruption::duplicate_box_index,
      FakeSpatialBoundsProposalCorruption::out_of_range_box_index,
      FakeSpatialBoundsProposalCorruption::invalid_decision};
  for (const FakeSpatialBoundsProposalCorruption corruption : corruptions) {
    reset_fake_gpu_spatial_bounds();
    SpatialLbvhContext poisoned{index, cloud};
    configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{
        FakeSpatialBoundsProposalPermutation::canonical,
        FakeSpatialBoundsProposalValues::actual_interval_recipe,
        corruption});
    check_throws<std::runtime_error>(
        [&] {
          static_cast<void>(poisoned.closed_ball(
              cloud, origin(), ExactLevel{BigInt{1}}));
        },
        "a malformed terminal cover is rejected after its proposal call");
    check(
        fake_gpu_spatial_bounds_launch_count() == 1U,
        "each malformed cover consumes exactly one proposal call");
    configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{});
    check_throws<std::runtime_error>(
        [&] {
          static_cast<void>(poisoned.closed_ball(
              cloud, origin(), ExactLevel{BigInt{1}}));
        },
        "a malformed terminal cover poisons its resident context");
    check(
        fake_gpu_spatial_bounds_launch_count() == 1U,
        "a context poisoned by malformed cover metadata cannot relaunch");
  }

  reset_fake_gpu_spatial_bounds();
  SpatialLbvhContext launch_failure{index, cloud};
  configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{
      FakeSpatialBoundsProposalPermutation::canonical,
      FakeSpatialBoundsProposalValues::actual_interval_recipe,
      FakeSpatialBoundsProposalCorruption::simulated_gpu_failure});
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(launch_failure.closed_ball(
            cloud, origin(), ExactLevel{BigInt{1}}));
      },
      "a traversal launch failure is converted to a fail-closed exception");
  check(
      fake_gpu_spatial_bounds_launch_count() == 0U,
      "a simulated pre-publication launch failure emits no proposal count");
  configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{});
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(launch_failure.closed_ball(
            cloud, origin(), ExactLevel{BigInt{1}}));
      },
      "a launch failure poisons its resident context before retry");
}

}  // namespace

int main() {
  test_singleton_context_and_exact_partition();
  test_unit_ball_pruning_permutation_and_unknown_fallback();
  test_top_k_seed_shell_and_exclusion_accounting();
  test_unsupported_range_falls_back_without_launch_or_poisoning();
  test_move_and_point_namespace_contract();
  test_false_prune_poisons_only_its_context();
  test_frontier_overflow_poisons_only_its_context();
  test_malformed_covers_and_launch_failure_poison_fail_closed();
  if (failures != 0) {
    std::cerr << failures << " GPU spatial-LBVH context test(s) failed\n";
    return 1;
  }
  std::cout << "GPU spatial-LBVH context tests passed\n";
  return 0;
}
