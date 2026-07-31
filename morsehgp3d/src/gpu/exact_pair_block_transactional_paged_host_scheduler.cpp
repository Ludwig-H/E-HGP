#include "exact_pair_block_transactional_paged_host_scheduler.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace morsehgp3d::gpu::detail {
namespace {

[[nodiscard]] bool checked_add(
    std::uint64_t left,
    std::uint64_t right,
    std::uint64_t& sum) noexcept {
  if (right > std::numeric_limits<std::uint64_t>::max() - left) {
    return false;
  }
  sum = left + right;
  return true;
}

[[nodiscard]] bool checked_add_totals(
    const ExactPairBlockTransactionalPageTotals& left,
    const ExactPairBlockTransactionalPageTotals& right,
    ExactPairBlockTransactionalPageTotals& sum) noexcept {
  return checked_add(
             left.successor_block_count,
             right.successor_block_count,
             sum.successor_block_count) &&
      checked_add(
             left.prune_receipt_count,
             right.prune_receipt_count,
             sum.prune_receipt_count) &&
      checked_add(
             left.terminal_pair_count,
             right.terminal_pair_count,
             sum.terminal_pair_count) &&
      checked_add(
             left.successor_unordered_pair_mass,
             right.successor_unordered_pair_mass,
             sum.successor_unordered_pair_mass) &&
      checked_add(
             left.pruned_unordered_pair_mass,
             right.pruned_unordered_pair_mass,
             sum.pruned_unordered_pair_mass) &&
      checked_add(
             left.terminal_unordered_pair_mass,
             right.terminal_unordered_pair_mass,
             sum.terminal_unordered_pair_mass);
}

[[nodiscard]] bool resolved_mass(
    const ExactPairBlockTransactionalPageTotals& totals,
    std::uint64_t& mass) noexcept {
  std::uint64_t partial = 0U;
  return checked_add(
             totals.successor_unordered_pair_mass,
             totals.pruned_unordered_pair_mass,
             partial) &&
      checked_add(
             partial,
             totals.terminal_unordered_pair_mass,
             mass);
}

[[nodiscard]] ExactPairBlockTransactionalPageTotals subtract_totals(
    const ExactPairBlockTransactionalPageTotals& total,
    const ExactPairBlockTransactionalPageTotals& prefix) noexcept {
  return {
      total.successor_block_count - prefix.successor_block_count,
      total.prune_receipt_count - prefix.prune_receipt_count,
      total.terminal_pair_count - prefix.terminal_pair_count,
      total.successor_unordered_pair_mass -
          prefix.successor_unordered_pair_mass,
      total.pruned_unordered_pair_mass - prefix.pruned_unordered_pair_mass,
      total.terminal_unordered_pair_mass -
          prefix.terminal_unordered_pair_mass};
}

[[nodiscard]] bool totals_partition(
    const ExactPairBlockTransactionalPageTotals& total,
    const ExactPairBlockTransactionalPageTotals& prefix,
    const ExactPairBlockTransactionalPageTotals& suffix) noexcept {
  ExactPairBlockTransactionalPageTotals recomposed{};
  return checked_add_totals(prefix, suffix, recomposed) && recomposed == total;
}

}  // namespace

