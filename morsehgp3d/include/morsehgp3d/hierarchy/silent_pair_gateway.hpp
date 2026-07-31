#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t silent_pair_gateway_schema_version = 1U;
inline constexpr std::size_t silent_pair_gateway_order = 2U;
inline constexpr std::string_view silent_pair_gateway_backend =
    "reference_cpu";
inline constexpr std::string_view silent_pair_gateway_profile =
    "hgp_reduced";
inline constexpr std::string_view silent_pair_gateway_mode =
    "certified_regular_q3_silent_pair_gateway";
inline constexpr std::string_view silent_pair_gateway_deployment_status =
    "architecture_only";
inline constexpr std::string_view silent_pair_gateway_refinement_status =
    "partial_refinement";
inline constexpr std::string_view silent_pair_gateway_public_status =
    "not_claimed";
inline constexpr std::string_view silent_pair_gateway_proof_basis =
    "caller_certified_unique_essential_diametral_support_complete_no_extra_"
    "shell_two_canonical_strict_interiors_two_strict_bridge_cofaces_and_"
    "common_frozen_pre_batch_reduced_root_v1";

using ExactSilentPairGatewayPairIds =
    std::array<spatial::PointId, 2U>;
using ExactSilentPairGatewayCofaceIds =
    std::array<spatial::PointId, 3U>;
using ExactSilentPairGatewayComponentHandle = std::size_t;

// This input is one already-certified exact decision.  The gateway does not
// own the geometric authority that produced it and does not rescan a cloud.
// Fresh scientific replay therefore means replaying this relative
// sub-contract from the same caller-owned decisions, not recertifying their
// geometry.
struct ExactSilentPairGatewayStrictInteriorDecision {
  spatial::PointId point_id{};
  exact::ExactLevel squared_distance_to_diametral_center{};
  contract::CanonicalId source_decision_id{};
  bool exact_strict_interior_certified{false};

  friend bool operator==(
      const ExactSilentPairGatewayStrictInteriorDecision&,
      const ExactSilentPairGatewayStrictInteriorDecision&) = default;
};

// The counts describe a complete partition of the cloud by the diametral
// ball: two support points, strict interiors, extra shell and exterior.
// Acceptance requires extra_shell_count == 0 and exact count accounting.
struct ExactSilentPairGatewayClosedBallDecision {
  std::size_t strict_interior_count{};
  std::size_t extra_shell_count{};
  std::size_t exterior_count{};
  contract::CanonicalId source_decision_id{};
  bool exact_diametral_pair_level_certified{false};
  bool unique_essential_diametral_support_certified{false};
  bool complete_closed_ball_partition_certified{false};
  bool canonical_two_witness_selection_certified{false};

  friend bool operator==(
      const ExactSilentPairGatewayClosedBallDecision&,
      const ExactSilentPairGatewayClosedBallDecision&) = default;
};

// With canonical support {p0,p1} and canonical witnesses {w0,w1}, the leg
// omitting pi must describe the strict path
//
//   {p(1-i),w0} -- {w0,w1}
//
// carried by the strict coface {p(1-i),w0,w1}.  The remaining certified path
// to the supplied root may be longer; its maximum exact level is retained.
struct ExactSilentPairGatewayStrictLegDecision {
  spatial::PointId removed_support_point_id{};
  ExactSilentPairGatewayPairIds strict_facet_point_ids{};
  ExactSilentPairGatewayPairIds bridge_facet_point_ids{};
  ExactSilentPairGatewayCofaceIds strict_replacement_coface_point_ids{};
  exact::ExactLevel strict_replacement_coface_squared_level{};
  exact::ExactLevel strict_path_maximum_squared_level{};
  ExactSilentPairGatewayComponentHandle target_component_handle{};
  contract::CanonicalId pre_batch_snapshot_digest{};
  contract::CanonicalId source_decision_id{};
  bool exact_strict_replacement_coface_certified{false};
  bool exact_path_to_pre_batch_target_certified{false};

  friend bool operator==(
      const ExactSilentPairGatewayStrictLegDecision&,
      const ExactSilentPairGatewayStrictLegDecision&) = default;
};

struct ExactSilentPairGatewayPreBatchTargetDecision {
  std::size_t order{silent_pair_gateway_order};
  exact::ExactLevel batch_squared_level{};
  ExactSilentPairGatewayComponentHandle component_handle{};
  std::size_t pre_batch_component_count{};
  std::size_t pre_batch_prefix_count{};
  contract::CanonicalId pre_batch_snapshot_digest{};
  contract::CanonicalId source_decision_id{};
  bool target_frozen_before_equal_level_batch{false};
  bool target_is_nontrivial_reduced_root{false};

