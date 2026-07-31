#pragma once

#include "morsehgp3d/hierarchy/facet_miniball.hpp"
#include "morsehgp3d/hierarchy/reduced_gamma_batch.hpp"
#include "morsehgp3d/hierarchy/reduced_gamma_history.hpp"
#include "morsehgp3d/hierarchy/silent_pair_gateway.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    silent_pair_gateway_gamma2_differential_schema_version = 1U;
inline constexpr std::string_view
    silent_pair_gateway_gamma2_differential_backend = "reference_cpu";
inline constexpr std::string_view
    silent_pair_gateway_gamma2_differential_profile = "hgp_reduced";
inline constexpr std::string_view
    silent_pair_gateway_gamma2_differential_mode =
        "bounded_exact_silent_pair_gateway_gamma2_differential";
inline constexpr std::string_view
    silent_pair_gateway_gamma2_differential_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    silent_pair_gateway_gamma2_differential_public_status =
        "not_claimed";
inline constexpr std::string_view
    silent_pair_gateway_gamma2_differential_proof_basis =
        "fresh_exact_diametral_closed_ball_two_canonical_interiors_strict_"
        "replacement_cofaces_exhaustive_gamma2_pre_batch_root_equal_level_"
        "delta_and_later_history_differential_v1";

// The exhaustive history budget owns every candidate-space and replay bound.
// The same nested Gamma budget is reused for the explicit source-facet cut and
// the equal-level reduced batch, so the three exact views cannot silently run
// under different work contracts.
struct ExactSilentPairGatewayGamma2DifferentialBudget {
  ExactPersistentReducedGammaOrderHistoryBudget history_budget{};

  friend bool operator==(
      const ExactSilentPairGatewayGamma2DifferentialBudget&,
      const ExactSilentPairGatewayGamma2DifferentialBudget&) = default;
};

enum class ExactSilentPairGatewayGamma2DifferentialDecision :
    std::uint8_t {
  not_certified,
  no_differential_preflight_budget_insufficient,
  rejected_point_count_or_pair,
  rejected_nonunique_essential_diametral_support,
  rejected_extra_shell_not_excluded,
  rejected_fewer_than_two_strict_interiors,
  rejected_zero_or_nonstrict_replacement_coface,
  rejected_incomplete_strict_gamma_sources,
  rejected_divergent_pre_batch_targets,
  rejected_trivial_pre_batch_target,
  rejected_equal_level_gamma2_effect,
  rejected_persistent_gamma2_history,
  complete_exact_regular_silent_pair_gateway_gamma2_differential,
};

enum class ExactSilentPairGatewayGamma2DifferentialScope :
    std::uint8_t {
  unspecified,
  bounded_n14_order2_regular_diametral_pair_gateway_to_exhaustive_gamma2_history_only,
};

struct ExactSilentPairGatewayGamma2LegReplay {
  spatial::PointId removed_support_point_id{};
  ExactSilentPairGatewayPairIds strict_facet_point_ids{};
  ExactSilentPairGatewayPairIds bridge_facet_point_ids{};
  ExactSilentPairGatewayCofaceIds
      strict_replacement_coface_point_ids{};
  ExactFacetMiniballResult strict_replacement_coface_miniball{};
  std::vector<std::vector<spatial::PointId>> strict_path_facet_point_ids;
  std::vector<std::vector<spatial::PointId>>
      strict_path_coface_point_ids;
  exact::ExactLevel strict_path_maximum_squared_level{};
  std::size_t strict_gamma_component_index{};

  friend bool operator==(
      const ExactSilentPairGatewayGamma2LegReplay&,
      const ExactSilentPairGatewayGamma2LegReplay&) = default;
};

// One post-batch observation of the unique active reduced root containing the
// silent pair.  The trace begins at the pair's own activation batch and runs
// through every later exhaustive Gamma_2 batch.
struct ExactSilentPairGatewayGamma2LaterRootWitness {
  std::size_t batch_index{};
  exact::ExactLevel squared_level{};
  std::size_t root_node_id{};
  std::size_t root_facet_count{};
  std::size_t root_covered_point_count{};

