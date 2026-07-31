#include "morsehgp3d/gpu/exact_pair_block_transactional_frontier_cuda.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/exact_block_rank_prune_receipt.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::ExactPairBlockAuthority;
using morsehgp3d::gpu::ExactPairBlockAuthorityKind;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierCudaAudit;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierCudaConfig;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierCudaContext;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierCudaProposal;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierCudaProposalKind;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierCudaTerminalAuthority;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierCudaWaveDecision;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierCudaWaveStart;
using morsehgp3d::hierarchy::ExactBlockRankPruneStatus;
using morsehgp3d::hierarchy::certify_exact_block_rank_prune_receipt;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

template <typename Index, typename Cloud>
concept CanStartTransactionalFrontier = requires(
    Index&& index,
    Cloud&& cloud) {
  ExactPairBlockTransactionalFrontierCudaContext::start(
      std::forward<Index>(index),
      std::forward<Cloud>(cloud),
      ExactPairBlockTransactionalFrontierCudaConfig{});
};

static_assert(
    !std::is_copy_constructible_v<
        ExactPairBlockTransactionalFrontierCudaContext>);
static_assert(
    std::is_nothrow_move_constructible_v<
        ExactPairBlockTransactionalFrontierCudaContext>);
static_assert(
    !std::is_copy_constructible_v<
        ExactPairBlockTransactionalFrontierCudaTerminalAuthority>);
static_assert(
    std::is_nothrow_move_constructible_v<
        ExactPairBlockTransactionalFrontierCudaTerminalAuthority>);
static_assert(
    CanStartTransactionalFrontier<MortonLbvhIndex&, CanonicalPointCloud&>);
static_assert(
    !CanStartTransactionalFrontier<MortonLbvhIndex, CanonicalPointCloud&>);
static_assert(
    !CanStartTransactionalFrontier<
        const MortonLbvhIndex,
        CanonicalPointCloud&>);
static_assert(
    !CanStartTransactionalFrontier<MortonLbvhIndex&, CanonicalPointCloud>);
static_assert(
    !CanStartTransactionalFrontier<
        MortonLbvhIndex&,
        const CanonicalPointCloud>);

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] CanonicalPointCloud make_line_cloud(
    std::size_t point_count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(point_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    points.push_back(CertifiedPoint3::from_binary64(
        static_cast<double>(point_index), 0.0, 0.0));
  }
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] bool mass_is_conserved(
    const ExactPairBlockTransactionalFrontierCudaAudit& audit) {
  return audit.pending_unordered_pair_mass +
          audit.inflight_unordered_pair_mass +
          audit.pruned_unordered_pair_mass +
          audit.terminal_unordered_pair_mass ==
      audit.unordered_pair_universe_mass;
}

[[nodiscard]] std::vector<ExactPairBlockAuthority> copy_pending(
    const ExactPairBlockTransactionalFrontierCudaContext& context) {
  const std::span<const ExactPairBlockAuthority> pending =
      context.pending_blocks();
  return {pending.begin(), pending.end()};
}

[[nodiscard]] std::vector<ExactPairBlockTransactionalFrontierCudaProposal>
open_proposals(
    std::span<const ExactPairBlockAuthority> sources,
    std::uint64_t& transaction_id) {
  std::vector<ExactPairBlockTransactionalFrontierCudaProposal> proposals;
  proposals.reserve(sources.size());
  for (const ExactPairBlockAuthority& source : sources) {
    ExactPairBlockTransactionalFrontierCudaProposal proposal;
    proposal.transaction_id = transaction_id;
    ++transaction_id;
    proposal.source = source;
    proposal.kind =
        ExactPairBlockTransactionalFrontierCudaProposalKind::open;
    proposals.push_back(proposal);
  }
  return proposals;
}

[[nodiscard]] std::size_t node_for_range(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t leaf_begin,
    std::size_t leaf_end) {
  for (std::size_t node_index = 0U;
       node_index < index.build_counters().node_count;
       ++node_index) {
    const auto probe = certify_exact_block_rank_prune_receipt(
        index,
        cloud,
        node_index,
        node_index,
        std::span<const std::size_t>{},
        2U);
    if (probe.status() ==
            ExactBlockRankPruneStatus::rejected_support_overlap &&
        probe.audit().support_nodes[0].leaf_begin == leaf_begin &&
        probe.audit().support_nodes[0].leaf_end == leaf_end) {
      return node_index;
    }
  }
  throw std::runtime_error("the requested exact LBVH range was not found");
}

