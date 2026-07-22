#pragma once

#include "morsehgp3d/hierarchy/direct_saddle_arm_seed_journal.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_closed_saddle_incidence_journal_schema_version = 1U;
inline constexpr std::string_view
    direct_closed_saddle_incidence_journal_backend = "reference_cpu";
inline constexpr std::string_view
    direct_closed_saddle_incidence_journal_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_closed_saddle_incidence_journal_mode = "certified";
inline constexpr std::string_view
    direct_closed_saddle_incidence_journal_refinement_status =
        "partial_refinement";
inline constexpr std::string_view
    direct_closed_saddle_incidence_journal_public_status = "not_claimed";
inline constexpr std::string_view
    direct_closed_saddle_incidence_journal_proof_basis =
        "source_relative_direct_saddle_all_deletions_partition_strict_"
        "support_arms_equal_level_interior_facets_v1";

// All caps are checked from the Phase-9 terminal facade before the source arm
// journal is replayed and before an output vector is reserved.
struct ExactDirectClosedSaddleIncidenceBudget {
  std::size_t maximum_source_family_scan_count{};
  std::size_t maximum_source_arm_seed_scan_count{};
  std::size_t maximum_incidence_family_count{};
  std::size_t maximum_equal_level_facet_seed_count{};

  friend bool operator==(
      const ExactDirectClosedSaddleIncidenceBudget&,
      const ExactDirectClosedSaddleIncidenceBudget&) = default;
};

// One record augments, without copying, one Phase-10.2 family.  Its strict
// facets remain in the source arm journal; only equal-level deletions receive
// new factorized records here.
struct ExactDirectClosedSaddleIncidenceFamilyRecord {
  std::size_t family_index{};
  std::size_t source_arm_family_index{};
  std::size_t source_event_index{};
  std::size_t journal_batch_index{};
  std::size_t order{};
  exact::ExactLevel critical_squared_level{};
  std::size_t strict_arm_seed_offset{};
  std::size_t strict_arm_seed_count{};
  std::size_t equal_level_facet_seed_offset{};
  std::size_t equal_level_facet_seed_count{};
  std::size_t closed_facet_count{};
  contract::CanonicalId source_event_identity_digest{};

  friend bool operator==(
      const ExactDirectClosedSaddleIncidenceFamilyRecord&,
      const ExactDirectClosedSaddleIncidenceFamilyRecord&) = default;
};

// This is the identity of (I union U) minus {x}, x in I.  The corresponding
// facet is rebuilt on demand in the existing fixed ten-PointId facet type.
struct ExactDirectEqualLevelFacetSeedRecord {
  std::size_t equal_level_facet_seed_index{};
  std::size_t family_index{};
  spatial::PointId removed_interior_point_id{};

  friend bool operator==(
      const ExactDirectEqualLevelFacetSeedRecord&,
      const ExactDirectEqualLevelFacetSeedRecord&) = default;
};

enum class ExactDirectClosedSaddleIncidenceDecision : std::uint8_t {
  not_certified,
  no_incidence_journal_capacity_overflow,
  no_incidence_journal_budget_exhausted,
  no_incidence_journal_source_not_certified,
  no_incidence_journal_source_join_inconsistent,
  complete_certified_direct_saddle_deletion_incidences,
};

enum class ExactDirectClosedSaddleIncidenceScope : std::uint8_t {
  unspecified,
  direct_saddle_strict_arms_and_equal_level_interior_facets_only,
};

struct ExactDirectClosedSaddleIncidenceJournalResult {
  static constexpr std::string_view backend =
      direct_closed_saddle_incidence_journal_backend;
  static constexpr std::string_view profile =
      direct_closed_saddle_incidence_journal_profile;
  static constexpr std::string_view mode =
      direct_closed_saddle_incidence_journal_mode;
  static constexpr std::string_view refinement_status =
      direct_closed_saddle_incidence_journal_refinement_status;
  static constexpr std::string_view public_status =
      direct_closed_saddle_incidence_journal_public_status;
  static constexpr std::string_view proof_basis =
      direct_closed_saddle_incidence_journal_proof_basis;

