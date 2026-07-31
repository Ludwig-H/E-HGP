#include "morsehgp3d/hierarchy/exact_q3_gabriel_triangle_oracle.hpp"

#include "morsehgp3d/exact/point.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactQ3GabrielMinimalSupportKind;
using morsehgp3d::hierarchy::ExactQ3GabrielTriangleIds;
using morsehgp3d::hierarchy::ExactQ3GabrielTriangleOracleResult;
using morsehgp3d::hierarchy::ExactQ3GabrielTriangleRecord;
using morsehgp3d::hierarchy::build_exact_q3_gabriel_triangle_oracle;
using morsehgp3d::hierarchy::
    exact_q3_gabriel_triangle_oracle_maximum_point_count;
using morsehgp3d::hierarchy::verify_exact_q3_gabriel_triangle_oracle;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::vector<CertifiedPoint3>& points) {
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] PointId canonical_id_for_source(
    const CanonicalPointCloud& cloud,
    std::size_t source_index) {
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    const PointId point_id = static_cast<PointId>(index);
    if (cloud.source_index(point_id) == source_index) {
      return point_id;
    }
  }
  throw std::logic_error("a fixture source index has no canonical PointId");
}

template <std::size_t Size>
[[nodiscard]] std::array<PointId, Size> canonical_ids_for_sources(
    const CanonicalPointCloud& cloud,
    const std::array<std::size_t, Size>& source_indices) {
  std::array<PointId, Size> result{};
  for (std::size_t index = 0U; index < Size; ++index) {
    result[index] = canonical_id_for_source(cloud, source_indices[index]);
  }
  std::sort(result.begin(), result.end());
  return result;
}

[[nodiscard]] const ExactQ3GabrielTriangleRecord* find_triangle(
    const ExactQ3GabrielTriangleOracleResult& result,
    const ExactQ3GabrielTriangleIds& triangle) {
  const auto position = std::find_if(
      result.triangles.begin(),
      result.triangles.end(),
      [&triangle](const ExactQ3GabrielTriangleRecord& record) {
        return record.triangle_point_ids == triangle;
      });
  return position == result.triangles.end() ? nullptr : &*position;
}

[[nodiscard]] std::vector<std::array<std::size_t, 3U>>
source_triangles_with_support(
    const CanonicalPointCloud& cloud,
    const ExactQ3GabrielTriangleOracleResult& result,
    const std::vector<PointId>& support) {
  std::vector<std::array<std::size_t, 3U>> triangles;
  for (const ExactQ3GabrielTriangleRecord& record : result.triangles) {
    if (record.support_point_ids != support) {
      continue;
    }
    std::array<std::size_t, 3U> sources{};
    for (std::size_t index = 0U; index < sources.size(); ++index) {
      sources[index] = cloud.source_index(record.triangle_point_ids[index]);
    }
    std::sort(sources.begin(), sources.end());
    triangles.push_back(sources);
  }
  std::sort(triangles.begin(), triangles.end());
  return triangles;
}

void require_freshly_certified(
    const CanonicalPointCloud& cloud,
    const ExactQ3GabrielTriangleOracleResult& result,
    const std::string& fixture) {
  const auto verification =
      verify_exact_q3_gabriel_triangle_oracle(cloud, result);
  require(
      result.locally_structurally_valid(),
      fixture + " did not close its local structure");
  require(
      verification.exact_q3_gabriel_triangle_oracle_certified(),
      fixture + " did not pass a fresh exhaustive replay");
  require(
      result.counters.enumerated_unordered_triangle_count ==
              result.counters.expected_unordered_triangle_count &&
          result.counters.local_exact_miniball_build_count ==
              result.counters.expected_unordered_triangle_count &&
          result.counters.full_cloud_point_classification_count ==
              result.counters.expected_unordered_triangle_count * cloud.size(),
      fixture + " did not close C(n,3) miniballs and full-cloud scans");
  require(
      result.counters.bounded_exhaustive_reference_only &&
          !result.counters.future_product_producer_called &&
          !result.counters.delaunay_or_higher_order_mosaic_materialized &&
          !result.counters.gamma2_completeness_claimed &&
          !result.counters.silent_incidence_coverage_certified &&
          !result.counters.hierarchy_reduction_performed &&
          !result.counters.public_status_claimed,
      fixture + " escaped its architecture-only non-Gamma2 scope");
}

