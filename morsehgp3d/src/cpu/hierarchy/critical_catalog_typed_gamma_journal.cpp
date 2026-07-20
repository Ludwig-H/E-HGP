#include "morsehgp3d/hierarchy/critical_catalog_typed_gamma_journal.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;
using JournalBudget = ExactCriticalCatalogTypedGammaJournalBudget;
using JournalDecision = ExactCriticalCatalogTypedGammaJournalDecision;
using JournalResult = ExactCriticalCatalogTypedGammaJournalResult;
using LabelEntry = ExactCriticalCatalogTypedGammaLabelEntry;
using LabelKind = ExactCriticalCatalogReducedGammaHistoryLabelKind;
using LabelSemantic = ExactCriticalCatalogTypedGammaLabelSemantic;
using SaddleRecord = ExactCriticalCatalogTypedGammaSaddleRecord;
using TerminalClassRecord =
    ExactCriticalCatalogTypedGammaTerminalClassRecord;
using ArmRecord = ExactCriticalCatalogTypedGammaArmRecord;
using StrictTargetRecord =
    ExactCriticalCatalogTypedGammaStrictTargetRecord;

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
        "the typed Gamma journal binomial coefficient overflows");
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
        "the nested reduced-Gamma history budget exceeds its bounded "
        "cap");
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

void validate_domain(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const JournalBudget& budget) {
  if (cloud.size() < JournalResult::minimum_supported_point_count ||
      cloud.size() > JournalResult::maximum_supported_point_count ||
      order < JournalResult::minimum_supported_order ||
      order > JournalResult::maximum_supported_order ||
      order >= cloud.size()) {
    throw std::invalid_argument(
        "the typed critical-catalog Gamma journal requires "
        "3<=n<=14, 2<=k<n and k<=10");
  }
  validate_journal_budget_caps(budget);
}

void derive_journal_preflight(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    JournalResult& result) {
  result.exhaustive_facet_count = bounded_binomial(cloud.size(), order);
  result.exhaustive_coface_count =
      bounded_binomial(cloud.size(), order + 1U);
  result.required_label_entry_capacity = checked_add(
      result.exhaustive_facet_count,
      result.exhaustive_coface_count,
      "the typed Gamma label-entry capacity overflows");
  const std::size_t maximum_support_size =
      std::min({std::size_t{4U}, order + 1U, cloud.size()});
  for (std::size_t support_size = 2U;
       support_size <= maximum_support_size;
       ++support_size) {
    result.critical_event_support_bound = checked_add(
        result.critical_event_support_bound,
        bounded_binomial(cloud.size(), support_size),
        "the typed Gamma critical-event support bound overflows");
  }
  result.critical_arm_bound = checked_multiply(
      4U,
      result.critical_event_support_bound,
      "the typed Gamma critical-arm bound overflows");
  result.required_saddle_record_capacity =
      result.critical_event_support_bound;
  result.required_terminal_class_record_capacity =
      result.critical_arm_bound;
  result.required_arm_record_capacity = result.critical_arm_bound;
  result.required_strict_target_record_capacity =
      result.critical_arm_bound;
  result.required_terminal_class_point_id_reference_capacity =
      checked_multiply(
          order + 1U,
          result.critical_arm_bound,
          "the terminal-class PointId-reference capacity overflows");
  result.required_saddle_index_reference_capacity = checked_multiply(
      2U,
      result.critical_arm_bound,
      "the saddle index-reference capacity overflows");
  result.required_target_facet_reference_capacity = checked_multiply(
      result.critical_event_support_bound,
      result.exhaustive_facet_count,
      "the strict-target facet-reference capacity overflows");
  const std::size_t target_stored_facet_label_capacity = checked_add(
      result.required_target_facet_reference_capacity,
      result.critical_arm_bound,
      "the strict-target stored facet-label capacity overflows");
  result.required_target_point_id_reference_capacity = checked_multiply(
      order,
      target_stored_facet_label_capacity,
      "the strict-target PointId-reference capacity overflows");

  if (result.required_label_entry_capacity >
          JournalBudget::maximum_supported_label_entry_count ||
      result.required_saddle_record_capacity >
          JournalBudget::maximum_supported_saddle_record_count ||
      result.required_terminal_class_record_capacity >
          JournalBudget::maximum_supported_terminal_class_record_count ||
      result.required_arm_record_capacity >
          JournalBudget::maximum_supported_arm_record_count ||
      result.required_strict_target_record_capacity >
          JournalBudget::maximum_supported_strict_target_record_count ||
      result.required_terminal_class_point_id_reference_capacity >
          JournalBudget::
              maximum_supported_terminal_class_point_id_reference_count ||
      result.required_saddle_index_reference_capacity >
          JournalBudget::maximum_supported_saddle_index_reference_count ||
      result.required_target_facet_reference_capacity >
          JournalBudget::maximum_supported_target_facet_reference_count ||
      result.required_target_point_id_reference_capacity >
          JournalBudget::maximum_supported_target_point_id_reference_count) {
    throw std::logic_error(
        "the derived typed Gamma journal preflight exceeds a certified "
        "static cap");
  }
}

[[nodiscard]] bool budget_covers_preflight(
    const JournalBudget& budget,
    const JournalResult& result) {
  return budget.maximum_label_entry_count >=
             result.required_label_entry_capacity &&
         budget.maximum_saddle_record_count >=
             result.required_saddle_record_capacity &&
         budget.maximum_terminal_class_record_count >=
             result.required_terminal_class_record_capacity &&
         budget.maximum_arm_record_count >=
             result.required_arm_record_capacity &&
         budget.maximum_strict_target_record_count >=
             result.required_strict_target_record_capacity &&
         budget.maximum_terminal_class_point_id_reference_count >=
             result.required_terminal_class_point_id_reference_capacity &&
         budget.maximum_saddle_index_reference_count >=
             result.required_saddle_index_reference_capacity &&
         budget.maximum_target_facet_reference_count >=
             result.required_target_facet_reference_capacity &&
         budget.maximum_target_point_id_reference_count >=
             result.required_target_point_id_reference_capacity;
}

