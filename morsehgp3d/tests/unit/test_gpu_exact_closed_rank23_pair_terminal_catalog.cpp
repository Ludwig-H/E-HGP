#include "fake_gpu_morton_yao48_device_tiled_pair_frontier_launchers.hpp"
#include "fake_gpu_morton_yao48_ranked_pair_tile_classifier_launchers.hpp"
#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/exact_closed_rank23_pair_terminal_catalog.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::ExactClosedRank23PairTerminalCatalog;
using morsehgp3d::gpu::ExactClosedRank23PairTerminalCatalogConfig;
using morsehgp3d::gpu::ExactClosedRank23PairTerminalCatalogResult;
using morsehgp3d::gpu::ExactClosedRank23PairTerminalCatalogStatus;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierStopReason;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierYieldReason;
using morsehgp3d::gpu::MortonYao48RankedPairTileClassifierStopReason;
using morsehgp3d::gpu::build_exact_closed_rank23_pair_terminal_catalog;
using morsehgp3d::gpu::test_support::
    FakeMortonYao48DeviceTiledAnchor;
using morsehgp3d::gpu::test_support::
    FakeMortonYao48DeviceTiledAnchorStatus;
using morsehgp3d::gpu::test_support::
    FakeMortonYao48DeviceTiledPairFrontierConfiguration;
using morsehgp3d::gpu::test_support::
    FakeMortonYao48DeviceTiledPairFrontierCorruption;
using morsehgp3d::gpu::test_support::
    FakeMortonYao48DeviceTiledPairFrontierLaunch;
using morsehgp3d::gpu::test_support::
    FakeMortonYao48RankedPairTileClassifierConfiguration;
using morsehgp3d::gpu::test_support::
    FakeMortonYao48RankedPairTileClassifierLaunch;
using morsehgp3d::gpu::test_support::
    configure_fake_gpu_morton_yao48_device_tiled_pair_frontier;
using morsehgp3d::gpu::test_support::
    configure_fake_gpu_morton_yao48_ranked_pair_tile_classifier;
using morsehgp3d::gpu::test_support::
    fake_gpu_morton_yao48_ranked_pair_tile_classifier_launch_count;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_morton_yao48_device_tiled_pair_frontier;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_morton_yao48_ranked_pair_tile_classifier;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::spatial::CanonicalPointCloud;

static_assert(
    !std::is_copy_constructible_v<ExactClosedRank23PairTerminalCatalog>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactClosedRank23PairTerminalCatalog>);
static_assert(
    !std::is_copy_constructible_v<ExactClosedRank23PairTerminalCatalogResult>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactClosedRank23PairTerminalCatalogResult>);

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

