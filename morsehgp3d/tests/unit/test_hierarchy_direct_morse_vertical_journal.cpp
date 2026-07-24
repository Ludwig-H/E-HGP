#include "morsehgp3d/hierarchy/direct_morse_vertical_journal.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::PointId;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] ExactLevel level(std::int64_t numerator) {
  return ExactLevel{BigInt{numerator}, BigInt{1}};
}

[[nodiscard]] ExactDirectSparseFacetKey key(
    std::initializer_list<PointId> points) {
  ExactDirectSparseFacetKey result;
  result.point_count = points.size();
  std::size_t index = 0U;
  for (const PointId point : points) {
    result.point_ids[index] = point;
    ++index;
  }
  return result;
}

[[nodiscard]] ExactDirectMorseForestNode node(
    ExactDirectMorseForestNodeId node_id,
    std::size_t order,
    std::int64_t squared_level,
    ExactDirectMorseForestNodeKind kind,
    std::size_t child_offset,
    std::size_t child_count,
    std::optional<std::size_t> birth_record_index,
    std::optional<std::size_t> atomic_group_index) {
  ExactDirectMorseForestNode result;
  result.node_id = node_id;
  result.order = order;
  result.squared_level = level(squared_level);
  result.kind = kind;
  result.child_offset = child_offset;
  result.child_count = child_count;
  result.birth_record_index = birth_record_index;
  result.atomic_group_index = atomic_group_index;
  return result;
}

[[nodiscard]] ExactDirectMorseForestBirthRecord birth(
    std::size_t index,
    std::size_t order,
    ExactDirectSparseFacetKey facet_key,
    std::optional<ExactDirectMorseForestNodeId> order_one_node) {
  ExactDirectMorseForestBirthRecord result;
  result.birth_record_index = index;
  result.order = order;
  result.facet_key = std::move(facet_key);
  result.order_one_birth_node_id = order_one_node;
  return result;
}

[[nodiscard]] ExactDirectMorseForestArmRootBinding binding(
    std::size_t index,
    ExactDirectSparseFacetKey strict_arm_key,
    std::optional<ExactDirectMorseForestNodeId> prior_root) {
  ExactDirectMorseForestArmRootBinding result;
  result.binding_index = index;
  result.strict_arm_key = std::move(strict_arm_key);
  result.prior_reduced_root_node_id = prior_root;
  return result;
}

[[nodiscard]] ExactDirectMorseForestSaddleRecord saddle(
    std::size_t index,
    std::size_t binding_offset,
    std::size_t binding_count,
    std::size_t atomic_group_index) {
  ExactDirectMorseForestSaddleRecord result;
  result.saddle_record_index = index;
  result.arm_binding_offset = binding_offset;
  result.arm_binding_count = binding_count;
  result.atomic_group_index = atomic_group_index;
  return result;
}

[[nodiscard]] ExactDirectMorseForestAtomicGroup group(
    std::size_t index,
    std::size_t batch_index,
    std::size_t saddle_offset,
    std::size_t prior_root_count,
    std::size_t child_offset,
    std::size_t child_count,
    std::optional<ExactDirectMorseForestNodeId> created_node,
    ExactDirectMorseForestNodeId resulting_root,
    ExactDirectMorseForestAtomicGroupKind kind) {
  ExactDirectMorseForestAtomicGroup result;
  result.atomic_group_index = index;
  result.batch_index = batch_index;
  result.saddle_record_offset = saddle_offset;
  result.saddle_record_count = 1U;
  result.prior_reduced_root_count = prior_root_count;
  result.child_offset = child_offset;
  result.child_count = child_count;
  result.created_node_id = created_node;
  result.resulting_root_node_id = resulting_root;
  result.kind = kind;
  return result;
}

[[nodiscard]] ExactDirectMorseForestBatch batch(
    std::size_t index,
    std::size_t order,
    std::int64_t squared_level,
    std::size_t group_offset,
    std::size_t group_count) {
  ExactDirectMorseForestBatch result;
  result.batch_index = index;
  result.order = order;
  result.squared_level = level(squared_level);
  result.atomic_group_offset = group_offset;
  result.atomic_group_count = group_count;
  return result;
}

