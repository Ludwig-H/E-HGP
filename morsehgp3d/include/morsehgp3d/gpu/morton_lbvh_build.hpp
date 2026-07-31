#pragma once

#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace morsehgp3d::gpu {

class DirectSparseFacetTopKProposalContext;
class AnchoredPairCandidateProposalContext;
class MortonLbvhDeviceTraversalLease;
class MortonYao48PairFrontierContext;

namespace detail {
class Phase14MortonLbvhBuildContextState;
class Phase15ExactPairBlockWitnessCudaTraversalAccess;
class Phase15ExactPairBlockTransactionalFrontierResidentCudaTraversalAccess;
struct Phase15MortonYao48DeviceTiledAdoptedTraversal;
[[nodiscard]] Phase15MortonYao48DeviceTiledAdoptedTraversal
adopt_phase15_morton_yao48_device_tiled_traversal(
    MortonLbvhDeviceTraversalLease&& traversal_lease);
}

inline constexpr std::uint32_t morton_lbvh_device_build_schema_version = 1U;
inline constexpr std::uint32_t morton_lbvh_device_lease_schema_version = 1U;
inline constexpr std::uint32_t
    morton_lbvh_device_traversal_lease_schema_version = 1U;
inline constexpr std::string_view morton_lbvh_device_build_backend =
    "cuda_g4";
inline constexpr std::string_view morton_lbvh_device_build_profile =
    "hgp_reduced";
inline constexpr std::string_view morton_lbvh_device_build_mode =
    "device_morton_lbvh_snapshot_import";
inline constexpr std::string_view morton_lbvh_device_build_deployment_status =
    "architecture_only";
inline constexpr std::string_view morton_lbvh_device_build_public_status =
    "not_claimed";
inline constexpr std::string_view morton_lbvh_device_build_proof_basis =
    "directed_binary64_morton_bin_proposal_compact_exact_cpu_ambiguity_"
    "fallback_stable_morton_point_id_sort_strict_find_split_postorder_"
    "minimum_point_id_aabb_witnesses_then_cpu_certified_snapshot_import_v1";
inline constexpr std::string_view morton_lbvh_device_lease_backend =
    "cuda_g4_plus_reference_cpu";
inline constexpr std::string_view morton_lbvh_device_lease_profile =
    "hgp_reduced";
inline constexpr std::string_view morton_lbvh_device_lease_mode =
    "device_morton_lbvh_lease";
inline constexpr std::string_view
    morton_lbvh_device_lease_deployment_status = "architecture_only";
inline constexpr std::string_view morton_lbvh_device_lease_public_status =
    "not_claimed";
inline constexpr std::string_view
    morton_lbvh_device_traversal_lease_backend =
        "cuda_g4_plus_reference_cpu";
inline constexpr std::string_view
    morton_lbvh_device_traversal_lease_profile = "hgp_reduced";
inline constexpr std::string_view
    morton_lbvh_device_traversal_lease_mode =
        "device_morton_lbvh_traversal_lease";
inline constexpr std::string_view
    morton_lbvh_device_traversal_lease_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    morton_lbvh_device_traversal_lease_public_status = "not_claimed";

enum class MortonLbvhDeviceBuildDecision : std::uint8_t {
  complete,
  capacity_exhausted,
};

enum class MortonLbvhDeviceBuildStopReason : std::uint8_t {
  none,
  point_capacity,
};

struct MortonLbvhDeviceBuildAudit {
  // Immutable logical capacities.  A CUDA implementation may allocate the
  // corresponding storage lazily, but no call is allowed to exceed them.
  std::size_t maximum_point_count{};
  std::size_t maximum_axis_count{};
  std::size_t maximum_node_count{};
  std::size_t host_coordinate_word_capacity{};
  std::size_t host_encoded_bin_capacity{};
  std::size_t host_morton_code_capacity{};
  std::size_t host_snapshot_leaf_capacity{};
  std::size_t host_snapshot_node_capacity{};
  std::size_t host_snapshot_byte_capacity{};

  // Required extents are reported even for a preflight refusal.
  std::size_t point_count{};
  std::size_t required_axis_count{};
  std::size_t required_node_count{};
  std::size_t required_snapshot_byte_count{};

