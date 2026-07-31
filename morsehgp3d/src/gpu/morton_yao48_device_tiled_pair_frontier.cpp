#include "morsehgp3d/gpu/morton_yao48_device_tiled_pair_frontier.hpp"

#include "../cuda/phase15_morton_yao48_device_tiled_pair_frontier_internal.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::detail {

struct Phase15MortonYao48DeviceTiledAnchorProgress final {
  std::uint64_t candidate_count{};
  std::uint64_t prune_region_count{};
  std::uint64_t certified_pruned_pair_mass{};
  std::uint64_t node_visit_count{};
  std::uint64_t ambiguous_cone_candidate_count{};
  std::uint64_t unbanked_candidate_count{};
  bool complete{false};
};

class Phase15MortonYao48DeviceTiledPairFrontierHostState final {
 public:
  explicit Phase15MortonYao48DeviceTiledPairFrontierHostState(
      Phase15MortonYao48DeviceTiledAdoptedTraversal adopted)
      : traversal(std::move(adopted)) {}

  Phase15MortonYao48DeviceTiledAdoptedTraversal traversal;
  bool active_tile{false};
  std::size_t active_tile_anchor_begin{};
  std::size_t active_tile_anchor_count{};
  std::uint64_t active_tile_epoch{};
  std::uint64_t next_chunk_sequence{};
  std::vector<Phase15MortonYao48DeviceTiledAnchorProgress> anchor_progress;
};

}  // namespace morsehgp3d::gpu::detail

