#include "phase15_higher_support_device_tiled_frontier_internal.hpp"

#include "phase15_higher_support_device_tiled_slot_engine.cuh"

#include "morsehgp3d/hierarchy/higher_support_product.hpp"
#include "morsehgp3d/spatial/aabb.hpp"

#include <cuda_runtime.h>

#include <algorithm>
#include <array>
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
#error "phase15_higher_support_device_tiled_frontier.cu requires NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Phase 15 higher-support tiled frontier requires CUDA 12.9"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the Phase 15 higher-support frontier"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the Phase 15 higher-support frontier"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Phase 15 higher-support frontier must contain only sm_120 code"
#endif

// Native launcher of the Phase 15 higher-support device tiled frontier.
// Every scientific step is executed by the shared slot engine
// (phase15_higher_support_device_tiled_slot_engine.cuh), whose host
// instantiation is certified bit-identical to the scientific host fake by
// the slot-engine parity suite; this translation unit contributes only the
// resident arena, the kernel and the bounded orchestration protocol.
//
// Execution model: one thread per slot in a grid-stride loop.  The per-slot
// DFS is inherently sequential and every exact decision is a wide fixed
// integer evaluation, so thread-per-slot preserves the proven engine
// verbatim; intra-warp cooperation on the probe plan remains a measured
// optimisation for the M5 qualification, never a correctness lever.

namespace morsehgp3d::gpu::detail {
namespace {

namespace engine = higher_support_slot_engine;

constexpr unsigned int kThreadsPerBlock = 128U;

class Phase15HigherSupportDeviceTiledCudaFailure final
    : public std::runtime_error {
 public:
  Phase15HigherSupportDeviceTiledCudaFailure(
      cudaError_t code,
      std::string operation)
      : std::runtime_error(message(code, operation)) {}

 private:
  [[nodiscard]] static std::string message(
      cudaError_t code,
      const std::string& operation) {
    const char* description = cudaGetErrorString(code);
    return operation + " failed: " +
        (description == nullptr ? std::string{"unknown CUDA error"}
                                : std::string{description});
  }
};

void check_cuda(cudaError_t code, std::string operation) {
  if (code != cudaSuccess) {
    throw Phase15HigherSupportDeviceTiledCudaFailure(
        code, std::move(operation));
  }
}

[[nodiscard]] std::size_t checked_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::length_error(message);
  }
  return left * right;
}

[[nodiscard]] std::size_t checked_sum(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
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
          "a Phase 15 higher-support device allocation is invalid");
    }
    check_cuda(
        cudaMalloc(
            reinterpret_cast<void**>(&data_), count * sizeof(Value)),
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
        "cudaGetDevice before the Phase 15 higher-support frontier");
    if (previous_device_ != target_device) {
      check_cuda(
          cudaSetDevice(target_device),
          "cudaSetDevice for the Phase 15 higher-support frontier");
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

 private:
  int previous_device_{};
  bool restore_required_{false};
};

void require_device_pointer(
    const void* pointer,
    int expected_device,
    std::size_t alignment,
    const char* label) {
  if (pointer == nullptr ||
      reinterpret_cast<std::uintptr_t>(pointer) % alignment != 0U) {
    throw std::invalid_argument(
        std::string{"the Phase 15 higher-support "} + label +
        " pointer is null or misaligned");
  }
  cudaPointerAttributes attributes{};
  check_cuda(
      cudaPointerGetAttributes(&attributes, pointer),
      std::string{
          "cudaPointerGetAttributes for the Phase 15 higher-support "} +
          label);
  if (attributes.type != cudaMemoryTypeDevice ||
      attributes.device != expected_device) {
    throw std::invalid_argument(
        std::string{"the Phase 15 higher-support "} + label +
        " pointer is not owned by the traversal CUDA device");
  }
}

// ---------------------------------------------------------------------------
// Kernel.
// ---------------------------------------------------------------------------

struct KernelParams {
  const std::uint64_t* coordinate_bits;
  const std::uint64_t* morton_point_ids;
  const engine::EngineNode* nodes;
  Phase15HigherSupportDeviceTiledProductRecord* products;
  Phase15HigherSupportDeviceTiledPruneRecord* prunes;
  Phase15HigherSupportDeviceTiledTerminalRecord* terminals;
  Phase15HigherSupportDeviceTiledProbeReceipt* receipts;
  Phase15HigherSupportDeviceTiledSlotControl* controls;
  engine::EngineSlotCheckpoint* checkpoints;
  Phase15HigherSupportDeviceTiledRationalDrainTask* rational_tasks;
  // control_block[0]: quantum-paused slot count; control_block[1]: staged
  // rational-drain request count.
  unsigned long long* control_block;
  const std::uint8_t* verdicts;
  std::uint64_t point_count;
  std::uint64_t node_count;
  std::uint64_t slot_count;
  std::uint64_t maximum_relevant_closed_rank;
  std::uint64_t gate_evaluations_per_slot_per_chunk;
  std::uint64_t products_per_slot;
  std::uint64_t prune_records_per_slot;
  std::uint64_t terminal_records_per_slot;
  std::uint64_t probe_receipts_per_slot;
  std::uint64_t tile_epoch;
  std::uint64_t chunk_sequence;
  std::uint8_t terminal_policy;
  std::uint8_t grant_fresh_quantum;
  std::uint8_t fatalize_on_pause;
  std::uint8_t consume_verdicts;
};

__device__ void stage_rational_task(
    const KernelParams& params,
    const engine::EngineSlotContext& context,
    engine::EngineSlotCheckpoint& checkpoint,
    std::uint64_t slot_index) {
  const Phase15HigherSupportDeviceTiledProductRecord& entry =
      context.stack[checkpoint.stack_top - 1U];
  engine::EngineBox boxes[4]{};
  if (!engine::engine_support_boxes(context.geometry, entry, boxes)) {
    engine::engine_set_fatal(
        checkpoint,
        Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
    engine::engine_rollback_slot_chunk(context, checkpoint);
    return;
  }
  const unsigned long long staged =
      atomicAdd(params.control_block + 1U, 1ULL);
  if (staged >= params.slot_count) {
    engine::engine_set_fatal(
        checkpoint,
        Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
    engine::engine_rollback_slot_chunk(context, checkpoint);
    return;
  }
  Phase15HigherSupportDeviceTiledRationalDrainTask task{};
  for (std::size_t support = 0U; support < entry.support_size; ++support) {
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      task.support_lower_bits[support][axis] = boxes[support].lower[axis];
      task.support_upper_bits[support][axis] = boxes[support].upper[axis];
    }
  }
  if (static_cast<engine::EngineWideKind>(checkpoint.wide_kind) ==
      engine::EngineWideKind::query_cell) {
    engine::EngineBox query_box{};
    engine::engine_node_box(
        context.geometry, checkpoint.wide_query_node_index, query_box);
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      task.query_lower_bits[axis] = query_box.lower[axis];
      task.query_upper_bits[axis] = query_box.upper[axis];
    }
    task.has_query = 1U;
  }
  task.wide_kind = checkpoint.wide_kind;
  task.origin_slot_index = slot_index;
  task.support_size = entry.support_size;
  params.rational_tasks[staged] = task;
}

