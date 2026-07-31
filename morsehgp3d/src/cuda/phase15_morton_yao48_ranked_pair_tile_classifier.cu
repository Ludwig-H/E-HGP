#include "phase15_morton_yao48_ranked_pair_tile_classifier_internal.hpp"

#include "phase15_exact_diametral_phi_fixed.cuh"
#include "phase2b_interval.cuh"

#include <cuda_runtime.h>

#include <cub/device/device_radix_sort.cuh>
#include <cub/device/device_scan.cuh>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#if !defined(__CUDACC__)
#error "phase15_morton_yao48_ranked_pair_tile_classifier.cu requires NVCC"
#endif

#if __CUDACC_VER_MAJOR__ != 12 || __CUDACC_VER_MINOR__ != 9
#error "The Phase 15 resident ranked-pair classifier requires CUDA 12.9"
#endif

#if defined(__FAST_MATH__) || defined(__CUDA_FAST_MATH__)
#error "Fast math is forbidden for the Phase 15 ranked-pair classifier"
#endif

#if defined(__CUDA_ARCH__) && defined(__CUDA_FTZ) && __CUDA_FTZ != 0
#error "Flush-to-zero is forbidden for the Phase 15 ranked-pair classifier"
#endif

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ != 1200
#error "The Phase 15 ranked-pair classifier must contain only sm_120 code"
#endif

namespace morsehgp3d::gpu::detail {
namespace {

constexpr std::size_t kAxisCount = 3U;
constexpr std::size_t kMaximumClosedRank = 11U;
constexpr std::size_t kMaximumNonSupportCount = kMaximumClosedRank - 2U;
constexpr unsigned int kThreadsPerBlock = 128U;
constexpr std::uint64_t kInvalid = UINT64_MAX;
constexpr std::uint64_t kSortKeyRejected = UINT64_MAX;

constexpr std::uint64_t kAnchorStatusChunkReady = static_cast<std::uint64_t>(
    Phase15MortonYao48DeviceTiledAnchorStatus::chunk_ready);
constexpr std::uint64_t kAnchorStatusComplete = static_cast<std::uint64_t>(
    Phase15MortonYao48DeviceTiledAnchorStatus::complete);
constexpr std::uint64_t kAnchorYieldNone = static_cast<std::uint64_t>(
    Phase15MortonYao48DeviceTiledYieldReason::none);
constexpr std::uint64_t kAnchorStopNone = static_cast<std::uint64_t>(
    MortonYao48DeviceTiledPairFrontierStopReason::none);
constexpr std::uint64_t kAnchorFailureNone = static_cast<std::uint64_t>(
    Phase15MortonYao48DeviceTiledFailureCode::none);

constexpr std::uint64_t kStatusChunkCommitted = static_cast<std::uint64_t>(
    MortonYao48RankedPairTileClassifierStatus::chunk_committed);
constexpr std::uint64_t kStatusFrontierClosed = static_cast<std::uint64_t>(
    MortonYao48RankedPairTileClassifierStatus::frontier_closed);
constexpr std::uint64_t kStatusCapacityExhausted =
    static_cast<std::uint64_t>(
        MortonYao48RankedPairTileClassifierStatus::capacity_exhausted);
constexpr std::uint64_t kStatusExactPredicateFailure =
    static_cast<std::uint64_t>(
        MortonYao48RankedPairTileClassifierStatus::exact_predicate_failure);
constexpr std::uint64_t kStopNone = static_cast<std::uint64_t>(
    MortonYao48RankedPairTileClassifierStopReason::none);
constexpr std::uint64_t kStopRecordCapacity = static_cast<std::uint64_t>(
    MortonYao48RankedPairTileClassifierStopReason::catalog_record_capacity);
constexpr std::uint64_t kStopPayloadCapacity = static_cast<std::uint64_t>(
    MortonYao48RankedPairTileClassifierStopReason::payload_point_id_capacity);
constexpr std::uint64_t kStopFallbackCapacity = static_cast<std::uint64_t>(
    MortonYao48RankedPairTileClassifierStopReason::exact_fallback_capacity);
constexpr std::uint64_t kStopExactPredicate = static_cast<std::uint64_t>(
    MortonYao48RankedPairTileClassifierStopReason::exact_predicate_failure);
constexpr std::uint64_t kFailureNone = static_cast<std::uint64_t>(
    Phase15MortonYao48RankedPairTileClassifierFailureCode::none);
constexpr std::uint64_t kFailureCapacity = static_cast<std::uint64_t>(
    Phase15MortonYao48RankedPairTileClassifierFailureCode::capacity_exhausted);
constexpr std::uint64_t kFailureMalformed = static_cast<std::uint64_t>(
    Phase15MortonYao48RankedPairTileClassifierFailureCode::malformed_input);
constexpr std::uint64_t kFailureExactPredicate = static_cast<std::uint64_t>(
    Phase15MortonYao48RankedPairTileClassifierFailureCode::
        exact_predicate_failure);
constexpr std::uint64_t kFailureInvariant = static_cast<std::uint64_t>(
    Phase15MortonYao48RankedPairTileClassifierFailureCode::internal_invariant);

enum class CandidateOutcome : std::uint8_t {
  inactive = 0U,
  accepted = 1U,
  above_window = 2U,
  exact_fallback = 3U,
  malformed = 4U,
};

// Private ABI retained by MortonLbvhDeviceTraversalLease.  It is repeated
// here instead of exporting the traversal representation through the product
// API.  The certified tree is postorder: every subtree occupies one contiguous
// interval and the right child of an internal node is node_index - 1.
struct Phase15MortonYao48RankedPairNode {
  std::uint64_t lower_point_ids[kAxisCount];
  std::uint64_t upper_point_ids[kAxisCount];
  std::uint64_t left_child;
  std::uint64_t right_child;
  std::uint64_t leaf_begin;
  std::uint64_t leaf_end;
};

static_assert(sizeof(Phase15MortonYao48RankedPairNode) == 80U);
static_assert(alignof(Phase15MortonYao48RankedPairNode) == 8U);
static_assert(std::is_standard_layout_v<Phase15MortonYao48RankedPairNode>);
static_assert(
    std::is_trivially_copyable_v<Phase15MortonYao48RankedPairNode>);
static_assert(
    offsetof(Phase15MortonYao48RankedPairNode, lower_point_ids) == 0U);
static_assert(
    offsetof(Phase15MortonYao48RankedPairNode, upper_point_ids) == 24U);
static_assert(offsetof(Phase15MortonYao48RankedPairNode, left_child) == 48U);
static_assert(offsetof(Phase15MortonYao48RankedPairNode, right_child) == 56U);
static_assert(offsetof(Phase15MortonYao48RankedPairNode, leaf_begin) == 64U);
static_assert(offsetof(Phase15MortonYao48RankedPairNode, leaf_end) == 72U);

struct alignas(16) CandidateClassification {
  std::uint64_t support_u{};
  std::uint64_t support_v{};
  std::uint64_t payload_point_ids[kMaximumNonSupportCount]{};
  std::uint8_t closed_rank{};
  std::uint8_t strict_count{};
  std::uint8_t shell_count{};
  std::uint8_t outcome{};
  std::uint32_t reserved_zero{};
};

static_assert(sizeof(CandidateClassification) == 96U);
static_assert(std::is_trivially_copyable_v<CandidateClassification>);

struct alignas(16) DeviceReceiptControl {
  std::uint64_t processed_candidate_count{};
  std::uint64_t above_window_count{};
  std::uint64_t exact_fallback_count{};
  std::uint64_t malformed_count{};
  std::uint64_t invariant_count{};
  std::uint64_t transaction_accepted_record_count{};
  std::uint64_t transaction_strict_payload_count{};
  std::uint64_t transaction_shell_payload_count{};
  std::uint64_t required_catalog_record_count{};
  std::uint64_t required_payload_point_id_count{};
  std::uint64_t required_exact_fallback_count{};
  std::uint64_t cumulative_candidate_count{};
  std::uint64_t cumulative_accepted_record_count{};
  std::uint64_t cumulative_above_window_count{};
  std::uint64_t cumulative_unresolved_count{};
  std::uint64_t cumulative_strict_payload_count{};
  std::uint64_t cumulative_shell_payload_count{};
  std::uint64_t cumulative_exact_fallback_count{};
  std::uint64_t status{};
  std::uint64_t stop_reason{};
  std::uint64_t failure_code{};
  std::uint64_t commit_allowed{};
};

static_assert(std::is_trivially_copyable_v<DeviceReceiptControl>);

class CudaFailure final : public std::runtime_error {
 public:
  CudaFailure(cudaError_t code, std::string operation)
      : std::runtime_error(message(code, std::move(operation))) {}

