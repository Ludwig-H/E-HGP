#pragma once

#include "morsehgp3d/gpu/jung_chord_csr_tile.hpp"

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <vector>

namespace morsehgp3d::gpu::detail {

inline constexpr std::uint8_t phase15_jung_chord_anchor_complete = 1U << 0U;
inline constexpr std::uint8_t phase15_jung_chord_anchor_depth_rejected =
    1U << 1U;
inline constexpr std::uint8_t phase15_jung_chord_anchor_degenerate = 1U << 2U;
inline constexpr std::uint8_t phase15_jung_chord_support_eligible = 1U << 0U;
inline constexpr std::uint8_t phase15_jung_chord_support_eligible_ambiguous =
    1U << 1U;

struct Phase15JungChordAnchorControl final {
  std::uint64_t chord_count{};
  std::uint64_t range_candidate_count{};
  std::uint64_t constant_interior_lower_bound{};
  std::uint64_t count_node_visits{};
  std::uint64_t emit_node_visits{};
  std::uint64_t count_leaf_tests{};
  std::uint64_t emit_leaf_tests{};
  std::uint64_t bulk_constant_interior_points{};
  std::uint64_t bulk_constant_exterior_points{};
  std::uint64_t range_pruned_points{};
  std::uint64_t certified_active_chords{};
  std::uint64_t ambiguous_active_chords{};
  std::uint64_t support_eligible_chords{};
  std::uint64_t support_eligible_ambiguous_chords{};
  std::uint8_t flags{};
};

struct Phase15JungChordDeviceControl final {
  std::uint64_t required_chord_count{};
  std::uint64_t emitted_chord_count{};
  std::uint32_t failure_code{};
  std::uint8_t commit_allowed{};
  std::uint8_t output_valid{};
};

struct Phase15JungChordCsrTileRequest final {
  JungChordCsrTileConfig config{};
  std::span<const JungChordAnchor> anchors;
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t tile_epoch{};
  const std::uint64_t* device_coordinate_bits{};
  const std::uint64_t* device_morton_point_ids{};
  const void* device_nodes{};
  int cuda_device{-1};
  bool host_fake{};
};

struct Phase15JungChordCsrTileReceipt final {
  JungChordCsrTileStatus status{JungChordCsrTileStatus::execution_failure};
  JungChordCsrTileStopReason stop_reason{
      JungChordCsrTileStopReason::traversal_failure};
  std::shared_ptr<void> output_owner;
  const JungChordAnchor* device_anchors{};
  const std::uint64_t* device_offsets{};
  const spatial::PointId* device_chord_point_ids{};
  const std::uint8_t* device_chord_flags{};
  const std::uint64_t* device_constant_interior_counts{};
  const std::uint8_t* device_anchor_flags{};
  std::vector<Phase15JungChordAnchorControl> anchor_controls;
  std::size_t required_chord_count{};
  std::size_t committed_chord_count{};
  std::size_t physical_device_arena_bytes{};
  std::size_t required_device_arena_bytes{};
  std::size_t retained_output_device_bytes{};
  std::size_t logical_output_device_bytes{};
  std::size_t configured_capacity_upper_bound_bytes{};
  std::size_t cub_scan_temporary_bytes{};
  std::size_t anchor_h2d_bytes{};
  std::size_t control_d2h_bytes{};
  std::size_t chord_d2h_bytes{};
  std::size_t kernel_launches{};
  std::size_t library_submissions{};
  std::size_t synchronizations{};
  std::uint64_t anchor_upload_ns{};
  std::uint64_t count_kernel_ns{};
  std::uint64_t scan_ns{};
  std::uint64_t capacity_preflight_ns{};
  std::uint64_t emit_kernel_ns{};
  std::uint64_t total_device_ns{};
  bool output_valid{};
  bool host_fake{};
  bool cuda_executed{};
};

struct Phase15JungChordCsrQualificationSnapshot final {
  std::vector<JungChordAnchor> anchors;
  std::vector<std::uint64_t> offsets;
  std::vector<spatial::PointId> chord_point_ids;
  std::vector<std::uint8_t> chord_flags;
  std::vector<std::uint64_t> constant_interior_counts;
  std::vector<std::uint8_t> anchor_flags;
};

// Private, in-process chaining ABI for the next resident shallow-support
// kernel.  It deliberately exposes no host payload and its pointers are valid
// only while `owner` remains alive.
struct Phase15JungChordCsrResidentView final {
  std::shared_ptr<void> owner;
  const JungChordAnchor* anchors{};
  const std::uint64_t* offsets{};
  const spatial::PointId* chord_point_ids{};
  const std::uint8_t* chord_flags{};
  const std::uint64_t* constant_interior_counts{};
  const std::uint8_t* anchor_flags{};
  const std::uint64_t* source_coordinate_bits{};
  const std::uint64_t* source_morton_point_ids{};
  const void* source_nodes{};
  std::size_t point_count{};
  std::size_t node_count{};
  std::size_t anchor_count{};
  std::size_t chord_count{};
  int cuda_device{-1};
};

class Phase15JungChordCsrTileContextState final {
 public:
  std::mutex mutex;
  std::atomic<bool> poisoned{false};
};

[[nodiscard]] Phase15JungChordCsrTileReceipt
launch_phase15_jung_chord_csr_tile(
    const Phase15JungChordCsrTileRequest& request);

[[nodiscard]] Phase15JungChordCsrQualificationSnapshot
copy_phase15_jung_chord_csr_qualification_snapshot(
    const JungChordCsrTileLease& lease);

class Phase15JungChordCsrTilePrivateViewAccess final {
 public:
  [[nodiscard]] static Phase15JungChordCsrResidentView resident_view(
      const JungChordCsrTileLease& lease) {
    if (!lease.cuda_resident()) {
      throw std::invalid_argument(
          "a Jung-chord resident view requires a native CUDA lease");
    }
    return Phase15JungChordCsrResidentView{
        lease.retained_owner_,
        lease.device_anchors_,
        lease.device_chord_offsets_,
        lease.device_chord_point_ids_,
        lease.device_chord_flags_,
        lease.device_constant_interior_counts_,
        lease.device_anchor_flags_,
        lease.source_device_coordinate_bits_,
        lease.source_device_morton_point_ids_,
        lease.source_device_nodes_,
        lease.audit_.point_count,
        lease.audit_.certified_node_count,
        lease.audit_.anchor_count,
        lease.audit_.committed_chord_point_id_count,
        lease.cuda_device_};
  }

  [[nodiscard]] static Phase15JungChordCsrQualificationSnapshot
  qualification_snapshot(JungChordCsrTileLease& lease) {
    if (!lease.ready() || lease.audit_.point_count > 32U) {
      throw std::invalid_argument(
          "the Jung-chord qualification snapshot is restricted to ready "
          "n<=32 leases");
    }
    auto snapshot = copy_phase15_jung_chord_csr_qualification_snapshot(lease);
    lease.audit_.qualification_snapshot_chord_count =
        snapshot.chord_point_ids.size();
    lease.audit_.chord_device_to_host_byte_count =
        snapshot.anchors.size() * sizeof(JungChordAnchor) +
        snapshot.offsets.size() * sizeof(std::uint64_t) +
        snapshot.chord_point_ids.size() * sizeof(spatial::PointId) +
        snapshot.chord_flags.size() * sizeof(std::uint8_t) +
        snapshot.constant_interior_counts.size() * sizeof(std::uint64_t) +
        snapshot.anchor_flags.size() * sizeof(std::uint8_t);
    lease.audit_.chord_device_to_host_performed = true;
    return snapshot;
  }
};

}  // namespace morsehgp3d::gpu::detail
