#include "morsehgp3d/hierarchy/critical_catalog_arm_gamma_overlay.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactCriticalArmFamilyDecision;
using morsehgp3d::hierarchy::ExactCriticalCatalogArmGammaArmTarget;
using morsehgp3d::hierarchy::ExactCriticalCatalogArmGammaOverlayBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogArmGammaOverlayDecision;
using morsehgp3d::hierarchy::ExactCriticalCatalogArmGammaOverlayResult;
using morsehgp3d::hierarchy::ExactCriticalCatalogArmGammaOverlayScope;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogArmGammaOverlayVerification;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogArmGammaSaddleFamilyRecord;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogArmGammaTargetComponent;
using morsehgp3d::hierarchy::ExactCriticalCatalogBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogDecision;
using morsehgp3d::hierarchy::ExactFacetDescentChainBudget;
using morsehgp3d::hierarchy::ExactReducedGammaBatchGroupKind;
using morsehgp3d::hierarchy::ExactReducedGammaStrictComponentKind;
using morsehgp3d::hierarchy::ExactStrictGammaBudget;
using morsehgp3d::hierarchy::build_exact_critical_catalog_arm_gamma_overlay;
using morsehgp3d::hierarchy::verify_exact_critical_catalog_arm_gamma_overlay;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

using PointLabel = std::vector<PointId>;

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
    double x,
    double y = 0.0,
    double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] ExactCriticalCatalogBudget full_catalog_budget() {
  return {
      ExactCriticalCatalogBudget::maximum_supported_candidate_count,
      ExactCriticalCatalogBudget::
          maximum_supported_point_classification_count};
}

[[nodiscard]] ExactStrictGammaBudget full_gamma_budget() {
  return {
      ExactStrictGammaBudget::maximum_supported_facet_count,
      ExactStrictGammaBudget::maximum_supported_coface_count,
      ExactStrictGammaBudget::maximum_supported_union_attempt_count};
}

[[nodiscard]] ExactCriticalCatalogArmGammaOverlayBudget full_budget(
    std::size_t per_arm_chain_capacity = 2U,
    ExactStrictGammaBudget gamma_budget = full_gamma_budget()) {
  ExactCriticalCatalogArmGammaOverlayBudget budget;
  budget.critical_catalog_budget = full_catalog_budget();
  budget.per_arm_chain_budget = {per_arm_chain_capacity};
  budget.reduced_gamma_batch_budget = gamma_budget;
  budget.maximum_saddle_event_count =
      ExactCriticalCatalogArmGammaOverlayBudget::
          maximum_supported_saddle_event_count;
  budget.maximum_arm_count =
      ExactCriticalCatalogArmGammaOverlayBudget::maximum_supported_arm_count;
  budget.maximum_saddle_batch_count =
      ExactCriticalCatalogArmGammaOverlayBudget::
          maximum_supported_saddle_batch_count;
  budget.maximum_target_component_count =
      ExactCriticalCatalogArmGammaOverlayBudget::
          maximum_supported_target_component_count;
  budget.maximum_target_component_facet_reference_count =
      ExactCriticalCatalogArmGammaOverlayBudget::
          maximum_supported_target_component_facet_reference_count;
  budget.maximum_target_component_point_id_reference_count =
      ExactCriticalCatalogArmGammaOverlayBudget::
          maximum_supported_target_component_point_id_reference_count;
  budget.maximum_committed_chain_segment_count =
      ExactCriticalCatalogArmGammaOverlayBudget::
          maximum_supported_committed_chain_segment_count;
  return budget;
}

