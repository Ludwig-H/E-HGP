#include "morsehgp3d/hierarchy/critical_catalog_reduced_gamma_overlay.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;
using EventRole = ExactCriticalCatalogReducedGammaEventRole;
using LabelKind = ExactCriticalCatalogReducedGammaHistoryLabelKind;
using OverlayBudget = ExactCriticalCatalogReducedGammaOverlayBudget;
using OverlayDecision = ExactCriticalCatalogReducedGammaOverlayDecision;
using OverlayResult = ExactCriticalCatalogReducedGammaOverlayResult;

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
        "the critical-catalog reduced-Gamma overlay binomial "
        "coefficient overflows");
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

void validate_reduced_gamma_history_budget_caps(
    const ExactPersistentReducedGammaOrderHistoryBudget& budget) {
  if (budget.gamma_budget.maximum_enumerated_facet_count >
          ExactStrictGammaBudget::maximum_supported_facet_count ||
      budget.gamma_budget.maximum_enumerated_coface_count >
          ExactStrictGammaBudget::maximum_supported_coface_count ||
      budget.gamma_budget.maximum_union_attempt_count >
          ExactStrictGammaBudget::maximum_supported_union_attempt_count ||
      budget.maximum_activation_level_count >
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
        "the nested persistent reduced-Gamma history budget exceeds its "
        "bounded cap");
  }
}

void validate_overlay_budget_caps(const OverlayBudget& budget) {
  validate_critical_catalog_budget_caps(budget.critical_catalog_budget);
  validate_reduced_gamma_history_budget_caps(
      budget.reduced_gamma_history_budget);
  if (budget.maximum_event_projection_count >
          OverlayBudget::maximum_supported_event_projection_count ||
      budget.maximum_group_overlay_count >
          OverlayBudget::maximum_supported_group_overlay_count ||
      budget.maximum_label_slot_count >
          OverlayBudget::maximum_supported_label_slot_count ||
      budget.maximum_history_point_id_scan_count >
          OverlayBudget::maximum_supported_history_point_id_scan_count ||
      budget.maximum_catalog_point_id_scan_count >
          OverlayBudget::maximum_supported_catalog_point_id_scan_count ||
      budget.maximum_group_event_reference_count >
          OverlayBudget::maximum_supported_group_event_reference_count) {
    throw std::invalid_argument(
        "a critical-catalog reduced-Gamma overlay capacity exceeds its "
        "bounded cap");
  }
}

void validate_domain(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const OverlayBudget& budget) {
  if (cloud.size() < OverlayResult::minimum_supported_point_count ||
      cloud.size() > OverlayResult::maximum_supported_point_count ||
      order < OverlayResult::minimum_supported_order ||
      order > OverlayResult::maximum_supported_order ||
      order >= cloud.size()) {
    throw std::invalid_argument(
        "the critical-catalog reduced-Gamma overlay requires "
        "2<=k<n<=14 and k<=10");
  }
  validate_overlay_budget_caps(budget);
}

[[nodiscard]] std::size_t selected_event_support_bound(
    std::size_t point_count,
    std::size_t order) {
  const std::size_t maximum_support_size =
      std::min({std::size_t{4U}, order + 1U, point_count});
  std::size_t count = 0U;
  for (std::size_t support_size = 2U;
       support_size <= maximum_support_size;
       ++support_size) {
    count = checked_add(
        count,
        bounded_binomial(point_count, support_size),
        "the selected critical-event support bound overflows");
  }
  return count;
}

[[nodiscard]] bool overlay_budget_covers_preflight(
    const OverlayBudget& budget,
    const OverlayResult& result) {
  return budget.maximum_event_projection_count >=
             result.required_event_projection_capacity &&
         budget.maximum_group_overlay_count >=
             result.required_group_overlay_capacity &&
         budget.maximum_label_slot_count >=
             result.required_label_slot_capacity &&
         budget.maximum_history_point_id_scan_count >=
             result.required_history_point_id_scan_capacity &&
         budget.maximum_catalog_point_id_scan_count >=
             result.required_catalog_point_id_scan_capacity &&
         budget.maximum_group_event_reference_count >=
             result.required_group_event_reference_capacity;
}

struct HistoryLabelKey {
  exact::ExactLevel squared_level;
  LabelKind kind{LabelKind::newly_active_facet};
  std::vector<PointId> point_ids;
};

