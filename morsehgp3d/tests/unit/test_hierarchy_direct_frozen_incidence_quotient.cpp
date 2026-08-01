#include "morsehgp3d/hierarchy/direct_frozen_incidence_quotient.hpp"

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

[[nodiscard]] ExactFrozenIncidenceQuotientBudget unlimited_budget() {
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
    const ExactFrozenIncidenceQuotientStreamingVerification& verification) {
  return verification.requested_budget_certified &&
         verification.requirements_certified &&
         verification.input_shape_certified &&
         verification.hyperedge_bindings_certified &&
         verification.token_bindings_certified &&
         verification.groups_certified &&
         verification.group_tokens_certified &&
         verification.counters_certified &&
         verification.result_facts_certified &&
         verification.combinatorial_non_scope_certified &&
         verification.decision_and_scope_certified &&
         verification.no_second_persistent_output_arena_certified &&
         verification.fresh_streaming_replay_certified &&
         verification.result_certified;
}

struct DiscriminatingFixture {
  std::array<ExactFrozenIncidenceHyperedge, 7U> hyperedges{{
      {0U, 0U, 2U},
      {1U, 2U, 2U},
      {2U, 4U, 2U},
      {3U, 6U, 2U},
      {4U, 8U, 2U},
      {5U, 10U, 3U},
      {6U, 13U, 3U},
  }};
  std::array<ExactFrozenIncidenceToken, 16U> tokens{{
      rooted(1U),
      equal_facet(100U),
      equal_facet(100U),
      rooted(2U),
      rooted(5U),
      latent(50U),
      latent(50U),
      rooted(6U),
      latent(7U),
      equal_facet(8U),
      rooted(8U),
      latent(81U),
      equal_facet(82U),
      rooted(9U),
      rooted(10U),
      rooted(11U),
  }};
};

void check_empty_quotient() {
  const auto result = build_exact_direct_frozen_incidence_quotient(
      std::span<const ExactFrozenIncidenceHyperedge>{},
      std::span<const ExactFrozenIncidenceToken>{},
      ExactFrozenIncidenceQuotientBudget{});
  check(
      result.certified_frozen_incidence_quotient() &&
          result.hyperedge_bindings.empty() &&
          result.token_bindings.empty() && result.groups.empty() &&
          result.group_tokens.empty() &&
          result.counters.logical_output_entry_count == 0U &&
          result.counters.logical_scratch_entry_count == 0U,
      "the empty typed-incidence quotient is a certified empty result");
}

void check_equal_chain_latent_bridge_and_q_r_partition() {
  const DiscriminatingFixture fixture;
  const auto result = build_exact_direct_frozen_incidence_quotient(
      fixture.hyperedges, fixture.tokens, unlimited_budget());
  check(
      result.certified_frozen_incidence_quotient() &&
          result.groups.size() == 5U &&
          result.counters.q_r_zero_group_count == 1U &&
          result.counters.q_r_one_group_count == 1U &&
          result.counters.q_r_multiple_group_count == 3U,
      "equal and latent bridges close before the q_R=0/1/2+ partition");
  if (result.groups.size() != 5U) {
    return;
  }

  const std::vector<ExactFrozenIncidenceQuotientGroup> expected_groups{
      {0U,
       0U,
       3U,
       2U,
       0U,
       1U,
       2U,
       ExactFrozenIncidenceQuotientDisposition::q_r_multiple},
      {1U,
       3U,
       3U,
       2U,
       1U,
       0U,
       2U,
       ExactFrozenIncidenceQuotientDisposition::q_r_multiple},
      {2U,
       6U,
       3U,
       1U,
       1U,
       1U,
       1U,
       ExactFrozenIncidenceQuotientDisposition::q_r_one},
      {3U,
       9U,
       3U,
       3U,
       0U,
       0U,
       1U,
       ExactFrozenIncidenceQuotientDisposition::q_r_multiple},
      {4U,
       12U,
       2U,
       0U,
       1U,
       1U,
       1U,
       ExactFrozenIncidenceQuotientDisposition::q_r_zero},
  };
  const std::vector<ExactFrozenIncidenceToken> expected_group_tokens{
      rooted(1U),
      rooted(2U),
      equal_facet(100U),
      rooted(5U),
      rooted(6U),
      latent(50U),
      rooted(8U),
      latent(81U),
      equal_facet(82U),
      rooted(9U),
      rooted(10U),
      rooted(11U),
      latent(7U),
      equal_facet(8U),
  };
  const std::vector<ExactFrozenIncidenceHyperedgeBinding>
      expected_hyperedge_bindings{
          {0U, 0U, 2U, 0U},
          {1U, 2U, 2U, 0U},
          {2U, 4U, 2U, 1U},
          {3U, 6U, 2U, 1U},
          {4U, 8U, 2U, 4U},
          {5U, 10U, 3U, 2U},
          {6U, 13U, 3U, 3U},
      };
  check(
      result.groups == expected_groups &&
          result.group_tokens == expected_group_tokens &&
          result.hyperedge_bindings == expected_hyperedge_bindings,
      "groups and source bindings expose the complete canonical closure");
  check(
      result.counters.distinct_token_count == 14U &&
          result.counters.distinct_rooted_carrier_count == 8U &&
          result.counters.distinct_latent_carrier_count == 3U &&
          result.counters.distinct_equal_facet_count == 3U &&
          result.counters.maximum_hyperedge_reference_count == 3U &&
          result.counters.maximum_group_token_count == 3U &&
          result.counters.maximum_group_q_r == 3U &&
          result.counters.logical_output_entry_count == 40U &&
          result.counters.logical_scratch_entry_count == 49U &&
          result.required_scratch_entry_capacity == 55U &&
          result.required_logical_output_entry_capacity == 46U,
      "typed distinct counts and conservative/actual logical sizes are exact");
}

