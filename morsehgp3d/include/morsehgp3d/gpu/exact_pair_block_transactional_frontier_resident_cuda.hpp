#pragma once

#include "morsehgp3d/gpu/exact_pair_block_transactional_frontier_cuda.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t
    exact_pair_block_transactional_frontier_resident_cuda_schema_version = 2U;
inline constexpr std::size_t
    exact_pair_block_transactional_frontier_resident_cuda_maximum_closed_rank =
        11U;
inline constexpr std::size_t
    exact_pair_block_transactional_frontier_resident_cuda_maximum_witness_node_count =
        exact_pair_block_transactional_frontier_resident_cuda_maximum_closed_rank -
        1U;
inline constexpr std::size_t
    exact_pair_block_transactional_frontier_resident_cuda_maximum_capacity_per_point =
        64U;
inline constexpr std::string_view
    exact_pair_block_transactional_frontier_resident_cuda_backend =
        "cuda_g4_plus_host_fake_contract";
inline constexpr std::string_view
    exact_pair_block_transactional_frontier_resident_cuda_profile =
        "hgp_reduced";
inline constexpr std::string_view
    exact_pair_block_transactional_frontier_resident_cuda_mode =
        "bounded_single_launch_device_resident_double_buffered_transactional_"
        "pair_block_scheduler";
inline constexpr std::string_view
    exact_pair_block_transactional_frontier_resident_cuda_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    exact_pair_block_transactional_frontier_resident_cuda_public_status =
        "not_claimed";

struct ExactPairBlockTransactionalFrontierResidentCudaConfig {
  std::size_t maximum_closed_rank{2U};
  std::size_t block_capacity_per_point{16U};
  std::size_t prune_receipt_capacity_per_point{8U};
  std::size_t terminal_pair_capacity_per_point{16U};
  std::size_t prune_recipe_capacity_per_point{4U};

  friend bool operator==(
      const ExactPairBlockTransactionalFrontierResidentCudaConfig&,
      const ExactPairBlockTransactionalFrontierResidentCudaConfig&) = default;
};

// A recipe only asks the resident scheduler to try one exact antichain prune
// when this native support block appears.  It is not trusted: node ranges,
// product mass, antichain disjointness, witness mass and every Q sign are
// authenticated again on the device.  Any failed check is a scientific
// fail-open and takes the same native split/terminal transition as no recipe.
struct ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe {
  ExactPairBlockAuthority source{};
  std::array<
      std::size_t,
      exact_pair_block_transactional_frontier_resident_cuda_maximum_witness_node_count>
      witness_node_indices{};
  std::size_t witness_node_count{};

  friend bool operator==(
      const ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe&,
      const ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe&) =
      default;
};

enum class ExactPairBlockTransactionalFrontierResidentCudaStatus :
    std::uint8_t {
  qualified_cuda_terminal_authority,
  non_authoritative_host_fake_terminal,
  capacity_exhausted_wave_rolled_back,
  preflight_invalid_recipe,
  arithmetic_capacity_rejected,
};

