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

inline constexpr std::size_t
    phase14_facet_top_k_proposal_maximum_point_count = 10U;
inline constexpr std::uint64_t phase14_facet_top_k_proposal_sentinel =
    std::numeric_limits<std::uint64_t>::max();
[[nodiscard]] constexpr std::uint64_t
phase14_facet_top_k_proposal_mix_fingerprint_word(
    std::uint64_t hash,
    std::uint64_t word) noexcept {
  word ^= word >> 30U;
  word *= UINT64_C(0xbf58476d1ce4e5b9);
  word ^= word >> 27U;
  word *= UINT64_C(0x94d049bb133111eb);
  word ^= word >> 31U;
  hash ^= word + UINT64_C(0x9e3779b97f4a7c15) + (hash << 6U) +
          (hash >> 2U);
  return hash;
}

// This is the full-mask fingerprint of the canonical direct-sparse locator:
// point_count followed by the already strictly increasing PointIds.
[[nodiscard]] constexpr std::uint64_t
phase14_facet_top_k_proposal_key_fingerprint(
    std::size_t point_count,
    std::span<const std::uint64_t> point_ids) noexcept {
  if (point_count == 0U ||
      point_count > phase14_facet_top_k_proposal_maximum_point_count ||
      point_count > point_ids.size()) {
    return phase14_facet_top_k_proposal_sentinel;
  }
  std::uint64_t hash = phase14_facet_top_k_proposal_mix_fingerprint_word(
      UINT64_C(0x6a09e667f3bcc909),
      static_cast<std::uint64_t>(point_count));
  for (std::size_t point = 0U; point < point_count; ++point) {
    hash = phase14_facet_top_k_proposal_mix_fingerprint_word(
        hash, point_ids[point]);
  }
  return hash;
}

enum class Phase14FacetTopKProposalFailureCode : std::uint64_t {
  none = 0U,
  invalid_point_count = 1U,
  source_point_out_of_range = 2U,
  source_morton_position_out_of_range = 3U,
  source_morton_position_mismatch = 4U,
  neighbor_point_out_of_range = 5U,
  non_canonical_source_key = 6U,
  non_orderable_distance = 7U,
};

struct Phase14FacetTopKProposalQueryInputRecord {
  std::uint64_t query_index{phase14_facet_top_k_proposal_sentinel};
  std::uint64_t key_fingerprint{phase14_facet_top_k_proposal_sentinel};
  std::uint64_t point_count{};
  std::uint64_t center_bits[3]{};
  std::uint64_t
      point_ids[phase14_facet_top_k_proposal_maximum_point_count]{};
  std::uint64_t
      morton_positions[phase14_facet_top_k_proposal_maximum_point_count]{};
};
static_assert(
    std::is_standard_layout_v<Phase14FacetTopKProposalQueryInputRecord>);
static_assert(
    std::is_trivially_copyable_v<Phase14FacetTopKProposalQueryInputRecord>);
static_assert(
    sizeof(Phase14FacetTopKProposalQueryInputRecord) ==
    26U * sizeof(std::uint64_t));

struct Phase14FacetTopKProposalDeviceRecord {
  std::uint64_t query_index{phase14_facet_top_k_proposal_sentinel};
  std::uint64_t key_fingerprint{phase14_facet_top_k_proposal_sentinel};
  std::uint64_t buffer_epoch{phase14_facet_top_k_proposal_sentinel};
  std::uint64_t candidate_count{phase14_facet_top_k_proposal_sentinel};
  std::uint64_t
      inspected_neighbor_count{phase14_facet_top_k_proposal_sentinel};
  std::uint64_t floating_distance_evaluation_count{
      phase14_facet_top_k_proposal_sentinel};
  std::uint64_t
      floating_rejection_count{phase14_facet_top_k_proposal_sentinel};
  std::uint64_t failure_code{phase14_facet_top_k_proposal_sentinel};
  std::uint64_t
      candidates[phase14_facet_top_k_proposal_maximum_point_count]{
          phase14_facet_top_k_proposal_sentinel,
          phase14_facet_top_k_proposal_sentinel,
          phase14_facet_top_k_proposal_sentinel,
          phase14_facet_top_k_proposal_sentinel,
          phase14_facet_top_k_proposal_sentinel,
          phase14_facet_top_k_proposal_sentinel,
          phase14_facet_top_k_proposal_sentinel,
          phase14_facet_top_k_proposal_sentinel,
          phase14_facet_top_k_proposal_sentinel,
          phase14_facet_top_k_proposal_sentinel};
};
static_assert(
    std::is_standard_layout_v<Phase14FacetTopKProposalDeviceRecord>);
static_assert(
    std::is_trivially_copyable_v<Phase14FacetTopKProposalDeviceRecord>);
static_assert(
    sizeof(Phase14FacetTopKProposalDeviceRecord) ==
    18U * sizeof(std::uint64_t));

struct Phase14FacetTopKProposalDeviceBatch {
  // Only the active prefix is initialized and copied back.  This keeps memset
  // and D2H traffic O(D) rather than O(C).  The inactive suffix has no
  // authority and cannot be consumed by host validation.
  std::vector<Phase14FacetTopKProposalDeviceRecord> records;
  std::size_t record_count{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  std::uint64_t buffer_epoch{};
};

class Phase14FacetTopKProposalContextState final {
 public:
  Phase14FacetTopKProposalContextState() = default;
  ~Phase14FacetTopKProposalContextState() = default;

  Phase14FacetTopKProposalContextState(
      const Phase14FacetTopKProposalContextState&) = delete;
  Phase14FacetTopKProposalContextState& operator=(
      const Phase14FacetTopKProposalContextState&) = delete;

  template <typename Operation>
  decltype(auto) with_gpu_section(Operation&& operation) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (poisoned_.load(std::memory_order_acquire)) {
      throw std::runtime_error(
          "the Phase 14 facet top-k proposal context is poisoned by a prior "
          "GPU or validation failure");
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
          "the Phase 14 facet top-k proposal buffer epoch overflowed");
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
  std::uint64_t epoch_{};
};

[[nodiscard]] Phase14FacetTopKProposalDeviceBatch
propose_phase14_facet_top_k_candidates_on_gpu(
    Phase14FacetTopKProposalContextState& context,
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::span<const std::uint64_t> morton_point_ids,
    std::span<const Phase14FacetTopKProposalQueryInputRecord> queries,
    std::size_t maximum_query_count,
    std::size_t morton_window_radius);

}  // namespace morsehgp3d::gpu::detail
