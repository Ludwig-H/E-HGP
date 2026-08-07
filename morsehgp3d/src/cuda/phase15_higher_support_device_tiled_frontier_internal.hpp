#pragma once

#include "morsehgp3d/gpu/higher_support_device_tiled_frontier.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::detail {

enum class Phase15HigherSupportDeviceTiledExecutionKind : std::uint64_t {
  host_fake = 0U,
  cuda = 1U,
};

enum class Phase15HigherSupportDeviceTiledSlotStatus : std::uint64_t {
  active = 0U,
  chunk_ready = 1U,
  complete = 2U,
  fatal = 3U,
};

enum class Phase15HigherSupportDeviceTiledYieldReason : std::uint64_t {
  none = 0U,
  prune_segment_full = 1U,
  terminal_segment_full = 2U,
  deferred_queue_full = 3U,
};

enum class Phase15HigherSupportDeviceTiledStopReason : std::uint64_t {
  none = 0U,
  subdivision_capacity = 1U,
  fatal_failure = 2U,
};

// stack_capacity_exceeded is fatal by proof: the per-slot product segment is
// sized from the certified LBVH depth bound 16*d_T + 1, so overflowing it
// contradicts the stack-bound certificate instead of exhausting a budget.
enum class Phase15HigherSupportDeviceTiledFailureCode : std::uint64_t {
  none = 0U,
  malformed_traversal = 1U,
  arithmetic_overflow = 2U,
  internal_invariant = 3U,
  stack_capacity_exceeded = 4U,
};

enum class Phase15HigherSupportDeviceTiledProductStage : std::uint8_t {
  analyze_product = 0U,
  rank_search = 1U,
  emit_well_prune = 2U,
  emit_rank_prune = 3U,
  expand_product = 4U,
  classify_leaf = 5U,
};

enum class Phase15HigherSupportDeviceTiledPruneKind : std::uint64_t {
  no_well_centered_support = 0U,
  rank_window = 1U,
  strict_interior_carrier = 2U,
};

enum class Phase15HigherSupportDeviceTiledDeferredKind : std::uint64_t {
  support_prune = 0U,
  query_strict_interior = 1U,
  terminal_support_geometry = 2U,
};

#if defined(__CUDACC__)
#define MORSEHGP3D_PHASE15_HS_HOST_DEVICE __host__ __device__
#else
#define MORSEHGP3D_PHASE15_HS_HOST_DEVICE
#endif

// Unsigned 128-bit accumulator carried as two explicit 64-bit words so the
// ABI never depends on a compiler extension type.  Overflow reports are
// fail-closed: the caller must treat them as
// Phase15HigherSupportDeviceTiledFailureCode::arithmetic_overflow.
struct Phase15HigherSupportDeviceTiledUnsigned128 {
  std::uint64_t lo{};
  std::uint64_t hi{};

  friend constexpr bool operator==(
      const Phase15HigherSupportDeviceTiledUnsigned128&,
      const Phase15HigherSupportDeviceTiledUnsigned128&) = default;
};

