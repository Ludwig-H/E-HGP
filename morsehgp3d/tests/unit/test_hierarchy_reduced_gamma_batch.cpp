#include "morsehgp3d/hierarchy/reduced_gamma_batch.hpp"

#include <algorithm>
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
using morsehgp3d::hierarchy::ExactGammaTransitionDecision;
using morsehgp3d::hierarchy::ExactReducedGammaBatchDecision;
using morsehgp3d::hierarchy::ExactReducedGammaBatchGroup;
using morsehgp3d::hierarchy::ExactReducedGammaBatchGroupKind;
using morsehgp3d::hierarchy::ExactReducedGammaBatchResult;
using morsehgp3d::hierarchy::ExactReducedGammaBatchScope;
using morsehgp3d::hierarchy::ExactReducedGammaBatchVerification;
using morsehgp3d::hierarchy::ExactReducedGammaStrictComponentKind;
using morsehgp3d::hierarchy::ExactStrictGammaBudget;
using morsehgp3d::hierarchy::build_exact_reduced_gamma_batch;
using morsehgp3d::hierarchy::verify_exact_reduced_gamma_batch;
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
    const ExactReducedGammaBatchVerification& verification) {
  return verification.requested_budget_certified &&
         verification.external_inputs_certified &&
         verification.gamma_transition_certified &&
         verification.strict_component_classifications_certified &&
         verification.groups_certified &&
         verification.result_facts_certified &&
         verification.counters_certified &&
         verification.decision_certified &&
         verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.exact_reduced_gamma_batch_decision_certified;
}

void check_complete_facts(
    const ExactReducedGammaBatchResult& result,
    const std::string& label) {
  check(
      result.decision == ExactReducedGammaBatchDecision::
                             complete_exhaustive_reduced_gamma_batch &&
          result.scope == ExactReducedGammaBatchScope::
                              bounded_exhaustive_gamma_single_equal_level_hgp_reduced_semantics_orders_two_to_ten_with_k_less_than_n_only &&
          result.gamma_transition.decision ==
              ExactGammaTransitionDecision::
                  complete_exhaustive_open_to_closed_transition &&
          result.gamma_transition_fresh_replay_certified &&
          result.strict_components_exhaustively_classified &&
          result.
              strict_component_incidence_nontriviality_equivalence_certified &&
          result.strict_reduced_roots_exactly_nontrivial_components &&
          result.transition_groups_exhaustively_classified &&
          result.strict_components_partitioned_within_groups &&
          result.isolated_facets_deferred_without_reduced_root &&
          result.equal_level_coface_groups_use_reduced_root_count &&
          result.coverage_deltas_are_exact_set_differences &&
          result.equal_level_batch_semantics_certified,
      label + ": all bounded exhaustive reduction facts close");
}

void check_strict_classifications(
    const ExactReducedGammaBatchResult& result,
    std::size_t expected_root_count,
    std::size_t expected_isolated_count,
    const std::string& label) {
  const auto& components =
      result.gamma_transition.strict_gamma.components;
  check(
      result.strict_component_classifications.size() ==
          components.size(),
      label + ": every strict component is classified");
  std::size_t root_count = 0U;
  std::size_t isolated_count = 0U;
  const std::size_t count = std::min(
      result.strict_component_classifications.size(),
      components.size());
  for (std::size_t index = 0U; index < count; ++index) {
    const auto& classification =
        result.strict_component_classifications[index];
    const auto& component = components[index];
    const bool nontrivial = component.facet_point_ids.size() > 1U;
    check(
        classification.strict_component_index == index &&
            classification.canonical_representative_facet_point_ids ==
                component.canonical_representative_facet_point_ids &&
            classification.facet_count ==
                component.facet_point_ids.size() &&
            classification.incident_to_strict_coface == nontrivial &&
            classification.facet_count_is_nontrivial == nontrivial &&
            classification.
                incidence_nontriviality_equivalence_certified &&
            classification.carries_prior_reduced_root == nontrivial &&
            classification.incident_strict_coface_indices.empty() !=
                nontrivial,
        label + ": incidence and nontriviality agree for component " +
            std::to_string(index));
    if (classification.carries_prior_reduced_root) {
      ++root_count;
      check(
          classification.kind ==
              ExactReducedGammaStrictComponentKind::
                  prior_nontrivial_reduced_root,
          label + ": a nontrivial component carries a prior root");
    } else {
      ++isolated_count;
      check(
          classification.kind ==
              ExactReducedGammaStrictComponentKind::
                  omitted_isolated_facet,
          label + ": an isolated facet carries no prior root");
    }
  }
  check(
      root_count == expected_root_count &&
          isolated_count == expected_isolated_count &&
          result.counters.prior_reduced_root_count ==
              expected_root_count &&
          result.counters.omitted_isolated_strict_component_count ==
              expected_isolated_count,
      label + ": strict root and isolated counts are exact");
}