 private:
  [[nodiscard]] static std::string message(
      cudaError_t code,
      std::string operation) {
    const char* description = cudaGetErrorString(code);
    return std::move(operation) + " failed: " +
           (description == nullptr ? std::string{"unknown CUDA error"}
                                   : std::string{description});
  }
};

void check_cuda(cudaError_t code, std::string operation) {
  if (code != cudaSuccess) {
    throw CudaFailure(code, std::move(operation));
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
  if (point_count < 2U ||
      point_count > std::numeric_limits<std::size_t>::max() / 2U + 1U) {
    throw std::length_error(
        "the ranked-pair classifier node extent overflows size_t");
  }
  return 2U * point_count - 1U;
}

[[nodiscard]] int checked_cub_count(
    std::size_t count,
    const char* message) {
  if (count == 0U ||
      count > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
    throw std::length_error(message);
  }
  return static_cast<int>(count);
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
          "a ranked-pair classifier device buffer was allocated twice");
    }
    if (count == 0U) {
      return;
    }
    if (count > std::numeric_limits<std::size_t>::max() / sizeof(Value)) {
      throw std::length_error(
          "a ranked-pair classifier allocation extent overflows size_t");
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
  explicit DeviceGuard(int requested_device) {
    check_cuda(
        cudaGetDevice(&previous_device_),
        "cudaGetDevice before resident ranked-pair classification");
    if (previous_device_ != requested_device) {
      check_cuda(
          cudaSetDevice(requested_device),
          "cudaSetDevice for resident ranked-pair classification");
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

  void restore() {
    if (restore_required_) {
      check_cuda(
          cudaSetDevice(previous_device_),
          "cudaSetDevice restore after resident ranked-pair classification");
      restore_required_ = false;
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
        std::string{"the resident ranked-pair "} + label +
        " pointer is null or misaligned");
  }
  cudaPointerAttributes attributes{};
  check_cuda(
      cudaPointerGetAttributes(&attributes, pointer),
      std::string{"cudaPointerGetAttributes for ranked-pair "} + label);
  if (attributes.type != cudaMemoryTypeDevice ||
      attributes.device != expected_device) {
    throw std::invalid_argument(
        std::string{"the resident ranked-pair "} + label +
        " pointer is not owned by the expected CUDA device");
  }
}

struct CatalogBuffers final {
  DeviceBuffer<std::uint64_t> support_u;
  DeviceBuffer<std::uint64_t> support_v;
  DeviceBuffer<std::uint8_t> closed_rank;
  DeviceBuffer<std::uint64_t> rank_offsets;
  DeviceBuffer<std::uint64_t> strict_offsets;
  DeviceBuffer<std::uint64_t> strict_point_ids;
  DeviceBuffer<std::uint64_t> shell_offsets;
  DeviceBuffer<std::uint64_t> shell_point_ids;
};

class RankedPairCudaResources final {
 public:
  RankedPairCudaResources(
      const Phase15MortonYao48DeviceCandidateTilePrivateViews& views,
      const Phase15MortonYao48RankedPairTileClassifierRequest& request,
      std::size_t anchor_capacity,
      std::size_t candidate_workspace_capacity)
      : device_(views.cuda_device),
        source_owner_identity_(views.source_owner_identity),
        source_cloud_identity_(views.source_cloud_identity),
        source_snapshot_epoch_(request.source_snapshot_epoch),
        point_count_(request.point_count),
        certified_node_count_(request.certified_node_count),
        config_(request.fixed_config),
        anchor_workspace_capacity_(anchor_capacity),
        candidate_workspace_capacity_(candidate_workspace_capacity) {
    cudaStream_t created_stream = nullptr;
    check_cuda(
        cudaStreamCreateWithFlags(&created_stream, cudaStreamNonBlocking),
        "cudaStreamCreateWithFlags for resident ranked-pair classifier");
    stream_ = created_stream;
    try {
      allocate_catalog(catalogs_[0], "A");
      allocate_catalog(catalogs_[1], "B");
      anchor_counts_.allocate(
          anchor_workspace_capacity_ + 1U,
          "cudaMalloc ranked-pair anchor counts");
      anchor_offsets_.allocate(
          anchor_workspace_capacity_ + 1U,
          "cudaMalloc ranked-pair anchor offsets");
      last_partner_positions_.allocate(
          anchor_workspace_capacity_,
          "cudaMalloc ranked-pair continuation witnesses");
      classifications_.allocate(
          candidate_workspace_capacity_,
          "cudaMalloc ranked-pair classifications");
      accepted_counts_.allocate(
          candidate_workspace_capacity_ + 1U,
          "cudaMalloc ranked-pair accepted counts");
      accepted_offsets_.allocate(
          candidate_workspace_capacity_ + 1U,
          "cudaMalloc ranked-pair accepted offsets");
      strict_counts_.allocate(
          candidate_workspace_capacity_ + 1U,
          "cudaMalloc ranked-pair strict counts");
      strict_offsets_.allocate(
          candidate_workspace_capacity_ + 1U,
          "cudaMalloc ranked-pair strict offsets");
      shell_counts_.allocate(
          candidate_workspace_capacity_ + 1U,
          "cudaMalloc ranked-pair shell counts");
      shell_offsets_.allocate(
          candidate_workspace_capacity_ + 1U,
          "cudaMalloc ranked-pair shell offsets");
      sort_keys_a_.allocate(
          candidate_workspace_capacity_,
          "cudaMalloc ranked-pair sort keys A");
      sort_keys_b_.allocate(
          candidate_workspace_capacity_,
          "cudaMalloc ranked-pair sort keys B");
      sort_values_a_.allocate(
          candidate_workspace_capacity_,
          "cudaMalloc ranked-pair sort values A");
      sort_values_b_.allocate(
          candidate_workspace_capacity_,
          "cudaMalloc ranked-pair sort values B");
      transaction_rank_counts_.allocate(
          config_.maximum_closed_rank + 2U,
          "cudaMalloc ranked-pair transaction rank counts");
      catalog_strict_counts_.allocate(
          config_.catalog_record_capacity + 1U,
          "cudaMalloc ranked-pair catalog strict counts");
      catalog_shell_counts_.allocate(
          config_.catalog_record_capacity + 1U,
          "cudaMalloc ranked-pair catalog shell counts");
      fallback_candidate_indices_.allocate(
          config_.exact_fallback_capacity,
          "cudaMalloc ranked-pair exact fallback queue");
      control_.allocate(1U, "cudaMalloc ranked-pair scalar control");

      size_scan_workspace();
      size_sort_workspace();
      check_cuda(
          cudaMemsetAsync(
              catalogs_[0].rank_offsets.get(),
              0,
              (config_.maximum_closed_rank + 2U) * sizeof(std::uint64_t),
              stream_),
          "cudaMemsetAsync ranked-pair initial rank offsets A");
      check_cuda(
          cudaMemsetAsync(
              catalogs_[1].rank_offsets.get(),
              0,
              (config_.maximum_closed_rank + 2U) * sizeof(std::uint64_t),
              stream_),
          "cudaMemsetAsync ranked-pair initial rank offsets B");
      check_cuda(
          cudaMemsetAsync(
              catalogs_[0].strict_offsets.get(),
              0,
              sizeof(std::uint64_t),
              stream_),
          "cudaMemsetAsync ranked-pair initial strict offset A");
      check_cuda(
          cudaMemsetAsync(
              catalogs_[0].shell_offsets.get(),
              0,
              sizeof(std::uint64_t),
              stream_),
          "cudaMemsetAsync ranked-pair initial shell offset A");
      check_cuda(
          cudaMemsetAsync(
              catalogs_[1].strict_offsets.get(),
              0,
              sizeof(std::uint64_t),
              stream_),
          "cudaMemsetAsync ranked-pair initial strict offset B");
      check_cuda(
          cudaMemsetAsync(
              catalogs_[1].shell_offsets.get(),
              0,
              sizeof(std::uint64_t),
              stream_),
          "cudaMemsetAsync ranked-pair initial shell offset B");
    } catch (...) {
      static_cast<void>(cudaStreamDestroy(stream_));
      stream_ = nullptr;
      throw;
    }
  }

  RankedPairCudaResources(const RankedPairCudaResources&) = delete;
  RankedPairCudaResources& operator=(const RankedPairCudaResources&) = delete;

  ~RankedPairCudaResources() {
    int prior_device = 0;
    const cudaError_t query = cudaGetDevice(&prior_device);
    bool restore = false;
    if (query == cudaSuccess && prior_device != device_) {
      restore = cudaSetDevice(device_) == cudaSuccess;
    }
    if (query != cudaSuccess || (prior_device != device_ && !restore)) {
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
    if (restore) {
      static_cast<void>(cudaSetDevice(prior_device));
    }
  }

  [[nodiscard]] bool matches(
      const Phase15MortonYao48DeviceCandidateTilePrivateViews& views,
      const Phase15MortonYao48RankedPairTileClassifierRequest& request,
      std::size_t anchor_count,
      std::size_t physical_candidate_capacity) const noexcept {
    return device_ == views.cuda_device &&
           source_owner_identity_ == views.source_owner_identity &&
           source_cloud_identity_ == views.source_cloud_identity &&
           source_snapshot_epoch_ == request.source_snapshot_epoch &&
           point_count_ == request.point_count &&
           certified_node_count_ == request.certified_node_count &&
           config_ == request.fixed_config &&
           anchor_count <= anchor_workspace_capacity_ &&
           physical_candidate_capacity <= candidate_workspace_capacity_ &&
           request.prior_candidate_count == candidate_count_ &&
           request.prior_accepted_record_count == record_count_ &&
           request.prior_above_window_count == above_window_count_ &&
           request.prior_unresolved_count == 0U &&
           request.prior_strict_payload_point_id_count == strict_count_ &&
           request.prior_shell_payload_point_id_count == shell_count_ &&
           request.prior_exact_fallback_count == 0U;
  }

  void commit_success(
      const Phase15MortonYao48RankedPairTileClassifierRequest& request,
      const DeviceReceiptControl& control,
      bool destination_catalog_built) noexcept {
    if (destination_catalog_built) {
      active_catalog_ = 1U - active_catalog_;
    }
    candidate_count_ = control.cumulative_candidate_count;
    record_count_ = control.cumulative_accepted_record_count;
    above_window_count_ = control.cumulative_above_window_count;
    strict_count_ = control.cumulative_strict_payload_count;
    shell_count_ = control.cumulative_shell_payload_count;
    last_tile_epoch_ = request.tile_epoch;
    last_chunk_sequence_ = request.chunk_sequence;
    last_anchor_begin_ = request.anchor_begin;
    last_anchor_end_ = request.anchor_end;
  }

  [[nodiscard]] cudaStream_t stream() const noexcept { return stream_; }
  [[nodiscard]] int device() const noexcept { return device_; }
  [[nodiscard]] CatalogBuffers& current_catalog() noexcept {
    return catalogs_[active_catalog_];
  }
  [[nodiscard]] CatalogBuffers& destination_catalog() noexcept {
    return catalogs_[1U - active_catalog_];
  }
  [[nodiscard]] std::size_t record_count() const noexcept {
    return static_cast<std::size_t>(record_count_);
  }
  [[nodiscard]] std::size_t strict_count() const noexcept {
    return static_cast<std::size_t>(strict_count_);
  }
  [[nodiscard]] std::size_t shell_count() const noexcept {
    return static_cast<std::size_t>(shell_count_);
  }

  DeviceBuffer<std::uint64_t>& anchor_counts() noexcept {
    return anchor_counts_;
  }
  DeviceBuffer<std::uint64_t>& anchor_offsets() noexcept {
    return anchor_offsets_;
  }
  DeviceBuffer<std::uint64_t>& last_partner_positions() noexcept {
    return last_partner_positions_;
  }
  DeviceBuffer<CandidateClassification>& classifications() noexcept {
    return classifications_;
  }
  DeviceBuffer<std::uint64_t>& accepted_counts() noexcept {
    return accepted_counts_;
  }
  DeviceBuffer<std::uint64_t>& accepted_offsets() noexcept {
    return accepted_offsets_;
  }
  DeviceBuffer<std::uint64_t>& strict_counts() noexcept {
    return strict_counts_;
  }
  DeviceBuffer<std::uint64_t>& strict_offsets() noexcept {
    return strict_offsets_;
  }
  DeviceBuffer<std::uint64_t>& shell_counts() noexcept {
    return shell_counts_;
  }
  DeviceBuffer<std::uint64_t>& shell_offsets() noexcept {
    return shell_offsets_;
  }
  DeviceBuffer<std::uint64_t>& sort_keys_a() noexcept { return sort_keys_a_; }
  DeviceBuffer<std::uint64_t>& sort_keys_b() noexcept { return sort_keys_b_; }
  DeviceBuffer<std::uint64_t>& sort_values_a() noexcept {
    return sort_values_a_;
  }
  DeviceBuffer<std::uint64_t>& sort_values_b() noexcept {
    return sort_values_b_;
  }
  DeviceBuffer<std::uint64_t>& transaction_rank_counts() noexcept {
    return transaction_rank_counts_;
  }
  DeviceBuffer<std::uint64_t>& catalog_strict_counts() noexcept {
    return catalog_strict_counts_;
  }
  DeviceBuffer<std::uint64_t>& catalog_shell_counts() noexcept {
    return catalog_shell_counts_;
  }
  DeviceBuffer<std::uint64_t>& fallback_candidate_indices() noexcept {
    return fallback_candidate_indices_;
  }
  DeviceBuffer<DeviceReceiptControl>& control() noexcept { return control_; }
  DeviceBuffer<std::byte>& scan_workspace() noexcept {
    return scan_workspace_;
  }
  DeviceBuffer<std::byte>& sort_workspace() noexcept {
    return sort_workspace_;
  }
  [[nodiscard]] std::size_t scan_workspace_bytes() const noexcept {
    return scan_workspace_bytes_;
  }
  [[nodiscard]] std::size_t sort_workspace_bytes() const noexcept {
    return sort_workspace_bytes_;
  }

 private:
  void allocate_catalog(CatalogBuffers& catalog, const char* suffix) {
    const std::size_t rank_offset_count = config_.maximum_closed_rank + 2U;
    const std::size_t record_offset_count =
        config_.catalog_record_capacity + 1U;
    catalog.support_u.allocate(
        config_.catalog_record_capacity,
        suffix[0] == 'A' ? "cudaMalloc ranked-pair support-u A"
                         : "cudaMalloc ranked-pair support-u B");
    catalog.support_v.allocate(
        config_.catalog_record_capacity,
        suffix[0] == 'A' ? "cudaMalloc ranked-pair support-v A"
                         : "cudaMalloc ranked-pair support-v B");
    catalog.closed_rank.allocate(
        config_.catalog_record_capacity,
        suffix[0] == 'A' ? "cudaMalloc ranked-pair closed-rank A"
                         : "cudaMalloc ranked-pair closed-rank B");
    catalog.rank_offsets.allocate(
        rank_offset_count,
        suffix[0] == 'A' ? "cudaMalloc ranked-pair rank offsets A"
                         : "cudaMalloc ranked-pair rank offsets B");
    catalog.strict_offsets.allocate(
        record_offset_count,
        suffix[0] == 'A' ? "cudaMalloc ranked-pair strict offsets A"
                         : "cudaMalloc ranked-pair strict offsets B");
    catalog.strict_point_ids.allocate(
        config_.payload_point_id_capacity,
        suffix[0] == 'A' ? "cudaMalloc ranked-pair strict payload A"
                         : "cudaMalloc ranked-pair strict payload B");
    catalog.shell_offsets.allocate(
        record_offset_count,
        suffix[0] == 'A' ? "cudaMalloc ranked-pair shell offsets A"
                         : "cudaMalloc ranked-pair shell offsets B");
    catalog.shell_point_ids.allocate(
        config_.payload_point_id_capacity,
        suffix[0] == 'A' ? "cudaMalloc ranked-pair shell payload A"
                         : "cudaMalloc ranked-pair shell payload B");
  }

  void size_scan_workspace() {
    std::size_t maximum_bytes = 0U;
    const auto size_scan = [this, &maximum_bytes](
                               const std::uint64_t* input,
                               std::uint64_t* output,
                               std::size_t count,
                               const char* operation) {
      std::size_t bytes = 0U;
      check_cuda(
          cub::DeviceScan::ExclusiveSum(
              nullptr,
              bytes,
              input,
              output,
              checked_cub_count(
                  count,
                  "a ranked-pair scan extent exceeds the CUB int ABI"),
              stream_),
          operation);
      maximum_bytes = std::max(maximum_bytes, bytes);
    };
    size_scan(
        anchor_counts_.get(),
        anchor_offsets_.get(),
        anchor_workspace_capacity_ + 1U,
        "CUB ranked-pair anchor scan sizing");
    size_scan(
        accepted_counts_.get(),
        accepted_offsets_.get(),
        candidate_workspace_capacity_ + 1U,
        "CUB ranked-pair candidate scan sizing");
    size_scan(
        catalog_strict_counts_.get(),
        catalogs_[0].strict_offsets.get(),
        config_.catalog_record_capacity + 1U,
        "CUB ranked-pair catalog scan sizing");
    scan_workspace_bytes_ = std::max<std::size_t>(1U, maximum_bytes);
    scan_workspace_.allocate(
        scan_workspace_bytes_,
        "cudaMalloc ranked-pair shared scan workspace");
  }

  void size_sort_workspace() {
    cub::DoubleBuffer<std::uint64_t> keys{
        sort_keys_a_.get(), sort_keys_b_.get()};
    cub::DoubleBuffer<std::uint64_t> values{
        sort_values_a_.get(), sort_values_b_.get()};
    std::size_t bytes = 0U;
    check_cuda(
        cub::DeviceRadixSort::SortPairs(
            nullptr,
            bytes,
            keys,
            values,
            checked_cub_count(
                candidate_workspace_capacity_,
                "a ranked-pair sort extent exceeds the CUB int ABI"),
            0,
            64,
            stream_),
        "CUB ranked-pair sort sizing");
    sort_workspace_bytes_ = std::max<std::size_t>(1U, bytes);
    sort_workspace_.allocate(
        sort_workspace_bytes_,
        "cudaMalloc ranked-pair sort workspace");
  }

  void abandon_catalog(CatalogBuffers& catalog) noexcept {
    catalog.shell_point_ids.abandon();
    catalog.shell_offsets.abandon();
    catalog.strict_point_ids.abandon();
    catalog.strict_offsets.abandon();
    catalog.rank_offsets.abandon();
    catalog.closed_rank.abandon();
    catalog.support_v.abandon();
    catalog.support_u.abandon();
  }

  void reset_catalog(CatalogBuffers& catalog) noexcept {
    catalog.shell_point_ids.reset();
    catalog.shell_offsets.reset();
    catalog.strict_point_ids.reset();
    catalog.strict_offsets.reset();
    catalog.rank_offsets.reset();
    catalog.closed_rank.reset();
    catalog.support_v.reset();
    catalog.support_u.reset();
  }

  void abandon_all() noexcept {
    sort_workspace_.abandon();
    scan_workspace_.abandon();
    control_.abandon();
    fallback_candidate_indices_.abandon();
    catalog_shell_counts_.abandon();
    catalog_strict_counts_.abandon();
    transaction_rank_counts_.abandon();
    sort_values_b_.abandon();
    sort_values_a_.abandon();
    sort_keys_b_.abandon();
    sort_keys_a_.abandon();
    shell_offsets_.abandon();
    shell_counts_.abandon();
    strict_offsets_.abandon();
    strict_counts_.abandon();
    accepted_offsets_.abandon();
    accepted_counts_.abandon();
    classifications_.abandon();
    last_partner_positions_.abandon();
    anchor_offsets_.abandon();
    anchor_counts_.abandon();
    abandon_catalog(catalogs_[1]);
    abandon_catalog(catalogs_[0]);
  }

  void reset_all() noexcept {
    sort_workspace_.reset();
    scan_workspace_.reset();
    control_.reset();
    fallback_candidate_indices_.reset();
    catalog_shell_counts_.reset();
    catalog_strict_counts_.reset();
    transaction_rank_counts_.reset();
    sort_values_b_.reset();
    sort_values_a_.reset();
    sort_keys_b_.reset();
    sort_keys_a_.reset();
    shell_offsets_.reset();
    shell_counts_.reset();
    strict_offsets_.reset();
    strict_counts_.reset();
    accepted_offsets_.reset();
    accepted_counts_.reset();
    classifications_.reset();
    last_partner_positions_.reset();
    anchor_offsets_.reset();
    anchor_counts_.reset();
    reset_catalog(catalogs_[1]);
    reset_catalog(catalogs_[0]);
  }

  int device_{-1};
  cudaStream_t stream_{nullptr};
  const void* source_owner_identity_{};
  const void* source_cloud_identity_{};
  std::uint64_t source_snapshot_epoch_{};
  std::size_t point_count_{};
  std::size_t certified_node_count_{};
  MortonYao48RankedPairTileClassifierConfig config_{};
  std::size_t anchor_workspace_capacity_{};
  std::size_t candidate_workspace_capacity_{};
  CatalogBuffers catalogs_[2];
  std::size_t active_catalog_{};
  DeviceBuffer<std::uint64_t> anchor_counts_;
  DeviceBuffer<std::uint64_t> anchor_offsets_;
  DeviceBuffer<std::uint64_t> last_partner_positions_;
  DeviceBuffer<CandidateClassification> classifications_;
  DeviceBuffer<std::uint64_t> accepted_counts_;
  DeviceBuffer<std::uint64_t> accepted_offsets_;
  DeviceBuffer<std::uint64_t> strict_counts_;
  DeviceBuffer<std::uint64_t> strict_offsets_;
  DeviceBuffer<std::uint64_t> shell_counts_;
  DeviceBuffer<std::uint64_t> shell_offsets_;
  DeviceBuffer<std::uint64_t> sort_keys_a_;
  DeviceBuffer<std::uint64_t> sort_keys_b_;
  DeviceBuffer<std::uint64_t> sort_values_a_;
  DeviceBuffer<std::uint64_t> sort_values_b_;
  DeviceBuffer<std::uint64_t> transaction_rank_counts_;
  DeviceBuffer<std::uint64_t> catalog_strict_counts_;
  DeviceBuffer<std::uint64_t> catalog_shell_counts_;
  DeviceBuffer<std::uint64_t> fallback_candidate_indices_;
  DeviceBuffer<DeviceReceiptControl> control_;
  DeviceBuffer<std::byte> scan_workspace_;
  DeviceBuffer<std::byte> sort_workspace_;
  std::size_t scan_workspace_bytes_{};
  std::size_t sort_workspace_bytes_{};
  std::uint64_t candidate_count_{};
  std::uint64_t record_count_{};
  std::uint64_t above_window_count_{};
  std::uint64_t strict_count_{};
  std::uint64_t shell_count_{};
  std::uint64_t last_tile_epoch_{};
  std::uint64_t last_chunk_sequence_{};
  std::size_t last_anchor_begin_{};
  std::size_t last_anchor_end_{};
};

[[nodiscard]] __device__ bool checked_add_u64(
    std::uint64_t left,
    std::uint64_t right,
    std::uint64_t& output) noexcept {
  if (right > UINT64_MAX - left) {
    return false;
  }
  output = left + right;
  return true;
}

[[nodiscard]] __device__ bool valid_node(
    const Phase15MortonYao48RankedPairNode* nodes,
    std::uint64_t node_index,
    std::uint64_t node_count,
    std::uint64_t point_count) noexcept {
  if (node_index >= node_count) {
    return false;
  }
  const Phase15MortonYao48RankedPairNode& node = nodes[node_index];
  if (node.leaf_begin >= node.leaf_end || node.leaf_end > point_count) {
    return false;
  }
  const std::uint64_t width = node.leaf_end - node.leaf_begin;
  if (width == UINT64_C(1)) {
    return node.left_child == kInvalid && node.right_child == kInvalid;
  }
  if (node_index == 0U || node.right_child != node_index - UINT64_C(1) ||
      node.left_child >= node.right_child || node.right_child >= node_count) {
    return false;
  }
  const auto& left = nodes[node.left_child];
  const auto& right = nodes[node.right_child];
  if (left.leaf_begin != node.leaf_begin ||
      left.leaf_end != right.leaf_begin ||
      right.leaf_end != node.leaf_end ||
      right.leaf_begin >= right.leaf_end) {
    return false;
  }
  const std::uint64_t right_width = right.leaf_end - right.leaf_begin;
  return right_width <= node_index / UINT64_C(2) &&
         node.left_child == node_index - UINT64_C(2) * right_width;
}

[[nodiscard]] __device__ bool finish_or_skip_subtree(
    std::uint64_t width,
    std::uint64_t& cursor,
    bool& complete) noexcept {
  if (width == 0U ||
      width > UINT64_MAX / UINT64_C(2) + UINT64_C(1)) {
    return false;
  }
  const std::uint64_t node_extent = UINT64_C(2) * width - UINT64_C(1);
  if (node_extent > cursor + UINT64_C(1)) {
    return false;
  }
  complete = node_extent == cursor + UINT64_C(1);
  if (!complete) {
    cursor -= node_extent;
  }
  return true;
}

[[nodiscard]] __device__ bool node_phi_lower_is_strictly_positive(
    const Phase15MortonYao48RankedPairNode& node,
    const std::uint64_t* coordinate_bits,
    std::uint64_t point_count,
    const std::uint64_t first_support_bits[kAxisCount],
    const std::uint64_t second_support_bits[kAxisCount]) noexcept {
  device::DeviceInterval sum = device::point_interval(UINT64_C(0));
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const std::uint64_t lower_id = node.lower_point_ids[axis];
    const std::uint64_t upper_id = node.upper_point_ids[axis];
    if (lower_id >= point_count || upper_id >= point_count) {
      return false;
    }
    const device::DeviceInterval lower = device::point_interval(
        coordinate_bits[axis * point_count + lower_id]);
    const device::DeviceInterval upper = device::point_interval(
        coordinate_bits[axis * point_count + upper_id]);
    if (!lower.valid || !upper.valid || lower.lower > upper.upper) {
      return false;
    }
    const device::DeviceInterval coordinate{
        lower.lower, upper.upper, true};
    const device::DeviceInterval first_delta = device::subtract_intervals(
        coordinate, device::point_interval(first_support_bits[axis]));
    const device::DeviceInterval second_delta = device::subtract_intervals(
        coordinate, device::point_interval(second_support_bits[axis]));
    sum = device::add_intervals(
        sum, device::multiply_intervals(first_delta, second_delta));
    if (!sum.valid) {
      return false;
    }
  }
  // Equality is deliberately descended.  Only a strictly positive directed
  // lower bound can remove a complete AABB from the closed diametral ball.
  return sum.lower > 0.0;
}

__global__ void validate_anchor_controls_kernel(
    const Phase15MortonYao48DeviceTiledAnchorControl* controls,
    std::uint64_t anchor_count,
    std::uint64_t anchor_begin,
    std::uint64_t candidate_stride,
    std::uint64_t tile_epoch,
    std::uint64_t chunk_sequence,
    std::uint64_t* anchor_counts,
    DeviceReceiptControl* receipt_control) {
  const std::uint64_t index =
      static_cast<std::uint64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index > anchor_count) {
    return;
  }
  if (index == anchor_count) {
    anchor_counts[index] = UINT64_C(0);
    return;
  }
  const Phase15MortonYao48DeviceTiledAnchorControl control = controls[index];
  const bool publishable =
      control.status == kAnchorStatusChunkReady ||
      control.status == kAnchorStatusComplete;
  const bool valid =
      control.anchor_morton_position == anchor_begin + index &&
      control.candidate_count <= candidate_stride && publishable &&
      control.stop_reason == kAnchorStopNone &&
      control.failure_code == kAnchorFailureNone &&
      control.tile_epoch == tile_epoch &&
      control.chunk_sequence == chunk_sequence &&
      control.reserved_zero == UINT64_C(0) &&
      ((control.status == kAnchorStatusComplete &&
        control.yield_reason == kAnchorYieldNone) ||
       control.status == kAnchorStatusChunkReady);
  anchor_counts[index] = valid ? control.candidate_count : UINT64_C(0);
  if (!valid) {
    atomicAdd(
        reinterpret_cast<unsigned long long*>(
            &receipt_control->malformed_count),
        1ULL);
  }
}

[[nodiscard]] __device__ std::uint64_t find_anchor_slot(
    const std::uint64_t* anchor_offsets,
    std::uint64_t anchor_count,
    std::uint64_t logical_candidate_index) noexcept {
  std::uint64_t lower = 0U;
  std::uint64_t upper = anchor_count + UINT64_C(1);
  while (lower < upper) {
    const std::uint64_t middle = lower + (upper - lower) / UINT64_C(2);
    if (anchor_offsets[middle] <= logical_candidate_index) {
      lower = middle + UINT64_C(1);
    } else {
      upper = middle;
    }
  }
  return lower == 0U ? kInvalid : lower - UINT64_C(1);
}

__global__ void classify_candidates_multiorder_kernel(
    const std::uint64_t* coordinate_bits,
    const std::uint64_t* morton_point_ids,
    const Phase15MortonYao48RankedPairNode* nodes,
    const Phase15MortonYao48DeviceTiledCandidateRecord* candidate_records,
    const std::uint64_t* anchor_offsets,
    const std::uint64_t* prior_last_partner_positions,
    std::uint64_t point_count,
    std::uint64_t node_count,
    std::uint64_t anchor_count,
    std::uint64_t anchor_begin,
    std::uint64_t candidate_stride,
    std::uint64_t candidate_count,
    std::uint64_t maximum_closed_rank,
    std::uint64_t fallback_capacity,
    bool same_tile_continuation,
    CandidateClassification* classifications,
    std::uint64_t* accepted_counts,
    std::uint64_t* strict_counts,
    std::uint64_t* shell_counts,
    std::uint64_t* sort_keys,
    std::uint64_t* sort_values,
    std::uint64_t* transaction_rank_counts,
    std::uint64_t* fallback_candidate_indices,
    DeviceReceiptControl* receipt_control) {
  const std::uint64_t logical_index =
      static_cast<std::uint64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (logical_index >= candidate_count) {
    return;
  }

  CandidateClassification result{};
  result.outcome = static_cast<std::uint8_t>(CandidateOutcome::malformed);
  accepted_counts[logical_index] = UINT64_C(0);
  strict_counts[logical_index] = UINT64_C(0);
  shell_counts[logical_index] = UINT64_C(0);
  sort_keys[logical_index] = kSortKeyRejected;
  sort_values[logical_index] = logical_index;

  const std::uint64_t anchor_slot = find_anchor_slot(
      anchor_offsets, anchor_count, logical_index);
  bool malformed = anchor_slot >= anchor_count;
  std::uint64_t candidate_slot = 0U;
  std::uint64_t physical_index = 0U;
  Phase15MortonYao48DeviceTiledCandidateRecord candidate{};
  if (!malformed) {
    candidate_slot = logical_index - anchor_offsets[anchor_slot];
    malformed = candidate_slot >= candidate_stride;
  }
  if (!malformed) {
    physical_index = anchor_slot * candidate_stride + candidate_slot;
    candidate = candidate_records[physical_index];
    const std::uint64_t anchor_position = anchor_begin + anchor_slot;
    malformed =
        candidate.support_u >= candidate.support_v ||
        candidate.support_v >= point_count ||
        candidate.anchor_morton_position != anchor_position ||
        candidate.partner_morton_position >= anchor_position ||
        morton_point_ids[anchor_position] >= point_count ||
        morton_point_ids[candidate.partner_morton_position] >= point_count;
    if (!malformed) {
      const std::uint64_t anchor_id = morton_point_ids[anchor_position];
      const std::uint64_t partner_id =
          morton_point_ids[candidate.partner_morton_position];
      malformed = candidate.support_u !=
                          (anchor_id < partner_id ? anchor_id : partner_id) ||
                  candidate.support_v !=
                          (anchor_id < partner_id ? partner_id : anchor_id);
    }
    if (!malformed && candidate_slot != 0U) {
      const auto prior = candidate_records[physical_index - UINT64_C(1)];
      malformed =
          candidate.partner_morton_position >= prior.partner_morton_position;
    }
    if (!malformed && candidate_slot == 0U && same_tile_continuation &&
        prior_last_partner_positions[anchor_slot] != kInvalid) {
      malformed = candidate.partner_morton_position >=
                  prior_last_partner_positions[anchor_slot];
    }
  }

  if (malformed) {
    classifications[logical_index] = result;
    atomicAdd(
        reinterpret_cast<unsigned long long*>(
            &receipt_control->malformed_count),
        1ULL);
    atomicAdd(
        reinterpret_cast<unsigned long long*>(
            &receipt_control->processed_candidate_count),
        1ULL);
    return;
  }

  result.support_u = candidate.support_u;
  result.support_v = candidate.support_v;
  std::uint64_t first_support_bits[kAxisCount]{};
  std::uint64_t second_support_bits[kAxisCount]{};
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    first_support_bits[axis] =
        coordinate_bits[axis * point_count + candidate.support_u];
    second_support_bits[axis] =
        coordinate_bits[axis * point_count + candidate.support_v];
  }

  std::uint64_t closed_ids[kMaximumNonSupportCount]{};
  std::uint8_t closed_is_shell[kMaximumNonSupportCount]{};
  std::uint64_t closed_count = 0U;
  std::uint64_t strict_count = 0U;
  std::uint64_t shell_count = 0U;
  std::uint64_t support_seen_mask = 0U;
  std::uint64_t cursor = node_count - UINT64_C(1);
  std::uint64_t visited_node_count = 0U;
  bool complete = false;
  CandidateOutcome outcome = CandidateOutcome::inactive;

  if (!valid_node(nodes, cursor, node_count, point_count) ||
      nodes[cursor].leaf_begin != 0U ||
      nodes[cursor].leaf_end != point_count) {
    outcome = CandidateOutcome::malformed;
  }

  while (outcome == CandidateOutcome::inactive && !complete) {
    if (!valid_node(nodes, cursor, node_count, point_count) ||
        visited_node_count >= node_count) {
      outcome = CandidateOutcome::malformed;
      break;
    }
    ++visited_node_count;
    const Phase15MortonYao48RankedPairNode node = nodes[cursor];
    const std::uint64_t width = node.leaf_end - node.leaf_begin;
    if (node_phi_lower_is_strictly_positive(
            node,
            coordinate_bits,
            point_count,
            first_support_bits,
            second_support_bits)) {
      if (!finish_or_skip_subtree(width, cursor, complete)) {
        outcome = CandidateOutcome::malformed;
      }
      continue;
    }
    if (width != UINT64_C(1)) {
      cursor = node.right_child;
      continue;
    }

    const std::uint64_t witness_position = node.leaf_begin;
    const std::uint64_t witness_id = morton_point_ids[witness_position];
    if (witness_id >= point_count) {
      outcome = CandidateOutcome::malformed;
      break;
    }
    std::uint64_t witness_bits[kAxisCount]{};
    for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
      witness_bits[axis] = coordinate_bits[axis * point_count + witness_id];
    }
    const exact_phi_fixed::ResultCode exact = exact_phi_fixed::exact_sign(
        witness_bits, first_support_bits, second_support_bits);
    if (exact == exact_phi_fixed::ResultCode::requires_cpu_fallback) {
      outcome = CandidateOutcome::exact_fallback;
      const std::uint64_t fallback_slot = atomicAdd(
          reinterpret_cast<unsigned long long*>(
              &receipt_control->exact_fallback_count),
          1ULL);
      if (fallback_slot < fallback_capacity) {
        fallback_candidate_indices[fallback_slot] = logical_index;
      }
      break;
    }
    if (exact == exact_phi_fixed::ResultCode::invalid_input) {
      outcome = CandidateOutcome::malformed;
      break;
    }

    const bool is_first_support = witness_id == candidate.support_u;
    const bool is_second_support = witness_id == candidate.support_v;
    if (is_first_support || is_second_support) {
      if (exact != exact_phi_fixed::ResultCode::zero) {
        outcome = CandidateOutcome::malformed;
        break;
      }
      support_seen_mask |= is_first_support ? UINT64_C(1) : UINT64_C(2);
    } else if (
        exact == exact_phi_fixed::ResultCode::negative ||
        exact == exact_phi_fixed::ResultCode::zero) {
      if (closed_count == maximum_closed_rank - UINT64_C(2)) {
        outcome = CandidateOutcome::above_window;
        break;
      }
      closed_ids[closed_count] = witness_id;
      closed_is_shell[closed_count] = static_cast<std::uint8_t>(
          exact == exact_phi_fixed::ResultCode::zero ? 1U : 0U);
      ++closed_count;
      if (exact == exact_phi_fixed::ResultCode::zero) {
        ++shell_count;
      } else {
        ++strict_count;
      }
    }

    if (!finish_or_skip_subtree(UINT64_C(1), cursor, complete)) {
      outcome = CandidateOutcome::malformed;
    }
  }

