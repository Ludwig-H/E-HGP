#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::detail {

inline constexpr std::uint64_t pair_support_phi_strict_interior_code = 1U;
inline constexpr std::uint64_t pair_support_phi_requires_descent_code = 2U;
inline constexpr std::uint64_t pair_support_phi_invalid_code = 3U;
inline constexpr std::uint64_t pair_support_phi_sentinel =
    std::numeric_limits<std::uint64_t>::max();

struct PairSupportPhiNodeInputRecord {
  std::uint64_t lower_bits[3]{};
  std::uint64_t upper_bits[3]{};
  std::uint64_t leaf_begin{};
  std::uint64_t leaf_end{};
  std::uint64_t left_child{pair_support_phi_sentinel};
  std::uint64_t right_child{pair_support_phi_sentinel};
};
static_assert(std::is_standard_layout_v<PairSupportPhiNodeInputRecord>);
static_assert(std::is_trivially_copyable_v<PairSupportPhiNodeInputRecord>);
static_assert(
    sizeof(PairSupportPhiNodeInputRecord) == 10U * sizeof(std::uint64_t));

struct PairSupportPhiQueryInputRecord {
  std::uint64_t first_support_node_index{};
  std::uint64_t second_support_node_index{};
  std::uint64_t witness_node_index{};
};
static_assert(std::is_standard_layout_v<PairSupportPhiQueryInputRecord>);
static_assert(std::is_trivially_copyable_v<PairSupportPhiQueryInputRecord>);
static_assert(
    sizeof(PairSupportPhiQueryInputRecord) == 3U * sizeof(std::uint64_t));

struct PairSupportPhiDeviceRecord {
  std::uint64_t query_index{pair_support_phi_sentinel};
  std::uint64_t first_support_node_index{pair_support_phi_sentinel};
  std::uint64_t second_support_node_index{pair_support_phi_sentinel};
  std::uint64_t witness_node_index{pair_support_phi_sentinel};
  std::uint64_t upper_phi_bits{pair_support_phi_sentinel};
  std::uint64_t proposal_code{pair_support_phi_sentinel};
};
static_assert(std::is_standard_layout_v<PairSupportPhiDeviceRecord>);
static_assert(std::is_trivially_copyable_v<PairSupportPhiDeviceRecord>);
static_assert(
    sizeof(PairSupportPhiDeviceRecord) == 6U * sizeof(std::uint64_t));

struct PairSupportPhiDeviceBatch {
  // The whole fixed-capacity vector is copied back.  record_count identifies
  // the initialized prefix; every later slot must remain the all-ones sentinel.
  std::vector<PairSupportPhiDeviceRecord> records;
  std::size_t record_count{};
  std::size_t kernel_launch_count{};
  std::uint64_t buffer_epoch{};
};

struct PairSupportRankProductInputRecord {
  std::uint64_t first_support_node_index{};
  std::uint64_t second_support_node_index{};
  // Authenticated singleton leaves at the first Morton position of each
  // support range.  They define one necessary anchor-ball test only; they
  // never authorize a positive rank receipt.
  std::uint64_t first_anchor_leaf_node_index{};
  std::uint64_t second_anchor_leaf_node_index{};
};
static_assert(std::is_standard_layout_v<PairSupportRankProductInputRecord>);
static_assert(std::is_trivially_copyable_v<PairSupportRankProductInputRecord>);
static_assert(
    sizeof(PairSupportRankProductInputRecord) ==
    4U * sizeof(std::uint64_t));

struct PairSupportRankWorkItem {
  std::uint64_t product_slot{pair_support_phi_sentinel};
  std::uint64_t witness_node_index{pair_support_phi_sentinel};
};
static_assert(std::is_standard_layout_v<PairSupportRankWorkItem>);
static_assert(std::is_trivially_copyable_v<PairSupportRankWorkItem>);
static_assert(
    sizeof(PairSupportRankWorkItem) == 2U * sizeof(std::uint64_t));

struct PairSupportRankDeviceTerminal {
  std::uint64_t product_slot{pair_support_phi_sentinel};
  std::uint64_t witness_node_index{pair_support_phi_sentinel};
};
static_assert(std::is_standard_layout_v<PairSupportRankDeviceTerminal>);
static_assert(std::is_trivially_copyable_v<PairSupportRankDeviceTerminal>);
static_assert(
    sizeof(PairSupportRankDeviceTerminal) ==
    2U * sizeof(std::uint64_t));

