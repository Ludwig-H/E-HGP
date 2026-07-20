#pragma once

#include "morsehgp3d/hierarchy/critical_catalog_typed_gamma_journal.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace morsehgp3d::hierarchy {

// The source journal is supplied externally and remains unowned.  These ten
// scalar capacities cover only the bounded target-to-pre-batch-root sweep.
// They are derived from (n, k) before the external journal is geometrically
// recertified or any source-sized arena is allocated.
struct ExactCriticalCatalogTypedGammaRootOverlayBudget {
  static constexpr std::size_t
      maximum_supported_target_root_binding_count = 5824U;
  static constexpr std::size_t maximum_supported_live_root_state_count =
      858U;
  static constexpr std::size_t
      maximum_supported_live_root_facet_reference_count = 6864U;
  static constexpr std::size_t
      maximum_supported_live_root_point_id_reference_count = 48048U;
  static constexpr std::size_t
      maximum_supported_root_facet_replay_work_count = 22084920U;
  static constexpr std::size_t
      maximum_supported_root_point_id_replay_work_count = 154594440U;
  static constexpr std::size_t
      maximum_supported_target_facet_comparison_count = 4996992U;
  static constexpr std::size_t
      maximum_supported_target_point_id_comparison_count = 34978944U;
  static constexpr std::size_t
      maximum_supported_snapshot_facet_index_count = 4996992U;
  static constexpr std::size_t
      maximum_supported_snapshot_point_id_index_count = 34978944U;

  ExactCriticalCatalogTypedGammaJournalBudget
      typed_gamma_journal_budget{};
  std::size_t maximum_target_root_binding_count{};
  std::size_t maximum_live_root_state_count{};
  std::size_t maximum_live_root_facet_reference_count{};
  std::size_t maximum_live_root_point_id_reference_count{};
  std::size_t maximum_root_facet_replay_work_count{};
  std::size_t maximum_root_point_id_replay_work_count{};
  std::size_t maximum_target_facet_comparison_count{};
  std::size_t maximum_target_point_id_comparison_count{};
  std::size_t maximum_snapshot_facet_index_count{};
  std::size_t maximum_snapshot_point_id_index_count{};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaRootOverlayBudget&,
      const ExactCriticalCatalogTypedGammaRootOverlayBudget&) = default;
};

enum class ExactCriticalCatalogTypedGammaRootOverlayDecision :
    std::uint8_t {
  not_certified,
  no_root_overlay_preflight_budget_insufficient,
  no_root_overlay_source_journal_rejected,
  no_root_overlay_source_journal_incomplete,
  complete_exhaustive_pre_batch_root_overlay,
};

enum class ExactCriticalCatalogTypedGammaRootOverlayScope : std::uint8_t {
  unspecified,
  bounded_n14_k10_single_order_full_pi0_target_families_to_frozen_pre_batch_local_hgp_reduced_roots_with_explicit_isolated_singletons_only,
};

enum class ExactCriticalCatalogTypedGammaTargetDisposition : std::uint8_t {
  omitted_isolated_singleton,
  matched_pre_batch_persistent_reduced_root,
};

// The source target index is dense and remains relative to the externally
// supplied 6.18 journal.  A local root_node_id is present exactly for an
// exact full-facet-family match in the immutable pre-batch root state.
struct ExactCriticalCatalogTypedGammaTargetRootBinding {
  std::size_t target_root_binding_index{};
  std::size_t strict_target_record_index{};
  std::size_t history_batch_index{};
  std::size_t history_group_record_index{};
  ExactCriticalCatalogTypedGammaTargetDisposition disposition{
      ExactCriticalCatalogTypedGammaTargetDisposition::
          omitted_isolated_singleton};
  std::optional<std::size_t> root_node_id;

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaTargetRootBinding&,
      const ExactCriticalCatalogTypedGammaTargetRootBinding&) = default;
};

struct ExactCriticalCatalogTypedGammaRootOverlayCounters {
  std::size_t preflight_count{};
  std::size_t source_journal_verification_count{};
  std::size_t history_batch_replay_count{};
  std::size_t target_bearing_batch_count{};
  std::size_t history_group_replay_count{};
  std::size_t prior_root_reference_replay_count{};
  std::size_t coverage_delta_replay_count{};
  std::size_t delta_facet_reference_replay_count{};
  std::size_t delta_point_id_reference_replay_count{};
  std::size_t batch_atomic_commit_count{};
  std::size_t root_mutation_count{};
  std::size_t peak_live_root_state_count{};
  std::size_t peak_live_root_facet_reference_count{};
  std::size_t peak_live_root_point_id_reference_count{};
  // Replay, comparison and index counts below are logical payload-reference
  // units.  They do not claim to count each comparison instruction, tree
  // lookup, sort step or defensive rescan performed by the CPU container
  // implementation.
  std::size_t root_facet_replay_work_count{};
  std::size_t root_point_id_replay_work_count{};
  std::size_t snapshot_facet_index_count{};
  std::size_t snapshot_point_id_index_count{};
  std::size_t target_root_binding_count{};
  std::size_t target_facet_comparison_count{};
  std::size_t target_point_id_comparison_count{};
  std::size_t matched_pre_batch_root_target_count{};
  std::size_t omitted_isolated_singleton_target_count{};
  std::size_t group_prior_root_membership_check_count{};
  std::size_t reduced_component_kind_postcheck_count{};
  std::size_t final_active_root_count{};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaRootOverlayCounters&,
      const ExactCriticalCatalogTypedGammaRootOverlayCounters&) = default;
};

