#pragma once

// Archived point-MST surrogate v6; excluded from the active product build.

#include "morsehgp3d/gpu/binary64_lbvh_top_k.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

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

[[nodiscard]] Binary64LbvhCsrNeighborRankResult
run_phase15_binary64_lbvh_csr_neighbor_rank_on_gpu(
    const std::uint64_t* device_coordinate_bits,
    const std::uint64_t* device_morton_point_ids,
    const void* device_nodes,
    std::size_t point_count,
    std::size_t certified_node_count,
    std::size_t persistent_input_device_byte_capacity,
    std::uint64_t source_snapshot_epoch,
    std::span<const std::uint64_t> csr_offsets,
    std::span<const spatial::PointId> csr_neighbors,
    std::span<const spatial::PointId> source_point_id_by_canonical_id,
    std::size_t maximum_rank,
    std::size_t seed_window_radius,
    std::size_t query_batch_size,
    int cuda_device);

}  // namespace morsehgp3d::gpu::detail
