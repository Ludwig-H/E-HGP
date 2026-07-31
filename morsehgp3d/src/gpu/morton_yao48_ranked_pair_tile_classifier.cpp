#include "morsehgp3d/gpu/morton_yao48_ranked_pair_tile_classifier.hpp"

#include "phase15_morton_yao48_ranked_pair_tile_classifier_internal.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::gpu {
namespace {

using detail::Phase15MortonYao48DeviceCandidateTilePrivateViewAccess;
using detail::Phase15MortonYao48DeviceCandidateTilePrivateViews;
using detail::Phase15MortonYao48RankedPairTileClassifierExecutionKind;
using detail::Phase15MortonYao48RankedPairTileClassifierFailureCode;
using detail::Phase15MortonYao48RankedPairTileClassifierReceipt;
using detail::Phase15MortonYao48RankedPairTileClassifierRequest;

[[nodiscard]] MortonYao48RankedPairTileClassifierConfig validate_config(
    MortonYao48RankedPairTileClassifierConfig config) {
  if (config.maximum_closed_rank <
          morton_yao48_ranked_pair_tile_classifier_minimum_closed_rank ||
      config.maximum_closed_rank >
          morton_yao48_ranked_pair_tile_classifier_maximum_closed_rank) {
    throw std::out_of_range(
        "the resident ranked-pair tile classifier closed-rank window must "
        "be in [2, 11]");
  }
  return config;
}

[[nodiscard]] std::uint64_t checked_sum(
    std::uint64_t left,
    std::uint64_t right,
    const char* message) {
  if (right > std::numeric_limits<std::uint64_t>::max() - left) {
    throw std::runtime_error(message);
  }
  return left + right;
}

[[nodiscard]] std::uint64_t checked_sum3(
    std::uint64_t first,
    std::uint64_t second,
    std::uint64_t third,
    const char* message) {
  return checked_sum(checked_sum(first, second, message), third, message);
}

[[nodiscard]] std::size_t checked_size(
    std::uint64_t value,
    const char* message) {
  if (value > static_cast<std::uint64_t>(
                  std::numeric_limits<std::size_t>::max())) {
    throw std::runtime_error(message);
  }
  return static_cast<std::size_t>(value);
}

struct CandidateAuthority final {
  Phase15MortonYao48DeviceCandidateTilePrivateViews views{};
  MortonYao48DeviceCandidateTileLeaseAudit audit{};
  bool tile_closed_after_chunk{false};
  bool frontier_closed_after_chunk{false};
};

struct FrontierAuthority final {
  MortonYao48DeviceTiledPairFrontierAudit audit{};
  MortonYao48DeviceTiledPairFrontierStatus status{
      MortonYao48DeviceTiledPairFrontierStatus::censored};
  bool tile_closed_after_chunk{false};
  bool frontier_closed_after_chunk{false};
};

[[nodiscard]] FrontierAuthority inspect_frontier_authority(
    const MortonYao48DeviceTiledPairFrontierAdvance& advance) {
  const auto& audit = advance.audit;
  if (advance.status == MortonYao48DeviceTiledPairFrontierStatus::censored ||
      advance.stop_reason !=
          MortonYao48DeviceTiledPairFrontierStopReason::none ||
      advance.yield_reason != audit.yield_reason ||
      audit.schema_version !=
          morton_yao48_device_tiled_pair_frontier_schema_version ||
      audit.source_snapshot_epoch == 0U || audit.point_count == 0U ||
      audit.maximum_closed_rank <
          morton_yao48_ranked_pair_tile_classifier_minimum_closed_rank ||
      audit.maximum_closed_rank >
          morton_yao48_ranked_pair_tile_classifier_maximum_closed_rank ||
      audit.prune_semantics !=
          MortonYao48DeviceTiledPairFrontierPruneSemantics::
              closed_rank_window ||
      audit.required_witness_count != audit.maximum_closed_rank - 1U ||
      !audit.nonnegative_diametral_witness_interval_lower_bound_required ||
      audit.strictly_positive_diametral_witness_interval_lower_bound_required ||
      audit.q3_exact_diametral_pair_support_gabriel_lane_partition_complete ||
      audit.gamma2_silent_handoff_required ||
      audit.gamma2_prune_or_discard_authorized ||
      !audit.source_traversal_lease_authenticated ||
      !audit.fixed_per_anchor_caps_enforced ||
      !audit.atomic_completed_anchor_prefix_validated ||
      !audit.candidate_pruned_unresolved_partition_validated ||
      !audit.traversal_lease_owner_retained ||
      !audit.source_cloud_identity_retained ||
      audit.terminally_censored || audit.process_restart_resumable ||
      audit.candidate_device_to_host_count != 0U ||
      audit.certified_prune_device_to_host_count != 0U ||
      audit.candidate_device_to_host_performed ||
      audit.certified_prune_device_to_host_performed ||
      audit.exact_diametral_rank_evaluated ||
      audit.scientific_pair_catalog_published ||
      audit.scientific_decision_published ||
      audit.dense_pair_fallback_performed ||
      audit.global_pair_matrix_materialized ||
      audit.ordinary_delaunay_materialized ||
      audit.higher_order_delaunay_mosaic_materialized ||
      audit.global_cell_or_coface_arena_materialized ||
      audit.public_status_claimed ||
      checked_sum3(
          audit.cumulative_candidate_pair_mass,
          audit.cumulative_certified_pruned_pair_mass,
          audit.unresolved_pair_mass,
          "the frontier coverage mass overflowed") !=
          audit.unordered_pair_universe_count) {
    throw std::runtime_error(
        "the ranked-pair classifier rejected a malformed frontier advance");
  }

  const bool yielded =
      advance.status ==
      MortonYao48DeviceTiledPairFrontierStatus::chunk_ready;
  const bool tile_closed =
      advance.status ==
          MortonYao48DeviceTiledPairFrontierStatus::tile_complete ||
      advance.status ==
          MortonYao48DeviceTiledPairFrontierStatus::frontier_complete;
  if ((!yielded && !tile_closed) ||
      yielded != audit.resumable_capacity_yield ||
      yielded !=
          (audit.yield_reason !=
           MortonYao48DeviceTiledPairFrontierYieldReason::none) ||
      (tile_closed &&
       audit.yield_reason !=
           MortonYao48DeviceTiledPairFrontierYieldReason::none) ||
      (advance.status ==
           MortonYao48DeviceTiledPairFrontierStatus::frontier_complete &&
       (!audit.pair_coverage_partition_complete ||
        audit.unresolved_pair_mass != 0U))) {
    throw std::runtime_error(
        "the frontier status does not authenticate its yield or closure");
  }
  if (!advance.candidate_tile.has_value() &&
      (audit.transaction_candidate_pair_mass != 0U ||
       audit.transaction_certified_pruned_pair_mass != 0U ||
       audit.transaction_certified_prune_region_count != 0U)) {
    throw std::runtime_error(
        "a nonempty frontier transaction omitted its resident tile lease");
  }
  FrontierAuthority authority;
  authority.audit = audit;
  authority.status = advance.status;
  authority.tile_closed_after_chunk = tile_closed;
  authority.frontier_closed_after_chunk =
      advance.status ==
      MortonYao48DeviceTiledPairFrontierStatus::frontier_complete;
  return authority;
}

[[nodiscard]] CandidateAuthority inspect_candidate_authority(
    const MortonYao48DeviceCandidateTileLease& lease,
    const MortonYao48RankedPairTileClassifierConfig& config) {
  if (!lease.ready()) {
    throw std::invalid_argument(
        "the resident ranked-pair tile classifier requires a ready "
        "candidate lease");
  }
  CandidateAuthority authority;
  authority.views =
      Phase15MortonYao48DeviceCandidateTilePrivateViewAccess::inspect(lease);
  authority.audit = lease.audit();
  const auto& views = authority.views;
  const auto& audit = authority.audit;
  if (!views.ready || views.retained_authority_identity == nullptr ||
      views.source_owner_identity == nullptr ||
      views.source_cloud_identity == nullptr ||
      !views.source_owner_authority ||
      !views.source_cloud_identity_authority ||
      views.source_owner_authority.get() != views.source_owner_identity ||
      views.source_cloud_identity_authority.get() !=
          views.source_cloud_identity ||
      audit.schema_version !=
          morton_yao48_device_tiled_pair_frontier_schema_version ||
      audit.source_snapshot_epoch == 0U ||
      audit.candidate_buffer_epoch == 0U || audit.tile_epoch == 0U ||
      audit.chunk_sequence == 0U || audit.point_count < 2U ||
      audit.certified_node_count == 0U || audit.anchor_begin == 0U ||
      audit.anchor_begin >= audit.anchor_end ||
      audit.anchor_end > audit.point_count ||
      audit.maximum_closed_rank != config.maximum_closed_rank ||
      audit.prune_semantics !=
          MortonYao48DeviceTiledPairFrontierPruneSemantics::
              closed_rank_window ||
      audit.required_witness_count != config.maximum_closed_rank - 1U ||
      !audit.nonnegative_diametral_witness_interval_lower_bound_required ||
      audit.strictly_positive_diametral_witness_interval_lower_bound_required ||
      audit.q3_exact_diametral_pair_support_gabriel_negative_only ||
      audit.gamma2_silent_handoff_required ||
      audit.gamma2_prune_or_discard_authorized ||
      audit.candidate_count > audit.physical_candidate_capacity ||
      audit.fixed_candidate_capacity_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_candidates_per_anchor ||
      audit.fixed_prune_region_capacity_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor ||
      audit.fixed_witness_bank_count_per_anchor !=
          morton_yao48_device_tiled_pair_frontier_witness_bank_count ||
      audit.process_restart_resumable || !audit.traversal_owner_retained ||
      !audit.source_cloud_identity_retained ||
      audit.source_device_views_retained !=
          audit.cuda_device_storage_retained ||
      !audit.source_device_extents_retained ||
      !audit.source_views_bound_to_snapshot_identity ||
      !audit.output_owner_retained ||
      !audit.output_buffers_detached_for_tile_lifetime ||
      audit.candidate_device_to_host_performed ||
      audit.certified_prune_device_to_host_performed ||
      audit.exact_diametral_rank_evaluated ||
      audit.scientific_pair_catalog_published ||
      audit.dense_pair_fallback_performed ||
      audit.global_pair_matrix_materialized ||
      audit.higher_order_structure_materialized ||
      audit.public_status_claimed ||
      views.source_snapshot_epoch != audit.source_snapshot_epoch ||
      views.candidate_buffer_epoch != audit.candidate_buffer_epoch ||
      views.point_count != audit.point_count ||
      views.certified_node_count != audit.certified_node_count ||
      views.prune_semantics != audit.prune_semantics ||
      views.required_witness_count != audit.required_witness_count ||
      views.anchor_begin != audit.anchor_begin ||
      views.anchor_end != audit.anchor_end ||
      views.physical_candidate_record_capacity !=
          audit.physical_candidate_capacity ||
      views.candidate_segment_stride_records !=
          audit.fixed_candidate_capacity_per_anchor ||
      views.physical_anchor_control_capacity !=
          audit.physical_anchor_control_capacity ||
      views.physical_device_arena_capacity_bytes !=
          audit.physical_device_arena_capacity_bytes ||
      views.source_device_views_retained != !views.host_fake ||
      !views.source_views_bound_to_snapshot_identity ||
      views.host_fake != lease.host_fake()) {
    throw std::runtime_error(
        "the resident ranked-pair tile classifier rejected malformed "
        "candidate authority");
  }
  const bool yielded =
      audit.yield_reason !=
      MortonYao48DeviceTiledPairFrontierYieldReason::none;
  if (yielded != audit.resumable_after_lease_release ||
      (yielded && audit.censored_anchor_outputs_withheld)) {
    throw std::runtime_error(
        "the candidate lease yield and resumability claims disagree");
  }
  authority.tile_closed_after_chunk = !yielded;
  authority.frontier_closed_after_chunk =
      authority.tile_closed_after_chunk &&
      audit.anchor_end == audit.point_count;
  return authority;
}

void validate_sequence(
    const MortonYao48DeviceTiledPairFrontierAudit& audit,
    const Phase15MortonYao48DeviceCandidateTilePrivateViews* views,
    bool frontier_closed_after_chunk,
    bool initialized,
    bool prior_tile_resumable,
    std::uint64_t last_candidate_buffer_epoch,
    std::uint64_t last_tile_epoch,
    std::uint64_t last_chunk_sequence,
    std::size_t last_anchor_begin,
    std::size_t last_anchor_end,
    const void* source_owner_identity,
    const void* source_cloud_identity,
    std::uint64_t source_snapshot_epoch,
    std::size_t point_count,
    std::size_t certified_node_count) {
  if (!initialized) {
    const bool trivial_empty_frontier =
        frontier_closed_after_chunk && audit.unordered_pair_universe_count == 0U &&
        audit.transaction_candidate_pair_mass == 0U &&
        audit.transaction_certified_pruned_pair_mass == 0U;
    if ((!trivial_empty_frontier &&
         (audit.tile_epoch != 1U || audit.chunk_sequence != 1U ||
          audit.transaction_anchor_begin != 1U || audit.resumes_same_tile)) ||
        (trivial_empty_frontier &&
         (audit.tile_epoch != 0U || audit.chunk_sequence != 0U))) {
      throw std::runtime_error(
          "the ranked-pair classifier journal did not start at tile one, "
          "chunk one, anchor one");
    }
    return;
  }
  if ((views != nullptr && source_owner_identity != nullptr &&
       (views->source_owner_identity != source_owner_identity ||
        views->source_cloud_identity != source_cloud_identity)) ||
      audit.source_snapshot_epoch != source_snapshot_epoch ||
      audit.point_count != point_count ||
      audit.certified_node_count != certified_node_count ||
      audit.candidate_buffer_epoch < last_candidate_buffer_epoch ||
      (views != nullptr &&
       audit.candidate_buffer_epoch == last_candidate_buffer_epoch)) {
    throw std::runtime_error(
        "a ranked-pair classifier continuation changed its source identity "
        "or rolled back its candidate epoch");
  }
  if (prior_tile_resumable) {
    if (audit.tile_epoch != last_tile_epoch ||
        last_chunk_sequence == std::numeric_limits<std::uint64_t>::max() ||
        audit.chunk_sequence != last_chunk_sequence + 1U ||
        audit.transaction_anchor_begin != last_anchor_begin ||
        audit.transaction_anchor_end != last_anchor_end ||
        !audit.resumes_same_tile) {
      throw std::runtime_error(
          "a resumed ranked-pair tile changed its extent, epoch, or chunk "
          "sequence");
    }
    return;
  }
  if (last_tile_epoch == std::numeric_limits<std::uint64_t>::max() ||
      audit.tile_epoch != last_tile_epoch + 1U ||
      audit.chunk_sequence != 1U ||
      audit.transaction_anchor_begin != last_anchor_end ||
      audit.resumes_same_tile) {
    throw std::runtime_error(
        "a new ranked-pair tile is not the monotone successor of the prior "
        "closed tile");
  }
}

[[nodiscard]] bool pointer_present_for_count(
    const void* pointer,
    std::size_t count) noexcept {
  return count == 0U || pointer != nullptr;
}

[[nodiscard]] bool host_fake_catalog_is_canonical(
    const detail::Phase15MortonYao48RankedPairTileClassifierDeviceOutput&
        output,
    std::size_t point_count,
    std::size_t maximum_closed_rank) noexcept {
  const std::size_t record_count = output.catalog_record_count;
  if (output.rank_offsets[0U] != 0U ||
      output.strict_offsets[0U] != 0U ||
      output.shell_offsets[0U] != 0U ||
      output.strict_offsets[record_count] !=
          static_cast<std::uint64_t>(output.strict_point_id_count) ||
      output.shell_offsets[record_count] !=
          static_cast<std::uint64_t>(output.shell_point_id_count)) {
    return false;
  }

  for (std::size_t index = 0U; index < record_count; ++index) {
    const std::uint64_t support_u = output.support_u[index];
    const std::uint64_t support_v = output.support_v[index];
    const std::size_t closed_rank = output.closed_rank[index];
    if (support_u >= support_v || support_v >= point_count ||
        closed_rank <
            morton_yao48_ranked_pair_tile_classifier_minimum_closed_rank ||
        closed_rank > maximum_closed_rank) {
      return false;
    }
    if (index != 0U) {
      const std::size_t prior_rank = output.closed_rank[index - 1U];
      const std::uint64_t prior_u = output.support_u[index - 1U];
      const std::uint64_t prior_v = output.support_v[index - 1U];
      if (closed_rank < prior_rank ||
          (closed_rank == prior_rank &&
           (support_u < prior_u ||
            (support_u == prior_u && support_v <= prior_v)))) {
        return false;
      }
    }

    const std::uint64_t strict_begin = output.strict_offsets[index];
    const std::uint64_t strict_end = output.strict_offsets[index + 1U];
    const std::uint64_t shell_begin = output.shell_offsets[index];
    const std::uint64_t shell_end = output.shell_offsets[index + 1U];
    if (strict_begin > strict_end ||
        strict_end > output.strict_point_id_count ||
        shell_begin > shell_end ||
        shell_end > output.shell_point_id_count) {
      return false;
    }
    const std::uint64_t strict_count = strict_end - strict_begin;
    const std::uint64_t shell_count = shell_end - shell_begin;
    if (strict_count + shell_count != closed_rank - 2U) {
      return false;
    }

    for (std::uint64_t payload = strict_begin; payload < strict_end;
         ++payload) {
      const std::uint64_t point_id = output.strict_point_ids[payload];
      if (point_id >= point_count || point_id == support_u ||
          point_id == support_v ||
          (payload != strict_begin &&
           output.strict_point_ids[payload - 1U] >= point_id)) {
        return false;
      }
    }
    for (std::uint64_t payload = shell_begin; payload < shell_end;
         ++payload) {
      const std::uint64_t point_id = output.shell_point_ids[payload];
      if (point_id >= point_count || point_id == support_u ||
          point_id == support_v ||
          (payload != shell_begin &&
           output.shell_point_ids[payload - 1U] >= point_id)) {
        return false;
      }
      for (std::uint64_t strict_payload = strict_begin;
           strict_payload < strict_end;
           ++strict_payload) {
        if (output.strict_point_ids[strict_payload] == point_id) {
          return false;
        }
      }
    }
  }

  std::size_t record_cursor = 0U;
  for (std::size_t rank = 0U; rank <= maximum_closed_rank + 1U; ++rank) {
    while (record_cursor < record_count &&
           output.closed_rank[record_cursor] < rank) {
      ++record_cursor;
    }
    if (output.rank_offsets[rank] != record_cursor) {
      return false;
    }
  }
  return record_cursor == record_count;
}

struct ValidatedReceipt final {
  Phase15MortonYao48RankedPairTileClassifierReceipt receipt;
  MortonYao48RankedPairTileClassifierStatus status{};
  MortonYao48RankedPairTileClassifierStopReason stop_reason{};
};

[[nodiscard]] ValidatedReceipt validate_receipt(
    Phase15MortonYao48RankedPairTileClassifierReceipt receipt,
    const Phase15MortonYao48RankedPairTileClassifierRequest& request,
    const Phase15MortonYao48DeviceCandidateTilePrivateViews& views) {
  if (receipt.receipt_digest !=
      detail::
          phase15_morton_yao48_ranked_pair_tile_classifier_receipt_digest(
              receipt)) {
    throw std::runtime_error(
        "the resident ranked-pair classifier receipt digest is invalid");
  }
  if (receipt.schema_version !=
          morton_yao48_ranked_pair_tile_classifier_schema_version ||
      receipt.fixed_config != request.fixed_config ||
      receipt.source_snapshot_epoch != request.source_snapshot_epoch ||
      receipt.candidate_buffer_epoch != request.candidate_buffer_epoch ||
      receipt.catalog_buffer_epoch != request.catalog_buffer_epoch ||
      receipt.tile_epoch != request.tile_epoch ||
      receipt.chunk_sequence != request.chunk_sequence ||
      receipt.point_count != request.point_count ||
      receipt.certified_node_count != request.certified_node_count ||
      receipt.anchor_begin != request.anchor_begin ||
      receipt.anchor_end != request.anchor_end ||
      receipt.transaction_candidate_count != request.candidate_count ||
      receipt.transaction_certified_pruned_pair_mass !=
          request.transaction_certified_pruned_pair_mass ||
      receipt.cumulative_certified_pruned_pair_mass !=
          request.cumulative_certified_pruned_pair_mass ||
      receipt.unordered_pair_universe_count !=
          request.unordered_pair_universe_count ||
      receipt.frontier_unresolved_pair_mass !=
          request.frontier_unresolved_pair_mass ||
      receipt.same_tile_continuation != request.same_tile_continuation ||
      receipt.tile_closed_after_chunk != request.tile_closed_after_chunk ||
      receipt.frontier_closed_after_chunk !=
          request.frontier_closed_after_chunk ||
      !receipt.source_identity_authenticated ||
      !receipt.source_epoch_authenticated ||
      !receipt.candidate_epoch_authenticated ||
      !receipt.tile_chunk_sequence_authenticated ||
      !receipt.candidate_tile_lease_alive_during_launch ||
      !receipt.fixed_linear_capacities_validated ||
      !receipt.exact_multiorder_classification_implemented ||
      !receipt.one_multiorder_launch_used ||
      !receipt.count_scan_payload_layout_validated ||
      !receipt.output_rollback_validated ||
      receipt.launcher_call_count != 1U ||
      receipt.candidate_device_to_host_count != 0U ||
      receipt.certified_prune_device_to_host_count != 0U ||
      receipt.legacy_pair_callback_count != 0U ||
      receipt.candidate_device_to_host_performed ||
      receipt.certified_prune_device_to_host_performed ||
      receipt.callback_per_pair_performed || receipt.dense_pair_scan_performed ||
      receipt.global_pair_matrix_materialized ||
      receipt.ordinary_delaunay_materialized ||
      receipt.higher_order_delaunay_mosaic_materialized ||
      receipt.scientific_decision_published || receipt.public_status_claimed) {
    throw std::runtime_error(
        "the resident ranked-pair classifier receipt violates its resident "
        "execution contract");
  }
  const bool expected_host_fake = views.host_fake;
  if ((expected_host_fake &&
       (receipt.execution_kind !=
            Phase15MortonYao48RankedPairTileClassifierExecutionKind::
                host_fake ||
        receipt.cuda_device != -1)) ||
      (!expected_host_fake &&
       (receipt.execution_kind !=
            Phase15MortonYao48RankedPairTileClassifierExecutionKind::cuda ||
        receipt.cuda_device != views.cuda_device ||
        receipt.cuda_device < 0))) {
    throw std::runtime_error(
        "the ranked-pair classifier execution kind does not match the "
        "candidate lease");
  }

  const std::uint64_t transaction_partition = checked_sum3(
      receipt.transaction_accepted_record_count,
      receipt.transaction_above_window_count,
      receipt.transaction_unresolved_count,
      "the ranked-pair transaction mass overflowed");
  if (transaction_partition != receipt.transaction_candidate_count ||
      receipt.cumulative_candidate_count !=
          checked_sum(
              request.prior_candidate_count,
              receipt.transaction_candidate_count,
              "the cumulative candidate count overflowed") ||
      receipt.cumulative_accepted_record_count !=
          checked_sum(
              request.prior_accepted_record_count,
              receipt.transaction_accepted_record_count,
              "the cumulative accepted count overflowed") ||
      receipt.cumulative_above_window_count !=
          checked_sum(
              request.prior_above_window_count,
              receipt.transaction_above_window_count,
              "the cumulative above-window count overflowed") ||
      receipt.cumulative_unresolved_count !=
          checked_sum(
              request.prior_unresolved_count,
              receipt.transaction_unresolved_count,
              "the cumulative unresolved count overflowed") ||
      receipt.cumulative_strict_payload_point_id_count !=
          checked_sum(
              request.prior_strict_payload_point_id_count,
              receipt.transaction_strict_payload_point_id_count,
              "the cumulative strict payload count overflowed") ||
      receipt.cumulative_shell_payload_point_id_count !=
          checked_sum(
              request.prior_shell_payload_point_id_count,
              receipt.transaction_shell_payload_point_id_count,
              "the cumulative shell payload count overflowed") ||
      receipt.cumulative_exact_fallback_count !=
          checked_sum(
              request.prior_exact_fallback_count,
              receipt.transaction_exact_fallback_count,
              "the cumulative exact fallback count overflowed") ||
      checked_sum3(
          receipt.cumulative_accepted_record_count,
          receipt.cumulative_above_window_count,
          receipt.cumulative_unresolved_count,
          "the cumulative ranked-pair mass overflowed") !=
          receipt.cumulative_candidate_count ||
      checked_sum3(
          checked_sum(
              receipt.cumulative_accepted_record_count,
              receipt.cumulative_above_window_count,
              "the global classified mass overflowed"),
          checked_sum(
              receipt.cumulative_certified_pruned_pair_mass,
              receipt.cumulative_unresolved_count,
              "the global closed mass overflowed"),
          receipt.frontier_unresolved_pair_mass,
          "the global frontier/classifier mass overflowed") !=
          receipt.unordered_pair_universe_count) {
    throw std::runtime_error(
        "the ranked-pair classifier receipt rolled back or failed a mass "
        "identity");
  }

  const auto status =
      static_cast<MortonYao48RankedPairTileClassifierStatus>(receipt.status);
  const auto stop_reason =
      static_cast<MortonYao48RankedPairTileClassifierStopReason>(
          receipt.stop_reason);
  const auto failure =
      static_cast<Phase15MortonYao48RankedPairTileClassifierFailureCode>(
          receipt.failure_code);
  const std::uint64_t payload_count = checked_sum(
      receipt.cumulative_strict_payload_point_id_count,
      receipt.cumulative_shell_payload_point_id_count,
      "the cumulative payload count overflowed");
  const auto& output = receipt.output;

  if (status ==
          MortonYao48RankedPairTileClassifierStatus::chunk_committed ||
      status ==
          MortonYao48RankedPairTileClassifierStatus::frontier_closed) {
    const auto expected_status = request.frontier_closed_after_chunk
                                     ? MortonYao48RankedPairTileClassifierStatus::
                                           frontier_closed
                                     : MortonYao48RankedPairTileClassifierStatus::
                                           chunk_committed;
    if (status != expected_status ||
        stop_reason != MortonYao48RankedPairTileClassifierStopReason::none ||
        failure !=
            Phase15MortonYao48RankedPairTileClassifierFailureCode::none ||
        receipt.transaction_unresolved_count != 0U ||
        receipt.cumulative_unresolved_count != 0U ||
        receipt.output_invalidated || !receipt.output_records_sorted_unique ||
        receipt.required_catalog_record_count !=
            receipt.cumulative_accepted_record_count ||
        receipt.required_payload_point_id_count != payload_count ||
        receipt.required_exact_fallback_count !=
            receipt.cumulative_exact_fallback_count ||
        receipt.required_catalog_record_count >
            request.fixed_config.catalog_record_capacity ||
        receipt.required_payload_point_id_count >
            request.fixed_config.payload_point_id_capacity ||
        receipt.required_exact_fallback_count >
            request.fixed_config.exact_fallback_capacity ||
        !output.owner ||
        output.catalog_record_capacity !=
            request.fixed_config.catalog_record_capacity ||
        output.payload_point_id_capacity !=
            request.fixed_config.payload_point_id_capacity ||
        output.exact_fallback_capacity !=
            request.fixed_config.exact_fallback_capacity ||
        output.catalog_record_count != checked_size(
            receipt.cumulative_accepted_record_count,
            "the catalog record count does not fit size_t") ||
        output.strict_point_id_count != checked_size(
            receipt.cumulative_strict_payload_point_id_count,
            "the strict payload count does not fit size_t") ||
        output.shell_point_id_count != checked_size(
            receipt.cumulative_shell_payload_point_id_count,
            "the shell payload count does not fit size_t") ||
        output.rank_offset_count !=
            request.fixed_config.maximum_closed_rank + 2U ||
        output.record_offset_count != output.catalog_record_count + 1U ||
        !pointer_present_for_count(
            output.support_u, output.catalog_record_count) ||
        !pointer_present_for_count(
            output.support_v, output.catalog_record_count) ||
        !pointer_present_for_count(
            output.closed_rank, output.catalog_record_count) ||
        output.rank_offsets == nullptr || output.strict_offsets == nullptr ||
        output.shell_offsets == nullptr ||
        !pointer_present_for_count(
            output.strict_point_ids, output.strict_point_id_count) ||
        !pointer_present_for_count(
            output.shell_point_ids, output.shell_point_id_count)) {
      throw std::runtime_error(
          "a successful ranked-pair classifier receipt is not exact, "
          "closed, or layout-complete");
    }
    if (expected_host_fake &&
        !host_fake_catalog_is_canonical(
            output,
            request.point_count,
            request.fixed_config.maximum_closed_rank)) {
      throw std::runtime_error(
          "a successful host-fake ranked-pair catalog is not canonical");
    }
  } else if (
      status ==
      MortonYao48RankedPairTileClassifierStatus::capacity_exhausted) {
    const bool record_exhausted =
        stop_reason ==
            MortonYao48RankedPairTileClassifierStopReason::
                catalog_record_capacity &&
        receipt.required_catalog_record_count >
            request.fixed_config.catalog_record_capacity;
    const bool payload_exhausted =
        stop_reason ==
            MortonYao48RankedPairTileClassifierStopReason::
                payload_point_id_capacity &&
        receipt.required_payload_point_id_count >
            request.fixed_config.payload_point_id_capacity;
    const bool fallback_exhausted =
        stop_reason ==
            MortonYao48RankedPairTileClassifierStopReason::
                exact_fallback_capacity &&
        receipt.required_exact_fallback_count >
            request.fixed_config.exact_fallback_capacity;
    if (failure !=
            Phase15MortonYao48RankedPairTileClassifierFailureCode::
                capacity_exhausted ||
        !(record_exhausted || payload_exhausted || fallback_exhausted) ||
        !receipt.output_invalidated || !receipt.output_rollback_validated ||
        receipt.transaction_accepted_record_count != 0U ||
        receipt.transaction_above_window_count != 0U ||
        receipt.transaction_unresolved_count !=
            receipt.transaction_candidate_count ||
        receipt.transaction_strict_payload_point_id_count != 0U ||
        receipt.transaction_shell_payload_point_id_count != 0U ||
        ((record_exhausted || payload_exhausted) &&
         receipt.transaction_exact_fallback_count != 0U) ||
        (fallback_exhausted &&
         receipt.transaction_exact_fallback_count == 0U) ||
        output.owner ||
        output.support_u != nullptr || output.support_v != nullptr ||
        output.closed_rank != nullptr || output.rank_offsets != nullptr ||
        output.strict_offsets != nullptr ||
        output.strict_point_ids != nullptr || output.shell_offsets != nullptr ||
        output.shell_point_ids != nullptr ||
        output.catalog_record_capacity != 0U ||
        output.payload_point_id_capacity != 0U ||
        output.exact_fallback_capacity != 0U ||
        output.catalog_record_count != 0U ||
        output.strict_point_id_count != 0U ||
        output.shell_point_id_count != 0U ||
        output.rank_offset_count != 0U || output.record_offset_count != 0U) {
      throw std::runtime_error(
          "a capacity-exhausted ranked-pair classifier receipt did not "
          "fail closed");
    }
  } else if (
      status == MortonYao48RankedPairTileClassifierStatus::
                    exact_predicate_failure) {
    if (stop_reason !=
            MortonYao48RankedPairTileClassifierStopReason::
                exact_predicate_failure ||
        failure !=
            Phase15MortonYao48RankedPairTileClassifierFailureCode::
                exact_predicate_failure ||
        !receipt.output_invalidated || !receipt.output_rollback_validated ||
        receipt.transaction_accepted_record_count != 0U ||
        receipt.transaction_above_window_count != 0U ||
        receipt.transaction_unresolved_count !=
            receipt.transaction_candidate_count ||
        receipt.transaction_strict_payload_point_id_count != 0U ||
        receipt.transaction_shell_payload_point_id_count != 0U ||
        receipt.transaction_exact_fallback_count == 0U ||
        receipt.required_catalog_record_count !=
            request.prior_accepted_record_count ||
        receipt.required_payload_point_id_count != checked_sum(
            request.prior_strict_payload_point_id_count,
            request.prior_shell_payload_point_id_count,
            "the prior payload count overflowed") ||
        receipt.required_exact_fallback_count !=
            checked_sum(
                request.prior_exact_fallback_count,
                receipt.transaction_exact_fallback_count,
                "the failed exact fallback count overflowed") ||
        output.owner || output.support_u != nullptr ||
        output.support_v != nullptr || output.closed_rank != nullptr ||
        output.rank_offsets != nullptr || output.strict_offsets != nullptr ||
        output.strict_point_ids != nullptr || output.shell_offsets != nullptr ||
        output.shell_point_ids != nullptr ||
        output.catalog_record_capacity != 0U ||
        output.payload_point_id_capacity != 0U ||
        output.exact_fallback_capacity != 0U ||
        output.catalog_record_count != 0U ||
        output.strict_point_id_count != 0U ||
        output.shell_point_id_count != 0U ||
        output.rank_offset_count != 0U || output.record_offset_count != 0U) {
      throw std::runtime_error(
          "an exact-predicate-failed ranked-pair receipt did not fail "
          "closed");
    }
  } else {
    throw std::runtime_error(
        "the ranked-pair classifier receipt carries an unknown status");
  }
  return ValidatedReceipt{std::move(receipt), status, stop_reason};
}

}  // namespace

