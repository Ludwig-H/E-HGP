#include "fake_gpu_jung_center_cover_prune_mass_launchers.hpp"
#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"
#include "jung_center_cover_prune_mass_receipt_oracle.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/gpu/jung_center_cover_prune_mass.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
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
using morsehgp3d::gpu::JungCenterCoverPruneMassConfig;
using morsehgp3d::gpu::JungCenterCoverPruneMassContext;
using morsehgp3d::gpu::JungCenterCoverPruneMassStatus;
using morsehgp3d::gpu::JungCenterCoverQualificationReceipt;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::gpu::detail::JungCenterCoverExactReceiptBox;
using morsehgp3d::gpu::detail::JungCenterCoverPatchDecodeStatus;
using morsehgp3d::gpu::detail::decode_jung_center_cover_receipt_patch;
using morsehgp3d::gpu::detail::
    jung_center_cover_patch_cover_violation_count;
using morsehgp3d::gpu::jung_center_cover_minimum_stack_capacity;
using morsehgp3d::gpu::test_support::FakeJungCenterCoverConfiguration;
using morsehgp3d::gpu::test_support::FakeJungCenterCoverCorruption;
using morsehgp3d::gpu::test_support::
    configure_fake_gpu_jung_center_cover_prune_mass;
using morsehgp3d::gpu::test_support::
    fake_gpu_jung_center_cover_prune_mass_launch_count;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_jung_center_cover_prune_mass;
using morsehgp3d::gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::spatial::CanonicalPointCloud;

static_assert(!std::is_copy_constructible_v<JungCenterCoverPruneMassContext>);
static_assert(std::is_nothrow_move_constructible_v<
              JungCenterCoverPruneMassContext>);
static_assert(jung_center_cover_minimum_stack_capacity == 255U);
static_assert(
    morsehgp3d::gpu::jung_center_cover_prune_mass_schema_version == 2U);
static_assert(morsehgp3d::gpu::jung_center_cover_axis_count == 3U);
static_assert(
    morsehgp3d::gpu::jung_center_cover_prune_mass_phase ==
    "P15-HOCUDA-P1a");
