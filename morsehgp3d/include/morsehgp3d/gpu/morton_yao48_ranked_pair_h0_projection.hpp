#pragma once

#include "morsehgp3d/gpu/morton_yao48_ranked_pair_tile_classifier.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

namespace morsehgp3d::gpu {

namespace detail {
class Phase15MortonYao48RankedPairH0ProjectionPrivateViewAccess;
}

struct MortonYao48RankedPairH0ProjectionResult;

inline constexpr std::uint32_t
    morton_yao48_ranked_pair_h0_projection_schema_version = 1U;
inline constexpr std::size_t
    morton_yao48_ranked_pair_h0_projection_maximum_closed_rank = 6U;
inline constexpr std::size_t
    morton_yao48_ranked_pair_h0_projection_maximum_linear_capacity_factor =
        4096U;
inline constexpr std::string_view
    morton_yao48_ranked_pair_h0_projection_backend =
        "cuda_g4_plus_host_fake_contract";
inline constexpr std::string_view
    morton_yao48_ranked_pair_h0_projection_profile = "hgp_reduced";
inline constexpr std::string_view
    morton_yao48_ranked_pair_h0_projection_mode =
        "resident_ranked_pair_regular_candidate_or_carrier_projection";
inline constexpr std::string_view
    morton_yao48_ranked_pair_h0_projection_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    morton_yao48_ranked_pair_h0_projection_public_status = "not_claimed";

enum class MortonYao48RankedPairH0ProjectionStatus : std::uint8_t {
  complete_resident_projection,
  non_authoritative_host_fake,
  capacity_exhausted,
  exact_level_unresolved,
};

// One projected record is retained for each source record.  This fixed
// point-count factor prevents a caller from disguising a dense pair arena as
// a run-time allocation choice.
struct MortonYao48RankedPairH0ProjectionConfig {
  std::size_t record_capacity_per_point{128U};

  friend bool operator==(
      const MortonYao48RankedPairH0ProjectionConfig&,
      const MortonYao48RankedPairH0ProjectionConfig&) = default;
};

struct MortonYao48RankedPairH0ProjectionAudit {
  std::uint32_t schema_version{
      morton_yao48_ranked_pair_h0_projection_schema_version};
  std::size_t point_count{};
  std::size_t maximum_closed_rank{};
  std::size_t record_capacity_per_point{};
  std::size_t record_capacity{};
  std::size_t source_record_count{};
  std::size_t projected_record_count{};
  std::size_t regular_pair_candidate_count{};
  std::size_t diagnostic_or_carrier_candidate_count{};
  std::size_t strict_payload_point_id_count{};
  std::size_t shell_payload_point_id_count{};
  std::size_t exact_level_unresolved_count{};
  std::size_t cuda_kernel_launch_count{};
  std::size_t cuda_synchronization_count{};
  std::size_t scalar_control_device_to_host_count{};
  int cuda_device{-1};
  bool source_catalog_consumed{false};
  bool source_catalog_retained{false};
  bool source_identity_retained{false};
  bool fixed_linear_capacity_validated{false};
  bool one_record_per_source_validated{false};
  bool exact_level_payload_constructed{false};
  bool exact_levels_computed_from_source_coordinates{false};
  bool strict_and_shell_payload_ranges_validated{false};
  bool regular_pair_requires_empty_extra_shell{false};
  bool regular_pair_closed_rank_identity_validated{false};
  bool scientific_birth_and_saddle_roles_withheld{false};
  bool exact_center_derivable_from_retained_support_coordinates{false};
  bool sparse_direct_h0_payload_adapter_prerequisites_retained{false};
  bool sorted_by_sparse_direct_h0_terminal_key{false};
  bool host_fake_lifecycle_exercised{false};
  bool cuda_execution_performed{false};
  bool cuda_device_storage_retained{false};
  bool intermediate_record_device_to_host_performed{false};
  bool payload_device_to_host_performed{false};
  bool callback_per_pair_performed{false};
  bool dense_pair_scan_performed{false};
  bool global_pair_matrix_materialized{false};
  bool ordinary_delaunay_materialized{false};
  bool higher_order_delaunay_mosaic_materialized{false};
  bool support3_complete_claimed{false};
  bool support4_complete_claimed{false};
  bool gamma2_complete_claimed{false};
  bool hierarchy_complete_claimed{false};
  bool latency_100ms_claimed{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const MortonYao48RankedPairH0ProjectionAudit&,
      const MortonYao48RankedPairH0ProjectionAudit&) = default;
};

// Move-only authority over private resident projection records.  A regular
// pair candidate is emitted only when the source has no non-support shell
// point and closed_rank == 2 + strict_count.  Every other source record stays
// typed as diagnostic_or_carrier_candidate.  This projection assigns neither
// birth_order nor saddle_order; those scientific roles require a separately
// authorized adapter.  Neither lane is an admission to a hierarchy.
class MortonYao48RankedPairH0ProjectionLease final {
 public:
  ~MortonYao48RankedPairH0ProjectionLease() noexcept = default;
  MortonYao48RankedPairH0ProjectionLease(
      MortonYao48RankedPairH0ProjectionLease&&) noexcept = default;
  MortonYao48RankedPairH0ProjectionLease& operator=(
      MortonYao48RankedPairH0ProjectionLease&&) noexcept = default;
  MortonYao48RankedPairH0ProjectionLease(
      const MortonYao48RankedPairH0ProjectionLease&) = delete;
  MortonYao48RankedPairH0ProjectionLease& operator=(
      const MortonYao48RankedPairH0ProjectionLease&) = delete;

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool cuda_resident() const noexcept;
  [[nodiscard]] bool host_fake() const noexcept;
  [[nodiscard]] const MortonYao48RankedPairH0ProjectionAudit& audit()
      const noexcept {
    return audit_;
  }

