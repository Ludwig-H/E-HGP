#pragma once

#include "morsehgp3d/hierarchy/direct_morse_forest_carrier_cut_replay_session.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_vertical_target_proposal_pipeline_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_vertical_target_proposal_pipeline_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_vertical_target_proposal_pipeline_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_morse_vertical_target_proposal_pipeline_mode =
        "certified_multiorder_group_local_historical_cut_proposal_pipeline";
inline constexpr std::string_view
    direct_morse_vertical_target_proposal_pipeline_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_morse_vertical_target_proposal_pipeline_proof_basis =
        "accepted_exact_direct_morse_forest_one_monotone_historical_session_"
        "per_referenced_target_order_group_local_k_facet_plan_then_one_10_5c_"
        "closure_then_compact_proposal_adapter_with_post_advance_atomic_"
        "publication_and_disjoint_query_tokens_v1";

// Nested budgets apply independently to every referenced target-order session
// or source atomic group.  Aggregate caps are exact over the group-local plans:
// a bounded preflight constructs and destroys one plan at a time, before any
// session is initialized or any scientific payload is published.
struct ExactDirectMorseVerticalTargetProposalPipelineBudget {
  ExactDirectMorseForestCarrierCutReplaySessionBudget session_budget{};
  ExactDirectMorseVerticalTargetFacetPlanBudget facet_plan_budget{};
  ExactDirectMorseForestCarrierCutClosureAdapterBudget closure_budget{};
  ExactDirectMorseVerticalTargetProposalAdapterBudget proposal_budget{};
  std::size_t maximum_source_batch_scan_count{};
  std::size_t maximum_source_atomic_group_scan_count{};
  std::size_t maximum_referenced_target_order_count{};
  std::size_t maximum_target_order_lookup_count{};
  std::size_t maximum_preflight_facet_plan_count{};
  std::size_t maximum_executed_facet_plan_count{};
  std::size_t maximum_replay_advance_count{};
  std::size_t maximum_closure_build_count{};
  std::size_t maximum_proposal_adapter_count{};
  std::size_t maximum_aggregate_representative_count{};
  std::size_t maximum_aggregate_projected_target_facet_reference_count{};
  std::size_t maximum_aggregate_distinct_target_facet_count{};
  std::size_t maximum_aggregate_retained_key_point_reference_count{};
  std::size_t maximum_aggregate_plan_logical_output_entry_count{};
  std::size_t maximum_aggregate_closure_terminal_summary_count{};
  std::size_t maximum_aggregate_proposal_count{};
  std::size_t maximum_session_audit_count{};
  std::size_t maximum_group_audit_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectMorseVerticalTargetProposalPipelineBudget&,
      const ExactDirectMorseVerticalTargetProposalPipelineBudget&) = default;
};

struct ExactDirectMorseVerticalTargetProposalPipelineSessionAudit {
  std::size_t session_audit_index{};
  std::size_t target_order{};
  std::size_t processed_group_count{};
  std::size_t final_committed_global_batch_prefix_count{};
  ExactDirectMorseForestCarrierCutReplaySessionCounters
      final_cumulative_counters{};
  ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision
      initialization_decision{
          ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
              not_certified};
  bool ready_before_first_advance{false};
  bool remained_unpoisoned{false};

  friend bool operator==(
      const ExactDirectMorseVerticalTargetProposalPipelineSessionAudit&,
      const ExactDirectMorseVerticalTargetProposalPipelineSessionAudit&) =
      default;
};

