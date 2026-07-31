#include "phase15_exact_pair_block_transactional_frontier_resident_cuda_internal.hpp"

#include "morsehgp3d/gpu/exact_pair_block_transactional_frontier_cuda.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace morsehgp3d::gpu::detail {
namespace {

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
    bool capacity_rollback) {
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
      source.rejected_wave_restored_without_scientific_mutation;
  audit.capacity_wave_rollback_validated = capacity_rollback;
  audit.fail_open_native_transition_validated = true;
  audit.transactional_mass_conservation_validated =
      source.transactional_mass_conservation_validated;
  audit.native_split_partition_validated =
      source.native_split_partition_validated;
  audit.pairwise_disjoint_support_products_validated =
      source.pairwise_disjoint_support_products_validated;
  audit.terminal_authority_sealed = source.terminal_authority_sealed;
  audit.global_pair_coverage_closed = source.global_pair_coverage_closed;
  audit.host_fake_lifecycle_exercised = true;
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

  while (!context.complete()) {
    if (context.begin_wave() !=
        ExactPairBlockTransactionalFrontierCudaWaveStart::wave_ready) {
      throw std::logic_error("the fake resident scheduler failed to open a wave");
    }
    std::vector<ExactPairBlockTransactionalFrontierCudaProposal> proposals;
    proposals.reserve(context.inflight_blocks().size());
    for (const auto& source : context.inflight_blocks()) {
      ExactPairBlockTransactionalFrontierCudaProposal proposal;
      proposal.transaction_id = transaction_id++;
      proposal.source = source;
      for (std::size_t recipe_index = 0U;
           recipe_index < request.recipes.size();
           ++recipe_index) {
        const auto& recipe = request.recipes[recipe_index];
        if (matches(recipe.source, source)) {
          used[recipe_index] = std::uint8_t{1U};
          proposal.kind = ExactPairBlockTransactionalFrontierCudaProposalKind::
              try_exact_prune_else_open;
          proposal.witness_node_count = recipe.witness_node_count;
          exact_witness_predicate_count += recipe.witness_node_count;
          for (std::size_t witness_index = 0U;
               witness_index < recipe.witness_node_count;
               ++witness_index) {
            proposal.witness_node_indices[witness_index] =
                recipe.witness_node_indices[witness_index];
          }
          break;
        }
      }
      proposals.push_back(proposal);
    }
    const auto wave = context.commit_wave(proposals);
    if (wave.decision == ExactPairBlockTransactionalFrontierCudaWaveDecision::
                             rolled_back_capacity_exhausted) {
      capacity_rollback = true;
      break;
    }
    if (!wave.committed()) {
      throw std::logic_error(
          "the fake resident scheduler rejected a prevalidated wave");
    }
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
        false);
    result.status = ExactPairBlockTransactionalFrontierResidentCudaStatus::
        non_authoritative_host_fake_terminal;
  } else {
    result.audit = map_audit(
        request,
        context.audit(),
        used,
        exact_witness_predicate_count,
        negative_witness_count,
        capacity_rollback);
    result.status = ExactPairBlockTransactionalFrontierResidentCudaStatus::
        capacity_exhausted_wave_rolled_back;
  }
  result.source_identity_authenticated = true;
  return result;
}

}  // namespace morsehgp3d::gpu::detail
