#include "morsehgp3d/hierarchy/critical_event_gamma_overlay.hpp"

#include <array>
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

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactCriticalEventGammaOverlayBudget;
using morsehgp3d::hierarchy::ExactCriticalEventGammaOverlayDecision;
using morsehgp3d::hierarchy::ExactCriticalEventGammaOverlayRequest;
using morsehgp3d::hierarchy::ExactCriticalEventGammaOverlayResult;
using morsehgp3d::hierarchy::ExactCriticalEventGammaOverlayScope;
using morsehgp3d::hierarchy::ExactCriticalEventGammaOverlayVerification;
using morsehgp3d::hierarchy::ExactFacetDescentChainBudget;
using morsehgp3d::hierarchy::ExactGammaTransitionGroupKind;
using morsehgp3d::hierarchy::ExactStrictGammaBudget;
using morsehgp3d::hierarchy::build_exact_supplied_critical_event_gamma_overlay;
using morsehgp3d::hierarchy::verify_exact_supplied_critical_event_gamma_overlay;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

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
    std::cerr << "FAIL: " << message
              << " (unexpected exception: " << error.what() << ")\n";
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

[[nodiscard]] ExactCriticalEventGammaOverlayRequest request(
    std::vector<PointId> shell,
    std::size_t chain_budget) {
  return ExactCriticalEventGammaOverlayRequest{
      std::move(shell),
      ExactFacetDescentChainBudget{chain_budget}};
}

[[nodiscard]] bool all_certificates_close(
    const ExactCriticalEventGammaOverlayVerification& verification) {
  return verification.requested_overlay_budget_certified &&
         verification.requested_gamma_budget_certified &&
         verification.canonical_event_requests_certified &&
         verification.preflight_counts_certified &&
         verification.event_classifications_certified &&
         verification.common_order_and_level_certified &&
         verification.gamma_transition_presence_certified &&
         verification.gamma_transition_certified &&
         verification.event_projections_certified &&
         verification.group_overlays_certified &&
         verification.result_facts_certified &&
         verification.counters_certified &&
         verification.decision_certified &&
         verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.
             exact_critical_event_gamma_overlay_decision_certified;
}

[[nodiscard]] CanonicalPointCloud mirror_cloud() {
  const std::array<CertifiedPoint3, 4> input{
      point(-2.0, 0.0),
      point(0.0, -3.0),
      point(0.0, 3.0),
      point(2.0, 0.0)};
  return canonical_cloud(input);
}

[[nodiscard]] std::vector<ExactCriticalEventGammaOverlayRequest>
mirror_requests_reversed() {
  return {
      request({3U, 2U, 0U}, 0U),
      request({3U, 1U, 0U}, 0U)};
}

