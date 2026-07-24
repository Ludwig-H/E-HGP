#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/morton_lbvh_build.hpp"

#include <array>
#include <climits>
#include <cmath>
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
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceBuildDecision;
using morsehgp3d::gpu::MortonLbvhDeviceBuildStopReason;
using morsehgp3d::gpu::test_support::
    FakePhase14MortonLbvhBuildConfiguration;
using morsehgp3d::gpu::test_support::
    FakePhase14MortonLbvhBuildCorruption;
using morsehgp3d::gpu::test_support::
    configure_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::gpu::test_support::
    fake_gpu_phase14_morton_bin_proposal_count;
using morsehgp3d::gpu::test_support::
    fake_gpu_phase14_morton_lbvh_last_point_capacity;
using morsehgp3d::gpu::test_support::
    fake_gpu_phase14_morton_lbvh_last_point_count;
using morsehgp3d::gpu::test_support::
    fake_gpu_phase14_morton_lbvh_snapshot_count;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::MortonLbvhSnapshotLeaf;
using morsehgp3d::spatial::MortonLbvhSnapshotNode;

static_assert(!std::is_copy_constructible_v<MortonLbvhBuildContext>);
static_assert(!std::is_copy_assignable_v<MortonLbvhBuildContext>);
static_assert(
    std::is_nothrow_move_constructible_v<MortonLbvhBuildContext>);
