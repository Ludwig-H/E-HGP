#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <limits>
#include <mutex>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::detail {

struct SpatialProposalRecord {
  std::uint64_t point_id{0U};
  std::uint64_t squared_distance_bits{0U};
};
static_assert(sizeof(SpatialProposalRecord) == 2U * sizeof(std::uint64_t));
static_assert(std::is_trivially_copyable_v<SpatialProposalRecord>);

struct SpatialProposalBatch {
  std::vector<SpatialProposalRecord> records;
  std::uint64_t buffer_epoch{0U};
};

class SpatialReferenceContextState final {
 public:
  SpatialReferenceContextState() = default;
  ~SpatialReferenceContextState() = default;

  SpatialReferenceContextState(const SpatialReferenceContextState&) = delete;
  SpatialReferenceContextState& operator=(
      const SpatialReferenceContextState&) = delete;

  template <typename Operation>
  decltype(auto) with_gpu_section(Operation&& operation) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (poisoned_.load(std::memory_order_acquire)) {
      throw std::runtime_error(
          "the Phase 4 spatial reference context is poisoned by a prior GPU failure");
    }
    try {
      return std::forward<Operation>(operation)();
    } catch (...) {
      mark_poisoned();
      throw;
    }
  }

  void mark_poisoned() noexcept {
    poisoned_.store(true, std::memory_order_release);
  }

  // Resource and epoch access is valid only inside with_gpu_section().
  [[nodiscard]] std::shared_ptr<void>& cuda_resources() noexcept {
    return cuda_resources_;
  }

  [[nodiscard]] std::uint64_t advance_epoch() {
    if (epoch_ == std::numeric_limits<std::uint64_t>::max()) {
      throw std::overflow_error("the Phase 4 spatial buffer epoch overflowed");
    }
    return ++epoch_;
  }

 private:
  std::mutex mutex_;
  std::shared_ptr<void> cuda_resources_;
  std::atomic<bool> poisoned_{false};
  std::uint64_t epoch_{0U};
};

[[nodiscard]] SpatialProposalBatch propose_squared_distances_on_gpu(
    SpatialReferenceContextState& context,
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    const std::array<std::uint64_t, 3>& query_bits);

}  // namespace morsehgp3d::gpu::detail
