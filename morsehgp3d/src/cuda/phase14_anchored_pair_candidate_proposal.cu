#include "phase14_anchored_pair_candidate_proposal_internal.hpp"

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
#include <type_traits>
#include <utility>
#include <vector>

#if !defined(__CUDACC__)
#error "phase14_anchored_pair_candidate_proposal.cu requires NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Phase 14 anchored-pair proposal requires CUDA 12.9"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the Phase 14 anchored-pair proposal"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the Phase 14 anchored-pair proposal"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Phase 14 anchored-pair proposal must contain only sm_120 code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

constexpr unsigned int kWarpSize = 32U;
constexpr unsigned int kMaximumWarpsPerBlock = 8U;
constexpr unsigned int kFullWarpMask = 0xffffffffU;
constexpr std::size_t kAxisCount = 3U;
constexpr std::uint64_t kProposalSchemaVersion = UINT64_C(1);
constexpr std::uint64_t kQueryDigestDomain =
    UINT64_C(0x5038645155455259);
constexpr std::uint64_t kFnvOffsetBasis =
    UINT64_C(14695981039346656037);

constexpr std::uint64_t kCursorInitial = static_cast<std::uint64_t>(
    Phase14AnchoredPairCandidateCursorKind::initial);
constexpr std::uint64_t kCursorActive = static_cast<std::uint64_t>(
    Phase14AnchoredPairCandidateCursorKind::active);
constexpr std::uint64_t kCursorComplete = static_cast<std::uint64_t>(
    Phase14AnchoredPairCandidateCursorKind::complete);
constexpr std::uint64_t kStatusComplete = static_cast<std::uint64_t>(
    Phase14AnchoredPairCandidateSegmentStatus::complete);
constexpr std::uint64_t kStatusBudget = static_cast<std::uint64_t>(
    Phase14AnchoredPairCandidateSegmentStatus::budget_exhausted);
constexpr std::uint64_t kStatusCapacity = static_cast<std::uint64_t>(
    Phase14AnchoredPairCandidateSegmentStatus::capacity_exhausted);
constexpr std::uint64_t kStopNone = static_cast<std::uint64_t>(
    Phase14AnchoredPairCandidateStopReason::none);
constexpr std::uint64_t kStopNodes = static_cast<std::uint64_t>(
    Phase14AnchoredPairCandidateStopReason::node_visit_limit);
constexpr std::uint64_t kStopRecords = static_cast<std::uint64_t>(
    Phase14AnchoredPairCandidateStopReason::transcript_record_limit);
constexpr std::uint64_t kFailureNone = static_cast<std::uint64_t>(
    Phase14AnchoredPairCandidateFailureCode::none);
constexpr std::uint64_t kFailureMalformedQuery =
    static_cast<std::uint64_t>(
        Phase14AnchoredPairCandidateFailureCode::malformed_query);
constexpr std::uint64_t kFailureNodeOutOfRange =
    static_cast<std::uint64_t>(
        Phase14AnchoredPairCandidateFailureCode::node_out_of_range);
constexpr std::uint64_t kFailureTraversal = static_cast<std::uint64_t>(
    Phase14AnchoredPairCandidateFailureCode::traversal_failure);

// Private ABI retained by MortonLbvhDeviceTraversalLease.  This duplicate is
// intentionally layout-checked here: the proposal never owns or copies the
// O(n) node arena.
struct Phase14AnchoredPairCandidateDeviceNode {
  std::uint64_t lower_point_ids[3];
  std::uint64_t upper_point_ids[3];
  std::uint64_t left_child;
  std::uint64_t right_child;
  std::uint64_t leaf_begin;
  std::uint64_t leaf_end;
};

static_assert(
    std::is_standard_layout_v<Phase14AnchoredPairCandidateDeviceNode>);
static_assert(
    std::is_trivially_copyable_v<Phase14AnchoredPairCandidateDeviceNode>);
static_assert(sizeof(Phase14AnchoredPairCandidateDeviceNode) == 80U);
static_assert(alignof(Phase14AnchoredPairCandidateDeviceNode) == 8U);
static_assert(
    offsetof(Phase14AnchoredPairCandidateDeviceNode, lower_point_ids) ==
    0U);
static_assert(
    offsetof(Phase14AnchoredPairCandidateDeviceNode, upper_point_ids) ==
    24U);
static_assert(
    offsetof(Phase14AnchoredPairCandidateDeviceNode, left_child) == 48U);
static_assert(
    offsetof(Phase14AnchoredPairCandidateDeviceNode, right_child) == 56U);
static_assert(
    offsetof(Phase14AnchoredPairCandidateDeviceNode, leaf_begin) == 64U);
static_assert(
    offsetof(Phase14AnchoredPairCandidateDeviceNode, leaf_end) == 72U);

struct Phase14AnchoredPairCandidateQueryPlan {
  std::uint64_t raw_record_count{};
  std::uint64_t record_begin{};
  std::uint64_t allocated_record_count{};
  std::uint64_t failure_code{};
};

static_assert(
    std::is_standard_layout_v<Phase14AnchoredPairCandidateQueryPlan>);
static_assert(
    std::is_trivially_copyable_v<Phase14AnchoredPairCandidateQueryPlan>);
static_assert(
    sizeof(Phase14AnchoredPairCandidateQueryPlan) ==
    4U * sizeof(std::uint64_t));

struct Phase14AnchoredPairCandidateTraversalOutcome {
  std::uint64_t next_cursor_kind{kCursorActive};
  std::uint64_t next_cursor_node_index{
      phase14_anchored_pair_candidate_invalid_node_index};
  std::uint64_t status{kStatusBudget};
  std::uint64_t stop_reason{kStopNodes};
  std::uint64_t node_visit_count{};
  std::uint64_t record_count{};
  std::uint64_t failure_code{kFailureNone};
};

class Phase14AnchoredPairCandidateCudaFailure final
    : public std::runtime_error {
 public:
  Phase14AnchoredPairCandidateCudaFailure(
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
    throw Phase14AnchoredPairCandidateCudaFailure(
        code, std::move(operation));
  }
}

