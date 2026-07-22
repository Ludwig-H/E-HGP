#include "morsehgp3d/hierarchy/direct_frozen_root_quotient.hpp"

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

[[nodiscard]] ExactFrozenRootQuotientBudget unlimited_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] bool complete_streaming_verification(
    const ExactFrozenRootQuotientStreamingVerification& verification) {
  return verification.requested_budget_certified &&
         verification.requirements_certified &&
         verification.input_shape_certified &&
         verification.hyperedge_bindings_certified &&
         verification.groups_certified &&
         verification.group_root_ids_certified &&
         verification.counters_certified &&
         verification.result_facts_certified &&
         verification.root_only_non_scope_certified &&
         verification.decision_and_scope_certified &&
         verification.no_second_persistent_output_arena_certified &&
         verification.fresh_streaming_replay_certified &&
         verification.result_certified;
}

void check_empty_quotient() {
  const auto result = build_exact_direct_frozen_root_quotient(
      std::span<const ExactFrozenRootHyperedge>{},
      std::span<const ExactFrozenRootId>{},
      ExactFrozenRootQuotientBudget{});
  check(
      result.certified_frozen_root_quotient() &&
          result.hyperedge_bindings.empty() && result.groups.empty() &&
          result.group_root_ids.empty() &&
          result.counters.logical_output_entry_count == 0U &&
          result.counters.logical_scratch_entry_count == 0U,
      "the empty frozen quotient is a certified empty structural result");
}

void check_one_root_class_and_duplicate_root_references() {
  const std::array<ExactFrozenRootHyperedge, 1U> hyperedges{{
      {0U, 0U, 3U},
  }};
  constexpr ExactFrozenRootId root_id =
      std::numeric_limits<ExactFrozenRootId>::max();
  const std::array<ExactFrozenRootId, 3U> roots{
      root_id, root_id, root_id};
  const auto result = build_exact_direct_frozen_root_quotient(
      hyperedges, roots, unlimited_budget());
  check(
      result.certified_frozen_root_quotient() &&
          result.groups.size() == 1U &&
          result.groups[0].root_count == 1U &&
          result.groups[0].hyperedge_count == 1U &&
          result.groups[0].disposition ==
              ExactFrozenRootQuotientDisposition::one_root_class &&
          result.group_root_ids ==
              std::vector<ExactFrozenRootId>{root_id} &&
          result.hyperedge_bindings ==
              std::vector<ExactFrozenRootHyperedgeBinding>{
                  {0U, 0U, 3U, 0U}} &&
          result.counters.one_root_group_count == 1U &&
          result.counters.multiple_root_group_count == 0U,
      "a repeated singleton root remains an explicit one-root class");
}

void check_chain_and_canonical_flat_groups() {
  const std::array<ExactFrozenRootHyperedge, 3U> hyperedges{{
      {0U, 0U, 3U},
      {1U, 3U, 2U},
      {2U, 5U, 1U},
  }};
  const std::array<ExactFrozenRootId, 6U> roots{7U, 3U, 7U, 11U, 7U, 100U};
  const auto result = build_exact_direct_frozen_root_quotient(
      hyperedges, roots, unlimited_budget());
  check(
      result.certified_frozen_root_quotient() &&
          result.groups.size() == 2U &&
          result.group_root_ids ==
              std::vector<ExactFrozenRootId>({3U, 7U, 11U, 100U}) &&
          result.hyperedge_bindings ==
              std::vector<ExactFrozenRootHyperedgeBinding>(
                  {{0U, 0U, 3U, 0U},
                   {1U, 3U, 2U, 0U},
                   {2U, 5U, 1U, 1U}}),
      "shared roots close a chain while a disjoint q=1 edge stays separate");
  if (result.groups.size() != 2U) {
    return;
  }
  check(
      result.groups[0] == ExactFrozenRootQuotientGroup{
                              0U,
                              0U,
                              3U,
                              2U,
                              ExactFrozenRootQuotientDisposition::
                                  multiple_root_class} &&
          result.groups[1] == ExactFrozenRootQuotientGroup{
                                  1U,
                                  3U,
                                  1U,
                                  1U,
                                  ExactFrozenRootQuotientDisposition::
                                      one_root_class} &&
          result.counters.maximum_hyperedge_reference_count == 3U &&
          result.counters.maximum_group_root_count == 3U &&
          result.counters.logical_output_entry_count == 9U &&
          result.counters.logical_scratch_entry_count == 16U,
      "groups expose canonical CSR roots, source counts and exact logical sizes");

  const std::array<ExactFrozenRootId, 6U> permuted_roots{
      7U, 7U, 3U, 7U, 11U, 100U};
  const auto permuted = build_exact_direct_frozen_root_quotient(
      hyperedges, permuted_roots, unlimited_budget());
  check(
      result == permuted,
      "root-reference order inside each hyperedge does not alter the canonical quotient");
}

