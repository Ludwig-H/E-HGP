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
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierPruneSemantics;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierStatus;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierStopReason;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierYieldReason;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceCandidateTilePrivateViewAccess;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledAnchorCheckpoint;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledAnchorControl;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledCandidateRecord;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledPruneRegionRecord;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledWitnessBankSlot;
using morsehgp3d::gpu::detail::
    phase15_morton_yao48_device_tiled_witness_lower_bound_certifies;
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
    configure_fake_gpu_morton_yao48_device_tiled_pair_frontier;
using morsehgp3d::gpu::test_support::
    fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_morton_yao48_device_tiled_pair_frontier;
using morsehgp3d::gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::spatial::CanonicalPointCloud;

static_assert(sizeof(Phase15MortonYao48DeviceTiledAnchorControl) == 168U);
static_assert(sizeof(Phase15MortonYao48DeviceTiledAnchorCheckpoint) == 160U);
static_assert(!std::is_copy_constructible_v<
              MortonYao48DeviceCandidateTileLease>);
static_assert(std::is_nothrow_move_constructible_v<
              MortonYao48DeviceCandidateTileLease>);
static_assert(!std::is_copy_constructible_v<
              MortonYao48DeviceTiledPairFrontierContext>);
static_assert(std::is_nothrow_move_constructible_v<
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
    std::uint64_t node_visits) {
  FakeMortonYao48DeviceTiledAnchor anchor;
  anchor.candidate_count = candidates;
  anchor.prune_region_count = prune_regions;
  anchor.certified_pruned_pair_mass = pruned_mass;
  anchor.node_visit_count = node_visits;
  return anchor;
}

[[nodiscard]] FakeMortonYao48DeviceTiledAnchor completed_zero_delta() {
  return complete_anchor(0U, 0U, 0U, 0U);
}

[[nodiscard]] FakeMortonYao48DeviceTiledAnchor complete_by_one_prune(
    std::uint64_t mass) {
  return complete_anchor(0U, 1U, mass, 1U);
}

[[nodiscard]] FakeMortonYao48DeviceTiledAnchor candidate_chunk(
    std::uint64_t count = 640U) {
  FakeMortonYao48DeviceTiledAnchor anchor =
      complete_anchor(count, 0U, 0U, count);
  anchor.status = FakeMortonYao48DeviceTiledAnchorStatus::chunk_ready;
  anchor.yield_reason =
      MortonYao48DeviceTiledPairFrontierYieldReason::
          candidate_segment_full;
  return anchor;
}

[[nodiscard]] FakeMortonYao48DeviceTiledAnchor prune_chunk(
    std::uint64_t mass = 2'048U) {
  FakeMortonYao48DeviceTiledAnchor anchor =
      complete_anchor(0U, 2'048U, mass, 2'048U);
  anchor.status = FakeMortonYao48DeviceTiledAnchorStatus::chunk_ready;
  anchor.yield_reason =
      MortonYao48DeviceTiledPairFrontierYieldReason::prune_segment_full;
  return anchor;
}

[[nodiscard]] FakeMortonYao48DeviceTiledAnchor node_capacity_fatal(
    std::uint64_t certified_node_count) {
  FakeMortonYao48DeviceTiledAnchor anchor;
  anchor.node_visit_count = certified_node_count;
  anchor.status = FakeMortonYao48DeviceTiledAnchorStatus::fatal;
  anchor.stop_reason =
      MortonYao48DeviceTiledPairFrontierStopReason::node_visit_capacity;
  return anchor;
}

[[nodiscard]] std::size_t expected_arena_bytes(
    std::size_t anchor_count,
    std::size_t maximum_closed_rank) {
  return anchor_count * 640U *
             sizeof(Phase15MortonYao48DeviceTiledCandidateRecord) +
      anchor_count * 2'048U *
          sizeof(Phase15MortonYao48DeviceTiledPruneRegionRecord) +
      anchor_count * 48U * (maximum_closed_rank - 1U) *
          sizeof(Phase15MortonYao48DeviceTiledWitnessBankSlot) +
      anchor_count * sizeof(Phase15MortonYao48DeviceTiledAnchorControl) +
      anchor_count * sizeof(Phase15MortonYao48DeviceTiledAnchorCheckpoint) +
      sizeof(std::uint64_t);
}

