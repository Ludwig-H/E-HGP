#pragma once

// Archived Phase 15 prototype; excluded from the active API and build.

#include "morsehgp3d/hierarchy/direct_normalized_h0_source_plan.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_normalized_h0_retraction_authority_schema_version = 1U;
inline constexpr std::string_view
    direct_normalized_h0_retraction_authority_backend = "reference_cpu";
inline constexpr std::string_view
    direct_normalized_h0_retraction_authority_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_normalized_h0_retraction_authority_mode =
        "certified_normalized_higher_order_horizontal_h0_retraction";
inline constexpr std::string_view
    direct_normalized_h0_retraction_authority_public_status = "not_claimed";
inline constexpr std::string_view
    direct_normalized_h0_retraction_authority_proof_basis =
        "fresh_direct_plus_complete_first_incidence_source_plan_replay_"
        "fresh_rank_window_saturated_h0_authority_replay_and_corollaries_"
        "4_1_4_2_for_atomic_normalized_horizontal_orders_two_through_"
        "effective_maximum_without_global_regularity_or_forest_fold_v1";

enum class ExactDirectNormalizedH0RetractionDecision : std::uint8_t {
  not_certified,
  no_retraction_rank_window_authority_rejected,
  no_retraction_unsupported_rank_relevant_extra_shell_degeneracy,
  no_retraction_source_plan_replay_rejected,
  no_retraction_source_association_rejected,
  no_retraction_empty_higher_order_scope,
  complete_certified_normalized_horizontal_h0_retraction,
};

enum class ExactDirectNormalizedH0RetractionScope : std::uint8_t {
  unspecified,
  horizontal_h0_orders_two_through_effective_maximum_direct_plus_complete_first_incidence_only,
};

// This fixed-size receipt proves source sufficiency for the normalized
// horizontal H0 reduction semantics.  It does not say that a quotient, fold
// or forest was executed.  The source plan's exact-level batches are complete
// atomic quotient inputs; a later consumer must still contract each complete
// batch against one strict pre-batch snapshot and preserve latent carriers.
//
// structurally_consistent() is intentionally not a standalone capability.
// Callers must require the fresh source-relative verifier below, which rebuilds
// both nested authorities and recursively compares this complete record.
struct ExactDirectNormalizedH0RetractionAuthority {
  static constexpr std::string_view backend =
      direct_normalized_h0_retraction_authority_backend;
  static constexpr std::string_view profile =
      direct_normalized_h0_retraction_authority_profile;
  static constexpr std::string_view mode =
      direct_normalized_h0_retraction_authority_mode;
  static constexpr std::string_view public_status =
      direct_normalized_h0_retraction_authority_public_status;
  static constexpr std::string_view proof_basis =
      direct_normalized_h0_retraction_authority_proof_basis;

