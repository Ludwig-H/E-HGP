#pragma once

#include "morsehgp3d/hierarchy/direct_morse_event_rank_tower_link_journal.hpp"
#include "morsehgp3d/hierarchy/direct_morse_resident_all_orders_vertical_bridge.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_resident_event_crosswalk_journal_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_resident_event_crosswalk_journal_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_resident_event_crosswalk_journal_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_morse_resident_event_crosswalk_journal_mode =
        "certified_conditional_forest_event_to_live_sealed_resident_"
        "adjacent_target_crosswalk_n_at_least_k_plus_two_v1";
inline constexpr std::string_view
    direct_morse_resident_event_crosswalk_journal_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_morse_resident_event_crosswalk_journal_proof_basis =
        "fresh_forest_relative_adjacent_event_link_replay_live_sealed_"
        "normalized_incidence_complete_v7_bridge_direct_k2_birth_to_k1_"
        "binding_and_direct_lower_saddle_to_complete_frozen_o4_group_"
        "binding_exact_level_order_and_event_projection_join_conditional_"
        "on_shared_deterministic_canonical_source_order_without_a_common_"
        "event_semantic_digest_typed_target_namespaces_without_cross_domain_"
        "numeric_root_comparison_v1";

// Every source is already sparse.  These caps bound the logical cardinality
// of each retained source arena and the dense event-projection scratch.  They
// do not claim a single physical pass: certification, joining and canonical
// digest replay may revisit bounded records.  The crosswalk never enumerates
// supports or pairs and never materializes Gamma cells, cofaces or a Delaunay
// mosaic.
struct ExactDirectMorseResidentEventCrosswalkBudget {
  std::size_t maximum_link_scan_count{};
  std::size_t maximum_resident_plan_batch_scan_count{};
  std::size_t maximum_k2_batch_scan_count{};
  std::size_t maximum_higher_batch_scan_count{};
  std::size_t maximum_k2_direct_birth_binding_scan_count{};
  std::size_t maximum_direct_saddle_group_binding_scan_count{};
  std::size_t maximum_forest_birth_record_scan_count{};
  std::size_t maximum_resident_plan_direct_reference_scan_count{};
  std::size_t maximum_resident_direct_reference_replay_count{};
  std::size_t maximum_event_projection_scratch_entry_count{};
  std::size_t maximum_record_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectMorseResidentEventCrosswalkBudget&,
      const ExactDirectMorseResidentEventCrosswalkBudget&) = default;
};

// K1NodeId and ExactFrozenIncidencePriorRootId currently have the same
// machine representation upstream.  These wrappers deliberately make their
// scientific namespaces different in this journal.
struct ExactDirectMorseResidentEventCrosswalkK1NodeHandle {
  K1NodeId value{};

  friend bool operator==(
      const ExactDirectMorseResidentEventCrosswalkK1NodeHandle&,
      const ExactDirectMorseResidentEventCrosswalkK1NodeHandle&) = default;
};

struct ExactDirectMorseResidentEventCrosswalkAdjacentRootHandle {
  ExactFrozenIncidencePriorRootId value{};

  friend bool operator==(
      const ExactDirectMorseResidentEventCrosswalkAdjacentRootHandle&,
      const ExactDirectMorseResidentEventCrosswalkAdjacentRootHandle&) =
      default;
};

struct ExactDirectMorseResidentEventCrosswalkK1BirthTarget {
  std::size_t k2_batch_record_index{};
  std::size_t source_batch_index{};
  std::size_t binding_index{};
  std::size_t source_direct_reference_index{};
  std::size_t source_role_record_index{};
  std::size_t source_facet_token_index{};
  ExactDirectMorseResidentEventCrosswalkK1NodeHandle closed_k1_node{};

  friend bool operator==(
      const ExactDirectMorseResidentEventCrosswalkK1BirthTarget&,
      const ExactDirectMorseResidentEventCrosswalkK1BirthTarget&) = default;
};

struct ExactDirectMorseResidentEventCrosswalkO4SaddleTarget {
  bool target_batch_is_k2{false};
  std::size_t bridge_batch_record_index{};
  std::size_t source_batch_index{};
  std::size_t binding_index{};
  std::size_t source_direct_reference_index{};
  std::size_t source_role_record_index{};
  std::size_t source_incidence_family_index{};
  std::size_t source_hyperedge_index{};
  std::size_t owner_group_index{};
  std::size_t group_image_index{};
  ExactDirectMorseResidentEventCrosswalkAdjacentRootHandle
      resident_resultant_root{};

  friend bool operator==(
      const ExactDirectMorseResidentEventCrosswalkO4SaddleTarget&,
      const ExactDirectMorseResidentEventCrosswalkO4SaddleTarget&) = default;
};