__global__ void phase15_higher_support_device_tiled_frontier_kernel(
    KernelParams params) {
  const std::uint64_t thread_index =
      static_cast<std::uint64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  const std::uint64_t thread_stride =
      static_cast<std::uint64_t>(gridDim.x) * blockDim.x;
  for (std::uint64_t slot = thread_index; slot < params.slot_count;
       slot += thread_stride) {
    engine::EngineSlotCheckpoint& checkpoint = params.checkpoints[slot];
    engine::EngineSlotContext context;
    context.geometry.coordinate_bits = params.coordinate_bits;
    context.geometry.morton_point_ids = params.morton_point_ids;
    context.geometry.nodes = params.nodes;
    context.geometry.point_count = params.point_count;
    context.geometry.node_count = params.node_count;
    context.terminal_policy = params.terminal_policy;
    context.maximum_relevant_closed_rank =
        params.maximum_relevant_closed_rank;
    context.gate_evaluations_per_slot_per_chunk =
        params.gate_evaluations_per_slot_per_chunk;
    context.products_per_slot = params.products_per_slot;
    context.prune_records_per_slot = params.prune_records_per_slot;
    context.terminal_records_per_slot = params.terminal_records_per_slot;
    context.probe_receipts_per_slot = params.probe_receipts_per_slot;
    context.stack = params.products + slot * params.products_per_slot;
    context.prunes = params.prunes + slot * params.prune_records_per_slot;
    context.terminals =
        params.terminals + slot * params.terminal_records_per_slot;
    context.receipts =
        params.receipts + slot * params.probe_receipts_per_slot;
    context.slot_index = slot;

    // Chunk rollover: commit the previous chunk exactly once, then reset
    // the chunk-scoped accumulators for the requested sequence.
    if (checkpoint.chunk_sequence != params.chunk_sequence) {
      if (checkpoint.chunk_sequence != 0U &&
          !engine::engine_commit_chunk(checkpoint)) {
        engine::engine_set_fatal(
            checkpoint,
            Phase15HigherSupportDeviceTiledFailureCode::
                arithmetic_overflow);
      }
      engine::engine_begin_chunk(checkpoint, params.chunk_sequence);
    }
    if (params.consume_verdicts != 0U && checkpoint.wide_pending != 0U &&
        checkpoint.wide_verdict_valid == 0U) {
      checkpoint.wide_verdict = params.verdicts[slot];
      checkpoint.wide_verdict_valid = 1U;
      checkpoint.run_state =
          static_cast<std::uint8_t>(engine::EngineRunState::active);
    }
    if (checkpoint.run_state ==
        static_cast<std::uint8_t>(engine::EngineRunState::paused)) {
      checkpoint.run_state =
          static_cast<std::uint8_t>(engine::EngineRunState::active);
    }
    if (checkpoint.run_state ==
        static_cast<std::uint8_t>(engine::EngineRunState::active)) {
      engine::engine_run_subdivision(
          context, checkpoint, params.grant_fresh_quantum != 0U);
      if (checkpoint.run_state ==
          static_cast<std::uint8_t>(engine::EngineRunState::fatal)) {
        engine::engine_rollback_slot_chunk(context, checkpoint);
      } else if (
          checkpoint.wide_pending != 0U &&
          checkpoint.wide_verdict_valid == 0U) {
        stage_rational_task(params, context, checkpoint, slot);
      } else if (
          params.fatalize_on_pause != 0U &&
          checkpoint.run_state ==
              static_cast<std::uint8_t>(engine::EngineRunState::paused)) {
        // The certified subdivision budget is exhausted: rollback and stop
        // fatally with the capacity proof, never a resumable yield.
        engine::engine_rollback_slot_chunk(context, checkpoint);
        checkpoint.run_state =
            static_cast<std::uint8_t>(engine::EngineRunState::fatal);
        checkpoint.stop_reason = static_cast<std::uint8_t>(
            Phase15HigherSupportDeviceTiledStopReason::
                subdivision_capacity);
        checkpoint.failure_code = static_cast<std::uint8_t>(
            Phase15HigherSupportDeviceTiledFailureCode::none);
      }
    }
    if (checkpoint.run_state ==
            static_cast<std::uint8_t>(engine::EngineRunState::paused) &&
        checkpoint.wide_pending == 0U) {
      atomicAdd(params.control_block, 1ULL);
    }
    if (!engine::engine_write_slot_control(
            checkpoint,
            params.tile_epoch,
            params.chunk_sequence,
            params.controls[slot])) {
      engine::engine_set_fatal(
          checkpoint,
          Phase15HigherSupportDeviceTiledFailureCode::arithmetic_overflow);
      engine::engine_rollback_slot_chunk(context, checkpoint);
      static_cast<void>(engine::engine_write_slot_control(
          checkpoint,
          params.tile_epoch,
          params.chunk_sequence,
          params.controls[slot]));
    }
  }
}

// ---------------------------------------------------------------------------
// Resident arena.
// ---------------------------------------------------------------------------

class Phase15HigherSupportDeviceTiledCudaResources final {
 public:
  Phase15HigherSupportDeviceTiledCudaResources(
      const Phase15HigherSupportDeviceTiledAdoptedTraversal& traversal,
      const Phase15HigherSupportDeviceTiledRequest& request,
      std::size_t product_capacity,
      std::size_t prune_capacity,
      std::size_t terminal_capacity,
      std::size_t receipt_capacity,
      std::size_t control_capacity,
      std::size_t checkpoint_capacity)
      : device_(traversal.cuda_device),
        traversal_owner_(traversal.retained_owner),
        source_cloud_identity_(traversal.source_cloud_identity),
        device_coordinate_bits_(traversal.device_coordinate_bits),
        device_morton_point_ids_(traversal.device_morton_point_ids),
        device_nodes_(traversal.device_nodes),
        source_snapshot_epoch_(request.source_snapshot_epoch),
        tile_epoch_(request.tile_epoch),
        chunk_sequence_(request.chunk_sequence),
        slot_count_(request.slot_count),
        terminal_policy_(request.terminal_policy),
        maximum_relevant_closed_rank_(
            request.maximum_relevant_closed_rank) {
    cudaStream_t created_stream = nullptr;
    check_cuda(
        cudaStreamCreateWithFlags(&created_stream, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags for the Phase 15 higher-support tile");
    try {
      products_.allocate(
          product_capacity,
          "cudaMalloc Phase 15 higher-support product stacks");
      prunes_.allocate(
          prune_capacity,
          "cudaMalloc Phase 15 higher-support prune segments");
      terminals_.allocate(
          terminal_capacity,
          "cudaMalloc Phase 15 higher-support terminal segments");
      receipts_.allocate(
          receipt_capacity,
          "cudaMalloc Phase 15 higher-support receipt segments");
      controls_.allocate(
          control_capacity,
          "cudaMalloc Phase 15 higher-support slot controls");
      checkpoints_.allocate(
          checkpoint_capacity,
          "cudaMalloc Phase 15 higher-support slot checkpoints");
      rational_tasks_.allocate(
          control_capacity,
          "cudaMalloc Phase 15 higher-support rational tasks");
      verdicts_.allocate(
          control_capacity,
          "cudaMalloc Phase 15 higher-support rational verdicts");
      control_block_.allocate(
          2U,
          "cudaMalloc Phase 15 higher-support device control block");
    } catch (...) {
      static_cast<void>(cudaStreamDestroy(created_stream));
      throw;
    }
    stream_ = created_stream;
  }

  Phase15HigherSupportDeviceTiledCudaResources(
      const Phase15HigherSupportDeviceTiledCudaResources&) = delete;
  Phase15HigherSupportDeviceTiledCudaResources& operator=(
      const Phase15HigherSupportDeviceTiledCudaResources&) = delete;

  ~Phase15HigherSupportDeviceTiledCudaResources() {
    int previous_device = 0;
    const cudaError_t query_status = cudaGetDevice(&previous_device);
    bool restore_device = false;
    if (query_status == cudaSuccess && previous_device != device_) {
      restore_device = cudaSetDevice(device_) == cudaSuccess;
    }
    if (query_status != cudaSuccess ||
        (previous_device != device_ && !restore_device)) {
      abandon_all();
      stream_ = nullptr;
      return;
    }
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
    reset_all();
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamDestroy(stream_));
      stream_ = nullptr;
    }
    if (restore_device) {
      static_cast<void>(cudaSetDevice(previous_device));
    }
  }

