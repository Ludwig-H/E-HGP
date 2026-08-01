#include "morsehgp3d/gpu/exact_higher_support_product_cuda.hpp"

#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/hierarchy/higher_support_product.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::test_support {
void corrupt_next_exact_higher_support_product_receipt() noexcept;
void reset_exact_higher_support_product_fake() noexcept;
[[nodiscard]] std::size_t
exact_higher_support_product_fake_launcher_call_count() noexcept;
}  // namespace morsehgp3d::gpu::test_support

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::ExactHigherSupportProductCudaBackend;
using morsehgp3d::gpu::ExactHigherSupportProductCudaContext;
using morsehgp3d::gpu::ExactHigherSupportProductCudaOutcome;
using morsehgp3d::gpu::ExactHigherSupportProductCudaStatus;
using morsehgp3d::gpu::ExactHigherSupportProductCudaStopReason;
using morsehgp3d::gpu::ExactHigherSupportProductCudaTask;
using morsehgp3d::gpu::ExactHigherSupportProductCudaTaskKind;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::gpu::test_support::
    corrupt_next_exact_higher_support_product_receipt;
using morsehgp3d::gpu::test_support::
    exact_higher_support_product_fake_launcher_call_count;
using morsehgp3d::gpu::test_support::
    reset_exact_higher_support_product_fake;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::hierarchy::
    ExactHigherSupportProductAabbDecisionBackend;
using morsehgp3d::hierarchy::ExactHigherSupportFrontierEntry;
using morsehgp3d::hierarchy::ExactHigherSupportNodeReceipt;
using morsehgp3d::hierarchy::
    exact_higher_support_product_no_well_centered_certified;
using morsehgp3d::hierarchy::
    exact_higher_support_product_query_strictly_inside_every_independent_sphere_certified;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactDyadicAabb3;
using morsehgp3d::spatial::MortonLbvhIndex;

static_assert(
    !std::is_copy_constructible_v<ExactHigherSupportProductCudaContext>);
static_assert(
    std::is_nothrow_move_constructible_v<
        ExactHigherSupportProductCudaContext>);
static_assert(
    !std::is_constructible_v<
        ExactHigherSupportProductCudaContext,
        MortonLbvhIndex&&,
        const CanonicalPointCloud&,
        MortonLbvhDeviceTraversalLease&&,
        std::size_t>);
