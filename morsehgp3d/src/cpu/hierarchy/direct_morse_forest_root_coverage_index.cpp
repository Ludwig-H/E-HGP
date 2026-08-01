#include "morsehgp3d/hierarchy/direct_morse_forest_root_coverage_index.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

enum class BuildFailure : std::uint8_t {
  capacity_overflow,
  allocation_failed,
  budget_exhausted,
  source_forest_rejected,
  source_cut_rejected,
  order_or_cut_mismatch,
  replay_contradiction,
};

enum class AuditStatus : std::uint8_t {
  okay,
  budget_exhausted,
  contradiction,
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

[[nodiscard]] AuditStatus audit_canonical_key_shape(
    const ExactDirectSparseFacetKey& key,
    std::size_t expected_order,
    std::size_t point_count,
    std::size_t maximum_point_scan_count,
    std::size_t& point_scan_count) noexcept {
  if (expected_order == 0U || key.point_count != expected_order ||
      key.point_count > direct_sparse_positive_facet_maximum_point_count) {
    return AuditStatus::contradiction;
  }
  std::optional<spatial::PointId> previous_point_id;
  for (std::size_t index = 0U; index < key.point_ids.size(); ++index) {
    if (point_scan_count >= maximum_point_scan_count) {
      return AuditStatus::budget_exhausted;
    }
    const spatial::PointId point_id = key.point_ids[index];
    ++point_scan_count;
    if (index < key.point_count) {
      if (point_id >= point_count ||
          (previous_point_id.has_value() &&
           *previous_point_id >= point_id)) {
        return AuditStatus::contradiction;
      }
      previous_point_id = point_id;
    } else if (point_id != 0U) {
      return AuditStatus::contradiction;
    }
  }
  return AuditStatus::okay;
}

void clear_payload(
    ExactDirectMorseForestRootCoverageIndexResult& result) noexcept {
  result.roots.clear();
  result.point_ids.clear();
  result.carrier_root_bindings.clear();
  result.logical_output_entry_count = 0U;
  result.counters = {};
  result.implicit_order_one_prefix_consumed_through_logical_view = false;
  result.physical_higher_order_suffix_consumed_through_logical_view = false;
  result.every_resolved_carrier_birth_revalidated = false;
  result.every_resolved_carrier_bound_exactly_once = false;
  result.every_referenced_root_revalidated = false;
  result.reachable_node_forest_acyclic_and_unshared = false;
  result.order_one_carrier_birth_nodes_reachable_from_bound_roots = false;
  result.canonical_root_point_csr_reconstructed = false;
}

[[nodiscard]] ExactDirectMorseForestRootCoverageIndexResult fail(
    ExactDirectMorseForestRootCoverageIndexResult result,
    BuildFailure failure) noexcept {
  clear_payload(result);
  result.no_partial_scientific_payload_published_on_failure = true;
  result.scope = ExactDirectMorseForestRootCoverageIndexScope::unspecified;
  switch (failure) {
    case BuildFailure::capacity_overflow:
      result.decision = ExactDirectMorseForestRootCoverageIndexDecision::
          no_coverage_capacity_overflow;
      break;
    case BuildFailure::allocation_failed:
      result.decision = ExactDirectMorseForestRootCoverageIndexDecision::
          no_coverage_allocation_failed;
      break;
    case BuildFailure::budget_exhausted:
      result.decision = ExactDirectMorseForestRootCoverageIndexDecision::
          no_coverage_budget_exhausted;
      break;
    case BuildFailure::source_forest_rejected:
      result.decision = ExactDirectMorseForestRootCoverageIndexDecision::
          no_coverage_source_forest_rejected;
      break;
    case BuildFailure::source_cut_rejected:
      result.decision = ExactDirectMorseForestRootCoverageIndexDecision::
          no_coverage_source_cut_rejected;
      break;
    case BuildFailure::order_or_cut_mismatch:
      result.decision = ExactDirectMorseForestRootCoverageIndexDecision::
          no_coverage_order_or_cut_mismatch;
      break;
    case BuildFailure::replay_contradiction:
      result.decision = ExactDirectMorseForestRootCoverageIndexDecision::
          no_coverage_replay_contradiction;
      break;
  }
  return result;
}

[[nodiscard]] bool result_facts_honest(
    const ExactDirectMorseForestRootCoverageIndexResult& result) noexcept {
  return result.conditional_on_caller_fresh_source_forest_replay &&
         result.forest_relative_only &&
         !result.strict_pre_batch_alignment_replayed &&
         !result.strict_pre_batch_alignment_claimed &&
         !result.external_locator_authority_replayed &&
         !result.original_geometry_replayed &&
         !result.global_morse_obligation_replayed &&
         !result.gamma_cells_or_global_cofaces_materialized &&
         !result.higher_order_delaunay_materialized &&
         !result.forbidden_global_structure_materialized &&
         !result.public_status_claimed;
}

[[nodiscard]] bool storage_within(
    const ExactDirectMorseForestRootCoverageIndexResult& result,
    const ExactDirectMorseForestRootCoverageIndexBudget& budget) noexcept {
  return result.roots.size() <= budget.maximum_referenced_root_count &&
         result.carrier_root_bindings.size() <=
             budget.maximum_carrier_root_binding_count &&
         result.point_ids.size() <=
             budget.maximum_coverage_point_count &&
         result.logical_output_entry_count <=
             budget.maximum_logical_output_entry_count &&
         result.counters.cut_entry_scan_count <=
             budget.maximum_cut_entry_scan_count &&
         result.counters.forest_birth_record_scan_count <=
             budget.maximum_forest_birth_record_scan_count &&
         result.counters.forest_node_scan_count <=
             budget.maximum_forest_node_scan_count &&
         result.counters.forest_atomic_group_scan_count <=
             budget.maximum_forest_atomic_group_scan_count &&
         result.counters.forest_batch_scan_count <=
             budget.maximum_forest_batch_scan_count &&
         result.counters.child_edge_scan_count <=
             budget.maximum_child_edge_scan_count &&
         result.counters.facet_key_point_scan_count <=
             budget.maximum_facet_key_point_scan_count &&
         result.counters.point_reference_scan_count <=
             budget.maximum_point_reference_scan_count &&
         result.counters.node_state_count <=
             budget.maximum_node_state_count &&
         result.counters.maximum_depth_first_scratch_count <=
             budget.maximum_depth_first_scratch_count &&
         result.counters.resolved_carrier_scratch_count <=
             budget.maximum_resolved_carrier_scratch_count &&
         result.counters.point_reference_scratch_count <=
             budget.maximum_point_reference_scratch_count;
}

[[nodiscard]] bool output_shape(
    const ExactDirectMorseForestRootCoverageIndexResult& result) noexcept {
  if (result.counters.referenced_root_count != result.roots.size() ||
      result.counters.carrier_root_binding_count !=
          result.carrier_root_bindings.size() ||
      result.counters.coverage_point_count != result.point_ids.size() ||
      result.counters.resolved_carrier_scratch_count !=
          result.carrier_root_bindings.size()) {
    return false;
  }
  std::size_t logical_output_entry_count = 0U;
  if (!checked_add(
          result.roots.size(),
          result.carrier_root_bindings.size(),
          logical_output_entry_count) ||
      !checked_add(
          logical_output_entry_count,
          result.point_ids.size(),
          logical_output_entry_count) ||
      logical_output_entry_count != result.logical_output_entry_count) {
    return false;
  }

  std::size_t point_cursor = 0U;
  std::size_t carrier_binding_count = 0U;
  for (std::size_t index = 0U; index < result.roots.size(); ++index) {
    const auto& root = result.roots[index];
    if (root.root_coverage_index != index ||
        root.point_offset != point_cursor || root.point_count == 0U ||
        root.carrier_binding_count == 0U ||
        !root.last_carrier_binding_index.has_value() ||
        *root.last_carrier_binding_index >=
            result.carrier_root_bindings.size() ||
        root.point_offset > result.point_ids.size() ||
        root.point_count > result.point_ids.size() - root.point_offset ||
        (index != 0U &&
         result.roots[index - 1U].reduced_root_node_id >=
             root.reduced_root_node_id)) {
      return false;
    }
    const auto& last_binding = result.carrier_root_bindings[
        *root.last_carrier_binding_index];
    if (last_binding.root_coverage_index != index ||
        last_binding.root_local_binding_index !=
            root.carrier_binding_count - 1U) {
      return false;
    }
    for (std::size_t local = 0U; local < root.point_count; ++local) {
      const spatial::PointId point_id =
          result.point_ids[root.point_offset + local];
      if (point_id >= result.point_count ||
          (local != 0U &&
           result.point_ids[root.point_offset + local - 1U] >= point_id)) {
        return false;
      }
    }
    if (!checked_add(point_cursor, root.point_count, point_cursor)) {
      return false;
    }
    if (!checked_add(
            carrier_binding_count,
            root.carrier_binding_count,
            carrier_binding_count)) {
      return false;
    }
  }
  if (point_cursor != result.point_ids.size() ||
      carrier_binding_count != result.carrier_root_bindings.size()) {
    return false;
  }

  for (std::size_t index = 0U;
       index < result.carrier_root_bindings.size();
       ++index) {
    const auto& binding = result.carrier_root_bindings[index];
    if (binding.binding_index != index ||
        binding.birth_record_index != binding.component_handle ||
        binding.root_coverage_index >= result.roots.size() ||
        (index != 0U &&
         result.carrier_root_bindings[index - 1U].component_handle >=
             binding.component_handle)) {
      return false;
    }
    if (binding.root_local_binding_index == 0U) {
      if (binding.previous_root_binding_index.has_value()) {
        return false;
      }
    } else {
      if (!binding.previous_root_binding_index.has_value() ||
          *binding.previous_root_binding_index >= index) {
        return false;
      }
      const auto& previous = result.carrier_root_bindings[
          *binding.previous_root_binding_index];
      if (previous.root_coverage_index != binding.root_coverage_index ||
          previous.root_local_binding_index !=
              binding.root_local_binding_index - 1U) {
        return false;
      }
    }
  }
  return true;
}

struct PendingCarrier {
  ExactDirectSparseComponentHandle component_handle{};
  std::size_t birth_record_index{};
  ExactDirectMorseForestNodeId root_node_id{};
  std::optional<ExactDirectMorseForestNodeId> order_one_birth_node_id;
};

struct PointReference {
  ExactDirectMorseForestNodeId root_node_id{};
  spatial::PointId point_id{};
};

struct DfsFrame {
  ExactDirectMorseForestNodeId node_id{};
  std::size_t child_offset{};
  std::size_t child_count{};
  std::size_t next_child{};
};

[[nodiscard]] AuditStatus audit_reachable_forest(
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseForestJournalView& forest_view,
    std::size_t logical_birth_count,
    std::size_t logical_node_count,
    std::size_t target_order,
    const std::vector<ExactDirectMorseForestRootCoverage>& roots,
    const std::vector<PendingCarrier>& pending_carriers,
    const ExactDirectMorseForestRootCoverageIndexBudget& budget,
    ExactDirectMorseForestRootCoverageIndexCounters& counters) {
  if (roots.empty()) {
    return pending_carriers.empty() ? AuditStatus::okay
                                    : AuditStatus::contradiction;
  }
  // This ownership table is sparse in the source forest: only nodes reached
  // from roots actually referenced by the cut are retained.  It therefore
  // does not scale with unrelated orders or future portions of the journal.
  std::unordered_map<ExactDirectMorseForestNodeId, std::size_t>
      reachable_node_owners;
  std::vector<DfsFrame> stack;

  const auto root_index_for_id =
      [&roots](ExactDirectMorseForestNodeId root_node_id)
      -> std::optional<std::size_t> {
    const auto found = std::lower_bound(
        roots.begin(),
        roots.end(),
        root_node_id,
        [](const ExactDirectMorseForestRootCoverage& root,
           ExactDirectMorseForestNodeId id) {
          return root.reduced_root_node_id < id;
        });
    if (found == roots.end() ||
        found->reduced_root_node_id != root_node_id) {
      return std::nullopt;
    }
    return found->root_coverage_index;
  };

  const auto push_node =
      [&](ExactDirectMorseForestNodeId node_id,
          std::size_t owner_root_index) -> AuditStatus {
    if (node_id >= logical_node_count) {
      return AuditStatus::contradiction;
    }
    if (reachable_node_owners.find(node_id) !=
        reachable_node_owners.end()) {
      return AuditStatus::contradiction;
    }
    if (reachable_node_owners.size() >=
            budget.maximum_node_state_count ||
        stack.size() >= budget.maximum_depth_first_scratch_count ||
        counters.forest_node_scan_count >=
            budget.maximum_forest_node_scan_count) {
      return AuditStatus::budget_exhausted;
    }

    const auto node = forest_view.node_at(node_id);
    ++counters.forest_node_scan_count;
    if (node.node_id != node_id || node.order != target_order ||
        node.child_offset > source_forest.child_node_ids.size() ||
        node.child_count >
            source_forest.child_node_ids.size() - node.child_offset) {
      return AuditStatus::contradiction;
    }

    switch (node.kind) {
      case ExactDirectMorseForestNodeKind::order_one_birth: {
        if (target_order != 1U ||
            node_id >= source_forest.implicit_order_one_prefix_count ||
            node.child_count != 0U ||
            !node.birth_record_index.has_value() ||
            *node.birth_record_index >= logical_birth_count ||
            node.atomic_group_index.has_value() ||
            counters.forest_birth_record_scan_count >=
                budget.maximum_forest_birth_record_scan_count) {
          return counters.forest_birth_record_scan_count >=
                         budget.maximum_forest_birth_record_scan_count
                     ? AuditStatus::budget_exhausted
                     : AuditStatus::contradiction;
        }
        const auto birth =
            forest_view.birth_record_at(*node.birth_record_index);
        ++counters.forest_birth_record_scan_count;
        const AuditStatus key_audit = audit_canonical_key_shape(
            birth.facet_key,
            1U,
            source_forest.point_count,
            budget.maximum_facet_key_point_scan_count,
            counters.facet_key_point_scan_count);
        if (birth.birth_record_index != *node.birth_record_index ||
            birth.component_handle != *node.birth_record_index ||
            birth.order != 1U ||
            birth.order_one_birth_node_id !=
                std::optional<ExactDirectMorseForestNodeId>{node_id} ||
            key_audit == AuditStatus::contradiction) {
          return AuditStatus::contradiction;
        }
        if (key_audit == AuditStatus::budget_exhausted) {
          return AuditStatus::budget_exhausted;
        }
        break;
      }
      case ExactDirectMorseForestNodeKind::reduced_birth:
      case ExactDirectMorseForestNodeKind::multifusion: {
        const bool reduced_birth =
            node.kind == ExactDirectMorseForestNodeKind::reduced_birth;
        if (node.birth_record_index.has_value() ||
            !node.atomic_group_index.has_value() ||
            *node.atomic_group_index >= source_forest.atomic_groups.size() ||
            (reduced_birth &&
             (target_order < 2U || node.child_count != 0U)) ||
            (!reduced_birth && node.child_count < 2U) ||
            counters.forest_atomic_group_scan_count >=
                budget.maximum_forest_atomic_group_scan_count) {
          return counters.forest_atomic_group_scan_count >=
                         budget.maximum_forest_atomic_group_scan_count
                     ? AuditStatus::budget_exhausted
                     : AuditStatus::contradiction;
        }
        const auto& group =
            source_forest.atomic_groups[*node.atomic_group_index];
        ++counters.forest_atomic_group_scan_count;
        const auto expected_group_kind =
            reduced_birth
                ? ExactDirectMorseForestAtomicGroupKind::reduced_birth
                : ExactDirectMorseForestAtomicGroupKind::multifusion;
        if (group.atomic_group_index != *node.atomic_group_index ||
            group.kind != expected_group_kind ||
            group.created_node_id !=
                std::optional<ExactDirectMorseForestNodeId>{node_id} ||
            group.resulting_root_node_id != node_id ||
            group.child_offset != node.child_offset ||
            group.child_count != node.child_count ||
            group.batch_index >= source_forest.batches.size() ||
            counters.forest_batch_scan_count >=
                budget.maximum_forest_batch_scan_count) {
          return counters.forest_batch_scan_count >=
                         budget.maximum_forest_batch_scan_count
                     ? AuditStatus::budget_exhausted
                     : AuditStatus::contradiction;
        }
        const auto& batch = source_forest.batches[group.batch_index];
        ++counters.forest_batch_scan_count;
        if (batch.batch_index != group.batch_index ||
            batch.order != target_order ||
            node.squared_level != batch.squared_level ||
            batch.atomic_group_offset > group.atomic_group_index ||
            group.atomic_group_index - batch.atomic_group_offset >=
                batch.atomic_group_count) {
          return AuditStatus::contradiction;
        }
        break;
      }
      default:
        return AuditStatus::contradiction;
    }

    const auto inserted =
        reachable_node_owners.emplace(node_id, owner_root_index);
    if (!inserted.second) {
      return AuditStatus::contradiction;
    }
    counters.node_state_count = reachable_node_owners.size();
    stack.push_back(
        {node_id, node.child_offset, node.child_count, 0U});
    counters.maximum_depth_first_scratch_count = std::max(
        counters.maximum_depth_first_scratch_count, stack.size());
    return AuditStatus::okay;
  };

  for (const auto& root : roots) {
    AuditStatus status =
        push_node(root.reduced_root_node_id, root.root_coverage_index);
    if (status != AuditStatus::okay) {
      return status;
    }
    while (!stack.empty()) {
      DfsFrame& frame = stack.back();
      if (frame.next_child == frame.child_count) {
        stack.pop_back();
        continue;
      }
      if (counters.child_edge_scan_count >=
          budget.maximum_child_edge_scan_count) {
        return AuditStatus::budget_exhausted;
      }
      const std::size_t local_child = frame.next_child;
      ++frame.next_child;
      const ExactDirectMorseForestNodeId child =
          source_forest.child_node_ids[frame.child_offset + local_child];
      ++counters.child_edge_scan_count;
      if (child >= logical_node_count || child >= frame.node_id ||
          (local_child != 0U &&
           source_forest.child_node_ids[
               frame.child_offset + local_child - 1U] >= child)) {
        return AuditStatus::contradiction;
      }
      const auto owner = reachable_node_owners.find(frame.node_id);
      if (owner == reachable_node_owners.end()) {
        return AuditStatus::contradiction;
      }
      const AuditStatus child_status =
          push_node(child, owner->second);
      if (child_status != AuditStatus::okay) {
        return child_status;
      }
    }
  }

  for (const PendingCarrier& carrier : pending_carriers) {
    if (target_order != 1U) {
      if (carrier.order_one_birth_node_id.has_value()) {
        return AuditStatus::contradiction;
      }
      continue;
    }
    const auto root_index = root_index_for_id(carrier.root_node_id);
    if (!root_index.has_value() ||
        !carrier.order_one_birth_node_id.has_value() ||
        *carrier.order_one_birth_node_id >= logical_node_count) {
      return AuditStatus::contradiction;
    }
    const auto state = reachable_node_owners.find(
        *carrier.order_one_birth_node_id);
    if (state == reachable_node_owners.end() ||
        state->second != *root_index) {
      return AuditStatus::contradiction;
    }
  }
  return AuditStatus::okay;
}

}  // namespace