[[nodiscard]] CanonicalPointCloud line_cloud(std::size_t count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(count);
  for (std::size_t index = 0U; index < count; ++index) {
    const double coordinate = static_cast<double>(index);
    points.push_back(point(
        coordinate, coordinate / 8.0, coordinate / 64.0));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] MortonLbvhDeviceTraversalLease traversal_lease(
    std::size_t point_count) {
  reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = line_cloud(point_count);
  MortonLbvhBuildContext builder{point_count + 2U};
  auto build = builder.build(cloud);
  return builder.release_device_traversal_lease(build);
}

[[nodiscard]] FakeMortonYao48DeviceTiledAnchor complete_anchor(
    std::uint64_t candidates,
    std::uint64_t prune_regions,
    std::uint64_t pruned_mass,
    std::uint64_t node_visits) {
  FakeMortonYao48DeviceTiledAnchor anchor;
  anchor.candidate_count = candidates;
  anchor.prune_region_count = prune_regions;
  anchor.certified_pruned_pair_mass = pruned_mass;
  anchor.node_visit_count = node_visits;
  return anchor;
}

[[nodiscard]] FakeMortonYao48DeviceTiledAnchor candidate_chunk() {
  FakeMortonYao48DeviceTiledAnchor anchor =
      complete_anchor(640U, 0U, 0U, 640U);
  anchor.status = FakeMortonYao48DeviceTiledAnchorStatus::chunk_ready;
  anchor.yield_reason =
      MortonYao48DeviceTiledPairFrontierYieldReason::candidate_segment_full;
  return anchor;
}

[[nodiscard]] FakeMortonYao48DeviceTiledAnchor node_capacity_fatal() {
  FakeMortonYao48DeviceTiledAnchor anchor;
  anchor.node_visit_count = 5U;
  anchor.status = FakeMortonYao48DeviceTiledAnchorStatus::fatal;
  anchor.stop_reason =
      MortonYao48DeviceTiledPairFrontierStopReason::node_visit_capacity;
  return anchor;
}

[[nodiscard]] std::vector<FakeMortonYao48DeviceTiledAnchor>
complete_prefix_by_prune(std::size_t count) {
  std::vector<FakeMortonYao48DeviceTiledAnchor> anchors;
  anchors.reserve(count);
  for (std::size_t position = 1U; position <= count; ++position) {
    anchors.push_back(complete_anchor(0U, 1U, position, 1U));
  }
  return anchors;
}

void configure_frontier_transcripts(
    std::vector<FakeMortonYao48DeviceTiledPairFrontierLaunch> launches) {
  FakeMortonYao48DeviceTiledPairFrontierConfiguration configuration;
  configuration.launches = std::move(launches);
  configure_fake_gpu_morton_yao48_device_tiled_pair_frontier(
      std::move(configuration));
}

void configure_classifier_transcripts(
    std::vector<FakeMortonYao48RankedPairTileClassifierLaunch> launches) {
  configure_fake_gpu_morton_yao48_ranked_pair_tile_classifier(
      FakeMortonYao48RankedPairTileClassifierConfiguration{
          std::move(launches)});
}

[[nodiscard]] FakeMortonYao48RankedPairTileClassifierLaunch exact_launch(
    std::uint64_t accepted,
    std::uint64_t above,
    std::uint64_t strict_payload,
    std::uint64_t shell_payload,
    std::uint64_t fallback = 0U) {
  FakeMortonYao48RankedPairTileClassifierLaunch launch;
  launch.accepted_record_count = accepted;
  launch.above_window_count = above;
  launch.strict_payload_point_id_count = strict_payload;
  launch.shell_payload_point_id_count = shell_payload;
  launch.exact_fallback_count = fallback;
  return launch;
}

void reset_transcripts() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  reset_fake_gpu_morton_yao48_ranked_pair_tile_classifier();
}

void test_structurally_complete_host_fake_is_not_scientific_authority() {
  reset_transcripts();
  auto traversal = traversal_lease(642U);
  auto first_anchors = complete_prefix_by_prune(640U);
  first_anchors.push_back(candidate_chunk());
  std::vector<FakeMortonYao48DeviceTiledAnchor> second_anchors(
      640U, complete_anchor(0U, 0U, 0U, 0U));
  second_anchors.push_back(complete_anchor(1U, 0U, 0U, 1U));
  configure_frontier_transcripts({
      {std::move(first_anchors),
       FakeMortonYao48DeviceTiledPairFrontierCorruption::none},
      {std::move(second_anchors),
       FakeMortonYao48DeviceTiledPairFrontierCorruption::none}});
  configure_classifier_transcripts({
      exact_launch(400U, 240U, 300U, 100U),
      exact_launch(1U, 0U, 0U, 1U)});

  const ExactClosedRank23PairTerminalCatalogConfig config{
      641U, 1U, 1U, 1U};
  auto result = build_exact_closed_rank23_pair_terminal_catalog(
      std::move(traversal), config);
  check(
      result.status == ExactClosedRank23PairTerminalCatalogStatus::
                           non_authoritative_host_fake &&
          !result.complete() && !result.catalog.has_value() &&
          result.audit.host_fake_lifecycle_exercised &&
          !result.audit.cuda_execution_performed &&
          result.audit.output_withheld &&
          !result.audit.closed_rank23_pair_catalog_complete,
      "a structurally valid host-fake catalog exercises the complete "
      "pipeline but cannot publish scientific rank-2/3 authority");
  const auto& audit = result.audit;
  check(
      audit.maximum_closed_rank == 3U &&
          audit.frontier_advance_count == 2U &&
          audit.classifier_commit_count == 2U &&
          audit.committed_chunk_count == 2U &&
          audit.catalog_record_capacity == 642U &&
          audit.payload_point_id_capacity == 642U &&
          audit.catalog_record_count == 401U &&
          audit.candidate_count == 641U &&
          audit.above_window_count == 240U &&
          audit.certified_pruned_pair_mass == 205'120U &&
          audit.unordered_pair_universe_count == 205'761U &&
          audit.frontier_closed && audit.frontier_mass_reconciled &&
          audit.classifier_mass_reconciled &&
          audit.zero_unresolved_certified &&
          audit.zero_exact_fallback_certified &&
          !audit.closed_rank23_pair_catalog_complete &&
          audit.output_withheld && !audit.gamma2_complete_claimed &&
          !audit.k2_complete_claimed &&
          !audit.hierarchy_complete_claimed &&
          !audit.latency_100ms_claimed,
      "the rejected host-fake run still reports its closed mass and zero "
      "fallback evidence without upgrading it to geometric authority");
}