[[nodiscard]] std::size_t checked_bytes(
    std::size_t count,
    std::size_t width,
    const char* message) {
  if (count != 0U &&
      width > std::numeric_limits<std::size_t>::max() / count) {
    throw std::length_error(message);
  }
  return count * width;
}

[[nodiscard]] std::size_t checked_node_count(std::size_t point_count) {
  if (point_count == 0U ||
      point_count >
          std::numeric_limits<std::size_t>::max() / 2U + 1U) {
    throw std::length_error(
        "the Phase 14 anchored-pair node count overflows size_t");
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
          "a Phase 14 anchored-pair device allocation is invalid");
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
        "cudaGetDevice before Phase 14 anchored-pair proposal");
    if (previous_device_ != target_device) {
      check_cuda(
          cudaSetDevice(target_device),
          "cudaSetDevice for Phase 14 anchored-pair proposal");
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
          "cudaSetDevice restore after Phase 14 anchored-pair proposal");
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
        std::string{"the Phase 14 anchored-pair "} + label +
        " pointer is null or misaligned");
  }
  cudaPointerAttributes attributes{};
  check_cuda(
      cudaPointerGetAttributes(&attributes, pointer),
      std::string{"cudaPointerGetAttributes for Phase 14 anchored-pair "} +
          label);
  if (attributes.type != cudaMemoryTypeDevice ||
      attributes.device != expected_device) {
    throw std::invalid_argument(
        std::string{"the Phase 14 anchored-pair "} + label +
        " pointer is not owned by the traversal CUDA device");
  }
}

