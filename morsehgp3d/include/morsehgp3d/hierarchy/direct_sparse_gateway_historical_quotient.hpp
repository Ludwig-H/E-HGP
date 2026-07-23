#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_gateway_temporal_resolution.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_sparse_gateway_historical_quotient_schema_version = 1U;
inline constexpr std::string_view
    direct_sparse_gateway_historical_quotient_backend = "reference_cpu";
inline constexpr std::string_view
    direct_sparse_gateway_historical_quotient_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_gateway_historical_quotient_mode = "certified";
inline constexpr std::string_view
    direct_sparse_gateway_historical_quotient_refinement_status =
        "partial_refinement";
inline constexpr std::string_view
    direct_sparse_gateway_historical_quotient_public_status = "not_claimed";
inline constexpr std::string_view
    direct_sparse_gateway_historical_quotient_proof_basis =
        "batch_local_typed_historical_root_or_latent_facet_incidence_"
        "closure_and_exact_known_root_projection_v1";

// Population caps cover both output and the conservative preallocation
// bounds G<=C, R<=P and L<=P.  The comparison cap is shared by the three
// deterministic in-place heapsorts; DSU parent traversal and path-compression
// hops have their own explicit cap.  Scratch bytes cover only logical vector
// payload, not allocator metadata.
struct ExactDirectSparseGatewayHistoricalQuotientBudget {
  std::size_t maximum_batch_count{};
  std::size_t maximum_candidate_count{};
  std::size_t maximum_projection_count{};
  std::size_t maximum_temporal_resolution_count{};
  std::size_t maximum_incidence_scan_count{};
  std::size_t maximum_sort_comparison_count{};
  std::size_t maximum_sort_scratch_entry_count{};
  std::size_t maximum_candidate_dsu_entry_count{};
  std::size_t maximum_candidate_dsu_parent_hop_count{};
  std::size_t maximum_component_output_count{};
  std::size_t maximum_root_output_count{};
  std::size_t maximum_latent_output_count{};
  std::size_t maximum_logical_output_entry_count{};
  std::size_t maximum_temporary_scratch_byte_count{};

  friend bool operator==(
      const ExactDirectSparseGatewayHistoricalQuotientBudget&,
      const ExactDirectSparseGatewayHistoricalQuotientBudget&) = default;
};

enum class ExactDirectSparseGatewayHistoricalQuotientDisposition
    : std::uint8_t {
  not_certified,
  latent_only_unresolved,
  one_known_root_class,
  multiple_known_root_class,
};

struct ExactDirectSparseGatewayHistoricalQuotientBatch {
  std::size_t batch_index{};
  std::size_t strict_pre_locator_prefix_count{};
  std::size_t candidate_count{};
  std::size_t component_offset{};
  std::size_t component_count{};
  std::size_t rooted_component_count{};
  std::size_t latent_only_component_count{};

  friend bool operator==(
      const ExactDirectSparseGatewayHistoricalQuotientBatch&,
      const ExactDirectSparseGatewayHistoricalQuotientBatch&) = default;
};

// This arena remains in gateway-candidate order.  Projection ranges refer to
// the already published 10.9 arena and therefore copy neither a facet key nor
// a PointId.
struct ExactDirectSparseGatewayCandidateToHistoricalComponent {
  std::size_t gateway_candidate_index{};
  std::size_t source_batch_index{};
  std::size_t historical_component_index{};
  std::size_t deletion_projection_offset{};
  std::size_t deletion_projection_count{};
  std::size_t historical_positive_projection_count{};
  std::size_t historical_latent_projection_count{};

  friend bool operator==(
      const ExactDirectSparseGatewayCandidateToHistoricalComponent&,
      const ExactDirectSparseGatewayCandidateToHistoricalComponent&) =
      default;
};

