#include "morsehgp3d/hierarchy/direct_morse_vertical_journal.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <boost/multiprecision/integer.hpp>

namespace morsehgp3d::hierarchy {
namespace {

constexpr ExactDirectMorseForestNodeId no_node =
    std::numeric_limits<ExactDirectMorseForestNodeId>::max();

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
    const exact::BigInt& value) noexcept {
  if (value == 0) {
    return 1U;
  }
  return static_cast<std::size_t>(boost::multiprecision::msb(value)) + 1U;
}

[[nodiscard]] bool key_valid(
    const ExactDirectSparseFacetKey& key,
    std::size_t point_count,
    std::size_t expected_cardinality) noexcept {
  if (key.point_count != expected_cardinality ||
      key.point_count == 0U ||
      key.point_count >
          direct_sparse_positive_facet_maximum_point_count) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (key.point_ids[index] >= point_count ||
        (index != 0U &&
         key.point_ids[index - 1U] >= key.point_ids[index])) {
      return false;
    }
  }
  for (std::size_t index = key.point_count;
       index < key.point_ids.size();
       ++index) {
    if (key.point_ids[index] != 0U) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool key_less(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right) noexcept {
  if (left.point_count != right.point_count) {
    return left.point_count < right.point_count;
  }
  for (std::size_t index = 0U; index < left.point_count; ++index) {
    if (left.point_ids[index] != right.point_ids[index]) {
      return left.point_ids[index] < right.point_ids[index];
    }
  }
  return false;
}

void clear_payload(ExactDirectMorseVerticalJournalResult& result) noexcept {
  result.adjacent_families.clear();
  result.label_resolutions.clear();
  result.group_checks.clear();
  result.checkpoints.clear();
  result.logical_output_entry_count = 0U;
  result.counters = {};
}

[[nodiscard]] ExactDirectMorseVerticalJournalResult fail(
    ExactDirectMorseVerticalJournalResult result,
    ExactDirectMorseVerticalDecision decision) noexcept {
  clear_payload(result);
  result.decision = decision;
  result.scope = ExactDirectMorseVerticalScope::unspecified;
  result.no_partial_scientific_payload_published_on_failure = true;
  return result;
}

struct GroupPlan {
  std::size_t group_index{};
  std::vector<std::size_t> representative_binding_indices;
};

struct ForestShape {
  std::vector<ExactDirectMorseForestNodeId> parent_by_node;
  std::vector<std::vector<ExactDirectMorseForestNodeId>> nodes_by_order;
  std::vector<std::size_t> omitted_births_by_order;
  std::vector<GroupPlan> group_plans;
};

class LevelAccounting {
 public:
  explicit LevelAccounting(
      const ExactDirectMorseVerticalBudget& budget,
      ExactDirectMorseVerticalCounters& counters) noexcept
      : budget_(budget), counters_(counters) {}

  [[nodiscard]] bool measure(const exact::ExactLevel& level) noexcept {
    const std::size_t observed = std::max(
        integer_bit_count(level.numerator()),
        integer_bit_count(level.denominator()));
    counters_.maximum_observed_exact_level_integer_bit_count = std::max(
        counters_.maximum_observed_exact_level_integer_bit_count,
        observed);
    return observed <=
           budget_.maximum_single_exact_level_integer_bit_count;
  }

  [[nodiscard]] bool less(
      const exact::ExactLevel& left,
      const exact::ExactLevel& right) {
    if (!increment()) {
      return false;
    }
    last_relation_ = left < right ? -1 : (right < left ? 1 : 0);
    return true;
  }

  [[nodiscard]] bool less_equal(
      const exact::ExactLevel& left,
      const exact::ExactLevel& right) {
    if (!increment()) {
      return false;
    }
    last_relation_ = left < right ? -1 : (right < left ? 1 : 0);
    return true;
  }

  [[nodiscard]] int relation() const noexcept {
    return last_relation_;
  }

  [[nodiscard]] bool exhausted() const noexcept {
    return exhausted_;
  }

 private:
  [[nodiscard]] bool increment() noexcept {
    if (counters_.exact_level_comparison_count >=
        budget_.maximum_exact_level_comparison_count) {
      exhausted_ = true;
      return false;
    }
    ++counters_.exact_level_comparison_count;
    return true;
  }

  const ExactDirectMorseVerticalBudget& budget_;
  ExactDirectMorseVerticalCounters& counters_;
  int last_relation_{};
  bool exhausted_{false};
};

[[nodiscard]] bool same_root_set(
    const ExactDirectMorseForestJournalResult& forest,
    const std::vector<ExactDirectMorseForestNodeId>& parent_by_node) {
  std::vector<ExactDirectMorseForestNodeId> structural;
  structural.reserve(forest.nodes.size());
  for (std::size_t index = 0U; index < parent_by_node.size(); ++index) {
    if (parent_by_node[index] == no_node) {
      structural.push_back(
          static_cast<ExactDirectMorseForestNodeId>(index));
    }
  }
  std::vector<ExactDirectMorseForestNodeId> declared;
  declared.reserve(forest.final_roots.size());
  for (const auto& root : forest.final_roots) {
    declared.push_back(root.root_node_id);
  }
  std::sort(structural.begin(), structural.end());
  std::sort(declared.begin(), declared.end());
  return structural == declared;
}

enum class ShapeStatus : std::uint8_t {
  okay,
  rejected,
  budget_exhausted,
};

[[nodiscard]] ShapeStatus reconstruct_shape(
    const ExactDirectMorseForestJournalResult& forest,
    const ExactDirectMorseVerticalBudget& budget,
    ExactDirectMorseVerticalCounters& counters,
    ForestShape& shape) {
  if (forest.schema_version !=
          direct_morse_forest_journal_schema_version ||
      forest.decision != ExactDirectMorseForestDecision::
                             complete_conditional_exact_direct_morse_forest ||
      forest.point_count == 0U ||
      forest.effective_maximum_order == 0U ||
      forest.effective_maximum_order >
          direct_sparse_positive_facet_maximum_point_count ||
      forest.effective_maximum_order > forest.point_count) {
    return ShapeStatus::rejected;
  }
  if (forest.nodes.size() > budget.maximum_forest_node_scan_count ||
      forest.child_node_ids.size() >
          budget.maximum_child_reference_scan_count ||
      forest.birth_records.size() >
          budget.maximum_birth_record_scan_count ||
      forest.batches.size() > budget.maximum_batch_scan_count ||
      forest.atomic_groups.size() >
          budget.maximum_atomic_group_scan_count ||
      forest.saddle_records.size() > budget.maximum_saddle_scan_count ||
      forest.arm_root_bindings.size() >
          budget.maximum_arm_binding_scan_count) {
    return ShapeStatus::budget_exhausted;
  }

  LevelAccounting levels{budget, counters};
  for (const auto& node : forest.nodes) {
    if (!levels.measure(node.squared_level)) {
      return ShapeStatus::budget_exhausted;
    }
  }
  for (const auto& batch : forest.batches) {
    if (!levels.measure(batch.squared_level)) {
      return ShapeStatus::budget_exhausted;
    }
  }

  shape.parent_by_node.assign(forest.nodes.size(), no_node);
  shape.nodes_by_order.assign(
      forest.effective_maximum_order + 1U, {});
  for (std::size_t index = 0U; index < forest.nodes.size(); ++index) {
    const auto& node = forest.nodes[index];
    if (node.node_id !=
            static_cast<ExactDirectMorseForestNodeId>(index) ||
        node.order == 0U ||
        node.order > forest.effective_maximum_order ||
        node.child_offset > forest.child_node_ids.size() ||
        node.child_count >
            forest.child_node_ids.size() - node.child_offset) {
      return ShapeStatus::rejected;
    }
    auto& order_nodes = shape.nodes_by_order[node.order];
    if (!order_nodes.empty()) {
      const auto& prior = forest.nodes[order_nodes.back()];
      if (!levels.less_equal(
              prior.squared_level, node.squared_level)) {
        return ShapeStatus::budget_exhausted;
      }
      if (levels.relation() > 0) {
        return ShapeStatus::rejected;
      }
    }
    order_nodes.push_back(node.node_id);
    for (std::size_t local = 0U; local < node.child_count; ++local) {
      const ExactDirectMorseForestNodeId child =
          forest.child_node_ids[node.child_offset + local];
      if (child >= forest.nodes.size() ||
          forest.nodes[child].order != node.order ||
          shape.parent_by_node[child] != no_node) {
        return ShapeStatus::rejected;
      }
      if (!levels.less(
              forest.nodes[child].squared_level,
              node.squared_level)) {
        return ShapeStatus::budget_exhausted;
      }
      if (levels.relation() >= 0) {
        return ShapeStatus::rejected;
      }
      shape.parent_by_node[child] = node.node_id;
    }
  }
  if (!same_root_set(forest, shape.parent_by_node)) {
    return ShapeStatus::rejected;
  }

  shape.omitted_births_by_order.assign(
      forest.effective_maximum_order + 1U, 0U);
  std::vector<bool> order_one_birth_node_seen(
      forest.nodes.size(), false);
  for (std::size_t index = 0U;
       index < forest.birth_records.size();
       ++index) {
    const auto& birth = forest.birth_records[index];
    if (birth.birth_record_index != index ||
        birth.order == 0U ||
        birth.order > forest.effective_maximum_order ||
        !key_valid(
            birth.facet_key, forest.point_count, birth.order)) {
      return ShapeStatus::rejected;
    }
    if (birth.order == 1U) {
      if (!birth.order_one_birth_node_id.has_value() ||
          *birth.order_one_birth_node_id >= forest.nodes.size()) {
        return ShapeStatus::rejected;
      }
      const auto node_id = *birth.order_one_birth_node_id;
      const auto& node = forest.nodes[node_id];
      if (order_one_birth_node_seen[node_id] ||
          node.order != 1U ||
          node.kind != ExactDirectMorseForestNodeKind::order_one_birth ||
          !node.birth_record_index.has_value() ||
          *node.birth_record_index != index) {
        return ShapeStatus::rejected;
      }
      order_one_birth_node_seen[node_id] = true;
    } else {
      if (birth.order_one_birth_node_id.has_value()) {
        return ShapeStatus::rejected;
      }
      ++shape.omitted_births_by_order[birth.order];
    }
  }

  std::size_t group_partition_cursor = 0U;
  for (std::size_t index = 0U; index < forest.batches.size(); ++index) {
    const auto& batch = forest.batches[index];
    if (batch.batch_index != index ||
        batch.order == 0U ||
        batch.order > forest.effective_maximum_order ||
        batch.atomic_group_offset != group_partition_cursor ||
        batch.atomic_group_offset > forest.atomic_groups.size() ||
        batch.atomic_group_count >
            forest.atomic_groups.size() - batch.atomic_group_offset) {
      return ShapeStatus::rejected;
    }
    group_partition_cursor += batch.atomic_group_count;
    for (std::size_t local = 0U;
         local < batch.atomic_group_count;
         ++local) {
      const std::size_t group_index =
          batch.atomic_group_offset + local;
      if (forest.atomic_groups[group_index].atomic_group_index !=
              group_index ||
          forest.atomic_groups[group_index].batch_index != index) {
        return ShapeStatus::rejected;
      }
    }
    if (index != 0U) {
      const auto& prior = forest.batches[index - 1U];
      if (prior.order > batch.order) {
        return ShapeStatus::rejected;
      }
      if (prior.order == batch.order) {
        if (!levels.less(
                prior.squared_level, batch.squared_level)) {
          return ShapeStatus::budget_exhausted;
        }
        if (levels.relation() >= 0) {
          return ShapeStatus::rejected;
        }
      }
    }
  }
  if (group_partition_cursor != forest.atomic_groups.size()) {
    return ShapeStatus::rejected;
  }

  shape.group_plans.reserve(forest.atomic_groups.size());
  std::size_t saddle_partition_cursor = 0U;
  std::size_t binding_partition_cursor = 0U;
  for (std::size_t group_index = 0U;
       group_index < forest.atomic_groups.size();
       ++group_index) {
    const auto& group = forest.atomic_groups[group_index];
    if (group.atomic_group_index != group_index ||
        group.batch_index >= forest.batches.size() ||
        group.saddle_record_offset != saddle_partition_cursor ||
        group.saddle_record_offset > forest.saddle_records.size() ||
        group.saddle_record_count >
            forest.saddle_records.size() -
                group.saddle_record_offset ||
        group.saddle_record_count == 0U ||
        group.resulting_root_node_id >= forest.nodes.size()) {
      return ShapeStatus::rejected;
    }
    const auto& batch = forest.batches[group.batch_index];
    if (forest.nodes[group.resulting_root_node_id].order != batch.order) {
      return ShapeStatus::rejected;
    }
    switch (group.kind) {
      case ExactDirectMorseForestAtomicGroupKind::reduced_birth:
        if (group.prior_reduced_root_count != 0U ||
            group.child_count != 0U ||
            !group.created_node_id.has_value() ||
            *group.created_node_id != group.resulting_root_node_id) {
          return ShapeStatus::rejected;
        }
        break;
      case ExactDirectMorseForestAtomicGroupKind::continuation:
        if (group.prior_reduced_root_count != 1U ||
            group.child_count != 0U ||
            group.created_node_id.has_value()) {
          return ShapeStatus::rejected;
        }
        break;
      case ExactDirectMorseForestAtomicGroupKind::multifusion:
        if (group.prior_reduced_root_count < 2U ||
            group.child_offset > forest.child_node_ids.size() ||
            group.child_count >
                forest.child_node_ids.size() - group.child_offset ||
            group.child_count != group.prior_reduced_root_count ||
            !group.created_node_id.has_value() ||
            *group.created_node_id != group.resulting_root_node_id) {
          return ShapeStatus::rejected;
        }
        break;
      default:
        return ShapeStatus::rejected;
    }

    GroupPlan plan;
    plan.group_index = group_index;
    std::vector<std::size_t> bindings;
    for (std::size_t local_saddle = 0U;
         local_saddle < group.saddle_record_count;
         ++local_saddle) {
      const auto& saddle = forest.saddle_records[
          group.saddle_record_offset + local_saddle];
      const std::size_t saddle_index =
          group.saddle_record_offset + local_saddle;
      if (saddle.saddle_record_index != saddle_index ||
          saddle.arm_binding_offset != binding_partition_cursor ||
          saddle.arm_binding_offset >
              forest.arm_root_bindings.size() ||
          saddle.arm_binding_count >
              forest.arm_root_bindings.size() -
                  saddle.arm_binding_offset ||
          saddle.arm_binding_count == 0U ||
          saddle.atomic_group_index != group_index) {
        return ShapeStatus::rejected;
      }
      binding_partition_cursor += saddle.arm_binding_count;
      for (std::size_t local_binding = 0U;
           local_binding < saddle.arm_binding_count;
           ++local_binding) {
        const std::size_t binding_index =
            saddle.arm_binding_offset + local_binding;
        const auto& binding =
            forest.arm_root_bindings[binding_index];
        if (binding.binding_index != binding_index ||
            !key_valid(
                binding.strict_arm_key,
                forest.point_count,
                batch.order)) {
          return ShapeStatus::rejected;
        }
        bindings.push_back(binding_index);
      }
    }
    if (bindings.size() >
        budget.maximum_group_sort_scratch_count) {
      return ShapeStatus::budget_exhausted;
    }
    bool sort_exhausted = false;
    std::sort(
        bindings.begin(),
        bindings.end(),
        [&](std::size_t left, std::size_t right) {
          if (counters.group_sort_comparison_count >=
              budget.maximum_group_sort_comparison_count) {
            sort_exhausted = true;
            return left < right;
          }
          ++counters.group_sort_comparison_count;
          const auto& left_key =
              forest.arm_root_bindings[left].strict_arm_key;
          const auto& right_key =
              forest.arm_root_bindings[right].strict_arm_key;
          if (key_less(left_key, right_key)) {
            return true;
          }
          if (key_less(right_key, left_key)) {
            return false;
          }
          return left < right;
        });
    if (sort_exhausted) {
      return ShapeStatus::budget_exhausted;
    }
    for (const std::size_t binding_index : bindings) {
      if (plan.representative_binding_indices.empty() ||
          forest.arm_root_bindings[
              plan.representative_binding_indices.back()]
                  .strict_arm_key !=
              forest.arm_root_bindings[binding_index].strict_arm_key) {
        plan.representative_binding_indices.push_back(binding_index);
      }
    }
    if (plan.representative_binding_indices.empty()) {
      return ShapeStatus::rejected;
    }
    shape.group_plans.push_back(std::move(plan));
    saddle_partition_cursor += group.saddle_record_count;
  }
  if (saddle_partition_cursor != forest.saddle_records.size() ||
      binding_partition_cursor != forest.arm_root_bindings.size()) {
    return ShapeStatus::rejected;
  }
  if (levels.exhausted()) {
    return ShapeStatus::budget_exhausted;
  }
  return ShapeStatus::okay;
}

class TargetSweep {
 public:
  TargetSweep(
      const ExactDirectMorseForestJournalResult& forest,
      const ForestShape& shape,
      std::size_t order,
      const ExactDirectMorseVerticalBudget& budget,
      ExactDirectMorseVerticalCounters& counters)
      : forest_(forest),
        nodes_(shape.nodes_by_order[order]),
        budget_(budget),
        counters_(counters),
        parent_(forest.nodes.size(), no_node),
        active_(forest.nodes.size(), false) {}

  [[nodiscard]] bool advance(const exact::ExactLevel& level) {
    while (cursor_ < nodes_.size()) {
      const auto node_id = nodes_[cursor_];
      if (!compare_less_equal(
              forest_.nodes[node_id].squared_level, level)) {
        return false;
      }
      if (last_relation_ > 0) {
        break;
      }
      active_[node_id] = true;
      parent_[node_id] = node_id;
      const auto& node = forest_.nodes[node_id];
      for (std::size_t local = 0U; local < node.child_count; ++local) {
        const auto child =
            forest_.child_node_ids[node.child_offset + local];
        if (!active_[child]) {
          invalid_ = true;
          return false;
        }
        const auto child_root = find(child);
        if (!child_root.has_value()) {
          return false;
        }
        parent_[*child_root] = node_id;
      }
      ++cursor_;
    }
    return true;
  }

  [[nodiscard]] std::optional<ExactDirectMorseForestNodeId> find(
      ExactDirectMorseForestNodeId node) {
    if (node >= parent_.size() || !active_[node]) {
      invalid_ = true;
      return std::nullopt;
    }
    ExactDirectMorseForestNodeId root = node;
    while (parent_[root] != root) {
      if (!consume_hop()) {
        return std::nullopt;
      }
      root = parent_[root];
    }
    while (parent_[node] != node) {
      if (!consume_hop()) {
        return std::nullopt;
      }
      const auto next = parent_[node];
      parent_[node] = root;
      node = next;
    }
    return root;
  }

  [[nodiscard]] bool invalid() const noexcept {
    return invalid_;
  }

  [[nodiscard]] bool budget_exhausted() const noexcept {
    return budget_exhausted_;
  }

 private:
  [[nodiscard]] bool compare_less_equal(
      const exact::ExactLevel& left,
      const exact::ExactLevel& right) {
    if (counters_.exact_level_comparison_count >=
        budget_.maximum_exact_level_comparison_count) {
      budget_exhausted_ = true;
      return false;
    }
    ++counters_.exact_level_comparison_count;
    last_relation_ = left < right ? -1 : (right < left ? 1 : 0);
    return true;
  }

  [[nodiscard]] bool consume_hop() noexcept {
    if (counters_.target_parent_hop_count >=
        budget_.maximum_target_parent_hop_count) {
      budget_exhausted_ = true;
      return false;
    }
    ++counters_.target_parent_hop_count;
    return true;
  }

  const ExactDirectMorseForestJournalResult& forest_;
  const std::vector<ExactDirectMorseForestNodeId>& nodes_;
  const ExactDirectMorseVerticalBudget& budget_;
  ExactDirectMorseVerticalCounters& counters_;
  std::vector<ExactDirectMorseForestNodeId> parent_;
  std::vector<bool> active_;
  std::size_t cursor_{};
  int last_relation_{};
  bool invalid_{false};
  bool budget_exhausted_{false};
};

[[nodiscard]] bool proposal_less(
    const std::pair<ExactDirectMorseVerticalTargetProposal, std::size_t>& left,
    const std::pair<ExactDirectMorseVerticalTargetProposal, std::size_t>&
        right) noexcept {
  return left.first.representative_arm_root_binding_index <
         right.first.representative_arm_root_binding_index;
}

[[nodiscard]] bool group_complete(
    ExactDirectMorseVerticalGroupDisposition disposition) noexcept {
  return disposition ==
             ExactDirectMorseVerticalGroupDisposition::
                 complete_reduced_birth ||
         disposition ==
             ExactDirectMorseVerticalGroupDisposition::
                 complete_continuation ||
         disposition ==
             ExactDirectMorseVerticalGroupDisposition::
                 complete_multifusion;
}

[[nodiscard]] ExactDirectMorseVerticalGroupDisposition disposition_for(
    ExactDirectMorseForestAtomicGroupKind kind,
    bool complete) noexcept {
  switch (kind) {
    case ExactDirectMorseForestAtomicGroupKind::reduced_birth:
      return complete
                 ? ExactDirectMorseVerticalGroupDisposition::
                       complete_reduced_birth
                 : ExactDirectMorseVerticalGroupDisposition::
                       partial_reduced_birth;
    case ExactDirectMorseForestAtomicGroupKind::continuation:
      return complete
                 ? ExactDirectMorseVerticalGroupDisposition::
                       complete_continuation
                 : ExactDirectMorseVerticalGroupDisposition::
                       partial_continuation;
    case ExactDirectMorseForestAtomicGroupKind::multifusion:
      return complete
                 ? ExactDirectMorseVerticalGroupDisposition::
                       complete_multifusion
                 : ExactDirectMorseVerticalGroupDisposition::
                       partial_multifusion;
  }
  return ExactDirectMorseVerticalGroupDisposition::partial_reduced_birth;
}

[[nodiscard]] bool add_logical(
    std::size_t count,
    ExactDirectMorseVerticalJournalResult& result) noexcept {
  return checked_add(
             result.logical_output_entry_count,
             count,
             result.logical_output_entry_count) &&
         result.logical_output_entry_count <=
             result.requested_budget.maximum_logical_output_entry_count;
}

[[nodiscard]] bool storage_within(
    const ExactDirectMorseVerticalJournalResult& result,
    const ExactDirectMorseVerticalBudget& budget) noexcept {
  return result.adjacent_families.size() <=
             budget.maximum_adjacent_family_count &&
         result.label_resolutions.size() <=
             budget.maximum_label_resolution_count &&
         result.group_checks.size() <=
             budget.maximum_group_check_count &&
         result.checkpoints.size() <= budget.maximum_checkpoint_count &&
         result.logical_output_entry_count <=
             budget.maximum_logical_output_entry_count;
}

[[nodiscard]] std::vector<ExactDirectMorseForestNodeId> prior_roots(
    const ExactDirectMorseForestJournalResult& forest,
    const ExactDirectMorseForestAtomicGroup& group) {
  if (group.kind ==
      ExactDirectMorseForestAtomicGroupKind::reduced_birth) {
    return {};
  }
  if (group.kind ==
      ExactDirectMorseForestAtomicGroupKind::continuation) {
    return {group.resulting_root_node_id};
  }
  return {
      forest.child_node_ids.begin() +
          static_cast<std::ptrdiff_t>(group.child_offset),
      forest.child_node_ids.begin() +
          static_cast<std::ptrdiff_t>(
              group.child_offset + group.child_count)};
}

[[nodiscard]] bool result_facts_honest(
    const ExactDirectMorseVerticalJournalResult& result) noexcept {
  return result.conditional_on_caller_fresh_source_forest_replay &&
         !result.external_target_authority_replayed &&
         !result.global_morse_obligation_replayed &&
         !result.all_naturality_squares_replayed &&
         !result.vertical_maps_complete &&
         !result.gamma_cells_or_global_cofaces_materialized &&
         !result.higher_order_delaunay_materialized &&
         !result.public_status_claimed;
}

[[nodiscard]] bool two_part_partition(
    std::size_t first,
    std::size_t second,
    std::size_t total) noexcept {
  return first <= total && second == total - first;
}

[[nodiscard]] bool three_part_partition(
    std::size_t first,
    std::size_t second,
    std::size_t third,
    std::size_t total) noexcept {
  return first <= total && second <= total - first &&
         third == total - first - second;
}

}  // namespace

bool ExactDirectMorseVerticalJournalResult::
    certified_conditional_vertical_candidate() const noexcept {
  const bool complete_decision =
      decision ==
          ExactDirectMorseVerticalDecision::
              complete_conditional_partial_vertical_journal ||
      decision ==
          ExactDirectMorseVerticalDecision::
              complete_conditional_total_relative_vertical_journal;
  return schema_version == direct_morse_vertical_journal_schema_version &&
         complete_decision && budget_preflight_certified &&
         source_forest_shape_replayed &&
         representative_labels_reconstructed_without_key_copy &&
         missing_and_unresolved_labels_distinguished &&
         closed_target_roots_normalized_at_group_level &&
         all_group_conflicts_rejected_atomically &&
         elementary_group_square_partition_closed &&
         higher_order_isolated_births_have_no_source_node &&
         missing_target_never_classified_as_isolated &&
         scope ==
             ExactDirectMorseVerticalScope::
                 adjacent_reduced_group_labels_external_target_candidates_and_forest_propagation_only &&
         storage_within(*this, requested_budget) &&
         counters.expected_label_count == label_resolutions.size() &&
         three_part_partition(
             counters.missing_label_count,
             counters.unresolved_label_count,
             counters.resolved_label_count,
             counters.expected_label_count) &&
         two_part_partition(
             counters.complete_group_count,
             counters.partial_group_count,
             group_checks.size()) &&
         counters.checkpoint_count == checkpoints.size() &&
         two_part_partition(
             counters.checked_elementary_group_square_count,
             counters.unresolved_elementary_group_square_count,
             counters.expected_elementary_group_square_count) &&
         result_facts_honest(*this);
}

bool ExactDirectMorseVerticalJournalResult::certified_atomic_failure()
    const noexcept {
  return schema_version == direct_morse_vertical_journal_schema_version &&
         decision != ExactDirectMorseVerticalDecision::not_certified &&
         decision !=
             ExactDirectMorseVerticalDecision::
                 complete_conditional_partial_vertical_journal &&
         decision !=
             ExactDirectMorseVerticalDecision::
                 complete_conditional_total_relative_vertical_journal &&
         adjacent_families.empty() && label_resolutions.empty() &&
         group_checks.empty() && checkpoints.empty() &&
         logical_output_entry_count == 0U &&
         no_partial_scientific_payload_published_on_failure &&
         result_facts_honest(*this);
}

bool ExactDirectMorseVerticalJournalResult::certified_outcome()
    const noexcept {
  return certified_conditional_vertical_candidate() ||
         certified_atomic_failure();
}

ExactDirectMorseVerticalJournalResult
build_exact_direct_morse_vertical_journal(
    const ExactDirectMorseForestJournalResult& source_forest,
    std::span<const ExactDirectMorseVerticalTargetProposal> proposals,
    const ExactDirectMorseVerticalBudget& budget,
    const ExactDirectMorseVerticalConfig& config) {
  ExactDirectMorseVerticalJournalResult result;
  result.config = config;
  result.requested_budget = budget;
  result.point_count = source_forest.point_count;
  result.effective_maximum_order =
      source_forest.effective_maximum_order;
  if (config.external_target_authority_id == 0U) {
    return fail(
        std::move(result),
        ExactDirectMorseVerticalDecision::
            no_vertical_proposal_partition_rejected);
  }
  if (proposals.size() > budget.maximum_proposal_count) {
    return fail(
        std::move(result),
        ExactDirectMorseVerticalDecision::no_vertical_budget_exhausted);
  }

  try {
    ForestShape shape;
    const ShapeStatus shape_status = reconstruct_shape(
        source_forest, budget, result.counters, shape);
    if (shape_status == ShapeStatus::budget_exhausted) {
      return fail(
          std::move(result),
          ExactDirectMorseVerticalDecision::no_vertical_budget_exhausted);
    }
    if (shape_status != ShapeStatus::okay) {
      return fail(
          std::move(result),
          ExactDirectMorseVerticalDecision::
              no_vertical_forest_shape_rejected);
    }
    result.source_forest_shape_replayed = true;

    std::vector<
        std::pair<ExactDirectMorseVerticalTargetProposal, std::size_t>>
        sorted_proposals;
    sorted_proposals.reserve(proposals.size());
    for (std::size_t index = 0U; index < proposals.size(); ++index) {
      sorted_proposals.push_back({proposals[index], index});
    }
    std::sort(
        sorted_proposals.begin(),
        sorted_proposals.end(),
        proposal_less);
    for (std::size_t index = 0U;
         index < sorted_proposals.size();
         ++index) {
      const auto& proposal = sorted_proposals[index].first;
      if (proposal.replay_token == 0U ||
          proposal.representative_arm_root_binding_index >=
              source_forest.arm_root_bindings.size() ||
          (index != 0U &&
           sorted_proposals[index - 1U]
                   .first.representative_arm_root_binding_index ==
               proposal.representative_arm_root_binding_index) ||
          (proposal.disposition ==
               ExactDirectMorseVerticalProposalDisposition::unresolved &&
           proposal.target_seed_node_id.has_value()) ||
          (proposal.disposition ==
               ExactDirectMorseVerticalProposalDisposition::
                   resolved_target_seed &&
           !proposal.target_seed_node_id.has_value())) {
        return fail(
            std::move(result),
            ExactDirectMorseVerticalDecision::
                no_vertical_proposal_partition_rejected);
      }
    }

    std::vector<bool> expected_representative(
        source_forest.arm_root_bindings.size(), false);
    std::size_t expected_label_count = 0U;
    for (const auto& plan : shape.group_plans) {
      if (!checked_add(
              expected_label_count,
              plan.representative_binding_indices.size(),
              expected_label_count)) {
        return fail(
            std::move(result),
            ExactDirectMorseVerticalDecision::
                no_vertical_capacity_overflow);
      }
      for (const std::size_t binding :
           plan.representative_binding_indices) {
        expected_representative[binding] = true;
      }
    }
    for (const auto& proposal : sorted_proposals) {
      if (!expected_representative[
              proposal.first
                  .representative_arm_root_binding_index]) {
        return fail(
            std::move(result),
            ExactDirectMorseVerticalDecision::
                no_vertical_proposal_partition_rejected);
      }
    }
    if (expected_label_count >
            budget.maximum_label_resolution_count ||
        source_forest.atomic_groups.size() >
            budget.maximum_group_check_count ||
        source_forest.effective_maximum_order - 1U >
            budget.maximum_adjacent_family_count) {
      return fail(
          std::move(result),
          ExactDirectMorseVerticalDecision::no_vertical_budget_exhausted);
    }
    result.budget_preflight_certified = true;
    result.label_resolutions.reserve(expected_label_count);
    result.group_checks.reserve(source_forest.atomic_groups.size());
    result.adjacent_families.reserve(
        source_forest.effective_maximum_order - 1U);
    result.checkpoints.reserve(std::min(
        budget.maximum_checkpoint_count,
        source_forest.atomic_groups.size()));

    std::vector<std::optional<std::size_t>> latest_checkpoint(
        source_forest.nodes.size());
    std::vector<bool> consumed_proposals(
        sorted_proposals.size(), false);
    for (std::size_t source_order = 2U;
         source_order <= source_forest.effective_maximum_order;
         ++source_order) {
      ExactDirectMorseVerticalAdjacentFamily family;
      family.family_index = result.adjacent_families.size();
      family.source_order = source_order;
      family.target_order = source_order - 1U;
      family.group_check_offset = result.group_checks.size();
      family.label_resolution_offset = result.label_resolutions.size();
      family.checkpoint_offset = result.checkpoints.size();
      family.source_reduced_node_count =
          shape.nodes_by_order[source_order].size();
      family.omitted_isolated_source_birth_count =
          shape.omitted_births_by_order[source_order];

      TargetSweep target_sweep{
          source_forest,
          shape,
          source_order - 1U,
          budget,
          result.counters};
      for (const auto& plan : shape.group_plans) {
        const auto& group =
            source_forest.atomic_groups[plan.group_index];
        const auto& batch =
            source_forest.batches[group.batch_index];
        if (batch.order != source_order) {
          continue;
        }
        if (!target_sweep.advance(batch.squared_level)) {
          return fail(
              std::move(result),
              target_sweep.budget_exhausted()
                  ? ExactDirectMorseVerticalDecision::
                        no_vertical_budget_exhausted
                  : ExactDirectMorseVerticalDecision::
                        no_vertical_forest_shape_rejected);
        }

        ExactDirectMorseVerticalGroupCheck check;
        check.group_check_index = result.group_checks.size();
        check.atomic_group_index = plan.group_index;
        check.source_batch_index = group.batch_index;
        check.label_resolution_offset =
            result.label_resolutions.size();
        const auto roots = prior_roots(source_forest, group);
        check.expected_elementary_group_square_count = roots.size();
        std::optional<ExactDirectMorseForestNodeId> common_target;
        bool conflict = false;
        bool all_labels_resolved = true;

        for (const std::size_t representative :
             plan.representative_binding_indices) {
          const auto proposal_iterator = std::lower_bound(
              sorted_proposals.begin(),
              sorted_proposals.end(),
              representative,
              [](const auto& candidate, std::size_t binding_index) {
                return candidate.first
                           .representative_arm_root_binding_index <
                       binding_index;
              });
          const bool proposal_present =
              proposal_iterator != sorted_proposals.end() &&
              proposal_iterator->first
                      .representative_arm_root_binding_index ==
                  representative;
          ExactDirectMorseVerticalLabelResolution resolution;
          resolution.label_resolution_index =
              result.label_resolutions.size();
          resolution.atomic_group_index = plan.group_index;
          resolution.representative_arm_root_binding_index =
              representative;
          ++result.counters.expected_label_count;
          if (!proposal_present) {
            resolution.disposition =
                ExactDirectMorseVerticalLabelDisposition::missing;
            ++result.counters.missing_label_count;
            all_labels_resolved = false;
          } else {
            const auto proposal_index = static_cast<std::size_t>(
                proposal_iterator - sorted_proposals.begin());
            if (consumed_proposals[proposal_index]) {
              return fail(
                  std::move(result),
                  ExactDirectMorseVerticalDecision::
                      no_vertical_proposal_partition_rejected);
            }
            consumed_proposals[proposal_index] = true;
            const auto& proposal = proposal_iterator->first;
            resolution.source_proposal_index =
                proposal_iterator->second;
            if (proposal.disposition ==
                ExactDirectMorseVerticalProposalDisposition::unresolved) {
              resolution.disposition =
                  ExactDirectMorseVerticalLabelDisposition::unresolved;
              ++result.counters.unresolved_label_count;
              all_labels_resolved = false;
            } else {
              const auto seed = *proposal.target_seed_node_id;
              if (seed >= source_forest.nodes.size() ||
                  source_forest.nodes[seed].order != source_order - 1U) {
                return fail(
                    std::move(result),
                    ExactDirectMorseVerticalDecision::
                        no_vertical_target_rejected);
              }
              if (result.counters.exact_level_comparison_count >=
                  budget.maximum_exact_level_comparison_count) {
                return fail(
                    std::move(result),
                    ExactDirectMorseVerticalDecision::
                        no_vertical_budget_exhausted);
              }
              ++result.counters.exact_level_comparison_count;
              if (source_forest.nodes[seed].squared_level >
                  batch.squared_level) {
                return fail(
                    std::move(result),
                    ExactDirectMorseVerticalDecision::
                        no_vertical_target_rejected);
              }
              const auto root = target_sweep.find(seed);
              if (!root.has_value()) {
                return fail(
                    std::move(result),
                    target_sweep.budget_exhausted()
                        ? ExactDirectMorseVerticalDecision::
                              no_vertical_budget_exhausted
                        : ExactDirectMorseVerticalDecision::
                              no_vertical_target_rejected);
              }
              resolution.closed_target_root_node_id = *root;
              resolution.disposition =
                  ExactDirectMorseVerticalLabelDisposition::
                      resolved_closed_target_root;
              ++result.counters.resolved_label_count;
              if (!common_target.has_value()) {
                common_target = *root;
              } else if (*common_target != *root) {
                conflict = true;
              }
            }
          }
          result.label_resolutions.push_back(std::move(resolution));
        }
        check.label_resolution_count =
            result.label_resolutions.size() -
            check.label_resolution_offset;

        bool all_prior_complete = true;
        std::size_t present_prior_count = 0U;
        std::size_t complete_agreeing_prior_count = 0U;
        for (const auto root : roots) {
          if (root >= latest_checkpoint.size() ||
              !latest_checkpoint[root].has_value()) {
            all_prior_complete = false;
            continue;
          }
          const auto& prior_checkpoint =
              result.checkpoints[*latest_checkpoint[root]];
          const auto lifted = target_sweep.find(
              prior_checkpoint.closed_target_root_node_id);
          if (!lifted.has_value()) {
            return fail(
                std::move(result),
                target_sweep.budget_exhausted()
                    ? ExactDirectMorseVerticalDecision::
                          no_vertical_budget_exhausted
                    : ExactDirectMorseVerticalDecision::
                          no_vertical_target_rejected);
          }
          ++present_prior_count;
          if (!prior_checkpoint
                   .complete_relative_to_supplied_proposals) {
            all_prior_complete = false;
          } else {
            ++complete_agreeing_prior_count;
          }
          if (!common_target.has_value()) {
            common_target = *lifted;
          } else if (*common_target != *lifted) {
            conflict = true;
          }
        }
        if (conflict) {
          return fail(
              std::move(result),
              ExactDirectMorseVerticalDecision::
                  no_vertical_relative_target_conflict);
        }

        const bool complete =
            all_labels_resolved && all_prior_complete;
        check.checked_elementary_group_square_count =
            all_labels_resolved
                ? complete_agreeing_prior_count
                : 0U;
        check.unresolved_elementary_group_square_count =
            roots.size() -
            check.checked_elementary_group_square_count;
        result.counters.expected_elementary_group_square_count +=
            roots.size();
        result.counters.checked_elementary_group_square_count +=
            check.checked_elementary_group_square_count;
        result.counters.unresolved_elementary_group_square_count +=
            check.unresolved_elementary_group_square_count;
        check.disposition = disposition_for(group.kind, complete);
        if (complete) {
          ++result.counters.complete_group_count;
        } else {
          ++result.counters.partial_group_count;
        }

        const bool can_checkpoint =
            common_target.has_value() &&
            (group.kind !=
                 ExactDirectMorseForestAtomicGroupKind::reduced_birth ||
             all_labels_resolved);
        if (can_checkpoint) {
          if (result.checkpoints.size() >=
              budget.maximum_checkpoint_count) {
            return fail(
                std::move(result),
                ExactDirectMorseVerticalDecision::
                    no_vertical_budget_exhausted);
          }
          ExactDirectMorseVerticalCheckpoint checkpoint;
          checkpoint.checkpoint_index = result.checkpoints.size();
          checkpoint.atomic_group_index = plan.group_index;
          checkpoint.source_root_node_id =
              group.resulting_root_node_id;
          checkpoint.closed_target_root_node_id = *common_target;
          checkpoint.complete_relative_to_supplied_proposals =
              complete;
          if (group.kind ==
              ExactDirectMorseForestAtomicGroupKind::reduced_birth) {
            checkpoint.kind =
                ExactDirectMorseVerticalCheckpointKind::
                    reduced_birth_anchor;
          } else if (
              group.kind ==
              ExactDirectMorseForestAtomicGroupKind::continuation) {
            const bool late = present_prior_count == 0U;
            checkpoint.kind =
                late
                    ? ExactDirectMorseVerticalCheckpointKind::
                          late_continuation_anchor
                    : ExactDirectMorseVerticalCheckpointKind::
                          continuation_propagation;
            if (late) {
              ++result.counters.late_checkpoint_count;
              checkpoint.complete_relative_to_supplied_proposals =
                  false;
            }
          } else {
            checkpoint.kind =
                ExactDirectMorseVerticalCheckpointKind::
                    multifusion_propagation;
          }
          latest_checkpoint[checkpoint.source_root_node_id] =
              checkpoint.checkpoint_index;
          check.checkpoint_index = checkpoint.checkpoint_index;
          result.checkpoints.push_back(std::move(checkpoint));
          ++result.counters.checkpoint_count;
        }
        result.group_checks.push_back(std::move(check));
      }
      family.group_check_count =
          result.group_checks.size() - family.group_check_offset;
      family.label_resolution_count =
          result.label_resolutions.size() -
          family.label_resolution_offset;
      family.checkpoint_count =
          result.checkpoints.size() - family.checkpoint_offset;
      family.complete_relative_to_supplied_proposals =
          std::all_of(
              result.group_checks.begin() +
                  static_cast<std::ptrdiff_t>(
                      family.group_check_offset),
              result.group_checks.end(),
              [](const ExactDirectMorseVerticalGroupCheck& check) {
                return group_complete(check.disposition);
              });
      result.adjacent_families.push_back(std::move(family));
    }

    if (!std::all_of(
            consumed_proposals.begin(),
            consumed_proposals.end(),
            [](bool consumed) { return consumed; })) {
      return fail(
          std::move(result),
          ExactDirectMorseVerticalDecision::
              no_vertical_proposal_partition_rejected);
    }
    if (!add_logical(result.adjacent_families.size(), result) ||
        !add_logical(result.label_resolutions.size(), result) ||
        !add_logical(result.group_checks.size(), result) ||
        !add_logical(result.checkpoints.size(), result)) {
      return fail(
          std::move(result),
          ExactDirectMorseVerticalDecision::no_vertical_budget_exhausted);
    }
    result.representative_labels_reconstructed_without_key_copy = true;
    result.missing_and_unresolved_labels_distinguished = true;
    result.closed_target_roots_normalized_at_group_level = true;
    result.all_group_conflicts_rejected_atomically = true;
    result.elementary_group_square_partition_closed =
        result.counters.checked_elementary_group_square_count +
                result.counters.unresolved_elementary_group_square_count ==
            result.counters.expected_elementary_group_square_count;
    result.higher_order_isolated_births_have_no_source_node = true;
    result.missing_target_never_classified_as_isolated = true;
    result.scope =
        ExactDirectMorseVerticalScope::
            adjacent_reduced_group_labels_external_target_candidates_and_forest_propagation_only;
    const bool total_relative =
        result.counters.missing_label_count == 0U &&
        result.counters.unresolved_label_count == 0U &&
        result.counters.partial_group_count == 0U;
    result.decision =
        total_relative
            ? ExactDirectMorseVerticalDecision::
                  complete_conditional_total_relative_vertical_journal
            : ExactDirectMorseVerticalDecision::
                  complete_conditional_partial_vertical_journal;
    return result;
  } catch (const std::bad_alloc&) {
    return fail(
        std::move(result),
        ExactDirectMorseVerticalDecision::no_vertical_allocation_failed);
  } catch (...) {
    return fail(
        std::move(result),
        ExactDirectMorseVerticalDecision::
            no_vertical_source_forest_rejected);
  }
}

ExactDirectMorseVerticalVerification
verify_exact_direct_morse_vertical_journal(
    const ExactDirectMorseForestJournalResult& source_forest,
    std::span<const ExactDirectMorseVerticalTargetProposal> proposals,
    const ExactDirectMorseVerticalBudget& trusted_budget,
    const ExactDirectMorseVerticalConfig& config,
    const ExactDirectMorseVerticalJournalResult& observed) {
  ExactDirectMorseVerticalVerification verification;
  verification.observed_storage_within_budget =
      storage_within(observed, trusted_budget);
  if (!verification.observed_storage_within_budget) {
    return verification;
  }
  const auto expected = build_exact_direct_morse_vertical_journal(
      source_forest, proposals, trusted_budget, config);
  verification.expected_journal_freshly_reconstructed =
      expected.certified_outcome();
  verification.observed_recursively_equal = expected == observed;
  verification.source_forest_shape_replayed =
      expected.source_forest_shape_replayed;
  verification.result_certified =
      verification.expected_journal_freshly_reconstructed &&
      verification.observed_recursively_equal &&
      observed.certified_outcome();
  return verification;
}

namespace {

[[nodiscard]] std::optional<ExactDirectMorseForestNodeId>
root_at_level_with_budget(
    const ExactDirectMorseForestJournalResult& forest,
    const std::vector<ExactDirectMorseForestNodeId>& parent_by_node,
    ExactDirectMorseForestNodeId node,
    const exact::ExactLevel& level,
    const ExactDirectMorseVerticalTraceBudget& budget,
    ExactDirectMorseVerticalTraceResult& result) {
  if (node >= forest.nodes.size() ||
      forest.nodes[node].squared_level > level) {
    return std::nullopt;
  }
  while (parent_by_node[node] != no_node) {
    if (result.parent_hop_count >=
        budget.maximum_parent_hop_count) {
      result.disposition =
          ExactDirectMorseVerticalTraceDisposition::budget_exhausted;
      return std::nullopt;
    }
    const auto parent = parent_by_node[node];
    if (forest.nodes[parent].squared_level > level) {
      break;
    }
    ++result.parent_hop_count;
    node = parent;
  }
  return node;
}

}  // namespace

ExactDirectMorseVerticalTraceResult
trace_exact_direct_morse_vertical_component(
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseVerticalJournalResult& vertical_journal,
    ExactDirectMorseForestNodeId source_node_id,
    const exact::ExactLevel& at_squared_level,
    std::size_t target_order,
    const ExactDirectMorseVerticalTraceBudget& budget) {
  ExactDirectMorseVerticalTraceResult result;
  result.at_squared_level = at_squared_level;
  result.requested_target_order = target_order;
  if (!vertical_journal.certified_conditional_vertical_candidate() ||
      source_node_id >= source_forest.nodes.size()) {
    result.disposition =
        ExactDirectMorseVerticalTraceDisposition::invalid_query;
    return result;
  }
  result.requested_source_order =
      source_forest.nodes[source_node_id].order;
  if (target_order == 0U ||
      target_order >= result.requested_source_order ||
      result.requested_source_order - target_order >
          budget.maximum_adjacent_step_count) {
    result.disposition =
        ExactDirectMorseVerticalTraceDisposition::invalid_query;
    return result;
  }

  std::vector<ExactDirectMorseForestNodeId> parent(
      source_forest.nodes.size(), no_node);
  for (const auto& node : source_forest.nodes) {
    if (node.child_offset > source_forest.child_node_ids.size() ||
        node.child_count >
            source_forest.child_node_ids.size() - node.child_offset) {
      result.disposition =
          ExactDirectMorseVerticalTraceDisposition::invalid_query;
      return result;
    }
    for (std::size_t local = 0U; local < node.child_count; ++local) {
      const auto child =
          source_forest.child_node_ids[node.child_offset + local];
      if (child >= parent.size() || parent[child] != no_node) {
        result.disposition =
            ExactDirectMorseVerticalTraceDisposition::invalid_query;
        return result;
      }
      parent[child] = node.node_id;
    }
  }

  bool every_checkpoint_complete = true;
  ExactDirectMorseForestNodeId current = source_node_id;
  for (std::size_t order = result.requested_source_order;
       order > target_order;
       --order) {
    const auto source_root = root_at_level_with_budget(
        source_forest,
        parent,
        current,
        at_squared_level,
        budget,
        result);
    if (!source_root.has_value()) {
      if (result.disposition !=
          ExactDirectMorseVerticalTraceDisposition::budget_exhausted) {
        result.disposition =
            ExactDirectMorseVerticalTraceDisposition::invalid_query;
      }
      return result;
    }
    std::optional<std::size_t> checkpoint_index;
    for (std::size_t index = vertical_journal.checkpoints.size();
         index > 0U;
         --index) {
      if (result.checkpoint_scan_count >=
          budget.maximum_checkpoint_scan_count) {
        result.disposition =
            ExactDirectMorseVerticalTraceDisposition::budget_exhausted;
        return result;
      }
      ++result.checkpoint_scan_count;
      const auto& checkpoint =
          vertical_journal.checkpoints[index - 1U];
      if (checkpoint.source_root_node_id == *source_root) {
        const auto& group = source_forest.atomic_groups[
            checkpoint.atomic_group_index];
        const auto& batch =
            source_forest.batches[group.batch_index];
        if (batch.squared_level <= at_squared_level) {
          checkpoint_index = index - 1U;
          break;
        }
      }
    }
    if (!checkpoint_index.has_value()) {
      result.disposition =
          ExactDirectMorseVerticalTraceDisposition::
              unresolved_missing_checkpoint;
      return result;
    }
    const auto& checkpoint =
        vertical_journal.checkpoints[*checkpoint_index];
    const auto target_root = root_at_level_with_budget(
        source_forest,
        parent,
        checkpoint.closed_target_root_node_id,
        at_squared_level,
        budget,
        result);
    if (!target_root.has_value() ||
        source_forest.nodes[*target_root].order != order - 1U) {
      if (result.disposition !=
          ExactDirectMorseVerticalTraceDisposition::budget_exhausted) {
        result.disposition =
            ExactDirectMorseVerticalTraceDisposition::invalid_query;
      }
      return result;
    }
    result.steps.push_back(
        {result.steps.size(),
         order,
         order - 1U,
         *source_root,
         *target_root,
         *checkpoint_index,
         checkpoint.complete_relative_to_supplied_proposals});
    every_checkpoint_complete =
        every_checkpoint_complete &&
        checkpoint.complete_relative_to_supplied_proposals;
    current = *target_root;
  }
  result.disposition =
      every_checkpoint_complete
          ? ExactDirectMorseVerticalTraceDisposition::
                complete_relative_trace
          : ExactDirectMorseVerticalTraceDisposition::
                partial_relative_trace;
  return result;
}

}  // namespace morsehgp3d::hierarchy
