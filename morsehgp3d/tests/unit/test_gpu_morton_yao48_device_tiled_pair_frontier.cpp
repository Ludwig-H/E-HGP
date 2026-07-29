#include "fake_gpu_morton_yao48_device_tiled_pair_frontier_launchers.hpp"
#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/morton_yao48_device_tiled_pair_frontier.hpp"

#include "phase15_morton_yao48_device_tiled_pair_frontier_internal.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
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
using morsehgp3d::gpu::MortonYao48DeviceCandidateTileLease;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierAdvance;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierConfig;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierContext;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierStatus;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierStopReason;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceCandidateTilePrivateViewAccess;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceCandidateTilePrivateViews;
using morsehgp3d::gpu::test_support::
    FakeMortonYao48DeviceTiledAnchor;
using morsehgp3d::gpu::test_support::
    FakeMortonYao48DeviceTiledPairFrontierConfiguration;
using morsehgp3d::gpu::test_support::
    FakeMortonYao48DeviceTiledPairFrontierCorruption;
using morsehgp3d::gpu::test_support::
    configure_fake_gpu_morton_yao48_device_tiled_pair_frontier;
using morsehgp3d::gpu::test_support::
    fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_morton_yao48_device_tiled_pair_frontier;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::spatial::CanonicalPointCloud;

static_assert(!std::is_copy_constructible_v<
              MortonYao48DeviceCandidateTileLease>);
static_assert(!std::is_copy_assignable_v<
              MortonYao48DeviceCandidateTileLease>);
static_assert(std::is_nothrow_move_constructible_v<
              MortonYao48DeviceCandidateTileLease>);
static_assert(std::is_nothrow_move_assignable_v<
              MortonYao48DeviceCandidateTileLease>);
static_assert(!std::is_copy_constructible_v<
              MortonYao48DeviceTiledPairFrontierContext>);
static_assert(!std::is_copy_assignable_v<
              MortonYao48DeviceTiledPairFrontierContext>);
static_assert(std::is_nothrow_move_constructible_v<
              MortonYao48DeviceTiledPairFrontierContext>);
static_assert(std::is_nothrow_move_assignable_v<
              MortonYao48DeviceTiledPairFrontierContext>);
static_assert(!std::is_copy_constructible_v<
              MortonYao48DeviceTiledPairFrontierAdvance>);

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
    std::uint64_t ambiguous = 0U,
    std::uint64_t unbanked = 0U) {
  FakeMortonYao48DeviceTiledAnchor anchor;
  anchor.candidate_count = candidates;
  anchor.prune_region_count = prune_regions;
  anchor.certified_pruned_pair_mass = pruned_mass;
  anchor.node_visit_count = candidates + prune_regions;
  anchor.ambiguous_cone_candidate_count = ambiguous;
  anchor.unbanked_candidate_count = unbanked;
  return anchor;
}

