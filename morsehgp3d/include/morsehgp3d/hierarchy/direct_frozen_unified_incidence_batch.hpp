#pragma once

#include "morsehgp3d/hierarchy/direct_frozen_incidence_hgp_action_plan.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_unified_level_plan.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_frozen_unified_incidence_batch_schema_version = 3U;
inline constexpr std::string_view
    direct_frozen_unified_incidence_batch_backend = "reference_cpu";
inline constexpr std::string_view
    direct_frozen_unified_incidence_batch_profile = "hgp_reduced";
inline constexpr std::string_view direct_frozen_unified_incidence_batch_mode =
    "certified_frozen_unified_incidence_batch_v3";
inline constexpr std::string_view
    direct_frozen_unified_incidence_batch_public_status = "not_claimed";
inline constexpr std::string_view
    direct_frozen_unified_incidence_batch_proof_basis =
        "fresh_unified_level_plan_replay_external_exhaustive_facet_"
        "resolution_prior_root_and_latent_carrier_point_coverage_"
        "factorized_coface_"
        "hyperedges_frozen_typed_quotient_qr_actions_and_exact_group_"
        "facet_point_set_differences_fresh_call_or_verified_immutable_"
        "resident_plan_authority_with_typed_normalized_source_scope_v3";

// One record is required for every facet token touched by a non-deferred
// coface in the selected batch.  The records are strictly ordered by
// facet_token_index and exhaustive: direct births do not make a token
// touched, because they remain deferred outside the incidence quotient.
// A rooted carrier has exactly one prior_root_id.  Latent carriers and equal
// facets have none.  The complete typed pair is caller-owned and opaque.
struct ExactDirectFrozenUnifiedFacetResolution {
  std::size_t facet_token_index{};
  ExactFrozenIncidenceToken token{};
  std::optional<ExactFrozenIncidencePriorRootId> prior_root_id;

  friend bool operator==(
      const ExactDirectFrozenUnifiedFacetResolution&,
      const ExactDirectFrozenUnifiedFacetResolution&) = default;
};

// This is an exact CSR snapshot of the pre-batch point coverage of every
// prior root named by a rooted facet resolution.  Root records are strictly
// prior-root ordered, their slices partition point_references, and each
// slice is strictly PointId ordered.  Coverage may overlap between roots.
struct ExactDirectFrozenUnifiedPriorRootCoverage {
  ExactFrozenIncidencePriorRootId prior_root_id{};
  std::size_t point_reference_offset{};
  std::size_t point_reference_count{};

  friend bool operator==(
      const ExactDirectFrozenUnifiedPriorRootCoverage&,
      const ExactDirectFrozenUnifiedPriorRootCoverage&) = default;
};

// This second exact CSR authority covers every distinct latent_carrier token
// named by facet_resolutions.  Records are strictly latent-token-id ordered,
// their slices partition point_references, and every slice is strictly PointId
// ordered.  A slice is the complete pre-batch point coverage of that latent
// carrier, not merely the points of the source facet that resolved to it.
// Consequently, every source facet mapped to the token must be contained in
// the slice, but the slice may contain additional carrier points.
struct ExactDirectFrozenUnifiedLatentCarrierCoverage {
  ExactFrozenIncidenceTokenId latent_carrier_token_id{};
  std::size_t point_reference_offset{};
  std::size_t point_reference_count{};

  friend bool operator==(
      const ExactDirectFrozenUnifiedLatentCarrierCoverage&,
      const ExactDirectFrozenUnifiedLatentCarrierCoverage&) = default;
};

