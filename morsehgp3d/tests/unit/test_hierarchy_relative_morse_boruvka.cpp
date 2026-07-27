#include "morsehgp3d/hierarchy/morse_gamma_partition_sweep.hpp"
#include "morsehgp3d/hierarchy/relative_morse_boruvka.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::BigInt;
using namespace morsehgp3d::hierarchy;
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

[[nodiscard]] ExactLevel level(std::int64_t numerator) {
  return ExactLevel{BigInt{numerator}, BigInt{1}};
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud q2_triangle_cloud() {
  const std::array<CertifiedPoint3, 3> input{
      point(-2.0, 0.0), point(0.0, 1.0), point(2.0, 0.0)};
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

[[nodiscard]] ExactMorseGammaPartitionSweepBudget full_sweep_budget(
    std::size_t per_arm_chain_capacity) {
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

[[nodiscard]] ExactRelativeMorseBoruvkaInput adapt_certified_sweep(
    const ExactMorseGammaPartitionSweepResult& sweep) {
  ExactRelativeMorseBoruvkaInput input;
  input.point_count = sweep.point_count;
  input.order = sweep.order;
  input.births.reserve(sweep.birth_records.size());
  input.saddles.reserve(sweep.saddle_records.size());
  input.terminal_birth_indices.reserve(sweep.counters.arm_reference_count);
  for (const ExactMorseGammaBirthRecord& birth : sweep.birth_records) {
    input.births.push_back(ExactRelativeMorseBoruvkaBirth{
        birth.birth_record_index, birth.squared_level});
  }
  for (const ExactMorseGammaSaddleRecord& saddle : sweep.saddle_records) {
    input.saddles.push_back(ExactRelativeMorseBoruvkaSaddle{
        saddle.saddle_record_index,
        saddle.squared_level,
        input.terminal_birth_indices.size(),
        saddle.terminal_birth_record_indices.size()});
    input.terminal_birth_indices.insert(
        input.terminal_birth_indices.end(),
        saddle.terminal_birth_record_indices.begin(),
        saddle.terminal_birth_record_indices.end());
  }
  return input;
}

[[nodiscard]] ExactRelativeMorseBoruvkaBudget full_relative_budget(
    const ExactRelativeMorseBoruvkaInput& input) {
  std::size_t canonical_link_count = 0U;
  for (const ExactRelativeMorseBoruvkaSaddle& saddle : input.saddles) {
    const auto begin = input.terminal_birth_indices.begin() +
        static_cast<std::ptrdiff_t>(
            saddle.terminal_birth_reference_offset);
    const auto end = begin + static_cast<std::ptrdiff_t>(
        saddle.terminal_birth_reference_count);
    std::vector<std::size_t> distinct(begin, end);
    std::sort(distinct.begin(), distinct.end());
    distinct.erase(std::unique(distinct.begin(), distinct.end()), distinct.end());
    if (!distinct.empty()) {
      canonical_link_count += distinct.size() - 1U;
    }
  }
  const std::size_t round_capacity = input.births.size() <= 1U ||
          canonical_link_count == 0U
      ? 0U
      : std::min(
            static_cast<std::size_t>(
                std::bit_width(input.births.size() - 1U)),
            canonical_link_count);
  const std::size_t selected_link_capacity = input.births.empty()
      ? 0U
      : std::min(canonical_link_count, input.births.size() - 1U);
  const std::size_t retained_capacity =
      std::min(input.saddles.size(), selected_link_capacity);
  return ExactRelativeMorseBoruvkaBudget{
      input.births.size(),
      input.saddles.size(),
      input.terminal_birth_indices.size(),
      canonical_link_count,
      round_capacity,
      2U * selected_link_capacity,
      selected_link_capacity,
      retained_capacity};
}

[[nodiscard]] bool verification_closes(
    const ExactRelativeMorseBoruvkaVerification& verification) {
  return verification.requested_budget_certified &&
      verification.requirements_certified &&
      verification.rounds_certified &&
      verification.selected_links_and_saddles_certified &&
      verification.final_components_certified &&
      verification.audit_certified &&
      verification.result_facts_certified &&
      verification.decision_and_scope_certified &&
      verification.fresh_replay_certified &&
      verification.result_certified;
}

[[nodiscard]] bool complete_relative_facts(
    const ExactRelativeMorseBoruvkaResult& result) {
  return result.certified_relative_partition_sparsifier() &&
      result.input_shape_certified && result.budget_preflight_certified &&
      result.canonical_star_expansion_certified &&
      result.roots_frozen_in_every_round &&
      result.all_exact_cominimizers_scanned_for_every_root_cut &&
      result.exactly_one_canonical_link_selected_per_root &&
      result.selected_links_are_a_canonical_expansion_subset &&
      result.frozen_selected_links_contracted_atomically &&
      result.retained_saddles_form_a_canonical_weighted_partition_basis &&
      result.all_input_threshold_partitions_preserved_relative_to_input &&
      !result.caller_catalog_completeness_certified &&
      !result.silent_incidence_coverage_certified &&
      !result.m1_certified && !result.hierarchy_or_forest_constructed &&
      !result.public_status_claimed &&
      !result.diagnostic_outcomes_have_no_payload;
}

class DisjointSet {
 public:
  explicit DisjointSet(std::size_t size) : parents_(size) {
    std::iota(parents_.begin(), parents_.end(), std::size_t{0});
  }

  [[nodiscard]] std::size_t find(std::size_t value) {
    std::size_t root = value;
    while (parents_[root] != root) {
      root = parents_[root];
    }
    while (parents_[value] != value) {
      const std::size_t parent = parents_[value];
      parents_[value] = root;
      value = parent;
    }
    return root;
  }

  void unite(std::size_t left, std::size_t right) {
    left = find(left);
    right = find(right);
    if (left != right) {
      parents_[std::max(left, right)] = std::min(left, right);
    }
  }

 private:
  std::vector<std::size_t> parents_;
};

[[nodiscard]] bool active_at(
    const ExactLevel& event_level,
    const ExactLevel& threshold,
    bool closed) {
  return event_level < threshold || (closed && event_level == threshold);
}

[[nodiscard]] DisjointSet partition_at(
    const ExactRelativeMorseBoruvkaInput& input,
    const std::vector<std::size_t>& saddle_indices,
    const ExactLevel& threshold,
    bool closed) {
  DisjointSet partition(input.births.size());
  for (const std::size_t saddle_index : saddle_indices) {
    const ExactRelativeMorseBoruvkaSaddle& saddle =
        input.saddles[saddle_index];
    if (!active_at(saddle.squared_level, threshold, closed)) {
      continue;
    }
    const std::size_t first_birth = input.terminal_birth_indices[
        saddle.terminal_birth_reference_offset];
    for (std::size_t relative_index = 1U;
         relative_index < saddle.terminal_birth_reference_count;
         ++relative_index) {
      partition.unite(
          first_birth,
          input.terminal_birth_indices[
              saddle.terminal_birth_reference_offset + relative_index]);
    }
  }
  return partition;
}

[[nodiscard]] bool same_active_partition(
    const ExactRelativeMorseBoruvkaInput& input,
    const std::vector<std::size_t>& full_saddles,
    const std::vector<std::size_t>& retained_saddles,
    const ExactLevel& threshold,
    bool closed) {
  DisjointSet full = partition_at(input, full_saddles, threshold, closed);
  DisjointSet retained =
      partition_at(input, retained_saddles, threshold, closed);
  for (std::size_t left = 0U; left < input.births.size(); ++left) {
    if (!active_at(input.births[left].squared_level, threshold, closed)) {
      continue;
    }
    for (std::size_t right = left + 1U; right < input.births.size();
         ++right) {
      if (!active_at(input.births[right].squared_level, threshold, closed)) {
        continue;
      }
      if ((full.find(left) == full.find(right)) !=
          (retained.find(left) == retained.find(right))) {
        return false;
      }
    }
  }
  return true;
}

void check_every_threshold_partition(
    const ExactMorseGammaPartitionSweepResult& sweep,
    const ExactRelativeMorseBoruvkaInput& input,
    const ExactRelativeMorseBoruvkaResult& result,
    const std::string& context) {
  std::vector<ExactLevel> levels;
  levels.reserve(
      input.births.size() + input.saddles.size() +
      sweep.oracle_checkpoints.size());
  for (const ExactRelativeMorseBoruvkaBirth& birth : input.births) {
    levels.push_back(birth.squared_level);
  }
  for (const ExactRelativeMorseBoruvkaSaddle& saddle : input.saddles) {
    levels.push_back(saddle.squared_level);
  }
  for (const ExactMorseGammaOracleCheckpoint& checkpoint :
       sweep.oracle_checkpoints) {
    levels.push_back(checkpoint.squared_level);
  }
  std::sort(levels.begin(), levels.end());
  levels.erase(std::unique(levels.begin(), levels.end()), levels.end());

  std::vector<std::size_t> full_saddles(input.saddles.size());
  std::iota(full_saddles.begin(), full_saddles.end(), std::size_t{0});
  bool every_strict_partition_matches = true;
  bool every_closed_partition_matches = true;
  for (const ExactLevel& threshold : levels) {
    every_strict_partition_matches = every_strict_partition_matches &&
        same_active_partition(
            input,
            full_saddles,
            result.retained_saddle_indices,
            threshold,
            false);
    every_closed_partition_matches = every_closed_partition_matches &&
        same_active_partition(
            input,
            full_saddles,
            result.retained_saddle_indices,
            threshold,
            true);
  }
  check(
      !levels.empty() && every_strict_partition_matches,
      context + " preserves every strict partition of the supplied authority");
  check(
      !levels.empty() && every_closed_partition_matches,
      context + " preserves every closed partition of the supplied authority");
}

struct FixtureResult {
  ExactMorseGammaPartitionSweepResult sweep;
  ExactRelativeMorseBoruvkaInput input;
  ExactRelativeMorseBoruvkaBudget budget;
  ExactRelativeMorseBoruvkaResult result;
};

[[nodiscard]] FixtureResult evaluate_fixture(
    const CanonicalPointCloud& cloud,
    std::size_t order,
    std::size_t per_arm_chain_capacity,
    const std::string& context) {
  const ExactMorseGammaPartitionSweepBudget sweep_budget =
      full_sweep_budget(per_arm_chain_capacity);
  FixtureResult fixture;
  fixture.sweep =
      build_exact_morse_gamma_partition_sweep(cloud, order, sweep_budget);
  check(
      fixture.sweep.morse_gamma_partition_sweep_certified &&
          fixture.sweep.decision ==
              ExactMorseGammaPartitionSweepDecision::
                  complete_morse_gamma_partition_sweep &&
          fixture.sweep.scope ==
              ExactMorseGammaPartitionSweepScope::
                  bounded_n14_k10_single_order_morse_minimum_saddle_partition_sweep_compared_to_exhaustive_gamma_at_every_activation_level_only,
      context + " starts from a certified sweep-6.23 authority");

  fixture.input = adapt_certified_sweep(fixture.sweep);
  fixture.budget = full_relative_budget(fixture.input);
  fixture.result =
      build_exact_relative_morse_boruvka(fixture.input, fixture.budget);
  const ExactRelativeMorseBoruvkaVerification verification =
      verify_exact_relative_morse_boruvka(
          fixture.input, fixture.budget, fixture.result);
  check(
      complete_relative_facts(fixture.result) &&
          verification_closes(verification),
      context + " closes the relative Morse--Boruvka transcript");
  check_every_threshold_partition(
      fixture.sweep, fixture.input, fixture.result, context);
  return fixture;
}

void test_q2_and_fresh_verifier(const FixtureResult& fixture) {
  check(
      fixture.input.births.size() == 2U &&
          fixture.input.saddles.size() == 1U &&
          fixture.result.retained_saddle_indices ==
              std::vector<std::size_t>{0U} &&
          fixture.result.audit.initial_component_count == 2U &&
          fixture.result.audit.final_component_count == 1U,
      "q2 retains its unique saddle and connects its two minima");
  check(
      fixture.result.rounds.size() == 1U &&
          fixture.result.root_choices.size() == 2U &&
          fixture.result.canonical_links.size() == 1U &&
          fixture.result.selected_links.size() == 1U &&
          fixture.result.audit.exact_cominimizer_link_observation_count ==
              2U,
      "q2 records both frozen-root cut choices and their common saddle");

  auto bad_retained = fixture.result;
  ++bad_retained.retained_saddle_indices.front();
  const ExactRelativeMorseBoruvkaVerification retained_verification =
      verify_exact_relative_morse_boruvka(
          fixture.input, fixture.budget, bad_retained);
  check(
      !retained_verification.selected_links_and_saddles_certified &&
          !retained_verification.fresh_replay_certified &&
          !retained_verification.result_certified,
      "fresh replay rejects a mutated retained saddle");

  auto bad_round = fixture.result;
  ++bad_round.rounds.front().post_round_component_count;
  const ExactRelativeMorseBoruvkaVerification round_verification =
      verify_exact_relative_morse_boruvka(
          fixture.input, fixture.budget, bad_round);
  check(
      !round_verification.rounds_certified &&
          !round_verification.fresh_replay_certified &&
          !round_verification.result_certified,
      "fresh replay rejects a mutated Boruvka round");

  auto bad_cominimizer_count = fixture.result;
  ++bad_cominimizer_count.root_choices.front().exact_cominimizer_link_count;
  const ExactRelativeMorseBoruvkaVerification count_verification =
      verify_exact_relative_morse_boruvka(
          fixture.input, fixture.budget, bad_cominimizer_count);
  check(
      !count_verification.rounds_certified &&
          !count_verification.fresh_replay_certified &&
          !count_verification.result_certified,
      "fresh rescan rejects a mutated exact co-minimizer count");

  auto bad_chosen_link = fixture.result;
  ++bad_chosen_link.root_choices.front().selected_canonical_link_index;
  const ExactRelativeMorseBoruvkaVerification chosen_link_verification =
      verify_exact_relative_morse_boruvka(
          fixture.input, fixture.budget, bad_chosen_link);
  check(
      !chosen_link_verification.rounds_certified &&
          !chosen_link_verification.fresh_replay_certified &&
          !chosen_link_verification.result_certified,
      "fresh rescan rejects a mutated canonical cut-minimum link");

  auto bad_fact = fixture.result;
  bad_fact.all_input_threshold_partitions_preserved_relative_to_input = false;
  const ExactRelativeMorseBoruvkaVerification fact_verification =
      verify_exact_relative_morse_boruvka(
          fixture.input, fixture.budget, bad_fact);
  check(
      !fact_verification.result_facts_certified &&
          !fact_verification.fresh_replay_certified &&
          !fact_verification.result_certified,
      "fresh replay rejects a mutated relative-only fact");

  auto bad_decision = fixture.result;
  bad_decision.decision = ExactRelativeMorseBoruvkaDecision::not_certified;
  const ExactRelativeMorseBoruvkaVerification decision_verification =
      verify_exact_relative_morse_boruvka(
          fixture.input, fixture.budget, bad_decision);
  check(
      !decision_verification.decision_and_scope_certified &&
          !decision_verification.fresh_replay_certified &&
          !decision_verification.result_certified,
      "fresh replay rejects a mutated public decision");
}

void test_mirror_simultaneous_saddles() {
  const FixtureResult fixture =
      evaluate_fixture(mirror_cloud(), 2U, 1U, "mirror");
  check(
      fixture.input.births.size() == 5U &&
          fixture.input.saddles.size() == 2U &&
          fixture.result.retained_saddle_indices ==
              std::vector<std::size_t>{0U, 1U} &&
          fixture.result.audit.final_component_count == 1U,
      "mirror retains both equal-level overlapping hyperedges");
  check(
      fixture.result.canonical_links.size() == 4U,
      "mirror expands its two ternary saddles into four canonical links");
  const bool every_choice_has_the_simultaneous_level = std::all_of(
      fixture.result.root_choices.begin(),
      fixture.result.root_choices.end(),
      [&](const ExactRelativeMorseBoruvkaRootChoice& choice) {
        const ExactRelativeMorseBoruvkaLink& link =
            fixture.result.canonical_links[
                choice.selected_canonical_link_index];
        return fixture.input.saddles[link.saddle_index].squared_level ==
            fixture.input.saddles.front().squared_level;
      });
  check(
      !fixture.result.rounds.empty() &&
          fixture.result.rounds.size() <=
              fixture.result.requirements.required_round_capacity &&
          every_choice_has_the_simultaneous_level,
      "mirror keeps its exact simultaneous level independent of internal Boruvka rounds");
  check(
      fixture.result.audit.exact_cominimizer_link_observation_count >= 5U,
      "mirror audits every exact co-minimizer before canonical selection");
}

void test_shared_terminal_multiplicity() {
  const FixtureResult fixture =
      evaluate_fixture(shared_terminal_cloud(), 3U, 2U, "shared terminal");
  bool duplicate_arm_preserved = false;
  for (const ExactRelativeMorseBoruvkaSaddle& saddle :
       fixture.input.saddles) {
    const auto begin = fixture.input.terminal_birth_indices.begin() +
        static_cast<std::ptrdiff_t>(
            saddle.terminal_birth_reference_offset);
    const auto end = begin + static_cast<std::ptrdiff_t>(
        saddle.terminal_birth_reference_count);
    std::vector<std::size_t> distinct(begin, end);
    std::sort(distinct.begin(), distinct.end());
    distinct.erase(std::unique(distinct.begin(), distinct.end()), distinct.end());
    duplicate_arm_preserved = duplicate_arm_preserved ||
        distinct.size() < saddle.terminal_birth_reference_count;
  }
  const bool every_reference_is_a_birth = std::all_of(
      fixture.input.terminal_birth_indices.begin(),
      fixture.input.terminal_birth_indices.end(),
      [&](std::size_t birth_index) {
        return birth_index < fixture.input.births.size();
      });
  check(
      duplicate_arm_preserved && every_reference_is_a_birth &&
          fixture.result.final_component_representative_birth_indices.size() ==
              fixture.input.births.size(),
      "shared-terminal keeps repeated arms without inventing a birth vertex");
}

void test_e5_continuations_are_not_edges() {
  const FixtureResult fixture =
      evaluate_fixture(continuation_cloud(), 2U, 2U, "E5");
  std::vector<std::size_t> continuation_saddles;
  for (const ExactMorseGammaContractionGroup& group :
       fixture.sweep.contraction_groups) {
    if (group.prior_root_node_ids.size() == 1U) {
      continuation_saddles.insert(
          continuation_saddles.end(),
          group.saddle_record_indices.begin(),
          group.saddle_record_indices.end());
    }
  }
  std::sort(continuation_saddles.begin(), continuation_saddles.end());
  continuation_saddles.erase(
      std::unique(
          continuation_saddles.begin(), continuation_saddles.end()),
      continuation_saddles.end());
  const bool none_selected = std::none_of(
      fixture.result.selected_links.begin(),
      fixture.result.selected_links.end(),
      [&](const ExactRelativeMorseBoruvkaSelectedLink& selected) {
        return std::find(
                   continuation_saddles.begin(),
                   continuation_saddles.end(),
                   selected.saddle_index) != continuation_saddles.end();
      });
  const bool none_retained = std::none_of(
      fixture.result.retained_saddle_indices.begin(),
      fixture.result.retained_saddle_indices.end(),
      [&](std::size_t saddle_index) {
        return std::find(
                   continuation_saddles.begin(),
                   continuation_saddles.end(),
                   saddle_index) != continuation_saddles.end();
      });
  check(
      fixture.sweep.counters.continuation_group_count > 0U &&
          !continuation_saddles.empty() && none_selected && none_retained,
      "E5 keeps one-root continuations in the authority but outside the partition basis");
}

void check_diagnostic(
    const ExactRelativeMorseBoruvkaResult& result,
    ExactRelativeMorseBoruvkaDecision expected_decision,
    const std::string& context) {
  check(
      result.decision == expected_decision &&
          result.diagnostic_outcomes_have_no_payload &&
          result.rounds.empty() && result.canonical_links.empty() &&
          result.root_choices.empty() && result.selected_links.empty() &&
          result.retained_saddle_indices.empty() &&
          result.final_component_representative_birth_indices.empty() &&
          !result.certified_relative_partition_sparsifier(),
      context + " is rejected atomically without a partial sparsifier");
}

void test_input_errors_and_each_budget_cap(const FixtureResult& fixture) {
  auto malformed_csr = fixture.input;
  ++malformed_csr.saddles.front().terminal_birth_reference_offset;
  check_diagnostic(
      build_exact_relative_morse_boruvka(malformed_csr, fixture.budget),
      ExactRelativeMorseBoruvkaDecision::
          no_sparsification_input_shape_rejected,
      "a noncanonical saddle CSR offset");

  auto out_of_range = fixture.input;
  out_of_range.terminal_birth_indices.front() = out_of_range.births.size();
  check_diagnostic(
      build_exact_relative_morse_boruvka(out_of_range, fixture.budget),
      ExactRelativeMorseBoruvkaDecision::
          no_sparsification_input_shape_rejected,
      "an out-of-range terminal birth");

  auto non_strict_terminal = fixture.input;
  const std::size_t terminal_birth =
      non_strict_terminal.terminal_birth_indices.front();
  non_strict_terminal.births[terminal_birth].squared_level =
      non_strict_terminal.saddles.front().squared_level;
  check_diagnostic(
      build_exact_relative_morse_boruvka(
          non_strict_terminal, fixture.budget),
      ExactRelativeMorseBoruvkaDecision::
          no_sparsification_input_shape_rejected,
      "a terminal that is not strictly earlier than its saddle");

  using BudgetMember =
      std::size_t ExactRelativeMorseBoruvkaBudget::*;
  const std::array<BudgetMember, 8> members{
      &ExactRelativeMorseBoruvkaBudget::maximum_birth_count,
      &ExactRelativeMorseBoruvkaBudget::maximum_saddle_count,
      &ExactRelativeMorseBoruvkaBudget::
          maximum_terminal_birth_reference_count,
      &ExactRelativeMorseBoruvkaBudget::maximum_canonical_link_count,
      &ExactRelativeMorseBoruvkaBudget::maximum_round_count,
      &ExactRelativeMorseBoruvkaBudget::maximum_root_choice_count,
      &ExactRelativeMorseBoruvkaBudget::maximum_selected_link_count,
      &ExactRelativeMorseBoruvkaBudget::maximum_retained_saddle_count};
  const std::array<const char*, 8> names{
      "birth", "saddle", "terminal reference", "canonical link", "round",
      "root choice", "selected link", "retained saddle"};
  for (std::size_t index = 0U; index < members.size(); ++index) {
    ExactRelativeMorseBoruvkaBudget insufficient = fixture.budget;
    check(
        insufficient.*members[index] > 0U,
        std::string{"q2 exercises the "} + names[index] + " budget cap");
    --(insufficient.*members[index]);
    check_diagnostic(
        build_exact_relative_morse_boruvka(fixture.input, insufficient),
        ExactRelativeMorseBoruvkaDecision::
            no_sparsification_preflight_budget_insufficient,
        std::string{"a one-below "} + names[index] + " budget");
  }
}

void test_threshold_bridge_counterexample() {
  ExactRelativeMorseBoruvkaInput input;
  input.point_count = 5U;
  input.order = 2U;
  for (std::size_t birth_index = 0U; birth_index < 5U; ++birth_index) {
    input.births.push_back(
        ExactRelativeMorseBoruvkaBirth{birth_index, level(0)});
  }
  input.saddles = {
      ExactRelativeMorseBoruvkaSaddle{0U, level(1), 0U, 2U},
      ExactRelativeMorseBoruvkaSaddle{1U, level(1), 2U, 2U},
      ExactRelativeMorseBoruvkaSaddle{2U, level(2), 4U, 2U},
      ExactRelativeMorseBoruvkaSaddle{3U, level(3), 6U, 3U}};
  input.terminal_birth_indices = {1U, 2U, 3U, 4U, 2U, 3U, 0U, 2U, 3U};
  const ExactRelativeMorseBoruvkaBudget budget = full_relative_budget(input);
  const ExactRelativeMorseBoruvkaResult result =
      build_exact_relative_morse_boruvka(input, budget);
  const ExactRelativeMorseBoruvkaVerification verification =
      verify_exact_relative_morse_boruvka(input, budget, result);
  check(
      complete_relative_facts(result) && verification_closes(verification),
      "the P0 threshold-bridge counterexample closes the corrected link transcript");

  const bool bridge_retained = std::find(
      result.retained_saddle_indices.begin(),
      result.retained_saddle_indices.end(),
      std::size_t{2}) != result.retained_saddle_indices.end();
  const bool bridge_selected = std::any_of(
      result.selected_links.begin(),
      result.selected_links.end(),
      [](const ExactRelativeMorseBoruvkaSelectedLink& selected) {
        return selected.saddle_index == 2U;
      });
  const ExactRelativeMorseBoruvkaRound& first_round =
      result.rounds.front();
  const auto first_selected = result.selected_links.begin() +
      static_cast<std::ptrdiff_t>(first_round.selected_link_offset);
  const auto first_selected_end = first_selected +
      static_cast<std::ptrdiff_t>(first_round.selected_link_count);
  const std::size_t first_round_high_spoke_count =
      static_cast<std::size_t>(std::count_if(
          first_selected,
          first_selected_end,
          [](const ExactRelativeMorseBoruvkaSelectedLink& selected) {
            return selected.saddle_index == 3U;
          }));
  const bool canonical_high_spoke_selected = std::any_of(
      first_selected,
      first_selected_end,
      [](const ExactRelativeMorseBoruvkaSelectedLink& selected) {
        return selected.link_index == 3U;
      });
  std::vector<std::size_t> full_saddles(input.saddles.size());
  std::iota(full_saddles.begin(), full_saddles.end(), std::size_t{0});
  const bool closed_level_two_preserved = same_active_partition(
      input,
      full_saddles,
      result.retained_saddle_indices,
      level(2),
      true);
  DisjointSet retained_at_two = partition_at(
      input, result.retained_saddle_indices, level(2), true);
  check(
      result.canonical_links.size() == 5U && result.rounds.size() == 2U &&
          result.selected_links.size() == 4U &&
          first_round_high_spoke_count == 1U &&
          canonical_high_spoke_selected && bridge_selected &&
          bridge_retained && closed_level_two_preserved &&
          retained_at_two.find(1U) == retained_at_two.find(4U) &&
          retained_at_two.find(0U) != retained_at_two.find(1U),
      "the permanent P0 fixture selects one high spoke, then retains the level-two bridge");
}

}  // namespace

int main() {
  const FixtureResult q2 =
      evaluate_fixture(q2_triangle_cloud(), 2U, 1U, "q2");
  test_q2_and_fresh_verifier(q2);
  test_mirror_simultaneous_saddles();
  test_shared_terminal_multiplicity();
  test_e5_continuations_are_not_edges();
  test_input_errors_and_each_budget_cap(q2);
  test_threshold_bridge_counterexample();

  if (failures != 0) {
    std::cerr << failures
              << " relative Morse--Boruvka test(s) failed\n";
    return 1;
  }
  std::cout << "all relative Morse--Boruvka tests passed\n";
  return 0;
}