// Components are ordered by (source_batch_index,
// representative_gateway_candidate_index).  Root and latent slices are
// disjoint, strictly increasing and explicit.  A latent id is a 10.13
// temporal-resolution index, hence the collision-free identity of the typed
// vertex L(prefix, localized-token); it is never cast to a root id.
struct ExactDirectSparseGatewayHistoricalComponent {
  std::size_t historical_component_index{};
  std::size_t source_batch_index{};
  std::size_t strict_pre_locator_prefix_count{};
  std::size_t representative_gateway_candidate_index{};
  std::size_t candidate_count{};
  std::size_t root_offset{};
  std::size_t root_count{};
  std::size_t latent_resolution_offset{};
  std::size_t latent_resolution_count{};
  ExactDirectSparseGatewayHistoricalQuotientDisposition disposition{
      ExactDirectSparseGatewayHistoricalQuotientDisposition::not_certified};

  friend bool operator==(
      const ExactDirectSparseGatewayHistoricalComponent&,
      const ExactDirectSparseGatewayHistoricalComponent&) = default;
};

struct ExactDirectSparseGatewayHistoricalQuotientCounters {
  std::size_t batch_scan_count{};
  std::size_t candidate_scan_count{};
  std::size_t projection_scan_count{};
  std::size_t incidence_scan_count{};
  std::size_t sort_comparison_count{};
  std::size_t candidate_union_count{};
  std::size_t candidate_dsu_parent_hop_count{};
  std::size_t component_count{};
  std::size_t rooted_component_count{};
  std::size_t latent_only_component_count{};
  std::size_t one_known_root_component_count{};
  std::size_t multiple_known_root_component_count{};
  std::size_t distinct_root_vertex_count{};
  std::size_t distinct_latent_vertex_count{};

  friend bool operator==(
      const ExactDirectSparseGatewayHistoricalQuotientCounters&,
      const ExactDirectSparseGatewayHistoricalQuotientCounters&) = default;
};

enum class ExactDirectSparseGatewayHistoricalQuotientDecision
    : std::uint8_t {
  not_certified,
  no_historical_quotient_capacity_overflow,
  no_historical_quotient_budget_exhausted,
  no_historical_quotient_allocation_failed,
  no_historical_quotient_source_rejected,
  complete_certified_batch_local_historical_root_quotient_proposal,
};

enum class ExactDirectSparseGatewayHistoricalQuotientScope : std::uint8_t {
  unspecified,
  supplied_gateway_candidates_batch_local_typed_incidence_known_root_projection_only,
};

struct ExactDirectSparseGatewayHistoricalQuotientResult {
  static constexpr std::string_view backend =
      direct_sparse_gateway_historical_quotient_backend;
  static constexpr std::string_view profile =
      direct_sparse_gateway_historical_quotient_profile;
  static constexpr std::string_view mode =
      direct_sparse_gateway_historical_quotient_mode;
  static constexpr std::string_view refinement_status =
      direct_sparse_gateway_historical_quotient_refinement_status;
  static constexpr std::string_view public_status =
      direct_sparse_gateway_historical_quotient_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_gateway_historical_quotient_proof_basis;

