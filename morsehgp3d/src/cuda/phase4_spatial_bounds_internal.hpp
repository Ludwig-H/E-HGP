#pragma once

#include <array>
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

inline constexpr std::uint64_t spatial_bounds_prune_code = 0U;
inline constexpr std::uint64_t spatial_bounds_visit_code = 1U;
inline constexpr std::uint64_t spatial_bounds_unknown_code = 2U;
inline constexpr std::uint64_t spatial_bounds_sentinel_code =
    std::numeric_limits<std::uint64_t>::max();

struct SpatialBoundsInputRecord {
  std::uint64_t lower_bits[3]{};
  std::uint64_t upper_bits[3]{};
};
static_assert(std::is_trivially_copyable_v<SpatialBoundsInputRecord>);

struct SpatialBoundsProposalRecord {
  std::uint64_t box_index{spatial_bounds_sentinel_code};
  std::uint64_t lower_squared_distance_bits{spatial_bounds_sentinel_code};
  std::uint64_t upper_squared_distance_bits{spatial_bounds_sentinel_code};
  std::uint64_t decision_code{spatial_bounds_sentinel_code};
};
static_assert(sizeof(SpatialBoundsProposalRecord) == 4U * sizeof(std::uint64_t));
static_assert(std::is_trivially_copyable_v<SpatialBoundsProposalRecord>);

struct SpatialBoundsProposalBatch {
  std::vector<SpatialBoundsProposalRecord> records;
  std::uint64_t buffer_epoch{0U};
};

class SpatialBoundsContextState final {
 public:
  SpatialBoundsContextState() = default;
  ~SpatialBoundsContextState() = default;

  SpatialBoundsContextState(const SpatialBoundsContextState&) = delete;
  SpatialBoundsContextState& operator=(const SpatialBoundsContextState&) = delete;

  template <typename Operation>
  decltype(auto) with_gpu_section(Operation&& operation) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (poisoned_.load(std::memory_order_acquire)) {
      throw std::runtime_error(
          "the Phase 4 spatial-bounds context is poisoned by a prior GPU failure");
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
          "the Phase 4 spatial-bounds buffer epoch overflowed");
    }
    return ++epoch_;
  }

 private:
  std::mutex mutex_;
  std::shared_ptr<void> cuda_resources_;
  std::atomic<bool> poisoned_{false};
  std::uint64_t epoch_{0U};
};

[[nodiscard]] SpatialBoundsProposalBatch propose_strict_aabb_prunes_on_gpu(
    SpatialBoundsContextState& context,
    std::span<const SpatialBoundsInputRecord> boxes,
    const std::array<std::uint64_t, 3>& query_lower_bits,
    const std::array<std::uint64_t, 3>& query_upper_bits,
    std::uint64_t cutoff_lower_bits,
    std::uint64_t cutoff_upper_bits);

}  // namespace morsehgp3d::gpu::detail
