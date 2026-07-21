#include "morsehgp3d/spatial/ordinary_cell_closure.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
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
using morsehgp3d::spatial::ExactOrdinaryCellClosureBudget;
using morsehgp3d::spatial::ExactOrdinaryCellClosureDecision;
using morsehgp3d::spatial::ExactOrdinaryCellClosureResult;
using morsehgp3d::spatial::ExactOrdinaryCellVertexClassification;
using morsehgp3d::spatial::ExactPowerCellReferenceDecision;
using morsehgp3d::spatial::PointId;
using morsehgp3d::spatial::PowerCellBoundaryKind;
using morsehgp3d::spatial::StrictDyadicPaddingDecision;
using morsehgp3d::spatial::StrictlyPaddedDyadicAabb3Result;
using morsehgp3d::spatial::build_exact_bounded_ordinary_cell_closure;
using morsehgp3d::spatial::build_strictly_padded_dyadic_aabb;
using morsehgp3d::spatial::verify_exact_bounded_ordinary_cell_closure;

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
  const auto expected_bits = expected.canonical_input_bits();
  for (std::size_t point_index = 0U;
       point_index < cloud.size();
       ++point_index) {
    const PointId candidate = static_cast<PointId>(point_index);
    if (cloud.point(candidate).canonical_input_bits() == expected_bits) {
      return candidate;
    }
  }
  throw std::logic_error("the test point is absent from its canonical cloud");
}

void check_complete_verification(
    const CanonicalPointCloud& cloud,
    PointId owner_id,
    const StrictlyPaddedDyadicAabb3Result& clipping_box,
    std::span<const PointId> initial_ids,
    const ExactOrdinaryCellClosureResult& result,
    const std::string& message,
    ExactOrdinaryCellClosureBudget budget = {}) {
  const auto verification = verify_exact_bounded_ordinary_cell_closure(
      cloud, owner_id, clipping_box, initial_ids, result, budget);
  check(
      verification.input_identity_certified &&
          verification.clipping_box_certified &&
          verification.decision_certified &&
          verification.requirements_certified &&
          verification.audit_certified &&
          verification.payload_shape_certified &&
          verification.transcript_replay_certified &&
          verification.monotone_queue_certified &&
          verification.owner_strict_feasible_certified &&
          verification.full_dimensional_nonempty_certified &&
          verification.active_nearest_shells_reconciled_certified &&
          verification.artificial_box_boundaries_certified &&
          verification.complete_oracle_projection_certified &&
          verification.result_certified,
      message);
}

void check_insufficient(
    const CanonicalPointCloud& cloud,
    PointId owner_id,
    const StrictlyPaddedDyadicAabb3Result& clipping_box,
    std::span<const PointId> initial_ids,
    ExactOrdinaryCellClosureBudget budget,
    const std::string& message) {
  const auto result = build_exact_bounded_ordinary_cell_closure(
      cloud, owner_id, clipping_box, initial_ids, budget);
  const auto verification = verify_exact_bounded_ordinary_cell_closure(
      cloud, owner_id, clipping_box, initial_ids, result, budget);
  check(
      result.decision ==
              ExactOrdinaryCellClosureDecision::insufficient_budget &&
          result.rounds.empty() && result.final_cell() == nullptr &&
          result.audit == morsehgp3d::spatial::
                              ExactOrdinaryCellClosureAudit{} &&
          verification.result_certified,
      message);
}

struct CubeFixture {
  CanonicalPointCloud cloud;
  PointId owner_id;
  std::vector<PointId> axial_seed_ids;
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
  const PointId owner_id = point_id(cloud, points[0]);
  std::vector<PointId> seed{
      point_id(cloud, points[3]),
      point_id(cloud, points[1]),
      point_id(cloud, points[2])};
  return CubeFixture{
      cloud,
      owner_id,
      std::move(seed),
      build_strictly_padded_dyadic_aabb(cloud)};
}