const ExactDirectMorseForestRootCoverage*
ExactDirectMorseForestRootCoverageIndexResult::find_root(
    ExactDirectMorseForestNodeId reduced_root_node_id) const noexcept {
  const auto found = std::lower_bound(
      roots.begin(),
      roots.end(),
      reduced_root_node_id,
      [](const ExactDirectMorseForestRootCoverage& root,
         ExactDirectMorseForestNodeId node_id) {
        return root.reduced_root_node_id < node_id;
      });
  return found != roots.end() &&
             found->reduced_root_node_id == reduced_root_node_id
         ? &*found
         : nullptr;
}

std::span<const spatial::PointId>
ExactDirectMorseForestRootCoverageIndexResult::points_for_root(
    std::size_t root_coverage_index) const noexcept {
  if (root_coverage_index >= roots.size()) {
    return {};
  }
  const auto& root = roots[root_coverage_index];
  if (root.point_offset > point_ids.size() ||
      root.point_count > point_ids.size() - root.point_offset) {
    return {};
  }
  return std::span<const spatial::PointId>{point_ids}.subspan(
      root.point_offset, root.point_count);
}

const ExactDirectMorseForestRootCoverageCarrierBinding*
ExactDirectMorseForestRootCoverageIndexResult::find_binding(
    ExactDirectSparseComponentHandle component_handle) const noexcept {
  const auto found = std::lower_bound(
      carrier_root_bindings.begin(),
      carrier_root_bindings.end(),
      component_handle,
      [](const ExactDirectMorseForestRootCoverageCarrierBinding& binding,
         ExactDirectSparseComponentHandle handle) {
        return binding.component_handle < handle;
      });
  return found != carrier_root_bindings.end() &&
             found->component_handle == component_handle
         ? &*found
         : nullptr;
}