  friend bool operator==(
      const ExactSilentPairGatewayPreBatchTargetDecision&,
      const ExactSilentPairGatewayPreBatchTargetDecision&) = default;
};

// Pair, witnesses and legs may arrive in any permutation.  The builder
// canonicalizes them by PointId and rejects every inconsistency.  It consumes
// only the supplied decisions; coordinates, Gamma, Delaunay and any global
// coface catalogue are deliberately absent from this API.
struct ExactSilentPairGatewayInput {
  std::uint32_t schema_version{silent_pair_gateway_schema_version};
  std::size_t point_count{};
  ExactSilentPairGatewayPairIds pair_point_ids{};
  exact::ExactLevel squared_level{};
  ExactSilentPairGatewayClosedBallDecision closed_ball{};
  std::array<ExactSilentPairGatewayStrictInteriorDecision, 2U>
      strict_interior_witnesses{};
  std::array<ExactSilentPairGatewayStrictLegDecision, 2U> strict_legs{};
  ExactSilentPairGatewayPreBatchTargetDecision pre_batch_target{};

  friend bool operator==(
      const ExactSilentPairGatewayInput&,
      const ExactSilentPairGatewayInput&) = default;
};

struct ExactSilentPairGatewayCoverageDelta {
  std::array<ExactSilentPairGatewayPairIds, 1U> added_facet_point_ids{};
  std::array<spatial::PointId, 0U> added_point_ids{};
  std::size_t added_facet_count{};
  std::size_t added_point_count{};
  bool exact_pair_facet_only{false};

  friend bool operator==(
      const ExactSilentPairGatewayCoverageDelta&,
      const ExactSilentPairGatewayCoverageDelta&) = default;
};

struct ExactSilentPairGatewayStrictLegWitness {
  spatial::PointId removed_support_point_id{};
  ExactSilentPairGatewayPairIds strict_facet_point_ids{};
  ExactSilentPairGatewayPairIds bridge_facet_point_ids{};
  ExactSilentPairGatewayCofaceIds strict_replacement_coface_point_ids{};
  exact::ExactLevel strict_replacement_coface_squared_level{};
  exact::ExactLevel strict_path_maximum_squared_level{};
  contract::CanonicalId source_decision_id{};

  friend bool operator==(
      const ExactSilentPairGatewayStrictLegWitness&,
      const ExactSilentPairGatewayStrictLegWitness&) = default;
};

// One compact q=3 silent incidence.  It adds the pair facet to one already
// existing pre-batch reduced root, adds no PointId and creates no node.  The
// record is not a Gamma_2 prune receipt, a completeness certificate, a public
// Attachment, a forest mutation or an exact public hierarchy claim.
struct ExactSilentPairGatewayRecord {
  std::size_t order{silent_pair_gateway_order};
  ExactSilentPairGatewayPairIds pair_point_ids{};
  exact::ExactLevel squared_level{};
  std::array<spatial::PointId, 2U>
      strict_interior_witness_point_ids{};
  std::array<exact::ExactLevel, 2U>
      strict_interior_witness_squared_distances{};
  std::array<contract::CanonicalId, 2U>
      strict_interior_source_decision_ids{};
  std::size_t certified_strict_interior_count{};
  std::size_t certified_extra_shell_count{};
  std::size_t certified_exterior_count{};
  contract::CanonicalId closed_ball_source_decision_id{};
  std::array<ExactSilentPairGatewayStrictLegWitness, 2U> strict_legs{};
  ExactSilentPairGatewayComponentHandle target_component_handle{};
  std::size_t pre_batch_component_count{};
  std::size_t pre_batch_prefix_count{};
  contract::CanonicalId pre_batch_snapshot_digest{};
  contract::CanonicalId pre_batch_target_source_decision_id{};
  ExactSilentPairGatewayCoverageDelta coverage_delta{};
  bool autonomous_node_created{false};
  bool root_birth_published{false};
  bool merge_node_published{false};
  bool gateway_attach_published{false};
  bool gamma2_prune_authority_claimed{false};
  bool gamma2_completeness_claimed{false};
  bool public_hierarchy_exactness_claimed{false};
  bool public_status_claimed{false};
  contract::CanonicalId canonical_digest{};

  friend bool operator==(
      const ExactSilentPairGatewayRecord&,
      const ExactSilentPairGatewayRecord&) = default;
};

