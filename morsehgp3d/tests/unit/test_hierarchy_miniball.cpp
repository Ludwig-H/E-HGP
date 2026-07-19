#include "morsehgp3d/hierarchy/critical_arm.hpp"
#include "morsehgp3d/hierarchy/miniball.hpp"
#include "morsehgp3d/spatial/brute_force.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <optional>
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
using morsehgp3d::exact::ExactRational;
using morsehgp3d::hierarchy::ExactFacetDescentArcDecision;
using morsehgp3d::hierarchy::ExactFacetDescentArcResult;
using morsehgp3d::hierarchy::ExactFacetDescentArcScope;
using morsehgp3d::hierarchy::ExactFacetDescentArcVerification;
using morsehgp3d::hierarchy::ExactFacetDescentChainBudget;
using morsehgp3d::hierarchy::ExactFacetDescentChainCounters;
using morsehgp3d::hierarchy::ExactFacetDescentChainDecision;
using morsehgp3d::hierarchy::ExactFacetDescentChainResult;
using morsehgp3d::hierarchy::ExactFacetDescentChainScope;
using morsehgp3d::hierarchy::ExactFacetDescentChainVerification;
using morsehgp3d::hierarchy::ExactFacetDescentSegmentDecision;
using morsehgp3d::hierarchy::ExactFacetDescentSegmentCounters;
using morsehgp3d::hierarchy::ExactFacetDescentSegmentResult;
using morsehgp3d::hierarchy::ExactFacetDescentSegmentScope;
using morsehgp3d::hierarchy::ExactFacetDescentSegmentVerification;
using morsehgp3d::hierarchy::ExactFacetDescentSegmentWitness;
using morsehgp3d::hierarchy::ExactFacetDescentPreconditionDecision;
using morsehgp3d::hierarchy::ExactFacetDescentPreconditionResult;
using morsehgp3d::hierarchy::ExactFacetDescentPreconditionScope;
using morsehgp3d::hierarchy::ExactFacetDescentPreconditionVerification;
using morsehgp3d::hierarchy::ExactFacetMiniballResult;
using morsehgp3d::hierarchy::ExactFacetMiniballScope;
using morsehgp3d::hierarchy::ExactFacetMiniballStatus;
using morsehgp3d::hierarchy::ExactFacetMiniballVerification;
using morsehgp3d::hierarchy::ExactCriticalArmDescentCounters;
using morsehgp3d::hierarchy::ExactCriticalArmDescentDecision;
using morsehgp3d::hierarchy::ExactCriticalArmDescentResult;
using morsehgp3d::hierarchy::ExactCriticalArmDescentScope;
using morsehgp3d::hierarchy::ExactCriticalArmDescentVerification;
using morsehgp3d::hierarchy::ExactCriticalArmInitialSegmentCounters;
using morsehgp3d::hierarchy::ExactCriticalArmInitialSegmentDecision;
using morsehgp3d::hierarchy::ExactCriticalArmInitialSegmentResult;
using morsehgp3d::hierarchy::ExactCriticalArmInitialSegmentScope;
using morsehgp3d::hierarchy::ExactCriticalArmInitialSegmentVerification;
using morsehgp3d::hierarchy::ExactCriticalArmSourceDecision;
using morsehgp3d::hierarchy::build_exact_facet_descent_arc;
using morsehgp3d::hierarchy::build_exact_facet_descent_chain;
using morsehgp3d::hierarchy::build_exact_facet_descent_preconditions;
using morsehgp3d::hierarchy::build_exact_facet_descent_segment;
using morsehgp3d::hierarchy::build_exact_facet_miniball;
using morsehgp3d::hierarchy::build_exact_critical_arm_descent;
using morsehgp3d::hierarchy::build_exact_critical_arm_initial_segment;
using morsehgp3d::hierarchy::verify_exact_facet_descent_arc;
using morsehgp3d::hierarchy::verify_exact_facet_descent_chain;
using morsehgp3d::hierarchy::verify_exact_facet_descent_preconditions;
using morsehgp3d::hierarchy::verify_exact_facet_descent_segment;
using morsehgp3d::hierarchy::verify_exact_facet_miniball;
using morsehgp3d::hierarchy::verify_exact_critical_arm_descent;
using morsehgp3d::hierarchy::verify_exact_critical_arm_initial_segment;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExclusionSet;
using morsehgp3d::spatial::PointId;
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

