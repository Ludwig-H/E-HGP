#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_successive_incidence_star_journal.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t direct_sparse_unified_level_plan_schema_version =
    1U;
inline constexpr std::string_view direct_sparse_unified_level_plan_backend =
    "reference_cpu";
inline constexpr std::string_view direct_sparse_unified_level_plan_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_sparse_unified_level_plan_mode =
    "certified";
inline constexpr std::string_view
    direct_sparse_unified_level_plan_refinement_status = "partial_refinement";
inline constexpr std::string_view direct_sparse_unified_level_plan_public_status =
    "not_claimed";
inline constexpr std::string_view direct_sparse_unified_level_plan_proof_basis =
    "fresh_higher_order_direct_role_and_successive_star_replay_order_one_"
    "exclusion_to_boruvka_direct_saddle_to_star_coface_bijection_birth_and_"
    "coface_deletion_facet_token_deduplication_then_unique_exact_level_order_"
    "batches_with_direct_roles_residual_cofaces_and_complete_factorized_"
    "coface_deletion_references_v1";

// All caps are global to one plan.  They cover every scanned source record,
// every transient star coface point/deletion, and every persistent arena.
// No reducer state is consulted while deriving these requirements.
struct ExactDirectSparseUnifiedLevelPlanBudget {
  std::size_t maximum_source_role_scan_count{};
  std::size_t maximum_higher_order_direct_role_count{};
  std::size_t maximum_source_family_scan_count{};
  std::size_t maximum_higher_order_saddle_family_count{};
  std::size_t maximum_star_coface_scan_count{};
  std::size_t maximum_star_direct_coface_count{};
  std::size_t maximum_star_residual_coface_count{};
  std::size_t maximum_star_coface_point_scan_count{};
  std::size_t maximum_coface_deletion_reference_count{};
  std::size_t maximum_distinct_facet_token_count{};
  std::size_t maximum_facet_key_point_count{};
  std::size_t maximum_batch_count{};
  std::size_t maximum_direct_reference_count{};
  std::size_t maximum_residual_reference_count{};
  std::size_t maximum_logical_storage_entry_count{};

  friend bool operator==(
      const ExactDirectSparseUnifiedLevelPlanBudget&,
      const ExactDirectSparseUnifiedLevelPlanBudget&) = default;
};

struct ExactDirectSparseUnifiedLevelPlanFacetToken {
  std::size_t facet_token_index{};
  ExactDirectSparseFacetKey facet_key{};
  std::optional<std::size_t> source_star_facet_token_index;
  std::size_t direct_birth_reference_count{};
  std::size_t coface_deletion_reference_count{};

  friend bool operator==(
      const ExactDirectSparseUnifiedLevelPlanFacetToken&,
      const ExactDirectSparseUnifiedLevelPlanFacetToken&) = default;
};

struct ExactDirectSparseUnifiedLevelPlanDirectReference {
  std::size_t direct_reference_index{};
  std::size_t source_role_record_index{};
  std::size_t source_event_projection_index{};
  ExactDirectMorseH0Role role{ExactDirectMorseH0Role::birth};
  std::optional<std::size_t> source_incidence_family_index;
  std::optional<std::size_t> source_star_direct_coface_index;
  std::optional<std::size_t> direct_birth_facet_token_index;

  friend bool operator==(
      const ExactDirectSparseUnifiedLevelPlanDirectReference&,
      const ExactDirectSparseUnifiedLevelPlanDirectReference&) = default;
};

struct ExactDirectSparseUnifiedLevelPlanResidualReference {
  std::size_t residual_reference_index{};
  std::size_t source_star_coface_index{};

  friend bool operator==(
      const ExactDirectSparseUnifiedLevelPlanResidualReference&,
      const ExactDirectSparseUnifiedLevelPlanResidualReference&) = default;
};

// One record names one deletion of one factorized star coface.  The full
// K+1 key is reconstructed only transiently; the persistent target is the
// deduplicated K-facet token.
struct ExactDirectSparseUnifiedLevelPlanCofaceFacetReference {
  std::size_t coface_facet_reference_index{};
  std::size_t source_star_coface_index{};
  std::size_t removed_union_point_index{};
  spatial::PointId removed_point_id{};
  std::size_t facet_token_index{};

  friend bool operator==(
      const ExactDirectSparseUnifiedLevelPlanCofaceFacetReference&,
      const ExactDirectSparseUnifiedLevelPlanCofaceFacetReference&) = default;
};

// Product order is ascending (exact squared level, order).  Thus all direct
// and residual work for one order at one equality level shares exactly one
// planned future snapshot.  Different orders remain cardinality-isolated.
struct ExactDirectSparseUnifiedLevelPlanBatch {
  std::size_t batch_index{};
  std::size_t future_snapshot_index{};
  exact::ExactLevel squared_level{};
  std::size_t order{};
  std::size_t direct_reference_offset{};
  std::size_t direct_reference_count{};
  std::size_t residual_reference_offset{};
  std::size_t residual_reference_count{};
  std::size_t coface_facet_reference_offset{};
  std::size_t coface_facet_reference_count{};

  friend bool operator==(
      const ExactDirectSparseUnifiedLevelPlanBatch&,
      const ExactDirectSparseUnifiedLevelPlanBatch&) = default;
};

enum class ExactDirectSparseUnifiedLevelPlanDecision : std::uint8_t {
  not_certified,
  no_plan_capacity_overflow,
  no_plan_allocation_failed,
  no_plan_budget_exhausted,
  no_plan_source_star_not_certified,
  no_plan_source_join_inconsistent,
  no_plan_direct_star_crosscheck_contradiction,
  no_plan_batch_partition_contradiction,
  complete_bounded_unified_level_plan,
};

