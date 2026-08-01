#pragma once

#include "morsehgp3d/hierarchy/direct_closed_saddle_incidence_journal.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_successive_incidence.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_sparse_successive_incidence_star_journal_schema_version = 1U;
inline constexpr std::string_view
    direct_sparse_successive_incidence_star_journal_backend = "reference_cpu";
inline constexpr std::string_view
    direct_sparse_successive_incidence_star_journal_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_successive_incidence_star_journal_mode = "certified";
inline constexpr std::string_view
    direct_sparse_successive_incidence_star_journal_refinement_status =
        "partial_refinement";
inline constexpr std::string_view
    direct_sparse_successive_incidence_star_journal_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_sparse_successive_incidence_star_journal_proof_basis =
        "fresh_direct_saddle_strict_and_equal_deletion_replay_full_facet_"
        "order_one_exclusion_to_dedicated_boruvka_authority_"
        "deduplication_complete_successive_one_point_coface_shells_canonical_"
        "transient_union_owner_deduplication_exact_direct_family_join_and_"
        "factorized_residual_order_level_batches_v1";

// Scalar caps cover one complete star build.  The nested budget is applied
// independently to every successive-incidence query.  In particular, the
// call cap includes the terminal complete-no-later-coface query of every
// distinct supplied facet.
struct ExactDirectSparseSuccessiveIncidenceStarJournalBudget {
  std::size_t maximum_source_family_scan_count{};
  std::size_t maximum_deletion_reference_count{};
  std::size_t maximum_distinct_facet_count{};
  std::size_t maximum_facet_key_point_count{};
  std::size_t maximum_successive_incidence_call_count{};
  std::size_t maximum_incidence_shell_count{};
  std::size_t maximum_coface_occurrence_count{};
  std::size_t maximum_distinct_coface_count{};
  std::size_t maximum_direct_coface_count{};
  std::size_t maximum_residual_coface_count{};
  std::size_t maximum_residual_batch_count{};
  std::size_t maximum_residual_batch_reference_count{};
  std::size_t maximum_logical_storage_entry_count{};
  ExactDirectSparseSuccessiveIncidenceBudget successive_incidence_budget{};

  friend bool operator==(
      const ExactDirectSparseSuccessiveIncidenceStarJournalBudget&,
      const ExactDirectSparseSuccessiveIncidenceStarJournalBudget&) = default;
};

struct ExactDirectSparseSuccessiveIncidenceStarFacetToken {
  std::size_t facet_token_index{};
  ExactDirectSparseFacetKey source_facet_key{};
  exact::ExactLevel source_miniball_squared_level{};
  std::size_t source_deletion_reference_count{};
  std::size_t shell_offset{};
  std::size_t shell_count{};
  std::size_t canonical_owner_coface_count{};
  ExactDirectSparseSuccessiveIncidenceAudit terminal_query_audit{};
  bool complete_no_later_coface{false};

  friend bool operator==(
      const ExactDirectSparseSuccessiveIncidenceStarFacetToken&,
      const ExactDirectSparseSuccessiveIncidenceStarFacetToken&) = default;
};

// One record represents one exact level returned for one supplied facet.
// Cofaces themselves are not copied into this arena: minimizer_count only
// accounts for the complete atomic shell supplied to the transient owner
// deduplication pass.
struct ExactDirectSparseSuccessiveIncidenceStarShell {
  std::size_t shell_index{};
  std::size_t facet_token_index{};
  std::size_t successive_query_index{};
  exact::ExactLevel squared_level{};
  std::size_t minimizer_count{};
  ExactDirectSparseSuccessiveIncidenceAudit query_audit{};

  friend bool operator==(
      const ExactDirectSparseSuccessiveIncidenceStarShell&,
      const ExactDirectSparseSuccessiveIncidenceStarShell&) = default;
};

enum class ExactDirectSparseSuccessiveIncidenceStarCofaceKind : std::uint8_t {
  direct_family,
  residual,
};

