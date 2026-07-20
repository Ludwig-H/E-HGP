#include "morsehgp3d/hierarchy/critical_catalog_typed_gamma_root_overlay.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::ExactCriticalCatalogArmGammaOverlayBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogReducedGammaOverlayBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogTypedGammaJournalBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogTypedGammaJournalDecision;
using morsehgp3d::hierarchy::ExactCriticalCatalogTypedGammaJournalResult;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaRootOverlayBudget;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaRootOverlayDecision;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaRootOverlayResult;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaRootOverlayScope;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaRootOverlayVerification;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaTargetDisposition;
using morsehgp3d::hierarchy::
    ExactCriticalCatalogTypedGammaTargetRootBinding;
using morsehgp3d::hierarchy::ExactFacetDescentChainBudget;
using morsehgp3d::hierarchy::ExactPersistentReducedGammaOrderHistoryBudget;
using morsehgp3d::hierarchy::ExactReducedGammaBatchGroupKind;
using morsehgp3d::hierarchy::ExactStrictGammaBudget;
using morsehgp3d::hierarchy::
    build_exact_critical_catalog_typed_gamma_journal;
using morsehgp3d::hierarchy::
    build_exact_critical_catalog_typed_gamma_root_overlay;
using morsehgp3d::hierarchy::
    verify_exact_critical_catalog_typed_gamma_root_overlay;
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
    std::forward<Function>(function)();
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

