#include "phase9_pair_support_phi_internal.hpp"

#include "phase2b_interval.cuh"

#include <cuda_runtime.h>
#include <cub/device/device_scan.cuh>

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
constexpr std::uint64_t kRankDecisionDiscard = 1U;
constexpr std::uint64_t kRankDecisionExpand = 2U;
constexpr std::uint64_t kRankDecisionStrictTerminal = 3U;
constexpr std::uint64_t kRankDecisionAnchorTerminal = 4U;
constexpr std::uint64_t kRankDecisionLeafTerminal = 5U;
constexpr std::uint64_t kRankDecisionInvalid = 6U;
constexpr std::uint64_t kRankStacklessRunning = 0U;
constexpr std::uint64_t kRankStacklessKeepComplete = 1U;
constexpr std::uint64_t kRankStacklessPruneComplete = 2U;
constexpr std::uint64_t kRankStacklessVisitBudget = 3U;
constexpr std::uint64_t kRankStacklessTerminalCapacity = 4U;
constexpr std::uint64_t kRankStacklessFailure = 5U;

struct PairSupportRankDeviceDecision {
  std::uint64_t code{kRankDecisionInvalid};
  std::uint64_t reserved{pair_support_phi_sentinel};
};
static_assert(std::is_standard_layout_v<PairSupportRankDeviceDecision>);
static_assert(std::is_trivially_copyable_v<PairSupportRankDeviceDecision>);
static_assert(
    sizeof(PairSupportRankDeviceDecision) ==
    2U * sizeof(std::uint64_t));

struct PairSupportRankStacklessControl {
  std::uint64_t status{kRankStacklessRunning};
  std::uint64_t failure_code{};
  std::uint64_t visited_node_count{};
  std::uint64_t terminal_count{};
  std::uint64_t strict_interior_point_count{};
};
static_assert(std::is_standard_layout_v<PairSupportRankStacklessControl>);
static_assert(std::is_trivially_copyable_v<PairSupportRankStacklessControl>);
static_assert(
    sizeof(PairSupportRankStacklessControl) ==
    pair_support_rank_stackless_control_byte_count);

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

