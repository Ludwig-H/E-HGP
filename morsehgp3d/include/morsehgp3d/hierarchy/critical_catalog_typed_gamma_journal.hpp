#pragma once

#include "morsehgp3d/hierarchy/critical_catalog_arm_gamma_overlay.hpp"
#include "morsehgp3d/hierarchy/critical_catalog_reduced_gamma_overlay.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace morsehgp3d::hierarchy {

// Both 6.16 and 6.17 remain transient producers.  The nine scalar capacities
// cover only the typed, single-order reconciliation journal and are checked
// from (n, k) before either producer starts geometry.
struct ExactCriticalCatalogTypedGammaJournalBudget {
  static constexpr std::size_t maximum_supported_label_entry_count =
      6435U;
  static constexpr std::size_t maximum_supported_saddle_record_count =
      1456U;
  static constexpr std::size_t
      maximum_supported_terminal_class_record_count = 5824U;
  static constexpr std::size_t maximum_supported_arm_record_count =
      5824U;
  static constexpr std::size_t
      maximum_supported_strict_target_record_count = 5824U;
  static constexpr std::size_t
      maximum_supported_terminal_class_point_id_reference_count = 64064U;
  static constexpr std::size_t
      maximum_supported_saddle_index_reference_count = 11648U;
  static constexpr std::size_t
      maximum_supported_target_facet_reference_count = 4996992U;
  static constexpr std::size_t
      maximum_supported_target_point_id_reference_count = 35025536U;

  ExactCriticalCatalogReducedGammaOverlayBudget
      provenance_overlay_budget{};
  ExactCriticalCatalogArmGammaOverlayBudget arm_overlay_budget{};
  std::size_t maximum_label_entry_count{};
  std::size_t maximum_saddle_record_count{};
  std::size_t maximum_terminal_class_record_count{};
  std::size_t maximum_arm_record_count{};
  std::size_t maximum_strict_target_record_count{};
  std::size_t maximum_terminal_class_point_id_reference_count{};
  std::size_t maximum_saddle_index_reference_count{};
  std::size_t maximum_target_facet_reference_count{};
  std::size_t maximum_target_point_id_reference_count{};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaJournalBudget&,
      const ExactCriticalCatalogTypedGammaJournalBudget&) = default;
};

enum class ExactCriticalCatalogTypedGammaJournalDecision : std::uint8_t {
  not_certified,
  no_journal_preflight_budget_insufficient,
  no_journal_provenance_overlay_incomplete,
  no_journal_arm_overlay_incomplete,
  complete_exhaustive_typed_gamma_journal,
};

enum class ExactCriticalCatalogTypedGammaJournalScope : std::uint8_t {
  unspecified,
  bounded_n14_k10_single_order_exhaustive_gamma_groups_typed_catalog_h0_roles_and_strict_full_pi0_arm_targets_with_separate_hgp_reduced_effect_annotations_only,
};

enum class ExactCriticalCatalogTypedGammaLabelSemantic : std::uint8_t {
  catalog_birth,
  catalog_saddle,
  residual_newly_active_facet,
  residual_equal_level_coface,
};

// There is exactly one entry for every 6.16 history-label slot.  Source
// indices are optional precisely because exhaustive Gamma retains residual
// labels with no accepted catalogue H0 provenance.
struct ExactCriticalCatalogTypedGammaLabelEntry {
  std::size_t label_entry_index{};
  std::size_t history_batch_index{};
  std::size_t history_group_record_index{};
  ExactCriticalCatalogReducedGammaHistoryLabelKind history_label_kind{
      ExactCriticalCatalogReducedGammaHistoryLabelKind::
          newly_active_facet};
  std::size_t history_group_local_label_index{};
  ExactCriticalCatalogTypedGammaLabelSemantic semantic{
      ExactCriticalCatalogTypedGammaLabelSemantic::
          residual_newly_active_facet};
  std::optional<std::size_t> source_event_projection_index;
  std::optional<std::size_t> catalog_event_index;
  std::optional<std::size_t> catalog_h0_batch_index;
  std::optional<std::size_t> saddle_record_index;

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaLabelEntry&,
      const ExactCriticalCatalogTypedGammaLabelEntry&) = default;
};

struct ExactCriticalCatalogTypedGammaSaddleRecord {
  std::size_t saddle_record_index{};
  std::size_t label_entry_index{};
  std::size_t source_event_projection_index{};
  std::size_t catalog_event_index{};
  std::size_t catalog_h0_batch_index{};
  std::size_t source_saddle_family_record_index{};
  std::size_t source_arm_gamma_batch_record_index{};
  std::size_t history_batch_index{};
  std::size_t history_group_record_index{};
  ExactReducedGammaBatchGroupKind reduced_group_kind{
      ExactReducedGammaBatchGroupKind::deferred_isolated_facet};
  std::vector<std::size_t> terminal_class_record_indices;
  std::vector<std::size_t> arm_record_indices;

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaSaddleRecord&,
      const ExactCriticalCatalogTypedGammaSaddleRecord&) = default;
};

