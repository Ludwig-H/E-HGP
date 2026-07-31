#include "fake_gpu_morton_yao48_device_tiled_pair_frontier_launchers.hpp"
#include "fake_gpu_morton_yao48_ranked_pair_tile_classifier_launchers.hpp"
#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/morton_yao48_device_tiled_pair_frontier.hpp"
#include "morsehgp3d/gpu/morton_yao48_ranked_pair_tile_classifier.hpp"
#include "phase15_morton_yao48_ranked_pair_tile_classifier_internal.hpp"

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
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierPruneSemantics;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierYieldReason;
using morsehgp3d::gpu::MortonYao48RankedPairTileCatalogLease;
using morsehgp3d::gpu::MortonYao48RankedPairTileClassifierConfig;
using morsehgp3d::gpu::MortonYao48RankedPairTileClassifierContext;
using morsehgp3d::gpu::MortonYao48RankedPairTileClassifierStatus;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48RankedPairTileCatalogPrivateViewAccess;
using morsehgp3d::gpu::MortonYao48RankedPairTileClassifierStopReason;
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
    FakeMortonYao48RankedPairTileClassifierCorruption;
using morsehgp3d::gpu::test_support::
    FakeMortonYao48RankedPairTileClassifierLaunch;
using morsehgp3d::gpu::test_support::
    configure_fake_gpu_morton_yao48_device_tiled_pair_frontier;
using morsehgp3d::gpu::test_support::
    configure_fake_gpu_morton_yao48_ranked_pair_tile_classifier;
using morsehgp3d::gpu::test_support::
    fake_gpu_morton_yao48_ranked_pair_tile_classifier_launch_count;
using morsehgp3d::gpu::test_support::
    fake_gpu_morton_yao48_ranked_pair_tile_classifier_live_lease_count;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_morton_yao48_device_tiled_pair_frontier;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_morton_yao48_ranked_pair_tile_classifier;
using morsehgp3d::gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::spatial::CanonicalPointCloud;

static_assert(!std::is_copy_constructible_v<
              MortonYao48RankedPairTileClassifierContext>);
static_assert(std::is_nothrow_move_constructible_v<
              MortonYao48RankedPairTileClassifierContext>);
static_assert(!std::is_copy_constructible_v<
              MortonYao48RankedPairTileCatalogLease>);
static_assert(std::is_nothrow_move_constructible_v<
              MortonYao48RankedPairTileCatalogLease>);

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
    std::uint64_t node_visits) {
  FakeMortonYao48DeviceTiledAnchor anchor;
  anchor.candidate_count = candidates;
  anchor.prune_region_count = prune_regions;
  anchor.certified_pruned_pair_mass = pruned_mass;
  anchor.node_visit_count = node_visits;
  return anchor;
}

