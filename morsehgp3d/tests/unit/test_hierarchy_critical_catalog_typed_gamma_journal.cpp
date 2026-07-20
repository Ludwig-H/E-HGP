#include "morsehgp3d/hierarchy/critical_catalog_typed_gamma_journal.hpp"

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
using morsehgp3d::hierarchy::ExactCriticalCatalogArmGammaOverlayDecision;
using morsehgp3d::hierarchy::ExactCriticalCatalogArmGammaOverlayBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogBudget;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogReducedGammaHistoryLabelKind;
using morsehgp3d::hierarchy::ExactCriticalCatalogReducedGammaOverlayBudget;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogReducedGammaOverlayDecision;
using morsehgp3d::hierarchy::ExactCriticalCatalogTypedGammaArmRecord;
using morsehgp3d::hierarchy::ExactCriticalCatalogTypedGammaJournalBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogTypedGammaJournalDecision;
using morsehgp3d::hierarchy::ExactCriticalCatalogTypedGammaJournalResult;
using morsehgp3d::hierarchy::ExactCriticalCatalogTypedGammaJournalScope;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaJournalVerification;
using morsehgp3d::hierarchy::ExactCriticalCatalogTypedGammaLabelEntry;
using morsehgp3d::hierarchy::ExactCriticalCatalogTypedGammaLabelSemantic;
using morsehgp3d::hierarchy::ExactCriticalCatalogTypedGammaSaddleRecord;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaStrictTargetRecord;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaTerminalClassRecord;
using morsehgp3d::hierarchy::ExactFacetDescentChainBudget;
using morsehgp3d::hierarchy::
    ExactPersistentReducedGammaHistoryGroupRecord;
using morsehgp3d::hierarchy::
    ExactPersistentReducedGammaOrderHistoryBudget;
using morsehgp3d::hierarchy::ExactReducedGammaBatchGroupKind;
using morsehgp3d::hierarchy::ExactReducedGammaStrictComponentKind;
using morsehgp3d::hierarchy::ExactStrictGammaBudget;
using morsehgp3d::hierarchy::
    build_exact_critical_catalog_typed_gamma_journal;
using morsehgp3d::hierarchy::
    verify_exact_critical_catalog_typed_gamma_journal;
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