void test_complete_tiles_close_one_coverage_partition() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  auto lease = traversal_lease(6U);
  MortonYao48DeviceTiledPairFrontierContext context{
      std::move(lease), MortonYao48DeviceTiledPairFrontierConfig{2U, 2U}};
  check(
      context.ready() && !lease.ready(),
      "the device frontier exclusively adopts the Phase 14 traversal lease");

  configure_fake_gpu_morton_yao48_device_tiled_pair_frontier(
      FakeMortonYao48DeviceTiledPairFrontierConfiguration{
          {complete_anchor(1U, 0U, 0U),
           complete_anchor(1U, 1U, 1U, 1U, 1U)},
          FakeMortonYao48DeviceTiledPairFrontierCorruption::none});
  auto first = context.advance();
  check(
      first.status ==
              MortonYao48DeviceTiledPairFrontierStatus::tile_complete &&
          first.stop_reason ==
              MortonYao48DeviceTiledPairFrontierStopReason::none &&
          first.candidate_tile.has_value() &&
          first.candidate_tile->ready() &&
          first.candidate_tile->host_fake() &&
          !first.candidate_tile->cuda_resident(),
      "the first complete prefix publishes one opaque host-fake tile lease");
  const auto& first_lease_audit = first.candidate_tile->audit();
  check(
      first_lease_audit.anchor_begin == 1U &&
          first_lease_audit.anchor_end == 3U &&
          first_lease_audit.candidate_count == 2U &&
          first_lease_audit.certified_prune_region_count == 1U &&
          first_lease_audit.traversal_owner_retained &&
          first_lease_audit.source_cloud_identity_retained &&
          !first_lease_audit.source_device_views_retained &&
          first_lease_audit.source_device_extents_retained &&
          first_lease_audit.source_views_bound_to_snapshot_identity &&
          first_lease_audit.output_owner_retained &&
          first_lease_audit.output_buffers_detached_for_tile_lifetime &&
          !first_lease_audit.candidate_device_to_host_performed &&
          !first_lease_audit.certified_prune_device_to_host_performed &&
          !first_lease_audit.exact_diametral_rank_evaluated &&
          !first_lease_audit.scientific_pair_catalog_published,
      "the tile lease retains detached device authorities without a product D2H view");
  check(
      first.audit.transaction_candidate_pair_mass == 2U &&
          first.audit.transaction_certified_pruned_pair_mass == 1U &&
          first.audit.transaction_ambiguous_cone_candidate_count == 1U &&
          first.audit.transaction_unbanked_candidate_count == 1U &&
          first.audit.cumulative_candidate_pair_mass == 2U &&
          first.audit.cumulative_certified_pruned_pair_mass == 1U &&
          first.audit.unresolved_pair_mass == 12U &&
          first.audit.atomic_completed_anchor_prefix_validated &&
          first.audit.candidate_pruned_unresolved_partition_validated &&
          first.audit.candidate_tile_lease_backpressure_bounded_to_one &&
          first.audit.candidate_tile_lease_outstanding &&
          first.audit.output_buffers_detached_for_tile_lifetime &&
          first.audit.host_fake_launcher_exercised &&
          !first.audit.cuda_execution_performed &&
          first.audit.candidate_device_to_host_count == 0U &&
          first.audit.certified_prune_device_to_host_count == 0U &&
          first.audit.anchor_control_device_to_host_count == 0U &&
          !first.audit.pair_coverage_partition_complete,
      "the first transaction closes only its two-anchor coverage prefix");

  const std::size_t launches_before_backpressure =
      fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count();
  check_throws<std::logic_error>(
      [&context] { (void)context.advance(); },
      "an outstanding detached tile blocks a second physical allocation");
  check(
      context.ready() && !context.poisoned() &&
          fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count() ==
              launches_before_backpressure,
      "pre-launch tile backpressure preserves context state and submits no launcher work");
  first.candidate_tile.reset();

  configure_fake_gpu_morton_yao48_device_tiled_pair_frontier(
      FakeMortonYao48DeviceTiledPairFrontierConfiguration{
          {complete_anchor(2U, 1U, 1U),
           complete_anchor(1U, 1U, 3U)},
          FakeMortonYao48DeviceTiledPairFrontierCorruption::none});
  auto second = context.advance();
  check(
      second.status ==
              MortonYao48DeviceTiledPairFrontierStatus::tile_complete &&
          second.audit.advance_sequence == 2U &&
          second.candidate_tile.has_value() &&
          second.candidate_tile->audit().candidate_buffer_epoch == 2U &&
          second.audit.cumulative_candidate_pair_mass == 5U &&
          second.audit.cumulative_certified_pruned_pair_mass == 5U &&
          second.audit.unresolved_pair_mass == 5U,
      "the second detached tile extends the same global pair partition");
  second.candidate_tile.reset();

  configure_fake_gpu_morton_yao48_device_tiled_pair_frontier(
      FakeMortonYao48DeviceTiledPairFrontierConfiguration{
          {complete_anchor(2U, 1U, 3U)},
          FakeMortonYao48DeviceTiledPairFrontierCorruption::none});
  auto third = context.advance();
  check(
      third.status ==
              MortonYao48DeviceTiledPairFrontierStatus::frontier_complete &&
          third.audit.unordered_pair_universe_count == 15U &&
          third.audit.cumulative_candidate_pair_mass == 7U &&
          third.audit.cumulative_certified_pruned_pair_mass == 8U &&
          third.audit.unresolved_pair_mass == 0U &&
          third.audit.pair_coverage_partition_complete &&
          !third.audit.exact_diametral_rank_evaluated &&
          !third.audit.scientific_pair_catalog_published &&
          !third.audit.scientific_decision_published &&
          !third.audit.ordinary_delaunay_materialized &&
          !third.audit.higher_order_delaunay_mosaic_materialized,
      "only candidate plus certified-prune mass closes the 15-pair universe");
  third.candidate_tile.reset();

  const std::size_t launches_before_idempotent =
      fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count();
  auto stable = context.advance();
  check(
      stable.status ==
              MortonYao48DeviceTiledPairFrontierStatus::frontier_complete &&
          !stable.candidate_tile.has_value() &&
          stable.audit.transaction_candidate_pair_mass == 0U &&
          stable.audit.transaction_certified_pruned_pair_mass == 0U &&
          stable.audit.unresolved_pair_mass == 0U &&
          stable.audit.candidate_tile_lease_backpressure_bounded_to_one &&
          !stable.audit.candidate_tile_lease_outstanding &&
          fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count() ==
              launches_before_idempotent,
      "a closed frontier is idempotent and launches no hidden fallback");
}

