#include "exact_pair_block_transactional_page_plan.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>

namespace {

using morsehgp3d::gpu::detail::
    ExactPairBlockTransactionalPageCapacity;
using morsehgp3d::gpu::detail::
    ExactPairBlockTransactionalPageContribution;
using morsehgp3d::gpu::detail::ExactPairBlockTransactionalPagePlanStatus;
using morsehgp3d::gpu::detail::ExactPairBlockTransactionalPageTotals;
using morsehgp3d::gpu::detail::
    exact_pair_block_transactional_page_no_source_index;
using morsehgp3d::gpu::detail::
    plan_exact_pair_block_transactional_page;
using morsehgp3d::gpu::detail::
    plan_exact_pair_block_transactional_page_into;

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

void test_complete_scan_and_largest_partial_prefix() {
  const std::array sources{
      contribution(10U, 3U, 0U, 0U, 10U, 0U, 0U),
      contribution(7U, 0U, 1U, 0U, 0U, 7U, 0U),
      contribution(1U, 0U, 0U, 1U, 0U, 0U, 1U)};
  const ExactPairBlockTransactionalPageCapacity exact_capacity{3U, 1U, 1U};
  const auto complete = plan_exact_pair_block_transactional_page(
      std::span{sources}, exact_capacity);
  const std::array expected_offsets{
      ExactPairBlockTransactionalPageTotals{},
      ExactPairBlockTransactionalPageTotals{3U, 0U, 0U, 10U, 0U, 0U},
      ExactPairBlockTransactionalPageTotals{3U, 1U, 0U, 10U, 7U, 0U},
      ExactPairBlockTransactionalPageTotals{3U, 1U, 1U, 10U, 7U, 1U}};
  require(
      complete.status ==
              ExactPairBlockTransactionalPagePlanStatus::complete_prefix &&
          complete.can_commit() && complete.complete_page() &&
          complete.source_count == sources.size() &&
          complete.committed_source_count == sources.size() &&
          complete.first_uncommitted_source_index == sources.size() &&
          complete.failure_source_index ==
              exact_pair_block_transactional_page_no_source_index &&
          complete.committed_source_unordered_pair_mass == 18U &&
          complete.committed_totals == expected_offsets.back() &&
          complete.exclusive_offsets.size() == expected_offsets.size() &&
          std::equal(
              complete.exclusive_offsets.begin(),
              complete.exclusive_offsets.end(),
              expected_offsets.begin(),
              expected_offsets.end()),
      "the complete page must expose six exact exclusive scans and close "
      "all source mass");

  const std::array extended_sources{
      sources[0U],
      sources[1U],
      sources[2U],
      contribution(4U, 2U, 0U, 0U, 4U, 0U, 0U)};
  const ExactPairBlockTransactionalPageCapacity partial_capacity{4U, 1U, 1U};
  const auto partial = plan_exact_pair_block_transactional_page(
      std::span{extended_sources}, partial_capacity);
  require(
      partial.status ==
              ExactPairBlockTransactionalPagePlanStatus::partial_prefix &&
          partial.can_commit() && !partial.complete_page() &&
          partial.committed_source_count == 3U &&
          partial.first_uncommitted_source_index == 3U &&
          partial.committed_source_unordered_pair_mass == 18U &&
          partial.committed_totals == expected_offsets.back() &&
          partial.exclusive_offsets.size() == extended_sources.size() + 1U &&
          partial.exclusive_offsets.back() ==
              ExactPairBlockTransactionalPageTotals{
                  5U, 1U, 1U, 14U, 7U, 1U},
      "the planner must choose the largest source prefix fitting every cap");

  const auto repeated = plan_exact_pair_block_transactional_page(
      std::span{extended_sources}, partial_capacity);
  require(
      repeated == partial,
      "identical contributions and capacities must produce a byte-stable "
      "deterministic page plan");
}

void test_first_source_impossible_and_empty_page() {
  const std::array sources{
      contribution(12U, 3U, 0U, 0U, 12U, 0U, 0U),
      contribution(1U, 0U, 0U, 1U, 0U, 0U, 1U)};
  const auto impossible = plan_exact_pair_block_transactional_page(
      std::span{sources},
      ExactPairBlockTransactionalPageCapacity{2U, 1U, 1U});
  require(
      impossible.status == ExactPairBlockTransactionalPagePlanStatus::
                               first_source_capacity_exhausted &&
          impossible.no_commit() && !impossible.can_commit() &&
          impossible.committed_source_count == 0U &&
          impossible.first_uncommitted_source_index == 0U &&
          impossible.committed_source_unordered_pair_mass == 0U &&
          impossible.committed_totals ==
              ExactPairBlockTransactionalPageTotals{} &&
          impossible.exclusive_offsets.size() == sources.size() + 1U,
      "an impossible first source must leave the scientific prefix empty");

  const std::array<ExactPairBlockTransactionalPageContribution, 0U> empty{};
  const auto empty_plan = plan_exact_pair_block_transactional_page(
      std::span{empty},
      ExactPairBlockTransactionalPageCapacity{9U, 9U, 9U});
  require(
      empty_plan.status ==
              ExactPairBlockTransactionalPagePlanStatus::empty_page &&
          empty_plan.no_commit() && empty_plan.source_count == 0U &&
          empty_plan.exclusive_offsets.size() == 1U &&
          empty_plan.exclusive_offsets[0U] ==
              ExactPairBlockTransactionalPageTotals{},
      "an empty page must retain only the zero exclusive-scan sentinel");
}

void test_invalid_mass_and_overflow_fail_before_commit() {
  const std::array invalid_sources{
      contribution(5U, 2U, 0U, 0U, 5U, 0U, 0U),
      contribution(9U, 0U, 1U, 0U, 0U, 8U, 0U)};
  const auto invalid = plan_exact_pair_block_transactional_page(
      std::span{invalid_sources},
      ExactPairBlockTransactionalPageCapacity{10U, 10U, 10U});
  require(
      invalid.status == ExactPairBlockTransactionalPagePlanStatus::
                            invalid_source_mass_partition &&
          invalid.no_commit() && invalid.failure_source_index == 1U &&
          invalid.first_uncommitted_source_index == 0U &&
          invalid.exclusive_offsets.size() == 1U &&
          invalid.exclusive_offsets[0U] ==
              ExactPairBlockTransactionalPageTotals{},
      "an invalid suffix mass identity must revoke the whole candidate page");

  constexpr std::uint64_t maximum =
      std::numeric_limits<std::uint64_t>::max();
  const std::array count_overflow_sources{
      contribution(1U, maximum, 0U, 0U, 1U, 0U, 0U),
      contribution(1U, 1U, 0U, 0U, 1U, 0U, 0U)};
  const auto count_overflow = plan_exact_pair_block_transactional_page(
      std::span{count_overflow_sources},
      ExactPairBlockTransactionalPageCapacity{maximum, maximum, maximum});
  require(
      count_overflow.status ==
              ExactPairBlockTransactionalPagePlanStatus::
                  arithmetic_overflow &&
          count_overflow.no_commit() &&
          count_overflow.failure_source_index == 1U &&
          count_overflow.exclusive_offsets.size() == 1U,
      "overflow in any of the six scans must fail closed without a prefix");

  const std::array mass_overflow_sources{
      contribution(maximum, 1U, 0U, 0U, maximum, 0U, 0U),
      contribution(1U, 0U, 0U, 1U, 0U, 0U, 1U)};
  const auto mass_overflow = plan_exact_pair_block_transactional_page(
      std::span{mass_overflow_sources},
      ExactPairBlockTransactionalPageCapacity{2U, 2U, 2U});
  require(
      mass_overflow.status ==
              ExactPairBlockTransactionalPagePlanStatus::
                  arithmetic_overflow &&
          mass_overflow.no_commit() &&
          mass_overflow.failure_source_index == 1U &&
          mass_overflow.exclusive_offsets.size() == 1U,
      "overflow in aggregate source mass must also revoke the whole page");
}

void test_allocation_free_scan_storage_contract() {
  const std::array sources{
      contribution(3U, 2U, 0U, 0U, 3U, 0U, 0U),
      contribution(1U, 0U, 0U, 1U, 0U, 0U, 1U)};
  std::array<ExactPairBlockTransactionalPageTotals, 3U> storage{};
  const auto bounded = plan_exact_pair_block_transactional_page_into(
      std::span{sources},
      ExactPairBlockTransactionalPageCapacity{2U, 0U, 1U},
      std::span{storage});
  require(
      bounded.complete_page() && bounded.can_commit() &&
          bounded.committed_source_count == 2U &&
          bounded.exclusive_offsets.size() == storage.size() &&
          bounded.exclusive_offsets.data() == storage.data() &&
          bounded.committed_totals ==
              ExactPairBlockTransactionalPageTotals{
                  2U, 0U, 1U, 3U, 0U, 1U},
      "the bounded planner must use caller-owned scan storage");

  std::array<ExactPairBlockTransactionalPageTotals, 2U> short_storage{
      ExactPairBlockTransactionalPageTotals{9U, 9U, 9U, 9U, 9U, 9U},
      ExactPairBlockTransactionalPageTotals{9U, 9U, 9U, 9U, 9U, 9U}};
  const auto exhausted = plan_exact_pair_block_transactional_page_into(
      std::span{sources},
      ExactPairBlockTransactionalPageCapacity{2U, 0U, 1U},
      std::span{short_storage});
  require(
      exhausted.status == ExactPairBlockTransactionalPagePlanStatus::
                              exclusive_scan_storage_exhausted &&
          exhausted.no_commit() &&
          exhausted.exclusive_offsets.size() == 1U &&
          exhausted.exclusive_offsets.front() ==
              ExactPairBlockTransactionalPageTotals{},
      "insufficient caller storage must expose only a zero sentinel and no "
      "publishable prefix");
}

}  // namespace

int main() {
  try {
    test_complete_scan_and_largest_partial_prefix();
    test_first_source_impossible_and_empty_page();
    test_invalid_mass_and_overflow_fail_before_commit();
    test_allocation_free_scan_storage_contract();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "transactional pair-block page planner tests passed\n";
  return 0;
}
