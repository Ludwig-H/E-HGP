#include "fake_gpu_morton_yao48_radial_subtree_filter_launchers.hpp"
#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/gpu/morton_yao48_radial_subtree_filter.hpp"
#include "morsehgp3d/hierarchy/yao48_cone.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonYao48RadialSubtreeDecision;
using morsehgp3d::gpu::MortonYao48RadialSubtreeFilterContext;
using morsehgp3d::gpu::MortonYao48RadialSubtreeQuery;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

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
    std::cerr << "FAIL: " << message << " (unexpected: "
              << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] std::uint64_t bits(double value) {
  return std::bit_cast<std::uint64_t>(value);
}

constexpr std::array<std::array<std::uint8_t, 3U>, 6U> permutations{{
    {{0U, 1U, 2U}},
    {{0U, 2U, 1U}},
    {{1U, 0U, 2U}},
    {{1U, 2U, 0U}},
    {{2U, 0U, 1U}},
    {{2U, 1U, 0U}},
}};

[[nodiscard]] std::vector<CertifiedPoint3> cone_cloud_points(
    std::array<double, 3U> target,
    double scale = 1.0) {
  std::vector<CertifiedPoint3> points;
  points.reserve(50U);
  points.push_back(CertifiedPoint3::from_binary64(
      target[0], target[1], target[2]));
  points.push_back(CertifiedPoint3::from_binary64(0.0, 0.0, 0.0));
  for (std::uint8_t sign_mask = 0U; sign_mask < 8U; ++sign_mask) {
    for (const auto& axes : permutations) {
      std::array<double, 3U> coordinate{};
      coordinate[axes[0]] = 3.0 * scale;
      coordinate[axes[1]] = 2.0 * scale;
      coordinate[axes[2]] = 1.0 * scale;
      for (std::size_t axis = 0U; axis < 3U; ++axis) {
        if ((sign_mask & static_cast<std::uint8_t>(1U << axis)) != 0U) {
          coordinate[axis] = -coordinate[axis];
        }
      }
      points.push_back(CertifiedPoint3::from_binary64(
          coordinate[0], coordinate[1], coordinate[2]));
    }
  }
  return points;
}

[[nodiscard]] PointId find_origin(const CanonicalPointCloud& cloud) {
  for (PointId id = 0U; id < static_cast<PointId>(cloud.size()); ++id) {
    if (cloud.point(id).binary64_coordinate(0U) == 0.0 &&
        cloud.point(id).binary64_coordinate(1U) == 0.0 &&
        cloud.point(id).binary64_coordinate(2U) == 0.0) {
      return id;
    }
  }
  throw std::logic_error("the cone fixture has no origin");
}

[[nodiscard]] MortonYao48RadialSubtreeQuery make_query(
    const CanonicalPointCloud& cloud,
    const morsehgp3d::gpu::MortonLbvhDeviceBuildResult& build,
    std::uint64_t replay_id,
    double radius_upper) {
  const auto leaves = build.certified_index().leaves();
  const PointId anchor = find_origin(cloud);
  std::size_t anchor_position = leaves.size();
  for (std::size_t position = 0U; position < leaves.size(); ++position) {
    if (leaves[position].point_id == anchor) {
      anchor_position = position;
      break;
    }
  }
  if (anchor_position == leaves.size() || anchor_position == 0U) {
    throw std::logic_error("the origin does not own the leftmost leaf");
  }

  MortonYao48RadialSubtreeQuery query;
  query.replay_id = replay_id;
  query.anchor_morton_position = anchor_position;
  query.node_index = 0U;
  query.maximum_closed_rank = 2U;
  query.bank_squared_radius_upper_bits.fill(bits(radius_upper));
  for (PointId id = 0U; id < static_cast<PointId>(cloud.size()); ++id) {
    if (id == anchor || id == leaves[0U].point_id) {
      continue;
    }
    const std::size_t cone =
        morsehgp3d::hierarchy::classify_exact_yao48_cone(
            cloud.point(anchor), cloud.point(id))
            .cone_index;
    if (query.bank_witness_counts[cone] == 0U) {
      query.bank_witness_point_ids[cone][0U] = id;
      query.bank_witness_counts[cone] = 1U;
    }
  }
  for (std::size_t cone = 0U; cone < 48U; ++cone) {
    if (query.bank_witness_counts[cone] != 1U) {
      throw std::logic_error("the cone fixture did not fill all 48 banks");
    }
  }
  return query;
}