[[nodiscard]] CanonicalPointCloud e5_cloud() {
  const std::array<CertifiedPoint3, 5> input{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0)};
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
full_journal_budget(std::size_t per_arm_chain_capacity) {
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
full_overlay_budget(
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

[[nodiscard]] bool all_certificates_close(
    const ExactCriticalCatalogTypedGammaRootOverlayVerification&
        verification) {
  return verification.requested_budget_certified &&
         verification.external_inputs_certified &&
         verification.derived_preflight_sizes_certified &&
         verification.source_journal_decision_certified &&
         verification.source_journal_fresh_replay_certified &&
         verification.target_root_bindings_certified &&
         verification.result_facts_certified &&
         verification.counters_certified &&
         verification.decision_certified &&
         verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.
             exact_critical_catalog_typed_gamma_root_overlay_decision_certified;
}

[[nodiscard]] bool complete_facts(
    const ExactCriticalCatalogTypedGammaRootOverlayResult& result) {
  return result.root_overlay_candidate_space_size_certified &&
         result.root_overlay_preflight_budget_sufficient &&
         result.source_journal_is_external_and_not_retained &&
         result.source_journal_budget_seam_certified &&
         result.source_journal_fresh_replay_certified &&
         result.root_sweep_started_only_after_complete_source_journal &&
         result.every_history_batch_replayed_exactly_once &&
         result.targets_resolved_against_frozen_pre_batch_snapshots &&
         result.snapshot_indices_cover_all_active_facets &&
         result.nontrivial_targets_match_complete_facet_families &&
         result.omitted_targets_are_singletons_absent_from_active_facets &&
         result.reduced_component_kinds_checked_after_geometric_binding &&
         result.matched_roots_belong_to_target_history_group_prior_roots &&
         result.every_strict_target_bound_exactly_once &&
         result.persistent_root_ids_preserved &&
         result.mutations_applied_after_complete_batch_resolution &&
         result.final_replayed_roots_match_source_history &&
         result.diagnostic_outcomes_have_no_bindings &&
         result.critical_catalog_typed_gamma_root_overlay_certified &&
         result.decision ==
             ExactCriticalCatalogTypedGammaRootOverlayDecision::
                 complete_exhaustive_pre_batch_root_overlay &&
         result.scope ==
             ExactCriticalCatalogTypedGammaRootOverlayScope::
                 bounded_n14_k10_single_order_full_pi0_target_families_to_frozen_pre_batch_local_hgp_reduced_roots_with_explicit_isolated_singletons_only;
}

void check_empty_payload(
    const ExactCriticalCatalogTypedGammaRootOverlayResult& result,
    const std::string& label) {
  check(
      result.target_root_bindings.empty(),
      label + ": no partial target-to-root binding is published");
}

void test_q2_complete_overlay(
    const CanonicalPointCloud& cloud,
    const ExactCriticalCatalogTypedGammaJournalBudget& journal_budget,
    const ExactCriticalCatalogTypedGammaJournalResult& journal,
    const ExactCriticalCatalogTypedGammaRootOverlayBudget& overlay_budget,
    const ExactCriticalCatalogTypedGammaRootOverlayResult& overlay) {
  const ExactCriticalCatalogTypedGammaRootOverlayVerification verification =
      verify_exact_critical_catalog_typed_gamma_root_overlay(
          cloud, 2U, overlay_budget, journal, overlay);

  check(
      complete_facts(overlay) && all_certificates_close(verification) &&
          std::string{
              ExactCriticalCatalogTypedGammaRootOverlayResult::
                  proof_basis} ==
              "exact_fresh_typed_full_pi0_target_families_reconciled_with_"
              "frozen_pre_batch_local_reduced_gamma_roots_v1" &&
          overlay.source_journal_decision ==
              ExactCriticalCatalogTypedGammaJournalDecision::
                  complete_exhaustive_typed_gamma_journal,
      "q2 closes the exhaustive fresh target-to-pre-batch-root overlay");
  check(
      overlay.requested_budget.typed_gamma_journal_budget == journal_budget &&
          overlay.point_count == 3U && overlay.order == 2U &&
          overlay.exhaustive_facet_count == 3U &&
          overlay.exhaustive_coface_count == 1U &&
          overlay.critical_event_support_bound == 4U &&
          overlay.critical_arm_bound == 16U &&
          overlay.maximum_active_root_bound == 1U &&
          overlay.required_target_root_binding_capacity == 16U &&
          overlay.required_live_root_state_capacity == 2U &&
          overlay.required_live_root_facet_reference_capacity == 6U &&
          overlay.required_live_root_point_id_reference_capacity == 12U &&
          overlay.required_root_facet_replay_work_capacity == 12U &&
          overlay.required_root_point_id_replay_work_capacity == 24U &&
          overlay.required_target_facet_comparison_capacity == 12U &&
          overlay.required_target_point_id_comparison_capacity == 24U &&
          overlay.required_snapshot_facet_index_capacity == 12U &&
          overlay.required_snapshot_point_id_index_capacity == 24U,
      "q2 exposes the ten exact closed preflight bounds");
  check(
      overlay.target_root_bindings.size() ==
              journal.strict_target_records.size() &&
          overlay.target_root_bindings.size() == 2U &&
          overlay.counters.source_journal_verification_count == 1U &&
          overlay.counters.target_root_binding_count == 2U &&
          overlay.counters.matched_pre_batch_root_target_count == 0U &&
          overlay.counters.omitted_isolated_singleton_target_count == 2U &&
          overlay.counters.reduced_component_kind_postcheck_count == 2U &&
          overlay.counters.group_prior_root_membership_check_count == 0U,
      "q2 binds both strict targets once as omitted pre-batch singletons");

  bool dense_exact_omissions = true;
  for (std::size_t index = 0U;
       index < overlay.target_root_bindings.size();
       ++index) {
    const ExactCriticalCatalogTypedGammaTargetRootBinding& binding =
        overlay.target_root_bindings[index];
    const auto& target = journal.strict_target_records[index];
    dense_exact_omissions =
        dense_exact_omissions &&
        binding.target_root_binding_index == index &&
        binding.strict_target_record_index == index &&
        binding.history_batch_index == target.history_batch_index &&
        binding.history_group_record_index ==
            target.history_group_record_index &&
        binding.disposition ==
            ExactCriticalCatalogTypedGammaTargetDisposition::
                omitted_isolated_singleton &&
        !binding.root_node_id.has_value() &&
        target.strict_component.facet_point_ids.size() == 1U;
  }
  check(
      dense_exact_omissions,
      "q2 never mistakes the root created by the saddle lot for a pre-lot target");
}

void test_budget_seam_caps_and_domain_fail_closed(
    const CanonicalPointCloud& cloud,
    const ExactCriticalCatalogTypedGammaJournalResult& journal,
    const ExactCriticalCatalogTypedGammaRootOverlayBudget& budget) {
  ExactCriticalCatalogTypedGammaRootOverlayBudget mismatched = budget;
  --mismatched.typed_gamma_journal_budget.maximum_label_entry_count;
  const ExactCriticalCatalogTypedGammaRootOverlayResult seam_failure =
      build_exact_critical_catalog_typed_gamma_root_overlay(
          cloud, 2U, mismatched, journal);
  check(
      seam_failure.root_overlay_candidate_space_size_certified &&
          !seam_failure.source_journal_budget_seam_certified &&
          !seam_failure.root_overlay_preflight_budget_sufficient &&
          seam_failure.counters.source_journal_verification_count == 0U &&
          seam_failure.decision ==
              ExactCriticalCatalogTypedGammaRootOverlayDecision::
                  no_root_overlay_preflight_budget_insufficient &&
          all_certificates_close(
              verify_exact_critical_catalog_typed_gamma_root_overlay(
                  cloud,
                  2U,
                  mismatched,
                  journal,
                  seam_failure)),
      "a mismatched nested journal budget fails before source recertification");
  check_empty_payload(seam_failure, "mismatched source-budget seam");

  ExactCriticalCatalogTypedGammaRootOverlayBudget excessive = budget;
  excessive.maximum_live_root_state_count =
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_live_root_state_count +
      1U;
  check_invalid_argument(
      [&] {
        static_cast<void>(
            build_exact_critical_catalog_typed_gamma_root_overlay(
                cloud, 2U, excessive, journal));
      },
      "a root-overlay capacity above its static cap is rejected");

  ExactCriticalCatalogTypedGammaRootOverlayBudget excessive_nested =
      budget;
  excessive_nested.typed_gamma_journal_budget.maximum_label_entry_count =
      ExactCriticalCatalogTypedGammaJournalBudget::
          maximum_supported_label_entry_count +
      1U;
  check_invalid_argument(
      [&] {
        static_cast<void>(
            build_exact_critical_catalog_typed_gamma_root_overlay(
                cloud, 2U, excessive_nested, journal));
      },
      "a nested journal capacity above its static cap is rejected");

  check_invalid_argument(
      [&] {
        static_cast<void>(
            build_exact_critical_catalog_typed_gamma_root_overlay(
                cloud, 1U, budget, journal));
      },
      "order one is outside the root-overlay domain");
  check_invalid_argument(
      [&] {
        static_cast<void>(
            build_exact_critical_catalog_typed_gamma_root_overlay(
                cloud, cloud.size(), budget, journal));
      },
      "the terminal order is outside the root-overlay domain");
  const std::array<CertifiedPoint3, 2> two_point_input{
      point(-1.0), point(1.0)};
  const CanonicalPointCloud two_points = canonical_cloud(two_point_input);
  check_invalid_argument(
      [&] {
        static_cast<void>(
            build_exact_critical_catalog_typed_gamma_root_overlay(
                two_points, 2U, budget, journal));
      },
      "a two-point cloud is outside the root-overlay domain");
}

void test_preflight_atomicity(
    const CanonicalPointCloud& cloud,
    const ExactCriticalCatalogTypedGammaJournalResult& journal,
    const ExactCriticalCatalogTypedGammaRootOverlayBudget& budget,
    const ExactCriticalCatalogTypedGammaRootOverlayResult& successful) {
  using BudgetMember =
      std::size_t ExactCriticalCatalogTypedGammaRootOverlayBudget::*;
  using ResultMember =
      std::size_t ExactCriticalCatalogTypedGammaRootOverlayResult::*;
  struct Dimension {
    const char* name;
    BudgetMember maximum;
    ResultMember required;
  };
  const std::array<Dimension, 10> dimensions{{
      {"target-root bindings",
       &ExactCriticalCatalogTypedGammaRootOverlayBudget::
           maximum_target_root_binding_count,
       &ExactCriticalCatalogTypedGammaRootOverlayResult::
           required_target_root_binding_capacity},
      {"live root states",
       &ExactCriticalCatalogTypedGammaRootOverlayBudget::
           maximum_live_root_state_count,
       &ExactCriticalCatalogTypedGammaRootOverlayResult::
           required_live_root_state_capacity},
      {"live root facet references",
       &ExactCriticalCatalogTypedGammaRootOverlayBudget::
           maximum_live_root_facet_reference_count,
       &ExactCriticalCatalogTypedGammaRootOverlayResult::
           required_live_root_facet_reference_capacity},
      {"live root PointId references",
       &ExactCriticalCatalogTypedGammaRootOverlayBudget::
           maximum_live_root_point_id_reference_count,
       &ExactCriticalCatalogTypedGammaRootOverlayResult::
           required_live_root_point_id_reference_capacity},
      {"root facet replay work",
       &ExactCriticalCatalogTypedGammaRootOverlayBudget::
           maximum_root_facet_replay_work_count,
       &ExactCriticalCatalogTypedGammaRootOverlayResult::
           required_root_facet_replay_work_capacity},
      {"root PointId replay work",
       &ExactCriticalCatalogTypedGammaRootOverlayBudget::
           maximum_root_point_id_replay_work_count,
       &ExactCriticalCatalogTypedGammaRootOverlayResult::
           required_root_point_id_replay_work_capacity},
      {"target facet comparisons",
       &ExactCriticalCatalogTypedGammaRootOverlayBudget::
           maximum_target_facet_comparison_count,
       &ExactCriticalCatalogTypedGammaRootOverlayResult::
           required_target_facet_comparison_capacity},
      {"target PointId comparisons",
       &ExactCriticalCatalogTypedGammaRootOverlayBudget::
           maximum_target_point_id_comparison_count,
       &ExactCriticalCatalogTypedGammaRootOverlayResult::
           required_target_point_id_comparison_capacity},
      {"snapshot facet indices",
       &ExactCriticalCatalogTypedGammaRootOverlayBudget::
           maximum_snapshot_facet_index_count,
       &ExactCriticalCatalogTypedGammaRootOverlayResult::
           required_snapshot_facet_index_capacity},
      {"snapshot PointId indices",
       &ExactCriticalCatalogTypedGammaRootOverlayBudget::
           maximum_snapshot_point_id_index_count,
       &ExactCriticalCatalogTypedGammaRootOverlayResult::
           required_snapshot_point_id_index_capacity},
  }};

  for (const Dimension& dimension : dimensions) {
    const std::size_t required = successful.*(dimension.required);
    check(required > 0U, std::string{dimension.name} + " has a positive bound");
    if (required == 0U) {
      continue;
    }
    ExactCriticalCatalogTypedGammaRootOverlayBudget insufficient = budget;
    insufficient.*(dimension.maximum) = required - 1U;
    const ExactCriticalCatalogTypedGammaRootOverlayResult result =
        build_exact_critical_catalog_typed_gamma_root_overlay(
            cloud, 2U, insufficient, journal);
    check(
        result.root_overlay_candidate_space_size_certified &&
            !result.root_overlay_preflight_budget_sufficient &&
            result.counters.preflight_count == 1U &&
            result.counters.source_journal_verification_count == 0U &&
            result.counters.history_batch_replay_count == 0U &&
            result.decision ==
                ExactCriticalCatalogTypedGammaRootOverlayDecision::
                    no_root_overlay_preflight_budget_insufficient,
        std::string{dimension.name} +
            " fails before the external source journal is recertified");
    check_empty_payload(result, dimension.name);
    check(
        all_certificates_close(
            verify_exact_critical_catalog_typed_gamma_root_overlay(
                cloud, 2U, insufficient, journal, result)),
        std::string{dimension.name} +
            " diagnostic is accepted by independent replay");
  }
}

void test_all_domain_preflight_bounds() {
  std::array<std::size_t, 10U> observed_maxima{};
  for (std::size_t point_count = 3U; point_count <= 14U;
       ++point_count) {
    const CanonicalPointCloud cloud = line_cloud(point_count);
    for (std::size_t order = 2U;
         order < point_count && order <= 10U;
         ++order) {
      const std::size_t facet_count = binomial(point_count, order);
      const std::size_t coface_count =
          binomial(point_count, order + 1U);
      std::size_t event_support_bound = 0U;
      const std::size_t maximum_support_size =
          std::min({std::size_t{4U}, order + 1U, point_count});
      for (std::size_t support_size = 2U;
           support_size <= maximum_support_size;
           ++support_size) {
        event_support_bound += binomial(point_count, support_size);
      }
      const std::size_t maximum_active_root_bound =
          facet_count / (order + 1U);
      const std::array<std::size_t, 10U> expected{
          4U * event_support_bound,
          2U * maximum_active_root_bound,
          2U * facet_count,
          2U * order * facet_count,
          (facet_count + coface_count) * facet_count,
          order * (facet_count + coface_count) * facet_count,
          event_support_bound * facet_count,
          order * event_support_bound * facet_count,
          event_support_bound * facet_count,
          order * event_support_bound * facet_count,
      };

      const ExactCriticalCatalogTypedGammaJournalBudget journal_budget =
          full_journal_budget(1U);
      ExactCriticalCatalogTypedGammaJournalResult source;
      source.requested_budget = journal_budget;
      ExactCriticalCatalogTypedGammaRootOverlayBudget budget =
          full_overlay_budget(journal_budget);
      budget.maximum_target_root_binding_count = expected.front() - 1U;
      const ExactCriticalCatalogTypedGammaRootOverlayResult result =
          build_exact_critical_catalog_typed_gamma_root_overlay(
              cloud, order, budget, source);
      const std::array<std::size_t, 10U> observed{
          result.required_target_root_binding_capacity,
          result.required_live_root_state_capacity,
          result.required_live_root_facet_reference_capacity,
          result.required_live_root_point_id_reference_capacity,
          result.required_root_facet_replay_work_capacity,
          result.required_root_point_id_replay_work_capacity,
          result.required_target_facet_comparison_capacity,
          result.required_target_point_id_comparison_capacity,
          result.required_snapshot_facet_index_capacity,
          result.required_snapshot_point_id_index_capacity,
      };
      check(
          observed == expected &&
              result.root_overlay_candidate_space_size_certified &&
              !result.root_overlay_preflight_budget_sufficient &&
              result.counters.source_journal_verification_count == 0U &&
              result.decision ==
                  ExactCriticalCatalogTypedGammaRootOverlayDecision::
                      no_root_overlay_preflight_budget_insufficient &&
              result.target_root_bindings.empty(),
          "every bounded (n,k) pair derives the ten closed capacities "
          "before geometry");
      for (std::size_t dimension = 0U;
           dimension < observed.size();
           ++dimension) {
        observed_maxima[dimension] =
            std::max(observed_maxima[dimension], observed[dimension]);
      }
    }
  }

  const std::array<std::size_t, 10U> certified_maxima{
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_target_root_binding_count,
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_live_root_state_count,
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_live_root_facet_reference_count,
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_live_root_point_id_reference_count,
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_root_facet_replay_work_count,
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_root_point_id_replay_work_count,
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_target_facet_comparison_count,
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_target_point_id_comparison_count,
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_snapshot_facet_index_count,
      ExactCriticalCatalogTypedGammaRootOverlayBudget::
          maximum_supported_snapshot_point_id_index_count,
  };
  check(
      observed_maxima == certified_maxima,
      "the full bounded domain attains exactly the ten static maxima");
}

void test_source_diagnostics_are_atomic(
    const CanonicalPointCloud& cloud,
    const ExactCriticalCatalogTypedGammaJournalResult& complete_journal,
    const ExactCriticalCatalogTypedGammaRootOverlayBudget& complete_budget) {
  ExactCriticalCatalogTypedGammaJournalBudget incomplete_journal_budget =
      complete_budget.typed_gamma_journal_budget;
  incomplete_journal_budget.maximum_strict_target_record_count = 15U;
  const ExactCriticalCatalogTypedGammaJournalResult incomplete_journal =
      build_exact_critical_catalog_typed_gamma_journal(
          cloud, 2U, incomplete_journal_budget);
  ExactCriticalCatalogTypedGammaRootOverlayBudget incomplete_budget =
      full_overlay_budget(incomplete_journal_budget);
  const ExactCriticalCatalogTypedGammaRootOverlayResult incomplete =
      build_exact_critical_catalog_typed_gamma_root_overlay(
          cloud, 2U, incomplete_budget, incomplete_journal);
  check(
      incomplete_journal.decision ==
              ExactCriticalCatalogTypedGammaJournalDecision::
                  no_journal_preflight_budget_insufficient &&
          incomplete.root_overlay_preflight_budget_sufficient &&
          incomplete.source_journal_fresh_replay_certified &&
          incomplete.counters.source_journal_verification_count == 1U &&
          incomplete.counters.history_batch_replay_count == 0U &&
          incomplete.decision ==
              ExactCriticalCatalogTypedGammaRootOverlayDecision::
                  no_root_overlay_source_journal_incomplete,
      "a freshly certified but incomplete source journal fails closed before the root sweep");
  check_empty_payload(incomplete, "incomplete source journal");
  check(
      all_certificates_close(
          verify_exact_critical_catalog_typed_gamma_root_overlay(
              cloud,
              2U,
              incomplete_budget,
              incomplete_journal,
              incomplete)),
      "the incomplete-source diagnostic is freshly replayable");

  ExactCriticalCatalogTypedGammaJournalResult falsified = complete_journal;
  check(
      !falsified.strict_target_records.empty(),
      "the q2 source exposes a strict target to falsify");
  if (!falsified.strict_target_records.empty()) {
    falsified.strict_target_records.front()
        .strict_component.facet_point_ids.clear();
  }
  const ExactCriticalCatalogTypedGammaRootOverlayResult rejected =
      build_exact_critical_catalog_typed_gamma_root_overlay(
          cloud, 2U, complete_budget, falsified);
  check(
      rejected.root_overlay_preflight_budget_sufficient &&
          !rejected.source_journal_fresh_replay_certified &&
          rejected.counters.source_journal_verification_count == 1U &&
          rejected.counters.history_batch_replay_count == 0U &&
          rejected.decision ==
              ExactCriticalCatalogTypedGammaRootOverlayDecision::
                  no_root_overlay_source_journal_rejected,
      "a geometrically falsified source journal is rejected without root replay");
  check_empty_payload(rejected, "rejected source journal");
  check(
      all_certificates_close(
          verify_exact_critical_catalog_typed_gamma_root_overlay(
              cloud, 2U, complete_budget, falsified, rejected)),
      "the rejected-source diagnostic is itself freshly replayable");
}

void test_e5_multifusion_uses_only_frozen_prior_roots() {
  const CanonicalPointCloud cloud = e5_cloud();
  const ExactCriticalCatalogTypedGammaJournalBudget journal_budget =
      full_journal_budget(
          ExactFacetDescentChainBudget::
              maximum_supported_committed_strict_segment_count);
  const ExactCriticalCatalogTypedGammaJournalResult journal =
      build_exact_critical_catalog_typed_gamma_journal(
          cloud, 2U, journal_budget);
  const ExactCriticalCatalogTypedGammaRootOverlayBudget overlay_budget =
      full_overlay_budget(journal_budget);
  const ExactCriticalCatalogTypedGammaRootOverlayResult overlay =
      build_exact_critical_catalog_typed_gamma_root_overlay(
          cloud, 2U, overlay_budget, journal);
  const ExactCriticalCatalogTypedGammaRootOverlayVerification
      overlay_verification =
          verify_exact_critical_catalog_typed_gamma_root_overlay(
              cloud, 2U, overlay_budget, journal, overlay);

  check(
      complete_facts(overlay) && all_certificates_close(overlay_verification) &&
          journal.reduced_gamma_history.has_value(),
      "E5 builds a complete overlay over its external typed journal");
  if (!journal.reduced_gamma_history.has_value()) {
    return;
  }

  const auto& history = *journal.reduced_gamma_history;
  const auto multifusion = std::find_if(
      history.group_records.begin(),
      history.group_records.end(),
      [](const auto& group) {
        return group.kind == ExactReducedGammaBatchGroupKind::multifusion &&
               group.prior_root_node_ids ==
                   std::vector<std::size_t>({0U, 1U}) &&
               group.resulting_root_node_id ==
                   std::optional<std::size_t>{2U};
      });
  check(
      multifusion != history.group_records.end(),
      "E5 retains its binary multifusion from roots zero and one into root two");
  if (multifusion == history.group_records.end()) {
    return;
  }

  std::set<std::size_t> multifusion_target_roots;
  bool multifusion_bindings_are_prior = true;
  for (const ExactCriticalCatalogTypedGammaTargetRootBinding& binding :
       overlay.target_root_bindings) {
    if (binding.history_group_record_index !=
        multifusion->group_record_index) {
      continue;
    }
    multifusion_bindings_are_prior =
        multifusion_bindings_are_prior &&
        binding.history_batch_index == multifusion->batch_index &&
        binding.disposition ==
            ExactCriticalCatalogTypedGammaTargetDisposition::
                matched_pre_batch_persistent_reduced_root &&
        binding.root_node_id.has_value() &&
        *binding.root_node_id != 2U;
    if (binding.root_node_id.has_value()) {
      multifusion_target_roots.insert(*binding.root_node_id);
    }
  }
  check(
      multifusion_bindings_are_prior &&
          multifusion_target_roots == std::set<std::size_t>({0U, 1U}),
      "E5 multifusion targets roots zero and one from the frozen snapshot, never newly created root two");

  std::optional<std::size_t> root_zero_binding_index;
  std::optional<std::size_t> root_one_binding_index;
  for (std::size_t binding_index = 0U;
       binding_index < overlay.target_root_bindings.size();
       ++binding_index) {
    const ExactCriticalCatalogTypedGammaTargetRootBinding& binding =
        overlay.target_root_bindings[binding_index];
    if (binding.history_group_record_index !=
        multifusion->group_record_index) {
      continue;
    }
    if (binding.root_node_id == std::optional<std::size_t>{0U}) {
      root_zero_binding_index = binding_index;
    } else if (binding.root_node_id == std::optional<std::size_t>{1U}) {
      root_one_binding_index = binding_index;
    }
  }
  check(
      root_zero_binding_index.has_value() &&
          root_one_binding_index.has_value(),
      "E5 exposes one multifusion target family for each prior root");
  if (root_zero_binding_index.has_value() &&
      root_one_binding_index.has_value()) {
    ExactCriticalCatalogTypedGammaRootOverlayResult swapped = overlay;
    swapped.target_root_bindings[*root_zero_binding_index].root_node_id =
        1U;
    swapped.target_root_bindings[*root_one_binding_index].root_node_id =
        0U;
    const ExactCriticalCatalogTypedGammaRootOverlayVerification
        swap_verification =
            verify_exact_critical_catalog_typed_gamma_root_overlay(
                cloud, 2U, overlay_budget, journal, swapped);
    check(
        !swap_verification.target_root_bindings_certified &&
            !swap_verification.fresh_replay_certified &&
            !swap_verification.
                exact_critical_catalog_typed_gamma_root_overlay_decision_certified,
        "fresh replay rejects swapping roots zero and one between exact target families");
  }

  const auto continuation = std::find_if(
      history.group_records.begin(),
      history.group_records.end(),
      [&](const auto& group) {
        const bool has_strict_target = std::any_of(
            journal.strict_target_records.begin(),
            journal.strict_target_records.end(),
            [&](const auto& target) {
              return target.history_group_record_index ==
                     group.group_record_index;
            });
        return group.batch_index > multifusion->batch_index &&
               group.kind == ExactReducedGammaBatchGroupKind::continuation &&
               group.prior_root_node_ids ==
                   std::vector<std::size_t>({2U}) &&
               group.resulting_root_node_id ==
                   std::optional<std::size_t>{2U} &&
               has_strict_target;
      });
  check(
      continuation != history.group_records.end(),
      "E5 retains a later target-bearing continuation of root two");
  if (continuation == history.group_records.end()) {
    return;
  }

  std::size_t continuation_binding_count = 0U;
  bool continuation_bindings_use_root_two = true;
  for (const ExactCriticalCatalogTypedGammaTargetRootBinding& binding :
       overlay.target_root_bindings) {
    if (binding.history_group_record_index !=
        continuation->group_record_index) {
      continue;
    }
    ++continuation_binding_count;
    continuation_bindings_use_root_two =
        continuation_bindings_use_root_two &&
        binding.disposition ==
            ExactCriticalCatalogTypedGammaTargetDisposition::
                matched_pre_batch_persistent_reduced_root &&
        binding.root_node_id == std::optional<std::size_t>{2U};
  }
  check(
      continuation_binding_count > 0U &&
          continuation_bindings_use_root_two,
      "a later E5 continuation may target root two once it belongs to the pre-batch snapshot");
}

void test_fresh_verifier_rejects_composite_mutations_and_twin(
    const CanonicalPointCloud& cloud,
    const ExactCriticalCatalogTypedGammaJournalResult& journal,
    const ExactCriticalCatalogTypedGammaRootOverlayBudget& budget,
    const ExactCriticalCatalogTypedGammaRootOverlayResult& baseline) {
  check(
      complete_facts(baseline) &&
          baseline.target_root_bindings.size() == 2U,
      "the q2 root-overlay mutation baseline is complete and compact");
  if (baseline.target_root_bindings.size() == 2U) {
    ExactCriticalCatalogTypedGammaRootOverlayResult mutated = baseline;
    --mutated.requested_budget.maximum_target_root_binding_count;
    ++mutated.required_live_root_state_capacity;
    mutated.source_journal_decision =
        ExactCriticalCatalogTypedGammaJournalDecision::
            no_journal_arm_overlay_incomplete;
    mutated.target_root_bindings.front().disposition =
        ExactCriticalCatalogTypedGammaTargetDisposition::
            matched_pre_batch_persistent_reduced_root;
    mutated.target_root_bindings.front().root_node_id = 0U;
    mutated.targets_resolved_against_frozen_pre_batch_snapshots = false;
    ++mutated.counters.matched_pre_batch_root_target_count;
    mutated.decision =
        ExactCriticalCatalogTypedGammaRootOverlayDecision::
            no_root_overlay_source_journal_incomplete;
    mutated.scope =
        ExactCriticalCatalogTypedGammaRootOverlayScope::unspecified;

    const ExactCriticalCatalogTypedGammaRootOverlayVerification verification =
        verify_exact_critical_catalog_typed_gamma_root_overlay(
            cloud, 2U, budget, journal, mutated);
    check(
        !verification.requested_budget_certified &&
            verification.external_inputs_certified &&
            !verification.derived_preflight_sizes_certified &&
            !verification.source_journal_decision_certified &&
            verification.source_journal_fresh_replay_certified &&
            !verification.target_root_bindings_certified &&
            !verification.result_facts_certified &&
            !verification.counters_certified &&
            !verification.decision_certified &&
            !verification.scope_certified &&
            !verification.fresh_replay_certified &&
            !verification.
                exact_critical_catalog_typed_gamma_root_overlay_decision_certified,
        "one fresh replay rejects simultaneous mutations of every compact overlay layer");
  }

  const std::array<CertifiedPoint3, 3> twin_input{
      point(-2.0, 0.0), point(0.0, 1.25), point(2.0, 0.0)};
  const CanonicalPointCloud twin = canonical_cloud(twin_input);
  const ExactCriticalCatalogTypedGammaRootOverlayVerification twin_check =
      verify_exact_critical_catalog_typed_gamma_root_overlay(
          twin, 2U, budget, journal, baseline);
  check(
      !twin_check.source_journal_fresh_replay_certified &&
          !twin_check.fresh_replay_certified &&
          !twin_check.
              exact_critical_catalog_typed_gamma_root_overlay_decision_certified,
      "a same-size twin cloud cannot reuse either the external journal or its local root overlay");
}

}  // namespace

int main() {
  const CanonicalPointCloud q2 = q2_triangle_cloud();
  const ExactCriticalCatalogTypedGammaJournalBudget journal_budget =
      full_journal_budget(1U);
  const ExactCriticalCatalogTypedGammaJournalResult journal =
      build_exact_critical_catalog_typed_gamma_journal(
          q2, 2U, journal_budget);
  const ExactCriticalCatalogTypedGammaRootOverlayBudget overlay_budget =
      full_overlay_budget(journal_budget);
  const ExactCriticalCatalogTypedGammaRootOverlayResult overlay =
      build_exact_critical_catalog_typed_gamma_root_overlay(
          q2, 2U, overlay_budget, journal);

  test_q2_complete_overlay(
      q2, journal_budget, journal, overlay_budget, overlay);
  test_preflight_atomicity(q2, journal, overlay_budget, overlay);
  test_all_domain_preflight_bounds();
  test_budget_seam_caps_and_domain_fail_closed(
      q2, journal, overlay_budget);
  test_source_diagnostics_are_atomic(
      q2, journal, overlay_budget);
  test_e5_multifusion_uses_only_frozen_prior_roots();
  test_fresh_verifier_rejects_composite_mutations_and_twin(
      q2, journal, overlay_budget, overlay);
  if (failures != 0) {
    std::cerr << failures << " typed Gamma root-overlay test(s) failed\n";
    return 1;
  }
  std::cout << "all typed Gamma root-overlay tests passed\n";
  return 0;
}
