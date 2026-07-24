#pragma once

#include "morsehgp3d/hierarchy/direct_morse_forest_journal.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t direct_morse_vertical_journal_schema_version =
    1U;
inline constexpr std::string_view direct_morse_vertical_journal_backend =
    "reference_cpu";
inline constexpr std::string_view direct_morse_vertical_journal_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_morse_vertical_journal_mode =
    "certified";
inline constexpr std::string_view
    direct_morse_vertical_journal_deployment_status = "architecture_only";
inline constexpr std::string_view
    direct_morse_vertical_journal_refinement_status =
        "conditional_vertical_candidate";
inline constexpr std::string_view direct_morse_vertical_journal_public_status =
    "not_claimed";
inline constexpr std::string_view direct_morse_vertical_journal_proof_basis =
    "conditional_external_target_seed_closed_adjacent_order_forest_"
    "normalization_group_local_distinct_arm_labels_qr_checkpoint_"
    "propagation_and_bounded_multiorder_trace_v1";

struct ExactDirectMorseVerticalConfig {
  std::uint64_t external_target_authority_id{};

  friend bool operator==(
      const ExactDirectMorseVerticalConfig&,
      const ExactDirectMorseVerticalConfig&) = default;
};

struct ExactDirectMorseVerticalBudget {
  std::size_t maximum_forest_node_scan_count{};
  std::size_t maximum_child_reference_scan_count{};
  std::size_t maximum_birth_record_scan_count{};
  std::size_t maximum_batch_scan_count{};
  std::size_t maximum_atomic_group_scan_count{};
  std::size_t maximum_saddle_scan_count{};
  std::size_t maximum_arm_binding_scan_count{};
  std::size_t maximum_proposal_count{};
  std::size_t maximum_label_resolution_count{};
  std::size_t maximum_group_check_count{};
  std::size_t maximum_checkpoint_count{};
  std::size_t maximum_adjacent_family_count{};
  std::size_t maximum_group_sort_scratch_count{};
  std::size_t maximum_group_sort_comparison_count{};
  std::size_t maximum_target_parent_hop_count{};
  std::size_t maximum_exact_level_comparison_count{};
  std::size_t maximum_single_exact_level_integer_bit_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectMorseVerticalBudget&,
      const ExactDirectMorseVerticalBudget&) = default;
};

enum class ExactDirectMorseVerticalProposalDisposition : std::uint8_t {
  unresolved,
  resolved_target_seed,
};

// The binding index addresses the smallest binding carrying one distinct
// strict_arm_key inside its atomic source group.  No facet key is copied.
struct ExactDirectMorseVerticalTargetProposal {
  std::size_t representative_arm_root_binding_index{};
  std::optional<ExactDirectMorseForestNodeId> target_seed_node_id;
  std::uint64_t replay_token{};
  ExactDirectMorseVerticalProposalDisposition disposition{
      ExactDirectMorseVerticalProposalDisposition::unresolved};

  friend bool operator==(
      const ExactDirectMorseVerticalTargetProposal&,
      const ExactDirectMorseVerticalTargetProposal&) = default;
};

enum class ExactDirectMorseVerticalLabelDisposition : std::uint8_t {
  missing,
  unresolved,
  resolved_closed_target_root,
};

struct ExactDirectMorseVerticalLabelResolution {
  std::size_t label_resolution_index{};
  std::size_t atomic_group_index{};
  std::size_t representative_arm_root_binding_index{};
  std::optional<std::size_t> source_proposal_index;
  std::optional<ExactDirectMorseForestNodeId> closed_target_root_node_id;
  ExactDirectMorseVerticalLabelDisposition disposition{
      ExactDirectMorseVerticalLabelDisposition::missing};

  friend bool operator==(
      const ExactDirectMorseVerticalLabelResolution&,
      const ExactDirectMorseVerticalLabelResolution&) = default;
};

enum class ExactDirectMorseVerticalCheckpointKind : std::uint8_t {
  reduced_birth_anchor,
  continuation_propagation,
  late_continuation_anchor,
  multifusion_propagation,
};

struct ExactDirectMorseVerticalCheckpoint {
  std::size_t checkpoint_index{};
  std::size_t atomic_group_index{};
  ExactDirectMorseForestNodeId source_root_node_id{};
  ExactDirectMorseForestNodeId closed_target_root_node_id{};
  ExactDirectMorseVerticalCheckpointKind kind{
      ExactDirectMorseVerticalCheckpointKind::reduced_birth_anchor};
  bool complete_relative_to_supplied_proposals{false};

  friend bool operator==(
      const ExactDirectMorseVerticalCheckpoint&,
      const ExactDirectMorseVerticalCheckpoint&) = default;
};

