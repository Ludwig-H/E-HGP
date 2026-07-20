#include "morsehgp3d/hierarchy/morse_gamma_partition_sweep.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using namespace morsehgp3d::hierarchy;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Function>
void check_invalid_argument(Function&& function, const std::string& message) {
  bool rejected = false;
  try {
    function();
  } catch (const std::invalid_argument&) {
    rejected = true;
  } catch (...) {
  }
  check(rejected, message);
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

[[nodiscard]] ExactLevel level(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] CanonicalPointCloud q2_triangle_cloud() {
  const std::array<CertifiedPoint3, 3> input{
      point(-2.0, 0.0), point(0.0, 1.0), point(2.0, 0.0)};
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud q2_twin_cloud() {
  const std::array<CertifiedPoint3, 3> input{
      point(-3.0, 0.0), point(0.0, 1.0), point(3.0, 0.0)};
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud mirror_cloud() {
  const std::array<CertifiedPoint3, 4> input{
      point(-2.0, 0.0),
      point(0.0, -3.0),
      point(0.0, 3.0),
      point(2.0, 0.0)};
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud shared_terminal_cloud() {
  const std::array<CertifiedPoint3, 5> input{
      point(-8.0, 1.0),
      point(-5.0, -7.0),
      point(-3.0, -8.0),
      point(4.0, 8.0),
      point(5.0, -7.0)};
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud continuation_cloud() {
  const std::array<CertifiedPoint3, 5> input{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0)};
  return canonical_cloud(input);
}

[[nodiscard]] ExactCriticalCatalogBudget full_catalog_budget() {
  return {
      ExactCriticalCatalogBudget::maximum_supported_candidate_count,
      ExactCriticalCatalogBudget::
          maximum_supported_point_classification_count};
}

[[nodiscard]] ExactStrictGammaBudget full_gamma_budget() {
  return {
      ExactStrictGammaBudget::maximum_supported_facet_count,
      ExactStrictGammaBudget::maximum_supported_coface_count,
      ExactStrictGammaBudget::maximum_supported_union_attempt_count};
}

[[nodiscard]] ExactPersistentReducedGammaOrderHistoryBudget
full_history_budget() {
  ExactPersistentReducedGammaOrderHistoryBudget budget;
  budget.gamma_budget = full_gamma_budget();
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

[[nodiscard]] ExactMorseGammaPartitionSweepBudget full_budget(
    std::size_t per_arm_chain_capacity = 2U) {
  ExactMorseGammaPartitionSweepBudget budget;
  budget.critical_catalog_budget = full_catalog_budget();
  budget.per_arm_chain_budget = {per_arm_chain_capacity};
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

[[nodiscard]] bool all_certificates_close(
    const ExactMorseGammaPartitionSweepVerification& verification) {
  return verification.requested_budget_certified &&
         verification.external_inputs_certified &&
         verification.derived_preflight_sizes_certified &&
         verification.subordinate_decisions_certified &&
         verification.diagnostic_witness_certified &&
         verification.birth_records_certified &&
         verification.saddle_records_certified &&
         verification.nodes_certified &&
         verification.contraction_groups_certified &&
         verification.batch_records_certified &&
         verification.oracle_checkpoints_certified &&
         verification.final_roots_certified &&
         verification.result_facts_certified &&
         verification.counters_certified &&
         verification.decision_certified &&
         verification.scope_certified && verification.fresh_replay_certified &&
         verification.exact_morse_gamma_partition_sweep_decision_certified;
}

[[nodiscard]] bool complete_facts(
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

void check_empty_payload(
    const ExactMorseGammaPartitionSweepResult& result,
    const std::string& context) {
  check(
      result.birth_records.empty() && result.saddle_records.empty() &&
          result.nodes.empty() && result.contraction_groups.empty() &&
          result.batch_records.empty() && result.oracle_checkpoints.empty() &&
          result.final_root_node_ids.empty(),
      context + " publishes no partial genealogy or oracle checkpoint");
}

[[nodiscard]] const ExactMorseGammaBirthRecord* birth_with_facet(
    const ExactMorseGammaPartitionSweepResult& result,
    const std::vector<PointId>& facet_point_ids) {
  const auto position = std::find_if(
      result.birth_records.begin(),
      result.birth_records.end(),
      [&](const ExactMorseGammaBirthRecord& birth) {
        return birth.facet_point_ids == facet_point_ids;
      });
  return position == result.birth_records.end() ? nullptr : &*position;
}

[[nodiscard]] const ExactMorseGammaSaddleRecord* saddle_with_shell(
    const ExactMorseGammaPartitionSweepResult& result,
    const std::vector<PointId>& shell_point_ids) {
  const auto position = std::find_if(
      result.saddle_records.begin(),
      result.saddle_records.end(),
      [&](const ExactMorseGammaSaddleRecord& saddle) {
        return saddle.shell_point_ids == shell_point_ids;
      });
  return position == result.saddle_records.end() ? nullptr : &*position;
}

[[nodiscard]] const ExactMorseGammaBatchRecord* batch_at(
    const ExactMorseGammaPartitionSweepResult& result,
    const ExactLevel& squared_level) {
  const auto position = std::find_if(
      result.batch_records.begin(),
      result.batch_records.end(),
      [&](const ExactMorseGammaBatchRecord& batch) {
        return batch.squared_level == squared_level;
      });
  return position == result.batch_records.end() ? nullptr : &*position;
}

[[nodiscard]] const ExactMorseGammaOracleCheckpoint* checkpoint_at(
    const ExactMorseGammaPartitionSweepResult& result,
    const ExactLevel& squared_level) {
  const auto position = std::find_if(
      result.oracle_checkpoints.begin(),
      result.oracle_checkpoints.end(),
      [&](const ExactMorseGammaOracleCheckpoint& checkpoint) {
        return checkpoint.squared_level == squared_level;
      });
  return position == result.oracle_checkpoints.end() ? nullptr : &*position;
}

[[nodiscard]] const ExactMorseGammaContractionGroup* group_for_saddle(
    const ExactMorseGammaPartitionSweepResult& result,
    const ExactMorseGammaSaddleRecord* saddle) {
  if (saddle == nullptr ||
      saddle->contraction_group_index >= result.contraction_groups.size()) {
    return nullptr;
  }
  return &result.contraction_groups[saddle->contraction_group_index];
}

void test_q2_complete_sweep(
    const CanonicalPointCloud& cloud,
    const ExactMorseGammaPartitionSweepBudget& budget,
    const ExactMorseGammaPartitionSweepResult& result) {
  const auto verification = verify_exact_morse_gamma_partition_sweep(
      cloud, 2U, budget, result);
  check(
      complete_facts(result) && all_certificates_close(verification),
      "q2 closes the direct Morse genealogy and posterior Gamma audit");
  check(
      result.critical_event_support_bound == 4U &&
          result.critical_arm_bound == 16U &&
          result.exhaustive_facet_count == 3U &&
          result.exhaustive_coface_count == 1U &&
          result.required_birth_record_capacity == 4U &&
          result.required_saddle_record_capacity == 4U &&
          result.required_arm_reference_capacity == 16U &&
          result.required_node_capacity == 7U &&
          result.required_child_reference_capacity == 6U &&
          result.required_batch_record_capacity == 4U &&
          result.required_contraction_group_capacity == 4U &&
          result.required_group_root_reference_capacity == 16U &&
          result.required_batch_reference_capacity == 12U &&
          result.required_checkpoint_capacity == 4U,
      "q2 exposes the ten conservative capacities before geometry");

  const ExactMorseGammaBirthRecord* birth01 =
      birth_with_facet(result, {0U, 1U});
  const ExactMorseGammaBirthRecord* birth12 =
      birth_with_facet(result, {1U, 2U});
  const ExactMorseGammaSaddleRecord* saddle =
      saddle_with_shell(result, {0U, 2U});
  const ExactMorseGammaBatchRecord* birth_batch = batch_at(result, level(5, 4));
  const ExactMorseGammaBatchRecord* saddle_batch = batch_at(result, level(4));
  const ExactMorseGammaContractionGroup* group =
      group_for_saddle(result, saddle);
  check(
      result.birth_records.size() == 2U &&
          result.saddle_records.size() == 1U && result.nodes.size() == 3U &&
          result.contraction_groups.size() == 1U &&
          result.batch_records.size() == 2U && birth01 != nullptr &&
          birth12 != nullptr && saddle != nullptr && birth_batch != nullptr &&
          saddle_batch != nullptr && group != nullptr,
      "q2 retains two minima, one saddle, one multifusion and two lots");
  if (birth01 != nullptr && birth12 != nullptr && saddle != nullptr &&
      birth_batch != nullptr && saddle_batch != nullptr && group != nullptr) {
    check(
        birth01->squared_level == level(5, 4) &&
            birth12->squared_level == level(5, 4) &&
            saddle->squared_level == level(4) &&
            saddle->terminal_birth_record_indices ==
                std::vector<std::size_t>{
                    birth12->birth_record_index,
                    birth01->birth_record_index} &&
            saddle->pre_batch_root_node_ids ==
                std::vector<std::size_t>{birth12->node_id, birth01->node_id},
        "q2 maps each shell removal to its unique strictly earlier minimum");
    std::vector<std::size_t> expected_roots{birth01->node_id, birth12->node_id};
    std::sort(expected_roots.begin(), expected_roots.end());
    check(
        birth_batch->birth_record_indices.size() == 2U &&
            birth_batch->saddle_record_indices.empty() &&
            birth_batch->strict_root_count == 0U &&
            birth_batch->closed_root_count == 2U &&
            saddle_batch->birth_record_indices.empty() &&
            saddle_batch->saddle_record_indices ==
                std::vector<std::size_t>{saddle->saddle_record_index} &&
            saddle_batch->strict_root_count == 2U &&
            saddle_batch->closed_root_count == 1U &&
            saddle_batch->all_saddles_resolved_from_frozen_pre_batch_roots &&
            saddle_batch->
                quotient_components_invariant_under_reversed_saddle_order &&
            saddle_batch->mutations_committed_after_complete_group_resolution,
        "q2 freezes both roots before committing its one equal-level contraction");
    check(
        group->saddle_record_indices ==
                std::vector<std::size_t>{saddle->saddle_record_index} &&
            group->prior_root_node_ids == expected_roots &&
            group->created_node_id.has_value() &&
            group->resulting_root_node_id == *group->created_node_id &&
            result.final_root_node_ids ==
                std::vector<std::size_t>{group->resulting_root_node_id},
        "q2 creates one canonical multifusion node from the two prior roots");
  }

  const ExactMorseGammaOracleCheckpoint* checkpoint =
      checkpoint_at(result, level(4));
  const bool every_checkpoint_closes = std::all_of(
      result.oracle_checkpoints.begin(),
      result.oracle_checkpoints.end(),
      [](const ExactMorseGammaOracleCheckpoint& current) {
        return current.strict_birth_projection_is_bijective &&
               current.closed_birth_projection_is_bijective;
      });
  check(
      checkpoint != nullptr && checkpoint->morse_batch_index.has_value() &&
          checkpoint->strict_morse_root_count == 2U &&
          checkpoint->strict_gamma_component_count == 2U &&
          checkpoint->closed_morse_root_count == 1U &&
          checkpoint->closed_gamma_component_count == 1U &&
          checkpoint->strict_birth_projection_is_bijective &&
          checkpoint->closed_birth_projection_is_bijective,
      "q2 agrees with the independent strict and closed Gamma partitions");
  check(
      result.oracle_checkpoints.size() == 2U && every_checkpoint_closes &&
          result.counters.gamma_transition_build_count ==
              result.oracle_checkpoints.size() &&
          result.counters.checkpoint_count == result.oracle_checkpoints.size() &&
          result.counters.strict_partition_bijection_count ==
              result.oracle_checkpoints.size() &&
          result.counters.closed_partition_bijection_count ==
              result.oracle_checkpoints.size(),
      "q2 audits every Gamma activation level, not only its saddle lot");
}

void test_preflight_caps_are_atomic(
    const CanonicalPointCloud& cloud,
    const ExactMorseGammaPartitionSweepBudget& full,
    const ExactMorseGammaPartitionSweepResult& baseline) {
  std::array<ExactMorseGammaPartitionSweepBudget, 10> insufficient{
      full, full, full, full, full, full, full, full, full, full};
  insufficient[0].maximum_birth_record_count =
      baseline.required_birth_record_capacity - 1U;
  insufficient[1].maximum_saddle_record_count =
      baseline.required_saddle_record_capacity - 1U;
  insufficient[2].maximum_arm_reference_count =
      baseline.required_arm_reference_capacity - 1U;
  insufficient[3].maximum_node_count = baseline.required_node_capacity - 1U;
  insufficient[4].maximum_child_reference_count =
      baseline.required_child_reference_capacity - 1U;
  insufficient[5].maximum_batch_record_count =
      baseline.required_batch_record_capacity - 1U;
  insufficient[6].maximum_contraction_group_count =
      baseline.required_contraction_group_capacity - 1U;
  insufficient[7].maximum_group_root_reference_count =
      baseline.required_group_root_reference_capacity - 1U;
  insufficient[8].maximum_batch_reference_count =
      baseline.required_batch_reference_capacity - 1U;
  insufficient[9].maximum_checkpoint_count =
      baseline.required_checkpoint_capacity - 1U;

  for (const auto& budget : insufficient) {
    const auto result =
        build_exact_morse_gamma_partition_sweep(cloud, 2U, budget);
    check(
        result.conservative_preflight_bounds_certified &&
            !result.preflight_budget_sufficient &&
            result.counters.preflight_count == 1U &&
            result.counters.critical_catalog_build_count == 0U &&
            result.counters.critical_arm_family_build_count == 0U &&
            result.counters.gamma_history_build_count == 0U &&
            result.counters.gamma_transition_build_count == 0U &&
            result.diagnostic_outcomes_have_no_genealogy_payload &&
            result.decision ==
                ExactMorseGammaPartitionSweepDecision::
                    no_sweep_preflight_budget_insufficient,
        "each one-below local capacity stops before catalogue, arms and Gamma");
    check_empty_payload(result, "one-below sweep capacity");
  }

  auto excessive = full;
  excessive.maximum_birth_record_count =
      ExactMorseGammaPartitionSweepBudget::
          maximum_supported_birth_record_count +
      1U;
  check_invalid_argument(
      [&] {
        static_cast<void>(
            build_exact_morse_gamma_partition_sweep(cloud, 2U, excessive));
      },
      "a local capacity above its proved static cap is rejected");

  auto excessive_nested = full;
  excessive_nested.gamma_oracle_history_budget.maximum_activation_level_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_activation_level_count +
      1U;
  check_invalid_argument(
      [&] {
        static_cast<void>(build_exact_morse_gamma_partition_sweep(
            cloud, 2U, excessive_nested));
      },
      "a nested Gamma history capacity above its proved static cap is rejected");
}

void test_mirror_simultaneous_saddles(
    const CanonicalPointCloud& cloud,
    const ExactMorseGammaPartitionSweepBudget& budget,
    const ExactMorseGammaPartitionSweepResult& result) {
  const auto verification = verify_exact_morse_gamma_partition_sweep(
      cloud, 2U, budget, result);
  const ExactMorseGammaBirthRecord* shared_birth =
      birth_with_facet(result, {0U, 3U});
  const ExactMorseGammaSaddleRecord* lower =
      saddle_with_shell(result, {0U, 1U, 3U});
  const ExactMorseGammaSaddleRecord* upper =
      saddle_with_shell(result, {0U, 2U, 3U});
  const ExactMorseGammaBatchRecord* batch =
      batch_at(result, level(169, 36));
  const ExactMorseGammaContractionGroup* lower_group =
      group_for_saddle(result, lower);
  const ExactMorseGammaContractionGroup* upper_group =
      group_for_saddle(result, upper);
  check(
      complete_facts(result) && all_certificates_close(verification) &&
          result.birth_records.size() == 5U &&
          result.saddle_records.size() == 2U &&
          result.counters.arm_reference_count == 6U &&
          shared_birth != nullptr && lower != nullptr && upper != nullptr &&
          batch != nullptr && lower_group != nullptr &&
          lower_group == upper_group,
      "the mirror fixture closes two simultaneous saddles over five minima");
  if (shared_birth != nullptr && lower != nullptr && upper != nullptr &&
      batch != nullptr && lower_group != nullptr &&
      lower_group == upper_group) {
    const auto lower_shared = std::find(
        lower->terminal_birth_record_indices.begin(),
        lower->terminal_birth_record_indices.end(),
        shared_birth->birth_record_index);
    const auto upper_shared = std::find(
        upper->terminal_birth_record_indices.begin(),
        upper->terminal_birth_record_indices.end(),
        shared_birth->birth_record_index);
    bool shared_root_reused = false;
    if (lower_shared != lower->terminal_birth_record_indices.end() &&
        upper_shared != upper->terminal_birth_record_indices.end()) {
      const std::size_t lower_position = static_cast<std::size_t>(
          lower_shared - lower->terminal_birth_record_indices.begin());
      const std::size_t upper_position = static_cast<std::size_t>(
          upper_shared - upper->terminal_birth_record_indices.begin());
      shared_root_reused =
          lower_position < lower->pre_batch_root_node_ids.size() &&
          upper_position < upper->pre_batch_root_node_ids.size() &&
          lower->pre_batch_root_node_ids[lower_position] ==
              shared_birth->node_id &&
          upper->pre_batch_root_node_ids[upper_position] ==
              shared_birth->node_id;
    }
    check(
        lower->batch_index == upper->batch_index &&
            lower->terminal_birth_record_indices.size() == 3U &&
            upper->terminal_birth_record_indices.size() == 3U &&
            lower->pre_batch_root_node_ids.size() == 3U &&
            upper->pre_batch_root_node_ids.size() == 3U &&
            lower_shared != lower->terminal_birth_record_indices.end() &&
            upper_shared != upper->terminal_birth_record_indices.end() &&
            shared_root_reused,
        "the two mirror saddles share the same frozen {0,3} minimum root");
    check(
        batch->saddle_record_indices.size() == 2U &&
            batch->contraction_group_indices.size() == 1U &&
            batch->strict_root_count == 5U &&
            batch->closed_root_count == 1U &&
            batch->all_saddles_resolved_from_frozen_pre_batch_roots &&
            batch->quotient_components_invariant_under_reversed_saddle_order &&
            batch->mutations_committed_after_complete_group_resolution &&
            lower_group->saddle_record_indices.size() == 2U &&
            lower_group->prior_root_node_ids.size() == 5U &&
            lower_group->created_node_id.has_value() &&
            result.counters.reversed_order_group_comparison_count > 0U,
        "the overlapping mirror hyperedges are quotient-contracted once, never sequentialized");
  }

  const ExactMorseGammaOracleCheckpoint* checkpoint =
      checkpoint_at(result, level(169, 36));
  const ExactMorseGammaOracleCheckpoint* residual_checkpoint =
      checkpoint_at(result, level(9));
  const bool every_checkpoint_closes = std::all_of(
      result.oracle_checkpoints.begin(),
      result.oracle_checkpoints.end(),
      [](const ExactMorseGammaOracleCheckpoint& current) {
        return current.strict_birth_projection_is_bijective &&
               current.closed_birth_projection_is_bijective;
      });
  check(
      checkpoint != nullptr && checkpoint->morse_batch_index.has_value() &&
          checkpoint->strict_morse_root_count == 5U &&
          checkpoint->strict_gamma_component_count == 5U &&
          checkpoint->closed_morse_root_count == 1U &&
          checkpoint->closed_gamma_component_count == 1U &&
          checkpoint->strict_birth_projection_is_bijective &&
          checkpoint->closed_birth_projection_is_bijective,
      "the simultaneous mirror quotient matches strict and closed Gamma");
  check(
      residual_checkpoint != nullptr &&
          !residual_checkpoint->morse_batch_index.has_value() &&
          residual_checkpoint->strict_morse_root_count ==
              residual_checkpoint->strict_gamma_component_count &&
          residual_checkpoint->closed_morse_root_count ==
              residual_checkpoint->closed_gamma_component_count &&
          residual_checkpoint->strict_birth_projection_is_bijective &&
          residual_checkpoint->closed_birth_projection_is_bijective,
      "the level-nine mirror checkpoint audits Gamma without inventing a Morse batch");
  check(
      every_checkpoint_closes &&
          result.counters.gamma_transition_build_count ==
              result.oracle_checkpoints.size() &&
          result.counters.checkpoint_count == result.oracle_checkpoints.size() &&
          result.counters.strict_partition_bijection_count ==
              result.oracle_checkpoints.size() &&
          result.counters.closed_partition_bijection_count ==
              result.oracle_checkpoints.size(),
      "the mirror audit also covers every residual Gamma activation level");
}

void test_shared_terminal_is_one_minimum(
    const CanonicalPointCloud& cloud,
    const ExactMorseGammaPartitionSweepBudget& budget,
    const ExactMorseGammaPartitionSweepResult& result) {
  const auto verification = verify_exact_morse_gamma_partition_sweep(
      cloud, 3U, budget, result);
  const ExactMorseGammaBirthRecord* birth012 =
      birth_with_facet(result, {0U, 1U, 2U});
  const ExactMorseGammaBirthRecord* birth013 =
      birth_with_facet(result, {0U, 1U, 3U});
  const ExactMorseGammaSaddleRecord* saddle =
      saddle_with_shell(result, {1U, 2U, 3U});
  const ExactMorseGammaContractionGroup* group =
      group_for_saddle(result, saddle);
  check(
      complete_facts(result) && all_certificates_close(verification) &&
          result.saddle_records.size() == 3U &&
          result.counters.arm_reference_count == 7U && birth012 != nullptr &&
          birth013 != nullptr && saddle != nullptr && group != nullptr,
      "the order-three fixture closes all families including its shared terminal");
  if (birth012 != nullptr && birth013 != nullptr && saddle != nullptr &&
      group != nullptr) {
    check(
        saddle->squared_level == level(25925, 338) &&
            saddle->terminal_birth_record_indices ==
                std::vector<std::size_t>{
                    birth012->birth_record_index,
                    birth013->birth_record_index,
                    birth012->birth_record_index} &&
            saddle->pre_batch_root_node_ids.size() == 3U &&
            saddle->pre_batch_root_node_ids[0] ==
                saddle->pre_batch_root_node_ids[2] &&
            saddle->pre_batch_root_node_ids[0] !=
                saddle->pre_batch_root_node_ids[1] &&
            group->prior_root_node_ids.size() == 2U &&
            group->created_node_id.has_value(),
        "two shell removals keep distinct arms while sharing the unique {0,1,2} minimum");
  }
}

void test_e5_exercises_continuation() {
  const CanonicalPointCloud cloud = continuation_cloud();
  const ExactMorseGammaPartitionSweepResult result =
      build_exact_morse_gamma_partition_sweep(cloud, 2U, full_budget(2U));
  check(
      complete_facts(result) && result.counters.continuation_group_count > 0U,
      "E5 closes the complete sweep while exercising a one-root continuation group");
}

void test_fresh_verifier_rejects_mutations_and_twin(
    const CanonicalPointCloud& cloud,
    const ExactMorseGammaPartitionSweepBudget& budget,
    const ExactMorseGammaPartitionSweepResult& baseline) {
  check(
      complete_facts(baseline) && !baseline.birth_records.empty() &&
          !baseline.batch_records.empty() &&
          !baseline.oracle_checkpoints.empty(),
      "the q2 mutation baseline exposes every compact layer");
  if (baseline.birth_records.empty() || baseline.batch_records.empty() ||
      baseline.oracle_checkpoints.empty()) {
    return;
  }

  auto bad_record = baseline;
  ++bad_record.birth_records.front().node_id;
  const auto record_verification = verify_exact_morse_gamma_partition_sweep(
      cloud, 2U, budget, bad_record);
  check(
      !record_verification.birth_records_certified &&
          !record_verification.fresh_replay_certified &&
          !record_verification.
              exact_morse_gamma_partition_sweep_decision_certified,
      "fresh replay rejects an altered minimum record");

  auto bad_checkpoint = baseline;
  ++bad_checkpoint.oracle_checkpoints.front().strict_gamma_component_count;
  const auto checkpoint_verification =
      verify_exact_morse_gamma_partition_sweep(
          cloud, 2U, budget, bad_checkpoint);
  check(
      !checkpoint_verification.oracle_checkpoints_certified &&
          !checkpoint_verification.fresh_replay_certified &&
          !checkpoint_verification.
              exact_morse_gamma_partition_sweep_decision_certified,
      "fresh replay rejects an altered posterior Gamma checkpoint");

  auto permuted_batches = baseline;
  std::reverse(
      permuted_batches.batch_records.begin(),
      permuted_batches.batch_records.end());
  const auto batch_verification = verify_exact_morse_gamma_partition_sweep(
      cloud, 2U, budget, permuted_batches);
  check(
      !batch_verification.batch_records_certified &&
          !batch_verification.fresh_replay_certified,
      "fresh replay rejects a noncanonical permutation of distinct-level lots");

  auto bad_fact_and_decision = baseline;
  bad_fact_and_decision.contractions_invariant_under_saddle_permutation =
      false;
  bad_fact_and_decision.decision =
      ExactMorseGammaPartitionSweepDecision::not_certified;
  const auto fact_verification = verify_exact_morse_gamma_partition_sweep(
      cloud, 2U, budget, bad_fact_and_decision);
  check(
      !fact_verification.result_facts_certified &&
          !fact_verification.decision_certified &&
          !fact_verification.fresh_replay_certified &&
          !fact_verification.
              exact_morse_gamma_partition_sweep_decision_certified,
      "fresh replay rejects a promoted fact and decision mutation");

  const CanonicalPointCloud twin = q2_twin_cloud();
  const auto twin_verification = verify_exact_morse_gamma_partition_sweep(
      twin, 2U, budget, baseline);
  check(
      !twin_verification.fresh_replay_certified &&
          !twin_verification.
              exact_morse_gamma_partition_sweep_decision_certified,
      "a same-size twin cloud cannot reuse the q2 genealogy or Gamma audit");
}

}  // namespace

int main() {
  const CanonicalPointCloud q2 = q2_triangle_cloud();
  const ExactMorseGammaPartitionSweepBudget q2_budget = full_budget(1U);
  const ExactMorseGammaPartitionSweepResult q2_result =
      build_exact_morse_gamma_partition_sweep(q2, 2U, q2_budget);
  test_q2_complete_sweep(q2, q2_budget, q2_result);
  test_preflight_caps_are_atomic(q2, q2_budget, q2_result);
  test_fresh_verifier_rejects_mutations_and_twin(
      q2, q2_budget, q2_result);

  const CanonicalPointCloud mirror = mirror_cloud();
  const ExactMorseGammaPartitionSweepBudget mirror_budget = full_budget(1U);
  const ExactMorseGammaPartitionSweepResult mirror_result =
      build_exact_morse_gamma_partition_sweep(mirror, 2U, mirror_budget);
  test_mirror_simultaneous_saddles(
      mirror, mirror_budget, mirror_result);

  const CanonicalPointCloud shared = shared_terminal_cloud();
  const ExactMorseGammaPartitionSweepBudget shared_budget = full_budget(2U);
  const ExactMorseGammaPartitionSweepResult shared_result =
      build_exact_morse_gamma_partition_sweep(shared, 3U, shared_budget);
  test_shared_terminal_is_one_minimum(
      shared, shared_budget, shared_result);

  test_e5_exercises_continuation();

  if (failures != 0) {
    std::cerr << failures << " Morse--Gamma partition sweep test(s) failed\n";
    return 1;
  }
  std::cout << "all Morse--Gamma partition sweep tests passed\n";
  return 0;
}
