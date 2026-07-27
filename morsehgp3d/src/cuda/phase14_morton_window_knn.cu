#include "phase14_morton_window_knn_internal.hpp"

#include <cuda_runtime.h>

#include <algorithm>
#include <chrono>
#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

#if !defined(__CUDACC__)
#error "phase14_morton_window_knn.cu must be compiled ahead of time with NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Phase 14 Morton-window heuristic requires CUDA 12.9"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the Phase 14 Morton-window heuristic"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Phase 14 Morton-window heuristic must contain only sm_120 code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

constexpr unsigned int kThreadsPerBlock = 256U;
constexpr std::size_t kMaximumOrder = 10U;
constexpr std::size_t kAxisCount = 3U;
constexpr std::size_t kBlocksPerMultiprocessor = 32U;

class CudaFailure final : public std::runtime_error {
 public:
  CudaFailure(cudaError_t code, std::string operation)
      : std::runtime_error(
            std::move(operation) + " failed: " + cudaGetErrorString(code)) {}
};

void check_cuda(cudaError_t code, std::string operation) {
  if (code != cudaSuccess) {
    throw CudaFailure(code, std::move(operation));
  }
}

template <typename Value>
class DeviceBuffer final {
 public:
  DeviceBuffer() = default;

  explicit DeviceBuffer(std::size_t count) {
    if (count == 0U ||
        count > std::numeric_limits<std::size_t>::max() / sizeof(Value)) {
      throw std::length_error(
          "the Morton-window device allocation extent is invalid");
    }
    check_cuda(
        cudaMalloc(reinterpret_cast<void**>(&data_), count * sizeof(Value)),
        "cudaMalloc");
  }

  ~DeviceBuffer() {
    if (data_ != nullptr) {
      static_cast<void>(cudaFree(data_));
    }
  }

  DeviceBuffer(const DeviceBuffer&) = delete;
  DeviceBuffer& operator=(const DeviceBuffer&) = delete;
  DeviceBuffer(DeviceBuffer&&) = delete;
  DeviceBuffer& operator=(DeviceBuffer&&) = delete;

  [[nodiscard]] Value* get() noexcept { return data_; }
  [[nodiscard]] const Value* get() const noexcept { return data_; }

 private:
  Value* data_{};
};

class CudaEvent final {
 public:
  CudaEvent() { check_cuda(cudaEventCreate(&event_), "cudaEventCreate"); }

  ~CudaEvent() {
    if (event_ != nullptr) {
      static_cast<void>(cudaEventDestroy(event_));
    }
  }

  CudaEvent(const CudaEvent&) = delete;
  CudaEvent& operator=(const CudaEvent&) = delete;

  void record() { check_cuda(cudaEventRecord(event_), "cudaEventRecord"); }

  void synchronize() {
    check_cuda(cudaEventSynchronize(event_), "cudaEventSynchronize");
  }

  [[nodiscard]] cudaEvent_t get() const noexcept { return event_; }

 private:
  cudaEvent_t event_{};
};

[[nodiscard]] std::size_t checked_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::length_error(message);
  }
  return left * right;
}

