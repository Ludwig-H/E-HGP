#include "morsehgp3d/hierarchy/exact_k2_k1_hierarchy.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactK2K1HierarchyBudget;
using morsehgp3d::hierarchy::ExactK2K1HierarchyDecision;
using morsehgp3d::hierarchy::ExactK2K1HierarchyResult;
using morsehgp3d::hierarchy::ExactK2K1VerticalCheckpoint;
using morsehgp3d::hierarchy::ExactK2K1VerticalCheckpointKind;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaOrderHistory;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaOrderHistoryBudget;
using morsehgp3d::hierarchy::ExactReducedGammaBatchGroupKind;
using morsehgp3d::hierarchy::K1CompactForest;
using morsehgp3d::hierarchy::K1NodeId;
using morsehgp3d::hierarchy::build_compact_k1_forest;
using morsehgp3d::hierarchy::build_exact_complete_graph_emst;
using morsehgp3d::hierarchy::build_exact_k2_k1_hierarchy;
using morsehgp3d::hierarchy::
    build_exact_persistent_reduced_gamma_order_history;
using morsehgp3d::hierarchy::verify_exact_k2_k1_hierarchy;
using morsehgp3d::spatial::CanonicalPointCloud;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(
    double x, double y = 0.0, double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator, std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] std::size_t binomial(std::size_t n, std::size_t k) {
  if (k > n) {
    return 0U;
  }
  k = std::min(k, n - k);
  std::size_t value = 1U;
  for (std::size_t index = 1U; index <= k; ++index) {
    value = value * (n - k + index) / index;
  }
  return value;
}

[[nodiscard]] ExactPersistentReducedGammaOrderHistoryBudget tight_budget(
    std::size_t point_count, std::size_t order = 2U) {
  const std::size_t facet_count = binomial(point_count, order);
  const std::size_t coface_count = binomial(point_count, order + 1U);
  const std::size_t union_count = order * coface_count;
  const std::size_t level_count = facet_count + coface_count;
  const std::size_t replay_count = level_count + 1U;

  ExactPersistentReducedGammaOrderHistoryBudget budget;
  budget.gamma_budget = {facet_count, coface_count, union_count};
  budget.maximum_activation_level_count = level_count;
  budget.maximum_total_facet_work_count = replay_count * facet_count;
  budget.maximum_total_coface_work_count = replay_count * coface_count;
  budget.maximum_total_union_work_count = replay_count * union_count;
  budget.maximum_node_count = coface_count;
  budget.maximum_child_reference_count =
      coface_count == 0U ? 0U : coface_count - 1U;
  budget.maximum_group_root_reference_count =
      coface_count == 0U ? 0U : coface_count - 1U;
  budget.maximum_group_count = level_count;
  budget.maximum_group_newly_active_facet_count = facet_count;
  budget.maximum_group_equal_level_coface_count = coface_count;
  budget.maximum_delta_facet_count = facet_count;
  budget.maximum_delta_point_reference_count = order * facet_count;
  return budget;
}

[[nodiscard]] ExactK2K1HierarchyBudget full_projection_budget() {
  ExactK2K1HierarchyBudget budget;
  budget.maximum_source_batch_count =
      ExactK2K1HierarchyBudget::maximum_supported_source_batch_count;
  budget.maximum_source_group_count =
      ExactK2K1HierarchyBudget::maximum_supported_source_group_count;
  budget.maximum_source_node_count =
      ExactK2K1HierarchyBudget::maximum_supported_source_node_count;
  budget.maximum_source_child_reference_count =
      ExactK2K1HierarchyBudget::
          maximum_supported_source_child_reference_count;
  budget.maximum_source_root_reference_count =
      ExactK2K1HierarchyBudget::
          maximum_supported_source_root_reference_count;
  budget.maximum_checkpoint_count =
      ExactK2K1HierarchyBudget::maximum_supported_checkpoint_count;
  budget.maximum_source_facet_replay_count =
      ExactK2K1HierarchyBudget::
          maximum_supported_source_facet_replay_count;
  budget.maximum_vertical_endpoint_lookup_count =
      ExactK2K1HierarchyBudget::
          maximum_supported_vertical_endpoint_lookup_count;
  budget.maximum_k1_parent_hop_count =
      ExactK2K1HierarchyBudget::maximum_supported_k1_parent_hop_count;
  budget.maximum_target_coverage_point_reference_count =
      ExactK2K1HierarchyBudget::
          maximum_supported_target_coverage_point_reference_count;
  budget.maximum_digest_logical_entry_count =
      ExactK2K1HierarchyBudget::
          maximum_supported_digest_logical_entry_count;
  budget.maximum_digest_exact_text_byte_count =
      ExactK2K1HierarchyBudget::
          maximum_supported_digest_exact_text_byte_count;
  return budget;
}