[[nodiscard]] ExactRational rational(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return ExactRational{BigInt{numerator}, BigInt{denominator}};
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

[[nodiscard]] bool all_descent_arc_certificates_close(
    const ExactFacetDescentArcVerification& verification) {
  return verification.source_preconditions_certified &&
         verification.successor_payload_presence_certified &&
         verification.successor_facet_certified &&
         verification.successor_miniball_certified &&
         verification.successor_is_canonical_top_k_choice_certified &&
         verification.successor_is_exact_top_k_member_certified &&
         verification.successor_differs_from_source_certified &&
         verification.successor_level_within_top_k_cutoff_certified &&
         verification.strict_level_decrease_certified &&
         verification.counters_certified &&
         verification.decision_certified && verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.exact_descent_arc_decision_certified;
}

[[nodiscard]] bool all_descent_segment_certificates_close(
    const ExactFacetDescentSegmentVerification& verification) {
  return verification.source_arc_certified &&
         verification.segment_witness_presence_certified &&
         verification.source_atom_level_certified &&
         verification.successor_atom_level_certified &&
         verification.center_squared_displacement_certified &&
         verification.centers_equal_certified &&
         verification.source_endpoint_strict_sublevel_certified &&
         verification.quadratic_max_upper_bound_certified &&
         verification.closed_segment_nonstrict_sublevel_certified &&
         verification.half_open_segment_strict_sublevel_certified &&
         verification.exact_level_relations_certified &&
         verification.counters_certified &&
         verification.decision_certified && verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.exact_descent_segment_decision_certified;
}

[[nodiscard]] bool all_descent_chain_certificates_close(
    const ExactFacetDescentChainVerification& verification) {
  return verification.requested_budget_certified &&
         verification.effective_budget_certified &&
         verification.compact_path_shape_certified &&
         verification.initial_facet_identity_certified &&
         verification.compact_nodes_certified &&
         verification.committed_segment_witnesses_certified &&
         verification.stopping_probe_presence_certified &&
         verification.stopping_probe_certified &&
         verification.exact_seams_certified &&
         verification.strict_facet_potential_certified &&
         verification.finite_strict_facet_orbit_theorem_certified &&
         verification.closed_polyline_nonstrict_initial_sublevel_certified &&
         verification.source_open_polyline_strict_initial_sublevel_certified &&
         verification.counters_certified &&
         verification.decision_certified && verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.exact_descent_chain_decision_certified;
}

[[nodiscard]] bool all_critical_arm_initial_certificates_close(
    const ExactCriticalArmInitialSegmentVerification& verification) {
  return verification.input_shell_identity_certified &&
         verification.removed_shell_point_identity_certified &&
         verification.critical_shell_miniball_certified &&
         verification.global_closed_ball_presence_certified &&
         verification.global_closed_ball_certified &&
         verification.source_facts_certified &&
         verification.source_decision_certified &&
         verification.arm_payload_presence_certified &&
         verification.arm_facet_certified &&
         verification.arm_miniball_certified &&
         verification.analytic_segment_coefficients_certified &&
         verification.removed_point_witnesses_certified &&
         verification.exterior_constraint_witnesses_certified &&
         verification.strict_local_parameter_bound_certified &&
         verification.strict_arm_consequences_certified &&
         verification.counters_certified &&
         verification.decision_certified &&
         verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.exact_critical_arm_initial_segment_decision_certified;
}

[[nodiscard]] bool all_critical_arm_descent_certificates_close(
    const ExactCriticalArmDescentVerification& verification) {
  return verification.requested_chain_budget_certified &&
         verification.initial_segment_certified &&
         verification.facet_chain_presence_certified &&
         verification.facet_chain_certified &&
         verification.initial_segment_budget_separation_certified &&
         verification.exact_initial_to_chain_seam_certified &&
         verification.source_open_composite_path_certified &&
         verification.counters_certified &&
         verification.decision_mapping_certified &&
         verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.exact_critical_arm_descent_decision_certified;
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

void test_strict_descent_arc_with_equal_source_cutoff() {
  const ExactFacetDescentArcResult empty;
  check(
      empty.decision == ExactFacetDescentArcDecision::not_certified &&
          empty.scope == ExactFacetDescentArcScope::unspecified &&
          !empty.successor_facet_point_ids.has_value() &&
          !empty.successor_miniball.has_value(),
      "default descent arc has neither a successor nor a certified decision");

  const std::array<CertifiedPoint3, 3> input{
      point(1.0, 0.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 2> facet{2U, 0U};
  const ExactFacetDescentArcResult result =
      build_exact_facet_descent_arc(cloud, facet);
  const ExactFacetDescentArcVerification verification =
      verify_exact_facet_descent_arc(cloud, facet, result);

  check(
      result.source_preconditions.decision ==
              ExactFacetDescentPreconditionDecision::
                  strict_descent_admissible &&
          result.source_preconditions.exact_top_k.has_value() &&
          result.successor_facet_point_ids.has_value() &&
          result.successor_miniball.has_value(),
      "strict equal-cutoff branch emits exactly one chosen successor payload");
  if (!result.source_preconditions.exact_top_k.has_value() ||
      !result.successor_facet_point_ids.has_value() ||
      !result.successor_miniball.has_value()) {
    return;
  }
  const auto& source = result.source_preconditions.facet_miniball;
  const auto& source_top_k = *result.source_preconditions.exact_top_k;
  const auto& successor_ids = *result.successor_facet_point_ids;
  const auto& successor = *result.successor_miniball;
  check(
      source.facet_point_ids == std::vector<PointId>({0U, 2U}) &&
          source.center == center(0, 0, 0) &&
          source.squared_radius == level(1) &&
          source_top_k.cutoff_squared_distance() == level(1) &&
          source_top_k.strict_below().size() == 1U &&
          source_top_k.strict_below()[0].point_id == 1U &&
          source_top_k.strict_below()[0].squared_distance == level(0),
      "equal-cutoff source has one strict intruder at its exact center");
  check(
      matches_ids(source_top_k.cutoff_shell_ids(), {0U, 2U}) &&
          matches_ids(source_top_k.canonical_choice_ids(), {0U, 1U}) &&
          successor_ids == std::vector<PointId>({0U, 1U}),
      "chosen successor is the explicit canonical member of the top-two family");
  check(
      successor.facet_point_ids == std::vector<PointId>({0U, 1U}) &&
          successor.support_point_ids == std::vector<PointId>({0U, 1U}) &&
          successor.boundary_point_ids == std::vector<PointId>({0U, 1U}) &&
          successor.strictly_inside_point_ids.empty() &&
          successor.center == center(-1, 0, 0, 2) &&
          successor.squared_radius == level(1, 4) &&
          successor.counters.optimal_support_count == 1U,
      "chosen equal-cutoff successor has exact midpoint and quarter level");
  check(
      successor.squared_radius < source_top_k.cutoff_squared_distance() &&
          source_top_k.cutoff_squared_distance() == source.squared_radius &&
          result.successor_is_canonical_top_k_choice &&
          result.successor_is_exact_top_k_member &&
          result.successor_differs_from_source &&
          result.successor_level_within_top_k_cutoff &&
          result.strict_level_decrease,
      "fresh minimization supplies strictness when the source cutoff is equal");
  check(
      result.counters.precondition_classification_count == 1U &&
          result.counters.canonical_top_k_selection_count == 1U &&
          result.counters.successor_miniball_build_count == 1U &&
          result.counters.exact_level_comparison_count == 2U &&
          result.decision ==
              ExactFacetDescentArcDecision::strict_descent_arc_certified &&
          result.scope == ExactFacetDescentArcScope::
                              canonical_top_k_selected_strict_level_arc_only &&
          std::string_view{ExactFacetDescentArcResult::proof_basis} ==
              "exact_descent_preconditions_canonical_top_k_member_fresh_miniball_strict_level_v1" &&
          all_descent_arc_certificates_close(verification),
      "equal-cutoff arc counters, decision, scope and replay close exactly");

  const ExactFacetDescentPreconditionResult successor_preconditions =
      build_exact_facet_descent_preconditions(
          cloud, std::span<const PointId>{successor_ids});
  check(
      successor_preconditions.decision == ExactFacetDescentPreconditionDecision::
                                              already_active_at_own_center &&
          successor_preconditions.facet_is_exact_top_k_member &&
          successor_preconditions.global_closed_ball.has_value() &&
          matches_ids(
              successor_preconditions.global_closed_ball->shell_ids(),
              {0U, 1U}),
      "chosen successor independently reclassifies as an active terminal facet");
}

void test_strict_descent_arc_with_lower_source_cutoff() {
  const std::array<CertifiedPoint3, 4> input{
      point(2.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(-2.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 2> facet{3U, 0U};
  const ExactFacetDescentArcResult result =
      build_exact_facet_descent_arc(cloud, facet);
  const auto verification =
      verify_exact_facet_descent_arc(cloud, facet, result);

  check(
      result.source_preconditions.exact_top_k.has_value() &&
          result.successor_facet_point_ids.has_value() &&
          result.successor_miniball.has_value(),
      "strict lower-cutoff branch emits its chosen successor payload");
  if (!result.source_preconditions.exact_top_k.has_value() ||
      !result.successor_facet_point_ids.has_value() ||
      !result.successor_miniball.has_value()) {
    return;
  }
  const auto& source = result.source_preconditions.facet_miniball;
  const auto& source_top_k = *result.source_preconditions.exact_top_k;
  const auto& successor_ids = *result.successor_facet_point_ids;
  const auto& successor = *result.successor_miniball;
  check(
      source.facet_point_ids == std::vector<PointId>({0U, 3U}) &&
          source.center == center(0, 0, 0) &&
          source.squared_radius == level(4) &&
          source_top_k.strict_below().empty() &&
          source_top_k.cutoff_squared_distance() == level(1) &&
          matches_ids(source_top_k.cutoff_shell_ids(), {1U, 2U}) &&
          matches_ids(source_top_k.canonical_choice_ids(), {1U, 2U}),
      "lower-cutoff source separates the inner antipodal pair exactly");
  check(
      successor_ids == std::vector<PointId>({1U, 2U}) &&
          successor.facet_point_ids == std::vector<PointId>({1U, 2U}) &&
          successor.support_point_ids == std::vector<PointId>({1U, 2U}) &&
          successor.center == center(0, 0, 0) &&
          successor.squared_radius == level(1),
      "lower-cutoff successor has the same center and a strictly lower level");
  check(
      successor.center == source.center &&
          successor.squared_radius ==
              source_top_k.cutoff_squared_distance() &&
          successor.squared_radius < source.squared_radius &&
          result.successor_is_canonical_top_k_choice &&
          result.successor_is_exact_top_k_member &&
          result.successor_differs_from_source &&
          result.successor_level_within_top_k_cutoff &&
          result.strict_level_decrease &&
          result.counters.precondition_classification_count == 1U &&
          result.counters.canonical_top_k_selection_count == 1U &&
          result.counters.successor_miniball_build_count == 1U &&
          result.counters.exact_level_comparison_count == 2U &&
          result.decision ==
              ExactFacetDescentArcDecision::strict_descent_arc_certified &&
          all_descent_arc_certificates_close(verification),
      "cutoff strictness certifies a level arc without requiring center motion");

  const ExactFacetDescentPreconditionResult successor_preconditions =
      build_exact_facet_descent_preconditions(
          cloud, std::span<const PointId>{successor_ids});
  check(
      successor_preconditions.decision == ExactFacetDescentPreconditionDecision::
                                              already_active_at_own_center &&
          successor_preconditions.facet_is_exact_top_k_member,
      "same-center successor independently closes as an active terminal facet");
}

void test_descent_arc_omits_successor_for_active_facet() {
  const std::array<CertifiedPoint3, 3> input{
      point(3.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 2> facet{1U, 0U};
  const ExactFacetDescentArcResult result =
      build_exact_facet_descent_arc(cloud, facet);
  const auto verification =
      verify_exact_facet_descent_arc(cloud, facet, result);

  check(
      result.source_preconditions.decision ==
              ExactFacetDescentPreconditionDecision::
                  already_active_at_own_center &&
          result.source_preconditions.facet_is_exact_top_k_member &&
          !result.successor_facet_point_ids.has_value() &&
          !result.successor_miniball.has_value(),
      "already-active source emits no self-loop successor payload");
  check(
      !result.successor_is_canonical_top_k_choice &&
          !result.successor_is_exact_top_k_member &&
          !result.successor_differs_from_source &&
          !result.successor_level_within_top_k_cutoff &&
          !result.strict_level_decrease &&
          result.counters.precondition_classification_count == 1U &&
          result.counters.canonical_top_k_selection_count == 0U &&
          result.counters.successor_miniball_build_count == 0U &&
          result.counters.exact_level_comparison_count == 0U &&
          result.decision == ExactFacetDescentArcDecision::
                                 no_arc_already_active_at_own_center &&
          result.scope == ExactFacetDescentArcScope::
                              canonical_top_k_selected_strict_level_arc_only &&
          all_descent_arc_certificates_close(verification),
      "active no-arc branch closes with zero successor work");
}

void test_descent_arc_omits_successor_for_nonessential_facet() {
  const std::array<CertifiedPoint3, 4> input{
      point(2.0, 0.0, 0.0),
      point(1.0, 1.0, 0.0),
      point(0.0, 2.0, 0.0),
      point(0.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 3> facet{3U, 0U, 1U};
  const ExactFacetDescentArcResult result =
      build_exact_facet_descent_arc(cloud, facet);
  const auto verification =
      verify_exact_facet_descent_arc(cloud, facet, result);

  check(
      result.source_preconditions.decision ==
              ExactFacetDescentPreconditionDecision::
                  unsupported_degeneracy &&
          !result.source_preconditions.local_boundary_equals_support &&
          !result.successor_facet_point_ids.has_value() &&
          !result.successor_miniball.has_value(),
      "nonessential source emits no chosen successor payload");
  check(
      !result.successor_is_canonical_top_k_choice &&
          !result.successor_is_exact_top_k_member &&
          !result.successor_differs_from_source &&
          !result.successor_level_within_top_k_cutoff &&
          !result.strict_level_decrease &&
          result.counters.precondition_classification_count == 1U &&
          result.counters.canonical_top_k_selection_count == 0U &&
          result.counters.successor_miniball_build_count == 0U &&
          result.counters.exact_level_comparison_count == 0U &&
          result.decision == ExactFacetDescentArcDecision::
                                 no_arc_unsupported_degeneracy &&
          all_descent_arc_certificates_close(verification),
      "unsupported no-arc branch closes with zero successor work");

  const std::array<PointId, 3> diagnostic_canonical_choice{0U, 1U, 2U};
  const std::array<PointId, 3> diagnostic_plateau_choice{1U, 2U, 3U};
  const ExactFacetMiniballResult canonical_miniball =
      build_exact_facet_miniball(cloud, diagnostic_canonical_choice);
  const ExactFacetMiniballResult plateau_miniball =
      build_exact_facet_miniball(cloud, diagnostic_plateau_choice);
  check(
      canonical_miniball.center == center(0, 1, 0) &&
          canonical_miniball.squared_radius == level(1) &&
          plateau_miniball.center == center(1, 1, 0) &&
          plateau_miniball.squared_radius == level(2),
      "unsupported branch suppresses a descending canonical choice beside a plateau choice");
}

void test_descent_arc_verifier_rejects_every_new_result_layer() {
  const std::array<CertifiedPoint3, 3> input{
      point(-1.0, 0.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 2> facet{0U, 2U};
  const ExactFacetDescentArcResult result =
      build_exact_facet_descent_arc(cloud, facet);

  ExactFacetDescentArcResult bad_preconditions = result;
  bad_preconditions.source_preconditions.decision =
      ExactFacetDescentPreconditionDecision::already_active_at_own_center;
  const auto precondition_check =
      verify_exact_facet_descent_arc(cloud, facet, bad_preconditions);
  check(
      !precondition_check.source_preconditions_certified &&
          !precondition_check.exact_descent_arc_decision_certified,
      "arc verifier rejects falsified embedded source preconditions");

  ExactFacetDescentArcResult missing_successor_facet = result;
  missing_successor_facet.successor_facet_point_ids.reset();
  const auto missing_successor_facet_check =
      verify_exact_facet_descent_arc(cloud, facet, missing_successor_facet);
  check(
      !missing_successor_facet_check.successor_payload_presence_certified &&
          !missing_successor_facet_check.exact_descent_arc_decision_certified,
      "strict arc verifier rejects an absent successor facet");

  ExactFacetDescentArcResult missing_successor_miniball = result;
  missing_successor_miniball.successor_miniball.reset();
  const auto missing_successor_miniball_check =
      verify_exact_facet_descent_arc(cloud, facet, missing_successor_miniball);
  check(
      !missing_successor_miniball_check.successor_payload_presence_certified &&
          !missing_successor_miniball_check.exact_descent_arc_decision_certified,
      "strict arc verifier rejects an absent successor miniball");

  const std::array<PointId, 2> alternate_member{1U, 2U};
  ExactFacetDescentArcResult noncanonical_member = result;
  noncanonical_member.successor_facet_point_ids =
      std::vector<PointId>{1U, 2U};
  noncanonical_member.successor_miniball =
      build_exact_facet_miniball(cloud, alternate_member);
  const auto noncanonical_member_check =
      verify_exact_facet_descent_arc(cloud, facet, noncanonical_member);
  check(
      !noncanonical_member_check.successor_facet_certified &&
          !noncanonical_member_check.exact_descent_arc_decision_certified,
      "arc verifier rejects another mathematically valid but noncanonical top-k member");

  ExactFacetDescentArcResult mismatched_miniball = result;
  mismatched_miniball.successor_miniball =
      build_exact_facet_miniball(cloud, alternate_member);
  const auto mismatched_miniball_check =
      verify_exact_facet_descent_arc(cloud, facet, mismatched_miniball);
  check(
      !mismatched_miniball_check.successor_miniball_certified &&
          !mismatched_miniball_check.exact_descent_arc_decision_certified,
      "arc verifier rejects a successor facet-miniball identity mismatch");

  ExactFacetDescentArcResult bad_canonical_fact = result;
  bad_canonical_fact.successor_is_canonical_top_k_choice = false;
  const auto canonical_fact_check =
      verify_exact_facet_descent_arc(cloud, facet, bad_canonical_fact);
  check(
      !canonical_fact_check.successor_is_canonical_top_k_choice_certified &&
          !canonical_fact_check.exact_descent_arc_decision_certified,
      "arc verifier rejects a falsified canonical-choice fact");

  ExactFacetDescentArcResult bad_member_fact = result;
  bad_member_fact.successor_is_exact_top_k_member = false;
  const auto member_fact_check =
      verify_exact_facet_descent_arc(cloud, facet, bad_member_fact);
  check(
      !member_fact_check.successor_is_exact_top_k_member_certified &&
          !member_fact_check.exact_descent_arc_decision_certified,
      "arc verifier rejects a falsified top-k-membership fact");

  ExactFacetDescentArcResult bad_difference_fact = result;
  bad_difference_fact.successor_differs_from_source = false;
  const auto difference_fact_check =
      verify_exact_facet_descent_arc(cloud, facet, bad_difference_fact);
  check(
      !difference_fact_check.successor_differs_from_source_certified &&
          !difference_fact_check.exact_descent_arc_decision_certified,
      "arc verifier rejects a falsified successor-difference fact");

  ExactFacetDescentArcResult bad_cutoff_fact = result;
  bad_cutoff_fact.successor_level_within_top_k_cutoff = false;
  const auto cutoff_fact_check =
      verify_exact_facet_descent_arc(cloud, facet, bad_cutoff_fact);
  check(
      !cutoff_fact_check.successor_level_within_top_k_cutoff_certified &&
          !cutoff_fact_check.exact_descent_arc_decision_certified,
      "arc verifier rejects a falsified successor-cutoff fact");

  ExactFacetDescentArcResult bad_decrease_fact = result;
  bad_decrease_fact.strict_level_decrease = false;
  const auto decrease_fact_check =
      verify_exact_facet_descent_arc(cloud, facet, bad_decrease_fact);
  check(
      !decrease_fact_check.strict_level_decrease_certified &&
          !decrease_fact_check.exact_descent_arc_decision_certified,
      "arc verifier rejects a falsified strict-level fact");

  ExactFacetDescentArcResult bad_counters = result;
  ++bad_counters.counters.exact_level_comparison_count;
  const auto counter_check =
      verify_exact_facet_descent_arc(cloud, facet, bad_counters);
  check(
      !counter_check.counters_certified &&
          !counter_check.exact_descent_arc_decision_certified,
      "arc verifier rejects falsified strict-arc counters");

  ExactFacetDescentArcResult bad_decision = result;
  bad_decision.decision =
      ExactFacetDescentArcDecision::no_arc_already_active_at_own_center;
  const auto decision_check =
      verify_exact_facet_descent_arc(cloud, facet, bad_decision);
  check(
      !decision_check.decision_certified &&
          !decision_check.exact_descent_arc_decision_certified,
      "arc verifier treats the arc decision as untrusted");

  ExactFacetDescentArcResult bad_scope = result;
  bad_scope.scope = ExactFacetDescentArcScope::unspecified;
  const auto scope_check =
      verify_exact_facet_descent_arc(cloud, facet, bad_scope);
  check(
      !scope_check.scope_certified &&
          !scope_check.exact_descent_arc_decision_certified,
      "arc verifier treats the strict-level-only scope as untrusted");

  const CanonicalPointCloud twin_cloud = canonical_cloud(input);
  const ExactFacetDescentArcResult twin_result =
      build_exact_facet_descent_arc(twin_cloud, facet);
  const auto cloud_identity_check =
      verify_exact_facet_descent_arc(cloud, facet, twin_result);
  check(
      !cloud_identity_check.source_preconditions_certified &&
          !cloud_identity_check.exact_descent_arc_decision_certified,
      "arc verifier rejects embedded partitions from another cloud identity");

  const std::array<CertifiedPoint3, 3> active_input{
      point(-1.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(3.0, 0.0, 0.0)};
  const CanonicalPointCloud active_cloud = canonical_cloud(active_input);
  const std::array<PointId, 2> active_facet{0U, 1U};
  ExactFacetDescentArcResult injected_active =
      build_exact_facet_descent_arc(active_cloud, active_facet);
  injected_active.successor_facet_point_ids =
      std::vector<PointId>{0U, 1U};
  injected_active.successor_miniball =
      build_exact_facet_miniball(active_cloud, active_facet);
  const auto injected_active_check = verify_exact_facet_descent_arc(
      active_cloud, active_facet, injected_active);
  check(
      !injected_active_check.successor_payload_presence_certified &&
          !injected_active_check.exact_descent_arc_decision_certified,
      "arc verifier rejects an injected self-loop on an already-active facet");

  const std::array<CertifiedPoint3, 4> unsupported_input{
      point(0.0, 0.0, 0.0),
      point(0.0, 2.0, 0.0),
      point(1.0, 1.0, 0.0),
      point(2.0, 0.0, 0.0)};
  const CanonicalPointCloud unsupported_cloud =
      canonical_cloud(unsupported_input);
  const std::array<PointId, 3> unsupported_facet{0U, 1U, 3U};
  const std::array<PointId, 3> unsupported_canonical_choice{0U, 1U, 2U};
  ExactFacetDescentArcResult injected_unsupported =
      build_exact_facet_descent_arc(unsupported_cloud, unsupported_facet);
  injected_unsupported.successor_facet_point_ids =
      std::vector<PointId>{0U, 1U, 2U};
  injected_unsupported.successor_miniball = build_exact_facet_miniball(
      unsupported_cloud, unsupported_canonical_choice);
  const auto injected_unsupported_check = verify_exact_facet_descent_arc(
      unsupported_cloud, unsupported_facet, injected_unsupported);
  check(
      !injected_unsupported_check.successor_payload_presence_certified &&
          !injected_unsupported_check.exact_descent_arc_decision_certified,
      "arc verifier rejects a descending payload on an unsupported source");
}

void test_descent_segment_equal_cutoff_is_half_open_strict() {
  const ExactFacetDescentSegmentResult empty;
  check(
      empty.decision == ExactFacetDescentSegmentDecision::not_certified &&
          empty.scope == ExactFacetDescentSegmentScope::unspecified &&
          !empty.segment_witness.has_value(),
      "default descent segment has no witness or certified decision");

  const std::array<CertifiedPoint3, 3> input{
      point(1.0, 0.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 2> facet{2U, 0U};
  const ExactFacetDescentSegmentResult result =
      build_exact_facet_descent_segment(cloud, facet);
  const ExactFacetDescentSegmentVerification verification =
      verify_exact_facet_descent_segment(cloud, facet, result);

  check(
      result.source_arc.decision ==
              ExactFacetDescentArcDecision::strict_descent_arc_certified &&
          result.source_arc.successor_facet_point_ids.has_value() &&
          result.source_arc.successor_miniball.has_value() &&
          result.segment_witness.has_value(),
      "equal-cutoff strict arc emits one analytic segment witness");
  if (!result.source_arc.successor_facet_point_ids.has_value() ||
      !result.source_arc.successor_miniball.has_value() ||
      !result.segment_witness.has_value()) {
    return;
  }
  const auto& source =
      result.source_arc.source_preconditions.facet_miniball;
  const auto& successor_ids =
      *result.source_arc.successor_facet_point_ids;
  const auto& successor = *result.source_arc.successor_miniball;
  const auto& witness = *result.segment_witness;
  check(
      source.facet_point_ids == std::vector<PointId>({0U, 2U}) &&
          source.center == center(0, 0, 0) &&
          source.squared_radius == level(1) &&
          successor_ids == std::vector<PointId>({0U, 1U}) &&
          successor.center == center(-1, 0, 0, 2) &&
          successor.squared_radius == level(1, 4),
      "equal-cutoff segment retains its exact source and target geometry");
  check(
      witness.source_atom_level == level(1) &&
          witness.successor_atom_level == level(1, 4) &&
          witness.center_squared_displacement == level(1, 4) &&
          !witness.centers_equal &&
          !witness.source_endpoint_strict_sublevel &&
          witness.quadratic_max_upper_bound_certified &&
          witness.closed_segment_nonstrict_sublevel &&
          witness.half_open_segment_strict_sublevel,
      "equal-cutoff witness distinguishes its non-strict source from the strict half-open segment");
  check(
      witness.source_atom_level == source.squared_radius &&
          witness.successor_atom_level < witness.source_atom_level &&
          witness.center_squared_displacement == level(1, 4) &&
          result.counters.source_arc_classification_count == 1U &&
          result.counters.source_atom_distance_evaluation_count == 2U &&
          result.counters.source_atom_maximum_comparison_count == 1U &&
          result.counters.center_displacement_evaluation_count == 1U &&
          result.counters.exact_level_relation_count == 4U &&
          result.counters.convex_identity_certification_count == 1U,
      "equal-cutoff segment closes its exact coefficients and bounded work");
  check(
      result.decision == ExactFacetDescentSegmentDecision::
                             strict_half_open_segment_certified &&
          result.scope == ExactFacetDescentSegmentScope::
                              canonical_strict_arc_half_open_sublevel_segment_only &&
          std::string_view{ExactFacetDescentSegmentResult::proof_basis} ==
              "exact_squared_distance_chord_identity_max_envelope_half_open_segment_v1" &&
          all_descent_segment_certificates_close(verification),
      "equal-cutoff segment decision, scope and fresh replay close exactly");

  const ExclusionSet empty_exclusions = ExclusionSet::from_ids(
      std::span<const PointId>{}, cloud, 0U);
  const ExactRational half{BigInt{1}, BigInt{2}};
  const ExactRational one{BigInt{1}};
  const ExactLevel midpoint_quadratic_upper{
      (one - half) * witness.source_atom_level.rational() +
      half * witness.successor_atom_level.rational() -
      half * (one - half) *
          witness.center_squared_displacement.rational()};
  const auto midpoint_top_k = brute_force_top_k(
      cloud, center(-1, 0, 0, 4), 2U, empty_exclusions);
  check(
      midpoint_quadratic_upper == level(9, 16) &&
          midpoint_top_k.cutoff_squared_distance() ==
              midpoint_quadratic_upper &&
          midpoint_top_k.strict_below().size() == 1U &&
          midpoint_top_k.strict_below()[0].point_id == 1U &&
          midpoint_top_k.strict_below()[0].squared_distance == level(1, 16) &&
          matches_ids(midpoint_top_k.cutoff_shell_ids(), {0U}) &&
          matches_ids(midpoint_top_k.canonical_choice_ids(), {0U, 1U}) &&
          midpoint_top_k.cutoff_squared_distance() < source.squared_radius,
      "independent midpoint diagnostic observes q one-half equal to nine sixteenths");
}

void test_descent_segment_accepts_equal_centers_below_source_level() {
  const std::array<CertifiedPoint3, 4> input{
      point(2.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(-2.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 2> facet{3U, 0U};
  const ExactFacetDescentSegmentResult result =
      build_exact_facet_descent_segment(cloud, facet);
  const auto verification =
      verify_exact_facet_descent_segment(cloud, facet, result);

  check(
      result.source_arc.successor_miniball.has_value() &&
          result.segment_witness.has_value(),
      "same-center strict arc emits one analytic segment witness");
  if (!result.source_arc.successor_miniball.has_value() ||
      !result.segment_witness.has_value()) {
    return;
  }
  const auto& source =
      result.source_arc.source_preconditions.facet_miniball;
  const auto& successor = *result.source_arc.successor_miniball;
  const auto& witness = *result.segment_witness;
  check(
      source.center == center(0, 0, 0) &&
          source.squared_radius == level(4) &&
          successor.center == source.center &&
          successor.squared_radius == level(1) &&
          witness.source_atom_level == level(1) &&
          witness.successor_atom_level == level(1) &&
          witness.center_squared_displacement == level(0),
      "same-center witness has a equal to b equal to one and delta zero");
  check(
      witness.centers_equal && witness.source_endpoint_strict_sublevel &&
          witness.quadratic_max_upper_bound_certified &&
          witness.closed_segment_nonstrict_sublevel &&
          witness.half_open_segment_strict_sublevel &&
          witness.source_atom_level < source.squared_radius &&
          result.counters.source_arc_classification_count == 1U &&
          result.counters.source_atom_distance_evaluation_count == 2U &&
          result.counters.source_atom_maximum_comparison_count == 1U &&
          result.counters.center_displacement_evaluation_count == 1U &&
          result.counters.exact_level_relation_count == 4U &&
          result.counters.convex_identity_certification_count == 1U &&
          result.decision == ExactFacetDescentSegmentDecision::
                                 strict_half_open_segment_certified &&
          all_descent_segment_certificates_close(verification),
      "zero-displacement segment remains strictly below the source level");
}

void test_descent_segment_omits_witness_for_active_facet() {
  const std::array<CertifiedPoint3, 3> input{
      point(3.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 2> facet{1U, 0U};
  const ExactFacetDescentSegmentResult result =
      build_exact_facet_descent_segment(cloud, facet);
  const auto verification =
      verify_exact_facet_descent_segment(cloud, facet, result);

  check(
      result.source_arc.decision == ExactFacetDescentArcDecision::
                                        no_arc_already_active_at_own_center &&
          !result.segment_witness.has_value() &&
          result.counters.source_arc_classification_count == 1U &&
          result.counters.source_atom_distance_evaluation_count == 0U &&
          result.counters.source_atom_maximum_comparison_count == 0U &&
          result.counters.center_displacement_evaluation_count == 0U &&
          result.counters.exact_level_relation_count == 0U &&
          result.counters.convex_identity_certification_count == 0U,
      "active source emits no segment witness or geometric work");
  check(
      result.decision == ExactFacetDescentSegmentDecision::
                             no_segment_already_active_at_own_center &&
          result.scope == ExactFacetDescentSegmentScope::
                              canonical_strict_arc_half_open_sublevel_segment_only &&
          all_descent_segment_certificates_close(verification),
      "active no-segment decision closes its exact replay");
}

void test_descent_segment_omits_witness_for_unsupported_facet() {
  const std::array<CertifiedPoint3, 4> input{
      point(2.0, 0.0, 0.0),
      point(1.0, 1.0, 0.0),
      point(0.0, 2.0, 0.0),
      point(0.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 3> facet{3U, 0U, 1U};
  const ExactFacetDescentSegmentResult result =
      build_exact_facet_descent_segment(cloud, facet);
  const auto verification =
      verify_exact_facet_descent_segment(cloud, facet, result);

  check(
      result.source_arc.decision ==
              ExactFacetDescentArcDecision::
                  no_arc_unsupported_degeneracy &&
          !result.segment_witness.has_value() &&
          result.counters.source_arc_classification_count == 1U &&
          result.counters.source_atom_distance_evaluation_count == 0U &&
          result.counters.source_atom_maximum_comparison_count == 0U &&
          result.counters.center_displacement_evaluation_count == 0U &&
          result.counters.exact_level_relation_count == 0U &&
          result.counters.convex_identity_certification_count == 0U,
      "unsupported source emits no segment witness or geometric work");
  check(
      result.decision == ExactFacetDescentSegmentDecision::
                             no_segment_unsupported_degeneracy &&
          all_descent_segment_certificates_close(verification),
      "unsupported no-segment decision closes its exact replay");
}

void test_descent_segment_verifier_rejects_every_new_result_layer() {
  const std::array<CertifiedPoint3, 3> input{
      point(-1.0, 0.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 2> facet{0U, 2U};
  const ExactFacetDescentSegmentResult result =
      build_exact_facet_descent_segment(cloud, facet);
  check(
      result.segment_witness.has_value(),
      "segment falsification fixture starts from a strict witness");
  if (!result.segment_witness.has_value()) {
    return;
  }

  ExactFacetDescentSegmentResult bad_arc = result;
  bad_arc.source_arc.decision =
      ExactFacetDescentArcDecision::no_arc_already_active_at_own_center;
  const auto arc_check =
      verify_exact_facet_descent_segment(cloud, facet, bad_arc);
  check(
      !arc_check.source_arc_certified &&
          !arc_check.exact_descent_segment_decision_certified,
      "segment verifier rejects a falsified embedded arc");

  ExactFacetDescentSegmentResult missing_witness = result;
  missing_witness.segment_witness.reset();
  const auto presence_check =
      verify_exact_facet_descent_segment(cloud, facet, missing_witness);
  check(
      !presence_check.segment_witness_presence_certified &&
          !presence_check.exact_descent_segment_decision_certified,
      "strict segment verifier rejects an absent witness");

  ExactFacetDescentSegmentResult bad_source_atom = result;
  bad_source_atom.segment_witness->source_atom_level = level(1, 4);
  const auto source_atom_check =
      verify_exact_facet_descent_segment(cloud, facet, bad_source_atom);
  check(
      !source_atom_check.source_atom_level_certified &&
          !source_atom_check.exact_descent_segment_decision_certified,
      "segment verifier rejects a falsified source atom level");

  ExactFacetDescentSegmentResult bad_successor_atom = result;
  bad_successor_atom.segment_witness->successor_atom_level = level(1);
  const auto successor_atom_check =
      verify_exact_facet_descent_segment(cloud, facet, bad_successor_atom);
  check(
      !successor_atom_check.successor_atom_level_certified &&
          !successor_atom_check.exact_descent_segment_decision_certified,
      "segment verifier rejects a falsified successor atom level");

  ExactFacetDescentSegmentResult bad_displacement = result;
  bad_displacement.segment_witness->center_squared_displacement = level(0);
  const auto displacement_check =
      verify_exact_facet_descent_segment(cloud, facet, bad_displacement);
  check(
      !displacement_check.center_squared_displacement_certified &&
          !displacement_check.exact_descent_segment_decision_certified,
      "segment verifier rejects a falsified squared center displacement");

  ExactFacetDescentSegmentResult bad_centers_equal = result;
  bad_centers_equal.segment_witness->centers_equal = true;
  const auto centers_equal_check =
      verify_exact_facet_descent_segment(cloud, facet, bad_centers_equal);
  check(
      !centers_equal_check.centers_equal_certified &&
          !centers_equal_check.exact_descent_segment_decision_certified,
      "segment verifier rejects a falsified center-equality fact");

  ExactFacetDescentSegmentResult bad_source_strict = result;
  bad_source_strict.segment_witness->source_endpoint_strict_sublevel = true;
  const auto source_strict_check =
      verify_exact_facet_descent_segment(cloud, facet, bad_source_strict);
  check(
      !source_strict_check.source_endpoint_strict_sublevel_certified &&
          !source_strict_check.exact_descent_segment_decision_certified,
      "segment verifier rejects a falsified source-endpoint fact");

  ExactFacetDescentSegmentResult bad_quadratic_bound = result;
  bad_quadratic_bound.segment_witness->
      quadratic_max_upper_bound_certified = false;
  const auto quadratic_bound_check =
      verify_exact_facet_descent_segment(cloud, facet, bad_quadratic_bound);
  check(
      !quadratic_bound_check.quadratic_max_upper_bound_certified &&
          !quadratic_bound_check.exact_descent_segment_decision_certified,
      "segment verifier rejects a falsified quadratic-envelope fact");

  ExactFacetDescentSegmentResult bad_closed_segment = result;
  bad_closed_segment.segment_witness->
      closed_segment_nonstrict_sublevel = false;
  const auto closed_segment_check =
      verify_exact_facet_descent_segment(cloud, facet, bad_closed_segment);
  check(
      !closed_segment_check.closed_segment_nonstrict_sublevel_certified &&
          !closed_segment_check.exact_descent_segment_decision_certified,
      "segment verifier rejects a falsified closed-segment fact");

  ExactFacetDescentSegmentResult bad_half_open_segment = result;
  bad_half_open_segment.segment_witness->
      half_open_segment_strict_sublevel = false;
  const auto half_open_segment_check =
      verify_exact_facet_descent_segment(cloud, facet, bad_half_open_segment);
  check(
      !half_open_segment_check.half_open_segment_strict_sublevel_certified &&
          !half_open_segment_check.exact_descent_segment_decision_certified,
      "segment verifier rejects a falsified half-open-segment fact");

  constexpr std::array<
      std::size_t ExactFacetDescentSegmentCounters::*, 6>
      counter_fields{
          &ExactFacetDescentSegmentCounters::
              source_arc_classification_count,
          &ExactFacetDescentSegmentCounters::
              source_atom_distance_evaluation_count,
          &ExactFacetDescentSegmentCounters::
              source_atom_maximum_comparison_count,
          &ExactFacetDescentSegmentCounters::
              center_displacement_evaluation_count,
          &ExactFacetDescentSegmentCounters::exact_level_relation_count,
          &ExactFacetDescentSegmentCounters::
              convex_identity_certification_count};
  for (const auto counter_field : counter_fields) {
    ExactFacetDescentSegmentResult bad_counter = result;
    ++(bad_counter.counters.*counter_field);
    const auto counter_check =
        verify_exact_facet_descent_segment(cloud, facet, bad_counter);
    check(
        !counter_check.counters_certified &&
            !counter_check.exact_descent_segment_decision_certified,
        "segment verifier rejects every falsified proof counter");
  }

  ExactFacetDescentSegmentResult bad_decision = result;
  bad_decision.decision = ExactFacetDescentSegmentDecision::
                              no_segment_already_active_at_own_center;
  const auto decision_check =
      verify_exact_facet_descent_segment(cloud, facet, bad_decision);
  check(
      !decision_check.decision_certified &&
          !decision_check.exact_descent_segment_decision_certified,
      "segment verifier treats the segment decision as untrusted");

  ExactFacetDescentSegmentResult bad_scope = result;
  bad_scope.scope = ExactFacetDescentSegmentScope::unspecified;
  const auto scope_check =
      verify_exact_facet_descent_segment(cloud, facet, bad_scope);
  check(
      !scope_check.scope_certified &&
          !scope_check.exact_descent_segment_decision_certified,
      "segment verifier treats the half-open scope as untrusted");

  const CanonicalPointCloud twin_cloud = canonical_cloud(input);
  const ExactFacetDescentSegmentResult twin_result =
      build_exact_facet_descent_segment(twin_cloud, facet);
  const auto identity_check =
      verify_exact_facet_descent_segment(cloud, facet, twin_result);
  check(
      !identity_check.source_arc_certified &&
          !identity_check.exact_descent_segment_decision_certified,
      "segment verifier rejects an arc from another cloud identity");

  const std::array<CertifiedPoint3, 3> active_input{
      point(-1.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(3.0, 0.0, 0.0)};
  const CanonicalPointCloud active_cloud = canonical_cloud(active_input);
  const std::array<PointId, 2> active_facet{0U, 1U};
  ExactFacetDescentSegmentResult injected_active =
      build_exact_facet_descent_segment(active_cloud, active_facet);
  ExactFacetDescentSegmentWitness active_witness;
  active_witness.source_atom_level = level(1);
  active_witness.successor_atom_level = level(1);
  active_witness.center_squared_displacement = level(0);
  active_witness.centers_equal = true;
  active_witness.closed_segment_nonstrict_sublevel = true;
  injected_active.segment_witness.emplace(std::move(active_witness));
  const auto injected_active_check = verify_exact_facet_descent_segment(
      active_cloud, active_facet, injected_active);
  check(
      !injected_active_check.segment_witness_presence_certified &&
          !injected_active_check.exact_descent_segment_decision_certified,
      "segment verifier rejects an injected witness on an active source");

  const std::array<CertifiedPoint3, 4> unsupported_input{
      point(0.0, 0.0, 0.0),
      point(0.0, 2.0, 0.0),
      point(1.0, 1.0, 0.0),
      point(2.0, 0.0, 0.0)};
  const CanonicalPointCloud unsupported_cloud =
      canonical_cloud(unsupported_input);
  const std::array<PointId, 3> unsupported_facet{0U, 1U, 3U};
  ExactFacetDescentSegmentResult injected_unsupported =
      build_exact_facet_descent_segment(
          unsupported_cloud, unsupported_facet);
  ExactFacetDescentSegmentWitness unsupported_witness;
  unsupported_witness.source_atom_level = level(2);
  unsupported_witness.successor_atom_level = level(1);
  unsupported_witness.center_squared_displacement = level(1);
  unsupported_witness.source_endpoint_strict_sublevel = false;
  unsupported_witness.quadratic_max_upper_bound_certified = true;
  unsupported_witness.closed_segment_nonstrict_sublevel = true;
  unsupported_witness.half_open_segment_strict_sublevel = true;
  injected_unsupported.segment_witness.emplace(
      std::move(unsupported_witness));
  const auto injected_unsupported_check =
      verify_exact_facet_descent_segment(
          unsupported_cloud, unsupported_facet, injected_unsupported);
  check(
      !injected_unsupported_check.segment_witness_presence_certified &&
          !injected_unsupported_check.exact_descent_segment_decision_certified,
      "segment verifier rejects a valid-looking witness on an unsupported source");
}

[[nodiscard]] CanonicalPointCloud descent_chain_fixture_cloud() {
  const std::array<CertifiedPoint3, 6> input{
      point(-5.0, -6.0),
      point(-5.0, 5.0),
      point(-3.0, 3.0),
      point(3.0, 1.0),
      point(3.0, 6.0),
      point(4.0, 5.0)};
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud
descent_chain_nontrivial_atom_seam_fixture_cloud() {
  const std::array<CertifiedPoint3, 5> input{
      point(-3.0, -6.0),
      point(-2.0, 6.0),
      point(3.0, 8.0),
      point(4.0, 8.0),
      point(5.0, 6.0)};
  return canonical_cloud(input);
}

void test_descent_chain_concatenates_exact_segments() {
  const ExactFacetDescentChainResult empty;
  check(
      empty.decision == ExactFacetDescentChainDecision::not_certified &&
          empty.scope == ExactFacetDescentChainScope::unspecified &&
          empty.nodes.empty() &&
          empty.committed_segment_witnesses.empty() &&
          !empty.stopping_probe.has_value() &&
          !empty.exact_seams_certified &&
          !empty.strict_facet_potential_certified &&
          !empty.finite_strict_facet_orbit_theorem_certified &&
          !empty.closed_polyline_nonstrict_initial_sublevel &&
          !empty.source_open_polyline_strict_initial_sublevel,
      "default descent chain certifies no orbit or polyline");

  const CanonicalPointCloud cloud = descent_chain_fixture_cloud();
  const std::array<PointId, 4> facet{4U, 0U, 2U, 1U};
  const ExactFacetDescentChainBudget budget{20U};
  const ExactFacetDescentChainResult result =
      build_exact_facet_descent_chain(cloud, facet, budget);
  const ExactFacetDescentChainVerification verification =
      verify_exact_facet_descent_chain(cloud, facet, budget, result);

  check(
      result.requested_budget == budget &&
          result.effective_maximum_committed_strict_segment_count == 14U &&
          result.nodes.size() == 3U &&
          result.committed_segment_witnesses.size() == 2U &&
          result.stopping_probe.has_value(),
      "two-step chain closes below the exact fifteen-facet orbit bound");
  if (result.nodes.size() != 3U ||
      result.committed_segment_witnesses.size() != 2U ||
      !result.stopping_probe.has_value()) {
    return;
  }

  check(
      result.nodes[0].facet_point_ids ==
              std::vector<PointId>({0U, 1U, 2U, 4U}) &&
          result.nodes[0].center == center(-1, 0, 0) &&
          result.nodes[0].squared_level == level(52) &&
          result.nodes[1].facet_point_ids ==
              std::vector<PointId>({1U, 2U, 3U, 5U}) &&
          result.nodes[1].center == center(-1, 8, 0, 2) &&
          result.nodes[1].squared_level == level(85, 4) &&
          result.nodes[2].facet_point_ids ==
              std::vector<PointId>({1U, 2U, 3U, 4U}) &&
          result.nodes[2].center == center(-3, 14, 0, 4) &&
          result.nodes[2].squared_level == level(325, 16),
      "two-step chain stores the exact canonical facets, centers and levels");

  const ExactFacetDescentSegmentWitness& first =
      result.committed_segment_witnesses[0];
  const ExactFacetDescentSegmentWitness& second =
      result.committed_segment_witnesses[1];
  check(
      first.source_atom_level == level(50) &&
          first.successor_atom_level == level(85, 4) &&
          first.center_squared_displacement == level(65, 4) &&
          !first.centers_equal && first.source_endpoint_strict_sublevel &&
          first.quadratic_max_upper_bound_certified &&
          first.closed_segment_nonstrict_sublevel &&
          first.half_open_segment_strict_sublevel &&
          second.source_atom_level == level(85, 4) &&
          second.successor_atom_level == level(325, 16) &&
          second.center_squared_displacement == level(5, 16) &&
          !second.centers_equal &&
          !second.source_endpoint_strict_sublevel &&
          second.quadratic_max_upper_bound_certified &&
          second.closed_segment_nonstrict_sublevel &&
          second.half_open_segment_strict_sublevel,
      "aligned segment witnesses preserve both seam-side atom levels");
  check(
      result.nodes[1].squared_level == first.successor_atom_level &&
          result.nodes[1].squared_level == second.source_atom_level &&
          result.nodes[2].squared_level == second.successor_atom_level &&
          result.nodes[2].squared_level < result.nodes[1].squared_level &&
          result.nodes[1].squared_level < result.nodes[0].squared_level,
      "fresh seams agree while the exact facet potential strictly decreases");

  const ExactFacetDescentChainCounters& counters = result.counters;
  check(
      counters.facet_probe_count == 3U &&
          counters.strict_segment_probe_count == 2U &&
          counters.committed_strict_segment_count == 2U &&
          counters.visited_facet_count == 3U &&
          counters.successor_cycle_lookup_count == 2U &&
          counters.inter_step_seam_replay_count == 2U &&
          counters.active_terminal_count == 1U &&
          counters.unsupported_terminal_count == 0U &&
          counters.structural_budget_stop_count == 0U &&
          counters.accumulated_probe_counters.
                  source_arc_classification_count == 3U &&
          counters.accumulated_probe_counters.
                  source_atom_distance_evaluation_count == 8U &&
          counters.accumulated_probe_counters.
                  source_atom_maximum_comparison_count == 6U &&
          counters.accumulated_probe_counters.
                  center_displacement_evaluation_count == 2U &&
          counters.accumulated_probe_counters.
                  exact_level_relation_count == 8U &&
          counters.accumulated_probe_counters.
                  convex_identity_certification_count == 2U,
      "two-step chain closes all probe, seam, cycle and analytic counters");
  check(
      result.stopping_probe->decision ==
              ExactFacetDescentSegmentDecision::
                  no_segment_already_active_at_own_center &&
          !result.stopping_probe->segment_witness.has_value() &&
          result.exact_seams_certified &&
          result.strict_facet_potential_certified &&
          result.finite_strict_facet_orbit_theorem_certified &&
          result.closed_polyline_nonstrict_initial_sublevel &&
          result.source_open_polyline_strict_initial_sublevel &&
          result.decision == ExactFacetDescentChainDecision::
                                 complete_at_regular_active_facet &&
          result.scope == ExactFacetDescentChainScope::
                              single_source_canonical_strict_descent_chain_only &&
          std::string_view{ExactFacetDescentChainResult::proof_basis} ==
              "exact_replayed_half_open_segments_exact_seams_strict_facet_"
              "potential_finite_orbit_v1" &&
          all_descent_chain_certificates_close(verification),
      "complete chain publishes only its single-source exact polyline scope");
}

void test_descent_chain_keeps_miniball_and_atom_seams_distinct() {
  const CanonicalPointCloud cloud =
      descent_chain_nontrivial_atom_seam_fixture_cloud();
  const std::array<PointId, 2> facet{0U, 2U};
  const ExactFacetDescentChainBudget budget{2U};
  const ExactFacetDescentChainResult result =
      build_exact_facet_descent_chain(cloud, facet, budget);
  const ExactFacetDescentChainVerification verification =
      verify_exact_facet_descent_chain(cloud, facet, budget, result);

  check(
      result.nodes.size() == 3U &&
          result.committed_segment_witnesses.size() == 2U &&
          result.stopping_probe.has_value() &&
          result.decision == ExactFacetDescentChainDecision::
                                 complete_at_regular_active_facet &&
          all_descent_chain_certificates_close(verification),
      "nontrivial atom seam fixture closes its two-step active orbit");
  if (result.nodes.size() != 3U ||
      result.committed_segment_witnesses.size() != 2U) {
    return;
  }

  const ExactFacetDescentSegmentWitness& first =
      result.committed_segment_witnesses[0];
  const ExactFacetDescentSegmentWitness& second =
      result.committed_segment_witnesses[1];
  check(
      result.nodes[0].facet_point_ids ==
              std::vector<PointId>({0U, 2U}) &&
          result.nodes[0].center == center(0, 1, 0) &&
          result.nodes[0].squared_level == level(58) &&
          result.nodes[1].facet_point_ids ==
              std::vector<PointId>({1U, 4U}) &&
          result.nodes[1].center == center(3, 12, 0, 2) &&
          result.nodes[1].squared_level == level(49, 4) &&
          result.nodes[2].facet_point_ids ==
              std::vector<PointId>({2U, 3U}) &&
          result.nodes[2].center == center(7, 16, 0, 2) &&
          result.nodes[2].squared_level == level(1, 4),
      "nontrivial atom seam stores all exact facets, centers and miniballs");
  check(
      first.source_atom_level == level(50) &&
          first.successor_atom_level == level(49, 4) &&
          first.center_squared_displacement == level(109, 4) &&
          first.source_endpoint_strict_sublevel &&
          second.source_atom_level == level(41, 4) &&
          second.successor_atom_level == level(1, 4) &&
          second.center_squared_displacement == level(8) &&
          second.source_endpoint_strict_sublevel,
      "nontrivial atom seam stores the independently recomputed segment levels");
  check(
      result.nodes[1].squared_level == first.successor_atom_level &&
          second.source_atom_level < result.nodes[1].squared_level &&
          second.source_atom_level != first.successor_atom_level &&
          result.nodes[2].squared_level == second.successor_atom_level,
      "chain continuity uses the exact miniball seam without equating the next atom level");
}

void test_descent_chain_budgets_and_terminal_decisions() {
  const CanonicalPointCloud cloud = descent_chain_fixture_cloud();
  const std::array<PointId, 4> facet{0U, 1U, 2U, 4U};

  const std::array<CertifiedPoint3, 1> singleton_input{
      point(2.0, -3.0, 4.0)};
  const CanonicalPointCloud singleton_cloud =
      canonical_cloud(singleton_input);
  const std::array<PointId, 1> singleton_facet{0U};
  const ExactFacetDescentChainBudget singleton_budget{4U};
  const ExactFacetDescentChainResult singleton =
      build_exact_facet_descent_chain(
          singleton_cloud, singleton_facet, singleton_budget);
  const auto singleton_verification = verify_exact_facet_descent_chain(
      singleton_cloud,
      singleton_facet,
      singleton_budget,
      singleton);
  check(
      singleton.effective_maximum_committed_strict_segment_count == 0U &&
          singleton.nodes.size() == 1U &&
          singleton.committed_segment_witnesses.empty() &&
          singleton.stopping_probe.has_value() &&
          singleton.counters.facet_probe_count == 1U &&
          singleton.counters.active_terminal_count == 1U &&
          singleton.decision == ExactFacetDescentChainDecision::
                                    complete_at_regular_active_facet &&
          all_descent_chain_certificates_close(singleton_verification),
      "n equals k equals one closes the zero structural orbit bound");

  const ExactFacetDescentChainBudget zero_budget{0U};
  const ExactFacetDescentChainResult zero =
      build_exact_facet_descent_chain(cloud, facet, zero_budget);
  const auto zero_verification =
      verify_exact_facet_descent_chain(
          cloud, facet, zero_budget, zero);
  check(
      zero.effective_maximum_committed_strict_segment_count == 0U &&
          zero.nodes.size() == 1U &&
          zero.committed_segment_witnesses.empty() &&
          zero.stopping_probe.has_value() &&
          zero.stopping_probe->decision ==
              ExactFacetDescentSegmentDecision::
                  strict_half_open_segment_certified &&
          zero.counters.facet_probe_count == 1U &&
          zero.counters.strict_segment_probe_count == 1U &&
          zero.counters.committed_strict_segment_count == 0U &&
          zero.counters.visited_facet_count == 1U &&
          zero.counters.successor_cycle_lookup_count == 1U &&
          zero.counters.inter_step_seam_replay_count == 0U &&
          zero.counters.structural_budget_stop_count == 1U &&
          zero.decision == ExactFacetDescentChainDecision::
                               certified_prefix_strict_segment_budget_exhausted &&
          all_descent_chain_certificates_close(zero_verification),
      "zero budget retains one certified source and the uncommitted strict frontier");

  const ExactFacetDescentChainBudget one_budget{1U};
  const ExactFacetDescentChainResult one =
      build_exact_facet_descent_chain(cloud, facet, one_budget);
  const auto one_verification =
      verify_exact_facet_descent_chain(
          cloud, facet, one_budget, one);
  check(
      one.effective_maximum_committed_strict_segment_count == 1U &&
          one.nodes.size() == 2U &&
          one.committed_segment_witnesses.size() == 1U &&
          one.stopping_probe.has_value() &&
          one.stopping_probe->decision ==
              ExactFacetDescentSegmentDecision::
                  strict_half_open_segment_certified &&
          one.counters.facet_probe_count == 2U &&
          one.counters.strict_segment_probe_count == 2U &&
          one.counters.committed_strict_segment_count == 1U &&
          one.counters.visited_facet_count == 2U &&
          one.counters.successor_cycle_lookup_count == 2U &&
          one.counters.inter_step_seam_replay_count == 1U &&
          one.counters.structural_budget_stop_count == 1U &&
          one.decision == ExactFacetDescentChainDecision::
                              certified_prefix_strict_segment_budget_exhausted &&
          all_descent_chain_certificates_close(one_verification),
      "one-segment budget commits exactly one seam before the strict frontier");

  const ExactFacetDescentChainBudget exact_budget{2U};
  const ExactFacetDescentChainResult exact =
      build_exact_facet_descent_chain(cloud, facet, exact_budget);
  const auto exact_verification =
      verify_exact_facet_descent_chain(
          cloud, facet, exact_budget, exact);
  check(
      exact.committed_segment_witnesses.size() == 2U &&
          exact.counters.structural_budget_stop_count == 0U &&
          exact.decision == ExactFacetDescentChainDecision::
                                complete_at_regular_active_facet &&
          all_descent_chain_certificates_close(exact_verification),
      "terminal probe succeeds when the exact two-segment chain meets its budget");

  check_throws<std::invalid_argument>(
      [&cloud, &facet]() {
        const ExactFacetDescentChainBudget excessive{
            ExactFacetDescentChainBudget::
                maximum_supported_committed_strict_segment_count + 1U};
        static_cast<void>(
            build_exact_facet_descent_chain(cloud, facet, excessive));
      },
      "reference descent chain rejects a budget above its explicit cap");

  const std::array<CertifiedPoint3, 3> active_input{
      point(-1.0, 0.0), point(1.0, 0.0), point(3.0, 0.0)};
  const CanonicalPointCloud active_cloud = canonical_cloud(active_input);
  const std::array<PointId, 2> active_facet{0U, 1U};
  const ExactFacetDescentChainBudget terminal_budget{4U};
  const ExactFacetDescentChainResult active =
      build_exact_facet_descent_chain(
          active_cloud, active_facet, terminal_budget);
  const auto active_verification = verify_exact_facet_descent_chain(
      active_cloud, active_facet, terminal_budget, active);
  check(
      active.effective_maximum_committed_strict_segment_count == 2U &&
          active.nodes.size() == 1U &&
          active.committed_segment_witnesses.empty() &&
          active.counters.facet_probe_count == 1U &&
          active.counters.active_terminal_count == 1U &&
          active.counters.unsupported_terminal_count == 0U &&
          active.counters.structural_budget_stop_count == 0U &&
          active.decision == ExactFacetDescentChainDecision::
                                 complete_at_regular_active_facet &&
          all_descent_chain_certificates_close(active_verification),
      "already-active source closes as a zero-segment complete chain");

  const std::array<CertifiedPoint3, 4> unsupported_input{
      point(0.0, 0.0),
      point(0.0, 2.0),
      point(1.0, 1.0),
      point(2.0, 0.0)};
  const CanonicalPointCloud unsupported_cloud =
      canonical_cloud(unsupported_input);
  const std::array<PointId, 3> unsupported_facet{0U, 1U, 3U};
  const ExactFacetDescentChainResult unsupported =
      build_exact_facet_descent_chain(
          unsupported_cloud, unsupported_facet, terminal_budget);
  const auto unsupported_verification =
      verify_exact_facet_descent_chain(
          unsupported_cloud,
          unsupported_facet,
          terminal_budget,
          unsupported);
  check(
      unsupported.effective_maximum_committed_strict_segment_count == 3U &&
          unsupported.nodes.size() == 1U &&
          unsupported.committed_segment_witnesses.empty() &&
          unsupported.counters.facet_probe_count == 1U &&
          unsupported.counters.active_terminal_count == 0U &&
          unsupported.counters.unsupported_terminal_count == 1U &&
          unsupported.counters.structural_budget_stop_count == 0U &&
          unsupported.decision == ExactFacetDescentChainDecision::
                                      certified_prefix_blocked_unsupported_degeneracy &&
          all_descent_chain_certificates_close(
              unsupported_verification),
      "unsupported source returns only its zero-segment certified prefix");
}

void test_descent_chain_verifier_rejects_every_result_layer() {
  const CanonicalPointCloud cloud = descent_chain_fixture_cloud();
  const std::array<PointId, 4> facet{0U, 1U, 2U, 4U};
  const ExactFacetDescentChainBudget budget{20U};
  const ExactFacetDescentChainResult result =
      build_exact_facet_descent_chain(cloud, facet, budget);
  check(
      result.nodes.size() == 3U &&
          result.committed_segment_witnesses.size() == 2U &&
          result.stopping_probe.has_value(),
      "chain falsification fixture starts from two segments and one terminal probe");
  if (result.nodes.size() != 3U ||
      result.committed_segment_witnesses.size() != 2U ||
      !result.stopping_probe.has_value()) {
    return;
  }

  const auto rejects =
      [&cloud, &facet, budget](
          const ExactFacetDescentChainResult& candidate,
          const std::string& message) {
        const auto verification = verify_exact_facet_descent_chain(
            cloud, facet, budget, candidate);
        check(
            !verification.exact_descent_chain_decision_certified,
            message);
      };

  ExactFacetDescentChainResult bad_requested_budget = result;
  bad_requested_budget.requested_budget.
      maximum_committed_strict_segment_count = 19U;
  rejects(
      bad_requested_budget,
      "chain verifier rejects a falsified requested budget");

  ExactFacetDescentChainResult bad_effective_budget = result;
  ++bad_effective_budget.
      effective_maximum_committed_strict_segment_count;
  rejects(
      bad_effective_budget,
      "chain verifier rejects a falsified effective budget");

  ExactFacetDescentChainResult bad_shape = result;
  bad_shape.nodes.pop_back();
  rejects(bad_shape, "chain verifier rejects an unaligned compact path");

  ExactFacetDescentChainResult bad_initial_node = result;
  bad_initial_node.nodes.front().facet_point_ids.front() = 5U;
  rejects(
      bad_initial_node,
      "chain verifier rejects a falsified initial facet identity");

  ExactFacetDescentChainResult bad_node_geometry = result;
  bad_node_geometry.nodes[1].squared_level = level(21);
  rejects(
      bad_node_geometry,
      "chain verifier rejects a falsified compact seam node");

  ExactFacetDescentChainResult repeated_facet = result;
  repeated_facet.nodes.back().facet_point_ids =
      repeated_facet.nodes.front().facet_point_ids;
  rejects(
      repeated_facet,
      "chain verifier rejects an explicitly repeated facet");

  ExactFacetDescentChainResult bad_witness = result;
  bad_witness.committed_segment_witnesses[0].source_atom_level = level(49);
  rejects(
      bad_witness,
      "chain verifier rejects a falsified committed segment witness");

  ExactFacetDescentChainResult missing_stopping_probe = result;
  missing_stopping_probe.stopping_probe.reset();
  rejects(
      missing_stopping_probe,
      "chain verifier rejects an absent complete stopping probe");

  ExactFacetDescentChainResult bad_stopping_decision = result;
  bad_stopping_decision.stopping_probe->decision =
      ExactFacetDescentSegmentDecision::no_segment_unsupported_degeneracy;
  rejects(
      bad_stopping_decision,
      "chain verifier rejects a falsified stopping decision");

  ExactFacetDescentChainResult bad_stopping_scope = result;
  bad_stopping_scope.stopping_probe->scope =
      ExactFacetDescentSegmentScope::unspecified;
  rejects(
      bad_stopping_scope,
      "chain verifier rejects a falsified stopping probe scope");

  ExactFacetDescentChainResult bad_stopping_geometry = result;
  bad_stopping_geometry.stopping_probe->source_arc.source_preconditions.
      facet_miniball.center = center(0, 0, 0);
  rejects(
      bad_stopping_geometry,
      "chain verifier rejects falsified geometry inside the stopping probe");

  constexpr std::array<
      bool ExactFacetDescentChainResult::*, 5>
      proof_fields{
          &ExactFacetDescentChainResult::exact_seams_certified,
          &ExactFacetDescentChainResult::strict_facet_potential_certified,
          &ExactFacetDescentChainResult::
              finite_strict_facet_orbit_theorem_certified,
          &ExactFacetDescentChainResult::
              closed_polyline_nonstrict_initial_sublevel,
          &ExactFacetDescentChainResult::
              source_open_polyline_strict_initial_sublevel};
  for (const auto proof_field : proof_fields) {
    ExactFacetDescentChainResult bad_proof = result;
    bad_proof.*proof_field = false;
    rejects(bad_proof, "chain verifier rejects every falsified proof fact");
  }

  constexpr std::array<
      std::size_t ExactFacetDescentChainCounters::*, 9>
      chain_counter_fields{
          &ExactFacetDescentChainCounters::facet_probe_count,
          &ExactFacetDescentChainCounters::strict_segment_probe_count,
          &ExactFacetDescentChainCounters::committed_strict_segment_count,
          &ExactFacetDescentChainCounters::visited_facet_count,
          &ExactFacetDescentChainCounters::successor_cycle_lookup_count,
          &ExactFacetDescentChainCounters::inter_step_seam_replay_count,
          &ExactFacetDescentChainCounters::active_terminal_count,
          &ExactFacetDescentChainCounters::unsupported_terminal_count,
          &ExactFacetDescentChainCounters::structural_budget_stop_count};
  for (const auto counter_field : chain_counter_fields) {
    ExactFacetDescentChainResult bad_counter = result;
    ++(bad_counter.counters.*counter_field);
    rejects(
        bad_counter,
        "chain verifier rejects every falsified orbit counter");
  }

  constexpr std::array<
      std::size_t ExactFacetDescentSegmentCounters::*, 6>
      aggregate_counter_fields{
          &ExactFacetDescentSegmentCounters::
              source_arc_classification_count,
          &ExactFacetDescentSegmentCounters::
              source_atom_distance_evaluation_count,
          &ExactFacetDescentSegmentCounters::
              source_atom_maximum_comparison_count,
          &ExactFacetDescentSegmentCounters::
              center_displacement_evaluation_count,
          &ExactFacetDescentSegmentCounters::exact_level_relation_count,
          &ExactFacetDescentSegmentCounters::
              convex_identity_certification_count};
  for (const auto counter_field : aggregate_counter_fields) {
    ExactFacetDescentChainResult bad_counter = result;
    ++(bad_counter.counters.accumulated_probe_counters.*counter_field);
    rejects(
        bad_counter,
        "chain verifier rejects every falsified accumulated segment counter");
  }

  ExactFacetDescentChainResult bad_decision = result;
  bad_decision.decision = ExactFacetDescentChainDecision::
                              certified_prefix_strict_segment_budget_exhausted;
  rejects(
      bad_decision,
      "chain verifier treats the terminal outcome as untrusted");

  ExactFacetDescentChainResult bad_scope = result;
  bad_scope.scope = ExactFacetDescentChainScope::unspecified;
  rejects(
      bad_scope,
      "chain verifier treats the single-source scope as untrusted");

  const auto wrong_policy = verify_exact_facet_descent_chain(
      cloud,
      facet,
      ExactFacetDescentChainBudget{1U},
      result);
  check(
      !wrong_policy.requested_budget_certified &&
          !wrong_policy.exact_descent_chain_decision_certified,
      "chain verifier never accepts a result under a different external budget");

  const CanonicalPointCloud twin_cloud = descent_chain_fixture_cloud();
  const ExactFacetDescentChainResult twin_result =
      build_exact_facet_descent_chain(twin_cloud, facet, budget);
  const auto identity_check = verify_exact_facet_descent_chain(
      cloud, facet, budget, twin_result);
  check(
      !identity_check.stopping_probe_certified &&
          !identity_check.exact_descent_chain_decision_certified,
      "chain verifier rejects the stopping probe of a twin cloud identity");
}

[[nodiscard]] CanonicalPointCloud critical_arm_fixture_cloud() {
  const std::array<CertifiedPoint3, 4> input{
      point(-2.0, 0.0),
      point(0.0, 3.0),
      point(2.0, 0.0),
      point(2.0, 2.0)};
  return canonical_cloud(input);
}

void test_critical_arm_initial_segment_exact_geometry() {
  const ExactCriticalArmInitialSegmentResult empty;
  check(
      empty.source_decision ==
              ExactCriticalArmSourceDecision::not_certified &&
          empty.decision ==
              ExactCriticalArmInitialSegmentDecision::not_certified &&
          empty.scope == ExactCriticalArmInitialSegmentScope::unspecified &&
          !empty.global_closed_ball.has_value() &&
          !empty.arm_miniball.has_value() &&
          !empty.initial_segment_coefficients.has_value() &&
          !empty.strict_local_parameter_upper_bound.has_value(),
      "default critical arm certifies neither source nor initial germ");

  const CanonicalPointCloud cloud = critical_arm_fixture_cloud();
  const std::array<PointId, 3> critical_shell{2U, 0U, 1U};
  const PointId removed = 0U;
  const ExactCriticalArmInitialSegmentResult result =
      build_exact_critical_arm_initial_segment(
          cloud, critical_shell, removed);
  const ExactCriticalArmInitialSegmentVerification verification =
      verify_exact_critical_arm_initial_segment(
          cloud, critical_shell, removed, result);

  check(
      result.critical_shell_point_ids ==
              std::vector<PointId>({0U, 1U, 2U}) &&
          result.removed_shell_point_id == removed &&
          result.critical_shell_miniball.center ==
              center(0, 5, 0, 6) &&
          result.critical_shell_miniball.squared_radius ==
              level(169, 36) &&
          result.critical_shell_miniball.support_point_ids ==
              std::vector<PointId>({0U, 1U, 2U}),
      "critical triangle reconstructs c=(0,5/6) and a=169/36");
  check(
      result.global_closed_ball.has_value() &&
          result.closed_rank == 3U && result.order == 2U,
      "complete critical sphere has closed rank three and order two");
  if (!result.global_closed_ball.has_value()) {
    return;
  }
  const auto& source_ball = *result.global_closed_ball;
  check(
      source_ball.validated_for(cloud) &&
          source_ball.interior_ids().empty() &&
          matches_ids(source_ball.shell_ids(), {0U, 1U, 2U}) &&
          matches_ids(source_ball.exterior_ids(), {3U}),
      "critical source partition keeps the fourth point strictly exterior");

  check(
      result.arm_facet_point_ids ==
              std::vector<PointId>({1U, 2U}) &&
          result.arm_miniball.has_value() &&
          result.initial_segment_coefficients.has_value() &&
          result.removed_point_target_squared_distance.has_value() &&
          result.removed_point_outgoing_linear_coefficient.has_value() &&
          result.strict_local_parameter_upper_bound.has_value(),
      "removing point zero constructs the complete two-point arm payload");
  if (!result.arm_miniball.has_value() ||
      !result.initial_segment_coefficients.has_value() ||
      !result.removed_point_target_squared_distance.has_value() ||
      !result.removed_point_outgoing_linear_coefficient.has_value() ||
      !result.strict_local_parameter_upper_bound.has_value()) {
    return;
  }
  const auto& arm = *result.arm_miniball;
  const auto& segment = *result.initial_segment_coefficients;
  check(
      arm.center == center(2, 3, 0, 2) &&
          arm.squared_radius == level(13, 4) &&
          arm.support_point_ids == std::vector<PointId>({1U, 2U}) &&
          segment.source_atom_level == level(169, 36) &&
          segment.successor_atom_level == level(13, 4) &&
          segment.center_squared_displacement == level(13, 9),
      "initial arm stores cF=(1,3/2), b=13/4 and delta=13/9");
  check(
      !segment.centers_equal &&
          !segment.source_endpoint_strict_sublevel &&
          segment.quadratic_max_upper_bound_certified &&
          segment.closed_segment_nonstrict_sublevel &&
          segment.half_open_segment_strict_sublevel &&
          *result.removed_point_target_squared_distance ==
              level(45, 4) &&
          *result.removed_point_outgoing_linear_coefficient ==
              rational(46, 9),
      "removed point has target distance 45/4 and outgoing coefficient 46/9");

  check(
      result.negative_exterior_direction_constraints.size() == 1U,
      "one negatively directed exterior point constrains the local germ");
  if (result.negative_exterior_direction_constraints.size() != 1U) {
    return;
  }
  const auto& exterior =
      result.negative_exterior_direction_constraints.front();
  check(
      exterior.point_id == 3U &&
          exterior.source_clearance_above_critical_level ==
              level(2, 3) &&
          exterior.source_outgoing_linear_coefficient ==
              rational(-50, 9) &&
          exterior.parameter_upper_bound == rational(3, 50) &&
          *result.strict_local_parameter_upper_bound ==
              rational(3, 50),
      "exterior point has A=2/3, B=-50/9 and exact bound tau=3/50");

  check(
      result.critical_shell_is_positive_minimal_support &&
          result.global_shell_matches_critical_shell &&
          result.closed_rank_and_order_supported &&
          result.critical_source_certified &&
          result.arm_facet_cardinality_certified &&
          result.arm_miniball_strict_decrease_certified &&
          result.positive_center_displacement_certified &&
          result.removed_point_outgoing_direction_certified &&
          result.removed_point_target_outside_arm_ball_certified &&
          result.exterior_prefix_bound_certified &&
          result.closed_initial_segment_nonstrict_critical_sublevel &&
          result.half_open_initial_segment_strict_critical_sublevel,
      "every exact source, direction and local-prefix fact closes");
  check(
      result.counters.global_closed_ball_query_count == 1U &&
          result.counters.
                  global_closed_ball_distance_evaluation_count == 4U &&
          result.counters.arm_miniball_build_count == 1U &&
          result.counters.arm_source_distance_evaluation_count == 2U &&
          result.counters.removed_point_target_distance_evaluation_count ==
              1U &&
          result.counters.
                  removed_point_directional_coefficient_evaluation_count ==
              1U &&
          result.counters.exterior_point_clearance_evaluation_count == 1U &&
          result.counters.
                  exterior_point_directional_dot_product_evaluation_count ==
              1U &&
          result.counters.negative_exterior_direction_constraint_count ==
              1U,
      "nontrivial arm exposes its bounded exact work counters");
  check(
      result.source_decision ==
              ExactCriticalArmSourceDecision::
                  critical_source_certified &&
          result.decision ==
              ExactCriticalArmInitialSegmentDecision::
                  strict_initial_arm_segment_certified &&
          result.scope ==
              ExactCriticalArmInitialSegmentScope::
                  single_index_one_critical_arm_initial_germ_segment_only &&
          std::string_view{
              ExactCriticalArmInitialSegmentResult::proof_basis} ==
              "exact_complete_positive_critical_shell_removed_arm_fresh_"
              "miniball_outgoing_direction_local_exterior_bound_v1" &&
          all_critical_arm_initial_certificates_close(verification),
      "initial critical arm decision, scope and fresh replay close exactly");
}

void test_critical_arm_descent_budgets_exclude_initial_segment() {
  const ExactCriticalArmDescentResult empty;
  check(
      empty.decision == ExactCriticalArmDescentDecision::not_certified &&
          empty.scope == ExactCriticalArmDescentScope::unspecified &&
          !empty.facet_descent_chain.has_value() &&
          !empty.initial_segment_excluded_from_chain_budget &&
          !empty.exact_initial_to_chain_seam_certified &&
          !empty.source_open_composite_path_strict_critical_sublevel,
      "default composite arm certifies no path or budget convention");

  const CanonicalPointCloud cloud = critical_arm_fixture_cloud();
  const std::array<PointId, 3> critical_shell{0U, 1U, 2U};
  constexpr PointId removed = 0U;

  const ExactFacetDescentChainBudget zero_budget{0U};
  const ExactCriticalArmDescentResult zero =
      build_exact_critical_arm_descent(
          cloud, critical_shell, removed, zero_budget);
  const ExactCriticalArmDescentVerification zero_verification =
      verify_exact_critical_arm_descent(
          cloud, critical_shell, removed, zero_budget, zero);
  check(
      zero.facet_descent_chain.has_value() &&
          zero.initial_segment.decision ==
              ExactCriticalArmInitialSegmentDecision::
                  strict_initial_arm_segment_certified,
      "zero chain budget still owns the dedicated initial segment");
  if (!zero.facet_descent_chain.has_value()) {
    return;
  }
  const auto& zero_chain = *zero.facet_descent_chain;
  check(
      zero_chain.nodes.size() == 1U &&
          zero_chain.nodes.front().facet_point_ids ==
              std::vector<PointId>({1U, 2U}) &&
          zero_chain.nodes.front().center == center(2, 3, 0, 2) &&
          zero_chain.nodes.front().squared_level == level(13, 4) &&
          zero_chain.committed_segment_witnesses.empty() &&
          zero_chain.stopping_probe.has_value() &&
          zero_chain.decision ==
              ExactFacetDescentChainDecision::
                  certified_prefix_strict_segment_budget_exhausted,
      "budget zero starts the 6.5 chain at F without committing its frontier");
  check(
      zero.initial_segment_excluded_from_chain_budget &&
          zero.exact_initial_to_chain_seam_certified &&
          zero.source_open_composite_path_strict_critical_sublevel &&
          zero.counters.initial_segment_probe_count == 1U &&
          zero.counters.certified_initial_segment_count == 1U &&
          zero.counters.facet_chain_build_count == 1U &&
          zero.counters.initial_to_chain_seam_replay_count == 1U &&
          zero.counters.committed_chain_strict_segment_count == 0U &&
          zero.counters.committed_composite_path_segment_count == 1U &&
          zero.counters.chain_budget_terminal_count == 1U &&
          zero.decision ==
              ExactCriticalArmDescentDecision::
                  certified_prefix_strict_segment_budget_exhausted &&
          zero.scope ==
              ExactCriticalArmDescentScope::
                  single_index_one_critical_arm_plus_canonical_strict_chain_only &&
          all_critical_arm_descent_certificates_close(zero_verification),
      "zero chain budget counts exactly the initial germ outside the budget");

  const ExactFacetDescentChainBudget one_budget{1U};
  const ExactCriticalArmDescentResult one =
      build_exact_critical_arm_descent(
          cloud, critical_shell, removed, one_budget);
  const ExactCriticalArmDescentVerification one_verification =
      verify_exact_critical_arm_descent(
          cloud, critical_shell, removed, one_budget, one);
  check(
      one.facet_descent_chain.has_value(),
      "one chain budget returns the regular continuation payload");
  if (!one.facet_descent_chain.has_value()) {
    return;
  }
  const auto& one_chain = *one.facet_descent_chain;
  check(
      one_chain.nodes.size() == 2U &&
          one_chain.nodes[0].facet_point_ids ==
              std::vector<PointId>({1U, 2U}) &&
          one_chain.nodes[1].facet_point_ids ==
              std::vector<PointId>({1U, 3U}) &&
          one_chain.nodes[1].center == center(2, 5, 0, 2) &&
          one_chain.nodes[1].squared_level == level(5, 4) &&
          one_chain.committed_segment_witnesses.size() == 1U &&
          one_chain.decision ==
              ExactFacetDescentChainDecision::
                  complete_at_regular_active_facet,
      "budget one commits F={1,2} to active facet {1,3} exactly");
  check(
      one.initial_segment_excluded_from_chain_budget &&
          one.exact_initial_to_chain_seam_certified &&
          one.source_open_composite_path_strict_critical_sublevel &&
          one.counters.committed_chain_strict_segment_count == 1U &&
          one.counters.committed_composite_path_segment_count == 2U &&
          one.counters.active_terminal_count == 1U &&
          one.counters.chain_budget_terminal_count == 0U &&
          one.decision ==
              ExactCriticalArmDescentDecision::
                  complete_at_regular_active_facet &&
          std::string_view{ExactCriticalArmDescentResult::proof_basis} ==
              "exact_critical_arm_initial_segment_exact_seam_single_source_"
              "canonical_facet_descent_chain_v1" &&
          all_critical_arm_descent_certificates_close(one_verification),
      "one budgeted chain segment closes a two-segment composite arm");

  const std::array<CertifiedPoint3, 4> degenerate_input{
      point(-3.0, 0.0),
      point(-1.0, 0.0),
      point(1.0, 2.0),
      point(3.0, 0.0)};
  const CanonicalPointCloud degenerate_cloud =
      canonical_cloud(degenerate_input);
  const std::array<PointId, 2> degenerate_shell{0U, 3U};
  const ExactFacetDescentChainBudget terminal_budget{4U};
  const ExactCriticalArmDescentResult degenerate =
      build_exact_critical_arm_descent(
          degenerate_cloud,
          degenerate_shell,
          0U,
          terminal_budget);
  const auto degenerate_verification = verify_exact_critical_arm_descent(
      degenerate_cloud,
      degenerate_shell,
      0U,
      terminal_budget,
      degenerate);
  check(
      degenerate.initial_segment.arm_facet_point_ids ==
              std::vector<PointId>({1U, 2U, 3U}) &&
          degenerate.initial_segment.arm_miniball.has_value() &&
          degenerate.facet_descent_chain.has_value(),
      "supported critical source reaches its nonessential arm facet");
  if (!degenerate.initial_segment.arm_miniball.has_value() ||
      !degenerate.facet_descent_chain.has_value()) {
    return;
  }
  check(
      degenerate.initial_segment.arm_miniball->center ==
              center(1, 0, 0) &&
          degenerate.initial_segment.arm_miniball->squared_radius ==
              level(4) &&
          degenerate.facet_descent_chain->nodes.size() == 1U &&
          degenerate.facet_descent_chain->
              committed_segment_witnesses.empty() &&
          degenerate.facet_descent_chain->decision ==
              ExactFacetDescentChainDecision::
                  certified_prefix_blocked_unsupported_degeneracy &&
          degenerate.counters.unsupported_chain_terminal_count == 1U &&
          degenerate.counters.
                  committed_composite_path_segment_count == 1U &&
          degenerate.decision ==
              ExactCriticalArmDescentDecision::
                  certified_prefix_blocked_unsupported_degeneracy &&
          all_critical_arm_descent_certificates_close(
              degenerate_verification),
      "composite maps a nonessential arm facet to an unsupported prefix");
}

void test_critical_arm_supports_interior_and_maximum_rank() {
  {
    const std::array<CertifiedPoint3, 3> input{
        point(-2.0), point(0.0), point(2.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const std::array<PointId, 2> critical_shell{0U, 2U};
    const ExactCriticalArmInitialSegmentResult result =
        build_exact_critical_arm_initial_segment(
            cloud, critical_shell, 0U);
    const auto verification = verify_exact_critical_arm_initial_segment(
        cloud, critical_shell, 0U, result);
    check(
        result.global_closed_ball.has_value() &&
            result.arm_miniball.has_value() &&
            result.initial_segment_coefficients.has_value(),
        "critical event with an interior point retains every exact payload");
    if (!result.global_closed_ball.has_value() ||
        !result.arm_miniball.has_value() ||
        !result.initial_segment_coefficients.has_value()) {
      return;
    }
    check(
        matches_ids(result.global_closed_ball->interior_ids(), {1U}) &&
            matches_ids(
                result.global_closed_ball->shell_ids(), {0U, 2U}) &&
            result.closed_rank == 3U && result.order == 2U &&
            result.arm_facet_point_ids ==
                std::vector<PointId>({1U, 2U}) &&
            result.arm_miniball->center == center(1, 0, 0) &&
            result.arm_miniball->squared_radius == level(1),
        "interior point joins F while the removed endpoint leaves the shell");
    check(
        result.initial_segment_coefficients->source_atom_level ==
                level(4) &&
            result.initial_segment_coefficients->successor_atom_level ==
                level(1) &&
            result.initial_segment_coefficients->
                    center_squared_displacement == level(1) &&
            result.removed_point_target_squared_distance ==
                std::optional<ExactLevel>{level(9)} &&
            result.removed_point_outgoing_linear_coefficient ==
                std::optional<ExactRational>{rational(4)} &&
            result.negative_exterior_direction_constraints.empty() &&
            result.strict_local_parameter_upper_bound ==
                std::optional<ExactRational>{rational(1)} &&
            all_critical_arm_initial_certificates_close(verification),
        "interior fixture has coefficients a=4, b=1, delta=1 and tau=1");
  }

  {
    const std::array<CertifiedPoint3, 4> input{
        point(-1.0, -1.0, 1.0),
        point(-1.0, 1.0, -1.0),
        point(1.0, -1.0, -1.0),
        point(1.0, 1.0, 1.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const std::array<PointId, 4> critical_shell{0U, 1U, 2U, 3U};
    const ExactCriticalArmInitialSegmentResult result =
        build_exact_critical_arm_initial_segment(
            cloud, critical_shell, 0U);
    const auto verification = verify_exact_critical_arm_initial_segment(
        cloud, critical_shell, 0U, result);
    check(
        result.closed_rank == 4U && result.order == 3U &&
            result.critical_shell_is_positive_minimal_support &&
            result.global_shell_matches_critical_shell &&
            result.arm_facet_point_ids ==
                std::vector<PointId>({1U, 2U, 3U}) &&
            result.arm_miniball.has_value() &&
            result.initial_segment_coefficients.has_value(),
        "positive four-point critical support constructs a tetrahedral arm");
    if (!result.arm_miniball.has_value() ||
        !result.initial_segment_coefficients.has_value()) {
      return;
    }
    check(
        result.critical_shell_miniball.center == center(0, 0, 0) &&
            result.critical_shell_miniball.squared_radius == level(3) &&
            result.arm_miniball->center == center(1, 1, -1, 3) &&
            result.arm_miniball->squared_radius == level(8, 3) &&
            result.initial_segment_coefficients->
                    center_squared_displacement == level(1, 3) &&
            result.removed_point_outgoing_linear_coefficient ==
                std::optional<ExactRational>{rational(2)} &&
            result.strict_local_parameter_upper_bound ==
                std::optional<ExactRational>{rational(1)} &&
            all_critical_arm_initial_certificates_close(verification),
        "tetrahedral arm closes b=8/3, delta=1/3, B_u=2 and tau=1");
  }

  {
    const std::array<CertifiedPoint3, 11> input{
        point(-5.0),
        point(-4.0),
        point(-3.0),
        point(-2.0),
        point(-1.0),
        point(0.0),
        point(1.0),
        point(2.0),
        point(3.0),
        point(4.0),
        point(5.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const std::array<PointId, 2> critical_shell{0U, 10U};
    const ExactCriticalArmInitialSegmentResult result =
        build_exact_critical_arm_initial_segment(
            cloud, critical_shell, 0U);
    const auto verification = verify_exact_critical_arm_initial_segment(
        cloud, critical_shell, 0U, result);
    check(
        result.closed_rank == 11U && result.order == 10U &&
            result.closed_rank_and_order_supported &&
            result.arm_facet_point_ids ==
                std::vector<PointId>(
                    {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U}) &&
            result.arm_miniball.has_value() &&
            result.initial_segment_coefficients.has_value(),
        "closed rank eleven constructs the maximum ten-point arm facet");
    if (!result.arm_miniball.has_value() ||
        !result.initial_segment_coefficients.has_value()) {
      return;
    }
    check(
        result.critical_shell_miniball.center == center(0, 0, 0) &&
            result.critical_shell_miniball.squared_radius == level(25) &&
            result.arm_miniball->center == center(1, 0, 0, 2) &&
            result.arm_miniball->squared_radius == level(81, 4) &&
            result.initial_segment_coefficients->source_atom_level ==
                level(25) &&
            result.initial_segment_coefficients->successor_atom_level ==
                level(81, 4) &&
            result.initial_segment_coefficients->
                    center_squared_displacement == level(1, 4) &&
            result.removed_point_outgoing_linear_coefficient ==
                std::optional<ExactRational>{rational(5)} &&
            result.decision ==
                ExactCriticalArmInitialSegmentDecision::
                    strict_initial_arm_segment_certified &&
            all_critical_arm_initial_certificates_close(verification),
        "rank-eleven boundary closes cF=1/2, b=81/4 and delta=1/4");
    check(
        ExactCriticalArmInitialSegmentResult::
                maximum_supported_closed_rank == 11U &&
            ExactCriticalArmInitialSegmentResult::
                maximum_supported_order == 10U,
        "critical arm advertises the phase-six closed-rank cap");
  }
}

void test_critical_arm_unsupported_sources_fail_closed() {
  {
    const std::array<CertifiedPoint3, 4> input{
        point(-1.0, 0.0),
        point(0.0, 1.0),
        point(1.0, 0.0),
        point(0.0, -1.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const std::array<PointId, 4> nonminimal_shell{0U, 1U, 2U, 3U};
    const ExactCriticalArmInitialSegmentResult result =
        build_exact_critical_arm_initial_segment(
            cloud, nonminimal_shell, 0U);
    const auto verification = verify_exact_critical_arm_initial_segment(
        cloud, nonminimal_shell, 0U, result);
    check(
        !result.critical_shell_is_positive_minimal_support &&
            !result.global_shell_matches_critical_shell &&
            !result.global_closed_ball.has_value() &&
            result.source_decision ==
                ExactCriticalArmSourceDecision::
                    unsupported_nonminimal_or_nonpositive_shell &&
            result.decision ==
                ExactCriticalArmInitialSegmentDecision::
                    no_segment_unsupported_critical_source &&
            result.arm_facet_point_ids.empty() &&
            !result.arm_miniball.has_value() &&
            all_critical_arm_initial_certificates_close(verification),
        "nonminimal four-point shell is classified exactly and emits no arm");

    const ExactFacetDescentChainBudget zero_budget{0U};
    const ExactCriticalArmDescentResult unsupported_composite =
        build_exact_critical_arm_descent(
            cloud, nonminimal_shell, 0U, zero_budget);
    const auto unsupported_composite_verification =
        verify_exact_critical_arm_descent(
            cloud,
            nonminimal_shell,
            0U,
            zero_budget,
            unsupported_composite);
    check(
        unsupported_composite.initial_segment.decision ==
                ExactCriticalArmInitialSegmentDecision::
                    no_segment_unsupported_critical_source &&
            !unsupported_composite.facet_descent_chain.has_value() &&
            !unsupported_composite.
                initial_segment_excluded_from_chain_budget &&
            unsupported_composite.counters.
                    source_unsupported_terminal_count == 1U &&
            unsupported_composite.decision ==
                ExactCriticalArmDescentDecision::
                    no_descent_unsupported_critical_source &&
            all_critical_arm_descent_certificates_close(
                unsupported_composite_verification),
        "unsupported critical source maps to no composite chain");

    check_throws<std::invalid_argument>(
        [&cloud, &nonminimal_shell]() {
          const ExactFacetDescentChainBudget excessive{
              ExactFacetDescentChainBudget::
                  maximum_supported_committed_strict_segment_count + 1U};
          static_cast<void>(build_exact_critical_arm_descent(
              cloud, nonminimal_shell, 0U, excessive));
        },
        "an unsupported source cannot bypass the composite chain budget cap");
  }

  {
    const std::array<CertifiedPoint3, 3> input{
        point(-1.0, 0.0), point(1.0, 0.0), point(0.0, 1.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const std::array<PointId, 2> incomplete_shell{0U, 2U};
    const ExactCriticalArmInitialSegmentResult result =
        build_exact_critical_arm_initial_segment(
            cloud, incomplete_shell, 0U);
    const auto verification = verify_exact_critical_arm_initial_segment(
        cloud, incomplete_shell, 0U, result);
    check(
        result.critical_shell_is_positive_minimal_support &&
            !result.global_shell_matches_critical_shell &&
            result.global_closed_ball.has_value() &&
            matches_ids(
                result.global_closed_ball->shell_ids(), {0U, 1U, 2U}) &&
            result.source_decision ==
                ExactCriticalArmSourceDecision::
                    unsupported_incomplete_global_shell &&
            result.decision ==
                ExactCriticalArmInitialSegmentDecision::
                    no_segment_unsupported_critical_source &&
            !result.arm_miniball.has_value() &&
            all_critical_arm_initial_certificates_close(verification),
        "omitted global shell point blocks the initial germ fail-closed");
  }

  {
    const std::array<CertifiedPoint3, 12> input{
        point(-6.0),
        point(-5.0),
        point(-4.0),
        point(-3.0),
        point(-2.0),
        point(-1.0),
        point(0.0),
        point(1.0),
        point(2.0),
        point(3.0),
        point(4.0),
        point(5.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const std::array<PointId, 2> critical_shell{0U, 11U};
    const ExactCriticalArmInitialSegmentResult result =
        build_exact_critical_arm_initial_segment(
            cloud, critical_shell, 0U);
    const auto verification = verify_exact_critical_arm_initial_segment(
        cloud, critical_shell, 0U, result);
    check(
        result.critical_shell_is_positive_minimal_support &&
            result.global_shell_matches_critical_shell &&
            result.closed_rank == 12U && result.order == 11U &&
            !result.closed_rank_and_order_supported &&
            result.source_decision ==
                ExactCriticalArmSourceDecision::
                    unsupported_closed_rank_or_order &&
            result.decision ==
                ExactCriticalArmInitialSegmentDecision::
                    no_segment_unsupported_critical_source &&
            result.arm_facet_point_ids.empty() &&
            !result.arm_miniball.has_value() &&
            all_critical_arm_initial_certificates_close(verification),
        "closed rank twelve is reported but never enters the bounded arm oracle");
  }
}

void test_critical_arm_verifiers_reject_falsifications() {
  const CanonicalPointCloud cloud = critical_arm_fixture_cloud();
  const std::array<PointId, 3> critical_shell{0U, 1U, 2U};
  constexpr PointId removed = 0U;
  const ExactCriticalArmInitialSegmentResult result =
      build_exact_critical_arm_initial_segment(
          cloud, critical_shell, removed);

  const auto rejects_initial =
      [&cloud, &critical_shell](
          const ExactCriticalArmInitialSegmentResult& candidate,
          const std::string& message) {
        const auto verification =
            verify_exact_critical_arm_initial_segment(
                cloud, critical_shell, removed, candidate);
        check(
            !verification.
                exact_critical_arm_initial_segment_decision_certified,
            message);
      };

  ExactCriticalArmInitialSegmentResult bad_miniball = result;
  bad_miniball.critical_shell_miniball.squared_radius = level(5);
  rejects_initial(
      bad_miniball,
      "critical-arm verifier rejects a falsified source miniball");

  ExactCriticalArmInitialSegmentResult bad_partition = result;
  bad_partition.global_closed_ball.emplace(
      morsehgp3d::spatial::brute_force_closed_ball(
          cloud, center(0, 5, 0, 6), level(5)));
  rejects_initial(
      bad_partition,
      "critical-arm verifier rejects a falsified global partition");

  ExactCriticalArmInitialSegmentResult bad_source_decision = result;
  bad_source_decision.source_decision =
      ExactCriticalArmSourceDecision::unsupported_incomplete_global_shell;
  rejects_initial(
      bad_source_decision,
      "critical-arm verifier treats the source decision as untrusted");

  ExactCriticalArmInitialSegmentResult bad_arm_facet = result;
  bad_arm_facet.arm_facet_point_ids.front() = 0U;
  rejects_initial(
      bad_arm_facet,
      "critical-arm verifier rejects a falsified arm facet");

  ExactCriticalArmInitialSegmentResult bad_arm_miniball = result;
  bad_arm_miniball.arm_miniball->squared_radius = level(3);
  rejects_initial(
      bad_arm_miniball,
      "critical-arm verifier rejects a falsified arm miniball");

  ExactCriticalArmInitialSegmentResult bad_coefficients = result;
  bad_coefficients.initial_segment_coefficients->
      center_squared_displacement = level(1);
  rejects_initial(
      bad_coefficients,
      "critical-arm verifier rejects falsified initial coefficients");

  ExactCriticalArmInitialSegmentResult bad_exterior = result;
  bad_exterior.negative_exterior_direction_constraints.front().
      source_outgoing_linear_coefficient = rational(-5);
  rejects_initial(
      bad_exterior,
      "critical-arm verifier rejects a falsified exterior constraint");

  ExactCriticalArmInitialSegmentResult bad_removed_witness = result;
  bad_removed_witness.removed_point_outgoing_linear_coefficient =
      rational(5);
  rejects_initial(
      bad_removed_witness,
      "critical-arm verifier rejects a falsified removed-point witness");

  ExactCriticalArmInitialSegmentResult bad_bound = result;
  bad_bound.strict_local_parameter_upper_bound = rational(1);
  rejects_initial(
      bad_bound,
      "critical-arm verifier rejects a falsified local parameter bound");

  ExactCriticalArmInitialSegmentResult bad_fact = result;
  bad_fact.removed_point_outgoing_direction_certified = false;
  rejects_initial(
      bad_fact,
      "critical-arm verifier rejects a falsified outgoing-direction fact");

  ExactCriticalArmInitialSegmentResult bad_counter = result;
  ++bad_counter.counters.convex_identity_certification_count;
  rejects_initial(
      bad_counter,
      "critical-arm verifier rejects falsified exact work counters");

  ExactCriticalArmInitialSegmentResult bad_decision = result;
  bad_decision.decision =
      ExactCriticalArmInitialSegmentDecision::
          no_segment_unsupported_critical_source;
  rejects_initial(
      bad_decision,
      "critical-arm verifier treats the initial decision as untrusted");

  ExactCriticalArmInitialSegmentResult bad_scope = result;
  bad_scope.scope = ExactCriticalArmInitialSegmentScope::unspecified;
  rejects_initial(
      bad_scope,
      "critical-arm verifier treats the initial scope as untrusted");

  const auto wrong_removed_check = verify_exact_critical_arm_initial_segment(
      cloud, critical_shell, 1U, result);
  check(
      !wrong_removed_check.removed_shell_point_identity_certified &&
          !wrong_removed_check.
              exact_critical_arm_initial_segment_decision_certified,
      "critical-arm verifier rejects a different external arm label");

  const CanonicalPointCloud twin_cloud = critical_arm_fixture_cloud();
  const ExactCriticalArmInitialSegmentResult twin_result =
      build_exact_critical_arm_initial_segment(
          twin_cloud, critical_shell, removed);
  const auto twin_initial_check =
      verify_exact_critical_arm_initial_segment(
          cloud, critical_shell, removed, twin_result);
  check(
      !twin_initial_check.global_closed_ball_certified &&
          !twin_initial_check.
              exact_critical_arm_initial_segment_decision_certified,
      "critical-arm verifier rejects a partition from a twin cloud identity");

  const ExactFacetDescentChainBudget budget{1U};
  const ExactCriticalArmDescentResult composite =
      build_exact_critical_arm_descent(
          cloud, critical_shell, removed, budget);
  const auto rejects_composite =
      [&cloud, &critical_shell, budget](
          const ExactCriticalArmDescentResult& candidate,
          const std::string& message) {
        const auto verification = verify_exact_critical_arm_descent(
            cloud, critical_shell, removed, budget, candidate);
        check(
            !verification.exact_critical_arm_descent_decision_certified,
            message);
      };

  ExactCriticalArmDescentResult bad_initial = composite;
  bad_initial.initial_segment.removed_point_outgoing_direction_certified =
      false;
  rejects_composite(
      bad_initial,
      "composite verifier rejects a falsified embedded initial segment");

  ExactCriticalArmDescentResult missing_chain = composite;
  missing_chain.facet_descent_chain.reset();
  rejects_composite(
      missing_chain,
      "composite verifier rejects an absent supported-source chain");

  ExactCriticalArmDescentResult bad_chain = composite;
  bad_chain.facet_descent_chain->nodes.front().squared_level = level(3);
  rejects_composite(
      bad_chain,
      "composite verifier rejects a falsified canonical chain");

  ExactCriticalArmDescentResult bad_seam = composite;
  bad_seam.exact_initial_to_chain_seam_certified = false;
  rejects_composite(
      bad_seam,
      "composite verifier rejects a falsified initial-to-chain seam");

  ExactCriticalArmDescentResult bad_budget_separation = composite;
  bad_budget_separation.initial_segment_excluded_from_chain_budget = false;
  rejects_composite(
      bad_budget_separation,
      "composite verifier rejects a falsified initial-budget separation");

  ExactCriticalArmDescentResult bad_path = composite;
  bad_path.source_open_composite_path_strict_critical_sublevel = false;
  rejects_composite(
      bad_path,
      "composite verifier rejects a falsified strict path fact");

  ExactCriticalArmDescentResult bad_composite_counter = composite;
  ++bad_composite_counter.counters.
      committed_composite_path_segment_count;
  rejects_composite(
      bad_composite_counter,
      "composite verifier rejects falsified aggregate counters");

  ExactCriticalArmDescentResult bad_composite_decision = composite;
  bad_composite_decision.decision =
      ExactCriticalArmDescentDecision::
          certified_prefix_strict_segment_budget_exhausted;
  rejects_composite(
      bad_composite_decision,
      "composite verifier treats its mapped decision as untrusted");

  ExactCriticalArmDescentResult bad_composite_scope = composite;
  bad_composite_scope.scope = ExactCriticalArmDescentScope::unspecified;
  rejects_composite(
      bad_composite_scope,
      "composite verifier treats its scope as untrusted");

  const auto wrong_budget_check = verify_exact_critical_arm_descent(
      cloud,
      critical_shell,
      removed,
      ExactFacetDescentChainBudget{0U},
      composite);
  check(
      !wrong_budget_check.requested_chain_budget_certified &&
          !wrong_budget_check.
              exact_critical_arm_descent_decision_certified,
      "composite verifier rejects a different external chain budget");

  const ExactCriticalArmDescentResult twin_composite =
      build_exact_critical_arm_descent(
          twin_cloud, critical_shell, removed, budget);
  const auto twin_composite_check = verify_exact_critical_arm_descent(
      cloud, critical_shell, removed, budget, twin_composite);
  check(
      !twin_composite_check.initial_segment_certified &&
          !twin_composite_check.
              exact_critical_arm_descent_decision_certified,
      "composite verifier rejects exact payloads from a twin cloud identity");
}

void test_invalid_critical_arm_inputs_are_rejected() {
  const std::array<CertifiedPoint3, 6> input{
      point(-2.0, 0.0),
      point(0.0, 3.0),
      point(2.0, 0.0),
      point(2.0, 2.0),
      point(4.0, 0.0),
      point(5.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);

  check_throws<std::invalid_argument>(
      [&cloud]() {
        static_cast<void>(
            build_exact_critical_arm_initial_segment(cloud, {}, 0U));
      },
      "empty critical shell is rejected");

  const std::array<PointId, 1> singleton{0U};
  check_throws<std::invalid_argument>(
      [&cloud, &singleton]() {
        static_cast<void>(
            build_exact_critical_arm_initial_segment(
                cloud, singleton, 0U));
      },
      "one-point critical shell is rejected");

  const std::array<PointId, 5> too_large{0U, 1U, 2U, 3U, 4U};
  check_throws<std::invalid_argument>(
      [&cloud, &too_large]() {
        static_cast<void>(
            build_exact_critical_arm_initial_segment(
                cloud, too_large, 0U));
      },
      "critical shell larger than four points is rejected");

  const std::array<PointId, 3> duplicate{0U, 1U, 1U};
  check_throws<std::invalid_argument>(
      [&cloud, &duplicate]() {
        static_cast<void>(
            build_exact_critical_arm_initial_segment(
                cloud, duplicate, 0U));
      },
      "duplicate critical-shell PointIds are rejected");

  const std::array<PointId, 3> outside{0U, 1U, 6U};
  check_throws<std::out_of_range>(
      [&cloud, &outside]() {
        static_cast<void>(
            build_exact_critical_arm_initial_segment(
                cloud, outside, 0U));
      },
      "critical-shell PointId outside the cloud is rejected");

  const std::array<PointId, 3> valid_shell{0U, 1U, 2U};
  check_throws<std::invalid_argument>(
      [&cloud, &valid_shell]() {
        static_cast<void>(
            build_exact_critical_arm_initial_segment(
                cloud, valid_shell, 3U));
      },
      "removed point outside the supplied critical shell is rejected");

  check_throws<std::invalid_argument>(
      [&cloud, &valid_shell]() {
        const ExactFacetDescentChainBudget excessive{
            ExactFacetDescentChainBudget::
                maximum_supported_committed_strict_segment_count + 1U};
        static_cast<void>(build_exact_critical_arm_descent(
            cloud, valid_shell, 0U, excessive));
      },
      "critical-arm composite rejects a chain budget above its cap");
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
  test_strict_descent_arc_with_equal_source_cutoff();
  test_strict_descent_arc_with_lower_source_cutoff();
  test_descent_arc_omits_successor_for_active_facet();
  test_descent_arc_omits_successor_for_nonessential_facet();
  test_descent_arc_verifier_rejects_every_new_result_layer();
  test_descent_segment_equal_cutoff_is_half_open_strict();
  test_descent_segment_accepts_equal_centers_below_source_level();
  test_descent_segment_omits_witness_for_active_facet();
  test_descent_segment_omits_witness_for_unsupported_facet();
  test_descent_segment_verifier_rejects_every_new_result_layer();
  test_descent_chain_concatenates_exact_segments();
  test_descent_chain_keeps_miniball_and_atom_seams_distinct();
  test_descent_chain_budgets_and_terminal_decisions();
  test_descent_chain_verifier_rejects_every_result_layer();
  test_critical_arm_initial_segment_exact_geometry();
  test_critical_arm_descent_budgets_exclude_initial_segment();
  test_critical_arm_supports_interior_and_maximum_rank();
  test_critical_arm_unsupported_sources_fail_closed();
  test_critical_arm_verifiers_reject_falsifications();
  test_invalid_critical_arm_inputs_are_rejected();
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
