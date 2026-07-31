#pragma once

#include "morsehgp3d/gpu/exact_pair_block_transactional_frontier_resident_cuda.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <type_traits>
#include <vector>

namespace morsehgp3d::gpu::detail {

struct Phase15ExactPairBlockTransactionalFrontierResidentCudaAdoptedTraversal {
  std::shared_ptr<void> retained_owner;
  std::shared_ptr<const void> source_cloud_identity;
  const std::uint64_t* device_coordinate_bits{};
  const std::uint64_t* device_morton_point_ids{};
  const void* device_nodes{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::uint64_t source_snapshot_epoch{};
  int cuda_device{-1};
  bool host_fake{false};
  bool ready{false};
};

class Phase15ExactPairBlockTransactionalFrontierResidentCudaTraversalAccess
    final {
 public:
  [[nodiscard]] static
      Phase15ExactPairBlockTransactionalFrontierResidentCudaAdoptedTraversal
      consume(MortonLbvhDeviceTraversalLease&& lease);
};

struct Phase15ExactPairBlockTransactionalFrontierResidentCudaNode {
  std::uint32_t node_index{};
  std::uint32_t leaf_begin{};
  std::uint32_t leaf_end{};
};

struct Phase15ExactPairBlockTransactionalFrontierResidentCudaBlock {
  std::uint64_t unordered_pair_mass{};
  Phase15ExactPairBlockTransactionalFrontierResidentCudaNode first{};
  Phase15ExactPairBlockTransactionalFrontierResidentCudaNode second{};
  std::uint8_t kind{};
  std::uint8_t reserved[7]{};
};

struct Phase15ExactPairBlockTransactionalFrontierResidentCudaRecipe {
  Phase15ExactPairBlockTransactionalFrontierResidentCudaBlock source{};
  std::uint32_t witness_node_indices[
      exact_pair_block_transactional_frontier_resident_cuda_maximum_witness_node_count]{};
  std::uint32_t witness_node_count{};
  std::uint32_t reserved{};
};

static_assert(
    std::is_trivially_copyable_v<
        Phase15ExactPairBlockTransactionalFrontierResidentCudaNode>);
static_assert(
    std::is_trivially_copyable_v<
        Phase15ExactPairBlockTransactionalFrontierResidentCudaBlock>);
static_assert(
    std::is_trivially_copyable_v<
        Phase15ExactPairBlockTransactionalFrontierResidentCudaRecipe>);

struct Phase15ExactPairBlockTransactionalFrontierResidentCudaRequest {
  const Phase15ExactPairBlockTransactionalFrontierResidentCudaAdoptedTraversal*
      traversal{};
  const spatial::MortonLbvhIndex* host_index{};
  const spatial::CanonicalPointCloud* host_cloud{};
  Phase15ExactPairBlockTransactionalFrontierResidentCudaBlock root{};
  std::span<
      const Phase15ExactPairBlockTransactionalFrontierResidentCudaRecipe>
      recipes;
  ExactPairBlockTransactionalFrontierResidentCudaConfig config{};
  std::size_t block_capacity{};
  std::size_t prune_receipt_capacity{};
  std::size_t terminal_pair_capacity{};
  std::size_t recipe_capacity{};
  std::size_t unordered_pair_universe_mass{};
  std::uint64_t submitted_recipe_digest{};
};

struct Phase15ExactPairBlockTransactionalFrontierResidentCudaReceipt {
  ExactPairBlockTransactionalFrontierResidentCudaStatus status{
      ExactPairBlockTransactionalFrontierResidentCudaStatus::
          arithmetic_capacity_rejected};
  ExactPairBlockTransactionalFrontierResidentCudaAudit audit{};
  std::vector<ExactPairBlockTransactionalFrontierCudaPruneReceipt>
      prune_receipts;
  std::vector<ExactPairBlockTransactionalFrontierCudaTerminalPair>
      terminal_pairs;
  std::vector<ExactPairBlockAuthority> pending_blocks;
  bool source_identity_authenticated{false};
};

[[nodiscard]] Phase15ExactPairBlockTransactionalFrontierResidentCudaReceipt
phase15_launch_exact_pair_block_transactional_frontier_resident_cuda(
    const Phase15ExactPairBlockTransactionalFrontierResidentCudaRequest&
        request);

[[nodiscard]] std::uint64_t
phase15_exact_pair_block_transactional_frontier_resident_cuda_recipe_digest(
    std::span<
        const Phase15ExactPairBlockTransactionalFrontierResidentCudaRecipe>
        recipes) noexcept;

}  // namespace morsehgp3d::gpu::detail
