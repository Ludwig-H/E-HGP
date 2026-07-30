#include "morsehgp3d/hierarchy/critical_catalog.hpp"
#include "morsehgp3d/hierarchy/emst.hpp"
#include "morsehgp3d/hierarchy/morse_gamma_partition_sweep.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::ExactCriticalCatalogBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogDecision;
using morsehgp3d::hierarchy::ExactFacetDescentChainBudget;
using morsehgp3d::hierarchy::ExactMorseGammaPartitionSweepBudget;
using morsehgp3d::hierarchy::ExactMorseGammaPartitionSweepDecision;
using morsehgp3d::hierarchy::ExactMorseGammaPartitionSweepResult;
using morsehgp3d::hierarchy::ExactMorseGammaPartitionSweepScope;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaOrderHistoryBudget;
using morsehgp3d::hierarchy::ExactStrictGammaBudget;
using morsehgp3d::hierarchy::K1CutClosure;
using morsehgp3d::hierarchy::K1CutEdgeSource;
using morsehgp3d::hierarchy::build_exact_complete_graph_emst;
using morsehgp3d::hierarchy::build_exact_critical_catalog;
using morsehgp3d::hierarchy::build_exact_morse_gamma_partition_sweep;
using morsehgp3d::spatial::CanonicalPointCloud;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(
    double x,
    double y = 0.0,
    double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud line_cloud(std::size_t point_count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    points.push_back(point(static_cast<double>(index)));
  }
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] ExactCriticalCatalogBudget full_catalog_budget() {
  return {
      ExactCriticalCatalogBudget::maximum_supported_candidate_count,
      ExactCriticalCatalogBudget::
          maximum_supported_point_classification_count};
}

[[nodiscard]] ExactPersistentReducedGammaOrderHistoryBudget
full_history_budget() {
  ExactPersistentReducedGammaOrderHistoryBudget budget;
  budget.gamma_budget = {
      ExactStrictGammaBudget::maximum_supported_facet_count,
      ExactStrictGammaBudget::maximum_supported_coface_count,
      ExactStrictGammaBudget::maximum_supported_union_attempt_count};
  budget.maximum_activation_level_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_activation_level_count;
  budget.maximum_total_facet_work_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_total_facet_work_count;
  budget.maximum_total_coface_work_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_total_coface_work_count;
  budget.maximum_total_union_work_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_total_union_work_count;
  budget.maximum_node_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_node_count;
  budget.maximum_child_reference_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_child_reference_count;
  budget.maximum_group_root_reference_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_group_root_reference_count;
  budget.maximum_group_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_group_count;
  budget.maximum_group_newly_active_facet_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_group_newly_active_facet_count;
  budget.maximum_group_equal_level_coface_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_group_equal_level_coface_count;
  budget.maximum_delta_facet_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_delta_facet_count;
  budget.maximum_delta_point_reference_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_delta_point_reference_count;
  return budget;
}

[[nodiscard]] ExactMorseGammaPartitionSweepBudget full_sweep_budget() {
  ExactMorseGammaPartitionSweepBudget budget;
  budget.critical_catalog_budget = full_catalog_budget();
  budget.per_arm_chain_budget = {
      ExactFacetDescentChainBudget::
          maximum_supported_committed_strict_segment_count};
  budget.gamma_oracle_history_budget = full_history_budget();
  budget.maximum_birth_record_count =
      ExactMorseGammaPartitionSweepBudget::
          maximum_supported_birth_record_count;
  budget.maximum_saddle_record_count =
      ExactMorseGammaPartitionSweepBudget::
          maximum_supported_saddle_record_count;
  budget.maximum_arm_reference_count =
      ExactMorseGammaPartitionSweepBudget::
          maximum_supported_arm_reference_count;
  budget.maximum_node_count =
      ExactMorseGammaPartitionSweepBudget::maximum_supported_node_count;
  budget.maximum_child_reference_count =
      ExactMorseGammaPartitionSweepBudget::
          maximum_supported_child_reference_count;
  budget.maximum_batch_record_count =
      ExactMorseGammaPartitionSweepBudget::
          maximum_supported_batch_record_count;
  budget.maximum_contraction_group_count =
      ExactMorseGammaPartitionSweepBudget::
          maximum_supported_contraction_group_count;
  budget.maximum_group_root_reference_count =
      ExactMorseGammaPartitionSweepBudget::
          maximum_supported_group_root_reference_count;
  budget.maximum_batch_reference_count =
      ExactMorseGammaPartitionSweepBudget::
          maximum_supported_batch_reference_count;
  budget.maximum_checkpoint_count =
      ExactMorseGammaPartitionSweepBudget::
          maximum_supported_checkpoint_count;
  return budget;
}

