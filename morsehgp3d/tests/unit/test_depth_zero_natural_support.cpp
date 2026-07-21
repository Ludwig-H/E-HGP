#include "morsehgp3d/hierarchy/critical_catalog.hpp"
#include "morsehgp3d/hierarchy/depth_zero_natural_support.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::hierarchy::ExactCriticalCatalogBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogCandidateOutcome;
using morsehgp3d::hierarchy::ExactDepthZeroNaturalSupportBudget;
using morsehgp3d::hierarchy::ExactDepthZeroNaturalSupportCandidate;
using morsehgp3d::hierarchy::ExactDepthZeroNaturalSupportCandidateOutcome;
using morsehgp3d::hierarchy::ExactDepthZeroNaturalSupportDecision;
using morsehgp3d::hierarchy::ExactDepthZeroNaturalSupportResult;
using morsehgp3d::hierarchy::ExactDepthZeroNaturalSupportVerification;
using morsehgp3d::hierarchy::build_exact_bounded_depth_zero_natural_supports;
using morsehgp3d::hierarchy::build_exact_critical_catalog;
using morsehgp3d::hierarchy::verify_exact_bounded_depth_zero_natural_supports;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactOrdinaryDiagramClosureResult;
using morsehgp3d::spatial::ExactOrdinaryDiagramContactKind;
using morsehgp3d::spatial::PointId;
using morsehgp3d::spatial::build_exact_bounded_ordinary_diagram_closure;
using morsehgp3d::spatial::build_strictly_padded_dyadic_aabb;

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

[[nodiscard]] ExactLevel level(
    std::int64_t numerator, std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
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
  throw std::logic_error("a fixture point is absent from its cloud");
}

template <std::size_t Size>
[[nodiscard]] std::vector<PointId> ids(
    const CanonicalPointCloud& cloud,
    const std::array<CertifiedPoint3, Size>& points,
    std::initializer_list<std::size_t> indices) {
  std::vector<PointId> result;
  result.reserve(indices.size());
  for (const std::size_t index : indices) {
    result.push_back(point_id(cloud, points[index]));
  }
  std::sort(result.begin(), result.end());
  return result;
}

[[nodiscard]] bool all_certificates_close(
    const ExactDepthZeroNaturalSupportVerification& verification) {
  return verification.requested_budget_certified &&
         verification.input_identity_certified &&
         verification.clipping_box_certified &&
         verification.requirements_certified &&
         verification.source_diagram_certified &&
         verification.candidates_certified &&
         verification.supports_certified &&
         verification.relevant_extra_shell_diagnostics_certified &&
         verification.predicate_counters_certified &&
         verification.audit_certified &&
         verification.result_facts_certified &&
         verification.decision_certified &&
         verification.fresh_replay_certified &&
         verification.result_certified;
}

[[nodiscard]] std::size_t outcome_count(
    const ExactDepthZeroNaturalSupportResult& result,
    ExactDepthZeroNaturalSupportCandidateOutcome outcome) {
  return static_cast<std::size_t>(std::count_if(
      result.candidates.begin(),
      result.candidates.end(),
      [outcome](const ExactDepthZeroNaturalSupportCandidate& candidate) {
        return candidate.outcome == outcome;
      }));
}

[[nodiscard]] const ExactDepthZeroNaturalSupportCandidate* candidate_for(
    const ExactDepthZeroNaturalSupportResult& result,
    const std::vector<PointId>& support) {
  const auto found = std::find_if(
      result.candidates.begin(),
      result.candidates.end(),
      [&](const ExactDepthZeroNaturalSupportCandidate& candidate) {
        return candidate.support_point_ids == support;
      });
  return found == result.candidates.end() ? nullptr : &*found;
}

void check_complete_result(
    const CanonicalPointCloud& cloud,
    const morsehgp3d::spatial::StrictlyPaddedDyadicAabb3Result& box,
    const ExactOrdinaryDiagramClosureResult& source,
    std::size_t maximum_order,
    const ExactDepthZeroNaturalSupportResult& result,
    const std::string& message) {
  check(
      all_certificates_close(
          verify_exact_bounded_depth_zero_natural_supports(
              cloud, box, source, maximum_order, {}, result)),
      message);
}

