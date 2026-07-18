#include "phase4_spatial_reference_internal.hpp"

#include <cuda_runtime.h>
#include <cuda/std/bit>

#include <algorithm>
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
#error "phase4_spatial_reference.cu must be compiled ahead of time with NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Phase 4 spatial reference requires the CUDA 12.9 compiler"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the Phase 4 spatial reference"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the Phase 4 spatial reference"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Phase 4 spatial reference must contain only sm_120 device code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

constexpr unsigned int kThreadsPerBlock = 256U;
constexpr std::size_t kAxisCount = 3U;

class SpatialReferenceCudaFailure final : public std::runtime_error {
 public:
  SpatialReferenceCudaFailure(cudaError_t code, std::string operation)
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
    throw SpatialReferenceCudaFailure(code, std::move(operation));
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
    if (data_ != nullptr) {
      throw std::logic_error("a Phase 4 device buffer was initialized twice");
    }
    if (count == 0U ||
        count > std::numeric_limits<std::size_t>::max() / sizeof(Value)) {
      throw std::length_error("a Phase 4 device allocation size is invalid");
    }
    check_cuda(
        cudaMalloc(reinterpret_cast<void**>(&data_), count * sizeof(Value)),
        operation);
    count_ = count;
  }

  void reset() noexcept {
    if (data_ != nullptr) {
      static_cast<void>(cudaFree(data_));
      data_ = nullptr;
      count_ = 0U;
    }
  }

  void abandon() noexcept {
    data_ = nullptr;
    count_ = 0U;
  }

  [[nodiscard]] Value* get() noexcept { return data_; }
  [[nodiscard]] std::size_t count() const noexcept { return count_; }

 private:
  Value* data_{nullptr};
  std::size_t count_{0U};
};

class DeviceGuard final {
 public:
  explicit DeviceGuard(int target_device) {
    check_cuda(
        cudaGetDevice(&previous_device_),
        "cudaGetDevice before Phase 4 spatial proposal");
    if (previous_device_ != target_device) {
      check_cuda(
          cudaSetDevice(target_device),
          "cudaSetDevice for Phase 4 spatial context");
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
          "cudaSetDevice restore after Phase 4 spatial proposal");
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

  int previous_device_{0};
  bool restore_required_{false};
};

class SpatialReferenceCudaResources final {
 public:
  SpatialReferenceCudaResources() {
    check_cuda(
        cudaGetDevice(&device_),
        "cudaGetDevice for Phase 4 spatial context creation");
    cudaDeviceProp properties{};
    check_cuda(
        cudaGetDeviceProperties(&properties, device_),
        "cudaGetDeviceProperties for Phase 4 spatial context");
    if (properties.maxGridSize[0] <= 0) {
      throw std::runtime_error("the CUDA device exposes no one-dimensional grid");
    }
    maximum_grid_x_ = static_cast<unsigned int>(properties.maxGridSize[0]);
    check_cuda(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags for Phase 4 spatial context");
  }

  SpatialReferenceCudaResources(const SpatialReferenceCudaResources&) = delete;
  SpatialReferenceCudaResources& operator=(
      const SpatialReferenceCudaResources&) = delete;

  ~SpatialReferenceCudaResources() {
    int previous_device = 0;
    const cudaError_t query_status = cudaGetDevice(&previous_device);
    bool restore_device = false;
    if (query_status == cudaSuccess && previous_device != device_) {
      restore_device = cudaSetDevice(device_) == cudaSuccess;
    }
    if (query_status != cudaSuccess ||
        (previous_device != device_ && !restore_device)) {
      records_.abandon();
      coordinate_bits_.abandon();
      stream_ = nullptr;
      return;
    }
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
    records_.reset();
    coordinate_bits_.reset();
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamDestroy(stream_));
      stream_ = nullptr;
    }
    if (restore_device) {
      static_cast<void>(cudaSetDevice(previous_device));
    }
  }

  void initialize_points(
      std::span<const std::uint64_t> coordinate_bits,
      std::size_t point_count) {
    if (point_count_ != 0U) {
      if (point_count_ != point_count ||
          coordinate_bits_.count() != coordinate_bits.size() ||
          records_.count() != point_count) {
        throw std::logic_error(
            "a Phase 4 CUDA context was reused with another point namespace");
      }
      return;
    }
    if (point_count == 0U || coordinate_bits.size() != point_count * kAxisCount) {
      throw std::invalid_argument("invalid Phase 4 spatial coordinate packing");
    }
    coordinate_bits_.allocate(
        coordinate_bits.size(), "cudaMalloc Phase 4 point coordinates");
    records_.allocate(point_count, "cudaMalloc Phase 4 proposal records");
    check_cuda(
        cudaMemcpyAsync(
            coordinate_bits_.get(),
            coordinate_bits.data(),
            coordinate_bits.size() * sizeof(std::uint64_t),
            cudaMemcpyHostToDevice,
            stream_),
        "cudaMemcpyAsync Phase 4 point coordinates host-to-device");
    synchronize();
    point_count_ = point_count;
  }

