#pragma once

#include "morsehgp3d/hierarchy/critical_catalog_typed_gamma_arm_root_composition.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace morsehgp3d::hierarchy {

// The nested 6.20 budget remains authoritative.  The five scalar capacities
// bound only the compact replayable paths retained by 6.21.  In particular,
// the transient catalog, arm families, closed-ball partitions and exhaustive
// miniball payloads are never copied into this overlay.
struct ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget {
  static constexpr std::size_t maximum_supported_path_record_count = 5824U;
  static constexpr std::size_t
      maximum_supported_committed_chain_segment_count = 23855104U;
  static constexpr std::size_t
      maximum_supported_composite_path_segment_count = 23860928U;
  static constexpr std::size_t
      maximum_supported_chain_node_point_id_reference_count = 238609280U;
  static constexpr std::size_t
      maximum_supported_exterior_constraint_count = 64064U;

  ExactCriticalCatalogTypedGammaArmRootCompositionBudget
      arm_root_composition_budget{};
  std::size_t maximum_path_record_count{};
  std::size_t maximum_committed_chain_segment_count{};
  std::size_t maximum_composite_path_segment_count{};
  std::size_t maximum_chain_node_point_id_reference_count{};
  std::size_t maximum_exterior_constraint_count{};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget&,
      const ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget&) =
      default;
};

enum class ExactCriticalCatalogTypedGammaArmRootPathOverlayDecision :
    std::uint8_t {
  not_certified,
  no_arm_root_path_overlay_preflight_budget_insufficient,
  no_arm_root_path_overlay_source_composition_rejected,
  no_arm_root_path_overlay_source_composition_incomplete,
  complete_exhaustive_event_local_replayable_arm_root_path_overlay,
};

enum class ExactCriticalCatalogTypedGammaArmRootPathOverlayScope :
    std::uint8_t {
  unspecified,
  bounded_n14_k10_single_order_event_local_typed_critical_arms_with_replayable_strict_descent_paths_linked_to_external_full_pi0_targets_and_separate_frozen_pre_batch_local_hgp_reduced_root_or_omitted_singleton_dispositions_only,
};

// This compact record contains all analytic data needed to replay the
// canonical strict arm path, but it remains an internal certificate.  It is
// not a public Attachment and its local reduced root is not a geometric path
// endpoint.  The external 6.18 strict target retains full-pi0 authority.
struct ExactCriticalCatalogTypedGammaEventLocalArmRootPathRecord {
  std::size_t path_record_index{};
  std::size_t arm_candidate_index{};
  std::size_t arm_record_index{};
  std::size_t source_arm_target_index{};
  std::size_t catalog_event_index{};
  std::size_t saddle_record_index{};
  std::size_t terminal_class_record_index{};
  std::size_t order{};
  spatial::PointId removed_shell_point_id{};
  exact::ExactCenter3 critical_center;
  exact::ExactLevel critical_squared_level;
  std::size_t history_batch_index{};
  std::size_t history_group_record_index{};
  std::size_t strict_target_record_index{};
  std::size_t target_root_binding_index{};
  ExactCriticalCatalogTypedGammaTargetDisposition disposition{
      ExactCriticalCatalogTypedGammaTargetDisposition::
          omitted_isolated_singleton};
  std::optional<std::size_t> local_reduced_root_node_id;
  ExactFacetDescentSegmentWitness initial_segment_witness;
  exact::ExactLevel removed_point_target_squared_distance;
  exact::ExactRational removed_point_outgoing_linear_coefficient;
  exact::ExactRational strict_local_parameter_upper_bound;
  std::vector<ExactCriticalArmExteriorConstraintWitness>
      negative_exterior_direction_constraints;
  std::vector<ExactFacetDescentChainNodeWitness> chain_nodes;
  std::vector<ExactFacetDescentSegmentWitness>
      committed_chain_segment_witnesses;
  bool exact_initial_to_chain_seam_certified{false};
  bool source_open_composite_path_strict_critical_sublevel{false};
  bool regular_active_terminal_certified{false};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaEventLocalArmRootPathRecord&,
      const ExactCriticalCatalogTypedGammaEventLocalArmRootPathRecord&) =
      default;
};

struct ExactCriticalCatalogTypedGammaArmRootPathOverlayCounters {
  std::size_t preflight_count{};
  std::size_t source_composition_verification_count{};
  std::size_t critical_catalog_build_count{};
  std::size_t critical_arm_family_build_count{};
  std::size_t saddle_event_reconciliation_count{};
  std::size_t arm_candidate_path_reconciliation_count{};
  std::size_t initial_facet_target_membership_check_count{};
  std::size_t terminal_facet_target_membership_check_count{};
  std::size_t path_record_count{};
  std::size_t committed_chain_segment_count{};
  std::size_t composite_path_segment_count{};
  std::size_t chain_node_point_id_reference_count{};
  std::size_t exterior_constraint_count{};
  std::size_t shared_target_path_count{};
  std::size_t matched_local_reduced_root_path_count{};
  std::size_t omitted_isolated_singleton_path_count{};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaArmRootPathOverlayCounters&,
      const ExactCriticalCatalogTypedGammaArmRootPathOverlayCounters&) =
      default;
};