[[nodiscard]] ExactPersistentReducedGammaOrderHistoryBudget
full_history_budget() {
  ExactPersistentReducedGammaOrderHistoryBudget budget;
  budget.gamma_budget = full_gamma_budget();
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

[[nodiscard]] ExactCriticalCatalogReducedGammaOverlayBudget
full_provenance_budget() {
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

[[nodiscard]] ExactCriticalCatalogArmGammaOverlayBudget full_arm_budget(
    std::size_t per_arm_chain_capacity) {
  ExactCriticalCatalogArmGammaOverlayBudget budget;
  budget.critical_catalog_budget = full_catalog_budget();
  budget.per_arm_chain_budget = {per_arm_chain_capacity};
  budget.reduced_gamma_batch_budget = full_gamma_budget();
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

[[nodiscard]] ExactCriticalCatalogTypedGammaJournalBudget full_budget(
    std::size_t per_arm_chain_capacity = 2U) {
  ExactCriticalCatalogTypedGammaJournalBudget budget;
  budget.provenance_overlay_budget = full_provenance_budget();
  budget.arm_overlay_budget = full_arm_budget(per_arm_chain_capacity);
  budget.maximum_label_entry_count =
      ExactCriticalCatalogTypedGammaJournalBudget::
          maximum_supported_label_entry_count;
  budget.maximum_saddle_record_count =
      ExactCriticalCatalogTypedGammaJournalBudget::
          maximum_supported_saddle_record_count;
  budget.maximum_terminal_class_record_count =
      ExactCriticalCatalogTypedGammaJournalBudget::
          maximum_supported_terminal_class_record_count;
  budget.maximum_arm_record_count =
      ExactCriticalCatalogTypedGammaJournalBudget::
          maximum_supported_arm_record_count;
  budget.maximum_strict_target_record_count =
      ExactCriticalCatalogTypedGammaJournalBudget::
          maximum_supported_strict_target_record_count;
  budget.maximum_terminal_class_point_id_reference_count =
      ExactCriticalCatalogTypedGammaJournalBudget::
          maximum_supported_terminal_class_point_id_reference_count;
  budget.maximum_saddle_index_reference_count =
      ExactCriticalCatalogTypedGammaJournalBudget::
          maximum_supported_saddle_index_reference_count;
  budget.maximum_target_facet_reference_count =
      ExactCriticalCatalogTypedGammaJournalBudget::
          maximum_supported_target_facet_reference_count;
  budget.maximum_target_point_id_reference_count =
      ExactCriticalCatalogTypedGammaJournalBudget::
          maximum_supported_target_point_id_reference_count;
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

[[nodiscard]] bool all_certificates_close(
    const ExactCriticalCatalogTypedGammaJournalVerification& verification) {
  return verification.requested_budget_certified &&
         verification.external_inputs_certified &&
         verification.derived_preflight_sizes_certified &&
         verification.source_decisions_certified &&
         verification.reduced_gamma_history_certified &&
         verification.label_entries_certified &&
         verification.saddle_records_certified &&
         verification.terminal_class_records_certified &&
         verification.arm_records_certified &&
         verification.strict_target_records_certified &&
         verification.result_facts_certified &&
         verification.counters_certified && verification.decision_certified &&
         verification.scope_certified && verification.fresh_replay_certified &&
         verification.
             exact_critical_catalog_typed_gamma_journal_decision_certified;
}

[[nodiscard]] bool complete_facts(
    const ExactCriticalCatalogTypedGammaJournalResult& result) {
  return result.journal_candidate_space_size_certified &&
         result.journal_preflight_budget_sufficient &&
         result.source_budget_seam_certified &&
         result.
             subordinate_geometry_started_only_after_successful_journal_preflight &&
         result.provenance_overlay_fresh_replay_certified &&
         result.arm_overlay_started_only_after_complete_provenance &&
         result.arm_overlay_fresh_replay_certified &&
         result.source_catalogs_identical && result.source_objects_transient &&
         result.all_history_label_slots_typed_exactly_once &&
         result.catalog_births_are_deferred_facets_without_saddles_or_arms &&
         result.residual_labels_typed_only_from_history_kind &&
         result.catalog_saddles_are_non_deferred_equal_level_cofaces &&
         result.every_catalog_saddle_has_exactly_one_record &&
         result.every_terminal_class_has_one_shared_strict_target &&
         result.every_arm_has_one_terminal_class_and_strict_target &&
         result.every_source_family_arm_class_and_target_used_exactly_once &&
         result.arm_initial_facets_derived_from_closed_labels &&
         result.full_pi0_witnesses_retain_target_authority &&
         result.reduced_component_and_group_kinds_are_annotations_only &&
         result.all_saddle_targets_join_their_history_group &&
         result.reduced_gamma_history_stored_fresh &&
         result.diagnostic_outcomes_have_no_journal_or_history &&
         result.critical_catalog_typed_gamma_journal_certified &&
         result.decision ==
             ExactCriticalCatalogTypedGammaJournalDecision::
                 complete_exhaustive_typed_gamma_journal &&
         result.scope ==
             ExactCriticalCatalogTypedGammaJournalScope::
                 bounded_n14_k10_single_order_exhaustive_gamma_groups_typed_catalog_h0_roles_and_strict_full_pi0_arm_targets_with_separate_hgp_reduced_effect_annotations_only;
}

void check_empty_payload(
    const ExactCriticalCatalogTypedGammaJournalResult& result,
    const std::string& label) {
  check(
      !result.reduced_gamma_history.has_value() &&
          result.label_entries.empty() && result.saddle_records.empty() &&
          result.terminal_class_records.empty() && result.arm_records.empty() &&
          result.strict_target_records.empty(),
      label + ": no history or partial typed payload is published");
}

[[nodiscard]] const ExactPersistentReducedGammaHistoryGroupRecord*
history_group_for_entry(
    const ExactCriticalCatalogTypedGammaJournalResult& result,
    const ExactCriticalCatalogTypedGammaLabelEntry& entry) {
  if (!result.reduced_gamma_history.has_value() ||
      entry.history_group_record_index >=
          result.reduced_gamma_history->group_records.size()) {
    return nullptr;
  }
  const auto& group = result.reduced_gamma_history
                          ->group_records[entry.history_group_record_index];
  return group.group_record_index == entry.history_group_record_index &&
                 group.batch_index == entry.history_batch_index
             ? &group
             : nullptr;
}

[[nodiscard]] const PointLabel* label_for_entry(
    const ExactCriticalCatalogTypedGammaJournalResult& result,
    const ExactCriticalCatalogTypedGammaLabelEntry& entry) {
  const auto* group = history_group_for_entry(result, entry);
  if (group == nullptr) {
    return nullptr;
  }
  if (entry.history_label_kind ==
      ExactCriticalCatalogReducedGammaHistoryLabelKind::newly_active_facet) {
    return entry.history_group_local_label_index <
                   group->newly_active_facet_point_ids.size()
               ? &group->newly_active_facet_point_ids
                      [entry.history_group_local_label_index]
               : nullptr;
  }
  return entry.history_group_local_label_index <
                 group->equal_level_coface_point_ids.size()
             ? &group->equal_level_coface_point_ids
                    [entry.history_group_local_label_index]
             : nullptr;
}

[[nodiscard]] const ExactCriticalCatalogTypedGammaLabelEntry*
entry_with_label_and_semantic(
    const ExactCriticalCatalogTypedGammaJournalResult& result,
    const PointLabel& label,
    ExactCriticalCatalogTypedGammaLabelSemantic semantic) {
  const auto iterator = std::find_if(
      result.label_entries.begin(),
      result.label_entries.end(),
      [&](const ExactCriticalCatalogTypedGammaLabelEntry& entry) {
        const PointLabel* observed = label_for_entry(result, entry);
        return observed != nullptr && *observed == label &&
               entry.semantic == semantic;
      });
  return iterator == result.label_entries.end() ? nullptr : &*iterator;
}

[[nodiscard]] const ExactCriticalCatalogTypedGammaSaddleRecord*
saddle_with_closed_label(
    const ExactCriticalCatalogTypedGammaJournalResult& result,
    const PointLabel& closed_label) {
  const auto* entry = entry_with_label_and_semantic(
      result,
      closed_label,
      ExactCriticalCatalogTypedGammaLabelSemantic::catalog_saddle);
  if (entry == nullptr || !entry->saddle_record_index.has_value() ||
      *entry->saddle_record_index >= result.saddle_records.size()) {
    return nullptr;
  }
  const auto& saddle = result.saddle_records[*entry->saddle_record_index];
  return saddle.label_entry_index == entry->label_entry_index ? &saddle
                                                               : nullptr;
}

[[nodiscard]] const ExactCriticalCatalogTypedGammaArmRecord*
arm_for_saddle_and_removed(
    const ExactCriticalCatalogTypedGammaJournalResult& result,
    const ExactCriticalCatalogTypedGammaSaddleRecord* saddle,
    PointId removed_shell_point_id) {
  if (saddle == nullptr) {
    return nullptr;
  }
  const auto iterator = std::find_if(
      saddle->arm_record_indices.begin(),
      saddle->arm_record_indices.end(),
      [&](std::size_t arm_record_index) {
        return arm_record_index < result.arm_records.size() &&
               result.arm_records[arm_record_index].removed_shell_point_id ==
                   removed_shell_point_id;
      });
  return iterator == saddle->arm_record_indices.end()
             ? nullptr
             : &result.arm_records[*iterator];
}

[[nodiscard]] const ExactCriticalCatalogTypedGammaStrictTargetRecord*
target_for_arm(
    const ExactCriticalCatalogTypedGammaJournalResult& result,
    const ExactCriticalCatalogTypedGammaArmRecord* arm) {
  return arm != nullptr &&
                 arm->strict_target_record_index <
                     result.strict_target_records.size()
             ? &result.strict_target_records[arm->strict_target_record_index]
             : nullptr;
}

void test_defaults_domain_and_declared_caps() {
  const ExactCriticalCatalogTypedGammaJournalResult defaults;
  check(
      defaults.decision ==
              ExactCriticalCatalogTypedGammaJournalDecision::not_certified &&
          defaults.scope ==
              ExactCriticalCatalogTypedGammaJournalScope::unspecified &&
          defaults.provenance_overlay_decision ==
              ExactCriticalCatalogReducedGammaOverlayDecision::not_certified &&
          defaults.arm_overlay_decision ==
              ExactCriticalCatalogArmGammaOverlayDecision::not_certified,
      "default typed-journal states are explicitly uncertified");
  check_empty_payload(defaults, "default typed journal");

  const CanonicalPointCloud q2 = q2_triangle_cloud();
  const ExactCriticalCatalogTypedGammaJournalBudget budget = full_budget(1U);
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_critical_catalog_typed_gamma_journal(
            q2, 1U, budget));
      },
      "the typed journal rejects order one");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_critical_catalog_typed_gamma_journal(
            q2, 3U, budget));
      },
      "the typed journal rejects the terminal order");

  const std::array<CertifiedPoint3, 2> two_points{
      point(0.0), point(1.0)};
  const CanonicalPointCloud two_point_cloud = canonical_cloud(two_points);
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_critical_catalog_typed_gamma_journal(
            two_point_cloud, 2U, budget));
      },
      "the typed journal rejects clouds below three points");

  using BudgetMember =
      std::size_t ExactCriticalCatalogTypedGammaJournalBudget::*;
  struct CapCase {
    const char* name;
    BudgetMember member;
    std::size_t cap;
  };
  const std::array<CapCase, 9> cap_cases{{
      {"label entry",
       &ExactCriticalCatalogTypedGammaJournalBudget::maximum_label_entry_count,
       ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_supported_label_entry_count},
      {"saddle record",
       &ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_saddle_record_count,
       ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_supported_saddle_record_count},
      {"terminal class",
       &ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_terminal_class_record_count,
       ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_supported_terminal_class_record_count},
      {"arm record",
       &ExactCriticalCatalogTypedGammaJournalBudget::maximum_arm_record_count,
       ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_supported_arm_record_count},
      {"strict target",
       &ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_strict_target_record_count,
       ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_supported_strict_target_record_count},
      {"terminal-class PointId reference",
       &ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_terminal_class_point_id_reference_count,
       ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_supported_terminal_class_point_id_reference_count},
      {"saddle index reference",
       &ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_saddle_index_reference_count,
       ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_supported_saddle_index_reference_count},
      {"target facet reference",
       &ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_target_facet_reference_count,
       ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_supported_target_facet_reference_count},
      {"target PointId reference",
       &ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_target_point_id_reference_count,
       ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_supported_target_point_id_reference_count},
  }};
  for (const CapCase& cap_case : cap_cases) {
    ExactCriticalCatalogTypedGammaJournalBudget invalid = budget;
    invalid.*(cap_case.member) = cap_case.cap + 1U;
    check_throws<std::invalid_argument>(
        [&] {
          static_cast<void>(build_exact_critical_catalog_typed_gamma_journal(
              q2, 2U, invalid));
        },
        std::string{"the typed journal rejects an oversized "} +
            cap_case.name + " cap");
  }
}