[[nodiscard]] bool diagnostic_payload_empty(
    const JournalResult& result) {
  return !result.reduced_gamma_history.has_value() &&
         result.label_entries.empty() && result.saddle_records.empty() &&
         result.terminal_class_records.empty() &&
         result.arm_records.empty() &&
         result.strict_target_records.empty();
}

[[nodiscard]] const std::vector<PointId>& history_slot_label(
    const ExactPersistentReducedGammaOrderHistory& history,
    const ExactCriticalCatalogReducedGammaHistoryLabelSlot& slot) {
  if (slot.history_group_record_index >= history.group_records.size()) {
    throw std::logic_error(
        "a provenance slot references no history group record");
  }
  const ExactPersistentReducedGammaHistoryGroupRecord& group =
      history.group_records[slot.history_group_record_index];
  if (group.group_record_index != slot.history_group_record_index ||
      group.batch_index != slot.history_batch_index ||
      group.batch_index >= history.batch_metadata.size() ||
      history.batch_metadata[group.batch_index].batch_index !=
          group.batch_index ||
      history.batch_metadata[group.batch_index].squared_level !=
          group.squared_level) {
    throw std::logic_error(
        "a provenance slot has incoherent history batch metadata");
  }
  if (slot.kind == LabelKind::newly_active_facet) {
    if (slot.history_group_local_label_index >=
        group.newly_active_facet_point_ids.size()) {
      throw std::logic_error(
          "a provenance facet slot has an invalid group-local index");
    }
    return group.newly_active_facet_point_ids[
        slot.history_group_local_label_index];
  }
  if (slot.history_group_local_label_index >=
      group.equal_level_coface_point_ids.size()) {
    throw std::logic_error(
        "a provenance coface slot has an invalid group-local index");
  }
  return group.equal_level_coface_point_ids[
      slot.history_group_local_label_index];
}

struct SaddleKey {
  std::size_t catalog_event_index{};
  std::size_t catalog_h0_batch_index{};

  friend bool operator<(const SaddleKey& left, const SaddleKey& right) {
    return std::tie(
               left.catalog_event_index,
               left.catalog_h0_batch_index) <
           std::tie(
               right.catalog_event_index,
               right.catalog_h0_batch_index);
  }
};

[[nodiscard]] std::map<SaddleKey, std::size_t>
index_saddle_families(
    const ExactCriticalCatalogArmGammaOverlayResult& arm_overlay) {
  std::map<SaddleKey, std::size_t> family_index_by_key;
  for (std::size_t family_index = 0U;
       family_index < arm_overlay.saddle_family_records.size();
       ++family_index) {
    const ExactCriticalCatalogArmGammaSaddleFamilyRecord& family =
        arm_overlay.saddle_family_records[family_index];
    if (family.saddle_family_record_index != family_index ||
        !family_index_by_key
             .emplace(
                 SaddleKey{
                     family.catalog_event_index,
                     family.catalog_h0_batch_index},
                 family_index)
             .second) {
      throw std::logic_error(
          "the transient arm overlay has a duplicate saddle key");
    }
  }
  return family_index_by_key;
}

[[nodiscard]] bool contains_index(
    const std::vector<std::size_t>& indices,
    std::size_t index) {
  return std::find(indices.begin(), indices.end(), index) !=
         indices.end();
}

[[nodiscard]] bool contains_point_id(
    const std::vector<PointId>& point_ids,
    PointId point_id) {
  return std::find(point_ids.begin(), point_ids.end(), point_id) !=
         point_ids.end();
}

[[nodiscard]] bool contains_facet(
    const ExactStrictGammaComponentWitness& component,
    const std::vector<PointId>& facet) {
  return std::find(
             component.facet_point_ids.begin(),
             component.facet_point_ids.end(),
             facet) != component.facet_point_ids.end();
}

[[nodiscard]] std::vector<PointId> derive_initial_facet(
    const ExactCriticalEvent& event,
    PointId removed_shell_point_id) {
  std::vector<PointId> facet = event.closed_point_ids;
  const auto removed =
      std::find(facet.begin(), facet.end(), removed_shell_point_id);
  if (removed == facet.end() ||
      std::find(std::next(removed), facet.end(), removed_shell_point_id) !=
          facet.end()) {
    throw std::logic_error(
        "a typed arm cannot remove exactly one shell PointId from its "
        "closed catalogue label");
  }
  facet.erase(removed);
  return facet;
}

struct TargetAppendContext {
  const ExactCriticalCatalogArmGammaOverlayResult& arm_overlay;
  const ExactPersistentReducedGammaOrderHistory& history;
  JournalResult& result;
  std::map<std::size_t, std::size_t>& target_record_by_source;
};

