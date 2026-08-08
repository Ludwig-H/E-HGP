#pragma once

#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace morsehgp3d::gpu {

namespace detail {
class Phase15JungChordCsrTilePrivateViewAccess;
class Phase15JungChordCsrTileContextState;
}

inline constexpr std::uint32_t jung_chord_csr_tile_schema_version = 1U;
inline constexpr std::size_t jung_chord_csr_tile_maximum_closed_rank = 11U;
inline constexpr std::size_t jung_chord_csr_tile_maximum_anchor_capacity =
    1U << 20U;
inline constexpr std::string_view jung_chord_csr_tile_backend =
    "cuda_g4_plus_reference_cpu_oracle";
inline constexpr std::string_view jung_chord_csr_tile_profile = "hgp_reduced";
inline constexpr std::string_view jung_chord_csr_tile_mode =
    "proposal_only_shallow_higher_support_work_falsifier_v1";
inline constexpr std::string_view jung_chord_csr_tile_deployment_status =
    "prototype_only";
inline constexpr std::string_view jung_chord_csr_tile_public_status =
    "not_claimed";

// The source of these anchors is deliberately explicit and external.  This
// prototype proves no global anchor completeness theorem: in particular, an
// RNG, a ranked-pair catalog, or a finite Jung growth must remain proposal-only.
struct JungChordAnchor final {
  spatial::PointId support_u{};
  spatial::PointId support_v{};

  friend auto operator<=>(const JungChordAnchor&, const JungChordAnchor&) =
      default;
};

static_assert(sizeof(JungChordAnchor) == 16U);

struct JungChordAnchorTile final {
  // The producer names its proposal source for diagnostics.  It is never a
  // proof-basis identifier and cannot promote the output to a scientific source.
  std::string anchor_source{"explicit_proposal_only"};
  std::size_t source_window{};
  std::vector<JungChordAnchor> anchors;
};

struct JungChordCsrTileConfig final {
  std::size_t support_size{4U};
  std::size_t maximum_relevant_closed_rank{11U};
  std::size_t anchor_capacity{4096U};
  std::size_t chord_point_id_capacity{1U << 20U};
  bool collect_per_anchor_diagnostics{false};

  friend bool operator==(
      const JungChordCsrTileConfig&,
      const JungChordCsrTileConfig&) = default;
};

enum class JungChordCsrTileStatus : std::uint8_t {
  tile_complete = 0U,
  capacity_exhausted = 1U,
  malformed_input = 2U,
  execution_failure = 3U,
};

enum class JungChordCsrTileStopReason : std::uint8_t {
  none = 0U,
  anchor_capacity = 1U,
  chord_point_id_capacity = 2U,
  malformed_input = 3U,
  traversal_failure = 4U,
  count_emit_mismatch = 5U,
};

