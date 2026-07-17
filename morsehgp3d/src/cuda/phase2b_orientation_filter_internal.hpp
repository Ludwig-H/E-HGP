#pragma once

#include "phase2b_predicate_context_internal.hpp"

#include "morsehgp3d/gpu/predicate_filter.hpp"

#include <span>
#include <vector>

namespace morsehgp3d::gpu::detail {

[[nodiscard]] std::vector<FilterSign> filter_orientation_3d_signs_on_gpu(
    PredicateFilterContextState& context,
    std::span<const Orientation3DFilterInput> inputs);

}  // namespace morsehgp3d::gpu::detail
