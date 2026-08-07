#include "morsehgp3d/gpu/higher_support_device_tiled_frontier.hpp"

#include "../cuda/phase15_higher_support_device_tiled_frontier_internal.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::detail {

// Exact per-slot continuation retained by the host between chunks of one
// tile.  The device only ever reports 128-bit lo/hi deltas; every cumulative
// mass below is host-owned exact::BigInt and is the sole commit authority.
struct Phase15HigherSupportDeviceTiledSlotProgress final {
  exact::BigInt well_prune_mass{0};
  exact::BigInt rank_prune_mass{0};
  exact::BigInt terminal_mass{0};
  bool complete{false};
};

class Phase15HigherSupportDeviceTiledFrontierHostState final {
 public:
  explicit Phase15HigherSupportDeviceTiledFrontierHostState(
      Phase15HigherSupportDeviceTiledAdoptedTraversal adopted)
      : traversal(std::move(adopted)) {}

  Phase15HigherSupportDeviceTiledAdoptedTraversal traversal;
  bool active_tile{false};
  std::vector<hierarchy::ExactHigherSupportFrontierEntry> slot_entries;
  std::vector<exact::BigInt> slot_universe_masses;
  std::vector<std::uint64_t> slot_root_digests;
  exact::BigInt tile_universe_mass{0};
  std::uint64_t active_tile_epoch{};
  std::uint64_t next_chunk_sequence{};
  std::vector<Phase15HigherSupportDeviceTiledSlotProgress> slot_progress;
  // Context-lifetime exact masses committed across all validated chunks.
  exact::BigInt cumulative_well_prune_mass{0};
  exact::BigInt cumulative_rank_prune_mass{0};
  exact::BigInt cumulative_terminal_mass{0};
};

}  // namespace morsehgp3d::gpu::detail

namespace morsehgp3d::gpu {
namespace {

using AdoptedTraversal =
    detail::Phase15HigherSupportDeviceTiledAdoptedTraversal;
using DeviceBatch = detail::Phase15HigherSupportDeviceTiledBatch;
using ExecutionKind = detail::Phase15HigherSupportDeviceTiledExecutionKind;
using FailureCode = detail::Phase15HigherSupportDeviceTiledFailureCode;
using FrontierEntry = hierarchy::ExactHigherSupportFrontierEntry;
using InternalStopReason =
    detail::Phase15HigherSupportDeviceTiledStopReason;
using InternalYieldReason =
    detail::Phase15HigherSupportDeviceTiledYieldReason;
using ProbeReceipt = detail::Phase15HigherSupportDeviceTiledProbeReceipt;
using ProductRecord = detail::Phase15HigherSupportDeviceTiledProductRecord;
using PruneRecord = detail::Phase15HigherSupportDeviceTiledPruneRecord;
using DeferredDecision =
    detail::Phase15HigherSupportDeviceTiledDeferredDecision;
using Request = detail::Phase15HigherSupportDeviceTiledRequest;
using SlotControl = detail::Phase15HigherSupportDeviceTiledSlotControl;
using SlotProgress = detail::Phase15HigherSupportDeviceTiledSlotProgress;
using SlotStatus = detail::Phase15HigherSupportDeviceTiledSlotStatus;
using TerminalRecord =
    detail::Phase15HigherSupportDeviceTiledTerminalRecord;
using Unsigned128 = detail::Phase15HigherSupportDeviceTiledUnsigned128;

struct DetachedTileAuthority final {
  std::shared_ptr<void> traversal_owner;
  std::shared_ptr<const void> source_cloud_identity;
  std::shared_ptr<void> output_owner;
};

struct ValidatedBatch final {
  std::size_t authorized_slot_count{};
  std::size_t completed_slot_count{};
  std::size_t prune_record_count{};
  std::size_t terminal_record_count{};
  std::size_t probe_receipt_count{};
  std::uint64_t expansion_count{};
  std::uint64_t gate_evaluation_count{};
  std::uint64_t probe_root_decision_count{};
  std::uint64_t probe_fallback_decision_count{};
  std::uint64_t stack_high_water{};
  std::uint64_t deferred_int512_count{};
  std::uint64_t deferred_int1024_count{};
  std::uint64_t rational_drain_count{};
  exact::BigInt well_prune_mass{0};
  exact::BigInt rank_prune_mass{0};
  exact::BigInt terminal_mass{0};
  std::vector<SlotProgress> next_progress;
  bool tile_complete{false};
  bool capacity_yield{false};
  bool fatal{false};
  HigherSupportDeviceTiledFrontierYieldReason yield_reason{
      HigherSupportDeviceTiledFrontierYieldReason::none};
  HigherSupportDeviceTiledFrontierStopReason stop_reason{
      HigherSupportDeviceTiledFrontierStopReason::none};
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

[[nodiscard]] exact::BigInt bigint_from_words(
    std::uint64_t lo,
    std::uint64_t hi) {
  return (exact::BigInt{hi} << 64U) + lo;
}

// Exact binomial C(range_size, multiplicity) for multiplicity in [1, 4].
// The sequential integer divisions are exact because the product of k
// consecutive integers is divisible by k!.
[[nodiscard]] exact::BigInt bigint_binomial(
    std::uint64_t range_size,
    std::size_t multiplicity) {
  if (multiplicity == 0U || multiplicity > 4U) {
    throw std::logic_error(
        "a Phase 15 higher-support binomial multiplicity must be in "
        "[1, 4]");
  }
  if (range_size < multiplicity) {
    return exact::BigInt{0};
  }
  exact::BigInt value{1};
  for (std::size_t step = 0U; step < multiplicity; ++step) {
    value *= exact::BigInt{range_size - step};
  }
  for (std::size_t divisor = 2U; divisor <= multiplicity; ++divisor) {
    value /= exact::BigInt{static_cast<std::uint64_t>(divisor)};
  }
  return value;
}

// Fail-closed structural validation of one canonical host frontier entry
// before any work is scheduled on it.  Mirrors the CPU stream's
// entry_support_count contract: arity, sorted disjoint nonempty Morton
// intervals inside the adopted traversal, multiplicity partition of the
// support size, and canonical zero padding.
void validate_frontier_entry(
    const FrontierEntry& entry,
    std::size_t point_count,
    std::size_t certified_node_count,
    HigherSupportDeviceTiledFrontierTerminalPolicy terminal_policy) {
  const std::size_t support_size = entry.support_size;
  const std::size_t group_count = entry.group_count;
  if ((support_size != 3U && support_size != 4U) || group_count == 0U ||
      group_count > 4U || group_count > support_size) {
    throw std::invalid_argument(
        "a Phase 15 higher-support slot tile entry has an invalid arity");
  }
  if (terminal_policy ==
          HigherSupportDeviceTiledFrontierTerminalPolicy::
              strict_interior_carrier_q3 &&
      support_size != 3U) {
    throw std::invalid_argument(
        "the Phase 15 strict-interior carrier policy admits support-three "
        "slot tile entries only");
  }
  std::size_t multiplicity_sum = 0U;
  std::uint64_t previous_end = 0U;
  for (std::size_t index = 0U; index < group_count; ++index) {
    const hierarchy::ExactHigherSupportNodeGroup& group =
        entry.groups[index];
    const std::uint64_t multiplicity = group.multiplicity;
    if (multiplicity == 0U || multiplicity > 4U ||
        group.leaf_begin >= group.leaf_end ||
        group.leaf_end - group.leaf_begin < multiplicity ||
        group.leaf_end > static_cast<std::uint64_t>(point_count) ||
        group.node_index >=
            static_cast<std::uint64_t>(certified_node_count) ||
        (index != 0U && previous_end > group.leaf_begin)) {
      throw std::invalid_argument(
          "a Phase 15 higher-support slot tile entry has overlapping, "
          "unsorted, or out-of-range Morton groups");
    }
    previous_end = group.leaf_end;
    multiplicity_sum = checked_size_sum(
        multiplicity_sum,
        static_cast<std::size_t>(multiplicity),
        "a Phase 15 higher-support multiplicity sum overflows size_t");
  }
  const hierarchy::ExactHigherSupportNodeGroup padding{};
  for (std::size_t index = group_count; index < entry.groups.size();
       ++index) {
    if (entry.groups[index] != padding) {
      throw std::invalid_argument(
          "a Phase 15 higher-support slot tile entry has noncanonical "
          "padding");
    }
  }
  if (multiplicity_sum != support_size) {
    throw std::invalid_argument(
        "a Phase 15 higher-support slot tile entry does not partition its "
        "support size across group multiplicities");
  }
}

// Exact slot universe mass as the BigInt product of per-group binomials,
// certified against the unsigned 128-bit progressive scheme the device
// uses.  Any u128 overflow or disagreement with the BigInt product fails
// closed: the tile is rejected before any work is scheduled.  Since the
// group multiplicities partition a support of size at most four, at most
// one group can carry multiplicity three or four; every other per-group
// binomial fits one 64-bit word, so the widest group seeds the u128
// accumulator and the rest multiply in as small factors.
[[nodiscard]] exact::BigInt certified_slot_universe_mass(
    const FrontierEntry& entry) {
  const std::size_t group_count = entry.group_count;
  exact::BigInt mass{1};
  std::size_t widest_group = 0U;
  for (std::size_t index = 0U; index < group_count; ++index) {
    const hierarchy::ExactHigherSupportNodeGroup& group =
        entry.groups[index];
    mass *= bigint_binomial(
        group.leaf_end - group.leaf_begin, group.multiplicity);
    if (group.multiplicity >
        entry.groups[widest_group].multiplicity) {
      widest_group = index;
    }
  }
  Unsigned128 accumulated{};
  bool certified =
      detail::phase15_higher_support_device_tiled_binomial_u128(
          entry.groups[widest_group].leaf_end -
              entry.groups[widest_group].leaf_begin,
          entry.groups[widest_group].multiplicity,
          accumulated);
  for (std::size_t index = 0U; certified && index < group_count;
       ++index) {
    if (index == widest_group) {
      continue;
    }
    const hierarchy::ExactHigherSupportNodeGroup& group =
        entry.groups[index];
    Unsigned128 factor{};
    certified =
        detail::phase15_higher_support_device_tiled_binomial_u128(
            group.leaf_end - group.leaf_begin,
            group.multiplicity,
            factor) &&
        factor.hi == 0U &&
        detail::phase15_higher_support_device_tiled_u128_multiply_small(
            accumulated, factor.lo);
  }
  if (!certified) {
    throw std::runtime_error(
        "a Phase 15 higher-support slot universe mass overflowed the "
        "certified unsigned 128-bit binomial scheme");
  }
  if (bigint_from_words(accumulated.lo, accumulated.hi) != mass) {
    throw std::runtime_error(
        "the Phase 15 unsigned 128-bit slot universe mass disagrees with "
        "the exact BigInt product");
  }
  return mass;
}

[[nodiscard]] HigherSupportDeviceTiledFrontierConfig validate_config(
    HigherSupportDeviceTiledFrontierConfig config) {
  if (!higher_support_device_tiled_frontier_terminal_policy_known(
          config.terminal_policy)) {
    throw std::out_of_range(
        "a Phase 15 higher-support device tiled terminal policy tag is "
        "unknown");
  }
  if (config.maximum_relevant_closed_rank < 2U ||
      config.maximum_relevant_closed_rank >
          higher_support_device_tiled_frontier_maximum_closed_rank) {
    throw std::out_of_range(
        "a Phase 15 higher-support device tiled closed-rank cap must be "
        "in [2, 11]");
  }
  if (config.slot_tile_capacity == 0U ||
      config.slot_tile_capacity >
          higher_support_device_tiled_frontier_maximum_slot_tile_capacity) {
    throw std::out_of_range(
        "a Phase 15 higher-support device tiled slot tile capacity must "
        "be in [1, 1024]");
  }
  // The analyze stage of one product consumes two gate evaluations
  // atomically (universal prune gate, then positive gate), so a quantum of
  // one could never retire any product and would surface as a launcher
  // failure after the poisoning boundary.  Reject it as a configuration
  // error instead.
  if (config.gate_evaluations_per_slot_per_chunk < 2U) {
    throw std::out_of_range(
        "a Phase 15 higher-support device tiled gate-evaluation quantum "
        "must be at least two");
  }
  if (config.maximum_subdivision_count == 0U) {
    throw std::out_of_range(
        "a Phase 15 higher-support device tiled subdivision budget must "
        "be at least one");
  }
  if (config.certified_maximum_lbvh_depth >
          higher_support_device_tiled_frontier_maximum_certified_lbvh_depth ||
      higher_support_device_tiled_frontier_proved_stack_bound(
          config.certified_maximum_lbvh_depth) >
          higher_support_device_tiled_frontier_products_per_slot) {
    throw std::invalid_argument(
        "the Phase 15 certified LBVH depth breaks the proved 16*depth + 1 "
        "per-slot product stack bound");
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
        "the Phase 15 higher-support traversal adoption returned a "
        "foreign or malformed authority");
  }