void test_singleton_and_canonical_fallback_seed() {
  const std::array<CertifiedPoint3, 1> singleton_points{
      point(0.0, 0.0, 0.0)};
  const CanonicalPointCloud singleton = canonical_cloud(singleton_points);
  const auto singleton_box =
      build_strictly_padded_dyadic_aabb(singleton);
  const std::array<PointId, 0> empty_seed{};
  const auto singleton_cell = build_exact_bounded_ordinary_cell_closure(
      singleton, 0U, singleton_box, empty_seed);
  check(
      singleton_cell.decision ==
              ExactOrdinaryCellClosureDecision::complete_nonempty &&
          !singleton_cell.fallback_seed_injected &&
          singleton_cell.complete_competitor_ids.empty() &&
          singleton_cell.canonical_closed_competitor_ids.empty() &&
          singleton_cell.rounds.size() == 1U &&
          singleton_cell.rounds.front().candidate_cell.vertices.size() == 8U &&
          singleton_cell.rounds.front().candidate_cell.boundary_planes.size() ==
              6U &&
          singleton_cell.audit.exact_vertex_query_count == 8U &&
          singleton_cell.audit.exact_distance_evaluation_count == 8U &&
          singleton_cell.audit.owner_strict_vertex_count == 8U,
      "the singleton closes directly as the padded box");
  check_complete_verification(
      singleton,
      0U,
      singleton_box,
      empty_seed,
      singleton_cell,
      "the singleton cell passes fresh verification");

  const std::array<CertifiedPoint3, 2> pair_points{
      point(-1.0, 0.0, 0.0), point(1.0, 0.0, 0.0)};
  const CanonicalPointCloud pair = canonical_cloud(pair_points);
  const PointId owner_id = point_id(pair, pair_points[0]);
  const PointId competitor_id = point_id(pair, pair_points[1]);
  const auto pair_box = build_strictly_padded_dyadic_aabb(pair);
  const auto pair_cell = build_exact_bounded_ordinary_cell_closure(
      pair, owner_id, pair_box, empty_seed);
  check(
      pair_cell.fallback_seed_injected &&
          pair_cell.canonical_requested_initial_competitor_ids.empty() &&
          pair_cell.canonical_effective_initial_competitor_ids ==
              std::vector<PointId>{competitor_id} &&
          pair_cell.canonical_closed_competitor_ids ==
              std::vector<PointId>{competitor_id} &&
          pair_cell.rounds.size() == 1U &&
          pair_cell.audit.owner_tie_vertex_count == 4U &&
          pair_cell.audit.missing_active_vertex_count == 0U,
      "an empty pair proposal records the least exterior fallback and its "
      "natural bisector face");
  check_complete_verification(
      pair,
      owner_id,
      pair_box,
      empty_seed,
      pair_cell,
      "the fallback-seeded pair passes fresh verification");
}

