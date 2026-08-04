#pragma once

#include "morsehgp3d/hierarchy/direct_morse_event_rank_tower_link_journal.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_event_vertical_propagation_journal_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_event_vertical_propagation_journal_backend =
        "reference_cpu";
inline constexpr std::string_view
    direct_morse_event_vertical_propagation_journal_profile =
        "hgp_reduced";
inline constexpr std::string_view
    direct_morse_event_vertical_propagation_journal_mode = "certified";
inline constexpr std::string_view
    direct_morse_event_vertical_propagation_journal_refinement_status =
        "conditional_event_vertical_propagation_candidate";
inline constexpr std::string_view
    direct_morse_event_vertical_propagation_journal_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_morse_event_vertical_propagation_journal_proof_basis =
        "forest_relative_adjacent_event_carrier_anchors_monotone_exact_"
        "lower_order_parent_activation_all_atomic_group_inputs_converge_"
        "final_reduced_roots_all_nonterminal_carriers_consumed_and_"
        "terminal_latent_carrier_retained_"
        "conditional_on_o3_o4_and_not_m1_v1";

// Every storage cap is checked before an output arena is reserved.  The flat
// parent/anchor arenas remain linear in the already reduced forest topology;
// the aggregate parent-find work has its own explicit runtime cap.  No facet
// universe, Gamma cell, coface catalogue or higher-order Delaunay mosaic is
// admitted by this reference certificate.
struct ExactDirectMorseEventVerticalPropagationBudget {
  std::size_t maximum_source_link_scan_count{};
  std::size_t maximum_source_group_scan_count{};
  std::size_t maximum_source_arm_binding_scan_count{};
  std::size_t maximum_source_node_scan_count{};
  std::size_t maximum_source_child_reference_scan_count{};
  std::size_t maximum_source_final_root_scan_count{};
  std::size_t maximum_carrier_anchor_count{};
  std::size_t maximum_group_anchor_count{};
  std::size_t maximum_group_input_reference_count{};
  std::size_t maximum_final_root_anchor_count{};
  std::size_t maximum_parent_activation_count{};
  std::size_t maximum_parent_find_step_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectMorseEventVerticalPropagationBudget&,
      const ExactDirectMorseEventVerticalPropagationBudget&) = default;
};

