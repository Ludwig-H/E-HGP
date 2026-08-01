#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_unified_level_plan.hpp"
#include "morsehgp3d/hierarchy/gamma.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_star_h0_retraction_differential_schema_version = 2U;
inline constexpr std::string_view
    direct_star_h0_retraction_differential_backend = "reference_cpu";
inline constexpr std::string_view
    direct_star_h0_retraction_differential_profile = "hgp_reduced";
inline constexpr std::string_view direct_star_h0_retraction_differential_mode =
    "bounded_exact_direct_star_h0_retraction_differential";
inline constexpr std::string_view
    direct_star_h0_retraction_differential_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    direct_star_h0_retraction_differential_public_status = "not_claimed";
inline constexpr std::string_view
    direct_star_h0_retraction_differential_universality_status =
        "bounded_falsification_only_no_incidence_complete_reduction_proof";
inline constexpr std::string_view
    direct_star_h0_retraction_differential_proof_basis =
        "fresh_direct_facade_incidence_successive_star_and_unified_plan_"
        "replay_single_bounded_exhaustive_gamma_high_cut_regular_direct_core_"
        "h0_retraction_"
        "direct_plus_complete_core_first_incidence_cominimizer_subflow_"
        "strict_closed_exact_level_qr_action_and_point_delta_differential_"
        "without_public_v2_identity_equivalence_v2";

struct ExactDirectStarH0RetractionDifferentialBudget {
  ExactStrictGammaBudget gamma_budget{};
  std::size_t maximum_checkpoint_count{};
  std::size_t maximum_partition_component_observation_count{};
  std::size_t maximum_batch_group_audit_count{};
  std::size_t maximum_omitted_coface_audit_count{};
  std::size_t maximum_first_incidence_facet_audit_count{};
  std::size_t maximum_first_incidence_cominimizer_reference_count{};
  std::size_t maximum_late_star_coface_audit_count{};
  std::size_t maximum_regularity_label_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectStarH0RetractionDifferentialBudget&,
      const ExactDirectStarH0RetractionDifferentialBudget&) = default;
};

enum class ExactDirectStarH0RetractionCutSide : std::uint8_t {
  strict,
  closed,
};

enum class ExactDirectStarH0RetractionAction : std::uint8_t {
  reduced_birth,
  continuation,
  multifusion,
};

struct ExactDirectStarH0RetractionCheckpointAudit {
  std::size_t checkpoint_audit_index{};
  exact::ExactLevel squared_level{};
  ExactDirectStarH0RetractionCutSide cut_side{
      ExactDirectStarH0RetractionCutSide::strict};
  std::size_t exhaustive_gamma_component_count{};
  std::size_t retained_star_component_count{};
  std::size_t first_incidence_subflow_component_count{};
  std::size_t exhaustive_gamma_core_facet_count{};
  std::size_t retained_star_core_facet_count{};
  std::size_t first_incidence_subflow_core_facet_count{};
  std::size_t exhaustive_gamma_covered_point_reference_count{};
  std::size_t retained_star_covered_point_reference_count{};
  std::size_t first_incidence_subflow_covered_point_reference_count{};
  contract::CanonicalId exhaustive_gamma_projected_partition_digest{};
  contract::CanonicalId retained_star_projected_partition_digest{};
  contract::CanonicalId
      first_incidence_subflow_projected_partition_digest{};
  std::vector<std::vector<std::vector<spatial::PointId>>>
      divergent_exhaustive_gamma_component_core_facets;
  std::vector<std::vector<spatial::PointId>>
      divergent_exhaustive_gamma_component_covered_point_ids;
  std::vector<std::vector<std::vector<spatial::PointId>>>
      divergent_first_incidence_subflow_component_core_facets;
  std::vector<std::vector<spatial::PointId>>
      divergent_first_incidence_subflow_component_covered_point_ids;
  bool every_component_meets_direct_core{false};
  bool canonical_core_partition_equal{false};
  bool covered_point_unions_equal{false};
  bool first_incidence_subflow_canonical_core_partition_equal{false};
  bool first_incidence_subflow_covered_point_unions_equal{false};

  friend bool operator==(
      const ExactDirectStarH0RetractionCheckpointAudit&,
      const ExactDirectStarH0RetractionCheckpointAudit&) = default;
};

