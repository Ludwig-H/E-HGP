#include "morsehgp3d/spatial/h_polytope_reference.hpp"

#include "morsehgp3d/exact/binary64.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace morsehgp3d::spatial {
namespace {

using exact::AffineFormKind;
using exact::ExactAffineForm3;
using exact::ExactPlane3;
using exact::ExactRational;
using exact::ExactRational3;
using exact::ThreePlaneIntersection;
using exact::ThreePlaneIntersectionKind;

[[nodiscard]] std::size_t plane_triple_count(std::size_t plane_count) {
  if (plane_count < 3U) {
    return 0U;
  }
  return plane_count * (plane_count - 1U) * (plane_count - 2U) / 6U;
}

void validate_budget(const ExactBoundedHPolytopeReferenceBudget& budget) {
  if (budget.maximum_input_halfspace_count >
          ExactBoundedHPolytopeReferenceBudget::
              trusted_maximum_input_halfspace_count ||
      budget.maximum_boundary_count >
          ExactBoundedHPolytopeReferenceBudget::
              trusted_maximum_boundary_count ||
      budget.maximum_plane_triple_count >
          ExactBoundedHPolytopeReferenceBudget::
              trusted_maximum_plane_triple_count ||
      budget.maximum_feasibility_test_count >
          ExactBoundedHPolytopeReferenceBudget::
              trusted_maximum_feasibility_test_count ||
      budget.maximum_vertex_count >
          ExactBoundedHPolytopeReferenceBudget::
              trusted_maximum_vertex_count ||
      budget.maximum_incidence_test_count >
          ExactBoundedHPolytopeReferenceBudget::
              trusted_maximum_incidence_test_count) {
    throw std::invalid_argument(
        "a bounded H-polytope reference budget exceeds its trusted cap");
  }
}

void validate_constraint_identity(const ExactHPolytopeHalfspace3& halfspace) {
  switch (halfspace.constraint_id.domain) {
    case HPolytopeConstraintDomain::generic_affine:
    case HPolytopeConstraintDomain::power_owner_competitor:
    case HPolytopeConstraintDomain::canonical_parent_cross_pair:
    case HPolytopeConstraintDomain::restricted_piece_pair:
      break;
    default:
      throw std::invalid_argument(
          "an H-polytope constraint domain is invalid");
  }
  switch (halfspace.role) {
    case ExactHPolytopeHalfspaceRole::parent_constraint:
    case ExactHPolytopeHalfspaceRole::new_clip:
      break;
    default:
      throw std::invalid_argument(
          "an H-polytope halfspace role is invalid");
  }
}

[[nodiscard]] std::array<ExactRational, 3> validate_box_side(
    const ExactDyadicAabb3& box,
    bool lower) {
  std::array<ExactRational, 3> result{};
  for (std::size_t axis = 0U; axis < result.size(); ++axis) {
    const std::uint64_t bits = lower ? box.lower_binary64_bits[axis]
                                     : box.upper_binary64_bits[axis];
    if (exact::canonicalize_binary64_bits(bits) != bits) {
      throw std::invalid_argument(
          "a bounded H-polytope box must use canonical finite binary64 "
          "bounds");
    }
    result[axis] = ExactRational::from_binary64_bits(bits);
  }
  return result;
}

struct ValidatedClippingBox {
  std::array<ExactRational, 3> lower;
  std::array<ExactRational, 3> upper;
};

[[nodiscard]] ValidatedClippingBox validate_clipping_box(
    const ExactDyadicAabb3& box) {
  ValidatedClippingBox result{
      validate_box_side(box, true), validate_box_side(box, false)};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (result.lower[axis] >= result.upper[axis]) {
      throw std::invalid_argument(
          "a bounded H-polytope box must have positive extent on every "
          "axis");
    }
  }
  return result;
}

[[nodiscard]] ExactAffineForm3 axis_form(
    std::size_t axis,
    const ExactRational& coefficient,
    const ExactRational& offset) {
  std::array<ExactRational, 4> coefficients{};
  coefficients[axis] = coefficient;
  coefficients[3U] = offset;
  return ExactAffineForm3::from_rational_coefficients(
      std::move(coefficients));
}

