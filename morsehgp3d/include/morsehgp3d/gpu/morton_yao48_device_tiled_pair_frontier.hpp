#pragma once

#include "morsehgp3d/gpu/morton_lbvh_build.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

namespace morsehgp3d::gpu {

namespace detail {
class Phase15MortonYao48DeviceCandidateTilePrivateViewAccess;
}

inline constexpr std::uint32_t
    morton_yao48_device_tiled_pair_frontier_schema_version = 1U;
inline constexpr std::size_t
    morton_yao48_device_tiled_pair_frontier_maximum_closed_rank = 11U;
inline constexpr std::size_t
    morton_yao48_device_tiled_pair_frontier_maximum_anchor_tile_capacity =
        4096U;
inline constexpr std::size_t
    morton_yao48_device_tiled_pair_frontier_node_visits_per_anchor = 2048U;
inline constexpr std::size_t
    morton_yao48_device_tiled_pair_frontier_candidates_per_anchor = 640U;
inline constexpr std::size_t
    morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor = 256U;
inline constexpr std::size_t
    morton_yao48_device_tiled_pair_frontier_witness_bank_count = 48U;
inline constexpr std::string_view
    morton_yao48_device_tiled_pair_frontier_backend =
        "cuda_g4_plus_host_fake_contract";
inline constexpr std::string_view
    morton_yao48_device_tiled_pair_frontier_profile = "hgp_reduced";
inline constexpr std::string_view
    morton_yao48_device_tiled_pair_frontier_mode =
        "device_resident_budgeted_morton_yao48_anchor_tiles";
inline constexpr std::string_view
    morton_yao48_device_tiled_pair_frontier_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    morton_yao48_device_tiled_pair_frontier_public_status = "not_claimed";
inline constexpr bool
    morton_yao48_device_tiled_pair_frontier_cuda_implementation_available =
        true;
inline constexpr std::string_view
    morton_yao48_device_tiled_pair_frontier_proof_basis =
        "interval_cone_classification_ambiguity_to_unbanked_candidate_"
        "target_tested_before_bank_insert_retained_witnesses_outside_"
        "subtree_nonnegative_diametral_witness_interval_lower_bound_v1";

enum class MortonYao48DeviceTiledPairFrontierStatus : std::uint8_t {
  frontier_complete,
  tile_complete,
  censored,
};

enum class MortonYao48DeviceTiledPairFrontierStopReason : std::uint8_t {
  none = 0U,
  node_visit_capacity = 1U,
  candidate_capacity = 2U,
  prune_region_capacity = 3U,
};

struct MortonYao48DeviceTiledPairFrontierConfig {
  std::size_t maximum_closed_rank{2U};
  std::size_t anchor_tile_capacity{4096U};

  friend bool operator==(
      const MortonYao48DeviceTiledPairFrontierConfig&,
      const MortonYao48DeviceTiledPairFrontierConfig&) = default;
};

struct MortonYao48DeviceCandidateTileLeaseAudit {
  std::uint32_t schema_version{
      morton_yao48_device_tiled_pair_frontier_schema_version};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t candidate_buffer_epoch{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t retained_coordinate_word_capacity{};
  std::size_t retained_morton_point_id_capacity{};
  std::size_t retained_node_capacity{};
  std::size_t maximum_closed_rank{};
  std::size_t anchor_begin{};
  std::size_t anchor_end{};
  std::size_t candidate_count{};
  std::size_t certified_prune_region_count{};
  std::size_t physical_candidate_capacity{};
  std::size_t physical_prune_region_capacity{};
  std::size_t fixed_candidate_capacity_per_anchor{};
  std::size_t fixed_prune_region_capacity_per_anchor{};
  std::size_t fixed_witness_bank_count_per_anchor{};
  bool traversal_owner_retained{false};
  bool source_cloud_identity_retained{false};
  bool source_device_views_retained{false};
  bool source_device_extents_retained{false};
  bool source_views_bound_to_snapshot_identity{false};
  bool output_owner_retained{false};
  bool output_buffers_detached_for_tile_lifetime{false};
  bool host_fake_lifecycle_exercised{false};
  bool cuda_device_storage_retained{false};
  bool candidate_device_to_host_performed{false};
  bool certified_prune_device_to_host_performed{false};
  bool censored_anchor_outputs_withheld{false};
  bool exact_diametral_rank_evaluated{false};
  bool scientific_pair_catalog_published{false};
  bool dense_pair_fallback_performed{false};
  bool global_pair_matrix_materialized{false};
  bool higher_order_structure_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const MortonYao48DeviceCandidateTileLeaseAudit&,
      const MortonYao48DeviceCandidateTileLeaseAudit&) = default;
};

// Move-only capability for a segmented candidate tile.  Product code can
// inspect counts and authority only; raw device views stay private so no
// intermediate candidate D2H path can accidentally become part of the API.
class MortonYao48DeviceCandidateTileLease final {
 public:
  ~MortonYao48DeviceCandidateTileLease() noexcept = default;
  MortonYao48DeviceCandidateTileLease(
      MortonYao48DeviceCandidateTileLease&&) noexcept = default;
  MortonYao48DeviceCandidateTileLease& operator=(
      MortonYao48DeviceCandidateTileLease&&) noexcept = default;
  MortonYao48DeviceCandidateTileLease(
      const MortonYao48DeviceCandidateTileLease&) = delete;
  MortonYao48DeviceCandidateTileLease& operator=(
      const MortonYao48DeviceCandidateTileLease&) = delete;

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool cuda_resident() const noexcept;
  [[nodiscard]] bool host_fake() const noexcept;
  [[nodiscard]] const MortonYao48DeviceCandidateTileLeaseAudit& audit()
      const noexcept {
    return audit_;
  }

