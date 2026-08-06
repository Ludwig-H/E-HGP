#pragma once

#include "morsehgp3d/hierarchy/direct_saddle_arm_seed_journal.hpp"
#include "morsehgp3d/hierarchy/direct_support_terminal.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace morsehgp3d::hierarchy {

// Verrou ④, incrément a (design:
// docs/research/EXACT_TOWER_PRODUCER_DESIGN.md): one bounded producer
// composes the complete exact tower from a raw point set -- canonical
// cloud, LBVH, pair stream, higher stream, terminal facade, Morse event
// journal and saddle-arm seed journal -- behind a sealed backend selector
// and a per-stage fail-closed receipt.  This increment carries the
// exhaustive host backend only; the device-tiled session backend is the
// next increment.

inline constexpr std::uint32_t direct_morse_tower_producer_schema_version =
    1U;
inline constexpr std::string_view direct_morse_tower_producer_backend =
    "reference_cpu";
inline constexpr std::string_view direct_morse_tower_producer_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_morse_tower_producer_mode =
    "composed_exact_tower_from_cloud_v1";
inline constexpr std::string_view
    direct_morse_tower_producer_deployment_status =
        "architecture_only_exhaustive_host_backend_only_no_device_backend_"
        "no_massive_qualification";
inline constexpr std::string_view
    direct_morse_tower_producer_public_status = "not_claimed";

enum class ExactDirectMorseTowerBackendKind : std::uint8_t {
  exhaustive_host_v1,
  device_tiled_session_v1,
};

enum class ExactDirectMorseTowerDecision : std::uint8_t {
  not_produced,
  no_backend_rejected,
  no_cloud_rejected,
  no_pair_stream_rejected,
  no_higher_stream_rejected,
  no_facade_rejected,
  no_event_journal_rejected,
  no_seed_journal_rejected,
  no_allocation_failed,
  complete_exact_tower,
};

struct ExactDirectMorseTowerBudget {
  ExactDirectSupportTerminalBudget terminal{};
  ExactDirectSaddleArmSeedBudget seeds{};

  friend bool operator==(
      const ExactDirectMorseTowerBudget&,
      const ExactDirectMorseTowerBudget&) = default;
};

// The complete produced tower.  Every member is the unmodified output of
// its stage builder; the producer adds composition and fail-closed
// sequencing, never scientific content.
struct ExactDirectMorseTower {
  spatial::CanonicalPointCloud cloud;
  spatial::MortonLbvhIndex index;
  ExactPairSupportStreamResult pair;
  ExactHigherSupportStreamResult higher;
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedJournalResult seed_journal;
};

struct ExactDirectMorseTowerReceipt {
  std::uint32_t schema_version{direct_morse_tower_producer_schema_version};
  ExactDirectMorseTowerBackendKind backend{
      ExactDirectMorseTowerBackendKind::exhaustive_host_v1};
  std::size_t point_count{};
  std::size_t requested_maximum_order{};
  contract::CanonicalId higher_canonical_cloud_digest{};
  std::size_t pair_event_count{};
  std::size_t higher_event_count{};
  std::size_t facade_event_count{};
  std::size_t morse_event_projection_count{};
  std::size_t saddle_arm_seed_count{};
  bool cloud_canonical{false};
  bool pair_stream_complete{false};
  bool higher_stream_complete{false};
  bool facade_terminal_catalog_certified{false};
  bool event_journal_certified{false};
  bool seed_journal_certified{false};
  bool single_backend_sealed{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectMorseTowerDecision decision{
      ExactDirectMorseTowerDecision::not_produced};

  friend bool operator==(
      const ExactDirectMorseTowerReceipt&,
      const ExactDirectMorseTowerReceipt&) = default;
};

struct ExactDirectMorseTowerResult {
  std::optional<ExactDirectMorseTower> tower;
  ExactDirectMorseTowerReceipt receipt{};

  [[nodiscard]] bool certified_tower() const noexcept;
};

[[nodiscard]] ExactDirectMorseTowerResult
build_exact_direct_morse_tower_from_cloud(
    std::span<const exact::CertifiedPoint3> points,
    std::size_t requested_maximum_order,
    ExactDirectMorseTowerBackendKind backend,
    const ExactDirectMorseTowerBudget& budget);

// Shared downstream composition consumed by every backend: seals the
// receipt over the already-produced streams, then builds the facade and
// both journals fail-closed.  The streams enter unmodified; the helper adds
// sequencing and certification only.
[[nodiscard]] ExactDirectMorseTowerResult
finish_exact_direct_morse_tower(
    spatial::CanonicalPointCloud&& cloud,
    spatial::MortonLbvhIndex&& index,
    ExactPairSupportStreamResult&& pair,
    ExactHigherSupportStreamResult&& higher,
    std::size_t requested_maximum_order,
    ExactDirectMorseTowerBackendKind backend,
    const ExactDirectMorseTowerBudget& budget);

// Anchored-session variant: the facade is built through the sealed
// certificate overload instead of the fresh exhaustive replay.
[[nodiscard]] ExactDirectMorseTowerResult
finish_exact_direct_morse_tower(
    spatial::CanonicalPointCloud&& cloud,
    spatial::MortonLbvhIndex&& index,
    ExactPairSupportStreamResult&& pair,
    ExactHigherSupportStreamResult&& higher,
    const ExactHigherSupportAnchoredStreamCertificate& higher_certificate,
    std::size_t requested_maximum_order,
    ExactDirectMorseTowerBackendKind backend,
    const ExactDirectMorseTowerBudget& budget);

}  // namespace morsehgp3d::hierarchy