struct Fixture {
  CanonicalPointCloud cloud;
  ExactPersistentReducedGammaOrderHistoryBudget source_budget;
  ExactPersistentReducedGammaOrderHistory source;
  K1CompactForest k1;
};

[[nodiscard]] Fixture fixture(CanonicalPointCloud cloud) {
  ExactPersistentReducedGammaOrderHistoryBudget source_budget =
      tight_budget(cloud.size());
  ExactPersistentReducedGammaOrderHistory source =
      build_exact_persistent_reduced_gamma_order_history(
          cloud, 2U, source_budget);
  K1CompactForest k1 =
      build_compact_k1_forest(build_exact_complete_graph_emst(cloud));
  return Fixture{
      std::move(cloud),
      std::move(source_budget),
      std::move(source),
      std::move(k1)};
}

[[nodiscard]] const ExactK2K1VerticalCheckpoint* checkpoint_at(
    const ExactK2K1HierarchyResult& result,
    const ExactLevel& squared_level,
    std::size_t source_root_node_id) {
  const auto found = std::find_if(
      result.vertical_checkpoints.begin(),
      result.vertical_checkpoints.end(),
      [&](const ExactK2K1VerticalCheckpoint& checkpoint) {
        return checkpoint.squared_level == squared_level &&
               checkpoint.source_root_node_id == source_root_node_id;
      });
  return found == result.vertical_checkpoints.end() ? nullptr : &*found;
}

[[nodiscard]] std::vector<K1NodeId> k1_parents(
    const K1CompactForest& forest) {
  const K1NodeId no_parent = std::numeric_limits<K1NodeId>::max();
  std::vector<K1NodeId> parents(forest.node_count(), no_parent);
  for (const auto& node : forest.merge_nodes) {
    for (const K1NodeId child : forest.children(node.id)) {
      parents[child] = node.id;
    }
  }
  return parents;
}

[[nodiscard]] K1NodeId normalize(
    const K1CompactForest& forest,
    const std::vector<K1NodeId>& parents,
    K1NodeId target,
    const ExactLevel& squared_level,
    bool closed) {
  const K1NodeId no_parent = std::numeric_limits<K1NodeId>::max();
  while (parents[target] != no_parent) {
    const K1NodeId parent = parents[target];
    const ExactLevel parent_level = forest.node_level(parent);
    if (closed ? parent_level > squared_level
               : parent_level >= squared_level) {
      break;
    }
    target = parent;
  }
  return target;
}

void check_success_contract(
    const Fixture& source, const ExactK2K1HierarchyResult& result) {
  check(
      result.well_formed_success_receipt() &&
          result.decision ==
              ExactK2K1HierarchyDecision::
                  complete_bounded_exact_k2_k1_hierarchy &&
          result.strict_open_closed_vertical_naturality_certified &&
          result.k1_only_interlevel_target_evolution_certified &&
          result
              .vertical_map_complete_for_bounded_hgp_reduced_k2_to_k1_source &&
          !result.all_order_vertical_maps_complete &&
          !result.product_architecture_claimed &&
          !result.scalable_50k_claimed && !result.global_m1_claimed &&
          !result.public_status_claimed &&
          result.provenance.conditional_direct_morse_authority_not_used,
      "the bounded seam certifies exactly Gamma2-to-K1 without product or global claims");
  const auto verification = verify_exact_k2_k1_hierarchy(
      source.cloud,
      source.source_budget,
      source.source,
      source.k1,
      result.requested_budget,
      result);
  check(
      verification.fresh_replay_certified &&
          verification.exact_k2_k1_hierarchy_decision_certified,
      "the bounded seam passes its independent fresh replay");
}

