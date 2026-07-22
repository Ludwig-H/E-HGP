#pragma once

#include "morsehgp3d/hierarchy/direct_closed_saddle_incidence_journal.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_first_incidence.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_sparse_gateway_candidate_journal_schema_version = 1U;
inline constexpr std::string_view
    direct_sparse_gateway_candidate_journal_backend = "reference_cpu";
inline constexpr std::string_view
    direct_sparse_gateway_candidate_journal_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_gateway_candidate_journal_mode = "certified";
inline constexpr std::string_view
    direct_sparse_gateway_candidate_journal_refinement_status =
        "partial_refinement";
inline constexpr std::string_view
    direct_sparse_gateway_candidate_journal_public_status = "not_claimed";
inline constexpr std::string_view
    direct_sparse_gateway_candidate_journal_proof_basis =
        "fresh_closed_saddle_deletion_replay_full_key_deduplication_one_"
        "exact_first_incidence_query_per_distinct_facet_atomic_positive_"
        "support_candidates_and_cardinality_level_batches_v1";

// Each cap is global to one journal build and is checked before the
// corresponding reserve, query or publication.  first_incidence_budget is
// applied independently, with the same values, to every distinct facet.
struct ExactDirectSparseGatewayCandidateBudget {
  std::size_t maximum_source_family_scan_count{};
  std::size_t maximum_deletion_reference_count{};
  std::size_t maximum_distinct_facet_count{};
  std::size_t maximum_facet_key_point_count{};
  std::size_t maximum_gateway_candidate_count{};
  std::size_t maximum_batch_count{};
  std::size_t maximum_batch_facet_reference_count{};
  std::size_t maximum_logical_storage_entry_count{};
  ExactDirectSparseFirstIncidenceBudget first_incidence_budget{};

  friend bool operator==(
      const ExactDirectSparseGatewayCandidateBudget&,
      const ExactDirectSparseGatewayCandidateBudget&) = default;
};

enum class ExactDirectSparseGatewayDeletionSource : std::uint8_t {
  unspecified,
  strict_arm_seed,
  equal_level_facet_seed,
};

enum class ExactDirectSparseGatewayLevelRelation : std::uint8_t {
  unspecified,
  first_incidence_strictly_below_saddle,
  first_incidence_equal_to_saddle,
};

// There is one projection per direct saddle deletion, including repeated
// full facet keys produced by distinct saddles.  Projections are sorted by
// full key and then by source provenance; the key itself lives once in the
// referenced token.
struct ExactDirectSparseGatewayDeletionProjection {
  std::size_t deletion_projection_index{};
  std::size_t source_family_index{};
  ExactDirectSparseGatewayDeletionSource source{
      ExactDirectSparseGatewayDeletionSource::unspecified};
  std::size_t source_deletion_index{};
  std::size_t source_event_index{};
  std::size_t source_order{};
  spatial::PointId removed_point_id{};
  std::size_t facet_token_index{};
  exact::ExactLevel saddle_squared_level{};
  ExactDirectSparseGatewayLevelRelation level_relation{
      ExactDirectSparseGatewayLevelRelation::unspecified};
  bool removed_point_is_first_incidence_cominimizer{false};

  friend bool operator==(
      const ExactDirectSparseGatewayDeletionProjection&,
      const ExactDirectSparseGatewayDeletionProjection&) = default;
};

// A token owns the complete full-key identity of one distinct deletion
// facet.  beta(F), lambda(F), the complete 10.6 audit and both contiguous
// projection/candidate ranges are retained without copying a coface key.
struct ExactDirectSparseGatewayFacetToken {
  std::size_t facet_token_index{};
  ExactDirectSparseFacetKey source_facet_key{};
  exact::ExactLevel source_miniball_squared_level{};
  exact::ExactLevel first_incidence_squared_level{};
  ExactDirectSparseFirstIncidenceAudit first_incidence_audit{};
  std::size_t deletion_projection_offset{};
  std::size_t deletion_projection_count{};
  std::size_t gateway_candidate_offset{};
  std::size_t gateway_candidate_count{};
  std::size_t batch_index{};

