#include "fake_gpu_jung_chord_csr_tile_launchers.hpp"

#include "phase15_jung_chord_csr_tile_internal.hpp"

#include <atomic>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::gpu::test_support {
namespace {

std::mutex fake_mutex;
FakeJungChordCsrTileConfiguration fake_configuration;
std::atomic<std::size_t> fake_launch_count{};

}  // namespace

void configure_fake_gpu_jung_chord_csr_tile(
    FakeJungChordCsrTileConfiguration configuration) {
  std::lock_guard lock{fake_mutex};
  fake_configuration = std::move(configuration);
}

void reset_fake_gpu_jung_chord_csr_tile() noexcept {
  std::lock_guard lock{fake_mutex};
  fake_configuration = {};
  fake_launch_count.store(0U, std::memory_order_relaxed);
}

std::size_t fake_gpu_jung_chord_csr_tile_launch_count() noexcept {
  return fake_launch_count.load(std::memory_order_relaxed);
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {
namespace {

struct FakeJungChordOwner final {};

[[nodiscard]] std::uint64_t configured_value(
    const std::vector<std::uint64_t>& values,
    std::size_t index,
    std::uint64_t fallback) {
  return index < values.size() ? values[index] : fallback;
}

}  // namespace

Phase15JungChordCsrTileReceipt launch_phase15_jung_chord_csr_tile(
    const Phase15JungChordCsrTileRequest& request) {
  using test_support::FakeJungChordCsrTileCorruption;
  if (!request.host_fake || request.cuda_device != -1 ||
      request.device_coordinate_bits != nullptr ||
      request.device_morton_point_ids != nullptr ||
      request.device_nodes != nullptr) {
    throw std::invalid_argument(
        "the fake Jung-chord launcher rejected a native/mixed request");
  }
  test_support::FakeJungChordCsrTileConfiguration configuration;
  {
    std::lock_guard lock{test_support::fake_mutex};
    configuration = test_support::fake_configuration;
  }
  test_support::fake_launch_count.fetch_add(1U, std::memory_order_relaxed);
  Phase15JungChordCsrTileReceipt receipt;
  receipt.host_fake = true;
  receipt.anchor_controls.resize(request.anchors.size());
  std::uint64_t required = 0U;
  for (std::size_t index = 0U; index < request.anchors.size(); ++index) {
    auto& control = receipt.anchor_controls[index];
    control.chord_count = configured_value(
        configuration.chord_counts, index, index % 3U);
    control.range_candidate_count = configured_value(
        configuration.range_candidate_counts,
        index,
        control.chord_count + 2U);
    control.constant_interior_lower_bound = configured_value(
        configuration.constant_interior_counts, index, 0U);
    control.support_eligible_chords = configured_value(
        configuration.support_eligible_counts,
        index,
        control.chord_count);
    control.count_node_visits = control.range_candidate_count + 1U;
    control.emit_node_visits = control.count_node_visits;
    control.count_leaf_tests = control.range_candidate_count;
    control.emit_leaf_tests = control.count_leaf_tests;
    control.certified_active_chords = control.chord_count;
    control.flags = phase15_jung_chord_anchor_complete;
    if (control.constant_interior_lower_bound >
        request.config.maximum_relevant_closed_rank -
            request.config.support_size) {
      control.flags |= phase15_jung_chord_anchor_depth_rejected;
      control.chord_count = 0U;
      control.certified_active_chords = 0U;
      control.support_eligible_chords = 0U;
    }
    if (control.chord_count > UINT64_MAX - required) {
      throw std::overflow_error("fake Jung-chord count overflow");
    }
    required += control.chord_count;
  }
  if (required > std::numeric_limits<std::size_t>::max()) {
    throw std::overflow_error("fake Jung-chord size_t overflow");
  }
  receipt.required_chord_count = static_cast<std::size_t>(required);
  receipt.required_device_arena_bytes =
      request.anchors.size() * sizeof(JungChordAnchor) +
      (request.anchors.size() + 1U) * sizeof(std::uint64_t) +
      receipt.required_chord_count *
          (sizeof(spatial::PointId) + sizeof(std::uint8_t));
  receipt.physical_device_arena_bytes = receipt.required_device_arena_bytes;
  receipt.anchor_h2d_bytes = request.anchors.size() * sizeof(JungChordAnchor);
  receipt.control_d2h_bytes =
      request.anchors.size() * sizeof(Phase15JungChordAnchorControl);
  receipt.kernel_launches = 4U;
  receipt.library_submissions = 1U;
  receipt.synchronizations = 2U;
  if (!request.config.collect_per_anchor_diagnostics) {
    receipt.anchor_controls.clear();
    receipt.control_d2h_bytes = sizeof(Phase15JungChordDeviceControl);
  }
  if (configuration.corruption ==
      FakeJungChordCsrTileCorruption::execution_failure) {
    receipt.status = JungChordCsrTileStatus::execution_failure;
    receipt.stop_reason = JungChordCsrTileStopReason::traversal_failure;
    return receipt;
  }
  if (configuration.corruption ==
      FakeJungChordCsrTileCorruption::count_emit_mismatch) {
    receipt.status = JungChordCsrTileStatus::execution_failure;
    receipt.stop_reason = JungChordCsrTileStopReason::count_emit_mismatch;
    return receipt;
  }
  if (receipt.required_chord_count > request.config.chord_point_id_capacity) {
    receipt.status = JungChordCsrTileStatus::capacity_exhausted;
    receipt.stop_reason = JungChordCsrTileStopReason::chord_point_id_capacity;
    return receipt;
  }
  receipt.status = JungChordCsrTileStatus::tile_complete;
  receipt.stop_reason = JungChordCsrTileStopReason::none;
  receipt.committed_chord_count = receipt.required_chord_count;
  receipt.output_valid = true;
  receipt.output_owner = std::make_shared<FakeJungChordOwner>();
  return receipt;
}

Phase15JungChordCsrQualificationSnapshot
copy_phase15_jung_chord_csr_qualification_snapshot(
    const JungChordCsrTileLease&) {
  throw std::logic_error(
      "the host fake has no qualification payload snapshot");
}

}  // namespace morsehgp3d::gpu::detail
