#include "morsehgp3d/hierarchy/critical_catalog_reduced_gamma_overlay.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactCriticalCatalogBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogDecision;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogReducedGammaEventProjection;
using morsehgp3d::hierarchy::ExactCriticalCatalogReducedGammaEventRole;
using morsehgp3d::hierarchy::ExactCriticalCatalogReducedGammaGroupOverlay;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogReducedGammaHistoryLabelKind;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogReducedGammaHistoryLabelSlot;
using morsehgp3d::hierarchy::ExactCriticalCatalogReducedGammaOverlayBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogReducedGammaOverlayDecision;
using morsehgp3d::hierarchy::ExactCriticalCatalogReducedGammaOverlayResult;
using morsehgp3d::hierarchy::ExactCriticalCatalogReducedGammaOverlayScope;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogReducedGammaOverlayVerification;
using morsehgp3d::hierarchy::
    ExactPersistentReducedGammaHistoryGroupRecord;
using morsehgp3d::hierarchy::
    ExactPersistentReducedGammaOrderHistoryBudget;
using morsehgp3d::hierarchy::
    ExactPersistentReducedGammaOrderHistoryDecision;
using morsehgp3d::hierarchy::ExactReducedGammaBatchGroupKind;
using morsehgp3d::hierarchy::ExactStrictGammaBudget;
using morsehgp3d::hierarchy::build_exact_critical_catalog_reduced_gamma_overlay;
using morsehgp3d::hierarchy::verify_exact_critical_catalog_reduced_gamma_overlay;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

using PointLabel = std::vector<PointId>;
using LeveledLabel = std::pair<ExactLevel, PointLabel>;

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

[[nodiscard]] ExactPersistentReducedGammaOrderHistoryBudget
full_history_budget() {
  ExactPersistentReducedGammaOrderHistoryBudget budget;
  budget.gamma_budget = {
      ExactStrictGammaBudget::maximum_supported_facet_count,
      ExactStrictGammaBudget::maximum_supported_coface_count,
      ExactStrictGammaBudget::maximum_supported_union_attempt_count};
  budget.maximum_activation_level_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_activation_level_count;
  budget.maximum_total_facet_work_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_total_facet_work_count;
  budget.maximum_total_coface_work_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_total_coface_work_count;
  budget.maximum_total_union_work_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_total_union_work_count;
  budget.maximum_node_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_node_count;
  budget.maximum_child_reference_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_child_reference_count;
  budget.maximum_group_root_reference_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_group_root_reference_count;
  budget.maximum_group_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_group_count;
  budget.maximum_group_newly_active_facet_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_group_newly_active_facet_count;
  budget.maximum_group_equal_level_coface_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_group_equal_level_coface_count;
  budget.maximum_delta_facet_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_delta_facet_count;
  budget.maximum_delta_point_reference_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_delta_point_reference_count;
  return budget;
}

[[nodiscard]] ExactCriticalCatalogReducedGammaOverlayBudget full_budget() {
  ExactCriticalCatalogReducedGammaOverlayBudget budget;
  budget.critical_catalog_budget = full_catalog_budget();
  budget.reduced_gamma_history_budget = full_history_budget();
  budget.maximum_event_projection_count =
      ExactCriticalCatalogReducedGammaOverlayBudget::
          maximum_supported_event_projection_count;
  budget.maximum_group_overlay_count =
      ExactCriticalCatalogReducedGammaOverlayBudget::
          maximum_supported_group_overlay_count;
  budget.maximum_label_slot_count =
      ExactCriticalCatalogReducedGammaOverlayBudget::
          maximum_supported_label_slot_count;
  budget.maximum_history_point_id_scan_count =
      ExactCriticalCatalogReducedGammaOverlayBudget::
          maximum_supported_history_point_id_scan_count;
  budget.maximum_catalog_point_id_scan_count =
      ExactCriticalCatalogReducedGammaOverlayBudget::
          maximum_supported_catalog_point_id_scan_count;
  budget.maximum_group_event_reference_count =
      ExactCriticalCatalogReducedGammaOverlayBudget::
          maximum_supported_group_event_reference_count;
  return budget;
}