void test_fake_treated_fallback_is_still_unresolved() {
  reset_transcripts();
  auto traversal = traversal_lease(3U);
  configure_classifier_transcripts({exact_launch(2U, 0U, 0U, 0U, 1U)});

  auto result = build_exact_closed_rank23_pair_terminal_catalog(
      std::move(traversal),
      ExactClosedRank23PairTerminalCatalogConfig{2U, 1U, 0U, 1U});
  check(
      result.status == ExactClosedRank23PairTerminalCatalogStatus::
                           exact_predicate_unresolved &&
          !result.complete() && !result.catalog.has_value() &&
          result.audit.exact_fallback_count == 1U &&
          !result.audit.zero_exact_fallback_certified &&
          !result.audit.closed_rank23_pair_catalog_complete &&
          result.audit.output_withheld,
      "a fallback reported as handled by the fake classifier is refused "
      "without publishing its resident partial output");
}

void test_linear_catalog_capacity_exhaustion_withholds_output() {
  reset_transcripts();
  auto traversal = traversal_lease(3U);
  auto launch = exact_launch(3U, 0U, 0U, 0U);
  launch.capacity_stop_reason =
      MortonYao48RankedPairTileClassifierStopReason::
          catalog_record_capacity;
  configure_classifier_transcripts({launch});

  auto result = build_exact_closed_rank23_pair_terminal_catalog(
      std::move(traversal),
      ExactClosedRank23PairTerminalCatalogConfig{2U, 0U, 0U, 0U});
  check(
      result.status == ExactClosedRank23PairTerminalCatalogStatus::
                           capacity_exhausted &&
          !result.complete() && !result.catalog.has_value() &&
          result.audit.classifier_stop_reason ==
              MortonYao48RankedPairTileClassifierStopReason::
                  catalog_record_capacity &&
          result.audit.catalog_record_capacity == 0U &&
          result.audit.fixed_linear_capacities_validated &&
          !result.audit.closed_rank23_pair_catalog_complete &&
          result.audit.output_withheld,
      "a linear arena refusal is terminal and cannot leak a prefix "
      "catalog");
}

void test_censored_frontier_never_reaches_classifier() {
  reset_transcripts();
  auto traversal = traversal_lease(3U);
  configure_frontier_transcripts({
      {{node_capacity_fatal(), node_capacity_fatal()},
       FakeMortonYao48DeviceTiledPairFrontierCorruption::none}});

  auto result = build_exact_closed_rank23_pair_terminal_catalog(
      std::move(traversal),
      ExactClosedRank23PairTerminalCatalogConfig{2U, 1U, 1U, 1U});
  check(
      result.status == ExactClosedRank23PairTerminalCatalogStatus::
                           frontier_censored &&
          !result.complete() && !result.catalog.has_value() &&
          result.audit.frontier_advance_count == 1U &&
          result.audit.classifier_commit_count == 0U &&
          result.audit.frontier_stop_reason ==
              MortonYao48DeviceTiledPairFrontierStopReason::
                  node_visit_capacity &&
          !result.audit.frontier_closed &&
          !result.audit.closed_rank23_pair_catalog_complete &&
          result.audit.output_withheld &&
          fake_gpu_morton_yao48_ranked_pair_tile_classifier_launch_count() ==
              0U,
      "a censored frontier fails closed before classifier launch and "
      "publishes no candidate-derived catalog");
}

}  // namespace

int main() {
  test_structurally_complete_host_fake_is_not_scientific_authority();
  test_fake_treated_fallback_is_still_unresolved();
  test_linear_catalog_capacity_exhaustion_withholds_output();
  test_censored_frontier_never_reaches_classifier();
  if (failures != 0) {
    std::cerr << failures
              << " exact closed-rank 2/3 terminal catalog test(s) failed\n";
    return 1;
  }
  std::cout << "exact closed-rank 2/3 terminal catalog tests passed\n";
  return 0;
}
