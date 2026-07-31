#include "morsehgp3d/gpu/exact_pair_block_transactional_frontier_cuda.hpp"

#include "morsehgp3d/hierarchy/exact_block_rank_prune_receipt.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

inline constexpr std::uint64_t digest_offset =
    UINT64_C(1469598103934665603);
inline constexpr std::uint64_t digest_prime = UINT64_C(1099511628211);

[[nodiscard]] bool checked_add(
    std::size_t first,
    std::size_t second,
    std::size_t& sum) noexcept {
  if (second > std::numeric_limits<std::size_t>::max() - first) {
    return false;
  }
  sum = first + second;
  return true;
}

[[nodiscard]] bool checked_product(
    std::size_t first,
    std::size_t second,
    std::size_t& product) noexcept {
  if (first != 0U &&
      second > std::numeric_limits<std::size_t>::max() / first) {
    return false;
  }
  product = first * second;
  return true;
}

[[nodiscard]] bool checked_unordered_pair_count(
    std::size_t point_count,
    std::size_t& pair_count) noexcept {
  if (point_count < 2U) {
    pair_count = 0U;
    return true;
  }
  const std::size_t first =
      point_count % 2U == 0U ? point_count / 2U : point_count;
  const std::size_t second =
      point_count % 2U == 0U ? point_count - 1U
                             : (point_count - 1U) / 2U;
  return checked_product(first, second, pair_count);
}

[[nodiscard]] bool ranges_are_disjoint(
    const ExactPairBlockNodeAuthority& first,
    const ExactPairBlockNodeAuthority& second) noexcept {
  return first.leaf_end <= second.leaf_begin ||
      second.leaf_end <= first.leaf_begin;
}

[[nodiscard]] std::uint64_t digest_word(
    std::uint64_t digest,
    std::uint64_t word) noexcept {
  for (unsigned int byte_index = 0U; byte_index < 8U; ++byte_index) {
    digest ^= word & UINT64_C(0xff);
    digest *= digest_prime;
    word >>= 8U;
  }
  return digest;
}

[[nodiscard]] std::uint64_t final_cut_digest(
    std::size_t point_count,
    std::size_t maximum_closed_rank,
    std::span<const ExactPairBlockTransactionalFrontierCudaPruneReceipt>
        prune_receipts,
    std::span<const ExactPairBlockTransactionalFrontierCudaTerminalPair>
        terminal_pairs) noexcept {
  std::uint64_t digest =
      digest_word(digest_offset, UINT64_C(0x4d48475050424631));
  digest = digest_word(digest, static_cast<std::uint64_t>(point_count));
  digest = digest_word(
      digest, static_cast<std::uint64_t>(maximum_closed_rank));
  digest = digest_word(
      digest, static_cast<std::uint64_t>(prune_receipts.size()));
  for (const auto& receipt : prune_receipts) {
    const auto add_node = [&digest](const ExactPairBlockNodeAuthority& node) {
      digest = digest_word(
          digest, static_cast<std::uint64_t>(node.node_index));
      digest = digest_word(
          digest, static_cast<std::uint64_t>(node.leaf_begin));
      digest = digest_word(
          digest, static_cast<std::uint64_t>(node.leaf_end));
    };
    add_node(receipt.support_block.first);
    add_node(receipt.support_block.second);
    digest = digest_word(
        digest, static_cast<std::uint64_t>(receipt.witness_node_count));
    for (std::size_t witness_index = 0U;
         witness_index < receipt.witness_node_count;
         ++witness_index) {
      add_node(receipt.witness_nodes[witness_index]);
    }
    digest = digest_word(
        digest, static_cast<std::uint64_t>(receipt.unordered_pair_mass));
  }
  digest = digest_word(
      digest, static_cast<std::uint64_t>(terminal_pairs.size()));
  for (const auto& terminal : terminal_pairs) {
    digest = digest_word(
        digest, static_cast<std::uint64_t>(terminal.first_node.node_index));
    digest = digest_word(
        digest, static_cast<std::uint64_t>(terminal.second_node.node_index));
    digest = digest_word(digest, terminal.point_ids[0]);
    digest = digest_word(digest, terminal.point_ids[1]);
  }
  return digest;
}

[[nodiscard]] bool audit_mass_is_conserved(
    const ExactPairBlockTransactionalFrontierCudaAudit& audit) noexcept {
  std::size_t observed = 0U;
  return checked_add(
             audit.pending_unordered_pair_mass,
             audit.inflight_unordered_pair_mass,
             observed) &&
      checked_add(observed, audit.pruned_unordered_pair_mass, observed) &&
      checked_add(observed, audit.terminal_unordered_pair_mass, observed) &&
      observed == audit.unordered_pair_universe_mass;
}

}  // namespace

ExactPairBlockTransactionalFrontierCudaTerminalAuthority::
    ExactPairBlockTransactionalFrontierCudaTerminalAuthority(
        ExactPairBlockTransactionalFrontierCudaAudit audit,
        std::vector<ExactPairBlockTransactionalFrontierCudaPruneReceipt>&&
            prune_receipts,
        std::vector<ExactPairBlockTransactionalFrontierCudaTerminalPair>&&
            terminal_pairs,
        std::shared_ptr<const void> lbvh_identity) noexcept
    : audit_(audit),
      prune_receipts_(std::move(prune_receipts)),
      terminal_pairs_(std::move(terminal_pairs)),
      lbvh_identity_(std::move(lbvh_identity)) {}

bool ExactPairBlockTransactionalFrontierCudaTerminalAuthority::ready()
    const noexcept {
  return lbvh_identity_ != nullptr && audit_.global_pair_coverage_closed &&
      audit_.terminal_authority_sealed &&
      audit_.transactional_mass_conservation_validated &&
      audit_.pairwise_disjoint_support_products_validated &&
      audit_.pending_unordered_pair_mass == 0U &&
      audit_.inflight_unordered_pair_mass == 0U &&
      audit_.prune_receipt_count == prune_receipts_.size() &&
      audit_.terminal_pair_count == terminal_pairs_.size() &&
      audit_.terminal_unordered_pair_mass == terminal_pairs_.size() &&
      !audit_.cuda_execution_performed &&
      !audit_.pair_catalog_complete_claimed &&
      !audit_.hierarchy_or_tree_claimed && !audit_.slo_claimed &&
      !audit_.global_pair_matrix_materialized &&
      !audit_.ordinary_or_higher_order_delaunay_materialized &&
      !audit_.global_facet_coface_or_incidence_arena_materialized &&
      !audit_.durable_receipt_claimed && !audit_.public_status_claimed;
}

