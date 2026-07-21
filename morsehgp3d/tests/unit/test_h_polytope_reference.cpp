#include "morsehgp3d/spatial/h_polytope_reference.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactAffineForm3;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::spatial::ExactBoundedHPolytopeReferenceBudget;
using morsehgp3d::spatial::ExactBoundedHPolytopeReferenceDecision;
using morsehgp3d::spatial::ExactBoundedHPolytopeReferenceResult;
using morsehgp3d::spatial::ExactDyadicAabb3;
using morsehgp3d::spatial::ExactHPolytopeBoundaryKind;
using morsehgp3d::spatial::ExactHPolytopeHalfspace3;
using morsehgp3d::spatial::ExactHPolytopeHalfspaceKind;
using morsehgp3d::spatial::ExactHPolytopeHalfspaceRole;
using morsehgp3d::spatial::HPolytopeConstraintDomain;
using morsehgp3d::spatial::HPolytopeConstraintId;
using morsehgp3d::spatial::build_exact_bounded_h_polytope_reference;

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
    std::cerr << "FAIL: " << message << " (unexpected exception: "
              << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] std::uint64_t word(double value) {
  return std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] ExactDyadicAabb3 box(
    double lower_x,
    double lower_y,
    double lower_z,
    double upper_x,
    double upper_y,
    double upper_z) {
  return ExactDyadicAabb3{
      {word(lower_x), word(lower_y), word(lower_z)},
      {word(upper_x), word(upper_y), word(upper_z)}};
}

[[nodiscard]] HPolytopeConstraintId constraint_id(
    HPolytopeConstraintDomain domain,
    std::uint64_t first,
    std::uint64_t second) {
  return HPolytopeConstraintId{domain, first, second};
}

[[nodiscard]] ExactAffineForm3 form(int a, int b, int c, int d) {
  return ExactAffineForm3::from_integer_coefficients(
      {BigInt{a}, BigInt{b}, BigInt{c}, BigInt{d}});
}

[[nodiscard]] ExactHPolytopeHalfspace3 halfspace(
    HPolytopeConstraintId id,
    ExactHPolytopeHalfspaceRole role,
    int a,
    int b,
    int c,
    int d) {
  return ExactHPolytopeHalfspace3{id, role, form(a, b, c, d)};
}

[[nodiscard]] std::string point_key(const ExactRational3& point) {
  return point.coordinate(0U).canonical_key() + ":" +
         point.coordinate(1U).canonical_key() + ":" +
         point.coordinate(2U).canonical_key();
}

[[nodiscard]] std::string point_key(int x, int y, int z) {
  return point_key(ExactRational3{BigInt{x}, BigInt{y}, BigInt{z}, BigInt{1}});
}

[[nodiscard]] std::set<std::string> vertex_keys(
    const ExactBoundedHPolytopeReferenceResult& result) {
  std::set<std::string> keys;
  for (const auto& vertex : result.vertices) {
    keys.insert(point_key(vertex.position));
  }
  return keys;
}

[[nodiscard]] std::set<std::string> grid_keys(
    const std::vector<int>& xs,
    const std::vector<int>& ys,
    const std::vector<int>& zs) {
  std::set<std::string> result;
  for (const int x : xs) {
    for (const int y : ys) {
      for (const int z : zs) {
        result.insert(point_key(x, y, z));
      }
    }
  }
  return result;
}

void check_preflight_only(
    const ExactBoundedHPolytopeReferenceResult& result,
    const std::string& message) {
  check(
      result.decision ==
          ExactBoundedHPolytopeReferenceDecision::insufficient_budget,
      message + " returns insufficient_budget");
  check(
      result.audit == morsehgp3d::spatial::ExactBoundedHPolytopeReferenceAudit{},
      message + " does not perform audited geometric work");
  check(
      result.classified_halfspaces.empty() && result.boundary_planes.empty() &&
          result.vertices.empty() && !result.affine_dimension.has_value(),
      message + " does not materialize partial geometry");
}