[[nodiscard]] std::size_t checked_sum(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

template <typename Duration>
[[nodiscard]] std::uint64_t duration_nanoseconds(Duration duration) {
  const auto count =
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
  if (count < 0) {
    throw std::runtime_error("the steady CUDA allocation clock moved backwards");
  }
  return static_cast<std::uint64_t>(count);
}

[[nodiscard]] std::uint64_t event_nanoseconds(
    const CudaEvent& begin,
    const CudaEvent& end) {
  float milliseconds = 0.0F;
  check_cuda(
      cudaEventElapsedTime(&milliseconds, begin.get(), end.get()),
      "cudaEventElapsedTime");
  if (!(milliseconds >= 0.0F)) {
    throw std::runtime_error("CUDA returned a negative event duration");
  }
  return static_cast<std::uint64_t>(
      static_cast<double>(milliseconds) * 1'000'000.0);
}

__device__ bool nearer(
    double candidate_distance,
    std::uint64_t candidate_id,
    double incumbent_distance,
    std::uint64_t incumbent_id) {
  return candidate_distance < incumbent_distance ||
         (candidate_distance == incumbent_distance &&
          candidate_id < incumbent_id);
}

__global__ void morton_window_knn_kernel(
    const double* coordinates,
    const std::uint64_t* point_ids,
    std::size_t point_count,
    std::size_t maximum_order,
    std::size_t window_radius,
    std::uint64_t* output_ids,
    double* output_distances) {
  const std::size_t stride =
      static_cast<std::size_t>(blockDim.x) *
      static_cast<std::size_t>(gridDim.x);
  std::size_t position =
      static_cast<std::size_t>(blockIdx.x) *
          static_cast<std::size_t>(blockDim.x) +
      static_cast<std::size_t>(threadIdx.x);
  for (; position < point_count; position += stride) {
    double best_distances[kMaximumOrder];
    std::uint64_t best_ids[kMaximumOrder];
    for (std::size_t rank = 0U; rank < maximum_order; ++rank) {
      best_distances[rank] = DBL_MAX;
      best_ids[rank] = UINT64_MAX;
    }

    const std::size_t begin =
        position > window_radius ? position - window_radius : 0U;
    const std::size_t remaining = point_count - 1U - position;
    const std::size_t end =
        position + (window_radius < remaining ? window_radius : remaining) + 1U;
    const double x = coordinates[position * kAxisCount];
    const double y = coordinates[position * kAxisCount + 1U];
    const double z = coordinates[position * kAxisCount + 2U];
    for (std::size_t candidate = begin; candidate < end; ++candidate) {
      if (candidate == position) {
        continue;
      }
      const double dx = x - coordinates[candidate * kAxisCount];
      const double dy = y - coordinates[candidate * kAxisCount + 1U];
      const double dz = z - coordinates[candidate * kAxisCount + 2U];
      const double squared_distance =
          dx * dx + dy * dy + dz * dz;
      const std::uint64_t candidate_id = point_ids[candidate];
      if (!nearer(
              squared_distance,
              candidate_id,
              best_distances[maximum_order - 1U],
              best_ids[maximum_order - 1U])) {
        continue;
      }
      std::size_t insertion = maximum_order - 1U;
      while (insertion > 0U &&
             nearer(
                 squared_distance,
                 candidate_id,
                 best_distances[insertion - 1U],
                 best_ids[insertion - 1U])) {
        best_distances[insertion] = best_distances[insertion - 1U];
        best_ids[insertion] = best_ids[insertion - 1U];
        --insertion;
      }
      best_distances[insertion] = squared_distance;
      best_ids[insertion] = candidate_id;
    }

    const std::size_t output_begin = position * maximum_order;
    for (std::size_t rank = 0U; rank < maximum_order; ++rank) {
      output_ids[output_begin + rank] = best_ids[rank];
      output_distances[output_begin + rank] = best_distances[rank];
    }
  }
}

}  // namespace

