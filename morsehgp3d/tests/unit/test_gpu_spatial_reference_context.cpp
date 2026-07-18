#include "fake_gpu_spatial_reference_launchers.hpp"

#include "morsehgp3d/gpu/spatial_reference.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <barrier>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::gpu::QueryCoordinateProjection;
using morsehgp3d::gpu::SpatialReferenceAudit;
using morsehgp3d::gpu::SpatialReferenceContext;
using morsehgp3d::gpu::test_support::FakeSpatialProposalConfiguration;
using morsehgp3d::gpu::test_support::FakeSpatialProposalCorruption;
using morsehgp3d::gpu::test_support::FakeSpatialProposalPermutation;
using morsehgp3d::gpu::test_support::FakeSpatialProposalValues;
using morsehgp3d::gpu::test_support::configure_fake_gpu_spatial_reference;
using morsehgp3d::gpu::test_support::fake_gpu_spatial_last_point_count;
using morsehgp3d::gpu::test_support::fake_gpu_spatial_last_query_bits;
using morsehgp3d::gpu::test_support::fake_gpu_spatial_launch_count;
using morsehgp3d::gpu::test_support::reset_fake_gpu_spatial_reference;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ClosedBallPartition;
using morsehgp3d::spatial::ExclusionSet;
using morsehgp3d::spatial::PointId;
using morsehgp3d::spatial::TopKPartition;
using morsehgp3d::spatial::brute_force_closed_ball;
using morsehgp3d::spatial::brute_force_top_k;

static_assert(!std::is_copy_constructible_v<SpatialReferenceContext>);
static_assert(!std::is_copy_assignable_v<SpatialReferenceContext>);
static_assert(std::is_nothrow_move_constructible_v<SpatialReferenceContext>);
static_assert(std::is_nothrow_move_assignable_v<SpatialReferenceContext>);

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

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactRational3 origin() {
  return ExactRational3{};
}

template <std::size_t Size>
[[nodiscard]] ExclusionSet exclusion_set(
    const CanonicalPointCloud& cloud,
    const std::array<PointId, Size>& ids,
    std::size_t run_m_star = Size) {
  return ExclusionSet::from_ids(
      std::span<const PointId>{ids}, cloud, run_m_star);
}

[[nodiscard]] ExclusionSet empty_exclusions(
    const CanonicalPointCloud& cloud) {
  return exclusion_set(cloud, std::array<PointId, 0>{});
}

template <typename Range>
[[nodiscard]] std::vector<PointId> materialize_ids(const Range& range) {
  return std::vector<PointId>{range.begin(), range.end()};
}

template <typename Range>
void check_ids(
    const Range& actual,
    std::initializer_list<PointId> expected,
    const std::string& message) {
  check(
      materialize_ids(actual) == std::vector<PointId>{expected},
      message);
}

void check_top_k_equal(
    const TopKPartition& actual,
    const TopKPartition& expected,
    const std::string& label) {
  check(
      actual.requested_rank() == expected.requested_rank(),
      label + " requested rank");
  check(
      actual.cutoff_squared_distance() ==
          expected.cutoff_squared_distance(),
      label + " exact cutoff");
  check(
      actual.strict_below().size() == expected.strict_below().size(),
      label + " strict-below cardinality");
  const std::size_t common_size =
      std::min(actual.strict_below().size(), expected.strict_below().size());
  for (std::size_t index = 0U; index < common_size; ++index) {
    check(
        actual.strict_below()[index].point_id ==
            expected.strict_below()[index].point_id &&
            actual.strict_below()[index].squared_distance ==
                expected.strict_below()[index].squared_distance,
        label + " strict-below entry " + std::to_string(index));
  }
  check(
      materialize_ids(actual.cutoff_shell_ids()) ==
          materialize_ids(expected.cutoff_shell_ids()),
      label + " complete cutoff shell");
  check(
      materialize_ids(actual.canonical_choice_ids()) ==
          materialize_ids(expected.canonical_choice_ids()),
      label + " canonical choice");
  check(
      actual.eligible_point_count() == expected.eligible_point_count(),
      label + " eligible count");
  check(
      actual.query_counters() == expected.query_counters(),
      label + " exact CPU counters");
  check(actual.shell_complete(), label + " complete result marker");
}

