#pragma once

#include "morsehgp3d/gpu/predicate_filter.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace morsehgp3d::gpu::detail {

struct RawPowerBisectorFilterOutput {
  std::uint64_t replay_id{0};
  FilterSign sign{FilterSign::unknown};
};

[[nodiscard]] std::vector<RawPowerBisectorFilterOutput>
filter_power_bisectors_on_gpu(
    std::span<const PowerBisectorFilterInput> inputs);

}  // namespace morsehgp3d::gpu::detail
