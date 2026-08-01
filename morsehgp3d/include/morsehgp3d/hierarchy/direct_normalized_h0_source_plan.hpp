#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_gateway_clock.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t direct_normalized_h0_source_plan_schema_version =
    1U;
inline constexpr std::string_view direct_normalized_h0_source_plan_backend =
    "reference_cpu";
inline constexpr std::string_view direct_normalized_h0_source_plan_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_normalized_h0_source_plan_mode =
    "certified_complete_candidate_source_plan_not_campaign_v3";
inline constexpr std::string_view
    direct_normalized_h0_source_plan_public_status = "not_claimed";
inline constexpr std::string_view direct_normalized_h0_source_plan_proof_basis =
    "fresh_sparse_gateway_candidate_replay_terminal_direct_event_union_"
    "canonical_factorized_coface_deduplication_and_exact_level_order_batches_"
    "without_successive_star_global_gamma_or_higher_delaunay_v1";

// The expensive per-facet first-incidence caps belong to the existing gateway
// candidate authority and are not duplicated here.  These caps cover only the
// linear integration pass and its normalized persistent arenas.
struct ExactDirectNormalizedH0SourcePlanBudget {
  std::size_t maximum_source_role_scan_count{};
  std::size_t maximum_source_gateway_token_scan_count{};
  std::size_t maximum_source_gateway_candidate_scan_count{};
  std::size_t maximum_distinct_coface_count{};
  std::size_t maximum_direct_coface_count{};
  std::size_t maximum_residual_coface_count{};
  std::size_t maximum_batch_count{};
  std::size_t maximum_direct_birth_reference_count{};
  std::size_t maximum_batch_coface_reference_count{};
  std::size_t maximum_logical_storage_entry_count{};
  ExactDirectSparseGatewayCandidateScientificIdentityBudget
      source_gateway_identity_budget{};

  friend bool operator==(
      const ExactDirectNormalizedH0SourcePlanBudget&,
      const ExactDirectNormalizedH0SourcePlanBudget&) = default;
};

enum class ExactDirectNormalizedH0SourceCofaceKind : std::uint8_t {
  direct_saddle,
  first_incidence_residual,
};

// This is a candidate-complete audit/integration plan, not the campaign-v3
// product source: all co-minimizers are retained so one facet can still map to
// several records.  The persistent (K+1)-coface identity is the first direct-
// core gateway facet contained in it plus one point.  The external gateway
// authority owns that facet key and must outlive consumers of this plan.
struct ExactDirectNormalizedH0SourceCoface {
  std::size_t coface_index{};
  std::size_t owner_source_gateway_token_index{};
  spatial::PointId added_point_id{};
  std::size_t order{};
  std::optional<std::size_t> source_role_record_index;
  std::optional<std::size_t> source_event_projection_index;
  std::optional<std::size_t> source_incidence_family_index;
  std::array<spatial::PointId, 4U> positive_support_point_ids{};
  std::size_t positive_support_point_count{};
  exact::ExactLevel squared_level{};
  std::size_t supplied_core_facet_occurrence_count{};
  std::size_t gateway_candidate_reference_offset{};
  std::size_t gateway_candidate_reference_count{};
  std::size_t batch_coface_reference_index{};
  std::size_t direct_family_match_count{};
  ExactDirectNormalizedH0SourceCofaceKind kind{
      ExactDirectNormalizedH0SourceCofaceKind::direct_saddle};
  bool observed_by_first_incidence_gateway{false};

  friend bool operator==(
      const ExactDirectNormalizedH0SourceCoface&,
      const ExactDirectNormalizedH0SourceCoface&) = default;
};

struct ExactDirectNormalizedH0SourceDirectBirthReference {
  std::size_t direct_birth_reference_index{};
  std::size_t source_role_record_index{};
  std::size_t source_event_projection_index{};
  ExactDirectSparseFacetKey birth_facet_key{};

  friend bool operator==(
      const ExactDirectNormalizedH0SourceDirectBirthReference&,
      const ExactDirectNormalizedH0SourceDirectBirthReference&) = default;
};

struct ExactDirectNormalizedH0SourceBatchCofaceReference {
  std::size_t batch_coface_reference_index{};
  std::size_t source_coface_index{};

  friend bool operator==(
      const ExactDirectNormalizedH0SourceBatchCofaceReference&,
      const ExactDirectNormalizedH0SourceBatchCofaceReference&) = default;
};

struct ExactDirectNormalizedH0GatewayCandidateReference {
  std::size_t gateway_candidate_reference_index{};
  std::size_t source_gateway_candidate_index{};
  std::size_t source_coface_index{};

