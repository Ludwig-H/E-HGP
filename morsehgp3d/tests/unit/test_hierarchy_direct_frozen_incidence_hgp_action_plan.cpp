#include "morsehgp3d/hierarchy/direct_frozen_incidence_hgp_action_plan.hpp"

#include <array>
#include <cstddef>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] constexpr ExactFrozenIncidenceToken rooted(
    ExactFrozenIncidenceTokenId id) {
  return {ExactFrozenIncidenceTokenKind::rooted_carrier, id};
}

[[nodiscard]] constexpr ExactFrozenIncidenceToken latent(
    ExactFrozenIncidenceTokenId id) {
  return {ExactFrozenIncidenceTokenKind::latent_carrier, id};
}

[[nodiscard]] constexpr ExactFrozenIncidenceToken equal_facet(
    ExactFrozenIncidenceTokenId id) {
  return {ExactFrozenIncidenceTokenKind::equal_facet, id};
}

[[nodiscard]] ExactFrozenIncidenceQuotientBudget unlimited_quotient_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum,
          maximum,
          maximum,
          maximum,
          maximum,
          maximum,
          maximum,
          maximum,
          maximum};
}

[[nodiscard]] ExactFrozenIncidenceHgpActionPlanBudget
unlimited_action_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum,
          maximum,
          maximum,
          maximum,
          maximum,
          maximum,
          maximum,
          maximum,
          maximum};
}

[[nodiscard]] bool complete_streaming_verification(
    const ExactFrozenIncidenceHgpActionPlanStreamingVerification&
        verification) {
  return verification.requested_budget_certified &&
         verification.requirements_certified &&
         verification.source_quotient_certified &&
         verification.groups_certified &&
         verification.direct_hyperedge_slices_certified &&
         verification.residual_hyperedge_slices_certified &&
         verification.prior_root_slices_certified &&
         verification.counters_certified &&
         verification.result_facts_certified &&
         verification.mutation_non_scope_certified &&
         verification.decision_and_scope_certified &&
         verification.no_second_persistent_output_arena_certified &&
         verification.fresh_streaming_replay_certified &&
         verification.result_certified;
}

struct E5SuffixFixture {
  std::array<ExactFrozenIncidenceHyperedge, 10U> hyperedges{{
      {0U, 0U, 2U},
      {1U, 2U, 2U},
      {2U, 4U, 2U},
      {3U, 6U, 2U},
      {4U, 8U, 2U},
      {5U, 10U, 3U},
      {6U, 13U, 2U},
      {7U, 15U, 1U},
      {8U, 16U, 2U},
      {9U, 18U, 2U},
  }};
  std::array<ExactFrozenIncidenceToken, 20U> tokens{{
      rooted(30U),
      equal_facet(300U),
      rooted(10U),
      latent(101U),
      latent(60U),
      equal_facet(600U),
      rooted(40U),
      latent(400U),
      equal_facet(300U),
      latent(301U),
      rooted(20U),
      rooted(21U),
      equal_facet(200U),
      rooted(50U),
      equal_facet(500U),
      latent(70U),
      latent(400U),
      equal_facet(401U),
      equal_facet(500U),
      latent(501U),
  }};
  std::array<ExactFrozenIncidenceHyperedgeProvenance, 10U> provenance{{
      {0U,
       ExactFrozenIncidenceHyperedgeProvenanceKind::residual_incidence,
       0U,
       true,
       false},
      {1U,
       ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family,
       1U,
       false,
       false},
      {2U,
       ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family,
       1U,
       false,
       false},
      {3U,
       ExactFrozenIncidenceHyperedgeProvenanceKind::residual_incidence,
       0U,
       true,
       false},
      {4U,
       ExactFrozenIncidenceHyperedgeProvenanceKind::residual_incidence,
       0U,
       true,
       false},
      {5U,
       ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family,
       1U,
       false,
       false},
      {6U,
       ExactFrozenIncidenceHyperedgeProvenanceKind::residual_incidence,
       0U,
       true,
       true},
      {7U,
       ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family,
       1U,
       false,
       false},
      {8U,
       ExactFrozenIncidenceHyperedgeProvenanceKind::residual_incidence,
       0U,
       true,
       false},
      {9U,
       ExactFrozenIncidenceHyperedgeProvenanceKind::residual_incidence,
       0U,
       true,
       true},
  }};
  std::array<ExactFrozenIncidenceRootAttachment, 6U> attachments{{
      {10U, 1000U},
      {20U, 2000U},
      {21U, 2001U},
      {30U, 3000U},
      {40U, 4000U},
      {50U, 5000U},
  }};
};