[[nodiscard]] bool complete_sweep_facts(
    const ExactMorseGammaPartitionSweepResult& result) {
  return result.conservative_preflight_bounds_certified &&
         result.preflight_budget_sufficient &&
         result.critical_catalog_fresh_and_generic &&
         result.every_rank_k_birth_has_one_canonical_record &&
         result.every_rank_k_plus_one_saddle_family_is_complete &&
         result.every_arm_terminal_maps_to_one_strictly_earlier_birth &&
         result.all_saddle_targets_resolved_from_frozen_pre_batch_roots &&
         result.equal_level_saddles_contracted_as_one_hypergraph &&
         result.contractions_invariant_under_saddle_permutation &&
         result.local_genealogy_is_canonical_and_acyclic &&
         result.gamma_oracle_started_only_after_complete_morse_genealogy &&
         result.gamma_activation_catalog_fresh_and_complete &&
         result.every_morse_batch_level_is_a_gamma_activation_level &&
         result.strict_partitions_biject_gamma_at_every_activation_level &&
         result.closed_partitions_biject_gamma_at_every_activation_level &&
         result.gamma_objects_never_select_morse_births_targets_or_unions &&
         result.
             records_are_internal_falsifier_objects_not_public_forest_or_attachments &&
         result.diagnostic_outcomes_have_no_genealogy_payload &&
         result.morse_gamma_partition_sweep_certified &&
         result.decision ==
             ExactMorseGammaPartitionSweepDecision::
                 complete_morse_gamma_partition_sweep &&
         result.scope ==
             ExactMorseGammaPartitionSweepScope::
                 bounded_n14_k10_single_order_morse_minimum_saddle_partition_sweep_compared_to_exhaustive_gamma_at_every_activation_level_only;
}

void test_generic_3d_catalog_exercises_supports_two_three_and_four() {
  const std::array<CertifiedPoint3, 6> points{
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0),
      point(-9.0, 3.0, 16.0),
      point(4.0, -5.0, 4.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const ExactCriticalCatalogBudget budget = full_catalog_budget();
  const auto result = build_exact_critical_catalog(cloud, 6U, budget);
  const auto verification =
      morsehgp3d::hierarchy::verify_exact_critical_catalog(
          cloud, 6U, budget, result);

  std::array<std::size_t, 5> support_counts{};
  std::array<std::size_t, 7> birth_counts{};
  std::array<std::size_t, 7> saddle_counts{};
  for (const auto& event : result.events) {
    ++support_counts.at(event.support_point_ids.size());
    if (event.birth_order.has_value()) {
      ++birth_counts.at(*event.birth_order);
    }
    if (event.saddle_order.has_value()) {
      ++saddle_counts.at(*event.saddle_order);
    }
  }

  check(
      result.decision ==
              ExactCriticalCatalogDecision::complete_supported_critical_catalog &&
          verification.fresh_replay_certified &&
          verification.exact_critical_catalog_decision_certified &&
          result.no_relevant_extra_shell_degeneracy &&
          support_counts[2] == 15U && support_counts[3] == 10U &&
          support_counts[4] == 2U,
      "the generic 3D fixture certifies nonempty minimal supports of sizes two, three and four");
  for (std::size_t order = 1U; order <= 6U; ++order) {
    check(
        birth_counts[order] > 0U || saddle_counts[order] > 0U,
        "the generic 3D catalogue exercises effective order " +
            std::to_string(order));
  }
}

void test_order_one_complete_graph_and_emst_cuts_agree() {
  const auto result = build_exact_complete_graph_emst(line_cloud(11U));
  check(
      result.emst_edges.size() == 10U && result.nodes.size() == 12U &&
          result.nodes.at(static_cast<std::size_t>(result.root_node_id))
                  .point_ids.size() == 11U,
      "k=1 builds the exact eleven-point EMST hierarchy");
  for (const auto& batch : result.equal_level_batches) {
    check(
        result.cut(
            batch.level,
            K1CutClosure::strict,
            K1CutEdgeSource::complete_graph) ==
            result.cut(
                batch.level,
                K1CutClosure::strict,
                K1CutEdgeSource::selected_emst),
        "k=1 strict complete-graph and EMST cuts agree");
    check(
        result.cut(
            batch.level,
            K1CutClosure::closed,
            K1CutEdgeSource::complete_graph) ==
            result.cut(
                batch.level,
                K1CutClosure::closed,
                K1CutEdgeSource::selected_emst),
        "k=1 closed complete-graph and EMST cuts agree");
  }
}

void test_orders_two_through_ten_match_gamma_at_every_level() {
  const ExactMorseGammaPartitionSweepBudget budget = full_sweep_budget();
  for (std::size_t order = 2U; order <= 10U; ++order) {
    const ExactMorseGammaPartitionSweepResult result =
        build_exact_morse_gamma_partition_sweep(
            line_cloud(order + 1U), order, budget);
    const bool every_checkpoint_bijects = std::all_of(
        result.oracle_checkpoints.begin(),
        result.oracle_checkpoints.end(),
        [](const auto& checkpoint) {
          return checkpoint.strict_birth_projection_is_bijective &&
                 checkpoint.closed_birth_projection_is_bijective;
        });
    const std::string context = "k=" + std::to_string(order);
    check(
        complete_sweep_facts(result),
        context + " certifies the Morse candidate before its posterior Gamma audit");
    check(
        result.point_count == order + 1U &&
            result.exhaustive_facet_count == order + 1U &&
            result.exhaustive_coface_count == 1U &&
            result.birth_records.size() == 2U &&
            result.saddle_records.size() == 1U &&
            result.final_root_node_ids.size() == 1U &&
            !result.oracle_checkpoints.empty() && every_checkpoint_bijects,
        context + " matches strict and closed exhaustive Gamma partitions at every activation level");
  }
}

}  // namespace

int main() {
  test_generic_3d_catalog_exercises_supports_two_three_and_four();
  test_order_one_complete_graph_and_emst_cuts_agree();
  test_orders_two_through_ten_match_gamma_at_every_level();

  if (failures != 0) {
    std::cerr << failures << " bounded Hartigan all-order gate(s) failed\n";
    return 1;
  }
  std::cout << "bounded Hartigan gates passed for every order k=1..10\n";
  return 0;
}
