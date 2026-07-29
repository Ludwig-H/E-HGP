#include "phase15_exact_diametral_phi_internal.hpp"

#include "phase15_exact_diametral_phi_fixed.cuh"
#include "phase2b_interval.cuh"

#include <cuda_runtime.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if !defined(__CUDACC__)
#error "phase15_exact_diametral_phi.cu must be compiled AOT with NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The exact diametral-Phi kernel requires the CUDA 12.9 compiler"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the exact diametral-Phi kernel"
#endif

// Defense in depth only. The auditable authority is the CMake target's
// mandatory `--ftz=false` option; this compiler macro is not relied upon.
#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the exact diametral-Phi interval filter"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The exact diametral-Phi target must contain only sm_120 device code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

inline constexpr unsigned int threads_per_block = 256U;
inline constexpr std::size_t point_count = 3U;
inline constexpr std::size_t axis_count = 3U;
inline constexpr std::size_t coordinate_field_count = point_count * axis_count;

class CudaFailure final : public std::runtime_error {
 public:
  CudaFailure(cudaError_t code, std::string operation)
      : std::runtime_error(message(code, operation)) {}

 private:
  [[nodiscard]] static std::string message(
      cudaError_t code,
      const std::string& operation) {
    const char* name = cudaGetErrorName(code);
    const char* description = cudaGetErrorString(code);
    return operation + ": " +
           (name == nullptr ? std::string{"unknown CUDA error"}
                            : std::string{name}) +
           " (" +
           (description == nullptr ? std::string{"no description"}
                                   : std::string{description}) +
           ")";
  }
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

  ~DeviceBuffer() {
    reset();
  }

  void allocate(std::size_t count, const char* operation) {
    if (count == 0U) {
      return;
    }
    if (count > std::numeric_limits<std::size_t>::max() / sizeof(Value)) {
      throw std::length_error("exact diametral-Phi device allocation overflow");
    }
    void* allocation = nullptr;
    check_cuda(cudaMalloc(&allocation, count * sizeof(Value)), operation);
    pointer_ = static_cast<Value*>(allocation);
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
  [[nodiscard]] const Value* get() const noexcept { return pointer_; }
  [[nodiscard]] std::size_t count() const noexcept { return count_; }

 private:
  Value* pointer_{};
  std::size_t count_{};
};

class DeviceGuard final {
 public:
  explicit DeviceGuard(int target_device) {
    check_cuda(
        cudaGetDevice(&previous_device_),
        "cudaGetDevice before exact diametral-Phi launch");
    if (previous_device_ != target_device) {
      check_cuda(
          cudaSetDevice(target_device),
          "cudaSetDevice for exact diametral-Phi launch");
      restore_required_ = true;
    }
  }

  DeviceGuard(const DeviceGuard&) = delete;
  DeviceGuard& operator=(const DeviceGuard&) = delete;
  ~DeviceGuard() { restore_noexcept(); }

  void restore() {
    if (restore_required_) {
      check_cuda(
          cudaSetDevice(previous_device_),
          "cudaSetDevice restore after exact diametral-Phi launch");
      restore_required_ = false;
    }
  }

 private:
  void restore_noexcept() noexcept {
    if (restore_required_) {
      static_cast<void>(cudaSetDevice(previous_device_));
      restore_required_ = false;
    }
  }

