#pragma once

#include "morsehgp3d/hierarchy/direct_normalized_h0_source_plan.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_normalized_h0_incidence_reduction_authority_schema_version = 1U;
inline constexpr std::string_view
    direct_normalized_h0_incidence_reduction_authority_backend =
        "reference_cpu";
inline constexpr std::string_view
    direct_normalized_h0_incidence_reduction_authority_profile =
        "hgp_reduced";
inline constexpr std::string_view
    direct_normalized_h0_incidence_reduction_authority_mode =
        "certified_rank_window_normalized_horizontal_h0_incidence_"
        "reduction_v1";
inline constexpr std::string_view
    direct_normalized_h0_incidence_reduction_authority_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_normalized_h0_incidence_reduction_authority_proof_basis =
        "fresh_complete_direct_and_first_incidence_source_replay_rank_"
        "relevant_support_only_shells_and_above_window_saturated_h0_"
        "quiescence_corollary_4_1_and_theorem_4_2_without_global_"
        "regularity_star_gamma_or_higher_delaunay_v1";

enum class ExactDirectNormalizedH0IncidenceReductionDecision
    : std::uint8_t {
  not_certified,
  no_point_authority_rejected,
  no_source_plan_replay_rejected,
  no_rank_window_authority_rejected,
  no_source_authority_binding_rejected,
  no_higher_order_scope,
  no_rank_relevant_degeneracy,
  no_horizontal_retraction_obligation_rejected,
  complete_certified_horizontal_incidence_reduction,
};

enum class ExactDirectNormalizedH0IncidenceReductionScope
    : std::uint8_t {
  unspecified,
  normalized_horizontal_h0_orders_two_through_effective_maximum,
};

// Fixed-size scientific receipt for the source reduction only.  It proves
// that complete direct events plus all co-minimizing first incidences preserve
// the normalized horizontal H0 forest in the requested rank window.  It does
// not claim that a resident fold, an adjacent-order vertical map, a durable
// checkpoint, or a public hierarchy was produced.  Payload trust always
// requires the fresh source-relative verifier declared below.
struct ExactDirectNormalizedH0IncidenceReductionAuthority {
  static constexpr std::string_view backend =
      direct_normalized_h0_incidence_reduction_authority_backend;
  static constexpr std::string_view profile =
      direct_normalized_h0_incidence_reduction_authority_profile;
  static constexpr std::string_view mode =
      direct_normalized_h0_incidence_reduction_authority_mode;
  static constexpr std::string_view public_status =
      direct_normalized_h0_incidence_reduction_authority_public_status;
  static constexpr std::string_view proof_basis =
      direct_normalized_h0_incidence_reduction_authority_proof_basis;

  std::uint32_t schema_version{
      direct_normalized_h0_incidence_reduction_authority_schema_version};
  ExactDirectNormalizedH0SourcePlanBudget source_plan_budget{};
  ExactDirectSparseGatewayCandidateBudget source_gateway_budget{};
  spatial::LbvhTraversalOrder source_gateway_traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  std::size_t point_count{};
  std::size_t requested_maximum_order{};
  std::size_t effective_maximum_order{};
  std::size_t maximum_relevant_closed_rank{};
  std::size_t source_direct_event_count{};
  std::size_t source_core_facet_count{};
  std::size_t source_gateway_candidate_count{};
  std::size_t normalized_coface_count{};
  std::size_t normalized_batch_count{};
  contract::CanonicalId source_pair_canonical_cloud_digest{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  contract::CanonicalId source_pair_semantic_digest{};
  contract::CanonicalId source_higher_semantic_digest{};
  contract::CanonicalId source_terminal_digest{};
  contract::CanonicalId source_gateway_scientific_identity_digest{};
  ExactDirectNormalizedH0SourcePlanDecision source_plan_decision{
      ExactDirectNormalizedH0SourcePlanDecision::not_certified};
  ExactDirectRankWindowSaturatedH0Decision rank_window_decision{
      ExactDirectRankWindowSaturatedH0Decision::not_certified};

  bool source_plan_freshly_replayed{false};
  bool rank_window_authority_freshly_replayed{false};
  bool source_authorities_bound_to_one_terminal_facade{false};
  bool every_higher_order_direct_birth_present{false};
  bool every_higher_order_direct_coface_present{false};
  bool every_direct_core_facet_present{false};
  bool every_first_incidence_family_complete{false};
  bool every_first_incidence_cominimizer_retained{false};
  bool normalized_batches_are_complete_atomic_quotient_inputs{false};
  bool factorized_cofaces_reconstructible_for_latent_carriers{false};
  bool every_rank_relevant_ball_has_support_only_shell{false};
  bool every_above_window_saturated_block_h0_quiescent{false};
  bool rank_relevant_corollary_4_1_certified{false};
  bool above_window_theorem_4_2_certified{false};
  bool normalized_horizontal_h0_equivalence_certified{false};
  bool incidence_complete_reduction_proved{false};

  bool geometric_global_regularity_claimed{false};
  bool resident_fold_executed{false};
  bool order_one_boruvka_seam_certified{false};
  bool vertical_maps_complete{false};
  bool all_naturality_squares_replayed{false};
  bool contract_v2_identity_compatible{false};
  bool campaign_product_claimed{false};
  bool global_star_or_gamma_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool public_status_claimed{false};
  bool performance_claimed{false};
  ExactDirectNormalizedH0IncidenceReductionDecision decision{
      ExactDirectNormalizedH0IncidenceReductionDecision::not_certified};
  ExactDirectNormalizedH0IncidenceReductionScope scope{
      ExactDirectNormalizedH0IncidenceReductionScope::unspecified};

  [[nodiscard]] bool certified_horizontal_incidence_reduction()
      const noexcept;

  friend bool operator==(
      const ExactDirectNormalizedH0IncidenceReductionAuthority&,
      const ExactDirectNormalizedH0IncidenceReductionAuthority&) = default;
};

[[nodiscard]] ExactDirectNormalizedH0IncidenceReductionAuthority
build_exact_direct_normalized_h0_incidence_reduction_authority(
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
    const ExactDirectRankWindowSaturatedH0Authority& rank_window_authority);

struct ExactDirectNormalizedH0IncidenceReductionVerification {
  bool point_authority_certified{false};
  bool source_plan_freshly_replayed{false};
  bool rank_window_authority_freshly_replayed{false};
  bool expected_authority_freshly_reconstructed{false};
  bool observed_recursively_equal{false};
  bool unsupported_claims_remain_false{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectNormalizedH0IncidenceReductionVerification&,
      const ExactDirectNormalizedH0IncidenceReductionVerification&) =
      default;
};

[[nodiscard]] ExactDirectNormalizedH0IncidenceReductionVerification
verify_exact_direct_normalized_h0_incidence_reduction_authority(
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
    const ExactDirectRankWindowSaturatedH0Authority& rank_window_authority,
    const ExactDirectNormalizedH0IncidenceReductionAuthority& observed);

}  // namespace morsehgp3d::hierarchy
