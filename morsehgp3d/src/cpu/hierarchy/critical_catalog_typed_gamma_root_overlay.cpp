#include "morsehgp3d/hierarchy/critical_catalog_typed_gamma_root_overlay.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;
using FacetLabel = std::vector<PointId>;
using FacetSet = std::vector<FacetLabel>;
struct ReplayRoot {
  std::size_t root_node_id{};
  FacetSet facet_point_ids;
};
using ActiveRoot = ReplayRoot;
using ActiveRootMap = std::map<std::size_t, ActiveRoot>;
using RootOverlayBudget =
    ExactCriticalCatalogTypedGammaRootOverlayBudget;
using RootOverlayDecision =
    ExactCriticalCatalogTypedGammaRootOverlayDecision;
using RootOverlayResult =
    ExactCriticalCatalogTypedGammaRootOverlayResult;
using RootOverlayScope = ExactCriticalCatalogTypedGammaRootOverlayScope;
using TargetBinding =
    ExactCriticalCatalogTypedGammaTargetRootBinding;
using TargetDisposition =
    ExactCriticalCatalogTypedGammaTargetDisposition;
using JournalBudget = ExactCriticalCatalogTypedGammaJournalBudget;
using JournalDecision = ExactCriticalCatalogTypedGammaJournalDecision;
using JournalResult = ExactCriticalCatalogTypedGammaJournalResult;

struct PendingRootMutation {
  std::vector<std::size_t> prior_root_ids;
  std::size_t resulting_root_id{};
  ActiveRoot resulting_root;
};

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(std::string{message});
  }
  return left + right;
}

void checked_accumulate(
    std::size_t& value,
    std::size_t increment,
    std::string_view message) {
  value = checked_add(value, increment, message);
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
        "the typed Gamma root-overlay binomial coefficient overflows");
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
          RootOverlayBudget::
              maximum_supported_snapshot_facet_index_count ||
      budget.maximum_snapshot_point_id_index_count >
          RootOverlayBudget::
              maximum_supported_snapshot_point_id_index_count) {
    throw std::invalid_argument(
        "a typed Gamma root-overlay capacity exceeds its bounded cap");
  }
}

void validate_domain(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const RootOverlayBudget& budget) {
  if (cloud.size() < RootOverlayResult::minimum_supported_point_count ||
      cloud.size() > RootOverlayResult::maximum_supported_point_count ||
      order < RootOverlayResult::minimum_supported_order ||
      order > RootOverlayResult::maximum_supported_order ||
      order >= cloud.size()) {
    throw std::invalid_argument(
        "the typed critical-catalog Gamma root overlay requires "
        "3<=n<=14, 2<=k<n and k<=10");
  }
  validate_root_overlay_budget_caps(budget);
}

void derive_root_overlay_preflight(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    RootOverlayResult& result) {
  result.exhaustive_facet_count = bounded_binomial(cloud.size(), order);
  result.exhaustive_coface_count =
      bounded_binomial(cloud.size(), order + 1U);
  const std::size_t level_bound = checked_add(
      result.exhaustive_facet_count,
      result.exhaustive_coface_count,
      "the typed Gamma root-overlay level bound overflows");
  const std::size_t maximum_support_size =
      std::min({std::size_t{4U}, order + 1U, cloud.size()});
  for (std::size_t support_size = 2U;
       support_size <= maximum_support_size;
       ++support_size) {
    result.critical_event_support_bound = checked_add(
        result.critical_event_support_bound,
        bounded_binomial(cloud.size(), support_size),
        "the typed Gamma root-overlay event bound overflows");
  }
  result.critical_arm_bound = checked_multiply(
      4U,
      result.critical_event_support_bound,
      "the typed Gamma root-overlay arm bound overflows");
  result.maximum_active_root_bound =
      result.exhaustive_facet_count / (order + 1U);
  result.required_target_root_binding_capacity =
      result.critical_arm_bound;
  result.required_live_root_state_capacity = checked_multiply(
      2U,
      result.maximum_active_root_bound,
      "the live root-state bound overflows");
  result.required_live_root_facet_reference_capacity = checked_multiply(
      2U,
      result.exhaustive_facet_count,
      "the live root-facet-reference bound overflows");
  result.required_live_root_point_id_reference_capacity = checked_multiply(
      order,
      result.required_live_root_facet_reference_capacity,
      "the live root PointId-reference bound overflows");
  result.required_root_facet_replay_work_capacity = checked_multiply(
      level_bound,
      result.exhaustive_facet_count,
      "the root facet-replay work bound overflows");
  result.required_root_point_id_replay_work_capacity = checked_multiply(
      order,
      result.required_root_facet_replay_work_capacity,
      "the root PointId-replay work bound overflows");
  result.required_target_facet_comparison_capacity = checked_multiply(
      result.critical_event_support_bound,
      result.exhaustive_facet_count,
      "the target facet-comparison bound overflows");
  result.required_target_point_id_comparison_capacity = checked_multiply(
      order,
      result.required_target_facet_comparison_capacity,
      "the target PointId-comparison bound overflows");
  result.required_snapshot_facet_index_capacity =
      result.required_target_facet_comparison_capacity;
  result.required_snapshot_point_id_index_capacity =
      result.required_target_point_id_comparison_capacity;

  if (result.required_target_root_binding_capacity >
          RootOverlayBudget::
              maximum_supported_target_root_binding_count ||
      result.required_live_root_state_capacity >
          RootOverlayBudget::maximum_supported_live_root_state_count ||
      result.required_live_root_facet_reference_capacity >
          RootOverlayBudget::
              maximum_supported_live_root_facet_reference_count ||
      result.required_live_root_point_id_reference_capacity >
          RootOverlayBudget::
              maximum_supported_live_root_point_id_reference_count ||
      result.required_root_facet_replay_work_capacity >
          RootOverlayBudget::
              maximum_supported_root_facet_replay_work_count ||
      result.required_root_point_id_replay_work_capacity >
          RootOverlayBudget::
              maximum_supported_root_point_id_replay_work_count ||
      result.required_target_facet_comparison_capacity >
          RootOverlayBudget::
              maximum_supported_target_facet_comparison_count ||
      result.required_target_point_id_comparison_capacity >
          RootOverlayBudget::
              maximum_supported_target_point_id_comparison_count ||
      result.required_snapshot_facet_index_capacity >
          RootOverlayBudget::
              maximum_supported_snapshot_facet_index_count ||
      result.required_snapshot_point_id_index_capacity >
          RootOverlayBudget::
              maximum_supported_snapshot_point_id_index_count) {
    throw std::logic_error(
        "the derived typed Gamma root-overlay preflight exceeds a "
        "certified static cap");
  }
}

