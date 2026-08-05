#pragma once

#include "phase15_higher_support_device_tiled_frontier_internal.hpp"

#include "phase15_exact_higher_support_product_fixed.cuh"
#include "phase15_exact_higher_support_product_fixed256.cuh"
#include "phase15_exact_higher_support_product_fixed512.cuh"

#include "morsehgp3d/spatial/lbvh.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

// Slot engine of the Phase 15 higher-support device tiled frontier.  This
// header is the single scientific authority shared by the native CUDA kernel
// and the host-compiled parity test: every function below is an exact
// transcription of the scientific host fake
// (fake_gpu_higher_support_device_tiled_frontier_launchers.cpp), with the
// host mirror geometry replaced by the resident traversal views and the
// unbounded host decision primitives replaced by the proven fixed-width
// ladder int256 -> int512 -> int1024 -> host rational drain.  Every verdict a
// fitting width produces is exact and definitive; escalation happens only on
// evaluation overflow, so the ladder reproduces the host decisions bit for
// bit while counting its escalations in the sealed slot-control counters.
//
// The engine never allocates, never throws and never touches a floating
// point value: errors the fake surfaces as host exceptions become fail-closed
// fatal failure codes that poison the owning context through the sealed
// slot-control validation.

namespace morsehgp3d::gpu::detail::higher_support_slot_engine {

#if defined(__CUDACC__)
#define MORSEHGP3D_PHASE15_HS_ENGINE_HD __host__ __device__
#else
#define MORSEHGP3D_PHASE15_HS_ENGINE_HD
#endif

namespace fixed256 = exact_higher_support_product_fixed256;
namespace fixed512 = exact_higher_support_product_fixed512;
namespace fixed1024 = exact_higher_support_product_fixed;

using EngineU128 = Phase15HigherSupportDeviceTiledUnsigned128;
using EngineBox = fixed1024::Binary64Aabb3;
using EngineProduct = Phase15HigherSupportDeviceTiledProductRecord;
using EnginePrune = Phase15HigherSupportDeviceTiledPruneRecord;
using EngineTerminal = Phase15HigherSupportDeviceTiledTerminalRecord;
using EngineReceipt = Phase15HigherSupportDeviceTiledProbeReceipt;

inline constexpr std::uint64_t engine_invalid_node_index =
    spatial::morton_lbvh_snapshot_invalid_node_index;
inline constexpr std::size_t engine_maximum_probe_candidate_count =
    hierarchy::higher_support_local_rank_probe_maximum_candidate_count;
inline constexpr std::size_t engine_maximum_probe_threshold =
    hierarchy::higher_support_local_rank_probe_maximum_threshold;

// Private ABI of the certified 80-byte traversal node arena, identical to
// spatial::MortonLbvhSnapshotNode.  The layout static_asserts below make the
// reinterpretation fail closed at compile time.
struct EngineNode {
  std::uint64_t lower_point_ids[3];
  std::uint64_t upper_point_ids[3];
  std::uint64_t left_child;
  std::uint64_t right_child;
  std::uint64_t leaf_begin;
  std::uint64_t leaf_end;
};

static_assert(std::is_standard_layout_v<EngineNode>);
static_assert(std::is_trivially_copyable_v<EngineNode>);
static_assert(sizeof(EngineNode) == 80U);
static_assert(sizeof(EngineNode) == sizeof(spatial::MortonLbvhSnapshotNode));
static_assert(offsetof(EngineNode, lower_point_ids) == 0U);
static_assert(offsetof(EngineNode, upper_point_ids) == 24U);
static_assert(offsetof(EngineNode, left_child) == 48U);
static_assert(offsetof(EngineNode, right_child) == 56U);
static_assert(offsetof(EngineNode, leaf_begin) == 64U);
static_assert(offsetof(EngineNode, leaf_end) == 72U);

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline bool engine_node_is_leaf(
    const EngineNode& node) noexcept {
  return node.left_child == engine_invalid_node_index;
}

// Read-only resident geometry of one adopted traversal.  coordinate_bits is
// axis-major (axis * point_count + point_id) exactly as retained by the
// Phase 14 traversal lease; morton_point_ids maps an active Morton leaf
// position to its canonical PointId; nodes is the certified strict-postorder
// arena whose last node is the root.
struct EngineGeometryView {
  const std::uint64_t* coordinate_bits{};
  const std::uint64_t* morton_point_ids{};
  const EngineNode* nodes{};
  std::uint64_t point_count{};
  std::uint64_t node_count{};
};

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline std::uint64_t
engine_root_index(const EngineGeometryView& geometry) noexcept {
  return geometry.node_count - 1U;
}

// Exact witness-rule node box: the per-axis bounds are the canonical
// coordinate bit patterns of the certified extremum PointIds, so the box
// equals the CPU stream's private node_box bit for bit.
MORSEHGP3D_PHASE15_HS_ENGINE_HD inline void engine_node_box(
    const EngineGeometryView& geometry,
    std::uint64_t node_index,
    EngineBox& box) noexcept {
  const EngineNode& node = geometry.nodes[node_index];
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    box.lower[axis] = geometry.coordinate_bits
        [axis * geometry.point_count + node.lower_point_ids[axis]];
    box.upper[axis] = geometry.coordinate_bits
        [axis * geometry.point_count + node.upper_point_ids[axis]];
  }
}

// ---------------------------------------------------------------------------
// Failure codes and engine-local run state (mirrors FakeSlotRunState).
// ---------------------------------------------------------------------------

enum class EngineRunState : std::uint8_t {
  active = 0U,
  paused = 1U,
  chunk_ready = 2U,
  complete = 3U,
  fatal = 4U,
};

enum class EngineWideKind : std::uint8_t {
  none = 0U,
  gate_no_well_centered = 1U,
  gate_all_well_centered = 2U,
  query_cell = 3U,
  terminal_geometry = 4U,
};

// Encoded verdict bytes written back by the host rational drain.  Gates use
// 0/1; query cells use the public 3-valued ordinal; terminal geometry packs
// the category in the low nibble and the rational-backend flag in bit 4.
inline constexpr std::uint8_t engine_terminal_verdict_rational_flag = 0x10U;

// ---------------------------------------------------------------------------
// Unsigned 128-bit slot masses (fail-closed mirrors of the fake helpers).
// ---------------------------------------------------------------------------

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline bool
engine_entry_mass(const EngineProduct& entry, EngineU128& out) noexcept {
  EngineU128 value{1U, 0U};
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    EngineU128 factor{};
    if (!phase15_higher_support_device_tiled_binomial_u128(
            entry.group_leaf_end[index] - entry.group_leaf_begin[index],
            entry.group_multiplicity[index],
            factor)) {
      return false;
    }
    if (factor.hi == 0U) {
      if (!phase15_higher_support_device_tiled_u128_multiply_small(
              value, factor.lo)) {
        return false;
      }
      continue;
    }
    // At most one group can carry multiplicity three or four, so at most one
    // factor is wide; a second wide factor is a certification failure.
    if (value.hi != 0U) {
      return false;
    }
    EngineU128 widened = factor;
    if (!phase15_higher_support_device_tiled_u128_multiply_small(
            widened, value.lo)) {
      return false;
    }
    value = widened;
  }
  out = value;
  return true;
}

// ---------------------------------------------------------------------------
// Root entry digest (device replica of the host FNV-1a digest; the host
// keeps its own copy authoritative and the parity suite pins the equality).
// ---------------------------------------------------------------------------

MORSEHGP3D_PHASE15_HS_ENGINE_HD inline void engine_hash_word(
    std::uint64_t& digest,
    std::uint64_t word) noexcept {
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= phase15_higher_support_device_tiled_fnv_prime;
  }
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline std::uint64_t
engine_root_entry_digest(const EngineProduct& entry) noexcept {
  std::uint64_t digest = phase15_higher_support_device_tiled_fnv_offset_basis;
  engine_hash_word(digest, entry.support_size);
  engine_hash_word(digest, entry.group_count);
  for (std::size_t index = 0U; index < 4U; ++index) {
    engine_hash_word(digest, entry.group_node_index[index]);
    engine_hash_word(digest, entry.group_leaf_begin[index]);
    engine_hash_word(digest, entry.group_leaf_end[index]);
    engine_hash_word(digest, entry.group_multiplicity[index]);
  }
  return digest;
}

// ---------------------------------------------------------------------------
// Geometry predicates over resident product records (fake mirror accessors).
// ---------------------------------------------------------------------------

