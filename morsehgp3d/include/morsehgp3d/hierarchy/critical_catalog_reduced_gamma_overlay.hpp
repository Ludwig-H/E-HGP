#pragma once

#include "morsehgp3d/hierarchy/critical_catalog.hpp"
#include "morsehgp3d/hierarchy/reduced_gamma_history.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace morsehgp3d::hierarchy {

// The two subordinate budgets retain their original authority.  The six
// scalar capacities cover only the bounded, compact reconciliation layer and
// are checked from (n, k) before either subordinate geometry builder starts.
struct ExactCriticalCatalogReducedGammaOverlayBudget {
  static constexpr std::size_t maximum_supported_event_projection_count =
      1456U;
  static constexpr std::size_t maximum_supported_group_overlay_count =
      6435U;
  static constexpr std::size_t maximum_supported_label_slot_count = 6435U;
  static constexpr std::size_t
      maximum_supported_history_point_id_scan_count = 48048U;
  static constexpr std::size_t
      maximum_supported_catalog_point_id_scan_count = 16016U;
  static constexpr std::size_t
      maximum_supported_group_event_reference_count = 1456U;

  ExactCriticalCatalogBudget critical_catalog_budget{};
  ExactPersistentReducedGammaOrderHistoryBudget
      reduced_gamma_history_budget{};
  std::size_t maximum_event_projection_count{};
  std::size_t maximum_group_overlay_count{};
  std::size_t maximum_label_slot_count{};
  std::size_t maximum_history_point_id_scan_count{};
  std::size_t maximum_catalog_point_id_scan_count{};
  std::size_t maximum_group_event_reference_count{};

  friend bool operator==(
      const ExactCriticalCatalogReducedGammaOverlayBudget&,
      const ExactCriticalCatalogReducedGammaOverlayBudget&) = default;
};

enum class ExactCriticalCatalogReducedGammaOverlayDecision : std::uint8_t {
  not_certified,
  no_overlay_preflight_budget_insufficient,
  no_catalog_preflight_budget_insufficient,
  no_overlay_relevant_extra_shell_degeneracy,
  no_history_preflight_budget_insufficient,
  complete_exhaustive_critical_catalog_reduced_gamma_overlay,
};

enum class ExactCriticalCatalogReducedGammaOverlayScope : std::uint8_t {
  unspecified,
  bounded_n14_k10_single_order_freshly_verified_critical_catalog_h0_provenance_to_exhaustive_persistent_reduced_gamma_history_groups_only,
};

enum class ExactCriticalCatalogReducedGammaEventRole : std::uint8_t {
  birth,
  saddle,
};

enum class ExactCriticalCatalogReducedGammaHistoryLabelKind : std::uint8_t {
  newly_active_facet,
  equal_level_coface,
};

// Indices are into the two optional, freshly verified source objects stored in
// the result.  The closed label itself is deliberately not copied.
struct ExactCriticalCatalogReducedGammaEventProjection {
  std::size_t projection_index{};
  std::size_t catalog_event_index{};
  std::size_t catalog_h0_batch_index{};
  ExactCriticalCatalogReducedGammaEventRole role{
      ExactCriticalCatalogReducedGammaEventRole::birth};
  std::size_t history_batch_index{};
  std::size_t history_group_record_index{};
  std::size_t history_label_slot_index{};
  ExactCriticalCatalogReducedGammaHistoryLabelKind history_label_kind{
      ExactCriticalCatalogReducedGammaHistoryLabelKind::
          newly_active_facet};
  std::size_t history_group_local_label_index{};

  friend bool operator==(
      const ExactCriticalCatalogReducedGammaEventProjection&,
      const ExactCriticalCatalogReducedGammaEventProjection&) = default;
};

