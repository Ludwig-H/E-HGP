#pragma once

#include "morsehgp3d/gpu/morton_yao48_device_tiled_pair_frontier.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t
    morton_yao48_ranked_pair_tile_classifier_schema_version = 1U;
inline constexpr std::size_t
    morton_yao48_ranked_pair_tile_classifier_minimum_closed_rank = 2U;
inline constexpr std::size_t
    morton_yao48_ranked_pair_tile_classifier_maximum_closed_rank = 11U;
inline constexpr std::string_view
    morton_yao48_ranked_pair_tile_classifier_backend =
        "cuda_g4_plus_host_fake_contract";
inline constexpr std::string_view
    morton_yao48_ranked_pair_tile_classifier_profile = "hgp_reduced";
inline constexpr std::string_view
    morton_yao48_ranked_pair_tile_classifier_mode =
        "resident_frontier_chunks_exact_multiorder_count_scan";
inline constexpr std::string_view
    morton_yao48_ranked_pair_tile_classifier_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    morton_yao48_ranked_pair_tile_classifier_public_status = "not_claimed";
inline constexpr bool
    morton_yao48_ranked_pair_tile_classifier_cuda_implementation_available =
        false;

enum class MortonYao48RankedPairTileClassifierStatus : std::uint8_t {
  chunk_committed,
  frontier_closed,
  capacity_exhausted,
  exact_predicate_failure,
};

enum class MortonYao48RankedPairTileClassifierStopReason : std::uint8_t {
  none = 0U,
  catalog_record_capacity = 1U,
  payload_point_id_capacity = 2U,
  exact_fallback_capacity = 3U,
  exact_predicate_failure = 4U,
};

// All three arenas grow only with resident frontier output.  No capacity is
// derived from the quadratic unordered-pair universe.
struct MortonYao48RankedPairTileClassifierConfig {
  std::size_t maximum_closed_rank{2U};
  std::size_t catalog_record_capacity{};
  std::size_t payload_point_id_capacity{};
  std::size_t exact_fallback_capacity{};

  friend bool operator==(
      const MortonYao48RankedPairTileClassifierConfig&,
      const MortonYao48RankedPairTileClassifierConfig&) = default;
};

