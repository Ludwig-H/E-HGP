#include "morsehgp3d/hierarchy/direct_morse_forest_reducer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;

constexpr std::uint64_t authority_id = UINT64_C(0x15C);
int failures = 0;

template <typename Source>
concept ForestReducerProjectable = requires(Source&& source) {
  project_exact_direct_morse_forest_reducer_batch(
      std::forward<Source>(source));
};

static_assert(ForestReducerProjectable<
              const ExactDirectSparseFacetDescentBatchExecutionResult&>);
static_assert(
    !ForestReducerProjectable<
        ExactDirectSparseFacetDescentBatchExecutionResult>);

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactPairSupportStreamBudget pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
  };
}

[[nodiscard]] ExactHigherSupportStreamBudget higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
  };
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget seed_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectSparseFacetDescentStepBudget step_budget() {
  return {
      ExactDirectSparsePositiveFacetProbeBudget{513U, 256U},
      ExactLbvhTopKBudget{
          4096U, 4096U, 4096U, 4096U, 256U, 16U, 16U},
      ExactDirectSparsePositiveFacetProbeBudget{513U, 256U},
  };
}

[[nodiscard]] ExactDirectMorseForestBudget forest_budget() {
  constexpr std::size_t capacity = 4096U;
  ExactDirectMorseForestBudget budget;
  budget.maximum_source_role_scan_count = capacity;
  budget.maximum_source_batch_scan_count = capacity;
  budget.maximum_source_family_scan_count = capacity;
  budget.maximum_source_arm_seed_scan_count = capacity;
  budget.maximum_birth_record_count = capacity;
  budget.maximum_arm_root_binding_count = capacity;
  budget.maximum_saddle_record_count = capacity;
  budget.maximum_atomic_group_count = capacity;
  budget.maximum_child_reference_count = capacity;
  budget.maximum_batch_record_count = capacity;
  budget.maximum_node_count = capacity;
  budget.maximum_final_root_count = capacity;
  budget.maximum_batch_distinct_arm_count = 256U;
  budget.maximum_logical_output_entry_count = capacity;
  budget.maximum_aggregate_closure_node_count = capacity;
  budget.maximum_aggregate_closure_step_call_count = capacity;
  budget.locator_budget = {
      256U,
      256U,
      2560U,
      256U,
      256U,
      256U,
      256U,
      256U,
      2560U,
      513U,
      513U,
  };
  budget.closure_budget = {
      256U,
      256U,
      256U,
      513U,
      step_budget(),
  };
  budget.quotient_budget = {256U, 256U, 256U, 256U, 1024U};
  return budget;
}

[[nodiscard]] ExactDirectMorseForestConfig forest_config() {
  ExactDirectMorseForestConfig config;
  config.locator_config.external_authority_id = authority_id;
  return config;
}

