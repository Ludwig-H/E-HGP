#include "morsehgp3d/hierarchy/critical_catalog_typed_gamma_arm_root_composition.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using CompositionBudget =
    ExactCriticalCatalogTypedGammaArmRootCompositionBudget;
using CompositionCounters =
    ExactCriticalCatalogTypedGammaArmRootCompositionCounters;
using CompositionDecision =
    ExactCriticalCatalogTypedGammaArmRootCompositionDecision;
using CompositionResult =
    ExactCriticalCatalogTypedGammaArmRootCompositionResult;
using CompositionScope =
    ExactCriticalCatalogTypedGammaArmRootCompositionScope;
using Candidate =
    ExactCriticalCatalogTypedGammaEventLocalArmTargetRootCandidate;
using JournalResult = ExactCriticalCatalogTypedGammaJournalResult;
using JournalBudget = ExactCriticalCatalogTypedGammaJournalBudget;
using RootOverlayBudget =
    ExactCriticalCatalogTypedGammaRootOverlayBudget;
using RootOverlayDecision =
    ExactCriticalCatalogTypedGammaRootOverlayDecision;
using RootOverlayResult =
    ExactCriticalCatalogTypedGammaRootOverlayResult;
using TargetDisposition =
    ExactCriticalCatalogTypedGammaTargetDisposition;

static_assert(
    CompositionBudget::maximum_supported_arm_candidate_count ==
    JournalBudget::maximum_supported_arm_record_count);
static_assert(
    CompositionBudget::maximum_supported_arm_candidate_count ==
    RootOverlayBudget::maximum_supported_target_root_binding_count);

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(std::string{message});
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_multiply(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::overflow_error(std::string{message});
  }
  return left * right;
}

[[nodiscard]] std::size_t bounded_binomial(
    std::size_t point_count,
    std::size_t subset_size) {
  if (subset_size > point_count) {
    return 0U;
  }
  subset_size = std::min(subset_size, point_count - subset_size);
  std::size_t value = 1U;
  for (std::size_t factor = 1U; factor <= subset_size; ++factor) {
    value = checked_multiply(
        value,
        point_count - subset_size + factor,
        "the typed Gamma arm-root composition binomial overflows");
    value /= factor;
  }
  return value;
}

void validate_critical_catalog_budget_caps(
    const ExactCriticalCatalogBudget& budget) {
  if (budget.maximum_candidate_count >
          ExactCriticalCatalogBudget::maximum_supported_candidate_count ||
      budget.maximum_point_classification_count >
          ExactCriticalCatalogBudget::
              maximum_supported_point_classification_count) {
    throw std::invalid_argument(
        "the nested critical-catalog budget exceeds its bounded cap");
  }
}

void validate_strict_gamma_budget_caps(
    const ExactStrictGammaBudget& budget) {
  if (budget.maximum_enumerated_facet_count >
          ExactStrictGammaBudget::maximum_supported_facet_count ||
      budget.maximum_enumerated_coface_count >
          ExactStrictGammaBudget::maximum_supported_coface_count ||
      budget.maximum_union_attempt_count >
          ExactStrictGammaBudget::maximum_supported_union_attempt_count) {
    throw std::invalid_argument(
        "a nested strict-Gamma budget exceeds its bounded cap");
  }
}

void validate_history_budget_caps(
    const ExactPersistentReducedGammaOrderHistoryBudget& budget) {
  validate_strict_gamma_budget_caps(budget.gamma_budget);
  if (budget.maximum_activation_level_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_activation_level_count ||
      budget.maximum_total_facet_work_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_total_facet_work_count ||
      budget.maximum_total_coface_work_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_total_coface_work_count ||
      budget.maximum_total_union_work_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_total_union_work_count ||
      budget.maximum_node_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_node_count ||
      budget.maximum_child_reference_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_child_reference_count ||
      budget.maximum_group_root_reference_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_group_root_reference_count ||
      budget.maximum_group_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_group_count ||
      budget.maximum_group_newly_active_facet_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_group_newly_active_facet_count ||
      budget.maximum_group_equal_level_coface_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_group_equal_level_coface_count ||
      budget.maximum_delta_facet_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_delta_facet_count ||
      budget.maximum_delta_point_reference_count >
          ExactPersistentReducedGammaOrderHistoryBudget::
              maximum_supported_delta_point_reference_count) {
    throw std::invalid_argument(
        "the nested reduced-Gamma history budget exceeds its bounded cap");
  }
}

