#pragma once

#include "morsehgp3d/gpu/jung_center_cover_prune_mass.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace morsehgp3d::gpu::test_support {

enum class FakeJungCenterCoverCorruption : std::uint8_t {
  none = 0U,
  mass_mismatch = 1U,
  stack_capacity_exhausted = 2U,
  execution_failure = 3U,
  missing_qualification_receipt = 4U,
};

struct FakeJungCenterCoverConfiguration final {
  std::uint64_t pruned_pair_mass{};
  std::vector<JungCenterCoverQualificationReceipt> receipts;
  FakeJungCenterCoverCorruption corruption{
      FakeJungCenterCoverCorruption::none};
};

void configure_fake_gpu_jung_center_cover_prune_mass(
    FakeJungCenterCoverConfiguration configuration);
void reset_fake_gpu_jung_center_cover_prune_mass() noexcept;
[[nodiscard]] std::size_t
fake_gpu_jung_center_cover_prune_mass_launch_count() noexcept;

}  // namespace morsehgp3d::gpu::test_support
