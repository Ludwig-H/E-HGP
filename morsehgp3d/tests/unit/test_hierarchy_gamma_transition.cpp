#include "morsehgp3d/hierarchy/gamma_transition.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactGammaTransitionDecision;
using morsehgp3d::hierarchy::ExactGammaTransitionGroupKind;
using morsehgp3d::hierarchy::ExactGammaTransitionResult;
using morsehgp3d::hierarchy::ExactGammaTransitionScope;
using morsehgp3d::hierarchy::ExactGammaTransitionVerification;
using morsehgp3d::hierarchy::ExactStrictGammaBudget;
using morsehgp3d::hierarchy::ExactStrictGammaDecision;
using morsehgp3d::hierarchy::build_exact_gamma_equal_level_transition;
using morsehgp3d::hierarchy::verify_exact_gamma_equal_level_transition;
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

[[nodiscard]] bool all_certificates_close(
    const ExactGammaTransitionVerification& verification) {
  return verification.requested_budget_certified &&
         verification.external_inputs_certified &&
         verification.preflight_counts_certified &&
         verification.strict_gamma_certified &&
         verification.equal_level_facets_certified &&
         verification.equal_level_cofaces_certified &&
         verification.equal_level_incidences_certified &&
         verification.closed_components_certified &&
         verification.strict_component_projection_certified &&
         verification.transition_groups_certified &&
         verification.result_facts_certified &&
         verification.counters_certified &&
         verification.decision_certified &&
         verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.exact_gamma_transition_decision_certified;
}

[[nodiscard]] CanonicalPointCloud q2_fixture_cloud() {
  const std::array<CertifiedPoint3, 3> input{
      point(-2.0, 0.0),
      point(0.0, 1.0),
      point(2.0, 0.0)};
  return canonical_cloud(input);
}

[[nodiscard]] ExactGammaTransitionResult q2_fixture_result(
    const CanonicalPointCloud& cloud) {
  return build_exact_gamma_equal_level_transition(
      cloud,
      2U,
      level(4),
      ExactStrictGammaBudget{3U, 1U, 2U});
}

