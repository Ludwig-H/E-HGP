#include "fake_gpu_morton_yao48_device_tiled_pair_frontier_launchers.hpp"

#include "phase15_morton_yao48_device_tiled_pair_frontier_internal.hpp"

#include <algorithm>
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

struct FakeCumulativeAnchor final {
  std::uint64_t candidate_count{};
  std::uint64_t prune_region_count{};
  std::uint64_t certified_pruned_pair_mass{};
  std::uint64_t node_visit_count{};
  std::uint64_t ambiguous_cone_candidate_count{};
  std::uint64_t unbanked_candidate_count{};
  bool complete{false};
};

std::mutex fake_mutex;
FakeMortonYao48DeviceTiledPairFrontierConfiguration fake_configuration;
FakeMortonYao48DeviceTiledAdoptionCorruption fake_adoption_corruption{
    FakeMortonYao48DeviceTiledAdoptionCorruption::none};
std::shared_ptr<void> held_shared_output_owner;
std::vector<FakeCumulativeAnchor> fake_cumulative_anchors;
std::size_t fake_next_transcript{};
std::size_t fake_anchor_begin{};
std::size_t fake_anchor_count{};
std::uint64_t fake_tile_epoch{};
std::uint64_t fake_chunk_sequence{};
std::atomic<std::size_t> fake_launch_count{0U};

}  // namespace

void configure_fake_gpu_morton_yao48_device_tiled_pair_frontier(
    FakeMortonYao48DeviceTiledPairFrontierConfiguration configuration) {
  std::lock_guard<std::mutex> lock{fake_mutex};
  fake_configuration = std::move(configuration);
  held_shared_output_owner.reset();
  fake_cumulative_anchors.clear();
  fake_next_transcript = 0U;
  fake_anchor_begin = 0U;
  fake_anchor_count = 0U;
  fake_tile_epoch = 0U;
  fake_chunk_sequence = 0U;
}

void configure_fake_gpu_morton_yao48_device_tiled_adoption_corruption(
    FakeMortonYao48DeviceTiledAdoptionCorruption corruption) {
  std::lock_guard<std::mutex> lock{fake_mutex};
  fake_adoption_corruption = corruption;
}