// Fail-closed authentication of one group receipt against the resident node
// arena.  The fake throws here; the engine reports an internal-invariant
// fatal through the caller.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline bool
engine_authenticate_group(
    const EngineGeometryView& geometry,
    const EngineProduct& entry,
    std::size_t group_index) noexcept {
  const std::uint64_t node_index = entry.group_node_index[group_index];
  if (node_index >= geometry.node_count) {
    return false;
  }
  const EngineNode& node = geometry.nodes[node_index];
  return entry.group_leaf_begin[group_index] == node.leaf_begin &&
      entry.group_leaf_end[group_index] == node.leaf_end;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline bool
engine_support_boxes(
    const EngineGeometryView& geometry,
    const EngineProduct& entry,
    EngineBox boxes[4]) noexcept {
  std::size_t output_index = 0U;
  for (std::size_t group_index = 0U; group_index < entry.group_count;
       ++group_index) {
    if (!engine_authenticate_group(geometry, entry, group_index)) {
      return false;
    }
    EngineBox box{};
    engine_node_box(geometry, entry.group_node_index[group_index], box);
    for (std::size_t copy = 0U;
         copy < entry.group_multiplicity[group_index];
         ++copy) {
      if (output_index >= entry.support_size) {
        return false;
      }
      boxes[output_index] = box;
      ++output_index;
    }
  }
  return output_index == entry.support_size;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline bool
engine_entry_terminal(
    const EngineGeometryView& geometry,
    const EngineProduct& entry,
    bool& terminal) noexcept {
  terminal = false;
  if (entry.group_count != entry.support_size) {
    return true;
  }
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    if (!engine_authenticate_group(geometry, entry, index)) {
      return false;
    }
    if (entry.group_multiplicity[index] != 1U ||
        !engine_node_is_leaf(
            geometry.nodes[entry.group_node_index[index]])) {
      return true;
    }
  }
  terminal = true;
  return true;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline bool
engine_leaf_position_in_support_domain(
    std::uint64_t leaf_position,
    const EngineProduct& entry) noexcept {
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    if (entry.group_leaf_begin[index] <= leaf_position &&
        leaf_position < entry.group_leaf_end[index]) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline bool
engine_node_range_intersects_support_domain(
    const EngineNode& query,
    const EngineProduct& entry) noexcept {
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    if (query.leaf_begin < entry.group_leaf_end[index] &&
        entry.group_leaf_begin[index] < query.leaf_end) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline std::uint64_t
engine_possible_external_witness_count(
    const EngineGeometryView& geometry,
    const EngineProduct& entry) noexcept {
  std::uint64_t excluded = 0U;
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    excluded +=
        entry.group_leaf_end[index] - entry.group_leaf_begin[index];
  }
  return excluded > geometry.point_count
      ? 0U
      : geometry.point_count - excluded;
}

// ---------------------------------------------------------------------------
// Fixed-width decision ladder.  Every wrapper below composes the proven
// per-width primitives with the exact host decision rules; escalation is a
// pure overflow event and never changes a verdict.
// ---------------------------------------------------------------------------

enum class EngineGateVerdict : std::uint8_t {
  exact_false = 0U,
  certified = 1U,
  overflow = 2U,
};

enum class EngineQueryVerdict : std::uint8_t {
  inconclusive = 0U,
  strictly_inside = 1U,
  outside_or_boundary = 2U,
  overflow = 3U,
};

enum class EngineTerminalVerdict : std::uint8_t {
  affinely_dependent = 0U,
  boundary_reduced = 1U,
  exterior_circumcenter = 2U,
  minimal = 3U,
  overflow = 4U,
};

// The positive all-well-centred gate at one width: Gram determinant strictly
// positive and every barycentric numerator strictly positive.  The triangle
// specialisation of the universal gate deliberately does not participate,
// exactly as in the host analysis.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline EngineGateVerdict
engine_no_well_centered_256(
    const EngineBox boxes[4],
    std::size_t support_size) noexcept {
  const fixed256::Decision decision =
      fixed256::no_well_centered_support(boxes, support_size);
  if (decision == fixed256::Decision::certified) {
    return EngineGateVerdict::certified;
  }
  if (decision == fixed256::Decision::exact_false) {
    return EngineGateVerdict::exact_false;
  }
  return EngineGateVerdict::overflow;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline EngineGateVerdict
engine_no_well_centered_512(
    const EngineBox boxes[4],
    std::size_t support_size) noexcept {
  const fixed512::Decision decision =
      fixed512::no_well_centered_support(boxes, support_size);
  if (decision == fixed512::Decision::certified) {
    return EngineGateVerdict::certified;
  }
  if (decision == fixed512::Decision::exact_false) {
    return EngineGateVerdict::exact_false;
  }
  return EngineGateVerdict::overflow;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline EngineGateVerdict
engine_no_well_centered_1024(
    const EngineBox boxes[4],
    std::size_t support_size) noexcept {
  const fixed1024::Decision decision =
      fixed1024::no_well_centered_support(boxes, support_size);
  if (decision == fixed1024::Decision::certified) {
    return EngineGateVerdict::certified;
  }
  if (decision == fixed1024::Decision::exact_false) {
    return EngineGateVerdict::exact_false;
  }
  return EngineGateVerdict::overflow;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline EngineGateVerdict
engine_all_well_centered_256(
    const EngineBox boxes[4],
    std::size_t support_size) noexcept {
  fixed256::AlignedProduct product{};
  fixed256::SupportEvaluation support{};
  if (!fixed256::align_product(boxes, support_size, nullptr, product) ||
      !fixed256::evaluate_support(product, support)) {
    return EngineGateVerdict::overflow;
  }
  fixed256::Interval256 barycentric[4]{};
  if (!fixed256::barycentric_numerators(support, barycentric)) {
    return EngineGateVerdict::overflow;
  }
  if (fixed256::is_nonpositive(support.gram_determinant.lower)) {
    return EngineGateVerdict::exact_false;
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    if (fixed256::is_nonpositive(barycentric[index].lower)) {
      return EngineGateVerdict::exact_false;
    }
  }
  return EngineGateVerdict::certified;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline EngineGateVerdict
engine_all_well_centered_512(
    const EngineBox boxes[4],
    std::size_t support_size) noexcept {
  fixed512::AlignedProduct product{};
  fixed512::SupportEvaluation support{};
  if (!fixed512::align_product(boxes, support_size, nullptr, product) ||
      !fixed512::evaluate_support(product, support)) {
    return EngineGateVerdict::overflow;
  }
  fixed512::Interval512 barycentric[4]{};
  if (!fixed512::barycentric_numerators(support, barycentric)) {
    return EngineGateVerdict::overflow;
  }
  if (fixed512::is_nonpositive(support.gram_determinant.lower)) {
    return EngineGateVerdict::exact_false;
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    if (fixed512::is_nonpositive(barycentric[index].lower)) {
      return EngineGateVerdict::exact_false;
    }
  }
  return EngineGateVerdict::certified;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline EngineGateVerdict
engine_all_well_centered_1024(
    const EngineBox boxes[4],
    std::size_t support_size) noexcept {
  fixed1024::AlignedProduct product{};
  fixed1024::SupportEvaluation support{};
  if (!fixed1024::align_product(boxes, support_size, nullptr, product) ||
      !fixed1024::evaluate_support(product, support)) {
    return EngineGateVerdict::overflow;
  }
  fixed1024::Interval1024 barycentric[4]{};
  if (!fixed1024::barycentric_numerators(support, barycentric)) {
    return EngineGateVerdict::overflow;
  }
  if (fixed1024::is_nonpositive(support.gram_determinant.lower)) {
    return EngineGateVerdict::exact_false;
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    if (fixed1024::is_nonpositive(barycentric[index].lower)) {
      return EngineGateVerdict::exact_false;
    }
  }
  return EngineGateVerdict::certified;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline EngineQueryVerdict
engine_query_cell_256(
    const EngineBox boxes[4],
    std::size_t support_size,
    const EngineBox& query_box) noexcept {
  fixed256::AlignedProduct product{};
  fixed256::SupportEvaluation support{};
  fixed256::Interval256 power{};
  if (!fixed256::align_product(
          boxes, support_size, &query_box, product) ||
      !fixed256::evaluate_support(product, support) ||
      !fixed256::query_scaled_power(product, support, power)) {
    return EngineQueryVerdict::overflow;
  }
  if (fixed256::is_negative(power.upper)) {
    return EngineQueryVerdict::strictly_inside;
  }
  if (!fixed256::is_negative(power.lower)) {
    return EngineQueryVerdict::outside_or_boundary;
  }
  return EngineQueryVerdict::inconclusive;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline EngineQueryVerdict
engine_query_cell_512(
    const EngineBox boxes[4],
    std::size_t support_size,
    const EngineBox& query_box) noexcept {
  fixed512::AlignedProduct product{};
  fixed512::SupportEvaluation support{};
  fixed512::Interval512 power{};
  if (!fixed512::align_product(
          boxes, support_size, &query_box, product) ||
      !fixed512::evaluate_support(product, support) ||
      !fixed512::query_scaled_power(product, support, power)) {
    return EngineQueryVerdict::overflow;
  }
  if (fixed512::is_negative(power.upper)) {
    return EngineQueryVerdict::strictly_inside;
  }
  if (!fixed512::is_negative(power.lower)) {
    return EngineQueryVerdict::outside_or_boundary;
  }
  return EngineQueryVerdict::inconclusive;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline EngineQueryVerdict
engine_query_cell_1024(
    const EngineBox boxes[4],
    std::size_t support_size,
    const EngineBox& query_box) noexcept {
  fixed1024::AlignedProduct product{};
  fixed1024::SupportEvaluation support{};
  fixed1024::Interval1024 power{};
  if (!fixed1024::align_product(
          boxes, support_size, &query_box, product) ||
      !fixed1024::evaluate_support(product, support) ||
      !fixed1024::query_scaled_power(product, support, power)) {
    return EngineQueryVerdict::overflow;
  }
  if (fixed1024::is_negative(power.upper)) {
    return EngineQueryVerdict::strictly_inside;
  }
  if (!fixed1024::is_negative(power.lower)) {
    return EngineQueryVerdict::outside_or_boundary;
  }
  return EngineQueryVerdict::inconclusive;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline EngineTerminalVerdict
engine_terminal_geometry_width(
    int width_step,
    const EngineBox boxes[4],
    std::size_t support_size) noexcept {
  if (width_step == 0) {
    const fixed256::TerminalGeometryDecision decision =
        fixed256::terminal_support_geometry(boxes, support_size);
    switch (decision) {
      case fixed256::TerminalGeometryDecision::affinely_dependent:
        return EngineTerminalVerdict::affinely_dependent;
      case fixed256::TerminalGeometryDecision::boundary_reduced:
        return EngineTerminalVerdict::boundary_reduced;
      case fixed256::TerminalGeometryDecision::exterior_circumcenter:
        return EngineTerminalVerdict::exterior_circumcenter;
      case fixed256::TerminalGeometryDecision::minimal:
        return EngineTerminalVerdict::minimal;
      default:
        return EngineTerminalVerdict::overflow;
    }
  }
  if (width_step == 1) {
    const fixed512::TerminalGeometryDecision decision =
        fixed512::terminal_support_geometry(boxes, support_size);
    switch (decision) {
      case fixed512::TerminalGeometryDecision::affinely_dependent:
        return EngineTerminalVerdict::affinely_dependent;
      case fixed512::TerminalGeometryDecision::boundary_reduced:
        return EngineTerminalVerdict::boundary_reduced;
      case fixed512::TerminalGeometryDecision::exterior_circumcenter:
        return EngineTerminalVerdict::exterior_circumcenter;
      case fixed512::TerminalGeometryDecision::minimal:
        return EngineTerminalVerdict::minimal;
      default:
        return EngineTerminalVerdict::overflow;
    }
  }
  const fixed1024::TerminalGeometryDecision decision =
      fixed1024::terminal_support_geometry(boxes, support_size);
  switch (decision) {
    case fixed1024::TerminalGeometryDecision::affinely_dependent:
      return EngineTerminalVerdict::affinely_dependent;
    case fixed1024::TerminalGeometryDecision::boundary_reduced:
      return EngineTerminalVerdict::boundary_reduced;
    case fixed1024::TerminalGeometryDecision::exterior_circumcenter:
      return EngineTerminalVerdict::exterior_circumcenter;
    case fixed1024::TerminalGeometryDecision::minimal:
      return EngineTerminalVerdict::minimal;
    default:
      return EngineTerminalVerdict::overflow;
  }
}

// ---------------------------------------------------------------------------
// Resident slot checkpoint.  Never copied to the host; the layout is
// launcher-private and deliberately excluded from the audited arena bytes.
// ---------------------------------------------------------------------------

struct alignas(16) EngineSlotCheckpoint {
  // Tile-scoped identity and committed cumulatives.
  std::uint64_t root_entry_digest{};
  EngineU128 universe_mass{};
  EngineU128 well_cumulative{};
  EngineU128 rank_cumulative{};
  EngineU128 terminal_cumulative{};
  std::uint64_t stack_high_water{};
  std::uint64_t stack_top{};
  std::uint64_t tile_epoch{};
  std::uint64_t chunk_sequence{};
  std::uint8_t complete{};
  std::uint8_t pending_terminal{};
  // Probe cursor (plan rebuilt deterministically on demand).
  std::uint8_t probe_active{};
  std::uint8_t probe_fallback_active{};
  std::uint64_t probe_threshold{};
  std::uint64_t probe_next_cell_index{};
  std::uint64_t probe_next_fallback_index{};
  std::uint64_t probe_certified_point_count{};
  std::uint64_t probe_pending_receipt_count{};
  EngineReceipt probe_pending_receipts[engine_maximum_probe_threshold]{};
  // Wide-decision suspension awaiting the host rational drain.
  std::uint8_t wide_pending{};
  std::uint8_t wide_verdict_valid{};
  std::uint8_t wide_verdict{};
  std::uint8_t wide_kind{};
  std::uint8_t wide_probe_was_fallback{};
  // Set at chunk start: the slot completed in an EARLIER chunk and must
  // report zero chunk activity, including a zero stack high water.
  std::uint8_t complete_before_chunk{};
  std::uint64_t wide_query_node_index{};
  // Chunk-scoped accumulators (reset when the chunk sequence advances).
  EngineU128 chunk_well_delta{};
  EngineU128 chunk_rank_delta{};
  EngineU128 chunk_terminal_delta{};
  std::uint64_t chunk_expansion_count{};
  std::uint64_t chunk_gate_evaluation_count{};
  std::uint64_t chunk_probe_root_decision_count{};
  std::uint64_t chunk_probe_fallback_decision_count{};
  std::uint64_t chunk_deferred_int512_count{};
  std::uint64_t chunk_deferred_int1024_count{};
  std::uint64_t chunk_rational_drain_count{};
  std::uint64_t chunk_prune_used{};
  std::uint64_t chunk_terminal_used{};
  std::uint64_t chunk_receipt_used{};
  std::uint64_t budget_remaining{};
  std::uint8_t run_state{};
  std::uint8_t yield_reason{};
  std::uint8_t stop_reason{};
  std::uint8_t failure_code{};
};

static_assert(std::is_standard_layout_v<EngineSlotCheckpoint>);
static_assert(std::is_trivially_copyable_v<EngineSlotCheckpoint>);

// Per-slot request parameters and slot-local segment views.
struct EngineSlotContext {
  EngineGeometryView geometry{};
  std::uint8_t terminal_policy{};
  std::uint64_t maximum_relevant_closed_rank{};
  std::uint64_t gate_evaluations_per_slot_per_chunk{};
  std::uint64_t products_per_slot{};
  std::uint64_t prune_records_per_slot{};
  std::uint64_t terminal_records_per_slot{};
  std::uint64_t probe_receipts_per_slot{};
  EngineProduct* stack{};
  EnginePrune* prunes{};
  EngineTerminal* terminals{};
  EngineReceipt* receipts{};
  std::uint64_t slot_index{};
};

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline bool
engine_carrier_policy(const EngineSlotContext& context) noexcept {
  return context.terminal_policy ==
      static_cast<std::uint8_t>(
          HigherSupportDeviceTiledFrontierTerminalPolicy::
              strict_interior_carrier_q3);
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline std::uint64_t
engine_required_witness_count(
    const EngineSlotContext& context,
    std::size_t support_size) noexcept {
  if (engine_carrier_policy(context)) {
    return 1U;
  }
  return support_size > context.maximum_relevant_closed_rank
      ? 0U
      : context.maximum_relevant_closed_rank - support_size + 1U;
}

// ---------------------------------------------------------------------------
// Deterministic probe plan (rebuilt on demand; a pure function of the
// geometry, the entry and the threshold, identical to the fake plan).
// ---------------------------------------------------------------------------

struct EngineProbePlan {
  std::uint64_t cell_root_node[engine_maximum_probe_candidate_count]{};
  std::uint64_t fallback_cell[engine_maximum_probe_candidate_count]{};
  std::uint64_t fallback_leaf_node[engine_maximum_probe_candidate_count]{};
  std::uint64_t cell_count{};
  std::uint64_t fallback_count{};
};

// Highest ancestor of the external leaf position whose mass is at most the
// threshold and whose Morton range avoids the support domain, by descent
// from the root exactly as in the CPU stream.  Returns false on the
// invariant violations the fake reports as logic errors.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline bool
engine_bounded_external_node_index(
    const EngineGeometryView& geometry,
    std::uint64_t leaf_position,
    const EngineProduct& entry,
    std::uint64_t maximum_point_count,
    std::uint64_t& out) noexcept {
  if (leaf_position >= geometry.point_count ||
      maximum_point_count == 0U ||
      engine_leaf_position_in_support_domain(leaf_position, entry)) {
    return false;
  }
  std::uint64_t current_index = engine_root_index(geometry);
  while (true) {
    const EngineNode& current = geometry.nodes[current_index];
    if (current.leaf_end - current.leaf_begin <= maximum_point_count &&
        !engine_node_range_intersects_support_domain(current, entry)) {
      out = current_index;
      return true;
    }
    if (engine_node_is_leaf(current)) {
      return false;
    }
    const EngineNode& left = geometry.nodes[current.left_child];
    const EngineNode& right = geometry.nodes[current.right_child];
    if (left.leaf_begin <= leaf_position &&
        leaf_position < left.leaf_end) {
      current_index = current.left_child;
    } else if (
        right.leaf_begin <= leaf_position &&
        leaf_position < right.leaf_end) {
      current_index = current.right_child;
    } else {
      return false;
    }
  }
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline bool
engine_leaf_node_index_below(
    const EngineGeometryView& geometry,
    std::uint64_t ancestor_node_index,
    std::uint64_t leaf_position,
    std::uint64_t& out) noexcept {
  std::uint64_t current_index = ancestor_node_index;
  while (true) {
    const EngineNode& current = geometry.nodes[current_index];
    if (!(current.leaf_begin <= leaf_position &&
          leaf_position < current.leaf_end)) {
      return false;
    }
    if (engine_node_is_leaf(current)) {
      out = current_index;
      return true;
    }
    const EngineNode& left = geometry.nodes[current.left_child];
    const EngineNode& right = geometry.nodes[current.right_child];
    if (left.leaf_begin <= leaf_position &&
        leaf_position < left.leaf_end) {
      current_index = current.left_child;
    } else if (
        right.leaf_begin <= leaf_position &&
        leaf_position < right.leaf_end) {
      current_index = current.right_child;
    } else {
      return false;
    }
  }
}

// Slot-local probe plan under the frozen substitute v1 seed: seed =
// leaf_begin of the first group, halo seed +- d, then group boundaries, with
// linear first-appearance deduplication of positions and cells, per-position
// fallback leaves and the audited antichain contract, verbatim from the
// fake.  Returns false on any contract violation.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline bool
engine_build_probe_plan(
    const EngineGeometryView& geometry,
    const EngineProduct& entry,
    std::uint64_t threshold,
    EngineProbePlan& plan) noexcept {
  plan = EngineProbePlan{};
  if (threshold == 0U || threshold > engine_maximum_probe_threshold) {
    return false;
  }
  const std::uint64_t leaf_count = geometry.point_count;
  std::uint64_t positions[engine_maximum_probe_candidate_count]{};
  std::uint64_t position_count = 0U;
  const auto append_position = [&](std::uint64_t position) noexcept {
    if (position >= leaf_count ||
        engine_leaf_position_in_support_domain(position, entry)) {
      return true;
    }
    for (std::uint64_t index = 0U; index < position_count; ++index) {
      if (positions[index] == position) {
        return true;
      }
    }
    if (position_count >= engine_maximum_probe_candidate_count) {
      return false;
    }
    positions[position_count] = position;
    ++position_count;
    return true;
  };

  const std::uint64_t seed = entry.group_leaf_begin[0];
  if (!append_position(seed)) {
    return false;
  }
  for (std::uint64_t distance = 1U; distance <= threshold; ++distance) {
    if (seed >= distance && !append_position(seed - distance)) {
      return false;
    }
    if (distance < leaf_count && seed < leaf_count - distance &&
        !append_position(seed + distance)) {
      return false;
    }
  }
  for (std::uint64_t distance = 1U; distance <= threshold; ++distance) {
    for (std::size_t group_index = 0U;
         group_index < entry.group_count;
         ++group_index) {
      const std::uint64_t group_begin =
          entry.group_leaf_begin[group_index];
      const std::uint64_t group_end = entry.group_leaf_end[group_index];
      if (group_begin >= distance &&
          !append_position(group_begin - distance)) {
        return false;
      }
      if (distance <= leaf_count && group_end <= leaf_count - distance &&
          !append_position(group_end + distance - 1U)) {
        return false;
      }
    }
  }

  for (std::uint64_t index = 0U; index < position_count; ++index) {
    const std::uint64_t position = positions[index];
    std::uint64_t candidate_node_index = 0U;
    if (!engine_bounded_external_node_index(
            geometry, position, entry, threshold, candidate_node_index)) {
      return false;
    }
    std::uint64_t cell_index = plan.cell_count;
    for (std::uint64_t existing = 0U; existing < plan.cell_count;
         ++existing) {
      if (plan.cell_root_node[existing] == candidate_node_index) {
        cell_index = existing;
        break;
      }
    }
    if (cell_index == plan.cell_count) {
      if (plan.cell_count >= engine_maximum_probe_candidate_count) {
        return false;
      }
      plan.cell_root_node[plan.cell_count] = candidate_node_index;
      ++plan.cell_count;
    }
    std::uint64_t fallback_leaf = 0U;
    if (!engine_leaf_node_index_below(
            geometry, candidate_node_index, position, fallback_leaf)) {
      return false;
    }
    if (plan.fallback_count >= engine_maximum_probe_candidate_count) {
      return false;
    }
    plan.fallback_cell[plan.fallback_count] = cell_index;
    plan.fallback_leaf_node[plan.fallback_count] = fallback_leaf;
    ++plan.fallback_count;
  }

  // Audited plan contract: bounded external antichain cells, nonempty
  // fallback lists of true leaves inside their cells, and the fixed
  // evaluation bound.
  std::uint64_t evaluation_count = plan.cell_count + plan.fallback_count;
  if (evaluation_count >
      hierarchy::higher_support_local_rank_probe_maximum_evaluation_count) {
    return false;
  }
  for (std::uint64_t left = 0U; left < plan.cell_count; ++left) {
    const EngineNode& left_node =
        geometry.nodes[plan.cell_root_node[left]];
    if (left_node.leaf_end - left_node.leaf_begin > threshold ||
        engine_node_range_intersects_support_domain(left_node, entry)) {
      return false;
    }
    bool has_fallback = false;
    for (std::uint64_t entry_index = 0U;
         entry_index < plan.fallback_count;
         ++entry_index) {
      if (plan.fallback_cell[entry_index] != left) {
        continue;
      }
      has_fallback = true;
      const EngineNode& fallback_leaf =
          geometry.nodes[plan.fallback_leaf_node[entry_index]];
      if (!engine_node_is_leaf(fallback_leaf) ||
          fallback_leaf.leaf_begin < left_node.leaf_begin ||
          fallback_leaf.leaf_end > left_node.leaf_end) {
        return false;
      }
    }
    if (!has_fallback) {
      return false;
    }
    for (std::uint64_t right = left + 1U; right < plan.cell_count;
         ++right) {
      const EngineNode& right_node =
          geometry.nodes[plan.cell_root_node[right]];
      if (left_node.leaf_begin < right_node.leaf_end &&
          right_node.leaf_begin < left_node.leaf_end) {
        return false;
      }
    }
  }
  return true;
}

// Per-cell fallback enumeration helper: returns the ordinal-th fallback leaf
// of the given cell in position order, mirroring the fake's per-cell vector.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline bool
engine_plan_fallback_leaf(
    const EngineProbePlan& plan,
    std::uint64_t cell_index,
    std::uint64_t ordinal,
    std::uint64_t& leaf_node_out) noexcept {
  std::uint64_t seen = 0U;
  for (std::uint64_t index = 0U; index < plan.fallback_count; ++index) {
    if (plan.fallback_cell[index] != cell_index) {
      continue;
    }
    if (seen == ordinal) {
      leaf_node_out = plan.fallback_leaf_node[index];
      return true;
    }
    ++seen;
  }
  return false;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline std::uint64_t
engine_plan_fallback_count_of_cell(
    const EngineProbePlan& plan,
    std::uint64_t cell_index) noexcept {
  std::uint64_t count = 0U;
  for (std::uint64_t index = 0U; index < plan.fallback_count; ++index) {
    if (plan.fallback_cell[index] == cell_index) {
      ++count;
    }
  }
  return count;
}

// ---------------------------------------------------------------------------
// Canonical expansion (mirror of make_mirror_entry + expand_stack_top).
// ---------------------------------------------------------------------------

// Canonical child construction: groups sorted by (leaf_begin, node_index),
// multiplicities validated against their Morton ranges.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline bool
engine_make_entry(
    const EngineGeometryView& geometry,
    std::uint8_t support_size,
    const std::uint64_t group_nodes[4],
    const std::uint64_t group_multiplicities[4],
    std::size_t group_count,
    EngineProduct& out) noexcept {
  if (group_count == 0U || group_count > support_size || group_count > 4U) {
    return false;
  }
  std::uint64_t sorted_nodes[4]{};
  std::uint64_t sorted_multiplicities[4]{};
  for (std::size_t index = 0U; index < group_count; ++index) {
    sorted_nodes[index] = group_nodes[index];
    sorted_multiplicities[index] = group_multiplicities[index];
  }
  for (std::size_t sorted = 1U; sorted < group_count; ++sorted) {
    const std::uint64_t node_value = sorted_nodes[sorted];
    const std::uint64_t multiplicity_value = sorted_multiplicities[sorted];
    const std::uint64_t begin_value =
        geometry.nodes[node_value].leaf_begin;
    std::size_t position = sorted;
    while (position > 0U) {
      const std::uint64_t previous_node = sorted_nodes[position - 1U];
      const std::uint64_t previous_begin =
          geometry.nodes[previous_node].leaf_begin;
      if (previous_begin < begin_value ||
          (previous_begin == begin_value && previous_node < node_value)) {
        break;
      }
      sorted_nodes[position] = previous_node;
      sorted_multiplicities[position] =
          sorted_multiplicities[position - 1U];
      --position;
    }
    sorted_nodes[position] = node_value;
    sorted_multiplicities[position] = multiplicity_value;
  }
  out = EngineProduct{};
  out.support_size = support_size;
  out.group_count = static_cast<std::uint8_t>(group_count);
  for (std::size_t index = 0U; index < group_count; ++index) {
    const std::uint64_t node_index = sorted_nodes[index];
    const std::uint64_t multiplicity = sorted_multiplicities[index];
    if (node_index >= geometry.node_count) {
      return false;
    }
    const EngineNode& node = geometry.nodes[node_index];
    if (multiplicity == 0U || multiplicity > 4U ||
        node.leaf_end - node.leaf_begin < multiplicity) {
      return false;
    }
    out.group_node_index[index] = node_index;
    out.group_leaf_begin[index] = node.leaf_begin;
    out.group_leaf_end[index] = node.leaf_end;
    out.group_multiplicity[index] =
        static_cast<std::uint8_t>(multiplicity);
  }
  return true;
}

// ---------------------------------------------------------------------------
// Decision ladder with escalation counting and rational suspension.
// ---------------------------------------------------------------------------

// A ladder outcome: decided (verdict meaningful), or the int1024 evaluation
// itself overflowed and the caller must suspend for the host rational drain.
struct EngineLadderGate {
  EngineGateVerdict verdict{EngineGateVerdict::overflow};
  bool needs_rational{false};
};

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline EngineLadderGate
engine_ladder_no_well_centered(
    EngineSlotCheckpoint& checkpoint,
    const EngineBox boxes[4],
    std::size_t support_size) noexcept {
  EngineLadderGate outcome;
  outcome.verdict = engine_no_well_centered_256(boxes, support_size);
  if (outcome.verdict != EngineGateVerdict::overflow) {
    return outcome;
  }
  ++checkpoint.chunk_deferred_int512_count;
  outcome.verdict = engine_no_well_centered_512(boxes, support_size);
  if (outcome.verdict != EngineGateVerdict::overflow) {
    return outcome;
  }
  ++checkpoint.chunk_deferred_int1024_count;
  outcome.verdict = engine_no_well_centered_1024(boxes, support_size);
  if (outcome.verdict != EngineGateVerdict::overflow) {
    return outcome;
  }
  outcome.needs_rational = true;
  return outcome;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline EngineLadderGate
engine_ladder_all_well_centered(
    EngineSlotCheckpoint& checkpoint,
    const EngineBox boxes[4],
    std::size_t support_size) noexcept {
  EngineLadderGate outcome;
  outcome.verdict = engine_all_well_centered_256(boxes, support_size);
  if (outcome.verdict != EngineGateVerdict::overflow) {
    return outcome;
  }
  ++checkpoint.chunk_deferred_int512_count;
  outcome.verdict = engine_all_well_centered_512(boxes, support_size);
  if (outcome.verdict != EngineGateVerdict::overflow) {
    return outcome;
  }
  ++checkpoint.chunk_deferred_int1024_count;
  outcome.verdict = engine_all_well_centered_1024(boxes, support_size);
  if (outcome.verdict != EngineGateVerdict::overflow) {
    return outcome;
  }
  outcome.needs_rational = true;
  return outcome;
}

struct EngineLadderQuery {
  EngineQueryVerdict verdict{EngineQueryVerdict::overflow};
  bool needs_rational{false};
};

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline EngineLadderQuery
engine_ladder_query_cell(
    EngineSlotCheckpoint& checkpoint,
    const EngineBox boxes[4],
    std::size_t support_size,
    const EngineBox& query_box) noexcept {
  EngineLadderQuery outcome;
  outcome.verdict =
      engine_query_cell_256(boxes, support_size, query_box);
  if (outcome.verdict != EngineQueryVerdict::overflow) {
    return outcome;
  }
  ++checkpoint.chunk_deferred_int512_count;
  outcome.verdict =
      engine_query_cell_512(boxes, support_size, query_box);
  if (outcome.verdict != EngineQueryVerdict::overflow) {
    return outcome;
  }
  ++checkpoint.chunk_deferred_int1024_count;
  outcome.verdict =
      engine_query_cell_1024(boxes, support_size, query_box);
  if (outcome.verdict != EngineQueryVerdict::overflow) {
    return outcome;
  }
  outcome.needs_rational = true;
  return outcome;
}

struct EngineLadderTerminal {
  EngineTerminalVerdict verdict{EngineTerminalVerdict::overflow};
  bool rational_backend{false};
  bool needs_rational{false};
};

[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline EngineLadderTerminal
engine_ladder_terminal_geometry(
    EngineSlotCheckpoint& checkpoint,
    const EngineBox boxes[4],
    std::size_t support_size) noexcept {
  EngineLadderTerminal outcome;
  outcome.verdict = engine_terminal_geometry_width(0, boxes, support_size);
  if (outcome.verdict != EngineTerminalVerdict::overflow) {
    return outcome;
  }
  ++checkpoint.chunk_deferred_int512_count;
  outcome.verdict = engine_terminal_geometry_width(1, boxes, support_size);
  if (outcome.verdict != EngineTerminalVerdict::overflow) {
    return outcome;
  }
  ++checkpoint.chunk_deferred_int1024_count;
  outcome.verdict = engine_terminal_geometry_width(2, boxes, support_size);
  if (outcome.verdict != EngineTerminalVerdict::overflow) {
    return outcome;
  }
  outcome.needs_rational = true;
  return outcome;
}

// ---------------------------------------------------------------------------
// Slot executor: an exact transcription of FakeSlotExecutor over the
// resident segments.  Fatal outcomes set run_state/stop/failure and leave
// the chunk rollback to the driver, exactly like the fake launcher.
// ---------------------------------------------------------------------------

MORSEHGP3D_PHASE15_HS_ENGINE_HD inline void engine_set_fatal(
    EngineSlotCheckpoint& checkpoint,
    Phase15HigherSupportDeviceTiledFailureCode failure) noexcept {
  checkpoint.run_state = static_cast<std::uint8_t>(EngineRunState::fatal);
  checkpoint.stop_reason = static_cast<std::uint8_t>(
      Phase15HigherSupportDeviceTiledStopReason::fatal_failure);
  checkpoint.failure_code = static_cast<std::uint8_t>(failure);
}

MORSEHGP3D_PHASE15_HS_ENGINE_HD inline void engine_yield_chunk(
    EngineSlotCheckpoint& checkpoint,
    Phase15HigherSupportDeviceTiledYieldReason reason) noexcept {
  checkpoint.run_state =
      static_cast<std::uint8_t>(EngineRunState::chunk_ready);
  checkpoint.yield_reason = static_cast<std::uint8_t>(reason);
}

MORSEHGP3D_PHASE15_HS_ENGINE_HD inline void engine_suspend_wide(
    EngineSlotCheckpoint& checkpoint,
    EngineWideKind kind,
    std::uint64_t query_node_index,
    bool probe_was_fallback) noexcept {
  checkpoint.wide_pending = 1U;
  checkpoint.wide_verdict_valid = 0U;
  checkpoint.wide_verdict = 0U;
  checkpoint.wide_kind = static_cast<std::uint8_t>(kind);
  checkpoint.wide_query_node_index = query_node_index;
  checkpoint.wide_probe_was_fallback = probe_was_fallback ? 1U : 0U;
  ++checkpoint.chunk_rational_drain_count;
  checkpoint.run_state = static_cast<std::uint8_t>(EngineRunState::paused);
}

MORSEHGP3D_PHASE15_HS_ENGINE_HD inline void engine_clear_wide(
    EngineSlotCheckpoint& checkpoint) noexcept {
  checkpoint.wide_pending = 0U;
  checkpoint.wide_verdict_valid = 0U;
  checkpoint.wide_verdict = 0U;
  checkpoint.wide_kind =
      static_cast<std::uint8_t>(EngineWideKind::none);
  checkpoint.wide_query_node_index = 0U;
  checkpoint.wide_probe_was_fallback = 0U;
}

// Consumes one gate evaluation if the quantum allows it.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline bool
engine_consume_gate(EngineSlotCheckpoint& checkpoint) noexcept {
  if (checkpoint.budget_remaining == 0U) {
    return false;
  }
  --checkpoint.budget_remaining;
  ++checkpoint.chunk_gate_evaluation_count;
  return true;
}

MORSEHGP3D_PHASE15_HS_ENGINE_HD inline void engine_fill_prune_identity(
    const EngineSlotContext& context,
    EnginePrune& record,
    const EngineProduct& entry) noexcept {
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    record.group_node_index[index] = entry.group_node_index[index];
    record.group_leaf_begin[index] = entry.group_leaf_begin[index];
    record.group_leaf_end[index] = entry.group_leaf_end[index];
    record.group_multiplicity[index] = entry.group_multiplicity[index];
  }
  record.support_size = entry.support_size;
  record.group_count = entry.group_count;
  record.terminal_policy = context.terminal_policy;
  record.flags = engine_carrier_policy(context)
      ? phase15_higher_support_device_tiled_flag_carrier_policy
      : std::uint8_t{0U};
}

MORSEHGP3D_PHASE15_HS_ENGINE_HD inline void engine_reset_probe(
    EngineSlotCheckpoint& checkpoint) noexcept {
  checkpoint.probe_active = 0U;
  checkpoint.probe_fallback_active = 0U;
  checkpoint.probe_threshold = 0U;
  checkpoint.probe_next_cell_index = 0U;
  checkpoint.probe_next_fallback_index = 0U;
  checkpoint.probe_certified_point_count = 0U;
  checkpoint.probe_pending_receipt_count = 0U;
}

MORSEHGP3D_PHASE15_HS_ENGINE_HD inline void engine_emit_well_prune(
    const EngineSlotContext& context,
    EngineSlotCheckpoint& checkpoint,
    const EngineProduct& entry) noexcept {
  if (checkpoint.chunk_prune_used == context.prune_records_per_slot) {
    engine_yield_chunk(
        checkpoint,
        Phase15HigherSupportDeviceTiledYieldReason::prune_segment_full);
    return;
  }
  EngineU128 mass{};
  if (!engine_entry_mass(entry, mass)) {
    engine_set_fatal(
        checkpoint,
        Phase15HigherSupportDeviceTiledFailureCode::arithmetic_overflow);
    return;
  }
  EnginePrune record{};
  engine_fill_prune_identity(context, record, entry);
  record.pruned_mass_lo = mass.lo;
  record.pruned_mass_hi = mass.hi;
  record.prune_kind = static_cast<std::uint64_t>(
      Phase15HigherSupportDeviceTiledPruneKind::no_well_centered_support);
  record.receipt_count = 0U;
  record.receipt_segment_begin = checkpoint.chunk_receipt_used;
  context.prunes[checkpoint.chunk_prune_used] = record;
  ++checkpoint.chunk_prune_used;
  if (!phase15_higher_support_device_tiled_u128_add(
          checkpoint.chunk_well_delta, mass)) {
    engine_set_fatal(
        checkpoint,
        Phase15HigherSupportDeviceTiledFailureCode::arithmetic_overflow);
    return;
  }
  --checkpoint.stack_top;
  checkpoint.pending_terminal = 0U;
}

MORSEHGP3D_PHASE15_HS_ENGINE_HD inline void engine_emit_rank_prune(
    const EngineSlotContext& context,
    EngineSlotCheckpoint& checkpoint,
    const EngineProduct& entry,
    Phase15HigherSupportDeviceTiledPruneKind kind,
    const EngineReceipt* receipts,
    std::uint64_t receipt_count) noexcept {
  if (checkpoint.chunk_prune_used == context.prune_records_per_slot ||
      checkpoint.chunk_receipt_used + receipt_count >
          context.probe_receipts_per_slot) {
    engine_reset_probe(checkpoint);
    engine_yield_chunk(
        checkpoint,
        Phase15HigherSupportDeviceTiledYieldReason::prune_segment_full);
    return;
  }
  EngineU128 mass{};
  if (!engine_entry_mass(entry, mass)) {
    engine_set_fatal(
        checkpoint,
        Phase15HigherSupportDeviceTiledFailureCode::arithmetic_overflow);
    return;
  }
  EnginePrune record{};
  engine_fill_prune_identity(context, record, entry);
  record.pruned_mass_lo = mass.lo;
  record.pruned_mass_hi = mass.hi;
  record.prune_kind = static_cast<std::uint64_t>(kind);
  record.receipt_count = receipt_count;
  record.receipt_segment_begin = checkpoint.chunk_receipt_used;
  for (std::uint64_t index = 0U; index < receipt_count; ++index) {
    context.receipts[checkpoint.chunk_receipt_used + index] =
        receipts[index];
  }
  context.prunes[checkpoint.chunk_prune_used] = record;
  ++checkpoint.chunk_prune_used;
  checkpoint.chunk_receipt_used += receipt_count;
  if (!phase15_higher_support_device_tiled_u128_add(
          checkpoint.chunk_rank_delta, mass)) {
    engine_set_fatal(
        checkpoint,
        Phase15HigherSupportDeviceTiledFailureCode::arithmetic_overflow);
    return;
  }
  --checkpoint.stack_top;
  engine_reset_probe(checkpoint);
  checkpoint.pending_terminal = 0U;
}

// Exact expansion: the non-leaf group of strictly largest extent (first
// strict winner) splits into (left, a) and (right, m - a) children whose
// unsigned 128-bit masses must partition the parent mass exactly.
MORSEHGP3D_PHASE15_HS_ENGINE_HD inline void engine_expand_stack_top(
    const EngineSlotContext& context,
    EngineSlotCheckpoint& checkpoint) noexcept {
  const EngineProduct entry = context.stack[checkpoint.stack_top - 1U];
  std::size_t split_group_index = entry.group_count;
  std::uint64_t largest_range = 0U;
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    if (!engine_authenticate_group(context.geometry, entry, index)) {
      engine_set_fatal(
          checkpoint,
          Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
      return;
    }
    const EngineNode& current =
        context.geometry.nodes[entry.group_node_index[index]];
    const std::uint64_t range = current.leaf_end - current.leaf_begin;
    if (!engine_node_is_leaf(current) && range > largest_range) {
      split_group_index = index;
      largest_range = range;
    }
  }
  if (split_group_index == entry.group_count) {
    engine_set_fatal(
        checkpoint,
        Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
    return;
  }
  const EngineNode& parent =
      context.geometry.nodes[entry.group_node_index[split_group_index]];
  const EngineNode& left = context.geometry.nodes[parent.left_child];
  const EngineNode& right = context.geometry.nodes[parent.right_child];
  const std::uint64_t multiplicity =
      entry.group_multiplicity[split_group_index];
  const std::uint64_t left_size = left.leaf_end - left.leaf_begin;
  const std::uint64_t right_size = right.leaf_end - right.leaf_begin;
  const std::uint64_t minimum_left =
      multiplicity > right_size ? multiplicity - right_size : 0U;
  const std::uint64_t maximum_left =
      multiplicity < left_size ? multiplicity : left_size;
  if (minimum_left > maximum_left) {
    engine_set_fatal(
        checkpoint,
        Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
    return;
  }
  EngineU128 parent_mass{};
  if (!engine_entry_mass(entry, parent_mass)) {
    engine_set_fatal(
        checkpoint,
        Phase15HigherSupportDeviceTiledFailureCode::arithmetic_overflow);
    return;
  }
  EngineProduct children[5]{};
  std::size_t child_count = 0U;
  EngineU128 child_mass_sum{};
  for (std::uint64_t left_multiplicity = minimum_left;
       left_multiplicity <= maximum_left;
       ++left_multiplicity) {
    std::uint64_t group_nodes[4]{};
    std::uint64_t group_multiplicities[4]{};
    std::size_t group_count = 0U;
    for (std::size_t index = 0U; index < entry.group_count; ++index) {
      if (index != split_group_index) {
        group_nodes[group_count] = entry.group_node_index[index];
        group_multiplicities[group_count] =
            entry.group_multiplicity[index];
        ++group_count;
      }
    }
    if (left_multiplicity != 0U) {
      if (group_count >= 4U) {
        engine_set_fatal(
            checkpoint,
            Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
        return;
      }
      group_nodes[group_count] = parent.left_child;
      group_multiplicities[group_count] = left_multiplicity;
      ++group_count;
    }
    const std::uint64_t right_multiplicity =
        multiplicity - left_multiplicity;
    if (right_multiplicity != 0U) {
      if (group_count >= 4U) {
        engine_set_fatal(
            checkpoint,
            Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
        return;
      }
      group_nodes[group_count] = parent.right_child;
      group_multiplicities[group_count] = right_multiplicity;
      ++group_count;
    }
    if (child_count >= 5U ||
        !engine_make_entry(
            context.geometry,
            entry.support_size,
            group_nodes,
            group_multiplicities,
            group_count,
            children[child_count])) {
      engine_set_fatal(
          checkpoint,
          Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
      return;
    }
    EngineU128 child_mass{};
    if (!engine_entry_mass(children[child_count], child_mass) ||
        !phase15_higher_support_device_tiled_u128_add(
            child_mass_sum, child_mass)) {
      engine_set_fatal(
          checkpoint,
          Phase15HigherSupportDeviceTiledFailureCode::arithmetic_overflow);
      return;
    }
    ++child_count;
  }
  if (!(child_mass_sum == parent_mass)) {
    engine_set_fatal(
        checkpoint,
        Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
    return;
  }
  const std::uint64_t resident_after =
      checkpoint.stack_top - 1U + child_count;
  if (resident_after > context.products_per_slot) {
    // The proved bound 16*d_T + 1 is contradicted: fatal, never a yield.
    engine_set_fatal(
        checkpoint,
        Phase15HigherSupportDeviceTiledFailureCode::stack_capacity_exceeded);
    return;
  }
  --checkpoint.stack_top;
  for (std::size_t index = child_count; index != 0U; --index) {
    context.stack[checkpoint.stack_top] = children[index - 1U];
    ++checkpoint.stack_top;
  }
  if (checkpoint.stack_top > checkpoint.stack_high_water) {
    checkpoint.stack_high_water = checkpoint.stack_top;
  }
  ++checkpoint.chunk_expansion_count;
}

// Applies one strict-inside receipt to the pending probe and reports
// whether the policy threshold is reached.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline bool
engine_probe_apply_inside(
    const EngineSlotContext& context,
    EngineSlotCheckpoint& checkpoint,
    std::uint64_t query_node_index) noexcept {
  const EngineNode& query = context.geometry.nodes[query_node_index];
  const std::uint64_t point_count = query.leaf_end - query.leaf_begin;
  checkpoint.probe_certified_point_count += point_count;
  if (checkpoint.probe_pending_receipt_count <
      engine_maximum_probe_threshold) {
    EngineReceipt& receipt = checkpoint.probe_pending_receipts
        [checkpoint.probe_pending_receipt_count];
    receipt.node_index = query_node_index;
    receipt.leaf_begin = query.leaf_begin;
    receipt.leaf_end = query.leaf_end;
    receipt.certified_point_count = point_count;
    ++checkpoint.probe_pending_receipt_count;
  }
  if (checkpoint.probe_certified_point_count >=
      checkpoint.probe_threshold) {
    checkpoint.probe_next_fallback_index = 0U;
    checkpoint.probe_fallback_active = 0U;
    return true;
  }
  return false;
}

enum class EngineProbeOutcome : std::uint8_t {
  paused = 0U,
  threshold_reached = 1U,
  fail_open = 2U,
  fatal = 3U,
  suspended = 4U,
};

// Continues the slot-local probe from its persistent cursor, consuming one
// gate per committed decision, in exact plan order.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline EngineProbeOutcome
engine_continue_probe(
    const EngineSlotContext& context,
    EngineSlotCheckpoint& checkpoint,
    const EngineProbePlan& plan,
    const EngineProduct& entry,
    const EngineBox boxes[4]) noexcept {
  // Resume from a suspended rational query decision: the cursor and the
  // decision counters already advanced when the gate was charged.
  if (checkpoint.wide_pending != 0U) {
    if (checkpoint.wide_verdict_valid == 0U) {
      checkpoint.run_state =
          static_cast<std::uint8_t>(EngineRunState::paused);
      return EngineProbeOutcome::paused;
    }
    const auto verdict =
        static_cast<EngineQueryVerdict>(checkpoint.wide_verdict);
    const std::uint64_t query_node_index =
        checkpoint.wide_query_node_index;
    const bool evaluating_fallback =
        checkpoint.wide_probe_was_fallback != 0U;
    engine_clear_wide(checkpoint);
    if (verdict == EngineQueryVerdict::strictly_inside) {
      if (engine_probe_apply_inside(
              context, checkpoint, query_node_index)) {
        return EngineProbeOutcome::threshold_reached;
      }
    } else if (verdict == EngineQueryVerdict::inconclusive) {
      if (!evaluating_fallback &&
          !engine_node_is_leaf(
              context.geometry.nodes[query_node_index])) {
        checkpoint.probe_next_fallback_index = 0U;
        checkpoint.probe_fallback_active = 1U;
      }
    } else if (verdict != EngineQueryVerdict::outside_or_boundary) {
      return EngineProbeOutcome::fatal;
    }
  }
  while (checkpoint.probe_fallback_active != 0U ||
         checkpoint.probe_next_cell_index < plan.cell_count) {
    if (!engine_consume_gate(checkpoint)) {
      checkpoint.run_state =
          static_cast<std::uint8_t>(EngineRunState::paused);
      return EngineProbeOutcome::paused;
    }
    const bool evaluating_fallback =
        checkpoint.probe_fallback_active != 0U;
    const std::uint64_t cell_index = evaluating_fallback
        ? checkpoint.probe_next_cell_index - 1U
        : checkpoint.probe_next_cell_index;
    std::uint64_t query_node_index = plan.cell_root_node[cell_index];
    if (evaluating_fallback) {
      if (!engine_plan_fallback_leaf(
              plan,
              cell_index,
              checkpoint.probe_next_fallback_index,
              query_node_index)) {
        return EngineProbeOutcome::fatal;
      }
      ++checkpoint.probe_next_fallback_index;
      if (checkpoint.probe_next_fallback_index ==
          engine_plan_fallback_count_of_cell(plan, cell_index)) {
        checkpoint.probe_next_fallback_index = 0U;
        checkpoint.probe_fallback_active = 0U;
      }
      ++checkpoint.chunk_probe_fallback_decision_count;
    } else {
      ++checkpoint.probe_next_cell_index;
      ++checkpoint.chunk_probe_root_decision_count;
    }
    const EngineNode& query = context.geometry.nodes[query_node_index];
    if (query.leaf_end - query.leaf_begin > checkpoint.probe_threshold ||
        engine_node_range_intersects_support_domain(query, entry)) {
      return EngineProbeOutcome::fatal;
    }
    EngineBox query_box{};
    engine_node_box(context.geometry, query_node_index, query_box);
    const EngineLadderQuery decision = engine_ladder_query_cell(
        checkpoint, boxes, entry.support_size, query_box);
    if (decision.needs_rational) {
      engine_suspend_wide(
          checkpoint,
          EngineWideKind::query_cell,
          query_node_index,
          evaluating_fallback);
      return EngineProbeOutcome::suspended;
    }
    if (decision.verdict == EngineQueryVerdict::strictly_inside) {
      if (engine_probe_apply_inside(
              context, checkpoint, query_node_index)) {
        return EngineProbeOutcome::threshold_reached;
      }
      continue;
    }
    if (decision.verdict == EngineQueryVerdict::outside_or_boundary) {
      continue;
    }
    if (!evaluating_fallback && !engine_node_is_leaf(query)) {
      checkpoint.probe_next_fallback_index = 0U;
      checkpoint.probe_fallback_active = 1U;
    }
  }
  return EngineProbeOutcome::fail_open;
}

enum class EngineStepOutcome : std::uint8_t {
  advanced = 0U,
  settled = 1U,
  suspended = 2U,
};

// Terminal classification of the pending stack top (mirror of
// classify_pending_terminal, with the width ladder in place of the host
// primitive and the sealed backend byte mapping: any fitting bounded width
// reports the bounded backend, the rational drain reports rational).
[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline EngineStepOutcome
engine_classify_pending_terminal(
    const EngineSlotContext& context,
    EngineSlotCheckpoint& checkpoint) noexcept {
  if (checkpoint.stack_top == 0U) {
    engine_set_fatal(
        checkpoint,
        Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
    return EngineStepOutcome::settled;
  }
  EngineTerminalVerdict verdict = EngineTerminalVerdict::overflow;
  bool rational_backend = false;
  const EngineProduct entry = context.stack[checkpoint.stack_top - 1U];
  if (checkpoint.wide_pending != 0U) {
    if (checkpoint.wide_verdict_valid == 0U) {
      checkpoint.run_state =
          static_cast<std::uint8_t>(EngineRunState::paused);
      return EngineStepOutcome::suspended;
    }
    rational_backend =
        (checkpoint.wide_verdict &
         engine_terminal_verdict_rational_flag) != 0U;
    verdict = static_cast<EngineTerminalVerdict>(
        checkpoint.wide_verdict &
        static_cast<std::uint8_t>(0x0fU));
    engine_clear_wide(checkpoint);
  } else {
    if (checkpoint.chunk_terminal_used ==
        context.terminal_records_per_slot) {
      engine_yield_chunk(
          checkpoint,
          Phase15HigherSupportDeviceTiledYieldReason::
              terminal_segment_full);
      return EngineStepOutcome::settled;
    }
    if (!engine_consume_gate(checkpoint)) {
      checkpoint.run_state =
          static_cast<std::uint8_t>(EngineRunState::paused);
      return EngineStepOutcome::settled;
    }
    EngineBox boxes[4]{};
    if (!engine_support_boxes(context.geometry, entry, boxes)) {
      engine_set_fatal(
          checkpoint,
          Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
      return EngineStepOutcome::settled;
    }
    const EngineLadderTerminal ladder =
        engine_ladder_terminal_geometry(
            checkpoint, boxes, entry.support_size);
    if (ladder.needs_rational) {
      engine_suspend_wide(
          checkpoint, EngineWideKind::terminal_geometry, 0U, false);
      return EngineStepOutcome::suspended;
    }
    verdict = ladder.verdict;
    rational_backend = ladder.rational_backend;
  }
  if (verdict == EngineTerminalVerdict::overflow) {
    engine_set_fatal(
        checkpoint,
        Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
    return EngineStepOutcome::settled;
  }

  std::uint64_t point_ids[4]{};
  for (std::size_t index = 0U; index < entry.group_count; ++index) {
    if (!engine_authenticate_group(context.geometry, entry, index)) {
      engine_set_fatal(
          checkpoint,
          Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
      return EngineStepOutcome::settled;
    }
    point_ids[index] = context.geometry.morton_point_ids
        [context.geometry.nodes[entry.group_node_index[index]]
             .leaf_begin];
  }
  const std::size_t sorted_support_size = entry.support_size;
  for (std::size_t sorted = 1U; sorted < sorted_support_size; ++sorted) {
    const std::uint64_t value = point_ids[sorted];
    std::size_t position = sorted;
    while (position > 0U && point_ids[position - 1U] > value) {
      point_ids[position] = point_ids[position - 1U];
      --position;
    }
    point_ids[position] = value;
  }
  for (std::size_t index = 1U; index < sorted_support_size; ++index) {
    if (point_ids[index - 1U] == point_ids[index]) {
      engine_set_fatal(
          checkpoint,
          Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
      return EngineStepOutcome::settled;
    }
  }
  EngineTerminal record{};
  for (std::size_t index = 0U; index < 4U; ++index) {
    record.point_ids[index] = point_ids[index];
  }
  record.slot_index = context.slot_index;
  record.support_size = entry.support_size;
  record.terminal_geometry_decision =
      static_cast<std::uint8_t>(verdict);
  record.decision_backend = rational_backend ? std::uint8_t{1U}
                                             : std::uint8_t{0U};
  record.terminal_policy = context.terminal_policy;
  record.flags = engine_carrier_policy(context)
      ? phase15_higher_support_device_tiled_flag_carrier_policy
      : std::uint8_t{0U};
  context.terminals[checkpoint.chunk_terminal_used] = record;
  ++checkpoint.chunk_terminal_used;
  if (!phase15_higher_support_device_tiled_u128_add(
          checkpoint.chunk_terminal_delta, EngineU128{1U, 0U})) {
    engine_set_fatal(
        checkpoint,
        Phase15HigherSupportDeviceTiledFailureCode::arithmetic_overflow);
    return EngineStepOutcome::settled;
  }
  --checkpoint.stack_top;
  checkpoint.pending_terminal = 0U;
  return EngineStepOutcome::advanced;
}

// One analyze step of the DFS stack top (mirror of process_stack_top with
// the rational suspension sites of the two exact gates).
[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline EngineStepOutcome
engine_process_stack_top(
    const EngineSlotContext& context,
    EngineSlotCheckpoint& checkpoint) noexcept {
  const EngineProduct entry = context.stack[checkpoint.stack_top - 1U];
  EngineBox boxes[4]{};
  if (!engine_support_boxes(context.geometry, entry, boxes)) {
    engine_set_fatal(
        checkpoint,
        Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
    return EngineStepOutcome::settled;
  }

  bool have_no_well_verdict = false;
  bool no_well_certified = false;
  bool have_all_well_verdict = false;
  bool all_well_certified = false;
  if (checkpoint.wide_pending != 0U) {
    if (checkpoint.wide_verdict_valid == 0U) {
      checkpoint.run_state =
          static_cast<std::uint8_t>(EngineRunState::paused);
      return EngineStepOutcome::suspended;
    }
    const auto kind =
        static_cast<EngineWideKind>(checkpoint.wide_kind);
    const bool verdict = checkpoint.wide_verdict != 0U;
    engine_clear_wide(checkpoint);
    if (kind == EngineWideKind::gate_no_well_centered) {
      have_no_well_verdict = true;
      no_well_certified = verdict;
    } else if (kind == EngineWideKind::gate_all_well_centered) {
      have_no_well_verdict = true;
      no_well_certified = false;
      have_all_well_verdict = true;
      all_well_certified = verdict;
    } else {
      engine_set_fatal(
          checkpoint,
          Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
      return EngineStepOutcome::settled;
    }
  } else {
    // The analyze stage evaluates at most two exact gates atomically, so a
    // fresh quantum of at least two always makes progress.
    if (checkpoint.budget_remaining < 2U) {
      checkpoint.run_state =
          static_cast<std::uint8_t>(EngineRunState::paused);
      return EngineStepOutcome::settled;
    }
  }

  if (!have_no_well_verdict) {
    static_cast<void>(engine_consume_gate(checkpoint));
    const EngineLadderGate gate = engine_ladder_no_well_centered(
        checkpoint, boxes, entry.support_size);
    if (gate.needs_rational) {
      engine_suspend_wide(
          checkpoint, EngineWideKind::gate_no_well_centered, 0U, false);
      return EngineStepOutcome::suspended;
    }
    no_well_certified = gate.verdict == EngineGateVerdict::certified;
  }
  if (no_well_certified) {
    engine_emit_well_prune(context, checkpoint, entry);
    return EngineStepOutcome::advanced;
  }
  const std::uint64_t threshold =
      engine_required_witness_count(context, entry.support_size);
  if (threshold == 0U) {
    engine_emit_rank_prune(
        context,
        checkpoint,
        entry,
        Phase15HigherSupportDeviceTiledPruneKind::rank_window,
        nullptr,
        0U);
    return EngineStepOutcome::advanced;
  }
  if (!have_all_well_verdict) {
    static_cast<void>(engine_consume_gate(checkpoint));
    const EngineLadderGate gate = engine_ladder_all_well_centered(
        checkpoint, boxes, entry.support_size);
    if (gate.needs_rational) {
      engine_suspend_wide(
          checkpoint, EngineWideKind::gate_all_well_centered, 0U, false);
      return EngineStepOutcome::suspended;
    }
    all_well_certified = gate.verdict == EngineGateVerdict::certified;
  }
  if (all_well_certified &&
      engine_possible_external_witness_count(context.geometry, entry) >=
          threshold) {
    checkpoint.probe_active = 1U;
    checkpoint.probe_fallback_active = 0U;
    checkpoint.probe_threshold = threshold;
    checkpoint.probe_next_cell_index = 0U;
    checkpoint.probe_next_fallback_index = 0U;
    checkpoint.probe_certified_point_count = 0U;
    checkpoint.probe_pending_receipt_count = 0U;
    return EngineStepOutcome::advanced;
  }
  // Exact fail-open continuation: no support tuple is discarded.
  bool terminal = false;
  if (!engine_entry_terminal(context.geometry, entry, terminal)) {
    engine_set_fatal(
        checkpoint,
        Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
    return EngineStepOutcome::settled;
  }
  if (terminal) {
    checkpoint.pending_terminal = 1U;
    return EngineStepOutcome::advanced;
  }
  engine_expand_stack_top(context, checkpoint);
  return EngineStepOutcome::advanced;
}

// ---------------------------------------------------------------------------
// Subdivision runner: one bounded pass of one slot (mirror of
// FakeSlotExecutor::run_subdivision).  grant_fresh_quantum distinguishes a
// regular subdivision (fresh gate quantum) from a rational-drain relaunch
// that must continue on the remaining budget.
// ---------------------------------------------------------------------------

MORSEHGP3D_PHASE15_HS_ENGINE_HD inline void engine_run_subdivision(
    const EngineSlotContext& context,
    EngineSlotCheckpoint& checkpoint,
    bool grant_fresh_quantum) noexcept {
  if (grant_fresh_quantum) {
    checkpoint.budget_remaining =
        context.gate_evaluations_per_slot_per_chunk;
  }
  while (checkpoint.run_state ==
         static_cast<std::uint8_t>(EngineRunState::active)) {
    if (checkpoint.wide_pending != 0U &&
        checkpoint.wide_verdict_valid == 0U) {
      checkpoint.run_state =
          static_cast<std::uint8_t>(EngineRunState::paused);
      return;
    }
    if (checkpoint.probe_active != 0U) {
      if (checkpoint.stack_top == 0U) {
        engine_set_fatal(
            checkpoint,
            Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
        return;
      }
      const EngineProduct entry =
          context.stack[checkpoint.stack_top - 1U];
      EngineBox boxes[4]{};
      if (!engine_support_boxes(context.geometry, entry, boxes)) {
        engine_set_fatal(
            checkpoint,
            Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
        return;
      }
      EngineProbePlan plan{};
      if (!engine_build_probe_plan(
              context.geometry,
              entry,
              checkpoint.probe_threshold,
              plan)) {
        engine_set_fatal(
            checkpoint,
            Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
        return;
      }
      const EngineProbeOutcome outcome = engine_continue_probe(
          context, checkpoint, plan, entry, boxes);
      if (outcome == EngineProbeOutcome::paused ||
          outcome == EngineProbeOutcome::suspended) {
        return;
      }
      if (outcome == EngineProbeOutcome::fatal) {
        engine_set_fatal(
            checkpoint,
            Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
        return;
      }
      if (outcome == EngineProbeOutcome::threshold_reached) {
        const EngineReceipt* receipts =
            checkpoint.probe_pending_receipts;
        engine_emit_rank_prune(
            context,
            checkpoint,
            entry,
            engine_carrier_policy(context)
                ? Phase15HigherSupportDeviceTiledPruneKind::
                      strict_interior_carrier
                : Phase15HigherSupportDeviceTiledPruneKind::rank_window,
            receipts,
            checkpoint.probe_pending_receipt_count);
        continue;
      }
      // Fail-open: the probe certifies nothing and the product continues.
      engine_reset_probe(checkpoint);
      bool terminal = false;
      if (!engine_entry_terminal(context.geometry, entry, terminal)) {
        engine_set_fatal(
            checkpoint,
            Phase15HigherSupportDeviceTiledFailureCode::internal_invariant);
        return;
      }
      if (terminal) {
        checkpoint.pending_terminal = 1U;
        continue;
      }
      engine_expand_stack_top(context, checkpoint);
      continue;
    }
    if (checkpoint.pending_terminal != 0U) {
      static_cast<void>(
          engine_classify_pending_terminal(context, checkpoint));
      continue;
    }
    if (checkpoint.stack_top == 0U) {
      checkpoint.run_state =
          static_cast<std::uint8_t>(EngineRunState::complete);
      return;
    }
    static_cast<void>(engine_process_stack_top(context, checkpoint));
  }
}

// ---------------------------------------------------------------------------
// Chunk lifecycle helpers shared by the CUDA launcher and the host parity
// driver (mirrors of the fake's chunk workspace and rollback).
// ---------------------------------------------------------------------------

MORSEHGP3D_PHASE15_HS_ENGINE_HD inline void engine_begin_chunk(
    EngineSlotCheckpoint& checkpoint,
    std::uint64_t chunk_sequence) noexcept {
  checkpoint.complete_before_chunk = checkpoint.complete;
  checkpoint.run_state = checkpoint.complete != 0U
      ? static_cast<std::uint8_t>(EngineRunState::complete)
      : static_cast<std::uint8_t>(EngineRunState::active);
  checkpoint.chunk_well_delta = EngineU128{};
  checkpoint.chunk_rank_delta = EngineU128{};
  checkpoint.chunk_terminal_delta = EngineU128{};
  checkpoint.chunk_expansion_count = 0U;
  checkpoint.chunk_gate_evaluation_count = 0U;
  checkpoint.chunk_probe_root_decision_count = 0U;
  checkpoint.chunk_probe_fallback_decision_count = 0U;
  checkpoint.chunk_deferred_int512_count = 0U;
  checkpoint.chunk_deferred_int1024_count = 0U;
  checkpoint.chunk_rational_drain_count = 0U;
  checkpoint.chunk_prune_used = 0U;
  checkpoint.chunk_terminal_used = 0U;
  checkpoint.chunk_receipt_used = 0U;
  checkpoint.budget_remaining = 0U;
  checkpoint.yield_reason = static_cast<std::uint8_t>(
      Phase15HigherSupportDeviceTiledYieldReason::none);
  checkpoint.stop_reason = static_cast<std::uint8_t>(
      Phase15HigherSupportDeviceTiledStopReason::none);
  checkpoint.failure_code = static_cast<std::uint8_t>(
      Phase15HigherSupportDeviceTiledFailureCode::none);
  checkpoint.chunk_sequence = chunk_sequence;
}

// Rolls one slot's chunk back to its previous commit: zero deltas and
// counters, no published record, cumulative masses untouched.
MORSEHGP3D_PHASE15_HS_ENGINE_HD inline void engine_rollback_slot_chunk(
    const EngineSlotContext& context,
    EngineSlotCheckpoint& checkpoint) noexcept {
  for (std::uint64_t index = 0U; index < checkpoint.chunk_prune_used;
       ++index) {
    context.prunes[index] = EnginePrune{};
  }
  for (std::uint64_t index = 0U; index < checkpoint.chunk_terminal_used;
       ++index) {
    context.terminals[index] = EngineTerminal{};
  }
  for (std::uint64_t index = 0U; index < checkpoint.chunk_receipt_used;
       ++index) {
    context.receipts[index] = EngineReceipt{};
  }
  checkpoint.chunk_well_delta = EngineU128{};
  checkpoint.chunk_rank_delta = EngineU128{};
  checkpoint.chunk_terminal_delta = EngineU128{};
  checkpoint.chunk_expansion_count = 0U;
  checkpoint.chunk_gate_evaluation_count = 0U;
  checkpoint.chunk_probe_root_decision_count = 0U;
  checkpoint.chunk_probe_fallback_decision_count = 0U;
  checkpoint.chunk_deferred_int512_count = 0U;
  checkpoint.chunk_deferred_int1024_count = 0U;
  checkpoint.chunk_rational_drain_count = 0U;
  checkpoint.chunk_prune_used = 0U;
  checkpoint.chunk_terminal_used = 0U;
  checkpoint.chunk_receipt_used = 0U;
  checkpoint.yield_reason = static_cast<std::uint8_t>(
      Phase15HigherSupportDeviceTiledYieldReason::none);
  engine_reset_probe(checkpoint);
  checkpoint.pending_terminal = 0U;
  engine_clear_wide(checkpoint);
}

// Assembles the sealed 264-byte slot control from the checkpoint.  Returns
// false when a cumulative mass or the local partition leaves the certified
// unsigned 128-bit range; the caller must fail closed.  The checkpoint is
// not mutated: engine_commit_chunk applies the cumulative advance.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline bool
engine_write_slot_control(
    const EngineSlotCheckpoint& checkpoint,
    std::uint64_t tile_epoch,
    std::uint64_t chunk_sequence,
    Phase15HigherSupportDeviceTiledSlotControl& control) noexcept {
  EngineU128 well_new = checkpoint.well_cumulative;
  EngineU128 rank_new = checkpoint.rank_cumulative;
  EngineU128 terminal_new = checkpoint.terminal_cumulative;
  if (!phase15_higher_support_device_tiled_u128_add(
          well_new, checkpoint.chunk_well_delta) ||
      !phase15_higher_support_device_tiled_u128_add(
          rank_new, checkpoint.chunk_rank_delta) ||
      !phase15_higher_support_device_tiled_u128_add(
          terminal_new, checkpoint.chunk_terminal_delta)) {
    return false;
  }
  EngineU128 resolved = well_new;
  if (!phase15_higher_support_device_tiled_u128_add(resolved, rank_new) ||
      !phase15_higher_support_device_tiled_u128_add(
          resolved, terminal_new)) {
    return false;
  }
  EngineU128 unresolved = checkpoint.universe_mass;
  if (!phase15_higher_support_device_tiled_u128_subtract(
          unresolved, resolved)) {
    return false;
  }
  control = Phase15HigherSupportDeviceTiledSlotControl{};
  control.root_entry_digest = checkpoint.root_entry_digest;
  control.well_prune_mass_delta_lo = checkpoint.chunk_well_delta.lo;
  control.well_prune_mass_delta_hi = checkpoint.chunk_well_delta.hi;
  control.rank_prune_mass_delta_lo = checkpoint.chunk_rank_delta.lo;
  control.rank_prune_mass_delta_hi = checkpoint.chunk_rank_delta.hi;
  control.terminal_mass_delta_lo = checkpoint.chunk_terminal_delta.lo;
  control.terminal_mass_delta_hi = checkpoint.chunk_terminal_delta.hi;
  control.well_prune_mass_cumulative_lo = well_new.lo;
  control.well_prune_mass_cumulative_hi = well_new.hi;
  control.rank_prune_mass_cumulative_lo = rank_new.lo;
  control.rank_prune_mass_cumulative_hi = rank_new.hi;
  control.terminal_mass_cumulative_lo = terminal_new.lo;
  control.terminal_mass_cumulative_hi = terminal_new.hi;
  control.unresolved_mass_lo = unresolved.lo;
  control.unresolved_mass_hi = unresolved.hi;
  control.expansion_count = checkpoint.chunk_expansion_count;
  control.gate_evaluation_count =
      checkpoint.chunk_gate_evaluation_count;
  control.probe_root_decision_count =
      checkpoint.chunk_probe_root_decision_count;
  control.probe_fallback_decision_count =
      checkpoint.chunk_probe_fallback_decision_count;
  control.deferred_int512_count = checkpoint.chunk_deferred_int512_count;
  control.deferred_int1024_count =
      checkpoint.chunk_deferred_int1024_count;
  control.rational_drain_count = checkpoint.chunk_rational_drain_count;
  control.prune_record_count = checkpoint.chunk_prune_used;
  control.terminal_record_count = checkpoint.chunk_terminal_used;
  control.probe_receipt_count = checkpoint.chunk_receipt_used;
  control.stack_high_water = checkpoint.complete_before_chunk != 0U
      ? 0U
      : checkpoint.stack_high_water;
  control.status = static_cast<std::uint64_t>(
      checkpoint.run_state ==
              static_cast<std::uint8_t>(EngineRunState::chunk_ready)
          ? Phase15HigherSupportDeviceTiledSlotStatus::chunk_ready
          : checkpoint.run_state ==
                  static_cast<std::uint8_t>(EngineRunState::complete)
              ? Phase15HigherSupportDeviceTiledSlotStatus::complete
              : checkpoint.run_state ==
                      static_cast<std::uint8_t>(EngineRunState::fatal)
                  ? Phase15HigherSupportDeviceTiledSlotStatus::fatal
                  : Phase15HigherSupportDeviceTiledSlotStatus::active);
  control.yield_reason = checkpoint.yield_reason;
  control.stop_reason = checkpoint.stop_reason;
  control.failure_code = checkpoint.failure_code;
  control.tile_epoch = tile_epoch;
  control.chunk_sequence = chunk_sequence;
  control.reserved_zero = 0U;
  return true;
}

// Commits the chunk into the checkpoint after its control was assembled:
// cumulative masses advance by the chunk deltas and the completion latch is
// updated.  Returns false on the same certified-range violations as the
// control assembly.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_ENGINE_HD inline bool
engine_commit_chunk(EngineSlotCheckpoint& checkpoint) noexcept {
  if (!phase15_higher_support_device_tiled_u128_add(
          checkpoint.well_cumulative, checkpoint.chunk_well_delta) ||
      !phase15_higher_support_device_tiled_u128_add(
          checkpoint.rank_cumulative, checkpoint.chunk_rank_delta) ||
      !phase15_higher_support_device_tiled_u128_add(
          checkpoint.terminal_cumulative,
          checkpoint.chunk_terminal_delta)) {
    return false;
  }
  checkpoint.complete = checkpoint.run_state ==
          static_cast<std::uint8_t>(EngineRunState::complete)
      ? 1U
      : 0U;
  return true;
}

#undef MORSEHGP3D_PHASE15_HS_ENGINE_HD

}  // namespace morsehgp3d::gpu::detail::higher_support_slot_engine