[[nodiscard]] FakeMortonYao48DeviceTiledAnchor candidate_chunk() {
  auto anchor = complete_anchor(640U, 0U, 0U, 640U);
  anchor.status = FakeMortonYao48DeviceTiledAnchorStatus::chunk_ready;
  anchor.yield_reason =
      MortonYao48DeviceTiledPairFrontierYieldReason::candidate_segment_full;
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

void test_two_chunk_exact_closure_and_lease_lifetime() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  reset_fake_gpu_morton_yao48_ranked_pair_tile_classifier();
  auto traversal = traversal_lease(642U);
  MortonYao48DeviceTiledPairFrontierContext frontier{
      std::move(traversal),
      MortonYao48DeviceTiledPairFrontierConfig{3U, 641U}};
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
  MortonYao48RankedPairTileClassifierContext classifier{
      MortonYao48RankedPairTileClassifierConfig{3U, 401U, 401U, 2U}};

  auto first = frontier.advance();
  auto first_commit = classifier.commit(std::move(first));
  check(
      first_commit.status ==
              MortonYao48RankedPairTileClassifierStatus::chunk_committed &&
          first_commit.audit.transaction_candidate_count == 640U &&
          first_commit.audit.transaction_accepted_record_count == 400U &&
          first_commit.audit.transaction_above_window_count == 240U &&
          first_commit.audit.frontier_unresolved_pair_mass == 1U &&
          first_commit.audit.candidate_tile_lease_consumed &&
          first_commit.audit.candidate_tile_lease_retained_during_launch &&
          first_commit.audit.frontier_global_mass_validated &&
          fake_gpu_morton_yao48_ranked_pair_tile_classifier_live_lease_count() ==
              0U,
      "the first resident chunk commits exact multi-order mass and releases "
      "its lease");
  check_throws<std::logic_error>(
      [&classifier] { (void)classifier.finish(); },
      "finish is forbidden before an authenticated frontier closure");

  auto second = frontier.advance();
  auto second_commit = classifier.commit(std::move(second));
  check(
      second_commit.status ==
              MortonYao48RankedPairTileClassifierStatus::frontier_closed &&
          second_commit.audit.cumulative_candidate_count == 641U &&
          second_commit.audit.cumulative_accepted_record_count == 401U &&
          second_commit.audit.cumulative_above_window_count == 240U &&
          second_commit.audit.cumulative_certified_pruned_pair_mass ==
              205'120U &&
          second_commit.audit.frontier_unresolved_pair_mass == 0U &&
          second_commit.audit.unordered_pair_universe_count == 205'761U,
      "the resumed chunk authenticates the exact global closure");
  auto catalog = classifier.finish();
  const auto private_views =
      Phase15MortonYao48RankedPairTileCatalogPrivateViewAccess::inspect(
          catalog);
  check(
      catalog.ready() && catalog.host_fake() &&
          catalog.audit().closed_rank_catalog_complete &&
          catalog.audit().source_owner_retained &&
          catalog.audit().source_cloud_identity_retained &&
          !catalog.audit().source_device_coordinates_retained &&
          catalog.audit().catalog_record_count == 401U &&
          catalog.audit().candidate_count == 641U &&
          catalog.audit().above_window_count == 240U &&
          catalog.audit().certified_pruned_pair_mass == 205'120U &&
          catalog.audit().unordered_pair_universe_count == 205'761U &&
          catalog.audit().committed_chunk_count == 2U &&
          private_views.ready && private_views.host_fake &&
          private_views.output_owner &&
          private_views.source_owner_authority &&
          private_views.source_cloud_identity_authority &&
          private_views.device_coordinate_bits == nullptr &&
          private_views.rank_offset_count == 5U &&
          private_views.device_rank_offsets[0U] == 0U &&
          private_views.device_rank_offsets[1U] == 0U &&
          private_views.device_rank_offsets[2U] == 0U &&
          private_views.device_rank_offsets[3U] == 0U &&
          private_views.device_rank_offsets[4U] == 401U &&
          private_views.device_closed_rank[0U] == 3U &&
          private_views.device_closed_rank[400U] == 3U &&
          private_views.device_strict_offsets[401U] == 300U &&
          private_views.device_shell_offsets[401U] == 101U,
      "finish publishes the resident catalog only after full frontier "
      "closure while retaining the source authority needed to derive exact "
      "levels");
}

void test_vacuous_frontier_closes_without_candidate_lease() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  reset_fake_gpu_morton_yao48_ranked_pair_tile_classifier();
  auto traversal = traversal_lease(1U);
  MortonYao48DeviceTiledPairFrontierContext frontier{std::move(traversal)};
  MortonYao48RankedPairTileClassifierContext classifier{
      MortonYao48RankedPairTileClassifierConfig{2U, 0U, 0U, 0U}};
  auto empty = frontier.advance();
  check(!empty.candidate_tile.has_value(),
        "the singleton frontier legitimately has no candidate lease");
  auto commit = classifier.commit(std::move(empty));
  check(
      commit.status ==
              MortonYao48RankedPairTileClassifierStatus::frontier_closed &&
          commit.audit.transaction_candidate_count == 0U &&
          commit.audit.transaction_certified_pruned_pair_mass == 0U &&
          !commit.audit.candidate_tile_lease_consumed &&
          commit.audit.launcher_call_count == 0U &&
          fake_gpu_morton_yao48_ranked_pair_tile_classifier_launch_count() ==
              0U,
      "a zero-delta frontier closure is committed without fabricating a "
      "launcher call");
  auto catalog = classifier.finish();
  check(
      catalog.ready() && !catalog.host_fake() && !catalog.cuda_resident() &&
          !catalog.audit().source_owner_retained &&
          !catalog.audit().source_cloud_identity_retained &&
          !catalog.audit().source_device_coordinates_retained &&
          catalog.audit().catalog_record_count == 0U &&
          catalog.audit().unordered_pair_universe_count == 0U,
      "the no-lease closure publishes a vacuous catalog authority");
}

