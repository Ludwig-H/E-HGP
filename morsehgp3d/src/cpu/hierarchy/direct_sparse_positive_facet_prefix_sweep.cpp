#include "morsehgp3d/hierarchy/direct_sparse_positive_facet_prefix_sweep.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

enum class SweepFailure : std::uint8_t {
  capacity_overflow,
  budget_exhausted,
  input_shape_rejected,
  locator_history_not_certified,
  probe_budget_exhausted,
};

enum class PrefixProbeStatus : std::uint8_t {
  latent_unresolved,
  relative_positive,
  budget_exhausted,
  locator_history_not_certified,
};

struct PrefixProbeResult {
  PrefixProbeStatus status{PrefixProbeStatus::locator_history_not_certified};
  ExactDirectSparseComponentHandle component_handle{};
  ExactDirectSparseFacetWitness source_binding_witness{};
  std::size_t slot_visit_count{};
  std::size_t query_parent_hop_count{};
  std::size_t full_key_comparison_count{};
  std::size_t equal_fingerprint_distinct_key_count{};
  bool future_binding_terminator{false};
  bool slot_visit_budget_exhausted{false};
  bool query_parent_hop_budget_exhausted{false};
};

[[nodiscard]] bool try_add_size(
    std::size_t left,
    std::size_t right,
    std::size_t& sum) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  sum = left + right;
  return true;
}

[[nodiscard]] bool try_multiply_size(
    std::size_t left,
    std::size_t right,
    std::size_t& product) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  product = left * right;
  return true;
}

[[nodiscard]] bool checked_accumulate(
    std::size_t increment,
    std::size_t& counter) noexcept {
  std::size_t updated = 0U;
  if (!try_add_size(counter, increment, updated)) {
    return false;
  }
  counter = updated;
  return true;
}