 private:
  MortonYao48DeviceCandidateTileLease(
      MortonYao48DeviceCandidateTileLeaseAudit audit,
      std::shared_ptr<void> retained_owner,
      std::shared_ptr<void> source_owner_authority,
      std::shared_ptr<const void> source_cloud_identity_authority,
      const std::uint64_t* device_coordinate_bits,
      const std::uint64_t* device_morton_point_ids,
      const void* device_nodes,
      const void* device_candidate_records,
      const void* device_certified_prune_regions,
      std::size_t certified_node_count,
      std::size_t retained_coordinate_word_capacity,
      std::size_t retained_morton_point_id_capacity,
      std::size_t retained_node_capacity,
      int cuda_device,
      bool host_fake);

  MortonYao48DeviceCandidateTileLeaseAudit audit_{};
  std::shared_ptr<void> retained_owner_;
  std::shared_ptr<void> source_owner_authority_;
  std::shared_ptr<const void> source_cloud_identity_authority_;
  const std::uint64_t* device_coordinate_bits_{};
  const std::uint64_t* device_morton_point_ids_{};
  const void* device_nodes_{};
  const void* device_candidate_records_{};
  const void* device_certified_prune_regions_{};
  std::size_t certified_node_count_{};
  std::size_t retained_coordinate_word_capacity_{};
  std::size_t retained_morton_point_id_capacity_{};
  std::size_t retained_node_capacity_{};
  int cuda_device_{-1};
  bool host_fake_{false};

