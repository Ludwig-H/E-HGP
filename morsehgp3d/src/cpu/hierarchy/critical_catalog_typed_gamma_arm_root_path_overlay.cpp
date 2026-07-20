#include "morsehgp3d/hierarchy/critical_catalog_typed_gamma_arm_root_path_overlay.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using PathBudget =
    ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget;
using PathCounters =
    ExactCriticalCatalogTypedGammaArmRootPathOverlayCounters;
using PathDecision =
    ExactCriticalCatalogTypedGammaArmRootPathOverlayDecision;
using PathRecord =
    ExactCriticalCatalogTypedGammaEventLocalArmRootPathRecord;
using PathResult =
    ExactCriticalCatalogTypedGammaArmRootPathOverlayResult;
using PathScope =
    ExactCriticalCatalogTypedGammaArmRootPathOverlayScope;
using CompositionBudget =
    ExactCriticalCatalogTypedGammaArmRootCompositionBudget;
using CompositionDecision =
    ExactCriticalCatalogTypedGammaArmRootCompositionDecision;
using CompositionResult =
    ExactCriticalCatalogTypedGammaArmRootCompositionResult;
using JournalResult = ExactCriticalCatalogTypedGammaJournalResult;
using RootOverlayResult =
    ExactCriticalCatalogTypedGammaRootOverlayResult;
using TargetDisposition =
    ExactCriticalCatalogTypedGammaTargetDisposition;

static_assert(
    PathBudget::maximum_supported_path_record_count ==
    CompositionBudget::maximum_supported_arm_candidate_count);
static_assert(
    PathBudget::maximum_supported_committed_chain_segment_count ==
    PathBudget::maximum_supported_path_record_count *
        ExactFacetDescentChainBudget::
            maximum_supported_committed_strict_segment_count);
static_assert(
    PathBudget::maximum_supported_composite_path_segment_count ==
    PathBudget::maximum_supported_path_record_count *
        (ExactFacetDescentChainBudget::
             maximum_supported_committed_strict_segment_count +
         1U));
static_assert(
    PathBudget::maximum_supported_chain_node_point_id_reference_count ==
    PathResult::maximum_supported_order *
        PathBudget::maximum_supported_composite_path_segment_count);
static_assert(
    PathBudget::maximum_supported_exterior_constraint_count ==
    PathBudget::maximum_supported_path_record_count *
        (PathResult::maximum_supported_point_count -
         PathResult::minimum_supported_order - 1U));

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
        "the typed Gamma arm-root path binomial overflows");
    value /= factor;
  }
  return value;
}

void validate_domain(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const PathBudget& budget) {
  if (cloud.size() < PathResult::minimum_supported_point_count ||
      cloud.size() > PathResult::maximum_supported_point_count ||
      order < PathResult::minimum_supported_order ||
      order > PathResult::maximum_supported_order ||
      order >= cloud.size()) {
    throw std::invalid_argument(
        "the typed Gamma arm-root path overlay requires 3<=n<=14, "
        "2<=k<n and k<=10");
  }
  validate_exact_critical_catalog_typed_gamma_arm_root_composition_budget_caps(
      budget.arm_root_composition_budget);
  if (budget.maximum_path_record_count >
          PathBudget::maximum_supported_path_record_count ||
      budget.maximum_committed_chain_segment_count >
          PathBudget::
              maximum_supported_committed_chain_segment_count ||
      budget.maximum_composite_path_segment_count >
          PathBudget::
              maximum_supported_composite_path_segment_count ||
      budget.maximum_chain_node_point_id_reference_count >
          PathBudget::
              maximum_supported_chain_node_point_id_reference_count ||
      budget.maximum_exterior_constraint_count >
          PathBudget::maximum_supported_exterior_constraint_count) {
    throw std::invalid_argument(
        "a typed Gamma arm-root path capacity exceeds its bounded cap");
  }
}

