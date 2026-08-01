#include "phase15_exact_higher_support_product_cuda_internal.hpp"

#include "phase15_exact_higher_support_product_fixed.cuh"
#include "phase15_exact_higher_support_product_fixed512.cuh"

#include "morsehgp3d/spatial/lbvh.hpp"

#include <cuda_runtime.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#if !defined(__CUDACC__)
#error "phase15_exact_higher_support_product_cuda.cu requires NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The exact higher-support product kernel requires CUDA 12.9"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for exact higher-support product decisions"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the exact higher-support target"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The exact higher-support product target must contain only sm_120 code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

namespace fixed = exact_higher_support_product_fixed;
namespace fixed512 = exact_higher_support_product_fixed512;

inline constexpr unsigned int threads_per_block = 64U;
inline constexpr std::size_t axis_count =
    phase15_exact_higher_support_product_axis_count;
inline constexpr std::size_t maximum_support_size =
    phase15_exact_higher_support_product_maximum_support_size;

struct DeviceNode {
  std::uint64_t lower_point_ids[axis_count];
  std::uint64_t upper_point_ids[axis_count];
  std::uint64_t left_child;
  std::uint64_t right_child;
  std::uint64_t leaf_begin;
  std::uint64_t leaf_end;
};

static_assert(
    std::is_standard_layout_v<DeviceNode> &&
    std::is_trivially_copyable_v<DeviceNode> &&
    sizeof(DeviceNode) == sizeof(spatial::MortonLbvhSnapshotNode) &&
    alignof(DeviceNode) == alignof(spatial::MortonLbvhSnapshotNode));
static_assert(
    offsetof(DeviceNode, lower_point_ids) ==
        offsetof(spatial::MortonLbvhSnapshotNode, lower_point_ids) &&
    offsetof(DeviceNode, upper_point_ids) ==
        offsetof(spatial::MortonLbvhSnapshotNode, upper_point_ids) &&
    offsetof(DeviceNode, left_child) ==
        offsetof(spatial::MortonLbvhSnapshotNode, left_child) &&
    offsetof(DeviceNode, right_child) ==
        offsetof(spatial::MortonLbvhSnapshotNode, right_child) &&
    offsetof(DeviceNode, leaf_begin) ==
        offsetof(spatial::MortonLbvhSnapshotNode, leaf_begin) &&
    offsetof(DeviceNode, leaf_end) ==
        offsetof(spatial::MortonLbvhSnapshotNode, leaf_end));

class CudaFailure final : public std::runtime_error {
 public:
  CudaFailure(cudaError_t code, const char* operation)
      : std::runtime_error(message(code, operation)) {}

 private:
  [[nodiscard]] static std::string message(
      cudaError_t code,
      const char* operation) {
    const char* name = cudaGetErrorName(code);
    const char* description = cudaGetErrorString(code);
    return std::string{operation} + ": " +
        (name == nullptr ? "unknown CUDA error" : name) + " (" +
        (description == nullptr ? "no description" : description) + ")";
  }
};

void check_cuda(cudaError_t code, const char* operation) {
  if (code != cudaSuccess) {
    throw CudaFailure(code, operation);
  }
}

class DeviceGuard final {
 public:
  explicit DeviceGuard(int target) {
    check_cuda(
        cudaGetDevice(&previous_),
        "cudaGetDevice before exact higher-support product batch");
    if (previous_ != target) {
      check_cuda(
          cudaSetDevice(target),
          "cudaSetDevice for exact higher-support product batch");
      restore_ = true;
    }
  }
  DeviceGuard(const DeviceGuard&) = delete;
  DeviceGuard& operator=(const DeviceGuard&) = delete;
  ~DeviceGuard() {
    if (restore_) {
      static_cast<void>(cudaSetDevice(previous_));
    }
  }
  void restore() {
    if (restore_) {
      check_cuda(
          cudaSetDevice(previous_),
          "cudaSetDevice restore after exact higher-support product batch");
      restore_ = false;
    }
  }

 private:
  int previous_{};
  bool restore_{false};
};

