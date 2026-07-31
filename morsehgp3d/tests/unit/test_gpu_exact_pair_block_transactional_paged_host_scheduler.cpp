#include "exact_pair_block_transactional_paged_host_scheduler.hpp"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace {

using morsehgp3d::gpu::detail::
    ExactPairBlockTransactionalPageCapacity;
using morsehgp3d::gpu::detail::
    ExactPairBlockTransactionalPageContribution;
using morsehgp3d::gpu::detail::ExactPairBlockTransactionalPageTotals;
using morsehgp3d::gpu::detail::
    ExactPairBlockTransactionalPagedHostScheduler;
using morsehgp3d::gpu::detail::
    ExactPairBlockTransactionalPagedHostSchedulerStatus;

static_assert(
    std::is_trivially_destructible_v<
        ExactPairBlockTransactionalPagedHostScheduler>);

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] ExactPairBlockTransactionalPageContribution contribution(
    std::uint64_t source_mass,
    std::uint64_t successor_count,
    std::uint64_t receipt_count,
    std::uint64_t terminal_count,
    std::uint64_t successor_mass,
    std::uint64_t pruned_mass,
    std::uint64_t terminal_mass) {
  return {
      source_mass,
      ExactPairBlockTransactionalPageTotals{
          successor_count,
          receipt_count,
          terminal_count,
          successor_mass,
          pruned_mass,
          terminal_mass}};
}

void test_partial_prefix_is_published_and_suffix_is_retried_first() {
  ExactPairBlockTransactionalPagedHostScheduler scheduler;
  const std::array page{
      contribution(10U, 3U, 0U, 0U, 10U, 0U, 0U),
      contribution(7U, 0U, 1U, 0U, 0U, 7U, 0U),
      contribution(1U, 0U, 0U, 1U, 0U, 0U, 1U),
      contribution(4U, 2U, 0U, 0U, 4U, 0U, 0U)};

  const auto partial = scheduler.consume_page(
      std::span{page}, ExactPairBlockTransactionalPageCapacity{4U, 1U, 1U});
  require(
      partial.status == ExactPairBlockTransactionalPagedHostSchedulerStatus::
                            prefix_published_suffix_retained &&
          partial.planner_exercised && partial.atomic_prefix_published &&
          partial.published() && partial.suffix_retained &&
          partial.page_mass_closed && partial.cumulative_mass_closed &&
          partial.page_source_count == 4U &&
          partial.published_source_count == 3U &&
          partial.retained_source_count == 1U &&
          partial.page_source_unordered_pair_mass == 22U &&
          partial.published_source_unordered_pair_mass == 18U &&
          partial.retained_source_unordered_pair_mass == 4U &&
          partial.published_totals ==
              ExactPairBlockTransactionalPageTotals{
                  3U, 1U, 1U, 10U, 7U, 1U} &&
          partial.retained_totals ==
              ExactPairBlockTransactionalPageTotals{
                  2U, 0U, 0U, 4U, 0U, 0U} &&
          scheduler.publication_epoch() == 1U &&
          scheduler.retained_contributions().size() == 1U &&
          scheduler.retained_contributions().front() == page.back(),
      "the host scheduler must atomically publish the largest prefix and "
      "retain the exact suffix");

  const std::array blocked_new_page{
      contribution(2U, 0U, 0U, 2U, 0U, 0U, 2U)};
  const auto blocked = scheduler.consume_page(
      std::span{blocked_new_page},
      ExactPairBlockTransactionalPageCapacity{9U, 9U, 9U});
  require(
      blocked.status == ExactPairBlockTransactionalPagedHostSchedulerStatus::
                            retained_suffix_requires_retry &&
          !blocked.planner_exercised && !blocked.published() &&
          blocked.submitted_source_count == 1U &&
          blocked.retained_source_count == 1U &&
          scheduler.publication_epoch() == 1U &&
          scheduler.retained_contributions().front() == page.back(),
      "a new page must never overtake a retained suffix");

  const auto drained = scheduler.retry_retained_suffix(
      ExactPairBlockTransactionalPageCapacity{2U, 0U, 0U});
  require(
      drained.status == ExactPairBlockTransactionalPagedHostSchedulerStatus::
                            complete_page_published &&
          drained.published_source_count == 1U &&
          drained.retained_source_count == 0U && !drained.suffix_retained &&
          drained.page_mass_closed && drained.cumulative_mass_closed &&
          drained.publication_epoch == 2U &&
          drained.cumulative_published_source_unordered_pair_mass == 22U &&
          drained.cumulative_published_totals ==
              ExactPairBlockTransactionalPageTotals{
                  5U, 1U, 1U, 14U, 7U, 1U} &&
          !scheduler.has_retained_suffix(),
      "retrying with sufficient caps must publish the preserved suffix and "
      "close the cumulative mass ledger");
}