[[nodiscard]] ExactPairBlockTransactionalFrontierCudaTerminalAuthority
open_every_pair(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank) {
  auto context = ExactPairBlockTransactionalFrontierCudaContext::start(
      index,
      cloud,
      ExactPairBlockTransactionalFrontierCudaConfig{
          maximum_closed_rank, 16U, 4U, 16U});
  std::uint64_t transaction_id = UINT64_C(1);
  std::size_t wave_count = 0U;
  while (!context.complete()) {
    require(++wave_count < 64U, "the complete frontier did not progress");
    require(
        context.begin_wave() ==
            ExactPairBlockTransactionalFrontierCudaWaveStart::wave_ready,
        "a nonterminal complete-frontier wave did not open");
    require(
        context.audit().pending_unordered_pair_mass == 0U &&
            context.audit().inflight_unordered_pair_mass != 0U &&
            mass_is_conserved(context.audit()),
        "begin_wave did not transfer pending mass to in-flight mass");
    auto proposals =
        open_proposals(context.inflight_blocks(), transaction_id);
    const auto result = context.commit_wave(proposals);
    require(
        result.committed() && result.source_wave_restored_on_rejection == false,
        "an adequately bounded native wave did not commit");
    require(
        context.audit().inflight_unordered_pair_mass == 0U &&
            mass_is_conserved(context.audit()),
        "a native commit lost the four-way mass partition");
  }
  require(
      context.begin_wave() ==
          ExactPairBlockTransactionalFrontierCudaWaveStart::frontier_complete,
      "a closed frontier did not reject a new wave as complete");
  return std::move(context).seal();
}

void test_exhaustive_k10_terminal_authority() {
  const CanonicalPointCloud cloud = make_line_cloud(16U);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  auto authority = open_every_pair(index, cloud, 11U);

  require(
      authority.ready() && authority.validated_for(index, cloud),
      "the exhaustive K=10 final cut did not yield a replayable authority");
  const auto& audit = authority.audit();
  require(
      audit.unordered_pair_universe_mass == 120U &&
          audit.pending_unordered_pair_mass == 0U &&
          audit.inflight_unordered_pair_mass == 0U &&
          audit.pruned_unordered_pair_mass == 0U &&
          audit.terminal_unordered_pair_mass == 120U &&
          authority.prune_receipts().empty() &&
          authority.terminal_pairs().size() == 120U &&
          mass_is_conserved(audit),
      "the exhaustive final cut did not contain each of 120 pairs once");

  std::set<std::array<PointId, 2U>> observed_pairs;
  for (const auto& terminal : authority.terminal_pairs()) {
    require(
        terminal.first_node.leaf_count() == 1U &&
            terminal.second_node.leaf_count() == 1U &&
            terminal.point_ids[0] < terminal.point_ids[1] &&
            observed_pairs.insert(terminal.point_ids).second,
        "the terminal stream contains an invalid or duplicate leaf pair");
  }
  require(
      observed_pairs.size() == 120U && audit.wave_commit_count > 1U &&
          audit.double_buffer_swap_count == audit.wave_commit_count &&
          audit.logical_double_buffer_frontier_exercised &&
          audit.global_pair_coverage_closed &&
          audit.terminal_authority_sealed && audit.final_cut_digest != 0U,
      "the final authority did not close the double-buffered partition");
  require(
      audit.fixed_linear_capacities_validated &&
          audit.compact_rank_and_witness_bounds_validated &&
          audit.every_wave_source_matched_positionally &&
          audit.unique_transaction_ids_validated &&
          audit.native_split_partition_validated &&
          audit.pairwise_disjoint_support_products_validated &&
          audit.transactional_mass_conservation_validated &&
          !audit.cuda_execution_performed &&
          !audit.pair_catalog_complete_claimed &&
          !audit.hierarchy_or_tree_claimed && !audit.slo_claimed &&
          !audit.global_pair_matrix_materialized &&
          !audit.ordinary_or_higher_order_delaunay_materialized &&
          !audit.global_facet_coface_or_incidence_arena_materialized &&
          !audit.public_status_claimed,
      "the host contract either lost a proof gate or overclaimed scope");

  const CanonicalPointCloud other_cloud = make_line_cloud(16U);
  const MortonLbvhIndex other_index = MortonLbvhIndex::build(other_cloud);
  require(
      !authority.validated_for(other_index, other_cloud),
      "a process-local authority validated against a foreign LBVH identity");
}

