#include "phase15_morton_yao48_device_tiled_pair_frontier_internal.hpp"

#include "phase2b_interval.cuh"

#include <cuda_runtime.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if !defined(__CUDACC__)
#error "phase15_morton_yao48_device_tiled_pair_frontier.cu requires NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Phase 15 tiled Morton/Yao48 frontier requires CUDA 12.9"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the Phase 15 tiled Morton/Yao48 frontier"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the Phase 15 tiled Morton/Yao48 frontier"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Phase 15 tiled Morton/Yao48 frontier must contain only sm_120 code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

constexpr unsigned int kWarpSize = 32U;
constexpr unsigned int kMaximumWarpsPerBlock = 8U;
constexpr unsigned int kFullWarpMask = 0xffffffffU;
constexpr std::size_t kAxisCount = 3U;
constexpr std::uint64_t kInvalid = UINT64_MAX;
constexpr std::uint64_t kConeCount = UINT64_C(48);
constexpr std::uint64_t kAmbiguousCone = kConeCount;
constexpr std::uint64_t kCandidateFlagAmbiguousCone = UINT64_C(1) << 0U;
constexpr std::uint64_t kCandidateFlagCertifiedCone = UINT64_C(1) << 1U;
constexpr std::uint64_t kCandidateFlagBankInserted = UINT64_C(1) << 2U;
constexpr std::uint64_t kCandidateFlagBankReplaced = UINT64_C(1) << 3U;
constexpr std::uint64_t kPruneFlagClosedNonnegativeInterval =
    phase15_morton_yao48_device_tiled_prune_flag_closed_nonnegative_interval;
constexpr std::uint64_t kPruneFlagStrictPositiveInterval =
    phase15_morton_yao48_device_tiled_prune_flag_strict_positive_interval;

constexpr std::uint64_t kStatusActive = static_cast<std::uint64_t>(
    Phase15MortonYao48DeviceTiledAnchorStatus::active);
constexpr std::uint64_t kStatusChunkReady = static_cast<std::uint64_t>(
    Phase15MortonYao48DeviceTiledAnchorStatus::chunk_ready);
constexpr std::uint64_t kStatusComplete = static_cast<std::uint64_t>(
    Phase15MortonYao48DeviceTiledAnchorStatus::complete);
constexpr std::uint64_t kStatusFatal = static_cast<std::uint64_t>(
    Phase15MortonYao48DeviceTiledAnchorStatus::fatal);
constexpr std::uint64_t kYieldNone = static_cast<std::uint64_t>(
    Phase15MortonYao48DeviceTiledYieldReason::none);
constexpr std::uint64_t kYieldCandidates = static_cast<std::uint64_t>(
    Phase15MortonYao48DeviceTiledYieldReason::candidate_segment_full);
constexpr std::uint64_t kYieldPrunes = static_cast<std::uint64_t>(
    Phase15MortonYao48DeviceTiledYieldReason::prune_segment_full);
constexpr std::uint64_t kStopNone = static_cast<std::uint64_t>(
    MortonYao48DeviceTiledPairFrontierStopReason::none);
constexpr std::uint64_t kStopNodes = static_cast<std::uint64_t>(
    MortonYao48DeviceTiledPairFrontierStopReason::node_visit_capacity);
constexpr std::uint64_t kStopFatal = static_cast<std::uint64_t>(
    MortonYao48DeviceTiledPairFrontierStopReason::fatal_failure);
constexpr std::uint64_t kFailureNone = static_cast<std::uint64_t>(
    Phase15MortonYao48DeviceTiledFailureCode::none);
constexpr std::uint64_t kFailureMalformed = static_cast<std::uint64_t>(
    Phase15MortonYao48DeviceTiledFailureCode::malformed_traversal);
constexpr std::uint64_t kFailureOverflow = static_cast<std::uint64_t>(
    Phase15MortonYao48DeviceTiledFailureCode::arithmetic_overflow);
constexpr std::uint64_t kFailureInvariant = static_cast<std::uint64_t>(
    Phase15MortonYao48DeviceTiledFailureCode::internal_invariant);

static_assert(
    morton_yao48_device_tiled_pair_frontier_node_visits_per_anchor ==
    2048U);
static_assert(
    morton_yao48_device_tiled_pair_frontier_candidates_per_anchor ==
    640U);
static_assert(
    morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor ==
    2048U);
static_assert(
    morton_yao48_device_tiled_pair_frontier_witness_bank_count == 48U);
static_assert(
    morton_yao48_device_tiled_pair_frontier_maximum_closed_rank == 11U);

// Private ABI retained by MortonLbvhDeviceTraversalLease.  The frontier never
// owns or copies the O(n) certified node arena.
struct Phase15MortonYao48DeviceTiledNode {
  std::uint64_t lower_point_ids[3];
  std::uint64_t upper_point_ids[3];
  std::uint64_t left_child;
  std::uint64_t right_child;
  std::uint64_t leaf_begin;
  std::uint64_t leaf_end;
};

// Register-resident view of one node from the already-certified traversal
// lease.  Bounds are resolved from their extremum PointIds exactly once per
// warp and per visited node; witness tests therefore never repeat the six
// indirect coordinate loads.  Structural certification belongs to the LBVH
// lease and is deliberately not replayed for every (anchor, node) pair.
struct Phase15MortonYao48DeviceTiledHotNode {
  std::uint64_t lower_bits[3];
  std::uint64_t upper_bits[3];
  std::uint64_t right_child;
  std::uint64_t leaf_begin;
  std::uint64_t leaf_end;
};

static_assert(std::is_standard_layout_v<Phase15MortonYao48DeviceTiledNode>);
static_assert(
    std::is_trivially_copyable_v<Phase15MortonYao48DeviceTiledNode>);
static_assert(sizeof(Phase15MortonYao48DeviceTiledNode) == 80U);
static_assert(alignof(Phase15MortonYao48DeviceTiledNode) == 8U);
static_assert(
    offsetof(Phase15MortonYao48DeviceTiledNode, lower_point_ids) == 0U);
static_assert(
    offsetof(Phase15MortonYao48DeviceTiledNode, upper_point_ids) == 24U);
static_assert(
    offsetof(Phase15MortonYao48DeviceTiledNode, left_child) == 48U);
static_assert(
    offsetof(Phase15MortonYao48DeviceTiledNode, right_child) == 56U);
static_assert(
    offsetof(Phase15MortonYao48DeviceTiledNode, leaf_begin) == 64U);
static_assert(
    offsetof(Phase15MortonYao48DeviceTiledNode, leaf_end) == 72U);

