#pragma once

#include "morsehgp3d/hierarchy/direct_morse_forest_carrier_cut_closure_adapter.hpp"
#include "morsehgp3d/hierarchy/direct_morse_vertical_journal.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_vertical_target_proposal_adapter_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_vertical_target_proposal_adapter_backend =
        "reference_cpu";
inline constexpr std::string_view
    direct_morse_vertical_target_proposal_adapter_profile =
        "hgp_reduced";
inline constexpr std::string_view
    direct_morse_vertical_target_proposal_adapter_mode =
        "certified_group_local_k_to_k_minus_one_closure_summary_aggregation";
inline constexpr std::string_view
    direct_morse_vertical_target_proposal_adapter_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_morse_vertical_target_proposal_adapter_proof_basis =
        "certified_source_forest_borrow_identity_one_atomic_group_exact_cut_"
        "canonical_k_facet_deletions_to_distinct_k_minus_one_10_5c_"
        "summaries_and_unique_known_reduced_target_root_aggregation_v1";

// The plan is the only preparation needed before entering the target-order
// replay view.  It retains one source representative per distinct strict arm,
// all K deletion references, and only the distinct canonical (K-1)-keys.
struct ExactDirectMorseVerticalTargetFacetPlanBudget {
  std::size_t maximum_group_saddle_scan_count{};
  std::size_t maximum_group_arm_binding_scan_count{};
  std::size_t maximum_binding_sort_scratch_count{};
  std::size_t maximum_representative_binding_count{};
  std::size_t maximum_projected_target_facet_reference_count{};
  std::size_t maximum_distinct_target_facet_count{};
  std::size_t maximum_sort_comparison_count{};
  std::size_t maximum_target_facet_lookup_comparison_count{};
  std::size_t maximum_retained_key_point_reference_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectMorseVerticalTargetFacetPlanBudget&,
      const ExactDirectMorseVerticalTargetFacetPlanBudget&) = default;
};

struct ExactDirectMorseVerticalTargetFacetPlanRepresentative {
  std::size_t representative_index{};
  std::size_t representative_arm_root_binding_index{};
  ExactDirectSparseFacetKey source_strict_arm_key{};
  std::size_t target_facet_index_offset{};
  std::size_t target_facet_index_count{};

  friend bool operator==(
      const ExactDirectMorseVerticalTargetFacetPlanRepresentative&,
      const ExactDirectMorseVerticalTargetFacetPlanRepresentative&) =
      default;
};

struct ExactDirectMorseVerticalTargetFacetPlanCounters {
  std::size_t group_saddle_scan_count{};
  std::size_t group_arm_binding_scan_count{};
  std::size_t generated_target_facet_reference_count{};
  std::size_t sort_comparison_count{};
  std::size_t target_facet_lookup_comparison_count{};

  friend bool operator==(
      const ExactDirectMorseVerticalTargetFacetPlanCounters&,
      const ExactDirectMorseVerticalTargetFacetPlanCounters&) = default;
};

enum class ExactDirectMorseVerticalTargetFacetPlanDecision
    : std::uint8_t {
  not_certified,
  no_plan_source_forest_rejected,
  no_plan_group_rejected,
  no_plan_capacity_overflow,
  no_plan_budget_exhausted,
  no_plan_allocation_failed,
  complete_group_local_canonical_target_facet_plan,
};

enum class ExactDirectMorseVerticalTargetFacetPlanScope : std::uint8_t {
  unspecified,
  one_source_atomic_group_k_keys_to_canonical_distinct_k_minus_one_deletions_only,
};