[[nodiscard]] MORSEHGP3D_PHASE15_HS_HOST_DEVICE constexpr bool
phase15_higher_support_device_tiled_u128_add(
    Phase15HigherSupportDeviceTiledUnsigned128& value,
    Phase15HigherSupportDeviceTiledUnsigned128 addend) noexcept {
  const std::uint64_t low = value.lo + addend.lo;
  const std::uint64_t carry = low < value.lo ? 1U : 0U;
  const std::uint64_t high_partial = value.hi + addend.hi;
  if (high_partial < value.hi) {
    return false;
  }
  const std::uint64_t high = high_partial + carry;
  if (high < high_partial) {
    return false;
  }
  value.lo = low;
  value.hi = high;
  return true;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_HOST_DEVICE constexpr bool
phase15_higher_support_device_tiled_u128_subtract(
    Phase15HigherSupportDeviceTiledUnsigned128& value,
    Phase15HigherSupportDeviceTiledUnsigned128 subtrahend) noexcept {
  if (value.hi < subtrahend.hi ||
      (value.hi == subtrahend.hi && value.lo < subtrahend.lo)) {
    return false;
  }
  const std::uint64_t borrow = value.lo < subtrahend.lo ? 1U : 0U;
  value.lo -= subtrahend.lo;
  value.hi -= subtrahend.hi;
  value.hi -= borrow;
  return true;
}

[[nodiscard]] MORSEHGP3D_PHASE15_HS_HOST_DEVICE constexpr
    Phase15HigherSupportDeviceTiledUnsigned128
    phase15_higher_support_device_tiled_u128_multiply_u64(
        std::uint64_t left,
        std::uint64_t right) noexcept {
  const std::uint64_t left_low = left & UINT64_C(0xffffffff);
  const std::uint64_t left_high = left >> 32U;
  const std::uint64_t right_low = right & UINT64_C(0xffffffff);
  const std::uint64_t right_high = right >> 32U;
  const std::uint64_t low_low = left_low * right_low;
  const std::uint64_t low_high = left_low * right_high;
  const std::uint64_t high_low = left_high * right_low;
  const std::uint64_t high_high = left_high * right_high;
  const std::uint64_t middle =
      (low_low >> 32U) + (low_high & UINT64_C(0xffffffff)) +
      (high_low & UINT64_C(0xffffffff));
  Phase15HigherSupportDeviceTiledUnsigned128 product;
  product.lo = (low_low & UINT64_C(0xffffffff)) | (middle << 32U);
  product.hi = high_high + (low_high >> 32U) + (high_low >> 32U) +
      (middle >> 32U);
  return product;
}

// Multiplies a 128-bit value by a 64-bit word and reports overflow.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_HOST_DEVICE constexpr bool
phase15_higher_support_device_tiled_u128_multiply_small(
    Phase15HigherSupportDeviceTiledUnsigned128& value,
    std::uint64_t factor) noexcept {
  const Phase15HigherSupportDeviceTiledUnsigned128 low_product =
      phase15_higher_support_device_tiled_u128_multiply_u64(
          value.lo, factor);
  const Phase15HigherSupportDeviceTiledUnsigned128 high_product =
      phase15_higher_support_device_tiled_u128_multiply_u64(
          value.hi, factor);
  if (high_product.hi != 0U) {
    return false;
  }
  const std::uint64_t high = low_product.hi + high_product.lo;
  if (high < low_product.hi) {
    return false;
  }
  value.lo = low_product.lo;
  value.hi = high;
  return true;
}

// Divides an exact multiple by a divisor in [1, 2^32).  The caller must
// guarantee divisibility; a nonzero remainder fails closed.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_HOST_DEVICE constexpr bool
phase15_higher_support_device_tiled_u128_divide_exact_small(
    Phase15HigherSupportDeviceTiledUnsigned128& value,
    std::uint64_t divisor) noexcept {
  if (divisor == 0U || divisor >= (UINT64_C(1) << 32U)) {
    return false;
  }
  const std::uint64_t high_quotient = value.hi / divisor;
  const std::uint64_t high_remainder = value.hi % divisor;
  // Long division of (high_remainder:lo) by divisor, 32 bits at a time so
  // no intermediate exceeds 96 bits.
  const std::uint64_t upper = (high_remainder << 32U) | (value.lo >> 32U);
  const std::uint64_t upper_quotient = upper / divisor;
  const std::uint64_t upper_remainder = upper % divisor;
  const std::uint64_t lower =
      (upper_remainder << 32U) | (value.lo & UINT64_C(0xffffffff));
  const std::uint64_t lower_quotient = lower / divisor;
  if (lower % divisor != 0U) {
    return false;
  }
  value.hi = high_quotient;
  value.lo = (upper_quotient << 32U) | lower_quotient;
  return true;
}

// Progressive exact binomial scheme certified for range_size <= 2^31:
//   C(x,2) = x(x-1)/2                (< 2^61)
//   C(x,3) = C(x,2)*(x-2)/3          (product < 2^92, division exact)
//   C(x,4) = C(x,3)*(x-3)/4          (product < 2^121, division exact)
// Every intermediate stays strictly below 2^128 and every division is exact
// because k consecutive integers always contain the full factor k!, and the
// prime factors removed earlier never include the pending divisor.
[[nodiscard]] MORSEHGP3D_PHASE15_HS_HOST_DEVICE constexpr bool
phase15_higher_support_device_tiled_binomial_u128(
    std::uint64_t range_size,
    std::uint64_t multiplicity,
    Phase15HigherSupportDeviceTiledUnsigned128& out) noexcept {
  if (range_size >
          higher_support_device_tiled_frontier_maximum_point_count ||
      multiplicity == 0U || multiplicity > 4U) {
    return false;
  }
  if (range_size < multiplicity) {
    out = Phase15HigherSupportDeviceTiledUnsigned128{};
    return true;
  }
  Phase15HigherSupportDeviceTiledUnsigned128 value{range_size, 0U};
  if (multiplicity >= 2U &&
      (!phase15_higher_support_device_tiled_u128_multiply_small(
           value, range_size - 1U) ||
       !phase15_higher_support_device_tiled_u128_divide_exact_small(
           value, 2U))) {
    return false;
  }
  if (multiplicity >= 3U &&
      (!phase15_higher_support_device_tiled_u128_multiply_small(
           value, range_size - 2U) ||
       !phase15_higher_support_device_tiled_u128_divide_exact_small(
           value, 3U))) {
    return false;
  }
  if (multiplicity == 4U &&
      (!phase15_higher_support_device_tiled_u128_multiply_small(
           value, range_size - 3U) ||
       !phase15_higher_support_device_tiled_u128_divide_exact_small(
           value, 4U))) {
    return false;
  }
  out = value;
  return true;
}

#undef MORSEHGP3D_PHASE15_HS_HOST_DEVICE

inline constexpr std::uint8_t
    phase15_higher_support_device_tiled_flag_needs_wide_decision =
        std::uint8_t{1} << 0U;
inline constexpr std::uint8_t
    phase15_higher_support_device_tiled_flag_carrier_policy =
        std::uint8_t{1} << 1U;

// Fixed 128-byte resident product record.  The segment
// products_ + slot*products_per_slot IS the per-slot DFS stack and never
// crosses the device-to-host frontier.
struct alignas(16) Phase15HigherSupportDeviceTiledProductRecord {
  std::uint64_t group_node_index[4U]{};
  std::uint64_t group_leaf_begin[4U]{};
  std::uint64_t group_leaf_end[4U]{};
  std::uint8_t group_multiplicity[4U]{};
  std::uint8_t support_size{};
  std::uint8_t group_count{};
  std::uint8_t stage{};
  std::uint8_t flags{};
  std::uint64_t reserved_zero[3U]{};
};

// The only per-slot payload copied to the host on every chunk.  Mass fields
// are unsigned 128-bit lo/hi pairs; the host recomputes cumulative =
// previous + delta in exact::BigInt and closes the local partition
// slot_universe_mass == well + rank + terminal + unresolved before any
// commit.  Resolved mass is derived, never transferred, and exact::BigInt
// never crosses this frontier.
struct Phase15HigherSupportDeviceTiledSlotControl {
  std::uint64_t root_entry_digest{};
  std::uint64_t well_prune_mass_delta_lo{};
  std::uint64_t well_prune_mass_delta_hi{};
  std::uint64_t rank_prune_mass_delta_lo{};
  std::uint64_t rank_prune_mass_delta_hi{};
  std::uint64_t terminal_mass_delta_lo{};
  std::uint64_t terminal_mass_delta_hi{};
  std::uint64_t well_prune_mass_cumulative_lo{};
  std::uint64_t well_prune_mass_cumulative_hi{};
  std::uint64_t rank_prune_mass_cumulative_lo{};
  std::uint64_t rank_prune_mass_cumulative_hi{};
  std::uint64_t terminal_mass_cumulative_lo{};
  std::uint64_t terminal_mass_cumulative_hi{};
  std::uint64_t unresolved_mass_lo{};
  std::uint64_t unresolved_mass_hi{};
  std::uint64_t expansion_count{};
  std::uint64_t gate_evaluation_count{};
  std::uint64_t probe_root_decision_count{};
  std::uint64_t probe_fallback_decision_count{};
  std::uint64_t deferred_int512_count{};
  std::uint64_t deferred_int1024_count{};
  std::uint64_t rational_drain_count{};
  std::uint64_t prune_record_count{};
  std::uint64_t terminal_record_count{};
  std::uint64_t probe_receipt_count{};
  // Tile-cumulative by contract (never reset between chunks of one tile,
  // reported as zero by a slot that was already complete before the
  // chunk).  A native launcher must reproduce this exact semantics for
  // bit-parity with the host fake.
  std::uint64_t stack_high_water{};
  std::uint64_t status{};
  std::uint64_t yield_reason{};
  std::uint64_t stop_reason{};
  std::uint64_t failure_code{};
  std::uint64_t tile_epoch{};
  std::uint64_t chunk_sequence{};
  std::uint64_t reserved_zero{};
};

// 32-byte strict-interior probe receipt in ExactHigherSupportNodeReceipt
// shape plus its exact certified point mass.
struct Phase15HigherSupportDeviceTiledProbeReceipt {
  std::uint64_t node_index{};
  std::uint64_t leaf_begin{};
  std::uint64_t leaf_end{};
  std::uint64_t certified_point_count{};
};

// 160-byte replayable prune record.  Receipts live in the fixed per-slot
// receipt segment; receipt_segment_begin indexes into it.
struct alignas(16) Phase15HigherSupportDeviceTiledPruneRecord {
  std::uint64_t group_node_index[4U]{};
  std::uint64_t group_leaf_begin[4U]{};
  std::uint64_t group_leaf_end[4U]{};
  std::uint64_t pruned_mass_lo{};
  std::uint64_t pruned_mass_hi{};
  std::uint64_t prune_kind{};
  std::uint64_t receipt_count{};
  std::uint64_t receipt_segment_begin{};
  std::uint8_t group_multiplicity[4U]{};
  std::uint8_t support_size{};
  std::uint8_t group_count{};
  std::uint8_t terminal_policy{};
  std::uint8_t flags{};
  std::uint64_t reserved_zero{};
};

// 96-byte categorical terminal record.  Closed-ball classification stays on
// the host in v1 (exact_higher_support_terminal_classification_native_cuda
// remains false); the record carries only the identity, the exact geometry
// category and its provenance.
struct alignas(16) Phase15HigherSupportDeviceTiledTerminalRecord {
  std::uint64_t point_ids[4U]{};
  std::uint64_t slot_index{};
  std::uint8_t support_size{};
  std::uint8_t terminal_geometry_decision{};
  std::uint8_t decision_backend{};
  std::uint8_t terminal_policy{};
  std::uint8_t flags{};
  std::uint8_t reserved_zero_bytes[3U]{};
  std::uint64_t reserved_zero[5U]{};
};

// 264-byte host rational-drain task.  When the int1024 evaluation of one
// exact decision overflows the 124-bit aligned envelope, the native kernel
// suspends the owning slot and stages the complete decision input here; the
// launcher resolves it with the arbitrary-precision host primitives between
// kernel relaunches of the same subdivision and writes the verdict back.
// The host fake never stages one (it resolves rationally inline), so its
// rational-task transfer accounting stays zero.
struct Phase15HigherSupportDeviceTiledRationalDrainTask {
  std::uint64_t support_lower_bits[4U][3U]{};
  std::uint64_t support_upper_bits[4U][3U]{};
  std::uint64_t query_lower_bits[3U]{};
  std::uint64_t query_upper_bits[3U]{};
  std::uint64_t wide_kind{};
  std::uint64_t origin_slot_index{};
  std::uint8_t support_size{};
  std::uint8_t has_query{};
  std::uint8_t reserved_zero_bytes[6U]{};
};

// 176-byte deferred wide decision.  Unused by the host fake (its queues stay
// empty and their counters zero); the native path batches int512/int1024
// escalations through it in M4.
struct alignas(16) Phase15HigherSupportDeviceTiledDeferredDecision {
  std::uint64_t group_node_index[4U]{};
  std::uint64_t group_leaf_begin[4U]{};
  std::uint64_t group_leaf_end[4U]{};
  std::uint64_t query_node_index{};
  std::uint64_t query_leaf_begin{};
  std::uint64_t query_leaf_end{};
  std::uint64_t deferred_kind{};
  std::uint64_t required_width_bits{};
  std::uint64_t origin_slot_index{};
  std::uint64_t write_back_index{};
  std::uint8_t group_multiplicity[4U]{};
  std::uint8_t support_size{};
  std::uint8_t group_count{};
  std::uint8_t stage{};
  std::uint8_t flags{};
  std::uint64_t reserved_zero[2U]{};
};

static_assert(
    sizeof(Phase15HigherSupportDeviceTiledProductRecord) == 128U);
static_assert(
    sizeof(Phase15HigherSupportDeviceTiledSlotControl) == 264U);
static_assert(
    sizeof(Phase15HigherSupportDeviceTiledProbeReceipt) == 32U);
static_assert(
    sizeof(Phase15HigherSupportDeviceTiledPruneRecord) == 160U);
static_assert(
    sizeof(Phase15HigherSupportDeviceTiledTerminalRecord) == 96U);
static_assert(
    sizeof(Phase15HigherSupportDeviceTiledDeferredDecision) == 176U);
static_assert(
    sizeof(Phase15HigherSupportDeviceTiledRationalDrainTask) == 264U);
static_assert(std::is_standard_layout_v<
                  Phase15HigherSupportDeviceTiledRationalDrainTask> &&
              std::is_trivially_copyable_v<
                  Phase15HigherSupportDeviceTiledRationalDrainTask>);
static_assert(std::is_standard_layout_v<
                  Phase15HigherSupportDeviceTiledProductRecord> &&
              std::is_trivially_copyable_v<
                  Phase15HigherSupportDeviceTiledProductRecord>);
static_assert(std::is_standard_layout_v<
                  Phase15HigherSupportDeviceTiledSlotControl> &&
              std::is_trivially_copyable_v<
                  Phase15HigherSupportDeviceTiledSlotControl>);
static_assert(std::is_standard_layout_v<
                  Phase15HigherSupportDeviceTiledProbeReceipt> &&
              std::is_trivially_copyable_v<
                  Phase15HigherSupportDeviceTiledProbeReceipt>);
static_assert(std::is_standard_layout_v<
                  Phase15HigherSupportDeviceTiledPruneRecord> &&
              std::is_trivially_copyable_v<
                  Phase15HigherSupportDeviceTiledPruneRecord>);
static_assert(std::is_standard_layout_v<
                  Phase15HigherSupportDeviceTiledTerminalRecord> &&
              std::is_trivially_copyable_v<
                  Phase15HigherSupportDeviceTiledTerminalRecord>);
static_assert(std::is_standard_layout_v<
                  Phase15HigherSupportDeviceTiledDeferredDecision> &&
              std::is_trivially_copyable_v<
                  Phase15HigherSupportDeviceTiledDeferredDecision>);

struct Phase15HigherSupportDeviceTiledAdoptedTraversal {
  std::shared_ptr<void> retained_owner;
  std::shared_ptr<const void> source_cloud_identity;
  const std::uint64_t* device_coordinate_bits{};
  const std::uint64_t* device_morton_point_ids{};
  const void* device_nodes{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t maximum_point_count{};
  std::size_t maximum_node_count{};
  std::size_t retained_coordinate_word_capacity{};
  std::size_t retained_morton_point_id_capacity{};
  std::size_t retained_node_capacity{};
  std::uint64_t source_snapshot_epoch{};
  int cuda_device{-1};
  Phase15HigherSupportDeviceTiledExecutionKind execution_kind{
      Phase15HigherSupportDeviceTiledExecutionKind::host_fake};
  bool canonical_coordinate_words_retained{false};
  bool active_morton_point_ids_retained{false};
  bool certified_device_nodes_retained{false};
  bool host_fake_lifecycle_exercised{false};
  bool cuda_device_storage_retained{false};
};

struct Phase15HigherSupportDeviceTiledRequest {
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t record_buffer_epoch{};
  std::uint64_t tile_epoch{};
  std::uint64_t chunk_sequence{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t certified_maximum_lbvh_depth{};
  std::size_t slot_count{};
  HigherSupportDeviceTiledFrontierTerminalPolicy terminal_policy{
      HigherSupportDeviceTiledFrontierTerminalPolicy::closed_rank_window};
  std::size_t maximum_relevant_closed_rank{};
  std::size_t gate_evaluations_per_slot_per_chunk{};
  std::size_t maximum_subdivision_count{};
  std::size_t products_per_slot{};
  std::size_t prune_records_per_slot{};
  std::size_t terminal_records_per_slot{};
  std::size_t probe_receipts_per_slot{};
  std::size_t pending_decisions_per_slot{};
  // Root entries of the freshly opened tile, already fail-closed validated
  // by the host context.  Empty exactly when resume_same_tile is true: a
  // resumed chunk continues from the retained slot state and must not
  // re-submit roots.
  std::span<const hierarchy::ExactHigherSupportFrontierEntry>
      root_entries{};
  bool resume_same_tile{false};
};

struct Phase15HigherSupportDeviceTiledBatch {
  std::shared_ptr<void> retained_output_owner;
  std::shared_ptr<const void> source_cloud_identity_authority;
  // In host-fake execution these views address host memory retained by
  // retained_output_owner so an exact consumer can replay every record; in
  // CUDA execution they address the resident arena.
  const Phase15HigherSupportDeviceTiledPruneRecord* prune_records{};
  const Phase15HigherSupportDeviceTiledTerminalRecord* terminal_records{};
  const Phase15HigherSupportDeviceTiledProbeReceipt* probe_receipts{};
  const Phase15HigherSupportDeviceTiledSlotControl* slot_controls{};
  std::vector<Phase15HigherSupportDeviceTiledSlotControl>
      host_slot_controls;
  // R2-f: host staging of the three record arenas.
  //
  // In CUDA execution the arenas above are the engine's own cudaMalloc
  // allocations, so the four segment pointers are DEVICE pointers and no
  // host consumer may walk them.  The driver therefore stages them
  // device-to-host into these vectors and repoints prune_records,
  // terminal_records and probe_receipts at them before publishing the
  // batch.  The staging keeps the ORIGINAL per-slot stride -- consumers
  // address `segment[slot * stride + index]`, so a compacted copy would
  // silently mis-address every slot but the first.  Only the committed
  // prefix of each slot carries meaning; the copy width is the maximum
  // committed count over the slots, read from host_slot_controls.
  //
  // The host fake allocates its arenas in host memory and leaves these
  // empty, publishing its own pointers directly.
  std::vector<Phase15HigherSupportDeviceTiledPruneRecord>
      host_prune_records;
  std::vector<Phase15HigherSupportDeviceTiledTerminalRecord>
      host_terminal_records;
  std::vector<Phase15HigherSupportDeviceTiledProbeReceipt>
      host_probe_receipts;
  // The single fact the drain lease publishes.  True only when the four
  // segment pointers may be dereferenced by the host: always for the host
  // fake, and for CUDA only after the staging above has run.  A producer
  // that cannot honour it must leave it false, and the consumer fails
  // closed.
  bool record_segments_host_readable{false};
  std::size_t physical_product_record_capacity{};
  std::size_t physical_prune_record_capacity{};
  std::size_t physical_terminal_record_capacity{};
  std::size_t physical_probe_receipt_capacity{};
  std::size_t physical_slot_control_capacity{};
  std::size_t physical_slot_checkpoint_capacity{};
  std::size_t physical_deferred_int512_capacity{};
  std::size_t physical_deferred_int1024_capacity{};
  std::size_t physical_pending_slot_count_capacity{};
  std::size_t physical_device_arena_capacity_bytes{};
  std::size_t slot_control_device_to_host_count{};
  std::size_t slot_control_device_to_host_byte_count{};
  std::size_t resume_control_device_to_host_count{};
  std::size_t resume_control_device_to_host_byte_count{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  std::size_t traversal_subdivision_count{};
  // Rational-drain relaunches of one chunk: the launcher resolves staged
  // int1024-overflow decisions on the host and relaunches the same
  // subdivision without granting a fresh gate quantum.  Zero on the fake.
  std::size_t host_rational_drain_relaunch_count{};
  std::size_t rational_task_device_to_host_count{};
  std::size_t rational_task_device_to_host_byte_count{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t record_buffer_epoch{};
  std::uint64_t tile_epoch{};
  std::uint64_t chunk_sequence{};
  std::uint64_t metadata_digest{};
  int cuda_device{-1};
  Phase15HigherSupportDeviceTiledExecutionKind execution_kind{
      Phase15HigherSupportDeviceTiledExecutionKind::host_fake};
  HigherSupportDeviceTiledFrontierTerminalPolicy terminal_policy{
      HigherSupportDeviceTiledFrontierTerminalPolicy::closed_rank_window};
  std::size_t maximum_relevant_closed_rank{};
  bool fixed_slot_segments_allocated{false};
  bool output_owner_detached_for_tile_lifetime{false};
  bool unsigned128_slot_mass_accounting_requested{false};
  bool probe_plan_order_receipt_commit_requested{false};
  bool substitute_probe_center_seed_policy_v1_requested{false};
  bool closed_rank_window_receipts_sealed_to_policy{false};
  bool strict_interior_carrier_receipts_sealed_to_policy{false};
  bool strict_interior_carrier_never_authorizes_gamma2_drop{false};
  bool censored_slot_outputs_invalidated{false};
  bool bigint_support_mass_transferred{false};
  bool floating_point_decision_performed{false};
  bool exact_higher_support_terminal_classification_native_cuda{false};
  bool dense_product_fallback_performed{false};
  bool global_product_frontier_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool global_cell_coface_or_incidence_arena_materialized{false};
  bool cuda_execution_contract_satisfied{false};
  bool fresh_tile_device_arena_allocated{false};
  bool fresh_tile_device_arena_reused{false};
  bool resume_same_tile{false};
  bool capacity_yield_resumable{false};
  bool process_restart_resumable{false};
};

// Private borrowed capability for the exact drain stage.  Callers must
// retain the move-only drain lease while using these views, so the combined
// source and output authority continues to dominate every address below.
struct Phase15HigherSupportDeviceTiledRecordDrainPrivateViews {
  const void* retained_authority_identity{};
  const void* source_owner_identity{};
  const void* source_cloud_identity{};
  std::shared_ptr<void> source_owner_authority;
  std::shared_ptr<const void> source_cloud_identity_authority;
  const Phase15HigherSupportDeviceTiledPruneRecord* prune_records{};
  const Phase15HigherSupportDeviceTiledTerminalRecord* terminal_records{};
  const Phase15HigherSupportDeviceTiledProbeReceipt* probe_receipts{};
  const Phase15HigherSupportDeviceTiledSlotControl* slot_controls{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t record_buffer_epoch{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  HigherSupportDeviceTiledFrontierTerminalPolicy terminal_policy{
      HigherSupportDeviceTiledFrontierTerminalPolicy::closed_rank_window};
  std::size_t maximum_relevant_closed_rank{};
  std::size_t prune_record_stride{};
  std::size_t terminal_record_stride{};
  std::size_t probe_receipt_stride{};
  std::size_t physical_slot_control_capacity{};
  std::size_t authorized_slot_control_extent{};
  std::size_t slot_begin{};
  std::size_t slot_end{};
  int cuda_device{-1};
  bool host_fake{false};
  bool ready{false};
};

class Phase15HigherSupportDeviceTiledRecordDrainPrivateViewAccess final {
 public:
  [[nodiscard]] static
      Phase15HigherSupportDeviceTiledRecordDrainPrivateViews
      inspect(
          const HigherSupportDeviceTiledRecordDrainLease& lease) noexcept;
};

inline constexpr std::uint64_t
    phase15_higher_support_device_tiled_fnv_offset_basis =
        UINT64_C(14695981039346656037);
inline constexpr std::uint64_t
    phase15_higher_support_device_tiled_fnv_prime =
        UINT64_C(1099511628211);

inline void phase15_higher_support_device_tiled_hash_word(
    std::uint64_t& digest,
    std::uint64_t word) noexcept {
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= phase15_higher_support_device_tiled_fnv_prime;
  }
}

[[nodiscard]] inline std::uint64_t
phase15_higher_support_device_tiled_root_entry_digest(
    const hierarchy::ExactHigherSupportFrontierEntry& entry) noexcept {
  std::uint64_t digest =
      phase15_higher_support_device_tiled_fnv_offset_basis;
  phase15_higher_support_device_tiled_hash_word(digest, entry.support_size);
  phase15_higher_support_device_tiled_hash_word(digest, entry.group_count);
  for (const hierarchy::ExactHigherSupportNodeGroup& group : entry.groups) {
    phase15_higher_support_device_tiled_hash_word(digest, group.node_index);
    phase15_higher_support_device_tiled_hash_word(digest, group.leaf_begin);
    phase15_higher_support_device_tiled_hash_word(digest, group.leaf_end);
    phase15_higher_support_device_tiled_hash_word(
        digest, group.multiplicity);
  }
  return digest;
}

[[nodiscard]] inline std::uint64_t
phase15_higher_support_device_tiled_metadata_digest(
    const Phase15HigherSupportDeviceTiledBatch& batch) noexcept {
  std::uint64_t digest =
      phase15_higher_support_device_tiled_fnv_offset_basis;
  const auto hash_size = [&digest](std::size_t value) noexcept {
    phase15_higher_support_device_tiled_hash_word(
        digest, static_cast<std::uint64_t>(value));
  };
  hash_size(batch.host_slot_controls.size());
  hash_size(batch.physical_product_record_capacity);
  hash_size(batch.physical_prune_record_capacity);
  hash_size(batch.physical_terminal_record_capacity);
  hash_size(batch.physical_probe_receipt_capacity);
  hash_size(batch.physical_slot_control_capacity);
  hash_size(batch.physical_slot_checkpoint_capacity);
  hash_size(batch.physical_deferred_int512_capacity);
  hash_size(batch.physical_deferred_int1024_capacity);
  hash_size(batch.physical_pending_slot_count_capacity);
  hash_size(batch.physical_device_arena_capacity_bytes);
  hash_size(batch.slot_control_device_to_host_count);
  hash_size(batch.slot_control_device_to_host_byte_count);
  hash_size(batch.resume_control_device_to_host_count);
  hash_size(batch.resume_control_device_to_host_byte_count);
  hash_size(batch.kernel_launch_count);
  hash_size(batch.synchronization_count);
  hash_size(batch.traversal_subdivision_count);
  hash_size(batch.host_rational_drain_relaunch_count);
  hash_size(batch.rational_task_device_to_host_count);
  hash_size(batch.rational_task_device_to_host_byte_count);
  hash_size(batch.maximum_relevant_closed_rank);
  phase15_higher_support_device_tiled_hash_word(
      digest, batch.source_snapshot_epoch);
  phase15_higher_support_device_tiled_hash_word(
      digest, batch.record_buffer_epoch);
  phase15_higher_support_device_tiled_hash_word(digest, batch.tile_epoch);
  phase15_higher_support_device_tiled_hash_word(
      digest, batch.chunk_sequence);
  phase15_higher_support_device_tiled_hash_word(
      digest, static_cast<std::uint64_t>(batch.cuda_device));
  phase15_higher_support_device_tiled_hash_word(
      digest, static_cast<std::uint64_t>(batch.execution_kind));
  phase15_higher_support_device_tiled_hash_word(
      digest, static_cast<std::uint64_t>(batch.terminal_policy));
  for (const bool flag : {
           batch.fixed_slot_segments_allocated,
           batch.output_owner_detached_for_tile_lifetime,
           batch.unsigned128_slot_mass_accounting_requested,
           batch.probe_plan_order_receipt_commit_requested,
           batch.substitute_probe_center_seed_policy_v1_requested,
           batch.closed_rank_window_receipts_sealed_to_policy,
           batch.strict_interior_carrier_receipts_sealed_to_policy,
           batch.strict_interior_carrier_never_authorizes_gamma2_drop,
           batch.censored_slot_outputs_invalidated,
           batch.bigint_support_mass_transferred,
           batch.floating_point_decision_performed,
           batch.exact_higher_support_terminal_classification_native_cuda,
           batch.dense_product_fallback_performed,
           batch.global_product_frontier_materialized,
           batch.ordinary_or_higher_order_delaunay_materialized,
           batch.global_cell_coface_or_incidence_arena_materialized,
           batch.cuda_execution_contract_satisfied,
           batch.fresh_tile_device_arena_allocated,
           batch.fresh_tile_device_arena_reused,
           batch.resume_same_tile,
           batch.capacity_yield_resumable,
           batch.process_restart_resumable}) {
    phase15_higher_support_device_tiled_hash_word(
        digest, flag ? UINT64_C(1) : UINT64_C(0));
  }
  for (const Phase15HigherSupportDeviceTiledSlotControl& control :
       batch.host_slot_controls) {
    for (const std::uint64_t word : {
             control.root_entry_digest,
             control.well_prune_mass_delta_lo,
             control.well_prune_mass_delta_hi,
             control.rank_prune_mass_delta_lo,
             control.rank_prune_mass_delta_hi,
             control.terminal_mass_delta_lo,
             control.terminal_mass_delta_hi,
             control.well_prune_mass_cumulative_lo,
             control.well_prune_mass_cumulative_hi,
             control.rank_prune_mass_cumulative_lo,
             control.rank_prune_mass_cumulative_hi,
             control.terminal_mass_cumulative_lo,
             control.terminal_mass_cumulative_hi,
             control.unresolved_mass_lo,
             control.unresolved_mass_hi,
             control.expansion_count,
             control.gate_evaluation_count,
             control.probe_root_decision_count,
             control.probe_fallback_decision_count,
             control.deferred_int512_count,
             control.deferred_int1024_count,
             control.rational_drain_count,
             control.prune_record_count,
             control.terminal_record_count,
             control.probe_receipt_count,
             control.stack_high_water,
             control.status,
             control.yield_reason,
             control.stop_reason,
             control.failure_code,
             control.tile_epoch,
             control.chunk_sequence,
             control.reserved_zero}) {
      phase15_higher_support_device_tiled_hash_word(digest, word);
    }
  }
  return digest;
}

class Phase15HigherSupportDeviceTiledFrontierContextState final {
 public:
  Phase15HigherSupportDeviceTiledFrontierContextState() = default;
  ~Phase15HigherSupportDeviceTiledFrontierContextState() = default;

  Phase15HigherSupportDeviceTiledFrontierContextState(
      const Phase15HigherSupportDeviceTiledFrontierContextState&) = delete;
  Phase15HigherSupportDeviceTiledFrontierContextState& operator=(
      const Phase15HigherSupportDeviceTiledFrontierContextState&) = delete;

  template <typename Preflight, typename Operation>
  decltype(auto) with_launcher_section(
      Preflight&& preflight,
      Operation&& operation) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (poisoned_.load(std::memory_order_acquire)) {
      throw std::runtime_error(
          "the Phase 15 higher-support device tiled context is poisoned "
          "by a prior launcher or validation failure");
    }
    // Caller-side backpressure runs under the same mutex but before the
    // poisoning boundary: a live drain lease is a reversible precondition
    // failure, never a launcher failure.
    std::forward<Preflight>(preflight)();
    try {
      return std::forward<Operation>(operation)();
    } catch (...) {
      poisoned_.store(true, std::memory_order_release);
      throw;
    }
  }

  [[nodiscard]] std::uint64_t advance_epoch() {
    if (epoch_ == std::numeric_limits<std::uint64_t>::max()) {
      throw std::overflow_error(
          "the Phase 15 higher-support device tiled output epoch "
          "overflowed");
    }
    return ++epoch_;
  }

  [[nodiscard]] bool poisoned() const noexcept {
    return poisoned_.load(std::memory_order_acquire);
  }

  [[nodiscard]] std::shared_ptr<void>& device_resources() noexcept {
    return device_resources_;
  }
  [[nodiscard]] const std::shared_ptr<void>& device_resources()
      const noexcept {
    return device_resources_;
  }

 private:
  std::mutex mutex_;
  std::atomic<bool> poisoned_{false};
  std::uint64_t epoch_{};
  std::shared_ptr<void> device_resources_;
};

[[nodiscard]] Phase15HigherSupportDeviceTiledBatch
build_phase15_higher_support_device_tiled_frontier_on_device(
    Phase15HigherSupportDeviceTiledFrontierContextState& context,
    const Phase15HigherSupportDeviceTiledAdoptedTraversal& traversal,
    const Phase15HigherSupportDeviceTiledRequest& request);

}  // namespace morsehgp3d::gpu::detail
