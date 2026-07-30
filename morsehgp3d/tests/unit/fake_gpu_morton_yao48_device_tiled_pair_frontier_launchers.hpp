#pragma once

#include "morsehgp3d/gpu/morton_yao48_device_tiled_pair_frontier.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace morsehgp3d::gpu::test_support {

enum class FakeMortonYao48DeviceTiledPairFrontierCorruption : std::uint8_t {
  none,
  stale_output_epoch,
  candidate_device_to_host,
  forged_cuda_execution,
  missing_output_owner,
  shared_output_owner,
  corrupt_metadata_digest,
  nonzero_failure_code,
  wrong_anchor_position,
  foreign_source_cloud_identity,
  pruned_mass_without_region,
  too_many_regions_for_pruned_mass,
  outputs_exceed_node_visits,
  stale_tile_epoch,
  stale_chunk_sequence,
  cumulative_candidate_rollback,
  cumulative_prune_rollback,
  process_restart_claimed,
  simulated_launcher_failure,
};

enum class FakeMortonYao48DeviceTiledAnchorStatus : std::uint8_t {
  active,
  chunk_ready,
  complete,
  fatal,
};

struct FakeMortonYao48DeviceTiledAnchor {
  std::uint64_t candidate_count{};
  std::uint64_t prune_region_count{};
  std::uint64_t certified_pruned_pair_mass{};
  std::uint64_t node_visit_count{};
  std::uint64_t ambiguous_cone_candidate_count{};
  std::uint64_t unbanked_candidate_count{};
  FakeMortonYao48DeviceTiledAnchorStatus status{
      FakeMortonYao48DeviceTiledAnchorStatus::complete};
  MortonYao48DeviceTiledPairFrontierYieldReason yield_reason{
      MortonYao48DeviceTiledPairFrontierYieldReason::none};
  MortonYao48DeviceTiledPairFrontierStopReason stop_reason{
      MortonYao48DeviceTiledPairFrontierStopReason::none};
};

struct FakeMortonYao48DeviceTiledPairFrontierLaunch {
  std::vector<FakeMortonYao48DeviceTiledAnchor> anchors;
  FakeMortonYao48DeviceTiledPairFrontierCorruption corruption{
      FakeMortonYao48DeviceTiledPairFrontierCorruption::none};
};

struct FakeMortonYao48DeviceTiledPairFrontierConfiguration {
  // Backward-compatible single-launch shorthand.
  std::vector<FakeMortonYao48DeviceTiledAnchor> anchors;
  FakeMortonYao48DeviceTiledPairFrontierCorruption corruption{
      FakeMortonYao48DeviceTiledPairFrontierCorruption::none};
  // When non-empty, one transcript is consumed per launcher call.
  std::vector<FakeMortonYao48DeviceTiledPairFrontierLaunch> launches;
};

void configure_fake_gpu_morton_yao48_device_tiled_pair_frontier(
    FakeMortonYao48DeviceTiledPairFrontierConfiguration configuration);
void reset_fake_gpu_morton_yao48_device_tiled_pair_frontier() noexcept;

[[nodiscard]] std::size_t
fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count() noexcept;

}  // namespace morsehgp3d::gpu::test_support