void test_cube_reconciles_face_edge_and_vertex_shells() {
  CubeFixture fixture = cube_fixture();
  const auto result = build_exact_bounded_ordinary_cell_closure(
      fixture.cloud,
      fixture.owner_id,
      fixture.clipping_box,
      fixture.axial_seed_ids);
  check(
      result.decision ==
              ExactOrdinaryCellClosureDecision::complete_nonempty &&
          result.rounds.size() == 2U &&
          result.rounds.front().simultaneously_added_competitor_ids.size() ==
              4U &&
          !result.rounds.front().local_queue_empty &&
          result.rounds.back().local_queue_empty &&
          result.canonical_closed_competitor_ids ==
              result.complete_competitor_ids &&
          result.audit.simultaneous_addition_batch_count == 1U &&
          result.audit.simultaneously_added_competitor_count == 4U &&
          result.audit.maximum_simultaneous_addition_count == 4U,
      "the cube adds all four diagonal co-incidences in one frozen batch");

  bool saw_face_shell = false;
  bool saw_edge_shell = false;
  bool saw_vertex_shell = false;
  bool saw_missing_active = false;
  for (const auto& query : result.rounds.front().vertex_queries) {
    saw_face_shell =
        saw_face_shell || query.complete_nearest_shell_ids.size() == 2U;
    saw_edge_shell =
        saw_edge_shell || query.complete_nearest_shell_ids.size() == 4U;
    saw_vertex_shell =
        saw_vertex_shell || query.complete_nearest_shell_ids.size() == 8U;
    saw_missing_active =
        saw_missing_active ||
        query.classification ==
            ExactOrdinaryCellVertexClassification::
                missing_active_nearest_shell;
  }
  check(
      saw_face_shell && saw_edge_shell && saw_vertex_shell &&
          saw_missing_active,
      "complete co-nearest shells expose natural face, edge and vertex "
      "incidences without canonical-choice truncation");
  check_complete_verification(
      fixture.cloud,
      fixture.owner_id,
      fixture.clipping_box,
      fixture.axial_seed_ids,
      result,
      "the cube shell closure passes full-oracle projection replay");

  std::reverse(
      fixture.axial_seed_ids.begin(), fixture.axial_seed_ids.end());
  const auto reversed = build_exact_bounded_ordinary_cell_closure(
      fixture.cloud,
      fixture.owner_id,
      fixture.clipping_box,
      fixture.axial_seed_ids);
  check(result == reversed, "seed permutation preserves the exact transcript");
}

void test_hidden_seed_violator_and_tangent_coincidence() {
  const std::array<CertifiedPoint3, 3> line_points{
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(10.0, 0.0, 0.0)};
  const CanonicalPointCloud line_cloud = canonical_cloud(line_points);
  const PointId line_owner = point_id(line_cloud, line_points[0]);
  const PointId near_id = point_id(line_cloud, line_points[1]);
  const PointId far_id = point_id(line_cloud, line_points[2]);
  const auto line_box = build_strictly_padded_dyadic_aabb(line_cloud);
  const std::array<PointId, 1> bad_seed{far_id};
  const auto repaired = build_exact_bounded_ordinary_cell_closure(
      line_cloud, line_owner, line_box, bad_seed);
  check(
      repaired.rounds.size() == 2U &&
          repaired.rounds.front().simultaneously_added_competitor_ids ==
              std::vector<PointId>{near_id} &&
          repaired.audit.violating_vertex_count != 0U &&
          repaired.canonical_closed_competitor_ids ==
              std::vector<PointId>({near_id, far_id}),
      "a site absent from a bad far seed is recovered at an exact violating "
      "vertex while the authentic over-seed is retained");
  check_complete_verification(
      line_cloud,
      line_owner,
      line_box,
      bad_seed,
      repaired,
      "the hidden-seed violator passes fresh replay");

  const std::array<PointId, 1> near_seed{near_id};
  const auto closed_without_redundant_far_site =
      build_exact_bounded_ordinary_cell_closure(
          line_cloud, line_owner, line_box, near_seed);
  check(
      closed_without_redundant_far_site.rounds.size() == 1U &&
          closed_without_redundant_far_site
                  .canonical_closed_competitor_ids ==
              std::vector<PointId>{near_id} &&
          closed_without_redundant_far_site.complete_competitor_ids ==
              std::vector<PointId>({near_id, far_id}),
      "a strictly redundant omitted site need not enter the closed seed");
  check_complete_verification(
      line_cloud,
      line_owner,
      line_box,
      near_seed,
      closed_without_redundant_far_site,
      "a proper closed subset matches the normalized full oracle");

  const std::array<CertifiedPoint3, 5> tangent_points{
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(0.0, 2.0, 0.0),
      point(0.0, 0.0, 2.0),
      point(2.0, 2.0, 2.0)};
  const CanonicalPointCloud tangent_cloud = canonical_cloud(tangent_points);
  const PointId tangent_owner = point_id(tangent_cloud, tangent_points[0]);
  const PointId diagonal_id = point_id(tangent_cloud, tangent_points[4]);
  const std::array<PointId, 3> axial_seed{
      point_id(tangent_cloud, tangent_points[1]),
      point_id(tangent_cloud, tangent_points[2]),
      point_id(tangent_cloud, tangent_points[3])};
  const auto tangent_box =
      build_strictly_padded_dyadic_aabb(tangent_cloud);
  const auto tangent = build_exact_bounded_ordinary_cell_closure(
      tangent_cloud, tangent_owner, tangent_box, axial_seed);
  bool diagonal_added_as_tie = false;
  for (const auto& query : tangent.rounds.front().vertex_queries) {
    diagonal_added_as_tie =
        diagonal_added_as_tie ||
        (query.classification ==
             ExactOrdinaryCellVertexClassification::
                 missing_active_nearest_shell &&
         std::binary_search(
             query.newly_required_competitor_ids.begin(),
             query.newly_required_competitor_ids.end(),
             diagonal_id));
  }
  check(
      tangent.rounds.size() == 2U && diagonal_added_as_tie &&
          tangent.audit.violating_vertex_count == 0U &&
          tangent.audit.missing_active_vertex_count != 0U,
      "a tangent diagonal is added as an active equality even though it does "
      "not change the candidate geometry");
  check_complete_verification(
      tangent_cloud,
      tangent_owner,
      tangent_box,
      axial_seed,
      tangent,
      "the tangent co-incidence passes fresh replay");
}