[[nodiscard]] std::size_t checked_add_bytes(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left > std::numeric_limits<std::size_t>::max() - right) {
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

  [[nodiscard]] bool initialize_snapshot(
      std::span<const PairSupportPhiNodeInputRecord> nodes) {
    if (node_count_ != 0U) {
      if (node_count_ != nodes.size() || nodes_.count() != nodes.size()) {
        throw std::logic_error(
            "a Phase 9 pair-support phi context changed its resident snapshot");
      }
      return false;
    }
    if (nodes.empty()) {
      throw std::invalid_argument(
          "a Phase 9 pair-support phi resident snapshot is empty");
    }
    nodes_.allocate(nodes.size(), "cudaMalloc Phase 9 pair-support LBVH nodes");
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
    return true;
  }

  void initialize_phi(std::size_t maximum_query_count) {
    if (maximum_query_count_ != 0U) {
      if (maximum_query_count_ != maximum_query_count ||
          queries_.count() != maximum_query_count ||
          records_.count() != maximum_query_count) {
        throw std::logic_error(
            "a Phase 9 pair-support phi context changed its P1 capacity");
      }
      return;
    }
    if (maximum_query_count == 0U) {
      throw std::invalid_argument(
          "a Phase 9 pair-support phi capacity is empty");
    }
    queries_.allocate(
        maximum_query_count,
        "cudaMalloc Phase 9 pair-support phi query buffer");
    records_.allocate(
        maximum_query_count,
        "cudaMalloc Phase 9 pair-support phi proposal buffer");
    maximum_query_count_ = maximum_query_count;
  }

  [[nodiscard]] bool initialize_rank_escape_snapshot(
      std::span<const std::uint32_t> escape_node_indices) {
    if (rank_escape_node_count_ != 0U) {
      if (rank_escape_node_count_ != escape_node_indices.size() ||
          rank_escape_node_indices_.count() !=
              escape_node_indices.size()) {
        throw std::logic_error(
            "a Phase 9 stackless rank-prune context changed its resident "
            "escape snapshot");
      }
      return false;
    }
    if (escape_node_indices.empty()) {
      throw std::invalid_argument(
          "a Phase 9 stackless rank-prune escape snapshot is empty");
    }
    rank_escape_node_indices_.allocate(
        escape_node_indices.size(),
        "cudaMalloc Phase 9 stackless rank-prune escape snapshot");
    check_cuda(
        cudaMemcpyAsync(
            rank_escape_node_indices_.get(),
            escape_node_indices.data(),
            escape_node_indices.size() * sizeof(std::uint32_t),
            cudaMemcpyHostToDevice,
            stream_),
        "cudaMemcpyAsync Phase 9 stackless rank-prune escape snapshot "
        "host-to-device");
    synchronize();
    rank_escape_node_count_ = escape_node_indices.size();
    return true;
  }

  void initialize_rank_stackless(
      std::size_t maximum_product_count,
      std::size_t maximum_terminal_count) {
    if (rank_stackless_product_capacity_ != 0U) {
      if (rank_stackless_product_capacity_ != maximum_product_count ||
          rank_stackless_terminal_capacity_ != maximum_terminal_count ||
          rank_stackless_products_.count() != maximum_product_count ||
          rank_stackless_terminals_.count() != maximum_terminal_count ||
          rank_stackless_control_.count() != 1U) {
        throw std::logic_error(
            "a Phase 9 stackless rank-prune context changed its fixed "
            "capacities");
      }
      return;
    }
    if (maximum_product_count != 1U ||
        maximum_terminal_count == 0U) {
      throw std::invalid_argument(
          "a Phase 9 stackless rank-prune capacity is invalid");
    }
    rank_stackless_products_.allocate(
        maximum_product_count,
        "cudaMalloc Phase 9 stackless rank-prune product buffer");
    rank_stackless_terminals_.allocate(
        maximum_terminal_count,
        "cudaMalloc Phase 9 stackless rank-prune terminal buffer");
    rank_stackless_control_.allocate(
        1U, "cudaMalloc Phase 9 stackless rank-prune control");
    rank_stackless_product_capacity_ = maximum_product_count;
    rank_stackless_terminal_capacity_ = maximum_terminal_count;
  }

  void initialize_rank(
      std::size_t maximum_product_count,
      std::size_t maximum_work_item_count,
      std::size_t maximum_terminal_count) {
    if (rank_product_capacity_ != 0U) {
      if (rank_product_capacity_ != maximum_product_count ||
          rank_work_item_capacity_ != maximum_work_item_count ||
          rank_terminal_capacity_ != maximum_terminal_count) {
        throw std::logic_error(
            "a Phase 9 rank-prune context changed its fixed capacities");
      }
      return;
    }
    if (maximum_product_count == 0U ||
        maximum_work_item_count == 0U ||
        maximum_terminal_count == 0U) {
      throw std::invalid_argument(
          "a Phase 9 rank-prune capacity is empty");
    }
    rank_products_.allocate(
        maximum_product_count,
        "cudaMalloc Phase 9 rank-prune product buffer");
    rank_product_point_counts_.allocate(
        maximum_product_count,
        "cudaMalloc Phase 9 rank-prune product counters");
    rank_frontier_a_.allocate(
        maximum_work_item_count,
        "cudaMalloc Phase 9 rank-prune frontier A");
    rank_frontier_b_.allocate(
        maximum_work_item_count,
        "cudaMalloc Phase 9 rank-prune frontier B");
    rank_next_counts_.allocate(
        maximum_work_item_count,
        "cudaMalloc Phase 9 rank-prune next counts");
    rank_terminal_counts_.allocate(
        maximum_work_item_count,
        "cudaMalloc Phase 9 rank-prune terminal counts");
    rank_next_offsets_.allocate(
        maximum_work_item_count,
        "cudaMalloc Phase 9 rank-prune next offsets");
    rank_terminal_offsets_.allocate(
        maximum_work_item_count,
        "cudaMalloc Phase 9 rank-prune terminal offsets");
    rank_decisions_.allocate(
        maximum_work_item_count,
        "cudaMalloc Phase 9 rank-prune decisions");
    rank_terminals_.allocate(
        maximum_terminal_count,
        "cudaMalloc Phase 9 rank-prune terminals");
    rank_failure_.allocate(
        1U, "cudaMalloc Phase 9 rank-prune failure word");

    std::size_t next_scan_bytes = 0U;
    check_cuda(
        cub::DeviceScan::ExclusiveSum(
            nullptr,
            next_scan_bytes,
            rank_next_counts_.get(),
            rank_next_offsets_.get(),
            maximum_work_item_count,
            stream_),
        "CUB Phase 9 rank-prune next-count scan sizing");
    std::size_t terminal_scan_bytes = 0U;
    check_cuda(
        cub::DeviceScan::ExclusiveSum(
            nullptr,
            terminal_scan_bytes,
            rank_terminal_counts_.get(),
            rank_terminal_offsets_.get(),
            maximum_work_item_count,
            stream_),
        "CUB Phase 9 rank-prune terminal-count scan sizing");
    rank_scan_workspace_byte_capacity_ =
        std::max<std::size_t>(
            1U, std::max(next_scan_bytes, terminal_scan_bytes));
    rank_scan_workspace_.allocate(
        rank_scan_workspace_byte_capacity_,
        "cudaMalloc Phase 9 rank-prune scan workspace");
    rank_product_capacity_ = maximum_product_count;
    rank_work_item_capacity_ = maximum_work_item_count;
    rank_terminal_capacity_ = maximum_terminal_count;
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
  [[nodiscard]] PairSupportRankProductInputRecord* rank_products() noexcept {
    return rank_products_.get();
  }
  [[nodiscard]] const std::uint32_t* rank_escape_node_indices()
      const noexcept {
    return rank_escape_node_indices_.get();
  }
  [[nodiscard]] PairSupportRankProductInputRecord*
  rank_stackless_products() noexcept {
    return rank_stackless_products_.get();
  }
  [[nodiscard]] PairSupportRankDeviceTerminal*
  rank_stackless_terminals() noexcept {
    return rank_stackless_terminals_.get();
  }
  [[nodiscard]] PairSupportRankStacklessControl*
  rank_stackless_control() noexcept {
    return rank_stackless_control_.get();
  }
  [[nodiscard]] std::uint64_t* rank_product_point_counts() noexcept {
    return rank_product_point_counts_.get();
  }
  [[nodiscard]] PairSupportRankWorkItem* rank_frontier_a() noexcept {
    return rank_frontier_a_.get();
  }
  [[nodiscard]] PairSupportRankWorkItem* rank_frontier_b() noexcept {
    return rank_frontier_b_.get();
  }
  [[nodiscard]] std::uint64_t* rank_next_counts() noexcept {
    return rank_next_counts_.get();
  }
  [[nodiscard]] std::uint64_t* rank_terminal_counts() noexcept {
    return rank_terminal_counts_.get();
  }
  [[nodiscard]] std::uint64_t* rank_next_offsets() noexcept {
    return rank_next_offsets_.get();
  }
  [[nodiscard]] std::uint64_t* rank_terminal_offsets() noexcept {
    return rank_terminal_offsets_.get();
  }
  [[nodiscard]] PairSupportRankDeviceDecision* rank_decisions() noexcept {
    return rank_decisions_.get();
  }
  [[nodiscard]] PairSupportRankDeviceTerminal* rank_terminals() noexcept {
    return rank_terminals_.get();
  }
  [[nodiscard]] std::uint64_t* rank_failure() noexcept {
    return rank_failure_.get();
  }
  [[nodiscard]] unsigned char* rank_scan_workspace() noexcept {
    return rank_scan_workspace_.get();
  }
  [[nodiscard]] std::size_t rank_scan_workspace_byte_capacity()
      const noexcept {
    return rank_scan_workspace_byte_capacity_;
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
  void abandon_all() noexcept {
    rank_stackless_control_.abandon();
    rank_stackless_terminals_.abandon();
    rank_stackless_products_.abandon();
    rank_escape_node_indices_.abandon();
    rank_scan_workspace_.abandon();
    rank_failure_.abandon();
    rank_terminals_.abandon();
    rank_decisions_.abandon();
    rank_terminal_offsets_.abandon();
    rank_next_offsets_.abandon();
    rank_terminal_counts_.abandon();
    rank_next_counts_.abandon();
    rank_frontier_b_.abandon();
    rank_frontier_a_.abandon();
    rank_product_point_counts_.abandon();
    rank_products_.abandon();
    records_.abandon();
    queries_.abandon();
    nodes_.abandon();
  }

  void reset_all() noexcept {
    rank_stackless_control_.reset();
    rank_stackless_terminals_.reset();
    rank_stackless_products_.reset();
    rank_escape_node_indices_.reset();
    rank_scan_workspace_.reset();
    rank_failure_.reset();
    rank_terminals_.reset();
    rank_decisions_.reset();
    rank_terminal_offsets_.reset();
    rank_next_offsets_.reset();
    rank_terminal_counts_.reset();
    rank_next_counts_.reset();
    rank_frontier_b_.reset();
    rank_frontier_a_.reset();
    rank_product_point_counts_.reset();
    rank_products_.reset();
    records_.reset();
    queries_.reset();
    nodes_.reset();
  }

  int device_{-1};
  cudaStream_t stream_{nullptr};
  unsigned int maximum_grid_x_{};
  std::size_t node_count_{};
  std::size_t maximum_query_count_{};
  std::size_t rank_product_capacity_{};
  std::size_t rank_work_item_capacity_{};
  std::size_t rank_terminal_capacity_{};
  std::size_t rank_scan_workspace_byte_capacity_{};
  std::size_t rank_escape_node_count_{};
  std::size_t rank_stackless_product_capacity_{};
  std::size_t rank_stackless_terminal_capacity_{};
  DeviceBuffer<PairSupportPhiNodeInputRecord> nodes_;
  DeviceBuffer<PairSupportPhiQueryInputRecord> queries_;
  DeviceBuffer<PairSupportPhiDeviceRecord> records_;
  DeviceBuffer<std::uint32_t> rank_escape_node_indices_;
  DeviceBuffer<PairSupportRankProductInputRecord> rank_stackless_products_;
  DeviceBuffer<PairSupportRankDeviceTerminal> rank_stackless_terminals_;
  DeviceBuffer<PairSupportRankStacklessControl> rank_stackless_control_;
  DeviceBuffer<PairSupportRankProductInputRecord> rank_products_;
  DeviceBuffer<std::uint64_t> rank_product_point_counts_;
  DeviceBuffer<PairSupportRankWorkItem> rank_frontier_a_;
  DeviceBuffer<PairSupportRankWorkItem> rank_frontier_b_;
  DeviceBuffer<std::uint64_t> rank_next_counts_;
  DeviceBuffer<std::uint64_t> rank_terminal_counts_;
  DeviceBuffer<std::uint64_t> rank_next_offsets_;
  DeviceBuffer<std::uint64_t> rank_terminal_offsets_;
  DeviceBuffer<PairSupportRankDeviceDecision> rank_decisions_;
  DeviceBuffer<PairSupportRankDeviceTerminal> rank_terminals_;
  DeviceBuffer<std::uint64_t> rank_failure_;
  DeviceBuffer<unsigned char> rank_scan_workspace_;
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

[[nodiscard]] inline __device__ bool singleton_anchor_leaf(
    const PairSupportPhiNodeInputRecord& anchor,
    std::uint64_t expected_leaf_begin) noexcept {
  if (anchor.leaf_begin != expected_leaf_begin ||
      anchor.leaf_begin >= anchor.leaf_end ||
      anchor.leaf_end - anchor.leaf_begin != 1U ||
      anchor.left_child != pair_support_phi_sentinel ||
      anchor.right_child != pair_support_phi_sentinel) {
    return false;
  }
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    if (anchor.lower_bits[axis] != anchor.upper_bits[axis] ||
        !device::point_interval(anchor.lower_bits[axis]).valid) {
      return false;
    }
  }
  return true;
}

// Necessary-condition cull only.  Losing the shared x dependency makes the
// interval lower bound weaker, never optimistic: if it is still nonnegative,
// no point in witness can be strictly inside the diametral ball of the two
// authenticated anchor leaves.  Such a point therefore cannot satisfy the
// stronger all-support-pairs predicate.  P4 records this closure as a
// non-strict terminal; only strict-phi terminals increment the rank count.
[[nodiscard]] inline __device__ bool anchor_phi_lower_bound(
    const PairSupportPhiNodeInputRecord& first_anchor,
    const PairSupportPhiNodeInputRecord& second_anchor,
    const PairSupportPhiNodeInputRecord& witness,
    double& lower) noexcept {
  using device::DeviceInterval;
  DeviceInterval sum = device::point_interval(UINT64_C(0));
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    const DeviceInterval witness_lower =
        device::point_interval(witness.lower_bits[axis]);
    const DeviceInterval witness_upper =
        device::point_interval(witness.upper_bits[axis]);
    if (!witness_lower.valid || !witness_upper.valid) {
      return false;
    }
    const DeviceInterval x = device::checked_interval(
        witness_lower.lower, witness_upper.upper);
    const DeviceInterval u =
        device::point_interval(first_anchor.lower_bits[axis]);
    const DeviceInterval v =
        device::point_interval(second_anchor.lower_bits[axis]);
    const DeviceInterval candidate = device::multiply_intervals(
        device::subtract_intervals(x, u),
        device::subtract_intervals(x, v));
    sum = device::add_intervals(sum, candidate);
    if (!sum.valid) {
      return false;
    }
  }
  lower = sum.lower;
  return device::is_finite(lower);
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

[[nodiscard]] inline __device__ bool rank_ranges_intersect(
    const PairSupportPhiNodeInputRecord& left,
    const PairSupportPhiNodeInputRecord& right) noexcept {
  return left.leaf_begin < right.leaf_end &&
         right.leaf_begin < left.leaf_end;
}

[[nodiscard]] inline __device__ bool rank_range_contained_in(
    const PairSupportPhiNodeInputRecord& candidate,
    const PairSupportPhiNodeInputRecord& container) noexcept {
  return container.leaf_begin <= candidate.leaf_begin &&
         candidate.leaf_end <= container.leaf_end;
}

inline __device__ void record_stackless_rank_failure(
    PairSupportRankStacklessControl* control,
    std::uint64_t code) noexcept {
  control->failure_code = code;
  control->status = kRankStacklessFailure;
}

[[nodiscard]] inline __device__ bool append_stackless_rank_terminal(
    std::uint32_t witness_node_index,
    std::uint64_t terminal_capacity,
    PairSupportRankDeviceTerminal* terminals,
    PairSupportRankStacklessControl* control) noexcept {
  if (control->terminal_count >= terminal_capacity) {
    control->status = kRankStacklessTerminalCapacity;
    return false;
  }
  terminals[control->terminal_count] = PairSupportRankDeviceTerminal{
      0U, static_cast<std::uint64_t>(witness_node_index)};
  ++control->terminal_count;
  return true;
}

__global__ void morsehgp3d_phase9_pair_support_rank_stackless_kernel(
    const PairSupportPhiNodeInputRecord* nodes,
    std::size_t node_count,
    const std::uint32_t* escape_node_indices,
    std::uint32_t root_node_index,
    const PairSupportRankProductInputRecord* products,
    std::uint64_t required_strict_interior_point_count,
    std::uint64_t visit_budget,
    std::uint64_t terminal_capacity,
    PairSupportRankDeviceTerminal* terminals,
    PairSupportRankStacklessControl* control) {
  if (blockIdx.x != 0U || threadIdx.x != 0U) {
    return;
  }
  if (node_count == 0U || root_node_index >= node_count ||
      escape_node_indices[root_node_index] !=
          pair_support_rank_escape_sentinel ||
      required_strict_interior_point_count == 0U ||
      visit_budget == 0U || terminal_capacity == 0U) {
    record_stackless_rank_failure(control, 101U);
    return;
  }

  const PairSupportRankProductInputRecord product = products[0];
  if (product.first_support_node_index >= node_count ||
      product.second_support_node_index >= node_count ||
      product.first_anchor_leaf_node_index >= node_count ||
      product.second_anchor_leaf_node_index >= node_count) {
    record_stackless_rank_failure(control, 102U);
    return;
  }
  const PairSupportPhiNodeInputRecord first =
      nodes[product.first_support_node_index];
  const PairSupportPhiNodeInputRecord second =
      nodes[product.second_support_node_index];
  const PairSupportPhiNodeInputRecord first_anchor =
      nodes[product.first_anchor_leaf_node_index];
  const PairSupportPhiNodeInputRecord second_anchor =
      nodes[product.second_anchor_leaf_node_index];
  if (first.leaf_begin >= first.leaf_end ||
      second.leaf_begin >= second.leaf_end ||
      !singleton_anchor_leaf(first_anchor, first.leaf_begin) ||
      !singleton_anchor_leaf(second_anchor, second.leaf_begin)) {
    record_stackless_rank_failure(control, 103U);
    return;
  }

  std::uint32_t current = root_node_index;
  while (current != pair_support_rank_escape_sentinel) {
    if (control->visited_node_count >= visit_budget) {
      control->status = kRankStacklessVisitBudget;
      return;
    }
    if (current >= node_count) {
      record_stackless_rank_failure(control, 104U);
      return;
    }
    ++control->visited_node_count;

    const PairSupportPhiNodeInputRecord witness = nodes[current];
    const std::uint32_t escape = escape_node_indices[current];
    if (witness.leaf_begin >= witness.leaf_end ||
        (escape != pair_support_rank_escape_sentinel &&
         (escape >= node_count || escape <= current))) {
      record_stackless_rank_failure(control, 105U);
      return;
    }

    const bool left_missing =
        witness.left_child == pair_support_phi_sentinel;
    const bool right_missing =
        witness.right_child == pair_support_phi_sentinel;
    if (left_missing != right_missing ||
        (!left_missing &&
         (witness.left_child >= node_count ||
          witness.right_child >= node_count ||
          witness.left_child == witness.right_child))) {
      record_stackless_rank_failure(control, 106U);
      return;
    }
    if (!left_missing) {
      const std::uint32_t left =
          static_cast<std::uint32_t>(witness.left_child);
      const std::uint32_t right =
          static_cast<std::uint32_t>(witness.right_child);
      if (escape_node_indices[left] != right ||
          escape_node_indices[right] != escape) {
        record_stackless_rank_failure(control, 107U);
        return;
      }
    }

    const bool contained_in_support =
        rank_range_contained_in(witness, first) ||
        rank_range_contained_in(witness, second);
    if (contained_in_support) {
      current = escape;
      continue;
    }

    const bool overlaps =
        rank_ranges_intersect(witness, first) ||
        rank_ranges_intersect(witness, second);
    if (!overlaps) {
      double upper = 0.0;
      if (phi_upper_bound(first, second, witness, upper) &&
          upper < 0.0) {
        if (!append_stackless_rank_terminal(
                current, terminal_capacity, terminals, control)) {
          return;
        }
        const std::uint64_t point_count =
            witness.leaf_end - witness.leaf_begin;
        const std::uint64_t remaining =
            required_strict_interior_point_count -
            control->strict_interior_point_count;
        if (point_count >= remaining) {
          control->strict_interior_point_count =
              required_strict_interior_point_count;
          control->status = kRankStacklessPruneComplete;
          return;
        }
        control->strict_interior_point_count += point_count;
        current = escape;
        continue;
      }
      double anchor_lower = 0.0;
      if (product.first_support_node_index !=
              product.second_support_node_index &&
          anchor_phi_lower_bound(
              first_anchor, second_anchor, witness, anchor_lower) &&
          anchor_lower >= 0.0) {
        if (!append_stackless_rank_terminal(
                current, terminal_capacity, terminals, control)) {
          return;
        }
        current = escape;
        continue;
      }
    }

    if (left_missing) {
      if (overlaps) {
        record_stackless_rank_failure(control, 108U);
        return;
      }
      if (!append_stackless_rank_terminal(
              current, terminal_capacity, terminals, control)) {
        return;
      }
      current = escape;
      continue;
    }
    current = static_cast<std::uint32_t>(witness.left_child);
  }
  control->status = kRankStacklessKeepComplete;
}

inline __device__ void record_rank_failure(
    std::uint64_t* failure_word,
    std::uint64_t code) noexcept {
  atomicCAS(
      reinterpret_cast<unsigned long long*>(failure_word),
      0ULL,
      static_cast<unsigned long long>(code));
}

__global__ void morsehgp3d_phase9_pair_support_rank_count_kernel(
    const PairSupportPhiNodeInputRecord* nodes,
    std::size_t node_count,
    const PairSupportRankProductInputRecord* products,
    std::size_t product_count,
    const PairSupportRankWorkItem* current_frontier,
    std::size_t current_frontier_count,
    const std::uint64_t* product_point_counts,
    std::uint64_t required_strict_interior_point_count,
    std::uint64_t* next_counts,
    std::uint64_t* terminal_counts,
    PairSupportRankDeviceDecision* decisions,
    std::uint64_t* failure_word) {
  const std::size_t first_index =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  const std::size_t stride =
      static_cast<std::size_t>(gridDim.x) * blockDim.x;
  std::size_t item_index = first_index;
  while (item_index < current_frontier_count) {
    next_counts[item_index] = 0U;
    terminal_counts[item_index] = 0U;
    PairSupportRankDeviceDecision decision{};
    const PairSupportRankWorkItem item = current_frontier[item_index];
    if (item.product_slot >= product_count ||
        item.witness_node_index >= node_count) {
      record_rank_failure(failure_word, 1U);
      decisions[item_index] = decision;
      if (current_frontier_count - item_index <= stride) {
        break;
      }
      item_index += stride;
      continue;
    }
    const PairSupportRankProductInputRecord product =
        products[item.product_slot];
    if (product.first_support_node_index >= node_count ||
        product.second_support_node_index >= node_count ||
        product.first_anchor_leaf_node_index >= node_count ||
        product.second_anchor_leaf_node_index >= node_count) {
      record_rank_failure(failure_word, 2U);
      decisions[item_index] = decision;
      if (current_frontier_count - item_index <= stride) {
        break;
      }
      item_index += stride;
      continue;
    }
    const PairSupportPhiNodeInputRecord witness =
        nodes[item.witness_node_index];
    const PairSupportPhiNodeInputRecord first =
        nodes[product.first_support_node_index];
    const PairSupportPhiNodeInputRecord second =
        nodes[product.second_support_node_index];
    const PairSupportPhiNodeInputRecord first_anchor =
        nodes[product.first_anchor_leaf_node_index];
    const PairSupportPhiNodeInputRecord second_anchor =
        nodes[product.second_anchor_leaf_node_index];
    if (!singleton_anchor_leaf(first_anchor, first.leaf_begin) ||
        !singleton_anchor_leaf(second_anchor, second.leaf_begin)) {
      record_rank_failure(failure_word, 9U);
      decisions[item_index] = decision;
      if (current_frontier_count - item_index <= stride) {
        break;
      }
      item_index += stride;
      continue;
    }
    if (product_point_counts[item.product_slot] >=
        required_strict_interior_point_count) {
      decision.code = kRankDecisionDiscard;
      decisions[item_index] = decision;
      if (current_frontier_count - item_index <= stride) {
        break;
      }
      item_index += stride;
      continue;
    }
    if (witness.leaf_begin >= witness.leaf_end) {
      record_rank_failure(failure_word, 3U);
      decisions[item_index] = decision;
      if (current_frontier_count - item_index <= stride) {
        break;
      }
      item_index += stride;
      continue;
    }

    const bool contained_in_support =
        rank_range_contained_in(witness, first) ||
        rank_range_contained_in(witness, second);
    if (contained_in_support) {
      decision.code = kRankDecisionDiscard;
      decisions[item_index] = decision;
      if (current_frontier_count - item_index <= stride) {
        break;
      }
      item_index += stride;
      continue;
    }

    const bool overlaps =
        rank_ranges_intersect(witness, first) ||
        rank_ranges_intersect(witness, second);
    if (!overlaps) {
      double upper = 0.0;
      if (phi_upper_bound(first, second, witness, upper) && upper < 0.0) {
        decision.code = kRankDecisionStrictTerminal;
        terminal_counts[item_index] = 1U;
        decisions[item_index] = decision;
        if (current_frontier_count - item_index <= stride) {
          break;
        }
        item_index += stride;
        continue;
      }
      double anchor_lower = 0.0;
      if (product.first_support_node_index !=
              product.second_support_node_index &&
          anchor_phi_lower_bound(
              first_anchor, second_anchor, witness, anchor_lower) &&
          anchor_lower >= 0.0) {
        decision.code = kRankDecisionAnchorTerminal;
        terminal_counts[item_index] = 1U;
        decisions[item_index] = decision;
        if (current_frontier_count - item_index <= stride) {
          break;
        }
        item_index += stride;
        continue;
      }
    }

    const bool left_missing =
        witness.left_child == pair_support_phi_sentinel;
    const bool right_missing =
        witness.right_child == pair_support_phi_sentinel;
    if (left_missing != right_missing ||
        (!left_missing &&
         (witness.left_child >= node_count ||
          witness.right_child >= node_count ||
          witness.left_child == witness.right_child))) {
      record_rank_failure(failure_word, 4U);
    } else if (left_missing) {
      if (overlaps) {
        record_rank_failure(failure_word, 10U);
      } else {
        decision.code = kRankDecisionLeafTerminal;
        terminal_counts[item_index] = 1U;
      }
    } else {
      decision.code = kRankDecisionExpand;
      next_counts[item_index] = 2U;
    }
    decisions[item_index] = decision;
    if (current_frontier_count - item_index <= stride) {
      break;
    }
    item_index += stride;
  }
}

__global__ void morsehgp3d_phase9_pair_support_rank_emit_kernel(
    const PairSupportPhiNodeInputRecord* nodes,
    std::size_t node_count,
    const PairSupportRankWorkItem* current_frontier,
    std::size_t current_frontier_count,
    const std::uint64_t* next_offsets,
    const std::uint64_t* terminal_offsets,
    const PairSupportRankDeviceDecision* decisions,
    std::uint64_t next_frontier_capacity,
    std::uint64_t terminal_base,
    std::uint64_t terminal_capacity,
    PairSupportRankWorkItem* next_frontier,
    PairSupportRankDeviceTerminal* terminals,
    std::uint64_t* product_point_counts,
    std::uint64_t* failure_word) {
  const std::size_t first_index =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  const std::size_t stride =
      static_cast<std::size_t>(gridDim.x) * blockDim.x;
  std::size_t item_index = first_index;
  while (item_index < current_frontier_count) {
    const PairSupportRankWorkItem item = current_frontier[item_index];
    if (item.witness_node_index >= node_count) {
      record_rank_failure(failure_word, 5U);
    } else {
      const PairSupportPhiNodeInputRecord witness =
          nodes[item.witness_node_index];
      const PairSupportRankDeviceDecision decision =
          decisions[item_index];
      if (decision.code == kRankDecisionExpand) {
        const std::uint64_t output = next_offsets[item_index];
        if (output >= next_frontier_capacity ||
            output + 1U >= next_frontier_capacity) {
          record_rank_failure(failure_word, 6U);
        } else {
          next_frontier[output] = PairSupportRankWorkItem{
              item.product_slot, witness.left_child};
          next_frontier[output + 1U] = PairSupportRankWorkItem{
              item.product_slot, witness.right_child};
        }
      } else if (
          decision.code == kRankDecisionStrictTerminal ||
          decision.code == kRankDecisionAnchorTerminal ||
          decision.code == kRankDecisionLeafTerminal) {
        const std::uint64_t relative_output =
            terminal_offsets[item_index];
        if (terminal_base > terminal_capacity ||
            relative_output >= terminal_capacity - terminal_base) {
          record_rank_failure(failure_word, 7U);
        } else {
          const std::uint64_t output = terminal_base + relative_output;
          terminals[output] = PairSupportRankDeviceTerminal{
              item.product_slot, item.witness_node_index};
          if (decision.code == kRankDecisionStrictTerminal) {
            atomicAdd(
                reinterpret_cast<unsigned long long*>(
                    &product_point_counts[item.product_slot]),
                static_cast<unsigned long long>(
                    witness.leaf_end - witness.leaf_begin));
          }
        }
      } else if (decision.code != kRankDecisionDiscard) {
        record_rank_failure(failure_word, 8U);
      }
    }
    if (current_frontier_count - item_index <= stride) {
      break;
    }
    item_index += stride;
  }
}

[[nodiscard]] PairSupportRankDeviceBatch
propose_pair_support_rank_stackless_on_gpu(
    PairSupportPhiContextState& context,
    PairSupportPhiCudaResources& cuda,
    std::span<const PairSupportPhiNodeInputRecord> nodes,
    std::span<const std::uint32_t> escape_node_indices,
    std::uint64_t root_node_index,
    std::span<const PairSupportRankProductInputRecord> products,
    std::size_t required_strict_interior_point_count,
    std::size_t maximum_product_count,
    std::size_t maximum_work_item_count,
    std::size_t maximum_terminal_count,
    std::size_t visit_budget_count,
    bool snapshot_uploaded) {
  const std::size_t snapshot_bytes = checked_bytes(
      nodes.size(),
      sizeof(PairSupportPhiNodeInputRecord),
      "the Phase 9 stackless rank-prune snapshot bytes overflow");
  const std::size_t escape_snapshot_bytes = checked_bytes(
      escape_node_indices.size(),
      sizeof(std::uint32_t),
      "the Phase 9 stackless rank-prune escape bytes overflow");
  const std::size_t product_bytes = checked_bytes(
      products.size(),
      sizeof(PairSupportRankProductInputRecord),
      "the Phase 9 stackless rank-prune product bytes overflow");
  const std::size_t terminal_capacity_bytes = checked_bytes(
      maximum_terminal_count,
      sizeof(PairSupportRankDeviceTerminal),
      "the Phase 9 stackless rank-prune terminal capacity overflows");

  const bool escape_snapshot_uploaded =
      cuda.initialize_rank_escape_snapshot(escape_node_indices);
  cuda.initialize_rank_stackless(
      maximum_product_count, maximum_terminal_count);
  check_cuda(
      cudaMemcpyAsync(
          cuda.rank_stackless_products(),
          products.data(),
          product_bytes,
          cudaMemcpyHostToDevice,
          cuda.stream()),
      "cudaMemcpyAsync Phase 9 stackless rank-prune product "
      "host-to-device");
  check_cuda(
      cudaMemsetAsync(
          cuda.rank_stackless_control(),
          0,
          sizeof(PairSupportRankStacklessControl),
          cuda.stream()),
      "cudaMemsetAsync Phase 9 stackless rank-prune control");

  morsehgp3d_phase9_pair_support_rank_stackless_kernel
      <<<1U, 1U, 0U, cuda.stream()>>>(
          cuda.nodes(),
          nodes.size(),
          cuda.rank_escape_node_indices(),
          static_cast<std::uint32_t>(root_node_index),
          cuda.rank_stackless_products(),
          static_cast<std::uint64_t>(
              required_strict_interior_point_count),
          static_cast<std::uint64_t>(visit_budget_count),
          static_cast<std::uint64_t>(maximum_terminal_count),
          cuda.rank_stackless_terminals(),
          cuda.rank_stackless_control());
  check_cuda(
      cudaPeekAtLastError(),
      "Phase 9 stackless rank-prune kernel launch");

  PairSupportRankStacklessControl control;
  check_cuda(
      cudaMemcpyAsync(
          &control,
          cuda.rank_stackless_control(),
          sizeof(control),
          cudaMemcpyDeviceToHost,
          cuda.stream()),
      "cudaMemcpyAsync Phase 9 stackless rank-prune control "
      "device-to-host");
  cuda.synchronize();
  if (control.status == kRankStacklessFailure) {
    throw std::runtime_error(
        "the Phase 9 stackless rank-prune traversal failed closed with "
        "code " +
        std::to_string(control.failure_code));
  }
  if (control.status != kRankStacklessKeepComplete &&
      control.status != kRankStacklessPruneComplete &&
      control.status != kRankStacklessVisitBudget &&
      control.status != kRankStacklessTerminalCapacity) {
    throw std::runtime_error(
        "the Phase 9 stackless rank-prune traversal returned an invalid "
        "completion status");
  }
  if (control.visited_node_count > visit_budget_count ||
      control.terminal_count > maximum_terminal_count) {
    throw std::runtime_error(
        "the Phase 9 stackless rank-prune control exceeds its fixed "
        "capacity");
  }

  PairSupportRankDeviceBatch batch;
  batch.terminal_count =
      static_cast<std::size_t>(control.terminal_count);
  batch.input_product_count = products.size();
  batch.product_capacity = maximum_product_count;
  batch.work_item_capacity = maximum_work_item_count;
  batch.terminal_capacity = maximum_terminal_count;
  batch.traversal_epoch_count = 1U;
  batch.emit_kernel_launch_count = 1U;
  batch.host_synchronization_count = 1U;
  batch.visited_work_item_count =
      static_cast<std::size_t>(control.visited_node_count);
  batch.peak_frontier_count = 1U;
  batch.snapshot_h2d_byte_count =
      snapshot_uploaded ? snapshot_bytes : 0U;
  batch.escape_snapshot_h2d_byte_count =
      escape_snapshot_uploaded ? escape_snapshot_bytes : 0U;
  batch.active_product_h2d_byte_count = product_bytes;
  batch.initial_frontier_h2d_byte_count = 0U;
  batch.traversal_metadata_d2h_byte_count =
      sizeof(PairSupportRankStacklessControl);
  batch.active_terminal_d2h_byte_count = checked_bytes(
      batch.terminal_count,
      sizeof(PairSupportRankDeviceTerminal),
      "the Phase 9 stackless active terminal bytes overflow");
  batch.physical_terminal_d2h_byte_count =
      batch.active_terminal_d2h_byte_count;
  batch.device_frontier_double_buffer_byte_capacity = 0U;
  batch.device_escape_snapshot_byte_capacity =
      escape_snapshot_bytes;
  batch.device_terminal_byte_capacity = terminal_capacity_bytes;
  batch.device_scan_workspace_byte_capacity = 0U;
  batch.device_fixed_workspace_byte_capacity = checked_add_bytes(
      checked_bytes(
          maximum_product_count,
          sizeof(PairSupportRankProductInputRecord),
          "the Phase 9 stackless fixed product bytes overflow"),
      terminal_capacity_bytes,
      "the Phase 9 stackless fixed workspace bytes overflow");
  batch.device_fixed_workspace_byte_capacity = checked_add_bytes(
      batch.device_fixed_workspace_byte_capacity,
      sizeof(PairSupportRankStacklessControl),
      "the Phase 9 stackless fixed workspace bytes overflow");
  batch.visit_budget_count = visit_budget_count;
  batch.traversal_backend =
      PairSupportRankTraversalBackend::stackless_single_product;
  batch.capacity_stop =
      control.status == kRankStacklessTerminalCapacity
          ? PairSupportRankCapacityStop::receipt_capacity
          : PairSupportRankCapacityStop::none;
  batch.frontier_exhausted =
      control.status == kRankStacklessKeepComplete ||
      control.status == kRankStacklessPruneComplete;
  batch.visit_budget_exhausted =
      control.status == kRankStacklessVisitBudget;
  batch.anchor_ball_culling_enabled = true;
  batch.terminals.resize(batch.terminal_count);
  if (batch.active_terminal_d2h_byte_count != 0U) {
    check_cuda(
        cudaMemcpyAsync(
            batch.terminals.data(),
            cuda.rank_stackless_terminals(),
            batch.active_terminal_d2h_byte_count,
            cudaMemcpyDeviceToHost,
            cuda.stream()),
        "cudaMemcpyAsync Phase 9 stackless active terminals "
        "device-to-host");
    cuda.synchronize();
    ++batch.host_synchronization_count;
  }
  batch.buffer_epoch = context.advance_epoch();
  return batch;
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
    static_cast<void>(cuda.initialize_snapshot(nodes));
    cuda.initialize_phi(maximum_query_count);
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

PairSupportRankDeviceBatch propose_pair_support_rank_prunes_on_gpu(
    PairSupportPhiContextState& context,
    std::span<const PairSupportPhiNodeInputRecord> nodes,
    std::span<const std::uint32_t> escape_node_indices,
    std::uint64_t root_node_index,
    std::span<const PairSupportRankProductInputRecord> products,
    std::size_t required_strict_interior_point_count,
    std::size_t maximum_product_count,
    std::size_t maximum_work_item_count,
    std::size_t maximum_terminal_count,
    std::size_t maximum_epoch_count) {
  if (nodes.empty() || products.empty() ||
      root_node_index >= nodes.size() ||
      (!escape_node_indices.empty() &&
       escape_node_indices.size() != nodes.size()) ||
      required_strict_interior_point_count == 0U ||
      products.size() > maximum_product_count ||
      products.size() > maximum_work_item_count ||
      maximum_terminal_count == 0U || maximum_epoch_count == 0U) {
    throw std::invalid_argument(
        "invalid Phase 9 rank-prune launch extent");
  }
  const bool use_stackless_single_product =
      maximum_product_count == 1U && products.size() == 1U &&
      escape_node_indices.size() == nodes.size() &&
      nodes.size() <= pair_support_rank_escape_sentinel;
  std::size_t stackless_visit_budget_count = 0U;
  if (use_stackless_single_product) {
    const std::size_t multiplied_visit_budget =
        maximum_work_item_count >
                std::numeric_limits<std::size_t>::max() /
                    maximum_epoch_count
            ? std::numeric_limits<std::size_t>::max()
            : maximum_work_item_count * maximum_epoch_count;
    stackless_visit_budget_count = std::min(
        nodes.size(), multiplied_visit_budget);
  }
  const std::size_t snapshot_bytes = checked_bytes(
      nodes.size(),
      sizeof(PairSupportPhiNodeInputRecord),
      "the Phase 9 rank-prune snapshot byte count overflows");
  const std::size_t product_bytes = checked_bytes(
      products.size(),
      sizeof(PairSupportRankProductInputRecord),
      "the Phase 9 rank-prune product byte count overflows");
  const std::size_t terminal_capacity_bytes = checked_bytes(
      maximum_terminal_count,
      sizeof(PairSupportRankDeviceTerminal),
      "the Phase 9 rank-prune terminal byte capacity overflows");

  PairSupportPhiCudaResources& cuda = resources(context);
  DeviceGuard guard{cuda.device()};
  try {
    const bool snapshot_uploaded = cuda.initialize_snapshot(nodes);
    if (use_stackless_single_product) {
      PairSupportRankDeviceBatch batch =
          propose_pair_support_rank_stackless_on_gpu(
              context,
              cuda,
              nodes,
              escape_node_indices,
              root_node_index,
              products,
              required_strict_interior_point_count,
              maximum_product_count,
              maximum_work_item_count,
              maximum_terminal_count,
              stackless_visit_budget_count,
              snapshot_uploaded);
      guard.restore();
      return batch;
    }
    const std::size_t initial_frontier_bytes = checked_bytes(
        products.size(),
        sizeof(PairSupportRankWorkItem),
        "the Phase 9 rank-prune initial-frontier byte count overflows");
    const std::size_t one_frontier_capacity_bytes = checked_bytes(
        maximum_work_item_count,
        sizeof(PairSupportRankWorkItem),
        "the Phase 9 rank-prune frontier byte capacity overflows");
    if (one_frontier_capacity_bytes >
        std::numeric_limits<std::size_t>::max() / 2U) {
      throw std::length_error(
          "the Phase 9 rank-prune double-frontier byte capacity "
          "overflows");
    }
    cuda.initialize_rank(
        maximum_product_count,
        maximum_work_item_count,
        maximum_terminal_count);

    check_cuda(
        cudaMemcpyAsync(
            cuda.rank_products(),
            products.data(),
            product_bytes,
            cudaMemcpyHostToDevice,
            cuda.stream()),
        "cudaMemcpyAsync Phase 9 rank-prune products host-to-device");
    check_cuda(
        cudaMemsetAsync(
            cuda.rank_product_point_counts(),
            0,
            checked_bytes(
                maximum_product_count,
                sizeof(std::uint64_t),
                "the Phase 9 rank-prune point-counter bytes overflow"),
            cuda.stream()),
        "cudaMemsetAsync Phase 9 rank-prune product counters");
    check_cuda(
        cudaMemsetAsync(
            cuda.rank_failure(),
            0,
            sizeof(std::uint64_t),
            cuda.stream()),
        "cudaMemsetAsync Phase 9 rank-prune failure word");

    std::vector<PairSupportRankWorkItem> initial_frontier;
    initial_frontier.reserve(products.size());
    for (std::size_t product_slot = 0U;
         product_slot < products.size();
         ++product_slot) {
      initial_frontier.push_back(PairSupportRankWorkItem{
          static_cast<std::uint64_t>(product_slot), root_node_index});
    }
    check_cuda(
        cudaMemcpyAsync(
            cuda.rank_frontier_a(),
            initial_frontier.data(),
            initial_frontier_bytes,
            cudaMemcpyHostToDevice,
            cuda.stream()),
        "cudaMemcpyAsync Phase 9 rank-prune initial frontier host-to-device");

    PairSupportRankDeviceBatch batch;
    batch.input_product_count = products.size();
    batch.product_capacity = maximum_product_count;
    batch.work_item_capacity = maximum_work_item_count;
    batch.terminal_capacity = maximum_terminal_count;
    batch.snapshot_h2d_byte_count =
        snapshot_uploaded ? snapshot_bytes : 0U;
    batch.active_product_h2d_byte_count = product_bytes;
    batch.initial_frontier_h2d_byte_count = initial_frontier_bytes;
    batch.device_frontier_double_buffer_byte_capacity =
        2U * one_frontier_capacity_bytes;
    batch.device_terminal_byte_capacity = terminal_capacity_bytes;
    batch.device_scan_workspace_byte_capacity =
        cuda.rank_scan_workspace_byte_capacity();
    batch.anchor_ball_culling_enabled = true;
    const std::size_t fixed_product_bytes = checked_bytes(
        maximum_product_count,
        sizeof(PairSupportRankProductInputRecord) +
            sizeof(std::uint64_t),
        "the Phase 9 rank-prune fixed product bytes overflow");
    const std::size_t fixed_work_bytes = checked_bytes(
        maximum_work_item_count,
        2U * sizeof(PairSupportRankWorkItem) +
            6U * sizeof(std::uint64_t),
        "the Phase 9 rank-prune fixed work bytes overflow");
    batch.device_fixed_workspace_byte_capacity = checked_add_bytes(
        fixed_product_bytes,
        fixed_work_bytes,
        "the Phase 9 rank-prune fixed workspace bytes overflow");
    batch.device_fixed_workspace_byte_capacity = checked_add_bytes(
        batch.device_fixed_workspace_byte_capacity,
        terminal_capacity_bytes,
        "the Phase 9 rank-prune fixed workspace bytes overflow");
    batch.device_fixed_workspace_byte_capacity = checked_add_bytes(
        batch.device_fixed_workspace_byte_capacity,
        sizeof(std::uint64_t),
        "the Phase 9 rank-prune fixed workspace bytes overflow");
    batch.device_fixed_workspace_byte_capacity = checked_add_bytes(
        batch.device_fixed_workspace_byte_capacity,
        batch.device_scan_workspace_byte_capacity,
        "the Phase 9 rank-prune fixed workspace bytes overflow");

    PairSupportRankWorkItem* current_frontier =
        cuda.rank_frontier_a();
    PairSupportRankWorkItem* next_frontier =
        cuda.rank_frontier_b();
    std::size_t current_frontier_count = products.size();
    std::size_t terminal_count = 0U;
    while (current_frontier_count != 0U &&
           batch.traversal_epoch_count < maximum_epoch_count) {
      if (batch.visited_work_item_count >
          std::numeric_limits<std::size_t>::max() -
              current_frontier_count) {
        throw std::overflow_error(
            "the Phase 9 rank-prune visited-work counter overflowed");
      }
      batch.visited_work_item_count += current_frontier_count;
      batch.peak_frontier_count = std::max(
          batch.peak_frontier_count, current_frontier_count);
      ++batch.traversal_epoch_count;

      const std::size_t requested_blocks =
          (current_frontier_count + kThreadsPerBlock - 1U) /
          kThreadsPerBlock;
      const unsigned int blocks = static_cast<unsigned int>(std::min(
          requested_blocks,
          static_cast<std::size_t>(cuda.maximum_grid_x())));
      if (blocks == 0U) {
        throw std::runtime_error(
            "the Phase 9 rank-prune launch grid is empty");
      }
      morsehgp3d_phase9_pair_support_rank_count_kernel
          <<<blocks, kThreadsPerBlock, 0U, cuda.stream()>>>(
              cuda.nodes(),
              nodes.size(),
              cuda.rank_products(),
              products.size(),
              current_frontier,
              current_frontier_count,
              cuda.rank_product_point_counts(),
              static_cast<std::uint64_t>(
                  required_strict_interior_point_count),
              cuda.rank_next_counts(),
              cuda.rank_terminal_counts(),
              cuda.rank_decisions(),
              cuda.rank_failure());
      check_cuda(
          cudaPeekAtLastError(),
          "Phase 9 rank-prune count kernel launch");
      ++batch.count_kernel_launch_count;

      std::size_t scan_bytes =
          cuda.rank_scan_workspace_byte_capacity();
      check_cuda(
          cub::DeviceScan::ExclusiveSum(
              cuda.rank_scan_workspace(),
              scan_bytes,
              cuda.rank_next_counts(),
              cuda.rank_next_offsets(),
              current_frontier_count,
              cuda.stream()),
          "CUB Phase 9 rank-prune next-count exclusive scan");
      scan_bytes = cuda.rank_scan_workspace_byte_capacity();
      check_cuda(
          cub::DeviceScan::ExclusiveSum(
              cuda.rank_scan_workspace(),
              scan_bytes,
              cuda.rank_terminal_counts(),
              cuda.rank_terminal_offsets(),
              current_frontier_count,
              cuda.stream()),
          "CUB Phase 9 rank-prune terminal-count exclusive scan");
      batch.exclusive_scan_count += 2U;

      std::uint64_t last_next_offset = 0U;
      std::uint64_t last_next_count = 0U;
      std::uint64_t last_terminal_offset = 0U;
      std::uint64_t last_terminal_count = 0U;
      std::uint64_t failure_word = 0U;
      const std::size_t last = current_frontier_count - 1U;
      check_cuda(
          cudaMemcpyAsync(
              &last_next_offset,
              cuda.rank_next_offsets() + last,
              sizeof(last_next_offset),
              cudaMemcpyDeviceToHost,
              cuda.stream()),
          "cudaMemcpyAsync Phase 9 rank-prune last next offset");
      check_cuda(
          cudaMemcpyAsync(
              &last_next_count,
              cuda.rank_next_counts() + last,
              sizeof(last_next_count),
              cudaMemcpyDeviceToHost,
              cuda.stream()),
          "cudaMemcpyAsync Phase 9 rank-prune last next count");
      check_cuda(
          cudaMemcpyAsync(
              &last_terminal_offset,
              cuda.rank_terminal_offsets() + last,
              sizeof(last_terminal_offset),
              cudaMemcpyDeviceToHost,
              cuda.stream()),
          "cudaMemcpyAsync Phase 9 rank-prune last terminal offset");
      check_cuda(
          cudaMemcpyAsync(
              &last_terminal_count,
              cuda.rank_terminal_counts() + last,
              sizeof(last_terminal_count),
              cudaMemcpyDeviceToHost,
              cuda.stream()),
          "cudaMemcpyAsync Phase 9 rank-prune last terminal count");
      check_cuda(
          cudaMemcpyAsync(
              &failure_word,
              cuda.rank_failure(),
              sizeof(failure_word),
              cudaMemcpyDeviceToHost,
              cuda.stream()),
          "cudaMemcpyAsync Phase 9 rank-prune failure word");
      cuda.synchronize();
      if (failure_word != 0U) {
        throw std::runtime_error(
            "the Phase 9 rank-prune count/emit path failed closed with code " +
            std::to_string(failure_word));
      }
      if (last_next_offset >
              std::numeric_limits<std::uint64_t>::max() -
                  last_next_count ||
          last_terminal_offset >
              std::numeric_limits<std::uint64_t>::max() -
                  last_terminal_count) {
        throw std::runtime_error(
            "the Phase 9 rank-prune scan total overflowed uint64");
      }
      const std::uint64_t next_count_word =
          last_next_offset + last_next_count;
      const std::uint64_t epoch_terminal_count_word =
          last_terminal_offset + last_terminal_count;
      if (next_count_word > maximum_work_item_count) {
        batch.capacity_stop =
            PairSupportRankCapacityStop::work_item_capacity;
        break;
      }
      if (epoch_terminal_count_word >
          maximum_terminal_count - terminal_count) {
        batch.capacity_stop =
            PairSupportRankCapacityStop::receipt_capacity;
        break;
      }

      morsehgp3d_phase9_pair_support_rank_emit_kernel
          <<<blocks, kThreadsPerBlock, 0U, cuda.stream()>>>(
              cuda.nodes(),
              nodes.size(),
              current_frontier,
              current_frontier_count,
              cuda.rank_next_offsets(),
              cuda.rank_terminal_offsets(),
              cuda.rank_decisions(),
              static_cast<std::uint64_t>(maximum_work_item_count),
              static_cast<std::uint64_t>(terminal_count),
              static_cast<std::uint64_t>(maximum_terminal_count),
              next_frontier,
              cuda.rank_terminals(),
              cuda.rank_product_point_counts(),
              cuda.rank_failure());
      check_cuda(
          cudaPeekAtLastError(),
          "Phase 9 rank-prune emit kernel launch");
      ++batch.emit_kernel_launch_count;

      terminal_count +=
          static_cast<std::size_t>(epoch_terminal_count_word);
      current_frontier_count =
          static_cast<std::size_t>(next_count_word);
      std::swap(current_frontier, next_frontier);
    }
    batch.frontier_exhausted = current_frontier_count == 0U;
    batch.host_synchronization_count =
        batch.traversal_epoch_count + 1U;
    batch.terminal_count = terminal_count;
    batch.traversal_metadata_d2h_byte_count = checked_add_bytes(
        checked_bytes(
            batch.traversal_epoch_count,
            5U * sizeof(std::uint64_t),
            "the Phase 9 rank-prune metadata D2H bytes overflow"),
        sizeof(std::uint64_t),
        "the Phase 9 rank-prune metadata D2H bytes overflow");
    batch.active_terminal_d2h_byte_count = checked_bytes(
        terminal_count,
        sizeof(PairSupportRankDeviceTerminal),
        "the Phase 9 active rank-prune terminal bytes overflow");
    batch.physical_terminal_d2h_byte_count =
        batch.active_terminal_d2h_byte_count;
    batch.terminals.resize(terminal_count);

    std::uint64_t final_failure_word = 0U;
    if (batch.active_terminal_d2h_byte_count != 0U) {
      check_cuda(
          cudaMemcpyAsync(
              batch.terminals.data(),
              cuda.rank_terminals(),
              batch.active_terminal_d2h_byte_count,
              cudaMemcpyDeviceToHost,
              cuda.stream()),
          "cudaMemcpyAsync Phase 9 active rank-prune terminals "
          "device-to-host");
    }
    check_cuda(
        cudaMemcpyAsync(
            &final_failure_word,
            cuda.rank_failure(),
            sizeof(final_failure_word),
            cudaMemcpyDeviceToHost,
            cuda.stream()),
        "cudaMemcpyAsync Phase 9 rank-prune final failure word");
    cuda.synchronize();
    if (final_failure_word != 0U) {
      throw std::runtime_error(
          "the Phase 9 rank-prune emit path failed closed with code " +
          std::to_string(final_failure_word));
    }
    batch.buffer_epoch = context.advance_epoch();
    guard.restore();
    return batch;
  } catch (...) {
    cuda.synchronize_after_failure();
    throw;
  }
}

}  // namespace morsehgp3d::gpu::detail