struct JungChordCsrTileAudit final {
  std::uint32_t schema_version{jung_chord_csr_tile_schema_version};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t tile_epoch{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t support_size{};
  std::size_t maximum_relevant_closed_rank{};
  std::size_t maximum_constant_interior_count{};
  std::size_t anchor_capacity{};
  std::size_t chord_point_id_capacity{};
  std::size_t anchor_count{};
  std::size_t required_chord_point_id_count{};
  std::size_t committed_chord_point_id_count{};
  std::size_t depth_rejected_anchor_count{};
  std::size_t degenerate_anchor_count{};
  std::uint64_t constant_interior_lower_bound_sum{};
  std::uint64_t count_pass_node_visit_count{};
  std::uint64_t emit_pass_node_visit_count{};
  std::uint64_t count_pass_leaf_test_count{};
  std::uint64_t emit_pass_leaf_test_count{};
  std::uint64_t bulk_constant_interior_point_count{};
  std::uint64_t bulk_constant_exterior_point_count{};
  std::uint64_t certified_outward_jung_range_pruned_point_count{};
  std::uint64_t range_candidate_point_count_sum{};
  std::uint64_t certified_active_chord_count{};
  std::uint64_t ambiguous_fail_open_chord_count{};
  std::uint64_t support_eligible_chord_count{};
  std::uint64_t support_eligible_ambiguous_chord_count{};
  std::uint64_t quadratic_chord_pair_baseline_count{};
  std::size_t range_candidate_count_p50{};
  std::size_t range_candidate_count_p95{};
  std::size_t range_candidate_count_p99{};
  std::size_t range_candidate_count_maximum{};
  std::size_t chord_count_p50{};
  std::size_t chord_count_p95{};
  std::size_t chord_count_p99{};
  std::size_t chord_count_maximum{};
  std::size_t support_eligible_count_p50{};
  std::size_t support_eligible_count_p95{};
  std::size_t support_eligible_count_p99{};
  std::size_t support_eligible_count_maximum{};
  std::size_t physical_device_arena_capacity_bytes{};
  std::size_t required_device_arena_capacity_bytes{};
  std::size_t retained_output_device_byte_count{};
  std::size_t logical_output_device_byte_count{};
  std::size_t configured_capacity_upper_bound_byte_count{};
  std::size_t cub_scan_temporary_capacity_bytes{};
  std::size_t retained_source_coordinate_word_capacity{};
  std::size_t retained_source_morton_point_id_capacity{};
  std::size_t retained_source_node_capacity{};
  std::size_t anchor_host_to_device_byte_count{};
  std::size_t control_device_to_host_byte_count{};
  std::size_t chord_device_to_host_byte_count{};
  std::size_t qualification_snapshot_chord_count{};
  std::size_t cuda_kernel_launch_count{};
  std::size_t cuda_library_submission_count{};
  std::size_t cuda_synchronization_count{};
  std::uint64_t anchor_upload_nanoseconds{};
  std::uint64_t count_kernel_nanoseconds{};
  std::uint64_t scan_nanoseconds{};
  std::uint64_t capacity_preflight_nanoseconds{};
  std::uint64_t emit_kernel_nanoseconds{};
  std::uint64_t total_device_nanoseconds{};
  std::string anchor_source{"explicit_proposal_only"};
  std::size_t anchor_source_window{};
  bool source_identity_authenticated{false};
  bool source_epoch_authenticated{false};
  bool anchors_sorted_unique{false};
  bool count_scan_emit_used{false};
  bool per_anchor_diagnostics_collected{false};
  bool census_censored_by_depth_cutoff{false};
  bool stackless_lbvh_traversal_used{false};
  bool directed_binary64_intervals_used{false};
  bool ambiguity_emitted_fail_open{false};
  bool diameter_lens_support_eligibility_flagged{false};
  bool certified_outward_jung_range_bound_used{false};
  bool depth_cutoff_certified{false};
  bool output_publication_atomic{false};
  bool output_withheld{true};
  bool host_fake_launcher_exercised{false};
  bool cuda_execution_performed{false};
  bool cuda_device_storage_retained{false};
  bool source_device_coordinates_retained{false};
  bool source_device_morton_point_ids_retained{false};
  bool source_device_nodes_retained{false};
  bool chord_device_to_host_performed{false};
  bool quadratic_chord_pair_baseline_saturated{false};
  bool quadratic_chord_pair_baseline_executed{false};
  bool global_pair_matrix_materialized{false};
  bool support_tuple_universe_materialized{false};
  bool ordinary_delaunay_materialized{false};
  bool higher_order_delaunay_mosaic_materialized{false};
  bool global_cell_or_coface_arena_materialized{false};
  bool scientific_decision_published{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const JungChordCsrTileAudit&,
      const JungChordCsrTileAudit&) = default;
};

// Move-only capability for a device-resident CSR.  The public API exposes only
// its audit.  Chord PointIds remain private so a diagnostic D2H path cannot
// silently become the product architecture.
class JungChordCsrTileLease final {
 public:
  ~JungChordCsrTileLease() noexcept = default;
  JungChordCsrTileLease(JungChordCsrTileLease&&) noexcept = default;
  JungChordCsrTileLease& operator=(JungChordCsrTileLease&&) noexcept = default;
  JungChordCsrTileLease(const JungChordCsrTileLease&) = delete;
  JungChordCsrTileLease& operator=(const JungChordCsrTileLease&) = delete;

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool cuda_resident() const noexcept;
  [[nodiscard]] bool host_fake() const noexcept;
  [[nodiscard]] const JungChordCsrTileAudit& audit() const noexcept {
    return audit_;
  }

