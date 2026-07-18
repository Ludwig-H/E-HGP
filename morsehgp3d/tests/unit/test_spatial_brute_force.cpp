#include "morsehgp3d/spatial/brute_force.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <initializer_list>
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
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExclusionSet;
using morsehgp3d::spatial::PointId;
using morsehgp3d::spatial::brute_force_closed_ball;
using morsehgp3d::spatial::brute_force_nearest;
using morsehgp3d::spatial::brute_force_top_k;

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

template <typename Value>
void self_copy_assign(Value& value) {
  const Value* alias = &value;
  value = *alias;
}

template <typename Value>
void self_move_assign(Value& value) {
  Value* alias = &value;
  value = std::move(*alias);
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactRational3 origin() {
  return ExactRational3{};
}

[[nodiscard]] ExactRational3 rational_query() {
  return ExactRational3{BigInt{3}, BigInt{2}, BigInt{0}, BigInt{6}};
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExclusionSet empty_exclusions(
    const CanonicalPointCloud& cloud, std::size_t run_m_star = 0U) {
  const std::array<PointId, 0> ids{};
  return ExclusionSet::from_ids(
      std::span<const PointId>{ids}, cloud, run_m_star);
}

template <typename Range>
[[nodiscard]] std::vector<PointId> materialize_ids(const Range& range) {
  return std::vector<PointId>{range.begin(), range.end()};
}

template <typename Range>
[[nodiscard]] std::vector<PointId> materialize_ranked_ids(const Range& range) {
  std::vector<PointId> result;
  result.reserve(range.size());
  for (const auto& neighbor : range) {
    result.push_back(neighbor.point_id);
  }
  return result;
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

struct ExpectedRankedPoint {
  PointId point_id;
  ExactLevel squared_distance;
};

template <typename Range>
void check_ranked_points(
    const Range& actual,
    std::initializer_list<ExpectedRankedPoint> expected,
    const std::string& message) {
  check(actual.size() == expected.size(), message + " has the expected size");
  auto actual_iterator = actual.begin();
  auto expected_iterator = expected.begin();
  std::size_t index = 0U;
  while (actual_iterator != actual.end() && expected_iterator != expected.end()) {
    check(
        actual_iterator->point_id == expected_iterator->point_id,
        message + " has point id " + std::to_string(index));
    check(
        actual_iterator->squared_distance == expected_iterator->squared_distance,
        message + " has exact distance " + std::to_string(index));
    ++actual_iterator;
    ++expected_iterator;
    ++index;
  }
}

template <typename Range>
[[nodiscard]] std::vector<PointId> without_exclusions(
    const Range& ids, const ExclusionSet& exclusions) {
  std::vector<PointId> result;
  for (const PointId id : ids) {
    if (!exclusions.contains(id)) {
      result.push_back(id);
    }
  }
  return result;
}

void test_rational_query_complete_cutoff_shell() {
  const std::array<CertifiedPoint3, 4> input{
      point(1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.5, 0.0, 0.0),
      point(0.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);

  check(cloud.size() == 4U, "the rational fixture has four canonical points");
  check(
      cloud.point(PointId{0}).exact() == point(0.0, 0.0, 0.0).exact() &&
          cloud.source_index(PointId{0}) == 3U,
      "canonical id zero retains the origin and its source index");
  check(
      cloud.point(PointId{1}).exact() == point(0.0, 1.0, 0.0).exact() &&
          cloud.source_index(PointId{1}) == 1U,
      "canonical id one retains the y-axis point and its source index");
  check(
      cloud.point(PointId{2}).exact() == point(0.5, 0.0, 0.0).exact() &&
          cloud.source_index(PointId{2}) == 2U,
      "canonical id two retains the half-axis point and its source index");
  check(
      cloud.point(PointId{3}).exact() == point(1.0, 0.0, 0.0).exact() &&
          cloud.source_index(PointId{3}) == 0U,
      "canonical id three retains the unit x-axis point and its source index");

  const ExclusionSet exclusions = empty_exclusions(cloud);
  const auto top = brute_force_top_k(
      cloud, rational_query(), 2U, exclusions);
  const ExactLevel inner_level{BigInt{1}, BigInt{9}};
  const ExactLevel cutoff{BigInt{13}, BigInt{36}};

  check(
      top.requested_rank() == 2U && top.cutoff_squared_distance() == cutoff,
      "the rational top-2 query has exact cutoff 13/36");
  check_ranked_points(
      top.strict_below(),
      {{PointId{2}, inner_level}},
      "the rational top-2 strict partition");
  check_ids(
      top.cutoff_shell_ids(),
      {PointId{0}, PointId{3}},
      "the rational top-2 query returns the complete cutoff shell");
  check_ids(
      top.canonical_choice_ids(),
      {PointId{0}, PointId{2}},
      "the rational top-2 canonical choice does not replace its shell");
  check(
      top.strict_below().size() < top.requested_rank() &&
          top.requested_rank() <=
              top.strict_below().size() + top.cutoff_shell_ids().size() &&
          top.eligible_point_count() == 4U &&
          top.distance_evaluation_count() == 4U && top.shell_complete(),
      "the rational top-2 partition satisfies the cutoff invariant and counter");

  const auto ball = brute_force_closed_ball(cloud, rational_query(), cutoff);
  check_ids(
      ball.interior_ids(),
      {PointId{2}},
      "the rational closed ball has one strict interior point");
  check_ids(
      ball.shell_ids(),
      {PointId{0}, PointId{3}},
      "the rational closed ball has the complete two-point shell");
  check_ids(
      ball.exterior_ids(),
      {PointId{1}},
      "the rational closed ball has one exterior point");
  check(
      ball.squared_radius() == cutoff && ball.closed_rank() == 3U &&
          ball.evaluation_count() == 4U && ball.partition_complete(),
      "the rational closed ball reports its exact radius, global rank and counter");
}

void test_all_six_axis_cominimizers() {
  const std::array<CertifiedPoint3, 6> input{
      point(0.0, 0.0, 1.0),
      point(1.0, 0.0, 0.0),
      point(0.0, -1.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.0, 0.0, -1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExclusionSet exclusions = empty_exclusions(cloud);
  const auto top = brute_force_top_k(cloud, origin(), 3U, exclusions);

  check(top.cutoff_squared_distance() == ExactLevel{BigInt{1}},
        "the six-axis top-3 cutoff is exactly one");
  check(top.strict_below().empty(),
        "the six-axis top-3 strict partition is empty");
  check_ids(
      top.cutoff_shell_ids(),
      {PointId{0}, PointId{1}, PointId{2}, PointId{3}, PointId{4}, PointId{5}},
      "the six-axis top-3 query keeps all six co-minimizers");
  check_ids(
      top.canonical_choice_ids(),
      {PointId{0}, PointId{1}, PointId{2}},
      "the six-axis top-3 canonical choice uses the three smallest ids");
  check(top.eligible_point_count() == 6U &&
            top.distance_evaluation_count() == 6U && top.shell_complete(),
        "the six-axis top-3 query evaluates all points");

  const auto nearest = brute_force_nearest(cloud, origin(), exclusions);
  check(
      nearest.requested_rank() == 1U && nearest.strict_below().empty() &&
          nearest.cutoff_squared_distance() == ExactLevel{BigInt{1}},
      "nearest is the rank-one partition at exact level one");
  check_ids(
      nearest.cutoff_shell_ids(),
      {PointId{0}, PointId{1}, PointId{2}, PointId{3}, PointId{4}, PointId{5}},
      "nearest returns all six co-minimizers");
  check_ids(
      nearest.canonical_choice_ids(),
      {PointId{0}},
      "nearest exposes a canonical representative without truncating the shell");
}

void test_signed_zero_duplicate_rejection() {
  const std::array<CertifiedPoint3, 2> signed_zero_duplicates{
      point(0.0, 1.0, -2.0), point(-0.0, 1.0, -2.0)};
  check_throws<std::invalid_argument>(
      [&signed_zero_duplicates] {
        static_cast<void>(CanonicalPointCloud::rejecting_duplicates(
            std::span<const CertifiedPoint3>{signed_zero_duplicates}));
      },
      "signed-zero variants are rejected as one exact geometric point");

  const std::array<CertifiedPoint3, 1> negative_zero_input{
      point(-0.0, 1.0, 2.0)};
  const CanonicalPointCloud canonical_zero = canonical_cloud(negative_zero_input);
  check(
      canonical_zero.point(PointId{0}).canonical_input_bits()[0] == 0U,
      "a retained negative zero is stored as canonical positive zero");

  const std::array<CertifiedPoint3, 0> empty_input{};
  check_throws<std::invalid_argument>(
      [&empty_input] {
        static_cast<void>(CanonicalPointCloud::rejecting_duplicates(
            std::span<const CertifiedPoint3>{empty_input}));
      },
      "an empty point cloud is rejected");
  check_throws<std::out_of_range>(
      [&canonical_zero] {
        static_cast<void>(canonical_zero.point(PointId{1}));
      },
      "a point id outside the canonical cloud is rejected");
}

void test_permutation_invariant_canonicalization_and_provenance() {
  const CertifiedPoint3 negative = point(-1.0, 3.0, 0.0);
  const CertifiedPoint3 middle = point(0.0, -4.0, 0.0);
  const CertifiedPoint3 positive = point(2.0, 1.0, 0.0);
  const std::array<CertifiedPoint3, 3> first_input{positive, negative, middle};
  const std::array<CertifiedPoint3, 3> second_input{middle, positive, negative};
  const CanonicalPointCloud first = canonical_cloud(first_input);
  const CanonicalPointCloud second = canonical_cloud(second_input);

  for (PointId id = 0U; id < PointId{3}; ++id) {
    check(
        first.point(id).canonical_input_bits() ==
            second.point(id).canonical_input_bits(),
        "canonical point ids are independent of input permutation");
  }
  check(
      first.source_index(PointId{0}) == 1U &&
          first.source_index(PointId{1}) == 2U &&
          first.source_index(PointId{2}) == 0U,
      "the first permutation preserves exact source provenance");
  check(
      second.source_index(PointId{0}) == 2U &&
          second.source_index(PointId{1}) == 0U &&
          second.source_index(PointId{2}) == 1U,
      "the second permutation preserves exact source provenance");
}

void test_one_ulp_distance_order() {
  const double below = std::nextafter(1.0, 0.0);
  const double above =
      std::nextafter(1.0, std::numeric_limits<double>::infinity());
  const std::array<CertifiedPoint3, 3> input{
      point(above, 0.0, 0.0),
      point(below, 0.0, 0.0),
      point(1.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExclusionSet exclusions = empty_exclusions(cloud);
  const ExactRational below_exact = ExactRational::from_binary64(below);
  const ExactLevel below_squared{below_exact * below_exact};
  const auto top = brute_force_top_k(cloud, origin(), 2U, exclusions);

  check_ranked_points(
      top.strict_below(),
      {{PointId{0}, below_squared}},
      "one ULP below one remains strictly below the unit cutoff");
  check(
      top.cutoff_squared_distance() == ExactLevel{BigInt{1}},
      "the one-ULP fixture has exact unit cutoff");
  check_ids(
      top.cutoff_shell_ids(),
      {PointId{1}},
      "only the exact unit point lies on the one-ULP cutoff shell");
  check_ids(
      top.canonical_choice_ids(),
      {PointId{0}, PointId{1}},
      "the one-ULP canonical order follows exact squared distances");

  const auto unit_ball = brute_force_closed_ball(
      cloud, origin(), ExactLevel{BigInt{1}});
  check_ids(unit_ball.interior_ids(), {PointId{0}},
            "the unit ball contains the below-one point strictly");
  check_ids(unit_ball.shell_ids(), {PointId{1}},
            "the unit ball places the exact-one point on its shell");
  check_ids(unit_ball.exterior_ids(), {PointId{2}},
            "the unit ball excludes the above-one point exactly");
}

void test_extreme_finite_distances() {
  const double maximum = std::numeric_limits<double>::max();
  const double tiny = std::numeric_limits<double>::denorm_min();
  const std::array<CertifiedPoint3, 4> input{
      point(maximum, 0.0, 0.0),
      point(-tiny, 0.0, 0.0),
      point(-maximum, 0.0, 0.0),
      point(tiny, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExclusionSet exclusions = empty_exclusions(cloud);
  const ExactRational tiny_exact = ExactRational::from_binary64(tiny);
  const ExactRational maximum_exact = ExactRational::from_binary64(maximum);
  const ExactLevel tiny_squared{tiny_exact * tiny_exact};
  const ExactLevel maximum_squared{maximum_exact * maximum_exact};

  const auto nearest = brute_force_nearest(cloud, origin(), exclusions);
  check(
      nearest.cutoff_squared_distance() == tiny_squared &&
          !nearest.cutoff_squared_distance().rational().is_zero(),
      "minimum-subnormal distances remain positive instead of underflowing");
  check(nearest.strict_below().empty(),
        "both minimum-subnormal points lie on the first shell");
  check_ids(nearest.cutoff_shell_ids(), {PointId{1}, PointId{2}},
            "both signed minimum-subnormal points form the exact shell");

  const auto top = brute_force_top_k(cloud, origin(), 3U, exclusions);
  check_ranked_points(
      top.strict_below(),
      {{PointId{1}, tiny_squared}, {PointId{2}, tiny_squared}},
      "the extreme top-3 query retains exact tiny squared distances");
  check(
      top.cutoff_squared_distance() == maximum_squared,
      "maximum-finite squared distance remains exact instead of overflowing");
  check_ids(top.cutoff_shell_ids(), {PointId{0}, PointId{3}},
            "both maximum-finite points form the exact outer shell");
  check_ids(top.canonical_choice_ids(),
            {PointId{0}, PointId{1}, PointId{2}},
            "the extreme top-3 canonical choice respects exact distance order");

  const auto tiny_ball = brute_force_closed_ball(cloud, origin(), tiny_squared);
  check(tiny_ball.interior_ids().empty(),
        "the tiny exact ball has no strict interior point");
  check_ids(tiny_ball.shell_ids(), {PointId{1}, PointId{2}},
            "the tiny exact ball keeps both subnormal shell points");
  check_ids(tiny_ball.exterior_ids(), {PointId{0}, PointId{3}},
            "the tiny exact ball classifies maximum-finite points outside");
}

void test_exclusion_contract_and_global_ball_invariant() {
  const std::array<CertifiedPoint3, 11> input{
      point(-5.0, 0.0, 0.0),
      point(-4.0, 0.0, 0.0),
      point(-3.0, 0.0, 0.0),
      point(-2.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(3.0, 0.0, 0.0),
      point(4.0, 0.0, 0.0),
      point(5.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 9> excluded_ids{
      PointId{10}, PointId{0}, PointId{8},
      PointId{1}, PointId{7}, PointId{2},
      PointId{5}, PointId{3}, PointId{9}};
  const ExclusionSet exclusions = ExclusionSet::from_ids(
      std::span<const PointId>{excluded_ids}, cloud, 9U);

  check(
      exclusions.point_count() == 11U && exclusions.run_m_star() == 9U &&
          exclusions.ids().size() == 9U,
      "the target profile accepts exactly nine exclusions");
  check_ids(
      exclusions.ids(),
      {PointId{0}, PointId{1}, PointId{2}, PointId{3}, PointId{5},
       PointId{7}, PointId{8}, PointId{9}, PointId{10}},
      "exclusion ids are stored sorted and unique");
  check(
      exclusions.contains(PointId{5}) && !exclusions.contains(PointId{4}),
      "exclusion membership distinguishes excluded and eligible ids");

  const auto top = brute_force_top_k(cloud, origin(), 1U, exclusions);
  check(top.strict_below().empty(),
        "the eligible symmetric pair has no point below its cutoff");
  check_ids(top.cutoff_shell_ids(), {PointId{4}, PointId{6}},
            "top-k preserves both eligible points on the cutoff shell");
  check_ids(top.canonical_choice_ids(), {PointId{4}},
            "top-k chooses the smallest eligible shell id canonically");
  check(top.eligible_point_count() == 2U &&
            top.distance_evaluation_count() == 2U && top.shell_complete(),
        "top-k counts only the two eligible point evaluations");

  const auto global_ball = brute_force_closed_ball(
      cloud, origin(), top.cutoff_squared_distance());
  const std::vector<PointId> eligible_interior =
      without_exclusions(global_ball.interior_ids(), exclusions);
  const std::vector<PointId> eligible_shell =
      without_exclusions(global_ball.shell_ids(), exclusions);
  check_ids(eligible_interior, {},
            "filtering the global ball reproduces the eligible strict set");
  check_ids(eligible_shell, {PointId{4}, PointId{6}},
            "filtering the global ball reproduces the eligible complete shell");
  check(
      materialize_ids(top.cutoff_shell_ids()) == eligible_shell &&
          materialize_ranked_ids(top.strict_below()) == eligible_interior,
      "top-k agrees with the global ball after applying the same exclusions");
  check(
      global_ball.closed_rank() == 3U &&
          global_ball.evaluation_count() == 11U &&
          global_ball.partition_complete(),
      "the global closed rank and counter are not confused with excluded top-k");

  const std::array<PointId, 10> ten_ids{
      PointId{0}, PointId{1}, PointId{2}, PointId{3}, PointId{4},
      PointId{5}, PointId{6}, PointId{7}, PointId{8}, PointId{9}};
  check_throws<std::invalid_argument>(
      [&cloud, &ten_ids] {
        static_cast<void>(ExclusionSet::from_ids(
            std::span<const PointId>{ten_ids}, cloud, 9U));
      },
      "ten exclusions exceed the target m-star bound");

  const std::array<PointId, 2> duplicate_ids{PointId{1}, PointId{1}};
  check_throws<std::invalid_argument>(
      [&cloud, &duplicate_ids] {
        static_cast<void>(ExclusionSet::from_ids(
            std::span<const PointId>{duplicate_ids}, cloud, 9U));
      },
      "duplicate exclusion ids are rejected rather than deduplicated");

  const std::array<PointId, 1> out_of_range_id{PointId{11}};
  check_throws<std::out_of_range>(
      [&cloud, &out_of_range_id] {
        static_cast<void>(ExclusionSet::from_ids(
            std::span<const PointId>{out_of_range_id}, cloud, 9U));
      },
      "an exclusion id outside the point cloud is rejected");

  const std::array<PointId, 0> no_ids{};
  check_throws<std::invalid_argument>(
      [&cloud, &no_ids] {
        CanonicalPointCloud moved_from_cloud = cloud;
        const CanonicalPointCloud move_target = std::move(moved_from_cloud);
        static_cast<void>(move_target);
        static_cast<void>(ExclusionSet::from_ids(
            std::span<const PointId>{no_ids}, moved_from_cloud, 0U));
      },
      "an exclusion set cannot claim an empty canonical point cloud");
  check_throws<std::invalid_argument>(
      [&cloud, &no_ids] {
        static_cast<void>(ExclusionSet::from_ids(
            std::span<const PointId>{no_ids}, cloud, 10U));
      },
      "run m-star above nine is rejected even for an empty exclusion set");
  const ExclusionSet zero_depth = ExclusionSet::from_ids(
      std::span<const PointId>{no_ids}, cloud, 0U);
  check(zero_depth.ids().empty() && zero_depth.run_m_star() == 0U,
        "run m-star zero is valid when the exclusion set is empty");
  check_throws<std::invalid_argument>(
      [&no_ids] {
        const std::array<CertifiedPoint3, 2> two_points{
            point(-1.0, 0.0, 0.0), point(1.0, 0.0, 0.0)};
        const CanonicalPointCloud two_point_cloud = canonical_cloud(two_points);
        static_cast<void>(ExclusionSet::from_ids(
            std::span<const PointId>{no_ids}, two_point_cloud, 1U));
      },
      "run m-star cannot exceed the refinement depth of a small cloud");

  check_throws<std::out_of_range>(
      [&cloud, &exclusions] {
        static_cast<void>(brute_force_top_k(
            cloud, origin(), 0U, exclusions));
      },
      "top-k rank zero is rejected");
  check_throws<std::out_of_range>(
      [&cloud, &exclusions] {
        static_cast<void>(brute_force_top_k(
            cloud, origin(), 3U, exclusions));
      },
      "top-k rank above the eligible population is rejected");

  const std::array<CertifiedPoint3, 3> other_input{
      point(-1.0, 0.0, 0.0), point(0.0, 0.0, 0.0), point(1.0, 0.0, 0.0)};
  const CanonicalPointCloud other_cloud = canonical_cloud(other_input);
  check_throws<std::invalid_argument>(
      [&other_cloud, &exclusions] {
        static_cast<void>(brute_force_top_k(
            other_cloud, origin(), 1U, exclusions));
      },
      "an exclusion set cannot be replayed against a mismatched point cloud");
}

void test_relevant_gp_shell_is_never_rank_truncated() {
  const std::array<CertifiedPoint3, 3> input{
      point(-1.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const auto ball = brute_force_closed_ball(
      cloud, origin(), ExactLevel{BigInt{1}});

  check(ball.interior_ids().empty(),
        "the RelevantGP fixture satisfies the empty-interior antecedent");
  check_ids(ball.shell_ids(), {PointId{0}, PointId{1}, PointId{2}},
            "a shell of size three is complete even above a useful rank two");
  check(ball.exterior_ids().empty(),
        "all three RelevantGP fixture points lie exactly on the sphere");
  check(ball.closed_rank() == 3U && ball.evaluation_count() == 3U &&
            ball.partition_complete(),
        "the RelevantGP fixture exposes its full global closed rank");
}

void test_namespace_binding_and_move_invalidation() {
  const std::array<CertifiedPoint3, 3> input{
      point(-1.0, 0.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0)};
  CanonicalPointCloud cloud = canonical_cloud(input);
  const ExclusionSet exclusions = empty_exclusions(cloud);

  const CanonicalPointCloud copied_cloud = cloud;
  check(
      brute_force_nearest(copied_cloud, origin(), exclusions).shell_complete(),
      "a copied cloud preserves its canonical PointId namespace token");

  const std::array<CertifiedPoint3, 3> unrelated_input{
      point(-2.0, 0.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0)};
  const CanonicalPointCloud unrelated_cloud = canonical_cloud(unrelated_input);
  check_throws<std::invalid_argument>(
      [&unrelated_cloud, &exclusions] {
        static_cast<void>(brute_force_nearest(
            unrelated_cloud, origin(), exclusions));
      },
      "equal point counts do not merge distinct canonical PointId namespaces");

  CanonicalPointCloud assigned_cloud = unrelated_cloud;
  assigned_cloud = cloud;
  check(
      brute_force_nearest(
          assigned_cloud, origin(), exclusions).shell_complete(),
      "cloud copy assignment transactionally adopts the source namespace");
  self_copy_assign(assigned_cloud);
  self_move_assign(assigned_cloud);
  check(
      assigned_cloud.size() == cloud.size() &&
          brute_force_nearest(
              assigned_cloud, origin(), exclusions).shell_complete(),
      "cloud self-copy and self-move preserve a valid namespace");

  const std::array<PointId, 1> excluded_id{PointId{0}};
  ExclusionSet nonempty_exclusions = ExclusionSet::from_ids(
      std::span<const PointId>{excluded_id}, cloud, 1U);
  self_copy_assign(nonempty_exclusions);
  self_move_assign(nonempty_exclusions);
  check(
      nonempty_exclusions.validated_for(cloud) &&
          nonempty_exclusions.point_count() == 3U &&
          nonempty_exclusions.run_m_star() == 1U &&
          nonempty_exclusions.ids().size() == 1U &&
          nonempty_exclusions.contains(PointId{0}),
      "exclusion self-copy and self-move cannot silently erase an exclusion");
  ExclusionSet assigned_exclusions = exclusions;
  assigned_exclusions = nonempty_exclusions;
  check(
      assigned_exclusions.validated_for(cloud) &&
          assigned_exclusions.contains(PointId{0}),
      "exclusion copy assignment transactionally preserves its payload");

  ExclusionSet exclusion_source = exclusions;
  ExclusionSet exclusion_target = std::move(exclusion_source);
  check(
      exclusion_source.point_count() == 0U &&
          exclusion_source.run_m_star() == 0U &&
          exclusion_source.ids().empty() &&
          !exclusion_source.contains(PointId{0}),
      "a moved-from exclusion set becomes explicitly invalid and empty");
  check_throws<std::invalid_argument>(
      [&cloud, &exclusion_source] {
        static_cast<void>(brute_force_nearest(
            cloud, origin(), exclusion_source));
      },
      "a moved-from exclusion set cannot alter a later query");
  check(
      brute_force_nearest(cloud, origin(), exclusion_target).shell_complete(),
      "the move target retains the valid exclusion namespace");

  auto top_source = brute_force_top_k(cloud, origin(), 3U, exclusions);
  auto top_target = std::move(top_source);
  check(
      !top_source.shell_complete() && top_source.requested_rank() == 0U &&
          top_source.strict_below().empty() &&
          top_source.cutoff_shell_ids().empty() &&
          top_source.canonical_choice_ids().empty() &&
          top_source.eligible_point_count() == 0U &&
          top_source.distance_evaluation_count() == 0U,
      "a moved-from top-k result cannot retain a complete certificate");
  check_throws<std::logic_error>(
      [&top_source] {
        static_cast<void>(top_source.cutoff_squared_distance());
      },
      "a moved-from top-k result exposes no stale cutoff");
  check(
      top_target.shell_complete() && top_target.requested_rank() == 3U,
      "the top-k move target retains the complete certificate");
  self_copy_assign(top_target);
  self_move_assign(top_target);
  check(
      top_target.shell_complete() && top_target.requested_rank() == 3U,
      "top-k self-copy and self-move preserve the complete certificate");
  auto top_assigned = brute_force_top_k(cloud, origin(), 1U, exclusions);
  top_assigned = top_target;
  check(
      top_assigned.shell_complete() && top_assigned.requested_rank() == 3U &&
          materialize_ids(top_assigned.canonical_choice_ids()) ==
              materialize_ids(top_target.canonical_choice_ids()),
      "top-k copy assignment transactionally replaces a certificate");

  auto ball_source = brute_force_closed_ball(
      cloud, origin(), ExactLevel{BigInt{0}});
  check_ids(ball_source.interior_ids(), {},
            "the zero-radius ball has no strict interior");
  check_ids(ball_source.shell_ids(), {PointId{1}},
            "the zero-radius ball keeps the coincident point on its shell");
  check_ids(ball_source.exterior_ids(), {PointId{0}, PointId{2}},
            "the zero-radius ball classifies the nonzero points outside");
  auto ball_target = std::move(ball_source);
  check(
      !ball_source.partition_complete() && ball_source.closed_rank() == 0U &&
          ball_source.evaluation_count() == 0U &&
          ball_source.interior_ids().empty() && ball_source.shell_ids().empty() &&
          ball_source.exterior_ids().empty(),
      "a moved-from ball cannot retain a complete partition certificate");
  check_throws<std::logic_error>(
      [&ball_source] {
        static_cast<void>(ball_source.squared_radius());
      },
      "a moved-from ball exposes no stale radius");
  check(
      ball_target.partition_complete() && ball_target.closed_rank() == 1U,
      "the ball move target retains the complete partition");
  self_copy_assign(ball_target);
  self_move_assign(ball_target);
  check(
      ball_target.partition_complete() && ball_target.closed_rank() == 1U,
      "ball self-copy and self-move preserve the complete partition");
  auto ball_assigned = brute_force_closed_ball(
      cloud, origin(), ExactLevel{BigInt{4}});
  ball_assigned = ball_target;
  check(
      ball_assigned.partition_complete() &&
          ball_assigned.closed_rank() == ball_target.closed_rank() &&
          materialize_ids(ball_assigned.shell_ids()) ==
              materialize_ids(ball_target.shell_ids()),
      "ball copy assignment transactionally replaces a partition");

  const ExclusionSet move_cloud_exclusions = empty_exclusions(cloud);
  CanonicalPointCloud moved_cloud = std::move(cloud);
  check(cloud.size() == 0U,
        "a moved-from cloud reports no queryable points");
  check_throws<std::invalid_argument>(
      [&cloud] {
        static_cast<void>(brute_force_closed_ball(
            cloud, origin(), ExactLevel{BigInt{0}}));
      },
      "a moved-from cloud cannot publish an empty complete partition");
  check(
      brute_force_nearest(
          moved_cloud, origin(), move_cloud_exclusions).shell_complete(),
      "the cloud move target retains its PointId namespace");
}

}  // namespace

int main() {
  test_rational_query_complete_cutoff_shell();
  test_all_six_axis_cominimizers();
  test_signed_zero_duplicate_rejection();
  test_permutation_invariant_canonicalization_and_provenance();
  test_one_ulp_distance_order();
  test_extreme_finite_distances();
  test_exclusion_contract_and_global_ball_invariant();
  test_relevant_gp_shell_is_never_rank_truncated();
  test_namespace_binding_and_move_invalidation();

  if (failures != 0) {
    std::cerr << failures << " spatial brute-force test(s) failed\n";
    return 1;
  }
  std::cout << "MorseHGP3D spatial brute-force tests passed\n";
  return 0;
}