void test_preflight_atomicity_and_budget_seams(
    const CanonicalPointCloud& cloud,
    const ExactCriticalCatalogTypedGammaJournalBudget& budget,
    const ExactCriticalCatalogTypedGammaJournalResult& successful) {
  check(
      successful.required_label_entry_capacity == 4U &&
          successful.required_saddle_record_capacity == 4U &&
          successful.required_terminal_class_record_capacity == 16U &&
          successful.required_arm_record_capacity == 16U &&
          successful.required_strict_target_record_capacity == 16U &&
          successful.required_terminal_class_point_id_reference_capacity ==
              48U &&
          successful.required_saddle_index_reference_capacity == 32U &&
          successful.required_target_facet_reference_capacity == 12U &&
          successful.required_target_point_id_reference_capacity == 56U,
      "q2 derives the nine exact closed typed-journal preflight bounds");

  using BudgetMember =
      std::size_t ExactCriticalCatalogTypedGammaJournalBudget::*;
  using ResultMember =
      std::size_t ExactCriticalCatalogTypedGammaJournalResult::*;
  struct Dimension {
    const char* name;
    BudgetMember maximum;
    ResultMember required;
  };
  const std::array<Dimension, 9> dimensions{{
      {"label entries",
       &ExactCriticalCatalogTypedGammaJournalBudget::maximum_label_entry_count,
       &ExactCriticalCatalogTypedGammaJournalResult::
           required_label_entry_capacity},
      {"saddle records",
       &ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_saddle_record_count,
       &ExactCriticalCatalogTypedGammaJournalResult::
           required_saddle_record_capacity},
      {"terminal classes",
       &ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_terminal_class_record_count,
       &ExactCriticalCatalogTypedGammaJournalResult::
           required_terminal_class_record_capacity},
      {"arm records",
       &ExactCriticalCatalogTypedGammaJournalBudget::maximum_arm_record_count,
       &ExactCriticalCatalogTypedGammaJournalResult::
           required_arm_record_capacity},
      {"strict targets",
       &ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_strict_target_record_count,
       &ExactCriticalCatalogTypedGammaJournalResult::
           required_strict_target_record_capacity},
      {"terminal-class PointId references",
       &ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_terminal_class_point_id_reference_count,
       &ExactCriticalCatalogTypedGammaJournalResult::
           required_terminal_class_point_id_reference_capacity},
      {"saddle index references",
       &ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_saddle_index_reference_count,
       &ExactCriticalCatalogTypedGammaJournalResult::
           required_saddle_index_reference_capacity},
      {"target facet references",
       &ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_target_facet_reference_count,
       &ExactCriticalCatalogTypedGammaJournalResult::
           required_target_facet_reference_capacity},
      {"target PointId references",
       &ExactCriticalCatalogTypedGammaJournalBudget::
           maximum_target_point_id_reference_count,
       &ExactCriticalCatalogTypedGammaJournalResult::
           required_target_point_id_reference_capacity},
  }};
  for (const Dimension& dimension : dimensions) {
    ExactCriticalCatalogTypedGammaJournalBudget insufficient = budget;
    insufficient.*(dimension.maximum) =
        successful.*(dimension.required) - 1U;
    const ExactCriticalCatalogTypedGammaJournalResult result =
        build_exact_critical_catalog_typed_gamma_journal(
            cloud, 2U, insufficient);
    check(
        result.journal_candidate_space_size_certified &&
            !result.journal_preflight_budget_sufficient &&
            result.counters.preflight_count == 1U &&
            result.counters.provenance_overlay_build_count == 0U &&
            result.counters.arm_overlay_build_count == 0U &&
            result.decision ==
                ExactCriticalCatalogTypedGammaJournalDecision::
                    no_journal_preflight_budget_insufficient,
        std::string{dimension.name} +
            " fail atomically before both source overlays");
    check_empty_payload(result, dimension.name);
    check(
        all_certificates_close(
            verify_exact_critical_catalog_typed_gamma_journal(
                cloud, 2U, insufficient, result)),
        std::string{dimension.name} +
            " diagnostic is accepted by fresh replay");
  }

  const auto check_seam_failure = [&](const auto& mutate,
                                      const std::string& label) {
    ExactCriticalCatalogTypedGammaJournalBudget mismatched = budget;
    mutate(mismatched);
    const ExactCriticalCatalogTypedGammaJournalResult result =
        build_exact_critical_catalog_typed_gamma_journal(
            cloud, 2U, mismatched);
    check(
        result.journal_candidate_space_size_certified &&
            !result.source_budget_seam_certified &&
            result.counters.provenance_overlay_build_count == 0U &&
            result.counters.arm_overlay_build_count == 0U &&
            result.decision ==
                ExactCriticalCatalogTypedGammaJournalDecision::
                    no_journal_preflight_budget_insufficient,
        label + " is rejected before source geometry");
    check_empty_payload(result, label);
    check(
        all_certificates_close(
            verify_exact_critical_catalog_typed_gamma_journal(
                cloud, 2U, mismatched, result)),
        label + " diagnostic is freshly replayable");
  };
  check_seam_failure(
      [](ExactCriticalCatalogTypedGammaJournalBudget& mismatched) {
        --mismatched.arm_overlay_budget.critical_catalog_budget
              .maximum_candidate_count;
      },
      "unequal catalogue budgets");
  check_seam_failure(
      [](ExactCriticalCatalogTypedGammaJournalBudget& mismatched) {
        --mismatched.arm_overlay_budget.reduced_gamma_batch_budget
              .maximum_enumerated_facet_count;
      },
      "unequal Gamma budgets");
}