struct ExactCriticalCatalogTypedGammaTerminalClassRecord {
  std::size_t terminal_class_record_index{};
  std::size_t saddle_record_index{};
  std::size_t source_terminal_label_class_index{};
  ExactCriticalArmTerminalLabelClass terminal_class;
  std::size_t strict_target_record_index{};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaTerminalClassRecord&,
      const ExactCriticalCatalogTypedGammaTerminalClassRecord&) =
      default;
};

struct ExactCriticalCatalogTypedGammaArmRecord {
  std::size_t arm_record_index{};
  std::size_t saddle_record_index{};
  std::size_t terminal_class_record_index{};
  std::size_t source_arm_target_index{};
  spatial::PointId removed_shell_point_id{};
  std::size_t strict_target_record_index{};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaArmRecord&,
      const ExactCriticalCatalogTypedGammaArmRecord&) = default;
};

// Full strict full-pi0 witnesses retain target authority.  The reduced
// component kind below and the group kind stored on the saddle are copied
// annotations; neither selects a target or implies a root.
struct ExactCriticalCatalogTypedGammaStrictTargetRecord {
  std::size_t strict_target_record_index{};
  std::size_t source_target_component_index{};
  std::size_t source_arm_gamma_batch_record_index{};
  std::size_t history_batch_index{};
  std::size_t history_group_record_index{};
  std::size_t strict_component_index{};
  ExactStrictGammaComponentWitness strict_component;
  ExactReducedGammaStrictComponentKind reduced_component_kind{
      ExactReducedGammaStrictComponentKind::omitted_isolated_facet};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaStrictTargetRecord&,
      const ExactCriticalCatalogTypedGammaStrictTargetRecord&) =
      default;
};

struct ExactCriticalCatalogTypedGammaJournalCounters {
  std::size_t preflight_count{};
  std::size_t provenance_overlay_build_count{};
  std::size_t provenance_overlay_verification_count{};
  std::size_t arm_overlay_build_count{};
  std::size_t arm_overlay_verification_count{};
  std::size_t source_catalog_comparison_count{};
  std::size_t history_verification_count{};
  std::size_t label_entry_count{};
  std::size_t catalog_birth_label_count{};
  std::size_t catalog_saddle_label_count{};
  std::size_t residual_newly_active_facet_label_count{};
  std::size_t residual_equal_level_coface_label_count{};
  std::size_t saddle_record_count{};
  std::size_t terminal_class_record_count{};
  std::size_t terminal_class_point_id_reference_count{};
  std::size_t arm_record_count{};
  std::size_t saddle_index_reference_count{};
  std::size_t strict_target_record_count{};
  std::size_t target_facet_reference_count{};
  std::size_t target_point_id_reference_count{};
  std::size_t shared_strict_target_arm_count{};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaJournalCounters&,
      const ExactCriticalCatalogTypedGammaJournalCounters&) = default;
};

// The complete payload retains exactly one freshly verified reduced-Gamma
// history plus the typed reconciliation layers below.  Both source overlays,
// their catalogues, arm families and chains are transient.  A diagnostic
// decision retains neither the history nor any journal record.
struct ExactCriticalCatalogTypedGammaJournalResult {
  static constexpr std::size_t minimum_supported_point_count = 3U;
  static constexpr std::size_t maximum_supported_point_count = 14U;
  static constexpr std::size_t minimum_supported_order = 2U;
  static constexpr std::size_t maximum_supported_order = 10U;
  static constexpr const char* proof_basis =
      "exact_fresh_catalog_h0_provenance_and_strict_full_pi0_arm_"
      "targets_reconciled_through_one_typed_single_order_reduced_"
      "gamma_journal_v1";