class Phase14AnchoredPairCandidateCudaResources final {
 public:
  Phase14AnchoredPairCandidateCudaResources(
      const Phase14AnchoredPairCandidateAdoptedTraversal& traversal,
      std::size_t maximum_query_count,
      std::size_t physical_transcript_record_capacity)
      : device_(traversal.cuda_device),
        maximum_query_count_(maximum_query_count),
        physical_transcript_record_capacity_(
            physical_transcript_record_capacity),
        traversal_owner_(traversal.owner),
        coordinate_bits_(traversal.device_coordinate_bits),
        morton_point_ids_(traversal.device_morton_point_ids),
        nodes_(traversal.device_nodes),
        point_count_(traversal.point_count),
        node_count_(traversal.node_count),
        source_snapshot_epoch_(traversal.source_snapshot_epoch) {
    cudaDeviceProp properties{};
    check_cuda(
        cudaGetDeviceProperties(&properties, device_),
        "cudaGetDeviceProperties for Phase 14 anchored-pair context");
    if (properties.major != 12 || properties.minor != 0 ||
        properties.warpSize != static_cast<int>(kWarpSize) ||
        properties.maxGridSize[0] <= 0 ||
        properties.multiProcessorCount <= 0 ||
        properties.maxThreadsPerBlock <
            static_cast<int>(kMaximumWarpsPerBlock * kWarpSize)) {
      throw std::runtime_error(
          "the Phase 14 anchored-pair proposal requires an sm_120 CUDA "
          "device with 32-lane warps");
    }
    maximum_grid_x_ =
        static_cast<unsigned int>(properties.maxGridSize[0]);
    multiprocessor_count_ =
        static_cast<unsigned int>(properties.multiProcessorCount);
    cudaStream_t created_stream = nullptr;
    check_cuda(
        cudaStreamCreateWithFlags(&created_stream, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags for Phase 14 anchored-pair context");
    try {
      queries_.allocate(
          maximum_query_count,
          "cudaMalloc Phase 14 anchored-pair fixed query arena");
      plans_.allocate(
          maximum_query_count,
          "cudaMalloc Phase 14 anchored-pair fixed plan arena");
      segments_.allocate(
          maximum_query_count,
          "cudaMalloc Phase 14 anchored-pair fixed segment arena");
      records_.allocate(
          physical_transcript_record_capacity,
          "cudaMalloc Phase 14 anchored-pair fixed transcript arena");
    } catch (...) {
      static_cast<void>(cudaStreamDestroy(created_stream));
      throw;
    }
    stream_ = created_stream;
  }

  Phase14AnchoredPairCandidateCudaResources(
      const Phase14AnchoredPairCandidateCudaResources&) = delete;
  Phase14AnchoredPairCandidateCudaResources& operator=(
      const Phase14AnchoredPairCandidateCudaResources&) = delete;

  ~Phase14AnchoredPairCandidateCudaResources() {
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

  void require_binding(
      const Phase14AnchoredPairCandidateAdoptedTraversal& traversal,
      std::size_t maximum_query_count,
      std::size_t physical_transcript_record_capacity) const {
    if (!traversal.owner ||
        traversal.owner.get() != traversal_owner_.get() ||
        traversal.cuda_device != device_ ||
        traversal.device_coordinate_bits != coordinate_bits_ ||
        traversal.device_morton_point_ids != morton_point_ids_ ||
        traversal.device_nodes != nodes_ ||
        traversal.point_count != point_count_ ||
        traversal.node_count != node_count_ ||
        traversal.source_snapshot_epoch != source_snapshot_epoch_ ||
        maximum_query_count != maximum_query_count_ ||
        physical_transcript_record_capacity !=
            physical_transcript_record_capacity_ ||
        queries_.count() != maximum_query_count_ ||
        plans_.count() != maximum_query_count_ ||
        segments_.count() != maximum_query_count_ ||
        records_.count() != physical_transcript_record_capacity_) {
      throw std::logic_error(
          "the Phase 14 anchored-pair context changed its immutable "
          "traversal binding or fixed capacities");
    }
  }

  [[nodiscard]] cudaStream_t stream() const noexcept { return stream_; }
  [[nodiscard]] unsigned int maximum_grid_x() const noexcept {
    return maximum_grid_x_;
  }
  [[nodiscard]] unsigned int multiprocessor_count() const noexcept {
    return multiprocessor_count_;
  }
  [[nodiscard]] Phase14AnchoredPairCandidateQueryInputRecord* queries()
      noexcept {
    return queries_.get();
  }
  [[nodiscard]] Phase14AnchoredPairCandidateQueryPlan* plans() noexcept {
    return plans_.get();
  }
  [[nodiscard]] Phase14AnchoredPairCandidateSegmentRecord* segments()
      noexcept {
    return segments_.get();
  }
  [[nodiscard]] Phase14AnchoredPairCandidateTranscriptRecord* records()
      noexcept {
    return records_.get();
  }

  void synchronize() {
    check_cuda(
        cudaStreamSynchronize(stream_),
        "cudaStreamSynchronize Phase 14 anchored-pair proposal");
  }

  void synchronize_after_failure() noexcept {
    if (stream_ != nullptr) {
      static_cast<void>(cudaStreamSynchronize(stream_));
    }
  }

 private:
  void abandon_all() noexcept {
    records_.abandon();
    segments_.abandon();
    plans_.abandon();
    queries_.abandon();
  }

  void reset_all() noexcept {
    records_.reset();
    segments_.reset();
    plans_.reset();
    queries_.reset();
  }

  int device_{-1};
  cudaStream_t stream_{nullptr};
  unsigned int maximum_grid_x_{};
  unsigned int multiprocessor_count_{};
  const std::size_t maximum_query_count_{};
  const std::size_t physical_transcript_record_capacity_{};
  std::shared_ptr<void> traversal_owner_;
  const std::uint64_t* coordinate_bits_{};
  const std::uint64_t* morton_point_ids_{};
  const void* nodes_{};
  std::size_t point_count_{};
  std::size_t node_count_{};
  std::uint64_t source_snapshot_epoch_{};
  DeviceBuffer<Phase14AnchoredPairCandidateQueryInputRecord> queries_;
  DeviceBuffer<Phase14AnchoredPairCandidateQueryPlan> plans_;
  DeviceBuffer<Phase14AnchoredPairCandidateSegmentRecord> segments_;
  DeviceBuffer<Phase14AnchoredPairCandidateTranscriptRecord> records_;
};

[[nodiscard]] Phase14AnchoredPairCandidateCudaResources& require_resources(
    Phase14AnchoredPairCandidateProposalContextState& context,
    const Phase14AnchoredPairCandidateAdoptedTraversal& traversal,
    std::size_t maximum_query_count,
    std::size_t physical_transcript_record_capacity) {
  std::shared_ptr<void>& opaque = context.cuda_resources();
  if (!opaque) {
    opaque =
        std::make_shared<Phase14AnchoredPairCandidateCudaResources>(
            traversal,
            maximum_query_count,
            physical_transcript_record_capacity);
  }
  auto* resources =
      static_cast<Phase14AnchoredPairCandidateCudaResources*>(opaque.get());
  resources->require_binding(
      traversal,
      maximum_query_count,
      physical_transcript_record_capacity);
  return *resources;
}

void validate_launch(
    const Phase14AnchoredPairCandidateProposalContextState& context,
    const Phase14AnchoredPairCandidateAdoptedTraversal& traversal,
    std::span<const Phase14AnchoredPairCandidateQueryInputRecord> queries,
    std::size_t maximum_query_count,
    std::size_t physical_transcript_record_capacity,
    std::size_t active_total_transcript_record_limit) {
  const std::size_t node_count = checked_node_count(traversal.point_count);
  const std::size_t coordinate_word_count = checked_bytes(
      traversal.point_count,
      kAxisCount,
      "the Phase 14 anchored-pair coordinate extent overflows size_t");
  if (!context.fixed_capacities_match(
          maximum_query_count, physical_transcript_record_capacity) ||
      !traversal.owner || !traversal.cuda_resident ||
      traversal.host_fake || traversal.cuda_device < 0 ||
      traversal.device_coordinate_bits == nullptr ||
      traversal.device_morton_point_ids == nullptr ||
      traversal.device_nodes == nullptr ||
      traversal.node_count != node_count ||
      traversal.coordinate_word_capacity < coordinate_word_count ||
      traversal.morton_point_id_capacity < traversal.point_count ||
      traversal.node_capacity < traversal.node_count ||
      traversal.source_snapshot_epoch == 0U ||
      maximum_query_count == 0U ||
      physical_transcript_record_capacity == 0U || queries.empty() ||
      queries.size() > maximum_query_count ||
      active_total_transcript_record_limit == 0U ||
      active_total_transcript_record_limit >
          physical_transcript_record_capacity) {
    throw std::invalid_argument(
        "the Phase 14 anchored-pair CUDA launch has invalid ownership, "
        "extents or fixed capacities");
  }
  static_cast<void>(checked_bytes(
      maximum_query_count,
      sizeof(Phase14AnchoredPairCandidateQueryInputRecord),
      "the Phase 14 anchored-pair fixed query bytes overflow size_t"));
  static_cast<void>(checked_bytes(
      maximum_query_count,
      sizeof(Phase14AnchoredPairCandidateQueryPlan),
      "the Phase 14 anchored-pair fixed plan bytes overflow size_t"));
  static_cast<void>(checked_bytes(
      maximum_query_count,
      sizeof(Phase14AnchoredPairCandidateSegmentRecord),
      "the Phase 14 anchored-pair fixed segment bytes overflow size_t"));
  static_cast<void>(checked_bytes(
      physical_transcript_record_capacity,
      sizeof(Phase14AnchoredPairCandidateTranscriptRecord),
      "the Phase 14 anchored-pair fixed transcript bytes overflow size_t"));
}

[[nodiscard]] __device__ std::uint64_t digest_word(
    std::uint64_t digest,
    std::uint64_t word) noexcept {
  constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);
#pragma unroll
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= kFnvPrime;
  }
  return digest;
}

[[nodiscard]] __device__ bool validate_query_warp(
    const Phase14AnchoredPairCandidateQueryInputRecord& query,
    std::uint64_t query_slot,
    std::uint64_t previous_anchor_point_id,
    std::uint64_t point_count,
    std::uint64_t node_count,
    std::uint64_t source_snapshot_epoch,
    std::uint64_t physical_transcript_record_capacity,
    unsigned int lane) noexcept {
  bool invalid = false;
  if (lane == 0U) {
    invalid =
        query.query_index != query_slot ||
        (query_slot != 0U &&
         previous_anchor_point_id >= query.anchor_point_id) ||
        query.anchor_point_id >= point_count ||
        query.maximum_closed_rank < UINT64_C(2) ||
        query.maximum_closed_rank > UINT64_C(11) ||
        query.witness_count > UINT64_C(64) ||
        query.maximum_node_visit_count == 0U ||
        query.maximum_transcript_record_count == 0U ||
        query.maximum_transcript_record_count >
            physical_transcript_record_capacity;
    if (!invalid) {
      if (query.cursor_kind == kCursorInitial) {
        invalid = query.cursor_node_index !=
                      phase14_anchored_pair_candidate_invalid_node_index ||
                  query.cursor_query_digest_fnv1a != 0U ||
                  query.cursor_source_snapshot_epoch != 0U;
      } else if (query.cursor_kind == kCursorActive) {
        invalid =
            query.cursor_node_index >= node_count ||
            query.cursor_query_digest_fnv1a != query.query_digest_fnv1a ||
            query.cursor_source_snapshot_epoch != source_snapshot_epoch;
      } else {
        invalid = true;
      }
    }
  }
  if (__any_sync(kFullWarpMask, invalid)) {
    return false;
  }

  for (std::uint64_t witness_index = lane;
       witness_index < UINT64_C(64);
       witness_index += kWarpSize) {
    const std::uint64_t witness = query.witnesses[witness_index];
    bool witness_invalid = false;
    if (witness_index < query.witness_count) {
      witness_invalid = witness >= point_count ||
                        witness == query.anchor_point_id;
      for (std::uint64_t prior = 0U;
           !witness_invalid && prior < witness_index;
           ++prior) {
        witness_invalid = query.witnesses[prior] == witness;
      }
    } else {
      witness_invalid =
          witness != phase14_anchored_pair_candidate_sentinel;
    }
    invalid = invalid || witness_invalid;
  }
  if (__any_sync(kFullWarpMask, invalid)) {
    return false;
  }

  if (lane == 0U) {
    std::uint64_t digest = kFnvOffsetBasis;
    digest = digest_word(digest, kQueryDigestDomain);
    digest = digest_word(digest, kProposalSchemaVersion);
    digest = digest_word(digest, source_snapshot_epoch);
    digest = digest_word(digest, query.anchor_point_id);
    digest = digest_word(digest, query.maximum_closed_rank);
    digest = digest_word(digest, query.witness_count);
    for (std::uint64_t witness_index = 0U;
         witness_index < query.witness_count;
         ++witness_index) {
      digest = digest_word(digest, query.witnesses[witness_index]);
    }
    invalid = digest != query.query_digest_fnv1a;
  }
  return !__any_sync(kFullWarpMask, invalid);
}

[[nodiscard]] __device__ bool validate_node_warp(
    const Phase14AnchoredPairCandidateDeviceNode* nodes,
    std::uint64_t node_index,
    std::uint64_t node_count,
    std::uint64_t point_count,
    unsigned int lane) noexcept {
  bool valid = true;
  if (lane == 0U) {
    if (node_index >= node_count) {
      valid = false;
    } else {
      const Phase14AnchoredPairCandidateDeviceNode& node =
          nodes[node_index];
      valid = node.leaf_begin < node.leaf_end &&
              node.leaf_end <= point_count;
      for (std::size_t axis = 0U; valid && axis < kAxisCount; ++axis) {
        valid = node.lower_point_ids[axis] < point_count &&
                node.upper_point_ids[axis] < point_count;
      }
      if (valid) {
        const std::uint64_t leaf_count =
            node.leaf_end - node.leaf_begin;
        valid = leaf_count <=
                    UINT64_MAX / UINT64_C(2) + UINT64_C(1) &&
                leaf_count <= node_index / UINT64_C(2) + UINT64_C(1);
        if (leaf_count == UINT64_C(1)) {
          valid =
              node.left_child ==
                  phase14_anchored_pair_candidate_invalid_node_index &&
              node.right_child ==
                  phase14_anchored_pair_candidate_invalid_node_index;
        } else if (valid) {
          valid = node_index != 0U &&
                  node.right_child == node_index - UINT64_C(1) &&
                  node.right_child < node_count &&
                  node.left_child < node.right_child;
          if (valid) {
            const Phase14AnchoredPairCandidateDeviceNode& left =
                nodes[node.left_child];
            const Phase14AnchoredPairCandidateDeviceNode& right =
                nodes[node.right_child];
            valid = left.leaf_begin == node.leaf_begin &&
                    right.leaf_end == node.leaf_end &&
                    left.leaf_end == right.leaf_begin &&
                    left.leaf_begin < left.leaf_end &&
                    right.leaf_begin < right.leaf_end;
            if (valid) {
              const std::uint64_t right_leaf_count =
                  right.leaf_end - right.leaf_begin;
              valid = right_leaf_count <= node_index / UINT64_C(2) &&
                      node.left_child ==
                          node_index - UINT64_C(2) * right_leaf_count;
            }
          }
        }
      }
    }
  }
  return __shfl_sync(kFullWarpMask, valid ? 1U : 0U, 0) != 0U;
}

[[nodiscard]] __device__ bool witness_strictly_separates_node(
    const Phase14AnchoredPairCandidateQueryInputRecord& query,
    std::uint64_t witness_index,
    const Phase14AnchoredPairCandidateDeviceNode& node,
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count) noexcept {
  using device::DeviceInterval;
  DeviceInterval sum = device::point_interval(UINT64_C(0));
  const std::uint64_t witness_point_id = query.witnesses[witness_index];
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const std::uint64_t anchor_bits = coordinate_bits[
        static_cast<std::uint64_t>(axis) * point_count +
        query.anchor_point_id];
    const std::uint64_t witness_bits = coordinate_bits[
        static_cast<std::uint64_t>(axis) * point_count + witness_point_id];
    if (anchor_bits == witness_bits) {
      continue;
    }
    const DeviceInterval direction = device::subtract_intervals(
        device::point_interval(witness_bits),
        device::point_interval(anchor_bits));
    if (!direction.valid ||
        (direction.lower <= 0.0 && direction.upper >= 0.0)) {
      return false;
    }
    const bool positive_direction = direction.lower > 0.0;
    const std::uint64_t bound_point_id =
        positive_direction ? node.lower_point_ids[axis]
                           : node.upper_point_ids[axis];
    const std::uint64_t bound_bits = coordinate_bits[
        static_cast<std::uint64_t>(axis) * point_count + bound_point_id];
    const DeviceInterval delta = device::subtract_intervals(
        device::point_interval(bound_bits),
        device::point_interval(witness_bits));
    const DeviceInterval product =
        device::multiply_intervals(direction, delta);
    sum = device::add_intervals(sum, product);
    if (!sum.valid) {
      return false;
    }
  }
  // Strictness is essential.  Equality, overflow and every interval crossing
  // zero remain fail-open proposals and therefore cannot prune.
  return sum.valid && sum.lower > 0.0;
}

