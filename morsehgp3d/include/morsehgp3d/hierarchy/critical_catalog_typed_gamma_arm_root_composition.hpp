#pragma once

#include "morsehgp3d/hierarchy/critical_catalog_typed_gamma_root_overlay.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace morsehgp3d::hierarchy {

// The 6.18 journal and 6.19 root overlay remain external and unowned.  This
// single scalar capacity covers one fixed-size event-local candidate per
// typed critical arm.  No component witness, facet family or descent path is
// copied into this layer.
struct ExactCriticalCatalogTypedGammaArmRootCompositionBudget {
  static constexpr std::size_t maximum_supported_arm_candidate_count =
      5824U;

  ExactCriticalCatalogTypedGammaRootOverlayBudget root_overlay_budget{};
  std::size_t maximum_arm_candidate_count{};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaArmRootCompositionBudget&,
      const ExactCriticalCatalogTypedGammaArmRootCompositionBudget&) =
      default;
};

enum class ExactCriticalCatalogTypedGammaArmRootCompositionDecision :
    std::uint8_t {
  not_certified,
  no_arm_root_composition_preflight_budget_insufficient,
  no_arm_root_composition_source_pair_rejected,
  no_arm_root_composition_source_pair_incomplete,
  complete_exhaustive_event_local_arm_root_composition,
};

enum class ExactCriticalCatalogTypedGammaArmRootCompositionScope :
    std::uint8_t {
  unspecified,
  bounded_n14_k10_single_order_event_local_typed_critical_arms_to_strict_full_pi0_targets_and_frozen_pre_batch_local_hgp_reduced_root_or_explicit_omitted_singleton_candidates_only,
};

// This is an internal relational candidate, not a public Attachment.  The
// semantic key is (catalog_event_index, result.order,
// removed_shell_point_id).  The strict target remains authoritative in the
// external 6.18 journal; the optional local reduced root is copied from the
// external 6.19 binding without reclassification.
struct ExactCriticalCatalogTypedGammaEventLocalArmTargetRootCandidate {
  std::size_t candidate_index{};
  std::size_t arm_record_index{};
  std::size_t catalog_event_index{};
  spatial::PointId removed_shell_point_id{};
  std::size_t strict_target_record_index{};
  std::size_t target_root_binding_index{};
  ExactCriticalCatalogTypedGammaTargetDisposition disposition{
      ExactCriticalCatalogTypedGammaTargetDisposition::
          omitted_isolated_singleton};
  std::optional<std::size_t> local_reduced_root_node_id;

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaEventLocalArmTargetRootCandidate&,
      const ExactCriticalCatalogTypedGammaEventLocalArmTargetRootCandidate&) =
      default;
};

struct ExactCriticalCatalogTypedGammaArmRootCompositionCounters {
  std::size_t preflight_count{};
  std::size_t source_root_overlay_verification_count{};
  std::size_t arm_candidate_count{};
  std::size_t saddle_arm_membership_check_count{};
  std::size_t terminal_class_target_chain_check_count{};
  std::size_t target_binding_check_count{};
  std::size_t matched_local_reduced_root_candidate_count{};
  std::size_t omitted_isolated_singleton_candidate_count{};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaArmRootCompositionCounters&,
      const ExactCriticalCatalogTypedGammaArmRootCompositionCounters&) =
      default;
};

struct ExactCriticalCatalogTypedGammaArmRootCompositionResult {
  static constexpr std::size_t minimum_supported_point_count = 3U;
  static constexpr std::size_t maximum_supported_point_count = 14U;
  static constexpr std::size_t minimum_supported_order = 2U;
  static constexpr std::size_t maximum_supported_order = 10U;
  static constexpr const char* proof_basis =
      "exact_fresh_typed_critical_arm_target_indices_composed_with_"
      "recertified_target_root_bindings_v1";

