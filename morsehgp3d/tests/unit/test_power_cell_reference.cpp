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
using morsehgp3d::spatial::ExactPowerCellSubsetClosureBudget;
using morsehgp3d::spatial::ExactPowerCellSubsetClosureDecision;
using morsehgp3d::spatial::ExactPowerCellSubsetClosureWitnessKind;
using morsehgp3d::spatial::ExactPowerCellSubsetRepairBudget;
using morsehgp3d::spatial::ExactPowerCellSubsetRepairDecision;
using morsehgp3d::spatial::ExactPowerCellSubsetRepairResult;
using morsehgp3d::spatial::PointId;
using morsehgp3d::spatial::PowerBisectorConstraintKind;
using morsehgp3d::spatial::build_exact_bounded_power_cell_reference;
using morsehgp3d::spatial::certify_exact_bounded_power_cell_subset_closure;
using morsehgp3d::spatial::exact_power_difference_affine_form;
using morsehgp3d::spatial::exact_power_distance;
using morsehgp3d::spatial::make_exact_power_bisector_constraint;
using morsehgp3d::spatial::repair_exact_bounded_power_cell_subset_closure;

template <typename Result>
concept HasRvalueFinalCell = requires(Result&& result) {
  std::move(result).final_cell();
};

static_assert(!HasRvalueFinalCell<ExactPowerCellSubsetRepairResult>);
static_assert(requires(const ExactPowerCellSubsetRepairResult& result) {
  result.final_cell();
});

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

template <typename Function>
void check_invalid_argument_message(
    Function&& function,
    const std::string& expected_message,
    const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const std::invalid_argument& error) {
    check(error.what() == expected_message, message + " (wrong message)");
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

void test_subset_closure_repairs_and_authentic_supersets() {
  const Binary64WeightedSite3 owner = site(0U, 0.0, 0.0, 0.0);
  const std::array<Binary64WeightedSite3, 2> complete_competitors{
      site(7U, 10.0, 0.0, 0.0),
      site(2U, 2.0, 0.0, 0.0)};
  const ExactDyadicAabb3 clipping_box =
      box(-2.0, -2.0, -2.0, 2.0, 2.0, 2.0);
  const std::array<PointId, 1> missing_active{7U};
  const auto incomplete =
      certify_exact_bounded_power_cell_subset_closure(
          owner, complete_competitors, clipping_box, missing_active);
  check(
      incomplete.decision ==
              ExactPowerCellSubsetClosureDecision::incomplete &&
          incomplete.canonical_candidate_competitor_ids ==
              std::vector<PointId>{7U} &&
          incomplete.first_omitted_witness.has_value() &&
          incomplete.first_omitted_witness->omitted_competitor_id == 2U &&
          incomplete.first_omitted_witness->witness_kind ==
              ExactPowerCellSubsetClosureWitnessKind::violating_halfspace &&
          incomplete.first_omitted_witness->candidate_vertex_index == 4U &&
          incomplete.required_candidate_additions ==
              std::vector<PointId>{2U} &&
          incomplete.audit.exact_omitted_vertex_test_count == 8U,
      "the first omitted cutting halfspace has a canonical exact vertex "
      "witness");

  const std::array<PointId, 2> repaired_with_authentic_superset{7U, 2U};
  const auto repaired = certify_exact_bounded_power_cell_subset_closure(
      owner,
      complete_competitors,
      clipping_box,
      repaired_with_authentic_superset);
  check(
      repaired.decision ==
              ExactPowerCellSubsetClosureDecision::complete_nonempty &&
          repaired.canonical_candidate_competitor_ids ==
              std::vector<PointId>({2U, 7U}) &&
          repaired.required_candidate_additions.empty() &&
          !repaired.first_omitted_witness.has_value(),
      "adding the missing id closes the cell and keeps an authentic far "
      "superset");

  const std::array<PointId, 1> exact_active_subset{2U};
  const auto far_omitted =
      certify_exact_bounded_power_cell_subset_closure(
          owner, complete_competitors, clipping_box, exact_active_subset);
  check(
      far_omitted.decision ==
              ExactPowerCellSubsetClosureDecision::complete_nonempty &&
          far_omitted.audit.omitted_proper_halfspace_count == 1U &&
          far_omitted.required_candidate_additions.empty() &&
          far_omitted.audit.exact_omitted_vertex_test_count == 8U,
      "a proper omitted constraint strictly outside every vertex is "
      "certified redundant on the candidate cell");

  std::array<Binary64WeightedSite3, 2> reversed = complete_competitors;
  std::reverse(reversed.begin(), reversed.end());
  const std::array<PointId, 2> reversed_candidates{2U, 7U};
  const auto replay = certify_exact_bounded_power_cell_subset_closure(
      owner, reversed, clipping_box, reversed_candidates);
  check(
      replay == repaired,
      "full-site and candidate-id permutations preserve the closure result");
}

void test_subset_closure_active_incidence_and_lower_dimension() {
  const Binary64WeightedSite3 owner = site(0U, 0.0, 0.0, 0.0);
  const ExactDyadicAabb3 clipping_box =
      box(-2.0, -2.0, -2.0, 2.0, 2.0, 2.0);
  const std::array<Binary64WeightedSite3, 1> box_face_competitor{
      site(4U, 4.0, 0.0, 0.0)};
  const auto missing_box_face =
      certify_exact_bounded_power_cell_subset_closure(
          owner,
          box_face_competitor,
          clipping_box,
          std::span<const PointId>{});
  check(
      missing_box_face.decision ==
              ExactPowerCellSubsetClosureDecision::incomplete &&
          missing_box_face.first_omitted_witness.has_value() &&
          missing_box_face.first_omitted_witness->witness_kind ==
              ExactPowerCellSubsetClosureWitnessKind::
                  missing_active_incidence &&
          missing_box_face.first_omitted_witness->candidate_vertex_index ==
              4U &&
          missing_box_face.required_candidate_additions ==
              std::vector<PointId>{4U} &&
          missing_box_face.audit.omitted_missing_active_incidence_count ==
              1U &&
          missing_box_face.audit.exact_omitted_vertex_test_count == 8U,
      "an omitted plane coincident with a box face is an incomplete active "
      "incidence, not a redundant constraint");

  const std::array<Binary64WeightedSite3, 2> simultaneous_competitors{
      site(4U, 0.0, 4.0, 0.0),
      site(2U, 2.0, 0.0, 0.0)};
  const auto simultaneous =
      certify_exact_bounded_power_cell_subset_closure(
          owner,
          simultaneous_competitors,
          clipping_box,
          std::span<const PointId>{});
  check(
      simultaneous.decision ==
              ExactPowerCellSubsetClosureDecision::incomplete &&
          simultaneous.required_candidate_additions ==
              std::vector<PointId>({2U, 4U}) &&
          simultaneous.first_omitted_witness.has_value() &&
          simultaneous.first_omitted_witness->omitted_competitor_id == 2U &&
          simultaneous.first_omitted_witness->witness_kind ==
              ExactPowerCellSubsetClosureWitnessKind::violating_halfspace &&
          simultaneous.audit.omitted_violating_halfspace_count == 1U &&
          simultaneous.audit.omitted_missing_active_incidence_count == 1U &&
          simultaneous.audit.exact_omitted_vertex_test_count == 16U,
      "one exhaustive scan returns all cutting and active omitted ids for a "
      "simultaneous rebuild");
  const std::array<PointId, 2> simultaneous_repair{4U, 2U};
  const auto simultaneous_closed =
      certify_exact_bounded_power_cell_subset_closure(
          owner,
          simultaneous_competitors,
          clipping_box,
          simultaneous_repair);
  check(
      simultaneous_closed.decision ==
              ExactPowerCellSubsetClosureDecision::complete_nonempty &&
          simultaneous_closed.required_candidate_additions.empty(),
      "the canonical simultaneous additions close in one rebuilt cell");

  const std::array<Binary64WeightedSite3, 3> lower_dimensional_competitors{
      site(1U, -1.0, 0.0, 0.0, 1.0),
      site(2U, 1.0, 0.0, 0.0, 1.0),
      site(3U, 0.0, 2.0, 0.0)};
  const std::array<PointId, 2> plane_candidate{2U, 1U};
  const auto lower_dimensional =
      certify_exact_bounded_power_cell_subset_closure(
          owner,
          lower_dimensional_competitors,
          box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0),
          plane_candidate);
  check(
      lower_dimensional.candidate_cell.vertices.size() == 4U &&
          lower_dimensional.decision ==
              ExactPowerCellSubsetClosureDecision::incomplete &&
          lower_dimensional.first_omitted_witness.has_value() &&
          lower_dimensional.first_omitted_witness->witness_kind ==
              ExactPowerCellSubsetClosureWitnessKind::
                  missing_active_incidence &&
          lower_dimensional.first_omitted_witness
                  ->candidate_vertex_index == 2U &&
          lower_dimensional.required_candidate_additions ==
              std::vector<PointId>{3U},
      "the exact vertex test retains active incidences on a dimensional "
      "cell");
}