enum class ExactDirectMorseVerticalGroupDisposition : std::uint8_t {
  partial_reduced_birth,
  complete_reduced_birth,
  partial_continuation,
  complete_continuation,
  partial_multifusion,
  complete_multifusion,
};

struct ExactDirectMorseVerticalGroupCheck {
  std::size_t group_check_index{};
  std::size_t atomic_group_index{};
  std::size_t source_batch_index{};
  std::size_t label_resolution_offset{};
  std::size_t label_resolution_count{};
  std::size_t expected_elementary_group_square_count{};
  std::size_t checked_elementary_group_square_count{};
  std::size_t unresolved_elementary_group_square_count{};
  std::optional<std::size_t> checkpoint_index;
  ExactDirectMorseVerticalGroupDisposition disposition{
      ExactDirectMorseVerticalGroupDisposition::partial_reduced_birth};

  friend bool operator==(
      const ExactDirectMorseVerticalGroupCheck&,
      const ExactDirectMorseVerticalGroupCheck&) = default;
};

struct ExactDirectMorseVerticalAdjacentFamily {
  std::size_t family_index{};
  std::size_t source_order{};
  std::size_t target_order{};
  std::size_t group_check_offset{};
  std::size_t group_check_count{};
  std::size_t label_resolution_offset{};
  std::size_t label_resolution_count{};
  std::size_t checkpoint_offset{};
  std::size_t checkpoint_count{};
  std::size_t source_reduced_node_count{};
  std::size_t omitted_isolated_source_birth_count{};
  bool complete_relative_to_supplied_proposals{false};

  friend bool operator==(
      const ExactDirectMorseVerticalAdjacentFamily&,
      const ExactDirectMorseVerticalAdjacentFamily&) = default;
};

struct ExactDirectMorseVerticalCounters {
  std::size_t expected_label_count{};
  std::size_t missing_label_count{};
  std::size_t unresolved_label_count{};
  std::size_t resolved_label_count{};
  std::size_t complete_group_count{};
  std::size_t partial_group_count{};
  std::size_t expected_elementary_group_square_count{};
  std::size_t checked_elementary_group_square_count{};
  std::size_t unresolved_elementary_group_square_count{};
  std::size_t checkpoint_count{};
  std::size_t late_checkpoint_count{};
  std::size_t group_sort_comparison_count{};
  std::size_t target_parent_hop_count{};
  std::size_t exact_level_comparison_count{};
  std::size_t maximum_observed_exact_level_integer_bit_count{};

  friend bool operator==(
      const ExactDirectMorseVerticalCounters&,
      const ExactDirectMorseVerticalCounters&) = default;
};

enum class ExactDirectMorseVerticalDecision : std::uint8_t {
  not_certified,
  no_vertical_capacity_overflow,
  no_vertical_budget_exhausted,
  no_vertical_allocation_failed,
  no_vertical_source_forest_rejected,
  no_vertical_forest_shape_rejected,
  no_vertical_proposal_partition_rejected,
  no_vertical_target_rejected,
  no_vertical_relative_target_conflict,
  complete_conditional_partial_vertical_journal,
  complete_conditional_total_relative_vertical_journal,
};

enum class ExactDirectMorseVerticalScope : std::uint8_t {
  unspecified,
  adjacent_reduced_group_labels_external_target_candidates_and_forest_propagation_only,
};

struct ExactDirectMorseVerticalJournalResult {
  static constexpr std::string_view backend =
      direct_morse_vertical_journal_backend;
  static constexpr std::string_view profile =
      direct_morse_vertical_journal_profile;
  static constexpr std::string_view mode = direct_morse_vertical_journal_mode;
  static constexpr std::string_view deployment_status =
      direct_morse_vertical_journal_deployment_status;
  static constexpr std::string_view refinement_status =
      direct_morse_vertical_journal_refinement_status;
  static constexpr std::string_view public_status =
      direct_morse_vertical_journal_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_vertical_journal_proof_basis;