  if (outcome == CandidateOutcome::inactive) {
    if (!complete || support_seen_mask != UINT64_C(3) ||
        closed_count != strict_count + shell_count ||
        closed_count > kMaximumNonSupportCount) {
      outcome = CandidateOutcome::malformed;
    } else {
      for (std::uint64_t index = 1U; index < closed_count; ++index) {
        const std::uint64_t point_id = closed_ids[index];
        const std::uint8_t is_shell = closed_is_shell[index];
        std::uint64_t destination = index;
        while (destination != 0U &&
               point_id < closed_ids[destination - UINT64_C(1)]) {
          closed_ids[destination] = closed_ids[destination - UINT64_C(1)];
          closed_is_shell[destination] =
              closed_is_shell[destination - UINT64_C(1)];
          --destination;
        }
        closed_ids[destination] = point_id;
        closed_is_shell[destination] = is_shell;
      }
      std::uint64_t strict_destination = 0U;
      std::uint64_t shell_destination = strict_count;
      for (std::uint64_t index = 0U; index < closed_count; ++index) {
        if (closed_is_shell[index] == 0U) {
          result.payload_point_ids[strict_destination++] = closed_ids[index];
        } else {
          result.payload_point_ids[shell_destination++] = closed_ids[index];
        }
      }
      if (strict_destination != strict_count ||
          shell_destination != closed_count) {
        outcome = CandidateOutcome::malformed;
      } else {
        outcome = CandidateOutcome::accepted;
      }
    }
  }