void check_no_product_path(
    const morsehgp3d::gpu::MortonYao48RadialSubtreeFilterAudit& audit) {
  check(
      audit.subtree_pruning_implemented &&
          !audit.leaf_traversal_performed && !audit.pair_record_emitted &&
          !audit.product_path_enabled && !audit.dense_pair_fallback_performed &&
          !audit.global_pair_matrix_materialized &&
          !audit.ordinary_delaunay_materialized &&
          !audit.higher_order_delaunay_mosaic_materialized &&
          !audit.scientific_frontier_complete_claimed &&
          !audit.public_status_claimed,
      "the standalone radial filter must not become a leaf/pair product path");
}

void test_exact_equality_and_hostile_false_proposal() {
  morsehgp3d::gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud equality_cloud =
      CanonicalPointCloud::rejecting_duplicates(
          cone_cloud_points({-5.0, -4.0, -1.0}));
  MortonLbvhBuildContext equality_builder{equality_cloud.size()};
  auto equality_build = equality_builder.build(equality_cloud);
  const auto equality_query =
      make_query(equality_cloud, equality_build, 1U, 14.0);
  check(
      equality_build.certified_index().leaves()[0U].point_id == 0U,
      "the equality target must be the authenticated leftmost leaf");

  morsehgp3d::gpu::test_support::configure_fake_morton_yao48_radial_records(
      {{true, true}});
  MortonYao48RadialSubtreeFilterContext equality_filter{1U};
  const std::array equality_queries{equality_query};
  auto equality = equality_filter.decide(
      equality_cloud, equality_build, equality_queries);
  check(
      equality.decisions.size() == 1U &&
          equality.decisions[0U].decision ==
              MortonYao48RadialSubtreeDecision::certified_prune &&
          equality.decisions[0U].exact_replay_accepted &&
          equality.decisions[0U].exact_receipt != nullptr &&
          equality.decisions[0U].exact_receipt
                  ->exact_aabb_minimum_squared_distance ==
              equality.decisions[0U].exact_receipt
                  ->exact_three_times_maximum_bank_radius &&
          equality.decisions[0U].exact_receipt
                  ->authenticated_subtree_leaf_count == 1U,
      "non-strict radial equality must survive exact replay");
  check(
      equality.audit.exact_replay_count == 1U &&
          equality.audit.exact_witness_distance_evaluation_count == 48U &&
          equality.audit.exact_cone_recertification_count == 48U &&
          equality.audit.exact_accepted_prune_count == 1U,
      "only the CUDA prune proposal must pay the 48-bank exact replay");
  check_no_product_path(equality.audit);

  const CanonicalPointCloud near_cloud =
      CanonicalPointCloud::rejecting_duplicates(
          cone_cloud_points({-4.0, -2.0, -1.0}));
  MortonLbvhBuildContext near_builder{near_cloud.size()};
  auto near_build = near_builder.build(near_cloud);
  const std::array near_queries{
      make_query(near_cloud, near_build, 2U, 14.0)};
  morsehgp3d::gpu::test_support::configure_fake_morton_yao48_radial_records(
      {{true, true}});
  MortonYao48RadialSubtreeFilterContext near_filter{1U};
  auto rejected = near_filter.decide(near_cloud, near_build, near_queries);
  check(
      rejected.decisions[0U].decision ==
              MortonYao48RadialSubtreeDecision::descend &&
          !rejected.decisions[0U].exact_replay_accepted &&
          rejected.decisions[0U].exact_receipt == nullptr &&
          rejected.audit.hostile_or_false_proposal_rejected_count == 1U,
      "a hostile false radial proposal must fail open to descent");
}

