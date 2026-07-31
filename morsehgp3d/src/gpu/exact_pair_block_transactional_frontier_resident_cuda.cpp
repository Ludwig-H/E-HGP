#include "morsehgp3d/gpu/exact_pair_block_transactional_frontier_resident_cuda.hpp"

#include "phase15_exact_pair_block_transactional_frontier_resident_cuda_internal.hpp"

#include "morsehgp3d/hierarchy/exact_block_rank_prune_receipt.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::detail {
namespace {

inline constexpr std::uint64_t digest_offset =
    UINT64_C(1469598103934665603);
inline constexpr std::uint64_t digest_prime = UINT64_C(1099511628211);

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

}  // namespace

Phase15ExactPairBlockTransactionalFrontierResidentCudaAdoptedTraversal
Phase15ExactPairBlockTransactionalFrontierResidentCudaTraversalAccess::consume(
    MortonLbvhDeviceTraversalLease&& lease) {
  if (!lease.ready()) {
    throw std::invalid_argument(
        "the resident transactional frontier requires a ready traversal "
        "lease");
  }
  const MortonLbvhDeviceTraversalLeaseAudit audit = lease.audit();
  auto owner = std::make_shared<MortonLbvhDeviceTraversalLease>(
      std::move(lease));

  Phase15ExactPairBlockTransactionalFrontierResidentCudaAdoptedTraversal
      adopted;
  adopted.retained_owner = owner;
  adopted.source_cloud_identity = owner->source_cloud_identity_;
  adopted.device_coordinate_bits = owner->device_coordinate_bits_;
  adopted.device_morton_point_ids = owner->device_morton_point_ids_;
  adopted.device_nodes = owner->device_nodes_;
  adopted.point_count = audit.point_count;
  adopted.certified_node_count = audit.certified_node_count;
  adopted.source_snapshot_epoch = audit.source_snapshot_epoch;
  adopted.cuda_device = owner->cuda_device_;
  adopted.host_fake = audit.host_fake_lifecycle_exercised;
  adopted.ready = owner->ready();

  const bool fake_shape = adopted.host_fake && !owner->cuda_resident() &&
      adopted.device_coordinate_bits == nullptr &&
      adopted.device_morton_point_ids == nullptr &&
      adopted.device_nodes == nullptr && adopted.cuda_device == -1;
  const bool cuda_shape = !adopted.host_fake && owner->cuda_resident() &&
      adopted.device_coordinate_bits != nullptr &&
      adopted.device_morton_point_ids != nullptr &&
      adopted.device_nodes != nullptr && adopted.cuda_device >= 0;
  if (!adopted.ready || !adopted.retained_owner ||
      !adopted.source_cloud_identity || (!fake_shape && !cuda_shape)) {
    throw std::logic_error(
        "the resident transactional frontier failed to adopt the native "
        "LBVH authority");
  }
  return adopted;
}

std::uint64_t
phase15_exact_pair_block_transactional_frontier_resident_cuda_recipe_digest(
    std::span<
        const Phase15ExactPairBlockTransactionalFrontierResidentCudaRecipe>
        recipes) noexcept {
  std::uint64_t digest = digest_word(
      digest_offset, UINT64_C(0x4d48475052534331));
  digest = digest_word(digest, static_cast<std::uint64_t>(recipes.size()));
  for (const auto& recipe : recipes) {
    digest = digest_word(digest, recipe.source.kind);
    digest = digest_word(digest, recipe.source.first.node_index);
    digest = digest_word(digest, recipe.source.first.leaf_begin);
    digest = digest_word(digest, recipe.source.first.leaf_end);
    digest = digest_word(digest, recipe.source.second.node_index);
    digest = digest_word(digest, recipe.source.second.leaf_begin);
    digest = digest_word(digest, recipe.source.second.leaf_end);
    digest = digest_word(digest, recipe.source.unordered_pair_mass);
    digest = digest_word(digest, recipe.witness_node_count);
    for (std::size_t witness_index = 0U;
         witness_index < recipe.witness_node_count;
         ++witness_index) {
      digest = digest_word(
          digest, recipe.witness_node_indices[witness_index]);
    }
  }
  return digest;
}

}  // namespace morsehgp3d::gpu::detail