static_assert(
    !std::is_constructible_v<
        ExactHigherSupportProductCudaContext,
        const MortonLbvhIndex&,
        CanonicalPointCloud&&,
        MortonLbvhDeviceTraversalLease&&,
        std::size_t>);

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

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud ordinary_cloud() {
  const std::array<CertifiedPoint3, 5> points{
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(0.0, 2.0, 0.0),
      point(0.0, 0.0, 2.0),
      point(1.0, 1.0, 1.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud wide_exponent_cloud() {
  const double denormal = std::numeric_limits<double>::denorm_min();
  const std::array<CertifiedPoint3, 3> points{
      point(denormal, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(1.0, 1.0, 0.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud positive_certificate_cloud() {
  const std::array<CertifiedPoint3, 7> points{
      point(-1.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, 2.0, 0.0),
      point(0.0, 0.75, 0.0),
      point(10.0, 0.0, 0.0),
      point(11.0, 0.0, 0.0),
      point(12.0, 0.0, 0.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] morsehgp3d::spatial::PointId canonical_point_id_for_source(
    const CanonicalPointCloud& cloud,
    std::size_t source_index) {
  for (std::size_t point_index = 0U;
       point_index < cloud.size();
       ++point_index) {
    const auto point_id =
        static_cast<morsehgp3d::spatial::PointId>(point_index);
    if (cloud.source_index(point_id) == source_index) {
      return point_id;
    }
  }
  throw std::logic_error("a test source point has no canonical PointId");
}

struct LeafNodeFixture {
  std::vector<std::uint64_t> node_index_by_leaf_position;
  std::vector<std::size_t> leaf_position_by_point_id;
};

// The imported LBVH contract fixes strict left/right postorder.  Replaying
// only its public Morton radix partition lets the test name individual leaf
// receipts; the production context still authenticates every resulting
// index/range against its private certified nodes.
[[nodiscard]] LeafNodeFixture individual_leaf_nodes(
    const MortonLbvhIndex& index) {
  const auto leaves = index.leaves();
  LeafNodeFixture fixture;
  fixture.node_index_by_leaf_position.resize(leaves.size());
  fixture.leaf_position_by_point_id.resize(leaves.size());
  std::size_t next_node_index = 0U;

  const auto find_split = [leaves](std::size_t begin, std::size_t end) {
    const std::uint64_t first_code = leaves[begin].morton_code;
    const std::uint64_t last_code = leaves[end - 1U].morton_code;
    if (first_code == last_code) {
      return begin + (end - begin) / 2U;
    }
    const std::uint64_t difference = first_code ^ last_code;
    const unsigned int highest_bit = static_cast<unsigned int>(
        std::bit_width(difference) - 1);
    const std::uint64_t mask = std::uint64_t{1} << highest_bit;
    std::size_t low = begin + 1U;
    std::size_t high = end;
    while (low < high) {
      const std::size_t middle = low + (high - low) / 2U;
      if ((leaves[middle].morton_code & mask) == 0U) {
        low = middle + 1U;
      } else {
        high = middle;
      }
    }
    if (low <= begin || low >= end) {
      throw std::logic_error("the test radix split did not divide its range");
    }
    return low;
  };

  std::function<void(std::size_t, std::size_t)> replay_postorder;
  replay_postorder = [&](std::size_t begin, std::size_t end) {
    if (end - begin == 1U) {
      if (!std::in_range<std::uint64_t>(next_node_index)) {
        throw std::length_error("a test leaf node index does not fit uint64");
      }
      fixture.node_index_by_leaf_position[begin] =
          static_cast<std::uint64_t>(next_node_index++);
      return;
    }
    const std::size_t split = find_split(begin, end);
    replay_postorder(begin, split);
    replay_postorder(split, end);
    ++next_node_index;
  };
  replay_postorder(0U, leaves.size());
  if (next_node_index != index.build_counters().node_count) {
    throw std::logic_error("the test postorder replay lost an LBVH node");
  }
  for (std::size_t position = 0U; position < leaves.size(); ++position) {
    const auto point_id = leaves[position].point_id;
    if (!std::in_range<std::size_t>(point_id) ||
        static_cast<std::size_t>(point_id) >= leaves.size()) {
      throw std::logic_error("the test LBVH leaf point id is out of range");
    }
    fixture.leaf_position_by_point_id[static_cast<std::size_t>(point_id)] =
        position;
  }
  return fixture;
}

[[nodiscard]] ExactHigherSupportNodeReceipt individual_leaf_receipt(
    const LeafNodeFixture& fixture,
    morsehgp3d::spatial::PointId point_id) {
  if (!std::in_range<std::size_t>(point_id) ||
      static_cast<std::size_t>(point_id) >=
          fixture.leaf_position_by_point_id.size()) {
    throw std::logic_error("a requested test leaf point id is out of range");
  }
  const std::size_t position =
      fixture.leaf_position_by_point_id[static_cast<std::size_t>(point_id)];
  return ExactHigherSupportNodeReceipt{
      fixture.node_index_by_leaf_position[position],
      static_cast<std::uint64_t>(position),
      static_cast<std::uint64_t>(position + 1U)};
}

template <std::size_t SupportSize>
[[nodiscard]] ExactHigherSupportFrontierEntry individual_leaf_product(
    const LeafNodeFixture& fixture,
    const std::array<morsehgp3d::spatial::PointId, SupportSize>& point_ids) {
  static_assert(SupportSize == 3U || SupportSize == 4U);
  std::array<ExactHigherSupportNodeReceipt, SupportSize> receipts{};
  for (std::size_t index = 0U; index < SupportSize; ++index) {
    receipts[index] = individual_leaf_receipt(fixture, point_ids[index]);
  }
  std::sort(
      receipts.begin(),
      receipts.end(),
      [](const auto& left, const auto& right) {
        return left.leaf_begin < right.leaf_begin;
      });
  ExactHigherSupportFrontierEntry product;
  product.support_size = static_cast<std::uint8_t>(SupportSize);
  product.group_count = static_cast<std::uint8_t>(SupportSize);
  for (std::size_t index = 0U; index < SupportSize; ++index) {
    product.groups[index] = {
        receipts[index].node_index,
        receipts[index].leaf_begin,
        receipts[index].leaf_end,
        1U};
  }
  return product;
}

[[nodiscard]] ExactHigherSupportFrontierEntry root_product(
    const MortonLbvhIndex& index,
    std::size_t point_count,
    std::uint8_t support_size) {
  ExactHigherSupportFrontierEntry product;
  product.support_size = support_size;
  product.group_count = 1U;
  product.groups[0].node_index =
      static_cast<std::uint64_t>(index.build_counters().node_count - 1U);
  product.groups[0].leaf_begin = 0U;
  product.groups[0].leaf_end = static_cast<std::uint64_t>(point_count);
  product.groups[0].multiplicity = support_size;
  return product;
}

[[nodiscard]] ExactHigherSupportNodeReceipt root_receipt(
    const MortonLbvhIndex& index,
    std::size_t point_count) {
  return ExactHigherSupportNodeReceipt{
      static_cast<std::uint64_t>(index.build_counters().node_count - 1U),
      0U,
      static_cast<std::uint64_t>(point_count)};
}

[[nodiscard]] ExactHigherSupportProductCudaTask support_task(
    const MortonLbvhIndex& index,
    std::size_t point_count,
    std::uint64_t epoch,
    std::uint64_t task_id) {
  ExactHigherSupportProductCudaTask task;
  task.task_id = task_id;
  task.source_snapshot_epoch = epoch;
  task.kind = ExactHigherSupportProductCudaTaskKind::support_prune;
  task.product = root_product(index, point_count, 3U);
  return task;
}

[[nodiscard]] ExactHigherSupportProductCudaTask query_task(
    const MortonLbvhIndex& index,
    std::size_t point_count,
    std::uint64_t epoch,
    std::uint64_t task_id) {
  ExactHigherSupportProductCudaTask task =
      support_task(index, point_count, epoch, task_id);
  task.kind =
      ExactHigherSupportProductCudaTaskKind::query_strict_interior;
  task.query_node = root_receipt(index, point_count);
  return task;
}

[[nodiscard]] std::array<ExactDyadicAabb3, 3> repeated_root_boxes(
    const MortonLbvhIndex& index) {
  return {index.root_aabb(), index.root_aabb(), index.root_aabb()};
}

void test_batched_parity_and_preflight() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  reset_exact_higher_support_product_fake();
  const CanonicalPointCloud cloud = ordinary_cloud();
  MortonLbvhBuildContext builder{cloud.size()};
  auto build = builder.build(cloud);
  const MortonLbvhIndex& index = build.certified_index();
  auto lease = builder.release_device_traversal_lease(build);
  const std::uint64_t epoch = lease.audit().source_snapshot_epoch;
  ExactHigherSupportProductCudaContext context{
      index, cloud, std::move(lease), 2U};

  const ExactHigherSupportProductCudaTask task =
      support_task(index, cloud.size(), epoch, 10U);
  const auto result = context.evaluate(
      std::span<const ExactHigherSupportProductCudaTask>{&task, 1U});
  check(
      result.complete() &&
          result.status ==
              ExactHigherSupportProductCudaStatus::complete_exact_batch &&
          result.stop_reason == ExactHigherSupportProductCudaStopReason::none,
      "the host fake completes one exact support-predicate batch");
  check(
      result.records.size() == 1U,
      "the exact batch publishes every record atomically");

  const auto boxes = repeated_root_boxes(index);
  ExactHigherSupportProductAabbDecisionBackend support_backend{};
  const bool expected_support =
      exact_higher_support_product_no_well_centered_certified(
          boxes, &support_backend);
  check(
      result.records[0].outcome ==
              (expected_support
                   ? ExactHigherSupportProductCudaOutcome::certified
                   : ExactHigherSupportProductCudaOutcome::fail_open),
      "the support task matches the existing exact P6a decision DAG");
  check(
      support_backend == ExactHigherSupportProductAabbDecisionBackend::
              bounded_dyadic_int1024 &&
          result.records[0].backend ==
              ExactHigherSupportProductCudaBackend::
                  bounded_dyadic_int1024 &&
          !result.records[0].cpu_fallback_performed,
      "ordinary dyadics stay on the existing exact int1024 host-fake path");

  const auto& audit = result.audit;
  check(
      audit.submitted_task_count == 1U &&
          audit.completed_task_count == 1U &&
          audit.support_prune_task_count == 1U &&
          audit.query_strict_interior_task_count == 0U &&
          audit.bounded_dyadic_int1024_count == 1U &&
          audit.arbitrary_precision_rational_fallback_count == 0U &&
          audit.launcher_call_count == 1U &&
          audit.source_snapshot_epoch == epoch,
      "the audit closes the exact local task partition");
  check(
      audit.source_authority_validated &&
          audit.resident_lease_index_identity_validated &&
          !audit.kernel_elapsed_ns_available &&
          audit.kernel_elapsed_ns == 0U &&
          audit.every_task_validated_before_launch &&
          audit.every_result_validated_once &&
          audit.output_published_atomically &&
          !audit.floating_point_decision_performed &&
          !audit.bigint_support_mass_transferred &&
          audit.host_fake_lifecycle_exercised &&
          !audit.cuda_execution_performed &&
          !audit.native_lbvh_nodes_read_on_device &&
          !audit.global_product_frontier_mutated &&
          !audit.ordinary_or_higher_order_delaunay_materialized &&
          !audit.global_cell_coface_or_incidence_arena_materialized &&
          !audit.hierarchy_or_tree_claimed && !audit.slo_claimed &&
          !audit.public_status_claimed,
      "the fake makes no FP64, CUDA, global-frontier or public-status claim");

  const ExactHigherSupportProductCudaTask overlapping_query =
      query_task(index, cloud.size(), epoch, 11U);
  const auto overlapping = context.evaluate(
      std::span<const ExactHigherSupportProductCudaTask>{
          &overlapping_query, 1U});
  check(
      overlapping.status ==
              ExactHigherSupportProductCudaStatus::invalid_batch &&
          overlapping.stop_reason ==
              ExactHigherSupportProductCudaStopReason::invalid_task &&
          overlapping.records.empty() &&
          overlapping.audit.launcher_call_count == 0U &&
          exact_higher_support_product_fake_launcher_call_count() == 1U,
      "a query node intersecting the support domain is rejected before "
      "launch");

  ExactHigherSupportProductCudaTask stale = task;
  ++stale.source_snapshot_epoch;
  const auto invalid = context.evaluate(
      std::span<const ExactHigherSupportProductCudaTask>{&stale, 1U});
  check(
      invalid.status == ExactHigherSupportProductCudaStatus::invalid_batch &&
          invalid.stop_reason ==
              ExactHigherSupportProductCudaStopReason::invalid_task &&
          invalid.records.empty() && invalid.audit.launcher_call_count == 0U &&
          exact_higher_support_product_fake_launcher_call_count() == 1U,
      "a stale epoch is rejected before launch with no partial output");

  std::array<ExactHigherSupportProductCudaTask, 3> over_capacity{
      task, overlapping_query, overlapping_query};
  over_capacity[2].task_id = 12U;
  const auto refused = context.evaluate(over_capacity);
  check(
      refused.status ==
              ExactHigherSupportProductCudaStatus::capacity_exhausted &&
          refused.stop_reason ==
              ExactHigherSupportProductCudaStopReason::task_capacity &&
          refused.records.empty() &&
          exact_higher_support_product_fake_launcher_call_count() == 1U,
      "capacity exhaustion invokes no launcher and publishes no records");
}

void test_positive_certificates_and_disjoint_query_receipt() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  reset_exact_higher_support_product_fake();
  const CanonicalPointCloud cloud = positive_certificate_cloud();
  MortonLbvhBuildContext builder{cloud.size()};
  auto build = builder.build(cloud);
  const MortonLbvhIndex& index = build.certified_index();
  const LeafNodeFixture leaf_nodes = individual_leaf_nodes(index);
  auto lease = builder.release_device_traversal_lease(build);
  const std::uint64_t epoch = lease.audit().source_snapshot_epoch;
  ExactHigherSupportProductCudaContext context{
      index, cloud, std::move(lease), 2U};

  ExactHigherSupportProductCudaTask support;
  support.task_id = 100U;
  support.source_snapshot_epoch = epoch;
  support.kind = ExactHigherSupportProductCudaTaskKind::support_prune;
  const std::array<morsehgp3d::spatial::PointId, 3> collinear_support{
      canonical_point_id_for_source(cloud, 4U),
      canonical_point_id_for_source(cloud, 5U),
      canonical_point_id_for_source(cloud, 6U)};
  support.product = individual_leaf_product(
      leaf_nodes, collinear_support);

  ExactHigherSupportProductCudaTask query;
  query.task_id = 101U;
  query.source_snapshot_epoch = epoch;
  query.kind =
      ExactHigherSupportProductCudaTaskKind::query_strict_interior;
  const std::array<morsehgp3d::spatial::PointId, 3> acute_support{
      canonical_point_id_for_source(cloud, 0U),
      canonical_point_id_for_source(cloud, 1U),
      canonical_point_id_for_source(cloud, 2U)};
  query.product = individual_leaf_product(
      leaf_nodes, acute_support);
  query.query_node = individual_leaf_receipt(
      leaf_nodes, canonical_point_id_for_source(cloud, 3U));

  const std::array<ExactHigherSupportProductCudaTask, 2> tasks{
      support, query};
  const auto result = context.evaluate(tasks);
  check(
      result.complete() && result.records.size() == 2U &&
          result.records[0].task_id == 100U &&
          result.records[0].kind ==
              ExactHigherSupportProductCudaTaskKind::support_prune &&
          result.records[0].outcome ==
              ExactHigherSupportProductCudaOutcome::certified &&
          result.records[0].backend ==
              ExactHigherSupportProductCudaBackend::
                  bounded_dyadic_int1024 &&
          !result.records[0].cpu_fallback_performed,
      "a collinear singleton support product exercises the positive exact "
      "prune branch");
  check(
      result.complete() && result.records.size() == 2U &&
          result.records[1].task_id == 101U &&
          result.records[1].kind ==
              ExactHigherSupportProductCudaTaskKind::
                  query_strict_interior &&
          result.records[1].outcome ==
              ExactHigherSupportProductCudaOutcome::certified &&
          result.records[1].backend ==
              ExactHigherSupportProductCudaBackend::
                  bounded_dyadic_int1024 &&
          !result.records[1].cpu_fallback_performed,
      "a disjoint interior singleton query exercises the positive exact "
      "rank-prune branch");
  check(
      result.audit.support_prune_task_count == 1U &&
          result.audit.query_strict_interior_task_count == 1U &&
          result.audit.certified_count == 2U &&
          result.audit.fail_open_count == 0U &&
          result.audit.bounded_dyadic_int1024_count == 2U &&
          result.audit.arbitrary_precision_rational_fallback_count == 0U &&
          result.audit.submitted_task_digest != 0U &&
          result.audit.completed_result_digest != 0U &&
          result.audit.launcher_call_count == 1U &&
          exact_higher_support_product_fake_launcher_call_count() == 1U,
      "positive support/query records close counters and both batch digests");
}

void test_foreign_resident_index_identity_is_rejected() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  reset_exact_higher_support_product_fake();
  const CanonicalPointCloud cloud = ordinary_cloud();
  MortonLbvhBuildContext first_builder{cloud.size()};
  MortonLbvhBuildContext second_builder{cloud.size()};
  auto first_build = first_builder.build(cloud);
  auto second_build = second_builder.build(cloud);
  const MortonLbvhIndex& first_index = first_build.certified_index();
  auto foreign_lease =
      second_builder.release_device_traversal_lease(second_build);
  check(
      foreign_lease.audit().source_index_identity_retained,
      "a traversal lease reports retention of its certified index token");
  check_throws<std::invalid_argument>(
      [&] {
        ExactHigherSupportProductCudaContext context{
            first_index, cloud, std::move(foreign_lease), 1U};
        static_cast<void>(context);
      },
      "an index and resident lease from distinct builds of the same cloud "
      "are rejected by shared identity before launch");
  check(
      exact_higher_support_product_fake_launcher_call_count() == 0U,
      "a foreign resident index identity never reaches the launcher");
}

void test_integral_bigint_fallback() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  reset_exact_higher_support_product_fake();
  const CanonicalPointCloud cloud = wide_exponent_cloud();
  MortonLbvhBuildContext builder{cloud.size()};
  auto build = builder.build(cloud);
  const MortonLbvhIndex& index = build.certified_index();
  auto lease = builder.release_device_traversal_lease(build);
  const std::uint64_t epoch = lease.audit().source_snapshot_epoch;
  ExactHigherSupportProductCudaContext context{
      index, cloud, std::move(lease), 1U};
  const ExactHigherSupportProductCudaTask task =
      support_task(index, cloud.size(), epoch, 20U);
  const auto result = context.evaluate(
      std::span<const ExactHigherSupportProductCudaTask>{&task, 1U});

  const auto boxes = repeated_root_boxes(index);
  ExactHigherSupportProductAabbDecisionBackend backend{};
  const bool expected =
      exact_higher_support_product_no_well_centered_certified(
          boxes, &backend);
  check(
      result.complete() && result.records.size() == 1U &&
          backend == ExactHigherSupportProductAabbDecisionBackend::
              arbitrary_precision_rational &&
          result.records[0].backend ==
              ExactHigherSupportProductCudaBackend::
                  arbitrary_precision_rational &&
          result.records[0].cpu_fallback_performed &&
          result.records[0].outcome ==
              (expected
                   ? ExactHigherSupportProductCudaOutcome::certified
                   : ExactHigherSupportProductCudaOutcome::fail_open) &&
          result.audit.arbitrary_precision_rational_fallback_count == 1U &&
          result.audit.bounded_dyadic_int1024_count == 0U,
      "a wide dyadic exponent range falls back integrally to the exact "
      "host rational DAG");
}

void test_malformed_receipt_is_atomic_and_poisoning() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  reset_exact_higher_support_product_fake();
  const CanonicalPointCloud cloud = ordinary_cloud();
  MortonLbvhBuildContext builder{cloud.size()};
  auto build = builder.build(cloud);
  const MortonLbvhIndex& index = build.certified_index();
  auto lease = builder.release_device_traversal_lease(build);
  const std::uint64_t epoch = lease.audit().source_snapshot_epoch;
  ExactHigherSupportProductCudaContext context{
      index, cloud, std::move(lease), 1U};
  const ExactHigherSupportProductCudaTask task =
      support_task(index, cloud.size(), epoch, 30U);
  corrupt_next_exact_higher_support_product_receipt();
  check_throws<std::logic_error>(
      [&] {
        static_cast<void>(context.evaluate(
            std::span<const ExactHigherSupportProductCudaTask>{&task, 1U}));
      },
      "a malformed receipt is rejected before result publication");
  check_throws<std::logic_error>(
      [&] {
        static_cast<void>(context.evaluate(
            std::span<const ExactHigherSupportProductCudaTask>{&task, 1U}));
      },
      "a launcher-integrity failure poisons the context");
  check(
      exact_higher_support_product_fake_launcher_call_count() == 1U,
      "a poisoned context never re-enters the launcher");
}

}  // namespace

int main() {
  test_batched_parity_and_preflight();
  test_positive_certificates_and_disjoint_query_receipt();
  test_foreign_resident_index_identity_is_rejected();
  test_integral_bigint_fallback();
  test_malformed_receipt_is_atomic_and_poisoning();
  if (failures != 0) {
    std::cerr << failures
              << " exact higher-support CUDA host-contract test(s) failed\n";
    return 1;
  }
  return 0;
}
