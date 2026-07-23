#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_gateway_candidate_journal.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_positive_facet_locator.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_sparse_gateway_candidate_localization_schema_version = 1U;
inline constexpr std::string_view
    direct_sparse_gateway_candidate_localization_backend = "reference_cpu";
inline constexpr std::string_view
    direct_sparse_gateway_candidate_localization_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_gateway_candidate_localization_mode = "certified";
inline constexpr std::string_view
    direct_sparse_gateway_candidate_localization_refinement_status =
        "partial_refinement";
inline constexpr std::string_view
    direct_sparse_gateway_candidate_localization_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_sparse_gateway_candidate_localization_proof_basis =
        "fresh_10_7_replay_transient_candidate_deletions_full_key_global_"
        "deduplication_one_const_probe_per_distinct_facet_common_frozen_"
        "locator_snapshot_atomic_relative_positive_or_latent_tokens_v1";

// Every cap is global to one localization build.  The per-facet probe budget
// remains separate, so local exhaustion can never be disguised as a complete
// unresolved miss.  Logical storage counts the two records arenas plus every
// PointId logically owned by a distinct token key.
struct ExactDirectSparseGatewayCandidateLocalizationBudget {
  std::size_t maximum_source_candidate_scan_count{};
  std::size_t maximum_deletion_reference_count{};
  std::size_t maximum_distinct_facet_count{};
  std::size_t maximum_facet_key_point_count{};
  std::size_t maximum_aggregate_slot_visit_count{};
  std::size_t maximum_aggregate_component_parent_hop_count{};
  std::size_t maximum_logical_storage_entry_count{};
  ExactDirectSparsePositiveFacetProbeBudget facet_probe_budget{};

  friend bool operator==(
      const ExactDirectSparseGatewayCandidateLocalizationBudget&,
      const ExactDirectSparseGatewayCandidateLocalizationBudget&) = default;
};

enum class ExactDirectSparseGatewayFacetLocalizationDisposition
    : std::uint8_t {
  not_certified,
  latent_unresolved,
  relative_positive,
};

// Projections are canonically ordered by candidate index and then by the
// removed position in the sorted logical union F union {x}.  Repeated full
// deletion keys share one localized token.
struct ExactDirectSparseGatewayCandidateDeletionProjection {
  std::size_t deletion_projection_index{};
  std::size_t gateway_candidate_index{};
  std::size_t source_batch_index{};
  std::size_t removed_union_point_index{};
  std::size_t localized_facet_token_index{};

  friend bool operator==(
      const ExactDirectSparseGatewayCandidateDeletionProjection&,
      const ExactDirectSparseGatewayCandidateDeletionProjection&) = default;
};

// A latent token has no handle or binding witness.  A positive token contains
// only an existing relative locator binding; it never creates a component.
struct ExactDirectSparseGatewayLocalizedFacetToken {
  std::size_t localized_facet_token_index{};
  ExactDirectSparseFacetKey facet_key{};
  ExactDirectSparseComponentHandle component_handle{};
  ExactDirectSparseFacetWitness source_binding_witness{};
  bool component_handle_present{false};
  bool source_binding_witness_present{false};
  ExactDirectSparseGatewayFacetLocalizationDisposition disposition{
      ExactDirectSparseGatewayFacetLocalizationDisposition::not_certified};

  friend bool operator==(
      const ExactDirectSparseGatewayLocalizedFacetToken&,
      const ExactDirectSparseGatewayLocalizedFacetToken&) = default;
};

struct ExactDirectSparseGatewayCandidateLocalizationCounters {
  std::size_t source_candidate_scan_count{};
  std::size_t deletion_reference_count{};
  std::size_t distinct_facet_count{};
  std::size_t facet_key_point_count{};
  std::size_t locator_probe_count{};
  std::size_t relative_positive_facet_count{};
  std::size_t latent_unresolved_facet_count{};
  std::size_t slot_visit_count{};
  std::size_t component_parent_hop_count{};
  std::size_t full_key_comparison_count{};
  std::size_t equal_fingerprint_distinct_key_count{};
  std::size_t locator_snapshot_check_count{};

  friend bool operator==(
      const ExactDirectSparseGatewayCandidateLocalizationCounters&,
      const ExactDirectSparseGatewayCandidateLocalizationCounters&) =
      default;
};

enum class ExactDirectSparseGatewayCandidateLocalizationDecision
    : std::uint8_t {
  not_certified,
  no_localization_capacity_overflow,
  no_localization_budget_exhausted,
  no_localization_source_not_certified,
  no_localization_locator_probe_budget_exhausted,
  complete_certified_relative_gateway_candidate_localizations,
};

enum class ExactDirectSparseGatewayCandidateLocalizationScope
    : std::uint8_t {
  unspecified,
  candidate_deletion_facets_relative_to_frozen_positive_locator_only,
};