struct ExactPairBlockTransactionalFrontierResidentCudaAudit {
  std::uint32_t schema_version{
      exact_pair_block_transactional_frontier_resident_cuda_schema_version};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t maximum_closed_rank{};
  std::size_t required_witness_point_count{};
  std::size_t block_capacity_per_point{};
  std::size_t block_capacity{};
  std::size_t prune_receipt_capacity{};
  std::size_t terminal_pair_capacity{};
  std::size_t prune_recipe_capacity{};
  std::size_t submitted_prune_recipe_count{};
  std::size_t matched_prune_recipe_count{};
  std::size_t unused_prune_recipe_count{};
  std::size_t unordered_pair_universe_mass{};
  std::size_t pending_unordered_pair_mass{};
  std::size_t inflight_unordered_pair_mass{};
  std::size_t pruned_unordered_pair_mass{};
  std::size_t terminal_unordered_pair_mass{};
  std::size_t pending_block_count{};
  std::size_t prune_receipt_count{};
  std::size_t terminal_pair_count{};
  std::size_t wave_begin_count{};
  std::size_t wave_commit_count{};
  std::size_t wave_rollback_count{};
  std::size_t exact_prune_attempt_count{};
  std::size_t certified_prune_count{};
  std::size_t exact_prune_fail_open_count{};
  std::size_t exact_witness_predicate_count{};
  std::size_t exact_negative_witness_predicate_count{};
  std::size_t nonnegative_or_residual_witness_predicate_count{};
  std::size_t diagonal_split_count{};
  std::size_t cross_split_count{};
  std::size_t maximum_pending_block_count{};
  std::size_t host_to_device_recipe_byte_count{};
  std::size_t device_to_host_final_byte_count{};
  std::size_t device_arena_byte_count{};
  std::size_t device_arena_allocation_count{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  std::size_t intermediate_control_readback_count{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t submitted_recipe_digest{};
  std::uint64_t final_cut_digest{};
  std::uint64_t kernel_elapsed_nanoseconds{};
  int cuda_device{-1};
  bool native_lbvh_authority_consumed{false};
  bool native_lbvh_nodes_read_on_device{false};
  bool fixed_linear_capacities_validated{false};
  bool compact_index_width_validated{false};
  bool unique_recipe_sources_validated{false};
  bool resident_double_buffer_allocated_once{false};
  bool resident_wave_loop_executed{false};
  bool one_persistent_kernel_launch_validated{false};
  bool zero_intermediate_d2h_validated{false};
  bool complete_wave_atomic_commit_validated{false};
  bool capacity_wave_rollback_validated{false};
  bool fail_open_native_transition_validated{false};
  bool recipe_catalog_nonempty{false};
  bool recipe_path_exercised{false};
  bool certified_prune_path_exercised{false};
  bool fail_open_path_exercised{false};
  bool transactional_mass_conservation_validated{false};
  bool native_split_partition_validated{false};
  bool pairwise_disjoint_support_products_validated{false};
  bool terminal_authority_sealed{false};
  bool global_pair_coverage_closed{false};
  bool host_fake_lifecycle_exercised{false};
  bool cuda_execution_performed{false};
  bool serial_device_reference{false};
  bool scale_eligible{false};
  bool pair_catalog_complete_claimed{false};
  bool hierarchy_or_tree_claimed{false};
  bool slo_claimed{false};
  bool global_pair_matrix_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool global_facet_coface_or_incidence_arena_materialized{false};
  bool durable_receipt_claimed{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactPairBlockTransactionalFrontierResidentCudaAudit&,
      const ExactPairBlockTransactionalFrontierResidentCudaAudit&) = default;
};

class ExactPairBlockTransactionalFrontierResidentCudaContext;

class ExactPairBlockTransactionalFrontierResidentCudaResult final {
 public:
  ExactPairBlockTransactionalFrontierResidentCudaResult(
      const ExactPairBlockTransactionalFrontierResidentCudaResult&) = delete;
  ExactPairBlockTransactionalFrontierResidentCudaResult& operator=(
      const ExactPairBlockTransactionalFrontierResidentCudaResult&) = delete;
  ExactPairBlockTransactionalFrontierResidentCudaResult(
      ExactPairBlockTransactionalFrontierResidentCudaResult&&) noexcept =
      default;
  ExactPairBlockTransactionalFrontierResidentCudaResult& operator=(
      ExactPairBlockTransactionalFrontierResidentCudaResult&&) noexcept =
      default;
  ~ExactPairBlockTransactionalFrontierResidentCudaResult() = default;

  [[nodiscard]] ExactPairBlockTransactionalFrontierResidentCudaStatus status()
      const noexcept {
    return status_;
  }
  [[nodiscard]] bool complete() const noexcept;
  [[nodiscard]] bool qualified_cuda_terminal_authority() const noexcept;
  [[nodiscard]] bool validated_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) const;
  [[nodiscard]] const ExactPairBlockTransactionalFrontierResidentCudaAudit&
  audit() const & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactPairBlockTransactionalFrontierResidentCudaAudit&
  audit() const && = delete;
  [[nodiscard]] std::span<
      const ExactPairBlockTransactionalFrontierCudaPruneReceipt>
  prune_receipts() const & noexcept {
    return prune_receipts_;
  }
  [[nodiscard]] std::span<
      const ExactPairBlockTransactionalFrontierCudaPruneReceipt>
  prune_receipts() const && = delete;
  [[nodiscard]] std::span<
      const ExactPairBlockTransactionalFrontierCudaTerminalPair>
  terminal_pairs() const & noexcept {
    return terminal_pairs_;
  }
  [[nodiscard]] std::span<
      const ExactPairBlockTransactionalFrontierCudaTerminalPair>
  terminal_pairs() const && = delete;
  [[nodiscard]] std::span<const ExactPairBlockAuthority> pending_blocks()
      const & noexcept {
    return pending_blocks_;
  }
  [[nodiscard]] std::span<const ExactPairBlockAuthority> pending_blocks()
      const && = delete;

 private:
  ExactPairBlockTransactionalFrontierResidentCudaResult(
      ExactPairBlockTransactionalFrontierResidentCudaStatus status,
      ExactPairBlockTransactionalFrontierResidentCudaAudit audit,
      std::vector<ExactPairBlockTransactionalFrontierCudaPruneReceipt>&&
          prune_receipts,
      std::vector<ExactPairBlockTransactionalFrontierCudaTerminalPair>&&
          terminal_pairs,
      std::vector<ExactPairBlockAuthority>&& pending_blocks,
      std::shared_ptr<const void> lbvh_identity) noexcept;

  ExactPairBlockTransactionalFrontierResidentCudaStatus status_{
      ExactPairBlockTransactionalFrontierResidentCudaStatus::
          arithmetic_capacity_rejected};
  ExactPairBlockTransactionalFrontierResidentCudaAudit audit_{};
  std::vector<ExactPairBlockTransactionalFrontierCudaPruneReceipt>
      prune_receipts_;
  std::vector<ExactPairBlockTransactionalFrontierCudaTerminalPair>
      terminal_pairs_;
  std::vector<ExactPairBlockAuthority> pending_blocks_;
  std::shared_ptr<const void> lbvh_identity_;

  friend class ExactPairBlockTransactionalFrontierResidentCudaContext;
};

// The context consumes the one native traversal lease, keeps that immutable
// authority alive, and permits exactly one resident scheduler run.  The host
// index/cloud are borrowed only for source authentication and final replay;
// they must outlive the context and result validation.
class ExactPairBlockTransactionalFrontierResidentCudaContext final {
 public:
  static constexpr std::string_view backend =
      exact_pair_block_transactional_frontier_resident_cuda_backend;
  static constexpr std::string_view profile =
      exact_pair_block_transactional_frontier_resident_cuda_profile;
  static constexpr std::string_view mode =
      exact_pair_block_transactional_frontier_resident_cuda_mode;
  static constexpr std::string_view deployment_status =
      exact_pair_block_transactional_frontier_resident_cuda_deployment_status;
  static constexpr std::string_view public_status =
      exact_pair_block_transactional_frontier_resident_cuda_public_status;

