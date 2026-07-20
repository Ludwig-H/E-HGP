#include "morsehgp3d/hierarchy/critical_catalog_typed_gamma_arm_root_path_overlay.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::ExactCriticalCatalogArmGammaOverlayBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogReducedGammaOverlayBudget;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaArmRootCompositionBudget;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaArmRootCompositionResult;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaArmRootPathOverlayDecision;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaArmRootPathOverlayResult;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaArmRootPathOverlayScope;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaArmRootPathOverlayVerification;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaEventLocalArmRootPathRecord;
using morsehgp3d::hierarchy::ExactCriticalCatalogTypedGammaJournalBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogTypedGammaJournalResult;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaRootOverlayBudget;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaRootOverlayResult;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaTargetDisposition;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaOrderHistoryBudget;
using morsehgp3d::hierarchy::ExactStrictGammaBudget;
using morsehgp3d::hierarchy::
    build_exact_critical_catalog_typed_gamma_arm_root_composition;
using morsehgp3d::hierarchy::
    build_exact_critical_catalog_typed_gamma_arm_root_path_overlay;
using morsehgp3d::hierarchy::
    build_exact_critical_catalog_typed_gamma_journal;
using morsehgp3d::hierarchy::
    build_exact_critical_catalog_typed_gamma_root_overlay;
using morsehgp3d::hierarchy::
    verify_exact_critical_catalog_typed_gamma_arm_root_path_overlay;