void test_q2_complete_typed_journal(
    const CanonicalPointCloud& cloud,
    const ExactCriticalCatalogTypedGammaJournalBudget& budget,
    const ExactCriticalCatalogTypedGammaJournalResult& result) {
  const ExactCriticalCatalogTypedGammaJournalVerification verification =
      verify_exact_critical_catalog_typed_gamma_journal(
          cloud, 2U, budget, result);

  check(
      complete_facts(result) && all_certificates_close(verification) &&
          result.reduced_gamma_history.has_value(),
      "q2 closes the complete fresh typed journal");
  check(
      result.exhaustive_facet_count == 3U &&
          result.exhaustive_coface_count == 1U &&
          result.reduced_gamma_history.has_value() &&
          result.reduced_gamma_history->group_records.size() == 3U &&
          result.label_entries.size() == 4U &&
          result.saddle_records.size() == 1U &&
          result.terminal_class_records.size() == 2U &&
          result.arm_records.size() == 2U &&
          result.strict_target_records.size() == 2U,
      "q2 retains one history, four typed labels, one saddle, two classes, two arms and two targets");
  check(
      result.counters.provenance_overlay_build_count == 1U &&
          result.counters.provenance_overlay_verification_count == 1U &&
          result.counters.arm_overlay_build_count == 1U &&
          result.counters.arm_overlay_verification_count == 1U &&
          result.counters.source_catalog_comparison_count == 1U &&
          result.counters.history_verification_count == 1U &&
          result.counters.label_entry_count == 4U &&
          result.counters.catalog_birth_label_count == 2U &&
          result.counters.catalog_saddle_label_count == 1U &&
          result.counters.residual_newly_active_facet_label_count == 1U &&
          result.counters.residual_equal_level_coface_label_count == 0U &&
          result.counters.saddle_record_count == 1U &&
          result.counters.terminal_class_record_count == 2U &&
          result.counters.terminal_class_point_id_reference_count == 6U &&
          result.counters.arm_record_count == 2U &&
          result.counters.saddle_index_reference_count == 4U &&
          result.counters.strict_target_record_count == 2U &&
          result.counters.target_facet_reference_count == 2U &&
          result.counters.target_point_id_reference_count == 8U,
      "q2 counters expose one transient build of each source and the exact compact payload");

  const auto* birth01 = entry_with_label_and_semantic(
      result,
      {0U, 1U},
      ExactCriticalCatalogTypedGammaLabelSemantic::catalog_birth);
  const auto* birth12 = entry_with_label_and_semantic(
      result,
      {1U, 2U},
      ExactCriticalCatalogTypedGammaLabelSemantic::catalog_birth);
  const auto* residual02 = entry_with_label_and_semantic(
      result,
      {0U, 2U},
      ExactCriticalCatalogTypedGammaLabelSemantic::
          residual_newly_active_facet);
  const auto* saddle012 = entry_with_label_and_semantic(
      result,
      {0U, 1U, 2U},
      ExactCriticalCatalogTypedGammaLabelSemantic::catalog_saddle);
  const auto* birth01_group =
      birth01 == nullptr ? nullptr : history_group_for_entry(result, *birth01);
  const auto* birth12_group =
      birth12 == nullptr ? nullptr : history_group_for_entry(result, *birth12);
  const auto* residual_group = residual02 == nullptr
                                   ? nullptr
                                   : history_group_for_entry(result, *residual02);
  const auto* saddle_group = saddle012 == nullptr
                                 ? nullptr
                                 : history_group_for_entry(result, *saddle012);
  check(
      birth01 != nullptr && birth12 != nullptr && residual02 != nullptr &&
          saddle012 != nullptr &&
          birth01->history_label_kind ==
              ExactCriticalCatalogReducedGammaHistoryLabelKind::
                  newly_active_facet &&
          birth12->history_label_kind ==
              ExactCriticalCatalogReducedGammaHistoryLabelKind::
                  newly_active_facet &&
          residual02->history_label_kind ==
              ExactCriticalCatalogReducedGammaHistoryLabelKind::
                  newly_active_facet &&
          saddle012->history_label_kind ==
              ExactCriticalCatalogReducedGammaHistoryLabelKind::
                  equal_level_coface,
      "q2 partitions the four history labels into two births, one saddle and one residual facet");
  check(
      birth01 != nullptr && birth12 != nullptr &&
          birth01->source_event_projection_index.has_value() &&
          birth01->catalog_event_index.has_value() &&
          birth01->catalog_h0_batch_index.has_value() &&
          !birth01->saddle_record_index.has_value() &&
          birth12->source_event_projection_index.has_value() &&
          birth12->catalog_event_index.has_value() &&
          birth12->catalog_h0_batch_index.has_value() &&
          !birth12->saddle_record_index.has_value() &&
          birth01_group != nullptr && birth12_group != nullptr &&
          birth01_group->kind ==
              ExactReducedGammaBatchGroupKind::deferred_isolated_facet &&
          birth12_group->kind ==
              ExactReducedGammaBatchGroupKind::deferred_isolated_facet,
      "catalogue births retain H0 provenance but no saddle and remain deferred reduced facets");
  check(
      residual02 != nullptr &&
          !residual02->source_event_projection_index.has_value() &&
          !residual02->catalog_event_index.has_value() &&
          !residual02->catalog_h0_batch_index.has_value() &&
          !residual02->saddle_record_index.has_value() &&
          residual_group != nullptr && residual_group == saddle_group &&
          residual_group->squared_level == level(4),
      "the level-four residual facet remains in the saddle group without invented H0 provenance");
  check(
      saddle012 != nullptr &&
          saddle012->source_event_projection_index.has_value() &&
          saddle012->catalog_event_index.has_value() &&
          saddle012->catalog_h0_batch_index.has_value() &&
          saddle012->saddle_record_index == 0U && saddle_group != nullptr &&
          saddle_group->kind == ExactReducedGammaBatchGroupKind::birth,
      "the H0 saddle remains a saddle while its independent reduced-group effect is birth");

  const ExactCriticalCatalogTypedGammaSaddleRecord* saddle =
      saddle_with_closed_label(result, {0U, 1U, 2U});
  check(
      saddle != nullptr && saddle012 != nullptr &&
          saddle->terminal_class_record_indices.size() == 2U &&
          saddle->arm_record_indices.size() == 2U &&
          saddle->reduced_group_kind ==
              ExactReducedGammaBatchGroupKind::birth &&
          saddle->history_group_record_index ==
              saddle012->history_group_record_index,
      "q2 factorizes the unique saddle into two classes and two arms inside one history group");

  const ExactCriticalCatalogTypedGammaArmRecord* removed_zero =
      arm_for_saddle_and_removed(result, saddle, 0U);
  const ExactCriticalCatalogTypedGammaArmRecord* removed_two =
      arm_for_saddle_and_removed(result, saddle, 2U);
  const auto* target_zero = target_for_arm(result, removed_zero);
  const auto* target_two = target_for_arm(result, removed_two);
  check(
      removed_zero != nullptr && removed_two != nullptr &&
          removed_zero->terminal_class_record_index !=
              removed_two->terminal_class_record_index &&
          removed_zero->strict_target_record_index !=
              removed_two->strict_target_record_index,
      "the two q2 shell removals retain distinct terminal classes and strict targets");

  const std::set<PointLabel> expected_targets{{0U, 1U}, {1U, 2U}};
  std::set<PointLabel> observed_targets;
  bool targets_are_full_isolated_singletons = true;
  for (const ExactCriticalCatalogTypedGammaStrictTargetRecord& target :
       result.strict_target_records) {
    targets_are_full_isolated_singletons =
        targets_are_full_isolated_singletons &&
        saddle != nullptr &&
        target.history_group_record_index ==
            saddle->history_group_record_index &&
        target.reduced_component_kind ==
            ExactReducedGammaStrictComponentKind::omitted_isolated_facet &&
        target.strict_component.facet_point_ids.size() == 1U &&
        target.strict_component.canonical_representative_facet_point_ids ==
            target.strict_component.facet_point_ids.front();
    if (target.strict_component.facet_point_ids.size() == 1U) {
      observed_targets.insert(target.strict_component.facet_point_ids.front());
    }
  }
  check(
      target_zero != nullptr && target_two != nullptr &&
          targets_are_full_isolated_singletons &&
          observed_targets == expected_targets,
      "q2 retains two complete strict full-pi0 witnesses and only separate reduced annotations");
}