  friend bool operator==(
      const ExactDirectSparseGatewayFacetToken&,
      const ExactDirectSparseGatewayFacetToken&) = default;
};

// This factorized record is the exact identity F union {x}.  The selected
// positive support is copied from the certified 10.6 minimizer; no persistent
// eleven-PointId coface key is introduced at K=10.
struct ExactDirectSparseGatewayCandidateRecord {
  std::size_t gateway_candidate_index{};
  std::size_t facet_token_index{};
  spatial::PointId added_point_id{};
  std::array<spatial::PointId, 4U> positive_support_point_ids{};
  std::size_t positive_support_point_count{};
  bool added_point_in_source_closed_ball{false};
  bool added_point_in_selected_positive_support{false};

  friend bool operator==(
      const ExactDirectSparseGatewayCandidateRecord&,
      const ExactDirectSparseGatewayCandidateRecord&) = default;
};

// Tokens are partitioned by the canonical (facet cardinality, lambda) key.
// The separate index arena keeps token records in full-key order.
struct ExactDirectSparseGatewayCandidateBatch {
  std::size_t batch_index{};
  std::size_t facet_cardinality{};
  exact::ExactLevel first_incidence_squared_level{};
  std::size_t facet_token_index_offset{};
  std::size_t facet_token_index_count{};

  friend bool operator==(
      const ExactDirectSparseGatewayCandidateBatch&,
      const ExactDirectSparseGatewayCandidateBatch&) = default;
};

enum class ExactDirectSparseGatewayCandidateDecision : std::uint8_t {
  not_certified,
  no_gateway_candidate_capacity_overflow,
  no_gateway_candidate_budget_exhausted,
  no_gateway_candidate_source_not_certified,
  no_gateway_candidate_source_join_inconsistent,
  no_gateway_candidate_first_incidence_budget_exhausted,
  no_gateway_candidate_no_coface_contradiction,
  no_gateway_candidate_level_contradiction,
  complete_certified_sparse_gateway_candidates,
};

enum class ExactDirectSparseGatewayCandidateScope : std::uint8_t {
  unspecified,
  direct_saddle_deletion_facets_first_incidence_candidates_only,
};

struct ExactDirectSparseGatewayCandidateJournalResult {
  static constexpr std::string_view backend =
      direct_sparse_gateway_candidate_journal_backend;
  static constexpr std::string_view profile =
      direct_sparse_gateway_candidate_journal_profile;
  static constexpr std::string_view mode =
      direct_sparse_gateway_candidate_journal_mode;
  static constexpr std::string_view refinement_status =
      direct_sparse_gateway_candidate_journal_refinement_status;
  static constexpr std::string_view public_status =
      direct_sparse_gateway_candidate_journal_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_gateway_candidate_journal_proof_basis;

