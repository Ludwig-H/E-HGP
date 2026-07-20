#include "morsehgp3d/hierarchy/critical_catalog_typed_gamma_durable_arm_key_catalog.hpp"

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

using KeyBudget = ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget;
using KeyCounters =
    ExactCriticalCatalogTypedGammaDurableArmKeyCatalogCounters;
using KeyDecision = ExactCriticalCatalogTypedGammaDurableArmKeyCatalogDecision;
using KeyResult = ExactCriticalCatalogTypedGammaDurableArmKeyCatalogResult;
using KeyScope = ExactCriticalCatalogTypedGammaDurableArmKeyCatalogScope;
using EventKeyRecord =
    ExactCriticalCatalogTypedGammaDurableEventKeyRecord;
using ArmKeyRecord = ExactCriticalCatalogTypedGammaDurableArmKeyRecord;
using EventProjection = ExactCriticalEventV2IdentityProjection;
using DurableArmKey = ExactCriticalArmDurableKey;
using PathBudget =
    ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget;
using PathDecision =
    ExactCriticalCatalogTypedGammaArmRootPathOverlayDecision;
using PathResult = ExactCriticalCatalogTypedGammaArmRootPathOverlayResult;
using JournalResult = ExactCriticalCatalogTypedGammaJournalResult;
using RootOverlayResult = ExactCriticalCatalogTypedGammaRootOverlayResult;
using CompositionResult =
    ExactCriticalCatalogTypedGammaArmRootCompositionResult;

static_assert(
    KeyBudget::maximum_supported_event_key_record_count == 1456U);
static_assert(
    KeyBudget::maximum_supported_arm_key_record_count ==
    PathBudget::maximum_supported_path_record_count);
static_assert(
    KeyBudget::maximum_supported_event_arm_key_reference_count ==
    KeyBudget::maximum_supported_arm_key_record_count);
static_assert(
    KeyBudget::maximum_supported_event_projection_point_id_reference_count ==
    KeyBudget::maximum_supported_event_key_record_count *
        (KeyResult::maximum_supported_order + 5U));

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
        "the durable arm-key binomial overflows");
    value /= factor;
  }
  return value;
}

void validate_domain(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    const KeyBudget& budget) {
  if (cloud.size() < KeyResult::minimum_supported_point_count ||
      cloud.size() > KeyResult::maximum_supported_point_count ||
      order < KeyResult::minimum_supported_order ||
      order > KeyResult::maximum_supported_order || order >= cloud.size()) {
    throw std::invalid_argument(
        "the durable arm-key catalog requires 3<=n<=14, 2<=k<n and k<=10");
  }
  validate_exact_critical_catalog_typed_gamma_durable_arm_key_catalog_budget_caps(
      budget);
}

void derive_preflight(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    KeyResult& result) {
  const std::size_t maximum_support_size =
      std::min({std::size_t{4U}, order + 1U, cloud.size()});
  for (std::size_t support_size = 2U;
       support_size <= maximum_support_size;
       ++support_size) {
    result.critical_event_support_bound = checked_add(
        result.critical_event_support_bound,
        bounded_binomial(cloud.size(), support_size),
        "the durable arm-key event bound overflows");
  }
  result.critical_arm_bound = checked_multiply(
      4U,
      result.critical_event_support_bound,
      "the durable arm-key arm bound overflows");
  result.required_event_key_record_capacity =
      result.critical_event_support_bound;
  result.required_arm_key_record_capacity = result.critical_arm_bound;
  result.required_event_arm_key_reference_capacity =
      result.critical_arm_bound;
  result.required_event_projection_point_id_reference_capacity =
      checked_multiply(
          result.critical_event_support_bound,
          order + 5U,
          "the durable event-projection PointId bound overflows");
  if (result.required_event_key_record_capacity >
          KeyBudget::maximum_supported_event_key_record_count ||
      result.required_arm_key_record_capacity >
          KeyBudget::maximum_supported_arm_key_record_count ||
      result.required_event_arm_key_reference_capacity >
          KeyBudget::maximum_supported_event_arm_key_reference_count ||
      result.required_event_projection_point_id_reference_capacity >
          KeyBudget::
              maximum_supported_event_projection_point_id_reference_count) {
    throw std::logic_error(
        "the derived durable arm-key preflight exceeds its bounded caps");
  }
}