  [[nodiscard]] static
      ExactPairBlockTransactionalFrontierResidentCudaContext start(
          MortonLbvhDeviceTraversalLease&& traversal_lease,
          const spatial::MortonLbvhIndex& index,
          const spatial::CanonicalPointCloud& cloud,
          ExactPairBlockTransactionalFrontierResidentCudaConfig config = {});
  [[nodiscard]] static
      ExactPairBlockTransactionalFrontierResidentCudaContext start(
          MortonLbvhDeviceTraversalLease&&,
          spatial::MortonLbvhIndex&&,
          const spatial::CanonicalPointCloud&,
          ExactPairBlockTransactionalFrontierResidentCudaConfig = {}) =
      delete;
  [[nodiscard]] static
      ExactPairBlockTransactionalFrontierResidentCudaContext start(
          MortonLbvhDeviceTraversalLease&&,
          const spatial::MortonLbvhIndex&&,
          const spatial::CanonicalPointCloud&,
          ExactPairBlockTransactionalFrontierResidentCudaConfig = {}) =
      delete;
  [[nodiscard]] static
      ExactPairBlockTransactionalFrontierResidentCudaContext start(
          MortonLbvhDeviceTraversalLease&&,
          const spatial::MortonLbvhIndex&,
          spatial::CanonicalPointCloud&&,
          ExactPairBlockTransactionalFrontierResidentCudaConfig = {}) =
      delete;
  [[nodiscard]] static
      ExactPairBlockTransactionalFrontierResidentCudaContext start(
          MortonLbvhDeviceTraversalLease&&,
          const spatial::MortonLbvhIndex&,
          const spatial::CanonicalPointCloud&&,
          ExactPairBlockTransactionalFrontierResidentCudaConfig = {}) =
      delete;

  ExactPairBlockTransactionalFrontierResidentCudaContext(
      const ExactPairBlockTransactionalFrontierResidentCudaContext&) = delete;
  ExactPairBlockTransactionalFrontierResidentCudaContext& operator=(
      const ExactPairBlockTransactionalFrontierResidentCudaContext&) = delete;
  ExactPairBlockTransactionalFrontierResidentCudaContext(
      ExactPairBlockTransactionalFrontierResidentCudaContext&&) noexcept;
  ExactPairBlockTransactionalFrontierResidentCudaContext& operator=(
      ExactPairBlockTransactionalFrontierResidentCudaContext&&) noexcept;
  ~ExactPairBlockTransactionalFrontierResidentCudaContext();

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool consumed() const noexcept;
  [[nodiscard]] const ExactPairBlockTransactionalFrontierResidentCudaConfig&
  config() const & noexcept;
  [[nodiscard]] const ExactPairBlockTransactionalFrontierResidentCudaConfig&
  config() const && = delete;

  [[nodiscard]] ExactPairBlockTransactionalFrontierResidentCudaResult run(
      std::span<
          const ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe>
          prune_recipes) &;

 private:
  struct HostState;
  explicit ExactPairBlockTransactionalFrontierResidentCudaContext(
      std::unique_ptr<HostState>&& host_state) noexcept;

  std::unique_ptr<HostState> host_;
};

}  // namespace morsehgp3d::gpu
