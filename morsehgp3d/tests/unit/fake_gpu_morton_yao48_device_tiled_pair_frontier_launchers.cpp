#include "fake_gpu_morton_yao48_device_tiled_pair_frontier_launchers.hpp"

#include "phase15_morton_yao48_device_tiled_pair_frontier_internal.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::test_support {
namespace {

std::mutex fake_mutex;
FakeMortonYao48DeviceTiledPairFrontierConfiguration fake_configuration;
std::shared_ptr<void> held_shared_output_owner;
std::atomic<std::size_t> fake_launch_count{0U};

}  // namespace

void configure_fake_gpu_morton_yao48_device_tiled_pair_frontier(
    FakeMortonYao48DeviceTiledPairFrontierConfiguration configuration) {
  std::lock_guard<std::mutex> lock{fake_mutex};
  fake_configuration = std::move(configuration);
  held_shared_output_owner.reset();
}

void reset_fake_gpu_morton_yao48_device_tiled_pair_frontier() noexcept {
  std::lock_guard<std::mutex> lock{fake_mutex};
  fake_configuration = {};
  held_shared_output_owner.reset();
  fake_launch_count.store(0U, std::memory_order_relaxed);
}

std::size_t
fake_gpu_morton_yao48_device_tiled_pair_frontier_launch_count() noexcept {
  return fake_launch_count.load(std::memory_order_relaxed);
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {
namespace {

using Corruption = test_support::
    FakeMortonYao48DeviceTiledPairFrontierCorruption;
using FakeAnchor = test_support::FakeMortonYao48DeviceTiledAnchor;
using FakeConfiguration = test_support::
    FakeMortonYao48DeviceTiledPairFrontierConfiguration;

[[nodiscard]] std::size_t checked_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::length_error(message);
  }
  return left * right;
}

[[nodiscard]] FakeConfiguration current_configuration() {
  std::lock_guard<std::mutex> lock{test_support::fake_mutex};
  return test_support::fake_configuration;
}

void retain_shared_output_owner(const std::shared_ptr<void>& owner) {
  std::lock_guard<std::mutex> lock{test_support::fake_mutex};
  test_support::held_shared_output_owner = owner;
}

}  // namespace

Phase15MortonYao48DeviceTiledAdoptedTraversal
adopt_phase15_morton_yao48_device_tiled_traversal(
    MortonLbvhDeviceTraversalLease&& traversal_lease) {
  if (!traversal_lease.ready() || traversal_lease.cuda_resident() ||
      !traversal_lease.audit().host_fake_lifecycle_exercised) {
    throw std::invalid_argument(
        "the fake Phase 15 adoption requires a ready host-fake traversal "
        "lease");
  }
  const MortonLbvhDeviceTraversalLeaseAudit audit =
      traversal_lease.audit();
  auto owner = std::make_shared<MortonLbvhDeviceTraversalLease>(
      std::move(traversal_lease));

  Phase15MortonYao48DeviceTiledAdoptedTraversal adopted;
  adopted.retained_owner = owner;
  adopted.source_cloud_identity = owner->source_cloud_identity_;
  adopted.point_count = audit.point_count;
  adopted.certified_node_count = audit.certified_node_count;
  adopted.maximum_point_count = audit.maximum_point_count;
  adopted.maximum_node_count = audit.maximum_node_count;
  adopted.retained_coordinate_word_capacity =
      audit.retained_coordinate_word_capacity;
  adopted.retained_morton_point_id_capacity =
      audit.retained_morton_point_id_capacity;
  adopted.retained_node_capacity = audit.retained_node_capacity;
  adopted.source_snapshot_epoch = audit.source_snapshot_epoch;
  adopted.execution_kind =
      Phase15MortonYao48DeviceTiledExecutionKind::host_fake;
  adopted.canonical_coordinate_words_retained = true;
  adopted.active_morton_point_ids_retained = true;
  adopted.certified_device_nodes_retained = true;
  adopted.host_fake_lifecycle_exercised = true;
  adopted.cuda_device_storage_retained = false;
  return adopted;
}

