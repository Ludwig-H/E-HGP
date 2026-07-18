#include "phase4_spatial_bounds_internal.hpp"

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

#if !defined(__CUDACC__)
#error "phase4_spatial_bounds.cu must be compiled ahead of time with NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Phase 4 spatial-bounds filter requires the CUDA 12.9 compiler"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the Phase 4 spatial-bounds filter"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the Phase 4 spatial-bounds filter"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Phase 4 spatial-bounds filter must contain only sm_120 device code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

constexpr unsigned int kThreadsPerBlock = 256U;
constexpr std::size_t kAxisCount = 3U;
constexpr std::uint64_t kExponentMask = UINT64_C(0x7ff0000000000000);
constexpr std::uint64_t kPositiveInfinityBits =
    UINT64_C(0x7ff0000000000000);

class SpatialBoundsCudaFailure final : public std::runtime_error {
 public:
  SpatialBoundsCudaFailure(cudaError_t code, std::string operation)
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
    throw SpatialBoundsCudaFailure(code, std::move(operation));
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
      throw std::logic_error(
          "a Phase 4 spatial-bounds device buffer was initialized twice");
    }
    if (count == 0U ||
        count > std::numeric_limits<std::size_t>::max() / sizeof(Value)) {
      throw std::length_error(
          "a Phase 4 spatial-bounds device allocation size is invalid");
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
  [[nodiscard]] const Value* get() const noexcept { return data_; }
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
        "cudaGetDevice before Phase 4 spatial-bounds proposal");
    if (previous_device_ != target_device) {
      check_cuda(
          cudaSetDevice(target_device),
          "cudaSetDevice for Phase 4 spatial-bounds context");
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
          "cudaSetDevice restore after Phase 4 spatial-bounds proposal");
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

class SpatialBoundsCudaResources final {
 public:
  SpatialBoundsCudaResources() {
    check_cuda(
        cudaGetDevice(&device_),
        "cudaGetDevice for Phase 4 spatial-bounds context creation");
    cudaDeviceProp properties{};
    check_cuda(
        cudaGetDeviceProperties(&properties, device_),
        "cudaGetDeviceProperties for Phase 4 spatial-bounds context");
    if (properties.maxGridSize[0] <= 0) {
      throw std::runtime_error(
          "the CUDA device exposes no one-dimensional grid");
    }
    maximum_grid_x_ = static_cast<unsigned int>(properties.maxGridSize[0]);
    check_cuda(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags for Phase 4 spatial-bounds context");
  }

  SpatialBoundsCudaResources(const SpatialBoundsCudaResources&) = delete;
  SpatialBoundsCudaResources& operator=(
      const SpatialBoundsCudaResources&) = delete;

  ~SpatialBoundsCudaResources() {
    int previous_device = 0;
    const cudaError_t query_status = cudaGetDevice(&previous_device);
    bool restore_device = false;
    if (query_status == cudaSuccess && previous_device != device_) {
      restore_device = cudaSetDevice(device_) == cudaSuccess;
    }
    if (query_status != cudaSuccess ||
        (previous_device != device_ && !restore_device)) {
      records_.abandon();
      boxes_.abandon();
      stream_ = nullptr;
      return;
    }
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
    records_.reset();
    boxes_.reset();
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamDestroy(stream_));
      stream_ = nullptr;
    }
    if (restore_device) {
      static_cast<void>(cudaSetDevice(previous_device));
    }
  }

  void initialize_boxes(std::span<const SpatialBoundsInputRecord> boxes) {
    if (box_count_ != 0U) {
      if (box_count_ != boxes.size() || boxes_.count() != boxes.size() ||
          records_.count() != boxes.size()) {
        throw std::logic_error(
            "a Phase 4 spatial-bounds context was reused with another AABB batch");
      }
      return;
    }
    if (boxes.empty()) {
      throw std::invalid_argument(
          "a Phase 4 spatial-bounds context requires at least one AABB");
    }
    boxes_.allocate(boxes.size(), "cudaMalloc Phase 4 spatial-bounds AABBs");
    records_.allocate(
        boxes.size(), "cudaMalloc Phase 4 spatial-bounds proposal records");
    check_cuda(
        cudaMemcpyAsync(
            boxes_.get(),
            boxes.data(),
            boxes.size() * sizeof(SpatialBoundsInputRecord),
            cudaMemcpyHostToDevice,
            stream_),
        "cudaMemcpyAsync Phase 4 spatial-bounds AABBs host-to-device");
    synchronize();
    box_count_ = boxes.size();
  }

  [[nodiscard]] int device() const noexcept { return device_; }
  [[nodiscard]] cudaStream_t stream() const noexcept { return stream_; }
  [[nodiscard]] const SpatialBoundsInputRecord* boxes() const noexcept {
    return boxes_.get();
  }
  [[nodiscard]] SpatialBoundsProposalRecord* records() noexcept {
    return records_.get();
  }
  [[nodiscard]] unsigned int maximum_grid_x() const noexcept {
    return maximum_grid_x_;
  }

  void synchronize() {
    check_cuda(
        cudaStreamSynchronize(stream_),
        "cudaStreamSynchronize Phase 4 spatial-bounds context");
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
  std::size_t box_count_{0U};
  DeviceBuffer<SpatialBoundsInputRecord> boxes_;
  DeviceBuffer<SpatialBoundsProposalRecord> records_;
};