void test_exact_k10_prune_and_fail_open_share_one_partition() {
  const CanonicalPointCloud cloud = make_line_cloud(16U);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::size_t first_support = node_for_range(index, cloud, 0U, 2U);
  const std::size_t second_support = node_for_range(index, cloud, 12U, 14U);
  std::array<std::size_t, 10U> witnesses{};
  for (std::size_t witness_index = 0U;
       witness_index < witnesses.size();
       ++witness_index) {
    witnesses[witness_index] =
        node_for_range(index, cloud, witness_index + 2U, witness_index + 3U);
  }

  auto context = ExactPairBlockTransactionalFrontierCudaContext::start(
      index,
      cloud,
      ExactPairBlockTransactionalFrontierCudaConfig{11U, 16U, 4U, 16U});
  std::uint64_t transaction_id = UINT64_C(1000);
  bool prune_submitted = false;
  bool fail_open_submitted = false;
  std::size_t wave_count = 0U;
  while (!context.complete()) {
    require(++wave_count < 64U, "the pruned frontier did not progress");
    require(
        context.begin_wave() ==
            ExactPairBlockTransactionalFrontierCudaWaveStart::wave_ready,
        "the pruned frontier could not begin its next wave");
    std::vector<ExactPairBlockTransactionalFrontierCudaProposal> proposals;
    proposals.reserve(context.inflight_blocks().size());
    for (const ExactPairBlockAuthority& source : context.inflight_blocks()) {
      ExactPairBlockTransactionalFrontierCudaProposal proposal;
      proposal.transaction_id = transaction_id;
      ++transaction_id;
      proposal.source = source;
      proposal.kind =
          ExactPairBlockTransactionalFrontierCudaProposalKind::open;
      if (!prune_submitted &&
          source.kind == ExactPairBlockAuthorityKind::cross &&
          source.first.node_index == first_support &&
          source.second.node_index == second_support) {
        proposal.kind = ExactPairBlockTransactionalFrontierCudaProposalKind::
            try_exact_prune_else_open;
        proposal.witness_node_count = witnesses.size();
        proposal.witness_node_indices = witnesses;
        prune_submitted = true;
      } else if (!fail_open_submitted &&
                 source.kind == ExactPairBlockAuthorityKind::cross) {
        proposal.kind = ExactPairBlockTransactionalFrontierCudaProposalKind::
            try_exact_prune_else_open;
        fail_open_submitted = true;
      }
      proposals.push_back(proposal);
    }
    const auto result = context.commit_wave(proposals);
    require(result.committed(), "a prune/fail-open wave did not commit");
    require(
        mass_is_conserved(context.audit()),
        "a prune/fail-open wave lost pair mass");
  }

  require(
      prune_submitted && fail_open_submitted,
      "the native partition did not expose both planned transitions");
  auto authority = std::move(context).seal();
  require(
      authority.ready() && authority.validated_for(index, cloud),
      "the mixed K=10 partition did not replay exactly");
  const auto& audit = authority.audit();
  require(
      audit.exact_prune_attempt_count == 2U &&
          audit.certified_prune_count == 1U &&
          audit.exact_prune_fail_open_count == 1U &&
          audit.prune_receipt_count == 1U &&
          audit.pruned_unordered_pair_mass == 4U &&
          audit.terminal_pair_count == 116U &&
          audit.terminal_unordered_pair_mass == 116U &&
          authority.prune_receipts().size() == 1U &&
          authority.terminal_pairs().size() == 116U &&
          mass_is_conserved(audit),
      "prune, fail-open and terminal transitions did not partition 120 pairs");
  const auto& receipt = authority.prune_receipts().front();
  require(
      receipt.maximum_closed_rank == 11U &&
          receipt.witness_node_count == 10U &&
          receipt.certified_witness_point_count == 10U &&
          receipt.unordered_pair_mass == 4U &&
          receipt.support_block.first.node_index == first_support &&
          receipt.support_block.second.node_index == second_support,
      "the compact K=10 prune receipt lost its exact replay material");
}

