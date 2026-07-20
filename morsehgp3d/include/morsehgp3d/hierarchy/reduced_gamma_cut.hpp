#pragma once

#include "morsehgp3d/hierarchy/reduced_gamma_history.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace morsehgp3d::hierarchy {

enum class ExactReducedGammaCutBoundary : std::uint8_t {
  strict_open,
  closed,
};

// A cut replays only a prefix of the compact 6.14 journal. Every requested
// capacity covers that prefix replay and is checked atomically before an
// active-root payload map is constructed. A separate, always-bounded global
// source-shape audit precedes prefix selection and is intentionally outside
// this budget. Its scratch is bounded by three facet/coface sets of at most F,
// C and F labels, a scalar root-count map of at most 429 entries, and at most
// 6435 pending scalar group mutations.
struct ExactReducedGammaCutBudget {
  static constexpr std::size_t maximum_supported_batch_count = 6435U;
  static constexpr std::size_t maximum_supported_group_record_count = 6435U;
  static constexpr std::size_t maximum_supported_node_record_count = 3432U;
  static constexpr std::size_t
      maximum_supported_prior_root_reference_count = 3431U;
  static constexpr std::size_t
      maximum_supported_child_reference_count = 3431U;
  static constexpr std::size_t
      maximum_supported_newly_active_facet_count = 3432U;
  static constexpr std::size_t
      maximum_supported_equal_level_coface_count = 3432U;
  static constexpr std::size_t maximum_supported_delta_facet_count = 3432U;
  static constexpr std::size_t
      maximum_supported_delta_point_reference_count = 24024U;
  static constexpr std::size_t maximum_supported_active_root_count = 429U;
  static constexpr std::size_t
      maximum_supported_output_facet_reference_count = 3432U;
  static constexpr std::size_t
      maximum_supported_output_point_reference_count = 6006U;
  static constexpr std::size_t
      maximum_supported_facet_replay_work_count = 22084920U;
  static constexpr std::size_t
      maximum_supported_point_id_replay_work_count = 154594440U;
  static constexpr std::size_t
      maximum_supported_result_incidence_facet_check_count = 27456U;
  static constexpr std::size_t
      maximum_supported_result_incidence_point_id_work_count = 192192U;

  std::size_t maximum_batch_count{};
  std::size_t maximum_group_record_count{};
  std::size_t maximum_node_record_count{};
  std::size_t maximum_prior_root_reference_count{};
  std::size_t maximum_child_reference_count{};
  std::size_t maximum_newly_active_facet_count{};
  std::size_t maximum_equal_level_coface_count{};
  std::size_t maximum_delta_facet_count{};
  std::size_t maximum_delta_point_reference_count{};
  std::size_t maximum_active_root_count{};
  std::size_t maximum_output_facet_reference_count{};
  std::size_t maximum_output_point_reference_count{};
  std::size_t maximum_facet_replay_work_count{};
  std::size_t maximum_point_id_replay_work_count{};
  std::size_t maximum_result_incidence_facet_check_count{};
  std::size_t maximum_result_incidence_point_id_work_count{};

  friend bool operator==(
      const ExactReducedGammaCutBudget&,
      const ExactReducedGammaCutBudget&) = default;
};

enum class ExactReducedGammaCutDecision : std::uint8_t {
  not_certified,
  source_history_claims_or_structure_rejected,
  no_cut_preflight_budget_insufficient,
  complete_empty_terminal_order,
  complete_empty_prefix,
  complete_strict_journal_relative_reduced_gamma_cut,
  complete_closed_journal_relative_reduced_gamma_cut,
};

enum class ExactReducedGammaCutScope : std::uint8_t {
  unspecified,
  bounded_n14_k10_single_order_strict_or_closed_hgp_reduced_cut_from_certified_6_14_journal_only,
};

// The cursor is derived solely from the exact threshold and boundary.  A
// strict cut uses lower_bound and a closed cut uses upper_bound.  The first
// excluded level is diagnostic; callers never provide a prefix index.
struct ExactReducedGammaCutCursor {
  std::size_t activation_level_prefix_count{};
  std::size_t batch_prefix_count{};
  std::size_t group_record_prefix_count{};
  std::size_t node_record_prefix_count{};
  std::optional<exact::ExactLevel> first_excluded_squared_level;
  bool selected_by_exact_lower_bound{false};
  bool selected_by_exact_upper_bound{false};

  friend bool operator==(
      const ExactReducedGammaCutCursor&,
      const ExactReducedGammaCutCursor&) = default;
};

struct ExactReducedGammaCutCounters {
  std::size_t source_history_gate_check_count{};
  std::size_t global_structure_activation_level_count{};
  std::size_t global_structure_batch_metadata_count{};
  std::size_t global_structure_node_record_count{};
  std::size_t global_structure_group_record_count{};
  std::size_t global_structure_label_validation_count{};
  std::size_t global_structure_point_id_reference_validation_count{};
  std::size_t global_dry_batch_count{};
  std::size_t global_dry_group_record_count{};
  std::size_t global_dry_node_record_count{};
  std::size_t global_dry_prior_root_reference_count{};
  std::size_t global_dry_child_reference_count{};
  std::size_t global_dry_facet_state_work_count{};
  std::size_t exact_prefix_search_count{};
  std::size_t preflight_count{};
  std::size_t replayed_batch_count{};
  std::size_t replayed_group_record_count{};
  std::size_t replayed_node_record_count{};
  std::size_t replayed_prior_root_reference_count{};
  std::size_t replayed_child_reference_count{};
  std::size_t replayed_newly_active_facet_count{};
  std::size_t replayed_equal_level_coface_count{};
  std::size_t replayed_delta_facet_count{};
  std::size_t replayed_delta_point_reference_count{};
  std::size_t fully_redundant_group_count{};
  std::size_t frozen_root_snapshot_count{};
  std::size_t applied_root_mutation_count{};
  std::size_t peak_active_root_count{};
  std::size_t final_active_root_count{};
  std::size_t output_facet_reference_count{};
  std::size_t output_point_reference_count{};
  std::size_t facet_replay_work_count{};
  std::size_t point_id_replay_work_count{};
  std::size_t result_incidence_facet_check_count{};
  std::size_t result_incidence_point_id_work_count{};