struct HistoryLabelKeyLess {
  [[nodiscard]] bool operator()(
      const HistoryLabelKey& left,
      const HistoryLabelKey& right) const {
    if (left.squared_level != right.squared_level) {
      return left.squared_level < right.squared_level;
    }
    if (left.kind != right.kind) {
      return static_cast<std::uint8_t>(left.kind) <
             static_cast<std::uint8_t>(right.kind);
    }
    return left.point_ids < right.point_ids;
  }
};

struct CatalogRoleReference {
  std::size_t catalog_event_index{};
  std::size_t catalog_h0_batch_index{};
  EventRole role{EventRole::birth};
};

[[nodiscard]] bool catalog_role_reference_less(
    const CatalogRoleReference& left,
    const CatalogRoleReference& right) {
  if (left.catalog_event_index != right.catalog_event_index) {
    return left.catalog_event_index < right.catalog_event_index;
  }
  return static_cast<std::uint8_t>(left.role) <
         static_cast<std::uint8_t>(right.role);
}

[[nodiscard]] bool result_facts_match(
    const OverlayResult& observed,
    const OverlayResult& expected) {
  return observed.overlay_candidate_space_size_certified ==
             expected.overlay_candidate_space_size_certified &&
         observed.overlay_preflight_budget_sufficient ==
             expected.overlay_preflight_budget_sufficient &&
         observed.
                 subordinate_geometry_started_only_after_successful_overlay_preflight ==
             expected.
                 subordinate_geometry_started_only_after_successful_overlay_preflight &&
         observed.critical_catalog_fresh_replay_certified ==
             expected.critical_catalog_fresh_replay_certified &&
         observed.no_relevant_extra_shell_degeneracy ==
             expected.no_relevant_extra_shell_degeneracy &&
         observed.reduced_gamma_history_fresh_replay_certified ==
             expected.reduced_gamma_history_fresh_replay_certified &&
         observed.history_equality_slots_exhaustively_indexed ==
             expected.history_equality_slots_exhaustively_indexed &&
         observed.closed_label_theorem_applied_to_every_projection ==
             expected.closed_label_theorem_applied_to_every_projection &&
         observed.every_catalog_h0_role_projected_exactly_once ==
             expected.every_catalog_h0_role_projected_exactly_once &&
         observed.every_history_label_slot_partitioned_by_provenance ==
             expected.every_history_label_slot_partitioned_by_provenance &&
         observed.catalog_births_exactly_deferred_newly_active_facets ==
             expected.catalog_births_exactly_deferred_newly_active_facets &&
         observed.catalog_saddles_only_non_deferred_groups ==
             expected.catalog_saddles_only_non_deferred_groups &&
         observed.group_kinds_inherited_only_from_history ==
             expected.group_kinds_inherited_only_from_history &&
         observed.simultaneous_history_batches_preserved ==
             expected.simultaneous_history_batches_preserved &&
         observed.
                 birth_projection_plus_residual_facet_count_equals_exhaustive_facet_count ==
             expected.
                 birth_projection_plus_residual_facet_count_equals_exhaustive_facet_count &&
         observed.
                 saddle_projection_plus_residual_coface_count_equals_exhaustive_coface_count ==
             expected.
                 saddle_projection_plus_residual_coface_count_equals_exhaustive_coface_count &&
         observed.critical_catalog_reduced_gamma_overlay_certified ==
             expected.critical_catalog_reduced_gamma_overlay_certified;
}

