#include "morsehgp3d/hierarchy/direct_morse_forest_journal.hpp"
#include "morsehgp3d/hierarchy/direct_morse_industrial_plan.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <utility>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;

constexpr std::uint64_t authority_id = UINT64_C(0x10F0);
int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(
    double x,
    double y,
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

[[nodiscard]] ExactPairSupportStreamBudget source_pair_budget() {
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

[[nodiscard]] ExactHigherSupportStreamBudget source_higher_budget() {
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

[[nodiscard]] ExactDirectSaddleArmSeedBudget source_seed_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectSparseFacetDescentStepBudget step_budget() {
  return {
      ExactDirectSparsePositiveFacetProbeBudget{129U, 64U},
      ExactLbvhTopKBudget{
          256U, 256U, 512U, 256U, 64U, 16U, 16U},
      ExactDirectSparsePositiveFacetProbeBudget{129U, 64U},
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
  budget.maximum_batch_distinct_arm_count = 16U;
  budget.maximum_logical_output_entry_count = capacity;
  budget.maximum_aggregate_closure_node_count = capacity;
  budget.maximum_aggregate_closure_step_call_count = capacity;
  budget.locator_budget = {
      64U,
      64U,
      640U,
      64U,
      64U,
      64U,
      64U,
      64U,
      640U,
      129U,
      129U,
  };
  budget.closure_budget = {
      16U,
      64U,
      64U,
      129U,
      step_budget(),
  };
  budget.quotient_budget = {64U, 64U, 64U, 64U, 256U};
  return budget;
}

[[nodiscard]] ExactDirectMorseForestConfig forest_config() {
  ExactDirectMorseForestConfig config;
  config.locator_config.external_authority_id = authority_id;
  return config;
}

[[nodiscard]] ExactDirectMorseIndustrialPlanConfig plan_config(
    ExactDirectMorseIndustrialPolicy policy,
    std::uint64_t maximum_batch_count) {
  ExactDirectMorseIndustrialPlanConfig config;
  config.policy = policy;
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

struct Scenario {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedJournalResult seed_journal;
};

[[nodiscard]] Scenario make_scenario(CanonicalPointCloud cloud) {
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSupportTerminalBudget terminal_budget{
      source_pair_budget(), source_higher_budget()};
  const auto pair = build_exact_pair_support_stream(
      index, cloud, 2U, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, 2U, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, 2U, terminal_budget, pair, higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  auto seed_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, source_seed_budget());
  return {
      std::move(cloud),
      std::move(index),
      std::move(facade),
      std::move(event_journal),
      std::move(seed_journal),
  };
}

[[nodiscard]] ExactDirectMorseForestJournalResult build_forest(
    const Scenario& scenario) {
  return build_exact_direct_morse_forest_journal(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      source_seed_budget(),
      scenario.seed_journal,
      forest_budget(),
      forest_config(),
      LbvhTraversalOrder::near_first);
}

[[nodiscard]] const ExactDirectMorseForestAtomicGroup* group_at(
    const ExactDirectMorseForestJournalResult& result,
    std::size_t order,
    const ExactLevel& squared_level) {
  const auto batch = std::find_if(
      result.batches.begin(),
      result.batches.end(),
      [order, &squared_level](const ExactDirectMorseForestBatch& candidate) {
        return candidate.order == order &&
               candidate.squared_level == squared_level;
      });
  if (batch == result.batches.end() || batch->atomic_group_count != 1U) {
    return nullptr;
  }
  return &result.atomic_groups[batch->atomic_group_offset];
}

void test_acute_triangle_creates_one_reduced_birth_from_latent_edges() {
  const std::array<CertifiedPoint3, 3U> points{
      point(0.0, 0.0),
      point(4.0, 0.0),
      point(1.0, 3.0),
  };
  const Scenario scenario = make_scenario(canonical_cloud(points));
  const auto result = build_forest(scenario);
  const auto* triangle = group_at(result, 2U, level(5));

  check(
      result.certified_conditional_exact_h0() && triangle != nullptr &&
          triangle->kind ==
              ExactDirectMorseForestAtomicGroupKind::reduced_birth &&
          triangle->saddle_record_count == 1U &&
          triangle->frozen_carrier_count == 3U &&
          triangle->latent_carrier_count == 3U &&
          triangle->prior_reduced_root_count == 0U &&
          triangle->child_count == 0U &&
          triangle->created_node_id.has_value(),
      "an acute K=2 triangle turns three latent edge minima into one q_R=0 reduced birth");
}

void test_mirror_closes_shared_latent_carrier_before_q_r() {
  const std::array<CertifiedPoint3, 4U> points{
      point(-2.0, 0.0),
      point(0.0, -3.0),
      point(0.0, 3.0),
      point(2.0, 0.0),
  };
  const Scenario scenario = make_scenario(canonical_cloud(points));
  const auto result = build_forest(scenario);
  const auto* mirror = group_at(result, 2U, level(169, 36));

  check(
      result.certified_conditional_exact_h0() && mirror != nullptr &&
          mirror->kind ==
              ExactDirectMorseForestAtomicGroupKind::reduced_birth &&
          mirror->saddle_record_count == 2U &&
          mirror->frozen_carrier_count == 5U &&
          mirror->latent_carrier_count == 5U &&
          mirror->prior_reduced_root_count == 0U &&
          result.counters.duplicate_strict_arm_reference_count >= 1U,
      "the simultaneous mirror saddles close through five carriers, including their shared latent edge, before q_R=0");

  const auto verification = verify_exact_direct_morse_forest_journal(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      source_seed_budget(),
      scenario.seed_journal,
      forest_budget(),
      forest_config(),
      LbvhTraversalOrder::near_first,
      result);
  check(
      verification.expected_journal_freshly_reconstructed &&
          verification.observed_recursively_equal &&
          verification.result_certified,
      "the fresh verifier reconstructs the mirror journal exactly");

  auto falsified = result;
  if (mirror != nullptr) {
    const std::size_t group_index = mirror->atomic_group_index;
    --falsified.atomic_groups[group_index].latent_carrier_count;
  }
  const auto rejected = verify_exact_direct_morse_forest_journal(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      source_seed_budget(),
      scenario.seed_journal,
      forest_budget(),
      forest_config(),
      LbvhTraversalOrder::near_first,
      falsified);
  check(
      rejected.expected_journal_freshly_reconstructed &&
          !rejected.observed_recursively_equal &&
          !rejected.result_certified,
      "one falsified latent-carrier count is rejected by fresh replay");
}

void test_e5_has_two_births_one_continuation_and_one_multifusion() {
  const std::array<CertifiedPoint3, 5U> points{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0),
  };
  const Scenario scenario = make_scenario(canonical_cloud(points));
  const auto result = build_forest(scenario);

  std::array<std::size_t, 3U> order_two_kind_counts{};
  const ExactDirectMorseForestAtomicGroup* multifusion = nullptr;
  for (const auto& group : result.atomic_groups) {
    if (result.batches[group.batch_index].order != 2U) {
      continue;
    }
    switch (group.kind) {
      case ExactDirectMorseForestAtomicGroupKind::reduced_birth:
        ++order_two_kind_counts[0];
        break;
      case ExactDirectMorseForestAtomicGroupKind::continuation:
        ++order_two_kind_counts[1];
        break;
      case ExactDirectMorseForestAtomicGroupKind::multifusion:
        ++order_two_kind_counts[2];
        multifusion = &group;
        break;
    }
  }

  check(
      result.certified_conditional_exact_h0() &&
          order_two_kind_counts == std::array<std::size_t, 3U>{2U, 1U, 1U} &&
          multifusion != nullptr &&
          multifusion->prior_reduced_root_count == 2U &&
          multifusion->child_count == 2U &&
          multifusion->created_node_id.has_value(),
      "E5 keeps exactly two q_R=0 births, one q_R=1 continuation and one binary q_R=2 multifusion");

  const auto resident = build_exact_direct_morse_industrial_plan(
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      source_seed_budget(),
      scenario.seed_journal,
      plan_config(
          ExactDirectMorseIndustrialPolicy::interactive_resident_50k,
          4096U));
  check(
      resident.complete_architecture_plan() &&
          resident.selected_policy ==
              ExactDirectMorseIndustrialPolicy::interactive_resident_50k &&
          resident.chunks.size() == 1U,
      "the E5 resident policy keeps every exact batch in one architecture-only chunk");

  const auto streaming = build_exact_direct_morse_industrial_plan(
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      source_seed_budget(),
      scenario.seed_journal,
      plan_config(
          ExactDirectMorseIndustrialPolicy::massive_external_streaming,
          2U));
  check(
      streaming.complete_architecture_plan() &&
          streaming.chunks.size() > 1U &&
          std::all_of(
              streaming.chunks.begin(),
              streaming.chunks.end(),
              [](const ExactDirectMorseIndustrialChunk& chunk) {
                return chunk.checkpoint_boundary_after_chunk &&
                       chunk.external_run_boundary_after_chunk;
              }),
      "the E5 streaming policy cuts only between complete exact batches");

  auto malformed_plan = streaming;
  if (!malformed_plan.chunks.empty()) {
    ++malformed_plan.chunks.front().chunk_index;
  }
  check(
      !malformed_plan.complete_architecture_plan(),
      "the architecture plan predicate rejects one malformed chunk identity");
}

}  // namespace

int main() {
  test_acute_triangle_creates_one_reduced_birth_from_latent_edges();
  test_mirror_closes_shared_latent_carrier_before_q_r();
  test_e5_has_two_births_one_continuation_and_one_multifusion();

  if (failures != 0) {
    std::cerr << failures
              << " direct Morse forest journal test(s) failed\n";
    return 1;
  }
  return 0;
}
