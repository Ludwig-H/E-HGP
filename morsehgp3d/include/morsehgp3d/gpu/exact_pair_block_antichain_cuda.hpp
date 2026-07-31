#pragma once

#include "morsehgp3d/gpu/exact_pair_block_witness_cuda.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t
    exact_pair_block_antichain_cuda_schema_version = 1U;
inline constexpr std::size_t
    exact_pair_block_antichain_cuda_maximum_closed_rank = 11U;
inline constexpr std::size_t
    exact_pair_block_antichain_cuda_maximum_witness_node_count =
        exact_pair_block_antichain_cuda_maximum_closed_rank - 1U;
inline constexpr std::size_t
    exact_pair_block_antichain_cuda_maximum_transaction_capacity_per_point =
        12U;
inline constexpr std::string_view exact_pair_block_antichain_cuda_backend =
    "cuda_g4_plus_host_fake_contract";
inline constexpr std::string_view exact_pair_block_antichain_cuda_profile =
    "hgp_reduced";
inline constexpr std::string_view exact_pair_block_antichain_cuda_mode =
    "bounded_transactional_multi_node_witness_antichain_product_coverage";
inline constexpr std::string_view
    exact_pair_block_antichain_cuda_deployment_status = "architecture_only";
inline constexpr std::string_view exact_pair_block_antichain_cuda_public_status =
    "not_claimed";

struct ExactPairBlockAntichainCudaTransaction {
  std::uint64_t transaction_id{};
  ExactPairBlockAuthority support_block{};
  std::array<
      ExactPairBlockNodeAuthority,
      exact_pair_block_antichain_cuda_maximum_witness_node_count>
      witness_nodes{};
  std::size_t witness_node_count{};

  friend bool operator==(
      const ExactPairBlockAntichainCudaTransaction&,
      const ExactPairBlockAntichainCudaTransaction&) = default;
};

struct ExactPairBlockAntichainCudaConfig {
  std::size_t maximum_closed_rank{2U};
  std::size_t transaction_capacity_per_point{
      exact_pair_block_antichain_cuda_maximum_transaction_capacity_per_point};

  friend bool operator==(
      const ExactPairBlockAntichainCudaConfig&,
      const ExactPairBlockAntichainCudaConfig&) = default;
};

enum class ExactPairBlockAntichainCudaDecision : std::uint8_t {
  certified_closed,
  inconclusive_insufficient_witness_mass,
  residual_invalid_native_authority,
  residual_support_overlap,
  residual_nonnegative_q,
  residual_fixed_limb_overflow,
};

struct ExactPairBlockAntichainCudaRecord {
  std::uint64_t transaction_id{};
  ExactPairBlockAntichainCudaDecision decision{
      ExactPairBlockAntichainCudaDecision::
          residual_invalid_native_authority};
  std::size_t witness_node_count{};
  std::size_t authenticated_witness_point_count{};
  std::size_t exact_negative_witness_node_count{};
  std::size_t unordered_pair_mass{};

  friend bool operator==(
      const ExactPairBlockAntichainCudaRecord&,
      const ExactPairBlockAntichainCudaRecord&) = default;
};

enum class ExactPairBlockAntichainCudaStatus : std::uint8_t {
  qualified_component_batch,
  non_authoritative_host_fake,
  preflight_inconclusive,
  capacity_exhausted,
};