void test_source_failures_are_atomic() {
  const CanonicalPointCloud q2 = q2_triangle_cloud();

  ExactCriticalCatalogTypedGammaJournalBudget provenance_insufficient =
      full_budget(1U);
  provenance_insufficient.provenance_overlay_budget
      .maximum_label_slot_count = 3U;
  const ExactCriticalCatalogTypedGammaJournalResult no_provenance =
      build_exact_critical_catalog_typed_gamma_journal(
          q2, 2U, provenance_insufficient);
  check(
      no_provenance.journal_preflight_budget_sufficient &&
          no_provenance.source_budget_seam_certified &&
          no_provenance.provenance_overlay_decision ==
              ExactCriticalCatalogReducedGammaOverlayDecision::
                  no_overlay_preflight_budget_insufficient &&
          no_provenance.arm_overlay_decision ==
              ExactCriticalCatalogArmGammaOverlayDecision::not_certified &&
          no_provenance.counters.provenance_overlay_build_count == 1U &&
          no_provenance.counters.arm_overlay_build_count == 0U &&
          no_provenance.decision ==
              ExactCriticalCatalogTypedGammaJournalDecision::
                  no_journal_provenance_overlay_incomplete,
      "an incomplete provenance overlay blocks the arm overlay and typed journal");
  check_empty_payload(no_provenance, "provenance overlay failure");
  check(
      all_certificates_close(
          verify_exact_critical_catalog_typed_gamma_journal(
              q2, 2U, provenance_insufficient, no_provenance)),
      "the provenance-overlay diagnostic is freshly replayable");

  ExactCriticalCatalogTypedGammaJournalBudget arm_insufficient =
      full_budget(1U);
  arm_insufficient.arm_overlay_budget.maximum_target_component_count = 15U;
  const ExactCriticalCatalogTypedGammaJournalResult no_arm =
      build_exact_critical_catalog_typed_gamma_journal(
          q2, 2U, arm_insufficient);
  check(
      no_arm.provenance_overlay_decision ==
              ExactCriticalCatalogReducedGammaOverlayDecision::
                  complete_exhaustive_critical_catalog_reduced_gamma_overlay &&
          no_arm.arm_overlay_decision ==
              ExactCriticalCatalogArmGammaOverlayDecision::
                  no_overlay_preflight_budget_insufficient &&
          no_arm.counters.provenance_overlay_build_count == 1U &&
          no_arm.counters.arm_overlay_build_count == 1U &&
          no_arm.decision ==
              ExactCriticalCatalogTypedGammaJournalDecision::
                  no_journal_arm_overlay_incomplete,
      "an incomplete arm-Gamma overlay follows complete provenance but publishes no journal");
  check_empty_payload(no_arm, "arm-Gamma overlay failure");

  const CanonicalPointCloud transverse = critical_arm_cloud();
  const ExactCriticalCatalogTypedGammaJournalBudget zero_chain_budget =
      full_budget(0U);
  const ExactCriticalCatalogTypedGammaJournalResult incomplete_arm =
      build_exact_critical_catalog_typed_gamma_journal(
          transverse, 2U, zero_chain_budget);
  check(
      incomplete_arm.provenance_overlay_decision ==
              ExactCriticalCatalogReducedGammaOverlayDecision::
                  complete_exhaustive_critical_catalog_reduced_gamma_overlay &&
          incomplete_arm.arm_overlay_decision ==
              ExactCriticalCatalogArmGammaOverlayDecision::
                  no_overlay_incomplete_critical_arm_family &&
          incomplete_arm.decision ==
              ExactCriticalCatalogTypedGammaJournalDecision::
                  no_journal_arm_overlay_incomplete,
      "the transverse critical-arm fixture stops atomically at chain budget zero");
  check_empty_payload(incomplete_arm, "incomplete transverse arm family");
}

