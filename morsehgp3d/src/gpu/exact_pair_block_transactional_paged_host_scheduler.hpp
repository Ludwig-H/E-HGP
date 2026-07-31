#pragma once

#include "exact_pair_block_transactional_page_plan.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace morsehgp3d::gpu::detail {

// This is deliberately independent of the point count.  A production driver
// may feed arbitrarily many pages, but one host/fake scheduling step can never
// retain or scan more than this fixed number of source transitions.
inline constexpr std::size_t
    exact_pair_block_transactional_host_page_maximum_source_count = 1024U;

enum class ExactPairBlockTransactionalPagedHostSchedulerStatus : std::uint8_t {
  empty_page,
  complete_page_published,
  prefix_published_suffix_retained,
  first_source_capacity_exhausted_suffix_retained,
  invalid_page_rejected,
  arithmetic_overflow_rejected,
  page_working_set_exhausted,
  retained_suffix_requires_retry,
};

// One receipt describes a single attempted page publication.  "Published"
// means committed to the scheduler ledger; merely scanned sources never alter
// cumulative totals.  For every valid page, page = published + retained for
// all three counts, all three masses and source mass.
struct ExactPairBlockTransactionalPagedHostSchedulerReceipt {
  ExactPairBlockTransactionalPagedHostSchedulerStatus status{
      ExactPairBlockTransactionalPagedHostSchedulerStatus::empty_page};
  ExactPairBlockTransactionalPagePlanStatus planner_status{
      ExactPairBlockTransactionalPagePlanStatus::empty_page};
  std::size_t submitted_source_count{};
  std::size_t page_source_count{};
  std::size_t published_source_count{};
  std::size_t retained_source_count{};
  std::uint64_t publication_epoch{};
  std::uint64_t page_source_unordered_pair_mass{};
  std::uint64_t published_source_unordered_pair_mass{};
  std::uint64_t retained_source_unordered_pair_mass{};
  ExactPairBlockTransactionalPageTotals page_totals{};
  ExactPairBlockTransactionalPageTotals published_totals{};
  ExactPairBlockTransactionalPageTotals retained_totals{};
  ExactPairBlockTransactionalPageTotals cumulative_published_totals{};
  std::uint64_t cumulative_published_source_unordered_pair_mass{};
  bool planner_exercised{false};
  bool atomic_prefix_published{false};
  bool suffix_retained{false};
  bool page_mass_closed{false};
  bool cumulative_mass_closed{true};

  [[nodiscard]] bool published() const noexcept {
    return atomic_prefix_published && published_source_count != 0U;
  }

  friend bool operator==(
      const ExactPairBlockTransactionalPagedHostSchedulerReceipt&,
      const ExactPairBlockTransactionalPagedHostSchedulerReceipt&) = default;
};

// Allocation-free executable seam between count/scan/emit planning and the
// resident host/fake contract.  It publishes only the largest valid prefix,
// compacts the exact suffix into fixed storage, and refuses a new page until
// that suffix has been retried.  No member or operation is sized from n.
class ExactPairBlockTransactionalPagedHostScheduler final {
 public:
  static constexpr std::size_t maximum_page_source_count =
      exact_pair_block_transactional_host_page_maximum_source_count;

  [[nodiscard]] ExactPairBlockTransactionalPagedHostSchedulerReceipt
  consume_page(
      std::span<const ExactPairBlockTransactionalPageContribution>
          contributions,
      ExactPairBlockTransactionalPageCapacity capacity) noexcept;

  [[nodiscard]] ExactPairBlockTransactionalPagedHostSchedulerReceipt
  retry_retained_suffix(
      ExactPairBlockTransactionalPageCapacity capacity) noexcept;

  [[nodiscard]] std::span<
      const ExactPairBlockTransactionalPageContribution>
  retained_contributions() const noexcept {
    return std::span{retained_}.first(retained_source_count_);
  }

  [[nodiscard]] const ExactPairBlockTransactionalPageTotals&
  cumulative_published_totals() const noexcept {
    return cumulative_published_totals_;
  }

  [[nodiscard]] std::uint64_t
  cumulative_published_source_unordered_pair_mass() const noexcept {
    return cumulative_published_source_unordered_pair_mass_;
  }

  [[nodiscard]] std::uint64_t publication_epoch() const noexcept {
    return publication_epoch_;
  }

  [[nodiscard]] bool has_retained_suffix() const noexcept {
    return retained_source_count_ != 0U;
  }

  [[nodiscard]] static constexpr std::size_t bounded_working_set_byte_count()
      noexcept {
    return sizeof(ExactPairBlockTransactionalPagedHostScheduler);
  }

 private:
  [[nodiscard]] ExactPairBlockTransactionalPagedHostSchedulerReceipt
  publish_staged(
      std::size_t submitted_source_count,
      ExactPairBlockTransactionalPageCapacity capacity) noexcept;

  std::array<
      ExactPairBlockTransactionalPageContribution,
      maximum_page_source_count>
      retained_{};
  std::array<
      ExactPairBlockTransactionalPageTotals,
      maximum_page_source_count + 1U>
      exclusive_offset_storage_{};
  std::size_t retained_source_count_{};
  ExactPairBlockTransactionalPageTotals cumulative_published_totals_{};
  std::uint64_t cumulative_published_source_unordered_pair_mass_{};
  std::uint64_t publication_epoch_{};
};

}  // namespace morsehgp3d::gpu::detail
