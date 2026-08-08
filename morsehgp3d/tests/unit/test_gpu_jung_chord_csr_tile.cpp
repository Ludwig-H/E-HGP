#include "fake_gpu_jung_chord_csr_tile_launchers.hpp"
#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/jung_chord_csr_tile.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cstddef>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::JungChordAnchor;
using morsehgp3d::gpu::JungChordAnchorTile;
using morsehgp3d::gpu::JungChordCsrTileConfig;
using morsehgp3d::gpu::JungChordCsrTileContext;
using morsehgp3d::gpu::JungChordCsrTileLease;
using morsehgp3d::gpu::JungChordCsrTileStatus;
using morsehgp3d::gpu::JungChordCsrTileStopReason;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::gpu::test_support::FakeJungChordCsrTileConfiguration;
using morsehgp3d::gpu::test_support::FakeJungChordCsrTileCorruption;
using morsehgp3d::gpu::test_support::
    configure_fake_gpu_jung_chord_csr_tile;
using morsehgp3d::gpu::test_support::
    fake_gpu_jung_chord_csr_tile_launch_count;
using morsehgp3d::gpu::test_support::reset_fake_gpu_jung_chord_csr_tile;
using morsehgp3d::gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::spatial::CanonicalPointCloud;

static_assert(!std::is_copy_constructible_v<JungChordCsrTileLease>);
static_assert(std::is_nothrow_move_constructible_v<JungChordCsrTileLease>);
static_assert(!std::is_copy_constructible_v<JungChordCsrTileContext>);
static_assert(std::is_nothrow_move_constructible_v<JungChordCsrTileContext>);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Exception, typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message << " (unexpected: " << error.what()
              << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CanonicalPointCloud cloud(std::size_t count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(count);
  for (std::size_t index = 0U; index < count; ++index) {
    const double value = static_cast<double>(index);
    points.push_back(CertifiedPoint3::from_binary64(
        value, value / 7.0, value * value / 29.0));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] MortonLbvhDeviceTraversalLease traversal(std::size_t count) {
  reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud points = cloud(count);
  MortonLbvhBuildContext builder{count + 2U};
  auto build = builder.build(points);
  return builder.release_device_traversal_lease(build);
}

[[nodiscard]] JungChordAnchorTile anchors() {
  return JungChordAnchorTile{
      "morton_window_proposal_only",
      2U,
      {{0U, 1U}, {0U, 2U}, {1U, 2U}}};
}

void success_and_backpressure() {
  reset_fake_gpu_jung_chord_csr_tile();
  configure_fake_gpu_jung_chord_csr_tile(FakeJungChordCsrTileConfiguration{
      {2U, 3U, 1U}, {5U, 7U, 4U}, {0U, 8U, 0U}, {1U, 3U, 1U}, {}});
  JungChordCsrTileContext context{
      traversal(8U), JungChordCsrTileConfig{4U, 11U, 8U, 32U, true}};
  auto result = context.classify(anchors());
  check(result.status == JungChordCsrTileStatus::tile_complete,
        "the fake tile should complete");
  check(result.stop_reason == JungChordCsrTileStopReason::none,
        "the completed tile should have no stop reason");
  check(result.tile.has_value() && result.tile->ready() &&
            result.tile->host_fake(),
        "the completed fake tile should publish a ready lease");
  check(result.audit.depth_rejected_anchor_count == 1U,
        "the certified c_e cutoff should reject one anchor");
  check(result.audit.required_chord_point_id_count == 3U &&
            result.audit.committed_chord_point_id_count == 3U,
        "the CSR count should exclude the depth-rejected anchor");
  check(result.audit.range_candidate_point_count_sum == 16U,
        "the range-candidate census should be aggregated");
  check(result.audit.quadratic_chord_pair_baseline_count == 1U &&
            !result.audit.quadratic_chord_pair_baseline_executed,
        "the quadratic baseline should be analytic and unexecuted");
  check(result.audit.diameter_lens_support_eligibility_flagged,
        "the support-eligibility sidecar should be declared");
  check_throws<std::logic_error>(
      [&] { static_cast<void>(context.classify(anchors())); },
      "an outstanding resident lease should apply backpressure");
  result.tile.reset();
  auto second = context.classify(anchors());
  check(second.status == JungChordCsrTileStatus::tile_complete,
        "classification should resume after lease release");
  check(fake_gpu_jung_chord_csr_tile_launch_count() == 2U,
        "only accepted tiles should reach the launcher");
}

void recoverable_capacity_and_validation() {
  reset_fake_gpu_jung_chord_csr_tile();
  configure_fake_gpu_jung_chord_csr_tile(FakeJungChordCsrTileConfiguration{
      {2U, 2U, 2U}, {}, {}, {}, {}});
  JungChordCsrTileContext context{
      traversal(8U), JungChordCsrTileConfig{3U, 11U, 8U, 4U, true}};
  auto capacity = context.classify(anchors());
  check(capacity.status == JungChordCsrTileStatus::capacity_exhausted &&
            capacity.stop_reason ==
                JungChordCsrTileStopReason::chord_point_id_capacity &&
            !capacity.tile.has_value() && !context.poisoned(),
        "chord capacity should withhold atomically and remain recoverable");
  auto malformed = anchors();
  std::swap(malformed.anchors[0], malformed.anchors[2]);
  auto rejected = context.classify(std::move(malformed));
  check(rejected.status == JungChordCsrTileStatus::malformed_input &&
            !rejected.tile.has_value(),
        "unsorted anchors should be rejected before launch");
}

void fatal_launcher_poisoning() {
  reset_fake_gpu_jung_chord_csr_tile();
  configure_fake_gpu_jung_chord_csr_tile(FakeJungChordCsrTileConfiguration{
      {}, {}, {}, {}, FakeJungChordCsrTileCorruption::count_emit_mismatch});
  JungChordCsrTileContext context{
      traversal(8U), JungChordCsrTileConfig{4U, 11U, 8U, 32U, true}};
  const auto failed = context.classify(anchors());
  check(failed.status == JungChordCsrTileStatus::execution_failure &&
            failed.stop_reason ==
                JungChordCsrTileStopReason::count_emit_mismatch &&
            context.poisoned(),
        "a count/emit mismatch should poison the context");
  check_throws<std::logic_error>(
      [&] { static_cast<void>(context.classify(anchors())); },
      "a poisoned context must refuse replay");
}

}  // namespace

int main() {
  success_and_backpressure();
  recoverable_capacity_and_validation();
  fatal_launcher_poisoning();
  if (failures != 0) {
    std::cerr << failures << " Jung-chord CSR unit assertion(s) failed\n";
    return 1;
  }
  std::cout << "Jung-chord CSR host contract tests passed\n";
  return 0;
}