  result.outcome = static_cast<std::uint8_t>(outcome);
  if (outcome == CandidateOutcome::accepted) {
    result.closed_rank = static_cast<std::uint8_t>(closed_count + 2U);
    result.strict_count = static_cast<std::uint8_t>(strict_count);
    result.shell_count = static_cast<std::uint8_t>(shell_count);
    accepted_counts[logical_index] = UINT64_C(1);
    strict_counts[logical_index] = strict_count;
    shell_counts[logical_index] = shell_count;
    sort_keys[logical_index] = result.closed_rank;
    atomicAdd(
        reinterpret_cast<unsigned long long*>(
            &transaction_rank_counts[result.closed_rank]),
        1ULL);
  } else if (outcome == CandidateOutcome::above_window) {
    atomicAdd(
        reinterpret_cast<unsigned long long*>(
            &receipt_control->above_window_count),
        1ULL);
  } else if (outcome == CandidateOutcome::malformed) {
    atomicAdd(
        reinterpret_cast<unsigned long long*>(
            &receipt_control->malformed_count),
        1ULL);
  }
  classifications[logical_index] = result;
  atomicAdd(
      reinterpret_cast<unsigned long long*>(
          &receipt_control->processed_candidate_count),
      1ULL);
}

__global__ void prepare_receipt_control_kernel(
    const std::uint64_t* anchor_offsets,
    std::uint64_t anchor_count,
    const std::uint64_t* accepted_offsets,
    const std::uint64_t* strict_offsets,
    const std::uint64_t* shell_offsets,
    std::uint64_t candidate_count,
    std::uint64_t prior_candidate_count,
    std::uint64_t prior_record_count,
    std::uint64_t prior_above_window_count,
    std::uint64_t prior_unresolved_count,
    std::uint64_t prior_strict_count,
    std::uint64_t prior_shell_count,
    std::uint64_t prior_fallback_count,
    std::uint64_t record_capacity,
    std::uint64_t payload_capacity,
    std::uint64_t fallback_capacity,
    bool frontier_closed,
    DeviceReceiptControl* control) {
  if (blockIdx.x != 0U || threadIdx.x != 0U) {
    return;
  }
  control->transaction_accepted_record_count =
      accepted_offsets[candidate_count];
  control->transaction_strict_payload_count = strict_offsets[candidate_count];
  control->transaction_shell_payload_count = shell_offsets[candidate_count];
  control->status = frontier_closed ? kStatusFrontierClosed
                                    : kStatusChunkCommitted;
  control->stop_reason = kStopNone;
  control->failure_code = kFailureNone;
  control->commit_allowed = UINT64_C(0);

  bool invariant =
      control->processed_candidate_count != candidate_count ||
      anchor_offsets[anchor_count] != candidate_count;
  std::uint64_t classified_partition = 0U;
  invariant = invariant ||
      !checked_add_u64(
          control->transaction_accepted_record_count,
          control->above_window_count,
          classified_partition);
  if (control->exact_fallback_count == 0U &&
      control->malformed_count == 0U) {
    invariant = invariant || classified_partition != candidate_count;
  } else {
    std::uint64_t with_fallback = 0U;
    invariant = invariant ||
        !checked_add_u64(
            classified_partition,
            control->exact_fallback_count,
            with_fallback) ||
        with_fallback != candidate_count;
  }

  std::uint64_t prior_payload = 0U;
  std::uint64_t transaction_payload = 0U;
  std::uint64_t cumulative_payload = 0U;
  invariant = invariant ||
      !checked_add_u64(prior_strict_count, prior_shell_count, prior_payload) ||
      !checked_add_u64(
          control->transaction_strict_payload_count,
          control->transaction_shell_payload_count,
          transaction_payload) ||
      !checked_add_u64(prior_payload, transaction_payload, cumulative_payload);
  invariant = invariant ||
      !checked_add_u64(
          prior_record_count,
          control->transaction_accepted_record_count,
          control->required_catalog_record_count) ||
      !checked_add_u64(
          prior_fallback_count,
          control->exact_fallback_count,
          control->required_exact_fallback_count) ||
      !checked_add_u64(
          prior_candidate_count,
          candidate_count,
          control->cumulative_candidate_count);
  control->required_payload_point_id_count = cumulative_payload;

  if (control->malformed_count != 0U) {
    control->failure_code = kFailureMalformed;
    return;
  }
  if (control->invariant_count != 0U || invariant) {
    control->failure_code = kFailureInvariant;
    return;
  }

  if (control->exact_fallback_count > fallback_capacity) {
    control->status = kStatusCapacityExhausted;
    control->stop_reason = kStopFallbackCapacity;
    control->failure_code = kFailureCapacity;
  } else if (control->exact_fallback_count != 0U) {
    control->status = kStatusExactPredicateFailure;
    control->stop_reason = kStopExactPredicate;
    control->failure_code = kFailureExactPredicate;
    control->required_catalog_record_count = prior_record_count;
    control->required_payload_point_id_count = prior_payload;
  } else if (control->required_catalog_record_count > record_capacity) {
    control->status = kStatusCapacityExhausted;
    control->stop_reason = kStopRecordCapacity;
    control->failure_code = kFailureCapacity;
  } else if (control->required_payload_point_id_count > payload_capacity) {
    control->status = kStatusCapacityExhausted;
    control->stop_reason = kStopPayloadCapacity;
    control->failure_code = kFailureCapacity;
  } else {
    control->commit_allowed = UINT64_C(1);
  }

  if (control->commit_allowed != 0U) {
    if (!checked_add_u64(
            prior_above_window_count,
            control->above_window_count,
            control->cumulative_above_window_count) ||
        !checked_add_u64(
            prior_record_count,
            control->transaction_accepted_record_count,
            control->cumulative_accepted_record_count) ||
        !checked_add_u64(
            prior_strict_count,
            control->transaction_strict_payload_count,
            control->cumulative_strict_payload_count) ||
        !checked_add_u64(
            prior_shell_count,
            control->transaction_shell_payload_count,
            control->cumulative_shell_payload_count)) {
      control->commit_allowed = UINT64_C(0);
      control->failure_code = kFailureInvariant;
      return;
    }
    control->cumulative_unresolved_count = prior_unresolved_count;
    control->cumulative_exact_fallback_count = prior_fallback_count;
    return;
  }

  control->cumulative_accepted_record_count = prior_record_count;
  control->cumulative_above_window_count = prior_above_window_count;
  control->cumulative_strict_payload_count = prior_strict_count;
  control->cumulative_shell_payload_count = prior_shell_count;
  control->cumulative_exact_fallback_count =
      control->required_exact_fallback_count;
  if (!checked_add_u64(
          prior_unresolved_count,
          candidate_count,
          control->cumulative_unresolved_count)) {
    control->failure_code = kFailureInvariant;
  }
}

enum class CanonicalSortComponent : std::uint8_t {
  support_v,
  support_u,
  closed_rank,
};

__global__ void build_canonical_sort_keys_kernel(
    const CandidateClassification* classifications,
    const std::uint64_t* ordered_candidate_indices,
    std::uint64_t candidate_count,
    CanonicalSortComponent component,
    std::uint64_t* keys) {
  const std::uint64_t index =
      static_cast<std::uint64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index >= candidate_count) {
    return;
  }
  const std::uint64_t logical_index = ordered_candidate_indices[index];
  if (logical_index >= candidate_count) {
    keys[index] = kSortKeyRejected;
    return;
  }
  const CandidateClassification classified = classifications[logical_index];
  if (classified.outcome !=
      static_cast<std::uint8_t>(CandidateOutcome::accepted)) {
    keys[index] = kSortKeyRejected;
    return;
  }
  switch (component) {
    case CanonicalSortComponent::support_v:
      keys[index] = classified.support_v;
      return;
    case CanonicalSortComponent::support_u:
      keys[index] = classified.support_u;
      return;
    case CanonicalSortComponent::closed_rank:
      keys[index] = classified.closed_rank;
      return;
  }
  keys[index] = kSortKeyRejected;
}