  std::uint32_t schema_version{
      direct_sparse_gateway_candidate_journal_schema_version};
  ExactDirectSparseGatewayCandidateBudget requested_budget{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  std::size_t point_count{};
  std::size_t source_direct_event_count{};
  std::size_t required_source_family_scan_count{};
  std::size_t required_deletion_reference_count{};
  std::size_t required_distinct_facet_count{};
  std::size_t required_facet_key_point_count{};
  std::size_t required_first_incidence_call_count{};
  std::size_t required_gateway_candidate_count{};
  std::size_t required_batch_count{};
  std::size_t required_batch_facet_reference_count{};
  std::size_t logical_storage_entry_count{};
  contract::CanonicalId source_pair_canonical_cloud_digest{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  contract::CanonicalId source_pair_semantic_digest{};
  contract::CanonicalId source_higher_semantic_digest{};

  // These are the only five scientific output arenas.
  std::vector<ExactDirectSparseGatewayDeletionProjection>
      deletion_projections;
  std::vector<ExactDirectSparseGatewayFacetToken> facet_tokens;
  std::vector<ExactDirectSparseGatewayCandidateRecord>
      gateway_candidates;
  std::vector<ExactDirectSparseGatewayCandidateBatch> batches;
  std::vector<std::size_t> batch_facet_token_indices;

  bool budget_preflight_certified{false};
  bool source_incidence_journal_freshly_replayed{false};
  bool every_strict_and_equal_deletion_reconstructed{false};
  bool deletion_references_sorted_by_full_key{false};
  bool distinct_full_keys_deduplicated{false};
  bool one_first_incidence_call_per_distinct_facet{false};
  bool every_first_incidence_complete{false};
  bool every_first_incidence_at_or_below_each_saddle{false};
  bool strict_and_equal_level_relations_classified{false};
  bool all_positive_support_candidates_retained_atomically{false};
  bool batches_canonical_and_partition_tokens{false};
  bool logical_storage_within_budget{false};
  bool output_linear_in_references_key_points_and_candidates{false};
  bool no_partial_scientific_payload_published{false};
  bool no_forbidden_global_structure_materialized{false};
  bool eleven_point_coface_keys_materialized{false};
  bool locator_or_quotient_consulted{false};
  bool root_union_or_forest_mutated{false};
  bool gateway_attach_published{false};
  bool gamma_cells_or_higher_order_delaunay_materialized{false};
  bool public_status_claimed{false};
  bool partial_refinement_only{false};
  ExactDirectSparseGatewayCandidateDecision decision{
      ExactDirectSparseGatewayCandidateDecision::not_certified};
  ExactDirectSparseGatewayCandidateScope scope{
      ExactDirectSparseGatewayCandidateScope::unspecified};

  [[nodiscard]] bool certified_partial_refinement() const;

  friend bool operator==(
      const ExactDirectSparseGatewayCandidateJournalResult&,
      const ExactDirectSparseGatewayCandidateJournalResult&) = default;
};

[[nodiscard]] ExactDirectSparseGatewayCandidateJournalResult
build_exact_direct_sparse_gateway_candidate_journal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseGatewayCandidateBudget& budget,
    spatial::LbvhTraversalOrder traversal_order =
        spatial::LbvhTraversalOrder::near_first);

// Rebuilds one of the k+1 deletion facets of F union {x} directly in the
// fixed ten-PointId key.  The transient merge uses an eleven-slot local array
// only; the journal never owns a coface key.
[[nodiscard]] ExactDirectSparseFacetKey
reconstruct_exact_direct_sparse_gateway_candidate_deletion_facet(
    const ExactDirectSparseGatewayCandidateJournalResult& journal,
    std::size_t gateway_candidate_index,
    std::size_t removed_union_point_index);

struct ExactDirectSparseGatewayCandidateVerification {
  bool observed_storage_within_budget{false};
  bool source_incidence_journal_freshly_replayed{false};
  bool deletion_projections_freshly_replayed{false};
  bool facet_tokens_freshly_replayed{false};
  bool gateway_candidates_freshly_replayed{false};
  bool batches_freshly_replayed{false};
  bool counters_and_result_facts_freshly_replayed{false};
  bool no_forbidden_global_structure_materialized{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectSparseGatewayCandidateVerification&,
      const ExactDirectSparseGatewayCandidateVerification&) = default;
};

// The five observed vector sizes are bounded before their records are scanned.
// Their key/support storage is then recomputed before any source replay.
// Expected output is rebuilt solely from external authorities and trusted
// budgets.
[[nodiscard]] ExactDirectSparseGatewayCandidateVerification
verify_exact_direct_sparse_gateway_candidate_journal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseGatewayCandidateBudget& trusted_budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& observed);

}  // namespace morsehgp3d::hierarchy
