#include "morsehgp3d/hierarchy/critical_catalog_typed_gamma_durable_arm_key_catalog.hpp"

#include <array>
#include <cstddef>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::contract::CanonicalId;
using morsehgp3d::exact::CertifiedPoint3;
using namespace morsehgp3d::hierarchy;
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

[[nodiscard]] CanonicalPointCloud q2_triangle_cloud(bool reversed = false) {
  const std::array<CertifiedPoint3, 3> forward{
      point(-2.0, 0.0), point(0.0, 1.0), point(2.0, 0.0)};
  const std::array<CertifiedPoint3, 3> backward{
      point(2.0, 0.0), point(0.0, 1.0), point(-2.0, 0.0)};
  return reversed ? canonical_cloud(backward) : canonical_cloud(forward);
}

[[nodiscard]] CanonicalPointCloud mirror_cloud() {
  const std::array<CertifiedPoint3, 4> input{
      point(-2.0, 0.0),
      point(0.0, -3.0),
      point(0.0, 3.0),
      point(2.0, 0.0)};
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
full_journal_budget() {
  ExactCriticalCatalogTypedGammaJournalBudget budget;
  budget.provenance_overlay_budget = full_provenance_budget();
  budget.arm_overlay_budget = full_arm_budget(1U);
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

[[nodiscard]] ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget
full_key_budget(
    const ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget& path_budget) {
  ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget budget;
  budget.path_overlay_budget = path_budget;
  budget.maximum_event_key_record_count =
      ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget::
          maximum_supported_event_key_record_count;
  budget.maximum_arm_key_record_count =
      ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget::
          maximum_supported_arm_key_record_count;
  budget.maximum_event_arm_key_reference_count =
      ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget::
          maximum_supported_event_arm_key_reference_count;
  budget.maximum_event_projection_point_id_reference_count =
      ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget::
          maximum_supported_event_projection_point_id_reference_count;
  return budget;
}

struct SourceStack {
  ExactCriticalCatalogTypedGammaJournalBudget journal_budget;
  ExactCriticalCatalogTypedGammaJournalResult journal;
  ExactCriticalCatalogTypedGammaRootOverlayBudget root_budget;
  ExactCriticalCatalogTypedGammaRootOverlayResult root_overlay;
  ExactCriticalCatalogTypedGammaArmRootCompositionBudget composition_budget;
  ExactCriticalCatalogTypedGammaArmRootCompositionResult composition;
  ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget path_budget;
  ExactCriticalCatalogTypedGammaArmRootPathOverlayResult paths;
};

[[nodiscard]] SourceStack build_source_stack(
    const CanonicalPointCloud& cloud) {
  SourceStack stack;
  stack.journal_budget = full_journal_budget();
  stack.journal = build_exact_critical_catalog_typed_gamma_journal(
      cloud, 2U, stack.journal_budget);
  stack.root_budget = full_root_overlay_budget(stack.journal_budget);
  stack.root_overlay = build_exact_critical_catalog_typed_gamma_root_overlay(
      cloud, 2U, stack.root_budget, stack.journal);
  stack.composition_budget = full_composition_budget(stack.root_budget);
  stack.composition =
      build_exact_critical_catalog_typed_gamma_arm_root_composition(
          cloud,
          2U,
          stack.composition_budget,
          stack.journal,
          stack.root_overlay);
  stack.path_budget = full_path_budget(stack.composition_budget);
  stack.paths = build_exact_critical_catalog_typed_gamma_arm_root_path_overlay(
      cloud,
      2U,
      stack.path_budget,
      stack.journal,
      stack.root_overlay,
      stack.composition);
  return stack;
}

[[nodiscard]] bool all_certificates_close(
    const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogVerification&
        verification) {
  return verification.requested_budget_certified &&
         verification.external_inputs_certified &&
         verification.derived_preflight_sizes_certified &&
         verification.source_path_overlay_decision_certified &&
         verification.source_path_overlay_fresh_replay_certified &&
         verification.event_key_records_certified &&
         verification.arm_key_records_certified &&
         verification.result_facts_certified &&
         verification.counters_certified &&
         verification.decision_certified && verification.scope_certified &&
         verification.fresh_replay_certified &&
         verification.
             exact_critical_catalog_typed_gamma_durable_arm_key_catalog_decision_certified;
}

[[nodiscard]] bool complete_facts(
    const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogResult& result) {
  return result.durable_key_conservative_preflight_bounds_certified &&
         result.durable_key_preflight_budget_sufficient &&
         result.all_four_external_budget_seams_certified &&
         result.source_path_overlay_is_external_and_not_retained &&
         result.source_path_overlay_fresh_replay_certified &&
         result.reconstruction_started_only_after_complete_source_path_overlay &&
         result.transient_critical_catalog_fresh_replay_certified &&
         result.
             event_identity_projections_are_complete_schema_version_free_v2_keys &&
         result.critical_event_ids_are_domain_separated_sha256_v2 &&
         result.event_hash_collisions_checked_against_complete_projections &&
         result.every_requested_order_saddle_has_one_durable_event_key &&
         result.
             every_complete_shell_has_exactly_one_sorted_durable_arm_tuple_per_point &&
         result.arm_tuples_biject_replayable_source_paths &&
         result.event_to_arm_aggregation_is_complete_and_canonical &&
         result.identities_exclude_paths_targets_reduced_roots_and_local_indices &&
         result.
             records_are_internal_keys_and_not_public_attachments_or_equal_level_batches &&
         result.diagnostic_outcomes_have_no_key_payload &&
         result.
             critical_catalog_typed_gamma_durable_arm_key_catalog_certified &&
         result.decision ==
             ExactCriticalCatalogTypedGammaDurableArmKeyCatalogDecision::
                 complete_exhaustive_single_order_durable_arm_key_catalog;
}

void test_q2_complete_and_permutation_invariant(
    const CanonicalPointCloud& q2,
    const SourceStack& source,
    const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget& budget,
    const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogResult& result) {
  const auto verification =
      verify_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
          q2,
          2U,
          budget,
          source.journal,
          source.root_overlay,
          source.composition,
          source.paths,
          result);
  check(
      complete_facts(result) && all_certificates_close(verification) &&
          std::string{
              ExactCriticalCatalogTypedGammaDurableArmKeyCatalogResult::
                  proof_basis} ==
              "v2_domain_separated_sha256_critical_event_keys_with_full_"
              "projection_collision_checks_and_exhaustive_single_order_arm_"
              "identity_tuple_catalog_v1",
      "q2 closes the bounded durable event and arm-tuple key catalog");
  check(
      result.point_count == 3U && result.order == 2U &&
          result.critical_event_support_bound == 4U &&
          result.critical_arm_bound == 16U &&
          result.required_event_key_record_capacity == 4U &&
          result.required_arm_key_record_capacity == 16U &&
          result.required_event_arm_key_reference_capacity == 16U &&
          result.required_event_projection_point_id_reference_capacity ==
              28U &&
          result.event_key_records.size() == 1U &&
          result.arm_key_records.size() == 2U,
      "q2 exposes all four conservative capacities and one two-arm event");
  if (result.event_key_records.size() == 1U &&
      result.arm_key_records.size() == 2U) {
    const auto& event = result.event_key_records.front();
    check(
        event.event_id.to_lower_hex() ==
                "f6acb8180318d9879b84d5ebfd4368ca9841b9dc767be97eeaf4b51fedf1d88c" &&
            event.identity_projection.interior_point_ids ==
                std::vector<morsehgp3d::spatial::PointId>{1U} &&
            event.identity_projection.shell_point_ids ==
                std::vector<morsehgp3d::spatial::PointId>{0U, 2U} &&
            event.identity_projection.minimal_support_point_ids ==
                std::vector<morsehgp3d::spatial::PointId>{0U, 2U} &&
            event.arm_key_record_indices ==
                std::vector<std::size_t>{0U, 1U} &&
            result.arm_key_records[0].durable_key.event_id == event.event_id &&
            result.arm_key_records[0].durable_key.order == 2U &&
            result.arm_key_records[0]
                    .durable_key.removed_shell_point_id == 0U &&
            result.arm_key_records[1]
                    .durable_key.removed_shell_point_id == 2U,
        "q2 matches the independent Python event digest and exact shell tuple");
  }
  check(
      result.counters.preflight_count == 1U &&
          result.counters.source_path_overlay_verification_count == 1U &&
          result.counters.critical_catalog_build_count == 1U &&
          result.counters.saddle_event_reconciliation_count == 1U &&
          result.counters.event_projection_count == 1U &&
          result.counters.event_id_hash_count == 1U &&
          result.counters.event_hash_semantic_comparison_count == 0U &&
          result.counters.arm_path_key_reconciliation_count == 2U &&
          result.counters.event_key_record_count == 1U &&
          result.counters.arm_key_record_count == 2U &&
          result.counters.event_arm_key_reference_count == 2U &&
          result.counters.event_projection_point_id_reference_count == 5U,
      "q2 accounts for every projection, hash, path join and aggregation");

  const CanonicalPointCloud reversed = q2_triangle_cloud(true);
  const SourceStack reversed_source = build_source_stack(reversed);
  const auto reversed_budget = full_key_budget(reversed_source.path_budget);
  const auto reversed_result =
      build_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
          reversed,
          2U,
          reversed_budget,
          reversed_source.journal,
          reversed_source.root_overlay,
          reversed_source.composition,
          reversed_source.paths);
  check(
      result == reversed_result,
      "reversing input arrival order preserves the complete canonical catalog");
}

