#include "exact_pair_block_transactional_page_plan.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>

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

[[nodiscard]] bool fits(
    const ExactPairBlockTransactionalPageTotals& totals,
    const ExactPairBlockTransactionalPageCapacity& capacity) noexcept {
  return totals.successor_block_count <= capacity.successor_block_count &&
      totals.prune_receipt_count <= capacity.prune_receipt_count &&
      totals.terminal_pair_count <= capacity.terminal_pair_count;
}

[[nodiscard]] ExactPairBlockTransactionalPagePlan failed_plan(
    ExactPairBlockTransactionalPagePlanStatus status,
    ExactPairBlockTransactionalPageCapacity capacity,
    std::size_t source_count,
    std::size_t failure_source_index) {
  ExactPairBlockTransactionalPagePlan plan;
  plan.status = status;
  plan.capacity = capacity;
  plan.source_count = source_count;
  plan.first_uncommitted_source_index = 0U;
  plan.failure_source_index = failure_source_index;
  plan.exclusive_offsets.emplace_back();
  return plan;
}

[[nodiscard]] ExactPairBlockTransactionalPagePlanView failed_plan_view(
    ExactPairBlockTransactionalPagePlanStatus status,
    ExactPairBlockTransactionalPageCapacity capacity,
    std::size_t source_count,
    std::size_t failure_source_index,
    std::span<ExactPairBlockTransactionalPageTotals> storage) noexcept {
  ExactPairBlockTransactionalPagePlanView plan;
  plan.status = status;
  plan.capacity = capacity;
  plan.source_count = source_count;
  plan.first_uncommitted_source_index = 0U;
  plan.failure_source_index = failure_source_index;
  if (!storage.empty()) {
    storage.front() = {};
    plan.exclusive_offsets = storage.first(1U);
  }
  return plan;
}

}  // namespace

ExactPairBlockTransactionalPagePlan
plan_exact_pair_block_transactional_page(
    std::span<const ExactPairBlockTransactionalPageContribution>
        contributions,
    ExactPairBlockTransactionalPageCapacity capacity) {
  if (contributions.size() == std::numeric_limits<std::size_t>::max()) {
    return failed_plan(
        ExactPairBlockTransactionalPagePlanStatus::arithmetic_overflow,
        capacity,
        contributions.size(),
        0U);
  }

  std::vector<ExactPairBlockTransactionalPageTotals> storage(
      contributions.size() + 1U);
  const ExactPairBlockTransactionalPagePlanView view =
      plan_exact_pair_block_transactional_page_into(
          contributions, capacity, std::span{storage});
  ExactPairBlockTransactionalPagePlan plan;
  plan.status = view.status;
  plan.capacity = capacity;
  plan.source_count = view.source_count;
  plan.committed_source_count = view.committed_source_count;
  plan.first_uncommitted_source_index =
      view.first_uncommitted_source_index;
  plan.failure_source_index = view.failure_source_index;
  plan.committed_source_unordered_pair_mass =
      view.committed_source_unordered_pair_mass;
  plan.committed_totals = view.committed_totals;
  storage.resize(view.exclusive_offsets.size());
  plan.exclusive_offsets = std::move(storage);
  return plan;
}