void derive_preflight(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    PathResult& result) {
  const std::size_t maximum_support_size =
      std::min({std::size_t{4U}, order + 1U, cloud.size()});
  for (std::size_t support_size = 2U;
       support_size <= maximum_support_size;
       ++support_size) {
    result.critical_event_support_bound = checked_add(
        result.critical_event_support_bound,
        bounded_binomial(cloud.size(), support_size),
        "the typed Gamma arm-root path event bound overflows");
  }
  result.critical_arm_bound = checked_multiply(
      4U,
      result.critical_event_support_bound,
      "the typed Gamma arm-root path arm bound overflows");
  const std::size_t per_arm_chain_capacity =
      result.requested_budget.arm_root_composition_budget
          .root_overlay_budget.typed_gamma_journal_budget
          .arm_overlay_budget.per_arm_chain_budget
          .maximum_committed_strict_segment_count;
  result.required_path_record_capacity = result.critical_arm_bound;
  result.required_committed_chain_segment_capacity = checked_multiply(
      result.critical_arm_bound,
      per_arm_chain_capacity,
      "the typed Gamma arm-root path chain-segment bound overflows");
  result.required_composite_path_segment_capacity = checked_multiply(
      result.critical_arm_bound,
      checked_add(
          per_arm_chain_capacity,
          1U,
          "the typed Gamma arm-root path per-arm composite bound overflows"),
      "the typed Gamma arm-root composite-segment bound overflows");
  result.required_chain_node_point_id_reference_capacity = checked_multiply(
      order,
      result.required_composite_path_segment_capacity,
      "the typed Gamma arm-root path point-reference bound overflows");
  result.required_exterior_constraint_capacity = checked_multiply(
      result.critical_arm_bound,
      cloud.size() - order - 1U,
      "the typed Gamma arm-root exterior-constraint bound overflows");
  if (result.required_path_record_capacity >
          PathBudget::maximum_supported_path_record_count ||
      result.required_committed_chain_segment_capacity >
          PathBudget::
              maximum_supported_committed_chain_segment_count ||
      result.required_composite_path_segment_capacity >
          PathBudget::
              maximum_supported_composite_path_segment_count ||
      result.required_chain_node_point_id_reference_capacity >
          PathBudget::
              maximum_supported_chain_node_point_id_reference_count ||
      result.required_exterior_constraint_capacity >
          PathBudget::maximum_supported_exterior_constraint_count) {
    throw std::logic_error(
        "the derived arm-root path preflight exceeds its bounded caps");
  }
}

[[nodiscard]] bool budget_covers_preflight(
    const PathBudget& budget,
    const PathResult& result) {
  return budget.maximum_path_record_count >=
             result.required_path_record_capacity &&
         budget.maximum_committed_chain_segment_count >=
             result.required_committed_chain_segment_capacity &&
         budget.maximum_composite_path_segment_count >=
             result.required_composite_path_segment_capacity &&
         budget.maximum_chain_node_point_id_reference_count >=
             result.required_chain_node_point_id_reference_capacity &&
         budget.maximum_exterior_constraint_count >=
             result.required_exterior_constraint_capacity;
}

[[nodiscard]] bool component_contains_facet(
    const ExactStrictGammaComponentWitness& component,
    const std::vector<spatial::PointId>& facet_point_ids) {
  return std::find(
             component.facet_point_ids.begin(),
             component.facet_point_ids.end(),
             facet_point_ids) != component.facet_point_ids.end();
}

[[nodiscard]] std::vector<spatial::PointId> initial_arm_facet(
    const ExactCriticalEvent& event,
    spatial::PointId removed_shell_point_id) {
  std::vector<spatial::PointId> facet = event.closed_point_ids;
  const auto removed = std::lower_bound(
      facet.begin(), facet.end(), removed_shell_point_id);
  if (removed == facet.end() || *removed != removed_shell_point_id) {
    throw std::logic_error(
        "a fresh critical event does not contain the removed shell point");
  }
  facet.erase(removed);
  return facet;
}