enum class ExactDirectSparseUnifiedLevelPlanScope : std::uint8_t {
  unspecified,
  bounded_unified_direct_and_residual_level_plan_of_supplied_higher_order_star_only,
};

struct ExactDirectSparseUnifiedLevelPlanResult {
  static constexpr std::string_view backend =
      direct_sparse_unified_level_plan_backend;
  static constexpr std::string_view profile =
      direct_sparse_unified_level_plan_profile;
  static constexpr std::string_view mode = direct_sparse_unified_level_plan_mode;
  static constexpr std::string_view refinement_status =
      direct_sparse_unified_level_plan_refinement_status;
  static constexpr std::string_view public_status =
      direct_sparse_unified_level_plan_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_unified_level_plan_proof_basis;

  std::uint32_t schema_version{direct_sparse_unified_level_plan_schema_version};
  ExactDirectSparseUnifiedLevelPlanBudget requested_budget{};
  ExactDirectSparseSuccessiveIncidenceStarJournalBudget source_star_budget{};
  spatial::LbvhTraversalOrder source_star_traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  std::size_t point_count{};
  std::size_t source_event_projection_count{};
  std::size_t source_role_record_count{};
  std::size_t source_incidence_family_count{};
  std::size_t source_star_facet_token_count{};
  std::size_t source_star_coface_count{};
  std::size_t required_source_role_scan_count{};
  std::size_t required_higher_order_direct_role_count{};
  std::size_t excluded_order_one_role_count{};
  std::size_t required_direct_birth_reference_count{};
  std::size_t required_direct_saddle_reference_count{};
  std::size_t required_source_family_scan_count{};
  std::size_t required_higher_order_saddle_family_count{};
  std::size_t excluded_order_one_family_count{};
  std::size_t required_star_coface_scan_count{};
  std::size_t required_star_direct_coface_count{};
  std::size_t required_star_residual_coface_count{};
  std::size_t required_star_coface_point_scan_count{};
  std::size_t required_direct_star_crosscheck_count{};
  std::size_t required_coface_deletion_reference_count{};
  std::size_t required_distinct_facet_token_count{};
  std::size_t required_facet_key_point_count{};
  std::size_t required_batch_count{};
  std::size_t required_direct_reference_count{};
  std::size_t required_residual_reference_count{};
  std::size_t logical_storage_entry_count{};
  contract::CanonicalId source_pair_canonical_cloud_digest{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  contract::CanonicalId source_pair_semantic_digest{};
  contract::CanonicalId source_higher_semantic_digest{};

  std::vector<ExactDirectSparseUnifiedLevelPlanFacetToken> facet_tokens;
  std::vector<ExactDirectSparseUnifiedLevelPlanDirectReference>
      direct_references;
  std::vector<ExactDirectSparseUnifiedLevelPlanResidualReference>
      residual_references;
  std::vector<ExactDirectSparseUnifiedLevelPlanCofaceFacetReference>
      coface_facet_references;
  std::vector<ExactDirectSparseUnifiedLevelPlanBatch> batches;

  bool source_star_freshly_verified{false};
  bool order_one_roles_and_families_excluded_to_preserve_boruvka_authority{
      false};
  bool every_higher_order_direct_role_projected_once{false};
  bool every_higher_order_saddle_role_joined_to_one_family{false};
  bool direct_star_cofaces_crosschecked_bijectively{false};
  bool star_direct_cofaces_are_crosscheck_only_not_residual_references{false};
  bool every_star_residual_coface_referenced_once{false};
  bool every_star_coface_contributes_all_factorized_deletions{false};
  bool facet_tokens_canonical_and_deduplicated{false};
  bool unique_batch_per_exact_level_and_order{false};
  bool direct_and_residual_same_level_order_share_one_future_snapshot{false};
  bool batches_sorted_by_exact_level_then_order{false};
  bool logical_storage_within_budget{false};
  bool no_partial_scientific_payload_published{false};
  bool no_k_plus_one_coface_key_persisted{false};
  bool no_global_facet_or_coface_catalog_materialized{false};
  bool reducer_snapshot_or_forest_mutated{false};
  bool bounded_star_global_completeness_claimed{false};
  bool public_status_claimed{false};
  bool partial_refinement_only{false};
  ExactDirectSparseUnifiedLevelPlanDecision decision{
      ExactDirectSparseUnifiedLevelPlanDecision::not_certified};
  ExactDirectSparseUnifiedLevelPlanScope scope{
      ExactDirectSparseUnifiedLevelPlanScope::unspecified};

  [[nodiscard]] bool certified_bounded_plan() const noexcept;

  friend bool operator==(
      const ExactDirectSparseUnifiedLevelPlanResult&,
      const ExactDirectSparseUnifiedLevelPlanResult&) = default;
};

[[nodiscard]] ExactDirectSparseUnifiedLevelPlanResult
build_exact_direct_sparse_unified_level_plan(
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
    const ExactDirectSparseUnifiedLevelPlanBudget& budget);

struct ExactDirectSparseUnifiedLevelPlanVerification {
  bool observed_storage_within_budget{false};
  bool source_star_freshly_verified{false};
  bool expected_result_freshly_reconstructed{false};
  bool observed_recursively_equal{false};
  bool no_forbidden_global_structure_materialized{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectSparseUnifiedLevelPlanVerification&,
      const ExactDirectSparseUnifiedLevelPlanVerification&) = default;
};

[[nodiscard]] ExactDirectSparseUnifiedLevelPlanVerification
verify_exact_direct_sparse_unified_level_plan(
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
    const ExactDirectSparseUnifiedLevelPlanBudget& budget,
    const ExactDirectSparseUnifiedLevelPlanResult& observed);

}  // namespace morsehgp3d::hierarchy