  [[nodiscard]] cudaStream_t stream() const noexcept { return stream_; }
  [[nodiscard]] Phase15HigherSupportDeviceTiledProductRecord* products()
      noexcept {
    return products_.get();
  }
  [[nodiscard]] Phase15HigherSupportDeviceTiledPruneRecord* prunes()
      noexcept {
    return prunes_.get();
  }
  [[nodiscard]] Phase15HigherSupportDeviceTiledTerminalRecord* terminals()
      noexcept {
    return terminals_.get();
  }
  [[nodiscard]] Phase15HigherSupportDeviceTiledProbeReceipt* receipts()
      noexcept {
    return receipts_.get();
  }
  [[nodiscard]] Phase15HigherSupportDeviceTiledSlotControl* controls()
      noexcept {
    return controls_.get();
  }
  [[nodiscard]] engine::EngineSlotCheckpoint* checkpoints() noexcept {
    return checkpoints_.get();
  }
  [[nodiscard]] Phase15HigherSupportDeviceTiledRationalDrainTask*
      rational_tasks() noexcept {
    return rational_tasks_.get();
  }
  [[nodiscard]] std::uint8_t* verdicts() noexcept {
    return verdicts_.get();
  }
  [[nodiscard]] unsigned long long* control_block() noexcept {
    return control_block_.get();
  }

  [[nodiscard]] bool matches_resume(
      const Phase15HigherSupportDeviceTiledAdoptedTraversal& traversal,
      const Phase15HigherSupportDeviceTiledRequest& request) const noexcept {
    return request.resume_same_tile && device_ == traversal.cuda_device &&
        traversal_owner_.get() == traversal.retained_owner.get() &&
        source_cloud_identity_.get() ==
            traversal.source_cloud_identity.get() &&
        device_coordinate_bits_ == traversal.device_coordinate_bits &&
        device_morton_point_ids_ == traversal.device_morton_point_ids &&
        device_nodes_ == traversal.device_nodes &&
        source_snapshot_epoch_ == request.source_snapshot_epoch &&
        tile_epoch_ == request.tile_epoch &&
        chunk_sequence_ != UINT64_MAX &&
        request.chunk_sequence == chunk_sequence_ + UINT64_C(1) &&
        slot_count_ == request.slot_count &&
        terminal_policy_ == request.terminal_policy &&
        maximum_relevant_closed_rank_ ==
            request.maximum_relevant_closed_rank;
  }

  [[nodiscard]] bool can_rebind_fresh_tile(
      const Phase15HigherSupportDeviceTiledAdoptedTraversal& traversal,
      const Phase15HigherSupportDeviceTiledRequest& request) const noexcept {
    return !request.resume_same_tile &&
        request.chunk_sequence == UINT64_C(1) &&
        request.tile_epoch != tile_epoch_ &&
        device_ == traversal.cuda_device &&
        traversal_owner_.get() == traversal.retained_owner.get() &&
        source_cloud_identity_.get() ==
            traversal.source_cloud_identity.get() &&
        device_coordinate_bits_ == traversal.device_coordinate_bits &&
        device_morton_point_ids_ == traversal.device_morton_point_ids &&
        device_nodes_ == traversal.device_nodes &&
        source_snapshot_epoch_ == request.source_snapshot_epoch &&
        slot_count_ == request.slot_count &&
        terminal_policy_ == request.terminal_policy &&
        maximum_relevant_closed_rank_ ==
            request.maximum_relevant_closed_rank &&
        controls_.count() == request.slot_count;
  }

  void rebind_fresh_tile(
      const Phase15HigherSupportDeviceTiledRequest& request) noexcept {
    tile_epoch_ = request.tile_epoch;
    chunk_sequence_ = request.chunk_sequence;
  }

  void commit_chunk_sequence(std::uint64_t chunk_sequence) noexcept {
    chunk_sequence_ = chunk_sequence;
  }

  void synchronize() {
    check_cuda(
        cudaStreamSynchronize(stream_),
        "cudaStreamSynchronize Phase 15 higher-support frontier");
  }

 private:
  void abandon_all() noexcept {
    control_block_.abandon();
    verdicts_.abandon();
    rational_tasks_.abandon();
    checkpoints_.abandon();
    controls_.abandon();
    receipts_.abandon();
    terminals_.abandon();
    prunes_.abandon();
    products_.abandon();
  }

  void reset_all() noexcept {
    control_block_.reset();
    verdicts_.reset();
    rational_tasks_.reset();
    checkpoints_.reset();
    controls_.reset();
    receipts_.reset();
    terminals_.reset();
    prunes_.reset();
    products_.reset();
  }

  int device_{-1};
  cudaStream_t stream_{nullptr};
  std::shared_ptr<void> traversal_owner_;
  std::shared_ptr<const void> source_cloud_identity_;
  const std::uint64_t* device_coordinate_bits_{};
  const std::uint64_t* device_morton_point_ids_{};
  const void* device_nodes_{};
  std::uint64_t source_snapshot_epoch_{};
  std::uint64_t tile_epoch_{};
  std::uint64_t chunk_sequence_{};
  std::size_t slot_count_{};
  HigherSupportDeviceTiledFrontierTerminalPolicy terminal_policy_{
      HigherSupportDeviceTiledFrontierTerminalPolicy::closed_rank_window};
  std::size_t maximum_relevant_closed_rank_{};
  DeviceBuffer<Phase15HigherSupportDeviceTiledProductRecord> products_;
  DeviceBuffer<Phase15HigherSupportDeviceTiledPruneRecord> prunes_;
  DeviceBuffer<Phase15HigherSupportDeviceTiledTerminalRecord> terminals_;
  DeviceBuffer<Phase15HigherSupportDeviceTiledProbeReceipt> receipts_;
  DeviceBuffer<Phase15HigherSupportDeviceTiledSlotControl> controls_;
  DeviceBuffer<engine::EngineSlotCheckpoint> checkpoints_;
  DeviceBuffer<Phase15HigherSupportDeviceTiledRationalDrainTask>
      rational_tasks_;
  DeviceBuffer<std::uint8_t> verdicts_;
  DeviceBuffer<unsigned long long> control_block_;
};

