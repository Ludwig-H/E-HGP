#pragma once

#include "morsehgp3d/gpu/higher_support_device_tiled_frontier.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cstddef>
#include <cstdint>

namespace morsehgp3d::gpu::test_support {

// Injectable device-side forgeries for the scientific Phase 15 higher-support
// device tiled host fake.  Each corruption perturbs exactly one seam
// invariant after the exact slot machine has run, so the host transactional
// validation must reject the batch and poison the context.  The corruption
// stays armed until it is reset or replaced.
enum class FakeHigherSupportDeviceTiledFrontierCorruption : std::uint8_t {
  none,
  stale_tile_epoch,
  stale_chunk_sequence,
  cumulative_rollback,
  broken_partition,
  chunk_ready_without_full_segment,
  fatal_with_partial_output,
  active_slot_status,
  mass_without_record,
  forged_root_digest,
  stack_high_water_overflow,
  nonzero_failure_code,
  forged_cuda_execution,
  metadata_digest_corruption,
};

// Binds the host geometry the scientific fake replays.  The fake rebuilds a
// self-contained read-only mirror of the Morton LBVH from the public leaf
// records with the exact radix/median split and witness rules of the CPU
// builder, and fails closed unless the mirror reproduces the certified node
// count and observed maximum depth.  Nothing references the index or the
// cloud after a successful bind; rebinding drops any retained tile
// continuation.
void bind_fake_higher_support_device_tiled_geometry(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud);

void set_fake_gpu_higher_support_device_tiled_frontier_corruption(
    FakeHigherSupportDeviceTiledFrontierCorruption corruption);

void reset_fake_gpu_higher_support_device_tiled_frontier() noexcept;

[[nodiscard]] std::size_t
fake_gpu_higher_support_device_tiled_frontier_launch_count() noexcept;

}  // namespace morsehgp3d::gpu::test_support
