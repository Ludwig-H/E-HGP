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

inline constexpr std::uint64_t kHPolytopeProposalInitializedSlotSentinel =
    UINT64_C(0x4850475033445631);
inline constexpr std::uint64_t kHPolytopeProposalNoBoundaryWitness =
    std::numeric_limits<std::uint64_t>::max();

enum class HPolytopeProposalDeviceCellStatus : std::uint64_t {
  validated_exhaustive_transcript = 1U,
  exact_fallback_interval_unknown = 2U,
  exact_fallback_capacity_exhausted = 3U,
  exact_fallback_unsupported_projection = 4U,
};

// Zero is reserved for an untouched physical-capacity tail slot.  Initialized
// records may use only the three proposal statuses below.
enum class HPolytopeProposalDeviceRecordStatus : std::uint64_t {
  unknown_requires_cpu_exact = 1U,
  proposed_strict_reject = 2U,
  proposed_survivor = 3U,
};

struct HPolytopeProposalInputCell {
  std::uint64_t cell_id{0U};
  std::uint64_t boundary_begin{0U};
  std::uint64_t boundary_end{0U};
  std::uint64_t unsupported_projection{0U};
  // Exact constant classification can prove the cell empty without yielding a
  // boundary plane.  Such a fact is intentionally not reconstructed from the
  // binary64 boundary transcript: the device must return an empty-row interval
  // fallback and the host keeps the independent exact result.
  std::uint64_t force_interval_fallback{0U};
};
static_assert(std::is_trivially_copyable_v<HPolytopeProposalInputCell>);

struct HPolytopeProposalInputBoundary {
  std::array<std::uint64_t, 4> coefficient_lower_bits{};
  std::array<std::uint64_t, 4> coefficient_upper_bits{};
};
static_assert(std::is_trivially_copyable_v<HPolytopeProposalInputBoundary>);

struct HPolytopeProposalDeviceRecord {
  // A default-initialized record is the required all-zero tail sentinel.
  std::uint64_t initialized_slot_sentinel{0U};
  std::uint64_t buffer_epoch{0U};
  std::uint64_t cell_id{0U};
  std::uint64_t first_boundary_index{0U};
  std::uint64_t second_boundary_index{0U};
  std::uint64_t third_boundary_index{0U};
  std::uint64_t status_code{0U};
  std::uint64_t strict_reject_boundary_witness{0U};
  std::uint64_t could_be_active_boundary_mask{0U};
  std::array<std::uint64_t, 3> coordinate_lower_bits{};
  std::array<std::uint64_t, 3> coordinate_upper_bits{};
};
static_assert(std::is_trivially_copyable_v<HPolytopeProposalDeviceRecord>);

struct HPolytopeProposalDeviceBatch {
  std::vector<std::uint64_t> cell_ids;
  std::vector<HPolytopeProposalDeviceCellStatus> cell_statuses;
  std::vector<std::uint64_t> cell_record_offsets;
  // The vector owns the entire physical capacity.  Only the prefix ending at
  // record_count is initialized; every later record must remain all-zero.
  std::vector<HPolytopeProposalDeviceRecord> records;
  std::uint64_t record_count{0U};
  std::uint64_t buffer_epoch{0U};
};

class HPolytopeProposalContextState final {
 public:
  HPolytopeProposalContextState() = default;
  ~HPolytopeProposalContextState() = default;

  HPolytopeProposalContextState(const HPolytopeProposalContextState&) =
      delete;
  HPolytopeProposalContextState& operator=(
      const HPolytopeProposalContextState&) = delete;

  template <typename Operation>
  decltype(auto) with_gpu_section(Operation&& operation) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (poisoned_.load(std::memory_order_acquire)) {
      throw std::runtime_error(
          "the Phase 7 H-polytope proposal context is poisoned by a prior "
          "GPU failure");
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
      throw std::overflow_error(
          "the Phase 7 H-polytope proposal buffer epoch overflowed");
    }
    return ++epoch_;
  }

  // Valid only while the caller owns with_gpu_section().
  [[nodiscard]] std::uint64_t current_epoch() const noexcept {
    return epoch_;
  }

 private:
  std::mutex mutex_;
  std::shared_ptr<void> cuda_resources_;
  std::atomic<bool> poisoned_{false};
  std::uint64_t epoch_{0U};
};

[[nodiscard]] HPolytopeProposalDeviceBatch
propose_h_polytope_transcript_on_gpu(
    HPolytopeProposalContextState& context,
    std::span<const HPolytopeProposalInputCell> cells,
    std::span<const HPolytopeProposalInputBoundary> boundaries,
    std::size_t maximum_total_proposal_record_count);

}  // namespace morsehgp3d::gpu::detail
