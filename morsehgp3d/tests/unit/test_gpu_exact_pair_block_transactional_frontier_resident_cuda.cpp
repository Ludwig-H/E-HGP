#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/exact_pair_block_transactional_frontier_resident_cuda.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cstddef>
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
          !audit.cuda_execution_performed,
      "the fake resident ledger must partition every pair exactly once");
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
    test_complete_k10_and_atomic_capacity_rollback();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "resident transactional frontier host/fake tests passed\n";
  return 0;
}
