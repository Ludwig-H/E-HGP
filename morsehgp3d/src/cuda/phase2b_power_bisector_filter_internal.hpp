#pragma once

#include "morsehgp3d/gpu/predicate_filter.hpp"

#include <span>
#include <vector>

namespace morsehgp3d::gpu::detail {

[[nodiscard]] std::vector<FilterSign> filter_power_bisector_signs_on_gpu(
    std::span<const PowerBisectorFilterInput> inputs);

}  // namespace morsehgp3d::gpu::detail