[[nodiscard]] std::size_t append_or_validate_target(
    std::size_t source_target_index,
    std::size_t source_arm_batch_index,
    std::size_t history_batch_index,
    std::size_t history_group_record_index,
    TargetAppendContext& context) {
  if (source_target_index >= context.arm_overlay.target_components.size() ||
      source_arm_batch_index >= context.arm_overlay.batch_records.size() ||
      history_group_record_index >= context.history.group_records.size()) {
    throw std::logic_error(
        "a typed strict target references an invalid transient source");
  }
  const ExactCriticalCatalogArmGammaTargetComponent& source_target =
      context.arm_overlay.target_components[source_target_index];
  const ExactCriticalCatalogArmGammaBatchRecord& source_batch =
      context.arm_overlay.batch_records[source_arm_batch_index];
  const ExactPersistentReducedGammaHistoryGroupRecord& history_group =
      context.history.group_records[history_group_record_index];
  if (source_target.target_component_index != source_target_index ||
      source_target.batch_record_index != source_arm_batch_index ||
      source_batch.batch_record_index != source_arm_batch_index ||
      !contains_index(
          source_batch.target_component_indices, source_target_index) ||
      history_group.group_record_index != history_group_record_index ||
      history_group.batch_index != history_batch_index) {
    throw std::logic_error(
        "a transient strict target has incoherent batch provenance");
  }

  const auto existing =
      context.target_record_by_source.find(source_target_index);
  if (existing != context.target_record_by_source.end()) {
    const StrictTargetRecord& record =
        context.result.strict_target_records.at(existing->second);
    if (record.source_arm_gamma_batch_record_index !=
            source_arm_batch_index ||
        record.history_batch_index != history_batch_index ||
        record.history_group_record_index != history_group_record_index ||
        record.strict_component_index !=
            source_target.strict_component_index ||
        record.strict_component != source_target.strict_component ||
        record.reduced_component_kind !=
            source_target.reduced_component_kind) {
      throw std::logic_error(
          "arms sharing one strict target disagree on their history "
          "group");
    }
    return existing->second;
  }

  if (source_target.strict_component.facet_point_ids.empty() ||
      source_target.strict_component
              .canonical_representative_facet_point_ids.size() !=
          context.result.order ||
      source_target.strict_component
              .canonical_representative_facet_point_ids !=
          source_target.strict_component.facet_point_ids.front()) {
    throw std::logic_error(
        "a transient strict target has a malformed full-pi0 witness");
  }
  StrictTargetRecord record;
  record.strict_target_record_index =
      context.result.strict_target_records.size();
  record.source_target_component_index = source_target_index;
  record.source_arm_gamma_batch_record_index = source_arm_batch_index;
  record.history_batch_index = history_batch_index;
  record.history_group_record_index = history_group_record_index;
  record.strict_component_index = source_target.strict_component_index;
  record.strict_component = source_target.strict_component;
  record.reduced_component_kind = source_target.reduced_component_kind;
  context.result.counters.target_point_id_reference_count = checked_add(
      context.result.counters.target_point_id_reference_count,
      record.strict_component.canonical_representative_facet_point_ids.size(),
      "the actual target representative PointId count overflows");
  for (const std::vector<PointId>& facet :
       record.strict_component.facet_point_ids) {
    if (facet.size() != context.result.order) {
      throw std::logic_error(
          "a typed strict target contains a malformed facet label");
    }
    context.result.counters.target_point_id_reference_count = checked_add(
        context.result.counters.target_point_id_reference_count,
        facet.size(),
        "the actual target PointId-reference count overflows");
  }
  context.result.counters.target_facet_reference_count = checked_add(
      context.result.counters.target_facet_reference_count,
      record.strict_component.facet_point_ids.size(),
      "the actual target facet-reference count overflows");
  const std::size_t target_record_index =
      record.strict_target_record_index;
  if (!context.target_record_by_source
           .emplace(source_target_index, target_record_index)
           .second) {
    throw std::logic_error(
        "a typed strict target lost source-index uniqueness");
  }
  context.result.strict_target_records.push_back(std::move(record));
  return target_record_index;
}

void type_all_history_labels(
    const ExactCriticalCatalogReducedGammaOverlayResult& provenance,
    const ExactPersistentReducedGammaOrderHistory& history,
    JournalResult& result,
    std::vector<std::pair<std::size_t, std::size_t>>&
        saddle_projection_label_indices) {
  result.label_entries.reserve(provenance.history_label_slots.size());
  std::vector<bool> projection_used(
      provenance.event_projections.size(), false);
  for (std::size_t slot_index = 0U;
       slot_index < provenance.history_label_slots.size();
       ++slot_index) {
    const ExactCriticalCatalogReducedGammaHistoryLabelSlot& slot =
        provenance.history_label_slots[slot_index];
    if (slot.label_slot_index != slot_index) {
      throw std::logic_error(
          "a transient provenance slot lost its dense index");
    }
    const std::vector<PointId>& label =
        history_slot_label(history, slot);
    const std::size_t expected_label_size =
        slot.kind == LabelKind::newly_active_facet ? result.order
                                                   : result.order + 1U;
    if (label.size() != expected_label_size) {
      throw std::logic_error(
          "a typed history label has an invalid cardinality");
    }

    LabelEntry entry;
    entry.label_entry_index = slot_index;
    entry.history_batch_index = slot.history_batch_index;
    entry.history_group_record_index = slot.history_group_record_index;
    entry.history_label_kind = slot.kind;
    entry.history_group_local_label_index =
        slot.history_group_local_label_index;
    if (!slot.event_projection_index.has_value()) {
      if (slot.kind == LabelKind::newly_active_facet) {
        entry.semantic =
            LabelSemantic::residual_newly_active_facet;
        ++result.counters.residual_newly_active_facet_label_count;
      } else {
        entry.semantic = LabelSemantic::residual_equal_level_coface;
        ++result.counters.residual_equal_level_coface_label_count;
      }
      result.label_entries.push_back(std::move(entry));
      continue;
    }

    const std::size_t projection_index = *slot.event_projection_index;
    if (projection_index >= provenance.event_projections.size() ||
        projection_used[projection_index]) {
      throw std::logic_error(
          "a history slot has a duplicate or invalid event projection");
    }
    const ExactCriticalCatalogReducedGammaEventProjection& projection =
        provenance.event_projections[projection_index];
    if (projection.projection_index != projection_index ||
        projection.history_label_slot_index != slot_index ||
        projection.history_batch_index != slot.history_batch_index ||
        projection.history_group_record_index !=
            slot.history_group_record_index ||
        projection.history_label_kind != slot.kind ||
        projection.history_group_local_label_index !=
            slot.history_group_local_label_index ||
        !provenance.critical_catalog.has_value() ||
        projection.catalog_event_index >=
            provenance.critical_catalog->events.size() ||
        projection.catalog_h0_batch_index >=
            provenance.critical_catalog->h0_batches.size()) {
      throw std::logic_error(
          "a transient event projection is incoherent with its slot");
    }
    const ExactCriticalEvent& event =
        provenance.critical_catalog->events[projection.catalog_event_index];
    const ExactCriticalH0Batch& batch =
        provenance.critical_catalog
            ->h0_batches[projection.catalog_h0_batch_index];
    const ExactPersistentReducedGammaHistoryGroupRecord& group =
        history.group_records[slot.history_group_record_index];
    if (event.event_index != projection.catalog_event_index ||
        event.squared_level != batch.squared_level ||
        event.squared_level != group.squared_level ||
        event.closed_point_ids != label || batch.order != result.order) {
      throw std::logic_error(
          "a typed catalogue event lost its exact order, level or closed "
          "label defense");
    }
    entry.source_event_projection_index = projection_index;
    entry.catalog_event_index = projection.catalog_event_index;
    entry.catalog_h0_batch_index = projection.catalog_h0_batch_index;
    projection_used[projection_index] = true;
    if (projection.role ==
        ExactCriticalCatalogReducedGammaEventRole::birth) {
      if (slot.kind != LabelKind::newly_active_facet ||
          !event.birth_order.has_value() ||
          *event.birth_order != result.order ||
          !contains_index(
              batch.birth_event_indices, projection.catalog_event_index) ||
          group.kind != ExactReducedGammaBatchGroupKind::
                            deferred_isolated_facet) {
        throw std::logic_error(
            "a catalogue birth is not its deferred history facet");
      }
      entry.semantic = LabelSemantic::catalog_birth;
      ++result.counters.catalog_birth_label_count;
    } else {
      if (slot.kind != LabelKind::equal_level_coface ||
          !event.saddle_order.has_value() ||
          *event.saddle_order != result.order ||
          !contains_index(
              batch.saddle_event_indices,
              projection.catalog_event_index) ||
          group.kind == ExactReducedGammaBatchGroupKind::
                            deferred_isolated_facet) {
        throw std::logic_error(
            "a catalogue saddle is not its non-deferred history coface");
      }
      entry.semantic = LabelSemantic::catalog_saddle;
      saddle_projection_label_indices.emplace_back(
          projection_index, slot_index);
      ++result.counters.catalog_saddle_label_count;
    }
    result.label_entries.push_back(std::move(entry));
  }
  if (std::find(projection_used.begin(), projection_used.end(), false) !=
          projection_used.end() ||
      result.label_entries.size() !=
          result.required_label_entry_capacity ||
      result.label_entries.size() !=
          provenance.history_label_slots.size()) {
    throw std::logic_error(
        "the typed journal did not consume every history slot and event "
        "projection exactly once");
  }
  result.counters.label_entry_count = result.label_entries.size();
  result.all_history_label_slots_typed_exactly_once = true;
  result.catalog_births_are_deferred_facets_without_saddles_or_arms = true;
  result.residual_labels_typed_only_from_history_kind = true;
  result.catalog_saddles_are_non_deferred_equal_level_cofaces = true;
}

