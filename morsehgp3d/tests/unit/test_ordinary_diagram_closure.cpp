#include "morsehgp3d/spatial/ordinary_diagram_closure.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactOrdinaryDiagramClosureBudget;
using morsehgp3d::spatial::ExactOrdinaryDiagramClosureDecision;
using morsehgp3d::spatial::ExactOrdinaryDiagramClosureRequirements;
using morsehgp3d::spatial::ExactOrdinaryDiagramClosureResult;
using morsehgp3d::spatial::ExactOrdinaryDiagramContact;
using morsehgp3d::spatial::ExactOrdinaryDiagramContactKind;
using morsehgp3d::spatial::PointId;
using morsehgp3d::spatial::StrictlyPaddedDyadicAabb3Result;
using morsehgp3d::spatial::build_exact_bounded_ordinary_diagram_closure;
using morsehgp3d::spatial::build_strictly_padded_dyadic_aabb;
using morsehgp3d::spatial::verify_exact_bounded_ordinary_diagram_closure;

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

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::vector<CertifiedPoint3>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] PointId point_id(
    const CanonicalPointCloud& cloud,
    const CertifiedPoint3& expected) {
  const auto bits = expected.canonical_input_bits();
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    const PointId id = static_cast<PointId>(index);
    if (cloud.point(id).canonical_input_bits() == bits) {
      return id;
    }
  }
  throw std::logic_error("a fixture point is absent from its canonical cloud");
}

template <std::size_t Size>
[[nodiscard]] std::vector<PointId> canonical_ids(
    const CanonicalPointCloud& cloud,
    const std::array<CertifiedPoint3, Size>& points,
    std::span<const std::size_t> indices) {
  std::vector<PointId> result;
  result.reserve(indices.size());
  for (const std::size_t index : indices) {
    result.push_back(point_id(cloud, points[index]));
  }
  std::sort(result.begin(), result.end());
  return result;
}

[[nodiscard]] const ExactOrdinaryDiagramContact* find_contact(
    const ExactOrdinaryDiagramClosureResult& result,
    const std::vector<PointId>& query_ids) {
  const auto found = std::find_if(
      result.contacts.begin(),
      result.contacts.end(),
      [&query_ids](const ExactOrdinaryDiagramContact& contact) {
        return contact.query_ids == query_ids;
      });
  return found == result.contacts.end() ? nullptr : &*found;
}

void check_complete_verification(
    const CanonicalPointCloud& cloud,
    const StrictlyPaddedDyadicAabb3Result& clipping_box,
    const ExactOrdinaryDiagramClosureResult& result,
    const std::string& message,
    ExactOrdinaryDiagramClosureBudget budget = {}) {
  const auto verification = verify_exact_bounded_ordinary_diagram_closure(
      cloud, clipping_box, result, budget);
  check(
      verification.input_identity_certified &&
          verification.clipping_box_certified &&
          verification.decision_certified &&
          verification.requirements_certified &&
          verification.audit_certified &&
          verification.payload_shape_certified &&
          verification.transcript_replay_certified &&
          verification.all_cells_freshly_verified_certified &&
          verification.all_local_queues_empty_certified &&
          verification.all_cells_full_dimensional_nonempty_certified &&
          verification.global_vertex_occurrence_bijection_certified &&
          verification.natural_incidences_reconciled_certified &&
          verification.artificial_box_boundaries_certified &&
          verification.result_certified,
      message);
}

void check_insufficient(
    const CanonicalPointCloud& cloud,
    const StrictlyPaddedDyadicAabb3Result& clipping_box,
    ExactOrdinaryDiagramClosureBudget budget,
    const std::string& message) {
  const auto result = build_exact_bounded_ordinary_diagram_closure(
      cloud, clipping_box, budget);
  const auto verification = verify_exact_bounded_ordinary_diagram_closure(
      cloud, clipping_box, result, budget);
  check(
      result.decision ==
              ExactOrdinaryDiagramClosureDecision::insufficient_budget &&
          result.cells.empty() && result.global_vertices.empty() &&
          result.contacts.empty() &&
          result.audit ==
              morsehgp3d::spatial::ExactOrdinaryDiagramClosureAudit{} &&
          verification.result_certified,
      message);
}

