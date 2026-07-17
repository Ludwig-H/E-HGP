#include "phase2b_distance_filter_internal.hpp"

#include <cuda_runtime.h>

#include <cmath>
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
constexpr std::uint64_t kBinary64ExponentMask = UINT64_C(0x7ff0000000000000);

struct DeviceInterval {
  double lower{0.0};
  double upper{0.0};
  bool valid{false};
};

struct DeviceSquaredDistanceFilterInput {
  std::uint64_t replay_id;
  std::uint64_t witness_bits[3];
  std::uint64_t left_bits[3];
  std::uint64_t right_bits[3];
};

[[nodiscard]] __device__ bool device_is_finite(double value) noexcept {
  return isfinite(value) != 0;
}

[[nodiscard]] __device__ DeviceInterval invalid_interval() noexcept {
  return DeviceInterval{};
}

[[nodiscard]] __device__ DeviceInterval point_interval(
    std::uint64_t bits) noexcept {
  if ((bits & kBinary64ExponentMask) == kBinary64ExponentMask) {
    return invalid_interval();
  }
  const double value = __longlong_as_double(static_cast<long long int>(bits));
  return DeviceInterval{value, value, true};
}

[[nodiscard]] __device__ DeviceInterval checked_interval(
    double lower, double upper) noexcept {
  if (!device_is_finite(lower) || !device_is_finite(upper) || lower > upper) {
    return invalid_interval();
  }
  return DeviceInterval{lower, upper, true};
}

[[nodiscard]] __device__ DeviceInterval add_intervals(
    const DeviceInterval& left, const DeviceInterval& right) noexcept {
  if (!left.valid || !right.valid) {
    return invalid_interval();
  }
  return checked_interval(
      __dadd_rd(left.lower, right.lower),
      __dadd_ru(left.upper, right.upper));
}

[[nodiscard]] __device__ DeviceInterval subtract_intervals(
    const DeviceInterval& left, const DeviceInterval& right) noexcept {
  if (!left.valid || !right.valid) {
    return invalid_interval();
  }
  return checked_interval(
      __dsub_rd(left.lower, right.upper),
      __dsub_ru(left.upper, right.lower));
}

[[nodiscard]] __device__ DeviceInterval square_interval(
    const DeviceInterval& value) noexcept {
  if (!value.valid) {
    return invalid_interval();
  }

  double lower = 0.0;
  double upper = 0.0;
  if (value.lower > 0.0) {
    lower = __dmul_rd(value.lower, value.lower);
    upper = __dmul_ru(value.upper, value.upper);
  } else if (value.upper < 0.0) {
    lower = __dmul_rd(value.upper, value.upper);
    upper = __dmul_ru(value.lower, value.lower);
  } else {
    const double lower_square = __dmul_ru(value.lower, value.lower);
    const double upper_square = __dmul_ru(value.upper, value.upper);
    lower = 0.0;
    upper = lower_square > upper_square ? lower_square : upper_square;
  }
  return checked_interval(lower, upper);
}

[[nodiscard]] __device__ FilterSign filter_squared_distance(
    const DeviceSquaredDistanceFilterInput& input) noexcept {
  DeviceInterval left_squared = point_interval(0U);
  DeviceInterval right_squared = point_interval(0U);
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const DeviceInterval witness = point_interval(input.witness_bits[axis]);
    const DeviceInterval left_delta = subtract_intervals(
        witness, point_interval(input.left_bits[axis]));
    const DeviceInterval right_delta = subtract_intervals(
        witness, point_interval(input.right_bits[axis]));
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

__global__ void morsehgp3d_phase2b_squared_distance_filter_kernel(
    const DeviceSquaredDistanceFilterInput* inputs,
    RawSquaredDistanceFilterOutput* outputs,
    std::size_t count) {
  const std::size_t index =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index >= count) {
    return;
  }
  outputs[index].replay_id = inputs[index].replay_id;
  outputs[index].sign = filter_squared_distance(inputs[index]);
}

class CudaFailure final : public std::runtime_error {
 public:
  CudaFailure(cudaError_t code, std::string operation)
      : std::runtime_error(message(code, operation)),
        code_(code),
        operation_(std::move(operation)) {}

  [[nodiscard]] cudaError_t code() const noexcept { return code_; }
  [[nodiscard]] const std::string& operation() const noexcept {
    return operation_;
  }

 private:
  [[nodiscard]] static std::string message(
      cudaError_t code, const std::string& operation) {
    const char* description = cudaGetErrorString(code);
    return operation + " failed: " +
           (description == nullptr ? std::string{"unknown CUDA error"}
                                   : std::string{description});
  }

  cudaError_t code_;
  std::string operation_;
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

std::vector<RawSquaredDistanceFilterOutput> filter_squared_distances_on_gpu(
    std::span<const SquaredDistanceFilterInput> inputs) {
  if (inputs.empty()) {
    return {};
  }
  const std::size_t block_count =
      (inputs.size() + kThreadsPerBlock - 1U) / kThreadsPerBlock;
  if (block_count > std::numeric_limits<unsigned int>::max()) {
    throw std::length_error("Phase 2B distance batch exceeds the CUDA grid");
  }

  Stream stream;
  std::vector<DeviceSquaredDistanceFilterInput> packed_inputs(inputs.size());
  for (std::size_t index = 0U; index < inputs.size(); ++index) {
    packed_inputs[index].replay_id = inputs[index].replay_id;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      packed_inputs[index].witness_bits[axis] = inputs[index].witness_bits[axis];
      packed_inputs[index].left_bits[axis] = inputs[index].left_bits[axis];
      packed_inputs[index].right_bits[axis] = inputs[index].right_bits[axis];
    }
  }
  DeviceBuffer<DeviceSquaredDistanceFilterInput> device_inputs(inputs.size());
  DeviceBuffer<RawSquaredDistanceFilterOutput> device_outputs(inputs.size());
  std::vector<RawSquaredDistanceFilterOutput> outputs(inputs.size());

  check_cuda(
      cudaMemcpyAsync(
          device_inputs.get(),
          packed_inputs.data(),
          packed_inputs.size() * sizeof(DeviceSquaredDistanceFilterInput),
          cudaMemcpyHostToDevice,
          stream.get()),
      "cudaMemcpyAsync host-to-device");
  morsehgp3d_phase2b_squared_distance_filter_kernel
      <<<static_cast<unsigned int>(block_count), kThreadsPerBlock, 0U,
         stream.get()>>>(
          device_inputs.get(), device_outputs.get(), inputs.size());
  check_cuda(cudaGetLastError(), "Phase 2B distance filter launch");
  check_cuda(
      cudaMemcpyAsync(
          outputs.data(),
          device_outputs.get(),
          outputs.size() * sizeof(RawSquaredDistanceFilterOutput),
          cudaMemcpyDeviceToHost,
          stream.get()),
      "cudaMemcpyAsync device-to-host");
  check_cuda(cudaStreamSynchronize(stream.get()), "cudaStreamSynchronize");
  return outputs;
}

}  // namespace morsehgp3d::gpu::detail