[[nodiscard]] ExactDirectMorseForestFinalRoot final_root(
    std::size_t index,
    std::size_t order,
    ExactDirectMorseForestNodeId root_node_id) {
  ExactDirectMorseForestFinalRoot result;
  result.final_root_index = index;
  result.order = order;
  result.root_node_id = root_node_id;
  return result;
}

[[nodiscard]] ExactDirectMorseForestJournalResult forest_fixture() {
  ExactDirectMorseForestJournalResult forest;
  forest.point_count = 4U;
  forest.effective_maximum_order = 4U;
  forest.decision = ExactDirectMorseForestDecision::
      complete_conditional_exact_direct_morse_forest;

  forest.child_node_ids = {0U, 1U, 3U, 4U};
  forest.nodes = {
      node(
          0U,
          1U,
          0,
          ExactDirectMorseForestNodeKind::order_one_birth,
          0U,
          0U,
          0U,
          std::nullopt),
      node(
          1U,
          1U,
          0,
          ExactDirectMorseForestNodeKind::order_one_birth,
          0U,
          0U,
          1U,
          std::nullopt),
      node(
          2U,
          1U,
          1,
          ExactDirectMorseForestNodeKind::multifusion,
          0U,
          2U,
          std::nullopt,
          std::nullopt),
      node(
          3U,
          2U,
          1,
          ExactDirectMorseForestNodeKind::reduced_birth,
          0U,
          0U,
          std::nullopt,
          0U),
      node(
          4U,
          2U,
          2,
          ExactDirectMorseForestNodeKind::reduced_birth,
          0U,
          0U,
          std::nullopt,
          2U),
      node(
          5U,
          2U,
          3,
          ExactDirectMorseForestNodeKind::multifusion,
          2U,
          2U,
          std::nullopt,
          3U),
      node(
          6U,
          3U,
          3,
          ExactDirectMorseForestNodeKind::reduced_birth,
          0U,
          0U,
          std::nullopt,
          4U),
  };
  forest.birth_records = {
      birth(0U, 1U, key({0U}), 0U),
      birth(1U, 1U, key({1U}), 1U),
      birth(2U, 2U, key({0U, 1U}), std::nullopt),
      birth(3U, 2U, key({2U, 3U}), std::nullopt),
      birth(4U, 3U, key({0U, 1U, 2U}), std::nullopt),
      birth(5U, 4U, key({0U, 1U, 2U, 3U}), std::nullopt),
  };
  forest.arm_root_bindings = {
      binding(0U, key({0U, 1U}), std::nullopt),
      binding(1U, key({0U, 2U}), 3U),
      binding(2U, key({2U, 3U}), std::nullopt),
      binding(3U, key({1U, 3U}), 4U),
      binding(4U, key({1U, 3U}), 4U),
      binding(5U, key({0U, 3U}), 3U),
      binding(6U, key({0U, 1U, 2U}), std::nullopt),
  };
  forest.saddle_records = {
      saddle(0U, 0U, 1U, 0U),
      saddle(1U, 1U, 1U, 1U),
      saddle(2U, 2U, 1U, 2U),
      saddle(3U, 3U, 3U, 3U),
      saddle(4U, 6U, 1U, 4U),
  };
  forest.atomic_groups = {
      group(
          0U,
          0U,
          0U,
          0U,
          0U,
          0U,
          3U,
          3U,
          ExactDirectMorseForestAtomicGroupKind::reduced_birth),
      group(
          1U,
          1U,
          1U,
          1U,
          0U,
          0U,
          std::nullopt,
          3U,
          ExactDirectMorseForestAtomicGroupKind::continuation),
      group(
          2U,
          1U,
          2U,
          0U,
          0U,
          0U,
          4U,
          4U,
          ExactDirectMorseForestAtomicGroupKind::reduced_birth),
      group(
          3U,
          2U,
          3U,
          2U,
          2U,
          2U,
          5U,
          5U,
          ExactDirectMorseForestAtomicGroupKind::multifusion),
      group(
          4U,
          3U,
          4U,
          0U,
          0U,
          0U,
          6U,
          6U,
          ExactDirectMorseForestAtomicGroupKind::reduced_birth),
  };
  forest.batches = {
      batch(0U, 2U, 1, 0U, 1U),
      batch(1U, 2U, 2, 1U, 2U),
      batch(2U, 2U, 3, 3U, 1U),
      batch(3U, 3U, 3, 4U, 1U),
  };
  forest.final_roots = {
      final_root(0U, 1U, 2U),
      final_root(1U, 2U, 5U),
      final_root(2U, 3U, 6U),
  };
  return forest;
}