  friend class MortonYao48DeviceTiledPairFrontierContext;
  friend class detail::
      Phase15MortonYao48DeviceCandidateTilePrivateViewAccess;
};

struct MortonYao48DeviceTiledPairFrontierAudit {
  std::uint32_t schema_version{
      morton_yao48_device_tiled_pair_frontier_schema_version};
  std::uint64_t advance_sequence{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t candidate_buffer_epoch{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t maximum_closed_rank{};
  std::size_t anchor_tile_capacity{};
  std::size_t fixed_node_visit_capacity_per_anchor{};
  std::size_t fixed_candidate_capacity_per_anchor{};
  std::size_t fixed_prune_region_capacity_per_anchor{};
  std::size_t fixed_witness_bank_count_per_anchor{};
  std::size_t transaction_anchor_begin{};
  std::size_t transaction_anchor_end{};
  std::size_t transaction_committed_anchor_count{};
  std::size_t transaction_certified_prune_region_count{};
  std::uint64_t transaction_ambiguous_cone_candidate_count{};
  std::uint64_t transaction_unbanked_candidate_count{};
  std::uint64_t transaction_candidate_pair_mass{};
  std::uint64_t transaction_certified_pruned_pair_mass{};
  std::uint64_t transaction_physical_node_visit_count{};
  std::size_t completed_anchor_count{};
  std::size_t next_anchor_position{};
  std::uint64_t unordered_pair_universe_count{};
  std::uint64_t cumulative_candidate_pair_mass{};
  std::uint64_t cumulative_certified_pruned_pair_mass{};
  std::uint64_t unresolved_pair_mass{};
  std::uint64_t cumulative_physical_node_visit_count{};
  std::size_t launcher_call_count{};
  std::size_t cuda_kernel_launch_count{};
  std::size_t cuda_synchronization_count{};
  std::size_t anchor_control_device_to_host_count{};
  std::size_t anchor_control_device_to_host_byte_count{};
  std::size_t candidate_device_to_host_count{};
  std::size_t certified_prune_device_to_host_count{};
  int cuda_device{-1};
  bool source_traversal_lease_authenticated{false};
  bool fixed_per_anchor_caps_enforced{false};
  bool atomic_completed_anchor_prefix_validated{false};
  bool censored_anchor_outputs_withheld{false};
  bool candidate_pruned_unresolved_partition_validated{false};
  // Coverage only: candidates remain proposals until a separate exact-rank
  // consumer closes them.
  bool pair_coverage_partition_complete{false};
  bool terminally_censored{false};
  bool traversal_lease_owner_retained{false};
  bool source_cloud_identity_retained{false};
  bool candidate_tile_lease_published{false};
  bool candidate_tile_lease_backpressure_bounded_to_one{false};
  bool candidate_tile_lease_outstanding{false};
  bool output_buffers_detached_for_tile_lifetime{false};
  bool host_fake_launcher_exercised{false};
  bool cuda_execution_performed{false};
  bool candidate_device_to_host_performed{false};
  bool certified_prune_device_to_host_performed{false};
  bool interval_cone_classification_required{false};
  bool ambiguous_cone_routed_to_unbanked_candidate{false};
  bool target_tested_before_witness_bank_insert{false};
  bool retained_witnesses_outside_pruned_subtree_required{false};
  bool nonnegative_diametral_witness_interval_lower_bound_required{false};
  bool exact_diametral_rank_evaluated{false};
  bool scientific_pair_catalog_published{false};
  bool scientific_decision_published{false};
  bool dense_pair_fallback_performed{false};
  bool global_pair_matrix_materialized{false};
  bool ordinary_delaunay_materialized{false};
  bool higher_order_delaunay_mosaic_materialized{false};
  bool global_cell_or_coface_arena_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const MortonYao48DeviceTiledPairFrontierAudit&,
      const MortonYao48DeviceTiledPairFrontierAudit&) = default;
};

struct MortonYao48DeviceTiledPairFrontierAdvance {
  MortonYao48DeviceTiledPairFrontierStatus status{
      MortonYao48DeviceTiledPairFrontierStatus::censored};
  MortonYao48DeviceTiledPairFrontierStopReason stop_reason{
      MortonYao48DeviceTiledPairFrontierStopReason::node_visit_capacity};
  std::optional<MortonYao48DeviceCandidateTileLease> candidate_tile;
  MortonYao48DeviceTiledPairFrontierAudit audit{};

  MortonYao48DeviceTiledPairFrontierAdvance() = default;
  MortonYao48DeviceTiledPairFrontierAdvance(
      MortonYao48DeviceTiledPairFrontierAdvance&&) noexcept = default;
  MortonYao48DeviceTiledPairFrontierAdvance& operator=(
      MortonYao48DeviceTiledPairFrontierAdvance&&) noexcept = default;
  MortonYao48DeviceTiledPairFrontierAdvance(
      const MortonYao48DeviceTiledPairFrontierAdvance&) = delete;
  MortonYao48DeviceTiledPairFrontierAdvance& operator=(
      const MortonYao48DeviceTiledPairFrontierAdvance&) = delete;
};

namespace detail {
struct Phase15MortonYao48DeviceTiledAdoptedTraversal;
class Phase15MortonYao48DeviceTiledPairFrontierContextState;
class Phase15MortonYao48DeviceTiledPairFrontierHostState;

// This is the single private-lease access seam.  Host tests provide a fake;
// the CUDA definition transfers the retained owner, immutable cloud identity
// and three resident raw views without exposing them through the public API.
[[nodiscard]] Phase15MortonYao48DeviceTiledAdoptedTraversal
adopt_phase15_morton_yao48_device_tiled_traversal(
    MortonLbvhDeviceTraversalLease&& traversal_lease);
}  // namespace detail

class MortonYao48DeviceTiledPairFrontierContext final {
 public:
  static constexpr std::string_view backend =
      morton_yao48_device_tiled_pair_frontier_backend;
  static constexpr std::string_view profile =
      morton_yao48_device_tiled_pair_frontier_profile;
  static constexpr std::string_view mode =
      morton_yao48_device_tiled_pair_frontier_mode;
  static constexpr std::string_view deployment_status =
      morton_yao48_device_tiled_pair_frontier_deployment_status;
  static constexpr std::string_view public_status =
      morton_yao48_device_tiled_pair_frontier_public_status;
  static constexpr std::string_view proof_basis =
      morton_yao48_device_tiled_pair_frontier_proof_basis;

