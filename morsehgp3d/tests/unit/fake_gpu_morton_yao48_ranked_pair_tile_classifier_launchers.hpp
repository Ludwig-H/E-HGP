#pragma once

#include "morsehgp3d/gpu/morton_yao48_ranked_pair_tile_classifier.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace morsehgp3d::gpu::test_support {

enum class FakeMortonYao48RankedPairTileClassifierCorruption
    : std::uint8_t {
  none,
  stale_source_epoch,
  stale_candidate_epoch,
  stale_catalog_epoch,
  stale_tile_epoch,
  stale_chunk_sequence,
  cumulative_record_rollback,
  wrong_candidate_mass,
  corrupt_receipt_digest,
  candidate_device_to_host,
  certified_prune_device_to_host,
  callback_per_pair,
  dense_pair_scan,
  missing_source_authentication,
  missing_output_owner,
  malformed_output_layout,
  invalid_closed_rank,
  unsorted_record_order,
  invalid_rank_offsets,
  invalid_payload_offsets,
  invalid_payload_point_id,
  simulated_launcher_failure,
};

struct FakeMortonYao48RankedPairTileClassifierLaunch {
  std::uint64_t accepted_record_count{};
  std::uint64_t above_window_count{};
  std::uint64_t strict_payload_point_id_count{};
  std::uint64_t shell_payload_point_id_count{};
  std::uint64_t exact_fallback_count{};
  MortonYao48RankedPairTileClassifierStopReason capacity_stop_reason{
      MortonYao48RankedPairTileClassifierStopReason::none};
  FakeMortonYao48RankedPairTileClassifierCorruption corruption{
      FakeMortonYao48RankedPairTileClassifierCorruption::none};
};

struct FakeMortonYao48RankedPairTileClassifierConfiguration {
  std::vector<FakeMortonYao48RankedPairTileClassifierLaunch> launches;
};

void configure_fake_gpu_morton_yao48_ranked_pair_tile_classifier(
    FakeMortonYao48RankedPairTileClassifierConfiguration configuration);
void reset_fake_gpu_morton_yao48_ranked_pair_tile_classifier() noexcept;

[[nodiscard]] std::size_t
fake_gpu_morton_yao48_ranked_pair_tile_classifier_launch_count() noexcept;
[[nodiscard]] std::size_t
fake_gpu_morton_yao48_ranked_pair_tile_classifier_live_lease_count()
    noexcept;

}  // namespace morsehgp3d::gpu::test_support