void test_first_source_failure_has_no_observable_commit() {
  ExactPairBlockTransactionalPagedHostScheduler scheduler;
  const std::array page{
      contribution(12U, 3U, 0U, 0U, 12U, 0U, 0U),
      contribution(1U, 0U, 0U, 1U, 0U, 0U, 1U)};
  const auto stopped = scheduler.consume_page(
      std::span{page}, ExactPairBlockTransactionalPageCapacity{2U, 1U, 1U});
  require(
      stopped.status ==
              ExactPairBlockTransactionalPagedHostSchedulerStatus::
                  first_source_capacity_exhausted_suffix_retained &&
          !stopped.published() && stopped.published_source_count == 0U &&
          stopped.retained_source_count == page.size() &&
          stopped.retained_source_unordered_pair_mass == 13U &&
          stopped.page_mass_closed && stopped.suffix_retained &&
          scheduler.publication_epoch() == 0U &&
          scheduler.cumulative_published_totals() ==
              ExactPairBlockTransactionalPageTotals{} &&
          scheduler.retained_contributions()[0U] == page[0U] &&
          scheduler.retained_contributions()[1U] == page[1U],
      "an impossible first source must publish nothing and retain the whole "
      "validated page");

  const auto retried = scheduler.retry_retained_suffix(
      ExactPairBlockTransactionalPageCapacity{3U, 0U, 1U});
  require(
      retried.status == ExactPairBlockTransactionalPagedHostSchedulerStatus::
                            complete_page_published &&
          retried.published_source_count == 2U &&
          retried.published_source_unordered_pair_mass == 13U &&
          retried.page_mass_closed && retried.cumulative_mass_closed &&
          scheduler.publication_epoch() == 1U,
      "the unchanged page must be retryable without reclassification");
}

void test_invalid_or_oversized_pages_cannot_mutate_the_ledger() {
  ExactPairBlockTransactionalPagedHostScheduler scheduler;
  const std::array invalid_page{
      contribution(5U, 2U, 0U, 0U, 5U, 0U, 0U),
      contribution(9U, 0U, 1U, 0U, 0U, 8U, 0U)};
  const auto invalid = scheduler.consume_page(
      std::span{invalid_page},
      ExactPairBlockTransactionalPageCapacity{9U, 9U, 9U});
  require(
      invalid.status == ExactPairBlockTransactionalPagedHostSchedulerStatus::
                            invalid_page_rejected &&
          invalid.planner_exercised && !invalid.published() &&
          !invalid.page_mass_closed && !scheduler.has_retained_suffix() &&
          scheduler.publication_epoch() == 0U &&
          scheduler.cumulative_published_totals() ==
              ExactPairBlockTransactionalPageTotals{},
      "an invalid suffix identity must reject the whole page before any "
      "publication");

  const std::array<
      ExactPairBlockTransactionalPageContribution,
      ExactPairBlockTransactionalPagedHostScheduler::
              maximum_page_source_count +
          1U>
      oversized{};
  const auto exhausted = scheduler.consume_page(
      std::span{oversized},
      ExactPairBlockTransactionalPageCapacity{9U, 9U, 9U});
  require(
      exhausted.status ==
              ExactPairBlockTransactionalPagedHostSchedulerStatus::
                  page_working_set_exhausted &&
          !exhausted.planner_exercised && !exhausted.published() &&
          exhausted.submitted_source_count == oversized.size() &&
          !scheduler.has_retained_suffix() &&
          scheduler.publication_epoch() == 0U,
      "a page beyond the fixed working set must fail before copying or "
      "planning");
}

}  // namespace

int main() {
  try {
    test_partial_prefix_is_published_and_suffix_is_retried_first();
    test_first_source_failure_has_no_observable_commit();
    test_invalid_or_oversized_pages_cannot_mutate_the_ledger();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "transactional paged host scheduler tests passed\n";
  return 0;
}