// Exactly one target is populated.  source_order==2 selects k1_birth_target;
// source_order>=3 selects o4_saddle_target.  No code path compares their
// wrapped numeric values.
struct ExactDirectMorseResidentEventCrosswalkRecord {
  std::size_t record_index{};
  std::size_t source_link_index{};
  std::size_t source_birth_record_index{};
  std::size_t source_event_projection_index{};
  std::size_t source_order{};
  std::size_t target_order{};
  exact::ExactLevel squared_level{};
  std::size_t source_upper_birth_direct_reference_index{};
  std::size_t source_upper_birth_role_record_index{};
  std::size_t source_upper_birth_facet_token_index{};
  contract::CanonicalId source_event_arm_identity_digest{};
  contract::CanonicalId source_bridge_batch_membership_digest{};
  std::optional<ExactDirectMorseResidentEventCrosswalkK1BirthTarget>
      k1_birth_target;
  std::optional<ExactDirectMorseResidentEventCrosswalkO4SaddleTarget>
      o4_saddle_target;

  friend bool operator==(
      const ExactDirectMorseResidentEventCrosswalkRecord&,
      const ExactDirectMorseResidentEventCrosswalkRecord&) = default;
};

// Logical source cardinalities committed by the result digest.  A value is a
// bounded record count, not a hardware or physical-pass counter.
struct ExactDirectMorseResidentEventCrosswalkCounters {
  std::size_t link_scan_count{};
  std::size_t resident_plan_batch_scan_count{};
  std::size_t k2_batch_scan_count{};
  std::size_t higher_batch_scan_count{};
  std::size_t k2_direct_birth_binding_scan_count{};
  std::size_t direct_saddle_group_binding_scan_count{};
  std::size_t forest_birth_record_scan_count{};
  std::size_t resident_plan_direct_reference_scan_count{};
  std::size_t resident_direct_reference_replay_count{};
  std::size_t event_projection_scratch_entry_count{};
  std::size_t rank_two_k1_target_count{};
  std::size_t higher_rank_o4_target_count{};
  std::size_t record_count{};

  friend bool operator==(
      const ExactDirectMorseResidentEventCrosswalkCounters&,
      const ExactDirectMorseResidentEventCrosswalkCounters&) = default;
};

enum class ExactDirectMorseResidentEventCrosswalkDecision : std::uint8_t {
  not_certified,
  no_crosswalk_capacity_overflow,
  no_crosswalk_allocation_failed,
  no_crosswalk_budget_exhausted,
  no_crosswalk_product_n_at_least_k_plus_two_rejected,
  no_crosswalk_source_forest_rejected,
  no_crosswalk_source_link_replay_rejected,
  no_crosswalk_live_sealed_bridge_rejected,
  no_crosswalk_normalized_incidence_complete_v7_capability,
  no_crosswalk_source_namespace_mismatch,
  no_crosswalk_resident_plan_structure_inconsistent,
  no_crosswalk_bridge_membership_inconsistent,
  no_crosswalk_missing_event_target,
  no_crosswalk_duplicate_event_target,
  no_crosswalk_exact_level_or_order_mismatch,
  no_crosswalk_digest_encoding_failed,
  complete_conditional_resident_event_crosswalk,
};

enum class ExactDirectMorseResidentEventCrosswalkScope : std::uint8_t {
  unspecified,
  all_forest_rank_at_least_two_adjacent_event_links_to_live_sealed_resident_targets_under_n_at_least_k_plus_two,
};

struct ExactDirectMorseResidentEventCrosswalkJournalResult {
  static constexpr std::string_view backend =
      direct_morse_resident_event_crosswalk_journal_backend;
  static constexpr std::string_view profile =
      direct_morse_resident_event_crosswalk_journal_profile;
  static constexpr std::string_view mode =
      direct_morse_resident_event_crosswalk_journal_mode;
  static constexpr std::string_view public_status =
      direct_morse_resident_event_crosswalk_journal_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_resident_event_crosswalk_journal_proof_basis;