void test_closed_equality_targets_the_post_batch_k1_root() {
  Fixture source = fixture(canonical_cloud(std::array{
      point(-2.0), point(-1.0), point(0.0), point(2.0)}));
  const ExactK2K1HierarchyResult result = build_exact_k2_k1_hierarchy(
      source.cloud,
      source.source_budget,
      source.source,
      source.k1,
      full_projection_budget());
  check_success_contract(source, result);

  const ExactK2K1VerticalCheckpoint* birth =
      checkpoint_at(result, level(1), 0U);
  const ExactK2K1VerticalCheckpoint* first_continuation =
      checkpoint_at(result, level(9, 4), 0U);
  check(
      source.k1.merge_nodes.size() == 2U &&
          source.k1.node_level(4U) == level(1, 4) &&
          source.k1.node_level(5U) == level(1) && birth != nullptr &&
          birth->kind == ExactK2K1VerticalCheckpointKind::birth &&
          birth->strict_prior_target_node_ids.empty() &&
          birth->closed_target_node_id == 5U &&
          birth->source_covered_point_count == 3U &&
          birth->closed_target_covered_point_count == 4U &&
          first_continuation != nullptr &&
          first_continuation->closed_target_node_id == 5U,
      "a Gamma2 birth at equality maps to the closed K1 node, whose coverage may strictly contain the source coverage");
  check(
      result.vertical_checkpoints.size() == 3U &&
          result.counters.birth_checkpoint_count == 1U &&
          result.counters.continuation_checkpoint_count == 2U,
      "the equality fixture retains its birth and both continuations");

  ExactK2K1HierarchyResult stale = result;
  stale.vertical_checkpoints.front().closed_target_node_id = 4U;
  const auto stale_verification = verify_exact_k2_k1_hierarchy(
      source.cloud,
      source.source_budget,
      source.source,
      source.k1,
      result.requested_budget,
      stale);
  check(
      !stale.well_formed_success_receipt() &&
          !stale_verification.checkpoints_certified &&
          !stale_verification.success_digest_recomputed &&
          !stale_verification.exact_k2_k1_hierarchy_decision_certified,
      "the receipt and fresh verifier reject a cached strict target substituted for a closed target");
}

void test_deferred_only_level_still_updates_the_vertical_map() {
  Fixture source = fixture(canonical_cloud(std::array{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0)}));
  const ExactK2K1HierarchyResult result = build_exact_k2_k1_hierarchy(
      source.cloud,
      source.source_budget,
      source.source,
      source.k1,
      full_projection_budget());
  check_success_contract(source, result);

  const bool deferred_at_bridge = std::any_of(
      source.source.group_records.begin(),
      source.source.group_records.end(),
      [](const auto& record) {
        return record.squared_level == level(13, 4) &&
               record.kind == ExactReducedGammaBatchGroupKind::
                                  deferred_isolated_facet;
      });
  const bool checkpoint_at_bridge = std::any_of(
      result.vertical_checkpoints.begin(),
      result.vertical_checkpoints.end(),
      [](const auto& checkpoint) {
        return checkpoint.squared_level == level(13, 4);
      });
  const std::vector<K1NodeId> parents = k1_parents(source.k1);
  check(
      source.k1.node_level(6U) == level(5, 4) &&
          source.k1.node_level(8U) == level(13, 4) &&
          normalize(source.k1, parents, 6U, level(13, 4), false) == 6U &&
          normalize(source.k1, parents, 6U, level(13, 4), true) == 8U &&
          deferred_at_bridge && !checkpoint_at_bridge &&
          result.counters.k1_target_normalization_update_count > 0U,
      "the union sweep changes root zero from K1 node 6 to node 8 across a deferred-only equality without inventing a checkpoint");

  const ExactK2K1VerticalCheckpoint* first_birth =
      checkpoint_at(result, level(25, 16), 0U);
  const ExactK2K1VerticalCheckpoint* second_birth =
      checkpoint_at(result, level(1105, 242), 1U);
  const ExactK2K1VerticalCheckpoint* merge =
      checkpoint_at(result, level(13, 2), 2U);
  check(
      first_birth != nullptr && first_birth->closed_target_node_id == 6U &&
          second_birth != nullptr &&
          second_birth->closed_target_node_id == 8U && merge != nullptr &&
          merge->strict_prior_target_node_ids ==
              std::vector<K1NodeId>({8U, 8U}) &&
          merge->closed_target_node_id == 8U,
      "later births and multifusions consume normalized K1 targets, not stale checkpoint identifiers");
}

void test_silent_incidence_checkpoint_updates_target_without_a_new_point() {
  Fixture source = fixture(canonical_cloud(std::array{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0)}));
  const ExactK2K1HierarchyResult result = build_exact_k2_k1_hierarchy(
      source.cloud,
      source.source_budget,
      source.source,
      source.k1,
      full_projection_budget());
  check_success_contract(source, result);

  const ExactK2K1VerticalCheckpoint* birth =
      checkpoint_at(result, level(162, 25), 0U);
  const ExactK2K1VerticalCheckpoint* growth =
      checkpoint_at(result, level(189, 17), 0U);
  const ExactK2K1VerticalCheckpoint* silent =
      checkpoint_at(result, level(33, 2), 0U);
  const ExactK2K1VerticalCheckpoint* later =
      checkpoint_at(result, level(83886, 3563), 0U);
  check(
      birth != nullptr && birth->closed_target_node_id == 5U &&
          growth != nullptr && growth->closed_target_node_id == 6U &&
          silent != nullptr &&
          silent->kind == ExactK2K1VerticalCheckpointKind::continuation &&
          silent->closed_target_node_id == 7U &&
          silent->added_facet_count == 1U &&
          silent->added_point_count == 0U && later != nullptr &&
          later->closed_target_node_id == 7U,
      "a silent Gamma2 incidence is retained and advances the K1 target without fabricating a point or source node");
}

