#include "phase15_exact_pair_block_transactional_frontier_resident_cuda_internal.hpp"

#include "exact_pair_block_transactional_paged_host_scheduler.hpp"

#include "morsehgp3d/gpu/exact_pair_block_transactional_frontier_cuda.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>

namespace morsehgp3d::gpu::detail {
namespace {

inline constexpr std::size_t host_fake_page_source_count = 2U;
inline constexpr std::uint64_t host_fake_page_successor_capacity = 3U;
inline constexpr std::uint64_t host_fake_page_prune_capacity = 1U;
inline constexpr std::uint64_t host_fake_page_terminal_capacity = 1U;

static_assert(
    host_fake_page_source_count <=
    ExactPairBlockTransactionalPagedHostScheduler::
        maximum_page_source_count);

struct PagedHostExecutionAudit {
  std::size_t submission_count{};
  std::size_t publication_count{};
  std::size_t suffix_retry_count{};
  std::size_t maximum_submitted_source_count{};
  std::size_t maximum_published_source_count{};
  bool mass_closed{true};
};

inline constexpr std::size_t no_recipe_index =
    std::numeric_limits<std::size_t>::max();

struct PreparedHostPageSource {
  ExactPairBlockTransactionalFrontierCudaProposal proposal{};
  ExactPairBlockTransactionalPageContribution contribution{};
  std::size_t recipe_index{no_recipe_index};
};

static_assert(
    std::numeric_limits<std::size_t>::digits <=
    std::numeric_limits<std::uint64_t>::digits);

[[nodiscard]] ExactPairBlockTransactionalPageContribution contribution_from(
    const ExactPairBlockAuthority& source,
    const ExactPairBlockTransactionalFrontierCudaTransitionPreview& preview) {
  if (!preview.ready()) {
    throw std::logic_error(
        "the paged fake resident scheduler could not classify a source");
  }
  return {
      static_cast<std::uint64_t>(source.unordered_pair_mass),
      ExactPairBlockTransactionalPageTotals{
          static_cast<std::uint64_t>(preview.successor_block_count),
          static_cast<std::uint64_t>(preview.prune_receipt_count),
          static_cast<std::uint64_t>(preview.terminal_pair_count),
          static_cast<std::uint64_t>(
              preview.successor_unordered_pair_mass),
          static_cast<std::uint64_t>(preview.pruned_unordered_pair_mass),
          static_cast<std::uint64_t>(
              preview.terminal_unordered_pair_mass)}};
}

[[nodiscard]] bool matches(
    const Phase15ExactPairBlockTransactionalFrontierResidentCudaBlock& recipe,
    const ExactPairBlockAuthority& source) noexcept {
  return recipe.kind == static_cast<std::uint8_t>(source.kind) &&
      recipe.unordered_pair_mass == source.unordered_pair_mass &&
      recipe.first.node_index == source.first.node_index &&
      recipe.first.leaf_begin == source.first.leaf_begin &&
      recipe.first.leaf_end == source.first.leaf_end &&
      recipe.second.node_index == source.second.node_index &&
      recipe.second.leaf_begin == source.second.leaf_begin &&
      recipe.second.leaf_end == source.second.leaf_end;
}

[[nodiscard]] ExactPairBlockTransactionalFrontierResidentCudaAudit map_audit(
    const Phase15ExactPairBlockTransactionalFrontierResidentCudaRequest& request,
    const ExactPairBlockTransactionalFrontierCudaAudit& source,
    std::span<const std::uint8_t> used_recipes,
    std::size_t exact_witness_predicate_count,
    std::size_t exact_negative_witness_predicate_count,
    bool capacity_rollback,
    const PagedHostExecutionAudit& page_audit) {
  ExactPairBlockTransactionalFrontierResidentCudaAudit audit;
  audit.point_count = source.point_count;
  audit.certified_node_count = source.certified_node_count;
  audit.maximum_closed_rank = source.maximum_closed_rank;
  audit.required_witness_point_count = source.required_witness_point_count;
  audit.block_capacity_per_point =
      request.config.block_capacity_per_point;
  audit.block_capacity = request.block_capacity;
  audit.prune_receipt_capacity = request.prune_receipt_capacity;
  audit.terminal_pair_capacity = request.terminal_pair_capacity;
  audit.prune_recipe_capacity = request.recipe_capacity;
  audit.submitted_prune_recipe_count = request.recipes.size();
  audit.matched_prune_recipe_count = static_cast<std::size_t>(
      std::count(used_recipes.begin(), used_recipes.end(), std::uint8_t{1U}));
  audit.unused_prune_recipe_count =
      audit.submitted_prune_recipe_count - audit.matched_prune_recipe_count;
  audit.unordered_pair_universe_mass = source.unordered_pair_universe_mass;
  audit.pending_unordered_pair_mass = source.pending_unordered_pair_mass;
  audit.inflight_unordered_pair_mass = source.inflight_unordered_pair_mass;
  audit.pruned_unordered_pair_mass = source.pruned_unordered_pair_mass;
  audit.terminal_unordered_pair_mass = source.terminal_unordered_pair_mass;
  audit.pending_block_count = source.pending_block_count;
  audit.prune_receipt_count = source.prune_receipt_count;
  audit.terminal_pair_count = source.terminal_pair_count;
  audit.wave_begin_count = source.wave_begin_count;
  audit.wave_commit_count = source.wave_commit_count;
  audit.wave_rollback_count = source.wave_rollback_count;
  audit.host_page_submission_count = page_audit.submission_count;
  audit.host_page_publication_count = page_audit.publication_count;
  audit.host_page_suffix_retry_count = page_audit.suffix_retry_count;
  audit.host_page_maximum_submitted_source_count =
      page_audit.maximum_submitted_source_count;
  audit.host_page_maximum_published_source_count =
      page_audit.maximum_published_source_count;
  audit.host_page_working_set_byte_count =
      ExactPairBlockTransactionalPagedHostScheduler::
          bounded_working_set_byte_count();
  audit.exact_prune_attempt_count = source.exact_prune_attempt_count;
  audit.certified_prune_count = source.certified_prune_count;
  audit.exact_prune_fail_open_count = source.exact_prune_fail_open_count;
  audit.exact_witness_predicate_count = exact_witness_predicate_count;
  audit.exact_negative_witness_predicate_count =
      exact_negative_witness_predicate_count;
  audit.nonnegative_or_residual_witness_predicate_count =
      exact_witness_predicate_count - exact_negative_witness_predicate_count;
  audit.diagonal_split_count = source.diagonal_split_count;
  audit.cross_split_count = source.cross_split_count;
  audit.maximum_pending_block_count = source.maximum_pending_block_count;
  audit.source_snapshot_epoch = request.traversal->source_snapshot_epoch;
  audit.submitted_recipe_digest = request.submitted_recipe_digest;
  audit.native_lbvh_authority_consumed = true;
  audit.fixed_linear_capacities_validated =
      source.fixed_linear_capacities_validated;
  audit.compact_index_width_validated = true;
  audit.unique_recipe_sources_validated = true;
  audit.resident_wave_loop_executed = source.wave_begin_count != 0U;
  audit.zero_intermediate_d2h_validated = true;
  audit.complete_wave_atomic_commit_validated =
      source.wave_begin_count ==
          source.wave_commit_count + source.wave_rollback_count;
  audit.capacity_wave_rollback_validated = capacity_rollback;
  audit.bounded_paged_host_scheduler_exercised =
      page_audit.submission_count != 0U;
  audit.host_page_storage_independent_of_point_count =
      host_fake_page_source_count <=
          ExactPairBlockTransactionalPagedHostScheduler::
              maximum_page_source_count;
  audit.host_page_mass_closure_validated = page_audit.mass_closed;
  audit.fail_open_native_transition_validated =
      source.exact_prune_fail_open_count != 0U;
  audit.recipe_catalog_nonempty = !request.recipes.empty();
  audit.recipe_path_exercised =
      source.exact_prune_attempt_count != 0U;
  audit.certified_prune_path_exercised =
      source.certified_prune_count != 0U;
  audit.fail_open_path_exercised =
      source.exact_prune_fail_open_count != 0U;
  audit.transactional_mass_conservation_validated =
      source.transactional_mass_conservation_validated;
  audit.native_split_partition_validated =
      source.native_split_partition_validated;
  audit.pairwise_disjoint_support_products_validated =
      source.pairwise_disjoint_support_products_validated;
  audit.terminal_authority_sealed = source.terminal_authority_sealed;
  audit.global_pair_coverage_closed = source.global_pair_coverage_closed;
  audit.host_fake_lifecycle_exercised = true;
  audit.serial_device_reference = false;
  audit.scale_eligible = false;
  return audit;
}

}  // namespace

Phase15ExactPairBlockTransactionalFrontierResidentCudaReceipt
phase15_launch_exact_pair_block_transactional_frontier_resident_cuda(
    const Phase15ExactPairBlockTransactionalFrontierResidentCudaRequest&
        request) {
  if (request.traversal == nullptr || request.host_index == nullptr ||
      request.host_cloud == nullptr || !request.traversal->host_fake ||
      !request.traversal->ready) {
    throw std::invalid_argument(
        "the fake resident scheduler requires an adopted fake traversal");
  }

  auto context = ExactPairBlockTransactionalFrontierCudaContext::start(
      *request.host_index,
      *request.host_cloud,
      ExactPairBlockTransactionalFrontierCudaConfig{
          request.config.maximum_closed_rank,
          request.config.block_capacity_per_point,
          request.config.prune_receipt_capacity_per_point,
          request.config.terminal_pair_capacity_per_point});
  std::vector<std::uint8_t> used(request.recipes.size(), std::uint8_t{0U});
  std::uint64_t transaction_id = UINT64_C(1);
  std::size_t exact_witness_predicate_count = 0U;
  bool capacity_rollback = false;
  ExactPairBlockTransactionalPagedHostScheduler page_scheduler;
  PagedHostExecutionAudit page_audit;

  while (!context.complete()) {
    const auto pending = context.pending_blocks();
    if (pending.empty()) {
      throw std::logic_error(
          "the paged fake resident scheduler lost its pending frontier");
    }

    const bool retry_suffix = page_scheduler.has_retained_suffix();
    const std::size_t page_source_count = retry_suffix
        ? page_scheduler.retained_contributions().size()
        : std::min(host_fake_page_source_count, pending.size());
    if (page_source_count == 0U || page_source_count > pending.size() ||
        page_source_count > host_fake_page_source_count) {
      throw std::logic_error(
          "the paged fake resident scheduler exposed an invalid source page");
    }

    std::array<PreparedHostPageSource, host_fake_page_source_count> prepared{};
    for (std::size_t source_index = 0U;
         source_index < page_source_count;
         ++source_index) {
      PreparedHostPageSource& item = prepared[source_index];
      const ExactPairBlockAuthority& source = pending[source_index];
      item.proposal.transaction_id = transaction_id++;
      item.proposal.source = source;
      for (std::size_t recipe_index = 0U;
           recipe_index < request.recipes.size();
           ++recipe_index) {
        const auto& recipe = request.recipes[recipe_index];
        if (matches(recipe.source, source)) {
          item.recipe_index = recipe_index;
          item.proposal.kind =
              ExactPairBlockTransactionalFrontierCudaProposalKind::
                  try_exact_prune_else_open;
          item.proposal.witness_node_count = recipe.witness_node_count;
          for (std::size_t witness_index = 0U;
               witness_index < recipe.witness_node_count;
               ++witness_index) {
            item.proposal.witness_node_indices[witness_index] =
                recipe.witness_node_indices[witness_index];
          }
          break;
        }
      }
      item.contribution = contribution_from(
          source, context.preview_transition(item.proposal));
    }

    const auto contributions =
        std::span{prepared}.first(page_source_count);
    std::array<
        ExactPairBlockTransactionalPageContribution,
        host_fake_page_source_count>
        contribution_page{};
    for (std::size_t source_index = 0U;
         source_index < page_source_count;
         ++source_index) {
      contribution_page[source_index] =
          contributions[source_index].contribution;
    }
    const auto contribution_span =
        std::span{contribution_page}.first(page_source_count);
    if (retry_suffix &&
        !std::equal(
            contribution_span.begin(),
            contribution_span.end(),
            page_scheduler.retained_contributions().begin(),
            page_scheduler.retained_contributions().end())) {
      throw std::logic_error(
          "the resident frontier no longer begins with its retained page "
          "suffix");
    }

    const auto& frontier_audit = context.audit();
    const std::size_t retained_after_first = pending.size() - 1U;
    if (retained_after_first > frontier_audit.pending_block_capacity ||
        frontier_audit.prune_receipt_count >
            frontier_audit.prune_receipt_capacity ||
        frontier_audit.terminal_pair_count >
            frontier_audit.terminal_pair_capacity) {
      throw std::logic_error(
          "the resident frontier exceeded a physical output capacity");
    }
    const std::uint64_t available_successors = static_cast<std::uint64_t>(
        frontier_audit.pending_block_capacity - retained_after_first);
    const std::uint64_t available_prune_receipts =
        static_cast<std::uint64_t>(
            frontier_audit.prune_receipt_capacity -
            frontier_audit.prune_receipt_count);
    const std::uint64_t available_terminal_pairs =
        static_cast<std::uint64_t>(
            frontier_audit.terminal_pair_capacity -
            frontier_audit.terminal_pair_count);
    const ExactPairBlockTransactionalPageCapacity page_capacity{
        std::min(
            available_successors, host_fake_page_successor_capacity),
        std::min(
            available_prune_receipts, host_fake_page_prune_capacity),
        std::min(
            available_terminal_pairs, host_fake_page_terminal_capacity)};

    const ExactPairBlockTransactionalPagedHostSchedulerReceipt page =
        retry_suffix
        ? page_scheduler.retry_retained_suffix(page_capacity)
        : page_scheduler.consume_page(contribution_span, page_capacity);
    ++page_audit.submission_count;
    page_audit.suffix_retry_count +=
        static_cast<std::size_t>(retry_suffix);
    page_audit.maximum_submitted_source_count = std::max(
        page_audit.maximum_submitted_source_count,
        page.page_source_count);
    page_audit.maximum_published_source_count = std::max(
        page_audit.maximum_published_source_count,
        page.published_source_count);
    page_audit.mass_closed = page_audit.mass_closed &&
        page.page_mass_closed && page.cumulative_mass_closed;

    const auto record_attempted_recipes =
        [&prepared,
         &used,
         &exact_witness_predicate_count](std::size_t source_count) {
          for (std::size_t source_index = 0U;
               source_index < source_count;
               ++source_index) {
            const PreparedHostPageSource& item = prepared[source_index];
            if (item.recipe_index != no_recipe_index) {
              used[item.recipe_index] = std::uint8_t{1U};
              exact_witness_predicate_count +=
                  item.proposal.witness_node_count;
            }
          }
        };

    if (page.status ==
        ExactPairBlockTransactionalPagedHostSchedulerStatus::
            first_source_capacity_exhausted_suffix_retained) {
      if (context.begin_wave(1U) !=
          ExactPairBlockTransactionalFrontierCudaWaveStart::wave_ready) {
        throw std::logic_error(
            "the paged fake resident scheduler failed to open its rejected "
            "source");
      }
      record_attempted_recipes(1U);
      const std::array rejected_proposal{prepared[0U].proposal};
      const auto wave = context.commit_wave(rejected_proposal);
      if (wave.decision !=
          ExactPairBlockTransactionalFrontierCudaWaveDecision::
              rolled_back_capacity_exhausted) {
        throw std::logic_error(
            "the page planner and resident capacity decision diverged");
      }
      capacity_rollback = true;
      break;
    }
    if (!page.published() || page.published_source_count > page_source_count) {
      throw std::logic_error(
          "the paged fake resident scheduler rejected a prevalidated page");
    }

    if (context.begin_wave(page.published_source_count) !=
        ExactPairBlockTransactionalFrontierCudaWaveStart::wave_ready) {
      throw std::logic_error(
          "the fake resident scheduler failed to open a source page");
    }
    record_attempted_recipes(page.published_source_count);
    std::array<
        ExactPairBlockTransactionalFrontierCudaProposal,
        host_fake_page_source_count>
        proposals{};
    for (std::size_t source_index = 0U;
         source_index < page.published_source_count;
         ++source_index) {
      proposals[source_index] = prepared[source_index].proposal;
    }
    const auto wave = context.commit_wave(
        std::span{proposals}.first(page.published_source_count));
    if (wave.decision == ExactPairBlockTransactionalFrontierCudaWaveDecision::
                             rolled_back_capacity_exhausted) {
      throw std::logic_error(
          "the resident scheduler rejected a page already published by its "
          "bounded planner");
    }
    if (!wave.committed()) {
      throw std::logic_error(
          "the fake resident scheduler rejected a prevalidated wave");
    }
    ++page_audit.publication_count;
  }

  if (context.complete() && page_scheduler.has_retained_suffix()) {
    throw std::logic_error(
        "the complete resident frontier retained an unpublished page suffix");
  }

  Phase15ExactPairBlockTransactionalFrontierResidentCudaReceipt result;
  result.prune_receipts.assign(
      context.committed_prune_receipts().begin(),
      context.committed_prune_receipts().end());
  result.terminal_pairs.assign(
      context.committed_terminal_pairs().begin(),
      context.committed_terminal_pairs().end());
  result.pending_blocks.assign(
      context.pending_blocks().begin(), context.pending_blocks().end());
  std::size_t negative_witness_count = 0U;
  for (const auto& receipt : result.prune_receipts) {
    negative_witness_count += receipt.witness_node_count;
  }

  if (context.complete()) {
    auto authority = std::move(context).seal();
    result.audit = map_audit(
        request,
        authority.audit(),
        used,
        exact_witness_predicate_count,
        negative_witness_count,
        false,
        page_audit);
    result.status = ExactPairBlockTransactionalFrontierResidentCudaStatus::
        non_authoritative_host_fake_terminal;
  } else {
    result.audit = map_audit(
        request,
        context.audit(),
        used,
        exact_witness_predicate_count,
        negative_witness_count,
        capacity_rollback,
        page_audit);
    result.status = ExactPairBlockTransactionalFrontierResidentCudaStatus::
        capacity_exhausted_wave_rolled_back;
  }
  result.source_identity_authenticated = true;
  return result;
}

}  // namespace morsehgp3d::gpu::detail