[[nodiscard]] std::size_t expected_arena_bytes_for_witness_count(
    std::size_t anchor_count,
    std::size_t required_witness_count) {
  return anchor_count * 640U *
             sizeof(Phase15MortonYao48DeviceTiledCandidateRecord) +
      anchor_count * 2'048U *
          sizeof(Phase15MortonYao48DeviceTiledPruneRegionRecord) +
      anchor_count * 48U * required_witness_count *
          sizeof(Phase15MortonYao48DeviceTiledWitnessBankSlot) +
      anchor_count * sizeof(Phase15MortonYao48DeviceTiledAnchorControl) +
      anchor_count * sizeof(Phase15MortonYao48DeviceTiledAnchorCheckpoint) +
      sizeof(std::uint64_t);
}

[[nodiscard]] std::vector<FakeMortonYao48DeviceTiledAnchor>
complete_prefix_by_prune(std::size_t count) {
  std::vector<FakeMortonYao48DeviceTiledAnchor> anchors;
  anchors.reserve(count);
  for (std::size_t position = 1U; position <= count; ++position) {
    anchors.push_back(complete_by_one_prune(position));
  }
  return anchors;
}

[[nodiscard]] std::vector<FakeMortonYao48DeviceTiledAnchor>
completed_prefix_zero_delta(std::size_t count) {
  return std::vector<FakeMortonYao48DeviceTiledAnchor>(
      count, completed_zero_delta());
}

void configure_transcripts(
    std::vector<FakeMortonYao48DeviceTiledPairFrontierLaunch> launches) {
  FakeMortonYao48DeviceTiledPairFrontierConfiguration configuration;
  configuration.launches = std::move(launches);
  configure_fake_gpu_morton_yao48_device_tiled_pair_frontier(
      std::move(configuration));
}

void test_exact_640_closes_without_empty_resume() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  auto traversal = traversal_lease(641U);
  MortonYao48DeviceTiledPairFrontierContext context{
      std::move(traversal),
      MortonYao48DeviceTiledPairFrontierConfig{2U, 640U}};
  configure_fake_gpu_morton_yao48_device_tiled_pair_frontier({});

  auto closed = context.advance();
  check(
      closed.status ==
              MortonYao48DeviceTiledPairFrontierStatus::frontier_complete &&
          closed.yield_reason ==
              MortonYao48DeviceTiledPairFrontierYieldReason::none &&
          closed.candidate_tile.has_value() &&
          closed.candidate_tile->audit().candidate_count ==
              closed.audit.unordered_pair_universe_count &&
          closed.audit.cumulative_candidate_pair_mass ==
              closed.audit.unordered_pair_universe_count &&
          closed.audit.next_anchor_position == 641U &&
          closed.audit.completed_anchor_count == 640U &&
          !closed.audit.resumable_capacity_yield &&
          !closed.audit.process_restart_resumable &&
          fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count() ==
              1U,
      "an exact 640-record closure completes without an empty resume");
}

