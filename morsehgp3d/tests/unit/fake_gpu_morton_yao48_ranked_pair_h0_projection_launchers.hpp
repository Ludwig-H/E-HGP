#pragma once

#include <cstddef>

namespace morsehgp3d::gpu::test_support {

void reset_fake_gpu_morton_yao48_ranked_pair_h0_projection() noexcept;

[[nodiscard]] std::size_t
fake_gpu_morton_yao48_ranked_pair_h0_projection_launch_count() noexcept;

}  // namespace morsehgp3d::gpu::test_support