[[nodiscard]] bool budget_covers_preflight(
    const KeyBudget& budget,
    const KeyResult& result) {
  return budget.maximum_event_key_record_count >=
             result.required_event_key_record_capacity &&
         budget.maximum_arm_key_record_count >=
             result.required_arm_key_record_capacity &&
         budget.maximum_event_arm_key_reference_count >=
             result.required_event_arm_key_reference_capacity &&
         budget.maximum_event_projection_point_id_reference_count >=
             result.required_event_projection_point_id_reference_capacity;
}

[[nodiscard]] bool external_budget_seams_match(
    const KeyBudget& budget,
    const JournalResult& journal,
    const RootOverlayResult& root_overlay,
    const CompositionResult& composition,
    const PathResult& paths) {
  const PathBudget& path_budget = budget.path_overlay_budget;
  const auto& composition_budget = path_budget.arm_root_composition_budget;
  const auto& root_budget = composition_budget.root_overlay_budget;
  const auto& journal_budget = root_budget.typed_gamma_journal_budget;
  return journal.requested_budget == journal_budget &&
         root_overlay.requested_budget == root_budget &&
         composition.requested_budget == composition_budget &&
         paths.requested_budget == path_budget;
}

[[nodiscard]] std::string point_ids_json(
    const std::vector<spatial::PointId>& point_ids) {
  std::string result{"["};
  for (std::size_t index = 0U; index < point_ids.size(); ++index) {
    if (index != 0U) {
      result.push_back(',');
    }
    result.append(std::to_string(point_ids[index]));
  }
  result.push_back(']');
  return result;
}

[[nodiscard]] std::string canonical_event_projection_json(
    const EventProjection& projection) {
  const exact::ExactRational3Record center =
      projection.center_witness_homogeneous.to_record();
  const exact::ExactLevelRecord level =
      projection.squared_level_exact.to_record();
  return
      "{\"center_witness_homogeneous\":{\"denominator\":\"" +
      center.denominator + "\",\"unit\":\"" + center.unit +
      "\",\"x_numerator\":\"" + center.x_numerator +
      "\",\"y_numerator\":\"" + center.y_numerator +
      "\",\"z_numerator\":\"" + center.z_numerator +
      "\"},\"interior_ids\":" +
      point_ids_json(projection.interior_point_ids) +
      ",\"minimal_support_ids\":" +
      point_ids_json(projection.minimal_support_point_ids) +
      ",\"shell_ids\":" + point_ids_json(projection.shell_point_ids) +
      ",\"squared_level_exact\":{\"denominator\":\"" +
      level.denominator + "\",\"numerator\":\"" + level.numerator +
      "\",\"unit\":\"" + level.unit + "\"}}";
}

[[nodiscard]] EventProjection event_projection(
    const ExactCriticalEvent& event) {
  return EventProjection{
      event.interior_point_ids,
      event.shell_point_ids,
      event.support_point_ids,
      event.center,
      event.squared_level};
}