  // The first pass proposes one encoded bin per point/axis.  Only records
  // carrying the ambiguity bit are recomputed with ExactRational on the host.
  std::size_t device_bin_proposal_count{};
  std::size_t device_unambiguous_axis_count{};
  std::size_t device_ambiguous_axis_count{};
  std::size_t cpu_exact_fallback_axis_count{};

  // The current certified import independently replays all exact Morton
  // coordinates.  Keeping this counter separate makes that Phase 14M
  // performance debt explicit instead of attributing it to ambiguity.
  std::size_t cpu_import_exact_morton_axis_recertification_count{};
  std::size_t cpu_exact_point_cloud_aabb_scan_count{};
  std::size_t cpu_exact_extremum_comparison_count{};

  std::size_t host_to_device_coordinate_byte_count{};
  std::size_t device_to_host_encoded_bin_byte_count{};
  std::size_t host_to_device_morton_code_byte_count{};
  std::size_t device_to_host_leaf_byte_count{};
  std::size_t device_to_host_node_byte_count{};
  std::size_t device_coordinate_byte_capacity{};
  std::size_t device_encoded_bin_byte_capacity{};
  std::size_t device_morton_code_double_buffer_byte_capacity{};
  std::size_t device_point_id_double_buffer_byte_capacity{};
  std::size_t device_leaf_byte_capacity{};
  std::size_t device_node_byte_capacity{};
  std::size_t device_frontier_double_buffer_byte_capacity{};
  std::size_t device_level_schedule_byte_capacity{};
  std::size_t device_control_byte_capacity{};
  std::size_t device_sort_temporary_byte_capacity{};
  std::size_t total_fixed_device_byte_capacity{};
  // Project-owned kernels only. Opaque CUB calls are counted separately.
  std::size_t device_kernel_launch_count{};
  std::size_t device_library_submission_count{};
  std::size_t device_synchronization_count{};
  std::uint64_t proposal_buffer_epoch{};
  std::uint64_t snapshot_buffer_epoch{};

  bool fixed_capacity_preflight_satisfied{false};
  bool every_ambiguous_axis_exactly_resolved{false};
  bool encoded_bin_extent_validated{false};
  bool stable_morton_point_id_order_validated{false};
  bool strict_find_split_postorder_requested{false};
  bool minimum_point_id_aabb_witness_rule_requested{false};
  bool snapshot_import_certified{false};
  bool cpu_reference_build_invoked{false};
  bool gpu_execution_performed{false};
  bool cuda_builder_qualified{false};
  bool fake_launcher_executed{false};
  bool higher_order_delaunay_mosaic_materialized{false};
  bool global_cell_or_coface_arena_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const MortonLbvhDeviceBuildAudit&,
      const MortonLbvhDeviceBuildAudit&) = default;
};

// Phase 14N keeps only the two device authorities needed by subsequent
// batched proposal contexts: 3*C canonical binary64 coordinate words and
// C Morton-ordered PointIds.  The CPU index remains the sole host LBVH;
// extracting a lease never retains the imported leaf/node snapshot.
struct MortonLbvhDeviceLeaseAudit {
  std::size_t maximum_point_count{};
  std::size_t point_count{};
  std::size_t retained_coordinate_word_capacity{};
  std::size_t retained_morton_point_id_capacity{};
  std::size_t retained_coordinate_byte_capacity{};
  std::size_t retained_morton_point_id_byte_capacity{};
  std::size_t persistent_device_byte_capacity{};
  std::size_t source_fixed_device_byte_capacity{};
  std::size_t released_transient_device_byte_capacity{};
  std::size_t retained_host_snapshot_byte_count{};
  std::uint64_t source_snapshot_epoch{};

  bool source_snapshot_import_certified{false};
  bool canonical_coordinate_words_retained{false};
  bool active_morton_point_ids_retained{false};
  bool builder_transients_released{false};
  bool cuda_device_storage_retained{false};
  bool host_fake_lifecycle_exercised{false};
  bool second_host_snapshot_retained{false};
  bool higher_order_delaunay_mosaic_materialized{false};
  bool global_cell_or_coface_arena_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const MortonLbvhDeviceLeaseAudit&,
      const MortonLbvhDeviceLeaseAudit&) = default;
};

