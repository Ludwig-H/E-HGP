#include "morsehgp3d/hierarchy/critical_catalog_typed_gamma_arm_root_composition.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
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
    ExactCriticalCatalogTypedGammaArmRootCompositionDecision;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaArmRootCompositionResult;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaArmRootCompositionScope;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaArmRootCompositionVerification;
using morsehgp3d::hierarchy::ExactCriticalCatalogTypedGammaArmRecord;
using morsehgp3d::hierarchy::ExactCriticalCatalogTypedGammaJournalBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogTypedGammaJournalResult;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaRootOverlayBudget;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaRootOverlayDecision;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaRootOverlayResult;
using morsehgp3d::hierarchy::ExactCriticalCatalogTypedGammaSaddleRecord;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaTargetDisposition;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaEventLocalArmTargetRootCandidate;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaOrderHistoryBudget;
using morsehgp3d::hierarchy::ExactStrictGammaBudget;
using morsehgp3d::hierarchy::
    build_exact_critical_catalog_typed_gamma_arm_root_composition;
using morsehgp3d::hierarchy::
    build_exact_critical_catalog_typed_gamma_journal;
using morsehgp3d::hierarchy::
    build_exact_critical_catalog_typed_gamma_root_overlay;
using morsehgp3d::hierarchy::
    verify_exact_critical_catalog_typed_gamma_arm_root_composition;
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