struct MortonYao48RankedPairTileClassifierCommitAudit {
  std::uint32_t schema_version{
      morton_yao48_ranked_pair_tile_classifier_schema_version};
  std::uint64_t commit_sequence{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t candidate_buffer_epoch{};
  std::uint64_t catalog_buffer_epoch{};
  std::uint64_t tile_epoch{};
  std::uint64_t chunk_sequence{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t maximum_closed_rank{};
  std::size_t anchor_begin{};
  std::size_t anchor_end{};
  std::uint64_t transaction_candidate_count{};
  std::uint64_t transaction_accepted_record_count{};
  std::uint64_t transaction_above_window_count{};
  std::uint64_t transaction_unresolved_count{};
  std::uint64_t transaction_strict_payload_point_id_count{};
  std::uint64_t transaction_shell_payload_point_id_count{};
  std::uint64_t transaction_exact_fallback_count{};
  std::uint64_t transaction_certified_pruned_pair_mass{};
  std::uint64_t cumulative_candidate_count{};
  std::uint64_t cumulative_accepted_record_count{};
  std::uint64_t cumulative_above_window_count{};
  std::uint64_t cumulative_unresolved_count{};
  std::uint64_t cumulative_strict_payload_point_id_count{};
  std::uint64_t cumulative_shell_payload_point_id_count{};
  std::uint64_t cumulative_exact_fallback_count{};
  std::uint64_t cumulative_certified_pruned_pair_mass{};
  std::uint64_t unordered_pair_universe_count{};
  std::uint64_t frontier_unresolved_pair_mass{};
  std::size_t launcher_call_count{};
  std::size_t cuda_kernel_launch_count{};
  std::size_t cuda_synchronization_count{};
  std::size_t candidate_device_to_host_count{};
  std::size_t certified_prune_device_to_host_count{};
  std::size_t legacy_pair_callback_count{};
  int cuda_device{-1};
  bool same_tile_continuation{false};
  bool tile_closed_after_chunk{false};
  bool frontier_closed_after_chunk{false};
  bool source_identity_authenticated{false};
  bool source_epoch_authenticated{false};
  bool candidate_epoch_authenticated{false};
  bool tile_chunk_sequence_authenticated{false};
  bool candidate_tile_lease_retained_during_launch{false};
  bool candidate_tile_lease_consumed{false};
  bool fixed_linear_capacities_validated{false};
  bool exact_multiorder_classification_validated{false};
  bool count_scan_payload_layout_validated{false};
  bool transaction_candidate_mass_validated{false};
  bool cumulative_candidate_mass_validated{false};
  bool frontier_global_mass_validated{false};
  bool output_rollback_validated{false};
  bool output_withheld{false};
  bool host_fake_launcher_exercised{false};
  bool cuda_execution_performed{false};
  bool candidate_device_to_host_performed{false};
  bool certified_prune_device_to_host_performed{false};
  bool callback_per_pair_performed{false};
  bool dense_pair_scan_performed{false};
  bool global_pair_matrix_materialized{false};
  bool ordinary_delaunay_materialized{false};
  bool higher_order_delaunay_mosaic_materialized{false};
  bool scientific_decision_published{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const MortonYao48RankedPairTileClassifierCommitAudit&,
      const MortonYao48RankedPairTileClassifierCommitAudit&) = default;
};

struct MortonYao48RankedPairTileClassifierCommit {
  MortonYao48RankedPairTileClassifierStatus status{
      MortonYao48RankedPairTileClassifierStatus::capacity_exhausted};
  MortonYao48RankedPairTileClassifierStopReason stop_reason{
      MortonYao48RankedPairTileClassifierStopReason::catalog_record_capacity};
  MortonYao48RankedPairTileClassifierCommitAudit audit{};
};

struct MortonYao48RankedPairTileCatalogLeaseAudit {
  std::uint32_t schema_version{
      morton_yao48_ranked_pair_tile_classifier_schema_version};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t final_candidate_buffer_epoch{};
  std::uint64_t catalog_buffer_epoch{};
  std::uint64_t final_tile_epoch{};
  std::uint64_t final_chunk_sequence{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t maximum_closed_rank{};
  std::size_t catalog_record_count{};
  std::size_t strict_payload_point_id_count{};
  std::size_t shell_payload_point_id_count{};
  std::size_t rank_offset_count{};
  std::size_t record_offset_count{};
  std::uint64_t candidate_count{};
  std::uint64_t above_window_count{};
  std::uint64_t unresolved_count{};
  std::uint64_t exact_fallback_count{};
  std::uint64_t certified_pruned_pair_mass{};
  std::uint64_t frontier_unresolved_pair_mass{};
  std::uint64_t unordered_pair_universe_count{};
  std::size_t committed_chunk_count{};
  int cuda_device{-1};
  bool frontier_closed{false};
  bool closed_rank_catalog_complete{false};
  bool source_identity_authenticated{false};
  bool monotone_chunk_journal_validated{false};
  bool exact_multiorder_classification_validated{false};
  bool count_scan_payload_layout_validated{false};
  bool candidate_mass_validated{false};
  bool frontier_global_mass_validated{false};
  bool host_fake_lifecycle_exercised{false};
  bool cuda_device_storage_retained{false};
  bool intermediate_candidate_device_to_host_performed{false};
  bool certified_prune_device_to_host_performed{false};
  bool callback_per_pair_performed{false};
  bool dense_pair_scan_performed{false};
  bool global_pair_matrix_materialized{false};
  bool ordinary_delaunay_materialized{false};
  bool higher_order_delaunay_mosaic_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const MortonYao48RankedPairTileCatalogLeaseAudit&,
      const MortonYao48RankedPairTileCatalogLeaseAudit&) = default;
};

// Final resident catalog.  Its device views stay private so downstream
// consumers must receive an explicit capability instead of introducing a
// candidate or payload D2H escape hatch.
class MortonYao48RankedPairTileCatalogLease final {
 public:
  ~MortonYao48RankedPairTileCatalogLease() noexcept = default;
  MortonYao48RankedPairTileCatalogLease(
      MortonYao48RankedPairTileCatalogLease&&) noexcept = default;
  MortonYao48RankedPairTileCatalogLease& operator=(
      MortonYao48RankedPairTileCatalogLease&&) noexcept = default;
  MortonYao48RankedPairTileCatalogLease(
      const MortonYao48RankedPairTileCatalogLease&) = delete;
  MortonYao48RankedPairTileCatalogLease& operator=(
      const MortonYao48RankedPairTileCatalogLease&) = delete;

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool cuda_resident() const noexcept;
  [[nodiscard]] bool host_fake() const noexcept;
  [[nodiscard]] const MortonYao48RankedPairTileCatalogLeaseAudit& audit()
      const noexcept {
    return audit_;
  }

 private:
  MortonYao48RankedPairTileCatalogLease(
      MortonYao48RankedPairTileCatalogLeaseAudit audit,
      std::shared_ptr<void> retained_owner,
      const std::uint64_t* device_support_u,
      const std::uint64_t* device_support_v,
      const std::uint8_t* device_closed_rank,
      const std::uint64_t* device_rank_offsets,
      const std::uint64_t* device_strict_offsets,
      const std::uint64_t* device_strict_point_ids,
      const std::uint64_t* device_shell_offsets,
      const std::uint64_t* device_shell_point_ids,
      int cuda_device,
      bool host_fake);

  MortonYao48RankedPairTileCatalogLeaseAudit audit_{};
  std::shared_ptr<void> retained_owner_;
  const std::uint64_t* device_support_u_{};
  const std::uint64_t* device_support_v_{};
  const std::uint8_t* device_closed_rank_{};
  const std::uint64_t* device_rank_offsets_{};
  const std::uint64_t* device_strict_offsets_{};
  const std::uint64_t* device_strict_point_ids_{};
  const std::uint64_t* device_shell_offsets_{};
  const std::uint64_t* device_shell_point_ids_{};
  int cuda_device_{-1};
  bool host_fake_{false};

  friend class MortonYao48RankedPairTileClassifierContext;
};

namespace detail {
class Phase15MortonYao48RankedPairTileClassifierContextState;
}

class MortonYao48RankedPairTileClassifierContext final {
 public:
  static constexpr std::string_view backend =
      morton_yao48_ranked_pair_tile_classifier_backend;
  static constexpr std::string_view profile =
      morton_yao48_ranked_pair_tile_classifier_profile;
  static constexpr std::string_view mode =
      morton_yao48_ranked_pair_tile_classifier_mode;
  static constexpr std::string_view deployment_status =
      morton_yao48_ranked_pair_tile_classifier_deployment_status;
  static constexpr std::string_view public_status =
      morton_yao48_ranked_pair_tile_classifier_public_status;

  explicit MortonYao48RankedPairTileClassifierContext(
      MortonYao48RankedPairTileClassifierConfig config);
  ~MortonYao48RankedPairTileClassifierContext() noexcept;

  MortonYao48RankedPairTileClassifierContext(
      MortonYao48RankedPairTileClassifierContext&&) noexcept;
  MortonYao48RankedPairTileClassifierContext& operator=(
      MortonYao48RankedPairTileClassifierContext&&) noexcept;
  MortonYao48RankedPairTileClassifierContext(
      const MortonYao48RankedPairTileClassifierContext&) = delete;
  MortonYao48RankedPairTileClassifierContext& operator=(
      const MortonYao48RankedPairTileClassifierContext&) = delete;

  [[nodiscard]] MortonYao48RankedPairTileClassifierCommit commit(
      MortonYao48DeviceTiledPairFrontierAdvance&& frontier_advance);
  [[nodiscard]] MortonYao48RankedPairTileCatalogLease finish();

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool poisoned() const noexcept;
  [[nodiscard]] bool terminally_censored() const noexcept {
    return terminally_censored_;
  }
  [[nodiscard]] bool frontier_closed() const noexcept {
    return frontier_closed_;
  }
  [[nodiscard]] const MortonYao48RankedPairTileClassifierConfig& config()
      const noexcept {
    return config_;
  }

 private:
  MortonYao48RankedPairTileClassifierConfig config_{};
  std::shared_ptr<detail::
                      Phase15MortonYao48RankedPairTileClassifierContextState>
      state_;
  const void* source_owner_identity_{};
  const void* source_cloud_identity_{};
  std::uint64_t source_snapshot_epoch_{};
  std::uint64_t last_candidate_buffer_epoch_{};
  std::uint64_t last_catalog_buffer_epoch_{};
  std::uint64_t last_tile_epoch_{};
  std::uint64_t last_chunk_sequence_{};
  std::size_t point_count_{};
  std::size_t certified_node_count_{};
  std::size_t last_anchor_begin_{};
  std::size_t last_anchor_end_{};
  std::uint64_t cumulative_candidate_count_{};
  std::uint64_t cumulative_accepted_record_count_{};
  std::uint64_t cumulative_above_window_count_{};
  std::uint64_t cumulative_unresolved_count_{};
  std::uint64_t cumulative_strict_payload_point_id_count_{};
  std::uint64_t cumulative_shell_payload_point_id_count_{};
  std::uint64_t cumulative_exact_fallback_count_{};
  std::uint64_t cumulative_certified_pruned_pair_mass_{};
  std::uint64_t unordered_pair_universe_count_{};
  std::uint64_t frontier_unresolved_pair_mass_{};
  std::size_t committed_chunk_count_{};
  std::size_t launcher_call_count_{};
  std::size_t cuda_kernel_launch_count_{};
  std::size_t cuda_synchronization_count_{};
  std::shared_ptr<void> output_owner_;
  const std::uint64_t* device_support_u_{};
  const std::uint64_t* device_support_v_{};
  const std::uint8_t* device_closed_rank_{};
  const std::uint64_t* device_rank_offsets_{};
  const std::uint64_t* device_strict_offsets_{};
  const std::uint64_t* device_strict_point_ids_{};
  const std::uint64_t* device_shell_offsets_{};
  const std::uint64_t* device_shell_point_ids_{};
  std::size_t rank_offset_count_{};
  std::size_t record_offset_count_{};
  int cuda_device_{-1};
  bool initialized_{false};
  bool prior_tile_resumable_{false};
  bool frontier_closed_{false};
  bool terminally_censored_{false};
  bool finished_{false};
  bool host_fake_launcher_exercised_{false};
  bool cuda_execution_performed_{false};
};

}  // namespace morsehgp3d::gpu