void check_natural_counts(
    const ExactOrdinaryDiagramClosureResult& result,
    std::size_t faces,
    std::size_t edges,
    std::size_t vertices,
    std::size_t lower_contacts,
    const std::string& message) {
  check(
      result.audit.natural_face_count == faces &&
          result.audit.natural_edge_count == edges &&
          result.audit.natural_vertex_count == vertices &&
          result.audit.noncanonical_quotient_contact_count == lower_contacts &&
          result.audit.box_supported_contact_count == 0U,
      message);
}

void test_singleton_and_pair_face() {
  const std::array<CertifiedPoint3, 1> singleton_points{
      point(0.0, 0.0, 0.0)};
  const CanonicalPointCloud singleton = canonical_cloud(singleton_points);
  const auto singleton_box = build_strictly_padded_dyadic_aabb(singleton);
  const auto singleton_result =
      build_exact_bounded_ordinary_diagram_closure(
          singleton, singleton_box);
  check(
      singleton_result.decision ==
              ExactOrdinaryDiagramClosureDecision::complete &&
          singleton_result.cells.size() == 1U &&
          singleton_result.global_vertices.size() == 8U &&
          singleton_result.contacts.empty() &&
          singleton_result.audit.final_cell_vertex_occurrence_count == 8U &&
          singleton_result.audit.global_nearest_shell_entry_count == 8U,
      "the singleton diagram is exactly its artificial clipping box");
  check_natural_counts(
      singleton_result, 0U, 0U, 0U, 0U,
      "the singleton publishes no natural contact");
  check_complete_verification(
      singleton,
      singleton_box,
      singleton_result,
      "the singleton diagram passes fresh verification");

  const std::array<CertifiedPoint3, 2> pair_points{
      point(-1.0, 0.0, 0.0), point(1.0, 0.0, 0.0)};
  const CanonicalPointCloud pair = canonical_cloud(pair_points);
  const auto pair_box = build_strictly_padded_dyadic_aabb(pair);
  const auto pair_result = build_exact_bounded_ordinary_diagram_closure(
      pair, pair_box);
  const std::array<std::size_t, 2> both_indices{0U, 1U};
  const std::vector<PointId> both =
      canonical_ids(pair, pair_points, both_indices);
  const ExactOrdinaryDiagramContact* face =
      find_contact(pair_result, both);
  check(
      face != nullptr &&
          face->kind == ExactOrdinaryDiagramContactKind::natural_face &&
          face->carrier_shell_ids == both && face->site_affine_rank == 1U &&
          face->affine_dimension == 2U &&
          face->global_vertex_indices.size() == 4U,
      "a pair has one reciprocal four-vertex natural face");
  check_natural_counts(
      pair_result, 1U, 0U, 0U, 0U,
      "the pair contact is classified only as a face");
  check_complete_verification(
      pair,
      pair_box,
      pair_result,
      "the reciprocal pair face passes fresh verification");
}