// Host resolution of one staged rational-drain task through the exact
// arbitrary-precision primitives; the verdict encoding matches the engine's
// sealed resume protocol.
[[nodiscard]] std::uint8_t resolve_rational_task(
    const Phase15HigherSupportDeviceTiledRationalDrainTask& task) {
  std::array<spatial::ExactDyadicAabb3, 4> boxes{};
  for (std::size_t support = 0U; support < task.support_size; ++support) {
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      boxes[support].lower_binary64_bits[axis] =
          task.support_lower_bits[support][axis];
      boxes[support].upper_binary64_bits[axis] =
          task.support_upper_bits[support][axis];
    }
  }
  const std::span<const spatial::ExactDyadicAabb3> support_span{
      boxes.data(), task.support_size};
  switch (static_cast<engine::EngineWideKind>(task.wide_kind)) {
    case engine::EngineWideKind::gate_no_well_centered:
      return hierarchy::
                     exact_higher_support_product_no_well_centered_certified(
                         support_span)
          ? std::uint8_t{1U}
          : std::uint8_t{0U};
    case engine::EngineWideKind::gate_all_well_centered:
      return hierarchy::
                     exact_higher_support_product_all_well_centered_certified(
                         support_span)
          ? std::uint8_t{1U}
          : std::uint8_t{0U};
    case engine::EngineWideKind::query_cell: {
      spatial::ExactDyadicAabb3 query{};
      for (std::size_t axis = 0U; axis < 3U; ++axis) {
        query.lower_binary64_bits[axis] = task.query_lower_bits[axis];
        query.upper_binary64_bits[axis] = task.query_upper_bits[axis];
      }
      const auto decision =
          hierarchy::exact_higher_support_product_query_cell_decision(
              support_span, query);
      if (decision ==
          hierarchy::ExactHigherSupportProductQueryCellDecision::
              strictly_inside_every_independent_sphere) {
        return static_cast<std::uint8_t>(
            engine::EngineQueryVerdict::strictly_inside);
      }
      if (decision ==
          hierarchy::ExactHigherSupportProductQueryCellDecision::
              outside_or_boundary_every_independent_sphere) {
        return static_cast<std::uint8_t>(
            engine::EngineQueryVerdict::outside_or_boundary);
      }
      return static_cast<std::uint8_t>(
          engine::EngineQueryVerdict::inconclusive);
    }
    case engine::EngineWideKind::terminal_geometry: {
      hierarchy::ExactHigherSupportProductAabbDecisionBackend backend{};
      const auto decision =
          hierarchy::exact_higher_support_terminal_geometry_decision(
              support_span, &backend);
      std::uint8_t verdict = static_cast<std::uint8_t>(decision);
      if (backend ==
          hierarchy::ExactHigherSupportProductAabbDecisionBackend::
              arbitrary_precision_rational) {
        verdict = static_cast<std::uint8_t>(
            verdict | engine::engine_terminal_verdict_rational_flag);
      }
      return verdict;
    }
    default:
      throw std::runtime_error(
          "a Phase 15 higher-support rational task carries an unknown "
          "wide kind");
  }
}

void validate_native_launch(
    const Phase15HigherSupportDeviceTiledAdoptedTraversal& traversal,
    const Phase15HigherSupportDeviceTiledRequest& request) {
  if (!traversal.retained_owner || !traversal.source_cloud_identity ||
      traversal.execution_kind !=
          Phase15HigherSupportDeviceTiledExecutionKind::cuda ||
      traversal.host_fake_lifecycle_exercised ||
      !traversal.cuda_device_storage_retained ||
      traversal.cuda_device < 0 ||
      request.source_snapshot_epoch != traversal.source_snapshot_epoch ||
      request.record_buffer_epoch == 0U || request.tile_epoch == 0U ||
      request.chunk_sequence == 0U ||
      request.point_count != traversal.point_count ||
      request.certified_node_count != traversal.certified_node_count ||
      request.point_count == 0U ||
      static_cast<std::uint64_t>(request.point_count) >
          higher_support_device_tiled_frontier_maximum_point_count ||
      request.certified_maximum_lbvh_depth >
          higher_support_device_tiled_frontier_maximum_certified_lbvh_depth ||
      higher_support_device_tiled_frontier_proved_stack_bound(
          request.certified_maximum_lbvh_depth) >
          request.products_per_slot ||
      request.slot_count == 0U ||
      request.slot_count >
          higher_support_device_tiled_frontier_maximum_slot_tile_capacity ||
      request.products_per_slot !=
          higher_support_device_tiled_frontier_products_per_slot ||
      request.prune_records_per_slot !=
          higher_support_device_tiled_frontier_prune_records_per_slot ||
      request.terminal_records_per_slot !=
          higher_support_device_tiled_frontier_terminal_records_per_slot ||
      request.probe_receipts_per_slot !=
          higher_support_device_tiled_frontier_probe_receipts_per_slot ||
      request.pending_decisions_per_slot !=
          higher_support_device_tiled_frontier_pending_decisions_per_slot ||
      !higher_support_device_tiled_frontier_terminal_policy_known(
          request.terminal_policy) ||
      request.maximum_relevant_closed_rank < 2U ||
      request.maximum_relevant_closed_rank >
          higher_support_device_tiled_frontier_maximum_closed_rank ||
      request.gate_evaluations_per_slot_per_chunk < 2U ||
      request.maximum_subdivision_count == 0U ||
      (request.resume_same_tile
           ? !request.root_entries.empty()
           : request.root_entries.size() != request.slot_count)) {
    throw std::invalid_argument(
        "the Phase 15 higher-support CUDA launcher received an invalid "
        "request or traversal authority");
  }
  cudaDeviceProp properties{};
  check_cuda(
      cudaGetDeviceProperties(&properties, traversal.cuda_device),
      "cudaGetDeviceProperties for the Phase 15 higher-support frontier");
  if (properties.major != 12 || properties.minor != 0) {
    throw std::runtime_error(
        "the Phase 15 higher-support frontier requires an sm_120 device");
  }
}

}  // namespace

Phase15HigherSupportDeviceTiledAdoptedTraversal
adopt_phase15_higher_support_device_tiled_traversal(
    MortonLbvhDeviceTraversalLease&& traversal_lease) {
  if (!traversal_lease.ready() || !traversal_lease.cuda_resident() ||
      traversal_lease.audit().host_fake_lifecycle_exercised ||
      !traversal_lease.audit().cuda_device_storage_retained) {
    throw std::invalid_argument(
        "the production Phase 15 higher-support adoption requires a ready "
        "CUDA-resident traversal lease");
  }
  const MortonLbvhDeviceTraversalLeaseAudit audit = traversal_lease.audit_;
  DeviceGuard guard{traversal_lease.cuda_device_};
  require_device_pointer(
      traversal_lease.device_coordinate_bits_,
      traversal_lease.cuda_device_,
      alignof(std::uint64_t),
      "coordinate");
  require_device_pointer(
      traversal_lease.device_morton_point_ids_,
      traversal_lease.cuda_device_,
      alignof(std::uint64_t),
      "Morton PointId");
  require_device_pointer(
      traversal_lease.device_nodes_,
      traversal_lease.cuda_device_,
      alignof(engine::EngineNode),
      "node");

  auto owner = std::make_shared<MortonLbvhDeviceTraversalLease>(
      std::move(traversal_lease));
  Phase15HigherSupportDeviceTiledAdoptedTraversal adopted;
  adopted.retained_owner = owner;
  adopted.source_cloud_identity =
      std::shared_ptr<const void>{owner, owner.get()};
  adopted.device_coordinate_bits = owner->device_coordinate_bits_;
  adopted.device_morton_point_ids = owner->device_morton_point_ids_;
  adopted.device_nodes = owner->device_nodes_;
  adopted.point_count = audit.point_count;
  adopted.certified_node_count = audit.certified_node_count;
  adopted.maximum_point_count = audit.maximum_point_count;
  adopted.maximum_node_count = audit.maximum_node_count;
  adopted.retained_coordinate_word_capacity =
      audit.retained_coordinate_word_capacity;
  adopted.retained_morton_point_id_capacity =
      audit.retained_morton_point_id_capacity;
  adopted.retained_node_capacity = audit.retained_node_capacity;
  adopted.source_snapshot_epoch = audit.source_snapshot_epoch;
  adopted.cuda_device = owner->cuda_device_;
  adopted.execution_kind =
      Phase15HigherSupportDeviceTiledExecutionKind::cuda;
  adopted.canonical_coordinate_words_retained =
      audit.canonical_coordinate_words_retained;
  adopted.active_morton_point_ids_retained =
      audit.active_morton_point_ids_retained;
  adopted.certified_device_nodes_retained =
      audit.certified_device_nodes_retained;
  adopted.host_fake_lifecycle_exercised = false;
  adopted.cuda_device_storage_retained = true;
  return adopted;
}