void test_invalid_and_capacity_rejections_restore_complete_wave() {
  const CanonicalPointCloud cloud = make_line_cloud(16U);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  auto context = ExactPairBlockTransactionalFrontierCudaContext::start(
      index,
      cloud,
      ExactPairBlockTransactionalFrontierCudaConfig{6U, 1U, 1U, 16U});
  std::uint64_t transaction_id = UINT64_C(5000);

  require(
      context.begin_wave() ==
          ExactPairBlockTransactionalFrontierCudaWaveStart::wave_ready,
      "the rollback fixture could not start its root wave");
  auto root_proposals =
      open_proposals(context.inflight_blocks(), transaction_id);
  require(
      context.commit_wave(root_proposals).committed(),
      "the rollback fixture could not open its root block");

  const std::vector<ExactPairBlockAuthority> before_invalid =
      copy_pending(context);
  const auto invalid_audit = context.audit();
  require(before_invalid.size() > 1U, "the duplicate-id fixture is singular");
  require(
      context.begin_wave() ==
          ExactPairBlockTransactionalFrontierCudaWaveStart::wave_ready,
      "the duplicate-id wave did not open");
  auto duplicate_proposals =
      open_proposals(context.inflight_blocks(), transaction_id);
  for (auto& proposal : duplicate_proposals) {
    proposal.transaction_id = UINT64_C(7);
  }
  const auto invalid_result = context.commit_wave(duplicate_proposals);
  require(
      invalid_result.decision ==
              ExactPairBlockTransactionalFrontierCudaWaveDecision::
                  rolled_back_invalid_proposal &&
          !invalid_result.scientific_state_mutated &&
          invalid_result.source_wave_restored_on_rejection &&
          copy_pending(context) == before_invalid &&
          context.audit().pending_unordered_pair_mass ==
              invalid_audit.pending_unordered_pair_mass &&
          context.audit().pruned_unordered_pair_mass ==
              invalid_audit.pruned_unordered_pair_mass &&
          context.audit().terminal_unordered_pair_mass ==
              invalid_audit.terminal_unordered_pair_mass &&
          context.audit().inflight_unordered_pair_mass == 0U &&
          mass_is_conserved(context.audit()),
      "an invalid transaction batch mutated or lost its source wave");

  bool capacity_rolled_back = false;
  for (std::size_t attempt = 0U; attempt < 32U; ++attempt) {
    const std::vector<ExactPairBlockAuthority> source_wave =
        copy_pending(context);
    const auto before_wave = context.audit();
    require(
        context.begin_wave() ==
            ExactPairBlockTransactionalFrontierCudaWaveStart::wave_ready,
        "the bounded fixture completed before exercising capacity rollback");
    auto proposals =
        open_proposals(context.inflight_blocks(), transaction_id);
    const auto result = context.commit_wave(proposals);
    if (result.decision ==
        ExactPairBlockTransactionalFrontierCudaWaveDecision::
            rolled_back_capacity_exhausted) {
      require(
          !result.scientific_state_mutated &&
              result.source_wave_restored_on_rejection &&
              copy_pending(context) == source_wave &&
              context.audit().pending_unordered_pair_mass ==
                  before_wave.pending_unordered_pair_mass &&
              context.audit().pruned_unordered_pair_mass ==
                  before_wave.pruned_unordered_pair_mass &&
              context.audit().terminal_unordered_pair_mass ==
                  before_wave.terminal_unordered_pair_mass &&
              context.audit().inflight_unordered_pair_mass == 0U &&
              mass_is_conserved(context.audit()),
          "capacity exhaustion did not atomically restore the source wave");
      capacity_rolled_back = true;
      break;
    }
    require(result.committed(), "the bounded fixture failed unexpectedly");
  }
  require(
      capacity_rolled_back && context.audit().wave_rollback_count >= 2U &&
          context.audit().rejected_wave_restored_without_scientific_mutation,
      "the fixed linear frontier never exercised capacity fail-closed");

  const std::vector<ExactPairBlockAuthority> before_explicit =
      copy_pending(context);
  require(
      context.begin_wave() ==
              ExactPairBlockTransactionalFrontierCudaWaveStart::wave_ready &&
          context.rollback_wave() && copy_pending(context) == before_explicit &&
          mass_is_conserved(context.audit()) &&
          context.audit().explicit_rollback_count == 1U,
      "explicit rollback did not restore the complete source wave");
}

}  // namespace

int main() {
  try {
    test_exhaustive_k10_terminal_authority();
    test_exact_k10_prune_and_fail_open_share_one_partition();
    test_invalid_and_capacity_rejections_restore_complete_wave();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout
      << "transactional pair-block frontier host-contract tests passed\n";
  return 0;
}