void check_group(
    const ExactReducedGammaBatchGroup& group,
    ExactReducedGammaBatchGroupKind expected_kind,
    std::size_t expected_parent_count,
    std::size_t expected_absorbed_count,
    const std::vector<std::vector<PointId>>& expected_added_facets,
    const std::vector<PointId>& expected_added_points,
    bool expected_fully_redundant,
    const std::string& label) {
  check(
      group.kind == expected_kind &&
          group.prior_reduced_root_strict_component_indices.size() ==
              expected_parent_count &&
          group.absorbed_isolated_strict_component_indices.size() ==
              expected_absorbed_count,
      label + ": reduced arity ignores absorbed isolated facets");
  check(
      group.coverage_delta.has_value(),
      label + ": a coface group publishes a coverage delta");
  if (group.coverage_delta.has_value()) {
    check(
        group.coverage_delta->added_facet_point_ids ==
                expected_added_facets &&
            group.coverage_delta->added_point_ids ==
                expected_added_points &&
            group.coverage_delta->fully_redundant ==
                expected_fully_redundant,
        label +
            ": facet and point deltas are taken after the parent unions");
  }
}

[[nodiscard]] CanonicalPointCloud q2_cloud() {
  const std::array<CertifiedPoint3, 3> input{
      point(-2.0, 0.0),
      point(0.0, 1.0),
      point(2.0, 0.0)};
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud e5_cloud() {
  const std::array<CertifiedPoint3, 5> input{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0)};
  return canonical_cloud(input);
}