[[nodiscard]] CanonicalPointCloud triangle_cloud() {
  const std::array<CertifiedPoint3, 3> input{
      point(-2.0, 0.0), point(0.0, 1.0), point(2.0, 0.0)};
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

[[nodiscard]] bool all_certificates_close(
    const ExactCriticalCatalogReducedGammaOverlayVerification& verification) {
  return verification.requested_budget_certified &&
         verification.external_inputs_certified &&
         verification.derived_preflight_sizes_certified &&
         verification.critical_catalog_certified &&
         verification.reduced_gamma_history_certified &&
         verification.event_projections_certified &&
         verification.history_label_slots_certified &&
         verification.group_overlays_certified &&
         verification.result_facts_certified &&
         verification.counters_certified &&
         verification.decision_certified &&
         verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.
             exact_critical_catalog_reduced_gamma_overlay_decision_certified;
}

[[nodiscard]] bool complete_facts(
    const ExactCriticalCatalogReducedGammaOverlayResult& result) {
  return result.overlay_candidate_space_size_certified &&
         result.overlay_preflight_budget_sufficient &&
         result.
             subordinate_geometry_started_only_after_successful_overlay_preflight &&
         result.critical_catalog_fresh_replay_certified &&
         result.no_relevant_extra_shell_degeneracy &&
         result.reduced_gamma_history_fresh_replay_certified &&
         result.history_equality_slots_exhaustively_indexed &&
         result.closed_label_theorem_applied_to_every_projection &&
         result.every_catalog_h0_role_projected_exactly_once &&
         result.every_history_label_slot_partitioned_by_provenance &&
         result.catalog_births_exactly_deferred_newly_active_facets &&
         result.catalog_saddles_only_non_deferred_groups &&
         result.group_kinds_inherited_only_from_history &&
         result.simultaneous_history_batches_preserved &&
         result.
             birth_projection_plus_residual_facet_count_equals_exhaustive_facet_count &&
         result.
             saddle_projection_plus_residual_coface_count_equals_exhaustive_coface_count &&
         result.critical_catalog_reduced_gamma_overlay_certified &&
         result.decision == ExactCriticalCatalogReducedGammaOverlayDecision::
                                complete_exhaustive_critical_catalog_reduced_gamma_overlay &&
         result.scope == ExactCriticalCatalogReducedGammaOverlayScope::
                             bounded_n14_k10_single_order_freshly_verified_critical_catalog_h0_provenance_to_exhaustive_persistent_reduced_gamma_history_groups_only;
}

[[nodiscard]] const PointLabel* label_for_slot(
    const ExactCriticalCatalogReducedGammaOverlayResult& result,
    const ExactCriticalCatalogReducedGammaHistoryLabelSlot& slot) {
  if (!result.reduced_gamma_history.has_value() ||
      slot.history_group_record_index >=
          result.reduced_gamma_history->group_records.size()) {
    return nullptr;
  }
  const ExactPersistentReducedGammaHistoryGroupRecord& record =
      result.reduced_gamma_history
          ->group_records[slot.history_group_record_index];
  if (slot.kind == ExactCriticalCatalogReducedGammaHistoryLabelKind::
                       newly_active_facet) {
    if (slot.history_group_local_label_index >=
        record.newly_active_facet_point_ids.size()) {
      return nullptr;
    }
    return &record.newly_active_facet_point_ids
                [slot.history_group_local_label_index];
  }
  if (slot.history_group_local_label_index >=
      record.equal_level_coface_point_ids.size()) {
    return nullptr;
  }
  return &record.equal_level_coface_point_ids
              [slot.history_group_local_label_index];
}

[[nodiscard]] const ExactCriticalCatalogReducedGammaEventProjection*
projection_with_closed_label(
    const ExactCriticalCatalogReducedGammaOverlayResult& result,
    ExactCriticalCatalogReducedGammaEventRole role,
    const PointLabel& label) {
  if (!result.critical_catalog.has_value()) {
    return nullptr;
  }
  const auto iterator = std::find_if(
      result.event_projections.begin(),
      result.event_projections.end(),
      [&](const ExactCriticalCatalogReducedGammaEventProjection& projection) {
        return projection.role == role &&
               projection.catalog_event_index <
                   result.critical_catalog->events.size() &&
               result.critical_catalog
                       ->events[projection.catalog_event_index]
                       .closed_point_ids == label;
      });
  return iterator == result.event_projections.end() ? nullptr : &*iterator;
}

[[nodiscard]] const ExactPersistentReducedGammaHistoryGroupRecord*
history_record_for_projection(
    const ExactCriticalCatalogReducedGammaOverlayResult& result,
    const ExactCriticalCatalogReducedGammaEventProjection* projection) {
  if (projection == nullptr ||
      !result.reduced_gamma_history.has_value() ||
      projection->history_group_record_index >=
          result.reduced_gamma_history->group_records.size()) {
    return nullptr;
  }
  return &result.reduced_gamma_history
              ->group_records[projection->history_group_record_index];
}

[[nodiscard]] const ExactCriticalCatalogReducedGammaGroupOverlay*
group_overlay_for_record(
    const ExactCriticalCatalogReducedGammaOverlayResult& result,
    std::size_t record_index) {
  const auto iterator = std::find_if(
      result.group_overlays.begin(),
      result.group_overlays.end(),
      [record_index](const ExactCriticalCatalogReducedGammaGroupOverlay& group) {
        return group.history_group_record_index == record_index;
      });
  return iterator == result.group_overlays.end() ? nullptr : &*iterator;
}

[[nodiscard]] std::set<LeveledLabel> residual_labels(
    const ExactCriticalCatalogReducedGammaOverlayResult& result,
    ExactCriticalCatalogReducedGammaHistoryLabelKind kind) {
  std::set<LeveledLabel> residuals;
  if (!result.reduced_gamma_history.has_value()) {
    return residuals;
  }
  for (const ExactCriticalCatalogReducedGammaHistoryLabelSlot& slot :
       result.history_label_slots) {
    if (slot.kind != kind || slot.event_projection_index.has_value() ||
        slot.history_group_record_index >=
            result.reduced_gamma_history->group_records.size()) {
      continue;
    }
    const PointLabel* label = label_for_slot(result, slot);
    if (label != nullptr) {
      const ExactLevel& squared_level =
          result.reduced_gamma_history
              ->group_records[slot.history_group_record_index]
              .squared_level;
      residuals.emplace(squared_level, *label);
    }
  }
  return residuals;
}

void check_empty_overlay_payload(
    const ExactCriticalCatalogReducedGammaOverlayResult& result,
    const std::string& label) {
  check(
      result.event_projections.empty() &&
          result.history_label_slots.empty() &&
          result.group_overlays.empty(),
      label + ": no partial overlay payload is published");
}

void test_default_invalid_domain_and_caps() {
  const ExactCriticalCatalogReducedGammaOverlayResult empty;
  check(
      empty.decision ==
              ExactCriticalCatalogReducedGammaOverlayDecision::not_certified &&
          empty.scope ==
              ExactCriticalCatalogReducedGammaOverlayScope::unspecified &&
          !empty.critical_catalog.has_value() &&
          !empty.reduced_gamma_history.has_value() &&
          empty.event_projections.empty() &&
          empty.history_label_slots.empty() && empty.group_overlays.empty() &&
          !empty.critical_catalog_reduced_gamma_overlay_certified,
      "the default catalogue-to-history overlay certifies no provenance");

  const CanonicalPointCloud cloud = triangle_cloud();
  const ExactCriticalCatalogReducedGammaOverlayBudget budget = full_budget();
  const std::array<CertifiedPoint3, 2> two_points{point(0.0), point(2.0)};
  const CanonicalPointCloud two_point_cloud = canonical_cloud(two_points);
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_critical_catalog_reduced_gamma_overlay(
            two_point_cloud, 1U, budget));
      },
      "the composite overlay rejects n below three");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_critical_catalog_reduced_gamma_overlay(
            cloud, 1U, budget));
      },
      "the composite overlay rejects order one");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_critical_catalog_reduced_gamma_overlay(
            cloud, 3U, budget));
      },
      "the composite overlay rejects the terminal order");

  using BudgetMember =
      std::size_t ExactCriticalCatalogReducedGammaOverlayBudget::*;
  struct CapCase {
    const char* name;
    BudgetMember member;
    std::size_t cap;
  };
  const std::array<CapCase, 6> cap_cases{{
      {"event projection",
       &ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_event_projection_count,
       ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_supported_event_projection_count},
      {"group overlay",
       &ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_group_overlay_count,
       ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_supported_group_overlay_count},
      {"label slot",
       &ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_label_slot_count,
       ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_supported_label_slot_count},
      {"history point scan",
       &ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_history_point_id_scan_count,
       ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_supported_history_point_id_scan_count},
      {"catalogue point scan",
       &ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_catalog_point_id_scan_count,
       ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_supported_catalog_point_id_scan_count},
      {"group event reference",
       &ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_group_event_reference_count,
       ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_supported_group_event_reference_count},
  }};
  for (const CapCase& cap_case : cap_cases) {
    ExactCriticalCatalogReducedGammaOverlayBudget invalid = budget;
    invalid.*(cap_case.member) = cap_case.cap + 1U;
    check_throws<std::invalid_argument>(
        [&] {
          static_cast<void>(build_exact_critical_catalog_reduced_gamma_overlay(
              cloud, 2U, invalid));
        },
        std::string{"the composite overlay rejects an oversized "} +
            cap_case.name + " cap");
  }

  ExactCriticalCatalogReducedGammaOverlayBudget invalid_catalog = budget;
  invalid_catalog.critical_catalog_budget.maximum_candidate_count =
      ExactCriticalCatalogBudget::maximum_supported_candidate_count + 1U;
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_critical_catalog_reduced_gamma_overlay(
            cloud, 2U, invalid_catalog));
      },
      "the composite overlay rejects an oversized subordinate catalogue cap");

  ExactCriticalCatalogReducedGammaOverlayBudget invalid_history = budget;
  invalid_history.reduced_gamma_history_budget.maximum_group_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_group_count +
      1U;
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_critical_catalog_reduced_gamma_overlay(
            cloud, 2U, invalid_history));
      },
      "the composite overlay rejects an oversized subordinate history cap");
}

