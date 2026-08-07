#include "morsehgp3d/gpu/higher_support_device_tiled_tower.hpp"

#include "morsehgp3d/gpu/morton_lbvh_build.hpp"

#include <chrono>
#include <new>
#include <optional>
#include <utility>

namespace morsehgp3d::gpu {

std::string_view higher_support_device_tiled_assembly_censure_text(
    const HigherSupportDeviceTiledAssemblyCensure censure) noexcept {
  switch (censure) {
    case HigherSupportDeviceTiledAssemblyCensure::none:
      return "none";
    case HigherSupportDeviceTiledAssemblyCensure::operational_deadline:
      return "operational_deadline";
    case HigherSupportDeviceTiledAssemblyCensure::
        committed_transaction_ceiling:
      return "committed_transaction_ceiling";
  }
  return "none";
}

HigherSupportDeviceTiledStreamAssembly
assemble_exact_higher_support_stream_device_tiled(
    const spatial::CanonicalPointCloud& cloud,
    const spatial::MortonLbvhIndex& index,
    std::size_t requested_maximum_order,
    const hierarchy::ExactHigherSupportStreamBudget& declared_budget,
    const HigherSupportDeviceTiledSessionBridgeConfig& bridge_config,
    const HigherSupportDeviceTiledAssemblyOperationalGuard& guard) {
  HigherSupportDeviceTiledStreamAssembly output;
  try {
    hierarchy::ExactHigherSupportAuthorityContext authority{
        index, cloud, requested_maximum_order};
    // The assembler owns the anchored scientific session and appropriates
    // every verified transition; it must outlive the bridge that borrows
    // it.
    hierarchy::ExactHigherSupportAnchoredStreamAssembler assembler{
        authority};
    MortonLbvhBuildContext lease_builder{cloud.size() + 2U};
    const auto lease_build = lease_builder.build(cloud);
    auto lease = lease_builder.release_device_traversal_lease(lease_build);
    HigherSupportDeviceTiledSessionBridge bridge{
        authority, assembler, std::move(lease), bridge_config};

    // R2-c: the progress transcript is snapshotted only on the way out --
    // never per transaction, because copying the audit's exact::BigInt
    // masses inside the loop would add host cost to the very stage this
    // guard exists to measure.  The bridge audit and the trusted
    // checkpoint are live, so a single read at any exit point is exact.
    const auto snapshot_progress = [&] {
      output.bridge_audit = bridge.audit();
      const hierarchy::ExactHigherSupportCheckpoint& trusted =
          bridge.trusted_checkpoint();
      output.progress_audit = trusted.cumulative_audit;
      output.progress_frontier_entry_count = trusted.frontier.size();
      output.progress_committed_transaction_count =
          output.bridge_audit.committed_transaction_count;
    };

    std::size_t committed_transaction_count = 0U;
    while (!bridge.session_terminal()) {
      // Both guard tests sit strictly between committed transitions: the
      // anchored chain is on a verified checkpoint here, and stopping now
      // leaves it exactly as the last commit left it.
      const bool ceiling_reached =
          committed_transaction_count >=
          guard.maximum_committed_transaction_count;
      const bool deadline_reached =
          guard.deadline.has_value() &&
          std::chrono::steady_clock::now() >= *guard.deadline;
      if (ceiling_reached || deadline_reached) {
        output.censure =
            ceiling_reached
                ? HigherSupportDeviceTiledAssemblyCensure::
                      committed_transaction_ceiling
                : HigherSupportDeviceTiledAssemblyCensure::
                      operational_deadline;
        snapshot_progress();
        output.session_terminal = bridge.session_terminal();
        output.bridge_poisoned = bridge.poisoned();
        return output;
      }
      // R2-h: the same deadline also reaches INSIDE the transaction.  A
      // tile consumes the whole frontier, so without this the guard above
      // would have no purchase on the one transaction that carries the
      // computation.
      HigherSupportDeviceTiledSessionBridgeOperationalGuard bridge_guard;
      bridge_guard.deadline = guard.deadline;
      auto advance = bridge.advance_one_tile_transaction(bridge_guard);
      if (advance.status ==
          HigherSupportDeviceTiledSessionBridgeStatus::session_terminal) {
        break;
      }
      if (advance.operational_deadline_censure) {
        // Nothing was committed and the anchored chain is untouched, so
        // this is a censure and not a rejection: the assembly certifies
        // nothing either way, but the transcript must say which one.
        output.censure =
            HigherSupportDeviceTiledAssemblyCensure::operational_deadline;
        snapshot_progress();
        output.session_terminal = bridge.session_terminal();
        output.bridge_poisoned = bridge.poisoned();
        return output;
      }
      if (advance.status !=
              HigherSupportDeviceTiledSessionBridgeStatus::
                  transaction_committed ||
          bridge.poisoned()) {
        snapshot_progress();
        output.bridge_poisoned = bridge.poisoned();
        return output;
      }
      ++committed_transaction_count;
    }
    snapshot_progress();
    output.session_terminal = bridge.session_terminal();
    output.bridge_poisoned = bridge.poisoned();
    if (!output.session_terminal || output.bridge_poisoned) {
      return output;
    }

    // The anchored assembler -- not this driver -- assembles the stream
    // result from the records it appropriated commit by commit, and mints
    // the sealed certificate at the locally-complete terminal.
    auto seal = assembler.seal_terminal_stream(declared_budget);
    if (!seal.certified_sealed()) {
      return output;
    }
    output.higher = std::move(seal.result);
    output.certificate = seal.certificate;
    return output;
  } catch (...) {
    output.higher.reset();
    output.certificate = {};
    return output;
  }
}

hierarchy::ExactDirectMorseTowerResult
build_exact_direct_morse_tower_from_cloud_device_tiled(
    spatial::CanonicalPointCloud&& cloud,
    spatial::MortonLbvhIndex&& index,
    std::size_t requested_maximum_order,
    const hierarchy::ExactDirectMorseTowerBudget& budget,
    const HigherSupportDeviceTiledSessionBridgeConfig& bridge_config,
    const HigherSupportDeviceTiledAssemblyOperationalGuard& guard) {
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
        bridge_config,
        guard);
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
        assembly.certificate,
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
    const HigherSupportDeviceTiledSessionBridgeConfig& bridge_config,
    const HigherSupportDeviceTiledAssemblyOperationalGuard& guard) {
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
        bridge_config,
        guard);
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
