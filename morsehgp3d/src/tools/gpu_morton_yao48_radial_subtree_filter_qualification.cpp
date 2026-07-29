#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/gpu/morton_yao48_radial_subtree_filter.hpp"
#include "morsehgp3d/hierarchy/yao48_cone.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonYao48RadialSubtreeDecision;
using morsehgp3d::gpu::MortonYao48RadialSubtreeFilterContext;
using morsehgp3d::gpu::MortonYao48RadialSubtreeQuery;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

constexpr std::array<std::array<std::uint8_t, 3U>, 6U> permutations{{
    {{0U, 1U, 2U}},
    {{0U, 2U, 1U}},
    {{1U, 0U, 2U}},
    {{1U, 2U, 0U}},
    {{2U, 0U, 1U}},
    {{2U, 1U, 0U}},
}};

[[nodiscard]] std::uint64_t bits(double value) {
  return std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] CanonicalPointCloud make_cloud(
    std::array<double, 3U> target) {
  std::vector<CertifiedPoint3> points;
  points.reserve(50U);
  points.push_back(CertifiedPoint3::from_binary64(
      target[0], target[1], target[2]));
  points.push_back(CertifiedPoint3::from_binary64(0.0, 0.0, 0.0));
  for (std::uint8_t sign_mask = 0U; sign_mask < 8U; ++sign_mask) {
    for (const auto& axes : permutations) {
      std::array<double, 3U> coordinate{};
      coordinate[axes[0]] = 3.0;
      coordinate[axes[1]] = 2.0;
      coordinate[axes[2]] = 1.0;
      for (std::size_t axis = 0U; axis < 3U; ++axis) {
        if ((sign_mask & static_cast<std::uint8_t>(1U << axis)) != 0U) {
          coordinate[axis] = -coordinate[axis];
        }
      }
      points.push_back(CertifiedPoint3::from_binary64(
          coordinate[0], coordinate[1], coordinate[2]));
    }
  }
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] PointId origin_id(const CanonicalPointCloud& cloud) {
  for (PointId id = 0U; id < static_cast<PointId>(cloud.size()); ++id) {
    if (cloud.point(id).binary64_coordinate(0U) == 0.0 &&
        cloud.point(id).binary64_coordinate(1U) == 0.0 &&
        cloud.point(id).binary64_coordinate(2U) == 0.0) {
      return id;
    }
  }
  throw std::logic_error("the CUDA radial fixture has no origin");
}

[[nodiscard]] MortonYao48RadialSubtreeQuery make_query(
    const CanonicalPointCloud& cloud,
    const morsehgp3d::gpu::MortonLbvhDeviceBuildResult& build,
    std::uint64_t replay_id,
    std::uint64_t radius_upper_bits) {
  const auto leaves = build.certified_index().leaves();
  const PointId anchor = origin_id(cloud);
  std::size_t anchor_position = leaves.size();
  for (std::size_t position = 0U; position < leaves.size(); ++position) {
    if (leaves[position].point_id == anchor) {
      anchor_position = position;
      break;
    }
  }
  if (anchor_position == 0U || anchor_position == leaves.size() ||
      leaves[0U].point_id != 0U) {
    throw std::logic_error(
        "the CUDA radial fixture does not authenticate its expected leaf");
  }
  MortonYao48RadialSubtreeQuery query;
  query.replay_id = replay_id;
  query.anchor_morton_position = anchor_position;
  query.node_index = 0U;
  query.maximum_closed_rank = 2U;
  query.bank_squared_radius_upper_bits.fill(radius_upper_bits);
  for (PointId id = 0U; id < static_cast<PointId>(cloud.size()); ++id) {
    if (id == anchor || id == leaves[0U].point_id) {
      continue;
    }
    const std::size_t cone =
        morsehgp3d::hierarchy::classify_exact_yao48_cone(
            cloud.point(anchor), cloud.point(id))
            .cone_index;
    if (query.bank_witness_counts[cone] == 0U) {
      query.bank_witness_counts[cone] = 1U;
      query.bank_witness_point_ids[cone][0U] = id;
    }
  }
  for (std::size_t cone = 0U; cone < 48U; ++cone) {
    if (query.bank_witness_counts[cone] != 1U) {
      throw std::logic_error("the CUDA radial fixture has an underfull bank");
    }
  }
  return query;
}