// Every potentially input-sized scan, transient arena and persistent arena
// has an explicit cap.  Subordinate quotient and action budgets are derived
// from these caps, so there is no second unguarded capacity namespace.
struct ExactDirectFrozenUnifiedIncidenceBatchBudget {
  std::size_t maximum_facet_resolution_count{};
  std::size_t maximum_prior_root_coverage_count{};
  std::size_t maximum_prior_root_coverage_point_reference_count{};
  std::size_t maximum_latent_carrier_coverage_count{};
  std::size_t maximum_latent_carrier_coverage_point_reference_count{};
  std::size_t maximum_batch_direct_reference_scan_count{};
  std::size_t maximum_direct_saddle_hyperedge_count{};
  std::size_t maximum_residual_hyperedge_count{};
  std::size_t maximum_coface_facet_reference_scan_count{};
  // Counts logical reads of source PointId entries by this builder: each
  // supplied coverage reference, a conservative implementation-independent
  // upper bound for every rooted/latent containment binary-search probe,
  // every factorized facet occurrence expanded once, and each distinct latent
  // carrier coverage expanded once per source hyperedge that names it.
  // Sorting comparisons over already-copied scratch entries are not rescans of
  // a source authority and are bounded separately by maximum_scratch_entry_count.
  std::size_t maximum_source_point_reference_scan_count{};
  std::size_t maximum_hyperedge_count{};
  std::size_t maximum_token_reference_count{};
  std::size_t maximum_distinct_typed_token_count{};
  std::size_t maximum_root_attachment_count{};
  std::size_t maximum_group_count{};
  std::size_t maximum_residual_incidence_record_count{};
  std::size_t maximum_equal_facet_binding_record_count{};
  std::size_t maximum_coverage_delta_record_count{};
  std::size_t maximum_coverage_delta_facet_reference_count{};
  std::size_t maximum_coverage_delta_point_reference_count{};
  std::size_t maximum_scratch_entry_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectFrozenUnifiedIncidenceBatchBudget&,
      const ExactDirectFrozenUnifiedIncidenceBatchBudget&) = default;
};

// One record survives for every residual coface.  The facet slice is in
// incidence_facet_token_indices and is aligned with the typed-token slice of
// quotient_hyperedges[source_hyperedge_index].  Direct saddles are already
// named by the freshly replayed unified plan and need no duplicate record.
// Canonical ownership and local scientific truth are deliberately separate.
// The owned counts concern only delta references whose canonical
// first_source_hyperedge_index is this record's hyperedge.  local_* is copied
// from provenance[source_hyperedge_index] and therefore remains the exact
// per-incidence point delta consumed by the HGP action planner.  An incidence
// may have a non-zero local delta while canonically owning no group delta.
struct ExactDirectFrozenUnifiedResidualIncidenceRecord {
  std::size_t residual_incidence_record_index{};
  std::size_t source_residual_reference_index{};
  std::size_t source_star_coface_index{};
  std::size_t source_hyperedge_index{};
  std::size_t owner_group_index{};
  std::size_t facet_reference_offset{};
  std::size_t facet_reference_count{};
  std::size_t canonically_owned_facet_delta_count{};
  std::size_t canonically_owned_point_delta_count{};
  std::size_t local_added_point_count{};
  bool local_zero_point_delta_certified{false};
  bool local_fully_redundant_certified{false};
  bool canonically_owns_no_coverage_delta{false};

  friend bool operator==(
      const ExactDirectFrozenUnifiedResidualIncidenceRecord&,
      const ExactDirectFrozenUnifiedResidualIncidenceRecord&) = default;
};

// Equal facets are retained separately so a later reducer can bind the
// newly active facet without inventing a latent carrier.  Records are
// facet-token ordered and the owner is the canonical quotient group.
struct ExactDirectFrozenUnifiedEqualFacetBindingRecord {
  std::size_t equal_facet_binding_record_index{};
  std::size_t facet_token_index{};
  ExactFrozenIncidenceTokenId equal_facet_token_id{};
  std::size_t owner_group_index{};
  std::size_t coverage_delta_facet_reference_index{};

  friend bool operator==(
      const ExactDirectFrozenUnifiedEqualFacetBindingRecord&,
      const ExactDirectFrozenUnifiedEqualFacetBindingRecord&) = default;
};

// One delta is emitted per non-deferred quotient/action group.  Facets are
// the exact set difference between all group facets and the facets already
// resolved to prior roots.  Points are the exact set difference between the
// union of group-facet PointIds and complete latent-carrier coverages, minus
// the supplied complete pre-batch coverages of all prior roots in the group.
struct ExactDirectFrozenUnifiedCoverageDeltaRecord {
  std::size_t coverage_delta_record_index{};
  std::size_t owner_group_index{};
  std::size_t facet_reference_offset{};
  std::size_t facet_reference_count{};
  std::size_t point_reference_offset{};
  std::size_t point_reference_count{};
  std::size_t q_r{};
  ExactFrozenIncidenceHgpAction action{
      ExactFrozenIncidenceHgpAction::reduced_birth};
  bool fully_redundant{false};

