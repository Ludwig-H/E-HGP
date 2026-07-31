#pragma once

#include "morsehgp3d/gpu/exact_pair_block_frontier.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t
    exact_pair_block_witness_cuda_schema_version = 1U;
inline constexpr std::size_t
    exact_pair_block_witness_cuda_maximum_closed_rank = 6U;
inline constexpr std::size_t
    exact_pair_block_witness_cuda_maximum_task_capacity_per_point = 64U;
inline constexpr std::string_view exact_pair_block_witness_cuda_backend =
    "cuda_g4_plus_host_fake_contract";
inline constexpr std::string_view exact_pair_block_witness_cuda_profile =
    "hgp_reduced";
inline constexpr std::string_view exact_pair_block_witness_cuda_mode =
    "bounded_native_AxBxW_exact_witness_qualification";
inline constexpr std::string_view
    exact_pair_block_witness_cuda_deployment_status = "component_only";
inline constexpr std::string_view exact_pair_block_witness_cuda_public_status =
    "not_claimed";

struct ExactPairBlockWitnessCudaTask {
  std::uint64_t task_id{};
  ExactPairBlockAuthority support_block{};
  ExactPairBlockNodeAuthority witness{};

  friend bool operator==(
      const ExactPairBlockWitnessCudaTask&,
      const ExactPairBlockWitnessCudaTask&) = default;
};

struct ExactPairBlockWitnessCudaConfig {
  std::size_t maximum_closed_rank{2U};
  std::size_t task_capacity_per_point{64U};

  friend bool operator==(
      const ExactPairBlockWitnessCudaConfig&,
      const ExactPairBlockWitnessCudaConfig&) = default;
};

enum class ExactPairBlockWitnessCudaDecision : std::uint8_t {
  certified_closed,
  inconclusive_nonnegative_q,
  inconclusive_insufficient_witness_mass,
  residual_invalid_authority,
  residual_support_overlap,
  residual_fixed_limb_overflow,
};

enum class ExactPairBlockWitnessCudaProposalState : std::uint8_t {
  not_evaluated,
  directed_negative,
  directed_nonnegative,
  directed_ambiguous,
  directed_arithmetic_overflow,
};

struct ExactPairBlockWitnessCudaRecord {
  std::uint64_t task_id{};
  ExactPairBlockWitnessCudaDecision decision{
      ExactPairBlockWitnessCudaDecision::residual_invalid_authority};
  ExactPairBlockWitnessCudaProposalState proposal_state{
      ExactPairBlockWitnessCudaProposalState::not_evaluated};
  std::int8_t exact_sign{};
  std::size_t unordered_pair_mass{};
  bool fixed_limb_evaluation_performed{false};

  friend bool operator==(
      const ExactPairBlockWitnessCudaRecord&,
      const ExactPairBlockWitnessCudaRecord&) = default;
};

enum class ExactPairBlockWitnessCudaStatus : std::uint8_t {
  qualified_component_batch,
  non_authoritative_host_fake,
  capacity_exhausted,
};

struct ExactPairBlockWitnessCudaAudit {
  std::uint32_t schema_version{
      exact_pair_block_witness_cuda_schema_version};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t maximum_closed_rank{};
  std::size_t task_capacity_per_point{};
  std::size_t task_capacity{};
  std::size_t submitted_task_count{};
  std::size_t completed_task_count{};
  std::size_t directed_negative_count{};
  std::size_t directed_nonnegative_count{};
  std::size_t directed_ambiguous_count{};
  std::size_t directed_arithmetic_overflow_count{};
  std::size_t fixed_limb_exact_decision_count{};
  std::size_t exact_negative_count{};
  std::size_t exact_zero_count{};
  std::size_t exact_positive_count{};
  std::size_t fixed_limb_residual_count{};
  std::size_t certified_task_count{};
  std::size_t nonnegative_task_count{};
  std::size_t insufficient_witness_task_count{};
  std::size_t invalid_authority_task_count{};
  std::size_t support_overlap_task_count{};
  std::size_t submitted_unordered_pair_mass{};
  std::size_t pruned_unordered_pair_mass{};
  std::size_t terminal_unordered_pair_mass{};
  std::size_t pending_unordered_pair_mass{};
  std::size_t residual_unordered_pair_mass{};
  std::size_t host_to_device_task_byte_count{};
  std::size_t device_to_host_result_byte_count{};
  std::size_t compact_task_record_byte_size{};
  std::size_t compact_result_record_byte_size{};
  std::size_t device_arena_allocation_count{};
  std::size_t per_task_allocation_count{};
  std::size_t per_task_synchronization_count{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t submitted_task_digest{};
  std::uint64_t completed_result_digest{};
  std::uint64_t kernel_elapsed_nanoseconds{};
  int cuda_device{-1};
  bool native_lbvh_authority_consumed{false};
  bool native_lbvh_nodes_read_on_device{false};
  bool fixed_linear_task_capacity_validated{false};
  bool local_submitted_mass_conservation_validated{false};
  bool bounded_final_qualification_readback_performed{false};
  bool compact_batch_abi_validated{false};
  bool resident_batch_submission_validated{false};
  bool fp64_used_as_proposal_only{false};
  bool negative_closure_requires_fixed_limb_exact_decision{false};
  bool host_fake_lifecycle_exercised{false};
  bool cuda_execution_performed{false};
  bool double_buffered_transactional_frontier_claimed{false};
  bool global_pair_coverage_closed{false};
  bool pair_catalog_complete_claimed{false};
  bool hierarchy_or_tree_claimed{false};
  bool slo_claimed{false};
  bool global_pair_matrix_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactPairBlockWitnessCudaAudit&,
      const ExactPairBlockWitnessCudaAudit&) = default;
};

struct ExactPairBlockWitnessCudaResult {
  ExactPairBlockWitnessCudaStatus status{
      ExactPairBlockWitnessCudaStatus::capacity_exhausted};
  ExactPairBlockWitnessCudaAudit audit{};
  std::vector<ExactPairBlockWitnessCudaRecord> records;

  [[nodiscard]] bool qualified_cuda_batch() const noexcept {
    return status ==
               ExactPairBlockWitnessCudaStatus::qualified_component_batch &&
        audit.cuda_execution_performed &&
        audit.local_submitted_mass_conservation_validated &&
        records.size() == audit.completed_task_count;
  }
};

// This is a bounded final-readback qualification component for the exact
// A x B x W certification kernel.  It is not the product frontier: it neither
// closes unsubmitted pair coverage nor implements the future transactional
// double-buffer queue.
[[nodiscard]] ExactPairBlockWitnessCudaResult
qualify_exact_pair_block_witnesses_cuda(
    MortonLbvhDeviceTraversalLease&& traversal_lease,
    std::span<const ExactPairBlockWitnessCudaTask> tasks,
    ExactPairBlockWitnessCudaConfig config = {});

}  // namespace morsehgp3d::gpu