[[nodiscard]] __device__ std::uint64_t lowest_bits(
    std::uint64_t mask,
    std::uint64_t count) noexcept {
  std::uint64_t selected = 0U;
  std::uint64_t remaining = mask;
  // count is maximum_closed_rank-1 and has already been validated in [1, 10].
  // Extracting the least set bit preserves exactly the former increasing-bit
  // order while bounding this loop by ten iterations instead of scanning 64.
  for (std::uint64_t selected_count = 0U;
       selected_count < count;
       ++selected_count) {
    if (remaining == 0U) {
      return UINT64_C(0);
    }
    const std::uint64_t least_bit =
        remaining & (~remaining + UINT64_C(1));
    selected |= least_bit;
    remaining ^= least_bit;
  }
  return selected;
}

[[nodiscard]] __device__ std::uint64_t proposed_prune_mask_warp(
    const Phase14AnchoredPairCandidateQueryInputRecord& query,
    const Phase14AnchoredPairCandidateDeviceNode& node,
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    unsigned int lane) noexcept {
  const std::uint64_t required =
      query.maximum_closed_rank - UINT64_C(1);
  if (query.witness_count < required) {
    return UINT64_C(0);
  }
  const bool first_positive =
      static_cast<std::uint64_t>(lane) < query.witness_count &&
      witness_strictly_separates_node(
          query, lane, node, coordinate_bits, point_count);
  const std::uint64_t first_mask = static_cast<std::uint64_t>(
      __ballot_sync(kFullWarpMask, first_positive));
  // Bank bits 0..31 precede every bit in the second wave.  Once the first
  // wave contains the required cardinality, selecting its lowest required
  // bits is therefore exactly the same canonical mask as selecting from the
  // former combined 64-bit ballot.  The branch is warp-uniform.
  if (static_cast<std::uint64_t>(__popc(
          static_cast<unsigned int>(first_mask))) >= required) {
    return lowest_bits(first_mask, required);
  }
  const std::uint64_t second_index =
      static_cast<std::uint64_t>(lane) + UINT64_C(32);
  const bool second_positive =
      second_index < query.witness_count &&
      witness_strictly_separates_node(
          query, second_index, node, coordinate_bits, point_count);
  const std::uint64_t mask =
      first_mask |
      (static_cast<std::uint64_t>(
           __ballot_sync(kFullWarpMask, second_positive))
       << 32U);
  return lowest_bits(mask, required);
}