class MortonLbvhDeviceLease final {
 public:
  static constexpr std::string_view backend =
      morton_lbvh_device_lease_backend;
  static constexpr std::string_view profile =
      morton_lbvh_device_lease_profile;
  static constexpr std::string_view mode =
      morton_lbvh_device_lease_mode;
  static constexpr std::string_view deployment_status =
      morton_lbvh_device_lease_deployment_status;
  static constexpr std::string_view public_status =
      morton_lbvh_device_lease_public_status;

  ~MortonLbvhDeviceLease() noexcept = default;
  MortonLbvhDeviceLease(MortonLbvhDeviceLease&&) noexcept = default;
  MortonLbvhDeviceLease& operator=(
      MortonLbvhDeviceLease&&) noexcept = default;
  MortonLbvhDeviceLease(const MortonLbvhDeviceLease&) = delete;
  MortonLbvhDeviceLease& operator=(
      const MortonLbvhDeviceLease&) = delete;

  [[nodiscard]] std::uint32_t schema_version() const noexcept {
    return schema_version_;
  }
  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool cuda_resident() const noexcept;
  [[nodiscard]] const MortonLbvhDeviceLeaseAudit& audit() const noexcept {
    return audit_;
  }

 private:
  MortonLbvhDeviceLease(
      MortonLbvhDeviceLeaseAudit audit,
      std::shared_ptr<void> retained_resources,
      std::shared_ptr<const void> source_cloud_identity,
      const std::uint64_t* device_coordinate_bits,
      const std::uint64_t* device_morton_point_ids,
      int cuda_device);

  std::uint32_t schema_version_{morton_lbvh_device_lease_schema_version};
  MortonLbvhDeviceLeaseAudit audit_{};
  std::shared_ptr<void> retained_resources_;
  // The raw addresses are an opaque cross-translation-unit CUDA view.  They
  // are never exposed without transferring retained_resources_, which owns
  // both allocations and therefore dominates their lifetime.
  std::shared_ptr<const void> source_cloud_identity_;
  const std::uint64_t* device_coordinate_bits_{};
  const std::uint64_t* device_morton_point_ids_{};
  int cuda_device_{-1};

  friend class DirectSparseFacetTopKProposalContext;
  friend class MortonLbvhBuildContext;
};

// Traversal consumers additionally retain the certified 80-byte device-node
// arena.  Coordinates, Morton-ordered PointIds and nodes remain private,
// opaque device views whose lifetime is dominated by retained_resources_.
// The sole CPU index remains in MortonLbvhDeviceBuildResult; no second host
// snapshot is retained by this lease.
struct MortonLbvhDeviceTraversalLeaseAudit {
  std::size_t maximum_point_count{};
  std::size_t maximum_node_count{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t retained_coordinate_word_capacity{};
  std::size_t retained_morton_point_id_capacity{};
  std::size_t retained_node_capacity{};
  std::size_t retained_coordinate_byte_capacity{};
  std::size_t retained_morton_point_id_byte_capacity{};
  std::size_t retained_node_byte_capacity{};
  std::size_t persistent_device_byte_capacity{};
  std::size_t source_fixed_device_byte_capacity{};
  std::size_t released_transient_device_byte_capacity{};
  std::size_t retained_host_snapshot_byte_count{};
  std::uint64_t source_snapshot_epoch{};

  bool source_snapshot_import_certified{false};
  bool canonical_coordinate_words_retained{false};
  bool active_morton_point_ids_retained{false};
  bool certified_device_nodes_retained{false};
  bool builder_transients_released{false};
  bool cuda_device_storage_retained{false};
  bool host_fake_lifecycle_exercised{false};
  bool second_host_snapshot_retained{false};
  bool higher_order_delaunay_mosaic_materialized{false};
  bool global_cell_or_coface_arena_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const MortonLbvhDeviceTraversalLeaseAudit&,
      const MortonLbvhDeviceTraversalLeaseAudit&) = default;
};