void test_default_and_isolated_equal_facet_is_deferred() {
  const ExactReducedGammaBatchResult empty;
  check(
      empty.decision == ExactReducedGammaBatchDecision::not_certified &&
          empty.scope == ExactReducedGammaBatchScope::unspecified &&
          empty.strict_component_classifications.empty() &&
          empty.groups.empty() &&
          !empty.equal_level_batch_semantics_certified,
      "the default reduced batch certifies no semantics");

  const std::array<CertifiedPoint3, 3> input{
      point(0.0), point(2.0), point(10.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactStrictGammaBudget budget{3U, 1U, 2U};
  const ExactReducedGammaBatchResult result =
      build_exact_reduced_gamma_batch(
          cloud, 2U, level(1), budget);
  const ExactReducedGammaBatchVerification verification =
      verify_exact_reduced_gamma_batch(
          cloud, 2U, level(1), budget, result);

  check_complete_facts(result, "isolated equality facet");
  check_strict_classifications(
      result, 0U, 0U, "isolated equality facet");
  check(
      result.groups.size() == 1U &&
          result.groups[0].kind ==
              ExactReducedGammaBatchGroupKind::
                  deferred_isolated_facet &&
          result.groups[0].strict_component_indices.empty() &&
          result.groups[0].
              prior_reduced_root_strict_component_indices.empty() &&
          result.groups[0].
              absorbed_isolated_strict_component_indices.empty() &&
          result.groups[0].newly_active_facet_point_ids ==
              std::vector<std::vector<PointId>>({{0U, 1U}}) &&
          result.groups[0].equal_level_coface_point_ids.empty() &&
          !result.groups[0].coverage_delta.has_value() &&
          result.counters.deferred_isolated_facet_group_count == 1U &&
          result.counters.coverage_delta_count == 0U &&
          all_certificates_close(verification),
      "an equal isolated facet is deferred without root or delta");
}

void test_q2_is_a_birth_and_a_noncritical_level_has_no_group() {
  const CanonicalPointCloud cloud = q2_cloud();
  const ExactStrictGammaBudget budget{3U, 1U, 2U};
  const ExactReducedGammaBatchResult result =
      build_exact_reduced_gamma_batch(
          cloud, 2U, level(4), budget);
  const ExactReducedGammaBatchVerification verification =
      verify_exact_reduced_gamma_batch(
          cloud, 2U, level(4), budget, result);

  check_complete_facts(result, "q=2 triangle");
  check_strict_classifications(result, 0U, 2U, "q=2 triangle");
  check(
      result.groups.size() == 1U &&
          result.groups[0].strict_component_indices ==
              std::vector<std::size_t>({0U, 1U}) &&
          result.groups[0].newly_active_facet_point_ids ==
              std::vector<std::vector<PointId>>({{0U, 2U}}) &&
          result.groups[0].equal_level_coface_point_ids ==
              std::vector<std::vector<PointId>>({{0U, 1U, 2U}}),
      "q=2 keeps both frozen isolated components as absorbed evidence");
  if (!result.groups.empty()) {
    check_group(
        result.groups[0],
        ExactReducedGammaBatchGroupKind::birth,
        0U,
        2U,
        {{0U, 1U}, {0U, 2U}, {1U, 2U}},
        {0U, 1U, 2U},
        false,
        "q=2 triangle");
  }
  check(
      result.counters.birth_group_count == 1U &&
          result.counters.multifusion_group_count == 0U &&
          all_certificates_close(verification),
      "two strict isolated facets produce a reduced birth, never a fusion");

  const ExactReducedGammaBatchResult no_equality =
      build_exact_reduced_gamma_batch(
          cloud, 2U, level(2), budget);
  const ExactReducedGammaBatchVerification no_equality_verification =
      verify_exact_reduced_gamma_batch(
          cloud, 2U, level(2), budget, no_equality);
  check_complete_facts(no_equality, "noncritical level");
  check_strict_classifications(
      no_equality, 0U, 2U, "noncritical level");
  check(
      no_equality.gamma_transition.equal_level_facets.empty() &&
          no_equality.gamma_transition.equal_level_cofaces.empty() &&
          no_equality.groups.empty() &&
          no_equality.counters.transition_group_classification_count ==
              0U &&
          no_equality.counters.coverage_delta_count == 0U &&
          all_certificates_close(no_equality_verification),
      "a level without equality emits no reduced batch group");
}

void test_mirror_q5_is_still_one_reduced_birth() {
  const std::array<CertifiedPoint3, 4> input{
      point(-2.0, 0.0),
      point(0.0, -3.0),
      point(0.0, 3.0),
      point(2.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactStrictGammaBudget budget{6U, 4U, 8U};
  const ExactReducedGammaBatchResult result =
      build_exact_reduced_gamma_batch(
          cloud, 2U, level(169, 36), budget);
  const ExactReducedGammaBatchVerification verification =
      verify_exact_reduced_gamma_batch(
          cloud, 2U, level(169, 36), budget, result);

  check_complete_facts(result, "mirror q=5");
  check_strict_classifications(result, 0U, 5U, "mirror q=5");
  check(
      result.groups.size() == 1U &&
          result.groups[0].strict_component_indices ==
              std::vector<std::size_t>({0U, 1U, 2U, 3U, 4U}) &&
          result.groups[0].equal_level_coface_point_ids ==
              std::vector<std::vector<PointId>>(
                  {{0U, 1U, 3U}, {0U, 2U, 3U}}),
      "the simultaneous mirror group retains all five absorbed components");
  if (!result.groups.empty()) {
    check_group(
        result.groups[0],
        ExactReducedGammaBatchGroupKind::birth,
        0U,
        5U,
        {{0U, 1U},
         {0U, 2U},
         {0U, 3U},
         {1U, 3U},
         {2U, 3U}},
        {0U, 1U, 2U, 3U},
        false,
        "mirror q=5");
  }
  check(
      result.counters.birth_group_count == 1U &&
          result.counters.multifusion_group_count == 0U &&
          all_certificates_close(verification),
      "five isolated strict components remain a reduced birth");
}

void test_redundant_equal_coface_has_an_empty_exact_delta() {
  const std::array<CertifiedPoint3, 4> input{
      point(0.0, 0.0),
      point(4.0, 0.0),
      point(1.0, 3.0),
      point(1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactStrictGammaBudget budget{6U, 4U, 8U};
  const ExactReducedGammaBatchResult result =
      build_exact_reduced_gamma_batch(
          cloud, 2U, level(5), budget);
  const ExactReducedGammaBatchVerification verification =
      verify_exact_reduced_gamma_batch(
          cloud, 2U, level(5), budget, result);

  check_complete_facts(result, "redundant equal coface");
  check_strict_classifications(
      result, 1U, 0U, "redundant equal coface");
  check(
      result.groups.size() == 1U &&
          result.groups[0].newly_active_facet_point_ids.empty() &&
          result.groups[0].equal_level_coface_point_ids ==
              std::vector<std::vector<PointId>>({{0U, 2U, 3U}}),
      "the center point connects every outer-triangle facet strictly");
  if (!result.groups.empty()) {
    check_group(
        result.groups[0],
        ExactReducedGammaBatchGroupKind::continuation,
        1U,
        0U,
        {},
        {},
        true,
        "redundant equal coface");
  }
  check(
      result.counters.fully_redundant_coverage_delta_count == 1U &&
          all_certificates_close(verification),
      "an equality coface already covered by its parent is explicitly fully redundant");
}

void test_order_ten_boundary_is_a_reduced_birth() {
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
  const ExactReducedGammaBatchResult result =
      build_exact_reduced_gamma_batch(
          cloud, 10U, level(25), budget);
  const ExactReducedGammaBatchVerification verification =
      verify_exact_reduced_gamma_batch(
          cloud, 10U, level(25), budget, result);

  std::vector<std::vector<PointId>> expected_facets;
  for (std::size_t reverse_omitted = 11U;
       reverse_omitted > 0U;
       --reverse_omitted) {
    const PointId omitted =
        static_cast<PointId>(reverse_omitted - 1U);
    std::vector<PointId> facet;
    for (PointId point_id = 0U; point_id < 11U; ++point_id) {
      if (point_id != omitted) {
        facet.push_back(point_id);
      }
    }
    expected_facets.push_back(std::move(facet));
  }

  check_complete_facts(result, "order-ten boundary");
  check_strict_classifications(
      result, 0U, 2U, "order-ten boundary");
  check(
      result.groups.size() == 1U &&
          result.gamma_transition.equal_level_facets.size() == 9U &&
          result.gamma_transition.equal_level_cofaces.size() == 1U &&
          result.groups[0].newly_active_facet_point_ids.size() == 9U &&
          result.groups[0].equal_level_coface_point_ids ==
              std::vector<std::vector<PointId>>(
                  {{0U, 1U, 2U, 3U, 4U, 5U,
                    6U, 7U, 8U, 9U, 10U}}),
      "the maximal supported order remains one simultaneous equality batch");
  if (!result.groups.empty()) {
    check_group(
        result.groups[0],
        ExactReducedGammaBatchGroupKind::birth,
        0U,
        2U,
        expected_facets,
        {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U},
        false,
        "order-ten boundary");
  }
  check(
      result.counters.birth_group_count == 1U &&
          result.counters.multifusion_group_count == 0U &&
          all_certificates_close(verification),
      "two isolated order-ten facets produce one reduced birth");
}

void test_e5_births_multifusion_and_continuation() {
  const CanonicalPointCloud cloud = e5_cloud();
  const ExactStrictGammaBudget budget{10U, 10U, 20U};

  const auto run =
      [&cloud, budget](
          const ExactLevel& squared_level,
          const std::string& label) {
        const ExactReducedGammaBatchResult result =
            build_exact_reduced_gamma_batch(
                cloud, 2U, squared_level, budget);
        const ExactReducedGammaBatchVerification verification =
            verify_exact_reduced_gamma_batch(
                cloud, 2U, squared_level, budget, result);
        check_complete_facts(result, label);
        check(
            result.groups.size() == 1U &&
                all_certificates_close(verification),
            label + ": the event is one freshly replayed batch group");
        return result;
      };

  const ExactReducedGammaBatchResult birth_012 =
      run(level(25, 16), "E5 birth 012");
  check_strict_classifications(
      birth_012, 0U, 3U, "E5 birth 012");
  if (!birth_012.groups.empty()) {
    check(
        birth_012.groups[0].equal_level_coface_point_ids ==
            std::vector<std::vector<PointId>>({{0U, 1U, 2U}}),
        "E5 25/16 activates coface 012");
    check_group(
        birth_012.groups[0],
        ExactReducedGammaBatchGroupKind::birth,
        0U,
        3U,
        {{0U, 1U}, {0U, 2U}, {1U, 2U}},
        {0U, 1U, 2U},
        false,
        "E5 birth 012");
  }

  const ExactReducedGammaBatchResult birth_234 =
      run(level(1105, 242), "E5 birth 234");
  check_strict_classifications(
      birth_234, 1U, 3U, "E5 birth 234");
  if (!birth_234.groups.empty()) {
    check(
        birth_234.groups[0].equal_level_coface_point_ids ==
            std::vector<std::vector<PointId>>({{2U, 3U, 4U}}),
        "E5 1105/242 activates coface 234");
    check_group(
        birth_234.groups[0],
        ExactReducedGammaBatchGroupKind::birth,
        0U,
        3U,
        {{2U, 3U}, {2U, 4U}, {3U, 4U}},
        {2U, 3U, 4U},
        false,
        "E5 birth 234");
  }

  const ExactReducedGammaBatchResult merge =
      run(level(13, 2), "E5 binary multifusion");
  check_strict_classifications(
      merge, 2U, 0U, "E5 binary multifusion");
  if (!merge.groups.empty()) {
    check(
        merge.groups[0].newly_active_facet_point_ids ==
                std::vector<std::vector<PointId>>({{1U, 3U}}) &&
            merge.groups[0].equal_level_coface_point_ids ==
                std::vector<std::vector<PointId>>({{1U, 2U, 3U}}),
        "E5 13/2 attaches facet 13 through coface 123");
    check_group(
        merge.groups[0],
        ExactReducedGammaBatchGroupKind::multifusion,
        2U,
        0U,
        {{1U, 3U}},
        {},
        false,
        "E5 binary multifusion");
  }

  const ExactReducedGammaBatchResult continuation =
      run(level(17, 2), "E5 continuation");
  check_strict_classifications(
      continuation, 1U, 0U, "E5 continuation");
  if (!continuation.groups.empty()) {
    check(
        continuation.groups[0].newly_active_facet_point_ids ==
                std::vector<std::vector<PointId>>({{0U, 3U}}) &&
            continuation.groups[0].equal_level_coface_point_ids ==
                std::vector<std::vector<PointId>>(
                    {{0U, 1U, 3U}, {0U, 2U, 3U}}),
        "E5 17/2 attaches facet 03 through both equal cofaces");
    check_group(
        continuation.groups[0],
        ExactReducedGammaBatchGroupKind::continuation,
        1U,
        0U,
        {{0U, 3U}},
        {},
        false,
        "E5 continuation");
  }
}

void test_preflight_is_atomic_and_order_one_is_rejected() {
  const CanonicalPointCloud cloud = q2_cloud();
  const std::array<ExactStrictGammaBudget, 3> insufficient_budgets{
      ExactStrictGammaBudget{2U, 1U, 2U},
      ExactStrictGammaBudget{3U, 0U, 2U},
      ExactStrictGammaBudget{3U, 1U, 1U}};
  for (const ExactStrictGammaBudget& budget : insufficient_budgets) {
    const ExactReducedGammaBatchResult result =
        build_exact_reduced_gamma_batch(
            cloud, 2U, level(4), budget);
    const ExactReducedGammaBatchVerification verification =
        verify_exact_reduced_gamma_batch(
            cloud, 2U, level(4), budget, result);
    check(
        result.decision == ExactReducedGammaBatchDecision::
                               no_batch_preflight_budget_insufficient &&
            result.gamma_transition.decision ==
                ExactGammaTransitionDecision::
                    no_transition_preflight_budget_insufficient &&
            result.strict_component_classifications.empty() &&
            result.groups.empty() &&
            !result.gamma_transition_fresh_replay_certified &&
            !result.strict_components_exhaustively_classified &&
            !result.transition_groups_exhaustively_classified &&
            !result.equal_level_batch_semantics_certified &&
            result.counters.gamma_transition_build_count == 1U &&
            result.counters.strict_component_classification_count ==
                0U &&
            result.counters.transition_group_classification_count ==
                0U &&
            result.counters.coverage_delta_count == 0U &&
            all_certificates_close(verification),
        "an insufficient preflight publishes no partial reduced semantics");
  }

  check_throws<std::invalid_argument>(
      [&cloud]() {
        static_cast<void>(build_exact_reduced_gamma_batch(
            cloud,
            1U,
            level(1),
            ExactStrictGammaBudget{3U, 3U, 3U}));
      },
      "order one is outside the certified reduced Gamma scope");

  const std::array<CertifiedPoint3, 2> terminal_input{
      point(0.0), point(2.0)};
  const CanonicalPointCloud terminal_cloud =
      canonical_cloud(terminal_input);
  check_throws<std::invalid_argument>(
      [&terminal_cloud]() {
        static_cast<void>(build_exact_reduced_gamma_batch(
            terminal_cloud,
            2U,
            level(1),
            ExactStrictGammaBudget{1U, 0U, 0U}));
      },
      "k=n is outside the single-level Gamma transition domain");
}

void test_verifier_rejects_mutations_of_every_layer() {
  const CanonicalPointCloud cloud = q2_cloud();
  const ExactStrictGammaBudget budget{3U, 1U, 2U};
  const ExactReducedGammaBatchResult result =
      build_exact_reduced_gamma_batch(
          cloud, 2U, level(4), budget);

  const auto verify =
      [&cloud, budget](const ExactReducedGammaBatchResult& candidate) {
        return verify_exact_reduced_gamma_batch(
            cloud, 2U, level(4), budget, candidate);
      };
  const auto rejects =
      [&verify](
          const ExactReducedGammaBatchResult& candidate,
          bool layer_rejected,
          const std::string& message) {
        const ExactReducedGammaBatchVerification verification =
            verify(candidate);
        check(
            layer_rejected &&
                !verification.fresh_replay_certified &&
                !verification.
                    exact_reduced_gamma_batch_decision_certified,
            message);
      };

  ExactReducedGammaBatchResult bad_budget = result;
  --bad_budget.requested_budget.maximum_union_attempt_count;
  const auto bad_budget_verification = verify(bad_budget);
  rejects(
      bad_budget,
      !bad_budget_verification.requested_budget_certified,
      "the verifier rejects a mutated requested budget");

  ExactReducedGammaBatchResult bad_input = result;
  ++bad_input.point_count;
  const auto bad_input_verification = verify(bad_input);
  rejects(
      bad_input,
      !bad_input_verification.external_inputs_certified,
      "the verifier rejects mutated external input facts");

  ExactReducedGammaBatchResult bad_transition = result;
  ++bad_transition.gamma_transition.counters.equal_level_incidence_count;
  const auto bad_transition_verification = verify(bad_transition);
  rejects(
      bad_transition,
      !bad_transition_verification.gamma_transition_certified,
      "the verifier freshly replays the embedded 6.10 transition");

  ExactReducedGammaBatchResult bad_classification = result;
  ++bad_classification.strict_component_classifications[0].facet_count;
  const auto bad_classification_verification =
      verify(bad_classification);
  rejects(
      bad_classification,
      !bad_classification_verification.
          strict_component_classifications_certified,
      "the verifier rejects a mutated strict-component classification");

  ExactReducedGammaBatchResult bad_group = result;
  bad_group.groups[0].coverage_delta->added_point_ids.pop_back();
  const auto bad_group_verification = verify(bad_group);
  rejects(
      bad_group,
      !bad_group_verification.groups_certified,
      "the verifier rejects a mutated post-union coverage delta");

  ExactReducedGammaBatchResult bad_fact = result;
  bad_fact.coverage_deltas_are_exact_set_differences = false;
  const auto bad_fact_verification = verify(bad_fact);
  rejects(
      bad_fact,
      !bad_fact_verification.result_facts_certified,
      "the verifier rejects a mutated certified result fact");

  ExactReducedGammaBatchResult bad_counter = result;
  ++bad_counter.counters.coverage_delta_count;
  const auto bad_counter_verification = verify(bad_counter);
  rejects(
      bad_counter,
      !bad_counter_verification.counters_certified,
      "the verifier rejects a mutated exhaustive counter");

  ExactReducedGammaBatchResult bad_decision = result;
  bad_decision.decision = ExactReducedGammaBatchDecision::
      no_batch_preflight_budget_insufficient;
  const auto bad_decision_verification = verify(bad_decision);
  rejects(
      bad_decision,
      !bad_decision_verification.decision_certified,
      "the verifier rejects a mutated reduced decision");

  ExactReducedGammaBatchResult bad_scope = result;
  bad_scope.scope = ExactReducedGammaBatchScope::unspecified;
  const auto bad_scope_verification = verify(bad_scope);
  rejects(
      bad_scope,
      !bad_scope_verification.scope_certified,
      "the verifier rejects a mutated bounded scope");
}

}  // namespace

int main() {
  test_default_and_isolated_equal_facet_is_deferred();
  test_q2_is_a_birth_and_a_noncritical_level_has_no_group();
  test_mirror_q5_is_still_one_reduced_birth();
  test_redundant_equal_coface_has_an_empty_exact_delta();
  test_order_ten_boundary_is_a_reduced_birth();
  test_e5_births_multifusion_and_continuation();
  test_preflight_is_atomic_and_order_one_is_rejected();
  test_verifier_rejects_mutations_of_every_layer();

  if (failures != 0) {
    std::cerr << failures << " test assertion(s) failed\n";
    return 1;
  }
  std::cout << "all exact reduced Gamma batch tests passed\n";
  return 0;
}