void test_four_local_preflight_caps(
    const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget&
        full_budget) {
  const CanonicalPointCloud cloud = line_cloud(4U);
  ExactCriticalCatalogTypedGammaJournalResult empty_journal;
  empty_journal.requested_budget =
      full_budget.path_overlay_budget.arm_root_composition_budget
          .root_overlay_budget.typed_gamma_journal_budget;
  ExactCriticalCatalogTypedGammaRootOverlayResult empty_root;
  empty_root.requested_budget =
      full_budget.path_overlay_budget.arm_root_composition_budget
          .root_overlay_budget;
  ExactCriticalCatalogTypedGammaArmRootCompositionResult empty_composition;
  empty_composition.requested_budget =
      full_budget.path_overlay_budget.arm_root_composition_budget;
  ExactCriticalCatalogTypedGammaArmRootPathOverlayResult empty_paths;
  empty_paths.requested_budget = full_budget.path_overlay_budget;
  std::array<ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget, 4>
      insufficient{
          full_budget, full_budget, full_budget, full_budget};
  insufficient[0].maximum_event_key_record_count = 9U;
  insufficient[1].maximum_arm_key_record_count = 39U;
  insufficient[2].maximum_event_arm_key_reference_count = 39U;
  insufficient[3].maximum_event_projection_point_id_reference_count = 69U;
  for (const auto& budget : insufficient) {
    const auto result =
        build_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
            cloud,
            2U,
            budget,
            empty_journal,
            empty_root,
            empty_composition,
            empty_paths);
    const auto verification =
        verify_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
            cloud,
            2U,
            budget,
            empty_journal,
            empty_root,
            empty_composition,
            empty_paths,
            result);
    check(
        result.critical_event_support_bound == 10U &&
            result.critical_arm_bound == 40U &&
            result.required_event_key_record_capacity == 10U &&
            result.required_arm_key_record_capacity == 40U &&
            result.required_event_arm_key_reference_capacity == 40U &&
            result.required_event_projection_point_id_reference_capacity ==
                70U &&
            !result.durable_key_preflight_budget_sufficient &&
            result.counters.source_path_overlay_verification_count == 0U &&
            result.event_key_records.empty() &&
            result.arm_key_records.empty() &&
            result.decision ==
                ExactCriticalCatalogTypedGammaDurableArmKeyCatalogDecision::
                    no_durable_arm_key_catalog_preflight_budget_insufficient &&
            all_certificates_close(verification),
        "each one-below durable-key capacity fails before source replay");
  }
}