void check_typed_identity_and_duplicate_references() {
  const std::array<ExactFrozenIncidenceHyperedge, 1U> hyperedges{{
      {0U, 0U, 4U},
  }};
  const std::array<ExactFrozenIncidenceToken, 4U> tokens{{
      rooted(7U), latent(7U), equal_facet(7U), rooted(7U)}};
  const auto result = build_exact_direct_frozen_incidence_quotient(
      hyperedges, tokens, unlimited_budget());
  check(
      result.certified_frozen_incidence_quotient() &&
          result.group_tokens ==
              std::vector<ExactFrozenIncidenceToken>{
                  rooted(7U), latent(7U), equal_facet(7U)} &&
          result.groups.size() == 1U &&
          result.groups[0].rooted_carrier_count == 1U &&
          result.groups[0].latent_carrier_count == 1U &&
          result.groups[0].equal_facet_count == 1U &&
          result.groups[0].disposition ==
              ExactFrozenIncidenceQuotientDisposition::q_r_one,
      "the typed pair is the identity and repeated R references count once");
}

void check_canonical_output_under_permutations() {
  const DiscriminatingFixture fixture;
  const auto reference = build_exact_direct_frozen_incidence_quotient(
      fixture.hyperedges, fixture.tokens, unlimited_budget());

  const std::array<ExactFrozenIncidenceHyperedge, 7U> hyperedges{{
      {0U, 0U, 3U},
      {1U, 3U, 2U},
      {2U, 5U, 2U},
      {3U, 7U, 2U},
      {4U, 9U, 3U},
      {5U, 12U, 2U},
      {6U, 14U, 2U},
  }};
  const std::array<ExactFrozenIncidenceToken, 16U> tokens{{
      rooted(11U),
      rooted(9U),
      rooted(10U),
      equal_facet(8U),
      latent(7U),
      rooted(6U),
      latent(50U),
      rooted(2U),
      equal_facet(100U),
      equal_facet(82U),
      latent(81U),
      rooted(8U),
      equal_facet(100U),
      rooted(1U),
      latent(50U),
      rooted(5U),
  }};
  const auto permuted = build_exact_direct_frozen_incidence_quotient(
      hyperedges, tokens, unlimited_budget());
  check(
      permuted.certified_frozen_incidence_quotient() &&
          permuted.groups == reference.groups &&
          permuted.group_tokens == reference.group_tokens &&
          permuted.token_bindings == reference.token_bindings &&
          permuted.counters == reference.counters,
      "token order and hyperedge order do not alter canonical quotient arenas");
  check(
      permuted.hyperedge_bindings ==
          std::vector<ExactFrozenIncidenceHyperedgeBinding>{
              {0U, 0U, 3U, 3U},
              {1U, 3U, 2U, 4U},
              {2U, 5U, 2U, 1U},
              {3U, 7U, 2U, 0U},
              {4U, 9U, 3U, 2U},
              {5U, 12U, 2U, 0U},
              {6U, 14U, 2U, 1U},
          },
      "source binding provenance follows the permuted CSR without changing groups");
}