void test_box_cube_and_axis_clip() {
  const ExactDyadicAabb3 clipping_box = box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0);
  const std::array<ExactHPolytopeHalfspace3, 0> no_halfspaces{};
  const auto cube = build_exact_bounded_h_polytope_reference(
      no_halfspaces, clipping_box);

  check(
      cube.decision ==
          ExactBoundedHPolytopeReferenceDecision::complete_nonempty,
      "the bare clipping box is a complete nonempty cube");
  check(
      cube.requirements.input_halfspace_count == 0U &&
          cube.requirements.conservative_boundary_count == 6U &&
          cube.requirements.conservative_plane_triple_count == 20U &&
          cube.requirements.conservative_feasibility_test_count == 120U &&
          cube.requirements.conservative_vertex_count == 20U &&
          cube.requirements.conservative_incidence_test_count == 120U,
      "the cube preflight accounts for its six artificial boundaries");
  check(
      cube.audit.enumerated_plane_triple_count == 20U &&
          cube.audit.exact_feasibility_test_count == 48U &&
          cube.audit.unique_feasible_vertex_count == 8U &&
          cube.audit.exact_incidence_test_count == 48U &&
          cube.audit.active_incidence_count == 24U,
      "the cube separates 8*6 feasibility and incidence tests from 24 active incidences");
  check(
      cube.affine_dimension == 3U &&
          vertex_keys(cube) ==
              grid_keys({-1, 1}, {-1, 1}, {-1, 1}),
      "the cube has the expected exact vertices and affine dimension");

  const std::array<ExactHPolytopeBoundaryKind, 6> expected_box_kinds{
      ExactHPolytopeBoundaryKind::box_lower_x,
      ExactHPolytopeBoundaryKind::box_upper_x,
      ExactHPolytopeBoundaryKind::box_lower_y,
      ExactHPolytopeBoundaryKind::box_upper_y,
      ExactHPolytopeBoundaryKind::box_lower_z,
      ExactHPolytopeBoundaryKind::box_upper_z};
  check(
      cube.boundary_planes.size() == expected_box_kinds.size(),
      "the cube exposes exactly six artificial boundary planes");
  for (std::size_t index = 0U;
       index < std::min(cube.boundary_planes.size(), expected_box_kinds.size());
       ++index) {
    check(
        cube.boundary_planes[index].kind == expected_box_kinds[index] &&
            !cube.boundary_planes[index].constraint_id.has_value(),
        "box boundary " + std::to_string(index) +
            " has a stable kind and no semantic constraint id");
  }

  const HPolytopeConstraintId x_clip_id = constraint_id(
      HPolytopeConstraintDomain::generic_affine, 4U, 9U);
  const std::array<ExactHPolytopeHalfspace3, 1> x_clip{halfspace(
      x_clip_id,
      ExactHPolytopeHalfspaceRole::new_clip,
      1,
      0,
      0,
      0)};
  const auto half_cube = build_exact_bounded_h_polytope_reference(
      x_clip, clipping_box);
  check(
      half_cube.decision ==
              ExactBoundedHPolytopeReferenceDecision::complete_nonempty &&
          half_cube.affine_dimension == 3U,
      "the axis clip x <= 0 retains a full-dimensional half-cube");
  check(
      vertex_keys(half_cube) == grid_keys({-1, 0}, {-1, 1}, {-1, 1}),
      "the axis clip retains exactly the eight expected vertices");
  check(
      half_cube.classified_halfspaces.size() == 1U &&
          half_cube.classified_halfspaces.front().constraint_id == x_clip_id &&
          half_cube.classified_halfspaces.front().role ==
              ExactHPolytopeHalfspaceRole::new_clip &&
          half_cube.classified_halfspaces.front().kind ==
              ExactHPolytopeHalfspaceKind::proper_halfspace,
      "the axis clip preserves its semantic identity and role");
  check(
      half_cube.boundary_planes.size() == 7U &&
          half_cube.boundary_planes.back().kind ==
              ExactHPolytopeBoundaryKind::input_halfspace &&
          half_cube.boundary_planes.back().constraint_id == x_clip_id,
      "the proper axis clip becomes one semantic boundary");
}