// This compact overlay owns no copy of the 6.18 journal.  Its dense indices
// are meaningful only relative to the exact external journal freshly
// recertified from the same cloud, order and trusted composite budget.
struct ExactCriticalCatalogTypedGammaRootOverlayResult {
  static constexpr std::size_t minimum_supported_point_count = 3U;
  static constexpr std::size_t maximum_supported_point_count = 14U;
  static constexpr std::size_t minimum_supported_order = 2U;
  static constexpr std::size_t maximum_supported_order = 10U;
  static constexpr const char* proof_basis =
      "exact_fresh_typed_full_pi0_target_families_reconciled_with_"
      "frozen_pre_batch_local_reduced_gamma_roots_v1";

  ExactCriticalCatalogTypedGammaRootOverlayBudget requested_budget{};
  std::size_t point_count{};
  std::size_t order{};
  std::size_t exhaustive_facet_count{};
  std::size_t exhaustive_coface_count{};
  std::size_t critical_event_support_bound{};
  std::size_t critical_arm_bound{};
  std::size_t maximum_active_root_bound{};
  std::size_t required_target_root_binding_capacity{};
  std::size_t required_live_root_state_capacity{};
  std::size_t required_live_root_facet_reference_capacity{};
  std::size_t required_live_root_point_id_reference_capacity{};
  std::size_t required_root_facet_replay_work_capacity{};
  std::size_t required_root_point_id_replay_work_capacity{};
  std::size_t required_target_facet_comparison_capacity{};
  std::size_t required_target_point_id_comparison_capacity{};
  std::size_t required_snapshot_facet_index_capacity{};
  std::size_t required_snapshot_point_id_index_capacity{};
  ExactCriticalCatalogTypedGammaJournalDecision source_journal_decision{
      ExactCriticalCatalogTypedGammaJournalDecision::not_certified};
  std::vector<ExactCriticalCatalogTypedGammaTargetRootBinding>
      target_root_bindings;
  bool root_overlay_candidate_space_size_certified{false};
  bool root_overlay_preflight_budget_sufficient{false};
  bool source_journal_is_external_and_not_retained{false};
  bool source_journal_budget_seam_certified{false};
  bool source_journal_fresh_replay_certified{false};
  bool root_sweep_started_only_after_complete_source_journal{false};
  bool every_history_batch_replayed_exactly_once{false};
  bool targets_resolved_against_frozen_pre_batch_snapshots{false};
  bool snapshot_indices_cover_all_active_facets{false};
  bool nontrivial_targets_match_complete_facet_families{false};
  bool omitted_targets_are_singletons_absent_from_active_facets{false};
  bool reduced_component_kinds_checked_after_geometric_binding{false};
  bool matched_roots_belong_to_target_history_group_prior_roots{false};
  bool every_strict_target_bound_exactly_once{false};
  bool persistent_root_ids_preserved{false};
  bool mutations_applied_after_complete_batch_resolution{false};
  bool final_replayed_roots_match_source_history{false};
  bool diagnostic_outcomes_have_no_bindings{false};
  bool critical_catalog_typed_gamma_root_overlay_certified{false};
  ExactCriticalCatalogTypedGammaRootOverlayCounters counters{};
  ExactCriticalCatalogTypedGammaRootOverlayDecision decision{
      ExactCriticalCatalogTypedGammaRootOverlayDecision::not_certified};
  ExactCriticalCatalogTypedGammaRootOverlayScope scope{
      ExactCriticalCatalogTypedGammaRootOverlayScope::unspecified};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaRootOverlayResult&,
      const ExactCriticalCatalogTypedGammaRootOverlayResult&) = default;
};

struct ExactCriticalCatalogTypedGammaRootOverlayVerification {
  bool requested_budget_certified{false};
  bool external_inputs_certified{false};
  bool derived_preflight_sizes_certified{false};
  bool source_journal_decision_certified{false};
  bool source_journal_fresh_replay_certified{false};
  bool target_root_bindings_certified{false};
  bool result_facts_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_critical_catalog_typed_gamma_root_overlay_decision_certified{
      false};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaRootOverlayVerification&,
      const ExactCriticalCatalogTypedGammaRootOverlayVerification&) =
      default;
};

[[nodiscard]] ExactCriticalCatalogTypedGammaRootOverlayResult
build_exact_critical_catalog_typed_gamma_root_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactCriticalCatalogTypedGammaRootOverlayBudget budget,
    const ExactCriticalCatalogTypedGammaJournalResult& source_journal);

// The external journal and observed overlay are both untrusted.  The source
// journal is first reconstructed geometrically by its 6.18 verifier; no
// observed binding, root ID, disposition or counter steers the fresh sweep.
[[nodiscard]] ExactCriticalCatalogTypedGammaRootOverlayVerification
verify_exact_critical_catalog_typed_gamma_root_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactCriticalCatalogTypedGammaRootOverlayBudget budget,
    const ExactCriticalCatalogTypedGammaJournalResult& source_journal,
    const ExactCriticalCatalogTypedGammaRootOverlayResult& result);

}  // namespace morsehgp3d::hierarchy