  ExactCriticalCatalogTypedGammaArmRootCompositionBudget requested_budget{};
  std::size_t point_count{};
  std::size_t order{};
  std::size_t critical_event_support_bound{};
  std::size_t critical_arm_bound{};
  std::size_t required_arm_candidate_capacity{};
  ExactCriticalCatalogTypedGammaRootOverlayDecision
      source_root_overlay_decision{
          ExactCriticalCatalogTypedGammaRootOverlayDecision::not_certified};
  std::vector<
      ExactCriticalCatalogTypedGammaEventLocalArmTargetRootCandidate>
      arm_candidates;
  bool arm_root_composition_candidate_space_size_certified{false};
  bool arm_root_composition_preflight_budget_sufficient{false};
  bool source_journal_is_external_and_not_retained{false};
  bool source_root_overlay_is_external_and_not_retained{false};
  bool source_journal_budget_seam_certified{false};
  bool source_root_overlay_budget_seam_certified{false};
  bool source_root_overlay_fresh_replay_certified{false};
  bool composition_started_only_after_complete_source_pair{false};
  bool every_arm_record_composed_exactly_once{false};
  bool arm_saddle_memberships_preserved{false};
  bool arm_terminal_class_target_chains_preserved{false};
  bool candidate_indices_dense_and_target_binding_indices_exact{false};
  bool target_binding_history_coordinates_match_targets_and_saddles{false};
  bool full_pi0_target_authority_preserved_by_external_indices{false};
  bool reduced_root_dispositions_copied_without_reclassification{false};
  bool shared_targets_preserve_distinct_arm_candidates{false};
  bool candidates_are_event_local_and_not_public_attachments{false};
  bool diagnostic_outcomes_have_no_candidates{false};
  bool critical_catalog_typed_gamma_arm_root_composition_certified{false};
  ExactCriticalCatalogTypedGammaArmRootCompositionCounters counters{};
  ExactCriticalCatalogTypedGammaArmRootCompositionDecision decision{
      ExactCriticalCatalogTypedGammaArmRootCompositionDecision::
          not_certified};
  ExactCriticalCatalogTypedGammaArmRootCompositionScope scope{
      ExactCriticalCatalogTypedGammaArmRootCompositionScope::unspecified};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaArmRootCompositionResult&,
      const ExactCriticalCatalogTypedGammaArmRootCompositionResult&) =
      default;
};

struct ExactCriticalCatalogTypedGammaArmRootCompositionVerification {
  bool requested_budget_certified{false};
  bool external_inputs_certified{false};
  bool derived_preflight_sizes_certified{false};
  bool source_root_overlay_decision_certified{false};
  bool source_root_overlay_fresh_replay_certified{false};
  bool arm_candidates_certified{false};
  bool result_facts_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_critical_catalog_typed_gamma_arm_root_composition_decision_certified{
      false};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaArmRootCompositionVerification&,
      const ExactCriticalCatalogTypedGammaArmRootCompositionVerification&) =
      default;
};

// Checks every nested scalar cap without starting geometry.  Later bounded
// overlays reuse this fail-closed entry point before their own preflight.
void validate_exact_critical_catalog_typed_gamma_arm_root_composition_budget_caps(
    const ExactCriticalCatalogTypedGammaArmRootCompositionBudget& budget);

[[nodiscard]] ExactCriticalCatalogTypedGammaArmRootCompositionResult
build_exact_critical_catalog_typed_gamma_arm_root_composition(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactCriticalCatalogTypedGammaArmRootCompositionBudget budget,
    const ExactCriticalCatalogTypedGammaJournalResult& source_journal,
    const ExactCriticalCatalogTypedGammaRootOverlayResult&
        source_root_overlay);

// Both external sources and the observed composition are untrusted.  The
// 6.19 verifier recertifies the journal transitively; no observed candidate,
// target, disposition or local root ID steers the fresh composition.
[[nodiscard]] ExactCriticalCatalogTypedGammaArmRootCompositionVerification
verify_exact_critical_catalog_typed_gamma_arm_root_composition(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactCriticalCatalogTypedGammaArmRootCompositionBudget budget,
    const ExactCriticalCatalogTypedGammaJournalResult& source_journal,
    const ExactCriticalCatalogTypedGammaRootOverlayResult&
        source_root_overlay,
    const ExactCriticalCatalogTypedGammaArmRootCompositionResult& result);

}  // namespace morsehgp3d::hierarchy