[[nodiscard]] bool budget_covers_preflight(
    const RootOverlayBudget& budget,
    const RootOverlayResult& result) {
  return budget.maximum_target_root_binding_count >=
             result.required_target_root_binding_capacity &&
         budget.maximum_live_root_state_count >=
             result.required_live_root_state_capacity &&
         budget.maximum_live_root_facet_reference_count >=
             result.required_live_root_facet_reference_capacity &&
         budget.maximum_live_root_point_id_reference_count >=
             result.required_live_root_point_id_reference_capacity &&
         budget.maximum_root_facet_replay_work_count >=
             result.required_root_facet_replay_work_capacity &&
         budget.maximum_root_point_id_replay_work_count >=
             result.required_root_point_id_replay_work_capacity &&
         budget.maximum_target_facet_comparison_count >=
             result.required_target_facet_comparison_capacity &&
         budget.maximum_target_point_id_comparison_count >=
             result.required_target_point_id_comparison_capacity &&
         budget.maximum_snapshot_facet_index_count >=
             result.required_snapshot_facet_index_capacity &&
         budget.maximum_snapshot_point_id_index_count >=
             result.required_snapshot_point_id_index_capacity;
}

[[nodiscard]] std::size_t active_facet_reference_count(
    const ActiveRootMap& active_roots) {
  std::size_t count = 0U;
  for (const auto& [root_id, root] : active_roots) {
    static_cast<void>(root_id);
    checked_accumulate(
        count,
        root.facet_point_ids.size(),
        "the live root facet-reference count overflows");
  }
  return count;
}

void update_live_resource_peaks(
    const ActiveRootMap& active_roots,
    std::size_t pending_root_count,
    std::size_t pending_facet_reference_count,
    std::size_t order,
    RootOverlayResult& result) {
  const std::size_t live_root_count = checked_add(
      active_roots.size(),
      pending_root_count,
      "the live root-state count overflows");
  const std::size_t live_facet_count = checked_add(
      active_facet_reference_count(active_roots),
      pending_facet_reference_count,
      "the live root facet-reference count overflows");
  const std::size_t live_point_id_count = checked_multiply(
      order,
      live_facet_count,
      "the live root PointId-reference count overflows");
  result.counters.peak_live_root_state_count = std::max(
      result.counters.peak_live_root_state_count, live_root_count);
  result.counters.peak_live_root_facet_reference_count = std::max(
      result.counters.peak_live_root_facet_reference_count,
      live_facet_count);
  result.counters.peak_live_root_point_id_reference_count = std::max(
      result.counters.peak_live_root_point_id_reference_count,
      live_point_id_count);
}

[[nodiscard]] std::vector<PointId> points_covered_by_facets(
    const FacetSet& facets) {
  std::set<PointId> points;
  for (const FacetLabel& facet : facets) {
    points.insert(facet.begin(), facet.end());
  }
  return {points.begin(), points.end()};
}

