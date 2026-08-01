#include "morsehgp3d/hierarchy/direct_morse_forest_carrier_cut_index.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include <boost/multiprecision/integer.hpp>

namespace morsehgp3d::hierarchy {
namespace {

enum class BuildFailure : std::uint8_t {
  capacity_overflow,
  allocation_failed,
  budget_exhausted,
  source_forest_rejected,
  invalid_target_order,
  invalid_closed_squared_level,
  replay_contradiction,
};

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

[[nodiscard]] std::size_t integer_bit_count(
    const exact::BigInt& value) {
  if (value == 0) {
    return 1U;
  }
  return static_cast<std::size_t>(boost::multiprecision::msb(value)) + 1U;
}

class LevelAccounting {
 public:
  LevelAccounting(
      const ExactDirectMorseForestCarrierCutIndexBudget& budget,
      ExactDirectMorseForestCarrierCutIndexCounters& counters) noexcept
      : budget_(budget), counters_(counters) {}

  [[nodiscard]] bool measure(const exact::ExactLevel& level) {
    const std::size_t observed = std::max(
        integer_bit_count(level.numerator()),
        integer_bit_count(level.denominator()));
    counters_.maximum_observed_exact_level_integer_bit_count = std::max(
        counters_.maximum_observed_exact_level_integer_bit_count,
        observed);
    return observed <=
           budget_.maximum_single_exact_level_integer_bit_count;
  }

  [[nodiscard]] bool compare(
      const exact::ExactLevel& left,
      const exact::ExactLevel& right) {
    if (counters_.exact_level_comparison_count >=
        budget_.maximum_exact_level_comparison_count) {
      exhausted_ = true;
      return false;
    }
    ++counters_.exact_level_comparison_count;
    relation_ = left < right ? -1 : (right < left ? 1 : 0);
    return true;
  }

  [[nodiscard]] int relation() const noexcept { return relation_; }
  [[nodiscard]] bool exhausted() const noexcept { return exhausted_; }