Phase15HigherSupportDeviceTiledBatch
build_phase15_higher_support_device_tiled_frontier_on_device(
    Phase15HigherSupportDeviceTiledFrontierContextState& context,
    const Phase15HigherSupportDeviceTiledAdoptedTraversal& traversal,
    const Phase15HigherSupportDeviceTiledRequest& request) {
  validate_native_launch(traversal, request);
  DeviceGuard guard{traversal.cuda_device};

  const std::size_t product_capacity = checked_product(
      request.slot_count,
      request.products_per_slot,
      "the Phase 15 higher-support product capacity overflows size_t");
  const std::size_t prune_capacity = checked_product(
      request.slot_count,
      request.prune_records_per_slot,
      "the Phase 15 higher-support prune capacity overflows size_t");
  const std::size_t terminal_capacity = checked_product(
      request.slot_count,
      request.terminal_records_per_slot,
      "the Phase 15 higher-support terminal capacity overflows size_t");
  const std::size_t receipt_capacity = checked_product(
      request.slot_count,
      request.probe_receipts_per_slot,
      "the Phase 15 higher-support receipt capacity overflows size_t");
  const std::size_t deferred_capacity = checked_product(
      request.slot_count,
      request.pending_decisions_per_slot,
      "the Phase 15 higher-support deferred capacity overflows size_t");

  std::shared_ptr<void>& retained_device_resources =
      context.device_resources();
  if (retained_device_resources != nullptr &&
      retained_device_resources.use_count() != 1L) {
    throw std::logic_error(
        "the Phase 15 higher-support CUDA arena cannot be reused while a "
        "detached record drain lease is alive");
  }
  std::shared_ptr<Phase15HigherSupportDeviceTiledCudaResources> resources;
  bool fresh_tile_device_arena_allocated = false;
  bool fresh_tile_device_arena_reused = false;
  if (request.resume_same_tile) {
    if (retained_device_resources == nullptr) {
      throw std::invalid_argument(
          "the Phase 15 higher-support CUDA tile cannot resume without "
          "its retained device arena");
    }
    resources = std::static_pointer_cast<
        Phase15HigherSupportDeviceTiledCudaResources>(
        retained_device_resources);
    if (!resources->matches_resume(traversal, request)) {
      throw std::invalid_argument(
          "the Phase 15 higher-support CUDA continuation identity does "
          "not match the retained tile");
    }
  } else {
    if (retained_device_resources != nullptr) {
      resources = std::static_pointer_cast<
          Phase15HigherSupportDeviceTiledCudaResources>(
          retained_device_resources);
      if (resources->can_rebind_fresh_tile(traversal, request)) {
        resources->rebind_fresh_tile(request);
        fresh_tile_device_arena_reused = true;
      } else {
        resources.reset();
        retained_device_resources.reset();
      }
    }
    if (resources == nullptr) {
      resources =
          std::make_shared<Phase15HigherSupportDeviceTiledCudaResources>(
              traversal,
              request,
              product_capacity,
              prune_capacity,
              terminal_capacity,
              receipt_capacity,
              request.slot_count,
              request.slot_count);
      retained_device_resources = resources;
      fresh_tile_device_arena_allocated = true;
    }
  }

  std::size_t kernel_launch_count = 0U;
  std::size_t synchronization_count = 0U;
  std::size_t traversal_subdivision_count = 0U;
  std::size_t host_rational_drain_relaunch_count = 0U;
  std::size_t rational_task_device_to_host_count = 0U;
  std::uint64_t chunk_rational_drain_hits = 0U;

  try {
    if (!request.resume_same_tile) {
      // Fresh tile: validate the roots on the host with the engine's own
      // exact functions, then install the rooted stacks and checkpoints.
      std::vector<engine::EngineSlotCheckpoint> host_checkpoints(
          request.slot_count);
      std::vector<Phase15HigherSupportDeviceTiledProductRecord> roots(
          request.slot_count);
      for (std::size_t slot = 0U; slot < request.slot_count; ++slot) {
        const hierarchy::ExactHigherSupportFrontierEntry& entry =
            request.root_entries[slot];
        Phase15HigherSupportDeviceTiledProductRecord root{};
        root.support_size = entry.support_size;
        root.group_count = entry.group_count;
        for (std::size_t index = 0U; index < entry.group_count; ++index) {
          root.group_node_index[index] = entry.groups[index].node_index;
          root.group_leaf_begin[index] = entry.groups[index].leaf_begin;
          root.group_leaf_end[index] = entry.groups[index].leaf_end;
          root.group_multiplicity[index] =
              entry.groups[index].multiplicity;
        }
        roots[slot] = root;
        engine::EngineSlotCheckpoint& checkpoint = host_checkpoints[slot];
        checkpoint.stack_top = 1U;
        checkpoint.stack_high_water = 1U;
        checkpoint.root_entry_digest =
            engine::engine_root_entry_digest(root);
        if (!engine::engine_entry_mass(root, checkpoint.universe_mass)) {
          throw std::invalid_argument(
              "a Phase 15 higher-support root mass leaves the certified "
              "unsigned 128-bit range");
        }
        checkpoint.tile_epoch = request.tile_epoch;
        checkpoint.chunk_sequence = 0U;
      }
      check_cuda(
          cudaMemsetAsync(
              resources->prunes(),
              0,
              prune_capacity *
                  sizeof(Phase15HigherSupportDeviceTiledPruneRecord),
              resources->stream()),
          "cudaMemsetAsync Phase 15 higher-support prune segments");
      check_cuda(
          cudaMemsetAsync(
              resources->terminals(),
              0,
              terminal_capacity *
                  sizeof(Phase15HigherSupportDeviceTiledTerminalRecord),
              resources->stream()),
          "cudaMemsetAsync Phase 15 higher-support terminal segments");
      check_cuda(
          cudaMemsetAsync(
              resources->receipts(),
              0,
              receipt_capacity *
                  sizeof(Phase15HigherSupportDeviceTiledProbeReceipt),
              resources->stream()),
          "cudaMemsetAsync Phase 15 higher-support receipt segments");
      check_cuda(
          cudaMemcpyAsync(
              resources->checkpoints(),
              host_checkpoints.data(),
              request.slot_count * sizeof(engine::EngineSlotCheckpoint),
              cudaMemcpyHostToDevice,
              resources->stream()),
          "cudaMemcpyAsync Phase 15 higher-support checkpoint install");
      check_cuda(
          cudaMemcpy2DAsync(
              resources->products(),
              request.products_per_slot *
                  sizeof(Phase15HigherSupportDeviceTiledProductRecord),
              roots.data(),
              sizeof(Phase15HigherSupportDeviceTiledProductRecord),
              sizeof(Phase15HigherSupportDeviceTiledProductRecord),
              request.slot_count,
              cudaMemcpyHostToDevice,
              resources->stream()),
          "cudaMemcpy2DAsync Phase 15 higher-support root install");
    }

    KernelParams params{};
    params.coordinate_bits = traversal.device_coordinate_bits;
    params.morton_point_ids = traversal.device_morton_point_ids;
    params.nodes =
        static_cast<const engine::EngineNode*>(traversal.device_nodes);
    params.products = resources->products();
    params.prunes = resources->prunes();
    params.terminals = resources->terminals();
    params.receipts = resources->receipts();
    params.controls = resources->controls();
    params.checkpoints = resources->checkpoints();
    params.rational_tasks = resources->rational_tasks();
    params.control_block = resources->control_block();
    params.verdicts = resources->verdicts();
    params.point_count = request.point_count;
    params.node_count = request.certified_node_count;
    params.slot_count = request.slot_count;
    params.maximum_relevant_closed_rank =
        request.maximum_relevant_closed_rank;
    params.gate_evaluations_per_slot_per_chunk =
        request.gate_evaluations_per_slot_per_chunk;
    params.products_per_slot = request.products_per_slot;
    params.prune_records_per_slot = request.prune_records_per_slot;
    params.terminal_records_per_slot = request.terminal_records_per_slot;
    params.probe_receipts_per_slot = request.probe_receipts_per_slot;
    params.tile_epoch = request.tile_epoch;
    params.chunk_sequence = request.chunk_sequence;
    params.terminal_policy =
        static_cast<std::uint8_t>(request.terminal_policy);

    const unsigned int block_count = static_cast<unsigned int>(
        (request.slot_count + kThreadsPerBlock - 1U) / kThreadsPerBlock);
    std::array<unsigned long long, 2> host_control_block{};

    const auto launch_once = [&](bool grant_fresh,
                                 bool fatalize_on_pause,
                                 bool consume_verdicts) {
      params.grant_fresh_quantum = grant_fresh ? 1U : 0U;
      params.fatalize_on_pause = fatalize_on_pause ? 1U : 0U;
      params.consume_verdicts = consume_verdicts ? 1U : 0U;
      check_cuda(
          cudaMemsetAsync(
              resources->control_block(),
              0,
              2U * sizeof(unsigned long long),
              resources->stream()),
          "cudaMemsetAsync Phase 15 higher-support control block");
      phase15_higher_support_device_tiled_frontier_kernel<<<
          block_count,
          kThreadsPerBlock,
          0U,
          resources->stream()>>>(params);
      check_cuda(
          cudaGetLastError(),
          "phase15_higher_support_device_tiled_frontier_kernel launch");
      check_cuda(
          cudaMemcpyAsync(
              host_control_block.data(),
              resources->control_block(),
              2U * sizeof(unsigned long long),
              cudaMemcpyDeviceToHost,
              resources->stream()),
          "cudaMemcpyAsync Phase 15 higher-support control block drain");
      resources->synchronize();
      ++kernel_launch_count;
      ++synchronization_count;
    };

    const auto drain_rational_requests = [&]() {
      while (host_control_block[1] != 0U) {
        const std::size_t staged =
            static_cast<std::size_t>(host_control_block[1]);
        std::vector<Phase15HigherSupportDeviceTiledRationalDrainTask>
            tasks(staged);
        check_cuda(
            cudaMemcpy(
                tasks.data(),
                resources->rational_tasks(),
                staged *
                    sizeof(
                        Phase15HigherSupportDeviceTiledRationalDrainTask),
                cudaMemcpyDeviceToHost),
            "cudaMemcpy Phase 15 higher-support rational task drain");
        rational_task_device_to_host_count += staged;
        chunk_rational_drain_hits += staged;
        std::vector<std::uint8_t> host_verdicts(request.slot_count, 0U);
        for (const auto& task : tasks) {
          host_verdicts[static_cast<std::size_t>(
              task.origin_slot_index)] = resolve_rational_task(task);
        }
        check_cuda(
            cudaMemcpyAsync(
                const_cast<std::uint8_t*>(params.verdicts),
                host_verdicts.data(),
                request.slot_count * sizeof(std::uint8_t),
                cudaMemcpyHostToDevice,
                resources->stream()),
            "cudaMemcpyAsync Phase 15 higher-support verdict install");
        launch_once(false, false, true);
        ++host_rational_drain_relaunch_count;
      }
    };

    for (std::size_t subdivision_index = 0U;
         subdivision_index < request.maximum_subdivision_count;
         ++subdivision_index) {
      launch_once(true, false, false);
      ++traversal_subdivision_count;
      drain_rational_requests();
      if (host_control_block[0] == 0U) {
        break;
      }
      if (subdivision_index + 1U == request.maximum_subdivision_count) {
        // The certified budget is exhausted with unresolved slots: one
        // terminal fatalize sweep rolls them back with the capacity stop.
        launch_once(false, true, false);
        ++host_rational_drain_relaunch_count;
        drain_rational_requests();
      }
    }

    Phase15HigherSupportDeviceTiledBatch batch;
    batch.host_slot_controls.resize(request.slot_count);
    check_cuda(
        cudaMemcpyAsync(
            batch.host_slot_controls.data(),
            resources->controls(),
            request.slot_count *
                sizeof(Phase15HigherSupportDeviceTiledSlotControl),
            cudaMemcpyDeviceToHost,
            resources->stream()),
        "cudaMemcpyAsync Phase 15 higher-support slot control drain");
    resources->synchronize();
    ++synchronization_count;

    // Tile-level fatal collapse, mirror of the scientific host fake: a
    // fatal slot censors the whole tile, chunk-ready siblings withhold
    // their chunk, and a slot that completed within the fatal chunk breaks
    // atomicity and fails the launch.
    bool tile_fatal = false;
    bool capacity_trigger = false;
    for (const Phase15HigherSupportDeviceTiledSlotControl& control :
         batch.host_slot_controls) {
      if (control.status ==
          static_cast<std::uint64_t>(
              Phase15HigherSupportDeviceTiledSlotStatus::fatal)) {
        tile_fatal = true;
        capacity_trigger = capacity_trigger ||
            control.stop_reason ==
                static_cast<std::uint64_t>(
                    Phase15HigherSupportDeviceTiledStopReason::
                        subdivision_capacity);
      }
    }
    if (tile_fatal) {
      for (Phase15HigherSupportDeviceTiledSlotControl& control :
           batch.host_slot_controls) {
        if (control.status ==
            static_cast<std::uint64_t>(
                Phase15HigherSupportDeviceTiledSlotStatus::chunk_ready)) {
          Phase15HigherSupportDeviceTiledUnsigned128 well{
              control.well_prune_mass_cumulative_lo,
              control.well_prune_mass_cumulative_hi};
          Phase15HigherSupportDeviceTiledUnsigned128 rank{
              control.rank_prune_mass_cumulative_lo,
              control.rank_prune_mass_cumulative_hi};
          Phase15HigherSupportDeviceTiledUnsigned128 terminal{
              control.terminal_mass_cumulative_lo,
              control.terminal_mass_cumulative_hi};
          Phase15HigherSupportDeviceTiledUnsigned128 unresolved{
              control.unresolved_mass_lo, control.unresolved_mass_hi};
          const bool arithmetic_sound =
              phase15_higher_support_device_tiled_u128_subtract(
                  well,
                  Phase15HigherSupportDeviceTiledUnsigned128{
                      control.well_prune_mass_delta_lo,
                      control.well_prune_mass_delta_hi}) &&
              phase15_higher_support_device_tiled_u128_subtract(
                  rank,
                  Phase15HigherSupportDeviceTiledUnsigned128{
                      control.rank_prune_mass_delta_lo,
                      control.rank_prune_mass_delta_hi}) &&
              phase15_higher_support_device_tiled_u128_subtract(
                  terminal,
                  Phase15HigherSupportDeviceTiledUnsigned128{
                      control.terminal_mass_delta_lo,
                      control.terminal_mass_delta_hi}) &&
              phase15_higher_support_device_tiled_u128_add(
                  unresolved,
                  Phase15HigherSupportDeviceTiledUnsigned128{
                      control.well_prune_mass_delta_lo,
                      control.well_prune_mass_delta_hi}) &&
              phase15_higher_support_device_tiled_u128_add(
                  unresolved,
                  Phase15HigherSupportDeviceTiledUnsigned128{
                      control.rank_prune_mass_delta_lo,
                      control.rank_prune_mass_delta_hi}) &&
              phase15_higher_support_device_tiled_u128_add(
                  unresolved,
                  Phase15HigherSupportDeviceTiledUnsigned128{
                      control.terminal_mass_delta_lo,
                      control.terminal_mass_delta_hi});
          if (!arithmetic_sound) {
            throw std::runtime_error(
                "a Phase 15 higher-support fatal-collapse rollback left "
                "the certified unsigned 128-bit range");
          }
          control.well_prune_mass_cumulative_lo = well.lo;
          control.well_prune_mass_cumulative_hi = well.hi;
          control.rank_prune_mass_cumulative_lo = rank.lo;
          control.rank_prune_mass_cumulative_hi = rank.hi;
          control.terminal_mass_cumulative_lo = terminal.lo;
          control.terminal_mass_cumulative_hi = terminal.hi;
          control.unresolved_mass_lo = unresolved.lo;
          control.unresolved_mass_hi = unresolved.hi;
          control.well_prune_mass_delta_lo = 0U;
          control.well_prune_mass_delta_hi = 0U;
          control.rank_prune_mass_delta_lo = 0U;
          control.rank_prune_mass_delta_hi = 0U;
          control.terminal_mass_delta_lo = 0U;
          control.terminal_mass_delta_hi = 0U;
          control.expansion_count = 0U;
          control.gate_evaluation_count = 0U;
          control.probe_root_decision_count = 0U;
          control.probe_fallback_decision_count = 0U;
          control.deferred_int512_count = 0U;
          control.deferred_int1024_count = 0U;
          control.rational_drain_count = 0U;
          control.prune_record_count = 0U;
          control.terminal_record_count = 0U;
          control.probe_receipt_count = 0U;
          control.status = static_cast<std::uint64_t>(
              Phase15HigherSupportDeviceTiledSlotStatus::fatal);
          control.yield_reason = static_cast<std::uint64_t>(
              Phase15HigherSupportDeviceTiledYieldReason::none);
          control.stop_reason = capacity_trigger
              ? static_cast<std::uint64_t>(
                    Phase15HigherSupportDeviceTiledStopReason::
                        subdivision_capacity)
              : static_cast<std::uint64_t>(
                    Phase15HigherSupportDeviceTiledStopReason::
                        fatal_failure);
          control.failure_code = static_cast<std::uint64_t>(
              Phase15HigherSupportDeviceTiledFailureCode::none);
          continue;
        }
        const bool completed_this_chunk =
            control.status ==
                static_cast<std::uint64_t>(
                    Phase15HigherSupportDeviceTiledSlotStatus::complete) &&
            (control.well_prune_mass_delta_lo != 0U ||
             control.well_prune_mass_delta_hi != 0U ||
             control.rank_prune_mass_delta_lo != 0U ||
             control.rank_prune_mass_delta_hi != 0U ||
             control.terminal_mass_delta_lo != 0U ||
             control.terminal_mass_delta_hi != 0U ||
             control.expansion_count != 0U ||
             control.gate_evaluation_count != 0U ||
             control.prune_record_count != 0U ||
             control.terminal_record_count != 0U ||
             control.probe_receipt_count != 0U);
        if (completed_this_chunk) {
          throw std::runtime_error(
              "the Phase 15 higher-support subdivision-capacity stop "
              "could not form an atomic terminal batch after withholding "
              "its current output chunk");
        }
      }
    }

    resources->commit_chunk_sequence(request.chunk_sequence);

    batch.retained_output_owner = resources;
    batch.source_cloud_identity_authority =
        traversal.source_cloud_identity;
    // R2-f: stage the three record arenas device-to-host.
    //
    // Everything below the driver walks these segments with host loads.
    // The arenas are cudaMalloc allocations, so publishing them raw -- as
    // this code did until now -- handed a host consumer a device pointer.
    // The staging preserves the ORIGINAL per-slot stride: consumers address
    // `segment[slot * stride + index]`, so compacting would mis-address
    // every slot but the first.  Only the committed prefix of a slot means
    // anything, so the copied row width is the maximum committed count over
    // the slots, taken from the controls already staged above; the rest of
    // each row stays value-initialised and is never read.
    std::size_t staged_prunes_per_slot = 0U;
    std::size_t staged_terminals_per_slot = 0U;
    std::size_t staged_receipts_per_slot = 0U;
    for (const Phase15HigherSupportDeviceTiledSlotControl& control :
         batch.host_slot_controls) {
      staged_prunes_per_slot = std::max(
          staged_prunes_per_slot,
          static_cast<std::size_t>(control.prune_record_count));
      staged_terminals_per_slot = std::max(
          staged_terminals_per_slot,
          static_cast<std::size_t>(control.terminal_record_count));
      staged_receipts_per_slot = std::max(
          staged_receipts_per_slot,
          static_cast<std::size_t>(control.probe_receipt_count));
    }
    // A control that overran its fixed segment is a device defect; the
    // consumer rejects it, and clamping here only keeps the copy inside
    // the arena that was actually allocated.
    staged_prunes_per_slot =
        std::min(staged_prunes_per_slot, request.prune_records_per_slot);
    staged_terminals_per_slot = std::min(
        staged_terminals_per_slot, request.terminal_records_per_slot);
    staged_receipts_per_slot =
        std::min(staged_receipts_per_slot, request.probe_receipts_per_slot);

    // One strided copy per arena.  A zero row width means no slot committed
    // a record of that kind; the destination stays empty and the published
    // pointer stays null, which the consumer already refuses.
    std::size_t staging_copy_count = 0U;
    std::size_t staging_byte_count = 0U;
    const auto stage_segment =
        [&](void* destination,
            const void* source,
            std::size_t element_bytes,
            std::size_t stride_elements,
            std::size_t staged_elements,
            const char* what) {
          if (staged_elements == 0U || request.slot_count == 0U) {
            return;
          }
          check_cuda(
              cudaMemcpy2DAsync(
                  destination,
                  stride_elements * element_bytes,
                  source,
                  stride_elements * element_bytes,
                  staged_elements * element_bytes,
                  request.slot_count,
                  cudaMemcpyDeviceToHost,
                  resources->stream()),
              what);
          ++staging_copy_count;
          staging_byte_count += checked_product(
              staged_elements * element_bytes,
              request.slot_count,
              "the Phase 15 record staging transfer extent overflows "
              "size_t");
        };

    batch.host_prune_records.assign(
        prune_capacity, Phase15HigherSupportDeviceTiledPruneRecord{});
    batch.host_terminal_records.assign(
        terminal_capacity, Phase15HigherSupportDeviceTiledTerminalRecord{});
    batch.host_probe_receipts.assign(
        receipt_capacity, Phase15HigherSupportDeviceTiledProbeReceipt{});
    stage_segment(
        batch.host_prune_records.data(),
        resources->prunes(),
        sizeof(Phase15HigherSupportDeviceTiledPruneRecord),
        request.prune_records_per_slot,
        staged_prunes_per_slot,
        "cudaMemcpy2DAsync Phase 15 higher-support prune record staging");
    stage_segment(
        batch.host_terminal_records.data(),
        resources->terminals(),
        sizeof(Phase15HigherSupportDeviceTiledTerminalRecord),
        request.terminal_records_per_slot,
        staged_terminals_per_slot,
        "cudaMemcpy2DAsync Phase 15 higher-support terminal record staging");
    stage_segment(
        batch.host_probe_receipts.data(),
        resources->receipts(),
        sizeof(Phase15HigherSupportDeviceTiledProbeReceipt),
        request.probe_receipts_per_slot,
        staged_receipts_per_slot,
        "cudaMemcpy2DAsync Phase 15 higher-support probe receipt staging");
    // Only synchronise when something was actually issued, so the declared
    // synchronisation count and the declared copy count stay in lockstep.
    if (staging_copy_count != 0U) {
      resources->synchronize();
      ++synchronization_count;
    }
    batch.record_staging_device_to_host_count = staging_copy_count;
    batch.record_staging_device_to_host_byte_count = staging_byte_count;

    batch.prune_records = batch.host_prune_records.data();
    batch.terminal_records = batch.host_terminal_records.data();
    batch.probe_receipts = batch.host_probe_receipts.data();
    batch.slot_controls = batch.host_slot_controls.data();
    // Asserted only after the three copies and their synchronisation.
    batch.record_segments_host_readable = true;
    batch.physical_product_record_capacity = product_capacity;
    batch.physical_prune_record_capacity = prune_capacity;
    batch.physical_terminal_record_capacity = terminal_capacity;
    batch.physical_probe_receipt_capacity = receipt_capacity;
    batch.physical_slot_control_capacity = request.slot_count;
    batch.physical_slot_checkpoint_capacity = request.slot_count;
    batch.physical_deferred_int512_capacity = deferred_capacity;
    batch.physical_deferred_int1024_capacity = deferred_capacity;
    batch.physical_pending_slot_count_capacity = 1U;
    std::size_t arena_bytes = checked_product(
        product_capacity,
        sizeof(Phase15HigherSupportDeviceTiledProductRecord),
        "the Phase 15 higher-support product arena bytes overflow size_t");
    arena_bytes = checked_sum(
        arena_bytes,
        checked_product(
            prune_capacity,
            sizeof(Phase15HigherSupportDeviceTiledPruneRecord),
            "the Phase 15 higher-support prune arena bytes overflow "
            "size_t"),
        "the Phase 15 higher-support arena bytes overflow size_t");
    arena_bytes = checked_sum(
        arena_bytes,
        checked_product(
            terminal_capacity,
            sizeof(Phase15HigherSupportDeviceTiledTerminalRecord),
            "the Phase 15 higher-support terminal arena bytes overflow "
            "size_t"),
        "the Phase 15 higher-support arena bytes overflow size_t");
    arena_bytes = checked_sum(
        arena_bytes,
        checked_product(
            receipt_capacity,
            sizeof(Phase15HigherSupportDeviceTiledProbeReceipt),
            "the Phase 15 higher-support receipt arena bytes overflow "
            "size_t"),
        "the Phase 15 higher-support arena bytes overflow size_t");
    arena_bytes = checked_sum(
        arena_bytes,
        checked_product(
            request.slot_count,
            sizeof(Phase15HigherSupportDeviceTiledSlotControl),
            "the Phase 15 higher-support control arena bytes overflow "
            "size_t"),
        "the Phase 15 higher-support arena bytes overflow size_t");
    arena_bytes = checked_sum(
        arena_bytes,
        checked_product(
            checked_sum(
                deferred_capacity,
                deferred_capacity,
                "the Phase 15 higher-support deferred capacity overflows "
                "size_t"),
            sizeof(Phase15HigherSupportDeviceTiledDeferredDecision),
            "the Phase 15 higher-support deferred arena bytes overflow "
            "size_t"),
        "the Phase 15 higher-support arena bytes overflow size_t");
    arena_bytes = checked_sum(
        arena_bytes,
        checked_product(
            request.slot_count,
            sizeof(Phase15HigherSupportDeviceTiledRationalDrainTask),
            "the Phase 15 higher-support rational-task arena bytes "
            "overflow size_t"),
        "the Phase 15 higher-support arena bytes overflow size_t");
    arena_bytes = checked_sum(
        arena_bytes,
        sizeof(std::uint64_t),
        "the Phase 15 higher-support rational-counter arena bytes "
        "overflow size_t");
    batch.physical_device_arena_capacity_bytes = checked_sum(
        arena_bytes,
        sizeof(std::uint64_t),
        "the Phase 15 higher-support pending-slot arena bytes overflow "
        "size_t");
    batch.slot_control_device_to_host_count = request.slot_count;
    batch.slot_control_device_to_host_byte_count = checked_product(
        request.slot_count,
        sizeof(Phase15HigherSupportDeviceTiledSlotControl),
        "the Phase 15 higher-support control transfer bytes overflow "
        "size_t");
    batch.resume_control_device_to_host_count = kernel_launch_count;
    batch.resume_control_device_to_host_byte_count = checked_product(
        kernel_launch_count,
        2U * sizeof(std::uint64_t),
        "the Phase 15 higher-support resume transfer bytes overflow "
        "size_t");
    batch.kernel_launch_count = kernel_launch_count;
    batch.synchronization_count = synchronization_count;
    batch.traversal_subdivision_count = traversal_subdivision_count;
    batch.host_rational_drain_relaunch_count =
        host_rational_drain_relaunch_count;
    batch.rational_task_device_to_host_count =
        rational_task_device_to_host_count;
    batch.rational_task_device_to_host_byte_count = checked_product(
        rational_task_device_to_host_count,
        sizeof(Phase15HigherSupportDeviceTiledRationalDrainTask),
        "the Phase 15 higher-support rational transfer bytes overflow "
        "size_t");
    batch.source_snapshot_epoch = request.source_snapshot_epoch;
    batch.record_buffer_epoch = request.record_buffer_epoch;
    batch.tile_epoch = request.tile_epoch;
    batch.chunk_sequence = request.chunk_sequence;
    batch.cuda_device = traversal.cuda_device;
    batch.execution_kind =
        Phase15HigherSupportDeviceTiledExecutionKind::cuda;
    batch.terminal_policy = request.terminal_policy;
    batch.maximum_relevant_closed_rank =
        request.maximum_relevant_closed_rank;
    batch.fixed_slot_segments_allocated = true;
    batch.output_owner_detached_for_tile_lifetime = true;
    batch.unsigned128_slot_mass_accounting_requested = true;
    batch.probe_plan_order_receipt_commit_requested = true;
    batch.substitute_probe_center_seed_policy_v1_requested = true;
    batch.closed_rank_window_receipts_sealed_to_policy =
        request.terminal_policy ==
        HigherSupportDeviceTiledFrontierTerminalPolicy::closed_rank_window;
    batch.strict_interior_carrier_receipts_sealed_to_policy =
        request.terminal_policy ==
        HigherSupportDeviceTiledFrontierTerminalPolicy::
            strict_interior_carrier_q3;
    batch.strict_interior_carrier_never_authorizes_gamma2_drop = true;
    batch.censored_slot_outputs_invalidated = true;
    batch.bigint_support_mass_transferred = false;
    batch.floating_point_decision_performed = false;
    batch.exact_higher_support_terminal_classification_native_cuda =
        false;
    batch.dense_product_fallback_performed = false;
    batch.global_product_frontier_materialized = false;
    batch.ordinary_or_higher_order_delaunay_materialized = false;
    batch.global_cell_coface_or_incidence_arena_materialized = false;
    batch.cuda_execution_contract_satisfied = true;
    batch.fresh_tile_device_arena_allocated =
        fresh_tile_device_arena_allocated;
    batch.fresh_tile_device_arena_reused = fresh_tile_device_arena_reused;
    batch.resume_same_tile = request.resume_same_tile;
    bool any_chunk_ready = false;
    for (const Phase15HigherSupportDeviceTiledSlotControl& control :
         batch.host_slot_controls) {
      any_chunk_ready = any_chunk_ready ||
          control.status ==
              static_cast<std::uint64_t>(
                  Phase15HigherSupportDeviceTiledSlotStatus::chunk_ready);
    }
    batch.capacity_yield_resumable = any_chunk_ready;
    batch.process_restart_resumable = false;
    static_cast<void>(chunk_rational_drain_hits);
    batch.metadata_digest =
        phase15_higher_support_device_tiled_metadata_digest(batch);
    return batch;
  } catch (...) {
    if (resources != nullptr) {
      static_cast<void>(cudaStreamSynchronize(resources->stream()));
    }
    throw;
  }
}

}  // namespace morsehgp3d::gpu::detail
