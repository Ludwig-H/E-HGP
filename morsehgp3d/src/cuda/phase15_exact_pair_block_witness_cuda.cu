#include "phase15_exact_pair_block_witness_cuda_internal.hpp"

#include "phase15_exact_pair_block_q_fixed.cuh"
#include "phase2b_interval.cuh"

#include "morsehgp3d/spatial/lbvh.hpp"

#include <cuda_runtime.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if !defined(__CUDACC__)
#error "phase15_exact_pair_block_witness_cuda.cu requires NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The exact pair-block witness kernel requires CUDA 12.9"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for exact pair-block witness certification"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the directed FP64 proposal"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The exact pair-block witness target must contain only sm_120 code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

inline constexpr unsigned int threads_per_block = 256U;
inline constexpr std::size_t axis_count = 3U;

struct DeviceNode {
  std::uint64_t lower_point_ids[3];
  std::uint64_t upper_point_ids[3];
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
        "cudaGetDevice before exact pair-block witness batch");
    if (previous_ != target) {
      check_cuda(
          cudaSetDevice(target),
          "cudaSetDevice for exact pair-block witness batch");
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
          "cudaSetDevice restore after exact pair-block witness batch");
      restore_ = false;
    }
  }

 private:
  int previous_{};
  bool restore_{false};
};