  friend bool operator==(
      const ExactDirectFrozenUnifiedCoverageDeltaRecord&,
      const ExactDirectFrozenUnifiedCoverageDeltaRecord&) = default;
};

struct ExactDirectFrozenUnifiedCoverageDeltaFacetReference {
  std::size_t owner_group_index{};
  std::size_t first_source_hyperedge_index{};
  std::size_t facet_token_index{};

  friend bool operator==(
      const ExactDirectFrozenUnifiedCoverageDeltaFacetReference&,
      const ExactDirectFrozenUnifiedCoverageDeltaFacetReference&) = default;
};

struct ExactDirectFrozenUnifiedCoverageDeltaPointReference {
  std::size_t owner_group_index{};
  std::size_t first_source_hyperedge_index{};
  spatial::PointId point_id{};

  friend bool operator==(
      const ExactDirectFrozenUnifiedCoverageDeltaPointReference&,
      const ExactDirectFrozenUnifiedCoverageDeltaPointReference&) = default;
};

struct ExactDirectFrozenUnifiedIncidenceBatchCounters {
  std::size_t facet_resolution_count{};
  std::size_t prior_root_coverage_count{};
  std::size_t prior_root_coverage_point_reference_count{};
  std::size_t latent_carrier_coverage_count{};
  std::size_t latent_carrier_coverage_point_reference_count{};
  std::size_t batch_direct_reference_scan_count{};
  std::size_t deferred_direct_birth_count{};
  std::size_t direct_saddle_hyperedge_count{};
  std::size_t residual_hyperedge_count{};
  std::size_t coface_facet_reference_scan_count{};
  std::size_t source_point_reference_scan_count{};
  std::size_t hyperedge_count{};
  std::size_t token_reference_count{};
  std::size_t distinct_typed_token_count{};
  std::size_t root_attachment_count{};
  std::size_t group_count{};
  std::size_t q_r_zero_group_count{};
  std::size_t q_r_one_group_count{};
  std::size_t q_r_multiple_group_count{};
  std::size_t residual_incidence_record_count{};
  std::size_t equal_facet_binding_record_count{};
  std::size_t coverage_delta_record_count{};
  std::size_t coverage_delta_facet_reference_count{};
  std::size_t coverage_delta_point_reference_count{};
  std::size_t fully_redundant_coverage_delta_count{};
  std::size_t residual_local_zero_point_delta_count{};
  std::size_t residual_canonical_ownerless_delta_count{};
  std::size_t logical_scratch_entry_count{};
  std::size_t logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectFrozenUnifiedIncidenceBatchCounters&,
      const ExactDirectFrozenUnifiedIncidenceBatchCounters&) = default;
};

enum class ExactDirectFrozenUnifiedIncidenceBatchDecision : std::uint8_t {
  not_certified,
  no_batch_capacity_overflow,
  no_batch_budget_exhausted,
  no_batch_source_plan_not_freshly_verified,
  no_batch_index_rejected,
  no_batch_facet_resolution_authority_rejected,
  no_batch_prior_root_coverage_authority_rejected,
  no_batch_latent_carrier_coverage_authority_rejected,
  no_batch_factorized_hyperedge_reconstruction_rejected,
  no_batch_frozen_incidence_quotient_rejected,
  no_batch_frozen_hgp_action_plan_rejected,
  no_batch_coverage_delta_scope_rejected,
  no_batch_allocation_failed,
  complete_certified_frozen_unified_incidence_batch,
};

enum class ExactDirectFrozenUnifiedIncidenceBatchScope : std::uint8_t {
  unspecified,
  exact_selected_batch_relative_to_supplied_successive_star_and_external_facet_resolution_prior_root_and_latent_carrier_coverage_authorities_only,
  exact_selected_batch_relative_to_verified_normalized_direct_source_and_external_facet_resolution_prior_root_and_latent_carrier_coverage_authorities_only,
};

struct ExactDirectFrozenUnifiedIncidenceBatchResult {
  static constexpr std::string_view backend =
      direct_frozen_unified_incidence_batch_backend;
  static constexpr std::string_view profile =
      direct_frozen_unified_incidence_batch_profile;
  static constexpr std::string_view mode =
      direct_frozen_unified_incidence_batch_mode;
  static constexpr std::string_view public_status =
      direct_frozen_unified_incidence_batch_public_status;
  static constexpr std::string_view proof_basis =
      direct_frozen_unified_incidence_batch_proof_basis;