void derive_overlay_preflight(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    OverlayResult& result) {
  result.exhaustive_facet_count = bounded_binomial(cloud.size(), order);
  result.exhaustive_coface_count =
      bounded_binomial(cloud.size(), order + 1U);
  result.required_label_slot_capacity = checked_add(
      result.exhaustive_facet_count,
      result.exhaustive_coface_count,
      "the overlay equality-slot capacity overflows");
  result.required_group_overlay_capacity =
      result.required_label_slot_capacity;
  result.required_event_projection_capacity = std::min(
      result.required_label_slot_capacity,
      selected_event_support_bound(cloud.size(), order));
  result.required_history_point_id_scan_capacity = checked_add(
      checked_multiply(
          order,
          result.exhaustive_facet_count,
          "the overlay history facet PointId scan overflows"),
      checked_multiply(
          order + 1U,
          result.exhaustive_coface_count,
          "the overlay history coface PointId scan overflows"),
      "the overlay history PointId scan overflows");
  result.required_catalog_point_id_scan_capacity = checked_multiply(
      order + 1U,
      result.required_event_projection_capacity,
      "the overlay catalogue closed-label PointId scan overflows");
  result.required_group_event_reference_capacity =
      result.required_event_projection_capacity;
  if (result.required_event_projection_capacity >
          OverlayBudget::maximum_supported_event_projection_count ||
      result.required_group_overlay_capacity >
          OverlayBudget::maximum_supported_group_overlay_count ||
      result.required_label_slot_capacity >
          OverlayBudget::maximum_supported_label_slot_count ||
      result.required_history_point_id_scan_capacity >
          OverlayBudget::maximum_supported_history_point_id_scan_count ||
      result.required_catalog_point_id_scan_capacity >
          OverlayBudget::maximum_supported_catalog_point_id_scan_count ||
      result.required_group_event_reference_capacity >
          OverlayBudget::maximum_supported_group_event_reference_count) {
    throw std::logic_error(
        "the derived overlay preflight exceeds a certified static cap");
  }
}

void append_history_label_slot(
    const ExactPersistentReducedGammaHistoryGroupRecord& record,
    std::size_t local_label_index,
    LabelKind kind,
    const std::vector<PointId>& label,
    std::size_t order,
    OverlayResult& result,
    std::map<HistoryLabelKey, std::size_t, HistoryLabelKeyLess>&
        slot_by_key) {
  const std::size_t expected_label_size =
      kind == LabelKind::newly_active_facet ? order : order + 1U;
  if (label.size() != expected_label_size ||
      !std::is_sorted(label.begin(), label.end()) ||
      std::adjacent_find(label.begin(), label.end()) != label.end()) {
    throw std::logic_error(
        "a freshly verified Gamma history exposed a malformed equality "
        "label");
  }
  ExactCriticalCatalogReducedGammaHistoryLabelSlot slot;
  slot.label_slot_index = result.history_label_slots.size();
  slot.history_batch_index = record.batch_index;
  slot.history_group_record_index = record.group_record_index;
  slot.kind = kind;
  slot.history_group_local_label_index = local_label_index;
  const HistoryLabelKey key{record.squared_level, kind, label};
  if (!slot_by_key.emplace(key, slot.label_slot_index).second) {
    throw std::logic_error(
        "a Gamma equality label appears more than once in the history");
  }
  result.history_label_slots.push_back(std::move(slot));
  result.counters.history_point_id_scan_count = checked_add(
      result.counters.history_point_id_scan_count,
      label.size(),
      "the actual overlay history PointId scan overflows");
  if (kind == LabelKind::newly_active_facet) {
    ++result.counters.history_newly_active_facet_slot_count;
  } else {
    ++result.counters.history_equal_level_coface_slot_count;
  }
}

void index_history_equality_slots(
    const ExactPersistentReducedGammaOrderHistory& history,
    OverlayResult& result,
    std::map<HistoryLabelKey, std::size_t, HistoryLabelKeyLess>&
        slot_by_key) {
  result.history_label_slots.reserve(result.required_label_slot_capacity);
  result.group_overlays.reserve(history.group_records.size());
  result.counters.history_batch_count = history.batch_metadata.size();
  result.counters.history_group_count = history.group_records.size();
  for (std::size_t record_index = 0U;
       record_index < history.group_records.size();
       ++record_index) {
    const ExactPersistentReducedGammaHistoryGroupRecord& record =
        history.group_records[record_index];
    if (record.group_record_index != record_index ||
        record.batch_index >= history.batch_metadata.size() ||
        history.batch_metadata[record.batch_index].squared_level !=
            record.squared_level) {
      throw std::logic_error(
          "a freshly verified Gamma history exposed an invalid group "
          "record index");
    }
    ExactCriticalCatalogReducedGammaGroupOverlay group_overlay;
    group_overlay.history_group_record_index = record_index;
    group_overlay.history_batch_index = record.batch_index;
    group_overlay.first_label_slot_index =
        result.history_label_slots.size();
    for (std::size_t label_index = 0U;
         label_index < record.newly_active_facet_point_ids.size();
         ++label_index) {
      append_history_label_slot(
          record,
          label_index,
          LabelKind::newly_active_facet,
          record.newly_active_facet_point_ids[label_index],
          result.order,
          result,
          slot_by_key);
    }
    for (std::size_t label_index = 0U;
         label_index < record.equal_level_coface_point_ids.size();
         ++label_index) {
      append_history_label_slot(
          record,
          label_index,
          LabelKind::equal_level_coface,
          record.equal_level_coface_point_ids[label_index],
          result.order,
          result,
          slot_by_key);
    }
    group_overlay.label_slot_count =
        result.history_label_slots.size() -
        group_overlay.first_label_slot_index;
    if (group_overlay.label_slot_count == 0U) {
      throw std::logic_error(
          "a freshly verified Gamma history group has no equality label");
    }
    result.group_overlays.push_back(std::move(group_overlay));
  }
  result.counters.label_slot_count = result.history_label_slots.size();
  if (result.history_label_slots.size() !=
          result.required_label_slot_capacity ||
      result.counters.history_newly_active_facet_slot_count !=
          result.exhaustive_facet_count ||
      result.counters.history_equal_level_coface_slot_count !=
          result.exhaustive_coface_count ||
      result.counters.history_point_id_scan_count !=
          result.required_history_point_id_scan_capacity ||
      slot_by_key.size() != result.history_label_slots.size() ||
      result.group_overlays.size() != history.group_records.size() ||
      result.group_overlays.size() >
          result.required_group_overlay_capacity) {
    throw std::logic_error(
        "the exhaustive Gamma equality slots violated overlay preflight");
  }
  result.history_equality_slots_exhaustively_indexed = true;
}