template <bool WriteRecords>
[[nodiscard]] __device__ Phase14AnchoredPairCandidateTraversalOutcome
traverse_query_warp(
    const Phase14AnchoredPairCandidateQueryInputRecord& query,
    const std::uint64_t* coordinate_bits,
    const std::uint64_t* morton_point_ids,
    const Phase14AnchoredPairCandidateDeviceNode* nodes,
    std::uint64_t point_count,
    std::uint64_t node_count,
    std::uint64_t record_limit,
    bool globally_truncated,
    std::uint64_t record_begin,
    std::uint64_t active_total_record_limit,
    Phase14AnchoredPairCandidateTranscriptRecord* records,
    unsigned int lane) noexcept {
  Phase14AnchoredPairCandidateTraversalOutcome outcome;
  std::uint64_t cursor = query.cursor_kind == kCursorInitial
                             ? node_count - UINT64_C(1)
                             : query.cursor_node_index;
  outcome.next_cursor_node_index = cursor;

  if (globally_truncated && record_limit == 0U) {
    outcome.status = kStatusCapacity;
    outcome.stop_reason = kStopRecords;
    return outcome;
  }

  while (true) {
    if (cursor >= node_count) {
      outcome.failure_code = kFailureNodeOutOfRange;
      return outcome;
    }
    if (!validate_node_warp(
            nodes, cursor, node_count, point_count, lane)) {
      outcome.failure_code = kFailureTraversal;
      return outcome;
    }
    const Phase14AnchoredPairCandidateDeviceNode& node = nodes[cursor];
    const std::uint64_t witness_mask = proposed_prune_mask_warp(
        query, node, coordinate_bits, point_count, lane);
    const bool prune = witness_mask != 0U;
    const std::uint64_t leaf_count = node.leaf_end - node.leaf_begin;
    bool candidate = false;
    bool point_id_valid = true;
    if (!prune && leaf_count == UINT64_C(1) && lane == 0U) {
      const std::uint64_t point_id = morton_point_ids[node.leaf_begin];
      point_id_valid = point_id < point_count;
      candidate = point_id_valid && point_id > query.anchor_point_id;
    }
    point_id_valid =
        __shfl_sync(kFullWarpMask, point_id_valid ? 1U : 0U, 0) != 0U;
    candidate =
        __shfl_sync(kFullWarpMask, candidate ? 1U : 0U, 0) != 0U;
    if (!point_id_valid) {
      outcome.failure_code = kFailureTraversal;
      return outcome;
    }
    const bool emit = prune || candidate;
    if (emit && outcome.record_count == record_limit) {
      outcome.status = kStatusCapacity;
      outcome.stop_reason = kStopRecords;
      outcome.next_cursor_node_index = cursor;
      return outcome;
    }

    ++outcome.node_visit_count;
    if (emit) {
      if constexpr (WriteRecords) {
        if (record_begin > active_total_record_limit ||
            outcome.record_count >=
                active_total_record_limit - record_begin) {
          outcome.failure_code = kFailureTraversal;
          return outcome;
        }
        if (lane == 0U) {
          records[record_begin + outcome.record_count] =
              Phase14AnchoredPairCandidateTranscriptRecord{
                  cursor, witness_mask};
        }
      }
      ++outcome.record_count;
    }

    bool traversal_complete = false;
    std::uint64_t next =
        phase14_anchored_pair_candidate_invalid_node_index;
    if (prune) {
      const std::uint64_t subtree_node_count =
          UINT64_C(2) * leaf_count - UINT64_C(1);
      if (cursor + UINT64_C(1) < subtree_node_count) {
        outcome.failure_code = kFailureTraversal;
        return outcome;
      }
      traversal_complete = cursor + UINT64_C(1) == subtree_node_count;
      if (!traversal_complete) {
        next = cursor - subtree_node_count;
      }
    } else {
      traversal_complete = cursor == 0U;
      if (!traversal_complete) {
        next = cursor - UINT64_C(1);
      }
    }
    if (traversal_complete) {
      // Completion wins when the last reachable node emits exactly the last
      // allowed record: there is then no omitted node to resume.
      outcome.next_cursor_kind = kCursorComplete;
      outcome.next_cursor_node_index =
          phase14_anchored_pair_candidate_invalid_node_index;
      outcome.status = kStatusComplete;
      outcome.stop_reason = kStopNone;
      return outcome;
    }
    outcome.next_cursor_kind = kCursorActive;
    outcome.next_cursor_node_index = next;

    if (emit && outcome.record_count == record_limit) {
      // Here a successor still exists, so reaching the exact record limit is
      // a genuine capacity stop and the cursor names the first omitted node.
      outcome.status = kStatusCapacity;
      outcome.stop_reason = kStopRecords;
      return outcome;
    }
    if (outcome.node_visit_count == query.maximum_node_visit_count) {
      outcome.status = kStatusBudget;
      outcome.stop_reason = kStopNodes;
      return outcome;
    }
    cursor = next;
  }
}

