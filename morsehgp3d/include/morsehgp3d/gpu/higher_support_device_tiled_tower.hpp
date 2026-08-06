#pragma once

#include "morsehgp3d/gpu/higher_support_device_tiled_session_bridge.hpp"
#include "morsehgp3d/hierarchy/direct_morse_tower_producer.hpp"

#include <optional>
#include <span>

namespace morsehgp3d::gpu {

// Verrou ④, incrément b: the device-tiled exact tower.  The pair stream and
// every downstream stage reuse the host tower helper unchanged; the higher
// stage runs through the sealed M2 session bridge -- the anchored session
// commits every transition through its own fresh CPU replay, the device
// engine only proposes tiles that are cross-validated in exact::BigInt --
// and the committed events and diagnostics are assembled into a
// facade-compatible stream result under the exhaustive oracle's public
// canonical record order.  Locally the launchers resolve to the certified
// scientific host fake; on G4 the same code binds the native sm_120 kernel.
// The device-tiled higher-stream assembly on its own: the anchored session
// driven to terminal through the sealed M2 bridge, its committed events and
// diagnostics re-ordered under the exhaustive oracle's public canonical
// sort, the terminal checkpoint's cumulative audit sealed as the result
// audit.  Sealed limitation of this increment: the terminal facade's
// certification freshly re-runs the exhaustive stream and requires total
// equality, so the device tower below stops fail-closed at
// no_facade_rejected until the facade gains an anchored-session source
// kind -- the next increment of the lock.
struct HigherSupportDeviceTiledStreamAssembly {
  std::optional<hierarchy::ExactHigherSupportStreamResult> higher;
  bool session_terminal{false};
  bool bridge_poisoned{false};

  [[nodiscard]] bool certified_assembled() const noexcept {
    return higher.has_value() && session_terminal && !bridge_poisoned;
  }
};

[[nodiscard]] HigherSupportDeviceTiledStreamAssembly
assemble_exact_higher_support_stream_device_tiled(
    const spatial::CanonicalPointCloud& cloud,
    const spatial::MortonLbvhIndex& index,
    std::size_t requested_maximum_order,
    const hierarchy::ExactHigherSupportStreamBudget& declared_budget,
    const HigherSupportDeviceTiledSessionBridgeConfig& bridge_config = {});

[[nodiscard]] hierarchy::ExactDirectMorseTowerResult
build_exact_direct_morse_tower_from_cloud_device_tiled(
    std::span<const exact::CertifiedPoint3> points,
    std::size_t requested_maximum_order,
    const hierarchy::ExactDirectMorseTowerBudget& budget,
    const HigherSupportDeviceTiledSessionBridgeConfig& bridge_config = {});

// Injected-authority overload: callers that already own the canonical cloud
// and its certified index (production pipelines, and the local differential
// whose scientific fake must observe the exact instances) hand them over;
// the tower consumes them into the result.
[[nodiscard]] hierarchy::ExactDirectMorseTowerResult
build_exact_direct_morse_tower_from_cloud_device_tiled(
    spatial::CanonicalPointCloud&& cloud,
    spatial::MortonLbvhIndex&& index,
    std::size_t requested_maximum_order,
    const hierarchy::ExactDirectMorseTowerBudget& budget,
    const HigherSupportDeviceTiledSessionBridgeConfig& bridge_config = {});

}  // namespace morsehgp3d::gpu
