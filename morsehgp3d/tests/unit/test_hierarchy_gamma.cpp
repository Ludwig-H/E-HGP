#include "morsehgp3d/hierarchy/gamma.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <numeric>
#include <optional>
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
using morsehgp3d::hierarchy::ExactStrictGammaBudget;
using morsehgp3d::hierarchy::ExactStrictGammaDecision;
using morsehgp3d::hierarchy::ExactStrictGammaResult;
using morsehgp3d::hierarchy::ExactStrictGammaScope;
using morsehgp3d::hierarchy::ExactStrictGammaVerification;
using morsehgp3d::hierarchy::build_exact_strict_gamma_source_classification;
using morsehgp3d::hierarchy::verify_exact_strict_gamma_source_classification;
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

[[nodiscard]] ExactStrictGammaBudget full_budget() {
  return ExactStrictGammaBudget{
      ExactStrictGammaBudget::maximum_supported_facet_count,
      ExactStrictGammaBudget::maximum_supported_coface_count,
      ExactStrictGammaBudget::maximum_supported_union_attempt_count};
}

[[nodiscard]] bool all_certificates_close(
    const ExactStrictGammaVerification& verification) {
  return verification.requested_budget_certified &&
         verification.external_inputs_certified &&
         verification.preflight_counts_certified &&
         verification.active_facets_certified &&
         verification.active_cofaces_certified &&
         verification.components_certified &&
         verification.source_classifications_certified &&
         verification.result_facts_certified &&
         verification.counters_certified &&
         verification.decision_certified &&
         verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.exact_strict_gamma_decision_certified;
}

