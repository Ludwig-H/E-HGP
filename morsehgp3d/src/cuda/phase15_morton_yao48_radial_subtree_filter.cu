#include "phase15_morton_yao48_radial_subtree_filter_internal.hpp"

#include <cuda_runtime.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

#if !defined(__CUDACC__)
#error "the Morton/Yao48 radial-subtree filter must be compiled by NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "the Morton/Yao48 radial-subtree filter requires CUDA 12.9"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "fast math is forbidden for the radial-subtree directed bounds"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "flush-to-zero is forbidden for the radial-subtree directed bounds"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "the radial-subtree filter must contain only sm_120 device code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

inline constexpr unsigned int kThreadsPerBlock = 256U;
inline constexpr std::uint64_t kExponentMask =
    UINT64_C(0x7ff0000000000000);
inline constexpr std::uint64_t kSignMask =
    UINT64_C(0x8000000000000000);
inline constexpr std::uint64_t kAll48BanksMask =
    (UINT64_C(1) << 48U) - UINT64_C(1);

class CudaFailure final : public std::runtime_error {
 public:
  CudaFailure(cudaError_t code, std::string operation)
      : std::runtime_error(
            std::move(operation) + ": " + cudaGetErrorString(code)) {}
};

void check_cuda(cudaError_t code, const char* operation) {
  if (code != cudaSuccess) {
    throw CudaFailure(code, operation);
  }
}

template <typename Value>
class DeviceBuffer final {
 public:
  DeviceBuffer() = default;
  DeviceBuffer(const DeviceBuffer&) = delete;
  DeviceBuffer& operator=(const DeviceBuffer&) = delete;
  ~DeviceBuffer() { reset(); }

  void allocate(std::size_t count, const char* operation) {
    if (count == 0U ||
        count > std::numeric_limits<std::size_t>::max() / sizeof(Value)) {
      throw std::length_error(
          "a radial-subtree device allocation extent is invalid");
    }
    check_cuda(
        cudaMalloc(reinterpret_cast<void**>(&pointer_), count * sizeof(Value)),
        operation);
    count_ = count;
  }

  void reset() noexcept {
    if (pointer_ != nullptr) {
      static_cast<void>(cudaFree(pointer_));
      pointer_ = nullptr;
      count_ = 0U;
    }
  }

  void abandon() noexcept {
    pointer_ = nullptr;
    count_ = 0U;
  }

  [[nodiscard]] Value* get() noexcept { return pointer_; }

 private:
  Value* pointer_{};
  std::size_t count_{};
};

class DeviceGuard final {
 public:
  explicit DeviceGuard(int target_device) {
    check_cuda(
        cudaGetDevice(&previous_device_),
        "cudaGetDevice before radial-subtree filtering");
    if (previous_device_ != target_device) {
      check_cuda(
          cudaSetDevice(target_device),
          "cudaSetDevice for radial-subtree filtering");
      restore_required_ = true;
    }
  }
  DeviceGuard(const DeviceGuard&) = delete;
  DeviceGuard& operator=(const DeviceGuard&) = delete;
  ~DeviceGuard() {
    if (restore_required_) {
      static_cast<void>(cudaSetDevice(previous_device_));
    }
  }
  void restore() {
    if (restore_required_) {
      check_cuda(
          cudaSetDevice(previous_device_),
          "cudaSetDevice restore after radial-subtree filtering");
      restore_required_ = false;
    }
  }

 private:
  int previous_device_{};
  bool restore_required_{};
};

