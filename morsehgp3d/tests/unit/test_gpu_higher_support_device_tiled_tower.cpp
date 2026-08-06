#include "fake_gpu_higher_support_device_tiled_frontier_launchers.hpp"
#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/higher_support_device_tiled_tower.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace {

using namespace morsehgp3d;
using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] ExactDirectMorseTowerBudget unlimited_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  ExactDirectMorseTowerBudget budget;
  budget.terminal.pair = {
      maximum, maximum, maximum, maximum, maximum, maximum, maximum};
  budget.terminal.higher = unlimited_higher_budget();
  budget.seeds = {maximum, maximum, maximum, maximum};
  return budget;
}

// 4-b1 differential: the device-tiled assembly -- the anchored session
// driven to terminal through the sealed M2 bridge, backed locally by the
// certified scientific fake -- must reproduce the exhaustive oracle's
// events, diagnostics, requirements and universe accounting under the
// oracle's own public canonical order.
void run_assembly_differential(
    std::span<const CertifiedPoint3> points,
    std::size_t requested_maximum_order,
    const std::string& label) {
  CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(points);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto oracle = build_exact_higher_support_stream(
      index, cloud, requested_maximum_order, unlimited_higher_budget());
  check(oracle.stream_complete(), label + ": the exhaustive oracle completes");

  gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  gpu::test_support::reset_fake_gpu_higher_support_device_tiled_frontier();
  gpu::test_support::bind_fake_higher_support_device_tiled_geometry(
      index, cloud);
  const auto assembly =
      gpu::assemble_exact_higher_support_stream_device_tiled(
          cloud,
          index,
          requested_maximum_order,
          unlimited_higher_budget());
  check(
      assembly.certified_assembled(),
      label + ": the device-tiled assembly reaches session terminal");
  if (!assembly.certified_assembled() || !oracle.stream_complete()) {
    return;
  }
  const auto& higher = *assembly.higher;
  check(
      higher.events == oracle.events,
      label + ": the assembled events equal the oracle in canonical order");
  check(
      higher.relevant_extra_shell_diagnostics ==
          oracle.relevant_extra_shell_diagnostics,
      label + ": the assembled diagnostics equal the oracle in canonical "
              "order");
  check(
      higher.requirements == oracle.requirements,
      label + ": the assembled requirements equal the oracle");
  check(
      higher.audit.exact_bigint_universe_certified &&
          higher.audit.total_support_count ==
              oracle.audit.total_support_count &&
          higher.audit.resolved_support_count ==
              oracle.audit.resolved_support_count &&
          higher.audit.remaining_frontier_support_count == 0U,
      label + ": the assembled universe accounting equals the oracle");
  check(
      higher.stream_complete() &&
          higher.absence_of_additional_higher_supports_certified(),
      label + ": the assembled result carries the completion certification");
}

// Sealed limitation of this increment: the terminal facade freshly re-runs
// the exhaustive stream and requires total result equality (including the
// prune-certificate payloads the assembly deliberately does not retain),
// so the device tower stops fail-closed at no_facade_rejected until the
// facade gains an anchored-session source kind -- the next increment.
void test_sealed_facade_limitation() {
  const std::array<CertifiedPoint3, 4U> tetrahedron{
      point(1.0, 1.0, 1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, 1.0, -1.0),
      point(1.0, -1.0, -1.0),
  };
  auto cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{tetrahedron});
  auto index = MortonLbvhIndex::build(cloud);
  gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  gpu::test_support::reset_fake_gpu_higher_support_device_tiled_frontier();
  gpu::test_support::bind_fake_higher_support_device_tiled_geometry(
      index, cloud);
  const auto device =
      gpu::build_exact_direct_morse_tower_from_cloud_device_tiled(
          std::move(cloud), std::move(index), 1U, unlimited_budget());
  check(
      !device.certified_tower() && !device.tower.has_value() &&
          device.receipt.pair_stream_complete &&
          device.receipt.higher_stream_complete &&
          device.receipt.decision ==
              ExactDirectMorseTowerDecision::no_facade_rejected,
      "the device tower stops fail-closed at the sealed facade limitation "
      "with both streams complete (decision=" +
          std::to_string(static_cast<int>(device.receipt.decision)) + ")");
}

void test_assembly_differentials() {
  const std::array<CertifiedPoint3, 4U> tetrahedron{
      point(1.0, 1.0, 1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, 1.0, -1.0),
      point(1.0, -1.0, -1.0),
  };
  run_assembly_differential(tetrahedron, 1U, "tetrahedron/K1");

  std::vector<CertifiedPoint3> line12;
  for (std::size_t index = 0U; index < 12U; ++index) {
    const double coordinate = static_cast<double>(index);
    line12.push_back(
        point(coordinate, coordinate / 8.0, coordinate / 64.0));
  }
  run_assembly_differential(line12, 2U, "line12/K2");

  const std::array<CertifiedPoint3, 8U> sphere{
      point(1.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.0, -1.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(0.0, 0.0, -1.0),
      point(0.5773502691896258, 0.5773502691896258, 0.5773502691896257),
      point(-0.5773502691896258, -0.5773502691896258, 0.5773502691896258),
  };
  run_assembly_differential(sphere, 3U, "sphere8/K3");
}

}  // namespace

int main() {
  try {
    test_assembly_differentials();
    test_sealed_facade_limitation();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: unexpected exception: " << error.what() << '\n';
    return 1;
  }
  if (failures != 0) {
    std::cerr << failures << " device-tiled tower checks failed\n";
    return 1;
  }
  std::cout << "device-tiled tower checks passed\n";
  return 0;
}