void check_malformed_input_is_rejected_atomically() {
  const std::array<ExactFrozenIncidenceToken, 2U> tokens{{
      rooted(1U), latent(2U)}};
  const auto rejects = [](
                           std::span<const ExactFrozenIncidenceHyperedge>
                               hyperedges,
                           std::span<const ExactFrozenIncidenceToken>
                               references) {
    const auto result = build_exact_direct_frozen_incidence_quotient(
        hyperedges, references, unlimited_budget());
    check(
        result.decision == ExactFrozenIncidenceQuotientDecision::
                               no_frozen_incidence_quotient_input_shape_rejected &&
            result.budget_preflight_certified &&
            !result.input_shape_certified && result.atomic_empty_failure(),
        "malformed CSR or token kind is rejected with empty atomic output");
  };

  const std::array<ExactFrozenIncidenceHyperedge, 1U> wrong_index{{
      {1U, 0U, 2U},
  }};
  const std::array<ExactFrozenIncidenceHyperedge, 1U> wrong_offset{{
      {0U, 1U, 1U},
  }};
  const std::array<ExactFrozenIncidenceHyperedge, 1U> empty_edge{{
      {0U, 0U, 0U},
  }};
  const std::array<ExactFrozenIncidenceHyperedge, 1U> out_of_bounds{{
      {0U, 0U, 3U},
  }};
  const std::array<ExactFrozenIncidenceHyperedge, 1U> trailing_reference{{
      {0U, 0U, 1U},
  }};
  rejects(wrong_index, tokens);
  rejects(wrong_offset, tokens);
  rejects(empty_edge, std::span<const ExactFrozenIncidenceToken>{});
  rejects(out_of_bounds, tokens);
  rejects(trailing_reference, tokens);

  const std::array<ExactFrozenIncidenceHyperedge, 1U> valid_shape{{
      {0U, 0U, 1U},
  }};
  const std::array<ExactFrozenIncidenceToken, 1U> invalid_kind{{
      {static_cast<ExactFrozenIncidenceTokenKind>(99U), 4U},
  }};
  rejects(valid_shape, invalid_kind);
}

void check_every_budget_cap_is_preflighted_atomically() {
  const std::array<ExactFrozenIncidenceHyperedge, 2U> hyperedges{{
      {0U, 0U, 2U},
      {1U, 2U, 2U},
  }};
  const std::array<ExactFrozenIncidenceToken, 4U> tokens{{
      rooted(1U), latent(2U), equal_facet(3U), equal_facet(4U)}};
  const ExactFrozenIncidenceQuotientBudget exact_budget{
      2U, 4U, 4U, 2U, 2U, 4U, 4U, 14U, 12U};
  const auto exact = build_exact_direct_frozen_incidence_quotient(
      hyperedges, tokens, exact_budget);
  check(
      exact.certified_frozen_incidence_quotient() &&
          exact.required_scratch_entry_capacity == 14U &&
          exact.required_logical_output_entry_capacity == 12U,
      "the exact conservative typed-incidence budget is sufficient");

  const std::array<ExactFrozenIncidenceQuotientBudget, 9U> insufficient{{
      {1U, 4U, 4U, 2U, 2U, 4U, 4U, 14U, 12U},
      {2U, 3U, 4U, 2U, 2U, 4U, 4U, 14U, 12U},
      {2U, 4U, 3U, 2U, 2U, 4U, 4U, 14U, 12U},
      {2U, 4U, 4U, 1U, 2U, 4U, 4U, 14U, 12U},
      {2U, 4U, 4U, 2U, 1U, 4U, 4U, 14U, 12U},
      {2U, 4U, 4U, 2U, 2U, 3U, 4U, 14U, 12U},
      {2U, 4U, 4U, 2U, 2U, 4U, 3U, 14U, 12U},
      {2U, 4U, 4U, 2U, 2U, 4U, 4U, 13U, 12U},
      {2U, 4U, 4U, 2U, 2U, 4U, 4U, 14U, 11U},
  }};
  for (const ExactFrozenIncidenceQuotientBudget& budget : insufficient) {
    const auto result = build_exact_direct_frozen_incidence_quotient(
        hyperedges, tokens, budget);
    check(
        result.decision == ExactFrozenIncidenceQuotientDecision::
                               no_frozen_incidence_quotient_budget_exhausted &&
            !result.budget_preflight_certified &&
            !result.input_shape_certified && result.atomic_empty_failure(),
        "each one-short cap rejects before scratch or output allocation");
  }
}