void test_subset_closure_distinct_ids_and_vertex_tangency() {
  const Binary64WeightedSite3 owner = site(0U, 0.0, 0.0, 0.0);
  const ExactDyadicAabb3 clipping_box =
      box(-2.0, -2.0, -2.0, 2.0, 2.0, 2.0);
  const std::array<Binary64WeightedSite3, 2> same_plane_competitors{
      site(2U, 2.0, 0.0, 0.0),
      site(7U, 4.0, 0.0, 0.0, 8.0)};
  const auto first_plane = make_exact_power_bisector_constraint(
      owner, same_plane_competitors[0U]);
  const auto second_plane = make_exact_power_bisector_constraint(
      owner, same_plane_competitors[1U]);
  const std::array<PointId, 1> one_semantic_id{2U};
  const auto missing_same_plane_id =
      certify_exact_bounded_power_cell_subset_closure(
          owner,
          same_plane_competitors,
          clipping_box,
          one_semantic_id);
  check(
      first_plane.plane.has_value() && second_plane.plane.has_value() &&
          first_plane.owner_minus_competitor.oriented_key() ==
              second_plane.owner_minus_competitor.oriented_key() &&
          first_plane.plane->same_geometric_plane(*second_plane.plane) &&
          missing_same_plane_id.decision ==
              ExactPowerCellSubsetClosureDecision::incomplete &&
          missing_same_plane_id.required_candidate_additions ==
              std::vector<PointId>{7U} &&
          missing_same_plane_id.first_omitted_witness.has_value() &&
          missing_same_plane_id.first_omitted_witness->witness_kind ==
              ExactPowerCellSubsetClosureWitnessKind::
                  missing_active_incidence,
      "distinct competitor ids remain distinct active incidences even when "
      "they induce the same oriented plane");
  const std::array<PointId, 2> both_semantic_ids{7U, 2U};
  const auto both_planes = certify_exact_bounded_power_cell_subset_closure(
      owner,
      same_plane_competitors,
      clipping_box,
      both_semantic_ids);
  check(
      both_planes.decision ==
              ExactPowerCellSubsetClosureDecision::complete_nonempty &&
          both_planes.canonical_candidate_competitor_ids ==
              std::vector<PointId>({2U, 7U}) &&
          both_planes.candidate_cell.boundary_planes.size() == 8U,
      "repairing a coincident semantic plane preserves both authenticated "
      "competitor ids");

  const std::array<Binary64WeightedSite3, 1> corner_tangent{
      site(3U, 2.0, 2.0, 2.0)};
  const auto tangent = certify_exact_bounded_power_cell_subset_closure(
      owner,
      corner_tangent,
      box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0),
      std::span<const PointId>{});
  const bool tangent_witness_is_upper_corner =
      tangent.first_omitted_witness.has_value() &&
      tangent.first_omitted_witness->candidate_vertex_index.has_value() &&
      tangent.candidate_cell
              .vertices[*tangent.first_omitted_witness
                             ->candidate_vertex_index]
              .position ==
          CertifiedPoint3::from_binary64(1.0, 1.0, 1.0).exact();
  check(
      tangent.decision == ExactPowerCellSubsetClosureDecision::incomplete &&
          tangent.required_candidate_additions ==
              std::vector<PointId>{3U} &&
          tangent.first_omitted_witness.has_value() &&
          tangent.first_omitted_witness->witness_kind ==
              ExactPowerCellSubsetClosureWitnessKind::
                  missing_active_incidence &&
          tangent.audit.omitted_violating_halfspace_count == 0U &&
          tangent.audit.omitted_missing_active_incidence_count == 1U &&
          tangent.audit.exact_omitted_vertex_test_count == 8U &&
          tangent_witness_is_upper_corner,
      "a proper plane tangent at one box vertex is an active incidence, not "
      "a redundant halfspace");

  const std::array<Binary64WeightedSite3, 1>
      active_before_violating_competitor{
          site(8U, 2.0, 0.0, 0.0, 8.0)};
  const auto active_before_violating_constraint =
      make_exact_power_bisector_constraint(
          owner, active_before_violating_competitor.front());
  const auto active_before_violating =
      certify_exact_bounded_power_cell_subset_closure(
          owner,
          active_before_violating_competitor,
          box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0),
          std::span<const PointId>{});
  bool canonical_priority_is_witnessed = false;
  if (active_before_violating.candidate_cell.vertices.size() == 8U &&
      active_before_violating.first_omitted_witness.has_value() &&
      active_before_violating.first_omitted_witness
          ->candidate_vertex_index.has_value()) {
    const std::size_t witness_index =
        *active_before_violating.first_omitted_witness
             ->candidate_vertex_index;
    canonical_priority_is_witnessed =
        active_before_violating_constraint.owner_minus_competitor
            .evaluate(active_before_violating.candidate_cell.vertices.front()
                          .position)
            .is_zero() &&
        witness_index == 4U &&
        active_before_violating_constraint.owner_minus_competitor
                .evaluate(active_before_violating.candidate_cell
                              .vertices[witness_index]
                              .position)
                .sign() > 0;
  }
  check(
      active_before_violating_constraint.kind ==
              PowerBisectorConstraintKind::proper_halfspace &&
          active_before_violating.decision ==
              ExactPowerCellSubsetClosureDecision::incomplete &&
          active_before_violating.required_candidate_additions ==
              std::vector<PointId>{8U} &&
          active_before_violating.first_omitted_witness.has_value() &&
          active_before_violating.first_omitted_witness->witness_kind ==
              ExactPowerCellSubsetClosureWitnessKind::violating_halfspace &&
          active_before_violating.audit.omitted_violating_halfspace_count ==
              1U &&
          active_before_violating.audit
                  .omitted_missing_active_incidence_count == 0U &&
          canonical_priority_is_witnessed,
      "a later strict violation takes priority over an earlier active vertex "
      "and points to the first positive canonical witness");
}