[[nodiscard]] bool result_facts_match(
    const KeyResult& observed,
    const KeyResult& expected) {
  return observed.durable_key_conservative_preflight_bounds_certified ==
             expected.durable_key_conservative_preflight_bounds_certified &&
         observed.durable_key_preflight_budget_sufficient ==
             expected.durable_key_preflight_budget_sufficient &&
         observed.all_four_external_budget_seams_certified ==
             expected.all_four_external_budget_seams_certified &&
         observed.source_path_overlay_is_external_and_not_retained ==
             expected.source_path_overlay_is_external_and_not_retained &&
         observed.source_path_overlay_fresh_replay_certified ==
             expected.source_path_overlay_fresh_replay_certified &&
         observed.
                 reconstruction_started_only_after_complete_source_path_overlay ==
             expected.
                 reconstruction_started_only_after_complete_source_path_overlay &&
         observed.transient_critical_catalog_fresh_replay_certified ==
             expected.transient_critical_catalog_fresh_replay_certified &&
         observed.
                 event_identity_projections_are_complete_schema_version_free_v2_keys ==
             expected.
                 event_identity_projections_are_complete_schema_version_free_v2_keys &&
         observed.critical_event_ids_are_domain_separated_sha256_v2 ==
             expected.critical_event_ids_are_domain_separated_sha256_v2 &&
         observed.
                 event_hash_collisions_checked_against_complete_projections ==
             expected.
                 event_hash_collisions_checked_against_complete_projections &&
         observed.every_requested_order_saddle_has_one_durable_event_key ==
             expected.every_requested_order_saddle_has_one_durable_event_key &&
         observed.
                 every_complete_shell_has_exactly_one_sorted_durable_arm_tuple_per_point ==
             expected.
                 every_complete_shell_has_exactly_one_sorted_durable_arm_tuple_per_point &&
         observed.arm_tuples_biject_replayable_source_paths ==
             expected.arm_tuples_biject_replayable_source_paths &&
         observed.event_to_arm_aggregation_is_complete_and_canonical ==
             expected.event_to_arm_aggregation_is_complete_and_canonical &&
         observed.identities_exclude_paths_targets_reduced_roots_and_local_indices ==
             expected.
                 identities_exclude_paths_targets_reduced_roots_and_local_indices &&
         observed.
                 records_are_internal_keys_and_not_public_attachments_or_equal_level_batches ==
             expected.
                 records_are_internal_keys_and_not_public_attachments_or_equal_level_batches &&
         observed.diagnostic_outcomes_have_no_key_payload ==
             expected.diagnostic_outcomes_have_no_key_payload &&
         observed.
                 critical_catalog_typed_gamma_durable_arm_key_catalog_certified ==
             expected.
                 critical_catalog_typed_gamma_durable_arm_key_catalog_certified;
}