Phase14MortonWindowKnnResult run_phase14_morton_window_knn_on_gpu(
    std::span<const double> coordinates_by_morton_position,
    std::span<const std::uint64_t> point_ids_by_morton_position,
    std::size_t maximum_order,
    std::size_t morton_window_radius) {
  const std::size_t point_count = point_ids_by_morton_position.size();
  if (point_count <= maximum_order || maximum_order == 0U ||
      maximum_order > kMaximumOrder ||
      morton_window_radius < maximum_order ||
      coordinates_by_morton_position.size() !=
          checked_product(
              point_count,
              kAxisCount,
              "the Morton-window coordinate extent overflows size_t")) {
    throw std::invalid_argument(
        "the Morton-window GPU heuristic received invalid extents");
  }
  const std::size_t neighbor_count = checked_product(
      point_count,
      maximum_order,
      "the Morton-window neighbor extent overflows size_t");
  const std::size_t coordinate_bytes = checked_product(
      coordinates_by_morton_position.size(),
      sizeof(double),
      "the Morton-window coordinate byte extent overflows size_t");
  const std::size_t point_id_bytes = checked_product(
      point_count,
      sizeof(std::uint64_t),
      "the Morton-window point-id byte extent overflows size_t");
  const std::size_t neighbor_id_bytes = checked_product(
      neighbor_count,
      sizeof(std::uint64_t),
      "the Morton-window neighbor-id byte extent overflows size_t");
  const std::size_t neighbor_distance_bytes = checked_product(
      neighbor_count,
      sizeof(double),
      "the Morton-window distance byte extent overflows size_t");

  Phase14MortonWindowKnnResult result;
  result.neighbor_point_ids.resize(neighbor_count);
  result.squared_distances.resize(neighbor_count);

  cudaDeviceProp properties{};
  int device = 0;
  check_cuda(cudaGetDevice(&device), "cudaGetDevice");
  check_cuda(cudaGetDeviceProperties(&properties, device),
             "cudaGetDeviceProperties");
  if (properties.multiProcessorCount <= 0) {
    throw std::runtime_error("CUDA reported no available multiprocessor");
  }
  result.cuda_device = device;
  result.multiprocessor_count = properties.multiProcessorCount;
  result.device_name = properties.name;

  using Clock = std::chrono::steady_clock;
  const auto allocation_begin = Clock::now();
  DeviceBuffer<double> device_coordinates(
      coordinates_by_morton_position.size());
  DeviceBuffer<std::uint64_t> device_point_ids(point_count);
  DeviceBuffer<std::uint64_t> device_neighbor_ids(neighbor_count);
  DeviceBuffer<double> device_neighbor_distances(neighbor_count);
  const auto allocation_end = Clock::now();
  result.allocation_nanoseconds =
      duration_nanoseconds(allocation_end - allocation_begin);
  result.device_byte_capacity = checked_sum(
      checked_sum(
          coordinate_bytes,
          point_id_bytes,
          "the Morton-window input device bytes overflow size_t"),
      checked_sum(
          neighbor_id_bytes,
          neighbor_distance_bytes,
          "the Morton-window output device bytes overflow size_t"),
      "the Morton-window total device bytes overflow size_t");

  CudaEvent transfer_begin;
  CudaEvent transfer_end;
  CudaEvent kernel_end;
  CudaEvent output_end;
  transfer_begin.record();
  check_cuda(
      cudaMemcpy(
          device_coordinates.get(),
          coordinates_by_morton_position.data(),
          coordinate_bytes,
          cudaMemcpyHostToDevice),
      "cudaMemcpy Morton coordinates H2D");
  check_cuda(
      cudaMemcpy(
          device_point_ids.get(),
          point_ids_by_morton_position.data(),
          point_id_bytes,
          cudaMemcpyHostToDevice),
      "cudaMemcpy Morton point ids H2D");
  transfer_end.record();

  const std::size_t desired_block_count =
      (point_count + static_cast<std::size_t>(kThreadsPerBlock) - 1U) /
      static_cast<std::size_t>(kThreadsPerBlock);
  const std::size_t occupancy_block_count = checked_product(
      static_cast<std::size_t>(properties.multiProcessorCount),
      kBlocksPerMultiprocessor,
      "the Morton-window occupancy block count overflows size_t");
  const std::size_t block_count =
      std::max(std::size_t{1},
               std::min(desired_block_count, occupancy_block_count));
  if (block_count >
      static_cast<std::size_t>(std::numeric_limits<unsigned int>::max())) {
    throw std::length_error(
        "the Morton-window CUDA grid exceeds unsigned int");
  }
  result.kernel_block_count = block_count;
  result.kernel_thread_count = checked_product(
      block_count,
      static_cast<std::size_t>(kThreadsPerBlock),
      "the Morton-window kernel thread count overflows size_t");
  morton_window_knn_kernel<<<
      static_cast<unsigned int>(block_count), kThreadsPerBlock>>>(
      device_coordinates.get(),
      device_point_ids.get(),
      point_count,
      maximum_order,
      morton_window_radius,
      device_neighbor_ids.get(),
      device_neighbor_distances.get());
  check_cuda(cudaGetLastError(), "morton_window_knn_kernel launch");
  kernel_end.record();

  check_cuda(
      cudaMemcpy(
          result.neighbor_point_ids.data(),
          device_neighbor_ids.get(),
          neighbor_id_bytes,
          cudaMemcpyDeviceToHost),
      "cudaMemcpy Morton neighbor ids D2H");
  check_cuda(
      cudaMemcpy(
          result.squared_distances.data(),
          device_neighbor_distances.get(),
          neighbor_distance_bytes,
          cudaMemcpyDeviceToHost),
      "cudaMemcpy Morton neighbor distances D2H");
  output_end.record();
  output_end.synchronize();

  result.host_to_device_nanoseconds =
      event_nanoseconds(transfer_begin, transfer_end);
  result.kernel_nanoseconds = event_nanoseconds(transfer_end, kernel_end);
  result.device_to_host_nanoseconds = event_nanoseconds(kernel_end, output_end);
  return result;
}

}  // namespace morsehgp3d::gpu::detail
