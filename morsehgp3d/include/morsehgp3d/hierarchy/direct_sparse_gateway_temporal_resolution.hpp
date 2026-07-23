#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_gateway_candidate_localization.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_gateway_clock_authority.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_positive_facet_prefix_sweep.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_sparse_gateway_temporal_resolution_schema_version = 1U;
inline constexpr std::string_view
    direct_sparse_gateway_temporal_resolution_backend = "reference_cpu";
inline constexpr std::string_view
    direct_sparse_gateway_temporal_resolution_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_gateway_temporal_resolution_mode = "certified";
inline constexpr std::string_view
    direct_sparse_gateway_temporal_resolution_refinement_status =
        "partial_refinement";
inline constexpr std::string_view
    direct_sparse_gateway_temporal_resolution_public_status = "not_claimed";
inline constexpr std::string_view
    direct_sparse_gateway_temporal_resolution_proof_basis =
        "sealed_source_batch_strict_pre_locator_prefix_cross_localized_token_"
        "bounded_pair_heapsort_prefix_sweep_and_append_only_monotonicity_v1";

struct ExactDirectSparseGatewayTemporalResolutionBudget {
  std::size_t maximum_source_batch_count{};
  std::size_t maximum_projection_count{};
  std::size_t maximum_localized_token_count{};
  std::size_t maximum_boundary_scan_count{};
  std::size_t maximum_localized_token_scan_count{};
  std::size_t maximum_projection_scan_count{};
  std::size_t maximum_resolution_scan_count{};
  std::size_t maximum_sort_comparison_count{};
  std::size_t maximum_sort_scratch_entry_count{};
  std::size_t maximum_prefix_query_scratch_entry_count{};
  std::size_t maximum_prefix_resolution_scratch_entry_count{};
  std::size_t maximum_temporary_scratch_byte_count{};
  std::size_t maximum_projection_output_count{};
  std::size_t maximum_temporal_resolution_output_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectSparseGatewayTemporalResolutionBudget&,
      const ExactDirectSparseGatewayTemporalResolutionBudget&) = default;
};

enum class ExactDirectSparseGatewayTemporalDisposition : std::uint8_t {
  not_certified,
  latent_unresolved,
  relative_positive,
};

struct ExactDirectSparseGatewayProjectionToTemporalResolution {
  std::size_t projection_index{};
  std::size_t temporal_resolution_index{};

  friend bool operator==(
      const ExactDirectSparseGatewayProjectionToTemporalResolution&,
      const ExactDirectSparseGatewayProjectionToTemporalResolution&) =
      default;
};

// No facet key is copied here.  It remains available by
// localized_facet_token_index in the freshly verified 10.9 arena.
struct ExactDirectSparseGatewayTemporalResolution {
  std::size_t temporal_resolution_index{};
  std::size_t strict_pre_locator_prefix_count{};
  std::size_t localized_facet_token_index{};
  std::size_t projection_reference_count{};
  ExactDirectSparseGatewayTemporalDisposition disposition{
      ExactDirectSparseGatewayTemporalDisposition::not_certified};
  ExactDirectSparseComponentHandle historical_component_root{};
  ExactDirectSparseFacetWitness source_binding_witness{};
  bool positive_payload_present{false};

  friend bool operator==(
      const ExactDirectSparseGatewayTemporalResolution&,
      const ExactDirectSparseGatewayTemporalResolution&) = default;
};

struct ExactDirectSparseGatewayTemporalResolutionCounters {
  std::size_t boundary_scan_count{};
  std::size_t localized_token_scan_count{};
  std::size_t projection_scan_count{};
  std::size_t resolution_scan_count{};
  std::size_t sort_comparison_count{};
  std::size_t distinct_prefix_token_pair_count{};
  std::size_t historical_positive_count{};
  std::size_t historical_latent_count{};
  std::size_t final_prefix_comparison_count{};
  std::size_t locator_snapshot_check_count{};

  friend bool operator==(
      const ExactDirectSparseGatewayTemporalResolutionCounters&,
      const ExactDirectSparseGatewayTemporalResolutionCounters&) = default;
};

enum class ExactDirectSparseGatewayTemporalResolutionDecision
    : std::uint8_t {
  not_certified,
  no_temporal_resolution_capacity_overflow,
  no_temporal_resolution_budget_exhausted,
  no_temporal_resolution_allocation_failed,
  no_temporal_resolution_source_rejected,
  no_temporal_resolution_prefix_sweep_rejected,
  no_temporal_resolution_append_only_contradiction,
  complete_certified_gateway_temporal_resolutions,
};