void test_two_supplied_events_form_one_simultaneous_q5_group() {
  const ExactCriticalEventGammaOverlayResult empty;
  check(
      empty.decision ==
              ExactCriticalEventGammaOverlayDecision::not_certified &&
          empty.scope == ExactCriticalEventGammaOverlayScope::unspecified &&
          empty.canonical_event_requests.empty() &&
          empty.event_classifications.empty() &&
          !empty.gamma_transition.has_value() &&
          empty.event_projections.empty() &&
          empty.group_overlays.empty(),
      "the default supplied-event overlay certifies no transition provenance");

  const CanonicalPointCloud cloud = mirror_cloud();
  const std::vector<ExactCriticalEventGammaOverlayRequest> requests =
      mirror_requests_reversed();
  const ExactCriticalEventGammaOverlayBudget overlay_budget{2U, 6U};
  const ExactStrictGammaBudget gamma_budget{6U, 4U, 8U};
  const ExactCriticalEventGammaOverlayResult result =
      build_exact_supplied_critical_event_gamma_overlay(
          cloud, requests, overlay_budget, gamma_budget);
  const ExactCriticalEventGammaOverlayVerification verification =
      verify_exact_supplied_critical_event_gamma_overlay(
          cloud,
          requests,
          overlay_budget,
          gamma_budget,
          result);

  check(
      result.canonical_event_requests.size() == 2U &&
          result.canonical_event_requests[0].critical_shell_point_ids ==
              std::vector<PointId>({0U, 1U, 3U}) &&
          result.canonical_event_requests[1].critical_shell_point_ids ==
              std::vector<PointId>({0U, 2U, 3U}) &&
          result.common_order == 2U &&
          result.common_squared_level == level(169, 36) &&
          result.event_classifications.size() == 2U &&
          result.gamma_transition.has_value(),
      "the supplied requests are canonicalized before deriving one exact pair");

  check(
      result.gamma_transition->strict_gamma.components.size() == 5U &&
          result.gamma_transition->equal_level_facets.empty() &&
          result.gamma_transition->equal_level_cofaces.size() == 2U &&
          result.gamma_transition->transition_groups.size() == 1U &&
          result.gamma_transition->transition_groups[0]
                  .strict_component_indices ==
              std::vector<std::size_t>({0U, 1U, 2U, 3U, 4U}) &&
          result.gamma_transition->transition_groups[0]
                  .equal_level_coface_point_ids ==
              std::vector<std::vector<PointId>>(
                  {{0U, 1U, 3U}, {0U, 2U, 3U}}) &&
          result.gamma_transition->transition_groups[0].kind ==
              ExactGammaTransitionGroupKind::
                  multiple_strict_component_coalescence,
      "the exhaustive authority is one q=5 group, never two sequential q=3 groups");

  check(
      result.event_projections.size() == 2U &&
          result.event_projections[0].critical_coface_point_ids ==
              std::vector<PointId>({0U, 1U, 3U}) &&
          result.event_projections[0].interior_point_ids.empty() &&
          result.event_projections[0].arm_incidences.size() == 3U &&
          result.event_projections[0].arm_incidences[0]
                  .initial_arm_facet_point_ids ==
              std::vector<PointId>({1U, 3U}) &&
          result.event_projections[0].arm_incidences[0]
                  .strict_gamma_component_index == 3U &&
          result.event_projections[0].arm_incidences[1]
                  .initial_arm_facet_point_ids ==
              std::vector<PointId>({0U, 3U}) &&
          result.event_projections[0].arm_incidences[1]
                  .strict_gamma_component_index == 2U &&
          result.event_projections[0].arm_incidences[2]
                  .initial_arm_facet_point_ids ==
              std::vector<PointId>({0U, 1U}) &&
          result.event_projections[0].arm_incidences[2]
                  .strict_gamma_component_index == 0U,
      "the lower event reconciles each initial arm with its 6.9 strict token");
  check(
      result.event_projections[1].critical_coface_point_ids ==
              std::vector<PointId>({0U, 2U, 3U}) &&
          result.event_projections[1].arm_incidences.size() == 3U &&
          result.event_projections[1].arm_incidences[0]
                  .strict_gamma_component_index == 4U &&
          result.event_projections[1].arm_incidences[1]
                  .strict_gamma_component_index == 2U &&
          result.event_projections[1].arm_incidences[2]
                  .strict_gamma_component_index == 1U &&
          result.event_projections[0].transition_group_index == 0U &&
          result.event_projections[1].transition_group_index == 0U &&
          result.event_projections[0].closed_component_index == 0U &&
          result.event_projections[1].closed_component_index == 0U,
      "both events retain shared-component provenance inside one closed group");

  check(
      result.group_overlays.size() == 1U &&
          result.group_overlays[0].canonical_event_indices ==
              std::vector<std::size_t>({0U, 1U}) &&
          result.group_overlays[0]
              .equal_level_cofaces_without_supplied_event_provenance
              .empty() &&
          result.group_overlays[0].has_supplied_event_provenance &&
          result.counters.required_event_count == 2U &&
          result.counters.required_total_arm_count == 6U &&
          result.counters.event_classification_build_count == 2U &&
          result.counters.complete_event_classification_count == 2U &&
          result.counters.common_order_and_level_comparison_count == 1U &&
          result.counters.strict_gamma_core_comparison_count == 2U &&
          result.counters.deletion_incidence_projection_count == 6U &&
          result.counters.arm_incidence_projection_count == 6U &&
          result.counters.interior_incidence_projection_count == 0U &&
          result.counters.group_supplied_event_reference_count == 2U &&
          result.decision == ExactCriticalEventGammaOverlayDecision::
                                 complete_supplied_event_provenance_overlay &&
          all_certificates_close(verification),
      "the complete supplied provenance overlay and all fresh certificates close");

  const std::vector<ExactCriticalEventGammaOverlayRequest> canonical_order{
      request({0U, 1U, 3U}, 0U),
      request({0U, 2U, 3U}, 0U)};
  const ExactCriticalEventGammaOverlayResult permuted =
      build_exact_supplied_critical_event_gamma_overlay(
          cloud,
          canonical_order,
          overlay_budget,
          gamma_budget);
  check(
      permuted.canonical_event_requests ==
              result.canonical_event_requests &&
          permuted.event_projections == result.event_projections &&
          permuted.group_overlays == result.group_overlays,
      "event order and shell permutations cannot change the overlay");
}