[[nodiscard]] bool result_facts_match(
    const RootOverlayResult& observed,
    const RootOverlayResult& expected) {
  return observed.root_overlay_candidate_space_size_certified ==
             expected.root_overlay_candidate_space_size_certified &&
         observed.root_overlay_preflight_budget_sufficient ==
             expected.root_overlay_preflight_budget_sufficient &&
         observed.source_journal_is_external_and_not_retained ==
             expected.source_journal_is_external_and_not_retained &&
         observed.source_journal_budget_seam_certified ==
             expected.source_journal_budget_seam_certified &&
         observed.source_journal_fresh_replay_certified ==
             expected.source_journal_fresh_replay_certified &&
         observed.root_sweep_started_only_after_complete_source_journal ==
             expected.root_sweep_started_only_after_complete_source_journal &&
         observed.every_history_batch_replayed_exactly_once ==
             expected.every_history_batch_replayed_exactly_once &&
         observed.targets_resolved_against_frozen_pre_batch_snapshots ==
             expected.targets_resolved_against_frozen_pre_batch_snapshots &&
         observed.snapshot_indices_cover_all_active_facets ==
             expected.snapshot_indices_cover_all_active_facets &&
         observed.nontrivial_targets_match_complete_facet_families ==
             expected.nontrivial_targets_match_complete_facet_families &&
         observed.omitted_targets_are_singletons_absent_from_active_facets ==
             expected.omitted_targets_are_singletons_absent_from_active_facets &&
         observed.reduced_component_kinds_checked_after_geometric_binding ==
             expected.reduced_component_kinds_checked_after_geometric_binding &&
         observed.matched_roots_belong_to_target_history_group_prior_roots ==
             expected.matched_roots_belong_to_target_history_group_prior_roots &&
         observed.every_strict_target_bound_exactly_once ==
             expected.every_strict_target_bound_exactly_once &&
         observed.persistent_root_ids_preserved ==
             expected.persistent_root_ids_preserved &&
         observed.mutations_applied_after_complete_batch_resolution ==
             expected.mutations_applied_after_complete_batch_resolution &&
         observed.final_replayed_roots_match_source_history ==
             expected.final_replayed_roots_match_source_history &&
         observed.diagnostic_outcomes_have_no_bindings ==
             expected.diagnostic_outcomes_have_no_bindings &&
         observed.critical_catalog_typed_gamma_root_overlay_certified ==
             expected.critical_catalog_typed_gamma_root_overlay_certified;
}

void require_actual_work_within_derived_bounds(
    const RootOverlayResult& result) {
  if (result.counters.target_root_binding_count >
          result.required_target_root_binding_capacity ||
      result.counters.peak_live_root_state_count >
          result.required_live_root_state_capacity ||
      result.counters.peak_live_root_facet_reference_count >
          result.required_live_root_facet_reference_capacity ||
      result.counters.peak_live_root_point_id_reference_count >
          result.required_live_root_point_id_reference_capacity ||
      result.counters.root_facet_replay_work_count >
          result.required_root_facet_replay_work_capacity ||
      result.counters.root_point_id_replay_work_count >
          result.required_root_point_id_replay_work_capacity ||
      result.counters.target_facet_comparison_count >
          result.required_target_facet_comparison_capacity ||
      result.counters.target_point_id_comparison_count >
          result.required_target_point_id_comparison_capacity ||
      result.counters.snapshot_facet_index_count >
          result.required_snapshot_facet_index_capacity ||
      result.counters.snapshot_point_id_index_count >
          result.required_snapshot_point_id_index_capacity) {
    throw std::logic_error(
        "the exact typed Gamma root-overlay replay exceeded a derived "
        "closed bound");
  }
}

