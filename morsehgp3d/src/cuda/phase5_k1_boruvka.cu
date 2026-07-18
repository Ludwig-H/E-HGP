#include "phase5_k1_boruvka_internal.hpp"

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
#include <type_traits>
#include <utility>
#include <vector>

#if !defined(__CUDACC__)
#error "phase5_k1_boruvka.cu must be compiled ahead of time with NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Phase 5 K1 Boruvka proposal requires the CUDA 12.9 compiler"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the Phase 5 K1 Boruvka proposal"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the Phase 5 K1 Boruvka proposal"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Phase 5 K1 Boruvka proposal must contain only sm_120 device code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

constexpr unsigned int kThreadsPerBlock = 256U;
constexpr std::size_t kAxisCount = 3U;
constexpr std::uint64_t kSignMask = UINT64_C(0x8000000000000000);
constexpr std::uint64_t kExponentMask = UINT64_C(0x7ff0000000000000);
constexpr std::uint64_t kPositiveInfinityBits =
    UINT64_C(0x7ff0000000000000);

enum class TraversalFailure : std::uint64_t {
  none = 0U,
  source_label_out_of_range = 1U,
  node_index_out_of_range = 2U,
  rope_cycle_or_revisit = 3U,
  component_tag_invalid = 4U,
  internal_node_malformed = 5U,
  leaf_node_malformed = 6U,
  leaf_component_tag_mismatch = 7U,
  escape_index_invalid = 8U,
  candidate_count_overflow = 9U,
  emit_offsets_invalid = 10U,
  emit_segment_overflow = 11U,
  count_emit_divergence = 12U,
};

struct K1BoruvkaCountRecord {
  std::uint64_t candidate_count{};
  std::uint64_t node_visit_count{};
  std::uint64_t uniform_component_prune_count{};
  std::uint64_t strict_aabb_prune_count{};
  std::uint64_t invalid_bound_descent_count{};
  std::uint64_t failure_code{};
};
static_assert(std::is_standard_layout_v<K1BoruvkaCountRecord>);
static_assert(std::is_trivially_copyable_v<K1BoruvkaCountRecord>);

struct K1BoruvkaEmitRecord {
  std::uint64_t emitted_count{};
  std::uint64_t node_visit_count{};
  std::uint64_t failure_code{};
};
static_assert(std::is_standard_layout_v<K1BoruvkaEmitRecord>);
static_assert(std::is_trivially_copyable_v<K1BoruvkaEmitRecord>);

class K1BoruvkaCudaFailure final : public std::runtime_error {
 public:
  K1BoruvkaCudaFailure(cudaError_t code, std::string operation)
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
    throw K1BoruvkaCudaFailure(code, std::move(operation));
  }
}

template <typename Value>
class DeviceBuffer final {
 public:
  DeviceBuffer() = default;
  DeviceBuffer(const DeviceBuffer&) = delete;
  DeviceBuffer& operator=(const DeviceBuffer&) = delete;
  ~DeviceBuffer() { reset(); }

