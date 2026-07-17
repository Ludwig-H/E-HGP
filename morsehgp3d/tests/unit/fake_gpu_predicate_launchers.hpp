#pragma once

#include <cstdint>

namespace morsehgp3d::gpu::test_support {

inline constexpr std::uint64_t poison_replay_id =
    0xffffffffffffffffULL;
inline constexpr std::uint64_t invalid_filter_sign_replay_id =
    0xfffffffffffffffeULL;

void reset_fake_gpu_counters() noexcept;

[[nodiscard]] int fake_gpu_section_count() noexcept;
[[nodiscard]] int fake_gpu_maximum_concurrency() noexcept;

}  // namespace morsehgp3d::gpu::test_support
