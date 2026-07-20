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

struct ValidatedClippingBox {
  std::array<ExactRational, 3> lower;
  std::array<ExactRational, 3> upper;
};

[[nodiscard]] ValidatedClippingBox validate_clipping_box(
    const ExactDyadicAabb3& box) {
  ValidatedClippingBox result{
      validate_box(box, true), validate_box(box, false)};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (result.lower[axis] >= result.upper[axis]) {
      throw std::invalid_argument(
          "the Phase 7 clipping box must have positive extent on every axis");
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

void validate_subset_closure_budget(
    const ExactPowerCellSubsetClosureBudget& budget) {
  validate_budget(budget.candidate_cell_budget);
  if (budget.maximum_omitted_vertex_test_count >
      ExactPowerCellSubsetClosureBudget::
          trusted_maximum_omitted_vertex_test_count) {
    throw std::invalid_argument(
        "a Phase 7 subset-closure scan budget exceeds its trusted cap");
  }
}

[[nodiscard]] ExactPowerCellSubsetClosureRequirements
subset_closure_requirements_for(
    std::size_t complete_competitor_count,
    std::size_t candidate_competitor_count) {
  const std::size_t omitted_competitor_count =
      complete_competitor_count - candidate_competitor_count;
  ExactPowerCellSubsetClosureRequirements result{
      complete_competitor_count + 1U,
      candidate_competitor_count,
      omitted_competitor_count,
      requirements_for(candidate_competitor_count),
      0U};
  result.conservative_omitted_vertex_test_count =
      omitted_competitor_count *
      result.candidate_cell_requirements.conservative_vertex_count;
  if (result.conservative_omitted_vertex_test_count >
      ExactPowerCellSubsetClosureBudget::
          trusted_maximum_omitted_vertex_test_count) {
    throw std::logic_error(
        "the Phase 7 subset-closure scan requirement exceeded its proof cap");
  }
  return result;
}

struct ValidatedPowerCellSubsetInput {
  std::map<PointId, const Binary64WeightedSite3*> complete_by_id;
  std::vector<PointId> canonical_candidate_competitor_ids;
  ExactPowerCellSubsetClosureRequirements requirements;
};

[[nodiscard]] ValidatedPowerCellSubsetInput validate_power_cell_subset_input(
    const Binary64WeightedSite3& owner,
    std::span<const Binary64WeightedSite3> complete_competitors,
    const ExactDyadicAabb3& clipping_box,
    std::span<const PointId> candidate_competitor_ids) {
  if (complete_competitors.size() >
      ExactPowerCellReferenceBudget::trusted_maximum_site_count - 1U) {
    throw std::invalid_argument(
        "the bounded Phase 7 subset closure accepts at most eight sites");
  }
  static_cast<void>(validate_clipping_box(clipping_box));

  ValidatedPowerCellSubsetInput result;
  for (const Binary64WeightedSite3& competitor : complete_competitors) {
    if (competitor.point_id() == owner.point_id() ||
        !result.complete_by_id
             .emplace(competitor.point_id(), &competitor)
             .second) {
      throw std::invalid_argument(
          "power-cell subset-closure site ids must be pairwise distinct");
    }
  }
  if (candidate_competitor_ids.size() > complete_competitors.size()) {
    throw std::invalid_argument(
        "a power-cell candidate list cannot exceed the complete competitor "
        "table");
  }

  result.canonical_candidate_competitor_ids.assign(
      candidate_competitor_ids.begin(), candidate_competitor_ids.end());
  std::sort(
      result.canonical_candidate_competitor_ids.begin(),
      result.canonical_candidate_competitor_ids.end());
  if (std::adjacent_find(
          result.canonical_candidate_competitor_ids.begin(),
          result.canonical_candidate_competitor_ids.end()) !=
      result.canonical_candidate_competitor_ids.end()) {
    throw std::invalid_argument(
        "power-cell candidate competitor ids must be unique");
  }
  for (const PointId candidate_id :
       result.canonical_candidate_competitor_ids) {
    if (!result.complete_by_id.contains(candidate_id)) {
      throw std::invalid_argument(
          "every power-cell candidate id must name an authentic competitor");
    }
  }
  result.requirements = subset_closure_requirements_for(
      complete_competitors.size(),
      result.canonical_candidate_competitor_ids.size());
  return result;
}

[[nodiscard]] bool subset_closure_budget_covers(
    const ExactPowerCellSubsetClosureBudget& budget,
    const ExactPowerCellSubsetClosureRequirements& requirements) {
  return budget_covers(
             budget.candidate_cell_budget,
             requirements.candidate_cell_requirements) &&
         budget.maximum_omitted_vertex_test_count >=
             requirements.conservative_omitted_vertex_test_count;
}

[[nodiscard]] ExactPowerCellSubsetRepairRequirements
subset_repair_requirements_for(
    const ExactPowerCellSubsetClosureRequirements& closure_requirements) {
  ExactPowerCellSubsetRepairRequirements result;
  result.subset_closure_requirements = closure_requirements;
  result.conservative_exact_cell_construction_count = 1U;
  result.conservative_omitted_scan_pass_count =
      closure_requirements.omitted_competitor_count == 0U ? 0U : 1U;
  result.conservative_cumulative_plane_triple_count =
      closure_requirements.candidate_cell_requirements
          .conservative_plane_triple_count;
  result.conservative_cumulative_vertex_count =
      closure_requirements.candidate_cell_requirements
          .conservative_vertex_count;
  result.conservative_cumulative_incidence_count =
      closure_requirements.candidate_cell_requirements
          .conservative_incidence_count;
  if (closure_requirements.omitted_competitor_count != 0U) {
    result.conservative_repaired_cell_requirements = requirements_for(
        closure_requirements.complete_site_count - 1U);
    ++result.conservative_exact_cell_construction_count;
    result.conservative_cumulative_plane_triple_count +=
        result.conservative_repaired_cell_requirements
            ->conservative_plane_triple_count;
    result.conservative_cumulative_vertex_count +=
        result.conservative_repaired_cell_requirements
            ->conservative_vertex_count;
    result.conservative_cumulative_incidence_count +=
        result.conservative_repaired_cell_requirements
            ->conservative_incidence_count;
  }
  if (result.conservative_exact_cell_construction_count >
      ExactPowerCellSubsetRepairBudget::
              trusted_maximum_exact_cell_construction_count ||
      result.conservative_cumulative_plane_triple_count >
          ExactPowerCellSubsetRepairBudget::
              trusted_maximum_cumulative_plane_triple_count ||
      result.conservative_cumulative_vertex_count >
          ExactPowerCellSubsetRepairBudget::
              trusted_maximum_cumulative_vertex_count ||
      result.conservative_cumulative_incidence_count >
          ExactPowerCellSubsetRepairBudget::
              trusted_maximum_cumulative_incidence_count) {
    throw std::logic_error(
        "the Phase 7 subset-repair requirement exceeded its proof cap");
  }
  return result;
}

[[nodiscard]] bool subset_repair_budget_covers(
    const ExactPowerCellSubsetRepairBudget& budget,
    const ExactPowerCellSubsetRepairRequirements& requirements) {
  if (!subset_closure_budget_covers(
          budget.subset_closure_budget,
          requirements.subset_closure_requirements)) {
    return false;
  }
  return !requirements.conservative_repaired_cell_requirements.has_value() ||
         budget_covers(
             budget.repaired_cell_budget,
             *requirements.conservative_repaired_cell_requirements);
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

std::string_view to_string(ExactPowerCellSubsetClosureDecision decision) {
  switch (decision) {
    case ExactPowerCellSubsetClosureDecision::complete_nonempty:
      return "complete_nonempty";
    case ExactPowerCellSubsetClosureDecision::complete_empty:
      return "complete_empty";
    case ExactPowerCellSubsetClosureDecision::incomplete:
      return "incomplete";
    case ExactPowerCellSubsetClosureDecision::insufficient_budget:
      return "insufficient_budget";
  }
  throw std::invalid_argument(
      "a power-cell subset-closure decision is invalid");
}

std::string_view to_string(ExactPowerCellSubsetClosureWitnessKind kind) {
  switch (kind) {
    case ExactPowerCellSubsetClosureWitnessKind::violating_halfspace:
      return "violating_halfspace";
    case ExactPowerCellSubsetClosureWitnessKind::missing_active_incidence:
      return "missing_active_incidence";
    case ExactPowerCellSubsetClosureWitnessKind::competitor_dominates:
      return "competitor_dominates";
  }
  throw std::invalid_argument(
      "a power-cell subset-closure witness kind is invalid");
}

std::string_view to_string(ExactPowerCellSubsetRepairDecision decision) {
  switch (decision) {
    case ExactPowerCellSubsetRepairDecision::already_complete_nonempty:
      return "already_complete_nonempty";
    case ExactPowerCellSubsetRepairDecision::already_complete_empty:
      return "already_complete_empty";
    case ExactPowerCellSubsetRepairDecision::repaired_complete_nonempty:
      return "repaired_complete_nonempty";
    case ExactPowerCellSubsetRepairDecision::repaired_complete_empty:
      return "repaired_complete_empty";
    case ExactPowerCellSubsetRepairDecision::insufficient_budget:
      return "insufficient_budget";
  }
  throw std::invalid_argument(
      "a power-cell subset-repair decision is invalid");
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

  const ValidatedClippingBox validated_box =
      validate_clipping_box(clipping_box);

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
            validated_box.lower[axis])));
    result.boundary_planes.push_back(boundary_plane(
        upper_kinds[axis],
        std::nullopt,
        axis_form(
            axis,
            ExactRational{exact::BigInt{1}},
            -validated_box.upper[axis])));
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

ExactPowerCellSubsetClosureResult
certify_exact_bounded_power_cell_subset_closure(
    const Binary64WeightedSite3& owner,
    std::span<const Binary64WeightedSite3> complete_competitors,
    const ExactDyadicAabb3& clipping_box,
    std::span<const PointId> candidate_competitor_ids,
    ExactPowerCellSubsetClosureBudget budget) {
  validate_subset_closure_budget(budget);
  const ValidatedPowerCellSubsetInput validated =
      validate_power_cell_subset_input(
          owner,
          complete_competitors,
          clipping_box,
          candidate_competitor_ids);
  ExactPowerCellSubsetClosureResult result;
  result.canonical_candidate_competitor_ids =
      validated.canonical_candidate_competitor_ids;
  const std::size_t candidate_count =
      result.canonical_candidate_competitor_ids.size();
  const std::size_t omitted_count =
      validated.requirements.omitted_competitor_count;
  result.requirements = validated.requirements;
  if (!subset_closure_budget_covers(budget, result.requirements)) {
    return result;
  }

  std::vector<Binary64WeightedSite3> candidate_competitors;
  candidate_competitors.reserve(candidate_count);
  for (const PointId candidate_id :
       result.canonical_candidate_competitor_ids) {
    candidate_competitors.push_back(
        *validated.complete_by_id.at(candidate_id));
  }
  result.candidate_cell = build_exact_bounded_power_cell_reference(
      owner,
      candidate_competitors,
      clipping_box,
      budget.candidate_cell_budget);
  if (result.candidate_cell.decision ==
      ExactPowerCellReferenceDecision::insufficient_budget) {
    throw std::logic_error(
        "a covered subset-closure preflight produced an insufficient cell");
  }
  if (result.candidate_cell.decision ==
      ExactPowerCellReferenceDecision::complete_empty) {
    result.decision = ExactPowerCellSubsetClosureDecision::complete_empty;
    return result;
  }

  result.required_candidate_additions.reserve(omitted_count);
  const auto record_required_addition =
      [&result](PointId competitor_id,
                PowerBisectorConstraintKind constraint_kind,
                ExactPowerCellSubsetClosureWitnessKind witness_kind,
                std::optional<std::size_t> candidate_vertex_index) {
        result.required_candidate_additions.push_back(competitor_id);
        if (!result.first_omitted_witness.has_value()) {
          result.first_omitted_witness = ExactPowerCellSubsetClosureWitness{
              competitor_id,
              constraint_kind,
              witness_kind,
              candidate_vertex_index};
        }
      };
  for (const auto& [competitor_id, competitor] :
       validated.complete_by_id) {
    if (std::binary_search(
            result.canonical_candidate_competitor_ids.begin(),
            result.canonical_candidate_competitor_ids.end(),
            competitor_id)) {
      continue;
    }
    const ExactPowerBisectorConstraint constraint =
        make_exact_power_bisector_constraint(owner, *competitor);
    ++result.audit.classified_omitted_constraint_count;
    switch (constraint.kind) {
      case PowerBisectorConstraintKind::proper_halfspace:
        ++result.audit.omitted_proper_halfspace_count;
        {
          std::optional<std::size_t> first_violating_vertex_index;
          std::optional<std::size_t> first_active_vertex_index;
          for (std::size_t vertex_index = 0U;
               vertex_index < result.candidate_cell.vertices.size();
               ++vertex_index) {
            ++result.audit.exact_omitted_vertex_test_count;
            if (result.audit.exact_omitted_vertex_test_count >
                result.requirements.conservative_omitted_vertex_test_count) {
              throw std::logic_error(
                  "the Phase 7 subset-closure scan exceeded its preflight "
                  "contract");
            }
            const int sign =
                constraint.owner_minus_competitor
                    .evaluate(
                        result.candidate_cell.vertices[vertex_index].position)
                    .sign();
            if (sign > 0 &&
                !first_violating_vertex_index.has_value()) {
              first_violating_vertex_index = vertex_index;
            }
            if (sign == 0 && !first_active_vertex_index.has_value()) {
              first_active_vertex_index = vertex_index;
            }
          }
          if (first_violating_vertex_index.has_value()) {
            ++result.audit.omitted_violating_halfspace_count;
            record_required_addition(
                competitor_id,
                constraint.kind,
                ExactPowerCellSubsetClosureWitnessKind::violating_halfspace,
                first_violating_vertex_index);
          } else if (first_active_vertex_index.has_value()) {
            ++result.audit.omitted_missing_active_incidence_count;
            record_required_addition(
                competitor_id,
                constraint.kind,
                ExactPowerCellSubsetClosureWitnessKind::
                    missing_active_incidence,
                first_active_vertex_index);
          }
        }
        break;
      case PowerBisectorConstraintKind::owner_dominates:
        ++result.audit.omitted_owner_dominates_count;
        break;
      case PowerBisectorConstraintKind::coincident_tie:
        ++result.audit.omitted_coincident_tie_count;
        break;
      case PowerBisectorConstraintKind::competitor_dominates:
        ++result.audit.omitted_competitor_dominates_count;
        record_required_addition(
            competitor_id,
            constraint.kind,
            ExactPowerCellSubsetClosureWitnessKind::competitor_dominates,
            std::nullopt);
        break;
    }
  }
  if (result.audit.classified_omitted_constraint_count != omitted_count ||
      result.audit.exact_omitted_vertex_test_count >
          result.requirements.conservative_omitted_vertex_test_count ||
      result.required_candidate_additions.size() > omitted_count) {
    throw std::logic_error(
        "the Phase 7 subset-closure scan exceeded its preflight contract");
  }
  result.decision = result.required_candidate_additions.empty()
                        ? ExactPowerCellSubsetClosureDecision::complete_nonempty
                        : ExactPowerCellSubsetClosureDecision::incomplete;
  return result;
}

ExactPowerCellSubsetRepairResult
repair_exact_bounded_power_cell_subset_closure(
    const Binary64WeightedSite3& owner,
    std::span<const Binary64WeightedSite3> complete_competitors,
    const ExactDyadicAabb3& clipping_box,
    std::span<const PointId> candidate_competitor_ids,
    ExactPowerCellSubsetRepairBudget budget) {
  validate_subset_closure_budget(budget.subset_closure_budget);
  validate_budget(budget.repaired_cell_budget);
  const ValidatedPowerCellSubsetInput validated =
      validate_power_cell_subset_input(
          owner,
          complete_competitors,
          clipping_box,
          candidate_competitor_ids);

  ExactPowerCellSubsetRepairResult result;
  result.requirements =
      subset_repair_requirements_for(validated.requirements);
  result.canonical_initial_candidate_competitor_ids =
      validated.canonical_candidate_competitor_ids;
  if (!subset_repair_budget_covers(budget, result.requirements)) {
    return result;
  }

  result.initial_closure =
      certify_exact_bounded_power_cell_subset_closure(
          owner,
          complete_competitors,
          clipping_box,
          result.canonical_initial_candidate_competitor_ids,
          budget.subset_closure_budget);
  if (result.initial_closure->decision ==
      ExactPowerCellSubsetClosureDecision::insufficient_budget) {
    throw std::logic_error(
        "a covered subset-repair preflight produced an insufficient "
        "closure");
  }
  result.audit.exact_cell_construction_count = 1U;
  result.audit.omitted_constraint_scan_pass_count =
      result.initial_closure->audit.classified_omitted_constraint_count == 0U
          ? 0U
          : 1U;

  switch (result.initial_closure->decision) {
    case ExactPowerCellSubsetClosureDecision::complete_nonempty:
      result.decision =
          ExactPowerCellSubsetRepairDecision::already_complete_nonempty;
      result.canonical_closed_candidate_competitor_ids =
          result.initial_closure->canonical_candidate_competitor_ids;
      return result;
    case ExactPowerCellSubsetClosureDecision::complete_empty:
      result.decision =
          ExactPowerCellSubsetRepairDecision::already_complete_empty;
      result.canonical_closed_candidate_competitor_ids =
          result.initial_closure->canonical_candidate_competitor_ids;
      return result;
    case ExactPowerCellSubsetClosureDecision::incomplete:
      break;
    case ExactPowerCellSubsetClosureDecision::insufficient_budget:
      throw std::logic_error(
          "an insufficient subset closure escaped the repair preflight");
  }

  if (result.initial_closure->required_candidate_additions.empty() ||
      !result.requirements.conservative_repaired_cell_requirements
           .has_value()) {
    throw std::logic_error(
        "an incomplete subset closure produced no repairable addition");
  }
  result.canonical_closed_candidate_competitor_ids =
      result.initial_closure->canonical_candidate_competitor_ids;
  result.canonical_closed_candidate_competitor_ids.insert(
      result.canonical_closed_candidate_competitor_ids.end(),
      result.initial_closure->required_candidate_additions.begin(),
      result.initial_closure->required_candidate_additions.end());
  std::sort(
      result.canonical_closed_candidate_competitor_ids.begin(),
      result.canonical_closed_candidate_competitor_ids.end());
  if (result.canonical_closed_candidate_competitor_ids.size() >
          complete_competitors.size() ||
      std::adjacent_find(
          result.canonical_closed_candidate_competitor_ids.begin(),
          result.canonical_closed_candidate_competitor_ids.end()) !=
          result.canonical_closed_candidate_competitor_ids.end()) {
    throw std::logic_error(
        "a subset repair produced a non-canonical competitor union");
  }

  std::vector<Binary64WeightedSite3> repaired_competitors;
  repaired_competitors.reserve(
      result.canonical_closed_candidate_competitor_ids.size());
  for (const PointId competitor_id :
       result.canonical_closed_candidate_competitor_ids) {
    repaired_competitors.push_back(*validated.complete_by_id.at(competitor_id));
  }
  result.repaired_cell = build_exact_bounded_power_cell_reference(
      owner,
      repaired_competitors,
      clipping_box,
      budget.repaired_cell_budget);
  if (result.repaired_cell->decision ==
      ExactPowerCellReferenceDecision::insufficient_budget) {
    throw std::logic_error(
        "a covered subset-repair preflight produced an insufficient rebuilt "
        "cell");
  }
  result.audit.exact_cell_construction_count = 2U;
  result.audit.simultaneous_repair_batch_count = 1U;
  result.audit.simultaneously_added_competitor_count =
      result.initial_closure->required_candidate_additions.size();
  result.decision =
      result.repaired_cell->decision ==
              ExactPowerCellReferenceDecision::complete_nonempty
          ? ExactPowerCellSubsetRepairDecision::repaired_complete_nonempty
          : ExactPowerCellSubsetRepairDecision::repaired_complete_empty;
  return result;
}

}  // namespace morsehgp3d::spatial