void test_multiple_monotone_revelation_rounds() {
  const std::array<CertifiedPoint3, 5> points{
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(4.0, 0.0, 0.0),
      point(8.0, 0.0, 0.0),
      point(16.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const PointId owner_id = point_id(cloud, points[0]);
  const std::array<PointId, 1> seed{point_id(cloud, points[4])};
  const auto clipping_box = build_strictly_padded_dyadic_aabb(cloud);
  const auto result = build_exact_bounded_ordinary_cell_closure(
      cloud, owner_id, clipping_box, seed);

  const std::array<PointId, 3> expected_additions{
      point_id(cloud, points[3]),
      point_id(cloud, points[2]),
      point_id(cloud, points[1])};
  bool additions_match = result.rounds.size() == 4U;
  if (additions_match) {
    for (std::size_t round_index = 0U;
         round_index < expected_additions.size();
         ++round_index) {
      additions_match =
          additions_match &&
          result.rounds[round_index]
                  .simultaneously_added_competitor_ids ==
              std::vector<PointId>{expected_additions[round_index]};
    }
  }
  check(
      additions_match && result.rounds.back().local_queue_empty &&
          result.audit.simultaneous_addition_batch_count == 3U &&
          result.audit.simultaneously_added_competitor_count == 3U &&
          result.audit.maximum_simultaneous_addition_count == 1U,
      "successive hidden violators force three strict monotone growth rounds");
  check_complete_verification(
      cloud,
      owner_id,
      clipping_box,
      seed,
      result,
      "the multi-round monotone closure passes fresh replay");
}

void test_atomic_budget_preflight() {
  CubeFixture fixture = cube_fixture();
  const std::array<PointId, 0> empty_seed{};
  const auto complete = build_exact_bounded_ordinary_cell_closure(
      fixture.cloud,
      fixture.owner_id,
      fixture.clipping_box,
      empty_seed);
  check(
      complete.requirements.conservative_cell_construction_count == 7U &&
          complete.requirements
                  .conservative_cumulative_plane_triple_count == 966U &&
          complete.requirements.conservative_cumulative_vertex_count ==
              966U &&
          complete.requirements.conservative_cumulative_incidence_count ==
              10822U &&
          complete.requirements.conservative_vertex_query_count == 966U &&
          complete.requirements
                  .conservative_exact_distance_evaluation_count == 7728U &&
          complete.requirements.conservative_nearest_shell_entry_count ==
              7728U &&
          complete.requirements
                  .conservative_owner_strict_feasibility_test_count == 7U &&
          complete.requirements
                  .conservative_simultaneous_addition_count == 6U,
      "the empty cube proposal derives every trusted n8 fallback cap");

  ExactOrdinaryCellClosureBudget short_budget;
  short_budget.maximum_cell_construction_count = 6U;
  check_insufficient(
      fixture.cloud,
      fixture.owner_id,
      fixture.clipping_box,
      empty_seed,
      short_budget,
      "a one-below construction cap fails before geometry");
  short_budget = {};
  short_budget.maximum_cumulative_plane_triple_count = 965U;
  check_insufficient(
      fixture.cloud, fixture.owner_id, fixture.clipping_box, empty_seed,
      short_budget, "a one-below plane-triple cap fails atomically");
  short_budget = {};
  short_budget.maximum_cumulative_vertex_count = 965U;
  check_insufficient(
      fixture.cloud, fixture.owner_id, fixture.clipping_box, empty_seed,
      short_budget, "a one-below vertex cap fails atomically");
  short_budget = {};
  short_budget.maximum_cumulative_incidence_count = 10821U;
  check_insufficient(
      fixture.cloud, fixture.owner_id, fixture.clipping_box, empty_seed,
      short_budget, "a one-below incidence cap fails atomically");
  short_budget = {};
  short_budget.maximum_vertex_query_count = 965U;
  check_insufficient(
      fixture.cloud, fixture.owner_id, fixture.clipping_box, empty_seed,
      short_budget, "a one-below query cap fails atomically");
  short_budget = {};
  short_budget.maximum_exact_distance_evaluation_count = 7727U;
  check_insufficient(
      fixture.cloud, fixture.owner_id, fixture.clipping_box, empty_seed,
      short_budget, "a one-below distance cap fails atomically");
  short_budget = {};
  short_budget.maximum_nearest_shell_entry_count = 7727U;
  check_insufficient(
      fixture.cloud, fixture.owner_id, fixture.clipping_box, empty_seed,
      short_budget, "a one-below shell-entry cap fails atomically");
  short_budget = {};
  short_budget.maximum_owner_strict_feasibility_test_count = 6U;
  check_insufficient(
      fixture.cloud, fixture.owner_id, fixture.clipping_box, empty_seed,
      short_budget, "a one-below strict-feasibility cap fails atomically");
  short_budget = {};
  short_budget.maximum_simultaneous_addition_count = 5U;
  check_insufficient(
      fixture.cloud, fixture.owner_id, fixture.clipping_box, empty_seed,
      short_budget, "a one-below addition cap fails atomically");

  ExactOrdinaryCellClosureBudget excessive;
  excessive.maximum_cell_construction_count = 8U;
  check_throws<std::invalid_argument>(
      [&fixture, &empty_seed, excessive] {
        static_cast<void>(build_exact_bounded_ordinary_cell_closure(
            fixture.cloud,
            fixture.owner_id,
            fixture.clipping_box,
            empty_seed,
            excessive));
      },
      "a budget above the trusted cap is rejected");
}

void test_fresh_verifier_rejects_layer_mutations() {
  CubeFixture fixture = cube_fixture();
  const auto valid = build_exact_bounded_ordinary_cell_closure(
      fixture.cloud,
      fixture.owner_id,
      fixture.clipping_box,
      fixture.axial_seed_ids);
  check_complete_verification(
      fixture.cloud,
      fixture.owner_id,
      fixture.clipping_box,
      fixture.axial_seed_ids,
      valid,
      "the mutation base is valid");

  const auto rejected = [&fixture](
                            const ExactOrdinaryCellClosureResult& candidate) {
    return !verify_exact_bounded_ordinary_cell_closure(
                fixture.cloud,
                fixture.owner_id,
                fixture.clipping_box,
                fixture.axial_seed_ids,
                candidate)
                .result_certified;
  };

  auto bad_decision = valid;
  bad_decision.decision =
      ExactOrdinaryCellClosureDecision::insufficient_budget;
  check(rejected(bad_decision), "a mutated closure decision is rejected");
  auto bad_owner = valid;
  ++bad_owner.owner_id;
  check(rejected(bad_owner), "a mutated owner identity is rejected");
  auto bad_complete_id = valid;
  bad_complete_id.complete_competitor_ids.back() = PointId{999U};
  check(
      rejected(bad_complete_id),
      "an out-of-range competitor id in an untrusted result is rejected");
  auto bad_seed_order = valid;
  std::reverse(
      bad_seed_order.canonical_effective_initial_competitor_ids.begin(),
      bad_seed_order.canonical_effective_initial_competitor_ids.end());
  std::reverse(
      bad_seed_order.rounds.front()
          .canonical_candidate_competitor_ids.begin(),
      bad_seed_order.rounds.front()
          .canonical_candidate_competitor_ids.end());
  check(
      rejected(bad_seed_order),
      "a coordinated noncanonical seed ordering is rejected before replay");
  auto bad_queue = valid;
  bad_queue.local_queue_empty_certified = false;
  check(rejected(bad_queue), "a falsified empty queue is rejected");
  auto bad_batch = valid;
  bad_batch.rounds.front().simultaneously_added_competitor_ids.pop_back();
  check(rejected(bad_batch), "a truncated simultaneous batch is rejected");

  auto bad_shell = valid;
  bool shell_mutated = false;
  for (auto& query : bad_shell.rounds.front().vertex_queries) {
    if (query.complete_nearest_shell_ids.size() == 8U) {
      query.complete_nearest_shell_ids.pop_back();
      shell_mutated = true;
      break;
    }
  }
  check(shell_mutated && rejected(bad_shell),
        "a removed vertex co-minimizer is rejected");

  auto bad_incidence = valid;
  bool incidence_mutated = false;
  for (auto& vertex : bad_incidence.rounds.back().candidate_cell.vertices) {
    if (vertex.position == ExactRational3{} &&
        !vertex.active_boundary_plane_indices.empty()) {
      vertex.active_boundary_plane_indices.pop_back();
      incidence_mutated = true;
      break;
    }
  }
  check(incidence_mutated && rejected(bad_incidence),
        "a removed natural center incidence is rejected");

  auto bad_artificial = valid;
  bad_artificial.rounds.back().candidate_cell.boundary_planes.front().kind =
      PowerCellBoundaryKind::power_bisector;
  check(rejected(bad_artificial),
        "a naturalized artificial box face is rejected");
  auto bad_audit = valid;
  ++bad_audit.audit.exact_vertex_query_count;
  check(rejected(bad_audit), "a mutated exact work counter is rejected");
  auto bad_box = valid;
  ++bad_box.clipping_box.certificate->omega.upper_binary64_bits[0];
  check(rejected(bad_box), "a mutated clipping-box face is rejected");

  std::vector<CertifiedPoint3> changed_points{
      point(-1.0, -1.0, -1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0),
      point(1.0, 1.0, -1.0),
      point(1.0, -1.0, 1.0),
      point(-1.0, 1.0, 1.0),
      point(2.0, 2.0, 2.0)};
  const CanonicalPointCloud changed_cloud = canonical_cloud(changed_points);
  const auto changed_box = build_strictly_padded_dyadic_aabb(changed_cloud);
  check(
      !verify_exact_bounded_ordinary_cell_closure(
           changed_cloud,
           point_id(changed_cloud, changed_points[0]),
           changed_box,
           fixture.axial_seed_ids,
           valid)
           .result_certified,
      "a transcript cannot be replayed against a different cloud");
}

void test_invalid_inputs_and_strings() {
  CubeFixture fixture = cube_fixture();
  const std::array<PointId, 0> empty_seed{};
  check_throws<std::out_of_range>(
      [&fixture, &empty_seed] {
        static_cast<void>(build_exact_bounded_ordinary_cell_closure(
            fixture.cloud,
            PointId{8},
            fixture.clipping_box,
            empty_seed));
      },
      "an owner outside the cloud is rejected");
  const std::array<PointId, 1> owner_seed{fixture.owner_id};
  check_throws<std::invalid_argument>(
      [&fixture, &owner_seed] {
        static_cast<void>(build_exact_bounded_ordinary_cell_closure(
            fixture.cloud,
            fixture.owner_id,
            fixture.clipping_box,
            owner_seed));
      },
      "the owner cannot seed its own cell");
  const std::array<PointId, 2> duplicate_seed{
      fixture.axial_seed_ids.front(), fixture.axial_seed_ids.front()};
  check_throws<std::invalid_argument>(
      [&fixture, &duplicate_seed] {
        static_cast<void>(build_exact_bounded_ordinary_cell_closure(
            fixture.cloud,
            fixture.owner_id,
            fixture.clipping_box,
            duplicate_seed));
      },
      "duplicate seed ids are rejected");

  auto mutated_box = fixture.clipping_box;
  mutated_box.certificate->omega.lower_binary64_bits[0] =
      mutated_box.certificate->exact_site_aabb.bounds
          .lower_binary64_bits[0];
  check_throws<std::invalid_argument>(
      [&fixture, &mutated_box, &empty_seed] {
        static_cast<void>(build_exact_bounded_ordinary_cell_closure(
            fixture.cloud,
            fixture.owner_id,
            mutated_box,
            empty_seed));
      },
      "a clipping-box certificate mutation is rejected before geometry");

  std::vector<CertifiedPoint3> nine_points;
  for (int value = 0; value < 9; ++value) {
    nine_points.push_back(point(static_cast<double>(value), 0.0, 0.0));
  }
  const CanonicalPointCloud nine_cloud = canonical_cloud(nine_points);
  const auto nine_box = build_strictly_padded_dyadic_aabb(nine_cloud);
  check_throws<std::invalid_argument>(
      [&nine_cloud, &nine_box, &empty_seed] {
        static_cast<void>(build_exact_bounded_ordinary_cell_closure(
            nine_cloud, 0U, nine_box, empty_seed));
      },
      "the local reference loop rejects n greater than eight");

  check(
      morsehgp3d::spatial::to_string(
          ExactOrdinaryCellClosureDecision::complete_nonempty) ==
              "complete_nonempty" &&
          morsehgp3d::spatial::to_string(
              ExactOrdinaryCellClosureDecision::insufficient_budget) ==
              "insufficient_budget" &&
          morsehgp3d::spatial::to_string(
              ExactOrdinaryCellVertexClassification::
                  owner_strict_nearest) == "owner_strict_nearest" &&
          morsehgp3d::spatial::to_string(
              ExactOrdinaryCellVertexClassification::
                  violating_nearest_shell) == "violating_nearest_shell" &&
          morsehgp3d::spatial::to_string(
              ExactOrdinaryCellVertexClassification::
                  missing_active_nearest_shell) ==
              "missing_active_nearest_shell" &&
          morsehgp3d::spatial::to_string(
              ExactOrdinaryCellVertexClassification::
                  reconciled_active_nearest_shell) ==
              "reconciled_active_nearest_shell",
      "ordinary-cell decisions and vertex classes have stable strings");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(morsehgp3d::spatial::to_string(
            static_cast<ExactOrdinaryCellClosureDecision>(255)));
      },
      "an invalid ordinary-cell decision cannot be stringified");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(morsehgp3d::spatial::to_string(
            static_cast<ExactOrdinaryCellVertexClassification>(255)));
      },
      "an invalid vertex classification cannot be stringified");
}

}  // namespace

int main() {
  test_singleton_and_canonical_fallback_seed();
  test_cube_reconciles_face_edge_and_vertex_shells();
  test_hidden_seed_violator_and_tangent_coincidence();
  test_multiple_monotone_revelation_rounds();
  test_atomic_budget_preflight();
  test_fresh_verifier_rejects_layer_mutations();
  test_invalid_inputs_and_strings();

  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "all exact ordinary-cell closure tests passed\n";
  return 0;
}