void check_default_result_is_not_an_atomic_failure() {
  const ExactFrozenIncidenceHgpActionPlanResult result;
  check(
      result.decision ==
              ExactFrozenIncidenceHgpActionPlanDecision::not_certified &&
          !result.atomic_empty_failure(),
      "a default not-certified action plan is not a certified atomic failure");
}

[[nodiscard]] ExactFrozenIncidenceQuotientResult build_quotient(
    std::span<const ExactFrozenIncidenceHyperedge> hyperedges,
    std::span<const ExactFrozenIncidenceToken> tokens) {
  return build_exact_direct_frozen_incidence_quotient(
      hyperedges, tokens, unlimited_quotient_budget());
}

void check_e5_suffix_shape_and_actions() {
  const E5SuffixFixture fixture;
  const auto quotient = build_quotient(fixture.hyperedges, fixture.tokens);
  check(
      quotient.certified_frozen_incidence_quotient() &&
          quotient.groups.size() == 7U,
      "the synthetic E5 suffix authority closes into seven groups");
  const auto plan = build_exact_direct_frozen_incidence_hgp_action_plan(
      fixture.hyperedges,
      fixture.tokens,
      unlimited_quotient_budget(),
      quotient,
      fixture.provenance,
      fixture.attachments,
      unlimited_action_budget());
  check(
      plan.certified_frozen_incidence_hgp_action_plan() &&
          plan.groups.size() == 7U &&
          plan.counters.reduced_birth_group_count == 2U &&
          plan.counters.continuation_group_count == 4U &&
          plan.counters.multifusion_group_count == 1U &&
          plan.counters.direct_hyperedge_reference_count == 4U &&
          plan.counters.residual_hyperedge_reference_count == 6U &&
          plan.counters.purely_residual_group_count == 3U &&
          plan.counters.silent_residual_continuation_group_count == 3U &&
          plan.counters.fully_redundant_residual_group_count == 1U,
      "E5 suffix has q0=2, q1=4, q2=1 and three silent residual continuations");
  if (plan.groups.size() != 7U) {
    return;
  }

  const std::vector<ExactFrozenIncidenceHgpActionGroup> expected_groups{
      {0U,
       0U,
       1U,
       0U,
       0U,
       0U,
       1U,
       1U,
       1U,
       0U,
       false,
       false,
       false,
       false,
       ExactFrozenIncidenceHgpAction::continuation},
      {1U,
       1U,
       1U,
       0U,
       0U,
       1U,
       2U,
       2U,
       1U,
       0U,
       false,
       false,
       false,
       false,
       ExactFrozenIncidenceHgpAction::multifusion},
      {2U,
       2U,
       0U,
       0U,
       2U,
       3U,
       1U,
       1U,
       0U,
       0U,
       true,
       false,
       true,
       false,
       ExactFrozenIncidenceHgpAction::continuation},
      {3U,
       2U,
       0U,
       2U,
       2U,
       4U,
       1U,
       1U,
       0U,
       0U,
       true,
       false,
       true,
       false,
       ExactFrozenIncidenceHgpAction::continuation},
      {4U,
       2U,
       0U,
       4U,
       2U,
       5U,
       1U,
       1U,
       0U,
       0U,
       true,
       false,
       true,
       true,
       ExactFrozenIncidenceHgpAction::continuation},
      {5U,
       2U,
       1U,
       6U,
       0U,
       6U,
       0U,
       0U,
       1U,
       0U,
       false,
       false,
       false,
       false,
       ExactFrozenIncidenceHgpAction::reduced_birth},
      {6U,
       3U,
       1U,
       6U,
       0U,
       6U,
       0U,
       0U,
       1U,
       0U,
       false,
       false,
       false,
       false,
       ExactFrozenIncidenceHgpAction::reduced_birth},
  };
  check(
      plan.groups == expected_groups &&
          plan.direct_hyperedge_indices ==
              std::vector<std::size_t>{1U, 5U, 2U, 7U} &&
          plan.residual_hyperedge_indices ==
              std::vector<std::size_t>{0U, 4U, 3U, 8U, 6U, 9U} &&
          plan.prior_root_ids ==
              std::vector<ExactFrozenIncidencePriorRootId>{
                  1000U, 2000U, 2001U, 3000U, 4000U, 5000U},
      "canonical groups preserve all direct/residual sources and sorted roots");
  check(
      plan.groups[4U].fully_redundant_residual_group &&
          plan.groups[4U].residual_hyperedge_count == 2U &&
          plan.counters.direct_added_point_count == 4U &&
          plan.counters.residual_added_point_count == 0U &&
          plan.counters.logical_output_entry_count == 23U &&
          plan.counters.logical_scratch_entry_count == 27U &&
          plan.required_scratch_entry_capacity == 36U &&
          plan.required_logical_output_entry_capacity == 26U,
      "the fully redundant residual group is retained and logical bounds are exact");
}

