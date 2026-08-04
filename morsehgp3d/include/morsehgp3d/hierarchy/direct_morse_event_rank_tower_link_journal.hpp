#pragma once

#include "morsehgp3d/hierarchy/direct_morse_forest_journal.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_event_rank_tower_link_journal_schema_version = 2U;
inline constexpr std::string_view
    direct_morse_event_rank_tower_link_journal_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_event_rank_tower_link_journal_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_morse_event_rank_tower_link_journal_mode = "certified";
inline constexpr std::string_view
    direct_morse_event_rank_tower_link_journal_refinement_status =
        "conditional_event_rank_tower_link_candidate";
inline constexpr std::string_view
    direct_morse_event_rank_tower_link_journal_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_morse_event_rank_tower_link_journal_proof_basis =
        "forest_relative_same_direct_event_rank_r_birth_to_order_r_minus_1_"
        "saddle_exact_level_identity_strict_arm_terminal_births_and_atomic_"
        "post_batch_root_links_adjacent_only_lower_ranks_by_composition_"
        "source_forest_snapshot_bound_and_fresh_upstream_forest_replay_"
        "available_v2";

// All caps are checked before any output arena is reserved.  The source scan
// caps are explicit because this certificate is allowed to remain linear in
// the already sparse event forest, never in a Gamma complex or a Delaunay
// mosaic.
struct ExactDirectMorseEventRankTowerLinkBudget {
  std::size_t maximum_forest_birth_record_scan_count{};
  std::size_t maximum_forest_saddle_record_scan_count{};
  std::size_t maximum_forest_arm_binding_scan_count{};
  std::size_t maximum_forest_atomic_group_scan_count{};
  std::size_t maximum_forest_batch_scan_count{};
  std::size_t maximum_forest_node_scan_count{};
  std::size_t maximum_link_count{};
  std::size_t maximum_arm_terminal_reference_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectMorseEventRankTowerLinkBudget&,
      const ExactDirectMorseEventRankTowerLinkBudget&) = default;
};

struct ExactDirectMorseEventRankTowerArmTerminal {
  std::size_t arm_terminal_reference_index{};
  std::size_t source_arm_root_binding_index{};
  std::size_t terminal_birth_record_index{};
  ExactDirectSparseComponentHandle frozen_carrier_component_handle{};
  std::optional<ExactDirectMorseForestNodeId> prior_reduced_root_node_id;
  spatial::PointId removed_support_point_id{};
  ExactDirectSparseFacetKey terminal_birth_facet_key{};
  ExactDirectSparseFacetWitness terminal_birth_binding_witness{};
  exact::ExactCenter3 terminal_birth_exact_center{};
  exact::ExactLevel terminal_birth_exact_squared_level{};

  friend bool operator==(
      const ExactDirectMorseEventRankTowerArmTerminal&,
      const ExactDirectMorseEventRankTowerArmTerminal&) = default;
};

