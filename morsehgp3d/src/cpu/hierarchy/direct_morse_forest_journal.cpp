#include "morsehgp3d/hierarchy/direct_morse_forest_journal.hpp"

#include <algorithm>
#include <array>
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

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

enum class BuildFailure : std::uint8_t {
  capacity_overflow,
  allocation_failed,
  budget_exhausted,
  source_rejected,
  locator_initialization,
  source_batch_inconsistent,
  closure_budget_exhausted,
  closure_unresolved,
  closure_contradiction,
  zero_carrier_saddle,
  quotient_rejected,
  locator_commit_rejected,
};

[[nodiscard]] bool try_add(
    std::size_t left,
    std::size_t right,
    std::size_t& sum) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  sum = left + right;
  return true;
}

[[nodiscard]] bool try_multiply(
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

[[nodiscard]] bool checked_increment(std::size_t& value) noexcept {
  if (value == std::numeric_limits<std::size_t>::max()) {
    return false;
  }
  ++value;
  return true;
}

[[nodiscard]] bool facet_key_less(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right) noexcept {
  const std::size_t common = std::min(left.point_count, right.point_count);
  for (std::size_t index = 0U; index < common; ++index) {
    if (left.point_ids[index] != right.point_ids[index]) {
      return left.point_ids[index] < right.point_ids[index];
    }
  }
  return left.point_count < right.point_count;
}

[[nodiscard]] bool valid_key(
    const ExactDirectSparseFacetKey& key,
    std::size_t point_count,
    std::size_t order) noexcept {
  if (key.point_count != order || order == 0U ||
      order > direct_sparse_positive_facet_maximum_point_count ||
      order > point_count) {
    return false;
  }
  for (std::size_t index = 0U; index < order; ++index) {
    if (static_cast<std::size_t>(key.point_ids[index]) >= point_count ||
        (index != 0U &&
         key.point_ids[index - 1U] >= key.point_ids[index])) {
      return false;
    }
  }
  for (std::size_t index = order; index < key.point_ids.size(); ++index) {
    if (key.point_ids[index] != 0U) {
      return false;
    }
  }
  return true;
}

void require_valid_traversal_order(
    spatial::LbvhTraversalOrder traversal_order) {
  switch (traversal_order) {
    case spatial::LbvhTraversalOrder::near_first:
    case spatial::LbvhTraversalOrder::far_first:
      return;
  }
  throw std::invalid_argument(
      "a direct Morse forest traversal order is invalid");
}

void require_closure_budget_within_confidence(
    const ExactDirectSparseFacetDescentClosureBudget& budget) {
  if (budget.maximum_seed_count >
          direct_sparse_facet_descent_closure_maximum_seed_count ||
      budget.maximum_node_count >
          direct_sparse_facet_descent_closure_maximum_node_count ||
      budget.maximum_step_call_count >
          direct_sparse_facet_descent_closure_maximum_step_call_count ||
      budget.maximum_memo_slot_count >
          direct_sparse_facet_descent_closure_maximum_memo_slot_count) {
    throw std::invalid_argument(
        "a direct Morse forest closure budget exceeds its confidence cap");
  }
}

[[nodiscard]] std::optional<std::uint64_t> replay_token(
    std::size_t index,
    std::uint64_t residue) noexcept {
  constexpr std::uint64_t modulus = 3U;
  if (residue == 0U || residue > modulus ||
      index >
          (std::numeric_limits<std::uint64_t>::max() - residue) /
              modulus) {
    return std::nullopt;
  }
  return static_cast<std::uint64_t>(index) * modulus + residue;
}

[[nodiscard]] std::optional<std::uint64_t> query_replay_token(
    std::size_t batch_index) noexcept {
  constexpr std::uint64_t modulus = 3U;
  if (batch_index >
      std::numeric_limits<std::uint64_t>::max() / modulus - 1U) {
    return std::nullopt;
  }
  return (static_cast<std::uint64_t>(batch_index) + 1U) * modulus;
}

[[nodiscard]] ExactDirectSparseFacetKey birth_key(
    const ExactDirectMorseEventProjection& projection,
    const ExactDirectSupportTerminalFacade& facade,
    std::size_t point_count,
    std::size_t order,
    const exact::ExactLevel& level) {
  if (projection.event_projection_index >=
          facade.events.size() + point_count ||
      projection.birth_order != std::optional<std::size_t>{order} ||
      projection.squared_level != level ||
      projection.closed_rank != order) {
    throw std::logic_error(
        "a direct Morse birth projection disagrees with its batch");
  }

  ExactDirectSparseFacetKey key;
  key.point_count = order;
  if (projection.source ==
      ExactDirectMorseEventSource::canonical_singleton) {
    if (order != 1U || projection.source_index >= point_count ||
        projection.support_size != 1U ||
        projection.support_ids[0U] !=
            static_cast<PointId>(projection.source_index)) {
      throw std::logic_error(
          "a canonical singleton birth has an invalid identity");
    }
    key.point_ids[0U] =
        static_cast<PointId>(projection.source_index);
    return key;
  }

  if (projection.source !=
          ExactDirectMorseEventSource::direct_support_terminal_event ||
      projection.source_index >= facade.events.size()) {
    throw std::logic_error(
        "a direct-support birth has no Phase-9 source event");
  }
  const ExactDirectSupportEvent& event =
      facade.events[projection.source_index];
  const std::size_t support_size =
      static_cast<std::size_t>(event.support_size);
  if (event.event_index != projection.source_index ||
      event.birth_order != std::optional<std::size_t>{order} ||
      event.squared_level != level || event.closed_rank != order ||
      event.support_size != projection.support_size ||
      event.support_ids != projection.support_ids ||
      support_size + event.interior_ids.size() != order) {
    throw std::logic_error(
        "a direct-support birth projection changed Phase-9 identity");
  }
  std::size_t write = 0U;
  for (const PointId point_id : event.interior_ids) {
    key.point_ids[write] = point_id;
    ++write;
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    key.point_ids[write] = event.support_ids[index];
    ++write;
  }
  std::sort(
      key.point_ids.begin(),
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(write));
  if (write != order || !valid_key(key, point_count, order)) {
    throw std::logic_error(
        "a direct-support birth did not reconstruct one canonical key");
  }
  return key;
}

[[nodiscard]] ExactDirectSparseFacetKey arm_key(
    const ExactDirectSaddleArmFacet& facet,
    std::size_t point_count,
    std::size_t order) {
  ExactDirectSparseFacetKey key;
  key.point_count = facet.point_count;
  key.point_ids = facet.point_ids;
  if (!valid_key(key, point_count, order)) {
    throw std::logic_error(
        "a strict direct saddle arm is not one canonical order-k key");
  }
  return key;
}

class ForestComponents {
 public:
  explicit ForestComponents(std::size_t handle_count)
      : parent_(handle_count),
        reduced_root_node_ids_(handle_count),
        orders_(handle_count),
        active_(handle_count, false) {
    std::iota(parent_.begin(), parent_.end(), std::size_t{0U});
  }

  [[nodiscard]] std::size_t root(std::size_t handle) const {
    if (handle >= parent_.size()) {
      throw std::logic_error("a forest component handle is out of range");
    }
    while (parent_[handle] != handle) {
      handle = parent_[handle];
    }
    return handle;
  }

  [[nodiscard]] bool active(std::size_t handle) const {
    return handle < active_.size() && active_[root(handle)];
  }

  [[nodiscard]] std::size_t order(std::size_t handle) const {
    const std::size_t component = root(handle);
    if (!active_[component]) {
      throw std::logic_error("an inactive forest component has no order");
    }
    return orders_[component];
  }

  [[nodiscard]] std::optional<ExactDirectMorseForestNodeId> reduced_root(
      std::size_t handle) const {
    const std::size_t component = root(handle);
    if (!active_[component]) {
      throw std::logic_error(
          "an inactive forest component has no reduced-root state");
    }
    return reduced_root_node_ids_[component];
  }

  void activate(
      std::size_t handle,
      std::size_t order,
      std::optional<ExactDirectMorseForestNodeId>
          initial_reduced_root_node_id) {
    if (handle >= parent_.size() || active_[handle] ||
        parent_[handle] != handle || order == 0U ||
        (order == 1U) != initial_reduced_root_node_id.has_value()) {
      throw std::logic_error(
          "a direct Morse minimum has an invalid carrier activation");
    }
    active_[handle] = true;
    orders_[handle] = order;
    reduced_root_node_ids_[handle] = initial_reduced_root_node_id;
  }

  [[nodiscard]] std::size_t unite(
      std::size_t left,
      std::size_t right) {
    left = root(left);
    right = root(right);
    if (!active_[left] || !active_[right] ||
        orders_[left] != orders_[right]) {
      throw std::logic_error(
          "a forest union crossed an inactive handle or an order");
    }
    if (left == right) {
      return left;
    }
    const std::size_t result = std::min(left, right);
    const std::size_t child = std::max(left, right);
    parent_[child] = result;
    return result;
  }

  void set_reduced_root(
      std::size_t handle,
      ExactDirectMorseForestNodeId node_id) {
    reduced_root_node_ids_[root(handle)] = node_id;
  }

  [[nodiscard]] std::size_t carrier_count(std::size_t order) const noexcept {
    std::size_t count = 0U;
    for (std::size_t handle = 0U; handle < parent_.size(); ++handle) {
      if (active_[handle] && parent_[handle] == handle &&
          orders_[handle] == order) {
        ++count;
      }
    }
    return count;
  }

  [[nodiscard]] std::size_t reduced_root_count(
      std::size_t order) const noexcept {
    std::size_t count = 0U;
    for (std::size_t handle = 0U; handle < parent_.size(); ++handle) {
      if (active_[handle] && parent_[handle] == handle &&
          orders_[handle] == order &&
          reduced_root_node_ids_[handle].has_value()) {
        ++count;
      }
    }
    return count;
  }

  [[nodiscard]] std::size_t handle_count() const noexcept {
    return parent_.size();
  }

  [[nodiscard]] bool is_active_carrier_root(
      std::size_t handle) const noexcept {
    return handle < parent_.size() && active_[handle] &&
           parent_[handle] == handle;
  }

 private:
  std::vector<std::size_t> parent_;
  std::vector<std::optional<ExactDirectMorseForestNodeId>>
      reduced_root_node_ids_;
  std::vector<std::size_t> orders_;
  std::vector<bool> active_;
};

struct TemporaryArm {
  std::size_t source_seed_index{};
  ExactDirectSparseFacetKey key{};
  ExactDirectSparseComponentHandle carrier_handle{};
  std::optional<ExactDirectMorseForestNodeId> prior_reduced_root_node_id;
};

struct TemporarySaddle {
  std::size_t source_family_index{};
  std::vector<TemporaryArm> arms;
};

struct ResolvedKey {
  ExactDirectSparseFacetKey key{};
  ExactDirectSparseComponentHandle carrier_handle{};
  std::optional<ExactDirectMorseForestNodeId> prior_reduced_root_node_id;
};

struct GroupPlan {
  std::vector<std::size_t> saddle_indices;
  std::vector<ExactDirectSparseComponentHandle> carrier_handles;
  std::vector<ExactDirectMorseForestNodeId> prior_reduced_root_node_ids;
  std::optional<ExactDirectMorseForestNodeId> created_node_id;
  ExactDirectMorseForestNodeId resulting_root_node_id{};
  ExactDirectMorseForestAtomicGroupKind kind{
      ExactDirectMorseForestAtomicGroupKind::reduced_birth};
  std::size_t atomic_group_index{};
};

void initialize_scope(ExactDirectMorseForestJournalResult& result) noexcept {
  result.source_phase9_facade_freshly_replayed = false;
  result.conditional_on_caller_fresh_phase9_facade_replay = true;
  result.external_locator_authority_replayed = true;
  result.conditional_on_caller_external_locator_authority_replay = false;
  result.global_morse_obligation_replayed = false;
  result.conditional_on_separate_global_morse_obligation = true;
  result.equal_or_interior_facets_consumed = false;
  result.gateway_10_6_or_later_consumed = false;
  result.closure_graph_persisted = false;
  result.gamma_cells_or_global_cofaces_materialized = false;
  result.higher_order_delaunay_materialized = false;
  result.forbidden_global_structure_materialized = false;
  result.public_status_claimed = false;
  result.conditional_exact_h0_only = true;
  result.scope = ExactDirectMorseForestScope::
      all_orders_direct_minimum_carriers_strict_arms_recursive_positive_terminals_and_atomic_full_component_saddle_quotients_with_reduced_qr_only;
}

[[nodiscard]] bool non_scope_is_honest(
    const ExactDirectMorseForestJournalResult& result) noexcept {
  return !result.source_phase9_facade_freshly_replayed &&
         result.conditional_on_caller_fresh_phase9_facade_replay &&
         result.external_locator_authority_replayed &&
         !result.conditional_on_caller_external_locator_authority_replay &&
         !result.global_morse_obligation_replayed &&
         result.conditional_on_separate_global_morse_obligation &&
         !result.equal_or_interior_facets_consumed &&
         !result.gateway_10_6_or_later_consumed &&
         !result.closure_graph_persisted &&
         !result.gamma_cells_or_global_cofaces_materialized &&
         !result.higher_order_delaunay_materialized &&
         !result.forbidden_global_structure_materialized &&
         !result.public_status_claimed && result.conditional_exact_h0_only &&
         result.scope == ExactDirectMorseForestScope::
             all_orders_direct_minimum_carriers_strict_arms_recursive_positive_terminals_and_atomic_full_component_saddle_quotients_with_reduced_qr_only;
}

void clear_payload(ExactDirectMorseForestJournalResult& result) noexcept {
  result.birth_records.clear();
  result.arm_root_bindings.clear();
  result.saddle_records.clear();
  result.atomic_groups.clear();
  result.child_node_ids.clear();
  result.batches.clear();
  result.final_roots.clear();
  result.nodes.clear();
  result.logical_output_entry_count = 0U;
  result.counters = {};
  result.final_locator_stamp = {};
  result.no_partial_scientific_payload_published = true;
}

[[nodiscard]] ExactDirectMorseForestJournalResult fail(
    ExactDirectMorseForestJournalResult result,
    BuildFailure failure) {
  clear_payload(result);
  switch (failure) {
    case BuildFailure::capacity_overflow:
      result.decision =
          ExactDirectMorseForestDecision::no_forest_capacity_overflow;
      break;
    case BuildFailure::allocation_failed:
      result.decision =
          ExactDirectMorseForestDecision::no_forest_allocation_failed;
      break;
    case BuildFailure::budget_exhausted:
      result.decision =
          ExactDirectMorseForestDecision::no_forest_budget_exhausted;
      break;
    case BuildFailure::source_rejected:
      result.decision =
          ExactDirectMorseForestDecision::no_forest_source_rejected;
      break;
    case BuildFailure::locator_initialization:
      result.decision =
          ExactDirectMorseForestDecision::no_forest_locator_initialization;
      break;
    case BuildFailure::source_batch_inconsistent:
      result.decision =
          ExactDirectMorseForestDecision::
              no_forest_source_batch_inconsistent;
      break;
    case BuildFailure::closure_budget_exhausted:
      result.decision =
          ExactDirectMorseForestDecision::
              no_forest_strict_arm_closure_budget_exhausted;
      break;
    case BuildFailure::closure_unresolved:
      result.decision =
          ExactDirectMorseForestDecision::
              no_forest_strict_arm_unresolved;
      break;
    case BuildFailure::closure_contradiction:
      result.decision =
          ExactDirectMorseForestDecision::
              no_forest_strict_arm_contradiction;
      break;
    case BuildFailure::zero_carrier_saddle:
      result.decision =
          ExactDirectMorseForestDecision::no_forest_zero_carrier_saddle;
      break;
    case BuildFailure::quotient_rejected:
      result.decision =
          ExactDirectMorseForestDecision::
              no_forest_frozen_carrier_quotient_rejected;
      break;
    case BuildFailure::locator_commit_rejected:
      result.decision =
          ExactDirectMorseForestDecision::
              no_forest_locator_commit_rejected;
      break;
  }
  return result;
}

[[nodiscard]] bool append_count_within(
    std::size_t current,
    std::size_t increment,
    std::size_t maximum) noexcept {
  std::size_t next = 0U;
  return try_add(current, increment, next) && next <= maximum;
}

[[nodiscard]] bool source_sizes_within_budget(
    const ExactDirectMorseEventJournalResult& journal,
    const ExactDirectSaddleArmSeedJournalResult& seeds,
    const ExactDirectMorseForestBudget& budget) noexcept {
  return journal.role_records.size() <=
             budget.maximum_source_role_scan_count &&
         journal.batches.size() <= budget.maximum_source_batch_scan_count &&
         seeds.families.size() <=
             budget.maximum_source_family_scan_count &&
         seeds.arm_seeds.size() <=
             budget.maximum_source_arm_seed_scan_count &&
         journal.batches.size() <= budget.maximum_batch_record_count &&
         seeds.families.size() <= budget.maximum_saddle_record_count &&
         seeds.arm_seeds.size() <= budget.maximum_arm_root_binding_count;
}

[[nodiscard]] bool payload_shape(
    const ExactDirectMorseForestJournalResult& result) noexcept {
  const std::size_t order_one_birth_count =
      static_cast<std::size_t>(std::count_if(
          result.birth_records.begin(),
          result.birth_records.end(),
          [](const ExactDirectMorseForestBirthRecord& birth) {
            return birth.order == 1U;
          }));
  const auto group_kind_count =
      [&result](ExactDirectMorseForestAtomicGroupKind kind) {
        return static_cast<std::size_t>(std::count_if(
            result.atomic_groups.begin(),
            result.atomic_groups.end(),
            [kind](const ExactDirectMorseForestAtomicGroup& group) {
              return group.kind == kind;
            }));
      };
  const std::size_t reduced_birth_group_count = group_kind_count(
      ExactDirectMorseForestAtomicGroupKind::reduced_birth);
  const std::size_t continuation_group_count = group_kind_count(
      ExactDirectMorseForestAtomicGroupKind::continuation);
  const std::size_t multifusion_group_count = group_kind_count(
      ExactDirectMorseForestAtomicGroupKind::multifusion);
  const auto node_kind_count =
      [&result](ExactDirectMorseForestNodeKind kind) {
        return static_cast<std::size_t>(std::count_if(
            result.nodes.begin(),
            result.nodes.end(),
            [kind](const ExactDirectMorseForestNode& node) {
              return node.kind == kind;
            }));
      };
  if (result.counters.birth_record_count != result.birth_records.size() ||
      result.counters.order_one_birth_node_count !=
          order_one_birth_count ||
      result.counters.latent_higher_order_birth_count !=
          result.birth_records.size() - order_one_birth_count ||
      result.counters.arm_root_binding_count !=
          result.arm_root_bindings.size() ||
      result.counters.saddle_record_count != result.saddle_records.size() ||
      result.counters.atomic_group_count != result.atomic_groups.size() ||
      result.counters.reduced_birth_group_count !=
          reduced_birth_group_count ||
      result.counters.continuation_group_count !=
          continuation_group_count ||
      result.counters.multifusion_group_count !=
          multifusion_group_count ||
      result.counters.child_reference_count != result.child_node_ids.size() ||
      result.counters.batch_record_count != result.batches.size() ||
      result.counters.node_count != result.nodes.size() ||
      result.counters.final_root_count != result.final_roots.size() ||
      node_kind_count(ExactDirectMorseForestNodeKind::order_one_birth) !=
          order_one_birth_count ||
      node_kind_count(ExactDirectMorseForestNodeKind::reduced_birth) !=
          reduced_birth_group_count ||
      node_kind_count(ExactDirectMorseForestNodeKind::multifusion) !=
          multifusion_group_count) {
    return false;
  }
  for (std::size_t index = 0U; index < result.birth_records.size(); ++index) {
    const auto& birth = result.birth_records[index];
    if (birth.birth_record_index != index ||
        birth.component_handle != index ||
        (birth.order == 1U) !=
            birth.order_one_birth_node_id.has_value() ||
        (birth.order_one_birth_node_id.has_value() &&
         *birth.order_one_birth_node_id >= result.nodes.size()) ||
        !valid_key(birth.facet_key, result.point_count, birth.order) ||
        birth.binding_witness.external_authority_id !=
            result.config.locator_config.external_authority_id ||
        birth.binding_witness.replay_token == 0U) {
      return false;
    }
  }
  for (std::size_t index = 0U;
       index < result.arm_root_bindings.size();
       ++index) {
    const auto& binding = result.arm_root_bindings[index];
    if (binding.binding_index != index ||
        binding.frozen_carrier_component_handle >=
            result.birth_records.size() ||
        (binding.prior_reduced_root_node_id.has_value() &&
         *binding.prior_reduced_root_node_id >= result.nodes.size())) {
      return false;
    }
  }
  for (std::size_t index = 0U; index < result.nodes.size(); ++index) {
    const auto& node = result.nodes[index];
    if (node.node_id != static_cast<ExactDirectMorseForestNodeId>(index) ||
        node.order == 0U ||
        node.order > result.effective_maximum_order ||
        node.child_offset > result.child_node_ids.size() ||
        node.child_count >
            result.child_node_ids.size() - node.child_offset) {
      return false;
    }
    for (std::size_t local = 0U; local < node.child_count; ++local) {
      const ExactDirectMorseForestNodeId child =
          result.child_node_ids[node.child_offset + local];
      if (child >= node.node_id ||
          (local != 0U &&
           result.child_node_ids[node.child_offset + local - 1U] >= child)) {
        return false;
      }
    }
    switch (node.kind) {
      case ExactDirectMorseForestNodeKind::order_one_birth:
        if (node.order != 1U || !node.birth_record_index.has_value() ||
            *node.birth_record_index >= result.birth_records.size() ||
            result.birth_records[*node.birth_record_index]
                    .order_one_birth_node_id !=
                std::optional<ExactDirectMorseForestNodeId>{node.node_id} ||
            node.atomic_group_index.has_value() ||
            node.child_count != 0U) {
          return false;
        }
        break;
      case ExactDirectMorseForestNodeKind::reduced_birth:
        if (node.order < 2U || node.birth_record_index.has_value() ||
            !node.atomic_group_index.has_value() ||
            *node.atomic_group_index >= result.atomic_groups.size() ||
            result.atomic_groups[*node.atomic_group_index].kind !=
                ExactDirectMorseForestAtomicGroupKind::reduced_birth ||
            node.child_count != 0U) {
          return false;
        }
        break;
      case ExactDirectMorseForestNodeKind::multifusion:
        if (node.birth_record_index.has_value() ||
            !node.atomic_group_index.has_value() ||
            *node.atomic_group_index >= result.atomic_groups.size() ||
            result.atomic_groups[*node.atomic_group_index].kind !=
                ExactDirectMorseForestAtomicGroupKind::multifusion ||
            node.child_count < 2U) {
          return false;
        }
        break;
      default:
        return false;
    }
  }
  std::size_t expected_arm_binding_offset = 0U;
  for (std::size_t index = 0U; index < result.saddle_records.size(); ++index) {
    const auto& saddle = result.saddle_records[index];
    if (saddle.saddle_record_index != index ||
        saddle.arm_binding_offset != expected_arm_binding_offset ||
        saddle.arm_binding_offset > result.arm_root_bindings.size() ||
        saddle.arm_binding_count >
            result.arm_root_bindings.size() -
                saddle.arm_binding_offset ||
        saddle.arm_binding_count == 0U ||
        saddle.arm_binding_count > 4U ||
        saddle.distinct_frozen_carrier_count == 0U ||
        saddle.distinct_frozen_carrier_count >
            saddle.arm_binding_count ||
        saddle.distinct_prior_reduced_root_count >
            saddle.distinct_frozen_carrier_count ||
        saddle.distinct_latent_carrier_count !=
            saddle.distinct_frozen_carrier_count -
                saddle.distinct_prior_reduced_root_count ||
        saddle.atomic_group_index >= result.atomic_groups.size() ||
        saddle.source_journal_batch_index >= result.batches.size() ||
        result.atomic_groups[saddle.atomic_group_index].batch_index !=
            saddle.source_journal_batch_index) {
      return false;
    }
    const std::size_t order =
        result.batches[saddle.source_journal_batch_index].order;
    std::size_t observed_carrier_count = 0U;
    std::size_t observed_reduced_root_count = 0U;
    for (std::size_t local = 0U;
         local < saddle.arm_binding_count;
         ++local) {
      const auto& binding = result.arm_root_bindings[
          saddle.arm_binding_offset + local];
      if (binding.source_family_index != saddle.source_family_index ||
          result.birth_records[
                  binding.frozen_carrier_component_handle]
                  .order != order ||
          (binding.prior_reduced_root_node_id.has_value() &&
           result.nodes[*binding.prior_reduced_root_node_id].order !=
               order)) {
        return false;
      }
      bool carrier_seen = false;
      bool reduced_root_seen = false;
      for (std::size_t previous = 0U;
           previous < local;
           ++previous) {
        const auto& prior_binding = result.arm_root_bindings[
            saddle.arm_binding_offset + previous];
        if (prior_binding.frozen_carrier_component_handle ==
            binding.frozen_carrier_component_handle) {
          if (prior_binding.prior_reduced_root_node_id !=
              binding.prior_reduced_root_node_id) {
            return false;
          }
          carrier_seen = true;
        }
        if (binding.prior_reduced_root_node_id.has_value() &&
            prior_binding.prior_reduced_root_node_id ==
                binding.prior_reduced_root_node_id) {
          reduced_root_seen = true;
        }
      }
      if (!carrier_seen) {
        ++observed_carrier_count;
      }
      if (binding.prior_reduced_root_node_id.has_value() &&
          !reduced_root_seen) {
        ++observed_reduced_root_count;
      }
    }
    if (observed_carrier_count !=
            saddle.distinct_frozen_carrier_count ||
        observed_reduced_root_count !=
            saddle.distinct_prior_reduced_root_count) {
      return false;
    }
    expected_arm_binding_offset += saddle.arm_binding_count;
  }
  if (expected_arm_binding_offset != result.arm_root_bindings.size()) {
    return false;
  }
  std::size_t expected_group_saddle_offset = 0U;
  for (std::size_t index = 0U; index < result.atomic_groups.size(); ++index) {
    const auto& group = result.atomic_groups[index];
    if (group.atomic_group_index != index ||
        group.batch_index >= result.batches.size() ||
        group.saddle_record_offset != expected_group_saddle_offset ||
        group.saddle_record_offset > result.saddle_records.size() ||
        group.saddle_record_count >
            result.saddle_records.size() - group.saddle_record_offset ||
        group.saddle_record_count == 0U ||
        group.frozen_carrier_count == 0U ||
        group.prior_reduced_root_count >
            group.frozen_carrier_count ||
        group.latent_carrier_count !=
            group.frozen_carrier_count -
                group.prior_reduced_root_count ||
        group.child_offset > result.child_node_ids.size() ||
        group.child_count >
            result.child_node_ids.size() - group.child_offset ||
        group.resulting_root_node_id >= result.nodes.size()) {
      return false;
    }
    const auto& batch = result.batches[group.batch_index];
    for (std::size_t local = 0U;
         local < group.saddle_record_count;
         ++local) {
      const auto& saddle =
          result.saddle_records[group.saddle_record_offset + local];
      if (saddle.atomic_group_index != index ||
          saddle.source_journal_batch_index !=
              batch.source_journal_batch_index) {
        return false;
      }
    }
    switch (group.kind) {
      case ExactDirectMorseForestAtomicGroupKind::reduced_birth:
        if (group.prior_reduced_root_count != 0U ||
            group.latent_carrier_count == 0U ||
            group.child_count != 0U ||
            !group.created_node_id.has_value() ||
            *group.created_node_id != group.resulting_root_node_id) {
          return false;
        }
        break;
      case ExactDirectMorseForestAtomicGroupKind::continuation:
        if (group.prior_reduced_root_count != 1U ||
            group.child_count != 0U ||
            group.created_node_id.has_value()) {
          return false;
        }
        break;
      case ExactDirectMorseForestAtomicGroupKind::multifusion:
        if (group.prior_reduced_root_count < 2U ||
            group.child_count != group.prior_reduced_root_count ||
            !group.created_node_id.has_value() ||
            *group.created_node_id != group.resulting_root_node_id) {
          return false;
        }
        break;
      default:
        return false;
    }
    const auto& resulting_node =
        result.nodes[group.resulting_root_node_id];
    if (resulting_node.order != batch.order) {
      return false;
    }
    if (group.created_node_id.has_value()) {
      if (resulting_node.atomic_group_index !=
              std::optional<std::size_t>{index} ||
          resulting_node.squared_level != batch.squared_level ||
          resulting_node.child_offset != group.child_offset ||
          resulting_node.child_count != group.child_count) {
        return false;
      }
    } else if (
        group.kind !=
        ExactDirectMorseForestAtomicGroupKind::continuation) {
      return false;
    }
    expected_group_saddle_offset += group.saddle_record_count;
  }
  if (expected_group_saddle_offset != result.saddle_records.size()) {
    return false;
  }
  std::size_t expected_birth_offset = 0U;
  std::size_t expected_saddle_offset = 0U;
  std::size_t expected_group_offset = 0U;
  for (std::size_t index = 0U; index < result.batches.size(); ++index) {
    const auto& batch = result.batches[index];
    if (batch.batch_index != index ||
        batch.source_journal_batch_index != index ||
        batch.order == 0U ||
        batch.order > result.effective_maximum_order ||
        (index != 0U &&
         !(result.batches[index - 1U].order < batch.order ||
           (result.batches[index - 1U].order == batch.order &&
            result.batches[index - 1U].squared_level <
                batch.squared_level))) ||
        batch.birth_record_offset != expected_birth_offset ||
        batch.birth_record_offset > result.birth_records.size() ||
        batch.birth_record_count >
            result.birth_records.size() - batch.birth_record_offset ||
        batch.saddle_record_offset != expected_saddle_offset ||
        batch.saddle_record_offset > result.saddle_records.size() ||
        batch.saddle_record_count >
            result.saddle_records.size() - batch.saddle_record_offset ||
        batch.atomic_group_offset != expected_group_offset ||
        batch.atomic_group_offset > result.atomic_groups.size() ||
        batch.atomic_group_count >
            result.atomic_groups.size() - batch.atomic_group_offset ||
        batch.strict_pre_batch_reduced_root_count >
            batch.strict_pre_batch_carrier_count ||
        batch.closed_post_batch_reduced_root_count >
            batch.closed_post_batch_carrier_count ||
        batch.strict_pre_batch_stamp.external_authority_id !=
            result.config.locator_config.external_authority_id ||
        batch.strict_pre_batch_stamp.committed_batch_count != index ||
        batch.committed_batch_stamp.external_authority_id !=
            result.config.locator_config.external_authority_id ||
        batch.committed_batch_stamp.committed_batch_count != index + 1U ||
        !batch.strict_arms_resolved_before_mutation ||
        !batch.quotient_resolved_before_mutation ||
        !batch.unions_then_births_committed_atomically) {
      return false;
    }
    for (std::size_t local = 0U;
         local < batch.birth_record_count;
         ++local) {
      const auto& birth =
          result.birth_records[batch.birth_record_offset + local];
      if (birth.source_journal_batch_index != index ||
          birth.order != batch.order) {
        return false;
      }
    }
    for (std::size_t local = 0U;
         local < batch.saddle_record_count;
         ++local) {
      if (result.saddle_records[
              batch.saddle_record_offset + local]
              .source_journal_batch_index != index) {
        return false;
      }
    }
    for (std::size_t local = 0U;
         local < batch.atomic_group_count;
         ++local) {
      if (result.atomic_groups[
              batch.atomic_group_offset + local]
              .batch_index != index) {
        return false;
      }
    }
    expected_birth_offset += batch.birth_record_count;
    expected_saddle_offset += batch.saddle_record_count;
    expected_group_offset += batch.atomic_group_count;
  }
  if (expected_birth_offset != result.birth_records.size() ||
      expected_saddle_offset != result.saddle_records.size() ||
      expected_group_offset != result.atomic_groups.size()) {
    return false;
  }
  if (result.effective_maximum_order > result.point_count ||
      result.effective_maximum_order >
          direct_sparse_positive_facet_maximum_point_count) {
    return false;
  }
  for (std::size_t index = 0U; index < result.final_roots.size(); ++index) {
    const auto& root = result.final_roots[index];
    if (root.final_root_index != index ||
        root.order == 0U ||
        root.order > result.effective_maximum_order ||
        root.component_handle >= result.birth_records.size() ||
        result.birth_records[root.component_handle].order != root.order ||
        root.root_node_id >= result.nodes.size() ||
        result.nodes[root.root_node_id].order != root.order ||
        (index != 0U &&
         (result.final_roots[index - 1U].order >
              root.order ||
          (result.final_roots[index - 1U].order ==
               root.order &&
           result.final_roots[index - 1U].root_node_id >=
               root.root_node_id)))) {
      return false;
    }
  }
  for (std::size_t order = 1U;
       order <= result.effective_maximum_order;
       ++order) {
    const std::size_t observed_count =
        static_cast<std::size_t>(std::count_if(
            result.final_roots.begin(),
            result.final_roots.end(),
            [order](const ExactDirectMorseForestFinalRoot& root) {
              return root.order == order;
            }));
    const bool expects_reduced_root =
        order == 1U || order < result.point_count;
    if (observed_count != (expects_reduced_root ? 1U : 0U)) {
      return false;
    }
  }
  return true;
}

}  // namespace