struct MixedBridgeFixture {
  std::array<ExactFrozenIncidenceHyperedge, 4U> hyperedges{{
      {0U, 0U, 2U},
      {1U, 2U, 2U},
      {2U, 4U, 2U},
      {3U, 6U, 2U},
  }};
  std::array<ExactFrozenIncidenceToken, 8U> tokens{{
      rooted(1U),
      equal_facet(10U),
      equal_facet(10U),
      rooted(2U),
      latent(5U),
      latent(6U),
      latent(6U),
      equal_facet(60U),
  }};
  std::array<ExactFrozenIncidenceHyperedgeProvenance, 4U> provenance{{
      {0U,
       ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family,
       0U,
       false,
       false},
      {1U,
       ExactFrozenIncidenceHyperedgeProvenanceKind::residual_incidence,
       1U,
       false,
       false},
      {2U,
       ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family,
       0U,
       false,
       false},
      {3U,
       ExactFrozenIncidenceHyperedgeProvenanceKind::residual_incidence,
       1U,
       false,
       false},
  }};
  std::array<ExactFrozenIncidenceRootAttachment, 2U> attachments{{
      {1U, 101U},
      {2U, 102U},
  }};
};

void check_mixed_bridges_authorize_q0_and_q2() {
  const MixedBridgeFixture fixture;
  const auto quotient = build_quotient(fixture.hyperedges, fixture.tokens);
  const auto plan = build_exact_direct_frozen_incidence_hgp_action_plan(
      fixture.hyperedges,
      fixture.tokens,
      unlimited_quotient_budget(),
      quotient,
      fixture.provenance,
      fixture.attachments,
      unlimited_action_budget());
  check(
      plan.certified_frozen_incidence_hgp_action_plan() &&
          plan.groups.size() == 2U &&
          plan.groups[0U].mixed_provenance &&
          plan.groups[0U].q_r == 2U &&
          plan.groups[0U].action ==
              ExactFrozenIncidenceHgpAction::multifusion &&
          plan.groups[1U].mixed_provenance &&
          plan.groups[1U].q_r == 0U &&
          plan.groups[1U].action ==
              ExactFrozenIncidenceHgpAction::reduced_birth &&
          plan.counters.mixed_provenance_group_count == 2U,
      "direct evidence authorizes q0/q2 components bridged to residual incidences");
}