void test_overlay_preflight_is_atomic_in_every_dimension() {
  const CanonicalPointCloud cloud = triangle_cloud();
  const ExactCriticalCatalogReducedGammaOverlayBudget budget = full_budget();
  const ExactCriticalCatalogReducedGammaOverlayResult successful =
      build_exact_critical_catalog_reduced_gamma_overlay(cloud, 2U, budget);

  using BudgetMember =
      std::size_t ExactCriticalCatalogReducedGammaOverlayBudget::*;
  using ResultMember =
      std::size_t ExactCriticalCatalogReducedGammaOverlayResult::*;
  struct Dimension {
    const char* name;
    BudgetMember maximum;
    ResultMember required;
  };
  const std::array<Dimension, 6> dimensions{{
      {"event projections",
       &ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_event_projection_count,
       &ExactCriticalCatalogReducedGammaOverlayResult::
           required_event_projection_capacity},
      {"group overlays",
       &ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_group_overlay_count,
       &ExactCriticalCatalogReducedGammaOverlayResult::
           required_group_overlay_capacity},
      {"label slots",
       &ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_label_slot_count,
       &ExactCriticalCatalogReducedGammaOverlayResult::
           required_label_slot_capacity},
      {"history point scans",
       &ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_history_point_id_scan_count,
       &ExactCriticalCatalogReducedGammaOverlayResult::
           required_history_point_id_scan_capacity},
      {"catalogue point scans",
       &ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_catalog_point_id_scan_count,
       &ExactCriticalCatalogReducedGammaOverlayResult::
           required_catalog_point_id_scan_capacity},
      {"group event references",
       &ExactCriticalCatalogReducedGammaOverlayBudget::
           maximum_group_event_reference_count,
       &ExactCriticalCatalogReducedGammaOverlayResult::
           required_group_event_reference_capacity},
  }};

  for (const Dimension& dimension : dimensions) {
    const std::size_t required = successful.*(dimension.required);
    check(required > 0U, std::string{dimension.name} + " preflight is positive");
    if (required == 0U) {
      continue;
    }
    ExactCriticalCatalogReducedGammaOverlayBudget insufficient = budget;
    insufficient.*(dimension.maximum) = required - 1U;
    const ExactCriticalCatalogReducedGammaOverlayResult result =
        build_exact_critical_catalog_reduced_gamma_overlay(
            cloud, 2U, insufficient);
    const ExactCriticalCatalogReducedGammaOverlayVerification verification =
        verify_exact_critical_catalog_reduced_gamma_overlay(
            cloud, 2U, insufficient, result);
    check(
        result.overlay_candidate_space_size_certified &&
            !result.overlay_preflight_budget_sufficient &&
            result.decision ==
                ExactCriticalCatalogReducedGammaOverlayDecision::
                    no_overlay_preflight_budget_insufficient &&
            result.counters.preflight_count == 1U &&
            result.counters.critical_catalog_build_count == 0U &&
            result.counters.reduced_gamma_history_build_count == 0U &&
            !result.critical_catalog.has_value() &&
            !result.reduced_gamma_history.has_value() &&
            all_certificates_close(verification),
        std::string{dimension.name} +
            " fails atomically before either subordinate geometry builder");
    check_empty_overlay_payload(result, dimension.name);
  }
}