[[nodiscard]] SpatialBoundsCudaResources& resources(
    SpatialBoundsContextState& state) {
  std::shared_ptr<void>& opaque = state.cuda_resources();
  if (!opaque) {
    opaque = std::make_shared<SpatialBoundsCudaResources>();
  }
  return *static_cast<SpatialBoundsCudaResources*>(opaque.get());
}

class SpatialLbvhCudaResources final {
 public:
  SpatialLbvhCudaResources() {
    check_cuda(
        cudaGetDevice(&device_),
        "cudaGetDevice for Phase 4 spatial-LBVH context creation");
    cudaDeviceProp properties{};
    check_cuda(
        cudaGetDeviceProperties(&properties, device_),
        "cudaGetDeviceProperties for Phase 4 spatial-LBVH context");
    if (properties.maxGridSize[0] <= 0) {
      throw std::runtime_error(
          "the CUDA device exposes no one-dimensional LBVH grid");
    }
    maximum_grid_x_ = static_cast<unsigned int>(properties.maxGridSize[0]);
    check_cuda(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags for Phase 4 spatial-LBVH context");
  }

  SpatialLbvhCudaResources(const SpatialLbvhCudaResources&) = delete;
  SpatialLbvhCudaResources& operator=(const SpatialLbvhCudaResources&) = delete;

  ~SpatialLbvhCudaResources() {
    int previous_device = 0;
    const cudaError_t query_status = cudaGetDevice(&previous_device);
    bool restore_device = false;
    if (query_status == cudaSuccess && previous_device != device_) {
      restore_device = cudaSetDevice(device_) == cudaSuccess;
    }
    if (query_status != cudaSuccess ||
        (previous_device != device_ && !restore_device)) {
      failure_code_.abandon();
      next_frontier_count_.abandon();
      record_count_.abandon();
      records_.abandon();
      next_frontier_.abandon();
      current_frontier_.abandon();
      nodes_.abandon();
      stream_ = nullptr;
      return;
    }
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
    failure_code_.reset();
    next_frontier_count_.reset();
    record_count_.reset();
    records_.reset();
    next_frontier_.reset();
    current_frontier_.reset();
    nodes_.reset();
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamDestroy(stream_));
      stream_ = nullptr;
    }
    if (restore_device) {
      static_cast<void>(cudaSetDevice(previous_device));
    }
  }

  void initialize_nodes(std::span<const SpatialLbvhNodeInputRecord> nodes) {
    if (node_count_ != 0U) {
      if (node_count_ != nodes.size() || nodes_.count() != nodes.size() ||
          current_frontier_.count() != nodes.size() ||
          next_frontier_.count() != nodes.size() ||
          records_.count() != nodes.size() || record_count_.count() != 1U ||
          next_frontier_count_.count() != 1U ||
          failure_code_.count() != 1U) {
        throw std::logic_error(
            "a Phase 4 spatial-LBVH context was reused with another tree");
      }
      return;
    }
    if (nodes.empty()) {
      throw std::invalid_argument(
          "a Phase 4 spatial-LBVH context requires at least one node");
    }
    nodes_.allocate(nodes.size(), "cudaMalloc Phase 4 spatial-LBVH nodes");
    current_frontier_.allocate(
        nodes.size(), "cudaMalloc Phase 4 spatial-LBVH current frontier");
    next_frontier_.allocate(
        nodes.size(), "cudaMalloc Phase 4 spatial-LBVH next frontier");
    records_.allocate(
        nodes.size(), "cudaMalloc Phase 4 spatial-LBVH cover records");
    record_count_.allocate(
        1U, "cudaMalloc Phase 4 spatial-LBVH cover count");
    next_frontier_count_.allocate(
        1U, "cudaMalloc Phase 4 spatial-LBVH next-frontier count");
    failure_code_.allocate(
        1U, "cudaMalloc Phase 4 spatial-LBVH failure code");
    check_cuda(
        cudaMemcpyAsync(
            nodes_.get(),
            nodes.data(),
            nodes.size() * sizeof(SpatialLbvhNodeInputRecord),
            cudaMemcpyHostToDevice,
            stream_),
        "cudaMemcpyAsync Phase 4 spatial-LBVH nodes host-to-device");
    synchronize();
    node_count_ = nodes.size();
  }

  [[nodiscard]] int device() const noexcept { return device_; }
  [[nodiscard]] cudaStream_t stream() const noexcept { return stream_; }
  [[nodiscard]] const SpatialLbvhNodeInputRecord* nodes() const noexcept {
    return nodes_.get();
  }
  [[nodiscard]] std::uint64_t* current_frontier() noexcept {
    return current_frontier_.get();
  }
  [[nodiscard]] std::uint64_t* next_frontier() noexcept {
    return next_frontier_.get();
  }
  [[nodiscard]] SpatialLbvhCoverRecord* records() noexcept {
    return records_.get();
  }
  [[nodiscard]] std::uint64_t* record_count() noexcept {
    return record_count_.get();
  }
  [[nodiscard]] std::uint64_t* next_frontier_count() noexcept {
    return next_frontier_count_.get();
  }
  [[nodiscard]] std::uint64_t* failure_code() noexcept {
    return failure_code_.get();
  }
  [[nodiscard]] unsigned int maximum_grid_x() const noexcept {
    return maximum_grid_x_;
  }

  void synchronize() {
    check_cuda(
        cudaStreamSynchronize(stream_),
        "cudaStreamSynchronize Phase 4 spatial-LBVH context");
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
  std::size_t node_count_{0U};
  DeviceBuffer<SpatialLbvhNodeInputRecord> nodes_;
  DeviceBuffer<std::uint64_t> current_frontier_;
  DeviceBuffer<std::uint64_t> next_frontier_;
  DeviceBuffer<SpatialLbvhCoverRecord> records_;
  DeviceBuffer<std::uint64_t> record_count_;
  DeviceBuffer<std::uint64_t> next_frontier_count_;
  DeviceBuffer<std::uint64_t> failure_code_;
};