void test_mirror_simultaneous_saddles_share_one_target() {
  const CanonicalPointCloud cloud = mirror_cloud();
  const ExactCriticalCatalogTypedGammaJournalBudget budget = full_budget(1U);
  const ExactCriticalCatalogTypedGammaJournalResult result =
      build_exact_critical_catalog_typed_gamma_journal(cloud, 2U, budget);

  const ExactCriticalCatalogTypedGammaSaddleRecord* lower =
      saddle_with_closed_label(result, {0U, 1U, 3U});
  const ExactCriticalCatalogTypedGammaSaddleRecord* upper =
      saddle_with_closed_label(result, {0U, 2U, 3U});
  check(
      complete_facts(result) && result.saddle_records.size() == 2U &&
          result.terminal_class_records.size() == 6U &&
          result.arm_records.size() == 6U &&
          result.strict_target_records.size() == 5U &&
          result.counters.residual_equal_level_coface_label_count == 2U &&
          std::count_if(
              result.label_entries.begin(),
              result.label_entries.end(),
              [](const ExactCriticalCatalogTypedGammaLabelEntry& entry) {
                return entry.semantic ==
                       ExactCriticalCatalogTypedGammaLabelSemantic::
                           residual_equal_level_coface;
              }) == 2,
      "the mirror journal retains two saddles, six classes, six arms and five deduplicated targets");
  check(
      lower != nullptr && upper != nullptr &&
          lower->catalog_h0_batch_index == upper->catalog_h0_batch_index &&
          lower->history_batch_index == upper->history_batch_index &&
          lower->history_group_record_index ==
              upper->history_group_record_index &&
          lower->reduced_group_kind ==
              ExactReducedGammaBatchGroupKind::birth &&
          upper->reduced_group_kind ==
              ExactReducedGammaBatchGroupKind::birth &&
          lower->terminal_class_record_indices.size() == 3U &&
          upper->terminal_class_record_indices.size() == 3U &&
          lower->arm_record_indices.size() == 3U &&
          upper->arm_record_indices.size() == 3U,
      "the two mirror Morse saddles stay simultaneous in one independent reduced-birth group");
  if (lower != nullptr && result.reduced_gamma_history.has_value() &&
      lower->history_group_record_index <
          result.reduced_gamma_history->group_records.size()) {
    check(
        result.reduced_gamma_history
                ->group_records[lower->history_group_record_index]
                .squared_level == level(169, 36),
        "the simultaneous mirror group retains its exact level 169/36");
  }

  const std::set<PointLabel> expected_singletons{
      {0U, 1U}, {0U, 2U}, {0U, 3U}, {1U, 3U}, {2U, 3U}};
  std::set<PointLabel> observed_singletons;
  bool every_target_is_an_isolated_full_witness = true;
  for (const ExactCriticalCatalogTypedGammaStrictTargetRecord& target :
       result.strict_target_records) {
    every_target_is_an_isolated_full_witness =
        every_target_is_an_isolated_full_witness &&
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
      every_target_is_an_isolated_full_witness &&
          observed_singletons == expected_singletons,
      "the mirror target arena preserves all five strict full-pi0 singleton witnesses");

  const ExactCriticalCatalogTypedGammaArmRecord* lower_shared =
      arm_for_saddle_and_removed(result, lower, 1U);
  const ExactCriticalCatalogTypedGammaArmRecord* upper_shared =
      arm_for_saddle_and_removed(result, upper, 2U);
  const auto* shared_target = target_for_arm(result, lower_shared);
  check(
      lower_shared != nullptr && upper_shared != nullptr &&
          lower_shared->strict_target_record_index ==
              upper_shared->strict_target_record_index &&
          shared_target != nullptr &&
          shared_target->strict_component.facet_point_ids ==
              std::vector<PointLabel>{{0U, 3U}} &&
          result.counters.shared_strict_target_arm_count == 1U,
      "two distinct simultaneous saddles share exactly the strict target {0,3} without merging roles");
}