__global__ void count_anchored_pair_candidate_records_kernel(
    const Phase14AnchoredPairCandidateQueryInputRecord* queries,
    std::uint64_t query_count,
    const std::uint64_t* coordinate_bits,
    const std::uint64_t* morton_point_ids,
    const Phase14AnchoredPairCandidateDeviceNode* nodes,
    std::uint64_t point_count,
    std::uint64_t node_count,
    std::uint64_t source_snapshot_epoch,
    std::uint64_t physical_transcript_record_capacity,
    Phase14AnchoredPairCandidateQueryPlan* plans) {
  const unsigned int lane = threadIdx.x & (kWarpSize - 1U);
  const std::uint64_t block_warp = threadIdx.x / kWarpSize;
  const std::uint64_t block_warp_count = blockDim.x / kWarpSize;
  std::uint64_t query_slot =
      static_cast<std::uint64_t>(blockIdx.x) * block_warp_count +
      block_warp;
  const std::uint64_t query_stride =
      static_cast<std::uint64_t>(gridDim.x) * block_warp_count;
  while (query_slot < query_count) {
    const Phase14AnchoredPairCandidateQueryInputRecord& query =
        queries[query_slot];
    const std::uint64_t previous_anchor_point_id =
        query_slot == 0U
            ? phase14_anchored_pair_candidate_sentinel
            : queries[query_slot - UINT64_C(1)].anchor_point_id;
    Phase14AnchoredPairCandidateTraversalOutcome outcome;
    if (!validate_query_warp(
            query,
            query_slot,
            previous_anchor_point_id,
            point_count,
            node_count,
            source_snapshot_epoch,
            physical_transcript_record_capacity,
            lane)) {
      outcome.failure_code = kFailureMalformedQuery;
    } else {
      outcome = traverse_query_warp<false>(
          query,
          coordinate_bits,
          morton_point_ids,
          nodes,
          point_count,
          node_count,
          query.maximum_transcript_record_count,
          false,
          0U,
          physical_transcript_record_capacity,
          nullptr,
          lane);
    }
    if (lane == 0U) {
      plans[query_slot] = Phase14AnchoredPairCandidateQueryPlan{
          outcome.record_count, 0U, 0U, outcome.failure_code};
    }
    query_slot += query_stride;
  }
}

__global__ void prefix_clamp_anchored_pair_candidate_records_kernel(
    Phase14AnchoredPairCandidateQueryPlan* plans,
    std::uint64_t query_count,
    std::uint64_t active_total_record_limit) {
  if (blockIdx.x != 0U || threadIdx.x != 0U) {
    return;
  }
  std::uint64_t prefix = 0U;
  for (std::uint64_t query_slot = 0U;
       query_slot < query_count;
       ++query_slot) {
    Phase14AnchoredPairCandidateQueryPlan& plan = plans[query_slot];
    plan.record_begin = prefix;
    const std::uint64_t remaining = active_total_record_limit - prefix;
    plan.allocated_record_count =
        plan.raw_record_count < remaining ? plan.raw_record_count
                                          : remaining;
    prefix += plan.allocated_record_count;
  }
}