void test_subset_closure_proper_empty_and_all_lower_dimensions() {
  const ExactDyadicAabb3 clipping_box =
      box(-2.0, -2.0, -2.0, 2.0, 2.0, 2.0);
  const Binary64WeightedSite3 disadvantaged_owner =
      site(0U, 0.0, 0.0, 0.0, -4.0);
  const std::array<Binary64WeightedSite3, 3> proper_empty_competitors{
      site(1U, -1.0, 0.0, 0.0),
      site(2U, 1.0, 0.0, 0.0),
      site(9U, 10.0, 0.0, 0.0)};
  const std::array<PointId, 2> incompatible_proper_ids{2U, 1U};
  const auto proper_empty = certify_exact_bounded_power_cell_subset_closure(
      disadvantaged_owner,
      proper_empty_competitors,
      clipping_box,
      incompatible_proper_ids);
  check(
      proper_empty.decision ==
              ExactPowerCellSubsetClosureDecision::complete_empty &&
          proper_empty.candidate_cell.decision ==
              ExactPowerCellReferenceDecision::complete_empty &&
          proper_empty.candidate_cell.audit.proper_bisector_count == 2U &&
          proper_empty.candidate_cell.audit.infeasible_constraint_count ==
              0U &&
          proper_empty.candidate_cell.vertices.empty() &&
          proper_empty.audit.classified_omitted_constraint_count == 0U &&
          proper_empty.audit.exact_omitted_vertex_test_count == 0U,
      "an empty intersection of proper candidate halfspaces proves the full "
      "cell empty before scanning omitted constraints");

  const Binary64WeightedSite3 owner = site(0U, 0.0, 0.0, 0.0);
  const std::array<Binary64WeightedSite3, 5> line_competitors{
      site(1U, -1.0, 0.0, 0.0, 1.0),
      site(2U, 1.0, 0.0, 0.0, 1.0),
      site(3U, 0.0, -1.0, 0.0, 1.0),
      site(4U, 0.0, 1.0, 0.0, 1.0),
      site(5U, 0.0, 0.0, 2.0)};
  const std::array<PointId, 4> line_candidate{4U, 2U, 1U, 3U};
  const auto line = certify_exact_bounded_power_cell_subset_closure(
      owner,
      line_competitors,
      box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0),
      line_candidate);
  const bool line_witness_is_upper_endpoint =
      line.first_omitted_witness.has_value() &&
      line.first_omitted_witness->candidate_vertex_index.has_value() &&
      line.candidate_cell
              .vertices[*line.first_omitted_witness
                             ->candidate_vertex_index]
              .position ==
          CertifiedPoint3::from_binary64(0.0, 0.0, 1.0).exact();
  check(
      line.candidate_cell.vertices.size() == 2U &&
          line.decision == ExactPowerCellSubsetClosureDecision::incomplete &&
          line.required_candidate_additions == std::vector<PointId>{5U} &&
          line.first_omitted_witness.has_value() &&
          line.first_omitted_witness->witness_kind ==
              ExactPowerCellSubsetClosureWitnessKind::
                  missing_active_incidence &&
          line.audit.exact_omitted_vertex_test_count == 2U &&
          line_witness_is_upper_endpoint,
      "the omitted-constraint scan retains endpoint incidences on an exact "
      "one-dimensional candidate cell");

  const std::array<Binary64WeightedSite3, 7> point_competitors{
      site(1U, -1.0, 0.0, 0.0, 1.0),
      site(2U, 1.0, 0.0, 0.0, 1.0),
      site(3U, 0.0, -1.0, 0.0, 1.0),
      site(4U, 0.0, 1.0, 0.0, 1.0),
      site(5U, 0.0, 0.0, -1.0, 1.0),
      site(6U, 0.0, 0.0, 1.0, 1.0),
      site(7U, 1.0, 1.0, 1.0, 3.0)};
  const std::array<PointId, 6> point_candidate{6U, 5U, 4U, 3U, 2U, 1U};
  const auto point = certify_exact_bounded_power_cell_subset_closure(
      owner,
      point_competitors,
      box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0),
      point_candidate);
  check(
      point.candidate_cell.vertices.size() == 1U &&
          point.candidate_cell.vertices.front().position ==
              ExactRational3{} &&
          point.decision == ExactPowerCellSubsetClosureDecision::incomplete &&
          point.required_candidate_additions == std::vector<PointId>{7U} &&
          point.first_omitted_witness.has_value() &&
          point.first_omitted_witness->witness_kind ==
              ExactPowerCellSubsetClosureWitnessKind::
                  missing_active_incidence &&
          point.first_omitted_witness->candidate_vertex_index == 0U &&
          point.audit.exact_omitted_vertex_test_count == 1U,
      "the omitted-constraint scan retains an active incidence on an exact "
      "zero-dimensional candidate cell");
}