[[nodiscard]] ExactHPolytopeBoundaryPlane boundary_plane(
    ExactHPolytopeBoundaryKind kind,
    std::optional<HPolytopeConstraintId> constraint_id,
    ExactAffineForm3 form) {
  return ExactHPolytopeBoundaryPlane{
      kind,
      constraint_id,
      form,
      ExactPlane3::from_affine_form(form)};
}

[[nodiscard]] std::string point_key(const ExactRational3& point) {
  return point.coordinate(0U).canonical_key() + ":" +
         point.coordinate(1U).canonical_key() + ":" +
         point.coordinate(2U).canonical_key();
}

[[nodiscard]] bool point_less(
    const ExactRational3& left,
    const ExactRational3& right) {
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (left.coordinate(axis) < right.coordinate(axis)) {
      return true;
    }
    if (left.coordinate(axis) > right.coordinate(axis)) {
      return false;
    }
  }
  return false;
}

[[nodiscard]] bool is_feasible(
    const ExactRational3& point,
    std::span<const ExactHPolytopeBoundaryPlane> boundaries,
    std::size_t& exact_test_count) {
  for (const ExactHPolytopeBoundaryPlane& boundary : boundaries) {
    ++exact_test_count;
    if (boundary.retained_nonpositive_form.evaluate(point).sign() > 0) {
      return false;
    }
  }
  return true;
}

using ExactVector3 = std::array<ExactRational, 3>;

[[nodiscard]] ExactVector3 subtract(
    const ExactRational3& left,
    const ExactRational3& right) {
  return {
      left.coordinate(0U) - right.coordinate(0U),
      left.coordinate(1U) - right.coordinate(1U),
      left.coordinate(2U) - right.coordinate(2U)};
}

[[nodiscard]] ExactVector3 cross(
    const ExactVector3& left,
    const ExactVector3& right) {
  return {
      left[1U] * right[2U] - left[2U] * right[1U],
      left[2U] * right[0U] - left[0U] * right[2U],
      left[0U] * right[1U] - left[1U] * right[0U]};
}

[[nodiscard]] ExactRational dot(
    const ExactVector3& left,
    const ExactVector3& right) {
  return left[0U] * right[0U] + left[1U] * right[1U] +
         left[2U] * right[2U];
}

[[nodiscard]] bool is_zero(const ExactVector3& vector) {
  return vector[0U].is_zero() && vector[1U].is_zero() &&
         vector[2U].is_zero();
}

[[nodiscard]] std::size_t affine_dimension(
    std::span<const ExactHPolytopeVertex> vertices) {
  if (vertices.empty()) {
    throw std::invalid_argument(
        "an empty vertex set has no affine dimension");
  }
  const ExactRational3& base = vertices.front().position;
  std::optional<ExactVector3> first_direction;
  for (std::size_t index = 1U; index < vertices.size(); ++index) {
    ExactVector3 direction = subtract(vertices[index].position, base);
    if (!is_zero(direction)) {
      first_direction = std::move(direction);
      break;
    }
  }
  if (!first_direction.has_value()) {
    return 0U;
  }

  std::optional<ExactVector3> spanning_normal;
  for (std::size_t index = 1U; index < vertices.size(); ++index) {
    ExactVector3 normal =
        cross(*first_direction, subtract(vertices[index].position, base));
    if (!is_zero(normal)) {
      spanning_normal = std::move(normal);
      break;
    }
  }
  if (!spanning_normal.has_value()) {
    return 1U;
  }
  for (std::size_t index = 1U; index < vertices.size(); ++index) {
    if (!dot(
             *spanning_normal,
             subtract(vertices[index].position, base))
             .is_zero()) {
      return 3U;
    }
  }
  return 2U;
}

[[nodiscard]] ExactBoundedHPolytopeReferenceRequirements requirements_for(
    std::size_t input_halfspace_count) {
  const std::size_t boundary_count = input_halfspace_count + 6U;
  const std::size_t triple_count = plane_triple_count(boundary_count);
  return ExactBoundedHPolytopeReferenceRequirements{
      input_halfspace_count,
      boundary_count,
      triple_count,
      triple_count * boundary_count,
      triple_count,
      triple_count * boundary_count};
}

