#include "morsehgp3d/spatial/power_cell_reference.hpp"

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

void validate_budget(const ExactPowerCellReferenceBudget& budget) {
  if (budget.maximum_site_count >
          ExactPowerCellReferenceBudget::trusted_maximum_site_count ||
      budget.maximum_constraint_count >
          ExactPowerCellReferenceBudget::trusted_maximum_constraint_count ||
      budget.maximum_plane_triple_count >
          ExactPowerCellReferenceBudget::trusted_maximum_plane_triple_count ||
      budget.maximum_vertex_count >
          ExactPowerCellReferenceBudget::trusted_maximum_vertex_count ||
      budget.maximum_incidence_count >
          ExactPowerCellReferenceBudget::trusted_maximum_incidence_count) {
    throw std::invalid_argument(
        "a Phase 7 power-cell reference budget exceeds its trusted cap");
  }
}

[[nodiscard]] std::array<ExactRational, 3> validate_box(
    const ExactDyadicAabb3& box,
    bool lower) {
  std::array<ExactRational, 3> result{};
  for (std::size_t axis = 0U; axis < result.size(); ++axis) {
    const std::uint64_t bits = lower ? box.lower_binary64_bits[axis]
                                     : box.upper_binary64_bits[axis];
    if (exact::canonicalize_binary64_bits(bits) != bits) {
      throw std::invalid_argument(
          "a Phase 7 clipping box must use canonical finite binary64 bounds");
    }
    result[axis] = ExactRational::from_binary64_bits(bits);
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

[[nodiscard]] ExactPowerCellBoundaryPlane boundary_plane(
    PowerCellBoundaryKind kind,
    std::optional<PointId> competitor_id,
    ExactAffineForm3 form) {
  return ExactPowerCellBoundaryPlane{
      kind,
      competitor_id,
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
    std::span<const ExactPowerCellBoundaryPlane> boundaries) {
  return std::all_of(
      boundaries.begin(),
      boundaries.end(),
      [&point](const ExactPowerCellBoundaryPlane& boundary) {
        return boundary.owner_halfspace_form.evaluate(point).sign() <= 0;
      });
}

[[nodiscard]] ExactPowerCellReferenceRequirements requirements_for(
    std::size_t competitor_count) {
  const std::size_t site_count = competitor_count + 1U;
  const std::size_t constraint_count = competitor_count + 6U;
  const std::size_t triple_count = plane_triple_count(constraint_count);
  return ExactPowerCellReferenceRequirements{
      site_count,
      constraint_count,
      triple_count,
      triple_count,
      triple_count * constraint_count};
}

[[nodiscard]] bool budget_covers(
    const ExactPowerCellReferenceBudget& budget,
    const ExactPowerCellReferenceRequirements& requirements) {
  return budget.maximum_site_count >= requirements.site_count &&
         budget.maximum_constraint_count >=
             requirements.conservative_constraint_count &&
         budget.maximum_plane_triple_count >=
             requirements.conservative_plane_triple_count &&
         budget.maximum_vertex_count >=
             requirements.conservative_vertex_count &&
         budget.maximum_incidence_count >=
             requirements.conservative_incidence_count;
}

void validate_completed_result(const ExactPowerCellReferenceResult& result) {
  const std::size_t classified_constraint_count =
      result.audit.proper_bisector_count +
      result.audit.redundant_constraint_count +
      result.audit.infeasible_constraint_count;
  if (result.pairwise_constraints.size() + 1U !=
          result.requirements.site_count ||
      classified_constraint_count != result.pairwise_constraints.size() ||
      result.boundary_planes.size() >
          result.requirements.conservative_constraint_count ||
      result.audit.enumerated_plane_triple_count >
          result.requirements.conservative_plane_triple_count ||
      result.vertices.size() >
          result.requirements.conservative_vertex_count ||
      result.audit.unique_feasible_vertex_count != result.vertices.size() ||
      result.audit.active_incidence_count >
          result.requirements.conservative_incidence_count) {
    throw std::logic_error(
        "a completed Phase 7 reference cell exceeded its preflight contract");
  }
}

}  // namespace

Binary64WeightedSite3 Binary64WeightedSite3::from_binary64_bits(
    PointId point_id,
    std::array<std::uint64_t, 3> position_bits,
    std::uint64_t weight_bits) {
  if (point_id > CanonicalPointCloud::max_point_id) {
    throw std::invalid_argument(
        "a weighted-site point id exceeds the exact binary64 id domain");
  }
  for (std::uint64_t& bits : position_bits) {
    bits = exact::canonicalize_binary64_bits(bits);
  }
  weight_bits = exact::canonicalize_binary64_bits(weight_bits);
  return Binary64WeightedSite3{
      point_id,
      exact::CertifiedPoint3::from_binary64_bits(position_bits),
      weight_bits,
      ExactRational::from_binary64_bits(weight_bits)};
}

std::string_view to_string(PowerBisectorConstraintKind kind) {
  switch (kind) {
    case PowerBisectorConstraintKind::proper_halfspace:
      return "proper_halfspace";
    case PowerBisectorConstraintKind::owner_dominates:
      return "owner_dominates";
    case PowerBisectorConstraintKind::competitor_dominates:
      return "competitor_dominates";
    case PowerBisectorConstraintKind::coincident_tie:
      return "coincident_tie";
  }
  throw std::invalid_argument("a power-bisector constraint kind is invalid");
}

ExactRational exact_power_distance(
    const Binary64WeightedSite3& site,
    const ExactRational3& query) {
  ExactRational result = -site.exact_weight();
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const ExactRational delta =
        query.coordinate(axis) - site.position().coordinate(axis);
    result = result + delta * delta;
  }
  return result;
}

ExactAffineForm3 exact_power_difference_affine_form(
    const Binary64WeightedSite3& owner,
    const Binary64WeightedSite3& competitor) {
  std::array<ExactRational, 4> coefficients{};
  ExactRational offset = competitor.exact_weight() - owner.exact_weight();
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const ExactRational owner_coordinate = owner.position().coordinate(axis);
    const ExactRational competitor_coordinate =
        competitor.position().coordinate(axis);
    coefficients[axis] = ExactRational{exact::BigInt{2}} *
                         (competitor_coordinate - owner_coordinate);
    offset = offset + owner_coordinate * owner_coordinate -
             competitor_coordinate * competitor_coordinate;
  }
  coefficients[3U] = std::move(offset);
  return ExactAffineForm3::from_rational_coefficients(
      std::move(coefficients));
}