void check_closed_ball_equal(
    const ClosedBallPartition& actual,
    const ClosedBallPartition& expected,
    const std::string& label) {
  check(
      actual.squared_radius() == expected.squared_radius(),
      label + " exact radius");
  check(
      materialize_ids(actual.interior_ids()) ==
          materialize_ids(expected.interior_ids()),
      label + " interior");
  check(
      materialize_ids(actual.shell_ids()) ==
          materialize_ids(expected.shell_ids()),
      label + " shell");
  check(
      materialize_ids(actual.exterior_ids()) ==
          materialize_ids(expected.exterior_ids()),
      label + " exterior");
  check(actual.closed_rank() == expected.closed_rank(), label + " closed rank");
  check(
      actual.query_counters() == expected.query_counters(),
      label + " exact CPU counters");
  check(actual.partition_complete(), label + " complete result marker");
}

void check_audit_closure(
    const SpatialReferenceAudit& audit,
    std::size_t point_count,
    std::size_t cpu_exact_count,
    std::size_t finite_count,
    std::size_t infinite_count,
    const std::string& label) {
  check(
      audit.gpu_input_point_count == point_count &&
          audit.gpu_output_record_count == point_count &&
          audit.gpu_unique_point_id_count == point_count,
      label + " exhaustive GPU namespace counts");
  check(
      audit.gpu_finite_distance_proposal_count == finite_count &&
          audit.gpu_infinite_distance_proposal_count == infinite_count &&
          audit.gpu_nan_distance_proposal_count == 0U,
      label + " diagnostic proposal classification");
  check(audit.gpu_launch_count == 1U, label + " one proposal launch");
  check(audit.all_points_enumerated, label + " permutation closure");
  check(
      audit.cpu_exact_distance_evaluation_count == cpu_exact_count &&
          audit.cpu_exact_recertification_complete,
      label + " exact CPU recertification closure");
}

[[nodiscard]] std::array<CertifiedPoint3, 6> axis_tie_points() {
  return {
      point(0.0, 0.0, 1.0),
      point(1.0, 0.0, 0.0),
      point(0.0, -1.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.0, 0.0, -1.0),
  };
}

void test_false_and_reversed_proposals_do_not_decide_ties() {
  reset_fake_gpu_spatial_reference();
  const CanonicalPointCloud cloud = canonical_cloud(axis_tie_points());
  const ExclusionSet exclusions = empty_exclusions(cloud);
  const TopKPartition expected =
      brute_force_top_k(cloud, origin(), 3U, exclusions);
  SpatialReferenceContext context{cloud};

  configure_fake_gpu_spatial_reference(FakeSpatialProposalConfiguration{
      FakeSpatialProposalPermutation::canonical,
      FakeSpatialProposalValues::ascending_by_point_id,
      FakeSpatialProposalCorruption::none});
  const auto ascending = context.top_k(cloud, origin(), 3U, exclusions);

  configure_fake_gpu_spatial_reference(FakeSpatialProposalConfiguration{
      FakeSpatialProposalPermutation::reversed,
      FakeSpatialProposalValues::descending_by_point_id,
      FakeSpatialProposalCorruption::none});
  const auto reversed = context.top_k(cloud, origin(), 3U, exclusions);

  check_top_k_equal(
      ascending.exact_partition, expected,
      "ascending false proposals preserve the exact result");
  check_top_k_equal(
      reversed.exact_partition, expected,
      "reversed false proposals preserve the exact result");
  check(
      ascending.exact_partition.cutoff_shell_ids().size() == cloud.size(),
      "all six exact co-minimizers survive false proposal ordering");
  check_audit_closure(
      ascending.audit, cloud.size(), cloud.size(), cloud.size(), 0U,
      "ascending proposal audit");
  check_audit_closure(
      reversed.audit, cloud.size(), cloud.size(), cloud.size(), 0U,
      "reversed proposal audit");
  check(
      ascending.audit.buffer_epoch == 1U &&
          reversed.audit.buffer_epoch == 2U,
      "resident proposal buffers expose monotone epochs");
  check(
      fake_gpu_spatial_launch_count() == 2U &&
          fake_gpu_spatial_last_point_count() == cloud.size(),
      "both deliberately false proposal batches scan every point");
}