 private:
  const ExactDirectMorseForestCarrierCutIndexBudget& budget_;
  ExactDirectMorseForestCarrierCutIndexCounters& counters_;
  int relation_{};
  bool exhausted_{false};
};

struct ComponentState {
  std::size_t parent{};
  std::size_t order{};
  std::optional<ExactDirectMorseForestNodeId> reduced_root_node_id;
  std::size_t last_group_marker{};
  bool active{false};
};

struct NodeMarkerState {
  std::optional<ExactDirectSparseComponentHandle> owner_component_handle;
  std::size_t last_group_marker{};
};

enum class FindStatus : std::uint8_t {
  okay,
  budget_exhausted,
  contradiction,
};

struct FindResult {
  std::size_t root{};
  FindStatus status{FindStatus::contradiction};
};

[[nodiscard]] FindResult find_component_root(
    std::size_t handle,
    std::vector<ComponentState>& components,
    const ExactDirectMorseForestCarrierCutIndexBudget& budget,
    ExactDirectMorseForestCarrierCutIndexCounters& counters) noexcept {
  if (handle >= components.size() || !components[handle].active) {
    return {};
  }
  std::size_t root = handle;
  while (components[root].parent != root) {
    if (counters.parent_hop_count >= budget.maximum_parent_hop_count) {
      return {0U, FindStatus::budget_exhausted};
    }
    ++counters.parent_hop_count;
    const std::size_t next = components[root].parent;
    if (next >= components.size() || !components[next].active) {
      return {};
    }
    root = next;
  }
  std::size_t cursor = handle;
  while (components[cursor].parent != cursor) {
    if (counters.parent_hop_count >= budget.maximum_parent_hop_count) {
      return {0U, FindStatus::budget_exhausted};
    }
    ++counters.parent_hop_count;
    const std::size_t next = components[cursor].parent;
    components[cursor].parent = root;
    cursor = next;
  }
  return {root, FindStatus::okay};
}

void clear_payload(
    ExactDirectMorseForestCarrierCutIndexResult& result) noexcept {
  result.entries.clear();
  result.logical_output_entry_count = 0U;
  result.counters = {};
  result.target_order_birth_partition_reconstructed = false;
  result.target_batches_replayed_in_exact_level_order = false;
  result.frozen_carrier_groups_reconstructed = false;
  result.group_roots_resolved_before_current_level_births = false;
  result.unions_then_births_replayed_atomically = false;
  result.strict_pre_and_closed_post_counts_replayed = false;
  result.inactive_latent_and_resolved_carriers_distinguished = false;
  result.entries_sorted_unique_by_component_handle = false;
}

[[nodiscard]] ExactDirectMorseForestCarrierCutIndexResult fail(
    ExactDirectMorseForestCarrierCutIndexResult result,
    BuildFailure failure) noexcept {
  clear_payload(result);
  result.scope = ExactDirectMorseForestCarrierCutIndexScope::unspecified;
  result.no_partial_scientific_payload_published_on_failure = true;
  switch (failure) {
    case BuildFailure::capacity_overflow:
      result.decision = ExactDirectMorseForestCarrierCutIndexDecision::
          no_index_capacity_overflow;
      break;
    case BuildFailure::allocation_failed:
      result.decision = ExactDirectMorseForestCarrierCutIndexDecision::
          no_index_allocation_failed;
      break;
    case BuildFailure::budget_exhausted:
      result.decision = ExactDirectMorseForestCarrierCutIndexDecision::
          no_index_budget_exhausted;
      break;
    case BuildFailure::source_forest_rejected:
      result.decision = ExactDirectMorseForestCarrierCutIndexDecision::
          no_index_source_forest_rejected;
      break;
    case BuildFailure::invalid_target_order:
      result.decision = ExactDirectMorseForestCarrierCutIndexDecision::
          no_index_invalid_target_order;
      break;
    case BuildFailure::invalid_closed_squared_level:
      result.decision = ExactDirectMorseForestCarrierCutIndexDecision::
          no_index_invalid_closed_squared_level;
      break;
    case BuildFailure::replay_contradiction:
      result.decision = ExactDirectMorseForestCarrierCutIndexDecision::
          no_index_replay_contradiction;
      break;
  }
  return result;
}

[[nodiscard]] bool result_facts_honest(
    const ExactDirectMorseForestCarrierCutIndexResult& result) noexcept {
  return result.conditional_on_caller_fresh_source_forest_replay &&
         result.forest_relative_only &&
         !result.external_locator_authority_replayed &&
         !result.original_geometry_replayed &&
         !result.global_morse_obligation_replayed &&
         !result.vertical_naturality_replayed &&
         !result.gamma_cells_or_global_cofaces_materialized &&
         !result.higher_order_delaunay_materialized &&
         !result.forbidden_global_structure_materialized &&
         !result.public_status_claimed;
}

[[nodiscard]] bool storage_within(
    const ExactDirectMorseForestCarrierCutIndexResult& result,
    const ExactDirectMorseForestCarrierCutIndexBudget& budget) noexcept {
  return result.entries.size() <= budget.maximum_index_entry_count &&
         result.logical_output_entry_count <=
             budget.maximum_logical_output_entry_count &&
         result.counters.forest_birth_record_scan_count <=
             budget.maximum_forest_birth_record_scan_count &&
         result.counters.forest_node_scan_count <=
             budget.maximum_forest_node_scan_count &&
         result.counters.forest_batch_scan_count <=
             budget.maximum_forest_batch_scan_count &&
         result.counters.replayed_atomic_group_count <=
             budget.maximum_forest_atomic_group_scan_count &&
         result.counters.replayed_saddle_count <=
             budget.maximum_forest_saddle_scan_count &&
         result.counters.replayed_arm_binding_count <=
             budget.maximum_forest_arm_binding_scan_count &&
         result.counters.replayed_child_reference_count <=
             budget.maximum_forest_child_reference_scan_count &&
         result.counters.forest_final_root_scan_count <=
             budget.maximum_forest_final_root_scan_count &&
         result.counters.component_state_count <=
             budget.maximum_component_state_count &&
         result.counters.node_marker_state_count <=
             budget.maximum_node_marker_state_count &&
         result.counters.maximum_group_carrier_scratch_count <=
             budget.maximum_group_carrier_scratch_count &&
         result.counters.maximum_group_prior_root_scratch_count <=
             budget.maximum_group_prior_root_scratch_count &&
         result.counters.parent_hop_count <=
             budget.maximum_parent_hop_count &&
         result.counters.exact_level_comparison_count <=
             budget.maximum_exact_level_comparison_count &&
         result.counters.maximum_observed_exact_level_integer_bit_count <=
             budget.maximum_single_exact_level_integer_bit_count;
}

[[nodiscard]] bool entry_shape(
    const ExactDirectMorseForestCarrierCutIndexResult& result) noexcept {
  std::size_t partition_count = 0U;
  if (!checked_add(
          result.counters.inactive_carrier_count,
          result.counters.active_latent_carrier_count,
          partition_count) ||
      !checked_add(
          partition_count,
          result.counters.resolved_reduced_root_carrier_count,
          partition_count) ||
      partition_count != result.entries.size() ||
      result.counters.target_carrier_count != result.entries.size() ||
      result.logical_output_entry_count != result.entries.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < result.entries.size(); ++index) {
    const auto& entry = result.entries[index];
    if (entry.entry_index != index ||
        entry.component_handle != entry.birth_record_index ||
        (index != 0U &&
         result.entries[index - 1U].component_handle >=
             entry.component_handle)) {
      return false;
    }
    switch (entry.disposition) {
      case ExactDirectMorseForestCarrierCutDisposition::
          inactive_at_closed_cut:
      case ExactDirectMorseForestCarrierCutDisposition::
          active_latent_without_reduced_root:
        if (entry.reduced_root_node_id.has_value()) {
          return false;
        }
        break;
      case ExactDirectMorseForestCarrierCutDisposition::
          resolved_reduced_root:
        if (!entry.reduced_root_node_id.has_value()) {
          return false;
        }
        break;
      default:
        return false;
    }
  }
  return true;
}

[[nodiscard]] bool source_storage_preflight(
    const ExactDirectMorseForestJournalResult& forest,
    const ExactDirectMorseForestCarrierCutIndexBudget& budget,
    std::size_t& birth_record_count,
    std::size_t& node_count,
    BuildFailure& failure) noexcept {
  if (!checked_add(
          forest.implicit_order_one_prefix_count,
          forest.birth_records.size(),
          birth_record_count) ||
      !checked_add(
          forest.implicit_order_one_prefix_count,
          forest.nodes.size(),
          node_count)) {
    failure = BuildFailure::capacity_overflow;
    return false;
  }
  if (birth_record_count >
          budget.maximum_forest_birth_record_scan_count ||
      node_count > budget.maximum_forest_node_scan_count ||
      forest.batches.size() > budget.maximum_forest_batch_scan_count ||
      forest.atomic_groups.size() >
          budget.maximum_forest_atomic_group_scan_count ||
      forest.saddle_records.size() >
          budget.maximum_forest_saddle_scan_count ||
      forest.arm_root_bindings.size() >
          budget.maximum_forest_arm_binding_scan_count ||
      forest.child_node_ids.size() >
          budget.maximum_forest_child_reference_scan_count ||
      forest.final_roots.size() >
          budget.maximum_forest_final_root_scan_count ||
      birth_record_count > budget.maximum_component_state_count ||
      node_count > budget.maximum_node_marker_state_count) {
    failure = BuildFailure::budget_exhausted;
    return false;
  }
  return true;
}

[[nodiscard]] bool add_counter(
    std::size_t increment,
    std::size_t& counter) noexcept {
  return checked_add(counter, increment, counter);
}

[[nodiscard]] bool node_matches_order(
    const ExactDirectMorseForestJournalView& view,
    std::size_t node_count,
    ExactDirectMorseForestNodeId node_id,
    std::size_t order) {
  return node_id < node_count && view.node_at(node_id).order == order;
}

}  // namespace

const ExactDirectMorseForestCarrierCutEntry*
ExactDirectMorseForestCarrierCutIndexResult::find_entry(
    ExactDirectSparseComponentHandle component_handle) const noexcept {
  const auto found = std::lower_bound(
      entries.begin(),
      entries.end(),
      component_handle,
      [](const ExactDirectMorseForestCarrierCutEntry& entry,
         ExactDirectSparseComponentHandle handle) {
        return entry.component_handle < handle;
      });
  return found != entries.end() &&
             found->component_handle == component_handle
         ? &*found
         : nullptr;
}

bool ExactDirectMorseForestCarrierCutIndexResult::
    certified_forest_relative_closed_cut_index() const noexcept {
  return schema_version ==
             direct_morse_forest_carrier_cut_index_schema_version &&
         decision == ExactDirectMorseForestCarrierCutIndexDecision::
                         complete_certified_forest_relative_closed_cut_index &&
         target_order != 0U &&
         target_order <= effective_maximum_order &&
         source_forest_certified_outcome_accepted &&
         budget_preflight_certified &&
         target_order_birth_partition_reconstructed &&
         target_batches_replayed_in_exact_level_order &&
         frozen_carrier_groups_reconstructed &&
         group_roots_resolved_before_current_level_births &&
         unions_then_births_replayed_atomically &&
         strict_pre_and_closed_post_counts_replayed &&
         inactive_latent_and_resolved_carriers_distinguished &&
         entries_sorted_unique_by_component_handle &&
         !no_partial_scientific_payload_published_on_failure &&
         scope == ExactDirectMorseForestCarrierCutIndexScope::
                      one_target_order_forest_carriers_to_optional_reduced_roots_at_one_closed_exact_cut_only &&
         storage_within(*this, requested_budget) && entry_shape(*this) &&
         result_facts_honest(*this);
}

bool ExactDirectMorseForestCarrierCutIndexResult::certified_atomic_failure()
    const noexcept {
  return schema_version ==
             direct_morse_forest_carrier_cut_index_schema_version &&
         decision !=
             ExactDirectMorseForestCarrierCutIndexDecision::not_certified &&
         decision != ExactDirectMorseForestCarrierCutIndexDecision::
                         complete_certified_forest_relative_closed_cut_index &&
         entries.empty() && logical_output_entry_count == 0U &&
         counters == ExactDirectMorseForestCarrierCutIndexCounters{} &&
         no_partial_scientific_payload_published_on_failure &&
         scope == ExactDirectMorseForestCarrierCutIndexScope::unspecified &&
         result_facts_honest(*this);
}

bool ExactDirectMorseForestCarrierCutIndexResult::certified_outcome()
    const noexcept {
  return certified_forest_relative_closed_cut_index() ||
         certified_atomic_failure();
}

ExactDirectMorseForestCarrierCutIndexResult
build_exact_direct_morse_forest_carrier_cut_index(
    const ExactDirectMorseForestJournalResult& source_forest,
    std::size_t target_order,
    const exact::ExactLevel& closed_squared_level,
    const ExactDirectMorseForestCarrierCutIndexBudget& budget) {
  ExactDirectMorseForestCarrierCutIndexResult result;
  result.requested_budget = budget;
  result.point_count = source_forest.point_count;
  result.effective_maximum_order = source_forest.effective_maximum_order;
  result.target_order = target_order;
  result.closed_squared_level = closed_squared_level;

  if (target_order == 0U ||
      target_order > source_forest.effective_maximum_order) {
    return fail(std::move(result), BuildFailure::invalid_target_order);
  }
  if (closed_squared_level.numerator() < 0 ||
      closed_squared_level.denominator() <= 0) {
    return fail(
        std::move(result), BuildFailure::invalid_closed_squared_level);
  }
  if (source_forest.schema_version !=
          direct_morse_forest_journal_schema_version ||
      source_forest.decision != ExactDirectMorseForestDecision::
                                    complete_conditional_exact_direct_morse_forest) {
    return fail(std::move(result), BuildFailure::source_forest_rejected);
  }

  std::size_t birth_record_count = 0U;
  std::size_t node_count = 0U;
  BuildFailure preflight_failure = BuildFailure::budget_exhausted;
  if (!source_storage_preflight(
          source_forest,
          budget,
          birth_record_count,
          node_count,
          preflight_failure)) {
    return fail(std::move(result), preflight_failure);
  }

  try {
    if (!source_forest.certified_conditional_h0_candidate()) {
      return fail(std::move(result), BuildFailure::source_forest_rejected);
    }
    result.source_forest_certified_outcome_accepted = true;
    result.counters.forest_final_root_scan_count =
        source_forest.final_roots.size();

    const ExactDirectMorseForestJournalView forest_view{source_forest};
    LevelAccounting levels{budget, result.counters};
    if (!levels.measure(closed_squared_level)) {
      return fail(std::move(result), BuildFailure::budget_exhausted);
    }

    std::vector<ComponentState> components(birth_record_count);
    std::vector<NodeMarkerState> node_markers(node_count);
    std::vector<ExactDirectSparseComponentHandle> target_handles;
    target_handles.reserve(std::min(
        birth_record_count, budget.maximum_index_entry_count));
    result.counters.component_state_count = birth_record_count;
    result.counters.node_marker_state_count = node_count;

    for (std::size_t index = 0U; index < birth_record_count; ++index) {
      const auto birth = forest_view.birth_record_at(index);
      ++result.counters.forest_birth_record_scan_count;
      if (birth.birth_record_index != index ||
          birth.component_handle != index || birth.order == 0U ||
          birth.order > source_forest.effective_maximum_order ||
          birth.source_journal_batch_index >= source_forest.batches.size() ||
          source_forest.batches[birth.source_journal_batch_index].order !=
              birth.order ||
          (birth.order == 1U) !=
              birth.order_one_birth_node_id.has_value()) {
        return fail(std::move(result), BuildFailure::replay_contradiction);
      }
      components[index].parent = index;
      components[index].order = birth.order;
      if (birth.order == target_order) {
        if (target_handles.size() >= budget.maximum_index_entry_count ||
            target_handles.size() >=
                budget.maximum_logical_output_entry_count) {
          return fail(std::move(result), BuildFailure::budget_exhausted);
        }
        target_handles.push_back(index);
      }
    }

    for (std::size_t index = 0U; index < node_count; ++index) {
      const auto node = forest_view.node_at(
          static_cast<ExactDirectMorseForestNodeId>(index));
      ++result.counters.forest_node_scan_count;
      if (node.node_id !=
              static_cast<ExactDirectMorseForestNodeId>(index) ||
          node.order == 0U ||
          node.order > source_forest.effective_maximum_order) {
        return fail(std::move(result), BuildFailure::replay_contradiction);
      }
      if (!levels.measure(node.squared_level)) {
        return fail(std::move(result), BuildFailure::budget_exhausted);
      }
    }
    for (const auto& batch : source_forest.batches) {
      if (!levels.measure(batch.squared_level)) {
        return fail(std::move(result), BuildFailure::budget_exhausted);
      }
    }

    result.counters.target_carrier_count = target_handles.size();
    result.budget_preflight_certified = true;
    result.target_order_birth_partition_reconstructed = true;

    std::size_t active_carrier_root_count = 0U;
    std::size_t active_reduced_root_count = 0U;

    for (std::size_t batch_index = 0U;
         batch_index < source_forest.batches.size();
         ++batch_index) {
      const auto& batch = source_forest.batches[batch_index];
      ++result.counters.forest_batch_scan_count;
      if (batch.batch_index != batch_index ||
          batch.order == 0U ||
          batch.order > source_forest.effective_maximum_order) {
        return fail(std::move(result), BuildFailure::replay_contradiction);
      }
      if (batch.order != target_order) {
        continue;
      }
      if (!levels.compare(batch.squared_level, closed_squared_level)) {
        return fail(std::move(result), BuildFailure::budget_exhausted);
      }
      if (levels.relation() > 0) {
        continue;
      }
      if (batch.strict_pre_batch_carrier_count !=
              active_carrier_root_count ||
          batch.strict_pre_batch_reduced_root_count !=
              active_reduced_root_count ||
          batch.atomic_group_offset > source_forest.atomic_groups.size() ||
          batch.atomic_group_count >
              source_forest.atomic_groups.size() -
                  batch.atomic_group_offset ||
          batch.birth_record_offset > birth_record_count ||
          batch.birth_record_count >
              birth_record_count - batch.birth_record_offset) {
        return fail(std::move(result), BuildFailure::replay_contradiction);
      }

      for (std::size_t local_group = 0U;
           local_group < batch.atomic_group_count;
           ++local_group) {
        const std::size_t group_index =
            batch.atomic_group_offset + local_group;
        const auto& group = source_forest.atomic_groups[group_index];
        if (group_index == std::numeric_limits<std::size_t>::max() ||
            group.atomic_group_index != group_index ||
            group.batch_index != batch_index ||
            group.saddle_record_count == 0U ||
            group.saddle_record_offset >
                source_forest.saddle_records.size() ||
            group.saddle_record_count >
                source_forest.saddle_records.size() -
                    group.saddle_record_offset ||
            group.frozen_carrier_count >
                budget.maximum_group_carrier_scratch_count ||
            group.prior_reduced_root_count >
                budget.maximum_group_prior_root_scratch_count) {
          return fail(
              std::move(result),
              group.frozen_carrier_count >
                          budget.maximum_group_carrier_scratch_count ||
                      group.prior_reduced_root_count >
                          budget.maximum_group_prior_root_scratch_count
                  ? BuildFailure::budget_exhausted
                  : BuildFailure::replay_contradiction);
        }
        ++result.counters.replayed_atomic_group_count;
        const std::size_t group_marker = group_index + 1U;
        std::vector<ExactDirectSparseComponentHandle> group_carriers;
        std::vector<ExactDirectMorseForestNodeId> group_prior_roots;
        group_carriers.reserve(group.frozen_carrier_count);
        group_prior_roots.reserve(group.prior_reduced_root_count);

        std::size_t group_latent_carrier_count = 0U;
        for (std::size_t local_saddle = 0U;
             local_saddle < group.saddle_record_count;
             ++local_saddle) {
          const std::size_t saddle_index =
              group.saddle_record_offset + local_saddle;
          const auto& saddle = source_forest.saddle_records[saddle_index];
          ++result.counters.replayed_saddle_count;
          if (saddle.saddle_record_index != saddle_index ||
              saddle.atomic_group_index != group_index ||
              saddle.source_journal_batch_index != batch_index ||
              saddle.arm_binding_count == 0U ||
              saddle.arm_binding_count > 4U ||
              saddle.arm_binding_offset >
                  source_forest.arm_root_bindings.size() ||
              saddle.arm_binding_count >
                  source_forest.arm_root_bindings.size() -
                      saddle.arm_binding_offset) {
            return fail(
                std::move(result), BuildFailure::replay_contradiction);
          }

          std::array<ExactDirectSparseComponentHandle, 4U>
              saddle_carriers{};
          std::array<ExactDirectMorseForestNodeId, 4U> saddle_roots{};
          std::size_t saddle_carrier_count = 0U;
          std::size_t saddle_root_count = 0U;

          for (std::size_t local_binding = 0U;
               local_binding < saddle.arm_binding_count;
               ++local_binding) {
            const std::size_t binding_index =
                saddle.arm_binding_offset + local_binding;
            const auto& binding =
                source_forest.arm_root_bindings[binding_index];
            ++result.counters.replayed_arm_binding_count;
            const std::size_t handle =
                binding.frozen_carrier_component_handle;
            if (binding.binding_index != binding_index ||
                binding.source_family_index !=
                    saddle.source_family_index ||
                handle >= components.size() ||
                components[handle].order != target_order ||
                !components[handle].active) {
              return fail(
                  std::move(result), BuildFailure::replay_contradiction);
            }
            const FindResult found = find_component_root(
                handle, components, budget, result.counters);
            if (found.status == FindStatus::budget_exhausted) {
              return fail(
                  std::move(result), BuildFailure::budget_exhausted);
            }
            if (found.status != FindStatus::okay || found.root != handle ||
                binding.prior_reduced_root_node_id !=
                    components[handle].reduced_root_node_id) {
              return fail(
                  std::move(result), BuildFailure::replay_contradiction);
            }

            bool saddle_carrier_seen = false;
            for (std::size_t prior = 0U;
                 prior < saddle_carrier_count;
                 ++prior) {
              saddle_carrier_seen =
                  saddle_carrier_seen || saddle_carriers[prior] == handle;
            }
            if (!saddle_carrier_seen) {
              saddle_carriers[saddle_carrier_count] = handle;
              ++saddle_carrier_count;
            }

            if (components[handle].last_group_marker != group_marker) {
              const std::size_t prior_marker =
                  components[handle].last_group_marker;
              if (prior_marker != 0U) {
                const std::size_t prior_group_index = prior_marker - 1U;
                if (prior_group_index >= source_forest.atomic_groups.size() ||
                    source_forest.atomic_groups[prior_group_index]
                            .batch_index == batch_index) {
                  return fail(
                      std::move(result),
                      BuildFailure::replay_contradiction);
                }
              }
              if (group_carriers.size() >=
                  group.frozen_carrier_count) {
                return fail(
                    std::move(result),
                    BuildFailure::replay_contradiction);
              }
              components[handle].last_group_marker = group_marker;
              group_carriers.push_back(handle);
              if (!components[handle].reduced_root_node_id.has_value()) {
                ++group_latent_carrier_count;
              }
            }

            if (binding.prior_reduced_root_node_id.has_value()) {
              const ExactDirectMorseForestNodeId root =
                  *binding.prior_reduced_root_node_id;
              if (root >= node_count ||
                  node_markers[root].owner_component_handle !=
                      std::optional<ExactDirectSparseComponentHandle>{
                          handle}) {
                return fail(
                    std::move(result), BuildFailure::replay_contradiction);
              }
              bool saddle_root_seen = false;
              for (std::size_t prior = 0U;
                   prior < saddle_root_count;
                   ++prior) {
                saddle_root_seen =
                    saddle_root_seen || saddle_roots[prior] == root;
              }
              if (!saddle_root_seen) {
                saddle_roots[saddle_root_count] = root;
                ++saddle_root_count;
              }
              if (node_markers[root].last_group_marker != group_marker) {
                if (group_prior_roots.size() >=
                    group.prior_reduced_root_count) {
                  return fail(
                      std::move(result),
                      BuildFailure::replay_contradiction);
                }
                node_markers[root].last_group_marker = group_marker;
                group_prior_roots.push_back(root);
              }
            }
          }
          if (saddle.distinct_frozen_carrier_count !=
                  saddle_carrier_count ||
              saddle.distinct_prior_reduced_root_count !=
                  saddle_root_count ||
              saddle.distinct_latent_carrier_count !=
                  saddle_carrier_count - saddle_root_count) {
            return fail(
                std::move(result), BuildFailure::replay_contradiction);
          }
        }

        if (group_carriers.empty() ||
            group.frozen_carrier_count != group_carriers.size() ||
            group.latent_carrier_count != group_latent_carrier_count ||
            group.prior_reduced_root_count != group_prior_roots.size() ||
            group.latent_carrier_count !=
                group.frozen_carrier_count -
                    group.prior_reduced_root_count) {
          return fail(
              std::move(result), BuildFailure::replay_contradiction);
        }
        if (!add_counter(
                group_carriers.size(),
                result.counters.unique_group_carrier_count) ||
            !add_counter(
                group_prior_roots.size(),
                result.counters.unique_group_prior_root_count)) {
          return fail(std::move(result), BuildFailure::capacity_overflow);
        }
        result.counters.maximum_group_carrier_scratch_count = std::max(
            result.counters.maximum_group_carrier_scratch_count,
            group_carriers.size());
        result.counters.maximum_group_prior_root_scratch_count = std::max(
            result.counters.maximum_group_prior_root_scratch_count,
            group_prior_roots.size());

        for (const ExactDirectMorseForestNodeId root : group_prior_roots) {
          const auto prior_node = forest_view.node_at(root);
          if (prior_node.order != target_order ||
              !levels.compare(
                  prior_node.squared_level, batch.squared_level)) {
            return fail(
                std::move(result),
                levels.exhausted() ? BuildFailure::budget_exhausted
                                   : BuildFailure::replay_contradiction);
          }
          if (levels.relation() >= 0) {
            return fail(
                std::move(result), BuildFailure::replay_contradiction);
          }
        }

        const auto validate_created_node =
            [&](ExactDirectMorseForestNodeKind expected_kind) -> bool {
          if (!group.created_node_id.has_value() ||
              *group.created_node_id != group.resulting_root_node_id ||
              group.resulting_root_node_id >= node_count) {
            return false;
          }
          const auto created =
              forest_view.node_at(group.resulting_root_node_id);
          if (created.order != target_order ||
              created.kind != expected_kind ||
              created.atomic_group_index !=
                  std::optional<std::size_t>{group_index} ||
              created.child_offset != group.child_offset ||
              created.child_count != group.child_count ||
              !levels.compare(created.squared_level, batch.squared_level) ||
              levels.relation() != 0) {
            return false;
          }
          return true;
        };

        switch (group.kind) {
          case ExactDirectMorseForestAtomicGroupKind::reduced_birth:
            if (!group_prior_roots.empty() ||
                group_latent_carrier_count == 0U ||
                group.child_count != 0U ||
                !validate_created_node(
                    ExactDirectMorseForestNodeKind::reduced_birth)) {
              return fail(
                  std::move(result),
                  levels.exhausted() ? BuildFailure::budget_exhausted
                                     : BuildFailure::replay_contradiction);
            }
            break;
          case ExactDirectMorseForestAtomicGroupKind::continuation:
            if (group_prior_roots.size() != 1U ||
                group.child_count != 0U ||
                group.created_node_id.has_value() ||
                group.resulting_root_node_id != group_prior_roots.front()) {
              return fail(
                  std::move(result), BuildFailure::replay_contradiction);
            }
            break;
          case ExactDirectMorseForestAtomicGroupKind::multifusion:
            if (group_prior_roots.size() < 2U ||
                group.child_offset > source_forest.child_node_ids.size() ||
                group.child_count >
                    source_forest.child_node_ids.size() -
                        group.child_offset ||
                group.child_count != group_prior_roots.size() ||
                !validate_created_node(
                    ExactDirectMorseForestNodeKind::multifusion)) {
              return fail(
                  std::move(result),
                  levels.exhausted() ? BuildFailure::budget_exhausted
                                     : BuildFailure::replay_contradiction);
            }
            for (std::size_t local_child = 0U;
                 local_child < group.child_count;
                 ++local_child) {
              const ExactDirectMorseForestNodeId child =
                  source_forest.child_node_ids[
                      group.child_offset + local_child];
              ++result.counters.replayed_child_reference_count;
              if (child >= node_count ||
                  node_markers[child].last_group_marker != group_marker) {
                return fail(
                    std::move(result),
                    BuildFailure::replay_contradiction);
              }
            }
            break;
          default:
            return fail(
                std::move(result), BuildFailure::replay_contradiction);
        }

        if (active_reduced_root_count < group_prior_roots.size()) {
          return fail(
              std::move(result), BuildFailure::replay_contradiction);
        }
        for (const ExactDirectMorseForestNodeId root : group_prior_roots) {
          node_markers[root].owner_component_handle.reset();
        }
        const std::size_t canonical_handle = *std::min_element(
            group_carriers.begin(), group_carriers.end());
        for (const std::size_t handle : group_carriers) {
          components[handle].reduced_root_node_id.reset();
          if (handle != canonical_handle) {
            components[handle].parent = canonical_handle;
            if (active_carrier_root_count == 0U) {
              return fail(
                  std::move(result),
                  BuildFailure::replay_contradiction);
            }
            --active_carrier_root_count;
            ++result.counters.carrier_union_count;
          }
        }
        if (group.resulting_root_node_id >= node_count ||
            node_markers[group.resulting_root_node_id]
                .owner_component_handle.has_value()) {
          return fail(
              std::move(result), BuildFailure::replay_contradiction);
        }
        components[canonical_handle].reduced_root_node_id =
            group.resulting_root_node_id;
        node_markers[group.resulting_root_node_id]
            .owner_component_handle = canonical_handle;
        active_reduced_root_count -= group_prior_roots.size();
        ++active_reduced_root_count;
      }

      for (std::size_t local_birth = 0U;
           local_birth < batch.birth_record_count;
           ++local_birth) {
        const std::size_t birth_index =
            batch.birth_record_offset + local_birth;
        const auto birth = forest_view.birth_record_at(birth_index);
        const std::size_t handle = birth.component_handle;
        if (birth.order != target_order || handle >= components.size() ||
            components[handle].active ||
            components[handle].parent != handle) {
          return fail(
              std::move(result), BuildFailure::replay_contradiction);
        }
        components[handle].active = true;
        ++active_carrier_root_count;
        if (target_order == 1U) {
          if (!birth.order_one_birth_node_id.has_value() ||
              *birth.order_one_birth_node_id >= node_count) {
            return fail(
                std::move(result), BuildFailure::replay_contradiction);
          }
          const auto birth_node =
              forest_view.node_at(*birth.order_one_birth_node_id);
          if (birth_node.node_id !=
                  static_cast<ExactDirectMorseForestNodeId>(handle) ||
              birth_node.order != 1U ||
              birth_node.kind !=
                  ExactDirectMorseForestNodeKind::order_one_birth ||
              birth_node.birth_record_index !=
                  std::optional<std::size_t>{birth_index} ||
              birth_node.child_count != 0U ||
              !levels.compare(
                  birth_node.squared_level, batch.squared_level) ||
              levels.relation() != 0 ||
              node_markers[birth_node.node_id]
                  .owner_component_handle.has_value()) {
            return fail(
                std::move(result),
                levels.exhausted() ? BuildFailure::budget_exhausted
                                   : BuildFailure::replay_contradiction);
          }
          components[handle].reduced_root_node_id = birth_node.node_id;
          node_markers[birth_node.node_id].owner_component_handle = handle;
          ++active_reduced_root_count;
        } else if (birth.order_one_birth_node_id.has_value()) {
          return fail(
              std::move(result), BuildFailure::replay_contradiction);
        }
      }

      if (batch.closed_post_batch_carrier_count !=
              active_carrier_root_count ||
          batch.closed_post_batch_reduced_root_count !=
              active_reduced_root_count) {
        return fail(std::move(result), BuildFailure::replay_contradiction);
      }
      ++result.counters.replayed_target_batch_count;
    }

    result.entries.reserve(target_handles.size());
    for (const std::size_t handle : target_handles) {
      ExactDirectMorseForestCarrierCutEntry entry;
      entry.entry_index = result.entries.size();
      entry.component_handle = handle;
      entry.birth_record_index = handle;
      if (!components[handle].active) {
        entry.disposition = ExactDirectMorseForestCarrierCutDisposition::
            inactive_at_closed_cut;
        ++result.counters.inactive_carrier_count;
      } else {
        const FindResult found = find_component_root(
            handle, components, budget, result.counters);
        if (found.status == FindStatus::budget_exhausted) {
          return fail(std::move(result), BuildFailure::budget_exhausted);
        }
        if (found.status != FindStatus::okay ||
            found.root >= components.size()) {
          return fail(
              std::move(result), BuildFailure::replay_contradiction);
        }
        const auto root_node_id =
            components[found.root].reduced_root_node_id;
        if (!root_node_id.has_value()) {
          entry.disposition = ExactDirectMorseForestCarrierCutDisposition::
              active_latent_without_reduced_root;
          ++result.counters.active_latent_carrier_count;
        } else {
          if (!node_matches_order(
                  forest_view,
                  node_count,
                  *root_node_id,
                  target_order) ||
              node_markers[*root_node_id].owner_component_handle !=
                  std::optional<ExactDirectSparseComponentHandle>{
                      found.root}) {
            return fail(
                std::move(result), BuildFailure::replay_contradiction);
          }
          const auto root_node = forest_view.node_at(*root_node_id);
          if (!levels.compare(
                  root_node.squared_level, closed_squared_level)) {
            return fail(std::move(result), BuildFailure::budget_exhausted);
          }
          if (levels.relation() > 0) {
            return fail(
                std::move(result), BuildFailure::replay_contradiction);
          }
          entry.reduced_root_node_id = root_node_id;
          entry.disposition = ExactDirectMorseForestCarrierCutDisposition::
              resolved_reduced_root;
          ++result.counters.resolved_reduced_root_carrier_count;
        }
      }
      result.entries.push_back(entry);
    }

    result.logical_output_entry_count = result.entries.size();
    result.target_batches_replayed_in_exact_level_order = true;
    result.frozen_carrier_groups_reconstructed = true;
    result.group_roots_resolved_before_current_level_births = true;
    result.unions_then_births_replayed_atomically = true;
    result.strict_pre_and_closed_post_counts_replayed = true;
    result.inactive_latent_and_resolved_carriers_distinguished = true;
    result.entries_sorted_unique_by_component_handle = true;
    result.scope = ExactDirectMorseForestCarrierCutIndexScope::
        one_target_order_forest_carriers_to_optional_reduced_roots_at_one_closed_exact_cut_only;
    result.decision = ExactDirectMorseForestCarrierCutIndexDecision::
        complete_certified_forest_relative_closed_cut_index;
    if (!result.certified_forest_relative_closed_cut_index()) {
      return fail(std::move(result), BuildFailure::replay_contradiction);
    }
    return result;
  } catch (const std::bad_alloc&) {
    return fail(std::move(result), BuildFailure::allocation_failed);
  } catch (const std::length_error&) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  } catch (const std::exception&) {
    return fail(std::move(result), BuildFailure::replay_contradiction);
  }
}

