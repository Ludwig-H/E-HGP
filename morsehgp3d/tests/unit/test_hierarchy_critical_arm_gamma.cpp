#include "morsehgp3d/hierarchy/critical_arm_gamma.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
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
using morsehgp3d::hierarchy::ExactCriticalArmGammaDecision;
using morsehgp3d::hierarchy::ExactCriticalArmGammaResult;
using morsehgp3d::hierarchy::ExactCriticalArmGammaScope;
using morsehgp3d::hierarchy::ExactCriticalArmGammaVerification;
using morsehgp3d::hierarchy::ExactFacetDescentChainBudget;
using morsehgp3d::hierarchy::ExactStrictGammaBudget;
using morsehgp3d::hierarchy::ExactStrictGammaDecision;
using morsehgp3d::hierarchy::build_exact_critical_arm_gamma_component_classification;
using morsehgp3d::hierarchy::verify_exact_critical_arm_gamma_component_classification;
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

[[nodiscard]] ExactStrictGammaBudget bridge_gamma_budget() {
  return ExactStrictGammaBudget{10U, 10U, 20U};
}

[[nodiscard]] ExactStrictGammaBudget maximum_gamma_budget() {
  return ExactStrictGammaBudget{
      ExactStrictGammaBudget::maximum_supported_facet_count,
      ExactStrictGammaBudget::maximum_supported_coface_count,
      ExactStrictGammaBudget::maximum_supported_union_attempt_count};
}

[[nodiscard]] bool all_certificates_close(
    const ExactCriticalArmGammaVerification& verification) {
  return verification.requested_per_arm_chain_budget_certified &&
         verification.requested_strict_gamma_budget_certified &&
         verification.input_shell_identity_certified &&
         verification.arm_family_certified &&
         verification.critical_order_and_level_certified &&
         verification.strict_gamma_presence_certified &&
         verification.strict_gamma_certified &&
         verification.terminal_class_classifications_certified &&
         verification.arm_classifications_certified &&
         verification.incident_components_certified &&
         verification.result_facts_certified &&
         verification.counters_certified &&
         verification.decision_certified &&
         verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.exact_critical_arm_gamma_decision_certified;
}