[[nodiscard]] bool budget_covers(
    const ExactBoundedHPolytopeReferenceBudget& budget,
    const ExactBoundedHPolytopeReferenceRequirements& requirements) {
  return budget.maximum_input_halfspace_count >=
             requirements.input_halfspace_count &&
         budget.maximum_boundary_count >=
             requirements.conservative_boundary_count &&
         budget.maximum_plane_triple_count >=
             requirements.conservative_plane_triple_count &&
         budget.maximum_feasibility_test_count >=
             requirements.conservative_feasibility_test_count &&
         budget.maximum_vertex_count >=
             requirements.conservative_vertex_count &&
         budget.maximum_incidence_test_count >=
             requirements.conservative_incidence_test_count;
}

void validate_completed_result(
    const ExactBoundedHPolytopeReferenceResult& result) {
  const std::size_t classified_count =
      result.audit.proper_halfspace_count +
      result.audit.redundant_strict_count + result.audit.infeasible_count +
      result.audit.identically_active_count;
  if (result.classified_halfspaces.size() !=
          result.requirements.input_halfspace_count ||
      classified_count != result.classified_halfspaces.size() ||
      result.boundary_planes.size() >
          result.requirements.conservative_boundary_count ||
      result.audit.enumerated_plane_triple_count >
          result.requirements.conservative_plane_triple_count ||
      result.audit.exact_feasibility_test_count >
          result.requirements.conservative_feasibility_test_count ||
      result.vertices.size() >
          result.requirements.conservative_vertex_count ||
      result.audit.unique_feasible_vertex_count != result.vertices.size() ||
      result.audit.exact_incidence_test_count >
          result.requirements.conservative_incidence_test_count ||
      result.audit.exact_incidence_test_count !=
          result.vertices.size() * result.boundary_planes.size() ||
      result.audit.active_incidence_count >
          result.audit.exact_incidence_test_count ||
      (result.decision ==
           ExactBoundedHPolytopeReferenceDecision::complete_nonempty) !=
          result.affine_dimension.has_value() ||
      (result.affine_dimension.has_value() &&
       *result.affine_dimension > 3U) ||
      (result.decision ==
           ExactBoundedHPolytopeReferenceDecision::complete_empty &&
       !result.vertices.empty())) {
    throw std::logic_error(
        "a completed bounded H-polytope exceeded its preflight contract");
  }
}

}  // namespace

std::string_view to_string(HPolytopeConstraintDomain domain) {
  switch (domain) {
    case HPolytopeConstraintDomain::generic_affine:
      return "generic_affine";
    case HPolytopeConstraintDomain::power_owner_competitor:
      return "power_owner_competitor";
    case HPolytopeConstraintDomain::canonical_parent_cross_pair:
      return "canonical_parent_cross_pair";
    case HPolytopeConstraintDomain::restricted_piece_pair:
      return "restricted_piece_pair";
  }
  throw std::invalid_argument("an H-polytope constraint domain is invalid");
}

std::string_view to_string(ExactHPolytopeHalfspaceRole role) {
  switch (role) {
    case ExactHPolytopeHalfspaceRole::parent_constraint:
      return "parent_constraint";
    case ExactHPolytopeHalfspaceRole::new_clip:
      return "new_clip";
  }
  throw std::invalid_argument("an H-polytope halfspace role is invalid");
}

std::string_view to_string(ExactHPolytopeHalfspaceKind kind) {
  switch (kind) {
    case ExactHPolytopeHalfspaceKind::proper_halfspace:
      return "proper_halfspace";
    case ExactHPolytopeHalfspaceKind::redundant_strict:
      return "redundant_strict";
    case ExactHPolytopeHalfspaceKind::infeasible:
      return "infeasible";
    case ExactHPolytopeHalfspaceKind::identically_active:
      return "identically_active";
  }
  throw std::invalid_argument("an H-polytope halfspace kind is invalid");
}