// A link is always adjacent.  A rank-r birth is anchored by the unique role
// of the same direct event in the closed order-(r-1) batch.  Non-adjacent
// tower maps are deliberately represented only by composition of these
// records.
struct ExactDirectMorseEventRankTowerLink {
  std::size_t link_index{};
  std::size_t source_birth_record_index{};
  std::size_t source_event_projection_index{};
  std::size_t source_order{};
  std::size_t target_order{};
  exact::ExactLevel squared_level{};
  std::size_t saddle_record_index{};
  std::size_t source_saddle_family_index{};
  std::size_t lower_batch_index{};
  std::size_t atomic_group_index{};
  ExactDirectMorseForestNodeId target_resulting_root_node_id{};
  std::size_t arm_terminal_offset{};
  std::size_t arm_terminal_count{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp
      lower_strict_pre_batch_stamp{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp
      lower_committed_post_batch_stamp{};
  contract::CanonicalId source_event_arm_identity_digest{};

  friend bool operator==(
      const ExactDirectMorseEventRankTowerLink&,
      const ExactDirectMorseEventRankTowerLink&) = default;
};

struct ExactDirectMorseEventRankTowerLinkCounters {
  std::size_t forest_birth_record_scan_count{};
  std::size_t forest_saddle_record_scan_count{};
  std::size_t forest_arm_binding_scan_count{};
  std::size_t forest_atomic_group_scan_count{};
  std::size_t forest_batch_scan_count{};
  std::size_t forest_node_scan_count{};
  std::size_t higher_rank_birth_count{};
  std::size_t terminal_maximum_rank_birth_count{};
  std::size_t link_count{};
  std::size_t arm_terminal_reference_count{};

  friend bool operator==(
      const ExactDirectMorseEventRankTowerLinkCounters&,
      const ExactDirectMorseEventRankTowerLinkCounters&) = default;
};

enum class ExactDirectMorseEventRankTowerLinkDecision : std::uint8_t {
  not_certified,
  no_link_capacity_overflow,
  no_link_allocation_failed,
  no_link_budget_exhausted,
  no_link_source_forest_rejected,
  no_link_source_forest_structure_inconsistent,
  no_link_missing_adjacent_saddle_role,
  no_link_duplicate_adjacent_saddle_role,
  no_link_duplicate_adjacent_birth_role,
  no_link_adjacent_order_mismatch,
  no_link_exact_level_mismatch,
  no_link_arm_terminal_inconsistent,
  no_link_atomic_group_inconsistent,
  no_link_terminal_maximum_rank_birth_missing_or_nonunique,
  complete_certified_conditional_event_rank_tower_links,
};

enum class ExactDirectMorseEventRankTowerLinkScope : std::uint8_t {
  unspecified,
  all_present_rank_at_least_two_births_to_same_event_adjacent_lower_order_saddles_only,
};

struct ExactDirectMorseEventRankTowerLinkJournalResult {
  static constexpr std::string_view backend =
      direct_morse_event_rank_tower_link_journal_backend;
  static constexpr std::string_view profile =
      direct_morse_event_rank_tower_link_journal_profile;
  static constexpr std::string_view mode =
      direct_morse_event_rank_tower_link_journal_mode;
  static constexpr std::string_view refinement_status =
      direct_morse_event_rank_tower_link_journal_refinement_status;
  static constexpr std::string_view public_status =
      direct_morse_event_rank_tower_link_journal_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_event_rank_tower_link_journal_proof_basis;

  std::uint32_t schema_version{
      direct_morse_event_rank_tower_link_journal_schema_version};
  ExactDirectMorseEventRankTowerLinkBudget requested_budget{};
  std::size_t point_count{};
  std::size_t effective_maximum_order{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  std::uint32_t source_forest_schema_version{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp
      source_forest_final_locator_stamp{};
  std::size_t source_logical_birth_record_count{};
  std::size_t source_saddle_record_count{};
  std::size_t source_arm_binding_count{};
  std::size_t source_atomic_group_count{};
  std::size_t source_batch_count{};
  std::vector<ExactDirectMorseEventRankTowerLink> links;
  std::vector<ExactDirectMorseEventRankTowerArmTerminal> arm_terminals;
  std::size_t logical_output_entry_count{};
  ExactDirectMorseEventRankTowerLinkCounters counters{};
  bool budget_preflight_certified{false};
  bool source_conditional_forest_certificate_replayed{false};
  bool source_forest_structure_replayed{false};
  bool every_rank_at_least_two_birth_linked_exactly_once{false};
  bool same_source_event_projection_replayed{false};
  bool adjacent_order_role_replayed{false};
  bool exact_level_identity_replayed{false};
  bool every_saddle_arm_terminal_is_strictly_earlier_lower_order_birth{
      false};
  bool strict_pre_batch_roots_replayed{false};
  bool atomic_post_batch_target_replayed{false};
  bool terminal_maximum_rank_link_required{false};
  bool all_present_terminal_maximum_rank_births_included{false};
  bool lower_rank_links_are_composition_only{false};
  bool output_csr_and_linear_in_source_events_and_arms{false};
  bool no_partial_scientific_payload_published{false};
  bool gamma_cells_or_global_cofaces_materialized{false};
  bool higher_order_delaunay_materialized{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseEventRankTowerLinkDecision decision{
      ExactDirectMorseEventRankTowerLinkDecision::not_certified};
  ExactDirectMorseEventRankTowerLinkScope scope{
      ExactDirectMorseEventRankTowerLinkScope::unspecified};
  std::size_t source_event_projection_count{};

  [[nodiscard]] bool certified_conditional_event_rank_tower_links()
      const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectMorseEventRankTowerLinkJournalResult&,
      const ExactDirectMorseEventRankTowerLinkJournalResult&) = default;
};

[[nodiscard]] ExactDirectMorseEventRankTowerLinkJournalResult
build_exact_direct_morse_event_rank_tower_link_journal(
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseEventRankTowerLinkBudget& budget);

struct ExactDirectMorseEventRankTowerLinkVerification {
  bool source_forest_certified{false};
  bool observed_storage_within_budget{false};
  bool expected_journal_freshly_rebuilt{false};
  bool observed_structure_certified{false};
  bool observed_recursively_equal{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectMorseEventRankTowerLinkVerification&,
      const ExactDirectMorseEventRankTowerLinkVerification&) = default;
};

// Rebuilds every link and every CSR arm reference from the immutable forest.
// No field of the observed result steers the replay.
[[nodiscard]] ExactDirectMorseEventRankTowerLinkVerification
verify_exact_direct_morse_event_rank_tower_link_journal(
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseEventRankTowerLinkBudget& trusted_budget,
    const ExactDirectMorseEventRankTowerLinkJournalResult& observed);

// The compact builder and verifier above are deliberately forest-relative.
// This stronger wrapper first reconstructs the complete source forest from
// its immutable upstream inputs and only then rebuilds the adjacent-rank link
// journal.  It remains conditional on the caller's separate fresh Phase-9
// facade replay, exactly like ExactDirectMorseForestVerification.
struct ExactDirectMorseEventRankTowerLinkFreshForestVerification {
  bool trusted_forest_inputs_certified{false};
  bool source_forest_storage_within_budget{false};
  bool source_forest_freshly_reconstructed{false};
  bool source_forest_recursively_equal{false};
  bool conditional_on_caller_fresh_phase9_facade_replay{true};
  bool source_forest_certified{false};
  bool observed_link_storage_within_budget{false};
  bool expected_link_journal_freshly_rebuilt{false};
  bool observed_link_structure_certified{false};
  bool observed_link_recursively_equal{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectMorseEventRankTowerLinkFreshForestVerification&,
      const ExactDirectMorseEventRankTowerLinkFreshForestVerification&) =
      default;
};

[[nodiscard]] ExactDirectMorseEventRankTowerLinkFreshForestVerification
verify_exact_direct_morse_event_rank_tower_link_journal_from_fresh_forest(
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
    const ExactDirectMorseEventRankTowerLinkJournalResult& observed_link);

}  // namespace morsehgp3d::hierarchy