void append_saddle_records(
    const ExactCriticalCatalogReducedGammaOverlayResult& provenance,
    const ExactCriticalCatalogArmGammaOverlayResult& arm_overlay,
    const ExactPersistentReducedGammaOrderHistory& history,
    const std::vector<std::pair<std::size_t, std::size_t>>&
        saddle_projection_label_indices,
    JournalResult& result) {
  if (!provenance.critical_catalog.has_value() ||
      !arm_overlay.critical_catalog.has_value()) {
    throw std::logic_error(
        "typed saddle construction requires both transient catalogues");
  }
  const ExactCriticalCatalogResult& catalog =
      *provenance.critical_catalog;
  const std::map<SaddleKey, std::size_t> family_index_by_key =
      index_saddle_families(arm_overlay);
  std::vector<bool> family_used(
      arm_overlay.saddle_family_records.size(), false);
  std::vector<bool> source_arm_used(arm_overlay.arm_targets.size(), false);
  std::map<std::size_t, std::size_t> target_record_by_source;
  TargetAppendContext target_context{
      arm_overlay, history, result, target_record_by_source};
  result.saddle_records.reserve(saddle_projection_label_indices.size());

  for (const auto& [projection_index, label_entry_index] :
       saddle_projection_label_indices) {
    if (projection_index >= provenance.event_projections.size() ||
        label_entry_index >= result.label_entries.size()) {
      throw std::logic_error(
          "a typed saddle references an invalid projection or label");
    }
    const ExactCriticalCatalogReducedGammaEventProjection& projection =
        provenance.event_projections[projection_index];
    const SaddleKey key{
        projection.catalog_event_index,
        projection.catalog_h0_batch_index};
    const auto family_position = family_index_by_key.find(key);
    if (family_position == family_index_by_key.end() ||
        family_used[family_position->second]) {
      throw std::logic_error(
          "a catalogue saddle has no unique transient arm family");
    }
    const std::size_t family_index = family_position->second;
    const ExactCriticalCatalogArmGammaSaddleFamilyRecord& family =
        arm_overlay.saddle_family_records[family_index];
    const ExactCriticalEvent& event =
        catalog.events.at(projection.catalog_event_index);
    const ExactCriticalH0Batch& catalog_batch =
        catalog.h0_batches.at(projection.catalog_h0_batch_index);
    if (!family.reduced_gamma_batch_record_index.has_value() ||
        !family.reduced_gamma_group_index.has_value() ||
        !family.reduced_gamma_group_kind.has_value() ||
        *family.reduced_gamma_batch_record_index >=
            arm_overlay.batch_records.size() ||
        projection.history_group_record_index >=
            history.group_records.size() ||
        projection.history_batch_index >= history.batch_metadata.size()) {
      throw std::logic_error(
          "a complete transient saddle lacks its batch or group join");
    }
    const std::size_t source_arm_batch_index =
        *family.reduced_gamma_batch_record_index;
    const ExactCriticalCatalogArmGammaBatchRecord& arm_batch =
        arm_overlay.batch_records[source_arm_batch_index];
    const ExactPersistentReducedGammaHistoryGroupRecord& history_group =
        history.group_records[projection.history_group_record_index];
    const ExactPersistentReducedGammaBatchMetadata& history_batch =
        history.batch_metadata[projection.history_batch_index];
    if (family.catalog_event_index != event.event_index ||
        family.catalog_h0_batch_index !=
            projection.catalog_h0_batch_index ||
        arm_batch.batch_record_index != source_arm_batch_index ||
        arm_batch.catalog_h0_batch_index !=
            projection.catalog_h0_batch_index ||
        !contains_index(
            arm_batch.saddle_family_record_indices, family_index) ||
        catalog_batch.order != result.order ||
        catalog_batch.squared_level != event.squared_level ||
        arm_batch.squared_level != event.squared_level ||
        !event.saddle_order.has_value() ||
        *event.saddle_order != result.order ||
        event.closed_point_ids.size() != result.order + 1U ||
        history_group.group_record_index !=
            projection.history_group_record_index ||
        history_group.batch_index != projection.history_batch_index ||
        history_group.batch_group_index !=
            *family.reduced_gamma_group_index ||
        history_group.squared_level != event.squared_level ||
        history_group.kind != *family.reduced_gamma_group_kind ||
        history_group.kind == ExactReducedGammaBatchGroupKind::
                                  deferred_isolated_facet ||
        history_batch.batch_index != projection.history_batch_index ||
        history_batch.squared_level != event.squared_level) {
      throw std::logic_error(
          "a typed saddle failed its distinct catalogue-batch and "
          "history-group join");
    }

    SaddleRecord saddle;
    saddle.saddle_record_index = result.saddle_records.size();
    saddle.label_entry_index = label_entry_index;
    saddle.source_event_projection_index = projection_index;
    saddle.catalog_event_index = projection.catalog_event_index;
    saddle.catalog_h0_batch_index = projection.catalog_h0_batch_index;
    saddle.source_saddle_family_record_index = family_index;
    saddle.source_arm_gamma_batch_record_index = source_arm_batch_index;
    saddle.history_batch_index = projection.history_batch_index;
    saddle.history_group_record_index =
        projection.history_group_record_index;
    saddle.reduced_group_kind = *family.reduced_gamma_group_kind;
    result.label_entries[label_entry_index].saddle_record_index =
        saddle.saddle_record_index;

    std::vector<std::optional<std::size_t>>
        source_target_by_terminal_class(
            family.family.terminal_label_classes.size());
    if (family.arm_target_indices.size() != family.family.arms.size()) {
      throw std::logic_error(
          "a complete transient family lost its arm-target bijection");
    }
    for (const std::size_t source_arm_target_index :
         family.arm_target_indices) {
      if (source_arm_target_index >= arm_overlay.arm_targets.size()) {
        throw std::logic_error(
            "a family references an invalid transient arm target");
      }
      const ExactCriticalCatalogArmGammaArmTarget& source_arm =
          arm_overlay.arm_targets[source_arm_target_index];
      if (source_arm.arm_target_index != source_arm_target_index ||
          source_arm.saddle_family_record_index != family_index ||
          source_arm.catalog_event_index != event.event_index ||
          source_arm.order != result.order ||
          source_arm.batch_record_index != source_arm_batch_index ||
          source_arm.terminal_label_class_index >=
              source_target_by_terminal_class.size()) {
        throw std::logic_error(
            "a transient arm target has incoherent saddle provenance");
      }
      std::optional<std::size_t>& class_target =
          source_target_by_terminal_class[
              source_arm.terminal_label_class_index];
      if (class_target.has_value() &&
          *class_target != source_arm.target_component_index) {
        throw std::logic_error(
            "the arms of one terminal class disagree on their strict "
            "target");
      }
      class_target = source_arm.target_component_index;
    }

    std::vector<std::size_t> terminal_record_by_source_class(
        family.family.terminal_label_classes.size());
    for (std::size_t class_index = 0U;
         class_index < family.family.terminal_label_classes.size();
         ++class_index) {
      if (!source_target_by_terminal_class[class_index].has_value()) {
        throw std::logic_error(
            "a transient terminal class has no arm or strict target");
      }
      const ExactCriticalArmTerminalLabelClass& terminal_class =
          family.family.terminal_label_classes[class_index];
      const std::size_t source_target_index =
          *source_target_by_terminal_class[class_index];
      const std::size_t target_record_index = append_or_validate_target(
          source_target_index,
          source_arm_batch_index,
          projection.history_batch_index,
          projection.history_group_record_index,
          target_context);
      const StrictTargetRecord& target_record =
          result.strict_target_records.at(target_record_index);
      if (terminal_class.canonical_terminal.facet_point_ids.size() !=
              result.order ||
          terminal_class.removed_shell_point_ids.empty() ||
          !contains_facet(
              target_record.strict_component,
              terminal_class.canonical_terminal.facet_point_ids)) {
        throw std::logic_error(
            "a terminal label class is incoherent with its full-pi0 "
            "strict target");
      }
      TerminalClassRecord terminal_record;
      terminal_record.terminal_class_record_index =
          result.terminal_class_records.size();
      terminal_record.saddle_record_index = saddle.saddle_record_index;
      terminal_record.source_terminal_label_class_index = class_index;
      terminal_record.terminal_class = terminal_class;
      terminal_record.strict_target_record_index = target_record_index;
      result.counters.terminal_class_point_id_reference_count =
          checked_add(
              result.counters.terminal_class_point_id_reference_count,
              checked_add(
                  terminal_class.canonical_terminal.facet_point_ids.size(),
                  terminal_class.removed_shell_point_ids.size(),
                  "the terminal-class PointId count overflows"),
              "the total terminal-class PointId count overflows");
      terminal_record_by_source_class[class_index] =
          terminal_record.terminal_class_record_index;
      saddle.terminal_class_record_indices.push_back(
          terminal_record.terminal_class_record_index);
      result.terminal_class_records.push_back(
          std::move(terminal_record));
    }

    std::set<std::tuple<std::size_t, std::size_t, PointId>> arm_keys;
    for (std::size_t family_arm_index = 0U;
         family_arm_index < family.family.arms.size();
         ++family_arm_index) {
      const ExactCriticalArmFamilyArmResult& family_arm =
          family.family.arms[family_arm_index];
      const std::size_t source_arm_target_index =
          family.arm_target_indices[family_arm_index];
      const ExactCriticalCatalogArmGammaArmTarget& source_arm =
          arm_overlay.arm_targets.at(source_arm_target_index);
      if (source_arm_used[source_arm_target_index] ||
          source_arm.removed_shell_point_id !=
              family_arm.removed_shell_point_id ||
          !family_arm.terminal_label_class_index.has_value() ||
          source_arm.terminal_label_class_index !=
              *family_arm.terminal_label_class_index ||
          !family_arm.active_terminal.has_value() ||
          source_arm.terminal_label_class_index >=
              terminal_record_by_source_class.size()) {
        throw std::logic_error(
            "a transient family arm lost its terminal-class bijection");
      }
      const std::size_t terminal_record_index =
          terminal_record_by_source_class[
              source_arm.terminal_label_class_index];
      const TerminalClassRecord& terminal_record =
          result.terminal_class_records.at(terminal_record_index);
      const std::size_t target_record_index =
          terminal_record.strict_target_record_index;
      const StrictTargetRecord& target_record =
          result.strict_target_records.at(target_record_index);
      if (source_arm.target_component_index !=
              target_record.source_target_component_index ||
          source_arm.strict_component_index !=
              target_record.strict_component_index ||
          *family_arm.active_terminal !=
              terminal_record.terminal_class.canonical_terminal ||
          !contains_point_id(
              terminal_record.terminal_class.removed_shell_point_ids,
              family_arm.removed_shell_point_id)) {
        throw std::logic_error(
            "an arm endpoint is incoherent with its copied terminal "
            "class or strict target");
      }
      const std::vector<PointId> initial_facet = derive_initial_facet(
          event, family_arm.removed_shell_point_id);
      if (initial_facet != family_arm.descent.initial_segment
                               .arm_facet_point_ids ||
          !contains_facet(target_record.strict_component, initial_facet) ||
          !contains_facet(
              target_record.strict_component,
              family_arm.active_terminal->facet_point_ids)) {
        throw std::logic_error(
            "an arm initial or terminal facet escaped its strict "
            "full-pi0 target");
      }
      const auto arm_key = std::make_tuple(
          event.event_index,
          result.order,
          family_arm.removed_shell_point_id);
      if (!arm_keys.insert(arm_key).second) {
        throw std::logic_error(
            "one catalogue saddle arm was typed more than once");
      }

      ArmRecord arm;
      arm.arm_record_index = result.arm_records.size();
      arm.saddle_record_index = saddle.saddle_record_index;
      arm.terminal_class_record_index = terminal_record_index;
      arm.source_arm_target_index = source_arm_target_index;
      arm.removed_shell_point_id = family_arm.removed_shell_point_id;
      arm.strict_target_record_index = target_record_index;
      saddle.arm_record_indices.push_back(arm.arm_record_index);
      result.arm_records.push_back(std::move(arm));
      source_arm_used[source_arm_target_index] = true;
    }
    if (arm_keys.size() != family.family.arms.size()) {
      throw std::logic_error(
          "a typed saddle did not exhaust its unique arm keys");
    }
    result.counters.saddle_index_reference_count = checked_add(
        result.counters.saddle_index_reference_count,
        checked_add(
            saddle.terminal_class_record_indices.size(),
            saddle.arm_record_indices.size(),
            "the per-saddle index-reference count overflows"),
        "the total saddle index-reference count overflows");
    family_used[family_index] = true;
    result.saddle_records.push_back(std::move(saddle));
  }

  if (std::find(family_used.begin(), family_used.end(), false) !=
          family_used.end() ||
      std::find(source_arm_used.begin(), source_arm_used.end(), false) !=
          source_arm_used.end() ||
      target_record_by_source.size() !=
          arm_overlay.target_components.size() ||
      result.saddle_records.size() !=
          arm_overlay.saddle_family_records.size() ||
      result.arm_records.size() != arm_overlay.arm_targets.size() ||
      result.strict_target_records.size() !=
          arm_overlay.target_components.size()) {
    throw std::logic_error(
        "the typed saddle journal did not exhaust every transient "
        "family, arm and strict target exactly once");
  }
  result.counters.saddle_record_count = result.saddle_records.size();
  result.counters.terminal_class_record_count =
      result.terminal_class_records.size();
  result.counters.arm_record_count = result.arm_records.size();
  result.counters.strict_target_record_count =
      result.strict_target_records.size();
  result.counters.shared_strict_target_arm_count =
      result.arm_records.size() - result.strict_target_records.size();
  if (result.saddle_records.size() >
          result.required_saddle_record_capacity ||
      result.terminal_class_records.size() >
          result.required_terminal_class_record_capacity ||
      result.arm_records.size() > result.required_arm_record_capacity ||
      result.strict_target_records.size() >
          result.required_strict_target_record_capacity ||
      result.counters.terminal_class_point_id_reference_count >
          result.required_terminal_class_point_id_reference_capacity ||
      result.counters.saddle_index_reference_count >
          result.required_saddle_index_reference_capacity ||
      result.counters.target_facet_reference_count >
          result.required_target_facet_reference_capacity ||
      result.counters.target_point_id_reference_count >
          result.required_target_point_id_reference_capacity) {
    throw std::logic_error(
        "the actual typed journal arenas violated their preflight");
  }
  result.every_catalog_saddle_has_exactly_one_record = true;
  result.every_terminal_class_has_one_shared_strict_target = true;
  result.every_arm_has_one_terminal_class_and_strict_target = true;
  result.every_source_family_arm_class_and_target_used_exactly_once = true;
  result.arm_initial_facets_derived_from_closed_labels = true;
  result.full_pi0_witnesses_retain_target_authority = true;
  result.reduced_component_and_group_kinds_are_annotations_only = true;
  result.all_saddle_targets_join_their_history_group = true;
}