[[nodiscard]] CanonicalPointCloud q2_triangle_cloud() {
  const std::array<CertifiedPoint3, 3> input{
      point(-2.0, 0.0), point(0.0, 1.0), point(2.0, 0.0)};
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud mirror_cloud() {
  const std::array<CertifiedPoint3, 4> input{
      point(-2.0, 0.0),
      point(0.0, -3.0),
      point(0.0, 3.0),
      point(2.0, 0.0)};
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud shared_terminal_cloud() {
  const std::array<CertifiedPoint3, 5> input{
      point(-8.0, 1.0),
      point(-5.0, -7.0),
      point(-3.0, -8.0),
      point(4.0, 8.0),
      point(5.0, -7.0)};
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud critical_arm_cloud() {
  const std::array<CertifiedPoint3, 4> input{
      point(-2.0, 0.0),
      point(0.0, 3.0),
      point(2.0, 0.0),
      point(2.25, 2.0)};
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud right_triangle_cloud() {
  const std::array<CertifiedPoint3, 3> input{
      point(0.0, 0.0), point(2.0, 0.0), point(0.0, 2.0)};
  return canonical_cloud(input);
}

[[nodiscard]] bool all_certificates_close(
    const ExactCriticalCatalogArmGammaOverlayVerification& verification) {
  return verification.requested_budget_certified &&
         verification.external_inputs_certified &&
         verification.derived_preflight_sizes_certified &&
         verification.critical_catalog_certified &&
         verification.saddle_family_records_certified &&
         verification.batch_records_certified &&
         verification.target_components_certified &&
         verification.arm_targets_certified &&
         verification.result_facts_certified &&
         verification.counters_certified && verification.decision_certified &&
         verification.scope_certified && verification.fresh_replay_certified &&
         verification.
             exact_critical_catalog_arm_gamma_overlay_decision_certified;
}

[[nodiscard]] bool complete_facts(
    const ExactCriticalCatalogArmGammaOverlayResult& result) {
  return result.overlay_candidate_space_size_certified &&
         result.overlay_preflight_budget_sufficient &&
         result.
             subordinate_geometry_started_only_after_successful_overlay_preflight &&
         result.critical_catalog_fresh_replay_certified &&
         result.no_relevant_extra_shell_degeneracy &&
         result.requested_order_saddles_exhaustively_selected &&
         result.critical_arm_families_fresh_replay_certified &&
         result.catalog_sources_match_all_arm_families &&
         result.all_critical_arm_families_complete &&
         result.reduced_gamma_batches_fresh_replay_certified &&
         result.one_reduced_gamma_batch_per_saddle_h0_batch &&
         result.every_saddle_coface_in_unique_non_deferred_group &&
         result.
             every_arm_initial_and_terminal_in_same_unique_group_strict_component &&
         result.target_components_deduplicated_by_batch_and_strict_component &&
         result.target_components_retain_full_pi0_witnesses &&
         result.reduced_component_kinds_copied_separately &&
         result.reduced_group_kinds_inherited_without_morse_inference &&
         result.every_catalog_saddle_arm_has_one_target &&
         result.diagnostic_outcomes_have_no_gamma_targets &&
         result.critical_catalog_arm_gamma_overlay_certified &&
         result.decision ==
             ExactCriticalCatalogArmGammaOverlayDecision::
                 complete_exhaustive_catalog_saddle_arm_gamma_overlay &&
         result.scope ==
             ExactCriticalCatalogArmGammaOverlayScope::
                 bounded_n14_k10_single_order_fresh_catalog_all_index_one_saddle_arm_families_to_exhaustive_strict_gamma_full_pi0_components_with_reduced_annotations_only;
}

void check_empty_gamma_target_payload(
    const ExactCriticalCatalogArmGammaOverlayResult& result,
    const std::string& label) {
  check(
      result.batch_records.empty() && result.target_components.empty() &&
          result.arm_targets.empty(),
      label + ": no partial Gamma target payload is published");
}

[[nodiscard]] std::optional<std::size_t> event_index_with_closed_label(
    const ExactCriticalCatalogArmGammaOverlayResult& result,
    const PointLabel& closed_label) {
  if (!result.critical_catalog.has_value()) {
    return std::nullopt;
  }
  const auto iterator = std::find_if(
      result.critical_catalog->events.begin(),
      result.critical_catalog->events.end(),
      [&](const auto& event) { return event.closed_point_ids == closed_label; });
  if (iterator == result.critical_catalog->events.end()) {
    return std::nullopt;
  }
  return iterator->event_index;
}

[[nodiscard]] const ExactCriticalCatalogArmGammaSaddleFamilyRecord*
family_with_closed_label(
    const ExactCriticalCatalogArmGammaOverlayResult& result,
    const PointLabel& closed_label) {
  const std::optional<std::size_t> event_index =
      event_index_with_closed_label(result, closed_label);
  if (!event_index.has_value()) {
    return nullptr;
  }
  const auto iterator = std::find_if(
      result.saddle_family_records.begin(),
      result.saddle_family_records.end(),
      [&](const ExactCriticalCatalogArmGammaSaddleFamilyRecord& record) {
        return record.catalog_event_index == *event_index;
      });
  return iterator == result.saddle_family_records.end() ? nullptr : &*iterator;
}

[[nodiscard]] const ExactCriticalCatalogArmGammaArmTarget*
arm_target_with_closed_label(
    const ExactCriticalCatalogArmGammaOverlayResult& result,
    const PointLabel& closed_label,
    PointId removed_shell_point_id) {
  const std::optional<std::size_t> event_index =
      event_index_with_closed_label(result, closed_label);
  if (!event_index.has_value()) {
    return nullptr;
  }
  const auto iterator = std::find_if(
      result.arm_targets.begin(),
      result.arm_targets.end(),
      [&](const ExactCriticalCatalogArmGammaArmTarget& target) {
        return target.catalog_event_index == *event_index &&
               target.removed_shell_point_id == removed_shell_point_id;
      });
  return iterator == result.arm_targets.end() ? nullptr : &*iterator;
}

[[nodiscard]] const ExactCriticalCatalogArmGammaTargetComponent*
component_for_arm_target(
    const ExactCriticalCatalogArmGammaOverlayResult& result,
    const ExactCriticalCatalogArmGammaArmTarget* arm_target) {
  if (arm_target == nullptr ||
      arm_target->target_component_index >= result.target_components.size()) {
    return nullptr;
  }
  return &result.target_components[arm_target->target_component_index];
}

void test_default_invalid_domain_and_caps() {
  const ExactCriticalCatalogArmGammaOverlayResult empty;
  check(
      empty.decision ==
              ExactCriticalCatalogArmGammaOverlayDecision::not_certified &&
          empty.scope == ExactCriticalCatalogArmGammaOverlayScope::unspecified &&
          !empty.critical_catalog.has_value() &&
          empty.saddle_family_records.empty() && empty.batch_records.empty() &&
          empty.target_components.empty() && empty.arm_targets.empty() &&
          !empty.critical_catalog_arm_gamma_overlay_certified,
      "the default catalogue-arm-Gamma overlay certifies no attachment");
  check(
      std::string_view{ExactCriticalCatalogArmGammaOverlayResult::proof_basis} ==
          "exact_exhaustive_critical_catalog_index_one_arm_families_"
          "reconciled_with_strict_gamma_full_pi0_components_and_separate_"
          "reduced_annotations_v1",
      "the composite proof basis remains local and exact");

  const CanonicalPointCloud cloud = q2_triangle_cloud();
  const ExactCriticalCatalogArmGammaOverlayBudget budget = full_budget(1U);
  const std::array<CertifiedPoint3, 2> two_points{point(0.0), point(2.0)};
  const CanonicalPointCloud two_point_cloud = canonical_cloud(two_points);
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_critical_catalog_arm_gamma_overlay(
            two_point_cloud, 2U, budget));
      },
      "the composite overlay rejects n below three");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_critical_catalog_arm_gamma_overlay(
            cloud, 1U, budget));
      },
      "the composite overlay rejects order one");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_critical_catalog_arm_gamma_overlay(
            cloud, 3U, budget));
      },
      "the composite overlay rejects the terminal order");

  const std::array<CertifiedPoint3, 12> twelve_points{
      point(0.0),  point(1.0),  point(2.0),  point(3.0),
      point(4.0),  point(5.0),  point(6.0),  point(7.0),
      point(8.0),  point(9.0),  point(10.0), point(11.0)};
  const CanonicalPointCloud twelve_point_cloud =
      canonical_cloud(twelve_points);
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_critical_catalog_arm_gamma_overlay(
            twelve_point_cloud, 11U, budget));
      },
      "the composite overlay rejects orders above ten before geometry");

  using BudgetMember =
      std::size_t ExactCriticalCatalogArmGammaOverlayBudget::*;
  struct CapCase {
    const char* name;
    BudgetMember member;
    std::size_t cap;
  };
  const std::array<CapCase, 7> cap_cases{{
      {"saddle event",
       &ExactCriticalCatalogArmGammaOverlayBudget::maximum_saddle_event_count,
       ExactCriticalCatalogArmGammaOverlayBudget::
           maximum_supported_saddle_event_count},
      {"arm",
       &ExactCriticalCatalogArmGammaOverlayBudget::maximum_arm_count,
       ExactCriticalCatalogArmGammaOverlayBudget::maximum_supported_arm_count},
      {"saddle batch",
       &ExactCriticalCatalogArmGammaOverlayBudget::maximum_saddle_batch_count,
       ExactCriticalCatalogArmGammaOverlayBudget::
           maximum_supported_saddle_batch_count},
      {"target component",
       &ExactCriticalCatalogArmGammaOverlayBudget::
           maximum_target_component_count,
       ExactCriticalCatalogArmGammaOverlayBudget::
           maximum_supported_target_component_count},
      {"target component facet reference",
       &ExactCriticalCatalogArmGammaOverlayBudget::
           maximum_target_component_facet_reference_count,
       ExactCriticalCatalogArmGammaOverlayBudget::
           maximum_supported_target_component_facet_reference_count},
      {"target component PointId reference",
       &ExactCriticalCatalogArmGammaOverlayBudget::
           maximum_target_component_point_id_reference_count,
       ExactCriticalCatalogArmGammaOverlayBudget::
           maximum_supported_target_component_point_id_reference_count},
      {"committed chain segment",
       &ExactCriticalCatalogArmGammaOverlayBudget::
           maximum_committed_chain_segment_count,
       ExactCriticalCatalogArmGammaOverlayBudget::
           maximum_supported_committed_chain_segment_count},
  }};
  for (const CapCase& cap_case : cap_cases) {
    ExactCriticalCatalogArmGammaOverlayBudget invalid = budget;
    invalid.*(cap_case.member) = cap_case.cap + 1U;
    check_throws<std::invalid_argument>(
        [&] {
          static_cast<void>(build_exact_critical_catalog_arm_gamma_overlay(
              cloud, 2U, invalid));
        },
        std::string{"the composite overlay rejects an oversized "} +
            cap_case.name + " cap");
  }

  ExactCriticalCatalogArmGammaOverlayBudget invalid_catalog = budget;
  invalid_catalog.critical_catalog_budget.maximum_candidate_count =
      ExactCriticalCatalogBudget::maximum_supported_candidate_count + 1U;
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_critical_catalog_arm_gamma_overlay(
            cloud, 2U, invalid_catalog));
      },
      "the composite overlay rejects an oversized catalogue cap");

  ExactCriticalCatalogArmGammaOverlayBudget invalid_chain = budget;
  invalid_chain.per_arm_chain_budget.maximum_committed_strict_segment_count =
      ExactFacetDescentChainBudget::
          maximum_supported_committed_strict_segment_count +
      1U;
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_critical_catalog_arm_gamma_overlay(
            cloud, 2U, invalid_chain));
      },
      "the composite overlay rejects an oversized per-arm chain cap");

  ExactCriticalCatalogArmGammaOverlayBudget invalid_gamma = budget;
  invalid_gamma.reduced_gamma_batch_budget.maximum_enumerated_facet_count =
      ExactStrictGammaBudget::maximum_supported_facet_count + 1U;
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_critical_catalog_arm_gamma_overlay(
            cloud, 2U, invalid_gamma));
      },
      "the composite overlay rejects an oversized reduced-Gamma cap");
}