[[nodiscard]] ExactDirectMorseVerticalBudget generous_budget() {
  ExactDirectMorseVerticalBudget budget;
  budget.maximum_forest_node_scan_count = 64U;
  budget.maximum_child_reference_scan_count = 64U;
  budget.maximum_birth_record_scan_count = 64U;
  budget.maximum_batch_scan_count = 64U;
  budget.maximum_atomic_group_scan_count = 64U;
  budget.maximum_saddle_scan_count = 64U;
  budget.maximum_arm_binding_scan_count = 64U;
  budget.maximum_proposal_count = 64U;
  budget.maximum_label_resolution_count = 64U;
  budget.maximum_group_check_count = 64U;
  budget.maximum_checkpoint_count = 64U;
  budget.maximum_adjacent_family_count = 8U;
  budget.maximum_group_sort_scratch_count = 64U;
  budget.maximum_group_sort_comparison_count = 512U;
  budget.maximum_target_parent_hop_count = 512U;
  budget.maximum_exact_level_comparison_count = 512U;
  budget.maximum_single_exact_level_integer_bit_count = 64U;
  budget.maximum_logical_output_entry_count = 256U;
  return budget;
}

[[nodiscard]] ExactDirectMorseVerticalConfig config() {
  return {UINT64_C(0x11A)};
}

[[nodiscard]] ExactDirectMorseVerticalTargetProposal resolved(
    std::size_t representative,
    ExactDirectMorseForestNodeId seed,
    std::uint64_t token) {
  ExactDirectMorseVerticalTargetProposal proposal;
  proposal.representative_arm_root_binding_index = representative;
  proposal.target_seed_node_id = seed;
  proposal.replay_token = token;
  proposal.disposition =
      ExactDirectMorseVerticalProposalDisposition::resolved_target_seed;
  return proposal;
}

[[nodiscard]] std::vector<ExactDirectMorseVerticalTargetProposal>
total_proposals() {
  return {
      resolved(6U, 3U, 106U),
      resolved(5U, 0U, 105U),
      resolved(3U, 1U, 103U),
      resolved(2U, 1U, 102U),
      resolved(1U, 0U, 101U),
      resolved(0U, 0U, 100U),
  };
}

[[nodiscard]] const ExactDirectMorseVerticalGroupCheck* check_for_group(
    const ExactDirectMorseVerticalJournalResult& result,
    std::size_t group_index) {
  const auto found = std::find_if(
      result.group_checks.begin(),
      result.group_checks.end(),
      [group_index](const auto& check) {
        return check.atomic_group_index == group_index;
      });
  return found == result.group_checks.end() ? nullptr : &*found;
}