[[nodiscard]] bool result_facts_match(
    const JournalResult& observed,
    const JournalResult& expected) {
  return observed.journal_candidate_space_size_certified ==
             expected.journal_candidate_space_size_certified &&
         observed.journal_preflight_budget_sufficient ==
             expected.journal_preflight_budget_sufficient &&
         observed.source_budget_seam_certified ==
             expected.source_budget_seam_certified &&
         observed.
                 subordinate_geometry_started_only_after_successful_journal_preflight ==
             expected.
                 subordinate_geometry_started_only_after_successful_journal_preflight &&
         observed.provenance_overlay_fresh_replay_certified ==
             expected.provenance_overlay_fresh_replay_certified &&
         observed.arm_overlay_started_only_after_complete_provenance ==
             expected.arm_overlay_started_only_after_complete_provenance &&
         observed.arm_overlay_fresh_replay_certified ==
             expected.arm_overlay_fresh_replay_certified &&
         observed.source_catalogs_identical ==
             expected.source_catalogs_identical &&
         observed.source_objects_transient ==
             expected.source_objects_transient &&
         observed.all_history_label_slots_typed_exactly_once ==
             expected.all_history_label_slots_typed_exactly_once &&
         observed.catalog_births_are_deferred_facets_without_saddles_or_arms ==
             expected.catalog_births_are_deferred_facets_without_saddles_or_arms &&
         observed.residual_labels_typed_only_from_history_kind ==
             expected.residual_labels_typed_only_from_history_kind &&
         observed.catalog_saddles_are_non_deferred_equal_level_cofaces ==
             expected.catalog_saddles_are_non_deferred_equal_level_cofaces &&
         observed.every_catalog_saddle_has_exactly_one_record ==
             expected.every_catalog_saddle_has_exactly_one_record &&
         observed.every_terminal_class_has_one_shared_strict_target ==
             expected.every_terminal_class_has_one_shared_strict_target &&
         observed.every_arm_has_one_terminal_class_and_strict_target ==
             expected.every_arm_has_one_terminal_class_and_strict_target &&
         observed.every_source_family_arm_class_and_target_used_exactly_once ==
             expected.every_source_family_arm_class_and_target_used_exactly_once &&
         observed.arm_initial_facets_derived_from_closed_labels ==
             expected.arm_initial_facets_derived_from_closed_labels &&
         observed.full_pi0_witnesses_retain_target_authority ==
             expected.full_pi0_witnesses_retain_target_authority &&
         observed.reduced_component_and_group_kinds_are_annotations_only ==
             expected.reduced_component_and_group_kinds_are_annotations_only &&
         observed.all_saddle_targets_join_their_history_group ==
             expected.all_saddle_targets_join_their_history_group &&
         observed.reduced_gamma_history_stored_fresh ==
             expected.reduced_gamma_history_stored_fresh &&
         observed.diagnostic_outcomes_have_no_journal_or_history ==
             expected.diagnostic_outcomes_have_no_journal_or_history &&
         observed.critical_catalog_typed_gamma_journal_certified ==
             expected.critical_catalog_typed_gamma_journal_certified;
}

