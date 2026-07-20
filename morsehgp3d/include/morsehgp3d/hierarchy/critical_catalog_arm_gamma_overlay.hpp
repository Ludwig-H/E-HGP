#pragma once

#include "morsehgp3d/hierarchy/critical_arm.hpp"
#include "morsehgp3d/hierarchy/critical_catalog.hpp"
#include "morsehgp3d/hierarchy/reduced_gamma_batch.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace morsehgp3d::hierarchy {

// The three subordinate budgets retain their original authority.  The seven
// scalar capacities cover only the exhaustive single-order reconciliation
// layer and are derived from (n, k) before any subordinate geometry starts.
struct ExactCriticalCatalogArmGammaOverlayBudget {
  static constexpr std::size_t maximum_supported_saddle_event_count =
      1456U;
  static constexpr std::size_t maximum_supported_arm_count = 5824U;
  static constexpr std::size_t maximum_supported_saddle_batch_count =
      1456U;
  static constexpr std::size_t maximum_supported_target_component_count =
      5824U;
  static constexpr std::size_t
      maximum_supported_target_component_facet_reference_count =
          4996992U;
  static constexpr std::size_t
      maximum_supported_target_component_point_id_reference_count =
          35025536U;
  static constexpr std::size_t
      maximum_supported_committed_chain_segment_count = 23855104U;

  ExactCriticalCatalogBudget critical_catalog_budget{};
  ExactFacetDescentChainBudget per_arm_chain_budget{};
  ExactStrictGammaBudget reduced_gamma_batch_budget{};
  std::size_t maximum_saddle_event_count{};
  std::size_t maximum_arm_count{};
  std::size_t maximum_saddle_batch_count{};
  std::size_t maximum_target_component_count{};
  std::size_t maximum_target_component_facet_reference_count{};
  std::size_t maximum_target_component_point_id_reference_count{};
  std::size_t maximum_committed_chain_segment_count{};

  friend bool operator==(
      const ExactCriticalCatalogArmGammaOverlayBudget&,
      const ExactCriticalCatalogArmGammaOverlayBudget&) = default;
};

enum class ExactCriticalCatalogArmGammaOverlayDecision : std::uint8_t {
  not_certified,
  no_overlay_preflight_budget_insufficient,
  no_catalog_preflight_budget_insufficient,
  no_overlay_relevant_extra_shell_degeneracy,
  no_overlay_incomplete_critical_arm_family,
  no_overlay_reduced_gamma_batch_preflight_budget_insufficient,
  complete_exhaustive_catalog_saddle_arm_gamma_overlay,
};

enum class ExactCriticalCatalogArmGammaOverlayScope : std::uint8_t {
  unspecified,
  bounded_n14_k10_single_order_fresh_catalog_all_index_one_saddle_arm_families_to_exhaustive_strict_gamma_full_pi0_components_with_reduced_annotations_only,
};

// The complete 6.7 family retains every independently certified arm path.
// The reduced-Gamma indices are populated only after every selected family
// and every selected equal-level batch have completed successfully.
struct ExactCriticalCatalogArmGammaSaddleFamilyRecord {
  std::size_t saddle_family_record_index{};
  std::size_t catalog_event_index{};
  std::size_t catalog_h0_batch_index{};
  ExactCriticalArmFamilyResult family;
  std::optional<std::size_t> reduced_gamma_batch_record_index;
  std::optional<std::size_t> reduced_gamma_group_index;
  std::optional<ExactReducedGammaBatchGroupKind>
      reduced_gamma_group_kind;
  std::vector<std::size_t> arm_target_indices;
};

// Exactly one compact record is retained for each catalogue H0 batch
// containing at least one saddle of the requested order.  Its 6.13 result is
// verified transiently: only target components actually reached by arms are
// copied into the bounded component arena below.
struct ExactCriticalCatalogArmGammaBatchRecord {
  std::size_t batch_record_index{};
  std::size_t catalog_h0_batch_index{};
  exact::ExactLevel squared_level;
  std::vector<std::size_t> saddle_family_record_indices;
  std::vector<std::size_t> target_component_indices;