void test_four_case_strict_interior_extra_shell_matrix() {
  // Exact C++ replay of
  // tests/fixtures/regressions/
  // gabriel_carrier_strict_interior_extra_shell_matrix.json.
  {
    const std::array<CertifiedPoint3, 4U> input{
        point(-5.0, 0.0, 0.0),
        point(5.0, 0.0, 0.0),
        point(0.0, 3.0, 4.0),
        point(0.0, -3.0, 4.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const auto result = build_exact_q3_gabriel_triangle_oracle(cloud);
    require_freshly_certified(cloud, result, "matrix I=0 E=2 pair case");
    const auto support_array =
        canonical_ids_for_sources(cloud, std::array<std::size_t, 2U>{0U, 1U});
    const std::vector<PointId> support{
        support_array.begin(), support_array.end()};
    require(
        source_triangles_with_support(cloud, result, support) ==
            std::vector<std::array<std::size_t, 3U>>{
                {0U, 1U, 2U}, {0U, 1U, 3U}},
        "I=0 E=2 did not emit one triangle per pair-shell choice");
    for (const ExactQ3GabrielTriangleRecord& record : result.triangles) {
      if (record.support_point_ids == support) {
        require(
            record.minimal_support_kind ==
                    ExactQ3GabrielMinimalSupportKind::pair &&
                record.strict_interior_point_ids.empty() &&
                record.extra_shell_point_ids.size() == 2U &&
                record.exterior_point_ids.empty() &&
                record.squared_level == level(25),
            "I=0 E=2 lost its complete pair-carrier partition");
      }
    }
  }

  {
    const std::array<CertifiedPoint3, 5U> input{
        point(-5.0, 0.0, 0.0),
        point(5.0, 0.0, 0.0),
        point(0.0, 0.0, 0.0),
        point(0.0, 3.0, 4.0),
        point(0.0, -3.0, 4.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const auto result = build_exact_q3_gabriel_triangle_oracle(cloud);
    require_freshly_certified(cloud, result, "matrix I=1 E=2 pair case");
    const auto support_array =
        canonical_ids_for_sources(cloud, std::array<std::size_t, 2U>{0U, 1U});
    const std::vector<PointId> support{
        support_array.begin(), support_array.end()};
    require(
        source_triangles_with_support(cloud, result, support) ==
            std::vector<std::array<std::size_t, 3U>>{{0U, 1U, 2U}},
        "I=1 E=2 did not emit only the triangle containing the interior");
    const auto triangle = canonical_ids_for_sources(
        cloud, std::array<std::size_t, 3U>{0U, 1U, 2U});
    const ExactQ3GabrielTriangleRecord* record = find_triangle(result, triangle);
    require(
        record != nullptr &&
            record->strict_interior_point_ids ==
                std::vector<PointId>{canonical_id_for_source(cloud, 2U)} &&
            record->extra_shell_point_ids.size() == 2U &&
            record->squared_level == level(25),
        "I=1 E=2 lost its complete strict-interior or shell payload");
  }

  {
    const std::array<CertifiedPoint3, 5U> input{
        point(-5.0, 0.0, 0.0),
        point(5.0, 0.0, 0.0),
        point(0.0, 0.0, 0.0),
        point(1.0, 0.0, 0.0),
        point(0.0, 3.0, 4.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const auto result = build_exact_q3_gabriel_triangle_oracle(cloud);
    require_freshly_certified(cloud, result, "matrix I=2 E=1 pair case");
    const auto support_array =
        canonical_ids_for_sources(cloud, std::array<std::size_t, 2U>{0U, 1U});
    const std::vector<PointId> support{
        support_array.begin(), support_array.end()};
    require(
        source_triangles_with_support(cloud, result, support).empty(),
        "I=2 E=1 emitted an impossible three-point pair carrier");
  }

  {
    const std::array<CertifiedPoint3, 4U> input{
        point(5.0, 0.0, 0.0),
        point(-3.0, 4.0, 0.0),
        point(-3.0, -4.0, 0.0),
        point(0.0, 0.0, 5.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const auto result = build_exact_q3_gabriel_triangle_oracle(cloud);
    require_freshly_certified(cloud, result, "matrix support3 I=0 E=1 case");
    const auto triangle = canonical_ids_for_sources(
        cloud, std::array<std::size_t, 3U>{0U, 1U, 2U});
    const ExactQ3GabrielTriangleRecord* record = find_triangle(result, triangle);
    require(
        record != nullptr &&
            record->minimal_support_kind ==
                ExactQ3GabrielMinimalSupportKind::triangle &&
            record->support_point_ids ==
                std::vector<PointId>{triangle.begin(), triangle.end()} &&
            record->strict_interior_point_ids.empty() &&
            record->extra_shell_point_ids ==
                std::vector<PointId>{canonical_id_for_source(cloud, 3U)} &&
            record->exterior_point_ids.empty() &&
            record->squared_level == level(25),
        "support3 I=0 E=1 was rejected by its harmless omitted shell");
  }
}

void test_hartigan_rank_three_triangle_with_all_sides_above_rank() {
  const std::array<CertifiedPoint3, 9U> input{
      point(10.0, 10.0, 10.0),
      point(10.0, -10.0, -10.0),
      point(-10.0, 10.0, -10.0),
      point(17.0, 9.0, 5.0),
      point(18.0, 8.0, 5.0),
      point(3.0, 18.0, 5.0),
      point(3.0, 19.0, 6.0),
      point(-6.0, -8.0, -15.0),
      point(-7.0, -9.0, -14.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const auto result = build_exact_q3_gabriel_triangle_oracle(cloud);
  require_freshly_certified(cloud, result, "Hartigan rank-three fixture");
  const auto target = canonical_ids_for_sources(
      cloud, std::array<std::size_t, 3U>{0U, 1U, 2U});
  const ExactQ3GabrielTriangleRecord* record = find_triangle(result, target);
  require(
      record != nullptr &&
          record->minimal_support_kind ==
              ExactQ3GabrielMinimalSupportKind::triangle &&
          record->support_point_ids ==
              std::vector<PointId>{target.begin(), target.end()} &&
          record->strict_interior_point_ids.empty() &&
          record->extra_shell_point_ids.empty() &&
          record->exterior_point_ids.size() == 6U &&
          record->squared_level == level(800, 3) &&
          record->facet_point_ids[0] ==
              std::array<PointId, 2U>{target[0], target[1]} &&
          record->facet_point_ids[1] ==
              std::array<PointId, 2U>{target[0], target[2]} &&
          record->facet_point_ids[2] ==
              std::array<PointId, 2U>{target[1], target[2]},
      "the Hartigan support-three triangle or its three facets were lost");
}

void test_silent_incidences_remain_explicitly_non_gamma2() {
  // A, B, C, D, E from gabriel_point_set_counterexample.json.
  const std::array<CertifiedPoint3, 5U> input{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const auto result = build_exact_q3_gabriel_triangle_oracle(cloud);
  require_freshly_certified(cloud, result, "silent-incidence fixture");
  const auto acd = canonical_ids_for_sources(
      cloud, std::array<std::size_t, 3U>{0U, 2U, 3U});
  const auto ace = canonical_ids_for_sources(
      cloud, std::array<std::size_t, 3U>{0U, 2U, 4U});
  const auto abc = canonical_ids_for_sources(
      cloud, std::array<std::size_t, 3U>{0U, 1U, 2U});
  require(
      find_triangle(result, acd) == nullptr &&
          find_triangle(result, ace) == nullptr &&
          find_triangle(result, abc) != nullptr &&
          result.counters.rejected_non_gabriel_triangle_count >= 2U,
      "non-Gabriel ACD/ACE silent incidences leaked into the Gabriel oracle");
  require(
      !result.counters.gamma2_completeness_claimed &&
          !result.counters.silent_incidence_coverage_certified,
      "a Gabriel-only triangle oracle claimed the missing Gamma2 incidences");
}

void test_n14_bound_mass_digest_and_input_permutation() {
  std::vector<CertifiedPoint3> forward;
  std::vector<CertifiedPoint3> reverse;
  forward.reserve(exact_q3_gabriel_triangle_oracle_maximum_point_count);
  for (std::size_t index = 0U;
       index < exact_q3_gabriel_triangle_oracle_maximum_point_count;
       ++index) {
    forward.push_back(point(static_cast<double>(index), 0.0, 0.0));
  }
  reverse = forward;
  std::reverse(reverse.begin(), reverse.end());
  const CanonicalPointCloud forward_cloud = canonical_cloud(forward);
  const CanonicalPointCloud reverse_cloud = canonical_cloud(reverse);
  const auto forward_result =
      build_exact_q3_gabriel_triangle_oracle(forward_cloud);
  const auto reverse_result =
      build_exact_q3_gabriel_triangle_oracle(reverse_cloud);
  require_freshly_certified(forward_cloud, forward_result, "n=14 bound");
  require_freshly_certified(
      reverse_cloud, reverse_result, "permuted n=14 bound");
  require(
      forward_result.counters.expected_unordered_triangle_count == 364U &&
          forward_result.counters.full_cloud_point_classification_count ==
              5096U &&
          forward_result == reverse_result,
      "n=14 did not close C(14,3), its full scans, or canonical digest");

  const std::array<CertifiedPoint3, 2U> small_input{
      point(0.0, 0.0, 0.0), point(1.0, 0.0, 0.0)};
  const CanonicalPointCloud small_cloud = canonical_cloud(small_input);
  const auto small_result =
      build_exact_q3_gabriel_triangle_oracle(small_cloud);
  require_freshly_certified(small_cloud, small_result, "n=2 empty q=3");
  require(
      small_result.triangles.empty() &&
          small_result.counters.expected_unordered_triangle_count == 0U,
      "n=2 invented a q=3 triangle");

  const std::array<CertifiedPoint3, 1U> singleton_input{
      point(0.0, 0.0, 0.0)};
  CanonicalPointCloud singleton_cloud = canonical_cloud(singleton_input);
  const auto singleton_result =
      build_exact_q3_gabriel_triangle_oracle(singleton_cloud);
  require_freshly_certified(
      singleton_cloud, singleton_result, "n=1 empty q=3");
  require(
      singleton_result.triangles.empty() &&
          singleton_result.counters.expected_unordered_triangle_count == 0U,
      "n=1 invented a q=3 triangle");

  const CanonicalPointCloud moved_to = std::move(singleton_cloud);
  require(moved_to.size() == 1U, "canonical cloud move lost its singleton");
  bool moved_from_rejected = false;
  try {
    static_cast<void>(
        build_exact_q3_gabriel_triangle_oracle(singleton_cloud));
  } catch (const std::invalid_argument&) {
    moved_from_rejected = true;
  }
  require(
      moved_from_rejected,
      "a moved-from empty canonical cloud escaped the explicit n=0 guard");
  require(
      !verify_exact_q3_gabriel_triangle_oracle(
           singleton_cloud, singleton_result)
           .exact_q3_gabriel_triangle_oracle_certified(),
      "a moved-from empty canonical cloud was certified");

  std::vector<CertifiedPoint3> too_large = forward;
  too_large.push_back(point(14.0, 0.0, 0.0));
  const CanonicalPointCloud too_large_cloud = canonical_cloud(too_large);
  bool rejected = false;
  try {
    static_cast<void>(
        build_exact_q3_gabriel_triangle_oracle(too_large_cloud));
  } catch (const std::invalid_argument&) {
    rejected = true;
  }
  require(rejected, "n=15 escaped the hard q=3 oracle bound");
}

void test_mutations_fail_closed() {
  const std::array<CertifiedPoint3, 4U> input{
      point(5.0, 0.0, 0.0),
      point(-3.0, 4.0, 0.0),
      point(-3.0, -4.0, 0.0),
      point(0.0, 0.0, 5.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactQ3GabrielTriangleOracleResult baseline =
      build_exact_q3_gabriel_triangle_oracle(cloud);
  require_freshly_certified(cloud, baseline, "mutation baseline");
  require(!baseline.triangles.empty(), "mutation baseline has no triangle");

  const auto expect_rejected =
      [&cloud](
          const ExactQ3GabrielTriangleOracleResult& mutated,
          const std::string& mutation) {
        const auto verification =
            verify_exact_q3_gabriel_triangle_oracle(cloud, mutated);
        require(
            !verification.exact_q3_gabriel_triangle_oracle_certified() &&
                !verification.fresh_replay_certified,
            mutation + " was accepted by the fresh verifier");
      };

  {
    auto mutated = baseline;
    ++mutated.schema_version;
    expect_rejected(mutated, "schema mutation");
  }
  {
    auto mutated = baseline;
    mutated.canonical_cloud_digest = {};
    expect_rejected(mutated, "cloud digest mutation");
  }
  {
    auto mutated = baseline;
    mutated.triangles.front().triangle_point_ids[0] =
        mutated.triangles.front().triangle_point_ids[1];
    expect_rejected(mutated, "triangle identity mutation");
  }
  {
    auto mutated = baseline;
    mutated.triangles.front().minimal_support_kind =
        mutated.triangles.front().minimal_support_kind ==
                ExactQ3GabrielMinimalSupportKind::pair
            ? ExactQ3GabrielMinimalSupportKind::triangle
            : ExactQ3GabrielMinimalSupportKind::pair;
    expect_rejected(mutated, "minimal support provenance mutation");
  }
  {
    auto mutated = baseline;
    mutated.triangles.front().strict_interior_point_ids.push_back(
        mutated.triangles.front().triangle_point_ids[0]);
    expect_rejected(mutated, "strict interior mutation");
  }
  {
    auto mutated = baseline;
    const auto position = std::find_if(
        mutated.triangles.begin(),
        mutated.triangles.end(),
        [](const ExactQ3GabrielTriangleRecord& record) {
          return !record.extra_shell_point_ids.empty();
        });
    require(
        position != mutated.triangles.end(),
        "mutation baseline has no extra shell payload");
    position->extra_shell_point_ids.clear();
    expect_rejected(mutated, "extra shell mutation");
  }
  {
    auto mutated = baseline;
    mutated.triangles.front().facet_point_ids[0] =
        mutated.triangles.front().facet_point_ids[1];
    expect_rejected(mutated, "facet mutation");
  }
  {
    auto mutated = baseline;
    mutated.triangles.front().squared_level = level(26);
    expect_rejected(mutated, "exact level mutation");
  }
  {
    auto mutated = baseline;
    ++mutated.counters.enumerated_unordered_triangle_count;
    expect_rejected(mutated, "triangle mass mutation");
  }
  {
    auto mutated = baseline;
    mutated.counters.strictly_inside_classification_count =
        std::numeric_limits<std::size_t>::max();
    mutated.counters.boundary_classification_count = 1U;
    require(
        !mutated.locally_structurally_valid(),
        "a wrapping hostile classification mass passed local validation");
    expect_rejected(mutated, "wrapping classification mass mutation");
  }
  {
    auto mutated = baseline;
    mutated.counters.gamma2_completeness_claimed = true;
    expect_rejected(mutated, "Gamma2 claim mutation");
  }
  {
    auto mutated = baseline;
    mutated.payload_digest = {};
    expect_rejected(mutated, "payload digest mutation");
  }
  if (baseline.triangles.size() > 1U) {
    auto mutated = baseline;
    std::swap(mutated.triangles[0], mutated.triangles[1]);
    expect_rejected(mutated, "canonical order mutation");
  }

  const std::array<CertifiedPoint3, 4U> other_input{
      point(6.0, 0.0, 0.0),
      point(-3.0, 4.0, 0.0),
      point(-3.0, -4.0, 0.0),
      point(0.0, 0.0, 5.0)};
  const CanonicalPointCloud other_cloud = canonical_cloud(other_input);
  require(
      !verify_exact_q3_gabriel_triangle_oracle(other_cloud, baseline)
           .exact_q3_gabriel_triangle_oracle_certified(),
      "a result bound to another cloud was accepted");
}

}  // namespace

int main() {
  try {
    test_four_case_strict_interior_extra_shell_matrix();
    test_hartigan_rank_three_triangle_with_all_sides_above_rank();
    test_silent_incidences_remain_explicitly_non_gamma2();
    test_n14_bound_mass_digest_and_input_permutation();
    test_mutations_fail_closed();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "all bounded exact q=3 Gabriel triangle oracle tests passed\n";
  return 0;
}