void test_q1_event_and_interior_deletion_are_reconciled() {
  const std::array<CertifiedPoint3, 3> q1_input{
      point(-1.0, 0.0),
      point(0.0, 1.5),
      point(1.0, 0.0)};
  const CanonicalPointCloud q1_cloud = canonical_cloud(q1_input);
  const std::vector<ExactCriticalEventGammaOverlayRequest> q1_requests{
      request({2U, 0U}, 0U)};
  const ExactCriticalEventGammaOverlayResult q1 =
      build_exact_supplied_critical_event_gamma_overlay(
          q1_cloud,
          q1_requests,
          ExactCriticalEventGammaOverlayBudget{1U, 2U},
          ExactStrictGammaBudget{3U, 3U, 3U});
  check(
      q1.common_order == 1U &&
          q1.common_squared_level == level(1) &&
          q1.event_projections.size() == 1U &&
          q1.event_projections[0].arm_incidences.size() == 2U &&
          q1.event_projections[0].arm_incidences[0]
                  .strict_gamma_component_index == 0U &&
          q1.event_projections[0].arm_incidences[1]
                  .strict_gamma_component_index == 0U &&
          q1.event_projections[0].interior_incidences.empty() &&
          q1.gamma_transition->transition_groups.size() == 1U &&
          q1.gamma_transition->transition_groups[0].kind ==
              ExactGammaTransitionGroupKind::
                  one_strict_component_continuation,
      "one complete event may reconcile with a redundant q=1 Gamma incidence");

  const std::array<CertifiedPoint3, 5> interior_input{
      point(-8.0, 1.0),
      point(-5.0, -7.0),
      point(-3.0, -8.0),
      point(4.0, 8.0),
      point(5.0, -7.0)};
  const CanonicalPointCloud interior_cloud = canonical_cloud(interior_input);
  const std::vector<ExactCriticalEventGammaOverlayRequest> interior_requests{
      request({3U, 1U, 2U}, 2U)};
  const ExactCriticalEventGammaOverlayResult interior =
      build_exact_supplied_critical_event_gamma_overlay(
          interior_cloud,
          interior_requests,
          ExactCriticalEventGammaOverlayBudget{1U, 3U},
          ExactStrictGammaBudget{10U, 5U, 15U});
  check(
      interior.common_order == 3U &&
          interior.common_squared_level == level(25925, 338) &&
          interior.event_projections.size() == 1U &&
          interior.event_projections[0].interior_point_ids ==
              std::vector<PointId>({0U}) &&
          interior.event_projections[0].critical_coface_point_ids ==
              std::vector<PointId>({0U, 1U, 2U, 3U}) &&
          interior.event_projections[0].arm_incidences.size() == 3U &&
          interior.event_projections[0].interior_incidences.size() == 1U &&
          interior.event_projections[0].interior_incidences[0]
                  .removed_interior_point_id == 0U &&
          interior.event_projections[0].interior_incidences[0]
                  .equal_level_facet_point_ids ==
              std::vector<PointId>({1U, 2U, 3U}) &&
          interior.counters.deletion_incidence_projection_count == 4U &&
          interior.counters.interior_incidence_projection_count == 1U,
      "an interior deletion is reconciled as a new equality facet, not a strict arm");
}