void validate_provenance_overlay_budget_caps(
    const ExactCriticalCatalogReducedGammaOverlayBudget& budget) {
  validate_critical_catalog_budget_caps(budget.critical_catalog_budget);
  validate_history_budget_caps(budget.reduced_gamma_history_budget);
  if (budget.maximum_event_projection_count >
          ExactCriticalCatalogReducedGammaOverlayBudget::
              maximum_supported_event_projection_count ||
      budget.maximum_group_overlay_count >
          ExactCriticalCatalogReducedGammaOverlayBudget::
              maximum_supported_group_overlay_count ||
      budget.maximum_label_slot_count >
          ExactCriticalCatalogReducedGammaOverlayBudget::
              maximum_supported_label_slot_count ||
      budget.maximum_history_point_id_scan_count >
          ExactCriticalCatalogReducedGammaOverlayBudget::
              maximum_supported_history_point_id_scan_count ||
      budget.maximum_catalog_point_id_scan_count >
          ExactCriticalCatalogReducedGammaOverlayBudget::
              maximum_supported_catalog_point_id_scan_count ||
      budget.maximum_group_event_reference_count >
          ExactCriticalCatalogReducedGammaOverlayBudget::
              maximum_supported_group_event_reference_count) {
    throw std::invalid_argument(
        "a nested provenance-overlay budget exceeds its bounded cap");
  }
}

void validate_arm_overlay_budget_caps(
    const ExactCriticalCatalogArmGammaOverlayBudget& budget) {
  validate_critical_catalog_budget_caps(budget.critical_catalog_budget);
  validate_strict_gamma_budget_caps(budget.reduced_gamma_batch_budget);
  if (budget.per_arm_chain_budget.maximum_committed_strict_segment_count >
          ExactFacetDescentChainBudget::
              maximum_supported_committed_strict_segment_count ||
      budget.maximum_saddle_event_count >
          ExactCriticalCatalogArmGammaOverlayBudget::
              maximum_supported_saddle_event_count ||
      budget.maximum_arm_count >
          ExactCriticalCatalogArmGammaOverlayBudget::
              maximum_supported_arm_count ||
      budget.maximum_saddle_batch_count >
          ExactCriticalCatalogArmGammaOverlayBudget::
              maximum_supported_saddle_batch_count ||
      budget.maximum_target_component_count >
          ExactCriticalCatalogArmGammaOverlayBudget::
              maximum_supported_target_component_count ||
      budget.maximum_target_component_facet_reference_count >
          ExactCriticalCatalogArmGammaOverlayBudget::
              maximum_supported_target_component_facet_reference_count ||
      budget.maximum_target_component_point_id_reference_count >
          ExactCriticalCatalogArmGammaOverlayBudget::
              maximum_supported_target_component_point_id_reference_count ||
      budget.maximum_committed_chain_segment_count >
          ExactCriticalCatalogArmGammaOverlayBudget::
              maximum_supported_committed_chain_segment_count) {
    throw std::invalid_argument(
        "a nested arm-overlay budget exceeds its bounded cap");
  }
}