static_assert(sizeof(JungCenterCoverQualificationReceipt) == 14'152U);
static_assert(alignof(JungCenterCoverQualificationReceipt) == 8U);
static_assert(
    offsetof(JungCenterCoverQualificationReceipt, block) == 0U);
static_assert(
    offsetof(
        JungCenterCoverQualificationReceipt,
        patch_bounds_valid_mask) == 40U);
static_assert(
    offsetof(
        JungCenterCoverQualificationReceipt,
        patch_lower_binary64_bits) == 64U);
static_assert(
    offsetof(
        JungCenterCoverQualificationReceipt,
        patch_upper_binary64_bits) == 1'600U);
static_assert(
    offsetof(
        JungCenterCoverQualificationReceipt,
        witness_node_counts) == 3'136U);
static_assert(
    offsetof(JungCenterCoverQualificationReceipt, disposition) == 14'144U);

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

[[nodiscard]] JungCenterCoverQualificationReceipt cartesian_patch_receipt() {
  JungCenterCoverQualificationReceipt receipt;
  receipt.patch_bounds_valid_mask = UINT64_MAX;
  for (std::size_t patch_index = 0U;
       patch_index < morsehgp3d::gpu::jung_center_cover_patch_count;
       ++patch_index) {
    for (std::size_t axis = 0U;
         axis < morsehgp3d::gpu::jung_center_cover_axis_count;
         ++axis) {
      const std::size_t ordinal =
          (patch_index >> (2U * axis)) & 3U;
      const std::size_t offset =
          patch_index * morsehgp3d::gpu::jung_center_cover_axis_count + axis;
      receipt.patch_lower_binary64_bits[offset] =
          std::bit_cast<std::uint64_t>(static_cast<double>(ordinal));
      receipt.patch_upper_binary64_bits[offset] =
          std::bit_cast<std::uint64_t>(
              static_cast<double>(ordinal + 1U));
    }
  }
  return receipt;
}

void exact_receipt_patch_validation() {
  auto receipt = cartesian_patch_receipt();
  std::array<
      JungCenterCoverExactReceiptBox,
      morsehgp3d::gpu::jung_center_cover_patch_count> patches{};
  bool decoded = true;
  for (std::size_t patch_index = 0U; patch_index < patches.size();
       ++patch_index) {
    decoded = decoded &&
        decode_jung_center_cover_receipt_patch(
            receipt, patch_index, patches[patch_index]) ==
            JungCenterCoverPatchDecodeStatus::valid;
  }
  JungCenterCoverExactReceiptBox required_cover;
  for (std::size_t axis = 0U;
       axis < morsehgp3d::gpu::jung_center_cover_axis_count;
       ++axis) {
    required_cover.lower[axis] =
        morsehgp3d::exact::ExactRational::from_binary64(0.0);
    required_cover.upper[axis] =
        morsehgp3d::exact::ExactRational::from_binary64(4.0);
  }
  check(decoded &&
            jung_center_cover_patch_cover_violation_count(
                patches, required_cover) == 0U,
        "a complete Cartesian patch receipt should cover the exact box");

  auto absent = receipt;
  absent.patch_bounds_valid_mask &= ~UINT64_C(1);
  check(
      decode_jung_center_cover_receipt_patch(absent, 0U, patches[0U]) ==
          JungCenterCoverPatchDecodeStatus::absent,
      "an uncomputed fail-open patch must remain distinguishable");

  auto non_finite = receipt;
  non_finite.patch_lower_binary64_bits[0U] =
      UINT64_C(0x7ff0000000000000);
  check(
      decode_jung_center_cover_receipt_patch(
          non_finite, 0U, patches[0U]) ==
          JungCenterCoverPatchDecodeStatus::invalid,
      "a non-finite reported patch bound must be rejected");

  auto nan_bound = receipt;
  nan_bound.patch_upper_binary64_bits[0U] =
      UINT64_C(0x7ff8000000000001);
  check(
      decode_jung_center_cover_receipt_patch(
          nan_bound, 0U, patches[0U]) ==
          JungCenterCoverPatchDecodeStatus::invalid,
      "a NaN reported patch bound must be rejected");

  auto subnormal = receipt;
  subnormal.patch_lower_binary64_bits[0U] = UINT64_C(0x0000000000000001);
  subnormal.patch_upper_binary64_bits[0U] = UINT64_C(0x0000000000000002);
  check(
      decode_jung_center_cover_receipt_patch(
          subnormal, 0U, patches[0U]) ==
          JungCenterCoverPatchDecodeStatus::valid,
      "finite subnormal patch bounds must remain replayable");

  auto inverted = receipt;
  inverted.patch_lower_binary64_bits[0U] =
      std::bit_cast<std::uint64_t>(2.0);
  inverted.patch_upper_binary64_bits[0U] =
      std::bit_cast<std::uint64_t>(1.0);
  check(
      decode_jung_center_cover_receipt_patch(inverted, 0U, patches[0U]) ==
          JungCenterCoverPatchDecodeStatus::invalid,
      "an inverted reported patch interval must be rejected");

  auto gap = receipt;
  for (std::size_t patch_index = 0U; patch_index < patches.size();
       ++patch_index) {
    if ((patch_index & 3U) == 1U) {
      const std::size_t offset =
          patch_index * morsehgp3d::gpu::jung_center_cover_axis_count;
      gap.patch_lower_binary64_bits[offset] =
          std::bit_cast<std::uint64_t>(1.5);
    }
    check(
        decode_jung_center_cover_receipt_patch(
            gap, patch_index, patches[patch_index]) ==
            JungCenterCoverPatchDecodeStatus::valid,
        "the gap fixture must retain individually valid intervals");
  }
  check(
      jung_center_cover_patch_cover_violation_count(
          patches, required_cover) != 0U,
      "a gap between adjacent patch slabs must be rejected");

  auto narrow = receipt;
  for (std::size_t patch_index = 0U; patch_index < patches.size();
       ++patch_index) {
    if ((patch_index & 3U) == 0U) {
      const std::size_t offset =
          patch_index * morsehgp3d::gpu::jung_center_cover_axis_count;
      narrow.patch_lower_binary64_bits[offset] =
          std::bit_cast<std::uint64_t>(0.5);
    }
    check(
        decode_jung_center_cover_receipt_patch(
            narrow, patch_index, patches[patch_index]) ==
            JungCenterCoverPatchDecodeStatus::valid,
        "the narrow fixture must retain individually valid intervals");
  }
  check(
      jung_center_cover_patch_cover_violation_count(
          patches, required_cover) != 0U,
      "an inward outer patch bound must be rejected");
}

void complete_profile_and_receipt_mass() {
  reset_fake_gpu_jung_center_cover_prune_mass();
  JungCenterCoverQualificationReceipt receipt;
  receipt.block.unordered_pair_mass = 3U;
  receipt.patch_bounds_valid_mask = UINT64_MAX;
  receipt.covered_patch_mask = 1U;
  receipt.patch_lower_binary64_bits[0U] = UINT64_C(0xbff0000000000000);
  receipt.patch_upper_binary64_bits[0U] = UINT64_C(0x3ff0000000000000);
  receipt.patch_lower_binary64_bits.back() =
      UINT64_C(0x4000000000000000);
  receipt.patch_upper_binary64_bits.back() =
      UINT64_C(0x4008000000000000);
  receipt.witness_node_counts[0U] = 2U;
  receipt.witness_point_masses[0U] = 9U;
  receipt.witness_node_point_masses[0U] = 4U;
  receipt.witness_node_point_masses[1U] = 5U;
  receipt.disposition =
      morsehgp3d::gpu::JungCenterCoverTerminalDisposition::pruned;
  JungCenterCoverQualificationReceipt microtile_receipt;
  microtile_receipt.block.unordered_pair_mass = 25U;
  microtile_receipt.patch_bounds_valid_mask = UINT64_MAX;
  microtile_receipt.patch_lower_binary64_bits[0U] =
      UINT64_C(0xc000000000000000);
  microtile_receipt.patch_upper_binary64_bits.back() =
      UINT64_C(0x4010000000000000);
  microtile_receipt.disposition =
      morsehgp3d::gpu::JungCenterCoverTerminalDisposition::microtile;
  configure_fake_gpu_jung_center_cover_prune_mass(
      FakeJungCenterCoverConfiguration{
          3U, {receipt, microtile_receipt}, {}});
  JungCenterCoverPruneMassContext context{
      traversal(8U), JungCenterCoverPruneMassConfig{8U, 25U, 255U, true}};
  const auto result = context.profile_once();
  check(result.status == JungCenterCoverPruneMassStatus::complete,
        "the fake center-cover profile should complete");
  check(result.audit.unordered_pair_universe_mass == 28U &&
            result.audit.pruned_pair_mass == 3U &&
            result.audit.microtile_pair_mass == 25U &&
            result.audit.exact_mass_identity_satisfied,
        "the fake should close pruned plus microtile mass exactly");
  check(result.audit.multi_cta_scheduler_used &&
            result.audit.worker_cta_with_work_count >= 2U &&
            result.audit.private_per_shard_dfs_used &&
            result.audit.one_terminal_synchronization_used,
        "the scheduler and synchronization invariants should be explicit");
  check(!result.audit.anchor_or_pair_payload_emitted &&
            !result.audit.endpoint_pair_array_materialized &&
            !result.audit.higher_order_delaunay_mosaic_materialized &&
            !result.audit.slo_eligible,
        "the profiler must not claim or materialize a pair/product path");
  check(result.qualification_receipts.size() == 2U &&
            result.qualification_receipts[0U].witness_point_masses[0U] == 9U &&
            result.qualification_receipts[0U]
                    .witness_node_point_masses[1U] == 5U &&
            result.qualification_receipts[0U]
                    .patch_lower_binary64_bits[0U] ==
                UINT64_C(0xbff0000000000000) &&
            result.qualification_receipts[0U]
                    .patch_upper_binary64_bits.back() ==
                UINT64_C(0x4008000000000000) &&
            result.qualification_receipts[1U]
                    .patch_lower_binary64_bits[0U] ==
                UINT64_C(0xc000000000000000) &&
            result.audit.qualification_patch_bounds_recorded,
        "n<=32 receipts must retain exact patch bounds and witness masses");
  check_throws<std::logic_error>(
      [&] { static_cast<void>(context.profile_once()); },
      "the profiler context should be single-use");
  check(fake_gpu_jung_center_cover_prune_mass_launch_count() == 1U,
        "only the accepted profile should reach the launcher");
}

void structural_stack_gate_and_receipt_bound() {
  reset_fake_gpu_jung_center_cover_prune_mass();
  check_throws<std::invalid_argument>(
      [&] {
        JungCenterCoverPruneMassContext context{
            traversal(8U),
            JungCenterCoverPruneMassConfig{8U, 4U, 254U, false}};
      },
      "a stack below 2H+1 must be rejected before launch");
  check_throws<std::invalid_argument>(
      [&] {
        JungCenterCoverPruneMassContext context{
            traversal(33U),
            JungCenterCoverPruneMassConfig{8U, 4U, 255U, true}};
      },
      "qualification receipts must be rejected above n=32");
  check(fake_gpu_jung_center_cover_prune_mass_launch_count() == 0U,
        "host validation failures must not reach the launcher");
}

void fatal_mass_mismatch_poisoning() {
  reset_fake_gpu_jung_center_cover_prune_mass();
  configure_fake_gpu_jung_center_cover_prune_mass(
      FakeJungCenterCoverConfiguration{
          0U, {}, FakeJungCenterCoverCorruption::mass_mismatch});
  JungCenterCoverPruneMassContext context{traversal(8U)};
  const auto failed = context.profile_once();
  check(failed.status == JungCenterCoverPruneMassStatus::execution_failure &&
            context.poisoned() &&
            !failed.audit.exact_mass_identity_satisfied,
        "a forged mass identity must fail and poison the context");
}

void incomplete_or_oversized_receipts_fail_closed() {
  reset_fake_gpu_jung_center_cover_prune_mass();
  JungCenterCoverQualificationReceipt oversized;
  oversized.block.unordered_pair_mass = 28U;
  oversized.patch_bounds_valid_mask = UINT64_MAX;
  oversized.disposition =
      morsehgp3d::gpu::JungCenterCoverTerminalDisposition::microtile;
  configure_fake_gpu_jung_center_cover_prune_mass(
      FakeJungCenterCoverConfiguration{0U, {oversized}, {}});
  JungCenterCoverPruneMassContext oversized_context{
      traversal(8U), JungCenterCoverPruneMassConfig{8U, 4U, 255U, true}};
  const auto oversized_result = oversized_context.profile_once();
  check(oversized_result.status ==
            JungCenterCoverPruneMassStatus::execution_failure &&
            oversized_context.poisoned() &&
            oversized_result.qualification_receipts.empty() &&
            !oversized_result.audit.qualification_receipts_collected,
        "a microtile receipt above the configured mass must fail closed");

  reset_fake_gpu_jung_center_cover_prune_mass();
  configure_fake_gpu_jung_center_cover_prune_mass(
      FakeJungCenterCoverConfiguration{
          0U,
          {},
          FakeJungCenterCoverCorruption::missing_qualification_receipt});
  JungCenterCoverPruneMassContext missing_context{
      traversal(8U), JungCenterCoverPruneMassConfig{8U, 28U, 255U, true}};
  const auto missing_result = missing_context.profile_once();
  check(missing_result.status ==
            JungCenterCoverPruneMassStatus::execution_failure &&
            missing_context.poisoned(),
        "an incomplete n<=32 receipt partition must fail closed");
}

}  // namespace

int main() {
  exact_receipt_patch_validation();
  complete_profile_and_receipt_mass();
  structural_stack_gate_and_receipt_bound();
  fatal_mass_mismatch_poisoning();
  incomplete_or_oversized_receipts_fail_closed();
  if (failures != 0) {
    std::cerr << failures << " Jung center-cover assertion(s) failed\n";
    return 1;
  }
  std::cout << "Jung center-cover host contract tests passed\n";
  return 0;
}