// This receipt contains scalar counts and stage decisions only.  It retains
// no facet key, incidence list, closure node, carrier entry, locator stamp or
// session/view reference.
struct ExactDirectMorseVerticalTargetProposalPipelineGroupAudit {
  std::size_t group_audit_index{};
  std::size_t source_atomic_group_index{};
  std::size_t source_batch_index{};
  std::size_t source_order{};
  std::size_t target_order{};
  exact::ExactLevel source_batch_squared_level{};
  std::uint64_t invocation_replay_token{};
  std::size_t committed_global_batch_prefix_count{};
  std::size_t proposal_offset{};
  std::size_t proposal_count{};
  std::size_t source_binding_scan_count{};
  std::size_t representative_count{};
  std::size_t projected_target_facet_reference_count{};
  std::size_t distinct_target_facet_count{};
  std::size_t retained_key_point_reference_count{};
  std::size_t plan_logical_output_entry_count{};
  ExactDirectMorseVerticalTargetFacetPlanCounters facet_plan_counters{};
  std::size_t closure_terminal_summary_count{};
  std::size_t closure_required_terminal_key_point_reference_count{};
  std::size_t closure_required_logical_output_entry_count{};
  std::size_t closure_required_memo_slot_count{};
  std::size_t closure_unresolved_terminal_count{};
  std::size_t closure_active_latent_terminal_count{};
  std::size_t closure_resolved_terminal_count{};
  ExactDirectSparseFacetDescentClosureCounters closure_counters{};
  ExactDirectMorseForestCarrierCutClosureAdapterAudit closure_audit{};
  std::size_t unresolved_proposal_count{};
  std::size_t resolved_proposal_count{};
  std::size_t adapter_required_carrier_entry_revalidation_count{};
  std::size_t adapter_required_logical_output_entry_count{};
  ExactDirectMorseVerticalTargetProposalAdapterCounters adapter_counters{};
  ExactDirectMorseVerticalTargetFacetPlanDecision facet_plan_decision{
      ExactDirectMorseVerticalTargetFacetPlanDecision::not_certified};
  ExactDirectMorseForestCarrierCutReplayAdvanceDecision advance_decision{
      ExactDirectMorseForestCarrierCutReplayAdvanceDecision::not_certified};
  ExactDirectMorseForestCarrierCutClosureAdapterDecision closure_decision{
      ExactDirectMorseForestCarrierCutClosureAdapterDecision::not_certified};
  ExactDirectMorseVerticalTargetProposalAdapterDecision adapter_decision{
      ExactDirectMorseVerticalTargetProposalAdapterDecision::not_certified};
  bool exact_cut_equals_source_batch_level{false};
  bool advance_post_stamp_certified_before_publication{false};
  bool plan_closure_and_locator_payload_destroyed{false};

  friend bool operator==(
      const ExactDirectMorseVerticalTargetProposalPipelineGroupAudit&,
      const ExactDirectMorseVerticalTargetProposalPipelineGroupAudit&) =
      default;
};

struct ExactDirectMorseVerticalTargetProposalPipelineCounters {
  std::size_t source_batch_scan_count{};
  std::size_t source_atomic_group_scan_count{};
  std::size_t target_order_lookup_count{};
  std::size_t referenced_target_order_count{};
  std::size_t initialized_session_count{};
  std::size_t preflight_facet_plan_count{};
  std::size_t executed_facet_plan_count{};
  std::size_t replay_advance_count{};
  std::size_t closure_build_count{};
  std::size_t proposal_adapter_count{};
  std::size_t representative_count{};
  std::size_t projected_target_facet_reference_count{};
  std::size_t distinct_target_facet_count{};
  std::size_t retained_key_point_reference_count{};
  std::size_t plan_logical_output_entry_count{};
  std::size_t closure_terminal_summary_count{};
  std::size_t closure_unresolved_terminal_count{};
  std::size_t closure_active_latent_terminal_count{};
  std::size_t closure_resolved_terminal_count{};
  std::size_t unresolved_proposal_count{};
  std::size_t resolved_proposal_count{};
  std::size_t equal_level_same_target_order_group_count{};

  friend bool operator==(
      const ExactDirectMorseVerticalTargetProposalPipelineCounters&,
      const ExactDirectMorseVerticalTargetProposalPipelineCounters&) =
      default;
};

enum class ExactDirectMorseVerticalTargetProposalPipelineDecision
    : std::uint8_t {
  not_certified,
  no_pipeline_source_forest_rejected,
  no_pipeline_point_namespace_rejected,
  no_pipeline_source_shape_rejected,
  no_pipeline_capacity_overflow,
  no_pipeline_budget_exhausted,
  no_pipeline_allocation_failed,
  no_pipeline_token_overflow,
  no_pipeline_facet_plan_rejected,
  no_pipeline_session_initialization_rejected,
  no_pipeline_replay_advance_rejected,
  no_pipeline_closure_rejected,
  no_pipeline_proposal_adapter_rejected,
  complete_empty_adjacent_group_set,
  complete_all_resolved_multiorder_target_proposals,
  complete_with_unresolved_multiorder_target_proposals,
};

enum class ExactDirectMorseVerticalTargetProposalPipelineScope
    : std::uint8_t {
  unspecified,
  all_source_orders_k_at_least_two_atomic_group_strict_arms_to_compact_adjacent_k_minus_one_target_proposals_only,
};

struct ExactDirectMorseVerticalTargetProposalPipelineResult {
  static constexpr std::string_view backend =
      direct_morse_vertical_target_proposal_pipeline_backend;
  static constexpr std::string_view profile =
      direct_morse_vertical_target_proposal_pipeline_profile;
  static constexpr std::string_view mode =
      direct_morse_vertical_target_proposal_pipeline_mode;
  static constexpr std::string_view public_status =
      direct_morse_vertical_target_proposal_pipeline_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_vertical_target_proposal_pipeline_proof_basis;