class Stream final {
 public:
  Stream() {
    check_cuda(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags exact higher-support product batch");
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
  cudaStream_t stream_{};
};

class Event final {
 public:
  Event() {
    check_cuda(
        cudaEventCreateWithFlags(&event_, cudaEventDefault),
        "cudaEventCreate exact higher-support product kernel timer");
  }
  Event(const Event&) = delete;
  Event& operator=(const Event&) = delete;
  ~Event() {
    if (event_ != nullptr) {
      static_cast<void>(cudaEventDestroy(event_));
    }
  }
  [[nodiscard]] cudaEvent_t get() const noexcept { return event_; }

 private:
  cudaEvent_t event_{};
};

class BatchArena final {
 public:
  explicit BatchArena(std::size_t byte_count) {
    void* allocation = nullptr;
    check_cuda(
        cudaMalloc(&allocation, byte_count),
        "cudaMalloc exact higher-support product batch arena");
    pointer_ = static_cast<std::byte*>(allocation);
  }
  BatchArena(const BatchArena&) = delete;
  BatchArena& operator=(const BatchArena&) = delete;
  ~BatchArena() {
    if (pointer_ != nullptr) {
      static_cast<void>(cudaFree(pointer_));
    }
  }
  [[nodiscard]] std::byte* get() noexcept { return pointer_; }

 private:
  std::byte* pointer_{};
};

[[nodiscard]] bool checked_sum(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  result = left + right;
  return true;
}

[[nodiscard]] bool checked_align_up(
    std::size_t value,
    std::size_t alignment,
    std::size_t& result) noexcept {
  if (alignment == 0U) {
    return false;
  }
  const std::size_t remainder = value % alignment;
  return remainder == 0U
      ? (result = value, true)
      : checked_sum(value, alignment - remainder, result);
}

[[nodiscard]] __device__ bool ranges_overlap(
    std::uint64_t first_begin,
    std::uint64_t first_end,
    std::uint64_t second_begin,
    std::uint64_t second_end) noexcept {
  return first_begin < second_end && second_begin < first_end;
}

[[nodiscard]] __device__ bool native_node_matches(
    const DeviceNode* nodes,
    std::size_t node_count,
    std::uint64_t node_index,
    std::uint64_t leaf_begin,
    std::uint64_t leaf_end,
    std::size_t point_count) noexcept {
  if (node_index >= node_count || leaf_begin >= leaf_end ||
      leaf_end > point_count) {
    return false;
  }
  const DeviceNode& node = nodes[node_index];
  return node.leaf_begin == leaf_begin && node.leaf_end == leaf_end;
}

[[nodiscard]] __device__ bool load_native_box(
    const DeviceNode& node,
    const std::uint64_t* coordinate_bits,
    std::size_t point_count,
    fixed::Binary64Aabb3& box) noexcept {
  for (std::size_t axis = 0U; axis < axis_count; ++axis) {
    const std::uint64_t lower_id = node.lower_point_ids[axis];
    const std::uint64_t upper_id = node.upper_point_ids[axis];
    if (lower_id >= point_count || upper_id >= point_count) {
      return false;
    }
    box.lower[axis] = coordinate_bits[axis * point_count + lower_id];
    box.upper[axis] = coordinate_bits[axis * point_count + upper_id];
    if (!fixed::binary64_interval_ordered(
            box.lower[axis], box.upper[axis])) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] __device__ bool matches_proposed_box(
    const fixed::Binary64Aabb3& native,
    const Phase15ExactHigherSupportProductCudaRawAabb3& proposed) noexcept {
  for (std::size_t axis = 0U; axis < axis_count; ++axis) {
    if (fixed::canonical_binary64_word(native.lower[axis]) !=
            fixed::canonical_binary64_word(proposed.lower[axis]) ||
        fixed::canonical_binary64_word(native.upper[axis]) !=
            fixed::canonical_binary64_word(proposed.upper[axis])) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] __device__ Phase15ExactHigherSupportProductCudaDeviceOutcome
fixed_outcome(fixed::Decision decision) noexcept {
  switch (decision) {
    case fixed::Decision::certified:
      return Phase15ExactHigherSupportProductCudaDeviceOutcome::certified;
    case fixed::Decision::exact_false:
      return Phase15ExactHigherSupportProductCudaDeviceOutcome::exact_false;
    case fixed::Decision::requires_cpu_rational_fallback:
      return Phase15ExactHigherSupportProductCudaDeviceOutcome::
          requires_cpu_rational_fallback;
  }
  return Phase15ExactHigherSupportProductCudaDeviceOutcome::
      requires_cpu_rational_fallback;
}

[[nodiscard]] __device__ Phase15ExactHigherSupportProductCudaDeviceOutcome
fixed512_outcome(fixed512::Decision decision) noexcept {
  switch (decision) {
    case fixed512::Decision::certified:
      return Phase15ExactHigherSupportProductCudaDeviceOutcome::certified;
    case fixed512::Decision::exact_false:
      return Phase15ExactHigherSupportProductCudaDeviceOutcome::exact_false;
    case fixed512::Decision::requires_cpu_rational_fallback:
      return Phase15ExactHigherSupportProductCudaDeviceOutcome::
          requires_cpu_rational_fallback;
  }
  return Phase15ExactHigherSupportProductCudaDeviceOutcome::
      requires_cpu_rational_fallback;
}

[[nodiscard]] __device__ __noinline__ fixed512::Decision
evaluate_fixed512(
    ExactHigherSupportProductCudaTaskKind kind,
    const fixed::Binary64Aabb3 support_boxes[maximum_support_size],
    std::size_t support_size,
    const fixed::Binary64Aabb3& query_box) noexcept {
  return kind == ExactHigherSupportProductCudaTaskKind::support_prune
      ? fixed512::no_well_centered_support(support_boxes, support_size)
      : fixed512::query_strictly_inside_every_independent_sphere(
            support_boxes, support_size, query_box);
}

[[nodiscard]] __device__ __noinline__ fixed::Decision evaluate_fixed1024(
    ExactHigherSupportProductCudaTaskKind kind,
    const fixed::Binary64Aabb3 support_boxes[maximum_support_size],
    std::size_t support_size,
    const fixed::Binary64Aabb3& query_box) noexcept {
  return kind == ExactHigherSupportProductCudaTaskKind::support_prune
      ? fixed::no_well_centered_support(support_boxes, support_size)
      : fixed::query_strictly_inside_every_independent_sphere(
            support_boxes, support_size, query_box);
}

template <bool UseInt512>
[[nodiscard]] __device__ Phase15ExactHigherSupportProductCudaDeviceRecord
evaluate_task(
    const std::uint64_t* coordinate_bits,
    const DeviceNode* nodes,
    std::size_t point_count,
    std::size_t node_count,
    std::uint64_t source_snapshot_epoch,
    bool resident_lease_index_identity_validated,
    const Phase15ExactHigherSupportProductCudaTask& task,
    unsigned int* authority_failure) noexcept {
  Phase15ExactHigherSupportProductCudaDeviceRecord record{};
  record.task_id = task.task_id;
  record.kind = task.kind;
  record.outcome = Phase15ExactHigherSupportProductCudaDeviceOutcome::
      requires_cpu_rational_fallback;
  record.backend = Phase15ExactHigherSupportProductCudaDeviceBackend::
      arbitrary_precision_rational;

  bool authority_valid =
      resident_lease_index_identity_validated &&
      task.source_snapshot_epoch == source_snapshot_epoch &&
      (task.support_size == 3U || task.support_size == 4U) &&
      task.support_group_count != 0U &&
      task.support_group_count <= task.support_size &&
      task.support_group_count <= maximum_support_size;
  fixed::Binary64Aabb3 support_boxes[maximum_support_size]{};
  std::size_t expanded_count = 0U;
  std::uint64_t previous_leaf_end = 0U;
  for (std::size_t group = 0U;
       authority_valid && group < task.support_group_count;
       ++group) {
    const std::uint64_t node_index = task.support_node_indices[group];
    const std::uint64_t leaf_begin = task.support_leaf_begins[group];
    const std::uint64_t leaf_end = task.support_leaf_ends[group];
    const std::uint8_t multiplicity = task.support_multiplicities[group];
    const std::uint64_t leaf_count = leaf_end >= leaf_begin
        ? leaf_end - leaf_begin
        : 0U;
    authority_valid = native_node_matches(
        nodes,
        node_count,
        node_index,
        leaf_begin,
        leaf_end,
        point_count) &&
        multiplicity != 0U && multiplicity <= leaf_count &&
        (group == 0U || leaf_begin >= previous_leaf_end) &&
        expanded_count + multiplicity <= task.support_size;
    if (!authority_valid) {
      break;
    }
    previous_leaf_end = leaf_end;
    fixed::Binary64Aabb3 native_box{};
    authority_valid = load_native_box(
        nodes[node_index], coordinate_bits, point_count, native_box);
    for (std::size_t copy = 0U;
         authority_valid && copy < multiplicity;
         ++copy) {
      authority_valid = matches_proposed_box(
          native_box, task.support_boxes[expanded_count]);
      support_boxes[expanded_count] = native_box;
      ++expanded_count;
    }
  }
  authority_valid = authority_valid &&
      expanded_count == task.support_size;

  fixed::Binary64Aabb3 query_box{};
  if (authority_valid &&
      task.kind == ExactHigherSupportProductCudaTaskKind::support_prune) {
    authority_valid = !task.has_query_box;
  } else if (
      authority_valid &&
      task.kind ==
          ExactHigherSupportProductCudaTaskKind::query_strict_interior) {
    authority_valid = task.has_query_box && native_node_matches(
        nodes,
        node_count,
        task.query_node_index,
        task.query_leaf_begin,
        task.query_leaf_end,
        point_count);
    for (std::size_t group = 0U;
         authority_valid && group < task.support_group_count;
         ++group) {
      authority_valid = !ranges_overlap(
          task.query_leaf_begin,
          task.query_leaf_end,
          task.support_leaf_begins[group],
          task.support_leaf_ends[group]);
    }
    if (authority_valid) {
      authority_valid = load_native_box(
          nodes[task.query_node_index],
          coordinate_bits,
          point_count,
          query_box) &&
          matches_proposed_box(query_box, task.query_box);
    }
  } else {
    authority_valid = false;
  }

  std::uint64_t native_coordinate_width = 0U;
  if (authority_valid) {
    const fixed::Binary64Aabb3* query =
        task.kind ==
            ExactHigherSupportProductCudaTaskKind::query_strict_interior
        ? &query_box
        : nullptr;
    authority_valid = fixed::aligned_product_coordinate_bit_width(
        support_boxes,
        task.support_size,
        query,
        native_coordinate_width);
    const auto expected_width =
        native_coordinate_width <= fixed512::aligned_coordinate_bit_limit
        ? Phase15ExactHigherSupportProductCudaArithmeticWidth::int512
        : Phase15ExactHigherSupportProductCudaArithmeticWidth::int1024;
    authority_valid = authority_valid &&
        native_coordinate_width == task.aligned_coordinate_bit_width &&
        task.arithmetic_width == expected_width;
  }

  if (!authority_valid) {
    atomicExch(authority_failure, 1U);
    return record;
  }

  if constexpr (UseInt512) {
    const fixed512::Decision narrow_decision = evaluate_fixed512(
        task.kind, support_boxes, task.support_size, query_box);
    if (narrow_decision ==
        fixed512::Decision::requires_cpu_rational_fallback) {
      record.outcome = Phase15ExactHigherSupportProductCudaDeviceOutcome::
          requires_host_int1024_fallback;
      record.backend =
          Phase15ExactHigherSupportProductCudaDeviceBackend::
              bounded_dyadic_int1024;
    } else {
      record.outcome = fixed512_outcome(narrow_decision);
      record.backend =
          Phase15ExactHigherSupportProductCudaDeviceBackend::
              bounded_dyadic_int512;
    }
  } else {
    const fixed::Decision wide_decision = evaluate_fixed1024(
        task.kind, support_boxes, task.support_size, query_box);
    record.outcome = fixed_outcome(wide_decision);
    record.backend = wide_decision ==
            fixed::Decision::requires_cpu_rational_fallback
        ? Phase15ExactHigherSupportProductCudaDeviceBackend::
              arbitrary_precision_rational
        : Phase15ExactHigherSupportProductCudaDeviceBackend::
              bounded_dyadic_int1024;
  }
  return record;
}

template <bool UseInt512>
__global__ void morsehgp3d_phase15_exact_higher_support_product_kernel(
    const std::uint64_t* coordinate_bits,
    const DeviceNode* nodes,
    std::size_t point_count,
    std::size_t node_count,
    std::uint64_t source_snapshot_epoch,
    bool resident_lease_index_identity_validated,
    const Phase15ExactHigherSupportProductCudaTask* tasks,
    Phase15ExactHigherSupportProductCudaDeviceRecord* records,
    unsigned int* authority_failure,
    std::size_t task_count) {
  const std::size_t index =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index >= task_count) {
    return;
  }
  constexpr auto selected_width = UseInt512
      ? Phase15ExactHigherSupportProductCudaArithmeticWidth::int512
      : Phase15ExactHigherSupportProductCudaArithmeticWidth::int1024;
  if (tasks[index].arithmetic_width != selected_width) {
    return;
  }
  records[index] = evaluate_task<UseInt512>(
      coordinate_bits,
      nodes,
      point_count,
      node_count,
      source_snapshot_epoch,
      resident_lease_index_identity_validated,
      tasks[index],
      authority_failure);
}

}  // namespace

Phase15ExactHigherSupportProductCudaReceipt
phase15_launch_exact_higher_support_product_cuda(
    const Phase15ExactHigherSupportProductCudaRequest& request) {
  if (request.host_fake || request.tasks.empty() ||
      request.tasks.size() > request.maximum_task_count ||
      request.point_count == 0U || request.certified_node_count == 0U ||
      request.source_snapshot_epoch == 0U ||
      !request.resident_lease_index_identity_validated ||
      request.device_coordinate_bits == nullptr ||
      request.device_nodes == nullptr || request.cuda_device < 0 ||
      request.submitted_task_digest !=
          phase15_exact_higher_support_product_task_digest(request.tasks)) {
    throw std::invalid_argument(
        "the exact higher-support product CUDA launcher rejected its batch");
  }

  const std::size_t task_bytes = request.tasks.size_bytes();
  if (request.tasks.size() >
      std::numeric_limits<std::size_t>::max() /
          sizeof(Phase15ExactHigherSupportProductCudaDeviceRecord)) {
    throw std::length_error(
        "the exact higher-support product result extent overflows size_t");
  }
  const std::size_t result_bytes = request.tasks.size() *
      sizeof(Phase15ExactHigherSupportProductCudaDeviceRecord);
  std::size_t record_offset = 0U;
  std::size_t control_offset = 0U;
  std::size_t arena_bytes = 0U;
  if (!checked_align_up(task_bytes, alignof(
          Phase15ExactHigherSupportProductCudaDeviceRecord), record_offset) ||
      !checked_sum(record_offset, result_bytes, control_offset) ||
      !checked_align_up(
          control_offset, alignof(unsigned int), control_offset) ||
      !checked_sum(control_offset, sizeof(unsigned int), arena_bytes)) {
    throw std::length_error(
        "the exact higher-support product batch arena overflows size_t");
  }
  const std::size_t block_count =
      (request.tasks.size() - 1U) / threads_per_block + 1U;
  std::size_t int512_task_count = 0U;
  std::size_t int1024_task_count = 0U;
  for (const auto& task : request.tasks) {
    switch (task.arithmetic_width) {
      case Phase15ExactHigherSupportProductCudaArithmeticWidth::int512:
        ++int512_task_count;
        break;
      case Phase15ExactHigherSupportProductCudaArithmeticWidth::int1024:
        ++int1024_task_count;
        break;
      default:
        throw std::invalid_argument(
            "the exact higher-support product batch has an invalid "
            "arithmetic route");
    }
  }
  const std::size_t arithmetic_kernel_count =
      (int512_task_count != 0U ? 1U : 0U) +
      (int1024_task_count != 0U ? 1U : 0U);
  if (arithmetic_kernel_count == 0U) {
    throw std::logic_error(
        "the exact higher-support product batch selected no arithmetic "
        "kernel");
  }

  DeviceGuard guard{request.cuda_device};
  cudaDeviceProp properties{};
  check_cuda(
      cudaGetDeviceProperties(&properties, request.cuda_device),
      "cudaGetDeviceProperties exact higher-support product batch");
  if (properties.major != 12 || properties.minor != 0 ||
      properties.maxGridSize[0] <= 0 ||
      block_count > static_cast<std::size_t>(properties.maxGridSize[0])) {
    throw std::runtime_error(
        "the exact higher-support product batch requires an sm_120 grid");
  }

  std::vector<Phase15ExactHigherSupportProductCudaDeviceRecord> host_records(
      request.tasks.size());
  unsigned int authority_failure = 0U;
  std::uint64_t kernel_elapsed_ns = 0U;
  {
    Stream stream;
    Event kernel_begin;
    Event kernel_end;
    BatchArena arena{arena_bytes};
    auto* device_tasks = reinterpret_cast<
        Phase15ExactHigherSupportProductCudaTask*>(arena.get());
    auto* device_records = reinterpret_cast<
        Phase15ExactHigherSupportProductCudaDeviceRecord*>(
            arena.get() + record_offset);
    auto* device_authority_failure = reinterpret_cast<unsigned int*>(
        arena.get() + control_offset);
    check_cuda(
        cudaMemcpyAsync(
            device_tasks,
            request.tasks.data(),
            task_bytes,
            cudaMemcpyHostToDevice,
            stream.get()),
        "cudaMemcpyAsync exact higher-support product tasks");
    check_cuda(
        cudaMemsetAsync(
            device_authority_failure,
            0,
            sizeof(unsigned int),
            stream.get()),
        "cudaMemsetAsync exact higher-support product authority control");
    check_cuda(
        cudaEventRecord(kernel_begin.get(), stream.get()),
        "cudaEventRecord exact higher-support product kernel begin");
    if (int512_task_count != 0U) {
      morsehgp3d_phase15_exact_higher_support_product_kernel<true>
          <<<static_cast<unsigned int>(block_count),
             threads_per_block,
             0U,
             stream.get()>>>(
              request.device_coordinate_bits,
              static_cast<const DeviceNode*>(request.device_nodes),
              request.point_count,
              request.certified_node_count,
              request.source_snapshot_epoch,
              request.resident_lease_index_identity_validated,
              device_tasks,
              device_records,
              device_authority_failure,
              request.tasks.size());
      check_cuda(
          cudaGetLastError(),
          "Phase 15 exact higher-support int512 kernel launch");
    }
    if (int1024_task_count != 0U) {
      morsehgp3d_phase15_exact_higher_support_product_kernel<false>
          <<<static_cast<unsigned int>(block_count),
             threads_per_block,
             0U,
             stream.get()>>>(
              request.device_coordinate_bits,
              static_cast<const DeviceNode*>(request.device_nodes),
              request.point_count,
              request.certified_node_count,
              request.source_snapshot_epoch,
              request.resident_lease_index_identity_validated,
              device_tasks,
              device_records,
              device_authority_failure,
              request.tasks.size());
      check_cuda(
          cudaGetLastError(),
          "Phase 15 exact higher-support int1024 kernel launch");
    }
    check_cuda(
        cudaEventRecord(kernel_end.get(), stream.get()),
        "cudaEventRecord exact higher-support product kernel end");
    check_cuda(
        cudaMemcpyAsync(
            host_records.data(),
            device_records,
            result_bytes,
            cudaMemcpyDeviceToHost,
            stream.get()),
        "cudaMemcpyAsync exact higher-support product records");
    check_cuda(
        cudaMemcpyAsync(
            &authority_failure,
            device_authority_failure,
            sizeof(unsigned int),
            cudaMemcpyDeviceToHost,
            stream.get()),
        "cudaMemcpyAsync exact higher-support authority control");
    check_cuda(
        cudaStreamSynchronize(stream.get()),
        "cudaStreamSynchronize exact higher-support product batch");
    float kernel_elapsed_ms = 0.0F;
    check_cuda(
        cudaEventElapsedTime(
            &kernel_elapsed_ms, kernel_begin.get(), kernel_end.get()),
        "cudaEventElapsedTime exact higher-support product kernel");
    const double elapsed_ns =
        static_cast<double>(kernel_elapsed_ms) * 1000000.0;
    if (!(elapsed_ns >= 0.0) ||
        elapsed_ns > static_cast<double>(
            std::numeric_limits<std::uint64_t>::max())) {
      throw std::runtime_error(
          "the exact higher-support product kernel timer returned an "
          "invalid duration");
    }
    kernel_elapsed_ns = static_cast<std::uint64_t>(elapsed_ns + 0.5);
  }
  guard.restore();

  Phase15ExactHigherSupportProductCudaReceipt receipt;
  receipt.records = std::move(host_records);
  receipt.submitted_task_digest = request.submitted_task_digest;
  receipt.completed_result_digest =
      phase15_exact_higher_support_product_device_result_digest(
          receipt.records);
  receipt.kernel_launch_count = arithmetic_kernel_count;
  receipt.synchronization_count = 1U;
  receipt.kernel_elapsed_ns = kernel_elapsed_ns;
  receipt.cuda_device = request.cuda_device;
  receipt.source_identity_authenticated = authority_failure == 0U;
  receipt.resident_lease_index_identity_validated =
      request.resident_lease_index_identity_validated &&
      authority_failure == 0U;
  receipt.kernel_elapsed_ns_available = true;
  receipt.every_task_classified_once = true;
  receipt.floating_point_decision_performed = false;
  receipt.bigint_support_mass_transferred = false;
  receipt.forbidden_intermediate_materialized = false;
  receipt.host_fake_lifecycle_exercised = false;
  receipt.cuda_execution_performed = true;
  receipt.native_lbvh_nodes_read_on_device = true;
  receipt.narrow_int512_kernel_executed = int512_task_count != 0U;
  return receipt;
}

}  // namespace morsehgp3d::gpu::detail
