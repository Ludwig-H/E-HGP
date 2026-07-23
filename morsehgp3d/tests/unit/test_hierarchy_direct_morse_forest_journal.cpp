#include "morsehgp3d/hierarchy/direct_morse_forest_journal.hpp"
#include "morsehgp3d/hierarchy/direct_morse_industrial_plan.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

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

[[nodiscard]] std::optional<ExactDirectSparseComponentHandle>
component_root_before_group(
    const ExactDirectMorseForestJournalResult& result,
    ExactDirectSparseComponentHandle component_handle,
    std::size_t stopped_atomic_group_index) {
  if (component_handle >= result.birth_records.size() ||
      stopped_atomic_group_index > result.atomic_groups.size()) {
    return std::nullopt;
  }
  std::vector<ExactDirectSparseComponentHandle> parents(
      result.birth_records.size());
  for (std::size_t index = 0U; index < parents.size(); ++index) {
    parents[index] = index;
  }
  const auto root = [&parents](
                        ExactDirectSparseComponentHandle handle) {
    while (parents[handle] != handle) {
      handle = parents[handle];
    }
    return handle;
  };
  for (std::size_t group_index = 0U;
       group_index < stopped_atomic_group_index;
       ++group_index) {
    const auto& group = result.atomic_groups[group_index];
    std::optional<ExactDirectSparseComponentHandle> first;
    for (std::size_t saddle_local = 0U;
         saddle_local < group.saddle_record_count;
         ++saddle_local) {
      const auto& saddle = result.saddle_records[
          group.saddle_record_offset + saddle_local];
      for (std::size_t binding_local = 0U;
           binding_local < saddle.arm_binding_count;
           ++binding_local) {
        const auto handle =
            result
                .arm_root_bindings[
                    saddle.arm_binding_offset + binding_local]
                .frozen_carrier_component_handle;
        if (handle >= parents.size()) {
          return std::nullopt;
        }
        if (!first.has_value()) {
          first = handle;
          continue;
        }
        const auto left_root = root(*first);
        const auto right_root = root(handle);
        parents[std::max(left_root, right_root)] =
            std::min(left_root, right_root);
      }
    }
  }
  return root(component_handle);
}

[[nodiscard]] bool two_point_key_is(
    const ExactDirectSparseFacetKey& key,
    PointId first,
    PointId second) {
  return key.point_count == 2U && key.point_ids[0U] == first &&
         key.point_ids[1U] == second;
}

[[nodiscard]] std::vector<PointId> closed_event_point_ids(
    const ExactDirectSupportEvent& event) {
  std::vector<PointId> point_ids = event.interior_ids;
  for (std::size_t local = 0U; local < event.support_size; ++local) {
    point_ids.push_back(event.support_ids[local]);
  }
  std::sort(point_ids.begin(), point_ids.end());
  return point_ids;
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
      result.certified_conditional_h0_candidate() && triangle != nullptr &&
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
      result.certified_conditional_h0_candidate() && mirror != nullptr &&
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
      result.certified_conditional_h0_candidate() &&
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

void test_true_gabriel_counterexample_descends_ac_to_de_end_to_end() {
  // Input labels A, B, C, D, E canonicalize to D=0, A=1, B=2, C=3, E=4.
  const std::array<CertifiedPoint3, 5U> points{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0),
  };
  const Scenario scenario = make_scenario(canonical_cloud(points));
  const auto result = build_forest(scenario);

  std::optional<ExactDirectSparseComponentHandle> de_carrier;
  for (const auto& birth : result.birth_records) {
    if (birth.order == 2U &&
        two_point_key_is(birth.facet_key, 0U, 4U)) {
      de_carrier = birth.component_handle;
      break;
    }
  }

  bool later_abc_ac_arm_reaches_de = false;
  const auto* later_continuation =
      group_at(result, 2U, level(83886, 3563));
  const auto de_root_before_later =
      de_carrier.has_value() && later_continuation != nullptr
          ? component_root_before_group(
                result,
                *de_carrier,
                later_continuation->atomic_group_index)
          : std::nullopt;
  for (const auto& binding : result.arm_root_bindings) {
    if (!two_point_key_is(binding.strict_arm_key, 1U, 3U) ||
        binding.source_family_index >=
            scenario.seed_journal.families.size()) {
      continue;
    }
    const auto& family =
        scenario.seed_journal.families[binding.source_family_index];
    if (family.source_event_index >= scenario.facade.events.size()) {
      continue;
    }
    const auto closed_ids =
        closed_event_point_ids(
            scenario.facade.events[family.source_event_index]);
    const auto saddle = std::find_if(
        result.saddle_records.begin(),
        result.saddle_records.end(),
        [&binding](const ExactDirectMorseForestSaddleRecord& candidate) {
          return candidate.source_family_index ==
                 binding.source_family_index;
        });
    if (closed_ids == std::vector<PointId>{1U, 2U, 3U} &&
        de_root_before_later.has_value() &&
        later_continuation != nullptr &&
        saddle != result.saddle_records.end() &&
        saddle->atomic_group_index ==
            later_continuation->atomic_group_index &&
        binding.binding_index >= saddle->arm_binding_offset &&
        binding.binding_index <
            saddle->arm_binding_offset +
                saddle->arm_binding_count &&
        binding.frozen_carrier_component_handle ==
            *de_root_before_later &&
        binding.prior_reduced_root_node_id ==
            std::optional<ExactDirectMorseForestNodeId>{
                later_continuation->resulting_root_node_id}) {
      later_abc_ac_arm_reaches_de = true;
      break;
    }
  }

  check(
      ExactDirectMorseForestJournalResult::refinement_status ==
              "conditional_h0_candidate" &&
          result.certified_conditional_h0_candidate() &&
          result.certified_conditional_exact_h0(),
      "the permanent three-dimensional Gabriel counterexample produces an explicitly conditional H0 candidate while the schema-v2 compatibility predicate remains available");
  check(
      de_carrier.has_value(),
      "the permanent three-dimensional Gabriel counterexample contains the earlier DE carrier");
  check(
      later_abc_ac_arm_reaches_de,
      "the later ABC event binds its strict AC arm to the frozen component containing the earlier DE carrier");
  check(
      later_continuation != nullptr,
      "the later ABC level contains one atomic forest group");
  check(
      later_continuation != nullptr &&
          later_continuation->kind ==
              ExactDirectMorseForestAtomicGroupKind::continuation &&
          later_continuation->prior_reduced_root_count == 1U &&
          later_continuation->child_count == 0U &&
          !later_continuation->created_node_id.has_value(),
      "the later ABC event stays a q_R=1 continuation without a new reduced node");

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
      "the true Gabriel counterexample forest composition is freshly replayed end to end");
}

}  // namespace

int main() {
  test_acute_triangle_creates_one_reduced_birth_from_latent_edges();
  test_mirror_closes_shared_latent_carrier_before_q_r();
  test_e5_has_two_births_one_continuation_and_one_multifusion();
  test_true_gabriel_counterexample_descends_ac_to_de_end_to_end();

  if (failures != 0) {
    std::cerr << failures
              << " direct Morse forest journal test(s) failed\n";
    return 1;
  }
  return 0;
}
