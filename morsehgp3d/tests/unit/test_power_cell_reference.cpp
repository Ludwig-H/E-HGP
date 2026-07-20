#include "morsehgp3d/exact/predicates.hpp"
#include "morsehgp3d/spatial/power_cell_reference.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactAffineForm3;
using morsehgp3d::exact::ExactLabelMoments;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::exact::power_bisector_affine_form;
using morsehgp3d::spatial::Binary64WeightedSite3;
using morsehgp3d::spatial::ExactDyadicAabb3;
using morsehgp3d::spatial::ExactPowerCellReferenceBudget;
using morsehgp3d::spatial::ExactPowerCellReferenceDecision;
using morsehgp3d::spatial::ExactPowerCellReferenceResult;
using morsehgp3d::spatial::PointId;
using morsehgp3d::spatial::PowerBisectorConstraintKind;
using morsehgp3d::spatial::build_exact_bounded_power_cell_reference;
using morsehgp3d::spatial::exact_power_difference_affine_form;
using morsehgp3d::spatial::exact_power_distance;
using morsehgp3d::spatial::make_exact_power_bisector_constraint;

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

[[nodiscard]] Binary64WeightedSite3 site(
    PointId id,
    double x,
    double y,
    double z,
    double weight = 0.0) {
  return Binary64WeightedSite3::from_binary64(id, x, y, z, weight);
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

[[nodiscard]] std::string point_key(const ExactRational3& point) {
  return point.coordinate(0U).canonical_key() + ":" +
         point.coordinate(1U).canonical_key() + ":" +
         point.coordinate(2U).canonical_key();
}

[[nodiscard]] std::set<std::string> vertex_keys(
    const ExactPowerCellReferenceResult& result) {
  std::set<std::string> keys;
  for (const auto& vertex : result.vertices) {
    keys.insert(point_key(vertex.position));
  }
  return keys;
}

[[nodiscard]] std::set<std::string> cube_keys(double lower, double upper) {
  std::set<std::string> keys;
  for (const double x : {lower, upper}) {
    for (const double y : {lower, upper}) {
      for (const double z : {lower, upper}) {
        keys.insert(point_key(CertifiedPoint3::from_binary64(x, y, z).exact()));
      }
    }
  }
  return keys;
}

void check_opposites(
    const ExactAffineForm3& first,
    const ExactAffineForm3& second,
    const std::string& message) {
  for (std::size_t coefficient = 0U; coefficient < 4U; ++coefficient) {
    check(
        first.coefficient(coefficient) == -second.coefficient(coefficient),
        message + " coefficient " + std::to_string(coefficient));
  }
}

void test_analytic_weight_convention_and_phase2a_seam() {
  const Binary64WeightedSite3 owner = site(10U, 0.0, 0.0, 0.0);
  const Binary64WeightedSite3 competitor = site(20U, 2.0, 0.0, 0.0);
  const auto constraint =
      make_exact_power_bisector_constraint(owner, competitor);
  check(
      constraint.kind == PowerBisectorConstraintKind::proper_halfspace &&
          constraint.plane.has_value(),
      "two distinct sites define a proper exact halfspace");
  check(
      constraint.owner_minus_competitor.oriented_key() == "1:0:0:-1",
      "the unweighted owner cell keeps x <= 1");
  check(
      constraint.owner_minus_competitor
              .evaluate(CertifiedPoint3::from_binary64(0.0, 0.0, 0.0))
              .sign() < 0 &&
          constraint.owner_minus_competitor
              .evaluate(CertifiedPoint3::from_binary64(1.0, 0.0, 0.0))
              .is_zero() &&
          constraint.owner_minus_competitor
              .evaluate(CertifiedPoint3::from_binary64(2.0, 0.0, 0.0))
              .sign() > 0,
      "the affine sign is power(owner)-power(competitor)");

  const ExactRational3 query{BigInt{3}, BigInt{0}, BigInt{0}, BigInt{2}};
  check(
      constraint.owner_minus_competitor.evaluate(query) ==
          exact_power_distance(owner, query) -
              exact_power_distance(competitor, query),
      "the affine form exactly equals the power-distance difference");

  const std::array<CertifiedPoint3, 2> point_table{
      owner.position(), competitor.position()};
  const std::array<std::uint32_t, 1> owner_id{0U};
  const std::array<std::uint32_t, 1> competitor_id{1U};
  const ExactLabelMoments owner_label = ExactLabelMoments::from_canonical_ids(
      owner_id, point_table);
  const ExactLabelMoments competitor_label =
      ExactLabelMoments::from_canonical_ids(competitor_id, point_table);
  check(
      power_bisector_affine_form(owner_label, competitor_label) ==
          constraint.owner_minus_competitor,
      "zero-weight singleton power agrees with the qualified label predicate");

  const auto reverse =
      make_exact_power_bisector_constraint(competitor, owner);
  check_opposites(
      constraint.owner_minus_competitor,
      reverse.owner_minus_competitor,
      "exchanging owner and competitor negates the form");
  check(
      constraint.plane->same_geometric_plane(*reverse.plane),
      "exchanging sites preserves the unoriented geometric plane");

  const auto shifted = exact_power_difference_affine_form(
      site(10U, 0.0, 0.0, 0.0, 10.0),
      site(20U, 2.0, 0.0, 0.0, 5.0));
  const auto original = exact_power_difference_affine_form(
      site(10U, 0.0, 0.0, 0.0, 3.0),
      site(20U, 2.0, 0.0, 0.0, -2.0));
  check(
      shifted == original,
      "adding one exact common weight leaves the bisector unchanged");
}

void test_weights_move_the_retained_halfspace() {
  const Binary64WeightedSite3 competitor = site(2U, 2.0, 0.0, 0.0);
  const auto positive_owner = make_exact_power_bisector_constraint(
      site(1U, 0.0, 0.0, 0.0, 4.0), competitor);
  const auto negative_owner = make_exact_power_bisector_constraint(
      site(1U, 0.0, 0.0, 0.0, -4.0), competitor);
  check(
      positive_owner.owner_minus_competitor
          .evaluate(CertifiedPoint3::from_binary64(2.0, 0.0, 0.0))
          .is_zero(),
      "a positive owner weight expands its boundary from x=1 to x=2");
  check(
      negative_owner.owner_minus_competitor
          .evaluate(CertifiedPoint3::from_binary64(0.0, 0.0, 0.0))
          .is_zero(),
      "a negative owner weight contracts its boundary from x=1 to x=0");

  const std::array<Binary64WeightedSite3, 1> competitors{competitor};
  const ExactPowerCellReferenceResult cell =
      build_exact_bounded_power_cell_reference(
          site(1U, 0.0, 0.0, 0.0, 4.0),
          competitors,
          box(-4.0, -2.0, -2.0, 4.0, 2.0, 2.0));
  check(
      cell.decision == ExactPowerCellReferenceDecision::complete_nonempty &&
          cell.vertices.size() == 8U,
      "the expanded halfspace clipped by a box has eight exact vertices");
  for (const auto& vertex : cell.vertices) {
    check(
        vertex.position.coordinate(0U) == ExactRational{BigInt{-4}} ||
            vertex.position.coordinate(0U) == ExactRational{BigInt{2}},
        "the clipped weighted cell uses x=-4 and x=2");
  }
}

void test_exact_cube_and_permutation() {
  const Binary64WeightedSite3 owner = site(0U, 0.0, 0.0, 0.0);
  const std::array<Binary64WeightedSite3, 6> competitors{
      site(6U, 0.0, 0.0, -2.0),
      site(2U, -2.0, 0.0, 0.0),
      site(5U, 0.0, 2.0, 0.0),
      site(1U, 2.0, 0.0, 0.0),
      site(4U, 0.0, -2.0, 0.0),
      site(3U, 0.0, 0.0, 2.0)};
  const ExactDyadicAabb3 clipping_box =
      box(-4.0, -4.0, -4.0, 4.0, 4.0, 4.0);
  const ExactPowerCellReferenceResult result =
      build_exact_bounded_power_cell_reference(
          owner, competitors, clipping_box);
  check(
      result.decision == ExactPowerCellReferenceDecision::complete_nonempty,
      "six symmetric competitors produce a nonempty exact cell");
  check(
      result.pairwise_constraints.size() == 6U &&
          result.audit.proper_bisector_count == 6U &&
          result.boundary_planes.size() == 12U,
      "the cube retains six box planes and six proper bisectors");
  check(
      result.vertices.size() == 8U &&
          vertex_keys(result) == cube_keys(-1.0, 1.0),
      "the symmetric power cell is exactly [-1,1]^3");
  for (const auto& vertex : result.vertices) {
    check(
        vertex.active_boundary_plane_indices.size() == 3U,
        "each generic cube vertex has three active power planes");
  }

  std::array<Binary64WeightedSite3, 6> reversed = competitors;
  std::reverse(reversed.begin(), reversed.end());
  const ExactPowerCellReferenceResult replay =
      build_exact_bounded_power_cell_reference(owner, reversed, clipping_box);
  check(
      vertex_keys(replay) == vertex_keys(result) &&
          replay.audit.active_incidence_count ==
              result.audit.active_incidence_count,
      "competitor arrival order cannot change the exact cell");
  check(
      replay == result,
      "competitor arrival order preserves the complete canonical result");
  for (std::size_t index = 0U; index < result.pairwise_constraints.size();
       ++index) {
    check(
        result.pairwise_constraints[index].competitor_id == index + 1U &&
            replay.pairwise_constraints[index].competitor_id == index + 1U,
        "pairwise constraints are canonically sorted by competitor id");
  }
}

void test_coincident_sites_fail_closed_mathematically() {
  const Binary64WeightedSite3 owner = site(1U, 0.0, 0.0, 0.0);
  const ExactDyadicAabb3 clipping_box =
      box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0);
  const std::array<Binary64WeightedSite3, 1> tied{
      site(2U, 0.0, 0.0, 0.0)};
  const auto tied_result = build_exact_bounded_power_cell_reference(
      owner, tied, clipping_box);
  check(
      tied_result.pairwise_constraints.front().kind ==
              PowerBisectorConstraintKind::coincident_tie &&
          tied_result.vertices.size() == 8U &&
          tied_result.audit.redundant_constraint_count == 1U &&
          tied_result.audit.has_coincident_tie,
      "coincident equal-weight sites tie everywhere without cutting the box");

  const std::array<Binary64WeightedSite3, 1> lighter_competitor{
      site(2U, 0.0, 0.0, 0.0, -2.0)};
  const auto owner_dominates = build_exact_bounded_power_cell_reference(
      owner, lighter_competitor, clipping_box);
  check(
      owner_dominates.pairwise_constraints.front().kind ==
              PowerBisectorConstraintKind::owner_dominates &&
          owner_dominates.vertices.size() == 8U,
      "the heavier coincident owner dominates throughout the box");

  const std::array<Binary64WeightedSite3, 1> heavier_competitor{
      site(2U, 0.0, 0.0, 0.0, 2.0)};
  const auto competitor_dominates =
      build_exact_bounded_power_cell_reference(
          owner, heavier_competitor, clipping_box);
  check(
      competitor_dominates.decision ==
              ExactPowerCellReferenceDecision::complete_empty &&
          competitor_dominates.pairwise_constraints.front().kind ==
              PowerBisectorConstraintKind::competitor_dominates &&
          competitor_dominates.vertices.empty(),
      "the lighter coincident owner has an exactly empty cell");
}

