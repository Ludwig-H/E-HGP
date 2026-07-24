#pragma once

#include "morsehgp3d/spatial/lbvh.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::detail {

inline constexpr std::uint32_t phase14_morton_bin_value_mask =
    (std::uint32_t{1}
     << spatial::morton_lbvh_snapshot_morton_bits_per_axis) -
    std::uint32_t{1};
inline constexpr std::uint32_t phase14_morton_bin_ambiguous_bit =
    std::uint32_t{1} << 31U;
inline constexpr std::uint32_t phase14_morton_bin_allowed_mask =
    phase14_morton_bin_ambiguous_bit | phase14_morton_bin_value_mask;

enum class Phase14MortonLbvhExecutionKind : std::uint8_t {
  host_fake,
  cuda,
};

struct Phase14MortonBinProposalBatch {
  // Axis-major: encoded_bins[axis * point_count + point_id].
  // A record is either a 21-bit certified candidate bin or exactly the
  // ambiguity bit, in which case the host performs one exact fallback.
  std::vector<std::uint32_t> encoded_bins;
  std::size_t axis_count{};
  std::size_t host_to_device_coordinate_byte_count{};
  std::size_t device_to_host_encoded_bin_byte_count{};
  // Project-owned kernel launches and opaque library submissions are kept
  // separate: a CUB call may choose more than one implementation kernel.
  std::size_t kernel_launch_count{};
  std::size_t library_submission_count{};
  std::size_t synchronization_count{};
  std::uint64_t buffer_epoch{};
  Phase14MortonLbvhExecutionKind execution_kind{
      Phase14MortonLbvhExecutionKind::host_fake};
  bool cuda_path_qualified{false};
};

struct Phase14MortonLbvhSnapshotBatch {
  std::uint64_t point_count{};
  std::uint64_t root_node_index{
      spatial::morton_lbvh_snapshot_invalid_node_index};
  spatial::MortonLbvhSnapshotCounters proposed_counters{};
  std::vector<spatial::MortonLbvhSnapshotLeaf> leaves;
  std::vector<spatial::MortonLbvhSnapshotNode> nodes;
  std::size_t host_to_device_morton_code_byte_count{};
  std::size_t device_to_host_leaf_byte_count{};
  std::size_t device_to_host_node_byte_count{};
  // Fixed resident capacities, not merely active transfer payloads.  The
  // double-buffer and double-frontier fields already include both arenas.
  std::size_t device_coordinate_byte_capacity{};
  std::size_t device_encoded_bin_byte_capacity{};
  std::size_t device_morton_code_double_buffer_byte_capacity{};
  std::size_t device_point_id_double_buffer_byte_capacity{};
  std::size_t device_leaf_byte_capacity{};
  std::size_t device_node_byte_capacity{};
  std::size_t device_frontier_double_buffer_byte_capacity{};
  std::size_t device_level_schedule_byte_capacity{};
  std::size_t device_control_byte_capacity{};
  std::size_t sort_temporary_byte_capacity{};
  std::size_t total_fixed_device_byte_capacity{};
  // Counts only kernels named in this translation unit.
  std::size_t kernel_launch_count{};
  // Counts opaque CUB calls, never their implementation-private kernels.
  std::size_t library_submission_count{};
  std::size_t synchronization_count{};
  std::uint64_t buffer_epoch{};
  Phase14MortonLbvhExecutionKind execution_kind{
      Phase14MortonLbvhExecutionKind::host_fake};
  bool cuda_path_qualified{false};
};

class Phase14MortonLbvhBuildContextState final {
 public:
  Phase14MortonLbvhBuildContextState() = default;
  ~Phase14MortonLbvhBuildContextState() = default;

  Phase14MortonLbvhBuildContextState(
      const Phase14MortonLbvhBuildContextState&) = delete;
  Phase14MortonLbvhBuildContextState& operator=(
      const Phase14MortonLbvhBuildContextState&) = delete;

  template <typename Operation>
  decltype(auto) with_gpu_section(Operation&& operation) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (poisoned_.load(std::memory_order_acquire)) {
      throw std::runtime_error(
          "the Phase 14 Morton LBVH build context is poisoned by a prior "
          "device or validation failure");
    }
    try {
      return std::forward<Operation>(operation)();
    } catch (...) {
      poisoned_.store(true, std::memory_order_release);
      throw;
    }
  }

  [[nodiscard]] std::shared_ptr<void>& cuda_resources() noexcept {
    return cuda_resources_;
  }

  [[nodiscard]] std::uint64_t advance_epoch() {
    if (epoch_ == std::numeric_limits<std::uint64_t>::max()) {
      throw std::overflow_error(
          "the Phase 14 Morton LBVH build buffer epoch overflowed");
    }
    return ++epoch_;
  }

  // Valid only while the caller owns with_gpu_section().
  [[nodiscard]] std::uint64_t current_epoch() const noexcept {
    return epoch_;
  }

 private:
  std::mutex mutex_;
  std::shared_ptr<void> cuda_resources_;
  std::atomic<bool> poisoned_{false};
  std::uint64_t epoch_{};
};

[[nodiscard]] Phase14MortonBinProposalBatch
propose_phase14_morton_bins_on_gpu(
    Phase14MortonLbvhBuildContextState& context,
    std::span<const std::uint64_t> axis_major_coordinate_bits,
    std::size_t point_count,
    const std::array<std::uint64_t, 3>& lower_coordinate_bits,
    const std::array<std::uint64_t, 3>& upper_coordinate_bits,
    std::size_t maximum_point_count);

// certified_morton_codes is indexed by PointId.  A stable code-only radix
// sort therefore has the exact (Morton code, PointId) order.  The launcher
// must emit the current find_split tree in strict left/right postorder.
[[nodiscard]] Phase14MortonLbvhSnapshotBatch
build_phase14_morton_lbvh_snapshot_on_gpu(
    Phase14MortonLbvhBuildContextState& context,
    std::span<const std::uint64_t> axis_major_coordinate_bits,
    std::size_t point_count,
    std::span<const std::uint64_t> certified_morton_codes,
    std::size_t maximum_point_count);

}  // namespace morsehgp3d::gpu::detail