void test_maximum_order_event_reconciles_shell_and_interior_deletions() {
  const std::array<CertifiedPoint3, 11> input{
      point(0.0), point(1.0), point(2.0), point(3.0),
      point(4.0), point(5.0), point(6.0), point(7.0),
      point(8.0), point(9.0), point(10.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::vector<ExactCriticalEventGammaOverlayRequest> requests{
      request({10U, 0U}, 0U)};
  const ExactCriticalEventGammaOverlayBudget overlay_budget{1U, 2U};
  const ExactStrictGammaBudget gamma_budget{11U, 1U, 10U};
  const ExactCriticalEventGammaOverlayResult result =
      build_exact_supplied_critical_event_gamma_overlay(
          cloud, requests, overlay_budget, gamma_budget);
  const ExactCriticalEventGammaOverlayVerification verification =
      verify_exact_supplied_critical_event_gamma_overlay(
          cloud,
          requests,
          overlay_budget,
          gamma_budget,
          result);

  check(
      result.common_order == 10U &&
          result.common_squared_level == level(25) &&
          result.event_projections.size() == 1U &&
          result.event_projections[0].interior_point_ids ==
              std::vector<PointId>(
                  {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U}) &&
          result.event_projections[0].critical_coface_point_ids ==
              std::vector<PointId>(
                  {0U, 1U, 2U, 3U, 4U, 5U,
                   6U, 7U, 8U, 9U, 10U}) &&
          result.event_projections[0].arm_incidences.size() == 2U &&
          result.event_projections[0].arm_incidences[0]
                  .strict_gamma_component_index == 1U &&
          result.event_projections[0].arm_incidences[1]
                  .strict_gamma_component_index == 0U &&
          result.event_projections[0].interior_incidences.size() == 9U,
      "the k=10 boundary splits exactly into two shell and nine interior deletions");
  check(
      result.gamma_transition->strict_gamma.components.size() == 2U &&
          result.gamma_transition->equal_level_facets.size() == 9U &&
          result.gamma_transition->equal_level_cofaces.size() == 1U &&
          result.gamma_transition->transition_groups.size() == 1U &&
          result.gamma_transition->transition_groups[0]
                  .strict_component_indices ==
              std::vector<std::size_t>({0U, 1U}) &&
          result.gamma_transition->transition_groups[0].kind ==
              ExactGammaTransitionGroupKind::
                  multiple_strict_component_coalescence &&
          result.counters.deletion_incidence_projection_count == 11U &&
          result.counters.arm_incidence_projection_count == 2U &&
          result.counters.interior_incidence_projection_count == 9U &&
          result.group_overlays[0].canonical_event_indices ==
              std::vector<std::size_t>({0U}) &&
          result.group_overlays[0]
              .equal_level_cofaces_without_supplied_event_provenance
              .empty() &&
          all_certificates_close(verification),
      "the maximum supported event reconciles all k+1 incidences in one exact q=2 group");
}

void test_two_distinct_groups_receive_canonical_event_provenance() {
  const std::array<CertifiedPoint3, 4> input{
      point(0.0), point(2.0), point(10.0), point(12.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::vector<ExactCriticalEventGammaOverlayRequest> requests{
      request({3U, 2U}, 0U), request({1U, 0U}, 0U)};
  const ExactCriticalEventGammaOverlayResult result =
      build_exact_supplied_critical_event_gamma_overlay(
          cloud,
          requests,
          ExactCriticalEventGammaOverlayBudget{2U, 4U},
          ExactStrictGammaBudget{4U, 6U, 6U});

  check(
      result.gamma_transition->transition_groups.size() == 2U &&
          result.event_projections.size() == 2U &&
          result.event_projections[0].transition_group_index == 0U &&
          result.event_projections[1].transition_group_index == 1U &&
          result.group_overlays.size() == 2U &&
          result.group_overlays[0].canonical_event_indices ==
              std::vector<std::size_t>({0U}) &&
          result.group_overlays[1].canonical_event_indices ==
              std::vector<std::size_t>({1U}) &&
          result.group_overlays[0]
              .equal_level_cofaces_without_supplied_event_provenance
              .empty() &&
          result.group_overlays[1]
              .equal_level_cofaces_without_supplied_event_provenance
              .empty() &&
          result.counters.group_supplied_event_reference_count == 2U &&
          result.counters.group_with_supplied_event_count == 2U &&
          result.counters.group_without_supplied_event_count == 0U,
      "canonical supplied events project exactly once into two distinct exhaustive groups");
}

void test_groups_without_supplied_provenance_remain_explicit() {
  const std::array<CertifiedPoint3, 6> input{
      point(-6.0, 0.0),
      point(0.0, -9.0),
      point(0.0, 9.0),
      point(6.0, 0.0),
      point(30.0, 0.0),
      point(43.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::vector<ExactCriticalEventGammaOverlayRequest> requests{
      request({0U, 1U, 3U}, 0U)};
  const ExactCriticalEventGammaOverlayResult result =
      build_exact_supplied_critical_event_gamma_overlay(
          cloud,
          requests,
          ExactCriticalEventGammaOverlayBudget{1U, 3U},
          ExactStrictGammaBudget{15U, 20U, 40U});

  check(
      result.gamma_transition->transition_groups.size() == 2U &&
          result.event_projections.size() == 1U &&
          result.event_projections[0].transition_group_index == 0U &&
          result.group_overlays.size() == 2U &&
          result.group_overlays[0].canonical_event_indices ==
              std::vector<std::size_t>({0U}) &&
          result.group_overlays[0].has_supplied_event_provenance &&
          result.group_overlays[0]
                  .equal_level_cofaces_without_supplied_event_provenance ==
              std::vector<std::vector<PointId>>({{0U, 2U, 3U}}) &&
          result.group_overlays[1].canonical_event_indices.empty() &&
          result.group_overlays[1]
              .equal_level_cofaces_without_supplied_event_provenance
              .empty() &&
          !result.group_overlays[1].has_supplied_event_provenance &&
          result.counters.group_with_supplied_event_count == 1U &&
          result.counters.group_without_supplied_event_count == 1U,
      "an exhaustive Gamma group without supplied provenance is retained and never called noncritical");
}

void test_fail_closed_decisions_and_invalid_inputs() {
  const CanonicalPointCloud cloud = mirror_cloud();
  const std::vector<ExactCriticalEventGammaOverlayRequest> requests =
      mirror_requests_reversed();
  const ExactCriticalEventGammaOverlayResult insufficient_overlay =
      build_exact_supplied_critical_event_gamma_overlay(
          cloud,
          requests,
          ExactCriticalEventGammaOverlayBudget{1U, 6U},
          ExactStrictGammaBudget{6U, 4U, 8U});
  check(
      insufficient_overlay.decision ==
              ExactCriticalEventGammaOverlayDecision::
                  no_overlay_preflight_budget_insufficient &&
          insufficient_overlay.required_event_count == 2U &&
          insufficient_overlay.required_total_arm_count == 6U &&
          insufficient_overlay.event_classifications.empty() &&
          !insufficient_overlay.gamma_transition.has_value() &&
          insufficient_overlay.event_projections.empty(),
      "the event and arm preflight is atomic before all geometry");

  const ExactCriticalEventGammaOverlayResult insufficient_arm_budget =
      build_exact_supplied_critical_event_gamma_overlay(
          cloud,
          requests,
          ExactCriticalEventGammaOverlayBudget{2U, 5U},
          ExactStrictGammaBudget{6U, 4U, 8U});
  check(
      insufficient_arm_budget.decision ==
              ExactCriticalEventGammaOverlayDecision::
                  no_overlay_preflight_budget_insufficient &&
          insufficient_arm_budget.event_classifications.empty() &&
          insufficient_arm_budget.counters.preflight_count == 1U,
      "the total-arm preflight fails atomically even when the event count fits");

  const std::vector<ExactCriticalEventGammaOverlayRequest> one_event{
      request({0U, 1U, 3U}, 0U)};
  const ExactCriticalEventGammaOverlayResult insufficient_gamma =
      build_exact_supplied_critical_event_gamma_overlay(
          cloud,
          one_event,
          ExactCriticalEventGammaOverlayBudget{1U, 3U},
          ExactStrictGammaBudget{5U, 4U, 8U});
  check(
      insufficient_gamma.decision ==
              ExactCriticalEventGammaOverlayDecision::
                  no_overlay_gamma_preflight_budget_insufficient &&
          insufficient_gamma.event_classifications.size() == 1U &&
          !insufficient_gamma.gamma_transition.has_value() &&
          insufficient_gamma.event_projections.empty(),
      "a shared Gamma preflight failure publishes no transition or overlay");

  const std::array<CertifiedPoint3, 4> square_input{
      point(-1.0, -1.0),
      point(-1.0, 1.0),
      point(1.0, -1.0),
      point(1.0, 1.0)};
  const CanonicalPointCloud square_cloud = canonical_cloud(square_input);
  const std::vector<ExactCriticalEventGammaOverlayRequest> square_request{
      request({0U, 1U, 2U, 3U}, 0U)};
  const ExactCriticalEventGammaOverlayResult incomplete =
      build_exact_supplied_critical_event_gamma_overlay(
          square_cloud,
          square_request,
          ExactCriticalEventGammaOverlayBudget{1U, 4U},
          ExactStrictGammaBudget{6U, 4U, 12U});
  check(
      incomplete.decision ==
              ExactCriticalEventGammaOverlayDecision::
                  no_overlay_event_family_not_complete &&
          incomplete.counters.incomplete_arm_family_event_count == 1U &&
          !incomplete.gamma_transition.has_value(),
      "a nonminimal supplied critical shell remains an incomplete event family");

  const std::array<CertifiedPoint3, 3> mixed_input{
      point(0.0), point(1.0), point(3.0)};
  const CanonicalPointCloud mixed_cloud = canonical_cloud(mixed_input);
  const std::vector<ExactCriticalEventGammaOverlayRequest> mixed_requests{
      request({0U, 1U}, 0U), request({1U, 2U}, 0U)};
  const ExactCriticalEventGammaOverlayResult mixed =
      build_exact_supplied_critical_event_gamma_overlay(
          mixed_cloud,
          mixed_requests,
          ExactCriticalEventGammaOverlayBudget{2U, 4U},
          ExactStrictGammaBudget{3U, 3U, 3U});
  check(
          mixed.decision ==
              ExactCriticalEventGammaOverlayDecision::
                  no_overlay_mixed_order_or_exact_level &&
          mixed.common_order == 0U &&
          mixed.common_squared_level == ExactLevel{} &&
          !mixed.common_order_and_exact_level_derived &&
          !mixed.gamma_transition.has_value(),
      "events at different exact levels cannot share one frozen transition");

  const std::vector<ExactCriticalEventGammaOverlayRequest>
      mixed_order_requests{
          request({0U, 3U}, 0U), request({0U, 1U, 3U}, 0U)};
  const ExactCriticalEventGammaOverlayResult mixed_order =
      build_exact_supplied_critical_event_gamma_overlay(
          cloud,
          mixed_order_requests,
          ExactCriticalEventGammaOverlayBudget{2U, 5U},
          ExactStrictGammaBudget{6U, 6U, 8U});
  check(
      mixed_order.decision ==
              ExactCriticalEventGammaOverlayDecision::
                  no_overlay_mixed_order_or_exact_level &&
          mixed_order.common_order == 0U &&
          mixed_order.common_squared_level == ExactLevel{} &&
          !mixed_order.common_order_and_exact_level_derived,
      "events of different orders cannot publish a spurious common pair");

  const std::vector<ExactCriticalEventGammaOverlayRequest> duplicates{
      request({0U, 1U, 3U}, 0U), request({3U, 0U, 1U}, 0U)};
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(build_exact_supplied_critical_event_gamma_overlay(
            cloud,
            duplicates,
            ExactCriticalEventGammaOverlayBudget{2U, 6U},
            ExactStrictGammaBudget{6U, 4U, 8U}));
      },
      "duplicate supplied shells are rejected before replay");
  const std::vector<ExactCriticalEventGammaOverlayRequest> no_requests;
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(build_exact_supplied_critical_event_gamma_overlay(
            cloud,
            no_requests,
            ExactCriticalEventGammaOverlayBudget{1U, 4U},
            ExactStrictGammaBudget{6U, 4U, 8U}));
      },
      "an overlay cannot derive an exact pair from an empty supplied list");
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(build_exact_supplied_critical_event_gamma_overlay(
            cloud,
            one_event,
            ExactCriticalEventGammaOverlayBudget{9U, 3U},
            ExactStrictGammaBudget{6U, 4U, 8U}));
      },
      "an overlay budget above its reference cap is rejected");
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(build_exact_supplied_critical_event_gamma_overlay(
            cloud,
            one_event,
            ExactCriticalEventGammaOverlayBudget{8U, 33U},
            ExactStrictGammaBudget{6U, 4U, 8U}));
      },
      "a total-arm budget above its reference cap is rejected");
}