[[nodiscard]] KeyResult compute_key_catalog(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    KeyBudget budget,
    const JournalResult& source_journal,
    const RootOverlayResult& source_root_overlay,
    const CompositionResult& source_composition,
    const PathResult& source_path_overlay) {
  validate_domain(cloud, order, budget);
  KeyResult result;
  result.requested_budget = budget;
  result.point_count = cloud.size();
  result.order = order;
  result.scope = KeyScope::
      bounded_n14_k10_single_order_v2_critical_event_ids_and_canonical_arm_identity_tuples_from_recertified_internal_replayable_paths_only;
  result.counters.preflight_count = 1U;
  result.source_path_overlay_is_external_and_not_retained = true;
  result.diagnostic_outcomes_have_no_key_payload = true;
  derive_preflight(cloud, order, result);
  result.durable_key_conservative_preflight_bounds_certified = true;
  result.all_four_external_budget_seams_certified = external_budget_seams_match(
      budget,
      source_journal,
      source_root_overlay,
      source_composition,
      source_path_overlay);
  result.durable_key_preflight_budget_sufficient =
      budget_covers_preflight(budget, result);
  if (!result.all_four_external_budget_seams_certified) {
    result.decision = KeyDecision::
        no_durable_arm_key_catalog_external_budget_seam_mismatch;
    return result;
  }
  if (!result.durable_key_preflight_budget_sufficient) {
    result.decision = KeyDecision::
        no_durable_arm_key_catalog_preflight_budget_insufficient;
    return result;
  }

  const auto source_verification =
      verify_exact_critical_catalog_typed_gamma_arm_root_path_overlay(
          cloud,
          order,
          budget.path_overlay_budget,
          source_journal,
          source_root_overlay,
          source_composition,
          source_path_overlay);
  ++result.counters.source_path_overlay_verification_count;
  if (!source_verification.
          exact_critical_catalog_typed_gamma_arm_root_path_overlay_decision_certified) {
    result.decision = KeyDecision::
        no_durable_arm_key_catalog_source_path_overlay_rejected;
    return result;
  }
  result.source_path_overlay_fresh_replay_certified = true;
  result.source_path_overlay_decision = source_path_overlay.decision;
  if (source_path_overlay.decision != PathDecision::
          complete_exhaustive_event_local_replayable_arm_root_path_overlay) {
    result.decision = KeyDecision::
        no_durable_arm_key_catalog_source_path_overlay_incomplete;
    return result;
  }
  result.reconstruction_started_only_after_complete_source_path_overlay = true;

  const auto& catalog_budget =
      budget.path_overlay_budget.arm_root_composition_budget
          .root_overlay_budget.typed_gamma_journal_budget.arm_overlay_budget
          .critical_catalog_budget;
  ExactCriticalCatalogResult catalog =
      build_exact_critical_catalog(cloud, order, catalog_budget);
  ++result.counters.critical_catalog_build_count;
  if (catalog.decision !=
          ExactCriticalCatalogDecision::complete_supported_critical_catalog ||
      !catalog.no_relevant_extra_shell_degeneracy) {
    throw std::logic_error(
        "a complete source path overlay has no complete fresh catalog");
  }
  result.transient_critical_catalog_fresh_replay_certified = true;
  if (source_journal.saddle_records.size() >
          result.required_event_key_record_capacity ||
      source_path_overlay.path_records.size() >
          result.required_arm_key_record_capacity) {
    throw std::logic_error(
        "a complete source exceeds the durable arm-key preflight");
  }

  std::vector<EventKeyRecord> pending_events;
  pending_events.reserve(source_journal.saddle_records.size());
  KeyCounters pending_counters = result.counters;
  for (std::size_t saddle_index = 0U;
       saddle_index < source_journal.saddle_records.size();
       ++saddle_index) {
    const ExactCriticalCatalogTypedGammaSaddleRecord& saddle =
        source_journal.saddle_records[saddle_index];
    if (saddle.saddle_record_index != saddle_index ||
        saddle.catalog_event_index >= catalog.events.size()) {
      throw std::logic_error(
          "a typed saddle has no dense fresh event for durable identity");
    }
    const ExactCriticalEvent& event =
        catalog.events[saddle.catalog_event_index];
    if (event.event_index != saddle.catalog_event_index ||
        !event.saddle_order.has_value() || *event.saddle_order != order ||
        event.closed_rank != order + 1U ||
        event.closed_point_ids.size() != order + 1U ||
        event.shell_point_ids.empty()) {
      throw std::logic_error(
          "a typed saddle disagrees with its durable event projection");
    }
    EventKeyRecord record;
    record.source_catalog_event_index = saddle.catalog_event_index;
    record.identity_projection = event_projection(event);
    record.event_id =
        contract::canonical_v2_id_from_canonical_json_unchecked(
            "CriticalEvent",
            canonical_event_projection_json(record.identity_projection));
    ++pending_counters.saddle_event_reconciliation_count;
    ++pending_counters.event_projection_count;
    ++pending_counters.event_id_hash_count;
    pending_counters.event_projection_point_id_reference_count = checked_add(
        pending_counters.event_projection_point_id_reference_count,
        record.identity_projection.interior_point_ids.size(),
        "the durable event interior-reference count overflows");
    pending_counters.event_projection_point_id_reference_count = checked_add(
        pending_counters.event_projection_point_id_reference_count,
        record.identity_projection.shell_point_ids.size(),
        "the durable event shell-reference count overflows");
    pending_counters.event_projection_point_id_reference_count = checked_add(
        pending_counters.event_projection_point_id_reference_count,
        record.identity_projection.minimal_support_point_ids.size(),
        "the durable event support-reference count overflows");
    pending_events.push_back(std::move(record));
  }
  std::sort(
      pending_events.begin(),
      pending_events.end(),
      [](const EventKeyRecord& left, const EventKeyRecord& right) {
        return left.event_id < right.event_id;
      });
  for (std::size_t index = 1U; index < pending_events.size(); ++index) {
    if (pending_events[index - 1U].event_id == pending_events[index].event_id) {
      ++pending_counters.event_hash_semantic_comparison_count;
      if (pending_events[index - 1U].identity_projection !=
          pending_events[index].identity_projection) {
        throw std::logic_error(
            "two distinct critical-event projections collide under SHA-256");
      }
      throw std::logic_error(
          "the fresh catalog duplicates one critical-event identity");
    }
  }

  std::vector<std::optional<std::size_t>>
      event_key_index_by_catalog_event(catalog.events.size());
  for (std::size_t index = 0U; index < pending_events.size(); ++index) {
    pending_events[index].event_key_record_index = index;
    const std::size_t catalog_event_index =
        pending_events[index].source_catalog_event_index;
    if (event_key_index_by_catalog_event[catalog_event_index].has_value()) {
      throw std::logic_error(
          "one catalog event receives two durable event-key records");
    }
    event_key_index_by_catalog_event[catalog_event_index] = index;
  }

  std::vector<ArmKeyRecord> pending_arms;
  pending_arms.reserve(source_path_overlay.path_records.size());
  for (const ExactCriticalCatalogTypedGammaEventLocalArmRootPathRecord& path :
       source_path_overlay.path_records) {
    if (path.catalog_event_index >=
            event_key_index_by_catalog_event.size() ||
        !event_key_index_by_catalog_event[path.catalog_event_index]
             .has_value() ||
        path.path_record_index >= source_path_overlay.path_records.size() ||
        path.order != order) {
      throw std::logic_error(
          "a replayable path has no durable event-key join");
    }
    const std::size_t event_key_index =
        *event_key_index_by_catalog_event[path.catalog_event_index];
    const EventKeyRecord& event_record = pending_events[event_key_index];
    if (path.critical_center !=
            event_record.identity_projection.center_witness_homogeneous ||
        path.critical_squared_level !=
            event_record.identity_projection.squared_level_exact ||
        !std::binary_search(
            event_record.identity_projection.shell_point_ids.begin(),
            event_record.identity_projection.shell_point_ids.end(),
            path.removed_shell_point_id)) {
      throw std::logic_error(
          "a replayable path disagrees with its durable event-local key");
    }
    ArmKeyRecord record;
    record.durable_key = DurableArmKey{
        event_record.event_id, order, path.removed_shell_point_id};
    record.event_key_record_index = event_key_index;
    record.source_path_record_index = path.path_record_index;
    pending_arms.push_back(std::move(record));
    ++pending_counters.arm_path_key_reconciliation_count;
  }
  std::sort(
      pending_arms.begin(),
      pending_arms.end(),
      [](const ArmKeyRecord& left, const ArmKeyRecord& right) {
        return left.durable_key < right.durable_key;
      });
  for (std::size_t index = 0U; index < pending_arms.size(); ++index) {
    if (index != 0U &&
        pending_arms[index - 1U].durable_key ==
            pending_arms[index].durable_key) {
      throw std::logic_error(
          "one durable event-order-removed-shell tuple appears twice");
    }
    pending_arms[index].arm_key_record_index = index;
    const std::size_t event_index =
        pending_arms[index].event_key_record_index;
    if (event_index >= pending_events.size() ||
        pending_arms[index].durable_key.event_id !=
            pending_events[event_index].event_id) {
      throw std::logic_error(
          "a sorted durable arm tuple lost its event-key join");
    }
    pending_events[event_index].arm_key_record_indices.push_back(index);
  }

  for (const EventKeyRecord& event_record : pending_events) {
    std::vector<spatial::PointId> observed_removed_points;
    observed_removed_points.reserve(event_record.arm_key_record_indices.size());
    for (const std::size_t arm_index : event_record.arm_key_record_indices) {
      if (arm_index >= pending_arms.size() ||
          pending_arms[arm_index].event_key_record_index !=
              event_record.event_key_record_index) {
        throw std::logic_error(
            "an event-to-arm durable aggregation reference is invalid");
      }
      observed_removed_points.push_back(
          pending_arms[arm_index].durable_key.removed_shell_point_id);
    }
    if (observed_removed_points !=
        event_record.identity_projection.shell_point_ids) {
      throw std::logic_error(
          "durable arm tuples do not exhaust exactly one complete shell");
    }
  }

  pending_counters.event_key_record_count = pending_events.size();
  pending_counters.arm_key_record_count = pending_arms.size();
  pending_counters.event_arm_key_reference_count = pending_arms.size();
  if (pending_counters.event_key_record_count >
          budget.maximum_event_key_record_count ||
      pending_counters.arm_key_record_count >
          budget.maximum_arm_key_record_count ||
      pending_counters.event_arm_key_reference_count >
          budget.maximum_event_arm_key_reference_count ||
      pending_counters.event_projection_point_id_reference_count >
          budget.maximum_event_projection_point_id_reference_count ||
      pending_counters.event_key_record_count !=
          source_journal.saddle_records.size() ||
      pending_counters.arm_key_record_count !=
          source_path_overlay.path_records.size() ||
      pending_counters.event_arm_key_reference_count !=
          source_path_overlay.path_records.size()) {
    throw std::logic_error(
        "the durable arm-key payload is incomplete or exceeds preflight");
  }

  result.event_key_records = std::move(pending_events);
  result.arm_key_records = std::move(pending_arms);
  result.counters = pending_counters;
  result.event_identity_projections_are_complete_schema_version_free_v2_keys =
      true;
  result.critical_event_ids_are_domain_separated_sha256_v2 = true;
  result.event_hash_collisions_checked_against_complete_projections = true;
  result.every_requested_order_saddle_has_one_durable_event_key = true;
  result.
      every_complete_shell_has_exactly_one_sorted_durable_arm_tuple_per_point =
      true;
  result.arm_tuples_biject_replayable_source_paths = true;
  result.event_to_arm_aggregation_is_complete_and_canonical = true;
  result.identities_exclude_paths_targets_reduced_roots_and_local_indices = true;
  result.
      records_are_internal_keys_and_not_public_attachments_or_equal_level_batches =
      true;
  result.critical_catalog_typed_gamma_durable_arm_key_catalog_certified =
      result.durable_key_conservative_preflight_bounds_certified &&
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
      result.diagnostic_outcomes_have_no_key_payload;
  if (!result.
          critical_catalog_typed_gamma_durable_arm_key_catalog_certified) {
    throw std::logic_error(
        "the durable arm-key catalog failed certification");
  }
  result.decision =
      KeyDecision::complete_exhaustive_single_order_durable_arm_key_catalog;
  return result;
}

}  // namespace