[[nodiscard]] bool result_facts_match(
    const PathResult& observed,
    const PathResult& expected) {
  return observed.arm_root_path_candidate_space_size_certified ==
             expected.arm_root_path_candidate_space_size_certified &&
         observed.arm_root_path_preflight_budget_sufficient ==
             expected.arm_root_path_preflight_budget_sufficient &&
         observed.source_journal_is_external_and_not_retained ==
             expected.source_journal_is_external_and_not_retained &&
         observed.source_root_overlay_is_external_and_not_retained ==
             expected.source_root_overlay_is_external_and_not_retained &&
         observed.source_composition_is_external_and_not_retained ==
             expected.source_composition_is_external_and_not_retained &&
         observed.source_journal_budget_seam_certified ==
             expected.source_journal_budget_seam_certified &&
         observed.source_root_overlay_budget_seam_certified ==
             expected.source_root_overlay_budget_seam_certified &&
         observed.source_composition_budget_seam_certified ==
             expected.source_composition_budget_seam_certified &&
         observed.source_composition_fresh_replay_certified ==
             expected.source_composition_fresh_replay_certified &&
         observed.
                 reconstruction_started_only_after_complete_source_composition ==
             expected.
                 reconstruction_started_only_after_complete_source_composition &&
         observed.transient_critical_catalog_fresh_replay_certified ==
             expected.transient_critical_catalog_fresh_replay_certified &&
         observed.transient_critical_arm_families_fresh_replay_certified ==
             expected.transient_critical_arm_families_fresh_replay_certified &&
         observed.every_arm_candidate_has_one_dense_replayable_path ==
             expected.every_arm_candidate_has_one_dense_replayable_path &&
         observed.event_saddle_arm_and_terminal_class_keys_reconciled ==
             expected.event_saddle_arm_and_terminal_class_keys_reconciled &&
         observed.exact_initial_germs_and_chain_shapes_replayable ==
             expected.exact_initial_germs_and_chain_shapes_replayable &&
         observed.exact_seams_strict_paths_and_regular_terminals_certified ==
             expected.
                 exact_seams_strict_paths_and_regular_terminals_certified &&
         observed.
                 initial_and_terminal_facets_belong_to_external_full_pi0_targets ==
             expected.
                 initial_and_terminal_facets_belong_to_external_full_pi0_targets &&
         observed.
                 target_bindings_and_reduced_dispositions_copied_without_reclassification ==
             expected.
                 target_bindings_and_reduced_dispositions_copied_without_reclassification &&
         observed.shared_targets_and_roots_preserve_distinct_paths ==
             expected.shared_targets_and_roots_preserve_distinct_paths &&
         observed.
                 records_are_event_local_internal_paths_and_not_public_attachments ==
             expected.
                 records_are_event_local_internal_paths_and_not_public_attachments &&
         observed.diagnostic_outcomes_have_no_paths ==
             expected.diagnostic_outcomes_have_no_paths &&
         observed.
                 critical_catalog_typed_gamma_arm_root_path_overlay_certified ==
             expected.
                 critical_catalog_typed_gamma_arm_root_path_overlay_certified;
}