  ExactCriticalCatalogTypedGammaJournalBudget requested_budget{};
  std::size_t point_count{};
  std::size_t order{};
  std::size_t exhaustive_facet_count{};
  std::size_t exhaustive_coface_count{};
  std::size_t critical_event_support_bound{};
  std::size_t critical_arm_bound{};
  std::size_t required_label_entry_capacity{};
  std::size_t required_saddle_record_capacity{};
  std::size_t required_terminal_class_record_capacity{};
  std::size_t required_arm_record_capacity{};
  std::size_t required_strict_target_record_capacity{};
  std::size_t required_terminal_class_point_id_reference_capacity{};
  std::size_t required_saddle_index_reference_capacity{};
  std::size_t required_target_facet_reference_capacity{};
  std::size_t required_target_point_id_reference_capacity{};
  ExactCriticalCatalogReducedGammaOverlayDecision
      provenance_overlay_decision{
          ExactCriticalCatalogReducedGammaOverlayDecision::
              not_certified};
  ExactCriticalCatalogArmGammaOverlayDecision arm_overlay_decision{
      ExactCriticalCatalogArmGammaOverlayDecision::not_certified};
  std::optional<ExactPersistentReducedGammaOrderHistory>
      reduced_gamma_history;
  std::vector<ExactCriticalCatalogTypedGammaLabelEntry> label_entries;
  std::vector<ExactCriticalCatalogTypedGammaSaddleRecord>
      saddle_records;
  std::vector<ExactCriticalCatalogTypedGammaTerminalClassRecord>
      terminal_class_records;
  std::vector<ExactCriticalCatalogTypedGammaArmRecord> arm_records;
  std::vector<ExactCriticalCatalogTypedGammaStrictTargetRecord>
      strict_target_records;
  bool journal_candidate_space_size_certified{false};
  bool journal_preflight_budget_sufficient{false};
  bool source_budget_seam_certified{false};
  bool subordinate_geometry_started_only_after_successful_journal_preflight{
      false};
  bool provenance_overlay_fresh_replay_certified{false};
  bool arm_overlay_started_only_after_complete_provenance{false};
  bool arm_overlay_fresh_replay_certified{false};
  bool source_catalogs_identical{false};
  bool source_objects_transient{false};
  bool all_history_label_slots_typed_exactly_once{false};
  bool catalog_births_are_deferred_facets_without_saddles_or_arms{false};
  bool residual_labels_typed_only_from_history_kind{false};
  bool catalog_saddles_are_non_deferred_equal_level_cofaces{false};
  bool every_catalog_saddle_has_exactly_one_record{false};
  bool every_terminal_class_has_one_shared_strict_target{false};
  bool every_arm_has_one_terminal_class_and_strict_target{false};
  // Every source family, arm and class is consumed once; every deduplicated
  // source target is copied once even when several arms reference that copy.
  bool every_source_family_arm_class_and_target_used_exactly_once{false};
  bool arm_initial_facets_derived_from_closed_labels{false};
  bool full_pi0_witnesses_retain_target_authority{false};
  bool reduced_component_and_group_kinds_are_annotations_only{false};
  bool all_saddle_targets_join_their_history_group{false};
  bool reduced_gamma_history_stored_fresh{false};
  bool diagnostic_outcomes_have_no_journal_or_history{false};
  bool critical_catalog_typed_gamma_journal_certified{false};
  ExactCriticalCatalogTypedGammaJournalCounters counters{};
  ExactCriticalCatalogTypedGammaJournalDecision decision{
      ExactCriticalCatalogTypedGammaJournalDecision::not_certified};
  ExactCriticalCatalogTypedGammaJournalScope scope{
      ExactCriticalCatalogTypedGammaJournalScope::unspecified};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaJournalResult&,
      const ExactCriticalCatalogTypedGammaJournalResult&) = default;
};

struct ExactCriticalCatalogTypedGammaJournalVerification {
  bool requested_budget_certified{false};
  bool external_inputs_certified{false};
  bool derived_preflight_sizes_certified{false};
  bool source_decisions_certified{false};
  bool reduced_gamma_history_certified{false};
  bool label_entries_certified{false};
  bool saddle_records_certified{false};
  bool terminal_class_records_certified{false};
  bool arm_records_certified{false};
  bool strict_target_records_certified{false};
  bool result_facts_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_critical_catalog_typed_gamma_journal_decision_certified{
      false};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaJournalVerification&,
      const ExactCriticalCatalogTypedGammaJournalVerification&) =
      default;
};

[[nodiscard]] ExactCriticalCatalogTypedGammaJournalResult
build_exact_critical_catalog_typed_gamma_journal(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactCriticalCatalogTypedGammaJournalBudget budget);

// Reconstructs both transient overlays and every typed record from the
// external cloud, order and trusted composite budget.  No observed source
// decision, history, index, annotation or witness steers replay.
[[nodiscard]] ExactCriticalCatalogTypedGammaJournalVerification
verify_exact_critical_catalog_typed_gamma_journal(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactCriticalCatalogTypedGammaJournalBudget budget,
    const ExactCriticalCatalogTypedGammaJournalResult& result);

}  // namespace morsehgp3d::hierarchy