struct ExactDirectMorseVerticalTargetFacetPlanResult {
  std::uint32_t schema_version{
      direct_morse_vertical_target_proposal_adapter_schema_version};
  ExactDirectMorseVerticalTargetFacetPlanBudget requested_budget{};
  std::size_t point_count{};
  std::size_t source_atomic_group_index{};
  std::size_t source_batch_index{};
  std::size_t source_order{};
  std::size_t target_order{};
  exact::ExactLevel source_batch_squared_level{};
  std::size_t required_representative_binding_count{};
  std::size_t required_projected_target_facet_reference_count{};
  std::size_t required_distinct_target_facet_count{};
  std::size_t required_retained_key_point_reference_count{};
  std::size_t required_logical_output_entry_count{};
  std::vector<ExactDirectMorseVerticalTargetFacetPlanRepresentative>
      representatives;
  // For representative r, its K entries occupy the recorded slice and follow
  // source_strict_arm_key deletion order.  Values index the distinct key list.
  std::vector<std::size_t> target_facet_indices;
  std::vector<ExactDirectSparseFacetKey>
      canonical_distinct_target_facet_keys;
  // A permutation of representatives, strictly increasing by binding index.
  std::vector<std::size_t> representative_indices_in_binding_order;
  ExactDirectMorseVerticalTargetFacetPlanCounters counters{};
  bool source_forest_certified{false};
  bool source_group_ranges_revalidated{false};
  bool source_representatives_are_smallest_binding_indices{false};
  bool source_representatives_sorted_unique_by_full_key{false};
  bool every_source_key_projected_to_all_k_deletions{false};
  bool target_facets_canonical_distinct_and_sorted{false};
  bool target_facet_mapping_total{false};
  bool output_preallocated_after_budget_preflight{false};
  bool no_locator_or_closure_graph_consumed{true};
  bool no_partial_payload_published_on_failure{false};
  bool forest_relative_only{true};
  bool global_facet_coface_incidence_cell_gamma_or_delaunay_materialized{
      false};
  bool public_status_claimed{false};
  ExactDirectMorseVerticalTargetFacetPlanDecision decision{
      ExactDirectMorseVerticalTargetFacetPlanDecision::not_certified};
  ExactDirectMorseVerticalTargetFacetPlanScope scope{
      ExactDirectMorseVerticalTargetFacetPlanScope::unspecified};

  [[nodiscard]] bool certified_group_local_target_facet_plan()
      const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectMorseVerticalTargetFacetPlanResult&,
      const ExactDirectMorseVerticalTargetFacetPlanResult&) = default;
};

[[nodiscard]] ExactDirectMorseVerticalTargetFacetPlanResult
build_exact_direct_morse_vertical_target_facet_plan(
    const ExactDirectMorseForestJournalResult& source_forest,
    std::size_t source_atomic_group_index,
    const ExactDirectMorseVerticalTargetFacetPlanBudget& budget);

struct ExactDirectMorseVerticalTargetProposalAdapterBudget {
  std::size_t maximum_source_saddle_revalidation_count{};
  std::size_t maximum_source_binding_revalidation_count{};
  std::size_t maximum_source_key_lookup_comparison_count{};
  std::size_t maximum_closure_summary_scan_count{};
  std::size_t maximum_positive_terminal_probe_count{};
  std::size_t maximum_positive_terminal_probe_slot_visit_count{};
  std::size_t maximum_positive_terminal_probe_parent_hop_count{};
  ExactDirectSparsePositiveFacetProbeBudget positive_terminal_probe_budget{};
  std::size_t maximum_carrier_entry_revalidation_count{};
  std::size_t maximum_target_node_lookup_count{};
  std::size_t maximum_exact_level_comparison_count{};
  std::size_t maximum_single_exact_level_integer_bit_count{};
  std::size_t maximum_proposal_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectMorseVerticalTargetProposalAdapterBudget&,
      const ExactDirectMorseVerticalTargetProposalAdapterBudget&) =
      default;
};

struct ExactDirectMorseVerticalTargetProposalAdapterCounters {
  std::size_t source_saddle_revalidation_count{};
  std::size_t source_binding_revalidation_count{};
  std::size_t source_key_lookup_comparison_count{};
  std::size_t closure_summary_scan_count{};
  std::size_t projected_target_facet_aggregation_count{};
  std::size_t positive_terminal_probe_count{};
  std::size_t positive_terminal_probe_slot_visit_count{};
  std::size_t positive_terminal_probe_parent_hop_count{};
  std::size_t positive_terminal_probe_full_key_comparison_count{};
  std::size_t carrier_entry_revalidation_count{};
  std::size_t target_node_lookup_count{};
  std::size_t exact_level_comparison_count{};
  std::size_t maximum_observed_exact_level_integer_bit_count{};
  std::size_t unresolved_proposal_count{};
  std::size_t resolved_proposal_count{};

  friend bool operator==(
      const ExactDirectMorseVerticalTargetProposalAdapterCounters&,
      const ExactDirectMorseVerticalTargetProposalAdapterCounters&) =
      default;
};