class Phase15MortonYao48DeviceTiledCudaFailure final
    : public std::runtime_error {
 public:
  Phase15MortonYao48DeviceTiledCudaFailure(
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
    throw Phase15MortonYao48DeviceTiledCudaFailure(
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

[[nodiscard]] std::size_t checked_node_count(std::size_t point_count) {
  if (point_count == 0U ||
      point_count > std::numeric_limits<std::size_t>::max() / 2U + 1U) {
    throw std::length_error(
        "the Phase 15 tiled Morton/Yao48 node count overflows size_t");
  }
  return 2U * point_count - 1U;
}

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value,
    const char* message) {
  if (!std::in_range<std::uint64_t>(value)) {
    throw std::length_error(message);
  }
  return static_cast<std::uint64_t>(value);
}

template <typename Value>
class DeviceBuffer final {
 public:
  DeviceBuffer() = default;
  DeviceBuffer(const DeviceBuffer&) = delete;
  DeviceBuffer& operator=(const DeviceBuffer&) = delete;

  DeviceBuffer(DeviceBuffer&& other) noexcept
      : data_(std::exchange(other.data_, nullptr)),
        count_(std::exchange(other.count_, 0U)) {}

  DeviceBuffer& operator=(DeviceBuffer&& other) noexcept {
    if (this != &other) {
      reset();
      data_ = std::exchange(other.data_, nullptr);
      count_ = std::exchange(other.count_, 0U);
    }
    return *this;
  }

  ~DeviceBuffer() { reset(); }

  void allocate(std::size_t count, const char* operation) {
    if (data_ != nullptr || count == 0U ||
        count > std::numeric_limits<std::size_t>::max() / sizeof(Value)) {
      throw std::length_error(
          "a Phase 15 tiled Morton/Yao48 device allocation is invalid");
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
        "cudaGetDevice before Phase 15 tiled Morton/Yao48 frontier");
    if (previous_device_ != target_device) {
      check_cuda(
          cudaSetDevice(target_device),
          "cudaSetDevice for Phase 15 tiled Morton/Yao48 frontier");
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
          "cudaSetDevice restore after Phase 15 tiled Morton/Yao48 frontier");
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

void require_device_pointer(
    const void* pointer,
    int expected_device,
    std::size_t alignment,
    const char* label) {
  if (pointer == nullptr ||
      reinterpret_cast<std::uintptr_t>(pointer) % alignment != 0U) {
    throw std::invalid_argument(
        std::string{"the Phase 15 tiled Morton/Yao48 "} + label +
        " pointer is null or misaligned");
  }
  cudaPointerAttributes attributes{};
  check_cuda(
      cudaPointerGetAttributes(&attributes, pointer),
      std::string{
          "cudaPointerGetAttributes for Phase 15 tiled Morton/Yao48 "} +
          label);
  if (attributes.type != cudaMemoryTypeDevice ||
      attributes.device != expected_device) {
    throw std::invalid_argument(
        std::string{"the Phase 15 tiled Morton/Yao48 "} + label +
        " pointer is not owned by the traversal CUDA device");
  }
}

class Phase15MortonYao48DeviceTiledCudaResources final {
 public:
  Phase15MortonYao48DeviceTiledCudaResources(
      const Phase15MortonYao48DeviceTiledAdoptedTraversal& traversal,
      const Phase15MortonYao48DeviceTiledRequest& request,
      std::size_t candidate_capacity,
      std::size_t prune_capacity,
      std::size_t witness_capacity,
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
        anchor_begin_(request.anchor_begin),
        anchor_count_(request.anchor_count),
        maximum_closed_rank_(request.maximum_closed_rank),
        prune_semantics_(request.prune_semantics),
        required_witness_count_(request.required_witness_count) {
    cudaStream_t created_stream = nullptr;
    check_cuda(
        cudaStreamCreateWithFlags(&created_stream, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags for Phase 15 tiled Morton/Yao48 tile");
    try {
      candidates_.allocate(
          candidate_capacity,
          "cudaMalloc Phase 15 tiled Morton/Yao48 candidate segments");
      prunes_.allocate(
          prune_capacity,
          "cudaMalloc Phase 15 tiled Morton/Yao48 prune segments");
      witnesses_.allocate(
          witness_capacity,
          "cudaMalloc Phase 15 tiled Morton/Yao48 witness banks");
      controls_.allocate(
          control_capacity,
          "cudaMalloc Phase 15 tiled Morton/Yao48 anchor controls");
      checkpoints_.allocate(
          checkpoint_capacity,
          "cudaMalloc Phase 15 tiled Morton/Yao48 anchor checkpoints");
      pending_anchor_count_.allocate(
          1U,
          "cudaMalloc Phase 15 tiled Morton/Yao48 pending-anchor control");
    } catch (...) {
      static_cast<void>(cudaStreamDestroy(created_stream));
      throw;
    }
    stream_ = created_stream;
  }

  Phase15MortonYao48DeviceTiledCudaResources(
      const Phase15MortonYao48DeviceTiledCudaResources&) = delete;
  Phase15MortonYao48DeviceTiledCudaResources& operator=(
      const Phase15MortonYao48DeviceTiledCudaResources&) = delete;

  ~Phase15MortonYao48DeviceTiledCudaResources() {
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
  [[nodiscard]] Phase15MortonYao48DeviceTiledCandidateRecord* candidates()
      noexcept {
    return candidates_.get();
  }
  [[nodiscard]] Phase15MortonYao48DeviceTiledPruneRegionRecord* prunes()
      noexcept {
    return prunes_.get();
  }
  [[nodiscard]] Phase15MortonYao48DeviceTiledWitnessBankSlot* witnesses()
      noexcept {
    return witnesses_.get();
  }
  [[nodiscard]] Phase15MortonYao48DeviceTiledAnchorControl* controls()
      noexcept {
    return controls_.get();
  }
  [[nodiscard]] Phase15MortonYao48DeviceTiledAnchorCheckpoint* checkpoints()
      noexcept {
    return checkpoints_.get();
  }
  [[nodiscard]] std::uint64_t* pending_anchor_count() noexcept {
    return pending_anchor_count_.get();
  }

  [[nodiscard]] bool matches_resume(
      const Phase15MortonYao48DeviceTiledAdoptedTraversal& traversal,
      const Phase15MortonYao48DeviceTiledRequest& request,
      std::size_t candidate_capacity,
      std::size_t prune_capacity,
      std::size_t witness_capacity,
      std::size_t control_capacity,
      std::size_t checkpoint_capacity) const noexcept {
    return request.resume_same_tile && device_ == traversal.cuda_device &&
           traversal_owner_.get() == traversal.retained_owner.get() &&
           source_cloud_identity_.get() ==
               traversal.source_cloud_identity.get() &&
           device_coordinate_bits_ == traversal.device_coordinate_bits &&
           device_morton_point_ids_ ==
               traversal.device_morton_point_ids &&
           device_nodes_ == traversal.device_nodes &&
           source_snapshot_epoch_ == request.source_snapshot_epoch &&
           tile_epoch_ == request.tile_epoch &&
           chunk_sequence_ != UINT64_MAX &&
           request.chunk_sequence == chunk_sequence_ + UINT64_C(1) &&
           anchor_begin_ == request.anchor_begin &&
           anchor_count_ == request.anchor_count &&
           maximum_closed_rank_ == request.maximum_closed_rank &&
           prune_semantics_ == request.prune_semantics &&
           required_witness_count_ == request.required_witness_count &&
           candidates_.count() == candidate_capacity &&
           prunes_.count() == prune_capacity &&
           witnesses_.count() == witness_capacity &&
           controls_.count() == control_capacity &&
           checkpoints_.count() == checkpoint_capacity &&
           pending_anchor_count_.count() == 1U;
  }

  [[nodiscard]] bool can_rebind_fresh_tile(
      const Phase15MortonYao48DeviceTiledAdoptedTraversal& traversal,
      const Phase15MortonYao48DeviceTiledRequest& request,
      std::size_t candidate_capacity,
      std::size_t prune_capacity,
      std::size_t witness_capacity,
      std::size_t control_capacity,
      std::size_t checkpoint_capacity) const noexcept {
    return !request.resume_same_tile &&
           request.chunk_sequence == UINT64_C(1) &&
           request.tile_epoch != tile_epoch_ &&
           request.anchor_begin != anchor_begin_ &&
           device_ == traversal.cuda_device &&
           traversal_owner_.get() == traversal.retained_owner.get() &&
           source_cloud_identity_.get() ==
               traversal.source_cloud_identity.get() &&
           device_coordinate_bits_ == traversal.device_coordinate_bits &&
           device_morton_point_ids_ ==
               traversal.device_morton_point_ids &&
           device_nodes_ == traversal.device_nodes &&
           source_snapshot_epoch_ == request.source_snapshot_epoch &&
           maximum_closed_rank_ == request.maximum_closed_rank &&
           prune_semantics_ == request.prune_semantics &&
           required_witness_count_ == request.required_witness_count &&
           candidates_.count() == candidate_capacity &&
           prunes_.count() == prune_capacity &&
           witnesses_.count() == witness_capacity &&
           controls_.count() == control_capacity &&
           checkpoints_.count() == checkpoint_capacity &&
           pending_anchor_count_.count() == 1U;
  }

  void rebind_fresh_tile(
      const Phase15MortonYao48DeviceTiledRequest& request) noexcept {
    tile_epoch_ = request.tile_epoch;
    chunk_sequence_ = request.chunk_sequence;
    anchor_begin_ = request.anchor_begin;
    anchor_count_ = request.anchor_count;
  }

  void commit_chunk_sequence(std::uint64_t chunk_sequence) noexcept {
    chunk_sequence_ = chunk_sequence;
  }

  void synchronize() {
    check_cuda(
        cudaStreamSynchronize(stream_),
        "cudaStreamSynchronize Phase 15 tiled Morton/Yao48 frontier");
  }

  void synchronize_after_failure() noexcept {
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
  }

 private:
  void abandon_all() noexcept {
    pending_anchor_count_.abandon();
    checkpoints_.abandon();
    controls_.abandon();
    witnesses_.abandon();
    prunes_.abandon();
    candidates_.abandon();
  }

  void reset_all() noexcept {
    pending_anchor_count_.reset();
    checkpoints_.reset();
    controls_.reset();
    witnesses_.reset();
    prunes_.reset();
    candidates_.reset();
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
  std::size_t anchor_begin_{};
  std::size_t anchor_count_{};
  std::size_t maximum_closed_rank_{};
  MortonYao48DeviceTiledPairFrontierPruneSemantics prune_semantics_{
      MortonYao48DeviceTiledPairFrontierPruneSemantics::closed_rank_window};
  std::size_t required_witness_count_{};
  DeviceBuffer<Phase15MortonYao48DeviceTiledCandidateRecord> candidates_;
  DeviceBuffer<Phase15MortonYao48DeviceTiledPruneRegionRecord> prunes_;
  DeviceBuffer<Phase15MortonYao48DeviceTiledWitnessBankSlot> witnesses_;
  DeviceBuffer<Phase15MortonYao48DeviceTiledAnchorControl> controls_;
  DeviceBuffer<Phase15MortonYao48DeviceTiledAnchorCheckpoint> checkpoints_;
  DeviceBuffer<std::uint64_t> pending_anchor_count_;
};

void validate_launch(
    const Phase15MortonYao48DeviceTiledAdoptedTraversal& traversal,
    const Phase15MortonYao48DeviceTiledRequest& request) {
  const std::size_t node_count = checked_node_count(traversal.point_count);
  const std::size_t coordinate_word_count = checked_product(
      traversal.point_count,
      kAxisCount,
      "the Phase 15 tiled Morton/Yao48 coordinate extent overflows size_t");
  if (!traversal.retained_owner || !traversal.source_cloud_identity ||
      traversal.execution_kind !=
          Phase15MortonYao48DeviceTiledExecutionKind::cuda ||
      traversal.host_fake_lifecycle_exercised ||
      !traversal.cuda_device_storage_retained ||
      !traversal.canonical_coordinate_words_retained ||
      !traversal.active_morton_point_ids_retained ||
      !traversal.certified_device_nodes_retained ||
      traversal.cuda_device < 0 ||
      traversal.device_coordinate_bits == nullptr ||
      traversal.device_morton_point_ids == nullptr ||
      traversal.device_nodes == nullptr || traversal.point_count == 0U ||
      traversal.certified_node_count != node_count ||
      traversal.maximum_point_count < traversal.point_count ||
      traversal.maximum_node_count < traversal.certified_node_count ||
      traversal.retained_coordinate_word_capacity < coordinate_word_count ||
      traversal.retained_morton_point_id_capacity < traversal.point_count ||
      traversal.retained_node_capacity < traversal.certified_node_count ||
      traversal.source_snapshot_epoch == 0U ||
      request.source_snapshot_epoch != traversal.source_snapshot_epoch ||
      request.output_buffer_epoch == 0U ||
      request.tile_epoch == 0U ||
      (request.resume_same_tile
           ? request.chunk_sequence <= UINT64_C(1)
           : request.chunk_sequence != UINT64_C(1)) ||
      request.point_count != traversal.point_count ||
      request.certified_node_count != traversal.certified_node_count ||
      request.anchor_count == 0U ||
      request.anchor_count >
          morton_yao48_device_tiled_pair_frontier_maximum_anchor_tile_capacity ||
      request.anchor_begin > request.point_count ||
      request.anchor_count > request.point_count - request.anchor_begin ||
      request.maximum_closed_rank < 2U ||
      request.maximum_closed_rank >
          morton_yao48_device_tiled_pair_frontier_maximum_closed_rank ||
      !morton_yao48_device_tiled_pair_frontier_prune_semantics_known(
          request.prune_semantics) ||
      request.required_witness_count !=
          morton_yao48_device_tiled_pair_frontier_required_witness_count(
              request.prune_semantics, request.maximum_closed_rank) ||
      request.node_visit_capacity_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_node_visits_per_anchor ||
      request.candidate_capacity_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_candidates_per_anchor ||
      request.prune_region_capacity_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor ||
      request.witness_bank_count_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_witness_bank_count ||
      request.witness_slot_count_per_bank !=
          request.required_witness_count) {
    throw std::invalid_argument(
        "the Phase 15 tiled Morton/Yao48 CUDA launch has invalid ownership, "
        "identity, extents or fixed capacities");
  }

  static_cast<void>(checked_product(
      request.anchor_count,
      request.candidate_capacity_per_anchor,
      "the Phase 15 tiled Morton/Yao48 candidate capacity overflows size_t"));
  static_cast<void>(checked_product(
      request.anchor_count,
      request.prune_region_capacity_per_anchor,
      "the Phase 15 tiled Morton/Yao48 prune capacity overflows size_t"));
  static_cast<void>(checked_product(
      checked_product(
          request.anchor_count,
          request.witness_bank_count_per_anchor,
          "the Phase 15 tiled Morton/Yao48 bank capacity overflows size_t"),
      request.witness_slot_count_per_bank,
      "the Phase 15 tiled Morton/Yao48 witness capacity overflows size_t"));
}

struct Phase15MortonYao48DeviceTiledLaunchShape {
  unsigned int block_count{};
  unsigned int thread_count{};
};

[[nodiscard]] Phase15MortonYao48DeviceTiledLaunchShape launch_shape(
    std::size_t anchor_count,
    const cudaDeviceProp& properties) {
  if (properties.major != 12 || properties.minor != 0 ||
      properties.warpSize != static_cast<int>(kWarpSize) ||
      properties.maxGridSize[0] <= 0 || properties.multiProcessorCount <= 0 ||
      properties.maxThreadsPerBlock <
          static_cast<int>(kMaximumWarpsPerBlock * kWarpSize)) {
    throw std::runtime_error(
        "the Phase 15 tiled Morton/Yao48 frontier requires an sm_120 CUDA "
        "device with 32-lane warps");
  }
  const std::size_t anchors_per_multiprocessor =
      anchor_count / static_cast<std::size_t>(properties.multiProcessorCount);
  unsigned int warps_per_block = 1U;
  if (anchors_per_multiprocessor >= 8U) {
    warps_per_block = 8U;
  } else if (anchors_per_multiprocessor >= 4U) {
    warps_per_block = 4U;
  } else if (anchors_per_multiprocessor >= 2U) {
    warps_per_block = 2U;
  }
  const std::size_t requested =
      anchor_count / warps_per_block +
      (anchor_count % warps_per_block == 0U ? 0U : 1U);
  const std::size_t bounded = std::min(
      requested, static_cast<std::size_t>(properties.maxGridSize[0]));
  if (bounded == 0U ||
      bounded >
          static_cast<std::size_t>(
              std::numeric_limits<unsigned int>::max())) {
    throw std::runtime_error(
        "the Phase 15 tiled Morton/Yao48 warp grid is invalid");
  }
  return Phase15MortonYao48DeviceTiledLaunchShape{
      static_cast<unsigned int>(bounded), warps_per_block * kWarpSize};
}

[[nodiscard]] __device__ std::uint64_t warp_broadcast_u64(
    std::uint64_t value,
    int source_lane) noexcept {
  const auto low = static_cast<unsigned int>(value & UINT64_C(0xffffffff));
  const auto high = static_cast<unsigned int>(value >> 32U);
  return static_cast<std::uint64_t>(
             __shfl_sync(kFullWarpMask, high, source_lane))
             << 32U |
         static_cast<std::uint64_t>(
             __shfl_sync(kFullWarpMask, low, source_lane));
}

// Returns -1, 0 or +1 for a certified sign and 2 for an interval ambiguity.
[[nodiscard]] __device__ int certified_difference_sign(
    std::uint64_t target_bits,
    std::uint64_t source_bits,
    device::DeviceInterval& difference) noexcept {
  const device::DeviceInterval target = device::point_interval(target_bits);
  const device::DeviceInterval source = device::point_interval(source_bits);
  if (!target.valid || !source.valid) {
    difference = device::invalid_interval();
    return 2;
  }
  if (target.lower == source.lower) {
    difference = device::point_interval(UINT64_C(0));
    return 0;
  }
  difference = device::subtract_intervals(target, source);
  if (!difference.valid) {
    return 2;
  }
  if (difference.lower > 0.0) {
    return 1;
  }
  if (difference.upper < 0.0) {
    return -1;
  }
  return 2;
}

[[nodiscard]] __device__ bool exact_interval_equal(
    const device::DeviceInterval& left,
    const device::DeviceInterval& right) noexcept {
  return left.valid && right.valid && left.lower == left.upper &&
         right.lower == right.upper && left.lower == right.lower;
}

[[nodiscard]] __device__ bool descending_axis_order_certified(
    const device::DeviceInterval absolute_deltas[3],
    unsigned int left_axis,
    unsigned int right_axis) noexcept {
  const device::DeviceInterval& left = absolute_deltas[left_axis];
  const device::DeviceInterval& right = absolute_deltas[right_axis];
  if (!left.valid || !right.valid) {
    return false;
  }
  if (left.lower > right.upper) {
    return true;
  }
  return exact_interval_equal(left, right) && left_axis < right_axis;
}

[[nodiscard]] __device__ unsigned int permutation_axis(
    unsigned int permutation,
    unsigned int position) noexcept {
  constexpr unsigned int permutations[6][3]{
      {0U, 1U, 2U},
      {0U, 2U, 1U},
      {1U, 0U, 2U},
      {1U, 2U, 0U},
      {2U, 0U, 1U},
      {2U, 1U, 0U},
  };
  return permutations[permutation][position];
}

struct Phase15MortonYao48DeviceTiledConeClassification {
  std::uint64_t cone_index{kAmbiguousCone};
  bool ambiguous{true};
  bool malformed{false};
};

[[nodiscard]] __device__ Phase15MortonYao48DeviceTiledConeClassification
classify_yao48_cone_by_intervals(
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    std::uint64_t anchor_point_id,
    std::uint64_t target_point_id) noexcept {
  Phase15MortonYao48DeviceTiledConeClassification result;
  device::DeviceInterval absolute_deltas[3]{};
  std::uint64_t negative_axis_mask = 0U;
  bool nonzero = false;
  bool ambiguous = false;
  for (unsigned int axis = 0U; axis < kAxisCount; ++axis) {
    const std::uint64_t anchor_bits =
        coordinate_bits[static_cast<std::uint64_t>(axis) * point_count +
                        anchor_point_id];
    const std::uint64_t target_bits =
        coordinate_bits[static_cast<std::uint64_t>(axis) * point_count +
                        target_point_id];
    if (!device::point_interval(anchor_bits).valid ||
        !device::point_interval(target_bits).valid) {
      result.malformed = true;
      return result;
    }
    device::DeviceInterval delta;
    const int sign = certified_difference_sign(
        target_bits, anchor_bits, delta);
    if (sign == 2) {
      ambiguous = true;
      continue;
    }
    if (sign == 0) {
      absolute_deltas[axis] = device::point_interval(UINT64_C(0));
      continue;
    }
    nonzero = true;
    if (sign < 0) {
      negative_axis_mask |= UINT64_C(1) << axis;
      absolute_deltas[axis] = device::subtract_intervals(
          device::point_interval(UINT64_C(0)), delta);
    } else {
      absolute_deltas[axis] = delta;
    }
    if (!absolute_deltas[axis].valid) {
      ambiguous = true;
    }
  }
  if (!nonzero && !ambiguous) {
    // Canonical clouds contain no geometrically duplicate points.
    result.malformed = true;
    return result;
  }
  if (ambiguous) {
    return result;
  }

  unsigned int matching_permutation = 6U;
  unsigned int match_count = 0U;
  for (unsigned int permutation = 0U; permutation < 6U; ++permutation) {
    const unsigned int first = permutation_axis(permutation, 0U);
    const unsigned int second = permutation_axis(permutation, 1U);
    const unsigned int third = permutation_axis(permutation, 2U);
    const bool matches = descending_axis_order_certified(
                             absolute_deltas, first, second) &&
                         descending_axis_order_certified(
                             absolute_deltas, second, third) &&
                         descending_axis_order_certified(
                             absolute_deltas, first, third);
    if (matches) {
      matching_permutation = permutation;
      ++match_count;
    }
  }
  if (match_count != 1U) {
    return result;
  }
  result.cone_index =
      negative_axis_mask * UINT64_C(6) + matching_permutation;
  result.ambiguous = result.cone_index >= kConeCount;
  if (result.ambiguous) {
    result.cone_index = kAmbiguousCone;
  }
  return result;
}

[[nodiscard]] __device__ Phase15MortonYao48DeviceTiledHotNode
load_certified_hot_node_warp(
    const Phase15MortonYao48DeviceTiledNode* nodes,
    std::uint64_t node_index,
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    unsigned int lane) noexcept {
  Phase15MortonYao48DeviceTiledHotNode hot{};
  if (lane == 0U) {
    const Phase15MortonYao48DeviceTiledNode& source = nodes[node_index];
    hot.right_child = source.right_child;
    hot.leaf_begin = source.leaf_begin;
    hot.leaf_end = source.leaf_end;
  }
  hot.right_child = warp_broadcast_u64(hot.right_child, 0);
  hot.leaf_begin = warp_broadcast_u64(hot.leaf_begin, 0);
  hot.leaf_end = warp_broadcast_u64(hot.leaf_end, 0);

  if (lane < kAxisCount) {
    const Phase15MortonYao48DeviceTiledNode& source = nodes[node_index];
    hot.lower_bits[lane] =
        coordinate_bits[static_cast<std::uint64_t>(lane) * point_count +
                        source.lower_point_ids[lane]];
    hot.upper_bits[lane] =
        coordinate_bits[static_cast<std::uint64_t>(lane) * point_count +
                        source.upper_point_ids[lane]];
  }
  for (unsigned int axis = 0U; axis < kAxisCount; ++axis) {
    hot.lower_bits[axis] =
        warp_broadcast_u64(hot.lower_bits[axis], static_cast<int>(axis));
    hot.upper_bits[axis] =
        warp_broadcast_u64(hot.upper_bits[axis], static_cast<int>(axis));
  }
  return hot;
}

[[nodiscard]] __device__ bool witness_point_certifies_node(
    std::uint64_t witness_point_id,
    const Phase15MortonYao48DeviceTiledHotNode& node,
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    std::uint64_t anchor_point_id,
    MortonYao48DeviceTiledPairFrontierPruneSemantics
        prune_semantics) noexcept {
  if (witness_point_id == kInvalid || witness_point_id >= point_count ||
      witness_point_id == anchor_point_id) {
    return false;
  }

  device::DeviceInterval sum = device::point_interval(UINT64_C(0));
  for (unsigned int axis = 0U; axis < kAxisCount; ++axis) {
    const std::uint64_t anchor_bits =
        coordinate_bits[static_cast<std::uint64_t>(axis) * point_count +
                        anchor_point_id];
    const std::uint64_t witness_bits =
        coordinate_bits[static_cast<std::uint64_t>(axis) * point_count +
                        witness_point_id];
    device::DeviceInterval direction;
    const int sign = certified_difference_sign(
        witness_bits, anchor_bits, direction);
    if (sign == 2) {
      return false;
    }
    if (sign == 0) {
      continue;
    }
    const std::uint64_t bound_bits =
        sign > 0 ? node.lower_bits[axis] : node.upper_bits[axis];
    const device::DeviceInterval delta = device::subtract_intervals(
        device::point_interval(bound_bits),
        device::point_interval(witness_bits));
    const device::DeviceInterval product =
        device::multiply_intervals(direction, delta);
    sum = device::add_intervals(sum, product);
    if (!sum.valid) {
      return false;
    }
  }
  // S=(w-a).(x-w) is -Phi.  Closed-rank pruning accepts S >= 0, whereas
  // carrier pruning requires S > 0: a shell witness must never reject the
  // q=3 carrier.
  return sum.valid &&
         phase15_morton_yao48_device_tiled_witness_lower_bound_certifies(
             sum.lower, prune_semantics);
}

[[nodiscard]] __device__ bool witness_certifies_node(
    const Phase15MortonYao48DeviceTiledWitnessBankSlot& witness,
    const Phase15MortonYao48DeviceTiledHotNode& node,
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    std::uint64_t anchor_point_id,
    std::uint64_t anchor_morton_position,
    MortonYao48DeviceTiledPairFrontierPruneSemantics
        prune_semantics) noexcept {
  return witness.witness_morton_position >= node.leaf_end &&
         witness.witness_morton_position < anchor_morton_position &&
         witness_point_certifies_node(
             witness.witness_point_id,
             node,
             coordinate_bits,
             point_count,
             anchor_point_id,
             prune_semantics);
}

struct Phase15MortonYao48DeviceTiledSelectedWitnesses {
  std::uint64_t bank_mask{};
  std::uint64_t count{};
};

// A certified prune is followed only by nodes whose Morton leaf interval is
// strictly earlier in the same reverse-postorder traversal.  Its ten witness
// PointIds therefore remain outside every subsequently visited node.  Trying
// that immutable receipt first is an exact cache: failure merely falls back
// to an exhaustive scan of all bank slots.
[[nodiscard]] __device__ Phase15MortonYao48DeviceTiledSelectedWitnesses
try_cached_certifying_witnesses_warp(
    const Phase15MortonYao48DeviceTiledPruneRegionRecord* cached,
    Phase15MortonYao48DeviceTiledPruneRegionRecord* output,
    const Phase15MortonYao48DeviceTiledHotNode& node,
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    std::uint64_t anchor_point_id,
    std::uint64_t required_witness_count,
    MortonYao48DeviceTiledPairFrontierPruneSemantics prune_semantics,
    std::uint64_t required_prune_flag,
    unsigned int lane) noexcept {
  Phase15MortonYao48DeviceTiledSelectedWitnesses selected{};
  if (cached == nullptr || required_witness_count == 0U ||
      required_witness_count > 10U || cached->flags != required_prune_flag ||
      cached->retained_witness_count != required_witness_count) {
    return selected;
  }
  std::uint64_t point_id = kInvalid;
  bool certifies = false;
  if (lane < required_witness_count) {
    point_id = cached->witness_point_ids[lane];
    certifies = witness_point_certifies_node(
        point_id,
        node,
        coordinate_bits,
        point_count,
        anchor_point_id,
        prune_semantics);
  }
  const unsigned int required_mask =
      required_witness_count == kWarpSize
          ? kFullWarpMask
          : ((1U << static_cast<unsigned int>(required_witness_count)) - 1U);
  const unsigned int certified_mask =
      __ballot_sync(kFullWarpMask, certifies) & required_mask;
  if (certified_mask != required_mask) {
    return selected;
  }
  if (lane < required_witness_count) {
    output->witness_point_ids[lane] = point_id;
  }
  if (lane == 0U) {
    for (std::uint64_t index = required_witness_count; index < 10U; ++index) {
      output->witness_point_ids[index] = UINT64_C(0);
    }
  }
  selected.bank_mask = cached->retained_witness_bank_mask;
  selected.count = required_witness_count;
  return selected;
}

[[nodiscard]] __device__ Phase15MortonYao48DeviceTiledSelectedWitnesses
select_certifying_witnesses_warp(
    const Phase15MortonYao48DeviceTiledWitnessBankSlot* witnesses,
    std::uint64_t witness_count,
    std::uint64_t witness_slots_per_bank,
    const Phase15MortonYao48DeviceTiledHotNode& node,
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    std::uint64_t anchor_point_id,
    std::uint64_t anchor_morton_position,
    std::uint64_t required_witness_count,
    MortonYao48DeviceTiledPairFrontierPruneSemantics prune_semantics,
    Phase15MortonYao48DeviceTiledPruneRegionRecord* output,
    unsigned int lane) noexcept {
  Phase15MortonYao48DeviceTiledSelectedWitnesses selected{};

  for (std::uint64_t base = 0U;
       base < witness_count && selected.count < required_witness_count;
       base += kWarpSize) {
    const std::uint64_t slot_index = base + lane;
    std::uint64_t candidate_point_id = kInvalid;
    std::uint64_t candidate_bank = 0U;
    bool certifies = false;
    if (slot_index < witness_count) {
      const Phase15MortonYao48DeviceTiledWitnessBankSlot slot =
          witnesses[slot_index];
      certifies = witness_certifies_node(
          slot,
          node,
          coordinate_bits,
          point_count,
          anchor_point_id,
          anchor_morton_position,
          prune_semantics);
      candidate_point_id = slot.witness_point_id;
      candidate_bank = slot_index / witness_slots_per_bank;
    }
    unsigned int mask = __ballot_sync(kFullWarpMask, certifies);
    while (mask != 0U && selected.count < required_witness_count) {
      const int owner_lane = __ffs(static_cast<int>(mask)) - 1;
      const std::uint64_t point_id =
          warp_broadcast_u64(candidate_point_id, owner_lane);
      const std::uint64_t bank =
          warp_broadcast_u64(candidate_bank, owner_lane);
      // Every target leaf is encountered once and has one unique certified
      // cone.  Consequently active bank slots cannot duplicate a PointId.
      if (point_id != kInvalid && bank < kConeCount) {
        if (lane == 0U) {
          output->witness_point_ids[selected.count] = point_id;
        }
        selected.bank_mask |= UINT64_C(1) << bank;
        ++selected.count;
      }
      mask &= mask - 1U;
    }
  }
  if (selected.count == required_witness_count && lane == 0U) {
    for (std::uint64_t index = required_witness_count; index < 10U; ++index) {
      output->witness_point_ids[index] = UINT64_C(0);
    }
  }
  return selected;
}

[[nodiscard]] __device__ device::DeviceInterval squared_distance_interval(
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    std::uint64_t anchor_point_id,
    std::uint64_t target_point_id) noexcept {
  device::DeviceInterval sum = device::point_interval(UINT64_C(0));
  for (unsigned int axis = 0U; axis < kAxisCount; ++axis) {
    const device::DeviceInterval delta = device::subtract_intervals(
        device::point_interval(
            coordinate_bits[
                static_cast<std::uint64_t>(axis) * point_count +
                target_point_id]),
        device::point_interval(
            coordinate_bits[
                static_cast<std::uint64_t>(axis) * point_count +
                anchor_point_id]));
    sum = device::add_intervals(sum, device::square_interval(delta));
    if (!sum.valid) {
      return device::invalid_interval();
    }
  }
  return sum;
}

[[nodiscard]] __device__ device::DeviceInterval decode_distance_interval(
    const Phase15MortonYao48DeviceTiledWitnessBankSlot& slot) noexcept {
  if (slot.squared_distance_lower_bits == kInvalid ||
      slot.squared_distance_upper_bits == kInvalid) {
    return device::invalid_interval();
  }
  const double lower = __longlong_as_double(
      static_cast<long long int>(slot.squared_distance_lower_bits));
  const double upper = __longlong_as_double(
      static_cast<long long int>(slot.squared_distance_upper_bits));
  return device::checked_interval(lower, upper);
}

struct Phase15MortonYao48DeviceTiledBankUpdate {
  bool inserted{false};
  bool replaced{false};
};

[[nodiscard]] __device__ Phase15MortonYao48DeviceTiledBankUpdate
update_witness_bank_after_candidate(
    Phase15MortonYao48DeviceTiledWitnessBankSlot* witnesses,
    std::uint64_t cone_index,
    std::uint64_t witness_slots_per_bank,
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    std::uint64_t anchor_point_id,
    std::uint64_t anchor_morton_position,
    std::uint64_t target_point_id,
    std::uint64_t target_morton_position) noexcept {
  Phase15MortonYao48DeviceTiledBankUpdate update;
  if (cone_index >= kConeCount || witness_slots_per_bank == 0U) {
    return update;
  }
  Phase15MortonYao48DeviceTiledWitnessBankSlot* bank =
      witnesses + cone_index * witness_slots_per_bank;
  std::uint64_t destination = kInvalid;
  for (std::uint64_t slot = 0U; slot < witness_slots_per_bank; ++slot) {
    if (bank[slot].witness_point_id == kInvalid) {
      destination = slot;
      break;
    }
  }

  const device::DeviceInterval distance = squared_distance_interval(
      coordinate_bits,
      point_count,
      anchor_point_id,
      target_point_id);
  if (destination == kInvalid && distance.valid) {
    double greatest_replaced_lower = -1.0;
    for (std::uint64_t slot = 0U; slot < witness_slots_per_bank; ++slot) {
      const device::DeviceInterval retained =
          decode_distance_interval(bank[slot]);
      // Replace only when the new candidate is strictly closer under
      // directed intervals.  Overlap is deliberately retained unchanged.
      if (retained.valid && distance.upper < retained.lower &&
          (destination == kInvalid ||
           retained.lower > greatest_replaced_lower)) {
        destination = slot;
        greatest_replaced_lower = retained.lower;
      }
    }
    update.replaced = destination != kInvalid;
  }
  if (destination == kInvalid) {
    return update;
  }

  Phase15MortonYao48DeviceTiledWitnessBankSlot stored;
  stored.witness_point_id = target_point_id;
  stored.witness_morton_position = target_morton_position;
  if (distance.valid) {
    stored.squared_distance_lower_bits = static_cast<std::uint64_t>(
        __double_as_longlong(distance.lower));
    stored.squared_distance_upper_bits = static_cast<std::uint64_t>(
        __double_as_longlong(distance.upper));
  } else {
    stored.squared_distance_lower_bits = kInvalid;
    stored.squared_distance_upper_bits = kInvalid;
  }
  bank[destination] = stored;
  update.inserted = true;
  return update;
}

[[nodiscard]] __device__ bool finish_or_skip_subtree(
    std::uint64_t width,
    std::uint64_t& cursor,
    bool& traversal_complete) noexcept {
  if (width == 0U ||
      width > UINT64_MAX / UINT64_C(2) + UINT64_C(1)) {
    return false;
  }
  const std::uint64_t subtree_node_count =
      UINT64_C(2) * width - UINT64_C(1);
  if (subtree_node_count > cursor + UINT64_C(1)) {
    return false;
  }
  traversal_complete = subtree_node_count == cursor + UINT64_C(1);
  if (!traversal_complete) {
    cursor -= subtree_node_count;
  }
  return true;
}

__device__ void invalidate_anchor_segments_warp(
    Phase15MortonYao48DeviceTiledCandidateRecord* candidates,
    std::uint64_t candidate_capacity,
    Phase15MortonYao48DeviceTiledPruneRegionRecord* prunes,
    std::uint64_t prune_capacity,
    unsigned int lane) noexcept {
  for (std::uint64_t index = lane;
       index < candidate_capacity;
       index += kWarpSize) {
    Phase15MortonYao48DeviceTiledCandidateRecord invalid;
    invalid.support_u = kInvalid;
    invalid.support_v = kInvalid;
    invalid.anchor_morton_position = kInvalid;
    invalid.partner_morton_position = kInvalid;
    invalid.owner_cone_index = kInvalid;
    invalid.flags = kInvalid;
    candidates[index] = invalid;
  }
  for (std::uint64_t index = lane;
       index < prune_capacity;
       index += kWarpSize) {
    Phase15MortonYao48DeviceTiledPruneRegionRecord invalid;
    invalid.anchor_morton_position = kInvalid;
    invalid.node_index = kInvalid;
    invalid.certified_pair_mass = kInvalid;
    invalid.retained_witness_count = kInvalid;
    invalid.retained_witness_bank_mask = kInvalid;
    invalid.flags = kInvalid;
    for (unsigned int witness_index = 0U;
         witness_index < 10U;
         ++witness_index) {
      invalid.witness_point_ids[witness_index] = kInvalid;
    }
    prunes[index] = invalid;
  }
}

__global__ void build_tiled_morton_yao48_pair_frontier_kernel(
    const std::uint64_t* coordinate_bits,
    const std::uint64_t* morton_point_ids,
    const Phase15MortonYao48DeviceTiledNode* nodes,
    std::uint64_t point_count,
    std::uint64_t node_count,
    std::uint64_t anchor_begin,
    std::uint64_t anchor_count,
    std::uint64_t maximum_closed_rank,
    std::uint64_t prune_semantics_raw,
    std::uint64_t required_witness_count,
    std::uint64_t node_visit_capacity,
    std::uint64_t candidate_capacity,
    std::uint64_t prune_capacity,
    std::uint64_t witness_bank_count,
    std::uint64_t witness_slots_per_bank,
    std::uint64_t subdivision_index,
    std::uint64_t maximum_subdivision_count,
    std::uint64_t resume_same_tile,
    std::uint64_t tile_epoch,
    std::uint64_t chunk_sequence,
    Phase15MortonYao48DeviceTiledCandidateRecord* candidates,
    Phase15MortonYao48DeviceTiledPruneRegionRecord* prunes,
    Phase15MortonYao48DeviceTiledWitnessBankSlot* witnesses,
    Phase15MortonYao48DeviceTiledAnchorControl* controls,
    Phase15MortonYao48DeviceTiledAnchorCheckpoint* checkpoints,
    std::uint64_t* pending_anchor_count) {
  const unsigned int lane = threadIdx.x & (kWarpSize - 1U);
  const std::uint64_t block_warp = threadIdx.x / kWarpSize;
  const std::uint64_t block_warp_count = blockDim.x / kWarpSize;
  std::uint64_t anchor_slot =
      static_cast<std::uint64_t>(blockIdx.x) * block_warp_count +
      block_warp;
  const std::uint64_t anchor_stride =
      static_cast<std::uint64_t>(gridDim.x) * block_warp_count;

  while (anchor_slot < anchor_count) {
    const std::uint64_t anchor_morton_position =
        anchor_begin + anchor_slot;
    Phase15MortonYao48DeviceTiledCandidateRecord* anchor_candidates =
        candidates + anchor_slot * candidate_capacity;
    Phase15MortonYao48DeviceTiledPruneRegionRecord* anchor_prunes =
        prunes + anchor_slot * prune_capacity;
    const std::uint64_t witness_capacity =
        witness_bank_count * witness_slots_per_bank;
    Phase15MortonYao48DeviceTiledWitnessBankSlot* anchor_witnesses =
        witnesses + anchor_slot * witness_capacity;
    Phase15MortonYao48DeviceTiledAnchorCheckpoint* checkpoint =
        checkpoints + anchor_slot;
    if (subdivision_index != 0U &&
        (checkpoint->state == kStatusChunkReady ||
         checkpoint->state == kStatusComplete ||
         checkpoint->state == kStatusFatal)) {
      anchor_slot += anchor_stride;
      continue;
    }

    const bool first_subdivision = subdivision_index == 0U;
    const bool resumed_first_subdivision =
        first_subdivision && resume_same_tile != UINT64_C(0);
    const bool load_checkpoint =
        resumed_first_subdivision || !first_subdivision;

    std::uint64_t cursor = node_count - UINT64_C(1);
    std::uint64_t candidate_count = 0U;
    std::uint64_t prune_count = 0U;
    std::uint64_t certified_pruned_mass = 0U;
    std::uint64_t node_visit_count = 0U;
    std::uint64_t ambiguous_candidate_count = 0U;
    std::uint64_t unbanked_candidate_count = 0U;
    std::uint64_t cumulative_candidate_count = 0U;
    std::uint64_t cumulative_prune_count = 0U;
    std::uint64_t cumulative_certified_pruned_mass = 0U;
    std::uint64_t cumulative_node_visit_count = 0U;
    std::uint64_t cumulative_ambiguous_candidate_count = 0U;
    std::uint64_t cumulative_unbanked_candidate_count = 0U;
    std::uint64_t retained_witness_count = 0U;
    std::uint64_t completed_subdivision_count = 0U;
    std::uint64_t prior_state = kStatusActive;
    if (load_checkpoint) {
      cursor = checkpoint->cursor;
      candidate_count = checkpoint->candidate_count;
      prune_count = checkpoint->prune_region_count;
      certified_pruned_mass = checkpoint->certified_pruned_pair_mass;
      node_visit_count = checkpoint->node_visit_count;
      ambiguous_candidate_count =
          checkpoint->ambiguous_cone_candidate_count;
      unbanked_candidate_count = checkpoint->unbanked_candidate_count;
      cumulative_candidate_count =
          checkpoint->cumulative_candidate_count;
      cumulative_prune_count =
          checkpoint->cumulative_prune_region_count;
      cumulative_certified_pruned_mass =
          checkpoint->cumulative_certified_pruned_pair_mass;
      cumulative_node_visit_count =
          checkpoint->cumulative_node_visit_count;
      cumulative_ambiguous_candidate_count =
          checkpoint->cumulative_ambiguous_cone_candidate_count;
      cumulative_unbanked_candidate_count =
          checkpoint->cumulative_unbanked_candidate_count;
      retained_witness_count = checkpoint->retained_witness_count;
      completed_subdivision_count =
          checkpoint->completed_subdivision_count;
      prior_state = checkpoint->state;
    }
    std::uint64_t subdivision_node_visit_count = 0U;
    std::uint64_t yield_reason = kYieldNone;
    std::uint64_t stop_reason = kStopNone;
    std::uint64_t failure_code = kFailureNone;
    bool complete = false;
    bool fatal = false;
    bool paused = false;
    bool chunk_ready = false;

    const std::uint64_t expected_maximum_subdivision_count =
        node_count / UINT64_C(2048) +
        (node_count % UINT64_C(2048) == 0U ? UINT64_C(0)
                                           : UINT64_C(1));
    const bool closed_rank_semantics =
        prune_semantics_raw == static_cast<std::uint64_t>(
                                   MortonYao48DeviceTiledPairFrontierPruneSemantics::
                                       closed_rank_window);
    const bool strict_interior_semantics =
        prune_semantics_raw == static_cast<std::uint64_t>(
                                   MortonYao48DeviceTiledPairFrontierPruneSemantics::
                                       strict_interior_threshold);
    const std::uint64_t expected_required_witness_count =
        closed_rank_semantics
            ? maximum_closed_rank - UINT64_C(1)
            : UINT64_C(2);
    const bool fixed_contract_valid =
        maximum_closed_rank >= UINT64_C(2) &&
        maximum_closed_rank <= UINT64_C(11) &&
        (closed_rank_semantics || strict_interior_semantics) &&
        required_witness_count == expected_required_witness_count &&
        node_visit_capacity == UINT64_C(2048) &&
        candidate_capacity == UINT64_C(640) &&
        prune_capacity == node_visit_capacity &&
        witness_bank_count == kConeCount &&
        witness_slots_per_bank == required_witness_count &&
        subdivision_index < maximum_subdivision_count &&
        maximum_subdivision_count ==
            expected_maximum_subdivision_count &&
        resume_same_tile <= UINT64_C(1) && tile_epoch != UINT64_C(0) &&
        (resume_same_tile != UINT64_C(0)
             ? chunk_sequence > UINT64_C(1)
             : chunk_sequence == UINT64_C(1)) &&
        anchor_morton_position < point_count &&
        node_count == UINT64_C(2) * point_count - UINT64_C(1);
    if (!__all_sync(kFullWarpMask, fixed_contract_valid)) {
      failure_code = kFailureInvariant;
      fatal = true;
    }
    const auto prune_semantics =
        static_cast<MortonYao48DeviceTiledPairFrontierPruneSemantics>(
            prune_semantics_raw);
    const std::uint64_t required_prune_flag =
        strict_interior_semantics ? kPruneFlagStrictPositiveInterval
                                  : kPruneFlagClosedNonnegativeInterval;

    if (!fatal && load_checkpoint) {
      const bool state_valid = resumed_first_subdivision
                                   ? (prior_state == kStatusChunkReady ||
                                      prior_state == kStatusComplete)
                                   : prior_state == kStatusActive;
      const bool sequence_valid =
          resumed_first_subdivision
              ? checkpoint->chunk_sequence != UINT64_MAX &&
                    checkpoint->chunk_sequence + UINT64_C(1) ==
                        chunk_sequence
              : checkpoint->chunk_sequence == chunk_sequence;
      const bool checkpoint_valid =
          checkpoint->reserved_zero == UINT64_C(0) &&
          checkpoint->anchor_morton_position == anchor_morton_position &&
          checkpoint->tile_epoch == tile_epoch && sequence_valid &&
          state_valid && cursor < node_count &&
          candidate_count <= candidate_capacity &&
          prune_count <= prune_capacity &&
          candidate_count <= cumulative_candidate_count &&
          prune_count <= cumulative_prune_count &&
          certified_pruned_mass <= cumulative_certified_pruned_mass &&
          node_visit_count <= cumulative_node_visit_count &&
          ambiguous_candidate_count <= unbanked_candidate_count &&
          unbanked_candidate_count <= candidate_count &&
          cumulative_ambiguous_candidate_count <=
              cumulative_unbanked_candidate_count &&
          cumulative_unbanked_candidate_count <=
              cumulative_candidate_count &&
          cumulative_candidate_count <= anchor_morton_position &&
          cumulative_certified_pruned_mass <=
              anchor_morton_position - cumulative_candidate_count &&
          cumulative_node_visit_count <= node_count &&
          retained_witness_count <= witness_capacity;
      if (!__all_sync(kFullWarpMask, checkpoint_valid)) {
        failure_code = kFailureInvariant;
        fatal = true;
      }
    }

    if (!fatal && resumed_first_subdivision) {
      candidate_count = 0U;
      prune_count = 0U;
      certified_pruned_mass = 0U;
      node_visit_count = 0U;
      ambiguous_candidate_count = 0U;
      unbanked_candidate_count = 0U;
      if (prior_state == kStatusComplete) {
        complete = true;
      } else if (prior_state == kStatusFatal) {
        failure_code = kFailureInvariant;
        fatal = true;
      }
    }

    std::uint64_t anchor_point_id = kInvalid;
    if (!fatal && !complete && lane == 0U) {
      anchor_point_id = morton_point_ids[anchor_morton_position];
    }
    anchor_point_id = warp_broadcast_u64(anchor_point_id, 0);
    if (!fatal && !complete && anchor_point_id >= point_count) {
      failure_code = kFailureMalformed;
      fatal = true;
    }

    bool malformed_initial_root = false;
    if (!fatal && !complete && !load_checkpoint && lane == 0U) {
      malformed_initial_root = nodes[cursor].leaf_begin != 0U ||
                               nodes[cursor].leaf_end != point_count;
    }
    if (!fatal && !complete &&
        __any_sync(kFullWarpMask, malformed_initial_root)) {
      failure_code = kFailureMalformed;
      fatal = true;
    }

    while (!fatal && !complete && !chunk_ready) {
      if (subdivision_node_visit_count == node_visit_capacity) {
        if (cumulative_node_visit_count == node_count) {
          stop_reason = kStopNodes;
          fatal = true;
        } else if (subdivision_index + UINT64_C(1) ==
                   maximum_subdivision_count) {
          stop_reason = kStopFatal;
          failure_code = kFailureInvariant;
          fatal = true;
        } else {
          paused = true;
        }
        break;
      }
      if (cursor >= node_count ||
          cumulative_node_visit_count >= node_count) {
        stop_reason = kStopNodes;
        fatal = true;
        break;
      }
      const Phase15MortonYao48DeviceTiledHotNode node =
          load_certified_hot_node_warp(
              nodes, cursor, coordinate_bits, point_count, lane);
      if (node.leaf_begin >= node.leaf_end ||
          node.leaf_end > point_count) {
        failure_code = kFailureMalformed;
        fatal = true;
        break;
      }
      ++node_visit_count;
      ++cumulative_node_visit_count;
      ++subdivision_node_visit_count;
      const std::uint64_t width = node.leaf_end - node.leaf_begin;
      const bool entirely_owned =
          node.leaf_end <= anchor_morton_position;
      const bool entirely_suffix =
          node.leaf_begin >= anchor_morton_position;

      if (!entirely_owned && !entirely_suffix) {
        if (width == UINT64_C(1) ||
            !(node.leaf_begin < anchor_morton_position &&
              anchor_morton_position < node.leaf_end) ||
            node.right_child >= cursor) {
          failure_code = kFailureMalformed;
          fatal = true;
          break;
        }
        cursor = node.right_child;
        continue;
      }

      if (entirely_suffix) {
        if (!finish_or_skip_subtree(width, cursor, complete)) {
          failure_code = kFailureMalformed;
          fatal = true;
        }
        continue;
      }

      Phase15MortonYao48DeviceTiledSelectedWitnesses selected{};
      if (retained_witness_count >= required_witness_count) {
        if (prune_count >= prune_capacity) {
          failure_code = kFailureInvariant;
          fatal = true;
          break;
        }
        Phase15MortonYao48DeviceTiledPruneRegionRecord* output_prune =
            anchor_prunes + prune_count;
        const Phase15MortonYao48DeviceTiledPruneRegionRecord* cached_prune =
            prune_count == 0U ? nullptr : output_prune - 1U;
        selected = try_cached_certifying_witnesses_warp(
            cached_prune,
            output_prune,
            node,
            coordinate_bits,
            point_count,
            anchor_point_id,
            required_witness_count,
            prune_semantics,
            required_prune_flag,
            lane);
        if (selected.count != required_witness_count) {
          selected = select_certifying_witnesses_warp(
              anchor_witnesses,
              witness_capacity,
              witness_slots_per_bank,
              node,
              coordinate_bits,
              point_count,
              anchor_point_id,
              anchor_morton_position,
              required_witness_count,
              prune_semantics,
              output_prune,
              lane);
        }
      }
      const bool can_prune =
          selected.count == required_witness_count;
      if (can_prune) {
        // Cached witnesses are written cooperatively by lanes 0..9.  Publish
        // the complete receipt before lane 0 commits its metadata and before
        // the next reverse-postorder node can reuse it as an exact cache.
        __syncwarp(kFullWarpMask);
        if (prune_count >= prune_capacity ||
            cumulative_prune_count == UINT64_MAX) {
          failure_code = kFailureInvariant;
          fatal = true;
          break;
        }
        if (lane == 0U) {
          Phase15MortonYao48DeviceTiledPruneRegionRecord& record =
              anchor_prunes[prune_count];
          record.anchor_morton_position = anchor_morton_position;
          record.node_index = cursor;
          record.certified_pair_mass = width;
          record.retained_witness_count = selected.count;
          record.retained_witness_bank_mask = selected.bank_mask;
          record.flags = required_prune_flag;
        }
        ++prune_count;
        ++cumulative_prune_count;
        if (certified_pruned_mass > UINT64_MAX - width ||
            cumulative_certified_pruned_mass > UINT64_MAX - width) {
          failure_code = kFailureOverflow;
          fatal = true;
          break;
        }
        certified_pruned_mass += width;
        cumulative_certified_pruned_mass += width;
        if (!finish_or_skip_subtree(width, cursor, complete)) {
          failure_code = kFailureMalformed;
          fatal = true;
        } else if (!complete && prune_count == prune_capacity) {
          yield_reason = kYieldPrunes;
          chunk_ready = true;
        }
        continue;
      }

      if (width != UINT64_C(1)) {
        if (node.right_child >= cursor) {
          failure_code = kFailureMalformed;
          fatal = true;
          break;
        }
        cursor = node.right_child;
        continue;
      }

      if (node.leaf_begin >= anchor_morton_position) {
        failure_code = kFailureMalformed;
        fatal = true;
        break;
      }
      if (candidate_count >= candidate_capacity ||
          cumulative_candidate_count == UINT64_MAX) {
        failure_code = kFailureInvariant;
        fatal = true;
        break;
      }
      const std::uint64_t target_morton_position = node.leaf_begin;
      std::uint64_t target_point_id = kInvalid;
      if (lane == 0U) {
        target_point_id = morton_point_ids[target_morton_position];
      }
      target_point_id = warp_broadcast_u64(target_point_id, 0);
      if (target_point_id >= point_count ||
          target_point_id == anchor_point_id) {
        failure_code = kFailureMalformed;
        fatal = true;
        break;
      }

      Phase15MortonYao48DeviceTiledConeClassification classification{};
      if (lane == 0U) {
        classification = classify_yao48_cone_by_intervals(
            coordinate_bits,
            point_count,
            anchor_point_id,
            target_point_id);
      }
      classification.cone_index =
          warp_broadcast_u64(classification.cone_index, 0);
      classification.ambiguous =
          __shfl_sync(
              kFullWarpMask,
              classification.ambiguous ? 1U : 0U,
              0) != 0U;
      classification.malformed =
          __shfl_sync(
              kFullWarpMask,
              classification.malformed ? 1U : 0U,
              0) != 0U;

      if (lane == 0U) {
        Phase15MortonYao48DeviceTiledCandidateRecord record;
        record.support_u = anchor_point_id < target_point_id
                               ? anchor_point_id
                               : target_point_id;
        record.support_v = anchor_point_id < target_point_id
                               ? target_point_id
                               : anchor_point_id;
        record.anchor_morton_position = anchor_morton_position;
        record.partner_morton_position = target_morton_position;
        record.owner_cone_index = classification.cone_index;
        record.flags = classification.ambiguous
                           ? kCandidateFlagAmbiguousCone
                           : kCandidateFlagCertifiedCone;
        anchor_candidates[candidate_count] = record;
      }
      ++candidate_count;
      ++cumulative_candidate_count;

      if (classification.malformed) {
        failure_code = kFailureMalformed;
        fatal = true;
        break;
      }
      if (classification.ambiguous) {
        ++ambiguous_candidate_count;
        ++unbanked_candidate_count;
        ++cumulative_ambiguous_candidate_count;
        ++cumulative_unbanked_candidate_count;
      } else {
        Phase15MortonYao48DeviceTiledBankUpdate bank_update{};
        if (lane == 0U) {
          bank_update = update_witness_bank_after_candidate(
              anchor_witnesses,
              classification.cone_index,
              witness_slots_per_bank,
              coordinate_bits,
              point_count,
              anchor_point_id,
              anchor_morton_position,
              target_point_id,
              target_morton_position);
          if (bank_update.inserted) {
            anchor_candidates[candidate_count - UINT64_C(1)].flags |=
                kCandidateFlagBankInserted;
          }
          if (bank_update.replaced) {
            anchor_candidates[candidate_count - UINT64_C(1)].flags |=
                kCandidateFlagBankReplaced;
          }
        }
        const bool inserted =
            __shfl_sync(
                kFullWarpMask, bank_update.inserted ? 1U : 0U, 0) != 0U;
        const bool replaced =
            __shfl_sync(
                kFullWarpMask, bank_update.replaced ? 1U : 0U, 0) != 0U;
        if (inserted && !replaced) {
          ++retained_witness_count;
        }
        __syncwarp(kFullWarpMask);
      }

      if (!finish_or_skip_subtree(UINT64_C(1), cursor, complete)) {
        failure_code = kFailureMalformed;
        fatal = true;
      } else if (!complete && candidate_count == candidate_capacity) {
        yield_reason = kYieldCandidates;
        chunk_ready = true;
      }
    }

    const bool delta_counts_valid =
        candidate_count <= candidate_capacity &&
        prune_count <= prune_capacity &&
        ambiguous_candidate_count <= unbanked_candidate_count &&
        unbanked_candidate_count <= candidate_count;
    const bool cumulative_counts_valid =
        cumulative_ambiguous_candidate_count <=
            cumulative_unbanked_candidate_count &&
        cumulative_unbanked_candidate_count <=
            cumulative_candidate_count &&
        cumulative_candidate_count <= anchor_morton_position &&
        cumulative_certified_pruned_mass <=
            anchor_morton_position - cumulative_candidate_count &&
        cumulative_node_visit_count <= node_count;
    std::uint64_t unresolved_pair_mass =
        cumulative_counts_valid
            ? anchor_morton_position - cumulative_candidate_count -
                  cumulative_certified_pruned_mass
            : anchor_morton_position;

    if (!fatal && complete &&
        (paused || chunk_ready || yield_reason != kYieldNone ||
         !delta_counts_valid || !cumulative_counts_valid ||
         unresolved_pair_mass != UINT64_C(0))) {
      failure_code = kFailureInvariant;
      fatal = true;
    }
    if (!fatal && chunk_ready &&
        (paused || complete || !delta_counts_valid ||
         !cumulative_counts_valid || unresolved_pair_mass == UINT64_C(0) ||
         !((yield_reason == kYieldCandidates &&
            candidate_count == candidate_capacity) ||
           (yield_reason == kYieldPrunes &&
            prune_count == prune_capacity)))) {
      failure_code = kFailureInvariant;
      fatal = true;
    }
    if (!fatal && paused &&
        (complete || chunk_ready || yield_reason != kYieldNone ||
         !delta_counts_valid || !cumulative_counts_valid ||
         unresolved_pair_mass == UINT64_C(0) || cursor >= node_count)) {
      failure_code = kFailureInvariant;
      fatal = true;
      paused = false;
    }
    if (!fatal && !complete && !chunk_ready && !paused) {
      failure_code = kFailureInvariant;
      fatal = true;
    }

    const bool resumed_already_complete =
        resumed_first_subdivision && prior_state == kStatusComplete;
    if (!resumed_already_complete) {
      if (completed_subdivision_count == UINT64_MAX) {
        failure_code = kFailureOverflow;
        fatal = true;
        paused = false;
        chunk_ready = false;
      } else {
        ++completed_subdivision_count;
      }
    }

    if (fatal) {
      paused = false;
      chunk_ready = false;
      complete = false;
      yield_reason = kYieldNone;
      if (stop_reason == kStopNone) {
        stop_reason = kStopFatal;
      }
      invalidate_anchor_segments_warp(
          anchor_candidates,
          candidate_capacity,
          anchor_prunes,
          prune_capacity,
          lane);
      __syncwarp(kFullWarpMask);
      cumulative_candidate_count =
          candidate_count <= cumulative_candidate_count
              ? cumulative_candidate_count - candidate_count
              : UINT64_C(0);
      cumulative_prune_count =
          prune_count <= cumulative_prune_count
              ? cumulative_prune_count - prune_count
              : UINT64_C(0);
      cumulative_certified_pruned_mass =
          certified_pruned_mass <= cumulative_certified_pruned_mass
              ? cumulative_certified_pruned_mass - certified_pruned_mass
              : UINT64_C(0);
      cumulative_ambiguous_candidate_count =
          ambiguous_candidate_count <=
                  cumulative_ambiguous_candidate_count
              ? cumulative_ambiguous_candidate_count -
                    ambiguous_candidate_count
              : UINT64_C(0);
      cumulative_unbanked_candidate_count =
          unbanked_candidate_count <= cumulative_unbanked_candidate_count
              ? cumulative_unbanked_candidate_count -
                    unbanked_candidate_count
              : UINT64_C(0);
      candidate_count = UINT64_C(0);
      prune_count = UINT64_C(0);
      certified_pruned_mass = UINT64_C(0);
      ambiguous_candidate_count = UINT64_C(0);
      unbanked_candidate_count = UINT64_C(0);
      unresolved_pair_mass =
          cumulative_candidate_count <= anchor_morton_position &&
                  cumulative_certified_pruned_mass <=
                      anchor_morton_position - cumulative_candidate_count
              ? anchor_morton_position - cumulative_candidate_count -
                    cumulative_certified_pruned_mass
              : anchor_morton_position;
    }

    const std::uint64_t status =
        fatal ? kStatusFatal
              : (chunk_ready ? kStatusChunkReady
                             : (paused ? kStatusActive : kStatusComplete));
    if (lane == 0U) {
      Phase15MortonYao48DeviceTiledAnchorCheckpoint next{};
      next.anchor_morton_position = anchor_morton_position;
      next.cursor = cursor;
      next.candidate_count = candidate_count;
      next.prune_region_count = prune_count;
      next.certified_pruned_pair_mass = certified_pruned_mass;
      next.node_visit_count = node_visit_count;
      next.ambiguous_cone_candidate_count = ambiguous_candidate_count;
      next.unbanked_candidate_count = unbanked_candidate_count;
      next.cumulative_candidate_count = cumulative_candidate_count;
      next.cumulative_prune_region_count = cumulative_prune_count;
      next.cumulative_certified_pruned_pair_mass =
          cumulative_certified_pruned_mass;
      next.cumulative_node_visit_count = cumulative_node_visit_count;
      next.cumulative_ambiguous_cone_candidate_count =
          cumulative_ambiguous_candidate_count;
      next.cumulative_unbanked_candidate_count =
          cumulative_unbanked_candidate_count;
      next.retained_witness_count = retained_witness_count;
      next.completed_subdivision_count = completed_subdivision_count;
      next.tile_epoch = tile_epoch;
      next.chunk_sequence = chunk_sequence;
      next.state = status;
      next.reserved_zero = UINT64_C(0);
      *checkpoint = next;

      Phase15MortonYao48DeviceTiledAnchorControl control{};
      control.anchor_morton_position = anchor_morton_position;
      control.candidate_count = candidate_count;
      control.prune_region_count = prune_count;
      control.certified_pruned_pair_mass = certified_pruned_mass;
      control.node_visit_count = node_visit_count;
      control.cumulative_candidate_count = cumulative_candidate_count;
      control.cumulative_prune_region_count = cumulative_prune_count;
      control.cumulative_certified_pruned_pair_mass =
          cumulative_certified_pruned_mass;
      control.cumulative_node_visit_count = cumulative_node_visit_count;
      control.status = status;
      control.yield_reason = yield_reason;
      control.stop_reason = stop_reason;
      control.failure_code = failure_code;
      control.ambiguous_cone_candidate_count =
          ambiguous_candidate_count;
      control.unbanked_candidate_count = unbanked_candidate_count;
      control.cumulative_ambiguous_cone_candidate_count =
          cumulative_ambiguous_candidate_count;
      control.cumulative_unbanked_candidate_count =
          cumulative_unbanked_candidate_count;
      control.unresolved_pair_mass = unresolved_pair_mass;
      control.tile_epoch = tile_epoch;
      control.chunk_sequence = chunk_sequence;
      control.reserved_zero = UINT64_C(0);
      controls[anchor_slot] = control;

      if (paused) {
        atomicAdd(
            reinterpret_cast<unsigned long long*>(pending_anchor_count),
            1ULL);
      }
    }
    anchor_slot += anchor_stride;
  }
}

}  // namespace

Phase15MortonYao48DeviceTiledAdoptedTraversal
adopt_phase15_morton_yao48_device_tiled_traversal(
    MortonLbvhDeviceTraversalLease&& traversal_lease) {
  if (!traversal_lease.ready() || !traversal_lease.cuda_resident() ||
      traversal_lease.audit().host_fake_lifecycle_exercised ||
      !traversal_lease.audit().cuda_device_storage_retained) {
    throw std::invalid_argument(
        "the production Phase 15 tiled Morton/Yao48 adoption requires a "
        "ready CUDA-resident traversal lease");
  }
  const MortonLbvhDeviceTraversalLeaseAudit audit = traversal_lease.audit_;

  Phase15MortonYao48DeviceTiledAdoptedTraversal adopted;
  adopted.retained_owner = std::move(traversal_lease.retained_resources_);
  adopted.source_cloud_identity =
      std::move(traversal_lease.source_cloud_identity_);
  adopted.device_coordinate_bits = traversal_lease.device_coordinate_bits_;
  adopted.device_morton_point_ids =
      traversal_lease.device_morton_point_ids_;
  adopted.device_nodes = traversal_lease.device_nodes_;
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
  adopted.cuda_device = traversal_lease.cuda_device_;
  adopted.execution_kind =
      Phase15MortonYao48DeviceTiledExecutionKind::cuda;
  adopted.canonical_coordinate_words_retained =
      audit.canonical_coordinate_words_retained;
  adopted.active_morton_point_ids_retained =
      audit.active_morton_point_ids_retained;
  adopted.certified_device_nodes_retained =
      audit.certified_device_nodes_retained;
  adopted.host_fake_lifecycle_exercised = false;
  adopted.cuda_device_storage_retained =
      audit.cuda_device_storage_retained;

  traversal_lease.device_coordinate_bits_ = nullptr;
  traversal_lease.device_morton_point_ids_ = nullptr;
  traversal_lease.device_nodes_ = nullptr;
  traversal_lease.cuda_device_ = -1;
  return adopted;
}

Phase15MortonYao48DeviceTiledBatch
build_phase15_morton_yao48_device_tiled_pair_frontier_on_device(
    Phase15MortonYao48DeviceTiledPairFrontierContextState& context,
    const Phase15MortonYao48DeviceTiledAdoptedTraversal& traversal,
    const Phase15MortonYao48DeviceTiledRequest& request) {
  (void)context;
  validate_launch(traversal, request);
  DeviceGuard guard{traversal.cuda_device};
  require_device_pointer(
      traversal.device_coordinate_bits,
      traversal.cuda_device,
      alignof(std::uint64_t),
      "coordinate");
  require_device_pointer(
      traversal.device_morton_point_ids,
      traversal.cuda_device,
      alignof(std::uint64_t),
      "Morton PointId");
  require_device_pointer(
      traversal.device_nodes,
      traversal.cuda_device,
      alignof(Phase15MortonYao48DeviceTiledNode),
      "node");

  cudaDeviceProp properties{};
  check_cuda(
      cudaGetDeviceProperties(&properties, traversal.cuda_device),
      "cudaGetDeviceProperties for Phase 15 tiled Morton/Yao48 frontier");
  const Phase15MortonYao48DeviceTiledLaunchShape shape =
      launch_shape(request.anchor_count, properties);

  const std::size_t candidate_capacity = checked_product(
      request.anchor_count,
      request.candidate_capacity_per_anchor,
      "the Phase 15 tiled Morton/Yao48 candidate capacity overflows size_t");
  const std::size_t prune_capacity = checked_product(
      request.anchor_count,
      request.prune_region_capacity_per_anchor,
      "the Phase 15 tiled Morton/Yao48 prune capacity overflows size_t");
  const std::size_t witness_capacity = checked_product(
      checked_product(
          request.anchor_count,
          request.witness_bank_count_per_anchor,
          "the Phase 15 tiled Morton/Yao48 bank count overflows size_t"),
      request.witness_slot_count_per_bank,
      "the Phase 15 tiled Morton/Yao48 witness capacity overflows size_t");
  const std::size_t control_capacity = request.anchor_count;
  const std::size_t checkpoint_capacity = request.anchor_count;
  const std::size_t maximum_subdivision_count =
      request.certified_node_count /
          request.node_visit_capacity_per_anchor +
      (request.certified_node_count %
                   request.node_visit_capacity_per_anchor ==
               0U
           ? 0U
           : 1U);
  const std::size_t candidate_bytes = checked_product(
      candidate_capacity,
      sizeof(Phase15MortonYao48DeviceTiledCandidateRecord),
      "the Phase 15 tiled Morton/Yao48 candidate bytes overflow size_t");
  const std::size_t prune_bytes = checked_product(
      prune_capacity,
      sizeof(Phase15MortonYao48DeviceTiledPruneRegionRecord),
      "the Phase 15 tiled Morton/Yao48 prune bytes overflow size_t");
  const std::size_t witness_bytes = checked_product(
      witness_capacity,
      sizeof(Phase15MortonYao48DeviceTiledWitnessBankSlot),
      "the Phase 15 tiled Morton/Yao48 witness bytes overflow size_t");
  const std::size_t control_bytes = checked_product(
      control_capacity,
      sizeof(Phase15MortonYao48DeviceTiledAnchorControl),
      "the Phase 15 tiled Morton/Yao48 control bytes overflow size_t");
  const std::size_t checkpoint_bytes = checked_product(
      checkpoint_capacity,
      sizeof(Phase15MortonYao48DeviceTiledAnchorCheckpoint),
      "the Phase 15 tiled Morton/Yao48 checkpoint bytes overflow size_t");
  std::size_t device_arena_capacity_bytes = checked_sum(
      candidate_bytes,
      prune_bytes,
      "the Phase 15 tiled Morton/Yao48 arena bytes overflow size_t");
  device_arena_capacity_bytes = checked_sum(
      device_arena_capacity_bytes,
      witness_bytes,
      "the Phase 15 tiled Morton/Yao48 arena bytes overflow size_t");
  device_arena_capacity_bytes = checked_sum(
      device_arena_capacity_bytes,
      control_bytes,
      "the Phase 15 tiled Morton/Yao48 arena bytes overflow size_t");
  device_arena_capacity_bytes = checked_sum(
      device_arena_capacity_bytes,
      checkpoint_bytes,
      "the Phase 15 tiled Morton/Yao48 arena bytes overflow size_t");
  device_arena_capacity_bytes = checked_sum(
      device_arena_capacity_bytes,
      sizeof(std::uint64_t),
      "the Phase 15 tiled Morton/Yao48 arena bytes overflow size_t");

  std::shared_ptr<void>& retained_device_resources =
      context.device_resources();
  if (retained_device_resources != nullptr &&
      retained_device_resources.use_count() != 1L) {
    throw std::logic_error(
        "the Phase 15 tiled Morton/Yao48 CUDA arena cannot be reused while "
        "a detached candidate tile lease is alive");
  }

  std::shared_ptr<Phase15MortonYao48DeviceTiledCudaResources> resources;
  bool fresh_tile_device_arena_allocated = false;
  bool fresh_tile_device_arena_reused = false;
  if (request.resume_same_tile) {
    if (retained_device_resources == nullptr) {
      throw std::invalid_argument(
          "the Phase 15 tiled Morton/Yao48 CUDA tile cannot resume without "
          "its retained device arena");
    }
    resources = std::static_pointer_cast<
        Phase15MortonYao48DeviceTiledCudaResources>(
        retained_device_resources);
    if (!resources->matches_resume(
            traversal,
            request,
            candidate_capacity,
            prune_capacity,
            witness_capacity,
            control_capacity,
            checkpoint_capacity)) {
      throw std::invalid_argument(
          "the Phase 15 tiled Morton/Yao48 CUDA continuation identity or "
          "arena extent does not match the retained tile");
    }
  } else {
    if (retained_device_resources != nullptr) {
      resources = std::static_pointer_cast<
          Phase15MortonYao48DeviceTiledCudaResources>(
          retained_device_resources);
      if (resources->can_rebind_fresh_tile(
              traversal,
              request,
              candidate_capacity,
              prune_capacity,
              witness_capacity,
              control_capacity,
              checkpoint_capacity)) {
        resources->rebind_fresh_tile(request);
        fresh_tile_device_arena_reused = true;
      } else {
        resources.reset();
        retained_device_resources.reset();
      }
    }
    if (resources == nullptr) {
      // The uniqueness check above proves that no detached tile lease
      // retains the completed arena. A different extent still replaces it,
      // so the physical peak remains exactly one output arena.
      resources =
          std::make_shared<Phase15MortonYao48DeviceTiledCudaResources>(
              traversal,
              request,
              candidate_capacity,
              prune_capacity,
              witness_capacity,
              control_capacity,
              checkpoint_capacity);
      retained_device_resources = resources;
      fresh_tile_device_arena_allocated = true;
    }
  }
  try {
    if (!request.resume_same_tile) {
      check_cuda(
          cudaMemsetAsync(
              resources->candidates(),
              0xff,
              candidate_bytes,
              resources->stream()),
          "cudaMemsetAsync Phase 15 tiled Morton/Yao48 candidates");
      check_cuda(
          cudaMemsetAsync(
              resources->prunes(),
              0xff,
              prune_bytes,
              resources->stream()),
          "cudaMemsetAsync Phase 15 tiled Morton/Yao48 prune regions");
      check_cuda(
          cudaMemsetAsync(
              resources->witnesses(),
              0xff,
              witness_bytes,
              resources->stream()),
          "cudaMemsetAsync Phase 15 tiled Morton/Yao48 witness banks");
    }
    check_cuda(
        cudaMemsetAsync(
            resources->controls(),
            0xff,
            control_bytes,
            resources->stream()),
        "cudaMemsetAsync Phase 15 tiled Morton/Yao48 anchor controls");
    if (!request.resume_same_tile) {
      check_cuda(
          cudaMemsetAsync(
              resources->checkpoints(),
              0xff,
              checkpoint_bytes,
              resources->stream()),
          "cudaMemsetAsync Phase 15 tiled Morton/Yao48 anchor checkpoints");
    }

    std::size_t subdivision_count = 0U;
    std::uint64_t pending_anchor_count = 0U;
    for (std::size_t subdivision_index = 0U;
         subdivision_index < maximum_subdivision_count;
         ++subdivision_index) {
      check_cuda(
          cudaMemsetAsync(
              resources->pending_anchor_count(),
              0,
              sizeof(std::uint64_t),
              resources->stream()),
          "cudaMemsetAsync Phase 15 tiled Morton/Yao48 pending-anchor control");
      build_tiled_morton_yao48_pair_frontier_kernel<<<
          shape.block_count,
          shape.thread_count,
          0U,
          resources->stream()>>>(
          traversal.device_coordinate_bits,
          traversal.device_morton_point_ids,
          static_cast<const Phase15MortonYao48DeviceTiledNode*>(
              traversal.device_nodes),
          checked_u64(
              request.point_count,
              "the Phase 15 tiled Morton/Yao48 point count does not fit "
              "uint64"),
          checked_u64(
              request.certified_node_count,
              "the Phase 15 tiled Morton/Yao48 node count does not fit "
              "uint64"),
          checked_u64(
              request.anchor_begin,
              "the Phase 15 tiled Morton/Yao48 anchor begin does not fit "
              "uint64"),
          checked_u64(
              request.anchor_count,
              "the Phase 15 tiled Morton/Yao48 anchor count does not fit "
              "uint64"),
          checked_u64(
              request.maximum_closed_rank,
              "the Phase 15 tiled Morton/Yao48 rank cap does not fit uint64"),
          static_cast<std::uint64_t>(request.prune_semantics),
          checked_u64(
              request.required_witness_count,
              "the Phase 15 tiled Morton/Yao48 required witness count does "
              "not fit uint64"),
          checked_u64(
              request.node_visit_capacity_per_anchor,
              "the Phase 15 tiled Morton/Yao48 node cap does not fit uint64"),
          checked_u64(
              request.candidate_capacity_per_anchor,
              "the Phase 15 tiled Morton/Yao48 candidate cap does not fit "
              "uint64"),
          checked_u64(
              request.prune_region_capacity_per_anchor,
              "the Phase 15 tiled Morton/Yao48 prune cap does not fit uint64"),
          checked_u64(
              request.witness_bank_count_per_anchor,
              "the Phase 15 tiled Morton/Yao48 bank count does not fit "
              "uint64"),
          checked_u64(
              request.witness_slot_count_per_bank,
              "the Phase 15 tiled Morton/Yao48 bank width does not fit "
              "uint64"),
          checked_u64(
              subdivision_index,
              "the Phase 15 tiled Morton/Yao48 subdivision index does not fit uint64"),
          checked_u64(
              maximum_subdivision_count,
              "the Phase 15 tiled Morton/Yao48 subdivision ceiling does not fit uint64"),
          request.resume_same_tile ? UINT64_C(1) : UINT64_C(0),
          request.tile_epoch,
          request.chunk_sequence,
          resources->candidates(),
          resources->prunes(),
          resources->witnesses(),
          resources->controls(),
          resources->checkpoints(),
          resources->pending_anchor_count());
      check_cuda(
          cudaPeekAtLastError(),
          "Phase 15 tiled Morton/Yao48 frontier subdivision kernel launch");
      check_cuda(
          cudaMemcpyAsync(
              &pending_anchor_count,
              resources->pending_anchor_count(),
              sizeof(std::uint64_t),
              cudaMemcpyDeviceToHost,
              resources->stream()),
          "cudaMemcpyAsync Phase 15 tiled Morton/Yao48 pending-anchor control device-to-host");
      resources->synchronize();
      ++subdivision_count;
      if (pending_anchor_count == 0U) {
        break;
      }
    }
    if (pending_anchor_count != 0U || subdivision_count == 0U) {
      throw std::runtime_error(
          "the Phase 15 tiled Morton/Yao48 traversal exceeded its certified subdivision ceiling");
    }

    Phase15MortonYao48DeviceTiledBatch batch;
    batch.source_cloud_identity_authority =
        traversal.source_cloud_identity;
    batch.host_anchor_controls.resize(control_capacity);
    check_cuda(
        cudaMemcpyAsync(
            batch.host_anchor_controls.data(),
            resources->controls(),
            control_bytes,
            cudaMemcpyDeviceToHost,
            resources->stream()),
        "cudaMemcpyAsync Phase 15 tiled Morton/Yao48 controls "
        "device-to-host");
    resources->synchronize();

    bool any_chunk_ready = false;
    bool any_fatal = false;
    bool all_complete = true;
    for (const Phase15MortonYao48DeviceTiledAnchorControl& control :
         batch.host_anchor_controls) {
      if (control.failure_code != kFailureNone) {
        throw std::runtime_error(
            "the Phase 15 tiled Morton/Yao48 CUDA kernel reported an "
            "internal failure code; no terminal frontier is published");
      }
      if (control.status == kStatusFatal &&
          control.stop_reason != kStopNodes) {
        throw std::runtime_error(
            "the Phase 15 tiled Morton/Yao48 CUDA kernel reported a fatal "
            "state without a certified node-capacity stop");
      }
      any_chunk_ready =
          any_chunk_ready || control.status == kStatusChunkReady;
      any_fatal = any_fatal || control.status == kStatusFatal;
      all_complete = all_complete && control.status == kStatusComplete;
    }
    if (any_fatal) {
      for (Phase15MortonYao48DeviceTiledAnchorControl& control :
           batch.host_anchor_controls) {
        if (control.candidate_count > control.cumulative_candidate_count ||
            control.prune_region_count >
                control.cumulative_prune_region_count ||
            control.certified_pruned_pair_mass >
                control.cumulative_certified_pruned_pair_mass ||
            control.ambiguous_cone_candidate_count >
                control.cumulative_ambiguous_cone_candidate_count ||
            control.unbanked_candidate_count >
                control.cumulative_unbanked_candidate_count) {
          throw std::runtime_error(
              "the Phase 15 tiled Morton/Yao48 fatal batch cannot roll "
              "back malformed output deltas");
        }
        control.cumulative_candidate_count -= control.candidate_count;
        control.cumulative_prune_region_count -=
            control.prune_region_count;
        control.cumulative_certified_pruned_pair_mass -=
            control.certified_pruned_pair_mass;
        control.cumulative_ambiguous_cone_candidate_count -=
            control.ambiguous_cone_candidate_count;
        control.cumulative_unbanked_candidate_count -=
            control.unbanked_candidate_count;
        control.candidate_count = UINT64_C(0);
        control.prune_region_count = UINT64_C(0);
        control.certified_pruned_pair_mass = UINT64_C(0);
        control.ambiguous_cone_candidate_count = UINT64_C(0);
        control.unbanked_candidate_count = UINT64_C(0);
        if (control.cumulative_candidate_count >
                control.anchor_morton_position ||
            control.cumulative_certified_pruned_pair_mass >
                control.anchor_morton_position -
                    control.cumulative_candidate_count) {
          throw std::runtime_error(
              "the Phase 15 tiled Morton/Yao48 fatal batch cannot close "
              "its rollback partition");
        }
        control.unresolved_pair_mass =
            control.anchor_morton_position -
            control.cumulative_candidate_count -
            control.cumulative_certified_pruned_pair_mass;
        control.yield_reason = kYieldNone;
        if (control.status != kStatusFatal) {
          control.status = control.unresolved_pair_mass == UINT64_C(0)
                               ? kStatusComplete
                               : kStatusActive;
          control.stop_reason = kStopNone;
          control.failure_code = kFailureNone;
        }
      }
      any_chunk_ready = false;
      all_complete = false;
      for (const Phase15MortonYao48DeviceTiledAnchorControl& control :
           batch.host_anchor_controls) {
        if (control.status == kStatusActive) {
          throw std::runtime_error(
              "the Phase 15 tiled Morton/Yao48 node-capacity stop could not "
              "form an atomic terminal batch after withholding its current "
              "output chunk");
        }
      }
    }
    if (!any_chunk_ready && !any_fatal && !all_complete) {
      throw std::runtime_error(
          "the Phase 15 tiled Morton/Yao48 CUDA launcher stopped without "
          "a resumable chunk, a terminal failure or a complete tile");
    }
    resources->commit_chunk_sequence(request.chunk_sequence);

    batch.device_candidate_records = resources->candidates();
    batch.device_prune_regions = resources->prunes();
    batch.device_witness_bank_slots = resources->witnesses();
    batch.device_anchor_controls = resources->controls();
    batch.physical_candidate_capacity = candidate_capacity;
    batch.physical_prune_region_capacity = prune_capacity;
    batch.physical_witness_bank_slot_capacity = witness_capacity;
    batch.physical_anchor_control_capacity = control_capacity;
    batch.physical_anchor_checkpoint_capacity = checkpoint_capacity;
    batch.physical_pending_anchor_count_capacity = 1U;
    batch.physical_device_arena_capacity_bytes =
        device_arena_capacity_bytes;
    batch.anchor_control_device_to_host_count = control_capacity;
    batch.anchor_control_device_to_host_byte_count = control_bytes;
    batch.candidate_device_to_host_count = 0U;
    batch.certified_prune_device_to_host_count = 0U;
    batch.kernel_launch_count = subdivision_count;
    batch.synchronization_count = subdivision_count + 1U;
    batch.traversal_subdivision_count = subdivision_count;
    batch.maximum_traversal_subdivision_count_per_anchor =
        maximum_subdivision_count;
    batch.resume_control_device_to_host_count = subdivision_count;
    batch.resume_control_device_to_host_byte_count = checked_product(
        subdivision_count,
        sizeof(std::uint64_t),
        "the Phase 15 tiled Morton/Yao48 resume-control bytes overflow size_t");
    batch.source_snapshot_epoch = request.source_snapshot_epoch;
    batch.output_buffer_epoch = request.output_buffer_epoch;
    batch.tile_epoch = request.tile_epoch;
    batch.chunk_sequence = request.chunk_sequence;
    batch.cuda_device = traversal.cuda_device;
    batch.execution_kind =
        Phase15MortonYao48DeviceTiledExecutionKind::cuda;
    batch.prune_semantics = request.prune_semantics;
    batch.required_witness_count = request.required_witness_count;
    batch.fixed_anchor_segments_allocated = true;
    batch.output_owner_detached_for_tile_lifetime = true;
    batch.interval_cone_classification_requested = true;
    batch.ambiguous_cone_to_unbanked_candidate_requested = true;
    batch.target_tested_before_bank_insert_requested = true;
    batch.retained_witnesses_outside_pruned_subtree_requested = true;
    batch.nonnegative_diametral_witness_interval_lower_bound_requested =
        request.prune_semantics ==
        MortonYao48DeviceTiledPairFrontierPruneSemantics::closed_rank_window;
    batch.strictly_positive_diametral_witness_interval_lower_bound_requested =
        request.prune_semantics ==
        MortonYao48DeviceTiledPairFrontierPruneSemantics::
            strict_interior_threshold;
    batch.censored_anchor_outputs_invalidated = true;
    batch.exact_diametral_rank_evaluated = false;
    batch.scientific_pair_catalog_published = false;
    batch.dense_pair_fallback_performed = false;
    batch.global_pair_matrix_materialized = false;
    batch.higher_order_structure_materialized = false;
    // This flag names the real CUDA envelope expected by the host contract;
    // it does not claim exact ranks or scientific catalog qualification.
    batch.cuda_execution_contract_satisfied = true;
    batch.fresh_tile_device_arena_allocated =
        fresh_tile_device_arena_allocated;
    batch.fresh_tile_device_arena_reused =
        fresh_tile_device_arena_reused;
    batch.resume_same_tile = request.resume_same_tile;
    batch.capacity_yield_resumable = any_chunk_ready;
    batch.process_restart_resumable = false;
    batch.retained_output_owner = resources;
    batch.metadata_digest =
        phase15_morton_yao48_device_tiled_metadata_digest(batch);
    guard.restore();
    return batch;
  } catch (...) {
    resources->synchronize_after_failure();
    throw;
  }
}

}  // namespace morsehgp3d::gpu::detail
