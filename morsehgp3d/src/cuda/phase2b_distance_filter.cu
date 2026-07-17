#include "phase2b_distance_filter_internal.hpp"
#include "phase2b_interval.cuh"

#include <cuda_runtime.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if !defined(__CUDACC__)
#error "phase2b_distance_filter.cu must be compiled ahead of time with NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Phase 2B distance filter requires the CUDA 12.9 compiler"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for certified Phase 2B filters"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for certified Phase 2B filters"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Phase 2B distance filter must contain only sm_120 device code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

constexpr unsigned int kThreadsPerBlock = 256U;
constexpr std::size_t kPointCount = 3U;
constexpr std::size_t kAxisCount = 3U;
constexpr std::size_t kCoordinateFieldCount = kPointCount * kAxisCount;

using device::DeviceInterval;
using device::add_intervals;
using device::point_interval;
using device::square_interval;
using device::subtract_intervals;

[[nodiscard]] __device__ DeviceInterval coordinate_interval(
    const std::uint64_t* coordinate_bits,
    std::size_t count,
    std::size_t index,
    std::size_t point,
    std::size_t axis) noexcept {
  const std::size_t field = point * kAxisCount + axis;
  return point_interval(coordinate_bits[field * count + index]);
}

[[nodiscard]] __device__ FilterSign filter_squared_distance(
    const std::uint64_t* coordinate_bits,
    std::size_t count,
    std::size_t index) noexcept {
  DeviceInterval left_squared = point_interval(0U);
  DeviceInterval right_squared = point_interval(0U);
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const DeviceInterval witness =
        coordinate_interval(coordinate_bits, count, index, 0U, axis);
    const DeviceInterval left_delta = subtract_intervals(
        witness,
        coordinate_interval(coordinate_bits, count, index, 1U, axis));
    const DeviceInterval right_delta = subtract_intervals(
        witness,
        coordinate_interval(coordinate_bits, count, index, 2U, axis));
    left_squared = add_intervals(left_squared, square_interval(left_delta));
    right_squared = add_intervals(right_squared, square_interval(right_delta));
  }
  const DeviceInterval difference =
      subtract_intervals(left_squared, right_squared);
  if (!difference.valid) {
    return FilterSign::unknown;
  }
  if (difference.lower > 0.0) {
    return FilterSign::positive;
  }
  if (difference.upper < 0.0) {
    return FilterSign::negative;
  }
  return FilterSign::unknown;
}

// Coordinates are stored field-major from witness.x through right.z, so a
// warp performs one coalesced load per field instead of striding over records.
__global__ void morsehgp3d_phase2b_squared_distance_filter_kernel(
    const std::uint64_t* coordinate_bits,
    FilterSign* outputs,
    std::size_t count) {
  const std::size_t index =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index >= count) {
    return;
  }
  outputs[index] = filter_squared_distance(coordinate_bits, count, index);
}

class CudaFailure final : public std::runtime_error {
 public:
  CudaFailure(cudaError_t code, std::string operation)
      : std::runtime_error(message(code, operation)) {}

 private:
  [[nodiscard]] static std::string message(
      cudaError_t code, const std::string& operation) {
    const char* description = cudaGetErrorString(code);
    return operation + " failed: " +
           (description == nullptr ? std::string{"unknown CUDA error"}
                                   : std::string{description});
  }
};

void check_cuda(cudaError_t code, std::string operation) {
  if (code != cudaSuccess) {
    throw CudaFailure(code, std::move(operation));
  }
}

class Stream final {
 public:
  Stream() {
    check_cuda(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags");
  }

  Stream(const Stream&) = delete;
  Stream& operator=(const Stream&) = delete;

  ~Stream() {
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamDestroy(stream_));
    }
  }

  [[nodiscard]] cudaStream_t get() const noexcept { return stream_; }

 private:
  cudaStream_t stream_{nullptr};
};

template <typename Value>
class DeviceBuffer final {
 public:
  explicit DeviceBuffer(std::size_t count) {
    if (count > std::numeric_limits<std::size_t>::max() / sizeof(Value)) {
      throw std::length_error("Phase 2B device allocation size overflow");
    }
    check_cuda(
        cudaMalloc(reinterpret_cast<void**>(&data_), count * sizeof(Value)),
        "cudaMalloc");
  }

  DeviceBuffer(const DeviceBuffer&) = delete;
  DeviceBuffer& operator=(const DeviceBuffer&) = delete;

  ~DeviceBuffer() {
    if (data_ != nullptr) {
      static_cast<void>(cudaFree(data_));
    }
  }

  [[nodiscard]] Value* get() noexcept { return data_; }

 private:
  Value* data_{nullptr};
};

}  // namespace

std::vector<FilterSign> filter_squared_distance_signs_on_gpu(
    std::span<const SquaredDistanceFilterInput> inputs) {
  if (inputs.empty()) {
    return {};
  }
  if (inputs.size() >
      std::numeric_limits<std::size_t>::max() / kCoordinateFieldCount) {
    throw std::length_error("Phase 2B distance input packing overflow");
  }
  const std::size_t block_count =
      (inputs.size() - 1U) / kThreadsPerBlock + 1U;
  if (block_count > std::numeric_limits<unsigned int>::max()) {
    throw std::length_error("Phase 2B distance batch exceeds the CUDA grid");
  }

  std::vector<std::uint64_t> coordinate_bits(
      inputs.size() * kCoordinateFieldCount);
  for (std::size_t index = 0U; index < inputs.size(); ++index) {
    for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
      coordinate_bits[(0U * kAxisCount + axis) * inputs.size() + index] =
          inputs[index].witness_bits[axis];
      coordinate_bits[(1U * kAxisCount + axis) * inputs.size() + index] =
          inputs[index].left_bits[axis];
      coordinate_bits[(2U * kAxisCount + axis) * inputs.size() + index] =
          inputs[index].right_bits[axis];
    }
  }

  Stream stream;
  DeviceBuffer<std::uint64_t> device_coordinate_bits(coordinate_bits.size());
  DeviceBuffer<FilterSign> device_outputs(inputs.size());
  std::vector<FilterSign> outputs(inputs.size());

  check_cuda(
      cudaMemcpyAsync(
          device_coordinate_bits.get(),
          coordinate_bits.data(),
          coordinate_bits.size() * sizeof(std::uint64_t),
          cudaMemcpyHostToDevice,
          stream.get()),
      "cudaMemcpyAsync distance coordinates host-to-device");
  morsehgp3d_phase2b_squared_distance_filter_kernel
      <<<static_cast<unsigned int>(block_count), kThreadsPerBlock, 0U,
         stream.get()>>>(
          device_coordinate_bits.get(),
          device_outputs.get(),
          inputs.size());
  check_cuda(cudaGetLastError(), "Phase 2B distance filter launch");
  check_cuda(
      cudaMemcpyAsync(
          outputs.data(),
          device_outputs.get(),
          outputs.size() * sizeof(FilterSign),
          cudaMemcpyDeviceToHost,
          stream.get()),
      "cudaMemcpyAsync device-to-host");
  check_cuda(cudaStreamSynchronize(stream.get()), "cudaStreamSynchronize");
  return outputs;
}

}  // namespace morsehgp3d::gpu::detail