void test_oblique_clip() {
  const std::array<ExactHPolytopeHalfspace3, 1> clips{halfspace(
      constraint_id(HPolytopeConstraintDomain::generic_affine, 1U, 0U),
      ExactHPolytopeHalfspaceRole::new_clip,
      1,
      1,
      1,
      0)};
  const auto result = build_exact_bounded_h_polytope_reference(
      clips, box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0));

  std::set<std::string> expected{
      point_key(-1, -1, -1),
      point_key(1, -1, -1),
      point_key(-1, 1, -1),
      point_key(-1, -1, 1),
      point_key(0, 1, -1),
      point_key(0, -1, 1),
      point_key(1, 0, -1),
      point_key(-1, 0, 1),
      point_key(1, -1, 0),
      point_key(-1, 1, 0)};
  check(
      result.decision ==
              ExactBoundedHPolytopeReferenceDecision::complete_nonempty &&
          result.affine_dimension == 3U,
      "the oblique clip x + y + z <= 0 stays full-dimensional");
  check(
      vertex_keys(result) == expected,
      "the oblique clip has its four retained corners and six edge cuts");
}

void test_composite_ids_coincident_planes_and_permutation() {
  const std::array<HPolytopeConstraintId, 4> ids{
      constraint_id(HPolytopeConstraintDomain::generic_affine, 7U, 9U),
      constraint_id(HPolytopeConstraintDomain::generic_affine, 7U, 10U),
      constraint_id(HPolytopeConstraintDomain::generic_affine, 8U, 9U),
      constraint_id(
          HPolytopeConstraintDomain::power_owner_competitor, 7U, 9U)};
  std::vector<ExactHPolytopeHalfspace3> clips{
      halfspace(ids[3], ExactHPolytopeHalfspaceRole::new_clip, 2, 0, 0, 0),
      halfspace(
          ids[1], ExactHPolytopeHalfspaceRole::parent_constraint, 1, 0, 0, 0),
      halfspace(ids[2], ExactHPolytopeHalfspaceRole::new_clip, 3, 0, 0, 0),
      halfspace(
          ids[0], ExactHPolytopeHalfspaceRole::parent_constraint, 4, 0, 0, 0)};
  const ExactDyadicAabb3 clipping_box = box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0);
  const auto first = build_exact_bounded_h_polytope_reference(
      clips, clipping_box);
  std::reverse(clips.begin(), clips.end());
  const auto reversed = build_exact_bounded_h_polytope_reference(
      clips, clipping_box);

  check(first == reversed, "input permutation does not change the exact result");
  check(
      first.classified_halfspaces.size() == ids.size() &&
          first.boundary_planes.size() == ids.size() + 6U,
      "coincident planes remain four distinct semantic constraints");
  for (std::size_t index = 0U;
       index < std::min(first.classified_halfspaces.size(), ids.size());
       ++index) {
    check(
        first.classified_halfspaces[index].constraint_id == ids[index] &&
            first.boundary_planes[index + 6U].constraint_id == ids[index],
        "domain/first/second id ordering is stable at index " +
            std::to_string(index));
  }
  if (first.classified_halfspaces.size() == ids.size()) {
    check(
        first.classified_halfspaces[0U].role ==
                ExactHPolytopeHalfspaceRole::parent_constraint &&
            first.classified_halfspaces[1U].role ==
                ExactHPolytopeHalfspaceRole::parent_constraint &&
            first.classified_halfspaces[2U].role ==
                ExactHPolytopeHalfspaceRole::new_clip &&
            first.classified_halfspaces[3U].role ==
                ExactHPolytopeHalfspaceRole::new_clip,
        "sorting semantic ids preserves each parent/new-clip role");
  }
  check(
      vertex_keys(first) == grid_keys({-1, 0}, {-1, 1}, {-1, 1}) &&
          first.audit.active_incidence_count == 36U,
      "all four coincident ids are active incidences on the clipped face");
  for (const auto& vertex : first.vertices) {
    if (!vertex.position.coordinate(0U).is_zero()) {
      continue;
    }
    for (std::size_t boundary_index = 6U; boundary_index < 10U;
         ++boundary_index) {
      check(
          std::find(
              vertex.active_boundary_plane_indices.begin(),
              vertex.active_boundary_plane_indices.end(),
              boundary_index) != vertex.active_boundary_plane_indices.end(),
          "each coincident semantic plane is active on every x=0 vertex");
    }
  }
}

