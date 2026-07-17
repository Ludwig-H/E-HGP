#pragma once

#include "phase2b_predicate_context_internal.hpp"

#include "morsehgp3d/gpu/predicate_filter.hpp"

#include <span>
#include <vector>

namespace morsehgp3d::gpu::detail {

[[nodiscard]] std::vector<FilterSign> filter_squared_distance_signs_on_gpu(
    PredicateFilterContextState& context,
    std::span<const SquaredDistanceFilterInput> inputs);

}  // namespace morsehgp3d::gpu::detail