[[nodiscard]] CanonicalPointCloud bridge_fixture_cloud() {
  // Canonical ids are A=0, Q=1, B=2, C=3 and P=4.  The critical shell is
  // U={A,B,C}.  Its arms descend as BC->BP, AC->AC and AB->AQ.
  const std::array<CertifiedPoint3, 5> input{
      point(-2.0, 0.0),
      point(-2.0, 2.0),
      point(0.0, 3.0),
      point(2.0, 0.0),
      point(2.0, 2.0)};
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud shared_terminal_fixture_cloud() {
  const std::array<CertifiedPoint3, 5> input{
      point(-8.0, 1.0),
      point(-5.0, -7.0),
      point(-3.0, -8.0),
      point(4.0, 8.0),
      point(5.0, -7.0)};
  return canonical_cloud(input);
}

[[nodiscard]] ExactCriticalArmGammaResult complete_bridge_result(
    const CanonicalPointCloud& cloud) {
  const std::array<PointId, 3> shell{3U, 0U, 2U};
  return build_exact_critical_arm_gamma_component_classification(
      cloud,
      shell,
      ExactFacetDescentChainBudget{1U},
      bridge_gamma_budget());
}

void test_complete_family_is_grouped_by_strict_gamma_component() {
  const ExactCriticalArmGammaResult empty;
  check(
      empty.decision == ExactCriticalArmGammaDecision::not_certified &&
          empty.scope == ExactCriticalArmGammaScope::unspecified &&
          !empty.strict_gamma.has_value() &&
          empty.terminal_class_classifications.empty() &&
          empty.arm_classifications.empty() &&
          empty.incident_components.empty(),
      "the default bridge certifies no pre-event component classification");

  const CanonicalPointCloud cloud = bridge_fixture_cloud();
  const std::array<PointId, 3> permuted_shell{3U, 0U, 2U};
  const ExactFacetDescentChainBudget arm_budget{1U};
  const ExactStrictGammaBudget gamma_budget = bridge_gamma_budget();
  const ExactCriticalArmGammaResult result =
      build_exact_critical_arm_gamma_component_classification(
          cloud,
          permuted_shell,
          arm_budget,
          gamma_budget);
  const ExactCriticalArmGammaVerification verification =
      verify_exact_critical_arm_gamma_component_classification(
          cloud,
          permuted_shell,
          arm_budget,
          gamma_budget,
          result);

  check(
      result.critical_shell_point_ids ==
              std::vector<PointId>({0U, 2U, 3U}) &&
          result.order == 2U &&
          result.critical_squared_level == level(169, 36) &&
          result.arm_family.complete_terminal_label_partition_certified &&
          result.strict_gamma.has_value() &&
          result.strict_gamma->decision ==
              ExactStrictGammaDecision::
                  complete_all_sources_active_and_classified,
      "the bridge derives the exact order and open cut from the shared critical source");

  check(
      result.terminal_class_classifications.size() == 3U &&
          result.terminal_class_classifications[0]
                  .terminal_facet_point_ids ==
              std::vector<PointId>({0U, 1U}) &&
          result.terminal_class_classifications[0]
                  .removed_shell_point_ids ==
              std::vector<PointId>({3U}) &&
          result.terminal_class_classifications[0]
                  .strict_gamma_component_index == 0U &&
          result.terminal_class_classifications[1]
                  .terminal_facet_point_ids ==
              std::vector<PointId>({0U, 3U}) &&
          result.terminal_class_classifications[1]
                  .removed_shell_point_ids ==
              std::vector<PointId>({2U}) &&
          result.terminal_class_classifications[1]
                  .strict_gamma_component_index == 1U &&
          result.terminal_class_classifications[2]
                  .terminal_facet_point_ids ==
              std::vector<PointId>({2U, 4U}) &&
          result.terminal_class_classifications[2]
                  .removed_shell_point_ids ==
              std::vector<PointId>({0U}) &&
          result.terminal_class_classifications[2]
                  .strict_gamma_component_index == 0U,
      "distinct terminal labels are classified before any event-level merge decision");

  check(
      result.arm_classifications.size() == 3U &&
          result.arm_classifications[0].removed_shell_point_id == 0U &&
          result.arm_classifications[0].terminal_label_class_index == 2U &&
          result.arm_classifications[0].strict_gamma_component_index == 0U &&
          result.arm_classifications[1].removed_shell_point_id == 2U &&
          result.arm_classifications[1].terminal_label_class_index == 1U &&
          result.arm_classifications[1].strict_gamma_component_index == 1U &&
          result.arm_classifications[2].removed_shell_point_id == 3U &&
          result.arm_classifications[2].terminal_label_class_index == 0U &&
          result.arm_classifications[2].strict_gamma_component_index == 0U,
      "every removed shell point keeps its label-class and Gamma-component provenance");

  check(
      result.incident_components.size() == 2U &&
          result.incident_components[0].strict_gamma_component_index == 0U &&
          result.incident_components[0]
                  .canonical_representative_facet_point_ids ==
              std::vector<PointId>({0U, 1U}) &&
          result.incident_components[0].terminal_label_class_indices ==
              std::vector<std::size_t>({0U, 2U}) &&
          result.incident_components[0].removed_shell_point_ids ==
              std::vector<PointId>({0U, 3U}) &&
          result.incident_components[1].strict_gamma_component_index == 1U &&
          result.incident_components[1]
                  .canonical_representative_facet_point_ids ==
              std::vector<PointId>({0U, 3U}) &&
          result.incident_components[1].terminal_label_class_indices ==
              std::vector<std::size_t>({1U}) &&
          result.incident_components[1].removed_shell_point_ids ==
              std::vector<PointId>({2U}),
      "the Gamma partition coarsens two distinct terminal labels in one component");

  check(
      result.counters.arm_family_build_count == 1U &&
          result.counters.terminal_label_class_count == 3U &&
          result.counters.strict_gamma_build_count == 1U &&
          result.counters.strict_gamma_source_facet_count == 3U &&
          result.counters.terminal_class_component_projection_count == 3U &&
          result.counters.arm_component_projection_count == 3U &&
          result.counters.incident_component_count == 2U &&
          result.counters.same_terminal_label_arm_coalescence_count == 0U &&
          result.counters.
                  distinct_terminal_label_component_coalescence_count == 1U &&
          result.arm_family_fresh_replay_certified &&
          result.critical_order_and_level_derived_from_shared_source &&
          result.strict_gamma_cut_fresh_replay_certified &&
          result.all_terminal_classes_active_and_classified &&
          result.every_arm_projected_once &&
          result.terminal_label_partition_refines_gamma_component_partition &&
          result.decision == ExactCriticalArmGammaDecision::
                                 complete_arm_to_strict_gamma_component_classification &&
          result.scope == ExactCriticalArmGammaScope::
                              bounded_complete_critical_arm_family_to_exhaustive_strict_gamma_components_only &&
          std::string_view{ExactCriticalArmGammaResult::proof_basis} ==
              "exact_complete_critical_arm_family_strict_path_bounded_"
              "exhaustive_open_gamma_component_classification_v1" &&
          all_certificates_close(verification),
      "the complete bridge closes only its bounded pre-event component scope");
}

void test_two_arms_share_one_terminal_label_and_one_gamma_source() {
  const CanonicalPointCloud cloud = shared_terminal_fixture_cloud();
  const std::array<PointId, 3> shell{3U, 1U, 2U};
  const ExactFacetDescentChainBudget arm_budget{2U};
  const ExactStrictGammaBudget gamma_budget{10U, 5U, 15U};
  const ExactCriticalArmGammaResult result =
      build_exact_critical_arm_gamma_component_classification(
          cloud,
          shell,
          arm_budget,
          gamma_budget);
  const ExactCriticalArmGammaVerification verification =
      verify_exact_critical_arm_gamma_component_classification(
          cloud,
          shell,
          arm_budget,
          gamma_budget,
          result);

  const std::vector<PointId> shared_terminal{0U, 1U, 2U};
  const std::vector<PointId> other_terminal{0U, 1U, 3U};
  check(
      result.order == 3U &&
          result.critical_squared_level == level(25925, 338) &&
          result.arm_family.arms.size() == 3U &&
          result.arm_family.terminal_label_classes.size() == 2U &&
          result.terminal_class_classifications.size() == 2U &&
          result.arm_classifications.size() == 3U &&
          result.strict_gamma.has_value() &&
          result.strict_gamma->source_facet_point_ids.size() == 2U &&
          result.counters.strict_gamma_source_facet_count == 2U &&
          result.counters.same_terminal_label_arm_coalescence_count == 1U &&
          result.counters.incident_component_count == 2U &&
          result.counters.
                  distinct_terminal_label_component_coalescence_count == 0U &&
          all_certificates_close(verification),
      "the shared-terminal fixture closes three arms with only two Gamma sources");

  check(
      result.terminal_class_classifications[0]
              .terminal_facet_point_ids == shared_terminal &&
          result.terminal_class_classifications[0]
              .removed_shell_point_ids ==
              std::vector<PointId>({1U, 3U}) &&
          result.terminal_class_classifications[1]
              .terminal_facet_point_ids == other_terminal &&
          result.terminal_class_classifications[1]
              .removed_shell_point_ids ==
              std::vector<PointId>({2U}) &&
          result.arm_family.terminal_label_classes[0]
                  .canonical_terminal.squared_level ==
              level(53, 2) &&
          result.arm_family.terminal_label_classes[1]
                  .canonical_terminal.squared_level ==
              level(153, 2),
      "the common terminal keeps both removed-shell provenances and exact geometry");

  check(
      result.arm_classifications[0].removed_shell_point_id == 1U &&
          result.arm_classifications[0].terminal_label_class_index == 0U &&
          result.arm_classifications[1].removed_shell_point_id == 2U &&
          result.arm_classifications[1].terminal_label_class_index == 1U &&
          result.arm_classifications[2].removed_shell_point_id == 3U &&
          result.arm_classifications[2].terminal_label_class_index == 0U &&
          result.arm_classifications[0].strict_gamma_component_index ==
              result.arm_classifications[2]
                  .strict_gamma_component_index,
      "both coalesced arms project through the same label class and Gamma component");
}

void test_incomplete_family_and_gamma_budget_fail_closed() {
  const CanonicalPointCloud cloud = bridge_fixture_cloud();
  const std::array<PointId, 3> shell{0U, 2U, 3U};
  const ExactStrictGammaBudget full_budget = bridge_gamma_budget();

  const ExactFacetDescentChainBudget zero_arm_budget{0U};
  const ExactCriticalArmGammaResult incomplete =
      build_exact_critical_arm_gamma_component_classification(
          cloud,
          shell,
          zero_arm_budget,
          full_budget);
  const ExactCriticalArmGammaVerification incomplete_verification =
      verify_exact_critical_arm_gamma_component_classification(
          cloud,
          shell,
          zero_arm_budget,
          full_budget,
          incomplete);
  check(
      incomplete.decision == ExactCriticalArmGammaDecision::
                                 no_classification_incomplete_arm_family &&
          !incomplete.strict_gamma.has_value() &&
          incomplete.counters.strict_gamma_build_count == 0U &&
          incomplete.terminal_class_classifications.empty() &&
          incomplete.arm_classifications.empty() &&
          incomplete.incident_components.empty() &&
          !incomplete.critical_order_and_level_derived_from_shared_source &&
          all_certificates_close(incomplete_verification),
      "an incomplete arm family never starts or publishes a Gamma classification");

  const ExactStrictGammaBudget insufficient_gamma_budget{9U, 10U, 20U};
  const ExactFacetDescentChainBudget one_arm_budget{1U};
  const ExactCriticalArmGammaResult no_gamma =
      build_exact_critical_arm_gamma_component_classification(
          cloud,
          shell,
          one_arm_budget,
          insufficient_gamma_budget);
  const ExactCriticalArmGammaVerification no_gamma_verification =
      verify_exact_critical_arm_gamma_component_classification(
          cloud,
          shell,
          one_arm_budget,
          insufficient_gamma_budget,
          no_gamma);
  check(
      no_gamma.decision == ExactCriticalArmGammaDecision::
                               no_classification_strict_gamma_preflight_budget_insufficient &&
          no_gamma.strict_gamma.has_value() &&
          no_gamma.strict_gamma->decision ==
              ExactStrictGammaDecision::
                  no_cut_preflight_budget_insufficient &&
          no_gamma.strict_gamma->required_facet_count == 10U &&
          no_gamma.strict_gamma->required_coface_count == 10U &&
          no_gamma.strict_gamma->required_union_attempt_count == 20U &&
          no_gamma.strict_gamma->active_facets.empty() &&
          no_gamma.strict_gamma->active_cofaces.empty() &&
          no_gamma.strict_gamma->components.empty() &&
          no_gamma.strict_gamma->source_classifications.empty() &&
          no_gamma.strict_gamma->counters.required_facet_count == 10U &&
          no_gamma.strict_gamma->counters.required_coface_count == 10U &&
          no_gamma.strict_gamma->counters.required_union_attempt_count ==
              20U &&
          no_gamma.strict_gamma->counters.enumerated_facet_count == 0U &&
          no_gamma.strict_gamma->counters.facet_miniball_build_count == 0U &&
          no_gamma.strict_gamma->counters.enumerated_coface_count == 0U &&
          no_gamma.strict_gamma->counters.union_attempt_count == 0U &&
          no_gamma.strict_gamma->counters.source_lookup_count == 0U &&
          no_gamma.counters.strict_gamma_build_count == 1U &&
          no_gamma.counters.strict_gamma_source_facet_count == 3U &&
          no_gamma.counters.terminal_class_component_projection_count == 0U &&
          no_gamma.terminal_class_classifications.empty() &&
          no_gamma.arm_classifications.empty() &&
          no_gamma.incident_components.empty() &&
          !no_gamma.strict_gamma_cut_fresh_replay_certified &&
          all_certificates_close(no_gamma_verification),
      "an insufficient all-or-nothing Gamma budget returns no partial component projection");
}

void test_unsupported_source_never_enters_gamma() {
  const std::array<CertifiedPoint3, 4> input{
      point(-1.0, 0.0),
      point(0.0, -1.0),
      point(0.0, 1.0),
      point(1.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 4> nonminimal_shell{0U, 1U, 2U, 3U};
  const ExactFacetDescentChainBudget arm_budget{0U};
  const ExactStrictGammaBudget gamma_budget = bridge_gamma_budget();
  const ExactCriticalArmGammaResult result =
      build_exact_critical_arm_gamma_component_classification(
          cloud,
          nonminimal_shell,
          arm_budget,
          gamma_budget);
  const ExactCriticalArmGammaVerification verification =
      verify_exact_critical_arm_gamma_component_classification(
          cloud,
          nonminimal_shell,
          arm_budget,
          gamma_budget,
          result);

  check(
      result.decision == ExactCriticalArmGammaDecision::
                             no_classification_unsupported_critical_source &&
          !result.strict_gamma.has_value() &&
          result.terminal_class_classifications.empty() &&
          result.arm_classifications.empty() &&
          result.incident_components.empty() &&
          result.counters.strict_gamma_build_count == 0U &&
          all_certificates_close(verification),
      "a nonminimal critical shell remains an exact unsupported classification without Gamma work");
}

void test_invalid_inputs_are_rejected() {
  const CanonicalPointCloud cloud = bridge_fixture_cloud();
  const std::array<PointId, 3> shell{0U, 2U, 3U};
  const ExactFacetDescentChainBudget arm_budget{1U};
  const ExactStrictGammaBudget gamma_budget = maximum_gamma_budget();

  ExactStrictGammaBudget oversized_gamma_budget = gamma_budget;
  ++oversized_gamma_budget.maximum_union_attempt_count;
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(
            build_exact_critical_arm_gamma_component_classification(
                cloud,
                shell,
                arm_budget,
                oversized_gamma_budget));
      },
      "a Gamma budget above the bounded reference cap is rejected before the arm family");

  const ExactFacetDescentChainBudget oversized_arm_budget{
      ExactFacetDescentChainBudget::
          maximum_supported_committed_strict_segment_count + 1U};
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(
            build_exact_critical_arm_gamma_component_classification(
                cloud,
                shell,
                oversized_arm_budget,
                gamma_budget));
      },
      "an arm-chain budget above its cap is rejected");

  std::vector<CertifiedPoint3> large_input;
  large_input.reserve(15U);
  for (std::size_t index = 0U; index < 15U; ++index) {
    large_input.push_back(point(static_cast<double>(index)));
  }
  const CanonicalPointCloud large_cloud =
      CanonicalPointCloud::rejecting_duplicates(
          std::span<const CertifiedPoint3>{large_input});
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(
            build_exact_critical_arm_gamma_component_classification(
                large_cloud,
                shell,
                arm_budget,
                gamma_budget));
      },
      "the event-to-Gamma bridge rejects clouds outside its n<=14 scope");
}