void test_overlay_preflight_is_atomic_in_every_dimension() {
  const CanonicalPointCloud cloud = q2_triangle_cloud();
  const ExactCriticalCatalogArmGammaOverlayBudget budget =
      full_budget(1U, ExactStrictGammaBudget{3U, 1U, 2U});
  const ExactCriticalCatalogArmGammaOverlayResult successful =
      build_exact_critical_catalog_arm_gamma_overlay(cloud, 2U, budget);
  check(
      successful.required_saddle_event_capacity == 4U &&
          successful.required_arm_capacity == 16U &&
          successful.required_saddle_batch_capacity == 4U &&
          successful.required_target_component_capacity == 16U &&
          successful.required_target_component_facet_reference_capacity ==
              12U &&
          successful.required_target_component_point_id_reference_capacity ==
              56U &&
          successful.required_committed_chain_segment_capacity == 16U,
      "the q2 preflight derives the seven closed combinatorial bounds");

  using BudgetMember =
      std::size_t ExactCriticalCatalogArmGammaOverlayBudget::*;
  using ResultMember =
      std::size_t ExactCriticalCatalogArmGammaOverlayResult::*;
  struct Dimension {
    const char* name;
    BudgetMember maximum;
    ResultMember required;
  };
  const std::array<Dimension, 7> dimensions{{
      {"saddle events",
       &ExactCriticalCatalogArmGammaOverlayBudget::maximum_saddle_event_count,
       &ExactCriticalCatalogArmGammaOverlayResult::
           required_saddle_event_capacity},
      {"arms",
       &ExactCriticalCatalogArmGammaOverlayBudget::maximum_arm_count,
       &ExactCriticalCatalogArmGammaOverlayResult::required_arm_capacity},
      {"saddle batches",
       &ExactCriticalCatalogArmGammaOverlayBudget::maximum_saddle_batch_count,
       &ExactCriticalCatalogArmGammaOverlayResult::
           required_saddle_batch_capacity},
      {"target components",
       &ExactCriticalCatalogArmGammaOverlayBudget::
           maximum_target_component_count,
       &ExactCriticalCatalogArmGammaOverlayResult::
           required_target_component_capacity},
      {"target component facet references",
       &ExactCriticalCatalogArmGammaOverlayBudget::
           maximum_target_component_facet_reference_count,
       &ExactCriticalCatalogArmGammaOverlayResult::
           required_target_component_facet_reference_capacity},
      {"target component PointId references",
       &ExactCriticalCatalogArmGammaOverlayBudget::
           maximum_target_component_point_id_reference_count,
       &ExactCriticalCatalogArmGammaOverlayResult::
           required_target_component_point_id_reference_capacity},
      {"committed chain segments",
       &ExactCriticalCatalogArmGammaOverlayBudget::
           maximum_committed_chain_segment_count,
       &ExactCriticalCatalogArmGammaOverlayResult::
           required_committed_chain_segment_capacity},
  }};
  for (const Dimension& dimension : dimensions) {
    ExactCriticalCatalogArmGammaOverlayBudget insufficient = budget;
    insufficient.*(dimension.maximum) = successful.*(dimension.required) - 1U;
    const ExactCriticalCatalogArmGammaOverlayResult result =
        build_exact_critical_catalog_arm_gamma_overlay(
            cloud, 2U, insufficient);
    const ExactCriticalCatalogArmGammaOverlayVerification verification =
        verify_exact_critical_catalog_arm_gamma_overlay(
            cloud, 2U, insufficient, result);
    check(
        result.overlay_candidate_space_size_certified &&
            !result.overlay_preflight_budget_sufficient &&
            result.
                subordinate_geometry_started_only_after_successful_overlay_preflight &&
            result.decision ==
                ExactCriticalCatalogArmGammaOverlayDecision::
                    no_overlay_preflight_budget_insufficient &&
            result.counters.preflight_count == 1U &&
            result.counters.critical_catalog_build_count == 0U &&
            result.counters.critical_arm_family_build_count == 0U &&
            result.counters.reduced_gamma_batch_build_count == 0U &&
            !result.critical_catalog.has_value() &&
            result.saddle_family_records.empty() &&
            all_certificates_close(verification),
        std::string{dimension.name} +
            " fails atomically before subordinate geometry");
    check_empty_gamma_target_payload(result, dimension.name);
  }
}