MortonYao48RankedPairTileCatalogLease::
    MortonYao48RankedPairTileCatalogLease(
        MortonYao48RankedPairTileCatalogLeaseAudit audit,
        std::shared_ptr<void> retained_owner,
        std::shared_ptr<void> source_owner_authority,
        std::shared_ptr<const void> source_cloud_identity_authority,
        const std::uint64_t* device_coordinate_bits,
        const std::uint64_t* device_support_u,
        const std::uint64_t* device_support_v,
        const std::uint8_t* device_closed_rank,
        const std::uint64_t* device_rank_offsets,
        const std::uint64_t* device_strict_offsets,
        const std::uint64_t* device_strict_point_ids,
        const std::uint64_t* device_shell_offsets,
        const std::uint64_t* device_shell_point_ids,
        int cuda_device,
        bool host_fake)
    : audit_(audit),
      retained_owner_(std::move(retained_owner)),
      source_owner_authority_(std::move(source_owner_authority)),
      source_cloud_identity_authority_(
          std::move(source_cloud_identity_authority)),
      device_coordinate_bits_(device_coordinate_bits),
      device_support_u_(device_support_u),
      device_support_v_(device_support_v),
      device_closed_rank_(device_closed_rank),
      device_rank_offsets_(device_rank_offsets),
      device_strict_offsets_(device_strict_offsets),
      device_strict_point_ids_(device_strict_point_ids),
      device_shell_offsets_(device_shell_offsets),
      device_shell_point_ids_(device_shell_point_ids),
      cuda_device_(cuda_device),
      host_fake_(host_fake) {}