void test_verifier_rejects_every_composed_layer() {
  const CanonicalPointCloud cloud = bridge_fixture_cloud();
  const std::array<PointId, 3> shell{0U, 2U, 3U};
  const ExactFacetDescentChainBudget arm_budget{1U};
  const ExactStrictGammaBudget gamma_budget = bridge_gamma_budget();
  const ExactCriticalArmGammaResult result = complete_bridge_result(cloud);

  const auto rejects =
      [&cloud, &shell, arm_budget, gamma_budget](
          const ExactCriticalArmGammaResult& candidate,
          const std::string& message) {
        const ExactCriticalArmGammaVerification verification =
            verify_exact_critical_arm_gamma_component_classification(
                cloud,
                shell,
                arm_budget,
                gamma_budget,
                candidate);
        check(
            !verification.exact_critical_arm_gamma_decision_certified,
            message);
      };

  ExactCriticalArmGammaResult bad_arm_budget = result;
  bad_arm_budget.requested_per_arm_chain_budget =
      ExactFacetDescentChainBudget{0U};
  rejects(bad_arm_budget, "the verifier rejects a falsified stored arm budget");

  ExactCriticalArmGammaResult bad_gamma_budget = result;
  --bad_gamma_budget.requested_strict_gamma_budget.
      maximum_union_attempt_count;
  rejects(
      bad_gamma_budget,
      "the verifier rejects a falsified stored strict-Gamma budget");

  ExactCriticalArmGammaResult bad_family = result;
  bad_family.arm_family.arms.front().terminal_label_class_index = 0U;
  rejects(bad_family, "the verifier freshly replays the embedded 6.7 family");

  ExactCriticalArmGammaResult bad_level = result;
  bad_level.critical_squared_level = level(4);
  rejects(bad_level, "the verifier derives the critical level independently");

  ExactCriticalArmGammaResult bad_order = result;
  ++bad_order.order;
  rejects(bad_order, "the verifier derives the critical order independently");

  ExactCriticalArmGammaResult bad_shell = result;
  bad_shell.critical_shell_point_ids.front() = 1U;
  rejects(bad_shell, "the verifier rejects a falsified stored critical shell");

  ExactCriticalArmGammaResult missing_gamma = result;
  missing_gamma.strict_gamma.reset();
  rejects(missing_gamma, "the verifier rejects an absent complete Gamma cut");

  ExactCriticalArmGammaResult bad_gamma = result;
  bad_gamma.strict_gamma->source_classifications.front()
      .component_index = 1U;
  rejects(bad_gamma, "the verifier freshly replays the embedded 6.8 cut");

  ExactCriticalArmGammaResult bad_terminal_mapping = result;
  ++bad_terminal_mapping.terminal_class_classifications.front()
        .strict_gamma_component_index;
  rejects(
      bad_terminal_mapping,
      "the verifier rejects a falsified terminal-class projection");

  ExactCriticalArmGammaResult bad_arm_mapping = result;
  ++bad_arm_mapping.arm_classifications.front()
        .strict_gamma_component_index;
  rejects(
      bad_arm_mapping,
      "the verifier rejects a falsified arm-to-component projection");

  ExactCriticalArmGammaResult bad_arm_class = result;
  ++bad_arm_class.arm_classifications.front()
        .terminal_label_class_index;
  rejects(
      bad_arm_class,
      "the verifier rejects a falsified arm-to-terminal-class projection");

  ExactCriticalArmGammaResult bad_component = result;
  bad_component.incident_components.front()
      .removed_shell_point_ids.pop_back();
  rejects(
      bad_component,
      "the verifier rejects a falsified incident-component provenance");

  ExactCriticalArmGammaResult bad_fact = result;
  bad_fact.every_arm_projected_once = false;
  rejects(bad_fact, "the verifier rejects a falsified result fact");

  ExactCriticalArmGammaResult bad_counter = result;
  ++bad_counter.counters.arm_component_projection_count;
  rejects(bad_counter, "the verifier rejects a falsified composed counter");

  ExactCriticalArmGammaResult bad_decision = result;
  bad_decision.decision = ExactCriticalArmGammaDecision::
      no_classification_incomplete_arm_family;
  rejects(bad_decision, "the verifier treats the bridge decision as untrusted");

  ExactCriticalArmGammaResult bad_scope = result;
  bad_scope.scope = ExactCriticalArmGammaScope::unspecified;
  rejects(bad_scope, "the verifier treats the bounded bridge scope as untrusted");

  const ExactCriticalArmGammaVerification wrong_budget_verification =
      verify_exact_critical_arm_gamma_component_classification(
          cloud,
          shell,
          ExactFacetDescentChainBudget{0U},
          gamma_budget,
          result);
  check(
      !wrong_budget_verification.
          requested_per_arm_chain_budget_certified &&
          !wrong_budget_verification.
              exact_critical_arm_gamma_decision_certified,
      "the verifier never accepts a result under another trusted arm budget");

  const ExactCriticalArmGammaVerification wrong_gamma_budget_verification =
      verify_exact_critical_arm_gamma_component_classification(
          cloud,
          shell,
          arm_budget,
          ExactStrictGammaBudget{10U, 10U, 19U},
          result);
  check(
      !wrong_gamma_budget_verification.
          requested_strict_gamma_budget_certified &&
          !wrong_gamma_budget_verification.
              exact_critical_arm_gamma_decision_certified,
      "the verifier never accepts a result under another trusted Gamma budget");

  const std::array<PointId, 3> wrong_shell{0U, 2U, 4U};
  const ExactCriticalArmGammaVerification wrong_shell_verification =
      verify_exact_critical_arm_gamma_component_classification(
          cloud,
          wrong_shell,
          arm_budget,
          gamma_budget,
          result);
  check(
      !wrong_shell_verification.input_shell_identity_certified &&
          !wrong_shell_verification.
              exact_critical_arm_gamma_decision_certified,
      "the verifier never accepts a result under another trusted critical shell");

  const std::array<CertifiedPoint3, 5> twin_input{
      point(-2.0, 0.0),
      point(-2.0, 2.0),
      point(0.0, 3.0),
      point(2.0, 0.0),
      point(2.0, 3.0)};
  const CanonicalPointCloud twin_cloud = canonical_cloud(twin_input);
  const ExactCriticalArmGammaVerification wrong_cloud_verification =
      verify_exact_critical_arm_gamma_component_classification(
          twin_cloud,
          shell,
          arm_budget,
          gamma_budget,
          result);
  check(
      !wrong_cloud_verification.exact_critical_arm_gamma_decision_certified,
      "the verifier freshly replays the bridge against the trusted cloud");
}

}  // namespace

int main() {
  test_complete_family_is_grouped_by_strict_gamma_component();
  test_two_arms_share_one_terminal_label_and_one_gamma_source();
  test_incomplete_family_and_gamma_budget_fail_closed();
  test_unsupported_source_never_enters_gamma();
  test_invalid_inputs_are_rejected();
  test_verifier_rejects_every_composed_layer();

  if (failures != 0) {
    std::cerr << failures << " test assertion(s) failed\n";
    return 1;
  }
  std::cout << "all exact critical-arm Gamma bridge tests passed\n";
  return 0;
}