bool ExactPairBlockTransactionalFrontierCudaTerminalAuthority::validated_for(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud) const {
  if (!ready() || !index.validated_for(cloud) ||
      lbvh_identity_.get() != index.identity_.get() ||
      audit_.point_count != cloud.size() ||
      audit_.certified_node_count != index.nodes_.size() ||
      audit_.maximum_closed_rank < 2U ||
      audit_.maximum_closed_rank >
          exact_pair_block_transactional_frontier_cuda_maximum_closed_rank ||
      audit_.required_witness_point_count !=
          audit_.maximum_closed_rank - 1U ||
      !audit_mass_is_conserved(audit_)) {
    return false;
  }

  std::size_t observed_pruned_mass = 0U;
  try {
    for (const auto& receipt : prune_receipts_) {
      if (receipt.maximum_closed_rank != audit_.maximum_closed_rank ||
          receipt.witness_node_count == 0U ||
          receipt.witness_node_count > receipt.witness_nodes.size() ||
          receipt.unordered_pair_mass !=
              receipt.support_block.unordered_pair_mass) {
        return false;
      }
      std::array<
          std::size_t,
          exact_pair_block_transactional_frontier_cuda_maximum_witness_node_count>
          witness_indices{};
      for (std::size_t witness_index = 0U;
           witness_index < receipt.witness_node_count;
           ++witness_index) {
        witness_indices[witness_index] =
            receipt.witness_nodes[witness_index].node_index;
      }
      hierarchy::ExactBlockRankPruneResult replay =
          hierarchy::certify_exact_block_rank_prune_receipt(
              index,
              cloud,
              receipt.support_block.first.node_index,
              receipt.support_block.second.node_index,
              std::span<const std::size_t>{
                  witness_indices.data(), receipt.witness_node_count},
              receipt.maximum_closed_rank);
      if (!replay.certified() || replay.receipt() == nullptr ||
          replay.receipt()->unordered_pair_mass() !=
              receipt.unordered_pair_mass ||
          replay.receipt()->certified_witness_point_count() !=
              receipt.certified_witness_point_count ||
          !checked_add(
              observed_pruned_mass,
              receipt.unordered_pair_mass,
              observed_pruned_mass)) {
        return false;
      }
    }
  } catch (const std::exception&) {
    return false;
  }

  for (const auto& terminal : terminal_pairs_) {
    if (terminal.first_node.node_index >= index.nodes_.size() ||
        terminal.second_node.node_index >= index.nodes_.size()) {
      return false;
    }
    const auto& first = index.nodes_[terminal.first_node.node_index];
    const auto& second = index.nodes_[terminal.second_node.node_index];
    if (!first.is_leaf() || !second.is_leaf() ||
        first.leaf_begin != terminal.first_node.leaf_begin ||
        first.leaf_end != terminal.first_node.leaf_end ||
        second.leaf_begin != terminal.second_node.leaf_begin ||
        second.leaf_end != terminal.second_node.leaf_end ||
        terminal.first_node.leaf_count() != 1U ||
        terminal.second_node.leaf_count() != 1U ||
        terminal.first_node.leaf_end > terminal.second_node.leaf_begin ||
        terminal.first_node.leaf_begin >= index.leaves_.size() ||
        terminal.second_node.leaf_begin >= index.leaves_.size()) {
      return false;
    }
    spatial::PointId first_id =
        index.leaves_[terminal.first_node.leaf_begin].point_id;
    spatial::PointId second_id =
        index.leaves_[terminal.second_node.leaf_begin].point_id;
    if (second_id < first_id) {
      std::swap(first_id, second_id);
    }
    if (terminal.point_ids !=
        std::array<spatial::PointId, 2U>{first_id, second_id}) {
      return false;
    }
  }

  std::size_t observed_final_mass = 0U;
  return observed_pruned_mass == audit_.pruned_unordered_pair_mass &&
      checked_add(
          observed_pruned_mass,
          terminal_pairs_.size(),
          observed_final_mass) &&
      observed_final_mass == audit_.unordered_pair_universe_mass &&
      audit_.final_cut_digest ==
          final_cut_digest(
              audit_.point_count,
              audit_.maximum_closed_rank,
              prune_receipts_,
              terminal_pairs_);
}

ExactPairBlockTransactionalFrontierCudaContext
ExactPairBlockTransactionalFrontierCudaContext::start(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    ExactPairBlockTransactionalFrontierCudaConfig config) {
  return ExactPairBlockTransactionalFrontierCudaContext(
      index, cloud, config, PrivateConstructionTag{});
}

