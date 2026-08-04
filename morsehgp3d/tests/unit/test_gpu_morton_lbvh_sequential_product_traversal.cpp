#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/exact_higher_support_product_cuda.hpp"
#include "morsehgp3d/gpu/exact_higher_support_stream_decision_adapter.hpp"
#include "morsehgp3d/gpu/exact_pair_block_transactional_frontier_resident_cuda.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace morsehgp3d::gpu::test_support {
void reset_exact_higher_support_product_fake() noexcept;
}  // namespace morsehgp3d::gpu::test_support

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::ExactHigherSupportProductCudaContext;
using morsehgp3d::gpu::
    ExactHigherSupportProductCudaPositiveDecisionAdapter;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierResidentCudaConfig;
using morsehgp3d::gpu::
    ExactPairBlockTransactionalFrontierResidentCudaContext;
using morsehgp3d::gpu::
    ExactPairBlockTransactionalFrontierResidentCudaStatus;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::gpu::
    MortonLbvhSequentialProductTraversalAuthority;
using morsehgp3d::gpu::test_support::
    fake_gpu_phase14_morton_lbvh_snapshot_count;
using morsehgp3d::gpu::test_support::
    fake_gpu_phase14_morton_lbvh_traversal_lease_release_count;
using morsehgp3d::gpu::test_support::
    reset_exact_higher_support_product_fake;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
using morsehgp3d::hierarchy::ExactHigherSupportTerminalRunStatus;
using morsehgp3d::hierarchy::ExactHigherSupportTerminalSession;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

static_assert(
    !std::is_default_constructible_v<
        MortonLbvhSequentialProductTraversalAuthority>);
static_assert(
    !std::is_copy_constructible_v<
        MortonLbvhSequentialProductTraversalAuthority>);
static_assert(
    !std::is_copy_assignable_v<
        MortonLbvhSequentialProductTraversalAuthority>);
static_assert(
    std::is_nothrow_move_constructible_v<
        MortonLbvhSequentialProductTraversalAuthority>);
static_assert(
    std::is_nothrow_move_assignable_v<
        MortonLbvhSequentialProductTraversalAuthority>);
static_assert(
    !std::is_constructible_v<
        MortonLbvhSequentialProductTraversalAuthority,
        MortonLbvhDeviceTraversalLease&>);

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