void test_triangle_and_tetrahedron_strata() {
  const std::array<CertifiedPoint3, 3> triangle_points{
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(0.0, 2.0, 0.0)};
  const CanonicalPointCloud triangle = canonical_cloud(triangle_points);
  const auto triangle_box = build_strictly_padded_dyadic_aabb(triangle);
  const auto triangle_result =
      build_exact_bounded_ordinary_diagram_closure(
          triangle, triangle_box);
  check(
      triangle_result.contacts.size() == 4U,
      "all nontrivial triangle cell intersections are materialized");
  check_natural_counts(
      triangle_result, 3U, 1U, 0U, 0U,
      "a planar triangle yields three faces and one reciprocal edge");
  const std::array<std::size_t, 3> triangle_all_indices{0U, 1U, 2U};
  const auto triangle_all =
      canonical_ids(triangle, triangle_points, triangle_all_indices);
  const ExactOrdinaryDiagramContact* triangle_edge =
      find_contact(triangle_result, triangle_all);
  check(
      triangle_edge != nullptr &&
          triangle_edge->kind ==
              ExactOrdinaryDiagramContactKind::natural_edge &&
          triangle_edge->site_affine_rank == 2U &&
          triangle_edge->affine_dimension == 1U &&
          triangle_edge->global_vertex_indices.size() == 2U,
      "the triangle co-shell has exact rank two and a clipped line edge");
  check_complete_verification(
      triangle,
      triangle_box,
      triangle_result,
      "the triangle diagram passes fresh verification");

  const std::array<CertifiedPoint3, 4> tetra_points{
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)};
  const CanonicalPointCloud tetra = canonical_cloud(tetra_points);
  const auto tetra_box = build_strictly_padded_dyadic_aabb(tetra);
  const auto tetra_result = build_exact_bounded_ordinary_diagram_closure(
      tetra, tetra_box);
  check(
      tetra_result.contacts.size() == 11U,
      "the tetrahedron materializes every nontrivial cell intersection");
  check_natural_counts(
      tetra_result, 6U, 4U, 1U, 0U,
      "a tetrahedron yields six faces, four edges and one vertex");
  const std::array<std::size_t, 4> tetra_all_indices{0U, 1U, 2U, 3U};
  const auto tetra_all =
      canonical_ids(tetra, tetra_points, tetra_all_indices);
  const ExactOrdinaryDiagramContact* tetra_vertex =
      find_contact(tetra_result, tetra_all);
  check(
      tetra_vertex != nullptr &&
          tetra_vertex->kind ==
              ExactOrdinaryDiagramContactKind::natural_vertex &&
          tetra_vertex->site_affine_rank == 3U &&
          tetra_vertex->affine_dimension == 0U &&
          tetra_vertex->global_vertex_indices.size() == 1U,
      "the tetrahedral co-shell remains one reciprocal exact vertex");
  check_complete_verification(
      tetra,
      tetra_box,
      tetra_result,
      "the tetrahedral diagram passes fresh verification");
}