void check_scope_rejections_are_atomic() {
  const auto rejects_scope = [](
                                 std::span<const ExactFrozenIncidenceHyperedge>
                                     hyperedges,
                                 std::span<const ExactFrozenIncidenceToken>
                                     tokens,
                                 std::span<const
                                               ExactFrozenIncidenceHyperedgeProvenance>
                                     provenance,
                                 std::span<const ExactFrozenIncidenceRootAttachment>
                                     attachments,
                                 const std::string& message) {
    const auto quotient = build_quotient(hyperedges, tokens);
    const auto plan = build_exact_direct_frozen_incidence_hgp_action_plan(
        hyperedges,
        tokens,
        unlimited_quotient_budget(),
        quotient,
        provenance,
        attachments,
        unlimited_action_budget());
    check(
        quotient.certified_frozen_incidence_quotient() &&
            plan.decision == ExactFrozenIncidenceHgpActionPlanDecision::
                                 no_frozen_incidence_hgp_action_plan_scope_rejected &&
            plan.atomic_empty_failure(),
        message);
  };

  const std::array<ExactFrozenIncidenceHyperedge, 1U> one_edge{{
      {0U, 0U, 1U},
  }};
  const std::array<ExactFrozenIncidenceToken, 1U> q0_tokens{{latent(1U)}};
  const std::array<ExactFrozenIncidenceHyperedgeProvenance, 1U>
      silent_residual{{
          {0U,
           ExactFrozenIncidenceHyperedgeProvenanceKind::residual_incidence,
           0U,
           true,
           false},
      }};
  rejects_scope(
      one_edge,
      q0_tokens,
      silent_residual,
      std::span<const ExactFrozenIncidenceRootAttachment>{},
      "a purely residual q_R=0 group is rejected atomically");

  const std::array<ExactFrozenIncidenceHyperedge, 1U> two_token_edge{{
      {0U, 0U, 2U},
  }};
  const std::array<ExactFrozenIncidenceToken, 2U> q2_tokens{{
      rooted(1U), rooted(2U)}};
  const std::array<ExactFrozenIncidenceRootAttachment, 2U> q2_roots{{
      {1U, 101U}, {2U, 102U}}};
  rejects_scope(
      two_token_edge,
      q2_tokens,
      silent_residual,
      q2_roots,
      "a purely residual q_R>=2 group is rejected atomically");

  const std::array<ExactFrozenIncidenceToken, 2U> q1_tokens{{
      rooted(1U), latent(2U)}};
  const std::array<ExactFrozenIncidenceRootAttachment, 1U> q1_root{{
      {1U, 101U},
  }};
  const std::array<ExactFrozenIncidenceHyperedgeProvenance, 1U>
      non_silent_residual{{
          {0U,
           ExactFrozenIncidenceHyperedgeProvenanceKind::residual_incidence,
           1U,
           false,
           false},
      }};
  rejects_scope(
      two_token_edge,
      q1_tokens,
      non_silent_residual,
      q1_root,
      "a purely residual q_R=1 group with an added point is rejected");

  const std::array<ExactFrozenIncidenceHyperedgeProvenance, 1U>
      uncertified_zero_residual{{
          {0U,
           ExactFrozenIncidenceHyperedgeProvenanceKind::residual_incidence,
           0U,
           false,
           false},
      }};
  rejects_scope(
      two_token_edge,
      q1_tokens,
      uncertified_zero_residual,
      q1_root,
      "a zero-count residual q_R=1 group still needs its local certificate");

  const std::array<ExactFrozenIncidenceToken, 1U> e_only_tokens{{
      equal_facet(9U),
  }};
  const std::array<ExactFrozenIncidenceHyperedgeProvenance, 1U> direct{{
      {0U,
       ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family,
       0U,
       false,
       false},
  }};
  rejects_scope(
      one_edge,
      e_only_tokens,
      direct,
      std::span<const ExactFrozenIncidenceRootAttachment>{},
      "an equal-facet-only group without any carrier is rejected");
}

