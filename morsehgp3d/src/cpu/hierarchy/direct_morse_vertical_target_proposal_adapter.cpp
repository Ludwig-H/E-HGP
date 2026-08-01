#include "morsehgp3d/hierarchy/direct_morse_forest_carrier_cut_replay_session.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <numeric>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include <boost/multiprecision/integer.hpp>

namespace morsehgp3d::hierarchy {
namespace {

using PlanDecision = ExactDirectMorseVerticalTargetFacetPlanDecision;
using PlanResult = ExactDirectMorseVerticalTargetFacetPlanResult;
using AdapterDecision =
    ExactDirectMorseVerticalTargetProposalAdapterDecision;
using AdapterResult = ExactDirectMorseVerticalTargetProposalAdapterResult;
using TerminalDisposition =
    ExactDirectMorseForestCarrierCutClosureTerminalDisposition;

[[nodiscard]] bool checked_add(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (left > std::numeric_limits<std::size_t>::max() - right) {
    return false;
  }
  result = left + right;
  return true;
}

[[nodiscard]] bool checked_multiply(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  result = left * right;
  return true;
}

[[nodiscard]] bool facet_key_less(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right) noexcept {
  if (left.point_count != right.point_count) {
    return left.point_count < right.point_count;
  }
  return std::lexicographical_compare(
      left.point_ids.begin(),
      left.point_ids.end(),
      right.point_ids.begin(),
      right.point_ids.end());
}

[[nodiscard]] bool canonical_key_shape(
    const ExactDirectSparseFacetKey& key,
    std::size_t expected_cardinality,
    std::size_t point_count) noexcept {
  if (expected_cardinality == 0U ||
      key.point_count != expected_cardinality ||
      key.point_count > direct_sparse_positive_facet_maximum_point_count) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (key.point_ids[index] >= point_count ||
        (index != 0U &&
         key.point_ids[index - 1U] >= key.point_ids[index])) {
      return false;
    }
  }
  return std::all_of(
      key.point_ids.begin() +
          static_cast<std::ptrdiff_t>(key.point_count),
      key.point_ids.end(),
      [](spatial::PointId point_id) { return point_id == 0U; });
}

[[nodiscard]] ExactDirectSparseFacetKey erase_point(
    const ExactDirectSparseFacetKey& source,
    std::size_t erased_index) noexcept {
  ExactDirectSparseFacetKey result;
  if (source.point_count <= 1U || erased_index >= source.point_count) {
    return result;
  }
  result.point_count = source.point_count - 1U;
  std::size_t output_index = 0U;
  for (std::size_t source_index = 0U;
       source_index < source.point_count;
       ++source_index) {
    if (source_index != erased_index) {
      result.point_ids[output_index] = source.point_ids[source_index];
      ++output_index;
    }
  }
  return result;
}

template <typename Value, typename CountedLess>
[[nodiscard]] bool sift_down(
    std::vector<Value>& values,
    std::size_t root,
    std::size_t end,
    CountedLess& less) {
  while (end >= 2U && root <= (end - 2U) / 2U) {
    std::size_t child = root * 2U + 1U;
    std::size_t selected = root;
    bool comparison = false;
    if (!less(values[selected], values[child], comparison)) {
      return false;
    }
    if (comparison) {
      selected = child;
    }
    if (child + 1U < end) {
      ++child;
      if (!less(values[selected], values[child], comparison)) {
        return false;
      }
      if (comparison) {
        selected = child;
      }
    }
    if (selected == root) {
      return true;
    }
    std::swap(values[root], values[selected]);
    root = selected;
  }
  return true;
}

template <typename Value, typename CountedLess>
[[nodiscard]] bool counted_heap_sort(
    std::vector<Value>& values,
    CountedLess&& less) {
  if (values.size() < 2U) {
    return true;
  }
  for (std::size_t start = values.size() / 2U; start != 0U; --start) {
    if (!sift_down(values, start - 1U, values.size(), less)) {
      return false;
    }
  }
  for (std::size_t end = values.size(); end > 1U; --end) {
    std::swap(values[0U], values[end - 1U]);
    if (!sift_down(values, 0U, end - 1U, less)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool group_ranges(
    const ExactDirectMorseForestJournalResult& forest,
    std::size_t group_index,
    const ExactDirectMorseForestAtomicGroup*& group,
    const ExactDirectMorseForestBatch*& batch) noexcept {
  if (group_index >= forest.atomic_groups.size()) {
    return false;
  }
  group = &forest.atomic_groups[group_index];
  if (group->atomic_group_index != group_index ||
      group->batch_index >= forest.batches.size() ||
      group->saddle_record_offset > forest.saddle_records.size() ||
      group->saddle_record_count == 0U ||
      group->saddle_record_count >
          forest.saddle_records.size() - group->saddle_record_offset) {
    return false;
  }
  batch = &forest.batches[group->batch_index];
  return batch->batch_index == group->batch_index &&
         batch->atomic_group_offset <= group_index &&
         group_index - batch->atomic_group_offset <
             batch->atomic_group_count &&
         batch->order >= 2U && batch->order <= forest.point_count;
}

void fail_plan(PlanResult& result, PlanDecision decision) noexcept {
  result.representatives.clear();
  result.target_facet_indices.clear();
  result.canonical_distinct_target_facet_keys.clear();
  result.representative_indices_in_binding_order.clear();
  result.no_partial_payload_published_on_failure = true;
  result.decision = decision;
  result.scope = ExactDirectMorseVerticalTargetFacetPlanScope::unspecified;
}

[[nodiscard]] bool plan_non_scope_honest(
    const PlanResult& result) noexcept {
  return result.no_locator_or_closure_graph_consumed &&
         result.forest_relative_only &&
         !result
              .global_facet_coface_incidence_cell_gamma_or_delaunay_materialized &&
         !result.public_status_claimed;
}

[[nodiscard]] bool adapter_non_scope_honest(
    const AdapterResult& result) noexcept {
  return !result.audit.plan_or_summary_payload_retained &&
         !result.audit.locator_reference_exposed &&
         !result.audit.closure_graph_or_projection_persisted &&
         !result.audit.locator_batch_committed &&
         !result.audit
              .global_facet_coface_or_incidence_structure_materialized &&
         !result.audit
              .gamma_cells_or_higher_order_delaunay_materialized &&
         !result.audit.external_vertical_authority_replayed &&
         !result.audit.vertical_maps_complete &&
         result.audit.forest_relative_only &&
         !result.audit.public_status_claimed;
}

[[nodiscard]] std::size_t positive_integer_bit_count(
    const exact::BigInt& value) {
  const exact::BigInt absolute = value < 0 ? -value : value;
  if (absolute == 0) {
    return 1U;
  }
  return static_cast<std::size_t>(boost::multiprecision::msb(absolute)) +
         1U;
}

[[nodiscard]] bool measure_level(
    const exact::ExactLevel& level,
    const ExactDirectMorseVerticalTargetProposalAdapterBudget& budget,
    ExactDirectMorseVerticalTargetProposalAdapterCounters& counters) {
  const std::size_t observed = std::max(
      positive_integer_bit_count(level.numerator()),
      positive_integer_bit_count(level.denominator()));
  counters.maximum_observed_exact_level_integer_bit_count = std::max(
      counters.maximum_observed_exact_level_integer_bit_count,
      observed);
  return observed <= budget.maximum_single_exact_level_integer_bit_count;
}

[[nodiscard]] bool plan_metadata_matches_group(
    const PlanResult& plan,
    const ExactDirectMorseForestJournalResult& forest,
    const ExactDirectMorseForestAtomicGroup& group,
    const ExactDirectMorseForestBatch& batch) {
  return plan.point_count == forest.point_count &&
         plan.source_atomic_group_index == group.atomic_group_index &&
         plan.source_batch_index == batch.batch_index &&
         plan.source_order == batch.order && plan.source_order >= 2U &&
         plan.target_order + 1U == plan.source_order &&
         plan.source_batch_squared_level == batch.squared_level;
}

struct AdapterBuildContext {
  const ExactDirectMorseForestCarrierCutReplayView* view{};
  const ExactDirectMorseForestJournalResult* forest{};
  const PlanResult* plan{};
  const ExactDirectMorseForestCarrierCutClosureAdapterResult* closure{};
  const ExactDirectMorseVerticalTargetProposalAdapterBudget* budget{};
  AdapterResult* result{};
};

void fail_adapter(AdapterResult& result, AdapterDecision decision) noexcept {
  result.proposals.clear();
  result.audit.no_partial_payload_published_on_failure = true;
  result.decision = decision;
  result.scope =
      ExactDirectMorseVerticalTargetProposalAdapterScope::unspecified;
}

[[nodiscard]] std::optional<std::size_t> find_plan_representative(
    const PlanResult& plan,
    const ExactDirectSparseFacetKey& key,
    const ExactDirectMorseVerticalTargetProposalAdapterBudget& budget,
    ExactDirectMorseVerticalTargetProposalAdapterCounters& counters,
    bool& budget_exhausted) {
  budget_exhausted = false;
  std::size_t first = 0U;
  std::size_t count = plan.representatives.size();
  while (count != 0U) {
    if (counters.source_key_lookup_comparison_count >=
        budget.maximum_source_key_lookup_comparison_count) {
      budget_exhausted = true;
      return std::nullopt;
    }
    ++counters.source_key_lookup_comparison_count;
    const std::size_t step = count / 2U;
    const std::size_t middle = first + step;
    if (facet_key_less(
            plan.representatives[middle].source_strict_arm_key, key)) {
      first = middle + 1U;
      count -= step + 1U;
    } else {
      count = step;
    }
  }
  if (first >= plan.representatives.size() ||
      plan.representatives[first].source_strict_arm_key != key) {
    return std::nullopt;
  }
  return first;
}

enum class SourcePlanRevalidation : std::uint8_t {
  complete,
  budget_exhausted,
  rejected,
};

[[nodiscard]] SourcePlanRevalidation revalidate_plan_source_group(
    AdapterBuildContext& context,
    const ExactDirectMorseForestAtomicGroup& group) {
  const auto& forest = *context.forest;
  const auto& plan = *context.plan;
  const auto& budget = *context.budget;
  auto& counters = context.result->counters;
  if (group.saddle_record_count >
      budget.maximum_source_saddle_revalidation_count) {
    return SourcePlanRevalidation::budget_exhausted;
  }

  std::size_t total_binding_count = 0U;
  for (std::size_t local_saddle = 0U;
       local_saddle < group.saddle_record_count;
       ++local_saddle) {
    ++counters.source_saddle_revalidation_count;
    const std::size_t saddle_index =
        group.saddle_record_offset + local_saddle;
    const auto& saddle = forest.saddle_records[saddle_index];
    if (saddle.saddle_record_index != saddle_index ||
        saddle.atomic_group_index != group.atomic_group_index ||
        saddle.arm_binding_offset > forest.arm_root_bindings.size() ||
        saddle.arm_binding_count >
            forest.arm_root_bindings.size() -
                saddle.arm_binding_offset ||
        !checked_add(
            total_binding_count,
            saddle.arm_binding_count,
            total_binding_count)) {
      return SourcePlanRevalidation::rejected;
    }
  }
  if (total_binding_count >
      budget.maximum_source_binding_revalidation_count) {
    return SourcePlanRevalidation::budget_exhausted;
  }

  std::size_t representative_hit_count = 0U;
  for (std::size_t local_saddle = 0U;
       local_saddle < group.saddle_record_count;
       ++local_saddle) {
    const auto& saddle = forest.saddle_records[
        group.saddle_record_offset + local_saddle];
    for (std::size_t local_binding = 0U;
         local_binding < saddle.arm_binding_count;
         ++local_binding) {
      ++counters.source_binding_revalidation_count;
      const std::size_t binding_index =
          saddle.arm_binding_offset + local_binding;
      const auto& binding = forest.arm_root_bindings[binding_index];
      if (binding.binding_index != binding_index) {
        return SourcePlanRevalidation::rejected;
      }
      bool lookup_budget_exhausted = false;
      const auto representative_index = find_plan_representative(
          plan,
          binding.strict_arm_key,
          budget,
          counters,
          lookup_budget_exhausted);
      if (!representative_index.has_value()) {
        return lookup_budget_exhausted
                   ? SourcePlanRevalidation::budget_exhausted
                   : SourcePlanRevalidation::rejected;
      }
      const auto& representative =
          plan.representatives[*representative_index];
      if (representative.representative_arm_root_binding_index >
          binding_index) {
        return SourcePlanRevalidation::rejected;
      }
      if (representative.representative_arm_root_binding_index ==
          binding_index) {
        ++representative_hit_count;
      }
    }
  }
  return representative_hit_count == plan.representatives.size()
             ? SourcePlanRevalidation::complete
             : SourcePlanRevalidation::rejected;
}

[[nodiscard]] bool build_proposals_with_frozen_locator(
    void* erased_context,
    std::size_t point_count,
    std::size_t target_order,
    const exact::ExactLevel& closed_squared_level,
    const ExactDirectSparsePositiveFacetLocatorSnapshotStamp& entry_stamp,
    const ExactDirectSparsePositiveFacetLocator& locator) {
  (void)locator;
  auto& context = *static_cast<AdapterBuildContext*>(erased_context);
  auto& result = *context.result;
  const auto& forest = *context.forest;
  const auto& plan = *context.plan;
  const auto& closure = *context.closure;
  const auto& budget = *context.budget;
  result.point_count = point_count;
  result.target_order = target_order;
  result.source_atomic_group_index = plan.source_atomic_group_index;
  result.source_batch_index = plan.source_batch_index;
  result.source_order = plan.source_order;
  result.source_batch_squared_level = plan.source_batch_squared_level;
  result.locator_snapshot_stamp = entry_stamp;
  result.input_representative_count = plan.representatives.size();
  result.input_distinct_target_facet_count =
      plan.canonical_distinct_target_facet_keys.size();
  result.input_projected_target_facet_reference_count =
      plan.target_facet_indices.size();
  result.audit.view_epoch_checked_at_entry = true;

  const ExactDirectMorseForestAtomicGroup* group = nullptr;
  const ExactDirectMorseForestBatch* batch = nullptr;
  if (!forest.certified_conditional_h0_candidate()) {
    fail_adapter(result, AdapterDecision::no_adapter_source_forest_rejected);
    return true;
  }
  result.audit.source_forest_certified = true;
  if (!plan.certified_group_local_target_facet_plan()) {
    fail_adapter(result, AdapterDecision::no_adapter_plan_rejected);
    return true;
  }
  result.audit.plan_certified = true;
  if (!group_ranges(
          forest,
          plan.source_atomic_group_index,
          group,
          batch) ||
      !plan_metadata_matches_group(plan, forest, *group, *batch)) {
    fail_adapter(result, AdapterDecision::no_adapter_source_group_mismatch);
    return true;
  }
  if (point_count != forest.point_count) {
    fail_adapter(result, AdapterDecision::no_adapter_source_group_mismatch);
    return true;
  }
  result.audit.point_count_matches = true;
  std::size_t required_exact_level_comparison_capacity = 0U;
  if (!checked_add(
          3U,
          plan.representatives.size(),
          required_exact_level_comparison_capacity)) {
    fail_adapter(result, AdapterDecision::no_adapter_capacity_overflow);
    return true;
  }
  if (plan.target_facet_indices.size() >
          budget.maximum_positive_terminal_probe_count ||
      plan.target_facet_indices.size() >
          budget.maximum_carrier_entry_revalidation_count ||
      plan.representatives.size() >
          budget.maximum_target_node_lookup_count ||
      required_exact_level_comparison_capacity >
          budget.maximum_exact_level_comparison_count ||
      !measure_level(batch->squared_level, budget, result.counters) ||
      !measure_level(
          plan.source_batch_squared_level, budget, result.counters) ||
      !measure_level(closed_squared_level, budget, result.counters)) {
    fail_adapter(result, AdapterDecision::no_adapter_budget_exhausted);
    return true;
  }
  if (target_order + 1U != batch->order ||
      target_order != plan.target_order) {
    fail_adapter(
        result, AdapterDecision::no_adapter_order_or_exact_cut_mismatch);
    return true;
  }
  result.audit.adjacent_target_order_matches = true;
  ++result.counters.exact_level_comparison_count;
  if (closed_squared_level != batch->squared_level) {
    fail_adapter(
        result, AdapterDecision::no_adapter_order_or_exact_cut_mismatch);
    return true;
  }
  ++result.counters.exact_level_comparison_count;
  if (closed_squared_level != plan.source_batch_squared_level) {
    fail_adapter(
        result, AdapterDecision::no_adapter_order_or_exact_cut_mismatch);
    return true;
  }
  result.audit.exact_cut_matches_source_batch = true;

  if (!closure.certified_compact_closure_summary()) {
    fail_adapter(
        result, AdapterDecision::no_adapter_closure_summary_rejected);
    return true;
  }
  result.audit.closure_summary_certified = true;
  if (!measure_level(
          closure.closed_squared_level, budget, result.counters)) {
    fail_adapter(result, AdapterDecision::no_adapter_budget_exhausted);
    return true;
  }
  ++result.counters.exact_level_comparison_count;
  if (closure.point_count != point_count ||
      closure.target_order != target_order ||
      closure.closed_squared_level != closed_squared_level ||
      closure.locator_snapshot_stamp != entry_stamp ||
      closure.locator_query_witness.external_authority_id == 0U ||
      closure.locator_query_witness.replay_token == 0U) {
    fail_adapter(
        result, AdapterDecision::no_adapter_closure_snapshot_mismatch);
    return true;
  }
  result.audit.closure_snapshot_matches_live_view = true;
  result.external_target_authority_id =
      closure.locator_query_witness.external_authority_id;
  result.invocation_replay_token =
      closure.locator_query_witness.replay_token;

  const std::size_t representative_count = plan.representatives.size();
  const std::size_t target_facet_count =
      plan.canonical_distinct_target_facet_keys.size();
  if (representative_count > budget.maximum_proposal_count ||
      target_facet_count > budget.maximum_closure_summary_scan_count ||
      representative_count > budget.maximum_logical_output_entry_count) {
    fail_adapter(result, AdapterDecision::no_adapter_budget_exhausted);
    return true;
  }
  result.required_logical_output_entry_count = representative_count;

  const auto source_revalidation =
      revalidate_plan_source_group(context, *group);
  if (source_revalidation == SourcePlanRevalidation::budget_exhausted) {
    fail_adapter(result, AdapterDecision::no_adapter_budget_exhausted);
    return true;
  }
  if (source_revalidation != SourcePlanRevalidation::complete) {
    fail_adapter(result, AdapterDecision::no_adapter_source_group_mismatch);
    return true;
  }
  result.audit.plan_revalidated_against_exact_source_group = true;

  if (closure.terminal_summaries.size() != target_facet_count) {
    fail_adapter(
        result, AdapterDecision::no_adapter_summary_bijection_rejected);
    return true;
  }
  for (std::size_t target_index = 0U;
       target_index < target_facet_count;
       ++target_index) {
    ++result.counters.closure_summary_scan_count;
    if (closure.terminal_summaries[target_index].seed_index !=
            target_index ||
        closure.terminal_summaries[target_index].source_facet_key !=
            plan.canonical_distinct_target_facet_keys[target_index]) {
      fail_adapter(
          result, AdapterDecision::no_adapter_summary_bijection_rejected);
      return true;
    }
  }
  result.audit.closure_sources_biject_canonical_target_facets = true;

  result.proposals.reserve(representative_count);
  result.audit.output_preallocated_after_budget_preflight = true;
  const ExactDirectMorseForestJournalView forest_view{forest};
  bool all_resolved = true;
  for (const std::size_t representative_index :
       plan.representative_indices_in_binding_order) {
    if (representative_index >= plan.representatives.size()) {
      fail_adapter(result, AdapterDecision::no_adapter_plan_rejected);
      return true;
    }
    const auto& representative =
        plan.representatives[representative_index];
    std::size_t unresolved_count = 0U;
    std::size_t latent_count = 0U;
    std::size_t resolved_count = 0U;
    std::optional<ExactDirectMorseForestNodeId> unique_root;
    for (std::size_t local_facet = 0U;
         local_facet < representative.target_facet_index_count;
         ++local_facet) {
      const std::size_t mapping_index =
          representative.target_facet_index_offset + local_facet;
      if (mapping_index >= plan.target_facet_indices.size()) {
        fail_adapter(result, AdapterDecision::no_adapter_plan_rejected);
        return true;
      }
      const std::size_t target_facet_index =
          plan.target_facet_indices[mapping_index];
      if (target_facet_index >= closure.terminal_summaries.size()) {
        fail_adapter(
            result, AdapterDecision::no_adapter_summary_bijection_rejected);
        return true;
      }
      ++result.counters.projected_target_facet_aggregation_count;
      const auto& summary =
          closure.terminal_summaries[target_facet_index];
      if (summary.disposition ==
              TerminalDisposition::positive_active_latent ||
          summary.disposition ==
              TerminalDisposition::positive_resolved_reduced_root) {
        if (!summary.canonical_component_handle.has_value() ||
            !summary.binding_witness.has_value() ||
            !summary.carrier_cut_entry.has_value()) {
          fail_adapter(
              result, AdapterDecision::no_adapter_target_seed_rejected);
          return true;
        }
        if (result.counters.positive_terminal_probe_count >=
            budget.maximum_positive_terminal_probe_count) {
          fail_adapter(result, AdapterDecision::no_adapter_budget_exhausted);
          return true;
        }
        const auto fresh_probe = context.view->probe_positive_facet(
            summary.terminal_facet_key,
            *summary.binding_witness,
            budget.positive_terminal_probe_budget);
        ++result.counters.positive_terminal_probe_count;
        if (!checked_add(
                result.counters.positive_terminal_probe_slot_visit_count,
                fresh_probe.slot_visit_count,
                result.counters
                    .positive_terminal_probe_slot_visit_count) ||
            !checked_add(
                result.counters.positive_terminal_probe_parent_hop_count,
                fresh_probe.component_parent_hop_count,
                result.counters
                    .positive_terminal_probe_parent_hop_count) ||
            !checked_add(
                result.counters
                    .positive_terminal_probe_full_key_comparison_count,
                fresh_probe.full_key_comparison_count,
                result.counters
                    .positive_terminal_probe_full_key_comparison_count)) {
          fail_adapter(result, AdapterDecision::no_adapter_capacity_overflow);
          return true;
        }
        if (result.counters.positive_terminal_probe_slot_visit_count >
                budget.maximum_positive_terminal_probe_slot_visit_count ||
            result.counters.positive_terminal_probe_parent_hop_count >
                budget.maximum_positive_terminal_probe_parent_hop_count ||
            fresh_probe.certified_budget_exhaustion()) {
          fail_adapter(result, AdapterDecision::no_adapter_budget_exhausted);
          return true;
        }
        if (!fresh_probe.certified_positive_hit() ||
            !fresh_probe.component_handle_present ||
            fresh_probe.component_handle !=
                *summary.canonical_component_handle) {
          fail_adapter(
              result, AdapterDecision::no_adapter_target_seed_rejected);
          return true;
        }
        if (result.counters.carrier_entry_revalidation_count >=
            budget.maximum_carrier_entry_revalidation_count) {
          fail_adapter(result, AdapterDecision::no_adapter_budget_exhausted);
          return true;
        }
        ++result.counters.carrier_entry_revalidation_count;
        try {
          const auto live_entry = context.view->find_entry(
              *summary.canonical_component_handle);
          if (!live_entry.has_value() ||
              *live_entry != *summary.carrier_cut_entry) {
            fail_adapter(
                result, AdapterDecision::no_adapter_target_seed_rejected);
            return true;
          }
        } catch (const std::logic_error&) {
          fail_adapter(
              result, AdapterDecision::no_adapter_target_seed_rejected);
          return true;
        }
      }
      switch (summary.disposition) {
        case TerminalDisposition::closure_unresolved:
          ++unresolved_count;
          break;
        case TerminalDisposition::positive_active_latent:
          ++latent_count;
          break;
        case TerminalDisposition::positive_resolved_reduced_root: {
          ++resolved_count;
          if (!summary.carrier_cut_entry.has_value() ||
              !summary.carrier_cut_entry->reduced_root_node_id.has_value()) {
            fail_adapter(
                result, AdapterDecision::no_adapter_target_seed_rejected);
            return true;
          }
          const auto root =
              *summary.carrier_cut_entry->reduced_root_node_id;
          if (unique_root.has_value() && *unique_root != root) {
            fail_adapter(
                result,
                AdapterDecision::no_adapter_known_target_root_contradiction);
            return true;
          }
          unique_root = root;
          break;
        }
        case TerminalDisposition::not_certified:
          fail_adapter(
              result, AdapterDecision::no_adapter_closure_summary_rejected);
          return true;
      }
    }
    if (unresolved_count + latent_count + resolved_count !=
        representative.target_facet_index_count) {
      fail_adapter(
          result, AdapterDecision::no_adapter_summary_bijection_rejected);
      return true;
    }

    ExactDirectMorseVerticalTargetProposal proposal;
    proposal.representative_arm_root_binding_index =
        representative.representative_arm_root_binding_index;
    proposal.replay_token = result.invocation_replay_token;
    if (unresolved_count != 0U || latent_count != 0U) {
      all_resolved = false;
      proposal.disposition =
          ExactDirectMorseVerticalProposalDisposition::unresolved;
      ++result.counters.unresolved_proposal_count;
    } else {
      if (resolved_count != representative.target_facet_index_count ||
          !unique_root.has_value() ||
          result.counters.target_node_lookup_count >=
              budget.maximum_target_node_lookup_count ||
          result.counters.exact_level_comparison_count >=
              budget.maximum_exact_level_comparison_count) {
        fail_adapter(
            result,
            result.counters.target_node_lookup_count >=
                        budget.maximum_target_node_lookup_count ||
                    result.counters.exact_level_comparison_count >=
                        budget.maximum_exact_level_comparison_count
                ? AdapterDecision::no_adapter_budget_exhausted
                : AdapterDecision::no_adapter_target_seed_rejected);
        return true;
      }
      ++result.counters.target_node_lookup_count;
      if (*unique_root >= forest_view.node_count()) {
        fail_adapter(result, AdapterDecision::no_adapter_target_seed_rejected);
        return true;
      }
      const auto node = forest_view.node_at(*unique_root);
      if (!measure_level(node.squared_level, budget, result.counters)) {
        fail_adapter(result, AdapterDecision::no_adapter_budget_exhausted);
        return true;
      }
      ++result.counters.exact_level_comparison_count;
      if (node.order != target_order ||
          node.squared_level > closed_squared_level) {
        fail_adapter(result, AdapterDecision::no_adapter_target_seed_rejected);
        return true;
      }
      proposal.target_seed_node_id = unique_root;
      proposal.disposition =
          ExactDirectMorseVerticalProposalDisposition::resolved_target_seed;
      ++result.counters.resolved_proposal_count;
    }
    result.proposals.push_back(proposal);
  }

  result.audit.every_projected_target_facet_aggregated = true;
  result.audit.every_positive_terminal_key_binding_reprobed = true;
  result.required_carrier_entry_revalidation_count =
      result.counters.carrier_entry_revalidation_count;
  result.audit.every_known_resolved_root_consistent_per_source_binding = true;
  result.audit.unresolved_or_latent_maps_only_to_unresolved = true;
  result.audit.all_resolved_unique_root_maps_to_target_seed = true;
  result.audit.common_invocation_replay_token_used = true;
  result.audit.proposals_sorted_unique_by_binding_index =
      std::adjacent_find(
          result.proposals.begin(),
          result.proposals.end(),
          [](const auto& left, const auto& right) {
            return left.representative_arm_root_binding_index >=
                   right.representative_arm_root_binding_index;
          }) == result.proposals.end();
  if (!result.audit.proposals_sorted_unique_by_binding_index) {
    fail_adapter(result, AdapterDecision::no_adapter_plan_rejected);
    return true;
  }
  result.decision =
      all_resolved
          ? AdapterDecision::
                complete_all_resolved_group_local_vertical_target_proposals
          : AdapterDecision::
                complete_with_unresolved_group_local_vertical_target_proposals;
  result.scope = ExactDirectMorseVerticalTargetProposalAdapterScope::
      one_live_target_order_cut_one_source_atomic_group_compact_vertical_target_proposals_only;
  return true;
}

[[nodiscard]] bool rejection_decision(AdapterDecision decision) noexcept {
  return decision != AdapterDecision::not_certified &&
         decision !=
             AdapterDecision::
                 complete_all_resolved_group_local_vertical_target_proposals &&
         decision !=
             AdapterDecision::
                 complete_with_unresolved_group_local_vertical_target_proposals;
}

}  // namespace

ExactDirectMorseVerticalTargetFacetPlanResult
build_exact_direct_morse_vertical_target_facet_plan(
    const ExactDirectMorseForestJournalResult& source_forest,
    std::size_t source_atomic_group_index,
    const ExactDirectMorseVerticalTargetFacetPlanBudget& budget) {
  PlanResult result;
  result.requested_budget = budget;
  result.point_count = source_forest.point_count;
  result.source_atomic_group_index = source_atomic_group_index;
  if (!source_forest.certified_conditional_h0_candidate()) {
    fail_plan(result, PlanDecision::no_plan_source_forest_rejected);
    return result;
  }
  result.source_forest_certified = true;

  const ExactDirectMorseForestAtomicGroup* group = nullptr;
  const ExactDirectMorseForestBatch* batch = nullptr;
  if (!group_ranges(
          source_forest,
          source_atomic_group_index,
          group,
          batch)) {
    fail_plan(result, PlanDecision::no_plan_group_rejected);
    return result;
  }
  result.source_batch_index = batch->batch_index;
  result.source_order = batch->order;
  result.target_order = batch->order - 1U;
  try {
    result.source_batch_squared_level = batch->squared_level;
  } catch (const std::bad_alloc&) {
    fail_plan(result, PlanDecision::no_plan_allocation_failed);
    return result;
  } catch (const std::length_error&) {
    fail_plan(result, PlanDecision::no_plan_capacity_overflow);
    return result;
  }

  try {
    if (group->saddle_record_count >
        budget.maximum_group_saddle_scan_count) {
      fail_plan(result, PlanDecision::no_plan_budget_exhausted);
      return result;
    }
    std::size_t binding_count = 0U;
    for (std::size_t local_saddle = 0U;
         local_saddle < group->saddle_record_count;
         ++local_saddle) {
      ++result.counters.group_saddle_scan_count;
      const std::size_t saddle_index =
          group->saddle_record_offset + local_saddle;
      const auto& saddle = source_forest.saddle_records[saddle_index];
      if (saddle.saddle_record_index != saddle_index ||
          saddle.atomic_group_index != source_atomic_group_index ||
          saddle.source_journal_batch_index != batch->batch_index ||
          saddle.arm_binding_offset >
              source_forest.arm_root_bindings.size() ||
          saddle.arm_binding_count >
              source_forest.arm_root_bindings.size() -
                  saddle.arm_binding_offset ||
          !checked_add(
              binding_count,
              saddle.arm_binding_count,
              binding_count)) {
        fail_plan(result, PlanDecision::no_plan_group_rejected);
        return result;
      }
    }
    if (binding_count == 0U ||
        binding_count > budget.maximum_group_arm_binding_scan_count ||
        binding_count > budget.maximum_binding_sort_scratch_count) {
      fail_plan(result, PlanDecision::no_plan_budget_exhausted);
      return result;
    }

    std::vector<std::size_t> binding_indices;
    binding_indices.reserve(binding_count);
    for (std::size_t local_saddle = 0U;
         local_saddle < group->saddle_record_count;
         ++local_saddle) {
      const auto& saddle = source_forest.saddle_records[
          group->saddle_record_offset + local_saddle];
      for (std::size_t local_binding = 0U;
           local_binding < saddle.arm_binding_count;
           ++local_binding) {
        const std::size_t binding_index =
            saddle.arm_binding_offset + local_binding;
        ++result.counters.group_arm_binding_scan_count;
        const auto& binding =
            source_forest.arm_root_bindings[binding_index];
        if (binding.binding_index != binding_index ||
            !canonical_key_shape(
                binding.strict_arm_key,
                batch->order,
                source_forest.point_count)) {
          fail_plan(result, PlanDecision::no_plan_group_rejected);
          return result;
        }
        binding_indices.push_back(binding_index);
      }
    }

    auto binding_less =
        [&](std::size_t left, std::size_t right, bool& is_less) {
          if (result.counters.sort_comparison_count >=
              budget.maximum_sort_comparison_count) {
            return false;
          }
          ++result.counters.sort_comparison_count;
          const auto& left_key =
              source_forest.arm_root_bindings[left].strict_arm_key;
          const auto& right_key =
              source_forest.arm_root_bindings[right].strict_arm_key;
          is_less = facet_key_less(left_key, right_key) ||
                    (left_key == right_key && left < right);
          return true;
        };
    if (!counted_heap_sort(binding_indices, binding_less)) {
      fail_plan(result, PlanDecision::no_plan_budget_exhausted);
      return result;
    }

    std::vector<std::size_t> representative_bindings;
    representative_bindings.reserve(binding_indices.size());
    for (const std::size_t binding_index : binding_indices) {
      if (representative_bindings.empty() ||
          source_forest
                  .arm_root_bindings[representative_bindings.back()]
                  .strict_arm_key !=
              source_forest.arm_root_bindings[binding_index]
                  .strict_arm_key) {
        representative_bindings.push_back(binding_index);
      }
    }
    const std::size_t representative_count =
        representative_bindings.size();
    if (representative_count == 0U ||
        representative_count >
            budget.maximum_representative_binding_count) {
      fail_plan(result, PlanDecision::no_plan_budget_exhausted);
      return result;
    }

    std::size_t projected_reference_count = 0U;
    if (!checked_multiply(
            representative_count,
            batch->order,
            projected_reference_count)) {
      fail_plan(result, PlanDecision::no_plan_capacity_overflow);
      return result;
    }
    if (projected_reference_count >
        budget.maximum_projected_target_facet_reference_count) {
      fail_plan(result, PlanDecision::no_plan_budget_exhausted);
      return result;
    }

    std::vector<ExactDirectSparseFacetKey> projected_facets;
    projected_facets.reserve(projected_reference_count);
    for (const std::size_t binding_index : representative_bindings) {
      const auto& source_key =
          source_forest.arm_root_bindings[binding_index].strict_arm_key;
      for (std::size_t erased_index = 0U;
           erased_index < source_key.point_count;
           ++erased_index) {
        projected_facets.push_back(erase_point(source_key, erased_index));
        ++result.counters.generated_target_facet_reference_count;
      }
    }
    auto facet_less = [&](const ExactDirectSparseFacetKey& left,
                          const ExactDirectSparseFacetKey& right,
                          bool& is_less) {
      if (result.counters.sort_comparison_count >=
          budget.maximum_sort_comparison_count) {
        return false;
      }
      ++result.counters.sort_comparison_count;
      is_less = facet_key_less(left, right);
      return true;
    };
    if (!counted_heap_sort(projected_facets, facet_less)) {
      fail_plan(result, PlanDecision::no_plan_budget_exhausted);
      return result;
    }
    const auto unique_end =
        std::unique(projected_facets.begin(), projected_facets.end());
    const std::size_t distinct_target_facet_count =
        static_cast<std::size_t>(unique_end - projected_facets.begin());
    if (distinct_target_facet_count == 0U ||
        distinct_target_facet_count >
            budget.maximum_distinct_target_facet_count) {
      fail_plan(result, PlanDecision::no_plan_budget_exhausted);
      return result;
    }

    std::size_t source_key_point_references = 0U;
    std::size_t target_key_point_references = 0U;
    std::size_t retained_key_point_references = 0U;
    std::size_t logical_output = 0U;
    if (!checked_multiply(
            representative_count,
            batch->order,
            source_key_point_references) ||
        !checked_multiply(
            distinct_target_facet_count,
            batch->order - 1U,
            target_key_point_references) ||
        !checked_add(
            source_key_point_references,
            target_key_point_references,
            retained_key_point_references) ||
        !checked_add(
            representative_count,
            projected_reference_count,
            logical_output) ||
        !checked_add(
            logical_output,
            distinct_target_facet_count,
            logical_output) ||
        !checked_add(
            logical_output,
            representative_count,
            logical_output) ||
        !checked_add(
            logical_output,
            retained_key_point_references,
            logical_output)) {
      fail_plan(result, PlanDecision::no_plan_capacity_overflow);
      return result;
    }
    if (retained_key_point_references >
            budget.maximum_retained_key_point_reference_count ||
        logical_output > budget.maximum_logical_output_entry_count) {
      fail_plan(result, PlanDecision::no_plan_budget_exhausted);
      return result;
    }

    result.required_representative_binding_count = representative_count;
    result.required_projected_target_facet_reference_count =
        projected_reference_count;
    result.required_distinct_target_facet_count =
        distinct_target_facet_count;
    result.required_retained_key_point_reference_count =
        retained_key_point_references;
    result.required_logical_output_entry_count = logical_output;
    result.representatives.reserve(representative_count);
    result.target_facet_indices.reserve(projected_reference_count);
    result.canonical_distinct_target_facet_keys.reserve(
        distinct_target_facet_count);
    result.representative_indices_in_binding_order.reserve(
        representative_count);
    result.output_preallocated_after_budget_preflight = true;
    result.canonical_distinct_target_facet_keys.assign(
        projected_facets.begin(), unique_end);

    auto lookup_target_facet =
        [&](const ExactDirectSparseFacetKey& key)
        -> std::optional<std::size_t> {
      std::size_t first = 0U;
      std::size_t count =
          result.canonical_distinct_target_facet_keys.size();
      while (count != 0U) {
        if (result.counters.target_facet_lookup_comparison_count >=
            budget.maximum_target_facet_lookup_comparison_count) {
          return std::nullopt;
        }
        ++result.counters.target_facet_lookup_comparison_count;
        const std::size_t step = count / 2U;
        const std::size_t middle = first + step;
        if (facet_key_less(
                result.canonical_distinct_target_facet_keys[middle],
                key)) {
          first = middle + 1U;
          count -= step + 1U;
        } else {
          count = step;
        }
      }
      if (first >= result.canonical_distinct_target_facet_keys.size() ||
          result.canonical_distinct_target_facet_keys[first] != key) {
        return std::nullopt;
      }
      return first;
    };

    for (std::size_t representative_index = 0U;
         representative_index < representative_count;
         ++representative_index) {
      const std::size_t binding_index =
          representative_bindings[representative_index];
      const auto& source_key =
          source_forest.arm_root_bindings[binding_index].strict_arm_key;
      ExactDirectMorseVerticalTargetFacetPlanRepresentative record;
      record.representative_index = representative_index;
      record.representative_arm_root_binding_index = binding_index;
      record.source_strict_arm_key = source_key;
      record.target_facet_index_offset =
          result.target_facet_indices.size();
      record.target_facet_index_count = source_key.point_count;
      for (std::size_t erased_index = 0U;
           erased_index < source_key.point_count;
           ++erased_index) {
        const auto target_index =
            lookup_target_facet(erase_point(source_key, erased_index));
        if (!target_index.has_value()) {
          fail_plan(
              result,
              result.counters.target_facet_lookup_comparison_count >=
                      budget.maximum_target_facet_lookup_comparison_count
                  ? PlanDecision::no_plan_budget_exhausted
                  : PlanDecision::no_plan_group_rejected);
          return result;
        }
        result.target_facet_indices.push_back(*target_index);
      }
      result.representatives.push_back(record);
    }

    result.representative_indices_in_binding_order.resize(
        representative_count);
    std::iota(
        result.representative_indices_in_binding_order.begin(),
        result.representative_indices_in_binding_order.end(),
        0U);
    auto representative_binding_less =
        [&](std::size_t left, std::size_t right, bool& is_less) {
          if (result.counters.sort_comparison_count >=
              budget.maximum_sort_comparison_count) {
            return false;
          }
          ++result.counters.sort_comparison_count;
          is_less = result.representatives[left]
                        .representative_arm_root_binding_index <
                    result.representatives[right]
                        .representative_arm_root_binding_index;
          return true;
        };
    if (!counted_heap_sort(
            result.representative_indices_in_binding_order,
            representative_binding_less)) {
      fail_plan(result, PlanDecision::no_plan_budget_exhausted);
      return result;
    }

    result.source_group_ranges_revalidated = true;
    result.source_representatives_are_smallest_binding_indices = true;
    result.source_representatives_sorted_unique_by_full_key = true;
    result.every_source_key_projected_to_all_k_deletions = true;
    result.target_facets_canonical_distinct_and_sorted = true;
    result.target_facet_mapping_total = true;
    result.decision =
        PlanDecision::complete_group_local_canonical_target_facet_plan;
    result.scope = ExactDirectMorseVerticalTargetFacetPlanScope::
        one_source_atomic_group_k_keys_to_canonical_distinct_k_minus_one_deletions_only;
    return result;
  } catch (const std::bad_alloc&) {
    fail_plan(result, PlanDecision::no_plan_allocation_failed);
  } catch (const std::length_error&) {
    fail_plan(result, PlanDecision::no_plan_capacity_overflow);
  } catch (const std::exception&) {
    fail_plan(result, PlanDecision::no_plan_group_rejected);
  }
  return result;
}

bool ExactDirectMorseVerticalTargetFacetPlanResult::
    certified_group_local_target_facet_plan() const noexcept {
  if (schema_version !=
          direct_morse_vertical_target_proposal_adapter_schema_version ||
      decision !=
          PlanDecision::complete_group_local_canonical_target_facet_plan ||
      point_count == 0U || source_order < 2U ||
      source_order > point_count || target_order + 1U != source_order ||
      representatives.empty() ||
      representatives.size() != required_representative_binding_count ||
      target_facet_indices.size() !=
          required_projected_target_facet_reference_count ||
      canonical_distinct_target_facet_keys.size() !=
          required_distinct_target_facet_count ||
      representative_indices_in_binding_order.size() !=
          representatives.size() ||
      representatives.size() >
          requested_budget.maximum_representative_binding_count ||
      target_facet_indices.size() >
          requested_budget
              .maximum_projected_target_facet_reference_count ||
      canonical_distinct_target_facet_keys.size() >
          requested_budget.maximum_distinct_target_facet_count ||
      required_retained_key_point_reference_count >
          requested_budget.maximum_retained_key_point_reference_count ||
      required_logical_output_entry_count >
          requested_budget.maximum_logical_output_entry_count ||
      counters.group_saddle_scan_count >
          requested_budget.maximum_group_saddle_scan_count ||
      counters.group_arm_binding_scan_count >
          requested_budget.maximum_group_arm_binding_scan_count ||
      counters.group_arm_binding_scan_count >
          requested_budget.maximum_binding_sort_scratch_count ||
      counters.generated_target_facet_reference_count !=
          target_facet_indices.size() ||
      counters.sort_comparison_count >
          requested_budget.maximum_sort_comparison_count ||
      counters.target_facet_lookup_comparison_count >
          requested_budget.maximum_target_facet_lookup_comparison_count ||
      !source_forest_certified || !source_group_ranges_revalidated ||
      !source_representatives_are_smallest_binding_indices ||
      !source_representatives_sorted_unique_by_full_key ||
      !every_source_key_projected_to_all_k_deletions ||
      !target_facets_canonical_distinct_and_sorted ||
      !target_facet_mapping_total ||
      !output_preallocated_after_budget_preflight ||
      no_partial_payload_published_on_failure ||
      scope != ExactDirectMorseVerticalTargetFacetPlanScope::
                   one_source_atomic_group_k_keys_to_canonical_distinct_k_minus_one_deletions_only ||
      !plan_non_scope_honest(*this)) {
    return false;
  }

  std::vector<std::uint8_t> referenced_target_facets;
  try {
    referenced_target_facets.assign(
        canonical_distinct_target_facet_keys.size(), 0U);
  } catch (...) {
    return false;
  }
  std::size_t expected_mapping_offset = 0U;
  for (std::size_t index = 0U; index < representatives.size(); ++index) {
    const auto& representative = representatives[index];
    if (representative.representative_index != index ||
        !canonical_key_shape(
            representative.source_strict_arm_key,
            source_order,
            point_count) ||
        representative.target_facet_index_offset !=
            expected_mapping_offset ||
        representative.target_facet_index_count != source_order ||
        expected_mapping_offset > target_facet_indices.size() ||
        representative.target_facet_index_count >
            target_facet_indices.size() - expected_mapping_offset ||
        (index != 0U &&
         !facet_key_less(
             representatives[index - 1U].source_strict_arm_key,
             representative.source_strict_arm_key))) {
      return false;
    }
    for (std::size_t local = 0U;
         local < representative.target_facet_index_count;
         ++local) {
      const std::size_t target_index =
          target_facet_indices[expected_mapping_offset + local];
      if (target_index >= canonical_distinct_target_facet_keys.size() ||
          canonical_distinct_target_facet_keys[target_index] !=
              erase_point(representative.source_strict_arm_key, local)) {
        return false;
      }
      referenced_target_facets[target_index] = 1U;
    }
    expected_mapping_offset += representative.target_facet_index_count;
  }
  if (expected_mapping_offset != target_facet_indices.size()) {
    return false;
  }
  if (std::any_of(
          referenced_target_facets.begin(),
          referenced_target_facets.end(),
          [](std::uint8_t referenced) { return referenced == 0U; })) {
    return false;
  }
  for (std::size_t index = 0U;
       index < canonical_distinct_target_facet_keys.size();
       ++index) {
    if (!canonical_key_shape(
            canonical_distinct_target_facet_keys[index],
            target_order,
            point_count) ||
        (index != 0U &&
         !facet_key_less(
             canonical_distinct_target_facet_keys[index - 1U],
             canonical_distinct_target_facet_keys[index]))) {
      return false;
    }
  }
  std::optional<std::size_t> prior_binding;
  for (const std::size_t representative_index :
       representative_indices_in_binding_order) {
    if (representative_index >= representatives.size()) {
      return false;
    }
    const std::size_t binding =
        representatives[representative_index]
            .representative_arm_root_binding_index;
    if (prior_binding.has_value() && *prior_binding >= binding) {
      return false;
    }
    prior_binding = binding;
  }

  std::size_t source_key_refs = 0U;
  std::size_t target_key_refs = 0U;
  std::size_t retained_refs = 0U;
  std::size_t logical = 0U;
  return checked_multiply(
             representatives.size(), source_order, source_key_refs) &&
         checked_multiply(
             canonical_distinct_target_facet_keys.size(),
             target_order,
             target_key_refs) &&
         checked_add(source_key_refs, target_key_refs, retained_refs) &&
         checked_add(
             representatives.size(), target_facet_indices.size(), logical) &&
         checked_add(
             logical,
             canonical_distinct_target_facet_keys.size(),
             logical) &&
         checked_add(logical, representatives.size(), logical) &&
         checked_add(logical, retained_refs, logical) &&
         retained_refs == required_retained_key_point_reference_count &&
         logical == required_logical_output_entry_count;
}

bool ExactDirectMorseVerticalTargetFacetPlanResult::
    certified_atomic_failure() const noexcept {
  return schema_version ==
             direct_morse_vertical_target_proposal_adapter_schema_version &&
         decision != PlanDecision::not_certified &&
         decision !=
             PlanDecision::complete_group_local_canonical_target_facet_plan &&
         representatives.empty() && target_facet_indices.empty() &&
         canonical_distinct_target_facet_keys.empty() &&
         representative_indices_in_binding_order.empty() &&
         no_partial_payload_published_on_failure &&
         scope == ExactDirectMorseVerticalTargetFacetPlanScope::unspecified &&
         plan_non_scope_honest(*this);
}

bool ExactDirectMorseVerticalTargetFacetPlanResult::certified_outcome()
    const noexcept {
  return certified_group_local_target_facet_plan() ||
         certified_atomic_failure();
}

ExactDirectMorseVerticalTargetProposalAdapterResult
ExactDirectMorseForestCarrierCutReplayView::
    build_vertical_target_proposals_from_group_plan(
        const ExactDirectMorseForestJournalResult& source_forest,
        const ExactDirectMorseVerticalTargetFacetPlanResult& plan,
        const ExactDirectMorseForestCarrierCutClosureAdapterResult&
            closure_summary,
        const ExactDirectMorseVerticalTargetProposalAdapterBudget& budget)
        const {
  AdapterResult result;
  result.requested_budget = budget;
  if (session_ == nullptr) {
    fail_adapter(result, AdapterDecision::no_adapter_stale_view_epoch);
    return result;
  }
  if (!session_->view_epoch_is_live(visit_epoch_)) {
    fail_adapter(
        result,
        session_->poisoned()
            ? AdapterDecision::no_adapter_session_poisoned
            : AdapterDecision::no_adapter_stale_view_epoch);
    result.audit.session_poisoned = session_->poisoned();
    return result;
  }
  if (!session_->view_borrows_source_forest(
          visit_epoch_, &source_forest)) {
    fail_adapter(
        result, AdapterDecision::no_adapter_source_forest_identity_mismatch);
    return result;
  }
  result.audit.source_forest_borrow_identity_matches_session = true;

  AdapterBuildContext context{
      this,
      &source_forest,
      &plan,
      &closure_summary,
      &budget,
      &result};
  try {
    const auto visit = session_->view_with_frozen_locator(
        visit_epoch_, &context, build_proposals_with_frozen_locator);
    using VisitDecision = ExactDirectMorseForestCarrierCutReplaySession::
        FrozenLocatorViewDecision;
    switch (visit) {
      case VisitDecision::complete_stable_snapshot:
        result.audit.entry_and_post_locator_stamps_equal = true;
        break;
      case VisitDecision::session_not_ready:
        fail_adapter(result, AdapterDecision::no_adapter_session_not_ready);
        break;
      case VisitDecision::session_poisoned:
        fail_adapter(result, AdapterDecision::no_adapter_session_poisoned);
        result.audit.session_poisoned = true;
        break;
      case VisitDecision::stale_view_epoch:
        fail_adapter(result, AdapterDecision::no_adapter_stale_view_epoch);
        break;
      case VisitDecision::locator_snapshot_changed:
        fail_adapter(
            result, AdapterDecision::no_adapter_locator_snapshot_changed);
        result.audit.locator_state_mutated = true;
        result.audit.session_poisoned = true;
        break;
    }
  } catch (const std::bad_alloc&) {
    fail_adapter(result, AdapterDecision::no_adapter_allocation_failed);
  } catch (const std::length_error&) {
    fail_adapter(result, AdapterDecision::no_adapter_capacity_overflow);
  } catch (const std::exception&) {
    fail_adapter(result, AdapterDecision::no_adapter_source_group_mismatch);
  }
  return result;
}

bool ExactDirectMorseVerticalTargetProposalAdapterResult::
    certified_group_local_vertical_target_proposals() const noexcept {
  if (schema_version !=
          direct_morse_vertical_target_proposal_adapter_schema_version ||
      (decision !=
           AdapterDecision::
               complete_all_resolved_group_local_vertical_target_proposals &&
       decision !=
           AdapterDecision::
               complete_with_unresolved_group_local_vertical_target_proposals) ||
      point_count == 0U || source_order < 2U ||
      source_order > point_count || target_order + 1U != source_order ||
      external_target_authority_id == 0U || invocation_replay_token == 0U ||
      locator_snapshot_stamp.schema_version !=
          direct_sparse_positive_facet_locator_schema_version ||
      locator_snapshot_stamp.external_authority_id !=
          external_target_authority_id ||
      proposals.empty() || proposals.size() != input_representative_count ||
      input_distinct_target_facet_count == 0U ||
      required_logical_output_entry_count != proposals.size() ||
      proposals.size() > requested_budget.maximum_proposal_count ||
      required_logical_output_entry_count >
          requested_budget.maximum_logical_output_entry_count ||
      counters.source_saddle_revalidation_count >
          requested_budget.maximum_source_saddle_revalidation_count ||
      counters.source_binding_revalidation_count >
          requested_budget.maximum_source_binding_revalidation_count ||
      counters.source_key_lookup_comparison_count >
          requested_budget.maximum_source_key_lookup_comparison_count ||
      counters.closure_summary_scan_count !=
          input_distinct_target_facet_count ||
      counters.closure_summary_scan_count >
          requested_budget.maximum_closure_summary_scan_count ||
      counters.projected_target_facet_aggregation_count !=
          input_projected_target_facet_reference_count ||
      counters.positive_terminal_probe_count !=
          required_carrier_entry_revalidation_count ||
      counters.positive_terminal_probe_count >
          requested_budget.maximum_positive_terminal_probe_count ||
      counters.positive_terminal_probe_count >
          input_projected_target_facet_reference_count ||
      counters.positive_terminal_probe_slot_visit_count >
          requested_budget.maximum_positive_terminal_probe_slot_visit_count ||
      counters.positive_terminal_probe_parent_hop_count >
          requested_budget.maximum_positive_terminal_probe_parent_hop_count ||
      counters.positive_terminal_probe_full_key_comparison_count <
          counters.positive_terminal_probe_count ||
      counters.carrier_entry_revalidation_count !=
          required_carrier_entry_revalidation_count ||
      counters.carrier_entry_revalidation_count >
          requested_budget.maximum_carrier_entry_revalidation_count ||
      required_carrier_entry_revalidation_count >
          input_projected_target_facet_reference_count ||
      counters.target_node_lookup_count >
          requested_budget.maximum_target_node_lookup_count ||
      counters.exact_level_comparison_count >
          requested_budget.maximum_exact_level_comparison_count ||
      counters.maximum_observed_exact_level_integer_bit_count >
          requested_budget.maximum_single_exact_level_integer_bit_count ||
      counters.unresolved_proposal_count +
              counters.resolved_proposal_count !=
          proposals.size() ||
      !audit.view_epoch_checked_at_entry ||
      !audit.source_forest_certified ||
      !audit.source_forest_borrow_identity_matches_session ||
      !audit.plan_certified ||
      !audit.plan_revalidated_against_exact_source_group ||
      !audit.point_count_matches ||
      !audit.adjacent_target_order_matches ||
      !audit.exact_cut_matches_source_batch ||
      !audit.closure_summary_certified ||
      !audit.closure_snapshot_matches_live_view ||
      !audit.closure_sources_biject_canonical_target_facets ||
      !audit.every_positive_terminal_key_binding_reprobed ||
      !audit.output_preallocated_after_budget_preflight ||
      !audit.every_projected_target_facet_aggregated ||
      !audit.every_known_resolved_root_consistent_per_source_binding ||
      !audit.unresolved_or_latent_maps_only_to_unresolved ||
      !audit.all_resolved_unique_root_maps_to_target_seed ||
      !audit.common_invocation_replay_token_used ||
      !audit.proposals_sorted_unique_by_binding_index ||
      !audit.entry_and_post_locator_stamps_equal ||
      audit.no_partial_payload_published_on_failure ||
      audit.locator_state_mutated || audit.session_poisoned ||
      scope != ExactDirectMorseVerticalTargetProposalAdapterScope::
                   one_live_target_order_cut_one_source_atomic_group_compact_vertical_target_proposals_only ||
      !adapter_non_scope_honest(*this)) {
    return false;
  }
  std::size_t unresolved_count = 0U;
  std::size_t resolved_count = 0U;
  for (std::size_t index = 0U; index < proposals.size(); ++index) {
    const auto& proposal = proposals[index];
    if (proposal.replay_token != invocation_replay_token ||
        (index != 0U &&
         proposals[index - 1U].representative_arm_root_binding_index >=
             proposal.representative_arm_root_binding_index)) {
      return false;
    }
    if (proposal.disposition ==
        ExactDirectMorseVerticalProposalDisposition::unresolved) {
      if (proposal.target_seed_node_id.has_value()) {
        return false;
      }
      ++unresolved_count;
    } else if (
        proposal.disposition ==
        ExactDirectMorseVerticalProposalDisposition::resolved_target_seed) {
      if (!proposal.target_seed_node_id.has_value()) {
        return false;
      }
      ++resolved_count;
    } else {
      return false;
    }
  }
  const bool decision_matches =
      unresolved_count == 0U
          ? decision ==
                AdapterDecision::
                    complete_all_resolved_group_local_vertical_target_proposals
          : decision ==
                AdapterDecision::
                    complete_with_unresolved_group_local_vertical_target_proposals;
  std::size_t expected_exact_level_comparison_count = 0U;
  std::size_t maximum_possible_full_key_comparison_count = 0U;
  return checked_add(
             3U,
             resolved_count,
             expected_exact_level_comparison_count) &&
         checked_add(
             counters.positive_terminal_probe_slot_visit_count,
             counters.positive_terminal_probe_count,
             maximum_possible_full_key_comparison_count) &&
         counters.positive_terminal_probe_full_key_comparison_count <=
             maximum_possible_full_key_comparison_count &&
         decision_matches &&
         unresolved_count == counters.unresolved_proposal_count &&
         resolved_count == counters.resolved_proposal_count &&
         counters.target_node_lookup_count == resolved_count &&
         counters.exact_level_comparison_count ==
             expected_exact_level_comparison_count &&
         (unresolved_count != 0U ||
          required_carrier_entry_revalidation_count ==
              input_projected_target_facet_reference_count);
}

bool ExactDirectMorseVerticalTargetProposalAdapterResult::
    certified_atomic_rejection() const noexcept {
  const bool locator_change =
      decision == AdapterDecision::no_adapter_locator_snapshot_changed;
  const bool poison =
      locator_change ||
      decision == AdapterDecision::no_adapter_session_poisoned;
  return schema_version ==
             direct_morse_vertical_target_proposal_adapter_schema_version &&
         rejection_decision(decision) && proposals.empty() &&
         audit.no_partial_payload_published_on_failure &&
         audit.locator_state_mutated == locator_change &&
         audit.session_poisoned == poison &&
         scope ==
             ExactDirectMorseVerticalTargetProposalAdapterScope::unspecified &&
         adapter_non_scope_honest(*this);
}

bool ExactDirectMorseVerticalTargetProposalAdapterResult::
    certified_outcome() const noexcept {
  return certified_group_local_vertical_target_proposals() ||
         certified_atomic_rejection();
}

}  // namespace morsehgp3d::hierarchy