void check_fresh_streaming_verifier_and_mutations() {
  const DiscriminatingFixture fixture;
  const ExactFrozenIncidenceQuotientBudget budget = unlimited_budget();
  const auto result = build_exact_direct_frozen_incidence_quotient(
      fixture.hyperedges, fixture.tokens, budget);
  const auto verified =
      verify_exact_direct_frozen_incidence_quotient_streaming(
          fixture.hyperedges, fixture.tokens, budget, result);
  check(
      complete_streaming_verification(verified) &&
          verified.source_hyperedge_scan_count ==
              fixture.hyperedges.size() &&
          verified.source_token_reference_scan_count ==
              fixture.tokens.size(),
      "fresh streaming replay certifies every flat arena without a second payload");

  auto mutated_hyperedge_binding = result;
  mutated_hyperedge_binding.hyperedge_bindings[0].group_index = 1U;
  const auto hyperedge_binding_verification =
      verify_exact_direct_frozen_incidence_quotient_streaming(
          fixture.hyperedges,
          fixture.tokens,
          budget,
          mutated_hyperedge_binding);
  check(
      !hyperedge_binding_verification.hyperedge_bindings_certified &&
          hyperedge_binding_verification.token_bindings_certified &&
          hyperedge_binding_verification.groups_certified &&
          !hyperedge_binding_verification.result_certified,
      "streaming replay rejects a mutated hyperedge binding granularly");

  auto mutated_token_binding = result;
  mutated_token_binding.token_bindings[0].group_index = 1U;
  const auto token_binding_verification =
      verify_exact_direct_frozen_incidence_quotient_streaming(
          fixture.hyperedges,
          fixture.tokens,
          budget,
          mutated_token_binding);
  check(
      token_binding_verification.hyperedge_bindings_certified &&
          !token_binding_verification.token_bindings_certified &&
          token_binding_verification.groups_certified &&
          !token_binding_verification.result_certified,
      "streaming replay rejects a mutated token binding granularly");

  auto mutated_group = result;
  --mutated_group.groups[0].rooted_carrier_count;
  const auto group_verification =
      verify_exact_direct_frozen_incidence_quotient_streaming(
          fixture.hyperedges, fixture.tokens, budget, mutated_group);
  check(
      group_verification.hyperedge_bindings_certified &&
          group_verification.token_bindings_certified &&
          !group_verification.groups_certified &&
          group_verification.group_tokens_certified &&
          !group_verification.result_certified,
      "streaming replay rejects a mutated q_R group record granularly");

  auto mutated_group_token = result;
  mutated_group_token.group_tokens[0] = latent(1U);
  const auto token_verification =
      verify_exact_direct_frozen_incidence_quotient_streaming(
          fixture.hyperedges, fixture.tokens, budget, mutated_group_token);
  check(
      token_verification.groups_certified &&
          !token_verification.group_tokens_certified &&
          !token_verification.result_certified,
      "streaming replay rejects a mutated typed token in a group slice");

  auto mutated_counters = result;
  ++mutated_counters.counters.q_r_one_group_count;
  const auto counter_verification =
      verify_exact_direct_frozen_incidence_quotient_streaming(
          fixture.hyperedges, fixture.tokens, budget, mutated_counters);
  check(
      !counter_verification.counters_certified &&
          !counter_verification.result_certified,
      "streaming replay rejects a mutated q_R counter");

  auto mutated_fact = result;
  mutated_fact.q_r_counts_distinct_rooted_carriers_only = false;
  const auto fact_verification =
      verify_exact_direct_frozen_incidence_quotient_streaming(
          fixture.hyperedges, fixture.tokens, budget, mutated_fact);
  check(
      !fact_verification.result_facts_certified &&
          !fact_verification.result_certified,
      "streaming replay rejects a mutated positive structural fact");

  auto mutated_non_scope = result;
  mutated_non_scope.geometry_gamma_facets_or_cells_materialized = true;
  const auto non_scope_verification =
      verify_exact_direct_frozen_incidence_quotient_streaming(
          fixture.hyperedges, fixture.tokens, budget, mutated_non_scope);
  check(
      !non_scope_verification.combinatorial_non_scope_certified &&
          !non_scope_verification.result_certified,
      "streaming replay rejects an invented Gamma or geometry capability");

  auto mutated_decision = result;
  mutated_decision.decision =
      ExactFrozenIncidenceQuotientDecision::not_certified;
  const auto decision_verification =
      verify_exact_direct_frozen_incidence_quotient_streaming(
          fixture.hyperedges, fixture.tokens, budget, mutated_decision);
  check(
      !decision_verification.decision_and_scope_certified &&
          !decision_verification.result_certified,
      "streaming replay rejects a mutated decision");
}

}  // namespace

int main() {
  check_empty_quotient();
  check_equal_chain_latent_bridge_and_q_r_partition();
  check_typed_identity_and_duplicate_references();
  check_canonical_output_under_permutations();
  check_malformed_input_is_rejected_atomically();
  check_every_budget_cap_is_preflighted_atomically();
  check_fresh_streaming_verifier_and_mutations();

  if (failures != 0) {
    std::cerr << failures
              << " direct frozen-incidence quotient test(s) failed\n";
    return 1;
  }
  std::cout << "direct frozen-incidence quotient tests passed\n";
  return 0;
}
