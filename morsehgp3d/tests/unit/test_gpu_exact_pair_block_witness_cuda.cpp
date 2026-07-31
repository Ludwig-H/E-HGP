#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/exact_pair_block_witness_cuda.hpp"

#include "phase15_exact_pair_block_q_fixed.cuh"
#include "phase15_exact_pair_block_witness_cuda_internal.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/aabb.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

namespace fixed =
    morsehgp3d::gpu::detail::exact_pair_block_q_fixed;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::ExactPairBlockAuthority;
using morsehgp3d::gpu::ExactPairBlockAuthorityKind;
using morsehgp3d::gpu::ExactPairBlockNodeAuthority;
using morsehgp3d::gpu::ExactPairBlockWitnessCudaConfig;
using morsehgp3d::gpu::ExactPairBlockWitnessCudaDecision;
using morsehgp3d::gpu::ExactPairBlockWitnessCudaStatus;
using morsehgp3d::gpu::ExactPairBlockWitnessCudaTask;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactDyadicAabb3;

static_assert(
    sizeof(morsehgp3d::gpu::detail::
               Phase15ExactPairBlockWitnessCudaTask) == 56U);
static_assert(
    sizeof(morsehgp3d::gpu::detail::
               Phase15ExactPairBlockWitnessCudaDeviceRecord) == 24U);

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