void test_subset_closure_symmetrized_id_orientation() {
  const Binary64WeightedSite3 left = site(10U, 0.0, 0.0, 0.0);
  const Binary64WeightedSite3 right = site(20U, 2.0, 0.0, 0.0);
  const std::array<Binary64WeightedSite3, 1> left_complete{right};
  const std::array<Binary64WeightedSite3, 1> right_complete{left};
  const std::array<PointId, 1> left_candidate{right.point_id()};
  const std::array<PointId, 1> right_candidate{left.point_id()};
  const ExactDyadicAabb3 clipping_box =
      box(-2.0, -2.0, -2.0, 2.0, 2.0, 2.0);
  const auto left_cell = certify_exact_bounded_power_cell_subset_closure(
      left, left_complete, clipping_box, left_candidate);
  const auto right_cell = certify_exact_bounded_power_cell_subset_closure(
      right, right_complete, clipping_box, right_candidate);
  check(
      left_cell.decision ==
              ExactPowerCellSubsetClosureDecision::complete_nonempty &&
          right_cell.decision ==
              ExactPowerCellSubsetClosureDecision::complete_nonempty &&
          left_cell.canonical_candidate_competitor_ids ==
              std::vector<PointId>{20U} &&
          right_cell.canonical_candidate_competitor_ids ==
              std::vector<PointId>{10U} &&
          left_cell.candidate_cell.pairwise_constraints.size() == 1U &&
          right_cell.candidate_cell.pairwise_constraints.size() == 1U,
      "a symmetrized semantic id is authenticated independently for each "
      "cell owner");
  check_opposites(
      left_cell.candidate_cell.pairwise_constraints.front()
          .owner_minus_competitor,
      right_cell.candidate_cell.pairwise_constraints.front()
          .owner_minus_competitor,
      "reciprocal semantic ids reconstruct locally oriented halfspaces");
  check(
      left_cell.candidate_cell.pairwise_constraints.front()
          .plane->same_geometric_plane(
              *right_cell.candidate_cell.pairwise_constraints.front().plane),
      "reciprocal semantic ids retain one unoriented geometric bisector");
}

void test_subset_closure_empty_and_constant_constraints() {
  const Binary64WeightedSite3 owner = site(0U, 0.0, 0.0, 0.0);
  const ExactDyadicAabb3 clipping_box =
      box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0);
  const std::array<Binary64WeightedSite3, 3> complete_competitors{
      site(3U, 0.0, 0.0, 0.0, 2.0),
      site(1U, 0.0, 0.0, 0.0, -2.0),
      site(2U, 0.0, 0.0, 0.0)};
  const std::array<PointId, 1> empty_candidate{3U};
  const auto included_empty =
      certify_exact_bounded_power_cell_subset_closure(
          owner, complete_competitors, clipping_box, empty_candidate);
  check(
      included_empty.decision ==
              ExactPowerCellSubsetClosureDecision::complete_empty &&
          included_empty.candidate_cell.decision ==
              ExactPowerCellReferenceDecision::complete_empty &&
          included_empty.audit.classified_omitted_constraint_count == 0U &&
          !included_empty.first_omitted_witness.has_value(),
      "an empty candidate cell proves the complete cell empty without an "
      "omitted scan");

  const auto omitted_empty =
      certify_exact_bounded_power_cell_subset_closure(
          owner,
          complete_competitors,
          clipping_box,
          std::span<const PointId>{});
  check(
      omitted_empty.decision ==
              ExactPowerCellSubsetClosureDecision::incomplete &&
          omitted_empty.first_omitted_witness.has_value() &&
          omitted_empty.first_omitted_witness->omitted_competitor_id == 3U &&
          omitted_empty.first_omitted_witness->witness_kind ==
              ExactPowerCellSubsetClosureWitnessKind::competitor_dominates &&
          !omitted_empty.first_omitted_witness->candidate_vertex_index
               .has_value() &&
          omitted_empty.audit.classified_omitted_constraint_count == 3U &&
          omitted_empty.audit.omitted_owner_dominates_count == 1U &&
          omitted_empty.audit.omitted_coincident_tie_count == 1U &&
          omitted_empty.audit.omitted_competitor_dominates_count == 1U &&
          omitted_empty.required_candidate_additions ==
              std::vector<PointId>{3U} &&
          omitted_empty.audit.exact_omitted_vertex_test_count == 0U,
      "omitted constant constraints distinguish redundant tie and owner "
      "dominance from an infeasible competitor dominance");
}