[[nodiscard]] JournalResult
compute_exact_critical_catalog_typed_gamma_journal(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    JournalBudget budget) {
  validate_domain(cloud, order, budget);
  JournalResult result;
  result.requested_budget = budget;
  result.point_count = cloud.size();
  result.order = order;
  result.scope = ExactCriticalCatalogTypedGammaJournalScope::
      bounded_n14_k10_single_order_exhaustive_gamma_groups_typed_catalog_h0_roles_and_strict_full_pi0_arm_targets_with_separate_hgp_reduced_effect_annotations_only;
  result.counters.preflight_count = 1U;
  result.source_budget_seam_certified =
      budget.provenance_overlay_budget.critical_catalog_budget ==
          budget.arm_overlay_budget.critical_catalog_budget &&
      budget.provenance_overlay_budget.reduced_gamma_history_budget
              .gamma_budget ==
          budget.arm_overlay_budget.reduced_gamma_batch_budget;
  result.diagnostic_outcomes_have_no_journal_or_history = true;
  derive_journal_preflight(cloud, order, result);
  result.journal_candidate_space_size_certified = true;
  result.journal_preflight_budget_sufficient =
      result.source_budget_seam_certified &&
      budget_covers_preflight(budget, result);
  result.
      subordinate_geometry_started_only_after_successful_journal_preflight =
      true;
  if (!result.journal_preflight_budget_sufficient) {
    result.decision =
        JournalDecision::no_journal_preflight_budget_insufficient;
    return result;
  }

  ExactCriticalCatalogReducedGammaOverlayResult provenance =
      build_exact_critical_catalog_reduced_gamma_overlay(
          cloud, order, budget.provenance_overlay_budget);
  ++result.counters.provenance_overlay_build_count;
  const ExactCriticalCatalogReducedGammaOverlayVerification
      provenance_verification =
          verify_exact_critical_catalog_reduced_gamma_overlay(
              cloud,
              order,
              budget.provenance_overlay_budget,
              provenance);
  ++result.counters.provenance_overlay_verification_count;
  if (!provenance_verification.
          exact_critical_catalog_reduced_gamma_overlay_decision_certified) {
    throw std::logic_error(
        "the transient provenance overlay failed its fresh verifier");
  }
  result.provenance_overlay_fresh_replay_certified = true;
  result.provenance_overlay_decision = provenance.decision;
  if (provenance.decision !=
      ExactCriticalCatalogReducedGammaOverlayDecision::
          complete_exhaustive_critical_catalog_reduced_gamma_overlay) {
    if (!diagnostic_payload_empty(result)) {
      throw std::logic_error(
          "an incomplete provenance diagnostic retained journal "
          "payload");
    }
    result.decision =
        JournalDecision::no_journal_provenance_overlay_incomplete;
    return result;
  }
  result.arm_overlay_started_only_after_complete_provenance = true;

  ExactCriticalCatalogArmGammaOverlayResult arm_overlay =
      build_exact_critical_catalog_arm_gamma_overlay(
          cloud, order, budget.arm_overlay_budget);
  ++result.counters.arm_overlay_build_count;
  const ExactCriticalCatalogArmGammaOverlayVerification
      arm_verification = verify_exact_critical_catalog_arm_gamma_overlay(
          cloud, order, budget.arm_overlay_budget, arm_overlay);
  ++result.counters.arm_overlay_verification_count;
  if (!arm_verification.
          exact_critical_catalog_arm_gamma_overlay_decision_certified) {
    throw std::logic_error(
        "the transient arm overlay failed its fresh verifier");
  }
  result.arm_overlay_fresh_replay_certified = true;
  result.arm_overlay_decision = arm_overlay.decision;
  if (arm_overlay.decision !=
      ExactCriticalCatalogArmGammaOverlayDecision::
          complete_exhaustive_catalog_saddle_arm_gamma_overlay) {
    if (!diagnostic_payload_empty(result)) {
      throw std::logic_error(
          "an incomplete arm-overlay diagnostic retained journal "
          "payload");
    }
    result.decision = JournalDecision::no_journal_arm_overlay_incomplete;
    return result;
  }

  if (!provenance.critical_catalog.has_value() ||
      !provenance.reduced_gamma_history.has_value() ||
      !arm_overlay.critical_catalog.has_value() ||
      *provenance.critical_catalog != *arm_overlay.critical_catalog) {
    throw std::logic_error(
        "the two transient overlays did not expose identical fresh "
        "catalogues");
  }
  ++result.counters.source_catalog_comparison_count;
  result.source_catalogs_identical = true;
  const ExactPersistentReducedGammaOrderHistoryVerification
      history_verification =
          verify_exact_persistent_reduced_gamma_order_history(
              cloud,
              order,
              budget.provenance_overlay_budget
                  .reduced_gamma_history_budget,
              *provenance.reduced_gamma_history);
  ++result.counters.history_verification_count;
  if (!history_verification.
          exact_persistent_reduced_gamma_order_history_decision_certified ||
      provenance.reduced_gamma_history->decision !=
          ExactPersistentReducedGammaOrderHistoryDecision::
              complete_persistent_reduced_gamma_history ||
      provenance.reduced_gamma_history->exhaustive_facet_count !=
          result.exhaustive_facet_count ||
      provenance.reduced_gamma_history->exhaustive_coface_count !=
          result.exhaustive_coface_count) {
    throw std::logic_error(
        "the transient provenance history is not a fresh complete "
        "single-order history");
  }

  std::vector<std::pair<std::size_t, std::size_t>>
      saddle_projection_label_indices;
  type_all_history_labels(
      provenance,
      *provenance.reduced_gamma_history,
      result,
      saddle_projection_label_indices);
  append_saddle_records(
      provenance,
      arm_overlay,
      *provenance.reduced_gamma_history,
      saddle_projection_label_indices,
      result);
  result.reduced_gamma_history =
      std::move(*provenance.reduced_gamma_history);
  result.reduced_gamma_history_stored_fresh = true;
  result.source_objects_transient = true;

  result.critical_catalog_typed_gamma_journal_certified =
      result.journal_candidate_space_size_certified &&
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
      result.diagnostic_outcomes_have_no_journal_or_history;
  if (!result.critical_catalog_typed_gamma_journal_certified) {
    throw std::logic_error(
        "the exhaustive typed critical-catalog Gamma journal failed "
        "certification");
  }
  result.decision =
      JournalDecision::complete_exhaustive_typed_gamma_journal;
  return result;
}

}  // namespace