void test_affine_dimensions_two_one_and_zero() {
  const ExactDyadicAabb3 clipping_box = box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0);
  const auto id = [](std::uint64_t value) {
    return constraint_id(
        HPolytopeConstraintDomain::generic_affine, value, 0U);
  };

  const std::array<ExactHPolytopeHalfspace3, 2> plane{
      halfspace(id(1U), ExactHPolytopeHalfspaceRole::parent_constraint, 0, 0, 1, 0),
      halfspace(id(2U), ExactHPolytopeHalfspaceRole::new_clip, 0, 0, -1, 0)};
  const auto plane_result = build_exact_bounded_h_polytope_reference(
      plane, clipping_box);
  check(
      plane_result.affine_dimension == 2U &&
          vertex_keys(plane_result) == grid_keys({-1, 1}, {-1, 1}, {0}),
      "opposite z halfspaces retain the exact square z=0");

  const std::array<ExactHPolytopeHalfspace3, 4> line{
      halfspace(id(1U), ExactHPolytopeHalfspaceRole::parent_constraint, 1, 0, 0, 0),
      halfspace(id(2U), ExactHPolytopeHalfspaceRole::parent_constraint, -1, 0, 0, 0),
      halfspace(id(3U), ExactHPolytopeHalfspaceRole::new_clip, 0, 1, 0, 0),
      halfspace(id(4U), ExactHPolytopeHalfspaceRole::new_clip, 0, -1, 0, 0)};
  const auto line_result = build_exact_bounded_h_polytope_reference(
      line, clipping_box);
  check(
      line_result.affine_dimension == 1U &&
          vertex_keys(line_result) == grid_keys({0}, {0}, {-1, 1}),
      "four opposite halfspaces retain the exact z-axis segment");

  const std::array<ExactHPolytopeHalfspace3, 6> point{
      halfspace(id(1U), ExactHPolytopeHalfspaceRole::parent_constraint, 1, 0, 0, 0),
      halfspace(id(2U), ExactHPolytopeHalfspaceRole::parent_constraint, -1, 0, 0, 0),
      halfspace(id(3U), ExactHPolytopeHalfspaceRole::parent_constraint, 0, 1, 0, 0),
      halfspace(id(4U), ExactHPolytopeHalfspaceRole::new_clip, 0, -1, 0, 0),
      halfspace(id(5U), ExactHPolytopeHalfspaceRole::new_clip, 0, 0, 1, 0),
      halfspace(id(6U), ExactHPolytopeHalfspaceRole::new_clip, 0, 0, -1, 0)};
  const auto point_result = build_exact_bounded_h_polytope_reference(
      point, clipping_box);
  check(
      point_result.affine_dimension == 0U &&
          vertex_keys(point_result) ==
              std::set<std::string>{point_key(0, 0, 0)},
      "six opposite halfspaces retain the exact origin");

  const std::array<ExactHPolytopeHalfspace3, 1> tangent_corner{halfspace(
      id(7U),
      ExactHPolytopeHalfspaceRole::new_clip,
      1,
      1,
      1,
      3)};
  const auto tangent_result = build_exact_bounded_h_polytope_reference(
      tangent_corner, clipping_box);
  check(
      tangent_result.decision ==
              ExactBoundedHPolytopeReferenceDecision::complete_nonempty &&
          tangent_result.affine_dimension == 0U &&
          vertex_keys(tangent_result) ==
              std::set<std::string>{point_key(-1, -1, -1)},
      "one semantic plane tangent to three box faces retains a point");
  check(
      tangent_result.audit.unique_feasible_vertex_count == 1U &&
          tangent_result.audit.exact_incidence_test_count == 7U &&
          tangent_result.audit.active_incidence_count == 4U,
      "the tangent point retains its semantic plane and three box incidences");
}

