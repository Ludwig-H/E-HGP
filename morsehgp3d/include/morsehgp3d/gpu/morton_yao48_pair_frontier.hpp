#pragma once

#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/hierarchy/yao48_cone.hpp"
#include "morsehgp3d/hierarchy/yao48_rank_cutoff.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t
    morton_yao48_pair_frontier_schema_version = 1U;
inline constexpr std::size_t
    morton_yao48_pair_frontier_maximum_closed_rank = 11U;
inline constexpr std::size_t
    morton_yao48_pair_frontier_maximum_witness_count =
        morton_yao48_pair_frontier_maximum_closed_rank - 1U;
inline constexpr std::string_view
    morton_yao48_pair_frontier_backend = "reference_cpu";
inline constexpr std::string_view
    morton_yao48_pair_frontier_profile = "hgp_reduced";
inline constexpr std::string_view
    morton_yao48_pair_frontier_mode =
        "budgeted_host_fake_morton_yao48_exact_frontier";
inline constexpr std::string_view
    morton_yao48_pair_frontier_deployment_status = "architecture_only";
inline constexpr std::string_view
    morton_yao48_pair_frontier_public_status = "not_claimed";
inline constexpr std::string_view
    morton_yao48_pair_frontier_proof_basis =
        "single_reverse_postorder_morton_prefix_traversal_per_owner_"
        "48_half_open_cone_arbitrary_K_witness_banks_exact_directional_"
        "leaf_cutoff_exact_closed_cone_aabb_mask_radial_3D_subtree_cutoff_"
        "canonical_point_id_survivors_streamed_mass_conservation_v1";
inline constexpr bool
    morton_yao48_pair_frontier_cuda_implementation_available = false;
inline constexpr std::size_t
    morton_yao48_pair_frontier_invalid_node_index =
        std::numeric_limits<std::size_t>::max();

enum class MortonYao48PairFrontierStatus : std::uint8_t {
  complete,
  budget_exhausted,
};

enum class MortonYao48PairFrontierStopReason : std::uint8_t {
  none,
  node_visit_capacity,
  candidate_capacity,
  witness_bank_receipt_capacity,
  certified_prune_region_capacity,
};

enum class MortonYao48CertifiedPruneKind : std::uint8_t {
  directional_leaf,
  radial_subtree,
};

struct MortonYao48PairFrontierConfig {
  // A bank contains maximum_closed_rank - 1 non-support witnesses.  A Yao48
  // cutoff then proves closed rank at least maximum_closed_rank + 1.
  std::size_t maximum_closed_rank{2U};

  friend bool operator==(
      const MortonYao48PairFrontierConfig&,
      const MortonYao48PairFrontierConfig&) = default;
};

// Every limit applies to one advance call.  Output records and the scalar
// traversal cursor are committed atomically, so any exhausted call can be
// resumed without duplicating or omitting a pair.  There is deliberately no
// dense fallback.
struct MortonYao48PairFrontierAdvanceBudget {
  std::size_t maximum_node_visit_count{};
  std::size_t maximum_candidate_count{};
  std::size_t maximum_witness_bank_receipt_count{};
  std::size_t maximum_certified_prune_region_count{};

  friend bool operator==(
      const MortonYao48PairFrontierAdvanceBudget&,
      const MortonYao48PairFrontierAdvanceBudget&) = default;
};

// Morton order assigns one operational owner; canonical PointId order is the
// only scientific/output orientation.  This is a survivor proposal, not an
// exact rank decision.
struct MortonYao48PairCandidate {
  std::uint64_t source_snapshot_epoch{};
  std::array<spatial::PointId, 2> support_ids{};
  std::uint64_t morton_owner_position{};
  std::uint64_t morton_partner_position{};
  std::size_t owner_cone_index{};
  std::size_t maximum_closed_rank{};

  friend bool operator==(
      const MortonYao48PairCandidate&,
      const MortonYao48PairCandidate&) = default;
};

// A bank freezes when it first reaches K witnesses.  Later candidates never
// replace those witnesses: any K distinct same-cone witnesses bounded by D
// are sufficient.  The source epoch and full identity tuple prevent a prune
// from being replayed against another tree, anchor, chamber or rank cap.
struct MortonYao48WitnessBankReceipt {
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t anchor_position{};
  spatial::PointId anchor_point_id{};
  std::size_t cone_index{};
  std::size_t maximum_closed_rank{};
  std::array<spatial::PointId,
             morton_yao48_pair_frontier_maximum_witness_count>
      witness_point_ids{};
  std::size_t witness_count{};
  exact::ExactLevel witness_squared_radius_upper_bound{};

  friend bool operator==(
      const MortonYao48WitnessBankReceipt&,
      const MortonYao48WitnessBankReceipt&) = default;
};

