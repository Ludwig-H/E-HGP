// Archived Phase 15 prototype; excluded from the active API and build.

#include "morsehgp3d/hierarchy/direct_normalized_h0_retraction_authority.hpp"

#include <algorithm>
#include <stdexcept>

namespace morsehgp3d::hierarchy {
namespace {

[[nodiscard]] ExactDirectNormalizedH0RetractionAuthority base_authority(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    spatial::LbvhTraversalOrder source_gateway_traversal_order,
    const ExactDirectNormalizedH0SourcePlanBudget& source_plan_budget) {
  ExactDirectNormalizedH0RetractionAuthority result;
  const ExactDirectSupportTerminalCertificate& terminal =
      source_facade.certificate;
  result.source_plan_budget = source_plan_budget;
  result.source_gateway_budget = source_gateway_budget;
  result.source_gateway_traversal_order = source_gateway_traversal_order;
  result.point_count = cloud.size();
  result.requested_maximum_order =
      terminal.requirements.requested_maximum_order;
  result.effective_maximum_order =
      terminal.requirements.effective_maximum_order;
  result.maximum_relevant_closed_rank =
      terminal.requirements.maximum_relevant_closed_rank;
  result.maximum_nonterminal_higher_order =
      result.point_count == 0U
          ? 0U
          : std::min(
                result.effective_maximum_order, result.point_count - 1U);
  result.source_pair_canonical_cloud_digest =
      terminal.pair_canonical_cloud_digest;
  result.source_higher_canonical_cloud_digest =
      terminal.higher_canonical_cloud_digest;
  result.source_pair_semantic_digest = terminal.pair_semantic_digest;
  result.source_higher_semantic_digest = terminal.higher_semantic_digest;
  result.source_normalized_terminal_output_digest =
      terminal.normalized_terminal_output_digest;
  return result;
}

[[nodiscard]] bool unsupported_claims_remain_false(
    const ExactDirectNormalizedH0RetractionAuthority& authority) noexcept {
  return !authority.order_one_boruvka_seam_certified &&
         !authority.geometric_global_regularity_claimed &&
         !authority.atomic_quotient_or_forest_fold_executed &&
         !authority.verticality_certified &&
         !authority.naturality_certified &&
         !authority.contract_v2_identity_compatible &&
         !authority.campaign_v3_product_source_claimed &&
         !authority.public_status_claimed && !authority.performance_claimed &&
         !authority.global_star_or_gamma_materialized &&
         !authority.higher_order_delaunay_materialized;
}

[[nodiscard]] bool same_source_authorities(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& facade,
    const ExactDirectMorseEventJournalResult& journal,
    const ExactDirectSparseGatewayCandidateBudget& gateway_budget,
    spatial::LbvhTraversalOrder gateway_traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& gateway,
    const ExactDirectNormalizedH0SourcePlanBudget& plan_budget,
    const ExactDirectNormalizedH0SourcePlanResult& plan,
    const ExactDirectRankWindowSaturatedH0Authority& rank) noexcept {
  const ExactDirectSupportTerminalCertificate& terminal = facade.certificate;
  return plan.requested_budget == plan_budget &&
         plan.source_gateway_budget == gateway_budget &&
         plan.source_gateway_traversal_order == gateway_traversal_order &&
         gateway.requested_budget == gateway_budget &&
         gateway.traversal_order == gateway_traversal_order &&
         plan.point_count == cloud.size() &&
         gateway.point_count == cloud.size() &&
         journal.point_count == cloud.size() &&
         rank.requirements == terminal.requirements &&
         terminal.requirements.point_count == cloud.size() &&
         plan.source_direct_event_count == facade.events.size() &&
         rank.source_event_count == facade.events.size() &&
         plan.source_pair_canonical_cloud_digest ==
             rank.source_pair_canonical_cloud_digest &&
         plan.source_higher_canonical_cloud_digest ==
             rank.source_higher_canonical_cloud_digest &&
         plan.source_pair_semantic_digest ==
             rank.source_pair_semantic_digest &&
         plan.source_higher_semantic_digest ==
             rank.source_higher_semantic_digest &&
         plan.source_pair_canonical_cloud_digest ==
             terminal.pair_canonical_cloud_digest &&
         plan.source_higher_canonical_cloud_digest ==
             terminal.higher_canonical_cloud_digest &&
         plan.source_pair_semantic_digest == terminal.pair_semantic_digest &&
         plan.source_higher_semantic_digest ==
             terminal.higher_semantic_digest &&
         rank.source_normalized_terminal_output_digest ==
             terminal.normalized_terminal_output_digest;
}

}  // namespace

bool ExactDirectNormalizedH0RetractionAuthority::structurally_consistent()
    const noexcept {
  if (schema_version !=
          direct_normalized_h0_retraction_authority_schema_version ||
      point_count == 0U || requested_maximum_order == 0U ||
      effective_maximum_order < 2U ||
      effective_maximum_order !=
          std::min(requested_maximum_order, point_count) ||
      maximum_relevant_closed_rank !=
          (effective_maximum_order < point_count
               ? effective_maximum_order + 1U
               : point_count) ||
      maximum_nonterminal_higher_order !=
          std::min(effective_maximum_order, point_count - 1U) ||
      terminal_order_equal_point_count_empty_by_cardinality !=
          (effective_maximum_order == point_count) ||
      source_plan_decision !=
          ExactDirectNormalizedH0SourcePlanDecision::
              complete_certified_direct_plus_first_incidence_source_plan ||
      source_rank_window_decision !=
          ExactDirectRankWindowSaturatedH0Decision::
              certified_rank_window_saturated_h0_quiescence ||
      decision != ExactDirectNormalizedH0RetractionDecision::
                      complete_certified_normalized_horizontal_h0_retraction ||
      scope != ExactDirectNormalizedH0RetractionScope::
                   horizontal_h0_orders_two_through_effective_maximum_direct_plus_complete_first_incidence_only) {
    return false;
  }
  return rank_window_authority_freshly_verified &&
         source_plan_freshly_replayed &&
         source_authorities_bound_to_same_terminal_facade &&
         every_higher_order_direct_birth_present &&
         every_higher_order_direct_coface_present &&
         every_higher_order_direct_core_facet_present &&
         every_higher_order_first_incidence_family_complete &&
         every_first_incidence_cominimizer_retained &&
         exact_level_batches_complete_atomic_quotient_inputs &&
         factorized_cofaces_reconstructible_for_latent_carriers &&
         rank_relevant_support_only_shells_certified &&
         above_window_saturated_blocks_h0_quiescent &&
         corollary_4_1_rank_relevant_branch_certified &&
         corollary_4_2_above_window_branch_certified &&
         normalized_horizontal_h0_equivalence_certified &&
         incidence_complete_reduction_proved &&
         unsupported_claims_remain_false(*this);
}

ExactDirectNormalizedH0RetractionAuthority
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
        source_rank_window_authority) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the normalized-H0 retraction authority has a different point authority");
  }

  ExactDirectNormalizedH0RetractionAuthority result = base_authority(
      cloud,
      source_facade,
      source_gateway_budget,
      source_gateway_traversal_order,
      source_plan_budget);

  const ExactDirectRankWindowSaturatedH0Verification rank_verification =
      verify_exact_direct_rank_window_saturated_h0_authority(
          source_facade, source_rank_window_authority);
  result.rank_window_authority_freshly_verified =
      rank_verification.result_certified;
  if (!rank_verification.result_certified) {
    if (rank_verification.observed_recursively_equal) {
      result.source_rank_window_decision =
          source_rank_window_authority.decision;
      if (source_rank_window_authority.decision ==
          ExactDirectRankWindowSaturatedH0Decision::
              unsupported_rank_relevant_extra_shell_degeneracy) {
        result.decision = ExactDirectNormalizedH0RetractionDecision::
            no_retraction_unsupported_rank_relevant_extra_shell_degeneracy;
        return result;
      }
    }
    result.decision = ExactDirectNormalizedH0RetractionDecision::
        no_retraction_rank_window_authority_rejected;
    return result;
  }
  result.source_rank_window_decision = source_rank_window_authority.decision;

  const ExactDirectNormalizedH0SourcePlanVerification plan_verification =
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
  result.source_plan_freshly_replayed = plan_verification.result_certified;
  if (!plan_verification.result_certified) {
    result.decision = ExactDirectNormalizedH0RetractionDecision::
        no_retraction_source_plan_replay_rejected;
    return result;
  }
  result.source_plan_decision = source_plan.decision;

  result.source_authorities_bound_to_same_terminal_facade =
      same_source_authorities(
          cloud,
          source_facade,
          source_journal,
          source_gateway_budget,
          source_gateway_traversal_order,
          source_gateway,
          source_plan_budget,
          source_plan,
          source_rank_window_authority);
  if (!result.source_authorities_bound_to_same_terminal_facade) {
    result.decision = ExactDirectNormalizedH0RetractionDecision::
        no_retraction_source_association_rejected;
    return result;
  }
  if (result.effective_maximum_order < 2U) {
    result.decision = ExactDirectNormalizedH0RetractionDecision::
        no_retraction_empty_higher_order_scope;
    return result;
  }

  result.source_direct_event_count = source_plan.source_direct_event_count;
  result.source_gateway_facet_count = source_gateway.facet_tokens.size();
  result.source_gateway_candidate_count =
      source_gateway.gateway_candidates.size();
  result.source_normalized_coface_count = source_plan.cofaces.size();
  result.source_normalized_batch_count = source_plan.batches.size();
  result.source_gateway_scientific_identity_digest =
      source_plan.source_gateway_scientific_identity_digest;

  result.every_higher_order_direct_birth_present =
      source_plan.every_higher_order_direct_birth_projected_once;
  result.every_higher_order_direct_coface_present =
      source_plan.every_higher_order_direct_saddle_joined_once;
  result.every_higher_order_direct_core_facet_present =
      source_gateway.every_strict_and_equal_deletion_reconstructed &&
      source_gateway.distinct_full_keys_deduplicated &&
      source_gateway.one_first_incidence_call_per_distinct_facet &&
      source_plan.source_gateway_token_count ==
          source_gateway.facet_tokens.size();
  result.every_higher_order_first_incidence_family_complete =
      source_gateway.every_first_incidence_complete &&
      source_plan.every_higher_order_source_gateway_candidate_mapped_once;
  result.every_first_incidence_cominimizer_retained =
      source_gateway.all_positive_support_candidates_retained_atomically &&
      source_plan
          .direct_union_complete_first_incidence_cominimizers_deduplicated;
  result.exact_level_batches_complete_atomic_quotient_inputs =
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

  result.rank_relevant_support_only_shells_certified =
      source_rank_window_authority
          .every_rank_relevant_minimal_ball_has_support_only_shell &&
      source_rank_window_authority
          .no_rank_relevant_extra_shell_diagnostic;
  result.above_window_saturated_blocks_h0_quiescent =
      source_rank_window_authority
          .every_above_window_minimal_ball_has_h0_quiescent_saturated_block &&
      source_rank_window_authority
          .hidden_above_window_extra_shells_explicitly_permitted &&
      !source_rank_window_authority.geometric_global_regularity_claimed;

  result.corollary_4_1_rank_relevant_branch_certified =
      result.every_higher_order_direct_birth_present &&
      result.every_higher_order_direct_coface_present &&
      result.every_higher_order_direct_core_facet_present &&
      result.every_higher_order_first_incidence_family_complete &&
      result.every_first_incidence_cominimizer_retained &&
      result.exact_level_batches_complete_atomic_quotient_inputs &&
      result.factorized_cofaces_reconstructible_for_latent_carriers &&
      result.rank_relevant_support_only_shells_certified;
  result.corollary_4_2_above_window_branch_certified =
      result.above_window_saturated_blocks_h0_quiescent;
  result.terminal_order_equal_point_count_empty_by_cardinality =
      result.effective_maximum_order == result.point_count;
  result.normalized_horizontal_h0_equivalence_certified =
      result.corollary_4_1_rank_relevant_branch_certified &&
      result.corollary_4_2_above_window_branch_certified;
  result.incidence_complete_reduction_proved =
      result.normalized_horizontal_h0_equivalence_certified;
  if (!result.incidence_complete_reduction_proved) {
    result.decision = ExactDirectNormalizedH0RetractionDecision::
        no_retraction_source_association_rejected;
    return result;
  }

  result.decision = ExactDirectNormalizedH0RetractionDecision::
      complete_certified_normalized_horizontal_h0_retraction;
  result.scope = ExactDirectNormalizedH0RetractionScope::
      horizontal_h0_orders_two_through_effective_maximum_direct_plus_complete_first_incidence_only;
  if (!result.structurally_consistent()) {
    throw std::logic_error(
        "a normalized-H0 retraction authority violated its construction invariants");
  }
  return result;
}