void test_exclusions_remain_host_only() {
  reset_fake_gpu_spatial_reference();
  const CanonicalPointCloud cloud = canonical_cloud(axis_tie_points());
  const std::array<PointId, 2> excluded_ids{PointId{0}, PointId{5}};
  const ExclusionSet exclusions = exclusion_set(cloud, excluded_ids);
  const TopKPartition expected =
      brute_force_top_k(cloud, origin(), 2U, exclusions);
  SpatialReferenceContext context{cloud};
  configure_fake_gpu_spatial_reference(FakeSpatialProposalConfiguration{
      FakeSpatialProposalPermutation::reversed,
      FakeSpatialProposalValues::positive_infinity,
      FakeSpatialProposalCorruption::none});

  const auto result = context.top_k(cloud, origin(), 2U, exclusions);
  check_top_k_equal(
      result.exact_partition, expected,
      "host exclusions preserve the exact eligible partition");
  check(
      result.exact_partition.eligible_point_count() == 4U &&
          result.exact_partition.query_counters().excluded_point_count == 2U,
      "the exact host query accounts for both exclusions");
  check_audit_closure(
      result.audit, cloud.size(), 4U, 0U, cloud.size(),
      "host-only exclusion audit");
  check(
      fake_gpu_spatial_last_point_count() == cloud.size(),
      "GPU proposals enumerate excluded points as well as eligible points");
}

void test_proposal_digest_is_record_order_invariant() {
  reset_fake_gpu_spatial_reference();
  const CanonicalPointCloud cloud = canonical_cloud(axis_tie_points());
  const ExclusionSet exclusions = empty_exclusions(cloud);
  SpatialReferenceContext context{cloud};

  configure_fake_gpu_spatial_reference(FakeSpatialProposalConfiguration{
      FakeSpatialProposalPermutation::canonical,
      FakeSpatialProposalValues::actual_binary64_recipe,
      FakeSpatialProposalCorruption::none});
  const auto canonical = context.nearest(cloud, origin(), exclusions);
  configure_fake_gpu_spatial_reference(FakeSpatialProposalConfiguration{
      FakeSpatialProposalPermutation::reversed,
      FakeSpatialProposalValues::actual_binary64_recipe,
      FakeSpatialProposalCorruption::none});
  const auto reversed = context.nearest(cloud, origin(), exclusions);

  check(
      canonical.audit.proposal_digest_fnv1a ==
          reversed.audit.proposal_digest_fnv1a,
      "proposal digest is keyed by PointId rather than copy-back order");
  check_top_k_equal(
      canonical.exact_partition,
      reversed.exact_partition,
      "record-order invariant exact decision");
}