void validate_exact_critical_catalog_typed_gamma_durable_arm_key_catalog_budget_caps(
    const KeyBudget& budget) {
  validate_exact_critical_catalog_typed_gamma_arm_root_path_overlay_budget_caps(
      budget.path_overlay_budget);
  if (budget.maximum_event_key_record_count >
          KeyBudget::maximum_supported_event_key_record_count ||
      budget.maximum_arm_key_record_count >
          KeyBudget::maximum_supported_arm_key_record_count ||
      budget.maximum_event_arm_key_reference_count >
          KeyBudget::maximum_supported_event_arm_key_reference_count ||
      budget.maximum_event_projection_point_id_reference_count >
          KeyBudget::
              maximum_supported_event_projection_point_id_reference_count) {
    throw std::invalid_argument(
        "a durable arm-key catalog capacity exceeds its bounded cap");
  }
}

ExactCriticalCatalogTypedGammaDurableArmKeyCatalogVerification
verify_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    KeyBudget budget,
    const JournalResult& source_journal,
    const RootOverlayResult& source_root_overlay,
    const CompositionResult& source_composition,
    const PathResult& source_path_overlay,
    const KeyResult& result) {
  const KeyResult expected = compute_key_catalog(
      cloud,
      order,
      budget,
      source_journal,
      source_root_overlay,
      source_composition,
      source_path_overlay);
  ExactCriticalCatalogTypedGammaDurableArmKeyCatalogVerification verification;
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
      result.required_event_key_record_capacity ==
          expected.required_event_key_record_capacity &&
      result.required_arm_key_record_capacity ==
          expected.required_arm_key_record_capacity &&
      result.required_event_arm_key_reference_capacity ==
          expected.required_event_arm_key_reference_capacity &&
      result.required_event_projection_point_id_reference_capacity ==
          expected.required_event_projection_point_id_reference_capacity &&
      result.durable_key_conservative_preflight_bounds_certified ==
          expected.durable_key_conservative_preflight_bounds_certified;
  verification.source_path_overlay_decision_certified =
      result.source_path_overlay_decision ==
      expected.source_path_overlay_decision;
  verification.source_path_overlay_fresh_replay_certified =
      result.source_path_overlay_fresh_replay_certified ==
      expected.source_path_overlay_fresh_replay_certified;
  verification.event_key_records_certified =
      result.event_key_records == expected.event_key_records;
  verification.arm_key_records_certified =
      result.arm_key_records == expected.arm_key_records;
  verification.result_facts_certified = result_facts_match(result, expected);
  verification.counters_certified = result.counters == expected.counters;
  verification.decision_certified = result.decision == expected.decision;
  verification.scope_certified =
      result.scope == KeyScope::
          bounded_n14_k10_single_order_v2_critical_event_ids_and_canonical_arm_identity_tuples_from_recertified_internal_replayable_paths_only &&
      result.scope == expected.scope;
  verification.fresh_replay_certified =
      verification.requested_budget_certified &&
      verification.external_inputs_certified &&
      verification.derived_preflight_sizes_certified &&
      verification.source_path_overlay_decision_certified &&
      verification.source_path_overlay_fresh_replay_certified &&
      verification.event_key_records_certified &&
      verification.arm_key_records_certified &&
      verification.result_facts_certified && verification.counters_certified &&
      verification.decision_certified && verification.scope_certified;
  verification.
      exact_critical_catalog_typed_gamma_durable_arm_key_catalog_decision_certified =
      verification.fresh_replay_certified;
  return verification;
}

ExactCriticalCatalogTypedGammaDurableArmKeyCatalogResult
build_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    KeyBudget budget,
    const JournalResult& source_journal,
    const RootOverlayResult& source_root_overlay,
    const CompositionResult& source_composition,
    const PathResult& source_path_overlay) {
  KeyResult result = compute_key_catalog(
      cloud,
      order,
      budget,
      source_journal,
      source_root_overlay,
      source_composition,
      source_path_overlay);
  const auto verification =
      verify_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
          cloud,
          order,
          budget,
          source_journal,
          source_root_overlay,
          source_composition,
          source_path_overlay,
          result);
  if (!verification.
          exact_critical_catalog_typed_gamma_durable_arm_key_catalog_decision_certified) {
    throw std::logic_error(
        "the durable arm-key catalog failed its fresh replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
