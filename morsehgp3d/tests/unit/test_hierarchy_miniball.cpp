#include "morsehgp3d/hierarchy/miniball.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactCenter3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactFacetDescentPreconditionDecision;
using morsehgp3d::hierarchy::ExactFacetDescentPreconditionResult;
using morsehgp3d::hierarchy::ExactFacetDescentPreconditionScope;
using morsehgp3d::hierarchy::ExactFacetDescentPreconditionVerification;
using morsehgp3d::hierarchy::ExactFacetMiniballResult;
using morsehgp3d::hierarchy::ExactFacetMiniballScope;
using morsehgp3d::hierarchy::ExactFacetMiniballStatus;
using morsehgp3d::hierarchy::ExactFacetMiniballVerification;
using morsehgp3d::hierarchy::build_exact_facet_descent_preconditions;
using morsehgp3d::hierarchy::build_exact_facet_miniball;
using morsehgp3d::hierarchy::verify_exact_facet_descent_preconditions;
using morsehgp3d::hierarchy::verify_exact_facet_miniball;
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
    std::cerr << "FAIL: " << message
              << " (unexpected exception: " << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CertifiedPoint3 point(
    double x, double y = 0.0, double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactCenter3 center(
    std::int64_t x,
    std::int64_t y,
    std::int64_t z,
    std::int64_t denominator = 1) {
  return ExactCenter3{
      BigInt{x}, BigInt{y}, BigInt{z}, BigInt{denominator}};
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] bool all_certificates_close(
    const ExactFacetMiniballVerification& verification) {
  return verification.facet_identity_certified &&
         verification.exhaustive_support_enumeration_certified &&
         verification.exact_center_and_radius_certified &&
         verification.enclosing_partition_certified &&
         verification.canonical_support_certified &&
         verification.counters_certified &&
         verification.status_certified &&
         verification.local_scope_certified &&
         verification.fresh_replay_certified &&
         verification.local_exact_facet_miniball_certified;
}

[[nodiscard]] bool all_descent_precondition_certificates_close(
    const ExactFacetDescentPreconditionVerification& verification) {
  return verification.facet_miniball_certified &&
         verification.global_closed_ball_identity_certified &&
         verification.global_closed_ball_partition_certified &&
         verification.exact_top_k_identity_certified &&
         verification.exact_top_k_partition_certified &&
         verification.top_k_cutoff_bound_certified &&
         verification.local_boundary_decision_certified &&
         verification.global_shell_decision_certified &&
         verification.facet_top_k_membership_decision_certified &&
         verification.counters_certified &&
         verification.decision_certified && verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.exact_descent_preconditions_certified;
}

[[nodiscard]] bool matches_ids(
    std::span<const PointId> actual,
    std::initializer_list<PointId> expected) {
  return actual.size() == expected.size() &&
         std::equal(actual.begin(), actual.end(), expected.begin());
}

void test_singleton_and_default_statuses() {
  const ExactFacetMiniballResult empty;
  check(
      empty.status == ExactFacetMiniballStatus::not_certified &&
          empty.scope == ExactFacetMiniballScope::unspecified,
      "default facet miniball certifies neither result nor scope");

  const std::array<CertifiedPoint3, 1> input{point(2.0, -3.0, 4.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 1> facet{0U};
  const ExactFacetMiniballResult result =
      build_exact_facet_miniball(cloud, facet);
  const ExactFacetMiniballVerification verification =
      verify_exact_facet_miniball(cloud, facet, result);

  check(
      result.facet_point_ids == std::vector<PointId>{0U} &&
          result.support_point_ids == std::vector<PointId>{0U} &&
          result.strictly_inside_point_ids.empty() &&
          result.boundary_point_ids == std::vector<PointId>{0U},
      "singleton is its own exact support and boundary");
  check(
      result.center == center(2, -3, 4) &&
          result.squared_radius == ExactLevel{},
      "singleton has its exact point as center and zero radius");
  check(
      result.counters.enumerated_support_count_by_size ==
              std::array<std::size_t, 4>{1U, 0U, 0U, 0U} &&
          result.counters.enumerated_support_count == 1U &&
          result.counters.minimal_support_candidate_count == 1U &&
          result.counters.candidate_point_classification_count == 1U &&
          result.counters.candidate_boundary_classification_count == 1U &&
          result.counters.enclosing_support_count == 1U &&
          result.counters.optimal_support_count == 1U &&
          result.counters.selected_support_size == 1U,
      "singleton exhaustive counters close");
  check(
      result.status ==
              ExactFacetMiniballStatus::exact_facet_miniball_certified &&
          result.scope ==
              ExactFacetMiniballScope::local_facet_miniball_only &&
          all_certificates_close(verification),
      "singleton publishes only its local exact miniball certificate");
  check(
      ExactFacetMiniballResult::maximum_facet_point_count == 10U &&
          ExactFacetMiniballResult::maximum_support_point_count == 4U &&
          ExactFacetMiniballResult::maximum_enumerated_support_count == 385U &&
          std::string_view{ExactFacetMiniballResult::proof_basis} ==
              "exhaustive_exact_supports_up_to_four_facet_miniball_v1",
      "facet miniball exposes its bounded proof basis");
}

void test_triangle_support_reductions() {
  {
    const std::array<CertifiedPoint3, 3> input{
        point(0.0, 0.0), point(1.0, 1.0), point(4.0, 0.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const std::array<PointId, 3> facet{2U, 0U, 1U};
    const ExactFacetMiniballResult result =
        build_exact_facet_miniball(cloud, facet);
    check(
        result.facet_point_ids == std::vector<PointId>({0U, 1U, 2U}) &&
            result.support_point_ids == std::vector<PointId>({0U, 2U}) &&
            result.strictly_inside_point_ids ==
                std::vector<PointId>({1U}) &&
            result.boundary_point_ids ==
                std::vector<PointId>({0U, 2U}),
        "obtuse triangle reduces to its exact diameter pair");
    check(
        result.center == center(2, 0, 0) &&
            result.squared_radius == level(4),
        "obtuse triangle diameter has exact center and squared radius");
  }
  {
    const std::array<CertifiedPoint3, 3> input{
        point(0.0, 0.0), point(0.0, 2.0), point(2.0, 0.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const std::array<PointId, 3> facet{0U, 1U, 2U};
    const ExactFacetMiniballResult result =
        build_exact_facet_miniball(cloud, facet);
    check(
        result.support_point_ids == std::vector<PointId>({1U, 2U}) &&
            result.strictly_inside_point_ids.empty() &&
            result.boundary_point_ids ==
                std::vector<PointId>({0U, 1U, 2U}) &&
            result.center == center(1, 1, 0) &&
            result.squared_radius == level(2),
        "right triangle exposes a reduced support and its extra shell point");
    check(
        result.counters.boundary_reduced_support_count >= 1U,
        "right triangle records the nonessential three-point support");
  }
  {
    const std::array<CertifiedPoint3, 3> input{
        point(0.0, 0.0), point(1.0, 3.0), point(4.0, 0.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const std::array<PointId, 3> facet{0U, 1U, 2U};
    const ExactFacetMiniballResult result =
        build_exact_facet_miniball(cloud, facet);
    check(
        result.support_point_ids == std::vector<PointId>({0U, 1U, 2U}) &&
            result.center == center(2, 1, 0) &&
            result.squared_radius == level(5) &&
            result.boundary_point_ids ==
                std::vector<PointId>({0U, 1U, 2U}),
        "acute triangle keeps its three-point exact support");
  }
}

void test_tetrahedron_supports() {
  {
    const std::array<CertifiedPoint3, 4> input{
        point(-1.0, -1.0, 1.0),
        point(-1.0, 1.0, -1.0),
        point(1.0, -1.0, -1.0),
        point(1.0, 1.0, 1.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const std::array<PointId, 4> facet{0U, 1U, 2U, 3U};
    const ExactFacetMiniballResult result =
        build_exact_facet_miniball(cloud, facet);
    check(
        result.support_point_ids ==
                std::vector<PointId>({0U, 1U, 2U, 3U}) &&
            result.center == center(0, 0, 0) &&
            result.squared_radius == level(3) &&
            result.strictly_inside_point_ids.empty() &&
            result.boundary_point_ids ==
                std::vector<PointId>({0U, 1U, 2U, 3U}),
        "regular tetrahedron requires its four-point exact support");
    check(
        all_certificates_close(
            verify_exact_facet_miniball(cloud, facet, result)),
        "regular tetrahedron closes the fresh exhaustive replay");
  }
  {
    const std::array<CertifiedPoint3, 4> input{
        point(0.0, 0.0, 0.0),
        point(0.0, 0.0, 2.0),
        point(0.0, 2.0, 0.0),
        point(2.0, 0.0, 0.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const std::array<PointId, 4> facet{0U, 1U, 2U, 3U};
    const ExactFacetMiniballResult result =
        build_exact_facet_miniball(cloud, facet);
    check(
        result.support_point_ids == std::vector<PointId>({1U, 2U, 3U}) &&
            result.center == center(2, 2, 2, 3) &&
            result.squared_radius == level(8, 3) &&
            result.strictly_inside_point_ids ==
                std::vector<PointId>({0U}) &&
            result.boundary_point_ids ==
                std::vector<PointId>({1U, 2U, 3U}),
        "non-well-centred tetrahedron reduces to its opposite face");
    check(
        result.counters.exterior_circumcenter_support_count >= 1U,
        "non-well-centred tetrahedron records its rejected four-point center");
  }
}

void test_square_exposes_multiple_optimal_supports() {
  const std::array<CertifiedPoint3, 4> input{
      point(-1.0, -1.0),
      point(-1.0, 1.0),
      point(1.0, -1.0),
      point(1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 4> facet{3U, 1U, 0U, 2U};
  const ExactFacetMiniballResult result =
      build_exact_facet_miniball(cloud, facet);
  check(
      result.facet_point_ids ==
              std::vector<PointId>({0U, 1U, 2U, 3U}) &&
          result.support_point_ids == std::vector<PointId>({0U, 3U}) &&
          result.center == center(0, 0, 0) &&
          result.squared_radius == level(2) &&
          result.boundary_point_ids ==
              std::vector<PointId>({0U, 1U, 2U, 3U}),
      "square chooses the smallest lexicographic diagonal without hiding shell points");
  check(
      result.counters.enumerated_support_count_by_size ==
              std::array<std::size_t, 4>{4U, 6U, 4U, 1U} &&
          result.counters.enumerated_support_count == 15U &&
          result.counters.optimal_support_count == 2U &&
          result.counters.selected_support_size == 2U,
      "square exposes both optimal exact supports and one canonical choice");
}

void test_smaller_exterior_circumsupport_is_rejected() {
  const std::array<CertifiedPoint3, 6> input{
      point(-1.0, 0.0, -2.0),
      point(-1.0, 0.0, 2.0),
      point(1.0, -2.0, 0.0),
      point(1.0, 2.0, 0.0),
      point(2.0, -1.0, 0.0),
      point(2.0, 1.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 6> facet{0U, 1U, 2U, 3U, 4U, 5U};
  const ExactFacetMiniballResult result =
      build_exact_facet_miniball(cloud, facet);

  check(
      result.support_point_ids ==
              std::vector<PointId>({0U, 1U, 2U, 3U}) &&
          result.center == center(0, 0, 0) &&
          result.squared_radius == level(5) &&
          result.strictly_inside_point_ids.empty() &&
          result.boundary_point_ids ==
              std::vector<PointId>({0U, 1U, 2U, 3U, 4U, 5U}),
      "positive four-point support wins over a smaller exterior triple");
  check(
      result.counters.exterior_circumcenter_support_count >= 1U &&
          all_certificates_close(
              verify_exact_facet_miniball(cloud, facet, result)),
      "exterior circumsupport regression closes its exact replay");
}

void test_ten_point_bound_is_exactly_385_supports() {
  const std::array<CertifiedPoint3, 10> input{
      point(0.0),
      point(1.0),
      point(2.0),
      point(3.0),
      point(4.0),
      point(5.0),
      point(6.0),
      point(7.0),
      point(8.0),
      point(9.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 10> facet{
      9U, 0U, 8U, 1U, 7U, 2U, 6U, 3U, 5U, 4U};
  const ExactFacetMiniballResult result =
      build_exact_facet_miniball(cloud, facet);
  check(
      result.support_point_ids == std::vector<PointId>({0U, 9U}) &&
          result.center == center(9, 0, 0, 2) &&
          result.squared_radius == level(81, 4) &&
          result.strictly_inside_point_ids ==
              std::vector<PointId>({1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U}) &&
          result.boundary_point_ids == std::vector<PointId>({0U, 9U}),
      "ten collinear points select their two extreme endpoints");
  check(
      result.counters.enumerated_support_count_by_size ==
              std::array<std::size_t, 4>{10U, 45U, 120U, 210U} &&
          result.counters.enumerated_support_count == 385U &&
          result.counters.affinely_dependent_support_count == 330U &&
          result.counters.minimal_support_candidate_count == 55U &&
          result.counters.candidate_point_classification_count == 550U &&
          result.counters.candidate_strictly_inside_classification_count ==
              120U &&
          result.counters.candidate_boundary_classification_count == 100U &&
          result.counters.candidate_outside_classification_count == 330U &&
          result.counters.enclosing_support_count == 1U &&
          result.counters.optimal_support_count == 1U,
      "ten-point exhaustive enumeration closes the 385-support work bound");
  check(
      all_certificates_close(
          verify_exact_facet_miniball(cloud, facet, result)),
      "ten-point bounded oracle closes its fresh replay");
}

void test_strict_descent_preconditions_with_equal_cutoff() {
  const ExactFacetDescentPreconditionResult empty;
  check(
      empty.decision == ExactFacetDescentPreconditionDecision::not_certified &&
          empty.scope == ExactFacetDescentPreconditionScope::unspecified &&
          !empty.global_closed_ball.has_value() &&
          !empty.exact_top_k.has_value(),
      "default descent preconditions certify neither partitions nor decision");

  const std::array<CertifiedPoint3, 3> input{
      point(1.0, 0.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 2> facet{2U, 0U};
  const ExactFacetDescentPreconditionResult result =
      build_exact_facet_descent_preconditions(cloud, facet);
  const ExactFacetDescentPreconditionVerification verification =
      verify_exact_facet_descent_preconditions(cloud, facet, result);

  check(
      result.facet_miniball.facet_point_ids ==
              std::vector<PointId>({0U, 2U}) &&
          result.facet_miniball.support_point_ids ==
              std::vector<PointId>({0U, 2U}) &&
          result.facet_miniball.boundary_point_ids ==
              std::vector<PointId>({0U, 2U}) &&
          result.facet_miniball.center == center(0, 0, 0) &&
          result.facet_miniball.squared_radius == level(1) &&
          result.facet_miniball.counters.optimal_support_count == 1U,
      "antipodal facet has its unique exact diameter support");
  check(
      result.global_closed_ball.has_value() &&
          result.exact_top_k.has_value(),
      "strict precondition result retains both complete exact partitions");
  if (!result.global_closed_ball.has_value() ||
      !result.exact_top_k.has_value()) {
    return;
  }
  const auto& global = *result.global_closed_ball;
  const auto& top_k = *result.exact_top_k;
  check(
      global.validated_for(cloud) && global.squared_radius() == level(1) &&
          matches_ids(global.interior_ids(), {1U}) &&
          matches_ids(global.shell_ids(), {0U, 2U}) &&
          global.exterior_ids().empty() && global.closed_rank() == 3U,
      "center intruder is globally interior to the antipodal miniball");
  check(
      top_k.validated_for(cloud) && top_k.requested_rank() == 2U &&
          top_k.cutoff_squared_distance() == level(1) &&
          top_k.strict_below().size() == 1U &&
          top_k.strict_below()[0].point_id == 1U &&
          top_k.strict_below()[0].squared_distance == level(0) &&
          matches_ids(top_k.cutoff_shell_ids(), {0U, 2U}) &&
          matches_ids(top_k.canonical_choice_ids(), {0U, 1U}),
      "top-two family keeps the intruder strict and both antipodes tied");
  check(
      top_k.cutoff_squared_distance() ==
              result.facet_miniball.squared_radius &&
          result.local_boundary_equals_support &&
          result.global_shell_equals_local_boundary &&
          !result.facet_is_exact_top_k_member &&
          result.decision ==
              ExactFacetDescentPreconditionDecision::strict_descent_admissible,
      "facet is inactive despite cutoff equal to its miniball level");
  check(
      result.counters.global_closed_ball_query_count == 1U &&
          result.counters.global_closed_ball_distance_evaluation_count == 3U &&
          result.counters.exact_top_k_query_count == 1U &&
          result.counters.exact_top_k_distance_evaluation_count == 3U &&
          result.counters.total_exact_point_distance_evaluation_count == 6U &&
          result.scope == ExactFacetDescentPreconditionScope::
                              global_shell_and_top_k_preconditions_only &&
          std::string_view{ExactFacetDescentPreconditionResult::proof_basis} ==
              "exact_facet_miniball_global_closed_ball_exact_top_k_membership_v1" &&
          all_descent_precondition_certificates_close(verification),
      "strict precondition counters, scope and fresh replay close exactly");

  const std::array<PointId, 2> diagnostic_canonical_choice{0U, 1U};
  const ExactFacetMiniballResult diagnostic_choice_miniball =
      build_exact_facet_miniball(cloud, diagnostic_canonical_choice);
  check(
      diagnostic_choice_miniball.center == center(-1, 0, 0, 2) &&
          diagnostic_choice_miniball.squared_radius == level(1, 4) &&
          result.facet_miniball.squared_radius == level(1),
      "separate diagnostic canonical choice drops to one quarter without "
      "becoming a certified successor");
}

void test_strict_descent_preconditions_below_source_level() {
  const std::array<CertifiedPoint3, 4> input{
      point(-2.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 2> facet{3U, 0U};
  const ExactFacetDescentPreconditionResult result =
      build_exact_facet_descent_preconditions(cloud, facet);
  const auto verification =
      verify_exact_facet_descent_preconditions(cloud, facet, result);

  check(
      result.global_closed_ball.has_value() &&
          result.exact_top_k.has_value(),
      "strict-below-source result retains both exact partitions");
  if (!result.global_closed_ball.has_value() ||
      !result.exact_top_k.has_value()) {
    return;
  }
  const auto& global = *result.global_closed_ball;
  const auto& top_k = *result.exact_top_k;
  check(
      result.facet_miniball.center == center(0, 0, 0) &&
          result.facet_miniball.squared_radius == level(4) &&
          matches_ids(global.interior_ids(), {1U, 2U}) &&
          matches_ids(global.shell_ids(), {0U, 3U}) &&
          global.exterior_ids().empty(),
      "two strict intruders lie inside the exact source miniball");
  check(
      top_k.cutoff_squared_distance() == level(1) &&
          top_k.strict_below().empty() &&
          matches_ids(top_k.cutoff_shell_ids(), {1U, 2U}) &&
          matches_ids(top_k.canonical_choice_ids(), {1U, 2U}) &&
          top_k.cutoff_squared_distance() <
              result.facet_miniball.squared_radius,
      "top-two cutoff is strictly below the source miniball level");
  check(
      result.local_boundary_equals_support &&
          result.global_shell_equals_local_boundary &&
          !result.facet_is_exact_top_k_member &&
          result.decision ==
              ExactFacetDescentPreconditionDecision::strict_descent_admissible &&
          all_descent_precondition_certificates_close(verification),
      "strict-lower cutoff closes the regular descent preconditions");
}

void test_external_shell_is_unsupported() {
  const std::array<CertifiedPoint3, 4> input{
      point(1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 2> facet{3U, 0U};
  const ExactFacetDescentPreconditionResult result =
      build_exact_facet_descent_preconditions(cloud, facet);
  const auto verification =
      verify_exact_facet_descent_preconditions(cloud, facet, result);

  check(
      result.global_closed_ball.has_value() &&
          result.exact_top_k.has_value(),
      "external-shell result retains both exact partitions");
  if (!result.global_closed_ball.has_value() ||
      !result.exact_top_k.has_value()) {
    return;
  }
  const auto& global = *result.global_closed_ball;
  const auto& top_k = *result.exact_top_k;
  check(
      result.facet_miniball.support_point_ids ==
              std::vector<PointId>({0U, 3U}) &&
          result.facet_miniball.boundary_point_ids ==
              std::vector<PointId>({0U, 3U}) &&
          matches_ids(global.interior_ids(), {1U}) &&
          matches_ids(global.shell_ids(), {0U, 2U, 3U}) &&
          global.exterior_ids().empty(),
      "global shell exposes the point outside the facet exactly");
  check(
      top_k.cutoff_squared_distance() == level(1) &&
          top_k.strict_below().size() == 1U &&
          top_k.strict_below()[0].point_id == 1U &&
          matches_ids(top_k.cutoff_shell_ids(), {0U, 2U, 3U}) &&
          matches_ids(top_k.canonical_choice_ids(), {0U, 1U}),
      "external-shell top-two partition retains every tied point");
  check(
      result.local_boundary_equals_support &&
          !result.global_shell_equals_local_boundary &&
          !result.facet_is_exact_top_k_member &&
          result.decision ==
              ExactFacetDescentPreconditionDecision::unsupported_degeneracy &&
          all_descent_precondition_certificates_close(verification),
      "external shell fails closed before any successor is constructed");
}

void test_nonessential_internal_shell_exposes_hidden_plateau() {
  const std::array<CertifiedPoint3, 4> input{
      point(2.0, 0.0, 0.0),
      point(1.0, 1.0, 0.0),
      point(0.0, 2.0, 0.0),
      point(0.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 3> facet{3U, 0U, 1U};
  const ExactFacetDescentPreconditionResult result =
      build_exact_facet_descent_preconditions(cloud, facet);
  const auto verification =
      verify_exact_facet_descent_preconditions(cloud, facet, result);

  check(
      result.global_closed_ball.has_value() &&
          result.exact_top_k.has_value(),
      "nonessential-shell result retains both exact partitions");
  if (!result.global_closed_ball.has_value() ||
      !result.exact_top_k.has_value()) {
    return;
  }
  const auto& global = *result.global_closed_ball;
  const auto& top_k = *result.exact_top_k;
  check(
      result.facet_miniball.center == center(1, 1, 0) &&
          result.facet_miniball.squared_radius == level(2) &&
          result.facet_miniball.support_point_ids ==
              std::vector<PointId>({1U, 3U}) &&
          result.facet_miniball.boundary_point_ids ==
              std::vector<PointId>({0U, 1U, 3U}) &&
          result.facet_miniball.counters.optimal_support_count == 1U,
      "right triangle has one canonical support but a larger local shell");
  check(
      matches_ids(global.interior_ids(), {2U}) &&
          matches_ids(global.shell_ids(), {0U, 1U, 3U}) &&
          global.exterior_ids().empty() &&
          top_k.cutoff_squared_distance() == level(2) &&
          top_k.strict_below().size() == 1U &&
          top_k.strict_below()[0].point_id == 2U &&
          matches_ids(top_k.cutoff_shell_ids(), {0U, 1U, 3U}) &&
          matches_ids(top_k.canonical_choice_ids(), {0U, 1U, 2U}),
      "right-triangle intruder produces the complete tied top-three family");
  check(
      !result.local_boundary_equals_support &&
          result.global_shell_equals_local_boundary &&
          !result.facet_is_exact_top_k_member &&
          result.decision ==
              ExactFacetDescentPreconditionDecision::unsupported_degeneracy &&
          all_descent_precondition_certificates_close(verification),
      "optimal support count one is insufficient for descent essentiality");

  const std::array<PointId, 3> canonical_choice{0U, 1U, 2U};
  const std::array<PointId, 3> plateau_choice{1U, 2U, 3U};
  const ExactFacetMiniballResult canonical_miniball =
      build_exact_facet_miniball(cloud, canonical_choice);
  const ExactFacetMiniballResult plateau_miniball =
      build_exact_facet_miniball(cloud, plateau_choice);
  check(
      canonical_miniball.center == center(0, 1, 0) &&
          canonical_miniball.squared_radius == level(1) &&
          plateau_miniball.center == center(1, 1, 0) &&
          plateau_miniball.squared_radius == level(2),
      "canonical top-three choice drops while another exact choice preserves the plateau");
}

void test_facet_already_active_at_own_center() {
  const std::array<CertifiedPoint3, 3> input{
      point(3.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 2> facet{1U, 0U};
  const ExactFacetDescentPreconditionResult result =
      build_exact_facet_descent_preconditions(cloud, facet);
  const auto verification =
      verify_exact_facet_descent_preconditions(cloud, facet, result);

  check(
      result.global_closed_ball.has_value() &&
          result.exact_top_k.has_value(),
      "active-facet result retains both exact partitions");
  if (!result.global_closed_ball.has_value() ||
      !result.exact_top_k.has_value()) {
    return;
  }
  const auto& global = *result.global_closed_ball;
  const auto& top_k = *result.exact_top_k;
  check(
      global.interior_ids().empty() &&
          matches_ids(global.shell_ids(), {0U, 1U}) &&
          matches_ids(global.exterior_ids(), {2U}) &&
          top_k.strict_below().empty() &&
          matches_ids(top_k.cutoff_shell_ids(), {0U, 1U}) &&
          matches_ids(top_k.canonical_choice_ids(), {0U, 1U}),
      "active antipodal facet is the unique exact top-two choice");
  check(
      result.local_boundary_equals_support &&
          result.global_shell_equals_local_boundary &&
          result.facet_is_exact_top_k_member &&
          result.decision == ExactFacetDescentPreconditionDecision::
                                 already_active_at_own_center &&
          all_descent_precondition_certificates_close(verification),
      "already-active facet terminates without a successor claim");
}

void test_top_k_family_membership_is_not_canonical_choice_equality() {
  const std::array<CertifiedPoint3, 3> input{
      point(1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(-1.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 2> facet{2U, 0U};
  const ExactFacetDescentPreconditionResult result =
      build_exact_facet_descent_preconditions(cloud, facet);
  const auto verification =
      verify_exact_facet_descent_preconditions(cloud, facet, result);

  check(
      result.global_closed_ball.has_value() &&
          result.exact_top_k.has_value(),
      "membership discriminant retains both exact partitions");
  if (!result.global_closed_ball.has_value() ||
      !result.exact_top_k.has_value()) {
    return;
  }
  const auto& global = *result.global_closed_ball;
  const auto& top_k = *result.exact_top_k;
  check(
      global.interior_ids().empty() &&
          matches_ids(global.shell_ids(), {0U, 1U, 2U}) &&
          top_k.strict_below().empty() &&
          matches_ids(top_k.cutoff_shell_ids(), {0U, 1U, 2U}) &&
          matches_ids(top_k.canonical_choice_ids(), {0U, 1U}) &&
          !matches_ids(top_k.canonical_choice_ids(), {0U, 2U}),
      "canonical top-two representative differs from the antipodal facet");
  check(
      result.local_boundary_equals_support &&
          !result.global_shell_equals_local_boundary &&
          result.facet_is_exact_top_k_member &&
          result.decision ==
              ExactFacetDescentPreconditionDecision::unsupported_degeneracy &&
          all_descent_precondition_certificates_close(verification),
      "unsupported shell keeps the exact family-membership fact true");
}

void test_descent_precondition_verifier_rejects_every_result_layer() {
  const std::array<CertifiedPoint3, 3> input{
      point(-1.0, 0.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 2> facet{0U, 2U};
  const ExactFacetDescentPreconditionResult result =
      build_exact_facet_descent_preconditions(cloud, facet);

  ExactFacetDescentPreconditionResult bad_miniball = result;
  bad_miniball.facet_miniball.squared_radius = level(2);
  const auto miniball_check = verify_exact_facet_descent_preconditions(
      cloud, facet, bad_miniball);
  check(
      !miniball_check.facet_miniball_certified &&
          !miniball_check.fresh_replay_certified &&
          !miniball_check.exact_descent_preconditions_certified,
      "descent verifier rejects a falsified local miniball");

  ExactFacetDescentPreconditionResult missing_global = result;
  missing_global.global_closed_ball.reset();
  const auto missing_global_check = verify_exact_facet_descent_preconditions(
      cloud, facet, missing_global);
  check(
      !missing_global_check.global_closed_ball_identity_certified &&
          !missing_global_check.global_closed_ball_partition_certified &&
          !missing_global_check.exact_descent_preconditions_certified,
      "descent verifier rejects an absent global partition");

  ExactFacetDescentPreconditionResult missing_top_k = result;
  missing_top_k.exact_top_k.reset();
  const auto missing_top_k_check = verify_exact_facet_descent_preconditions(
      cloud, facet, missing_top_k);
  check(
      !missing_top_k_check.exact_top_k_identity_certified &&
          !missing_top_k_check.exact_top_k_partition_certified &&
          !missing_top_k_check.exact_descent_preconditions_certified,
      "descent verifier rejects an absent top-k partition");

  const std::array<PointId, 2> other_facet{0U, 1U};
  const ExactFacetDescentPreconditionResult other_result =
      build_exact_facet_descent_preconditions(cloud, other_facet);
  ExactFacetDescentPreconditionResult wrong_global_partition = result;
  wrong_global_partition.global_closed_ball = other_result.global_closed_ball;
  const auto wrong_global_partition_check =
      verify_exact_facet_descent_preconditions(
          cloud, facet, wrong_global_partition);
  check(
      wrong_global_partition_check.global_closed_ball_identity_certified &&
          !wrong_global_partition_check.global_closed_ball_partition_certified &&
          !wrong_global_partition_check.exact_descent_preconditions_certified,
      "descent verifier separates global identity from partition contents");

  ExactFacetDescentPreconditionResult wrong_top_k_partition = result;
  wrong_top_k_partition.exact_top_k = other_result.exact_top_k;
  const auto wrong_top_k_partition_check =
      verify_exact_facet_descent_preconditions(
          cloud, facet, wrong_top_k_partition);
  check(
      wrong_top_k_partition_check.exact_top_k_identity_certified &&
          !wrong_top_k_partition_check.exact_top_k_partition_certified &&
          !wrong_top_k_partition_check.exact_descent_preconditions_certified,
      "descent verifier separates top-k identity from partition contents");

  const CanonicalPointCloud twin_cloud = canonical_cloud(input);
  const ExactFacetDescentPreconditionResult twin_result =
      build_exact_facet_descent_preconditions(twin_cloud, facet);
  ExactFacetDescentPreconditionResult wrong_global_identity = result;
  wrong_global_identity.global_closed_ball = twin_result.global_closed_ball;
  const auto wrong_global_identity_check =
      verify_exact_facet_descent_preconditions(
          cloud, facet, wrong_global_identity);
  check(
      !wrong_global_identity_check.global_closed_ball_identity_certified &&
          !wrong_global_identity_check.exact_descent_preconditions_certified,
      "descent verifier rejects a closed ball from another cloud identity");

  ExactFacetDescentPreconditionResult wrong_top_k_identity = result;
  wrong_top_k_identity.exact_top_k = twin_result.exact_top_k;
  const auto wrong_top_k_identity_check =
      verify_exact_facet_descent_preconditions(
          cloud, facet, wrong_top_k_identity);
  check(
      !wrong_top_k_identity_check.exact_top_k_identity_certified &&
          !wrong_top_k_identity_check.exact_descent_preconditions_certified,
      "descent verifier rejects a top-k partition from another cloud identity");

  ExactFacetDescentPreconditionResult bad_local_boundary = result;
  bad_local_boundary.local_boundary_equals_support = false;
  const auto local_boundary_check = verify_exact_facet_descent_preconditions(
      cloud, facet, bad_local_boundary);
  check(
      !local_boundary_check.local_boundary_decision_certified &&
          !local_boundary_check.exact_descent_preconditions_certified,
      "descent verifier rejects a falsified local-boundary decision");

  ExactFacetDescentPreconditionResult bad_global_shell = result;
  bad_global_shell.global_shell_equals_local_boundary = false;
  const auto global_shell_check = verify_exact_facet_descent_preconditions(
      cloud, facet, bad_global_shell);
  check(
      !global_shell_check.global_shell_decision_certified &&
          !global_shell_check.exact_descent_preconditions_certified,
      "descent verifier rejects a falsified global-shell decision");

  ExactFacetDescentPreconditionResult bad_membership = result;
  bad_membership.facet_is_exact_top_k_member = true;
  const auto membership_check = verify_exact_facet_descent_preconditions(
      cloud, facet, bad_membership);
  check(
      !membership_check.facet_top_k_membership_decision_certified &&
          !membership_check.exact_descent_preconditions_certified,
      "descent verifier rejects a falsified exact-family membership decision");

  ExactFacetDescentPreconditionResult bad_counters = result;
  ++bad_counters.counters.total_exact_point_distance_evaluation_count;
  const auto counter_check = verify_exact_facet_descent_preconditions(
      cloud, facet, bad_counters);
  check(
      !counter_check.counters_certified &&
          !counter_check.exact_descent_preconditions_certified,
      "descent verifier rejects falsified aggregate counters");

  ExactFacetDescentPreconditionResult bad_decision = result;
  bad_decision.decision =
      ExactFacetDescentPreconditionDecision::already_active_at_own_center;
  const auto decision_check = verify_exact_facet_descent_preconditions(
      cloud, facet, bad_decision);
  check(
      !decision_check.decision_certified &&
          !decision_check.exact_descent_preconditions_certified,
      "descent verifier treats the precondition decision as untrusted");

  ExactFacetDescentPreconditionResult bad_scope = result;
  bad_scope.scope = ExactFacetDescentPreconditionScope::unspecified;
  const auto scope_check = verify_exact_facet_descent_preconditions(
      cloud, facet, bad_scope);
  check(
      !scope_check.scope_certified &&
          !scope_check.exact_descent_preconditions_certified,
      "descent verifier treats the precondition scope as untrusted");
}

void test_verifier_rejects_every_result_layer() {
  const std::array<CertifiedPoint3, 4> input{
      point(-1.0, -1.0),
      point(-1.0, 1.0),
      point(1.0, -1.0),
      point(1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 4> facet{0U, 1U, 2U, 3U};
  const ExactFacetMiniballResult result =
      build_exact_facet_miniball(cloud, facet);

  ExactFacetMiniballResult bad_facet = result;
  bad_facet.facet_point_ids.pop_back();
  const auto facet_check =
      verify_exact_facet_miniball(cloud, facet, bad_facet);
  check(
      !facet_check.facet_identity_certified &&
          !facet_check.fresh_replay_certified &&
          !facet_check.local_exact_facet_miniball_certified,
      "facet identity falsification fails closed");

  ExactFacetMiniballResult bad_support = result;
  bad_support.support_point_ids = {1U, 2U};
  const auto support_check =
      verify_exact_facet_miniball(cloud, facet, bad_support);
  check(
      !support_check.canonical_support_certified &&
          !support_check.local_exact_facet_miniball_certified,
      "canonical support falsification fails closed");

  ExactFacetMiniballResult bad_center = result;
  bad_center.center = center(1, 0, 0);
  const auto center_check =
      verify_exact_facet_miniball(cloud, facet, bad_center);
  check(
      !center_check.exact_center_and_radius_certified &&
          !center_check.local_exact_facet_miniball_certified,
      "exact center falsification fails closed");

  ExactFacetMiniballResult bad_radius = result;
  bad_radius.squared_radius = level(3);
  const auto radius_check =
      verify_exact_facet_miniball(cloud, facet, bad_radius);
  check(
      !radius_check.exact_center_and_radius_certified &&
          !radius_check.local_exact_facet_miniball_certified,
      "exact radius falsification fails closed");

  ExactFacetMiniballResult bad_partition = result;
  bad_partition.strictly_inside_point_ids.push_back(0U);
  const auto partition_check =
      verify_exact_facet_miniball(cloud, facet, bad_partition);
  check(
      !partition_check.enclosing_partition_certified &&
          !partition_check.local_exact_facet_miniball_certified,
      "inside-boundary partition falsification fails closed");

  ExactFacetMiniballResult bad_counters = result;
  ++bad_counters.counters.enumerated_support_count;
  const auto counter_check =
      verify_exact_facet_miniball(cloud, facet, bad_counters);
  check(
      !counter_check.exhaustive_support_enumeration_certified &&
          !counter_check.counters_certified &&
          !counter_check.local_exact_facet_miniball_certified,
      "enumeration counter falsification fails closed");

  ExactFacetMiniballResult bad_status = result;
  bad_status.status = ExactFacetMiniballStatus::not_certified;
  const auto status_check =
      verify_exact_facet_miniball(cloud, facet, bad_status);
  check(
      !status_check.status_certified &&
          !status_check.local_exact_facet_miniball_certified,
      "local result status is treated as untrusted");

  ExactFacetMiniballResult bad_scope = result;
  bad_scope.scope = ExactFacetMiniballScope::unspecified;
  const auto scope_check =
      verify_exact_facet_miniball(cloud, facet, bad_scope);
  check(
      !scope_check.local_scope_certified &&
          !scope_check.local_exact_facet_miniball_certified,
      "local result scope is treated as untrusted");
}

void test_invalid_facets_are_rejected() {
  const std::array<CertifiedPoint3, 11> input{
      point(0.0),
      point(1.0),
      point(2.0),
      point(3.0),
      point(4.0),
      point(5.0),
      point(6.0),
      point(7.0),
      point(8.0),
      point(9.0),
      point(10.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  check_throws<std::invalid_argument>(
      [&cloud]() {
        static_cast<void>(build_exact_facet_miniball(cloud, {}));
      },
      "empty facet is rejected");
  const std::array<PointId, 2> duplicate{0U, 0U};
  check_throws<std::invalid_argument>(
      [&cloud, &duplicate]() {
        static_cast<void>(build_exact_facet_miniball(cloud, duplicate));
      },
      "duplicate PointIds are rejected");
  const std::array<PointId, 11> too_large{
      0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U};
  check_throws<std::invalid_argument>(
      [&cloud, &too_large]() {
        static_cast<void>(build_exact_facet_miniball(cloud, too_large));
      },
      "facet larger than ten points is rejected before enumeration");
  const std::array<PointId, 1> outside{11U};
  check_throws<std::out_of_range>(
      [&cloud, &outside]() {
        static_cast<void>(build_exact_facet_miniball(cloud, outside));
      },
      "PointId outside the cloud is rejected");
}

}  // namespace

int main() {
  test_singleton_and_default_statuses();
  test_triangle_support_reductions();
  test_tetrahedron_supports();
  test_square_exposes_multiple_optimal_supports();
  test_smaller_exterior_circumsupport_is_rejected();
  test_ten_point_bound_is_exactly_385_supports();
  test_strict_descent_preconditions_with_equal_cutoff();
  test_strict_descent_preconditions_below_source_level();
  test_external_shell_is_unsupported();
  test_nonessential_internal_shell_exposes_hidden_plateau();
  test_facet_already_active_at_own_center();
  test_top_k_family_membership_is_not_canonical_choice_equality();
  test_descent_precondition_verifier_rejects_every_result_layer();
  test_verifier_rejects_every_result_layer();
  test_invalid_facets_are_rejected();

  if (failures != 0) {
    std::cerr << failures << " exact facet-miniball test(s) failed\n";
    return 1;
  }
  std::cout << "Exact facet-miniball tests passed\n";
  return 0;
}