void test_subset_closure_preflight_and_id_contracts() {
  const Binary64WeightedSite3 owner = site(0U, 0.0, 0.0, 0.0);
  const std::array<Binary64WeightedSite3, 2> complete_competitors{
      site(2U, 2.0, 0.0, 0.0),
      site(7U, 10.0, 0.0, 0.0)};
  const ExactDyadicAabb3 clipping_box =
      box(-2.0, -2.0, -2.0, 2.0, 2.0, 2.0);
  const std::array<PointId, 1> candidate{7U};
  ExactPowerCellSubsetClosureBudget short_scan;
  short_scan.maximum_omitted_vertex_test_count = 34U;
  const auto insufficient =
      certify_exact_bounded_power_cell_subset_closure(
          owner,
          complete_competitors,
          clipping_box,
          candidate,
          short_scan);
  check(
      insufficient.decision ==
              ExactPowerCellSubsetClosureDecision::insufficient_budget &&
          insufficient.requirements.conservative_omitted_vertex_test_count ==
              35U &&
          insufficient.candidate_cell.pairwise_constraints.empty() &&
          insufficient.candidate_cell.boundary_planes.empty() &&
          insufficient.candidate_cell.vertices.empty() &&
          insufficient.audit.classified_omitted_constraint_count == 0U,
      "a one-below omitted-vertex budget returns no geometric payload");

  ExactPowerCellSubsetClosureBudget short_cell;
  short_cell.candidate_cell_budget.maximum_plane_triple_count = 34U;
  const auto insufficient_cell =
      certify_exact_bounded_power_cell_subset_closure(
          owner,
          complete_competitors,
          clipping_box,
          candidate,
          short_cell);
  check(
      insufficient_cell.decision ==
              ExactPowerCellSubsetClosureDecision::insufficient_budget &&
          insufficient_cell.candidate_cell.vertices.empty(),
      "a one-below nested candidate-cell budget fails before geometry");

  const std::array<Binary64WeightedSite3, 7> seven_competitors{
      site(1U, 1.0, 0.0, 0.0),
      site(2U, 2.0, 0.0, 0.0),
      site(3U, 3.0, 0.0, 0.0),
      site(4U, 4.0, 0.0, 0.0),
      site(5U, 5.0, 0.0, 0.0),
      site(6U, 6.0, 0.0, 0.0),
      site(7U, 7.0, 0.0, 0.0)};
  const std::array<PointId, 4> four_candidates{1U, 2U, 3U, 4U};
  ExactPowerCellSubsetClosureBudget worst_short;
  worst_short.maximum_omitted_vertex_test_count = 359U;
  const auto worst_preflight =
      certify_exact_bounded_power_cell_subset_closure(
          owner,
          seven_competitors,
          clipping_box,
          four_candidates,
          worst_short);
  check(
      worst_preflight.decision ==
              ExactPowerCellSubsetClosureDecision::insufficient_budget &&
          worst_preflight.requirements
                  .conservative_omitted_vertex_test_count == 360U,
      "the computed seven-site scan maximum is exactly 360 tests");

  const std::array<PointId, 2> duplicate_candidates{2U, 2U};
  const std::array<PointId, 3> oversized_duplicate_candidates{2U, 2U, 2U};
  check_invalid_argument_message(
      [&] {
        static_cast<void>(certify_exact_bounded_power_cell_subset_closure(
            owner,
            complete_competitors,
            clipping_box,
            oversized_duplicate_candidates));
      },
      "a power-cell candidate list cannot exceed the complete competitor "
      "table",
      "an oversized candidate list is rejected before copying or duplicate "
      "classification");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(
            certify_exact_bounded_power_cell_subset_closure(
                owner,
                complete_competitors,
                clipping_box,
                duplicate_candidates));
      },
      "duplicate candidate ids are rejected before geometry");
  const std::array<PointId, 1> unknown_candidate{5U};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(
            certify_exact_bounded_power_cell_subset_closure(
                owner,
                complete_competitors,
                clipping_box,
                unknown_candidate));
      },
      "an unknown candidate id is not treated as a floating proposal");
  const std::array<Binary64WeightedSite3, 2> duplicate_complete_ids{
      site(2U, 2.0, 0.0, 0.0),
      site(2U, 3.0, 0.0, 0.0)};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(
            certify_exact_bounded_power_cell_subset_closure(
                owner,
                duplicate_complete_ids,
                clipping_box,
                std::span<const PointId>{}));
      },
      "duplicate complete competitor ids are rejected before geometry");

  check_throws<std::invalid_argument>(
      [&] {
        ExactPowerCellSubsetClosureBudget excessive_scan;
        excessive_scan.maximum_omitted_vertex_test_count =
            ExactPowerCellSubsetClosureBudget::
                trusted_maximum_omitted_vertex_test_count +
            1U;
        static_cast<void>(
            certify_exact_bounded_power_cell_subset_closure(
                owner,
                complete_competitors,
                clipping_box,
                candidate,
                excessive_scan));
      },
      "an omitted-vertex scan cap above the trusted proof bound is rejected");

  check_throws<std::invalid_argument>(
      [&] {
        ExactPowerCellSubsetClosureBudget zero_scan;
        zero_scan.maximum_omitted_vertex_test_count = 0U;
        static_cast<void>(
            certify_exact_bounded_power_cell_subset_closure(
                owner,
                complete_competitors,
                box(-1.0, -1.0, -1.0, -1.0, 1.0, 1.0),
                candidate,
                zero_scan));
      },
      "an invalid clipping box is not masked by an insufficient budget");
  check_throws<std::invalid_argument>(
      [&] {
        ExactPowerCellSubsetClosureBudget zero_scan;
        zero_scan.maximum_omitted_vertex_test_count = 0U;
        ExactDyadicAabb3 noncanonical_box = clipping_box;
        noncanonical_box.lower_binary64_bits[0U] =
            std::uint64_t{1} << 63U;
        static_cast<void>(
            certify_exact_bounded_power_cell_subset_closure(
                owner,
                complete_competitors,
                noncanonical_box,
                candidate,
                zero_scan));
      },
      "a noncanonical binary64 box word is validated before the budget gate");
}