void reset_fake_gpu_morton_yao48_device_tiled_pair_frontier() noexcept {
  std::lock_guard<std::mutex> lock{fake_mutex};
  fake_configuration = {};
  fake_adoption_corruption =
      FakeMortonYao48DeviceTiledAdoptionCorruption::none;
  held_shared_output_owner.reset();
  fake_cumulative_anchors.clear();
  fake_next_transcript = 0U;
  fake_anchor_begin = 0U;
  fake_anchor_count = 0U;
  fake_tile_epoch = 0U;
  fake_chunk_sequence = 0U;
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
using AdoptionCorruption = test_support::
    FakeMortonYao48DeviceTiledAdoptionCorruption;
using FakeAnchor = test_support::FakeMortonYao48DeviceTiledAnchor;
using FakeAnchorStatus = test_support::
    FakeMortonYao48DeviceTiledAnchorStatus;
using FakeConfiguration = test_support::
    FakeMortonYao48DeviceTiledPairFrontierConfiguration;
using FakeLaunch = test_support::
    FakeMortonYao48DeviceTiledPairFrontierLaunch;
using FakeProgress = test_support::FakeCumulativeAnchor;

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

[[nodiscard]] std::uint64_t checked_sum(
    std::uint64_t left,
    std::uint64_t right,
    const char* message) {
  if (right > std::numeric_limits<std::uint64_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_size_sum(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] FakeLaunch next_launch() {
  std::lock_guard<std::mutex> lock{test_support::fake_mutex};
  const FakeConfiguration& configuration =
      test_support::fake_configuration;
  if (configuration.launches.empty()) {
    return FakeLaunch{configuration.anchors, configuration.corruption};
  }
  if (test_support::fake_next_transcript >=
      configuration.launches.size()) {
    throw std::runtime_error(
        "the fake Phase 15 launcher transcript is exhausted");
  }
  return configuration.launches[test_support::fake_next_transcript++];
}

[[nodiscard]] AdoptionCorruption adoption_corruption() {
  std::lock_guard<std::mutex> lock{test_support::fake_mutex};
  return test_support::fake_adoption_corruption;
}

[[nodiscard]] std::vector<FakeProgress> begin_continuation(
    const Phase15MortonYao48DeviceTiledRequest& request) {
  std::lock_guard<std::mutex> lock{test_support::fake_mutex};
  if (!request.resume_same_tile) {
    if (request.chunk_sequence != 1U) {
      throw std::invalid_argument(
          "a new fake Phase 15 tile did not start at chunk one");
    }
    test_support::fake_anchor_begin = request.anchor_begin;
    test_support::fake_anchor_count = request.anchor_count;
    test_support::fake_tile_epoch = request.tile_epoch;
    test_support::fake_chunk_sequence = request.chunk_sequence;
    test_support::fake_cumulative_anchors.assign(
        request.anchor_count, FakeProgress{});
  } else if (
      request.anchor_begin != test_support::fake_anchor_begin ||
      request.anchor_count != test_support::fake_anchor_count ||
      request.tile_epoch != test_support::fake_tile_epoch ||
      request.chunk_sequence != test_support::fake_chunk_sequence + 1U ||
      test_support::fake_cumulative_anchors.size() != request.anchor_count) {
    throw std::invalid_argument(
        "a resumed fake Phase 15 tile changed its identity or sequence");
  } else {
    test_support::fake_chunk_sequence = request.chunk_sequence;
  }
  return test_support::fake_cumulative_anchors;
}

void commit_continuation(std::vector<FakeProgress> progress) {
  std::lock_guard<std::mutex> lock{test_support::fake_mutex};
  test_support::fake_cumulative_anchors = std::move(progress);
}

void retain_shared_output_owner(const std::shared_ptr<void>& owner) {
  std::lock_guard<std::mutex> lock{test_support::fake_mutex};
  test_support::held_shared_output_owner = owner;
}

[[nodiscard]] Phase15MortonYao48DeviceTiledAnchorStatus anchor_status(
    FakeAnchorStatus status) {
  switch (status) {
    case FakeAnchorStatus::active:
      return Phase15MortonYao48DeviceTiledAnchorStatus::active;
    case FakeAnchorStatus::chunk_ready:
      return Phase15MortonYao48DeviceTiledAnchorStatus::chunk_ready;
    case FakeAnchorStatus::complete:
      return Phase15MortonYao48DeviceTiledAnchorStatus::complete;
    case FakeAnchorStatus::fatal:
      return Phase15MortonYao48DeviceTiledAnchorStatus::fatal;
  }
  throw std::invalid_argument("unknown fake Phase 15 anchor status");
}

[[nodiscard]] Phase15MortonYao48DeviceTiledYieldReason yield_reason(
    MortonYao48DeviceTiledPairFrontierYieldReason reason) {
  switch (reason) {
    case MortonYao48DeviceTiledPairFrontierYieldReason::none:
      return Phase15MortonYao48DeviceTiledYieldReason::none;
    case MortonYao48DeviceTiledPairFrontierYieldReason::
        candidate_segment_full:
      return Phase15MortonYao48DeviceTiledYieldReason::
          candidate_segment_full;
    case MortonYao48DeviceTiledPairFrontierYieldReason::
        prune_segment_full:
      return Phase15MortonYao48DeviceTiledYieldReason::prune_segment_full;
    case MortonYao48DeviceTiledPairFrontierYieldReason::
        mixed_segments_full:
      break;
  }
  throw std::invalid_argument(
      "a per-anchor fake Phase 15 yield cannot be mixed");
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
  const MortonLbvhDeviceTraversalLeaseAudit audit = traversal_lease.audit();
  const AdoptionCorruption corruption = adoption_corruption();
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
  adopted.retained_node_bound_view_capacity_bytes = checked_product(
      audit.certified_node_count,
      sizeof(Phase15MortonYao48DeviceTiledNodeBounds),
      "the fake Phase 15 node-bound view extent overflows size_t");
  adopted.resolved_node_bound_count = audit.certified_node_count;
  if (corruption == AdoptionCorruption::wrong_resolved_node_count) {
    adopted.resolved_node_bound_count =
        adopted.resolved_node_bound_count == 0U
            ? 1U
            : adopted.resolved_node_bound_count - 1U;
  }
  adopted.source_snapshot_epoch = audit.source_snapshot_epoch;
  adopted.execution_kind =
      Phase15MortonYao48DeviceTiledExecutionKind::host_fake;
  adopted.canonical_coordinate_words_retained = true;
  adopted.active_morton_point_ids_retained = true;
  adopted.certified_device_nodes_retained = true;
  adopted.node_bound_view_extent_authenticated = true;
  adopted.node_bound_view_validation_sentinel_authenticated =
      corruption != AdoptionCorruption::invalid_validation_sentinel;
  adopted.node_bound_view_bound_to_snapshot_identity = true;
  adopted.node_bound_view_built_once_per_adoption = true;
  adopted.node_bound_view_reused_without_tile_allocation = true;
  adopted.source_node_extremum_point_ids_retained = true;
  adopted.node_bound_view_build_included_in_context_creation = true;
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
  test_support::fake_launch_count.fetch_add(1U, std::memory_order_relaxed);
  const FakeLaunch launch = next_launch();
  if (launch.corruption == Corruption::simulated_launcher_failure) {
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
      traversal.device_nodes != nullptr ||
      traversal.device_node_bounds != nullptr ||
      traversal.resolved_node_bound_count != traversal.certified_node_count ||
      !traversal.node_bound_view_extent_authenticated ||
      !traversal.node_bound_view_validation_sentinel_authenticated ||
      !traversal.node_bound_view_bound_to_snapshot_identity ||
      !traversal.node_bound_view_built_once_per_adoption ||
      !traversal.node_bound_view_reused_without_tile_allocation ||
      !traversal.source_node_extremum_point_ids_retained ||
      !traversal.node_bound_view_build_included_in_context_creation ||
      traversal.node_bound_view_allocation_capacity_bytes != 0U ||
      traversal.node_bound_view_build_allocation_count != 0U ||
      traversal.node_bound_view_build_kernel_launch_count != 0U ||
      traversal.node_bound_view_build_synchronization_count != 0U ||
      traversal.node_bound_view_validation_device_to_host_byte_count != 0U ||
      traversal.cuda_device != -1 ||
      request.source_snapshot_epoch != traversal.source_snapshot_epoch ||
      request.point_count != traversal.point_count ||
      request.certified_node_count != traversal.certified_node_count ||
      request.anchor_begin == 0U || request.anchor_count == 0U ||
      request.anchor_begin + request.anchor_count > request.point_count ||
      request.maximum_closed_rank < 2U ||
      request.maximum_closed_rank >
          morton_yao48_device_tiled_pair_frontier_maximum_closed_rank ||
      !morton_yao48_device_tiled_pair_frontier_prune_semantics_known(
          request.prune_semantics) ||
      request.required_witness_count !=
          morton_yao48_device_tiled_pair_frontier_required_witness_count(
              request.prune_semantics, request.maximum_closed_rank) ||
      request.node_visit_capacity_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_node_visits_per_anchor ||
      request.candidate_capacity_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_candidates_per_anchor ||
      request.prune_region_capacity_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor ||
      request.witness_bank_count_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_witness_bank_count ||
      request.witness_slot_count_per_bank !=
          request.required_witness_count ||
      request.output_buffer_epoch == 0U || request.tile_epoch == 0U ||
      request.chunk_sequence == 0U ||
      (!launch.anchors.empty() &&
       launch.anchors.size() != request.anchor_count)) {
    throw std::invalid_argument(
        "the fake Phase 15 device tiled launcher received an invalid "
        "request or traversal authority");
  }

  std::vector<FakeProgress> progress = begin_continuation(request);
  Phase15MortonYao48DeviceTiledBatch batch;
  batch.retained_output_owner = std::make_shared<std::uint64_t>(
      request.output_buffer_epoch);
  batch.source_cloud_identity_authority = traversal.source_cloud_identity;
  batch.host_anchor_controls.reserve(request.anchor_count);
  std::size_t traversal_subdivision_count = 1U;
  bool any_chunk_ready = false;
  for (std::size_t control_index = 0U;
       control_index < request.anchor_count;
       ++control_index) {
    const std::size_t anchor_position = request.anchor_begin + control_index;
    FakeAnchor script;
    if (launch.anchors.empty()) {
      script.candidate_count = static_cast<std::uint64_t>(anchor_position);
      script.node_visit_count = static_cast<std::uint64_t>(anchor_position);
    } else {
      script = launch.anchors[control_index];
    }
    FakeProgress& prior = progress[control_index];
    const std::uint64_t cumulative_candidate_count = checked_sum(
        prior.candidate_count,
        script.candidate_count,
        "the fake Phase 15 candidate cumulative overflowed");
    const std::uint64_t cumulative_prune_region_count = checked_sum(
        prior.prune_region_count,
        script.prune_region_count,
        "the fake Phase 15 prune cumulative overflowed");
    const std::uint64_t cumulative_pruned_mass = checked_sum(
        prior.certified_pruned_pair_mass,
        script.certified_pruned_pair_mass,
        "the fake Phase 15 pruned mass cumulative overflowed");
    const std::uint64_t cumulative_node_visit_count = checked_sum(
        prior.node_visit_count,
        script.node_visit_count,
        "the fake Phase 15 visit cumulative overflowed");
    const std::uint64_t cumulative_ambiguous_count = checked_sum(
        prior.ambiguous_cone_candidate_count,
        script.ambiguous_cone_candidate_count,
        "the fake Phase 15 ambiguity cumulative overflowed");
    const std::uint64_t cumulative_unbanked_count = checked_sum(
        prior.unbanked_candidate_count,
        script.unbanked_candidate_count,
        "the fake Phase 15 unbanked cumulative overflowed");
    if (cumulative_candidate_count + cumulative_pruned_mass >
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
    control.cumulative_candidate_count = cumulative_candidate_count;
    control.cumulative_prune_region_count = cumulative_prune_region_count;
    control.cumulative_certified_pruned_pair_mass = cumulative_pruned_mass;
    control.cumulative_node_visit_count = cumulative_node_visit_count;
    control.status = static_cast<std::uint64_t>(anchor_status(script.status));
    control.yield_reason =
        static_cast<std::uint64_t>(yield_reason(script.yield_reason));
    control.stop_reason = static_cast<std::uint64_t>(script.stop_reason);
    control.failure_code = static_cast<std::uint64_t>(
        Phase15MortonYao48DeviceTiledFailureCode::none);
    control.ambiguous_cone_candidate_count =
        script.ambiguous_cone_candidate_count;
    control.unbanked_candidate_count = script.unbanked_candidate_count;
    control.cumulative_ambiguous_cone_candidate_count =
        cumulative_ambiguous_count;
    control.cumulative_unbanked_candidate_count =
        cumulative_unbanked_count;
    control.unresolved_pair_mass =
        static_cast<std::uint64_t>(anchor_position) -
        cumulative_candidate_count - cumulative_pruned_mass;
    control.tile_epoch = request.tile_epoch;
    control.chunk_sequence = request.chunk_sequence;
    batch.host_anchor_controls.push_back(control);

    prior = FakeProgress{
        cumulative_candidate_count,
        cumulative_prune_region_count,
        cumulative_pruned_mass,
        cumulative_node_visit_count,
        cumulative_ambiguous_count,
        cumulative_unbanked_count,
        script.status == FakeAnchorStatus::complete};
    any_chunk_ready =
        any_chunk_ready || script.status == FakeAnchorStatus::chunk_ready;
    const std::uint64_t quantum = static_cast<std::uint64_t>(
        request.node_visit_capacity_per_anchor);
    const std::uint64_t subdivisions =
        script.node_visit_count / quantum +
        (script.node_visit_count % quantum == 0U ? 0U : 1U);
    traversal_subdivision_count = std::max(
        traversal_subdivision_count,
        static_cast<std::size_t>(subdivisions));
  }
  commit_continuation(std::move(progress));

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
  batch.physical_cached_prune_witness_capacity = checked_product(
      request.anchor_count,
      morsehgp3d::gpu::
          morton_yao48_device_tiled_pair_frontier_cached_prune_witnesses_per_anchor,
      "the fake Phase 15 cached prune witness capacity overflows size_t");
  batch.physical_anchor_control_capacity = request.anchor_count;
  batch.physical_anchor_checkpoint_capacity = request.anchor_count;
  batch.physical_pending_anchor_count_capacity = 1U;
  std::size_t arena_bytes = checked_product(
      batch.physical_candidate_capacity,
      sizeof(Phase15MortonYao48DeviceTiledCandidateRecord),
      "the fake Phase 15 candidate arena bytes overflow size_t");
  arena_bytes = checked_size_sum(
      arena_bytes,
      checked_product(
          batch.physical_prune_region_capacity,
          sizeof(Phase15MortonYao48DeviceTiledPruneRegionRecord),
          "the fake Phase 15 prune arena bytes overflow size_t"),
      "the fake Phase 15 arena bytes overflow size_t");
  arena_bytes = checked_size_sum(
      arena_bytes,
      checked_product(
          batch.physical_witness_bank_slot_capacity,
          sizeof(Phase15MortonYao48DeviceTiledWitnessBankSlot),
          "the fake Phase 15 witness arena bytes overflow size_t"),
      "the fake Phase 15 arena bytes overflow size_t");
  arena_bytes = checked_size_sum(
      arena_bytes,
      checked_product(
          batch.physical_cached_prune_witness_capacity,
          sizeof(Phase15MortonYao48DeviceTiledCachedPruneWitness),
          "the fake Phase 15 cached prune witness arena bytes overflow "
          "size_t"),
      "the fake Phase 15 arena bytes overflow size_t");
  arena_bytes = checked_size_sum(
      arena_bytes,
      checked_product(
          batch.physical_anchor_control_capacity,
          sizeof(Phase15MortonYao48DeviceTiledAnchorControl),
          "the fake Phase 15 control arena bytes overflow size_t"),
      "the fake Phase 15 arena bytes overflow size_t");
  arena_bytes = checked_size_sum(
      arena_bytes,
      checked_product(
          batch.physical_anchor_checkpoint_capacity,
          sizeof(Phase15MortonYao48DeviceTiledAnchorCheckpoint),
          "the fake Phase 15 checkpoint arena bytes overflow size_t"),
      "the fake Phase 15 arena bytes overflow size_t");
  batch.physical_device_arena_capacity_bytes = checked_size_sum(
      arena_bytes,
      sizeof(std::uint64_t),
      "the fake Phase 15 pending-anchor arena bytes overflow size_t");
  batch.traversal_subdivision_count = traversal_subdivision_count;
  batch.maximum_traversal_subdivision_count_per_anchor =
      request.certified_node_count /
          request.node_visit_capacity_per_anchor +
      (request.certified_node_count %
                   request.node_visit_capacity_per_anchor ==
               0U
           ? 0U
           : 1U);
  batch.source_snapshot_epoch = request.source_snapshot_epoch;
  batch.output_buffer_epoch = request.output_buffer_epoch;
  batch.tile_epoch = request.tile_epoch;
  batch.chunk_sequence = request.chunk_sequence;
  batch.execution_kind =
      Phase15MortonYao48DeviceTiledExecutionKind::host_fake;
  batch.prune_semantics = request.prune_semantics;
  batch.required_witness_count = request.required_witness_count;
  batch.fixed_anchor_segments_allocated = true;
  batch.output_owner_detached_for_tile_lifetime = true;
  batch.interval_cone_classification_requested = true;
  batch.ambiguous_cone_to_unbanked_candidate_requested = true;
  batch.target_tested_before_bank_insert_requested = true;
  batch.retained_witnesses_outside_pruned_subtree_requested = true;
  batch.witness_direction_intervals_cached_per_bank_slot_requested = true;
  batch.active_witness_slot_mask_authenticated_requested = true;
  batch.inactive_witness_slots_skipped_in_physical_order_requested = true;
  batch.last_prune_witness_direction_cache_retained_requested = true;
  batch.nonnegative_diametral_witness_interval_lower_bound_requested =
      request.prune_semantics ==
      MortonYao48DeviceTiledPairFrontierPruneSemantics::closed_rank_window;
  batch.strictly_positive_diametral_witness_interval_lower_bound_requested =
      request.prune_semantics ==
      MortonYao48DeviceTiledPairFrontierPruneSemantics::
          strict_interior_threshold;
  batch.censored_anchor_outputs_invalidated = true;
  batch.resume_same_tile = request.resume_same_tile;
  batch.capacity_yield_resumable = any_chunk_ready;
  batch.process_restart_resumable = false;

  switch (launch.corruption) {
    case Corruption::none:
      break;
    case Corruption::stale_output_epoch:
      --batch.output_buffer_epoch;
      break;
    case Corruption::candidate_device_to_host:
      batch.candidate_device_to_host_count = 1U;
      break;
    case Corruption::forged_cuda_execution:
      batch.execution_kind = Phase15MortonYao48DeviceTiledExecutionKind::cuda;
      batch.cuda_execution_contract_satisfied = true;
      batch.cuda_device = 0;
      break;
    case Corruption::forged_device_arena_reuse:
      batch.fresh_tile_device_arena_reused = true;
      break;
    case Corruption::missing_output_owner:
      batch.retained_output_owner.reset();
      break;
    case Corruption::shared_output_owner:
      retain_shared_output_owner(batch.retained_output_owner);
      batch.output_owner_detached_for_tile_lifetime = false;
      break;
    case Corruption::corrupt_metadata_digest:
      break;
    case Corruption::corrupt_prune_semantics:
      batch.prune_semantics =
          batch.prune_semantics ==
                  MortonYao48DeviceTiledPairFrontierPruneSemantics::
                      closed_rank_window
              ? MortonYao48DeviceTiledPairFrontierPruneSemantics::
                    strict_interior_threshold
              : MortonYao48DeviceTiledPairFrontierPruneSemantics::
                    closed_rank_window;
      break;
    case Corruption::nonzero_failure_code:
      batch.host_anchor_controls.front().failure_code =
          static_cast<std::uint64_t>(
              Phase15MortonYao48DeviceTiledFailureCode::malformed_traversal);
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
      break;
    case Corruption::too_many_regions_for_pruned_mass:
      batch.host_anchor_controls.front().candidate_count = 0U;
      batch.host_anchor_controls.front().certified_pruned_pair_mass = 1U;
      batch.host_anchor_controls.front().prune_region_count = 2U;
      batch.host_anchor_controls.front().node_visit_count = 2U;
      break;
    case Corruption::outputs_exceed_node_visits:
      batch.host_anchor_controls.front().node_visit_count = 0U;
      break;
    case Corruption::stale_tile_epoch:
      --batch.host_anchor_controls.front().tile_epoch;
      break;
    case Corruption::stale_chunk_sequence:
      --batch.host_anchor_controls.front().chunk_sequence;
      break;
    case Corruption::cumulative_candidate_rollback:
      if (batch.host_anchor_controls.front().cumulative_candidate_count == 0U) {
        ++batch.host_anchor_controls.front().cumulative_candidate_count;
      } else {
        --batch.host_anchor_controls.front().cumulative_candidate_count;
      }
      break;
    case Corruption::cumulative_prune_rollback:
      if (batch.host_anchor_controls.front()
              .cumulative_certified_pruned_pair_mass == 0U) {
        ++batch.host_anchor_controls.front()
              .cumulative_certified_pruned_pair_mass;
      } else {
        --batch.host_anchor_controls.front()
              .cumulative_certified_pruned_pair_mass;
      }
      break;
    case Corruption::process_restart_claimed:
      batch.process_restart_resumable = true;
      break;
    case Corruption::simulated_launcher_failure:
      break;
  }
  batch.metadata_digest =
      phase15_morton_yao48_device_tiled_metadata_digest(batch);
  if (launch.corruption == Corruption::corrupt_metadata_digest) {
    batch.metadata_digest ^= UINT64_C(1);
  }
  return batch;
}

}  // namespace morsehgp3d::gpu::detail