void check_input_shape_and_fresh_quotient_gate() {
  const MixedBridgeFixture fixture;
  const auto quotient = build_quotient(fixture.hyperedges, fixture.tokens);

  auto wrong_attachment = fixture.attachments;
  wrong_attachment[0U].rooted_carrier_token_id = 9U;
  const auto wrong_attachment_plan =
      build_exact_direct_frozen_incidence_hgp_action_plan(
          fixture.hyperedges,
          fixture.tokens,
          unlimited_quotient_budget(),
          quotient,
          fixture.provenance,
          wrong_attachment,
          unlimited_action_budget());
  check(
      wrong_attachment_plan.decision ==
              ExactFrozenIncidenceHgpActionPlanDecision::
                  no_frozen_incidence_hgp_action_plan_input_shape_rejected &&
          wrong_attachment_plan.atomic_empty_failure(),
      "root attachments must exhaust the canonical rooted-carrier table");

  auto duplicate_root = fixture.attachments;
  duplicate_root[1U].prior_root_id = duplicate_root[0U].prior_root_id;
  const auto duplicate_plan =
      build_exact_direct_frozen_incidence_hgp_action_plan(
          fixture.hyperedges,
          fixture.tokens,
          unlimited_quotient_budget(),
          quotient,
          fixture.provenance,
          duplicate_root,
          unlimited_action_budget());
  check(
      duplicate_plan.decision ==
              ExactFrozenIncidenceHgpActionPlanDecision::
                  no_frozen_incidence_hgp_action_plan_input_shape_rejected &&
          duplicate_plan.atomic_empty_failure(),
      "one opaque prior root cannot be attached to two rooted carriers");

  auto bad_provenance = fixture.provenance;
  bad_provenance[0U].fully_redundant = true;
  const auto bad_provenance_plan =
      build_exact_direct_frozen_incidence_hgp_action_plan(
          fixture.hyperedges,
          fixture.tokens,
          unlimited_quotient_budget(),
          quotient,
          bad_provenance,
          fixture.attachments,
          unlimited_action_budget());
  check(
      bad_provenance_plan.decision ==
              ExactFrozenIncidenceHgpActionPlanDecision::
                  no_frozen_incidence_hgp_action_plan_input_shape_rejected &&
          bad_provenance_plan.atomic_empty_failure(),
      "fully redundant metadata requires a certified silent residual incidence");

  auto corrupted_quotient = quotient;
  ++corrupted_quotient.groups[0U].rooted_carrier_count;
  const auto corrupted_plan =
      build_exact_direct_frozen_incidence_hgp_action_plan(
          fixture.hyperedges,
          fixture.tokens,
          unlimited_quotient_budget(),
          corrupted_quotient,
          fixture.provenance,
          fixture.attachments,
          unlimited_action_budget());
  check(
      corrupted_plan.decision ==
              ExactFrozenIncidenceHgpActionPlanDecision::
                  no_frozen_incidence_hgp_action_plan_quotient_rejected &&
          corrupted_plan.atomic_empty_failure(),
      "the planner freshly replays and rejects a mutated source quotient");
}