bool MortonYao48RankedPairTileCatalogLease::ready() const noexcept {
  const bool vacuous_without_storage =
      audit_.candidate_count == 0U && audit_.catalog_record_count == 0U &&
      !retained_owner_ && device_support_u_ == nullptr &&
      device_support_v_ == nullptr && device_closed_rank_ == nullptr &&
      device_rank_offsets_ == nullptr && device_strict_offsets_ == nullptr &&
      device_strict_point_ids_ == nullptr && device_shell_offsets_ == nullptr &&
      device_shell_point_ids_ == nullptr && audit_.rank_offset_count == 0U &&
      audit_.record_offset_count == 0U && !source_owner_authority_ &&
      !source_cloud_identity_authority_ && device_coordinate_bits_ == nullptr &&
      cuda_device_ == -1;
  const bool record_views_ready =
      audit_.catalog_record_count == 0U ||
      (device_support_u_ != nullptr && device_support_v_ != nullptr &&
       device_closed_rank_ != nullptr);
  const bool strict_view_ready =
      audit_.strict_payload_point_id_count == 0U ||
      device_strict_point_ids_ != nullptr;
  const bool shell_view_ready =
      audit_.shell_payload_point_id_count == 0U ||
      device_shell_point_ids_ != nullptr;
  return audit_.schema_version ==
             morton_yao48_ranked_pair_tile_classifier_schema_version &&
         (retained_owner_ || vacuous_without_storage) &&
         audit_.frontier_closed &&
         audit_.closed_rank_catalog_complete &&
         audit_.unresolved_count == 0U &&
         audit_.source_identity_authenticated &&
         (vacuous_without_storage ||
          (audit_.source_owner_retained &&
           audit_.source_cloud_identity_retained &&
           source_owner_authority_ && source_cloud_identity_authority_)) &&
         audit_.monotone_chunk_journal_validated &&
         audit_.exact_multiorder_classification_validated &&
         audit_.count_scan_payload_layout_validated &&
         audit_.candidate_mass_validated &&
         audit_.frontier_global_mass_validated &&
         (vacuous_without_storage ||
          (audit_.rank_offset_count == audit_.maximum_closed_rank + 2U &&
           audit_.record_offset_count == audit_.catalog_record_count + 1U &&
           device_rank_offsets_ != nullptr &&
           device_strict_offsets_ != nullptr &&
           device_shell_offsets_ != nullptr)) &&
         record_views_ready &&
         strict_view_ready && shell_view_ready &&
         !audit_.intermediate_candidate_device_to_host_performed &&
         !audit_.certified_prune_device_to_host_performed &&
         !audit_.callback_per_pair_performed &&
         !audit_.dense_pair_scan_performed &&
         !audit_.global_pair_matrix_materialized &&
         !audit_.ordinary_delaunay_materialized &&
         !audit_.higher_order_delaunay_mosaic_materialized &&
         !audit_.public_status_claimed &&
         (vacuous_without_storage || (host_fake_
              ? audit_.host_fake_lifecycle_exercised &&
                    !audit_.cuda_device_storage_retained &&
                    !audit_.source_device_coordinates_retained &&
                    device_coordinate_bits_ == nullptr && cuda_device_ == -1
              : !audit_.host_fake_lifecycle_exercised &&
                    audit_.cuda_device_storage_retained &&
                    audit_.source_device_coordinates_retained &&
                    device_coordinate_bits_ != nullptr && cuda_device_ >= 0));
}