// This is the sole persistent coface identity: owner facet F plus x.  The
// sorted union F union {x} exists only transiently while selecting the
// canonical owner and while freshly verifying this record.
struct ExactDirectSparseSuccessiveIncidenceStarCoface {
  std::size_t coface_index{};
  std::size_t owner_facet_token_index{};
  spatial::PointId added_point_id{};
  std::array<spatial::PointId, 4U> positive_support_point_ids{};
  std::size_t positive_support_point_count{};
  exact::ExactLevel squared_level{};
  std::size_t supplied_facet_occurrence_count{};
  std::size_t direct_family_match_count{};
  ExactDirectSparseSuccessiveIncidenceStarCofaceKind kind{
      ExactDirectSparseSuccessiveIncidenceStarCofaceKind::direct_family};
  bool added_point_in_source_closed_ball{false};
  bool added_point_in_selected_positive_support{false};

  friend bool operator==(
      const ExactDirectSparseSuccessiveIncidenceStarCoface&,
      const ExactDirectSparseSuccessiveIncidenceStarCoface&) = default;
};

// Only residual cofaces enter batches.  The separate index arena preserves
// canonical coface order while grouping by (facet cardinality, exact level).
struct ExactDirectSparseSuccessiveIncidenceStarResidualBatch {
  std::size_t batch_index{};
  std::size_t order{};
  exact::ExactLevel squared_level{};
  std::size_t residual_coface_index_offset{};
  std::size_t residual_coface_index_count{};

  friend bool operator==(
      const ExactDirectSparseSuccessiveIncidenceStarResidualBatch&,
      const ExactDirectSparseSuccessiveIncidenceStarResidualBatch&) = default;
};

enum class ExactDirectSparseSuccessiveIncidenceStarJournalDecision :
    std::uint8_t {
  not_certified,
  no_star_capacity_overflow,
  no_star_allocation_failed,
  no_star_budget_exhausted,
  no_star_source_not_certified,
  no_star_source_join_inconsistent,
  no_star_successive_incidence_budget_exhausted,
  no_star_successive_incidence_contradiction,
  no_star_direct_family_join_contradiction,
  complete_bounded_successive_incidence_star_journal,
};

enum class ExactDirectSparseSuccessiveIncidenceStarJournalScope :
    std::uint8_t {
  unspecified,
  bounded_successive_incidence_star_of_supplied_direct_higher_order_facets_only,
};

struct ExactDirectSparseSuccessiveIncidenceStarJournalResult {
  static constexpr std::string_view backend =
      direct_sparse_successive_incidence_star_journal_backend;
  static constexpr std::string_view profile =
      direct_sparse_successive_incidence_star_journal_profile;
  static constexpr std::string_view mode =
      direct_sparse_successive_incidence_star_journal_mode;
  static constexpr std::string_view refinement_status =
      direct_sparse_successive_incidence_star_journal_refinement_status;
  static constexpr std::string_view public_status =
      direct_sparse_successive_incidence_star_journal_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_successive_incidence_star_journal_proof_basis;