[[nodiscard]] __device__ int compare_canonical_key(
    std::uint64_t left_rank,
    std::uint64_t left_u,
    std::uint64_t left_v,
    std::uint64_t right_rank,
    std::uint64_t right_u,
    std::uint64_t right_v) noexcept {
  if (left_rank != right_rank) {
    return left_rank < right_rank ? -1 : 1;
  }
  if (left_u != right_u) {
    return left_u < right_u ? -1 : 1;
  }
  if (left_v != right_v) {
    return left_v < right_v ? -1 : 1;
  }
  return 0;
}

__global__ void validate_canonical_inputs_kernel(
    const CandidateClassification* classifications,
    const std::uint64_t* sorted_candidate_indices,
    std::uint64_t candidate_count,
    const std::uint64_t* current_support_u,
    const std::uint64_t* current_support_v,
    const std::uint8_t* current_closed_rank,
    std::uint64_t prior_record_count,
    std::uint64_t point_count,
    std::uint64_t maximum_closed_rank,
    DeviceReceiptControl* control) {
  if (control->commit_allowed == 0U) {
    return;
  }
  const std::uint64_t index =
      static_cast<std::uint64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index < control->transaction_accepted_record_count) {
    const std::uint64_t logical_index = sorted_candidate_indices[index];
    bool invalid = logical_index >= candidate_count;
    CandidateClassification current{};
    if (!invalid) {
      current = classifications[logical_index];
      invalid =
          current.outcome !=
              static_cast<std::uint8_t>(CandidateOutcome::accepted) ||
          current.closed_rank < 2U ||
          current.closed_rank > maximum_closed_rank ||
          current.support_u >= current.support_v ||
          current.support_v >= point_count;
    }
    if (!invalid && index != 0U) {
      const std::uint64_t prior_logical =
          sorted_candidate_indices[index - UINT64_C(1)];
      if (prior_logical >= candidate_count) {
        invalid = true;
      } else {
        const CandidateClassification prior =
            classifications[prior_logical];
        invalid = compare_canonical_key(
                      prior.closed_rank,
                      prior.support_u,
                      prior.support_v,
                      current.closed_rank,
                      current.support_u,
                      current.support_v) >= 0;
      }
    }
    if (invalid) {
      atomicAdd(
          reinterpret_cast<unsigned long long*>(&control->invariant_count),
          1ULL);
    }
  }
  if (index < prior_record_count) {
    const std::uint64_t rank = current_closed_rank[index];
    bool invalid = rank < UINT64_C(2) || rank > maximum_closed_rank ||
                   current_support_u[index] >= current_support_v[index] ||
                   current_support_v[index] >= point_count;
    if (!invalid && index != 0U) {
      invalid = compare_canonical_key(
                    current_closed_rank[index - UINT64_C(1)],
                    current_support_u[index - UINT64_C(1)],
                    current_support_v[index - UINT64_C(1)],
                    rank,
                    current_support_u[index],
                    current_support_v[index]) >= 0;
    }
    if (invalid) {
      atomicAdd(
          reinterpret_cast<unsigned long long*>(&control->invariant_count),
          1ULL);
    }
  }
}

[[nodiscard]] __device__ std::uint64_t transaction_rank_prefix(
    const std::uint64_t* counts,
    std::uint64_t rank) noexcept {
  std::uint64_t prefix = 0U;
  for (std::uint64_t current = UINT64_C(2); current < rank; ++current) {
    prefix += counts[current];
  }
  return prefix;
}

[[nodiscard]] __device__ std::uint64_t lower_bound_current_pair(
    const std::uint64_t* support_u,
    const std::uint64_t* support_v,
    std::uint64_t begin,
    std::uint64_t end,
    std::uint64_t target_u,
    std::uint64_t target_v) noexcept {
  while (begin < end) {
    const std::uint64_t middle = begin + (end - begin) / UINT64_C(2);
    const bool less = support_u[middle] < target_u ||
                      (support_u[middle] == target_u &&
                       support_v[middle] < target_v);
    if (less) {
      begin = middle + UINT64_C(1);
    } else {
      end = middle;
    }
  }
  return begin;
}

[[nodiscard]] __device__ std::uint64_t lower_bound_transaction_pair(
    const CandidateClassification* classifications,
    const std::uint64_t* sorted_candidate_indices,
    std::uint64_t begin,
    std::uint64_t end,
    std::uint64_t target_u,
    std::uint64_t target_v) noexcept {
  while (begin < end) {
    const std::uint64_t middle = begin + (end - begin) / UINT64_C(2);
    const CandidateClassification candidate =
        classifications[sorted_candidate_indices[middle]];
    const bool less = candidate.support_u < target_u ||
                      (candidate.support_u == target_u &&
                       candidate.support_v < target_v);
    if (less) {
      begin = middle + UINT64_C(1);
    } else {
      end = middle;
    }
  }
  return begin;
}

__global__ void prepare_destination_records_kernel(
    const std::uint64_t* current_support_u,
    const std::uint64_t* current_support_v,
    const std::uint8_t* current_closed_rank,
    const std::uint64_t* current_rank_offsets,
    const std::uint64_t* current_strict_offsets,
    const std::uint64_t* current_shell_offsets,
    std::uint64_t prior_record_count,
    const CandidateClassification* classifications,
    const std::uint64_t* sorted_candidate_indices,
    std::uint64_t candidate_count,
    const std::uint64_t* transaction_rank_counts,
    std::uint64_t maximum_closed_rank,
    std::uint64_t destination_record_capacity,
    std::uint64_t prior_strict_payload_count,
    std::uint64_t prior_shell_payload_count,
    DeviceReceiptControl* control,
    std::uint64_t* destination_support_u,
    std::uint64_t* destination_support_v,
    std::uint8_t* destination_closed_rank,
    std::uint64_t* destination_strict_counts,
    std::uint64_t* destination_shell_counts) {
  if (control->commit_allowed == 0U) {
    return;
  }
  const std::uint64_t index =
      static_cast<std::uint64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index < prior_record_count) {
    const std::uint64_t rank = current_closed_rank[index];
    bool invalid = rank < UINT64_C(2) || rank > maximum_closed_rank;
    std::uint64_t rank_begin = 0U;
    std::uint64_t rank_end = 0U;
    std::uint64_t transaction_begin = 0U;
    std::uint64_t transaction_end = 0U;
    if (!invalid) {
      rank_begin = current_rank_offsets[rank];
      rank_end = current_rank_offsets[rank + UINT64_C(1)];
      transaction_begin =
          transaction_rank_prefix(transaction_rank_counts, rank);
      transaction_end = transaction_begin + transaction_rank_counts[rank];
      invalid = rank_begin > index || index >= rank_end ||
                rank_end > prior_record_count ||
                transaction_end >
                    control->transaction_accepted_record_count;
    }
    std::uint64_t transaction_insertion = 0U;
    if (!invalid) {
      transaction_insertion = lower_bound_transaction_pair(
          classifications,
          sorted_candidate_indices,
          transaction_begin,
          transaction_end,
          current_support_u[index],
          current_support_v[index]);
      if (transaction_insertion < transaction_end) {
        const CandidateClassification other = classifications[
            sorted_candidate_indices[transaction_insertion]];
        invalid = other.support_u == current_support_u[index] &&
                  other.support_v == current_support_v[index];
      }
    }
    const std::uint64_t destination =
        rank_begin + transaction_begin + (index - rank_begin) +
        (transaction_insertion - transaction_begin);
    const std::uint64_t strict_begin = current_strict_offsets[index];
    const std::uint64_t strict_end = current_strict_offsets[index + 1U];
    const std::uint64_t shell_begin = current_shell_offsets[index];
    const std::uint64_t shell_end = current_shell_offsets[index + 1U];
    invalid = invalid || destination >= destination_record_capacity ||
              destination >= control->required_catalog_record_count ||
              strict_begin > strict_end ||
              strict_end > prior_strict_payload_count ||
              shell_begin > shell_end ||
              shell_end > prior_shell_payload_count;
    if (invalid) {
      atomicAdd(
          reinterpret_cast<unsigned long long*>(&control->invariant_count),
          1ULL);
      return;
    }
    destination_support_u[destination] = current_support_u[index];
    destination_support_v[destination] = current_support_v[index];
    destination_closed_rank[destination] = static_cast<std::uint8_t>(rank);
    destination_strict_counts[destination] =
        strict_end - strict_begin;
    destination_shell_counts[destination] =
        shell_end - shell_begin;
  }

  if (index < control->transaction_accepted_record_count) {
    const std::uint64_t logical_index = sorted_candidate_indices[index];
    if (logical_index >= candidate_count) {
      atomicAdd(
          reinterpret_cast<unsigned long long*>(&control->invariant_count),
          1ULL);
      return;
    }
    const CandidateClassification classified = classifications[logical_index];
    const std::uint64_t rank = classified.closed_rank;
    if (classified.outcome !=
            static_cast<std::uint8_t>(CandidateOutcome::accepted) ||
        rank < UINT64_C(2) || rank > maximum_closed_rank) {
      atomicAdd(
          reinterpret_cast<unsigned long long*>(&control->invariant_count),
          1ULL);
      return;
    }
    const std::uint64_t rank_prefix =
        transaction_rank_prefix(transaction_rank_counts, rank);
    if (index < rank_prefix) {
      atomicAdd(
          reinterpret_cast<unsigned long long*>(&control->invariant_count),
          1ULL);
      return;
    }
    const std::uint64_t rank_local = index - rank_prefix;
    const std::uint64_t current_begin = current_rank_offsets[rank];
    const std::uint64_t current_end = current_rank_offsets[rank + UINT64_C(1)];
    bool invalid = current_begin > current_end ||
                   current_end > prior_record_count;
    std::uint64_t current_insertion = current_begin;
    if (!invalid) {
      current_insertion = lower_bound_current_pair(
          current_support_u,
          current_support_v,
          current_begin,
          current_end,
          classified.support_u,
          classified.support_v);
    }
    if (!invalid && current_insertion < current_end) {
      invalid = current_support_u[current_insertion] == classified.support_u &&
                current_support_v[current_insertion] == classified.support_v;
    }
    const std::uint64_t destination =
        current_begin + rank_prefix + rank_local +
        (current_insertion - current_begin);
    invalid = invalid || destination >= destination_record_capacity ||
              destination >= control->required_catalog_record_count ||
              classified.strict_count + classified.shell_count != rank - 2U;
    if (invalid) {
      atomicAdd(
          reinterpret_cast<unsigned long long*>(&control->invariant_count),
          1ULL);
      return;
    }
    destination_support_u[destination] = classified.support_u;
    destination_support_v[destination] = classified.support_v;
    destination_closed_rank[destination] = classified.closed_rank;
    destination_strict_counts[destination] = classified.strict_count;
    destination_shell_counts[destination] = classified.shell_count;
  }
}

