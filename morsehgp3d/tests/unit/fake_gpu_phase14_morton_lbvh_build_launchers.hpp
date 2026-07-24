#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace morsehgp3d::gpu::test_support {

enum class FakePhase14MortonLbvhBuildCorruption : std::uint8_t {
  none,
  wrong_proposal_extent,
  reserved_proposal_bits,
  wrong_unambiguous_bin,
  stale_proposal_epoch,
  wrong_snapshot_extent,
  wrong_snapshot_transfer_extent,
  wrong_snapshot_submission_count,
  wrong_postorder_child,
  wrong_aabb_witness,
  stale_snapshot_epoch,
  simulated_async_failure,
};

struct FakePhase14MortonLbvhBuildConfiguration {
  FakePhase14MortonLbvhBuildCorruption corruption{
      FakePhase14MortonLbvhBuildCorruption::none};
  // Axis-major index in [0, 3n).  The default requests no ambiguity.
  std::size_t forced_ambiguous_axis_index{
      std::numeric_limits<std::size_t>::max()};
};

void configure_fake_gpu_phase14_morton_lbvh_build(
    FakePhase14MortonLbvhBuildConfiguration configuration) noexcept;
void reset_fake_gpu_phase14_morton_lbvh_build() noexcept;

[[nodiscard]] std::size_t
fake_gpu_phase14_morton_bin_proposal_count() noexcept;
[[nodiscard]] std::size_t
fake_gpu_phase14_morton_lbvh_snapshot_count() noexcept;
[[nodiscard]] std::size_t
fake_gpu_phase14_morton_lbvh_last_point_count() noexcept;
[[nodiscard]] std::size_t
fake_gpu_phase14_morton_lbvh_last_point_capacity() noexcept;

}  // namespace morsehgp3d::gpu::test_support