std::string_view to_string(ExactHPolytopeBoundaryKind kind) {
  switch (kind) {
    case ExactHPolytopeBoundaryKind::box_lower_x:
      return "box_lower_x";
    case ExactHPolytopeBoundaryKind::box_upper_x:
      return "box_upper_x";
    case ExactHPolytopeBoundaryKind::box_lower_y:
      return "box_lower_y";
    case ExactHPolytopeBoundaryKind::box_upper_y:
      return "box_upper_y";
    case ExactHPolytopeBoundaryKind::box_lower_z:
      return "box_lower_z";
    case ExactHPolytopeBoundaryKind::box_upper_z:
      return "box_upper_z";
    case ExactHPolytopeBoundaryKind::input_halfspace:
      return "input_halfspace";
  }
  throw std::invalid_argument("an H-polytope boundary kind is invalid");
}

std::string_view to_string(
    ExactBoundedHPolytopeReferenceDecision decision) {
  switch (decision) {
    case ExactBoundedHPolytopeReferenceDecision::complete_nonempty:
      return "complete_nonempty";
    case ExactBoundedHPolytopeReferenceDecision::complete_empty:
      return "complete_empty";
    case ExactBoundedHPolytopeReferenceDecision::insufficient_budget:
      return "insufficient_budget";
  }
  throw std::invalid_argument(
      "a bounded H-polytope reference decision is invalid");
}

