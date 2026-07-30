#pragma once

#include "morsehgp3d/gpu/morton_yao48_device_tiled_pair_frontier.hpp"

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

enum class Phase15MortonYao48DeviceTiledExecutionKind : std::uint64_t {
  host_fake = 0U,
  cuda = 1U,
};

enum class Phase15MortonYao48DeviceTiledAnchorStatus : std::uint64_t {
  active = 0U,
  chunk_ready = 1U,
  complete = 2U,
  fatal = 3U,
};

enum class Phase15MortonYao48DeviceTiledYieldReason : std::uint64_t {
  none = 0U,
  candidate_segment_full = 1U,
  prune_segment_full = 2U,
};

enum class Phase15MortonYao48DeviceTiledFailureCode : std::uint64_t {
  none = 0U,
  malformed_traversal = 1U,
  arithmetic_overflow = 2U,
  internal_invariant = 3U,
};

// Fixed 48-byte proposal ABI.  Cone 48 is the unbanked ambiguity sentinel;
// it remains a candidate and can never support a prune certificate.
struct Phase15MortonYao48DeviceTiledCandidateRecord {
  std::uint64_t support_u{};
  std::uint64_t support_v{};
  std::uint64_t anchor_morton_position{};
  std::uint64_t partner_morton_position{};
  std::uint64_t owner_cone_index{};
  std::uint64_t flags{};
};

// A region is opaque to the product API.  Its mass is repeated in the
// AnchorControl so the host can close the coverage partition without copying
// this record.  Witnesses credited by the kernel are already outside the
// pruned node range.
struct alignas(16) Phase15MortonYao48DeviceTiledPruneRegionRecord {
  std::uint64_t anchor_morton_position{};
  std::uint64_t node_index{};
  std::uint64_t certified_pair_mass{};
  std::uint64_t retained_witness_count{};
  std::uint64_t retained_witness_bank_mask{};
  std::uint64_t flags{};
  std::uint64_t witness_point_ids[10U]{};
};

// Private fixed bank scratch.  The kernel owns the ordering key; the public
// lease retains the arena only to dominate its asynchronous lifetime.
struct Phase15MortonYao48DeviceTiledWitnessBankSlot {
  std::uint64_t witness_point_id{};
  std::uint64_t witness_morton_position{};
  std::uint64_t squared_distance_lower_bits{};
  std::uint64_t squared_distance_upper_bits{};
};

// This is the only per-anchor payload copied to the host.  Counts without a
// `cumulative_` prefix are deltas for this output chunk.  Cumulative fields
// authenticate continuation and make rollback/duplication fail closed.
struct Phase15MortonYao48DeviceTiledAnchorControl {
  std::uint64_t anchor_morton_position{};
  std::uint64_t candidate_count{};
  std::uint64_t prune_region_count{};
  std::uint64_t certified_pruned_pair_mass{};
  std::uint64_t node_visit_count{};
  std::uint64_t cumulative_candidate_count{};
  std::uint64_t cumulative_prune_region_count{};
  std::uint64_t cumulative_certified_pruned_pair_mass{};
  std::uint64_t cumulative_node_visit_count{};
  std::uint64_t status{};
  std::uint64_t yield_reason{};
  std::uint64_t stop_reason{};
  std::uint64_t failure_code{};
  std::uint64_t ambiguous_cone_candidate_count{};
  std::uint64_t unbanked_candidate_count{};
  std::uint64_t cumulative_ambiguous_cone_candidate_count{};
  std::uint64_t cumulative_unbanked_candidate_count{};
  std::uint64_t unresolved_pair_mass{};
  std::uint64_t tile_epoch{};
  std::uint64_t chunk_sequence{};
  std::uint64_t reserved_zero{};
};

// Private, device-resident continuation state.  Chunk-ready is exposed by the
// copied AnchorControl; this checkpoint remains private and preserves both the
// next chunk's deltas and authenticated cumulative totals between launches.
struct alignas(16) Phase15MortonYao48DeviceTiledAnchorCheckpoint {
  std::uint64_t anchor_morton_position{};
  std::uint64_t cursor{};
  std::uint64_t candidate_count{};
  std::uint64_t prune_region_count{};
  std::uint64_t certified_pruned_pair_mass{};
  std::uint64_t node_visit_count{};
  std::uint64_t ambiguous_cone_candidate_count{};
  std::uint64_t unbanked_candidate_count{};
  std::uint64_t cumulative_candidate_count{};
  std::uint64_t cumulative_prune_region_count{};
  std::uint64_t cumulative_certified_pruned_pair_mass{};
  std::uint64_t cumulative_node_visit_count{};
  std::uint64_t cumulative_ambiguous_cone_candidate_count{};
  std::uint64_t cumulative_unbanked_candidate_count{};
  std::uint64_t retained_witness_count{};
  std::uint64_t completed_subdivision_count{};
  std::uint64_t tile_epoch{};
  std::uint64_t chunk_sequence{};
  std::uint64_t state{};
  std::uint64_t reserved_zero{};
};

