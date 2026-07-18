#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace morsehgp3d::gpu::test_support {

enum class FakeSpatialProposalPermutation : std::uint8_t {
  canonical,
  reversed,
};

enum class FakeSpatialProposalValues : std::uint8_t {
  actual_binary64_recipe,
  all_zero,
  ascending_by_point_id,
  descending_by_point_id,
  positive_infinity,
};

enum class FakeSpatialProposalCorruption : std::uint8_t {
  none,
  missing_record,
  duplicate_point_id,
  out_of_range_point_id,
  nan_distance,
  negative_distance,
  simulated_gpu_failure,
};

struct FakeSpatialProposalConfiguration {
  FakeSpatialProposalPermutation permutation{
      FakeSpatialProposalPermutation::canonical};
  FakeSpatialProposalValues values{
      FakeSpatialProposalValues::actual_binary64_recipe};
  FakeSpatialProposalCorruption corruption{
      FakeSpatialProposalCorruption::none};
};

void configure_fake_gpu_spatial_reference(
    FakeSpatialProposalConfiguration configuration) noexcept;
void reset_fake_gpu_spatial_reference() noexcept;

[[nodiscard]] std::size_t fake_gpu_spatial_launch_count() noexcept;
[[nodiscard]] std::size_t fake_gpu_spatial_last_point_count() noexcept;
[[nodiscard]] std::array<std::uint64_t, 3>
fake_gpu_spatial_last_query_bits() noexcept;

}  // namespace morsehgp3d::gpu::test_support
