#pragma once

#include "morsehgp3d/gpu/predicate_filter.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace morsehgp3d::gpu::detail {

struct RawOrientation3DFilterOutput {
  std::uint64_t replay_id{0};
  FilterSign sign{FilterSign::unknown};
};

[[nodiscard]] std::vector<RawOrientation3DFilterOutput>
filter_orientations_3d_on_gpu(
    std::span<const Orientation3DFilterInput> inputs);

}  // namespace morsehgp3d::gpu::detail
