#include "morsehgp3d/hierarchy/direct_k1_forest_journal.hpp"

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

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactDirectK1ForestBudget;
using morsehgp3d::hierarchy::ExactDirectK1ForestDecision;
using morsehgp3d::hierarchy::ExactDirectK1ForestJournalResult;
using morsehgp3d::hierarchy::ExactDirectMorseEventJournalResult;
using morsehgp3d::hierarchy::ExactDirectSaddleArmSeedBudget;
using morsehgp3d::hierarchy::ExactDirectSaddleArmSeedJournalResult;
using morsehgp3d::hierarchy::ExactDirectSupportTerminalBudget;
using morsehgp3d::hierarchy::ExactDirectSupportTerminalFacade;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
using morsehgp3d::hierarchy::ExactPairSupportStreamBudget;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

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

[[nodiscard]] ExactLevel level(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] ExactPairSupportStreamBudget unlimited_pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactPairSupportStreamBudget{
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactHigherSupportStreamBudget{
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget unlimited_seed_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactDirectSaddleArmSeedBudget{
      maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectK1ForestBudget unlimited_k1_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactDirectK1ForestBudget{
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

struct DirectPipeline {
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult journal;
  ExactDirectSaddleArmSeedBudget seed_budget;
  ExactDirectSaddleArmSeedJournalResult seed_journal;
  ExactDirectK1ForestBudget k1_budget;
  ExactDirectK1ForestJournalResult forest;
};

[[nodiscard]] DirectPipeline direct_pipeline(
    const CanonicalPointCloud& cloud) {
  const ExactDirectSupportTerminalBudget terminal_budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto pair =
      morsehgp3d::hierarchy::build_exact_pair_support_stream(
          index, cloud, 10U, terminal_budget.pair);
  const auto higher =
      morsehgp3d::hierarchy::build_exact_higher_support_stream(
          index, cloud, 10U, terminal_budget.higher);
  ExactDirectSupportTerminalFacade facade =
      morsehgp3d::hierarchy::build_exact_direct_support_terminal_facade(
          index, cloud, 10U, terminal_budget, pair, higher);
  ExactDirectMorseEventJournalResult journal =
      morsehgp3d::hierarchy::build_exact_direct_morse_event_journal(
          cloud, facade);
  const ExactDirectSaddleArmSeedBudget seed_budget =
      unlimited_seed_budget();
  ExactDirectSaddleArmSeedJournalResult seed_journal =
      morsehgp3d::hierarchy::
          build_exact_direct_saddle_arm_seed_journal(
              cloud, facade, journal, seed_budget);
  const ExactDirectK1ForestBudget k1_budget = unlimited_k1_budget();
  ExactDirectK1ForestJournalResult forest =
      morsehgp3d::hierarchy::build_exact_direct_k1_forest_journal(
          cloud,
          facade,
          journal,
          seed_budget,
          seed_journal,
          k1_budget);
  return DirectPipeline{
      std::move(facade),
      std::move(journal),
      seed_budget,
      std::move(seed_journal),
      k1_budget,
      std::move(forest)};
}

void check_streaming_replay(
    const CanonicalPointCloud& cloud,
    const DirectPipeline& pipeline,
    const std::string& fixture) {
  const auto journal_replay =
      morsehgp3d::hierarchy::
          verify_exact_direct_morse_event_journal_streaming(
              cloud, pipeline.facade, pipeline.journal);
  const auto seed_replay =
      morsehgp3d::hierarchy::
          verify_exact_direct_saddle_arm_seed_journal_streaming(
              cloud,
              pipeline.facade,
              pipeline.journal,
              pipeline.seed_budget,
              pipeline.seed_journal);
  const auto forest_replay =
      morsehgp3d::hierarchy::
          verify_exact_direct_k1_forest_journal_streaming(
              cloud,
              pipeline.facade,
              pipeline.journal,
              pipeline.seed_budget,
              pipeline.seed_journal,
              pipeline.k1_budget,
              pipeline.forest);
  check(
      journal_replay.result_certified &&
          journal_replay.constant_auxiliary_record_storage_certified &&
          seed_replay.result_certified &&
          seed_replay.constant_auxiliary_record_storage_certified &&
          forest_replay.source_seed_journal_certified &&
          forest_replay.requirements_certified &&
          forest_replay.arm_root_bindings_certified &&
          forest_replay.saddle_records_certified &&
          forest_replay.atomic_groups_certified &&
          forest_replay.child_references_certified &&
          forest_replay.batches_certified &&
          forest_replay.result_facts_certified &&
          forest_replay.counters_certified &&
          forest_replay.decision_and_scope_certified &&
          forest_replay.no_second_persistent_output_arena_certified &&
          forest_replay.fresh_streaming_replay_certified &&
          forest_replay.result_certified,
      fixture + " replays Phase 10.1, 10.2 and the direct K1 forest in streaming mode");
}

void test_two_points_freeze_two_roots() {
  const std::array<CertifiedPoint3, 2U> points{
      point(-1.0), point(1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const DirectPipeline pipeline = direct_pipeline(cloud);
  const ExactDirectK1ForestJournalResult& forest = pipeline.forest;

  check(
      forest.certified_order_one_forest() &&
          forest.arm_root_bindings.size() == 2U &&
          forest.saddle_records.size() == 1U &&
          forest.atomic_groups.size() == 1U &&
          forest.child_node_ids ==
              std::vector<morsehgp3d::hierarchy::ExactDirectK1NodeId>(
                  {0U, 1U}) &&
          forest.batches.size() == 1U && forest.node_count == 3U &&
          forest.root_node_id == 2U,
      "the two-point chain creates one binary root from two singleton leaves");
  if (forest.arm_root_bindings.size() < 2U ||
      forest.saddle_records.empty() || forest.atomic_groups.empty() ||
      forest.batches.empty()) {
    check_streaming_replay(cloud, pipeline, "two-point K1 fixture");
    return;
  }
  check(
      forest.arm_root_bindings[0].singleton_point_id == PointId{1U} &&
          forest.arm_root_bindings[0].pre_batch_root_node_id == 1U &&
          forest.arm_root_bindings[1].singleton_point_id == PointId{0U} &&
          forest.arm_root_bindings[1].pre_batch_root_node_id == 0U &&
          forest.saddle_records[0].distinct_pre_batch_root_count == 2U,
      "each deleted endpoint is bound to the opposite singleton in the strict snapshot");
  check(
      forest.batches[0].squared_level == level(1) &&
          forest.batches[0].strict_root_count == 2U &&
          forest.batches[0].closed_root_count == 1U &&
          forest.batches[0].roots_resolved_from_frozen_pre_batch_snapshot &&
          forest.batches[0].quotient_committed_atomically &&
          forest.atomic_groups[0].child_count == 2U &&
          forest.atomic_groups[0].created_node_id ==
              std::optional<morsehgp3d::hierarchy::ExactDirectK1NodeId>{2U} &&
          forest.atomic_groups[0].resulting_root_node_id == 2U,
      "the level-one pair closes only after its two strict roots are resolved");
  check_streaming_replay(cloud, pipeline, "two-point K1 fixture");
}

void test_e3_equal_level_is_one_ternary_multifusion() {
  const std::array<CertifiedPoint3, 3U> points{
      point(-2.0), point(0.0), point(2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const DirectPipeline pipeline = direct_pipeline(cloud);
  const ExactDirectK1ForestJournalResult& forest = pipeline.forest;

  std::size_t middle_binding_count = 0U;
  for (const auto& binding : forest.arm_root_bindings) {
    if (binding.singleton_point_id == PointId{1U} &&
        binding.pre_batch_root_node_id == 1U) {
      ++middle_binding_count;
    }
  }
  check(
      forest.certified_order_one_forest() &&
          forest.arm_root_bindings.size() == 4U &&
          forest.saddle_records.size() == 2U &&
          forest.atomic_groups.size() == 1U &&
          forest.child_node_ids ==
              std::vector<morsehgp3d::hierarchy::ExactDirectK1NodeId>(
                  {0U, 1U, 2U}) &&
          forest.batches.size() == 1U && forest.node_count == 4U &&
          forest.root_node_id == 3U && middle_binding_count == 2U,
      "E3 keeps both middle-arm provenances and creates no binary intermediate node");
  if (forest.atomic_groups.empty() || forest.batches.empty()) {
    check_streaming_replay(cloud, pipeline, "E3 equal-level fixture");
    return;
  }
  check(
      forest.batches[0].squared_level == level(1) &&
          forest.batches[0].saddle_record_count == 2U &&
          forest.batches[0].strict_root_count == 3U &&
          forest.batches[0].closed_root_count == 1U &&
          forest.atomic_groups[0].saddle_record_count == 2U &&
          forest.atomic_groups[0].child_count == 3U &&
          forest.atomic_groups[0].created_node_id ==
              std::optional<morsehgp3d::hierarchy::ExactDirectK1NodeId>{3U} &&
          forest.counters.multifusion_count == 1U &&
          forest.counters.created_node_count == 1U &&
          forest.counters.maximum_batch_saddle_count == 2U &&
          forest.counters.maximum_merge_arity == 3U,
      "the two equal saddles contract the frozen three-root quotient as one ternary multifusion");

  auto mutated = forest;
  const auto middle = std::find_if(
      mutated.arm_root_bindings.begin(),
      mutated.arm_root_bindings.end(),
      [](const auto& binding) {
        return binding.singleton_point_id == PointId{1U};
      });
  check(
      middle != mutated.arm_root_bindings.end(),
      "E3 exposes a middle-arm binding to falsify");
  if (middle != mutated.arm_root_bindings.end()) {
    middle->pre_batch_root_node_id = forest.root_node_id;
    const auto verification =
        morsehgp3d::hierarchy::
            verify_exact_direct_k1_forest_journal_streaming(
                cloud,
                pipeline.facade,
                pipeline.journal,
                pipeline.seed_budget,
                pipeline.seed_journal,
                pipeline.k1_budget,
                mutated);
    check(
        !verification.arm_root_bindings_certified &&
            !verification.fresh_streaming_replay_certified &&
            !verification.result_certified,
        "streaming replay rejects a same-batch parent substituted for a strict singleton root");
  }
  check_streaming_replay(cloud, pipeline, "E3 equal-level fixture");
}

void test_q1_continuation_preserves_the_prior_root() {
  const std::array<CertifiedPoint3, 3U> points{
      point(-1.0, 0.0), point(0.0, 1.5), point(1.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const DirectPipeline pipeline = direct_pipeline(cloud);
  const ExactDirectK1ForestJournalResult& forest = pipeline.forest;

  check(
      forest.certified_order_one_forest() && forest.batches.size() == 2U &&
          forest.saddle_records.size() == 3U &&
          forest.arm_root_bindings.size() == 6U &&
          forest.atomic_groups.size() == 2U &&
          forest.child_node_ids ==
              std::vector<morsehgp3d::hierarchy::ExactDirectK1NodeId>(
                  {0U, 1U, 2U}) &&
          forest.node_count == 4U && forest.root_node_id == 3U,
      "the acute three-point fixture retains one merge followed by one continuation");
  if (forest.batches.size() < 2U || forest.atomic_groups.size() < 2U) {
    check_streaming_replay(cloud, pipeline, "q=1 continuation fixture");
    return;
  }
  check(
      forest.batches[0].squared_level == level(13, 16) &&
          forest.batches[0].strict_root_count == 3U &&
          forest.batches[0].closed_root_count == 1U &&
          forest.batches[1].squared_level == level(1) &&
          forest.batches[1].strict_root_count == 1U &&
          forest.batches[1].closed_root_count == 1U,
      "the q=1 saddle observes the root created only by the earlier strict level");

  const auto& continuation = forest.atomic_groups[1];
  if (continuation.saddle_record_offset >= forest.saddle_records.size()) {
    check(false, "the q=1 group references an existing saddle record");
    check_streaming_replay(cloud, pipeline, "q=1 continuation fixture");
    return;
  }
  const auto& continuation_saddle =
      forest.saddle_records[continuation.saddle_record_offset];
  if (continuation_saddle.binding_offset > forest.arm_root_bindings.size() ||
      continuation_saddle.binding_count >
          forest.arm_root_bindings.size() -
              continuation_saddle.binding_offset) {
    check(false, "the q=1 saddle references an existing binding slice");
    check_streaming_replay(cloud, pipeline, "q=1 continuation fixture");
    return;
  }
  const auto continuation_bindings = std::span{
      forest.arm_root_bindings.data() +
          static_cast<std::ptrdiff_t>(
              continuation_saddle.binding_offset),
      continuation_saddle.binding_count};
  check(
      continuation.saddle_record_count == 1U &&
          continuation.child_count == 0U &&
          !continuation.created_node_id.has_value() &&
          continuation.resulting_root_node_id == 3U &&
          continuation_saddle.distinct_pre_batch_root_count == 1U &&
          continuation_bindings.size() == 2U &&
          continuation_bindings[0].pre_batch_root_node_id == 3U &&
          continuation_bindings[1].pre_batch_root_node_id == 3U &&
          forest.counters.continuation_group_count == 1U &&
          forest.counters.created_node_count == 1U,
      "q=1 preserves two arm bindings but emits neither child nor new node");

  auto mutated = forest;
  mutated.atomic_groups[1].created_node_id =
      morsehgp3d::hierarchy::ExactDirectK1NodeId{3U};
  const auto mutation_verification =
      morsehgp3d::hierarchy::
          verify_exact_direct_k1_forest_journal_streaming(
              cloud,
              pipeline.facade,
              pipeline.journal,
              pipeline.seed_budget,
              pipeline.seed_journal,
              pipeline.k1_budget,
              mutated);
  check(
      !mutation_verification.atomic_groups_certified &&
          !mutation_verification.fresh_streaming_replay_certified &&
          !mutation_verification.result_certified,
      "streaming replay rejects an invented node on a q=1 continuation");

  ExactDirectK1ForestBudget short_budget = pipeline.k1_budget;
  short_budget.maximum_arm_root_binding_count =
      forest.required_arm_root_binding_count - 1U;
  const ExactDirectK1ForestJournalResult exhausted =
      morsehgp3d::hierarchy::build_exact_direct_k1_forest_journal(
          cloud,
          pipeline.facade,
          pipeline.journal,
          pipeline.seed_budget,
          pipeline.seed_journal,
          short_budget);
  check(
      exhausted.decision ==
              ExactDirectK1ForestDecision::no_k1_budget_exhausted &&
          !exhausted.budget_preflight_certified &&
          !exhausted.source_seed_journal_streaming_replayed &&
          exhausted.arm_root_bindings.empty() &&
          exhausted.saddle_records.empty() &&
          exhausted.atomic_groups.empty() &&
          exhausted.child_node_ids.empty() && exhausted.batches.empty(),
      "a one-short binding budget fails atomically before source replay or payload allocation");
  const auto exhausted_verification =
      morsehgp3d::hierarchy::
          verify_exact_direct_k1_forest_journal_streaming(
              cloud,
              pipeline.facade,
              pipeline.journal,
              pipeline.seed_budget,
              pipeline.seed_journal,
              short_budget,
              exhausted);
  check(
      !exhausted_verification.arm_root_bindings_certified &&
          !exhausted_verification.saddle_records_certified &&
          !exhausted_verification.atomic_groups_certified &&
          !exhausted_verification.child_references_certified &&
          !exhausted_verification.batches_certified &&
          !exhausted_verification.fresh_streaming_replay_certified &&
          !exhausted_verification.result_certified,
      "a diagnostic replay never certifies unvisited output arenas");

  auto forged_seed_journal = pipeline.seed_journal;
  if (!forged_seed_journal.families.empty()) {
    forged_seed_journal.families.front().order = 2U;
    const ExactDirectK1ForestJournalResult rejected_source =
        morsehgp3d::hierarchy::build_exact_direct_k1_forest_journal(
            cloud,
            pipeline.facade,
            pipeline.journal,
            pipeline.seed_budget,
            forged_seed_journal,
            pipeline.k1_budget);
    check(
        rejected_source.decision == ExactDirectK1ForestDecision::
                                        no_k1_source_seed_journal_rejected &&
            rejected_source.arm_root_bindings.empty() &&
            rejected_source.saddle_records.empty() &&
            rejected_source.atomic_groups.empty() &&
            rejected_source.child_node_ids.empty() &&
            rejected_source.batches.empty(),
        "an unverified seed journal cannot steer K1 preflight or allocation");
  }
  check_streaming_replay(cloud, pipeline, "q=1 continuation fixture");
}

}  // namespace

int main() {
  test_two_points_freeze_two_roots();
  test_e3_equal_level_is_one_ternary_multifusion();
  test_q1_continuation_preserves_the_prior_root();
  if (failures != 0) {
    std::cerr << failures << " direct K1 forest journal check(s) failed\n";
    return 1;
  }
  std::cout << "all direct K1 forest journal checks passed\n";
  return 0;
}