ExactBoundedHPolytopeReferenceResult
build_exact_bounded_h_polytope_reference(
    std::span<const ExactHPolytopeHalfspace3> halfspaces,
    const ExactDyadicAabb3& clipping_box,
    ExactBoundedHPolytopeReferenceBudget budget) {
  validate_budget(budget);
  if (halfspaces.size() >
      ExactBoundedHPolytopeReferenceBudget::
          trusted_maximum_input_halfspace_count) {
    throw std::invalid_argument(
        "the bounded H-polytope reference accepts at most 55 input "
        "halfspaces");
  }
  const ValidatedClippingBox validated_box =
      validate_clipping_box(clipping_box);

  std::vector<const ExactHPolytopeHalfspace3*> ordered_halfspaces;
  ordered_halfspaces.reserve(halfspaces.size());
  for (const ExactHPolytopeHalfspace3& halfspace : halfspaces) {
    validate_constraint_identity(halfspace);
    ordered_halfspaces.push_back(&halfspace);
  }
  std::sort(
      ordered_halfspaces.begin(),
      ordered_halfspaces.end(),
      [](const ExactHPolytopeHalfspace3* left,
         const ExactHPolytopeHalfspace3* right) {
        return left->constraint_id < right->constraint_id;
      });
  for (std::size_t index = 1U; index < ordered_halfspaces.size(); ++index) {
    if (ordered_halfspaces[index - 1U]->constraint_id ==
        ordered_halfspaces[index]->constraint_id) {
      throw std::invalid_argument(
          "bounded H-polytope constraint ids must be unique");
    }
  }

  ExactBoundedHPolytopeReferenceResult result;
  result.requirements = requirements_for(halfspaces.size());
  if (!budget_covers(budget, result.requirements)) {
    return result;
  }

  result.classified_halfspaces.reserve(halfspaces.size());
  result.boundary_planes.reserve(halfspaces.size() + 6U);
  const std::array<ExactHPolytopeBoundaryKind, 3> lower_kinds{
      ExactHPolytopeBoundaryKind::box_lower_x,
      ExactHPolytopeBoundaryKind::box_lower_y,
      ExactHPolytopeBoundaryKind::box_lower_z};
  const std::array<ExactHPolytopeBoundaryKind, 3> upper_kinds{
      ExactHPolytopeBoundaryKind::box_upper_x,
      ExactHPolytopeBoundaryKind::box_upper_y,
      ExactHPolytopeBoundaryKind::box_upper_z};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result.boundary_planes.push_back(boundary_plane(
        lower_kinds[axis],
        std::nullopt,
        axis_form(
            axis,
            ExactRational{exact::BigInt{-1}},
            validated_box.lower[axis])));
    result.boundary_planes.push_back(boundary_plane(
        upper_kinds[axis],
        std::nullopt,
        axis_form(
            axis,
            ExactRational{exact::BigInt{1}},
            -validated_box.upper[axis])));
  }

  bool intersection_is_empty = false;
  for (const ExactHPolytopeHalfspace3* halfspace : ordered_halfspaces) {
    const exact::AffineFormClassification classification =
        exact::classify_affine_form(halfspace->retained_nonpositive_form);
    ExactHPolytopeHalfspaceKind kind =
        ExactHPolytopeHalfspaceKind::identically_active;
    switch (classification.kind()) {
      case AffineFormKind::proper_plane:
        kind = ExactHPolytopeHalfspaceKind::proper_halfspace;
        ++result.audit.proper_halfspace_count;
        result.boundary_planes.push_back(boundary_plane(
            ExactHPolytopeBoundaryKind::input_halfspace,
            halfspace->constraint_id,
            halfspace->retained_nonpositive_form));
        break;
      case AffineFormKind::constant_negative:
        kind = ExactHPolytopeHalfspaceKind::redundant_strict;
        ++result.audit.redundant_strict_count;
        break;
      case AffineFormKind::constant_positive:
        kind = ExactHPolytopeHalfspaceKind::infeasible;
        ++result.audit.infeasible_count;
        intersection_is_empty = true;
        break;
      case AffineFormKind::identically_zero:
        kind = ExactHPolytopeHalfspaceKind::identically_active;
        ++result.audit.identically_active_count;
        break;
    }
    result.classified_halfspaces.push_back(
        ExactClassifiedHPolytopeHalfspace3{
            halfspace->constraint_id,
            halfspace->role,
            kind,
            halfspace->retained_nonpositive_form,
            classification.plane()});
  }
  if (intersection_is_empty) {
    result.decision =
        ExactBoundedHPolytopeReferenceDecision::complete_empty;
    validate_completed_result(result);
    return result;
  }

  std::map<std::string, ExactRational3> feasible_points;
  const std::size_t boundary_count = result.boundary_planes.size();
  for (std::size_t first_index = 0U; first_index < boundary_count;
       ++first_index) {
    for (std::size_t second_index = first_index + 1U;
         second_index < boundary_count;
         ++second_index) {
      for (std::size_t third_index = second_index + 1U;
           third_index < boundary_count;
           ++third_index) {
        ++result.audit.enumerated_plane_triple_count;
        const ThreePlaneIntersection intersection =
            exact::intersect_three_planes(
                result.boundary_planes[first_index].plane,
                result.boundary_planes[second_index].plane,
                result.boundary_planes[third_index].plane);
        if (intersection.kind() != ThreePlaneIntersectionKind::unique ||
            !is_feasible(
                *intersection.point(),
                result.boundary_planes,
                result.audit.exact_feasibility_test_count)) {
          continue;
        }
        feasible_points.emplace(
            point_key(*intersection.point()), *intersection.point());
      }
    }
  }

  result.vertices.reserve(feasible_points.size());
  for (auto& [key, point] : feasible_points) {
    static_cast<void>(key);
    result.vertices.push_back(ExactHPolytopeVertex{std::move(point), {}});
  }
  std::sort(
      result.vertices.begin(),
      result.vertices.end(),
      [](const ExactHPolytopeVertex& left,
         const ExactHPolytopeVertex& right) {
        return point_less(left.position, right.position);
      });
  for (ExactHPolytopeVertex& vertex : result.vertices) {
    vertex.active_boundary_plane_indices.reserve(boundary_count);
    for (std::size_t boundary_index = 0U; boundary_index < boundary_count;
         ++boundary_index) {
      ++result.audit.exact_incidence_test_count;
      if (result.boundary_planes[boundary_index]
              .retained_nonpositive_form.evaluate(vertex.position)
              .is_zero()) {
        vertex.active_boundary_plane_indices.push_back(boundary_index);
      }
    }
    result.audit.active_incidence_count +=
        vertex.active_boundary_plane_indices.size();
  }
  result.audit.unique_feasible_vertex_count = result.vertices.size();
  result.decision =
      result.vertices.empty()
          ? ExactBoundedHPolytopeReferenceDecision::complete_empty
          : ExactBoundedHPolytopeReferenceDecision::complete_nonempty;
  if (!result.vertices.empty()) {
    result.affine_dimension = affine_dimension(result.vertices);
  }
  validate_completed_result(result);
  return result;
}

}  // namespace morsehgp3d::spatial