  friend bool operator==(
      const ExactCriticalCatalogArmGammaBatchRecord&,
      const ExactCriticalCatalogArmGammaBatchRecord&) = default;
};

// This arena is deduplicated by (batch_record_index,
// strict_component_index).  The full strict full-pi0 witness is the target;
// the reduced kind is copied separately and never selects that target.
struct ExactCriticalCatalogArmGammaTargetComponent {
  std::size_t target_component_index{};
  std::size_t batch_record_index{};
  std::size_t strict_component_index{};
  ExactStrictGammaComponentWitness strict_component;
  ExactReducedGammaStrictComponentKind reduced_component_kind{
      ExactReducedGammaStrictComponentKind::omitted_isolated_facet};

  friend bool operator==(
      const ExactCriticalCatalogArmGammaTargetComponent&,
      const ExactCriticalCatalogArmGammaTargetComponent&) = default;
};

// There is exactly one target record for every
// (catalog_event_index, order, removed_shell_point_id).  Several arms may
// intentionally share one target_component_index.
struct ExactCriticalCatalogArmGammaArmTarget {
  std::size_t arm_target_index{};
  std::size_t saddle_family_record_index{};
  std::size_t catalog_event_index{};
  std::size_t order{};
  spatial::PointId removed_shell_point_id{};
  std::size_t terminal_label_class_index{};
  std::size_t batch_record_index{};
  std::size_t strict_component_index{};
  std::size_t target_component_index{};

  friend bool operator==(
      const ExactCriticalCatalogArmGammaArmTarget&,
      const ExactCriticalCatalogArmGammaArmTarget&) = default;
};

struct ExactCriticalCatalogArmGammaOverlayCounters {
  std::size_t preflight_count{};
  std::size_t critical_catalog_build_count{};
  std::size_t critical_catalog_verification_count{};
  std::size_t catalog_h0_batch_scan_count{};
  std::size_t catalog_saddle_event_reference_count{};
  std::size_t critical_arm_family_build_count{};
  std::size_t critical_arm_family_verification_count{};
  std::size_t catalog_source_replay_comparison_count{};
  std::size_t critical_arm_count{};
  std::size_t complete_critical_arm_family_count{};
  std::size_t incomplete_critical_arm_family_count{};
  std::size_t committed_chain_segment_count{};
  std::size_t reduced_gamma_batch_build_count{};
  std::size_t reduced_gamma_batch_verification_count{};
  std::size_t complete_reduced_gamma_batch_count{};
  std::size_t insufficient_reduced_gamma_batch_count{};
  std::size_t saddle_coface_group_lookup_count{};
  std::size_t saddle_coface_label_scan_count{};
  std::size_t arm_initial_component_lookup_count{};
  std::size_t arm_terminal_component_lookup_count{};
  std::size_t strict_component_facet_label_scan_count{};
  std::size_t target_component_count{};
  std::size_t target_component_facet_reference_count{};
  std::size_t target_component_point_id_reference_count{};
  std::size_t arm_target_count{};
  std::size_t shared_target_arm_count{};

  friend bool operator==(
      const ExactCriticalCatalogArmGammaOverlayCounters&,
      const ExactCriticalCatalogArmGammaOverlayCounters&) = default;
};

// This result closes the exhaustive catalogue-to-arm-to-Gamma proof seam for
// one order and remains a bounded local certificate only.  Reduced group
// kinds remain properties copied from 6.13 and are never inferred from a
// Morse saddle role.
struct ExactCriticalCatalogArmGammaOverlayResult {
  static constexpr std::size_t minimum_supported_point_count = 3U;
  static constexpr std::size_t maximum_supported_point_count = 14U;
  static constexpr std::size_t minimum_supported_order = 2U;
  static constexpr std::size_t maximum_supported_order = 10U;
  static constexpr const char* proof_basis =
      "exact_exhaustive_critical_catalog_index_one_arm_families_"
      "reconciled_with_strict_gamma_full_pi0_components_and_separate_"
      "reduced_annotations_v1";