[[nodiscard]] PathResult compute_path_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    PathBudget budget,
    const JournalResult& source_journal,
    const RootOverlayResult& source_root_overlay,
    const CompositionResult& source_composition) {
  validate_domain(cloud, order, budget);
  PathResult result;
  result.requested_budget = budget;
  result.point_count = cloud.size();
  result.order = order;
  result.scope = PathScope::
      bounded_n14_k10_single_order_event_local_typed_critical_arms_with_replayable_strict_descent_paths_linked_to_external_full_pi0_targets_and_separate_frozen_pre_batch_local_hgp_reduced_root_or_omitted_singleton_dispositions_only;
  result.counters.preflight_count = 1U;
  result.source_journal_is_external_and_not_retained = true;
  result.source_root_overlay_is_external_and_not_retained = true;
  result.source_composition_is_external_and_not_retained = true;
  const CompositionBudget& composition_budget =
      budget.arm_root_composition_budget;
  result.source_journal_budget_seam_certified =
      source_journal.requested_budget ==
      composition_budget.root_overlay_budget.typed_gamma_journal_budget;
  result.source_root_overlay_budget_seam_certified =
      source_root_overlay.requested_budget ==
      composition_budget.root_overlay_budget;
  result.source_composition_budget_seam_certified =
      source_composition.requested_budget == composition_budget;
  result.diagnostic_outcomes_have_no_paths = true;
  derive_preflight(cloud, order, result);
  result.arm_root_path_candidate_space_size_certified = true;
  result.arm_root_path_preflight_budget_sufficient =
      result.source_journal_budget_seam_certified &&
      result.source_root_overlay_budget_seam_certified &&
      result.source_composition_budget_seam_certified &&
      budget_covers_preflight(budget, result);
  if (!result.arm_root_path_preflight_budget_sufficient) {
    result.decision = PathDecision::
        no_arm_root_path_overlay_preflight_budget_insufficient;
    return result;
  }

  const auto source_verification =
      verify_exact_critical_catalog_typed_gamma_arm_root_composition(
          cloud,
          order,
          composition_budget,
          source_journal,
          source_root_overlay,
          source_composition);
  ++result.counters.source_composition_verification_count;
  if (!source_verification.
          exact_critical_catalog_typed_gamma_arm_root_composition_decision_certified) {
    result.decision = PathDecision::
        no_arm_root_path_overlay_source_composition_rejected;
    return result;
  }
  result.source_composition_fresh_replay_certified = true;
  result.source_composition_decision = source_composition.decision;
  if (source_composition.decision != CompositionDecision::
          complete_exhaustive_event_local_arm_root_composition) {
    result.decision = PathDecision::
        no_arm_root_path_overlay_source_composition_incomplete;
    return result;
  }
  if (source_journal.decision !=
          ExactCriticalCatalogTypedGammaJournalDecision::
              complete_exhaustive_typed_gamma_journal ||
      source_root_overlay.decision !=
          ExactCriticalCatalogTypedGammaRootOverlayDecision::
              complete_exhaustive_pre_batch_root_overlay) {
    throw std::logic_error(
        "a complete arm-root composition has incomplete source layers");
  }
  result.reconstruction_started_only_after_complete_source_composition = true;

  const ExactCriticalCatalogArmGammaOverlayBudget& arm_overlay_budget =
      composition_budget.root_overlay_budget.typed_gamma_journal_budget
          .arm_overlay_budget;
  ExactCriticalCatalogResult catalog = build_exact_critical_catalog(
      cloud, order, arm_overlay_budget.critical_catalog_budget);
  ++result.counters.critical_catalog_build_count;
  if (catalog.decision !=
          ExactCriticalCatalogDecision::complete_supported_critical_catalog ||
      !catalog.no_relevant_extra_shell_degeneracy) {
    throw std::logic_error(
        "a complete arm-root composition has no complete fresh catalog");
  }
  result.transient_critical_catalog_fresh_replay_certified = true;

  if (source_composition.arm_candidates.size() >
          result.required_path_record_capacity ||
      source_composition.arm_candidates.size() !=
          source_journal.arm_records.size()) {
    throw std::logic_error(
        "a complete composition exceeds the path preflight or is not total");
  }
  std::vector<std::optional<PathRecord>> pending_by_candidate(
      source_composition.arm_candidates.size());
  PathCounters pending_counters = result.counters;

  for (std::size_t saddle_index = 0U;
       saddle_index < source_journal.saddle_records.size();
       ++saddle_index) {
    const ExactCriticalCatalogTypedGammaSaddleRecord& saddle =
        source_journal.saddle_records[saddle_index];
    if (saddle.saddle_record_index != saddle_index ||
        saddle.catalog_event_index >= catalog.events.size()) {
      throw std::logic_error(
          "a typed saddle has no dense fresh catalog event");
    }
    const ExactCriticalEvent& event =
        catalog.events[saddle.catalog_event_index];
    ++pending_counters.saddle_event_reconciliation_count;
    if (event.event_index != saddle.catalog_event_index ||
        !event.saddle_order.has_value() ||
        *event.saddle_order != order || event.closed_rank != order + 1U ||
        event.closed_point_ids.size() != order + 1U) {
      throw std::logic_error(
          "a typed saddle disagrees with its fresh critical event");
    }

    ExactCriticalArmFamilyResult family =
        build_exact_critical_arm_family_descent(
            cloud,
            event.shell_point_ids,
            arm_overlay_budget.per_arm_chain_budget);
    ++pending_counters.critical_arm_family_build_count;
    if (family.decision != ExactCriticalArmFamilyDecision::
            all_arms_complete_at_regular_active_facets ||
        family.arms.size() != saddle.arm_record_indices.size() ||
        family.critical_shell_point_ids != event.shell_point_ids ||
        !family.complete_terminal_label_partition_certified) {
      throw std::logic_error(
          "a complete composition has no complete fresh arm family");
    }

    for (const std::size_t arm_record_index : saddle.arm_record_indices) {
      if (arm_record_index >= source_journal.arm_records.size() ||
          arm_record_index >= source_composition.arm_candidates.size() ||
          pending_by_candidate[arm_record_index].has_value()) {
        throw std::logic_error(
            "a typed saddle has an invalid or duplicate arm reference");
      }
      const ExactCriticalCatalogTypedGammaArmRecord& journal_arm =
          source_journal.arm_records[arm_record_index];
      const ExactCriticalCatalogTypedGammaEventLocalArmTargetRootCandidate&
          candidate = source_composition.arm_candidates[arm_record_index];
      if (journal_arm.arm_record_index != arm_record_index ||
          journal_arm.saddle_record_index != saddle_index ||
          journal_arm.terminal_class_record_index >=
              source_journal.terminal_class_records.size() ||
          journal_arm.strict_target_record_index >=
              source_journal.strict_target_records.size() ||
          candidate.candidate_index != arm_record_index ||
          candidate.arm_record_index != arm_record_index ||
          candidate.catalog_event_index != saddle.catalog_event_index ||
          candidate.removed_shell_point_id !=
              journal_arm.removed_shell_point_id ||
          candidate.strict_target_record_index !=
              journal_arm.strict_target_record_index ||
          candidate.target_root_binding_index >=
              source_root_overlay.target_root_bindings.size()) {
        throw std::logic_error(
            "an arm candidate breaks its dense event-local key");
      }

      auto family_arm = std::find_if(
          family.arms.begin(),
          family.arms.end(),
          [&](const ExactCriticalArmFamilyArmResult& arm) {
            return arm.removed_shell_point_id ==
                   journal_arm.removed_shell_point_id;
          });
      if (family_arm == family.arms.end() ||
          std::count_if(
              family.arms.begin(),
              family.arms.end(),
              [&](const ExactCriticalArmFamilyArmResult& arm) {
                return arm.removed_shell_point_id ==
                       journal_arm.removed_shell_point_id;
              }) != 1) {
        throw std::logic_error(
            "a fresh arm family does not identify one removed point");
      }
      const ExactCriticalCatalogTypedGammaTerminalClassRecord&
          terminal_class = source_journal.terminal_class_records[
              journal_arm.terminal_class_record_index];
      if (!family_arm->terminal_label_class_index.has_value() ||
          *family_arm->terminal_label_class_index !=
              terminal_class.source_terminal_label_class_index ||
          terminal_class.saddle_record_index != saddle_index ||
          terminal_class.terminal_class_record_index !=
              journal_arm.terminal_class_record_index ||
          terminal_class.source_terminal_label_class_index >=
              family.terminal_label_classes.size() ||
          terminal_class.terminal_class !=
              family.terminal_label_classes[
                  terminal_class.source_terminal_label_class_index] ||
          terminal_class.strict_target_record_index !=
              journal_arm.strict_target_record_index) {
        throw std::logic_error(
            "a fresh arm terminal class disagrees with the typed journal");
      }

      const ExactCriticalCatalogTypedGammaStrictTargetRecord& target =
          source_journal.strict_target_records[
              journal_arm.strict_target_record_index];
      const ExactCriticalCatalogTypedGammaTargetRootBinding& binding =
          source_root_overlay.target_root_bindings[
              candidate.target_root_binding_index];
      if (target.strict_target_record_index !=
              journal_arm.strict_target_record_index ||
          target.history_batch_index != saddle.history_batch_index ||
          target.history_group_record_index !=
              saddle.history_group_record_index ||
          binding.target_root_binding_index !=
              candidate.target_root_binding_index ||
          binding.strict_target_record_index !=
              journal_arm.strict_target_record_index ||
          binding.history_batch_index != saddle.history_batch_index ||
          binding.history_group_record_index !=
              saddle.history_group_record_index ||
          candidate.disposition != binding.disposition ||
          candidate.local_reduced_root_node_id != binding.root_node_id) {
        throw std::logic_error(
            "an arm path target or local reduced binding is inconsistent");
      }

      ExactCriticalArmDescentResult& descent = family_arm->descent;
      ExactCriticalArmInitialSegmentResult& initial =
          descent.initial_segment;
      const std::vector<spatial::PointId> expected_initial_facet =
          initial_arm_facet(event, journal_arm.removed_shell_point_id);
      if (descent.decision != ExactCriticalArmDescentDecision::
              complete_at_regular_active_facet ||
          initial.source_decision !=
              ExactCriticalArmSourceDecision::critical_source_certified ||
          initial.decision != ExactCriticalArmInitialSegmentDecision::
              strict_initial_arm_segment_certified ||
          initial.critical_shell_point_ids != event.shell_point_ids ||
          initial.closed_rank != order + 1U || initial.order != order ||
          initial.critical_shell_miniball.center != event.center ||
          initial.critical_shell_miniball.squared_radius !=
              event.squared_level ||
          initial.arm_facet_point_ids != expected_initial_facet ||
          !initial.initial_segment_coefficients.has_value() ||
          !initial.removed_point_target_squared_distance.has_value() ||
          !initial.removed_point_outgoing_linear_coefficient.has_value() ||
          !initial.strict_local_parameter_upper_bound.has_value() ||
          !initial.half_open_initial_segment_strict_critical_sublevel ||
          !descent.exact_initial_to_chain_seam_certified ||
          !descent.source_open_composite_path_strict_critical_sublevel ||
          !descent.facet_descent_chain.has_value() ||
          !family_arm->active_terminal.has_value()) {
        throw std::logic_error(
            "a fresh arm does not expose one complete replayable path");
      }
      ExactFacetDescentChainResult& chain =
          *descent.facet_descent_chain;
      if (chain.decision != ExactFacetDescentChainDecision::
              complete_at_regular_active_facet ||
          chain.nodes.empty() ||
          chain.nodes.size() !=
              chain.committed_segment_witnesses.size() + 1U ||
          chain.nodes.front().facet_point_ids != expected_initial_facet ||
          chain.nodes.back() != *family_arm->active_terminal ||
          !chain.exact_seams_certified ||
          !chain.strict_facet_potential_certified ||
          !chain.finite_strict_facet_orbit_theorem_certified ||
          !chain.source_open_polyline_strict_initial_sublevel ||
          *family_arm->active_terminal !=
              terminal_class.terminal_class.canonical_terminal) {
        throw std::logic_error(
            "a fresh arm chain breaks its strict path or terminal seam");
      }

      ++pending_counters.initial_facet_target_membership_check_count;
      ++pending_counters.terminal_facet_target_membership_check_count;
      if (!component_contains_facet(
              target.strict_component, expected_initial_facet) ||
          !component_contains_facet(
              target.strict_component,
              family_arm->active_terminal->facet_point_ids)) {
        throw std::logic_error(
            "a replayable arm path leaves its authoritative full-pi0 target");
      }

      if (std::any_of(
              pending_by_candidate.begin(),
              pending_by_candidate.end(),
              [&](const std::optional<PathRecord>& previous) {
                return previous.has_value() &&
                       previous->strict_target_record_index ==
                           journal_arm.strict_target_record_index;
              })) {
        ++pending_counters.shared_target_path_count;
      }
      switch (binding.disposition) {
        case TargetDisposition::omitted_isolated_singleton:
          if (binding.root_node_id.has_value()) {
            throw std::logic_error(
                "an omitted path target unexpectedly carries a local root");
          }
          ++pending_counters.omitted_isolated_singleton_path_count;
          break;
        case TargetDisposition::matched_pre_batch_persistent_reduced_root:
          if (!binding.root_node_id.has_value()) {
            throw std::logic_error(
                "a matched path target has no local reduced root");
          }
          ++pending_counters.matched_local_reduced_root_path_count;
          break;
      }

      PathRecord record;
      record.path_record_index = arm_record_index;
      record.arm_candidate_index = candidate.candidate_index;
      record.arm_record_index = arm_record_index;
      record.source_arm_target_index = journal_arm.source_arm_target_index;
      record.catalog_event_index = saddle.catalog_event_index;
      record.saddle_record_index = saddle_index;
      record.terminal_class_record_index =
          journal_arm.terminal_class_record_index;
      record.order = order;
      record.removed_shell_point_id = journal_arm.removed_shell_point_id;
      record.critical_center = event.center;
      record.critical_squared_level = event.squared_level;
      record.history_batch_index = saddle.history_batch_index;
      record.history_group_record_index =
          saddle.history_group_record_index;
      record.strict_target_record_index =
          journal_arm.strict_target_record_index;
      record.target_root_binding_index =
          candidate.target_root_binding_index;
      record.disposition = candidate.disposition;
      record.local_reduced_root_node_id =
          candidate.local_reduced_root_node_id;
      record.initial_segment_witness =
          *initial.initial_segment_coefficients;
      record.removed_point_target_squared_distance =
          *initial.removed_point_target_squared_distance;
      record.removed_point_outgoing_linear_coefficient =
          *initial.removed_point_outgoing_linear_coefficient;
      record.strict_local_parameter_upper_bound =
          *initial.strict_local_parameter_upper_bound;
      record.negative_exterior_direction_constraints =
          std::move(initial.negative_exterior_direction_constraints);
      record.chain_nodes = std::move(chain.nodes);
      record.committed_chain_segment_witnesses =
          std::move(chain.committed_segment_witnesses);
      record.exact_initial_to_chain_seam_certified =
          descent.exact_initial_to_chain_seam_certified;
      record.source_open_composite_path_strict_critical_sublevel =
          descent.source_open_composite_path_strict_critical_sublevel;
      record.regular_active_terminal_certified = true;

      ++pending_counters.arm_candidate_path_reconciliation_count;
      ++pending_counters.path_record_count;
      pending_counters.committed_chain_segment_count = checked_add(
          pending_counters.committed_chain_segment_count,
          record.committed_chain_segment_witnesses.size(),
          "the retained arm-root chain-segment count overflows");
      pending_counters.composite_path_segment_count = checked_add(
          pending_counters.composite_path_segment_count,
          record.chain_nodes.size(),
          "the retained arm-root composite-segment count overflows");
      pending_counters.exterior_constraint_count = checked_add(
          pending_counters.exterior_constraint_count,
          record.negative_exterior_direction_constraints.size(),
          "the retained arm-root exterior-constraint count overflows");
      for (const ExactFacetDescentChainNodeWitness& node :
           record.chain_nodes) {
        if (node.facet_point_ids.size() != order) {
          throw std::logic_error(
              "a replayable arm-root path node has the wrong order");
        }
        pending_counters.chain_node_point_id_reference_count = checked_add(
            pending_counters.chain_node_point_id_reference_count,
            node.facet_point_ids.size(),
            "the retained arm-root point-reference count overflows");
      }
      pending_by_candidate[arm_record_index].emplace(std::move(record));
    }
  }

  if (pending_counters.critical_arm_family_build_count !=
          source_journal.saddle_records.size() ||
      pending_counters.path_record_count !=
          source_composition.arm_candidates.size() ||
      pending_counters.arm_candidate_path_reconciliation_count !=
          source_composition.arm_candidates.size() ||
      pending_counters.matched_local_reduced_root_path_count +
              pending_counters.omitted_isolated_singleton_path_count !=
          source_composition.arm_candidates.size() ||
      pending_counters.path_record_count >
          budget.maximum_path_record_count ||
      pending_counters.committed_chain_segment_count >
          budget.maximum_committed_chain_segment_count ||
      pending_counters.composite_path_segment_count >
          budget.maximum_composite_path_segment_count ||
      pending_counters.chain_node_point_id_reference_count >
          budget.maximum_chain_node_point_id_reference_count ||
      pending_counters.exterior_constraint_count >
          budget.maximum_exterior_constraint_count) {
    throw std::logic_error(
        "the retained arm-root paths are incomplete or exceed preflight");
  }

  result.path_records.reserve(pending_by_candidate.size());
  for (std::size_t index = 0U;
       index < pending_by_candidate.size();
       ++index) {
    if (!pending_by_candidate[index].has_value() ||
        pending_by_candidate[index]->path_record_index != index ||
        pending_by_candidate[index]->arm_candidate_index != index) {
      throw std::logic_error(
          "the replayable arm-root path arena is not dense and total");
    }
    result.path_records.push_back(
        std::move(*pending_by_candidate[index]));
  }
  result.counters = pending_counters;
  result.transient_critical_arm_families_fresh_replay_certified = true;
  result.every_arm_candidate_has_one_dense_replayable_path = true;
  result.event_saddle_arm_and_terminal_class_keys_reconciled = true;
  result.exact_initial_germs_and_chain_shapes_replayable = true;
  result.exact_seams_strict_paths_and_regular_terminals_certified = true;
  result.initial_and_terminal_facets_belong_to_external_full_pi0_targets =
      true;
  result.
      target_bindings_and_reduced_dispositions_copied_without_reclassification =
      true;
  result.shared_targets_and_roots_preserve_distinct_paths = true;
  result.records_are_event_local_internal_paths_and_not_public_attachments =
      true;
  result.critical_catalog_typed_gamma_arm_root_path_overlay_certified =
      result.arm_root_path_candidate_space_size_certified &&
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
      result.diagnostic_outcomes_have_no_paths;
  if (!result.
          critical_catalog_typed_gamma_arm_root_path_overlay_certified) {
    throw std::logic_error(
        "the typed Gamma arm-root path overlay failed certification");
  }
  result.decision = PathDecision::
      complete_exhaustive_event_local_replayable_arm_root_path_overlay;
  return result;
}

}  // namespace