  std::uint32_t schema_version{
      direct_normalized_h0_retraction_authority_schema_version};
  ExactDirectNormalizedH0SourcePlanBudget source_plan_budget{};
  ExactDirectSparseGatewayCandidateBudget source_gateway_budget{};
  spatial::LbvhTraversalOrder source_gateway_traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  std::size_t point_count{};
  std::size_t requested_maximum_order{};
  std::size_t effective_maximum_order{};
  std::size_t maximum_relevant_closed_rank{};
  std::size_t maximum_nonterminal_higher_order{};
  std::size_t source_direct_event_count{};
  std::size_t source_gateway_facet_count{};
  std::size_t source_gateway_candidate_count{};
  std::size_t source_normalized_coface_count{};
  std::size_t source_normalized_batch_count{};
  contract::CanonicalId source_pair_canonical_cloud_digest{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  contract::CanonicalId source_pair_semantic_digest{};
  contract::CanonicalId source_higher_semantic_digest{};
  contract::CanonicalId source_normalized_terminal_output_digest{};
  contract::CanonicalId source_gateway_scientific_identity_digest{};
  ExactDirectNormalizedH0SourcePlanDecision source_plan_decision{
      ExactDirectNormalizedH0SourcePlanDecision::not_certified};
  ExactDirectRankWindowSaturatedH0Decision source_rank_window_decision{
      ExactDirectRankWindowSaturatedH0Decision::not_certified};

  bool rank_window_authority_freshly_verified{false};
  bool source_plan_freshly_replayed{false};
  bool source_authorities_bound_to_same_terminal_facade{false};
  bool every_higher_order_direct_birth_present{false};
  bool every_higher_order_direct_coface_present{false};
  bool every_higher_order_direct_core_facet_present{false};
  bool every_higher_order_first_incidence_family_complete{false};
  bool every_first_incidence_cominimizer_retained{false};
  bool exact_level_batches_complete_atomic_quotient_inputs{false};
  bool factorized_cofaces_reconstructible_for_latent_carriers{false};
  bool rank_relevant_support_only_shells_certified{false};
  bool above_window_saturated_blocks_h0_quiescent{false};
  bool corollary_4_1_rank_relevant_branch_certified{false};
  bool corollary_4_2_above_window_branch_certified{false};
  bool terminal_order_equal_point_count_empty_by_cardinality{false};
  bool normalized_horizontal_h0_equivalence_certified{false};
  bool incidence_complete_reduction_proved{false};

  bool order_one_boruvka_seam_certified{false};
  bool geometric_global_regularity_claimed{false};
  bool atomic_quotient_or_forest_fold_executed{false};
  bool verticality_certified{false};
  bool naturality_certified{false};
  bool contract_v2_identity_compatible{false};
  bool campaign_v3_product_source_claimed{false};
  bool public_status_claimed{false};
  bool performance_claimed{false};
  bool global_star_or_gamma_materialized{false};
  bool higher_order_delaunay_materialized{false};
  ExactDirectNormalizedH0RetractionDecision decision{
      ExactDirectNormalizedH0RetractionDecision::not_certified};
  ExactDirectNormalizedH0RetractionScope scope{
      ExactDirectNormalizedH0RetractionScope::unspecified};

  [[nodiscard]] bool structurally_consistent() const noexcept;

  friend bool operator==(
      const ExactDirectNormalizedH0RetractionAuthority&,
      const ExactDirectNormalizedH0RetractionAuthority&) = default;
};

// Every potentially allocating nested operation is owned by the already
// guarded source-plan replay.  Its trusted source_gateway_budget and
// source_plan_budget are forwarded unchanged; the source-plan verifier checks
// observed arena sizes before scanning them and preflights expected arenas
// before allocation.  This composition layer owns no variable-size arena.
[[nodiscard]] ExactDirectNormalizedH0RetractionAuthority
build_exact_direct_normalized_h0_retraction_authority(
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
    const ExactDirectNormalizedH0SourcePlanBudget& source_plan_budget,
    const ExactDirectNormalizedH0SourcePlanResult& source_plan,
    const ExactDirectRankWindowSaturatedH0Authority&
        source_rank_window_authority);

struct ExactDirectNormalizedH0RetractionVerification {
  bool rank_window_authority_freshly_verified{false};
  bool source_plan_freshly_replayed{false};
  bool expected_authority_freshly_reconstructed{false};
  bool observed_recursively_equal{false};
  bool normalized_horizontal_scope_only{false};
  bool unsupported_claims_remain_false{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectNormalizedH0RetractionVerification&,
      const ExactDirectNormalizedH0RetractionVerification&) = default;
};

// The observed receipt never steers scientific reconstruction.  Expected
// output is rebuilt from all nested sources and trusted budgets; recursive
// equality is required before the structural success predicate is accepted.
[[nodiscard]] ExactDirectNormalizedH0RetractionVerification
verify_exact_direct_normalized_h0_retraction_authority(
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
    const ExactDirectNormalizedH0SourcePlanBudget& source_plan_budget,
    const ExactDirectNormalizedH0SourcePlanResult& source_plan,
    const ExactDirectRankWindowSaturatedH0Authority&
        source_rank_window_authority,
    const ExactDirectNormalizedH0RetractionAuthority& observed);

}  // namespace morsehgp3d::hierarchy
