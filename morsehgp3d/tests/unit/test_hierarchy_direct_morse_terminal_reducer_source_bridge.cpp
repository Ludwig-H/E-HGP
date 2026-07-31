#include "morsehgp3d/hierarchy/direct_morse_terminal_reducer_source_bridge.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::
    ExactDirectMorseForestSourceBatchVisitDecision;
using morsehgp3d::hierarchy::ExactDirectMorseTerminalReducerSourceBridgeDecision;
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
using morsehgp3d::spatial::MortonLbvhIndex;

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

    auto coverage =
        result.bridge->make_downstream_complete_component_coverage();
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

}  // namespace

int main() {
  test_supported_orders_produce_complete_provider();
  test_scalar_policy_rejections_do_not_consume_authorities();
  test_incomplete_scientific_sources_never_expose_provider();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "direct terminal-to-reducer source bridge tests passed\n";
  return 0;
}
