#include "phase9_pair_support_phi_internal.hpp"

#include "phase2b_interval.cuh"

#include <cuda_runtime.h>

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
#error "phase9_pair_support_phi.cu must be compiled ahead of time with NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Phase 9 pair-support phi proposal requires CUDA 12.9"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the Phase 9 pair-support phi proposal"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the Phase 9 pair-support phi proposal"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Phase 9 pair-support phi proposal must contain only sm_120 device code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

constexpr unsigned int kThreadsPerBlock = 256U;
constexpr std::size_t kAxisCount = 3U;
constexpr std::uint64_t kPositiveInfinityBits =
    UINT64_C(0x7ff0000000000000);

class PairSupportPhiCudaFailure final : public std::runtime_error {
 public:
  PairSupportPhiCudaFailure(cudaError_t code, std::string operation)
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
    throw PairSupportPhiCudaFailure(code, std::move(operation));
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
    if (data_ != nullptr || count == 0U ||
        count > std::numeric_limits<std::size_t>::max() / sizeof(Value)) {
      throw std::length_error(
          "a Phase 9 pair-support phi device allocation size is invalid");
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
  std::size_t count_{};
};

class DeviceGuard final {
 public:
  explicit DeviceGuard(int target_device) {
    check_cuda(
        cudaGetDevice(&previous_device_),
        "cudaGetDevice before Phase 9 pair-support phi proposal");
    if (previous_device_ != target_device) {
      check_cuda(
          cudaSetDevice(target_device),
          "cudaSetDevice for Phase 9 pair-support phi context");
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
          "cudaSetDevice restore after Phase 9 pair-support phi proposal");
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

class PairSupportPhiCudaResources final {
 public:
  PairSupportPhiCudaResources() {
    check_cuda(
        cudaGetDevice(&device_),
        "cudaGetDevice for Phase 9 pair-support phi context creation");
    cudaDeviceProp properties{};
    check_cuda(
        cudaGetDeviceProperties(&properties, device_),
        "cudaGetDeviceProperties for Phase 9 pair-support phi context");
    if (properties.maxGridSize[0] <= 0) {
      throw std::runtime_error(
          "the CUDA device exposes no one-dimensional Phase 9 grid");
    }
    maximum_grid_x_ = static_cast<unsigned int>(properties.maxGridSize[0]);
    check_cuda(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags for Phase 9 pair-support phi context");
  }

  PairSupportPhiCudaResources(const PairSupportPhiCudaResources&) = delete;
  PairSupportPhiCudaResources& operator=(
      const PairSupportPhiCudaResources&) = delete;

  ~PairSupportPhiCudaResources() {
    int previous_device = 0;
    const cudaError_t query_status = cudaGetDevice(&previous_device);
    bool restore_device = false;
    if (query_status == cudaSuccess && previous_device != device_) {
      restore_device = cudaSetDevice(device_) == cudaSuccess;
    }
    if (query_status != cudaSuccess ||
        (previous_device != device_ && !restore_device)) {
      records_.abandon();
      queries_.abandon();
      nodes_.abandon();
      stream_ = nullptr;
      return;
    }
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
    records_.reset();
    queries_.reset();
    nodes_.reset();
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamDestroy(stream_));
      stream_ = nullptr;
    }
    if (restore_device) {
      static_cast<void>(cudaSetDevice(previous_device));
    }
  }

  void initialize(
      std::span<const PairSupportPhiNodeInputRecord> nodes,
      std::size_t maximum_query_count) {
    if (node_count_ != 0U) {
      if (node_count_ != nodes.size() || nodes_.count() != nodes.size() ||
          maximum_query_count_ != maximum_query_count ||
          queries_.count() != maximum_query_count ||
          records_.count() != maximum_query_count) {
        throw std::logic_error(
            "a Phase 9 pair-support phi context changed its resident snapshot");
      }
      return;
    }
    if (nodes.empty() || maximum_query_count == 0U) {
      throw std::invalid_argument(
          "a Phase 9 pair-support phi resident snapshot is empty");
    }
    nodes_.allocate(nodes.size(), "cudaMalloc Phase 9 pair-support LBVH nodes");
    queries_.allocate(
        maximum_query_count,
        "cudaMalloc Phase 9 pair-support phi query buffer");
    records_.allocate(
        maximum_query_count,
        "cudaMalloc Phase 9 pair-support phi proposal buffer");
    check_cuda(
        cudaMemcpyAsync(
            nodes_.get(),
            nodes.data(),
            nodes.size() * sizeof(PairSupportPhiNodeInputRecord),
            cudaMemcpyHostToDevice,
            stream_),
        "cudaMemcpyAsync Phase 9 pair-support LBVH snapshot host-to-device");
    synchronize();
    node_count_ = nodes.size();
    maximum_query_count_ = maximum_query_count;
  }

  [[nodiscard]] int device() const noexcept { return device_; }
  [[nodiscard]] cudaStream_t stream() const noexcept { return stream_; }
  [[nodiscard]] unsigned int maximum_grid_x() const noexcept {
    return maximum_grid_x_;
  }
  [[nodiscard]] const PairSupportPhiNodeInputRecord* nodes() const noexcept {
    return nodes_.get();
  }
  [[nodiscard]] PairSupportPhiQueryInputRecord* queries() noexcept {
    return queries_.get();
  }
  [[nodiscard]] PairSupportPhiDeviceRecord* records() noexcept {
    return records_.get();
  }

  void synchronize() {
    check_cuda(
        cudaStreamSynchronize(stream_),
        "cudaStreamSynchronize Phase 9 pair-support phi context");
  }

  void synchronize_after_failure() noexcept {
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
  }

 private:
  int device_{-1};
  cudaStream_t stream_{nullptr};
  unsigned int maximum_grid_x_{};
  std::size_t node_count_{};
  std::size_t maximum_query_count_{};
  DeviceBuffer<PairSupportPhiNodeInputRecord> nodes_;
  DeviceBuffer<PairSupportPhiQueryInputRecord> queries_;
  DeviceBuffer<PairSupportPhiDeviceRecord> records_;
};

[[nodiscard]] PairSupportPhiCudaResources& resources(
    PairSupportPhiContextState& state) {
  std::shared_ptr<void>& opaque = state.cuda_resources();
  if (!opaque) {
    opaque = std::make_shared<PairSupportPhiCudaResources>();
  }
  return *static_cast<PairSupportPhiCudaResources*>(opaque.get());
}

[[nodiscard]] inline __device__ std::uint64_t canonical_bits(
    double value) noexcept {
  const double canonical = value == 0.0 ? 0.0 : value;
  return static_cast<std::uint64_t>(__double_as_longlong(canonical));
}

[[nodiscard]] inline __device__ bool phi_upper_bound(
    const PairSupportPhiNodeInputRecord& first,
    const PairSupportPhiNodeInputRecord& second,
    const PairSupportPhiNodeInputRecord& witness,
    double& upper) noexcept {
  using device::DeviceInterval;
  DeviceInterval sum = device::point_interval(UINT64_C(0));
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const std::uint64_t first_bits[2]{
        first.lower_bits[axis], first.upper_bits[axis]};
    const std::uint64_t second_bits[2]{
        second.lower_bits[axis], second.upper_bits[axis]};
    const std::uint64_t witness_bits[2]{
        witness.lower_bits[axis], witness.upper_bits[axis]};
    bool initialized = false;
    double axis_upper = 0.0;
    for (std::size_t witness_selector = 0U;
         witness_selector < 2U;
         ++witness_selector) {
      for (std::size_t first_selector = 0U;
           first_selector < 2U;
           ++first_selector) {
        for (std::size_t second_selector = 0U;
             second_selector < 2U;
             ++second_selector) {
          const DeviceInterval x =
              device::point_interval(witness_bits[witness_selector]);
          const DeviceInterval u =
              device::point_interval(first_bits[first_selector]);
          const DeviceInterval v =
              device::point_interval(second_bits[second_selector]);
          const DeviceInterval candidate = device::multiply_intervals(
              device::subtract_intervals(x, u),
              device::subtract_intervals(x, v));
          if (!candidate.valid) {
            return false;
          }
          if (!initialized || candidate.upper > axis_upper) {
            initialized = true;
            axis_upper = candidate.upper;
          }
        }
      }
    }
    if (!initialized) {
      return false;
    }
    const DeviceInterval axis_interval =
        device::checked_interval(axis_upper, axis_upper);
    sum = device::add_intervals(sum, axis_interval);
    if (!sum.valid) {
      return false;
    }
  }
  upper = sum.upper;
  return device::is_finite(upper);
}

__global__ void morsehgp3d_phase9_pair_support_phi_kernel(
    const PairSupportPhiNodeInputRecord* nodes,
    std::size_t node_count,
    const PairSupportPhiQueryInputRecord* queries,
    std::size_t query_count,
    PairSupportPhiDeviceRecord* records) {
  const std::size_t first_index =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  const std::size_t stride =
      static_cast<std::size_t>(gridDim.x) * blockDim.x;
  std::size_t query_index = first_index;
  while (query_index < query_count) {
    const PairSupportPhiQueryInputRecord query = queries[query_index];
    PairSupportPhiDeviceRecord record{};
    record.query_index = static_cast<std::uint64_t>(query_index);
    record.first_support_node_index = query.first_support_node_index;
    record.second_support_node_index = query.second_support_node_index;
    record.witness_node_index = query.witness_node_index;
    record.upper_phi_bits = kPositiveInfinityBits;
    record.proposal_code = pair_support_phi_invalid_code;
    if (query.first_support_node_index < node_count &&
        query.second_support_node_index < node_count &&
        query.witness_node_index < node_count) {
      double upper = 0.0;
      const bool valid = phi_upper_bound(
          nodes[query.first_support_node_index],
          nodes[query.second_support_node_index],
          nodes[query.witness_node_index],
          upper);
      if (valid) {
        record.upper_phi_bits = canonical_bits(upper);
        record.proposal_code = upper < 0.0
            ? pair_support_phi_strict_interior_code
            : pair_support_phi_requires_descent_code;
      } else {
        record.proposal_code = pair_support_phi_requires_descent_code;
      }
    }
    records[query_index] = record;
    if (query_count - query_index <= stride) {
      break;
    }
    query_index += stride;
  }
}

}  // namespace

