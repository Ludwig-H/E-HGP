#pragma once

#include "morsehgp3d/hierarchy/critical_catalog_reduced_gamma_overlay.hpp"
#include "morsehgp3d/hierarchy/direct_morse_forest_carrier_cut_index.hpp"
#include "morsehgp3d/hierarchy/direct_morse_forest_journal.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_gamma_carrier_conformance_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_gamma_carrier_conformance_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_gamma_carrier_conformance_profile = "hgp_reduced";
inline constexpr std::string_view direct_morse_gamma_carrier_conformance_mode =
    "bounded_n14_single_order_direct_gamma_carrier_group_conformance";
inline constexpr std::string_view
    direct_morse_gamma_carrier_conformance_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    direct_morse_gamma_carrier_conformance_public_status = "not_claimed";
inline constexpr std::string_view
    direct_morse_gamma_carrier_conformance_proof_basis =
        "fresh_direct_forest_replay_and_fresh_bounded_critical_catalog_"
        "reduced_gamma_overlay_then_bidirectional_h0_role_and_atomic_group_"
        "reconciliation_plus_strict_closed_carrier_partition_bijections_at_"
        "every_exhaustive_gamma_activation_level_plus_each_direct_saddle_"
        "arm_exact_coface_incidence_strict_component_and_frozen_carrier_"
        "join_with_latent_iff_isolated_and_resolved_iff_nontrivial_"
        "distinction_with_residual_groups_accepted_only_as_one_root_zero_"
        "point_continuations_v1";

// The nested oracle and carrier-cut budgets retain their own authority.  The
// scalar caps below are preflighted from (n, k) and the trusted direct source
// sizes before the exhaustive overlay starts.  This remains a bounded
// conformance oracle and is never a product source for Gamma.
struct ExactDirectMorseGammaCarrierConformanceBudget {
  ExactCriticalCatalogReducedGammaOverlayBudget overlay_budget{};
  ExactDirectMorseForestCarrierCutIndexBudget carrier_cut_budget{};
  std::size_t maximum_direct_role_scan_count{};
  std::size_t maximum_forest_birth_scan_count{};
  std::size_t maximum_forest_saddle_scan_count{};
  std::size_t maximum_forest_atomic_group_scan_count{};
  std::size_t maximum_activation_level_count{};
  std::size_t maximum_gamma_transition_build_count{};
  std::size_t maximum_checkpoint_count{};
  std::size_t maximum_group_audit_count{};
  std::size_t maximum_carrier_anchor_scan_count{};
  std::size_t maximum_gamma_component_scan_count{};
  std::size_t maximum_gamma_facet_scan_count{};
  std::size_t maximum_group_event_reference_scan_count{};
  std::size_t maximum_arm_binding_scan_count{};
  std::size_t maximum_gamma_incidence_join_count{};
  std::size_t maximum_carrier_cut_entry_lookup_count{};
  std::size_t maximum_group_component_membership_count{};
  std::size_t maximum_gamma_label_comparison_count{};
  std::size_t maximum_logical_scratch_entry_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectMorseGammaCarrierConformanceBudget&,
      const ExactDirectMorseGammaCarrierConformanceBudget&) = default;
};

enum class ExactDirectMorseGammaCarrierConformanceDecision : std::uint8_t {
  not_certified,
  no_conformance_capacity_overflow,
  no_conformance_budget_exhausted,
  no_conformance_allocation_failed,
  no_conformance_point_count_or_order_rejected,
  no_conformance_direct_forest_replay_rejected,
  no_conformance_overlay_rejected,
  no_conformance_direct_role_overlay_mismatch,
  no_conformance_direct_group_overlay_mismatch,
  no_conformance_gamma_residual_group_rejected,
  no_conformance_gamma_transition_rejected,
  no_conformance_carrier_cut_replay_rejected,
  no_conformance_carrier_partition_mismatch,
  complete_bounded_single_order_direct_gamma_carrier_group_conformance,
};

enum class ExactDirectMorseGammaCarrierConformanceScope : std::uint8_t {
  unspecified,
  bounded_n14_single_order_direct_forest_carriers_and_h0_groups_against_fresh_exhaustive_gamma_only,
};

enum class ExactDirectMorseGammaGroupClassification : std::uint8_t {
  direct_birth_carrier,
  direct_saddle_group,
  mixed_direct_and_residual,
  silent_residual_continuation,
};