[[nodiscard]] std::vector<CatalogRoleReference>
selected_catalog_role_references(
    const ExactCriticalCatalogResult& catalog,
    std::size_t order,
    OverlayResult& result) {
  std::vector<CatalogRoleReference> references;
  references.reserve(result.required_event_projection_capacity);
  for (std::size_t batch_index = 0U;
       batch_index < catalog.h0_batches.size();
       ++batch_index) {
    const ExactCriticalH0Batch& batch = catalog.h0_batches[batch_index];
    ++result.counters.catalog_h0_batch_scan_count;
    if (batch.order != order) {
      continue;
    }
    for (const std::size_t event_index : batch.birth_event_indices) {
      references.push_back(
          CatalogRoleReference{event_index, batch_index, EventRole::birth});
      ++result.counters.catalog_birth_reference_count;
    }
    for (const std::size_t event_index : batch.saddle_event_indices) {
      references.push_back(CatalogRoleReference{
          event_index, batch_index, EventRole::saddle});
      ++result.counters.catalog_saddle_reference_count;
    }
  }
  if (references.size() > result.required_event_projection_capacity) {
    throw std::logic_error(
        "the selected catalogue H0 roles exceeded their support bound");
  }
  std::sort(
      references.begin(), references.end(), catalog_role_reference_less);
  for (std::size_t index = 1U; index < references.size(); ++index) {
    if (references[index - 1U].catalog_event_index ==
        references[index].catalog_event_index) {
      throw std::logic_error(
          "one catalogue event has two roles at the same order");
    }
  }
  return references;
}