void test_total_relative_and_trace() {
  const auto forest = forest_fixture();
  const auto proposals = total_proposals();
  const auto budget = generous_budget();
  const auto result = build_exact_direct_morse_vertical_journal(
      forest, proposals, budget, config());

  check(
      result.decision ==
          ExactDirectMorseVerticalDecision::
              complete_conditional_total_relative_vertical_journal,
      "complete proposals close the relative vertical journal");
  check(
      result.certified_conditional_vertical_candidate(),
      "complete relative journal satisfies its conditional certificate");
  check(
      result.adjacent_families.size() == 3U &&
          result.adjacent_families[2].group_check_count == 0U &&
          result.adjacent_families[2]
                  .omitted_isolated_source_birth_count ==
              1U,
      "every adjacent family is explicit, including the empty order-four family");
  check(
      result.adjacent_families[0]
              .omitted_isolated_source_birth_count ==
          2U &&
          result.adjacent_families[1]
                  .omitted_isolated_source_birth_count ==
              1U,
      "one bounded birth scan records the reduced perimeter");
  check(
      result.label_resolutions.size() == 6U &&
          result.label_resolutions[3]
                  .representative_arm_root_binding_index ==
              5U &&
          result.label_resolutions[4]
                  .representative_arm_root_binding_index ==
              3U &&
          result.label_resolutions[3].source_proposal_index.has_value() &&
          result.label_resolutions[4].source_proposal_index.has_value(),
      "key-canonical labels resolve when their binding order is reversed");
  check(
      result.counters.checked_elementary_group_square_count == 3U &&
          result.counters.unresolved_elementary_group_square_count == 0U,
      "all three available elementary group squares are checked");
  check(
      !result.external_target_authority_replayed &&
          !result.all_naturality_squares_replayed &&
          !result.vertical_maps_complete &&
          !result.public_status_claimed,
      "the conditional journal does not self-promote external or public facts");

  const auto verification =
      verify_exact_direct_morse_vertical_journal(
          forest, proposals, budget, config(), result);
  check(
      verification.result_certified &&
          !verification.external_target_authority_replayed,
      "fresh reconstruction verifies only the conditional journal");

  const auto trace = trace_exact_direct_morse_vertical_component(
      forest,
      result,
      6U,
      level(3),
      1U,
      ExactDirectMorseVerticalTraceBudget{2U, 16U, 32U});
  check(
      trace.disposition ==
              ExactDirectMorseVerticalTraceDisposition::
                  complete_relative_trace &&
          trace.steps.size() == 2U &&
          trace.steps[0].source_root_node_id == 6U &&
          trace.steps[0].target_root_node_id == 5U &&
          trace.steps[1].source_root_node_id == 5U &&
          trace.steps[1].target_root_node_id == 2U &&
          !trace.public_vertical_map_claimed,
      "the compact trace composes order three to order one");
}

void test_partial_labels_late_checkpoint_and_square_partition() {
  const auto forest = forest_fixture();
  auto proposals = total_proposals();
  proposals.erase(std::remove_if(
      proposals.begin(),
      proposals.end(),
      [](const auto& proposal) {
        return proposal.representative_arm_root_binding_index == 0U;
      }),
      proposals.end());
  const auto result = build_exact_direct_morse_vertical_journal(
      forest, proposals, generous_budget(), config());

  check(
      result.decision ==
              ExactDirectMorseVerticalDecision::
                  complete_conditional_partial_vertical_journal &&
          result.certified_conditional_vertical_candidate(),
      "a missing proposal yields a certified partial journal");
  check(
      result.counters.missing_label_count == 1U &&
          result.counters.unresolved_label_count == 0U &&
          result.counters.resolved_label_count == 5U &&
          result.counters.expected_label_count == 6U,
      "missing and resolved label counters form an exact partition");
  check(
      result.counters.late_checkpoint_count == 1U,
      "q=1 may install one explicitly incomplete late checkpoint");
  const auto* multifusion = check_for_group(result, 3U);
  check(
      multifusion != nullptr &&
          multifusion->expected_elementary_group_square_count == 2U &&
          multifusion->checked_elementary_group_square_count == 1U &&
          multifusion->unresolved_elementary_group_square_count == 1U,
      "a complete agreeing prior square remains checked beside a missing one");

  const auto trace = trace_exact_direct_morse_vertical_component(
      forest,
      result,
      6U,
      level(3),
      1U,
      ExactDirectMorseVerticalTraceBudget{2U, 16U, 32U});
  check(
      trace.disposition ==
          ExactDirectMorseVerticalTraceDisposition::partial_relative_trace,
      "a compact trace preserves incomplete checkpoint provenance");

  auto unresolved_proposals = total_proposals();
  for (auto& proposal : unresolved_proposals) {
    if (proposal.representative_arm_root_binding_index == 0U) {
      proposal.target_seed_node_id.reset();
      proposal.disposition =
          ExactDirectMorseVerticalProposalDisposition::unresolved;
    }
  }
  const auto unresolved = build_exact_direct_morse_vertical_journal(
      forest, unresolved_proposals, generous_budget(), config());
  check(
      unresolved.counters.missing_label_count == 0U &&
          unresolved.counters.unresolved_label_count == 1U &&
          unresolved.counters.resolved_label_count == 5U,
      "an explicit unresolved proposal is not collapsed into missing");
}

