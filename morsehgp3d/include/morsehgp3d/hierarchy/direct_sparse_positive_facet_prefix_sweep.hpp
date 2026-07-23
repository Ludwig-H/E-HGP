#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_positive_facet_locator.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_sparse_positive_facet_prefix_sweep_schema_version = 1U;
inline constexpr std::string_view
    direct_sparse_positive_facet_prefix_sweep_backend = "reference_cpu";
inline constexpr std::string_view
    direct_sparse_positive_facet_prefix_sweep_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_positive_facet_prefix_sweep_mode = "certified";
inline constexpr std::string_view
    direct_sparse_positive_facet_prefix_sweep_refinement_status =
        "partial_refinement";
inline constexpr std::string_view
    direct_sparse_positive_facet_prefix_sweep_public_status = "not_claimed";
inline constexpr std::string_view
    direct_sparse_positive_facet_prefix_sweep_proof_basis =
        "fixed_table_chronological_binding_prefix_and_deterministic_union_"
        "prefix_monotone_read_only_sweep_v1";

// Queries must be dense in input order and nondecreasing by committed prefix.
// Prefix p denotes the state after the first p committed locator batches; for
// p<T it is exactly the strict pre-batch state of committed batch p.
struct ExactDirectSparsePositiveFacetPrefixQuery {
  std::size_t query_index{};
  std::size_t committed_batch_prefix_count{};
  ExactDirectSparseFacetKey facet_key{};

  friend bool operator==(
      const ExactDirectSparsePositiveFacetPrefixQuery&,
      const ExactDirectSparsePositiveFacetPrefixQuery&) = default;
};

struct ExactDirectSparsePositiveFacetPrefixSweepBudget {
  std::size_t maximum_query_count{};
  std::size_t maximum_query_key_point_count{};
  std::size_t maximum_component_handle_scratch_count{};
  std::size_t maximum_batch_record_scan_count{};
  std::size_t maximum_union_record_replay_count{};
  std::size_t maximum_union_replay_parent_hop_count{};
  std::size_t maximum_aggregate_slot_visit_count{};
  std::size_t maximum_aggregate_query_parent_hop_count{};
  std::size_t maximum_logical_output_entry_count{};
  ExactDirectSparsePositiveFacetProbeBudget facet_probe_budget{};

  friend bool operator==(
      const ExactDirectSparsePositiveFacetPrefixSweepBudget&,
      const ExactDirectSparsePositiveFacetPrefixSweepBudget&) = default;
};

enum class ExactDirectSparsePositiveFacetPrefixDisposition : std::uint8_t {
  not_certified,
  latent_unresolved,
  relative_positive,
};

struct ExactDirectSparsePositiveFacetPrefixResolution {
  std::size_t query_index{};
  std::size_t committed_batch_prefix_count{};
  ExactDirectSparseComponentHandle component_handle{};
  ExactDirectSparseFacetWitness source_binding_witness{};
  bool component_handle_present{false};
  bool source_binding_witness_present{false};
  ExactDirectSparsePositiveFacetPrefixDisposition disposition{
      ExactDirectSparsePositiveFacetPrefixDisposition::not_certified};

  friend bool operator==(
      const ExactDirectSparsePositiveFacetPrefixResolution&,
      const ExactDirectSparsePositiveFacetPrefixResolution&) = default;
};

struct ExactDirectSparsePositiveFacetPrefixSweepCounters {
  std::size_t query_scan_count{};
  std::size_t query_key_point_count{};
  std::size_t component_handle_initialization_count{};
  std::size_t batch_record_scan_count{};
  std::size_t union_record_replay_count{};
  std::size_t union_replay_parent_hop_count{};
  std::size_t query_resolution_count{};
  std::size_t slot_visit_count{};
  std::size_t query_parent_hop_count{};
  std::size_t full_key_comparison_count{};
  std::size_t equal_fingerprint_distinct_key_count{};
  std::size_t future_binding_terminator_count{};
  std::size_t relative_positive_count{};
  std::size_t latent_unresolved_count{};
  std::size_t maximum_single_query_slot_visit_count{};
  std::size_t maximum_single_query_parent_hop_count{};
  std::size_t locator_snapshot_check_count{};

  friend bool operator==(
      const ExactDirectSparsePositiveFacetPrefixSweepCounters&,
      const ExactDirectSparsePositiveFacetPrefixSweepCounters&) = default;
};

enum class ExactDirectSparsePositiveFacetPrefixSweepDecision : std::uint8_t {
  not_certified,
  no_prefix_sweep_capacity_overflow,
  no_prefix_sweep_budget_exhausted,
  no_prefix_sweep_input_shape_rejected,
  no_prefix_sweep_locator_history_not_certified,
  no_prefix_sweep_probe_budget_exhausted,
  complete_certified_positive_facet_prefix_sweep,
};

enum class ExactDirectSparsePositiveFacetPrefixSweepScope : std::uint8_t {
  unspecified,
  locator_internal_committed_batch_prefixes_relative_to_frozen_positive_domain_only,
};

struct ExactDirectSparsePositiveFacetPrefixSweepResult {
  static constexpr std::string_view backend =
      direct_sparse_positive_facet_prefix_sweep_backend;
  static constexpr std::string_view profile =
      direct_sparse_positive_facet_prefix_sweep_profile;
  static constexpr std::string_view mode =
      direct_sparse_positive_facet_prefix_sweep_mode;
  static constexpr std::string_view refinement_status =
      direct_sparse_positive_facet_prefix_sweep_refinement_status;
  static constexpr std::string_view public_status =
      direct_sparse_positive_facet_prefix_sweep_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_positive_facet_prefix_sweep_proof_basis;

