#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace morsehgp3d::gpu::test_support {

enum class FakeJungChordCsrTileCorruption : std::uint8_t {
  none,
  execution_failure,
  count_emit_mismatch,
};

struct FakeJungChordCsrTileConfiguration final {
  std::vector<std::uint64_t> chord_counts;
  std::vector<std::uint64_t> range_candidate_counts;
  std::vector<std::uint64_t> constant_interior_counts;
  std::vector<std::uint64_t> support_eligible_counts;
  FakeJungChordCsrTileCorruption corruption{
      FakeJungChordCsrTileCorruption::none};
};

void configure_fake_gpu_jung_chord_csr_tile(
    FakeJungChordCsrTileConfiguration configuration);
void reset_fake_gpu_jung_chord_csr_tile() noexcept;
[[nodiscard]] std::size_t fake_gpu_jung_chord_csr_tile_launch_count() noexcept;

}  // namespace morsehgp3d::gpu::test_support
