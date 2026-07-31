#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/exact_pair_block_to_direct_pair_terminal.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/anchored_pair_candidate_classifier.hpp"
#include "morsehgp3d/hierarchy/direct_morse_terminal_reducer_source_bridge.hpp"
#include "morsehgp3d/hierarchy/exact_block_rank_prune_receipt.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::ExactPairBlockToDirectPairTerminalBudget;
using morsehgp3d::gpu::ExactPairBlockToDirectPairTerminalStatus;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierResidentCudaConfig;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierResidentCudaContext;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::hierarchy::ExactAnchoredPairCandidateClassificationBudget;
using morsehgp3d::hierarchy::ExactAnchoredPairCandidateClassificationContext;
using morsehgp3d::hierarchy::ExactAnchoredPairCandidateClassificationStepKind;
using morsehgp3d::hierarchy::ExactDirectPairTerminalRecord;
using morsehgp3d::hierarchy::ExactDirectSaddleArmSeedBudget;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
using morsehgp3d::hierarchy::ExactHigherSupportTerminalRunStatus;
using morsehgp3d::hierarchy::ExactHigherSupportTerminalSession;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] CanonicalPointCloud line_cloud(std::size_t count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(count);
  for (std::size_t index = 0U; index < count; ++index) {
    const double coordinate = static_cast<double>(index);
    points.push_back(CertifiedPoint3::from_binary64(
        coordinate, coordinate / 16.0, coordinate / 256.0));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud regular_tetrahedron() {
  const std::array<CertifiedPoint3, 4U> points{
      CertifiedPoint3::from_binary64(1.0, 1.0, 1.0),
      CertifiedPoint3::from_binary64(1.0, -1.0, -1.0),
      CertifiedPoint3::from_binary64(-1.0, 1.0, -1.0),
      CertifiedPoint3::from_binary64(-1.0, -1.0, 1.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud moment_curve_cloud() {
  std::vector<CertifiedPoint3> points;
  points.reserve(12U);
  for (std::size_t index = 0U; index < 12U; ++index) {
    const double value = static_cast<double>(index);
    points.push_back(CertifiedPoint3::from_binary64(
        value, value * value, value * value * value));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_higher_budget() {
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

[[nodiscard]] ExactDirectSaddleArmSeedBudget unlimited_seed_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
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
    throw std::runtime_error(
        "the higher-support integration fixture did not terminate");
  }
  return std::move(session).seal();
}

[[nodiscard]] std::size_t node_for_range(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t leaf_begin,
    std::size_t leaf_end) {
  for (std::size_t node_index = 0U;
       node_index < index.build_counters().node_count;
       ++node_index) {
    const auto probe = morsehgp3d::hierarchy::
        certify_exact_block_rank_prune_receipt(
            index,
            cloud,
            node_index,
            node_index,
            std::span<const std::size_t>{},
            2U);
    if (probe.status() == morsehgp3d::hierarchy::
                              ExactBlockRankPruneStatus::
                                  rejected_support_overlap &&
        probe.audit().support_nodes[0].leaf_begin == leaf_begin &&
        probe.audit().support_nodes[0].leaf_end == leaf_end) {
      return node_index;
    }
  }
  throw std::runtime_error("the requested exact LBVH range was not found");
}

[[nodiscard]] morsehgp3d::gpu::ExactPairBlockNodeAuthority node_authority(
    std::size_t node_index,
    std::size_t leaf_begin,
    std::size_t leaf_end) {
  return {node_index, leaf_begin, leaf_end};
}

[[nodiscard]] std::array<
    ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe,
    2U>
mixed_recipes(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud) {
  const std::size_t first_support = node_for_range(index, cloud, 0U, 2U);
  const std::size_t second_support = node_for_range(index, cloud, 12U, 14U);
  ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe certified;
  certified.source = {
      morsehgp3d::gpu::ExactPairBlockAuthorityKind::cross,
      node_authority(first_support, 0U, 2U),
      node_authority(second_support, 12U, 14U),
      4U};
  certified.witness_node_count = 10U;
  for (std::size_t witness_index = 0U;
       witness_index < certified.witness_node_count;
       ++witness_index) {
    certified.witness_node_indices[witness_index] =
        node_for_range(
            index,
            cloud,
            witness_index + 2U,
            witness_index + 3U);
  }

  const std::size_t root_left = node_for_range(index, cloud, 0U, 8U);
  const std::size_t root_right = node_for_range(index, cloud, 8U, 16U);
  ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe fail_open;
  fail_open.source = {
      morsehgp3d::gpu::ExactPairBlockAuthorityKind::cross,
      node_authority(root_left, 0U, 8U),
      node_authority(root_right, 8U, 16U),
      64U};
  return {certified, fail_open};
}

[[nodiscard]] auto run_mixed_cut(
    MortonLbvhBuildContext& builder,
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    morsehgp3d::gpu::MortonLbvhDeviceTraversalLease lease) {
  const auto recipes = mixed_recipes(index, cloud);
  auto context = ExactPairBlockTransactionalFrontierResidentCudaContext::start(
      std::move(lease),
      index,
      cloud,
      ExactPairBlockTransactionalFrontierResidentCudaConfig{
          11U, 16U, 8U, 16U, 4U});
  (void)builder;
  return context.run(recipes);
}

[[nodiscard]] auto run_complete_tetrahedron_cut(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    morsehgp3d::gpu::MortonLbvhDeviceTraversalLease lease) {
  auto context = ExactPairBlockTransactionalFrontierResidentCudaContext::start(
      std::move(lease),
      index,
      cloud,
      ExactPairBlockTransactionalFrontierResidentCudaConfig{
          4U, 16U, 8U, 16U, 4U});
  return context.run({});
}

[[nodiscard]] auto run_complete_unpruned_cut(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    morsehgp3d::gpu::MortonLbvhDeviceTraversalLease lease) {
  auto context = ExactPairBlockTransactionalFrontierResidentCudaContext::start(
      std::move(lease),
      index,
      cloud,
      ExactPairBlockTransactionalFrontierResidentCudaConfig{
          maximum_closed_rank, 16U, 8U, 16U, 4U});
  return context.run({});
}

struct ExpectedRecords {
  std::vector<ExactDirectPairTerminalRecord> records;
  std::size_t node_visit_count{};
  std::size_t point_id_reference_count{};
  std::size_t above_rank_count{};
};

[[nodiscard]] ExpectedRecords independently_classify_terminals(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::span<const morsehgp3d::gpu::
                        ExactPairBlockTransactionalFrontierCudaTerminalPair>
        terminals) {
  ExpectedRecords expected;
  expected.records.reserve(terminals.size());
  for (const auto& terminal : terminals) {
    auto context = ExactAnchoredPairCandidateClassificationContext::start(
        index, cloud, terminal.point_ids, 11U);
    const auto step = context.advance(
        index,
        cloud,
        ExactAnchoredPairCandidateClassificationBudget{
            std::numeric_limits<std::size_t>::max()});
    expected.node_visit_count += step.work.node_visit_count;
    if (step.kind ==
        ExactAnchoredPairCandidateClassificationStepKind::above_rank) {
      ++expected.above_rank_count;
      continue;
    }
    require(
        step.kind ==
                ExactAnchoredPairCandidateClassificationStepKind::
                    record_ready &&
            context.record_ready(),
        "the independent classifier must close every terminal pair");
    const auto requirements = context.pending_record_requirements();
    require(
        requirements.has_value(),
        "the independent classifier must expose atomic output requirements");
    expected.point_id_reference_count +=
        requirements->point_id_reference_count;
    auto result = context.take_result(index, cloud);
    if (result.event.has_value()) {
      expected.records.emplace_back(std::move(*result.event));
    } else {
      require(
          result.relevant_extra_shell_diagnostic.has_value(),
          "the independent classifier returned no terminal record");
      expected.records.emplace_back(
          std::move(*result.relevant_extra_shell_diagnostic));
    }
  }
  return expected;
}

void test_complete_atomic_adapter() {
  morsehgp3d::gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = line_cloud(16U);
  MortonLbvhBuildContext builder{18U};
  auto build = builder.build(cloud);
  const MortonLbvhIndex& index = build.certified_index();
  auto source = run_mixed_cut(
      builder,
      index,
      cloud,
      builder.release_device_traversal_lease(build));
  require(
      source.complete() && source.validated_for(index, cloud) &&
          source.audit().pruned_unordered_pair_mass == 4U &&
          source.terminal_pairs().size() == 116U,
      "the adapter fixture must expose the authenticated 4+116 cut");
  const ExpectedRecords expected = independently_classify_terminals(
      index, cloud, source.terminal_pairs());

  auto attempt = morsehgp3d::gpu::
      build_exact_pair_block_to_direct_pair_terminal(
          index,
          cloud,
          10U,
          ExactPairBlockToDirectPairTerminalBudget::unlimited(),
          std::move(source));
  require(
      attempt.complete() &&
          attempt.status == ExactPairBlockToDirectPairTerminalStatus::complete,
      "the complete pair cut must produce one sealed neutral authority");
  const auto& audit = attempt.audit;
  require(
      audit.unordered_pair_universe_size == 120U &&
          audit.pruned_unordered_pair_mass == 4U &&
          audit.source_terminal_pair_count == 116U &&
          audit.classification_started_count == 116U &&
          audit.classification_terminal_count == 116U &&
          audit.above_rank_count == expected.above_rank_count &&
          audit.emitted_record_count == expected.records.size() &&
          audit.classification_node_visit_count ==
              expected.node_visit_count &&
          audit.emitted_point_id_reference_count ==
              expected.point_id_reference_count &&
          audit.host_fake_execution && !audit.qualified_cuda_execution &&
          audit.no_forbidden_global_structure_materialized &&
          !audit.global_pair_matrix_materialized &&
          !audit.ordinary_or_higher_order_delaunay_materialized &&
          !audit.global_facet_coface_or_incidence_arena_materialized &&
          !audit.hierarchy_reduction_performed &&
          !audit.public_status_claimed,
      "the neutral authority audit must preserve the exact cut and output identities");
  require(
      attempt.authority->bound_to(index, cloud, 10U) &&
          attempt.authority->records().size() == expected.records.size() &&
          std::equal(
              attempt.authority->records().begin(),
              attempt.authority->records().end(),
              expected.records.begin(),
              expected.records.end()),
      "the adapter output must equal an independent exact classification record for record");

  auto hostile_exhaustion_audit = audit;
  hostile_exhaustion_audit.classification_budget_exhaustion_count = 1U;
  require(
      !hostile_exhaustion_audit.terminal_identities_hold(),
      "a sealed terminal audit must reject any claimed budget exhaustion");

  require(
      !expected.records.empty(),
      "the semantic line fixture must exercise rank-relevant records");
  auto second_build = builder.build(cloud);
  const MortonLbvhIndex& second_index = second_build.certified_index();
  auto bounded_source = run_mixed_cut(
      builder,
      second_index,
      cloud,
      builder.release_device_traversal_lease(second_build));
  ExactPairBlockToDirectPairTerminalBudget bounded_budget =
      ExactPairBlockToDirectPairTerminalBudget::unlimited();
  bounded_budget.maximum_emitted_record_count =
      expected.records.size() - 1U;
  auto rejected = morsehgp3d::gpu::
      build_exact_pair_block_to_direct_pair_terminal(
          second_index,
          cloud,
          10U,
          bounded_budget,
          std::move(bounded_source));
  require(
      rejected.status == ExactPairBlockToDirectPairTerminalStatus::
                             output_record_capacity_exhausted &&
          !rejected.complete() && rejected.authority == nullptr &&
          !rejected.audit.complete &&
          !rejected.audit.retained_records_certified,
      "one missing output slot must reject atomically without publishing a prefix authority");
}

void test_facade_composition_and_product_provenance_gate() {
  morsehgp3d::gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = regular_tetrahedron();
  MortonLbvhBuildContext builder{4U};
  const ExactHigherSupportStreamBudget higher_budget =
      unlimited_higher_budget();

  for (const std::size_t requested_order : {5U, 10U}) {
    auto build = builder.build(cloud);
    const MortonLbvhIndex& index = build.certified_index();
    auto cut = run_complete_tetrahedron_cut(
        index,
        cloud,
        builder.release_device_traversal_lease(build));
    auto pair_attempt = morsehgp3d::gpu::
        build_exact_pair_block_to_direct_pair_terminal(
            index,
            cloud,
            requested_order,
            ExactPairBlockToDirectPairTerminalBudget::unlimited(),
            std::move(cut));
    require(
        pair_attempt.complete(),
        "the tetrahedron pair cut must seal for both supported orders");
    auto higher = higher_authority(
        index, cloud, requested_order, higher_budget);
    auto facade =
        morsehgp3d::hierarchy::build_exact_direct_support_terminal_facade(
            index,
            cloud,
            requested_order,
            higher_budget,
            std::move(*pair_attempt.authority),
            std::move(higher));
    require(
        facade.terminal_catalog_certified() &&
            facade.certificate.pair_source_kind ==
                morsehgp3d::hierarchy::ExactDirectSupportPairSourceKind::
                    sealed_transactional_pair_terminal_authority &&
            facade.certificate.pair_transactional_audit.host_fake_execution &&
            facade.certificate.pair_terminal_authority_consumed &&
            facade.certificate.pair_terminal_records_captured_once &&
            facade.certificate.higher_terminal_authority_consumed &&
            facade.certificate.higher_terminal_records_captured_once &&
            facade.relevant_extra_shell_diagnostics.empty(),
        "the neutral pair authority and independent higher authority must close the direct catalogue for K=5 and K=10");
  }

  auto build = builder.build(cloud);
  const MortonLbvhIndex& index = build.certified_index();
  auto cut = run_complete_tetrahedron_cut(
      index,
      cloud,
      builder.release_device_traversal_lease(build));
  auto pair_attempt = morsehgp3d::gpu::
      build_exact_pair_block_to_direct_pair_terminal(
          index,
          cloud,
          10U,
          ExactPairBlockToDirectPairTerminalBudget::unlimited(),
          std::move(cut));
  require(
      pair_attempt.complete(),
      "the product provenance rejection fixture requires a sealed pair authority");
  auto higher = higher_authority(index, cloud, 10U, higher_budget);
  auto rejected = morsehgp3d::hierarchy::
      build_exact_direct_morse_terminal_reducer_source_bridge(
          index,
          cloud,
          10U,
          higher_budget,
          unlimited_seed_budget(),
          2U,
          std::move(*pair_attempt.authority),
          std::move(higher));
  require(
      !rejected.certified() && rejected.bridge == nullptr &&
          rejected.audit.decision ==
              morsehgp3d::hierarchy::
                  ExactDirectMorseTerminalReducerSourceBridgeDecision::
                      no_bridge_pair_terminal_authority_not_qualified_cuda &&
          pair_attempt.authority->sealed_in_process_terminal_authority() &&
          higher.sealed_in_process_terminal_authority(),
      "the production bridge must reject a host/fake pair provenance before consuming either authority");
}

void test_full_requested_rank_windows_on_generic_twelve_point_cloud() {
  morsehgp3d::gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = moment_curve_cloud();
  MortonLbvhBuildContext builder{cloud.size()};
  const ExactHigherSupportStreamBudget higher_budget =
      unlimited_higher_budget();

  for (const std::size_t requested_order : {5U, 10U}) {
    const std::size_t maximum_closed_rank = requested_order + 1U;
    auto build = builder.build(cloud);
    const MortonLbvhIndex& index = build.certified_index();
    auto cut = run_complete_unpruned_cut(
        index,
        cloud,
        maximum_closed_rank,
        builder.release_device_traversal_lease(build));
    auto pair_attempt = morsehgp3d::gpu::
        build_exact_pair_block_to_direct_pair_terminal(
            index,
            cloud,
            requested_order,
            ExactPairBlockToDirectPairTerminalBudget::unlimited(),
            std::move(cut));
    require(
        pair_attempt.complete() &&
            pair_attempt.audit.maximum_closed_rank == maximum_closed_rank,
        "the generic fixture must exercise the complete requested rank-six and rank-eleven pair lanes");
    auto higher = higher_authority(
        index, cloud, requested_order, higher_budget);
    auto facade =
        morsehgp3d::hierarchy::build_exact_direct_support_terminal_facade(
            index,
            cloud,
            requested_order,
            higher_budget,
            std::move(*pair_attempt.authority),
            std::move(higher));
    require(
        facade.terminal_catalog_certified() &&
            facade.relevant_extra_shell_diagnostics.empty() &&
            facade.certificate.requirements.maximum_relevant_closed_rank ==
                maximum_closed_rank,
        "the generic twelve-point fixture must close direct supports at rank six and rank eleven");
  }
}

}  // namespace

int main() {
  try {
    test_complete_atomic_adapter();
    test_facade_composition_and_product_provenance_gate();
    test_full_requested_rank_windows_on_generic_twelve_point_cloud();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "resident pair-cut to neutral terminal authority tests passed\n";
  return 0;
}
