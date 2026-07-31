#include "morsehgp3d/hierarchy/direct_morse_terminal_reducer_source_bridge.hpp"
#include "morsehgp3d/hierarchy/direct_morse_terminal_forest_reduction.hpp"
#include "morsehgp3d/hierarchy/direct_morse_vertical_journal.hpp"

#include <algorithm>
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

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::
    ExactDirectMorseForestSourceBatchVisitDecision;
using morsehgp3d::hierarchy::ExactDirectMorseForestBudget;
using morsehgp3d::hierarchy::ExactDirectMorseIndustrialPolicy;
using morsehgp3d::hierarchy::
    ExactDirectMorseTerminalForestReductionConfig;
using morsehgp3d::hierarchy::
    ExactDirectMorseTerminalForestReductionDecision;
using morsehgp3d::hierarchy::ExactDirectMorseTerminalReducerSourceBridgeDecision;
using morsehgp3d::hierarchy::ExactDirectSparseFacetDescentStepBudget;
using morsehgp3d::hierarchy::ExactDirectSparsePositiveFacetProbeBudget;
using morsehgp3d::hierarchy::ExactDirectSaddleArmSeedBudget;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
using morsehgp3d::hierarchy::ExactHigherSupportTerminalRunStatus;
using morsehgp3d::hierarchy::ExactHigherSupportTerminalSession;
using morsehgp3d::hierarchy::ExactMortonGroupedAnchoredPairScheduleConfig;
using morsehgp3d::hierarchy::ExactSparseAnchoredPairSession;
using morsehgp3d::hierarchy::ExactSparseAnchoredPairSessionAdvanceBudget;
using morsehgp3d::hierarchy::ExactSparseAnchoredPairSessionStepKind;
using morsehgp3d::hierarchy::ExactSparseAnchoredPairSessionTotalCapacity;
using morsehgp3d::hierarchy::ExactSparseAnchoredPairTerminalAuthority;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::MortonLbvhIndex;

constexpr std::uint64_t reduction_authority_id = UINT64_C(0x15D15D);
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