bool ExactDirectMorseForestRootCoverageIndexResult::
    certified_forest_relative_root_coverage_index() const noexcept {
  return schema_version ==
             direct_morse_forest_root_coverage_index_schema_version &&
         decision == ExactDirectMorseForestRootCoverageIndexDecision::
                         complete_certified_forest_relative_root_point_coverage_index &&
         target_order != 0U &&
         target_order <= effective_maximum_order &&
         source_forest_certified_outcome_accepted &&
         source_carrier_cut_index_freshly_verified &&
         target_order_and_closed_cut_match &&
         implicit_order_one_prefix_consumed_through_logical_view &&
         physical_higher_order_suffix_consumed_through_logical_view &&
         every_resolved_carrier_birth_revalidated &&
         every_resolved_carrier_bound_exactly_once &&
         every_referenced_root_revalidated &&
         reachable_node_forest_acyclic_and_unshared &&
         order_one_carrier_birth_nodes_reachable_from_bound_roots &&
         canonical_root_point_csr_reconstructed &&
         !no_partial_scientific_payload_published_on_failure &&
         scope == ExactDirectMorseForestRootCoverageIndexScope::
                      one_target_order_resolved_direct_forest_roots_at_one_closed_exact_cut_only &&
         storage_within(*this, requested_budget) && output_shape(*this) &&
         result_facts_honest(*this);
}