[[nodiscard]] SpatialLbvhCudaResources& lbvh_resources(
    SpatialLbvhContextState& state) {
  std::shared_ptr<void>& opaque = state.cuda_resources();
  if (!opaque) {
    opaque = std::make_shared<SpatialLbvhCudaResources>();
  }
  return *static_cast<SpatialLbvhCudaResources*>(opaque.get());
}

[[nodiscard]] __device__ bool finite_bits(std::uint64_t bits) noexcept {
  return (bits & kExponentMask) != kExponentMask;
}

[[nodiscard]] __device__ double value_from_bits(std::uint64_t bits) noexcept {
  return cuda::std::bit_cast<double>(bits);
}

[[nodiscard]] __device__ std::uint64_t bits_from_value(double value) noexcept {
  return cuda::std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] __device__ bool finite_value(double value) noexcept {
  return finite_bits(bits_from_value(value));
}

struct DirectedSquaredDistance {
  double lower{0.0};
  double upper{0.0};
  bool valid{true};
};

[[nodiscard]] __device__ DirectedSquaredDistance directed_squared_distance(
    const SpatialBoundsInputRecord& box,
    const std::uint64_t* query_lower_bits,
    const std::uint64_t* query_upper_bits) noexcept {
  DirectedSquaredDistance result;
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const std::uint64_t box_lower_bits = box.lower_bits[axis];
    const std::uint64_t box_upper_bits = box.upper_bits[axis];
    if (!finite_bits(box_lower_bits) || !finite_bits(box_upper_bits) ||
        !finite_bits(query_lower_bits[axis]) ||
        !finite_bits(query_upper_bits[axis])) {
      result.valid = false;
      return result;
    }

    const double box_lower = value_from_bits(box_lower_bits);
    const double box_upper = value_from_bits(box_upper_bits);
    const double query_lower = value_from_bits(query_lower_bits[axis]);
    const double query_upper = value_from_bits(query_upper_bits[axis]);
    if (box_lower > box_upper || query_lower > query_upper) {
      result.valid = false;
      return result;
    }

    double lower_delta = 0.0;
    if (query_upper < box_lower) {
      lower_delta = __dsub_rd(box_lower, query_upper);
    } else if (query_lower > box_upper) {
      lower_delta = __dsub_rd(query_lower, box_upper);
    }

    double upper_delta_from_lower = 0.0;
    if (query_lower < box_lower) {
      upper_delta_from_lower = __dsub_ru(box_lower, query_lower);
    }
    double upper_delta_from_upper = 0.0;
    if (query_upper > box_upper) {
      upper_delta_from_upper = __dsub_ru(query_upper, box_upper);
    }
    const double upper_delta =
        upper_delta_from_lower > upper_delta_from_upper
            ? upper_delta_from_lower
            : upper_delta_from_upper;

    if (!finite_value(lower_delta) || !finite_value(upper_delta) ||
        lower_delta < 0.0 || upper_delta < 0.0 ||
        lower_delta > upper_delta) {
      result.valid = false;
      return result;
    }

    const double lower_square = __dmul_rd(lower_delta, lower_delta);
    const double upper_square = __dmul_ru(upper_delta, upper_delta);
    const double next_lower = __dadd_rd(result.lower, lower_square);
    const double next_upper = __dadd_ru(result.upper, upper_square);
    if (!finite_value(lower_square) || !finite_value(upper_square) ||
        !finite_value(next_lower) || !finite_value(next_upper) ||
        lower_square < 0.0 || upper_square < 0.0 || next_lower < 0.0 ||
        next_upper < 0.0 || lower_square > upper_square ||
        next_lower > next_upper) {
      result.lower = next_lower;
      result.upper = next_upper;
      result.valid = false;
      return result;
    }
    result.lower = next_lower;
    result.upper = next_upper;
  }
  return result;
}