ExactPairBlockTransactionalPagedHostSchedulerReceipt
ExactPairBlockTransactionalPagedHostScheduler::consume_page(
    std::span<const ExactPairBlockTransactionalPageContribution>
        contributions,
    ExactPairBlockTransactionalPageCapacity capacity) noexcept {
  if (retained_source_count_ != 0U) {
    ExactPairBlockTransactionalPagedHostSchedulerReceipt receipt;
    receipt.status = ExactPairBlockTransactionalPagedHostSchedulerStatus::
        retained_suffix_requires_retry;
    receipt.submitted_source_count = contributions.size();
    receipt.retained_source_count = retained_source_count_;
    receipt.publication_epoch = publication_epoch_;
    receipt.cumulative_published_totals = cumulative_published_totals_;
    receipt.cumulative_published_source_unordered_pair_mass =
        cumulative_published_source_unordered_pair_mass_;
    receipt.suffix_retained = true;
    std::uint64_t cumulative_mass = 0U;
    receipt.cumulative_mass_closed =
        resolved_mass(cumulative_published_totals_, cumulative_mass) &&
        cumulative_mass ==
            cumulative_published_source_unordered_pair_mass_;
    return receipt;
  }
  if (contributions.size() > maximum_page_source_count) {
    ExactPairBlockTransactionalPagedHostSchedulerReceipt receipt;
    receipt.status = ExactPairBlockTransactionalPagedHostSchedulerStatus::
        page_working_set_exhausted;
    receipt.submitted_source_count = contributions.size();
    receipt.publication_epoch = publication_epoch_;
    receipt.cumulative_published_totals = cumulative_published_totals_;
    receipt.cumulative_published_source_unordered_pair_mass =
        cumulative_published_source_unordered_pair_mass_;
    std::uint64_t cumulative_mass = 0U;
    receipt.cumulative_mass_closed =
        resolved_mass(cumulative_published_totals_, cumulative_mass) &&
        cumulative_mass ==
            cumulative_published_source_unordered_pair_mass_;
    return receipt;
  }

  std::copy(
      contributions.begin(), contributions.end(), retained_.begin());
  retained_source_count_ = contributions.size();
  return publish_staged(contributions.size(), capacity);
}

ExactPairBlockTransactionalPagedHostSchedulerReceipt
ExactPairBlockTransactionalPagedHostScheduler::retry_retained_suffix(
    ExactPairBlockTransactionalPageCapacity capacity) noexcept {
  return publish_staged(retained_source_count_, capacity);
}