namespace morsehgp3d::gpu {
namespace {

using AdoptedTraversal =
    detail::Phase15MortonYao48DeviceTiledAdoptedTraversal;
using AnchorControl =
    detail::Phase15MortonYao48DeviceTiledAnchorControl;
using AnchorStatus =
    detail::Phase15MortonYao48DeviceTiledAnchorStatus;
using AnchorProgress =
    detail::Phase15MortonYao48DeviceTiledAnchorProgress;
using DeviceBatch = detail::Phase15MortonYao48DeviceTiledBatch;
using ExecutionKind =
    detail::Phase15MortonYao48DeviceTiledExecutionKind;
using FailureCode =
    detail::Phase15MortonYao48DeviceTiledFailureCode;
using InternalYieldReason =
    detail::Phase15MortonYao48DeviceTiledYieldReason;
using Request = detail::Phase15MortonYao48DeviceTiledRequest;

struct DetachedTileAuthority final {
  std::shared_ptr<void> traversal_owner;
  std::shared_ptr<const void> source_cloud_identity;
  std::shared_ptr<void> output_owner;
};

struct ValidatedBatch final {
  std::size_t authorized_anchor_count{};
  std::size_t completed_anchor_count{};
  std::size_t certified_prune_region_count{};
  std::uint64_t candidate_pair_mass{};
  std::uint64_t certified_pruned_pair_mass{};
  std::uint64_t physical_node_visit_count{};
  std::uint64_t ambiguous_cone_candidate_count{};
  std::uint64_t unbanked_candidate_count{};
  std::vector<AnchorProgress> next_progress;
  bool tile_complete{false};
  bool capacity_yield{false};
  bool fatal{false};
  MortonYao48DeviceTiledPairFrontierYieldReason yield_reason{
      MortonYao48DeviceTiledPairFrontierYieldReason::none};
  MortonYao48DeviceTiledPairFrontierStopReason stop_reason{
      MortonYao48DeviceTiledPairFrontierStopReason::none};
};

[[nodiscard]] std::size_t checked_size_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::length_error(message);
  }
  return left * right;
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

[[nodiscard]] std::uint64_t checked_u64_sum(
    std::uint64_t left,
    std::uint64_t right,
    const char* message) {
  if (right > std::numeric_limits<std::uint64_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_size(
    std::uint64_t value,
    const char* message) {
  if constexpr (
      std::numeric_limits<std::size_t>::max() <
      std::numeric_limits<std::uint64_t>::max()) {
    if (value > std::numeric_limits<std::size_t>::max()) {
      throw std::runtime_error(message);
    }
  }
  return static_cast<std::size_t>(value);
}

[[nodiscard]] std::size_t checked_node_count(
    std::size_t point_count,
    const char* message) {
  if (point_count == 0U ||
      point_count >
          std::numeric_limits<std::size_t>::max() / 2U + 1U) {
    throw std::length_error(message);
  }
  return point_count * 2U - 1U;
}

[[nodiscard]] std::size_t maximum_traversal_subdivision_count(
    std::size_t certified_node_count) {
  constexpr std::size_t quantum =
      morton_yao48_device_tiled_pair_frontier_node_visits_per_anchor;
  if (certified_node_count == 0U) {
    throw std::length_error(
        "the Phase 15 traversal subdivision ceiling requires nodes");
  }
  return certified_node_count / quantum +
      (certified_node_count % quantum == 0U ? 0U : 1U);
}

[[nodiscard]] std::size_t checked_device_arena_capacity_bytes(
    const Request& request) {
  const std::size_t candidate_count = checked_size_product(
      request.anchor_count,
      request.candidate_capacity_per_anchor,
      "the Phase 15 candidate arena extent overflows size_t");
  const std::size_t prune_count = checked_size_product(
      request.anchor_count,
      request.prune_region_capacity_per_anchor,
      "the Phase 15 prune arena extent overflows size_t");
  const std::size_t witness_count = checked_size_product(
      checked_size_product(
          request.anchor_count,
          request.witness_bank_count_per_anchor,
          "the Phase 15 witness-bank arena extent overflows size_t"),
      request.witness_slot_count_per_bank,
      "the Phase 15 witness-slot arena extent overflows size_t");
  std::size_t bytes = checked_size_product(
      candidate_count,
      sizeof(detail::Phase15MortonYao48DeviceTiledCandidateRecord),
      "the Phase 15 candidate arena bytes overflow size_t");
  bytes = checked_size_sum(
      bytes,
      checked_size_product(
          prune_count,
          sizeof(detail::Phase15MortonYao48DeviceTiledPruneRegionRecord),
          "the Phase 15 prune arena bytes overflow size_t"),
      "the Phase 15 output arena bytes overflow size_t");
  bytes = checked_size_sum(
      bytes,
      checked_size_product(
          witness_count,
          sizeof(detail::Phase15MortonYao48DeviceTiledWitnessBankSlot),
          "the Phase 15 witness arena bytes overflow size_t"),
      "the Phase 15 output arena bytes overflow size_t");
  bytes = checked_size_sum(
      bytes,
      checked_size_product(
          request.anchor_count,
          sizeof(AnchorControl),
          "the Phase 15 control arena bytes overflow size_t"),
      "the Phase 15 output arena bytes overflow size_t");
  bytes = checked_size_sum(
      bytes,
      checked_size_product(
          request.anchor_count,
          sizeof(detail::Phase15MortonYao48DeviceTiledAnchorCheckpoint),
          "the Phase 15 checkpoint arena bytes overflow size_t"),
      "the Phase 15 output arena bytes overflow size_t");
  return checked_size_sum(
      bytes,
      sizeof(std::uint64_t),
      "the Phase 15 pending-anchor arena bytes overflow size_t");
}

[[nodiscard]] std::uint64_t checked_pair_universe(
    std::size_t point_count) {
  if (point_count < 2U) {
    return 0U;
  }
  const std::uint64_t count = static_cast<std::uint64_t>(point_count);
  if (count >
      std::numeric_limits<std::uint64_t>::max() / (count - UINT64_C(1))) {
    throw std::length_error(
        "the Phase 15 unordered-pair universe overflows uint64_t");
  }
  return count * (count - UINT64_C(1)) / UINT64_C(2);
}

[[nodiscard]] MortonYao48DeviceTiledPairFrontierConfig validate_config(
    MortonYao48DeviceTiledPairFrontierConfig config) {
  if (config.maximum_closed_rank < 2U ||
      config.maximum_closed_rank >
          morton_yao48_device_tiled_pair_frontier_maximum_closed_rank) {
    throw std::out_of_range(
        "a Phase 15 device tiled Morton/Yao48 closed-rank cap must be in "
        "[2, 11]");
  }
  if (!morton_yao48_device_tiled_pair_frontier_prune_semantics_known(
          config.prune_semantics)) {
    throw std::out_of_range(
        "a Phase 15 device tiled Morton/Yao48 prune semantics tag is "
        "unknown");
  }
  if (config.anchor_tile_capacity == 0U ||
      config.anchor_tile_capacity >
          morton_yao48_device_tiled_pair_frontier_maximum_anchor_tile_capacity) {
    throw std::out_of_range(
        "a Phase 15 device tiled Morton/Yao48 anchor tile must be in "
        "[1, 4096]");
  }
  return config;
}

void validate_adopted_traversal(
    const AdoptedTraversal& adopted,
    const MortonLbvhDeviceTraversalLeaseAudit& source) {
  const std::size_t expected_maximum_node_count = checked_node_count(
      source.maximum_point_count,
      "the adopted Phase 15 maximum node count overflows size_t");
  const std::size_t expected_node_count = checked_node_count(
      source.point_count,
      "the adopted Phase 15 active node count overflows size_t");
  const std::size_t expected_coordinate_capacity = checked_size_product(
      source.maximum_point_count,
      3U,
      "the adopted Phase 15 coordinate capacity overflows size_t");
  if (!adopted.retained_owner || !adopted.source_cloud_identity ||
      adopted.point_count != source.point_count ||
      adopted.certified_node_count != source.certified_node_count ||
      adopted.maximum_point_count != source.maximum_point_count ||
      adopted.maximum_node_count != source.maximum_node_count ||
      adopted.certified_node_count != expected_node_count ||
      adopted.maximum_node_count != expected_maximum_node_count ||
      adopted.retained_coordinate_word_capacity !=
          expected_coordinate_capacity ||
      adopted.retained_morton_point_id_capacity !=
          source.maximum_point_count ||
      adopted.retained_node_capacity != expected_maximum_node_count ||
      adopted.source_snapshot_epoch != source.source_snapshot_epoch ||
      !adopted.canonical_coordinate_words_retained ||
      !adopted.active_morton_point_ids_retained ||
      !adopted.certified_device_nodes_retained ||
      adopted.host_fake_lifecycle_exercised ==
          adopted.cuda_device_storage_retained) {
    throw std::runtime_error(
        "the Phase 15 traversal adoption returned a foreign or malformed "
        "authority");
  }

  switch (adopted.execution_kind) {
    case ExecutionKind::host_fake:
      if (!adopted.host_fake_lifecycle_exercised ||
          adopted.cuda_device_storage_retained ||
          adopted.device_coordinate_bits != nullptr ||
          adopted.device_morton_point_ids != nullptr ||
          adopted.device_nodes != nullptr || adopted.cuda_device != -1) {
        throw std::runtime_error(
            "the Phase 15 host-fake traversal adoption forged device "
            "storage");
      }
      return;
    case ExecutionKind::cuda:
      if (adopted.host_fake_lifecycle_exercised ||
          !adopted.cuda_device_storage_retained ||
          adopted.device_coordinate_bits == nullptr ||
          adopted.device_morton_point_ids == nullptr ||
          adopted.device_nodes == nullptr || adopted.cuda_device < 0) {
        throw std::runtime_error(
            "the Phase 15 CUDA traversal adoption omitted resident views");
      }
      return;
  }
  throw std::runtime_error(
      "the Phase 15 traversal adoption returned an unknown backend");
}

[[nodiscard]] MortonYao48DeviceTiledPairFrontierStopReason
validate_stop_reason(std::uint64_t raw) {
  switch (raw) {
    case static_cast<std::uint64_t>(
        MortonYao48DeviceTiledPairFrontierStopReason::none):
      return MortonYao48DeviceTiledPairFrontierStopReason::none;
    case static_cast<std::uint64_t>(
        MortonYao48DeviceTiledPairFrontierStopReason::node_visit_capacity):
      return MortonYao48DeviceTiledPairFrontierStopReason::
          node_visit_capacity;
    case static_cast<std::uint64_t>(
        MortonYao48DeviceTiledPairFrontierStopReason::fatal_failure):
      return MortonYao48DeviceTiledPairFrontierStopReason::fatal_failure;
    default:
      throw std::runtime_error(
          "a Phase 15 anchor control returned an unknown stop reason");
  }
}

[[nodiscard]] MortonYao48DeviceTiledPairFrontierYieldReason
validate_yield_reason(std::uint64_t raw) {
  switch (raw) {
    case static_cast<std::uint64_t>(
        InternalYieldReason::none):
      return MortonYao48DeviceTiledPairFrontierYieldReason::none;
    case static_cast<std::uint64_t>(
        InternalYieldReason::candidate_segment_full):
      return MortonYao48DeviceTiledPairFrontierYieldReason::
          candidate_segment_full;
    case static_cast<std::uint64_t>(
        InternalYieldReason::prune_segment_full):
      return MortonYao48DeviceTiledPairFrontierYieldReason::
          prune_segment_full;
    default:
      throw std::runtime_error(
          "a Phase 15 anchor control returned an unknown yield reason");
  }
}

void validate_batch_envelope(
    const DeviceBatch& batch,
    const AdoptedTraversal& traversal,
    const Request& request) {
  const std::size_t expected_candidate_capacity = checked_size_product(
      request.anchor_count,
      request.candidate_capacity_per_anchor,
      "the Phase 15 candidate segment capacity overflows size_t");
  const std::size_t expected_prune_capacity = checked_size_product(
      request.anchor_count,
      request.prune_region_capacity_per_anchor,
      "the Phase 15 prune segment capacity overflows size_t");
  const std::size_t bank_count = checked_size_product(
      request.anchor_count,
      request.witness_bank_count_per_anchor,
      "the Phase 15 witness bank count overflows size_t");
  const std::size_t expected_bank_capacity = checked_size_product(
      bank_count,
      request.witness_slot_count_per_bank,
      "the Phase 15 witness bank slot capacity overflows size_t");
  const std::size_t expected_device_arena_capacity_bytes =
      checked_device_arena_capacity_bytes(request);
  const std::size_t expected_maximum_subdivision_count =
      maximum_traversal_subdivision_count(request.certified_node_count);
  const std::size_t expected_required_witness_count =
      morton_yao48_device_tiled_pair_frontier_required_witness_count(
          request.prune_semantics, request.maximum_closed_rank);
  const bool closed_rank_semantics =
      request.prune_semantics ==
      MortonYao48DeviceTiledPairFrontierPruneSemantics::closed_rank_window;
  const bool strict_interior_semantics =
      request.prune_semantics ==
      MortonYao48DeviceTiledPairFrontierPruneSemantics::
          strict_interior_threshold;
  if (!batch.retained_output_owner ||
      !batch.source_cloud_identity_authority ||
      batch.source_cloud_identity_authority.get() !=
          traversal.source_cloud_identity.get() ||
      batch.host_anchor_controls.size() != request.anchor_count ||
      batch.physical_candidate_capacity != expected_candidate_capacity ||
      batch.physical_prune_region_capacity != expected_prune_capacity ||
      batch.physical_witness_bank_slot_capacity != expected_bank_capacity ||
      batch.physical_anchor_control_capacity != request.anchor_count ||
      batch.physical_anchor_checkpoint_capacity != request.anchor_count ||
      batch.physical_pending_anchor_count_capacity != 1U ||
      batch.physical_device_arena_capacity_bytes !=
          expected_device_arena_capacity_bytes ||
      batch.traversal_subdivision_count == 0U ||
      batch.traversal_subdivision_count >
          expected_maximum_subdivision_count ||
      batch.maximum_traversal_subdivision_count_per_anchor !=
          expected_maximum_subdivision_count ||
      batch.source_snapshot_epoch != request.source_snapshot_epoch ||
      batch.output_buffer_epoch != request.output_buffer_epoch ||
      batch.tile_epoch != request.tile_epoch ||
      batch.chunk_sequence != request.chunk_sequence ||
      batch.resume_same_tile != request.resume_same_tile ||
      !morton_yao48_device_tiled_pair_frontier_prune_semantics_known(
          request.prune_semantics) ||
      request.required_witness_count != expected_required_witness_count ||
      request.witness_slot_count_per_bank !=
          request.required_witness_count ||
      batch.prune_semantics != request.prune_semantics ||
      batch.required_witness_count != request.required_witness_count ||
      batch.process_restart_resumable ||
      batch.execution_kind != traversal.execution_kind ||
      batch.candidate_device_to_host_count != 0U ||
      batch.certified_prune_device_to_host_count != 0U ||
      !batch.fixed_anchor_segments_allocated ||
      !batch.output_owner_detached_for_tile_lifetime ||
      !batch.interval_cone_classification_requested ||
      !batch.ambiguous_cone_to_unbanked_candidate_requested ||
      !batch.target_tested_before_bank_insert_requested ||
      !batch.retained_witnesses_outside_pruned_subtree_requested ||
      batch.nonnegative_diametral_witness_interval_lower_bound_requested !=
          closed_rank_semantics ||
      batch
              .strictly_positive_diametral_witness_interval_lower_bound_requested !=
          strict_interior_semantics ||
      !batch.censored_anchor_outputs_invalidated ||
      batch.exact_diametral_rank_evaluated ||
      batch.scientific_pair_catalog_published ||
      batch.dense_pair_fallback_performed ||
      batch.global_pair_matrix_materialized ||
      batch.higher_order_structure_materialized ||
      batch.metadata_digest !=
          detail::phase15_morton_yao48_device_tiled_metadata_digest(batch)) {
    throw std::runtime_error(
        "the Phase 15 device tile returned a malformed or forbidden "
        "batch envelope");
  }

  switch (batch.execution_kind) {
    case ExecutionKind::host_fake:
      if (batch.device_candidate_records != nullptr ||
          batch.device_prune_regions != nullptr ||
          batch.device_witness_bank_slots != nullptr ||
          batch.device_anchor_controls != nullptr ||
          batch.anchor_control_device_to_host_count != 0U ||
          batch.anchor_control_device_to_host_byte_count != 0U ||
          batch.resume_control_device_to_host_count != 0U ||
          batch.resume_control_device_to_host_byte_count != 0U ||
          batch.kernel_launch_count != 0U ||
          batch.synchronization_count != 0U ||
          batch.cuda_device != -1 ||
          batch.cuda_execution_contract_satisfied ||
          batch.fresh_tile_device_arena_allocated ||
          batch.fresh_tile_device_arena_reused) {
        throw std::runtime_error(
            "the Phase 15 host fake forged CUDA execution metadata");
      }
      return;
    case ExecutionKind::cuda: {
      const std::size_t expected_control_bytes = checked_size_product(
          request.anchor_count,
          sizeof(AnchorControl),
          "the Phase 15 anchor-control transfer extent overflows size_t");
      const std::size_t expected_resume_bytes = checked_size_product(
          batch.traversal_subdivision_count,
          sizeof(std::uint64_t),
          "the Phase 15 resume-control transfer extent overflows size_t");
      if (batch.device_candidate_records == nullptr ||
          batch.device_prune_regions == nullptr ||
          batch.device_witness_bank_slots == nullptr ||
          batch.device_anchor_controls == nullptr ||
          batch.anchor_control_device_to_host_count != request.anchor_count ||
          batch.anchor_control_device_to_host_byte_count !=
              expected_control_bytes ||
          batch.kernel_launch_count !=
              batch.traversal_subdivision_count ||
          batch.synchronization_count !=
              batch.traversal_subdivision_count + 1U ||
          batch.resume_control_device_to_host_count !=
              batch.traversal_subdivision_count ||
          batch.resume_control_device_to_host_byte_count !=
              expected_resume_bytes ||
          batch.cuda_device != traversal.cuda_device ||
          !batch.cuda_execution_contract_satisfied ||
          (request.resume_same_tile
               ? batch.fresh_tile_device_arena_allocated ||
                     batch.fresh_tile_device_arena_reused
               : batch.fresh_tile_device_arena_allocated ==
                     batch.fresh_tile_device_arena_reused)) {
        throw std::runtime_error(
            "the Phase 15 CUDA tile returned invalid resident views or "
            "control-only transfer metadata");
      }
      return;
    }
  }
  throw std::runtime_error(
      "the Phase 15 device tile returned an unknown backend");
}

[[nodiscard]] ValidatedBatch validate_anchor_controls(
    const DeviceBatch& batch,
    const Request& request,
    const std::vector<AnchorProgress>& previous_progress) {
  ValidatedBatch validated;
  if (previous_progress.size() != request.anchor_count) {
    throw std::logic_error(
        "the Phase 15 host continuation has a foreign anchor extent");
  }
  validated.authorized_anchor_count = request.anchor_count;
  validated.next_progress = previous_progress;
  const std::size_t launched_node_visit_count = checked_size_product(
      batch.traversal_subdivision_count,
      request.node_visit_capacity_per_anchor,
      "the Phase 15 launched resumed node-visit count overflows size_t");
  bool all_complete = true;
  bool any_chunk_ready = false;
  bool any_fatal = false;
  for (std::size_t control_index = 0U;
       control_index < batch.host_anchor_controls.size();
       ++control_index) {
    const AnchorControl& control =
        batch.host_anchor_controls[control_index];
    const std::size_t anchor_position = checked_size_sum(
        request.anchor_begin,
        control_index,
        "the Phase 15 anchor position overflows size_t");
    const AnchorProgress& previous = previous_progress[control_index];
    if (control.failure_code !=
        static_cast<std::uint64_t>(FailureCode::none)) {
      throw std::runtime_error(
          "a Phase 15 CUDA anchor reported an internal failure code; the "
          "context is poisoned and no terminal frontier is published");
    }
    const std::uint64_t expected_candidate_count = checked_u64_sum(
        previous.candidate_count,
        control.candidate_count,
        "the Phase 15 cumulative candidate count overflowed uint64_t");
    const std::uint64_t expected_prune_region_count = checked_u64_sum(
        previous.prune_region_count,
        control.prune_region_count,
        "the Phase 15 cumulative prune-region count overflowed uint64_t");
    const std::uint64_t expected_pruned_mass = checked_u64_sum(
        previous.certified_pruned_pair_mass,
        control.certified_pruned_pair_mass,
        "the Phase 15 cumulative pruned mass overflowed uint64_t");
    const std::uint64_t expected_node_visit_count = checked_u64_sum(
        previous.node_visit_count,
        control.node_visit_count,
        "the Phase 15 cumulative node-visit count overflowed uint64_t");
    const std::uint64_t expected_ambiguous_count = checked_u64_sum(
        previous.ambiguous_cone_candidate_count,
        control.ambiguous_cone_candidate_count,
        "the Phase 15 cumulative ambiguity count overflowed uint64_t");
    const std::uint64_t expected_unbanked_count = checked_u64_sum(
        previous.unbanked_candidate_count,
        control.unbanked_candidate_count,
        "the Phase 15 cumulative unbanked count overflowed uint64_t");
    if (control.anchor_morton_position !=
            static_cast<std::uint64_t>(anchor_position) ||
        control.tile_epoch != request.tile_epoch ||
        control.chunk_sequence != request.chunk_sequence ||
        control.reserved_zero != 0U ||
        control.candidate_count > request.candidate_capacity_per_anchor ||
        control.prune_region_count >
            request.prune_region_capacity_per_anchor ||
        control.node_visit_count > launched_node_visit_count ||
        control.cumulative_candidate_count != expected_candidate_count ||
        control.cumulative_prune_region_count !=
            expected_prune_region_count ||
        control.cumulative_certified_pruned_pair_mass !=
            expected_pruned_mass ||
        control.cumulative_node_visit_count != expected_node_visit_count ||
        control.cumulative_ambiguous_cone_candidate_count !=
            expected_ambiguous_count ||
        control.cumulative_unbanked_candidate_count !=
            expected_unbanked_count ||
        control.cumulative_node_visit_count > request.certified_node_count ||
        control.ambiguous_cone_candidate_count >
            control.unbanked_candidate_count ||
        control.unbanked_candidate_count > control.candidate_count ||
        control.cumulative_ambiguous_cone_candidate_count >
            control.cumulative_unbanked_candidate_count ||
        control.cumulative_unbanked_candidate_count >
            control.cumulative_candidate_count ||
        control.cumulative_certified_pruned_pair_mass >
            static_cast<std::uint64_t>(anchor_position)) {
      throw std::runtime_error(
          "a Phase 15 anchor control violated its chunk delta, cumulative, "
          "sequence, or fixed-segment contract");
    }
    if ((control.prune_region_count == 0U) !=
            (control.certified_pruned_pair_mass == 0U) ||
        control.prune_region_count >
            control.certified_pruned_pair_mass ||
        control.prune_region_count > control.node_visit_count ||
        control.candidate_count >
            control.node_visit_count - control.prune_region_count) {
      throw std::runtime_error(
          "a Phase 15 anchor control forged output records or certified "
          "mass without the required physical node visits");
    }
    const std::uint64_t classified_mass = checked_u64_sum(
        control.cumulative_candidate_count,
        control.cumulative_certified_pruned_pair_mass,
        "a Phase 15 anchor classified mass overflowed uint64_t");
    if (classified_mass > static_cast<std::uint64_t>(anchor_position) ||
        control.unresolved_pair_mass !=
            static_cast<std::uint64_t>(anchor_position) -
                classified_mass) {
      throw std::runtime_error(
          "a Phase 15 anchor control returned an invalid local pair "
          "partition");
    }

    const auto status =
        static_cast<AnchorStatus>(control.status);
    const MortonYao48DeviceTiledPairFrontierStopReason stop_reason =
        validate_stop_reason(control.stop_reason);
    const MortonYao48DeviceTiledPairFrontierYieldReason yield_reason =
        validate_yield_reason(control.yield_reason);
    bool complete = false;
    switch (status) {
      case AnchorStatus::active:
        throw std::runtime_error(
            "a Phase 15 launcher returned an active anchor instead of "
            "continuing its bounded node quantum");
      case AnchorStatus::chunk_ready:
        if (stop_reason !=
                MortonYao48DeviceTiledPairFrontierStopReason::none ||
            control.unresolved_pair_mass == 0U || previous.complete ||
            (yield_reason ==
                     MortonYao48DeviceTiledPairFrontierYieldReason::
                         candidate_segment_full &&
                 control.candidate_count !=
                     request.candidate_capacity_per_anchor) ||
            (yield_reason ==
                     MortonYao48DeviceTiledPairFrontierYieldReason::
                         prune_segment_full &&
                 control.prune_region_count !=
                     request.prune_region_capacity_per_anchor) ||
            yield_reason ==
                MortonYao48DeviceTiledPairFrontierYieldReason::none) {
          throw std::runtime_error(
              "a Phase 15 chunk-ready anchor did not exhaust its reported "
              "output segment");
        }
        any_chunk_ready = true;
        if (validated.yield_reason ==
            MortonYao48DeviceTiledPairFrontierYieldReason::none) {
          validated.yield_reason = yield_reason;
        } else if (validated.yield_reason != yield_reason) {
          validated.yield_reason =
              MortonYao48DeviceTiledPairFrontierYieldReason::
                  mixed_segments_full;
        }
        break;
      case AnchorStatus::complete:
        complete = true;
        if (stop_reason !=
                MortonYao48DeviceTiledPairFrontierStopReason::none ||
            yield_reason !=
                MortonYao48DeviceTiledPairFrontierYieldReason::none ||
            control.unresolved_pair_mass != 0U ||
            (previous.complete &&
             (control.candidate_count != 0U ||
              control.prune_region_count != 0U ||
              control.certified_pruned_pair_mass != 0U ||
              control.node_visit_count != 0U ||
              control.ambiguous_cone_candidate_count != 0U ||
              control.unbanked_candidate_count != 0U))) {
          throw std::runtime_error(
              "a Phase 15 complete anchor did not close its local pair "
              "partition");
        }
        break;
      case AnchorStatus::fatal:
        if (stop_reason !=
                MortonYao48DeviceTiledPairFrontierStopReason::
                    node_visit_capacity ||
            yield_reason !=
                MortonYao48DeviceTiledPairFrontierYieldReason::none ||
            previous.complete ||
            control.cumulative_node_visit_count !=
                request.certified_node_count) {
          throw std::runtime_error(
              "a Phase 15 fatal anchor omitted its certified node-capacity "
              "proof");
        }
        any_fatal = true;
        validated.stop_reason = stop_reason;
        break;
      default:
        throw std::runtime_error(
            "a Phase 15 anchor control returned an unknown status");
    }

    validated.physical_node_visit_count = checked_u64_sum(
        validated.physical_node_visit_count,
        control.node_visit_count,
        "the Phase 15 physical node-visit count overflowed uint64_t");
    validated.certified_prune_region_count = checked_size_sum(
        validated.certified_prune_region_count,
        checked_size(control.prune_region_count,
                     "a Phase 15 prune count does not fit size_t"),
        "the Phase 15 chunk prune count overflows size_t");
    validated.candidate_pair_mass = checked_u64_sum(
        validated.candidate_pair_mass,
        control.candidate_count,
        "the Phase 15 chunk candidate mass overflowed uint64_t");
    validated.certified_pruned_pair_mass = checked_u64_sum(
        validated.certified_pruned_pair_mass,
        control.certified_pruned_pair_mass,
        "the Phase 15 chunk prune mass overflowed uint64_t");
    validated.ambiguous_cone_candidate_count = checked_u64_sum(
        validated.ambiguous_cone_candidate_count,
        control.ambiguous_cone_candidate_count,
        "the Phase 15 chunk ambiguity count overflowed uint64_t");
    validated.unbanked_candidate_count = checked_u64_sum(
        validated.unbanked_candidate_count,
        control.unbanked_candidate_count,
        "the Phase 15 chunk unbanked count overflowed uint64_t");
    validated.next_progress[control_index] = AnchorProgress{
        control.cumulative_candidate_count,
        control.cumulative_prune_region_count,
        control.cumulative_certified_pruned_pair_mass,
        control.cumulative_node_visit_count,
        control.cumulative_ambiguous_cone_candidate_count,
        control.cumulative_unbanked_candidate_count,
        complete};
    all_complete = all_complete && complete;
  }
  if (any_fatal && any_chunk_ready) {
    throw std::runtime_error(
        "a Phase 15 batch mixed terminal failure with resumable output");
  }
  if (any_fatal &&
      (validated.candidate_pair_mass != 0U ||
       validated.certified_prune_region_count != 0U ||
       validated.certified_pruned_pair_mass != 0U)) {
    throw std::runtime_error(
        "a Phase 15 fatal batch attempted to publish partial outputs");
  }
  if (!all_complete && !any_chunk_ready && !any_fatal) {
    throw std::runtime_error(
        "a Phase 15 launcher returned without a yield, completion, or fatal stop");
  }
  if (batch.capacity_yield_resumable != any_chunk_ready) {
    throw std::runtime_error(
        "a Phase 15 batch forged its resumable-capacity flag");
  }
  validated.tile_complete = all_complete;
  validated.capacity_yield = any_chunk_ready;
  validated.fatal = any_fatal;
  validated.completed_anchor_count = all_complete ? request.anchor_count : 0U;
  return validated;
}

[[nodiscard]] ValidatedBatch validate_batch(
    const DeviceBatch& batch,
    const AdoptedTraversal& traversal,
    const Request& request,
    const std::vector<AnchorProgress>& previous_progress) {
  validate_batch_envelope(batch, traversal, request);
  return validate_anchor_controls(batch, request, previous_progress);
}

}  // namespace

MortonYao48DeviceCandidateTileLease::
    MortonYao48DeviceCandidateTileLease(
        MortonYao48DeviceCandidateTileLeaseAudit audit,
        std::shared_ptr<void> retained_owner,
        std::shared_ptr<void> source_owner_authority,
        std::shared_ptr<const void> source_cloud_identity_authority,
        const std::uint64_t* device_coordinate_bits,
        const std::uint64_t* device_morton_point_ids,
        const void* device_nodes,
        const void* device_candidate_records,
        const void* device_certified_prune_regions,
        const void* device_anchor_controls,
        std::size_t certified_node_count,
        std::size_t retained_coordinate_word_capacity,
        std::size_t retained_morton_point_id_capacity,
        std::size_t retained_node_capacity,
        std::size_t physical_anchor_control_capacity,
        std::size_t authorized_anchor_control_extent,
        int cuda_device,
        bool host_fake)
    : audit_(std::move(audit)),
      retained_owner_(std::move(retained_owner)),
      source_owner_authority_(std::move(source_owner_authority)),
      source_cloud_identity_authority_(
          std::move(source_cloud_identity_authority)),
      device_coordinate_bits_(device_coordinate_bits),
      device_morton_point_ids_(device_morton_point_ids),
      device_nodes_(device_nodes),
      device_candidate_records_(device_candidate_records),
      device_certified_prune_regions_(device_certified_prune_regions),
      device_anchor_controls_(device_anchor_controls),
      certified_node_count_(certified_node_count),
      retained_coordinate_word_capacity_(
          retained_coordinate_word_capacity),
      retained_morton_point_id_capacity_(
          retained_morton_point_id_capacity),
      retained_node_capacity_(retained_node_capacity),
      physical_anchor_control_capacity_(
          physical_anchor_control_capacity),
      authorized_anchor_control_extent_(
          authorized_anchor_control_extent),
      cuda_device_(cuda_device),
      host_fake_(host_fake) {}

bool MortonYao48DeviceCandidateTileLease::ready() const noexcept {
  const std::size_t authorized_anchor_count =
      audit_.anchor_end >= audit_.anchor_begin
          ? audit_.anchor_end - audit_.anchor_begin
          : 0U;
  const std::size_t expected_required_witness_count =
      morton_yao48_device_tiled_pair_frontier_required_witness_count(
          audit_.prune_semantics, audit_.maximum_closed_rank);
  const bool closed_rank_semantics =
      audit_.prune_semantics ==
      MortonYao48DeviceTiledPairFrontierPruneSemantics::closed_rank_window;
  const bool strict_interior_semantics =
      audit_.prune_semantics ==
      MortonYao48DeviceTiledPairFrontierPruneSemantics::
          strict_interior_threshold;
  if (audit_.maximum_closed_rank < 2U ||
      audit_.maximum_closed_rank >
          morton_yao48_device_tiled_pair_frontier_maximum_closed_rank ||
      !morton_yao48_device_tiled_pair_frontier_prune_semantics_known(
          audit_.prune_semantics) ||
      audit_.required_witness_count != expected_required_witness_count ||
      physical_anchor_control_capacity_ == 0U ||
      physical_anchor_control_capacity_ >
          morton_yao48_device_tiled_pair_frontier_maximum_anchor_tile_capacity) {
    return false;
  }
  const std::size_t expected_witness_capacity =
      physical_anchor_control_capacity_ *
      morton_yao48_device_tiled_pair_frontier_witness_bank_count *
      audit_.required_witness_count;
  const std::size_t expected_candidate_capacity =
      physical_anchor_control_capacity_ *
      morton_yao48_device_tiled_pair_frontier_candidates_per_anchor;
  const std::size_t expected_prune_capacity =
      physical_anchor_control_capacity_ *
      morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor;
  const std::size_t expected_device_arena_capacity_bytes =
      expected_candidate_capacity *
          sizeof(detail::Phase15MortonYao48DeviceTiledCandidateRecord) +
      expected_prune_capacity *
          sizeof(detail::Phase15MortonYao48DeviceTiledPruneRegionRecord) +
      expected_witness_capacity *
          sizeof(detail::Phase15MortonYao48DeviceTiledWitnessBankSlot) +
      physical_anchor_control_capacity_ * sizeof(AnchorControl) +
      physical_anchor_control_capacity_ *
          sizeof(detail::Phase15MortonYao48DeviceTiledAnchorCheckpoint) +
      sizeof(std::uint64_t);
  if (audit_.schema_version !=
          morton_yao48_device_tiled_pair_frontier_schema_version ||
      !retained_owner_ || !source_owner_authority_ ||
      !source_cloud_identity_authority_ ||
      audit_.source_snapshot_epoch == 0U ||
      audit_.candidate_buffer_epoch == 0U ||
      audit_.point_count < 2U || audit_.maximum_closed_rank < 2U ||
      audit_.point_count >
          std::numeric_limits<std::size_t>::max() / 2U + 1U ||
      audit_.point_count >
          std::numeric_limits<std::size_t>::max() / 3U ||
      audit_.certified_node_count != 2U * audit_.point_count - 1U ||
      audit_.retained_coordinate_word_capacity <
          3U * audit_.point_count ||
      audit_.retained_morton_point_id_capacity < audit_.point_count ||
      audit_.retained_node_capacity < audit_.certified_node_count ||
      audit_.certified_node_count != certified_node_count_ ||
      audit_.retained_coordinate_word_capacity !=
          retained_coordinate_word_capacity_ ||
      audit_.retained_morton_point_id_capacity !=
          retained_morton_point_id_capacity_ ||
      audit_.retained_node_capacity != retained_node_capacity_ ||
      audit_.maximum_closed_rank >
          morton_yao48_device_tiled_pair_frontier_maximum_closed_rank ||
      audit_.tile_epoch == 0U || audit_.chunk_sequence == 0U ||
      audit_.anchor_begin == 0U ||
      audit_.anchor_end <= audit_.anchor_begin ||
      audit_.anchor_end > audit_.point_count ||
      audit_.fixed_candidate_capacity_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_candidates_per_anchor ||
      audit_.fixed_prune_region_capacity_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor ||
      audit_.fixed_witness_bank_count_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_witness_bank_count ||
      authorized_anchor_control_extent_ != authorized_anchor_count ||
      physical_anchor_control_capacity_ <
          authorized_anchor_control_extent_ ||
      physical_anchor_control_capacity_ >
          std::numeric_limits<std::size_t>::max() /
              morton_yao48_device_tiled_pair_frontier_candidates_per_anchor ||
      audit_.physical_candidate_capacity !=
          physical_anchor_control_capacity_ *
              morton_yao48_device_tiled_pair_frontier_candidates_per_anchor ||
      physical_anchor_control_capacity_ >
          std::numeric_limits<std::size_t>::max() /
              morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor ||
      audit_.physical_prune_region_capacity !=
          physical_anchor_control_capacity_ *
              morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor ||
      audit_.physical_witness_bank_slot_capacity !=
          expected_witness_capacity ||
      audit_.physical_anchor_control_capacity !=
          physical_anchor_control_capacity_ ||
      audit_.physical_anchor_checkpoint_capacity !=
          physical_anchor_control_capacity_ ||
      audit_.physical_pending_anchor_count_capacity != 1U ||
      audit_.physical_device_arena_capacity_bytes !=
          expected_device_arena_capacity_bytes ||
      audit_.candidate_count >
          authorized_anchor_count *
              morton_yao48_device_tiled_pair_frontier_candidates_per_anchor ||
      audit_.certified_prune_region_count >
          authorized_anchor_count *
              morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor ||
      audit_.physical_candidate_capacity <
          authorized_anchor_count *
              morton_yao48_device_tiled_pair_frontier_candidates_per_anchor ||
      audit_.physical_prune_region_capacity <
          authorized_anchor_count *
              morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor ||
      audit_.process_restart_resumable ||
      (audit_.yield_reason ==
           MortonYao48DeviceTiledPairFrontierYieldReason::none) !=
          !audit_.resumable_after_lease_release ||
      !audit_.traversal_owner_retained ||
      !audit_.source_cloud_identity_retained ||
      audit_.source_device_views_retained !=
          audit_.cuda_device_storage_retained ||
      !audit_.source_device_extents_retained ||
      !audit_.source_views_bound_to_snapshot_identity ||
      !audit_.output_owner_retained ||
      !audit_.output_buffers_detached_for_tile_lifetime ||
      audit_
              .nonnegative_diametral_witness_interval_lower_bound_required !=
          closed_rank_semantics ||
      audit_
              .strictly_positive_diametral_witness_interval_lower_bound_required !=
          strict_interior_semantics ||
      audit_.q3_exact_diametral_pair_support_gabriel_negative_only !=
          strict_interior_semantics ||
      audit_.gamma2_silent_handoff_required !=
          strict_interior_semantics ||
      audit_.gamma2_prune_or_discard_authorized ||
      audit_.host_fake_lifecycle_exercised ==
          audit_.cuda_device_storage_retained ||
      audit_.candidate_device_to_host_performed ||
      audit_.certified_prune_device_to_host_performed ||
      audit_.exact_diametral_rank_evaluated ||
      audit_.scientific_pair_catalog_published ||
      audit_.dense_pair_fallback_performed ||
      audit_.global_pair_matrix_materialized ||
      audit_.higher_order_structure_materialized ||
      audit_.public_status_claimed) {
    return false;
  }
  if (host_fake_) {
    return audit_.host_fake_lifecycle_exercised &&
           !audit_.cuda_device_storage_retained &&
           device_coordinate_bits_ == nullptr &&
           device_morton_point_ids_ == nullptr &&
           device_nodes_ == nullptr &&
           device_candidate_records_ == nullptr &&
           device_certified_prune_regions_ == nullptr &&
           device_anchor_controls_ == nullptr &&
           cuda_device_ == -1;
  }
  return !audit_.host_fake_lifecycle_exercised &&
         audit_.cuda_device_storage_retained &&
         device_coordinate_bits_ != nullptr &&
         device_morton_point_ids_ != nullptr &&
         device_nodes_ != nullptr &&
         device_candidate_records_ != nullptr &&
         device_certified_prune_regions_ != nullptr &&
         device_anchor_controls_ != nullptr &&
         cuda_device_ >= 0;
}

bool MortonYao48DeviceCandidateTileLease::cuda_resident() const noexcept {
  return ready() && !host_fake_;
}

bool MortonYao48DeviceCandidateTileLease::host_fake() const noexcept {
  return ready() && host_fake_;
}

detail::Phase15MortonYao48DeviceCandidateTilePrivateViews
detail::Phase15MortonYao48DeviceCandidateTilePrivateViewAccess::inspect(
    const MortonYao48DeviceCandidateTileLease& lease) noexcept {
  Phase15MortonYao48DeviceCandidateTilePrivateViews views;
  if (!lease.ready()) {
    return views;
  }
  views.retained_authority_identity = lease.retained_owner_.get();
  views.source_owner_identity = lease.source_owner_authority_.get();
  views.source_cloud_identity =
      lease.source_cloud_identity_authority_.get();
  views.source_owner_authority = lease.source_owner_authority_;
  views.source_cloud_identity_authority =
      lease.source_cloud_identity_authority_;
  views.device_coordinate_bits = lease.device_coordinate_bits_;
  views.device_morton_point_ids = lease.device_morton_point_ids_;
  views.device_nodes = lease.device_nodes_;
  views.device_candidate_records =
      static_cast<const Phase15MortonYao48DeviceTiledCandidateRecord*>(
          lease.device_candidate_records_);
  views.device_prune_regions =
      static_cast<const Phase15MortonYao48DeviceTiledPruneRegionRecord*>(
          lease.device_certified_prune_regions_);
  views.device_anchor_controls =
      static_cast<const Phase15MortonYao48DeviceTiledAnchorControl*>(
          lease.device_anchor_controls_);
  views.source_snapshot_epoch = lease.audit_.source_snapshot_epoch;
  views.candidate_buffer_epoch = lease.audit_.candidate_buffer_epoch;
  views.point_count = lease.audit_.point_count;
  views.certified_node_count = lease.certified_node_count_;
  views.retained_coordinate_word_capacity =
      lease.retained_coordinate_word_capacity_;
  views.retained_morton_point_id_capacity =
      lease.retained_morton_point_id_capacity_;
  views.retained_node_capacity = lease.retained_node_capacity_;
  views.prune_semantics = lease.audit_.prune_semantics;
  views.required_witness_count = lease.audit_.required_witness_count;
  views.physical_candidate_record_capacity =
      lease.audit_.physical_candidate_capacity;
  views.candidate_segment_stride_records =
      lease.audit_.fixed_candidate_capacity_per_anchor;
  views.physical_anchor_control_capacity =
      lease.physical_anchor_control_capacity_;
  views.authorized_anchor_control_extent =
      lease.authorized_anchor_control_extent_;
  views.anchor_control_stride_bytes =
      sizeof(Phase15MortonYao48DeviceTiledAnchorControl);
  views.physical_device_arena_capacity_bytes =
      lease.audit_.physical_device_arena_capacity_bytes;
  views.anchor_begin = lease.audit_.anchor_begin;
  views.anchor_end = lease.audit_.anchor_end;
  views.cuda_device = lease.cuda_device_;
  views.host_fake = lease.host_fake_;
  views.source_device_views_retained =
      lease.audit_.source_device_views_retained;
  views.source_views_bound_to_snapshot_identity =
      lease.audit_.source_views_bound_to_snapshot_identity;
  views.ready = lease.ready();
  return views;
}

MortonYao48DeviceTiledPairFrontierContext::
    MortonYao48DeviceTiledPairFrontierContext(
        MortonLbvhDeviceTraversalLease&& traversal_lease,
        MortonYao48DeviceTiledPairFrontierConfig config)
    : config_(validate_config(config)) {
  if (!traversal_lease.ready()) {
    throw std::invalid_argument(
        "a Phase 15 device tiled Morton/Yao48 frontier requires a ready "
        "Phase 14 traversal lease");
  }
  const MortonLbvhDeviceTraversalLeaseAudit source_audit =
      traversal_lease.audit();
  AdoptedTraversal adopted =
      detail::adopt_phase15_morton_yao48_device_tiled_traversal(
          std::move(traversal_lease));
  validate_adopted_traversal(adopted, source_audit);

  point_count_ = adopted.point_count;
  certified_node_count_ = adopted.certified_node_count;
  unordered_pair_universe_count_ = checked_pair_universe(point_count_);
  next_anchor_position_ = point_count_ < 2U ? point_count_ : 1U;
  state_ = std::make_shared<
      detail::Phase15MortonYao48DeviceTiledPairFrontierContextState>();
  host_ = std::make_unique<
      detail::Phase15MortonYao48DeviceTiledPairFrontierHostState>(
      std::move(adopted));
}

MortonYao48DeviceTiledPairFrontierContext::
    ~MortonYao48DeviceTiledPairFrontierContext() noexcept = default;

MortonYao48DeviceTiledPairFrontierContext::
    MortonYao48DeviceTiledPairFrontierContext(
        MortonYao48DeviceTiledPairFrontierContext&&) noexcept = default;

MortonYao48DeviceTiledPairFrontierContext&
MortonYao48DeviceTiledPairFrontierContext::operator=(
    MortonYao48DeviceTiledPairFrontierContext&&) noexcept = default;

bool MortonYao48DeviceTiledPairFrontierContext::ready() const noexcept {
  return state_ != nullptr && host_ != nullptr &&
         !state_->poisoned() && host_->traversal.retained_owner != nullptr &&
         host_->traversal.source_cloud_identity != nullptr &&
         point_count_ == host_->traversal.point_count &&
         certified_node_count_ == host_->traversal.certified_node_count;
}

bool MortonYao48DeviceTiledPairFrontierContext::poisoned() const noexcept {
  return state_ != nullptr && state_->poisoned();
}

MortonYao48DeviceTiledPairFrontierAudit
MortonYao48DeviceTiledPairFrontierContext::make_audit(
    std::size_t transaction_anchor_begin,
    std::size_t transaction_anchor_end,
    std::size_t transaction_committed_anchor_count,
    std::size_t transaction_certified_prune_region_count,
    std::uint64_t transaction_ambiguous_cone_candidate_count,
    std::uint64_t transaction_unbanked_candidate_count,
    std::uint64_t transaction_candidate_pair_mass,
    std::uint64_t transaction_certified_pruned_pair_mass,
    std::uint64_t transaction_physical_node_visit_count,
    std::size_t transaction_traversal_subdivision_count,
    std::size_t transaction_physical_device_arena_capacity_bytes,
    std::uint64_t tile_epoch,
    std::uint64_t chunk_sequence,
    MortonYao48DeviceTiledPairFrontierYieldReason yield_reason,
    bool resumes_same_tile,
    bool resumable_capacity_yield,
    std::uint64_t candidate_buffer_epoch,
    std::size_t launcher_call_count,
    std::size_t cuda_kernel_launch_count,
    std::size_t cuda_synchronization_count,
    std::size_t anchor_control_device_to_host_count,
    std::size_t anchor_control_device_to_host_byte_count,
    std::size_t resume_control_device_to_host_count,
    std::size_t resume_control_device_to_host_byte_count,
    int cuda_device,
    bool candidate_tile_lease_published,
    bool censored_anchor_outputs_withheld) const noexcept {
  MortonYao48DeviceTiledPairFrontierAudit audit;
  audit.advance_sequence = advance_sequence_;
  audit.source_snapshot_epoch =
      host_ == nullptr ? 0U : host_->traversal.source_snapshot_epoch;
  audit.candidate_buffer_epoch = candidate_buffer_epoch;
  audit.point_count = point_count_;
  audit.certified_node_count = certified_node_count_;
  audit.maximum_closed_rank = config_.maximum_closed_rank;
  audit.prune_semantics = config_.prune_semantics;
  audit.required_witness_count =
      morton_yao48_device_tiled_pair_frontier_required_witness_count(
          config_.prune_semantics, config_.maximum_closed_rank);
  audit.anchor_tile_capacity = config_.anchor_tile_capacity;
  audit.fixed_node_visit_capacity_per_anchor =
      morton_yao48_device_tiled_pair_frontier_node_visits_per_anchor;
  audit.maximum_traversal_subdivision_count_per_anchor =
      certified_node_count_ == 0U
          ? 0U
          : certified_node_count_ /
                    morton_yao48_device_tiled_pair_frontier_node_visits_per_anchor +
                (certified_node_count_ %
                             morton_yao48_device_tiled_pair_frontier_node_visits_per_anchor ==
                         0U
                     ? 0U
                     : 1U);
  audit.fixed_candidate_capacity_per_anchor =
      morton_yao48_device_tiled_pair_frontier_candidates_per_anchor;
  audit.fixed_prune_region_capacity_per_anchor =
      morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor;
  audit.fixed_witness_bank_count_per_anchor =
      morton_yao48_device_tiled_pair_frontier_witness_bank_count;
  audit.tile_epoch = tile_epoch;
  audit.chunk_sequence = chunk_sequence;
  audit.yield_reason = yield_reason;
  audit.resumes_same_tile = resumes_same_tile;
  audit.resumable_capacity_yield = resumable_capacity_yield;
  audit.process_restart_resumable = false;
  audit.transaction_anchor_begin = transaction_anchor_begin;
  audit.transaction_anchor_end = transaction_anchor_end;
  audit.transaction_committed_anchor_count =
      transaction_committed_anchor_count;
  audit.transaction_certified_prune_region_count =
      transaction_certified_prune_region_count;
  audit.transaction_ambiguous_cone_candidate_count =
      transaction_ambiguous_cone_candidate_count;
  audit.transaction_unbanked_candidate_count =
      transaction_unbanked_candidate_count;
  audit.transaction_candidate_pair_mass =
      transaction_candidate_pair_mass;
  audit.transaction_certified_pruned_pair_mass =
      transaction_certified_pruned_pair_mass;
  audit.transaction_physical_node_visit_count =
      transaction_physical_node_visit_count;
  audit.transaction_traversal_subdivision_count =
      transaction_traversal_subdivision_count;
  audit.transaction_physical_device_arena_capacity_bytes =
      transaction_physical_device_arena_capacity_bytes;
  audit.completed_anchor_count = completed_anchor_count_;
  audit.next_anchor_position = next_anchor_position_;
  audit.unordered_pair_universe_count = unordered_pair_universe_count_;
  audit.cumulative_candidate_pair_mass = cumulative_candidate_pair_mass_;
  audit.cumulative_certified_pruned_pair_mass =
      cumulative_certified_pruned_pair_mass_;
  const std::uint64_t classified =
      cumulative_candidate_pair_mass_ +
      cumulative_certified_pruned_pair_mass_;
  audit.unresolved_pair_mass = unordered_pair_universe_count_ - classified;
  audit.cumulative_physical_node_visit_count =
      cumulative_physical_node_visit_count_;
  audit.cumulative_traversal_subdivision_count =
      cumulative_traversal_subdivision_count_;
  audit.launcher_call_count = launcher_call_count;
  audit.cuda_kernel_launch_count = cuda_kernel_launch_count;
  audit.cuda_synchronization_count = cuda_synchronization_count;
  audit.anchor_control_device_to_host_count =
      anchor_control_device_to_host_count;
  audit.anchor_control_device_to_host_byte_count =
      anchor_control_device_to_host_byte_count;
  audit.resume_control_device_to_host_count =
      resume_control_device_to_host_count;
  audit.resume_control_device_to_host_byte_count =
      resume_control_device_to_host_byte_count;
  audit.candidate_device_to_host_count = 0U;
  audit.certified_prune_device_to_host_count = 0U;
  audit.cuda_device = cuda_device;
  audit.source_traversal_lease_authenticated = host_ != nullptr;
  audit.fixed_per_anchor_caps_enforced = true;
  audit.atomic_completed_anchor_prefix_validated = true;
  audit.censored_anchor_outputs_withheld =
      censored_anchor_outputs_withheld;
  audit.candidate_pruned_unresolved_partition_validated = true;
  audit.pair_coverage_partition_complete =
      !terminally_censored_ && next_anchor_position_ >= point_count_ &&
      audit.unresolved_pair_mass == 0U;
  audit.q3_exact_diametral_pair_support_gabriel_lane_partition_complete =
      audit.pair_coverage_partition_complete &&
      config_.prune_semantics ==
          MortonYao48DeviceTiledPairFrontierPruneSemantics::
              strict_interior_threshold;
  audit.gamma2_silent_handoff_required =
      config_.prune_semantics ==
      MortonYao48DeviceTiledPairFrontierPruneSemantics::
          strict_interior_threshold;
  audit.gamma2_prune_or_discard_authorized = false;
  audit.terminally_censored = terminally_censored_;
  audit.traversal_lease_owner_retained =
      host_ != nullptr && host_->traversal.retained_owner != nullptr;
  audit.source_cloud_identity_retained =
      host_ != nullptr && host_->traversal.source_cloud_identity != nullptr;
  audit.candidate_tile_lease_published =
      candidate_tile_lease_published;
  audit.candidate_tile_lease_backpressure_bounded_to_one = true;
  audit.candidate_tile_lease_outstanding =
      !active_candidate_tile_authority_.expired();
  audit.output_buffers_detached_for_tile_lifetime =
      transaction_anchor_end > transaction_anchor_begin;
  audit.host_fake_launcher_exercised = host_fake_launcher_exercised_;
  audit.cuda_execution_performed = cuda_execution_performed_;
  audit.candidate_device_to_host_performed = false;
  audit.certified_prune_device_to_host_performed = false;
  audit.interval_cone_classification_required = true;
  audit.ambiguous_cone_routed_to_unbanked_candidate = true;
  audit.target_tested_before_witness_bank_insert = true;
  audit.retained_witnesses_outside_pruned_subtree_required = true;
  audit.nonnegative_diametral_witness_interval_lower_bound_required =
      config_.prune_semantics ==
      MortonYao48DeviceTiledPairFrontierPruneSemantics::closed_rank_window;
  audit.strictly_positive_diametral_witness_interval_lower_bound_required =
      config_.prune_semantics ==
      MortonYao48DeviceTiledPairFrontierPruneSemantics::
          strict_interior_threshold;
  return audit;
}

MortonYao48DeviceTiledPairFrontierAdvance
MortonYao48DeviceTiledPairFrontierContext::advance() {
  if (state_ == nullptr || host_ == nullptr) {
    throw std::logic_error(
        "a moved-from Phase 15 device tiled Morton/Yao48 context cannot "
        "advance");
  }
  return state_->with_launcher_section(
      [this]() {
        if (!active_candidate_tile_authority_.expired()) {
          throw std::logic_error(
              "the Phase 15 device tiled Morton/Yao48 context cannot launch "
              "while its preceding detached candidate tile is still alive");
        }
      },
      [this]() {
    if (terminally_censored_ || next_anchor_position_ >= point_count_) {
      ++advance_sequence_;
      MortonYao48DeviceTiledPairFrontierAdvance result;
      result.status = terminally_censored_
                          ? MortonYao48DeviceTiledPairFrontierStatus::censored
                          : MortonYao48DeviceTiledPairFrontierStatus::
                                frontier_complete;
      result.stop_reason = terminally_censored_
                               ? terminal_stop_reason_
                               : MortonYao48DeviceTiledPairFrontierStopReason::
                                     none;
      result.audit = make_audit(
          next_anchor_position_,
          next_anchor_position_,
          0U,
          0U,
          0U,
          0U,
          0U,
          0U,
          0U,
          0U,
          0U,
          0U,
          0U,
          MortonYao48DeviceTiledPairFrontierYieldReason::none,
          false,
          false,
          0U,
          launcher_call_count_,
          0U,
          0U,
          0U,
          0U,
          0U,
          0U,
          host_->traversal.cuda_device,
          false,
          terminally_censored_);
      return result;
    }

    const bool resume_same_tile = host_->active_tile;
    const std::uint64_t output_buffer_epoch = state_->advance_epoch();
    if (!host_->active_tile) {
      host_->active_tile = true;
      host_->active_tile_anchor_begin = next_anchor_position_;
      host_->active_tile_anchor_count = std::min(
          config_.anchor_tile_capacity,
          point_count_ - next_anchor_position_);
      host_->active_tile_epoch = output_buffer_epoch;
      host_->next_chunk_sequence = 1U;
      host_->anchor_progress.assign(
          host_->active_tile_anchor_count, AnchorProgress{});
    }
    const std::size_t anchor_begin = host_->active_tile_anchor_begin;
    const std::size_t anchor_count = host_->active_tile_anchor_count;
    const std::size_t anchor_end = checked_size_sum(
        anchor_begin,
        anchor_count,
        "the Phase 15 requested anchor tile overflows size_t");
    Request request;
    request.source_snapshot_epoch =
        host_->traversal.source_snapshot_epoch;
    request.output_buffer_epoch = output_buffer_epoch;
    request.tile_epoch = host_->active_tile_epoch;
    request.chunk_sequence = host_->next_chunk_sequence;
    request.point_count = point_count_;
    request.certified_node_count = certified_node_count_;
    request.anchor_begin = anchor_begin;
    request.anchor_count = anchor_count;
    request.maximum_closed_rank = config_.maximum_closed_rank;
    request.prune_semantics = config_.prune_semantics;
    request.required_witness_count =
        morton_yao48_device_tiled_pair_frontier_required_witness_count(
            config_.prune_semantics, config_.maximum_closed_rank);
    request.node_visit_capacity_per_anchor =
        morton_yao48_device_tiled_pair_frontier_node_visits_per_anchor;
    request.candidate_capacity_per_anchor =
        morton_yao48_device_tiled_pair_frontier_candidates_per_anchor;
    request.prune_region_capacity_per_anchor =
        morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor;
    request.witness_bank_count_per_anchor =
        morton_yao48_device_tiled_pair_frontier_witness_bank_count;
    request.witness_slot_count_per_bank =
        request.required_witness_count;
    request.resume_same_tile = resume_same_tile;

    DeviceBatch batch = detail::
        build_phase15_morton_yao48_device_tiled_pair_frontier_on_device(
            *state_, host_->traversal, request);
    const ValidatedBatch validated =
        validate_batch(
            batch,
            host_->traversal,
            request,
            host_->anchor_progress);

    const std::uint64_t new_candidate_mass = checked_u64_sum(
        cumulative_candidate_pair_mass_,
        validated.candidate_pair_mass,
        "the Phase 15 cumulative candidate mass overflowed uint64_t");
    const std::uint64_t new_pruned_mass = checked_u64_sum(
        cumulative_certified_pruned_pair_mass_,
        validated.certified_pruned_pair_mass,
        "the Phase 15 cumulative prune mass overflowed uint64_t");
    const std::uint64_t new_classified_mass = checked_u64_sum(
        new_candidate_mass,
        new_pruned_mass,
        "the Phase 15 cumulative classified mass overflowed uint64_t");
    if (new_classified_mass > unordered_pair_universe_count_) {
      throw std::runtime_error(
          "the Phase 15 committed prefix exceeds the unordered-pair "
          "universe");
    }
    const std::uint64_t new_physical_node_visit_count = checked_u64_sum(
        cumulative_physical_node_visit_count_,
        validated.physical_node_visit_count,
        "the Phase 15 cumulative node-visit count overflowed uint64_t");
    std::optional<MortonYao48DeviceCandidateTileLease> candidate_tile;
    if (!validated.fatal &&
        (validated.candidate_pair_mass != 0U ||
         validated.certified_prune_region_count != 0U)) {
      const bool host_fake =
          batch.execution_kind == ExecutionKind::host_fake;
      auto detached_authority = std::make_shared<DetachedTileAuthority>();
      detached_authority->traversal_owner =
          host_->traversal.retained_owner;
      detached_authority->source_cloud_identity =
          host_->traversal.source_cloud_identity;
      detached_authority->output_owner =
          std::move(batch.retained_output_owner);

      MortonYao48DeviceCandidateTileLeaseAudit lease_audit;
      lease_audit.source_snapshot_epoch = request.source_snapshot_epoch;
      lease_audit.candidate_buffer_epoch = request.output_buffer_epoch;
      lease_audit.point_count = point_count_;
      lease_audit.certified_node_count =
          host_->traversal.certified_node_count;
      lease_audit.retained_coordinate_word_capacity =
          host_->traversal.retained_coordinate_word_capacity;
      lease_audit.retained_morton_point_id_capacity =
          host_->traversal.retained_morton_point_id_capacity;
      lease_audit.retained_node_capacity =
          host_->traversal.retained_node_capacity;
      lease_audit.maximum_closed_rank = config_.maximum_closed_rank;
      lease_audit.prune_semantics = request.prune_semantics;
      lease_audit.required_witness_count = request.required_witness_count;
      lease_audit.tile_epoch = request.tile_epoch;
      lease_audit.chunk_sequence = request.chunk_sequence;
      lease_audit.anchor_begin = anchor_begin;
      lease_audit.anchor_end = anchor_end;
      lease_audit.candidate_count = checked_size(
          validated.candidate_pair_mass,
          "the Phase 15 committed candidate count does not fit size_t");
      lease_audit.certified_prune_region_count =
          validated.certified_prune_region_count;
      lease_audit.physical_candidate_capacity =
          batch.physical_candidate_capacity;
      lease_audit.physical_prune_region_capacity =
          batch.physical_prune_region_capacity;
      lease_audit.physical_witness_bank_slot_capacity =
          batch.physical_witness_bank_slot_capacity;
      lease_audit.physical_anchor_control_capacity =
          batch.physical_anchor_control_capacity;
      lease_audit.physical_anchor_checkpoint_capacity =
          batch.physical_anchor_checkpoint_capacity;
      lease_audit.physical_pending_anchor_count_capacity =
          batch.physical_pending_anchor_count_capacity;
      lease_audit.physical_device_arena_capacity_bytes =
          batch.physical_device_arena_capacity_bytes;
      lease_audit.fixed_candidate_capacity_per_anchor =
          request.candidate_capacity_per_anchor;
      lease_audit.fixed_prune_region_capacity_per_anchor =
          request.prune_region_capacity_per_anchor;
      lease_audit.fixed_witness_bank_count_per_anchor =
          request.witness_bank_count_per_anchor;
      lease_audit.yield_reason = validated.yield_reason;
      lease_audit.resumes_same_tile = request.resume_same_tile;
      lease_audit.resumable_after_lease_release =
          validated.capacity_yield;
      lease_audit.process_restart_resumable = false;
      lease_audit.traversal_owner_retained = true;
      lease_audit.source_cloud_identity_retained = true;
      lease_audit.source_device_views_retained = !host_fake;
      lease_audit.source_device_extents_retained = true;
      lease_audit.source_views_bound_to_snapshot_identity = true;
      lease_audit.output_owner_retained = true;
      lease_audit.output_buffers_detached_for_tile_lifetime = true;
      lease_audit.host_fake_lifecycle_exercised = host_fake;
      lease_audit.cuda_device_storage_retained = !host_fake;
      lease_audit.censored_anchor_outputs_withheld = validated.fatal;
      lease_audit.nonnegative_diametral_witness_interval_lower_bound_required =
          request.prune_semantics ==
          MortonYao48DeviceTiledPairFrontierPruneSemantics::
              closed_rank_window;
      lease_audit
          .strictly_positive_diametral_witness_interval_lower_bound_required =
          request.prune_semantics ==
          MortonYao48DeviceTiledPairFrontierPruneSemantics::
              strict_interior_threshold;
      lease_audit.q3_exact_diametral_pair_support_gabriel_negative_only =
          request.prune_semantics ==
          MortonYao48DeviceTiledPairFrontierPruneSemantics::
              strict_interior_threshold;
      lease_audit.gamma2_silent_handoff_required =
          request.prune_semantics ==
          MortonYao48DeviceTiledPairFrontierPruneSemantics::
              strict_interior_threshold;
      lease_audit.gamma2_prune_or_discard_authorized = false;

      MortonYao48DeviceCandidateTileLease detached_tile{
          std::move(lease_audit),
          detached_authority,
          detached_authority->traversal_owner,
          detached_authority->source_cloud_identity,
          host_->traversal.device_coordinate_bits,
          host_->traversal.device_morton_point_ids,
          host_->traversal.device_nodes,
          batch.device_candidate_records,
          batch.device_prune_regions,
          batch.device_anchor_controls,
          host_->traversal.certified_node_count,
          host_->traversal.retained_coordinate_word_capacity,
          host_->traversal.retained_morton_point_id_capacity,
          host_->traversal.retained_node_capacity,
          batch.physical_anchor_control_capacity,
          validated.authorized_anchor_count,
          batch.cuda_device,
          host_fake};
      candidate_tile.emplace(std::move(detached_tile));
      if (!candidate_tile->ready()) {
        throw std::runtime_error(
            "the Phase 15 output chunk did not form a valid detached "
            "device tile lease");
      }
      active_candidate_tile_authority_ = detached_authority;
    }

    cumulative_candidate_pair_mass_ = new_candidate_mass;
    cumulative_certified_pruned_pair_mass_ = new_pruned_mass;
    cumulative_physical_node_visit_count_ =
        new_physical_node_visit_count;
    cumulative_traversal_subdivision_count_ = checked_size_sum(
        cumulative_traversal_subdivision_count_,
        batch.traversal_subdivision_count,
        "the Phase 15 cumulative traversal subdivision count overflows size_t");
    host_->anchor_progress = validated.next_progress;
    if (validated.tile_complete) {
      completed_anchor_count_ = checked_size_sum(
          completed_anchor_count_,
          validated.completed_anchor_count,
          "the Phase 15 completed anchor count overflows size_t");
      next_anchor_position_ = anchor_end;
      host_->active_tile = false;
      host_->anchor_progress.clear();
    } else if (validated.capacity_yield) {
      if (host_->next_chunk_sequence ==
          std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error(
            "the Phase 15 tile chunk sequence overflowed uint64_t");
      }
      ++host_->next_chunk_sequence;
    } else if (validated.fatal) {
      host_->active_tile = false;
      host_->anchor_progress.clear();
    }
    terminally_censored_ = validated.fatal;
    terminal_stop_reason_ = validated.stop_reason;
    ++launcher_call_count_;
    ++advance_sequence_;
    host_fake_launcher_exercised_ =
        host_fake_launcher_exercised_ ||
        batch.execution_kind == ExecutionKind::host_fake;
    cuda_execution_performed_ =
        cuda_execution_performed_ ||
        batch.execution_kind == ExecutionKind::cuda;

    MortonYao48DeviceTiledPairFrontierAdvance result;
    if (validated.fatal) {
      result.status = MortonYao48DeviceTiledPairFrontierStatus::censored;
      result.stop_reason = validated.stop_reason;
    } else if (validated.capacity_yield) {
      result.status =
          MortonYao48DeviceTiledPairFrontierStatus::chunk_ready;
      result.stop_reason =
          MortonYao48DeviceTiledPairFrontierStopReason::none;
      result.yield_reason = validated.yield_reason;
    } else if (next_anchor_position_ >= point_count_) {
      result.status =
          MortonYao48DeviceTiledPairFrontierStatus::frontier_complete;
      result.stop_reason =
          MortonYao48DeviceTiledPairFrontierStopReason::none;
    } else {
      result.status =
          MortonYao48DeviceTiledPairFrontierStatus::tile_complete;
      result.stop_reason =
          MortonYao48DeviceTiledPairFrontierStopReason::none;
    }
    result.candidate_tile = std::move(candidate_tile);
    result.audit = make_audit(
        anchor_begin,
        anchor_end,
        validated.completed_anchor_count,
        validated.certified_prune_region_count,
        validated.ambiguous_cone_candidate_count,
        validated.unbanked_candidate_count,
        validated.candidate_pair_mass,
        validated.certified_pruned_pair_mass,
        validated.physical_node_visit_count,
        batch.traversal_subdivision_count,
        batch.physical_device_arena_capacity_bytes,
        request.tile_epoch,
        request.chunk_sequence,
        validated.yield_reason,
        request.resume_same_tile,
        validated.capacity_yield,
        request.output_buffer_epoch,
        launcher_call_count_,
        batch.kernel_launch_count,
        batch.synchronization_count,
        batch.anchor_control_device_to_host_count,
        batch.anchor_control_device_to_host_byte_count,
        batch.resume_control_device_to_host_count,
        batch.resume_control_device_to_host_byte_count,
        batch.cuda_device,
        result.candidate_tile.has_value(),
        validated.fatal);
    return result;
  });
}

}  // namespace morsehgp3d::gpu