[[nodiscard]] __device__ SpatialBoundsProposalRecord unknown_record(
    std::size_t box_index,
    double lower_squared_distance,
    double upper_squared_distance,
    bool preserve_finite_enclosure) noexcept {
  const bool finite_ordered_enclosure =
      finite_value(lower_squared_distance) &&
      finite_value(upper_squared_distance) &&
      lower_squared_distance >= 0.0 &&
      lower_squared_distance <= upper_squared_distance;
  const std::uint64_t lower_bits =
      preserve_finite_enclosure && finite_ordered_enclosure
          ? bits_from_value(lower_squared_distance)
          : UINT64_C(0);
  const std::uint64_t upper_bits =
      preserve_finite_enclosure && finite_ordered_enclosure
          ? bits_from_value(upper_squared_distance)
          : kPositiveInfinityBits;
  return SpatialBoundsProposalRecord{
      static_cast<std::uint64_t>(box_index),
      lower_bits,
      upper_bits,
      spatial_bounds_unknown_code};
}

__global__ void morsehgp3d_phase4_spatial_bounds_kernel(
    const SpatialBoundsInputRecord* boxes,
    SpatialBoundsProposalRecord* records,
    std::size_t box_count,
    std::uint64_t query_lower_x_bits,
    std::uint64_t query_lower_y_bits,
    std::uint64_t query_lower_z_bits,
    std::uint64_t query_upper_x_bits,
    std::uint64_t query_upper_y_bits,
    std::uint64_t query_upper_z_bits,
    std::uint64_t cutoff_lower_bits,
    std::uint64_t cutoff_upper_bits) {
  const std::uint64_t query_lower_bits[kAxisCount]{
      query_lower_x_bits, query_lower_y_bits, query_lower_z_bits};
  const std::uint64_t query_upper_bits[kAxisCount]{
      query_upper_x_bits, query_upper_y_bits, query_upper_z_bits};
  const bool cutoff_bits_finite =
      finite_bits(cutoff_lower_bits) && finite_bits(cutoff_upper_bits);
  const double cutoff_lower = value_from_bits(cutoff_lower_bits);
  const double cutoff_upper = value_from_bits(cutoff_upper_bits);
  const bool cutoff_valid = cutoff_bits_finite && cutoff_lower >= 0.0 &&
                            cutoff_lower <= cutoff_upper;

  const std::size_t first =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  const std::size_t stride =
      static_cast<std::size_t>(gridDim.x) * blockDim.x;
  std::size_t index = first;
  while (index < box_count) {
    const DirectedSquaredDistance distance = directed_squared_distance(
        boxes[index], query_lower_bits, query_upper_bits);
    if (!distance.valid || !cutoff_valid) {
      records[index] = unknown_record(
          index, distance.lower, distance.upper, false);
    } else {
      std::uint64_t decision_code = spatial_bounds_unknown_code;
      if (distance.lower > cutoff_upper) {
        decision_code = spatial_bounds_prune_code;
      } else if (distance.upper < cutoff_lower) {
        decision_code = spatial_bounds_visit_code;
      }
      records[index] = SpatialBoundsProposalRecord{
          static_cast<std::uint64_t>(index),
          bits_from_value(distance.lower),
          bits_from_value(distance.upper),
          decision_code};
    }
    if (box_count - index <= stride) {
      break;
    }
    index += stride;
  }
}