[[nodiscard]] RootOverlayResult
compute_exact_critical_catalog_typed_gamma_root_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    RootOverlayBudget budget,
    const JournalResult& source_journal) {
  validate_domain(cloud, order, budget);
  RootOverlayResult result;
  result.requested_budget = budget;
  result.point_count = cloud.size();
  result.order = order;
  result.scope = RootOverlayScope::
      bounded_n14_k10_single_order_full_pi0_target_families_to_frozen_pre_batch_local_hgp_reduced_roots_with_explicit_isolated_singletons_only;
  result.counters.preflight_count = 1U;
  result.source_journal_is_external_and_not_retained = true;
  result.source_journal_budget_seam_certified =
      source_journal.requested_budget == budget.typed_gamma_journal_budget;
  result.diagnostic_outcomes_have_no_bindings = true;
  derive_root_overlay_preflight(cloud, order, result);
  result.root_overlay_candidate_space_size_certified = true;
  result.root_overlay_preflight_budget_sufficient =
      result.source_journal_budget_seam_certified &&
      budget_covers_preflight(budget, result);
  if (!result.root_overlay_preflight_budget_sufficient) {
    result.decision =
        RootOverlayDecision::no_root_overlay_preflight_budget_insufficient;
    return result;
  }

  const ExactCriticalCatalogTypedGammaJournalVerification
      source_verification =
          verify_exact_critical_catalog_typed_gamma_journal(
              cloud,
              order,
              budget.typed_gamma_journal_budget,
              source_journal);
  ++result.counters.source_journal_verification_count;
  if (!source_verification.
          exact_critical_catalog_typed_gamma_journal_decision_certified) {
    result.decision =
        RootOverlayDecision::no_root_overlay_source_journal_rejected;
    return result;
  }
  result.source_journal_fresh_replay_certified = true;
  result.source_journal_decision = source_journal.decision;
  if (source_journal.decision !=
      JournalDecision::complete_exhaustive_typed_gamma_journal) {
    result.decision =
        RootOverlayDecision::no_root_overlay_source_journal_incomplete;
    return result;
  }
  if (!source_journal.reduced_gamma_history.has_value()) {
    throw std::logic_error(
        "a freshly verified complete typed Gamma journal has no reduced "
        "history");
  }
  result.root_sweep_started_only_after_complete_source_journal = true;

  const ExactPersistentReducedGammaOrderHistory& history =
      *source_journal.reduced_gamma_history;
  std::vector<std::vector<std::size_t>> targets_by_batch(
      history.batch_metadata.size());
  if (source_journal.strict_target_records.size() >
      result.required_target_root_binding_capacity) {
    throw std::logic_error(
        "a fresh typed Gamma journal exceeds the target-binding bound");
  }
  result.target_root_bindings.resize(
      source_journal.strict_target_records.size());
  for (TargetBinding& binding : result.target_root_bindings) {
    binding.target_root_binding_index =
        std::numeric_limits<std::size_t>::max();
    binding.strict_target_record_index =
        std::numeric_limits<std::size_t>::max();
  }
  for (std::size_t target_index = 0U;
       target_index < source_journal.strict_target_records.size();
       ++target_index) {
    const ExactCriticalCatalogTypedGammaStrictTargetRecord& target =
        source_journal.strict_target_records[target_index];
    if (target.strict_target_record_index != target_index ||
        target.history_batch_index >= targets_by_batch.size() ||
        target.history_group_record_index >= history.group_records.size()) {
      throw std::logic_error(
          "a fresh typed Gamma target has an invalid dense history index");
    }
    targets_by_batch[target.history_batch_index].push_back(target_index);
  }

  ActiveRootMap active_roots;
  for (std::size_t batch_index = 0U;
       batch_index < history.batch_metadata.size();
       ++batch_index) {
    const ExactPersistentReducedGammaBatchMetadata& metadata =
        history.batch_metadata[batch_index];
    if (metadata.batch_index != batch_index ||
        active_roots.size() != metadata.active_root_count_before ||
        metadata.first_group_record_index > history.group_records.size() ||
        metadata.group_record_count >
            history.group_records.size() -
                metadata.first_group_record_index ||
        !metadata.pre_batch_root_bijection_certified ||
        !metadata.all_groups_resolved_before_mutation ||
        !metadata.post_batch_root_bijection_certified) {
      throw std::logic_error(
          "a fresh reduced-Gamma history has incoherent batch metadata");
    }
    update_live_resource_peaks(active_roots, 0U, 0U, order, result);

    if (!targets_by_batch[batch_index].empty()) {
      ++result.counters.target_bearing_batch_count;
      std::map<FacetLabel, std::size_t> root_id_by_facet;
      for (const auto& [root_id, root] : active_roots) {
        if (root.root_node_id != root_id) {
          throw std::logic_error(
              "an active reduced-Gamma root has an incoherent ID");
        }
        for (const FacetLabel& facet : root.facet_point_ids) {
          if (!root_id_by_facet.emplace(facet, root_id).second) {
            throw std::logic_error(
                "two pre-batch reduced roots share an active facet");
          }
          ++result.counters.snapshot_facet_index_count;
          checked_accumulate(
              result.counters.snapshot_point_id_index_count,
              order,
              "the snapshot PointId-index work overflows");
        }
      }

      for (const std::size_t target_index :
           targets_by_batch[batch_index]) {
        const ExactCriticalCatalogTypedGammaStrictTargetRecord& target =
            source_journal.strict_target_records[target_index];
        const ExactPersistentReducedGammaHistoryGroupRecord& group =
            history.group_records[target.history_group_record_index];
        if (group.group_record_index !=
                target.history_group_record_index ||
            group.batch_index != batch_index) {
          throw std::logic_error(
              "a typed Gamma target does not join its claimed history "
              "group");
        }
        const FacetSet& target_facets =
            target.strict_component.facet_point_ids;
        if (target_facets.empty() ||
            target.strict_component
                    .canonical_representative_facet_point_ids !=
                target_facets.front()) {
          throw std::logic_error(
              "a fresh typed Gamma target has no canonical facet family");
        }
        checked_accumulate(
            result.counters.target_facet_comparison_count,
            target_facets.size(),
            "the target facet-comparison work overflows");
        checked_accumulate(
            result.counters.target_point_id_comparison_count,
            checked_multiply(
                order,
                target_facets.size(),
                "the target PointId-comparison work overflows"),
            "the target PointId-comparison work overflows");

        TargetBinding binding;
        binding.target_root_binding_index = target_index;
        binding.strict_target_record_index = target_index;
        binding.history_batch_index = batch_index;
        binding.history_group_record_index =
            target.history_group_record_index;
        if (target_facets.size() == 1U) {
          if (root_id_by_facet.contains(target_facets.front())) {
            throw std::logic_error(
                "an isolated strict target is present in a pre-batch "
                "reduced root");
          }
          binding.disposition =
              TargetDisposition::omitted_isolated_singleton;
          if (target.reduced_component_kind !=
              ExactReducedGammaStrictComponentKind::
                  omitted_isolated_facet) {
            throw std::logic_error(
                "an isolated strict target has an incoherent reduced "
                "annotation");
          }
          ++result.counters.omitted_isolated_singleton_target_count;
        } else {
          if (target_facets.size() < order + 1U) {
            throw std::logic_error(
                "a nontrivial strict target violates the coface-incidence "
                "facet lower bound");
          }
          const auto facet_match = root_id_by_facet.find(
              target.strict_component
                  .canonical_representative_facet_point_ids);
          if (facet_match == root_id_by_facet.end()) {
            throw std::logic_error(
                "a nontrivial strict target has no pre-batch reduced root");
          }
          const std::size_t root_id = facet_match->second;
          const auto root = active_roots.find(root_id);
          if (root == active_roots.end() ||
              root->second.facet_point_ids != target_facets) {
            throw std::logic_error(
                "a nontrivial strict target is not a complete root facet "
                "family");
          }
          ++result.counters.group_prior_root_membership_check_count;
          if (!std::binary_search(
                  group.prior_root_node_ids.begin(),
                  group.prior_root_node_ids.end(),
                  root_id)) {
            throw std::logic_error(
                "a matched target root does not belong to its history "
                "group's prior roots");
          }
          if (target.reduced_component_kind !=
              ExactReducedGammaStrictComponentKind::
                  prior_nontrivial_reduced_root) {
            throw std::logic_error(
                "a nontrivial strict target has an incoherent reduced "
                "annotation");
          }
          binding.disposition = TargetDisposition::
              matched_pre_batch_persistent_reduced_root;
          binding.root_node_id = root_id;
          ++result.counters.matched_pre_batch_root_target_count;
        }
        ++result.counters.reduced_component_kind_postcheck_count;
        if (result.target_root_bindings[target_index]
                .target_root_binding_index !=
            std::numeric_limits<std::size_t>::max()) {
          throw std::logic_error(
              "a strict typed Gamma target is bound more than once");
        }
        result.target_root_bindings[target_index] = std::move(binding);
        ++result.counters.target_root_binding_count;
      }
    }

    std::vector<PendingRootMutation> pending;
    pending.reserve(std::min(
        metadata.group_record_count,
        result.maximum_active_root_bound));
    std::set<std::size_t> referenced_snapshot_roots;
    std::set<std::size_t> pending_result_ids;
    std::size_t pending_facet_reference_count = 0U;
    for (std::size_t local_group_index = 0U;
         local_group_index < metadata.group_record_count;
         ++local_group_index) {
      const std::size_t group_index =
          metadata.first_group_record_index + local_group_index;
      const ExactPersistentReducedGammaHistoryGroupRecord& group =
          history.group_records[group_index];
      if (group.group_record_index != group_index ||
          group.batch_index != batch_index ||
          group.batch_group_index != local_group_index ||
          !group.resolved_from_pre_batch_snapshot) {
        throw std::logic_error(
            "a fresh reduced-Gamma history group is not a resolved dense "
            "batch record");
      }
      ++result.counters.history_group_replay_count;
      checked_accumulate(
          result.counters.prior_root_reference_replay_count,
          group.prior_root_node_ids.size(),
          "the prior-root replay count overflows");
      for (const std::size_t prior_root_id :
           group.prior_root_node_ids) {
        if (!active_roots.contains(prior_root_id) ||
            !referenced_snapshot_roots.insert(prior_root_id).second) {
          throw std::logic_error(
              "a history batch does not consume distinct pre-batch roots");
        }
      }
      if (group.kind ==
          ExactReducedGammaBatchGroupKind::deferred_isolated_facet) {
        if (!group.prior_root_node_ids.empty() ||
            group.resulting_root_node_id.has_value() ||
            group.created_node_id.has_value() ||
            group.coverage_delta.has_value()) {
          throw std::logic_error(
              "a deferred history group unexpectedly mutates roots");
        }
        continue;
      }
      if (!group.resulting_root_node_id.has_value() ||
          !group.coverage_delta.has_value()) {
        throw std::logic_error(
            "a non-deferred history group has no root mutation");
      }
      if (pending.size() >= result.maximum_active_root_bound) {
        throw std::logic_error(
            "a history batch exceeds the nontrivial active-root bound");
      }
      const ExactReducedGammaCoverageDelta& delta =
          *group.coverage_delta;
      ++result.counters.coverage_delta_replay_count;
      checked_accumulate(
          result.counters.delta_facet_reference_replay_count,
          delta.added_facet_point_ids.size(),
          "the delta facet-reference replay count overflows");
      checked_accumulate(
          result.counters.delta_point_id_reference_replay_count,
          delta.added_point_ids.size(),
          "the delta PointId-reference replay count overflows");

      FacetSet resulting_facets;
      std::size_t resulting_facet_count =
          delta.added_facet_point_ids.size();
      std::set<PointId> prior_points;
      for (const std::size_t prior_root_id :
           group.prior_root_node_ids) {
        const ActiveRoot& prior_root = active_roots.at(prior_root_id);
        resulting_facet_count = checked_add(
            resulting_facet_count,
            prior_root.facet_point_ids.size(),
            "the resulting root facet count overflows");
        const std::vector<PointId> prior_root_points =
            points_covered_by_facets(prior_root.facet_point_ids);
        prior_points.insert(
            prior_root_points.begin(), prior_root_points.end());
      }
      if (pending_facet_reference_count >
              result.exhaustive_facet_count ||
          resulting_facet_count >
              result.exhaustive_facet_count -
                  pending_facet_reference_count) {
        throw std::logic_error(
            "a history batch exceeds the pending root-facet bound");
      }
      resulting_facets.reserve(resulting_facet_count);
      for (const std::size_t prior_root_id :
           group.prior_root_node_ids) {
        const FacetSet& prior_facets =
            active_roots.at(prior_root_id).facet_point_ids;
        resulting_facets.insert(
            resulting_facets.end(),
            prior_facets.begin(),
            prior_facets.end());
      }
      resulting_facets.insert(
          resulting_facets.end(),
          delta.added_facet_point_ids.begin(),
          delta.added_facet_point_ids.end());
      std::sort(resulting_facets.begin(), resulting_facets.end());
      if (resulting_facets.empty() ||
          resulting_facets.size() != resulting_facet_count ||
          std::adjacent_find(
              resulting_facets.begin(), resulting_facets.end()) !=
              resulting_facets.end() ||
          group.canonical_representative_facet_point_ids !=
              resulting_facets.front()) {
        throw std::logic_error(
            "a replayed reduced root has a non-canonical facet family");
      }
      std::vector<PointId> resulting_points =
          points_covered_by_facets(resulting_facets);
      std::vector<PointId> expected_added_points;
      std::set_difference(
          resulting_points.begin(),
          resulting_points.end(),
          prior_points.begin(),
          prior_points.end(),
          std::back_inserter(expected_added_points));
      if (delta.added_point_ids != expected_added_points ||
          delta.fully_redundant !=
              (delta.added_facet_point_ids.empty() &&
               delta.added_point_ids.empty())) {
        throw std::logic_error(
            "a replayed reduced root has an incoherent coverage delta");
      }

      const std::size_t resulting_root_id =
          *group.resulting_root_node_id;
      switch (group.kind) {
        case ExactReducedGammaBatchGroupKind::birth:
          if (!group.prior_root_node_ids.empty() ||
              group.created_node_id != group.resulting_root_node_id) {
            throw std::logic_error(
                "a reduced-root birth does not preserve its created ID");
          }
          break;
        case ExactReducedGammaBatchGroupKind::continuation:
          if (group.prior_root_node_ids.size() != 1U ||
              group.prior_root_node_ids.front() != resulting_root_id ||
              group.created_node_id.has_value()) {
            throw std::logic_error(
                "a reduced-root continuation changed its persistent ID");
          }
          break;
        case ExactReducedGammaBatchGroupKind::multifusion:
          if (group.prior_root_node_ids.size() < 2U ||
              group.created_node_id != group.resulting_root_node_id) {
            throw std::logic_error(
                "a reduced-root multifusion has incoherent persistent IDs");
          }
          break;
        case ExactReducedGammaBatchGroupKind::deferred_isolated_facet:
          throw std::logic_error(
              "a deferred history group reached root preparation");
      }

      PendingRootMutation mutation;
      mutation.prior_root_ids = group.prior_root_node_ids;
      mutation.resulting_root_id = resulting_root_id;
      mutation.resulting_root.root_node_id = resulting_root_id;
      mutation.resulting_root.facet_point_ids =
          std::move(resulting_facets);
      if (!pending_result_ids.insert(resulting_root_id).second) {
        throw std::logic_error(
            "two groups in one batch produce the same reduced-root ID");
      }
      checked_accumulate(
          result.counters.root_facet_replay_work_count,
          resulting_facet_count,
          "the root facet-replay work overflows");
      checked_accumulate(
          result.counters.root_point_id_replay_work_count,
          checked_multiply(
              order,
              resulting_facet_count,
              "the root PointId-replay work overflows"),
          "the root PointId-replay work overflows");
      checked_accumulate(
          pending_facet_reference_count,
          resulting_facet_count,
          "the pending root facet-reference count overflows");
      pending.push_back(std::move(mutation));
      update_live_resource_peaks(
          active_roots,
          pending.size(),
          pending_facet_reference_count,
          order,
          result);
    }

    ++result.counters.batch_atomic_commit_count;
    checked_accumulate(
        result.counters.root_mutation_count,
        pending.size(),
        "the root mutation count overflows");
    for (const PendingRootMutation& mutation : pending) {
      for (const std::size_t prior_root_id : mutation.prior_root_ids) {
        if (active_roots.erase(prior_root_id) != 1U) {
          throw std::logic_error(
              "an atomic root commit cannot consume a prior root");
        }
      }
    }
    for (PendingRootMutation& mutation : pending) {
      if (!active_roots
               .emplace(
                   mutation.resulting_root_id,
                   std::move(mutation.resulting_root))
               .second) {
        throw std::logic_error(
            "an atomic root commit collides with a surviving root ID");
      }
    }
    if (active_roots.size() != metadata.active_root_count_after) {
      throw std::logic_error(
          "an atomic root commit violates the post-batch root count");
    }
    ++result.counters.history_batch_replay_count;
  }

  if (active_roots.size() != history.final_active_roots.size()) {
    throw std::logic_error(
        "the final replayed roots differ from the fresh source history");
  }
  auto active_root = active_roots.begin();
  for (const ExactPersistentReducedGammaActiveRoot& expected_root :
       history.final_active_roots) {
    if (active_root == active_roots.end() ||
        active_root->first != expected_root.root_node_id ||
        active_root->second.root_node_id != expected_root.root_node_id ||
        active_root->second.facet_point_ids !=
            expected_root.facet_point_ids ||
        points_covered_by_facets(
            active_root->second.facet_point_ids) !=
            expected_root.covered_point_ids) {
      throw std::logic_error(
          "the final replayed roots differ from the fresh source history");
    }
    ++active_root;
  }
  result.counters.final_active_root_count = active_roots.size();
  if (result.counters.target_root_binding_count !=
      result.target_root_bindings.size()) {
    throw std::logic_error(
        "a strict typed Gamma target was not bound exactly once");
  }
  for (std::size_t target_index = 0U;
       target_index < result.target_root_bindings.size();
       ++target_index) {
    const TargetBinding& binding =
        result.target_root_bindings[target_index];
    if (binding.target_root_binding_index != target_index ||
        binding.strict_target_record_index != target_index) {
      throw std::logic_error(
          "a strict typed Gamma target binding is not dense and exact");
    }
  }
  require_actual_work_within_derived_bounds(result);

  result.every_history_batch_replayed_exactly_once =
      result.counters.history_batch_replay_count ==
          history.batch_metadata.size();
  result.targets_resolved_against_frozen_pre_batch_snapshots = true;
  result.snapshot_indices_cover_all_active_facets = true;
  result.nontrivial_targets_match_complete_facet_families = true;
  result.omitted_targets_are_singletons_absent_from_active_facets = true;
  result.reduced_component_kinds_checked_after_geometric_binding =
      result.counters.reduced_component_kind_postcheck_count ==
          source_journal.strict_target_records.size();
  result.matched_roots_belong_to_target_history_group_prior_roots = true;
  result.every_strict_target_bound_exactly_once =
      result.counters.target_root_binding_count ==
          source_journal.strict_target_records.size();
  result.persistent_root_ids_preserved = true;
  result.mutations_applied_after_complete_batch_resolution = true;
  result.final_replayed_roots_match_source_history = true;
  result.critical_catalog_typed_gamma_root_overlay_certified =
      result.root_overlay_candidate_space_size_certified &&
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
      result.diagnostic_outcomes_have_no_bindings;
  if (!result.critical_catalog_typed_gamma_root_overlay_certified) {
    throw std::logic_error(
        "the exhaustive typed Gamma root overlay failed certification");
  }
  result.decision =
      RootOverlayDecision::complete_exhaustive_pre_batch_root_overlay;
  return result;
}

}  // namespace

