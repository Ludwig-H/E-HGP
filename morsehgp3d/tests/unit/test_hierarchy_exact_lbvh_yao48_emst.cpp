#include "morsehgp3d/hierarchy/exact_lbvh_yao48_emst.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactLbvhYao48EmstConfig;
using morsehgp3d::hierarchy::ExactLbvhYao48EmstResult;
using morsehgp3d::hierarchy::build_exact_lbvh_yao48_emst;
using morsehgp3d::hierarchy::exact_yao48_cone_count;
using morsehgp3d::hierarchy::verify_exact_lbvh_yao48_emst_bounded;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
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
    function();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message << " (unexpected exception: "
              << error.what() << ")\n";
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

[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::vector<CertifiedPoint3>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] bool same_geometry_transcript(
    const ExactLbvhYao48EmstResult& left,
    const ExactLbvhYao48EmstResult& right) {
  return left.point_count == right.point_count &&
         left.directed_candidates == right.directed_candidates &&
         left.unique_candidate_edges == right.unique_candidate_edges &&
         left.emst_edges == right.emst_edges &&
         left.total_squared_weight == right.total_squared_weight &&
         left.total_hgp_weight == right.total_hgp_weight;
}

void check_qualified(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const ExactLbvhYao48EmstResult& result,
    const std::string& fixture) {
  check(
      result.locally_structurally_valid(),
      fixture + " closes local structural accounting");
  const auto verification =
      verify_exact_lbvh_yao48_emst_bounded(index, cloud, result);
  check(
      verification.transcript_certified(),
      fixture + " matches the full quadratic Yao transcript and Boruvka");
  check(
      result.directed_candidates.size() <=
          cloud.size() * exact_yao48_cone_count,
      fixture + " retains at most 48 directed candidates per point");
  check(
      result.counters.complete_source_coverage &&
          result.counters.no_candidate_truncation &&
          result.counters.morton_permutation_validated &&
          !result.counters.higher_order_delaunay_mosaic_materialized &&
          !result.counters.global_pair_matrix_materialized &&
          !result.counters.public_status_claimed,
      fixture + " records complete LBVH-only non-public semantics");
  check(
      result.counters.cone_mask_empty_count == 0U,
      fixture + " never loses all closed cones for a nonempty AABB");
}