struct ExactPairBlockAntichainCudaAudit {
  std::uint32_t schema_version{
      exact_pair_block_antichain_cuda_schema_version};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t maximum_closed_rank{};
  std::size_t required_witness_point_count{};
  std::size_t transaction_capacity_per_point{};
  std::size_t transaction_capacity{};
  std::size_t submitted_transaction_count{};
  std::size_t completed_transaction_count{};
  std::size_t submitted_witness_node_count{};
  std::size_t predicate_task_count{};
  std::size_t predicate_pattern_task_count{};
  std::size_t physical_predicate_task_count{};
  std::size_t predicate_task_capacity{};
  std::size_t predicate_task_capacity_per_point{};
  std::size_t certified_transaction_count{};
  std::size_t insufficient_witness_transaction_count{};
  std::size_t invalid_native_authority_transaction_count{};
  std::size_t support_overlap_transaction_count{};
  std::size_t nonnegative_transaction_count{};
  std::size_t fixed_limb_residual_transaction_count{};
  std::size_t exact_negative_witness_node_count{};
  std::size_t nonnegative_witness_node_count{};
  std::size_t invalid_native_authority_witness_node_count{};
  std::size_t support_overlap_witness_node_count{};
  std::size_t fixed_limb_residual_witness_node_count{};
  std::size_t submitted_unordered_pair_mass{};
  std::size_t pruned_unordered_pair_mass{};
  std::size_t residual_unordered_pair_mass{};
  std::size_t host_to_device_predicate_task_byte_count{};
  std::size_t device_to_host_predicate_result_byte_count{};
  std::size_t device_arena_allocation_count{};
  std::size_t per_transaction_allocation_count{};
  std::size_t per_transaction_synchronization_count{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t submitted_transaction_digest{};
  std::uint64_t canonical_transaction_digest{};
  std::uint64_t completed_transaction_digest{};
  std::uint64_t predicate_task_digest{};
  std::uint64_t predicate_result_digest{};
  std::uint64_t first_transaction_id{};
  std::uint64_t last_transaction_id{};
  // Diagnostic binding for this process-local batch only.  FNV and the
  // snapshot epoch are not a durable cloud/LBVH identity or a certificate.
  std::uint64_t process_local_transcript_digest{};
  std::uint64_t predicate_kernel_elapsed_nanoseconds{};
  int cuda_device{-1};
  bool native_lbvh_authority_consumed{false};
  bool native_lbvh_nodes_read_on_device{false};
  bool fixed_linear_transaction_capacity_validated{false};
  bool fixed_linear_predicate_capacity_validated{false};
  bool compact_index_width_validated{false};
  bool unique_transaction_ids_validated{false};
  bool canonical_witness_antichains_validated{false};
  bool predicate_result_identities_validated{false};
  bool process_local_transcript_validated{false};
  bool transactional_product_mass_conservation_validated{false};
  bool every_certified_transaction_has_sufficient_disjoint_exact_negative_witness_mass{
      false};
  bool one_batched_predicate_submission_validated{false};
  bool compact_repeated_transaction_recipe_validated{false};
  bool affine_transaction_id_range_validated{false};
  bool every_logical_transaction_represented_once{false};
  bool per_transaction_host_materialization_avoided{false};
  bool physical_repeated_predicate_expansion_performed{false};
  bool host_fake_lifecycle_exercised{false};
  bool cuda_execution_performed{false};
  bool pairwise_disjoint_support_products_validated{false};
  bool double_buffered_transactional_frontier_claimed{false};
  bool global_pair_coverage_closed{false};
  bool pair_catalog_complete_claimed{false};
  bool hierarchy_or_tree_claimed{false};
  bool slo_claimed{false};
  bool global_pair_matrix_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool public_status_claimed{false};
  bool durable_receipt_claimed{false};

  friend bool operator==(
      const ExactPairBlockAntichainCudaAudit&,
      const ExactPairBlockAntichainCudaAudit&) = default;
};

struct ExactPairBlockAntichainCudaRepeatedResult {
  ExactPairBlockAntichainCudaStatus status{
      ExactPairBlockAntichainCudaStatus::capacity_exhausted};
  ExactPairBlockAntichainCudaAudit audit{};
  // The complete logical output is the checked affine transaction-id range
  // paired with this one repeated record shape.  No O(transaction_count)
  // host record vector is materialized.
  ExactPairBlockAntichainCudaRecord first_record{};

  [[nodiscard]] bool contains_transaction_id(
      std::uint64_t transaction_id) const noexcept {
    return audit.completed_transaction_count != 0U &&
        transaction_id >= audit.first_transaction_id &&
        transaction_id <= audit.last_transaction_id;
  }

  [[nodiscard]] bool qualified_cuda_batch() const noexcept {
    return status ==
               ExactPairBlockAntichainCudaStatus::qualified_component_batch &&
        audit.cuda_execution_performed &&
        audit.compact_repeated_transaction_recipe_validated &&
        audit.affine_transaction_id_range_validated &&
        audit.every_logical_transaction_represented_once &&
        audit.per_transaction_host_materialization_avoided &&
        audit.transactional_product_mass_conservation_validated;
  }
};

struct ExactPairBlockAntichainCudaResult {
  ExactPairBlockAntichainCudaStatus status{
      ExactPairBlockAntichainCudaStatus::capacity_exhausted};
  ExactPairBlockAntichainCudaAudit audit{};
  std::vector<ExactPairBlockAntichainCudaRecord> records;

  [[nodiscard]] bool qualified_cuda_batch() const noexcept {
    return status ==
               ExactPairBlockAntichainCudaStatus::qualified_component_batch &&
        audit.cuda_execution_performed &&
        audit.transactional_product_mass_conservation_validated &&
        records.size() == audit.completed_transaction_count;
  }
};

// Each transaction authenticates one submitted support product exactly once.
// Its canonical, pairwise-disjoint witness-node antichain is flattened into
// one already-qualified A x B x W predicate batch at rank two, where the
// predicate itself is independent of the requested rank.  The support product
// is committed only when the disjoint exact-negative witness mass reaches
// R-1.  Distinct submitted support products are not yet proved disjoint, so
// this component cannot close the global frontier.
[[nodiscard]] ExactPairBlockAntichainCudaResult
qualify_exact_pair_block_antichain_transactions_cuda(
    MortonLbvhDeviceTraversalLease&& traversal_lease,
    std::span<const ExactPairBlockAntichainCudaTransaction> transactions,
    ExactPairBlockAntichainCudaConfig config = {});

// Physically evaluates every predicate in a repeated transaction recipe while
// keeping the host-side transaction representation O(1).  The model's
// transaction_id is the first identity and subsequent identities are
// contiguous.  This optimization is invalid for a heterogeneous transaction
// stream and deliberately makes no global coverage or hierarchy claim.
[[nodiscard]] ExactPairBlockAntichainCudaRepeatedResult
qualify_repeated_exact_pair_block_antichain_transaction_cuda(
    MortonLbvhDeviceTraversalLease&& traversal_lease,
    ExactPairBlockAntichainCudaTransaction model,
    std::size_t transaction_count,
    ExactPairBlockAntichainCudaConfig config = {});

}  // namespace morsehgp3d::gpu
