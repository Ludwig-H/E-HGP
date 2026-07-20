#include "morsehgp3d/hierarchy/critical_catalog.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactCriticalCatalogBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogCandidate;
using morsehgp3d::hierarchy::ExactCriticalCatalogCandidateOutcome;
using morsehgp3d::hierarchy::ExactCriticalCatalogDecision;
using morsehgp3d::hierarchy::ExactCriticalCatalogResult;
using morsehgp3d::hierarchy::ExactCriticalCatalogScope;
using morsehgp3d::hierarchy::ExactCriticalCatalogVerification;
using morsehgp3d::hierarchy::ExactCriticalEvent;
using morsehgp3d::hierarchy::ExactCriticalH0Batch;
using morsehgp3d::hierarchy::build_exact_critical_catalog;
using morsehgp3d::hierarchy::verify_exact_critical_catalog;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
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

[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::vector<CertifiedPoint3>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator, std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] constexpr ExactCriticalCatalogBudget full_budget() {
  return {
      ExactCriticalCatalogBudget::maximum_supported_candidate_count,
      ExactCriticalCatalogBudget::
          maximum_supported_point_classification_count};
}

[[nodiscard]] bool all_certificates_close(
    const ExactCriticalCatalogVerification& verification) {
  return verification.requested_budget_certified &&
         verification.input_domain_certified &&
         verification.derived_sizes_certified &&
         verification.candidates_certified &&
         verification.events_certified &&
         verification.extra_shell_degeneracies_certified &&
         verification.h0_batches_certified &&
         verification.predicate_counters_certified &&
         verification.result_facts_certified &&
         verification.counters_certified &&
         verification.decision_certified &&
         verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.exact_critical_catalog_decision_certified;
}

[[nodiscard]] std::size_t outcome_count(
    const ExactCriticalCatalogResult& result,
    ExactCriticalCatalogCandidateOutcome outcome) {
  return static_cast<std::size_t>(std::count_if(
      result.candidates.begin(),
      result.candidates.end(),
      [outcome](const ExactCriticalCatalogCandidate& candidate) {
        return candidate.outcome == outcome;
      }));
}

[[nodiscard]] const ExactCriticalEvent* event_with_shell(
    const ExactCriticalCatalogResult& result,
    const std::vector<PointId>& shell) {
  const auto iterator = std::find_if(
      result.events.begin(),
      result.events.end(),
      [&shell](const ExactCriticalEvent& event) {
        return event.shell_point_ids == shell;
      });
  return iterator == result.events.end() ? nullptr : &*iterator;
}

[[nodiscard]] const ExactCriticalCatalogCandidate* candidate_with_support(
    const ExactCriticalCatalogResult& result,
    const std::vector<PointId>& support) {
  const auto iterator = std::find_if(
      result.candidates.begin(),
      result.candidates.end(),
      [&support](const ExactCriticalCatalogCandidate& candidate) {
        return candidate.support_point_ids == support;
      });
  return iterator == result.candidates.end() ? nullptr : &*iterator;
}

[[nodiscard]] const ExactCriticalH0Batch* batch_at(
    const ExactCriticalCatalogResult& result,
    std::size_t order,
    const ExactLevel& squared_level) {
  const auto iterator = std::find_if(
      result.h0_batches.begin(),
      result.h0_batches.end(),
      [order, &squared_level](const ExactCriticalH0Batch& batch) {
        return batch.order == order && batch.squared_level == squared_level;
      });
  return iterator == result.h0_batches.end() ? nullptr : &*iterator;
}

[[nodiscard]] bool center_less(
    const morsehgp3d::exact::ExactCenter3& left,
    const morsehgp3d::exact::ExactCenter3& right) {
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (left.coordinate(axis) != right.coordinate(axis)) {
      return left.coordinate(axis) < right.coordinate(axis);
    }
  }
  return false;
}

[[nodiscard]] bool canonical_event_less(
    const ExactCriticalEvent& left,
    const ExactCriticalEvent& right) {
  if (left.squared_level != right.squared_level) {
    return left.squared_level < right.squared_level;
  }
  if (left.closed_rank != right.closed_rank) {
    return left.closed_rank < right.closed_rank;
  }
  if (left.interior_point_ids != right.interior_point_ids) {
    return left.interior_point_ids < right.interior_point_ids;
  }
  if (left.shell_point_ids != right.shell_point_ids) {
    return left.shell_point_ids < right.shell_point_ids;
  }
  if (left.support_point_ids != right.support_point_ids) {
    return left.support_point_ids < right.support_point_ids;
  }
  return center_less(left.center, right.center);
}