  ExactCriticalCatalogArmGammaOverlayBudget requested_budget{};
  std::size_t point_count{};
  std::size_t order{};
  std::size_t exhaustive_facet_count{};
  std::size_t required_saddle_event_capacity{};
  std::size_t required_arm_capacity{};
  std::size_t required_saddle_batch_capacity{};
  std::size_t required_target_component_capacity{};
  std::size_t required_target_component_facet_reference_capacity{};
  std::size_t required_target_component_point_id_reference_capacity{};
  std::size_t required_committed_chain_segment_capacity{};
  std::optional<ExactCriticalCatalogResult> critical_catalog;
  std::vector<ExactCriticalCatalogArmGammaSaddleFamilyRecord>
      saddle_family_records;
  std::vector<ExactCriticalCatalogArmGammaBatchRecord> batch_records;
  std::vector<ExactCriticalCatalogArmGammaTargetComponent>
      target_components;
  std::vector<ExactCriticalCatalogArmGammaArmTarget> arm_targets;
  bool overlay_candidate_space_size_certified{false};
  bool overlay_preflight_budget_sufficient{false};
  bool subordinate_geometry_started_only_after_successful_overlay_preflight{
      false};
  bool critical_catalog_fresh_replay_certified{false};
  bool no_relevant_extra_shell_degeneracy{false};
  bool requested_order_saddles_exhaustively_selected{false};
  bool critical_arm_families_fresh_replay_certified{false};
  bool catalog_sources_match_all_arm_families{false};
  bool all_critical_arm_families_complete{false};
  bool reduced_gamma_batches_fresh_replay_certified{false};
  bool one_reduced_gamma_batch_per_saddle_h0_batch{false};
  bool every_saddle_coface_in_unique_non_deferred_group{false};
  bool every_arm_initial_and_terminal_in_same_unique_group_strict_component{
      false};
  bool target_components_deduplicated_by_batch_and_strict_component{false};
  bool target_components_retain_full_pi0_witnesses{false};
  bool reduced_component_kinds_copied_separately{false};
  bool reduced_group_kinds_inherited_without_morse_inference{false};
  bool every_catalog_saddle_arm_has_one_target{false};
  bool diagnostic_outcomes_have_no_gamma_targets{false};
  bool critical_catalog_arm_gamma_overlay_certified{false};
  ExactCriticalCatalogArmGammaOverlayCounters counters{};
  ExactCriticalCatalogArmGammaOverlayDecision decision{
      ExactCriticalCatalogArmGammaOverlayDecision::not_certified};
  ExactCriticalCatalogArmGammaOverlayScope scope{
      ExactCriticalCatalogArmGammaOverlayScope::unspecified};
};

struct ExactCriticalCatalogArmGammaOverlayVerification {
  bool requested_budget_certified{false};
  bool external_inputs_certified{false};
  bool derived_preflight_sizes_certified{false};
  bool critical_catalog_certified{false};
  bool saddle_family_records_certified{false};
  bool batch_records_certified{false};
  bool target_components_certified{false};
  bool arm_targets_certified{false};
  bool result_facts_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_critical_catalog_arm_gamma_overlay_decision_certified{false};

  friend bool operator==(
      const ExactCriticalCatalogArmGammaOverlayVerification&,
      const ExactCriticalCatalogArmGammaOverlayVerification&) = default;
};

[[nodiscard]] ExactCriticalCatalogArmGammaOverlayResult
build_exact_critical_catalog_arm_gamma_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactCriticalCatalogArmGammaOverlayBudget budget);

// Reconstructs the catalogue, every selected 6.7 family, every selected 6.13
// batch and both target arenas from the external cloud, order and trusted
// composite budget.  No observed payload or decision steers replay.
[[nodiscard]] ExactCriticalCatalogArmGammaOverlayVerification
verify_exact_critical_catalog_arm_gamma_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactCriticalCatalogArmGammaOverlayBudget budget,
    const ExactCriticalCatalogArmGammaOverlayResult& result);

}  // namespace morsehgp3d::hierarchy
