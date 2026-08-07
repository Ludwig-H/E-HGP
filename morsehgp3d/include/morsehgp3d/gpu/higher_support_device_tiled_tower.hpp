#pragma once

#include "morsehgp3d/gpu/higher_support_device_tiled_session_bridge.hpp"
#include "morsehgp3d/hierarchy/direct_morse_tower_producer.hpp"

#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>

namespace morsehgp3d::gpu {

// R2-c: operational guard of the device-tiled assembly driver.  This is a
// session guard, exactly like the GCP circuit breakers -- never a scientific
// parameter.  It therefore lives outside
// HigherSupportDeviceTiledSessionBridgeConfig (whose equality defines the
// sealed configuration identity) and it can only ever make the assembly
// STOP EARLIER: it never changes a single committed transition, a record, a
// mass or a digest.  Both limits are tested strictly BETWEEN committed
// transitions, so a guarded stop always leaves the anchored chain on a
// fully verified checkpoint, and the assembly it returns is never
// certified.
struct HigherSupportDeviceTiledAssemblyOperationalGuard {
  // Absent means unguarded, which is the default and reproduces the
  // pre-R2-c behaviour byte for byte.
  std::optional<std::chrono::steady_clock::time_point> deadline{};
  std::size_t maximum_committed_transaction_count{
      std::numeric_limits<std::size_t>::max()};

  [[nodiscard]] bool armed() const noexcept {
    return deadline.has_value() ||
           maximum_committed_transaction_count !=
               std::numeric_limits<std::size_t>::max();
  }

  friend bool operator==(
      const HigherSupportDeviceTiledAssemblyOperationalGuard&,
      const HigherSupportDeviceTiledAssemblyOperationalGuard&) = default;
};

enum class HigherSupportDeviceTiledAssemblyCensure : std::uint8_t {
  none,
  operational_deadline,
  committed_transaction_ceiling,
};

// Verrou ④, incréments b1+b2: the device-tiled exact tower.  The pair
// stream and every downstream stage reuse the host tower helper unchanged;
// the higher stage runs through the sealed M2 session bridge -- the
// anchored session commits every transition through its own fresh CPU
// replay, the device engine only proposes tiles that are cross-validated in
// exact::BigInt -- and the anchored stream assembler seals the committed
// records into a facade-compatible stream result plus the mint-private
// certificate the facade's anchored_session_chain source kind verifies
// without any exhaustive re-run.  Locally the launchers resolve to the
// certified scientific host fake; on G4 the same code binds the native
// sm_120 kernel.
// The device-tiled higher-stream assembly on its own: the anchored stream
// assembler owns the scientific session, the bridge borrows it so every
// verified transition is appropriated at commit time, and the terminal
// seal carries both the facade-compatible stream result (public canonical
// order, cumulative terminal audit) and the privately minted certificate
// that binds it to the anchored chain.  The certificate is what the
// terminal facade's anchored_session_chain source kind consumes instead of
// a fresh exhaustive replay.
struct HigherSupportDeviceTiledStreamAssembly {
  std::optional<hierarchy::ExactHigherSupportStreamResult> higher;
  hierarchy::ExactHigherSupportAnchoredStreamCertificate certificate{};
  bool session_terminal{false};
  bool bridge_poisoned{false};
  // R2-c progress transcript.  Published on EVERY outcome -- terminal,
  // poisoned, rejected or censored -- so that a run which does not reach
  // the terminal still yields a ventilation instead of nothing.  The
  // masses are the anchored checkpoint's own exact::BigInt accounting, so
  // `progress_audit.resolved_support_count / total_support_count` is the
  // exact fraction of C(n,3)+C(n,4) that was decided before the stop, and
  // `progress_audit.minimal_leaf_count` is the exact number of minimal
  // terminals drained so far.  None of this is a scientific claim: a
  // censored assembly certifies nothing at all.
  HigherSupportDeviceTiledAssemblyCensure censure{
      HigherSupportDeviceTiledAssemblyCensure::none};
  HigherSupportDeviceTiledSessionBridgeAudit bridge_audit{};
  hierarchy::ExactHigherSupportStreamAudit progress_audit{};
  std::size_t progress_frontier_entry_count{};
  std::size_t progress_committed_transaction_count{};
  // R2-i: an exception escaped the transaction loop or the terminal seal.
  // The transcript above is still filled in that case; it is empty only
  // when the bridge itself could not be constructed.
  bool assembly_threw{false};

  [[nodiscard]] bool censored() const noexcept {
    return censure != HigherSupportDeviceTiledAssemblyCensure::none;
  }

  [[nodiscard]] bool certified_assembled() const noexcept {
    return higher.has_value() && certificate.minted() && session_terminal &&
           !bridge_poisoned && !censored();
  }
};

[[nodiscard]] std::string_view
higher_support_device_tiled_assembly_censure_text(
    HigherSupportDeviceTiledAssemblyCensure censure) noexcept;

[[nodiscard]] HigherSupportDeviceTiledStreamAssembly
assemble_exact_higher_support_stream_device_tiled(
    const spatial::CanonicalPointCloud& cloud,
    const spatial::MortonLbvhIndex& index,
    std::size_t requested_maximum_order,
    const hierarchy::ExactHigherSupportStreamBudget& declared_budget,
    const HigherSupportDeviceTiledSessionBridgeConfig& bridge_config = {},
    const HigherSupportDeviceTiledAssemblyOperationalGuard& guard = {});

[[nodiscard]] hierarchy::ExactDirectMorseTowerResult
build_exact_direct_morse_tower_from_cloud_device_tiled(
    std::span<const exact::CertifiedPoint3> points,
    std::size_t requested_maximum_order,
    const hierarchy::ExactDirectMorseTowerBudget& budget,
    const HigherSupportDeviceTiledSessionBridgeConfig& bridge_config = {},
    const HigherSupportDeviceTiledAssemblyOperationalGuard& guard = {});

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
    const HigherSupportDeviceTiledSessionBridgeConfig& bridge_config = {},
    const HigherSupportDeviceTiledAssemblyOperationalGuard& guard = {});

}  // namespace morsehgp3d::gpu