Phase15MortonYao48DeviceTiledBatch
build_phase15_morton_yao48_device_tiled_pair_frontier_on_device(
    Phase15MortonYao48DeviceTiledPairFrontierContextState& context,
    const Phase15MortonYao48DeviceTiledAdoptedTraversal& traversal,
    const Phase15MortonYao48DeviceTiledRequest& request) {
  (void)context;
  test_support::fake_launch_count.fetch_add(
      1U, std::memory_order_relaxed);
  const FakeConfiguration configuration = current_configuration();
  if (configuration.corruption == Corruption::simulated_launcher_failure) {
    throw std::runtime_error(
        "simulated Phase 15 device tiled launcher failure");
  }
  if (!traversal.retained_owner || !traversal.source_cloud_identity ||
      traversal.execution_kind !=
          Phase15MortonYao48DeviceTiledExecutionKind::host_fake ||
      !traversal.host_fake_lifecycle_exercised ||
      traversal.cuda_device_storage_retained ||
      traversal.device_coordinate_bits != nullptr ||
      traversal.device_morton_point_ids != nullptr ||
      traversal.device_nodes != nullptr || traversal.cuda_device != -1 ||
      request.source_snapshot_epoch != traversal.source_snapshot_epoch ||
      request.point_count != traversal.point_count ||
      request.certified_node_count != traversal.certified_node_count ||
      request.anchor_begin == 0U || request.anchor_count == 0U ||
      request.anchor_begin + request.anchor_count > request.point_count ||
      request.maximum_closed_rank < 2U ||
      request.maximum_closed_rank >
          morton_yao48_device_tiled_pair_frontier_maximum_closed_rank ||
      request.node_visit_capacity_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_node_visits_per_anchor ||
      request.candidate_capacity_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_candidates_per_anchor ||
      request.prune_region_capacity_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor ||
      request.witness_bank_count_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_witness_bank_count ||
      request.witness_slot_count_per_bank !=
          request.maximum_closed_rank - 1U ||
      request.output_buffer_epoch == 0U ||
      (!configuration.anchors.empty() &&
       configuration.anchors.size() != request.anchor_count)) {
    throw std::invalid_argument(
        "the fake Phase 15 device tiled launcher received an invalid "
        "request or traversal authority");
  }

  Phase15MortonYao48DeviceTiledBatch batch;
  batch.retained_output_owner = std::make_shared<std::uint64_t>(
      request.output_buffer_epoch);
  batch.source_cloud_identity_authority =
      traversal.source_cloud_identity;
  batch.host_anchor_controls.reserve(request.anchor_count);
  for (std::size_t control_index = 0U;
       control_index < request.anchor_count;
       ++control_index) {
    const std::size_t anchor_position =
        request.anchor_begin + control_index;
    FakeAnchor script;
    if (configuration.anchors.empty()) {
      script.candidate_count =
          static_cast<std::uint64_t>(anchor_position);
      script.node_visit_count =
          static_cast<std::uint64_t>(anchor_position);
    } else {
      script = configuration.anchors[control_index];
    }
    if (script.candidate_count + script.certified_pruned_pair_mass >
        static_cast<std::uint64_t>(anchor_position)) {
      throw std::invalid_argument(
          "the fake Phase 15 anchor script exceeds its local pair mass");
    }
    Phase15MortonYao48DeviceTiledAnchorControl control;
    control.anchor_morton_position =
        static_cast<std::uint64_t>(anchor_position);
    control.candidate_count = script.candidate_count;
    control.prune_region_count = script.prune_region_count;
    control.certified_pruned_pair_mass =
        script.certified_pruned_pair_mass;
    control.node_visit_count = script.node_visit_count;
    control.status = static_cast<std::uint64_t>(
        script.complete
            ? Phase15MortonYao48DeviceTiledAnchorStatus::complete
            : Phase15MortonYao48DeviceTiledAnchorStatus::censored);
    control.stop_reason =
        static_cast<std::uint64_t>(script.stop_reason);
    control.failure_code = static_cast<std::uint64_t>(
        Phase15MortonYao48DeviceTiledFailureCode::none);
    control.ambiguous_cone_candidate_count =
        script.ambiguous_cone_candidate_count;
    control.unbanked_candidate_count =
        script.unbanked_candidate_count;
    control.unresolved_pair_mass =
        static_cast<std::uint64_t>(anchor_position) -
        script.candidate_count - script.certified_pruned_pair_mass;
    batch.host_anchor_controls.push_back(control);
  }

  batch.physical_candidate_capacity = checked_product(
      request.anchor_count,
      request.candidate_capacity_per_anchor,
      "the fake Phase 15 candidate capacity overflows size_t");
  batch.physical_prune_region_capacity = checked_product(
      request.anchor_count,
      request.prune_region_capacity_per_anchor,
      "the fake Phase 15 prune capacity overflows size_t");
  batch.physical_witness_bank_slot_capacity = checked_product(
      checked_product(
          request.anchor_count,
          request.witness_bank_count_per_anchor,
          "the fake Phase 15 bank count overflows size_t"),
      request.witness_slot_count_per_bank,
      "the fake Phase 15 bank slot capacity overflows size_t");
  batch.physical_anchor_control_capacity = request.anchor_count;
  batch.source_snapshot_epoch = request.source_snapshot_epoch;
  batch.output_buffer_epoch = request.output_buffer_epoch;
  batch.execution_kind =
      Phase15MortonYao48DeviceTiledExecutionKind::host_fake;
  batch.fixed_anchor_segments_allocated = true;
  batch.output_owner_detached_for_tile_lifetime = true;
  batch.interval_cone_classification_requested = true;
  batch.ambiguous_cone_to_unbanked_candidate_requested = true;
  batch.target_tested_before_bank_insert_requested = true;
  batch.retained_witnesses_outside_pruned_subtree_requested = true;
  batch.nonnegative_diametral_witness_interval_lower_bound_requested = true;
  batch.censored_anchor_outputs_invalidated = true;
  switch (configuration.corruption) {
    case Corruption::none:
      break;
    case Corruption::stale_output_epoch:
      --batch.output_buffer_epoch;
      break;
    case Corruption::candidate_device_to_host:
      batch.candidate_device_to_host_count = 1U;
      break;
    case Corruption::forged_cuda_execution:
      batch.execution_kind =
          Phase15MortonYao48DeviceTiledExecutionKind::cuda;
      batch.cuda_execution_contract_satisfied = true;
      batch.cuda_device = 0;
      break;
    case Corruption::missing_output_owner:
      batch.retained_output_owner.reset();
      break;
    case Corruption::shared_output_owner:
      retain_shared_output_owner(batch.retained_output_owner);
      break;
    case Corruption::corrupt_metadata_digest:
      break;
    case Corruption::nonzero_failure_code:
      batch.host_anchor_controls.front().failure_code =
          static_cast<std::uint64_t>(
              Phase15MortonYao48DeviceTiledFailureCode::
                  malformed_traversal);
      break;
    case Corruption::wrong_anchor_position:
      ++batch.host_anchor_controls.front().anchor_morton_position;
      break;
    case Corruption::foreign_source_cloud_identity:
      batch.source_cloud_identity_authority =
          std::make_shared<const std::uint64_t>(UINT64_C(0xbad1d));
      break;
    case Corruption::pruned_mass_without_region:
      batch.host_anchor_controls.front().candidate_count = 0U;
      batch.host_anchor_controls.front().certified_pruned_pair_mass = 1U;
      batch.host_anchor_controls.front().prune_region_count = 0U;
      batch.host_anchor_controls.front().unresolved_pair_mass = 0U;
      break;
    case Corruption::too_many_regions_for_pruned_mass:
      batch.host_anchor_controls.front().candidate_count = 0U;
      batch.host_anchor_controls.front().certified_pruned_pair_mass = 1U;
      batch.host_anchor_controls.front().prune_region_count = 2U;
      batch.host_anchor_controls.front().node_visit_count = 2U;
      batch.host_anchor_controls.front().unresolved_pair_mass = 0U;
      break;
    case Corruption::outputs_exceed_node_visits:
      batch.host_anchor_controls.front().node_visit_count = 0U;
      break;
    case Corruption::simulated_launcher_failure:
      break;
  }
  batch.metadata_digest =
      phase15_morton_yao48_device_tiled_metadata_digest(batch);
  if (configuration.corruption == Corruption::corrupt_metadata_digest) {
    batch.metadata_digest ^= UINT64_C(1);
  }
  return batch;
}

}  // namespace morsehgp3d::gpu::detail