void test_budget_and_input_failures_are_atomic() {
  Fixture source = fixture(canonical_cloud(std::array{
      point(-2.0), point(-1.0), point(0.0), point(2.0)}));
  const ExactK2K1HierarchyResult complete = build_exact_k2_k1_hierarchy(
      source.cloud,
      source.source_budget,
      source.source,
      source.k1,
      full_projection_budget());
  ExactK2K1HierarchyBudget insufficient = full_projection_budget();
  insufficient.maximum_source_facet_replay_count =
      complete.preflight_source_facet_replay_upper_bound - 1U;
  const ExactK2K1HierarchyResult failed = build_exact_k2_k1_hierarchy(
      source.cloud,
      source.source_budget,
      source.source,
      source.k1,
      insufficient);
  check(
      failed.well_formed_fail_closed_receipt() &&
          failed.decision ==
              ExactK2K1HierarchyDecision::
                  no_projection_preflight_budget_insufficient &&
          failed.failure_payload_empty && failed.atomic_failure &&
          failed.vertical_checkpoints.empty() &&
          failed.preflight_source_facet_replay_upper_bound ==
              complete.preflight_source_facet_replay_upper_bound,
      "one missing facet-replay slot yields a certified atomic failure with its conservative preflight bound and no scientific payload");

  ExactPersistentReducedGammaOrderHistory stale_source = source.source;
  stale_source.persistent_reduced_gamma_history_certified = false;
  const ExactK2K1HierarchyResult rejected_source =
      build_exact_k2_k1_hierarchy(
          source.cloud,
          source.source_budget,
          stale_source,
          source.k1,
          full_projection_budget());
  check(
      rejected_source.well_formed_fail_closed_receipt() &&
          rejected_source.decision ==
              ExactK2K1HierarchyDecision::no_source_history_rejected,
      "a stale horizontal authority is freshly rejected without partial output");

  K1CompactForest stale_k1 = source.k1;
  stale_k1.total_squared_weight = {};
  const ExactK2K1HierarchyResult rejected_k1 = build_exact_k2_k1_hierarchy(
      source.cloud,
      source.source_budget,
      source.source,
      stale_k1,
      full_projection_budget());
  check(
      rejected_k1.well_formed_fail_closed_receipt() &&
          rejected_k1.decision ==
              ExactK2K1HierarchyDecision::no_k1_forest_rejected,
      "a stale compact K1 forest is rejected by fresh complete-graph EMST replay");
}

void test_terminal_order_is_only_an_empty_reduced_family() {
  Fixture source = fixture(canonical_cloud(std::array{
      point(0.0), point(1.0)}));
  const ExactK2K1HierarchyResult result = build_exact_k2_k1_hierarchy(
      source.cloud,
      source.source_budget,
      source.source,
      source.k1,
      full_projection_budget());
  check(
      result.well_formed_success_receipt() &&
          result.decision ==
              ExactK2K1HierarchyDecision::complete_empty_terminal_order &&
          result.vertical_checkpoints.empty() &&
          !result.bounded_exhaustive_gamma_source_materialized &&
          !result.all_order_vertical_maps_complete &&
          !result.global_m1_claimed,
      "n=k=2 certifies only the empty hgp_reduced Gamma2 family, never full-pi0 or global M.1");
}

}  // namespace

int main() {
  test_closed_equality_targets_the_post_batch_k1_root();
  test_deferred_only_level_still_updates_the_vertical_map();
  test_silent_incidence_checkpoint_updates_target_without_a_new_point();
  test_budget_and_input_failures_are_atomic();
  test_terminal_order_is_only_an_empty_reduced_family();
  if (failures != 0) {
    std::cerr << failures << " exact Gamma2-to-K1 hierarchy test(s) failed\n";
    return 1;
  }
  std::cout << "all exact Gamma2-to-K1 hierarchy tests passed\n";
  return 0;
}