void validate_journal_budget_caps(const JournalBudget& budget) {
  validate_provenance_overlay_budget_caps(
      budget.provenance_overlay_budget);
  validate_arm_overlay_budget_caps(budget.arm_overlay_budget);
  if (budget.maximum_label_entry_count >
          JournalBudget::maximum_supported_label_entry_count ||
      budget.maximum_saddle_record_count >
          JournalBudget::maximum_supported_saddle_record_count ||
      budget.maximum_terminal_class_record_count >
          JournalBudget::maximum_supported_terminal_class_record_count ||
      budget.maximum_arm_record_count >
          JournalBudget::maximum_supported_arm_record_count ||
      budget.maximum_strict_target_record_count >
          JournalBudget::maximum_supported_strict_target_record_count ||
      budget.maximum_terminal_class_point_id_reference_count >
          JournalBudget::
              maximum_supported_terminal_class_point_id_reference_count ||
      budget.maximum_saddle_index_reference_count >
          JournalBudget::maximum_supported_saddle_index_reference_count ||
      budget.maximum_target_facet_reference_count >
          JournalBudget::maximum_supported_target_facet_reference_count ||
      budget.maximum_target_point_id_reference_count >
          JournalBudget::maximum_supported_target_point_id_reference_count) {
    throw std::invalid_argument(
        "a typed Gamma journal capacity exceeds its bounded cap");
  }
}

void validate_root_overlay_budget_caps(
    const RootOverlayBudget& budget) {
  validate_journal_budget_caps(budget.typed_gamma_journal_budget);
  if (budget.maximum_target_root_binding_count >
          RootOverlayBudget::
              maximum_supported_target_root_binding_count ||
      budget.maximum_live_root_state_count >
          RootOverlayBudget::maximum_supported_live_root_state_count ||
      budget.maximum_live_root_facet_reference_count >
          RootOverlayBudget::
              maximum_supported_live_root_facet_reference_count ||
      budget.maximum_live_root_point_id_reference_count >
          RootOverlayBudget::
              maximum_supported_live_root_point_id_reference_count ||
      budget.maximum_root_facet_replay_work_count >
          RootOverlayBudget::
              maximum_supported_root_facet_replay_work_count ||
      budget.maximum_root_point_id_replay_work_count >
          RootOverlayBudget::
              maximum_supported_root_point_id_replay_work_count ||
      budget.maximum_target_facet_comparison_count >
          RootOverlayBudget::
              maximum_supported_target_facet_comparison_count ||
      budget.maximum_target_point_id_comparison_count >
          RootOverlayBudget::
              maximum_supported_target_point_id_comparison_count ||
      budget.maximum_snapshot_facet_index_count >
          RootOverlayBudget::maximum_supported_snapshot_facet_index_count ||
      budget.maximum_snapshot_point_id_index_count >
          RootOverlayBudget::
              maximum_supported_snapshot_point_id_index_count) {
    throw std::invalid_argument(
        "a nested typed Gamma root-overlay capacity exceeds its cap");
  }
}

void validate_domain(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const CompositionBudget& budget) {
  if (cloud.size() < CompositionResult::minimum_supported_point_count ||
      cloud.size() > CompositionResult::maximum_supported_point_count ||
      order < CompositionResult::minimum_supported_order ||
      order > CompositionResult::maximum_supported_order ||
      order >= cloud.size()) {
    throw std::invalid_argument(
        "the typed Gamma arm-root composition requires 3<=n<=14, "
        "2<=k<n and k<=10");
  }
  validate_root_overlay_budget_caps(budget.root_overlay_budget);
  if (budget.maximum_arm_candidate_count >
      CompositionBudget::maximum_supported_arm_candidate_count) {
    throw std::invalid_argument(
        "the typed Gamma arm-root candidate capacity exceeds its cap");
  }
}

void derive_preflight(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    CompositionResult& result) {
  const std::size_t maximum_support_size =
      std::min({std::size_t{4U}, order + 1U, cloud.size()});
  for (std::size_t support_size = 2U;
       support_size <= maximum_support_size;
       ++support_size) {
    result.critical_event_support_bound = checked_add(
        result.critical_event_support_bound,
        bounded_binomial(cloud.size(), support_size),
        "the typed Gamma arm-root event bound overflows");
  }
  result.critical_arm_bound = checked_multiply(
      4U,
      result.critical_event_support_bound,
      "the typed Gamma arm-root candidate bound overflows");
  result.required_arm_candidate_capacity = result.critical_arm_bound;
  if (result.required_arm_candidate_capacity >
      CompositionBudget::maximum_supported_arm_candidate_count) {
    throw std::logic_error(
        "the derived arm-root composition preflight exceeds its cap");
  }
}