void test_subordinate_budget_failures() {
  const CanonicalPointCloud cloud = q2_triangle_cloud();

  ExactCriticalCatalogArmGammaOverlayBudget catalog_insufficient =
      full_budget(1U, ExactStrictGammaBudget{3U, 1U, 2U});
  catalog_insufficient.critical_catalog_budget.maximum_candidate_count = 6U;
  const ExactCriticalCatalogArmGammaOverlayResult no_catalog =
      build_exact_critical_catalog_arm_gamma_overlay(
          cloud, 2U, catalog_insufficient);
  check(
      no_catalog.overlay_preflight_budget_sufficient &&
          no_catalog.critical_catalog.has_value() &&
          no_catalog.critical_catalog->decision ==
              ExactCriticalCatalogDecision::
                  no_catalog_preflight_budget_insufficient &&
          no_catalog.saddle_family_records.empty() &&
          no_catalog.decision ==
              ExactCriticalCatalogArmGammaOverlayDecision::
                  no_catalog_preflight_budget_insufficient,
      "an insufficient catalogue budget retains only its atomic diagnostic");
  check_empty_gamma_target_payload(no_catalog, "catalogue budget failure");
  check(
      all_certificates_close(verify_exact_critical_catalog_arm_gamma_overlay(
          cloud, 2U, catalog_insufficient, no_catalog)),
      "the fresh verifier certifies the catalogue-budget decision");

  ExactCriticalCatalogArmGammaOverlayBudget gamma_insufficient =
      full_budget(1U, ExactStrictGammaBudget{2U, 1U, 2U});
  const ExactCriticalCatalogArmGammaOverlayResult no_gamma =
      build_exact_critical_catalog_arm_gamma_overlay(
          cloud, 2U, gamma_insufficient);
  check(
      no_gamma.critical_catalog.has_value() &&
          no_gamma.critical_catalog_fresh_replay_certified &&
          !no_gamma.saddle_family_records.empty() &&
          no_gamma.counters.complete_critical_arm_family_count == 1U &&
          no_gamma.counters.insufficient_reduced_gamma_batch_count == 1U &&
          no_gamma.decision ==
              ExactCriticalCatalogArmGammaOverlayDecision::
                  no_overlay_reduced_gamma_batch_preflight_budget_insufficient,
      "an insufficient Gamma budget follows a complete family but publishes no target");
  check_empty_gamma_target_payload(no_gamma, "Gamma budget failure");
  check(
      all_certificates_close(verify_exact_critical_catalog_arm_gamma_overlay(
          cloud, 2U, gamma_insufficient, no_gamma)),
      "the fresh verifier certifies the Gamma-budget decision");
}

