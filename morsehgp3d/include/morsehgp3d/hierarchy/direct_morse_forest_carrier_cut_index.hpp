#pragma once

#include "morsehgp3d/hierarchy/direct_morse_forest_journal.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_forest_carrier_cut_index_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_forest_carrier_cut_index_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_forest_carrier_cut_index_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_morse_forest_carrier_cut_index_mode =
        "certified_forest_relative_closed_exact_cut_replay";
inline constexpr std::string_view
    direct_morse_forest_carrier_cut_index_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    direct_morse_forest_carrier_cut_index_refinement_status =
        "forest_relative_prerequisite_only";
inline constexpr std::string_view
    direct_morse_forest_carrier_cut_index_public_status = "not_claimed";
inline constexpr std::string_view
    direct_morse_forest_carrier_cut_index_proof_basis =
        "accepted_self_contained_exact_direct_morse_forest_journal_"
        "target_order_carrier_activation_atomic_frozen_group_union_then_"
        "birth_replay_through_closed_exact_cut_only_v1";

// Every source arena and every replay scratch arena is covered explicitly.
// The component and node-state caps bound the dense forest-local state. No
// geometric cell, coface, Gamma table or Delaunay object is constructed.
struct ExactDirectMorseForestCarrierCutIndexBudget {
  std::size_t maximum_forest_birth_record_scan_count{};
  std::size_t maximum_forest_node_scan_count{};
  std::size_t maximum_forest_batch_scan_count{};
  std::size_t maximum_forest_atomic_group_scan_count{};
  std::size_t maximum_forest_saddle_scan_count{};
  std::size_t maximum_forest_arm_binding_scan_count{};
  std::size_t maximum_forest_child_reference_scan_count{};
  std::size_t maximum_forest_final_root_scan_count{};
  std::size_t maximum_component_state_count{};
  std::size_t maximum_node_marker_state_count{};
  std::size_t maximum_index_entry_count{};
  std::size_t maximum_group_carrier_scratch_count{};
  std::size_t maximum_group_prior_root_scratch_count{};
  std::size_t maximum_parent_hop_count{};
  std::size_t maximum_exact_level_comparison_count{};
  std::size_t maximum_single_exact_level_integer_bit_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectMorseForestCarrierCutIndexBudget&,
      const ExactDirectMorseForestCarrierCutIndexBudget&) = default;
};

enum class ExactDirectMorseForestCarrierCutDisposition : std::uint8_t {
  inactive_at_closed_cut,
  active_latent_without_reduced_root,
  resolved_reduced_root,
};

struct ExactDirectMorseForestCarrierCutEntry {
  std::size_t entry_index{};
  ExactDirectSparseComponentHandle component_handle{};
  std::size_t birth_record_index{};
  std::optional<ExactDirectMorseForestNodeId> reduced_root_node_id;
  ExactDirectMorseForestCarrierCutDisposition disposition{
      ExactDirectMorseForestCarrierCutDisposition::inactive_at_closed_cut};

  friend bool operator==(
      const ExactDirectMorseForestCarrierCutEntry&,
      const ExactDirectMorseForestCarrierCutEntry&) = default;
};

struct ExactDirectMorseForestCarrierCutIndexCounters {
  std::size_t forest_birth_record_scan_count{};
  std::size_t forest_node_scan_count{};
  std::size_t forest_batch_scan_count{};
  std::size_t replayed_atomic_group_count{};
  std::size_t replayed_saddle_count{};
  std::size_t replayed_arm_binding_count{};
  std::size_t replayed_child_reference_count{};
  std::size_t forest_final_root_scan_count{};
  std::size_t component_state_count{};
  std::size_t node_marker_state_count{};
  std::size_t target_carrier_count{};
  std::size_t replayed_target_batch_count{};
  std::size_t unique_group_carrier_count{};
  std::size_t unique_group_prior_root_count{};
  std::size_t maximum_group_carrier_scratch_count{};
  std::size_t maximum_group_prior_root_scratch_count{};
  std::size_t carrier_union_count{};
  std::size_t parent_hop_count{};
  std::size_t exact_level_comparison_count{};
  std::size_t maximum_observed_exact_level_integer_bit_count{};
  std::size_t inactive_carrier_count{};
  std::size_t active_latent_carrier_count{};
  std::size_t resolved_reduced_root_carrier_count{};

  friend bool operator==(
      const ExactDirectMorseForestCarrierCutIndexCounters&,
      const ExactDirectMorseForestCarrierCutIndexCounters&) = default;
};

enum class ExactDirectMorseForestCarrierCutIndexDecision : std::uint8_t {
  not_certified,
  no_index_capacity_overflow,
  no_index_allocation_failed,
  no_index_budget_exhausted,
  no_index_source_forest_rejected,
  no_index_invalid_target_order,
  no_index_invalid_closed_squared_level,
  no_index_replay_contradiction,
  complete_certified_forest_relative_closed_cut_index,
};