void test_shared_terminal_continuation_keeps_target_types_separate() {
  const CanonicalPointCloud cloud = shared_terminal_cloud();
  const ExactCriticalCatalogTypedGammaJournalBudget budget = full_budget(2U);
  const ExactCriticalCatalogTypedGammaJournalResult result =
      build_exact_critical_catalog_typed_gamma_journal(cloud, 3U, budget);
  const ExactCriticalCatalogTypedGammaSaddleRecord* saddle =
      saddle_with_closed_label(result, {0U, 1U, 2U, 3U});
  const ExactCriticalCatalogTypedGammaArmRecord* removed_one =
      arm_for_saddle_and_removed(result, saddle, 1U);
  const ExactCriticalCatalogTypedGammaArmRecord* removed_two =
      arm_for_saddle_and_removed(result, saddle, 2U);
  const ExactCriticalCatalogTypedGammaArmRecord* removed_three =
      arm_for_saddle_and_removed(result, saddle, 3U);
  const auto* prior_root = target_for_arm(result, removed_one);
  const auto* isolated = target_for_arm(result, removed_two);

  check(
      complete_facts(result) && result.saddle_records.size() == 3U &&
          result.arm_records.size() == 7U && saddle != nullptr &&
          saddle->reduced_group_kind ==
              ExactReducedGammaBatchGroupKind::continuation &&
          saddle->terminal_class_record_indices.size() == 2U &&
          saddle->arm_record_indices.size() == 3U,
      "the shared-terminal journal retains three families and types the selected saddle as a reduced continuation");
  if (saddle != nullptr && result.reduced_gamma_history.has_value() &&
      saddle->history_group_record_index <
          result.reduced_gamma_history->group_records.size()) {
    check(
        result.reduced_gamma_history
                ->group_records[saddle->history_group_record_index]
                .squared_level == level(25925, 338),
        "the selected continuation retains its exact level 25925/338");
  }
  check(
      removed_one != nullptr && removed_two != nullptr &&
          removed_three != nullptr &&
          removed_one->terminal_class_record_index ==
              removed_three->terminal_class_record_index &&
          removed_one->strict_target_record_index ==
              removed_three->strict_target_record_index &&
          removed_two->terminal_class_record_index !=
              removed_one->terminal_class_record_index &&
          removed_two->strict_target_record_index !=
              removed_one->strict_target_record_index,
      "removed shell points one and three share a class and target while point two remains distinct");

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
      "the full-pi0 targets distinguish a seven-facet prior root from an absorbed isolated facet");

  if (removed_one != nullptr && removed_two != nullptr &&
      removed_one->terminal_class_record_index <
          result.terminal_class_records.size() &&
      removed_two->terminal_class_record_index <
          result.terminal_class_records.size()) {
    const auto& shared_class = result.terminal_class_records
                                   [removed_one->terminal_class_record_index];
    const auto& isolated_class = result.terminal_class_records
                                     [removed_two->terminal_class_record_index];
    check(
        shared_class.terminal_class.canonical_terminal.facet_point_ids ==
                PointLabel({0U, 1U, 2U}) &&
            shared_class.terminal_class.removed_shell_point_ids ==
                PointLabel({1U, 3U}) &&
            isolated_class.terminal_class.canonical_terminal.facet_point_ids ==
                PointLabel({0U, 1U, 3U}) &&
            isolated_class.terminal_class.removed_shell_point_ids ==
                PointLabel({2U}),
        "terminal classes retain their canonical labels and exact shell-removal provenance");
  }
}