void test_local_caps_and_external_seam_rejected(
    const CanonicalPointCloud& cloud,
    const SourceStack& source,
    const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget& budget) {
  std::array<ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget, 4>
      excessive{budget, budget, budget, budget};
  excessive[0].maximum_event_key_record_count =
      ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget::
          maximum_supported_event_key_record_count +
      1U;
  excessive[1].maximum_arm_key_record_count =
      ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget::
          maximum_supported_arm_key_record_count +
      1U;
  excessive[2].maximum_event_arm_key_reference_count =
      ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget::
          maximum_supported_event_arm_key_reference_count +
      1U;
  excessive[3].maximum_event_projection_point_id_reference_count =
      ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget::
          maximum_supported_event_projection_point_id_reference_count +
      1U;
  for (const auto& invalid_budget : excessive) {
    check_invalid_argument(
        [&] {
          static_cast<void>(
              build_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
                  cloud,
                  2U,
                  invalid_budget,
                  source.journal,
                  source.root_overlay,
                  source.composition,
                  source.paths));
        },
        "each durable-key capacity above its static cap is rejected");
  }

  auto mismatched_paths = source.paths;
  --mismatched_paths.requested_budget.maximum_path_record_count;
  const auto mismatch =
      build_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
          cloud,
          2U,
          budget,
          source.journal,
          source.root_overlay,
          source.composition,
          mismatched_paths);
  check(
      !mismatch.all_four_external_budget_seams_certified &&
          mismatch.durable_key_preflight_budget_sufficient &&
          mismatch.counters.source_path_overlay_verification_count == 0U &&
          mismatch.event_key_records.empty() &&
          mismatch.arm_key_records.empty() &&
          mismatch.decision ==
              ExactCriticalCatalogTypedGammaDurableArmKeyCatalogDecision::
                  no_durable_arm_key_catalog_external_budget_seam_mismatch &&
          all_certificates_close(
              verify_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
                  cloud,
                  2U,
                  budget,
                  source.journal,
                  source.root_overlay,
                  source.composition,
                  mismatched_paths,
                  mismatch)),
      "a discordant 6.21 budget seam has a distinct atomic diagnostic");
}