[[nodiscard]] __device__ SpatialLbvhCoverRecord lbvh_cover_record(
    std::uint64_t node_index,
    const DirectedSquaredDistance& distance,
    std::uint64_t kind) noexcept {
  const bool finite_ordered_enclosure =
      distance.valid && finite_value(distance.lower) &&
      finite_value(distance.upper) && distance.lower >= 0.0 &&
      distance.lower <= distance.upper;
  return SpatialLbvhCoverRecord{
      node_index,
      finite_ordered_enclosure ? bits_from_value(distance.lower)
                               : UINT64_C(0),
      finite_ordered_enclosure ? bits_from_value(distance.upper)
                               : kPositiveInfinityBits,
      kind};
}

__device__ void signal_lbvh_failure(
    std::uint64_t* failure_code,
    std::uint64_t code) noexcept {
  static_assert(sizeof(std::uint64_t) == sizeof(unsigned long long));
  static_cast<void>(atomicCAS(
      reinterpret_cast<unsigned long long*>(failure_code),
      0ULL,
      static_cast<unsigned long long>(code)));
}

__device__ void append_lbvh_cover_record(
    SpatialLbvhCoverRecord* records,
    std::uint64_t* record_count,
    std::uint64_t* failure_code,
    std::size_t node_count,
    const SpatialLbvhCoverRecord& record) noexcept {
  const unsigned long long position = atomicAdd(
      reinterpret_cast<unsigned long long*>(record_count), 1ULL);
  if (position >= static_cast<unsigned long long>(node_count)) {
    signal_lbvh_failure(failure_code, UINT64_C(1));
    return;
  }
  records[static_cast<std::size_t>(position)] = record;
}