PairSupportPhiDeviceBatch propose_pair_support_phi_on_gpu(
    PairSupportPhiContextState& context,
    std::span<const PairSupportPhiNodeInputRecord> nodes,
    std::span<const PairSupportPhiQueryInputRecord> queries,
    std::size_t maximum_query_count) {
  if (nodes.empty() || queries.empty() || maximum_query_count == 0U ||
      queries.size() > maximum_query_count) {
    throw std::invalid_argument(
        "invalid Phase 9 pair-support phi launch extent");
  }
  PairSupportPhiCudaResources& cuda = resources(context);
  DeviceGuard guard{cuda.device()};
  try {
    cuda.initialize(nodes, maximum_query_count);
    check_cuda(
        cudaMemcpyAsync(
            cuda.queries(),
            queries.data(),
            queries.size() * sizeof(PairSupportPhiQueryInputRecord),
            cudaMemcpyHostToDevice,
            cuda.stream()),
        "cudaMemcpyAsync Phase 9 pair-support phi queries host-to-device");
    check_cuda(
        cudaMemsetAsync(
            cuda.records(),
            0xff,
            maximum_query_count * sizeof(PairSupportPhiDeviceRecord),
            cuda.stream()),
        "cudaMemsetAsync Phase 9 pair-support phi proposal sentinels");

    const std::size_t requested_blocks =
        (queries.size() + kThreadsPerBlock - 1U) / kThreadsPerBlock;
    const unsigned int blocks = static_cast<unsigned int>(std::min(
        requested_blocks,
        static_cast<std::size_t>(cuda.maximum_grid_x())));
    if (blocks == 0U) {
      throw std::runtime_error(
          "the Phase 9 pair-support phi launch grid is empty");
    }
    morsehgp3d_phase9_pair_support_phi_kernel
        <<<blocks, kThreadsPerBlock, 0U, cuda.stream()>>>(
            cuda.nodes(),
            nodes.size(),
            cuda.queries(),
            queries.size(),
            cuda.records());
    check_cuda(
        cudaPeekAtLastError(),
        "Phase 9 pair-support phi kernel launch");

    PairSupportPhiDeviceBatch batch;
    batch.records.resize(maximum_query_count);
    check_cuda(
        cudaMemcpyAsync(
            batch.records.data(),
            cuda.records(),
            maximum_query_count * sizeof(PairSupportPhiDeviceRecord),
            cudaMemcpyDeviceToHost,
            cuda.stream()),
        "cudaMemcpyAsync Phase 9 pair-support phi proposals device-to-host");
    cuda.synchronize();
    batch.record_count = queries.size();
    batch.kernel_launch_count = 1U;
    batch.buffer_epoch = context.advance_epoch();
    guard.restore();
    return batch;
  } catch (...) {
    cuda.synchronize_after_failure();
    throw;
  }
}

}  // namespace morsehgp3d::gpu::detail