void test_proper_halfspace_empty_intersection() {
  const std::array<ExactHPolytopeHalfspace3, 1> outside{halfspace(
      constraint_id(HPolytopeConstraintDomain::generic_affine, 1U, 2U),
      ExactHPolytopeHalfspaceRole::new_clip,
      1,
      0,
      0,
      2)};
  const auto result = build_exact_bounded_h_polytope_reference(
      outside, box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0));
  check(
      result.decision ==
              ExactBoundedHPolytopeReferenceDecision::complete_empty &&
          !result.affine_dimension.has_value() && result.vertices.empty(),
      "the proper halfspace x <= -2 is empty inside the clipping box");
  check(
      result.audit.proper_halfspace_count == 1U &&
          result.audit.enumerated_plane_triple_count == 35U &&
          result.boundary_planes.size() == 7U,
      "proper geometric emptiness is established by exhaustive triples");

  const std::array<ExactHPolytopeHalfspace3, 2> incompatible{
      halfspace(
          constraint_id(HPolytopeConstraintDomain::generic_affine, 3U, 0U),
          ExactHPolytopeHalfspaceRole::parent_constraint,
          1,
          0,
          0,
          1),
      halfspace(
          constraint_id(HPolytopeConstraintDomain::generic_affine, 4U, 0U),
          ExactHPolytopeHalfspaceRole::new_clip,
          -1,
          0,
          0,
          1)};
  const auto incompatible_result = build_exact_bounded_h_polytope_reference(
      incompatible, box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0));
  check(
      incompatible_result.decision ==
              ExactBoundedHPolytopeReferenceDecision::complete_empty &&
          incompatible_result.vertices.empty() &&
          !incompatible_result.affine_dimension.has_value(),
      "the two proper constraints x <= -1 and x >= 1 are incompatible");
  check(
      incompatible_result.audit.proper_halfspace_count == 2U &&
          incompatible_result.audit.infeasible_count == 0U &&
          incompatible_result.audit.enumerated_plane_triple_count == 56U &&
          incompatible_result.boundary_planes.size() == 8U,
      "proper-halfspace contradiction is distinct from constant infeasibility");
}

void test_constant_halfspace_classification() {
  const HPolytopeConstraintId zero_id = constraint_id(
      HPolytopeConstraintDomain::generic_affine, 1U, 0U);
  const HPolytopeConstraintId positive_id = constraint_id(
      HPolytopeConstraintDomain::generic_affine, 2U, 0U);
  const HPolytopeConstraintId negative_id = constraint_id(
      HPolytopeConstraintDomain::generic_affine, 3U, 0U);
  const std::array<ExactHPolytopeHalfspace3, 3> constants{
      halfspace(
          negative_id,
          ExactHPolytopeHalfspaceRole::parent_constraint,
          0,
          0,
          0,
          -1),
      halfspace(
          zero_id, ExactHPolytopeHalfspaceRole::new_clip, 0, 0, 0, 0),
      halfspace(
          positive_id,
          ExactHPolytopeHalfspaceRole::parent_constraint,
          0,
          0,
          0,
          1)};
  const auto result = build_exact_bounded_h_polytope_reference(
      constants, box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0));

  check(
      result.decision ==
              ExactBoundedHPolytopeReferenceDecision::complete_empty &&
          result.vertices.empty() && !result.affine_dimension.has_value(),
      "a positive constant makes the retained intersection empty");
  check(
      result.audit.redundant_strict_count == 1U &&
          result.audit.identically_active_count == 1U &&
          result.audit.infeasible_count == 1U &&
          result.audit.proper_halfspace_count == 0U,
      "negative, zero and positive constants have distinct exact kinds");
  check(
      result.classified_halfspaces.size() == 3U &&
          result.classified_halfspaces[0U].constraint_id == zero_id &&
          result.classified_halfspaces[0U].kind ==
              ExactHPolytopeHalfspaceKind::identically_active &&
          result.classified_halfspaces[1U].constraint_id == positive_id &&
          result.classified_halfspaces[1U].kind ==
              ExactHPolytopeHalfspaceKind::infeasible &&
          result.classified_halfspaces[2U].constraint_id == negative_id &&
          result.classified_halfspaces[2U].kind ==
              ExactHPolytopeHalfspaceKind::redundant_strict,
      "constant classifications are canonically ordered by semantic id");
  check(
      result.boundary_planes.size() == 6U &&
          result.audit.enumerated_plane_triple_count == 0U,
      "constant forms do not create planes and infeasibility exits early");
}