  std::uint32_t schema_version{
      direct_morse_vertical_target_proposal_pipeline_schema_version};
  ExactDirectMorseVerticalTargetProposalPipelineBudget requested_budget{};
  std::size_t point_count{};
  std::size_t effective_maximum_order{};
  std::uint64_t external_target_authority_id{};
  std::size_t required_referenced_target_order_count{};
  std::size_t required_group_count{};
  std::size_t required_representative_count{};
  std::size_t required_projected_target_facet_reference_count{};
  std::size_t required_distinct_target_facet_count{};
  std::size_t required_retained_key_point_reference_count{};
  std::size_t required_plan_logical_output_entry_count{};
  std::size_t required_closure_terminal_summary_count{};
  std::size_t required_proposal_count{};
  std::size_t required_logical_output_entry_count{};
  std::vector<ExactDirectMorseVerticalTargetProposal> proposals;
  std::vector<ExactDirectMorseVerticalTargetProposalPipelineSessionAudit>
      session_audits;
  std::vector<ExactDirectMorseVerticalTargetProposalPipelineGroupAudit>
      group_audits;
  ExactDirectMorseVerticalTargetProposalPipelineCounters counters{};
  std::optional<std::size_t> rejected_source_atomic_group_index;
  std::optional<std::size_t> rejected_target_order;
  ExactDirectMorseVerticalTargetFacetPlanDecision rejected_facet_plan_decision{
      ExactDirectMorseVerticalTargetFacetPlanDecision::not_certified};
  ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision
      rejected_session_initialization_decision{
          ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
              not_certified};
  ExactDirectMorseForestCarrierCutReplayAdvanceDecision
      rejected_advance_decision{
          ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
              not_certified};
  ExactDirectMorseForestCarrierCutClosureAdapterDecision
      rejected_closure_decision{
          ExactDirectMorseForestCarrierCutClosureAdapterDecision::
              not_certified};
  ExactDirectMorseVerticalTargetProposalAdapterDecision
      rejected_adapter_decision{
          ExactDirectMorseVerticalTargetProposalAdapterDecision::
              not_certified};
  bool source_forest_certified{false};
  bool lbvh_validated_for_cloud_and_point_count_matches{false};
  bool matching_canonical_point_namespace_required{true};
  bool forest_to_cloud_namespace_identity_certified{false};
  bool aggregate_budget_preflight_certified{false};
  bool one_session_per_referenced_target_order{false};
  bool source_groups_processed_in_batch_then_group_order{false};
  bool each_session_advanced_monotonically{false};
  bool query_tokens_follow_three_times_group_index_plus_one{false};
  bool query_tokens_disjoint_from_birth_and_union_domains{false};
  bool every_group_plan_certified{false};
  bool every_closure_summary_certified{false};
  bool every_group_adapter_certified{false};
  bool every_advance_post_stamp_certified_before_publication{false};
  bool output_preallocated_after_aggregate_preflight{false};
  bool proposals_sorted_unique_by_binding_index{false};
  bool source_binding_partition_proved_global_output_order{false};
  bool only_final_proposals_and_scalar_audits_retained{false};
  bool no_plan_closure_locator_or_session_reference_retained{false};
  bool no_partial_scientific_payload_published_on_failure{false};
  bool forest_relative_only{true};
  bool external_target_authority_replayed{false};
  bool vertical_maps_complete{false};
  bool global_facet_coface_incidence_cell_gamma_or_delaunay_materialized{
      false};
  bool higher_order_delaunay_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseVerticalTargetProposalPipelineDecision decision{
      ExactDirectMorseVerticalTargetProposalPipelineDecision::not_certified};
  ExactDirectMorseVerticalTargetProposalPipelineScope scope{
      ExactDirectMorseVerticalTargetProposalPipelineScope::unspecified};

  [[nodiscard]] bool certified_multiorder_target_proposals() const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectMorseVerticalTargetProposalPipelineResult&,
      const ExactDirectMorseVerticalTargetProposalPipelineResult&) = default;
};

[[nodiscard]] ExactDirectMorseVerticalTargetProposalPipelineResult
build_exact_direct_morse_vertical_target_proposal_pipeline(
    const ExactDirectMorseForestJournalResult& source_forest,
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectMorseVerticalTargetProposalPipelineBudget& budget);

}  // namespace morsehgp3d::hierarchy