[[nodiscard]] ExactDirectMorseIndustrialPlanConfig plan_config(
    std::uint64_t maximum_batch_count) {
  ExactDirectMorseIndustrialPlanConfig config;
  config.policy =
      ExactDirectMorseIndustrialPolicy::massive_external_streaming;
  config.memory_model = {
      64U,
      16U,
      16U,
      8U,
      16U,
      16U,
      16U,
      4U,
      16U,
      8U,
      16U,
      2U,
  };
  config.chunk_budget = {
      1'000'000U,
      maximum_batch_count,
      4096U,
      4096U,
      4096U,
      4096U,
  };
  return config;
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchPlanBudget plan_budget() {
  return {
      16U,
      256U,
      256U,
      1024U,
      256U,
      1024U,
      1'000'000U,
      64U,
  };
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchExecutionBudget
execution_budget() {
  return {3U, 256U, 256U, 2560U, 256U};
}

struct Scenario {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedJournalResult seed_journal;
};

[[nodiscard]] Scenario tetrahedron() {
  const std::array<CertifiedPoint3, 4U> points{
      point(1.0, 1.0, 1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, 1.0, -1.0),
      point(1.0, -1.0, -1.0),
  };
  CanonicalPointCloud cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSupportTerminalBudget terminal_budget{
      pair_budget(), higher_budget()};
  const auto pair = build_exact_pair_support_stream(
      index, cloud, 1U, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, 1U, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, 1U, terminal_budget, pair, higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  auto seed_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, seed_budget());
  return {
      std::move(cloud),
      std::move(index),
      std::move(facade),
      std::move(event_journal),
      std::move(seed_journal),
  };
}

[[nodiscard]] Scenario gabriel_ac_to_de() {
  // Input labels A, B, C, D, E canonicalize to D=0, A=1, B=2, C=3, E=4.
  const std::array<CertifiedPoint3, 5U> points{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0),
  };
  CanonicalPointCloud cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSupportTerminalBudget terminal_budget{
      pair_budget(), higher_budget()};
  const auto pair = build_exact_pair_support_stream(
      index, cloud, 2U, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, 2U, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, 2U, terminal_budget, pair, higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  auto seed_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, seed_budget());
  return {
      std::move(cloud),
      std::move(index),
      std::move(facade),
      std::move(event_journal),
      std::move(seed_journal),
  };
}

[[nodiscard]] ExactDirectMorseForestJournalResult run_stream(
    const Scenario& scenario,
    std::uint64_t maximum_batch_count,
    bool exercise_atomic_rejection) {
  const auto industrial_config = plan_config(maximum_batch_count);
  const auto observed_plan =
      build_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          seed_budget(),
          scenario.seed_journal,
          industrial_config,
          plan_budget());
  check(
      observed_plan.complete_architecture_plan(),
      "the reducer fixture has one complete 14C plan");

  ExactDirectMorseForestReducer reducer(
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      forest_budget(),
      forest_config());
  ExactDirectSparseFacetDescentAnchoredBatchExecutor executor(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      industrial_config,
      plan_budget(),
      observed_plan,
      reducer.strict_locator());
  bool terminal_substitution_exercised = false;

  while (!executor.complete()) {
    const std::size_t batch_index = executor.next_source_batch_index();
    const ExactDirectSparseFacetWitness witness{
        authority_id,
        (static_cast<std::uint64_t>(batch_index) + 1U) * 3U};
    const auto delta = executor.prepare_next(
        witness, execution_budget(), forest_budget().closure_budget);
    check(
        delta.complete_architecture_execution(),
        "14D produces one complete compact reducer delta");
    const auto replay = executor.commit_prepared(
        witness,
        execution_budget(),
        forest_budget().closure_budget,
        delta);
    check(
        replay.result_certified && replay.session_advanced,
        "the anchored 14D cursor advances before the reducer mutates its locator");

    auto projected =
        project_exact_direct_morse_forest_reducer_batch(delta);
    if (exercise_atomic_rejection && batch_index == 0U) {
      auto reordered = projected;
      ++reordered.source_batch_index;
      const auto stamp_before = reducer.strict_locator().snapshot_stamp();
      const auto rejected = reducer.fold(reordered);
      check(
          rejected.certified_atomic_rejection() &&
              reducer.next_source_batch_index() == 0U &&
              reducer.strict_locator().snapshot_stamp() == stamp_before,
          "a reordered batch is rejected without locator or reducer advance");
    }
    if (exercise_atomic_rejection &&
        !terminal_substitution_exercised) {
      std::vector<ExactDirectSparseFacetDescentBatchResolvedKey>
          substituted_keys(
              projected.resolved_keys.begin(),
              projected.resolved_keys.end());
      for (std::size_t left = 0U;
           left < substituted_keys.size() &&
           !terminal_substitution_exercised;
           ++left) {
        for (std::size_t right = left + 1U;
             right < substituted_keys.size();
             ++right) {
          if (substituted_keys[left].resolved_component_handle ==
              substituted_keys[right].resolved_component_handle) {
            continue;
          }
          substituted_keys[left].resolved_component_handle =
              substituted_keys[right].resolved_component_handle;
          substituted_keys[left].resolved_binding_witness =
              substituted_keys[right].resolved_binding_witness;
          auto substituted = projected;
          substituted.resolved_keys = substituted_keys;
          const auto stamp_before =
              reducer.strict_locator().snapshot_stamp();
          const auto rejected = reducer.fold(substituted);
          check(
              rejected.certified_atomic_rejection() &&
                  reducer.next_source_batch_index() == batch_index &&
                  reducer.strict_locator().snapshot_stamp() ==
                      stamp_before,
              "a substituted terminal carrier is rejected atomically");
          terminal_substitution_exercised = true;
          break;
        }
      }
    }
    const auto committed = reducer.fold(projected);
    check(
        committed.certified_committed_batch() &&
            reducer.next_source_batch_index() == batch_index + 1U,
        "one projected batch commits locator then scientific state");
  }
  check(
      reducer.complete(),
      "the reducer consumes every exact source batch once");
  if (exercise_atomic_rejection) {
    check(
        terminal_substitution_exercised,
        "the fixture exercised a distinct terminal-carrier substitution");
  }
  return reducer.finish();
}

void test_final_root_budget_is_rejected_at_open() {
  const Scenario scenario = tetrahedron();
  auto insufficient = forest_budget();
  insufficient.maximum_final_root_count = 0U;
  bool rejected = false;
  try {
    ExactDirectMorseForestReducer reducer(
        scenario.cloud,
        scenario.facade,
        scenario.event_journal,
        seed_budget(),
        scenario.seed_journal,
        insufficient,
        forest_config());
    static_cast<void>(reducer);
  } catch (const std::invalid_argument&) {
    rejected = true;
  }
  check(
      rejected,
      "an insufficient derivable final-root budget is rejected at open");
}

void test_incremental_identity_and_chunk_independence() {
  const Scenario scenario = tetrahedron();
  const auto resident = build_exact_direct_morse_forest_journal(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      forest_budget(),
      forest_config());
  const auto one_batch_chunks = run_stream(scenario, 1U, true);
  const auto two_batch_chunks = run_stream(scenario, 2U, false);

  check(
      resident.certified_conditional_h0_candidate() &&
          one_batch_chunks.certified_conditional_h0_candidate() &&
          two_batch_chunks.certified_conditional_h0_candidate(),
      "resident and streaming reductions certify the same conditional scope");
  check(
      one_batch_chunks == resident && two_batch_chunks == resident,
      "streaming output is recursively identical to resident output and independent of chunk cap");
}

void test_gabriel_arm_may_descend_to_a_different_terminal_key() {
  const Scenario scenario = gabriel_ac_to_de();
  const auto resident = build_exact_direct_morse_forest_journal(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      forest_budget(),
      forest_config());
  const auto streaming = run_stream(scenario, 2U, false);
  check(
      streaming == resident &&
          streaming.certified_conditional_h0_candidate(),
      "the permanent Gabriel AC-to-DE descent stays identical through the streaming reducer");
}

}  // namespace

int main() {
  test_final_root_budget_is_rejected_at_open();
  test_incremental_identity_and_chunk_independence();
  test_gabriel_arm_may_descend_to_a_different_terminal_key();
  if (failures != 0) {
    std::cerr << failures << " direct Morse forest reducer test(s) failed\n";
    return 1;
  }
  std::cout << "direct Morse forest reducer tests passed\n";
  return 0;
}