ExactPowerBisectorConstraint make_exact_power_bisector_constraint(
    const Binary64WeightedSite3& owner,
    const Binary64WeightedSite3& competitor) {
  if (owner.point_id() == competitor.point_id()) {
    throw std::invalid_argument(
        "a power-bisector owner and competitor must have distinct ids");
  }
  ExactAffineForm3 form =
      exact_power_difference_affine_form(owner, competitor);
  const exact::AffineFormClassification classification =
      exact::classify_affine_form(form);
  PowerBisectorConstraintKind kind =
      PowerBisectorConstraintKind::coincident_tie;
  switch (classification.kind()) {
    case AffineFormKind::proper_plane:
      kind = PowerBisectorConstraintKind::proper_halfspace;
      break;
    case AffineFormKind::constant_negative:
      kind = PowerBisectorConstraintKind::owner_dominates;
      break;
    case AffineFormKind::constant_positive:
      kind = PowerBisectorConstraintKind::competitor_dominates;
      break;
    case AffineFormKind::identically_zero:
      kind = PowerBisectorConstraintKind::coincident_tie;
      break;
  }
  return ExactPowerBisectorConstraint{
      owner.point_id(),
      competitor.point_id(),
      kind,
      std::move(form),
      classification.plane()};
}

std::string_view to_string(PowerCellBoundaryKind kind) {
  switch (kind) {
    case PowerCellBoundaryKind::box_lower_x:
      return "box_lower_x";
    case PowerCellBoundaryKind::box_upper_x:
      return "box_upper_x";
    case PowerCellBoundaryKind::box_lower_y:
      return "box_lower_y";
    case PowerCellBoundaryKind::box_upper_y:
      return "box_upper_y";
    case PowerCellBoundaryKind::box_lower_z:
      return "box_lower_z";
    case PowerCellBoundaryKind::box_upper_z:
      return "box_upper_z";
    case PowerCellBoundaryKind::power_bisector:
      return "power_bisector";
  }
  throw std::invalid_argument("a power-cell boundary kind is invalid");
}

std::string_view to_string(ExactPowerCellReferenceDecision decision) {
  switch (decision) {
    case ExactPowerCellReferenceDecision::complete_nonempty:
      return "complete_nonempty";
    case ExactPowerCellReferenceDecision::complete_empty:
      return "complete_empty";
    case ExactPowerCellReferenceDecision::insufficient_budget:
      return "insufficient_budget";
  }
  throw std::invalid_argument("a power-cell reference decision is invalid");
}