enum class ExactDirectMorseForestCarrierCutIndexScope : std::uint8_t {
  unspecified,
  one_target_order_forest_carriers_to_optional_reduced_roots_at_one_closed_exact_cut_only,
};

struct ExactDirectMorseForestCarrierCutIndexResult {
  static constexpr std::string_view backend =
      direct_morse_forest_carrier_cut_index_backend;
  static constexpr std::string_view profile =
      direct_morse_forest_carrier_cut_index_profile;
  static constexpr std::string_view mode =
      direct_morse_forest_carrier_cut_index_mode;
  static constexpr std::string_view deployment_status =
      direct_morse_forest_carrier_cut_index_deployment_status;
  static constexpr std::string_view refinement_status =
      direct_morse_forest_carrier_cut_index_refinement_status;
  static constexpr std::string_view public_status =
      direct_morse_forest_carrier_cut_index_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_forest_carrier_cut_index_proof_basis;

  std::uint32_t schema_version{
      direct_morse_forest_carrier_cut_index_schema_version};
  ExactDirectMorseForestCarrierCutIndexBudget requested_budget{};
  std::size_t point_count{};
  std::size_t effective_maximum_order{};
  std::size_t target_order{};
  exact::ExactLevel closed_squared_level{};
  std::vector<ExactDirectMorseForestCarrierCutEntry> entries;
  std::size_t logical_output_entry_count{};
  ExactDirectMorseForestCarrierCutIndexCounters counters{};

  bool source_forest_certified_outcome_accepted{false};
  bool conditional_on_caller_fresh_source_forest_replay{true};
  bool budget_preflight_certified{false};
  bool target_order_birth_partition_reconstructed{false};
  bool target_batches_replayed_in_exact_level_order{false};
  bool frozen_carrier_groups_reconstructed{false};
  bool group_roots_resolved_before_current_level_births{false};
  bool unions_then_births_replayed_atomically{false};
  bool strict_pre_and_closed_post_counts_replayed{false};
  bool inactive_latent_and_resolved_carriers_distinguished{false};
  bool entries_sorted_unique_by_component_handle{false};
  bool no_partial_scientific_payload_published_on_failure{false};
  bool forest_relative_only{true};
  bool external_locator_authority_replayed{false};
  bool original_geometry_replayed{false};
  bool global_morse_obligation_replayed{false};
  bool vertical_naturality_replayed{false};
  bool gamma_cells_or_global_cofaces_materialized{false};
  bool higher_order_delaunay_materialized{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseForestCarrierCutIndexDecision decision{
      ExactDirectMorseForestCarrierCutIndexDecision::not_certified};
  ExactDirectMorseForestCarrierCutIndexScope scope{
      ExactDirectMorseForestCarrierCutIndexScope::unspecified};

  [[nodiscard]] const ExactDirectMorseForestCarrierCutEntry* find_entry(
      ExactDirectSparseComponentHandle component_handle) const noexcept;
  [[nodiscard]] bool certified_forest_relative_closed_cut_index()
      const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectMorseForestCarrierCutIndexResult&,
      const ExactDirectMorseForestCarrierCutIndexResult&) = default;
};

struct ExactDirectMorseForestCarrierCutIndexVerification {
  bool trusted_source_forest_outcome_accepted{false};
  bool observed_storage_within_budget{false};
  bool expected_index_freshly_reconstructed{false};
  bool observed_recursively_equal{false};
  bool conditional_on_caller_fresh_source_forest_replay{true};
  bool external_locator_authority_replayed{false};
  bool original_geometry_replayed{false};
  bool global_morse_obligation_replayed{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectMorseForestCarrierCutIndexVerification&,
      const ExactDirectMorseForestCarrierCutIndexVerification&) = default;
};

[[nodiscard]] ExactDirectMorseForestCarrierCutIndexResult
build_exact_direct_morse_forest_carrier_cut_index(
    const ExactDirectMorseForestJournalResult& source_forest,
    std::size_t target_order,
    const exact::ExactLevel& closed_squared_level,
    const ExactDirectMorseForestCarrierCutIndexBudget& budget);

[[nodiscard]] ExactDirectMorseForestCarrierCutIndexVerification
verify_exact_direct_morse_forest_carrier_cut_index(
    const ExactDirectMorseForestJournalResult& source_forest,
    std::size_t target_order,
    const exact::ExactLevel& closed_squared_level,
    const ExactDirectMorseForestCarrierCutIndexBudget& trusted_budget,
    const ExactDirectMorseForestCarrierCutIndexResult& observed);

}  // namespace morsehgp3d::hierarchy