namespace morsehgp3d::gpu {
namespace {

inline constexpr std::uint64_t digest_offset =
    UINT64_C(1469598103934665603);
inline constexpr std::uint64_t digest_prime = UINT64_C(1099511628211);

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

[[nodiscard]] bool checked_pair_count(
    std::size_t point_count,
    std::size_t& pair_count) noexcept {
  if (point_count < 2U) {
    pair_count = 0U;
    return true;
  }
  const std::size_t first =
      point_count % 2U == 0U ? point_count / 2U : point_count;
  const std::size_t second = point_count % 2U == 0U
      ? point_count - 1U
      : (point_count - 1U) / 2U;
  return checked_product(first, second, pair_count);
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
      digest = digest_word(digest, node.node_index);
      digest = digest_word(digest, node.leaf_begin);
      digest = digest_word(digest, node.leaf_end);
    };
    add_node(receipt.support_block.first);
    add_node(receipt.support_block.second);
    digest = digest_word(digest, receipt.witness_node_count);
    for (std::size_t witness_index = 0U;
         witness_index < receipt.witness_node_count;
         ++witness_index) {
      add_node(receipt.witness_nodes[witness_index]);
    }
    digest = digest_word(digest, receipt.unordered_pair_mass);
  }
  digest = digest_word(
      digest, static_cast<std::uint64_t>(terminal_pairs.size()));
  for (const auto& terminal : terminal_pairs) {
    digest = digest_word(digest, terminal.first_node.node_index);
    digest = digest_word(digest, terminal.second_node.node_index);
    digest = digest_word(digest, terminal.point_ids[0]);
    digest = digest_word(digest, terminal.point_ids[1]);
  }
  return digest;
}

[[nodiscard]] bool mass_is_conserved(
    const ExactPairBlockTransactionalFrontierResidentCudaAudit& audit)
    noexcept {
  std::size_t observed = 0U;
  return checked_add(
             audit.pending_unordered_pair_mass,
             audit.inflight_unordered_pair_mass,
             observed) &&
      checked_add(observed, audit.pruned_unordered_pair_mass, observed) &&
      checked_add(observed, audit.terminal_unordered_pair_mass, observed) &&
      observed == audit.unordered_pair_universe_mass;
}

[[nodiscard]] auto compact_block_key(
    const detail::
        Phase15ExactPairBlockTransactionalFrontierResidentCudaBlock& block) {
  return std::tuple{
      block.kind,
      block.first.leaf_begin,
      block.first.leaf_end,
      block.first.node_index,
      block.second.leaf_begin,
      block.second.leaf_end,
      block.second.node_index};
}

}  // namespace

struct ExactPairBlockTransactionalFrontierResidentCudaContext::HostState {
  detail::Phase15ExactPairBlockTransactionalFrontierResidentCudaAdoptedTraversal
      traversal;
  const spatial::MortonLbvhIndex* index{};
  const spatial::CanonicalPointCloud* cloud{};
  ExactPairBlockTransactionalFrontierResidentCudaConfig config{};
  detail::Phase15ExactPairBlockTransactionalFrontierResidentCudaBlock root{};
  std::shared_ptr<const void> lbvh_identity;
  std::size_t block_capacity{};
  std::size_t prune_receipt_capacity{};
  std::size_t terminal_pair_capacity{};
  std::size_t recipe_capacity{};
  std::size_t unordered_pair_universe_mass{};
  bool consumed{false};
};

ExactPairBlockTransactionalFrontierResidentCudaResult::
    ExactPairBlockTransactionalFrontierResidentCudaResult(
        ExactPairBlockTransactionalFrontierResidentCudaStatus status,
        ExactPairBlockTransactionalFrontierResidentCudaAudit audit,
        std::vector<ExactPairBlockTransactionalFrontierCudaPruneReceipt>&&
            prune_receipts,
        std::vector<ExactPairBlockTransactionalFrontierCudaTerminalPair>&&
            terminal_pairs,
        std::vector<ExactPairBlockAuthority>&& pending_blocks,
        std::shared_ptr<const void> lbvh_identity) noexcept
    : status_(status),
      audit_(audit),
      prune_receipts_(std::move(prune_receipts)),
      terminal_pairs_(std::move(terminal_pairs)),
      pending_blocks_(std::move(pending_blocks)),
      lbvh_identity_(std::move(lbvh_identity)) {}