  friend bool operator==(
      const ExactSilentPairGatewayGamma2LaterRootWitness&,
      const ExactSilentPairGatewayGamma2LaterRootWitness&) = default;
};

struct ExactSilentPairGatewayGamma2DifferentialCounters {
  std::size_t preflight_count{};
  std::size_t pair_miniball_build_count{};
  std::size_t pair_miniball_verification_count{};
  std::size_t closed_ball_query_count{};
  std::size_t closed_ball_distance_evaluation_count{};
  std::size_t replacement_coface_miniball_build_count{};
  std::size_t replacement_coface_miniball_verification_count{};
  std::size_t strict_gamma_build_count{};
  std::size_t strict_gamma_verification_count{};
  std::size_t reduced_gamma_batch_build_count{};
  std::size_t reduced_gamma_batch_verification_count{};
  std::size_t persistent_gamma_history_build_count{};
  std::size_t persistent_gamma_history_verification_count{};
  std::size_t exhaustive_pair_equal_level_coface_count{};
  std::size_t strict_path_facet_visit_count{};
  std::size_t strict_path_coface_visit_count{};
  std::size_t pre_batch_root_count{};
  std::size_t later_history_batch_count{};
  std::size_t gateway_build_count{};
  std::size_t gateway_verification_count{};

  friend bool operator==(
      const ExactSilentPairGatewayGamma2DifferentialCounters&,
      const ExactSilentPairGatewayGamma2DifferentialCounters&) = default;
};

// This is a bounded scientific differential, not a production source.  It
// deliberately retains the exact subordinate results so a fresh verifier can
// rebuild and compare every geometric, Gamma_2 and history decision.  No
// Delaunay mosaic, global product catalogue, public hierarchy or prune
// authority is produced.
struct ExactSilentPairGatewayGamma2DifferentialResult {
  static constexpr std::size_t minimum_supported_point_count = 4U;
  static constexpr std::size_t maximum_supported_point_count = 14U;
  static constexpr std::size_t order = silent_pair_gateway_order;
  static constexpr std::string_view backend =
      silent_pair_gateway_gamma2_differential_backend;
  static constexpr std::string_view profile =
      silent_pair_gateway_gamma2_differential_profile;
  static constexpr std::string_view mode =
      silent_pair_gateway_gamma2_differential_mode;
  static constexpr std::string_view deployment_status =
      silent_pair_gateway_gamma2_differential_deployment_status;
  static constexpr std::string_view public_status =
      silent_pair_gateway_gamma2_differential_public_status;
  static constexpr std::string_view proof_basis =
      silent_pair_gateway_gamma2_differential_proof_basis;

  std::uint32_t schema_version{
      silent_pair_gateway_gamma2_differential_schema_version};
  ExactSilentPairGatewayGamma2DifferentialBudget requested_budget{};
  std::size_t point_count{};
  ExactSilentPairGatewayPairIds pair_point_ids{};
  contract::CanonicalId canonical_cloud_digest{};
  std::optional<ExactFacetMiniballResult> pair_miniball;
  std::vector<spatial::PointId> strict_interior_point_ids;
  std::vector<spatial::PointId> extra_shell_point_ids;
  std::vector<spatial::PointId> exterior_point_ids;
  std::array<spatial::PointId, 2U>
      canonical_strict_interior_witness_point_ids{};
  std::vector<ExactSilentPairGatewayCofaceIds>
      exhaustive_pair_equal_level_coface_point_ids;
  std::vector<ExactSilentPairGatewayGamma2LegReplay> strict_leg_replays;
  std::optional<ExactStrictGammaResult> strict_gamma;
  std::optional<ExactReducedGammaBatchResult> reduced_gamma_batch;
  std::optional<ExactPersistentReducedGammaOrderHistory>
      persistent_gamma_history;
  std::optional<std::size_t> target_strict_gamma_component_index;
  std::optional<std::size_t> target_pre_batch_component_handle;
  std::optional<std::size_t> target_prior_root_node_id;
  std::vector<std::vector<spatial::PointId>>
      target_pre_batch_root_facet_point_ids;
  contract::CanonicalId pre_batch_snapshot_digest{};
  std::size_t pre_batch_prefix_count{};
  std::optional<std::size_t> pair_history_batch_index;
  std::optional<std::size_t> pair_history_group_record_index;
  std::vector<ExactSilentPairGatewayGamma2LaterRootWitness>
      later_root_trace;
  std::optional<ExactSilentPairGatewayInput> gateway_input;
  std::optional<ExactSilentPairGatewayResult> gateway_result;