void test_candidate_640_plus_one_resumes_same_tile_with_backpressure() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  auto traversal = traversal_lease(642U);
  MortonYao48DeviceTiledPairFrontierContext context{
      std::move(traversal),
      MortonYao48DeviceTiledPairFrontierConfig{2U, 641U}};

  auto first_anchors = complete_prefix_by_prune(640U);
  first_anchors.push_back(candidate_chunk());
  auto second_anchors = completed_prefix_zero_delta(640U);
  second_anchors.push_back(complete_anchor(1U, 0U, 0U, 1U));
  configure_transcripts({
      {std::move(first_anchors),
       FakeMortonYao48DeviceTiledPairFrontierCorruption::none},
      {std::move(second_anchors),
       FakeMortonYao48DeviceTiledPairFrontierCorruption::none}});

  auto first = context.advance();
  check(
      first.status == MortonYao48DeviceTiledPairFrontierStatus::chunk_ready &&
          first.yield_reason ==
              MortonYao48DeviceTiledPairFrontierYieldReason::
                  candidate_segment_full &&
          first.candidate_tile.has_value() &&
          first.audit.tile_epoch != 0U && first.audit.chunk_sequence == 1U &&
          !first.audit.resumes_same_tile &&
          first.audit.resumable_capacity_yield &&
          first.audit.next_anchor_position == 1U &&
          first.audit.completed_anchor_count == 0U &&
          first.audit.unresolved_pair_mass == 1U &&
          first.candidate_tile->audit().resumable_after_lease_release &&
          !first.candidate_tile->audit().process_restart_resumable,
      "640+1 yields one resumable candidate chunk without advancing the tile");
  const std::uint64_t tile_epoch = first.audit.tile_epoch;
  const std::size_t launches_before_backpressure =
      fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count();
  check_throws<std::logic_error>(
      [&context] { (void)context.advance(); },
      "a live chunk lease applies one-lease backpressure");
  check(
      fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count() ==
          launches_before_backpressure &&
          !context.poisoned(),
      "backpressure launches no work and does not poison the context");
  first.candidate_tile.reset();

  auto second = context.advance();
  check(
      second.status ==
              MortonYao48DeviceTiledPairFrontierStatus::frontier_complete &&
          second.candidate_tile.has_value() &&
          second.candidate_tile->audit().candidate_count == 1U &&
          second.audit.tile_epoch == tile_epoch &&
          second.audit.chunk_sequence == 2U && second.audit.resumes_same_tile &&
          second.audit.next_anchor_position == 642U &&
          second.audit.completed_anchor_count == 641U &&
          second.audit.unresolved_pair_mass == 0U &&
          second.audit.cumulative_candidate_pair_mass +
                  second.audit.cumulative_certified_pruned_pair_mass ==
              second.audit.unordered_pair_universe_count,
      "the +1 chunk closes the same tile exactly once after lease release");
}