void test_validation_precedes_budget() {
  ExactBoundedHPolytopeReferenceBudget zero_budget;
  zero_budget.maximum_input_halfspace_count = 0U;
  zero_budget.maximum_boundary_count = 0U;
  zero_budget.maximum_plane_triple_count = 0U;
  zero_budget.maximum_feasibility_test_count = 0U;
  zero_budget.maximum_vertex_count = 0U;
  zero_budget.maximum_incidence_test_count = 0U;

  const std::array<ExactHPolytopeHalfspace3, 0> no_halfspaces{};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_bounded_h_polytope_reference(
            no_halfspaces,
            box(1.0, -1.0, -1.0, 1.0, 1.0, 1.0),
            zero_budget));
      },
      "an invalid box is rejected before an insufficient budget");

  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_bounded_h_polytope_reference(
            no_halfspaces,
            box(-0.0, -1.0, -1.0, 1.0, 1.0, 1.0),
            zero_budget));
      },
      "a noncanonical negative-zero bound is rejected before budget preflight");

  ExactDyadicAabb3 infinite_box =
      box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0);
  infinite_box.upper_binary64_bits[2U] = word(
      std::numeric_limits<double>::infinity());
  check_throws<std::domain_error>(
      [&] {
        static_cast<void>(build_exact_bounded_h_polytope_reference(
            no_halfspaces, infinite_box, zero_budget));
      },
      "a non-finite bound is rejected before budget preflight");

  const HPolytopeConstraintId duplicate_id = constraint_id(
      HPolytopeConstraintDomain::generic_affine, 5U, 8U);
  const std::array<ExactHPolytopeHalfspace3, 2> duplicates{
      halfspace(
          duplicate_id,
          ExactHPolytopeHalfspaceRole::parent_constraint,
          1,
          0,
          0,
          0),
      halfspace(
          duplicate_id, ExactHPolytopeHalfspaceRole::new_clip, 0, 1, 0, 0)};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_bounded_h_polytope_reference(
            duplicates,
            box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0),
            zero_budget));
      },
      "duplicate composite ids are rejected before an insufficient budget");

  const std::array<ExactHPolytopeHalfspace3, 1> invalid_domain{halfspace(
      constraint_id(static_cast<HPolytopeConstraintDomain>(255), 1U, 2U),
      ExactHPolytopeHalfspaceRole::new_clip,
      1,
      0,
      0,
      0)};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_bounded_h_polytope_reference(
            invalid_domain,
            box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0),
            zero_budget));
      },
      "an invalid constraint domain is rejected before budget preflight");

  const std::array<ExactHPolytopeHalfspace3, 1> invalid_role{halfspace(
      constraint_id(HPolytopeConstraintDomain::generic_affine, 1U, 2U),
      static_cast<ExactHPolytopeHalfspaceRole>(255),
      1,
      0,
      0,
      0)};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_bounded_h_polytope_reference(
            invalid_role,
            box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0),
            zero_budget));
      },
      "an invalid halfspace role is rejected before budget preflight");
}