class MortonLbvhDeviceTraversalLease final {
 public:
  static constexpr std::string_view backend =
      morton_lbvh_device_traversal_lease_backend;
  static constexpr std::string_view profile =
      morton_lbvh_device_traversal_lease_profile;
  static constexpr std::string_view mode =
      morton_lbvh_device_traversal_lease_mode;
  static constexpr std::string_view deployment_status =
      morton_lbvh_device_traversal_lease_deployment_status;
  static constexpr std::string_view public_status =
      morton_lbvh_device_traversal_lease_public_status;

  ~MortonLbvhDeviceTraversalLease() noexcept = default;
  MortonLbvhDeviceTraversalLease(
      MortonLbvhDeviceTraversalLease&&) noexcept = default;
  MortonLbvhDeviceTraversalLease& operator=(
      MortonLbvhDeviceTraversalLease&&) noexcept = default;
  MortonLbvhDeviceTraversalLease(
      const MortonLbvhDeviceTraversalLease&) = delete;
  MortonLbvhDeviceTraversalLease& operator=(
      const MortonLbvhDeviceTraversalLease&) = delete;

  [[nodiscard]] std::uint32_t schema_version() const noexcept {
    return schema_version_;
  }
  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool cuda_resident() const noexcept;
  [[nodiscard]] const MortonLbvhDeviceTraversalLeaseAudit& audit()
      const noexcept {
    return audit_;
  }

 private:
  MortonLbvhDeviceTraversalLease(
      MortonLbvhDeviceTraversalLeaseAudit audit,
      std::shared_ptr<void> retained_resources,
      std::shared_ptr<const void> source_cloud_identity,
      const std::uint64_t* device_coordinate_bits,
      const std::uint64_t* device_morton_point_ids,
      const void* device_nodes,
      int cuda_device);

  std::uint32_t schema_version_{
      morton_lbvh_device_traversal_lease_schema_version};
  MortonLbvhDeviceTraversalLeaseAudit audit_{};
  std::shared_ptr<void> retained_resources_;
  std::shared_ptr<const void> source_cloud_identity_;
  const std::uint64_t* device_coordinate_bits_{};
  const std::uint64_t* device_morton_point_ids_{};
  const void* device_nodes_{};
  int cuda_device_{-1};

  friend class AnchoredPairCandidateProposalContext;
  friend class Binary64LbvhTopKContext;
  friend class MortonLbvhBuildContext;
  friend class MortonYao48PairFrontierContext;
  friend class RankedDiametralPairCatalogContext;
  friend class detail::Phase15ExactPairBlockWitnessCudaTraversalAccess;
  friend class detail::
      Phase15ExactPairBlockTransactionalFrontierResidentCudaTraversalAccess;
  friend detail::Phase15MortonYao48DeviceTiledAdoptedTraversal
  detail::adopt_phase15_morton_yao48_device_tiled_traversal(
      MortonLbvhDeviceTraversalLease&& traversal_lease);
};

class MortonLbvhDeviceBuildResult final {
 public:
  static constexpr std::string_view backend =
      morton_lbvh_device_build_backend;
  static constexpr std::string_view profile =
      morton_lbvh_device_build_profile;
  static constexpr std::string_view mode =
      morton_lbvh_device_build_mode;
  static constexpr std::string_view deployment_status =
      morton_lbvh_device_build_deployment_status;
  static constexpr std::string_view public_status =
      morton_lbvh_device_build_public_status;
  static constexpr std::string_view proof_basis =
      morton_lbvh_device_build_proof_basis;

  MortonLbvhDeviceBuildResult(
      MortonLbvhDeviceBuildResult&&) noexcept = default;
  MortonLbvhDeviceBuildResult& operator=(
      MortonLbvhDeviceBuildResult&&) noexcept = default;
  MortonLbvhDeviceBuildResult(
      const MortonLbvhDeviceBuildResult&) = delete;
  MortonLbvhDeviceBuildResult& operator=(
      const MortonLbvhDeviceBuildResult&) = delete;