static_assert(
    std::is_nothrow_move_assignable_v<MortonLbvhBuildContext>);

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
    std::cerr << "FAIL: " << message
              << " (unexpected exception: " << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CertifiedPoint3 point(
    double x,
    double y,
    double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud collision_cloud() {
  const double close_to_zero = std::ldexp(1.0, -30);
  const std::array<CertifiedPoint3, 7> points{
      point(0.0, 0.0, 0.0),
      point(close_to_zero, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(-1.0, -1.0, -1.0),
      point(1.0, 1.0, 1.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

void check_matches_reference(
    const MortonLbvhIndex& actual,
    const MortonLbvhIndex& expected,
    const CanonicalPointCloud& cloud,
    const std::string& label) {
  check(
      actual.validated_for(cloud),
      label + " is validated for the original PointId namespace");
  check(
      std::vector<MortonLbvhSnapshotLeaf>{
          actual.leaves().begin(), actual.leaves().end()} ==
          std::vector<MortonLbvhSnapshotLeaf>{
              expected.leaves().begin(), expected.leaves().end()},
      label + " preserves the exact Morton/PointId order");
  check(
      actual.build_counters() == expected.build_counters(),
      label + " preserves every canonical build counter");
  check(
      actual.root_aabb() == expected.root_aabb(),
      label + " preserves the exact root AABB");
}

void test_exact_fake_snapshot_without_ambiguity() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = collision_cloud();
  const MortonLbvhIndex expected = MortonLbvhIndex::build(cloud);
  const std::size_t point_count = cloud.size();
  const std::size_t capacity = point_count + 3U;
  MortonLbvhBuildContext context{capacity};

  auto result = context.build(cloud);
  check(
      result.complete_certified_build(),
      "the fake two-pass snapshot closes the certified import");
  check(
      !result.cuda_qualified_build(),
      "the host fake never claims CUDA qualification");
  check(
      result.decision() == MortonLbvhDeviceBuildDecision::complete &&
          result.stop_reason() ==
              MortonLbvhDeviceBuildStopReason::none,
      "the exact fake reports a complete decision");
  check_matches_reference(
      result.certified_index(),
      expected,
      cloud,
      "the exact fake snapshot");

  const auto& audit = result.audit();
  const std::size_t axis_count = 3U * point_count;
  const std::size_t node_count = 2U * point_count - 1U;
  check(
      audit.maximum_point_count == capacity &&
          audit.maximum_axis_count == 3U * capacity &&
          audit.maximum_node_count == 2U * capacity - 1U &&
          audit.required_axis_count == axis_count &&
          audit.required_node_count == node_count,
      "the fake build reports fixed and active linear capacities");
  check(
      audit.host_snapshot_byte_capacity ==
              capacity * sizeof(MortonLbvhSnapshotLeaf) +
                  (2U * capacity - 1U) *
                      sizeof(MortonLbvhSnapshotNode) &&
          audit.required_snapshot_byte_count ==
              point_count * sizeof(MortonLbvhSnapshotLeaf) +
                  node_count * sizeof(MortonLbvhSnapshotNode),
      "the snapshot audit uses the 16-byte leaf and 80-byte node ABI");
  check(
      audit.device_bin_proposal_count == axis_count &&
          audit.device_unambiguous_axis_count == axis_count &&
          audit.device_ambiguous_axis_count == 0U &&
          audit.cpu_exact_fallback_axis_count == 0U &&
          audit.cpu_import_exact_morton_axis_recertification_count ==
              axis_count,
      "the audit separates compact ambiguity fallback from 3n import replay");
  check(
      audit.fixed_capacity_preflight_satisfied &&
          audit.every_ambiguous_axis_exactly_resolved &&
          audit.encoded_bin_extent_validated &&
          audit.stable_morton_point_id_order_validated &&
          audit.snapshot_import_certified &&
          !audit.cpu_reference_build_invoked &&
          !audit.gpu_execution_performed &&
          !audit.cuda_builder_qualified &&
          audit.fake_launcher_executed &&
          !audit.higher_order_delaunay_mosaic_materialized &&
          !audit.global_cell_or_coface_arena_materialized,
      "the fake audit is certified but makes no GPU or forbidden-structure claim");
  check(
      audit.device_kernel_launch_count == 0U &&
          audit.device_library_submission_count == 0U &&
          audit.device_synchronization_count == 0U &&
          audit.proposal_buffer_epoch == 1U &&
          audit.snapshot_buffer_epoch == 2U,
      "the host fake distinguishes launcher epochs from CUDA launches");
  check(
      audit.device_coordinate_byte_capacity == 24U * capacity &&
          audit.device_encoded_bin_byte_capacity == 12U * capacity &&
          audit.device_morton_code_double_buffer_byte_capacity ==
              16U * capacity &&
          audit.device_point_id_double_buffer_byte_capacity ==
              16U * capacity &&
          audit.device_leaf_byte_capacity == 16U * capacity &&
          audit.device_node_byte_capacity ==
              80U * (2U * capacity - 1U) &&
          audit.device_frontier_double_buffer_byte_capacity ==
              48U * capacity &&
          audit.device_level_schedule_byte_capacity ==
              8U * (2U * capacity - 1U) &&
          audit.device_control_byte_capacity == 32U &&
          audit.device_sort_temporary_byte_capacity == 0U &&
          audit.total_fixed_device_byte_capacity ==
              308U * capacity - 56U,
      "the fake exposes the exact fixed device capacity without inventing "
      "a CUB workspace");
  check(
      fake_gpu_phase14_morton_bin_proposal_count() == 1U &&
          fake_gpu_phase14_morton_lbvh_snapshot_count() == 1U &&
          fake_gpu_phase14_morton_lbvh_last_point_count() ==
              point_count &&
          fake_gpu_phase14_morton_lbvh_last_point_capacity() ==
              capacity,
      "the fake launcher observes one bounded two-pass build");
}

void test_one_exact_ambiguity_fallback() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = collision_cloud();
  const MortonLbvhIndex expected = MortonLbvhIndex::build(cloud);
  const std::size_t forced_axis_index = cloud.size() + 1U;
  configure_fake_gpu_phase14_morton_lbvh_build(
      FakePhase14MortonLbvhBuildConfiguration{
          FakePhase14MortonLbvhBuildCorruption::none,
          forced_axis_index});
  MortonLbvhBuildContext context{cloud.size()};

  auto result = context.build(cloud);
  check_matches_reference(
      result.certified_index(),
      expected,
      cloud,
      "the one-axis fallback snapshot");
  check(
      result.audit().device_ambiguous_axis_count == 1U &&
          result.audit().cpu_exact_fallback_axis_count == 1U &&
          result.audit().device_unambiguous_axis_count ==
              3U * cloud.size() - 1U,
      "one ambiguity triggers exactly one host exact quotient");
}

void test_capacity_refusal_does_not_launch() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = collision_cloud();
  MortonLbvhBuildContext context{cloud.size() - 1U};

  auto result = context.build(cloud);
  check(
      result.decision() ==
              MortonLbvhDeviceBuildDecision::capacity_exhausted &&
          result.stop_reason() ==
              MortonLbvhDeviceBuildStopReason::point_capacity &&
          !result.complete_certified_build() &&
          !result.audit().fixed_capacity_preflight_satisfied,
      "an undersized context refuses before any allocation or launch");
  check(
      fake_gpu_phase14_morton_bin_proposal_count() == 0U &&
          fake_gpu_phase14_morton_lbvh_snapshot_count() == 0U,
      "a capacity refusal does not enter either launcher");
  check_throws<std::logic_error>(
      [&result]() {
        static_cast<void>(result.certified_index());
      },
      "a capacity refusal exposes no index");
  check_throws<std::invalid_argument>(
      []() {
        MortonLbvhBuildContext oversized{
            static_cast<std::size_t>(INT_MAX) + 1U};
        static_cast<void>(oversized);
      },
      "a context above the CUB item-count domain is rejected before "
      "allocating proposal buffers");
}

void test_wrong_bin_is_rejected_and_poisons_context() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = collision_cloud();
  configure_fake_gpu_phase14_morton_lbvh_build(
      FakePhase14MortonLbvhBuildConfiguration{
          FakePhase14MortonLbvhBuildCorruption::wrong_unambiguous_bin});
  MortonLbvhBuildContext context{cloud.size()};

  check_throws<std::invalid_argument>(
      [&context, &cloud]() {
        static_cast<void>(context.build(cloud));
      },
      "the certified import rejects a plausible but wrong device bin");
  configure_fake_gpu_phase14_morton_lbvh_build(
      FakePhase14MortonLbvhBuildConfiguration{});
  check_throws<std::runtime_error>(
      [&context, &cloud]() {
        static_cast<void>(context.build(cloud));
      },
      "a malformed Morton proposal poisons its context");
}

void test_wrong_postorder_is_rejected_and_poisons_context() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = collision_cloud();
  configure_fake_gpu_phase14_morton_lbvh_build(
      FakePhase14MortonLbvhBuildConfiguration{
          FakePhase14MortonLbvhBuildCorruption::wrong_postorder_child});
  MortonLbvhBuildContext context{cloud.size()};

  check_throws<std::invalid_argument>(
      [&context, &cloud]() {
        static_cast<void>(context.build(cloud));
      },
      "the certified import rejects a non-postorder child");
  configure_fake_gpu_phase14_morton_lbvh_build(
      FakePhase14MortonLbvhBuildConfiguration{});
  check_throws<std::runtime_error>(
      [&context, &cloud]() {
        static_cast<void>(context.build(cloud));
      },
      "a malformed LBVH topology poisons its context");
}

void test_wrong_submission_count_is_rejected() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = collision_cloud();
  configure_fake_gpu_phase14_morton_lbvh_build(
      FakePhase14MortonLbvhBuildConfiguration{
          FakePhase14MortonLbvhBuildCorruption::
              wrong_snapshot_submission_count});
  MortonLbvhBuildContext context{cloud.size()};

  check_throws<std::runtime_error>(
      [&context, &cloud]() {
        static_cast<void>(context.build(cloud));
      },
      "a launcher cannot qualify with a falsified submission count");
}

}  // namespace

int main() {
  test_exact_fake_snapshot_without_ambiguity();
  test_one_exact_ambiguity_fallback();
  test_capacity_refusal_does_not_launch();
  test_wrong_bin_is_rejected_and_poisons_context();
  test_wrong_postorder_is_rejected_and_poisons_context();
  test_wrong_submission_count_is_rejected();

  if (failures != 0) {
    std::cerr << failures
              << " Phase 14 Morton LBVH build test(s) failed\n";
    return 1;
  }
  return 0;
}
