#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>
#include <vector>

namespace morsehgp3d::gpu::detail {

inline constexpr std::size_t
    exact_pair_block_transactional_page_no_source_index =
        std::numeric_limits<std::size_t>::max();

// The six additive fields scanned by the future CUDA count/scan/emit path.
// Counts determine whether a source prefix fits the physical page; masses
// close the scientific partition independently of those storage counts.
struct ExactPairBlockTransactionalPageTotals {
  std::uint64_t successor_block_count{};
  std::uint64_t prune_receipt_count{};
  std::uint64_t terminal_pair_count{};
  std::uint64_t successor_unordered_pair_mass{};
  std::uint64_t pruned_unordered_pair_mass{};
  std::uint64_t terminal_unordered_pair_mass{};

  friend bool operator==(
      const ExactPairBlockTransactionalPageTotals&,
      const ExactPairBlockTransactionalPageTotals&) = default;
};

// One immutable source contributes exactly one native transition.  Its three
// output masses must partition source_unordered_pair_mass before any prefix is
// allowed to commit.
struct ExactPairBlockTransactionalPageContribution {
  std::uint64_t source_unordered_pair_mass{};
  ExactPairBlockTransactionalPageTotals outputs{};

  friend bool operator==(
      const ExactPairBlockTransactionalPageContribution&,
      const ExactPairBlockTransactionalPageContribution&) = default;
};

static_assert(
    std::is_standard_layout_v<ExactPairBlockTransactionalPageTotals>);
static_assert(
    std::is_trivially_copyable_v<ExactPairBlockTransactionalPageTotals>);
static_assert(
    std::is_standard_layout_v<ExactPairBlockTransactionalPageContribution>);
static_assert(
    std::is_trivially_copyable_v<
        ExactPairBlockTransactionalPageContribution>);

// Capacities are page-local remaining capacities.  A caller with an inactive
// successor prefix subtracts that prefix before invoking the planner.
struct ExactPairBlockTransactionalPageCapacity {
  std::uint64_t successor_block_count{};
  std::uint64_t prune_receipt_count{};
  std::uint64_t terminal_pair_count{};

  friend bool operator==(
      const ExactPairBlockTransactionalPageCapacity&,
      const ExactPairBlockTransactionalPageCapacity&) = default;
};

enum class ExactPairBlockTransactionalPagePlanStatus : std::uint8_t {
  empty_page,
  complete_prefix,
  partial_prefix,
  first_source_capacity_exhausted,
  invalid_source_mass_partition,
  arithmetic_overflow,
  exclusive_scan_storage_exhausted,
};

// Non-owning form used by the bounded host/fake scheduler seam and, later,
// by fixed CUDA page storage.  The caller owns the scan storage, so planning
// performs no allocation and its working set cannot grow with the cloud.
struct ExactPairBlockTransactionalPagePlanView {
  ExactPairBlockTransactionalPagePlanStatus status{
      ExactPairBlockTransactionalPagePlanStatus::empty_page};
  ExactPairBlockTransactionalPageCapacity capacity{};
  std::size_t source_count{};
  std::size_t committed_source_count{};
  std::size_t first_uncommitted_source_index{};
  std::size_t failure_source_index{
      exact_pair_block_transactional_page_no_source_index};
  std::uint64_t committed_source_unordered_pair_mass{};
  ExactPairBlockTransactionalPageTotals committed_totals{};
  std::span<const ExactPairBlockTransactionalPageTotals> exclusive_offsets;

  [[nodiscard]] bool can_commit() const noexcept {
    return committed_source_count != 0U &&
        (status ==
             ExactPairBlockTransactionalPagePlanStatus::complete_prefix ||
         status ==
             ExactPairBlockTransactionalPagePlanStatus::partial_prefix);
  }

  [[nodiscard]] bool complete_page() const noexcept {
    return status ==
               ExactPairBlockTransactionalPagePlanStatus::complete_prefix &&
        committed_source_count == source_count;
  }

  [[nodiscard]] bool no_commit() const noexcept {
    return committed_source_count == 0U;
  }
};

struct ExactPairBlockTransactionalPagePlan {
  ExactPairBlockTransactionalPagePlanStatus status{
      ExactPairBlockTransactionalPagePlanStatus::empty_page};
  ExactPairBlockTransactionalPageCapacity capacity{};
  std::size_t source_count{};
  std::size_t committed_source_count{};
  std::size_t first_uncommitted_source_index{};
  std::size_t failure_source_index{
      exact_pair_block_transactional_page_no_source_index};
  std::uint64_t committed_source_unordered_pair_mass{};
  ExactPairBlockTransactionalPageTotals committed_totals{};

  // Entry i is the exclusive offset of source i.  The final sentinel is the
  // total over every source.  A validation or overflow failure retains only
  // the zero sentinel so no partial scan can accidentally steer an emit.
  std::vector<ExactPairBlockTransactionalPageTotals> exclusive_offsets;

  [[nodiscard]] bool can_commit() const noexcept {
    return committed_source_count != 0U &&
        (status ==
             ExactPairBlockTransactionalPagePlanStatus::complete_prefix ||
         status ==
             ExactPairBlockTransactionalPagePlanStatus::partial_prefix);
  }

  [[nodiscard]] bool complete_page() const noexcept {
    return status ==
               ExactPairBlockTransactionalPagePlanStatus::complete_prefix &&
        committed_source_count == source_count;
  }

  [[nodiscard]] bool no_commit() const noexcept {
    return committed_source_count == 0U;
  }

  friend bool operator==(
      const ExactPairBlockTransactionalPagePlan&,
      const ExactPairBlockTransactionalPagePlan&) = default;
};

// Pure executable specification for the CUDA page finalizer.  It validates
// every source first, computes six checked exclusive scans, then selects the
// largest source prefix fitting all three physical capacities.
[[nodiscard]] ExactPairBlockTransactionalPagePlan
plan_exact_pair_block_transactional_page(
    std::span<const ExactPairBlockTransactionalPageContribution>
        contributions,
    ExactPairBlockTransactionalPageCapacity capacity);

// Allocation-free planner entry point.  exclusive_offset_storage must contain
// at least contributions.size() + 1 entries.  On scientific or arithmetic
// failure, only its zero sentinel is exposed; on insufficient storage, no
// source can commit and any available first entry is reset to zero.
[[nodiscard]] ExactPairBlockTransactionalPagePlanView
plan_exact_pair_block_transactional_page_into(
    std::span<const ExactPairBlockTransactionalPageContribution>
        contributions,
    ExactPairBlockTransactionalPageCapacity capacity,
    std::span<ExactPairBlockTransactionalPageTotals>
        exclusive_offset_storage) noexcept;

}  // namespace morsehgp3d::gpu::detail
