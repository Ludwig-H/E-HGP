#include "fake_gpu_morton_yao48_device_tiled_pair_frontier_launchers.hpp"
#include "fake_gpu_morton_yao48_ranked_pair_h0_projection_launchers.hpp"
#include "fake_gpu_morton_yao48_ranked_pair_tile_classifier_launchers.hpp"
#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/morton_yao48_device_tiled_pair_frontier.hpp"
#include "morsehgp3d/gpu/morton_yao48_ranked_pair_h0_projection.hpp"
#include "morsehgp3d/gpu/morton_yao48_ranked_pair_tile_classifier.hpp"
#include "phase15_morton_yao48_ranked_pair_h0_projection_internal.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierConfig;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierContext;
using morsehgp3d::gpu::MortonYao48RankedPairH0ProjectionConfig;
using morsehgp3d::gpu::MortonYao48RankedPairH0ProjectionLease;
using morsehgp3d::gpu::MortonYao48RankedPairH0ProjectionResult;
using morsehgp3d::gpu::MortonYao48RankedPairH0ProjectionStatus;
using morsehgp3d::gpu::MortonYao48RankedPairTileCatalogLease;
using morsehgp3d::gpu::MortonYao48RankedPairTileClassifierConfig;
using morsehgp3d::gpu::MortonYao48RankedPairTileClassifierContext;
using morsehgp3d::gpu::MortonYao48RankedPairTileClassifierStatus;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48RankedPairH0ProjectionPrivateViewAccess;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48RankedPairH0RecordKind;
using morsehgp3d::gpu::project_morton_yao48_ranked_pair_h0_candidates;
using morsehgp3d::gpu::test_support::
    FakeMortonYao48DeviceTiledPairFrontierConfiguration;
using morsehgp3d::gpu::test_support::
    FakeMortonYao48RankedPairTileClassifierConfiguration;
using morsehgp3d::gpu::test_support::
    FakeMortonYao48RankedPairTileClassifierLaunch;
using morsehgp3d::gpu::test_support::
    configure_fake_gpu_morton_yao48_device_tiled_pair_frontier;
using morsehgp3d::gpu::test_support::
    configure_fake_gpu_morton_yao48_ranked_pair_tile_classifier;
using morsehgp3d::gpu::test_support::
    fake_gpu_morton_yao48_ranked_pair_h0_projection_launch_count;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_morton_yao48_device_tiled_pair_frontier;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_morton_yao48_ranked_pair_h0_projection;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_morton_yao48_ranked_pair_tile_classifier;
using morsehgp3d::gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::spatial::CanonicalPointCloud;

static_assert(
    !std::is_copy_constructible_v<
        MortonYao48RankedPairH0ProjectionLease>);
static_assert(
    std::is_nothrow_move_constructible_v<
        MortonYao48RankedPairH0ProjectionLease>);
static_assert(
    !std::is_copy_constructible_v<
        MortonYao48RankedPairH0ProjectionResult>);
static_assert(
    std::is_nothrow_move_constructible_v<
        MortonYao48RankedPairH0ProjectionResult>);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <class Exception, class Function>
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

void reset_fakes() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  reset_fake_gpu_morton_yao48_ranked_pair_tile_classifier();
  reset_fake_gpu_morton_yao48_ranked_pair_h0_projection();
}

[[nodiscard]] MortonYao48RankedPairTileCatalogLease source_catalog(
    std::uint64_t accepted_record_count,
    std::uint64_t strict_payload_count,
    std::uint64_t shell_payload_count) {
  constexpr std::size_t point_count = 6U;
  auto traversal = traversal_lease(point_count);

  configure_fake_gpu_morton_yao48_device_tiled_pair_frontier(
      FakeMortonYao48DeviceTiledPairFrontierConfiguration{});

  FakeMortonYao48RankedPairTileClassifierLaunch classifier_launch;
  classifier_launch.accepted_record_count = accepted_record_count;
  classifier_launch.above_window_count =
      15U - accepted_record_count;
  classifier_launch.strict_payload_point_id_count =
      strict_payload_count;
  classifier_launch.shell_payload_point_id_count =
      shell_payload_count;
  configure_fake_gpu_morton_yao48_ranked_pair_tile_classifier(
      FakeMortonYao48RankedPairTileClassifierConfiguration{
          {classifier_launch}});

  MortonYao48DeviceTiledPairFrontierContext frontier{
      std::move(traversal),
      MortonYao48DeviceTiledPairFrontierConfig{6U, 4096U}};
  MortonYao48RankedPairTileClassifierContext classifier{
      MortonYao48RankedPairTileClassifierConfig{
          6U, 15U, 60U, 1U}};
  auto advance = frontier.advance();
  const auto commit = classifier.commit(std::move(advance));
  if (commit.status !=
      MortonYao48RankedPairTileClassifierStatus::frontier_closed) {
    throw std::runtime_error(
        "the fake ranked-pair source did not close");
  }
  return classifier.finish();
}