void check_interleaved_components_have_contiguous_root_slices() {
  const std::array<ExactFrozenRootHyperedge, 2U> hyperedges{{
      {0U, 0U, 2U},
      {1U, 2U, 2U},
  }};
  const std::array<ExactFrozenRootId, 4U> roots{1U, 4U, 2U, 3U};
  const auto result = build_exact_direct_frozen_root_quotient(
      hyperedges, roots, unlimited_budget());
  check(
      result.certified_frozen_root_quotient() &&
          result.groups ==
              std::vector<ExactFrozenRootQuotientGroup>(
                  {{0U,
                    0U,
                    2U,
                    1U,
                    ExactFrozenRootQuotientDisposition::
                        multiple_root_class},
                   {1U,
                    2U,
                    2U,
                    1U,
                    ExactFrozenRootQuotientDisposition::
                        multiple_root_class}}) &&
          result.group_root_ids ==
              std::vector<ExactFrozenRootId>({1U, 4U, 2U, 3U}),
      "interleaved RootIds are packed into disjoint contiguous group slices");
}

void check_root_only_non_scope_is_explicit() {
  const std::array<ExactFrozenRootHyperedge, 1U> hyperedges{{
      {0U, 0U, 1U},
  }};
  const std::array<ExactFrozenRootId, 1U> roots{5U};
  const auto result = build_exact_direct_frozen_root_quotient(
      hyperedges, roots, unlimited_budget());
  check(
      result.certified_frozen_root_quotient() &&
          !result.external_root_snapshot_membership_checked &&
          !result.zero_root_hyperedges_supported &&
          !result.latent_facet_tokens_supported &&
          !result.hgp_batch_actions_claimed &&
          !result.root_attachment_or_global_mutation_performed,
      "the ABI claims only a closure of externally certified root labels");

  const std::array<ExactFrozenRootHyperedge, 1U> zero_root_edge{{
      {0U, 0U, 0U},
  }};
  const auto rejected = build_exact_direct_frozen_root_quotient(
      zero_root_edge,
      std::span<const ExactFrozenRootId>{},
      unlimited_budget());
  check(
      rejected.decision == ExactFrozenRootQuotientDecision::
                               no_frozen_root_quotient_input_shape_rejected &&
          !rejected.zero_root_hyperedges_supported &&
          !rejected.latent_facet_tokens_supported &&
          !rejected.hgp_batch_actions_claimed,
      "q_R=0 mirror events remain outside the root-only quotient ABI");
}

void check_streaming_verifier_and_mutation_rejection() {
  const std::array<ExactFrozenRootHyperedge, 3U> hyperedges{{
      {0U, 0U, 3U},
      {1U, 3U, 2U},
      {2U, 5U, 1U},
  }};
  const std::array<ExactFrozenRootId, 6U> roots{
      7U, 3U, 7U, 11U, 7U, 100U};
  const ExactFrozenRootQuotientBudget budget = unlimited_budget();
  const auto result = build_exact_direct_frozen_root_quotient(
      hyperedges, roots, budget);
  const auto verified =
      verify_exact_direct_frozen_root_quotient_streaming(
          hyperedges, roots, budget, result);
  check(
      complete_streaming_verification(verified) &&
          verified.source_hyperedge_scan_count == hyperedges.size() &&
          verified.source_root_reference_scan_count == roots.size(),
      "streaming replay certifies every flat arena without a second payload");

  auto mutated_binding = result;
  mutated_binding.hyperedge_bindings[0].group_index = 1U;
  const auto binding_verification =
      verify_exact_direct_frozen_root_quotient_streaming(
          hyperedges, roots, budget, mutated_binding);
  check(
      !binding_verification.hyperedge_bindings_certified &&
          binding_verification.groups_certified &&
          binding_verification.group_root_ids_certified &&
          !binding_verification.fresh_streaming_replay_certified &&
          !binding_verification.result_certified,
      "streaming replay rejects a mutated hyperedge binding granularly");

  auto mutated_group = result;
  ++mutated_group.groups[0].hyperedge_count;
  const auto group_verification =
      verify_exact_direct_frozen_root_quotient_streaming(
          hyperedges, roots, budget, mutated_group);
  check(
      group_verification.hyperedge_bindings_certified &&
          !group_verification.groups_certified &&
          group_verification.group_root_ids_certified &&
          !group_verification.result_certified,
      "streaming replay rejects a mutated group record granularly");

  auto mutated_root = result;
  mutated_root.group_root_ids[2] = 100U;
  const auto root_verification =
      verify_exact_direct_frozen_root_quotient_streaming(
          hyperedges, roots, budget, mutated_root);
  check(
      root_verification.groups_certified &&
          !root_verification.group_root_ids_certified &&
          !root_verification.result_certified,
      "streaming replay rejects a valid root moved into the wrong group");

  auto mutated_fact = result;
  mutated_fact.equivalence_closure_certified = false;
  const auto fact_verification =
      verify_exact_direct_frozen_root_quotient_streaming(
          hyperedges, roots, budget, mutated_fact);
  check(
      !fact_verification.result_facts_certified &&
          !fact_verification.result_certified,
      "streaming replay rejects a mutated structural fact");

  auto mutated_non_scope = result;
  mutated_non_scope.latent_facet_tokens_supported = true;
  const auto non_scope_verification =
      verify_exact_direct_frozen_root_quotient_streaming(
          hyperedges, roots, budget, mutated_non_scope);
  check(
      !non_scope_verification.root_only_non_scope_certified &&
          !non_scope_verification.result_certified,
      "streaming replay rejects an invented latent-facet capability");

  auto mutated_decision = result;
  mutated_decision.decision =
      ExactFrozenRootQuotientDecision::not_certified;
  const auto decision_verification =
      verify_exact_direct_frozen_root_quotient_streaming(
          hyperedges, roots, budget, mutated_decision);
  check(
      !decision_verification.decision_and_scope_certified &&
          !decision_verification.result_certified,
      "streaming replay rejects a mutated decision");
}