  std::uint32_t schema_version{
      direct_frozen_unified_incidence_batch_schema_version};
  ExactDirectFrozenUnifiedIncidenceBatchBudget requested_budget{};
  std::size_t source_batch_index{};
  std::size_t source_future_snapshot_index{};
  exact::ExactLevel squared_level{};
  std::size_t order{};
  std::size_t required_facet_resolution_capacity{};
  std::size_t required_prior_root_coverage_capacity{};
  std::size_t required_prior_root_coverage_point_reference_capacity{};
  std::size_t required_latent_carrier_coverage_capacity{};
  std::size_t required_latent_carrier_coverage_point_reference_capacity{};
  std::size_t required_batch_direct_reference_scan_capacity{};
  std::size_t required_direct_saddle_hyperedge_capacity{};
  std::size_t required_residual_hyperedge_capacity{};
  std::size_t required_coface_facet_reference_scan_capacity{};
  std::size_t required_source_point_reference_scan_capacity{};
  std::size_t required_hyperedge_capacity{};
  std::size_t required_token_reference_capacity{};
  std::size_t required_distinct_typed_token_capacity{};
  std::size_t required_root_attachment_capacity{};
  std::size_t required_group_capacity{};
  std::size_t required_residual_incidence_record_capacity{};
  std::size_t required_equal_facet_binding_record_capacity{};
  std::size_t required_coverage_delta_record_capacity{};
  std::size_t required_coverage_delta_facet_reference_capacity{};
  std::size_t required_coverage_delta_point_reference_capacity{};
  std::size_t required_scratch_entry_capacity{};
  std::size_t required_logical_output_entry_capacity{};

  // These four aligned source arenas make the subordinate frozen quotient
  // and action plan independently replayable.  incidence_facet_token_indices
  // preserves the geometric facet identity discarded by the typed quotient.
  std::vector<ExactFrozenIncidenceHyperedge> quotient_hyperedges;
  std::vector<ExactFrozenIncidenceToken> quotient_token_references;
  std::vector<std::size_t> incidence_facet_token_indices;
  std::vector<ExactFrozenIncidenceHyperedgeProvenance> provenance;
  std::vector<ExactFrozenIncidenceRootAttachment> root_attachments;
  ExactFrozenIncidenceQuotientResult quotient;
  ExactFrozenIncidenceHgpActionPlanResult action_plan;
  std::vector<ExactDirectFrozenUnifiedResidualIncidenceRecord>
      residual_incidence_records;
  std::vector<ExactDirectFrozenUnifiedEqualFacetBindingRecord>
      equal_facet_binding_records;
  std::vector<ExactDirectFrozenUnifiedCoverageDeltaRecord>
      coverage_deltas;
  std::vector<ExactDirectFrozenUnifiedCoverageDeltaFacetReference>
      coverage_delta_facets;
  std::vector<ExactDirectFrozenUnifiedCoverageDeltaPointReference>
      coverage_delta_points;
  ExactDirectFrozenUnifiedIncidenceBatchCounters counters{};

  bool source_plan_freshly_verified{false};
  bool source_plan_verified_once_by_immutable_resident_authority{false};
  bool selected_batch_partition_freshly_replayed{false};
  bool direct_births_excluded_and_deferred{false};
  bool one_hyperedge_per_direct_saddle_and_residual{false};
  bool every_hyperedge_uses_all_factorized_deletions{false};
  bool facet_resolution_authority_canonical_and_exhaustive{false};
  bool typed_tokens_deduplicated_by_frozen_quotient{false};
  bool prior_root_coverage_csr_canonical_and_exhaustive{false};
  bool rooted_facets_covered_by_their_prior_roots{false};
  bool latent_carrier_coverage_csr_canonical_and_exhaustive{false};
  bool latent_facets_covered_by_their_carriers{false};
  bool frozen_quotient_freshly_streaming_verified{false};
  bool frozen_hgp_action_plan_freshly_streaming_verified{false};
  bool residual_incidence_records_canonical_and_exhaustive{false};
  bool equal_facet_bindings_canonical_and_exhaustive{false};
  bool coverage_deltas_are_exact_facet_and_point_set_differences{false};
  bool flat_delta_arenas_have_canonical_group_owners{false};
  bool all_output_within_budget{false};
  bool no_partial_scientific_payload_published{false};
  bool reducer_locator_forest_or_caller_state_mutated{false};
  bool successive_star_source_authority{false};
  bool normalized_direct_source_authority{false};
  bool global_facet_coface_or_gamma_catalog_materialized{false};
  bool supplied_star_global_completeness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectFrozenUnifiedIncidenceBatchDecision decision{
      ExactDirectFrozenUnifiedIncidenceBatchDecision::not_certified};
  ExactDirectFrozenUnifiedIncidenceBatchScope scope{
      ExactDirectFrozenUnifiedIncidenceBatchScope::unspecified};