struct ExactDirectStarH0RetractionBatchGroupAudit {
  std::size_t batch_group_audit_index{};
  exact::ExactLevel squared_level{};
  std::size_t exhaustive_gamma_q_r{};
  std::size_t retained_star_q_r{};
  std::size_t first_incidence_subflow_q_r{};
  ExactDirectStarH0RetractionAction exhaustive_gamma_action{
      ExactDirectStarH0RetractionAction::reduced_birth};
  ExactDirectStarH0RetractionAction retained_star_action{
      ExactDirectStarH0RetractionAction::reduced_birth};
  ExactDirectStarH0RetractionAction first_incidence_subflow_action{
      ExactDirectStarH0RetractionAction::reduced_birth};
  std::size_t direct_core_facet_count{};
  std::size_t added_direct_core_facet_count{};
  std::size_t retained_new_coface_count{};
  std::size_t omitted_new_coface_count{};
  std::size_t first_incidence_subflow_new_coface_count{};
  std::size_t late_star_omitted_new_coface_count{};
  bool retained_star_group_present{false};
  bool retained_star_group_omitted_as_certified_noop{false};
  bool first_incidence_subflow_group_present{false};
  bool first_incidence_subflow_group_omitted_as_certified_noop{false};
  std::vector<spatial::PointId> exhaustive_gamma_added_point_ids;
  std::vector<spatial::PointId> retained_star_added_point_ids;
  std::vector<spatial::PointId>
      first_incidence_subflow_added_point_ids;
  contract::CanonicalId exhaustive_gamma_parent_partition_digest{};
  contract::CanonicalId retained_star_parent_partition_digest{};
  contract::CanonicalId
      first_incidence_subflow_parent_partition_digest{};
  bool q_r_equal{false};
  bool action_equal{false};
  bool parent_partition_equal{false};
  bool direct_core_delta_equal{false};
  bool added_point_delta_equal{false};
  bool first_incidence_subflow_q_r_equal{false};
  bool first_incidence_subflow_action_equal{false};
  bool first_incidence_subflow_parent_partition_equal{false};
  bool first_incidence_subflow_direct_core_delta_equal{false};
  bool first_incidence_subflow_added_point_delta_equal{false};

  friend bool operator==(
      const ExactDirectStarH0RetractionBatchGroupAudit&,
      const ExactDirectStarH0RetractionBatchGroupAudit&) = default;
};

struct ExactDirectStarH0RetractionOmittedCofaceAudit {
  std::size_t omitted_coface_audit_index{};
  std::vector<spatial::PointId> coface_point_ids;
  exact::ExactLevel squared_level{};
  std::size_t strict_prior_root_count{};
  std::size_t newly_attached_facet_count{};
  std::vector<spatial::PointId> added_point_ids;
  bool absent_from_direct_star{false};
  bool non_gabriel_certified{false};
  bool continuation_q_r_one{false};
  bool point_delta_empty{false};
  bool no_present_or_future_h0_effect{false};

  friend bool operator==(
      const ExactDirectStarH0RetractionOmittedCofaceAudit&,
      const ExactDirectStarH0RetractionOmittedCofaceAudit&) = default;
};

struct ExactDirectCoreFirstIncidenceAudit {
  std::size_t first_incidence_audit_index{};
  std::vector<spatial::PointId> direct_core_facet_point_ids;
  exact::ExactLevel first_incidence_squared_level{};
  std::vector<std::vector<spatial::PointId>>
      complete_cominimizer_coface_point_ids;
  bool exact_star_incidence_nonempty{false};
  bool complete_cominimizers_canonical{false};
  bool every_cominimizer_retained_in_subflow{false};

  friend bool operator==(
      const ExactDirectCoreFirstIncidenceAudit&,
      const ExactDirectCoreFirstIncidenceAudit&) = default;
};

struct ExactDirectLateStarCofaceAudit {
  std::size_t late_star_coface_audit_index{};
  std::vector<spatial::PointId> coface_point_ids;
  exact::ExactLevel squared_level{};
  std::vector<std::vector<spatial::PointId>>
      touched_direct_core_facet_point_ids;
  std::vector<exact::ExactLevel>
      touched_direct_core_first_incidence_squared_levels;
  std::size_t strict_prior_root_count{};
  std::size_t newly_attached_facet_count{};
  std::vector<spatial::PointId> added_point_ids;
  bool retained_in_exact_direct_star{false};
  bool non_direct_non_gabriel_certified{false};
  bool omitted_from_first_incidence_subflow{false};
  bool every_touched_core_first_incidence_strictly_lower{false};
  bool continuation_q_r_one{false};
  bool point_delta_empty{false};
  bool no_present_or_future_h0_effect{false};

  friend bool operator==(
      const ExactDirectLateStarCofaceAudit&,
      const ExactDirectLateStarCofaceAudit&) = default;
};