[[nodiscard]] CanonicalPointCloud line_cloud(std::size_t count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(count);
  for (std::size_t index = 0U; index < count; ++index) {
    const double coordinate = static_cast<double>(index);
    points.push_back(point(
        coordinate, coordinate / 16.0, coordinate / 256.0));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] MortonLbvhDeviceTraversalLease traversal_lease(
    std::size_t point_count) {
  morsehgp3d::gpu::test_support::
      reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = line_cloud(point_count);
  MortonLbvhBuildContext builder{point_count + 2U};
  auto build = builder.build(cloud);
  return builder.release_device_traversal_lease(build);
}

[[nodiscard]] ExactPairBlockNodeAuthority node(
    std::size_t index,
    std::size_t begin,
    std::size_t end) {
  return ExactPairBlockNodeAuthority{index, begin, end};
}

[[nodiscard]] ExactPairBlockWitnessCudaTask cross_task(
    std::uint64_t id,
    ExactPairBlockNodeAuthority first,
    ExactPairBlockNodeAuthority second,
    ExactPairBlockNodeAuthority witness) {
  return ExactPairBlockWitnessCudaTask{
      id,
      ExactPairBlockAuthority{
          ExactPairBlockAuthorityKind::cross,
          first,
          second,
          first.leaf_count() * second.leaf_count()},
      witness};
}

[[nodiscard]] std::uint64_t bits(double value) noexcept {
  return std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] ExactDyadicAabb3 box(
    std::array<double, 3U> lower,
    std::array<double, 3U> upper) {
  ExactDyadicAabb3 result{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result.lower_binary64_bits[axis] = bits(lower[axis]);
    result.upper_binary64_bits[axis] = bits(upper[axis]);
  }
  return result;
}

[[nodiscard]] fixed::Binary64Aabb3 fixed_box(
    const ExactDyadicAabb3& source) noexcept {
  fixed::Binary64Aabb3 result{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result.lower[axis] = source.lower_binary64_bits[axis];
    result.upper[axis] = source.upper_binary64_bits[axis];
  }
  return result;
}

[[nodiscard]] int fixed_sign(fixed::fixed::ResultCode code) {
  switch (code) {
    case fixed::fixed::ResultCode::negative:
      return -1;
    case fixed::fixed::ResultCode::zero:
      return 0;
    case fixed::fixed::ResultCode::positive:
      return 1;
    case fixed::fixed::ResultCode::requires_cpu_fallback:
    case fixed::fixed::ResultCode::invalid_input:
      break;
  }
  throw std::logic_error("a fixed-limb fixture did not produce a decision");
}

void check_exact_fixture(
    const ExactDyadicAabb3& first,
    const ExactDyadicAabb3& second,
    const ExactDyadicAabb3& witness,
    const std::string& label) {
  const int expected = morsehgp3d::hierarchy::
      exact_diametral_phi_aabb_maximum_sign(first, second, witness);
  const fixed::fixed::ResultCode actual = fixed::exact_maximum_sign(
      fixed_box(first), fixed_box(second), fixed_box(witness));
  check(
      actual != fixed::fixed::ResultCode::requires_cpu_fallback &&
          actual != fixed::fixed::ResultCode::invalid_input,
      label + " must stay in the proved fixed-limb domain");
  if (actual != fixed::fixed::ResultCode::requires_cpu_fallback &&
      actual != fixed::fixed::ResultCode::invalid_input) {
    check(fixed_sign(actual) == expected, label + " exact sign mismatch");
  }
}

void test_hostile_exact_fixed_limb_differential() {
  const double denormal = std::numeric_limits<double>::denorm_min();
  const double maximum = std::numeric_limits<double>::max();
  const double previous_maximum = std::nextafter(maximum, 0.0);
  const double negative_maximum = -maximum;
  const double next_negative_maximum =
      std::nextafter(negative_maximum, 0.0);

  check_exact_fixture(
      box({-0.0, -0.0, -0.0}, {+0.0, +0.0, +0.0}),
      box({+0.0, -0.0, +0.0}, {+0.0, +0.0, +0.0}),
      box({-0.0, +0.0, -0.0}, {+0.0, +0.0, +0.0}),
      "signed zero");
  check_exact_fixture(
      box({-denormal, 0.0, 0.0}, {-denormal, 0.0, 0.0}),
      box({denormal, 0.0, 0.0}, {denormal, 0.0, 0.0}),
      box({0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}),
      "subnormal negative");
  check_exact_fixture(
      box({-1.0, 0.0, 0.0}, {-1.0, 0.0, 0.0}),
      box({1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}),
      box({0.0, 1.0, 0.0}, {0.0, 1.0, 0.0}),
      "exact cancellation");
  check_exact_fixture(
      box({-2.0, -2.0, -0.0}, {-1.0, -1.0, +0.0}),
      box({1.0, 1.0, -0.0}, {2.0, 2.0, +0.0}),
      box({-0.5, -0.5, -0.0}, {0.5, 0.5, +0.0}),
      "AABB endpoint maximum");
  check_exact_fixture(
      box(
          {previous_maximum, previous_maximum, previous_maximum},
          {maximum, maximum, maximum}),
      box({maximum, maximum, maximum}, {maximum, maximum, maximum}),
      box(
          {previous_maximum, previous_maximum, previous_maximum},
          {previous_maximum, previous_maximum, previous_maximum}),
      "positive finite extreme adjacency");
  check_exact_fixture(
      box(
          {negative_maximum, negative_maximum, negative_maximum},
          {next_negative_maximum, next_negative_maximum,
           next_negative_maximum}),
      box(
          {negative_maximum, negative_maximum, negative_maximum},
          {negative_maximum, negative_maximum, negative_maximum}),
      box(
          {next_negative_maximum, next_negative_maximum,
           next_negative_maximum},
          {next_negative_maximum, next_negative_maximum,
           next_negative_maximum}),
      "negative finite extreme adjacency");

  check_exact_fixture(
      box({negative_maximum, 0.0, 0.0}, {negative_maximum, 0.0, 0.0}),
      box({maximum, 0.0, 0.0}, {maximum, 0.0, 0.0}),
      box({0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}),
      "opposite finite extremes");
  check(
      fixed::exact_maximum_sign(
          fixed_box(box({1.0, 0.0, 0.0}, {-1.0, 0.0, 0.0})),
          fixed_box(box({2.0, 0.0, 0.0}, {2.0, 0.0, 0.0})),
          fixed_box(box({3.0, 0.0, 0.0}, {3.0, 0.0, 0.0}))) ==
          fixed::fixed::ResultCode::invalid_input,
      "an inverted AABB must be rejected before exact certification");

  const ExactDyadicAabb3 huge_positive =
      box({maximum, 0.0, 0.0}, {maximum, 0.0, 0.0});
  const ExactDyadicAabb3 origin =
      box({0.0, 0.0, 0.0}, {0.0, 0.0, 0.0});
  const ExactDyadicAabb3 tiny_witness =
      box({denormal, 0.0, 0.0}, {denormal, 0.0, 0.0});
  static_cast<void>(morsehgp3d::hierarchy::
      exact_diametral_phi_aabb_maximum_sign(
          huge_positive, origin, tiny_witness));
  check(
      fixed::exact_maximum_sign(
          fixed_box(huge_positive), fixed_box(origin),
          fixed_box(tiny_witness)) ==
          fixed::fixed::ResultCode::requires_cpu_fallback,
      "an out-of-limb finite extreme must remain an explicit residual");
}

void test_compact_batch_contract() {
  std::vector<ExactPairBlockWitnessCudaTask> tasks;
  tasks.push_back(cross_task(0U, node(0U, 0U, 1U), node(1U, 1U, 2U),
                             node(2U, 2U, 4U)));
  tasks.push_back(cross_task(1U, node(0U, 0U, 1U), node(1U, 1U, 2U),
                             node(2U, 2U, 4U)));
  tasks.push_back(cross_task(2U, node(0U, 0U, 1U), node(1U, 1U, 2U),
                             node(2U, 2U, 4U)));
  tasks.push_back(cross_task(3U, node(0U, 0U, 1U), node(1U, 1U, 2U),
                             node(2U, 2U, 3U)));
  tasks.push_back(cross_task(4U, node(0U, 0U, 1U), node(1U, 1U, 2U),
                             node(2U, 0U, 2U)));
  ExactPairBlockWitnessCudaTask invalid =
      cross_task(5U, node(0U, 0U, 1U), node(1U, 1U, 2U),
                 node(2U, 2U, 4U));
  invalid.support_block.kind = ExactPairBlockAuthorityKind::diagonal;
  tasks.push_back(invalid);

  auto result = morsehgp3d::gpu::qualify_exact_pair_block_witnesses_cuda(
      traversal_lease(8U), tasks, ExactPairBlockWitnessCudaConfig{3U, 4U});
  check(
      result.status ==
          ExactPairBlockWitnessCudaStatus::non_authoritative_host_fake,
      "the GCC launcher must remain explicitly non-authoritative");
  check(result.records.size() == 6U, "every compact task must be returned");
  check(
      result.records[0].decision ==
              ExactPairBlockWitnessCudaDecision::certified_closed &&
          result.records[0].fixed_limb_evaluation_performed &&
          result.records[0].exact_sign == -1,
      "the fake negative path must carry the exact-decision proof fields");
  check(
      result.records[2].decision == ExactPairBlockWitnessCudaDecision::
          residual_fixed_limb_overflow,
      "fixed-limb overflow must remain residual");
  check(
      result.audit.certified_task_count == 1U &&
          result.audit.nonnegative_task_count == 1U &&
          result.audit.fixed_limb_residual_count == 1U &&
          result.audit.insufficient_witness_task_count == 1U &&
          result.audit.support_overlap_task_count == 1U &&
          result.audit.invalid_authority_task_count == 1U,
      "the compact receipt counters must partition the batch");
  check(
      result.audit.submitted_unordered_pair_mass == 6U &&
          result.audit.pruned_unordered_pair_mass == 1U &&
          result.audit.residual_unordered_pair_mass == 5U &&
          result.audit.local_submitted_mass_conservation_validated,
      "the local submitted mass must be conserved exactly");
  check(
      result.audit.submitted_task_digest != 0U &&
          result.audit.completed_result_digest != 0U &&
          result.audit.compact_batch_abi_validated &&
          result.audit.fp64_used_as_proposal_only &&
          result.audit.negative_closure_requires_fixed_limb_exact_decision,
      "the scheduler-facing compact ABI must expose digests and proof gates");
  check(
      !result.audit.global_pair_coverage_closed &&
          !result.audit.double_buffered_transactional_frontier_claimed &&
          !result.audit.pair_catalog_complete_claimed &&
          !result.audit.hierarchy_or_tree_claimed &&
          !result.audit.slo_claimed &&
          !result.audit.global_pair_matrix_materialized &&
          !result.audit.ordinary_or_higher_order_delaunay_materialized,
      "a component batch must make no frontier, hierarchy, SLO or Delaunay "
      "claim");
}

void test_preflight_refusal_does_not_consume_authority() {
  auto lease = traversal_lease(8U);
  std::vector<ExactPairBlockWitnessCudaTask> tasks(
      9U,
      cross_task(0U, node(0U, 0U, 1U), node(1U, 1U, 2U),
                 node(2U, 2U, 3U)));
  auto result = morsehgp3d::gpu::qualify_exact_pair_block_witnesses_cuda(
      std::move(lease), tasks, ExactPairBlockWitnessCudaConfig{2U, 1U});
  check(
      result.status == ExactPairBlockWitnessCudaStatus::capacity_exhausted &&
          result.audit.task_capacity == 8U &&
          !result.audit.native_lbvh_authority_consumed,
      "linear-capacity preflight must fail before lease adoption");
  check(lease.ready(), "a preflight refusal must leave the lease reusable");

  check_throws<std::out_of_range>(
      [&]() {
        static_cast<void>(
            morsehgp3d::gpu::qualify_exact_pair_block_witnesses_cuda(
                std::move(lease),
                std::span<const ExactPairBlockWitnessCudaTask>{tasks}.first(1U),
                ExactPairBlockWitnessCudaConfig{7U, 1U}));
      },
      "rank greater than six must be rejected");
}

}  // namespace

int main() {
  test_hostile_exact_fixed_limb_differential();
  test_compact_batch_contract();
  test_preflight_refusal_does_not_consume_authority();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "all exact pair-block CUDA host-contract tests passed\n";
  return 0;
}