void test_proper_empty_and_lower_dimensional_cells() {
  const ExactDyadicAabb3 clipping_box =
      box(-2.0, -2.0, -2.0, 2.0, 2.0, 2.0);
  const std::array<Binary64WeightedSite3, 2> incompatible{
      site(1U, -1.0, 0.0, 0.0),
      site(2U, 1.0, 0.0, 0.0)};
  const ExactPowerCellReferenceResult empty =
      build_exact_bounded_power_cell_reference(
          site(0U, 0.0, 0.0, 0.0, -4.0),
          incompatible,
          clipping_box);
  check(
      empty.decision == ExactPowerCellReferenceDecision::complete_empty &&
          empty.audit.proper_bisector_count == 2U &&
          empty.audit.infeasible_constraint_count == 0U &&
          empty.vertices.empty(),
      "incompatible proper halfspaces produce an exact empty cell");

  const std::array<Binary64WeightedSite3, 2> plane_constraints{
      site(1U, -1.0, 0.0, 0.0, 1.0),
      site(2U, 1.0, 0.0, 0.0, 1.0)};
  const ExactPowerCellReferenceResult plane =
      build_exact_bounded_power_cell_reference(
          site(0U, 0.0, 0.0, 0.0),
          plane_constraints,
          box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0));
  check(
      plane.decision == ExactPowerCellReferenceDecision::complete_nonempty &&
          plane.vertices.size() == 4U,
      "opposite proper constraints retain the exact plane x=0");
  for (const auto& vertex : plane.vertices) {
    check(
        vertex.position.coordinate(0U).is_zero() &&
            vertex.active_boundary_plane_indices.size() == 4U,
        "each two-dimensional cell vertex records both coincident boundaries");
  }

  const std::array<Binary64WeightedSite3, 4> line_constraints{
      site(1U, -1.0, 0.0, 0.0, 1.0),
      site(2U, 1.0, 0.0, 0.0, 1.0),
      site(3U, 0.0, -1.0, 0.0, 1.0),
      site(4U, 0.0, 1.0, 0.0, 1.0)};
  const ExactPowerCellReferenceResult line =
      build_exact_bounded_power_cell_reference(
          site(0U, 0.0, 0.0, 0.0),
          line_constraints,
          box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0));
  check(
      line.vertices.size() == 2U &&
          line.vertices.front().position.coordinate(0U).is_zero() &&
          line.vertices.front().position.coordinate(1U).is_zero(),
      "four proper constraints retain the exact line x=y=0");

  const std::array<Binary64WeightedSite3, 6> point_constraints{
      site(1U, -1.0, 0.0, 0.0, 1.0),
      site(2U, 1.0, 0.0, 0.0, 1.0),
      site(3U, 0.0, -1.0, 0.0, 1.0),
      site(4U, 0.0, 1.0, 0.0, 1.0),
      site(5U, 0.0, 0.0, -1.0, 1.0),
      site(6U, 0.0, 0.0, 1.0, 1.0)};
  const ExactPowerCellReferenceResult point =
      build_exact_bounded_power_cell_reference(
          site(0U, 0.0, 0.0, 0.0),
          point_constraints,
          box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0));
  check(
      point.vertices.size() == 1U &&
          point.vertices.front().position == ExactRational3{} &&
          point.vertices.front().active_boundary_plane_indices.size() == 6U,
      "six proper constraints retain the exact origin with six incidences");
}