bool ExactDirectMorseForestJournalResult::
    certified_conditional_h0_candidate() const noexcept {
  return schema_version == direct_morse_forest_journal_schema_version &&
         decision == ExactDirectMorseForestDecision::
                         complete_conditional_exact_direct_morse_forest &&
         budget_preflight_certified &&
         source_event_journal_freshly_replayed &&
         source_strict_arm_journal_freshly_replayed &&
         every_birth_key_reconstructed_from_closed_direct_event &&
         deterministic_disjoint_birth_union_and_query_tokens &&
         batches_processed_in_strict_order_level_order &&
         cardinality_isolates_orders_in_shared_locator &&
         current_level_births_hidden_from_arm_descent &&
         higher_order_direct_births_are_latent_carriers &&
         one_10_5c_call_per_nonempty_strict_arm_batch &&
         every_strict_arm_has_positive_terminal &&
         all_catalogued_saddle_families_consumed_once &&
         carrier_to_optional_reduced_root_authority_maintained &&
         every_saddle_has_positive_carrier &&
         typed_root_or_latent_carrier_hyperedges_closed_transitively &&
         q_r_counts_only_distinct_prior_reduced_roots &&
         all_equal_level_saddles_quotiented_before_mutation &&
         saddle_records_grouped_with_source_family_provenance &&
         q_zero_groups_create_one_reduced_birth &&
         q_one_continuations_create_no_node &&
         q_at_least_two_groups_create_one_multifusion &&
         current_batch_birth_nodes_never_same_batch_children &&
         all_group_carriers_attached_to_resulting_root_atomically &&
         locator_commits_unions_before_current_birth_bindings &&
         final_roots_cover_exactly_nonterminal_reduced_orders &&
         no_partial_scientific_payload_published &&
         non_scope_is_honest(*this) && payload_shape(*this) &&
         birth_records.size() <= requested_budget.maximum_birth_record_count &&
         arm_root_bindings.size() <=
             requested_budget.maximum_arm_root_binding_count &&
         saddle_records.size() <=
             requested_budget.maximum_saddle_record_count &&
         atomic_groups.size() <=
             requested_budget.maximum_atomic_group_count &&
         child_node_ids.size() <=
             requested_budget.maximum_child_reference_count &&
         batches.size() <= requested_budget.maximum_batch_record_count &&
         nodes.size() <= requested_budget.maximum_node_count &&
         final_roots.size() <= requested_budget.maximum_final_root_count &&
         logical_output_entry_count <=
             requested_budget.maximum_logical_output_entry_count &&
         counters.aggregate_closure_node_count <=
             requested_budget.maximum_aggregate_closure_node_count &&
         counters.aggregate_closure_step_call_count <=
             requested_budget.maximum_aggregate_closure_step_call_count &&
         final_locator_stamp.external_authority_id ==
             config.locator_config.external_authority_id &&
         final_locator_stamp.committed_batch_count == batches.size();
}