__global__ void morsehgp3d_phase4_spatial_lbvh_frontier_kernel(
    const SpatialLbvhNodeInputRecord* nodes,
    const std::uint64_t* current_frontier,
    std::uint64_t* next_frontier,
    SpatialLbvhCoverRecord* records,
    std::uint64_t* record_count,
    std::uint64_t* next_frontier_count,
    std::uint64_t* failure_code,
    std::size_t node_count,
    std::size_t current_frontier_count,
    std::uint64_t query_lower_x_bits,
    std::uint64_t query_lower_y_bits,
    std::uint64_t query_lower_z_bits,
    std::uint64_t query_upper_x_bits,
    std::uint64_t query_upper_y_bits,
    std::uint64_t query_upper_z_bits,
    std::uint64_t cutoff_lower_bits,
    std::uint64_t cutoff_upper_bits) {
  const std::uint64_t query_lower_bits[kAxisCount]{
      query_lower_x_bits, query_lower_y_bits, query_lower_z_bits};
  const std::uint64_t query_upper_bits[kAxisCount]{
      query_upper_x_bits, query_upper_y_bits, query_upper_z_bits};
  const bool cutoff_bits_finite =
      finite_bits(cutoff_lower_bits) && finite_bits(cutoff_upper_bits);
  const double cutoff_lower = value_from_bits(cutoff_lower_bits);
  const double cutoff_upper = value_from_bits(cutoff_upper_bits);
  const bool cutoff_valid = cutoff_bits_finite && cutoff_lower >= 0.0 &&
                            cutoff_lower <= cutoff_upper;

  const std::size_t first =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  const std::size_t stride =
      static_cast<std::size_t>(gridDim.x) * blockDim.x;
  std::size_t frontier_position = first;
  while (frontier_position < current_frontier_count) {
    const std::uint64_t node_index = current_frontier[frontier_position];
    if (node_index >= static_cast<std::uint64_t>(node_count)) {
      signal_lbvh_failure(failure_code, UINT64_C(2));
      if (current_frontier_count - frontier_position <= stride) {
        break;
      }
      frontier_position += stride;
      continue;
    }

    const SpatialLbvhNodeInputRecord& node = nodes[node_index];
    const DirectedSquaredDistance distance = directed_squared_distance(
        node.bounds, query_lower_bits, query_upper_bits);
    if (distance.valid && cutoff_valid && distance.lower > cutoff_upper) {
      append_lbvh_cover_record(
          records,
          record_count,
          failure_code,
          node_count,
          lbvh_cover_record(
              node_index, distance, spatial_lbvh_cover_prune_code));
    } else {
      const bool left_missing =
          node.left_child == spatial_bounds_sentinel_code;
      const bool right_missing =
          node.right_child == spatial_bounds_sentinel_code;
      if (left_missing && right_missing) {
        const bool leaf_range_valid =
            node.leaf_begin < node.leaf_end &&
            node.leaf_end - node.leaf_begin == UINT64_C(1);
        if (!leaf_range_valid) {
          signal_lbvh_failure(failure_code, UINT64_C(3));
        } else {
          append_lbvh_cover_record(
              records,
              record_count,
              failure_code,
              node_count,
              lbvh_cover_record(
                  node_index, distance, spatial_lbvh_cover_leaf_code));
        }
      } else {
        const bool children_valid =
            !left_missing && !right_missing &&
            node.left_child < node_index && node.right_child < node_index &&
            node.left_child != node.right_child &&
            node.left_child < static_cast<std::uint64_t>(node_count) &&
            node.right_child < static_cast<std::uint64_t>(node_count) &&
            node_count >= 2U;
        if (!children_valid) {
          signal_lbvh_failure(failure_code, UINT64_C(4));
        } else {
          const unsigned long long next_position = atomicAdd(
              reinterpret_cast<unsigned long long*>(next_frontier_count),
              2ULL);
          if (next_position >= static_cast<unsigned long long>(node_count) ||
              static_cast<unsigned long long>(node_count) - next_position <
                  UINT64_C(2)) {
            signal_lbvh_failure(failure_code, UINT64_C(5));
          } else {
            next_frontier[static_cast<std::size_t>(next_position)] =
                node.left_child;
            next_frontier[static_cast<std::size_t>(next_position + 1U)] =
                node.right_child;
          }
        }
      }
    }
    if (current_frontier_count - frontier_position <= stride) {
      break;
    }
    frontier_position += stride;
  }
}

}  // namespace

