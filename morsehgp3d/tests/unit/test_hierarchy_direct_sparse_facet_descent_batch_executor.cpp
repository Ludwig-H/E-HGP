#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_batch_executor.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
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
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

constexpr std::uint64_t authority_id = UINT64_C(0x14D);
int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
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

struct Scenario {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedJournalResult seed_journal;
};

[[nodiscard]] Scenario make_scenario(
    CanonicalPointCloud cloud,
    std::size_t maximum_order) {
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSupportTerminalBudget terminal_budget{
      source_pair_budget(), source_higher_budget()};
  const auto pair = build_exact_pair_support_stream(
      index, cloud, maximum_order, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, maximum_order, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, maximum_order, terminal_budget, pair, higher);
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

[[nodiscard]] Scenario regular_tetrahedron_order_one_scenario() {
  const std::array<CertifiedPoint3, 4U> points{
      point(1.0, 1.0, 1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, 1.0, -1.0),
      point(1.0, -1.0, -1.0),
  };
  return make_scenario(canonical_cloud(points), 1U);
}

[[nodiscard]] Scenario mixed_support_same_batch_scenario() {
  const std::array<CertifiedPoint3, 6U> points{
      point(-5.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(5.0, 0.0, 0.0),
      point(97.0, -4.0, 0.0),
      point(97.0, 4.0, 0.0),
      point(105.0, 0.0, 0.0),
  };
  return make_scenario(canonical_cloud(points), 5U);
}

[[nodiscard]] Scenario ac_to_de_order_two_scenario() {
  // Input labels A, B, C, D, E canonicalize to D=0, A=1, B=2, C=3, E=4.
  const std::array<CertifiedPoint3, 5U> points{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0),
  };
  return make_scenario(canonical_cloud(points), 2U);
}

[[nodiscard]] ExactDirectMorseIndustrialPlanConfig plan_config(
    ExactDirectMorseIndustrialPolicy policy =
        ExactDirectMorseIndustrialPolicy::interactive_resident_50k,
    std::uint64_t maximum_batch_count = 256U) {
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
  return {
      3U,
      256U,
      256U,
      2560U,
      256U,
  };
}

[[nodiscard]] ExactDirectSparseFacetDescentStepBudget step_budget() {
  return {
      ExactDirectSparsePositiveFacetProbeBudget{513U, 256U},
      ExactLbvhTopKBudget{
          4096U, 4096U, 4096U, 4096U, 256U, 16U, 16U},
      ExactDirectSparsePositiveFacetProbeBudget{513U, 256U},
  };
}

[[nodiscard]] ExactDirectSparseFacetDescentClosureBudget closure_budget() {
  return {
      256U,
      256U,
      256U,
      513U,
      step_budget(),
  };
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocatorBudget locator_budget() {
  return {
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
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocator make_locator(
    std::size_t component_handle_count) {
  return build_exact_direct_sparse_positive_facet_locator(
      component_handle_count,
      locator_budget(),
      ExactDirectSparsePositiveFacetLocatorConfig{
          authority_id, std::numeric_limits<std::uint64_t>::max()});
}

[[nodiscard]] ExactDirectSparseFacetWitness query_witness(
    std::uint64_t replay_token) {
  return {authority_id, replay_token};
}

[[nodiscard]] bool key_less(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right) noexcept {
  const std::size_t shared_count =
      std::min(left.point_count, right.point_count);
  for (std::size_t point_index = 0U;
       point_index < shared_count;
       ++point_index) {
    if (left.point_ids[point_index] != right.point_ids[point_index]) {
      return left.point_ids[point_index] < right.point_ids[point_index];
    }
  }
  return left.point_count < right.point_count;
}

[[nodiscard]] ExactDirectSparseFacetKey two_point_key(
    PointId first,
    PointId second) {
  ExactDirectSparseFacetKey key;
  key.point_ids[0U] = first;
  key.point_ids[1U] = second;
  key.point_count = 2U;
  return key;
}

[[nodiscard]] ExactDirectSparseFacetKey facet_key(
    const ExactDirectSaddleArmFacet& facet) {
  ExactDirectSparseFacetKey key;
  key.point_count = facet.point_count;
  key.point_ids = facet.point_ids;
  return key;
}

[[nodiscard]] std::vector<ExactDirectSparseFacetKey>
all_distinct_arm_keys(const Scenario& scenario) {
  std::vector<ExactDirectSparseFacetKey> keys;
  keys.reserve(scenario.seed_journal.arm_seeds.size());
  for (std::size_t arm_seed_index = 0U;
       arm_seed_index < scenario.seed_journal.arm_seeds.size();
       ++arm_seed_index) {
    keys.push_back(facet_key(
        reconstruct_exact_direct_saddle_arm_facet(
            scenario.facade, scenario.seed_journal, arm_seed_index)));
  }
  std::sort(keys.begin(), keys.end(), key_less);
  keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
  return keys;
}

void bind_positive_keys(
    ExactDirectSparsePositiveFacetLocator& locator,
    std::span<const ExactDirectSparseFacetKey> keys) {
  std::vector<ExactDirectSparseFacetBinding> bindings;
  bindings.reserve(keys.size());
  for (std::size_t key_index = 0U; key_index < keys.size(); ++key_index) {
    bindings.push_back(
        {key_index,
         keys[key_index],
         key_index,
         query_witness(
             static_cast<std::uint64_t>(key_index) * UINT64_C(3) +
             UINT64_C(1))});
  }
  const auto committed = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      bindings);
  check(
      committed.certified_committed_batch(),
      "the batch-executor fixture binds its compact positive-key authority");
}

[[nodiscard]] std::vector<ExactDirectSparseFacetKey> singleton_keys(
    std::size_t point_count) {
  std::vector<ExactDirectSparseFacetKey> keys;
  keys.reserve(point_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    ExactDirectSparseFacetKey key;
    key.point_count = 1U;
    key.point_ids[0U] = static_cast<PointId>(point_index);
    keys.push_back(key);
  }
  return keys;
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchPlanResult build_plan(
    const Scenario& scenario,
    const ExactDirectMorseIndustrialPlanConfig& config) {
  return build_exact_direct_sparse_facet_descent_batch_plan(
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      source_seed_budget(),
      scenario.seed_journal,
      config,
      plan_budget());
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchPlanResult build_plan(
    const Scenario& scenario) {
  return build_plan(scenario, plan_config());
}

void test_anchored_multibatch_retries_and_transient_closure_release() {
  const Scenario scenario = regular_tetrahedron_order_one_scenario();
  const auto observed_plan = build_plan(scenario);
  ExactDirectSparsePositiveFacetLocator locator =
      make_locator(scenario.cloud.size());
  ExactDirectSparseFacetDescentAnchoredBatchExecutor executor(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      source_seed_budget(),
      scenario.seed_journal,
      plan_config(),
      plan_budget(),
      observed_plan,
      locator);

  check(
      observed_plan.complete_architecture_plan() &&
          scenario.event_journal.batches.size() == 2U &&
          executor.audit().source_plan_verification_count == 1U &&
          executor.audit().source_plan_owned_by_session &&
          !executor.audit().full_source_plan_replayed_per_batch,
      "one order-one tetrahedron session owns one fresh plan for its two exact batches");
  if (scenario.event_journal.batches.size() != 2U) {
    return;
  }

  auto execution_caps = execution_budget();
  execution_caps.maximum_selected_arm_seed_count = 12U;
  execution_caps.maximum_resolved_key_count = 4U;
  const auto closure_caps = closure_budget();
  const auto first_witness = query_witness(UINT64_C(3));
  auto invalid_empty_batch_closure_caps = closure_caps;
  invalid_empty_batch_closure_caps.maximum_seed_count =
      direct_sparse_facet_descent_closure_maximum_seed_count + 1U;
  bool invalid_closure_caps_rejected = false;
  try {
    static_cast<void>(executor.prepare_next(
        first_witness,
        execution_caps,
        invalid_empty_batch_closure_caps));
  } catch (const std::invalid_argument&) {
    invalid_closure_caps_rejected = true;
  }
  check(
      invalid_closure_caps_rejected &&
          executor.next_source_batch_index() == 0U &&
          executor.audit().prepare_attempt_count == 0U,
      "an out-of-confidence closure cap is rejected even for an empty batch");

  const auto stale_first =
      executor.prepare_next(first_witness, execution_caps, closure_caps);
  check(
      stale_first.complete_architecture_execution() &&
          stale_first.required_selected_arm_seed_count == 0U &&
          stale_first.transient_closure_released_before_delta_publication &&
          !stale_first.closure_graph_persisted,
      "the first birth-only batch publishes an empty compact delta with no retained closure");

  const auto empty_locator_commit = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      std::span<const ExactDirectSparseFacetBinding>{});
  check(
      empty_locator_commit.certified_committed_batch(),
      "an empty locator transaction advances the stamp between prepare and replay");
  const auto stale_rejection = executor.commit_prepared(
      first_witness, execution_caps, closure_caps, stale_first);
  check(
      !stale_rejection.locator_snapshot_matches_observed_build &&
          !stale_rejection.exact_result_equality_certified &&
          !stale_rejection.session_advanced &&
          executor.next_source_batch_index() == 0U,
      "a stale locator stamp rejects the observed first batch without advancing its cursor");

  const auto retried_first =
      executor.prepare_next(first_witness, execution_caps, closure_caps);
  const auto accepted_first = executor.commit_prepared(
      first_witness, execution_caps, closure_caps, retried_first);
  check(
      accepted_first.result_certified &&
          accepted_first.session_advanced &&
          executor.next_source_batch_index() == 1U,
      "the same first batch succeeds when retried against the current locator stamp");

  const auto positive_singletons = singleton_keys(scenario.cloud.size());
  bind_positive_keys(locator, positive_singletons);

  auto one_arm_short = execution_caps;
  one_arm_short.maximum_selected_arm_seed_count = 11U;
  const auto exhausted = executor.prepare_next(
      query_witness(UINT64_C(6)), one_arm_short, closure_caps);
  check(
      exhausted.decision ==
              ExactDirectSparseFacetDescentBatchExecutionDecision::
                  no_execution_batch_budget_exhausted &&
          exhausted.required_selected_arm_seed_count == 12U &&
          exhausted.arm_joins.empty() &&
          exhausted.resolved_keys.empty() &&
          executor.next_source_batch_index() == 1U,
      "an arm cap of eleven reports the twelve-arm requirement without a partial delta");
  const auto exhausted_replay = executor.commit_prepared(
      query_witness(UINT64_C(6)),
      one_arm_short,
      closure_caps,
      exhausted);
  check(
      exhausted_replay.exact_batch_execution_freshly_replayed &&
          !exhausted_replay.session_advanced &&
          executor.next_source_batch_index() == 1U,
      "a freshly reproduced budget exhaustion remains retryable and does not advance");

  auto one_resolved_key_short = execution_caps;
  one_resolved_key_short.maximum_resolved_key_count = 3U;
  const auto resolved_key_exhausted = executor.prepare_next(
      query_witness(UINT64_C(6)),
      one_resolved_key_short,
      closure_caps);
  check(
      resolved_key_exhausted.decision ==
              ExactDirectSparseFacetDescentBatchExecutionDecision::
                  no_execution_batch_budget_exhausted &&
          resolved_key_exhausted.required_resolved_key_count == 4U &&
          !resolved_key_exhausted.batch_budget_preflight_satisfied &&
          resolved_key_exhausted.counters.shared_closure_build_count == 0U &&
          resolved_key_exhausted.arm_joins.empty() &&
          resolved_key_exhausted.resolved_keys.empty() &&
          executor.next_source_batch_index() == 1U,
      "a resolved-key cap of three rejects four distinct keys before closure or partial delta");
  const auto resolved_key_exhausted_replay = executor.commit_prepared(
      query_witness(UINT64_C(6)),
      one_resolved_key_short,
      closure_caps,
      resolved_key_exhausted);
  check(
      resolved_key_exhausted_replay
          .exact_batch_execution_freshly_replayed &&
          !resolved_key_exhausted_replay.session_advanced &&
          executor.next_source_batch_index() == 1U,
      "the freshly reproduced resolved-key exhaustion also remains retryable");

  const auto prepared = executor.prepare_next(
      query_witness(UINT64_C(6)), execution_caps, closure_caps);
  check(
      prepared.complete_architecture_execution() &&
          prepared.required_selected_lane_count == 1U &&
          prepared.required_selected_family_count == 6U &&
          prepared.required_selected_arm_seed_count == 12U &&
          prepared.required_resolved_key_count == 4U &&
          prepared.counters.duplicate_arm_key_reference_count == 8U &&
          prepared.counters.shared_closure_build_count == 1U &&
          prepared.closure_summary.transient_node_count == 4U &&
          prepared.closure_summary.transient_edge_count == 0U &&
          prepared.closure_summary.transient_seed_projection_count == 4U &&
          prepared.transient_closure_released_before_delta_publication &&
          !prepared.closure_graph_persisted &&
          !prepared.closure_summary.graph_payload_persisted,
      "six equal-level tetrahedron edges share one closure and compact twelve arm references into four keys");

  std::array<std::size_t, 4U> references_per_resolved_key{};
  bool every_join_is_dense = prepared.arm_joins.size() == 12U;
  for (const auto& join : prepared.arm_joins) {
    if (join.resolved_key_index >= references_per_resolved_key.size()) {
      every_join_is_dense = false;
      continue;
    }
    ++references_per_resolved_key[join.resolved_key_index];
    every_join_is_dense =
        every_join_is_dense && join.arm_identity_and_full_key_joined;
  }
  check(
      every_join_is_dense &&
          std::all_of(
              references_per_resolved_key.begin(),
              references_per_resolved_key.end(),
              [](std::size_t count) { return count == 3U; }),
      "each singleton key is retained once and referenced by its three incident tetrahedron edges");

  auto malformed_compact_delta = prepared;
  ++malformed_compact_delta.resolved_keys.front().resolved_key_index;
  check(
      !malformed_compact_delta.complete_architecture_execution(),
      "the standalone predicate rejects a compact-delta key-index mutation");
  auto malformed_duplicate_counter = prepared;
  ++malformed_duplicate_counter.counters.duplicate_arm_key_reference_count;
  check(
      !malformed_duplicate_counter.complete_architecture_execution(),
      "the standalone predicate rejects an inconsistent duplicate-key counter");

  const auto accepted = executor.commit_prepared(
      query_witness(UINT64_C(6)),
      execution_caps,
      closure_caps,
      prepared);
  const auto& audit = executor.audit();
  check(
      accepted.result_certified && accepted.session_advanced &&
          executor.complete() &&
          audit.source_plan_verification_count == 1U &&
          audit.prepare_attempt_count == 5U &&
          audit.fresh_batch_replay_count == 5U &&
          audit.accepted_batch_count == 2U &&
          audit.rejected_batch_replay_count == 3U &&
          audit.transient_closure_build_count == 2U &&
          audit.maximum_transient_closure_node_count == 4U &&
          audit.retained_closure_node_count == 0U &&
          !audit.full_source_plan_replayed_per_batch &&
          !audit.closure_graph_retained_between_batches,
      "the two-batch session verifies its source once and retains no closure graph across retries or batches");
}

void test_streaming_session_advances_across_chunk_boundaries() {
  const Scenario scenario = regular_tetrahedron_order_one_scenario();
  const auto config = plan_config(
      ExactDirectMorseIndustrialPolicy::massive_external_streaming, 1U);
  const auto observed_plan = build_plan(scenario, config);
  const auto positive_singletons = singleton_keys(scenario.cloud.size());
  ExactDirectSparsePositiveFacetLocator locator =
      make_locator(scenario.cloud.size());
  bind_positive_keys(locator, positive_singletons);
  ExactDirectSparseFacetDescentAnchoredBatchExecutor executor(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      source_seed_budget(),
      scenario.seed_journal,
      config,
      plan_budget(),
      observed_plan,
      locator);

  check(
      observed_plan.complete_architecture_plan() &&
          observed_plan.source_industrial_plan.chunks.size() == 2U,
      "a one-batch streaming cap puts the two exact order-one batches in two chunks");
  if (observed_plan.source_industrial_plan.chunks.size() != 2U) {
    return;
  }

  for (std::size_t batch_index = 0U; batch_index < 2U; ++batch_index) {
    const auto witness = query_witness(
        UINT64_C(2000) + static_cast<std::uint64_t>(batch_index));
    const auto prepared =
        executor.prepare_next(witness, execution_budget(), closure_budget());
    check(
        prepared.complete_architecture_execution() &&
            prepared.source_batch_index == batch_index &&
            prepared.source_chunk_index.has_value() &&
            *prepared.source_chunk_index == batch_index,
        "the streaming cursor executes each exact batch inside its owning chunk");
    const auto accepted = executor.commit_prepared(
        witness, execution_budget(), closure_budget(), prepared);
    check(
        accepted.result_certified && accepted.session_advanced,
        "the streaming cursor advances atomically across one chunk boundary");
  }

  check(
      executor.complete() &&
          executor.audit().source_plan_verification_count == 1U &&
          executor.audit().accepted_batch_count == 2U &&
          executor.audit().retained_closure_node_count == 0U &&
          !executor.audit().closure_graph_retained_between_batches,
      "the multi-chunk session keeps one anchored plan and no historical closure graph");
}

void test_mixed_lanes_feed_one_shared_closure() {
  const Scenario scenario = mixed_support_same_batch_scenario();
  const auto observed_plan = build_plan(scenario);

  std::size_t mixed_batch_index = scenario.event_journal.batches.size();
  for (std::size_t batch_index = 0U;
       batch_index < scenario.event_journal.batches.size();
       ++batch_index) {
    bool has_support_two = false;
    bool has_support_three = false;
    for (const auto& lane : observed_plan.lanes) {
      if (lane.source_batch_index != batch_index ||
          lane.facet_cardinality != 2U) {
        continue;
      }
      has_support_two =
          has_support_two || lane.source_support_cardinality == 2U;
      has_support_three =
          has_support_three || lane.source_support_cardinality == 3U;
    }
    if (has_support_two && has_support_three) {
      mixed_batch_index = batch_index;
      break;
    }
  }
  check(
      observed_plan.complete_architecture_plan() &&
          mixed_batch_index < scenario.event_journal.batches.size(),
      "the two-cluster fixture exposes one exact order-two batch with support-two and support-three lanes");
  if (mixed_batch_index >= scenario.event_journal.batches.size()) {
    return;
  }

  // This fixture isolates 14D scheduling: every factorized arm key is made a
  // relative-positive external authority before the session, so no unrelated
  // hierarchy commit is needed to reach the mixed structural batch.
  const auto positive_keys = all_distinct_arm_keys(scenario);
  ExactDirectSparsePositiveFacetLocator locator =
      make_locator(positive_keys.size());
  bind_positive_keys(locator, positive_keys);
  ExactDirectSparseFacetDescentAnchoredBatchExecutor executor(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      source_seed_budget(),
      scenario.seed_journal,
      plan_config(),
      plan_budget(),
      observed_plan,
      locator);

  bool target_executed = false;
  while (executor.next_source_batch_index() <= mixed_batch_index) {
    const std::size_t current_batch = executor.next_source_batch_index();
    const std::uint64_t replay_token =
        UINT64_C(1000) + static_cast<std::uint64_t>(current_batch);
    const auto witness = query_witness(replay_token);
    const auto prepared =
        executor.prepare_next(witness, execution_budget(), closure_budget());
    check(
        prepared.complete_architecture_execution(),
        "each prefix batch leading to the mixed-lane fixture produces a complete compact delta");
    if (!prepared.complete_architecture_execution()) {
      break;
    }

    if (current_batch == mixed_batch_index) {
      const auto lane_begin = prepared.source_lane_begin_index;
      const auto lane_end = prepared.source_lane_end_index;
      std::size_t support_two_lane = observed_plan.lanes.size();
      std::size_t support_three_lane = observed_plan.lanes.size();
      for (std::size_t lane_index = lane_begin;
           lane_index < lane_end;
           ++lane_index) {
        const auto support_cardinality =
            observed_plan.lanes[lane_index].source_support_cardinality;
        if (support_cardinality == 2U) {
          support_two_lane = lane_index;
        } else if (support_cardinality == 3U) {
          support_three_lane = lane_index;
        }
      }
      std::size_t support_two_joins = 0U;
      std::size_t support_three_joins = 0U;
      for (const auto& join : prepared.arm_joins) {
        if (join.lane_index == support_two_lane) {
          ++support_two_joins;
        } else if (join.lane_index == support_three_lane) {
          ++support_three_joins;
        }
      }
      check(
          prepared.required_selected_lane_count == 2U &&
              prepared.required_selected_family_count == 2U &&
              prepared.required_selected_arm_seed_count == 5U &&
              support_two_lane < observed_plan.lanes.size() &&
              support_three_lane < observed_plan.lanes.size() &&
              support_two_joins == 2U && support_three_joins == 3U &&
              prepared.counters.shared_closure_build_count == 1U &&
              prepared.one_shared_closure_and_memo_built_or_empty_batch &&
              prepared.transient_closure_released_before_delta_publication &&
              !prepared.closure_graph_persisted,
          "the support-two and support-three lanes select two plus three arms into one shared transient closure");
      target_executed = true;
    }

    const auto accepted = executor.commit_prepared(
        witness, execution_budget(), closure_budget(), prepared);
    check(
        accepted.result_certified && accepted.session_advanced,
        "the anchored session advances each freshly replayed mixed-fixture prefix batch");
    if (!accepted.session_advanced) {
      break;
    }
  }
  check(
      target_executed &&
          executor.audit().source_plan_verification_count == 1U &&
          executor.audit().retained_closure_node_count == 0U &&
          !executor.audit().closure_graph_retained_between_batches,
      "the mixed-lane batch reuses the one anchored plan and releases every closure before the next batch");
}

void test_canonical_key_view_executes_nonzero_strict_edge() {
  const Scenario scenario = ac_to_de_order_two_scenario();
  const auto observed_plan = build_plan(scenario);
  const ExactDirectSparseFacetKey ac = two_point_key(1U, 3U);
  const ExactDirectSparseFacetKey de = two_point_key(0U, 4U);

  std::vector<ExactDirectSparseFacetKey> positive_keys =
      all_distinct_arm_keys(scenario);
  positive_keys.erase(
      std::remove(positive_keys.begin(), positive_keys.end(), ac),
      positive_keys.end());
  positive_keys.push_back(de);
  std::sort(positive_keys.begin(), positive_keys.end(), key_less);
  positive_keys.erase(
      std::unique(positive_keys.begin(), positive_keys.end()),
      positive_keys.end());
  const auto de_position =
      std::lower_bound(positive_keys.begin(), positive_keys.end(), de, key_less);
  check(
      observed_plan.complete_architecture_plan() &&
          de_position != positive_keys.end() && *de_position == de,
      "the E5 14D fixture owns one complete plan and one pre-bound DE carrier");
  if (!observed_plan.complete_architecture_plan() ||
      de_position == positive_keys.end() || *de_position != de) {
    return;
  }
  const std::size_t de_handle =
      static_cast<std::size_t>(
          std::distance(positive_keys.begin(), de_position));

  ExactDirectSparsePositiveFacetLocator locator =
      make_locator(positive_keys.size());
  bind_positive_keys(locator, positive_keys);
  ExactDirectSparseFacetDescentAnchoredBatchExecutor executor(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      source_seed_budget(),
      scenario.seed_journal,
      plan_config(),
      plan_budget(),
      observed_plan,
      locator);

  bool strict_ac_edge_observed = false;
  while (!executor.complete()) {
    const std::size_t batch_index = executor.next_source_batch_index();
    const auto batch_witness = query_witness(
        UINT64_C(3000) + static_cast<std::uint64_t>(batch_index));
    const auto prepared = executor.prepare_next(
        batch_witness, execution_budget(), closure_budget());
    check(
        prepared.complete_architecture_execution(),
        "every E5 prefix lot remains complete when all arm keys except AC are pre-bound");
    if (!prepared.complete_architecture_execution()) {
      break;
    }

    const auto ac_resolution = std::find_if(
        prepared.resolved_keys.begin(),
        prepared.resolved_keys.end(),
        [&ac](
            const ExactDirectSparseFacetDescentBatchResolvedKey& resolved) {
          return resolved.source_facet_key == ac;
        });
    if (ac_resolution != prepared.resolved_keys.end()) {
      strict_ac_edge_observed =
          prepared.closure_summary.transient_edge_count != 0U &&
          prepared.closure_summary.counters.strict_edge_count != 0U &&
          prepared.closure_summary.counters.successor_positive_hit_count !=
              0U &&
          ac_resolution->resolved_component_handle == de_handle &&
          prepared.transient_closure_released_before_delta_publication &&
          !prepared.closure_graph_persisted;
    }

    const auto accepted = executor.commit_prepared(
        batch_witness, execution_budget(), closure_budget(), prepared);
    check(
        accepted.result_certified && accepted.session_advanced,
        "the E5 strict-edge lot and its prefixes replay exactly through the anchored session");
    if (!accepted.session_advanced) {
      break;
    }
  }
  check(
      strict_ac_edge_observed && executor.complete() &&
          executor.audit().maximum_transient_closure_node_count >= 2U &&
          executor.audit().retained_closure_node_count == 0U &&
          !executor.audit().closure_graph_retained_between_batches,
      "14D feeds AC through the canonical distinct-key view to a nonzero strict edge ending at DE, then releases the graph");
}

void test_falsified_plan_is_rejected_at_session_open() {
  const Scenario scenario = regular_tetrahedron_order_one_scenario();
  auto falsified_plan = build_plan(scenario);
  check(
      !falsified_plan.lanes.empty(),
      "the falsified-plan fixture contains one structural lane");
  if (falsified_plan.lanes.empty()) {
    return;
  }
  ++falsified_plan.lanes.front().matching_family_count;

  const ExactDirectSparsePositiveFacetLocator locator =
      make_locator(scenario.cloud.size());
  bool rejected = false;
  try {
    ExactDirectSparseFacetDescentAnchoredBatchExecutor executor(
        scenario.index,
        scenario.cloud,
        scenario.facade,
        scenario.event_journal,
        source_seed_budget(),
        scenario.seed_journal,
        plan_config(),
        plan_budget(),
        falsified_plan,
        locator);
    static_cast<void>(executor);
  } catch (const std::invalid_argument&) {
    rejected = true;
  }
  check(
      rejected,
      "one falsified lane count is rejected by the single fresh session anchor");
}

}  // namespace

int main() {
  test_anchored_multibatch_retries_and_transient_closure_release();
  test_streaming_session_advances_across_chunk_boundaries();
  test_mixed_lanes_feed_one_shared_closure();
  test_canonical_key_view_executes_nonzero_strict_edge();
  test_falsified_plan_is_rejected_at_session_open();

  if (failures != 0) {
    std::cerr << failures
              << " direct sparse facet descent batch-executor test(s) failed\n";
    return 1;
  }
  return 0;
}