  friend bool operator==(
      const ExactDirectNormalizedH0GatewayCandidateReference&,
      const ExactDirectNormalizedH0GatewayCandidateReference&) = default;
};

// Product order is ascending (exact squared level, order).  Direct births and
// all direct/first-incidence cofaces at one key share one normalized future
// snapshot.  This deliberately does not reuse the exhaustive v2 batch id.
struct ExactDirectNormalizedH0SourceBatch {
  std::size_t batch_index{};
  std::size_t future_snapshot_index{};
  exact::ExactLevel squared_level{};
  std::size_t order{};
  std::size_t direct_birth_reference_offset{};
  std::size_t direct_birth_reference_count{};
  std::size_t coface_reference_offset{};
  std::size_t coface_reference_count{};

  friend bool operator==(
      const ExactDirectNormalizedH0SourceBatch&,
      const ExactDirectNormalizedH0SourceBatch&) = default;
};

enum class ExactDirectNormalizedH0SourcePlanDecision : std::uint8_t {
  not_certified,
  no_source_capacity_overflow,
  no_source_allocation_failed,
  no_source_budget_exhausted,
  no_source_gateway_authority_rejected,
  no_source_join_inconsistent,
  complete_certified_direct_plus_first_incidence_source_plan,
};

enum class ExactDirectNormalizedH0SourcePlanScope : std::uint8_t {
  unspecified,
  complete_direct_events_and_complete_first_incidence_of_direct_core_facets_only,
};

struct ExactDirectNormalizedH0SourcePlanResult {
  static constexpr std::string_view backend =
      direct_normalized_h0_source_plan_backend;
  static constexpr std::string_view profile =
      direct_normalized_h0_source_plan_profile;
  static constexpr std::string_view mode =
      direct_normalized_h0_source_plan_mode;
  static constexpr std::string_view public_status =
      direct_normalized_h0_source_plan_public_status;
  static constexpr std::string_view proof_basis =
      direct_normalized_h0_source_plan_proof_basis;