  switch (adopted.execution_kind) {
    case ExecutionKind::host_fake:
      if (!adopted.host_fake_lifecycle_exercised ||
          adopted.cuda_device_storage_retained ||
          adopted.device_coordinate_bits != nullptr ||
          adopted.device_morton_point_ids != nullptr ||
          adopted.device_nodes != nullptr ||
          adopted.cuda_device != -1) {
        throw std::runtime_error(
            "the Phase 15 host-fake higher-support traversal adoption "
            "forged device storage");
      }
      return;
    case ExecutionKind::cuda:
      if (adopted.host_fake_lifecycle_exercised ||
          !adopted.cuda_device_storage_retained ||
          adopted.device_coordinate_bits == nullptr ||
          adopted.device_morton_point_ids == nullptr ||
          adopted.device_nodes == nullptr ||
          adopted.cuda_device < 0) {
        throw std::runtime_error(
            "the Phase 15 CUDA higher-support traversal adoption omitted "
            "resident views");
      }
      return;
  }
  throw std::runtime_error(
      "the Phase 15 higher-support traversal adoption returned an "
      "unknown backend");
}

[[nodiscard]] HigherSupportDeviceTiledFrontierStopReason
validate_stop_reason(std::uint64_t raw) {
  switch (raw) {
    case static_cast<std::uint64_t>(InternalStopReason::none):
      return HigherSupportDeviceTiledFrontierStopReason::none;
    case static_cast<std::uint64_t>(
        InternalStopReason::subdivision_capacity):
      return HigherSupportDeviceTiledFrontierStopReason::
          subdivision_capacity;
    case static_cast<std::uint64_t>(InternalStopReason::fatal_failure):
      return HigherSupportDeviceTiledFrontierStopReason::fatal_failure;
    default:
      throw std::runtime_error(
          "a Phase 15 higher-support slot control returned an unknown "
          "stop reason");
  }
}

[[nodiscard]] HigherSupportDeviceTiledFrontierYieldReason
validate_yield_reason(std::uint64_t raw) {
  switch (raw) {
    case static_cast<std::uint64_t>(InternalYieldReason::none):
      return HigherSupportDeviceTiledFrontierYieldReason::none;
    case static_cast<std::uint64_t>(
        InternalYieldReason::prune_segment_full):
      return HigherSupportDeviceTiledFrontierYieldReason::
          prune_segment_full;
    case static_cast<std::uint64_t>(
        InternalYieldReason::terminal_segment_full):
      return HigherSupportDeviceTiledFrontierYieldReason::
          terminal_segment_full;
    case static_cast<std::uint64_t>(
        InternalYieldReason::deferred_queue_full):
      return HigherSupportDeviceTiledFrontierYieldReason::
          deferred_queue_full;
    default:
      throw std::runtime_error(
          "a Phase 15 higher-support slot control returned an unknown "
          "yield reason");
  }
}

// Canonical host-checkable arena byte sum.  The private per-slot checkpoint
// mirror is deliberately excluded: its layout is launcher-private (a host
// mirror in the scientific fake, a native layout in the CUDA path), so only
// its slot extent is authenticated, never its bytes.
[[nodiscard]] std::size_t checked_device_arena_capacity_bytes(
    const Request& request) {
  const std::size_t product_count = checked_size_product(
      request.slot_count,
      request.products_per_slot,
      "the Phase 15 product arena extent overflows size_t");
  const std::size_t prune_count = checked_size_product(
      request.slot_count,
      request.prune_records_per_slot,
      "the Phase 15 prune arena extent overflows size_t");
  const std::size_t terminal_count = checked_size_product(
      request.slot_count,
      request.terminal_records_per_slot,
      "the Phase 15 terminal arena extent overflows size_t");
  const std::size_t receipt_count = checked_size_product(
      request.slot_count,
      request.probe_receipts_per_slot,
      "the Phase 15 probe-receipt arena extent overflows size_t");
  const std::size_t deferred_count = checked_size_product(
      request.slot_count,
      request.pending_decisions_per_slot,
      "the Phase 15 deferred arena extent overflows size_t");
  std::size_t bytes = checked_size_product(
      product_count,
      sizeof(ProductRecord),
      "the Phase 15 product arena bytes overflow size_t");
  bytes = checked_size_sum(
      bytes,
      checked_size_product(
          prune_count,
          sizeof(PruneRecord),
          "the Phase 15 prune arena bytes overflow size_t"),
      "the Phase 15 output arena bytes overflow size_t");
  bytes = checked_size_sum(
      bytes,
      checked_size_product(
          terminal_count,
          sizeof(TerminalRecord),
          "the Phase 15 terminal arena bytes overflow size_t"),
      "the Phase 15 output arena bytes overflow size_t");
  bytes = checked_size_sum(
      bytes,
      checked_size_product(
          receipt_count,
          sizeof(ProbeReceipt),
          "the Phase 15 probe-receipt arena bytes overflow size_t"),
      "the Phase 15 output arena bytes overflow size_t");
  bytes = checked_size_sum(
      bytes,
      checked_size_product(
          request.slot_count,
          sizeof(SlotControl),
          "the Phase 15 control arena bytes overflow size_t"),
      "the Phase 15 output arena bytes overflow size_t");
  bytes = checked_size_sum(
      bytes,
      checked_size_product(
          deferred_count,
          sizeof(DeferredDecision),
          "the Phase 15 deferred int512 arena bytes overflow size_t"),
      "the Phase 15 output arena bytes overflow size_t");
  bytes = checked_size_sum(
      bytes,
      checked_size_product(
          deferred_count,
          sizeof(DeferredDecision),
          "the Phase 15 deferred int1024 arena bytes overflow size_t"),
      "the Phase 15 output arena bytes overflow size_t");
  // Native rational-drain scratch: one staged task per slot plus the
  // device rational-request counter word.  Accounted identically by the
  // host fake so the canonical arena byte sum stays backend-independent.
  bytes = checked_size_sum(
      bytes,
      checked_size_product(
          request.slot_count,
          sizeof(detail::Phase15HigherSupportDeviceTiledRationalDrainTask),
          "the Phase 15 rational-task arena bytes overflow size_t"),
      "the Phase 15 output arena bytes overflow size_t");
  bytes = checked_size_sum(
      bytes,
      sizeof(std::uint64_t),
      "the Phase 15 rational-counter arena bytes overflow size_t");
  return checked_size_sum(
      bytes,
      sizeof(std::uint64_t),
      "the Phase 15 pending-slot arena bytes overflow size_t");
}

void validate_batch_envelope(
    const DeviceBatch& batch,
    const AdoptedTraversal& traversal,
    const Request& request) {
  const std::size_t expected_product_capacity = checked_size_product(
      request.slot_count,
      request.products_per_slot,
      "the Phase 15 product segment capacity overflows size_t");
  const std::size_t expected_prune_capacity = checked_size_product(
      request.slot_count,
      request.prune_records_per_slot,
      "the Phase 15 prune segment capacity overflows size_t");
  const std::size_t expected_terminal_capacity = checked_size_product(
      request.slot_count,
      request.terminal_records_per_slot,
      "the Phase 15 terminal segment capacity overflows size_t");
  const std::size_t expected_receipt_capacity = checked_size_product(
      request.slot_count,
      request.probe_receipts_per_slot,
      "the Phase 15 probe-receipt segment capacity overflows size_t");
  const std::size_t expected_deferred_capacity = checked_size_product(
      request.slot_count,
      request.pending_decisions_per_slot,
      "the Phase 15 deferred segment capacity overflows size_t");
  const std::size_t expected_device_arena_capacity_bytes =
      checked_device_arena_capacity_bytes(request);
  const bool closed_rank_policy =
      request.terminal_policy ==
      HigherSupportDeviceTiledFrontierTerminalPolicy::closed_rank_window;
  const bool carrier_policy =
      request.terminal_policy ==
      HigherSupportDeviceTiledFrontierTerminalPolicy::
          strict_interior_carrier_q3;
  // Host-side request self-consistency: the request is built by this
  // translation unit, so a contradiction here is a host logic defect.
  if (request.slot_count == 0U ||
      request.slot_count >
          higher_support_device_tiled_frontier_maximum_slot_tile_capacity ||
      request.products_per_slot !=
          higher_support_device_tiled_frontier_products_per_slot ||
      request.prune_records_per_slot !=
          higher_support_device_tiled_frontier_prune_records_per_slot ||
      request.terminal_records_per_slot !=
          higher_support_device_tiled_frontier_terminal_records_per_slot ||
      request.probe_receipts_per_slot !=
          higher_support_device_tiled_frontier_probe_receipts_per_slot ||
      request.pending_decisions_per_slot !=
          higher_support_device_tiled_frontier_pending_decisions_per_slot ||
      request.root_entries.empty() != request.resume_same_tile ||
      (!request.resume_same_tile &&
       request.root_entries.size() != request.slot_count)) {
    throw std::logic_error(
        "the Phase 15 host request contradicts the fixed slot-tile "
        "schema");
  }
  if (!batch.retained_output_owner ||
      !batch.source_cloud_identity_authority ||
      batch.source_cloud_identity_authority.get() !=
          traversal.source_cloud_identity.get() ||
      batch.host_slot_controls.size() != request.slot_count ||
      batch.physical_product_record_capacity !=
          expected_product_capacity ||
      batch.physical_prune_record_capacity != expected_prune_capacity ||
      batch.physical_terminal_record_capacity !=
          expected_terminal_capacity ||
      batch.physical_probe_receipt_capacity !=
          expected_receipt_capacity ||
      batch.physical_slot_control_capacity != request.slot_count ||
      batch.physical_slot_checkpoint_capacity != request.slot_count ||
      batch.physical_deferred_int512_capacity !=
          expected_deferred_capacity ||
      batch.physical_deferred_int1024_capacity !=
          expected_deferred_capacity ||
      batch.physical_pending_slot_count_capacity != 1U ||
      batch.physical_device_arena_capacity_bytes !=
          expected_device_arena_capacity_bytes ||
      batch.traversal_subdivision_count == 0U ||
      batch.traversal_subdivision_count >
          request.maximum_subdivision_count ||
      batch.source_snapshot_epoch != request.source_snapshot_epoch ||
      batch.record_buffer_epoch != request.record_buffer_epoch ||
      batch.tile_epoch != request.tile_epoch ||
      batch.chunk_sequence != request.chunk_sequence ||
      batch.resume_same_tile != request.resume_same_tile ||
      batch.terminal_policy != request.terminal_policy ||
      batch.maximum_relevant_closed_rank !=
          request.maximum_relevant_closed_rank ||
      batch.execution_kind != traversal.execution_kind ||
      batch.process_restart_resumable ||
      !batch.fixed_slot_segments_allocated ||
      !batch.output_owner_detached_for_tile_lifetime ||
      !batch.unsigned128_slot_mass_accounting_requested ||
      !batch.probe_plan_order_receipt_commit_requested ||
      !batch.substitute_probe_center_seed_policy_v1_requested ||
      batch.closed_rank_window_receipts_sealed_to_policy !=
          closed_rank_policy ||
      batch.strict_interior_carrier_receipts_sealed_to_policy !=
          carrier_policy ||
      !batch.strict_interior_carrier_never_authorizes_gamma2_drop ||
      !batch.censored_slot_outputs_invalidated ||
      batch.bigint_support_mass_transferred ||
      batch.floating_point_decision_performed ||
      batch.exact_higher_support_terminal_classification_native_cuda ||
      batch.dense_product_fallback_performed ||
      batch.global_product_frontier_materialized ||
      batch.ordinary_or_higher_order_delaunay_materialized ||
      batch.global_cell_coface_or_incidence_arena_materialized) {
    throw std::runtime_error(
        "the Phase 15 higher-support device tile returned a malformed or "
        "forbidden batch envelope");
  }

  switch (batch.execution_kind) {
    case ExecutionKind::host_fake:
      // The scientific host fake retains every record segment in host
      // memory owned by retained_output_owner, so the views must be
      // resident while all CUDA execution metadata stays inert.
      if (batch.prune_records == nullptr ||
          batch.terminal_records == nullptr ||
          batch.probe_receipts == nullptr ||
          batch.slot_controls == nullptr ||
          batch.slot_control_device_to_host_count != 0U ||
          batch.slot_control_device_to_host_byte_count != 0U ||
          batch.resume_control_device_to_host_count != 0U ||
          batch.resume_control_device_to_host_byte_count != 0U ||
          batch.kernel_launch_count != 0U ||
          batch.synchronization_count != 0U ||
          batch.host_rational_drain_relaunch_count != 0U ||
          batch.rational_task_device_to_host_count != 0U ||
          batch.rational_task_device_to_host_byte_count != 0U ||
          batch.cuda_device != -1 ||
          batch.cuda_execution_contract_satisfied ||
          batch.fresh_tile_device_arena_allocated ||
          batch.fresh_tile_device_arena_reused) {
        throw std::runtime_error(
            "the Phase 15 host fake forged CUDA execution metadata or "
            "dropped its resident host record segments");
      }
      break;
    case ExecutionKind::cuda: {
      // Every kernel launch is either one bounded subdivision (fresh gate
      // quantum) or one rational-drain relaunch of the same subdivision
      // (no fresh quantum); each launch drains one two-word device control
      // block (pending slots + staged rational requests) and one final
      // synchronization covers the slot-control drain.
      const std::size_t expected_control_bytes = checked_size_product(
          request.slot_count,
          sizeof(SlotControl),
          "the Phase 15 slot-control transfer extent overflows size_t");
      const std::size_t expected_launch_count = checked_size_sum(
          batch.traversal_subdivision_count,
          batch.host_rational_drain_relaunch_count,
          "the Phase 15 kernel launch count overflows size_t");
      const std::size_t expected_resume_bytes = checked_size_product(
          expected_launch_count,
          2U * sizeof(std::uint64_t),
          "the Phase 15 resume-control transfer extent overflows size_t");
      const std::size_t expected_rational_task_bytes =
          checked_size_product(
              batch.rational_task_device_to_host_count,
              sizeof(detail::
                         Phase15HigherSupportDeviceTiledRationalDrainTask),
              "the Phase 15 rational-task transfer extent overflows "
              "size_t");
      if (batch.prune_records == nullptr ||
          batch.terminal_records == nullptr ||
          batch.probe_receipts == nullptr ||
          batch.slot_controls == nullptr ||
          batch.slot_control_device_to_host_count !=
              request.slot_count ||
          batch.slot_control_device_to_host_byte_count !=
              expected_control_bytes ||
          batch.kernel_launch_count != expected_launch_count ||
          batch.synchronization_count !=
              checked_size_sum(
                  expected_launch_count,
                  1U,
                  "the Phase 15 synchronization count overflows size_t") ||
          batch.resume_control_device_to_host_count !=
              expected_launch_count ||
          batch.resume_control_device_to_host_byte_count !=
              expected_resume_bytes ||
          batch.rational_task_device_to_host_byte_count !=
              expected_rational_task_bytes ||
          (batch.host_rational_drain_relaunch_count == 0U) !=
              (batch.rational_task_device_to_host_count == 0U) ||
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
      break;
    }
    default:
      throw std::runtime_error(
          "the Phase 15 higher-support device tile returned an unknown "
          "backend");
  }
  // Deliberately validated last: every structural field above is
  // authenticated on its own before the digest seals the metadata.
  if (batch.metadata_digest !=
      detail::phase15_higher_support_device_tiled_metadata_digest(batch)) {
    throw std::runtime_error(
        "the Phase 15 higher-support device tile forged its metadata "
        "digest");
  }
}

[[nodiscard]] ValidatedBatch validate_slot_controls(
    const DeviceBatch& batch,
    const Request& request,
    const std::vector<SlotProgress>& previous_progress,
    const std::vector<exact::BigInt>& slot_universe_masses,
    const std::vector<std::uint64_t>& slot_root_digests) {
  ValidatedBatch validated;
  if (previous_progress.size() != request.slot_count ||
      slot_universe_masses.size() != request.slot_count ||
      slot_root_digests.size() != request.slot_count) {
    throw std::logic_error(
        "the Phase 15 host continuation has a foreign slot extent");
  }
  validated.authorized_slot_count = request.slot_count;
  validated.next_progress = previous_progress;
  const bool host_fake =
      batch.execution_kind == ExecutionKind::host_fake;
  // Each internal subdivision relaunch grants every slot a fresh gate
  // quantum, so the per-chunk evaluation count is bounded by the launched
  // budget, exactly like the pair frontier's node-visit accounting.
  const std::uint64_t launched_gate_evaluation_budget =
      static_cast<std::uint64_t>(checked_size_product(
          batch.traversal_subdivision_count,
          request.gate_evaluations_per_slot_per_chunk,
          "the Phase 15 launched gate-evaluation budget overflows "
          "size_t"));
  bool all_complete = true;
  bool any_chunk_ready = false;
  bool any_fatal = false;
  for (std::size_t slot_index = 0U;
       slot_index < batch.host_slot_controls.size();
       ++slot_index) {
    const SlotControl& control = batch.host_slot_controls[slot_index];
    const SlotProgress& previous = previous_progress[slot_index];
    if (control.failure_code !=
        static_cast<std::uint64_t>(FailureCode::none)) {
      throw std::runtime_error(
          "a Phase 15 higher-support slot reported an internal failure "
          "code; the context is poisoned and no records are published");
    }
    const exact::BigInt well_delta = bigint_from_words(
        control.well_prune_mass_delta_lo,
        control.well_prune_mass_delta_hi);
    const exact::BigInt rank_delta = bigint_from_words(
        control.rank_prune_mass_delta_lo,
        control.rank_prune_mass_delta_hi);
    const exact::BigInt terminal_delta = bigint_from_words(
        control.terminal_mass_delta_lo, control.terminal_mass_delta_hi);
    const exact::BigInt well_cumulative = bigint_from_words(
        control.well_prune_mass_cumulative_lo,
        control.well_prune_mass_cumulative_hi);
    const exact::BigInt rank_cumulative = bigint_from_words(
        control.rank_prune_mass_cumulative_lo,
        control.rank_prune_mass_cumulative_hi);
    const exact::BigInt terminal_cumulative = bigint_from_words(
        control.terminal_mass_cumulative_lo,
        control.terminal_mass_cumulative_hi);
    const exact::BigInt unresolved = bigint_from_words(
        control.unresolved_mass_lo, control.unresolved_mass_hi);
    if (control.root_entry_digest != slot_root_digests[slot_index] ||
        control.tile_epoch != request.tile_epoch ||
        control.chunk_sequence != request.chunk_sequence ||
        control.reserved_zero != 0U) {
      throw std::runtime_error(
          "a Phase 15 higher-support slot control violated its root "
          "digest, epoch, sequence, or reserved-word contract");
    }
    if (well_cumulative != previous.well_prune_mass + well_delta ||
        rank_cumulative != previous.rank_prune_mass + rank_delta ||
        terminal_cumulative != previous.terminal_mass + terminal_delta) {
      throw std::runtime_error(
          "a Phase 15 higher-support slot control broke its cumulative "
          "= previous + delta exact mass identity");
    }
    if (slot_universe_masses[slot_index] !=
        well_cumulative + rank_cumulative + terminal_cumulative +
            unresolved) {
      throw std::runtime_error(
          "a Phase 15 higher-support slot control returned an invalid "
          "local support partition");
    }
    if (control.prune_record_count >
            static_cast<std::uint64_t>(
                request.prune_records_per_slot) ||
        control.terminal_record_count >
            static_cast<std::uint64_t>(
                request.terminal_records_per_slot) ||
        control.probe_receipt_count >
            static_cast<std::uint64_t>(
                request.probe_receipts_per_slot) ||
        control.stack_high_water >
            static_cast<std::uint64_t>(request.products_per_slot) ||
        // Inline width escalations are bounded by the gate evaluations
        // that triggered them, never by the deferred queue extent: each
        // escalated decision consumed exactly one gate.
        control.deferred_int512_count >
            launched_gate_evaluation_budget ||
        control.deferred_int1024_count >
            launched_gate_evaluation_budget ||
        control.rational_drain_count >
            launched_gate_evaluation_budget ||
        control.gate_evaluation_count >
            launched_gate_evaluation_budget) {
      throw std::runtime_error(
          "a Phase 15 higher-support slot control exceeded a fixed "
          "per-slot segment or launched budget cap");
    }
    if (host_fake &&
        (control.deferred_int512_count != 0U ||
         control.deferred_int1024_count != 0U ||
         control.rational_drain_count != 0U)) {
      throw std::runtime_error(
          "the Phase 15 host fake forged deferred wide-decision or "
          "rational-drain activity");
    }
    if ((control.prune_record_count == 0U) !=
            (well_delta + rank_delta == 0) ||
        (control.terminal_record_count == 0U) != (terminal_delta == 0)) {
      throw std::runtime_error(
          "a Phase 15 higher-support slot control forged pruned or "
          "terminal mass without matching records");
    }

    const auto status = static_cast<SlotStatus>(control.status);
    const HigherSupportDeviceTiledFrontierStopReason stop_reason =
        validate_stop_reason(control.stop_reason);
    const HigherSupportDeviceTiledFrontierYieldReason yield_reason =
        validate_yield_reason(control.yield_reason);
    bool complete = false;
    switch (status) {
      case SlotStatus::active:
        throw std::runtime_error(
            "a Phase 15 launcher returned an active slot instead of "
            "finishing its bounded chunk");
      case SlotStatus::chunk_ready:
        if (stop_reason !=
                HigherSupportDeviceTiledFrontierStopReason::none ||
            yield_reason ==
                HigherSupportDeviceTiledFrontierYieldReason::none ||
            unresolved == 0 || previous.complete ||
            (yield_reason ==
                     HigherSupportDeviceTiledFrontierYieldReason::
                         prune_segment_full &&
                 control.prune_record_count !=
                     static_cast<std::uint64_t>(
                         request.prune_records_per_slot) &&
                 // The fixed receipt segment admits at most one
                 // worst-case rank-prune witness plan, so a prune yield
                 // is also honest when the next prune's receipts can no
                 // longer fit; the per-prune demand is bounded by the
                 // request's own witness thresholds.
                 checked_u64_sum(
                     control.probe_receipt_count,
                     static_cast<std::uint64_t>(std::max(
                         higher_support_device_tiled_frontier_required_witness_count(
                             request.terminal_policy,
                             3U,
                             request.maximum_relevant_closed_rank),
                         higher_support_device_tiled_frontier_required_witness_count(
                             request.terminal_policy,
                             4U,
                             request.maximum_relevant_closed_rank))),
                     "the Phase 15 receipt-exhaustion bound overflows "
                     "uint64_t") <=
                     static_cast<std::uint64_t>(
                         request.probe_receipts_per_slot)) ||
            (yield_reason ==
                     HigherSupportDeviceTiledFrontierYieldReason::
                         terminal_segment_full &&
                 control.terminal_record_count !=
                     static_cast<std::uint64_t>(
                         request.terminal_records_per_slot)) ||
            (yield_reason ==
                     HigherSupportDeviceTiledFrontierYieldReason::
                         deferred_queue_full &&
                 control.deferred_int512_count !=
                     static_cast<std::uint64_t>(
                         request.pending_decisions_per_slot) &&
                 control.deferred_int1024_count !=
                     static_cast<std::uint64_t>(
                         request.pending_decisions_per_slot))) {
          throw std::runtime_error(
              "a Phase 15 chunk-ready slot did not exhaust its reported "
              "output segment");
        }
        any_chunk_ready = true;
        if (validated.yield_reason ==
            HigherSupportDeviceTiledFrontierYieldReason::none) {
          validated.yield_reason = yield_reason;
        } else if (validated.yield_reason != yield_reason) {
          validated.yield_reason =
              HigherSupportDeviceTiledFrontierYieldReason::
                  mixed_segments_full;
        }
        break;
      case SlotStatus::complete:
        complete = true;
        if (stop_reason !=
                HigherSupportDeviceTiledFrontierStopReason::none ||
            yield_reason !=
                HigherSupportDeviceTiledFrontierYieldReason::none ||
            unresolved != 0 ||
            (previous.complete &&
             (well_delta != 0 || rank_delta != 0 ||
              terminal_delta != 0 || control.prune_record_count != 0U ||
              control.terminal_record_count != 0U ||
              control.probe_receipt_count != 0U ||
              control.expansion_count != 0U ||
              control.gate_evaluation_count != 0U ||
              control.probe_root_decision_count != 0U ||
              control.probe_fallback_decision_count != 0U ||
              control.stack_high_water != 0U))) {
          throw std::runtime_error(
              "a Phase 15 complete slot did not close its local support "
              "partition");
        }
        break;
      case SlotStatus::fatal:
        // A fatal slot must have rolled back its chunk entirely: zero
        // deltas, zero records, and the only admissible terminal stop is
        // the exhausted subdivision budget (any internal failure code
        // already threw above).
        if (stop_reason !=
                HigherSupportDeviceTiledFrontierStopReason::
                    subdivision_capacity ||
            yield_reason !=
                HigherSupportDeviceTiledFrontierYieldReason::none ||
            previous.complete || well_delta != 0 || rank_delta != 0 ||
            terminal_delta != 0 || control.prune_record_count != 0U ||
            control.terminal_record_count != 0U ||
            control.probe_receipt_count != 0U) {
          throw std::runtime_error(
              "a Phase 15 fatal slot omitted its rollback or its "
              "subdivision-capacity proof");
        }
        any_fatal = true;
        validated.stop_reason = stop_reason;
        break;
      default:
        throw std::runtime_error(
            "a Phase 15 higher-support slot control returned an unknown "
            "status");
    }

    validated.prune_record_count = checked_size_sum(
        validated.prune_record_count,
        checked_size(control.prune_record_count,
                     "a Phase 15 prune count does not fit size_t"),
        "the Phase 15 chunk prune record count overflows size_t");
    validated.terminal_record_count = checked_size_sum(
        validated.terminal_record_count,
        checked_size(control.terminal_record_count,
                     "a Phase 15 terminal count does not fit size_t"),
        "the Phase 15 chunk terminal record count overflows size_t");
    validated.probe_receipt_count = checked_size_sum(
        validated.probe_receipt_count,
        checked_size(control.probe_receipt_count,
                     "a Phase 15 receipt count does not fit size_t"),
        "the Phase 15 chunk probe receipt count overflows size_t");
    validated.expansion_count = checked_u64_sum(
        validated.expansion_count,
        control.expansion_count,
        "the Phase 15 chunk expansion count overflowed uint64_t");
    validated.gate_evaluation_count = checked_u64_sum(
        validated.gate_evaluation_count,
        control.gate_evaluation_count,
        "the Phase 15 chunk gate-evaluation count overflowed uint64_t");
    validated.probe_root_decision_count = checked_u64_sum(
        validated.probe_root_decision_count,
        control.probe_root_decision_count,
        "the Phase 15 chunk probe-root count overflowed uint64_t");
    validated.probe_fallback_decision_count = checked_u64_sum(
        validated.probe_fallback_decision_count,
        control.probe_fallback_decision_count,
        "the Phase 15 chunk probe-fallback count overflowed uint64_t");
    validated.deferred_int512_count = checked_u64_sum(
        validated.deferred_int512_count,
        control.deferred_int512_count,
        "the Phase 15 chunk deferred int512 count overflowed uint64_t");
    validated.deferred_int1024_count = checked_u64_sum(
        validated.deferred_int1024_count,
        control.deferred_int1024_count,
        "the Phase 15 chunk deferred int1024 count overflowed uint64_t");
    validated.rational_drain_count = checked_u64_sum(
        validated.rational_drain_count,
        control.rational_drain_count,
        "the Phase 15 chunk rational-drain count overflowed uint64_t");
    validated.stack_high_water =
        std::max(validated.stack_high_water, control.stack_high_water);
    validated.well_prune_mass += well_delta;
    validated.rank_prune_mass += rank_delta;
    validated.terminal_mass += terminal_delta;
    validated.next_progress[slot_index] = SlotProgress{
        well_cumulative, rank_cumulative, terminal_cumulative, complete};
    all_complete = all_complete && complete;
  }
  if (any_fatal && any_chunk_ready) {
    throw std::runtime_error(
        "a Phase 15 batch mixed terminal failure with resumable output");
  }
  if (any_fatal &&
      (validated.well_prune_mass != 0 ||
       validated.rank_prune_mass != 0 || validated.terminal_mass != 0 ||
       validated.prune_record_count != 0U ||
       validated.terminal_record_count != 0U ||
       validated.probe_receipt_count != 0U)) {
    throw std::runtime_error(
        "a Phase 15 fatal batch attempted to publish partial outputs");
  }
  if (any_fatal &&
      batch.traversal_subdivision_count !=
          request.maximum_subdivision_count) {
    throw std::runtime_error(
        "a Phase 15 fatal batch did not exhaust its certified "
        "subdivision budget");
  }
  if (!all_complete && !any_chunk_ready && !any_fatal) {
    throw std::runtime_error(
        "a Phase 15 launcher returned without a yield, completion, or "
        "fatal stop");
  }
  if (batch.capacity_yield_resumable != any_chunk_ready) {
    throw std::runtime_error(
        "a Phase 15 batch forged its resumable-capacity flag");
  }
  validated.tile_complete = all_complete;
  validated.capacity_yield = any_chunk_ready;
  validated.fatal = any_fatal;
  validated.completed_slot_count =
      all_complete ? request.slot_count : 0U;
  return validated;
}

[[nodiscard]] ValidatedBatch validate_batch(
    const DeviceBatch& batch,
    const AdoptedTraversal& traversal,
    const Request& request,
    const std::vector<SlotProgress>& previous_progress,
    const std::vector<exact::BigInt>& slot_universe_masses,
    const std::vector<std::uint64_t>& slot_root_digests) {
  validate_batch_envelope(batch, traversal, request);
  return validate_slot_controls(
      batch,
      request,
      previous_progress,
      slot_universe_masses,
      slot_root_digests);
}

}  // namespace

HigherSupportDeviceTiledRecordDrainLease::
    HigherSupportDeviceTiledRecordDrainLease(
        HigherSupportDeviceTiledRecordDrainLeaseAudit audit,
        std::shared_ptr<void> retained_owner,
        std::shared_ptr<void> source_owner_authority,
        std::shared_ptr<const void> source_cloud_identity_authority,
        const void* prune_records,
        const void* terminal_records,
        const void* probe_receipts,
        const void* slot_controls,
        std::size_t authorized_slot_control_extent,
        int cuda_device,
        bool host_fake)
    : audit_(std::move(audit)),
      retained_owner_(std::move(retained_owner)),
      source_owner_authority_(std::move(source_owner_authority)),
      source_cloud_identity_authority_(
          std::move(source_cloud_identity_authority)),
      prune_records_(prune_records),
      terminal_records_(terminal_records),
      probe_receipts_(probe_receipts),
      slot_controls_(slot_controls),
      authorized_slot_control_extent_(authorized_slot_control_extent),
      cuda_device_(cuda_device),
      host_fake_(host_fake) {}

bool HigherSupportDeviceTiledRecordDrainLease::ready() const noexcept {
  // Independent self-audit: every expectation below is recomputed from the
  // copied audit alone, never from the issuing context.
  const std::size_t slot_count =
      audit_.slot_end > audit_.slot_begin
          ? audit_.slot_end - audit_.slot_begin
          : 0U;
  const bool closed_rank_policy =
      audit_.terminal_policy ==
      HigherSupportDeviceTiledFrontierTerminalPolicy::closed_rank_window;
  const bool carrier_policy =
      audit_.terminal_policy ==
      HigherSupportDeviceTiledFrontierTerminalPolicy::
          strict_interior_carrier_q3;
  if (audit_.schema_version !=
          higher_support_device_tiled_frontier_schema_version ||
      !retained_owner_ || !source_owner_authority_ ||
      !source_cloud_identity_authority_ ||
      !higher_support_device_tiled_frontier_terminal_policy_known(
          audit_.terminal_policy) ||
      audit_.maximum_relevant_closed_rank < 2U ||
      audit_.maximum_relevant_closed_rank >
          higher_support_device_tiled_frontier_maximum_closed_rank ||
      audit_.source_snapshot_epoch == 0U ||
      audit_.record_buffer_epoch == 0U || audit_.tile_epoch == 0U ||
      audit_.chunk_sequence == 0U || audit_.point_count < 3U ||
      static_cast<std::uint64_t>(audit_.point_count) >
          higher_support_device_tiled_frontier_maximum_point_count ||
      audit_.certified_node_count != 2U * audit_.point_count - 1U ||
      audit_.certified_maximum_lbvh_depth >
          higher_support_device_tiled_frontier_maximum_certified_lbvh_depth ||
      audit_.fixed_products_per_slot !=
          higher_support_device_tiled_frontier_products_per_slot ||
      higher_support_device_tiled_frontier_proved_stack_bound(
          audit_.certified_maximum_lbvh_depth) >
          audit_.fixed_products_per_slot ||
      audit_.fixed_prune_records_per_slot !=
          higher_support_device_tiled_frontier_prune_records_per_slot ||
      audit_.fixed_terminal_records_per_slot !=
          higher_support_device_tiled_frontier_terminal_records_per_slot ||
      audit_.fixed_probe_receipts_per_slot !=
          higher_support_device_tiled_frontier_probe_receipts_per_slot ||
      audit_.slot_begin != 0U || slot_count == 0U ||
      slot_count >
          higher_support_device_tiled_frontier_maximum_slot_tile_capacity ||
      authorized_slot_control_extent_ != slot_count ||
      audit_.physical_slot_control_capacity != slot_count ||
      audit_.physical_product_record_capacity !=
          slot_count * audit_.fixed_products_per_slot ||
      audit_.physical_prune_record_capacity !=
          slot_count * audit_.fixed_prune_records_per_slot ||
      audit_.physical_terminal_record_capacity !=
          slot_count * audit_.fixed_terminal_records_per_slot ||
      audit_.physical_probe_receipt_capacity !=
          slot_count * audit_.fixed_probe_receipts_per_slot ||
      audit_.prune_record_count >
          audit_.physical_prune_record_capacity ||
      audit_.terminal_record_count >
          audit_.physical_terminal_record_capacity ||
      audit_.probe_receipt_count >
          audit_.physical_probe_receipt_capacity ||
      audit_.prune_record_count + audit_.terminal_record_count == 0U ||
      (audit_.yield_reason ==
           HigherSupportDeviceTiledFrontierYieldReason::none) !=
          !audit_.resumable_after_lease_release ||
      audit_.process_restart_resumable ||
      !audit_.traversal_owner_retained ||
      !audit_.source_cloud_identity_retained ||
      !audit_.output_owner_retained ||
      !audit_.output_buffers_detached_for_tile_lifetime ||
      audit_.host_fake_lifecycle_exercised ==
          audit_.cuda_device_storage_retained ||
      audit_.censored_slot_outputs_withheld ||
      audit_.bigint_support_mass_transferred ||
      audit_.floating_point_decision_performed ||
      audit_.closed_rank_window_receipts_sealed_to_policy !=
          closed_rank_policy ||
      audit_.strict_interior_carrier_receipts_sealed_to_policy !=
          carrier_policy ||
      !audit_.strict_interior_carrier_never_authorizes_gamma2_drop ||
      audit_.exact_higher_support_terminal_classification_native_cuda ||
      audit_.dense_product_fallback_performed ||
      audit_.global_product_frontier_materialized ||
      audit_.ordinary_or_higher_order_delaunay_materialized ||
      audit_.global_cell_coface_or_incidence_arena_materialized ||
      audit_.slo_claimed || audit_.public_status_claimed) {
    return false;
  }
  if (host_fake_) {
    return audit_.host_fake_lifecycle_exercised &&
           !audit_.cuda_device_storage_retained &&
           audit_.host_fake_record_segments_resident &&
           prune_records_ != nullptr && terminal_records_ != nullptr &&
           probe_receipts_ != nullptr && slot_controls_ != nullptr &&
           cuda_device_ == -1;
  }
  return !audit_.host_fake_lifecycle_exercised &&
         audit_.cuda_device_storage_retained &&
         !audit_.host_fake_record_segments_resident &&
         prune_records_ != nullptr && terminal_records_ != nullptr &&
         probe_receipts_ != nullptr && slot_controls_ != nullptr &&
         cuda_device_ >= 0;
}

bool HigherSupportDeviceTiledRecordDrainLease::cuda_resident()
    const noexcept {
  return ready() && !host_fake_;
}

bool HigherSupportDeviceTiledRecordDrainLease::host_fake()
    const noexcept {
  return ready() && host_fake_;
}

bool HigherSupportDeviceTiledRecordDrainLease::
    host_readable_record_segments() const noexcept {
  // Deliberately NOT `!cuda_resident()`: an unready lease exposes nothing
  // either.  The host fake allocates its segments in host memory; the CUDA
  // engine allocates them with cudaMalloc and republishes those pointers
  // unchanged, so they stay unreadable from the host until the driver
  // stages them device-to-host.
  return ready() && host_fake_;
}

detail::Phase15HigherSupportDeviceTiledRecordDrainPrivateViews
detail::Phase15HigherSupportDeviceTiledRecordDrainPrivateViewAccess::
    inspect(
        const HigherSupportDeviceTiledRecordDrainLease& lease) noexcept {
  Phase15HigherSupportDeviceTiledRecordDrainPrivateViews views;
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
  views.prune_records =
      static_cast<const Phase15HigherSupportDeviceTiledPruneRecord*>(
          lease.prune_records_);
  views.terminal_records =
      static_cast<const Phase15HigherSupportDeviceTiledTerminalRecord*>(
          lease.terminal_records_);
  views.probe_receipts =
      static_cast<const Phase15HigherSupportDeviceTiledProbeReceipt*>(
          lease.probe_receipts_);
  views.slot_controls =
      static_cast<const Phase15HigherSupportDeviceTiledSlotControl*>(
          lease.slot_controls_);
  views.source_snapshot_epoch = lease.audit_.source_snapshot_epoch;
  views.record_buffer_epoch = lease.audit_.record_buffer_epoch;
  views.point_count = lease.audit_.point_count;
  views.certified_node_count = lease.audit_.certified_node_count;
  views.terminal_policy = lease.audit_.terminal_policy;
  views.maximum_relevant_closed_rank =
      lease.audit_.maximum_relevant_closed_rank;
  views.prune_record_stride = lease.audit_.fixed_prune_records_per_slot;
  views.terminal_record_stride =
      lease.audit_.fixed_terminal_records_per_slot;
  views.probe_receipt_stride =
      lease.audit_.fixed_probe_receipts_per_slot;
  views.physical_slot_control_capacity =
      lease.audit_.physical_slot_control_capacity;
  views.authorized_slot_control_extent =
      lease.authorized_slot_control_extent_;
  views.slot_begin = lease.audit_.slot_begin;
  views.slot_end = lease.audit_.slot_end;
  views.cuda_device = lease.cuda_device_;
  views.host_fake = lease.host_fake_;
  views.ready = true;
  return views;
}

HigherSupportDeviceTiledFrontierContext::
    HigherSupportDeviceTiledFrontierContext(
        MortonLbvhDeviceTraversalLease&& traversal_lease,
        HigherSupportDeviceTiledFrontierConfig config)
    : config_(validate_config(config)) {
  if (!traversal_lease.ready()) {
    throw std::invalid_argument(
        "a Phase 15 higher-support device tiled frontier requires a "
        "ready Phase 14 traversal lease");
  }
  // The source audit must be copied before the move consumes the lease.
  const MortonLbvhDeviceTraversalLeaseAudit source_audit =
      traversal_lease.audit();
  AdoptedTraversal adopted =
      detail::adopt_phase15_higher_support_device_tiled_traversal(
          std::move(traversal_lease));
  validate_adopted_traversal(adopted, source_audit);
  if (static_cast<std::uint64_t>(adopted.point_count) >
      higher_support_device_tiled_frontier_maximum_point_count) {
    throw std::invalid_argument(
        "a Phase 15 higher-support device tiled frontier certifies its "
        "unsigned 128-bit slot accounting for at most 2^31 points");
  }

  point_count_ = adopted.point_count;
  certified_node_count_ = adopted.certified_node_count;
  state_ = std::make_shared<
      detail::Phase15HigherSupportDeviceTiledFrontierContextState>();
  host_ = std::make_unique<
      detail::Phase15HigherSupportDeviceTiledFrontierHostState>(
      std::move(adopted));
}

HigherSupportDeviceTiledFrontierContext::
    ~HigherSupportDeviceTiledFrontierContext() noexcept = default;

HigherSupportDeviceTiledFrontierContext::
    HigherSupportDeviceTiledFrontierContext(
        HigherSupportDeviceTiledFrontierContext&&) noexcept = default;

HigherSupportDeviceTiledFrontierContext&
HigherSupportDeviceTiledFrontierContext::operator=(
    HigherSupportDeviceTiledFrontierContext&&) noexcept = default;

bool HigherSupportDeviceTiledFrontierContext::ready() const noexcept {
  return state_ != nullptr && host_ != nullptr && !state_->poisoned() &&
         host_->traversal.retained_owner != nullptr &&
         host_->traversal.source_cloud_identity != nullptr &&
         point_count_ == host_->traversal.point_count &&
         certified_node_count_ == host_->traversal.certified_node_count;
}

bool HigherSupportDeviceTiledFrontierContext::poisoned() const noexcept {
  return state_ != nullptr && state_->poisoned();
}

bool HigherSupportDeviceTiledFrontierContext::tile_active()
    const noexcept {
  return state_ != nullptr && host_ != nullptr && host_->active_tile;
}

void HigherSupportDeviceTiledFrontierContext::open_tile(
    std::span<const hierarchy::ExactHigherSupportFrontierEntry> entries) {
  if (state_ == nullptr || host_ == nullptr) {
    throw std::logic_error(
        "a moved-from Phase 15 higher-support device tiled context "
        "cannot open a slot tile");
  }
  std::vector<hierarchy::ExactHigherSupportFrontierEntry> slot_entries;
  std::vector<exact::BigInt> slot_universe_masses;
  std::vector<std::uint64_t> slot_root_digests;
  exact::BigInt tile_universe_mass{0};
  state_->with_launcher_section(
      [&]() {
        // Everything here is a reversible precondition: no work has been
        // scheduled, so a rejected tile leaves the context reusable.
        if (!active_record_drain_authority_.expired()) {
          throw std::logic_error(
              "the Phase 15 higher-support device tiled context cannot "
              "open a slot tile while its preceding record drain lease "
              "is still alive");
        }
        if (terminally_censored_) {
          throw std::logic_error(
              "a terminally censored Phase 15 higher-support device "
              "tiled context cannot open a slot tile");
        }
        if (host_->active_tile) {
          throw std::logic_error(
              "the Phase 15 higher-support device tiled context cannot "
              "open a slot tile while another tile is active");
        }
        if (entries.empty() ||
            entries.size() > config_.slot_tile_capacity) {
          throw std::invalid_argument(
              "a Phase 15 higher-support slot tile must hold between one "
              "entry and the configured slot tile capacity");
        }
        slot_entries.reserve(entries.size());
        slot_universe_masses.reserve(entries.size());
        slot_root_digests.reserve(entries.size());
        for (const hierarchy::ExactHigherSupportFrontierEntry& entry :
             entries) {
          validate_frontier_entry(
              entry,
              point_count_,
              certified_node_count_,
              config_.terminal_policy);
          exact::BigInt mass = certified_slot_universe_mass(entry);
          tile_universe_mass += mass;
          slot_universe_masses.push_back(std::move(mass));
          slot_root_digests.push_back(
              detail::phase15_higher_support_device_tiled_root_entry_digest(
                  entry));
          slot_entries.push_back(entry);
        }
      },
      [&]() {
        const std::uint64_t tile_epoch = state_->advance_epoch();
        host_->active_tile = true;
        host_->slot_entries = std::move(slot_entries);
        host_->slot_universe_masses = std::move(slot_universe_masses);
        host_->slot_root_digests = std::move(slot_root_digests);
        host_->tile_universe_mass = std::move(tile_universe_mass);
        host_->active_tile_epoch = tile_epoch;
        host_->next_chunk_sequence = 1U;
        host_->slot_progress.assign(
            host_->slot_entries.size(), SlotProgress{});
      });
}

HigherSupportDeviceTiledFrontierAdvance
HigherSupportDeviceTiledFrontierContext::advance() {
  if (state_ == nullptr || host_ == nullptr) {
    throw std::logic_error(
        "a moved-from Phase 15 higher-support device tiled context "
        "cannot advance");
  }
  return state_->with_launcher_section(
      [this]() {
        if (!active_record_drain_authority_.expired()) {
          throw std::logic_error(
              "the Phase 15 higher-support device tiled context cannot "
              "launch while its preceding record drain lease is still "
              "alive");
        }
        if (!terminally_censored_ && !host_->active_tile) {
          throw std::logic_error(
              "the Phase 15 higher-support device tiled context cannot "
              "advance without an open slot tile");
        }
      },
      [this]() {
        const auto base_audit = [this]() {
          HigherSupportDeviceTiledFrontierAudit audit;
          audit.advance_sequence = advance_sequence_;
          audit.source_snapshot_epoch =
              host_->traversal.source_snapshot_epoch;
          audit.point_count = point_count_;
          audit.certified_node_count = certified_node_count_;
          audit.certified_maximum_lbvh_depth =
              config_.certified_maximum_lbvh_depth;
          audit.terminal_policy = config_.terminal_policy;
          audit.maximum_relevant_closed_rank =
              config_.maximum_relevant_closed_rank;
          audit.slot_tile_capacity = config_.slot_tile_capacity;
          audit.gate_evaluations_per_slot_per_chunk =
              config_.gate_evaluations_per_slot_per_chunk;
          audit.maximum_subdivision_count =
              config_.maximum_subdivision_count;
          audit.fixed_products_per_slot =
              higher_support_device_tiled_frontier_products_per_slot;
          audit.fixed_prune_records_per_slot =
              higher_support_device_tiled_frontier_prune_records_per_slot;
          audit.fixed_terminal_records_per_slot =
              higher_support_device_tiled_frontier_terminal_records_per_slot;
          audit.fixed_probe_receipts_per_slot =
              higher_support_device_tiled_frontier_probe_receipts_per_slot;
          audit.fixed_pending_decisions_per_slot =
              higher_support_device_tiled_frontier_pending_decisions_per_slot;
          audit.cumulative_well_prune_support_mass =
              host_->cumulative_well_prune_mass;
          audit.cumulative_rank_prune_support_mass =
              host_->cumulative_rank_prune_mass;
          audit.cumulative_terminal_support_mass =
              host_->cumulative_terminal_mass;
          audit.cumulative_resolved_support_mass =
              host_->cumulative_well_prune_mass +
              host_->cumulative_rank_prune_mass +
              host_->cumulative_terminal_mass;
          audit.completed_tile_count = completed_tile_count_;
          audit.launcher_call_count = launcher_call_count_;
          audit.cuda_device = host_->traversal.cuda_device;
          audit.source_traversal_lease_authenticated = true;
          audit.certified_lbvh_depth_stack_bound_enforced = true;
          audit.unsigned128_slot_mass_accounting_certified = true;
          audit.slot_universe_masses_host_bigint_revalidated = true;
          audit.fixed_per_slot_caps_enforced = true;
          audit.atomic_tile_commit_enforced = true;
          audit.censored_slot_outputs_withheld = terminally_censored_;
          audit.slot_partition_validated = true;
          audit.terminally_censored = terminally_censored_;
          audit.traversal_lease_owner_retained =
              host_->traversal.retained_owner != nullptr;
          audit.source_cloud_identity_retained =
              host_->traversal.source_cloud_identity != nullptr;
          audit.record_drain_lease_backpressure_bounded_to_one = true;
          audit.record_drain_lease_outstanding =
              !active_record_drain_authority_.expired();
          audit.host_fake_launcher_exercised =
              host_fake_launcher_exercised_;
          audit.cuda_execution_performed = cuda_execution_performed_;
          audit.probe_plan_order_receipt_commit_enforced = true;
          audit.substitute_probe_center_seed_policy_v1 = true;
          audit.closed_rank_window_receipts_sealed_to_policy =
              config_.terminal_policy ==
              HigherSupportDeviceTiledFrontierTerminalPolicy::
                  closed_rank_window;
          audit.strict_interior_carrier_receipts_sealed_to_policy =
              config_.terminal_policy ==
              HigherSupportDeviceTiledFrontierTerminalPolicy::
                  strict_interior_carrier_q3;
          audit.strict_interior_carrier_never_authorizes_gamma2_drop =
              true;
          return audit;
        };

        if (terminally_censored_) {
          ++advance_sequence_;
          HigherSupportDeviceTiledFrontierAdvance result;
          result.status =
              HigherSupportDeviceTiledFrontierStatus::censored;
          result.stop_reason = terminal_stop_reason_;
          result.yield_reason =
              HigherSupportDeviceTiledFrontierYieldReason::none;
          result.audit = base_audit();
          return result;
        }

        const std::size_t slot_count = host_->slot_entries.size();
        const bool resume_same_tile = host_->next_chunk_sequence > 1U;
        const std::uint64_t record_buffer_epoch =
            state_->advance_epoch();
        Request request;
        request.source_snapshot_epoch =
            host_->traversal.source_snapshot_epoch;
        request.record_buffer_epoch = record_buffer_epoch;
        request.tile_epoch = host_->active_tile_epoch;
        request.chunk_sequence = host_->next_chunk_sequence;
        request.point_count = point_count_;
        request.certified_node_count = certified_node_count_;
        request.certified_maximum_lbvh_depth =
            config_.certified_maximum_lbvh_depth;
        request.slot_count = slot_count;
        request.terminal_policy = config_.terminal_policy;
        request.maximum_relevant_closed_rank =
            config_.maximum_relevant_closed_rank;
        request.gate_evaluations_per_slot_per_chunk =
            config_.gate_evaluations_per_slot_per_chunk;
        request.maximum_subdivision_count =
            config_.maximum_subdivision_count;
        request.products_per_slot =
            higher_support_device_tiled_frontier_products_per_slot;
        request.prune_records_per_slot =
            higher_support_device_tiled_frontier_prune_records_per_slot;
        request.terminal_records_per_slot =
            higher_support_device_tiled_frontier_terminal_records_per_slot;
        request.probe_receipts_per_slot =
            higher_support_device_tiled_frontier_probe_receipts_per_slot;
        request.pending_decisions_per_slot =
            higher_support_device_tiled_frontier_pending_decisions_per_slot;
        // The certified roots cross the seam exactly once, on the first
        // chunk of a tile; a resumed chunk continues from retained state.
        request.root_entries =
            resume_same_tile
                ? std::span<
                      const hierarchy::ExactHigherSupportFrontierEntry>{}
                : std::span<
                      const hierarchy::ExactHigherSupportFrontierEntry>{
                      host_->slot_entries.data(),
                      host_->slot_entries.size()};
        request.resume_same_tile = resume_same_tile;

        DeviceBatch batch = detail::
            build_phase15_higher_support_device_tiled_frontier_on_device(
                *state_, host_->traversal, request);
        const ValidatedBatch validated = validate_batch(
            batch,
            host_->traversal,
            request,
            host_->slot_progress,
            host_->slot_universe_masses,
            host_->slot_root_digests);

        std::optional<HigherSupportDeviceTiledRecordDrainLease>
            record_drain;
        const std::size_t published_record_count = checked_size_sum(
            validated.prune_record_count,
            validated.terminal_record_count,
            "the Phase 15 published record count overflows size_t");
        if (!validated.fatal && published_record_count > 0U) {
          const bool host_fake =
              batch.execution_kind == ExecutionKind::host_fake;
          auto detached_authority =
              std::make_shared<DetachedTileAuthority>();
          detached_authority->traversal_owner =
              host_->traversal.retained_owner;
          detached_authority->source_cloud_identity =
              host_->traversal.source_cloud_identity;
          detached_authority->output_owner =
              std::move(batch.retained_output_owner);

          HigherSupportDeviceTiledRecordDrainLeaseAudit lease_audit;
          lease_audit.source_snapshot_epoch =
              request.source_snapshot_epoch;
          lease_audit.record_buffer_epoch = request.record_buffer_epoch;
          lease_audit.point_count = point_count_;
          lease_audit.certified_node_count = certified_node_count_;
          lease_audit.certified_maximum_lbvh_depth =
              config_.certified_maximum_lbvh_depth;
          lease_audit.terminal_policy = config_.terminal_policy;
          lease_audit.maximum_relevant_closed_rank =
              config_.maximum_relevant_closed_rank;
          lease_audit.tile_epoch = request.tile_epoch;
          lease_audit.chunk_sequence = request.chunk_sequence;
          lease_audit.slot_begin = 0U;
          lease_audit.slot_end = slot_count;
          lease_audit.prune_record_count = validated.prune_record_count;
          lease_audit.terminal_record_count =
              validated.terminal_record_count;
          lease_audit.probe_receipt_count =
              validated.probe_receipt_count;
          lease_audit.physical_product_record_capacity =
              batch.physical_product_record_capacity;
          lease_audit.physical_prune_record_capacity =
              batch.physical_prune_record_capacity;
          lease_audit.physical_terminal_record_capacity =
              batch.physical_terminal_record_capacity;
          lease_audit.physical_probe_receipt_capacity =
              batch.physical_probe_receipt_capacity;
          lease_audit.physical_slot_control_capacity =
              batch.physical_slot_control_capacity;
          lease_audit.fixed_products_per_slot =
              higher_support_device_tiled_frontier_products_per_slot;
          lease_audit.fixed_prune_records_per_slot =
              higher_support_device_tiled_frontier_prune_records_per_slot;
          lease_audit.fixed_terminal_records_per_slot =
              higher_support_device_tiled_frontier_terminal_records_per_slot;
          lease_audit.fixed_probe_receipts_per_slot =
              higher_support_device_tiled_frontier_probe_receipts_per_slot;
          lease_audit.yield_reason = validated.yield_reason;
          lease_audit.resumes_same_tile = request.resume_same_tile;
          lease_audit.resumable_after_lease_release =
              validated.capacity_yield;
          lease_audit.process_restart_resumable = false;
          lease_audit.traversal_owner_retained = true;
          lease_audit.source_cloud_identity_retained = true;
          lease_audit.output_owner_retained = true;
          lease_audit.output_buffers_detached_for_tile_lifetime = true;
          lease_audit.host_fake_lifecycle_exercised = host_fake;
          lease_audit.cuda_device_storage_retained = !host_fake;
          lease_audit.host_fake_record_segments_resident = host_fake;
          lease_audit.censored_slot_outputs_withheld = false;
          lease_audit.bigint_support_mass_transferred = false;
          lease_audit.floating_point_decision_performed = false;
          lease_audit.closed_rank_window_receipts_sealed_to_policy =
              config_.terminal_policy ==
              HigherSupportDeviceTiledFrontierTerminalPolicy::
                  closed_rank_window;
          lease_audit.strict_interior_carrier_receipts_sealed_to_policy =
              config_.terminal_policy ==
              HigherSupportDeviceTiledFrontierTerminalPolicy::
                  strict_interior_carrier_q3;
          lease_audit
              .strict_interior_carrier_never_authorizes_gamma2_drop =
              true;
          lease_audit
              .exact_higher_support_terminal_classification_native_cuda =
              false;

          HigherSupportDeviceTiledRecordDrainLease detached_lease{
              std::move(lease_audit),
              detached_authority,
              detached_authority->traversal_owner,
              detached_authority->source_cloud_identity,
              batch.prune_records,
              batch.terminal_records,
              batch.probe_receipts,
              batch.slot_controls,
              slot_count,
              batch.cuda_device,
              host_fake};
          record_drain.emplace(std::move(detached_lease));
          if (!record_drain->ready()) {
            throw std::runtime_error(
                "the Phase 15 output chunk did not form a valid "
                "detached record drain lease");
          }
          // The backpressure authority is armed only after the lease
          // passed its own independent self-audit.
          active_record_drain_authority_ = detached_authority;
        }

        if (!validated.fatal) {
          host_->cumulative_well_prune_mass +=
              validated.well_prune_mass;
          host_->cumulative_rank_prune_mass +=
              validated.rank_prune_mass;
          host_->cumulative_terminal_mass += validated.terminal_mass;
        }
        host_->slot_progress = validated.next_progress;

        exact::BigInt tile_universe = host_->tile_universe_mass;
        exact::BigInt tile_resolved{0};
        for (const SlotProgress& progress : host_->slot_progress) {
          tile_resolved += progress.well_prune_mass +
              progress.rank_prune_mass + progress.terminal_mass;
        }
        if (tile_resolved > tile_universe) {
          throw std::runtime_error(
              "the Phase 15 committed tile prefix exceeds its certified "
              "slot universe");
        }
        exact::BigInt tile_unresolved = tile_universe - tile_resolved;
        if (validated.tile_complete && tile_unresolved != 0) {
          throw std::runtime_error(
              "a Phase 15 complete tile left unresolved support mass");
        }

        if (validated.tile_complete) {
          completed_tile_count_ = checked_size_sum(
              completed_tile_count_,
              1U,
              "the Phase 15 completed tile count overflows size_t");
          host_->active_tile = false;
          host_->slot_entries.clear();
          host_->slot_universe_masses.clear();
          host_->slot_root_digests.clear();
          host_->slot_progress.clear();
          host_->tile_universe_mass = 0;
          host_->active_tile_epoch = 0U;
          host_->next_chunk_sequence = 0U;
        } else if (validated.capacity_yield) {
          if (host_->next_chunk_sequence ==
              std::numeric_limits<std::uint64_t>::max()) {
            throw std::overflow_error(
                "the Phase 15 tile chunk sequence overflowed uint64_t");
          }
          ++host_->next_chunk_sequence;
        } else {
          // Fatal: the tile closes with total retention, publishing
          // nothing, and the context censors terminally.
          host_->active_tile = false;
          host_->slot_entries.clear();
          host_->slot_universe_masses.clear();
          host_->slot_root_digests.clear();
          host_->slot_progress.clear();
          host_->tile_universe_mass = 0;
          host_->active_tile_epoch = 0U;
          host_->next_chunk_sequence = 0U;
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

        HigherSupportDeviceTiledFrontierAdvance result;
        if (validated.fatal) {
          result.status =
              HigherSupportDeviceTiledFrontierStatus::censored;
          result.stop_reason = validated.stop_reason;
          result.yield_reason =
              HigherSupportDeviceTiledFrontierYieldReason::none;
        } else if (validated.capacity_yield) {
          result.status =
              HigherSupportDeviceTiledFrontierStatus::chunk_ready;
          result.stop_reason =
              HigherSupportDeviceTiledFrontierStopReason::none;
          result.yield_reason = validated.yield_reason;
        } else {
          result.status = HigherSupportDeviceTiledFrontierStatus::
              frontier_tile_complete;
          result.stop_reason =
              HigherSupportDeviceTiledFrontierStopReason::none;
          result.yield_reason =
              HigherSupportDeviceTiledFrontierYieldReason::none;
        }
        result.record_drain = std::move(record_drain);

        HigherSupportDeviceTiledFrontierAudit audit = base_audit();
        audit.record_buffer_epoch = record_buffer_epoch;
        audit.tile_epoch = request.tile_epoch;
        audit.chunk_sequence = request.chunk_sequence;
        audit.yield_reason = validated.yield_reason;
        audit.resumes_same_tile = request.resume_same_tile;
        audit.resumable_capacity_yield = validated.capacity_yield;
        audit.transaction_slot_count = slot_count;
        audit.transaction_committed_slot_count =
            validated.completed_slot_count;
        audit.transaction_prune_record_count =
            validated.prune_record_count;
        audit.transaction_terminal_record_count =
            validated.terminal_record_count;
        audit.transaction_probe_receipt_count =
            validated.probe_receipt_count;
        audit.transaction_subdivision_count =
            batch.traversal_subdivision_count;
        audit.transaction_expansion_count = validated.expansion_count;
        audit.transaction_gate_evaluation_count =
            validated.gate_evaluation_count;
        audit.transaction_probe_root_decision_count =
            validated.probe_root_decision_count;
        audit.transaction_probe_fallback_decision_count =
            validated.probe_fallback_decision_count;
        audit.transaction_stack_high_water =
            validated.stack_high_water;
        audit.tile_universe_support_mass = std::move(tile_universe);
        audit.unresolved_support_mass = std::move(tile_unresolved);
        audit.cuda_kernel_launch_count = batch.kernel_launch_count;
        audit.cuda_synchronization_count = batch.synchronization_count;
        audit.slot_control_device_to_host_count =
            batch.slot_control_device_to_host_count;
        audit.slot_control_device_to_host_byte_count =
            batch.slot_control_device_to_host_byte_count;
        audit.resume_control_device_to_host_count =
            batch.resume_control_device_to_host_count;
        audit.resume_control_device_to_host_byte_count =
            batch.resume_control_device_to_host_byte_count;
        audit.deferred_int512_decision_count = checked_size(
            validated.deferred_int512_count,
            "the Phase 15 deferred int512 count does not fit size_t");
        audit.deferred_int1024_decision_count = checked_size(
            validated.deferred_int1024_count,
            "the Phase 15 deferred int1024 count does not fit size_t");
        audit.host_rational_drain_count = checked_size(
            validated.rational_drain_count,
            "the Phase 15 rational-drain count does not fit size_t");
        audit.cuda_device = batch.cuda_device;
        audit.record_drain_lease_published =
            result.record_drain.has_value();
        audit.record_drain_lease_outstanding =
            !active_record_drain_authority_.expired();
        audit.output_buffers_detached_for_tile_lifetime =
            result.record_drain.has_value();
        result.audit = std::move(audit);
        return result;
      });
}

}  // namespace morsehgp3d::gpu