ExactPairBlockTransactionalFrontierCudaContext::
    ExactPairBlockTransactionalFrontierCudaContext(
        const spatial::MortonLbvhIndex& index,
        const spatial::CanonicalPointCloud& cloud,
        ExactPairBlockTransactionalFrontierCudaConfig config,
        PrivateConstructionTag)
    : config_(config),
      index_(&index),
      cloud_(&cloud),
      lbvh_identity_(index.identity_) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "a transactional pair-block frontier requires its cloud's exact "
        "LBVH");
  }
  if (config.maximum_closed_rank < 2U ||
      config.maximum_closed_rank >
          exact_pair_block_transactional_frontier_cuda_maximum_closed_rank) {
    throw std::out_of_range(
        "a transactional pair-block maximum closed rank must be in [2, 11]");
  }
  const auto factor_is_valid = [](std::size_t factor) {
    return factor != 0U &&
        factor <=
            exact_pair_block_transactional_frontier_cuda_maximum_capacity_per_point;
  };
  if (!factor_is_valid(config.pending_block_capacity_per_point) ||
      !factor_is_valid(config.prune_receipt_capacity_per_point) ||
      !factor_is_valid(config.terminal_pair_capacity_per_point)) {
    throw std::out_of_range(
        "transactional pair-block capacity factors must be in [1, 64]");
  }
  if (cloud.size() == 0U || index.nodes_.empty() ||
      index.root_index_ >= index.nodes_.size() ||
      !checked_product(
          cloud.size(),
          config.pending_block_capacity_per_point,
          pending_block_capacity_) ||
      !checked_product(
          cloud.size(),
          config.prune_receipt_capacity_per_point,
          prune_receipt_capacity_) ||
      !checked_product(
          cloud.size(),
          config.terminal_pair_capacity_per_point,
          terminal_pair_capacity_)) {
    throw std::overflow_error(
        "transactional pair-block fixed capacities are not representable");
  }

  block_buffers_[0].reserve(pending_block_capacity_);
  block_buffers_[1].reserve(pending_block_capacity_);
  prune_receipts_.reserve(prune_receipt_capacity_);
  terminal_pairs_.reserve(terminal_pair_capacity_);

  audit_.point_count = cloud.size();
  audit_.certified_node_count = index.nodes_.size();
  audit_.maximum_closed_rank = config.maximum_closed_rank;
  audit_.required_witness_point_count = config.maximum_closed_rank - 1U;
  audit_.pending_block_capacity = pending_block_capacity_;
  audit_.prune_receipt_capacity = prune_receipt_capacity_;
  audit_.terminal_pair_capacity = terminal_pair_capacity_;
  audit_.fixed_linear_capacities_validated = true;
  audit_.compact_rank_and_witness_bounds_validated = true;
  audit_.every_wave_source_matched_positionally = true;
  audit_.unique_transaction_ids_validated = true;
  audit_.native_split_partition_validated = true;
  audit_.pairwise_disjoint_support_products_validated = true;
  audit_.rejected_wave_restored_without_scientific_mutation = true;

  if (!checked_unordered_pair_count(
          cloud.size(), audit_.unordered_pair_universe_mass)) {
    throw std::overflow_error(
        "the transactional pair-block universe mass overflows size_t");
  }
  pending_unordered_pair_mass_ = audit_.unordered_pair_universe_mass;
  if (pending_unordered_pair_mass_ != 0U) {
    const auto& root = index.nodes_[index.root_index_];
    const ExactPairBlockNodeAuthority root_authority{
        index.root_index_, root.leaf_begin, root.leaf_end};
    block_buffers_[0].push_back(ExactPairBlockAuthority{
        ExactPairBlockAuthorityKind::diagonal,
        root_authority,
        root_authority,
        pending_unordered_pair_mass_});
  } else {
    complete_ = true;
    audit_.global_pair_coverage_closed = true;
  }
  refresh_audit();
  verify_internal_mass();
}

ExactPairBlockTransactionalFrontierCudaContext::
    ExactPairBlockTransactionalFrontierCudaContext(
        ExactPairBlockTransactionalFrontierCudaContext&& other) noexcept
    : config_(other.config_),
      audit_(other.audit_),
      block_buffers_(std::move(other.block_buffers_)),
      active_buffer_index_(other.active_buffer_index_),
      prune_receipts_(std::move(other.prune_receipts_)),
      terminal_pairs_(std::move(other.terminal_pairs_)),
      index_(other.index_),
      cloud_(other.cloud_),
      lbvh_identity_(std::move(other.lbvh_identity_)),
      pending_unordered_pair_mass_(other.pending_unordered_pair_mass_),
      inflight_unordered_pair_mass_(other.inflight_unordered_pair_mass_),
      inflight_block_count_(other.inflight_block_count_),
      pruned_unordered_pair_mass_(other.pruned_unordered_pair_mass_),
      terminal_unordered_pair_mass_(other.terminal_unordered_pair_mass_),
      pending_block_capacity_(other.pending_block_capacity_),
      prune_receipt_capacity_(other.prune_receipt_capacity_),
      terminal_pair_capacity_(other.terminal_pair_capacity_),
      wave_open_(other.wave_open_),
      complete_(other.complete_),
      sealed_or_moved_(other.sealed_or_moved_) {
  other.index_ = nullptr;
  other.cloud_ = nullptr;
  other.wave_open_ = false;
  other.inflight_block_count_ = 0U;
  other.complete_ = false;
  other.sealed_or_moved_ = true;
}

bool ExactPairBlockTransactionalFrontierCudaContext::ready() const noexcept {
  return !sealed_or_moved_ && index_ != nullptr && cloud_ != nullptr &&
      lbvh_identity_ != nullptr && index_->validated_for(*cloud_) &&
      lbvh_identity_.get() == index_->identity_.get();
}

bool ExactPairBlockTransactionalFrontierCudaContext::complete()
    const noexcept {
  return ready() && complete_ && !wave_open_ &&
      block_buffers_[active_buffer_index_].empty() &&
      pending_unordered_pair_mass_ == 0U &&
      inflight_unordered_pair_mass_ == 0U && inflight_block_count_ == 0U &&
      audit_.global_pair_coverage_closed;
}

bool ExactPairBlockTransactionalFrontierCudaContext::validated_for(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud) const noexcept {
  return ready() && index_ == &index && cloud_ == &cloud &&
      index.validated_for(cloud) &&
      lbvh_identity_.get() == index.identity_.get();
}

std::span<const ExactPairBlockAuthority>
ExactPairBlockTransactionalFrontierCudaContext::pending_blocks()
    const & noexcept {
  return ready() && !wave_open_
      ? std::span<const ExactPairBlockAuthority>{
            block_buffers_[active_buffer_index_]}
      : std::span<const ExactPairBlockAuthority>{};
}

std::span<const ExactPairBlockAuthority>
ExactPairBlockTransactionalFrontierCudaContext::inflight_blocks()
    const & noexcept {
  return ready() && wave_open_
      ? std::span<const ExactPairBlockAuthority>{
            block_buffers_[active_buffer_index_]}
            .first(inflight_block_count_)
      : std::span<const ExactPairBlockAuthority>{};
}

void ExactPairBlockTransactionalFrontierCudaContext::refresh_audit() {
  audit_.pending_unordered_pair_mass = pending_unordered_pair_mass_;
  audit_.inflight_unordered_pair_mass = inflight_unordered_pair_mass_;
  audit_.pruned_unordered_pair_mass = pruned_unordered_pair_mass_;
  audit_.terminal_unordered_pair_mass = terminal_unordered_pair_mass_;
  audit_.pending_block_count = wave_open_
      ? block_buffers_[active_buffer_index_].size() - inflight_block_count_
      : block_buffers_[active_buffer_index_].size();
  audit_.inflight_block_count = wave_open_ ? inflight_block_count_ : 0U;
  audit_.prune_receipt_count = prune_receipts_.size();
  audit_.terminal_pair_count = terminal_pairs_.size();
  audit_.maximum_pending_block_count = std::max(
      audit_.maximum_pending_block_count, audit_.pending_block_count);
  audit_.maximum_inflight_block_count = std::max(
      audit_.maximum_inflight_block_count, audit_.inflight_block_count);
  audit_.transactional_mass_conservation_validated =
      audit_mass_is_conserved(audit_);
}