void project_catalog_roles(
    const ExactCriticalCatalogResult& catalog,
    const ExactPersistentReducedGammaOrderHistory& history,
    const std::vector<CatalogRoleReference>& references,
    const std::map<HistoryLabelKey, std::size_t, HistoryLabelKeyLess>&
        slot_by_key,
    OverlayResult& result) {
  result.event_projections.reserve(
      result.required_event_projection_capacity);
  for (const CatalogRoleReference& reference : references) {
    if (reference.catalog_event_index >= catalog.events.size() ||
        reference.catalog_h0_batch_index >= catalog.h0_batches.size()) {
      throw std::logic_error(
          "a freshly verified catalogue H0 role has an invalid index");
    }
    const ExactCriticalEvent& event =
        catalog.events[reference.catalog_event_index];
    const ExactCriticalH0Batch& catalog_batch =
        catalog.h0_batches[reference.catalog_h0_batch_index];
    const bool is_birth = reference.role == EventRole::birth;
    const std::size_t expected_closed_rank =
        is_birth ? result.order : result.order + 1U;
    const LabelKind expected_label_kind = is_birth
        ? LabelKind::newly_active_facet
        : LabelKind::equal_level_coface;
    const bool role_is_coherent = is_birth
        ? event.birth_order == std::optional<std::size_t>{result.order}
        : event.saddle_order == std::optional<std::size_t>{result.order};
    if (event.event_index != reference.catalog_event_index ||
        catalog_batch.order != result.order ||
        catalog_batch.squared_level != event.squared_level ||
        !role_is_coherent || event.closed_rank != expected_closed_rank ||
        event.closed_point_ids.size() != expected_closed_rank ||
        event.support_point_ids != event.shell_point_ids) {
      throw std::logic_error(
          "a freshly verified catalogue H0 role violates the closed-label "
          "theorem preconditions");
    }
    result.counters.catalog_closed_label_point_id_scan_count = checked_add(
        result.counters.catalog_closed_label_point_id_scan_count,
        event.closed_point_ids.size(),
        "the actual catalogue closed-label PointId scan overflows");
    const auto slot_position = slot_by_key.find(HistoryLabelKey{
        event.squared_level,
        expected_label_kind,
        event.closed_point_ids});
    if (slot_position == slot_by_key.end()) {
      throw std::logic_error(
          "an accepted catalogue H0 closed label has no exhaustive Gamma "
          "equality slot");
    }
    const std::size_t slot_index = slot_position->second;
    if (slot_index >= result.history_label_slots.size()) {
      throw std::logic_error(
          "a Gamma label index escaped the bounded slot arena");
    }
    ExactCriticalCatalogReducedGammaHistoryLabelSlot& slot =
        result.history_label_slots[slot_index];
    if (slot.event_projection_index.has_value() ||
        slot.kind != expected_label_kind ||
        slot.history_group_record_index >= history.group_records.size() ||
        slot.history_group_record_index >= result.group_overlays.size()) {
      throw std::logic_error(
          "two catalogue roles collide or a Gamma slot is malformed");
    }
    const ExactPersistentReducedGammaHistoryGroupRecord& history_group =
        history.group_records[slot.history_group_record_index];
    const bool history_kind_is_deferred =
        history_group.kind == ExactReducedGammaBatchGroupKind::
                                  deferred_isolated_facet;
    // This is an audit after the label-only join. The catalogue never selects
    // a group or determines its kind.
    if ((is_birth && !history_kind_is_deferred) ||
        (!is_birth && history_kind_is_deferred)) {
      throw std::logic_error(
          "a catalogue birth/deferred or saddle/nondeferred theorem was "
          "contradicted by the exhaustive Gamma history");
    }
    ExactCriticalCatalogReducedGammaEventProjection projection;
    projection.projection_index = result.event_projections.size();
    projection.catalog_event_index = reference.catalog_event_index;
    projection.catalog_h0_batch_index =
        reference.catalog_h0_batch_index;
    projection.role = reference.role;
    projection.history_batch_index = slot.history_batch_index;
    projection.history_group_record_index =
        slot.history_group_record_index;
    projection.history_label_slot_index = slot_index;
    projection.history_label_kind = slot.kind;
    projection.history_group_local_label_index =
        slot.history_group_local_label_index;
    result.event_projections.push_back(std::move(projection));
    slot.event_projection_index =
        result.event_projections.back().projection_index;
    ExactCriticalCatalogReducedGammaGroupOverlay& group_overlay =
        result.group_overlays[slot.history_group_record_index];
    if (is_birth) {
      group_overlay.birth_event_projection_indices.push_back(
          result.event_projections.back().projection_index);
      ++result.counters.birth_event_projection_count;
    } else {
      group_overlay.saddle_event_projection_indices.push_back(
          result.event_projections.back().projection_index);
      ++result.counters.saddle_event_projection_count;
    }
  }
  result.counters.event_projection_count =
      result.event_projections.size();
  if (result.counters.catalog_closed_label_point_id_scan_count >
          result.required_catalog_point_id_scan_capacity ||
      result.event_projections.size() != references.size()) {
    throw std::logic_error(
        "the actual catalogue projections violated overlay preflight");
  }
  result.closed_label_theorem_applied_to_every_projection = true;
  result.every_catalog_h0_role_projected_exactly_once = true;
}