  bool candidate_space_preflight_certified{false};
  bool pair_miniball_freshly_certified{false};
  bool unique_essential_diametral_support_certified{false};
  bool complete_closed_ball_partition_certified{false};
  bool no_extra_shell_certified{false};
  bool canonical_two_strict_interiors_certified{false};
  bool all_pair_interior_cofaces_exhaustively_reconciled{false};
  bool two_strict_replacement_cofaces_freshly_certified{false};
  bool strict_gamma_sources_freshly_certified{false};
  bool both_strict_legs_reach_same_component{false};
  bool target_is_frozen_nontrivial_pre_batch_root{false};
  bool reduced_equal_level_batch_freshly_certified{false};
  bool equal_level_effect_is_pair_only_zero_points_continuation{false};
  bool persistent_gamma_history_freshly_certified{false};
  bool pair_history_creates_no_node{false};
  bool pair_persists_through_every_later_gamma2_batch{false};
  bool compact_gateway_freshly_certified{false};
  bool no_global_product_delaunay_or_public_claim{true};
  ExactSilentPairGatewayGamma2DifferentialCounters counters{};
  ExactSilentPairGatewayGamma2DifferentialDecision decision{
      ExactSilentPairGatewayGamma2DifferentialDecision::not_certified};
  ExactSilentPairGatewayGamma2DifferentialScope scope{
      ExactSilentPairGatewayGamma2DifferentialScope::unspecified};

  [[nodiscard]] bool certified_differential() const noexcept;
  [[nodiscard]] bool certified_fail_closed_rejection() const noexcept;

  friend bool operator==(
      const ExactSilentPairGatewayGamma2DifferentialResult&,
      const ExactSilentPairGatewayGamma2DifferentialResult&) = default;
};

struct ExactSilentPairGatewayGamma2DifferentialVerification {
  bool schema_scope_and_inputs_certified{false};
  bool requested_budget_certified{false};
  bool fresh_geometric_recertification_replayed{false};
  bool exhaustive_gamma2_batch_replayed{false};
  bool exhaustive_gamma2_history_replayed{false};
  bool compact_gateway_replayed{false};
  bool expected_outcome_freshly_rebuilt{false};
  bool expected_complete_differential{false};
  bool expected_fail_closed_rejection{false};
  bool no_forbidden_structure_or_public_claim_certified{false};
  bool fresh_replay_certified{false};

  [[nodiscard]] bool certified_differential() const noexcept;
  [[nodiscard]] bool certified_fail_closed_rejection() const noexcept;
};

[[nodiscard]] ExactSilentPairGatewayGamma2DifferentialResult
build_exact_silent_pair_gateway_gamma2_differential(
    const spatial::CanonicalPointCloud& cloud,
    ExactSilentPairGatewayPairIds pair_point_ids,
    ExactSilentPairGatewayGamma2DifferentialBudget budget);

// Rebuilds the complete bounded differential from the cloud, pair and trusted
// budget.  No observed geometry, source decision, Gamma component, history
// root, digest or gateway field selects a replay branch.
[[nodiscard]] ExactSilentPairGatewayGamma2DifferentialVerification
verify_exact_silent_pair_gateway_gamma2_differential(
    const spatial::CanonicalPointCloud& cloud,
    ExactSilentPairGatewayPairIds pair_point_ids,
    ExactSilentPairGatewayGamma2DifferentialBudget budget,
    const ExactSilentPairGatewayGamma2DifferentialResult& result);

}  // namespace morsehgp3d::hierarchy