void test_detached_tile_retains_private_source_capability() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  std::optional<MortonYao48DeviceCandidateTileLease> detached_tile;
  Phase15MortonYao48DeviceCandidateTilePrivateViews before_destruction;
  {
    auto traversal = traversal_lease(4U);
    MortonYao48DeviceTiledPairFrontierContext context{
        std::move(traversal),
        MortonYao48DeviceTiledPairFrontierConfig{2U, 2U}};
    configure_fake_gpu_morton_yao48_device_tiled_pair_frontier(
        FakeMortonYao48DeviceTiledPairFrontierConfiguration{
            {complete_anchor(1U, 0U, 0U),
             complete_anchor(2U, 0U, 0U)},
            FakeMortonYao48DeviceTiledPairFrontierCorruption::none});
    auto advance = context.advance();
    check(
        advance.candidate_tile.has_value(),
        "a complete prefix provides a detached tile for lifetime testing");
    if (!advance.candidate_tile.has_value()) {
      return;
    }
    detached_tile.emplace(std::move(*advance.candidate_tile));
    before_destruction =
        Phase15MortonYao48DeviceCandidateTilePrivateViewAccess::inspect(
            *detached_tile);
    check(
        before_destruction.ready && before_destruction.host_fake &&
            before_destruction.retained_authority_identity != nullptr &&
            before_destruction.source_owner_identity != nullptr &&
            before_destruction.source_cloud_identity != nullptr &&
            before_destruction.point_count == 4U &&
            before_destruction.certified_node_count == 7U &&
            before_destruction.retained_coordinate_word_capacity == 18U &&
            before_destruction.retained_morton_point_id_capacity == 6U &&
            before_destruction.retained_node_capacity == 11U &&
            before_destruction.anchor_begin == 1U &&
            before_destruction.anchor_end == 3U &&
            before_destruction.device_coordinate_bits == nullptr &&
            before_destruction.device_morton_point_ids == nullptr &&
            before_destruction.device_nodes == nullptr &&
            !before_destruction.source_device_views_retained &&
            before_destruction.source_views_bound_to_snapshot_identity,
        "the private fake capability retains authenticated source identity and extents without forging CUDA views");
  }

  check(
      detached_tile.has_value() && detached_tile->ready(),
      "destroying the frontier context leaves the detached tile valid");
  if (!detached_tile.has_value()) {
    return;
  }
  const Phase15MortonYao48DeviceCandidateTilePrivateViews
      after_destruction =
          Phase15MortonYao48DeviceCandidateTilePrivateViewAccess::inspect(
              *detached_tile);
  check(
      after_destruction.ready &&
          after_destruction.retained_authority_identity ==
              before_destruction.retained_authority_identity &&
          after_destruction.source_owner_identity ==
              before_destruction.source_owner_identity &&
          after_destruction.source_cloud_identity ==
              before_destruction.source_cloud_identity &&
          after_destruction.source_snapshot_epoch ==
              before_destruction.source_snapshot_epoch &&
          after_destruction.candidate_buffer_epoch ==
              before_destruction.candidate_buffer_epoch &&
          after_destruction.point_count == before_destruction.point_count &&
          after_destruction.certified_node_count ==
              before_destruction.certified_node_count &&
          after_destruction.retained_coordinate_word_capacity ==
              before_destruction.retained_coordinate_word_capacity &&
          after_destruction.retained_morton_point_id_capacity ==
              before_destruction.retained_morton_point_id_capacity &&
          after_destruction.retained_node_capacity ==
              before_destruction.retained_node_capacity,
      "the detached private capability preserves the same source owner, identity, snapshot and extents after context destruction");
}