struct ExactDirectMorseVerticalTargetProposalAdapterAudit {
  bool view_epoch_checked_at_entry{false};
  bool source_forest_certified{false};
  bool source_forest_borrow_identity_matches_session{false};
  bool plan_certified{false};
  bool plan_revalidated_against_exact_source_group{false};
  bool point_count_matches{false};
  bool adjacent_target_order_matches{false};
  bool exact_cut_matches_source_batch{false};
  bool closure_summary_certified{false};
  bool closure_snapshot_matches_live_view{false};
  bool closure_sources_biject_canonical_target_facets{false};
  bool every_positive_terminal_key_binding_reprobed{false};
  bool output_preallocated_after_budget_preflight{false};
  bool every_projected_target_facet_aggregated{false};
  bool every_known_resolved_root_consistent_per_source_binding{false};
  bool unresolved_or_latent_maps_only_to_unresolved{false};
  bool all_resolved_unique_root_maps_to_target_seed{false};
  bool common_invocation_replay_token_used{false};
  bool proposals_sorted_unique_by_binding_index{false};
  bool entry_and_post_locator_stamps_equal{false};
  bool plan_or_summary_payload_retained{false};
  bool no_partial_payload_published_on_failure{false};
  bool locator_state_mutated{false};
  bool session_poisoned{false};
  bool locator_reference_exposed{false};
  bool closure_graph_or_projection_persisted{false};
  bool locator_batch_committed{false};
  bool global_facet_coface_or_incidence_structure_materialized{false};
  bool gamma_cells_or_higher_order_delaunay_materialized{false};
  bool external_vertical_authority_replayed{false};
  bool vertical_maps_complete{false};
  bool forest_relative_only{true};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactDirectMorseVerticalTargetProposalAdapterAudit&,
      const ExactDirectMorseVerticalTargetProposalAdapterAudit&) =
      default;
};

enum class ExactDirectMorseVerticalTargetProposalAdapterDecision
    : std::uint8_t {
  not_certified,
  no_adapter_session_not_ready,
  no_adapter_session_poisoned,
  no_adapter_stale_view_epoch,
  no_adapter_locator_snapshot_changed,
  no_adapter_source_forest_rejected,
  no_adapter_source_forest_identity_mismatch,
  no_adapter_plan_rejected,
  no_adapter_source_group_mismatch,
  no_adapter_order_or_exact_cut_mismatch,
  no_adapter_closure_summary_rejected,
  no_adapter_closure_snapshot_mismatch,
  no_adapter_summary_bijection_rejected,
  no_adapter_known_target_root_contradiction,
  no_adapter_target_seed_rejected,
  no_adapter_capacity_overflow,
  no_adapter_budget_exhausted,
  no_adapter_allocation_failed,
  complete_all_resolved_group_local_vertical_target_proposals,
  complete_with_unresolved_group_local_vertical_target_proposals,
};

enum class ExactDirectMorseVerticalTargetProposalAdapterScope
    : std::uint8_t {
  unspecified,
  one_live_target_order_cut_one_source_atomic_group_compact_vertical_target_proposals_only,
};

struct ExactDirectMorseVerticalTargetProposalAdapterResult {
  static constexpr std::string_view backend =
      direct_morse_vertical_target_proposal_adapter_backend;
  static constexpr std::string_view profile =
      direct_morse_vertical_target_proposal_adapter_profile;
  static constexpr std::string_view mode =
      direct_morse_vertical_target_proposal_adapter_mode;
  static constexpr std::string_view public_status =
      direct_morse_vertical_target_proposal_adapter_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_vertical_target_proposal_adapter_proof_basis;

  std::uint32_t schema_version{
      direct_morse_vertical_target_proposal_adapter_schema_version};
  ExactDirectMorseVerticalTargetProposalAdapterBudget requested_budget{};
  std::size_t point_count{};
  std::size_t source_atomic_group_index{};
  std::size_t source_batch_index{};
  std::size_t source_order{};
  std::size_t target_order{};
  exact::ExactLevel source_batch_squared_level{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp
      locator_snapshot_stamp{};
  std::uint64_t external_target_authority_id{};
  std::uint64_t invocation_replay_token{};
  std::size_t input_representative_count{};
  std::size_t input_distinct_target_facet_count{};
  std::size_t input_projected_target_facet_reference_count{};
  std::size_t required_carrier_entry_revalidation_count{};
  std::size_t required_logical_output_entry_count{};
  // The only scientific payload retained after aggregation.  It is sorted
  // strictly by representative binding index and can be passed directly to
  // build_exact_direct_morse_vertical_journal.
  std::vector<ExactDirectMorseVerticalTargetProposal> proposals;
  ExactDirectMorseVerticalTargetProposalAdapterCounters counters{};
  ExactDirectMorseVerticalTargetProposalAdapterAudit audit{};
  ExactDirectMorseVerticalTargetProposalAdapterDecision decision{
      ExactDirectMorseVerticalTargetProposalAdapterDecision::not_certified};
  ExactDirectMorseVerticalTargetProposalAdapterScope scope{
      ExactDirectMorseVerticalTargetProposalAdapterScope::unspecified};

  [[nodiscard]] bool certified_group_local_vertical_target_proposals()
      const noexcept;
  [[nodiscard]] bool certified_atomic_rejection() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectMorseVerticalTargetProposalAdapterResult&,
      const ExactDirectMorseVerticalTargetProposalAdapterResult&) =
      default;
};

}  // namespace morsehgp3d::hierarchy