void test_each_budget_dimension_one_below() {
  const std::array<ExactHPolytopeHalfspace3, 1> clips{halfspace(
      constraint_id(HPolytopeConstraintDomain::generic_affine, 1U, 0U),
      ExactHPolytopeHalfspaceRole::new_clip,
      1,
      0,
      0,
      0)};
  const ExactDyadicAabb3 clipping_box = box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0);
  const auto complete = build_exact_bounded_h_polytope_reference(
      clips, clipping_box);
  const auto& requirements = complete.requirements;

  ExactBoundedHPolytopeReferenceBudget budget;
  budget.maximum_input_halfspace_count = requirements.input_halfspace_count - 1U;
  check_preflight_only(
      build_exact_bounded_h_polytope_reference(clips, clipping_box, budget),
      "input-halfspace budget one below");

  budget = {};
  budget.maximum_boundary_count = requirements.conservative_boundary_count - 1U;
  check_preflight_only(
      build_exact_bounded_h_polytope_reference(clips, clipping_box, budget),
      "boundary budget one below");

  budget = {};
  budget.maximum_plane_triple_count =
      requirements.conservative_plane_triple_count - 1U;
  check_preflight_only(
      build_exact_bounded_h_polytope_reference(clips, clipping_box, budget),
      "plane-triple budget one below");

  budget = {};
  budget.maximum_feasibility_test_count =
      requirements.conservative_feasibility_test_count - 1U;
  check_preflight_only(
      build_exact_bounded_h_polytope_reference(clips, clipping_box, budget),
      "feasibility budget one below while incidence remains sufficient");

  budget = {};
  budget.maximum_vertex_count = requirements.conservative_vertex_count - 1U;
  check_preflight_only(
      build_exact_bounded_h_polytope_reference(clips, clipping_box, budget),
      "vertex budget one below");

  budget = {};
  budget.maximum_incidence_test_count =
      requirements.conservative_incidence_test_count - 1U;
  check_preflight_only(
      build_exact_bounded_h_polytope_reference(clips, clipping_box, budget),
      "incidence budget one below while feasibility remains sufficient");
}

void test_trusted_cap_preflight_is_short() {
  std::vector<ExactHPolytopeHalfspace3> halfspaces;
  halfspaces.reserve(
      ExactBoundedHPolytopeReferenceBudget::
          trusted_maximum_input_halfspace_count +
      1U);
  for (std::size_t index = 0U;
       index < ExactBoundedHPolytopeReferenceBudget::
                   trusted_maximum_input_halfspace_count;
       ++index) {
    halfspaces.push_back(halfspace(
        constraint_id(
            HPolytopeConstraintDomain::generic_affine,
            static_cast<std::uint64_t>(index),
            0U),
        ExactHPolytopeHalfspaceRole::parent_constraint,
        0,
        0,
        0,
        -1));
  }

  ExactBoundedHPolytopeReferenceBudget short_budget;
  short_budget.maximum_plane_triple_count =
      ExactBoundedHPolytopeReferenceBudget::
          trusted_maximum_plane_triple_count -
      1U;
  const auto capped = build_exact_bounded_h_polytope_reference(
      halfspaces,
      box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0),
      short_budget);
  check_preflight_only(capped, "the 55-halfspace trusted cap");
  check(
      capped.requirements.input_halfspace_count == 55U &&
          capped.requirements.conservative_boundary_count == 61U &&
          capped.requirements.conservative_plane_triple_count == 35990U &&
          capped.requirements.conservative_feasibility_test_count == 2195390U &&
          capped.requirements.conservative_vertex_count == 35990U &&
          capped.requirements.conservative_incidence_test_count == 2195390U,
      "the cap preflight derives the documented 55/61 exact work bounds");

  halfspaces.push_back(halfspace(
      constraint_id(HPolytopeConstraintDomain::generic_affine, 55U, 0U),
      ExactHPolytopeHalfspaceRole::new_clip,
      0,
      0,
      0,
      -1));
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_bounded_h_polytope_reference(
            halfspaces,
            box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0),
            short_budget));
      },
      "a 56th semantic halfspace is rejected without geometric work");

  ExactBoundedHPolytopeReferenceBudget excessive_budget;
  excessive_budget.maximum_feasibility_test_count =
      ExactBoundedHPolytopeReferenceBudget::
          trusted_maximum_feasibility_test_count +
      1U;
  const std::array<ExactHPolytopeHalfspace3, 0> no_halfspaces{};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_bounded_h_polytope_reference(
            no_halfspaces,
            box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0),
            excessive_budget));
      },
      "a caller cannot enlarge the trusted feasibility-test cap");

  excessive_budget = {};
  excessive_budget.maximum_incidence_test_count =
      ExactBoundedHPolytopeReferenceBudget::
          trusted_maximum_incidence_test_count +
      1U;
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_bounded_h_polytope_reference(
            no_halfspaces,
            box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0),
            excessive_budget));
      },
      "a caller cannot enlarge a trusted work cap");
}