struct ExactCriticalCatalogTypedGammaArmRootPathOverlayResult {
  static constexpr std::size_t minimum_supported_point_count = 3U;
  static constexpr std::size_t maximum_supported_point_count = 14U;
  static constexpr std::size_t minimum_supported_order = 2U;
  static constexpr std::size_t maximum_supported_order = 10U;
  static constexpr const char* proof_basis =
      "exact_fresh_event_local_typed_critical_arm_strict_descent_paths_"
      "replayed_and_linked_to_full_pi0_targets_with_separate_local_"
      "reduced_dispositions_v1";

  ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget requested_budget{};
  std::size_t point_count{};
  std::size_t order{};
  std::size_t critical_event_support_bound{};
  std::size_t critical_arm_bound{};
  std::size_t required_path_record_capacity{};
  std::size_t required_committed_chain_segment_capacity{};
  std::size_t required_composite_path_segment_capacity{};
  std::size_t required_chain_node_point_id_reference_capacity{};
  std::size_t required_exterior_constraint_capacity{};
  ExactCriticalCatalogTypedGammaArmRootCompositionDecision
      source_composition_decision{
          ExactCriticalCatalogTypedGammaArmRootCompositionDecision::
              not_certified};
  std::vector<ExactCriticalCatalogTypedGammaEventLocalArmRootPathRecord>
      path_records;
  bool arm_root_path_candidate_space_size_certified{false};
  bool arm_root_path_preflight_budget_sufficient{false};
  bool source_journal_is_external_and_not_retained{false};
  bool source_root_overlay_is_external_and_not_retained{false};
  bool source_composition_is_external_and_not_retained{false};
  bool source_journal_budget_seam_certified{false};
  bool source_root_overlay_budget_seam_certified{false};
  bool source_composition_budget_seam_certified{false};
  bool source_composition_fresh_replay_certified{false};
  bool reconstruction_started_only_after_complete_source_composition{false};
  bool transient_critical_catalog_fresh_replay_certified{false};
  bool transient_critical_arm_families_fresh_replay_certified{false};
  bool every_arm_candidate_has_one_dense_replayable_path{false};
  bool event_saddle_arm_and_terminal_class_keys_reconciled{false};
  bool exact_initial_germs_and_chain_shapes_replayable{false};
  bool exact_seams_strict_paths_and_regular_terminals_certified{false};
  bool initial_and_terminal_facets_belong_to_external_full_pi0_targets{false};
  bool target_bindings_and_reduced_dispositions_copied_without_reclassification{
      false};
  bool shared_targets_and_roots_preserve_distinct_paths{false};
  bool records_are_event_local_internal_paths_and_not_public_attachments{
      false};
  bool diagnostic_outcomes_have_no_paths{false};
  bool critical_catalog_typed_gamma_arm_root_path_overlay_certified{false};
  ExactCriticalCatalogTypedGammaArmRootPathOverlayCounters counters{};
  ExactCriticalCatalogTypedGammaArmRootPathOverlayDecision decision{
      ExactCriticalCatalogTypedGammaArmRootPathOverlayDecision::
          not_certified};
  ExactCriticalCatalogTypedGammaArmRootPathOverlayScope scope{
      ExactCriticalCatalogTypedGammaArmRootPathOverlayScope::unspecified};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaArmRootPathOverlayResult&,
      const ExactCriticalCatalogTypedGammaArmRootPathOverlayResult&) =
      default;
};

struct ExactCriticalCatalogTypedGammaArmRootPathOverlayVerification {
  bool requested_budget_certified{false};
  bool external_inputs_certified{false};
  bool derived_preflight_sizes_certified{false};
  bool source_composition_decision_certified{false};
  bool source_composition_fresh_replay_certified{false};
  bool path_records_certified{false};
  bool result_facts_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_critical_catalog_typed_gamma_arm_root_path_overlay_decision_certified{
      false};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaArmRootPathOverlayVerification&,
      const ExactCriticalCatalogTypedGammaArmRootPathOverlayVerification&) =
      default;
};

// Checks the complete nested 6.20 budget and all five path-arena caps without
// starting source verification or geometry.  Later overlays reuse this seam.
void validate_exact_critical_catalog_typed_gamma_arm_root_path_overlay_budget_caps(
    const ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget& budget);

[[nodiscard]] ExactCriticalCatalogTypedGammaArmRootPathOverlayResult
build_exact_critical_catalog_typed_gamma_arm_root_path_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget budget,
    const ExactCriticalCatalogTypedGammaJournalResult& source_journal,
    const ExactCriticalCatalogTypedGammaRootOverlayResult&
        source_root_overlay,
    const ExactCriticalCatalogTypedGammaArmRootCompositionResult&
        source_composition);

// All three external sources and the observed compact paths are untrusted.
// Verification reconstructs 6.20, then a fresh transient catalog and one 6.7
// family per saddle.  No observed path field steers that replay.
[[nodiscard]] ExactCriticalCatalogTypedGammaArmRootPathOverlayVerification
verify_exact_critical_catalog_typed_gamma_arm_root_path_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget budget,
    const ExactCriticalCatalogTypedGammaJournalResult& source_journal,
    const ExactCriticalCatalogTypedGammaRootOverlayResult&
        source_root_overlay,
    const ExactCriticalCatalogTypedGammaArmRootCompositionResult&
        source_composition,
    const ExactCriticalCatalogTypedGammaArmRootPathOverlayResult& result);

}  // namespace morsehgp3d::hierarchy
