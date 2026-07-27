#pragma once

#include "morsehgp3d/gpu/binary64_lbvh_top_k.hpp"

#include <cstddef>
#include <cstdint>

namespace morsehgp3d::gpu::detail {

[[nodiscard]] Binary64LbvhTopKResult
run_phase14_binary64_lbvh_top_k_on_gpu(
    const std::uint64_t* device_coordinate_bits,
    const std::uint64_t* device_morton_point_ids,
    const void* device_nodes,
    std::size_t point_count,
    std::size_t certified_node_count,
    std::size_t persistent_input_device_byte_capacity,
    std::uint64_t source_snapshot_epoch,
    std::size_t maximum_order,
    std::size_t seed_window_radius,
    int cuda_device);

}  // namespace morsehgp3d::gpu::detail