  std::uint32_t schema_version{
      direct_sparse_positive_facet_prefix_sweep_schema_version};
  ExactDirectSparsePositiveFacetPrefixSweepBudget requested_budget{};
  ExactDirectSparseFacetWitness locator_query_witness{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_snapshot_stamp{};
  std::size_t source_query_count{};
  std::size_t required_query_key_point_count{};
  std::size_t required_component_handle_scratch_count{};
  std::size_t required_committed_batch_prefix_count{};
  std::size_t required_batch_record_scan_count{};
  std::size_t required_active_binding_prefix_count{};
  std::size_t required_union_record_replay_count{};
  std::size_t logical_output_entry_count{};
  std::vector<ExactDirectSparsePositiveFacetPrefixResolution> resolutions;
  ExactDirectSparsePositiveFacetPrefixSweepCounters counters{};

  bool locator_certified_at_entry{false};
  bool budget_preflight_certified{false};
  bool queries_canonical_and_prefix_monotone{false};
  bool requested_locator_history_records_well_formed{false};
  bool dense_identity_dsu_scratch_initialized{false};
  bool each_batch_and_union_prefix_replayed_once{false};
  bool future_binding_slots_are_historical_terminators{false};
  bool every_fingerprint_candidate_compared_by_full_key{false};
  bool every_query_resolved_once{false};
  bool positive_and_latent_outcomes_separated{false};
  bool every_positive_has_historical_root_and_original_witness{false};
  bool every_latent_has_no_positive_payload{false};
  bool common_frozen_locator_snapshot_certified{false};
  bool no_partial_scientific_payload_published{false};
  bool locator_state_mutated{false};
  bool locator_batch_committed{false};
  bool external_binding_authority_replayed{false};
  bool source_batch_alignment_claimed{false};
  bool missing_facet_means_isolated{false};
  bool singleton_component_created{false};
  bool quotient_root_union_or_forest_mutated{false};
  bool gateway_attach_published{false};
  bool gamma_cells_or_higher_order_delaunay_materialized{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  bool partial_refinement_only{false};
  ExactDirectSparsePositiveFacetPrefixSweepDecision decision{
      ExactDirectSparsePositiveFacetPrefixSweepDecision::not_certified};
  ExactDirectSparsePositiveFacetPrefixSweepScope scope{
      ExactDirectSparsePositiveFacetPrefixSweepScope::unspecified};

  // Self-contained shape predicates only.  Scientific consumers must call
  // the fresh verifier with the live frozen locator and original queries.
  [[nodiscard]] bool certified_partial_refinement() const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectSparsePositiveFacetPrefixSweepResult&,
      const ExactDirectSparsePositiveFacetPrefixSweepResult&) = default;
};

struct ExactDirectSparsePositiveFacetPrefixSweepVerification {
  ExactDirectSparsePositiveFacetLocatorStructuralVerification
      locator_structural_verification{};
  bool trusted_live_locator_and_witness_certified{false};
  bool observed_storage_within_budget{false};
  bool locator_snapshot_matches_observed_build{false};
  bool locator_verification_budget_preflight_certified{false};
  bool locator_verification_budget_respected{false};
  bool locator_durable_structure_freshly_verified{false};
  bool committed_slot_insertion_chronology_freshly_replayed{false};
  bool observed_outcome_well_formed{false};
  bool queries_and_prefixes_freshly_replayed{false};
  bool union_prefixes_and_historical_roots_freshly_replayed{false};
  bool historical_slot_probes_freshly_replayed{false};
  bool counters_and_result_facts_freshly_replayed{false};
  bool no_locator_mutation_or_batch_commit{false};
  bool external_binding_authority_replayed{false};
  bool source_batch_alignment_replayed{false};
  bool no_isolation_singleton_quotient_forest_or_attachment_invented{false};
  bool no_forbidden_global_structure_materialized{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectSparsePositiveFacetPrefixSweepVerification&,
      const ExactDirectSparsePositiveFacetPrefixSweepVerification&) = default;
};

// The locator and every span backing its state must remain externally frozen
// for the whole build.  Prefixes are locator commit-clock prefixes only.
[[nodiscard]] ExactDirectSparsePositiveFacetPrefixSweepResult
build_exact_direct_sparse_positive_facet_prefix_sweep(
    std::span<const ExactDirectSparsePositiveFacetPrefixQuery> queries,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparsePositiveFacetPrefixSweepBudget& budget);

// Replays the complete durable locator structure under its separate explicit
// budget, including physical slot insertion chronology, then reconstructs the
// requested monotone sweep.  A structural-budget exhaustion returns before
// the sweep reconstruction and is not classified as malformed durable state.
[[nodiscard]] ExactDirectSparsePositiveFacetPrefixSweepVerification
verify_exact_direct_sparse_positive_facet_prefix_sweep(
    std::span<const ExactDirectSparsePositiveFacetPrefixQuery> queries,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparsePositiveFacetPrefixSweepBudget& budget,
    const ExactDirectSparsePositiveFacetLocatorStructuralVerificationBudget&
        locator_verification_budget,
    const ExactDirectSparsePositiveFacetPrefixSweepResult& observed);

}  // namespace morsehgp3d::hierarchy