void test_subordinate_fail_closed_decisions() {
  const CanonicalPointCloud cloud = triangle_cloud();

  ExactCriticalCatalogReducedGammaOverlayBudget catalog_insufficient =
      full_budget();
  catalog_insufficient.critical_catalog_budget.maximum_candidate_count = 6U;
  const ExactCriticalCatalogReducedGammaOverlayResult no_catalog =
      build_exact_critical_catalog_reduced_gamma_overlay(
          cloud, 2U, catalog_insufficient);
  check(
      no_catalog.overlay_preflight_budget_sufficient &&
          no_catalog.critical_catalog.has_value() &&
          no_catalog.critical_catalog->decision ==
              ExactCriticalCatalogDecision::
                  no_catalog_preflight_budget_insufficient &&
          !no_catalog.reduced_gamma_history.has_value() &&
          no_catalog.decision ==
              ExactCriticalCatalogReducedGammaOverlayDecision::
                  no_catalog_preflight_budget_insufficient,
      "an insufficient catalogue budget publishes only its atomic preflight result");
  check_empty_overlay_payload(no_catalog, "catalogue preflight failure");
  check(
      all_certificates_close(
          verify_exact_critical_catalog_reduced_gamma_overlay(
              cloud, 2U, catalog_insufficient, no_catalog)),
      "the fresh composite verifier certifies the catalogue-budget decision");

  ExactCriticalCatalogReducedGammaOverlayBudget history_insufficient =
      full_budget();
  history_insufficient.reduced_gamma_history_budget.gamma_budget
      .maximum_enumerated_facet_count = 2U;
  const ExactCriticalCatalogReducedGammaOverlayResult no_history =
      build_exact_critical_catalog_reduced_gamma_overlay(
          cloud, 2U, history_insufficient);
  check(
      no_history.critical_catalog.has_value() &&
          no_history.critical_catalog_fresh_replay_certified &&
          no_history.reduced_gamma_history.has_value() &&
          no_history.reduced_gamma_history->decision ==
              ExactPersistentReducedGammaOrderHistoryDecision::
                  no_history_preflight_budget_insufficient &&
          no_history.decision ==
              ExactCriticalCatalogReducedGammaOverlayDecision::
                  no_history_preflight_budget_insufficient,
      "an insufficient history budget retains the fresh catalogue and only the history preflight result");
  check_empty_overlay_payload(no_history, "history preflight failure");
  check(
      all_certificates_close(
          verify_exact_critical_catalog_reduced_gamma_overlay(
              cloud, 2U, history_insufficient, no_history)),
      "the fresh composite verifier certifies the history-budget decision");
}

