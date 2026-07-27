#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace morsehgp3d::gpu::detail {

struct Phase14MortonWindowKnnResult {
  // Row-major by Morton position, then increasing approximate distance.
  std::vector<std::uint64_t> neighbor_point_ids;
  std::vector<double> squared_distances;
  std::uint64_t allocation_nanoseconds{};
  std::uint64_t host_to_device_nanoseconds{};
  std::uint64_t kernel_nanoseconds{};
  std::uint64_t device_to_host_nanoseconds{};
  std::size_t device_byte_capacity{};
  std::size_t kernel_block_count{};
  std::size_t kernel_thread_count{};
  int cuda_device{};
  int multiprocessor_count{};
  std::string device_name;
};

// coordinates_by_morton_position is point-major xyz.  The result contains
// exactly point_count * maximum_order records and never constructs a global
// pair matrix.  The Morton window is a heuristic candidate restriction, not
// a completeness certificate.
[[nodiscard]] Phase14MortonWindowKnnResult
run_phase14_morton_window_knn_on_gpu(
    std::span<const double> coordinates_by_morton_position,
    std::span<const std::uint64_t> point_ids_by_morton_position,
    std::size_t maximum_order,
    std::size_t morton_window_radius);

}  // namespace morsehgp3d::gpu::detail