void ExactPairBlockTransactionalFrontierCudaContext::verify_internal_mass()
    const {
  if (!audit_.transactional_mass_conservation_validated ||
      audit_.pending_block_count > pending_block_capacity_ ||
      audit_.inflight_block_count > pending_block_capacity_ ||
      prune_receipts_.size() > prune_receipt_capacity_ ||
      terminal_pairs_.size() > terminal_pair_capacity_ ||
      terminal_unordered_pair_mass_ != terminal_pairs_.size()) {
    throw std::logic_error(
        "the transactional pair-block frontier lost its bounded mass "
        "partition");
  }
}

ExactPairBlockTransactionalFrontierCudaTransitionPreview
ExactPairBlockTransactionalFrontierCudaContext::preview_transition(
    const ExactPairBlockTransactionalFrontierCudaProposal& proposal)
    const & {
  ExactPairBlockTransactionalFrontierCudaTransitionPreview preview;
  if (!ready() || wave_open_ ||
      proposal.witness_node_count > proposal.witness_node_indices.size()) {
    return preview;
  }

  const ExactPairBlockAuthority& source = proposal.source;
  const auto node_is_current = [this](
                                   const ExactPairBlockNodeAuthority& node) {
    if (node.node_index >= index_->nodes_.size()) {
      return false;
    }
    const auto& current = index_->nodes_[node.node_index];
    return node.leaf_begin < node.leaf_end &&
        current.leaf_begin == node.leaf_begin &&
        current.leaf_end == node.leaf_end;
  };
  if (!node_is_current(source.first) ||
      !node_is_current(source.second)) {
    return preview;
  }
  std::size_t expected_source_mass = 0U;
  if (source.kind == ExactPairBlockAuthorityKind::diagonal) {
    if (source.first != source.second ||
        !checked_unordered_pair_count(
            source.first.leaf_count(), expected_source_mass)) {
      return preview;
    }
  } else if (source.first.leaf_end > source.second.leaf_begin ||
             !checked_product(
                 source.first.leaf_count(),
                 source.second.leaf_count(),
                 expected_source_mass)) {
    return preview;
  }
  if (expected_source_mass != source.unordered_pair_mass) {
    return preview;
  }

  switch (proposal.kind) {
    case ExactPairBlockTransactionalFrontierCudaProposalKind::open:
      if (proposal.witness_node_count != 0U) {
        return preview;
      }
      break;
    case ExactPairBlockTransactionalFrontierCudaProposalKind::
        try_exact_prune_else_open:
      if (source.kind != ExactPairBlockAuthorityKind::cross) {
        return preview;
      }
      break;
    default:
      return preview;
  }

  if (proposal.kind ==
      ExactPairBlockTransactionalFrontierCudaProposalKind::
          try_exact_prune_else_open) {
    hierarchy::ExactBlockRankPruneResult prune =
        hierarchy::certify_exact_block_rank_prune_receipt(
            *index_,
            *cloud_,
            source.first.node_index,
            source.second.node_index,
            std::span<const std::size_t>{
                proposal.witness_node_indices.data(),
                proposal.witness_node_count},
            config_.maximum_closed_rank);
    if (prune.certified() && prune.receipt() != nullptr) {
      if (prune.receipt()->witness_nodes().empty() ||
          prune.receipt()->witness_nodes().size() >
              exact_pair_block_transactional_frontier_cuda_maximum_witness_node_count ||
          prune.receipt()->unordered_pair_mass() !=
              source.unordered_pair_mass) {
        preview.status =
            ExactPairBlockTransactionalFrontierCudaTransitionPreviewStatus::
                arithmetic_overflow;
        return preview;
      }
      preview.status =
          ExactPairBlockTransactionalFrontierCudaTransitionPreviewStatus::
              ready;
      preview.prune_receipt_count = 1U;
      preview.pruned_unordered_pair_mass = source.unordered_pair_mass;
      return preview;
    }
  }

  if (source.kind == ExactPairBlockAuthorityKind::diagonal) {
    const auto& parent = index_->nodes_[source.first.node_index];
    if (parent.is_leaf() || parent.left_child >= index_->nodes_.size() ||
        parent.right_child >= index_->nodes_.size() ||
        parent.left_child == parent.right_child) {
      return preview;
    }
    const auto& left = index_->nodes_[parent.left_child];
    const auto& right = index_->nodes_[parent.right_child];
    std::size_t left_mass = 0U;
    std::size_t cross_mass = 0U;
    std::size_t right_mass = 0U;
    std::size_t child_mass = 0U;
    if (!checked_unordered_pair_count(
            left.leaf_end - left.leaf_begin, left_mass) ||
        !checked_product(
            left.leaf_end - left.leaf_begin,
            right.leaf_end - right.leaf_begin,
            cross_mass) ||
        !checked_unordered_pair_count(
            right.leaf_end - right.leaf_begin, right_mass) ||
        !checked_add(left_mass, cross_mass, child_mass) ||
        !checked_add(child_mass, right_mass, child_mass) ||
        child_mass != source.unordered_pair_mass) {
      preview.status =
          ExactPairBlockTransactionalFrontierCudaTransitionPreviewStatus::
              arithmetic_overflow;
      return preview;
    }
    preview.successor_block_count =
        static_cast<std::size_t>(left_mass != 0U) +
        static_cast<std::size_t>(cross_mass != 0U) +
        static_cast<std::size_t>(right_mass != 0U);
    preview.successor_unordered_pair_mass = source.unordered_pair_mass;
    preview.status =
        ExactPairBlockTransactionalFrontierCudaTransitionPreviewStatus::ready;
    return preview;
  }

  const auto& first_node = index_->nodes_[source.first.node_index];
  const auto& second_node = index_->nodes_[source.second.node_index];
  if (first_node.is_leaf() && second_node.is_leaf()) {
    if (source.unordered_pair_mass != 1U) {
      preview.status =
          ExactPairBlockTransactionalFrontierCudaTransitionPreviewStatus::
              arithmetic_overflow;
      return preview;
    }
    preview.terminal_pair_count = 1U;
    preview.terminal_unordered_pair_mass = 1U;
    preview.status =
        ExactPairBlockTransactionalFrontierCudaTransitionPreviewStatus::ready;
    return preview;
  }

  const bool split_second =
      !second_node.is_leaf() &&
      (first_node.is_leaf() ||
       source.second.leaf_count() >= source.first.leaf_count());
  std::size_t first_child_mass = 0U;
  std::size_t second_child_mass = 0U;
  if (split_second) {
    if (second_node.left_child >= index_->nodes_.size() ||
        second_node.right_child >= index_->nodes_.size()) {
      return preview;
    }
    const auto& left = index_->nodes_[second_node.left_child];
    const auto& right = index_->nodes_[second_node.right_child];
    if (!checked_product(
            source.first.leaf_count(),
            left.leaf_end - left.leaf_begin,
            first_child_mass) ||
        !checked_product(
            source.first.leaf_count(),
            right.leaf_end - right.leaf_begin,
            second_child_mass)) {
      preview.status =
          ExactPairBlockTransactionalFrontierCudaTransitionPreviewStatus::
              arithmetic_overflow;
      return preview;
    }
  } else {
    if (first_node.is_leaf() ||
        first_node.left_child >= index_->nodes_.size() ||
        first_node.right_child >= index_->nodes_.size()) {
      return preview;
    }
    const auto& left = index_->nodes_[first_node.left_child];
    const auto& right = index_->nodes_[first_node.right_child];
    if (!checked_product(
            left.leaf_end - left.leaf_begin,
            source.second.leaf_count(),
            first_child_mass) ||
        !checked_product(
            right.leaf_end - right.leaf_begin,
            source.second.leaf_count(),
            second_child_mass)) {
      preview.status =
          ExactPairBlockTransactionalFrontierCudaTransitionPreviewStatus::
              arithmetic_overflow;
      return preview;
    }
  }
  std::size_t child_mass = 0U;
  if (first_child_mass == 0U || second_child_mass == 0U ||
      !checked_add(first_child_mass, second_child_mass, child_mass) ||
      child_mass != source.unordered_pair_mass) {
    preview.status =
        ExactPairBlockTransactionalFrontierCudaTransitionPreviewStatus::
            arithmetic_overflow;
    return preview;
  }
  preview.successor_block_count = 2U;
  preview.successor_unordered_pair_mass = source.unordered_pair_mass;
  preview.status =
      ExactPairBlockTransactionalFrontierCudaTransitionPreviewStatus::ready;
  return preview;
}

