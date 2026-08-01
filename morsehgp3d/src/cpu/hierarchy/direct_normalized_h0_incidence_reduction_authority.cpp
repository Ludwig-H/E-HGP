#include "morsehgp3d/hierarchy/direct_normalized_h0_incidence_reduction_authority.hpp"

#include <algorithm>

namespace morsehgp3d::hierarchy {
namespace {

[[nodiscard]] bool unsupported_claims_are_false(
    const ExactDirectNormalizedH0IncidenceReductionAuthority& authority)
    noexcept {
  return !authority.geometric_global_regularity_claimed &&
         !authority.resident_fold_executed &&
         !authority.order_one_boruvka_seam_certified &&
         !authority.vertical_maps_complete &&
         !authority.all_naturality_squares_replayed &&
         !authority.contract_v2_identity_compatible &&
         !authority.campaign_product_claimed &&
         !authority.global_star_or_gamma_materialized &&
         !authority.ordinary_or_higher_order_delaunay_materialized &&
         !authority.public_status_claimed &&
         !authority.performance_claimed;
}

[[nodiscard]] ExactDirectNormalizedH0IncidenceReductionAuthority
base_authority(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    spatial::LbvhTraversalOrder source_gateway_traversal_order,
    const ExactDirectNormalizedH0SourcePlanBudget& source_plan_budget) {
  ExactDirectNormalizedH0IncidenceReductionAuthority result;
  result.source_plan_budget = source_plan_budget;
  result.source_gateway_budget = source_gateway_budget;
  result.source_gateway_traversal_order = source_gateway_traversal_order;
  result.point_count = cloud.size();
  const auto& terminal = source_facade.certificate;
  result.requested_maximum_order =
      terminal.requirements.requested_maximum_order;
  result.effective_maximum_order =
      terminal.requirements.effective_maximum_order;
  result.maximum_relevant_closed_rank =
      terminal.requirements.maximum_relevant_closed_rank;
  result.source_pair_canonical_cloud_digest =
      terminal.pair_canonical_cloud_digest;
  result.source_higher_canonical_cloud_digest =
      terminal.higher_canonical_cloud_digest;
  result.source_pair_semantic_digest = terminal.pair_semantic_digest;
  result.source_higher_semantic_digest = terminal.higher_semantic_digest;
  result.source_terminal_digest = terminal.normalized_terminal_output_digest;
  return result;
}

[[nodiscard]] bool source_authorities_match(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    spatial::LbvhTraversalOrder source_gateway_traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& source_gateway,
    const ExactDirectNormalizedH0SourcePlanBudget& source_plan_budget,
    const ExactDirectNormalizedH0SourcePlanResult& source_plan,
    const ExactDirectRankWindowSaturatedH0Authority& rank_window) noexcept {
  const auto& terminal = source_facade.certificate;
  return source_facade.terminal_catalog_certified() &&
         terminal.requirements.point_count == cloud.size() &&
         source_journal.point_count == cloud.size() &&
         source_gateway.point_count == cloud.size() &&
         source_plan.point_count == cloud.size() &&
         source_gateway.requested_budget == source_gateway_budget &&
         source_gateway.traversal_order == source_gateway_traversal_order &&
         source_plan.requested_budget == source_plan_budget &&
         source_plan.source_gateway_budget == source_gateway_budget &&
         source_plan.source_gateway_traversal_order ==
             source_gateway_traversal_order &&
         rank_window.requirements == terminal.requirements &&
         source_plan.source_direct_event_count == source_facade.events.size() &&
         source_gateway.source_direct_event_count ==
             source_facade.events.size() &&
         rank_window.source_event_count == source_facade.events.size() &&
         source_journal.source_pair_semantic_digest ==
             terminal.pair_semantic_digest &&
         source_journal.source_higher_semantic_digest ==
             terminal.higher_semantic_digest &&
         source_plan.source_pair_canonical_cloud_digest ==
             terminal.pair_canonical_cloud_digest &&
         source_plan.source_higher_canonical_cloud_digest ==
             terminal.higher_canonical_cloud_digest &&
         source_plan.source_pair_semantic_digest ==
             terminal.pair_semantic_digest &&
         source_plan.source_higher_semantic_digest ==
             terminal.higher_semantic_digest &&
         source_gateway.source_pair_canonical_cloud_digest ==
             terminal.pair_canonical_cloud_digest &&
         source_gateway.source_higher_canonical_cloud_digest ==
             terminal.higher_canonical_cloud_digest &&
         source_gateway.source_pair_semantic_digest ==
             terminal.pair_semantic_digest &&
         source_gateway.source_higher_semantic_digest ==
             terminal.higher_semantic_digest &&
         rank_window.source_pair_canonical_cloud_digest ==
             terminal.pair_canonical_cloud_digest &&
         rank_window.source_higher_canonical_cloud_digest ==
             terminal.higher_canonical_cloud_digest &&
         rank_window.source_pair_semantic_digest ==
             terminal.pair_semantic_digest &&
         rank_window.source_higher_semantic_digest ==
             terminal.higher_semantic_digest &&
         rank_window.source_normalized_terminal_output_digest ==
             terminal.normalized_terminal_output_digest;
}

}  // namespace

bool ExactDirectNormalizedH0IncidenceReductionAuthority::
    certified_horizontal_incidence_reduction() const noexcept {
  return schema_version ==
             direct_normalized_h0_incidence_reduction_authority_schema_version &&
         point_count != 0U && requested_maximum_order != 0U &&
         effective_maximum_order >= 2U &&
         effective_maximum_order ==
             std::min(requested_maximum_order, point_count) &&
         maximum_relevant_closed_rank ==
             (effective_maximum_order < point_count
                  ? effective_maximum_order + 1U
                  : point_count) &&
         source_plan_decision ==
             ExactDirectNormalizedH0SourcePlanDecision::
                 complete_certified_direct_plus_first_incidence_source_plan &&
         rank_window_decision ==
             ExactDirectRankWindowSaturatedH0Decision::
                 certified_rank_window_saturated_h0_quiescence &&
         source_plan_freshly_replayed &&
         rank_window_authority_freshly_replayed &&
         source_authorities_bound_to_one_terminal_facade &&
         every_higher_order_direct_birth_present &&
         every_higher_order_direct_coface_present &&
         every_direct_core_facet_present &&
         every_first_incidence_family_complete &&
         every_first_incidence_cominimizer_retained &&
         normalized_batches_are_complete_atomic_quotient_inputs &&
         factorized_cofaces_reconstructible_for_latent_carriers &&
         every_rank_relevant_ball_has_support_only_shell &&
         every_above_window_saturated_block_h0_quiescent &&
         rank_relevant_corollary_4_1_certified &&
         above_window_theorem_4_2_certified &&
         normalized_horizontal_h0_equivalence_certified &&
         incidence_complete_reduction_proved &&
         unsupported_claims_are_false(*this) &&
         decision ==
             ExactDirectNormalizedH0IncidenceReductionDecision::
                 complete_certified_horizontal_incidence_reduction &&
         scope ==
             ExactDirectNormalizedH0IncidenceReductionScope::
                 normalized_horizontal_h0_orders_two_through_effective_maximum;
}

ExactDirectNormalizedH0IncidenceReductionAuthority
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
    const ExactDirectRankWindowSaturatedH0Authority& rank_window_authority) {
  auto result = base_authority(
      cloud,
      source_facade,
      source_gateway_budget,
      source_gateway_traversal_order,
      source_plan_budget);
  if (!index.validated_for(cloud)) {
    result.decision =
        ExactDirectNormalizedH0IncidenceReductionDecision::
            no_point_authority_rejected;
    return result;
  }

  const auto source_verification =
      verify_exact_direct_normalized_h0_source_plan(
          index,
          cloud,
          source_facade,
          source_journal,
          source_arm_budget,
          source_arm_journal,
          source_incidence_budget,
          source_incidence_journal,
          source_gateway_budget,
          source_gateway_traversal_order,
          source_gateway,
          source_plan_budget,
          source_plan);
  result.source_plan_freshly_replayed =
      source_verification.result_certified;
  if (!result.source_plan_freshly_replayed) {
    result.decision =
        ExactDirectNormalizedH0IncidenceReductionDecision::
            no_source_plan_replay_rejected;
    return result;
  }
  result.source_plan_decision = source_plan.decision;

  const auto rank_verification =
      verify_exact_direct_rank_window_saturated_h0_authority(
          source_facade, rank_window_authority);
  result.rank_window_authority_freshly_replayed =
      rank_verification.result_certified;
  result.rank_window_decision = rank_window_authority.decision;
  if (!result.rank_window_authority_freshly_replayed) {
    result.decision =
        rank_window_authority.decision ==
                ExactDirectRankWindowSaturatedH0Decision::
                    unsupported_rank_relevant_extra_shell_degeneracy
            ? ExactDirectNormalizedH0IncidenceReductionDecision::
                  no_rank_relevant_degeneracy
            : ExactDirectNormalizedH0IncidenceReductionDecision::
                  no_rank_window_authority_rejected;
    return result;
  }

  result.source_authorities_bound_to_one_terminal_facade =
      source_authorities_match(
          cloud,
          source_facade,
          source_journal,
          source_gateway_budget,
          source_gateway_traversal_order,
          source_gateway,
          source_plan_budget,
          source_plan,
          rank_window_authority);
  if (!result.source_authorities_bound_to_one_terminal_facade) {
    result.decision =
        ExactDirectNormalizedH0IncidenceReductionDecision::
            no_source_authority_binding_rejected;
    return result;
  }
  if (result.effective_maximum_order < 2U) {
    result.decision =
        ExactDirectNormalizedH0IncidenceReductionDecision::
            no_higher_order_scope;
    return result;
  }

  result.source_direct_event_count = source_plan.source_direct_event_count;
  result.source_core_facet_count = source_gateway.facet_tokens.size();
  result.source_gateway_candidate_count =
      source_gateway.gateway_candidates.size();
  result.normalized_coface_count = source_plan.cofaces.size();
  result.normalized_batch_count = source_plan.batches.size();
  result.source_gateway_scientific_identity_digest =
      source_plan.source_gateway_scientific_identity_digest;

  result.every_higher_order_direct_birth_present =
      source_plan.every_higher_order_direct_birth_projected_once;
  result.every_higher_order_direct_coface_present =
      source_plan.every_higher_order_direct_saddle_joined_once;
  result.every_direct_core_facet_present =
      source_gateway.every_strict_and_equal_deletion_reconstructed &&
      source_gateway.distinct_full_keys_deduplicated &&
      source_gateway.one_first_incidence_call_per_distinct_facet &&
      source_plan.source_gateway_token_count ==
          source_gateway.facet_tokens.size();
  result.every_first_incidence_family_complete =
      source_gateway.every_first_incidence_complete &&
      source_plan.every_higher_order_source_gateway_candidate_mapped_once;
  result.every_first_incidence_cominimizer_retained =
      source_gateway.all_positive_support_candidates_retained_atomically &&
      source_plan
          .direct_union_complete_first_incidence_cominimizers_deduplicated;
  result.normalized_batches_are_complete_atomic_quotient_inputs =
      source_plan.unique_batch_per_exact_level_and_order &&
      source_plan.batches_sorted_by_exact_level_then_order &&
      source_plan.exact_rational_levels_retained &&
      source_plan.required_direct_birth_reference_count ==
          source_plan.direct_birth_references.size() &&
      source_plan.required_batch_coface_reference_count ==
          source_plan.cofaces.size() &&
      source_plan.batch_coface_references.size() ==
          source_plan.cofaces.size();
  result.factorized_cofaces_reconstructible_for_latent_carriers =
      source_plan.source_gateway_scientific_identity_certified &&
      source_plan.canonical_owner_is_lexicographically_first_core_facet &&
      source_plan.gateway_core_keys_and_audits_reused_without_copy &&
      source_plan.transient_k_plus_one_keys_released_before_publication &&
      !source_plan.persistent_k_plus_one_keys_materialized;
  result.every_rank_relevant_ball_has_support_only_shell =
      rank_window_authority
          .every_rank_relevant_minimal_ball_has_support_only_shell &&
      rank_window_authority.no_rank_relevant_extra_shell_diagnostic;
  result.every_above_window_saturated_block_h0_quiescent =
      rank_window_authority
          .every_above_window_minimal_ball_has_h0_quiescent_saturated_block &&
      rank_window_authority
          .hidden_above_window_extra_shells_explicitly_permitted &&
      !rank_window_authority.geometric_global_regularity_claimed;

  result.rank_relevant_corollary_4_1_certified =
      result.every_higher_order_direct_birth_present &&
      result.every_higher_order_direct_coface_present &&
      result.every_direct_core_facet_present &&
      result.every_first_incidence_family_complete &&
      result.every_first_incidence_cominimizer_retained &&
      result.normalized_batches_are_complete_atomic_quotient_inputs &&
      result.factorized_cofaces_reconstructible_for_latent_carriers &&
      result.every_rank_relevant_ball_has_support_only_shell;
  result.above_window_theorem_4_2_certified =
      result.every_above_window_saturated_block_h0_quiescent;
  result.normalized_horizontal_h0_equivalence_certified =
      result.rank_relevant_corollary_4_1_certified &&
      result.above_window_theorem_4_2_certified;
  result.incidence_complete_reduction_proved =
      result.normalized_horizontal_h0_equivalence_certified;
  if (!result.incidence_complete_reduction_proved) {
    result.decision =
        ExactDirectNormalizedH0IncidenceReductionDecision::
            no_horizontal_retraction_obligation_rejected;
    return result;
  }

  result.decision =
      ExactDirectNormalizedH0IncidenceReductionDecision::
          complete_certified_horizontal_incidence_reduction;
  result.scope =
      ExactDirectNormalizedH0IncidenceReductionScope::
          normalized_horizontal_h0_orders_two_through_effective_maximum;
  return result;
}