ExactDirectNormalizedH0RetractionVerification
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
    const ExactDirectNormalizedH0RetractionAuthority& observed) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the normalized-H0 retraction verifier has a different point authority");
  }
  ExactDirectNormalizedH0RetractionVerification verification;
  const ExactDirectNormalizedH0RetractionAuthority expected =
      build_exact_direct_normalized_h0_retraction_authority(
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
          source_rank_window_authority);
  verification.rank_window_authority_freshly_verified =
      expected.rank_window_authority_freshly_verified;
  verification.source_plan_freshly_replayed =
      expected.source_plan_freshly_replayed;
  verification.expected_authority_freshly_reconstructed =
      expected.decision !=
      ExactDirectNormalizedH0RetractionDecision::not_certified;
  verification.observed_recursively_equal = observed == expected;
  verification.normalized_horizontal_scope_only =
      observed.scope ==
          ExactDirectNormalizedH0RetractionScope::
              horizontal_h0_orders_two_through_effective_maximum_direct_plus_complete_first_incidence_only &&
      observed.effective_maximum_order >= 2U;
  verification.unsupported_claims_remain_false =
      unsupported_claims_remain_false(observed);
  verification.result_certified =
      verification.rank_window_authority_freshly_verified &&
      verification.source_plan_freshly_replayed &&
      verification.expected_authority_freshly_reconstructed &&
      verification.observed_recursively_equal &&
      verification.normalized_horizontal_scope_only &&
      verification.unsupported_claims_remain_false &&
      expected.structurally_consistent();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