SpatialBoundsProposalBatch propose_strict_aabb_prunes_on_gpu(
    SpatialBoundsContextState& context,
    std::span<const SpatialBoundsInputRecord> boxes,
    const std::array<std::uint64_t, 3>& query_lower_bits,
    const std::array<std::uint64_t, 3>& query_upper_bits,
    std::uint64_t cutoff_lower_bits,
    std::uint64_t cutoff_upper_bits) {
  if (boxes.empty()) {
    throw std::invalid_argument(
        "a Phase 4 spatial-bounds proposal requires at least one AABB");
  }

  SpatialBoundsCudaResources& cuda = resources(context);
  DeviceGuard device_guard{cuda.device()};
  try {
    cuda.initialize_boxes(boxes);
    check_cuda(
        cudaMemsetAsync(
            cuda.records(),
            0xff,
            boxes.size() * sizeof(SpatialBoundsProposalRecord),
            cuda.stream()),
        "cudaMemsetAsync Phase 4 spatial-bounds proposal sentinels");

    const std::size_t required_blocks =
        (boxes.size() - 1U) / kThreadsPerBlock + 1U;
    const std::size_t bounded_blocks = std::min(
        required_blocks,
        static_cast<std::size_t>(cuda.maximum_grid_x()));
    if (bounded_blocks == 0U ||
        bounded_blocks > std::numeric_limits<unsigned int>::max()) {
      throw std::length_error(
          "the Phase 4 spatial-bounds CUDA grid is not representable");
    }

    morsehgp3d_phase4_spatial_bounds_kernel
        <<<static_cast<unsigned int>(bounded_blocks),
           kThreadsPerBlock,
           0U,
           cuda.stream()>>>(
            cuda.boxes(),
            cuda.records(),
            boxes.size(),
            query_lower_bits[0],
            query_lower_bits[1],
            query_lower_bits[2],
            query_upper_bits[0],
            query_upper_bits[1],
            query_upper_bits[2],
            cutoff_lower_bits,
            cutoff_upper_bits);
    check_cuda(
        cudaGetLastError(), "Phase 4 spatial-bounds proposal launch");

    SpatialBoundsProposalBatch batch;
    batch.records.resize(boxes.size());
    check_cuda(
        cudaMemcpyAsync(
            batch.records.data(),
            cuda.records(),
            boxes.size() * sizeof(SpatialBoundsProposalRecord),
            cudaMemcpyDeviceToHost,
            cuda.stream()),
        "cudaMemcpyAsync Phase 4 spatial-bounds records device-to-host");
    cuda.synchronize();
    batch.buffer_epoch = context.advance_epoch();
    device_guard.restore();
    return batch;
  } catch (...) {
    cuda.synchronize_after_failure();
    throw;
  }
}