void test_censure_publishes_only_the_maximal_complete_prefix() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  auto lease = traversal_lease(6U);
  MortonYao48DeviceTiledPairFrontierContext context{
      std::move(lease), MortonYao48DeviceTiledPairFrontierConfig{3U, 4U}};
  FakeMortonYao48DeviceTiledAnchor censored = complete_anchor(1U, 0U, 0U);
  censored.complete = false;
  censored.node_visit_count =
      morsehgp3d::gpu::
          morton_yao48_device_tiled_pair_frontier_node_visits_per_anchor;
  censored.stop_reason =
      MortonYao48DeviceTiledPairFrontierStopReason::node_visit_capacity;
  configure_fake_gpu_morton_yao48_device_tiled_pair_frontier(
      FakeMortonYao48DeviceTiledPairFrontierConfiguration{
          {complete_anchor(1U, 0U, 0U),
           complete_anchor(1U, 1U, 1U),
           censored,
           complete_anchor(4U, 0U, 0U)},
          FakeMortonYao48DeviceTiledPairFrontierCorruption::none});

  auto advance = context.advance();
  check(
      advance.status ==
              MortonYao48DeviceTiledPairFrontierStatus::censored &&
          advance.stop_reason ==
              MortonYao48DeviceTiledPairFrontierStopReason::
                  node_visit_capacity &&
          context.terminally_censored() &&
          advance.candidate_tile.has_value() &&
          advance.candidate_tile->audit().anchor_begin == 1U &&
          advance.candidate_tile->audit().anchor_end == 3U &&
          advance.candidate_tile->audit().candidate_count == 2U &&
          advance.candidate_tile->audit().certified_prune_region_count ==
              1U &&
          advance.candidate_tile->audit().censored_anchor_outputs_withheld,
      "the censored anchor and every physical suffix output stay unpublished");
  check(
      advance.audit.transaction_anchor_begin == 1U &&
          advance.audit.transaction_anchor_end == 5U &&
          advance.audit.transaction_committed_anchor_count == 2U &&
          advance.audit.completed_anchor_count == 2U &&
          advance.audit.next_anchor_position == 3U &&
          advance.audit.cumulative_candidate_pair_mass == 2U &&
          advance.audit.cumulative_certified_pruned_pair_mass == 1U &&
          advance.audit.unresolved_pair_mass == 12U &&
          advance.audit.transaction_physical_node_visit_count == 2055U &&
          advance.audit.censored_anchor_outputs_withheld &&
          advance.audit.terminally_censored &&
          !advance.audit.pair_coverage_partition_complete,
      "censure commits the longest complete prefix but audits all bounded physical work");

  const std::size_t launch_count =
      fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count();
  advance.candidate_tile.reset();
  auto stable = context.advance();
  check(
      stable.status == MortonYao48DeviceTiledPairFrontierStatus::censored &&
          stable.stop_reason == advance.stop_reason &&
          !stable.candidate_tile.has_value() &&
          stable.audit.unresolved_pair_mass == 12U &&
          fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count() ==
              launch_count,
      "terminal censure cannot be bypassed by another advance call");
}