  int previous_device_{};
  bool restore_required_{false};
};

class CudaResources final {
 public:
  explicit CudaResources(std::size_t maximum_query_count)
      : maximum_query_count_(maximum_query_count) {
    if (maximum_query_count_ >
        std::numeric_limits<std::size_t>::max() / coordinate_field_count) {
      throw std::length_error(
          "exact diametral-Phi coordinate capacity overflows size_t");
    }
    check_cuda(cudaGetDevice(&device_), "cudaGetDevice exact diametral-Phi");
    cudaDeviceProp properties{};
    check_cuda(
        cudaGetDeviceProperties(&properties, device_),
        "cudaGetDeviceProperties exact diametral-Phi");
    if (properties.maxGridSize[0] <= 0) {
      throw std::runtime_error(
          "the exact diametral-Phi device reports no one-dimensional grid");
    }
    maximum_grid_x_ = static_cast<unsigned int>(properties.maxGridSize[0]);
    check_cuda(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags exact diametral-Phi");
    try {
      coordinate_bits_.allocate(
          maximum_query_count_ * coordinate_field_count,
          "cudaMalloc exact diametral-Phi coordinates");
      records_.allocate(
          maximum_query_count_,
          "cudaMalloc exact diametral-Phi records");
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
    bool restore_device = false;
    if (query_status == cudaSuccess && previous_device != device_) {
      restore_device = cudaSetDevice(device_) == cudaSuccess;
    }
    if (query_status != cudaSuccess ||
        (previous_device != device_ && !restore_device)) {
      coordinate_bits_.abandon();
      records_.abandon();
      stream_ = nullptr;
      return;
    }
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
    coordinate_bits_.reset();
    records_.reset();
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamDestroy(stream_));
      stream_ = nullptr;
    }
    if (restore_device) {
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
  [[nodiscard]] std::uint64_t* coordinate_bits() noexcept {
    return coordinate_bits_.get();
  }
  [[nodiscard]] ExactDiametralPhiDeviceRecord* records() noexcept {
    return records_.get();
  }

 private:
  std::size_t maximum_query_count_{};
  int device_{-1};
  unsigned int maximum_grid_x_{};
  cudaStream_t stream_{};
  DeviceBuffer<std::uint64_t> coordinate_bits_;
  DeviceBuffer<ExactDiametralPhiDeviceRecord> records_;
};

[[nodiscard]] CudaResources& resources(
    ExactDiametralPhiContextState& state,
    std::size_t maximum_query_count) {
  std::shared_ptr<void>& opaque = state.cuda_resources();
  if (!opaque) {
    opaque = std::make_shared<CudaResources>(maximum_query_count);
  }
  CudaResources& value = *static_cast<CudaResources*>(opaque.get());
  if (value.maximum_query_count() != maximum_query_count) {
    throw std::runtime_error(
        "the exact diametral-Phi CUDA capacity changed within a context");
  }
  return value;
}

[[nodiscard]] inline __device__ std::uint64_t coordinate_word(
    const std::uint64_t* coordinate_bits,
    std::size_t query_count,
    std::size_t query_index,
    std::size_t point,
    std::size_t axis) noexcept {
  return coordinate_bits[(point * axis_count + axis) * query_count +
                         query_index];
}

[[nodiscard]] inline __device__ bool interval_sign(
    const std::uint64_t witness_bits[3],
    const std::uint64_t first_support_bits[3],
    const std::uint64_t second_support_bits[3],
    ExactDiametralPhiDeviceSign& sign) noexcept {
  device::DeviceInterval sum = device::point_interval(UINT64_C(0));
  for (std::size_t axis = 0U; axis < axis_count; ++axis) {
    const device::DeviceInterval witness =
        device::point_interval(witness_bits[axis]);
    const device::DeviceInterval first =
        device::point_interval(first_support_bits[axis]);
    const device::DeviceInterval second =
        device::point_interval(second_support_bits[axis]);
    const device::DeviceInterval term = device::multiply_intervals(
        device::subtract_intervals(witness, first),
        device::subtract_intervals(witness, second));
    sum = device::add_intervals(sum, term);
    if (!sum.valid) {
      return false;
    }
  }
  if (sum.lower > 0.0) {
    sign = ExactDiametralPhiDeviceSign::positive;
    return true;
  }
  if (sum.upper < 0.0) {
    sign = ExactDiametralPhiDeviceSign::negative;
    return true;
  }
  return false;
}

[[nodiscard]] inline __device__ ExactDiametralPhiDeviceSign fixed_sign(
    exact_phi_fixed::ResultCode code) noexcept {
  switch (code) {
    case exact_phi_fixed::ResultCode::negative:
      return ExactDiametralPhiDeviceSign::negative;
    case exact_phi_fixed::ResultCode::zero:
      return ExactDiametralPhiDeviceSign::zero;
    case exact_phi_fixed::ResultCode::positive:
      return ExactDiametralPhiDeviceSign::positive;
    case exact_phi_fixed::ResultCode::requires_cpu_fallback:
      return ExactDiametralPhiDeviceSign::unresolved;
    case exact_phi_fixed::ResultCode::invalid_input:
      return ExactDiametralPhiDeviceSign::invalid;
  }
  return ExactDiametralPhiDeviceSign::invalid;
}

__global__ void morsehgp3d_phase15_exact_diametral_phi_kernel(
    const std::uint64_t* coordinate_bits,
    ExactDiametralPhiDeviceRecord* records,
    std::size_t query_count) {
  const std::size_t query_index =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (query_index >= query_count) {
    return;
  }

  std::uint64_t witness_bits[3]{};
  std::uint64_t first_support_bits[3]{};
  std::uint64_t second_support_bits[3]{};
  for (std::size_t axis = 0U; axis < axis_count; ++axis) {
    witness_bits[axis] =
        coordinate_word(coordinate_bits, query_count, query_index, 0U, axis);
    first_support_bits[axis] =
        coordinate_word(coordinate_bits, query_count, query_index, 1U, axis);
    second_support_bits[axis] =
        coordinate_word(coordinate_bits, query_count, query_index, 2U, axis);
  }

  ExactDiametralPhiDeviceRecord record{};
  record.query_index = static_cast<std::uint64_t>(query_index);
  ExactDiametralPhiDeviceSign filtered_sign =
      ExactDiametralPhiDeviceSign::invalid;
  if (interval_sign(
          witness_bits,
          first_support_bits,
          second_support_bits,
          filtered_sign)) {
    record.sign = filtered_sign;
    record.stage = ExactDiametralPhiDeviceStage::fp64_interval;
  } else {
    const exact_phi_fixed::ResultCode exact_code = exact_phi_fixed::exact_sign(
        witness_bits, first_support_bits, second_support_bits);
    record.sign = fixed_sign(exact_code);
    if (exact_code == exact_phi_fixed::ResultCode::requires_cpu_fallback) {
      record.stage =
          ExactDiametralPhiDeviceStage::requires_cpu_fallback;
    } else if (exact_code == exact_phi_fixed::ResultCode::invalid_input) {
      record.stage = ExactDiametralPhiDeviceStage::invalid;
    } else {
      record.stage = ExactDiametralPhiDeviceStage::fixed_limb_dyadic;
    }
  }
  records[query_index] = record;
}

}  // namespace

ExactDiametralPhiDeviceBatch decide_exact_diametral_phi_on_gpu(
    ExactDiametralPhiContextState& context,
    std::span<const ExactDiametralPhiInput> inputs,
    std::size_t maximum_query_count) {
  if (inputs.empty()) {
    throw std::invalid_argument(
        "the exact diametral-Phi launcher does not accept an empty batch");
  }
  if (inputs.size() > maximum_query_count ||
      inputs.size() >
          std::numeric_limits<std::size_t>::max() / coordinate_field_count) {
    throw std::length_error(
        "the exact diametral-Phi launch exceeds its fixed capacity");
  }
  const std::size_t block_count =
      (inputs.size() - 1U) / threads_per_block + 1U;
  CudaResources& cuda = resources(context, maximum_query_count);
  if (block_count > cuda.maximum_grid_x()) {
    throw std::length_error(
        "the exact diametral-Phi batch exceeds the device CUDA grid");
  }

  std::vector<std::uint64_t> coordinate_bits(
      inputs.size() * coordinate_field_count);
  for (std::size_t index = 0U; index < inputs.size(); ++index) {
    for (std::size_t axis = 0U; axis < axis_count; ++axis) {
      coordinate_bits[(0U * axis_count + axis) * inputs.size() + index] =
          inputs[index].witness_bits[axis];
      coordinate_bits[(1U * axis_count + axis) * inputs.size() + index] =
          inputs[index].first_support_bits[axis];
      coordinate_bits[(2U * axis_count + axis) * inputs.size() + index] =
          inputs[index].second_support_bits[axis];
    }
  }

  DeviceGuard device_guard{cuda.device()};
  check_cuda(
        cudaMemcpyAsync(
            cuda.coordinate_bits(),
            coordinate_bits.data(),
            coordinate_bits.size() * sizeof(std::uint64_t),
            cudaMemcpyHostToDevice,
            cuda.stream()),
        "cudaMemcpyAsync exact diametral-Phi coordinates host-to-device");
  morsehgp3d_phase15_exact_diametral_phi_kernel
        <<<static_cast<unsigned int>(block_count), threads_per_block, 0U,
           cuda.stream()>>>(
            cuda.coordinate_bits(), cuda.records(), inputs.size());
  check_cuda(
        cudaGetLastError(), "Phase 15 exact diametral-Phi kernel launch");

  ExactDiametralPhiDeviceBatch batch;
  batch.records.resize(inputs.size());
  check_cuda(
        cudaMemcpyAsync(
            batch.records.data(),
            cuda.records(),
            batch.records.size() * sizeof(ExactDiametralPhiDeviceRecord),
            cudaMemcpyDeviceToHost,
            cuda.stream()),
        "cudaMemcpyAsync exact diametral-Phi records device-to-host");
  check_cuda(
        cudaStreamSynchronize(cuda.stream()),
        "cudaStreamSynchronize exact diametral-Phi");
  batch.maximum_query_count = maximum_query_count;
  batch.launcher_call_count = 1U;
  batch.kernel_launch_count = 1U;
  batch.synchronization_count = 1U;
  batch.cuda_device = cuda.device();
  batch.execution_kind = ExactDiametralPhiExecutionKind::cuda;
  batch.exact_fixed_limb_implemented = true;
  batch.epsilon_used = false;
  device_guard.restore();
  return batch;
}

}  // namespace morsehgp3d::gpu::detail