void test_strict_cut_excludes_equal_coface() {
  const std::array<CertifiedPoint3, 2> input{
      point(-2.0), point(2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::vector<std::vector<PointId>> sources{{0U}, {1U}};
  const ExactStrictGammaBudget budget = full_budget();
  const ExactStrictGammaResult result =
      build_exact_strict_gamma_source_classification(
          cloud, 1U, level(4), sources, budget);
  const ExactStrictGammaVerification verification =
      verify_exact_strict_gamma_source_classification(
          cloud, 1U, level(4), sources, budget, result);

  check(
      result.active_facets.size() == 2U &&
          result.active_cofaces.empty() &&
          result.components.size() == 2U &&
          result.components[0].facet_point_ids ==
              std::vector<std::vector<PointId>>{{0U}} &&
          result.components[1].facet_point_ids ==
              std::vector<std::vector<PointId>>{{1U}},
      "the open cut at four excludes the equal-level AC coface");
  check(
      result.source_classifications.size() == 2U &&
          result.source_classifications[0].component_index ==
              std::optional<std::size_t>{0U} &&
          result.source_classifications[1].component_index ==
              std::optional<std::size_t>{1U} &&
          result.decision == ExactStrictGammaDecision::
                                 complete_all_sources_active_and_classified,
      "the two singleton sources remain in two canonical components");
  check(
      result.strict_open_cut_certified &&
          result.full_pi0_isolated_facets_included &&
          result.scope == ExactStrictGammaScope::
                              bounded_exhaustive_strict_gamma_full_pi0_source_components_only &&
          std::string_view{ExactStrictGammaResult::proof_basis} ==
              "exact_bounded_exhaustive_strict_gamma_full_pi0_source_"
              "component_classification_v1" &&
          all_certificates_close(verification),
      "the two-point result closes only its strict source-component scope");
}

void test_two_strict_cofaces_connect_three_singletons() {
  const std::array<CertifiedPoint3, 3> input{
      point(-2.0, 0.0), point(0.0, 3.0), point(2.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::vector<std::vector<PointId>> sources{{0U}, {2U}};
  const ExactStrictGammaBudget budget = full_budget();
  const ExactStrictGammaResult result =
      build_exact_strict_gamma_source_classification(
          cloud, 1U, level(4), sources, budget);
  const ExactStrictGammaVerification verification =
      verify_exact_strict_gamma_source_classification(
          cloud, 1U, level(4), sources, budget, result);

  check(
      result.active_facets.size() == 3U &&
          result.active_cofaces.size() == 2U &&
          result.active_cofaces[0].coface_point_ids ==
              std::vector<PointId>({0U, 1U}) &&
          result.active_cofaces[1].coface_point_ids ==
              std::vector<PointId>({1U, 2U}) &&
          result.active_cofaces[0].squared_level == level(13, 4) &&
          result.active_cofaces[1].squared_level == level(13, 4),
      "AP and PC are the two active cofaces strictly below four");
  check(
      result.components.size() == 1U &&
          result.components[0].facet_point_ids ==
              std::vector<std::vector<PointId>>{{0U}, {1U}, {2U}} &&
          result.source_classifications[0].component_index ==
              std::optional<std::size_t>{0U} &&
          result.source_classifications[1].component_index ==
              std::optional<std::size_t>{0U} &&
          all_certificates_close(verification),
      "the AP-PC path gives one exhaustive strict-Gamma component");
}

[[nodiscard]] ExactStrictGammaResult ternary_fixture_result(
    const CanonicalPointCloud& cloud,
    const std::vector<std::vector<PointId>>& sources,
    ExactStrictGammaBudget budget) {
  return build_exact_strict_gamma_source_classification(
      cloud, 2U, level(169, 36), sources, budget);
}

void test_ternary_sources_are_classified_by_exhaustive_gamma() {
  // Canonical ids are A=0, Q=1, B=2, C=3 and P=4.
  const std::array<CertifiedPoint3, 5> input{
      point(-2.0, 0.0),
      point(-2.0, 2.0),
      point(0.0, 3.0),
      point(2.0, 0.0),
      point(2.0, 2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::vector<std::vector<PointId>> sources{
      {2U, 3U}, {0U, 3U}, {0U, 2U}};
  const ExactStrictGammaBudget budget = full_budget();
  const ExactStrictGammaResult result =
      ternary_fixture_result(cloud, sources, budget);
  const ExactStrictGammaVerification verification =
      verify_exact_strict_gamma_source_classification(
          cloud, 2U, level(169, 36), sources, budget, result);

  const std::vector<std::vector<PointId>> expected_active_facets{
      {0U, 1U}, {0U, 2U}, {0U, 3U}, {1U, 2U},
      {1U, 4U}, {2U, 3U}, {2U, 4U}, {3U, 4U}};
  const std::vector<std::vector<PointId>> expected_active_cofaces{
      {0U, 1U, 2U}, {1U, 2U, 4U}, {2U, 3U, 4U}};
  std::vector<std::vector<PointId>> actual_active_facets;
  std::vector<std::vector<PointId>> actual_active_cofaces;
  for (const auto& facet : result.active_facets) {
    actual_active_facets.push_back(facet.facet_point_ids);
  }
  for (const auto& coface : result.active_cofaces) {
    actual_active_cofaces.push_back(coface.coface_point_ids);
  }

  check(
      actual_active_facets == expected_active_facets &&
          actual_active_cofaces == expected_active_cofaces &&
          result.active_cofaces[0].squared_level == level(13, 4) &&
          result.active_cofaces[1].squared_level == level(4) &&
          result.active_cofaces[2].squared_level == level(13, 4),
      "the ternary fixture has exactly eight facets and three open cofaces");
  check(
      result.components.size() == 2U &&
          result.components[0].canonical_representative_facet_point_ids ==
              std::vector<PointId>({0U, 1U}) &&
          result.components[0].facet_point_ids ==
              std::vector<std::vector<PointId>>{
                  {0U, 1U}, {0U, 2U}, {1U, 2U}, {1U, 4U},
                  {2U, 3U}, {2U, 4U}, {3U, 4U}} &&
          result.components[1].facet_point_ids ==
              std::vector<std::vector<PointId>>{{0U, 3U}},
      "the exhaustive open cut has one seven-facet and one isolated component");
  check(
      result.source_classifications.size() == 3U &&
          result.source_classifications[0].component_index ==
              std::optional<std::size_t>{0U} &&
          result.source_classifications[1].component_index ==
              std::optional<std::size_t>{1U} &&
          result.source_classifications[2].component_index ==
              std::optional<std::size_t>{0U} &&
          result.counters.active_facet_count == 8U &&
          result.counters.active_coface_count == 3U &&
          result.counters.component_count == 2U &&
          result.counters.isolated_component_count == 1U &&
          all_certificates_close(verification),
      "BC, AC and AB map canonically to components zero, one and zero");
}

void test_preflight_budget_stops_before_geometry() {
  const std::array<CertifiedPoint3, 3> input{
      point(-2.0), point(0.0), point(2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::vector<std::vector<PointId>> sources{{0U}};
  const ExactStrictGammaBudget budget{2U, 3U, 3U};
  const ExactStrictGammaResult result =
      build_exact_strict_gamma_source_classification(
          cloud, 1U, level(4), sources, budget);
  const ExactStrictGammaVerification verification =
      verify_exact_strict_gamma_source_classification(
          cloud, 1U, level(4), sources, budget, result);

  check(
      result.required_facet_count == 3U &&
          result.required_coface_count == 3U &&
          result.required_union_attempt_count == 3U &&
          result.candidate_space_size_certified &&
          result.decision == ExactStrictGammaDecision::
                                 no_cut_preflight_budget_insufficient,
      "preflight reports the exact candidate space before refusing the cut");
  check(
      result.active_facets.empty() && result.active_cofaces.empty() &&
          result.components.empty() &&
          result.source_classifications.empty() &&
          result.counters.preflight_count == 1U &&
          result.counters.enumerated_facet_count == 0U &&
          result.counters.facet_miniball_build_count == 0U &&
          result.counters.enumerated_coface_count == 0U &&
          result.counters.direct_coface_miniball_build_count == 0U &&
          result.counters.source_lookup_count == 0U &&
          all_certificates_close(verification),
      "an insufficient preflight budget performs no geometry or source lookup");

  const ExactStrictGammaBudget coface_budget{3U, 2U, 3U};
  const ExactStrictGammaResult coface_limited =
      build_exact_strict_gamma_source_classification(
          cloud, 1U, level(4), sources, coface_budget);
  const ExactStrictGammaVerification coface_verification =
      verify_exact_strict_gamma_source_classification(
          cloud,
          1U,
          level(4),
          sources,
          coface_budget,
          coface_limited);
  const ExactStrictGammaBudget union_budget{3U, 3U, 2U};
  const ExactStrictGammaResult union_limited =
      build_exact_strict_gamma_source_classification(
          cloud, 1U, level(4), sources, union_budget);
  const ExactStrictGammaVerification union_verification =
      verify_exact_strict_gamma_source_classification(
          cloud,
          1U,
          level(4),
          sources,
          union_budget,
          union_limited);
  check(
      coface_limited.decision == ExactStrictGammaDecision::
                                         no_cut_preflight_budget_insufficient &&
          union_limited.decision == ExactStrictGammaDecision::
                                        no_cut_preflight_budget_insufficient &&
          coface_limited.counters.enumerated_facet_count == 0U &&
          union_limited.counters.enumerated_facet_count == 0U &&
          all_certificates_close(coface_verification) &&
          all_certificates_close(union_verification),
      "facet, coface and union budgets independently gate the same atomic "
      "preflight");
}

void test_preflight_caps_cover_the_full_bounded_domain() {
  const std::array<CertifiedPoint3, 14> input{
      point(0.0),  point(1.0),  point(2.0),  point(3.0),
      point(4.0),  point(5.0),  point(6.0),  point(7.0),
      point(8.0),  point(9.0),  point(10.0), point(11.0),
      point(12.0), point(13.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactStrictGammaBudget zero_budget{};
  const std::vector<std::vector<PointId>> order_six_source{
      {0U, 1U, 2U, 3U, 4U, 5U}};
  const ExactStrictGammaResult order_six =
      build_exact_strict_gamma_source_classification(
          cloud,
          6U,
          level(1),
          order_six_source,
          zero_budget);
  const std::vector<std::vector<PointId>> order_seven_source{
      {0U, 1U, 2U, 3U, 4U, 5U, 6U}};
  const ExactStrictGammaResult order_seven =
      build_exact_strict_gamma_source_classification(
          cloud,
          7U,
          level(1),
          order_seven_source,
          zero_budget);

  check(
      order_six.required_facet_count == 3003U &&
          order_six.required_coface_count ==
              ExactStrictGammaBudget::maximum_supported_coface_count &&
          order_six.required_union_attempt_count == 20592U &&
          order_seven.required_facet_count ==
              ExactStrictGammaBudget::maximum_supported_facet_count &&
          order_seven.required_coface_count == 3003U &&
          order_seven.required_union_attempt_count ==
              ExactStrictGammaBudget::maximum_supported_union_attempt_count &&
          order_six.decision == ExactStrictGammaDecision::
                                    no_cut_preflight_budget_insufficient &&
          order_seven.decision == ExactStrictGammaDecision::
                                      no_cut_preflight_budget_insufficient,
      "the published caps cover the exact n=14 binomial maxima without "
      "starting geometry");
}

void test_inactive_source_is_explicit() {
  const std::array<CertifiedPoint3, 2> input{
      point(-2.0), point(2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::vector<std::vector<PointId>> sources{{0U}};
  const ExactStrictGammaBudget budget = full_budget();
  const ExactStrictGammaResult result =
      build_exact_strict_gamma_source_classification(
          cloud, 1U, level(0), sources, budget);
  const ExactStrictGammaVerification verification =
      verify_exact_strict_gamma_source_classification(
          cloud, 1U, level(0), sources, budget, result);

  check(
      result.active_facets.empty() && result.components.empty() &&
          result.source_classifications.size() == 1U &&
          result.source_classifications[0].squared_level == level(0) &&
          !result.source_classifications[0].active_strictly_below_cut &&
          !result.source_classifications[0].component_index.has_value() &&
          !result.all_sources_active_and_classified &&
          result.counters.inactive_source_count == 1U &&
          result.decision == ExactStrictGammaDecision::
                                 complete_with_inactive_sources &&
          all_certificates_close(verification),
      "a source born exactly at the cut is explicitly inactive");
}

void test_ternary_sources_include_an_explicit_inactive_facet() {
  // Canonical ids are A=0, Q=1, B=2, C=3 and P=4.
  const std::array<CertifiedPoint3, 5> input{
      point(-2.0, 0.0),
      point(-2.0, 2.0),
      point(0.0, 3.0),
      point(2.0, 0.0),
      point(2.0, 2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::vector<std::vector<PointId>> sources{
      {2U, 3U}, {0U, 3U}, {0U, 2U}};
  const ExactStrictGammaBudget budget = full_budget();
  const ExactStrictGammaResult result =
      build_exact_strict_gamma_source_classification(
          cloud, 2U, level(4), sources, budget);
  const ExactStrictGammaVerification verification =
      verify_exact_strict_gamma_source_classification(
          cloud, 2U, level(4), sources, budget, result);

  check(
      result.source_classifications.size() == 3U &&
          result.source_classifications[0].active_strictly_below_cut &&
          result.source_classifications[0].component_index ==
              std::optional<std::size_t>{1U} &&
          !result.source_classifications[1].active_strictly_below_cut &&
          !result.source_classifications[1].component_index.has_value() &&
          result.source_classifications[1].squared_level == level(4) &&
          result.source_classifications[2].active_strictly_below_cut &&
          result.source_classifications[2].component_index ==
              std::optional<std::size_t>{0U},
      "the ternary source family distinguishes two active components and "
      "one equal-level inactive facet");
  check(
      result.components.size() == 2U &&
          result.counters.active_source_count == 2U &&
          result.counters.inactive_source_count == 1U &&
          result.decision == ExactStrictGammaDecision::
                                 complete_with_inactive_sources &&
          all_certificates_close(verification),
      "a partially inactive ternary classification remains a complete cut");
}

void test_gabriel_counterexample_keeps_silent_gamma_incidences() {
  // Input labels are A, B, C, D and E from the permanent regression.
  // Canonical ids are D=0, A=1, B=2, C=3 and E=4.
  const std::array<CertifiedPoint3, 5> input{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::vector<std::vector<PointId>> sources{
      {1U, 3U}, {0U, 4U}, {1U, 2U}, {2U, 3U}};
  const ExactStrictGammaBudget budget = full_budget();
  const ExactLevel later_gabriel_level = level(83886, 3563);
  const ExactStrictGammaResult result =
      build_exact_strict_gamma_source_classification(
          cloud, 2U, later_gabriel_level, sources, budget);
  const ExactStrictGammaVerification verification =
      verify_exact_strict_gamma_source_classification(
          cloud,
          2U,
          later_gabriel_level,
          sources,
          budget,
          result);

  check(
      result.active_cofaces.size() == 4U &&
          result.active_cofaces[0].coface_point_ids ==
              std::vector<PointId>({0U, 1U, 3U}) &&
          result.active_cofaces[0].squared_level == level(33, 2) &&
          result.active_cofaces[3].coface_point_ids ==
              std::vector<PointId>({1U, 3U, 4U}) &&
          result.active_cofaces[3].squared_level == level(33, 2),
      "the open Gamma cut retains both earlier non-Gabriel AC incidences");
  check(
      result.components.size() == 3U &&
          result.source_classifications[0].component_index ==
              std::optional<std::size_t>{0U} &&
          result.source_classifications[1].component_index ==
              std::optional<std::size_t>{0U} &&
          result.source_classifications[2].component_index ==
              std::optional<std::size_t>{1U} &&
          result.source_classifications[3].component_index ==
              std::optional<std::size_t>{2U} &&
          all_certificates_close(verification),
      "AC is already attached through silent Gamma growth before the later "
      "equal-level ABC coface joins the remaining components");
}

void test_order_ten_uses_deletion_witness() {
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
  std::vector<PointId> source(10U);
  std::iota(source.begin(), source.end(), PointId{0});
  const std::vector<std::vector<PointId>> sources{source};
  const ExactStrictGammaBudget budget = full_budget();
  const ExactStrictGammaResult result =
      build_exact_strict_gamma_source_classification(
          cloud, 10U, level(26), sources, budget);
  const ExactStrictGammaVerification verification =
      verify_exact_strict_gamma_source_classification(
          cloud, 10U, level(26), sources, budget, result);

  check(
      result.required_facet_count == 11U &&
          result.required_coface_count == 1U &&
          result.required_union_attempt_count == 10U &&
          result.active_facets.size() == 11U &&
          result.active_cofaces.size() == 1U &&
          result.components.size() == 1U &&
          result.source_classifications[0].component_index ==
              std::optional<std::size_t>{0U},
      "the order-ten cut exhausts eleven deletion facets and one coface");
  check(
      result.active_cofaces[0].squared_level == level(25) &&
          result.active_cofaces[0].eleven_point_witness.has_value(),
      "the eleven-point coface carries its exact deletion witness");
  if (result.active_cofaces[0].eleven_point_witness.has_value()) {
    const auto& witness =
        *result.active_cofaces[0].eleven_point_witness;
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
        "the first maximal deletion ball covers its omitted point exactly");
  }
  check(
      result.counters.direct_coface_miniball_build_count == 0U &&
          result.counters.enumerated_facet_count == 11U &&
          result.counters.facet_miniball_build_count == 11U &&
          result.counters.facet_strict_level_comparison_count == 11U &&
          result.counters.active_facet_count == 11U &&
          result.counters.enumerated_coface_count == 1U &&
          result.counters.eleven_point_coface_count == 1U &&
          result.counters.eleven_point_deletion_level_lookup_count == 11U &&
          result.counters.eleven_point_level_maximum_comparison_count == 10U &&
          result.counters.
                  eleven_point_omitted_point_distance_evaluation_count == 1U &&
          result.counters.coface_strict_level_comparison_count == 1U &&
          result.counters.active_coface_count == 1U &&
          result.counters.coface_facet_lookup_count == 11U &&
          result.counters.disjoint_set_value_count == 11U &&
          result.counters.union_attempt_count == 10U &&
          result.counters.union_merge_count == 10U &&
          result.counters.component_count == 1U &&
          result.counters.isolated_component_count == 0U &&
          result.counters.source_lookup_count == 1U &&
          result.counters.active_source_count == 1U &&
          result.counters.inactive_source_count == 0U &&
          all_certificates_close(verification),
      "the order-ten witness and its bounded work replay exactly");

  if (result.active_cofaces[0].eleven_point_witness.has_value()) {
    ExactStrictGammaResult bad_witness = result;
    bad_witness.active_cofaces[0]
        .eleven_point_witness->omitted_point_squared_distance = level(17);
    const ExactStrictGammaVerification bad_verification =
        verify_exact_strict_gamma_source_classification(
            cloud,
            10U,
            level(26),
            sources,
            budget,
            bad_witness);
    check(
        !bad_verification.active_cofaces_certified &&
            !bad_verification.fresh_replay_certified &&
            !bad_verification.exact_strict_gamma_decision_certified,
        "the verifier rejects a falsified eleven-point deletion witness");
  }
}

void test_invalid_inputs_are_rejected() {
  const std::array<CertifiedPoint3, 3> input{
      point(-2.0), point(0.0), point(2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactStrictGammaBudget budget = full_budget();
  const std::vector<std::vector<PointId>> source_one{{0U}};
  const std::vector<std::vector<PointId>> source_pair{{0U, 1U}};
  const std::vector<std::vector<PointId>> no_sources;
  const std::vector<std::vector<PointId>> too_many_sources{
      {0U}, {1U}, {2U}, {0U}, {1U}};
  const std::vector<std::vector<PointId>> duplicate_sources{{0U}, {0U}};
  const std::vector<std::vector<PointId>> unsorted_source{{1U, 0U}};
  const std::vector<std::vector<PointId>> repeated_id_source{{0U, 0U}};
  const std::vector<std::vector<PointId>> out_of_range_source{{0U, 3U}};

  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(
            build_exact_strict_gamma_source_classification(
                cloud, 0U, level(4), source_one, budget));
      },
      "order zero is rejected");
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(
            build_exact_strict_gamma_source_classification(
                cloud, 3U, level(4), source_pair, budget));
      },
      "an order equal to the point count is rejected");
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(
            build_exact_strict_gamma_source_classification(
                cloud, 1U, level(4), no_sources, budget));
      },
      "an empty source family is rejected");
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(
            build_exact_strict_gamma_source_classification(
                cloud, 1U, level(4), too_many_sources, budget));
      },
      "more than four source facets are rejected before classification");
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(
            build_exact_strict_gamma_source_classification(
                cloud, 1U, level(4), source_pair, budget));
      },
      "a source with the wrong cardinality is rejected");
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(
            build_exact_strict_gamma_source_classification(
                cloud, 1U, level(4), duplicate_sources, budget));
      },
      "duplicate source facets are rejected");
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(
            build_exact_strict_gamma_source_classification(
                cloud, 2U, level(4), unsorted_source, budget));
      },
      "an unsorted source facet is rejected");
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(
            build_exact_strict_gamma_source_classification(
                cloud, 2U, level(4), repeated_id_source, budget));
      },
      "a source facet with repeated ids is rejected");
  check_throws<std::out_of_range>(
      [&]() {
        static_cast<void>(
            build_exact_strict_gamma_source_classification(
                cloud, 2U, level(4), out_of_range_source, budget));
      },
      "a source id outside the cloud is rejected");

  ExactStrictGammaBudget oversized_budget = budget;
  ++oversized_budget.maximum_enumerated_facet_count;
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(
            build_exact_strict_gamma_source_classification(
                cloud, 1U, level(4), source_one, oversized_budget));
      },
      "a budget above the bounded reference cap is rejected");
}