  std::uint32_t schema_version{
      direct_sparse_successive_incidence_star_journal_schema_version};
  ExactDirectSparseSuccessiveIncidenceStarJournalBudget requested_budget{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  std::size_t point_count{};
  std::size_t source_direct_event_count{};
  std::size_t required_source_family_scan_count{};
  std::size_t required_higher_order_family_count{};
  std::size_t excluded_order_one_family_count{};
  std::size_t required_deletion_reference_count{};
  std::size_t required_distinct_facet_count{};
  std::size_t required_facet_key_point_count{};
  std::size_t required_successive_incidence_call_count{};
  // Sum of the seven operation counters controlled directly by the child
  // budget, across every shell and terminal query.  The limit is derived
  // before traversal as maximum_call_count times the corresponding sum of
  // child caps, with checked arithmetic.
  std::size_t aggregate_successive_work_entry_count{};
  std::size_t aggregate_successive_work_entry_limit{};
  std::size_t required_incidence_shell_count{};
  std::size_t required_coface_occurrence_count{};
  std::size_t required_distinct_coface_count{};
  std::size_t required_direct_coface_count{};
  std::size_t required_residual_coface_count{};
  std::size_t required_residual_batch_count{};
  std::size_t required_residual_batch_reference_count{};
  std::size_t logical_storage_entry_count{};
  contract::CanonicalId source_pair_canonical_cloud_digest{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  contract::CanonicalId source_pair_semantic_digest{};
  contract::CanonicalId source_higher_semantic_digest{};

  std::vector<ExactDirectSparseSuccessiveIncidenceStarFacetToken>
      facet_tokens;
  std::vector<ExactDirectSparseSuccessiveIncidenceStarShell> shells;
  std::vector<ExactDirectSparseSuccessiveIncidenceStarCoface> cofaces;
  std::vector<ExactDirectSparseSuccessiveIncidenceStarResidualBatch>
      residual_batches;
  std::vector<std::size_t> residual_batch_coface_indices;

  bool budget_preflight_certified{false};
  bool source_incidence_journal_freshly_replayed{false};
  bool order_one_families_excluded_to_preserve_boruvka_authority{false};
  bool every_strict_and_equal_deletion_reconstructed{false};
  bool distinct_source_facets_deduplicated{false};
  bool every_facet_queried_through_complete_no_later_coface{false};
  bool aggregate_successive_work_within_derived_limit{false};
  bool successive_shell_levels_strictly_increasing{false};
  bool all_equal_level_minimizers_retained_atomically{false};
  bool every_factorized_occurrence_deduplicated_once{false};
  bool canonical_owner_is_lexicographically_first_supplied_facet{false};
  bool every_coface_joined_exactly_to_direct_families{false};
  bool residual_batches_canonical_and_complete_for_this_star{false};
  bool logical_storage_within_budget{false};
  bool no_partial_scientific_payload_published{false};
  bool no_global_facet_or_coface_catalog_materialized{false};
  bool transient_union_keys_released_before_publication{false};
  bool persistent_k_plus_one_keys_materialized{false};
  bool gamma_cells_or_higher_order_delaunay_materialized{false};
  bool hierarchy_or_forest_mutated{false};
  bool global_gamma_completeness_claimed{false};
  bool product_sparse_silent_source_complete{false};
  bool public_status_claimed{false};
  bool partial_refinement_only{false};
  ExactDirectSparseSuccessiveIncidenceStarJournalDecision decision{
      ExactDirectSparseSuccessiveIncidenceStarJournalDecision::not_certified};
  ExactDirectSparseSuccessiveIncidenceStarJournalScope scope{
      ExactDirectSparseSuccessiveIncidenceStarJournalScope::unspecified};

  [[nodiscard]] bool certified_bounded_star() const noexcept;

  friend bool operator==(
      const ExactDirectSparseSuccessiveIncidenceStarJournalResult&,
      const ExactDirectSparseSuccessiveIncidenceStarJournalResult&) = default;
};

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceStarJournalResult
build_exact_direct_sparse_successive_incidence_star_journal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget& budget,
    spatial::LbvhTraversalOrder traversal_order =
        spatial::LbvhTraversalOrder::near_first);

// On-demand reconstruction for tests and downstream exact joins.  The
// returned fixed-size value is not retained by the journal.
struct ExactDirectSparseSuccessiveIncidenceStarReconstructedCoface {
  std::array<spatial::PointId, 11U> point_ids{};
  std::size_t point_count{};

  friend bool operator==(
      const ExactDirectSparseSuccessiveIncidenceStarReconstructedCoface&,
      const ExactDirectSparseSuccessiveIncidenceStarReconstructedCoface&) =
      default;
};

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceStarReconstructedCoface
reconstruct_exact_direct_sparse_successive_incidence_star_coface(
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& journal,
    std::size_t coface_index);

struct ExactDirectSparseSuccessiveIncidenceStarJournalVerification {
  bool observed_storage_within_budget{false};
  bool source_incidence_journal_freshly_replayed{false};
  bool expected_result_freshly_reconstructed{false};
  bool observed_recursively_equal{false};
  bool no_forbidden_global_structure_materialized{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectSparseSuccessiveIncidenceStarJournalVerification&,
      const ExactDirectSparseSuccessiveIncidenceStarJournalVerification&) =
      default;
};

[[nodiscard]]
ExactDirectSparseSuccessiveIncidenceStarJournalVerification
verify_exact_direct_sparse_successive_incidence_star_journal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget& budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& observed);

}  // namespace morsehgp3d::hierarchy