struct ExactDirectMorseEventVerticalCarrierAnchor {
  std::size_t carrier_anchor_index{};
  std::size_t source_link_index{};
  std::size_t source_birth_record_index{};
  std::size_t source_order{};
  exact::ExactLevel squared_level{};
  ExactDirectMorseForestNodeId target_root_node_id{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp
      target_strict_pre_batch_stamp{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp
      target_committed_post_batch_stamp{};
  contract::CanonicalId source_event_arm_identity_digest{};
  bool referenced_by_source_atomic_group{false};
  bool remains_latent_without_source_group{false};
  bool terminal_maximum_rank_latent_carrier{false};

  friend bool operator==(
      const ExactDirectMorseEventVerticalCarrierAnchor&,
      const ExactDirectMorseEventVerticalCarrierAnchor&) = default;
};

enum class ExactDirectMorseEventVerticalGroupInputKind : std::uint8_t {
  carrier_birth_anchor,
  prior_source_root_anchor,
};

// A canonical input slice contains all distinct carrier handles first, then
// all distinct prior reduced roots.  Both source identity fields are retained
// explicitly so a carrier/root substitution cannot hide behind a shared
// lower-order target.
struct ExactDirectMorseEventVerticalGroupInputReference {
  std::size_t group_input_reference_index{};
  std::size_t group_anchor_index{};
  ExactDirectMorseEventVerticalGroupInputKind kind{
      ExactDirectMorseEventVerticalGroupInputKind::carrier_birth_anchor};
  std::optional<std::size_t> source_carrier_anchor_index;
  std::optional<std::size_t> source_birth_record_index;
  std::optional<ExactDirectMorseForestNodeId> source_root_node_id;
  ExactDirectMorseForestNodeId target_root_before_advance{};
  ExactDirectMorseForestNodeId target_root_at_group_level{};

  friend bool operator==(
      const ExactDirectMorseEventVerticalGroupInputReference&,
      const ExactDirectMorseEventVerticalGroupInputReference&) = default;
};

struct ExactDirectMorseEventVerticalGroupAnchor {
  std::size_t group_anchor_index{};
  std::size_t source_atomic_group_index{};
  std::size_t source_batch_index{};
  std::size_t source_order{};
  exact::ExactLevel squared_level{};
  ExactDirectMorseForestNodeId source_resulting_root_node_id{};
  std::size_t input_reference_offset{};
  std::size_t input_reference_count{};
  ExactDirectMorseForestNodeId target_resulting_root_node_id{};

  friend bool operator==(
      const ExactDirectMorseEventVerticalGroupAnchor&,
      const ExactDirectMorseEventVerticalGroupAnchor&) = default;
};

struct ExactDirectMorseEventVerticalFinalRootAnchor {
  std::size_t final_root_anchor_index{};
  std::size_t source_final_root_index{};
  std::size_t source_order{};
  ExactDirectMorseForestNodeId source_root_node_id{};
  ExactDirectMorseForestNodeId target_final_root_node_id{};

  friend bool operator==(
      const ExactDirectMorseEventVerticalFinalRootAnchor&,
      const ExactDirectMorseEventVerticalFinalRootAnchor&) = default;
};

struct ExactDirectMorseEventVerticalPropagationCounters {
  std::size_t source_link_scan_count{};
  std::size_t source_group_scan_count{};
  std::size_t source_arm_binding_scan_count{};
  std::size_t source_node_scan_count{};
  std::size_t source_child_reference_scan_count{};
  std::size_t source_final_root_scan_count{};
  std::size_t carrier_anchor_count{};
  std::size_t unconsumed_latent_carrier_anchor_count{};
  std::size_t terminal_maximum_rank_latent_carrier_anchor_count{};
  std::size_t group_anchor_count{};
  std::size_t group_input_reference_count{};
  std::size_t final_root_anchor_count{};
  std::size_t parent_activation_count{};
  std::size_t parent_find_step_count{};
  std::size_t path_compression_write_count{};

  friend bool operator==(
      const ExactDirectMorseEventVerticalPropagationCounters&,
      const ExactDirectMorseEventVerticalPropagationCounters&) = default;
};

enum class ExactDirectMorseEventVerticalPropagationDecision
    : std::uint8_t {
  not_certified,
  no_propagation_capacity_overflow,
  no_propagation_allocation_failed,
  no_propagation_budget_exhausted,
  no_propagation_source_forest_rejected,
  no_propagation_source_link_journal_rejected,
  no_propagation_source_structure_inconsistent,
  no_propagation_parent_forest_inconsistent,
  no_propagation_carrier_link_lookup_inconsistent,
  no_propagation_source_group_input_inconsistent,
  no_propagation_missing_source_root_anchor,
  no_propagation_nonconvergent_group_anchors,
  no_propagation_final_root_inconsistent,
  no_propagation_nonterminal_carrier_unconsumed,
  no_propagation_terminal_latent_carrier_inconsistent,
  complete_certified_conditional_event_vertical_propagation,
};

enum class ExactDirectMorseEventVerticalPropagationScope
    : std::uint8_t {
  unspecified,
  all_adjacent_event_carriers_order_at_least_two_atomic_source_groups_and_reduced_final_roots_relative_to_the_conditional_direct_forest,
};

struct ExactDirectMorseEventVerticalPropagationJournalResult {
  static constexpr std::string_view backend =
      direct_morse_event_vertical_propagation_journal_backend;
  static constexpr std::string_view profile =
      direct_morse_event_vertical_propagation_journal_profile;
  static constexpr std::string_view mode =
      direct_morse_event_vertical_propagation_journal_mode;
  static constexpr std::string_view refinement_status =
      direct_morse_event_vertical_propagation_journal_refinement_status;
  static constexpr std::string_view public_status =
      direct_morse_event_vertical_propagation_journal_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_event_vertical_propagation_journal_proof_basis;

  std::uint32_t schema_version{
      direct_morse_event_vertical_propagation_journal_schema_version};
  ExactDirectMorseEventVerticalPropagationBudget requested_budget{};
  ExactDirectMorseEventRankTowerLinkBudget trusted_source_link_budget{};
  std::size_t point_count{};
  std::size_t effective_maximum_order{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  std::uint32_t source_forest_schema_version{};
  std::uint32_t source_link_journal_schema_version{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp
      source_forest_final_locator_stamp{};
  std::size_t source_link_count{};
  // Counts every source forest group, including order one.  Vertical group
  // anchors below intentionally cover only source orders >= 2: order one has
  // no adjacent lower rank to which an event could be propagated.
  std::size_t source_group_count{};
  std::size_t source_arm_binding_count{};
  std::size_t source_logical_node_count{};
  std::size_t source_child_reference_count{};
  std::size_t source_final_root_count{};
  std::vector<ExactDirectMorseEventVerticalCarrierAnchor>
      carrier_anchors;
  std::vector<ExactDirectMorseEventVerticalGroupAnchor> group_anchors;
  std::vector<ExactDirectMorseEventVerticalGroupInputReference>
      group_input_references;
  std::vector<ExactDirectMorseEventVerticalFinalRootAnchor>
      final_root_anchors;
  std::size_t logical_output_entry_count{};
  ExactDirectMorseEventVerticalPropagationCounters counters{};
  bool budget_preflight_certified{false};
  bool source_conditional_forest_compact_contract_checked{false};
  bool source_link_journal_forest_relative_freshly_rebuilt{false};
  bool conditional_on_caller_fresh_upstream_forest_and_link_replay{true};
  bool source_parent_forest_reconstructed{false};
  bool one_anchor_per_adjacent_event_link{false};
  bool every_order_at_least_two_source_group_input_propagated{false};
  bool every_order_at_least_two_source_group_anchor_converged{false};
  bool every_reduced_final_root_advanced_to_lower_final_root{false};
  bool every_nonterminal_higher_order_carrier_reaches_source_group{false};
  bool terminal_maximum_rank_latent_carrier_required{false};
  bool terminal_maximum_rank_latent_carrier_retained{false};
  bool lower_rank_maps_are_composition_only{false};
  bool output_storage_flat_and_linear_in_sparse_forest{false};
  bool conditional_on_direct_forest_o3_o4{true};
  bool gamma_totality_replayed{false};
  bool m1_reconstruction_claimed{false};
  bool no_partial_scientific_payload_published{false};
  bool gamma_cells_or_global_cofaces_materialized{false};
  bool higher_order_delaunay_materialized{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseEventVerticalPropagationDecision decision{
      ExactDirectMorseEventVerticalPropagationDecision::not_certified};
  ExactDirectMorseEventVerticalPropagationScope scope{
      ExactDirectMorseEventVerticalPropagationScope::unspecified};

  [[nodiscard]] bool certified_conditional_event_vertical_propagation()
      const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectMorseEventVerticalPropagationJournalResult&,
      const ExactDirectMorseEventVerticalPropagationJournalResult&) =
      default;
};

[[nodiscard]] ExactDirectMorseEventVerticalPropagationJournalResult
build_exact_direct_morse_event_vertical_propagation_journal(
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseEventRankTowerLinkJournalResult&
        source_link_journal,
    const ExactDirectMorseEventRankTowerLinkBudget&
        trusted_source_link_budget,
    const ExactDirectMorseEventVerticalPropagationBudget& budget);

// Output and scratch storage are output-sensitive and linear in the compact
// forest/link payload.  The CPU reference builder sorts E reduced parent
// edges, so its construction time includes O(E log E); this is not advertised
// as a linear-time product kernel.

struct ExactDirectMorseEventVerticalPropagationVerification {
  bool source_forest_compact_contract_checked{false};
  bool source_link_journal_forest_relative_freshly_verified{false};
  bool conditional_on_caller_fresh_upstream_forest_and_link_replay{true};
  bool observed_storage_within_budget{false};
  bool expected_journal_freshly_rebuilt{false};
  bool observed_structure_certified{false};
  bool observed_recursively_equal{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectMorseEventVerticalPropagationVerification&,
      const ExactDirectMorseEventVerticalPropagationVerification&) =
      default;
};

// Reconstructs parent activation, every carrier/group/final-root anchor and
// every CSR input from the immutable forest and a freshly verified link
// journal.  No observed propagation field steers the replay.
[[nodiscard]] ExactDirectMorseEventVerticalPropagationVerification
verify_exact_direct_morse_event_vertical_propagation_journal(
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseEventRankTowerLinkJournalResult&
        source_link_journal,
    const ExactDirectMorseEventRankTowerLinkBudget&
        trusted_source_link_budget,
    const ExactDirectMorseEventVerticalPropagationBudget& trusted_budget,
    const ExactDirectMorseEventVerticalPropagationJournalResult& observed);

// Optional strong wrapper: the source forest, adjacent-rank links and this
// propagation journal are all freshly reconstructed from the upstream event
// and arm authorities.  As for the forest itself, the Phase-9 facade replay
// remains an explicit caller premise rather than a claim of this component.
struct ExactDirectMorseEventVerticalPropagationFreshForestVerification {
  ExactDirectMorseEventRankTowerLinkFreshForestVerification
      source_link_fresh_forest_verification{};
  ExactDirectMorseEventVerticalPropagationVerification
      propagation_verification{};
  bool conditional_on_caller_fresh_phase9_facade_replay{true};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectMorseEventVerticalPropagationFreshForestVerification&,
      const ExactDirectMorseEventVerticalPropagationFreshForestVerification&)
      = default;
};

[[nodiscard]]
ExactDirectMorseEventVerticalPropagationFreshForestVerification
verify_exact_direct_morse_event_vertical_propagation_journal_from_fresh_forest(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& trusted_forest_budget,
    const ExactDirectMorseForestConfig& forest_config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectMorseForestJournalResult& observed_forest,
    const ExactDirectMorseEventRankTowerLinkBudget& trusted_link_budget,
    const ExactDirectMorseEventRankTowerLinkJournalResult& observed_link,
    const ExactDirectMorseEventVerticalPropagationBudget&
        trusted_propagation_budget,
    const ExactDirectMorseEventVerticalPropagationJournalResult&
        observed_propagation);

}  // namespace morsehgp3d::hierarchy