void test_subset_repair_completed_paths_and_final_cell() {
  const Binary64WeightedSite3 owner = site(0U, 0.0, 0.0, 0.0);
  const ExactDyadicAabb3 clipping_box =
      box(-2.0, -2.0, -2.0, 2.0, 2.0, 2.0);

  const std::array<Binary64WeightedSite3, 1> far_complete{
      site(7U, 10.0, 0.0, 0.0)};
  const auto already_nonempty =
      repair_exact_bounded_power_cell_subset_closure(
          owner,
          far_complete,
          clipping_box,
          std::span<const PointId>{});
  const ExactPowerCellReferenceResult* nonempty_final =
      already_nonempty.final_cell();
  check(
      already_nonempty.decision ==
              ExactPowerCellSubsetRepairDecision::
                  already_complete_nonempty &&
          already_nonempty.initial_closure.has_value() &&
          already_nonempty.initial_closure->decision ==
              ExactPowerCellSubsetClosureDecision::complete_nonempty &&
          already_nonempty.initial_closure->required_candidate_additions
              .empty() &&
          !already_nonempty.repaired_cell.has_value() &&
          already_nonempty.canonical_initial_candidate_competitor_ids
              .empty() &&
          already_nonempty.canonical_closed_candidate_competitor_ids
              .empty() &&
          already_nonempty.audit.exact_cell_construction_count == 1U &&
          already_nonempty.audit.omitted_constraint_scan_pass_count == 1U &&
          already_nonempty.audit
                  .post_repair_omitted_constraint_scan_pass_count == 0U &&
          already_nonempty.audit.simultaneous_repair_batch_count == 0U &&
          nonempty_final != nullptr &&
          nonempty_final ==
              &already_nonempty.initial_closure->candidate_cell &&
          nonempty_final->decision ==
              ExactPowerCellReferenceDecision::complete_nonempty,
      "a strictly redundant omitted competitor leaves a nonempty seed "
      "already closed and final_cell selects that seed");

  const std::array<Binary64WeightedSite3, 1> dominating_complete{
      site(3U, 0.0, 0.0, 0.0, 2.0)};
  const std::array<PointId, 1> dominating_candidate{3U};
  ExactPowerCellSubsetRepairBudget unused_repair_budget;
  unused_repair_budget.repaired_cell_budget.maximum_site_count = 0U;
  unused_repair_budget.repaired_cell_budget.maximum_constraint_count = 0U;
  unused_repair_budget.repaired_cell_budget.maximum_plane_triple_count = 0U;
  unused_repair_budget.repaired_cell_budget.maximum_vertex_count = 0U;
  unused_repair_budget.repaired_cell_budget.maximum_incidence_count = 0U;
  const auto already_empty =
      repair_exact_bounded_power_cell_subset_closure(
          owner,
          dominating_complete,
          clipping_box,
          dominating_candidate,
          unused_repair_budget);
  const ExactPowerCellReferenceResult* empty_final = already_empty.final_cell();
  check(
      already_empty.decision ==
              ExactPowerCellSubsetRepairDecision::already_complete_empty &&
          already_empty.initial_closure.has_value() &&
          already_empty.initial_closure->decision ==
              ExactPowerCellSubsetClosureDecision::complete_empty &&
          !already_empty.repaired_cell.has_value() &&
          already_empty.canonical_initial_candidate_competitor_ids ==
              std::vector<PointId>{3U} &&
          already_empty.canonical_closed_candidate_competitor_ids ==
              std::vector<PointId>{3U} &&
          !already_empty.requirements.conservative_repaired_cell_requirements
               .has_value() &&
          already_empty.requirements
                  .conservative_exact_cell_construction_count == 1U &&
          already_empty.requirements
                  .conservative_omitted_scan_pass_count == 0U &&
          already_empty.audit.exact_cell_construction_count == 1U &&
          already_empty.audit.omitted_constraint_scan_pass_count == 0U &&
          empty_final != nullptr &&
          empty_final == &already_empty.initial_closure->candidate_cell &&
          empty_final->decision ==
              ExactPowerCellReferenceDecision::complete_empty,
      "when f equals c an empty seed is complete and a zero second-build "
      "budget is irrelevant");
}