void test_mirror_multi_event_aggregation() {
  const CanonicalPointCloud cloud = mirror_cloud();
  const SourceStack source = build_source_stack(cloud);
  const auto budget = full_key_budget(source.path_budget);
  const auto result =
      build_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
          cloud,
          2U,
          budget,
          source.journal,
          source.root_overlay,
          source.composition,
          source.paths);
  bool event_ranges_close = result.event_key_records.size() == 2U &&
                            result.arm_key_records.size() == 6U;
  for (const auto& event : result.event_key_records) {
    event_ranges_close =
        event_ranges_close && event.arm_key_record_indices.size() == 3U;
    for (const std::size_t arm_index : event.arm_key_record_indices) {
      event_ranges_close =
          event_ranges_close && arm_index < result.arm_key_records.size() &&
          result.arm_key_records[arm_index].event_key_record_index ==
              event.event_key_record_index &&
          result.arm_key_records[arm_index].durable_key.event_id ==
              event.event_id;
    }
  }
  check(
      complete_facts(result) && event_ranges_close &&
          result.event_key_records[0].event_id <
              result.event_key_records[1].event_id &&
          result.event_key_records[0]
                  .identity_projection.squared_level_exact ==
              result.event_key_records[1]
                  .identity_projection.squared_level_exact &&
          result.counters.event_key_record_count == 2U &&
          result.counters.arm_key_record_count == 6U &&
          result.counters.event_arm_key_reference_count == 6U,
      "the mirror lot sorts two simultaneous event IDs and aggregates all six arms");
}