void check_malformed_csr_is_rejected_atomically() {
  const std::array<ExactFrozenRootId, 2U> roots{1U, 2U};
  const std::array<std::array<ExactFrozenRootHyperedge, 1U>, 5U> malformed{{
      {{{1U, 0U, 2U}}},
      {{{0U, 1U, 1U}}},
      {{{0U, 0U, 0U}}},
      {{{0U, 0U, 3U}}},
      {{{0U, 0U, 1U}}},
  }};
  for (const auto& hyperedges : malformed) {
    const auto result = build_exact_direct_frozen_root_quotient(
        hyperedges, roots, unlimited_budget());
    check(
        result.decision == ExactFrozenRootQuotientDecision::
                               no_frozen_root_quotient_input_shape_rejected &&
            !result.input_shape_certified &&
            result.budget_preflight_certified &&
            result.hyperedge_bindings.empty() && result.groups.empty() &&
            result.group_root_ids.empty(),
        "a malformed dense CSR partition is rejected without partial output");
  }
}

void check_every_budget_cap_is_preflighted_atomically() {
  const std::array<ExactFrozenRootHyperedge, 2U> hyperedges{{
      {0U, 0U, 2U},
      {1U, 2U, 1U},
  }};
  const std::array<ExactFrozenRootId, 3U> roots{1U, 2U, 9U};
  const ExactFrozenRootQuotientBudget exact_budget{
      2U, 3U, 3U, 2U, 11U};
  const auto exact = build_exact_direct_frozen_root_quotient(
      hyperedges, roots, exact_budget);
  check(
      exact.certified_frozen_root_quotient() &&
          exact.required_scratch_entry_capacity == 11U &&
          exact.logical_output_entry_limit == 7U,
      "the exact conservative preflight budget is sufficient");

  const std::array<ExactFrozenRootQuotientBudget, 5U> insufficient{{
      {1U, 3U, 3U, 2U, 11U},
      {2U, 2U, 3U, 2U, 11U},
      {2U, 3U, 2U, 2U, 11U},
      {2U, 3U, 3U, 1U, 11U},
      {2U, 3U, 3U, 2U, 10U},
  }};
  for (const ExactFrozenRootQuotientBudget& budget : insufficient) {
    const auto result = build_exact_direct_frozen_root_quotient(
        hyperedges, roots, budget);
    check(
        !result.input_shape_certified &&
            !result.budget_preflight_certified &&
            result.decision == ExactFrozenRootQuotientDecision::
                                   no_frozen_root_quotient_budget_exhausted &&
            result.hyperedge_bindings.empty() && result.groups.empty() &&
            result.group_root_ids.empty(),
        "each one-short cap rejects the quotient before any output allocation");
  }
}

}  // namespace

int main() {
  check_empty_quotient();
  check_one_root_class_and_duplicate_root_references();
  check_chain_and_canonical_flat_groups();
  check_interleaved_components_have_contiguous_root_slices();
  check_root_only_non_scope_is_explicit();
  check_streaming_verifier_and_mutation_rejection();
  check_malformed_csr_is_rejected_atomically();
  check_every_budget_cap_is_preflighted_atomically();

  if (failures != 0) {
    std::cerr << failures << " direct frozen-root quotient test(s) failed\n";
    return 1;
  }
  std::cout << "direct frozen-root quotient tests passed\n";
  return 0;
}