  [[nodiscard]] std::uint32_t schema_version() const noexcept {
    return schema_version_;
  }
  [[nodiscard]] MortonLbvhDeviceBuildDecision decision() const noexcept {
    return decision_;
  }
  [[nodiscard]] MortonLbvhDeviceBuildStopReason stop_reason() const noexcept {
    return stop_reason_;
  }
  [[nodiscard]] bool complete_certified_build() const noexcept;
  [[nodiscard]] bool cuda_qualified_build() const noexcept;
  [[nodiscard]] const MortonLbvhDeviceBuildAudit& audit() const noexcept {
    return audit_;
  }
  [[nodiscard]] const spatial::MortonLbvhIndex& certified_index() const &;
  [[nodiscard]] const spatial::MortonLbvhIndex& certified_index() const && =
      delete;

 private:
  MortonLbvhDeviceBuildResult(
      MortonLbvhDeviceBuildDecision decision,
      MortonLbvhDeviceBuildStopReason stop_reason,
      MortonLbvhDeviceBuildAudit audit,
      std::optional<spatial::MortonLbvhIndex> certified_index,
      std::weak_ptr<detail::Phase14MortonLbvhBuildContextState>
          source_context_state);

  std::uint32_t schema_version_{morton_lbvh_device_build_schema_version};
  MortonLbvhDeviceBuildDecision decision_{
      MortonLbvhDeviceBuildDecision::capacity_exhausted};
  MortonLbvhDeviceBuildStopReason stop_reason_{
      MortonLbvhDeviceBuildStopReason::point_capacity};
  MortonLbvhDeviceBuildAudit audit_{};
  std::optional<spatial::MortonLbvhIndex> certified_index_;
  std::weak_ptr<detail::Phase14MortonLbvhBuildContextState>
      source_context_state_;

  friend class MortonLbvhBuildContext;
};

// Phase 14M owns only arrays linear in n: coordinates, encoded bins, Morton
// pairs and the 2n-1 LBVH snapshot.  It never materializes a higher-order
// Delaunay mosaic, a global cell complex, cofaces or incidences.  Calls are
// synchronous and serialized; malformed launcher output poisons the context.
class MortonLbvhBuildContext final {
 public:
  explicit MortonLbvhBuildContext(std::size_t maximum_point_count);
  ~MortonLbvhBuildContext() noexcept;

  MortonLbvhBuildContext(MortonLbvhBuildContext&&) noexcept;
  MortonLbvhBuildContext& operator=(MortonLbvhBuildContext&&) noexcept;

  MortonLbvhBuildContext(const MortonLbvhBuildContext&) = delete;
  MortonLbvhBuildContext& operator=(const MortonLbvhBuildContext&) = delete;

  [[nodiscard]] MortonLbvhDeviceBuildResult build(
      const spatial::CanonicalPointCloud& cloud);

  // The supplied result must be the latest complete 14M import produced by
  // this exact context.  On success the builder's transient device arenas are
  // released and the returned move-only lease owns the compact 32*C state.
  [[nodiscard]] MortonLbvhDeviceLease release_device_lease(
      const MortonLbvhDeviceBuildResult& certified_build);

  // Sibling extraction for device-side tree traversal.  It has the same
  // single-use latest-build capability as the compact 14N lease, but also
  // retains the certified 80-byte node arena (192*C-80 bytes in total).
  [[nodiscard]] MortonLbvhDeviceTraversalLease
  release_device_traversal_lease(
      const MortonLbvhDeviceBuildResult& certified_build);

  [[nodiscard]] std::size_t maximum_point_count() const noexcept;
  [[nodiscard]] std::size_t maximum_node_count() const noexcept;

 private:
  std::shared_ptr<detail::Phase14MortonLbvhBuildContextState> state_;
  std::size_t maximum_point_count_{};
  std::size_t maximum_axis_count_{};
  std::size_t maximum_node_count_{};
  std::uint64_t last_buffer_epoch_{};
  std::size_t last_completed_point_count_{};
  std::shared_ptr<const void> last_cloud_identity_;
  bool latest_device_build_available_for_lease_{false};
};

}  // namespace morsehgp3d::gpu