  // This structural predicate cross-checks all retained arenas, including
  // equal-token bindings, q_R/actions and the true first facet owner.  Expanded
  // per-hyperedge point occurrences are intentionally not retained; scientific
  // payload trust therefore still requires the fresh authority verifier below.
  [[nodiscard]] bool certified_frozen_unified_incidence_batch()
      const noexcept;
  [[nodiscard]] bool atomic_empty_failure() const noexcept;

  friend bool operator==(
      const ExactDirectFrozenUnifiedIncidenceBatchResult&,
      const ExactDirectFrozenUnifiedIncidenceBatchResult&) = default;
};

[[nodiscard]] ExactDirectFrozenUnifiedIncidenceBatchResult
build_exact_direct_frozen_unified_incidence_batch(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget&
        source_star_budget,
    spatial::LbvhTraversalOrder source_star_traversal_order,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& source_star,
    const ExactDirectSparseUnifiedLevelPlanBudget& source_plan_budget,
    const ExactDirectSparseUnifiedLevelPlanResult& source_plan,
    std::size_t batch_index,
    std::span<const ExactDirectFrozenUnifiedFacetResolution>
        facet_resolutions,
    std::span<const ExactDirectFrozenUnifiedPriorRootCoverage>
        prior_root_coverages,
    std::span<const spatial::PointId> prior_root_coverage_point_references,
    std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage>
        latent_carrier_coverages,
    std::span<const spatial::PointId>
        latent_carrier_coverage_point_references,
    const ExactDirectFrozenUnifiedIncidenceBatchBudget& budget);

struct ExactDirectFrozenUnifiedIncidenceBatchVerification {
  bool requested_budget_certified{false};
  bool source_plan_freshly_verified{false};
  bool source_plan_immutable_resident_authority_certified{false};
  bool expected_result_freshly_reconstructed{false};
  bool supplied_latent_carrier_coverage_freshly_replayed{false};
  bool quotient_freshly_streaming_verified{false};
  bool action_plan_freshly_streaming_verified{false};
  bool observed_recursively_equal{false};
  bool result_facts_and_scope_certified{false};
  bool no_forbidden_global_structure_or_mutation{false};
  bool fresh_replay_certified{false};
  bool immutable_authority_batch_reconstruction_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectFrozenUnifiedIncidenceBatchVerification&,
      const ExactDirectFrozenUnifiedIncidenceBatchVerification&) = default;
};

[[nodiscard]] ExactDirectFrozenUnifiedIncidenceBatchVerification
verify_exact_direct_frozen_unified_incidence_batch(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget&
        source_star_budget,
    spatial::LbvhTraversalOrder source_star_traversal_order,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& source_star,
    const ExactDirectSparseUnifiedLevelPlanBudget& source_plan_budget,
    const ExactDirectSparseUnifiedLevelPlanResult& source_plan,
    std::size_t batch_index,
    std::span<const ExactDirectFrozenUnifiedFacetResolution>
        facet_resolutions,
    std::span<const ExactDirectFrozenUnifiedPriorRootCoverage>
        prior_root_coverages,
    std::span<const spatial::PointId> prior_root_coverage_point_references,
    std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage>
        latent_carrier_coverages,
    std::span<const spatial::PointId>
        latent_carrier_coverage_point_references,
    const ExactDirectFrozenUnifiedIncidenceBatchBudget& trusted_budget,
    const ExactDirectFrozenUnifiedIncidenceBatchResult& observed);

}  // namespace morsehgp3d::hierarchy
