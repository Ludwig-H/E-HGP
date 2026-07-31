#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/exact_pair_block_transactional_frontier_resident_cuda.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/exact_block_rank_prune_receipt.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierResidentCudaConfig;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierResidentCudaContext;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierResidentCudaPruneRecipe;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierResidentCudaStatus;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

template <typename Index, typename Cloud>
concept CanStartResident = requires(
    MortonLbvhDeviceTraversalLease&& lease,
    Index&& index,
    Cloud&& cloud) {
  ExactPairBlockTransactionalFrontierResidentCudaContext::start(
      std::move(lease),
      std::forward<Index>(index),
      std::forward<Cloud>(cloud));
};

static_assert(
    !std::is_copy_constructible_v<
        ExactPairBlockTransactionalFrontierResidentCudaContext>);
static_assert(
    std::is_nothrow_move_constructible_v<
        ExactPairBlockTransactionalFrontierResidentCudaContext>);
static_assert(CanStartResident<MortonLbvhIndex&, CanonicalPointCloud&>);
static_assert(!CanStartResident<MortonLbvhIndex, CanonicalPointCloud&>);
static_assert(!CanStartResident<MortonLbvhIndex&, CanonicalPointCloud>);

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

void test_mixed_certified_prune_and_fail_open_recipes() {
  morsehgp3d::gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = line_cloud(16U);
  MortonLbvhBuildContext builder{18U};
  auto build = builder.build(cloud);
  const MortonLbvhIndex& index = build.certified_index();
  auto lease = builder.release_device_traversal_lease(build);

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

  const std::array recipes{certified, fail_open};
  auto context = ExactPairBlockTransactionalFrontierResidentCudaContext::start(
      std::move(lease),
      index,
      cloud,
      ExactPairBlockTransactionalFrontierResidentCudaConfig{
          11U, 16U, 8U, 16U, 4U});
  auto result = context.run(recipes);
  const auto& audit = result.audit();
  require(
      result.complete() && result.validated_for(index, cloud) &&
          audit.submitted_prune_recipe_count == 2U &&
          audit.matched_prune_recipe_count == 2U &&
          audit.unused_prune_recipe_count == 0U &&
          audit.exact_prune_attempt_count == 2U &&
          audit.certified_prune_count == 1U &&
          audit.exact_prune_fail_open_count == 1U &&
          audit.recipe_catalog_nonempty && audit.recipe_path_exercised &&
          audit.certified_prune_path_exercised &&
          audit.fail_open_path_exercised &&
          audit.fail_open_native_transition_validated &&
          audit.exact_witness_predicate_count == 10U &&
          audit.exact_negative_witness_predicate_count == 10U &&
          audit.nonnegative_or_residual_witness_predicate_count == 0U &&
          audit.pruned_unordered_pair_mass == 4U &&
          audit.terminal_unordered_pair_mass == 116U &&
          result.prune_receipts().size() == 1U &&
          result.terminal_pairs().size() == 116U,
      "the resident mixed recipe cut must exercise one exact prune and one "
      "scientific fail-open while closing all 120 pairs");
}

void test_complete_k10_and_atomic_capacity_rollback() {
  morsehgp3d::gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = line_cloud(16U);
  MortonLbvhBuildContext builder{18U};
  auto build = builder.build(cloud);
  const MortonLbvhIndex& index = build.certified_index();
  auto lease = builder.release_device_traversal_lease(build);
  auto context = ExactPairBlockTransactionalFrontierResidentCudaContext::start(
      std::move(lease),
      index,
      cloud,
      ExactPairBlockTransactionalFrontierResidentCudaConfig{
          11U, 16U, 8U, 16U, 4U});
  auto result = context.run({});
  require(
      result.status() == ExactPairBlockTransactionalFrontierResidentCudaStatus::
                             non_authoritative_host_fake_terminal &&
          result.complete() && result.validated_for(index, cloud),
      "the fake resident K=10 cut must be complete and replayable");
  const auto& audit = result.audit();
  require(
      result.terminal_pairs().size() == 120U &&
          result.prune_receipts().empty() &&
          audit.unordered_pair_universe_mass == 120U &&
          audit.terminal_unordered_pair_mass == 120U &&
          audit.pending_unordered_pair_mass == 0U &&
          audit.wave_commit_count > 1U &&
          audit.host_fake_lifecycle_exercised &&
          !audit.cuda_execution_performed &&
          !audit.serial_device_reference && !audit.scale_eligible,
      "the fake resident ledger must partition every pair exactly once");
  require(
      !audit.recipe_catalog_nonempty && !audit.recipe_path_exercised &&
          !audit.certified_prune_path_exercised &&
          !audit.fail_open_path_exercised &&
          !audit.fail_open_native_transition_validated,
      "an empty recipe catalogue must not self-attest an exercised prune or "
      "fail-open path");
  require(
      !audit.pair_catalog_complete_claimed &&
          !audit.hierarchy_or_tree_claimed && !audit.slo_claimed &&
          !audit.global_pair_matrix_materialized &&
          !audit.ordinary_or_higher_order_delaunay_materialized &&
          !audit.global_facet_coface_or_incidence_arena_materialized,
      "the architecture-only cut must make no tree, SLO, matrix or Delaunay claim");

  auto second_build = builder.build(cloud);
  const MortonLbvhIndex& second_index = second_build.certified_index();
  auto second_lease = builder.release_device_traversal_lease(second_build);
  auto bounded = ExactPairBlockTransactionalFrontierResidentCudaContext::start(
      std::move(second_lease),
      second_index,
      cloud,
      ExactPairBlockTransactionalFrontierResidentCudaConfig{
          6U, 1U, 1U, 1U, 1U});
  auto rollback = bounded.run({});
  require(
      rollback.status() == ExactPairBlockTransactionalFrontierResidentCudaStatus::
                               capacity_exhausted_wave_rolled_back &&
          !rollback.complete() && !rollback.pending_blocks().empty() &&
          rollback.audit().inflight_unordered_pair_mass == 0U &&
          rollback.audit().wave_rollback_count == 1U &&
          rollback.audit().capacity_wave_rollback_validated &&
          rollback.audit().pending_unordered_pair_mass +
                  rollback.audit().pruned_unordered_pair_mass +
                  rollback.audit().terminal_unordered_pair_mass ==
              rollback.audit().unordered_pair_universe_mass,
      "capacity saturation must restore the whole source wave and conserve mass");
}

}  // namespace

int main() {
  try {
    test_mixed_certified_prune_and_fail_open_recipes();
    test_complete_k10_and_atomic_capacity_rollback();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "resident transactional frontier host/fake tests passed\n";
  return 0;
}