void test_default_and_new_equal_facet_component() {
  const ExactGammaTransitionResult empty;
  check(
      empty.decision == ExactGammaTransitionDecision::not_certified &&
          empty.scope == ExactGammaTransitionScope::unspecified &&
          empty.equal_level_facets.empty() &&
          empty.equal_level_cofaces.empty() &&
          empty.equal_level_incidences.empty() &&
          empty.closed_components.empty() &&
          empty.transition_groups.empty(),
      "the default transition certifies no strict-to-closed cut");

  const std::array<CertifiedPoint3, 3> input{
      point(0.0), point(2.0), point(10.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactStrictGammaBudget budget{3U, 1U, 2U};
  const ExactGammaTransitionResult result =
      build_exact_gamma_equal_level_transition(
          cloud, 2U, level(1), budget);
  const ExactGammaTransitionVerification verification =
      verify_exact_gamma_equal_level_transition(
          cloud, 2U, level(1), budget, result);

  check(
      result.strict_gamma.active_facets.empty() &&
          result.strict_gamma.components.empty() &&
          result.equal_level_facets.size() == 1U &&
          result.equal_level_facets[0].facet_point_ids ==
              std::vector<PointId>({0U, 1U}) &&
          result.equal_level_facets[0].squared_level == level(1) &&
          result.equal_level_cofaces.empty() &&
          result.equal_level_incidences.empty() &&
          result.closed_components.size() == 1U &&
          result.closed_components[0].facet_point_ids ==
              std::vector<std::vector<PointId>>({{0U, 1U}}),
      "an isolated facet born at equality is absent strictly and present closed");

  check(
      result.transition_groups.size() == 1U &&
          result.transition_groups[0].strict_component_indices.empty() &&
          result.transition_groups[0].newly_active_facet_point_ids ==
              std::vector<std::vector<PointId>>({{0U, 1U}}) &&
          result.transition_groups[0].equal_level_coface_point_ids.empty() &&
          result.transition_groups[0].kind ==
              ExactGammaTransitionGroupKind::
                  new_closed_component_without_strict_component &&
          result.counters.new_component_group_count == 1U &&
          result.counters.continuation_group_count == 0U &&
          result.counters.coalescence_group_count == 0U &&
          all_certificates_close(verification),
      "q=0 remains a closed-cut transition group rather than a public birth");
}

void test_redundant_equal_coface_continues_one_strict_component() {
  const std::array<CertifiedPoint3, 3> input{
      point(0.0), point(1.0), point(2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactStrictGammaBudget budget{3U, 3U, 3U};
  const ExactGammaTransitionResult result =
      build_exact_gamma_equal_level_transition(
          cloud, 1U, level(1), budget);

  check(
      result.strict_gamma.active_facets.size() == 3U &&
          result.strict_gamma.active_cofaces.size() == 2U &&
          result.strict_gamma.components.size() == 1U &&
          result.equal_level_facets.empty() &&
          result.equal_level_cofaces.size() == 1U &&
          result.equal_level_cofaces[0].coface_point_ids ==
              std::vector<PointId>({0U, 2U}) &&
          result.closed_components.size() == 1U &&
          result.strict_component_to_closed_component_index ==
              std::vector<std::size_t>({0U}),
      "the strict path already connects both facets of the equal redundant edge");

  check(
      result.equal_level_incidences.size() == 2U &&
          result.equal_level_incidences[0].strict_component_index == 0U &&
          !result.equal_level_incidences[0].newly_active_at_level &&
          result.equal_level_incidences[1].strict_component_index == 0U &&
          !result.equal_level_incidences[1].newly_active_at_level &&
          result.transition_groups.size() == 1U &&
          result.transition_groups[0].strict_component_indices ==
              std::vector<std::size_t>({0U}) &&
          result.transition_groups[0].kind ==
              ExactGammaTransitionGroupKind::
                  one_strict_component_continuation &&
          result.counters.continuation_group_count == 1U &&
          result.counters.closed_union_attempt_count == 3U &&
          result.counters.closed_union_merge_count == 2U,
      "q=1 preserves the silent equal incidence even when it merges no DSU roots");
}

void test_new_facet_and_equal_coface_coalesce_two_strict_components() {
  const CanonicalPointCloud cloud = q2_fixture_cloud();
  const ExactStrictGammaBudget budget{3U, 1U, 2U};
  const ExactGammaTransitionResult result = q2_fixture_result(cloud);
  const ExactGammaTransitionVerification verification =
      verify_exact_gamma_equal_level_transition(
          cloud, 2U, level(4), budget, result);

  check(
      result.strict_gamma.active_facets.size() == 2U &&
          result.strict_gamma.active_cofaces.empty() &&
          result.strict_gamma.components.size() == 2U &&
          result.equal_level_facets.size() == 1U &&
          result.equal_level_facets[0].facet_point_ids ==
              std::vector<PointId>({0U, 2U}) &&
          result.equal_level_cofaces.size() == 1U &&
          result.equal_level_cofaces[0].coface_point_ids ==
              std::vector<PointId>({0U, 1U, 2U}) &&
          result.closed_components.size() == 1U &&
          result.closed_components[0].facet_point_ids ==
              std::vector<std::vector<PointId>>(
                  {{0U, 1U}, {0U, 2U}, {1U, 2U}}) &&
          result.strict_component_to_closed_component_index ==
              std::vector<std::size_t>({0U, 0U}),
      "the closed cut includes both frozen roots and the equality facet token");

  check(
      result.equal_level_incidences.size() == 3U &&
          result.equal_level_incidences[0].facet_point_ids ==
              std::vector<PointId>({0U, 1U}) &&
          result.equal_level_incidences[0].strict_component_index == 0U &&
          !result.equal_level_incidences[0].newly_active_at_level &&
          result.equal_level_incidences[1].facet_point_ids ==
              std::vector<PointId>({0U, 2U}) &&
          !result.equal_level_incidences[1]
               .strict_component_index.has_value() &&
          result.equal_level_incidences[1].newly_active_at_level &&
          result.equal_level_incidences[2].facet_point_ids ==
              std::vector<PointId>({1U, 2U}) &&
          result.equal_level_incidences[2].strict_component_index == 1U &&
          !result.equal_level_incidences[2].newly_active_at_level,
      "every equal coface facet is tokenized as exactly one frozen root or new facet");

  check(
      result.transition_groups.size() == 1U &&
          result.transition_groups[0].strict_component_indices ==
              std::vector<std::size_t>({0U, 1U}) &&
          result.transition_groups[0].newly_active_facet_point_ids ==
              std::vector<std::vector<PointId>>({{0U, 2U}}) &&
          result.transition_groups[0].equal_level_coface_point_ids ==
              std::vector<std::vector<PointId>>({{0U, 1U, 2U}}) &&
          result.transition_groups[0].kind ==
              ExactGammaTransitionGroupKind::
                  multiple_strict_component_coalescence &&
          result.counters.closed_union_attempt_count == 2U &&
          result.counters.closed_union_merge_count == 2U &&
          result.counters.coalescence_group_count == 1U &&
          result.equal_level_incidences_tokenized &&
          result.equal_level_batch_applied_simultaneously &&
          all_certificates_close(verification),
      "q=2 is one exhaustive cut coalescence group without becoming a public merge");
}

void test_overlapping_equal_cofaces_form_one_simultaneous_group() {
  const std::array<CertifiedPoint3, 4> input{
      point(-2.0, 0.0),
      point(0.0, -3.0),
      point(0.0, 3.0),
      point(2.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactGammaTransitionResult result =
      build_exact_gamma_equal_level_transition(
          cloud,
          2U,
          level(169, 36),
          ExactStrictGammaBudget{6U, 4U, 8U});

  check(
      result.strict_gamma.active_facets.size() == 5U &&
          result.strict_gamma.active_cofaces.empty() &&
          result.strict_gamma.components.size() == 5U &&
          result.equal_level_facets.empty() &&
          result.equal_level_cofaces.size() == 2U &&
          result.equal_level_cofaces[0].coface_point_ids ==
              std::vector<PointId>({0U, 1U, 3U}) &&
          result.equal_level_cofaces[1].coface_point_ids ==
              std::vector<PointId>({0U, 2U, 3U}) &&
          result.closed_components.size() == 1U &&
          result.closed_components[0].facet_point_ids ==
              std::vector<std::vector<PointId>>(
                  {{0U, 1U},
                   {0U, 2U},
                   {0U, 3U},
                   {1U, 3U},
                   {2U, 3U}}),
      "two equal critical cofaces overlap through the frozen base facet");

  check(
      result.transition_groups.size() == 1U &&
          result.transition_groups[0].strict_component_indices ==
              std::vector<std::size_t>({0U, 1U, 2U, 3U, 4U}) &&
          result.transition_groups[0].equal_level_coface_point_ids ==
              std::vector<std::vector<PointId>>(
                  {{0U, 1U, 3U}, {0U, 2U, 3U}}) &&
          result.transition_groups[0].kind ==
              ExactGammaTransitionGroupKind::
                  multiple_strict_component_coalescence &&
          result.counters.closed_union_attempt_count == 4U &&
          result.counters.closed_union_merge_count == 4U &&
          result.counters.transition_group_count == 1U,
      "overlapping equality hyperedges are contracted as one q=5 group, never sequential q=3 groups");
}

void test_disconnected_batches_keep_q0_q1_q2_separate() {
  const std::array<CertifiedPoint3, 10> input{
      point(-2.0, -1.0, 0.0),
      point(-2.0, 1.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(3.0, 2.0, 0.0),
      point(4.0, -1.0, 0.0),
      point(20.0, 0.0, 0.0),
      point(23.0, 3.0, 4.0),
      point(40.0, 0.0, 0.0),
      point(41.5, 1.5, 2.0),
      point(43.0, 3.0, 4.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactGammaTransitionResult result =
      build_exact_gamma_equal_level_transition(
          cloud,
          2U,
          level(17, 2),
          ExactStrictGammaBudget{45U, 120U, 240U});

  check(
      result.strict_gamma.active_facets.size() == 9U &&
          result.strict_gamma.active_cofaces.size() == 3U &&
          result.strict_gamma.components.size() == 3U &&
          result.equal_level_facets.size() == 3U &&
          result.equal_level_cofaces.size() == 3U &&
          result.equal_level_incidences.size() == 9U &&
          result.closed_components.size() == 3U &&
          result.strict_component_to_closed_component_index ==
              std::vector<std::size_t>({0U, 2U, 2U}),
      "one exact level can contain three disconnected transition batches");

  check(
      result.transition_groups.size() == 3U &&
          result.transition_groups[0].strict_component_indices ==
              std::vector<std::size_t>({0U}) &&
          result.transition_groups[0].newly_active_facet_point_ids ==
              std::vector<std::vector<PointId>>({{0U, 3U}}) &&
          result.transition_groups[0].equal_level_coface_point_ids ==
              std::vector<std::vector<PointId>>(
                  {{0U, 1U, 3U}, {0U, 2U, 3U}}) &&
          result.transition_groups[0].kind ==
              ExactGammaTransitionGroupKind::
                  one_strict_component_continuation,
      "the first disconnected batch is one q=1 continuation");
  check(
      result.transition_groups[1].strict_component_indices.empty() &&
          result.transition_groups[1].newly_active_facet_point_ids ==
              std::vector<std::vector<PointId>>({{5U, 6U}}) &&
          result.transition_groups[1].equal_level_coface_point_ids.empty() &&
          result.transition_groups[1].kind ==
              ExactGammaTransitionGroupKind::
                  new_closed_component_without_strict_component,
      "the second disconnected batch is one isolated q=0 component");
  check(
      result.transition_groups[2].strict_component_indices ==
              std::vector<std::size_t>({1U, 2U}) &&
          result.transition_groups[2].newly_active_facet_point_ids ==
              std::vector<std::vector<PointId>>({{7U, 9U}}) &&
          result.transition_groups[2].equal_level_coface_point_ids ==
              std::vector<std::vector<PointId>>({{7U, 8U, 9U}}) &&
          result.transition_groups[2].kind ==
              ExactGammaTransitionGroupKind::
                  multiple_strict_component_coalescence &&
          result.counters.new_component_group_count == 1U &&
          result.counters.continuation_group_count == 1U &&
          result.counters.coalescence_group_count == 1U &&
          result.counters.closed_union_attempt_count == 12U &&
          result.counters.closed_union_merge_count == 9U,
      "the third disconnected batch is one q=2 coalescence");
}

void test_level_without_equality_change_emits_no_group() {
  const std::array<CertifiedPoint3, 3> input{
      point(0.0), point(1.0), point(2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactGammaTransitionResult result =
      build_exact_gamma_equal_level_transition(
          cloud,
          1U,
          level(1, 2),
          ExactStrictGammaBudget{3U, 3U, 3U});

  check(
      result.strict_gamma.components.size() == 1U &&
          result.equal_level_facets.empty() &&
          result.equal_level_cofaces.empty() &&
          result.equal_level_incidences.empty() &&
          result.closed_components == result.strict_gamma.components &&
          result.strict_component_to_closed_component_index ==
              std::vector<std::size_t>({0U}) &&
          result.transition_groups.empty() &&
          result.counters.transition_group_count == 0U,
      "a noncritical level preserves the cut without inventing a transition group");
}

void test_order_ten_equal_coface_uses_deletion_witness() {
  const std::array<CertifiedPoint3, 11> input{
      point(0.0),
      point(1.0),
      point(2.0),
      point(3.0),
      point(4.0),
      point(5.0),
      point(6.0),
      point(7.0),
      point(8.0),
      point(9.0),
      point(10.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactStrictGammaBudget budget{11U, 1U, 10U};
  const ExactGammaTransitionResult result =
      build_exact_gamma_equal_level_transition(
          cloud, 10U, level(25), budget);
  const ExactGammaTransitionVerification verification =
      verify_exact_gamma_equal_level_transition(
          cloud, 10U, level(25), budget, result);

  check(
      result.strict_gamma.active_facets.size() == 2U &&
          result.strict_gamma.active_cofaces.empty() &&
          result.strict_gamma.components.size() == 2U &&
          result.equal_level_facets.size() == 9U &&
          result.equal_level_cofaces.size() == 1U &&
          result.equal_level_cofaces[0].eleven_point_witness.has_value() &&
          result.closed_components.size() == 1U &&
          result.strict_component_to_closed_component_index ==
              std::vector<std::size_t>({0U, 0U}),
      "the maximal supported order separates strict facets from the equality batch");
  if (result.equal_level_cofaces[0].eleven_point_witness.has_value()) {
    const auto& witness =
        *result.equal_level_cofaces[0].eleven_point_witness;
    check(
        witness.selected_deletion_facet_point_ids ==
                std::vector<PointId>(
                    {0U, 1U, 2U, 3U, 4U, 5U,
                     6U, 7U, 8U, 10U}) &&
            witness.omitted_point_id == 9U &&
            witness.squared_level == level(25) &&
            witness.omitted_point_squared_distance == level(16) &&
            witness.selected_deletion_attains_maximum &&
            witness.selected_ball_covers_omitted_point,
        "the equality coface retains the exact eleven-point deletion witness");
  }
  check(
      result.equal_level_incidences.size() == 11U &&
          result.transition_groups.size() == 1U &&
          result.transition_groups[0].strict_component_indices ==
              std::vector<std::size_t>({0U, 1U}) &&
          result.transition_groups[0].newly_active_facet_point_ids.size() ==
              9U &&
          result.transition_groups[0].equal_level_coface_point_ids ==
              std::vector<std::vector<PointId>>(
                  {{0U, 1U, 2U, 3U, 4U, 5U,
                    6U, 7U, 8U, 9U, 10U}}) &&
          result.transition_groups[0].kind ==
              ExactGammaTransitionGroupKind::
                  multiple_strict_component_coalescence &&
          result.counters.eleven_point_coface_count == 1U &&
          result.counters.eleven_point_deletion_level_lookup_count == 11U &&
          result.counters.
                  eleven_point_level_maximum_comparison_count == 10U &&
          result.counters.
                  eleven_point_omitted_point_distance_evaluation_count == 1U &&
          result.counters.closed_union_attempt_count == 10U &&
          all_certificates_close(verification),
      "the order-ten equality hyperedge is one simultaneous q=2 group");

  ExactGammaTransitionResult bad_witness = result;
  bad_witness.equal_level_cofaces[0]
      .eleven_point_witness->omitted_point_squared_distance = level(17);
  const ExactGammaTransitionVerification bad_verification =
      verify_exact_gamma_equal_level_transition(
          cloud, 10U, level(25), budget, bad_witness);
  check(
      !bad_verification.equal_level_cofaces_certified &&
          !bad_verification.fresh_replay_certified &&
          !bad_verification.exact_gamma_transition_decision_certified,
      "the transition verifier rejects a falsified equality deletion witness");
}

void test_preflight_bounds_cover_the_full_domain() {
  const std::array<CertifiedPoint3, 14> input{
      point(0.0),
      point(1.0),
      point(2.0),
      point(3.0),
      point(4.0),
      point(5.0),
      point(6.0),
      point(7.0),
      point(8.0),
      point(9.0),
      point(10.0),
      point(11.0),
      point(12.0),
      point(13.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactStrictGammaBudget zero_budget{};
  const ExactGammaTransitionResult order_six =
      build_exact_gamma_equal_level_transition(
          cloud, 6U, level(1), zero_budget);
  const ExactGammaTransitionResult order_seven =
      build_exact_gamma_equal_level_transition(
          cloud, 7U, level(1), zero_budget);
  check(
      order_six.required_facet_count == 3003U &&
          order_six.required_coface_count == 3432U &&
          order_six.required_union_attempt_count == 20592U &&
          order_six.counters.enumerated_facet_count == 0U &&
          order_seven.required_facet_count == 3432U &&
          order_seven.required_coface_count == 3003U &&
          order_seven.required_union_attempt_count == 21021U &&
          order_seven.counters.enumerated_facet_count == 0U,
      "the atomic preflight reaches both combinatorial caps without geometry");

  const std::array<CertifiedPoint3, 15> too_many_input{
      point(0.0),
      point(1.0),
      point(2.0),
      point(3.0),
      point(4.0),
      point(5.0),
      point(6.0),
      point(7.0),
      point(8.0),
      point(9.0),
      point(10.0),
      point(11.0),
      point(12.0),
      point(13.0),
      point(14.0)};
  const CanonicalPointCloud too_many_cloud =
      canonical_cloud(too_many_input);
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(build_exact_gamma_equal_level_transition(
            too_many_cloud, 7U, level(1), zero_budget));
      },
      "the transition rejects clouds above the certified cap");
}

void test_preflight_is_atomic_and_invalid_inputs_are_rejected() {
  const CanonicalPointCloud cloud = q2_fixture_cloud();
  const std::array<ExactStrictGammaBudget, 3> insufficient_budgets{
      ExactStrictGammaBudget{2U, 1U, 2U},
      ExactStrictGammaBudget{3U, 0U, 2U},
      ExactStrictGammaBudget{3U, 1U, 1U}};
  for (const ExactStrictGammaBudget& insufficient :
       insufficient_budgets) {
    const ExactGammaTransitionResult result =
        build_exact_gamma_equal_level_transition(
            cloud, 2U, level(4), insufficient);
    const ExactGammaTransitionVerification verification =
        verify_exact_gamma_equal_level_transition(
            cloud, 2U, level(4), insufficient, result);
    check(
        result.decision == ExactGammaTransitionDecision::
                               no_transition_preflight_budget_insufficient &&
            result.strict_gamma.decision ==
                ExactStrictGammaDecision::
                    no_cut_preflight_budget_insufficient &&
            result.required_facet_count == 3U &&
            result.required_coface_count == 1U &&
            result.required_union_attempt_count == 2U &&
            result.equal_level_facets.empty() &&
            result.equal_level_cofaces.empty() &&
            result.equal_level_incidences.empty() &&
            result.closed_components.empty() &&
            result.transition_groups.empty() &&
            result.counters.enumerated_facet_count == 0U &&
            result.counters.enumerated_coface_count == 0U &&
            result.counters.closed_union_attempt_count == 0U &&
            all_certificates_close(verification),
        "each insufficient preflight dimension publishes no partial equality transition");
  }

  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(build_exact_gamma_equal_level_transition(
            cloud,
            0U,
            level(4),
            ExactStrictGammaBudget{3U, 1U, 2U}));
      },
      "order zero is rejected");
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(build_exact_gamma_equal_level_transition(
            cloud,
            3U,
            level(4),
            ExactStrictGammaBudget{3U, 1U, 2U}));
      },
      "an order equal to n is rejected");

  ExactStrictGammaBudget oversized{
      ExactStrictGammaBudget::maximum_supported_facet_count,
      ExactStrictGammaBudget::maximum_supported_coface_count,
      ExactStrictGammaBudget::maximum_supported_union_attempt_count};
  ++oversized.maximum_union_attempt_count;
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(build_exact_gamma_equal_level_transition(
            cloud, 2U, level(4), oversized));
      },
      "a transition budget above the reference cap is rejected");
}

void test_verifier_rejects_every_transition_layer() {
  const CanonicalPointCloud cloud = q2_fixture_cloud();
  const ExactStrictGammaBudget budget{3U, 1U, 2U};
  const ExactGammaTransitionResult result = q2_fixture_result(cloud);

  const auto rejects =
      [&cloud, budget](
          const ExactGammaTransitionResult& candidate,
          const std::string& message) {
        const ExactGammaTransitionVerification verification =
            verify_exact_gamma_equal_level_transition(
                cloud, 2U, level(4), budget, candidate);
        check(
            !verification.exact_gamma_transition_decision_certified,
            message);
      };

  ExactGammaTransitionResult bad_budget = result;
  --bad_budget.requested_budget.maximum_union_attempt_count;
  rejects(bad_budget, "the requested budget is untrusted");

  ExactGammaTransitionResult bad_point_count = result;
  ++bad_point_count.point_count;
  rejects(bad_point_count, "the external point count is replayed");

  ExactGammaTransitionResult bad_order = result;
  --bad_order.order;
  rejects(bad_order, "the transition order is untrusted");

  ExactGammaTransitionResult bad_level = result;
  bad_level.squared_level = level(5);
  rejects(bad_level, "the equality level is untrusted");

  ExactGammaTransitionResult bad_preflight = result;
  ++bad_preflight.required_union_attempt_count;
  rejects(bad_preflight, "the preflight counts are replayed");

  ExactGammaTransitionResult bad_strict = result;
  bad_strict.strict_gamma.active_facets.front().squared_level = level(2);
  rejects(bad_strict, "the embedded strict cut is freshly verified");

  ExactGammaTransitionResult bad_equal_facet = result;
  bad_equal_facet.equal_level_facets.front().squared_level = level(5);
  rejects(bad_equal_facet, "the equal-level facet catalogue is replayed");

  ExactGammaTransitionResult bad_equal_coface = result;
  bad_equal_coface.equal_level_cofaces.front().squared_level = level(5);
  rejects(bad_equal_coface, "the equal-level coface catalogue is replayed");

  ExactGammaTransitionResult bad_incidence = result;
  bad_incidence.equal_level_incidences[1].newly_active_at_level = false;
  rejects(bad_incidence, "the frozen-root versus new-facet token is replayed");

  ExactGammaTransitionResult bad_closed = result;
  bad_closed.closed_components.front().facet_point_ids.pop_back();
  rejects(bad_closed, "the exhaustive closed partition is replayed");

  ExactGammaTransitionResult bad_projection = result;
  ++bad_projection.strict_component_to_closed_component_index.front();
  rejects(bad_projection, "the strict-to-closed component map is replayed");

  ExactGammaTransitionResult bad_group = result;
  bad_group.transition_groups.front().strict_component_indices.pop_back();
  rejects(bad_group, "the simultaneous transition groups are replayed");

  ExactGammaTransitionResult bad_fact = result;
  bad_fact.equal_level_batch_applied_simultaneously = false;
  rejects(bad_fact, "the transition facts are untrusted");

  ExactGammaTransitionResult bad_counter = result;
  ++bad_counter.counters.equal_level_incidence_count;
  rejects(bad_counter, "the exhaustive counters are replayed");

  ExactGammaTransitionResult bad_decision = result;
  bad_decision.decision = ExactGammaTransitionDecision::
      no_transition_preflight_budget_insufficient;
  rejects(bad_decision, "the transition decision is untrusted");

  ExactGammaTransitionResult bad_scope = result;
  bad_scope.scope = ExactGammaTransitionScope::unspecified;
  rejects(bad_scope, "the bounded transition scope is untrusted");

  const ExactGammaTransitionVerification wrong_budget_verification =
      verify_exact_gamma_equal_level_transition(
          cloud,
          2U,
          level(4),
          ExactStrictGammaBudget{3U, 1U, 1U},
          result);
  check(
      !wrong_budget_verification.requested_budget_certified &&
          !wrong_budget_verification.
              exact_gamma_transition_decision_certified,
      "the verifier never accepts another trusted budget");

  const std::array<CertifiedPoint3, 3> twin_input{
      point(-2.0, 0.0),
      point(0.0, 1.5),
      point(2.0, 0.0)};
  const CanonicalPointCloud twin_cloud = canonical_cloud(twin_input);
  const ExactGammaTransitionVerification wrong_cloud_verification =
      verify_exact_gamma_equal_level_transition(
          twin_cloud, 2U, level(4), budget, result);
  check(
      !wrong_cloud_verification.exact_gamma_transition_decision_certified,
      "the verifier freshly replays the trusted cloud");
}

}  // namespace

int main() {
  test_default_and_new_equal_facet_component();
  test_redundant_equal_coface_continues_one_strict_component();
  test_new_facet_and_equal_coface_coalesce_two_strict_components();
  test_overlapping_equal_cofaces_form_one_simultaneous_group();
  test_disconnected_batches_keep_q0_q1_q2_separate();
  test_level_without_equality_change_emits_no_group();
  test_order_ten_equal_coface_uses_deletion_witness();
  test_preflight_bounds_cover_the_full_domain();
  test_preflight_is_atomic_and_invalid_inputs_are_rejected();
  test_verifier_rejects_every_transition_layer();

  if (failures != 0) {
    std::cerr << failures << " test assertion(s) failed\n";
    return 1;
  }
  std::cout << "all exact Gamma equal-level transition tests passed\n";
  return 0;
}