void test_singleton_empty_extraction() {
  const std::array<CertifiedPoint3, 1> points{point(0.0, -0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const auto box = build_strictly_padded_dyadic_aabb(cloud);
  const auto source = build_exact_bounded_ordinary_diagram_closure(
      cloud, box);
  const auto result = build_exact_bounded_depth_zero_natural_supports(
      cloud, box, source, 10U);
  check(
      result.decision == ExactDepthZeroNaturalSupportDecision::
          complete_supported_extraction &&
          result.requirements.point_count == 1U &&
          result.requirements.effective_maximum_order == 1U &&
          result.requirements.maximum_relevant_support_rank == 1U &&
          result.candidates.empty() && result.supports.empty() &&
          result.relevant_extra_shell_diagnostics.empty() &&
          result.audit.source_contact_count == 0U &&
          result.audit.support_analysis_count == 0U &&
          result.proposal_space_complete &&
          result.candidate_queue_empty &&
          result.no_artificial_support_emitted,
      "a singleton closes depth zero without fabricating a positive-radius support");
  check_complete_result(
      cloud, box, source, 10U, result,
      "the empty singleton extraction passes fresh replay");
}

void test_regular_tetrahedron_strata_and_rank_window() {
  const std::array<CertifiedPoint3, 4> points{
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const auto box = build_strictly_padded_dyadic_aabb(cloud);
  const auto source = build_exact_bounded_ordinary_diagram_closure(
      cloud, box);
  const auto full = build_exact_bounded_depth_zero_natural_supports(
      cloud, box, source, 4U);
  check(
      full.decision == ExactDepthZeroNaturalSupportDecision::
          complete_supported_extraction &&
          full.candidates.size() == 11U &&
          full.supports.size() == 11U &&
          full.audit.source_contact_count == 11U &&
          full.audit.natural_carrier_contact_count == 11U &&
          full.audit.raw_support_proposal_count == 33U &&
          full.audit.raw_support_point_id_reference_count == 76U &&
          full.audit.unique_support_count == 11U &&
          full.audit.unique_support_point_id_reference_count == 28U &&
          full.audit.support_analysis_count == 11U &&
          full.audit.global_closed_ball_query_count == 11U &&
          full.audit.point_classification_count == 44U,
      "the regular tetrahedron exposes every depth-zero stratum exactly once");

  std::array<std::size_t, 3> kind_counts{};
  for (const auto& support : full.supports) {
    const std::size_t size = support.support_point_ids.size();
    check(
        support.closed_rank == size &&
            support.contact_affine_dimension == 4U - size &&
            support.contact_site_affine_rank == size - 1U &&
            support.natural_contact_index < source.contacts.size() &&
            source.contacts[support.natural_contact_index].query_ids ==
                support.support_point_ids &&
            source.contacts[support.natural_contact_index]
                    .carrier_shell_ids == support.support_point_ids &&
            source.contacts[support.natural_contact_index]
                    .common_artificial_box_face_mask == 0U,
        "every accepted tetrahedron support is reciprocal with its natural contact");
    if (size >= 2U && size <= 4U) {
      ++kind_counts[size - 2U];
    }
  }
  check(
      kind_counts == std::array<std::size_t, 3>{6U, 4U, 1U},
      "the tetrahedron has six faces, four edges and one natural vertex in the dual diagram");
  const auto vertex = std::find_if(
      full.supports.begin(),
      full.supports.end(),
      [](const auto& support) {
        return support.support_point_ids.size() == 4U;
      });
  check(
      vertex != full.supports.end() &&
          vertex->center ==
              ExactRational3{BigInt{0}, BigInt{0}, BigInt{0}, BigInt{1}} &&
          vertex->squared_level == level(3) &&
          std::all_of(
              vertex->support_barycentric_coordinates.begin(),
              vertex->support_barycentric_coordinates.end(),
              [](const ExactRational& coordinate) {
                return coordinate ==
                    ExactRational{BigInt{1}, BigInt{4}};
              }),
      "the tetrahedral natural vertex recomputes its exact critical sphere and barycentrics");
  check_complete_result(
      cloud, box, source, 4U, full,
      "the complete tetrahedron extraction passes fresh replay");

  const auto order_one =
      build_exact_bounded_depth_zero_natural_supports(
          cloud, box, source, 1U);
  check(
      order_one.supports.size() == 6U &&
          outcome_count(
              order_one,
              ExactDepthZeroNaturalSupportCandidateOutcome::
                  minimal_support_above_rank_window) == 5U &&
          std::all_of(
              order_one.supports.begin(),
              order_one.supports.end(),
              [](const auto& support) {
                return support.support_point_ids.size() == 2U &&
                    support.natural_contact_kind ==
                        ExactOrdinaryDiagramContactKind::natural_face;
              }),
      "Kmax=1 accepts only pair supports without relabeling higher strata");
}

void test_cocircular_pentagon_and_exhaustive_test_oracle() {
  const std::array<CertifiedPoint3, 5> points{
      point(-5.0, 0.0),
      point(-3.0, 4.0),
      point(0.0, -5.0),
      point(3.0, 4.0),
      point(5.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const auto box = build_strictly_padded_dyadic_aabb(cloud);
  const auto source = build_exact_bounded_ordinary_diagram_closure(
      cloud, box);
  const auto result = build_exact_bounded_depth_zero_natural_supports(
      cloud, box, source, 10U);
  check(
      source.contacts.size() == 26U &&
          source.audit.natural_face_count == 5U &&
          source.audit.natural_edge_count == 1U &&
          source.audit.natural_vertex_count == 0U &&
          source.audit.noncanonical_quotient_contact_count == 20U,
      "the cocircular pentagon has one shell-five natural carrier and explicit quotients");
  check(
      result.decision == ExactDepthZeroNaturalSupportDecision::
          complete_extraction_with_relevant_extra_shell_degeneracy &&
          result.audit.raw_support_proposal_count == 30U &&
          result.audit.raw_support_point_id_reference_count == 80U &&
          result.audit.unique_support_count == 25U &&
          result.audit.unique_support_point_id_reference_count == 70U &&
          result.audit.support_analysis_count == 25U &&
          result.audit.minimal_support_count == 13U &&
          result.audit.global_closed_ball_query_count == 13U &&
          result.audit.point_classification_count == 65U &&
          result.supports.size() == 5U,
      "the pentagon deduplicates every carrier subset before exact classification");
  check(
      outcome_count(
          result,
          ExactDepthZeroNaturalSupportCandidateOutcome::
              affinely_dependent_support) == 5U &&
          outcome_count(
              result,
              ExactDepthZeroNaturalSupportCandidateOutcome::
                  boundary_reduced_support) == 3U &&
          outcome_count(
              result,
              ExactDepthZeroNaturalSupportCandidateOutcome::
                  exterior_circumcenter_support) == 4U &&
          outcome_count(
              result,
              ExactDepthZeroNaturalSupportCandidateOutcome::
                  minimal_with_strict_interior_deferred) == 4U &&
          outcome_count(
              result,
              ExactDepthZeroNaturalSupportCandidateOutcome::
                  relevant_extra_shell_degeneracy) == 4U &&
          outcome_count(
              result,
              ExactDepthZeroNaturalSupportCandidateOutcome::
                  accepted_empty_interior_support) == 5U,
      "the pentagon exercises every depth-zero disposition without inventing an event");
  check(
      result.relevant_extra_shell_diagnostics.size() == 1U &&
          result.relevant_extra_shell_diagnostics.front()
                  .support_point_id_sets.size() == 4U &&
          result.relevant_extra_shell_diagnostics.front()
                  .shell_point_ids.size() == 5U &&
          result.relevant_extra_shell_diagnostics.front()
                  .observed_closed_rank == 5U &&
          result.relevant_extra_shell_diagnostics.front().center ==
              ExactRational3{BigInt{0}, BigInt{0}, BigInt{0}, BigInt{1}} &&
          result.relevant_extra_shell_diagnostics.front().squared_level ==
              level(25),
      "all relevant pentagon extra shells aggregate under one exact sphere identity");

  const std::vector<PointId> ordinary_pair = ids(cloud, points, {0U, 1U});
  const ExactDepthZeroNaturalSupportCandidate* pair_candidate =
      candidate_for(result, ordinary_pair);
  check(
      pair_candidate != nullptr && pair_candidate->center.has_value() &&
          *pair_candidate->center ==
              ExactRational3{BigInt{-4}, BigInt{2}, BigInt{0}, BigInt{1}} &&
          pair_candidate->squared_level ==
              std::optional<ExactLevel>{level(5)},
      "the critical pair center is recomputed and is not the contact witness barycenter");

  const auto catalog = build_exact_critical_catalog(
      cloud,
      10U,
      ExactCriticalCatalogBudget{
          ExactCriticalCatalogBudget::maximum_supported_candidate_count,
          ExactCriticalCatalogBudget::
              maximum_supported_point_classification_count});
  std::size_t matched_events = 0U;
  for (const auto& support : result.supports) {
    const auto match = std::find_if(
        catalog.events.begin(),
        catalog.events.end(),
        [&](const auto& event) {
          return event.support_point_ids == support.support_point_ids &&
              event.interior_point_ids.empty() &&
              event.shell_point_ids == support.support_point_ids &&
              event.center == support.center &&
              event.squared_level == support.squared_level;
        });
    matched_events += static_cast<std::size_t>(
        match != catalog.events.end());
  }
  check(
      matched_events == result.supports.size(),
      "the product extractor agrees with the exhaustive catalogue used only as a test oracle");
  std::size_t expected_non_singleton_event_count = 0U;
  for (const auto& event : catalog.events) {
    if (event.support_point_ids.size() < 2U ||
        event.support_point_ids.size() > 4U ||
        !event.interior_point_ids.empty() ||
        event.shell_point_ids != event.support_point_ids) {
      continue;
    }
    ++expected_non_singleton_event_count;
    const auto match = std::find_if(
        result.supports.begin(),
        result.supports.end(),
        [&](const auto& support) {
          return support.support_point_ids == event.support_point_ids &&
              support.center == event.center &&
              support.squared_level == event.squared_level;
        });
    check(
        match != result.supports.end(),
        "every admissible depth-zero oracle event is emitted by the product extractor");
  }
  check(
      expected_non_singleton_event_count == result.supports.size(),
      "the product and exhaustive oracle event sets agree in both directions");
  std::vector<std::vector<PointId>> expected_diagnostic_supports;
  for (const auto& candidate : catalog.candidates) {
    if (candidate.support_point_ids.size() >= 2U &&
        candidate.support_point_ids.size() <= 4U &&
        candidate.interior_point_ids.empty() &&
        candidate.outcome ==
            ExactCriticalCatalogCandidateOutcome::
                relevant_extra_shell_degeneracy) {
      expected_diagnostic_supports.push_back(candidate.support_point_ids);
    }
  }
  std::sort(
      expected_diagnostic_supports.begin(),
      expected_diagnostic_supports.end(),
      [](const auto& left, const auto& right) {
        if (left.size() != right.size()) {
          return left.size() < right.size();
        }
        return left < right;
      });
  check(
      result.relevant_extra_shell_diagnostics.size() == 1U &&
          result.relevant_extra_shell_diagnostics.front()
                  .support_point_id_sets == expected_diagnostic_supports,
      "the product and exhaustive oracle diagnostic support sets agree in both directions");
  for (const auto& diagnostic : result.relevant_extra_shell_diagnostics) {
    for (const auto& support_ids : diagnostic.support_point_id_sets) {
      const auto match = std::find_if(
          catalog.candidates.begin(),
          catalog.candidates.end(),
          [&](const auto& candidate) {
            return candidate.support_point_ids == support_ids &&
                candidate.interior_point_ids.empty() &&
                candidate.outcome ==
                    ExactCriticalCatalogCandidateOutcome::
                        relevant_extra_shell_degeneracy &&
                candidate.shell_point_ids == diagnostic.shell_point_ids;
          });
      check(
          match != catalog.candidates.end(),
          "each depth-zero diagnostic is witnessed by the exhaustive test oracle");
    }
  }
  check_complete_result(
      cloud, box, source, 10U, result,
      "the cocircular pentagon extraction passes fresh replay");

  const auto order_one =
      build_exact_bounded_depth_zero_natural_supports(
          cloud, box, source, 1U);
  const ExactDepthZeroNaturalSupportCandidate* diameter =
      candidate_for(order_one, ids(cloud, points, {0U, 4U}));
  check(
      diameter != nullptr &&
          diameter->outcome ==
              ExactDepthZeroNaturalSupportCandidateOutcome::
                  relevant_extra_shell_degeneracy &&
          diameter->support_relevance_rank == 2U &&
          diameter->observed_closed_rank == 5U &&
          outcome_count(
              order_one,
              ExactDepthZeroNaturalSupportCandidateOutcome::
                  extra_shell_outside_rank_window) == 3U &&
          outcome_count(
              order_one,
              ExactDepthZeroNaturalSupportCandidateOutcome::
                  minimal_with_strict_interior_deferred) == 4U &&
          order_one.relevant_extra_shell_diagnostics.size() == 1U &&
          order_one.relevant_extra_shell_diagnostics.front()
                  .support_point_id_sets.size() == 1U,
      "RelevantGP uses support rank while strict-interior candidates remain deferred");
}

[[nodiscard]] ExactDepthZeroNaturalSupportResult triangle_result(
    double height,
    CanonicalPointCloud& cloud,
    morsehgp3d::spatial::StrictlyPaddedDyadicAabb3Result& box,
    ExactOrdinaryDiagramClosureResult& source) {
  const std::array<CertifiedPoint3, 3> points{
      point(-1.0, 0.0), point(0.0, height), point(1.0, 0.0)};
  cloud = canonical_cloud(points);
  box = build_strictly_padded_dyadic_aabb(cloud);
  source = build_exact_bounded_ordinary_diagram_closure(cloud, box);
  return build_exact_bounded_depth_zero_natural_supports(
      cloud, box, source, 3U);
}

void test_ulp_transition_and_permutation() {
  CanonicalPointCloud below_cloud = canonical_cloud(
      std::array<CertifiedPoint3, 1>{point(0.0)});
  auto below_box = build_strictly_padded_dyadic_aabb(below_cloud);
  ExactOrdinaryDiagramClosureResult below_source;
  const auto below = triangle_result(
      std::nextafter(1.0, 0.0),
      below_cloud,
      below_box,
      below_source);
  check(
      below.candidates.size() == 2U && below.supports.size() == 2U &&
          outcome_count(
              below,
              ExactDepthZeroNaturalSupportCandidateOutcome::
                  minimal_with_strict_interior_deferred) == 0U,
      "the obtuse-side diameter is absent from natural proposals rather than mislabeled deferred");

  CanonicalPointCloud exact_cloud = canonical_cloud(
      std::array<CertifiedPoint3, 1>{point(0.0)});
  auto exact_box = build_strictly_padded_dyadic_aabb(exact_cloud);
  ExactOrdinaryDiagramClosureResult exact_source;
  const auto exact = triangle_result(
      1.0, exact_cloud, exact_box, exact_source);
  check(
      exact.candidates.size() == 4U && exact.supports.size() == 2U &&
          outcome_count(
              exact,
              ExactDepthZeroNaturalSupportCandidateOutcome::
                  boundary_reduced_support) == 1U &&
          outcome_count(
              exact,
              ExactDepthZeroNaturalSupportCandidateOutcome::
                  relevant_extra_shell_degeneracy) == 1U &&
          exact.relevant_extra_shell_diagnostics.size() == 1U,
      "the exact right-triangle boundary separates reduced and extra-shell supports");

  CanonicalPointCloud above_cloud = canonical_cloud(
      std::array<CertifiedPoint3, 1>{point(0.0)});
  auto above_box = build_strictly_padded_dyadic_aabb(above_cloud);
  ExactOrdinaryDiagramClosureResult above_source;
  const auto above = triangle_result(
      std::nextafter(1.0, std::numeric_limits<double>::infinity()),
      above_cloud,
      above_box,
      above_source);
  check(
      above.candidates.size() == 4U && above.supports.size() == 4U,
      "one ULP above the right boundary all three faces and the triangle are minimal");

  const std::array<CertifiedPoint3, 3> reversed_points{
      point(1.0, 0.0), point(0.0, 1.0), point(-1.0, 0.0)};
  const CanonicalPointCloud reversed = canonical_cloud(reversed_points);
  const auto reversed_box = build_strictly_padded_dyadic_aabb(reversed);
  const auto reversed_source =
      build_exact_bounded_ordinary_diagram_closure(
          reversed, reversed_box);
  const auto reversed_result =
      build_exact_bounded_depth_zero_natural_supports(
          reversed, reversed_box, reversed_source, 3U);
  check(
      reversed_box == exact_box && reversed_source == exact_source &&
          reversed_result == exact,
      "canonical IDs make depth-zero extraction invariant under input permutation");
}

void test_box_contacts_never_emit_supports() {
  constexpr double base = 0x1p52;
  const std::array<CertifiedPoint3, 3> points{
      point(-2.0, base + 1.0, 0.0),
      point(2.0, base + 1.0, 0.0),
      point(-1.0, base + 2.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const auto box = build_strictly_padded_dyadic_aabb(cloud);
  const auto source = build_exact_bounded_ordinary_diagram_closure(
      cloud, box);
  const auto result = build_exact_bounded_depth_zero_natural_supports(
      cloud, box, source, 3U);
  check(
      source.audit.box_supported_contact_count == 1U &&
          source.audit.natural_edge_count == 0U &&
          result.candidates.size() == 2U && result.supports.size() == 2U &&
          result.no_artificial_support_emitted &&
          std::all_of(
              result.supports.begin(),
              result.supports.end(),
              [](const auto& support) {
                return support.support_point_ids.size() == 2U &&
                    support.natural_contact_kind ==
                        ExactOrdinaryDiagramContactKind::natural_face;
              }),
      "quotient and clipping-box contacts never become natural supports");
}

void test_transactional_preflight_at_n8() {
  const std::array<CertifiedPoint3, 8> points{
      point(-1.0, -1.0, -1.0), point(-1.0, -1.0, 1.0),
      point(-1.0, 1.0, -1.0), point(-1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0), point(1.0, -1.0, 1.0),
      point(1.0, 1.0, -1.0), point(1.0, 1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const auto box = build_strictly_padded_dyadic_aabb(cloud);
  const ExactOrdinaryDiagramClosureResult invalid_source;

  const auto check_short = [&](ExactDepthZeroNaturalSupportBudget budget,
                               const std::string& name) {
    const auto receipt =
        build_exact_bounded_depth_zero_natural_supports(
            cloud, box, invalid_source, 10U, budget);
    check(
        receipt.decision ==
                ExactDepthZeroNaturalSupportDecision::insufficient_budget &&
            receipt.requirements.conservative_source_contact_count == 247U &&
            receipt.requirements
                    .conservative_raw_support_proposal_count == 4704U &&
            receipt.requirements
                    .conservative_raw_support_point_id_reference_count ==
                13440U &&
            receipt.requirements.conservative_unique_support_count == 154U &&
            receipt.requirements
                    .conservative_unique_support_point_id_reference_count ==
                504U &&
            receipt.requirements
                    .conservative_point_classification_count == 1232U &&
            !receipt.preflight_budget_sufficient &&
            !receipt.extraction_started_after_successful_preflight &&
            !receipt.source_diagram.has_value() &&
            receipt.candidates.empty() && receipt.supports.empty() &&
            receipt.relevant_extra_shell_diagnostics.empty() &&
            receipt.audit ==
                morsehgp3d::hierarchy::
                    ExactDepthZeroNaturalSupportAudit{} &&
            all_certificates_close(
                verify_exact_bounded_depth_zero_natural_supports(
                    cloud,
                    box,
                    invalid_source,
                    10U,
                    budget,
                    receipt)),
        "the " + name + " preflight fails atomically before source replay");
  };

  ExactDepthZeroNaturalSupportBudget budget;
  budget.maximum_source_contact_count = 246U;
  check_short(budget, "source-contact");
  budget = {};
  budget.maximum_raw_support_proposal_count = 4703U;
  check_short(budget, "raw-proposal");
  budget = {};
  budget.maximum_raw_support_point_id_reference_count = 13439U;
  check_short(budget, "raw-ID");
  budget = {};
  budget.maximum_unique_support_count = 153U;
  check_short(budget, "unique-support");
  budget = {};
  budget.maximum_unique_support_point_id_reference_count = 503U;
  check_short(budget, "unique-ID");
  budget = {};
  budget.maximum_point_classification_count = 1231U;
  check_short(budget, "classification");

  const auto check_oversized = [&](ExactDepthZeroNaturalSupportBudget oversized,
                                   const std::string& name) {
    check_throws<std::invalid_argument>(
        [&] {
          static_cast<void>(
              build_exact_bounded_depth_zero_natural_supports(
                  cloud, box, invalid_source, 10U, oversized));
        },
        "the " + name + " budget cannot exceed its trusted cap");
  };
  budget = {};
  budget.maximum_source_contact_count = 248U;
  check_oversized(budget, "source-contact");
  budget = {};
  budget.maximum_raw_support_proposal_count = 4705U;
  check_oversized(budget, "raw-proposal");
  budget = {};
  budget.maximum_raw_support_point_id_reference_count = 13441U;
  check_oversized(budget, "raw-ID");
  budget = {};
  budget.maximum_unique_support_count = 155U;
  check_oversized(budget, "unique-support");
  budget = {};
  budget.maximum_unique_support_point_id_reference_count = 505U;
  check_oversized(budget, "unique-ID");
  budget = {};
  budget.maximum_point_classification_count = 1233U;
  check_oversized(budget, "classification");
}

void test_insufficient_receipt_binds_exact_cloud() {
  const std::array<CertifiedPoint3, 3> first_points{
      point(-2.0, -2.0, -2.0),
      point(0.0, 0.0, 0.0),
      point(2.0, 2.0, 2.0)};
  const std::array<CertifiedPoint3, 3> second_points{
      point(-2.0, -2.0, -2.0),
      point(0.5, 0.0, 0.0),
      point(2.0, 2.0, 2.0)};
  const CanonicalPointCloud first_cloud = canonical_cloud(first_points);
  const CanonicalPointCloud second_cloud = canonical_cloud(second_points);
  const auto first_box = build_strictly_padded_dyadic_aabb(first_cloud);
  const auto second_box = build_strictly_padded_dyadic_aabb(second_cloud);
  check(
      first_box == second_box,
      "the depth-zero receipt fixture preserves the exact clipping box");
  ExactDepthZeroNaturalSupportBudget short_budget;
  short_budget.maximum_source_contact_count = 3U;
  const ExactOrdinaryDiagramClosureResult unused_source;
  const auto receipt = build_exact_bounded_depth_zero_natural_supports(
      first_cloud, first_box, unused_source, 3U, short_budget);
  const auto first_verification =
      verify_exact_bounded_depth_zero_natural_supports(
          first_cloud,
          first_box,
          unused_source,
          3U,
          short_budget,
          receipt);
  const auto second_verification =
      verify_exact_bounded_depth_zero_natural_supports(
          second_cloud,
          second_box,
          unused_source,
          3U,
          short_budget,
          receipt);
  check(
      receipt.decision ==
              ExactDepthZeroNaturalSupportDecision::insufficient_budget &&
          first_verification.result_certified &&
          !second_verification.input_identity_certified &&
          !second_verification.result_certified,
      "an insufficient depth-zero receipt binds every canonical point bit");
}

void test_source_gate_and_hostile_result_replay() {
  const std::array<CertifiedPoint3, 3> points{
      point(-1.0, 0.0), point(0.0, 1.0), point(1.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const auto box = build_strictly_padded_dyadic_aabb(cloud);
  morsehgp3d::spatial::ExactOrdinaryDiagramClosureBudget short_source_budget;
  short_source_budget.maximum_contact_count = 3U;
  const ExactOrdinaryDiagramClosureResult invalid_source =
      build_exact_bounded_ordinary_diagram_closure(
          cloud, box, short_source_budget);
  check(
      invalid_source.decision ==
              morsehgp3d::spatial::
                  ExactOrdinaryDiagramClosureDecision::insufficient_budget &&
          morsehgp3d::spatial::
              verify_exact_bounded_ordinary_diagram_closure(
                  cloud, box, invalid_source, short_source_budget)
                  .result_certified,
      "the source-gate fixture is a genuinely certified insufficient ordinary receipt");
  const auto rejected_source_receipt =
      build_exact_bounded_depth_zero_natural_supports(
          cloud, box, invalid_source, 3U);
  check(
      rejected_source_receipt.decision ==
              ExactDepthZeroNaturalSupportDecision::
                  source_diagram_not_complete_or_not_certified &&
          rejected_source_receipt.preflight_budget_sufficient &&
          !rejected_source_receipt
               .extraction_started_after_successful_preflight &&
          !rejected_source_receipt.source_diagram.has_value() &&
          rejected_source_receipt.candidates.empty() &&
          rejected_source_receipt.audit ==
              morsehgp3d::hierarchy::
                  ExactDepthZeroNaturalSupportAudit{} &&
          all_certificates_close(
              verify_exact_bounded_depth_zero_natural_supports(
                  cloud,
                  box,
                  invalid_source,
                  3U,
                  {},
                  rejected_source_receipt)),
      "an insufficient-but-certified ordinary receipt cannot open extraction");

  const auto source = build_exact_bounded_ordinary_diagram_closure(
      cloud, box);
  check(
      !source.contacts.empty(),
      "the hostile source fixture contains a contact to remove");
  if (source.contacts.empty()) {
    return;
  }
  auto corrupted_complete_source = source;
  corrupted_complete_source.contacts.pop_back();
  const auto corrupted_source_receipt =
      build_exact_bounded_depth_zero_natural_supports(
          cloud, box, corrupted_complete_source, 3U);
  check(
      corrupted_complete_source.decision ==
              morsehgp3d::spatial::
                  ExactOrdinaryDiagramClosureDecision::complete &&
          corrupted_source_receipt.decision ==
              ExactDepthZeroNaturalSupportDecision::
                  source_diagram_not_complete_or_not_certified &&
          !corrupted_source_receipt.source_diagram.has_value() &&
          !corrupted_source_receipt
               .extraction_started_after_successful_preflight &&
          corrupted_source_receipt.candidates.empty() &&
          corrupted_source_receipt.audit ==
              morsehgp3d::hierarchy::
                  ExactDepthZeroNaturalSupportAudit{},
      "a source claiming complete but missing one contact fails before extraction");
  const auto valid = build_exact_bounded_depth_zero_natural_supports(
      cloud, box, source, 3U);
  check(
      valid.source_diagram.has_value() &&
          !valid.source_diagram->contacts.empty() &&
          !valid.candidates.empty() && !valid.supports.empty() &&
          !valid.relevant_extra_shell_diagnostics.empty(),
      "the hostile replay fixture exposes every layer to mutate");
  if (!valid.source_diagram.has_value() ||
      valid.source_diagram->contacts.empty() ||
      valid.candidates.empty() || valid.supports.empty() ||
      valid.relevant_extra_shell_diagnostics.empty()) {
    return;
  }
  const auto rejected = [&](const ExactDepthZeroNaturalSupportResult& value) {
    return verify_exact_bounded_depth_zero_natural_supports(
        cloud, box, source, 3U, {}, value);
  };

  auto bad_source = valid;
  bad_source.source_diagram->contacts.pop_back();
  auto verification = rejected(bad_source);
  check(
      !verification.source_diagram_certified &&
          !verification.fresh_replay_certified &&
          !verification.result_certified,
      "a mutated embedded source diagram is rejected without steering replay");
  auto bad_candidate = valid;
  bad_candidate.candidates.front().outcome =
      ExactDepthZeroNaturalSupportCandidateOutcome::not_classified;
  verification = rejected(bad_candidate);
  check(
      !verification.candidates_certified &&
          !verification.result_certified,
      "a mutated candidate disposition is rejected");
  auto bad_support = valid;
  bad_support.supports.front().natural_contact_index =
      std::numeric_limits<std::size_t>::max();
  verification = rejected(bad_support);
  check(
      !verification.supports_certified && !verification.result_certified,
      "an untrusted out-of-range support contact is rejected safely");
  auto bad_diagnostic = valid;
  bad_diagnostic.relevant_extra_shell_diagnostics.front()
      .support_point_id_sets.clear();
  verification = rejected(bad_diagnostic);
  check(
      !verification.relevant_extra_shell_diagnostics_certified &&
          !verification.result_certified,
      "a broken diagnostic association is rejected");
  auto bad_predicates = valid;
  ++bad_predicates.predicate_counters.exact_zero_count;
  verification = rejected(bad_predicates);
  check(
      !verification.predicate_counters_certified &&
          !verification.result_certified,
      "a mutated predicate receipt is rejected");
  auto bad_audit = valid;
  ++bad_audit.audit.accepted_support_count;
  verification = rejected(bad_audit);
  check(
      !verification.audit_certified && !verification.result_certified,
      "a mutated extraction audit is rejected");
  auto bad_fact = valid;
  bad_fact.candidate_queue_empty = false;
  verification = rejected(bad_fact);
  check(
      !verification.result_facts_certified &&
          !verification.result_certified,
      "a mutated terminal fact is rejected");
  auto bad_decision = valid;
  bad_decision.decision =
      ExactDepthZeroNaturalSupportDecision::complete_supported_extraction;
  verification = rejected(bad_decision);
  check(
      !verification.decision_certified && !verification.result_certified,
      "a downgraded degeneracy decision is rejected");

  ExactDepthZeroNaturalSupportBudget other_budget;
  --other_budget.maximum_source_contact_count;
  verification = verify_exact_bounded_depth_zero_natural_supports(
      cloud, box, source, 3U, other_budget, valid);
  check(
      !verification.requested_budget_certified &&
          !verification.fresh_replay_certified &&
          !verification.result_certified,
      "the extraction receipt is bound to its exact trusted budget");
}

void test_strings() {
  check(
      morsehgp3d::hierarchy::to_string(
          ExactDepthZeroNaturalSupportCandidateOutcome::
              minimal_with_strict_interior_deferred) ==
              "minimal_with_strict_interior_deferred" &&
          morsehgp3d::hierarchy::to_string(
              ExactDepthZeroNaturalSupportDecision::
                  complete_extraction_with_relevant_extra_shell_degeneracy) ==
              "complete_extraction_with_relevant_extra_shell_degeneracy",
      "depth-zero outcomes have stable audit strings");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(morsehgp3d::hierarchy::to_string(
            static_cast<ExactDepthZeroNaturalSupportCandidateOutcome>(255U)));
      },
      "an invalid depth-zero candidate outcome is rejected");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(morsehgp3d::hierarchy::to_string(
            static_cast<ExactDepthZeroNaturalSupportDecision>(255U)));
      },
      "an invalid depth-zero extraction decision is rejected");
}

}  // namespace

int main() {
  test_singleton_empty_extraction();
  test_regular_tetrahedron_strata_and_rank_window();
  test_cocircular_pentagon_and_exhaustive_test_oracle();
  test_ulp_transition_and_permutation();
  test_box_contacts_never_emit_supports();
  test_transactional_preflight_at_n8();
  test_insufficient_receipt_binds_exact_cloud();
  test_source_gate_and_hostile_result_replay();
  test_strings();
  if (failures != 0) {
    std::cerr << failures << " depth-zero natural-support test(s) failed\n";
    return 1;
  }
  std::cout << "depth-zero natural-support tests passed\n";
  return 0;
}