  std::uint32_t schema_version{
      direct_morse_resident_event_crosswalk_journal_schema_version};
  ExactDirectMorseResidentEventCrosswalkBudget requested_budget{};
  std::size_t point_count{};
  std::size_t effective_maximum_order{};
  std::size_t source_event_projection_count{};
  std::size_t source_link_count{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  contract::CanonicalId source_link_identity_digest{};
  contract::CanonicalId source_bridge_membership_digest{};
  // Commits the complete conditional result.  On atomic failure it commits
  // only the reset operational envelope (requested budget, failure decision,
  // fixed scope and zero/false payload); it is not scientific evidence.
  contract::CanonicalId crosswalk_digest{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp
      source_forest_final_locator_stamp{};
  ExactDirectMorseResidentAllOrdersVerticalBridgeStamp source_bridge_stamp{};
  std::vector<ExactDirectMorseResidentEventCrosswalkRecord> records;
  std::size_t logical_output_entry_count{};
  ExactDirectMorseResidentEventCrosswalkCounters counters{};
  bool budget_preflight_certified{false};
  bool product_n_at_least_k_plus_two_replayed{false};
  bool source_conditional_forest_certificate_replayed{false};
  bool source_link_journal_freshly_replayed_relative_to_forest{false};
  bool live_bridge_final_seal_verified{false};
  bool normalized_incidence_complete_v7_live_capability_replayed{false};
  bool canonical_cloud_and_point_namespace_joined{false};
  bool resident_projection_indices_joined_conditionally_by_deterministic_canonical_source_order{
      false};
  bool common_source_event_semantic_digest_replayed{false};
  bool every_resident_direct_birth_replayed_against_exact_forest_birth{
      false};
  bool every_resident_direct_birth_has_one_source_link{false};
  bool every_resident_direct_saddle_below_k_has_one_source_link{false};
  bool every_source_link_bound_exactly_once{false};
  bool every_rank_two_link_bound_to_direct_birth_k1_target{false};
  bool every_higher_rank_link_bound_to_lower_direct_saddle_o4_group{false};
  bool exact_level_and_adjacent_order_replayed{false};
  bool bridge_membership_digests_freshly_replayed{false};
  bool target_identifier_namespaces_typed_and_disjoint{false};
  bool cross_domain_numeric_root_equality_used{false};
  bool o4_membership_complete_for_supplied_resident_batches{false};
  bool full_pi0_replayed{false};
  bool o7_bidirectional_completeness_replayed{false};
  bool global_morse_obligation_replayed{false};
  bool m1_replayed{false};
  bool gamma_cells_or_global_cofaces_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool pair_matrix_materialized{false};
  bool public_status_claimed{false};
  bool no_partial_scientific_payload_published{false};
  ExactDirectMorseResidentEventCrosswalkDecision decision{
      ExactDirectMorseResidentEventCrosswalkDecision::not_certified};
  ExactDirectMorseResidentEventCrosswalkScope scope{
      ExactDirectMorseResidentEventCrosswalkScope::unspecified};

  [[nodiscard]] bool certified_conditional_resident_event_crosswalk()
      const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectMorseResidentEventCrosswalkJournalResult&,
      const ExactDirectMorseResidentEventCrosswalkJournalResult&) = default;
};

[[nodiscard]] ExactDirectMorseResidentEventCrosswalkJournalResult
build_exact_direct_morse_resident_event_crosswalk_journal(
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseEventRankTowerLinkBudget& trusted_link_budget,
    const ExactDirectMorseEventRankTowerLinkJournalResult& source_link,
    const ExactDirectMorseResidentAllOrdersVerticalBridge& live_bridge,
    const ExactDirectMorseResidentEventCrosswalkBudget& budget);

struct ExactDirectMorseResidentEventCrosswalkVerification {
  bool source_link_freshly_replayed_relative_to_forest{false};
  bool live_bridge_final_seal_verified{false};
  bool observed_storage_within_budget{false};
  bool expected_journal_freshly_rebuilt{false};
  bool observed_structure_certified{false};
  bool observed_recursively_equal{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectMorseResidentEventCrosswalkVerification&,
      const ExactDirectMorseResidentEventCrosswalkVerification&) = default;
};

// Rebuilds the canonical success or atomic-failure envelope from the trusted
// inputs.  Link replay and final-seal checks remain explicit diagnostics and
// may consult the supplied later sources even when an earlier failure fixes
// the expected envelope.  The builder's n-at-least-K-plus-two gate itself
// precedes link replay.  No observed crosswalk field steers reconstruction,
// and an observed arena outside the trusted budget or unequal to the freshly
// rebuilt envelope is never structurally hashed.
[[nodiscard]] ExactDirectMorseResidentEventCrosswalkVerification
verify_exact_direct_morse_resident_event_crosswalk_journal(
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseEventRankTowerLinkBudget& trusted_link_budget,
    const ExactDirectMorseEventRankTowerLinkJournalResult& source_link,
    const ExactDirectMorseResidentAllOrdersVerticalBridge& live_bridge,
    const ExactDirectMorseResidentEventCrosswalkBudget& trusted_budget,
    const ExactDirectMorseResidentEventCrosswalkJournalResult& observed);

}  // namespace morsehgp3d::hierarchy
