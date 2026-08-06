#include "morsehgp3d/gpu/higher_support_device_tiled_tower.hpp"

#include "morsehgp3d/gpu/morton_lbvh_build.hpp"

#include <new>
#include <optional>
#include <utility>

namespace morsehgp3d::gpu {

HigherSupportDeviceTiledStreamAssembly
assemble_exact_higher_support_stream_device_tiled(
    const spatial::CanonicalPointCloud& cloud,
    const spatial::MortonLbvhIndex& index,
    std::size_t requested_maximum_order,
    const hierarchy::ExactHigherSupportStreamBudget& declared_budget,
    const HigherSupportDeviceTiledSessionBridgeConfig& bridge_config) {
  HigherSupportDeviceTiledStreamAssembly output;
  try {
    hierarchy::ExactHigherSupportAuthorityContext authority{
        index, cloud, requested_maximum_order};
    MortonLbvhBuildContext lease_builder{cloud.size() + 2U};
    const auto lease_build = lease_builder.build(cloud);
    auto lease = lease_builder.release_device_traversal_lease(lease_build);
    HigherSupportDeviceTiledSessionBridge bridge{
        authority, std::move(lease), bridge_config};

    hierarchy::ExactHigherSupportStreamResult higher;
    while (!bridge.session_terminal()) {
      auto advance = bridge.advance_one_tile_transaction();
      if (advance.status ==
          HigherSupportDeviceTiledSessionBridgeStatus::session_terminal) {
        break;
      }
      if (advance.status !=
              HigherSupportDeviceTiledSessionBridgeStatus::
                  transaction_committed ||
          bridge.poisoned()) {
        output.bridge_poisoned = bridge.poisoned();
        return output;
      }
      higher.events.insert(
          higher.events.end(),
          std::make_move_iterator(advance.committed_events.begin()),
          std::make_move_iterator(advance.committed_events.end()));
      higher.relevant_extra_shell_diagnostics.insert(
          higher.relevant_extra_shell_diagnostics.end(),
          std::make_move_iterator(
              advance.committed_extra_shell_diagnostics.begin()),
          std::make_move_iterator(
              advance.committed_extra_shell_diagnostics.end()));
    }
    output.session_terminal = bridge.session_terminal();
    output.bridge_poisoned = bridge.poisoned();

    // Facade-compatible assembly under the exhaustive oracle's public
    // canonical order.  The audit is the terminal anchored checkpoint's
    // cumulative audit -- accumulated by the same host stream builder the
    // exhaustive backend runs -- and the completion facts are carried by
    // the session-terminal state, its audit identities and the bridge's
    // per-commit rich prune replay.
    const auto& checkpoint = bridge.trusted_checkpoint();
    if (!output.session_terminal || output.bridge_poisoned ||
        !checkpoint.locally_complete()) {
      return output;
    }
    hierarchy::canonical_sort_exact_higher_support_events(higher.events);
    hierarchy::canonical_sort_exact_higher_support_extra_shell_diagnostics(
        higher.relevant_extra_shell_diagnostics);
    higher.requirements = {
        checkpoint.manifest.point_count,
        checkpoint.manifest.requested_maximum_order,
        checkpoint.manifest.effective_maximum_order,
        checkpoint.manifest.maximum_relevant_closed_rank};
    higher.budget = declared_budget;
    higher.status = hierarchy::ExactHigherSupportStreamStatus::complete;
    higher.stop_reason = hierarchy::ExactHigherSupportStopReason::none;
    higher.audit = checkpoint.cumulative_audit;
    higher.grouped_frontier_partition_certified =
        checkpoint.cumulative_audit.grouped_partition_accounting_certified;
    higher.all_prunes_replayable = true;
    higher.all_rank_relevant_shells_complete = true;
    higher.frontier_exhausted = checkpoint.frontier.empty();
    higher.no_forbidden_global_structure_materialized = true;
    higher.hierarchy_reduction_performed = false;
    output.higher.emplace(std::move(higher));
    return output;
  } catch (...) {
    output.higher.reset();
    return output;
  }
}

hierarchy::ExactDirectMorseTowerResult
build_exact_direct_morse_tower_from_cloud_device_tiled(
    spatial::CanonicalPointCloud&& cloud,
    spatial::MortonLbvhIndex&& index,
    std::size_t requested_maximum_order,
    const hierarchy::ExactDirectMorseTowerBudget& budget,
    const HigherSupportDeviceTiledSessionBridgeConfig& bridge_config) {
  using hierarchy::ExactDirectMorseTowerBackendKind;
  using hierarchy::ExactDirectMorseTowerDecision;
  hierarchy::ExactDirectMorseTowerResult output;
  output.receipt.backend =
      ExactDirectMorseTowerBackendKind::device_tiled_session_v1;
  output.receipt.point_count = cloud.size();
  output.receipt.requested_maximum_order = requested_maximum_order;
  output.receipt.single_backend_sealed = true;
  output.receipt.cloud_canonical = true;
  try {
    auto pair = hierarchy::build_exact_pair_support_stream(
        index, cloud, requested_maximum_order, budget.terminal.pair);
    output.receipt.pair_event_count = pair.events.size();
    output.receipt.pair_stream_complete = pair.stream_complete();
    if (!output.receipt.pair_stream_complete) {
      output.receipt.decision =
          ExactDirectMorseTowerDecision::no_pair_stream_rejected;
      return output;
    }
    auto assembly = assemble_exact_higher_support_stream_device_tiled(
        cloud,
        index,
        requested_maximum_order,
        budget.terminal.higher,
        bridge_config);
    if (!assembly.certified_assembled()) {
      output.receipt.decision =
          ExactDirectMorseTowerDecision::no_higher_stream_rejected;
      return output;
    }
    return hierarchy::finish_exact_direct_morse_tower(
        std::move(cloud),
        std::move(index),
        std::move(pair),
        std::move(*assembly.higher),
        requested_maximum_order,
        ExactDirectMorseTowerBackendKind::device_tiled_session_v1,
        budget);
  } catch (const std::bad_alloc&) {
    output.tower.reset();
    output.receipt.decision =
        ExactDirectMorseTowerDecision::no_allocation_failed;
    return output;
  } catch (...) {
    output.tower.reset();
    output.receipt.decision =
        ExactDirectMorseTowerDecision::no_higher_stream_rejected;
    return output;
  }
}

hierarchy::ExactDirectMorseTowerResult
build_exact_direct_morse_tower_from_cloud_device_tiled(
    std::span<const exact::CertifiedPoint3> points,
    std::size_t requested_maximum_order,
    const hierarchy::ExactDirectMorseTowerBudget& budget,
    const HigherSupportDeviceTiledSessionBridgeConfig& bridge_config) {
  using hierarchy::ExactDirectMorseTowerDecision;
  hierarchy::ExactDirectMorseTowerResult output;
  output.receipt.backend =
      hierarchy::ExactDirectMorseTowerBackendKind::device_tiled_session_v1;
  output.receipt.point_count = points.size();
  output.receipt.requested_maximum_order = requested_maximum_order;
  try {
    std::optional<spatial::CanonicalPointCloud> cloud;
    try {
      cloud.emplace(
          spatial::CanonicalPointCloud::rejecting_duplicates(points));
    } catch (const std::bad_alloc&) {
      throw;
    } catch (...) {
      output.receipt.single_backend_sealed = true;
      output.receipt.decision =
          ExactDirectMorseTowerDecision::no_cloud_rejected;
      return output;
    }
    auto index = spatial::MortonLbvhIndex::build(*cloud);
    return build_exact_direct_morse_tower_from_cloud_device_tiled(
        std::move(*cloud),
        std::move(index),
        requested_maximum_order,
        budget,
        bridge_config);
  } catch (const std::bad_alloc&) {
    output.receipt.single_backend_sealed = true;
    output.receipt.decision =
        ExactDirectMorseTowerDecision::no_allocation_failed;
    return output;
  } catch (...) {
    output.receipt.single_backend_sealed = true;
    output.receipt.decision =
        ExactDirectMorseTowerDecision::no_cloud_rejected;
    return output;
  }
}

}  // namespace morsehgp3d::gpu