  std::uint32_t schema_version{
      direct_closed_saddle_incidence_journal_schema_version};
  ExactDirectClosedSaddleIncidenceBudget requested_budget{};
  std::size_t point_count{};
  std::size_t source_direct_event_count{};
  std::size_t required_source_family_scan_count{};
  std::size_t required_source_arm_seed_scan_count{};
  std::size_t required_incidence_family_count{};
  std::size_t required_equal_level_facet_seed_count{};
  std::size_t logical_added_storage_entry_count{};
  std::size_t logical_added_storage_entry_limit{};
  std::size_t combined_logical_storage_entry_count{};
  std::size_t combined_logical_storage_entry_limit{};
  contract::CanonicalId source_pair_canonical_cloud_digest{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  contract::CanonicalId source_pair_semantic_digest{};
  contract::CanonicalId source_higher_semantic_digest{};
  std::vector<ExactDirectClosedSaddleIncidenceFamilyRecord> families;
  std::vector<ExactDirectEqualLevelFacetSeedRecord> equal_level_facet_seeds;
  bool budget_preflight_certified{false};
  bool source_arm_journal_freshly_replayed{false};
  bool source_facade_join_certified{false};
  bool every_source_family_projected_once{false};
  bool every_interior_point_has_one_equal_level_seed{false};
  bool strict_and_equal_deletions_partition_every_saddle{false};
  bool equal_level_seed_order_is_canonical{false};
  bool factorized_facets_reconstruct_exactly{false};
  bool equal_level_miniball_sandwich_theorem_applies{false};
  bool strict_arms_reused_without_copy{false};
  bool output_linear_in_direct_events{false};
  bool no_forbidden_global_structure_materialized{false};
  bool facets_materialized_in_journal{false};
  bool miniballs_or_global_partitions_computed{false};
  bool frozen_quotient_performed{false};
  bool non_direct_gateway_generation_complete{false};
  bool hierarchy_reduction_performed{false};
  bool forest_or_gateway_attach_performed{false};
  bool missing_facet_means_isolated{false};
  bool public_status_claimed{false};
  bool partial_refinement_only{false};
  ExactDirectClosedSaddleIncidenceDecision decision{
      ExactDirectClosedSaddleIncidenceDecision::not_certified};
  ExactDirectClosedSaddleIncidenceScope scope{
      ExactDirectClosedSaddleIncidenceScope::unspecified};

  [[nodiscard]] bool certified_partial_refinement() const;

  friend bool operator==(
      const ExactDirectClosedSaddleIncidenceJournalResult&,
      const ExactDirectClosedSaddleIncidenceJournalResult&) = default;
};

[[nodiscard]] ExactDirectClosedSaddleIncidenceJournalResult
build_exact_direct_closed_saddle_incidence_journal(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& budget);

// Reconstructs (I union U) minus {x} in sorted PointId order.  No geometry,
// component lookup, quotient or hierarchy mutation is performed.
[[nodiscard]] ExactDirectSaddleArmFacet
reconstruct_exact_direct_equal_level_saddle_facet(
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceJournalResult& incidence_journal,
    std::size_t equal_level_facet_seed_index);

struct ExactDirectClosedSaddleIncidenceStreamingVerification {
  std::size_t source_family_scan_count{};
  std::size_t source_arm_seed_scan_count{};
  std::size_t equal_level_facet_seed_scan_count{};
  bool source_arm_journal_certified{false};
  bool requirements_certified{false};
  bool family_records_certified{false};
  bool equal_level_facet_seed_records_certified{false};
  bool deletion_partition_certified{false};
  bool factorized_facets_certified{false};
  bool result_facts_certified{false};
  bool decision_and_scope_certified{false};
  bool constant_auxiliary_record_storage_certified{false};
  bool fresh_streaming_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectClosedSaddleIncidenceStreamingVerification&,
      const ExactDirectClosedSaddleIncidenceStreamingVerification&) = default;
};

// Validates the output one source family and one seed at a time.  It does not
// allocate a second O(J+P) incidence payload.
[[nodiscard]] ExactDirectClosedSaddleIncidenceStreamingVerification
verify_exact_direct_closed_saddle_incidence_journal_streaming(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& trusted_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult& observed);

}  // namespace morsehgp3d::hierarchy