  std::uint32_t schema_version{direct_morse_vertical_journal_schema_version};
  ExactDirectMorseVerticalConfig config{};
  ExactDirectMorseVerticalBudget requested_budget{};
  std::size_t point_count{};
  std::size_t effective_maximum_order{};
  std::vector<ExactDirectMorseVerticalAdjacentFamily> adjacent_families;
  std::vector<ExactDirectMorseVerticalLabelResolution> label_resolutions;
  std::vector<ExactDirectMorseVerticalGroupCheck> group_checks;
  std::vector<ExactDirectMorseVerticalCheckpoint> checkpoints;
  std::size_t logical_output_entry_count{};
  ExactDirectMorseVerticalCounters counters{};
  bool source_forest_shape_replayed{false};
  bool conditional_on_caller_fresh_source_forest_replay{true};
  bool budget_preflight_certified{false};
  bool representative_labels_reconstructed_without_key_copy{false};
  bool missing_and_unresolved_labels_distinguished{false};
  bool closed_target_roots_normalized_at_group_level{false};
  bool all_group_conflicts_rejected_atomically{false};
  bool elementary_group_square_partition_closed{false};
  bool higher_order_isolated_births_have_no_source_node{false};
  bool missing_target_never_classified_as_isolated{false};
  bool no_partial_scientific_payload_published_on_failure{false};
  bool external_target_authority_replayed{false};
  bool global_morse_obligation_replayed{false};
  bool all_naturality_squares_replayed{false};
  bool vertical_maps_complete{false};
  bool gamma_cells_or_global_cofaces_materialized{false};
  bool higher_order_delaunay_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseVerticalDecision decision{
      ExactDirectMorseVerticalDecision::not_certified};
  ExactDirectMorseVerticalScope scope{
      ExactDirectMorseVerticalScope::unspecified};

  [[nodiscard]] bool certified_conditional_vertical_candidate()
      const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectMorseVerticalJournalResult&,
      const ExactDirectMorseVerticalJournalResult&) = default;
};

struct ExactDirectMorseVerticalVerification {
  bool observed_storage_within_budget{false};
  bool expected_journal_freshly_reconstructed{false};
  bool observed_recursively_equal{false};
  bool source_forest_shape_replayed{false};
  bool external_target_authority_replayed{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectMorseVerticalVerification&,
      const ExactDirectMorseVerticalVerification&) = default;
};

[[nodiscard]] ExactDirectMorseVerticalJournalResult
build_exact_direct_morse_vertical_journal(
    const ExactDirectMorseForestJournalResult& source_forest,
    std::span<const ExactDirectMorseVerticalTargetProposal> proposals,
    const ExactDirectMorseVerticalBudget& budget,
    const ExactDirectMorseVerticalConfig& config);

[[nodiscard]] ExactDirectMorseVerticalVerification
verify_exact_direct_morse_vertical_journal(
    const ExactDirectMorseForestJournalResult& source_forest,
    std::span<const ExactDirectMorseVerticalTargetProposal> proposals,
    const ExactDirectMorseVerticalBudget& trusted_budget,
    const ExactDirectMorseVerticalConfig& config,
    const ExactDirectMorseVerticalJournalResult& observed);

struct ExactDirectMorseVerticalTraceBudget {
  std::size_t maximum_adjacent_step_count{};
  std::size_t maximum_parent_hop_count{};
  std::size_t maximum_checkpoint_scan_count{};

  friend bool operator==(
      const ExactDirectMorseVerticalTraceBudget&,
      const ExactDirectMorseVerticalTraceBudget&) = default;
};

enum class ExactDirectMorseVerticalTraceDisposition : std::uint8_t {
  not_traced,
  complete_relative_trace,
  partial_relative_trace,
  unresolved_missing_checkpoint,
  budget_exhausted,
  invalid_query,
};

struct ExactDirectMorseVerticalTraceStep {
  std::size_t step_index{};
  std::size_t source_order{};
  std::size_t target_order{};
  ExactDirectMorseForestNodeId source_root_node_id{};
  ExactDirectMorseForestNodeId target_root_node_id{};
  std::size_t checkpoint_index{};
  bool checkpoint_complete_relative_to_supplied_proposals{false};

  friend bool operator==(
      const ExactDirectMorseVerticalTraceStep&,
      const ExactDirectMorseVerticalTraceStep&) = default;
};

struct ExactDirectMorseVerticalTraceResult {
  std::size_t requested_source_order{};
  std::size_t requested_target_order{};
  exact::ExactLevel at_squared_level{};
  std::vector<ExactDirectMorseVerticalTraceStep> steps;
  std::size_t parent_hop_count{};
  std::size_t checkpoint_scan_count{};
  bool local_node_ids_only{true};
  bool public_vertical_map_claimed{false};
  ExactDirectMorseVerticalTraceDisposition disposition{
      ExactDirectMorseVerticalTraceDisposition::not_traced};

  friend bool operator==(
      const ExactDirectMorseVerticalTraceResult&,
      const ExactDirectMorseVerticalTraceResult&) = default;
};

[[nodiscard]] ExactDirectMorseVerticalTraceResult
trace_exact_direct_morse_vertical_component(
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseVerticalJournalResult& vertical_journal,
    ExactDirectMorseForestNodeId source_node_id,
    const exact::ExactLevel& at_squared_level,
    std::size_t target_order,
    const ExactDirectMorseVerticalTraceBudget& budget);

}  // namespace morsehgp3d::hierarchy