void test_all_linear_capacities_fail_closed() {
  const std::vector<MortonYao48RankedPairTileClassifierStopReason> reasons{
      MortonYao48RankedPairTileClassifierStopReason::catalog_record_capacity,
      MortonYao48RankedPairTileClassifierStopReason::
          payload_point_id_capacity,
      MortonYao48RankedPairTileClassifierStopReason::exact_fallback_capacity};
  for (const auto reason : reasons) {
    reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
    reset_fake_gpu_morton_yao48_ranked_pair_tile_classifier();
    auto traversal = traversal_lease(3U);
    MortonYao48DeviceTiledPairFrontierContext frontier{
        std::move(traversal)};
    auto launch =
        reason == MortonYao48RankedPairTileClassifierStopReason::
                      exact_fallback_capacity
            ? exact_launch(0U, 1U, 0U, 0U, 2U)
            : exact_launch(3U, 0U, 2U, 2U);
    launch.capacity_stop_reason = reason;
    configure_classifier_transcripts({launch});
    MortonYao48RankedPairTileClassifierContext classifier{
        MortonYao48RankedPairTileClassifierConfig{2U, 2U, 3U, 1U}};
    auto advance = frontier.advance();
    auto commit = classifier.commit(std::move(advance));
    check(
        commit.status ==
                MortonYao48RankedPairTileClassifierStatus::
                    capacity_exhausted &&
            commit.stop_reason == reason && commit.audit.output_withheld &&
            commit.audit.transaction_unresolved_count == 3U &&
            commit.audit.cumulative_unresolved_count == 3U &&
            classifier.terminally_censored() && !classifier.ready(),
        "each linear arena cap fails closed with all current candidates "
        "unresolved");
    check_throws<std::logic_error>(
        [&classifier] { (void)classifier.finish(); },
        "a capacity-censored context cannot publish a partial catalog");
  }
}

void test_unconsumed_exact_fallback_fails_closed_without_poisoning() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  reset_fake_gpu_morton_yao48_ranked_pair_tile_classifier();
  auto traversal = traversal_lease(3U);
  MortonYao48DeviceTiledPairFrontierContext frontier{std::move(traversal)};
  configure_classifier_transcripts({exact_launch(0U, 1U, 0U, 0U, 2U)});
  MortonYao48RankedPairTileClassifierContext classifier{
      MortonYao48RankedPairTileClassifierConfig{2U, 3U, 2U, 2U}};

  auto advance = frontier.advance();
  const auto commit = classifier.commit(std::move(advance));
  check(
      commit.status ==
              MortonYao48RankedPairTileClassifierStatus::
                  exact_predicate_failure &&
          commit.stop_reason ==
              MortonYao48RankedPairTileClassifierStopReason::
                  exact_predicate_failure &&
          commit.audit.transaction_exact_fallback_count == 2U &&
          commit.audit.cumulative_exact_fallback_count == 2U &&
          commit.audit.transaction_unresolved_count == 3U &&
          commit.audit.output_withheld && classifier.terminally_censored() &&
          !classifier.poisoned() && !classifier.ready(),
      "an exact fixed-limb fallback requirement is authenticated and "
      "withheld without being misclassified as receipt corruption");
  check_throws<std::logic_error>(
      [&classifier] { (void)classifier.finish(); },
      "an unconsumed exact fallback cannot publish a partial catalog");
}

void test_corrupt_receipts_poison_without_committing() {
  const std::vector<FakeMortonYao48RankedPairTileClassifierCorruption>
      corruptions{
          FakeMortonYao48RankedPairTileClassifierCorruption::
              stale_source_epoch,
          FakeMortonYao48RankedPairTileClassifierCorruption::
              stale_candidate_epoch,
          FakeMortonYao48RankedPairTileClassifierCorruption::
              stale_chunk_sequence,
          FakeMortonYao48RankedPairTileClassifierCorruption::
              cumulative_record_rollback,
          FakeMortonYao48RankedPairTileClassifierCorruption::
              corrupt_receipt_digest,
          FakeMortonYao48RankedPairTileClassifierCorruption::
              candidate_device_to_host,
          FakeMortonYao48RankedPairTileClassifierCorruption::
              callback_per_pair,
          FakeMortonYao48RankedPairTileClassifierCorruption::dense_pair_scan,
          FakeMortonYao48RankedPairTileClassifierCorruption::
              missing_output_owner,
          FakeMortonYao48RankedPairTileClassifierCorruption::
              invalid_closed_rank,
          FakeMortonYao48RankedPairTileClassifierCorruption::
              unsorted_record_order,
          FakeMortonYao48RankedPairTileClassifierCorruption::
              invalid_rank_offsets,
          FakeMortonYao48RankedPairTileClassifierCorruption::
              invalid_payload_offsets,
          FakeMortonYao48RankedPairTileClassifierCorruption::
              invalid_payload_point_id};
  for (const auto corruption : corruptions) {
    reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
    reset_fake_gpu_morton_yao48_ranked_pair_tile_classifier();
    auto traversal = traversal_lease(3U);
    MortonYao48DeviceTiledPairFrontierContext frontier{
        std::move(traversal),
        MortonYao48DeviceTiledPairFrontierConfig{3U, 4096U}};
    auto launch = exact_launch(3U, 0U, 1U, 0U);
    launch.corruption = corruption;
    configure_classifier_transcripts({launch});
    MortonYao48RankedPairTileClassifierContext classifier{
        MortonYao48RankedPairTileClassifierConfig{3U, 3U, 1U, 0U}};
    auto advance = frontier.advance();
    check_throws<std::runtime_error>(
        [&classifier, &advance] {
          (void)classifier.commit(std::move(advance));
        },
        "a stale, rolled-back, copied, callback, dense, or malformed "
        "receipt is rejected");
    check(
        classifier.poisoned() && !classifier.ready() &&
            !classifier.frontier_closed(),
        "receipt corruption poisons the context without committing "
        "frontier closure");
  }
}