[[nodiscard]] CanonicalPointCloud regular_tetrahedron() {
  const std::array<CertifiedPoint3, 4U> points{
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud right_triangle() {
  const std::array<CertifiedPoint3, 3U> points{
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(0.0, 2.0, 0.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud uniform_latin_six() {
  constexpr std::size_t modulus = 65537U;
  std::vector<CertifiedPoint3> points;
  points.reserve(6U);
  for (std::size_t index = 0U; index < 6U; ++index) {
    const std::size_t value = index + 1U;
    const std::size_t permuted_y =
        (value * 25173U + 13849U) % modulus;
    const std::size_t permuted_z =
        (value * 13849U + 25173U) % modulus;
    points.push_back(CertifiedPoint3::from_binary64(
        static_cast<double>(value) / static_cast<double>(modulus),
        static_cast<double>(permuted_y) /
            static_cast<double>(modulus),
        static_cast<double>(permuted_z) /
            static_cast<double>(modulus)));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
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
      maximum};
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget seed_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactSparseAnchoredPairSessionTotalCapacity pair_capacity() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] ExactSparseAnchoredPairSessionAdvanceBudget pair_budget() {
  constexpr std::size_t work = 4096U;
  return {{work, work, work, work}, {work}, work, work};
}

[[nodiscard]] ExactDirectSparseFacetDescentStepBudget descent_step_budget() {
  return {
      ExactDirectSparsePositiveFacetProbeBudget{513U, 256U},
      ExactLbvhTopKBudget{
          4096U, 4096U, 4096U, 4096U, 256U, 16U, 16U},
      ExactDirectSparsePositiveFacetProbeBudget{513U, 256U},
  };
}

[[nodiscard]] ExactDirectMorseForestBudget forest_reduction_budget() {
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
      descent_step_budget(),
  };
  budget.quotient_budget = {256U, 256U, 256U, 256U, 1024U};
  return budget;
}

[[nodiscard]] ExactDirectMorseTerminalForestReductionConfig
forest_reduction_config() {
  ExactDirectMorseTerminalForestReductionConfig config;
  config.industrial_config.policy =
      ExactDirectMorseIndustrialPolicy::massive_external_streaming;
  config.industrial_config.memory_model = {
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
  config.industrial_config.chunk_budget = {
      1'000'000U, 256U, 4096U, 4096U, 4096U, 4096U};
  config.plan_budget = {
      256U, 256U, 256U, 1024U, 256U, 1024U, 1'000'000U, 64U};
  config.forest_budget = forest_reduction_budget();
  config.execution_budget = {3U, 256U, 256U, 2560U, 256U};
  config.forest_config.locator_config.external_authority_id =
      reduction_authority_id;
  return config;
}

[[nodiscard]] morsehgp3d::hierarchy::ExactDirectMorseVerticalBudget
vertical_budget() {
  constexpr std::size_t capacity = 4096U;
  return {
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity * capacity,
      capacity,
      capacity * capacity,
      capacity,
      capacity * capacity,
  };
}

[[nodiscard]] ExactSparseAnchoredPairTerminalAuthority pair_authority(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  const auto manifest =
      morsehgp3d::hierarchy::make_exact_pair_support_checkpoint_manifest(
          index, cloud, requested_maximum_order);
  const std::size_t maximum_closed_rank =
      std::max<std::size_t>(2U, manifest.maximum_relevant_closed_rank);
  ExactSparseAnchoredPairSession session =
      ExactSparseAnchoredPairSession::start(
          index,
          cloud,
          maximum_closed_rank,
          ExactMortonGroupedAnchoredPairScheduleConfig{4U, 0U},
          pair_capacity());
  for (std::size_t call = 0U; call < 100000U; ++call) {
    const auto step = session.advance(index, cloud, pair_budget());
    if (step.kind() ==
        ExactSparseAnchoredPairSessionStepKind::total_capacity_exhausted) {
      throw std::logic_error("the pair terminal fixture exhausted capacity");
    }
    if (step.kind() == ExactSparseAnchoredPairSessionStepKind::complete) {
      return std::move(session).seal();
    }
  }
  throw std::logic_error("the pair terminal fixture did not terminate");
}

[[nodiscard]] morsehgp3d::hierarchy::ExactHigherSupportTerminalAuthority
higher_authority(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportStreamBudget& budget) {
  ExactHigherSupportTerminalSession session{
      index, cloud, requested_maximum_order, budget, 256U};
  if (session.run_to_terminal() !=
      ExactHigherSupportTerminalRunStatus::terminal) {
    throw std::logic_error("the higher terminal fixture did not terminate");
  }
  return std::move(session).seal();
}

void test_supported_orders_produce_complete_provider() {
  const CanonicalPointCloud cloud = regular_tetrahedron();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto higher = higher_budget();
  const auto seed = seed_budget();

  for (const std::size_t requested_order : {5U, 10U}) {
    auto pairs = pair_authority(index, cloud, requested_order);
    auto higher_supports =
        higher_authority(index, cloud, requested_order, higher);
    auto result = morsehgp3d::hierarchy::
        build_exact_direct_morse_terminal_reducer_source_bridge(
            index,
            cloud,
            requested_order,
            higher,
            seed,
            3U,
            std::move(pairs),
            std::move(higher_supports));

    check(
        result.certified() && result.bridge != nullptr &&
            result.audit.requested_maximum_order == requested_order &&
            result.audit.effective_maximum_order == cloud.size() &&
            result.audit.all_support_arities_two_through_four_terminal &&
            result.audit.no_relevant_extra_shell_diagnostics &&
            result.bridge->direct_support_facade()
                .terminal_catalog_certified() &&
            result.bridge->event_journal().certified_partial_refinement() &&
            result.bridge->saddle_arm_seed_journal()
                .certified_partial_refinement() &&
            result.bridge->source_manifest().certified() &&
            result.bridge->source_manifest().requested_maximum_order ==
                requested_order,
        "K=5 and K=10 each close the complete terminal-to-provider chain");

    if (result.bridge == nullptr) {
      continue;
    }
    auto provider = result.bridge->source_provider();
    std::size_t visited = 0U;
    for (std::size_t batch = 0U;
         batch < result.bridge->source_manifest().batch_count;
         ++batch) {
      auto inspect = [&](const auto& window) {
        ++visited;
        return window.certified_relative_to(
            result.bridge->source_manifest());
      };
      check(
          provider(batch, inspect) ==
              ExactDirectMorseForestSourceBatchVisitDecision::
                  complete_synchronous_visit,
          "every reducer source batch is synchronously certified");
    }
    check(
        visited == result.bridge->source_manifest().batch_count,
        "the provider exposes the complete dense batch partition");

    const auto manifest_before_move = result.bridge->source_manifest();
    auto moved_bridge = std::move(*result.bridge);
    result.bridge.reset();
    std::size_t post_move_visit_count = 0U;
    auto inspect_after_move = [&](const auto& window) {
      ++post_move_visit_count;
      return window.certified_relative_to(manifest_before_move);
    };
    check(
        manifest_before_move.batch_count != 0U &&
            provider(0U, inspect_after_move) ==
                ExactDirectMorseForestSourceBatchVisitDecision::
                    complete_synchronous_visit &&
            post_move_visit_count == 1U &&
            moved_bridge.source_manifest() == manifest_before_move,
        "a provider view remains bound to the stable pimpl after its bridge moves");

    auto coverage =
        moved_bridge.make_downstream_complete_component_coverage();
    static_assert(!decltype(coverage)::source_pruning_authority);
    const std::array<morsehgp3d::spatial::PointId, 4U> covered{
        3U, 1U, 3U, 2U};
    coverage.insert_facet(covered);
    check(
        coverage.threshold() == 3U &&
            coverage.coverage_size_at_least_threshold() &&
            result.audit.min_cluster_size_is_downstream_view_only &&
            result.audit
                .complete_equal_level_batch_required_before_cluster_size_view &&
            !result.audit.source_pruning_by_min_cluster_size_performed,
        "min_cluster_size is an exact capped downstream view, never a source prune");
  }
}

void test_scalar_policy_rejections_do_not_consume_authorities() {
  const CanonicalPointCloud cloud = regular_tetrahedron();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto higher = higher_budget();

  auto wrong_order_pairs = pair_authority(index, cloud, 5U);
  auto wrong_order_higher = higher_authority(index, cloud, 5U, higher);
  const auto wrong_order = morsehgp3d::hierarchy::
      build_exact_direct_morse_terminal_reducer_source_bridge(
          index,
          cloud,
          4U,
          higher,
          seed_budget(),
          2U,
          std::move(wrong_order_pairs),
          std::move(wrong_order_higher));
  check(
      !wrong_order.certified() && wrong_order.bridge == nullptr &&
          wrong_order.audit.decision ==
              ExactDirectMorseTerminalReducerSourceBridgeDecision::
                  no_bridge_requested_order_not_five_or_ten &&
          wrong_order_pairs.sealed_in_process_terminal_authority() &&
          wrong_order_higher.sealed_in_process_terminal_authority(),
      "an unsupported K fails before consuming either move-only authority");

  auto zero_size_pairs = pair_authority(index, cloud, 5U);
  auto zero_size_higher = higher_authority(index, cloud, 5U, higher);
  const auto zero_size = morsehgp3d::hierarchy::
      build_exact_direct_morse_terminal_reducer_source_bridge(
          index,
          cloud,
          5U,
          higher,
          seed_budget(),
          0U,
          std::move(zero_size_pairs),
          std::move(zero_size_higher));
  check(
      !zero_size.certified() && zero_size.bridge == nullptr &&
          zero_size.audit.decision ==
              ExactDirectMorseTerminalReducerSourceBridgeDecision::
                  no_bridge_min_cluster_size_zero &&
          zero_size_pairs.sealed_in_process_terminal_authority() &&
          zero_size_higher.sealed_in_process_terminal_authority(),
      "min_cluster_size=0 fails before consuming either authority");
}

void test_incomplete_scientific_sources_never_expose_provider() {
  const auto higher = higher_budget();
  {
    const CanonicalPointCloud cloud = regular_tetrahedron();
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    auto pairs = pair_authority(index, cloud, 5U);
    auto revoked_higher = higher_authority(index, cloud, 5U, higher);
    static_cast<void>(std::move(revoked_higher).release_segments());
    const auto rejected = morsehgp3d::hierarchy::
        build_exact_direct_morse_terminal_reducer_source_bridge(
            index,
            cloud,
            5U,
            higher,
            seed_budget(),
            2U,
            std::move(pairs),
            std::move(revoked_higher));
    check(
        !rejected.certified() && rejected.bridge == nullptr &&
            rejected.audit.decision ==
                ExactDirectMorseTerminalReducerSourceBridgeDecision::
                    no_bridge_higher_terminal_authority_not_sealed &&
            pairs.sealed_in_process_terminal_authority(),
        "a revoked higher-support authority fails closed before facade access");
  }

  {
    const CanonicalPointCloud cloud = right_triangle();
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    auto pairs = pair_authority(index, cloud, 5U);
    auto higher_supports = higher_authority(index, cloud, 5U, higher);
    const auto rejected = morsehgp3d::hierarchy::
        build_exact_direct_morse_terminal_reducer_source_bridge(
            index,
            cloud,
            5U,
            higher,
            seed_budget(),
            2U,
            std::move(pairs),
            std::move(higher_supports));
    check(
        !rejected.certified() && rejected.bridge == nullptr &&
            rejected.audit.all_support_arities_two_through_four_terminal &&
            !rejected.audit.no_relevant_extra_shell_diagnostics &&
            rejected.audit.decision ==
                ExactDirectMorseTerminalReducerSourceBridgeDecision::
                    no_bridge_relevant_extra_shell_diagnostics,
        "a terminal catalogue with a relevant extra shell exposes no reducer source");
  }

  {
    const CanonicalPointCloud cloud = regular_tetrahedron();
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    auto pairs = pair_authority(index, cloud, 5U);
    auto higher_supports = higher_authority(index, cloud, 5U, higher);
    const ExactDirectSaddleArmSeedBudget zero_seed_budget{};
    const auto rejected = morsehgp3d::hierarchy::
        build_exact_direct_morse_terminal_reducer_source_bridge(
            index,
            cloud,
            5U,
            higher,
            zero_seed_budget,
            2U,
            std::move(pairs),
            std::move(higher_supports));
    check(
        !rejected.certified() && rejected.bridge == nullptr &&
            rejected.audit.event_journal_certified &&
            !rejected.audit.saddle_arm_seed_journal_certified &&
            rejected.audit.decision ==
                ExactDirectMorseTerminalReducerSourceBridgeDecision::
                    no_bridge_saddle_arm_seed_journal_not_certified,
        "an exhausted seed budget exposes no partial reducer source");
  }
}

void test_certified_bridge_reduces_through_live_commits_to_forest() {
  const CanonicalPointCloud cloud = uniform_latin_six();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto higher = higher_budget();
  auto pairs = pair_authority(index, cloud, 5U);
  auto higher_supports = higher_authority(index, cloud, 5U, higher);
  auto bridge_result = morsehgp3d::hierarchy::
      build_exact_direct_morse_terminal_reducer_source_bridge(
          index,
          cloud,
          5U,
          higher,
          seed_budget(),
          2U,
          std::move(pairs),
          std::move(higher_supports));
  check(
      bridge_result.certified() && bridge_result.bridge != nullptr,
      "the forest integration fixture closes its terminal source bridge");
  if (bridge_result.bridge == nullptr) {
    return;
  }

  const auto reduced = morsehgp3d::hierarchy::
      reduce_exact_direct_morse_terminal_source_to_forest(
          index,
          cloud,
          *bridge_result.bridge,
          forest_reduction_config());
  if (!reduced.certified_reduction()) {
    std::cerr
        << "forest integration diagnostic: decision="
        << static_cast<unsigned>(reduced.audit.decision)
        << ", plan=" << static_cast<unsigned>(reduced.audit.plan_decision)
        << ", execution="
        << static_cast<unsigned>(
               reduced.audit.last_batch_execution_decision)
        << ", preparation="
        << static_cast<unsigned>(reduced.audit.last_preparation_decision)
        << ", live="
        << static_cast<unsigned>(reduced.audit.last_live_commit_decision)
        << ", fold="
        << static_cast<unsigned>(reduced.audit.last_reducer_fold_decision)
        << ", batches=" << reduced.audit.source_batch_count
        << ", prepared=" << reduced.audit.prepared_ticket_count
        << ", committed=" << reduced.audit.committed_batch_count
        << ", executor_complete=" << reduced.audit.executor_complete
        << ", reducer_complete=" << reduced.audit.reducer_complete
        << ", forest=" << reduced.audit.forest_materialized
        << ", forest_certified="
        << reduced.audit.forest_conditional_h0_candidate_certified << '\n';
  }
  check(
      reduced.certified_reduction() && reduced.forest.has_value() &&
          reduced.audit.provider_constructor_used &&
          reduced.audit.source_batch_count != 0U &&
          reduced.audit.prepared_ticket_count ==
              reduced.audit.source_batch_count &&
          reduced.audit.committed_batch_count ==
              reduced.audit.source_batch_count &&
          reduced.audit.executor_complete && reduced.audit.reducer_complete &&
          reduced.audit.hierarchy_reduction_performed &&
          !reduced.audit.forbidden_global_structure_materialized &&
          !reduced.audit.public_status_claimed &&
          reduced.audit.decision ==
              ExactDirectMorseTerminalForestReductionDecision::
                  complete_conditional_h0_forest,
      "the bridge feeds 14C/14H live commits and materializes one certified conditional H0 forest");

  if (reduced.forest.has_value()) {
    const std::span<
        const morsehgp3d::hierarchy::ExactDirectMorseVerticalTargetProposal>
        no_target_proposals{};
    const auto vertical = morsehgp3d::hierarchy::
        build_exact_direct_morse_vertical_journal(
            *reduced.forest,
            no_target_proposals,
            vertical_budget(),
            morsehgp3d::hierarchy::ExactDirectMorseVerticalConfig{
                UINT64_C(0x15D11A)});
    check(
        vertical.certified_conditional_vertical_candidate() &&
            vertical.counters.missing_label_count ==
                vertical.counters.expected_label_count &&
            vertical.counters.unresolved_label_count == 0U &&
            vertical.counters.resolved_label_count == 0U &&
            !vertical.external_target_authority_replayed &&
            !vertical.all_naturality_squares_replayed &&
            !vertical.vertical_maps_complete &&
            !vertical.public_status_claimed,
        "the real reduced forest yields an explicit bounded vertical target worklist without invented authority");
  }

  auto rejected_config = forest_reduction_config();
  rejected_config.plan_budget.maximum_source_batch_count = 0U;
  const auto rejected = morsehgp3d::hierarchy::
      reduce_exact_direct_morse_terminal_source_to_forest(
          index, cloud, *bridge_result.bridge, rejected_config);
  check(
      !rejected.certified_reduction() && !rejected.forest.has_value() &&
          !rejected.audit.hierarchy_reduction_performed &&
          rejected.audit.decision ==
              ExactDirectMorseTerminalForestReductionDecision::
                  no_reduction_source_plan_rejected,
      "a rejected source plan publishes no partial forest");
}

}  // namespace

int main() {
  test_supported_orders_produce_complete_provider();
  test_scalar_policy_rejections_do_not_consume_authorities();
  test_incomplete_scientific_sources_never_expose_provider();
  test_certified_bridge_reduces_through_live_commits_to_forest();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "direct terminal-to-reducer source bridge tests passed\n";
  return 0;
}