[[nodiscard]] bool budget_covers_preflight(
    const CompositionBudget& budget,
    const CompositionResult& result) {
  return budget.maximum_arm_candidate_count >=
         result.required_arm_candidate_capacity;
}

[[nodiscard]] bool result_facts_match(
    const CompositionResult& observed,
    const CompositionResult& expected) {
  return observed.arm_root_composition_candidate_space_size_certified ==
             expected.arm_root_composition_candidate_space_size_certified &&
         observed.arm_root_composition_preflight_budget_sufficient ==
             expected.arm_root_composition_preflight_budget_sufficient &&
         observed.source_journal_is_external_and_not_retained ==
             expected.source_journal_is_external_and_not_retained &&
         observed.source_root_overlay_is_external_and_not_retained ==
             expected.source_root_overlay_is_external_and_not_retained &&
         observed.source_journal_budget_seam_certified ==
             expected.source_journal_budget_seam_certified &&
         observed.source_root_overlay_budget_seam_certified ==
             expected.source_root_overlay_budget_seam_certified &&
         observed.source_root_overlay_fresh_replay_certified ==
             expected.source_root_overlay_fresh_replay_certified &&
         observed.composition_started_only_after_complete_source_pair ==
             expected.composition_started_only_after_complete_source_pair &&
         observed.every_arm_record_composed_exactly_once ==
             expected.every_arm_record_composed_exactly_once &&
         observed.arm_saddle_memberships_preserved ==
             expected.arm_saddle_memberships_preserved &&
         observed.arm_terminal_class_target_chains_preserved ==
             expected.arm_terminal_class_target_chains_preserved &&
         observed.candidate_indices_dense_and_target_binding_indices_exact ==
             expected.candidate_indices_dense_and_target_binding_indices_exact &&
         observed.
                 target_binding_history_coordinates_match_targets_and_saddles ==
             expected.
                 target_binding_history_coordinates_match_targets_and_saddles &&
         observed.full_pi0_target_authority_preserved_by_external_indices ==
             expected.
                 full_pi0_target_authority_preserved_by_external_indices &&
         observed.reduced_root_dispositions_copied_without_reclassification ==
             expected.
                 reduced_root_dispositions_copied_without_reclassification &&
         observed.shared_targets_preserve_distinct_arm_candidates ==
             expected.shared_targets_preserve_distinct_arm_candidates &&
         observed.candidates_are_event_local_and_not_public_attachments ==
             expected.
                 candidates_are_event_local_and_not_public_attachments &&
         observed.diagnostic_outcomes_have_no_candidates ==
             expected.diagnostic_outcomes_have_no_candidates &&
         observed.
                 critical_catalog_typed_gamma_arm_root_composition_certified ==
             expected.
                 critical_catalog_typed_gamma_arm_root_composition_certified;
}