// There is exactly one slot for every newly active facet and equal-level
// coface retained by 6.14.  A missing projection is an explicit residual
// Gamma incidence, never permission to discard the label or its group.
struct ExactCriticalCatalogReducedGammaHistoryLabelSlot {
  std::size_t label_slot_index{};
  std::size_t history_batch_index{};
  std::size_t history_group_record_index{};
  ExactCriticalCatalogReducedGammaHistoryLabelKind kind{
      ExactCriticalCatalogReducedGammaHistoryLabelKind::
          newly_active_facet};
  std::size_t history_group_local_label_index{};
  std::optional<std::size_t> event_projection_index;

  friend bool operator==(
      const ExactCriticalCatalogReducedGammaHistoryLabelSlot&,
      const ExactCriticalCatalogReducedGammaHistoryLabelSlot&) = default;
};

// Group existence, incidence and kind remain properties of the stored 6.14
// history.  This record only gives one contiguous range of equality slots and
// the catalogue-provenance references that fall in it.
struct ExactCriticalCatalogReducedGammaGroupOverlay {
  std::size_t history_group_record_index{};
  std::size_t history_batch_index{};
  std::size_t first_label_slot_index{};
  std::size_t label_slot_count{};
  std::vector<std::size_t> birth_event_projection_indices;
  std::vector<std::size_t> saddle_event_projection_indices;
  bool has_catalog_h0_provenance{false};

  friend bool operator==(
      const ExactCriticalCatalogReducedGammaGroupOverlay&,
      const ExactCriticalCatalogReducedGammaGroupOverlay&) = default;
};

struct ExactCriticalCatalogReducedGammaOverlayCounters {
  std::size_t preflight_count{};
  std::size_t critical_catalog_build_count{};
  std::size_t critical_catalog_verification_count{};
  std::size_t reduced_gamma_history_build_count{};
  std::size_t reduced_gamma_history_verification_count{};
  std::size_t catalog_h0_batch_scan_count{};
  std::size_t catalog_birth_reference_count{};
  std::size_t catalog_saddle_reference_count{};
  std::size_t catalog_closed_label_point_id_scan_count{};
  std::size_t history_batch_count{};
  std::size_t history_group_count{};
  std::size_t history_newly_active_facet_slot_count{};
  std::size_t history_equal_level_coface_slot_count{};
  std::size_t history_point_id_scan_count{};
  std::size_t label_slot_count{};
  std::size_t event_projection_count{};
  std::size_t birth_event_projection_count{};
  std::size_t saddle_event_projection_count{};
  std::size_t residual_newly_active_facet_slot_count{};
  std::size_t residual_equal_level_coface_slot_count{};
  std::size_t group_event_reference_count{};
  std::size_t group_with_catalog_provenance_count{};
  std::size_t group_without_catalog_provenance_count{};

  friend bool operator==(
      const ExactCriticalCatalogReducedGammaOverlayCounters&,
      const ExactCriticalCatalogReducedGammaOverlayCounters&) = default;
};

// This is an exhaustive provenance overlay, not a second incidence engine.
// The catalogue contributes only exact H0 event identities.  Gamma supplies
// every group, equality label and reduced-group kind, including residual
// labels without accepted 6.12 H0 provenance.  No root receives a public
// Morse type here.
struct ExactCriticalCatalogReducedGammaOverlayResult {
  static constexpr std::size_t minimum_supported_point_count = 3U;
  static constexpr std::size_t maximum_supported_point_count = 14U;
  static constexpr std::size_t minimum_supported_order = 2U;
  static constexpr std::size_t maximum_supported_order = 10U;
  static constexpr const char* proof_basis =
      "exact_critical_closed_label_h0_references_reconciled_with_"
      "exhaustive_persistent_reduced_gamma_equality_slots_v1";