void test_atomic_rejections_and_budgets() {
  const auto forest = forest_fixture();
  const auto proposals = total_proposals();

  auto conflict_forest = forest;
  conflict_forest.nodes[2].squared_level = level(4);
  const auto conflict = build_exact_direct_morse_vertical_journal(
      conflict_forest, proposals, generous_budget(), config());
  check(
      conflict.decision ==
              ExactDirectMorseVerticalDecision::
                  no_vertical_relative_target_conflict &&
          conflict.certified_atomic_failure(),
      "different closed targets reject the whole relative journal");

  auto future_proposals = proposals;
  for (auto& proposal : future_proposals) {
    if (proposal.representative_arm_root_binding_index == 0U) {
      proposal.target_seed_node_id = 2U;
    }
  }
  const auto future = build_exact_direct_morse_vertical_journal(
      conflict_forest, future_proposals, generous_budget(), config());
  check(
      future.decision ==
              ExactDirectMorseVerticalDecision::
                  no_vertical_target_rejected &&
          future.certified_atomic_failure(),
      "a future target seed is rejected atomically");

  auto wrong_order_proposals = proposals;
  for (auto& proposal : wrong_order_proposals) {
    if (proposal.representative_arm_root_binding_index == 0U) {
      proposal.target_seed_node_id = 3U;
    }
  }
  const auto wrong_order = build_exact_direct_morse_vertical_journal(
      forest, wrong_order_proposals, generous_budget(), config());
  check(
      wrong_order.decision ==
          ExactDirectMorseVerticalDecision::no_vertical_target_rejected,
      "a target seed of the wrong order is rejected");

  auto duplicate_proposals = proposals;
  duplicate_proposals.push_back(resolved(0U, 0U, 999U));
  const auto duplicate = build_exact_direct_morse_vertical_journal(
      forest, duplicate_proposals, generous_budget(), config());
  check(
      duplicate.decision ==
          ExactDirectMorseVerticalDecision::
              no_vertical_proposal_partition_rejected,
      "duplicate representative proposals are rejected");

  auto nonrepresentative_proposals = proposals;
  nonrepresentative_proposals.push_back(resolved(4U, 1U, 998U));
  const auto nonrepresentative =
      build_exact_direct_morse_vertical_journal(
          forest,
          nonrepresentative_proposals,
          generous_budget(),
          config());
  check(
      nonrepresentative.decision ==
          ExactDirectMorseVerticalDecision::
              no_vertical_proposal_partition_rejected,
      "a duplicate-key nonrepresentative binding is rejected");

  auto bad_birth = forest;
  bad_birth.birth_records[2].order_one_birth_node_id = 0U;
  const auto bad_birth_result =
      build_exact_direct_morse_vertical_journal(
          bad_birth, proposals, generous_budget(), config());
  check(
      bad_birth_result.decision ==
          ExactDirectMorseVerticalDecision::
              no_vertical_forest_shape_rejected,
      "higher-order direct births cannot masquerade as source nodes");

  auto double_parent = forest;
  double_parent.child_node_ids[2] = 0U;
  const auto double_parent_result =
      build_exact_direct_morse_vertical_journal(
          double_parent, proposals, generous_budget(), config());
  check(
      double_parent_result.decision ==
          ExactDirectMorseVerticalDecision::
              no_vertical_forest_shape_rejected,
      "a node cannot have two horizontal parents");

  auto divergent_final_root = forest;
  divergent_final_root.final_roots[1].root_node_id = 4U;
  const auto divergent_final_root_result =
      build_exact_direct_morse_vertical_journal(
          divergent_final_root,
          proposals,
          generous_budget(),
          config());
  check(
      divergent_final_root_result.decision ==
          ExactDirectMorseVerticalDecision::
              no_vertical_forest_shape_rejected,
      "declared final roots must equal the structural roots");

  auto noncanonical_group_partition = forest;
  noncanonical_group_partition.batches[1].atomic_group_offset = 2U;
  const auto noncanonical_group_partition_result =
      build_exact_direct_morse_vertical_journal(
          noncanonical_group_partition,
          proposals,
          generous_budget(),
          config());
  check(
      noncanonical_group_partition_result.decision ==
          ExactDirectMorseVerticalDecision::
              no_vertical_forest_shape_rejected,
      "batch-to-group slices must form one exhaustive partition");

  auto budget = generous_budget();
  budget.maximum_birth_record_scan_count =
      forest.birth_records.size() - 1U;
  const auto birth_budget = build_exact_direct_morse_vertical_journal(
      forest, proposals, budget, config());
  check(
      birth_budget.decision ==
              ExactDirectMorseVerticalDecision::
                  no_vertical_budget_exhausted &&
          birth_budget.certified_atomic_failure(),
      "the birth-record scan is preflight bounded");

  budget = generous_budget();
  budget.maximum_label_resolution_count = 5U;
  const auto label_budget = build_exact_direct_morse_vertical_journal(
      forest, proposals, budget, config());
  check(
      label_budget.decision ==
          ExactDirectMorseVerticalDecision::no_vertical_budget_exhausted,
      "label materialization fails closed before publication");

  const auto total = build_exact_direct_morse_vertical_journal(
      forest, proposals, generous_budget(), config());
  check(total.counters.target_parent_hop_count > 0U, "fixture uses target lifts");
  budget = generous_budget();
  budget.maximum_target_parent_hop_count =
      total.counters.target_parent_hop_count - 1U;
  const auto hop_budget = build_exact_direct_morse_vertical_journal(
      forest, proposals, budget, config());
  check(
      hop_budget.decision ==
          ExactDirectMorseVerticalDecision::no_vertical_budget_exhausted,
      "one-less target parent-hop budget fails closed");

  budget = generous_budget();
  budget.maximum_single_exact_level_integer_bit_count = 0U;
  const auto bit_budget = build_exact_direct_morse_vertical_journal(
      forest, proposals, budget, config());
  check(
      bit_budget.decision ==
          ExactDirectMorseVerticalDecision::no_vertical_budget_exhausted,
      "exact-level integer size has a hard cap");

  auto mutated = total;
  ++mutated.counters.resolved_label_count;
  const auto verification =
      verify_exact_direct_morse_vertical_journal(
          forest, proposals, generous_budget(), config(), mutated);
  check(
      !mutated.certified_conditional_vertical_candidate() &&
          !verification.result_certified,
      "counter mutations break both local and recursive verification");

  auto verify_mutation = [&](auto mutate, const std::string& message) {
    auto candidate = total;
    mutate(candidate);
    const auto checked =
        verify_exact_direct_morse_vertical_journal(
            forest,
            proposals,
            generous_budget(),
            config(),
            candidate);
    check(!checked.result_certified, message);
  };
  verify_mutation(
      [](auto& candidate) {
        ++candidate.adjacent_families[0]
              .omitted_isolated_source_birth_count;
      },
      "an adjacent-family mutation is rejected by fresh replay");
  verify_mutation(
      [](auto& candidate) {
        candidate.label_resolutions[0].disposition =
            ExactDirectMorseVerticalLabelDisposition::unresolved;
      },
      "a label-resolution mutation is rejected by fresh replay");
  verify_mutation(
      [](auto& candidate) {
        ++candidate.group_checks[0]
              .unresolved_elementary_group_square_count;
      },
      "a group-check mutation is rejected by fresh replay");
  verify_mutation(
      [](auto& candidate) {
        candidate.checkpoints[0]
            .complete_relative_to_supplied_proposals = false;
      },
      "a checkpoint mutation is rejected by fresh replay");

  const auto trace_budget = trace_exact_direct_morse_vertical_component(
      forest,
      total,
      6U,
      level(3),
      1U,
      ExactDirectMorseVerticalTraceBudget{2U, 16U, 0U});
  check(
      trace_budget.disposition ==
          ExactDirectMorseVerticalTraceDisposition::budget_exhausted,
      "trace checkpoint scans are independently bounded");
}

}  // namespace

int main() {
  test_total_relative_and_trace();
  test_partial_labels_late_checkpoint_and_square_partition();
  test_atomic_rejections_and_budgets();

  if (failures != 0) {
    std::cerr << failures
              << " direct Morse vertical journal test(s) failed\n";
    return 1;
  }
  std::cout << "direct Morse vertical journal tests passed\n";
  return 0;
}