[[nodiscard]] CompositionResult compute_composition(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    CompositionBudget budget,
    const JournalResult& source_journal,
    const RootOverlayResult& source_root_overlay) {
  validate_domain(cloud, order, budget);
  CompositionResult result;
  result.requested_budget = budget;
  result.point_count = cloud.size();
  result.order = order;
  result.scope = CompositionScope::
      bounded_n14_k10_single_order_event_local_typed_critical_arms_to_strict_full_pi0_targets_and_frozen_pre_batch_local_hgp_reduced_root_or_explicit_omitted_singleton_candidates_only;
  result.counters.preflight_count = 1U;
  result.source_journal_is_external_and_not_retained = true;
  result.source_root_overlay_is_external_and_not_retained = true;
  result.source_journal_budget_seam_certified =
      source_journal.requested_budget ==
      budget.root_overlay_budget.typed_gamma_journal_budget;
  result.source_root_overlay_budget_seam_certified =
      source_root_overlay.requested_budget == budget.root_overlay_budget;
  result.diagnostic_outcomes_have_no_candidates = true;
  derive_preflight(cloud, order, result);
  result.arm_root_composition_candidate_space_size_certified = true;
  result.arm_root_composition_preflight_budget_sufficient =
      result.source_journal_budget_seam_certified &&
      result.source_root_overlay_budget_seam_certified &&
      budget_covers_preflight(budget, result);
  if (!result.arm_root_composition_preflight_budget_sufficient) {
    result.decision = CompositionDecision::
        no_arm_root_composition_preflight_budget_insufficient;
    return result;
  }

  const ExactCriticalCatalogTypedGammaRootOverlayVerification
      source_verification =
          verify_exact_critical_catalog_typed_gamma_root_overlay(
              cloud,
              order,
              budget.root_overlay_budget,
              source_journal,
              source_root_overlay);
  ++result.counters.source_root_overlay_verification_count;
  if (!source_verification.
          exact_critical_catalog_typed_gamma_root_overlay_decision_certified) {
    result.decision =
        CompositionDecision::no_arm_root_composition_source_pair_rejected;
    return result;
  }
  result.source_root_overlay_fresh_replay_certified = true;
  result.source_root_overlay_decision = source_root_overlay.decision;
  if (source_root_overlay.decision !=
      RootOverlayDecision::complete_exhaustive_pre_batch_root_overlay) {
    result.decision =
        CompositionDecision::no_arm_root_composition_source_pair_incomplete;
    return result;
  }
  if (source_journal.decision !=
          ExactCriticalCatalogTypedGammaJournalDecision::
              complete_exhaustive_typed_gamma_journal ||
      source_root_overlay.source_journal_decision !=
          ExactCriticalCatalogTypedGammaJournalDecision::
              complete_exhaustive_typed_gamma_journal) {
    throw std::logic_error(
        "a complete root overlay does not carry a complete typed journal");
  }
  result.composition_started_only_after_complete_source_pair = true;

  if (source_journal.arm_records.size() >
          result.required_arm_candidate_capacity ||
      source_root_overlay.target_root_bindings.size() !=
          source_journal.strict_target_records.size()) {
    throw std::logic_error(
        "a fresh source pair exceeds the arm-root composition preflight");
  }

  std::vector<Candidate> pending_candidates;
  pending_candidates.reserve(source_journal.arm_records.size());
  CompositionCounters pending_counters = result.counters;
  for (std::size_t arm_index = 0U;
       arm_index < source_journal.arm_records.size();
       ++arm_index) {
    const ExactCriticalCatalogTypedGammaArmRecord& arm =
        source_journal.arm_records[arm_index];
    if (arm.arm_record_index != arm_index ||
        arm.saddle_record_index >= source_journal.saddle_records.size() ||
        arm.terminal_class_record_index >=
            source_journal.terminal_class_records.size() ||
        arm.strict_target_record_index >=
            source_journal.strict_target_records.size()) {
      throw std::logic_error(
          "a fresh typed Gamma arm has an invalid dense reference");
    }

    const ExactCriticalCatalogTypedGammaSaddleRecord& saddle =
        source_journal.saddle_records[arm.saddle_record_index];
    const ExactCriticalCatalogTypedGammaTerminalClassRecord&
        terminal_class = source_journal.terminal_class_records[
            arm.terminal_class_record_index];
    const ExactCriticalCatalogTypedGammaStrictTargetRecord& target =
        source_journal.strict_target_records[
            arm.strict_target_record_index];
    const ExactCriticalCatalogTypedGammaTargetRootBinding& binding =
        source_root_overlay.target_root_bindings[
            arm.strict_target_record_index];

    ++pending_counters.saddle_arm_membership_check_count;
    if (saddle.saddle_record_index != arm.saddle_record_index ||
        !std::binary_search(
            saddle.arm_record_indices.begin(),
            saddle.arm_record_indices.end(),
            arm_index)) {
      throw std::logic_error(
          "a typed Gamma arm does not belong to its claimed saddle");
    }

    ++pending_counters.terminal_class_target_chain_check_count;
    if (terminal_class.terminal_class_record_index !=
            arm.terminal_class_record_index ||
        terminal_class.saddle_record_index != arm.saddle_record_index ||
        terminal_class.strict_target_record_index !=
            arm.strict_target_record_index) {
      throw std::logic_error(
          "a typed Gamma arm breaks its terminal-class target chain");
    }

    ++pending_counters.target_binding_check_count;
    if (target.strict_target_record_index !=
            arm.strict_target_record_index ||
        binding.target_root_binding_index !=
            arm.strict_target_record_index ||
        binding.strict_target_record_index !=
            arm.strict_target_record_index ||
        binding.history_batch_index != target.history_batch_index ||
        binding.history_group_record_index !=
            target.history_group_record_index ||
        target.history_batch_index != saddle.history_batch_index ||
        target.history_group_record_index !=
            saddle.history_group_record_index) {
      throw std::logic_error(
          "a typed Gamma arm target does not match its root binding");
    }

    switch (binding.disposition) {
      case TargetDisposition::omitted_isolated_singleton:
        if (binding.root_node_id.has_value() ||
            target.reduced_component_kind !=
                ExactReducedGammaStrictComponentKind::
                    omitted_isolated_facet) {
          throw std::logic_error(
              "an omitted arm target unexpectedly carries a reduced root");
        }
        ++pending_counters.omitted_isolated_singleton_candidate_count;
        break;
      case TargetDisposition::matched_pre_batch_persistent_reduced_root:
        if (!binding.root_node_id.has_value() ||
            target.reduced_component_kind !=
                ExactReducedGammaStrictComponentKind::
                    prior_nontrivial_reduced_root) {
          throw std::logic_error(
              "a matched arm target has no local reduced root");
        }
        ++pending_counters.matched_local_reduced_root_candidate_count;
        break;
    }

    Candidate candidate;
    candidate.candidate_index = arm_index;
    candidate.arm_record_index = arm_index;
    candidate.catalog_event_index = saddle.catalog_event_index;
    candidate.removed_shell_point_id = arm.removed_shell_point_id;
    candidate.strict_target_record_index =
        arm.strict_target_record_index;
    candidate.target_root_binding_index =
        binding.target_root_binding_index;
    candidate.disposition = binding.disposition;
    candidate.local_reduced_root_node_id = binding.root_node_id;
    pending_candidates.push_back(std::move(candidate));
    ++pending_counters.arm_candidate_count;
  }

  if (pending_candidates.size() != source_journal.arm_records.size() ||
      pending_counters.arm_candidate_count !=
          source_journal.arm_records.size() ||
      pending_counters.matched_local_reduced_root_candidate_count +
              pending_counters.omitted_isolated_singleton_candidate_count !=
          source_journal.arm_records.size()) {
    throw std::logic_error(
        "the typed Gamma arm-root composition is not total on arms");
  }

  result.arm_candidates = std::move(pending_candidates);
  result.counters = pending_counters;
  result.every_arm_record_composed_exactly_once = true;
  result.arm_saddle_memberships_preserved = true;
  result.arm_terminal_class_target_chains_preserved = true;
  result.candidate_indices_dense_and_target_binding_indices_exact = true;
  result.target_binding_history_coordinates_match_targets_and_saddles = true;
  result.full_pi0_target_authority_preserved_by_external_indices = true;
  result.reduced_root_dispositions_copied_without_reclassification = true;
  result.shared_targets_preserve_distinct_arm_candidates = true;
  result.candidates_are_event_local_and_not_public_attachments = true;
  result.critical_catalog_typed_gamma_arm_root_composition_certified =
      result.arm_root_composition_candidate_space_size_certified &&
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
      result.target_binding_history_coordinates_match_targets_and_saddles &&
      result.full_pi0_target_authority_preserved_by_external_indices &&
      result.reduced_root_dispositions_copied_without_reclassification &&
      result.shared_targets_preserve_distinct_arm_candidates &&
      result.candidates_are_event_local_and_not_public_attachments &&
      result.diagnostic_outcomes_have_no_candidates;
  if (!result.
          critical_catalog_typed_gamma_arm_root_composition_certified) {
    throw std::logic_error(
        "the typed Gamma arm-root composition failed certification");
  }
  result.decision = CompositionDecision::
      complete_exhaustive_event_local_arm_root_composition;
  return result;
}

}  // namespace