void test_preflight_and_input_contracts() {
  const Binary64WeightedSite3 owner = site(0U, 0.0, 0.0, 0.0);
  const std::array<Binary64WeightedSite3, 1> competitors{
      site(1U, 2.0, 0.0, 0.0)};
  const ExactDyadicAabb3 clipping_box =
      box(-2.0, -2.0, -2.0, 2.0, 2.0, 2.0);

  std::array<ExactPowerCellReferenceBudget, 5> short_budgets{};
  short_budgets[0U].maximum_site_count = 1U;
  short_budgets[1U].maximum_constraint_count = 6U;
  short_budgets[2U].maximum_plane_triple_count = 34U;
  short_budgets[3U].maximum_vertex_count = 34U;
  short_budgets[4U].maximum_incidence_count = 244U;
  for (std::size_t index = 0U; index < short_budgets.size(); ++index) {
    const auto insufficient = build_exact_bounded_power_cell_reference(
        owner, competitors, clipping_box, short_budgets[index]);
    check(
        insufficient.decision ==
                ExactPowerCellReferenceDecision::insufficient_budget &&
            insufficient.requirements.conservative_constraint_count == 7U &&
            insufficient.requirements.conservative_plane_triple_count == 35U &&
            insufficient.pairwise_constraints.empty() &&
            insufficient.boundary_planes.empty() &&
            insufficient.vertices.empty(),
        "each one-below preflight returns an atomic empty payload " +
            std::to_string(index));
  }

  ExactPowerCellReferenceBudget excessive;
  excessive.maximum_vertex_count =
      ExactPowerCellReferenceBudget::trusted_maximum_vertex_count + 1U;
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_bounded_power_cell_reference(
            owner, competitors, clipping_box, excessive));
      },
      "an excessive trusted cap is rejected before geometry");

  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_bounded_power_cell_reference(
            owner,
            competitors,
            box(-1.0, -1.0, -1.0, -1.0, 1.0, 1.0)));
      },
      "a zero-width clipping box is rejected");
  check_throws<std::invalid_argument>(
      [&] {
        const std::array<Binary64WeightedSite3, 1> duplicate_ids{
            site(0U, 3.0, 0.0, 0.0)};
        static_cast<void>(build_exact_bounded_power_cell_reference(
            owner, duplicate_ids, clipping_box));
      },
      "an owner id reused by a competitor is rejected");

  const std::array<Binary64WeightedSite3, 8> too_many{
      site(1U, 1.0, 0.0, 0.0),
      site(2U, 2.0, 0.0, 0.0),
      site(3U, 3.0, 0.0, 0.0),
      site(4U, 4.0, 0.0, 0.0),
      site(5U, 5.0, 0.0, 0.0),
      site(6U, 6.0, 0.0, 0.0),
      site(7U, 7.0, 0.0, 0.0),
      site(8U, 8.0, 0.0, 0.0)};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_bounded_power_cell_reference(
            owner, too_many, clipping_box));
      },
      "the bounded oracle rejects more than eight total sites");
}

}  // namespace

int main() {
  test_analytic_weight_convention_and_phase2a_seam();
  test_weights_move_the_retained_halfspace();
  test_exact_cube_and_permutation();
  test_coincident_sites_fail_closed_mathematically();
  test_proper_empty_and_lower_dimensional_cells();
  test_preflight_and_input_contracts();
  if (failures != 0) {
    std::cerr << failures << " power-cell reference checks failed\n";
    return 1;
  }
  return 0;
}