// This audit intentionally retains no facet, coface, component, carrier,
// root, or node identity.  Exact levels and aggregate counts are sufficient
// for a fresh verifier to compare the canonical replay recursively.
struct ExactDirectMorseGammaCarrierCheckpointAudit {
  std::size_t activation_level_index{};
  exact::ExactLevel squared_level{};
  bool direct_batch_present{false};
  std::size_t strict_direct_carrier_count{};
  std::size_t strict_gamma_component_count{};
  std::size_t strict_birth_anchor_count{};
  std::size_t closed_direct_carrier_count{};
  std::size_t closed_gamma_component_count{};
  std::size_t closed_birth_anchor_count{};
  std::size_t gamma_group_count{};
  std::size_t silent_gamma_group_count{};
  bool strict_partition_bijective{false};
  bool closed_partition_bijective{false};

  friend bool operator==(
      const ExactDirectMorseGammaCarrierCheckpointAudit&,
      const ExactDirectMorseGammaCarrierCheckpointAudit&) = default;
};

struct ExactDirectMorseGammaGroupAudit {
  exact::ExactLevel squared_level{};
  ExactReducedGammaBatchGroupKind gamma_kind{
      ExactReducedGammaBatchGroupKind::deferred_isolated_facet};
  ExactDirectMorseGammaGroupClassification classification{
      ExactDirectMorseGammaGroupClassification::direct_birth_carrier};
  std::size_t direct_birth_role_count{};
  std::size_t direct_saddle_role_count{};
  std::size_t residual_newly_active_facet_count{};
  std::size_t residual_equal_level_coface_count{};
  std::size_t gamma_prior_root_count{};
  std::optional<std::size_t> direct_prior_root_count;
  std::size_t delta_facet_count{};
  std::size_t delta_point_count{};
  std::size_t direct_arm_binding_count{};
  std::size_t strict_gamma_component_count{};
  std::size_t latent_strict_gamma_component_count{};
  std::size_t resolved_strict_gamma_component_count{};
  bool coverage_delta_present{false};
  bool fully_redundant_delta{false};
  bool direct_atomic_group_present{false};
  bool direct_arm_strict_component_bijection{false};
  bool silent_one_root_zero_point_continuation{false};

  friend bool operator==(
      const ExactDirectMorseGammaGroupAudit&,
      const ExactDirectMorseGammaGroupAudit&) = default;
};

struct ExactDirectMorseGammaCarrierConformanceCounters {
  std::size_t preflight_count{};
  std::size_t direct_forest_verification_count{};
  std::size_t overlay_build_count{};
  std::size_t direct_role_scan_count{};
  std::size_t direct_birth_role_count{};
  std::size_t direct_saddle_role_count{};
  std::size_t forest_birth_scan_count{};
  std::size_t forest_saddle_scan_count{};
  std::size_t forest_atomic_group_scan_count{};
  std::size_t group_event_reference_scan_count{};
  std::size_t activation_level_count{};
  std::size_t gamma_transition_build_count{};
  std::size_t carrier_cut_build_count{};
  std::size_t checkpoint_count{};
  std::size_t strict_partition_bijection_count{};
  std::size_t closed_partition_bijection_count{};
  std::size_t carrier_anchor_scan_count{};
  std::size_t gamma_component_scan_count{};
  std::size_t gamma_facet_scan_count{};
  std::size_t group_audit_count{};
  std::size_t arm_binding_scan_count{};
  std::size_t gamma_incidence_join_count{};
  std::size_t carrier_cut_entry_lookup_count{};
  std::size_t group_component_membership_count{};
  std::size_t gamma_label_comparison_count{};
  std::size_t maximum_logical_scratch_entry_count{};
  std::size_t direct_birth_group_count{};
  std::size_t direct_saddle_group_count{};
  std::size_t mixed_direct_residual_group_count{};
  std::size_t silent_residual_continuation_group_count{};
  std::size_t silent_checkpoint_count{};

  friend bool operator==(
      const ExactDirectMorseGammaCarrierConformanceCounters&,
      const ExactDirectMorseGammaCarrierConformanceCounters&) = default;
};

struct ExactDirectMorseGammaCarrierConformanceResult {
  static constexpr std::string_view backend =
      direct_morse_gamma_carrier_conformance_backend;
  static constexpr std::string_view profile =
      direct_morse_gamma_carrier_conformance_profile;
  static constexpr std::string_view mode =
      direct_morse_gamma_carrier_conformance_mode;
  static constexpr std::string_view deployment_status =
      direct_morse_gamma_carrier_conformance_deployment_status;
  static constexpr std::string_view public_status =
      direct_morse_gamma_carrier_conformance_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_gamma_carrier_conformance_proof_basis;