bool ExactDirectMorseForestJournalResult::certified_conditional_exact_h0()
    const noexcept {
  return certified_conditional_h0_candidate();
}

bool ExactDirectMorseForestJournalResult::certified_atomic_failure()
    const noexcept {
  return schema_version == direct_morse_forest_journal_schema_version &&
         decision != ExactDirectMorseForestDecision::not_certified &&
         decision != ExactDirectMorseForestDecision::
                         complete_conditional_exact_direct_morse_forest &&
         birth_records.empty() && arm_root_bindings.empty() &&
         saddle_records.empty() && atomic_groups.empty() &&
         child_node_ids.empty() && batches.empty() && final_roots.empty() &&
         nodes.empty() && logical_output_entry_count == 0U &&
         no_partial_scientific_payload_published &&
         non_scope_is_honest(*this);
}

bool ExactDirectMorseForestJournalResult::certified_outcome() const noexcept {
  return certified_conditional_h0_candidate() ||
         certified_atomic_failure();
}

ExactDirectMorseForestJournalResult
build_exact_direct_morse_forest_journal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& budget,
    const ExactDirectMorseForestConfig& config,
    spatial::LbvhTraversalOrder traversal_order) {
  require_valid_traversal_order(traversal_order);
  require_closure_budget_within_confidence(budget.closure_budget);
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "a direct Morse forest requires a matching LBVH authority");
  }
  if (config.locator_config.external_authority_id == 0U) {
    throw std::invalid_argument(
        "a direct Morse forest requires a nonzero locator authority");
  }

  ExactDirectMorseForestJournalResult result;
  result.requested_budget = budget;
  result.config = config;
  result.traversal_order = traversal_order;
  result.point_count = cloud.size();
  result.effective_maximum_order = source_journal.effective_maximum_order;
  initialize_scope(result);

  if (!source_sizes_within_budget(
          source_journal, source_seed_journal, budget)) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }

  try {
    const auto source_verification =
        verify_exact_direct_saddle_arm_seed_journal_streaming(
            cloud,
            source_facade,
            source_journal,
            trusted_seed_budget,
            source_seed_journal);
    if (!source_verification.result_certified ||
        !source_journal.certified_partial_refinement() ||
        !source_seed_journal.certified_partial_refinement() ||
        source_journal.point_count != cloud.size() ||
        source_seed_journal.point_count != cloud.size()) {
      return fail(std::move(result), BuildFailure::source_rejected);
    }
    result.source_event_journal_freshly_replayed = true;
    result.source_strict_arm_journal_freshly_replayed = true;

    std::size_t birth_count = 0U;
    for (const auto& role : source_journal.role_records) {
      if (role.role == ExactDirectMorseH0Role::birth &&
          !checked_increment(birth_count)) {
        return fail(std::move(result), BuildFailure::capacity_overflow);
      }
    }
    if (birth_count == 0U ||
        birth_count > budget.maximum_birth_record_count ||
        source_journal.batches.size() >
            budget.maximum_batch_record_count ||
        source_seed_journal.families.size() >
            budget.maximum_saddle_record_count ||
        source_seed_journal.arm_seeds.size() >
            budget.maximum_arm_root_binding_count) {
      return fail(std::move(result), BuildFailure::budget_exhausted);
    }

    std::size_t birth_plus_family = 0U;
    std::size_t safe_birth_entries = 0U;
    std::size_t safe_arm_entries = 0U;
    std::size_t safe_family_entries = 0U;
    std::size_t safe_output_bound = 0U;
    if (!try_add(
            birth_count,
            source_seed_journal.families.size(),
            birth_plus_family) ||
        !try_multiply(13U, birth_count, safe_birth_entries) ||
        !try_multiply(
            12U,
            source_seed_journal.arm_seeds.size(),
            safe_arm_entries) ||
        !try_multiply(
            3U,
            source_seed_journal.families.size(),
            safe_family_entries) ||
        !try_add(
            safe_birth_entries,
            safe_arm_entries,
            safe_output_bound) ||
        !try_add(
            safe_output_bound,
            safe_family_entries,
            safe_output_bound) ||
        !try_add(
            safe_output_bound,
            source_journal.batches.size(),
            safe_output_bound)) {
      return fail(std::move(result), BuildFailure::capacity_overflow);
    }
    if (birth_plus_family > budget.maximum_node_count ||
        safe_output_bound >
            budget.maximum_logical_output_entry_count) {
      return fail(std::move(result), BuildFailure::budget_exhausted);
    }
    result.budget_preflight_certified = true;

    auto locator = build_exact_direct_sparse_positive_facet_locator(
        birth_count, budget.locator_budget, config.locator_config);
    if (!locator.certified_positive_locator()) {
      return fail(
          std::move(result), BuildFailure::locator_initialization);
    }
    ForestComponents components{birth_count};

    result.birth_records.reserve(birth_count);
    result.arm_root_bindings.reserve(
        source_seed_journal.arm_seeds.size());
    result.saddle_records.reserve(
        source_seed_journal.families.size());
    result.batches.reserve(source_journal.batches.size());
    result.nodes.reserve(birth_plus_family);

    std::size_t family_cursor = 0U;
    for (std::size_t source_batch_index = 0U;
         source_batch_index < source_journal.batches.size();
         ++source_batch_index) {
      const auto& source_batch =
          source_journal.batches[source_batch_index];
      if (source_batch.batch_index != source_batch_index ||
          source_batch.order == 0U ||
          source_batch.order >
              direct_sparse_positive_facet_maximum_point_count ||
          (source_batch_index != 0U &&
           !(source_journal.batches[source_batch_index - 1U].order <
                 source_batch.order ||
             (source_journal.batches[source_batch_index - 1U].order ==
                  source_batch.order &&
              source_journal.batches[source_batch_index - 1U]
                      .squared_level <
                  source_batch.squared_level)))) {
        return fail(
            std::move(result),
            BuildFailure::source_batch_inconsistent);
      }

      const auto strict_pre_stamp = locator.snapshot_stamp();
      const std::size_t strict_pre_carrier_count =
          components.carrier_count(source_batch.order);
      const std::size_t strict_pre_reduced_root_count =
          components.reduced_root_count(source_batch.order);
      // The saddle-family catalogue is authoritative independently of the
      // minimum stream: consume every family in this (order, level) batch,
      // including late families whose arms already all carry R roots.
      const std::size_t family_begin = family_cursor;
      while (family_cursor < source_seed_journal.families.size() &&
             source_seed_journal.families[family_cursor]
                     .journal_batch_index == source_batch_index) {
        ++family_cursor;
      }
      if ((family_cursor < source_seed_journal.families.size() &&
           source_seed_journal.families[family_cursor]
                   .journal_batch_index < source_batch_index) ||
          family_cursor - family_begin != source_batch.saddle_role_count) {
        return fail(
            std::move(result),
            BuildFailure::source_batch_inconsistent);
      }

      std::vector<TemporarySaddle> temporary_saddles;
      std::vector<ExactDirectSparseFacetKey> distinct_keys;
      temporary_saddles.reserve(family_cursor - family_begin);
      std::size_t batch_arm_count = 0U;
      for (std::size_t family_index = family_begin;
           family_index < family_cursor;
           ++family_index) {
        const auto& family = source_seed_journal.families[family_index];
        if (family.family_index != family_index ||
            family.order != source_batch.order ||
            family.critical_squared_level != source_batch.squared_level ||
            family.arm_seed_count == 0U ||
            family.arm_seed_count > 4U ||
            family.arm_seed_offset >
                source_seed_journal.arm_seeds.size() ||
            family.arm_seed_count >
                source_seed_journal.arm_seeds.size() -
                    family.arm_seed_offset) {
          return fail(
              std::move(result),
              BuildFailure::source_batch_inconsistent);
        }
        TemporarySaddle saddle;
        saddle.source_family_index = family_index;
        saddle.arms.reserve(family.arm_seed_count);
        for (std::size_t local = 0U;
             local < family.arm_seed_count;
             ++local) {
          const std::size_t seed_index = family.arm_seed_offset + local;
          const auto facet =
              reconstruct_exact_direct_saddle_arm_facet(
                  source_facade, source_seed_journal, seed_index);
          const auto key =
              arm_key(facet, cloud.size(), source_batch.order);
          saddle.arms.push_back(
              {seed_index, key, 0U, std::nullopt});
          distinct_keys.push_back(key);
          if (!checked_increment(batch_arm_count)) {
            return fail(
                std::move(result), BuildFailure::capacity_overflow);
          }
        }
        temporary_saddles.push_back(std::move(saddle));
      }
      result.counters.maximum_batch_arm_count =
          std::max(
              result.counters.maximum_batch_arm_count,
              batch_arm_count);

      std::sort(
          distinct_keys.begin(), distinct_keys.end(), facet_key_less);
      distinct_keys.erase(
          std::unique(distinct_keys.begin(), distinct_keys.end()),
          distinct_keys.end());
      if (distinct_keys.size() >
              budget.maximum_batch_distinct_arm_count ||
          distinct_keys.size() >
              budget.closure_budget.maximum_seed_count) {
        return fail(
            std::move(result),
            BuildFailure::closure_budget_exhausted);
      }
      result.counters.distinct_strict_arm_count +=
          distinct_keys.size();
      result.counters.duplicate_strict_arm_reference_count +=
          batch_arm_count - distinct_keys.size();

      std::vector<ResolvedKey> resolved_keys;
      if (!distinct_keys.empty()) {
        const auto token = query_replay_token(source_batch_index);
        if (!token.has_value()) {
          return fail(
              std::move(result), BuildFailure::capacity_overflow);
        }
        std::vector<ExactDirectSparseFacetDescentClosureSeed> closure_seeds;
        closure_seeds.reserve(distinct_keys.size());
        for (std::size_t seed_index = 0U;
             seed_index < distinct_keys.size();
             ++seed_index) {
          closure_seeds.push_back({seed_index, distinct_keys[seed_index]});
        }
        const ExactDirectSparseFacetWitness query_witness{
            config.locator_config.external_authority_id, *token};
        const auto closure =
            build_exact_direct_sparse_facet_descent_closure(
                index,
                cloud,
                closure_seeds,
                source_batch.squared_level,
                query_witness,
                locator,
                budget.closure_budget,
                config.closure_config,
                traversal_order);
        ++result.counters.closure_call_count;
        if (!append_count_within(
                result.counters.aggregate_closure_node_count,
                closure.nodes.size(),
                budget.maximum_aggregate_closure_node_count) ||
            !append_count_within(
                result.counters.aggregate_closure_step_call_count,
                closure.counters.evaluated_step_source_count,
                budget.maximum_aggregate_closure_step_call_count)) {
          return fail(
              std::move(result), BuildFailure::budget_exhausted);
        }
        result.counters.aggregate_closure_node_count +=
            closure.nodes.size();
        result.counters.aggregate_closure_step_call_count +=
            closure.counters.evaluated_step_source_count;
        if (!closure.certified_complete_relative_positive_closure()) {
          if (closure.certified_budget_exhaustion()) {
            return fail(
                std::move(result),
                BuildFailure::closure_budget_exhausted);
          }
          if (closure.certified_complete_with_unresolved_terminals()) {
            return fail(
                std::move(result),
                BuildFailure::closure_unresolved);
          }
          return fail(
              std::move(result),
              BuildFailure::closure_contradiction);
        }
        if (closure.seed_projections.size() != distinct_keys.size()) {
          return fail(
              std::move(result),
              BuildFailure::closure_contradiction);
        }
        resolved_keys.resize(distinct_keys.size());
        for (std::size_t seed_index = 0U;
             seed_index < closure.seed_projections.size();
             ++seed_index) {
          const auto& projection =
              closure.seed_projections[seed_index];
          if (projection.seed_index != seed_index ||
              projection.source_facet_key != distinct_keys[seed_index] ||
              projection.closure_disposition !=
                  ExactDirectSparseFacetDescentClosureDisposition::
                      relative_positive ||
              projection.terminal_node_index >= closure.nodes.size()) {
            return fail(
                std::move(result),
                BuildFailure::closure_contradiction);
          }
          const auto& terminal =
              closure.nodes[projection.terminal_node_index];
          if (!terminal.resolved_component_handle.has_value() ||
              !terminal.resolved_binding_witness.has_value()) {
            return fail(
                std::move(result),
                BuildFailure::closure_contradiction);
          }
          const std::size_t handle =
              *terminal.resolved_component_handle;
          const auto& binding_witness =
              *terminal.resolved_binding_witness;
          if (binding_witness.external_authority_id !=
                  config.locator_config.external_authority_id ||
              binding_witness.replay_token == 0U ||
              binding_witness.replay_token % 3U != 1U) {
            return fail(
                std::move(result),
                BuildFailure::closure_contradiction);
          }
          const std::uint64_t birth_index_u64 =
              (binding_witness.replay_token - 1U) / 3U;
          if (birth_index_u64 >
                  std::numeric_limits<std::size_t>::max() ||
              static_cast<std::size_t>(birth_index_u64) >=
                  result.birth_records.size()) {
            return fail(
                std::move(result),
                BuildFailure::closure_contradiction);
          }
          const auto& source_birth =
              result.birth_records[
                  static_cast<std::size_t>(birth_index_u64)];
          if (handle >= components.handle_count() ||
              !components.active(handle) ||
              components.order(handle) != source_batch.order ||
              components.root(source_birth.component_handle) != handle ||
              source_birth.binding_witness != binding_witness ||
              terminal.facet_key != source_birth.facet_key) {
            return fail(
                std::move(result),
                BuildFailure::closure_contradiction);
          }
          resolved_keys[seed_index] = {
              distinct_keys[seed_index],
              components.root(handle),
              components.reduced_root(handle)};
        }
      }

      for (auto& saddle : temporary_saddles) {
        for (auto& arm : saddle.arms) {
          const auto found = std::lower_bound(
              resolved_keys.begin(),
              resolved_keys.end(),
              arm.key,
              [](const ResolvedKey& resolved,
                 const ExactDirectSparseFacetKey& key) {
                return facet_key_less(resolved.key, key);
              });
          if (found == resolved_keys.end() || found->key != arm.key) {
            return fail(
                std::move(result),
                BuildFailure::closure_contradiction);
          }
          arm.carrier_handle = found->carrier_handle;
          arm.prior_reduced_root_node_id =
              found->prior_reduced_root_node_id;
        }
      }

      std::vector<ExactFrozenRootHyperedge> hyperedges;
      std::vector<ExactFrozenRootId> carrier_references;
      hyperedges.reserve(temporary_saddles.size());
      carrier_references.reserve(batch_arm_count);
      for (std::size_t saddle_index = 0U;
           saddle_index < temporary_saddles.size();
           ++saddle_index) {
        const auto& saddle = temporary_saddles[saddle_index];
        if (saddle.arms.empty()) {
          return fail(
              std::move(result), BuildFailure::zero_carrier_saddle);
        }
        std::vector<ExactFrozenRootId> distinct_carrier_ids;
        distinct_carrier_ids.reserve(saddle.arms.size());
        for (const auto& arm : saddle.arms) {
          if (arm.carrier_handle >
              std::numeric_limits<ExactFrozenRootId>::max()) {
            return fail(
                std::move(result), BuildFailure::capacity_overflow);
          }
          distinct_carrier_ids.push_back(
              static_cast<ExactFrozenRootId>(arm.carrier_handle));
        }
        std::sort(
            distinct_carrier_ids.begin(), distinct_carrier_ids.end());
        distinct_carrier_ids.erase(
            std::unique(
                distinct_carrier_ids.begin(),
                distinct_carrier_ids.end()),
            distinct_carrier_ids.end());
        if (distinct_carrier_ids.empty()) {
          return fail(
              std::move(result), BuildFailure::zero_carrier_saddle);
        }
        hyperedges.push_back(
            {saddle_index,
             carrier_references.size(),
             distinct_carrier_ids.size()});
        // The quotient kernel needs only opaque equality labels and nonempty
        // hyperedges.  These labels are frozen full-component DSU roots:
        // presence/absence of an attached reduced root types each one R/L.
        // No L carrier is filtered before the transitive batch closure.
        carrier_references.insert(
            carrier_references.end(),
            distinct_carrier_ids.begin(),
            distinct_carrier_ids.end());
      }

      std::optional<ExactFrozenRootQuotientResult> quotient;
      if (!hyperedges.empty()) {
        quotient.emplace(build_exact_direct_frozen_root_quotient(
            hyperedges, carrier_references, budget.quotient_budget));
        ++result.counters.quotient_call_count;
        if (!quotient->certified_frozen_root_quotient()) {
          return fail(
              std::move(result), BuildFailure::quotient_rejected);
        }
      }

      std::vector<GroupPlan> plans;
      if (quotient.has_value()) {
        plans.resize(quotient->groups.size());
        for (std::size_t group_index = 0U;
             group_index < quotient->groups.size();
             ++group_index) {
          const auto& group = quotient->groups[group_index];
          if (group.group_index != group_index ||
              group.root_count == 0U ||
              group.root_offset > quotient->group_root_ids.size() ||
              group.root_count >
                  quotient->group_root_ids.size() - group.root_offset) {
            return fail(
                std::move(result), BuildFailure::quotient_rejected);
          }
          GroupPlan& plan = plans[group_index];
          plan.atomic_group_index =
              result.atomic_groups.size() + group_index;
          plan.carrier_handles.reserve(group.root_count);
          for (std::size_t local = 0U; local < group.root_count; ++local) {
            const ExactFrozenRootId carrier_id =
                quotient->group_root_ids[group.root_offset + local];
            if (carrier_id >
                std::numeric_limits<
                    ExactDirectSparseComponentHandle>::max()) {
              return fail(
                  std::move(result), BuildFailure::capacity_overflow);
            }
            const auto carrier =
                static_cast<ExactDirectSparseComponentHandle>(carrier_id);
            if (carrier >= components.handle_count() ||
                components.root(carrier) != carrier ||
                !components.active(carrier) ||
                components.order(carrier) != source_batch.order) {
              return fail(
                  std::move(result),
                  BuildFailure::closure_contradiction);
            }
            plan.carrier_handles.push_back(carrier);
          }
        }
        for (const auto& binding : quotient->hyperedge_bindings) {
          if (binding.source_hyperedge_index >=
                  temporary_saddles.size() ||
              binding.group_index >= plans.size()) {
            return fail(
                std::move(result), BuildFailure::quotient_rejected);
          }
          plans[binding.group_index].saddle_indices.push_back(
              binding.source_hyperedge_index);
        }

        std::size_t pending_created_node_count = 0U;
        for (GroupPlan& plan : plans) {
          if (plan.saddle_indices.empty() ||
              plan.carrier_handles.empty()) {
            return fail(
                std::move(result), BuildFailure::zero_carrier_saddle);
          }
          for (const auto carrier : plan.carrier_handles) {
            const auto prior_root = components.reduced_root(carrier);
            if (prior_root.has_value()) {
              plan.prior_reduced_root_node_ids.push_back(*prior_root);
            }
          }
          std::sort(
              plan.prior_reduced_root_node_ids.begin(),
              plan.prior_reduced_root_node_ids.end());
          if (std::adjacent_find(
                  plan.prior_reduced_root_node_ids.begin(),
                  plan.prior_reduced_root_node_ids.end()) !=
              plan.prior_reduced_root_node_ids.end()) {
            return fail(
                std::move(result),
                BuildFailure::closure_contradiction);
          }
          result.counters.maximum_batch_carrier_arity =
              std::max(
                  result.counters.maximum_batch_carrier_arity,
                  plan.carrier_handles.size());
          result.counters.maximum_batch_merge_arity =
              std::max(
                  result.counters.maximum_batch_merge_arity,
                  plan.prior_reduced_root_node_ids.size());
          // Classification depends only on q_R after the complete transitive
          // carrier closure.  There is no first-incidence filter: a late
          // all-R group remains eligible for a multifusion.
          if (plan.prior_reduced_root_node_ids.size() == 1U) {
            plan.kind =
                ExactDirectMorseForestAtomicGroupKind::continuation;
            plan.resulting_root_node_id =
                plan.prior_reduced_root_node_ids.front();
          } else {
            if (plan.prior_reduced_root_node_ids.empty()) {
              if (source_batch.order == 1U) {
                return fail(
                    std::move(result),
                    BuildFailure::closure_contradiction);
              }
              plan.kind =
                  ExactDirectMorseForestAtomicGroupKind::reduced_birth;
            } else {
              plan.kind =
                  ExactDirectMorseForestAtomicGroupKind::multifusion;
            }
            const std::size_t node_index =
                result.nodes.size() + pending_created_node_count;
            if (node_index >= budget.maximum_node_count ||
                node_index >
                    std::numeric_limits<
                        ExactDirectMorseForestNodeId>::max()) {
              return fail(
                  std::move(result),
                  BuildFailure::budget_exhausted);
            }
            plan.created_node_id =
                static_cast<ExactDirectMorseForestNodeId>(node_index);
            plan.resulting_root_node_id = *plan.created_node_id;
            ++pending_created_node_count;
          }
        }
      }

      if (!append_count_within(
              result.atomic_groups.size(),
              plans.size(),
              budget.maximum_atomic_group_count) ||
          !append_count_within(
              result.saddle_records.size(),
              temporary_saddles.size(),
              budget.maximum_saddle_record_count) ||
          !append_count_within(
              result.arm_root_bindings.size(),
              batch_arm_count,
              budget.maximum_arm_root_binding_count)) {
        return fail(
            std::move(result), BuildFailure::budget_exhausted);
      }

      std::vector<ExactDirectMorseForestArmRootBinding> pending_arm_bindings;
      std::vector<ExactDirectMorseForestSaddleRecord> pending_saddles;
      std::vector<ExactDirectMorseForestAtomicGroup> pending_groups;
      std::vector<ExactDirectMorseForestNodeId> pending_children;
      std::vector<ExactDirectMorseForestNode> pending_group_nodes;
      std::vector<ExactDirectSparseComponentUnion> locator_unions;
      pending_arm_bindings.reserve(batch_arm_count);
      pending_saddles.reserve(temporary_saddles.size());
      pending_groups.reserve(plans.size());

      for (GroupPlan& plan : plans) {
        const std::size_t group_saddle_offset =
            result.saddle_records.size() + pending_saddles.size();
        for (const std::size_t saddle_index : plan.saddle_indices) {
          const auto& saddle = temporary_saddles[saddle_index];
          const auto& family =
              source_seed_journal.families[saddle.source_family_index];
          const std::size_t arm_offset =
              result.arm_root_bindings.size() +
              pending_arm_bindings.size();
          std::vector<ExactDirectSparseComponentHandle> saddle_carriers;
          std::vector<ExactDirectMorseForestNodeId>
              saddle_prior_reduced_roots;
          for (const auto& arm : saddle.arms) {
            pending_arm_bindings.push_back(
                {result.arm_root_bindings.size() +
                     pending_arm_bindings.size(),
                 arm.source_seed_index,
                 saddle.source_family_index,
                 arm.key,
                 arm.carrier_handle,
                 arm.prior_reduced_root_node_id});
            saddle_carriers.push_back(arm.carrier_handle);
            if (arm.prior_reduced_root_node_id.has_value()) {
              saddle_prior_reduced_roots.push_back(
                  *arm.prior_reduced_root_node_id);
            }
          }
          std::sort(saddle_carriers.begin(), saddle_carriers.end());
          saddle_carriers.erase(
              std::unique(
                  saddle_carriers.begin(), saddle_carriers.end()),
              saddle_carriers.end());
          std::sort(
              saddle_prior_reduced_roots.begin(),
              saddle_prior_reduced_roots.end());
          saddle_prior_reduced_roots.erase(
              std::unique(
                  saddle_prior_reduced_roots.begin(),
                  saddle_prior_reduced_roots.end()),
              saddle_prior_reduced_roots.end());
          if (saddle_carriers.empty() ||
              saddle_prior_reduced_roots.size() >
                  saddle_carriers.size()) {
            return fail(
                std::move(result), BuildFailure::zero_carrier_saddle);
          }
          pending_saddles.push_back(
              {result.saddle_records.size() + pending_saddles.size(),
               saddle.source_family_index,
               family.source_event_index,
               source_batch_index,
               arm_offset,
               saddle.arms.size(),
               saddle_carriers.size(),
               saddle_carriers.size() -
                   saddle_prior_reduced_roots.size(),
               saddle_prior_reduced_roots.size(),
               plan.atomic_group_index});
        }

        ExactDirectMorseForestAtomicGroup group_record;
        group_record.atomic_group_index = plan.atomic_group_index;
        group_record.batch_index = result.batches.size();
        group_record.saddle_record_offset = group_saddle_offset;
        group_record.saddle_record_count =
            result.saddle_records.size() + pending_saddles.size() -
            group_saddle_offset;
        group_record.frozen_carrier_count =
            plan.carrier_handles.size();
        group_record.latent_carrier_count =
            plan.carrier_handles.size() -
            plan.prior_reduced_root_node_ids.size();
        group_record.prior_reduced_root_count =
            plan.prior_reduced_root_node_ids.size();
        group_record.child_offset =
            result.child_node_ids.size() + pending_children.size();
        group_record.created_node_id = plan.created_node_id;
        group_record.resulting_root_node_id =
            plan.resulting_root_node_id;
        group_record.kind = plan.kind;
        switch (plan.kind) {
          case ExactDirectMorseForestAtomicGroupKind::reduced_birth:
            ++result.counters.reduced_birth_group_count;
            group_record.child_count = 0U;
            break;
          case ExactDirectMorseForestAtomicGroupKind::continuation:
            ++result.counters.continuation_group_count;
            group_record.child_count = 0U;
            break;
          case ExactDirectMorseForestAtomicGroupKind::multifusion:
            ++result.counters.multifusion_group_count;
            group_record.child_count =
                plan.prior_reduced_root_node_ids.size();
            if (!append_count_within(
                    result.child_node_ids.size() +
                        pending_children.size(),
                    plan.prior_reduced_root_node_ids.size(),
                    budget.maximum_child_reference_count)) {
              return fail(
                  std::move(result), BuildFailure::budget_exhausted);
            }
            pending_children.insert(
                pending_children.end(),
                plan.prior_reduced_root_node_ids.begin(),
                plan.prior_reduced_root_node_ids.end());
            break;
        }
        if (plan.kind !=
            ExactDirectMorseForestAtomicGroupKind::continuation) {
          pending_group_nodes.push_back(
              {*plan.created_node_id,
               source_batch.order,
               source_batch.squared_level,
               plan.kind ==
                       ExactDirectMorseForestAtomicGroupKind::reduced_birth
                   ? ExactDirectMorseForestNodeKind::reduced_birth
                   : ExactDirectMorseForestNodeKind::multifusion,
               group_record.child_offset,
               group_record.child_count,
               std::nullopt,
               plan.atomic_group_index});
        }
        for (std::size_t local = 1U;
             local < plan.carrier_handles.size();
             ++local) {
          const std::size_t local_union_index =
              locator_unions.size();
          const std::size_t global_union_index =
              result.counters.locator_union_count +
              local_union_index;
          const auto token = replay_token(global_union_index, 2U);
          if (!token.has_value()) {
            return fail(
                std::move(result),
                BuildFailure::capacity_overflow);
          }
          locator_unions.push_back(
              {local_union_index,
               plan.carrier_handles.front(),
               plan.carrier_handles[local],
               {config.locator_config.external_authority_id,
                *token}});
        }
        pending_groups.push_back(group_record);
      }

      std::vector<ExactDirectMorseForestBirthRecord> pending_births;
      std::vector<ExactDirectMorseForestNode> pending_birth_nodes;
      std::vector<ExactDirectSparseFacetBinding> locator_bindings;
      const std::size_t role_begin = source_batch.role_record_offset;
      if (role_begin > source_journal.role_records.size() ||
          source_batch.role_record_count >
              source_journal.role_records.size() - role_begin) {
        return fail(
            std::move(result),
            BuildFailure::source_batch_inconsistent);
      }
      for (std::size_t local = 0U;
           local < source_batch.role_record_count;
           ++local) {
        const auto& role =
            source_journal.role_records[role_begin + local];
        if (role.batch_index != source_batch_index ||
            role.event_projection_index >=
                source_journal.event_projections.size()) {
          return fail(
              std::move(result),
              BuildFailure::source_batch_inconsistent);
        }
        if (role.role != ExactDirectMorseH0Role::birth) {
          continue;
        }
        const auto& projection =
            source_journal.event_projections[
                role.event_projection_index];
        const auto key = birth_key(
            projection,
            source_facade,
            cloud.size(),
            source_batch.order,
            source_batch.squared_level);
        const std::size_t birth_index =
            result.birth_records.size() + pending_births.size();
        const std::size_t node_index =
            result.nodes.size() + pending_group_nodes.size() +
            pending_birth_nodes.size();
        if (birth_index >= birth_count) {
          return fail(
              std::move(result), BuildFailure::budget_exhausted);
        }
        const auto token = replay_token(birth_index, 1U);
        if (!token.has_value()) {
          return fail(
              std::move(result), BuildFailure::capacity_overflow);
        }
        const ExactDirectSparseFacetWitness witness{
            config.locator_config.external_authority_id, *token};
        std::optional<ExactDirectMorseForestNodeId>
            order_one_birth_node_id;
        if (source_batch.order == 1U) {
          if (node_index >= budget.maximum_node_count ||
              node_index >
                  std::numeric_limits<
                      ExactDirectMorseForestNodeId>::max()) {
            return fail(
                std::move(result), BuildFailure::budget_exhausted);
          }
          order_one_birth_node_id =
              static_cast<ExactDirectMorseForestNodeId>(node_index);
        }
        pending_births.push_back(
            {birth_index,
             role.event_projection_index,
             source_batch_index,
             source_batch.order,
             key,
             birth_index,
             order_one_birth_node_id,
             witness});
        if (order_one_birth_node_id.has_value()) {
          pending_birth_nodes.push_back(
              {*order_one_birth_node_id,
               source_batch.order,
               source_batch.squared_level,
               ExactDirectMorseForestNodeKind::order_one_birth,
               result.child_node_ids.size() + pending_children.size(),
               0U,
               birth_index,
               std::nullopt});
        }
        locator_bindings.push_back(
            {locator_bindings.size(), key, birth_index, witness});
      }
      if (pending_births.size() != source_batch.birth_role_count ||
          !append_count_within(
              result.birth_records.size(),
              pending_births.size(),
              budget.maximum_birth_record_count) ||
          !append_count_within(
              result.nodes.size(),
              pending_group_nodes.size() + pending_birth_nodes.size(),
              budget.maximum_node_count)) {
        return fail(
            std::move(result),
            BuildFailure::source_batch_inconsistent);
      }

      // All strict roots and the complete equal-level quotient are now
      // frozen.  Scientific mutation begins only here.
      try {
        for (const GroupPlan& plan : plans) {
          std::size_t root = plan.carrier_handles.front();
          for (std::size_t local = 1U;
               local < plan.carrier_handles.size();
               ++local) {
            root = components.unite(
                root, plan.carrier_handles[local]);
          }
          components.set_reduced_root(
              root, plan.resulting_root_node_id);
        }
        for (const auto& birth : pending_births) {
          components.activate(
              birth.component_handle,
              birth.order,
              birth.order_one_birth_node_id);
        }
      } catch (const std::logic_error&) {
        return fail(
            std::move(result),
            BuildFailure::source_batch_inconsistent);
      }

      const auto locator_commit = locator.apply_batch(
          std::span<const ExactDirectSparseFacetQuery>{},
          locator_unions,
          locator_bindings);
      if (!locator_commit.certified_committed_batch() ||
          locator_commit.counters.union_request_count !=
              locator_unions.size() ||
          locator_commit.counters.binding_request_count !=
              locator_bindings.size() ||
          locator_commit.counters.inserted_binding_count !=
              locator_bindings.size() ||
          locator_commit.counters.compatible_duplicate_binding_count !=
              0U) {
        return fail(
            std::move(result),
            BuildFailure::locator_commit_rejected);
      }

      result.arm_root_bindings.insert(
          result.arm_root_bindings.end(),
          pending_arm_bindings.begin(),
          pending_arm_bindings.end());
      result.saddle_records.insert(
          result.saddle_records.end(),
          pending_saddles.begin(),
          pending_saddles.end());
      result.atomic_groups.insert(
          result.atomic_groups.end(),
          pending_groups.begin(),
          pending_groups.end());
      result.child_node_ids.insert(
          result.child_node_ids.end(),
          pending_children.begin(),
          pending_children.end());
      result.nodes.insert(
          result.nodes.end(),
          pending_group_nodes.begin(),
          pending_group_nodes.end());
      result.birth_records.insert(
          result.birth_records.end(),
          pending_births.begin(),
          pending_births.end());
      result.nodes.insert(
          result.nodes.end(),
          pending_birth_nodes.begin(),
          pending_birth_nodes.end());
      result.counters.locator_union_count += locator_unions.size();

      result.batches.push_back(
          {result.batches.size(),
           source_batch_index,
           source_batch.order,
           source_batch.squared_level,
           result.birth_records.size() - pending_births.size(),
           pending_births.size(),
           result.saddle_records.size() - pending_saddles.size(),
           pending_saddles.size(),
           result.atomic_groups.size() - pending_groups.size(),
           pending_groups.size(),
           strict_pre_carrier_count,
           strict_pre_reduced_root_count,
           components.carrier_count(source_batch.order),
           components.reduced_root_count(source_batch.order),
           strict_pre_stamp,
           locator.snapshot_stamp(),
           true,
           true,
           true});
    }

    if (family_cursor != source_seed_journal.families.size() ||
        result.birth_records.size() != birth_count ||
        result.arm_root_bindings.size() !=
            source_seed_journal.arm_seeds.size() ||
        result.saddle_records.size() !=
            source_seed_journal.families.size() ||
        result.batches.size() != source_journal.batches.size()) {
      return fail(
          std::move(result),
          BuildFailure::source_batch_inconsistent);
    }

    for (std::size_t handle = 0U;
         handle < components.handle_count();
         ++handle) {
      if (!components.is_active_carrier_root(handle)) {
        continue;
      }
      const auto reduced_root = components.reduced_root(handle);
      if (!reduced_root.has_value()) {
        continue;
      }
      if (result.final_roots.size() >=
          budget.maximum_final_root_count) {
        return fail(
            std::move(result), BuildFailure::budget_exhausted);
      }
      result.final_roots.push_back(
          {result.final_roots.size(),
           components.order(handle),
           handle,
           *reduced_root});
    }
    std::sort(
        result.final_roots.begin(),
        result.final_roots.end(),
        [](const ExactDirectMorseForestFinalRoot& left,
           const ExactDirectMorseForestFinalRoot& right) {
          return left.order < right.order ||
                 (left.order == right.order &&
                  left.root_node_id < right.root_node_id);
        });
    for (std::size_t index = 0U;
         index < result.final_roots.size();
         ++index) {
      result.final_roots[index].final_root_index = index;
    }
    std::vector<std::size_t> root_count_by_order(
        result.effective_maximum_order + 1U, 0U);
    for (const auto& root : result.final_roots) {
      if (root.order == 0U ||
          root.order > result.effective_maximum_order) {
        return fail(
            std::move(result),
            BuildFailure::source_batch_inconsistent);
      }
      ++root_count_by_order[root.order];
    }
    for (std::size_t order = 1U;
         order <= result.effective_maximum_order;
         ++order) {
      const bool expects_reduced_root =
          order == 1U || order < result.point_count;
      if (root_count_by_order[order] !=
          (expects_reduced_root ? 1U : 0U)) {
        return fail(
            std::move(result),
            BuildFailure::source_batch_inconsistent);
      }
    }

    std::size_t key_point_count = 0U;
    for (const auto& birth : result.birth_records) {
      if (!try_add(
              key_point_count,
              birth.facet_key.point_count,
              key_point_count)) {
        return fail(
            std::move(result), BuildFailure::capacity_overflow);
      }
    }
    for (const auto& arm : result.arm_root_bindings) {
      if (!try_add(
              key_point_count,
              arm.strict_arm_key.point_count,
              key_point_count)) {
        return fail(
            std::move(result), BuildFailure::capacity_overflow);
      }
    }
    std::size_t logical = key_point_count;
    for (const std::size_t increment : {
             result.birth_records.size(),
             result.arm_root_bindings.size(),
             result.saddle_records.size(),
             result.atomic_groups.size(),
             result.child_node_ids.size(),
             result.batches.size(),
             result.final_roots.size(),
             result.nodes.size()}) {
      if (!try_add(logical, increment, logical)) {
        return fail(
            std::move(result), BuildFailure::capacity_overflow);
      }
    }
    if (logical > budget.maximum_logical_output_entry_count) {
      return fail(
          std::move(result), BuildFailure::budget_exhausted);
    }
    result.logical_output_entry_count = logical;
    result.final_locator_stamp = locator.snapshot_stamp();
    result.counters.birth_record_count = result.birth_records.size();
    result.counters.order_one_birth_node_count =
        static_cast<std::size_t>(std::count_if(
            result.birth_records.begin(),
            result.birth_records.end(),
            [](const ExactDirectMorseForestBirthRecord& birth) {
              return birth.order == 1U;
            }));
    result.counters.latent_higher_order_birth_count =
        result.birth_records.size() -
        result.counters.order_one_birth_node_count;
    result.counters.arm_root_binding_count =
        result.arm_root_bindings.size();
    result.counters.saddle_record_count = result.saddle_records.size();
    result.counters.atomic_group_count = result.atomic_groups.size();
    result.counters.child_reference_count =
        result.child_node_ids.size();
    result.counters.batch_record_count = result.batches.size();
    result.counters.node_count = result.nodes.size();
    result.counters.final_root_count = result.final_roots.size();

    result.every_birth_key_reconstructed_from_closed_direct_event = true;
    result.deterministic_disjoint_birth_union_and_query_tokens = true;
    result.batches_processed_in_strict_order_level_order = true;
    result.cardinality_isolates_orders_in_shared_locator = true;
    result.current_level_births_hidden_from_arm_descent = true;
    result.higher_order_direct_births_are_latent_carriers = true;
    result.one_10_5c_call_per_nonempty_strict_arm_batch = true;
    result.every_strict_arm_has_positive_terminal = true;
    result.all_catalogued_saddle_families_consumed_once = true;
    result.carrier_to_optional_reduced_root_authority_maintained = true;
    result.every_saddle_has_positive_carrier = true;
    result.typed_root_or_latent_carrier_hyperedges_closed_transitively = true;
    result.q_r_counts_only_distinct_prior_reduced_roots = true;
    result.all_equal_level_saddles_quotiented_before_mutation = true;
    result.saddle_records_grouped_with_source_family_provenance = true;
    result.q_zero_groups_create_one_reduced_birth = true;
    result.q_one_continuations_create_no_node = true;
    result.q_at_least_two_groups_create_one_multifusion = true;
    result.current_batch_birth_nodes_never_same_batch_children = true;
    result.all_group_carriers_attached_to_resulting_root_atomically = true;
    result.locator_commits_unions_before_current_birth_bindings = true;
    result.final_roots_cover_exactly_nonterminal_reduced_orders = true;
    result.no_partial_scientific_payload_published = true;
    result.decision = ExactDirectMorseForestDecision::
        complete_conditional_exact_direct_morse_forest;
    if (!result.certified_conditional_h0_candidate()) {
      throw std::logic_error(
          "a complete direct Morse forest failed its compact contract");
    }
    return result;
  } catch (const std::length_error&) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  } catch (const std::bad_alloc&) {
    return fail(std::move(result), BuildFailure::allocation_failed);
  } catch (const std::logic_error&) {
    return fail(
        std::move(result), BuildFailure::source_batch_inconsistent);
  }
}