ExactPowerCellReferenceResult build_exact_bounded_power_cell_reference(
    const Binary64WeightedSite3& owner,
    std::span<const Binary64WeightedSite3> competitors,
    const ExactDyadicAabb3& clipping_box,
    ExactPowerCellReferenceBudget budget) {
  validate_budget(budget);
  if (competitors.size() >
      ExactPowerCellReferenceBudget::trusted_maximum_site_count - 1U) {
    throw std::invalid_argument(
        "the bounded Phase 7 reference oracle accepts at most eight sites");
  }

  const std::array<ExactRational, 3> lower =
      validate_box(clipping_box, true);
  const std::array<ExactRational, 3> upper =
      validate_box(clipping_box, false);
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (lower[axis] >= upper[axis]) {
      throw std::invalid_argument(
          "the Phase 7 clipping box must have positive extent on every axis");
    }
  }

  std::vector<const Binary64WeightedSite3*> ordered_competitors;
  ordered_competitors.reserve(competitors.size());
  for (const Binary64WeightedSite3& competitor : competitors) {
    ordered_competitors.push_back(&competitor);
  }
  std::sort(
      ordered_competitors.begin(),
      ordered_competitors.end(),
      [](const Binary64WeightedSite3* left,
         const Binary64WeightedSite3* right) {
        return left->point_id() < right->point_id();
      });
  PointId previous_id = owner.point_id();
  bool first = true;
  for (const Binary64WeightedSite3* competitor : ordered_competitors) {
    if (competitor->point_id() == owner.point_id() ||
        (!first && competitor->point_id() == previous_id)) {
      throw std::invalid_argument(
          "power-cell site ids must be pairwise distinct");
    }
    previous_id = competitor->point_id();
    first = false;
  }

  ExactPowerCellReferenceResult result;
  result.requirements = requirements_for(competitors.size());
  if (!budget_covers(budget, result.requirements)) {
    return result;
  }

  result.pairwise_constraints.reserve(competitors.size());
  result.boundary_planes.reserve(competitors.size() + 6U);
  const std::array<PowerCellBoundaryKind, 3> lower_kinds{
      PowerCellBoundaryKind::box_lower_x,
      PowerCellBoundaryKind::box_lower_y,
      PowerCellBoundaryKind::box_lower_z};
  const std::array<PowerCellBoundaryKind, 3> upper_kinds{
      PowerCellBoundaryKind::box_upper_x,
      PowerCellBoundaryKind::box_upper_y,
      PowerCellBoundaryKind::box_upper_z};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result.boundary_planes.push_back(boundary_plane(
        lower_kinds[axis],
        std::nullopt,
        axis_form(
            axis,
            ExactRational{exact::BigInt{-1}},
            lower[axis])));
    result.boundary_planes.push_back(boundary_plane(
        upper_kinds[axis],
        std::nullopt,
        axis_form(
            axis,
            ExactRational{exact::BigInt{1}},
            -upper[axis])));
  }

  bool cell_is_empty = false;
  for (const Binary64WeightedSite3* competitor : ordered_competitors) {
    ExactPowerBisectorConstraint constraint =
        make_exact_power_bisector_constraint(owner, *competitor);
    switch (constraint.kind) {
      case PowerBisectorConstraintKind::proper_halfspace:
        ++result.audit.proper_bisector_count;
        result.boundary_planes.push_back(boundary_plane(
            PowerCellBoundaryKind::power_bisector,
            competitor->point_id(),
            constraint.owner_minus_competitor));
        break;
      case PowerBisectorConstraintKind::owner_dominates:
        ++result.audit.redundant_constraint_count;
        break;
      case PowerBisectorConstraintKind::coincident_tie:
        ++result.audit.redundant_constraint_count;
        result.audit.has_coincident_tie = true;
        break;
      case PowerBisectorConstraintKind::competitor_dominates:
        ++result.audit.infeasible_constraint_count;
        cell_is_empty = true;
        break;
    }
    result.pairwise_constraints.push_back(std::move(constraint));
  }

  if (cell_is_empty) {
    result.decision = ExactPowerCellReferenceDecision::complete_empty;
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
        const ThreePlaneIntersection intersection = exact::intersect_three_planes(
            result.boundary_planes[first_index].plane,
            result.boundary_planes[second_index].plane,
            result.boundary_planes[third_index].plane);
        if (intersection.kind() != ThreePlaneIntersectionKind::unique ||
            !is_feasible(*intersection.point(), result.boundary_planes)) {
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
    result.vertices.push_back(ExactPowerCellVertex{std::move(point), {}});
  }
  std::sort(
      result.vertices.begin(),
      result.vertices.end(),
      [](const ExactPowerCellVertex& left, const ExactPowerCellVertex& right) {
        return point_less(left.position, right.position);
      });
  for (ExactPowerCellVertex& vertex : result.vertices) {
    vertex.active_boundary_plane_indices.reserve(boundary_count);
    for (std::size_t boundary_index = 0U; boundary_index < boundary_count;
         ++boundary_index) {
      if (result.boundary_planes[boundary_index]
              .owner_halfspace_form.evaluate(vertex.position)
              .is_zero()) {
        vertex.active_boundary_plane_indices.push_back(boundary_index);
      }
    }
    result.audit.active_incidence_count +=
        vertex.active_boundary_plane_indices.size();
  }
  result.audit.unique_feasible_vertex_count = result.vertices.size();
  result.decision = result.vertices.empty()
                        ? ExactPowerCellReferenceDecision::complete_empty
                        : ExactPowerCellReferenceDecision::complete_nonempty;
  validate_completed_result(result);
  return result;
}

}  // namespace morsehgp3d::spatial