  ExactCriticalCatalogReducedGammaOverlayBudget requested_budget{};
  std::size_t point_count{};
  std::size_t order{};
  std::size_t exhaustive_facet_count{};
  std::size_t exhaustive_coface_count{};
  std::size_t required_event_projection_capacity{};
  std::size_t required_group_overlay_capacity{};
  std::size_t required_label_slot_capacity{};
  std::size_t required_history_point_id_scan_capacity{};
  std::size_t required_catalog_point_id_scan_capacity{};
  std::size_t required_group_event_reference_capacity{};
  std::optional<ExactCriticalCatalogResult> critical_catalog;
  std::optional<ExactPersistentReducedGammaOrderHistory>
      reduced_gamma_history;
  std::vector<ExactCriticalCatalogReducedGammaEventProjection>
      event_projections;
  std::vector<ExactCriticalCatalogReducedGammaHistoryLabelSlot>
      history_label_slots;
  std::vector<ExactCriticalCatalogReducedGammaGroupOverlay>
      group_overlays;
  bool overlay_candidate_space_size_certified{false};
  bool overlay_preflight_budget_sufficient{false};
  bool subordinate_geometry_started_only_after_successful_overlay_preflight{
      false};
  bool critical_catalog_fresh_replay_certified{false};
  bool no_relevant_extra_shell_degeneracy{false};
  bool reduced_gamma_history_fresh_replay_certified{false};
  bool history_equality_slots_exhaustively_indexed{false};
  bool closed_label_theorem_applied_to_every_projection{false};
  bool every_catalog_h0_role_projected_exactly_once{false};
  bool every_history_label_slot_partitioned_by_provenance{false};
  bool catalog_births_exactly_deferred_newly_active_facets{false};
  bool catalog_saddles_only_non_deferred_groups{false};
  bool group_kinds_inherited_only_from_history{false};
  bool simultaneous_history_batches_preserved{false};
  bool birth_projection_plus_residual_facet_count_equals_exhaustive_facet_count{
      false};
  bool saddle_projection_plus_residual_coface_count_equals_exhaustive_coface_count{
      false};
  bool critical_catalog_reduced_gamma_overlay_certified{false};
  ExactCriticalCatalogReducedGammaOverlayCounters counters{};
  ExactCriticalCatalogReducedGammaOverlayDecision decision{
      ExactCriticalCatalogReducedGammaOverlayDecision::not_certified};
  ExactCriticalCatalogReducedGammaOverlayScope scope{
      ExactCriticalCatalogReducedGammaOverlayScope::unspecified};

  friend bool operator==(
      const ExactCriticalCatalogReducedGammaOverlayResult&,
      const ExactCriticalCatalogReducedGammaOverlayResult&) = default;
};

struct ExactCriticalCatalogReducedGammaOverlayVerification {
  bool requested_budget_certified{false};
  bool external_inputs_certified{false};
  bool derived_preflight_sizes_certified{false};
  bool critical_catalog_certified{false};
  bool reduced_gamma_history_certified{false};
  bool event_projections_certified{false};
  bool history_label_slots_certified{false};
  bool group_overlays_certified{false};
  bool result_facts_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_critical_catalog_reduced_gamma_overlay_decision_certified{
      false};

  friend bool operator==(
      const ExactCriticalCatalogReducedGammaOverlayVerification&,
      const ExactCriticalCatalogReducedGammaOverlayVerification&) = default;
};

[[nodiscard]] ExactCriticalCatalogReducedGammaOverlayResult
build_exact_critical_catalog_reduced_gamma_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactCriticalCatalogReducedGammaOverlayBudget budget);

// Rebuilds both subordinate exact sources and every overlay layer from the
// external cloud, order and trusted composite budget.  No observed source,
// projection, slot, group, fact or decision selects a replay branch.
[[nodiscard]] ExactCriticalCatalogReducedGammaOverlayVerification
verify_exact_critical_catalog_reduced_gamma_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactCriticalCatalogReducedGammaOverlayBudget budget,
    const ExactCriticalCatalogReducedGammaOverlayResult& result);

}  // namespace morsehgp3d::hierarchy