[[nodiscard]] bool canonical_facet_key(
    const ExactDirectSparseFacetKey& key) noexcept {
  if (key.point_count == 0U ||
      key.point_count > direct_sparse_positive_facet_maximum_point_count) {
    return false;
  }
  for (std::size_t point_index = 0U;
       point_index < key.point_count;
       ++point_index) {
    if (point_index != 0U &&
        key.point_ids[point_index - 1U] >= key.point_ids[point_index]) {
      return false;
    }
  }
  for (std::size_t point_index = key.point_count;
       point_index < key.point_ids.size();
       ++point_index) {
    if (key.point_ids[point_index] != 0U) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool witness_matches_locator(
    const ExactDirectSparseFacetWitness& witness,
    const ExactDirectSparsePositiveFacetLocator& locator) noexcept {
  return witness.external_authority_id != 0U &&
         witness.external_authority_id ==
             locator.config().external_authority_id &&
         witness.replay_token != 0U;
}

[[nodiscard]] bool source_witness_well_formed(
    const ExactDirectSparseFacetWitness& witness,
    std::uint64_t external_authority_id) noexcept {
  return external_authority_id != 0U &&
         witness.external_authority_id == external_authority_id &&
         witness.replay_token != 0U;
}

[[nodiscard]] bool complete_key_matches_arena(
    const ExactDirectSparsePositiveFacetSlot& slot,
    const ExactDirectSparseFacetKey& key,
    std::span<const spatial::PointId> key_point_arena) noexcept {
  if (slot.key_point_count != key.point_count ||
      slot.key_point_offset > key_point_arena.size() ||
      slot.key_point_count >
          key_point_arena.size() - slot.key_point_offset) {
    return false;
  }
  for (std::size_t point_index = 0U;
       point_index < key.point_count;
       ++point_index) {
    if (key_point_arena[slot.key_point_offset + point_index] !=
        key.point_ids[point_index]) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool non_scope_is_honest(
    const ExactDirectSparsePositiveFacetPrefixSweepResult& result) noexcept {
  return !result.locator_state_mutated &&
         !result.locator_batch_committed &&
         !result.external_binding_authority_replayed &&
         !result.source_batch_alignment_claimed &&
         !result.missing_facet_means_isolated &&
         !result.singleton_component_created &&
         !result.quotient_root_union_or_forest_mutated &&
         !result.gateway_attach_published &&
         !result.gamma_cells_or_higher_order_delaunay_materialized &&
         !result.forbidden_global_structure_materialized &&
         !result.public_status_claimed && result.partial_refinement_only &&
         result.scope ==
             ExactDirectSparsePositiveFacetPrefixSweepScope::
                 locator_internal_committed_batch_prefixes_relative_to_frozen_positive_domain_only;
}

void initialize_scope(
    ExactDirectSparsePositiveFacetPrefixSweepResult& result) noexcept {
  result.scope = ExactDirectSparsePositiveFacetPrefixSweepScope::
      locator_internal_committed_batch_prefixes_relative_to_frozen_positive_domain_only;
  result.no_partial_scientific_payload_published = true;
  result.locator_state_mutated = false;
  result.locator_batch_committed = false;
  result.external_binding_authority_replayed = false;
  result.source_batch_alignment_claimed = false;
  result.missing_facet_means_isolated = false;
  result.singleton_component_created = false;
  result.quotient_root_union_or_forest_mutated = false;
  result.gateway_attach_published = false;
  result.gamma_cells_or_higher_order_delaunay_materialized = false;
  result.forbidden_global_structure_materialized = false;
  result.public_status_claimed = false;
  result.partial_refinement_only = true;
}

void clear_scientific_payload(
    ExactDirectSparsePositiveFacetPrefixSweepResult& result) noexcept {
  result.resolutions.clear();
  result.logical_output_entry_count = 0U;
  result.no_partial_scientific_payload_published = true;
}

[[nodiscard]] ExactDirectSparsePositiveFacetPrefixSweepDecision decision_for(
    SweepFailure failure) noexcept {
  switch (failure) {
    case SweepFailure::capacity_overflow:
      return ExactDirectSparsePositiveFacetPrefixSweepDecision::
          no_prefix_sweep_capacity_overflow;
    case SweepFailure::budget_exhausted:
      return ExactDirectSparsePositiveFacetPrefixSweepDecision::
          no_prefix_sweep_budget_exhausted;
    case SweepFailure::input_shape_rejected:
      return ExactDirectSparsePositiveFacetPrefixSweepDecision::
          no_prefix_sweep_input_shape_rejected;
    case SweepFailure::locator_history_not_certified:
      return ExactDirectSparsePositiveFacetPrefixSweepDecision::
          no_prefix_sweep_locator_history_not_certified;
    case SweepFailure::probe_budget_exhausted:
      return ExactDirectSparsePositiveFacetPrefixSweepDecision::
          no_prefix_sweep_probe_budget_exhausted;
  }
  return ExactDirectSparsePositiveFacetPrefixSweepDecision::not_certified;
}

void check_locator_snapshot(
    const ExactDirectSparsePositiveFacetLocator& locator,
    ExactDirectSparsePositiveFacetPrefixSweepResult& result) {
  if (!checked_accumulate(
          1U, result.counters.locator_snapshot_check_count)) {
    throw std::overflow_error(
        "a positive-facet prefix sweep snapshot counter overflowed");
  }
  if (locator.snapshot_stamp() != result.locator_snapshot_stamp) {
    clear_scientific_payload(result);
    throw std::runtime_error(
        "the positive-facet locator changed during a prefix sweep");
  }
}

[[nodiscard]] ExactDirectSparsePositiveFacetPrefixSweepResult fail(
    ExactDirectSparsePositiveFacetPrefixSweepResult result,
    SweepFailure failure,
    const ExactDirectSparsePositiveFacetLocator& locator) {
  clear_scientific_payload(result);
  result.decision = decision_for(failure);
  check_locator_snapshot(locator, result);
  result.common_frozen_locator_snapshot_certified = true;
  return result;
}

[[nodiscard]] bool batch_record_shape_is_usable(
    const ExactDirectSparseCommittedBatchRecord& record,
    std::size_t expected_batch_index) noexcept {
  std::size_t lookup_partition = 0U;
  std::size_t binding_partition = 0U;
  return record.committed_batch_index == expected_batch_index &&
         try_add_size(
             record.counters.positive_lookup_count,
             record.counters.unresolved_lookup_count,
             lookup_partition) &&
         lookup_partition == record.counters.query_count &&
         try_add_size(
             record.counters.inserted_binding_count,
             record.counters.compatible_duplicate_binding_count,
             binding_partition) &&
         binding_partition == record.counters.binding_request_count &&
         record.counters.inserted_key_point_count <=
             record.counters.batch_input_key_point_count &&
         record.counters.equal_fingerprint_distinct_key_count <=
             record.counters.full_key_comparison_count &&
         record.input_shape_certified &&
         record.input_witness_structure_certified &&
         record.strict_pre_batch_snapshot_certified &&
         record.sequential_atomic_commit_certified;
}

enum class RootFindStatus : std::uint8_t {
  complete,
  budget_exhausted,
  invalid_history,
};

[[nodiscard]] RootFindStatus find_root_with_budget(
    std::span<const ExactDirectSparseComponentHandle> parents,
    ExactDirectSparseComponentHandle start,
    std::size_t maximum_parent_hop_count,
    std::size_t& parent_hop_count,
    ExactDirectSparseComponentHandle& root) noexcept {
  if (start >= parents.size()) {
    return RootFindStatus::invalid_history;
  }
  ExactDirectSparseComponentHandle current = start;
  while (parents[current] != current) {
    if (parents[current] >= parents.size()) {
      return RootFindStatus::invalid_history;
    }
    if (parent_hop_count >= maximum_parent_hop_count) {
      return RootFindStatus::budget_exhausted;
    }
    current = parents[current];
    ++parent_hop_count;
  }
  root = current;
  return RootFindStatus::complete;
}

enum class UnionReplayStatus : std::uint8_t {
  complete,
  budget_exhausted,
  invalid_history,
};

[[nodiscard]] UnionReplayStatus replay_union(
    std::vector<ExactDirectSparseComponentHandle>& parents,
    ExactDirectSparseComponentHandle left,
    ExactDirectSparseComponentHandle right,
    std::size_t maximum_parent_hop_count,
    std::size_t& parent_hop_count) noexcept {
  if (left >= parents.size() || right >= parents.size()) {
    return UnionReplayStatus::invalid_history;
  }
  ExactDirectSparseComponentHandle left_root = 0U;
  ExactDirectSparseComponentHandle right_root = 0U;
  const RootFindStatus left_status = find_root_with_budget(
      parents,
      left,
      maximum_parent_hop_count,
      parent_hop_count,
      left_root);
  if (left_status != RootFindStatus::complete) {
    return left_status == RootFindStatus::budget_exhausted
               ? UnionReplayStatus::budget_exhausted
               : UnionReplayStatus::invalid_history;
  }
  const RootFindStatus right_status = find_root_with_budget(
      parents,
      right,
      maximum_parent_hop_count,
      parent_hop_count,
      right_root);
  if (right_status != RootFindStatus::complete) {
    return right_status == RootFindStatus::budget_exhausted
               ? UnionReplayStatus::budget_exhausted
               : UnionReplayStatus::invalid_history;
  }
  if (left_root != right_root) {
    parents[std::max(left_root, right_root)] =
        std::min(left_root, right_root);
  }
  return UnionReplayStatus::complete;
}

[[nodiscard]] PrefixProbeResult probe_prefix(
    const ExactDirectSparseFacetKey& key,
    std::size_t active_binding_prefix_count,
    std::span<const ExactDirectSparseComponentHandle> parents,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparsePositiveFacetProbeBudget& budget) noexcept {
  PrefixProbeResult result;
  const auto& slots = locator.slots();
  const auto& key_point_arena = locator.key_point_arena();
  if (slots.empty()) {
    return result;
  }
  const std::uint64_t fingerprint =
      fingerprint_exact_direct_sparse_facet_key(
          key, locator.config().fingerprint_mask);
  std::size_t slot_index =
      static_cast<std::size_t>(fingerprint % slots.size());
  for (std::size_t probe_index = 0U;
       probe_index < slots.size();
       ++probe_index) {
    if (result.slot_visit_count >= budget.maximum_slot_visit_count) {
      result.status = PrefixProbeStatus::budget_exhausted;
      result.slot_visit_budget_exhausted = true;
      return result;
    }
    const ExactDirectSparsePositiveFacetSlot& slot = slots[slot_index];
    ++result.slot_visit_count;
    if (!slot.occupied ||
        slot.committed_binding_index >= active_binding_prefix_count) {
      result.status = PrefixProbeStatus::latent_unresolved;
      result.future_binding_terminator = slot.occupied;
      return result;
    }
    if (slot.component_handle >= parents.size() ||
        slot.committed_binding_index >=
            locator.counters().inserted_binding_count ||
        slot.key_point_offset > key_point_arena.size() ||
        slot.key_point_count >
            key_point_arena.size() - slot.key_point_offset ||
        !source_witness_well_formed(
            slot.binding_witness,
            locator.config().external_authority_id)) {
      return result;
    }
    if (slot.fingerprint == fingerprint) {
      ++result.full_key_comparison_count;
      if (complete_key_matches_arena(slot, key, key_point_arena)) {
        ExactDirectSparseComponentHandle root = 0U;
        const RootFindStatus root_status = find_root_with_budget(
            parents,
            slot.component_handle,
            budget.maximum_component_parent_hop_count,
            result.query_parent_hop_count,
            root);
        if (root_status != RootFindStatus::complete) {
          if (root_status == RootFindStatus::budget_exhausted) {
            result.status = PrefixProbeStatus::budget_exhausted;
            result.query_parent_hop_budget_exhausted = true;
          }
          return result;
        }
        result.status = PrefixProbeStatus::relative_positive;
        result.component_handle = root;
        result.source_binding_witness = slot.binding_witness;
        return result;
      }
      ++result.equal_fingerprint_distinct_key_count;
    }
    slot_index = (slot_index + 1U) % slots.size();
  }
  result.status = PrefixProbeStatus::latent_unresolved;
  return result;
}

[[nodiscard]] bool add_probe_counters(
    const PrefixProbeResult& probe,
    ExactDirectSparsePositiveFacetPrefixSweepCounters& counters) noexcept {
  auto updated = counters;
  if (!checked_accumulate(
          probe.slot_visit_count, updated.slot_visit_count) ||
      !checked_accumulate(
          probe.query_parent_hop_count,
          updated.query_parent_hop_count) ||
      !checked_accumulate(
          probe.full_key_comparison_count,
          updated.full_key_comparison_count) ||
      !checked_accumulate(
          probe.equal_fingerprint_distinct_key_count,
          updated.equal_fingerprint_distinct_key_count) ||
      (probe.future_binding_terminator &&
       !checked_accumulate(
           1U, updated.future_binding_terminator_count))) {
    return false;
  }
  updated.maximum_single_query_slot_visit_count = std::max(
      updated.maximum_single_query_slot_visit_count,
      probe.slot_visit_count);
  updated.maximum_single_query_parent_hop_count = std::max(
      updated.maximum_single_query_parent_hop_count,
      probe.query_parent_hop_count);
  counters = updated;
  return true;
}

[[nodiscard]] bool complete_payload_shape(
    const ExactDirectSparsePositiveFacetPrefixSweepResult& result) noexcept {
  std::size_t expected_snapshot_check_count = 0U;
  if (!try_add_size(
          result.required_committed_batch_prefix_count,
          result.source_query_count,
          expected_snapshot_check_count) ||
      !try_add_size(
          expected_snapshot_check_count,
          2U,
          expected_snapshot_check_count)) {
    return false;
  }
  std::size_t outcome_count = 0U;
  if (!try_add_size(
          result.counters.relative_positive_count,
          result.counters.latent_unresolved_count,
          outcome_count) ||
      result.resolutions.size() != result.source_query_count ||
      result.logical_output_entry_count != result.source_query_count ||
      result.counters.query_scan_count != result.source_query_count ||
      result.counters.query_key_point_count !=
          result.required_query_key_point_count ||
      result.counters.component_handle_initialization_count !=
          result.required_component_handle_scratch_count ||
      result.counters.batch_record_scan_count !=
          result.required_batch_record_scan_count ||
      result.counters.union_record_replay_count !=
          result.required_union_record_replay_count ||
      result.counters.query_resolution_count != result.source_query_count ||
      result.counters.locator_snapshot_check_count !=
          expected_snapshot_check_count ||
      outcome_count != result.source_query_count) {
    return false;
  }

  std::size_t previous_prefix = 0U;
  std::size_t observed_relative_positive_count = 0U;
  std::size_t observed_latent_unresolved_count = 0U;
  for (std::size_t query_index = 0U;
       query_index < result.resolutions.size();
       ++query_index) {
    const auto& resolution = result.resolutions[query_index];
    if (resolution.query_index != query_index ||
        resolution.committed_batch_prefix_count < previous_prefix ||
        resolution.committed_batch_prefix_count >
            result.required_committed_batch_prefix_count) {
      return false;
    }
    previous_prefix = resolution.committed_batch_prefix_count;
    switch (resolution.disposition) {
      case ExactDirectSparsePositiveFacetPrefixDisposition::relative_positive:
        if (!resolution.component_handle_present ||
            !resolution.source_binding_witness_present ||
            resolution.component_handle >=
                result.required_component_handle_scratch_count ||
            !source_witness_well_formed(
                resolution.source_binding_witness,
                result.locator_query_witness.external_authority_id) ||
            !checked_accumulate(
                1U, observed_relative_positive_count)) {
          return false;
        }
        break;
      case ExactDirectSparsePositiveFacetPrefixDisposition::latent_unresolved:
        if (resolution.component_handle_present ||
            resolution.source_binding_witness_present ||
            resolution.component_handle !=
                ExactDirectSparseComponentHandle{} ||
            resolution.source_binding_witness !=
                ExactDirectSparseFacetWitness{} ||
            !checked_accumulate(
                1U, observed_latent_unresolved_count)) {
          return false;
        }
        break;
      case ExactDirectSparsePositiveFacetPrefixDisposition::not_certified:
        return false;
    }
  }
  return observed_relative_positive_count ==
             result.counters.relative_positive_count &&
         observed_latent_unresolved_count ==
             result.counters.latent_unresolved_count &&
         (result.resolutions.empty() ||
          result.resolutions.back().committed_batch_prefix_count ==
              result.required_committed_batch_prefix_count);
}

[[nodiscard]] bool observed_storage_within_budget(
    const ExactDirectSparsePositiveFacetPrefixSweepResult& observed,
    const ExactDirectSparsePositiveFacetPrefixSweepBudget& budget) noexcept {
  return observed.resolutions.size() <= budget.maximum_query_count &&
         observed.resolutions.size() <=
             budget.maximum_logical_output_entry_count &&
         observed.logical_output_entry_count == observed.resolutions.size();
}

}  // namespace

bool ExactDirectSparsePositiveFacetPrefixSweepResult::
    certified_partial_refinement() const noexcept {
  return schema_version ==
             direct_sparse_positive_facet_prefix_sweep_schema_version &&
         decision == ExactDirectSparsePositiveFacetPrefixSweepDecision::
                         complete_certified_positive_facet_prefix_sweep &&
         locator_certified_at_entry && budget_preflight_certified &&
         queries_canonical_and_prefix_monotone &&
         requested_locator_history_records_well_formed &&
         dense_identity_dsu_scratch_initialized &&
         each_batch_and_union_prefix_replayed_once &&
         future_binding_slots_are_historical_terminators &&
         every_fingerprint_candidate_compared_by_full_key &&
         every_query_resolved_once &&
         positive_and_latent_outcomes_separated &&
         every_positive_has_historical_root_and_original_witness &&
         every_latent_has_no_positive_payload &&
         common_frozen_locator_snapshot_certified &&
         no_partial_scientific_payload_published &&
         non_scope_is_honest(*this) && complete_payload_shape(*this) &&
         locator_query_witness.external_authority_id != 0U &&
         locator_query_witness.replay_token != 0U &&
         locator_snapshot_stamp.schema_version ==
             direct_sparse_positive_facet_locator_schema_version &&
         locator_snapshot_stamp.external_authority_id ==
             locator_query_witness.external_authority_id &&
         source_query_count <= requested_budget.maximum_query_count &&
         required_query_key_point_count <=
             requested_budget.maximum_query_key_point_count &&
         required_component_handle_scratch_count <=
             requested_budget.maximum_component_handle_scratch_count &&
         required_batch_record_scan_count <=
             requested_budget.maximum_batch_record_scan_count &&
         required_union_record_replay_count <=
             requested_budget.maximum_union_record_replay_count &&
         counters.union_replay_parent_hop_count <=
             requested_budget.maximum_union_replay_parent_hop_count &&
         counters.slot_visit_count <=
             requested_budget.maximum_aggregate_slot_visit_count &&
         counters.query_parent_hop_count <=
             requested_budget.maximum_aggregate_query_parent_hop_count &&
         logical_output_entry_count <=
             requested_budget.maximum_logical_output_entry_count &&
         counters.maximum_single_query_slot_visit_count <=
             requested_budget.facet_probe_budget.maximum_slot_visit_count &&
         counters.maximum_single_query_parent_hop_count <=
             requested_budget.facet_probe_budget
                 .maximum_component_parent_hop_count;
}

bool ExactDirectSparsePositiveFacetPrefixSweepResult::
    certified_atomic_failure() const noexcept {
  const bool recognized_failure =
      decision == ExactDirectSparsePositiveFacetPrefixSweepDecision::
                      no_prefix_sweep_capacity_overflow ||
      decision == ExactDirectSparsePositiveFacetPrefixSweepDecision::
                      no_prefix_sweep_budget_exhausted ||
      decision == ExactDirectSparsePositiveFacetPrefixSweepDecision::
                      no_prefix_sweep_input_shape_rejected ||
      decision == ExactDirectSparsePositiveFacetPrefixSweepDecision::
                      no_prefix_sweep_locator_history_not_certified ||
      decision == ExactDirectSparsePositiveFacetPrefixSweepDecision::
                      no_prefix_sweep_probe_budget_exhausted;
  return schema_version ==
             direct_sparse_positive_facet_prefix_sweep_schema_version &&
         recognized_failure && locator_certified_at_entry &&
         resolutions.empty() && logical_output_entry_count == 0U &&
         no_partial_scientific_payload_published &&
         common_frozen_locator_snapshot_certified &&
         locator_query_witness.external_authority_id != 0U &&
         locator_query_witness.replay_token != 0U &&
         locator_snapshot_stamp.schema_version ==
             direct_sparse_positive_facet_locator_schema_version &&
         locator_snapshot_stamp.external_authority_id ==
             locator_query_witness.external_authority_id &&
         counters.locator_snapshot_check_count >= 2U &&
         non_scope_is_honest(*this);
}

bool ExactDirectSparsePositiveFacetPrefixSweepResult::certified_outcome()
    const noexcept {
  return certified_partial_refinement() || certified_atomic_failure();
}

ExactDirectSparsePositiveFacetPrefixSweepResult
build_exact_direct_sparse_positive_facet_prefix_sweep(
    std::span<const ExactDirectSparsePositiveFacetPrefixQuery> queries,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparsePositiveFacetPrefixSweepBudget& budget) {
  if (!locator.certified_positive_locator()) {
    throw std::invalid_argument(
        "a positive-facet prefix sweep requires a certified locator");
  }
  if (!witness_matches_locator(locator_query_witness, locator)) {
    throw std::invalid_argument(
        "a positive-facet prefix sweep requires one matching locator witness");
  }

  ExactDirectSparsePositiveFacetPrefixSweepResult result;
  result.requested_budget = budget;
  result.locator_query_witness = locator_query_witness;
  result.locator_snapshot_stamp = locator.snapshot_stamp();
  result.counters.locator_snapshot_check_count = 1U;
  result.source_query_count = queries.size();
  result.locator_certified_at_entry = true;
  initialize_scope(result);

  if (queries.size() > budget.maximum_query_count) {
    return fail(
        std::move(result), SweepFailure::budget_exhausted, locator);
  }

  std::size_t query_key_point_count = 0U;
  std::size_t maximum_prefix_count = 0U;
  for (std::size_t query_index = 0U;
       query_index < queries.size();
       ++query_index) {
    const auto& query = queries[query_index];
    if (query.query_index != query_index ||
        !canonical_facet_key(query.facet_key) ||
        query.committed_batch_prefix_count < maximum_prefix_count ||
        query.committed_batch_prefix_count >
            locator.committed_batches().size()) {
      return fail(
          std::move(result), SweepFailure::input_shape_rejected, locator);
    }
    maximum_prefix_count = query.committed_batch_prefix_count;
    if (!checked_accumulate(
            query.facet_key.point_count, query_key_point_count) ||
        !checked_accumulate(1U, result.counters.query_scan_count)) {
      return fail(
          std::move(result), SweepFailure::capacity_overflow, locator);
    }
  }
  result.required_query_key_point_count = query_key_point_count;
  result.counters.query_key_point_count = query_key_point_count;
  result.required_component_handle_scratch_count =
      locator.component_parents().size();
  result.required_committed_batch_prefix_count = maximum_prefix_count;
  std::size_t double_batch_scan_count = 0U;
  if (!try_multiply_size(
          2U, maximum_prefix_count, double_batch_scan_count)) {
    return fail(
        std::move(result), SweepFailure::capacity_overflow, locator);
  }
  result.required_batch_record_scan_count = double_batch_scan_count;
  result.logical_output_entry_count = queries.size();
  if (query_key_point_count > budget.maximum_query_key_point_count ||
      result.required_component_handle_scratch_count >
          budget.maximum_component_handle_scratch_count ||
      double_batch_scan_count > budget.maximum_batch_record_scan_count ||
      queries.size() > budget.maximum_logical_output_entry_count) {
    return fail(
        std::move(result), SweepFailure::budget_exhausted, locator);
  }
  result.queries_canonical_and_prefix_monotone = true;

  std::size_t active_binding_prefix_count = 0U;
  std::size_t union_record_prefix_count = 0U;
  for (std::size_t batch_index = 0U;
       batch_index < maximum_prefix_count;
       ++batch_index) {
    const auto& record = locator.committed_batches()[batch_index];
    if (!batch_record_shape_is_usable(record, batch_index) ||
        !checked_accumulate(
            record.counters.inserted_binding_count,
            active_binding_prefix_count) ||
        !checked_accumulate(
            record.counters.union_request_count,
            union_record_prefix_count) ||
        !checked_accumulate(
            1U, result.counters.batch_record_scan_count)) {
      return fail(
          std::move(result),
          SweepFailure::locator_history_not_certified,
          locator);
    }
  }
  result.required_active_binding_prefix_count =
      active_binding_prefix_count;
  result.required_union_record_replay_count = union_record_prefix_count;
  if (active_binding_prefix_count >
          locator.counters().inserted_binding_count ||
      union_record_prefix_count > locator.committed_unions().size()) {
    return fail(
        std::move(result),
        SweepFailure::locator_history_not_certified,
        locator);
  }
  if (union_record_prefix_count >
      budget.maximum_union_record_replay_count) {
    return fail(
        std::move(result), SweepFailure::budget_exhausted, locator);
  }
  result.requested_locator_history_records_well_formed = true;
  result.budget_preflight_certified = true;

  std::vector<ExactDirectSparseComponentHandle> parents(
      result.required_component_handle_scratch_count);
  std::iota(
      parents.begin(),
      parents.end(),
      ExactDirectSparseComponentHandle{0U});
  result.counters.component_handle_initialization_count = parents.size();
  result.dense_identity_dsu_scratch_initialized = true;

  std::vector<ExactDirectSparsePositiveFacetPrefixResolution> resolutions;
  resolutions.reserve(queries.size());
  std::size_t replayed_batch_count = 0U;
  std::size_t replayed_binding_prefix_count = 0U;
  std::size_t replayed_union_prefix_count = 0U;

  for (const auto& query : queries) {
    while (replayed_batch_count <
           query.committed_batch_prefix_count) {
      const auto& batch =
          locator.committed_batches()[replayed_batch_count];
      std::size_t next_union_prefix_count = 0U;
      if (!try_add_size(
              replayed_union_prefix_count,
              batch.counters.union_request_count,
              next_union_prefix_count) ||
          next_union_prefix_count > locator.committed_unions().size()) {
        return fail(
            std::move(result),
            SweepFailure::locator_history_not_certified,
            locator);
      }
      for (std::size_t union_index = replayed_union_prefix_count;
           union_index < next_union_prefix_count;
           ++union_index) {
        const auto& component_union =
            locator.committed_unions()[union_index];
        if (component_union.committed_union_index != union_index ||
            !source_witness_well_formed(
                component_union.witness,
                locator.config().external_authority_id)) {
          return fail(
              std::move(result),
              SweepFailure::locator_history_not_certified,
              locator);
        }
        const UnionReplayStatus union_status = replay_union(
            parents,
            component_union.left_handle,
            component_union.right_handle,
            budget.maximum_union_replay_parent_hop_count,
            result.counters.union_replay_parent_hop_count);
        if (union_status == UnionReplayStatus::budget_exhausted) {
          return fail(
              std::move(result), SweepFailure::budget_exhausted, locator);
        }
        if (union_status == UnionReplayStatus::invalid_history) {
          return fail(
              std::move(result),
              SweepFailure::locator_history_not_certified,
              locator);
        }
        if (!checked_accumulate(
                1U, result.counters.union_record_replay_count)) {
          return fail(
              std::move(result), SweepFailure::capacity_overflow, locator);
        }
      }
      replayed_union_prefix_count = next_union_prefix_count;
      if (!checked_accumulate(
              batch.counters.inserted_binding_count,
              replayed_binding_prefix_count) ||
          !checked_accumulate(
              1U, result.counters.batch_record_scan_count)) {
        return fail(
            std::move(result), SweepFailure::capacity_overflow, locator);
      }
      ++replayed_batch_count;
      check_locator_snapshot(locator, result);
    }

    if (result.counters.slot_visit_count >
            budget.maximum_aggregate_slot_visit_count ||
        result.counters.query_parent_hop_count >
            budget.maximum_aggregate_query_parent_hop_count) {
      return fail(
          std::move(result), SweepFailure::capacity_overflow, locator);
    }
    const ExactDirectSparsePositiveFacetProbeBudget effective_probe_budget{
        std::min(
            budget.facet_probe_budget.maximum_slot_visit_count,
            budget.maximum_aggregate_slot_visit_count -
                result.counters.slot_visit_count),
        std::min(
            budget.facet_probe_budget.maximum_component_parent_hop_count,
            budget.maximum_aggregate_query_parent_hop_count -
                result.counters.query_parent_hop_count)};
    const PrefixProbeResult probe = probe_prefix(
        query.facet_key,
        replayed_binding_prefix_count,
        parents,
        locator,
        effective_probe_budget);
    if (!add_probe_counters(probe, result.counters)) {
      return fail(
          std::move(result), SweepFailure::capacity_overflow, locator);
    }
    if (probe.status ==
        PrefixProbeStatus::locator_history_not_certified) {
      return fail(
          std::move(result),
          SweepFailure::locator_history_not_certified,
          locator);
    }
    if (probe.status == PrefixProbeStatus::budget_exhausted) {
      const bool aggregate_exhaustion =
          (probe.slot_visit_budget_exhausted &&
           effective_probe_budget.maximum_slot_visit_count <
               budget.facet_probe_budget.maximum_slot_visit_count) ||
          (probe.query_parent_hop_budget_exhausted &&
           effective_probe_budget.maximum_component_parent_hop_count <
               budget.facet_probe_budget
                   .maximum_component_parent_hop_count);
      return fail(
          std::move(result),
          aggregate_exhaustion ? SweepFailure::budget_exhausted
                               : SweepFailure::probe_budget_exhausted,
          locator);
    }

    ExactDirectSparsePositiveFacetPrefixResolution resolution;
    resolution.query_index = query.query_index;
    resolution.committed_batch_prefix_count =
        query.committed_batch_prefix_count;
    if (probe.status == PrefixProbeStatus::relative_positive) {
      resolution.component_handle = probe.component_handle;
      resolution.source_binding_witness = probe.source_binding_witness;
      resolution.component_handle_present = true;
      resolution.source_binding_witness_present = true;
      resolution.disposition =
          ExactDirectSparsePositiveFacetPrefixDisposition::relative_positive;
      if (!checked_accumulate(
              1U, result.counters.relative_positive_count)) {
        return fail(
            std::move(result), SweepFailure::capacity_overflow, locator);
      }
    } else {
      resolution.disposition =
          ExactDirectSparsePositiveFacetPrefixDisposition::latent_unresolved;
      if (!checked_accumulate(
              1U, result.counters.latent_unresolved_count)) {
        return fail(
            std::move(result), SweepFailure::capacity_overflow, locator);
      }
    }
    resolutions.push_back(resolution);
    if (!checked_accumulate(
            1U, result.counters.query_resolution_count)) {
      return fail(
          std::move(result), SweepFailure::capacity_overflow, locator);
    }
    check_locator_snapshot(locator, result);
  }

  result.each_batch_and_union_prefix_replayed_once =
      replayed_batch_count == maximum_prefix_count &&
      replayed_binding_prefix_count ==
          result.required_active_binding_prefix_count &&
      replayed_union_prefix_count ==
          result.required_union_record_replay_count;
  result.future_binding_slots_are_historical_terminators = true;
  result.every_fingerprint_candidate_compared_by_full_key = true;
  result.every_query_resolved_once = true;
  result.positive_and_latent_outcomes_separated = true;
  result.every_positive_has_historical_root_and_original_witness = true;
  result.every_latent_has_no_positive_payload = true;
  check_locator_snapshot(locator, result);
  result.common_frozen_locator_snapshot_certified = true;
  result.resolutions = std::move(resolutions);
  result.decision = ExactDirectSparsePositiveFacetPrefixSweepDecision::
      complete_certified_positive_facet_prefix_sweep;
  if (!result.certified_partial_refinement()) {
    throw std::logic_error(
        "a complete positive-facet prefix sweep failed its contract");
  }
  return result;
}

ExactDirectSparsePositiveFacetPrefixSweepVerification
verify_exact_direct_sparse_positive_facet_prefix_sweep(
    std::span<const ExactDirectSparsePositiveFacetPrefixQuery> queries,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparsePositiveFacetPrefixSweepBudget& budget,
    const ExactDirectSparsePositiveFacetLocatorStructuralVerificationBudget&
        locator_verification_budget,
    const ExactDirectSparsePositiveFacetPrefixSweepResult& observed) {
  ExactDirectSparsePositiveFacetPrefixSweepVerification verification;
  verification.trusted_live_locator_and_witness_certified =
      locator.certified_positive_locator() &&
      witness_matches_locator(locator_query_witness, locator);
  if (!verification.trusted_live_locator_and_witness_certified) {
    return verification;
  }
  verification.observed_storage_within_budget =
      observed_storage_within_budget(observed, budget);
  if (!verification.observed_storage_within_budget) {
    return verification;
  }
  verification.locator_snapshot_matches_observed_build =
      locator.snapshot_stamp() == observed.locator_snapshot_stamp;
  if (!verification.locator_snapshot_matches_observed_build) {
    return verification;
  }

  verification.locator_structural_verification =
      verify_exact_direct_sparse_positive_facet_locator_structure(
          locator.component_parents().size(),
          locator.budget(),
          locator.config(),
          locator_verification_budget,
          locator.state_view());
  const auto& locator_verification =
      verification.locator_structural_verification;
  verification.locator_verification_budget_preflight_certified =
      locator_verification.budget_preflight_certified;
  verification.locator_verification_budget_respected =
      locator_verification.budget_preflight_certified &&
      !locator_verification.budget_exhausted;
  verification.locator_durable_structure_freshly_verified =
      locator_verification.result_certified;
  verification.committed_slot_insertion_chronology_freshly_replayed =
      locator_verification
          .committed_slot_insertion_chronology_freshly_replayed;
  if (!verification.locator_durable_structure_freshly_verified ||
      !verification
           .committed_slot_insertion_chronology_freshly_replayed) {
    return verification;
  }

  const auto expected =
      build_exact_direct_sparse_positive_facet_prefix_sweep(
          queries, locator_query_witness, locator, budget);
  verification.observed_outcome_well_formed = observed.certified_outcome();
  verification.queries_and_prefixes_freshly_replayed =
      observed.source_query_count == expected.source_query_count &&
      observed.required_query_key_point_count ==
          expected.required_query_key_point_count &&
      observed.required_committed_batch_prefix_count ==
          expected.required_committed_batch_prefix_count &&
      observed.resolutions.size() == expected.resolutions.size() &&
      std::equal(
          observed.resolutions.begin(),
          observed.resolutions.end(),
          expected.resolutions.begin(),
          [](const auto& left, const auto& right) {
            return left.query_index == right.query_index &&
                   left.committed_batch_prefix_count ==
                       right.committed_batch_prefix_count;
          });
  verification.union_prefixes_and_historical_roots_freshly_replayed =
      observed.required_active_binding_prefix_count ==
          expected.required_active_binding_prefix_count &&
      observed.required_union_record_replay_count ==
          expected.required_union_record_replay_count &&
      observed.resolutions == expected.resolutions;
  verification.historical_slot_probes_freshly_replayed =
      observed.resolutions == expected.resolutions &&
      observed.counters.slot_visit_count ==
          expected.counters.slot_visit_count &&
      observed.counters.query_parent_hop_count ==
          expected.counters.query_parent_hop_count &&
      observed.counters.full_key_comparison_count ==
          expected.counters.full_key_comparison_count &&
      observed.counters.equal_fingerprint_distinct_key_count ==
          expected.counters.equal_fingerprint_distinct_key_count &&
      observed.counters.future_binding_terminator_count ==
          expected.counters.future_binding_terminator_count;
  verification.counters_and_result_facts_freshly_replayed =
      observed == expected;
  verification.no_locator_mutation_or_batch_commit =
      !observed.locator_state_mutated &&
      !observed.locator_batch_committed &&
      !expected.locator_state_mutated &&
      !expected.locator_batch_committed;
  verification.external_binding_authority_replayed = false;
  verification.source_batch_alignment_replayed = false;
  verification
      .no_isolation_singleton_quotient_forest_or_attachment_invented =
      !observed.missing_facet_means_isolated &&
      !observed.singleton_component_created &&
      !observed.quotient_root_union_or_forest_mutated &&
      !observed.gateway_attach_published &&
      !observed.source_batch_alignment_claimed &&
      !expected.missing_facet_means_isolated &&
      !expected.singleton_component_created &&
      !expected.quotient_root_union_or_forest_mutated &&
      !expected.gateway_attach_published &&
      !expected.source_batch_alignment_claimed;
  verification.no_forbidden_global_structure_materialized =
      non_scope_is_honest(observed) && non_scope_is_honest(expected);
  verification.fresh_replay_certified = observed == expected;
  verification.result_certified =
      verification.trusted_live_locator_and_witness_certified &&
      verification.observed_storage_within_budget &&
      verification.locator_snapshot_matches_observed_build &&
      verification.locator_verification_budget_preflight_certified &&
      verification.locator_verification_budget_respected &&
      verification.locator_durable_structure_freshly_verified &&
      verification
          .committed_slot_insertion_chronology_freshly_replayed &&
      verification.observed_outcome_well_formed &&
      verification.queries_and_prefixes_freshly_replayed &&
      verification
          .union_prefixes_and_historical_roots_freshly_replayed &&
      verification.historical_slot_probes_freshly_replayed &&
      verification.counters_and_result_facts_freshly_replayed &&
      verification.no_locator_mutation_or_batch_commit &&
      !verification.external_binding_authority_replayed &&
      !verification.source_batch_alignment_replayed &&
      verification
          .no_isolation_singleton_quotient_forest_or_attachment_invented &&
      verification.no_forbidden_global_structure_materialized &&
      verification.fresh_replay_certified && expected.certified_outcome();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