void test_relevant_extra_shell_blocks_before_arm_families() {
  const CanonicalPointCloud cloud = right_triangle_cloud();
  const ExactCriticalCatalogArmGammaOverlayBudget budget =
      full_budget(1U, ExactStrictGammaBudget{3U, 1U, 2U});
  const ExactCriticalCatalogArmGammaOverlayResult result =
      build_exact_critical_catalog_arm_gamma_overlay(cloud, 2U, budget);

  check(
      result.critical_catalog.has_value() &&
          result.critical_catalog->decision ==
              ExactCriticalCatalogDecision::
                  complete_catalog_with_relevant_extra_shell_degeneracy &&
          result.critical_catalog->extra_shell_degeneracies.size() == 1U &&
          !result.no_relevant_extra_shell_degeneracy &&
          result.counters.critical_arm_family_build_count == 0U &&
          result.counters.reduced_gamma_batch_build_count == 0U &&
          result.saddle_family_records.empty() &&
          result.decision ==
              ExactCriticalCatalogArmGammaOverlayDecision::
                  no_overlay_relevant_extra_shell_degeneracy,
      "the right-triangle extra shell blocks every arm and Gamma build");
  check_empty_gamma_target_payload(result, "relevant extra-shell degeneracy");
  check(
      all_certificates_close(verify_exact_critical_catalog_arm_gamma_overlay(
          cloud, 2U, budget, result)),
      "the fresh verifier preserves the extra-shell decision");
}