detail::Phase15MortonYao48RankedPairTileCatalogPrivateViews
detail::Phase15MortonYao48RankedPairTileCatalogPrivateViewAccess::inspect(
    const MortonYao48RankedPairTileCatalogLease& lease) noexcept {
  Phase15MortonYao48RankedPairTileCatalogPrivateViews views;
  views.output_owner = lease.retained_owner_;
  views.source_owner_authority = lease.source_owner_authority_;
  views.source_cloud_identity_authority =
      lease.source_cloud_identity_authority_;
  views.device_coordinate_bits = lease.device_coordinate_bits_;
  views.device_support_u = lease.device_support_u_;
  views.device_support_v = lease.device_support_v_;
  views.device_closed_rank = lease.device_closed_rank_;
  views.device_rank_offsets = lease.device_rank_offsets_;
  views.device_strict_offsets = lease.device_strict_offsets_;
  views.device_strict_point_ids = lease.device_strict_point_ids_;
  views.device_shell_offsets = lease.device_shell_offsets_;
  views.device_shell_point_ids = lease.device_shell_point_ids_;
  views.point_count = lease.audit_.point_count;
  views.catalog_record_count = lease.audit_.catalog_record_count;
  views.strict_point_id_count = lease.audit_.strict_payload_point_id_count;
  views.shell_point_id_count = lease.audit_.shell_payload_point_id_count;
  views.rank_offset_count = lease.audit_.rank_offset_count;
  views.record_offset_count = lease.audit_.record_offset_count;
  views.cuda_device = lease.cuda_device_;
  views.host_fake = lease.host_fake_;
  views.ready = lease.ready();
  return views;
}