void test_malformed_launcher_outputs_poison_fail_stop() {
  const std::vector<FakeMortonYao48DeviceTiledPairFrontierCorruption>
      corruptions{
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              stale_output_epoch,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              candidate_device_to_host,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              forged_cuda_execution,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              missing_output_owner,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              shared_output_owner,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              corrupt_metadata_digest,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              nonzero_failure_code,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              wrong_anchor_position,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              foreign_source_cloud_identity,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              pruned_mass_without_region,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              too_many_regions_for_pruned_mass,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              outputs_exceed_node_visits,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              simulated_launcher_failure};
  for (const auto corruption : corruptions) {
    reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
    auto lease = traversal_lease(4U);
    MortonYao48DeviceTiledPairFrontierContext context{
        std::move(lease), MortonYao48DeviceTiledPairFrontierConfig{2U, 2U}};
    configure_fake_gpu_morton_yao48_device_tiled_pair_frontier(
        FakeMortonYao48DeviceTiledPairFrontierConfiguration{
            {}, corruption});
    check_throws<std::runtime_error>(
        [&context] { (void)context.advance(); },
        "a hostile Phase 15 launcher transcript is rejected");
    check(
        context.poisoned() && !context.ready(),
        "a rejected launcher transcript poisons the context");
    const std::size_t launch_count =
        fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count();
    check_throws<std::runtime_error>(
        [&context] { (void)context.advance(); },
        "a poisoned Phase 15 context fails closed on reuse");
    check(
        fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count() ==
            launch_count,
        "poisoned reuse cannot submit another launcher call");
  }
}

void test_preflight_and_trivial_frontier() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  auto lease = traversal_lease(3U);
  check_throws<std::out_of_range>(
      [&lease] {
        MortonYao48DeviceTiledPairFrontierContext invalid{
            std::move(lease),
            MortonYao48DeviceTiledPairFrontierConfig{2U, 0U}};
      },
      "an invalid anchor tile is rejected before lease adoption");
  check(
      lease.ready(),
      "configuration preflight leaves the source traversal lease usable");

  auto singleton_lease = traversal_lease(1U);
  MortonYao48DeviceTiledPairFrontierContext singleton{
      std::move(singleton_lease)};
  auto complete = singleton.advance();
  check(
      complete.status ==
              MortonYao48DeviceTiledPairFrontierStatus::frontier_complete &&
          complete.audit.unordered_pair_universe_count == 0U &&
          complete.audit.unresolved_pair_mass == 0U &&
          complete.audit.pair_coverage_partition_complete &&
          !complete.candidate_tile.has_value() &&
          fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count() ==
              0U,
      "a singleton closes its empty pair universe without a launcher");
}

}  // namespace

int main() {
  test_complete_tiles_close_one_coverage_partition();
  test_detached_tile_retains_private_source_capability();
  test_censure_publishes_only_the_maximal_complete_prefix();
  test_malformed_launcher_outputs_poison_fail_stop();
  test_preflight_and_trivial_frontier();
  if (failures != 0) {
    std::cerr << failures
              << " device tiled Morton/Yao48 contract test(s) failed\n";
    return 1;
  }
  std::cout << "device tiled Morton/Yao48 contract tests passed\n";
  return 0;
}