ExactPairBlockTransactionalPagedHostSchedulerReceipt
ExactPairBlockTransactionalPagedHostScheduler::publish_staged(
    std::size_t submitted_source_count,
    ExactPairBlockTransactionalPageCapacity capacity) noexcept {
  ExactPairBlockTransactionalPagedHostSchedulerReceipt receipt;
  receipt.submitted_source_count = submitted_source_count;
  receipt.page_source_count = retained_source_count_;
  receipt.publication_epoch = publication_epoch_;
  receipt.cumulative_published_totals = cumulative_published_totals_;
  receipt.cumulative_published_source_unordered_pair_mass =
      cumulative_published_source_unordered_pair_mass_;
  receipt.planner_exercised = true;
  std::uint64_t initial_cumulative_mass = 0U;
  receipt.cumulative_mass_closed =
      resolved_mass(
          cumulative_published_totals_, initial_cumulative_mass) &&
      initial_cumulative_mass ==
          cumulative_published_source_unordered_pair_mass_;

  const auto staged = std::span{retained_}.first(retained_source_count_);
  const ExactPairBlockTransactionalPagePlanView plan =
      plan_exact_pair_block_transactional_page_into(
          staged, capacity, std::span{exclusive_offset_storage_});
  receipt.planner_status = plan.status;

  if (plan.status == ExactPairBlockTransactionalPagePlanStatus::empty_page) {
    receipt.status =
        ExactPairBlockTransactionalPagedHostSchedulerStatus::empty_page;
    receipt.page_mass_closed = true;
    return receipt;
  }
  if (plan.status == ExactPairBlockTransactionalPagePlanStatus::
                         invalid_source_mass_partition) {
    retained_source_count_ = 0U;
    receipt.status = ExactPairBlockTransactionalPagedHostSchedulerStatus::
        invalid_page_rejected;
    return receipt;
  }
  if (plan.status ==
          ExactPairBlockTransactionalPagePlanStatus::arithmetic_overflow ||
      plan.status == ExactPairBlockTransactionalPagePlanStatus::
                         exclusive_scan_storage_exhausted) {
    retained_source_count_ = 0U;
    receipt.status = ExactPairBlockTransactionalPagedHostSchedulerStatus::
        arithmetic_overflow_rejected;
    return receipt;
  }

  receipt.page_totals = plan.exclusive_offsets.back();
  if (!resolved_mass(
          receipt.page_totals,
          receipt.page_source_unordered_pair_mass)) {
    retained_source_count_ = 0U;
    receipt.status = ExactPairBlockTransactionalPagedHostSchedulerStatus::
        arithmetic_overflow_rejected;
    receipt.planner_status =
        ExactPairBlockTransactionalPagePlanStatus::arithmetic_overflow;
    return receipt;
  }

  if (plan.status == ExactPairBlockTransactionalPagePlanStatus::
                         first_source_capacity_exhausted) {
    receipt.status = ExactPairBlockTransactionalPagedHostSchedulerStatus::
        first_source_capacity_exhausted_suffix_retained;
    receipt.retained_source_count = retained_source_count_;
    receipt.retained_source_unordered_pair_mass =
        receipt.page_source_unordered_pair_mass;
    receipt.retained_totals = receipt.page_totals;
    receipt.suffix_retained = true;
    receipt.page_mass_closed = true;
    return receipt;
  }

  receipt.published_source_count = plan.committed_source_count;
  receipt.published_source_unordered_pair_mass =
      plan.committed_source_unordered_pair_mass;
  receipt.published_totals = plan.committed_totals;
  receipt.retained_source_count =
      retained_source_count_ - plan.committed_source_count;
  receipt.retained_source_unordered_pair_mass =
      receipt.page_source_unordered_pair_mass -
      receipt.published_source_unordered_pair_mass;
  receipt.retained_totals =
      subtract_totals(receipt.page_totals, receipt.published_totals);
  receipt.page_mass_closed =
      totals_partition(
          receipt.page_totals,
          receipt.published_totals,
          receipt.retained_totals) &&
      receipt.published_source_unordered_pair_mass +
              receipt.retained_source_unordered_pair_mass ==
          receipt.page_source_unordered_pair_mass;

  ExactPairBlockTransactionalPageTotals cumulative_totals{};
  std::uint64_t cumulative_source_mass = 0U;
  if (!receipt.page_mass_closed ||
      publication_epoch_ == std::numeric_limits<std::uint64_t>::max() ||
      !checked_add_totals(
          cumulative_published_totals_,
          receipt.published_totals,
          cumulative_totals) ||
      !checked_add(
          cumulative_published_source_unordered_pair_mass_,
          receipt.published_source_unordered_pair_mass,
          cumulative_source_mass)) {
    receipt.status = ExactPairBlockTransactionalPagedHostSchedulerStatus::
        arithmetic_overflow_rejected;
    receipt.published_source_count = 0U;
    receipt.published_source_unordered_pair_mass = 0U;
    receipt.published_totals = {};
    receipt.retained_source_count = retained_source_count_;
    receipt.retained_source_unordered_pair_mass =
        receipt.page_source_unordered_pair_mass;
    receipt.retained_totals = receipt.page_totals;
    receipt.suffix_retained = retained_source_count_ != 0U;
    receipt.page_mass_closed = true;
    return receipt;
  }

  // All checks precede this mutation boundary.  The following fixed-size
  // copies and scalar assignments cannot fail, so the prefix publication is
  // atomic with respect to the observable scheduler ledger.
  for (std::size_t suffix_index = 0U;
       suffix_index < receipt.retained_source_count;
       ++suffix_index) {
    retained_[suffix_index] =
        retained_[plan.committed_source_count + suffix_index];
  }
  retained_source_count_ = receipt.retained_source_count;
  cumulative_published_totals_ = cumulative_totals;
  cumulative_published_source_unordered_pair_mass_ = cumulative_source_mass;
  ++publication_epoch_;

  receipt.publication_epoch = publication_epoch_;
  receipt.cumulative_published_totals = cumulative_published_totals_;
  receipt.cumulative_published_source_unordered_pair_mass =
      cumulative_published_source_unordered_pair_mass_;
  receipt.atomic_prefix_published = true;
  receipt.suffix_retained = retained_source_count_ != 0U;
  std::uint64_t closed_cumulative_mass = 0U;
  receipt.cumulative_mass_closed =
      resolved_mass(
          cumulative_published_totals_, closed_cumulative_mass) &&
      closed_cumulative_mass ==
          cumulative_published_source_unordered_pair_mass_;
  receipt.status = retained_source_count_ == 0U
      ? ExactPairBlockTransactionalPagedHostSchedulerStatus::
            complete_page_published
      : ExactPairBlockTransactionalPagedHostSchedulerStatus::
            prefix_published_suffix_retained;
  return receipt;
}

}  // namespace morsehgp3d::gpu::detail