void test_sparse_collinear_contacts() {
  const std::array<CertifiedPoint3, 4> points{
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(4.0, 0.0, 0.0),
      point(6.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const auto clipping_box = build_strictly_padded_dyadic_aabb(cloud);
  const auto result = build_exact_bounded_ordinary_diagram_closure(
      cloud, clipping_box);
  check(
      result.contacts.size() == 3U,
      "a collinear diagram materializes only its three adjacent contacts");
  check_natural_counts(
      result, 3U, 0U, 0U, 0U,
      "four collinear sites yield three slab faces and no spurious contact");
  const std::array<std::size_t, 2> nonadjacent_indices{0U, 2U};
  const auto nonadjacent = canonical_ids(cloud, points, nonadjacent_indices);
  check(
      find_contact(result, nonadjacent) == nullptr,
      "a nonadjacent collinear pair has an empty common cell intersection");
  check_complete_verification(
      cloud,
      clipping_box,
      result,
      "the sparse collinear diagram passes fresh verification");
}

void test_box_supported_contact_is_not_natural() {
  constexpr double base = 0x1p52;
  const std::array<CertifiedPoint3, 3> points{
      point(-2.0, base + 1.0, 0.0),
      point(2.0, base + 1.0, 0.0),
      point(-1.0, base + 2.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const auto clipping_box = build_strictly_padded_dyadic_aabb(cloud);
  const auto result = build_exact_bounded_ordinary_diagram_closure(
      cloud, clipping_box);
  const std::array<std::size_t, 3> all_indices{0U, 1U, 2U};
  const auto all = canonical_ids(cloud, points, all_indices);
  const ExactOrdinaryDiagramContact* contact = find_contact(result, all);
  check(
      contact != nullptr &&
          contact->kind ==
              ExactOrdinaryDiagramContactKind::box_supported_contact &&
          contact->common_artificial_box_face_mask == UINT8_C(4) &&
          contact->site_affine_rank == 2U &&
          contact->affine_dimension == 1U &&
          result.audit.box_supported_contact_count == 1U &&
          result.audit.natural_edge_count == 0U,
      "a shell-three line carried by lower-y is artificial, never a natural "
      "edge");
  check_complete_verification(
      cloud,
      clipping_box,
      result,
      "the box-supported contact passes fresh verification");
}

struct SquareFixture {
  std::array<CertifiedPoint3, 4> points;
  CanonicalPointCloud cloud;
  StrictlyPaddedDyadicAabb3Result clipping_box;
  ExactOrdinaryDiagramClosureResult result;
};

[[nodiscard]] SquareFixture square_fixture() {
  const std::array<CertifiedPoint3, 4> points{
      point(-1.0, -1.0, 0.0),
      point(1.0, -1.0, 0.0),
      point(-1.0, 1.0, 0.0),
      point(1.0, 1.0, 0.0)};
  CanonicalPointCloud cloud = canonical_cloud(points);
  StrictlyPaddedDyadicAabb3Result clipping_box =
      build_strictly_padded_dyadic_aabb(cloud);
  ExactOrdinaryDiagramClosureResult result =
      build_exact_bounded_ordinary_diagram_closure(cloud, clipping_box);
  return SquareFixture{
      points, std::move(cloud), std::move(clipping_box), std::move(result)};
}

void test_cocircular_square_quotients_false_faces() {
  SquareFixture fixture = square_fixture();
  check(
      fixture.result.contacts.size() == 11U,
      "the square keeps all eleven nontrivial contacts auditable");
  check_natural_counts(
      fixture.result, 4U, 1U, 0U, 6U,
      "the square quotients diagonal and triple contacts into one shell-four "
      "edge");

  const std::array<std::size_t, 2> diagonal_indices{0U, 3U};
  const auto diagonal =
      canonical_ids(fixture.cloud, fixture.points, diagonal_indices);
  const ExactOrdinaryDiagramContact* diagonal_contact =
      find_contact(fixture.result, diagonal);
  const std::array<std::size_t, 4> all_indices{0U, 1U, 2U, 3U};
  const auto all = canonical_ids(fixture.cloud, fixture.points, all_indices);
  const ExactOrdinaryDiagramContact* carrier =
      find_contact(fixture.result, all);
  check(
      diagonal_contact != nullptr && carrier != nullptr &&
          diagonal_contact->kind ==
              ExactOrdinaryDiagramContactKind::noncanonical_quotient_contact &&
          diagonal_contact->carrier_shell_ids == all &&
          diagonal_contact->global_vertex_indices ==
              carrier->global_vertex_indices &&
          carrier->kind == ExactOrdinaryDiagramContactKind::natural_edge &&
          carrier->site_affine_rank == 2U &&
          carrier->affine_dimension == 1U,
      "a diagonal square pair maps to the shell-four carrier and never to a "
      "false face");
  check_complete_verification(
      fixture.cloud,
      fixture.clipping_box,
      fixture.result,
      "the cocircular square passes fresh reciprocal verification");

  std::vector<CertifiedPoint3> reversed{
      fixture.points.rbegin(), fixture.points.rend()};
  const CanonicalPointCloud permuted_cloud = canonical_cloud(reversed);
  const auto permuted_box =
      build_strictly_padded_dyadic_aabb(permuted_cloud);
  const auto permuted_result =
      build_exact_bounded_ordinary_diagram_closure(
          permuted_cloud, permuted_box);
  check(
      fixture.result == permuted_result,
      "input permutation preserves the canonical full diagram transcript");
}

struct CubeFixture {
  CanonicalPointCloud cloud;
  StrictlyPaddedDyadicAabb3Result clipping_box;
};

[[nodiscard]] CubeFixture cube_fixture() {
  const std::array<CertifiedPoint3, 8> points{
      point(-1.0, -1.0, -1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0),
      point(1.0, 1.0, -1.0),
      point(1.0, -1.0, 1.0),
      point(-1.0, 1.0, 1.0),
      point(1.0, 1.0, 1.0)};
  CanonicalPointCloud cloud = canonical_cloud(points);
  return CubeFixture{cloud, build_strictly_padded_dyadic_aabb(cloud)};
}

void test_cube_shell_eight_and_atomic_budget() {
  CubeFixture fixture = cube_fixture();
  const auto result = build_exact_bounded_ordinary_diagram_closure(
      fixture.cloud, fixture.clipping_box);
  check(
      result.contacts.size() == 247U &&
          result.audit.contact_count == 247U &&
          result.audit.exact_stratum_witness_query_count == 247U &&
          result.audit.exact_stratum_witness_distance_evaluation_count ==
              1976U,
      "the cube audits every nontrivial contact and every exact barycenter "
      "shell");
  check_natural_counts(
      result, 12U, 6U, 1U, 228U,
      "the cube keeps twelve faces, six shell-four edges and one shell-eight "
      "vertex without triangulation");

  const auto center = std::find_if(
      result.global_vertices.begin(),
      result.global_vertices.end(),
      [](const auto& vertex) { return vertex.position == ExactRational3{}; });
  check(
      center != result.global_vertices.end() &&
          center->complete_nearest_shell_ids.size() == 8U &&
          center->cell_occurrences.size() == 8U &&
          center->artificial_box_face_mask == 0U,
      "the cube center is one shell-eight reciprocal global vertex");
  check_complete_verification(
      fixture.cloud,
      fixture.clipping_box,
      result,
      "the cube diagram passes fresh verification");

  const ExactOrdinaryDiagramClosureRequirements& requirements =
      result.requirements;
  check(
      requirements.conservative_cell_count == 8U &&
          requirements.conservative_cell_construction_count == 56U &&
          requirements.conservative_cumulative_plane_triple_count == 7728U &&
          requirements.conservative_cumulative_vertex_count == 7728U &&
          requirements.conservative_cumulative_incidence_count == 86576U &&
          requirements.conservative_vertex_query_count == 7728U &&
          requirements.conservative_exact_distance_evaluation_count ==
              61824U &&
          requirements.conservative_nearest_shell_entry_count == 61824U &&
          requirements.conservative_owner_strict_feasibility_test_count ==
              56U &&
          requirements.conservative_simultaneous_addition_batch_count ==
              48U &&
          requirements.conservative_total_simultaneous_addition_count ==
              48U &&
          requirements.conservative_maximum_simultaneous_batch_size == 6U &&
          requirements.conservative_final_cell_vertex_occurrence_count ==
              2288U &&
          requirements.conservative_global_vertex_count == 2288U &&
          requirements.conservative_global_nearest_shell_entry_count ==
              18304U &&
          requirements.conservative_contact_count == 247U &&
          requirements.conservative_contact_query_id_count == 1016U &&
          requirements.conservative_contact_carrier_shell_id_count == 1976U &&
          requirements.conservative_contact_vertex_reference_count ==
              565136U &&
          requirements.conservative_stratum_witness_query_count == 247U &&
          requirements
                  .conservative_stratum_witness_exact_distance_evaluation_count ==
              1976U,
      "the cube derives every global n8 proof cap exactly");

  using BudgetMember = std::size_t ExactOrdinaryDiagramClosureBudget::*;
  using RequirementMember =
      std::size_t ExactOrdinaryDiagramClosureRequirements::*;
  struct CapCase {
    BudgetMember budget_member;
    RequirementMember requirement_member;
    const char* name;
  };
  const std::array<CapCase, 21> cap_cases{{
      {&ExactOrdinaryDiagramClosureBudget::maximum_cell_count,
       &ExactOrdinaryDiagramClosureRequirements::conservative_cell_count,
       "cell"},
      {&ExactOrdinaryDiagramClosureBudget::maximum_cell_construction_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_cell_construction_count,
       "construction"},
      {&ExactOrdinaryDiagramClosureBudget::
           maximum_cumulative_plane_triple_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_cumulative_plane_triple_count,
       "plane triple"},
      {&ExactOrdinaryDiagramClosureBudget::maximum_cumulative_vertex_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_cumulative_vertex_count,
       "vertex"},
      {&ExactOrdinaryDiagramClosureBudget::maximum_cumulative_incidence_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_cumulative_incidence_count,
       "incidence"},
      {&ExactOrdinaryDiagramClosureBudget::maximum_vertex_query_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_vertex_query_count,
       "query"},
      {&ExactOrdinaryDiagramClosureBudget::
           maximum_exact_distance_evaluation_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_exact_distance_evaluation_count,
       "distance"},
      {&ExactOrdinaryDiagramClosureBudget::maximum_nearest_shell_entry_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_nearest_shell_entry_count,
       "local shell"},
      {&ExactOrdinaryDiagramClosureBudget::
           maximum_owner_strict_feasibility_test_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_owner_strict_feasibility_test_count,
       "strict owner"},
      {&ExactOrdinaryDiagramClosureBudget::
           maximum_simultaneous_addition_batch_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_simultaneous_addition_batch_count,
       "addition batch"},
      {&ExactOrdinaryDiagramClosureBudget::
           maximum_total_simultaneous_addition_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_total_simultaneous_addition_count,
       "total addition"},
      {&ExactOrdinaryDiagramClosureBudget::
           maximum_simultaneous_batch_size,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_maximum_simultaneous_batch_size,
       "addition batch size"},
      {&ExactOrdinaryDiagramClosureBudget::
           maximum_final_cell_vertex_occurrence_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_final_cell_vertex_occurrence_count,
       "occurrence"},
      {&ExactOrdinaryDiagramClosureBudget::maximum_global_vertex_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_global_vertex_count,
       "global vertex"},
      {&ExactOrdinaryDiagramClosureBudget::
           maximum_global_nearest_shell_entry_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_global_nearest_shell_entry_count,
       "global shell"},
      {&ExactOrdinaryDiagramClosureBudget::maximum_contact_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_contact_count,
       "contact"},
      {&ExactOrdinaryDiagramClosureBudget::maximum_contact_query_id_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_contact_query_id_count,
       "contact query id"},
      {&ExactOrdinaryDiagramClosureBudget::
           maximum_contact_carrier_shell_id_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_contact_carrier_shell_id_count,
       "carrier shell id"},
      {&ExactOrdinaryDiagramClosureBudget::
           maximum_contact_vertex_reference_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_contact_vertex_reference_count,
       "contact vertex reference"},
      {&ExactOrdinaryDiagramClosureBudget::
           maximum_stratum_witness_query_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_stratum_witness_query_count,
       "witness query"},
      {&ExactOrdinaryDiagramClosureBudget::
           maximum_stratum_witness_exact_distance_evaluation_count,
       &ExactOrdinaryDiagramClosureRequirements::
           conservative_stratum_witness_exact_distance_evaluation_count,
       "witness distance"},
  }};
  for (const CapCase& cap_case : cap_cases) {
    ExactOrdinaryDiagramClosureBudget short_budget;
    short_budget.*(cap_case.budget_member) =
        requirements.*(cap_case.requirement_member) - 1U;
    check_insufficient(
        fixture.cloud,
        fixture.clipping_box,
        short_budget,
        std::string{"a one-below "} + cap_case.name +
            " cap fails before the first cell");
    ExactOrdinaryDiagramClosureBudget excessive_budget;
    excessive_budget.*(cap_case.budget_member) =
        requirements.*(cap_case.requirement_member) + 1U;
    check_throws<std::invalid_argument>(
        [&fixture, excessive_budget] {
          static_cast<void>(build_exact_bounded_ordinary_diagram_closure(
              fixture.cloud, fixture.clipping_box, excessive_budget));
        },
        std::string{"an above-trust "} + cap_case.name +
            " cap is rejected");
  }
}

void test_fresh_verifier_rejects_hostile_mutations() {
  SquareFixture fixture = square_fixture();
  check_complete_verification(
      fixture.cloud,
      fixture.clipping_box,
      fixture.result,
      "the mutation base is valid");
  const auto rejected = [&fixture](
                            const ExactOrdinaryDiagramClosureResult& candidate) {
    return !verify_exact_bounded_ordinary_diagram_closure(
                fixture.cloud,
                fixture.clipping_box,
                candidate)
                .result_certified;
  };

  auto bad_decision = fixture.result;
  bad_decision.decision =
      ExactOrdinaryDiagramClosureDecision::insufficient_budget;
  check(rejected(bad_decision), "a mutated diagram decision is rejected");
  auto bad_manifest = fixture.result;
  ++bad_manifest.canonical_point_bits.front()[0];
  check(rejected(bad_manifest), "a mutated canonical cloud manifest is rejected");
  auto missing_cell = fixture.result;
  missing_cell.cells.pop_back();
  check(rejected(missing_cell), "a missing owner cell is rejected");
  auto bad_local_owner = fixture.result;
  ++bad_local_owner.cells.front().owner_id;
  check(rejected(bad_local_owner), "a mutated local owner is rejected");
  auto bad_occurrence = fixture.result;
  bad_occurrence.global_vertices.front().cell_occurrences.front().owner_id =
      PointId{999U};
  check(
      rejected(bad_occurrence),
      "an out-of-range untrusted occurrence owner is rejected safely");
  auto bad_global_shell = fixture.result;
  bad_global_shell.global_vertices.front().complete_nearest_shell_ids.push_back(
      PointId{999U});
  check(
      rejected(bad_global_shell),
      "an out-of-range untrusted global shell id is rejected safely");
  auto bad_contact_index = fixture.result;
  bad_contact_index.contacts.front().global_vertex_indices.front() = 999U;
  check(
      rejected(bad_contact_index),
      "an out-of-range untrusted contact vertex index is rejected safely");
  auto false_face = fixture.result;
  const auto lower_contact = std::find_if(
      false_face.contacts.begin(),
      false_face.contacts.end(),
      [](const ExactOrdinaryDiagramContact& contact) {
        return contact.kind ==
               ExactOrdinaryDiagramContactKind::
                   noncanonical_quotient_contact;
      });
  check(lower_contact != false_face.contacts.end(),
        "the mutation base contains a lower-dimensional contact");
  if (lower_contact != false_face.contacts.end()) {
    lower_contact->kind = ExactOrdinaryDiagramContactKind::natural_face;
  }
  check(rejected(false_face), "a fabricated diagonal face is rejected");
  auto bad_mask = fixture.result;
  bad_mask.global_vertices.front().artificial_box_face_mask ^= UINT8_C(1);
  check(rejected(bad_mask), "a flipped artificial support bit is rejected");
  auto bad_audit = fixture.result;
  ++bad_audit.audit.natural_edge_count;
  check(rejected(bad_audit), "a mutated global audit is rejected");
  auto bad_claim = fixture.result;
  bad_claim.natural_incidences_reconciled_certified = false;
  check(rejected(bad_claim), "a cleared reconciliation claim is rejected");
  auto bad_box = fixture.result;
  ++bad_box.clipping_box.certificate->omega.upper_binary64_bits[0];
  check(rejected(bad_box), "a mutated clipping box is rejected");

  const std::array<CertifiedPoint3, 4> changed_points{
      point(-1.0, -1.0, 0.0),
      point(1.0, -1.0, 0.0),
      point(-1.0, 1.0, 0.0),
      point(2.0, 1.0, 0.0)};
  const CanonicalPointCloud changed_cloud = canonical_cloud(changed_points);
  const auto changed_box = build_strictly_padded_dyadic_aabb(changed_cloud);
  check(
      !verify_exact_bounded_ordinary_diagram_closure(
           changed_cloud, changed_box, fixture.result)
           .result_certified,
      "a diagram transcript cannot be replayed against another cloud");
}

void test_insufficient_receipt_binds_exact_cloud() {
  const std::array<CertifiedPoint3, 4> first_points{
      point(-2.0, -2.0, -2.0),
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(2.0, 2.0, 2.0)};
  const std::array<CertifiedPoint3, 4> second_points{
      point(-2.0, -2.0, -2.0),
      point(0.0, 0.0, 0.0),
      point(0.5, 0.0, 0.0),
      point(2.0, 2.0, 2.0)};
  const CanonicalPointCloud first_cloud = canonical_cloud(first_points);
  const CanonicalPointCloud second_cloud = canonical_cloud(second_points);
  const auto first_box = build_strictly_padded_dyadic_aabb(first_cloud);
  const auto second_box = build_strictly_padded_dyadic_aabb(second_cloud);
  check(
      first_box == second_box,
      "the receipt fixture changes an interior point while preserving its "
      "certified box");

  ExactOrdinaryDiagramClosureBudget short_budget;
  short_budget.maximum_cell_count = 3U;
  const auto receipt = build_exact_bounded_ordinary_diagram_closure(
      first_cloud, first_box, short_budget);
  check(
      receipt.decision ==
              ExactOrdinaryDiagramClosureDecision::insufficient_budget &&
          verify_exact_bounded_ordinary_diagram_closure(
              first_cloud, first_box, receipt, short_budget)
              .result_certified &&
          !verify_exact_bounded_ordinary_diagram_closure(
               second_cloud, second_box, receipt, short_budget)
               .result_certified,
      "an insufficient receipt is bound to every canonical point bit, not "
      "only to n and the clipping box");
}

void test_invalid_inputs_and_strings() {
  SquareFixture fixture = square_fixture();
  auto bad_box = fixture.clipping_box;
  bad_box.certificate->omega.lower_binary64_bits[0] =
      bad_box.certificate->exact_site_aabb.bounds.lower_binary64_bits[0];
  check_throws<std::invalid_argument>(
      [&fixture, &bad_box] {
        static_cast<void>(build_exact_bounded_ordinary_diagram_closure(
            fixture.cloud, bad_box));
      },
      "a non-strict clipping box is rejected before any cell");

  std::vector<CertifiedPoint3> nine_points;
  for (int value = 0; value < 9; ++value) {
    nine_points.push_back(point(static_cast<double>(value), 0.0, 0.0));
  }
  const CanonicalPointCloud nine_cloud = canonical_cloud(nine_points);
  const auto nine_box = build_strictly_padded_dyadic_aabb(nine_cloud);
  check_throws<std::invalid_argument>(
      [&nine_cloud, &nine_box] {
        static_cast<void>(build_exact_bounded_ordinary_diagram_closure(
            nine_cloud, nine_box));
      },
      "the bounded global diagram rejects more than eight sites");

  check(
      morsehgp3d::spatial::to_string(
          ExactOrdinaryDiagramClosureDecision::complete) == "complete" &&
          morsehgp3d::spatial::to_string(
              ExactOrdinaryDiagramClosureDecision::insufficient_budget) ==
              "insufficient_budget" &&
          morsehgp3d::spatial::to_string(
              ExactOrdinaryDiagramContactKind::
                  noncanonical_quotient_contact) ==
              "noncanonical_quotient_contact" &&
          morsehgp3d::spatial::to_string(
              ExactOrdinaryDiagramContactKind::natural_face) ==
              "natural_face" &&
          morsehgp3d::spatial::to_string(
              ExactOrdinaryDiagramContactKind::natural_edge) ==
              "natural_edge" &&
          morsehgp3d::spatial::to_string(
              ExactOrdinaryDiagramContactKind::natural_vertex) ==
              "natural_vertex" &&
          morsehgp3d::spatial::to_string(
              ExactOrdinaryDiagramContactKind::box_supported_contact) ==
              "box_supported_contact",
      "diagram decisions and contact kinds have stable strings");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(morsehgp3d::spatial::to_string(
            static_cast<ExactOrdinaryDiagramClosureDecision>(255)));
      },
      "an invalid diagram decision cannot be stringified");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(morsehgp3d::spatial::to_string(
            static_cast<ExactOrdinaryDiagramContactKind>(255)));
      },
      "an invalid contact kind cannot be stringified");
}

}  // namespace

int main() {
  test_singleton_and_pair_face();
  test_triangle_and_tetrahedron_strata();
  test_sparse_collinear_contacts();
  test_box_supported_contact_is_not_natural();
  test_cocircular_square_quotients_false_faces();
  test_cube_shell_eight_and_atomic_budget();
  test_fresh_verifier_rejects_hostile_mutations();
  test_insufficient_receipt_binds_exact_cloud();
  test_invalid_inputs_and_strings();

  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "all exact ordinary-diagram closure tests passed\n";
  return 0;
}