ExactDirectMorseForestVerification
verify_exact_direct_morse_forest_journal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& trusted_budget,
    const ExactDirectMorseForestConfig& config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectMorseForestJournalResult& observed) {
  require_valid_traversal_order(traversal_order);
  require_closure_budget_within_confidence(trusted_budget.closure_budget);
  ExactDirectMorseForestVerification verification;
  verification.trusted_inputs_certified =
      index.validated_for(cloud) &&
      config.locator_config.external_authority_id != 0U;
  if (!verification.trusted_inputs_certified) {
    return verification;
  }
  verification.observed_storage_within_budget =
      observed.birth_records.size() <=
          trusted_budget.maximum_birth_record_count &&
      observed.arm_root_bindings.size() <=
          trusted_budget.maximum_arm_root_binding_count &&
      observed.saddle_records.size() <=
          trusted_budget.maximum_saddle_record_count &&
      observed.atomic_groups.size() <=
          trusted_budget.maximum_atomic_group_count &&
      observed.child_node_ids.size() <=
          trusted_budget.maximum_child_reference_count &&
      observed.batches.size() <=
          trusted_budget.maximum_batch_record_count &&
      observed.nodes.size() <= trusted_budget.maximum_node_count &&
      observed.final_roots.size() <=
          trusted_budget.maximum_final_root_count &&
      observed.logical_output_entry_count <=
          trusted_budget.maximum_logical_output_entry_count;
  if (!verification.observed_storage_within_budget) {
    return verification;
  }
  const auto expected = build_exact_direct_morse_forest_journal(
      index,
      cloud,
      source_facade,
      source_journal,
      trusted_seed_budget,
      source_seed_journal,
      trusted_budget,
      config,
      traversal_order);
  verification.expected_journal_freshly_reconstructed = true;
  verification.observed_recursively_equal = observed == expected;
  verification.result_certified =
      verification.observed_recursively_equal &&
      expected.certified_outcome();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