enum class ExactDirectStarH0RetractionDifferentialDecision : std::uint8_t {
  not_certified,
  no_preflight_budget_insufficient,
  no_source_pipeline_not_certified,
  no_source_plan_order_inconsistent,
  no_gamma_high_cut_not_certified,
  unsupported_regularity_gate,
  falsified_direct_star_of_core,
  falsified_core_first_incidence_completion,
  falsified_late_star_coface_silence,
  falsified_omitted_coface_silence,
  falsified_strict_or_closed_h0_partition,
  falsified_batch_qr_action_or_delta,
  complete_bounded_conditional_direct_star_h0_retraction_differential,
};

enum class ExactDirectStarH0RetractionDifferentialScope : std::uint8_t {
  unspecified,
  bounded_n9_orders2_and3_regular_direct_star_to_exhaustive_gamma_h0_only,
};

struct ExactDirectStarH0RetractionDifferentialCounters {
  std::size_t preflight_count{};
  std::size_t source_plan_verification_count{};
  std::size_t diameter_pair_distance_evaluation_count{};
  std::size_t high_cut_gamma_build_count{};
  std::size_t high_cut_gamma_verification_count{};
  std::size_t regularity_miniball_build_count{};
  std::size_t regularity_miniball_verification_count{};
  std::size_t regularity_closed_ball_query_count{};
  std::size_t exhaustive_gamma_coface_count{};
  std::size_t retained_star_coface_count{};
  std::size_t first_incidence_subflow_coface_count{};
  std::size_t direct_core_coface_count{};
  std::size_t direct_core_facet_count{};
  std::size_t omitted_gamma_coface_count{};
  std::size_t first_incidence_facet_count{};
  std::size_t first_incidence_cominimizer_reference_count{};
  std::size_t late_star_omitted_coface_count{};
  std::size_t late_star_silent_continuation_count{};
  std::size_t first_incidence_omitted_noop_group_count{};
  std::size_t retained_star_omitted_noop_group_count{};
  std::size_t checkpoint_count{};
  std::size_t partition_component_observation_count{};
  std::size_t batch_group_count{};
  std::size_t omitted_silent_continuation_count{};
  std::size_t logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectStarH0RetractionDifferentialCounters&,
      const ExactDirectStarH0RetractionDifferentialCounters&) = default;
};

// This object is a bounded falsifier for two conditional horizontal H0
// retractions: Gamma to the exact direct star, then the direct star to direct
// cofaces plus every exact first-incidence co-minimizer of every direct-core
// facet. Gamma is rebuilt only as one transient small-n high-cut catalogue;
// local snapshots sweep the unique coface levels. Equality of the public v2
// hierarchy identity is deliberately neither computed nor claimed: v2
// commits the exhaustive coface and facet journals.
struct ExactDirectStarH0RetractionDifferentialResult {
  static constexpr std::size_t minimum_supported_point_count = 4U;
  static constexpr std::size_t maximum_supported_point_count = 9U;
  static constexpr std::size_t minimum_supported_order = 2U;
  static constexpr std::size_t maximum_supported_order = 3U;
  static constexpr std::string_view backend =
      direct_star_h0_retraction_differential_backend;
  static constexpr std::string_view profile =
      direct_star_h0_retraction_differential_profile;
  static constexpr std::string_view mode =
      direct_star_h0_retraction_differential_mode;
  static constexpr std::string_view deployment_status =
      direct_star_h0_retraction_differential_deployment_status;
  static constexpr std::string_view public_status =
      direct_star_h0_retraction_differential_public_status;
  static constexpr std::string_view universality_status =
      direct_star_h0_retraction_differential_universality_status;
  static constexpr std::string_view proof_basis =
      direct_star_h0_retraction_differential_proof_basis;