 private:
  MortonYao48RankedPairH0ProjectionLease(
      MortonYao48RankedPairH0ProjectionAudit audit,
      std::shared_ptr<void> output_owner,
      std::shared_ptr<MortonYao48RankedPairTileCatalogLease> source_catalog,
      const void* device_records,
      int cuda_device,
      bool host_fake);

  MortonYao48RankedPairH0ProjectionAudit audit_{};
  std::shared_ptr<void> output_owner_;
  std::shared_ptr<MortonYao48RankedPairTileCatalogLease> source_catalog_;
  const void* device_records_{};
  int cuda_device_{-1};
  bool host_fake_{false};

  friend struct MortonYao48RankedPairH0ProjectionResult;
  friend class detail::
      Phase15MortonYao48RankedPairH0ProjectionPrivateViewAccess;
  friend MortonYao48RankedPairH0ProjectionResult
  project_morton_yao48_ranked_pair_h0_candidates(
      MortonYao48RankedPairTileCatalogLease&&,
      MortonYao48RankedPairH0ProjectionConfig);
};

struct MortonYao48RankedPairH0ProjectionResult {
  MortonYao48RankedPairH0ProjectionStatus status{
      MortonYao48RankedPairH0ProjectionStatus::capacity_exhausted};
  MortonYao48RankedPairH0ProjectionAudit audit{};
  std::optional<MortonYao48RankedPairH0ProjectionLease> projection;

  MortonYao48RankedPairH0ProjectionResult() = default;
  MortonYao48RankedPairH0ProjectionResult(
      MortonYao48RankedPairH0ProjectionResult&&) noexcept = default;
  MortonYao48RankedPairH0ProjectionResult& operator=(
      MortonYao48RankedPairH0ProjectionResult&&) noexcept = default;
  MortonYao48RankedPairH0ProjectionResult(
      const MortonYao48RankedPairH0ProjectionResult&) = delete;
  MortonYao48RankedPairH0ProjectionResult& operator=(
      const MortonYao48RankedPairH0ProjectionResult&) = delete;

  [[nodiscard]] bool complete() const noexcept {
    return status == MortonYao48RankedPairH0ProjectionStatus::
                         complete_resident_projection &&
           projection.has_value() && projection->cuda_resident();
  }
};

[[nodiscard]] MortonYao48RankedPairH0ProjectionResult
project_morton_yao48_ranked_pair_h0_candidates(
    MortonYao48RankedPairTileCatalogLease&& source_catalog,
    MortonYao48RankedPairH0ProjectionConfig config = {});

}  // namespace morsehgp3d::gpu