void certify_overlay_partition(
    const ExactPersistentReducedGammaOrderHistory& history,
    OverlayResult& result) {
  bool all_slots_cross_referenced = true;
  bool exact_birth_deferred_bijection = true;
  bool saddles_non_deferred = true;
  for (const ExactCriticalCatalogReducedGammaHistoryLabelSlot& slot :
       result.history_label_slots) {
    if (slot.history_group_record_index >= history.group_records.size()) {
      throw std::logic_error(
          "an overlay slot references no Gamma history group");
    }
    const ExactPersistentReducedGammaHistoryGroupRecord& group =
        history.group_records[slot.history_group_record_index];
    const bool deferred = group.kind ==
        ExactReducedGammaBatchGroupKind::deferred_isolated_facet;
    if (!slot.event_projection_index.has_value()) {
      if (slot.kind == LabelKind::newly_active_facet) {
        ++result.counters.residual_newly_active_facet_slot_count;
      } else {
        ++result.counters.residual_equal_level_coface_slot_count;
      }
      if (slot.kind == LabelKind::newly_active_facet && deferred) {
        exact_birth_deferred_bijection = false;
      }
      continue;
    }
    const std::size_t projection_index = *slot.event_projection_index;
    if (projection_index >= result.event_projections.size()) {
      all_slots_cross_referenced = false;
      continue;
    }
    const ExactCriticalCatalogReducedGammaEventProjection& projection =
        result.event_projections[projection_index];
    all_slots_cross_referenced =
        all_slots_cross_referenced &&
        projection.projection_index == projection_index &&
        projection.history_label_slot_index == slot.label_slot_index &&
        projection.history_group_record_index ==
            slot.history_group_record_index &&
        projection.history_batch_index == slot.history_batch_index &&
        projection.history_label_kind == slot.kind &&
        projection.history_group_local_label_index ==
            slot.history_group_local_label_index;
    if (slot.kind == LabelKind::newly_active_facet) {
      exact_birth_deferred_bijection =
          exact_birth_deferred_bijection && deferred &&
          projection.role == EventRole::birth;
    } else {
      saddles_non_deferred =
          saddles_non_deferred && !deferred &&
          projection.role == EventRole::saddle;
    }
  }
  for (std::size_t group_overlay_index = 0U;
       group_overlay_index < result.group_overlays.size();
       ++group_overlay_index) {
    ExactCriticalCatalogReducedGammaGroupOverlay& group_overlay =
        result.group_overlays[group_overlay_index];
    if (group_overlay.history_group_record_index >=
            history.group_records.size() ||
        group_overlay.history_group_record_index !=
            group_overlay_index) {
      throw std::logic_error(
          "a group overlay lost the canonical Gamma group order");
    }
    const bool has_provenance =
        !group_overlay.birth_event_projection_indices.empty() ||
        !group_overlay.saddle_event_projection_indices.empty();
    group_overlay.has_catalog_h0_provenance = has_provenance;
    const std::size_t group_reference_count = checked_add(
        group_overlay.birth_event_projection_indices.size(),
        group_overlay.saddle_event_projection_indices.size(),
        "the overlay group event-reference count overflows");
    result.counters.group_event_reference_count = checked_add(
        result.counters.group_event_reference_count,
        group_reference_count,
        "the total overlay group event-reference count overflows");
    if (has_provenance) {
      ++result.counters.group_with_catalog_provenance_count;
    } else {
      ++result.counters.group_without_catalog_provenance_count;
    }
  }
  result.every_history_label_slot_partitioned_by_provenance =
      all_slots_cross_referenced &&
      result.counters.residual_newly_active_facet_slot_count +
              result.counters.residual_equal_level_coface_slot_count +
              result.counters.event_projection_count ==
          result.history_label_slots.size();
  result.catalog_births_exactly_deferred_newly_active_facets =
      exact_birth_deferred_bijection;
  result.catalog_saddles_only_non_deferred_groups =
      saddles_non_deferred;
  result.group_kinds_inherited_only_from_history = true;
  result.simultaneous_history_batches_preserved =
      result.group_overlays.size() == history.group_records.size();
  result.
      birth_projection_plus_residual_facet_count_equals_exhaustive_facet_count =
      result.counters.birth_event_projection_count +
              result.counters.residual_newly_active_facet_slot_count ==
          result.exhaustive_facet_count;
  result.
      saddle_projection_plus_residual_coface_count_equals_exhaustive_coface_count =
      result.counters.saddle_event_projection_count +
              result.counters.residual_equal_level_coface_slot_count ==
          result.exhaustive_coface_count;
  if (result.counters.group_event_reference_count !=
          result.event_projections.size() ||
      result.counters.group_event_reference_count >
          result.required_group_event_reference_capacity ||
      result.counters.group_with_catalog_provenance_count +
              result.counters.group_without_catalog_provenance_count !=
          result.group_overlays.size()) {
    throw std::logic_error(
        "the compact group-provenance references violated preflight");
  }
}