void test_impossible_host_fake_payload_mass_poisoned() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  reset_fake_gpu_morton_yao48_ranked_pair_tile_classifier();
  auto traversal = traversal_lease(3U);
  MortonYao48DeviceTiledPairFrontierContext frontier{
      std::move(traversal),
      MortonYao48DeviceTiledPairFrontierConfig{3U, 4096U}};
  configure_classifier_transcripts({exact_launch(3U, 0U, 2U, 2U)});
  MortonYao48RankedPairTileClassifierContext classifier{
      MortonYao48RankedPairTileClassifierConfig{3U, 3U, 4U, 0U}};
  auto advance = frontier.advance();
  check_throws<std::invalid_argument>(
      [&classifier, &advance] {
        (void)classifier.commit(std::move(advance));
      },
      "a host-fake transcript cannot assign more than rank minus two "
      "non-support points per record");
  check(
      classifier.poisoned() && !classifier.ready() &&
          !classifier.frontier_closed(),
      "an impossible host-fake payload mass poisons the context before "
      "authority publication");
}

void test_rank_window_preflight() {
  check_throws<std::out_of_range>(
      [] {
        MortonYao48RankedPairTileClassifierContext invalid{
            MortonYao48RankedPairTileClassifierConfig{1U, 0U, 0U, 0U}};
      },
      "closed rank one is below the resident classifier contract");
  check_throws<std::out_of_range>(
      [] {
        MortonYao48RankedPairTileClassifierContext invalid{
            MortonYao48RankedPairTileClassifierConfig{12U, 0U, 0U, 0U}};
      },
      "closed rank twelve is above the resident classifier contract");
}

void test_strict_frontier_is_rejected_before_closed_rank_launch() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  reset_fake_gpu_morton_yao48_ranked_pair_tile_classifier();
  auto traversal = traversal_lease(3U);
  MortonYao48DeviceTiledPairFrontierContext frontier{
      std::move(traversal),
      MortonYao48DeviceTiledPairFrontierConfig{
          2U,
          2U,
          MortonYao48DeviceTiledPairFrontierPruneSemantics::
              strict_interior_threshold}};
  configure_fake_gpu_morton_yao48_device_tiled_pair_frontier({});
  auto strict_advance = frontier.advance();
  check(
      strict_advance.candidate_tile.has_value(),
      "the strict-frontier rejection fixture owns a resident tile");
  MortonYao48RankedPairTileClassifierContext classifier{
      MortonYao48RankedPairTileClassifierConfig{2U, 3U, 3U, 1U}};
  check_throws<std::runtime_error>(
      [&classifier, &strict_advance] {
        (void)classifier.commit(std::move(strict_advance));
      },
      "the closed-rank classifier rejects a strict-interior frontier lease");
  check(
      fake_gpu_morton_yao48_ranked_pair_tile_classifier_launch_count() == 0U,
      "strict-interior authority is rejected before the fake closed-rank "
      "launcher");
}

}  // namespace

int main() {
  test_two_chunk_exact_closure_and_lease_lifetime();
  test_vacuous_frontier_closes_without_candidate_lease();
  test_all_linear_capacities_fail_closed();
  test_unconsumed_exact_fallback_fails_closed_without_poisoning();
  test_corrupt_receipts_poison_without_committing();
  test_impossible_host_fake_payload_mass_poisoned();
  test_rank_window_preflight();
  test_strict_frontier_is_rejected_before_closed_rank_launch();
  if (failures != 0) {
    std::cerr << failures
              << " resident ranked-pair tile classifier test(s) failed\n";
    return 1;
  }
  std::cout << "resident ranked-pair tile classifier tests passed\n";
  return 0;
}