void test_mirror_two_saddles_share_one_birth_batch() {
  const CanonicalPointCloud cloud = mirror_cloud();
  const ExactCriticalCatalogArmGammaOverlayBudget budget =
      full_budget(1U, ExactStrictGammaBudget{6U, 4U, 8U});
  const ExactCriticalCatalogArmGammaOverlayResult result =
      build_exact_critical_catalog_arm_gamma_overlay(cloud, 2U, budget);
  const ExactCriticalCatalogArmGammaOverlayVerification verification =
      verify_exact_critical_catalog_arm_gamma_overlay(
          cloud, 2U, budget, result);

  check(
      complete_facts(result) && all_certificates_close(verification) &&
          result.critical_catalog.has_value() &&
          result.saddle_family_records.size() == 2U &&
          result.batch_records.size() == 1U &&
          result.arm_targets.size() == 6U &&
          result.target_components.size() == 5U,
      "the mirror cloud closes two saddles, one batch, six arms and five targets");
  check(
      result.batch_records.size() == 1U &&
          result.batch_records[0].squared_level == level(169, 36) &&
          result.batch_records[0].saddle_family_record_indices.size() == 2U &&
          result.batch_records[0].target_component_indices.size() == 5U,
      "the two exact mirror saddles remain simultaneous in one compact batch");

  bool all_families_inherit_birth = true;
  for (const ExactCriticalCatalogArmGammaSaddleFamilyRecord& family :
       result.saddle_family_records) {
    all_families_inherit_birth =
        all_families_inherit_birth &&
        family.family.decision ==
            ExactCriticalArmFamilyDecision::
                all_arms_complete_at_regular_active_facets &&
        family.reduced_gamma_batch_record_index == 0U &&
        family.reduced_gamma_group_index.has_value() &&
        family.reduced_gamma_group_kind ==
            ExactReducedGammaBatchGroupKind::birth &&
        family.arm_target_indices.size() == 3U;
  }
  check(
      all_families_inherit_birth,
      "both Morse saddle families inherit the one reduced birth without inference");

  const std::set<PointLabel> expected_singletons{
      {0U, 1U}, {0U, 2U}, {0U, 3U}, {1U, 3U}, {2U, 3U}};
  std::set<PointLabel> observed_singletons;
  bool every_target_is_full_isolated_singleton = true;
  for (const ExactCriticalCatalogArmGammaTargetComponent& target :
       result.target_components) {
    every_target_is_full_isolated_singleton =
        every_target_is_full_isolated_singleton &&
        target.batch_record_index == 0U &&
        target.reduced_component_kind ==
            ExactReducedGammaStrictComponentKind::omitted_isolated_facet &&
        target.strict_component.facet_point_ids.size() == 1U &&
        target.strict_component.canonical_representative_facet_point_ids ==
            target.strict_component.facet_point_ids.front();
    if (target.strict_component.facet_point_ids.size() == 1U) {
      observed_singletons.insert(target.strict_component.facet_point_ids.front());
    }
  }
  check(
      every_target_is_full_isolated_singleton &&
          observed_singletons == expected_singletons,
      "the target arena retains all five full-pi0 singleton witnesses and separate reduced kinds");

  const ExactCriticalCatalogArmGammaArmTarget* lower_shared =
      arm_target_with_closed_label(result, {0U, 1U, 3U}, 1U);
  const ExactCriticalCatalogArmGammaArmTarget* upper_shared =
      arm_target_with_closed_label(result, {0U, 2U, 3U}, 2U);
  const ExactCriticalCatalogArmGammaTargetComponent* shared_component =
      component_for_arm_target(result, lower_shared);
  check(
      lower_shared != nullptr && upper_shared != nullptr &&
          lower_shared->target_component_index ==
              upper_shared->target_component_index &&
          lower_shared->strict_component_index ==
              upper_shared->strict_component_index &&
          shared_component != nullptr &&
          shared_component->strict_component.facet_point_ids ==
              std::vector<PointLabel>{{0U, 3U}},
      "one singleton target is shared across the two distinct saddle events");
  check(
      result.counters.catalog_saddle_event_reference_count == 2U &&
          result.counters.critical_arm_count == 6U &&
          result.counters.complete_critical_arm_family_count == 2U &&
          result.counters.reduced_gamma_batch_build_count == 1U &&
          result.counters.complete_reduced_gamma_batch_count == 1U &&
          result.counters.target_component_count == 5U &&
          result.counters.target_component_facet_reference_count == 5U &&
          result.counters.target_component_point_id_reference_count == 20U &&
          result.counters.arm_target_count == 6U &&
          result.counters.shared_target_arm_count == 1U,
      "the mirror counters expose exactly one deduplicated shared target");
}