void test_regular_and_extra_shell_lanes_remain_separate() {
  reset_fakes();
  auto source = source_catalog(4U, 2U, 2U);
  auto result = project_morton_yao48_ranked_pair_h0_candidates(
      std::move(source),
      MortonYao48RankedPairH0ProjectionConfig{1U});
  check(
      result.status ==
              MortonYao48RankedPairH0ProjectionStatus::
                  non_authoritative_host_fake &&
          !result.complete() && result.projection.has_value() &&
          result.projection->ready() && result.projection->host_fake(),
      "the host fake exercises the move-only projection without publishing "
      "CUDA authority");
  if (!result.projection.has_value()) {
    return;
  }

  const auto& audit = result.projection->audit();
  const auto views =
      Phase15MortonYao48RankedPairH0ProjectionPrivateViewAccess::inspect(
          *result.projection);
  check(
      audit.maximum_closed_rank == 6U &&
          audit.source_record_count == 4U &&
          audit.projected_record_count == 4U &&
          audit.regular_pair_candidate_count == 3U &&
          audit.diagnostic_or_carrier_candidate_count == 1U &&
          audit.strict_payload_point_id_count == 2U &&
          audit.shell_payload_point_id_count == 2U &&
          audit.regular_pair_requires_empty_extra_shell &&
          audit.regular_pair_closed_rank_identity_validated &&
          audit.scientific_birth_and_saddle_roles_withheld &&
          !audit.sparse_direct_h0_payload_adapter_prerequisites_retained &&
          !audit.sorted_by_sparse_direct_h0_terminal_key &&
          !audit.support3_complete_claimed &&
          !audit.support4_complete_claimed &&
          !audit.gamma2_complete_claimed &&
          !audit.hierarchy_complete_claimed &&
          !audit.latency_100ms_claimed &&
          !audit.public_status_claimed,
      "the audit separates regular candidates from extra-shell carriers "
      "and withholds all downstream claims");
  check(
      views.ready && views.host_fake && views.output_owner &&
          views.source_catalog && views.record_count == 4U &&
          views.device_records != nullptr &&
          views.device_strict_point_ids != nullptr &&
          views.device_shell_point_ids != nullptr &&
          views.device_records[0U].kind ==
              Phase15MortonYao48RankedPairH0RecordKind::
                  regular_pair_candidate &&
          views.device_records[0U].closed_rank == 2U &&
          views.device_records[0U].shell_begin ==
              views.device_records[0U].shell_end &&
          views.device_records[3U].kind ==
              Phase15MortonYao48RankedPairH0RecordKind::
                  diagnostic_or_carrier_candidate &&
          views.device_records[3U].closed_rank == 6U &&
          views.device_records[3U].strict_end -
                  views.device_records[3U].strict_begin ==
              2U &&
          views.device_records[3U].shell_end -
                  views.device_records[3U].shell_begin ==
              2U,
      "the private friend capability retains exact-level records and "
      "borrowed strict/shell payloads without a public raw view");
  check(
      fake_gpu_morton_yao48_ranked_pair_h0_projection_launch_count() == 1U,
      "exactly one bounded projection launch is used");
}

void test_linear_capacity_fails_before_launch() {
  reset_fakes();
  auto source = source_catalog(7U, 0U, 0U);
  auto result = project_morton_yao48_ranked_pair_h0_candidates(
      std::move(source),
      MortonYao48RankedPairH0ProjectionConfig{1U});
  check(
      result.status ==
              MortonYao48RankedPairH0ProjectionStatus::
                  capacity_exhausted &&
          !result.projection.has_value() &&
          result.audit.source_record_count == 7U &&
          result.audit.record_capacity == 6U &&
          fake_gpu_morton_yao48_ranked_pair_h0_projection_launch_count() ==
              0U,
      "a source larger than point_count times the fixed factor fails before "
      "the launcher");
}

void test_unbounded_or_zero_factor_is_rejected() {
  reset_fakes();
  auto source = source_catalog(1U, 0U, 0U);
  check_throws<std::invalid_argument>(
      [&source] {
        (void)project_morton_yao48_ranked_pair_h0_candidates(
            std::move(source),
            MortonYao48RankedPairH0ProjectionConfig{0U});
      },
      "a zero linear capacity factor is rejected");
}

}  // namespace

int main() {
  test_regular_and_extra_shell_lanes_remain_separate();
  test_linear_capacity_fails_before_launch();
  test_unbounded_or_zero_factor_is_rejected();
  if (failures != 0) {
    std::cerr << failures
              << " ranked-pair H0 projection test(s) failed\n";
    return 1;
  }
  std::cout << "ranked-pair H0 projection tests passed\n";
  return 0;
}