bool ExactDirectMorseForestRootCoverageIndexResult::
    certified_atomic_failure() const noexcept {
  return schema_version ==
             direct_morse_forest_root_coverage_index_schema_version &&
         decision != ExactDirectMorseForestRootCoverageIndexDecision::
                         not_certified &&
         decision != ExactDirectMorseForestRootCoverageIndexDecision::
                         complete_certified_forest_relative_root_point_coverage_index &&
         roots.empty() && point_ids.empty() &&
         carrier_root_bindings.empty() &&
         logical_output_entry_count == 0U &&
         counters == ExactDirectMorseForestRootCoverageIndexCounters{} &&
         no_partial_scientific_payload_published_on_failure &&
         scope == ExactDirectMorseForestRootCoverageIndexScope::unspecified &&
         result_facts_honest(*this);
}

bool ExactDirectMorseForestRootCoverageIndexResult::certified_outcome()
    const noexcept {
  return certified_forest_relative_root_coverage_index() ||
         certified_atomic_failure();
}

ExactDirectMorseForestRootCoverageIndexResult
build_exact_direct_morse_forest_root_coverage_index(
    const ExactDirectMorseForestJournalResult& source_forest,
    std::size_t target_order,
    const exact::ExactLevel& closed_squared_level,
    const ExactDirectMorseForestCarrierCutIndexBudget& trusted_cut_budget,
    const ExactDirectMorseForestCarrierCutIndexResult& source_cut_index,
    const ExactDirectMorseForestRootCoverageIndexBudget& budget) {
  ExactDirectMorseForestRootCoverageIndexResult result;
  try {
    result.requested_budget = budget;
    result.point_count = source_forest.point_count;
    result.effective_maximum_order =
        source_forest.effective_maximum_order;
    result.target_order = target_order;
    result.closed_squared_level = closed_squared_level;

  if (source_forest.schema_version !=
          direct_morse_forest_journal_schema_version ||
      !source_forest.certified_conditional_h0_candidate()) {
    return fail(std::move(result), BuildFailure::source_forest_rejected);
  }
  result.source_forest_certified_outcome_accepted = true;
  if (target_order == 0U ||
      target_order > source_forest.effective_maximum_order ||
      source_cut_index.target_order != target_order ||
      source_cut_index.closed_squared_level != closed_squared_level ||
      source_cut_index.point_count != source_forest.point_count ||
      source_cut_index.effective_maximum_order !=
          source_forest.effective_maximum_order) {
    return fail(std::move(result), BuildFailure::order_or_cut_mismatch);
  }
  result.target_order_and_closed_cut_match = true;

  const auto cut_verification =
      verify_exact_direct_morse_forest_carrier_cut_index(
          source_forest,
          target_order,
          closed_squared_level,
          trusted_cut_budget,
          source_cut_index);
  if (!cut_verification.result_certified ||
      !source_cut_index.certified_forest_relative_closed_cut_index()) {
    return fail(std::move(result), BuildFailure::source_cut_rejected);
  }
  result.source_carrier_cut_index_freshly_verified = true;

  std::size_t logical_birth_count = 0U;
  std::size_t logical_node_count = 0U;
  if (!checked_add(
          source_forest.implicit_order_one_prefix_count,
          source_forest.birth_records.size(),
          logical_birth_count) ||
      !checked_add(
          source_forest.implicit_order_one_prefix_count,
          source_forest.nodes.size(),
          logical_node_count)) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  }
  if (source_cut_index.entries.size() >
          budget.maximum_cut_entry_scan_count ||
      source_cut_index.counters.resolved_reduced_root_carrier_count >
          budget.maximum_resolved_carrier_scratch_count ||
      source_cut_index.counters.resolved_reduced_root_carrier_count >
          budget.maximum_carrier_root_binding_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }

  std::size_t required_point_reference_count = 0U;
  if (!checked_multiply(
          source_cut_index.counters.resolved_reduced_root_carrier_count,
          target_order,
          required_point_reference_count)) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  }
  if (required_point_reference_count >
          budget.maximum_point_reference_scan_count ||
      required_point_reference_count >
          budget.maximum_point_reference_scratch_count ||
      source_cut_index.entries.size() >
          budget.maximum_forest_birth_record_scan_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }

    const ExactDirectMorseForestJournalView forest_view{source_forest};
    if (forest_view.birth_record_count() != logical_birth_count ||
        forest_view.node_count() != logical_node_count) {
      return fail(std::move(result), BuildFailure::replay_contradiction);
    }

    std::vector<PendingCarrier> pending_carriers;
    std::vector<PointReference> point_references;
    pending_carriers.reserve(
        source_cut_index.counters.resolved_reduced_root_carrier_count);
    point_references.reserve(required_point_reference_count);

    for (const auto& entry : source_cut_index.entries) {
      ++result.counters.cut_entry_scan_count;
      if (entry.birth_record_index >= logical_birth_count ||
          result.counters.forest_birth_record_scan_count >=
              budget.maximum_forest_birth_record_scan_count) {
        return fail(
            std::move(result),
            entry.birth_record_index >= logical_birth_count
                ? BuildFailure::replay_contradiction
                : BuildFailure::budget_exhausted);
      }
      const auto birth =
          forest_view.birth_record_at(entry.birth_record_index);
      ++result.counters.forest_birth_record_scan_count;
      const AuditStatus key_audit = audit_canonical_key_shape(
          birth.facet_key,
          target_order,
          source_forest.point_count,
          budget.maximum_facet_key_point_scan_count,
          result.counters.facet_key_point_scan_count);
      if (birth.birth_record_index != entry.birth_record_index ||
          birth.component_handle != entry.component_handle ||
          birth.order != target_order ||
          key_audit == AuditStatus::contradiction ||
          (target_order == 1U) !=
              birth.order_one_birth_node_id.has_value()) {
        return fail(std::move(result), BuildFailure::replay_contradiction);
      }
      if (key_audit == AuditStatus::budget_exhausted) {
        return fail(std::move(result), BuildFailure::budget_exhausted);
      }

      switch (entry.disposition) {
        case ExactDirectMorseForestCarrierCutDisposition::
            inactive_at_closed_cut:
        case ExactDirectMorseForestCarrierCutDisposition::
            active_latent_without_reduced_root:
          if (entry.reduced_root_node_id.has_value()) {
            return fail(
                std::move(result), BuildFailure::replay_contradiction);
          }
          break;
        case ExactDirectMorseForestCarrierCutDisposition::
            resolved_reduced_root:
          if (!entry.reduced_root_node_id.has_value() ||
              *entry.reduced_root_node_id >= logical_node_count ||
              pending_carriers.size() >=
                  budget.maximum_resolved_carrier_scratch_count) {
            return fail(
                std::move(result),
                pending_carriers.size() >=
                        budget.maximum_resolved_carrier_scratch_count
                    ? BuildFailure::budget_exhausted
                    : BuildFailure::replay_contradiction);
          }
          pending_carriers.push_back(
              {entry.component_handle,
               entry.birth_record_index,
               *entry.reduced_root_node_id,
               birth.order_one_birth_node_id});
          for (std::size_t point_index = 0U;
               point_index < birth.facet_key.point_count;
               ++point_index) {
            if (point_references.size() >=
                    budget.maximum_point_reference_scratch_count ||
                result.counters.point_reference_scan_count >=
                    budget.maximum_point_reference_scan_count) {
              return fail(
                  std::move(result), BuildFailure::budget_exhausted);
            }
            point_references.push_back(
                {*entry.reduced_root_node_id,
                 birth.facet_key.point_ids[point_index]});
            ++result.counters.point_reference_scan_count;
          }
          break;
        default:
          return fail(
              std::move(result), BuildFailure::replay_contradiction);
      }
    }

    if (pending_carriers.size() !=
            source_cut_index.counters
                .resolved_reduced_root_carrier_count ||
        point_references.size() != required_point_reference_count) {
      return fail(std::move(result), BuildFailure::replay_contradiction);
    }
    result.counters.resolved_carrier_scratch_count =
        pending_carriers.size();
    result.counters.point_reference_scratch_count =
        point_references.size();

    std::sort(
        point_references.begin(),
        point_references.end(),
        [](const PointReference& left, const PointReference& right) {
          return left.root_node_id < right.root_node_id ||
                 (left.root_node_id == right.root_node_id &&
                  left.point_id < right.point_id);
        });

    std::size_t referenced_root_count = 0U;
    std::size_t coverage_point_count = 0U;
    for (std::size_t index = 0U; index < point_references.size(); ++index) {
      const bool new_root =
          index == 0U ||
          point_references[index - 1U].root_node_id !=
              point_references[index].root_node_id;
      const bool new_point =
          new_root || point_references[index - 1U].point_id !=
                          point_references[index].point_id;
      if (new_root &&
          !checked_add(referenced_root_count, 1U, referenced_root_count)) {
        return fail(std::move(result), BuildFailure::capacity_overflow);
      }
      if (new_point &&
          !checked_add(coverage_point_count, 1U, coverage_point_count)) {
        return fail(std::move(result), BuildFailure::capacity_overflow);
      }
    }
    if (referenced_root_count > budget.maximum_referenced_root_count ||
        coverage_point_count > budget.maximum_coverage_point_count) {
      return fail(std::move(result), BuildFailure::budget_exhausted);
    }
    std::size_t logical_output_entry_count = 0U;
    if (!checked_add(
            referenced_root_count,
            pending_carriers.size(),
            logical_output_entry_count) ||
        !checked_add(
            logical_output_entry_count,
            coverage_point_count,
            logical_output_entry_count)) {
      return fail(std::move(result), BuildFailure::capacity_overflow);
    }
    if (logical_output_entry_count >
        budget.maximum_logical_output_entry_count) {
      return fail(std::move(result), BuildFailure::budget_exhausted);
    }

    result.roots.reserve(referenced_root_count);
    result.point_ids.reserve(coverage_point_count);
    result.carrier_root_bindings.reserve(pending_carriers.size());
    for (std::size_t index = 0U; index < point_references.size(); ++index) {
      const auto& reference = point_references[index];
      const bool new_root =
          index == 0U ||
          point_references[index - 1U].root_node_id !=
              reference.root_node_id;
      const bool new_point =
          new_root || point_references[index - 1U].point_id !=
                          reference.point_id;
      if (new_root) {
        if (!result.roots.empty()) {
          auto& prior = result.roots.back();
          prior.point_count = result.point_ids.size() - prior.point_offset;
        }
        result.roots.push_back(
            {result.roots.size(),
             reference.root_node_id,
             result.point_ids.size(),
             0U,
             0U,
             std::nullopt});
      }
      if (new_point) {
        result.point_ids.push_back(reference.point_id);
      }
    }
    if (!result.roots.empty()) {
      auto& final_root = result.roots.back();
      final_root.point_count =
          result.point_ids.size() - final_root.point_offset;
    }
    if (result.roots.size() != referenced_root_count ||
        result.point_ids.size() != coverage_point_count) {
      return fail(std::move(result), BuildFailure::replay_contradiction);
    }

    const AuditStatus audit = audit_reachable_forest(
        source_forest,
        forest_view,
        logical_birth_count,
        logical_node_count,
        target_order,
        result.roots,
        pending_carriers,
        budget,
        result.counters);
    if (audit != AuditStatus::okay) {
      return fail(
          std::move(result),
          audit == AuditStatus::budget_exhausted
              ? BuildFailure::budget_exhausted
              : BuildFailure::replay_contradiction);
    }

    for (const PendingCarrier& carrier : pending_carriers) {
      const auto* root = result.find_root(carrier.root_node_id);
      if (root == nullptr ||
          (!result.carrier_root_bindings.empty() &&
           result.carrier_root_bindings.back().component_handle >=
               carrier.component_handle)) {
        return fail(std::move(result), BuildFailure::replay_contradiction);
      }
      auto& mutable_root = result.roots[root->root_coverage_index];
      std::size_t updated_binding_count = 0U;
      if (!checked_add(
              mutable_root.carrier_binding_count,
              1U,
              updated_binding_count)) {
        return fail(std::move(result), BuildFailure::capacity_overflow);
      }
      const std::size_t binding_index =
          result.carrier_root_bindings.size();
      result.carrier_root_bindings.push_back(
          {binding_index,
           carrier.component_handle,
           carrier.birth_record_index,
           root->root_coverage_index,
           mutable_root.carrier_binding_count,
           mutable_root.last_carrier_binding_index});
      mutable_root.carrier_binding_count = updated_binding_count;
      mutable_root.last_carrier_binding_index = binding_index;
    }

    result.logical_output_entry_count = logical_output_entry_count;
    result.counters.referenced_root_count = result.roots.size();
    result.counters.carrier_root_binding_count =
        result.carrier_root_bindings.size();
    result.counters.coverage_point_count = result.point_ids.size();
    result.implicit_order_one_prefix_consumed_through_logical_view = true;
    result.physical_higher_order_suffix_consumed_through_logical_view = true;
    result.every_resolved_carrier_birth_revalidated = true;
    result.every_resolved_carrier_bound_exactly_once = true;
    result.every_referenced_root_revalidated = true;
    result.reachable_node_forest_acyclic_and_unshared = true;
    result.order_one_carrier_birth_nodes_reachable_from_bound_roots = true;
    result.canonical_root_point_csr_reconstructed = true;
    result.scope = ExactDirectMorseForestRootCoverageIndexScope::
        one_target_order_resolved_direct_forest_roots_at_one_closed_exact_cut_only;
    result.decision = ExactDirectMorseForestRootCoverageIndexDecision::
        complete_certified_forest_relative_root_point_coverage_index;
    if (!result.certified_forest_relative_root_coverage_index()) {
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

ExactDirectMorseForestRootCoverageIndexVerification
verify_exact_direct_morse_forest_root_coverage_index(
    const ExactDirectMorseForestJournalResult& source_forest,
    std::size_t target_order,
    const exact::ExactLevel& closed_squared_level,
    const ExactDirectMorseForestCarrierCutIndexBudget& trusted_cut_budget,
    const ExactDirectMorseForestCarrierCutIndexResult& source_cut_index,
    const ExactDirectMorseForestRootCoverageIndexBudget& trusted_budget,
    const ExactDirectMorseForestRootCoverageIndexResult& observed) {
  ExactDirectMorseForestRootCoverageIndexVerification verification;
  verification.observed_storage_within_budget =
      storage_within(observed, trusted_budget);
  const auto expected =
      build_exact_direct_morse_forest_root_coverage_index(
          source_forest,
          target_order,
          closed_squared_level,
          trusted_cut_budget,
          source_cut_index,
          trusted_budget);
  verification.trusted_source_forest_outcome_accepted =
      expected.source_forest_certified_outcome_accepted;
  verification.trusted_source_carrier_cut_index_freshly_verified =
      expected.source_carrier_cut_index_freshly_verified;
  verification.expected_index_freshly_reconstructed =
      expected.certified_outcome();
  verification.observed_recursively_equal = observed == expected;
  verification.result_certified =
      verification.trusted_source_forest_outcome_accepted &&
      verification.trusted_source_carrier_cut_index_freshly_verified &&
      verification.observed_storage_within_budget &&
      verification.expected_index_freshly_reconstructed &&
      verification.observed_recursively_equal &&
      observed.certified_outcome() && result_facts_honest(observed);
  return verification;
}

}  // namespace morsehgp3d::hierarchy