void test_singleton_and_identity_guard() {
  const std::array<CertifiedPoint3, 1> input{point(2.0, -1.0, 4.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactLbvhYao48EmstResult result =
      build_exact_lbvh_yao48_emst(index, cloud);
  check_qualified(index, cloud, result, "singleton");
  check(
      result.directed_candidates.empty() &&
          result.unique_candidate_edges.empty() && result.emst_edges.empty() &&
          result.counters.morton_seed_window_radius == 0U &&
          result.counters.node_visit_count == 1U &&
          result.counters.leaf_distance_evaluation_count == 0U,
      "singleton traverses its sole leaf without inventing a direction");

  const CanonicalPointCloud rebuilt_cloud = canonical_cloud(input);
  const MortonLbvhIndex rebuilt_index = MortonLbvhIndex::build(rebuilt_cloud);
  check_throws<std::invalid_argument>(
      [&rebuilt_index, &cloud]() {
        static_cast<void>(build_exact_lbvh_yao48_emst(
            rebuilt_index, cloud, ExactLbvhYao48EmstConfig{1U}));
      },
      "builder rejects an LBVH from another point namespace");
  check(
      !verify_exact_lbvh_yao48_emst_bounded(
           rebuilt_index, cloud, result)
           .transcript_certified(),
      "bounded verifier refuses an LBVH from another point namespace");
}

[[nodiscard]] std::vector<CertifiedPoint3> irregular_fixture(
    std::size_t point_count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    const double value = static_cast<double>(index);
    const double x =
        static_cast<double>((index * 37U + 11U) % 101U) + value / 256.0;
    const double y =
        static_cast<double>((index * 61U + 7U) % 103U) - value / 512.0;
    const double z =
        static_cast<double>((index * 43U + 19U) % 107U) + value / 1024.0;
    points.push_back(point(x, y, z));
  }
  return points;
}

void test_window_invariance_and_complete_transcript() {
  const CanonicalPointCloud cloud = canonical_cloud(irregular_fixture(40U));
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactLbvhYao48EmstResult radius_one =
      build_exact_lbvh_yao48_emst(
          index, cloud, ExactLbvhYao48EmstConfig{1U});
  const ExactLbvhYao48EmstResult radius_32 =
      build_exact_lbvh_yao48_emst(
          index, cloud, ExactLbvhYao48EmstConfig{32U});
  const ExactLbvhYao48EmstResult exhaustive =
      build_exact_lbvh_yao48_emst(
          index, cloud, ExactLbvhYao48EmstConfig{cloud.size() - 1U});

  check_qualified(index, cloud, radius_one, "irregular W=1");
  check_qualified(index, cloud, radius_32, "irregular W=32");
  check_qualified(index, cloud, exhaustive, "irregular exhaustive W");
  check(
      same_geometry_transcript(radius_one, radius_32) &&
          same_geometry_transcript(radius_one, exhaustive),
      "W=1, W=32 and exhaustive Morton seeds produce one exact transcript");
  check(
      radius_one.counters.strict_aabb_prune_count > 0U &&
          radius_one.counters.leaf_distance_evaluation_count > 0U &&
          radius_one.counters.seeded_subtree_skip_count > 0U,
      "small-W traversal exercises seeding, exact leaves and strict pruning");
  check(
      exhaustive.counters.seed_distance_evaluation_count ==
              cloud.size() * (cloud.size() - 1U) &&
          exhaustive.counters.leaf_distance_evaluation_count == 0U &&
          exhaustive.counters.node_visit_count == cloud.size() &&
          exhaustive.counters.seeded_subtree_skip_count == cloud.size(),
      "exhaustive seeds make each certified root a seeded subtree skip");
  check(
      radius_one.counters.maximum_node_visit_count_per_source <=
              radius_one.counters.lbvh_node_count &&
          radius_one.counters.maximum_leaf_distance_evaluation_count_per_source <
              cloud.size() &&
          radius_one.counters.fixed_output_record_byte_count > 0U,
      "per-source maxima and fixed output bytes are exposed");
}

void test_equal_distance_requires_equality_descent() {
  const std::array<CertifiedPoint3, 5> input{
      point(0.0, 0.0, 0.0),
      point(100.0, 56.0, 33.0),
      point(100.0, 63.0, 16.0),
      point(-17.0, -9.0, -4.0),
      point(151.0, 99.0, 71.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactLbvhYao48EmstResult result =
      build_exact_lbvh_yao48_emst(
          index, cloud, ExactLbvhYao48EmstConfig{1U});
  check_qualified(index, cloud, result, "equal-distance same-cone fixture");
  check(
      result.counters.equal_bound_descent_count > 0U,
      "an exact AABB bound equal to an incumbent descends instead of pruning");
  const auto candidate = std::find_if(
      result.directed_candidates.begin(),
      result.directed_candidates.end(),
      [](const auto& value) {
        return value.source_point_id == PointId{1} &&
               value.cone_index == 0U;
      });
  check(
      candidate != result.directed_candidates.end(),
      "equal-distance fixture retains a canonical half-open cone candidate");
}

void test_morton_collisions_and_boundary_degeneracies() {
  const double epsilon = std::ldexp(1.0, -50);
  const std::array<CertifiedPoint3, 10> input{
      point(-1.0, -1.0, -1.0),
      point(0.0, 0.0, 0.0),
      point(epsilon, 2.0 * epsilon, 3.0 * epsilon),
      point(2.0 * epsilon, 5.0 * epsilon, 7.0 * epsilon),
      point(1.0, 0.0, 0.0),
      point(1.0, 1.0, 0.0),
      point(1.0, 1.0, 1.0),
      point(0.0, -1.0, 0.0),
      point(0.0, 0.0, -1.0),
      point(1.0, -1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  check(
      index.build_counters().morton_collision_group_count > 0U &&
          index.build_counters().maximum_morton_collision_size > 1U,
      "fixture exercises certified Morton-code collisions");
  const ExactLbvhYao48EmstResult radius_one =
      build_exact_lbvh_yao48_emst(
          index, cloud, ExactLbvhYao48EmstConfig{1U});
  const ExactLbvhYao48EmstResult radius_32 =
      build_exact_lbvh_yao48_emst(
          index, cloud, ExactLbvhYao48EmstConfig{32U});
  check_qualified(index, cloud, radius_one, "Morton collision W=1");
  check_qualified(index, cloud, radius_32, "Morton collision W=32");
  check(
      same_geometry_transcript(radius_one, radius_32),
      "Morton collisions and cone boundaries are seed-window invariant");
}

void test_bounded_verifier_rejects_mutations() {
  const CanonicalPointCloud cloud = canonical_cloud(irregular_fixture(12U));
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactLbvhYao48EmstResult baseline =
      build_exact_lbvh_yao48_emst(
          index, cloud, ExactLbvhYao48EmstConfig{1U});
  check_qualified(index, cloud, baseline, "mutation baseline");

  ExactLbvhYao48EmstResult directed_mutation = baseline;
  auto& directed = directed_mutation.directed_candidates.front();
  PointId replacement =
      static_cast<PointId>((directed.target_point_id + 1U) % cloud.size());
  if (replacement == directed.source_point_id) {
    replacement =
        static_cast<PointId>((replacement + 1U) % cloud.size());
  }
  directed.target_point_id = replacement;
  const auto directed_verification =
      verify_exact_lbvh_yao48_emst_bounded(
          index, cloud, directed_mutation);
  check(
      !directed_verification.directed_candidates_match_reference &&
          !directed_verification.transcript_certified(),
      "fresh oracle rejects a mutated directed endpoint");

  ExactLbvhYao48EmstResult unique_mutation = baseline;
  unique_mutation.unique_candidate_edges.front().squared_length = ExactLevel{};
  const auto unique_verification = verify_exact_lbvh_yao48_emst_bounded(
      index, cloud, unique_mutation);
  check(
      !unique_verification.result_structurally_valid &&
          !unique_verification.unique_candidate_edges_match_reference &&
          !unique_verification.transcript_certified(),
      "fresh oracle rejects a mutated unique exact edge");

  ExactLbvhYao48EmstResult tree_mutation = baseline;
  tree_mutation.emst_edges.front().v = tree_mutation.emst_edges.front().u;
  const auto tree_verification = verify_exact_lbvh_yao48_emst_bounded(
      index, cloud, tree_mutation);
  check(
      !tree_verification.result_structurally_valid &&
          !tree_verification.canonical_kruskal_matches_reference &&
          !tree_verification.canonical_emst_matches_exact_lbvh_boruvka &&
          !tree_verification.transcript_certified(),
      "fresh oracles reject a mutated Kruskal tree edge");

  ExactLbvhYao48EmstResult total_mutation = baseline;
  total_mutation.total_squared_weight = ExactLevel{};
  const auto total_verification = verify_exact_lbvh_yao48_emst_bounded(
      index, cloud, total_mutation);
  check(
      !total_verification.result_structurally_valid &&
          !total_verification.exact_totals_match_reference &&
          !total_verification.transcript_certified(),
      "fresh oracle rejects a mutated aggregate exact weight");

  ExactLbvhYao48EmstResult counter_mutation = baseline;
  ++counter_mutation.counters.node_visit_count;
  const auto counter_verification = verify_exact_lbvh_yao48_emst_bounded(
      index, cloud, counter_mutation);
  check(
      !counter_mutation.locally_structurally_valid() &&
          !counter_verification.operational_replay_exact &&
          !counter_verification.transcript_certified(),
      "local accounting and fresh replay reject a falsified node count");
}

}  // namespace

int main() {
  test_singleton_and_identity_guard();
  test_window_invariance_and_complete_transcript();
  test_equal_distance_requires_equality_descent();
  test_morton_collisions_and_boundary_degeneracies();
  test_bounded_verifier_rejects_mutations();

  if (failures != 0) {
    std::cerr << failures << " exact LBVH Yao48 EMST test(s) failed\n";
    return 1;
  }
  return 0;
}