enum class ExactSilentPairGatewayDecision : std::uint8_t {
  not_certified,
  rejected_schema_or_point_count,
  rejected_pair_or_level,
  rejected_closed_ball_certificate,
  rejected_closed_ball_count_accounting,
  rejected_extra_shell_not_excluded,
  rejected_strict_interior_witnesses,
  rejected_pre_batch_target,
  rejected_strict_leg_shape,
  rejected_strict_leg_level,
  rejected_strict_leg_target,
  complete_certified_regular_q3_silent_pair_gateway,
};

enum class ExactSilentPairGatewayScope : std::uint8_t {
  unspecified,
  regular_q3_pair_silent_incidence_relative_to_supplied_certified_decisions_only,
};

struct ExactSilentPairGatewayAudit {
  bool supplied_decisions_only{false};
  bool pair_and_witness_permutations_canonicalized{false};
  bool pair_level_and_unique_essential_support_certified{false};
  bool complete_closed_ball_count_accounting_certified{false};
  bool no_extra_shell_certified{false};
  bool two_canonical_strict_interior_witnesses_certified{false};
  bool two_strict_bridge_cofaces_certified{false};
  bool both_strict_paths_target_same_frozen_pre_batch_root{false};
  bool coverage_delta_is_pair_facet_and_zero_points{false};
  bool no_autonomous_node_or_forest_action{false};
  bool no_cloud_or_gamma_rescan_performed{true};
  bool no_global_facet_coface_or_cell_catalog_materialized{true};
  bool no_delaunay_or_higher_order_mosaic_materialized{true};
  bool no_partial_gateway_published{true};
  bool gamma2_prune_or_completeness_claimed{false};
  bool public_hierarchy_or_status_claimed{false};
  bool partial_refinement_only{true};

  friend bool operator==(
      const ExactSilentPairGatewayAudit&,
      const ExactSilentPairGatewayAudit&) = default;
};

struct ExactSilentPairGatewayResult {
  static constexpr std::string_view backend = silent_pair_gateway_backend;
  static constexpr std::string_view profile = silent_pair_gateway_profile;
  static constexpr std::string_view mode = silent_pair_gateway_mode;
  static constexpr std::string_view deployment_status =
      silent_pair_gateway_deployment_status;
  static constexpr std::string_view refinement_status =
      silent_pair_gateway_refinement_status;
  static constexpr std::string_view public_status =
      silent_pair_gateway_public_status;
  static constexpr std::string_view proof_basis =
      silent_pair_gateway_proof_basis;

  std::uint32_t schema_version{silent_pair_gateway_schema_version};
  std::optional<ExactSilentPairGatewayRecord> gateway;
  ExactSilentPairGatewayAudit audit{};
  ExactSilentPairGatewayDecision decision{
      ExactSilentPairGatewayDecision::not_certified};
  ExactSilentPairGatewayScope scope{
      ExactSilentPairGatewayScope::unspecified};

  [[nodiscard]] bool certified_gateway() const noexcept;
  [[nodiscard]] bool certified_fail_closed_rejection() const noexcept;

  friend bool operator==(
      const ExactSilentPairGatewayResult&,
      const ExactSilentPairGatewayResult&) = default;
};

struct ExactSilentPairGatewayVerification {
  bool schema_and_scope_certified{false};
  bool supplied_decision_validation_replayed{false};
  bool canonical_permutation_normalization_replayed{false};
  bool expected_outcome_freshly_rebuilt{false};
  bool canonical_digest_freshly_replayed{false};
  bool coverage_and_target_payload_certified{false};
  bool no_node_prune_completeness_or_public_claim_certified{false};
  bool no_forbidden_global_structure_certified{false};
  bool fresh_replay_certified{false};

  [[nodiscard]] bool certified_gateway() const noexcept;
  [[nodiscard]] bool certified_fail_closed_rejection() const noexcept;
};

[[nodiscard]] ExactSilentPairGatewayResult
build_exact_silent_pair_gateway(
    const ExactSilentPairGatewayInput& input);

// Rebuilds the complete relative result from the same caller-owned certified
// decisions.  It trusts no field of the observed gateway.  This remains a
// structural/scientific replay relative to those decisions; it does not turn
// their opaque source ids into a geometric authority.
[[nodiscard]] ExactSilentPairGatewayVerification
verify_exact_silent_pair_gateway(
    const ExactSilentPairGatewayInput& input,
    const ExactSilentPairGatewayResult& observed);

}  // namespace morsehgp3d::hierarchy