struct ExactDirectSparseGatewayCandidateLocalizationResult {
  static constexpr std::string_view backend =
      direct_sparse_gateway_candidate_localization_backend;
  static constexpr std::string_view profile =
      direct_sparse_gateway_candidate_localization_profile;
  static constexpr std::string_view mode =
      direct_sparse_gateway_candidate_localization_mode;
  static constexpr std::string_view refinement_status =
      direct_sparse_gateway_candidate_localization_refinement_status;
  static constexpr std::string_view public_status =
      direct_sparse_gateway_candidate_localization_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_gateway_candidate_localization_proof_basis;

  std::uint32_t schema_version{
      direct_sparse_gateway_candidate_localization_schema_version};
  ExactDirectSparseGatewayCandidateLocalizationBudget requested_budget{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  std::size_t point_count{};
  std::size_t source_gateway_candidate_count{};
  std::size_t required_source_candidate_scan_count{};
  std::size_t required_deletion_reference_count{};
  std::size_t required_distinct_facet_count{};
  std::size_t required_facet_key_point_count{};
  std::size_t required_locator_probe_count{};
  std::size_t logical_storage_entry_count{};
  ExactDirectSparseFacetWitness locator_query_witness{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_snapshot_stamp{};

  // These are the only two scientific output arenas.
  std::vector<ExactDirectSparseGatewayCandidateDeletionProjection>
      deletion_projections;
  std::vector<ExactDirectSparseGatewayLocalizedFacetToken>
      localized_facet_tokens;

  ExactDirectSparseGatewayCandidateLocalizationCounters counters{};
  bool source_gateway_candidate_journal_freshly_replayed{false};
  bool budget_preflight_certified{false};
  bool every_candidate_deletion_reconstructed{false};
  bool projections_canonical_and_partition_candidates{false};
  bool distinct_full_keys_globally_deduplicated{false};
  bool one_locator_probe_per_distinct_full_key{false};
  bool every_locator_probe_complete{false};
  bool relative_positive_and_latent_outcomes_separated{false};
  bool every_positive_token_has_existing_handle_and_binding_witness{false};
  bool every_latent_token_has_no_positive_payload{false};
  bool common_frozen_locator_snapshot_certified{false};
  bool logical_storage_within_budget{false};
  bool no_partial_scientific_payload_published{false};
  bool locator_state_mutated{false};
  bool locator_batch_committed{false};
  bool external_binding_authority_replayed{false};
  bool locator_snapshot_batch_level_alignment_claimed{false};
  bool missing_facet_means_isolated{false};
  bool singleton_component_created{false};
  bool root_union_or_forest_mutated{false};
  bool gateway_attach_published{false};
  bool eleven_point_coface_keys_materialized{false};
  bool gamma_cells_or_higher_order_delaunay_materialized{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  bool partial_refinement_only{false};
  ExactDirectSparseGatewayCandidateLocalizationDecision decision{
      ExactDirectSparseGatewayCandidateLocalizationDecision::not_certified};
  ExactDirectSparseGatewayCandidateLocalizationScope scope{
      ExactDirectSparseGatewayCandidateLocalizationScope::unspecified};

  // These predicates validate the self-contained outcome shape only.  Any
  // scientific downstream consumer must additionally call the fresh public
  // verifier with the original 10.7 authorities and frozen live locator.
  [[nodiscard]] bool certified_partial_refinement() const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectSparseGatewayCandidateLocalizationResult&,
      const ExactDirectSparseGatewayCandidateLocalizationResult&) = default;
};

struct ExactDirectSparseGatewayCandidateLocalizationVerification {
  bool trusted_inputs_certified{false};
  bool observed_storage_within_budget{false};
  bool locator_snapshot_matches_observed_build{false};
  bool source_gateway_candidate_journal_freshly_replayed{false};
  bool observed_outcome_well_formed{false};
  bool candidate_deletions_freshly_replayed{false};
  bool distinct_full_keys_freshly_replayed{false};
  bool locator_probes_freshly_replayed{false};
  bool counters_and_result_facts_freshly_replayed{false};
  bool no_locator_mutation_or_batch_commit{false};
  bool external_binding_authority_replayed{false};
  bool no_isolation_singleton_union_forest_or_attachment_invented{false};
  bool no_forbidden_global_structure_materialized{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectSparseGatewayCandidateLocalizationVerification&,
      const ExactDirectSparseGatewayCandidateLocalizationVerification&) =
      default;
};

// The locator must remain externally frozen for the complete call.  Snapshot
// comparisons reject sequential mutations but do not synchronize writers.
[[nodiscard]] ExactDirectSparseGatewayCandidateLocalizationResult
build_exact_direct_sparse_gateway_candidate_localization(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    const ExactDirectSparseGatewayCandidateJournalResult&
        source_gateway_journal,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseGatewayCandidateLocalizationBudget& budget,
    spatial::LbvhTraversalOrder traversal_order =
        spatial::LbvhTraversalOrder::near_first);

// The same external locator freeze is required throughout fresh replay.
[[nodiscard]] ExactDirectSparseGatewayCandidateLocalizationVerification
verify_exact_direct_sparse_gateway_candidate_localization(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    const ExactDirectSparseGatewayCandidateJournalResult&
        source_gateway_journal,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseGatewayCandidateLocalizationBudget& budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseGatewayCandidateLocalizationResult& observed);

}  // namespace morsehgp3d::hierarchy
