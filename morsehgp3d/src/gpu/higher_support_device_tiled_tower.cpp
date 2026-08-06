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
    }
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