ExactCriticalCatalogTypedGammaArmRootPathOverlayVerification
verify_exact_critical_catalog_typed_gamma_arm_root_path_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    PathBudget budget,
    const JournalResult& source_journal,
    const RootOverlayResult& source_root_overlay,
    const CompositionResult& source_composition,
    const PathResult& result) {
  const PathResult expected = compute_path_overlay(
      cloud,
      order,
      budget,
      source_journal,
      source_root_overlay,
      source_composition);
  ExactCriticalCatalogTypedGammaArmRootPathOverlayVerification verification;
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
      result.required_path_record_capacity ==
          expected.required_path_record_capacity &&
      result.required_committed_chain_segment_capacity ==
          expected.required_committed_chain_segment_capacity &&
      result.required_composite_path_segment_capacity ==
          expected.required_composite_path_segment_capacity &&
      result.required_chain_node_point_id_reference_capacity ==
          expected.required_chain_node_point_id_reference_capacity &&
      result.required_exterior_constraint_capacity ==
          expected.required_exterior_constraint_capacity &&
      result.arm_root_path_candidate_space_size_certified ==
          expected.arm_root_path_candidate_space_size_certified;
  verification.source_composition_decision_certified =
      result.source_composition_decision ==
      expected.source_composition_decision;
  verification.source_composition_fresh_replay_certified =
      result.source_composition_fresh_replay_certified ==
      expected.source_composition_fresh_replay_certified;
  verification.path_records_certified =
      result.path_records == expected.path_records;
  verification.result_facts_certified = result_facts_match(result, expected);
  verification.counters_certified = result.counters == expected.counters;
  verification.decision_certified = result.decision == expected.decision;
  verification.scope_certified =
      result.scope == PathScope::
          bounded_n14_k10_single_order_event_local_typed_critical_arms_with_replayable_strict_descent_paths_linked_to_external_full_pi0_targets_and_separate_frozen_pre_batch_local_hgp_reduced_root_or_omitted_singleton_dispositions_only &&
      result.scope == expected.scope;
  verification.fresh_replay_certified =
      verification.requested_budget_certified &&
      verification.external_inputs_certified &&
      verification.derived_preflight_sizes_certified &&
      verification.source_composition_decision_certified &&
      verification.source_composition_fresh_replay_certified &&
      verification.path_records_certified &&
      verification.result_facts_certified &&
      verification.counters_certified && verification.decision_certified &&
      verification.scope_certified;
  verification.
      exact_critical_catalog_typed_gamma_arm_root_path_overlay_decision_certified =
      verification.fresh_replay_certified;
  return verification;
}

ExactCriticalCatalogTypedGammaArmRootPathOverlayResult
build_exact_critical_catalog_typed_gamma_arm_root_path_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    PathBudget budget,
    const JournalResult& source_journal,
    const RootOverlayResult& source_root_overlay,
    const CompositionResult& source_composition) {
  PathResult result = compute_path_overlay(
      cloud,
      order,
      budget,
      source_journal,
      source_root_overlay,
      source_composition);
  const auto verification =
      verify_exact_critical_catalog_typed_gamma_arm_root_path_overlay(
          cloud,
          order,
          budget,
          source_journal,
          source_root_overlay,
          source_composition,
          result);
  if (!verification.
          exact_critical_catalog_typed_gamma_arm_root_path_overlay_decision_certified) {
    throw std::logic_error(
        "the typed Gamma arm-root path overlay failed its fresh replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
