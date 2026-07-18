#pragma once

#include <cstddef>
#include <cstdint>

namespace morsehgp3d::gpu::test_support {

enum class FakeK1BoruvkaCorruption : std::uint8_t {
  none,
  offset_count_mismatch,
  missing_outgoing_candidate,
  same_component_target,
  out_of_range_target,
  simulated_gpu_failure,
};

struct FakeK1BoruvkaConfiguration {
  FakeK1BoruvkaCorruption corruption{FakeK1BoruvkaCorruption::none};
};

void configure_fake_gpu_k1_boruvka(
    FakeK1BoruvkaConfiguration configuration) noexcept;
void reset_fake_gpu_k1_boruvka() noexcept;

[[nodiscard]] std::size_t fake_gpu_k1_boruvka_launch_count() noexcept;
[[nodiscard]] std::size_t fake_gpu_k1_boruvka_last_point_count() noexcept;
[[nodiscard]] std::size_t fake_gpu_k1_boruvka_last_node_count() noexcept;

}  // namespace morsehgp3d::gpu::test_support