  std::uint32_t schema_version{
      direct_sparse_gateway_historical_quotient_schema_version};
  ExactDirectSparseGatewayHistoricalQuotientBudget requested_budget{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_snapshot_stamp{};
  std::size_t required_batch_count{};
  std::size_t required_candidate_count{};
  std::size_t required_projection_count{};
  std::size_t required_temporal_resolution_count{};
  std::size_t required_incidence_scan_count{};
  std::size_t required_sort_scratch_entry_count{};
  std::size_t required_candidate_dsu_entry_count{};
  std::size_t required_component_output_capacity{};
  std::size_t required_root_output_capacity{};
  std::size_t required_latent_output_capacity{};
  std::size_t required_logical_output_capacity{};
  std::size_t required_temporary_scratch_byte_count{};
  std::size_t logical_output_entry_count{};

  // These are the only five scientific output arenas.
  std::vector<ExactDirectSparseGatewayHistoricalQuotientBatch> batches;
  std::vector<ExactDirectSparseGatewayCandidateToHistoricalComponent>
      candidate_to_component;
  std::vector<ExactDirectSparseGatewayHistoricalComponent> components;
  std::vector<ExactDirectSparseComponentHandle> component_root_ids;
  std::vector<std::size_t> component_latent_resolution_indices;

  ExactDirectSparseGatewayHistoricalQuotientCounters counters{};
  bool source_and_temporal_resolution_prevalidated{false};
  bool budget_preflight_certified{false};
  bool every_candidate_partitioned_by_its_deletion_projections{false};
  bool every_projection_mapped_to_one_typed_vertex{false};
  bool typed_root_and_latent_namespaces_disjoint{false};
  bool closure_strictly_batch_local{false};
  bool shared_latent_facets_join_candidate_hyperedges{false};
  bool candidate_hyperedge_duplicates_are_idempotent{false};
  bool components_canonical_and_partition_candidates{false};
  bool known_root_projection_exact_for_supplied_candidates{false};
  bool latent_only_components_preserved_as_unresolved{false};
  bool roots_and_latent_resolution_slices_canonical{false};
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
  bool global_gateway_completeness_claimed{false};
  bool latent_facet_means_isolated{false};
  bool root_birth_continuation_or_multifusion_claimed{false};
  bool locator_root_union_or_forest_mutated{false};
  bool gateway_attach_published{false};
  bool gamma_cells_or_higher_order_delaunay_materialized{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  bool partial_refinement_only{true};
  ExactDirectSparseGatewayHistoricalQuotientDecision decision{
      ExactDirectSparseGatewayHistoricalQuotientDecision::not_certified};
  ExactDirectSparseGatewayHistoricalQuotientScope scope{
      ExactDirectSparseGatewayHistoricalQuotientScope::unspecified};

  // These predicates validate the self-contained output shape.  Scientific
  // consumers must additionally call the fresh verifier so that every typed
  // incidence and the batch-local closure are reconstructed from authorities.
  [[nodiscard]] bool certified_partial_refinement() const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectSparseGatewayHistoricalQuotientResult&,
      const ExactDirectSparseGatewayHistoricalQuotientResult&) = default;
};

struct ExactDirectSparseGatewayHistoricalQuotientVerification {
  ExactDirectSparseGatewayTemporalResolutionVerification
      temporal_resolution_verification{};
  bool authority_bundle_complete{false};
  bool observed_storage_within_budget{false};
  bool temporal_resolution_freshly_replayed{false};
  bool expected_result_freshly_reconstructed{false};
  bool observed_result_recursively_equal{false};
  bool batch_local_typed_closure_freshly_replayed{false};
  bool external_clock_authority_replayed{false};
  bool external_binding_authority_replayed{false};
  bool conditional_on_caller_external_binding_authority_replay{true};
  bool conditional_on_caller_strict_pre_lot_orchestration{true};
  bool external_freeze_synchronization_replayed{false};
  bool conditional_on_caller_external_freeze_synchronization{true};
  bool in_memory_replay_only{true};
  bool crash_durable{false};
  bool no_hgp_action_or_forbidden_global_structure_materialized{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectSparseGatewayHistoricalQuotientVerification&,
      const ExactDirectSparseGatewayHistoricalQuotientVerification&) =
      default;
};

[[nodiscard]] ExactDirectSparseGatewayHistoricalQuotientResult
build_exact_direct_sparse_gateway_historical_quotient(
    const ExactDirectSparseGatewayCandidateJournalResult& source,
    const ExactDirectSparseGatewayCandidateLocalizationResult& localization,
    const ExactDirectSparseGatewayTemporalResolutionResult&
        temporal_resolution,
    const ExactDirectSparseGatewayHistoricalQuotientBudget& budget);

[[nodiscard]] ExactDirectSparseGatewayHistoricalQuotientVerification
verify_exact_direct_sparse_gateway_historical_quotient(
    const ExactDirectSparseGatewayTemporalResolutionAuthorityBundle&
        authorities,
    const ExactDirectSparseGatewayTemporalResolutionBudget& temporal_budget,
    const ExactDirectSparsePositiveFacetPrefixSweepBudget&
        prefix_sweep_budget,
    const ExactDirectSparseGatewayTemporalResolutionResult&
        observed_temporal_resolution,
    const ExactDirectSparseGatewayHistoricalQuotientBudget& budget,
    const ExactDirectSparseGatewayHistoricalQuotientResult& observed);

}  // namespace morsehgp3d::hierarchy