  friend bool operator==(
      const ExactReducedGammaCutCounters&,
      const ExactReducedGammaCutCounters&) = default;
};

// This is an exact replay relative to the supplied 6.14 journal.  The input
// contract is an in-memory object returned by the 6.14 builder and separately
// accepted by its verifier, not an adversarially deserialized payload:
// cardinality caps cannot bound the limb count of a forged ExactLevel before
// exact lower/upper_bound. Because no cloud is accepted and neither the 6.14
// nor Gamma verifier is called, even a
// structurally coherent forged journal cannot be distinguished from a
// genuinely certified one.  Consequently this object is not a public-status
// decision, a fresh geometric certificate, durable persistence, a vertical
// map, a full-pi0 result, or a CUDA/G4 product.
struct ExactReducedGammaCut {
  static constexpr const char* proof_basis =
      "exact_certified_persistent_reduced_gamma_journal_prefix_cut_replay_v1";

  ExactReducedGammaCutBudget requested_budget{};
  std::size_t point_count{};
  std::size_t order{};
  exact::ExactLevel squared_level;
  ExactReducedGammaCutBoundary boundary{
      ExactReducedGammaCutBoundary::strict_open};
  ExactReducedGammaCutCursor cursor{};
  std::size_t required_batch_capacity{};
  std::size_t required_group_record_capacity{};
  std::size_t required_node_record_capacity{};
  std::size_t required_prior_root_reference_capacity{};
  std::size_t required_child_reference_capacity{};
  std::size_t required_newly_active_facet_capacity{};
  std::size_t required_equal_level_coface_capacity{};
  std::size_t required_delta_facet_capacity{};
  std::size_t required_delta_point_reference_capacity{};
  std::size_t required_active_root_capacity{};
  std::size_t required_output_facet_reference_capacity{};
  std::size_t required_output_point_reference_capacity{};
  std::size_t required_facet_replay_work_capacity{};
  std::size_t required_point_id_replay_work_capacity{};
  std::size_t required_result_incidence_facet_check_capacity{};
  std::size_t required_result_incidence_point_id_work_capacity{};
  std::vector<ExactPersistentReducedGammaActiveRoot> active_roots;
  bool source_history_claims_and_structure_accepted{false};
  bool source_history_certification_is_external_assumption{false};
  bool source_history_geometry_not_freshly_certified{false};
  bool coherent_forged_history_cannot_be_excluded_without_cloud{false};
  bool global_source_structure_audit_completed_before_prefix_selection{
      false};
  bool prefix_selected_from_exact_threshold_and_boundary{false};
  bool preflight_budget_sufficient{false};
  bool root_replay_started_after_successful_preflight{false};
  bool complete_batches_replayed_from_frozen_snapshots{false};
  bool coverage_deltas_applied_exactly{false};
  bool persistent_root_ids_preserved{false};
  bool active_roots_canonical_and_disjoint_by_facet{false};
  bool prefix_forest_accounting_certified{false};
  bool cursor_matches_replayed_prefix{false};
  bool terminal_order_complete_empty{false};
  bool empty_prefix_complete{false};
  bool journal_relative_cut_replay_certified{false};
  ExactReducedGammaCutCounters counters{};
  ExactReducedGammaCutDecision decision{
      ExactReducedGammaCutDecision::not_certified};
  ExactReducedGammaCutScope scope{
      ExactReducedGammaCutScope::unspecified};

  friend bool operator==(
      const ExactReducedGammaCut&,
      const ExactReducedGammaCut&) = default;
};

struct ExactReducedGammaCutVerification {
  bool requested_budget_certified{false};
  bool external_inputs_certified{false};
  bool source_history_gate_outcome_certified{false};
  bool derived_preflight_sizes_certified{false};
  bool cursor_certified{false};
  bool active_roots_certified{false};
  bool result_facts_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_journal_replay_certified{false};
  bool exact_journal_relative_reduced_gamma_cut_replay_decision_certified{
      false};

  friend bool operator==(
      const ExactReducedGammaCutVerification&,
      const ExactReducedGammaCutVerification&) = default;
};

[[nodiscard]] ExactReducedGammaCut build_exact_reduced_gamma_cut(
    const ExactPersistentReducedGammaOrderHistory& history,
    const exact::ExactLevel& squared_level,
    ExactReducedGammaCutBoundary boundary,
    ExactReducedGammaCutBudget budget);

// Freshly derives the expected result from the supplied history, threshold,
// boundary and budget.  No field of `cut` selects a prefix or replay branch.
// The verification is exact only relative to the conditionally trusted
// journal and deliberately does not call a 6.14 or Gamma verifier.
[[nodiscard]] ExactReducedGammaCutVerification
verify_exact_reduced_gamma_cut(
    const ExactPersistentReducedGammaOrderHistory& history,
    const exact::ExactLevel& squared_level,
    ExactReducedGammaCutBoundary boundary,
    ExactReducedGammaCutBudget budget,
    const ExactReducedGammaCut& cut);

}  // namespace morsehgp3d::hierarchy