void require_non_product_audit(
    const morsehgp3d::gpu::MortonYao48RadialSubtreeFilterAudit& audit) {
  if (!audit.cuda_execution_performed ||
      audit.host_fake_launcher_exercised ||
      !audit.subtree_pruning_implemented || audit.leaf_traversal_performed ||
      audit.pair_record_emitted || audit.product_path_enabled ||
      audit.dense_pair_fallback_performed ||
      audit.ordinary_delaunay_materialized ||
      audit.higher_order_delaunay_mosaic_materialized ||
      audit.scientific_frontier_complete_claimed || audit.public_status_claimed) {
    throw std::runtime_error(
        "the CUDA radial qualification reported a forbidden product claim");
  }
}

}  // namespace

int main() {
  try {
    const CanonicalPointCloud equality_cloud =
        make_cloud({-5.0, -4.0, -1.0});
    MortonLbvhBuildContext equality_builder{equality_cloud.size()};
    auto equality_build = equality_builder.build(equality_cloud);
    MortonYao48RadialSubtreeFilterContext equality_filter{1U};
    const std::array equality_query{
        make_query(equality_cloud, equality_build, 1U, bits(14.0))};
    auto equality = equality_filter.decide(
        equality_cloud, equality_build, equality_query);
    if (equality.decisions[0U].decision !=
            MortonYao48RadialSubtreeDecision::certified_prune ||
        equality.decisions[0U].exact_receipt == nullptr ||
        equality.decisions[0U].exact_receipt
                ->exact_aabb_minimum_squared_distance !=
            equality.decisions[0U].exact_receipt
                ->exact_three_times_maximum_bank_radius) {
      throw std::runtime_error(
          "the CUDA radial equality fixture was not certified");
    }
    require_non_product_audit(equality.audit);

    const CanonicalPointCloud near_cloud =
        make_cloud({-4.0, -2.0, -1.0});
    MortonLbvhBuildContext near_builder{near_cloud.size()};
    auto near_build = near_builder.build(near_cloud);
    MortonYao48RadialSubtreeFilterContext near_filter{1U};
    const std::array near_query{
        make_query(near_cloud, near_build, 2U, bits(14.0))};
    auto near = near_filter.decide(near_cloud, near_build, near_query);
    if (near.decisions[0U].decision !=
            MortonYao48RadialSubtreeDecision::descend ||
        near.audit.exact_replay_count != 0U) {
      throw std::runtime_error(
          "the CUDA radial near fixture did not fail open before replay");
    }
    require_non_product_audit(near.audit);

    auto nonfinite_query = near_query;
    nonfinite_query[0U].replay_id = 3U;
    nonfinite_query[0U].bank_squared_radius_upper_bits[0U] =
        UINT64_C(0x7ff0000000000000);
    auto nonfinite = near_filter.decide(
        near_cloud, near_build, nonfinite_query);
    if (nonfinite.decisions[0U].decision !=
            MortonYao48RadialSubtreeDecision::descend ||
        nonfinite.audit.device_fail_open_input_count != 1U ||
        nonfinite.audit.exact_replay_count != 0U) {
      throw std::runtime_error(
          "the CUDA radial non-finite fixture did not fail open");
    }
    require_non_product_audit(nonfinite.audit);
    std::cout << "Morton/Yao48 CUDA radial-subtree qualification passed\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Morton/Yao48 CUDA radial-subtree qualification failed: "
              << error.what() << '\n';
    return 1;
  }
}