using morsehgp3d::spatial::CanonicalPointCloud;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Function>
void check_invalid_argument(Function&& function, const std::string& message) {
  bool rejected = false;
  try {
    function();
  } catch (const std::invalid_argument&) {
    rejected = true;
  } catch (...) {
  }
  check(rejected, message);
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

[[nodiscard]] CanonicalPointCloud q2_triangle_cloud() {
  const std::array<CertifiedPoint3, 3> input{
      point(-2.0, 0.0), point(0.0, 1.0), point(2.0, 0.0)};
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud line_cloud(std::size_t point_count) {
  std::vector<CertifiedPoint3> input;
  input.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    input.push_back(point(static_cast<double>(index)));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{input});
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

[[nodiscard]] ExactCriticalCatalogTypedGammaJournalBudget
full_journal_budget(std::size_t per_arm_chain_capacity = 1U) {
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

[[nodiscard]] ExactCriticalCatalogTypedGammaRootOverlayBudget
full_root_overlay_budget(
    const ExactCriticalCatalogTypedGammaJournalBudget& journal_budget) {
  ExactCriticalCatalogTypedGammaRootOverlayBudget budget;
  budget.typed_gamma_journal_budget = journal_budget;
  budget.maximum_target_root_binding_count =
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_target_root_binding_count;
  budget.maximum_live_root_state_count =
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_live_root_state_count;
  budget.maximum_live_root_facet_reference_count =
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_live_root_facet_reference_count;
  budget.maximum_live_root_point_id_reference_count =
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_live_root_point_id_reference_count;
  budget.maximum_root_facet_replay_work_count =
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_root_facet_replay_work_count;
  budget.maximum_root_point_id_replay_work_count =
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_root_point_id_replay_work_count;
  budget.maximum_target_facet_comparison_count =
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_target_facet_comparison_count;
  budget.maximum_target_point_id_comparison_count =
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_target_point_id_comparison_count;
  budget.maximum_snapshot_facet_index_count =
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_snapshot_facet_index_count;
  budget.maximum_snapshot_point_id_index_count =
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_snapshot_point_id_index_count;
  return budget;
}

[[nodiscard]] ExactCriticalCatalogTypedGammaArmRootCompositionBudget
full_composition_budget(
    const ExactCriticalCatalogTypedGammaRootOverlayBudget& root_budget) {
  ExactCriticalCatalogTypedGammaArmRootCompositionBudget budget;
  budget.root_overlay_budget = root_budget;
  budget.maximum_arm_candidate_count =
      ExactCriticalCatalogTypedGammaArmRootCompositionBudget::
          maximum_supported_arm_candidate_count;
  return budget;
}

[[nodiscard]] ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget
full_path_budget(
    const ExactCriticalCatalogTypedGammaArmRootCompositionBudget&
        composition_budget) {
  ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget budget;
  budget.arm_root_composition_budget = composition_budget;
  budget.maximum_path_record_count =
      ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget::
          maximum_supported_path_record_count;
  budget.maximum_committed_chain_segment_count =
      ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget::
          maximum_supported_committed_chain_segment_count;
  budget.maximum_composite_path_segment_count =
      ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget::
          maximum_supported_composite_path_segment_count;
  budget.maximum_chain_node_point_id_reference_count =
      ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget::
          maximum_supported_chain_node_point_id_reference_count;
  budget.maximum_exterior_constraint_count =
      ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget::
          maximum_supported_exterior_constraint_count;
  return budget;
}

[[nodiscard]] bool all_certificates_close(
    const ExactCriticalCatalogTypedGammaArmRootPathOverlayVerification&
        verification) {
  return verification.requested_budget_certified &&
         verification.external_inputs_certified &&
         verification.derived_preflight_sizes_certified &&
         verification.source_composition_decision_certified &&
         verification.source_composition_fresh_replay_certified &&
         verification.path_records_certified &&
         verification.result_facts_certified &&
         verification.counters_certified &&
         verification.decision_certified && verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.
             exact_critical_catalog_typed_gamma_arm_root_path_overlay_decision_certified;
}

[[nodiscard]] bool complete_facts(
    const ExactCriticalCatalogTypedGammaArmRootPathOverlayResult& result) {
  return result.arm_root_path_candidate_space_size_certified &&
         result.arm_root_path_preflight_budget_sufficient &&
         result.source_journal_is_external_and_not_retained &&
         result.source_root_overlay_is_external_and_not_retained &&
         result.source_composition_is_external_and_not_retained &&
         result.source_journal_budget_seam_certified &&
         result.source_root_overlay_budget_seam_certified &&
         result.source_composition_budget_seam_certified &&
         result.source_composition_fresh_replay_certified &&
         result.reconstruction_started_only_after_complete_source_composition &&
         result.transient_critical_catalog_fresh_replay_certified &&
         result.transient_critical_arm_families_fresh_replay_certified &&
         result.every_arm_candidate_has_one_dense_replayable_path &&
         result.event_saddle_arm_and_terminal_class_keys_reconciled &&
         result.exact_initial_germs_and_chain_shapes_replayable &&
         result.exact_seams_strict_paths_and_regular_terminals_certified &&
         result.
             initial_and_terminal_facets_belong_to_external_full_pi0_targets &&
         result.
             target_bindings_and_reduced_dispositions_copied_without_reclassification &&
         result.shared_targets_and_roots_preserve_distinct_paths &&
         result.records_are_event_local_internal_paths_and_not_public_attachments &&
         result.diagnostic_outcomes_have_no_paths &&
         result.
             critical_catalog_typed_gamma_arm_root_path_overlay_certified &&
         result.decision ==
             ExactCriticalCatalogTypedGammaArmRootPathOverlayDecision::
                 complete_exhaustive_event_local_replayable_arm_root_path_overlay &&
         result.scope ==
             ExactCriticalCatalogTypedGammaArmRootPathOverlayScope::
                 bounded_n14_k10_single_order_event_local_typed_critical_arms_with_replayable_strict_descent_paths_linked_to_external_full_pi0_targets_and_separate_frozen_pre_batch_local_hgp_reduced_root_or_omitted_singleton_dispositions_only;
}

void check_empty_payload(
    const ExactCriticalCatalogTypedGammaArmRootPathOverlayResult& result,
    const std::string& label) {
  check(
      result.path_records.empty(),
      label + ": no partial replayable path is published");
}

void test_q2_complete_paths(
    const CanonicalPointCloud& cloud,
    const ExactCriticalCatalogTypedGammaJournalResult& journal,
    const ExactCriticalCatalogTypedGammaRootOverlayResult& root_overlay,
    const ExactCriticalCatalogTypedGammaArmRootCompositionResult& composition,
    const ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget& budget,
    const ExactCriticalCatalogTypedGammaArmRootPathOverlayResult& result) {
  const auto verification =
      verify_exact_critical_catalog_typed_gamma_arm_root_path_overlay(
          cloud,
          2U,
          budget,
          journal,
          root_overlay,
          composition,
          result);
  check(
      complete_facts(result) && all_certificates_close(verification) &&
          std::string{
              ExactCriticalCatalogTypedGammaArmRootPathOverlayResult::
                  proof_basis} ==
              "exact_fresh_event_local_typed_critical_arm_strict_descent_"
              "paths_replayed_and_linked_to_full_pi0_targets_with_separate_"
              "local_reduced_dispositions_v1",
      "q2 closes the compact internal replayable-path certificate");
  check(
      result.point_count == 3U && result.order == 2U &&
          result.critical_event_support_bound == 4U &&
          result.critical_arm_bound == 16U &&
          result.required_path_record_capacity == 16U &&
          result.required_committed_chain_segment_capacity == 16U &&
          result.required_composite_path_segment_capacity == 32U &&
          result.required_chain_node_point_id_reference_capacity == 64U &&
          result.required_exterior_constraint_capacity == 0U &&
          result.path_records.size() == 2U,
      "q2 exposes all five exact preflight bounds and two retained paths");

  std::size_t committed_segment_count = 0U;
  std::size_t composite_segment_count = 0U;
  std::size_t point_id_reference_count = 0U;
  std::size_t exterior_constraint_count = 0U;
  bool dense_compact_rootless_paths = true;
  for (std::size_t index = 0U; index < result.path_records.size(); ++index) {
    const ExactCriticalCatalogTypedGammaEventLocalArmRootPathRecord& record =
        result.path_records[index];
    const auto& candidate = composition.arm_candidates[index];
    dense_compact_rootless_paths =
        dense_compact_rootless_paths && record.path_record_index == index &&
        record.arm_candidate_index == index &&
        record.arm_record_index == candidate.arm_record_index &&
        record.catalog_event_index == candidate.catalog_event_index &&
        record.removed_shell_point_id ==
            candidate.removed_shell_point_id &&
        record.strict_target_record_index ==
            candidate.strict_target_record_index &&
        record.target_root_binding_index ==
            candidate.target_root_binding_index &&
        record.disposition ==
            ExactCriticalCatalogTypedGammaTargetDisposition::
                omitted_isolated_singleton &&
        !record.local_reduced_root_node_id.has_value() &&
        record.initial_segment_witness.quadratic_max_upper_bound_certified &&
        record.initial_segment_witness.closed_segment_nonstrict_sublevel &&
        record.initial_segment_witness.half_open_segment_strict_sublevel &&
        record.chain_nodes.size() ==
            record.committed_chain_segment_witnesses.size() + 1U &&
        !record.chain_nodes.empty() &&
        record.exact_initial_to_chain_seam_certified &&
        record.source_open_composite_path_strict_critical_sublevel &&
        record.regular_active_terminal_certified;
    committed_segment_count +=
        record.committed_chain_segment_witnesses.size();
    composite_segment_count += record.chain_nodes.size();
    exterior_constraint_count +=
        record.negative_exterior_direction_constraints.size();
    for (const auto& node : record.chain_nodes) {
      point_id_reference_count += node.facet_point_ids.size();
    }
  }
  check(
      dense_compact_rootless_paths,
      "q2 retains two dense exact paths while keeping both local roots omitted");
  check(
      result.counters.preflight_count == 1U &&
          result.counters.source_composition_verification_count == 1U &&
          result.counters.critical_catalog_build_count == 1U &&
          result.counters.critical_arm_family_build_count == 1U &&
          result.counters.saddle_event_reconciliation_count == 1U &&
          result.counters.arm_candidate_path_reconciliation_count == 2U &&
          result.counters.initial_facet_target_membership_check_count == 2U &&
          result.counters.terminal_facet_target_membership_check_count == 2U &&
          result.counters.path_record_count == 2U &&
          result.counters.committed_chain_segment_count ==
              committed_segment_count &&
          result.counters.composite_path_segment_count ==
              composite_segment_count &&
          result.counters.chain_node_point_id_reference_count ==
              point_id_reference_count &&
          result.counters.exterior_constraint_count ==
              exterior_constraint_count &&
          result.counters.matched_local_reduced_root_path_count == 0U &&
          result.counters.omitted_isolated_singleton_path_count == 2U,
      "q2 accounts for every replay, target membership and compact witness");
}

void test_five_scalar_preflight_caps(
    const ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget& full_budget) {
  const CanonicalPointCloud cloud = line_cloud(4U);
  ExactCriticalCatalogTypedGammaJournalResult empty_journal;
  empty_journal.requested_budget = full_budget.arm_root_composition_budget
                                       .root_overlay_budget
                                       .typed_gamma_journal_budget;
  ExactCriticalCatalogTypedGammaRootOverlayResult empty_root_overlay;
  empty_root_overlay.requested_budget =
      full_budget.arm_root_composition_budget.root_overlay_budget;
  ExactCriticalCatalogTypedGammaArmRootCompositionResult empty_composition;
  empty_composition.requested_budget =
      full_budget.arm_root_composition_budget;

  std::array<ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget, 5>
      insufficient{
          full_budget, full_budget, full_budget, full_budget, full_budget};
  insufficient[0].maximum_path_record_count = 39U;
  insufficient[1].maximum_committed_chain_segment_count = 39U;
  insufficient[2].maximum_composite_path_segment_count = 79U;
  insufficient[3].maximum_chain_node_point_id_reference_count = 159U;
  insufficient[4].maximum_exterior_constraint_count = 39U;

  for (std::size_t index = 0U; index < insufficient.size(); ++index) {
    const auto result =
        build_exact_critical_catalog_typed_gamma_arm_root_path_overlay(
            cloud,
            2U,
            insufficient[index],
            empty_journal,
            empty_root_overlay,
            empty_composition);
    const auto verification =
        verify_exact_critical_catalog_typed_gamma_arm_root_path_overlay(
            cloud,
            2U,
            insufficient[index],
            empty_journal,
            empty_root_overlay,
            empty_composition,
            result);
    check(
        result.critical_event_support_bound == 10U &&
            result.critical_arm_bound == 40U &&
            result.required_path_record_capacity == 40U &&
            result.required_committed_chain_segment_capacity == 40U &&
            result.required_composite_path_segment_capacity == 80U &&
            result.required_chain_node_point_id_reference_capacity == 160U &&
            result.required_exterior_constraint_capacity == 40U &&
            result.arm_root_path_candidate_space_size_certified &&
            !result.arm_root_path_preflight_budget_sufficient &&
            result.counters.preflight_count == 1U &&
            result.counters.source_composition_verification_count == 0U &&
            result.counters.critical_catalog_build_count == 0U &&
            result.counters.critical_arm_family_build_count == 0U &&
            result.decision ==
                ExactCriticalCatalogTypedGammaArmRootPathOverlayDecision::
                    no_arm_root_path_overlay_preflight_budget_insufficient &&
            all_certificates_close(verification),
        "each one-below scalar path capacity fails before geometry");
    check_empty_payload(result, "one-below scalar path capacity");
  }
}

void test_nested_cap_rejected(
    const CanonicalPointCloud& cloud,
    const ExactCriticalCatalogTypedGammaJournalResult& journal,
    const ExactCriticalCatalogTypedGammaRootOverlayResult& root_overlay,
    const ExactCriticalCatalogTypedGammaArmRootCompositionResult& composition,
    const ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget& budget) {
  auto excessive = budget;
  excessive.arm_root_composition_budget.root_overlay_budget
      .typed_gamma_journal_budget.provenance_overlay_budget
      .reduced_gamma_history_budget.gamma_budget
      .maximum_enumerated_facet_count =
      ExactStrictGammaBudget::maximum_supported_facet_count + 1U;
  check_invalid_argument(
      [&] {
        static_cast<void>(
            build_exact_critical_catalog_typed_gamma_arm_root_path_overlay(
                cloud,
                2U,
                excessive,
                journal,
                root_overlay,
                composition));
      },
      "an excessive deeply nested strict-Gamma cap is rejected recursively");
}

void test_source_diagnostics(
    const CanonicalPointCloud& cloud,
    const ExactCriticalCatalogTypedGammaJournalResult& journal,
    const ExactCriticalCatalogTypedGammaRootOverlayResult& root_overlay,
    const ExactCriticalCatalogTypedGammaArmRootCompositionResult& composition,
    const ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget& budget) {
  auto falsified_composition = composition;
  check(
      !falsified_composition.arm_candidates.empty(),
      "the q2 source composition exposes a candidate to falsify");
  if (!falsified_composition.arm_candidates.empty()) {
    ++falsified_composition.arm_candidates.front().candidate_index;
  }
  const auto rejected =
      build_exact_critical_catalog_typed_gamma_arm_root_path_overlay(
          cloud,
          2U,
          budget,
          journal,
          root_overlay,
          falsified_composition);
  check(
      rejected.arm_root_path_preflight_budget_sufficient &&
          !rejected.source_composition_fresh_replay_certified &&
          rejected.counters.source_composition_verification_count == 1U &&
          rejected.counters.critical_catalog_build_count == 0U &&
          rejected.counters.critical_arm_family_build_count == 0U &&
          rejected.decision ==
              ExactCriticalCatalogTypedGammaArmRootPathOverlayDecision::
                  no_arm_root_path_overlay_source_composition_rejected &&
          all_certificates_close(
              verify_exact_critical_catalog_typed_gamma_arm_root_path_overlay(
                  cloud,
                  2U,
                  budget,
                  journal,
                  root_overlay,
                  falsified_composition,
                  rejected)),
      "a falsified 6.20 composition is rejected before path reconstruction");
  check_empty_payload(rejected, "falsified 6.20 composition");

  auto incomplete_composition_budget =
      budget.arm_root_composition_budget;
  incomplete_composition_budget.maximum_arm_candidate_count = 15U;
  const auto incomplete_composition =
      build_exact_critical_catalog_typed_gamma_arm_root_composition(
          cloud,
          2U,
          incomplete_composition_budget,
          journal,
          root_overlay);
  auto incomplete_path_budget =
      full_path_budget(incomplete_composition_budget);
  const auto incomplete =
      build_exact_critical_catalog_typed_gamma_arm_root_path_overlay(
          cloud,
          2U,
          incomplete_path_budget,
          journal,
          root_overlay,
          incomplete_composition);
  check(
      incomplete.arm_root_path_preflight_budget_sufficient &&
          incomplete.source_composition_fresh_replay_certified &&
          incomplete.counters.source_composition_verification_count == 1U &&
          incomplete.counters.critical_catalog_build_count == 0U &&
          incomplete.counters.critical_arm_family_build_count == 0U &&
          incomplete.decision ==
              ExactCriticalCatalogTypedGammaArmRootPathOverlayDecision::
                  no_arm_root_path_overlay_source_composition_incomplete &&
          all_certificates_close(
              verify_exact_critical_catalog_typed_gamma_arm_root_path_overlay(
                  cloud,
                  2U,
                  incomplete_path_budget,
                  journal,
                  root_overlay,
                  incomplete_composition,
                  incomplete)),
      "a valid incomplete 6.20 composition stays an atomic diagnostic");
  check_empty_payload(incomplete, "incomplete 6.20 composition");
}

void test_path_record_mutation_rejected(
    const CanonicalPointCloud& cloud,
    const ExactCriticalCatalogTypedGammaJournalResult& journal,
    const ExactCriticalCatalogTypedGammaRootOverlayResult& root_overlay,
    const ExactCriticalCatalogTypedGammaArmRootCompositionResult& composition,
    const ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget& budget,
    const ExactCriticalCatalogTypedGammaArmRootPathOverlayResult& baseline) {
  check(
      complete_facts(baseline) && !baseline.path_records.empty(),
      "the q2 path mutation baseline is complete");
  if (baseline.path_records.empty()) {
    return;
  }
  auto mutated = baseline;
  mutated.path_records.front()
      .initial_segment_witness.half_open_segment_strict_sublevel = false;
  const auto verification =
      verify_exact_critical_catalog_typed_gamma_arm_root_path_overlay(
          cloud,
          2U,
          budget,
          journal,
          root_overlay,
          composition,
          mutated);
  check(
      verification.requested_budget_certified &&
          verification.external_inputs_certified &&
          verification.derived_preflight_sizes_certified &&
          verification.source_composition_decision_certified &&
          verification.source_composition_fresh_replay_certified &&
          !verification.path_records_certified &&
          verification.result_facts_certified &&
          verification.counters_certified &&
          verification.decision_certified && verification.scope_certified &&
          !verification.fresh_replay_certified &&
          !verification.
              exact_critical_catalog_typed_gamma_arm_root_path_overlay_decision_certified,
      "fresh replay rejects one altered analytic segment witness");
}

}  // namespace

int main() {
  const CanonicalPointCloud q2 = q2_triangle_cloud();
  const auto journal_budget = full_journal_budget(1U);
  const auto journal = build_exact_critical_catalog_typed_gamma_journal(
      q2, 2U, journal_budget);
  const auto root_budget = full_root_overlay_budget(journal_budget);
  const auto root_overlay =
      build_exact_critical_catalog_typed_gamma_root_overlay(
          q2, 2U, root_budget, journal);
  const auto composition_budget = full_composition_budget(root_budget);
  const auto composition =
      build_exact_critical_catalog_typed_gamma_arm_root_composition(
          q2, 2U, composition_budget, journal, root_overlay);
  const auto path_budget = full_path_budget(composition_budget);
  const auto paths =
      build_exact_critical_catalog_typed_gamma_arm_root_path_overlay(
          q2,
          2U,
          path_budget,
          journal,
          root_overlay,
          composition);

  test_q2_complete_paths(
      q2, journal, root_overlay, composition, path_budget, paths);
  test_five_scalar_preflight_caps(path_budget);
  test_nested_cap_rejected(
      q2, journal, root_overlay, composition, path_budget);
  test_source_diagnostics(
      q2, journal, root_overlay, composition, path_budget);
  test_path_record_mutation_rejected(
      q2,
      journal,
      root_overlay,
      composition,
      path_budget,
      paths);

  if (failures != 0) {
    std::cerr << failures
              << " typed Gamma arm-root path overlay test(s) failed\n";
    return 1;
  }
  std::cout << "all typed Gamma arm-root path overlay tests passed\n";
  return 0;
}