bool MortonYao48RankedPairTileCatalogLease::cuda_resident() const noexcept {
  return ready() && !host_fake_ && audit_.cuda_device_storage_retained;
}

bool MortonYao48RankedPairTileCatalogLease::host_fake() const noexcept {
  return ready() && host_fake_;
}

MortonYao48RankedPairTileClassifierContext::
    MortonYao48RankedPairTileClassifierContext(
        MortonYao48RankedPairTileClassifierConfig config)
    : config_(validate_config(config)),
      state_(std::make_shared<detail::
                                  Phase15MortonYao48RankedPairTileClassifierContextState>(
          config_)) {}

MortonYao48RankedPairTileClassifierContext::
    ~MortonYao48RankedPairTileClassifierContext() noexcept = default;

MortonYao48RankedPairTileClassifierContext::
    MortonYao48RankedPairTileClassifierContext(
        MortonYao48RankedPairTileClassifierContext&&) noexcept = default;

MortonYao48RankedPairTileClassifierContext&
MortonYao48RankedPairTileClassifierContext::operator=(
    MortonYao48RankedPairTileClassifierContext&&) noexcept = default;

MortonYao48RankedPairTileClassifierCommit
MortonYao48RankedPairTileClassifierContext::commit(
    MortonYao48DeviceTiledPairFrontierAdvance&& frontier_advance) {
  if (!state_) {
    throw std::logic_error(
        "a moved-from ranked-pair tile classifier cannot consume a chunk");
  }
  if (finished_) {
    throw std::logic_error(
        "a finished ranked-pair tile classifier cannot consume another "
        "chunk");
  }
  if (terminally_censored_) {
    throw std::logic_error(
        "a terminally censored ranked-pair tile classifier cannot resume");
  }
  if (frontier_closed_) {
    throw std::logic_error(
        "the ranked-pair tile classifier already consumed the closed "
        "frontier");
  }

  MortonYao48RankedPairTileClassifierCommit result;
  {
    MortonYao48DeviceTiledPairFrontierAdvance owned_advance{
        std::move(frontier_advance)};
    const FrontierAuthority frontier =
        inspect_frontier_authority(owned_advance);
    if (frontier.audit.maximum_closed_rank != config_.maximum_closed_rank) {
      throw std::runtime_error(
          "the frontier and resident classifier rank windows disagree");
    }
    std::optional<CandidateAuthority> candidate;
    if (owned_advance.candidate_tile.has_value()) {
      candidate.emplace(inspect_candidate_authority(
          *owned_advance.candidate_tile, config_));
      const auto& lease_audit = candidate->audit;
      const auto& frontier_audit = frontier.audit;
      if (lease_audit.source_snapshot_epoch !=
              frontier_audit.source_snapshot_epoch ||
          lease_audit.candidate_buffer_epoch !=
              frontier_audit.candidate_buffer_epoch ||
          lease_audit.point_count != frontier_audit.point_count ||
          lease_audit.certified_node_count !=
              frontier_audit.certified_node_count ||
          lease_audit.prune_semantics != frontier_audit.prune_semantics ||
          lease_audit.required_witness_count !=
              frontier_audit.required_witness_count ||
          lease_audit.tile_epoch != frontier_audit.tile_epoch ||
          lease_audit.chunk_sequence != frontier_audit.chunk_sequence ||
          lease_audit.anchor_begin !=
              frontier_audit.transaction_anchor_begin ||
          lease_audit.anchor_end !=
              frontier_audit.transaction_anchor_end ||
          lease_audit.candidate_count !=
              frontier_audit.transaction_candidate_pair_mass ||
          lease_audit.yield_reason != frontier_audit.yield_reason ||
          lease_audit.resumes_same_tile !=
              frontier_audit.resumes_same_tile ||
          candidate->tile_closed_after_chunk !=
              frontier.tile_closed_after_chunk ||
          candidate->frontier_closed_after_chunk !=
              frontier.frontier_closed_after_chunk) {
        throw std::runtime_error(
            "the frontier advance and its candidate lease disagree");
      }
    }

    std::optional<ValidatedReceipt> validated;
    state_->with_launcher_section([&] {
      validate_sequence(
          frontier.audit,
          candidate.has_value() ? &candidate->views : nullptr,
          frontier.frontier_closed_after_chunk,
          initialized_,
          prior_tile_resumable_,
          last_candidate_buffer_epoch_,
          last_tile_epoch_,
          last_chunk_sequence_,
          last_anchor_begin_,
          last_anchor_end_,
          source_owner_identity_,
          source_cloud_identity_,
          source_snapshot_epoch_,
          point_count_,
          certified_node_count_);
      if (!candidate.has_value()) {
        return;
      }
      const std::uint64_t catalog_buffer_epoch =
          state_->advance_catalog_buffer_epoch();
      Phase15MortonYao48RankedPairTileClassifierRequest request;
      request.fixed_config = config_;
      request.source_snapshot_epoch = frontier.audit.source_snapshot_epoch;
      request.candidate_buffer_epoch =
          frontier.audit.candidate_buffer_epoch;
      request.catalog_buffer_epoch = catalog_buffer_epoch;
      request.tile_epoch = frontier.audit.tile_epoch;
      request.chunk_sequence = frontier.audit.chunk_sequence;
      request.point_count = frontier.audit.point_count;
      request.certified_node_count = frontier.audit.certified_node_count;
      request.anchor_begin = frontier.audit.transaction_anchor_begin;
      request.anchor_end = frontier.audit.transaction_anchor_end;
      request.candidate_count = frontier.audit.transaction_candidate_pair_mass;
      request.transaction_certified_pruned_pair_mass =
          frontier.audit.transaction_certified_pruned_pair_mass;
      request.cumulative_certified_pruned_pair_mass =
          frontier.audit.cumulative_certified_pruned_pair_mass;
      request.unordered_pair_universe_count =
          frontier.audit.unordered_pair_universe_count;
      request.frontier_unresolved_pair_mass =
          frontier.audit.unresolved_pair_mass;
      request.prior_candidate_count = cumulative_candidate_count_;
      request.prior_accepted_record_count =
          cumulative_accepted_record_count_;
      request.prior_above_window_count = cumulative_above_window_count_;
      request.prior_unresolved_count = cumulative_unresolved_count_;
      request.prior_strict_payload_point_id_count =
          cumulative_strict_payload_point_id_count_;
      request.prior_shell_payload_point_id_count =
          cumulative_shell_payload_point_id_count_;
      request.prior_exact_fallback_count = cumulative_exact_fallback_count_;
      request.same_tile_continuation = frontier.audit.resumes_same_tile;
      request.tile_closed_after_chunk = frontier.tile_closed_after_chunk;
      request.frontier_closed_after_chunk =
          frontier.frontier_closed_after_chunk;
      auto receipt = detail::
          classify_phase15_morton_yao48_ranked_pair_tile_on_device(
              *state_, candidate->views, request);
      validated.emplace(
          validate_receipt(std::move(receipt), request, candidate->views));
    });

    const bool launched = validated.has_value();
    const auto status = launched
                            ? validated->status
                            : (frontier.frontier_closed_after_chunk
                                   ? MortonYao48RankedPairTileClassifierStatus::
                                         frontier_closed
                                   : MortonYao48RankedPairTileClassifierStatus::
                                         chunk_committed);
    const auto stop_reason =
        launched ? validated->stop_reason
                 : MortonYao48RankedPairTileClassifierStopReason::none;
    const Phase15MortonYao48RankedPairTileClassifierReceipt* receipt =
        launched ? &validated->receipt : nullptr;
    initialized_ = true;
    if (candidate.has_value()) {
      source_owner_identity_ = candidate->views.source_owner_identity;
      source_cloud_identity_ = candidate->views.source_cloud_identity;
      source_owner_authority_ = candidate->views.source_owner_authority;
      source_cloud_identity_authority_ =
          candidate->views.source_cloud_identity_authority;
      device_coordinate_bits_ = candidate->views.device_coordinate_bits;
    }
    source_snapshot_epoch_ = frontier.audit.source_snapshot_epoch;
    last_candidate_buffer_epoch_ = frontier.audit.candidate_buffer_epoch;
    last_tile_epoch_ = frontier.audit.tile_epoch;
    last_chunk_sequence_ = frontier.audit.chunk_sequence;
    point_count_ = frontier.audit.point_count;
    certified_node_count_ = frontier.audit.certified_node_count;
    last_anchor_begin_ = frontier.audit.transaction_anchor_begin;
    last_anchor_end_ = frontier.audit.transaction_anchor_end;
    cumulative_certified_pruned_pair_mass_ =
        frontier.audit.cumulative_certified_pruned_pair_mass;
    unordered_pair_universe_count_ =
        frontier.audit.unordered_pair_universe_count;
    frontier_unresolved_pair_mass_ = frontier.audit.unresolved_pair_mass;
    if (receipt != nullptr) {
      last_catalog_buffer_epoch_ = receipt->catalog_buffer_epoch;
      cumulative_candidate_count_ = receipt->cumulative_candidate_count;
      cumulative_accepted_record_count_ =
          receipt->cumulative_accepted_record_count;
      cumulative_above_window_count_ =
          receipt->cumulative_above_window_count;
      cumulative_unresolved_count_ = receipt->cumulative_unresolved_count;
      cumulative_strict_payload_point_id_count_ =
          receipt->cumulative_strict_payload_point_id_count;
      cumulative_shell_payload_point_id_count_ =
          receipt->cumulative_shell_payload_point_id_count;
      cumulative_exact_fallback_count_ =
          receipt->cumulative_exact_fallback_count;
    }
    ++committed_chunk_count_;
    launcher_call_count_ += receipt == nullptr ? 0U : receipt->launcher_call_count;
    cuda_kernel_launch_count_ +=
        receipt == nullptr ? 0U : receipt->kernel_launch_count;
    cuda_synchronization_count_ +=
        receipt == nullptr ? 0U : receipt->synchronization_count;
    prior_tile_resumable_ = !frontier.tile_closed_after_chunk;
    frontier_closed_ = frontier.frontier_closed_after_chunk;
    terminally_censored_ =
        status == MortonYao48RankedPairTileClassifierStatus::
                      capacity_exhausted ||
        status == MortonYao48RankedPairTileClassifierStatus::
                      exact_predicate_failure;
    if (receipt != nullptr) {
      host_fake_launcher_exercised_ =
          host_fake_launcher_exercised_ ||
          receipt->execution_kind ==
              Phase15MortonYao48RankedPairTileClassifierExecutionKind::
                  host_fake;
      cuda_execution_performed_ =
          cuda_execution_performed_ ||
          receipt->execution_kind ==
              Phase15MortonYao48RankedPairTileClassifierExecutionKind::cuda;
    }

    if (terminally_censored_) {
      output_owner_.reset();
      source_owner_authority_.reset();
      source_cloud_identity_authority_.reset();
      device_coordinate_bits_ = nullptr;
      device_support_u_ = nullptr;
      device_support_v_ = nullptr;
      device_closed_rank_ = nullptr;
      device_rank_offsets_ = nullptr;
      device_strict_offsets_ = nullptr;
      device_strict_point_ids_ = nullptr;
      device_shell_offsets_ = nullptr;
      device_shell_point_ids_ = nullptr;
      rank_offset_count_ = 0U;
      record_offset_count_ = 0U;
    } else if (receipt != nullptr) {
      output_owner_ = receipt->output.owner;
      device_support_u_ = receipt->output.support_u;
      device_support_v_ = receipt->output.support_v;
      device_closed_rank_ = receipt->output.closed_rank;
      device_rank_offsets_ = receipt->output.rank_offsets;
      device_strict_offsets_ = receipt->output.strict_offsets;
      device_strict_point_ids_ = receipt->output.strict_point_ids;
      device_shell_offsets_ = receipt->output.shell_offsets;
      device_shell_point_ids_ = receipt->output.shell_point_ids;
      rank_offset_count_ = receipt->output.rank_offset_count;
      record_offset_count_ = receipt->output.record_offset_count;
      cuda_device_ = receipt->cuda_device;
    }

    result.status = status;
    result.stop_reason = stop_reason;
    auto& audit = result.audit;
    audit.commit_sequence = committed_chunk_count_;
    audit.source_snapshot_epoch = frontier.audit.source_snapshot_epoch;
    audit.candidate_buffer_epoch = frontier.audit.candidate_buffer_epoch;
    audit.catalog_buffer_epoch = last_catalog_buffer_epoch_;
    audit.tile_epoch = frontier.audit.tile_epoch;
    audit.chunk_sequence = frontier.audit.chunk_sequence;
    audit.point_count = frontier.audit.point_count;
    audit.certified_node_count = frontier.audit.certified_node_count;
    audit.maximum_closed_rank = config_.maximum_closed_rank;
    audit.anchor_begin = frontier.audit.transaction_anchor_begin;
    audit.anchor_end = frontier.audit.transaction_anchor_end;
    audit.transaction_candidate_count =
        frontier.audit.transaction_candidate_pair_mass;
    audit.transaction_accepted_record_count =
        receipt == nullptr ? 0U : receipt->transaction_accepted_record_count;
    audit.transaction_above_window_count =
        receipt == nullptr ? 0U : receipt->transaction_above_window_count;
    audit.transaction_unresolved_count =
        receipt == nullptr ? 0U : receipt->transaction_unresolved_count;
    audit.transaction_strict_payload_point_id_count =
        receipt == nullptr
            ? 0U
            : receipt->transaction_strict_payload_point_id_count;
    audit.transaction_shell_payload_point_id_count =
        receipt == nullptr
            ? 0U
            : receipt->transaction_shell_payload_point_id_count;
    audit.transaction_exact_fallback_count =
        receipt == nullptr ? 0U : receipt->transaction_exact_fallback_count;
    audit.transaction_certified_pruned_pair_mass =
        frontier.audit.transaction_certified_pruned_pair_mass;
    audit.cumulative_candidate_count = cumulative_candidate_count_;
    audit.cumulative_accepted_record_count =
        cumulative_accepted_record_count_;
    audit.cumulative_above_window_count =
        cumulative_above_window_count_;
    audit.cumulative_unresolved_count = cumulative_unresolved_count_;
    audit.cumulative_strict_payload_point_id_count =
        cumulative_strict_payload_point_id_count_;
    audit.cumulative_shell_payload_point_id_count =
        cumulative_shell_payload_point_id_count_;
    audit.cumulative_exact_fallback_count =
        cumulative_exact_fallback_count_;
    audit.cumulative_certified_pruned_pair_mass =
        cumulative_certified_pruned_pair_mass_;
    audit.unordered_pair_universe_count = unordered_pair_universe_count_;
    audit.frontier_unresolved_pair_mass = frontier_unresolved_pair_mass_;
    audit.launcher_call_count = receipt == nullptr ? 0U : receipt->launcher_call_count;
    audit.cuda_kernel_launch_count =
        receipt == nullptr ? 0U : receipt->kernel_launch_count;
    audit.cuda_synchronization_count =
        receipt == nullptr ? 0U : receipt->synchronization_count;
    audit.candidate_device_to_host_count =
        receipt == nullptr ? 0U : receipt->candidate_device_to_host_count;
    audit.certified_prune_device_to_host_count =
        receipt == nullptr
            ? 0U
            : receipt->certified_prune_device_to_host_count;
    audit.legacy_pair_callback_count =
        receipt == nullptr ? 0U : receipt->legacy_pair_callback_count;
    audit.cuda_device = receipt == nullptr ? cuda_device_ : receipt->cuda_device;
    audit.same_tile_continuation = frontier.audit.resumes_same_tile;
    audit.tile_closed_after_chunk = frontier.tile_closed_after_chunk;
    audit.frontier_closed_after_chunk = frontier.frontier_closed_after_chunk;
    audit.source_identity_authenticated = true;
    audit.source_epoch_authenticated = true;
    audit.candidate_epoch_authenticated = true;
    audit.tile_chunk_sequence_authenticated = true;
    audit.candidate_tile_lease_retained_during_launch = launched;
    audit.candidate_tile_lease_consumed = candidate.has_value();
    audit.fixed_linear_capacities_validated = true;
    audit.exact_multiorder_classification_validated =
        receipt == nullptr ||
        receipt->exact_multiorder_classification_implemented;
    audit.count_scan_payload_layout_validated = true;
    audit.transaction_candidate_mass_validated = true;
    audit.cumulative_candidate_mass_validated = true;
    audit.frontier_global_mass_validated = true;
    audit.output_rollback_validated = true;
    audit.output_withheld = receipt != nullptr && receipt->output_invalidated;
    audit.host_fake_launcher_exercised =
        receipt != nullptr && receipt->execution_kind ==
        Phase15MortonYao48RankedPairTileClassifierExecutionKind::host_fake;
    audit.cuda_execution_performed =
        receipt != nullptr && receipt->execution_kind ==
        Phase15MortonYao48RankedPairTileClassifierExecutionKind::cuda;
  }
  return result;
}