class CudaResources final {
 public:
  explicit CudaResources(std::size_t maximum_query_count)
      : maximum_query_count_(maximum_query_count) {
    check_cuda(
        cudaGetDevice(&device_),
        "cudaGetDevice for radial-subtree context creation");
    cudaDeviceProp properties{};
    check_cuda(
        cudaGetDeviceProperties(&properties, device_),
        "cudaGetDeviceProperties for radial-subtree context");
    if (properties.maxGridSize[0] <= 0) {
      throw std::runtime_error(
          "the CUDA device exposes no radial-subtree one-dimensional grid");
    }
    maximum_grid_x_ =
        static_cast<unsigned int>(properties.maxGridSize[0]);
    check_cuda(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags for radial-subtree context");
    try {
      inputs_.allocate(
          maximum_query_count_,
          "cudaMalloc radial-subtree input capacity");
      records_.allocate(
          maximum_query_count_,
          "cudaMalloc radial-subtree record capacity");
    } catch (...) {
      static_cast<void>(cudaStreamDestroy(stream_));
      stream_ = nullptr;
      throw;
    }
  }

  CudaResources(const CudaResources&) = delete;
  CudaResources& operator=(const CudaResources&) = delete;
  ~CudaResources() {
    int previous_device = 0;
    const cudaError_t query_status = cudaGetDevice(&previous_device);
    bool restore = false;
    if (query_status == cudaSuccess && previous_device != device_) {
      restore = cudaSetDevice(device_) == cudaSuccess;
    }
    if (query_status != cudaSuccess ||
        (previous_device != device_ && !restore)) {
      inputs_.abandon();
      records_.abandon();
      stream_ = nullptr;
      return;
    }
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
    inputs_.reset();
    records_.reset();
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamDestroy(stream_));
      stream_ = nullptr;
    }
    if (restore) {
      static_cast<void>(cudaSetDevice(previous_device));
    }
  }

  [[nodiscard]] std::size_t maximum_query_count() const noexcept {
    return maximum_query_count_;
  }
  [[nodiscard]] int device() const noexcept { return device_; }
  [[nodiscard]] unsigned int maximum_grid_x() const noexcept {
    return maximum_grid_x_;
  }
  [[nodiscard]] cudaStream_t stream() const noexcept { return stream_; }
  [[nodiscard]] Phase15MortonYao48RadialDeviceInput* inputs() noexcept {
    return inputs_.get();
  }
  [[nodiscard]] Phase15MortonYao48RadialDeviceRecord* records() noexcept {
    return records_.get();
  }

 private:
  std::size_t maximum_query_count_{};
  int device_{-1};
  unsigned int maximum_grid_x_{};
  cudaStream_t stream_{};
  DeviceBuffer<Phase15MortonYao48RadialDeviceInput> inputs_;
  DeviceBuffer<Phase15MortonYao48RadialDeviceRecord> records_;
};

[[nodiscard]] CudaResources& resources(
    Phase15MortonYao48RadialSubtreeFilterState& state,
    std::size_t maximum_query_count) {
  std::shared_ptr<void>& opaque = state.cuda_resources();
  if (!opaque) {
    opaque = std::make_shared<CudaResources>(maximum_query_count);
  }
  auto& value = *static_cast<CudaResources*>(opaque.get());
  if (value.maximum_query_count() != maximum_query_count) {
    throw std::runtime_error(
        "the radial-subtree fixed CUDA capacity changed within a context");
  }
  return value;
}

[[nodiscard]] __device__ bool finite_word(std::uint64_t bits) noexcept {
  return (bits & kExponentMask) != kExponentMask;
}

[[nodiscard]] __device__ double value_from_bits(
    std::uint64_t bits) noexcept {
  return __longlong_as_double(static_cast<long long>(bits));
}

[[nodiscard]] __device__ bool directed_aabb_minimum_squared_distance(
    const Phase15MortonYao48RadialDeviceInput& input,
    double& output) noexcept {
  output = 0.0;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (!finite_word(input.anchor_bits[axis]) ||
        !finite_word(input.lower_bits[axis]) ||
        !finite_word(input.upper_bits[axis])) {
      return false;
    }
    const double anchor = value_from_bits(input.anchor_bits[axis]);
    const double lower = value_from_bits(input.lower_bits[axis]);
    const double upper = value_from_bits(input.upper_bits[axis]);
    if (lower > upper) {
      return false;
    }
    double delta = 0.0;
    if (anchor < lower) {
      delta = __dsub_rd(lower, anchor);
    } else if (anchor > upper) {
      delta = __dsub_rd(anchor, upper);
    }
    const double squared = __dmul_rd(delta, delta);
    const double next = __dadd_rd(output, squared);
    if (!isfinite(delta) || !isfinite(squared) || !isfinite(next) ||
        delta < 0.0 || squared < 0.0 || next < 0.0) {
      return false;
    }
    output = next;
  }
  return true;
}