void check_complete_generic_catalog(
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactCriticalCatalogResult& result,
    const std::string& fixture) {
  const ExactCriticalCatalogVerification verification =
      verify_exact_critical_catalog(
          cloud, requested_maximum_order, full_budget(), result);
  check(
      result.candidate_space_size_certified &&
          result.preflight_budget_sufficient &&
          result.geometry_started_after_successful_preflight &&
          result.all_support_candidates_classified &&
          result.global_closed_ball_queries_restricted_to_minimal_supports &&
          result.all_minimal_support_global_partitions_complete &&
          result.extra_shell_degeneracies_deduplicated &&
          result.accepted_events_canonical_and_indexed &&
          result.h0_batches_canonical_and_complete &&
          result.no_relevant_extra_shell_degeneracy &&
          result.decision == ExactCriticalCatalogDecision::
                                 complete_supported_critical_catalog &&
          result.scope == ExactCriticalCatalogScope::
                              bounded_n14_k10_exhaustive_supports_up_to_four_critical_catalog_h0_batches_only &&
          all_certificates_close(verification),
      fixture + " closes the complete generic-catalogue certificate");
}

void test_two_points_exhaust_all_three_supports() {
  const std::array<CertifiedPoint3, 2> input{point(2.0), point(0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactCriticalCatalogResult result =
      build_exact_critical_catalog(cloud, 10U, full_budget());

  check(
      result.point_count == 2U && result.effective_maximum_order == 2U &&
          result.maximum_relevant_closed_rank == 2U &&
          result.required_candidate_count == 3U &&
          result.required_point_classification_count == 6U &&
          result.candidates.size() == 3U && result.events.size() == 3U &&
          result.counters.enumerated_candidate_count == 3U &&
          result.counters.minimal_support_count == 3U &&
          result.counters.global_closed_ball_query_count == 3U &&
          result.counters.global_point_classification_count == 6U &&
          result.counters.accepted_event_count == 3U &&
          outcome_count(
              result,
              ExactCriticalCatalogCandidateOutcome::
                  accepted_critical_event) == 3U,
      "two points exhaust two singleton supports and their pair");

  const ExactCriticalEvent* pair = event_with_shell(result, {0U, 1U});
  check(
      pair != nullptr && pair->support_point_ids == std::vector<PointId>({0U, 1U}) &&
          pair->interior_point_ids.empty() && pair->closed_rank == 2U &&
          pair->squared_level == level(1) && pair->birth_order == 2U &&
          pair->saddle_order == 1U,
      "the pair retains its exact shared birth-and-saddle roles");
  check_complete_generic_catalog(cloud, 10U, result, "two points");
}

void test_acute_triangle_accepts_every_support() {
  const std::array<CertifiedPoint3, 3> input{
      point(0.0, 0.0), point(4.0, 0.0), point(2.0, 3.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactCriticalCatalogResult result =
      build_exact_critical_catalog(cloud, 3U, full_budget());

  check(
      result.required_candidate_count == 7U &&
          result.required_point_classification_count == 21U &&
          result.candidates.size() == 7U && result.events.size() == 7U &&
          result.counters.minimal_support_count == 7U &&
          result.counters.accepted_event_count == 7U &&
          result.counters.affinely_dependent_support_count == 0U &&
          result.counters.boundary_reduced_support_count == 0U &&
          result.counters.exterior_circumcenter_support_count == 0U &&
          result.counters.relevant_extra_shell_candidate_count == 0U &&
          outcome_count(
              result,
              ExactCriticalCatalogCandidateOutcome::
                  accepted_critical_event) == 7U,
      "an acute triangle accepts all seven one-to-three-point supports");

  const ExactCriticalEvent* triangle =
      event_with_shell(result, {0U, 1U, 2U});
  check(
      triangle != nullptr && triangle->support_point_ids ==
                                 std::vector<PointId>({0U, 1U, 2U}) &&
          triangle->interior_point_ids.empty() &&
          triangle->closed_rank == 3U &&
          triangle->squared_level == level(169, 36) &&
          triangle->birth_order == 3U && triangle->saddle_order == 2U,
      "the acute circumtriangle is an exact rank-three event");
  check_complete_generic_catalog(cloud, 3U, result, "acute triangle");
}

void test_obtuse_triangle_uses_the_diameter_event() {
  const std::array<CertifiedPoint3, 3> input{
      point(0.0, 0.0), point(4.0, 0.0), point(1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactCriticalCatalogResult result =
      build_exact_critical_catalog(cloud, 3U, full_budget());

  check(
      result.candidates.size() == 7U && result.events.size() == 6U &&
          result.counters.minimal_support_count == 6U &&
          result.counters.exterior_circumcenter_support_count == 1U &&
          result.counters.accepted_event_count == 6U &&
          outcome_count(
              result,
              ExactCriticalCatalogCandidateOutcome::
                  exterior_circumcenter_support) == 1U &&
          outcome_count(
              result,
              ExactCriticalCatalogCandidateOutcome::
                  accepted_critical_event) == 6U,
      "the obtuse three-point support is the sole non-well-centred candidate");

  const ExactCriticalEvent* event = event_with_shell(result, {0U, 2U});
  check(
      event != nullptr &&
          event->support_point_ids == std::vector<PointId>({0U, 2U}) &&
          event->interior_point_ids == std::vector<PointId>({1U}) &&
          event->closed_point_ids ==
              std::vector<PointId>({0U, 1U, 2U}) &&
          event->closed_rank == 3U && event->squared_level == level(4) &&
          event->birth_order == 3U && event->saddle_order == 2U,
      "the obtuse triangle's diameter closes one interior point at level four");
  check_complete_generic_catalog(cloud, 3U, result, "obtuse triangle");
}

void test_right_triangle_reports_the_relevant_extra_shell() {
  const std::array<CertifiedPoint3, 3> input{
      point(0.0, 0.0), point(2.0, 0.0), point(0.0, 2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactCriticalCatalogResult result =
      build_exact_critical_catalog(cloud, 3U, full_budget());
  const ExactCriticalCatalogVerification verification =
      verify_exact_critical_catalog(cloud, 3U, full_budget(), result);

  check(
      result.candidates.size() == 7U && result.events.size() == 5U &&
          result.counters.minimal_support_count == 6U &&
          result.counters.boundary_reduced_support_count == 1U &&
          result.counters.exterior_circumcenter_support_count == 0U &&
          result.counters.relevant_extra_shell_candidate_count == 1U &&
          result.counters.deduplicated_extra_shell_degeneracy_count == 1U &&
          result.counters.accepted_event_count == 5U &&
          outcome_count(
              result,
              ExactCriticalCatalogCandidateOutcome::
                  boundary_reduced_support) == 1U &&
          outcome_count(
              result,
              ExactCriticalCatalogCandidateOutcome::
                  relevant_extra_shell_degeneracy) == 1U &&
          outcome_count(
              result,
              ExactCriticalCatalogCandidateOutcome::
                  accepted_critical_event) == 5U,
      "the right triangle separates boundary reduction from the extra-shell equality");

  check(
      result.extra_shell_degeneracies.size() == 1U &&
          result.extra_shell_degeneracies[0].interior_point_ids.empty() &&
          result.extra_shell_degeneracies[0].shell_point_ids ==
              std::vector<PointId>({0U, 1U, 2U}) &&
          result.extra_shell_degeneracies[0].closed_point_ids ==
              std::vector<PointId>({0U, 1U, 2U}) &&
          result.extra_shell_degeneracies[0].observed_closed_rank == 3U &&
          result.extra_shell_degeneracies[0].support_point_id_sets ==
              std::vector<std::vector<PointId>>({{1U, 2U}}) &&
          result.extra_shell_degeneracies[0]
                  .relevant_support_candidate_indices.size() == 1U &&
          result.extra_shell_degeneracies[0].has_relevant_support &&
          event_with_shell(result, {0U, 1U, 2U}) == nullptr,
      "the hypotenuse equality is explicit and never perturbed into an event");
  check(
      !result.no_relevant_extra_shell_degeneracy &&
          result.decision == ExactCriticalCatalogDecision::
                                 complete_catalog_with_relevant_extra_shell_degeneracy &&
          all_certificates_close(verification),
      "a relevant equality closes a complete degenerate-domain catalogue");
}

void test_mirror_fixture_keeps_both_simultaneous_events() {
  const std::array<CertifiedPoint3, 4> input{
      point(-2.0, 0.0),
      point(0.0, -3.0),
      point(0.0, 3.0),
      point(2.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactCriticalCatalogResult result =
      build_exact_critical_catalog(cloud, 4U, full_budget());

  check(
      result.required_candidate_count == 15U &&
          result.candidates.size() == 15U && result.events.size() == 12U &&
          result.counters.affinely_dependent_support_count == 1U &&
          result.counters.exterior_circumcenter_support_count == 2U &&
          result.counters.minimal_support_count == 12U &&
          result.counters.accepted_event_count == 12U &&
          outcome_count(
              result,
              ExactCriticalCatalogCandidateOutcome::
                  affinely_dependent_support) == 1U &&
          outcome_count(
              result,
              ExactCriticalCatalogCandidateOutcome::
                  exterior_circumcenter_support) == 2U &&
          outcome_count(
              result,
              ExactCriticalCatalogCandidateOutcome::
                  accepted_critical_event) == 12U,
      "the mirror fixture classifies all fifteen supports without sequentializing equality");

  const ExactCriticalEvent* lower =
      event_with_shell(result, {0U, 1U, 3U});
  const ExactCriticalEvent* upper =
      event_with_shell(result, {0U, 2U, 3U});
  check(
      lower != nullptr && upper != nullptr &&
          lower->squared_level == level(169, 36) &&
          upper->squared_level == level(169, 36) &&
          lower->closed_rank == 3U && upper->closed_rank == 3U &&
          lower->saddle_order == 2U && upper->saddle_order == 2U,
      "the two distinct triangular events share the exact saddle pair (2,169/36)");

  if (lower != nullptr && upper != nullptr) {
    std::vector<std::size_t> expected_indices{
        lower->event_index, upper->event_index};
    std::sort(expected_indices.begin(), expected_indices.end());
    const ExactCriticalH0Batch* saddle_batch =
        batch_at(result, 2U, level(169, 36));
    const ExactCriticalH0Batch* birth_batch =
        batch_at(result, 3U, level(169, 36));
    check(
        saddle_batch != nullptr && birth_batch != nullptr &&
            saddle_batch->saddle_event_indices == expected_indices &&
            saddle_batch->birth_event_indices.empty() &&
            birth_batch->birth_event_indices == expected_indices &&
            birth_batch->saddle_event_indices.empty(),
        "canonical H0 batches preserve both simultaneous event references");
  }
  check_complete_generic_catalog(cloud, 4U, result, "mirror fixture");
}

void test_eleven_collinear_points_reach_the_rank_eleven_boundary() {
  std::vector<CertifiedPoint3> input;
  input.reserve(11U);
  for (std::size_t index = 0U; index < 11U; ++index) {
    input.push_back(point(static_cast<double>(index)));
  }
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactCriticalCatalogResult result =
      build_exact_critical_catalog(cloud, 10U, full_budget());

  check(
      result.required_candidate_count == 561U &&
          result.required_point_classification_count == 6171U &&
          result.candidates.size() == 561U && result.events.size() == 66U &&
          result.counters.affinely_dependent_support_count == 495U &&
          result.counters.minimal_support_count == 66U &&
          result.counters.global_closed_ball_query_count == 66U &&
          result.counters.global_point_classification_count == 726U &&
          result.counters.accepted_event_count == 66U &&
          outcome_count(
              result,
              ExactCriticalCatalogCandidateOutcome::
                  affinely_dependent_support) == 495U &&
          outcome_count(
              result,
              ExactCriticalCatalogCandidateOutcome::
                  accepted_critical_event) == 66U,
      "the collinear fixture exhausts 561 supports but queries only 66 minimal ones");

  const ExactCriticalEvent* terminal = event_with_shell(result, {0U, 10U});
  check(
      terminal != nullptr && terminal->interior_point_ids ==
                                 std::vector<PointId>(
                                     {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U}) &&
          terminal->closed_rank == 11U &&
          terminal->squared_level == level(25) &&
          !terminal->birth_order.has_value() && terminal->saddle_order == 10U,
      "the diameter closes rank eleven with a terminal saddle but no birth");
  check_complete_generic_catalog(cloud, 10U, result, "eleven-point line");
}

void test_events_follow_the_mathematical_canonical_key() {
  const std::array<CertifiedPoint3, 4> input{
      point(0.0), point(2.0), point(10.0), point(12.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactCriticalCatalogResult result =
      build_exact_critical_catalog(cloud, 4U, full_budget());

  check(
      std::is_sorted(
          result.events.begin(), result.events.end(), canonical_event_less),
      "events are sorted by (level,rank,interior,shell,support,center), not support enumeration");
  const std::vector<const ExactCriticalEvent*> level_one_events = [&result] {
    std::vector<const ExactCriticalEvent*> events;
    for (const ExactCriticalEvent& event : result.events) {
      if (event.squared_level == level(1)) {
        events.push_back(&event);
      }
    }
    return events;
  }();
  check(
      level_one_events.size() == 2U &&
          level_one_events[0]->shell_point_ids ==
              std::vector<PointId>({0U, 1U}) &&
          level_one_events[1]->shell_point_ids ==
              std::vector<PointId>({2U, 3U}),
      "equal-level events retain canonical shell ordering");
  for (std::size_t index = 0U; index < result.events.size(); ++index) {
    check(
        result.events[index].event_index == index,
        "canonical event indices are reassigned after sorting");
  }
  check_complete_generic_catalog(
      cloud, 4U, result, "four-point canonical-order fixture");

  ExactCriticalCatalogResult bad_order = result;
  std::swap(bad_order.events[0], bad_order.events[1]);
  const ExactCriticalCatalogVerification bad_order_verification =
      verify_exact_critical_catalog(
          cloud, 4U, full_budget(), bad_order);
  check(
      !bad_order_verification.events_certified &&
          !bad_order_verification.fresh_replay_certified &&
          !bad_order_verification.exact_critical_catalog_decision_certified,
      "the fresh verifier rejects a permutation of canonical events");

  ExactCriticalCatalogResult bad_index = result;
  ++bad_index.events[0].event_index;
  const ExactCriticalCatalogVerification bad_index_verification =
      verify_exact_critical_catalog(
          cloud, 4U, full_budget(), bad_index);
  check(
      !bad_index_verification.events_certified &&
          !bad_index_verification.fresh_replay_certified &&
          !bad_index_verification.exact_critical_catalog_decision_certified,
      "the fresh verifier rejects a forged canonical event index");
}

void test_extra_shell_relevance_uses_the_input_support_rank() {
  const std::array<CertifiedPoint3, 3> input{
      point(-1.0, 0.0), point(0.0, 1.0), point(1.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactCriticalCatalogResult result =
      build_exact_critical_catalog(cloud, 1U, full_budget());
  const ExactCriticalCatalogCandidate* diameter =
      candidate_with_support(result, {0U, 2U});
  const ExactCriticalCatalogVerification verification =
      verify_exact_critical_catalog(cloud, 1U, full_budget(), result);

  check(
      result.maximum_relevant_closed_rank == 2U && diameter != nullptr &&
          diameter->support_point_ids == std::vector<PointId>({0U, 2U}) &&
          diameter->interior_point_ids.empty() &&
          diameter->shell_point_ids ==
              std::vector<PointId>({0U, 1U, 2U}) &&
          diameter->support_relevance_rank == 2U &&
          diameter->observed_closed_rank == 3U &&
          diameter->outcome == ExactCriticalCatalogCandidateOutcome::
                                   relevant_extra_shell_degeneracy &&
          diameter->extra_shell_degeneracy_index.has_value() &&
          !diameter->event_index.has_value(),
      "RelevantGP uses |I| plus the input support size even when the observed shell exceeds s_max");
  check(
      result.counters.relevant_extra_shell_candidate_count == 1U &&
          result.counters.outside_window_extra_shell_candidate_count == 0U &&
          !result.no_relevant_extra_shell_degeneracy &&
          result.decision == ExactCriticalCatalogDecision::
                                 complete_catalog_with_relevant_extra_shell_degeneracy &&
          all_certificates_close(verification),
      "the rank-two support remains a relevant obstruction despite observed rank three");
}

void test_original_outside_window_hypothesis_preserves_its_contradiction() {
  const std::array<CertifiedPoint3, 4> input{
      point(-2.0, 0.0),
      point(0.0, 0.0),
      point(0.0, 2.0),
      point(2.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactCriticalCatalogResult result =
      build_exact_critical_catalog(cloud, 1U, full_budget());
  const ExactCriticalCatalogCandidate* diameter =
      candidate_with_support(result, {0U, 3U});
  const ExactCriticalCatalogCandidate* left_lateral =
      candidate_with_support(result, {0U, 2U});
  const ExactCriticalCatalogCandidate* right_lateral =
      candidate_with_support(result, {2U, 3U});
  const ExactCriticalCatalogVerification verification =
      verify_exact_critical_catalog(cloud, 1U, full_budget(), result);

  check(
      diameter != nullptr && diameter->interior_point_ids ==
                                 std::vector<PointId>({1U}) &&
          diameter->shell_point_ids ==
              std::vector<PointId>({0U, 2U, 3U}) &&
          diameter->support_relevance_rank == 3U &&
          diameter->observed_closed_rank == 4U &&
          diameter->outcome == ExactCriticalCatalogCandidateOutcome::
                                   extra_shell_outside_relevant_window,
      "the horizontal diameter is an explicit outside-window extra shell");
  check(
      left_lateral != nullptr && right_lateral != nullptr &&
          left_lateral->shell_point_ids ==
              std::vector<PointId>({0U, 1U, 2U}) &&
          right_lateral->shell_point_ids ==
              std::vector<PointId>({1U, 2U, 3U}) &&
          left_lateral->support_relevance_rank == 2U &&
          right_lateral->support_relevance_rank == 2U &&
          left_lateral->outcome == ExactCriticalCatalogCandidateOutcome::
                                       relevant_extra_shell_degeneracy &&
          right_lateral->outcome == ExactCriticalCatalogCandidateOutcome::
                                        relevant_extra_shell_degeneracy,
      "the centre point also creates two relevant lateral shell equalities");
  check(
      result.extra_shell_degeneracies.size() == 3U &&
          result.counters.relevant_extra_shell_candidate_count == 2U &&
          result.counters.outside_window_extra_shell_candidate_count == 1U &&
          !result.no_relevant_extra_shell_degeneracy &&
          result.decision == ExactCriticalCatalogDecision::
                                 complete_catalog_with_relevant_extra_shell_degeneracy &&
          all_certificates_close(verification),
      "the permanent contradiction fixture cannot be certified generic");
}

void test_corrected_outside_window_degeneracy_is_nonblocking() {
  const std::array<CertifiedPoint3, 4> input{
      point(-2.0, 0.0),
      point(0.0, 0.5),
      point(0.0, 2.0),
      point(2.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactCriticalCatalogResult result =
      build_exact_critical_catalog(cloud, 1U, full_budget());
  const ExactCriticalCatalogCandidate* diameter =
      candidate_with_support(result, {0U, 3U});

  check(
      diameter != nullptr && diameter->interior_point_ids ==
                                 std::vector<PointId>({1U}) &&
          diameter->shell_point_ids ==
              std::vector<PointId>({0U, 2U, 3U}) &&
          diameter->support_relevance_rank == 3U &&
          diameter->observed_closed_rank == 4U &&
          diameter->outcome == ExactCriticalCatalogCandidateOutcome::
                                   extra_shell_outside_relevant_window &&
          diameter->extra_shell_degeneracy_index.has_value(),
      "moving the interior point to one half preserves the intended outside-window equality");
  check(
      result.extra_shell_degeneracies.size() == 1U &&
          !result.extra_shell_degeneracies[0].has_relevant_support &&
          result.counters.relevant_extra_shell_candidate_count == 0U &&
          result.counters.outside_window_extra_shell_candidate_count == 1U &&
          result.no_relevant_extra_shell_degeneracy,
      "the corrected outside-window degeneracy is retained without blocking RelevantGP");
  check_complete_generic_catalog(
      cloud, 1U, result, "corrected outside-window fixture");
}

void test_multi_size_supports_aggregate_into_one_degeneracy() {
  const std::array<CertifiedPoint3, 5> input{
      point(-5.0, 0.0),
      point(0.0, -5.0),
      point(0.0, 5.0),
      point(3.0, -4.0),
      point(3.0, 4.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactCriticalCatalogResult result =
      build_exact_critical_catalog(cloud, 5U, full_budget());
  const ExactCriticalCatalogCandidate* pair =
      candidate_with_support(result, {1U, 2U});
  const ExactCriticalCatalogCandidate* triangle =
      candidate_with_support(result, {0U, 3U, 4U});

  check(
      pair != nullptr && triangle != nullptr &&
          pair->outcome == ExactCriticalCatalogCandidateOutcome::
                               relevant_extra_shell_degeneracy &&
          triangle->outcome == ExactCriticalCatalogCandidateOutcome::
                                   relevant_extra_shell_degeneracy &&
          pair->center.has_value() &&
          pair->center->coordinate(0U).is_zero() &&
          pair->center->coordinate(1U).is_zero() &&
          pair->center->coordinate(2U).is_zero() &&
          triangle->center == pair->center &&
          pair->squared_level == level(25) &&
          triangle->squared_level == pair->squared_level &&
          pair->interior_point_ids.empty() &&
          triangle->interior_point_ids.empty() &&
          pair->shell_point_ids ==
              std::vector<PointId>({0U, 1U, 2U, 3U, 4U}) &&
          triangle->shell_point_ids == pair->shell_point_ids &&
          pair->extra_shell_degeneracy_index.has_value() &&
          triangle->extra_shell_degeneracy_index ==
              pair->extra_shell_degeneracy_index,
      "the diameter and positive triangle expose the same exact five-point shell");

  if (pair != nullptr && triangle != nullptr &&
      pair->extra_shell_degeneracy_index.has_value() &&
      triangle->extra_shell_degeneracy_index ==
          pair->extra_shell_degeneracy_index) {
    const std::size_t degeneracy_index =
        *pair->extra_shell_degeneracy_index;
    check(
        degeneracy_index < result.extra_shell_degeneracies.size(),
        "the shared degeneracy index addresses the canonical catalogue");
    if (degeneracy_index < result.extra_shell_degeneracies.size()) {
      const auto& degeneracy =
          result.extra_shell_degeneracies[degeneracy_index];
      const auto association_is_present =
          [&degeneracy](
              const std::vector<PointId>& support,
              std::size_t candidate_index) {
            for (std::size_t position = 0U;
                 position < degeneracy.support_point_id_sets.size();
                 ++position) {
              if (degeneracy.support_point_id_sets[position] == support) {
                return position <
                           degeneracy.support_candidate_indices.size() &&
                       degeneracy.support_candidate_indices[position] ==
                           candidate_index;
              }
            }
            return false;
          };
      check(
          degeneracy.degeneracy_index == degeneracy_index &&
              degeneracy.squared_level == level(25) &&
              degeneracy.interior_point_ids.empty() &&
              degeneracy.shell_point_ids ==
                  std::vector<PointId>({0U, 1U, 2U, 3U, 4U}) &&
              degeneracy.closed_point_ids == degeneracy.shell_point_ids &&
              degeneracy.observed_closed_rank == 5U &&
              degeneracy.has_relevant_support &&
              association_is_present(
                  pair->support_point_ids, pair->candidate_index) &&
              association_is_present(
                  triangle->support_point_ids,
                  triangle->candidate_index) &&
              std::binary_search(
                  degeneracy.relevant_support_candidate_indices.begin(),
                  degeneracy.relevant_support_candidate_indices.end(),
                  pair->candidate_index) &&
              std::binary_search(
                  degeneracy.relevant_support_candidate_indices.begin(),
                  degeneracy.relevant_support_candidate_indices.end(),
                  triangle->candidate_index),
          "one canonical degeneracy preserves both support sizes and their candidate associations");
    }
  }

  const ExactCriticalCatalogVerification verification =
      verify_exact_critical_catalog(cloud, 5U, full_budget(), result);
  check(
      !result.no_relevant_extra_shell_degeneracy &&
          result.decision == ExactCriticalCatalogDecision::
                                 complete_catalog_with_relevant_extra_shell_degeneracy &&
          all_certificates_close(verification),
      "the multi-support degeneracy survives a fresh exhaustive replay");
}

void test_minimal_supports_above_the_rank_window_remain_explicit() {
  const std::array<CertifiedPoint3, 4> input{
      point(0.0), point(1.0), point(3.0), point(10.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactCriticalCatalogResult result =
      build_exact_critical_catalog(cloud, 1U, full_budget());
  const ExactCriticalCatalogCandidate* extreme =
      candidate_with_support(result, {0U, 3U});

  check(
      result.maximum_relevant_closed_rank == 2U && extreme != nullptr &&
          extreme->support_point_ids ==
              std::vector<PointId>({0U, 3U}) &&
          extreme->interior_point_ids ==
              std::vector<PointId>({1U, 2U}) &&
          extreme->shell_point_ids ==
              std::vector<PointId>({0U, 3U}) &&
          extreme->closed_point_ids ==
              std::vector<PointId>({0U, 1U, 2U, 3U}) &&
          extreme->support_relevance_rank == 4U &&
          extreme->observed_closed_rank == 4U &&
          extreme->global_closed_ball_classified &&
          extreme->outcome == ExactCriticalCatalogCandidateOutcome::
                                  minimal_support_above_rank_window &&
          !extreme->event_index.has_value() &&
          !extreme->extra_shell_degeneracy_index.has_value(),
      "the extreme pair is minimal with complete shell but lies strictly above s_max");
  check(
      result.required_candidate_count == 15U &&
          result.counters.minimal_support_count == 10U &&
          result.counters.global_closed_ball_query_count == 10U &&
          result.counters.global_point_classification_count == 40U &&
          result.counters.affinely_dependent_support_count == 5U &&
          result.counters.above_rank_candidate_count == 3U &&
          result.counters.accepted_event_count == 7U &&
          result.extra_shell_degeneracies.empty(),
      "the generic line accounts separately for dependent, above-window and accepted supports");

  constexpr std::array<ExactCriticalCatalogCandidateOutcome, 8> outcomes{
      ExactCriticalCatalogCandidateOutcome::not_classified,
      ExactCriticalCatalogCandidateOutcome::affinely_dependent_support,
      ExactCriticalCatalogCandidateOutcome::boundary_reduced_support,
      ExactCriticalCatalogCandidateOutcome::exterior_circumcenter_support,
      ExactCriticalCatalogCandidateOutcome::relevant_extra_shell_degeneracy,
      ExactCriticalCatalogCandidateOutcome::
          extra_shell_outside_relevant_window,
      ExactCriticalCatalogCandidateOutcome::minimal_support_above_rank_window,
      ExactCriticalCatalogCandidateOutcome::accepted_critical_event};
  std::size_t classified_candidate_count = 0U;
  for (const ExactCriticalCatalogCandidateOutcome outcome : outcomes) {
    classified_candidate_count += outcome_count(result, outcome);
  }
  check(
      outcome_count(
          result, ExactCriticalCatalogCandidateOutcome::not_classified) ==
              0U &&
          classified_candidate_count == result.required_candidate_count &&
          classified_candidate_count == result.candidates.size(),
      "the eight candidate outcomes form one exhaustive disjoint accounting");
  check_complete_generic_catalog(
      cloud, 1U, result, "generic above-rank line fixture");
}

void check_atomic_preflight_result(
    const CanonicalPointCloud& cloud,
    ExactCriticalCatalogBudget budget,
    const std::string& dimension) {
  const ExactCriticalCatalogResult result =
      build_exact_critical_catalog(cloud, 10U, budget);
  const ExactCriticalCatalogVerification verification =
      verify_exact_critical_catalog(cloud, 10U, budget, result);

  check(
      result.point_count == 14U && result.required_candidate_count == 1470U &&
          result.required_point_classification_count == 20580U &&
          result.candidate_space_size_certified &&
          !result.preflight_budget_sufficient &&
          !result.geometry_started_after_successful_preflight &&
          result.candidates.empty() && result.events.empty() &&
          result.extra_shell_degeneracies.empty() && result.h0_batches.empty() &&
          result.counters.preflight_count == 1U &&
          result.counters.enumerated_candidate_count == 0U &&
          result.counters.support_analysis_count == 0U &&
          result.counters.global_closed_ball_query_count == 0U &&
          result.counters.global_point_classification_count == 0U &&
          result.decision == ExactCriticalCatalogDecision::
                                 no_catalog_preflight_budget_insufficient &&
          all_certificates_close(verification),
      "the n=14 " + dimension + " budget failure is atomic at 1470/20580");

  ExactCriticalCatalogResult forged_geometry = result;
  forged_geometry.geometry_started_after_successful_preflight = true;
  const ExactCriticalCatalogVerification forged_verification =
      verify_exact_critical_catalog(
          cloud, 10U, budget, forged_geometry);
  check(
      !forged_verification.result_facts_certified &&
          !forged_verification.fresh_replay_certified &&
          !forged_verification.exact_critical_catalog_decision_certified,
      "the fresh verifier rejects geometry claimed after a failed " +
          dimension + " preflight");
}

void test_n14_preflight_is_exact_and_atomic() {
  std::vector<CertifiedPoint3> input;
  input.reserve(14U);
  for (std::size_t index = 0U; index < 14U; ++index) {
    input.push_back(point(static_cast<double>(index)));
  }
  const CanonicalPointCloud cloud = canonical_cloud(input);
  check_atomic_preflight_result(
      cloud,
      ExactCriticalCatalogBudget{1469U, 20580U},
      "candidate-count");
  check_atomic_preflight_result(
      cloud,
      ExactCriticalCatalogBudget{1470U, 20579U},
      "point-classification");
}

void test_fresh_verifier_rejects_each_catalogue_layer() {
  const std::array<CertifiedPoint3, 3> right_input{
      point(0.0, 0.0), point(2.0, 0.0), point(0.0, 2.0)};
  const CanonicalPointCloud right_cloud = canonical_cloud(right_input);
  const ExactCriticalCatalogResult right =
      build_exact_critical_catalog(right_cloud, 3U, full_budget());

  const auto rejects_right =
      [&right_cloud](
          const ExactCriticalCatalogResult& observed,
          bool ExactCriticalCatalogVerification::*layer,
          const std::string& message) {
        const ExactCriticalCatalogVerification verification =
            verify_exact_critical_catalog(
                right_cloud, 3U, full_budget(), observed);
        check(
            !(verification.*layer) && !verification.fresh_replay_certified &&
                !verification.exact_critical_catalog_decision_certified,
            message);
      };

  ExactCriticalCatalogResult bad_classification = right;
  bad_classification.candidates[0].outcome =
      ExactCriticalCatalogCandidateOutcome::not_classified;
  rejects_right(
      bad_classification,
      &ExactCriticalCatalogVerification::candidates_certified,
      "the verifier replays every candidate classification");

  ExactCriticalCatalogResult bad_event = right;
  ++bad_event.events[0].closed_rank;
  rejects_right(
      bad_event,
      &ExactCriticalCatalogVerification::events_certified,
      "the verifier replays every canonical critical event");

  ExactCriticalCatalogResult bad_degeneracy = right;
  bad_degeneracy.extra_shell_degeneracies[0].has_relevant_support = false;
  rejects_right(
      bad_degeneracy,
      &ExactCriticalCatalogVerification::
          extra_shell_degeneracies_certified,
      "the verifier replays every relevant extra-shell degeneracy");

  ExactCriticalCatalogResult bad_batch = right;
  ++bad_batch.h0_batches[0].order;
  rejects_right(
      bad_batch,
      &ExactCriticalCatalogVerification::h0_batches_certified,
      "the verifier replays every exact H0 batch");

  ExactCriticalCatalogResult bad_counter = right;
  ++bad_counter.counters.support_analysis_count;
  rejects_right(
      bad_counter,
      &ExactCriticalCatalogVerification::counters_certified,
      "the verifier replays the exhaustive work accounting");

  const ExactCriticalCatalogVerification wrong_budget =
      verify_exact_critical_catalog(
          right_cloud,
          3U,
          ExactCriticalCatalogBudget{6U, 21U},
          right);
  check(
      !wrong_budget.requested_budget_certified &&
          !wrong_budget.fresh_replay_certified &&
          !wrong_budget.exact_critical_catalog_decision_certified,
      "the verifier never trusts a catalogue built under another budget");
}

}  // namespace

int main() {
  test_two_points_exhaust_all_three_supports();
  test_acute_triangle_accepts_every_support();
  test_obtuse_triangle_uses_the_diameter_event();
  test_right_triangle_reports_the_relevant_extra_shell();
  test_mirror_fixture_keeps_both_simultaneous_events();
  test_eleven_collinear_points_reach_the_rank_eleven_boundary();
  test_events_follow_the_mathematical_canonical_key();
  test_extra_shell_relevance_uses_the_input_support_rank();
  test_original_outside_window_hypothesis_preserves_its_contradiction();
  test_corrected_outside_window_degeneracy_is_nonblocking();
  test_multi_size_supports_aggregate_into_one_degeneracy();
  test_minimal_supports_above_the_rank_window_remain_explicit();
  test_n14_preflight_is_exact_and_atomic();
  test_fresh_verifier_rejects_each_catalogue_layer();

  if (failures != 0) {
    std::cerr << failures << " test assertion(s) failed\n";
    return 1;
  }
  std::cout << "all exact critical-catalogue tests passed\n";
  return 0;
}