ExactCriticalCatalogTypedGammaRootOverlayVerification
verify_exact_critical_catalog_typed_gamma_root_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    RootOverlayBudget budget,
    const JournalResult& source_journal,
    const RootOverlayResult& result) {
  const RootOverlayResult expected =
      compute_exact_critical_catalog_typed_gamma_root_overlay(
          cloud, order, budget, source_journal);
  ExactCriticalCatalogTypedGammaRootOverlayVerification verification;
  verification.requested_budget_certified =
      result.requested_budget == budget &&
      result.requested_budget == expected.requested_budget;
  verification.external_inputs_certified =
      result.point_count == cloud.size() &&
      result.point_count == expected.point_count && result.order == order &&
      result.order == expected.order;
  verification.derived_preflight_sizes_certified =
      result.exhaustive_facet_count == expected.exhaustive_facet_count &&
      result.exhaustive_coface_count == expected.exhaustive_coface_count &&
      result.critical_event_support_bound ==
          expected.critical_event_support_bound &&
      result.critical_arm_bound == expected.critical_arm_bound &&
      result.maximum_active_root_bound ==
          expected.maximum_active_root_bound &&
      result.required_target_root_binding_capacity ==
          expected.required_target_root_binding_capacity &&
      result.required_live_root_state_capacity ==
          expected.required_live_root_state_capacity &&
      result.required_live_root_facet_reference_capacity ==
          expected.required_live_root_facet_reference_capacity &&
      result.required_live_root_point_id_reference_capacity ==
          expected.required_live_root_point_id_reference_capacity &&
      result.required_root_facet_replay_work_capacity ==
          expected.required_root_facet_replay_work_capacity &&
      result.required_root_point_id_replay_work_capacity ==
          expected.required_root_point_id_replay_work_capacity &&
      result.required_target_facet_comparison_capacity ==
          expected.required_target_facet_comparison_capacity &&
      result.required_target_point_id_comparison_capacity ==
          expected.required_target_point_id_comparison_capacity &&
      result.required_snapshot_facet_index_capacity ==
          expected.required_snapshot_facet_index_capacity &&
      result.required_snapshot_point_id_index_capacity ==
          expected.required_snapshot_point_id_index_capacity &&
      result.root_overlay_candidate_space_size_certified ==
          expected.root_overlay_candidate_space_size_certified;
  verification.source_journal_decision_certified =
      result.source_journal_decision == expected.source_journal_decision;
  verification.source_journal_fresh_replay_certified =
      result.source_journal_fresh_replay_certified ==
          expected.source_journal_fresh_replay_certified;
  verification.target_root_bindings_certified =
      result.target_root_bindings == expected.target_root_bindings;
  verification.result_facts_certified =
      result_facts_match(result, expected);
  verification.counters_certified =
      result.counters == expected.counters;
  verification.decision_certified =
      result.decision == expected.decision;
  verification.scope_certified =
      result.scope == RootOverlayScope::
          bounded_n14_k10_single_order_full_pi0_target_families_to_frozen_pre_batch_local_hgp_reduced_roots_with_explicit_isolated_singletons_only &&
      result.scope == expected.scope;
  verification.fresh_replay_certified =
      verification.requested_budget_certified &&
      verification.external_inputs_certified &&
      verification.derived_preflight_sizes_certified &&
      verification.source_journal_decision_certified &&
      verification.source_journal_fresh_replay_certified &&
      verification.target_root_bindings_certified &&
      verification.result_facts_certified &&
      verification.counters_certified &&
      verification.decision_certified && verification.scope_certified;
  verification.
      exact_critical_catalog_typed_gamma_root_overlay_decision_certified =
      verification.fresh_replay_certified;
  return verification;
}

ExactCriticalCatalogTypedGammaRootOverlayResult
build_exact_critical_catalog_typed_gamma_root_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    RootOverlayBudget budget,
    const JournalResult& source_journal) {
  RootOverlayResult result =
      compute_exact_critical_catalog_typed_gamma_root_overlay(
          cloud, order, budget, source_journal);
  const ExactCriticalCatalogTypedGammaRootOverlayVerification
      verification =
          verify_exact_critical_catalog_typed_gamma_root_overlay(
              cloud, order, budget, source_journal, result);
  if (!verification.
          exact_critical_catalog_typed_gamma_root_overlay_decision_certified) {
    throw std::logic_error(
        "the exact typed Gamma root overlay failed its fresh replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