void test_verifier_rejects_falsifications() {
  const std::array<CertifiedPoint3, 5> input{
      point(-2.0, 0.0),
      point(-2.0, 2.0),
      point(0.0, 3.0),
      point(2.0, 0.0),
      point(2.0, 2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::vector<std::vector<PointId>> sources{
      {2U, 3U}, {0U, 3U}, {0U, 2U}};
  const ExactStrictGammaBudget budget = full_budget();
  const ExactStrictGammaResult result =
      ternary_fixture_result(cloud, sources, budget);

  ExactStrictGammaResult bad_budget = result;
  --bad_budget.requested_budget.maximum_enumerated_facet_count;
  auto verification = verify_exact_strict_gamma_source_classification(
      cloud, 2U, level(169, 36), sources, budget, bad_budget);
  check(
      !verification.requested_budget_certified &&
          !verification.fresh_replay_certified &&
          !verification.exact_strict_gamma_decision_certified,
      "the verifier rejects a falsified requested budget");

  ExactStrictGammaResult bad_input = result;
  bad_input.strict_cut_squared_level = level(4);
  verification = verify_exact_strict_gamma_source_classification(
      cloud, 2U, level(169, 36), sources, budget, bad_input);
  check(
      !verification.external_inputs_certified &&
          !verification.exact_strict_gamma_decision_certified,
      "the verifier rejects a falsified external cut level");

  ExactStrictGammaResult bad_preflight = result;
  ++bad_preflight.required_union_attempt_count;
  verification = verify_exact_strict_gamma_source_classification(
      cloud, 2U, level(169, 36), sources, budget, bad_preflight);
  check(
      !verification.preflight_counts_certified &&
          !verification.exact_strict_gamma_decision_certified,
      "the verifier rejects falsified preflight counts");

  ExactStrictGammaResult bad_facet = result;
  bad_facet.active_facets.front().squared_level = level(99);
  verification = verify_exact_strict_gamma_source_classification(
      cloud, 2U, level(169, 36), sources, budget, bad_facet);
  check(
      !verification.active_facets_certified &&
          !verification.fresh_replay_certified &&
          !verification.exact_strict_gamma_decision_certified,
      "the verifier rejects a falsified active-facet level");

  ExactStrictGammaResult bad_coface = result;
  bad_coface.active_cofaces.front().facet_point_ids.pop_back();
  verification = verify_exact_strict_gamma_source_classification(
      cloud, 2U, level(169, 36), sources, budget, bad_coface);
  check(
      !verification.active_cofaces_certified &&
          !verification.exact_strict_gamma_decision_certified,
      "the verifier rejects a falsified active-coface incidence");

  ExactStrictGammaResult bad_component = result;
  bad_component.components.front().facet_point_ids.pop_back();
  verification = verify_exact_strict_gamma_source_classification(
      cloud, 2U, level(169, 36), sources, budget, bad_component);
  check(
      !verification.components_certified &&
          !verification.exact_strict_gamma_decision_certified,
      "the verifier rejects a falsified component catalog");

  ExactStrictGammaResult bad_source = result;
  bad_source.source_classifications.front().component_index = 1U;
  verification = verify_exact_strict_gamma_source_classification(
      cloud, 2U, level(169, 36), sources, budget, bad_source);
  check(
      !verification.source_classifications_certified &&
          !verification.exact_strict_gamma_decision_certified,
      "the verifier rejects a falsified source classification");

  ExactStrictGammaResult bad_counter = result;
  ++bad_counter.counters.union_attempt_count;
  verification = verify_exact_strict_gamma_source_classification(
      cloud, 2U, level(169, 36), sources, budget, bad_counter);
  check(
      !verification.counters_certified &&
          !verification.exact_strict_gamma_decision_certified,
      "the verifier rejects falsified exhaustive-work counters");

  ExactStrictGammaResult bad_fact = result;
  bad_fact.full_pi0_isolated_facets_included = false;
  verification = verify_exact_strict_gamma_source_classification(
      cloud, 2U, level(169, 36), sources, budget, bad_fact);
  check(
      !verification.result_facts_certified &&
          !verification.exact_strict_gamma_decision_certified,
      "the verifier rejects a falsified result fact");

  ExactStrictGammaResult bad_decision = result;
  bad_decision.decision =
      ExactStrictGammaDecision::complete_with_inactive_sources;
  verification = verify_exact_strict_gamma_source_classification(
      cloud, 2U, level(169, 36), sources, budget, bad_decision);
  check(
      !verification.decision_certified &&
          !verification.exact_strict_gamma_decision_certified,
      "the verifier rejects a falsified decision");

  ExactStrictGammaResult bad_scope = result;
  bad_scope.scope = ExactStrictGammaScope::unspecified;
  verification = verify_exact_strict_gamma_source_classification(
      cloud, 2U, level(169, 36), sources, budget, bad_scope);
  check(
      !verification.scope_certified &&
          !verification.exact_strict_gamma_decision_certified,
      "the verifier treats the source-component-only scope as untrusted");
}

}  // namespace

int main() {
  test_strict_cut_excludes_equal_coface();
  test_two_strict_cofaces_connect_three_singletons();
  test_ternary_sources_are_classified_by_exhaustive_gamma();
  test_preflight_budget_stops_before_geometry();
  test_preflight_caps_cover_the_full_bounded_domain();
  test_inactive_source_is_explicit();
  test_ternary_sources_include_an_explicit_inactive_facet();
  test_gabriel_counterexample_keeps_silent_gamma_incidences();
  test_order_ten_uses_deletion_witness();
  test_invalid_inputs_are_rejected();
  test_verifier_rejects_falsifications();

  if (failures != 0) {
    std::cerr << failures << " test assertion(s) failed\n";
    return 1;
  }
  std::cout << "all exact strict-Gamma hierarchy tests passed\n";
  return 0;
}