void check_every_budget_cap_is_preflighted_atomically() {
  const MixedBridgeFixture fixture;
  const auto quotient = build_quotient(fixture.hyperedges, fixture.tokens);
  const ExactFrozenIncidenceHgpActionPlanBudget exact_budget{
      4U, 4U, 2U, 4U, 4U, 4U, 2U, 14U, 10U};
  const auto exact = build_exact_direct_frozen_incidence_hgp_action_plan(
      fixture.hyperedges,
      fixture.tokens,
      unlimited_quotient_budget(),
      quotient,
      fixture.provenance,
      fixture.attachments,
      exact_budget);
  check(
      exact.certified_frozen_incidence_hgp_action_plan() &&
          exact.required_scratch_entry_capacity == 14U &&
          exact.required_logical_output_entry_capacity == 10U,
      "the exact conservative action-plan budget is sufficient");

  const std::array<ExactFrozenIncidenceHgpActionPlanBudget, 9U>
      insufficient{{
          {3U, 4U, 2U, 4U, 4U, 4U, 2U, 14U, 10U},
          {4U, 3U, 2U, 4U, 4U, 4U, 2U, 14U, 10U},
          {4U, 4U, 1U, 4U, 4U, 4U, 2U, 14U, 10U},
          {4U, 4U, 2U, 3U, 4U, 4U, 2U, 14U, 10U},
          {4U, 4U, 2U, 4U, 3U, 4U, 2U, 14U, 10U},
          {4U, 4U, 2U, 4U, 4U, 3U, 2U, 14U, 10U},
          {4U, 4U, 2U, 4U, 4U, 4U, 1U, 14U, 10U},
          {4U, 4U, 2U, 4U, 4U, 4U, 2U, 13U, 10U},
          {4U, 4U, 2U, 4U, 4U, 4U, 2U, 14U, 9U},
      }};
  for (const auto& budget : insufficient) {
    const auto plan = build_exact_direct_frozen_incidence_hgp_action_plan(
        fixture.hyperedges,
        fixture.tokens,
        unlimited_quotient_budget(),
        quotient,
        fixture.provenance,
        fixture.attachments,
        budget);
    check(
        plan.decision == ExactFrozenIncidenceHgpActionPlanDecision::
                             no_frozen_incidence_hgp_action_plan_budget_exhausted &&
            !plan.source_quotient_fresh_streaming_verified &&
            plan.atomic_empty_failure(),
        "each one-short action cap rejects before quotient replay or allocation");
  }
}