void test_shared_terminal_prior_root_and_isolated_target() {
  const CanonicalPointCloud cloud = shared_terminal_cloud();
  const ExactCriticalCatalogArmGammaOverlayBudget budget =
      full_budget(2U, ExactStrictGammaBudget{10U, 5U, 15U});
  const ExactCriticalCatalogArmGammaOverlayResult result =
      build_exact_critical_catalog_arm_gamma_overlay(cloud, 3U, budget);
  const ExactCriticalCatalogArmGammaOverlayVerification verification =
      verify_exact_critical_catalog_arm_gamma_overlay(
          cloud, 3U, budget, result);

  check(
      complete_facts(result) && all_certificates_close(verification) &&
          result.saddle_family_records.size() == 3U &&
          result.batch_records.size() == 3U &&
          result.arm_targets.size() == 7U,
      "the generic five-point catalogue closes all three order-three saddle families under budget two");

  const PointLabel closed_label{0U, 1U, 2U, 3U};
  const ExactCriticalCatalogArmGammaSaddleFamilyRecord* family =
      family_with_closed_label(result, closed_label);
  const ExactCriticalCatalogArmGammaArmTarget* removed_one =
      arm_target_with_closed_label(result, closed_label, 1U);
  const ExactCriticalCatalogArmGammaArmTarget* removed_two =
      arm_target_with_closed_label(result, closed_label, 2U);
  const ExactCriticalCatalogArmGammaArmTarget* removed_three =
      arm_target_with_closed_label(result, closed_label, 3U);
  const ExactCriticalCatalogArmGammaTargetComponent* prior_root =
      component_for_arm_target(result, removed_one);
  const ExactCriticalCatalogArmGammaTargetComponent* isolated =
      component_for_arm_target(result, removed_two);
  const std::vector<PointLabel> expected_prior_root_facets{
      {0U, 1U, 2U},
      {0U, 1U, 4U},
      {0U, 2U, 3U},
      {0U, 2U, 4U},
      {0U, 3U, 4U},
      {1U, 2U, 4U},
      {2U, 3U, 4U},
  };
  check(
      family != nullptr &&
          family->family.decision ==
              ExactCriticalArmFamilyDecision::
                  all_arms_complete_at_regular_active_facets &&
          family->family.critical_shell_point_ids ==
              PointLabel({1U, 2U, 3U}) &&
          family->reduced_gamma_group_kind ==
              ExactReducedGammaBatchGroupKind::continuation &&
          removed_one != nullptr && removed_two != nullptr &&
          removed_three != nullptr,
      "the selected level-25925/338 family inherits a reduced continuation");
  check(
      removed_one != nullptr && removed_three != nullptr &&
          removed_one->target_component_index ==
              removed_three->target_component_index &&
          removed_two != nullptr &&
          removed_two->target_component_index !=
              removed_one->target_component_index,
      "removed shell points one and three share a target while point two remains distinct");
  check(
      prior_root != nullptr &&
          prior_root->reduced_component_kind ==
              ExactReducedGammaStrictComponentKind::
                  prior_nontrivial_reduced_root &&
          prior_root->strict_component.facet_point_ids ==
              expected_prior_root_facets &&
          isolated != nullptr &&
          isolated->reduced_component_kind ==
              ExactReducedGammaStrictComponentKind::omitted_isolated_facet &&
          isolated->strict_component.facet_point_ids ==
              std::vector<PointLabel>{{0U, 1U, 3U}},
      "full-pi0 targets distinguish the prior root from the absorbed isolated facet");

  if (family != nullptr && removed_one != nullptr && removed_two != nullptr &&
      removed_three != nullptr) {
    const auto& classes = family->family.terminal_label_classes;
    const bool indices_valid =
        removed_one->terminal_label_class_index < classes.size() &&
        removed_two->terminal_label_class_index < classes.size() &&
        removed_three->terminal_label_class_index < classes.size();
    check(
        indices_valid &&
            classes[removed_one->terminal_label_class_index]
                    .canonical_terminal.facet_point_ids ==
                PointLabel({0U, 1U, 2U}) &&
            classes[removed_two->terminal_label_class_index]
                    .canonical_terminal.facet_point_ids ==
                PointLabel({0U, 1U, 3U}) &&
            removed_one->terminal_label_class_index ==
                removed_three->terminal_label_class_index,
        "the stored 6.7 terminal classes preserve the shared-label provenance");
  }

  const CanonicalPointCloud incomplete_cloud = critical_arm_cloud();
  const ExactCriticalCatalogArmGammaOverlayBudget zero_budget =
      full_budget(0U, ExactStrictGammaBudget{6U, 4U, 8U});
  const ExactCriticalCatalogArmGammaOverlayResult incomplete =
      build_exact_critical_catalog_arm_gamma_overlay(
          incomplete_cloud, 2U, zero_budget);
  check(
      incomplete.critical_catalog.has_value() &&
          incomplete.critical_catalog->no_relevant_extra_shell_degeneracy &&
          !incomplete.all_critical_arm_families_complete &&
          incomplete.counters.incomplete_critical_arm_family_count >= 1U &&
          incomplete.counters.reduced_gamma_batch_build_count == 0U &&
          incomplete.decision ==
              ExactCriticalCatalogArmGammaOverlayDecision::
                  no_overlay_incomplete_critical_arm_family,
      "zero chain budget fails closed when a catalogue family is incomplete");
  check_empty_gamma_target_payload(incomplete, "incomplete arm family");
  check(
      all_certificates_close(verify_exact_critical_catalog_arm_gamma_overlay(
          incomplete_cloud, 2U, zero_budget, incomplete)),
      "the fresh verifier certifies the incomplete-family diagnostic");

  const ExactCriticalCatalogArmGammaOverlayBudget one_budget =
      full_budget(1U, ExactStrictGammaBudget{6U, 4U, 8U});
  const ExactCriticalCatalogArmGammaOverlayResult complete =
      build_exact_critical_catalog_arm_gamma_overlay(
          incomplete_cloud, 2U, one_budget);
  check(
      complete_facts(complete) &&
          complete.counters.incomplete_critical_arm_family_count == 0U &&
          complete.counters.committed_chain_segment_count >= 1U &&
          all_certificates_close(
              verify_exact_critical_catalog_arm_gamma_overlay(
                  incomplete_cloud, 2U, one_budget, complete)),
      "one chain segment per arm closes the same generic catalogue");
}