MortonYao48RankedPairTileCatalogLease
MortonYao48RankedPairTileClassifierContext::finish() {
  if (!state_) {
    throw std::logic_error(
        "a moved-from ranked-pair tile classifier cannot finish");
  }
  if (finished_) {
    throw std::logic_error(
        "the ranked-pair tile classifier catalog was already released");
  }
  if (terminally_censored_) {
    throw std::logic_error(
        "a capacity-censored ranked-pair tile classifier has no catalog");
  }
  if (!frontier_closed_) {
    throw std::logic_error(
        "the ranked-pair tile classifier cannot finish before the frontier "
        "is closed");
  }
  const std::uint64_t globally_closed_mass = checked_sum3(
      checked_sum(
          cumulative_accepted_record_count_,
          cumulative_above_window_count_,
          "the final classified mass overflowed"),
      checked_sum(
          cumulative_certified_pruned_pair_mass_,
          cumulative_unresolved_count_,
          "the final closed mass overflowed"),
      frontier_unresolved_pair_mass_,
      "the final global mass overflowed");
  if (poisoned() ||
      (!output_owner_ && cumulative_candidate_count_ != 0U) ||
      cumulative_unresolved_count_ != 0U ||
      frontier_unresolved_pair_mass_ != 0U ||
      globally_closed_mass != unordered_pair_universe_count_) {
    throw std::runtime_error(
        "the ranked-pair tile classifier cannot publish an invalid catalog");
  }

  MortonYao48RankedPairTileCatalogLeaseAudit audit;
  audit.source_snapshot_epoch = source_snapshot_epoch_;
  audit.final_candidate_buffer_epoch = last_candidate_buffer_epoch_;
  audit.catalog_buffer_epoch = last_catalog_buffer_epoch_;
  audit.final_tile_epoch = last_tile_epoch_;
  audit.final_chunk_sequence = last_chunk_sequence_;
  audit.point_count = point_count_;
  audit.certified_node_count = certified_node_count_;
  audit.maximum_closed_rank = config_.maximum_closed_rank;
  audit.catalog_record_count = checked_size(
      cumulative_accepted_record_count_,
      "the final catalog record count does not fit size_t");
  audit.strict_payload_point_id_count = checked_size(
      cumulative_strict_payload_point_id_count_,
      "the final strict payload count does not fit size_t");
  audit.shell_payload_point_id_count = checked_size(
      cumulative_shell_payload_point_id_count_,
      "the final shell payload count does not fit size_t");
  audit.rank_offset_count = rank_offset_count_;
  audit.record_offset_count = record_offset_count_;
  audit.candidate_count = cumulative_candidate_count_;
  audit.above_window_count = cumulative_above_window_count_;
  audit.unresolved_count = cumulative_unresolved_count_;
  audit.exact_fallback_count = cumulative_exact_fallback_count_;
  audit.certified_pruned_pair_mass =
      cumulative_certified_pruned_pair_mass_;
  audit.frontier_unresolved_pair_mass = frontier_unresolved_pair_mass_;
  audit.unordered_pair_universe_count = unordered_pair_universe_count_;
  audit.committed_chunk_count = committed_chunk_count_;
  audit.cuda_device = cuda_device_;
  audit.frontier_closed = true;
  audit.closed_rank_catalog_complete = true;
  audit.source_identity_authenticated = true;
  audit.source_owner_retained = static_cast<bool>(source_owner_authority_);
  audit.source_cloud_identity_retained =
      static_cast<bool>(source_cloud_identity_authority_);
  audit.source_device_coordinates_retained =
      device_coordinate_bits_ != nullptr;
  audit.monotone_chunk_journal_validated = true;
  audit.exact_multiorder_classification_validated = true;
  audit.count_scan_payload_layout_validated = true;
  audit.candidate_mass_validated = true;
  audit.frontier_global_mass_validated = true;
  audit.host_fake_lifecycle_exercised = host_fake_launcher_exercised_;
  audit.cuda_device_storage_retained = cuda_execution_performed_;

  const bool host_fake = host_fake_launcher_exercised_;
  MortonYao48RankedPairTileCatalogLease catalog{
      audit,
      std::move(output_owner_),
      std::move(source_owner_authority_),
      std::move(source_cloud_identity_authority_),
      device_coordinate_bits_,
      device_support_u_,
      device_support_v_,
      device_closed_rank_,
      device_rank_offsets_,
      device_strict_offsets_,
      device_strict_point_ids_,
      device_shell_offsets_,
      device_shell_point_ids_,
      cuda_device_,
      host_fake};
  finished_ = true;
  if (!catalog.ready()) {
    throw std::runtime_error(
        "the ranked-pair tile classifier assembled an invalid final lease");
  }
  return catalog;
}

bool MortonYao48RankedPairTileClassifierContext::ready() const noexcept {
  return state_ && !state_->poisoned() && !terminally_censored_ && !finished_;
}

bool MortonYao48RankedPairTileClassifierContext::poisoned() const noexcept {
  return !state_ || state_->poisoned();
}

}  // namespace morsehgp3d::gpu