ExactCriticalCatalogTypedGammaArmRootCompositionVerification
verify_exact_critical_catalog_typed_gamma_arm_root_composition(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    CompositionBudget budget,
    const JournalResult& source_journal,
    const RootOverlayResult& source_root_overlay,
    const CompositionResult& result) {
  const CompositionResult expected = compute_composition(
      cloud, order, budget, source_journal, source_root_overlay);
  ExactCriticalCatalogTypedGammaArmRootCompositionVerification verification;
  verification.requested_budget_certified =
      result.requested_budget == budget &&
      result.requested_budget == expected.requested_budget;
  verification.external_inputs_certified =
      result.point_count == cloud.size() &&
      result.point_count == expected.point_count && result.order == order &&
      result.order == expected.order;
  verification.derived_preflight_sizes_certified =
      result.critical_event_support_bound ==
          expected.critical_event_support_bound &&
      result.critical_arm_bound == expected.critical_arm_bound &&
      result.required_arm_candidate_capacity ==
          expected.required_arm_candidate_capacity &&
      result.arm_root_composition_candidate_space_size_certified ==
          expected.arm_root_composition_candidate_space_size_certified;
  verification.source_root_overlay_decision_certified =
      result.source_root_overlay_decision ==
      expected.source_root_overlay_decision;
  verification.source_root_overlay_fresh_replay_certified =
      result.source_root_overlay_fresh_replay_certified ==
      expected.source_root_overlay_fresh_replay_certified;
  verification.arm_candidates_certified =
      result.arm_candidates == expected.arm_candidates;
  verification.result_facts_certified = result_facts_match(result, expected);
  verification.counters_certified = result.counters == expected.counters;
  verification.decision_certified = result.decision == expected.decision;
  verification.scope_certified =
      result.scope == CompositionScope::
          bounded_n14_k10_single_order_event_local_typed_critical_arms_to_strict_full_pi0_targets_and_frozen_pre_batch_local_hgp_reduced_root_or_explicit_omitted_singleton_candidates_only &&
      result.scope == expected.scope;
  verification.fresh_replay_certified =
      verification.requested_budget_certified &&
      verification.external_inputs_certified &&
      verification.derived_preflight_sizes_certified &&
      verification.source_root_overlay_decision_certified &&
      verification.source_root_overlay_fresh_replay_certified &&
      verification.arm_candidates_certified &&
      verification.result_facts_certified &&
      verification.counters_certified && verification.decision_certified &&
      verification.scope_certified;
  verification.
      exact_critical_catalog_typed_gamma_arm_root_composition_decision_certified =
      verification.fresh_replay_certified;
  return verification;
}

ExactCriticalCatalogTypedGammaArmRootCompositionResult
build_exact_critical_catalog_typed_gamma_arm_root_composition(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    CompositionBudget budget,
    const JournalResult& source_journal,
    const RootOverlayResult& source_root_overlay) {
  CompositionResult result = compute_composition(
      cloud, order, budget, source_journal, source_root_overlay);
  const ExactCriticalCatalogTypedGammaArmRootCompositionVerification
      verification =
          verify_exact_critical_catalog_typed_gamma_arm_root_composition(
              cloud,
              order,
              budget,
              source_journal,
              source_root_overlay,
              result);
  if (!verification.
          exact_critical_catalog_typed_gamma_arm_root_composition_decision_certified) {
    throw std::logic_error(
        "the typed Gamma arm-root composition failed its fresh replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