// One safe directed orientation is enough to reject an unordered pair.  A
// leaf record uses one full half-open-cone bank and the exact three Yao48
// inequalities.  A subtree record uses the exact closed-cone AABB mask, full
// banks for every intersected chamber, and the conservative radial bound
// min_distance_squared(AABB) >= max(3*D_cone).
struct MortonYao48CertifiedPruneRegion {
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t anchor_position{};
  spatial::PointId anchor_point_id{};
  std::uint64_t node_index{};
  std::uint64_t leaf_begin{};
  std::uint64_t leaf_end{};
  std::uint64_t cone_mask{};
  std::uint64_t certified_pair_mass{};
  std::size_t maximum_closed_rank{};
  MortonYao48CertifiedPruneKind kind{
      MortonYao48CertifiedPruneKind::directional_leaf};

  friend bool operator==(
      const MortonYao48CertifiedPruneRegion&,
      const MortonYao48CertifiedPruneRegion&) = default;
};

struct MortonYao48PairFrontierAudit {
  std::uint32_t schema_version{
      morton_yao48_pair_frontier_schema_version};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t maximum_closed_rank{};
  std::size_t required_witness_count{};
  std::size_t completed_anchor_count{};
  std::size_t next_anchor_position{};
  std::size_t active_node_index{
      morton_yao48_pair_frontier_invalid_node_index};
  std::size_t active_full_witness_bank_count{};
  std::size_t active_retained_witness_count{};
  std::size_t peak_active_retained_witness_count{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t unordered_pair_universe_count{};
  std::uint64_t active_anchor_candidate_pair_mass{};
  std::uint64_t active_anchor_certified_pruned_pair_mass{};
  std::uint64_t active_anchor_suffix_leaf_mass{};
  std::uint64_t cumulative_candidate_pair_mass{};
  std::uint64_t cumulative_certified_pruned_pair_mass{};
  std::uint64_t unresolved_pair_mass{};
  std::uint64_t cumulative_node_visit_count{};
  std::uint64_t cumulative_internal_node_descent_count{};
  std::uint64_t cumulative_boundary_node_descent_count{};
  std::uint64_t cumulative_suffix_skip_node_count{};
  std::uint64_t cumulative_suffix_skipped_leaf_mass{};
  std::uint64_t cumulative_leaf_visit_count{};
  std::uint64_t cumulative_exact_cone_classification_count{};
  std::uint64_t cumulative_exact_witness_distance_evaluation_count{};
  std::uint64_t cumulative_frozen_witness_bank_count{};
  std::uint64_t cumulative_witness_bank_receipt_count{};
  std::uint64_t cumulative_directional_leaf_cutoff_evaluation_count{};
  std::uint64_t cumulative_directional_leaf_prune_count{};
  std::uint64_t cumulative_closed_cone_aabb_test_count{};
  std::uint64_t cumulative_exact_aabb_lower_bound_evaluation_count{};
  std::uint64_t cumulative_radial_subtree_prune_count{};
  std::uint64_t cumulative_radial_subtree_pruned_pair_mass{};
  std::uint64_t cumulative_certified_prune_region_count{};
  bool source_build_authenticated{false};
  bool source_cloud_authenticated{false};
  bool morton_point_id_total_order_validated{false};
  bool strict_postorder_interval_tree_validated{false};
  bool every_completed_anchor_partition_validated{false};
  bool candidate_plus_certified_pruned_mass_validated{false};
  bool frontier_complete{false};
  bool traversal_lease_owner_retained{false};
  bool host_fake_reference_execution{false};
  bool second_host_snapshot_retained{false};
  bool active_storage_bounded_by_48_times_k{false};
  bool unfiltered_owned_subtree_emitted{false};
  bool dense_pair_fallback_performed{false};
  bool global_pair_matrix_materialized{false};
  bool exact_diametral_rank_predicate_evaluated{false};
  bool cuda_execution_performed{false};
  bool scientific_decision_published{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const MortonYao48PairFrontierAudit&,
      const MortonYao48PairFrontierAudit&) = default;
};

struct MortonYao48PairFrontierAdvance {
  MortonYao48PairFrontierStatus status{
      MortonYao48PairFrontierStatus::budget_exhausted};
  MortonYao48PairFrontierStopReason stop_reason{
      MortonYao48PairFrontierStopReason::node_visit_capacity};
  std::vector<MortonYao48PairCandidate> candidates;
  std::vector<MortonYao48WitnessBankReceipt> witness_bank_receipts;
  std::vector<MortonYao48CertifiedPruneRegion> certified_prune_regions;
  MortonYao48PairFrontierAudit audit{};
};

// Host/fake executable specification of the fused product traversal.  Each
// Morton-high endpoint owns its strict prefix and traverses the shared LBVH in
// reverse postorder.  Owned internal nodes are never emitted as unfiltered
// work: they either receive an exact Yao48 range certificate or descend to
// leaf survivors.  The worst case remains quadratic, but bounded exhaustion
// is terminal and never switches to an exhaustive pair scan.
class MortonYao48PairFrontierContext final {
 public:
  static constexpr std::string_view backend =
      morton_yao48_pair_frontier_backend;
  static constexpr std::string_view profile =
      morton_yao48_pair_frontier_profile;
  static constexpr std::string_view mode =
      morton_yao48_pair_frontier_mode;
  static constexpr std::string_view deployment_status =
      morton_yao48_pair_frontier_deployment_status;
  static constexpr std::string_view public_status =
      morton_yao48_pair_frontier_public_status;
  static constexpr std::string_view proof_basis =
      morton_yao48_pair_frontier_proof_basis;