[[nodiscard]] OverlayResult
compute_exact_critical_catalog_reduced_gamma_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    OverlayBudget budget) {
  validate_domain(cloud, order, budget);
  OverlayResult result;
  result.requested_budget = budget;
  result.point_count = cloud.size();
  result.order = order;
  result.scope = ExactCriticalCatalogReducedGammaOverlayScope::
      bounded_n14_k10_single_order_freshly_verified_critical_catalog_h0_provenance_to_exhaustive_persistent_reduced_gamma_history_groups_only;
  result.counters.preflight_count = 1U;
  derive_overlay_preflight(cloud, order, result);
  result.overlay_candidate_space_size_certified = true;
  result.overlay_preflight_budget_sufficient =
      overlay_budget_covers_preflight(budget, result);
  // This implication is true on both branches: the insufficient branch
  // returns with no subordinate source, and the sufficient branch sets it
  // before invoking either geometry builder.
  result.
      subordinate_geometry_started_only_after_successful_overlay_preflight =
      true;
  if (!result.overlay_preflight_budget_sufficient) {
    result.decision =
        OverlayDecision::no_overlay_preflight_budget_insufficient;
    return result;
  }

  result.critical_catalog = build_exact_critical_catalog(
      cloud, order, budget.critical_catalog_budget);
  ++result.counters.critical_catalog_build_count;
  const ExactCriticalCatalogVerification catalog_verification =
      verify_exact_critical_catalog(
          cloud,
          order,
          budget.critical_catalog_budget,
          *result.critical_catalog);
  ++result.counters.critical_catalog_verification_count;
  if (!catalog_verification.exact_critical_catalog_decision_certified) {
    throw std::logic_error(
        "the subordinate critical catalogue failed its fresh verifier");
  }
  result.critical_catalog_fresh_replay_certified = true;
  if (result.critical_catalog->decision ==
      ExactCriticalCatalogDecision::
          no_catalog_preflight_budget_insufficient) {
    result.decision =
        OverlayDecision::no_catalog_preflight_budget_insufficient;
    return result;
  }
  if (result.critical_catalog->decision ==
      ExactCriticalCatalogDecision::
          complete_catalog_with_relevant_extra_shell_degeneracy) {
    result.decision =
        OverlayDecision::no_overlay_relevant_extra_shell_degeneracy;
    return result;
  }
  if (result.critical_catalog->decision !=
          ExactCriticalCatalogDecision::
              complete_supported_critical_catalog ||
      !result.critical_catalog->no_relevant_extra_shell_degeneracy) {
    throw std::logic_error(
        "the fresh critical catalogue has no supported complete decision");
  }
  result.no_relevant_extra_shell_degeneracy = true;

  result.reduced_gamma_history =
      build_exact_persistent_reduced_gamma_order_history(
          cloud, order, budget.reduced_gamma_history_budget);
  ++result.counters.reduced_gamma_history_build_count;
  const ExactPersistentReducedGammaOrderHistoryVerification
      history_verification =
          verify_exact_persistent_reduced_gamma_order_history(
              cloud,
              order,
              budget.reduced_gamma_history_budget,
              *result.reduced_gamma_history);
  ++result.counters.reduced_gamma_history_verification_count;
  if (!history_verification.
          exact_persistent_reduced_gamma_order_history_decision_certified) {
    throw std::logic_error(
        "the subordinate reduced-Gamma history failed its fresh verifier");
  }
  result.reduced_gamma_history_fresh_replay_certified = true;
  if (result.reduced_gamma_history->decision ==
      ExactPersistentReducedGammaOrderHistoryDecision::
          no_history_preflight_budget_insufficient) {
    result.decision =
        OverlayDecision::no_history_preflight_budget_insufficient;
    return result;
  }
  if (result.reduced_gamma_history->decision !=
          ExactPersistentReducedGammaOrderHistoryDecision::
              complete_persistent_reduced_gamma_history ||
      !result.reduced_gamma_history->
          persistent_reduced_gamma_history_certified ||
      result.reduced_gamma_history->exhaustive_facet_count !=
          result.exhaustive_facet_count ||
      result.reduced_gamma_history->exhaustive_coface_count !=
          result.exhaustive_coface_count) {
    throw std::logic_error(
        "the fresh reduced-Gamma history has no supported complete "
        "decision");
  }

  std::map<HistoryLabelKey, std::size_t, HistoryLabelKeyLess>
      slot_by_key;
  index_history_equality_slots(
      *result.reduced_gamma_history, result, slot_by_key);
  const std::vector<CatalogRoleReference> references =
      selected_catalog_role_references(
          *result.critical_catalog, order, result);
  project_catalog_roles(
      *result.critical_catalog,
      *result.reduced_gamma_history,
      references,
      slot_by_key,
      result);
  certify_overlay_partition(*result.reduced_gamma_history, result);

  result.critical_catalog_reduced_gamma_overlay_certified =
      result.overlay_candidate_space_size_certified &&
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
          saddle_projection_plus_residual_coface_count_equals_exhaustive_coface_count;
  if (!result.critical_catalog_reduced_gamma_overlay_certified) {
    throw std::logic_error(
        "the exhaustive critical-catalog reduced-Gamma overlay failed "
        "certification");
  }
  result.decision = OverlayDecision::
      complete_exhaustive_critical_catalog_reduced_gamma_overlay;
  return result;
}

}  // namespace

