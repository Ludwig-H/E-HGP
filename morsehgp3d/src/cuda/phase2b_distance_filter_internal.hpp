#pragma once

#include "morsehgp3d/gpu/predicate_filter.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace morsehgp3d::gpu::detail {

struct RawSquaredDistanceFilterOutput {
  std::uint64_t replay_id{0};
  FilterSign sign{FilterSign::unknown};
};

[[nodiscard]] std::vector<RawSquaredDistanceFilterOutput>
filter_squared_distances_on_gpu(
    std::span<const SquaredDistanceFilterInput> inputs);

}  // namespace morsehgp3d::gpu::detail