  std::uint32_t schema_version{
      direct_star_h0_retraction_differential_schema_version};
  ExactDirectStarH0RetractionDifferentialBudget requested_budget{};
  std::size_t point_count{};
  std::size_t order{};
  contract::CanonicalId canonical_cloud_digest{};
  exact::ExactLevel exact_diameter_squared{};
  exact::ExactLevel exhaustive_gamma_high_cut_squared_level{};
  std::vector<ExactDirectStarH0RetractionCheckpointAudit> checkpoint_audits;
  std::vector<ExactDirectStarH0RetractionBatchGroupAudit> batch_group_audits;
  std::vector<ExactDirectStarH0RetractionOmittedCofaceAudit>
      omitted_coface_audits;
  std::vector<ExactDirectCoreFirstIncidenceAudit>
      first_incidence_audits;
  std::vector<ExactDirectLateStarCofaceAudit>
      late_star_coface_audits;
  bool candidate_space_preflight_certified{false};
  bool source_plan_freshly_replayed{false};
  bool gamma_high_cut_freshly_built{false};
  bool gamma_high_cut_freshly_verified{false};
  bool gamma_high_cut_equals_twice_exact_squared_diameter{false};
  bool regularity_gate_certified{false};
  bool direct_catalog_is_global_core_for_order{false};
  bool retained_cofaces_equal_exact_star_of_direct_core{false};
  bool every_direct_core_facet_has_complete_first_incidence_cominimizers{
      false};
  bool first_incidence_subflow_equals_direct_union_complete_cominimizers{
      false};
  bool every_late_star_omission_has_strictly_earlier_core_first_incidence{
      false};
  bool every_late_star_omission_is_qr1_zero_point_continuation{false};
  bool every_omitted_coface_is_qr1_zero_point_continuation{false};
  bool strict_and_closed_core_partitions_equal_at_every_exact_level{false};
  bool covered_point_unions_equal_at_every_exact_level{false};
  bool qr_actions_parents_and_point_deltas_equal_for_core_groups{false};
  bool first_incidence_subflow_strict_and_closed_core_partitions_equal{
      false};
  bool first_incidence_subflow_covered_point_unions_equal{false};
  bool first_incidence_subflow_qr_actions_parents_and_point_deltas_equal{
      false};
  bool every_missing_first_incidence_group_is_certified_noop{false};
  bool every_missing_retained_star_group_is_certified_noop{false};
  bool conditional_first_incidence_subflow_h0_retraction_certified{false};
  bool conditional_reduced_h0_forest_semantics_equivalent{false};
  bool conditional_horizontal_h0_retraction_certified{false};
  bool bounded_gamma_oracle_materialized_transiently{false};
  bool bounded_gamma_oracle_persisted{false};
  bool exhaustive_gamma_faceted_payload_reproduced{false};
  bool contract_v2_identity_equivalence_claimed{false};
  bool contract_v2_batch_identity_equivalence_claimed{false};
  bool contract_v2_forest_identity_equivalence_claimed{false};
  bool sparse_publication_requires_new_contract_revision{false};
  bool forest_semantics_exact_claimed{false};
  bool incidence_complete_reduction_proved{false};
  bool universal_product_completeness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectStarH0RetractionDifferentialCounters counters{};
  ExactDirectStarH0RetractionDifferentialDecision decision{
      ExactDirectStarH0RetractionDifferentialDecision::not_certified};
  ExactDirectStarH0RetractionDifferentialScope scope{
      ExactDirectStarH0RetractionDifferentialScope::unspecified};

  [[nodiscard]] bool certified_differential() const noexcept;

  friend bool operator==(
      const ExactDirectStarH0RetractionDifferentialResult&,
      const ExactDirectStarH0RetractionDifferentialResult&) = default;
};

struct ExactDirectStarH0RetractionDifferentialVerification {
  bool source_pipeline_freshly_replayed{false};
  bool gamma_oracle_freshly_replayed{false};
  bool expected_result_freshly_rebuilt{false};
  bool observed_recursively_equal{false};
  bool h0_and_v2_identity_claims_separated{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};
};

[[nodiscard]] ExactDirectStarH0RetractionDifferentialResult
build_exact_direct_star_h0_retraction_differential(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget&
        source_star_budget,
    spatial::LbvhTraversalOrder source_star_traversal_order,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& source_star,
    const ExactDirectSparseUnifiedLevelPlanBudget& source_plan_budget,
    const ExactDirectSparseUnifiedLevelPlanResult& source_plan,
    std::size_t order,
    const ExactDirectStarH0RetractionDifferentialBudget& budget);

// Rebuilds the bounded differential from the trusted cloud, source pipeline
// authorities, order and budgets.  No observed digest, partition, qR, action,
// delta or omission selects a replay branch.
[[nodiscard]] ExactDirectStarH0RetractionDifferentialVerification
verify_exact_direct_star_h0_retraction_differential(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget&
        source_star_budget,
    spatial::LbvhTraversalOrder source_star_traversal_order,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& source_star,
    const ExactDirectSparseUnifiedLevelPlanBudget& source_plan_budget,
    const ExactDirectSparseUnifiedLevelPlanResult& source_plan,
    std::size_t order,
    const ExactDirectStarH0RetractionDifferentialBudget& budget,
    const ExactDirectStarH0RetractionDifferentialResult& observed);

}  // namespace morsehgp3d::hierarchy