  MortonYao48DeviceTiledPairFrontierContext(
      MortonLbvhDeviceTraversalLease&& traversal_lease,
      MortonYao48DeviceTiledPairFrontierConfig config = {});
  ~MortonYao48DeviceTiledPairFrontierContext() noexcept;

  MortonYao48DeviceTiledPairFrontierContext(
      MortonYao48DeviceTiledPairFrontierContext&&) noexcept;
  MortonYao48DeviceTiledPairFrontierContext& operator=(
      MortonYao48DeviceTiledPairFrontierContext&&) noexcept;
  MortonYao48DeviceTiledPairFrontierContext(
      const MortonYao48DeviceTiledPairFrontierContext&) = delete;
  MortonYao48DeviceTiledPairFrontierContext& operator=(
      const MortonYao48DeviceTiledPairFrontierContext&) = delete;

  [[nodiscard]] MortonYao48DeviceTiledPairFrontierAdvance advance();
  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool poisoned() const noexcept;
  [[nodiscard]] bool terminally_censored() const noexcept {
    return terminally_censored_;
  }
  [[nodiscard]] const MortonYao48DeviceTiledPairFrontierConfig& config()
      const noexcept {
    return config_;
  }

 private:
  [[nodiscard]] MortonYao48DeviceTiledPairFrontierAudit make_audit(
      std::size_t transaction_anchor_begin,
      std::size_t transaction_anchor_end,
      std::size_t transaction_committed_anchor_count,
      std::size_t transaction_certified_prune_region_count,
      std::uint64_t transaction_ambiguous_cone_candidate_count,
      std::uint64_t transaction_unbanked_candidate_count,
      std::uint64_t transaction_candidate_pair_mass,
      std::uint64_t transaction_certified_pruned_pair_mass,
      std::uint64_t transaction_physical_node_visit_count,
      std::uint64_t candidate_buffer_epoch,
      std::size_t launcher_call_count,
      std::size_t cuda_kernel_launch_count,
      std::size_t cuda_synchronization_count,
      std::size_t anchor_control_device_to_host_count,
      std::size_t anchor_control_device_to_host_byte_count,
      int cuda_device,
      bool candidate_tile_lease_published,
      bool censored_anchor_outputs_withheld) const noexcept;

  MortonYao48DeviceTiledPairFrontierConfig config_{};
  std::shared_ptr<detail::Phase15MortonYao48DeviceTiledPairFrontierContextState>
      state_;
  std::unique_ptr<detail::Phase15MortonYao48DeviceTiledPairFrontierHostState>
      host_;
  std::weak_ptr<void> active_candidate_tile_authority_;
  std::size_t point_count_{};
  std::size_t certified_node_count_{};
  std::size_t completed_anchor_count_{};
  std::size_t next_anchor_position_{};
  std::uint64_t unordered_pair_universe_count_{};
  std::uint64_t cumulative_candidate_pair_mass_{};
  std::uint64_t cumulative_certified_pruned_pair_mass_{};
  std::uint64_t cumulative_physical_node_visit_count_{};
  std::uint64_t advance_sequence_{};
  std::size_t launcher_call_count_{};
  bool terminally_censored_{false};
  bool host_fake_launcher_exercised_{false};
  bool cuda_execution_performed_{false};
  MortonYao48DeviceTiledPairFrontierStopReason terminal_stop_reason_{
      MortonYao48DeviceTiledPairFrontierStopReason::none};
};

}  // namespace morsehgp3d::gpu