  std::uint32_t schema_version{
      direct_normalized_h0_source_plan_schema_version};
  ExactDirectNormalizedH0SourcePlanBudget requested_budget{};
  ExactDirectSparseGatewayCandidateBudget source_gateway_budget{};
  spatial::LbvhTraversalOrder source_gateway_traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  std::size_t point_count{};
  std::size_t source_direct_event_count{};
  std::size_t source_event_projection_count{};
  std::size_t source_role_record_count{};
  std::size_t source_incidence_family_count{};
  std::size_t source_gateway_token_count{};
  std::size_t source_gateway_candidate_count{};
  std::size_t required_source_role_scan_count{};
  std::size_t required_source_gateway_token_scan_count{};
  std::size_t required_source_gateway_candidate_scan_count{};
  std::size_t excluded_order_one_role_count{};
  std::size_t excluded_order_one_gateway_candidate_count{};
  std::size_t required_higher_order_gateway_candidate_count{};
  std::size_t required_distinct_coface_count{};
  std::size_t required_direct_coface_count{};
  std::size_t required_residual_coface_count{};
  std::size_t required_batch_count{};
  std::size_t required_direct_birth_reference_count{};
  std::size_t required_batch_coface_reference_count{};
  std::size_t logical_storage_entry_count{};
  contract::CanonicalId source_pair_canonical_cloud_digest{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  contract::CanonicalId source_pair_semantic_digest{};
  contract::CanonicalId source_higher_semantic_digest{};
  contract::CanonicalId source_gateway_scientific_identity_digest{};

  // This CSR arena maps every higher-order per-facet gateway candidate to its
  // globally deduplicated coface.  References are sorted by coface, then by
  // source candidate.  Order-one candidates remain delegated to the Boruvka
  // authority.  Core keys and query audits stay in the freshly verified
  // external gateway journal and are not copied.
  std::vector<ExactDirectNormalizedH0GatewayCandidateReference>
      gateway_candidate_references;
  std::vector<ExactDirectNormalizedH0SourceCoface> cofaces;
  std::vector<ExactDirectNormalizedH0SourceDirectBirthReference>
      direct_birth_references;
  std::vector<ExactDirectNormalizedH0SourceBatchCofaceReference>
      batch_coface_references;
  std::vector<ExactDirectNormalizedH0SourceBatch> batches;

  bool source_gateway_journal_freshly_verified{false};
  bool source_gateway_scientific_identity_certified{false};
  bool order_one_roles_excluded_to_preserve_boruvka_authority{false};
  bool every_higher_order_direct_birth_projected_once{false};
  bool every_higher_order_direct_saddle_joined_once{false};
  bool order_one_gateway_candidates_excluded_to_preserve_boruvka_authority{
      false};
  bool every_higher_order_source_gateway_candidate_mapped_once{false};
  bool direct_union_complete_first_incidence_cominimizers_deduplicated{false};
  bool canonical_owner_is_lexicographically_first_core_facet{false};
  bool unique_batch_per_exact_level_and_order{false};
  bool batches_sorted_by_exact_level_then_order{false};
  bool exact_rational_levels_retained{false};
  bool logical_storage_within_budget{false};
  bool no_partial_scientific_payload_published{false};
  bool gateway_core_keys_and_audits_reused_without_copy{false};
  bool no_successive_incidence_star_materialized{false};
  bool no_global_star_facet_or_coface_catalog_materialized{false};
  bool no_gamma_or_higher_order_delaunay_materialized{false};
  bool transient_k_plus_one_keys_released_before_publication{false};
  bool persistent_k_plus_one_keys_materialized{false};
  bool hierarchy_or_forest_mutated{false};

  // Corollary 4.1 still needs a separate scalable global-regularity authority
  // over omitted Star cofaces.  Until one exists, neither no-op normalization
  // nor the Star-typed resident session can honestly be certified here.
  bool global_regularity_authority_certified{false};
  bool omitted_late_cofaces_qr1_noop_certified{false};
  bool normalized_noop_level_receipts_emitted{false};
  bool incidence_complete_reduction_proved{false};
  bool resident_atomic_fold_performed{false};
  bool contract_v2_identity_compatible{false};
  bool campaign_v3_single_gateway_per_core_facet_satisfied{false};
  bool campaign_v3_product_source_claimed{false};
  bool public_status_claimed{false};
  ExactDirectNormalizedH0SourcePlanDecision decision{
      ExactDirectNormalizedH0SourcePlanDecision::not_certified};
  ExactDirectNormalizedH0SourcePlanScope scope{
      ExactDirectNormalizedH0SourcePlanScope::unspecified};

  [[nodiscard]] bool certified_complete_candidate_source_plan()
      const noexcept;

  friend bool operator==(
      const ExactDirectNormalizedH0SourcePlanResult&,
      const ExactDirectNormalizedH0SourcePlanResult&) = default;
};

struct ExactDirectNormalizedH0ReconstructedCoface {
  std::array<spatial::PointId, 11U> point_ids{};
  std::size_t point_count{};

  friend bool operator==(
      const ExactDirectNormalizedH0ReconstructedCoface&,
      const ExactDirectNormalizedH0ReconstructedCoface&) = default;
};

[[nodiscard]] ExactDirectNormalizedH0ReconstructedCoface
reconstruct_exact_direct_normalized_h0_source_coface(
    const ExactDirectSparseGatewayCandidateJournalResult& source_gateway,
    // Compute this receipt once for source_gateway, then keep both immutable
    // while reconstructing any number of cofaces.  This keeps each hot-path
    // reconstruction O(K) instead of rehashing all five gateway arenas.
    const ExactDirectSparseGatewayCandidateScientificIdentityResult&
        source_gateway_identity,
    const ExactDirectNormalizedH0SourcePlanResult& source,
    std::size_t coface_index);

[[nodiscard]] ExactDirectNormalizedH0SourcePlanResult
build_exact_direct_normalized_h0_source_plan(
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
    spatial::LbvhTraversalOrder source_gateway_traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& source_gateway,
    const ExactDirectNormalizedH0SourcePlanBudget& budget);

struct ExactDirectNormalizedH0SourcePlanVerification {
  bool observed_storage_within_budget{false};
  bool source_gateway_journal_freshly_verified{false};
  bool expected_result_freshly_reconstructed{false};
  bool observed_recursively_equal{false};
  bool no_forbidden_global_structure_materialized{false};
  bool unsupported_noop_and_resident_claims_remain_false{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectNormalizedH0SourcePlanVerification&,
      const ExactDirectNormalizedH0SourcePlanVerification&) = default;
};

[[nodiscard]] ExactDirectNormalizedH0SourcePlanVerification
verify_exact_direct_normalized_h0_source_plan(
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
    spatial::LbvhTraversalOrder source_gateway_traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& source_gateway,
    const ExactDirectNormalizedH0SourcePlanBudget& budget,
    const ExactDirectNormalizedH0SourcePlanResult& observed);

}  // namespace morsehgp3d::hierarchy