SpatialLbvhCoverBatch propose_strict_lbvh_cover_on_gpu(
    SpatialLbvhContextState& context,
    std::span<const SpatialLbvhNodeInputRecord> nodes,
    std::size_t root_index,
    const std::array<std::uint64_t, 3>& query_lower_bits,
    const std::array<std::uint64_t, 3>& query_upper_bits,
    std::uint64_t cutoff_lower_bits,
    std::uint64_t cutoff_upper_bits) {
  if (nodes.empty() || root_index >= nodes.size()) {
    throw std::invalid_argument(
        "a Phase 4 spatial-LBVH proposal requires a valid nonempty tree");
  }
  if (!std::in_range<std::uint64_t>(root_index)) {
    throw std::length_error(
        "the Phase 4 spatial-LBVH root index is not representable");
  }

  SpatialLbvhCudaResources& cuda = lbvh_resources(context);
  DeviceGuard device_guard{cuda.device()};
  try {
    cuda.initialize_nodes(nodes);
    check_cuda(
        cudaMemsetAsync(
            cuda.records(),
            0xff,
            nodes.size() * sizeof(SpatialLbvhCoverRecord),
            cuda.stream()),
        "cudaMemsetAsync Phase 4 spatial-LBVH cover sentinels");
    check_cuda(
        cudaMemsetAsync(
            cuda.record_count(),
            0,
            sizeof(std::uint64_t),
            cuda.stream()),
        "cudaMemsetAsync Phase 4 spatial-LBVH cover count");
    check_cuda(
        cudaMemsetAsync(
            cuda.failure_code(),
            0,
            sizeof(std::uint64_t),
            cuda.stream()),
        "cudaMemsetAsync Phase 4 spatial-LBVH failure code");

    const std::uint64_t root_word = static_cast<std::uint64_t>(root_index);
    check_cuda(
        cudaMemcpyAsync(
            cuda.current_frontier(),
            &root_word,
            sizeof(root_word),
            cudaMemcpyHostToDevice,
            cuda.stream()),
        "cudaMemcpyAsync Phase 4 spatial-LBVH root host-to-device");

    SpatialLbvhCoverBatch batch;
    std::uint64_t* current_frontier = cuda.current_frontier();
    std::uint64_t* next_frontier = cuda.next_frontier();
    std::size_t current_frontier_count = 1U;
    while (current_frontier_count != 0U) {
      if (current_frontier_count > nodes.size() ||
          batch.traversal_round_count >= nodes.size() ||
          batch.processed_node_count >
              nodes.size() - current_frontier_count) {
        throw std::runtime_error(
            "the Phase 4 spatial-LBVH frontier exceeded its structural node bound");
      }
      ++batch.traversal_round_count;
      ++batch.kernel_launch_count;
      batch.processed_node_count += current_frontier_count;
      batch.peak_frontier_count =
          std::max(batch.peak_frontier_count, current_frontier_count);
      if (current_frontier_count > 1U) {
        ++batch.parallel_round_count;
      }

      check_cuda(
          cudaMemsetAsync(
              cuda.next_frontier_count(),
              0,
              sizeof(std::uint64_t),
              cuda.stream()),
          "cudaMemsetAsync Phase 4 spatial-LBVH next-frontier count");
      const std::size_t required_blocks =
          (current_frontier_count - 1U) / kThreadsPerBlock + 1U;
      const std::size_t bounded_blocks = std::min(
          required_blocks,
          static_cast<std::size_t>(cuda.maximum_grid_x()));
      if (bounded_blocks == 0U ||
          bounded_blocks > std::numeric_limits<unsigned int>::max()) {
        throw std::length_error(
            "the Phase 4 spatial-LBVH CUDA grid is not representable");
      }

      morsehgp3d_phase4_spatial_lbvh_frontier_kernel
          <<<static_cast<unsigned int>(bounded_blocks),
             kThreadsPerBlock,
             0U,
             cuda.stream()>>>(
              cuda.nodes(),
              current_frontier,
              next_frontier,
              cuda.records(),
              cuda.record_count(),
              cuda.next_frontier_count(),
              cuda.failure_code(),
              nodes.size(),
              current_frontier_count,
              query_lower_bits[0],
              query_lower_bits[1],
              query_lower_bits[2],
              query_upper_bits[0],
              query_upper_bits[1],
              query_upper_bits[2],
              cutoff_lower_bits,
              cutoff_upper_bits);
      check_cuda(
          cudaGetLastError(), "Phase 4 spatial-LBVH frontier launch");

      std::uint64_t next_count_word = 0U;
      std::uint64_t failure_code = 0U;
      check_cuda(
          cudaMemcpyAsync(
              &next_count_word,
              cuda.next_frontier_count(),
              sizeof(next_count_word),
              cudaMemcpyDeviceToHost,
              cuda.stream()),
          "cudaMemcpyAsync Phase 4 spatial-LBVH next-frontier count device-to-host");
      check_cuda(
          cudaMemcpyAsync(
              &failure_code,
              cuda.failure_code(),
              sizeof(failure_code),
              cudaMemcpyDeviceToHost,
              cuda.stream()),
          "cudaMemcpyAsync Phase 4 spatial-LBVH failure code device-to-host");
      cuda.synchronize();
      if (failure_code != 0U) {
        throw std::runtime_error(
            "the Phase 4 spatial-LBVH frontier kernel failed closed with code " +
            std::to_string(failure_code));
      }
      if (!std::in_range<std::size_t>(next_count_word) ||
          static_cast<std::size_t>(next_count_word) > nodes.size()) {
        throw std::runtime_error(
            "the Phase 4 spatial-LBVH next frontier exceeded node capacity");
      }
      current_frontier_count = static_cast<std::size_t>(next_count_word);
      std::swap(current_frontier, next_frontier);
    }

    batch.records.resize(nodes.size());
    std::uint64_t output_count = 0U;
    check_cuda(
        cudaMemcpyAsync(
            batch.records.data(),
            cuda.records(),
            nodes.size() * sizeof(SpatialLbvhCoverRecord),
            cudaMemcpyDeviceToHost,
            cuda.stream()),
        "cudaMemcpyAsync Phase 4 spatial-LBVH cover device-to-host");
    check_cuda(
        cudaMemcpyAsync(
            &output_count,
            cuda.record_count(),
            sizeof(output_count),
            cudaMemcpyDeviceToHost,
            cuda.stream()),
        "cudaMemcpyAsync Phase 4 spatial-LBVH cover count device-to-host");
    cuda.synchronize();
    if (!std::in_range<std::size_t>(output_count) ||
        static_cast<std::size_t>(output_count) > nodes.size()) {
      throw std::runtime_error(
          "the Phase 4 spatial-LBVH kernel returned an invalid cover size");
    }
    batch.record_count = static_cast<std::size_t>(output_count);
    batch.buffer_epoch = context.advance_epoch();
    device_guard.restore();
    return batch;
  } catch (...) {
    cuda.synchronize_after_failure();
    throw;
  }
}

}  // namespace morsehgp3d::gpu::detail