 private:
  JungChordCsrTileLease(
      JungChordCsrTileAudit audit,
      std::shared_ptr<void> retained_owner,
      std::shared_ptr<const void> source_cloud_identity,
      std::shared_ptr<const void> source_index_identity,
      const JungChordAnchor* device_anchors,
      const std::uint64_t* device_chord_offsets,
      const spatial::PointId* device_chord_point_ids,
      const std::uint8_t* device_chord_flags,
      const std::uint64_t* device_constant_interior_counts,
      const std::uint8_t* device_anchor_flags,
      const std::uint64_t* source_device_coordinate_bits,
      const std::uint64_t* source_device_morton_point_ids,
      const void* source_device_nodes,
      int cuda_device,
      bool host_fake);

  JungChordCsrTileAudit audit_{};
  std::shared_ptr<void> retained_owner_;
  std::shared_ptr<const void> source_cloud_identity_;
  std::shared_ptr<const void> source_index_identity_;
  const JungChordAnchor* device_anchors_{};
  const std::uint64_t* device_chord_offsets_{};
  const spatial::PointId* device_chord_point_ids_{};
  const std::uint8_t* device_chord_flags_{};
  const std::uint64_t* device_constant_interior_counts_{};
  const std::uint8_t* device_anchor_flags_{};
  const std::uint64_t* source_device_coordinate_bits_{};
  const std::uint64_t* source_device_morton_point_ids_{};
  const void* source_device_nodes_{};
  int cuda_device_{-1};
  bool host_fake_{false};

  friend class JungChordCsrTileContext;
  friend class detail::Phase15JungChordCsrTilePrivateViewAccess;
};

struct JungChordCsrTileAdvance final {
  JungChordCsrTileStatus status{JungChordCsrTileStatus::execution_failure};
  JungChordCsrTileStopReason stop_reason{
      JungChordCsrTileStopReason::traversal_failure};
  std::optional<JungChordCsrTileLease> tile;
  JungChordCsrTileAudit audit{};

  JungChordCsrTileAdvance() = default;
  JungChordCsrTileAdvance(JungChordCsrTileAdvance&&) noexcept = default;
  JungChordCsrTileAdvance& operator=(JungChordCsrTileAdvance&&) noexcept =
      default;
  JungChordCsrTileAdvance(const JungChordCsrTileAdvance&) = delete;
  JungChordCsrTileAdvance& operator=(const JungChordCsrTileAdvance&) = delete;
};

class JungChordCsrTileContext final {
 public:
  static constexpr std::string_view backend = jung_chord_csr_tile_backend;
  static constexpr std::string_view profile = jung_chord_csr_tile_profile;
  static constexpr std::string_view mode = jung_chord_csr_tile_mode;
  static constexpr std::string_view deployment_status =
      jung_chord_csr_tile_deployment_status;
  static constexpr std::string_view public_status =
      jung_chord_csr_tile_public_status;

  JungChordCsrTileContext(
      MortonLbvhDeviceTraversalLease&& traversal_lease,
      JungChordCsrTileConfig config = {});
  ~JungChordCsrTileContext() noexcept;
  JungChordCsrTileContext(JungChordCsrTileContext&&) noexcept;
  JungChordCsrTileContext& operator=(JungChordCsrTileContext&&) noexcept;
  JungChordCsrTileContext(const JungChordCsrTileContext&) = delete;
  JungChordCsrTileContext& operator=(const JungChordCsrTileContext&) = delete;

  [[nodiscard]] JungChordCsrTileAdvance classify(JungChordAnchorTile tile);
  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool poisoned() const noexcept;
  [[nodiscard]] const JungChordCsrTileConfig& config() const noexcept {
    return config_;
  }

 private:
  JungChordCsrTileConfig config_{};
  MortonLbvhDeviceTraversalLeaseAudit source_audit_{};
  std::shared_ptr<detail::Phase15JungChordCsrTileContextState> state_;
  std::shared_ptr<void> source_owner_;
  std::shared_ptr<const void> source_cloud_identity_;
  std::shared_ptr<const void> source_index_identity_;
  const std::uint64_t* device_coordinate_bits_{};
  const std::uint64_t* device_morton_point_ids_{};
  const void* device_nodes_{};
  int cuda_device_{-1};
  bool host_fake_{false};
  std::uint64_t next_tile_epoch_{1U};
  std::weak_ptr<void> outstanding_tile_lifetime_;
};

}  // namespace morsehgp3d::gpu