[[nodiscard]] CanonicalPointCloud source_cloud() {
  const std::array<CertifiedPoint3, 7> points{
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(0.0, 2.0, 0.0),
      point(0.0, 0.0, 2.0),
      point(1.0, 1.0, 1.0),
      point(-1.0, 1.0, 0.0),
      point(1.0, 0.0, -1.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
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

void test_one_build_and_release_feed_both_existing_consumers() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  reset_exact_higher_support_product_fake();
  const CanonicalPointCloud cloud = source_cloud();
  const std::size_t capacity = cloud.size() + 3U;
  MortonLbvhBuildContext builder{capacity};
  auto build = builder.build(cloud);
  const MortonLbvhIndex& index = build.certified_index();
  MortonLbvhDeviceTraversalLease source =
      builder.release_device_traversal_lease(build);
  const auto source_audit = source.audit();
  const ExactHigherSupportStreamBudget higher_budget =
      unlimited_higher_budget();
  ExactHigherSupportTerminalSession cpu_higher_session{
      index, cloud, 4U, higher_budget, 256U};
  check(
      cpu_higher_session.run_to_terminal() ==
          ExactHigherSupportTerminalRunStatus::terminal,
      "the CPU higher-support reference reaches its terminal checkpoint");
  auto cpu_higher_authority = std::move(cpu_higher_session).seal();

  MortonLbvhSequentialProductTraversalAuthority initial{
      std::move(source)};
  MortonLbvhSequentialProductTraversalAuthority authority{
      std::move(initial)};
  check(
      !source.ready() && !initial.ready() && authority.ready(),
      "the source lease and moved authority are neutralized exactly once");
  check(
      authority.pair_view_available() &&
          !authority.higher_support_view_available() &&
          !authority.complete(),
      "only the pair view is initially available");

  const auto initial_audit = authority.audit();
  check(
      initial_audit.retained_device_snapshot_count == 1U &&
          initial_audit.persistent_capacity_accounting_count == 1U &&
          initial_audit.device_snapshot_copy_count == 0U &&
          initial_audit.source_traversal_lease_consumption_count == 1U &&
          initial_audit.pair_view_emission_count == 0U &&
          initial_audit.higher_support_view_emission_count == 0U,
      "the authority accounts for one source snapshot and no copy");
  check(
      initial_audit.maximum_point_count ==
              source_audit.maximum_point_count &&
          initial_audit.maximum_node_count ==
              source_audit.maximum_node_count &&
          initial_audit.persistent_device_byte_capacity ==
              source_audit.persistent_device_byte_capacity &&
          initial_audit.source_snapshot_epoch ==
              source_audit.source_snapshot_epoch &&
          initial_audit.source_cloud_identity_retained &&
          initial_audit.source_index_identity_retained &&
          initial_audit.source_snapshot_epoch_retained &&
          initial_audit.immutable_device_views_preserved &&
          initial_audit.pair_then_higher_support_order_enforced,
      "capacity, epoch and both sealed identity authorities are retained");
  check(
      !initial_audit.raw_device_pointer_exposed &&
          !initial_audit.second_host_snapshot_retained &&
          !initial_audit.higher_order_delaunay_mosaic_materialized &&
          !initial_audit.global_cell_or_coface_arena_materialized &&
          !initial_audit.public_status_claimed,
      "the authority exposes no raw pointer, duplicate snapshot or product "
      "claim");

  check_throws<std::logic_error>(
      [&authority]() {
        static_cast<void>(authority.release_higher_support_view());
      },
      "the higher-support view is rejected before the pair view");

  MortonLbvhDeviceTraversalLease pair_view =
      authority.release_pair_view();
  check(
      pair_view.ready() && pair_view.audit() == source_audit,
      "the pair view preserves the complete immutable source audit");
  check_throws<std::logic_error>(
      [&authority]() {
        static_cast<void>(authority.release_pair_view());
      },
      "the pair view cannot be emitted twice");
  check_throws<std::logic_error>(
      [&authority]() {
        static_cast<void>(authority.release_higher_support_view());
      },
      "the higher-support view cannot overlap a live pair view");

  {
    auto pair_consumer =
        ExactPairBlockTransactionalFrontierResidentCudaContext::start(
            std::move(pair_view),
            index,
            cloud,
            ExactPairBlockTransactionalFrontierResidentCudaConfig{
                5U, 16U, 8U, 16U, 4U});
    check(
        pair_consumer.ready() && !pair_consumer.consumed(),
        "the existing resident pair context accepts the emitted rvalue");
    check(
        authority.audit().pair_view_outstanding &&
            !authority.higher_support_view_available(),
        "the pair context keeps the first view lifetime sealed");
    auto pair_cut = pair_consumer.run({});
    check(
        pair_cut.status() ==
                ExactPairBlockTransactionalFrontierResidentCudaStatus::
                    non_authoritative_host_fake_terminal &&
            pair_cut.complete() && pair_cut.validated_for(index, cloud) &&
            pair_cut.terminal_pairs().size() == 21U &&
            pair_cut.audit().source_snapshot_epoch ==
                source_audit.source_snapshot_epoch,
        "the first view closes the complete host-fake pair cut on the shared "
        "snapshot");
  }

  check(
      authority.higher_support_view_available() &&
          authority.audit().pair_view_lifetime_completed,
      "destroying the pair consumer unlocks the higher-support view");
  MortonLbvhDeviceTraversalLease higher_support_view =
      authority.release_higher_support_view();
  check(
      higher_support_view.ready() &&
          higher_support_view.audit() == source_audit,
      "the higher-support view preserves the same audit and epoch");
  check_throws<std::logic_error>(
      [&authority]() {
        static_cast<void>(authority.release_higher_support_view());
      },
      "the higher-support view cannot be emitted twice");

  {
    ExactHigherSupportProductCudaContext higher_support_consumer{
        index, cloud, std::move(higher_support_view), 64U};
    check(
        higher_support_consumer.host_fake() &&
            higher_support_consumer.bound_to(index, cloud) &&
            higher_support_consumer.source_snapshot_epoch() ==
                source_audit.source_snapshot_epoch,
        "the existing higher-support context accepts the second rvalue and "
        "retains the same binding");
    check(
        authority.audit().higher_support_view_outstanding,
        "the second consumer dominates the higher-support view lifetime");
    ExactHigherSupportProductCudaPositiveDecisionAdapter higher_adapter{
        higher_support_consumer, index, cloud};
    ExactHigherSupportTerminalSession assisted_higher_session{
        index,
        cloud,
        4U,
        higher_adapter.source(),
        higher_budget,
        256U};
    check(
        assisted_higher_session.run_to_terminal() ==
            ExactHigherSupportTerminalRunStatus::terminal,
        "the second view drives the CUDA-assisted sparse terminal stream");
    auto assisted_higher_authority =
        std::move(assisted_higher_session).seal();
    check(
        assisted_higher_authority.sealed_in_process_terminal_authority() &&
            assisted_higher_authority.terminal_checkpoint() ==
                cpu_higher_authority.terminal_checkpoint() &&
            assisted_higher_authority.segments().size() ==
                cpu_higher_authority.segments().size() &&
            std::equal(
                assisted_higher_authority.segments().begin(),
                assisted_higher_authority.segments().end(),
                cpu_higher_authority.segments().begin()) &&
            assisted_higher_authority.event_count() ==
                cpu_higher_authority.event_count() &&
            assisted_higher_authority
                    .relevant_extra_shell_diagnostic_count() ==
                cpu_higher_authority
                    .relevant_extra_shell_diagnostic_count(),
        "the shared higher view preserves every CPU checkpoint, segment and "
        "terminal record");
    const auto adapter_audit = higher_adapter.audit();
    check(
        adapter_audit.source_binding_validated &&
            !adapter_audit.native_exact_authority &&
            adapter_audit.host_fake_positive_proposals_require_cpu_replay &&
            adapter_audit.submitted_task_count > 0U &&
            adapter_audit.prefetched_task_count ==
                adapter_audit.submitted_task_count &&
            adapter_audit.on_demand_task_count == 0U &&
            adapter_audit.host_fake_positive_proposal_count ==
                adapter_audit.certified_positive_count &&
            adapter_audit.native_certified_positive_count == 0U &&
            !adapter_audit.disabled_after_failure &&
            !adapter_audit.floating_point_decision_performed &&
            !adapter_audit.global_product_frontier_mutated &&
            !adapter_audit.higher_order_delaunay_materialized &&
            !adapter_audit.hierarchy_or_public_status_claimed,
        "the host fake remains a replayed positive proposal while exercising "
        "the batched higher-support adapter");
  }

  const auto complete_audit = authority.audit();
  check(
      authority.complete() &&
          complete_audit.pair_view_emission_count == 1U &&
          complete_audit.higher_support_view_emission_count == 1U &&
          complete_audit.pair_view_lifetime_completed &&
          complete_audit.higher_support_view_lifetime_completed &&
          !complete_audit.pair_view_outstanding &&
          !complete_audit.higher_support_view_outstanding,
      "both unique sequential emissions close after their consumers");
  check(
      fake_gpu_phase14_morton_lbvh_snapshot_count() == 1U &&
          fake_gpu_phase14_morton_lbvh_traversal_lease_release_count() ==
              1U &&
          complete_audit.retained_device_snapshot_count == 1U &&
          complete_audit.persistent_capacity_accounting_count == 1U &&
          complete_audit.device_snapshot_copy_count == 0U,
      "one Morton build and one traversal release feed both consumers");
}

void test_foreign_pair_and_higher_support_bindings_are_rejected() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = source_cloud();
  // Same PointId/cloud namespace and shape, but a separately built LBVH
  // authority.  Size-only and cloud-only checks would incorrectly accept it.
  const MortonLbvhIndex other_index = MortonLbvhIndex::build(cloud);
  MortonLbvhBuildContext builder{cloud.size()};
  auto build = builder.build(cloud);
  MortonLbvhSequentialProductTraversalAuthority authority{
      builder.release_device_traversal_lease(build)};

  {
    MortonLbvhDeviceTraversalLease pair_view =
        authority.release_pair_view();
    check_throws<std::invalid_argument>(
        [&pair_view, &other_index, &cloud]() {
          static_cast<void>(
              ExactPairBlockTransactionalFrontierResidentCudaContext::start(
                  std::move(pair_view), other_index, cloud));
        },
        "the pair consumer rejects a foreign index on the same cloud");
  }
  check(
      authority.higher_support_view_available(),
      "a rejected foreign pair consumer releases its sealed view lifetime");

  {
    MortonLbvhDeviceTraversalLease higher_support_view =
        authority.release_higher_support_view();
    check_throws<std::invalid_argument>(
        [&higher_support_view, &other_index, &cloud]() {
          ExactHigherSupportProductCudaContext foreign_consumer{
              other_index,
              cloud,
              std::move(higher_support_view),
              1U};
          static_cast<void>(foreign_consumer);
        },
        "the higher-support consumer rejects the same foreign index");
  }
  check(
      authority.complete() &&
          authority.audit().higher_support_view_lifetime_completed,
      "foreign rejection cannot leave a duplicate or outstanding view");
}

void test_a_consumed_source_lease_cannot_seed_a_second_authority() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = source_cloud();
  MortonLbvhBuildContext builder{cloud.size()};
  auto build = builder.build(cloud);
  MortonLbvhDeviceTraversalLease source =
      builder.release_device_traversal_lease(build);
  MortonLbvhSequentialProductTraversalAuthority first{
      std::move(source)};
  check_throws<std::invalid_argument>(
      [&source]() {
        MortonLbvhSequentialProductTraversalAuthority second{
            std::move(source)};
        static_cast<void>(second);
      },
      "one traversal lease cannot seed two sequential authorities");
  check(
      first.ready() && first.pair_view_available(),
      "the failed duplicate consume leaves the first authority intact");
}

}  // namespace

int main() {
  test_one_build_and_release_feed_both_existing_consumers();
  test_foreign_pair_and_higher_support_bindings_are_rejected();
  test_a_consumed_source_lease_cannot_seed_a_second_authority();

  if (failures != 0) {
    std::cerr << failures
              << " sequential Morton traversal authority test(s) failed\n";
    return 1;
  }
  return 0;
}