void test_verifier_rejects_every_overlay_layer() {
  const std::array<CertifiedPoint3, 3> input{
      point(-1.0, 0.0),
      point(0.0, 1.5),
      point(1.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::vector<ExactCriticalEventGammaOverlayRequest> requests{
      request({2U, 0U}, 0U)};
  const ExactCriticalEventGammaOverlayBudget overlay_budget{1U, 2U};
  const ExactStrictGammaBudget gamma_budget{3U, 3U, 3U};
  const ExactCriticalEventGammaOverlayResult result =
      build_exact_supplied_critical_event_gamma_overlay(
          cloud, requests, overlay_budget, gamma_budget);

  const auto rejects =
      [&cloud, &requests, overlay_budget, gamma_budget](
          const ExactCriticalEventGammaOverlayResult& candidate,
          const std::string& message) {
        const ExactCriticalEventGammaOverlayVerification verification =
            verify_exact_supplied_critical_event_gamma_overlay(
                cloud,
                requests,
                overlay_budget,
                gamma_budget,
                candidate);
        check(
            !verification.
                 exact_critical_event_gamma_overlay_decision_certified,
            message);
      };

  ExactCriticalEventGammaOverlayResult bad_overlay_budget = result;
  --bad_overlay_budget.requested_overlay_budget.maximum_total_arm_count;
  rejects(bad_overlay_budget, "the stored overlay budget is untrusted");

  ExactCriticalEventGammaOverlayResult bad_gamma_budget = result;
  --bad_gamma_budget.requested_gamma_budget.maximum_union_attempt_count;
  rejects(bad_gamma_budget, "the stored Gamma budget is untrusted");

  ExactCriticalEventGammaOverlayResult bad_requests = result;
  bad_requests.canonical_event_requests[0].critical_shell_point_ids =
      {0U, 1U};
  rejects(bad_requests, "the canonical supplied requests are replayed");

  ExactCriticalEventGammaOverlayResult bad_preflight = result;
  ++bad_preflight.required_total_arm_count;
  rejects(bad_preflight, "the event and arm counts are replayed");

  ExactCriticalEventGammaOverlayResult bad_event = result;
  --bad_event.event_classifications[0].order;
  rejects(bad_event, "each embedded 6.9 event is freshly verified");

  ExactCriticalEventGammaOverlayResult bad_pair = result;
  ++bad_pair.common_order;
  rejects(bad_pair, "the common exact pair is derived from the events");

  ExactCriticalEventGammaOverlayResult bad_transition = result;
  bad_transition.gamma_transition->equal_level_cofaces.clear();
  rejects(bad_transition, "the exhaustive 6.10 transition is freshly verified");

  ExactCriticalEventGammaOverlayResult bad_projection = result;
  ++bad_projection.event_projections[0].canonical_event_index;
  rejects(bad_projection, "the event deletion projection is replayed");

  ExactCriticalEventGammaOverlayResult bad_group = result;
  bad_group.group_overlays[0].canonical_event_indices.clear();
  rejects(bad_group, "every transition group overlay is replayed");

  ExactCriticalEventGammaOverlayResult bad_residual = result;
  bad_residual.group_overlays[0]
      .equal_level_cofaces_without_supplied_event_provenance
      .push_back({0U, 1U});
  rejects(
      bad_residual,
      "unattributed equality-coface provenance is replayed structurally");

  ExactCriticalEventGammaOverlayResult bad_fact = result;
  bad_fact.every_event_deletion_incidence_reconciled = false;
  rejects(bad_fact, "the reconciliation facts are untrusted");

  ExactCriticalEventGammaOverlayResult bad_counter = result;
  ++bad_counter.counters.arm_incidence_projection_count;
  rejects(bad_counter, "the overlay work counters are replayed");

  ExactCriticalEventGammaOverlayResult bad_decision = result;
  bad_decision.decision = ExactCriticalEventGammaOverlayDecision::
      no_overlay_event_family_not_complete;
  rejects(bad_decision, "the overlay decision is untrusted");

  ExactCriticalEventGammaOverlayResult bad_scope = result;
  bad_scope.scope = ExactCriticalEventGammaOverlayScope::unspecified;
  rejects(bad_scope, "the supplied-event-only scope is untrusted");

  const ExactCriticalEventGammaOverlayVerification wrong_budget =
      verify_exact_supplied_critical_event_gamma_overlay(
          cloud,
          requests,
          ExactCriticalEventGammaOverlayBudget{1U, 1U},
          gamma_budget,
          result);
  check(
      !wrong_budget.requested_overlay_budget_certified &&
          !wrong_budget.
              exact_critical_event_gamma_overlay_decision_certified,
      "the verifier never accepts another trusted overlay budget");

  const std::vector<ExactCriticalEventGammaOverlayRequest> other_requests{
      request({0U, 1U}, 0U)};
  const ExactCriticalEventGammaOverlayVerification wrong_requests =
      verify_exact_supplied_critical_event_gamma_overlay(
          cloud,
          other_requests,
          overlay_budget,
          gamma_budget,
          result);
  check(
      !wrong_requests.canonical_event_requests_certified &&
          !wrong_requests.
              exact_critical_event_gamma_overlay_decision_certified,
      "the verifier never accepts another trusted event list");
}

}  // namespace

int main() {
  test_two_supplied_events_form_one_simultaneous_q5_group();
  test_q1_event_and_interior_deletion_are_reconciled();
  test_maximum_order_event_reconciles_shell_and_interior_deletions();
  test_two_distinct_groups_receive_canonical_event_provenance();
  test_groups_without_supplied_provenance_remain_explicit();
  test_fail_closed_decisions_and_invalid_inputs();
  test_verifier_rejects_every_overlay_layer();

  if (failures != 0) {
    std::cerr << failures << " test assertion(s) failed\n";
    return 1;
  }
  std::cout << "all supplied critical-event Gamma overlay tests passed\n";
  return 0;
}