__global__ void build_destination_rank_offsets_kernel(
    const std::uint64_t* current_rank_offsets,
    const std::uint64_t* transaction_rank_counts,
    std::uint64_t maximum_closed_rank,
    const DeviceReceiptControl* control,
    std::uint64_t* destination_rank_offsets) {
  if (blockIdx.x != 0U || threadIdx.x != 0U ||
      control->commit_allowed == 0U) {
    return;
  }
  for (std::uint64_t rank = 0U; rank <= maximum_closed_rank + UINT64_C(1);
       ++rank) {
    destination_rank_offsets[rank] =
        current_rank_offsets[rank] +
        transaction_rank_prefix(transaction_rank_counts, rank);
  }
}

__global__ void finalize_destination_preparation_kernel(
    DeviceReceiptControl* control) {
  if (blockIdx.x != 0U || threadIdx.x != 0U) {
    return;
  }
  if (control->invariant_count != 0U) {
    control->commit_allowed = UINT64_C(0);
    control->failure_code = kFailureInvariant;
  }
}

__global__ void validate_destination_layout_kernel(
    const std::uint64_t* rank_offsets,
    const std::uint64_t* strict_offsets,
    const std::uint64_t* shell_offsets,
    std::uint64_t maximum_closed_rank,
    std::uint64_t record_capacity,
    std::uint64_t payload_capacity,
    DeviceReceiptControl* control) {
  if (control->commit_allowed == 0U) {
    return;
  }
  const std::uint64_t index = threadIdx.x;
  bool invalid = false;
  if (index <= maximum_closed_rank) {
    invalid = rank_offsets[index] > rank_offsets[index + UINT64_C(1)] ||
              rank_offsets[index + UINT64_C(1)] > record_capacity;
  }
  if (index == 0U) {
    const std::uint64_t records = control->required_catalog_record_count;
    invalid = invalid || rank_offsets[0] != UINT64_C(0) ||
              rank_offsets[maximum_closed_rank + UINT64_C(1)] != records ||
              records > record_capacity ||
              strict_offsets[records] !=
                  control->cumulative_strict_payload_count ||
              shell_offsets[records] !=
                  control->cumulative_shell_payload_count ||
              control->cumulative_strict_payload_count > payload_capacity ||
              control->cumulative_shell_payload_count > payload_capacity;
  }
  if (invalid) {
    atomicAdd(
        reinterpret_cast<unsigned long long*>(&control->invariant_count),
        1ULL);
  }
}

__global__ void emit_destination_payload_kernel(
    const std::uint64_t* current_support_u,
    const std::uint64_t* current_support_v,
    const std::uint8_t* current_closed_rank,
    const std::uint64_t* current_strict_offsets,
    const std::uint64_t* current_strict_point_ids,
    const std::uint64_t* current_shell_offsets,
    const std::uint64_t* current_shell_point_ids,
    std::uint64_t prior_record_count,
    const CandidateClassification* classifications,
    const std::uint64_t* sorted_candidate_indices,
    std::uint64_t candidate_count,
    std::uint64_t prior_strict_payload_count,
    std::uint64_t prior_shell_payload_count,
    std::uint64_t destination_payload_capacity,
    DeviceReceiptControl* control,
    const std::uint64_t* destination_support_u,
    const std::uint64_t* destination_support_v,
    const std::uint64_t* destination_rank_offsets,
    const std::uint64_t* destination_strict_offsets,
    std::uint64_t* destination_strict_point_ids,
    const std::uint64_t* destination_shell_offsets,
    std::uint64_t* destination_shell_point_ids) {
  if (control->commit_allowed == 0U) {
    return;
  }
  const std::uint64_t index =
      static_cast<std::uint64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index < prior_record_count) {
    const std::uint64_t rank = current_closed_rank[index];
    const std::uint64_t destination_begin = destination_rank_offsets[rank];
    const std::uint64_t destination_end =
        destination_rank_offsets[rank + UINT64_C(1)];
    const std::uint64_t destination = lower_bound_current_pair(
        destination_support_u,
        destination_support_v,
        destination_begin,
        destination_end,
        current_support_u[index],
        current_support_v[index]);
    if (destination >= destination_end ||
        destination >= control->required_catalog_record_count) {
      atomicAdd(
          reinterpret_cast<unsigned long long*>(&control->invariant_count),
          1ULL);
      return;
    }
    const std::uint64_t strict_begin = current_strict_offsets[index];
    const std::uint64_t strict_count =
        current_strict_offsets[index + UINT64_C(1)] - strict_begin;
    const std::uint64_t shell_begin = current_shell_offsets[index];
    const std::uint64_t shell_count =
        current_shell_offsets[index + UINT64_C(1)] - shell_begin;
    const std::uint64_t destination_strict =
        destination_strict_offsets[destination];
    const std::uint64_t destination_shell =
        destination_shell_offsets[destination];
    bool invalid = destination_support_u[destination] !=
                       current_support_u[index] ||
                   destination_support_v[destination] !=
                       current_support_v[index] ||
                   strict_begin > prior_strict_payload_count ||
                   strict_count > prior_strict_payload_count - strict_begin ||
                   shell_begin > prior_shell_payload_count ||
                   shell_count > prior_shell_payload_count - shell_begin ||
                   destination_strict > destination_payload_capacity ||
                   strict_count >
                       destination_payload_capacity - destination_strict ||
                   destination_shell > destination_payload_capacity ||
                   shell_count >
                       destination_payload_capacity - destination_shell;
    if (invalid) {
      atomicAdd(
          reinterpret_cast<unsigned long long*>(&control->invariant_count),
          1ULL);
      return;
    }
    for (std::uint64_t payload = 0U; payload < strict_count; ++payload) {
      destination_strict_point_ids[destination_strict + payload] =
          current_strict_point_ids[strict_begin + payload];
    }
    for (std::uint64_t payload = 0U; payload < shell_count; ++payload) {
      destination_shell_point_ids[destination_shell + payload] =
          current_shell_point_ids[shell_begin + payload];
    }
  }

  if (index < control->transaction_accepted_record_count) {
    const std::uint64_t logical_index = sorted_candidate_indices[index];
    if (logical_index >= candidate_count) {
      return;
    }
    const CandidateClassification classified = classifications[logical_index];
    const std::uint64_t rank = classified.closed_rank;
    const std::uint64_t destination_begin = destination_rank_offsets[rank];
    const std::uint64_t destination_end =
        destination_rank_offsets[rank + UINT64_C(1)];
    const std::uint64_t destination = lower_bound_current_pair(
        destination_support_u,
        destination_support_v,
        destination_begin,
        destination_end,
        classified.support_u,
        classified.support_v);
    if (destination >= destination_end ||
        destination >= control->required_catalog_record_count) {
      atomicAdd(
          reinterpret_cast<unsigned long long*>(&control->invariant_count),
          1ULL);
      return;
    }
    const std::uint64_t destination_strict =
        destination_strict_offsets[destination];
    const std::uint64_t destination_shell =
        destination_shell_offsets[destination];
    bool invalid = destination_support_u[destination] != classified.support_u ||
                   destination_support_v[destination] != classified.support_v ||
                   destination_strict > destination_payload_capacity ||
                   classified.strict_count >
                       destination_payload_capacity - destination_strict ||
                   destination_shell > destination_payload_capacity ||
                   classified.shell_count >
                       destination_payload_capacity - destination_shell;
    if (invalid) {
      atomicAdd(
          reinterpret_cast<unsigned long long*>(&control->invariant_count),
          1ULL);
      return;
    }
    for (std::uint64_t payload = 0U; payload < classified.strict_count;
         ++payload) {
      destination_strict_point_ids[destination_strict + payload] =
          classified.payload_point_ids[payload];
    }
    for (std::uint64_t payload = 0U; payload < classified.shell_count;
         ++payload) {
      destination_shell_point_ids[destination_shell + payload] =
          classified.payload_point_ids[classified.strict_count + payload];
    }
  }
}

__global__ void commit_last_partner_positions_kernel(
    const Phase15MortonYao48DeviceTiledAnchorControl* controls,
    const Phase15MortonYao48DeviceTiledCandidateRecord* candidates,
    std::uint64_t anchor_count,
    std::uint64_t candidate_stride,
    const DeviceReceiptControl* control,
    std::uint64_t* last_partner_positions) {
  if (control->commit_allowed == 0U) {
    return;
  }
  const std::uint64_t anchor =
      static_cast<std::uint64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (anchor >= anchor_count || controls[anchor].candidate_count == 0U) {
    return;
  }
  const std::uint64_t last = controls[anchor].candidate_count - UINT64_C(1);
  if (last < candidate_stride) {
    last_partner_positions[anchor] =
        candidates[anchor * candidate_stride + last].partner_morton_position;
  }
}

void validate_launch(
    const Phase15MortonYao48RankedPairTileClassifierContextState& context,
    const Phase15MortonYao48DeviceCandidateTilePrivateViews& views,
    const Phase15MortonYao48RankedPairTileClassifierRequest& request) {
  const std::size_t node_count = checked_node_count(request.point_count);
  const std::size_t anchor_count = request.anchor_end - request.anchor_begin;
  const std::size_t expected_candidate_capacity = checked_product(
      anchor_count,
      views.candidate_segment_stride_records,
      "the ranked-pair physical candidate extent overflows size_t");
  const std::size_t coordinate_word_count = checked_product(
      request.point_count,
      kAxisCount,
      "the ranked-pair coordinate extent overflows size_t");
  if (!context.config_matches(request.fixed_config) || !views.ready ||
      views.host_fake || views.retained_authority_identity == nullptr ||
      views.source_owner_identity == nullptr ||
      views.source_cloud_identity == nullptr ||
      !views.source_device_views_retained ||
      !views.source_views_bound_to_snapshot_identity ||
      views.cuda_device < 0 || request.source_snapshot_epoch == 0U ||
      request.candidate_buffer_epoch == 0U ||
      request.catalog_buffer_epoch == 0U || request.tile_epoch == 0U ||
      request.chunk_sequence == 0U || request.point_count < 2U ||
      request.certified_node_count != node_count ||
      request.anchor_begin == 0U ||
      request.anchor_begin >= request.anchor_end ||
      request.anchor_end > request.point_count ||
      request.fixed_config.maximum_closed_rank <
          morton_yao48_ranked_pair_tile_classifier_minimum_closed_rank ||
      request.fixed_config.maximum_closed_rank >
          morton_yao48_ranked_pair_tile_classifier_maximum_closed_rank ||
      views.prune_semantics !=
          MortonYao48DeviceTiledPairFrontierPruneSemantics::
              closed_rank_window ||
      views.required_witness_count !=
          request.fixed_config.maximum_closed_rank - 1U ||
      request.source_snapshot_epoch != views.source_snapshot_epoch ||
      request.candidate_buffer_epoch != views.candidate_buffer_epoch ||
      request.point_count != views.point_count ||
      request.certified_node_count != views.certified_node_count ||
      request.anchor_begin != views.anchor_begin ||
      request.anchor_end != views.anchor_end ||
      views.authorized_anchor_control_extent != anchor_count ||
      views.physical_anchor_control_capacity < anchor_count ||
      views.anchor_control_stride_bytes !=
          sizeof(Phase15MortonYao48DeviceTiledAnchorControl) ||
      views.candidate_segment_stride_records == 0U ||
      views.physical_candidate_record_capacity !=
          expected_candidate_capacity ||
      request.candidate_count > expected_candidate_capacity ||
      views.retained_coordinate_word_capacity < coordinate_word_count ||
      views.retained_morton_point_id_capacity < request.point_count ||
      views.retained_node_capacity < request.certified_node_count ||
      views.physical_device_arena_capacity_bytes == 0U) {
    throw std::invalid_argument(
        "the resident ranked-pair classifier received malformed authority "
        "or request metadata");
  }
  static_cast<void>(checked_cub_count(
      anchor_count + 1U,
      "the ranked-pair anchor extent exceeds the CUB int ABI"));
  if (request.candidate_count != 0U) {
    static_cast<void>(checked_cub_count(
        static_cast<std::size_t>(request.candidate_count),
        "the ranked-pair candidate extent exceeds the CUB int ABI"));
  }
  static_cast<void>(checked_cub_count(
      request.fixed_config.catalog_record_capacity + 1U,
      "the ranked-pair record extent exceeds the CUB int ABI"));
  require_device_pointer(
      views.device_coordinate_bits,
      views.cuda_device,
      alignof(std::uint64_t),
      "coordinate");
  require_device_pointer(
      views.device_morton_point_ids,
      views.cuda_device,
      alignof(std::uint64_t),
      "Morton point-id");
  require_device_pointer(
      views.device_nodes,
      views.cuda_device,
      alignof(Phase15MortonYao48RankedPairNode),
      "LBVH node");
  require_device_pointer(
      views.device_candidate_records,
      views.cuda_device,
      alignof(Phase15MortonYao48DeviceTiledCandidateRecord),
      "candidate");
  require_device_pointer(
      views.device_anchor_controls,
      views.cuda_device,
      alignof(Phase15MortonYao48DeviceTiledAnchorControl),
      "anchor control");
}