void test_enum_strings_and_invalid_values() {
  using morsehgp3d::spatial::to_string;
  check(
      to_string(HPolytopeConstraintDomain::generic_affine) ==
              "generic_affine" &&
          to_string(HPolytopeConstraintDomain::power_owner_competitor) ==
              "power_owner_competitor" &&
          to_string(HPolytopeConstraintDomain::canonical_parent_cross_pair) ==
              "canonical_parent_cross_pair" &&
          to_string(HPolytopeConstraintDomain::restricted_piece_pair) ==
              "restricted_piece_pair",
      "all constraint-domain strings are stable");
  check(
      to_string(ExactHPolytopeHalfspaceRole::parent_constraint) ==
              "parent_constraint" &&
          to_string(ExactHPolytopeHalfspaceRole::new_clip) == "new_clip",
      "both halfspace-role strings are stable");
  check(
      to_string(ExactHPolytopeHalfspaceKind::proper_halfspace) ==
              "proper_halfspace" &&
          to_string(ExactHPolytopeHalfspaceKind::redundant_strict) ==
              "redundant_strict" &&
          to_string(ExactHPolytopeHalfspaceKind::infeasible) == "infeasible" &&
          to_string(ExactHPolytopeHalfspaceKind::identically_active) ==
              "identically_active",
      "all halfspace-kind strings are stable");
  check(
      to_string(ExactHPolytopeBoundaryKind::box_lower_x) == "box_lower_x" &&
          to_string(ExactHPolytopeBoundaryKind::box_upper_x) == "box_upper_x" &&
          to_string(ExactHPolytopeBoundaryKind::box_lower_y) == "box_lower_y" &&
          to_string(ExactHPolytopeBoundaryKind::box_upper_y) == "box_upper_y" &&
          to_string(ExactHPolytopeBoundaryKind::box_lower_z) == "box_lower_z" &&
          to_string(ExactHPolytopeBoundaryKind::box_upper_z) == "box_upper_z" &&
          to_string(ExactHPolytopeBoundaryKind::input_halfspace) ==
              "input_halfspace",
      "all boundary-kind strings are stable");
  check(
      to_string(ExactBoundedHPolytopeReferenceDecision::complete_nonempty) ==
              "complete_nonempty" &&
          to_string(ExactBoundedHPolytopeReferenceDecision::complete_empty) ==
              "complete_empty" &&
          to_string(ExactBoundedHPolytopeReferenceDecision::insufficient_budget) ==
              "insufficient_budget",
      "all reference-decision strings are stable");

  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(to_string(
            static_cast<HPolytopeConstraintDomain>(255)));
      },
      "an invalid constraint domain cannot be stringified");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(to_string(
            static_cast<ExactHPolytopeHalfspaceRole>(255)));
      },
      "an invalid halfspace role cannot be stringified");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(to_string(
            static_cast<ExactHPolytopeHalfspaceKind>(255)));
      },
      "an invalid halfspace kind cannot be stringified");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(to_string(
            static_cast<ExactHPolytopeBoundaryKind>(255)));
      },
      "an invalid boundary kind cannot be stringified");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(to_string(
            static_cast<ExactBoundedHPolytopeReferenceDecision>(255)));
      },
      "an invalid reference decision cannot be stringified");
}

}  // namespace

int main() {
  test_box_cube_and_axis_clip();
  test_oblique_clip();
  test_composite_ids_coincident_planes_and_permutation();
  test_affine_dimensions_two_one_and_zero();
  test_proper_halfspace_empty_intersection();
  test_constant_halfspace_classification();
  test_validation_precedes_budget();
  test_each_budget_dimension_one_below();
  test_trusted_cap_preflight_is_short();
  test_enum_strings_and_invalid_values();

  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "all bounded H-polytope reference tests passed\n";
  return 0;
}