  void reserve(std::size_t required_count, const char* operation) {
    if (required_count <= capacity_) {
      return;
    }
    if (required_count >
        std::numeric_limits<std::size_t>::max() / sizeof(Value)) {
      throw std::length_error(
          "Phase 5 K1 Boruvka device allocation size overflow");
    }

    Value* replacement = nullptr;
    check_cuda(
        cudaMalloc(
            reinterpret_cast<void**>(&replacement),
            required_count * sizeof(Value)),
        operation);
    if (data_ != nullptr) {
      const cudaError_t release_status = cudaFree(data_);
      if (release_status != cudaSuccess) {
        static_cast<void>(cudaFree(replacement));
        throw K1BoruvkaCudaFailure(
            release_status,
            "cudaFree retired Phase 5 K1 Boruvka workspace");
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

  // If the owning device can no longer be selected, leaking is safer than
  // freeing an allocation against an unrelated current device.
  void abandon() noexcept {
    data_ = nullptr;
    capacity_ = 0U;
  }

  [[nodiscard]] Value* get() noexcept { return data_; }
  [[nodiscard]] const Value* get() const noexcept { return data_; }
  [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

 private:
  Value* data_{nullptr};
  std::size_t capacity_{0U};
};

class DeviceGuard final {
 public:
  explicit DeviceGuard(int target_device) {
    check_cuda(
        cudaGetDevice(&previous_device_),
        "cudaGetDevice before Phase 5 K1 Boruvka proposal");
    if (previous_device_ != target_device) {
      check_cuda(
          cudaSetDevice(target_device),
          "cudaSetDevice for Phase 5 K1 Boruvka context");
      restore_required_ = true;
    }
  }

  DeviceGuard(const DeviceGuard&) = delete;
  DeviceGuard& operator=(const DeviceGuard&) = delete;
  ~DeviceGuard() { restore_noexcept(); }

  void restore() {
    if (!restore_required_) {
      return;
    }
    check_cuda(
        cudaSetDevice(previous_device_),
        "cudaSetDevice restore after Phase 5 K1 Boruvka proposal");
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

class K1BoruvkaCudaResources final {
 public:
  K1BoruvkaCudaResources() {
    check_cuda(
        cudaGetDevice(&device_),
        "cudaGetDevice for Phase 5 K1 Boruvka context creation");
    cudaDeviceProp properties{};
    check_cuda(
        cudaGetDeviceProperties(&properties, device_),
        "cudaGetDeviceProperties for Phase 5 K1 Boruvka context");
    if (properties.maxGridSize[0] <= 0) {
      throw std::runtime_error(
          "the CUDA device exposes no one-dimensional K1 Boruvka grid");
    }
    maximum_grid_x_ = static_cast<unsigned int>(properties.maxGridSize[0]);
    check_cuda(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags for Phase 5 K1 Boruvka context");
  }

  K1BoruvkaCudaResources(const K1BoruvkaCudaResources&) = delete;
  K1BoruvkaCudaResources& operator=(const K1BoruvkaCudaResources&) = delete;

  ~K1BoruvkaCudaResources() {
    int previous_device = 0;
    const cudaError_t query_status = cudaGetDevice(&previous_device);
    bool restore_device = false;
    if (query_status == cudaSuccess && previous_device != device_) {
      restore_device = cudaSetDevice(device_) == cudaSuccess;
    }
    if (query_status != cudaSuccess ||
        (previous_device != device_ && !restore_device)) {
      candidates_.abandon();
      emit_records_.abandon();
      count_records_.abandon();
      offsets_.abandon();
      seed_cutoffs_.abandon();
      node_tags_.abandon();
      frozen_labels_.abandon();
      coordinate_bits_.abandon();
      nodes_.abandon();
      stream_ = nullptr;
      return;
    }
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
    candidates_.reset();
    emit_records_.reset();
    count_records_.reset();
    offsets_.reset();
    seed_cutoffs_.reset();
    node_tags_.reset();
    frozen_labels_.reset();
    coordinate_bits_.reset();
    nodes_.reset();
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamDestroy(stream_));
      stream_ = nullptr;
    }
    if (restore_device) {
      static_cast<void>(cudaSetDevice(previous_device));
    }
  }

  void initialize_static_inputs(
      std::span<const K1BoruvkaNodeInputRecord> nodes,
      std::size_t root_index,
      std::span<const std::uint64_t> coordinate_bits,
      std::size_t point_count) {
    if (initialized_) {
      if (point_count_ != point_count || node_count_ != nodes.size() ||
          root_index_ != root_index ||
          coordinate_word_count_ != coordinate_bits.size()) {
        throw std::logic_error(
            "a Phase 5 K1 Boruvka context was reused with another static index");
      }
      return;
    }

    nodes_.reserve(nodes.size(), "cudaMalloc Phase 5 K1 Boruvka nodes");
    coordinate_bits_.reserve(
        coordinate_bits.size(),
        "cudaMalloc Phase 5 K1 Boruvka coordinate SoA");
    frozen_labels_.reserve(
        point_count, "cudaMalloc Phase 5 K1 Boruvka frozen labels");
    node_tags_.reserve(
        nodes.size(), "cudaMalloc Phase 5 K1 Boruvka node tags");
    seed_cutoffs_.reserve(
        point_count, "cudaMalloc Phase 5 K1 Boruvka seed cutoffs");
    offsets_.reserve(
        point_count + 1U, "cudaMalloc Phase 5 K1 Boruvka offsets");
    count_records_.reserve(
        point_count, "cudaMalloc Phase 5 K1 Boruvka count records");
    emit_records_.reserve(
        point_count, "cudaMalloc Phase 5 K1 Boruvka emit records");

    check_cuda(
        cudaMemcpyAsync(
            nodes_.get(),
            nodes.data(),
            nodes.size() * sizeof(K1BoruvkaNodeInputRecord),
            cudaMemcpyHostToDevice,
            stream_),
        "cudaMemcpyAsync Phase 5 K1 Boruvka nodes host-to-device");
    check_cuda(
        cudaMemcpyAsync(
            coordinate_bits_.get(),
            coordinate_bits.data(),
            coordinate_bits.size() * sizeof(std::uint64_t),
            cudaMemcpyHostToDevice,
            stream_),
        "cudaMemcpyAsync Phase 5 K1 Boruvka coordinates host-to-device");

    point_count_ = point_count;
    node_count_ = nodes.size();
    root_index_ = root_index;
    coordinate_word_count_ = coordinate_bits.size();
    initialized_ = true;
  }

  void update_round_inputs(
      std::span<const std::uint64_t> frozen_component_labels,
      std::span<const std::uint64_t> node_component_tags,
      std::span<const std::uint64_t> seed_cutoff_upper_bits) {
    check_cuda(
        cudaMemcpyAsync(
            frozen_labels_.get(),
            frozen_component_labels.data(),
            frozen_component_labels.size() * sizeof(std::uint64_t),
            cudaMemcpyHostToDevice,
            stream_),
        "cudaMemcpyAsync Phase 5 K1 Boruvka frozen labels host-to-device");
    check_cuda(
        cudaMemcpyAsync(
            node_tags_.get(),
            node_component_tags.data(),
            node_component_tags.size() * sizeof(std::uint64_t),
            cudaMemcpyHostToDevice,
            stream_),
        "cudaMemcpyAsync Phase 5 K1 Boruvka node tags host-to-device");
    check_cuda(
        cudaMemcpyAsync(
            seed_cutoffs_.get(),
            seed_cutoff_upper_bits.data(),
            seed_cutoff_upper_bits.size() * sizeof(std::uint64_t),
            cudaMemcpyHostToDevice,
            stream_),
        "cudaMemcpyAsync Phase 5 K1 Boruvka cutoffs host-to-device");
  }

  void reserve_candidates(std::size_t candidate_count) {
    candidates_.reserve(
        candidate_count, "cudaMalloc Phase 5 K1 Boruvka candidates");
  }

  [[nodiscard]] int device() const noexcept { return device_; }
  [[nodiscard]] cudaStream_t stream() const noexcept { return stream_; }
  [[nodiscard]] unsigned int maximum_grid_x() const noexcept {
    return maximum_grid_x_;
  }
  [[nodiscard]] const K1BoruvkaNodeInputRecord* nodes() const noexcept {
    return nodes_.get();
  }
  [[nodiscard]] const std::uint64_t* coordinate_bits() const noexcept {
    return coordinate_bits_.get();
  }
  [[nodiscard]] const std::uint64_t* frozen_labels() const noexcept {
    return frozen_labels_.get();
  }
  [[nodiscard]] const std::uint64_t* node_tags() const noexcept {
    return node_tags_.get();
  }
  [[nodiscard]] const std::uint64_t* seed_cutoffs() const noexcept {
    return seed_cutoffs_.get();
  }
  [[nodiscard]] std::uint64_t* offsets() noexcept { return offsets_.get(); }
  [[nodiscard]] K1BoruvkaCountRecord* count_records() noexcept {
    return count_records_.get();
  }
  [[nodiscard]] K1BoruvkaEmitRecord* emit_records() noexcept {
    return emit_records_.get();
  }
  [[nodiscard]] K1BoruvkaCandidateRecord* candidates() noexcept {
    return candidates_.get();
  }
  [[nodiscard]] std::size_t candidate_capacity() const noexcept {
    return candidates_.capacity();
  }

  void synchronize() {
    check_cuda(
        cudaStreamSynchronize(stream_),
        "cudaStreamSynchronize Phase 5 K1 Boruvka context");
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
  bool initialized_{false};
  std::size_t point_count_{};
  std::size_t node_count_{};
  std::size_t root_index_{};
  std::size_t coordinate_word_count_{};
  DeviceBuffer<K1BoruvkaNodeInputRecord> nodes_;
  DeviceBuffer<std::uint64_t> coordinate_bits_;
  DeviceBuffer<std::uint64_t> frozen_labels_;
  DeviceBuffer<std::uint64_t> node_tags_;
  DeviceBuffer<std::uint64_t> seed_cutoffs_;
  DeviceBuffer<std::uint64_t> offsets_;
  DeviceBuffer<K1BoruvkaCountRecord> count_records_;
  DeviceBuffer<K1BoruvkaEmitRecord> emit_records_;
  DeviceBuffer<K1BoruvkaCandidateRecord> candidates_;
};

[[nodiscard]] K1BoruvkaCudaResources& resources(
    K1BoruvkaCandidateContextState& state) {
  std::shared_ptr<void>& opaque = state.cuda_resources();
  if (!opaque) {
    opaque = std::make_shared<K1BoruvkaCudaResources>();
  }
  return *static_cast<K1BoruvkaCudaResources*>(opaque.get());
}

[[nodiscard]] __host__ __device__ bool finite_bits(
    std::uint64_t bits) noexcept {
  return (bits & kExponentMask) != kExponentMask;
}

[[nodiscard]] __device__ double value_from_bits(
    std::uint64_t bits) noexcept {
  return cuda::std::bit_cast<double>(bits);
}

[[nodiscard]] __device__ std::uint64_t bits_from_value(
    double value) noexcept {
  return cuda::std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] __device__ bool finite_value(double value) noexcept {
  return finite_bits(bits_from_value(value));
}

struct DirectedLowerBound {
  double value{};
  bool valid{true};
};

[[nodiscard]] __device__ DirectedLowerBound directed_aabb_lower_bound(
    const K1BoruvkaNodeInputRecord& node,
    const std::uint64_t* coordinate_bits,
    std::size_t point_count,
    std::size_t source_index) noexcept {
  DirectedLowerBound result;
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const std::uint64_t lower_bits = node.lower_bits[axis];
    const std::uint64_t upper_bits = node.upper_bits[axis];
    const std::uint64_t query_bits =
        coordinate_bits[axis * point_count + source_index];
    if (!finite_bits(lower_bits) || !finite_bits(upper_bits) ||
        !finite_bits(query_bits)) {
      result.valid = false;
      return result;
    }

    const double lower = value_from_bits(lower_bits);
    const double upper = value_from_bits(upper_bits);
    const double query = value_from_bits(query_bits);
    if (lower > upper) {
      result.valid = false;
      return result;
    }

    double delta = 0.0;
    if (query < lower) {
      delta = __dsub_rd(lower, query);
    } else if (query > upper) {
      delta = __dsub_rd(query, upper);
    }
    if (!finite_value(delta) || delta < 0.0) {
      result.valid = false;
      return result;
    }

    const double squared = __dmul_rd(delta, delta);
    const double next = __dadd_rd(result.value, squared);
    if (!finite_value(squared) || !finite_value(next) || squared < 0.0 ||
        next < 0.0) {
      result.value = next;
      result.valid = false;
      return result;
    }
    result.value = next;
  }
  return result;
}

[[nodiscard]] __device__ bool cutoff_is_valid(
    std::uint64_t cutoff_bits) noexcept {
  if (cutoff_bits == kPositiveInfinityBits) {
    return true;
  }
  return finite_bits(cutoff_bits) && (cutoff_bits & kSignMask) == 0U;
}

[[nodiscard]] __device__ bool valid_escape(
    std::uint64_t escape,
    std::size_t node_count,
    std::uint64_t current_node) noexcept {
  return escape == k1_boruvka_sentinel ||
         (escape < static_cast<std::uint64_t>(node_count) &&
          escape != current_node);
}

template <bool EmitCandidates>
[[nodiscard]] __device__ K1BoruvkaCountRecord traverse_source(
    const K1BoruvkaNodeInputRecord* nodes,
    std::size_t node_count,
    std::size_t root_index,
    const std::uint64_t* coordinate_bits,
    std::size_t point_count,
    const std::uint64_t* frozen_component_labels,
    const std::uint64_t* node_component_tags,
    const std::uint64_t* seed_cutoff_upper_bits,
    const std::uint64_t* candidate_offsets,
    std::size_t candidate_capacity,
    K1BoruvkaCandidateRecord* candidates,
    std::size_t source_index) noexcept {
  K1BoruvkaCountRecord output;
  const std::uint64_t source_label =
      frozen_component_labels[source_index];
  if (source_label >= static_cast<std::uint64_t>(point_count)) {
    output.failure_code = static_cast<std::uint64_t>(
        TraversalFailure::source_label_out_of_range);
    return output;
  }

  std::uint64_t segment_begin = 0U;
  std::uint64_t segment_end = 0U;
  if constexpr (EmitCandidates) {
    segment_begin = candidate_offsets[source_index];
    segment_end = candidate_offsets[source_index + 1U];
    if (segment_begin > segment_end ||
        segment_end > static_cast<std::uint64_t>(candidate_capacity)) {
      output.failure_code = static_cast<std::uint64_t>(
          TraversalFailure::emit_offsets_invalid);
      return output;
    }
  }

  const std::uint64_t cutoff_bits = seed_cutoff_upper_bits[source_index];
  const bool cutoff_valid = cutoff_is_valid(cutoff_bits);
  const bool finite_cutoff = finite_bits(cutoff_bits);
  const double cutoff =
      cutoff_valid ? value_from_bits(cutoff_bits) : 0.0;
  std::uint64_t node_index = static_cast<std::uint64_t>(root_index);
  while (node_index != k1_boruvka_sentinel) {
    if (output.node_visit_count >=
        static_cast<std::uint64_t>(node_count)) {
      output.failure_code = static_cast<std::uint64_t>(
          TraversalFailure::rope_cycle_or_revisit);
      break;
    }
    if (node_index >= static_cast<std::uint64_t>(node_count)) {
      output.failure_code = static_cast<std::uint64_t>(
          TraversalFailure::node_index_out_of_range);
      break;
    }
    ++output.node_visit_count;

    const K1BoruvkaNodeInputRecord& node = nodes[node_index];
    const std::uint64_t tag = node_component_tags[node_index];
    if (tag == k1_boruvka_sentinel ||
        (tag != k1_boruvka_mixed_component &&
         tag >= static_cast<std::uint64_t>(point_count))) {
      output.failure_code = static_cast<std::uint64_t>(
          TraversalFailure::component_tag_invalid);
      break;
    }
    if (!valid_escape(node.escape, node_count, node_index)) {
      output.failure_code = static_cast<std::uint64_t>(
          TraversalFailure::escape_index_invalid);
      break;
    }
    if (tag == source_label) {
      ++output.uniform_component_prune_count;
      node_index = node.escape;
      continue;
    }

    const DirectedLowerBound lower = directed_aabb_lower_bound(
        node, coordinate_bits, point_count, source_index);
    if (!lower.valid || !cutoff_valid) {
      ++output.invalid_bound_descent_count;
    } else if (finite_cutoff && lower.value > cutoff) {
      ++output.strict_aabb_prune_count;
      node_index = node.escape;
      continue;
    }

    const bool left_missing = node.left_child == k1_boruvka_sentinel;
    const bool right_missing = node.right_child == k1_boruvka_sentinel;
    if (left_missing != right_missing) {
      output.failure_code = static_cast<std::uint64_t>(
          TraversalFailure::internal_node_malformed);
      break;
    }

    if (!left_missing) {
      const bool children_valid =
          node.left_child < static_cast<std::uint64_t>(node_count) &&
          node.right_child < static_cast<std::uint64_t>(node_count) &&
          node.left_child != node.right_child &&
          node.left_child != node_index && node.right_child != node_index &&
          node.leaf_point_id == k1_boruvka_sentinel;
      if (!children_valid) {
        output.failure_code = static_cast<std::uint64_t>(
            TraversalFailure::internal_node_malformed);
        break;
      }
      node_index = node.left_child;
      continue;
    }

    if (node.leaf_point_id >= static_cast<std::uint64_t>(point_count)) {
      output.failure_code = static_cast<std::uint64_t>(
          TraversalFailure::leaf_node_malformed);
      break;
    }
    const std::size_t target_index =
        static_cast<std::size_t>(node.leaf_point_id);
    if (tag == k1_boruvka_mixed_component ||
        tag != frozen_component_labels[target_index] ||
        tag == source_label) {
      output.failure_code = static_cast<std::uint64_t>(
          TraversalFailure::leaf_component_tag_mismatch);
      break;
    }
    if (output.candidate_count == k1_boruvka_sentinel) {
      output.failure_code = static_cast<std::uint64_t>(
          TraversalFailure::candidate_count_overflow);
      break;
    }
    if constexpr (EmitCandidates) {
      const std::uint64_t segment_size = segment_end - segment_begin;
      if (output.candidate_count >= segment_size) {
        output.failure_code = static_cast<std::uint64_t>(
            TraversalFailure::emit_segment_overflow);
        break;
      }
      const std::uint64_t output_index =
          segment_begin + output.candidate_count;
      candidates[static_cast<std::size_t>(output_index)] =
          K1BoruvkaCandidateRecord{
              static_cast<std::uint64_t>(source_index),
              node.leaf_point_id};
    }
    ++output.candidate_count;
    node_index = node.escape;
  }

  if constexpr (EmitCandidates) {
    if (output.failure_code == 0U &&
        output.candidate_count != segment_end - segment_begin) {
      output.failure_code = static_cast<std::uint64_t>(
          TraversalFailure::count_emit_divergence);
    }
  }
  return output;
}

__global__ void morsehgp3d_phase5_k1_boruvka_count_kernel(
    const K1BoruvkaNodeInputRecord* nodes,
    std::size_t node_count,
    std::size_t root_index,
    const std::uint64_t* coordinate_bits,
    std::size_t point_count,
    const std::uint64_t* frozen_component_labels,
    const std::uint64_t* node_component_tags,
    const std::uint64_t* seed_cutoff_upper_bits,
    K1BoruvkaCountRecord* count_records) {
  const std::size_t first =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  const std::size_t stride =
      static_cast<std::size_t>(gridDim.x) * blockDim.x;
  std::size_t source_index = first;
  while (source_index < point_count) {
    count_records[source_index] = traverse_source<false>(
        nodes,
        node_count,
        root_index,
        coordinate_bits,
        point_count,
        frozen_component_labels,
        node_component_tags,
        seed_cutoff_upper_bits,
        nullptr,
        0U,
        nullptr,
        source_index);
    if (point_count - source_index <= stride) {
      break;
    }
    source_index += stride;
  }
}

__global__ void morsehgp3d_phase5_k1_boruvka_emit_kernel(
    const K1BoruvkaNodeInputRecord* nodes,
    std::size_t node_count,
    std::size_t root_index,
    const std::uint64_t* coordinate_bits,
    std::size_t point_count,
    const std::uint64_t* frozen_component_labels,
    const std::uint64_t* node_component_tags,
    const std::uint64_t* seed_cutoff_upper_bits,
    const std::uint64_t* candidate_offsets,
    std::size_t candidate_capacity,
    K1BoruvkaCandidateRecord* candidates,
    K1BoruvkaEmitRecord* emit_records) {
  const std::size_t first =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  const std::size_t stride =
      static_cast<std::size_t>(gridDim.x) * blockDim.x;
  std::size_t source_index = first;
  while (source_index < point_count) {
    const K1BoruvkaCountRecord traversal = traverse_source<true>(
        nodes,
        node_count,
        root_index,
        coordinate_bits,
        point_count,
        frozen_component_labels,
        node_component_tags,
        seed_cutoff_upper_bits,
        candidate_offsets,
        candidate_capacity,
        candidates,
        source_index);
    emit_records[source_index] = K1BoruvkaEmitRecord{
        traversal.candidate_count,
        traversal.node_visit_count,
        traversal.failure_code};
    if (point_count - source_index <= stride) {
      break;
    }
    source_index += stride;
  }
}

[[nodiscard]] std::size_t checked_add(
    std::size_t total,
    std::uint64_t increment,
    const char* description) {
  if (increment >
      static_cast<std::uint64_t>(
          std::numeric_limits<std::size_t>::max() - total)) {
    throw std::length_error(description);
  }
  return total + static_cast<std::size_t>(increment);
}

[[nodiscard]] unsigned int launch_block_count(
    std::size_t point_count,
    unsigned int maximum_grid_x) {
  const std::size_t required_blocks =
      (point_count - 1U) / kThreadsPerBlock + 1U;
  const std::size_t bounded_blocks = std::min(
      required_blocks, static_cast<std::size_t>(maximum_grid_x));
  if (bounded_blocks == 0U ||
      bounded_blocks > std::numeric_limits<unsigned int>::max()) {
    throw std::length_error(
        "the Phase 5 K1 Boruvka CUDA grid is not representable");
  }
  return static_cast<unsigned int>(bounded_blocks);
}

void validate_inputs(
    std::span<const K1BoruvkaNodeInputRecord> nodes,
    std::size_t root_index,
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::span<const std::uint64_t> frozen_component_labels,
    std::span<const std::uint64_t> node_component_tags,
    std::span<const std::uint64_t> seed_cutoff_upper_bits) {
  if (point_count == 0U || nodes.empty() || root_index >= nodes.size()) {
    throw std::invalid_argument(
        "a Phase 5 K1 Boruvka proposal requires a valid nonempty LBVH");
  }
  if (point_count > std::numeric_limits<std::uint64_t>::max() ||
      nodes.size() > std::numeric_limits<std::uint64_t>::max()) {
    throw std::length_error(
        "a Phase 5 K1 Boruvka size is not representable on the device");
  }
  if (point_count > std::numeric_limits<std::size_t>::max() / kAxisCount ||
      coordinate_bits.size() != point_count * kAxisCount ||
      frozen_component_labels.size() != point_count ||
      node_component_tags.size() != nodes.size() ||
      seed_cutoff_upper_bits.size() != point_count) {
    throw std::invalid_argument(
        "a Phase 5 K1 Boruvka input span has an inconsistent size");
  }
  if (point_count == std::numeric_limits<std::size_t>::max()) {
    throw std::length_error(
        "the Phase 5 K1 Boruvka offset count overflows size_t");
  }
  if (nodes[root_index].escape != k1_boruvka_sentinel) {
    throw std::invalid_argument(
        "the Phase 5 K1 Boruvka root escape must be the sentinel");
  }
  for (const std::uint64_t label : frozen_component_labels) {
    if (label >= static_cast<std::uint64_t>(point_count)) {
      throw std::invalid_argument(
          "a Phase 5 K1 Boruvka frozen component label is out of range");
    }
  }
  for (const std::uint64_t tag : node_component_tags) {
    if (tag == k1_boruvka_sentinel ||
        (tag != k1_boruvka_mixed_component &&
         tag >= static_cast<std::uint64_t>(point_count))) {
      throw std::invalid_argument(
          "a Phase 5 K1 Boruvka node component tag is invalid");
    }
  }
  for (const std::uint64_t bits : coordinate_bits) {
    if (!finite_bits(bits)) {
      throw std::invalid_argument(
          "Phase 5 K1 Boruvka coordinates must be finite binary64 values");
    }
  }
}

}  // namespace

K1BoruvkaCandidateBatch propose_k1_boruvka_candidates_on_gpu(
    K1BoruvkaCandidateContextState& context,
    std::span<const K1BoruvkaNodeInputRecord> nodes,
    std::size_t root_index,
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::span<const std::uint64_t> frozen_component_labels,
    std::span<const std::uint64_t> node_component_tags,
    std::span<const std::uint64_t> seed_cutoff_upper_bits) {
  validate_inputs(
      nodes,
      root_index,
      coordinate_bits,
      point_count,
      frozen_component_labels,
      node_component_tags,
      seed_cutoff_upper_bits);

  K1BoruvkaCudaResources& cuda = resources(context);
  DeviceGuard device_guard{cuda.device()};
  try {
    cuda.initialize_static_inputs(
        nodes, root_index, coordinate_bits, point_count);
    cuda.update_round_inputs(
        frozen_component_labels,
        node_component_tags,
        seed_cutoff_upper_bits);

    const unsigned int block_count =
        launch_block_count(point_count, cuda.maximum_grid_x());
    morsehgp3d_phase5_k1_boruvka_count_kernel
        <<<block_count, kThreadsPerBlock, 0U, cuda.stream()>>>(
            cuda.nodes(),
            nodes.size(),
            root_index,
            cuda.coordinate_bits(),
            point_count,
            cuda.frozen_labels(),
            cuda.node_tags(),
            cuda.seed_cutoffs(),
            cuda.count_records());
    check_cuda(
        cudaGetLastError(), "Phase 5 K1 Boruvka count-pass launch");

    std::vector<K1BoruvkaCountRecord> count_records(point_count);
    check_cuda(
        cudaMemcpyAsync(
            count_records.data(),
            cuda.count_records(),
            point_count * sizeof(K1BoruvkaCountRecord),
            cudaMemcpyDeviceToHost,
            cuda.stream()),
        "cudaMemcpyAsync Phase 5 K1 Boruvka counts device-to-host");
    cuda.synchronize();

    K1BoruvkaCandidateBatch batch;
    batch.kernel_launch_count = 2U;
    batch.synchronization_count = 2U;
    batch.candidate_offsets.resize(point_count + 1U, 0U);
    std::size_t candidate_count = 0U;
    for (std::size_t source_index = 0U;
         source_index < point_count;
         ++source_index) {
      const K1BoruvkaCountRecord& record = count_records[source_index];
      if (record.failure_code != 0U) {
        throw std::runtime_error(
            "the Phase 5 K1 Boruvka count pass failed closed for source " +
            std::to_string(source_index) + " with code " +
            std::to_string(record.failure_code));
      }
      if (record.node_visit_count > nodes.size() ||
          record.candidate_count > point_count) {
        throw std::runtime_error(
            "the Phase 5 K1 Boruvka count pass exceeded a structural bound");
      }
      candidate_count = checked_add(
          candidate_count,
          record.candidate_count,
          "the Phase 5 K1 Boruvka candidate prefix sum overflowed");
      if (candidate_count > std::numeric_limits<std::uint64_t>::max()) {
        throw std::length_error(
            "the Phase 5 K1 Boruvka candidate offset is not uint64-representable");
      }
      batch.candidate_offsets[source_index + 1U] =
          static_cast<std::uint64_t>(candidate_count);
      batch.count_pass_node_visit_count = checked_add(
          batch.count_pass_node_visit_count,
          record.node_visit_count,
          "the Phase 5 K1 Boruvka count-pass visit total overflowed");
      batch.uniform_component_prune_count = checked_add(
          batch.uniform_component_prune_count,
          record.uniform_component_prune_count,
          "the Phase 5 K1 Boruvka uniform-prune total overflowed");
      batch.strict_aabb_prune_count = checked_add(
          batch.strict_aabb_prune_count,
          record.strict_aabb_prune_count,
          "the Phase 5 K1 Boruvka AABB-prune total overflowed");
      batch.invalid_bound_descent_count = checked_add(
          batch.invalid_bound_descent_count,
          record.invalid_bound_descent_count,
          "the Phase 5 K1 Boruvka invalid-bound total overflowed");
    }

    cuda.reserve_candidates(candidate_count);
    check_cuda(
        cudaMemcpyAsync(
            cuda.offsets(),
            batch.candidate_offsets.data(),
            batch.candidate_offsets.size() * sizeof(std::uint64_t),
            cudaMemcpyHostToDevice,
            cuda.stream()),
        "cudaMemcpyAsync Phase 5 K1 Boruvka offsets host-to-device");

    morsehgp3d_phase5_k1_boruvka_emit_kernel
        <<<block_count, kThreadsPerBlock, 0U, cuda.stream()>>>(
            cuda.nodes(),
            nodes.size(),
            root_index,
            cuda.coordinate_bits(),
            point_count,
            cuda.frozen_labels(),
            cuda.node_tags(),
            cuda.seed_cutoffs(),
            cuda.offsets(),
            candidate_count,
            cuda.candidates(),
            cuda.emit_records());
    check_cuda(
        cudaGetLastError(), "Phase 5 K1 Boruvka emit-pass launch");

    batch.records.resize(candidate_count);
    std::vector<K1BoruvkaEmitRecord> emit_records(point_count);
    if (candidate_count != 0U) {
      check_cuda(
          cudaMemcpyAsync(
              batch.records.data(),
              cuda.candidates(),
              candidate_count * sizeof(K1BoruvkaCandidateRecord),
              cudaMemcpyDeviceToHost,
              cuda.stream()),
          "cudaMemcpyAsync Phase 5 K1 Boruvka candidates device-to-host");
    }
    check_cuda(
        cudaMemcpyAsync(
            emit_records.data(),
            cuda.emit_records(),
            point_count * sizeof(K1BoruvkaEmitRecord),
            cudaMemcpyDeviceToHost,
            cuda.stream()),
        "cudaMemcpyAsync Phase 5 K1 Boruvka emit records device-to-host");
    cuda.synchronize();

    for (std::size_t source_index = 0U;
         source_index < point_count;
         ++source_index) {
      const K1BoruvkaEmitRecord& emitted = emit_records[source_index];
      const K1BoruvkaCountRecord& counted = count_records[source_index];
      if (emitted.failure_code != 0U) {
        throw std::runtime_error(
            "the Phase 5 K1 Boruvka emit pass failed closed for source " +
            std::to_string(source_index) + " with code " +
            std::to_string(emitted.failure_code));
      }
      if (emitted.emitted_count != counted.candidate_count ||
          emitted.node_visit_count != counted.node_visit_count) {
        throw std::runtime_error(
            "the Phase 5 K1 Boruvka count and emit traversals diverged");
      }
      batch.emit_pass_node_visit_count = checked_add(
          batch.emit_pass_node_visit_count,
          emitted.node_visit_count,
          "the Phase 5 K1 Boruvka emit-pass visit total overflowed");
    }

    // output_capacity is the exact logical prefix initialized for this round.
    // The private resident allocation may retain a larger high-water mark so a
    // later smaller round does not force a needless free/reallocation cycle.
    batch.output_capacity = candidate_count;
    batch.exact_capacity =
        batch.records.size() == candidate_count &&
        cuda.candidate_capacity() >= candidate_count;
    batch.no_truncation = true;
    batch.buffer_epoch = context.advance_epoch();
    device_guard.restore();
    return batch;
  } catch (...) {
    cuda.synchronize_after_failure();
    throw;
  }
}

}  // namespace morsehgp3d::gpu::detail