[[nodiscard]] unsigned int launch_blocks(std::size_t count) {
  const std::size_t blocks =
      (count + static_cast<std::size_t>(kThreadsPerBlock) - 1U) /
      static_cast<std::size_t>(kThreadsPerBlock);
  if (blocks == 0U || blocks > std::numeric_limits<unsigned int>::max()) {
    throw std::length_error(
        "a ranked-pair classifier CUDA launch grid is invalid");
  }
  return static_cast<unsigned int>(blocks);
}

void check_kernel(const char* operation) {
  check_cuda(cudaGetLastError(), operation);
}

}  // namespace

Phase15MortonYao48RankedPairTileClassifierReceipt
classify_phase15_morton_yao48_ranked_pair_tile_on_device(
    Phase15MortonYao48RankedPairTileClassifierContextState& context,
    const Phase15MortonYao48DeviceCandidateTilePrivateViews& candidate_views,
    const Phase15MortonYao48RankedPairTileClassifierRequest& request) {
  validate_launch(context, candidate_views, request);
  DeviceGuard guard{candidate_views.cuda_device};

  const std::size_t anchor_count = request.anchor_end - request.anchor_begin;
  const std::size_t physical_candidate_capacity =
      candidate_views.physical_candidate_record_capacity;
  std::shared_ptr<RankedPairCudaResources> resources;
  if (context.launcher_state()) {
    resources = std::static_pointer_cast<RankedPairCudaResources>(
        context.launcher_state());
    if (!resources->matches(
            candidate_views,
            request,
            anchor_count,
            physical_candidate_capacity)) {
      throw std::runtime_error(
          "the resident ranked-pair CUDA arena rejected a rollback, source "
          "change, or workspace growth");
    }
  } else {
    if (request.prior_candidate_count != 0U ||
        request.prior_accepted_record_count != 0U ||
        request.prior_above_window_count != 0U ||
        request.prior_unresolved_count != 0U ||
        request.prior_strict_payload_point_id_count != 0U ||
        request.prior_shell_payload_point_id_count != 0U ||
        request.prior_exact_fallback_count != 0U) {
      throw std::runtime_error(
          "a new resident ranked-pair CUDA arena received nonzero prior "
          "counters");
    }
    resources = std::make_shared<RankedPairCudaResources>(
        candidate_views,
        request,
        anchor_count,
        physical_candidate_capacity);
    context.replace_launcher_state(resources);
  }

  cudaStream_t stream = resources->stream();
  const std::size_t candidate_count =
      static_cast<std::size_t>(request.candidate_count);
  const std::size_t rank_offset_count =
      request.fixed_config.maximum_closed_rank + 2U;
  check_cuda(
      cudaMemsetAsync(
          resources->control().get(),
          0,
          sizeof(DeviceReceiptControl),
          stream),
      "cudaMemsetAsync ranked-pair scalar control");
  check_cuda(
      cudaMemsetAsync(
          resources->accepted_counts().get(),
          0,
          (candidate_count + 1U) * sizeof(std::uint64_t),
          stream),
      "cudaMemsetAsync ranked-pair accepted counts");
  check_cuda(
      cudaMemsetAsync(
          resources->strict_counts().get(),
          0,
          (candidate_count + 1U) * sizeof(std::uint64_t),
          stream),
      "cudaMemsetAsync ranked-pair strict counts");
  check_cuda(
      cudaMemsetAsync(
          resources->shell_counts().get(),
          0,
          (candidate_count + 1U) * sizeof(std::uint64_t),
          stream),
      "cudaMemsetAsync ranked-pair shell counts");
  check_cuda(
      cudaMemsetAsync(
          resources->transaction_rank_counts().get(),
          0,
          rank_offset_count * sizeof(std::uint64_t),
          stream),
      "cudaMemsetAsync ranked-pair transaction rank counts");
  if (!request.same_tile_continuation) {
    check_cuda(
        cudaMemsetAsync(
            resources->last_partner_positions().get(),
            0xff,
            anchor_count * sizeof(std::uint64_t),
            stream),
        "cudaMemsetAsync ranked-pair continuation witnesses");
  }

  std::size_t logical_launch_count = 0U;
  validate_anchor_controls_kernel<<<
      launch_blocks(anchor_count + 1U),
      kThreadsPerBlock,
      0,
      stream>>>(
      candidate_views.device_anchor_controls,
      static_cast<std::uint64_t>(anchor_count),
      static_cast<std::uint64_t>(request.anchor_begin),
      static_cast<std::uint64_t>(
          candidate_views.candidate_segment_stride_records),
      request.tile_epoch,
      request.chunk_sequence,
      resources->anchor_counts().get(),
      resources->control().get());
  check_kernel("ranked-pair anchor-control validation launch");
  ++logical_launch_count;

  std::size_t anchor_scan_workspace_bytes =
      resources->scan_workspace_bytes();
  check_cuda(
      cub::DeviceScan::ExclusiveSum(
          resources->scan_workspace().get(),
          anchor_scan_workspace_bytes,
          resources->anchor_counts().get(),
          resources->anchor_offsets().get(),
          checked_cub_count(
              anchor_count + 1U,
              "the ranked-pair anchor scan extent exceeds int"),
          stream),
      "CUB ranked-pair anchor count scan");
  ++logical_launch_count;

  if (candidate_count != 0U) {
    classify_candidates_multiorder_kernel<<<
        launch_blocks(candidate_count),
        kThreadsPerBlock,
        0,
        stream>>>(
        candidate_views.device_coordinate_bits,
        candidate_views.device_morton_point_ids,
        static_cast<const Phase15MortonYao48RankedPairNode*>(
            candidate_views.device_nodes),
        candidate_views.device_candidate_records,
        resources->anchor_offsets().get(),
        resources->last_partner_positions().get(),
        static_cast<std::uint64_t>(request.point_count),
        static_cast<std::uint64_t>(request.certified_node_count),
        static_cast<std::uint64_t>(anchor_count),
        static_cast<std::uint64_t>(request.anchor_begin),
        static_cast<std::uint64_t>(
            candidate_views.candidate_segment_stride_records),
        request.candidate_count,
        static_cast<std::uint64_t>(request.fixed_config.maximum_closed_rank),
        static_cast<std::uint64_t>(
            request.fixed_config.exact_fallback_capacity),
        request.same_tile_continuation,
        resources->classifications().get(),
        resources->accepted_counts().get(),
        resources->strict_counts().get(),
        resources->shell_counts().get(),
        resources->sort_keys_a().get(),
        resources->sort_values_a().get(),
        resources->transaction_rank_counts().get(),
        resources->fallback_candidate_indices().get(),
        resources->control().get());
    check_kernel("ranked-pair exact multiorder classification launch");
    ++logical_launch_count;
  }

  const auto scan_candidate_counts = [&](const std::uint64_t* input,
                                         std::uint64_t* output,
                                         const char* operation) {
    std::size_t workspace_bytes = resources->scan_workspace_bytes();
    check_cuda(
        cub::DeviceScan::ExclusiveSum(
            resources->scan_workspace().get(),
            workspace_bytes,
            input,
            output,
            checked_cub_count(
                candidate_count + 1U,
                "the ranked-pair candidate scan extent exceeds int"),
            stream),
        operation);
    ++logical_launch_count;
  };
  scan_candidate_counts(
      resources->accepted_counts().get(),
      resources->accepted_offsets().get(),
      "CUB ranked-pair accepted count scan");
  scan_candidate_counts(
      resources->strict_counts().get(),
      resources->strict_offsets().get(),
      "CUB ranked-pair strict payload scan");
  scan_candidate_counts(
      resources->shell_counts().get(),
      resources->shell_offsets().get(),
      "CUB ranked-pair shell payload scan");

  std::uint64_t* sorted_candidate_indices =
      resources->sort_values_a().get();
  std::uint64_t* alternate_candidate_indices =
      resources->sort_values_b().get();
  if (candidate_count != 0U) {
    const auto sort_component = [&](CanonicalSortComponent component,
                                    const char* key_operation,
                                    const char* sort_operation) {
      build_canonical_sort_keys_kernel<<<
          launch_blocks(candidate_count),
          kThreadsPerBlock,
          0,
          stream>>>(
          resources->classifications().get(),
          sorted_candidate_indices,
          request.candidate_count,
          component,
          resources->sort_keys_a().get());
      check_kernel(key_operation);
      ++logical_launch_count;
      cub::DoubleBuffer<std::uint64_t> keys{
          resources->sort_keys_a().get(), resources->sort_keys_b().get()};
      cub::DoubleBuffer<std::uint64_t> values{
          sorted_candidate_indices, alternate_candidate_indices};
      std::size_t workspace_bytes = resources->sort_workspace_bytes();
      check_cuda(
          cub::DeviceRadixSort::SortPairs(
              resources->sort_workspace().get(),
              workspace_bytes,
              keys,
              values,
              checked_cub_count(
                  candidate_count,
                  "the ranked-pair transaction sort extent exceeds int"),
              0,
              64,
              stream),
          sort_operation);
      ++logical_launch_count;
      sorted_candidate_indices = values.Current();
      alternate_candidate_indices = values.Alternate();
    };
    // Stable least-significant-component passes produce the canonical tuple
    // order (closed_rank, support_u, support_v).
    sort_component(
        CanonicalSortComponent::support_v,
        "ranked-pair support-v key launch",
        "CUB ranked-pair stable support-v sort");
    sort_component(
        CanonicalSortComponent::support_u,
        "ranked-pair support-u key launch",
        "CUB ranked-pair stable support-u sort");
    sort_component(
        CanonicalSortComponent::closed_rank,
        "ranked-pair closed-rank key launch",
        "CUB ranked-pair stable closed-rank sort");
  }

  prepare_receipt_control_kernel<<<1U, 1U, 0U, stream>>>(
      resources->anchor_offsets().get(),
      static_cast<std::uint64_t>(anchor_count),
      resources->accepted_offsets().get(),
      resources->strict_offsets().get(),
      resources->shell_offsets().get(),
      request.candidate_count,
      request.prior_candidate_count,
      request.prior_accepted_record_count,
      request.prior_above_window_count,
      request.prior_unresolved_count,
      request.prior_strict_payload_point_id_count,
      request.prior_shell_payload_point_id_count,
      request.prior_exact_fallback_count,
      static_cast<std::uint64_t>(
          request.fixed_config.catalog_record_capacity),
      static_cast<std::uint64_t>(
          request.fixed_config.payload_point_id_capacity),
      static_cast<std::uint64_t>(
          request.fixed_config.exact_fallback_capacity),
      request.frontier_closed_after_chunk,
      resources->control().get());
  check_kernel("ranked-pair resident capacity decision launch");
  ++logical_launch_count;

  CatalogBuffers& current = resources->current_catalog();
  CatalogBuffers& destination = resources->destination_catalog();
  validate_canonical_inputs_kernel<<<
      launch_blocks(std::max<std::size_t>(
          1U, std::max(resources->record_count(), candidate_count))),
      kThreadsPerBlock,
      0,
      stream>>>(
      resources->classifications().get(),
      sorted_candidate_indices,
      request.candidate_count,
      current.support_u.get(),
      current.support_v.get(),
      current.closed_rank.get(),
      static_cast<std::uint64_t>(resources->record_count()),
      static_cast<std::uint64_t>(request.point_count),
      static_cast<std::uint64_t>(request.fixed_config.maximum_closed_rank),
      resources->control().get());
  check_kernel("ranked-pair canonical input validation launch");
  ++logical_launch_count;
  finalize_destination_preparation_kernel<<<1U, 1U, 0U, stream>>>(
      resources->control().get());
  check_kernel("ranked-pair canonical input finalization launch");
  ++logical_launch_count;

  const bool destination_catalog_built = candidate_count != 0U;
  if (destination_catalog_built) {
  const std::size_t upper_record_count = std::min(
      request.fixed_config.catalog_record_capacity,
      checked_sum(
          resources->record_count(),
          candidate_count,
          "the ranked-pair transaction record upper bound overflows"));
  check_cuda(
      cudaMemsetAsync(
          resources->catalog_strict_counts().get(),
          0,
          (upper_record_count + 1U) * sizeof(std::uint64_t),
          stream),
      "cudaMemsetAsync ranked-pair catalog strict counts");
  check_cuda(
      cudaMemsetAsync(
          resources->catalog_shell_counts().get(),
          0,
          (upper_record_count + 1U) * sizeof(std::uint64_t),
          stream),
      "cudaMemsetAsync ranked-pair catalog shell counts");

  const std::size_t record_prepare_extent =
      std::max(resources->record_count(), candidate_count);
  prepare_destination_records_kernel<<<
      launch_blocks(std::max<std::size_t>(1U, record_prepare_extent)),
      kThreadsPerBlock,
      0,
      stream>>>(
      current.support_u.get(),
      current.support_v.get(),
      current.closed_rank.get(),
      current.rank_offsets.get(),
      current.strict_offsets.get(),
      current.shell_offsets.get(),
      static_cast<std::uint64_t>(resources->record_count()),
      resources->classifications().get(),
      sorted_candidate_indices,
      request.candidate_count,
      resources->transaction_rank_counts().get(),
      static_cast<std::uint64_t>(request.fixed_config.maximum_closed_rank),
      static_cast<std::uint64_t>(
          request.fixed_config.catalog_record_capacity),
      static_cast<std::uint64_t>(resources->strict_count()),
      static_cast<std::uint64_t>(resources->shell_count()),
      resources->control().get(),
      destination.support_u.get(),
      destination.support_v.get(),
      destination.closed_rank.get(),
      resources->catalog_strict_counts().get(),
      resources->catalog_shell_counts().get());
  check_kernel("ranked-pair destination record preparation launch");
  ++logical_launch_count;

  finalize_destination_preparation_kernel<<<1U, 1U, 0U, stream>>>(
      resources->control().get());
  check_kernel("ranked-pair destination preparation validation launch");
  ++logical_launch_count;

  build_destination_rank_offsets_kernel<<<1U, 1U, 0U, stream>>>(
      current.rank_offsets.get(),
      resources->transaction_rank_counts().get(),
      static_cast<std::uint64_t>(request.fixed_config.maximum_closed_rank),
      resources->control().get(),
      destination.rank_offsets.get());
  check_kernel("ranked-pair destination rank-offset launch");
  ++logical_launch_count;

  const auto scan_catalog_counts = [&](const std::uint64_t* input,
                                       std::uint64_t* output,
                                       const char* operation) {
    std::size_t workspace_bytes = resources->scan_workspace_bytes();
    check_cuda(
        cub::DeviceScan::ExclusiveSum(
            resources->scan_workspace().get(),
            workspace_bytes,
            input,
            output,
            checked_cub_count(
                upper_record_count + 1U,
                "the ranked-pair catalog scan extent exceeds int"),
            stream),
        operation);
    ++logical_launch_count;
  };
  scan_catalog_counts(
      resources->catalog_strict_counts().get(),
      destination.strict_offsets.get(),
      "CUB ranked-pair destination strict scan");
  scan_catalog_counts(
      resources->catalog_shell_counts().get(),
      destination.shell_offsets.get(),
      "CUB ranked-pair destination shell scan");

  validate_destination_layout_kernel<<<1U, kThreadsPerBlock, 0U, stream>>>(
      destination.rank_offsets.get(),
      destination.strict_offsets.get(),
      destination.shell_offsets.get(),
      static_cast<std::uint64_t>(request.fixed_config.maximum_closed_rank),
      static_cast<std::uint64_t>(
          request.fixed_config.catalog_record_capacity),
      static_cast<std::uint64_t>(
          request.fixed_config.payload_point_id_capacity),
      resources->control().get());
  check_kernel("ranked-pair destination layout validation launch");
  ++logical_launch_count;
  finalize_destination_preparation_kernel<<<1U, 1U, 0U, stream>>>(
      resources->control().get());
  check_kernel("ranked-pair destination layout finalization launch");
  ++logical_launch_count;

  emit_destination_payload_kernel<<<
      launch_blocks(std::max<std::size_t>(1U, record_prepare_extent)),
      kThreadsPerBlock,
      0,
      stream>>>(
      current.support_u.get(),
      current.support_v.get(),
      current.closed_rank.get(),
      current.strict_offsets.get(),
      current.strict_point_ids.get(),
      current.shell_offsets.get(),
      current.shell_point_ids.get(),
      static_cast<std::uint64_t>(resources->record_count()),
      resources->classifications().get(),
      sorted_candidate_indices,
      request.candidate_count,
      static_cast<std::uint64_t>(resources->strict_count()),
      static_cast<std::uint64_t>(resources->shell_count()),
      static_cast<std::uint64_t>(
          request.fixed_config.payload_point_id_capacity),
      resources->control().get(),
      destination.support_u.get(),
      destination.support_v.get(),
      destination.rank_offsets.get(),
      destination.strict_offsets.get(),
      destination.strict_point_ids.get(),
      destination.shell_offsets.get(),
      destination.shell_point_ids.get());
  check_kernel("ranked-pair destination payload emission launch");
  ++logical_launch_count;
  finalize_destination_preparation_kernel<<<1U, 1U, 0U, stream>>>(
      resources->control().get());
  check_kernel("ranked-pair payload emission finalization launch");
  ++logical_launch_count;

  commit_last_partner_positions_kernel<<<
      launch_blocks(anchor_count),
      kThreadsPerBlock,
      0,
      stream>>>(
      candidate_views.device_anchor_controls,
      candidate_views.device_candidate_records,
      static_cast<std::uint64_t>(anchor_count),
      static_cast<std::uint64_t>(
          candidate_views.candidate_segment_stride_records),
      resources->control().get(),
      resources->last_partner_positions().get());
  check_kernel("ranked-pair continuation witness commit launch");
  ++logical_launch_count;
  }

  DeviceReceiptControl host_control{};
  check_cuda(
      cudaMemcpyAsync(
          &host_control,
          resources->control().get(),
          sizeof(host_control),
          cudaMemcpyDeviceToHost,
          stream),
      "cudaMemcpyAsync ranked-pair scalar receipt D2H");
  check_cuda(
      cudaStreamSynchronize(stream),
      "cudaStreamSynchronize resident ranked-pair transaction");

  if (host_control.failure_code == kFailureMalformed ||
      host_control.failure_code == kFailureInvariant) {
    throw std::runtime_error(
        "the resident ranked-pair CUDA transaction failed closed on "
        "malformed input or an internal invariant");
  }

  Phase15MortonYao48RankedPairTileClassifierReceipt receipt;
  receipt.fixed_config = request.fixed_config;
  receipt.source_snapshot_epoch = request.source_snapshot_epoch;
  receipt.candidate_buffer_epoch = request.candidate_buffer_epoch;
  receipt.catalog_buffer_epoch = request.catalog_buffer_epoch;
  receipt.tile_epoch = request.tile_epoch;
  receipt.chunk_sequence = request.chunk_sequence;
  receipt.point_count = request.point_count;
  receipt.certified_node_count = request.certified_node_count;
  receipt.anchor_begin = request.anchor_begin;
  receipt.anchor_end = request.anchor_end;
  receipt.transaction_candidate_count = request.candidate_count;
  receipt.transaction_exact_fallback_count =
      host_control.exact_fallback_count;
  receipt.transaction_certified_pruned_pair_mass =
      request.transaction_certified_pruned_pair_mass;
  receipt.cumulative_certified_pruned_pair_mass =
      request.cumulative_certified_pruned_pair_mass;
  receipt.unordered_pair_universe_count =
      request.unordered_pair_universe_count;
  receipt.frontier_unresolved_pair_mass =
      request.frontier_unresolved_pair_mass;
  receipt.required_catalog_record_count =
      host_control.required_catalog_record_count;
  receipt.required_payload_point_id_count =
      host_control.required_payload_point_id_count;
  receipt.required_exact_fallback_count =
      host_control.required_exact_fallback_count;
  receipt.cumulative_candidate_count =
      host_control.cumulative_candidate_count;
  receipt.cumulative_accepted_record_count =
      host_control.cumulative_accepted_record_count;
  receipt.cumulative_above_window_count =
      host_control.cumulative_above_window_count;
  receipt.cumulative_unresolved_count =
      host_control.cumulative_unresolved_count;
  receipt.cumulative_strict_payload_point_id_count =
      host_control.cumulative_strict_payload_count;
  receipt.cumulative_shell_payload_point_id_count =
      host_control.cumulative_shell_payload_count;
  receipt.cumulative_exact_fallback_count =
      host_control.cumulative_exact_fallback_count;
  receipt.status = host_control.status;
  receipt.stop_reason = host_control.stop_reason;
  receipt.failure_code = host_control.failure_code;
  receipt.launcher_call_count = 1U;
  receipt.kernel_launch_count = logical_launch_count;
  receipt.synchronization_count = 1U;
  receipt.cuda_device = candidate_views.cuda_device;
  receipt.execution_kind =
      Phase15MortonYao48RankedPairTileClassifierExecutionKind::cuda;
  receipt.same_tile_continuation = request.same_tile_continuation;
  receipt.tile_closed_after_chunk = request.tile_closed_after_chunk;
  receipt.frontier_closed_after_chunk = request.frontier_closed_after_chunk;
  receipt.source_identity_authenticated = true;
  receipt.source_epoch_authenticated = true;
  receipt.candidate_epoch_authenticated = true;
  receipt.tile_chunk_sequence_authenticated = true;
  receipt.candidate_tile_lease_alive_during_launch = true;
  receipt.fixed_linear_capacities_validated = true;
  receipt.exact_multiorder_classification_implemented = true;
  receipt.one_multiorder_launch_used = true;
  receipt.count_scan_payload_layout_validated = true;
  receipt.output_records_sorted_unique = host_control.commit_allowed != 0U;
  receipt.output_rollback_validated = true;

  if (host_control.commit_allowed != 0U) {
    receipt.transaction_accepted_record_count =
        host_control.transaction_accepted_record_count;
    receipt.transaction_above_window_count = host_control.above_window_count;
    receipt.transaction_strict_payload_point_id_count =
        host_control.transaction_strict_payload_count;
    receipt.transaction_shell_payload_point_id_count =
        host_control.transaction_shell_payload_count;
    resources->commit_success(
        request, host_control, destination_catalog_built);
    CatalogBuffers& output = resources->current_catalog();
    receipt.output.owner = resources;
    receipt.output.support_u = output.support_u.get();
    receipt.output.support_v = output.support_v.get();
    receipt.output.closed_rank = output.closed_rank.get();
    receipt.output.rank_offsets = output.rank_offsets.get();
    receipt.output.strict_offsets = output.strict_offsets.get();
    receipt.output.strict_point_ids = output.strict_point_ids.get();
    receipt.output.shell_offsets = output.shell_offsets.get();
    receipt.output.shell_point_ids = output.shell_point_ids.get();
    receipt.output.catalog_record_capacity =
        request.fixed_config.catalog_record_capacity;
    receipt.output.payload_point_id_capacity =
        request.fixed_config.payload_point_id_capacity;
    receipt.output.exact_fallback_capacity =
        request.fixed_config.exact_fallback_capacity;
    receipt.output.catalog_record_count = static_cast<std::size_t>(
        host_control.cumulative_accepted_record_count);
    receipt.output.strict_point_id_count = static_cast<std::size_t>(
        host_control.cumulative_strict_payload_count);
    receipt.output.shell_point_id_count = static_cast<std::size_t>(
        host_control.cumulative_shell_payload_count);
    receipt.output.rank_offset_count = rank_offset_count;
    receipt.output.record_offset_count =
        receipt.output.catalog_record_count + 1U;
  } else {
    receipt.transaction_unresolved_count = request.candidate_count;
    receipt.output_invalidated = true;
    context.replace_launcher_state({});
  }

  receipt.receipt_digest =
      phase15_morton_yao48_ranked_pair_tile_classifier_receipt_digest(
          receipt);
  guard.restore();
  return receipt;
}

}  // namespace morsehgp3d::gpu::detail