ExactPairBlockTransactionalFrontierCudaWaveStart
ExactPairBlockTransactionalFrontierCudaContext::begin_wave() & {
  return begin_wave(block_buffers_[active_buffer_index_].size());
}

ExactPairBlockTransactionalFrontierCudaWaveStart
ExactPairBlockTransactionalFrontierCudaContext::begin_wave(
    std::size_t maximum_source_count) & {
  if (!ready()) {
    throw std::logic_error(
        "a moved or sealed transactional pair-block frontier cannot begin");
  }
  if (wave_open_) {
    throw std::logic_error(
        "a transactional pair-block frontier already has an open wave");
  }
  if (complete()) {
    return ExactPairBlockTransactionalFrontierCudaWaveStart::
        frontier_complete;
  }
  if (block_buffers_[active_buffer_index_].empty() ||
      pending_unordered_pair_mass_ == 0U || maximum_source_count == 0U) {
    throw std::logic_error(
        "a nonterminal transactional pair-block frontier has no pending "
        "authority or page capacity");
  }
  inflight_block_count_ = std::min(
      maximum_source_count,
      block_buffers_[active_buffer_index_].size());
  inflight_unordered_pair_mass_ = 0U;
  for (std::size_t block_index = 0U;
       block_index < inflight_block_count_;
       ++block_index) {
    std::size_t next_mass = 0U;
    if (!checked_add(
            inflight_unordered_pair_mass_,
            block_buffers_[active_buffer_index_][block_index]
                .unordered_pair_mass,
            next_mass) ||
        next_mass > pending_unordered_pair_mass_) {
      inflight_unordered_pair_mass_ = 0U;
      inflight_block_count_ = 0U;
      throw std::overflow_error(
          "a transactional pair-block page mass is not representable");
    }
    inflight_unordered_pair_mass_ = next_mass;
  }
  pending_unordered_pair_mass_ -= inflight_unordered_pair_mass_;
  wave_open_ = true;
  ++audit_.wave_begin_count;
  refresh_audit();
  verify_internal_mass();
  return ExactPairBlockTransactionalFrontierCudaWaveStart::wave_ready;
}

ExactPairBlockTransactionalFrontierCudaWaveResult
ExactPairBlockTransactionalFrontierCudaContext::rollback_result(
    ExactPairBlockTransactionalFrontierCudaWaveDecision decision,
    std::size_t submitted_block_count) noexcept {
  block_buffers_[1U - active_buffer_index_].clear();
  pending_unordered_pair_mass_ += inflight_unordered_pair_mass_;
  inflight_unordered_pair_mass_ = 0U;
  inflight_block_count_ = 0U;
  wave_open_ = false;
  ++audit_.wave_rollback_count;
  audit_.rejected_wave_restored_without_scientific_mutation = true;
  refresh_audit();
  ExactPairBlockTransactionalFrontierCudaWaveResult result;
  result.decision = decision;
  result.submitted_block_count = submitted_block_count;
  result.committed_pending_unordered_pair_mass =
      pending_unordered_pair_mass_;
  result.source_wave_restored_on_rejection = true;
  return result;
}

bool ExactPairBlockTransactionalFrontierCudaContext::rollback_wave()
    & noexcept {
  if (!ready() || !wave_open_) {
    return false;
  }
  ++audit_.explicit_rollback_count;
  (void)rollback_result(
      ExactPairBlockTransactionalFrontierCudaWaveDecision::
          rolled_back_invalid_proposal,
      block_buffers_[active_buffer_index_].size());
  return true;
}

