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
};
static_assert(std::is_standard_layout_v<PairSupportPhiNodeInputRecord>);
static_assert(std::is_trivially_copyable_v<PairSupportPhiNodeInputRecord>);
static_assert(
    sizeof(PairSupportPhiNodeInputRecord) == 8U * sizeof(std::uint64_t));

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

}  // namespace morsehgp3d::gpu::detail