  MortonYao48PairFrontierContext(
      MortonLbvhDeviceTraversalLease&& traversal_lease,
      MortonYao48PairFrontierConfig config);
  ~MortonYao48PairFrontierContext() noexcept = default;

  MortonYao48PairFrontierContext(
      MortonYao48PairFrontierContext&&) noexcept = default;
  MortonYao48PairFrontierContext& operator=(
      MortonYao48PairFrontierContext&&) noexcept = default;
  MortonYao48PairFrontierContext(
      const MortonYao48PairFrontierContext&) = delete;
  MortonYao48PairFrontierContext& operator=(
      const MortonYao48PairFrontierContext&) = delete;

  [[nodiscard]] MortonYao48PairFrontierAdvance advance(
      const MortonLbvhDeviceBuildResult& certified_build,
      const spatial::CanonicalPointCloud& cloud,
      MortonYao48PairFrontierAdvanceBudget budget);

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] std::size_t point_count() const noexcept;
  [[nodiscard]] std::size_t certified_node_count() const noexcept;
  [[nodiscard]] const MortonYao48PairFrontierConfig& config()
      const noexcept {
    return config_;
  }

 private:
  struct WitnessBank {
    std::array<spatial::PointId,
               morton_yao48_pair_frontier_maximum_witness_count>
        point_ids{};
    std::size_t count{};
    exact::ExactLevel squared_radius_upper_bound{};
  };

  void bind_or_validate_source(
      const MortonLbvhDeviceBuildResult& certified_build,
      const spatial::CanonicalPointCloud& cloud);
  void complete_active_anchor();
  void reset_active_witness_banks() noexcept;
  [[nodiscard]] MortonYao48PairFrontierAudit audit() const noexcept;

  MortonYao48PairFrontierConfig config_{};
  std::optional<MortonLbvhDeviceTraversalLease> traversal_lease_;
  std::shared_ptr<const void> source_index_identity_;
  std::shared_ptr<const void> source_cloud_identity_;
  std::array<WitnessBank, hierarchy::exact_yao48_cone_count>
      active_witness_banks_{};
  std::size_t point_count_{};
  std::size_t certified_node_count_{};
  std::size_t completed_anchor_count_{};
  std::size_t next_anchor_position_{};
  std::size_t active_node_index_{
      morton_yao48_pair_frontier_invalid_node_index};
  std::size_t active_full_witness_bank_count_{};
  std::size_t active_retained_witness_count_{};
  std::size_t peak_active_retained_witness_count_{};
  std::uint64_t unordered_pair_universe_count_{};
  std::uint64_t active_anchor_candidate_pair_mass_{};
  std::uint64_t active_anchor_certified_pruned_pair_mass_{};
  std::uint64_t active_anchor_suffix_leaf_mass_{};
  std::uint64_t cumulative_candidate_pair_mass_{};
  std::uint64_t cumulative_certified_pruned_pair_mass_{};
  std::uint64_t cumulative_node_visit_count_{};
  std::uint64_t cumulative_internal_node_descent_count_{};
  std::uint64_t cumulative_boundary_node_descent_count_{};
  std::uint64_t cumulative_suffix_skip_node_count_{};
  std::uint64_t cumulative_suffix_skipped_leaf_mass_{};
  std::uint64_t cumulative_leaf_visit_count_{};
  std::uint64_t cumulative_exact_cone_classification_count_{};
  std::uint64_t cumulative_exact_witness_distance_evaluation_count_{};
  std::uint64_t cumulative_frozen_witness_bank_count_{};
  std::uint64_t cumulative_witness_bank_receipt_count_{};
  std::uint64_t cumulative_directional_leaf_cutoff_evaluation_count_{};
  std::uint64_t cumulative_directional_leaf_prune_count_{};
  std::uint64_t cumulative_closed_cone_aabb_test_count_{};
  std::uint64_t cumulative_exact_aabb_lower_bound_evaluation_count_{};
  std::uint64_t cumulative_radial_subtree_prune_count_{};
  std::uint64_t cumulative_radial_subtree_pruned_pair_mass_{};
  std::uint64_t cumulative_certified_prune_region_count_{};
  bool source_build_authenticated_{false};
  bool source_cloud_authenticated_{false};
  bool morton_point_id_total_order_validated_{false};
  bool strict_postorder_interval_tree_validated_{false};
  bool every_completed_anchor_partition_validated_{true};
};

}  // namespace morsehgp3d::gpu