ExactPairBlockTransactionalPagePlanView
plan_exact_pair_block_transactional_page_into(
    std::span<const ExactPairBlockTransactionalPageContribution>
        contributions,
    ExactPairBlockTransactionalPageCapacity capacity,
    std::span<ExactPairBlockTransactionalPageTotals>
        exclusive_offset_storage) noexcept {
  if (contributions.size() == std::numeric_limits<std::size_t>::max()) {
    return failed_plan_view(
        ExactPairBlockTransactionalPagePlanStatus::arithmetic_overflow,
        capacity,
        contributions.size(),
        0U,
        exclusive_offset_storage);
  }
  const std::size_t required_storage = contributions.size() + 1U;
  if (exclusive_offset_storage.size() < required_storage) {
    return failed_plan_view(
        ExactPairBlockTransactionalPagePlanStatus::
            exclusive_scan_storage_exhausted,
        capacity,
        contributions.size(),
        0U,
        exclusive_offset_storage);
  }
  if (contributions.empty()) {
    return failed_plan_view(
        ExactPairBlockTransactionalPagePlanStatus::empty_page,
        capacity,
        0U,
        exact_pair_block_transactional_page_no_source_index,
        exclusive_offset_storage);
  }

  // Validate the complete page before retaining any prefix.  A malformed
  // suffix must not allow an otherwise plausible prefix to become authority.
  std::uint64_t source_mass_total = 0U;
  for (std::size_t source_index = 0U;
       source_index < contributions.size();
       ++source_index) {
    const ExactPairBlockTransactionalPageContribution& contribution =
        contributions[source_index];
    std::uint64_t output_mass = 0U;
    if (!resolved_mass(contribution.outputs, output_mass) ||
        !checked_add(
            source_mass_total,
            contribution.source_unordered_pair_mass,
            source_mass_total)) {
      return failed_plan_view(
          ExactPairBlockTransactionalPagePlanStatus::arithmetic_overflow,
          capacity,
          contributions.size(),
          source_index,
          exclusive_offset_storage);
    }
    if (output_mass != contribution.source_unordered_pair_mass) {
      return failed_plan_view(
          ExactPairBlockTransactionalPagePlanStatus::
              invalid_source_mass_partition,
          capacity,
          contributions.size(),
          source_index,
          exclusive_offset_storage);
    }
  }

  exclusive_offset_storage.front() = {};
  for (std::size_t source_index = 0U;
       source_index < contributions.size();
       ++source_index) {
    if (!checked_add_totals(
            exclusive_offset_storage[source_index],
            contributions[source_index].outputs,
            exclusive_offset_storage[source_index + 1U])) {
      return failed_plan_view(
          ExactPairBlockTransactionalPagePlanStatus::arithmetic_overflow,
          capacity,
          contributions.size(),
          source_index,
          exclusive_offset_storage);
    }
  }

  const auto offsets = exclusive_offset_storage.first(required_storage);
  std::uint64_t scanned_source_mass = 0U;
  if (!resolved_mass(offsets.back(), scanned_source_mass)) {
    return failed_plan_view(
        ExactPairBlockTransactionalPagePlanStatus::arithmetic_overflow,
        capacity,
        contributions.size(),
        contributions.size() - 1U,
        exclusive_offset_storage);
  }
  if (scanned_source_mass != source_mass_total) {
    return failed_plan_view(
        ExactPairBlockTransactionalPagePlanStatus::
            invalid_source_mass_partition,
        capacity,
        contributions.size(),
        contributions.size() - 1U,
        exclusive_offset_storage);
  }

  std::size_t committed_source_count = 0U;
  for (std::size_t prefix = 1U; prefix < offsets.size(); ++prefix) {
    if (!fits(offsets[prefix], capacity)) {
      break;
    }
    committed_source_count = prefix;
  }

  ExactPairBlockTransactionalPagePlanView plan;
  plan.capacity = capacity;
  plan.source_count = contributions.size();
  plan.first_uncommitted_source_index = committed_source_count;
  plan.failure_source_index =
      exact_pair_block_transactional_page_no_source_index;
  plan.exclusive_offsets = offsets;
  if (committed_source_count == 0U) {
    plan.status = ExactPairBlockTransactionalPagePlanStatus::
        first_source_capacity_exhausted;
    return plan;
  }

  plan.committed_source_count = committed_source_count;
  plan.committed_totals = offsets[committed_source_count];
  if (!resolved_mass(
          plan.committed_totals,
          plan.committed_source_unordered_pair_mass)) {
    return failed_plan_view(
        ExactPairBlockTransactionalPagePlanStatus::arithmetic_overflow,
        capacity,
        contributions.size(),
        committed_source_count - 1U,
        exclusive_offset_storage);
  }
  plan.status = committed_source_count == contributions.size()
      ? ExactPairBlockTransactionalPagePlanStatus::complete_prefix
      : ExactPairBlockTransactionalPagePlanStatus::partial_prefix;
  return plan;
}

}  // namespace morsehgp3d::gpu::detail