ExactPairBlockTransactionalFrontierCudaWaveResult
ExactPairBlockTransactionalFrontierCudaContext::commit_wave(
    std::span<const ExactPairBlockTransactionalFrontierCudaProposal>
        proposals) & {
  if (!ready() || !wave_open_) {
    ExactPairBlockTransactionalFrontierCudaWaveResult result;
    result.decision =
        ExactPairBlockTransactionalFrontierCudaWaveDecision::no_wave_open;
    return result;
  }
  const auto all_source_blocks = std::span<const ExactPairBlockAuthority>{
      block_buffers_[active_buffer_index_]};
  if (inflight_block_count_ == 0U ||
      inflight_block_count_ > all_source_blocks.size()) {
    return rollback_result(
        ExactPairBlockTransactionalFrontierCudaWaveDecision::
            rolled_back_arithmetic_overflow,
        0U);
  }
  const auto source_blocks =
      all_source_blocks.first(inflight_block_count_);
  const std::size_t submitted_block_count = source_blocks.size();
  if (proposals.size() != source_blocks.size()) {
    return rollback_result(
        ExactPairBlockTransactionalFrontierCudaWaveDecision::
            rolled_back_invalid_proposal,
        submitted_block_count);
  }

  const std::size_t inactive_index = 1U - active_buffer_index_;
  auto& successors = block_buffers_[inactive_index];
  successors.clear();
  const std::size_t retained_source_count =
      all_source_blocks.size() - submitted_block_count;
  try {
    successors.insert(
        successors.end(),
        all_source_blocks.begin() +
            static_cast<std::ptrdiff_t>(submitted_block_count),
        all_source_blocks.end());
  } catch (const std::bad_alloc&) {
    return rollback_result(
        ExactPairBlockTransactionalFrontierCudaWaveDecision::
            rolled_back_operational_allocation_failure,
        submitted_block_count);
  }
  std::vector<ExactPairBlockTransactionalFrontierCudaPruneReceipt>
      staged_prune_receipts;
  std::vector<ExactPairBlockTransactionalFrontierCudaTerminalPair>
      staged_terminal_pairs;
  try {
    staged_prune_receipts.reserve(std::min(
        submitted_block_count,
        prune_receipt_capacity_ - prune_receipts_.size()));
    staged_terminal_pairs.reserve(std::min(
        submitted_block_count,
        terminal_pair_capacity_ - terminal_pairs_.size()));
  } catch (const std::bad_alloc&) {
    return rollback_result(
        ExactPairBlockTransactionalFrontierCudaWaveDecision::
            rolled_back_operational_allocation_failure,
        submitted_block_count);
  }

  std::vector<std::uint64_t> transaction_ids;
  try {
    transaction_ids.reserve(proposals.size());
    for (const auto& proposal : proposals) {
      transaction_ids.push_back(proposal.transaction_id);
    }
    std::sort(transaction_ids.begin(), transaction_ids.end());
  } catch (const std::bad_alloc&) {
    return rollback_result(
        ExactPairBlockTransactionalFrontierCudaWaveDecision::
            rolled_back_operational_allocation_failure,
        submitted_block_count);
  }
  if (std::adjacent_find(
          transaction_ids.begin(), transaction_ids.end()) !=
      transaction_ids.end()) {
    return rollback_result(
        ExactPairBlockTransactionalFrontierCudaWaveDecision::
            rolled_back_invalid_proposal,
        submitted_block_count);
  }

  enum class StageFailure : std::uint8_t {
    none,
    invalid,
    capacity,
    arithmetic,
    operational_allocation,
  };
  StageFailure failure = StageFailure::none;
  std::size_t staged_pending_mass = 0U;
  std::size_t staged_pruned_mass = 0U;
  std::size_t staged_terminal_mass = 0U;
  std::size_t exact_prune_attempt_count = 0U;
  std::size_t certified_prune_count = 0U;
  std::size_t fail_open_count = 0U;
  std::size_t diagonal_split_count = 0U;
  std::size_t cross_split_count = 0U;

  const auto node_authority = [this](std::size_t node_index) {
    const auto& node = index_->nodes_[node_index];
    return ExactPairBlockNodeAuthority{
        node_index, node.leaf_begin, node.leaf_end};
  };
  const auto diagonal_authority =
      [&node_authority](std::size_t node_index, std::size_t& mass) {
        const ExactPairBlockNodeAuthority node = node_authority(node_index);
        if (!checked_unordered_pair_count(node.leaf_count(), mass)) {
          return ExactPairBlockAuthority{};
        }
        return ExactPairBlockAuthority{
            ExactPairBlockAuthorityKind::diagonal, node, node, mass};
      };
  const auto cross_authority =
      [&node_authority](
          std::size_t first_node_index,
          std::size_t second_node_index,
          std::size_t& mass) {
        ExactPairBlockNodeAuthority first =
            node_authority(first_node_index);
        ExactPairBlockNodeAuthority second =
            node_authority(second_node_index);
        if (second.leaf_begin < first.leaf_begin) {
          std::swap(first, second);
        }
        if (!ranges_are_disjoint(first, second) ||
            first.leaf_end > second.leaf_begin ||
            !checked_product(first.leaf_count(), second.leaf_count(), mass)) {
          return ExactPairBlockAuthority{};
        }
        return ExactPairBlockAuthority{
            ExactPairBlockAuthorityKind::cross, first, second, mass};
      };
  const auto add_successor =
      [this, &successors, &staged_pending_mass, &failure](
          const ExactPairBlockAuthority& authority) {
        if (authority.unordered_pair_mass == 0U) {
          return true;
        }
        if (successors.size() >= pending_block_capacity_) {
          failure = StageFailure::capacity;
          return false;
        }
        std::size_t next_mass = 0U;
        if (!checked_add(
                staged_pending_mass,
                authority.unordered_pair_mass,
                next_mass)) {
          failure = StageFailure::arithmetic;
          return false;
        }
        successors.push_back(authority);
        staged_pending_mass = next_mass;
        return true;
      };

  const auto authority_is_current = [this](
                                        const ExactPairBlockAuthority& value) {
    const auto node_is_current = [this](
                                     const ExactPairBlockNodeAuthority& node) {
      if (node.node_index >= index_->nodes_.size()) {
        return false;
      }
      const auto& source = index_->nodes_[node.node_index];
      return node.leaf_begin < node.leaf_end &&
          source.leaf_begin == node.leaf_begin &&
          source.leaf_end == node.leaf_end;
    };
    if (!node_is_current(value.first) || !node_is_current(value.second)) {
      return false;
    }
    std::size_t expected_mass = 0U;
    if (value.kind == ExactPairBlockAuthorityKind::diagonal) {
      return value.first == value.second &&
          checked_unordered_pair_count(
              value.first.leaf_count(), expected_mass) &&
          value.unordered_pair_mass == expected_mass;
    }
    return value.first.leaf_end <= value.second.leaf_begin &&
        checked_product(
            value.first.leaf_count(),
            value.second.leaf_count(),
            expected_mass) &&
        value.unordered_pair_mass == expected_mass;
  };

  try {
    for (std::size_t block_index = 0U;
         block_index < source_blocks.size() &&
         failure == StageFailure::none;
         ++block_index) {
      const ExactPairBlockAuthority& source = source_blocks[block_index];
      const auto& proposal = proposals[block_index];
      if (proposal.source != source || !authority_is_current(source) ||
          proposal.witness_node_count > proposal.witness_node_indices.size()) {
        failure = StageFailure::invalid;
        break;
      }
      switch (proposal.kind) {
        case ExactPairBlockTransactionalFrontierCudaProposalKind::open:
          if (proposal.witness_node_count != 0U) {
            failure = StageFailure::invalid;
          }
          break;
        case ExactPairBlockTransactionalFrontierCudaProposalKind::
            try_exact_prune_else_open:
          if (source.kind != ExactPairBlockAuthorityKind::cross) {
            failure = StageFailure::invalid;
          }
          break;
        default:
          failure = StageFailure::invalid;
          break;
      }
      if (failure != StageFailure::none) {
        break;
      }

      bool certified_prune = false;
      if (proposal.kind ==
          ExactPairBlockTransactionalFrontierCudaProposalKind::
              try_exact_prune_else_open) {
        ++exact_prune_attempt_count;
        hierarchy::ExactBlockRankPruneResult prune =
            hierarchy::certify_exact_block_rank_prune_receipt(
                *index_,
                *cloud_,
                source.first.node_index,
                source.second.node_index,
                std::span<const std::size_t>{
                    proposal.witness_node_indices.data(),
                    proposal.witness_node_count},
                config_.maximum_closed_rank);
        if (prune.certified() && prune.receipt() != nullptr) {
          if (prune_receipts_.size() + staged_prune_receipts.size() >=
              prune_receipt_capacity_) {
            failure = StageFailure::capacity;
            break;
          }
          ExactPairBlockTransactionalFrontierCudaPruneReceipt compact;
          compact.transaction_id = proposal.transaction_id;
          compact.support_block = source;
          compact.witness_node_count = prune.receipt()->witness_nodes().size();
          compact.maximum_closed_rank = config_.maximum_closed_rank;
          compact.certified_witness_point_count =
              prune.receipt()->certified_witness_point_count();
          compact.unordered_pair_mass =
              prune.receipt()->unordered_pair_mass();
          if (compact.witness_node_count == 0U ||
              compact.witness_node_count > compact.witness_nodes.size() ||
              compact.unordered_pair_mass != source.unordered_pair_mass) {
            failure = StageFailure::arithmetic;
            break;
          }
          for (std::size_t witness_index = 0U;
               witness_index < compact.witness_node_count;
               ++witness_index) {
            const auto witness =
                prune.receipt()->witness_nodes()[witness_index];
            compact.witness_nodes[witness_index] =
                ExactPairBlockNodeAuthority{
                    witness.node_index, witness.leaf_begin, witness.leaf_end};
          }
          std::size_t next_mass = 0U;
          if (!checked_add(
                  staged_pruned_mass,
                  source.unordered_pair_mass,
                  next_mass)) {
            failure = StageFailure::arithmetic;
            break;
          }
          staged_pruned_mass = next_mass;
          staged_prune_receipts.push_back(compact);
          ++certified_prune_count;
          certified_prune = true;
        } else {
          ++fail_open_count;
        }
      }
      if (certified_prune) {
        continue;
      }

      if (source.kind == ExactPairBlockAuthorityKind::diagonal) {
        const auto& parent = index_->nodes_[source.first.node_index];
        if (parent.is_leaf() || parent.left_child >= index_->nodes_.size() ||
            parent.right_child >= index_->nodes_.size() ||
            parent.left_child == parent.right_child) {
          failure = StageFailure::invalid;
          break;
        }
        std::size_t left_mass = 0U;
        std::size_t cross_mass = 0U;
        std::size_t right_mass = 0U;
        const ExactPairBlockAuthority left =
            diagonal_authority(parent.left_child, left_mass);
        const ExactPairBlockAuthority cross = cross_authority(
            parent.left_child, parent.right_child, cross_mass);
        const ExactPairBlockAuthority right =
            diagonal_authority(parent.right_child, right_mass);
        std::size_t child_mass = 0U;
        if (!checked_add(left_mass, cross_mass, child_mass) ||
            !checked_add(child_mass, right_mass, child_mass) ||
            child_mass != source.unordered_pair_mass) {
          failure = StageFailure::arithmetic;
          break;
        }
        if (!add_successor(left) || !add_successor(cross) ||
            !add_successor(right)) {
          break;
        }
        ++diagonal_split_count;
        continue;
      }

      const auto& first_node = index_->nodes_[source.first.node_index];
      const auto& second_node = index_->nodes_[source.second.node_index];
      if (first_node.is_leaf() && second_node.is_leaf()) {
        if (source.unordered_pair_mass != 1U ||
            terminal_pairs_.size() + staged_terminal_pairs.size() >=
                terminal_pair_capacity_) {
          failure = source.unordered_pair_mass == 1U
              ? StageFailure::capacity
              : StageFailure::arithmetic;
          break;
        }
        spatial::PointId first_id =
            index_->leaves_[source.first.leaf_begin].point_id;
        spatial::PointId second_id =
            index_->leaves_[source.second.leaf_begin].point_id;
        if (second_id < first_id) {
          std::swap(first_id, second_id);
        }
        staged_terminal_pairs.push_back(
            ExactPairBlockTransactionalFrontierCudaTerminalPair{
                proposal.transaction_id,
                source.first,
                source.second,
                {first_id, second_id}});
        if (!checked_add(
                staged_terminal_mass,
                1U,
                staged_terminal_mass)) {
          failure = StageFailure::arithmetic;
          break;
        }
        continue;
      }

      const bool split_second =
          !second_node.is_leaf() &&
          (first_node.is_leaf() ||
           source.second.leaf_count() >= source.first.leaf_count());
      std::array<ExactPairBlockAuthority, 2U> children{};
      std::array<std::size_t, 2U> child_masses{};
      if (split_second) {
        children = {
            cross_authority(
                source.first.node_index,
                second_node.left_child,
                child_masses[0]),
            cross_authority(
                source.first.node_index,
                second_node.right_child,
                child_masses[1])};
      } else {
        if (first_node.is_leaf()) {
          failure = StageFailure::invalid;
          break;
        }
        children = {
            cross_authority(
                first_node.left_child,
                source.second.node_index,
                child_masses[0]),
            cross_authority(
                first_node.right_child,
                source.second.node_index,
                child_masses[1])};
      }
      std::size_t child_mass = 0U;
      if (!checked_add(child_masses[0], child_masses[1], child_mass) ||
          child_mass != source.unordered_pair_mass) {
        failure = StageFailure::arithmetic;
        break;
      }
      if (!add_successor(children[0]) || !add_successor(children[1])) {
        break;
      }
      ++cross_split_count;
    }
  } catch (const std::bad_alloc&) {
    failure = StageFailure::operational_allocation;
  } catch (const std::overflow_error&) {
    failure = StageFailure::arithmetic;
  }

  if (failure != StageFailure::none) {
    ExactPairBlockTransactionalFrontierCudaWaveDecision decision =
        ExactPairBlockTransactionalFrontierCudaWaveDecision::
            rolled_back_invalid_proposal;
    if (failure == StageFailure::capacity) {
      decision = ExactPairBlockTransactionalFrontierCudaWaveDecision::
          rolled_back_capacity_exhausted;
    } else if (failure == StageFailure::arithmetic) {
      decision = ExactPairBlockTransactionalFrontierCudaWaveDecision::
          rolled_back_arithmetic_overflow;
    } else if (failure == StageFailure::operational_allocation) {
      decision = ExactPairBlockTransactionalFrontierCudaWaveDecision::
          rolled_back_operational_allocation_failure;
    }
    return rollback_result(decision, submitted_block_count);
  }

  std::size_t staged_resolved_mass = 0U;
  if (!checked_add(
          staged_pruned_mass,
          staged_terminal_mass,
          staged_resolved_mass) ||
      !checked_add(
          staged_resolved_mass,
          staged_pending_mass,
          staged_resolved_mass) ||
      staged_resolved_mass != inflight_unordered_pair_mass_) {
    return rollback_result(
        ExactPairBlockTransactionalFrontierCudaWaveDecision::
            rolled_back_arithmetic_overflow,
        submitted_block_count);
  }
  std::size_t next_pruned_mass = 0U;
  std::size_t next_terminal_mass = 0U;
  std::size_t next_pending_mass = 0U;
  if (!checked_add(
          pending_unordered_pair_mass_,
          staged_pending_mass,
          next_pending_mass) ||
      !checked_add(
          pruned_unordered_pair_mass_,
          staged_pruned_mass,
          next_pruned_mass) ||
      !checked_add(
          terminal_unordered_pair_mass_,
          staged_terminal_mass,
          next_terminal_mass)) {
    return rollback_result(
        ExactPairBlockTransactionalFrontierCudaWaveDecision::
            rolled_back_arithmetic_overflow,
        submitted_block_count);
  }

  for (auto& receipt : staged_prune_receipts) {
    prune_receipts_.push_back(std::move(receipt));
  }
  for (auto& terminal : staged_terminal_pairs) {
    terminal_pairs_.push_back(std::move(terminal));
  }
  block_buffers_[active_buffer_index_].clear();
  active_buffer_index_ = inactive_index;
  pending_unordered_pair_mass_ = next_pending_mass;
  inflight_unordered_pair_mass_ = 0U;
  inflight_block_count_ = 0U;
  pruned_unordered_pair_mass_ = next_pruned_mass;
  terminal_unordered_pair_mass_ = next_terminal_mass;
  wave_open_ = false;
  complete_ = block_buffers_[active_buffer_index_].empty();
  ++audit_.wave_commit_count;
  audit_.exact_prune_attempt_count += exact_prune_attempt_count;
  audit_.certified_prune_count += certified_prune_count;
  audit_.exact_prune_fail_open_count += fail_open_count;
  audit_.diagonal_split_count += diagonal_split_count;
  audit_.cross_split_count += cross_split_count;
  ++audit_.double_buffer_swap_count;
  audit_.logical_double_buffer_frontier_exercised = true;
  audit_.global_pair_coverage_closed = complete_;
  refresh_audit();
  verify_internal_mass();

  ExactPairBlockTransactionalFrontierCudaWaveResult result;
  result.decision = ExactPairBlockTransactionalFrontierCudaWaveDecision::
      complete_wave_commit;
  result.submitted_block_count = submitted_block_count;
  result.certified_prune_count = certified_prune_count;
  result.fail_open_count = fail_open_count;
  result.diagonal_split_count = diagonal_split_count;
  result.cross_split_count = cross_split_count;
  result.terminal_pair_count = staged_terminal_pairs.size();
  result.successor_block_count =
      successors.size() - retained_source_count;
  result.committed_pruned_unordered_pair_mass = staged_pruned_mass;
  result.committed_terminal_unordered_pair_mass = staged_terminal_mass;
  result.committed_pending_unordered_pair_mass = staged_pending_mass;
  result.scientific_state_mutated = true;
  return result;
}

ExactPairBlockTransactionalFrontierCudaTerminalAuthority
ExactPairBlockTransactionalFrontierCudaContext::seal() && {
  if (!complete() || wave_open_) {
    throw std::logic_error(
        "only a complete transactional pair-block frontier can be sealed");
  }
  audit_.final_cut_digest = final_cut_digest(
      audit_.point_count,
      audit_.maximum_closed_rank,
      prune_receipts_,
      terminal_pairs_);
  audit_.terminal_authority_sealed = true;
  audit_.global_pair_coverage_closed = true;
  refresh_audit();
  verify_internal_mass();
  sealed_or_moved_ = true;
  return ExactPairBlockTransactionalFrontierCudaTerminalAuthority(
      audit_,
      std::move(prune_receipts_),
      std::move(terminal_pairs_),
      std::move(lbvh_identity_));
}

}  // namespace morsehgp3d::gpu