void test_fresh_verifier_rejects_representative_mutations(
    const CanonicalPointCloud& cloud,
    const ExactCriticalCatalogTypedGammaJournalBudget& budget,
    const ExactCriticalCatalogTypedGammaJournalResult& baseline) {
  check(
      complete_facts(baseline) && baseline.reduced_gamma_history.has_value() &&
          baseline.label_entries.size() == 4U &&
          baseline.saddle_records.size() == 1U &&
          baseline.terminal_class_records.size() == 2U &&
          baseline.arm_records.size() == 2U &&
          baseline.strict_target_records.size() == 2U,
      "the q2 mutation baseline is complete and compact");

  const bool mutation_payload_available =
      baseline.reduced_gamma_history.has_value() &&
      !baseline.reduced_gamma_history->group_records.empty() &&
      !baseline.label_entries.empty() && !baseline.saddle_records.empty() &&
      !baseline.terminal_class_records.empty() &&
      !baseline.arm_records.empty() &&
      baseline.strict_target_records.size() >= 2U;
  check(
      mutation_payload_available,
      "the mutation baseline exposes every independently compared layer");
  if (mutation_payload_available) {
    ExactCriticalCatalogTypedGammaJournalResult mutated = baseline;
    --mutated.requested_budget.maximum_label_entry_count;
    ++mutated.required_saddle_record_capacity;
    mutated.provenance_overlay_decision =
        ExactCriticalCatalogReducedGammaOverlayDecision::
            no_overlay_preflight_budget_insufficient;
    mutated.reduced_gamma_history->group_records.front().kind =
        ExactReducedGammaBatchGroupKind::birth;
    mutated.label_entries.front().semantic =
        ExactCriticalCatalogTypedGammaLabelSemantic::
            residual_newly_active_facet;
    mutated.saddle_records.front().reduced_group_kind =
        ExactReducedGammaBatchGroupKind::continuation;
    mutated.terminal_class_records.front()
        .terminal_class.canonical_terminal.facet_point_ids.clear();
    mutated.arm_records.front().strict_target_record_index =
        (mutated.arm_records.front().strict_target_record_index + 1U) %
        mutated.strict_target_records.size();
    mutated.strict_target_records.front()
        .strict_component.facet_point_ids.clear();
    mutated.full_pi0_witnesses_retain_target_authority = false;
    ++mutated.counters.shared_strict_target_arm_count;
    mutated.decision =
        ExactCriticalCatalogTypedGammaJournalDecision::
            no_journal_arm_overlay_incomplete;
    mutated.scope = ExactCriticalCatalogTypedGammaJournalScope::unspecified;

    const ExactCriticalCatalogTypedGammaJournalVerification verification =
        verify_exact_critical_catalog_typed_gamma_journal(
            cloud, 2U, budget, mutated);
    check(
        !verification.requested_budget_certified &&
            verification.external_inputs_certified &&
            !verification.derived_preflight_sizes_certified &&
            !verification.source_decisions_certified &&
            !verification.reduced_gamma_history_certified &&
            !verification.label_entries_certified &&
            !verification.saddle_records_certified &&
            !verification.terminal_class_records_certified &&
            !verification.arm_records_certified &&
            !verification.strict_target_records_certified &&
            !verification.result_facts_certified &&
            !verification.counters_certified &&
            !verification.decision_certified &&
            !verification.scope_certified &&
            !verification.fresh_replay_certified &&
            !verification.
                exact_critical_catalog_typed_gamma_journal_decision_certified,
        "one fresh replay independently rejects simultaneous mutations of every stored layer");
  }

  const std::array<CertifiedPoint3, 3> twin_input{
      point(-2.0, 0.0), point(0.0, 1.25), point(2.0, 0.0)};
  const CanonicalPointCloud twin = canonical_cloud(twin_input);
  const ExactCriticalCatalogTypedGammaJournalVerification twin_verification =
      verify_exact_critical_catalog_typed_gamma_journal(
          twin, 2U, budget, baseline);
  check(
      !twin_verification.fresh_replay_certified &&
          !twin_verification.
              exact_critical_catalog_typed_gamma_journal_decision_certified,
      "a same-size twin cloud cannot reuse the q2 typed journal");
}

}  // namespace

int main() {
  test_defaults_domain_and_declared_caps();
  const CanonicalPointCloud q2 = q2_triangle_cloud();
  const ExactCriticalCatalogTypedGammaJournalBudget q2_budget =
      full_budget(1U);
  const ExactCriticalCatalogTypedGammaJournalResult q2_baseline =
      build_exact_critical_catalog_typed_gamma_journal(
          q2, 2U, q2_budget);
  test_preflight_atomicity_and_budget_seams(
      q2, q2_budget, q2_baseline);
  test_q2_complete_typed_journal(q2, q2_budget, q2_baseline);
  test_source_failures_are_atomic();
  test_mirror_simultaneous_saddles_share_one_target();
  test_shared_terminal_continuation_keeps_target_types_separate();
  test_fresh_verifier_rejects_representative_mutations(
      q2, q2_budget, q2_baseline);
  if (failures != 0) {
    std::cerr << failures << " typed Gamma journal test(s) failed\n";
    return 1;
  }
  std::cout << "all typed Gamma journal tests passed\n";
  return 0;
}