void test_fresh_verifier_rejects_outer_and_nested_mutations() {
  const CanonicalPointCloud cloud = q2_triangle_cloud();
  const ExactCriticalCatalogArmGammaOverlayBudget budget =
      full_budget(1U, ExactStrictGammaBudget{3U, 1U, 2U});
  const ExactCriticalCatalogArmGammaOverlayResult baseline =
      build_exact_critical_catalog_arm_gamma_overlay(cloud, 2U, budget);
  check(
      complete_facts(baseline) && baseline.critical_catalog.has_value() &&
          baseline.saddle_family_records.size() == 1U &&
          baseline.batch_records.size() == 1U &&
          baseline.target_components.size() == 2U &&
          baseline.arm_targets.size() == 2U,
      "the q2 mutation baseline is complete and deliberately small");

  const auto rejects = [&] (
                           ExactCriticalCatalogArmGammaOverlayResult mutated,
                           const std::string& label) {
    const ExactCriticalCatalogArmGammaOverlayVerification verification =
        verify_exact_critical_catalog_arm_gamma_overlay(
            cloud, 2U, budget, mutated);
    check(
        !verification.fresh_replay_certified &&
            !verification.
                exact_critical_catalog_arm_gamma_overlay_decision_certified,
        label);
  };

  {
    auto mutated = baseline;
    --mutated.requested_budget.maximum_saddle_event_count;
    rejects(std::move(mutated), "the verifier rejects the stored outer budget");
  }
  {
    auto mutated = baseline;
    ++mutated.required_target_component_capacity;
    rejects(std::move(mutated), "the verifier rejects a derived preflight size");
  }
  {
    auto mutated = baseline;
    if (mutated.critical_catalog.has_value()) {
      mutated.critical_catalog->decision =
          ExactCriticalCatalogDecision::
              complete_catalog_with_relevant_extra_shell_degeneracy;
    }
    rejects(
        std::move(mutated),
        "an observed catalogue decision cannot steer fresh replay");
  }
  {
    auto mutated = baseline;
    if (!mutated.saddle_family_records.empty()) {
      mutated.saddle_family_records.front().family.decision =
          ExactCriticalArmFamilyDecision::incomplete_chain_budget_exhausted;
    }
    rejects(std::move(mutated), "the verifier rejects a nested 6.7 mutation");
  }
  {
    auto mutated = baseline;
    if (!mutated.saddle_family_records.empty()) {
      mutated.saddle_family_records.front().reduced_gamma_group_kind =
          ExactReducedGammaBatchGroupKind::continuation;
    }
    rejects(
        std::move(mutated),
        "the verifier rejects an inherited reduced-group-kind mutation");
  }
  {
    auto mutated = baseline;
    if (!mutated.batch_records.empty()) {
      ++mutated.batch_records.front().catalog_h0_batch_index;
    }
    rejects(std::move(mutated), "the verifier rejects a compact batch mutation");
  }
  {
    auto mutated = baseline;
    if (!mutated.target_components.empty()) {
      mutated.target_components.front().strict_component.facet_point_ids.clear();
    }
    rejects(
        std::move(mutated),
        "the verifier rejects a full-pi0 target-witness mutation");
  }
  {
    auto mutated = baseline;
    if (!mutated.target_components.empty()) {
      mutated.target_components.front().reduced_component_kind =
          ExactReducedGammaStrictComponentKind::
              prior_nontrivial_reduced_root;
    }
    rejects(
        std::move(mutated),
        "the verifier rejects a separately copied reduced-kind mutation");
  }
  {
    auto mutated = baseline;
    if (!mutated.arm_targets.empty() &&
        !mutated.target_components.empty()) {
      mutated.arm_targets.front().target_component_index =
          (mutated.arm_targets.front().target_component_index + 1U) %
          mutated.target_components.size();
    }
    rejects(std::move(mutated), "the verifier rejects an arm target mutation");
  }
  {
    auto mutated = baseline;
    mutated.every_catalog_saddle_arm_has_one_target = false;
    rejects(std::move(mutated), "the verifier rejects a result-fact mutation");
  }
  {
    auto mutated = baseline;
    ++mutated.counters.shared_target_arm_count;
    rejects(std::move(mutated), "the verifier rejects a counter mutation");
  }
  {
    auto mutated = baseline;
    mutated.decision =
        ExactCriticalCatalogArmGammaOverlayDecision::
            no_overlay_incomplete_critical_arm_family;
    rejects(
        std::move(mutated),
        "an observed top-level decision cannot steer fresh replay");
  }
  {
    auto mutated = baseline;
    mutated.scope = ExactCriticalCatalogArmGammaOverlayScope::unspecified;
    rejects(std::move(mutated), "the verifier rejects a scope mutation");
  }

  ExactCriticalCatalogArmGammaOverlayBudget different_budget = budget;
  different_budget.per_arm_chain_budget = {2U};
  const ExactCriticalCatalogArmGammaOverlayVerification budget_mismatch =
      verify_exact_critical_catalog_arm_gamma_overlay(
          cloud, 2U, different_budget, baseline);
  check(
      !budget_mismatch.fresh_replay_certified &&
          !budget_mismatch.
              exact_critical_catalog_arm_gamma_overlay_decision_certified,
      "the verifier trusts its external budget rather than the stored copy");

  const std::array<CertifiedPoint3, 3> twin_input{
      point(-2.0, 0.0), point(0.0, 1.25), point(2.0, 0.0)};
  const CanonicalPointCloud twin_cloud = canonical_cloud(twin_input);
  const ExactCriticalCatalogArmGammaOverlayVerification twin_verification =
      verify_exact_critical_catalog_arm_gamma_overlay(
          twin_cloud, 2U, budget, baseline);
  check(
      !twin_verification.fresh_replay_certified &&
          !twin_verification.
              exact_critical_catalog_arm_gamma_overlay_decision_certified,
      "the fresh verifier rejects a same-cardinality twin cloud");
}

}  // namespace

int main() {
  test_default_invalid_domain_and_caps();
  test_overlay_preflight_is_atomic_in_every_dimension();
  test_subordinate_budget_failures();
  test_relevant_extra_shell_blocks_before_arm_families();
  test_mirror_two_saddles_share_one_birth_batch();
  test_shared_terminal_prior_root_and_isolated_target();
  test_fresh_verifier_rejects_outer_and_nested_mutations();

  if (failures != 0) {
    std::cerr << failures
              << " critical-catalog arm-Gamma overlay test(s) failed\n";
    return 1;
  }
  return 0;
}