  [[nodiscard]] int device() const noexcept { return device_; }
  [[nodiscard]] cudaStream_t stream() const noexcept { return stream_; }
  [[nodiscard]] std::uint64_t* coordinates() noexcept {
    return coordinate_bits_.get();
  }
  [[nodiscard]] SpatialProposalRecord* records() noexcept {
    return records_.get();
  }
  [[nodiscard]] unsigned int maximum_grid_x() const noexcept {
    return maximum_grid_x_;
  }

  void synchronize() {
    check_cuda(
        cudaStreamSynchronize(stream_),
        "cudaStreamSynchronize Phase 4 spatial context");
  }

  void synchronize_after_failure() noexcept {
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
  }

 private:
  int device_{-1};
  cudaStream_t stream_{nullptr};
  unsigned int maximum_grid_x_{0U};
  std::size_t point_count_{0U};
  DeviceBuffer<std::uint64_t> coordinate_bits_;
  DeviceBuffer<SpatialProposalRecord> records_;
};

[[nodiscard]] SpatialReferenceCudaResources& resources(
    SpatialReferenceContextState& state) {
  std::shared_ptr<void>& opaque = state.cuda_resources();
  if (!opaque) {
    opaque = std::make_shared<SpatialReferenceCudaResources>();
  }
  return *static_cast<SpatialReferenceCudaResources*>(opaque.get());
}

[[nodiscard]] __device__ double coordinate(
    const std::uint64_t* coordinate_bits,
    std::size_t point_count,
    std::size_t point_index,
    std::size_t axis) noexcept {
  return cuda::std::bit_cast<double>(
      coordinate_bits[axis * point_count + point_index]);
}

__global__ void morsehgp3d_phase4_spatial_reference_kernel(
    const std::uint64_t* coordinate_bits,
    SpatialProposalRecord* records,
    std::size_t point_count,
    std::uint64_t query_x_bits,
    std::uint64_t query_y_bits,
    std::uint64_t query_z_bits) {
  const std::uint64_t query_bits[kAxisCount]{
      query_x_bits, query_y_bits, query_z_bits};
  const std::size_t first =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  const std::size_t stride =
      static_cast<std::size_t>(gridDim.x) * blockDim.x;
  std::size_t index = first;
  while (index < point_count) {
    double squared_distance = 0.0;
    for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
      const double query_coordinate =
          cuda::std::bit_cast<double>(query_bits[axis]);
      const double difference = __dsub_rn(
          coordinate(coordinate_bits, point_count, index, axis),
          query_coordinate);
      const double square = __dmul_rn(difference, difference);
      squared_distance = __dadd_rn(squared_distance, square);
    }
    records[index] = SpatialProposalRecord{
        static_cast<std::uint64_t>(index),
        cuda::std::bit_cast<std::uint64_t>(squared_distance)};
    if (point_count - index <= stride) {
      break;
    }
    index += stride;
  }
}

}  // namespace

SpatialProposalBatch propose_squared_distances_on_gpu(
    SpatialReferenceContextState& context,
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    const std::array<std::uint64_t, 3>& query_bits) {
  if (point_count == 0U ||
      point_count > std::numeric_limits<std::size_t>::max() / kAxisCount ||
      coordinate_bits.size() != point_count * kAxisCount) {
    throw std::invalid_argument("invalid Phase 4 spatial proposal input");
  }

  // The host context owns the surrounding with_gpu_section transaction so
  // copy-back validation and poisoning are serialized with this launch.
  SpatialReferenceCudaResources& cuda = resources(context);
  DeviceGuard device_guard{cuda.device()};
  try {
    cuda.initialize_points(coordinate_bits, point_count);
    check_cuda(
        cudaMemsetAsync(
            cuda.records(),
            0xff,
            point_count * sizeof(SpatialProposalRecord),
            cuda.stream()),
        "cudaMemsetAsync Phase 4 proposal sentinels");

    const std::size_t required_blocks =
        (point_count - 1U) / kThreadsPerBlock + 1U;
    const std::size_t bounded_blocks = std::min(
        required_blocks,
        static_cast<std::size_t>(cuda.maximum_grid_x()));
    if (bounded_blocks == 0U ||
        bounded_blocks > std::numeric_limits<unsigned int>::max()) {
      throw std::length_error("the Phase 4 CUDA grid is not representable");
    }
    morsehgp3d_phase4_spatial_reference_kernel
        <<<static_cast<unsigned int>(bounded_blocks),
           kThreadsPerBlock,
           0U,
           cuda.stream()>>>(
            cuda.coordinates(),
            cuda.records(),
            point_count,
            query_bits[0],
            query_bits[1],
            query_bits[2]);
    check_cuda(cudaGetLastError(), "Phase 4 spatial proposal launch");

    SpatialProposalBatch batch;
    batch.records.resize(point_count);
    check_cuda(
        cudaMemcpyAsync(
            batch.records.data(),
            cuda.records(),
            point_count * sizeof(SpatialProposalRecord),
            cudaMemcpyDeviceToHost,
            cuda.stream()),
        "cudaMemcpyAsync Phase 4 proposal records device-to-host");
    cuda.synchronize();
    batch.buffer_epoch = context.advance_epoch();
    device_guard.restore();
    return batch;
  } catch (...) {
    cuda.synchronize_after_failure();
    throw;
  }
}

}  // namespace morsehgp3d::gpu::detail