void test_rejected_source_and_observed_key_mutation(
    const CanonicalPointCloud& cloud,
    const SourceStack& source,
    const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget& budget,
    const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogResult& baseline) {
  auto falsified_paths = source.paths;
  check(
      !falsified_paths.path_records.empty(),
      "the q2 durable-key source exposes a path to falsify");
  if (!falsified_paths.path_records.empty()) {
    falsified_paths.path_records.front().regular_active_terminal_certified =
        false;
  }
  const auto rejected =
      build_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
          cloud,
          2U,
          budget,
          source.journal,
          source.root_overlay,
          source.composition,
          falsified_paths);
  check(
      rejected.durable_key_preflight_budget_sufficient &&
          !rejected.source_path_overlay_fresh_replay_certified &&
          rejected.event_key_records.empty() &&
          rejected.arm_key_records.empty() &&
          rejected.decision ==
              ExactCriticalCatalogTypedGammaDurableArmKeyCatalogDecision::
                  no_durable_arm_key_catalog_source_path_overlay_rejected &&
          all_certificates_close(
              verify_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
                  cloud,
                  2U,
                  budget,
                  source.journal,
                  source.root_overlay,
                  source.composition,
                  falsified_paths,
                  rejected)),
      "a falsified 6.21 path source yields only an atomic diagnostic");

  check(
      complete_facts(baseline) && !baseline.event_key_records.empty(),
      "the q2 durable-key mutation baseline is complete");
  if (!baseline.event_key_records.empty()) {
    auto mutated = baseline;
    mutated.event_key_records.front().event_id = CanonicalId::from_lower_hex(
        "0000000000000000000000000000000000000000000000000000000000000000");
    const auto verification =
        verify_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
            cloud,
            2U,
            budget,
            source.journal,
            source.root_overlay,
            source.composition,
            source.paths,
            mutated);
    check(
        verification.requested_budget_certified &&
            verification.external_inputs_certified &&
            verification.derived_preflight_sizes_certified &&
            verification.source_path_overlay_decision_certified &&
            verification.source_path_overlay_fresh_replay_certified &&
            !verification.event_key_records_certified &&
            verification.arm_key_records_certified &&
            verification.result_facts_certified &&
            verification.counters_certified &&
            verification.decision_certified &&
            verification.scope_certified &&
            !verification.fresh_replay_certified,
        "fresh replay rejects one altered canonical event identifier");
  }
  if (!baseline.arm_key_records.empty()) {
    auto mutated = baseline;
    ++mutated.arm_key_records.front().durable_key.removed_shell_point_id;
    const auto verification =
        verify_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
            cloud,
            2U,
            budget,
            source.journal,
            source.root_overlay,
            source.composition,
            source.paths,
            mutated);
    check(
        verification.event_key_records_certified &&
            !verification.arm_key_records_certified &&
            !verification.fresh_replay_certified,
        "fresh replay rejects one altered durable arm tuple");
  }
}

}  // namespace

int main() {
  const CanonicalPointCloud q2 = q2_triangle_cloud();
  const SourceStack source = build_source_stack(q2);
  const auto budget = full_key_budget(source.path_budget);
  const auto result =
      build_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
          q2,
          2U,
          budget,
          source.journal,
          source.root_overlay,
          source.composition,
          source.paths);

  test_q2_complete_and_permutation_invariant(q2, source, budget, result);
  test_four_local_preflight_caps(budget);
  test_local_caps_and_external_seam_rejected(q2, source, budget);
  test_mirror_multi_event_aggregation();
  auto excessive = budget;
  excessive.path_overlay_budget.maximum_path_record_count =
      ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget::
          maximum_supported_path_record_count +
      1U;
  check_invalid_argument(
      [&] {
        static_cast<void>(
            build_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
                q2,
                2U,
                excessive,
                source.journal,
                source.root_overlay,
                source.composition,
                source.paths));
      },
      "an excessive nested 6.21 cap is rejected recursively");
  test_rejected_source_and_observed_key_mutation(
      q2, source, budget, result);

  if (failures != 0) {
    std::cerr << failures << " durable arm-key catalog test(s) failed\n";
    return 1;
  }
  std::cout << "all durable arm-key catalog tests passed\n";
  return 0;
}