__global__ void replay_anchored_pair_candidate_records_kernel(
    const Phase14AnchoredPairCandidateQueryInputRecord* queries,
    std::uint64_t query_count,
    const std::uint64_t* coordinate_bits,
    const std::uint64_t* morton_point_ids,
    const Phase14AnchoredPairCandidateDeviceNode* nodes,
    std::uint64_t point_count,
    std::uint64_t node_count,
    std::uint64_t source_snapshot_epoch,
    std::uint64_t buffer_epoch,
    std::uint64_t physical_transcript_record_capacity,
    std::uint64_t active_total_record_limit,
    const Phase14AnchoredPairCandidateQueryPlan* plans,
    Phase14AnchoredPairCandidateSegmentRecord* segments,
    Phase14AnchoredPairCandidateTranscriptRecord* records) {
  const unsigned int lane = threadIdx.x & (kWarpSize - 1U);
  const std::uint64_t block_warp = threadIdx.x / kWarpSize;
  const std::uint64_t block_warp_count = blockDim.x / kWarpSize;
  std::uint64_t query_slot =
      static_cast<std::uint64_t>(blockIdx.x) * block_warp_count +
      block_warp;
  const std::uint64_t query_stride =
      static_cast<std::uint64_t>(gridDim.x) * block_warp_count;
  while (query_slot < query_count) {
    const Phase14AnchoredPairCandidateQueryInputRecord& query =
        queries[query_slot];
    const std::uint64_t previous_anchor_point_id =
        query_slot == 0U
            ? phase14_anchored_pair_candidate_sentinel
            : queries[query_slot - UINT64_C(1)].anchor_point_id;
    const Phase14AnchoredPairCandidateQueryPlan plan = plans[query_slot];
    Phase14AnchoredPairCandidateTraversalOutcome outcome;
    if (plan.failure_code != kFailureNone) {
      outcome.failure_code = plan.failure_code;
    } else if (!validate_query_warp(
                   query,
                   query_slot,
                   previous_anchor_point_id,
                   point_count,
                   node_count,
                   source_snapshot_epoch,
                   physical_transcript_record_capacity,
                   lane) ||
               plan.record_begin > active_total_record_limit ||
               plan.allocated_record_count >
                   active_total_record_limit - plan.record_begin ||
               plan.allocated_record_count > plan.raw_record_count) {
      outcome.failure_code = kFailureMalformedQuery;
    } else {
      const bool globally_truncated =
          plan.allocated_record_count < plan.raw_record_count;
      outcome = traverse_query_warp<true>(
          query,
          coordinate_bits,
          morton_point_ids,
          nodes,
          point_count,
          node_count,
          globally_truncated ? plan.allocated_record_count
                             : query.maximum_transcript_record_count,
          globally_truncated,
          plan.record_begin,
          active_total_record_limit,
          records,
          lane);
      if (outcome.failure_code == kFailureNone &&
          outcome.record_count != plan.allocated_record_count) {
        outcome.failure_code = kFailureTraversal;
      }
    }
    if (lane == 0U) {
      segments[query_slot] = Phase14AnchoredPairCandidateSegmentRecord{
          query.query_index,
          query.query_digest_fnv1a,
          buffer_epoch,
          source_snapshot_epoch,
          query.cursor_kind,
          query.cursor_node_index,
          outcome.next_cursor_kind,
          outcome.next_cursor_node_index,
          outcome.status,
          outcome.stop_reason,
          outcome.node_visit_count,
          plan.record_begin,
          outcome.record_count,
          outcome.failure_code};
    }
    query_slot += query_stride;
  }
}

struct Phase14AnchoredPairCandidateLaunchShape {
  unsigned int block_count{};
  unsigned int thread_count{};
};

[[nodiscard]] Phase14AnchoredPairCandidateLaunchShape launch_shape(
    std::size_t query_count,
    unsigned int maximum_grid_x,
    unsigned int multiprocessor_count) {
  if (multiprocessor_count == 0U) {
    throw std::runtime_error(
        "the Phase 14 anchored-pair multiprocessor count is zero");
  }
  const std::size_t queries_per_multiprocessor =
      query_count / static_cast<std::size_t>(multiprocessor_count);
  unsigned int warps_per_block = 1U;
  if (queries_per_multiprocessor >= 8U) {
    warps_per_block = 8U;
  } else if (queries_per_multiprocessor >= 4U) {
    warps_per_block = 4U;
  } else if (queries_per_multiprocessor >= 2U) {
    warps_per_block = 2U;
  }
  const std::size_t requested =
      query_count / warps_per_block +
      (query_count % warps_per_block == 0U ? 0U : 1U);
  const std::size_t bounded = std::min(
      requested, static_cast<std::size_t>(maximum_grid_x));
  if (bounded == 0U ||
      bounded >
          static_cast<std::size_t>(
              std::numeric_limits<unsigned int>::max())) {
    throw std::runtime_error(
        "the Phase 14 anchored-pair warp grid is invalid");
  }
  return Phase14AnchoredPairCandidateLaunchShape{
      static_cast<unsigned int>(bounded),
      warps_per_block * kWarpSize};
}

void validate_returned_segments(
    const std::vector<Phase14AnchoredPairCandidateSegmentRecord>& segments,
    std::size_t active_total_transcript_record_limit,
    std::size_t& compact_record_count) {
  compact_record_count = 0U;
  for (const Phase14AnchoredPairCandidateSegmentRecord& segment : segments) {
    if (segment.failure_code != kFailureNone) {
      if (segment.failure_code == kFailureMalformedQuery) {
        throw std::runtime_error(
            "the Phase 14 anchored-pair device rejected a malformed query");
      }
      if (segment.failure_code == kFailureNodeOutOfRange) {
        throw std::runtime_error(
            "the Phase 14 anchored-pair device left its node domain");
      }
      if (segment.failure_code == kFailureTraversal) {
        throw std::runtime_error(
            "the Phase 14 anchored-pair device rejected the certified "
            "postorder traversal");
      }
      throw std::runtime_error(
          "the Phase 14 anchored-pair device returned an unknown failure");
    }
    if (segment.record_begin !=
            static_cast<std::uint64_t>(compact_record_count) ||
        segment.record_count >
            static_cast<std::uint64_t>(
                active_total_transcript_record_limit -
                compact_record_count)) {
      throw std::runtime_error(
          "the Phase 14 anchored-pair device returned a broken compact "
          "segment partition");
    }
    compact_record_count +=
        static_cast<std::size_t>(segment.record_count);
  }
}

}  // namespace