void test_device_fail_open_and_conditional_replay() {
  morsehgp3d::gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = CanonicalPointCloud::rejecting_duplicates(
      cone_cloud_points({-6.0, -4.0, -2.0}));
  MortonLbvhBuildContext builder{cloud.size()};
  auto build = builder.build(cloud);
  auto query = make_query(cloud, build, 3U, 14.0);

  morsehgp3d::gpu::test_support::configure_fake_morton_yao48_radial_records(
      {{false, false}});
  MortonYao48RadialSubtreeFilterContext filter{1U};
  const std::array queries{query};
  auto descended = filter.decide(cloud, build, queries);
  check(
      descended.audit.exact_replay_count == 0U &&
          descended.audit.exact_witness_distance_evaluation_count == 0U &&
          descended.audit.exact_cone_recertification_count == 0U &&
          descended.decisions[0U].decision ==
              MortonYao48RadialSubtreeDecision::descend,
      "a CUDA fail-open must avoid every multiprecision replay");

  auto underestimated = query;
  underestimated.replay_id = 30U;
  underestimated.bank_squared_radius_upper_bits[0U] = bits(13.0);
  const std::array underestimated_queries{underestimated};
  morsehgp3d::gpu::test_support::configure_fake_morton_yao48_radial_records(
      {{true, true}});
  auto underestimated_result =
      filter.decide(cloud, build, underestimated_queries);
  check(
      underestimated_result.decisions[0U].decision ==
              MortonYao48RadialSubtreeDecision::descend &&
          underestimated_result.audit
                  .hostile_or_false_proposal_rejected_count == 1U,
      "an underestimated D-up word must be rejected by exact replay");

  auto wrong_cone = query;
  wrong_cone.replay_id = 31U;
  std::swap(
      wrong_cone.bank_witness_point_ids[0U][0U],
      wrong_cone.bank_witness_point_ids[1U][0U]);
  const std::array wrong_cone_queries{wrong_cone};
  auto wrong_cone_result = filter.decide(cloud, build, wrong_cone_queries);
  check(
      wrong_cone_result.decisions[0U].decision ==
              MortonYao48RadialSubtreeDecision::descend &&
          wrong_cone_result.audit
                  .hostile_or_false_proposal_rejected_count == 1U,
      "a binary64 bank ambiguity must fail open, not poison the context");

  auto valid_after_ambiguity = query;
  valid_after_ambiguity.replay_id = 32U;
  const std::array valid_after_ambiguity_queries{valid_after_ambiguity};
  auto recovered = filter.decide(
      cloud, build, valid_after_ambiguity_queries);
  check(
      recovered.decisions[0U].decision ==
              MortonYao48RadialSubtreeDecision::certified_prune &&
          recovered.audit.exact_accepted_prune_count == 1U,
      "a valid query must succeed on the same context after cone ambiguity");

  query.bank_squared_radius_upper_bits[0U] =
      UINT64_C(0x7ff0000000000000);
  const std::array overflow_queries{query};
  morsehgp3d::gpu::test_support::configure_fake_morton_yao48_radial_records(
      {{true, true}});
  auto overflow = filter.decide(cloud, build, overflow_queries);
  check(
      overflow.audit.device_fail_open_input_count == 1U &&
          overflow.audit.hostile_or_false_proposal_rejected_count == 1U &&
          overflow.decisions[0U].decision ==
              MortonYao48RadialSubtreeDecision::descend,
      "a non-finite/overflow radius bound must reject even a hostile prune");

  query.bank_squared_radius_upper_bits[0U] =
      UINT64_C(0x7ff8000000000001);
  const std::array nan_queries{query};
  morsehgp3d::gpu::test_support::configure_fake_morton_yao48_radial_records(
      {{false, false}});
  auto ambiguous = filter.decide(cloud, build, nan_queries);
  check(
      ambiguous.audit.device_fail_open_input_count == 1U &&
          ambiguous.audit.exact_replay_count == 0U,
      "a NaN/ambiguous bound must fail open without exact work");
  check_no_product_path(ambiguous.audit);
}

void test_structural_and_bank_guards() {
  morsehgp3d::gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = CanonicalPointCloud::rejecting_duplicates(
      cone_cloud_points({-6.0, -4.0, -2.0}));
  MortonLbvhBuildContext builder{cloud.size()};
  auto build = builder.build(cloud);
  auto query = make_query(cloud, build, 4U, 14.0);
  query.bank_witness_counts[7U] = 0U;
  const std::array queries{query};
  MortonYao48RadialSubtreeFilterContext filter{1U};
  check_throws<std::invalid_argument>(
      [&] { static_cast<void>(filter.decide(cloud, build, queries)); },
      "an underfull chamber must be rejected before the launcher");
}

}  // namespace

int main() {
  try {
    test_exact_equality_and_hostile_false_proposal();
    test_device_fail_open_and_conditional_replay();
    test_structural_and_bank_guards();
  } catch (const std::exception& error) {
    std::cerr << "unexpected exception: " << error.what() << '\n';
    return 1;
  }
  if (failures != 0) {
    std::cerr << failures << " radial-subtree filter check(s) failed\n";
    return 1;
  }
  std::cout << "Morton/Yao48 radial-subtree filter checks passed\n";
  return 0;
}