  std::uint32_t schema_version{
      direct_morse_gamma_carrier_conformance_schema_version};
  ExactDirectMorseGammaCarrierConformanceBudget requested_budget{};
  std::size_t point_count{};
  std::size_t order{};
  std::size_t exhaustive_facet_count{};
  std::size_t exhaustive_coface_count{};
  std::size_t required_direct_role_scan_capacity{};
  std::size_t required_forest_birth_scan_capacity{};
  std::size_t required_forest_saddle_scan_capacity{};
  std::size_t required_forest_atomic_group_scan_capacity{};
  std::size_t required_activation_level_capacity{};
  std::size_t required_carrier_anchor_scan_capacity{};
  std::size_t required_gamma_component_scan_capacity{};
  std::size_t required_gamma_facet_scan_capacity{};
  std::size_t required_group_event_reference_scan_capacity{};
  std::size_t required_arm_binding_scan_capacity{};
  std::size_t required_gamma_incidence_join_capacity{};
  std::size_t required_carrier_cut_entry_lookup_capacity{};
  std::size_t required_group_component_membership_capacity{};
  std::size_t required_gamma_label_comparison_capacity{};
  std::size_t required_logical_scratch_entry_count{};
  std::size_t required_checkpoint_capacity{};
  std::size_t required_group_audit_capacity{};
  std::size_t required_logical_output_entry_count{};
  std::vector<ExactDirectMorseGammaCarrierCheckpointAudit> checkpoints;
  std::vector<ExactDirectMorseGammaGroupAudit> group_audits;
  ExactDirectMorseGammaCarrierConformanceCounters counters{};

  bool scalar_preflight_certified{false};
  bool source_forest_freshly_replayed_relative_to_facade{false};
  bool critical_catalog_gamma_overlay_freshly_replayed{false};
  bool direct_h0_catalog_roles_bidirectionally_reconciled{false};
  bool direct_atomic_groups_bidirectionally_reconciled{false};
  bool direct_saddle_arms_bidirectionally_reconciled_with_strict_gamma_components{
      false};
  bool carrier_partition_bijective_at_every_strict_checkpoint{false};
  bool carrier_partition_bijective_at_every_closed_checkpoint{false};
  bool every_gamma_group_classified_once{false};
  bool every_provenance_free_gamma_group_is_silent_continuation{false};
  bool every_silent_continuation_preserves_one_root_and_adds_zero_points{
      false};
  bool bounded_carrier_faithfulness_replayed{false};
  bool bounded_silent_gamma_group_completeness_replayed{false};
  bool bounded_bidirectional_gamma_group_completeness_replayed{false};
  bool bounded_exhaustive_gamma_oracle_used{false};
  bool product_sparse_silent_source_complete{false};
  bool global_morse_obligation_replayed{false};
  bool global_m1_claimed{false};
  bool all_naturality_squares_replayed{false};
  bool vertical_maps_complete{false};
  bool forest_semantics_exact{false};
  bool scalable_50k_claimed{false};
  bool gamma_cells_or_global_cofaces_persisted{false};
  bool higher_order_delaunay_materialized{false};
  bool public_status_claimed{false};
  bool no_partial_scientific_payload_published_on_failure{false};
  ExactDirectMorseGammaCarrierConformanceDecision decision{
      ExactDirectMorseGammaCarrierConformanceDecision::not_certified};
  ExactDirectMorseGammaCarrierConformanceScope scope{
      ExactDirectMorseGammaCarrierConformanceScope::unspecified};

  [[nodiscard]] bool certified_bounded_conformance() const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectMorseGammaCarrierConformanceResult&,
      const ExactDirectMorseGammaCarrierConformanceResult&) = default;
};

struct ExactDirectMorseGammaCarrierConformanceVerification {
  bool observed_storage_within_budget{false};
  bool source_forest_freshly_replayed{false};
  bool expected_result_freshly_reconstructed{false};
  bool observed_recursively_equal{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectMorseGammaCarrierConformanceVerification&,
      const ExactDirectMorseGammaCarrierConformanceVerification&) = default;
};

[[nodiscard]] ExactDirectMorseGammaCarrierConformanceResult
build_exact_direct_morse_gamma_carrier_conformance(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& trusted_forest_budget,
    const ExactDirectMorseForestConfig& trusted_forest_config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseGammaCarrierConformanceBudget& budget);

[[nodiscard]] ExactDirectMorseGammaCarrierConformanceVerification
verify_exact_direct_morse_gamma_carrier_conformance(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& trusted_forest_budget,
    const ExactDirectMorseForestConfig& trusted_forest_config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseGammaCarrierConformanceBudget& trusted_budget,
    const ExactDirectMorseGammaCarrierConformanceResult& observed);

}  // namespace morsehgp3d::hierarchy
