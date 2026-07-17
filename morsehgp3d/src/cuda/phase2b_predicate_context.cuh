#pragma once

#include "phase2b_predicate_context_internal.hpp"

#include "morsehgp3d/gpu/predicate_filter.hpp"

#include <cuda_runtime.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#if !defined(__CUDACC__)
#error "phase2b_predicate_context.cuh is a CUDA host-runtime contract"
#endif

namespace morsehgp3d::gpu::detail {

class PredicateFilterCudaFailure final : public std::runtime_error {
 public:
  PredicateFilterCudaFailure(cudaError_t code, std::string operation)
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

inline void check_predicate_filter_cuda(
    cudaError_t code, std::string operation) {
  if (code != cudaSuccess) {
    throw PredicateFilterCudaFailure(code, std::move(operation));
  }
}

template <typename Value>
class PredicateFilterDeviceBuffer final {
 public:
  PredicateFilterDeviceBuffer() = default;
  PredicateFilterDeviceBuffer(const PredicateFilterDeviceBuffer&) = delete;
  PredicateFilterDeviceBuffer& operator=(
      const PredicateFilterDeviceBuffer&) = delete;

  ~PredicateFilterDeviceBuffer() { reset(); }

  void reserve(std::size_t required_count, const char* operation) {
    if (required_count <= capacity_) {
      return;
    }
    if (required_count >
        std::numeric_limits<std::size_t>::max() / sizeof(Value)) {
      throw std::length_error("Phase 2B device allocation size overflow");
    }

    Value* replacement = nullptr;
    check_predicate_filter_cuda(
        cudaMalloc(
            reinterpret_cast<void**>(&replacement),
            required_count * sizeof(Value)),
        operation);
    if (data_ != nullptr) {
      const cudaError_t release_status = cudaFree(data_);
      if (release_status != cudaSuccess) {
        static_cast<void>(cudaFree(replacement));
        throw PredicateFilterCudaFailure(
            release_status, "cudaFree retired Phase 2B predicate workspace");
      }
    }
    data_ = replacement;
    capacity_ = required_count;
  }

  void reset() noexcept {
    if (data_ != nullptr) {
      static_cast<void>(cudaFree(data_));
      data_ = nullptr;
      capacity_ = 0U;
    }
  }

  // If the owning device can no longer be activated, leaking is safer than
  // issuing cudaFree against an unrelated current device during destruction.
  void abandon() noexcept {
    data_ = nullptr;
    capacity_ = 0U;
  }

  [[nodiscard]] Value* get() noexcept { return data_; }

 private:
  Value* data_{nullptr};
  std::size_t capacity_{0U};
};

struct PredicateFilterExecutionResources {
  cudaStream_t stream{nullptr};
  std::uint64_t* coordinate_bits{nullptr};
  std::uint32_t* cardinalities{nullptr};
  FilterSign* outputs{nullptr};
};

class PredicateFilterCudaDeviceGuard final {
 public:
  explicit PredicateFilterCudaDeviceGuard(int target_device) {
    check_predicate_filter_cuda(
        cudaGetDevice(&previous_device_),
        "cudaGetDevice before Phase 2B predicate execution");
    if (previous_device_ != target_device) {
      check_predicate_filter_cuda(
          cudaSetDevice(target_device),
          "cudaSetDevice for Phase 2B predicate context");
      restore_required_ = true;
    }
  }

  PredicateFilterCudaDeviceGuard(const PredicateFilterCudaDeviceGuard&) =
      delete;
  PredicateFilterCudaDeviceGuard& operator=(
      const PredicateFilterCudaDeviceGuard&) = delete;

  ~PredicateFilterCudaDeviceGuard() { restore_noexcept(); }

  void restore() {
    if (!restore_required_) {
      return;
    }
    check_predicate_filter_cuda(
        cudaSetDevice(previous_device_),
        "cudaSetDevice restore after Phase 2B predicate execution");
    restore_required_ = false;
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

class PredicateFilterCudaResources final {
 public:
  PredicateFilterCudaResources() {
    check_predicate_filter_cuda(
        cudaGetDevice(&device_),
        "cudaGetDevice for Phase 2B predicate context creation");
    check_predicate_filter_cuda(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags");
  }

  PredicateFilterCudaResources(const PredicateFilterCudaResources&) = delete;
  PredicateFilterCudaResources& operator=(
      const PredicateFilterCudaResources&) = delete;

  ~PredicateFilterCudaResources() {
    int previous_device = 0;
    const cudaError_t query_status = cudaGetDevice(&previous_device);
    bool restore_device = false;
    if (query_status == cudaSuccess && previous_device != device_) {
      restore_device = cudaSetDevice(device_) == cudaSuccess;
    }
    if (query_status != cudaSuccess ||
        (previous_device != device_ && !restore_device)) {
      outputs_.abandon();
      cardinalities_.abandon();
      coordinate_bits_.abandon();
      stream_ = nullptr;
      return;
    }
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
    outputs_.reset();
    cardinalities_.reset();
    coordinate_bits_.reset();
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamDestroy(stream_));
      stream_ = nullptr;
    }
    if (restore_device) {
      static_cast<void>(cudaSetDevice(previous_device));
    }
  }

  void reserve(
      std::size_t coordinate_word_count,
      std::size_t cardinality_count,
      std::size_t output_count) {
    coordinate_bits_.reserve(
        coordinate_word_count, "cudaMalloc Phase 2B coordinate workspace");
    cardinalities_.reserve(
        cardinality_count, "cudaMalloc Phase 2B cardinality workspace");
    outputs_.reserve(output_count, "cudaMalloc Phase 2B output workspace");
  }

  [[nodiscard]] PredicateFilterExecutionResources execution_resources()
      noexcept {
    return PredicateFilterExecutionResources{
        stream_, coordinate_bits_.get(), cardinalities_.get(), outputs_.get()};
  }

  [[nodiscard]] int device() const noexcept { return device_; }

  void synchronize() {
    check_predicate_filter_cuda(
        cudaStreamSynchronize(stream_),
        "cudaStreamSynchronize Phase 2B predicate context");
  }

  void synchronize_after_failure() noexcept {
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
  }

 private:
  int device_{-1};
  cudaStream_t stream_{nullptr};
  PredicateFilterDeviceBuffer<std::uint64_t> coordinate_bits_;
  PredicateFilterDeviceBuffer<std::uint32_t> cardinalities_;
  PredicateFilterDeviceBuffer<FilterSign> outputs_;
};

[[nodiscard]] inline PredicateFilterCudaResources&
predicate_filter_cuda_resources(PredicateFilterContextState& state) {
  std::shared_ptr<void>& opaque = state.cuda_resources();
  if (!opaque) {
    opaque = std::make_shared<PredicateFilterCudaResources>();
  }
  return *static_cast<PredicateFilterCudaResources*>(opaque.get());
}

template <typename Operation>
void execute_predicate_filter_gpu_section(
    PredicateFilterContextState& state,
    std::size_t coordinate_word_count,
    std::size_t cardinality_count,
    std::size_t output_count,
    Operation&& operation) {
  state.with_gpu_section([&] {
    PredicateFilterCudaResources& cuda =
        predicate_filter_cuda_resources(state);
    PredicateFilterCudaDeviceGuard device_guard{cuda.device()};
    try {
      cuda.reserve(
          coordinate_word_count, cardinality_count, output_count);
      std::forward<Operation>(operation)(cuda.execution_resources());
      cuda.synchronize();
    } catch (...) {
      cuda.synchronize_after_failure();
      throw;
    }
    device_guard.restore();
  });
}

}  // namespace morsehgp3d::gpu::detail