ExactCriticalCatalogTypedGammaJournalVerification
verify_exact_critical_catalog_typed_gamma_journal(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    JournalBudget budget,
    const JournalResult& result) {
  const JournalResult expected =
      compute_exact_critical_catalog_typed_gamma_journal(
          cloud, order, budget);
  ExactCriticalCatalogTypedGammaJournalVerification verification;
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
      result.required_label_entry_capacity ==
          expected.required_label_entry_capacity &&
      result.required_saddle_record_capacity ==
          expected.required_saddle_record_capacity &&
      result.required_terminal_class_record_capacity ==
          expected.required_terminal_class_record_capacity &&
      result.required_arm_record_capacity ==
          expected.required_arm_record_capacity &&
      result.required_strict_target_record_capacity ==
          expected.required_strict_target_record_capacity &&
      result.required_terminal_class_point_id_reference_capacity ==
          expected.required_terminal_class_point_id_reference_capacity &&
      result.required_saddle_index_reference_capacity ==
          expected.required_saddle_index_reference_capacity &&
      result.required_target_facet_reference_capacity ==
          expected.required_target_facet_reference_capacity &&
      result.required_target_point_id_reference_capacity ==
          expected.required_target_point_id_reference_capacity &&
      result.journal_candidate_space_size_certified ==
          expected.journal_candidate_space_size_certified;
  verification.source_decisions_certified =
      result.provenance_overlay_decision ==
          expected.provenance_overlay_decision &&
      result.arm_overlay_decision == expected.arm_overlay_decision;
  verification.reduced_gamma_history_certified =
      result.reduced_gamma_history == expected.reduced_gamma_history;
  if (result.reduced_gamma_history.has_value()) {
    const ExactPersistentReducedGammaOrderHistoryVerification
        history_verification =
            verify_exact_persistent_reduced_gamma_order_history(
                cloud,
                order,
                budget.provenance_overlay_budget
                    .reduced_gamma_history_budget,
                *result.reduced_gamma_history);
    verification.reduced_gamma_history_certified =
        verification.reduced_gamma_history_certified &&
        history_verification.
            exact_persistent_reduced_gamma_order_history_decision_certified;
  }
  verification.label_entries_certified =
      result.label_entries == expected.label_entries;
  verification.saddle_records_certified =
      result.saddle_records == expected.saddle_records;
  verification.terminal_class_records_certified =
      result.terminal_class_records == expected.terminal_class_records;
  verification.arm_records_certified =
      result.arm_records == expected.arm_records;
  verification.strict_target_records_certified =
      result.strict_target_records == expected.strict_target_records;
  verification.result_facts_certified =
      result_facts_match(result, expected);
  verification.counters_certified = result.counters == expected.counters;
  verification.decision_certified = result.decision == expected.decision;
  verification.scope_certified =
      result.scope == ExactCriticalCatalogTypedGammaJournalScope::
          bounded_n14_k10_single_order_exhaustive_gamma_groups_typed_catalog_h0_roles_and_strict_full_pi0_arm_targets_with_separate_hgp_reduced_effect_annotations_only &&
      result.scope == expected.scope;
  verification.fresh_replay_certified =
      verification.requested_budget_certified &&
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
      verification.counters_certified &&
      verification.decision_certified && verification.scope_certified;
  verification.
      exact_critical_catalog_typed_gamma_journal_decision_certified =
      verification.fresh_replay_certified;
  return verification;
}

ExactCriticalCatalogTypedGammaJournalResult
build_exact_critical_catalog_typed_gamma_journal(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    JournalBudget budget) {
  JournalResult result =
      compute_exact_critical_catalog_typed_gamma_journal(
          cloud, order, budget);
  const ExactCriticalCatalogTypedGammaJournalVerification verification =
      verify_exact_critical_catalog_typed_gamma_journal(
          cloud, order, budget, result);
  if (!verification.
          exact_critical_catalog_typed_gamma_journal_decision_certified) {
    throw std::logic_error(
        "the exact typed critical-catalog Gamma journal failed its fresh "
        "replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