[[nodiscard]] CanonicalPointCloud shared_terminal_cloud() {
  const std::array<CertifiedPoint3, 5> input{
      point(-8.0, 1.0),
      point(-5.0, -7.0),
      point(-3.0, -8.0),
      point(4.0, 8.0),
      point(5.0, -7.0)};
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

[[nodiscard]] std::size_t binomial(
    std::size_t point_count,
    std::size_t subset_size) {
  if (subset_size > point_count) {
    return 0U;
  }
  subset_size = std::min(subset_size, point_count - subset_size);
  std::size_t value = 1U;
  for (std::size_t factor = 1U; factor <= subset_size; ++factor) {
    value *= point_count - subset_size + factor;
    value /= factor;
  }
  return value;
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
full_journal_budget(std::size_t per_arm_chain_capacity = 2U) {
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

[[nodiscard]] bool all_certificates_close(
    const ExactCriticalCatalogTypedGammaArmRootCompositionVerification&
        verification) {
  return verification.requested_budget_certified &&
         verification.external_inputs_certified &&
         verification.derived_preflight_sizes_certified &&
         verification.source_root_overlay_decision_certified &&
         verification.source_root_overlay_fresh_replay_certified &&
         verification.arm_candidates_certified &&
         verification.result_facts_certified &&
         verification.counters_certified &&
         verification.decision_certified && verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.
             exact_critical_catalog_typed_gamma_arm_root_composition_decision_certified;
}

[[nodiscard]] bool complete_facts(
    const ExactCriticalCatalogTypedGammaArmRootCompositionResult& result) {
  return result.arm_root_composition_candidate_space_size_certified &&
         result.arm_root_composition_preflight_budget_sufficient &&
         result.source_journal_is_external_and_not_retained &&
         result.source_root_overlay_is_external_and_not_retained &&
         result.source_journal_budget_seam_certified &&
         result.source_root_overlay_budget_seam_certified &&
         result.source_root_overlay_fresh_replay_certified &&
         result.composition_started_only_after_complete_source_pair &&
         result.every_arm_record_composed_exactly_once &&
         result.arm_saddle_memberships_preserved &&
         result.arm_terminal_class_target_chains_preserved &&
         result.candidate_indices_dense_and_target_binding_indices_exact &&
         result.
             target_binding_history_coordinates_match_targets_and_saddles &&
         result.full_pi0_target_authority_preserved_by_external_indices &&
         result.reduced_root_dispositions_copied_without_reclassification &&
         result.shared_targets_preserve_distinct_arm_candidates &&
         result.candidates_are_event_local_and_not_public_attachments &&
         result.diagnostic_outcomes_have_no_candidates &&
         result.
             critical_catalog_typed_gamma_arm_root_composition_certified &&
         result.decision ==
             ExactCriticalCatalogTypedGammaArmRootCompositionDecision::
                 complete_exhaustive_event_local_arm_root_composition &&
         result.scope ==
             ExactCriticalCatalogTypedGammaArmRootCompositionScope::
                 bounded_n14_k10_single_order_event_local_typed_critical_arms_to_strict_full_pi0_targets_and_frozen_pre_batch_local_hgp_reduced_root_or_explicit_omitted_singleton_candidates_only;
}

void check_empty_payload(
    const ExactCriticalCatalogTypedGammaArmRootCompositionResult& result,
    const std::string& label) {
  check(
      result.arm_candidates.empty(),
      label + ": no partial arm-root candidate is published");
}

void test_q2_complete_composition(
    const CanonicalPointCloud& cloud,
    const ExactCriticalCatalogTypedGammaJournalResult& journal,
    const ExactCriticalCatalogTypedGammaRootOverlayResult& root_overlay,
    const ExactCriticalCatalogTypedGammaArmRootCompositionBudget& budget,
    const ExactCriticalCatalogTypedGammaArmRootCompositionResult& result) {
  const auto verification =
      verify_exact_critical_catalog_typed_gamma_arm_root_composition(
          cloud, 2U, budget, journal, root_overlay, result);
  check(
      complete_facts(result) && all_certificates_close(verification) &&
          std::string{
              ExactCriticalCatalogTypedGammaArmRootCompositionResult::
                  proof_basis} ==
              "exact_fresh_typed_critical_arm_target_indices_composed_with_"
              "recertified_target_root_bindings_v1",
      "q2 closes the event-local arm-to-target-to-root composition");
  check(
      result.point_count == 3U && result.order == 2U &&
          result.critical_event_support_bound == 4U &&
          result.critical_arm_bound == 16U &&
          result.required_arm_candidate_capacity == 16U &&
          result.arm_candidates.size() == journal.arm_records.size() &&
          result.arm_candidates.size() == 2U,
      "q2 exposes the exact combinatorial cap and one candidate per arm");
  check(
      result.counters.preflight_count == 1U &&
          result.counters.source_root_overlay_verification_count == 1U &&
          result.counters.arm_candidate_count == 2U &&
          result.counters.saddle_arm_membership_check_count == 2U &&
          result.counters.terminal_class_target_chain_check_count == 2U &&
          result.counters.target_binding_check_count == 2U &&
          result.counters.matched_local_reduced_root_candidate_count == 0U &&
          result.counters.omitted_isolated_singleton_candidate_count == 2U,
      "q2 counts the dense composition and both omitted singletons exactly");

  bool dense_exact_omissions = true;
  for (std::size_t index = 0U; index < result.arm_candidates.size();
       ++index) {
    const auto& candidate = result.arm_candidates[index];
    const auto& arm = journal.arm_records[index];
    const auto& saddle = journal.saddle_records[arm.saddle_record_index];
    const auto& binding =
        root_overlay.target_root_bindings[arm.strict_target_record_index];
    dense_exact_omissions =
        dense_exact_omissions && candidate.candidate_index == index &&
        candidate.arm_record_index == index &&
        candidate.catalog_event_index == saddle.catalog_event_index &&
        candidate.removed_shell_point_id == arm.removed_shell_point_id &&
        candidate.strict_target_record_index ==
            arm.strict_target_record_index &&
        candidate.target_root_binding_index ==
            binding.target_root_binding_index &&
        candidate.disposition ==
            ExactCriticalCatalogTypedGammaTargetDisposition::
                omitted_isolated_singleton &&
        !candidate.local_reduced_root_node_id.has_value();
  }
  check(
      dense_exact_omissions,
      "q2 preserves every event-local semantic key and copies no new root");
}

void test_single_cap_and_all_domain_preflight(
    const CanonicalPointCloud& q2,
    const ExactCriticalCatalogTypedGammaJournalResult& journal,
    const ExactCriticalCatalogTypedGammaRootOverlayResult& root_overlay,
    const ExactCriticalCatalogTypedGammaArmRootCompositionBudget& budget) {
  ExactCriticalCatalogTypedGammaArmRootCompositionBudget insufficient =
      budget;
  insufficient.maximum_arm_candidate_count = 15U;
  const auto q2_failure =
      build_exact_critical_catalog_typed_gamma_arm_root_composition(
          q2, 2U, insufficient, journal, root_overlay);
  check(
      q2_failure.arm_root_composition_candidate_space_size_certified &&
          !q2_failure.arm_root_composition_preflight_budget_sufficient &&
          q2_failure.counters.preflight_count == 1U &&
          q2_failure.counters.source_root_overlay_verification_count == 0U &&
          q2_failure.decision ==
              ExactCriticalCatalogTypedGammaArmRootCompositionDecision::
                  no_arm_root_composition_preflight_budget_insufficient &&
          all_certificates_close(
              verify_exact_critical_catalog_typed_gamma_arm_root_composition(
                  q2,
                  2U,
                  insufficient,
                  journal,
                  root_overlay,
                  q2_failure)),
      "a capacity one below q2's bound fails before source replay");
  check_empty_payload(q2_failure, "q2 insufficient cap");

  auto excessive_nested = budget;
  excessive_nested.maximum_arm_candidate_count = 15U;
  excessive_nested.root_overlay_budget.typed_gamma_journal_budget
      .provenance_overlay_budget.reduced_gamma_history_budget.gamma_budget
      .maximum_enumerated_facet_count =
      ExactStrictGammaBudget::maximum_supported_facet_count + 1U;
  check_invalid_argument(
      [&] {
        static_cast<void>(
            build_exact_critical_catalog_typed_gamma_arm_root_composition(
                q2,
                2U,
                excessive_nested,
                journal,
                root_overlay));
      },
      "a deeply nested strict-Gamma capacity above its static cap is rejected");

  std::size_t observed_maximum = 0U;
  for (std::size_t point_count = 3U; point_count <= 14U; ++point_count) {
    const CanonicalPointCloud cloud = line_cloud(point_count);
    for (std::size_t order = 2U;
         order < point_count && order <= 10U;
         ++order) {
      const std::size_t maximum_support_size =
          std::min({std::size_t{4U}, order + 1U, point_count});
      std::size_t event_bound = 0U;
      for (std::size_t support_size = 2U;
           support_size <= maximum_support_size;
           ++support_size) {
        event_bound += binomial(point_count, support_size);
      }
      const std::size_t arm_bound = 4U * event_bound;
      const auto journal_budget = full_journal_budget(1U);
      const auto root_budget = full_root_overlay_budget(journal_budget);
      auto composition_budget = full_composition_budget(root_budget);
      composition_budget.maximum_arm_candidate_count = arm_bound - 1U;
      ExactCriticalCatalogTypedGammaJournalResult empty_journal;
      empty_journal.requested_budget = journal_budget;
      ExactCriticalCatalogTypedGammaRootOverlayResult empty_root_overlay;
      empty_root_overlay.requested_budget = root_budget;
      const auto result =
          build_exact_critical_catalog_typed_gamma_arm_root_composition(
              cloud,
              order,
              composition_budget,
              empty_journal,
              empty_root_overlay);
      check(
          result.critical_event_support_bound == event_bound &&
              result.critical_arm_bound == arm_bound &&
              result.required_arm_candidate_capacity == arm_bound &&
              result.arm_root_composition_candidate_space_size_certified &&
              !result.arm_root_composition_preflight_budget_sufficient &&
              result.counters.source_root_overlay_verification_count == 0U &&
              result.arm_candidates.empty(),
          "every bounded (n,k) pair derives its single cap without geometry");
      observed_maximum = std::max(observed_maximum, arm_bound);
    }
  }
  check(
      observed_maximum ==
              ExactCriticalCatalogTypedGammaArmRootCompositionBudget::
                  maximum_supported_arm_candidate_count &&
          observed_maximum == 5824U,
      "the bounded domain attains exactly the declared 5824-arm cap");
}

void test_budget_seams(
    const CanonicalPointCloud& cloud,
    const ExactCriticalCatalogTypedGammaJournalResult& journal,
    const ExactCriticalCatalogTypedGammaRootOverlayResult& root_overlay,
    const ExactCriticalCatalogTypedGammaArmRootCompositionBudget& budget) {
  auto journal_mismatch_budget = budget;
  --journal_mismatch_budget.root_overlay_budget
        .typed_gamma_journal_budget.maximum_label_entry_count;
  auto matching_mutated_overlay = root_overlay;
  matching_mutated_overlay.requested_budget =
      journal_mismatch_budget.root_overlay_budget;
  const auto journal_seam_failure =
      build_exact_critical_catalog_typed_gamma_arm_root_composition(
          cloud,
          2U,
          journal_mismatch_budget,
          journal,
          matching_mutated_overlay);
  check(
      !journal_seam_failure.source_journal_budget_seam_certified &&
          journal_seam_failure.source_root_overlay_budget_seam_certified &&
          journal_seam_failure.counters
                  .source_root_overlay_verification_count ==
              0U &&
          journal_seam_failure.decision ==
              ExactCriticalCatalogTypedGammaArmRootCompositionDecision::
                  no_arm_root_composition_preflight_budget_insufficient,
      "the nested journal-budget seam fails before recertification");
  check_empty_payload(journal_seam_failure, "journal-budget seam");

  auto root_mismatch_budget = budget;
  --root_mismatch_budget.root_overlay_budget
        .maximum_target_root_binding_count;
  const auto root_seam_failure =
      build_exact_critical_catalog_typed_gamma_arm_root_composition(
          cloud,
          2U,
          root_mismatch_budget,
          journal,
          root_overlay);
  check(
      root_seam_failure.source_journal_budget_seam_certified &&
          !root_seam_failure.source_root_overlay_budget_seam_certified &&
          root_seam_failure.counters
                  .source_root_overlay_verification_count ==
              0U &&
          root_seam_failure.decision ==
              ExactCriticalCatalogTypedGammaArmRootCompositionDecision::
                  no_arm_root_composition_preflight_budget_insufficient,
      "the root-overlay budget seam fails independently before replay");
  check_empty_payload(root_seam_failure, "root-overlay budget seam");
}

void test_source_diagnostics(
    const CanonicalPointCloud& cloud,
    const ExactCriticalCatalogTypedGammaJournalResult& journal,
    const ExactCriticalCatalogTypedGammaRootOverlayResult& complete_overlay,
    const ExactCriticalCatalogTypedGammaArmRootCompositionBudget&
        complete_budget) {
  auto incomplete_root_budget = complete_budget.root_overlay_budget;
  incomplete_root_budget.maximum_target_root_binding_count = 15U;
  const auto incomplete_root_overlay =
      build_exact_critical_catalog_typed_gamma_root_overlay(
          cloud, 2U, incomplete_root_budget, journal);
  const auto incomplete_budget =
      full_composition_budget(incomplete_root_budget);
  const auto incomplete =
      build_exact_critical_catalog_typed_gamma_arm_root_composition(
          cloud,
          2U,
          incomplete_budget,
          journal,
          incomplete_root_overlay);
  check(
      incomplete_root_overlay.decision ==
              ExactCriticalCatalogTypedGammaRootOverlayDecision::
                  no_root_overlay_preflight_budget_insufficient &&
          incomplete.arm_root_composition_preflight_budget_sufficient &&
          incomplete.source_root_overlay_fresh_replay_certified &&
          incomplete.counters.source_root_overlay_verification_count == 1U &&
          incomplete.counters.arm_candidate_count == 0U &&
          incomplete.decision ==
              ExactCriticalCatalogTypedGammaArmRootCompositionDecision::
                  no_arm_root_composition_source_pair_incomplete &&
          all_certificates_close(
              verify_exact_critical_catalog_typed_gamma_arm_root_composition(
                  cloud,
                  2U,
                  incomplete_budget,
                  journal,
                  incomplete_root_overlay,
                  incomplete)),
      "a certified incomplete 6.19 source remains an atomic diagnostic");
  check_empty_payload(incomplete, "incomplete 6.19 source");

  auto falsified_overlay = complete_overlay;
  check(
      !falsified_overlay.target_root_bindings.empty(),
      "the q2 root overlay exposes a binding to falsify");
  if (!falsified_overlay.target_root_bindings.empty()) {
    auto& binding = falsified_overlay.target_root_bindings.front();
    binding.disposition =
        ExactCriticalCatalogTypedGammaTargetDisposition::
            matched_pre_batch_persistent_reduced_root;
    binding.root_node_id = 0U;
  }
  const auto rejected =
      build_exact_critical_catalog_typed_gamma_arm_root_composition(
          cloud,
          2U,
          complete_budget,
          journal,
          falsified_overlay);
  check(
      rejected.arm_root_composition_preflight_budget_sufficient &&
          !rejected.source_root_overlay_fresh_replay_certified &&
          rejected.counters.source_root_overlay_verification_count == 1U &&
          rejected.counters.arm_candidate_count == 0U &&
          rejected.decision ==
              ExactCriticalCatalogTypedGammaArmRootCompositionDecision::
                  no_arm_root_composition_source_pair_rejected &&
          all_certificates_close(
              verify_exact_critical_catalog_typed_gamma_arm_root_composition(
                  cloud,
                  2U,
                  complete_budget,
                  journal,
                  falsified_overlay,
                  rejected)),
      "a falsified 6.19 binding is rejected before composition");
  check_empty_payload(rejected, "rejected 6.19 source");
}

void test_shared_terminal_candidates() {
  const CanonicalPointCloud cloud = shared_terminal_cloud();
  const auto journal_budget = full_journal_budget(2U);
  const auto journal = build_exact_critical_catalog_typed_gamma_journal(
      cloud, 3U, journal_budget);
  const auto root_budget = full_root_overlay_budget(journal_budget);
  const auto root_overlay =
      build_exact_critical_catalog_typed_gamma_root_overlay(
          cloud, 3U, root_budget, journal);
  const auto budget = full_composition_budget(root_budget);
  const auto result =
      build_exact_critical_catalog_typed_gamma_arm_root_composition(
          cloud, 3U, budget, journal, root_overlay);

  const ExactCriticalCatalogTypedGammaArmRecord* removed_one = nullptr;
  const ExactCriticalCatalogTypedGammaArmRecord* removed_two = nullptr;
  const ExactCriticalCatalogTypedGammaArmRecord* removed_three = nullptr;
  const ExactCriticalCatalogTypedGammaSaddleRecord* selected_saddle = nullptr;
  for (const auto& saddle : journal.saddle_records) {
    const ExactCriticalCatalogTypedGammaArmRecord* one = nullptr;
    const ExactCriticalCatalogTypedGammaArmRecord* two = nullptr;
    const ExactCriticalCatalogTypedGammaArmRecord* three = nullptr;
    for (const std::size_t arm_index : saddle.arm_record_indices) {
      if (arm_index >= journal.arm_records.size()) {
        continue;
      }
      const auto& arm = journal.arm_records[arm_index];
      if (arm.removed_shell_point_id == 1U) {
        one = &arm;
      } else if (arm.removed_shell_point_id == 2U) {
        two = &arm;
      } else if (arm.removed_shell_point_id == 3U) {
        three = &arm;
      }
    }
    if (one != nullptr && two != nullptr && three != nullptr &&
        one->strict_target_record_index ==
            three->strict_target_record_index &&
        one->strict_target_record_index != two->strict_target_record_index) {
      selected_saddle = &saddle;
      removed_one = one;
      removed_two = two;
      removed_three = three;
      break;
    }
  }

  const auto candidate_for = [&result](
                                 const ExactCriticalCatalogTypedGammaArmRecord*
                                     arm)
      -> const ExactCriticalCatalogTypedGammaEventLocalArmTargetRootCandidate* {
    if (arm == nullptr || arm->arm_record_index >= result.arm_candidates.size()) {
      return nullptr;
    }
    return &result.arm_candidates[arm->arm_record_index];
  };
  const auto* candidate_one = candidate_for(removed_one);
  const auto* candidate_two = candidate_for(removed_two);
  const auto* candidate_three = candidate_for(removed_three);

  check(
      complete_facts(result) && result.arm_candidates.size() == 7U &&
          selected_saddle != nullptr && candidate_one != nullptr &&
          candidate_two != nullptr && candidate_three != nullptr,
      "the shared-terminal fixture composes all seven typed arms");
  if (candidate_one != nullptr && candidate_two != nullptr &&
      candidate_three != nullptr) {
    check(
        candidate_one->candidate_index != candidate_three->candidate_index &&
            candidate_one->removed_shell_point_id == 1U &&
            candidate_three->removed_shell_point_id == 3U &&
            candidate_one->catalog_event_index ==
                candidate_three->catalog_event_index &&
            candidate_one->strict_target_record_index ==
                candidate_three->strict_target_record_index &&
            candidate_one->target_root_binding_index ==
                candidate_three->target_root_binding_index &&
            candidate_one->disposition ==
                ExactCriticalCatalogTypedGammaTargetDisposition::
                    matched_pre_batch_persistent_reduced_root &&
            candidate_one->local_reduced_root_node_id.has_value() &&
            candidate_one->local_reduced_root_node_id ==
                candidate_three->local_reduced_root_node_id,
        "distinct removed-shell arms preserve their identity while sharing one target and prior root");
    check(
        candidate_two->removed_shell_point_id == 2U &&
            candidate_two->strict_target_record_index !=
                candidate_one->strict_target_record_index &&
            candidate_two->target_root_binding_index !=
                candidate_one->target_root_binding_index &&
            candidate_two->disposition ==
                ExactCriticalCatalogTypedGammaTargetDisposition::
                    omitted_isolated_singleton &&
            !candidate_two->local_reduced_root_node_id.has_value(),
        "the sibling isolated singleton stays explicit and rootless");
  }
  check(
      result.counters.matched_local_reduced_root_candidate_count +
              result.counters.omitted_isolated_singleton_candidate_count ==
          result.arm_candidates.size(),
      "shared targets never deduplicate the total arm composition");
}

void test_mutations_and_twin_cloud(
    const CanonicalPointCloud& cloud,
    const ExactCriticalCatalogTypedGammaJournalResult& journal,
    const ExactCriticalCatalogTypedGammaRootOverlayResult& root_overlay,
    const ExactCriticalCatalogTypedGammaArmRootCompositionBudget& budget,
    const ExactCriticalCatalogTypedGammaArmRootCompositionResult& baseline) {
  check(
      complete_facts(baseline) && baseline.arm_candidates.size() == 2U,
      "the q2 composition mutation baseline is complete and compact");
  if (!baseline.arm_candidates.empty()) {
    auto mutated = baseline;
    --mutated.requested_budget.maximum_arm_candidate_count;
    ++mutated.required_arm_candidate_capacity;
    mutated.source_root_overlay_decision =
        ExactCriticalCatalogTypedGammaRootOverlayDecision::not_certified;
    mutated.arm_candidates.front().disposition =
        ExactCriticalCatalogTypedGammaTargetDisposition::
            matched_pre_batch_persistent_reduced_root;
    mutated.arm_candidates.front().local_reduced_root_node_id = 0U;
    mutated.every_arm_record_composed_exactly_once = false;
    ++mutated.counters.matched_local_reduced_root_candidate_count;
    mutated.decision =
        ExactCriticalCatalogTypedGammaArmRootCompositionDecision::
            no_arm_root_composition_source_pair_incomplete;
    mutated.scope =
        ExactCriticalCatalogTypedGammaArmRootCompositionScope::unspecified;
    const auto verification =
        verify_exact_critical_catalog_typed_gamma_arm_root_composition(
            cloud, 2U, budget, journal, root_overlay, mutated);
    check(
        !verification.requested_budget_certified &&
            verification.external_inputs_certified &&
            !verification.derived_preflight_sizes_certified &&
            !verification.source_root_overlay_decision_certified &&
            verification.source_root_overlay_fresh_replay_certified &&
            !verification.arm_candidates_certified &&
            !verification.result_facts_certified &&
            !verification.counters_certified &&
            !verification.decision_certified &&
            !verification.scope_certified &&
            !verification.fresh_replay_certified &&
            !verification.
                exact_critical_catalog_typed_gamma_arm_root_composition_decision_certified,
        "fresh replay rejects simultaneous mutations of every compact composition layer");
  }

  const std::array<CertifiedPoint3, 3> twin_input{
      point(-2.0, 0.0), point(0.0, 1.25), point(2.0, 0.0)};
  const CanonicalPointCloud twin = canonical_cloud(twin_input);
  const auto twin_verification =
      verify_exact_critical_catalog_typed_gamma_arm_root_composition(
          twin, 2U, budget, journal, root_overlay, baseline);
  check(
      !twin_verification.source_root_overlay_fresh_replay_certified &&
          !twin_verification.arm_candidates_certified &&
          !twin_verification.fresh_replay_certified &&
          !twin_verification.
              exact_critical_catalog_typed_gamma_arm_root_composition_decision_certified,
      "a same-size twin cloud cannot reuse the external source pair or its composition");
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

  test_q2_complete_composition(
      q2, journal, root_overlay, composition_budget, composition);
  test_single_cap_and_all_domain_preflight(
      q2, journal, root_overlay, composition_budget);
  test_budget_seams(q2, journal, root_overlay, composition_budget);
  test_source_diagnostics(
      q2, journal, root_overlay, composition_budget);
  test_shared_terminal_candidates();
  test_mutations_and_twin_cloud(
      q2, journal, root_overlay, composition_budget, composition);

  if (failures != 0) {
    std::cerr << failures << " typed Gamma arm-root composition test(s) failed\n";
    return 1;
  }
  std::cout << "all typed Gamma arm-root composition tests passed\n";
  return 0;
}