static_assert(
    sizeof(Phase15MortonYao48DeviceTiledCandidateRecord) == 48U);
static_assert(
    sizeof(Phase15MortonYao48DeviceTiledPruneRegionRecord) == 128U);
static_assert(
    sizeof(Phase15MortonYao48DeviceTiledWitnessBankSlot) == 32U);
static_assert(
    sizeof(Phase15MortonYao48DeviceTiledAnchorControl) == 168U);
static_assert(
    sizeof(Phase15MortonYao48DeviceTiledAnchorCheckpoint) == 160U);
static_assert(std::is_trivially_copyable_v<
              Phase15MortonYao48DeviceTiledAnchorControl>);
static_assert(std::is_standard_layout_v<
              Phase15MortonYao48DeviceTiledAnchorControl>);

struct Phase15MortonYao48DeviceTiledAdoptedTraversal {
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
  Phase15MortonYao48DeviceTiledExecutionKind execution_kind{
      Phase15MortonYao48DeviceTiledExecutionKind::host_fake};
  bool canonical_coordinate_words_retained{false};
  bool active_morton_point_ids_retained{false};
  bool certified_device_nodes_retained{false};
  bool host_fake_lifecycle_exercised{false};
  bool cuda_device_storage_retained{false};
};

struct Phase15MortonYao48DeviceTiledRequest {
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t output_buffer_epoch{};
  std::uint64_t tile_epoch{};
  std::uint64_t chunk_sequence{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t anchor_begin{};
  std::size_t anchor_count{};
  std::size_t maximum_closed_rank{};
  std::size_t node_visit_capacity_per_anchor{};
  std::size_t candidate_capacity_per_anchor{};
  std::size_t prune_region_capacity_per_anchor{};
  std::size_t witness_bank_count_per_anchor{};
  std::size_t witness_slot_count_per_bank{};
  bool resume_same_tile{false};
};

struct Phase15MortonYao48DeviceTiledBatch {
  std::shared_ptr<void> retained_output_owner;
  std::shared_ptr<const void> source_cloud_identity_authority;
  // CUDA addresses remain valid exactly while retained_output_owner lives.
  // Host fakes leave them null and retain a lifecycle marker instead.
  const Phase15MortonYao48DeviceTiledCandidateRecord*
      device_candidate_records{};
  const Phase15MortonYao48DeviceTiledPruneRegionRecord*
      device_prune_regions{};
  const Phase15MortonYao48DeviceTiledWitnessBankSlot*
      device_witness_bank_slots{};
  const Phase15MortonYao48DeviceTiledAnchorControl*
      device_anchor_controls{};
  std::vector<Phase15MortonYao48DeviceTiledAnchorControl>
      host_anchor_controls;
  std::size_t physical_candidate_capacity{};
  std::size_t physical_prune_region_capacity{};
  std::size_t physical_witness_bank_slot_capacity{};
  std::size_t physical_anchor_control_capacity{};
  std::size_t physical_anchor_checkpoint_capacity{};
  std::size_t physical_pending_anchor_count_capacity{};
  std::size_t physical_device_arena_capacity_bytes{};
  std::size_t anchor_control_device_to_host_count{};
  std::size_t anchor_control_device_to_host_byte_count{};
  std::size_t candidate_device_to_host_count{};
  std::size_t certified_prune_device_to_host_count{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  std::size_t traversal_subdivision_count{};
  std::size_t maximum_traversal_subdivision_count_per_anchor{};
  // Control-plane D2H only: one uint64 pending count after each subdivision.
  std::size_t resume_control_device_to_host_count{};
  std::size_t resume_control_device_to_host_byte_count{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t output_buffer_epoch{};
  std::uint64_t tile_epoch{};
  std::uint64_t chunk_sequence{};
  std::uint64_t metadata_digest{};
  int cuda_device{-1};
  Phase15MortonYao48DeviceTiledExecutionKind execution_kind{
      Phase15MortonYao48DeviceTiledExecutionKind::host_fake};
  bool fixed_anchor_segments_allocated{false};
  bool output_owner_detached_for_tile_lifetime{false};
  bool interval_cone_classification_requested{false};
  bool ambiguous_cone_to_unbanked_candidate_requested{false};
  bool target_tested_before_bank_insert_requested{false};
  bool retained_witnesses_outside_pruned_subtree_requested{false};
  bool nonnegative_diametral_witness_interval_lower_bound_requested{false};
  bool censored_anchor_outputs_invalidated{false};
  bool exact_diametral_rank_evaluated{false};
  bool scientific_pair_catalog_published{false};
  bool dense_pair_fallback_performed{false};
  bool global_pair_matrix_materialized{false};
  bool higher_order_structure_materialized{false};
  bool cuda_execution_contract_satisfied{false};
  bool resume_same_tile{false};
  bool capacity_yield_resumable{false};
  bool process_restart_resumable{false};
};

// Private borrowed capability for the resident exact-classification stage.
// It is deliberately absent from the product header API: callers must retain
// the move-only tile lease while using these views, so the combined source and
// output authority continues to dominate every address below.
struct Phase15MortonYao48DeviceCandidateTilePrivateViews {
  const void* retained_authority_identity{};
  const void* source_owner_identity{};
  const void* source_cloud_identity{};
  std::shared_ptr<void> source_owner_authority;
  std::shared_ptr<const void> source_cloud_identity_authority;
  const std::uint64_t* device_coordinate_bits{};
  const std::uint64_t* device_morton_point_ids{};
  const void* device_nodes{};
  const Phase15MortonYao48DeviceTiledCandidateRecord*
      device_candidate_records{};
  const Phase15MortonYao48DeviceTiledPruneRegionRecord*
      device_prune_regions{};
  const Phase15MortonYao48DeviceTiledAnchorControl*
      device_anchor_controls{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t candidate_buffer_epoch{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t retained_coordinate_word_capacity{};
  std::size_t retained_morton_point_id_capacity{};
  std::size_t retained_node_capacity{};
  // Candidates are laid out in one fixed-stride segment per physical anchor.
  // Every control in authorized_anchor_control_extent authorizes this chunk's
  // fixed-stride segment; a chunk-ready control is intentionally publishable.
  std::size_t physical_candidate_record_capacity{};
  std::size_t candidate_segment_stride_records{};
  std::size_t physical_anchor_control_capacity{};
  std::size_t authorized_anchor_control_extent{};
  std::size_t anchor_control_stride_bytes{};
  std::size_t physical_device_arena_capacity_bytes{};
  std::size_t anchor_begin{};
  std::size_t anchor_end{};
  int cuda_device{-1};
  bool host_fake{false};
  bool source_device_views_retained{false};
  bool source_views_bound_to_snapshot_identity{false};
  bool ready{false};
};

class Phase15MortonYao48DeviceCandidateTilePrivateViewAccess final {
 public:
  [[nodiscard]] static
      Phase15MortonYao48DeviceCandidateTilePrivateViews
      inspect(
          const MortonYao48DeviceCandidateTileLease& lease) noexcept;
};

inline constexpr std::uint64_t
    phase15_morton_yao48_device_tiled_fnv_offset_basis =
        UINT64_C(14695981039346656037);
inline constexpr std::uint64_t
    phase15_morton_yao48_device_tiled_fnv_prime =
        UINT64_C(1099511628211);

inline void phase15_morton_yao48_device_tiled_hash_word(
    std::uint64_t& digest,
    std::uint64_t word) noexcept {
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= phase15_morton_yao48_device_tiled_fnv_prime;
  }
}

[[nodiscard]] inline std::uint64_t
phase15_morton_yao48_device_tiled_metadata_digest(
    const Phase15MortonYao48DeviceTiledBatch& batch) noexcept {
  std::uint64_t digest =
      phase15_morton_yao48_device_tiled_fnv_offset_basis;
  const auto hash_size = [&digest](std::size_t value) noexcept {
    phase15_morton_yao48_device_tiled_hash_word(
        digest, static_cast<std::uint64_t>(value));
  };
  hash_size(batch.host_anchor_controls.size());
  hash_size(batch.physical_candidate_capacity);
  hash_size(batch.physical_prune_region_capacity);
  hash_size(batch.physical_witness_bank_slot_capacity);
  hash_size(batch.physical_anchor_control_capacity);
  hash_size(batch.physical_anchor_checkpoint_capacity);
  hash_size(batch.physical_pending_anchor_count_capacity);
  hash_size(batch.physical_device_arena_capacity_bytes);
  hash_size(batch.anchor_control_device_to_host_count);
  hash_size(batch.anchor_control_device_to_host_byte_count);
  hash_size(batch.candidate_device_to_host_count);
  hash_size(batch.certified_prune_device_to_host_count);
  hash_size(batch.kernel_launch_count);
  hash_size(batch.synchronization_count);
  hash_size(batch.traversal_subdivision_count);
  hash_size(batch.maximum_traversal_subdivision_count_per_anchor);
  hash_size(batch.resume_control_device_to_host_count);
  hash_size(batch.resume_control_device_to_host_byte_count);
  phase15_morton_yao48_device_tiled_hash_word(
      digest, batch.source_snapshot_epoch);
  phase15_morton_yao48_device_tiled_hash_word(
      digest, batch.output_buffer_epoch);
  phase15_morton_yao48_device_tiled_hash_word(digest, batch.tile_epoch);
  phase15_morton_yao48_device_tiled_hash_word(digest, batch.chunk_sequence);
  phase15_morton_yao48_device_tiled_hash_word(
      digest, static_cast<std::uint64_t>(batch.cuda_device));
  phase15_morton_yao48_device_tiled_hash_word(
      digest, static_cast<std::uint64_t>(batch.execution_kind));
  for (const bool flag : {
           batch.fixed_anchor_segments_allocated,
           batch.output_owner_detached_for_tile_lifetime,
           batch.interval_cone_classification_requested,
           batch.ambiguous_cone_to_unbanked_candidate_requested,
           batch.target_tested_before_bank_insert_requested,
           batch.retained_witnesses_outside_pruned_subtree_requested,
           batch
               .nonnegative_diametral_witness_interval_lower_bound_requested,
           batch.censored_anchor_outputs_invalidated,
           batch.exact_diametral_rank_evaluated,
           batch.scientific_pair_catalog_published,
           batch.dense_pair_fallback_performed,
           batch.global_pair_matrix_materialized,
           batch.higher_order_structure_materialized,
           batch.cuda_execution_contract_satisfied,
           batch.resume_same_tile,
           batch.capacity_yield_resumable,
           batch.process_restart_resumable}) {
    phase15_morton_yao48_device_tiled_hash_word(
        digest, flag ? UINT64_C(1) : UINT64_C(0));
  }
  for (const Phase15MortonYao48DeviceTiledAnchorControl& control :
       batch.host_anchor_controls) {
    for (const std::uint64_t word : {
             control.anchor_morton_position,
             control.candidate_count,
             control.prune_region_count,
             control.certified_pruned_pair_mass,
             control.node_visit_count,
             control.cumulative_candidate_count,
             control.cumulative_prune_region_count,
             control.cumulative_certified_pruned_pair_mass,
             control.cumulative_node_visit_count,
             control.status,
             control.yield_reason,
             control.stop_reason,
             control.failure_code,
             control.ambiguous_cone_candidate_count,
             control.unbanked_candidate_count,
             control.cumulative_ambiguous_cone_candidate_count,
             control.cumulative_unbanked_candidate_count,
             control.unresolved_pair_mass,
             control.tile_epoch,
             control.chunk_sequence,
             control.reserved_zero}) {
      phase15_morton_yao48_device_tiled_hash_word(digest, word);
    }
  }
  return digest;
}

class Phase15MortonYao48DeviceTiledPairFrontierContextState final {
 public:
  Phase15MortonYao48DeviceTiledPairFrontierContextState() = default;
  ~Phase15MortonYao48DeviceTiledPairFrontierContextState() = default;

  Phase15MortonYao48DeviceTiledPairFrontierContextState(
      const Phase15MortonYao48DeviceTiledPairFrontierContextState&) = delete;
  Phase15MortonYao48DeviceTiledPairFrontierContextState& operator=(
      const Phase15MortonYao48DeviceTiledPairFrontierContextState&) = delete;

  template <typename Preflight, typename Operation>
  decltype(auto) with_launcher_section(
      Preflight&& preflight,
      Operation&& operation) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (poisoned_.load(std::memory_order_acquire)) {
      throw std::runtime_error(
          "the Phase 15 device tiled Morton/Yao48 context is poisoned by "
          "a prior launcher or validation failure");
    }
    // Expected caller-side backpressure is checked while holding the same
    // mutex as the launch, but before the poisoning boundary.  A live lease is
    // therefore a reversible precondition failure, never a launcher failure.
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
          "the Phase 15 device tiled Morton/Yao48 output epoch overflowed");
    }
    return ++epoch_;
  }

  [[nodiscard]] bool poisoned() const noexcept {
    return poisoned_.load(std::memory_order_acquire);
  }

  // The launcher owns the concrete type.  Access is serialized by
  // with_launcher_section(), so a resumed tile can retain one device arena
  // across calls without exposing CUDA types to the host contract.
  [[nodiscard]] std::shared_ptr<void>& device_resources() noexcept {
    return device_resources_;
  }

  [[nodiscard]] const std::shared_ptr<void>& device_resources() const noexcept {
    return device_resources_;
  }

 private:
  std::mutex mutex_;
  std::atomic<bool> poisoned_{false};
  std::uint64_t epoch_{};
  std::shared_ptr<void> device_resources_;
};

[[nodiscard]] Phase15MortonYao48DeviceTiledBatch
build_phase15_morton_yao48_device_tiled_pair_frontier_on_device(
    Phase15MortonYao48DeviceTiledPairFrontierContextState& context,
    const Phase15MortonYao48DeviceTiledAdoptedTraversal& traversal,
    const Phase15MortonYao48DeviceTiledRequest& request);

}  // namespace morsehgp3d::gpu::detail