void test_relevant_extra_shell_blocks_before_history() {
  const std::array<CertifiedPoint3, 3> input{
      point(0.0, 0.0), point(2.0, 0.0), point(0.0, 2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactCriticalCatalogReducedGammaOverlayBudget budget = full_budget();
  const ExactCriticalCatalogReducedGammaOverlayResult result =
      build_exact_critical_catalog_reduced_gamma_overlay(cloud, 2U, budget);

  check(
      result.critical_catalog.has_value() &&
          result.critical_catalog->decision ==
              ExactCriticalCatalogDecision::
                  complete_catalog_with_relevant_extra_shell_degeneracy &&
          result.critical_catalog->extra_shell_degeneracies.size() == 1U &&
          !result.no_relevant_extra_shell_degeneracy &&
          !result.reduced_gamma_history.has_value() &&
          result.counters.reduced_gamma_history_build_count == 0U &&
          result.decision ==
              ExactCriticalCatalogReducedGammaOverlayDecision::
                  no_overlay_relevant_extra_shell_degeneracy,
      "the right-triangle extra shell blocks before history construction");
  check_empty_overlay_payload(result, "relevant extra-shell degeneracy");
  check(
      all_certificates_close(
          verify_exact_critical_catalog_reduced_gamma_overlay(
              cloud, 2U, budget, result)),
      "the fresh composite verifier preserves the extra-shell blocking decision");
}

void test_triangle_generic_overlay() {
  const CanonicalPointCloud cloud = triangle_cloud();
  const ExactCriticalCatalogReducedGammaOverlayBudget budget = full_budget();
  const ExactCriticalCatalogReducedGammaOverlayResult result =
      build_exact_critical_catalog_reduced_gamma_overlay(cloud, 2U, budget);
  const ExactCriticalCatalogReducedGammaOverlayVerification verification =
      verify_exact_critical_catalog_reduced_gamma_overlay(
          cloud, 2U, budget, result);

  check(
      complete_facts(result) && all_certificates_close(verification) &&
          result.critical_catalog.has_value() &&
          result.reduced_gamma_history.has_value(),
      "the generic triangle closes the complete catalogue-to-history overlay");
  check(
      result.exhaustive_facet_count == 3U &&
          result.exhaustive_coface_count == 1U &&
          result.event_projections.size() == 3U &&
          result.history_label_slots.size() == 4U &&
          result.group_overlays.size() == 3U &&
          result.counters.catalog_birth_reference_count == 2U &&
          result.counters.catalog_saddle_reference_count == 1U &&
          result.counters.birth_event_projection_count == 2U &&
          result.counters.saddle_event_projection_count == 1U &&
          result.counters.residual_newly_active_facet_slot_count == 1U &&
          result.counters.residual_equal_level_coface_slot_count == 0U,
      "the triangle partitions three facets and one coface into exact provenance slots");

  const auto* birth01 = projection_with_closed_label(
      result, ExactCriticalCatalogReducedGammaEventRole::birth, {0U, 1U});
  const auto* birth12 = projection_with_closed_label(
      result, ExactCriticalCatalogReducedGammaEventRole::birth, {1U, 2U});
  const auto* saddle012 = projection_with_closed_label(
      result, ExactCriticalCatalogReducedGammaEventRole::saddle, {0U, 1U, 2U});
  const auto* birth01_record = history_record_for_projection(result, birth01);
  const auto* birth12_record = history_record_for_projection(result, birth12);
  const auto* saddle_record = history_record_for_projection(result, saddle012);
  check(
      birth01 != nullptr && birth12 != nullptr && saddle012 != nullptr &&
          birth01->history_label_kind ==
              ExactCriticalCatalogReducedGammaHistoryLabelKind::
                  newly_active_facet &&
          birth12->history_label_kind ==
              ExactCriticalCatalogReducedGammaHistoryLabelKind::
                  newly_active_facet &&
          saddle012->history_label_kind ==
              ExactCriticalCatalogReducedGammaHistoryLabelKind::
                  equal_level_coface &&
          birth01_record != nullptr && birth12_record != nullptr &&
          saddle_record != nullptr &&
          birth01_record->kind ==
              ExactReducedGammaBatchGroupKind::deferred_isolated_facet &&
          birth12_record->kind ==
              ExactReducedGammaBatchGroupKind::deferred_isolated_facet &&
          saddle_record->kind == ExactReducedGammaBatchGroupKind::birth,
      "catalogue births map to deferred facets while the triangle saddle creates the reduced root");

  check(
      residual_labels(
          result,
          ExactCriticalCatalogReducedGammaHistoryLabelKind::
              newly_active_facet) ==
              std::set<LeveledLabel>{{level(4), {0U, 2U}}} &&
          residual_labels(
              result,
              ExactCriticalCatalogReducedGammaHistoryLabelKind::
                  equal_level_coface)
              .empty(),
      "the diameter facet is the triangle's sole exact residual label");
}

void test_e5_exact_provenance_and_residuals() {
  const CanonicalPointCloud cloud = e5_cloud();
  const ExactCriticalCatalogReducedGammaOverlayBudget budget = full_budget();
  const ExactCriticalCatalogReducedGammaOverlayResult result =
      build_exact_critical_catalog_reduced_gamma_overlay(cloud, 2U, budget);

  check(
      complete_facts(result) && result.critical_catalog.has_value() &&
          result.reduced_gamma_history.has_value(),
      "E5 closes the complete catalogue-to-history overlay");
  check(
      result.event_projections.size() == 10U &&
          result.history_label_slots.size() == 20U &&
          result.group_overlays.size() == 13U &&
          result.counters.catalog_birth_reference_count == 6U &&
          result.counters.catalog_saddle_reference_count == 4U &&
          result.counters.birth_event_projection_count == 6U &&
          result.counters.saddle_event_projection_count == 4U &&
          result.counters.residual_newly_active_facet_slot_count == 4U &&
          result.counters.residual_equal_level_coface_slot_count == 6U &&
          result.counters.group_event_reference_count == 10U &&
          result.counters.group_with_catalog_provenance_count == 10U &&
          result.counters.group_without_catalog_provenance_count == 3U &&
          6U + 4U == result.exhaustive_facet_count &&
          4U + 6U == result.exhaustive_coface_count,
      "E5 partitions six births, four saddles and every residual Gamma label exactly");

  const std::set<LeveledLabel> expected_residual_facets{
      {level(13, 2), {1U, 3U}},
      {level(17, 2), {0U, 3U}},
      {level(9), {0U, 4U}},
      {level(10), {1U, 4U}},
  };
  const std::set<LeveledLabel> expected_residual_cofaces{
      {level(17, 2), {0U, 1U, 3U}},
      {level(17, 2), {0U, 2U, 3U}},
      {level(85, 9), {0U, 3U, 4U}},
      {level(10), {0U, 1U, 4U}},
      {level(10), {1U, 2U, 4U}},
      {level(10), {1U, 3U, 4U}},
  };
  check(
      residual_labels(
          result,
          ExactCriticalCatalogReducedGammaHistoryLabelKind::
              newly_active_facet) == expected_residual_facets &&
          residual_labels(
              result,
              ExactCriticalCatalogReducedGammaHistoryLabelKind::
                  equal_level_coface) == expected_residual_cofaces,
      "E5 retains the exact four facet and six coface residual labels");

  const auto* birth012 = projection_with_closed_label(
      result,
      ExactCriticalCatalogReducedGammaEventRole::saddle,
      {0U, 1U, 2U});
  const auto* birth234 = projection_with_closed_label(
      result,
      ExactCriticalCatalogReducedGammaEventRole::saddle,
      {2U, 3U, 4U});
  const auto* merge123 = projection_with_closed_label(
      result,
      ExactCriticalCatalogReducedGammaEventRole::saddle,
      {1U, 2U, 3U});
  const auto* continuation024 = projection_with_closed_label(
      result,
      ExactCriticalCatalogReducedGammaEventRole::saddle,
      {0U, 2U, 4U});
  const auto* birth012_record =
      history_record_for_projection(result, birth012);
  const auto* birth234_record =
      history_record_for_projection(result, birth234);
  const auto* merge_record = history_record_for_projection(result, merge123);
  const auto* continuation_record =
      history_record_for_projection(result, continuation024);
  check(
      birth012_record != nullptr && birth234_record != nullptr &&
          merge_record != nullptr && continuation_record != nullptr &&
          birth012_record->kind == ExactReducedGammaBatchGroupKind::birth &&
          birth234_record->kind == ExactReducedGammaBatchGroupKind::birth &&
          merge_record->kind ==
              ExactReducedGammaBatchGroupKind::multifusion &&
          continuation_record->kind ==
              ExactReducedGammaBatchGroupKind::continuation,
      "E5 saddles inherit reduced births, continuation and multifusion only from history");

  const auto redundant = std::find_if(
      result.reduced_gamma_history->group_records.begin(),
      result.reduced_gamma_history->group_records.end(),
      [](const ExactPersistentReducedGammaHistoryGroupRecord& record) {
        return record.squared_level == level(85, 9);
      });
  check(
      redundant != result.reduced_gamma_history->group_records.end(),
      "E5 retains its fully redundant level-85/9 group");
  if (redundant != result.reduced_gamma_history->group_records.end()) {
    const auto* overlay =
        group_overlay_for_record(result, redundant->group_record_index);
    check(
        redundant->kind == ExactReducedGammaBatchGroupKind::continuation &&
            redundant->coverage_delta.has_value() &&
            redundant->coverage_delta->fully_redundant &&
            overlay != nullptr && !overlay->has_catalog_h0_provenance &&
            overlay->birth_event_projection_indices.empty() &&
            overlay->saddle_event_projection_indices.empty(),
        "the fully redundant E5 group remains explicit without invented catalogue provenance");
  }
}

void test_two_saddles_share_one_ternary_multifusion() {
  const std::array<CertifiedPoint3, 6> input{
      point(0.0, 0.0),
      point(0.25, 2.0),
      point(2.0, -1.0),
      point(2.0, 3.0),
      point(3.75, 2.0),
      point(4.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactCriticalCatalogReducedGammaOverlayResult result =
      build_exact_critical_catalog_reduced_gamma_overlay(
          cloud, 2U, full_budget());
  const auto* lower = projection_with_closed_label(
      result,
      ExactCriticalCatalogReducedGammaEventRole::saddle,
      {0U, 1U, 3U});
  const auto* upper = projection_with_closed_label(
      result,
      ExactCriticalCatalogReducedGammaEventRole::saddle,
      {3U, 4U, 5U});
  const auto* record = history_record_for_projection(result, lower);
  check(
      complete_facts(result) && lower != nullptr && upper != nullptr &&
          lower->history_batch_index == upper->history_batch_index &&
          lower->history_group_record_index ==
              upper->history_group_record_index &&
          record != nullptr &&
          record->squared_level == level(13, 4) &&
          record->kind == ExactReducedGammaBatchGroupKind::multifusion &&
          record->prior_root_node_ids.size() == 3U,
      "two simultaneous catalogue saddles project to one ternary multifusion");
  if (lower != nullptr) {
    const auto* overlay = group_overlay_for_record(
        result, lower->history_group_record_index);
    check(
        overlay != nullptr &&
            overlay->saddle_event_projection_indices.size() == 2U,
        "the ternary group retains both saddle references without sequentialization");
  }
}

void test_fresh_verifier_rejects_every_layer() {
  const CanonicalPointCloud cloud = triangle_cloud();
  const ExactCriticalCatalogReducedGammaOverlayBudget budget = full_budget();
  const ExactCriticalCatalogReducedGammaOverlayResult baseline =
      build_exact_critical_catalog_reduced_gamma_overlay(cloud, 2U, budget);

  const auto rejects = [&](
                           ExactCriticalCatalogReducedGammaOverlayResult mutated,
                           const std::string& label) {
    const auto verification =
        verify_exact_critical_catalog_reduced_gamma_overlay(
            cloud, 2U, budget, mutated);
    check(
        !verification.fresh_replay_certified &&
            !verification.
                 exact_critical_catalog_reduced_gamma_overlay_decision_certified,
        label);
  };

  {
    auto mutated = baseline;
    ++mutated.requested_budget.maximum_event_projection_count;
    rejects(std::move(mutated), "the verifier rejects the stored budget");
  }
  {
    auto mutated = baseline;
    check(mutated.critical_catalog.has_value(), "the baseline stores its catalogue");
    if (mutated.critical_catalog.has_value()) {
      mutated.critical_catalog->decision =
          ExactCriticalCatalogDecision::
              complete_catalog_with_relevant_extra_shell_degeneracy;
      rejects(
          std::move(mutated),
          "an observed catalogue decision cannot steer the fresh replay");
    }
  }
  {
    auto mutated = baseline;
    check(mutated.reduced_gamma_history.has_value(), "the baseline stores its history");
    if (mutated.reduced_gamma_history.has_value()) {
      mutated.reduced_gamma_history->decision =
          ExactPersistentReducedGammaOrderHistoryDecision::
              no_history_preflight_budget_insufficient;
      rejects(
          std::move(mutated),
          "an observed history decision cannot steer the fresh replay");
    }
  }
  {
    auto mutated = baseline;
    check(!mutated.event_projections.empty(), "the baseline has projections");
    if (!mutated.event_projections.empty()) {
      ++mutated.event_projections.front().history_group_record_index;
      rejects(std::move(mutated), "the verifier rejects a projection mutation");
    }
  }
  {
    auto mutated = baseline;
    check(!mutated.history_label_slots.empty(), "the baseline has label slots");
    if (!mutated.history_label_slots.empty()) {
      mutated.history_label_slots.front().event_projection_index.reset();
      rejects(std::move(mutated), "the verifier rejects a label-slot mutation");
    }
  }
  {
    auto mutated = baseline;
    check(!mutated.group_overlays.empty(), "the baseline has group overlays");
    if (!mutated.group_overlays.empty()) {
      mutated.group_overlays.front().has_catalog_h0_provenance =
          !mutated.group_overlays.front().has_catalog_h0_provenance;
      rejects(std::move(mutated), "the verifier rejects a group-overlay mutation");
    }
  }
  {
    auto mutated = baseline;
    mutated.every_catalog_h0_role_projected_exactly_once = false;
    rejects(std::move(mutated), "the verifier rejects a result-fact mutation");
  }
  {
    auto mutated = baseline;
    ++mutated.counters.event_projection_count;
    rejects(std::move(mutated), "the verifier rejects a counter mutation");
  }
  {
    auto mutated = baseline;
    mutated.decision = ExactCriticalCatalogReducedGammaOverlayDecision::
        no_overlay_relevant_extra_shell_degeneracy;
    rejects(
        std::move(mutated),
        "an observed top-level decision cannot steer the fresh replay");
  }
  {
    auto mutated = baseline;
    mutated.scope = ExactCriticalCatalogReducedGammaOverlayScope::unspecified;
    rejects(std::move(mutated), "the verifier rejects a scope mutation");
  }

  const std::array<CertifiedPoint3, 3> twin_input{
      point(-2.0, 0.0), point(0.0, 1.25), point(2.0, 0.0)};
  const CanonicalPointCloud twin_cloud = canonical_cloud(twin_input);
  const auto twin_verification =
      verify_exact_critical_catalog_reduced_gamma_overlay(
          twin_cloud, 2U, budget, baseline);
  check(
      !twin_verification.fresh_replay_certified &&
          !twin_verification.
               exact_critical_catalog_reduced_gamma_overlay_decision_certified,
      "the fresh verifier rejects a same-cardinality twin cloud");
}

}  // namespace

int main() {
  test_default_invalid_domain_and_caps();
  test_overlay_preflight_is_atomic_in_every_dimension();
  test_subordinate_fail_closed_decisions();
  test_relevant_extra_shell_blocks_before_history();
  test_triangle_generic_overlay();
  test_e5_exact_provenance_and_residuals();
  test_two_saddles_share_one_ternary_multifusion();
  test_fresh_verifier_rejects_every_layer();

  if (failures != 0) {
    std::cerr << failures
              << " critical-catalog reduced-Gamma overlay test(s) failed\n";
    return 1;
  }
  return 0;
}