ExactDirectMorseForestCarrierCutIndexVerification
verify_exact_direct_morse_forest_carrier_cut_index(
    const ExactDirectMorseForestJournalResult& source_forest,
    std::size_t target_order,
    const exact::ExactLevel& closed_squared_level,
    const ExactDirectMorseForestCarrierCutIndexBudget& trusted_budget,
    const ExactDirectMorseForestCarrierCutIndexResult& observed) {
  ExactDirectMorseForestCarrierCutIndexVerification verification;
  verification.observed_storage_within_budget =
      storage_within(observed, trusted_budget);
  const auto expected = build_exact_direct_morse_forest_carrier_cut_index(
      source_forest,
      target_order,
      closed_squared_level,
      trusted_budget);
  verification.trusted_source_forest_outcome_accepted =
      expected.source_forest_certified_outcome_accepted;
  verification.expected_index_freshly_reconstructed =
      expected.certified_outcome();
  verification.observed_recursively_equal = observed == expected;
  verification.result_certified =
      verification.trusted_source_forest_outcome_accepted &&
      verification.observed_storage_within_budget &&
      verification.expected_index_freshly_reconstructed &&
      verification.observed_recursively_equal &&
      observed.certified_outcome() && result_facts_honest(observed);
  return verification;
}

}  // namespace morsehgp3d::hierarchy
