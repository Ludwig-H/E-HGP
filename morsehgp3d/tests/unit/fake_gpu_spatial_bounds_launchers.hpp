#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace morsehgp3d::gpu::test_support {

enum class FakeSpatialBoundsProposalPermutation : std::uint8_t {
  canonical,
  reversed,
};

enum class FakeSpatialBoundsProposalValues : std::uint8_t {
  actual_interval_recipe,
  all_unknown,
  all_visit,
  all_prune,
};

enum class FakeSpatialBoundsProposalCorruption : std::uint8_t {
  none,
  missing_record,
  duplicate_box_index,
  out_of_range_box_index,
  invalid_decision,
  false_prune,
  simulated_gpu_failure,
};

struct FakeSpatialBoundsProposalConfiguration {
  FakeSpatialBoundsProposalPermutation permutation{
      FakeSpatialBoundsProposalPermutation::canonical};
  FakeSpatialBoundsProposalValues values{
      FakeSpatialBoundsProposalValues::actual_interval_recipe};
  FakeSpatialBoundsProposalCorruption corruption{
      FakeSpatialBoundsProposalCorruption::none};
};

void configure_fake_gpu_spatial_bounds(
    FakeSpatialBoundsProposalConfiguration configuration) noexcept;
void reset_fake_gpu_spatial_bounds() noexcept;

[[nodiscard]] std::size_t fake_gpu_spatial_bounds_launch_count() noexcept;
[[nodiscard]] std::size_t fake_gpu_spatial_bounds_last_box_count() noexcept;
[[nodiscard]] std::array<std::uint64_t, 3>
fake_gpu_spatial_bounds_last_query_lower_bits() noexcept;
[[nodiscard]] std::array<std::uint64_t, 3>
fake_gpu_spatial_bounds_last_query_upper_bits() noexcept;
[[nodiscard]] std::array<std::uint64_t, 2>
fake_gpu_spatial_bounds_last_cutoff_bits() noexcept;

}  // namespace morsehgp3d::gpu::test_support