[[nodiscard]] __device__ bool directed_three_radius_upper_bound(
    const Phase15MortonYao48RadialDeviceInput& input,
    double& output) noexcept {
  if (input.full_bank_mask != kAll48BanksMask ||
      input.device_eligible == 0U) {
    return false;
  }
  output = 0.0;
  for (std::size_t cone = 0U;
       cone < morton_yao48_radial_subtree_filter_cone_count;
       ++cone) {
    const std::uint64_t bits = input.bank_radius_upper_bits[cone];
    if (!finite_word(bits) || (bits & kSignMask) != 0U) {
      return false;
    }
    const double radius = value_from_bits(bits);
    if (!(radius > 0.0) || !isfinite(radius)) {
      return false;
    }
    const double three_radius = __dmul_ru(3.0, radius);
    if (!isfinite(three_radius) || three_radius < 0.0) {
      return false;
    }
    output = three_radius > output ? three_radius : output;
  }
  return true;
}

__global__ void radial_subtree_filter_kernel(
    const Phase15MortonYao48RadialDeviceInput* inputs,
    Phase15MortonYao48RadialDeviceRecord* records,
    std::size_t count) {
  const std::size_t index =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index >= count) {
    return;
  }
  const Phase15MortonYao48RadialDeviceInput& input = inputs[index];
  double minimum_squared_distance = 0.0;
  double three_radius_upper = 0.0;
  const bool valid = directed_aabb_minimum_squared_distance(
                         input, minimum_squared_distance) &&
      directed_three_radius_upper_bound(input, three_radius_upper);
  Phase15MortonYao48RadialDeviceRecord record{};
  record.replay_id = input.replay_id;
  record.directed_bounds_valid = static_cast<std::uint8_t>(valid ? 1U : 0U);
  if (valid && minimum_squared_distance >= three_radius_upper) {
    record.proposal = Phase15MortonYao48RadialProposal::prune;
  }
  records[index] = record;
}

}  // namespace

Phase15MortonYao48RadialDeviceBatch
propose_phase15_morton_yao48_radial_subtrees_on_gpu(
    Phase15MortonYao48RadialSubtreeFilterState& state,
    std::span<const Phase15MortonYao48RadialDeviceInput> inputs,
    std::size_t maximum_query_count) {
  if (inputs.empty() || inputs.size() > maximum_query_count) {
    throw std::invalid_argument(
        "the radial-subtree CUDA launch has invalid extents");
  }
  CudaResources& cuda = resources(state, maximum_query_count);
  const std::size_t block_count =
      (inputs.size() + kThreadsPerBlock - 1U) / kThreadsPerBlock;
  if (block_count == 0U || block_count > cuda.maximum_grid_x()) {
    throw std::length_error(
        "the radial-subtree CUDA launch exceeds the device grid");
  }

  DeviceGuard guard{cuda.device()};
  check_cuda(
      cudaMemcpyAsync(
          cuda.inputs(),
          inputs.data(),
          inputs.size_bytes(),
          cudaMemcpyHostToDevice,
          cuda.stream()),
      "cudaMemcpyAsync radial-subtree inputs host-to-device");
  radial_subtree_filter_kernel<<<
      static_cast<unsigned int>(block_count),
      kThreadsPerBlock,
      0U,
      cuda.stream()>>>(cuda.inputs(), cuda.records(), inputs.size());
  check_cuda(
      cudaGetLastError(),
      "Morton/Yao48 radial-subtree kernel launch");

  Phase15MortonYao48RadialDeviceBatch batch;
  batch.records.resize(inputs.size());
  check_cuda(
      cudaMemcpyAsync(
          batch.records.data(),
          cuda.records(),
          batch.records.size() * sizeof(batch.records.front()),
          cudaMemcpyDeviceToHost,
          cuda.stream()),
      "cudaMemcpyAsync radial-subtree records device-to-host");
  check_cuda(
      cudaStreamSynchronize(cuda.stream()),
      "cudaStreamSynchronize radial-subtree filter");
  batch.maximum_query_count = maximum_query_count;
  batch.launcher_call_count = 1U;
  batch.kernel_launch_count = 1U;
  batch.synchronization_count = 1U;
  batch.cuda_device = cuda.device();
  batch.execution_kind = Phase15MortonYao48RadialExecutionKind::cuda;
  batch.directed_round_down_aabb_implemented = true;
  batch.directed_round_up_three_radius_implemented = true;
  batch.equality_proposes_prune = true;
  guard.restore();
  return batch;
}

}  // namespace morsehgp3d::gpu::detail