class BatchArena final {
 public:
  explicit BatchArena(std::size_t byte_count) {
    void* allocation = nullptr;
    check_cuda(
        cudaMalloc(&allocation, byte_count),
        "cudaMalloc exact pair-block witness compact batch arena");
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

class Stream final {
 public:
  Stream() {
    check_cuda(
        cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags exact pair-block witness batch");
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
        "cudaEventCreateWithFlags exact pair-block witness batch");
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

[[nodiscard]] __device__ bool intervals_overlap(
    std::uint32_t first_begin,
    std::uint32_t first_end,
    std::uint32_t second_begin,
    std::uint32_t second_end) noexcept {
  return first_begin < second_end && second_begin < first_end;
}

[[nodiscard]] __device__ bool native_node_matches(
    const DeviceNode* nodes,
    std::size_t node_count,
    std::uint32_t node_index,
    std::uint32_t leaf_begin,
    std::uint32_t leaf_end,
    std::size_t point_count) noexcept {
  if (node_index >= node_count || leaf_begin >= leaf_end ||
      leaf_end > point_count) {
    return false;
  }
  const DeviceNode& node = nodes[node_index];
  return node.leaf_begin == leaf_begin && node.leaf_end == leaf_end;
}

[[nodiscard]] __device__ bool load_box(
    const DeviceNode& node,
    const std::uint64_t* coordinate_bits,
    std::size_t point_count,
    exact_pair_block_q_fixed::Binary64Aabb3& box) noexcept {
  for (std::size_t axis = 0U; axis < axis_count; ++axis) {
    const std::uint64_t lower_id = node.lower_point_ids[axis];
    const std::uint64_t upper_id = node.upper_point_ids[axis];
    if (lower_id >= point_count || upper_id >= point_count) {
      return false;
    }
    box.lower[axis] = coordinate_bits[axis * point_count + lower_id];
    box.upper[axis] = coordinate_bits[axis * point_count + upper_id];
    if (!exact_pair_block_q_fixed::binary64_interval_ordered(
            box.lower[axis], box.upper[axis])) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] __device__ ExactPairBlockWitnessCudaProposalState
directed_proposal(
    const exact_pair_block_q_fixed::Binary64Aabb3& first,
    const exact_pair_block_q_fixed::Binary64Aabb3& second,
    const exact_pair_block_q_fixed::Binary64Aabb3& witness) noexcept {
  device::DeviceInterval sum = device::point_interval(UINT64_C(0));
  for (std::size_t axis = 0U; axis < axis_count; ++axis) {
    bool initialized = false;
    device::DeviceInterval axis_maximum{};
    for (std::size_t query_selector = 0U;
         query_selector < 2U;
         ++query_selector) {
      const std::uint64_t query_bits = query_selector == 0U
          ? witness.lower[axis]
          : witness.upper[axis];
      for (std::size_t first_selector = 0U;
           first_selector < 2U;
           ++first_selector) {
        const std::uint64_t first_bits = first_selector == 0U
            ? first.lower[axis]
            : first.upper[axis];
        for (std::size_t second_selector = 0U;
             second_selector < 2U;
             ++second_selector) {
          const std::uint64_t second_bits = second_selector == 0U
              ? second.lower[axis]
              : second.upper[axis];
          const device::DeviceInterval candidate =
              device::multiply_intervals(
                  device::subtract_intervals(
                      device::point_interval(query_bits),
                      device::point_interval(first_bits)),
                  device::subtract_intervals(
                      device::point_interval(query_bits),
                      device::point_interval(second_bits)));
          if (!candidate.valid) {
            return ExactPairBlockWitnessCudaProposalState::
                directed_arithmetic_overflow;
          }
          if (!initialized) {
            axis_maximum = candidate;
            initialized = true;
          } else {
            axis_maximum.lower = candidate.lower > axis_maximum.lower
                ? candidate.lower
                : axis_maximum.lower;
            axis_maximum.upper = candidate.upper > axis_maximum.upper
                ? candidate.upper
                : axis_maximum.upper;
          }
        }
      }
    }
    sum = device::add_intervals(sum, axis_maximum);
    if (!sum.valid) {
      return ExactPairBlockWitnessCudaProposalState::
          directed_arithmetic_overflow;
    }
  }
  if (sum.upper < 0.0) {
    return ExactPairBlockWitnessCudaProposalState::directed_negative;
  }
  if (sum.lower >= 0.0) {
    return ExactPairBlockWitnessCudaProposalState::directed_nonnegative;
  }
  return ExactPairBlockWitnessCudaProposalState::directed_ambiguous;
}

[[nodiscard]] __device__ std::uint8_t decision_word(
    ExactPairBlockWitnessCudaDecision decision) noexcept {
  return static_cast<std::uint8_t>(decision);
}

[[nodiscard]] __device__ std::uint8_t proposal_word(
    ExactPairBlockWitnessCudaProposalState state) noexcept {
  return static_cast<std::uint8_t>(state);
}

__global__ void morsehgp3d_phase15_exact_pair_block_witness_kernel(
    const std::uint64_t* coordinate_bits,
    const DeviceNode* nodes,
    std::size_t point_count,
    std::size_t node_count,
    const Phase15ExactPairBlockWitnessCudaTask* tasks,
    Phase15ExactPairBlockWitnessCudaDeviceRecord* records,
    std::size_t task_count,
    std::size_t maximum_closed_rank) {
  const std::size_t index =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index >= task_count) {
    return;
  }
  const Phase15ExactPairBlockWitnessCudaTask task = tasks[index];
  Phase15ExactPairBlockWitnessCudaDeviceRecord record{};
  record.task_id = task.task_id;
  record.unordered_pair_mass = task.unordered_pair_mass;
  record.decision = decision_word(
      ExactPairBlockWitnessCudaDecision::residual_invalid_authority);
  record.proposal_state = proposal_word(
      ExactPairBlockWitnessCudaProposalState::not_evaluated);

  const bool native = task.host_input_flags == 0U &&
      native_node_matches(
          nodes, node_count, task.first_node_index, task.first_leaf_begin,
          task.first_leaf_end, point_count) &&
      native_node_matches(
          nodes, node_count, task.second_node_index, task.second_leaf_begin,
          task.second_leaf_end, point_count) &&
      native_node_matches(
          nodes, node_count, task.witness_node_index,
          task.witness_leaf_begin, task.witness_leaf_end, point_count);
  const std::uint64_t first_count =
      static_cast<std::uint64_t>(task.first_leaf_end) -
      task.first_leaf_begin;
  const std::uint64_t second_count =
      static_cast<std::uint64_t>(task.second_leaf_end) -
      task.second_leaf_begin;
  const bool mass_valid = first_count == 0U ||
      (second_count <=
           ~std::uint64_t{0} / first_count &&
       first_count * second_count == task.unordered_pair_mass);
  if (!native || !mass_valid) {
    records[index] = record;
    return;
  }
  if (task.first_leaf_end > task.second_leaf_begin ||
      intervals_overlap(
          task.first_leaf_begin, task.first_leaf_end,
          task.witness_leaf_begin, task.witness_leaf_end) ||
      intervals_overlap(
          task.second_leaf_begin, task.second_leaf_end,
          task.witness_leaf_begin, task.witness_leaf_end)) {
    record.decision = decision_word(
        ExactPairBlockWitnessCudaDecision::residual_support_overlap);
    records[index] = record;
    return;
  }
  const std::uint64_t witness_count =
      static_cast<std::uint64_t>(task.witness_leaf_end) -
      task.witness_leaf_begin;
  if (witness_count < maximum_closed_rank - 1U) {
    record.decision = decision_word(ExactPairBlockWitnessCudaDecision::
        inconclusive_insufficient_witness_mass);
    records[index] = record;
    return;
  }

  exact_pair_block_q_fixed::Binary64Aabb3 first{};
  exact_pair_block_q_fixed::Binary64Aabb3 second{};
  exact_pair_block_q_fixed::Binary64Aabb3 witness{};
  if (!load_box(
          nodes[task.first_node_index], coordinate_bits, point_count, first) ||
      !load_box(
          nodes[task.second_node_index], coordinate_bits, point_count,
          second) ||
      !load_box(
          nodes[task.witness_node_index], coordinate_bits, point_count,
          witness)) {
    records[index] = record;
    return;
  }
  record.proposal_state = proposal_word(
      directed_proposal(first, second, witness));

  // The interval sign above is proposal-only.  Every possible negative
  // closure is independently decided again by exact checked limbs.
  record.fixed_limb_evaluation_performed = 1U;
  const exact_phi_fixed::ResultCode exact =
      exact_pair_block_q_fixed::exact_maximum_sign(first, second, witness);
  switch (exact) {
    case exact_phi_fixed::ResultCode::negative:
      record.exact_sign = -1;
      record.decision = decision_word(
          ExactPairBlockWitnessCudaDecision::certified_closed);
      break;
    case exact_phi_fixed::ResultCode::zero:
      record.exact_sign = 0;
      record.decision = decision_word(ExactPairBlockWitnessCudaDecision::
          inconclusive_nonnegative_q);
      break;
    case exact_phi_fixed::ResultCode::positive:
      record.exact_sign = 1;
      record.decision = decision_word(ExactPairBlockWitnessCudaDecision::
          inconclusive_nonnegative_q);
      break;
    case exact_phi_fixed::ResultCode::requires_cpu_fallback:
    case exact_phi_fixed::ResultCode::invalid_input:
      record.decision = decision_word(ExactPairBlockWitnessCudaDecision::
          residual_fixed_limb_overflow);
      break;
  }
  records[index] = record;
}

[[nodiscard]] ExactPairBlockWitnessCudaDecision decode_decision(
    std::uint8_t value) {
  if (value > static_cast<std::uint8_t>(
                  ExactPairBlockWitnessCudaDecision::
                      residual_fixed_limb_overflow)) {
    throw std::logic_error(
        "the exact pair-block witness CUDA kernel returned an invalid "
        "decision word");
  }
  return static_cast<ExactPairBlockWitnessCudaDecision>(value);
}

[[nodiscard]] ExactPairBlockWitnessCudaProposalState decode_proposal(
    std::uint8_t value) {
  if (value > static_cast<std::uint8_t>(
                  ExactPairBlockWitnessCudaProposalState::
                      directed_arithmetic_overflow)) {
    throw std::logic_error(
        "the exact pair-block witness CUDA kernel returned an invalid "
        "proposal word");
  }
  return static_cast<ExactPairBlockWitnessCudaProposalState>(value);
}

void accumulate(
    const ExactPairBlockWitnessCudaRecord& record,
    ExactPairBlockWitnessCudaAudit& audit) {
  switch (record.proposal_state) {
    case ExactPairBlockWitnessCudaProposalState::not_evaluated:
      break;
    case ExactPairBlockWitnessCudaProposalState::directed_negative:
      ++audit.directed_negative_count;
      break;
    case ExactPairBlockWitnessCudaProposalState::directed_nonnegative:
      ++audit.directed_nonnegative_count;
      break;
    case ExactPairBlockWitnessCudaProposalState::directed_ambiguous:
      ++audit.directed_ambiguous_count;
      break;
    case ExactPairBlockWitnessCudaProposalState::
        directed_arithmetic_overflow:
      ++audit.directed_arithmetic_overflow_count;
      break;
  }
  if (record.fixed_limb_evaluation_performed) {
    if (record.decision ==
        ExactPairBlockWitnessCudaDecision::residual_fixed_limb_overflow) {
      ++audit.fixed_limb_residual_count;
    } else {
      ++audit.fixed_limb_exact_decision_count;
      if (record.exact_sign < 0) {
        ++audit.exact_negative_count;
      } else if (record.exact_sign == 0) {
        ++audit.exact_zero_count;
      } else {
        ++audit.exact_positive_count;
      }
    }
  }
  switch (record.decision) {
    case ExactPairBlockWitnessCudaDecision::certified_closed:
      ++audit.certified_task_count;
      audit.pruned_unordered_pair_mass += record.unordered_pair_mass;
      return;
    case ExactPairBlockWitnessCudaDecision::inconclusive_nonnegative_q:
      ++audit.nonnegative_task_count;
      break;
    case ExactPairBlockWitnessCudaDecision::
        inconclusive_insufficient_witness_mass:
      ++audit.insufficient_witness_task_count;
      break;
    case ExactPairBlockWitnessCudaDecision::residual_invalid_authority:
      ++audit.invalid_authority_task_count;
      break;
    case ExactPairBlockWitnessCudaDecision::residual_support_overlap:
      ++audit.support_overlap_task_count;
      break;
    case ExactPairBlockWitnessCudaDecision::residual_fixed_limb_overflow:
      break;
  }
  audit.residual_unordered_pair_mass += record.unordered_pair_mass;
}

}  // namespace

Phase15ExactPairBlockWitnessCudaReceipt
phase15_launch_exact_pair_block_witness_cuda(
    const Phase15ExactPairBlockWitnessCudaRequest& request) {
  if (request.traversal == nullptr || !request.traversal->ready ||
      request.traversal->host_fake || !request.traversal->retained_owner ||
      !request.traversal->source_cloud_identity ||
      request.traversal->device_coordinate_bits == nullptr ||
      request.traversal->device_nodes == nullptr ||
      request.traversal->cuda_device < 0 || request.tasks.empty() ||
      request.tasks.size() > request.task_capacity ||
      request.submitted_task_digest !=
          phase15_exact_pair_block_witness_task_digest(request.tasks)) {
    throw std::invalid_argument(
        "the exact pair-block witness CUDA launcher received invalid native "
        "authority or batch metadata");
  }
  const std::size_t task_bytes = request.tasks.size_bytes();
  if (request.tasks.size() >
      std::numeric_limits<std::size_t>::max() /
          sizeof(Phase15ExactPairBlockWitnessCudaDeviceRecord)) {
    throw std::length_error(
        "the exact pair-block witness result extent overflows size_t");
  }
  const std::size_t result_bytes = request.tasks.size() *
      sizeof(Phase15ExactPairBlockWitnessCudaDeviceRecord);
  const std::size_t result_offset =
      (task_bytes + alignof(Phase15ExactPairBlockWitnessCudaDeviceRecord) -
       1U) &
      ~(alignof(Phase15ExactPairBlockWitnessCudaDeviceRecord) - 1U);
  if (result_offset > std::numeric_limits<std::size_t>::max() - result_bytes) {
    throw std::length_error(
        "the exact pair-block witness compact arena overflows size_t");
  }
  const std::size_t block_count =
      (request.tasks.size() - 1U) / threads_per_block + 1U;

  DeviceGuard guard{request.traversal->cuda_device};
  cudaDeviceProp properties{};
  check_cuda(
      cudaGetDeviceProperties(
          &properties, request.traversal->cuda_device),
      "cudaGetDeviceProperties exact pair-block witness batch");
  if (properties.major != 12 || properties.minor != 0 ||
      properties.maxGridSize[0] <= 0 ||
      block_count > static_cast<std::size_t>(properties.maxGridSize[0])) {
    throw std::runtime_error(
        "the exact pair-block witness batch requires a bounded sm_120 grid");
  }
  std::vector<Phase15ExactPairBlockWitnessCudaDeviceRecord> host_records(
      request.tasks.size());
  std::uint64_t kernel_elapsed_nanoseconds = 0U;
  {
    Stream stream;
    BatchArena arena{result_offset + result_bytes};
    Event kernel_begin;
    Event kernel_end;
    auto* device_tasks = reinterpret_cast<
        Phase15ExactPairBlockWitnessCudaTask*>(arena.get());
    auto* device_records = reinterpret_cast<
        Phase15ExactPairBlockWitnessCudaDeviceRecord*>(
            arena.get() + result_offset);
    check_cuda(
        cudaMemcpyAsync(
            device_tasks, request.tasks.data(), task_bytes,
        cudaMemcpyHostToDevice, stream.get()),
        "cudaMemcpyAsync exact pair-block witness compact tasks");

    check_cuda(
        cudaEventRecord(kernel_begin.get(), stream.get()),
        "cudaEventRecord before exact pair-block witness kernel");
    morsehgp3d_phase15_exact_pair_block_witness_kernel
        <<<static_cast<unsigned int>(block_count), threads_per_block, 0U,
           stream.get()>>>(
            request.traversal->device_coordinate_bits,
            static_cast<const DeviceNode*>(request.traversal->device_nodes),
            request.traversal->point_count,
            request.traversal->certified_node_count,
            device_tasks,
            device_records,
            request.tasks.size(),
            request.config.maximum_closed_rank);
    check_cuda(
        cudaGetLastError(),
        "Phase 15 exact pair-block witness compact kernel launch");
    check_cuda(
        cudaEventRecord(kernel_end.get(), stream.get()),
        "cudaEventRecord after exact pair-block witness kernel");
    check_cuda(
        cudaMemcpyAsync(
            host_records.data(), device_records, result_bytes,
            cudaMemcpyDeviceToHost, stream.get()),
        "cudaMemcpyAsync exact pair-block witness final records");
    check_cuda(
        cudaStreamSynchronize(stream.get()),
        "cudaStreamSynchronize exact pair-block witness compact batch");
    float kernel_elapsed_milliseconds = 0.0F;
    check_cuda(
        cudaEventElapsedTime(
            &kernel_elapsed_milliseconds,
            kernel_begin.get(),
            kernel_end.get()),
        "cudaEventElapsedTime exact pair-block witness kernel");
    kernel_elapsed_nanoseconds = static_cast<std::uint64_t>(
        kernel_elapsed_milliseconds * 1000000.0F);
  }
  // The stream and arena must be destroyed while the target CUDA device is
  // still current.  Only then may the prior caller device be restored.
  guard.restore();

  Phase15ExactPairBlockWitnessCudaReceipt receipt;
  receipt.records.reserve(host_records.size());
  ExactPairBlockWitnessCudaAudit& audit = receipt.audit;
  audit.point_count = request.traversal->point_count;
  audit.certified_node_count = request.traversal->certified_node_count;
  audit.maximum_closed_rank = request.config.maximum_closed_rank;
  audit.task_capacity_per_point = request.config.task_capacity_per_point;
  audit.task_capacity = request.task_capacity;
  audit.submitted_task_count = request.tasks.size();
  audit.completed_task_count = request.tasks.size();
  audit.host_to_device_task_byte_count = task_bytes;
  audit.device_to_host_result_byte_count = result_bytes;
  audit.compact_task_record_byte_size =
      sizeof(Phase15ExactPairBlockWitnessCudaTask);
  audit.compact_result_record_byte_size =
      sizeof(Phase15ExactPairBlockWitnessCudaDeviceRecord);
  audit.device_arena_allocation_count = 1U;
  audit.kernel_launch_count = 1U;
  audit.synchronization_count = 1U;
  audit.source_snapshot_epoch = request.traversal->source_snapshot_epoch;
  audit.submitted_task_digest = request.submitted_task_digest;
  audit.kernel_elapsed_nanoseconds = kernel_elapsed_nanoseconds;
  audit.cuda_device = request.traversal->cuda_device;
  audit.native_lbvh_authority_consumed = true;
  audit.native_lbvh_nodes_read_on_device = true;
  audit.fixed_linear_task_capacity_validated = true;
  audit.bounded_final_qualification_readback_performed = true;
  audit.compact_batch_abi_validated = true;
  audit.resident_batch_submission_validated = true;
  audit.fp64_used_as_proposal_only = true;
  audit.negative_closure_requires_fixed_limb_exact_decision = true;
  audit.cuda_execution_performed = true;

  for (const Phase15ExactPairBlockWitnessCudaDeviceRecord& device_record :
       host_records) {
    ExactPairBlockWitnessCudaRecord record;
    record.task_id = device_record.task_id;
    record.decision = decode_decision(device_record.decision);
    record.proposal_state = decode_proposal(device_record.proposal_state);
    record.exact_sign = device_record.exact_sign;
    record.unordered_pair_mass =
        static_cast<std::size_t>(device_record.unordered_pair_mass);
    record.fixed_limb_evaluation_performed =
        device_record.fixed_limb_evaluation_performed != 0U;
    audit.submitted_unordered_pair_mass += record.unordered_pair_mass;
    accumulate(record, audit);
    receipt.records.push_back(record);
  }
  audit.local_submitted_mass_conservation_validated =
      audit.pruned_unordered_pair_mass +
              audit.residual_unordered_pair_mass ==
          audit.submitted_unordered_pair_mass;
  receipt.submitted_task_digest = request.submitted_task_digest;
  receipt.completed_result_digest =
      phase15_exact_pair_block_witness_result_digest(receipt.records);
  audit.completed_result_digest = receipt.completed_result_digest;
  receipt.source_identity_authenticated = true;
  receipt.every_task_classified_once = true;
  return receipt;
}

}  // namespace morsehgp3d::gpu::detail