void test_subset_repair_simultaneous_batch_constants_and_permutation() {
  const Binary64WeightedSite3 owner = site(0U, 0.0, 0.0, 0.0);
  const ExactDyadicAabb3 clipping_box =
      box(-2.0, -2.0, -2.0, 2.0, 2.0, 2.0);
  const std::array<Binary64WeightedSite3, 2> complete_competitors{
      site(2U, 2.0, 0.0, 0.0),
      site(4U, 0.0, 4.0, 0.0)};
  const auto repaired = repair_exact_bounded_power_cell_subset_closure(
      owner,
      complete_competitors,
      clipping_box,
      std::span<const PointId>{});
  const auto complete_oracle = build_exact_bounded_power_cell_reference(
      owner, complete_competitors, clipping_box);
  const ExactPowerCellReferenceResult* repaired_final = repaired.final_cell();
  check(
      repaired.decision ==
              ExactPowerCellSubsetRepairDecision::
                  repaired_complete_nonempty &&
          repaired.initial_closure.has_value() &&
          repaired.initial_closure->decision ==
              ExactPowerCellSubsetClosureDecision::incomplete &&
          repaired.initial_closure->required_candidate_additions ==
              std::vector<PointId>({2U, 4U}) &&
          repaired.initial_closure->audit.omitted_violating_halfspace_count ==
              1U &&
          repaired.initial_closure->audit
                  .omitted_missing_active_incidence_count == 1U &&
          repaired.canonical_closed_candidate_competitor_ids ==
              std::vector<PointId>({2U, 4U}) &&
          repaired.repaired_cell.has_value() &&
          repaired.audit.exact_cell_construction_count == 2U &&
          repaired.audit.omitted_constraint_scan_pass_count == 1U &&
          repaired.audit.post_repair_omitted_constraint_scan_pass_count ==
              0U &&
          repaired.audit.simultaneous_repair_batch_count == 1U &&
          repaired.audit.simultaneously_added_competitor_count == 2U &&
          repaired_final != nullptr &&
          repaired_final == &*repaired.repaired_cell &&
          *repaired_final == complete_oracle,
      "one simultaneous batch repairs a violator and a missing active "
      "incidence, with no post-repair scan, to the complete exact oracle");

  std::array<Binary64WeightedSite3, 2> reversed = complete_competitors;
  std::reverse(reversed.begin(), reversed.end());
  const auto replay = repair_exact_bounded_power_cell_subset_closure(
      owner, reversed, clipping_box, std::span<const PointId>{});
  check(
      replay == repaired,
      "complete-table permutations preserve the canonical simultaneous "
      "repair result");

  const std::array<Binary64WeightedSite3, 2> redundant_constants{
      site(1U, 0.0, 0.0, 0.0, -2.0),
      site(2U, 0.0, 0.0, 0.0)};
  const auto constants = repair_exact_bounded_power_cell_subset_closure(
      owner,
      redundant_constants,
      clipping_box,
      std::span<const PointId>{});
  check(
      constants.decision ==
              ExactPowerCellSubsetRepairDecision::
                  already_complete_nonempty &&
          constants.initial_closure.has_value() &&
          constants.initial_closure->audit.omitted_owner_dominates_count ==
              1U &&
          constants.initial_closure->audit.omitted_coincident_tie_count ==
              1U &&
          constants.initial_closure->required_candidate_additions.empty() &&
          constants.canonical_closed_candidate_competitor_ids.empty() &&
          constants.audit.exact_cell_construction_count == 1U &&
          constants.audit.simultaneous_repair_batch_count == 0U &&
          !constants.repaired_cell.has_value(),
      "owner-dominates and coincident-tie constraints are audited but never "
      "added to a repair batch");

  const std::array<Binary64WeightedSite3, 1> infeasible_constant{
      site(3U, 0.0, 0.0, 0.0, 2.0)};
  const auto repaired_empty =
      repair_exact_bounded_power_cell_subset_closure(
          owner,
          infeasible_constant,
          clipping_box,
          std::span<const PointId>{});
  const ExactPowerCellReferenceResult* repaired_empty_final =
      repaired_empty.final_cell();
  check(
      repaired_empty.decision ==
              ExactPowerCellSubsetRepairDecision::repaired_complete_empty &&
          repaired_empty.initial_closure.has_value() &&
          repaired_empty.initial_closure->required_candidate_additions ==
              std::vector<PointId>{3U} &&
          repaired_empty.initial_closure->audit
                  .omitted_competitor_dominates_count == 1U &&
          repaired_empty.canonical_closed_candidate_competitor_ids ==
              std::vector<PointId>{3U} &&
          repaired_empty.audit.exact_cell_construction_count == 2U &&
          repaired_empty.audit.omitted_constraint_scan_pass_count == 1U &&
          repaired_empty.audit
                  .post_repair_omitted_constraint_scan_pass_count == 0U &&
          repaired_empty.audit.simultaneous_repair_batch_count == 1U &&
          repaired_empty.audit.simultaneously_added_competitor_count == 1U &&
          repaired_empty_final != nullptr &&
          repaired_empty_final->decision ==
              ExactPowerCellReferenceDecision::complete_empty,
      "an omitted competitor-dominates constant is added once and the "
      "rebuilt cell is exactly empty");
}

void test_subset_repair_atomic_preflight_and_derived_maxima() {
  const Binary64WeightedSite3 owner = site(0U, 0.0, 0.0, 0.0);
  const ExactDyadicAabb3 clipping_box =
      box(-2.0, -2.0, -2.0, 2.0, 2.0, 2.0);
  const std::array<Binary64WeightedSite3, 2> complete_competitors{
      site(2U, 2.0, 0.0, 0.0),
      site(7U, 10.0, 0.0, 0.0)};
  const std::array<PointId, 1> candidate{7U};

  ExactPowerCellSubsetRepairBudget short_closure;
  short_closure.subset_closure_budget
      .maximum_omitted_vertex_test_count = 34U;
  const auto closure_insufficient =
      repair_exact_bounded_power_cell_subset_closure(
          owner,
          complete_competitors,
          clipping_box,
          candidate,
          short_closure);
  check(
      closure_insufficient.decision ==
              ExactPowerCellSubsetRepairDecision::insufficient_budget &&
          closure_insufficient.requirements.subset_closure_requirements
                  .conservative_omitted_vertex_test_count == 35U &&
          !closure_insufficient.initial_closure.has_value() &&
          !closure_insufficient.repaired_cell.has_value() &&
          closure_insufficient.audit.exact_cell_construction_count == 0U &&
          closure_insufficient.audit.omitted_constraint_scan_pass_count ==
              0U &&
          closure_insufficient.audit.simultaneous_repair_batch_count == 0U &&
          closure_insufficient.final_cell() == nullptr,
      "a one-below closure budget fails atomically before any optional "
      "payload, scan, or cell construction");

  ExactPowerCellSubsetRepairBudget short_rebuild;
  short_rebuild.repaired_cell_budget.maximum_plane_triple_count = 55U;
  const auto rebuild_insufficient =
      repair_exact_bounded_power_cell_subset_closure(
          owner,
          complete_competitors,
          clipping_box,
          candidate,
          short_rebuild);
  check(
      rebuild_insufficient.decision ==
              ExactPowerCellSubsetRepairDecision::insufficient_budget &&
          rebuild_insufficient.requirements
                  .conservative_repaired_cell_requirements.has_value() &&
          rebuild_insufficient.requirements
                  .conservative_repaired_cell_requirements
                  ->conservative_plane_triple_count == 56U &&
          !rebuild_insufficient.initial_closure.has_value() &&
          !rebuild_insufficient.repaired_cell.has_value() &&
          rebuild_insufficient.audit.exact_cell_construction_count == 0U &&
          rebuild_insufficient.audit.omitted_constraint_scan_pass_count ==
              0U &&
          rebuild_insufficient.audit.simultaneous_repair_batch_count == 0U &&
          rebuild_insufficient.final_cell() == nullptr,
      "a one-below rebuild budget also fails before constructing the seed "
      "cell");

  const std::array<Binary64WeightedSite3, 7> seven_competitors{
      site(1U, 1.0, 0.0, 0.0),
      site(2U, 2.0, 0.0, 0.0),
      site(3U, 3.0, 0.0, 0.0),
      site(4U, 4.0, 0.0, 0.0),
      site(5U, 5.0, 0.0, 0.0),
      site(6U, 6.0, 0.0, 0.0),
      site(7U, 7.0, 0.0, 0.0)};
  const std::array<PointId, 6> six_candidates{6U, 5U, 4U, 3U, 2U, 1U};
  const auto derived_maximum =
      repair_exact_bounded_power_cell_subset_closure(
          owner,
          seven_competitors,
          clipping_box,
          six_candidates);
  check(
      ExactPowerCellSubsetRepairBudget::
              trusted_maximum_cumulative_plane_triple_count == 506U &&
          ExactPowerCellSubsetRepairBudget::
                  trusted_maximum_cumulative_vertex_count == 506U &&
          ExactPowerCellSubsetRepairBudget::
                  trusted_maximum_cumulative_incidence_count == 6358U &&
          derived_maximum.requirements.subset_closure_requirements
                  .candidate_cell_requirements
                  .conservative_plane_triple_count == 220U &&
          derived_maximum.requirements.subset_closure_requirements
                  .candidate_cell_requirements
                  .conservative_incidence_count == 2640U &&
          derived_maximum.requirements
                  .conservative_repaired_cell_requirements.has_value() &&
          derived_maximum.requirements
                  .conservative_repaired_cell_requirements
                  ->conservative_plane_triple_count == 286U &&
          derived_maximum.requirements
                  .conservative_repaired_cell_requirements
                  ->conservative_incidence_count == 3718U &&
          derived_maximum.requirements
                  .conservative_exact_cell_construction_count == 2U &&
          derived_maximum.requirements
                  .conservative_omitted_scan_pass_count == 1U &&
          derived_maximum.requirements
                  .conservative_cumulative_plane_triple_count == 506U &&
          derived_maximum.requirements
                  .conservative_cumulative_vertex_count == 506U &&
          derived_maximum.requirements
                  .conservative_cumulative_incidence_count == 6358U,
      "the f=7, c=6 preflight derives the trusted 220+286 and "
      "2640+3718 cumulative maxima");
}