ExactDirectNormalizedH0IncidenceReductionVerification
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
    const ExactDirectNormalizedH0IncidenceReductionAuthority& observed) {
  ExactDirectNormalizedH0IncidenceReductionVerification result;
  result.point_authority_certified = index.validated_for(cloud);
  const auto expected =
      build_exact_direct_normalized_h0_incidence_reduction_authority(
          index,
          cloud,
          source_facade,
          source_journal,
          source_arm_budget,
          source_arm_journal,
          source_incidence_budget,
          source_incidence_journal,
          source_gateway_budget,
          source_gateway_traversal_order,
          source_gateway,
          source_plan_budget,
          source_plan,
          rank_window_authority);
  result.source_plan_freshly_replayed =
      expected.source_plan_freshly_replayed;
  result.rank_window_authority_freshly_replayed =
      expected.rank_window_authority_freshly_replayed;
  result.expected_authority_freshly_reconstructed =
      expected.decision !=
      ExactDirectNormalizedH0IncidenceReductionDecision::not_certified;
  result.observed_recursively_equal = observed == expected;
  result.unsupported_claims_remain_false =
      unsupported_claims_are_false(observed);
  result.result_certified =
      result.point_authority_certified &&
      result.source_plan_freshly_replayed &&
      result.rank_window_authority_freshly_replayed &&
      result.expected_authority_freshly_reconstructed &&
      result.observed_recursively_equal &&
      result.unsupported_claims_remain_false &&
      expected.certified_horizontal_incidence_reduction();
  return result;
}

}  // namespace morsehgp3d::hierarchy