bool ExactPairBlockTransactionalFrontierResidentCudaResult::complete()
    const noexcept {
  return (status_ == ExactPairBlockTransactionalFrontierResidentCudaStatus::
                         qualified_cuda_terminal_authority ||
          status_ == ExactPairBlockTransactionalFrontierResidentCudaStatus::
                         non_authoritative_host_fake_terminal) &&
      audit_.global_pair_coverage_closed && audit_.terminal_authority_sealed &&
      audit_.pending_unordered_pair_mass == 0U &&
      audit_.inflight_unordered_pair_mass == 0U && pending_blocks_.empty() &&
      audit_.prune_receipt_count == prune_receipts_.size() &&
      audit_.terminal_pair_count == terminal_pairs_.size() &&
      mass_is_conserved(audit_);
}

bool ExactPairBlockTransactionalFrontierResidentCudaResult::
    qualified_cuda_terminal_authority() const noexcept {
  return complete() &&
      status_ == ExactPairBlockTransactionalFrontierResidentCudaStatus::
                     qualified_cuda_terminal_authority &&
      audit_.cuda_execution_performed &&
      audit_.one_persistent_kernel_launch_validated &&
      audit_.zero_intermediate_d2h_validated;
}

bool ExactPairBlockTransactionalFrontierResidentCudaResult::validated_for(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud) const {
  if (!complete() || !index.validated_for(cloud) ||
      lbvh_identity_.get() != index.identity_.get() ||
      audit_.point_count != cloud.size() ||
      audit_.certified_node_count != index.nodes_.size() ||
      !audit_.transactional_mass_conservation_validated ||
      !audit_.native_split_partition_validated ||
      !audit_.pairwise_disjoint_support_products_validated ||
      audit_.pair_catalog_complete_claimed || audit_.hierarchy_or_tree_claimed ||
      audit_.slo_claimed || audit_.global_pair_matrix_materialized ||
      audit_.ordinary_or_higher_order_delaunay_materialized ||
      audit_.global_facet_coface_or_incidence_arena_materialized ||
      audit_.durable_receipt_claimed || audit_.public_status_claimed) {
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
          exact_pair_block_transactional_frontier_resident_cuda_maximum_witness_node_count>
          witnesses{};
      for (std::size_t witness_index = 0U;
           witness_index < receipt.witness_node_count;
           ++witness_index) {
        witnesses[witness_index] =
            receipt.witness_nodes[witness_index].node_index;
      }
      auto replay = hierarchy::certify_exact_block_rank_prune_receipt(
          index,
          cloud,
          receipt.support_block.first.node_index,
          receipt.support_block.second.node_index,
          std::span<const std::size_t>{
              witnesses.data(), receipt.witness_node_count},
          receipt.maximum_closed_rank);
      if (!replay.certified() || replay.receipt() == nullptr ||
          replay.receipt()->certified_witness_point_count() !=
              receipt.certified_witness_point_count ||
          replay.receipt()->unordered_pair_mass() !=
              receipt.unordered_pair_mass ||
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
        terminal.first_node.leaf_end > terminal.second_node.leaf_begin) {
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

  std::size_t final_mass = 0U;
  return observed_pruned_mass == audit_.pruned_unordered_pair_mass &&
      checked_add(
          observed_pruned_mass, terminal_pairs_.size(), final_mass) &&
      final_mass == audit_.unordered_pair_universe_mass &&
      audit_.terminal_unordered_pair_mass == terminal_pairs_.size() &&
      audit_.final_cut_digest == final_cut_digest(
          audit_.point_count,
          audit_.maximum_closed_rank,
          prune_receipts_,
          terminal_pairs_);
}

ExactPairBlockTransactionalFrontierResidentCudaContext::
    ExactPairBlockTransactionalFrontierResidentCudaContext(
        std::unique_ptr<HostState>&& host_state) noexcept
    : host_(std::move(host_state)) {}

ExactPairBlockTransactionalFrontierResidentCudaContext::
    ExactPairBlockTransactionalFrontierResidentCudaContext(
        ExactPairBlockTransactionalFrontierResidentCudaContext&&) noexcept =
    default;

ExactPairBlockTransactionalFrontierResidentCudaContext&
ExactPairBlockTransactionalFrontierResidentCudaContext::operator=(
    ExactPairBlockTransactionalFrontierResidentCudaContext&&) noexcept =
    default;

ExactPairBlockTransactionalFrontierResidentCudaContext::
    ~ExactPairBlockTransactionalFrontierResidentCudaContext() = default;

ExactPairBlockTransactionalFrontierResidentCudaContext
ExactPairBlockTransactionalFrontierResidentCudaContext::start(
    MortonLbvhDeviceTraversalLease&& traversal_lease,
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    ExactPairBlockTransactionalFrontierResidentCudaConfig config) {
  if (!index.validated_for(cloud) ||
      config.maximum_closed_rank < 2U ||
      config.maximum_closed_rank >
          exact_pair_block_transactional_frontier_resident_cuda_maximum_closed_rank) {
    throw std::invalid_argument(
        "the resident transactional frontier received invalid authorities "
        "or rank");
  }
  const auto factor_is_valid = [](std::size_t factor) {
    return factor != 0U &&
        factor <=
            exact_pair_block_transactional_frontier_resident_cuda_maximum_capacity_per_point;
  };
  if (!factor_is_valid(config.block_capacity_per_point) ||
      !factor_is_valid(config.prune_receipt_capacity_per_point) ||
      !factor_is_valid(config.terminal_pair_capacity_per_point) ||
      !factor_is_valid(config.prune_recipe_capacity_per_point) ||
      cloud.size() > std::numeric_limits<std::uint32_t>::max() ||
      index.nodes_.size() > std::numeric_limits<std::uint32_t>::max()) {
    throw std::invalid_argument(
        "the resident transactional frontier capacity or compact width is "
        "invalid");
  }

  auto host = std::make_unique<HostState>();
  host->traversal = detail::
      Phase15ExactPairBlockTransactionalFrontierResidentCudaTraversalAccess::
          consume(std::move(traversal_lease));
  if (host->traversal.source_cloud_identity.get() != cloud.identity_.get() ||
      host->traversal.point_count != cloud.size() ||
      host->traversal.certified_node_count != index.nodes_.size()) {
    throw std::invalid_argument(
        "the resident transactional frontier traversal authority does not "
        "match the borrowed cloud and LBVH");
  }
  if (!checked_product(
          cloud.size(), config.block_capacity_per_point,
          host->block_capacity) ||
      !checked_product(
          cloud.size(), config.prune_receipt_capacity_per_point,
          host->prune_receipt_capacity) ||
      !checked_product(
          cloud.size(), config.terminal_pair_capacity_per_point,
          host->terminal_pair_capacity) ||
      !checked_product(
          cloud.size(), config.prune_recipe_capacity_per_point,
          host->recipe_capacity) ||
      host->block_capacity > std::numeric_limits<std::uint32_t>::max() ||
      host->prune_receipt_capacity >
          std::numeric_limits<std::uint32_t>::max() ||
      host->terminal_pair_capacity >
          std::numeric_limits<std::uint32_t>::max() ||
      host->recipe_capacity > std::numeric_limits<std::uint32_t>::max() ||
      !checked_pair_count(cloud.size(), host->unordered_pair_universe_mass)) {
    throw std::overflow_error(
        "the resident transactional frontier bounded capacities overflow");
  }
  if (host->unordered_pair_universe_mass != 0U &&
      host->block_capacity == 0U) {
    throw std::invalid_argument(
        "the resident transactional frontier cannot represent its root");
  }

  host->index = &index;
  host->cloud = &cloud;
  host->config = config;
  host->lbvh_identity = index.identity_;
  if (host->unordered_pair_universe_mass != 0U) {
    const auto& root = index.nodes_[index.root_index_];
    host->root.unordered_pair_mass = host->unordered_pair_universe_mass;
    host->root.first = {
        static_cast<std::uint32_t>(index.root_index_),
        static_cast<std::uint32_t>(root.leaf_begin),
        static_cast<std::uint32_t>(root.leaf_end)};
    host->root.second = host->root.first;
    host->root.kind = static_cast<std::uint8_t>(
        ExactPairBlockAuthorityKind::diagonal);
  }
  return ExactPairBlockTransactionalFrontierResidentCudaContext{
      std::move(host)};
}

bool ExactPairBlockTransactionalFrontierResidentCudaContext::ready()
    const noexcept {
  return host_ != nullptr && !host_->consumed && host_->index != nullptr &&
      host_->cloud != nullptr &&
      host_->index->validated_for(*host_->cloud) &&
      host_->lbvh_identity.get() == host_->index->identity_.get() &&
      host_->traversal.ready;
}

bool ExactPairBlockTransactionalFrontierResidentCudaContext::consumed()
    const noexcept {
  return host_ == nullptr || host_->consumed;
}

const ExactPairBlockTransactionalFrontierResidentCudaConfig&
ExactPairBlockTransactionalFrontierResidentCudaContext::config()
    const & noexcept {
  return host_->config;
}

ExactPairBlockTransactionalFrontierResidentCudaResult
ExactPairBlockTransactionalFrontierResidentCudaContext::run(
    std::span<
        const ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe>
        prune_recipes) & {
  if (!ready()) {
    throw std::logic_error(
        "a moved or consumed resident transactional frontier cannot run");
  }

  std::vector<
      detail::Phase15ExactPairBlockTransactionalFrontierResidentCudaRecipe>
      compact;
  bool valid = prune_recipes.size() <= host_->recipe_capacity;
  try {
    if (valid) {
      compact.reserve(prune_recipes.size());
    }
    for (const auto& recipe : prune_recipes) {
      if (!valid && compact.size() >= host_->recipe_capacity) {
        break;
      }
      detail::Phase15ExactPairBlockTransactionalFrontierResidentCudaRecipe
          value;
      value.source.unordered_pair_mass =
          static_cast<std::uint64_t>(recipe.source.unordered_pair_mass);
      value.source.kind = static_cast<std::uint8_t>(recipe.source.kind);
      const auto compact_node = [&valid](
                                    const ExactPairBlockNodeAuthority& source) {
        detail::
            Phase15ExactPairBlockTransactionalFrontierResidentCudaNode node;
        if (source.node_index > std::numeric_limits<std::uint32_t>::max() ||
            source.leaf_begin > std::numeric_limits<std::uint32_t>::max() ||
            source.leaf_end > std::numeric_limits<std::uint32_t>::max()) {
          valid = false;
          return node;
        }
        node.node_index = static_cast<std::uint32_t>(source.node_index);
        node.leaf_begin = static_cast<std::uint32_t>(source.leaf_begin);
        node.leaf_end = static_cast<std::uint32_t>(source.leaf_end);
        return node;
      };
      value.source.first = compact_node(recipe.source.first);
      value.source.second = compact_node(recipe.source.second);
      if (recipe.source.kind != ExactPairBlockAuthorityKind::cross ||
          recipe.witness_node_count > recipe.witness_node_indices.size()) {
        valid = false;
      }
      value.witness_node_count =
          static_cast<std::uint32_t>(recipe.witness_node_count);
      for (std::size_t witness_index = 0U;
           witness_index < recipe.witness_node_count &&
           witness_index < recipe.witness_node_indices.size();
           ++witness_index) {
        if (recipe.witness_node_indices[witness_index] >
            std::numeric_limits<std::uint32_t>::max()) {
          valid = false;
        } else {
          value.witness_node_indices[witness_index] =
              static_cast<std::uint32_t>(
                  recipe.witness_node_indices[witness_index]);
        }
      }
      compact.push_back(value);
    }
    std::sort(
        compact.begin(), compact.end(),
        [](const auto& first, const auto& second) {
          return compact_block_key(first.source) <
              compact_block_key(second.source);
        });
  } catch (const std::bad_alloc&) {
    valid = false;
  }
  for (std::size_t recipe_index = 1U;
       valid && recipe_index < compact.size();
       ++recipe_index) {
    if (compact_block_key(compact[recipe_index - 1U].source) ==
        compact_block_key(compact[recipe_index].source)) {
      valid = false;
    }
  }

  const std::uint64_t recipe_digest = detail::
      phase15_exact_pair_block_transactional_frontier_resident_cuda_recipe_digest(
          compact);
  host_->consumed = true;
  if (!valid) {
    ExactPairBlockTransactionalFrontierResidentCudaAudit audit;
    audit.point_count = host_->cloud->size();
    audit.certified_node_count = host_->index->nodes_.size();
    audit.maximum_closed_rank = host_->config.maximum_closed_rank;
    audit.required_witness_point_count =
        host_->config.maximum_closed_rank - 1U;
    audit.block_capacity_per_point = host_->config.block_capacity_per_point;
    audit.block_capacity = host_->block_capacity;
    audit.prune_receipt_capacity = host_->prune_receipt_capacity;
    audit.terminal_pair_capacity = host_->terminal_pair_capacity;
    audit.prune_recipe_capacity = host_->recipe_capacity;
    audit.submitted_prune_recipe_count = prune_recipes.size();
    audit.unordered_pair_universe_mass =
        host_->unordered_pair_universe_mass;
    audit.pending_unordered_pair_mass =
        host_->unordered_pair_universe_mass;
    audit.pending_block_count =
        host_->unordered_pair_universe_mass == 0U ? 0U : 1U;
    audit.submitted_recipe_digest = recipe_digest;
    audit.source_snapshot_epoch = host_->traversal.source_snapshot_epoch;
    audit.transactional_mass_conservation_validated = mass_is_conserved(audit);
    std::vector<ExactPairBlockAuthority> pending;
    if (host_->unordered_pair_universe_mass != 0U) {
      pending.push_back(ExactPairBlockAuthority{
          ExactPairBlockAuthorityKind::diagonal,
          {host_->root.first.node_index, host_->root.first.leaf_begin,
           host_->root.first.leaf_end},
          {host_->root.second.node_index, host_->root.second.leaf_begin,
           host_->root.second.leaf_end},
          host_->unordered_pair_universe_mass});
    }
    return ExactPairBlockTransactionalFrontierResidentCudaResult{
        ExactPairBlockTransactionalFrontierResidentCudaStatus::
            preflight_invalid_recipe,
        audit,
        {},
        {},
        std::move(pending),
        host_->lbvh_identity};
  }

  detail::Phase15ExactPairBlockTransactionalFrontierResidentCudaRequest
      request;
  request.traversal = &host_->traversal;
  request.host_index = host_->index;
  request.host_cloud = host_->cloud;
  request.root = host_->root;
  request.recipes = compact;
  request.config = host_->config;
  request.block_capacity = host_->block_capacity;
  request.prune_receipt_capacity = host_->prune_receipt_capacity;
  request.terminal_pair_capacity = host_->terminal_pair_capacity;
  request.recipe_capacity = host_->recipe_capacity;
  request.unordered_pair_universe_mass = host_->unordered_pair_universe_mass;
  request.submitted_recipe_digest = recipe_digest;
  auto receipt = detail::
      phase15_launch_exact_pair_block_transactional_frontier_resident_cuda(
          request);
  if (receipt.audit.global_pair_coverage_closed) {
    receipt.audit.final_cut_digest = final_cut_digest(
        receipt.audit.point_count,
        receipt.audit.maximum_closed_rank,
        receipt.prune_receipts,
        receipt.terminal_pairs);
  }
  return ExactPairBlockTransactionalFrontierResidentCudaResult{
      receipt.status,
      receipt.audit,
      std::move(receipt.prune_receipts),
      std::move(receipt.terminal_pairs),
      std::move(receipt.pending_blocks),
      host_->lbvh_identity};
}

}  // namespace morsehgp3d::gpu