void check_fresh_streaming_verifier_and_mutations() {
  const E5SuffixFixture fixture;
  const auto quotient = build_quotient(fixture.hyperedges, fixture.tokens);
  const auto quotient_budget = unlimited_quotient_budget();
  const auto action_budget = unlimited_action_budget();
  const auto plan = build_exact_direct_frozen_incidence_hgp_action_plan(
      fixture.hyperedges,
      fixture.tokens,
      quotient_budget,
      quotient,
      fixture.provenance,
      fixture.attachments,
      action_budget);
  const auto verified =
      verify_exact_direct_frozen_incidence_hgp_action_plan_streaming(
          fixture.hyperedges,
          fixture.tokens,
          quotient_budget,
          quotient,
          fixture.provenance,
          fixture.attachments,
          action_budget,
          plan);
  check(
      complete_streaming_verification(verified) &&
          verified.source_hyperedge_scan_count ==
              fixture.hyperedges.size() &&
          verified.source_provenance_scan_count ==
              fixture.provenance.size() &&
          verified.source_root_attachment_scan_count ==
              fixture.attachments.size(),
      "fresh verifier replays quotient, provenance, roots and flat plan slices");

  auto mutated_group = plan;
  mutated_group.groups[0U].q_r = 2U;
  const auto group_verification =
      verify_exact_direct_frozen_incidence_hgp_action_plan_streaming(
          fixture.hyperedges,
          fixture.tokens,
          quotient_budget,
          quotient,
          fixture.provenance,
          fixture.attachments,
          action_budget,
          mutated_group);
  check(
      !group_verification.groups_certified &&
          group_verification.direct_hyperedge_slices_certified &&
          group_verification.prior_root_slices_certified &&
          !group_verification.result_certified,
      "fresh replay rejects a mutated q_R action group granularly");

  auto mutated_direct = plan;
  mutated_direct.direct_hyperedge_indices[0U] = 5U;
  const auto direct_verification =
      verify_exact_direct_frozen_incidence_hgp_action_plan_streaming(
          fixture.hyperedges,
          fixture.tokens,
          quotient_budget,
          quotient,
          fixture.provenance,
          fixture.attachments,
          action_budget,
          mutated_direct);
  check(
      !direct_verification.direct_hyperedge_slices_certified &&
          direct_verification.residual_hyperedge_slices_certified &&
          !direct_verification.result_certified,
      "fresh replay rejects a direct source moved into the wrong group");

  auto mutated_residual = plan;
  mutated_residual.residual_hyperedge_indices[0U] = 3U;
  const auto residual_verification =
      verify_exact_direct_frozen_incidence_hgp_action_plan_streaming(
          fixture.hyperedges,
          fixture.tokens,
          quotient_budget,
          quotient,
          fixture.provenance,
          fixture.attachments,
          action_budget,
          mutated_residual);
  check(
      direct_verification.residual_hyperedge_slices_certified &&
          !residual_verification.residual_hyperedge_slices_certified &&
          !residual_verification.result_certified,
      "fresh replay rejects a residual incidence moved into the wrong group");

  auto mutated_root = plan;
  mutated_root.prior_root_ids[0U] = 3000U;
  const auto root_verification =
      verify_exact_direct_frozen_incidence_hgp_action_plan_streaming(
          fixture.hyperedges,
          fixture.tokens,
          quotient_budget,
          quotient,
          fixture.provenance,
          fixture.attachments,
          action_budget,
          mutated_root);
  check(
      !root_verification.prior_root_slices_certified &&
          root_verification.groups_certified &&
          !root_verification.result_certified,
      "fresh replay rejects an opaque root moved into the wrong group");

  auto mutated_counter = plan;
  ++mutated_counter.counters.continuation_group_count;
  const auto counter_verification =
      verify_exact_direct_frozen_incidence_hgp_action_plan_streaming(
          fixture.hyperedges,
          fixture.tokens,
          quotient_budget,
          quotient,
          fixture.provenance,
          fixture.attachments,
          action_budget,
          mutated_counter);
  check(
      !counter_verification.counters_certified &&
          !counter_verification.result_certified,
      "fresh replay rejects a mutated action counter");

  auto mutated_fact = plan;
  mutated_fact.pure_residual_scope_rule_certified = false;
  const auto fact_verification =
      verify_exact_direct_frozen_incidence_hgp_action_plan_streaming(
          fixture.hyperedges,
          fixture.tokens,
          quotient_budget,
          quotient,
          fixture.provenance,
          fixture.attachments,
          action_budget,
          mutated_fact);
  check(
      !fact_verification.result_facts_certified &&
          !fact_verification.result_certified,
      "fresh replay rejects a mutated scope fact");

  auto mutated_non_scope = plan;
  mutated_non_scope.nodes_created_or_mutated = true;
  const auto non_scope_verification =
      verify_exact_direct_frozen_incidence_hgp_action_plan_streaming(
          fixture.hyperedges,
          fixture.tokens,
          quotient_budget,
          quotient,
          fixture.provenance,
          fixture.attachments,
          action_budget,
          mutated_non_scope);
  check(
      !non_scope_verification.mutation_non_scope_certified &&
          !non_scope_verification.result_certified,
      "fresh replay rejects an invented node mutation");

  auto mutated_decision = plan;
  mutated_decision.decision =
      ExactFrozenIncidenceHgpActionPlanDecision::not_certified;
  const auto decision_verification =
      verify_exact_direct_frozen_incidence_hgp_action_plan_streaming(
          fixture.hyperedges,
          fixture.tokens,
          quotient_budget,
          quotient,
          fixture.provenance,
          fixture.attachments,
          action_budget,
          mutated_decision);
  check(
      !decision_verification.decision_and_scope_certified &&
          !decision_verification.result_certified,
      "fresh replay rejects a mutated action-plan decision");
}

}  // namespace

int main() {
  check_default_result_is_not_an_atomic_failure();
  check_e5_suffix_shape_and_actions();
  check_mixed_bridges_authorize_q0_and_q2();
  check_scope_rejections_are_atomic();
  check_input_shape_and_fresh_quotient_gate();
  check_every_budget_cap_is_preflighted_atomically();
  check_fresh_streaming_verifier_and_mutations();

  if (failures != 0) {
    std::cerr << failures
              << " direct frozen-incidence HGP action-plan test(s) failed\n";
    return 1;
  }
  std::cout << "direct frozen-incidence HGP action-plan tests passed\n";
  return 0;
}