Phase14AnchoredPairCandidateDeviceBatch
propose_phase14_anchored_pair_candidates_on_gpu(
    Phase14AnchoredPairCandidateProposalContextState& context,
    const Phase14AnchoredPairCandidateAdoptedTraversal& traversal,
    std::span<const Phase14AnchoredPairCandidateQueryInputRecord> queries,
    std::size_t maximum_query_count,
    std::size_t physical_transcript_record_capacity,
    std::size_t active_total_transcript_record_limit) {
  validate_launch(
      context,
      traversal,
      queries,
      maximum_query_count,
      physical_transcript_record_capacity,
      active_total_transcript_record_limit);
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
      alignof(Phase14AnchoredPairCandidateDeviceNode),
      "node");
  Phase14AnchoredPairCandidateCudaResources& cuda = require_resources(
      context,
      traversal,
      maximum_query_count,
      physical_transcript_record_capacity);

  const std::size_t query_bytes = checked_bytes(
      queries.size(),
      sizeof(Phase14AnchoredPairCandidateQueryInputRecord),
      "the Phase 14 anchored-pair active query bytes overflow size_t");
  const std::size_t segment_bytes = checked_bytes(
      queries.size(),
      sizeof(Phase14AnchoredPairCandidateSegmentRecord),
      "the Phase 14 anchored-pair active segment bytes overflow size_t");
  const std::size_t transcript_bytes = checked_bytes(
      active_total_transcript_record_limit,
      sizeof(Phase14AnchoredPairCandidateTranscriptRecord),
      "the Phase 14 anchored-pair active transcript bytes overflow size_t");
  const std::uint64_t query_count = checked_u64(
      queries.size(),
      "the Phase 14 anchored-pair query count does not fit uint64");
  const std::uint64_t point_count = checked_u64(
      traversal.point_count,
      "the Phase 14 anchored-pair point count does not fit uint64");
  const std::uint64_t node_count = checked_u64(
      traversal.node_count,
      "the Phase 14 anchored-pair node count does not fit uint64");
  const std::uint64_t physical_record_capacity = checked_u64(
      physical_transcript_record_capacity,
      "the Phase 14 anchored-pair transcript capacity does not fit uint64");
  const std::uint64_t active_record_limit = checked_u64(
      active_total_transcript_record_limit,
      "the Phase 14 anchored-pair active transcript limit does not fit "
      "uint64");
  const std::uint64_t buffer_epoch = context.advance_epoch();
  const auto* nodes =
      static_cast<const Phase14AnchoredPairCandidateDeviceNode*>(
          traversal.device_nodes);
  const Phase14AnchoredPairCandidateLaunchShape shape = launch_shape(
      queries.size(),
      cuda.maximum_grid_x(),
      cuda.multiprocessor_count());

  try {
    check_cuda(
        cudaMemcpyAsync(
            cuda.queries(),
            queries.data(),
            query_bytes,
            cudaMemcpyHostToDevice,
            cuda.stream()),
        "cudaMemcpyAsync Phase 14 anchored-pair queries host-to-device");
    check_cuda(
        cudaMemsetAsync(
            cuda.segments(), 0xff, segment_bytes, cuda.stream()),
        "cudaMemsetAsync Phase 14 anchored-pair active segments");
    check_cuda(
        cudaMemsetAsync(
            cuda.records(), 0xff, transcript_bytes, cuda.stream()),
        "cudaMemsetAsync Phase 14 anchored-pair active transcript");

    count_anchored_pair_candidate_records_kernel<<<
        shape.block_count, shape.thread_count, 0U, cuda.stream()>>>(
        cuda.queries(),
        query_count,
        traversal.device_coordinate_bits,
        traversal.device_morton_point_ids,
        nodes,
        point_count,
        node_count,
        traversal.source_snapshot_epoch,
        physical_record_capacity,
        cuda.plans());
    check_cuda(
        cudaPeekAtLastError(),
        "Phase 14 anchored-pair count kernel launch");

    prefix_clamp_anchored_pair_candidate_records_kernel<<<
        1U, 1U, 0U, cuda.stream()>>>(
        cuda.plans(), query_count, active_record_limit);
    check_cuda(
        cudaPeekAtLastError(),
        "Phase 14 anchored-pair prefix/clamp kernel launch");

    replay_anchored_pair_candidate_records_kernel<<<
        shape.block_count, shape.thread_count, 0U, cuda.stream()>>>(
        cuda.queries(),
        query_count,
        traversal.device_coordinate_bits,
        traversal.device_morton_point_ids,
        nodes,
        point_count,
        node_count,
        traversal.source_snapshot_epoch,
        buffer_epoch,
        physical_record_capacity,
        active_record_limit,
        cuda.plans(),
        cuda.segments(),
        cuda.records());
    check_cuda(
        cudaPeekAtLastError(),
        "Phase 14 anchored-pair replay/write kernel launch");

    Phase14AnchoredPairCandidateDeviceBatch batch;
    batch.segments.resize(queries.size());
    batch.records.resize(active_total_transcript_record_limit);
    check_cuda(
        cudaMemcpyAsync(
            batch.segments.data(),
            cuda.segments(),
            segment_bytes,
            cudaMemcpyDeviceToHost,
            cuda.stream()),
        "cudaMemcpyAsync Phase 14 anchored-pair segments device-to-host");
    check_cuda(
        cudaMemcpyAsync(
            batch.records.data(),
            cuda.records(),
            transcript_bytes,
            cudaMemcpyDeviceToHost,
            cuda.stream()),
        "cudaMemcpyAsync Phase 14 anchored-pair transcript device-to-host");
    cuda.synchronize();

    std::size_t compact_record_count = 0U;
    validate_returned_segments(
        batch.segments,
        active_total_transcript_record_limit,
        compact_record_count);
    batch.records.resize(compact_record_count);
    batch.segment_count = batch.segments.size();
    batch.record_count = batch.records.size();
    batch.host_to_device_query_byte_count = query_bytes;
    batch.initialized_segment_byte_count = segment_bytes;
    batch.initialized_transcript_byte_count = transcript_bytes;
    batch.device_to_host_segment_byte_count = segment_bytes;
    batch.device_to_host_transcript_byte_count = transcript_bytes;
    batch.kernel_launch_count = 3U;
    batch.synchronization_count = 1U;
    batch.compaction_kernel_launch_count = 1U;
    batch.physical_query_capacity = maximum_query_count;
    batch.physical_transcript_record_capacity =
        physical_transcript_record_capacity;
    batch.active_total_transcript_record_limit =
        active_total_transcript_record_limit;
    batch.buffer_epoch = buffer_epoch;
    batch.source_snapshot_epoch = traversal.source_snapshot_epoch;
    batch.cuda_device = traversal.cuda_device;
    batch.cuda_path_qualified = true;
    batch.execution_kind =
        Phase14AnchoredPairCandidateExecutionKind::cuda;
    batch.compaction_kind =
        Phase14AnchoredPairCandidateCompactionKind::
            device_serial_prefix_clamp;
    batch.proposal_digest_fnv1a =
        phase14_anchored_pair_candidate_batch_digest(batch);
    guard.restore();
    return batch;
  } catch (...) {
    cuda.synchronize_after_failure();
    throw;
  }
}

}  // namespace morsehgp3d::gpu::detail