enum class PairSupportRankCapacityStop : std::uint8_t {
  none,
  work_item_capacity,
  // Source-compatible name; this now denotes unified terminal capacity C.
  receipt_capacity,
};

struct PairSupportRankDeviceBatch {
  // Only the active terminal prefix is returned: terminals.size() must equal
  // terminal_count.  The device still reserves terminal_capacity records, but
  // no host decision may inspect or trust the inactive device tail.  Position
  // is the transcript index; ranges and classifications are reconstructed only
  // from the immutable CPU snapshot.
  std::vector<PairSupportRankDeviceTerminal> terminals;
  std::size_t terminal_count{};
  std::size_t input_product_count{};
  std::size_t product_capacity{};
  std::size_t work_item_capacity{};
  std::size_t terminal_capacity{};
  std::size_t traversal_epoch_count{};
  std::size_t count_kernel_launch_count{};
  std::size_t exclusive_scan_count{};
  std::size_t emit_kernel_launch_count{};
  std::size_t visited_work_item_count{};
  std::size_t peak_frontier_count{};
  std::size_t snapshot_h2d_byte_count{};
  std::size_t active_product_h2d_byte_count{};
  std::size_t initial_frontier_h2d_byte_count{};
  std::size_t traversal_metadata_d2h_byte_count{};
  std::size_t physical_terminal_d2h_byte_count{};
  std::size_t active_terminal_d2h_byte_count{};
  std::size_t device_frontier_double_buffer_byte_capacity{};
  std::size_t device_terminal_byte_capacity{};
  std::size_t device_scan_workspace_byte_capacity{};
  std::size_t device_fixed_workspace_byte_capacity{};
  std::uint64_t buffer_epoch{};
  PairSupportRankCapacityStop capacity_stop{
      PairSupportRankCapacityStop::none};
  bool frontier_exhausted{false};
  // Exact cull counts would require either another O(W) arena or a contended
  // global atomic.  This flag instead makes the enabled proposal policy
  // explicit without fabricating a counter.
  bool anchor_ball_culling_enabled{false};
};

class PairSupportPhiContextState final {
 public:
  PairSupportPhiContextState() = default;
  ~PairSupportPhiContextState() = default;

  PairSupportPhiContextState(const PairSupportPhiContextState&) = delete;
  PairSupportPhiContextState& operator=(
      const PairSupportPhiContextState&) = delete;

  template <typename Operation>
  decltype(auto) with_gpu_section(Operation&& operation) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (poisoned_.load(std::memory_order_acquire)) {
      throw std::runtime_error(
          "the Phase 9 pair-support phi context is poisoned by a prior "
          "GPU or recertification failure");
    }
    try {
      return std::forward<Operation>(operation)();
    } catch (...) {
      poisoned_.store(true, std::memory_order_release);
      throw;
    }
  }

  [[nodiscard]] std::shared_ptr<void>& cuda_resources() noexcept {
    return cuda_resources_;
  }

  [[nodiscard]] std::uint64_t advance_epoch() {
    if (epoch_ == std::numeric_limits<std::uint64_t>::max()) {
      throw std::overflow_error(
          "the Phase 9 pair-support phi buffer epoch overflowed");
    }
    return ++epoch_;
  }

 private:
  std::mutex mutex_;
  std::shared_ptr<void> cuda_resources_;
  std::atomic<bool> poisoned_{false};
  std::uint64_t epoch_{};
};

[[nodiscard]] PairSupportPhiDeviceBatch propose_pair_support_phi_on_gpu(
    PairSupportPhiContextState& context,
    std::span<const PairSupportPhiNodeInputRecord> nodes,
    std::span<const PairSupportPhiQueryInputRecord> queries,
    std::size_t maximum_query_count);

[[nodiscard]] PairSupportRankDeviceBatch
propose_pair_support_rank_prunes_on_gpu(
    PairSupportPhiContextState& context,
    std::span<const PairSupportPhiNodeInputRecord> nodes,
    std::uint64_t root_node_index,
    std::span<const PairSupportRankProductInputRecord> products,
    std::size_t required_strict_interior_point_count,
    std::size_t maximum_product_count,
    std::size_t maximum_work_item_count,
    std::size_t maximum_terminal_count,
    std::size_t maximum_epoch_count);

}  // namespace morsehgp3d::gpu::detail