void test_closed_ball_is_a_global_exact_partition() {
  reset_fake_gpu_spatial_reference();
  const std::array<CertifiedPoint3, 5> points{
      point(2.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(-2.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const ExactLevel radius{BigInt{1}};
  const ClosedBallPartition expected =
      brute_force_closed_ball(cloud, origin(), radius);
  SpatialReferenceContext context{cloud};
  configure_fake_gpu_spatial_reference(FakeSpatialProposalConfiguration{
      FakeSpatialProposalPermutation::reversed,
      FakeSpatialProposalValues::all_zero,
      FakeSpatialProposalCorruption::none});

  const auto result = context.closed_ball(cloud, origin(), radius);
  check_closed_ball_equal(
      result.exact_partition, expected,
      "global closed-ball CPU recertification");
  check_ids(
      result.exact_partition.interior_ids(), {PointId{2}},
      "the exact unit ball has the origin in its interior");
  check_ids(
      result.exact_partition.shell_ids(), {PointId{1}, PointId{3}},
      "the exact unit ball has both unit points on its shell");
  check_ids(
      result.exact_partition.exterior_ids(), {PointId{0}, PointId{4}},
      "the exact unit ball has both radius-two points outside");
  check_audit_closure(
      result.audit, cloud.size(), cloud.size(), cloud.size(), 0U,
      "global closed-ball audit");
}

void test_exact_rounded_and_underflowed_query_projection() {
  reset_fake_gpu_spatial_reference();
  const std::array<CertifiedPoint3, 3> points{
      point(-1.0, 0.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const ExclusionSet exclusions = empty_exclusions(cloud);
  SpatialReferenceContext context{cloud};

  const ExactRational3 exact_query{
      BigInt{1}, BigInt{-2}, BigInt{0}, BigInt{2}};
  const auto exact_result = context.nearest(cloud, exact_query, exclusions);
  check(
      exact_result.audit.query_projection ==
          std::array<QueryCoordinateProjection, 3>{
              QueryCoordinateProjection::exact,
              QueryCoordinateProjection::exact,
              QueryCoordinateProjection::exact},
      "binary-rational query coordinates project exactly");
  check(
      exact_result.audit.projected_query_bits ==
          std::array<std::uint64_t, 3>{
              std::bit_cast<std::uint64_t>(0.5),
              std::bit_cast<std::uint64_t>(-1.0),
              std::bit_cast<std::uint64_t>(0.0)},
      "exact projection publishes the expected binary64 words");

  const ExactRational3 rounded_query{
      BigInt{1}, BigInt{0}, BigInt{0}, BigInt{3}};
  const auto rounded_result =
      context.nearest(cloud, rounded_query, exclusions);
  check(
      rounded_result.audit.query_projection[0] ==
              QueryCoordinateProjection::rounded &&
          rounded_result.audit.query_projection[1] ==
              QueryCoordinateProjection::exact &&
          rounded_result.audit.query_projection[2] ==
              QueryCoordinateProjection::exact,
      "one third is visibly rounded without changing the exact CPU query");
  check(
      rounded_result.audit.projected_query_bits[0] ==
          std::bit_cast<std::uint64_t>(1.0 / 3.0),
      "one-third projection uses nearest-even binary64");

  const BigInt underflow_denominator = BigInt{1} << 1075U;
  const ExactRational3 underflow_query{
      BigInt{1}, BigInt{0}, BigInt{0}, underflow_denominator};
  const auto underflow_result =
      context.nearest(cloud, underflow_query, exclusions);
  check(
      underflow_result.audit.query_projection[0] ==
              QueryCoordinateProjection::underflow &&
          underflow_result.audit.projected_query_bits[0] == 0U,
      "a positive half-minimum-subnormal proposal underflows visibly to zero");
  check(
      underflow_result.audit.cpu_exact_recertification_complete,
      "underflowed proposal coordinates still receive exact CPU decisions");
  check(
      fake_gpu_spatial_last_query_bits() ==
          underflow_result.audit.projected_query_bits,
      "the fake launcher receives exactly the audited projection words");
  check(
      exact_result.audit.buffer_epoch == 1U &&
          rounded_result.audit.buffer_epoch == 2U &&
          underflow_result.audit.buffer_epoch == 3U &&
          fake_gpu_spatial_launch_count() == 3U,
      "all three projection classes reuse one resident proposal context");
}

void test_move_and_point_namespace_contract() {
  reset_fake_gpu_spatial_reference();
  const auto points = axis_tie_points();
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const CanonicalPointCloud namespace_alias = cloud;
  const CanonicalPointCloud independent_cloud = canonical_cloud(points);
  const ExclusionSet exclusions = empty_exclusions(cloud);
  const ExclusionSet alias_exclusions = empty_exclusions(namespace_alias);
  const ExclusionSet independent_exclusions =
      empty_exclusions(independent_cloud);

  SpatialReferenceContext original{cloud};
  SpatialReferenceContext moved{std::move(original)};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(original.nearest(cloud, origin(), exclusions));
      },
      "a moved-from spatial reference context rejects queries");

  const auto alias_result =
      moved.nearest(namespace_alias, origin(), alias_exclusions);
  check(
      alias_result.exact_partition.validated_for(namespace_alias),
      "a copied cloud preserves the canonical PointId namespace token");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(moved.nearest(
            independent_cloud, origin(), independent_exclusions));
      },
      "equal coordinates in a separately canonicalized cloud are another namespace");

  const auto reused = moved.nearest(cloud, origin(), exclusions);
  check(
      reused.exact_partition.validated_for(cloud) &&
          fake_gpu_spatial_launch_count() == 2U,
      "namespace rejection is pre-GPU and does not poison the moved context");
}

void test_pre_gpu_validation_errors_do_not_poison_context() {
  reset_fake_gpu_spatial_reference();
  const std::array<CertifiedPoint3, 5> points{
      point(-2.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const CanonicalPointCloud other_cloud = canonical_cloud(points);
  const ExclusionSet exclusions = empty_exclusions(cloud);
  const ExclusionSet alien_exclusions = empty_exclusions(other_cloud);
  SpatialReferenceContext context{cloud};

  check_throws<std::out_of_range>(
      [&] {
        static_cast<void>(context.top_k(cloud, origin(), 0U, exclusions));
      },
      "rank zero is rejected before GPU proposal");
  check_throws<std::out_of_range>(
      [&] {
        static_cast<void>(context.top_k(cloud, origin(), 6U, exclusions));
      },
      "rank above the eligible cardinality is rejected before GPU proposal");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(context.top_k(
            cloud, origin(), 1U, alien_exclusions));
      },
      "an alien exclusion namespace is rejected before GPU proposal");
  check(
      fake_gpu_spatial_launch_count() == 0U,
      "invalid query metadata never enters the fake GPU section");

  const ExactRational3 overflow_clamped_query{
      BigInt{1} << 1024U, BigInt{0}, BigInt{0}, BigInt{1}};
  const auto overflow_clamped =
      context.nearest(cloud, overflow_clamped_query, exclusions);
  check(
      overflow_clamped.audit.query_projection[0] ==
              QueryCoordinateProjection::overflow_clamped &&
          overflow_clamped.audit.projected_query_bits[0] ==
              UINT64_C(0x7fefffffffffffff) &&
          overflow_clamped.audit.cpu_exact_recertification_complete,
      "an out-of-range rational is clamped only in the diagnostic proposal");

  const auto valid = context.nearest(cloud, origin(), exclusions);
  check(
      valid.audit.cpu_exact_recertification_complete &&
          fake_gpu_spatial_launch_count() == 2U,
      "the same context remains usable after all pre-GPU validation errors");
}

void test_post_gpu_corruption_poisons_only_its_context() {
  const auto points = axis_tie_points();
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const ExclusionSet exclusions = empty_exclusions(cloud);
  struct CorruptionCase {
    FakeSpatialProposalCorruption corruption;
    std::size_t expected_launch_count;
    const char* label;
  };
  const std::array<CorruptionCase, 6> cases{{
      {FakeSpatialProposalCorruption::missing_record, 1U, "missing record"},
      {FakeSpatialProposalCorruption::duplicate_point_id, 1U, "duplicate PointId"},
      {FakeSpatialProposalCorruption::out_of_range_point_id,
       1U,
       "out-of-range PointId"},
      {FakeSpatialProposalCorruption::nan_distance, 1U, "NaN distance"},
      {FakeSpatialProposalCorruption::negative_distance,
       1U,
       "negative distance"},
      {FakeSpatialProposalCorruption::simulated_gpu_failure,
       0U,
       "GPU launch/copy failure"},
  }};

  for (const CorruptionCase& test_case : cases) {
    reset_fake_gpu_spatial_reference();
    SpatialReferenceContext context{cloud};
    configure_fake_gpu_spatial_reference(FakeSpatialProposalConfiguration{
        FakeSpatialProposalPermutation::reversed,
        FakeSpatialProposalValues::descending_by_point_id,
        test_case.corruption});
    check_throws<std::runtime_error>(
        [&] {
          static_cast<void>(context.nearest(cloud, origin(), exclusions));
        },
        std::string{"post-GPU corruption is rejected: "} + test_case.label);

    configure_fake_gpu_spatial_reference(FakeSpatialProposalConfiguration{});
    check_throws<std::runtime_error>(
        [&] {
          static_cast<void>(context.nearest(cloud, origin(), exclusions));
        },
        std::string{"corruption poisons its context: "} + test_case.label);
    check(
        fake_gpu_spatial_launch_count() == test_case.expected_launch_count,
        std::string{"poisoned context cannot launch again: "} +
            test_case.label);
  }

  reset_fake_gpu_spatial_reference();
  SpatialReferenceContext fresh_context{cloud};
  const auto fresh =
      fresh_context.nearest(cloud, origin(), exclusions);
  check(
      fresh.audit.cpu_exact_recertification_complete &&
          fake_gpu_spatial_launch_count() == 1U,
      "post-GPU poisoning is isolated from a fresh context");
}

void test_concurrent_corruption_is_linearized_with_poisoning() {
  reset_fake_gpu_spatial_reference();
  const CanonicalPointCloud cloud = canonical_cloud(axis_tie_points());
  const ExclusionSet exclusions = empty_exclusions(cloud);
  SpatialReferenceContext context{cloud};
  configure_fake_gpu_spatial_reference(FakeSpatialProposalConfiguration{
      FakeSpatialProposalPermutation::canonical,
      FakeSpatialProposalValues::actual_binary64_recipe,
      FakeSpatialProposalCorruption::missing_record});

  std::barrier start{3};
  std::atomic<unsigned int> rejection_count{0U};
  auto query = [&] {
    start.arrive_and_wait();
    try {
      static_cast<void>(context.nearest(cloud, origin(), exclusions));
    } catch (const std::runtime_error&) {
      rejection_count.fetch_add(1U, std::memory_order_relaxed);
    }
  };
  std::thread first{query};
  std::thread second{query};
  start.arrive_and_wait();
  first.join();
  second.join();

  check(
      rejection_count.load(std::memory_order_relaxed) == 2U,
      "both concurrent callers reject a corrupt proposal transaction");
  check(
      fake_gpu_spatial_launch_count() == 1U,
      "the second concurrent caller observes poison before another launch");
}

}  // namespace

int main() {
  test_false_and_reversed_proposals_do_not_decide_ties();
  test_exclusions_remain_host_only();
  test_proposal_digest_is_record_order_invariant();
  test_closed_ball_is_a_global_exact_partition();
  test_exact_rounded_and_underflowed_query_projection();
  test_move_and_point_namespace_contract();
  test_pre_gpu_validation_errors_do_not_poison_context();
  test_post_gpu_corruption_poisons_only_its_context();
  test_concurrent_corruption_is_linearized_with_poisoning();
  if (failures != 0) {
    std::cerr << failures << " GPU spatial reference context test(s) failed\n";
    return 1;
  }
  std::cout << "GPU spatial reference context tests passed\n";
  return 0;
}