void test_prune_2048_resumes_without_duplication() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  auto traversal = traversal_lease(2'050U);
  MortonYao48DeviceTiledPairFrontierContext context{
      std::move(traversal),
      MortonYao48DeviceTiledPairFrontierConfig{2U, 2'049U}};

  auto first_anchors = complete_prefix_by_prune(2'048U);
  first_anchors.push_back(prune_chunk());
  auto second_anchors = completed_prefix_zero_delta(2'048U);
  second_anchors.push_back(complete_anchor(1U, 0U, 0U, 1U));
  configure_transcripts({
      {std::move(first_anchors),
       FakeMortonYao48DeviceTiledPairFrontierCorruption::none},
      {std::move(second_anchors),
       FakeMortonYao48DeviceTiledPairFrontierCorruption::none}});

  auto first = context.advance();
  check(
      first.status == MortonYao48DeviceTiledPairFrontierStatus::chunk_ready &&
          first.yield_reason ==
              MortonYao48DeviceTiledPairFrontierYieldReason::
                  prune_segment_full &&
          first.candidate_tile.has_value() &&
          first.audit.transaction_certified_prune_region_count == 4'096U &&
          first.audit.next_anchor_position == 1U,
      "a full 2048-region segment is a resumable prune chunk");
  first.candidate_tile.reset();
  auto second = context.advance();
  check(
      second.status ==
              MortonYao48DeviceTiledPairFrontierStatus::frontier_complete &&
          second.audit.unresolved_pair_mass == 0U &&
          second.audit.cumulative_candidate_pair_mass == 1U &&
          second.audit.cumulative_candidate_pair_mass +
                  second.audit.cumulative_certified_pruned_pair_mass ==
              second.audit.unordered_pair_universe_count,
      "the resumed prune tile closes its universe without duplicated mass");
}

void test_mixed_candidate_and_prune_yields_are_reported_honestly() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  auto traversal = traversal_lease(2'050U);
  MortonYao48DeviceTiledPairFrontierContext context{
      std::move(traversal),
      MortonYao48DeviceTiledPairFrontierConfig{2U, 2'049U}};
  auto anchors = complete_prefix_by_prune(2'049U);
  anchors[640U] = candidate_chunk();
  anchors[2'048U] = prune_chunk();
  configure_transcripts({
      {std::move(anchors),
       FakeMortonYao48DeviceTiledPairFrontierCorruption::none}});

  auto mixed = context.advance();
  check(
      mixed.status ==
              MortonYao48DeviceTiledPairFrontierStatus::chunk_ready &&
          mixed.yield_reason ==
              MortonYao48DeviceTiledPairFrontierYieldReason::
                  mixed_segments_full &&
          mixed.audit.yield_reason ==
              MortonYao48DeviceTiledPairFrontierYieldReason::
                  mixed_segments_full &&
          mixed.candidate_tile.has_value() &&
          mixed.candidate_tile->audit().yield_reason ==
              MortonYao48DeviceTiledPairFrontierYieldReason::
                  mixed_segments_full,
      "simultaneous candidate and prune saturation is reported as mixed");
}

void test_chunk_subdivisions_are_not_a_global_call_ceiling() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  auto traversal = traversal_lease(1'922U);
  MortonYao48DeviceTiledPairFrontierContext context{
      std::move(traversal),
      MortonYao48DeviceTiledPairFrontierConfig{2U, 1'921U}};

  auto first = complete_prefix_by_prune(1'920U);
  first.push_back(candidate_chunk());
  auto second = completed_prefix_zero_delta(1'920U);
  second.push_back(candidate_chunk());
  auto third = completed_prefix_zero_delta(1'920U);
  third.push_back(candidate_chunk());
  auto fourth = completed_prefix_zero_delta(1'920U);
  fourth.push_back(complete_anchor(1U, 0U, 0U, 1U));
  configure_transcripts({
      {std::move(first),
       FakeMortonYao48DeviceTiledPairFrontierCorruption::none},
      {std::move(second),
       FakeMortonYao48DeviceTiledPairFrontierCorruption::none},
      {std::move(third),
       FakeMortonYao48DeviceTiledPairFrontierCorruption::none},
      {std::move(fourth),
       FakeMortonYao48DeviceTiledPairFrontierCorruption::none}});

  auto one = context.advance();
  one.candidate_tile.reset();
  auto two = context.advance();
  two.candidate_tile.reset();
  auto three = context.advance();
  three.candidate_tile.reset();
  auto four = context.advance();
  check(
      four.status ==
              MortonYao48DeviceTiledPairFrontierStatus::frontier_complete &&
          four.audit.cumulative_traversal_subdivision_count == 4U &&
          four.audit.maximum_traversal_subdivision_count_per_anchor == 2U &&
          four.audit.cumulative_physical_node_visit_count <=
              four.audit.certified_node_count *
                  four.audit.transaction_anchor_end &&
          four.audit.unresolved_pair_mass == 0U,
      "inter-chunk launcher calls may exceed the per-call subdivision maximum");
}

void test_stale_and_rollback_transcripts_poison_fail_stop() {
  const std::vector<FakeMortonYao48DeviceTiledPairFrontierCorruption>
      corruptions{
          FakeMortonYao48DeviceTiledPairFrontierCorruption::stale_tile_epoch,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              stale_chunk_sequence,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              cumulative_candidate_rollback,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              cumulative_prune_rollback,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              process_restart_claimed};
  for (const auto corruption : corruptions) {
    reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
    auto traversal = traversal_lease(2'050U);
    MortonYao48DeviceTiledPairFrontierContext context{
        std::move(traversal),
        MortonYao48DeviceTiledPairFrontierConfig{2U, 2'049U}};
    auto first_anchors = complete_prefix_by_prune(2'048U);
    first_anchors.push_back(prune_chunk());
    auto corrupt_anchors = completed_prefix_zero_delta(2'048U);
    corrupt_anchors.push_back(complete_anchor(1U, 0U, 0U, 1U));
    configure_transcripts({
        {std::move(first_anchors),
         FakeMortonYao48DeviceTiledPairFrontierCorruption::none},
        {std::move(corrupt_anchors), corruption}});
    auto first = context.advance();
    first.candidate_tile.reset();
    check_throws<std::runtime_error>(
        [&context] { (void)context.advance(); },
        "a stale or rolled-back continuation transcript is rejected");
    check(
        context.poisoned() && !context.ready(),
        "a rejected continuation transcript poisons the context");
  }
}

void test_multiple_tiles_keep_one_exact_arena_account() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  auto traversal = traversal_lease(6U);
  MortonYao48DeviceTiledPairFrontierContext context{
      std::move(traversal), MortonYao48DeviceTiledPairFrontierConfig{2U, 2U}};
  configure_transcripts({
      {{complete_anchor(1U, 0U, 0U, 1U),
        complete_anchor(2U, 0U, 0U, 2U)},
       FakeMortonYao48DeviceTiledPairFrontierCorruption::none},
      {{complete_anchor(3U, 0U, 0U, 3U),
        complete_anchor(4U, 0U, 0U, 4U)},
       FakeMortonYao48DeviceTiledPairFrontierCorruption::none},
      {{complete_anchor(5U, 0U, 0U, 5U)},
       FakeMortonYao48DeviceTiledPairFrontierCorruption::none}});

  auto first = context.advance();
  const std::uint64_t first_tile_epoch = first.audit.tile_epoch;
  check(
      first.status == MortonYao48DeviceTiledPairFrontierStatus::tile_complete &&
          first.candidate_tile.has_value() &&
          first.audit.transaction_physical_device_arena_capacity_bytes ==
              expected_arena_bytes(2U, 2U) &&
          first.candidate_tile->audit().physical_device_arena_capacity_bytes ==
              expected_arena_bytes(2U, 2U) &&
          first.candidate_tile->audit()
                  .physical_anchor_checkpoint_capacity == 2U &&
          first.candidate_tile->audit()
                  .physical_pending_anchor_count_capacity == 1U,
      "the first tile publishes the complete v4 arena account");
  first.candidate_tile.reset();

  auto second = context.advance();
  check(
      second.status == MortonYao48DeviceTiledPairFrontierStatus::tile_complete &&
          second.audit.tile_epoch != first_tile_epoch &&
          second.audit.chunk_sequence == 1U &&
          !second.audit.resumes_same_tile,
      "a new tile starts a fresh epoch and chunk sequence");
  second.candidate_tile.reset();
  auto third = context.advance();
  check(
      third.status ==
              MortonYao48DeviceTiledPairFrontierStatus::frontier_complete &&
          third.audit.cumulative_candidate_pair_mass == 15U &&
          third.audit.unresolved_pair_mass == 0U &&
          fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count() ==
              3U,
      "three physical tiles close one pair partition exactly once");
}

void test_detached_tile_survives_context_destruction() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  std::optional<MortonYao48DeviceCandidateTileLease> detached;
  const void* retained_identity = nullptr;
  std::size_t arena_bytes = 0U;
  {
    auto traversal = traversal_lease(4U);
    MortonYao48DeviceTiledPairFrontierContext context{
        std::move(traversal),
        MortonYao48DeviceTiledPairFrontierConfig{2U, 2U}};
    configure_transcripts({
        {{complete_anchor(1U, 0U, 0U, 1U),
          complete_anchor(2U, 0U, 0U, 2U)},
         FakeMortonYao48DeviceTiledPairFrontierCorruption::none}});
    auto advance = context.advance();
    check(
        advance.candidate_tile.has_value(),
        "the lifetime fixture publishes a detachable tile");
    if (!advance.candidate_tile.has_value()) {
      return;
    }
    detached.emplace(std::move(*advance.candidate_tile));
    const auto views =
        Phase15MortonYao48DeviceCandidateTilePrivateViewAccess::inspect(
            *detached);
    retained_identity = views.retained_authority_identity;
    arena_bytes = views.physical_device_arena_capacity_bytes;
  }
  if (!detached.has_value()) {
    return;
  }
  const auto views =
      Phase15MortonYao48DeviceCandidateTilePrivateViewAccess::inspect(
          *detached);
  check(
      detached->ready() && views.ready &&
          views.retained_authority_identity == retained_identity &&
          arena_bytes == expected_arena_bytes(2U, 2U) &&
          views.physical_device_arena_capacity_bytes == arena_bytes,
      "a detached v4 tile retains its authority and exact arena audit after context destruction");
}

void test_node_capacity_censure_is_terminal_and_idempotent() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  auto traversal = traversal_lease(3U);
  MortonYao48DeviceTiledPairFrontierContext context{
      std::move(traversal), MortonYao48DeviceTiledPairFrontierConfig{2U, 2U}};
  configure_transcripts({
      {{node_capacity_fatal(5U), node_capacity_fatal(5U)},
       FakeMortonYao48DeviceTiledPairFrontierCorruption::none}});
  auto censored = context.advance();
  const std::size_t launch_count =
      fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count();
  check(
      censored.status == MortonYao48DeviceTiledPairFrontierStatus::censored &&
          censored.stop_reason ==
              MortonYao48DeviceTiledPairFrontierStopReason::
                  node_visit_capacity &&
          context.terminally_censored() && !context.poisoned() &&
          !censored.candidate_tile.has_value() &&
          censored.audit.censored_anchor_outputs_withheld,
      "a coherent certified node-capacity stop is published as censure");
  auto stable = context.advance();
  check(
      stable.status == MortonYao48DeviceTiledPairFrontierStatus::censored &&
          fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count() ==
              launch_count,
      "a coherent node-capacity censure is idempotent");
}

void test_hostile_initial_envelopes_poison_fail_stop() {
  const std::vector<FakeMortonYao48DeviceTiledPairFrontierCorruption>
      corruptions{
          FakeMortonYao48DeviceTiledPairFrontierCorruption::stale_output_epoch,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              candidate_device_to_host,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              forged_cuda_execution,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              forged_device_arena_reuse,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              missing_output_owner,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::shared_output_owner,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              corrupt_metadata_digest,
          FakeMortonYao48DeviceTiledPairFrontierCorruption::
              corrupt_prune_semantics,
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
    auto traversal = traversal_lease(4U);
    MortonYao48DeviceTiledPairFrontierContext context{
        std::move(traversal),
        MortonYao48DeviceTiledPairFrontierConfig{2U, 2U}};
    configure_fake_gpu_morton_yao48_device_tiled_pair_frontier(
        FakeMortonYao48DeviceTiledPairFrontierConfiguration{
            {}, corruption, {}});
    check_throws<std::runtime_error>(
        [&context] { (void)context.advance(); },
        "a hostile initial v4 launcher envelope is rejected");
    check(
        context.poisoned() && !context.ready(),
        "a hostile initial v4 launcher envelope poisons the context");
  }
}

void test_strict_interior_threshold_is_authenticated_and_shell_open() {
  constexpr auto strict =
      MortonYao48DeviceTiledPairFrontierPruneSemantics::
          strict_interior_threshold;
  check(
      !phase15_morton_yao48_device_tiled_witness_lower_bound_certifies(
          0.0, strict) &&
          !phase15_morton_yao48_device_tiled_witness_lower_bound_certifies(
              -0.0, strict),
      "two shell witnesses never certify a strict-interior prune");
  check(
      phase15_morton_yao48_device_tiled_witness_lower_bound_certifies(
          0x1p-52, strict) &&
          phase15_morton_yao48_device_tiled_witness_lower_bound_certifies(
              0x1p-51, strict) &&
          morsehgp3d::gpu::
                  morton_yao48_device_tiled_pair_frontier_required_witness_count(
                      strict, 2U) ==
              2U,
      "two certified strict-interior witnesses can satisfy the q=3 prune "
      "threshold");

  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  auto traversal = traversal_lease(3U);
  const MortonYao48DeviceTiledPairFrontierConfig config{2U, 2U, strict};
  MortonYao48DeviceTiledPairFrontierContext context{
      std::move(traversal), config};
  configure_fake_gpu_morton_yao48_device_tiled_pair_frontier({});
  auto advance = context.advance();
  check(
      advance.status ==
              MortonYao48DeviceTiledPairFrontierStatus::frontier_complete &&
          advance.audit.prune_semantics == strict &&
          advance.audit.required_witness_count == 2U &&
          !advance.audit
               .nonnegative_diametral_witness_interval_lower_bound_required &&
          advance.audit
              .strictly_positive_diametral_witness_interval_lower_bound_required &&
          advance.audit
              .q3_exact_diametral_pair_support_gabriel_lane_partition_complete &&
          advance.audit.gamma2_silent_handoff_required &&
          !advance.audit.gamma2_prune_or_discard_authorized &&
          advance.candidate_tile.has_value(),
      "the strict-interior semantics and threshold are authenticated by the "
      "frontier audit");
  if (!advance.candidate_tile.has_value()) {
    return;
  }
  const auto& lease = *advance.candidate_tile;
  const auto views =
      Phase15MortonYao48DeviceCandidateTilePrivateViewAccess::inspect(lease);
  check(
      lease.ready() && lease.audit().prune_semantics == strict &&
          lease.audit().required_witness_count == 2U &&
          !lease.audit()
               .nonnegative_diametral_witness_interval_lower_bound_required &&
          lease.audit()
              .strictly_positive_diametral_witness_interval_lower_bound_required &&
          lease.audit()
              .q3_exact_diametral_pair_support_gabriel_negative_only &&
          lease.audit().gamma2_silent_handoff_required &&
          !lease.audit().gamma2_prune_or_discard_authorized &&
          views.prune_semantics == strict &&
          views.required_witness_count == 2U &&
          lease.audit().physical_device_arena_capacity_bytes ==
              expected_arena_bytes_for_witness_count(2U, 2U),
      "the strict-interior tag, threshold, and linear arena extent propagate "
      "through the detached lease");
}

void test_preflight_and_trivial_frontier() {
  reset_fake_gpu_morton_yao48_device_tiled_pair_frontier();
  auto traversal = traversal_lease(3U);
  check_throws<std::out_of_range>(
      [&traversal] {
        MortonYao48DeviceTiledPairFrontierContext invalid{
            std::move(traversal),
            MortonYao48DeviceTiledPairFrontierConfig{2U, 0U}};
      },
      "an invalid tile capacity is rejected before adoption");
  check(traversal.ready(), "preflight preserves the source lease");

  auto unknown_semantics_traversal = traversal_lease(3U);
  check_throws<std::out_of_range>(
      [&unknown_semantics_traversal] {
        MortonYao48DeviceTiledPairFrontierContext invalid{
            std::move(unknown_semantics_traversal),
            MortonYao48DeviceTiledPairFrontierConfig{
                2U,
                2U,
                static_cast<
                    MortonYao48DeviceTiledPairFrontierPruneSemantics>(255U)}};
      },
      "an unknown prune-semantics tag is rejected before adoption");
  check(
      unknown_semantics_traversal.ready(),
      "unknown-semantics preflight preserves the source lease");

  auto singleton_traversal = traversal_lease(1U);
  MortonYao48DeviceTiledPairFrontierContext singleton{
      std::move(singleton_traversal)};
  auto complete = singleton.advance();
  check(
      complete.status ==
              MortonYao48DeviceTiledPairFrontierStatus::frontier_complete &&
          complete.audit.unresolved_pair_mass == 0U &&
          !complete.candidate_tile.has_value(),
      "a singleton closes its empty pair universe without a launcher");
}

}  // namespace

int main() {
  test_exact_640_closes_without_empty_resume();
  test_candidate_640_plus_one_resumes_same_tile_with_backpressure();
  test_prune_2048_resumes_without_duplication();
  test_mixed_candidate_and_prune_yields_are_reported_honestly();
  test_chunk_subdivisions_are_not_a_global_call_ceiling();
  test_stale_and_rollback_transcripts_poison_fail_stop();
  test_multiple_tiles_keep_one_exact_arena_account();
  test_detached_tile_survives_context_destruction();
  test_node_capacity_censure_is_terminal_and_idempotent();
  test_hostile_initial_envelopes_poison_fail_stop();
  test_strict_interior_threshold_is_authenticated_and_shell_open();
  test_preflight_and_trivial_frontier();
  if (failures != 0) {
    std::cerr << failures
              << " device tiled Morton/Yao48 contract test(s) failed\n";
    return 1;
  }
  std::cout << "device tiled Morton/Yao48 contract tests passed\n";
  return 0;
}