ExactCriticalCatalogReducedGammaOverlayVerification
verify_exact_critical_catalog_reduced_gamma_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    OverlayBudget budget,
    const OverlayResult& result) {
  const OverlayResult expected =
      compute_exact_critical_catalog_reduced_gamma_overlay(
          cloud, order, budget);
  ExactCriticalCatalogReducedGammaOverlayVerification verification;
  verification.requested_budget_certified =
      result.requested_budget == budget &&
      result.requested_budget == expected.requested_budget;
  verification.external_inputs_certified =
      result.point_count == cloud.size() &&
      result.point_count == expected.point_count &&
      result.order == order && result.order == expected.order;
  verification.derived_preflight_sizes_certified =
      result.exhaustive_facet_count == expected.exhaustive_facet_count &&
      result.exhaustive_coface_count == expected.exhaustive_coface_count &&
      result.required_event_projection_capacity ==
          expected.required_event_projection_capacity &&
      result.required_group_overlay_capacity ==
          expected.required_group_overlay_capacity &&
      result.required_label_slot_capacity ==
          expected.required_label_slot_capacity &&
      result.required_history_point_id_scan_capacity ==
          expected.required_history_point_id_scan_capacity &&
      result.required_catalog_point_id_scan_capacity ==
          expected.required_catalog_point_id_scan_capacity &&
      result.required_group_event_reference_capacity ==
          expected.required_group_event_reference_capacity &&
      result.overlay_candidate_space_size_certified ==
          expected.overlay_candidate_space_size_certified;
  verification.critical_catalog_certified =
      result.critical_catalog == expected.critical_catalog;
  verification.reduced_gamma_history_certified =
      result.reduced_gamma_history == expected.reduced_gamma_history;
  verification.event_projections_certified =
      result.event_projections == expected.event_projections;
  verification.history_label_slots_certified =
      result.history_label_slots == expected.history_label_slots;
  verification.group_overlays_certified =
      result.group_overlays == expected.group_overlays;
  verification.result_facts_certified =
      result_facts_match(result, expected);
  verification.counters_certified =
      result.counters == expected.counters;
  verification.decision_certified =
      result.decision == expected.decision;
  verification.scope_certified =
      result.scope ==
          ExactCriticalCatalogReducedGammaOverlayScope::
              bounded_n14_k10_single_order_freshly_verified_critical_catalog_h0_provenance_to_exhaustive_persistent_reduced_gamma_history_groups_only &&
      result.scope == expected.scope;
  verification.fresh_replay_certified = result == expected;
  verification.
      exact_critical_catalog_reduced_gamma_overlay_decision_certified =
      verification.requested_budget_certified &&
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
      verification.fresh_replay_certified;
  return verification;
}

ExactCriticalCatalogReducedGammaOverlayResult
build_exact_critical_catalog_reduced_gamma_overlay(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    OverlayBudget budget) {
  OverlayResult result =
      compute_exact_critical_catalog_reduced_gamma_overlay(
          cloud, order, budget);
  const ExactCriticalCatalogReducedGammaOverlayVerification verification =
      verify_exact_critical_catalog_reduced_gamma_overlay(
          cloud, order, budget, result);
  if (!verification.
          exact_critical_catalog_reduced_gamma_overlay_decision_certified) {
    throw std::logic_error(
        "the exact critical-catalog reduced-Gamma overlay failed its "
        "fresh replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