enum class ExactDirectSparseGatewayTemporalResolutionScope : std::uint8_t {
  unspecified,
  source_batch_strict_pre_locator_prefixes_cross_localized_deletion_facets_only,
};

struct ExactDirectSparseGatewayTemporalResolutionResult {
  static constexpr std::string_view backend =
      direct_sparse_gateway_temporal_resolution_backend;
  static constexpr std::string_view profile =
      direct_sparse_gateway_temporal_resolution_profile;
  static constexpr std::string_view mode =
      direct_sparse_gateway_temporal_resolution_mode;
  static constexpr std::string_view refinement_status =
      direct_sparse_gateway_temporal_resolution_refinement_status;
  static constexpr std::string_view public_status =
      direct_sparse_gateway_temporal_resolution_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_gateway_temporal_resolution_proof_basis;

  std::uint32_t schema_version{
      direct_sparse_gateway_temporal_resolution_schema_version};
  ExactDirectSparseGatewayTemporalResolutionBudget requested_budget{};
  ExactDirectSparsePositiveFacetPrefixSweepBudget
      requested_prefix_sweep_budget{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_snapshot_stamp{};
  std::size_t required_source_batch_count{};
  std::size_t required_projection_count{};
  std::size_t required_localized_token_count{};
  std::size_t required_boundary_scan_count{};
  std::size_t required_localized_token_scan_count{};
  std::size_t required_projection_scan_count{};
  std::size_t required_resolution_scan_count{};
  std::size_t required_sort_scratch_entry_count{};
  std::size_t required_prefix_query_scratch_entry_count{};
  std::size_t required_prefix_resolution_scratch_entry_count{};
  std::size_t required_temporary_scratch_byte_count{};
  std::size_t logical_output_entry_count{};

  // These are the only two published scientific output arenas.  No
  // crash-durable representation is claimed.
  std::vector<ExactDirectSparseGatewayProjectionToTemporalResolution>
      projection_to_resolution;
  std::vector<ExactDirectSparseGatewayTemporalResolution>
      temporal_resolutions;

  ExactDirectSparseGatewayTemporalResolutionCounters counters{};
  bool source_localization_and_sealed_authority_prevalidated{false};
  bool budget_preflight_certified{false};
  bool source_boundaries_dense_and_in_locator_history{false};
  bool every_projection_bound_to_exact_prefix_token_pair{false};
  bool pairs_heapsorted_by_prefix_token_projection{false};
  bool pairs_deduplicated_only_by_prefix_and_token{false};
  bool one_prefix_query_per_distinct_pair{false};
  bool prefix_sweep_completed{false};
  bool historical_positive_implies_final_positive_with_same_witness{false};
  bool final_latent_implies_historical_latent{false};
  bool final_prefix_payload_identical{false};
  bool every_latent_has_no_positive_payload{false};
  bool two_scientific_output_arenas_without_copied_facet_keys{false};
  bool common_frozen_locator_snapshot_certified{false};
  bool no_partial_scientific_payload_published{false};
  bool nested_verifiers_replayed{false};
  bool external_clock_authority_replayed{false};
  bool external_binding_authority_replayed{false};
  bool conditional_on_caller_external_binding_authority_replay{true};
  bool conditional_on_caller_strict_pre_lot_orchestration{true};
  bool external_freeze_synchronization_replayed{false};
  bool conditional_on_caller_external_freeze_synchronization{true};
  bool in_memory_replay_only{true};
  bool crash_durable{false};
  bool missing_facet_means_isolated{false};
  bool singleton_component_created{false};
  bool quotient_root_union_or_forest_mutated{false};
  bool gateway_attach_published{false};
  bool gamma_cells_or_higher_order_delaunay_materialized{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  bool partial_refinement_only{true};
  ExactDirectSparseGatewayTemporalResolutionDecision decision{
      ExactDirectSparseGatewayTemporalResolutionDecision::not_certified};
  ExactDirectSparseGatewayTemporalResolutionScope scope{
      ExactDirectSparseGatewayTemporalResolutionScope::unspecified};

  // These predicates validate the self-contained output shape.  Scientific
  // consumers must additionally call the fresh verifier with every original
  // authority so that the projection-to-pair mapping is reconstructed.
  [[nodiscard]] bool certified_partial_refinement() const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectSparseGatewayTemporalResolutionResult&,
      const ExactDirectSparseGatewayTemporalResolutionResult&) = default;
};

// Pointers are borrowed raw authorities, never an independently usable proof.
// Every pointee and the locator's backing spans must remain alive and
// externally frozen for the complete fresh verification call.
struct ExactDirectSparseGatewayTemporalResolutionAuthorityBundle {
  const spatial::MortonLbvhIndex* index{};
  const spatial::CanonicalPointCloud* cloud{};
  const ExactDirectSupportTerminalFacade* source_facade{};
  const ExactDirectMorseEventJournalResult* source_journal{};
  const ExactDirectSaddleArmSeedBudget* source_arm_budget{};
  const ExactDirectSaddleArmSeedJournalResult* source_arm_journal{};
  const ExactDirectClosedSaddleIncidenceBudget* source_incidence_budget{};
  const ExactDirectClosedSaddleIncidenceJournalResult*
      source_incidence_journal{};
  const ExactDirectSparseGatewayCandidateBudget* source_gateway_budget{};
  const ExactDirectSparseGatewayCandidateJournalResult*
      source_gateway_journal{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  const ExactDirectSparseFacetWitness* locator_query_witness{};
  const ExactDirectSparsePositiveFacetLocatorBudget* locator_budget{};
  const ExactDirectSparsePositiveFacetLocatorConfig* locator_config{};
  const ExactDirectSparsePositiveFacetLocator* locator{};
  std::size_t trusted_component_handle_count{};
  const ExactDirectSparseGatewayCandidateLocalizationBudget*
      localization_budget{};
  const ExactDirectSparseGatewayCandidateLocalizationResult*
      observed_localization{};
  const ExactDirectSparseGatewayClockAuthorityExternalSealAnchor*
      external_seal_anchor{};
  const ExactDirectSparseGatewayClockAuthorityJournalBudget*
      authority_journal_budget{};
  const ExactDirectSparseGatewayClockAuthorityJournal* observed_authority{};
  const ExactDirectSparseGatewayClockAuthorityVerificationBudget*
      authority_verification_budget{};
  const ExactDirectSparsePositiveFacetLocatorStructuralVerificationBudget*
      prefix_locator_structure_budget{};
};

struct ExactDirectSparseGatewayTemporalResolutionVerification {
  ExactDirectSparseGatewayCandidateLocalizationVerification
      localization_verification{};
  ExactDirectSparseGatewayClockAuthorityVerification
      clock_authority_verification{};
  ExactDirectSparsePositiveFacetPrefixSweepVerification
      prefix_sweep_verification{};
  bool authority_bundle_complete{false};
  bool observed_storage_within_budget{false};
  bool localization_freshly_replayed{false};
  bool clock_authority_freshly_replayed{false};
  bool prefix_sweep_freshly_replayed{false};
  bool expected_result_freshly_reconstructed{false};
  bool observed_result_recursively_equal{false};
  bool nested_verifiers_replayed{false};
  bool external_clock_authority_replayed{false};
  bool external_binding_authority_replayed{false};
  bool conditional_on_caller_external_binding_authority_replay{true};
  bool conditional_on_caller_strict_pre_lot_orchestration{true};
  bool external_freeze_synchronization_replayed{false};
  bool conditional_on_caller_external_freeze_synchronization{true};
  bool in_memory_replay_only{true};
  bool crash_durable{false};
  bool no_forbidden_global_structure_materialized{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectSparseGatewayTemporalResolutionVerification&,
      const ExactDirectSparseGatewayTemporalResolutionVerification&) =
      default;
};

[[nodiscard]] ExactDirectSparseGatewayTemporalResolutionResult
build_exact_direct_sparse_gateway_temporal_resolution(
    const ExactDirectSparseGatewayCandidateJournalResult& source,
    const ExactDirectSparseGatewayCandidateLocalizationResult& localization,
    const ExactDirectSparseGatewayClockAuthorityJournal& sealed_authority,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseGatewayTemporalResolutionBudget& budget,
    const ExactDirectSparsePositiveFacetPrefixSweepBudget&
        prefix_sweep_budget);

[[nodiscard]] ExactDirectSparseGatewayTemporalResolutionVerification
verify_exact_direct_sparse_gateway_temporal_resolution(
    const ExactDirectSparseGatewayTemporalResolutionAuthorityBundle&
        authorities,
    const ExactDirectSparseGatewayTemporalResolutionBudget& budget,
    const ExactDirectSparsePositiveFacetPrefixSweepBudget&
        prefix_sweep_budget,
    const ExactDirectSparseGatewayTemporalResolutionResult& observed);

}  // namespace morsehgp3d::hierarchy