void test_subset_repair_to_string_contract() {
  check(
      morsehgp3d::spatial::to_string(
          ExactPowerCellSubsetRepairDecision::already_complete_nonempty) ==
              "already_complete_nonempty" &&
          morsehgp3d::spatial::to_string(
              ExactPowerCellSubsetRepairDecision::already_complete_empty) ==
              "already_complete_empty" &&
          morsehgp3d::spatial::to_string(
              ExactPowerCellSubsetRepairDecision::repaired_complete_nonempty) ==
              "repaired_complete_nonempty" &&
          morsehgp3d::spatial::to_string(
              ExactPowerCellSubsetRepairDecision::repaired_complete_empty) ==
              "repaired_complete_empty" &&
          morsehgp3d::spatial::to_string(
              ExactPowerCellSubsetRepairDecision::insufficient_budget) ==
              "insufficient_budget",
      "every subset-repair decision has a stable diagnostic spelling");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(morsehgp3d::spatial::to_string(
            static_cast<ExactPowerCellSubsetRepairDecision>(255U)));
      },
      "an invalid subset-repair decision cannot be serialized");
}

void test_subset_closure_to_string_contract() {
  check(
      morsehgp3d::spatial::to_string(
          ExactPowerCellSubsetClosureDecision::complete_nonempty) ==
              "complete_nonempty" &&
          morsehgp3d::spatial::to_string(
              ExactPowerCellSubsetClosureDecision::complete_empty) ==
              "complete_empty" &&
          morsehgp3d::spatial::to_string(
              ExactPowerCellSubsetClosureDecision::incomplete) ==
              "incomplete" &&
          morsehgp3d::spatial::to_string(
              ExactPowerCellSubsetClosureDecision::insufficient_budget) ==
              "insufficient_budget",
      "every subset-closure decision has a stable diagnostic spelling");
  check(
      morsehgp3d::spatial::to_string(
          ExactPowerCellSubsetClosureWitnessKind::violating_halfspace) ==
              "violating_halfspace" &&
          morsehgp3d::spatial::to_string(
              ExactPowerCellSubsetClosureWitnessKind::
                  missing_active_incidence) ==
              "missing_active_incidence" &&
          morsehgp3d::spatial::to_string(
              ExactPowerCellSubsetClosureWitnessKind::competitor_dominates) ==
              "competitor_dominates",
      "every subset-closure witness has a stable diagnostic spelling");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(morsehgp3d::spatial::to_string(
            static_cast<ExactPowerCellSubsetClosureDecision>(255U)));
      },
      "an invalid subset-closure decision cannot be serialized");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(morsehgp3d::spatial::to_string(
            static_cast<ExactPowerCellSubsetClosureWitnessKind>(255U)));
      },
      "an invalid subset-closure witness cannot be serialized");
}

}  // namespace

int main() {
  test_analytic_weight_convention_and_phase2a_seam();
  test_weights_move_the_retained_halfspace();
  test_exact_cube_and_permutation();
  test_coincident_sites_fail_closed_mathematically();
  test_proper_empty_and_lower_dimensional_cells();
  test_preflight_and_input_contracts();
  test_subset_closure_repairs_and_authentic_supersets();
  test_subset_closure_active_incidence_and_lower_dimension();
  test_subset_closure_distinct_ids_and_vertex_tangency();
  test_subset_closure_proper_empty_and_all_lower_dimensions();
  test_subset_closure_symmetrized_id_orientation();
  test_subset_closure_empty_and_constant_constraints();
  test_subset_closure_preflight_and_id_contracts();
  test_subset_repair_completed_paths_and_final_cell();
  test_subset_repair_simultaneous_batch_constants_and_permutation();
  test_subset_repair_atomic_preflight_and_derived_maxima();
  test_subset_repair_to_string_contract();
  test_subset_closure_to_string_contract();
  if (failures != 0) {
    std::cerr << failures << " power-cell reference checks failed\n";
    return 1;
  }
  return 0;
}
