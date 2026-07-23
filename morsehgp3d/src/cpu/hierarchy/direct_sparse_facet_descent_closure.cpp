#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_closure.hpp"

#include "direct_sparse_facet_descent_step_detail.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

enum class MemoVisitState : std::uint8_t {
  unseen,
  visiting,
  closed,
};

struct MemoSlot {
  std::uint64_t fingerprint{};
  std::size_t node_index{};
  bool occupied{false};
};

struct MemoSearchResult {
  std::size_t slot_index{};
  std::size_t node_index{};
  bool found{false};
};

struct SuccessorMiniballSlot {
  std::uint64_t fingerprint{};
  std::size_t cache_index{};
  bool occupied{false};
};

struct CachedSuccessorMiniball {
  ExactDirectSparseFacetKey facet_key{};
  ExactFacetMiniballResult miniball{};
};

enum class PathBuildStatus : std::uint8_t {
  complete,
  node_budget_exhausted,
  step_call_budget_exhausted,
  step_budget_exhausted,
  contradiction,
};

void require_valid_traversal_order(
    spatial::LbvhTraversalOrder traversal_order) {
  switch (traversal_order) {
    case spatial::LbvhTraversalOrder::near_first:
    case spatial::LbvhTraversalOrder::far_first:
      return;
  }
  throw std::invalid_argument("an LBVH traversal order is invalid");
}

[[nodiscard]] bool facet_key_less(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right) noexcept {
  const std::size_t shared_count =
      std::min(left.point_count, right.point_count);
  for (std::size_t index = 0U; index < shared_count; ++index) {
    if (left.point_ids[index] != right.point_ids[index]) {
      return left.point_ids[index] < right.point_ids[index];
    }
  }
  return left.point_count < right.point_count;
}

[[nodiscard]] bool valid_facet_key(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& key) noexcept {
  if (key.point_count == 0U ||
      key.point_count > direct_sparse_positive_facet_maximum_point_count ||
      key.point_count > cloud.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (static_cast<std::size_t>(key.point_ids[index]) >= cloud.size() ||
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

[[nodiscard]] std::span<const PointId> used_point_ids(
    const ExactDirectSparseFacetKey& key) noexcept {
  return std::span<const PointId>{key.point_ids}.first(key.point_count);
}

[[nodiscard]] std::size_t required_memo_slot_count(
    std::size_t maximum_node_count) {
  if (maximum_node_count >
      (std::numeric_limits<std::size_t>::max() - 1U) / 2U) {
    throw std::overflow_error("the closure memo-table size overflows size_t");
  }
  return maximum_node_count * 2U + 1U;
}

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

[[nodiscard]] bool try_increment_size(std::size_t& value) noexcept {
  if (value == std::numeric_limits<std::size_t>::max()) {
    return false;
  }
  ++value;
  return true;
}

[[nodiscard]] std::size_t checked_size_sum(
    std::size_t left,
    std::size_t right) {
  std::size_t sum = 0U;
  if (!try_add_size(left, right, sum)) {
    throw std::overflow_error(
        "a direct sparse descent-closure counter overflows size_t");
  }
  return sum;
}

void checked_add_counter(std::size_t& total, std::size_t increment) {
  total = checked_size_sum(total, increment);
}

void checked_increment_counter(std::size_t& counter) {
  checked_add_counter(counter, 1U);
}

void require_budget_within_confidence(
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
        "a direct sparse descent-closure budget exceeds its confidence cap");
  }
}

void add_step_counters(
    ExactDirectSparseFacetDescentStepCounters& total,
    const ExactDirectSparseFacetDescentStepCounters& step) {
  checked_add_counter(
      total.source_locator_probe_count, step.source_locator_probe_count);
  checked_add_counter(
      total.source_miniball_build_count, step.source_miniball_build_count);
  checked_add_counter(
      total.source_miniball_reuse_count, step.source_miniball_reuse_count);
  checked_add_counter(total.top_k_query_count, step.top_k_query_count);
  checked_add_counter(
      total.canonical_successor_selection_count,
      step.canonical_successor_selection_count);
  checked_add_counter(
      total.successor_miniball_build_count,
      step.successor_miniball_build_count);
  checked_add_counter(
      total.successor_miniball_reuse_count,
      step.successor_miniball_reuse_count);
  checked_add_counter(
      total.successor_source_distance_evaluation_count,
      step.successor_source_distance_evaluation_count);
  checked_add_counter(
      total.successor_source_maximum_comparison_count,
      step.successor_source_maximum_comparison_count);
  checked_add_counter(
      total.center_displacement_evaluation_count,
      step.center_displacement_evaluation_count);
  checked_add_counter(
      total.exact_level_relation_count, step.exact_level_relation_count);
  checked_add_counter(
      total.convex_segment_certification_count,
      step.convex_segment_certification_count);
  checked_add_counter(
      total.successor_locator_probe_count,
      step.successor_locator_probe_count);
}

[[nodiscard]] bool common_closed_scope(
    const ExactDirectSparseFacetDescentClosureResult& result) noexcept {
  return result.schema_version ==
             direct_sparse_facet_descent_closure_schema_version &&
         result.trusted_authorities_certified &&
         result.budget_preflight_completed &&
         result.common_locator_snapshot_certified &&
         result.every_memo_fingerprint_candidate_compared_by_full_key &&
         result.no_half_edge_published &&
         result.no_top_k_partition_or_shell_persisted &&
         !result.locator_state_mutated &&
         !result.locator_batch_committed &&
         !result.external_binding_authority_replayed &&
         !result.missing_facet_means_isolated &&
         !result.singleton_component_created &&
         !result.hierarchy_attachment_published &&
         !result.forbidden_global_structure_materialized &&
         !result.public_status_claimed && result.partial_refinement_only &&
         result.scope == ExactDirectSparseFacetDescentClosureScope::
             bounded_multi_source_memoized_strict_functional_forest_relative_positive_sink_propagation_only;
}

[[nodiscard]] bool graph_shape_well_formed(
    const ExactDirectSparseFacetDescentClosureResult& result) noexcept {
  std::size_t expected_cached_miniball_count = 0U;
  if (!try_add_size(
          result.counters.source_miniball_build_count,
          result.counters.successor_miniball_build_count,
          expected_cached_miniball_count)) {
    return false;
  }
  if (result.nodes.size() != result.counters.interned_node_count ||
      result.edges.size() != result.counters.strict_edge_count ||
      result.seed_projections.size() !=
          result.counters.processed_seed_reference_count ||
      result.counters.evaluated_step_source_count > result.nodes.size() ||
      result.counters.strict_edge_count >
          result.counters.aggregate_step_counters.top_k_query_count ||
      result.counters.aggregate_step_counters.top_k_query_count >
          result.counters.evaluated_step_source_count ||
      result.counters.aggregate_step_counters.source_locator_probe_count !=
          result.counters.evaluated_step_source_count ||
      result.counters.source_miniball_build_count !=
          result.counters.aggregate_step_counters
              .source_miniball_build_count ||
      result.counters.source_miniball_reuse_count !=
          result.counters.aggregate_step_counters
              .source_miniball_reuse_count ||
      result.counters.successor_miniball_build_count !=
          result.counters.aggregate_step_counters
              .successor_miniball_build_count ||
      result.counters.successor_miniball_reuse_count !=
          result.counters.aggregate_step_counters
              .successor_miniball_reuse_count ||
      result.counters.distinct_cached_miniball_count !=
          expected_cached_miniball_count) {
    return false;
  }

  std::size_t terminal_count = 0U;
  std::size_t positive_terminal_count = 0U;
  std::size_t unresolved_terminal_count = 0U;
  std::size_t budget_terminal_count = 0U;
  for (std::size_t node_index = 0U;
       node_index < result.nodes.size();
       ++node_index) {
    const ExactDirectSparseFacetDescentNode& node = result.nodes[node_index];
    if (node.node_index != node_index ||
        node.terminal_node_index >= result.nodes.size() ||
        !node.terminal_pointer_certified ||
        !node.full_miniball_not_persisted ||
        node.exact_center_and_level_present !=
            (node.exact_center.has_value() &&
             node.exact_squared_level.has_value())) {
      return false;
    }
    if (node.outgoing_edge_index.has_value()) {
      if (*node.outgoing_edge_index >= result.edges.size()) {
        return false;
      }
      const ExactDirectSparseFacetDescentEdge& edge =
          result.edges[*node.outgoing_edge_index];
      if (edge.source_node_index != node_index ||
          edge.target_node_index >= result.nodes.size() ||
          node.terminal_node_index !=
              result.nodes[edge.target_node_index].terminal_node_index ||
          node.closure_disposition !=
              result.nodes[edge.target_node_index].closure_disposition) {
        return false;
      }
    } else {
      if (!try_increment_size(terminal_count)) {
        return false;
      }
      if (node.terminal_node_index != node_index) {
        return false;
      }
      switch (node.closure_disposition) {
        case ExactDirectSparseFacetDescentClosureDisposition::relative_positive:
          if (!try_increment_size(positive_terminal_count)) {
            return false;
          }
          if (!node.resolved_component_handle.has_value() ||
              !node.resolved_binding_witness.has_value()) {
            return false;
          }
          break;
        case ExactDirectSparseFacetDescentClosureDisposition::unresolved:
          if (!try_increment_size(unresolved_terminal_count)) {
            return false;
          }
          if (node.resolved_component_handle.has_value() ||
              node.resolved_binding_witness.has_value()) {
            return false;
          }
          break;
        case ExactDirectSparseFacetDescentClosureDisposition::budget_exhausted:
          if (!try_increment_size(budget_terminal_count)) {
            return false;
          }
          if (node.resolved_component_handle.has_value() ||
              node.resolved_binding_witness.has_value()) {
            return false;
          }
          break;
        default:
          return false;
      }
    }
  }

  for (std::size_t edge_index = 0U;
       edge_index < result.edges.size();
       ++edge_index) {
    const ExactDirectSparseFacetDescentEdge& edge = result.edges[edge_index];
    if (edge.edge_index != edge_index ||
        edge.source_node_index >= result.nodes.size() ||
        edge.target_node_index >= result.nodes.size() ||
        edge.source_node_index == edge.target_node_index ||
        !edge.source_and_target_keys_match_nodes ||
        !edge.target_center_and_level_match_node ||
        !edge.strict_level_decrease_certified ||
        !edge.same_closed_batch_level_certified ||
        result.nodes[edge.source_node_index].facet_key !=
            edge.strict_step_witness.source_facet_key ||
        result.nodes[edge.target_node_index].facet_key !=
            edge.strict_step_witness.successor_facet_key ||
        !(edge.strict_step_witness.successor_facet_squared_level <
          edge.strict_step_witness.source_facet_squared_level) ||
        !(edge.strict_step_witness.source_facet_squared_level <=
          result.closed_batch_squared_level)) {
      return false;
    }
  }

  for (const ExactDirectSparseFacetDescentSeedProjection& seed :
       result.seed_projections) {
    if (seed.root_node_index >= result.nodes.size() ||
        seed.terminal_node_index >= result.nodes.size() ||
        result.nodes[seed.root_node_index].facet_key !=
            seed.source_facet_key ||
        result.nodes[seed.root_node_index].terminal_node_index !=
            seed.terminal_node_index ||
        result.nodes[seed.root_node_index].closure_disposition !=
            seed.closure_disposition) {
      return false;
    }
  }

  std::size_t graph_cardinality = 0U;
  if (!try_add_size(result.edges.size(), terminal_count, graph_cardinality)) {
    return false;
  }

  return terminal_count == result.counters.terminal_node_count &&
         positive_terminal_count ==
             result.counters.relative_positive_terminal_count &&
         unresolved_terminal_count ==
             result.counters.unresolved_terminal_count &&
         budget_terminal_count == result.counters.budget_terminal_count &&
         graph_cardinality == result.nodes.size() &&
         (result.nodes.empty() || result.edges.size() < result.nodes.size());
}

void initialize_closed_scope(
    ExactDirectSparseFacetDescentClosureResult& result) noexcept {
  result.locator_state_mutated = false;
  result.locator_batch_committed = false;
  result.external_binding_authority_replayed = false;
  result.missing_facet_means_isolated = false;
  result.singleton_component_created = false;
  result.hierarchy_attachment_published = false;
  result.forbidden_global_structure_materialized = false;
  result.public_status_claimed = false;
  result.partial_refinement_only = true;
  result.no_half_edge_published = true;
  result.no_top_k_partition_or_shell_persisted = true;
  result.scope = ExactDirectSparseFacetDescentClosureScope::
      bounded_multi_source_memoized_strict_functional_forest_relative_positive_sink_propagation_only;
}

class ClosureBuilder {
 public:
  ClosureBuilder(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      const exact::ExactLevel& closed_batch_squared_level,
      const ExactDirectSparseFacetWitness& locator_query_witness,
      const ExactDirectSparsePositiveFacetLocator& locator,
      const ExactDirectSparseFacetDescentClosureBudget& budget,
      const ExactDirectSparseFacetDescentClosureConfig& config,
      spatial::LbvhTraversalOrder traversal_order,
      ExactDirectSparseFacetDescentClosureResult& result)
      : index_(index),
        cloud_(cloud),
        closed_batch_squared_level_(closed_batch_squared_level),
        locator_query_witness_(locator_query_witness),
        locator_(locator),
        budget_(budget),
        config_(config),
        traversal_order_(traversal_order),
        result_(result),
        memo_slots_(result.required_memo_slot_count),
        successor_miniball_slots_(result.required_memo_slot_count) {
    const std::size_t initial_reserve_count = checked_size_sum(
        result_.counters.distinct_seed_key_count, 1U);
    result_.nodes.reserve(
        std::min(budget_.maximum_node_count,
                 initial_reserve_count));
    result_.edges.reserve(
        std::min(budget_.maximum_node_count,
                 initial_reserve_count));
    cached_miniballs_.reserve(
        std::min(budget_.maximum_node_count,
                 initial_reserve_count));
    successor_miniballs_.reserve(
        std::min(budget_.maximum_step_call_count,
                 initial_reserve_count));
    visit_states_.reserve(
        std::min(budget_.maximum_node_count,
                 initial_reserve_count));
    node_created_as_successor_.reserve(
        std::min(budget_.maximum_node_count,
                 initial_reserve_count));
    path_.reserve(
        std::min(budget_.maximum_node_count,
                 initial_reserve_count));
  }

  void intern_seed_roots(
      std::span<const ExactDirectSparseFacetKey> seed_keys) {
    for (const ExactDirectSparseFacetKey& seed_key : seed_keys) {
      const MemoSearchResult search = memo_search(seed_key, true);
      if (search.found ||
          result_.nodes.size() >= budget_.maximum_node_count) {
        throw std::logic_error(
            "canonical distinct roots did not fit their certified preflight");
      }
      static_cast<void>(create_node(
          seed_key,
          search.slot_index,
          ExactDirectSparseFacetDescentNodeKind::evaluated_step_source,
          MemoVisitState::unseen,
          false));
    }
  }

  [[nodiscard]] PathBuildStatus process_root(
      const ExactDirectSparseFacetKey& root_key) {
    MemoSearchResult root_search = memo_search(root_key, true);
    if (!root_search.found) {
      throw std::logic_error(
          "a preflighted direct sparse closure root was not interned");
    }
    if (visit_states_[root_search.node_index] == MemoVisitState::closed) {
      checked_increment_counter(
          result_.counters.memoized_suffix_reuse_count);
      return PathBuildStatus::complete;
    }
    if (visit_states_[root_search.node_index] == MemoVisitState::visiting) {
      poison_cycle(
          root_key,
          root_key,
          ExactDirectSparseFacetDescentStepDecision::not_certified,
          true,
          false);
      return PathBuildStatus::contradiction;
    }

    const std::size_t root_index = root_search.node_index;
    visit_states_[root_index] = MemoVisitState::visiting;
    path_.clear();
    path_.push_back(root_index);
    std::size_t current_index = root_index;

    for (;;) {
      if (result_.counters.evaluated_step_source_count >=
          budget_.maximum_step_call_count) {
        make_graph_budget_terminal(current_index);
        close_path_from_terminal(current_index);
        return PathBuildStatus::step_call_budget_exhausted;
      }

      const ExactFacetMiniballResult* prepared_source =
          find_cached_miniball(
              result_.nodes[current_index].facet_key);
      const detail::ExactDirectSparseCertifiedFacetMiniballLookup lookup{
          this, &ClosureBuilder::find_cached_miniball_callback};
      detail::ExactDirectSparseFacetDescentStepTransient computation =
          detail::build_exact_direct_sparse_facet_descent_step_transient(
              index_,
              cloud_,
              used_point_ids(result_.nodes[current_index].facet_key),
              closed_batch_squared_level_,
              locator_query_witness_,
              locator_,
              budget_.step_budget,
              traversal_order_,
              prepared_source,
              lookup);
      require_cache_counter_integrity();
      checked_increment_counter(
          result_.counters.evaluated_step_source_count);
      checked_increment_counter(
          result_.counters.locator_snapshot_check_count);
      if (locator_.snapshot_stamp() != result_.locator_snapshot_stamp) {
        throw std::runtime_error(
            "the positive-facet locator changed during one closure replay");
      }

      ExactDirectSparseFacetDescentStepResult& step = computation.result;
      if (!step.certified_partial_refinement_outcome()) {
        throw std::logic_error(
            "the 10.5b core returned no certified local disposition");
      }
      accumulate_step(step);
      ExactDirectSparseFacetDescentNode& current =
          result_.nodes[current_index];
      current.kind =
          ExactDirectSparseFacetDescentNodeKind::evaluated_step_source;
      current.step_evaluated = true;
      current.local_step_disposition = step.disposition;
      current.local_step_decision = step.decision;
      current.local_step_projection_certified = true;
      current.full_miniball_not_persisted = true;

      if (computation.newly_built_source_miniball.has_value()) {
        if (cached_miniballs_[current_index].has_value()) {
          throw std::logic_error(
              "a cached source miniball was rebuilt inside one closure");
        }
        cached_miniballs_[current_index].emplace(
            std::move(*computation.newly_built_source_miniball));
      }
      if (computation.newly_built_successor_miniball.has_value()) {
        cache_new_successor_miniball(
            std::move(*computation.newly_built_successor_miniball));
      }
      set_node_geometry_from_cache(current_index);

      if (step.certified_fail_closed_contradiction()) {
        poison_cycle(
            current.facet_key,
            {},
            step.decision,
            false,
            false);
        return PathBuildStatus::contradiction;
      }
      if (step.certified_budget_exhaustion()) {
        make_step_budget_terminal(current_index, step);
        close_path_from_terminal(current_index);
        return PathBuildStatus::step_budget_exhausted;
      }
      if (step.certified_relative_positive_resolution() &&
          step.decision == ExactDirectSparseFacetDescentStepDecision::
                               complete_relative_source_positive_hit) {
        checked_increment_counter(
            result_.counters.source_positive_hit_count);
        current.resolved_component_handle = step.resolved_component_handle;
        current.resolved_binding_witness = step.resolved_binding_witness;
        current.closure_disposition =
            ExactDirectSparseFacetDescentClosureDisposition::relative_positive;
        current.terminal_node_index = current_index;
        close_path_from_terminal(current_index);
        return PathBuildStatus::complete;
      }

      const bool strict_unresolved =
          step.certified_unresolved_without_isolation() &&
          step.decision == ExactDirectSparseFacetDescentStepDecision::
                               complete_unresolved_strict_successor_not_bound;
      const bool strict_positive =
          step.certified_relative_positive_resolution() &&
          step.decision == ExactDirectSparseFacetDescentStepDecision::
                               complete_relative_strict_successor_positive_hit;
      if (!strict_unresolved && !strict_positive) {
        current.closure_disposition =
            ExactDirectSparseFacetDescentClosureDisposition::unresolved;
        current.terminal_node_index = current_index;
        close_path_from_terminal(current_index);
        return PathBuildStatus::complete;
      }

      if (!step.strict_step_witness.has_value()) {
        throw std::logic_error(
            "a complete strict local decision lost its strict witness");
      }
      if (strict_positive) {
        checked_increment_counter(
            result_.counters.successor_positive_hit_count);
      }
      const ExactDirectSparseFacetDescentStepWitness witness =
          *step.strict_step_witness;
      MemoSearchResult target_search =
          memo_search(witness.successor_facet_key, true);
      const bool target_was_unseen =
          target_search.found &&
          visit_states_[target_search.node_index] == MemoVisitState::unseen;
      const bool target_was_closed =
          target_search.found &&
          visit_states_[target_search.node_index] == MemoVisitState::closed;
      if (target_search.found &&
          visit_states_[target_search.node_index] == MemoVisitState::visiting) {
        poison_cycle(
            current.facet_key,
            witness.successor_facet_key,
            step.decision,
            true,
            false);
        return PathBuildStatus::contradiction;
      }

      if (!target_search.found &&
          result_.nodes.size() >= budget_.maximum_node_count) {
        current.diagnostic_strict_step_witness = witness;
        checked_increment_counter(
            result_.counters.diagnostic_strict_witness_without_edge_count);
        current.closure_disposition =
            ExactDirectSparseFacetDescentClosureDisposition::budget_exhausted;
        current.terminal_node_index = current_index;
        close_path_from_terminal(current_index);
        return PathBuildStatus::node_budget_exhausted;
      }

      std::size_t target_index = 0U;
      if (target_search.found) {
        target_index = target_search.node_index;
        if (!attach_successor_miniball(target_index, witness)) {
          poison_cycle(
              current.facet_key,
              witness.successor_facet_key,
              step.decision,
              false,
              true);
          return PathBuildStatus::contradiction;
        }
      } else if (strict_positive) {
        target_index = create_node(
            witness.successor_facet_key,
            target_search.slot_index,
            ExactDirectSparseFacetDescentNodeKind::positive_locator_terminal,
            MemoVisitState::closed,
            true);
        if (!attach_successor_miniball(target_index, witness)) {
          throw std::logic_error(
              "a new positive target lost its certified successor miniball");
        }
        ExactDirectSparseFacetDescentNode& target =
            result_.nodes[target_index];
        target.resolved_component_handle = step.resolved_component_handle;
        target.resolved_binding_witness = step.resolved_binding_witness;
        target.closure_disposition =
            ExactDirectSparseFacetDescentClosureDisposition::relative_positive;
        target.terminal_node_index = target_index;
        target.terminal_pointer_certified = true;
        target.full_miniball_not_persisted = true;
      } else {
        target_index = create_node(
            witness.successor_facet_key,
            target_search.slot_index,
            ExactDirectSparseFacetDescentNodeKind::evaluated_step_source,
            MemoVisitState::visiting,
            true);
        if (!attach_successor_miniball(target_index, witness)) {
          throw std::logic_error(
              "a new strict target lost its certified successor miniball");
        }
      }

      if (target_was_unseen) {
        node_created_as_successor_[target_index] = 1U;
        if (strict_positive) {
          ExactDirectSparseFacetDescentNode& target =
              result_.nodes[target_index];
          target.kind = ExactDirectSparseFacetDescentNodeKind::
              positive_locator_terminal;
          target.resolved_component_handle = step.resolved_component_handle;
          target.resolved_binding_witness = step.resolved_binding_witness;
          target.closure_disposition =
              ExactDirectSparseFacetDescentClosureDisposition::
                  relative_positive;
          target.terminal_node_index = target_index;
          target.terminal_pointer_certified = true;
          target.full_miniball_not_persisted = true;
          visit_states_[target_index] = MemoVisitState::closed;
        } else {
          visit_states_[target_index] = MemoVisitState::visiting;
        }
      } else if (target_was_closed) {
        const ExactDirectSparseFacetDescentNode& target =
            result_.nodes[target_index];
        if (strict_positive &&
            (target.closure_disposition !=
                 ExactDirectSparseFacetDescentClosureDisposition::
                     relative_positive ||
             target.resolved_component_handle !=
                 step.resolved_component_handle ||
             target.resolved_binding_witness !=
                 step.resolved_binding_witness)) {
          poison_cycle(
              current.facet_key,
              witness.successor_facet_key,
              step.decision,
              false,
              true);
          return PathBuildStatus::contradiction;
        }
        if (strict_unresolved &&
            target.closure_disposition ==
                ExactDirectSparseFacetDescentClosureDisposition::
                    relative_positive) {
          poison_cycle(
              current.facet_key,
              witness.successor_facet_key,
              step.decision,
              false,
              true);
          return PathBuildStatus::contradiction;
        }
      }

      add_strict_edge(current_index, target_index, witness);
      if (target_was_closed) {
        checked_increment_counter(
            result_.counters.memoized_suffix_reuse_count);
        close_path_to_closed_target(target_index);
        return PathBuildStatus::complete;
      }
      if (strict_positive) {
        close_path_to_closed_target(target_index);
        return PathBuildStatus::complete;
      }

      path_.push_back(target_index);
      current_index = target_index;
    }
  }

  void close_unseen_seed_roots_as_budget_terminals() noexcept {
    for (std::size_t node_index = 0U;
         node_index < result_.nodes.size();
         ++node_index) {
      if (visit_states_[node_index] != MemoVisitState::unseen) {
        continue;
      }
      make_graph_budget_terminal(node_index);
      result_.nodes[node_index].terminal_pointer_certified = true;
      visit_states_[node_index] = MemoVisitState::closed;
    }
  }

  [[nodiscard]] MemoSearchResult find_node(
      const ExactDirectSparseFacetKey& key) {
    return memo_search(key, true);
  }

  [[nodiscard]] bool node_created_as_successor(
      std::size_t node_index) const noexcept {
    return node_created_as_successor_[node_index] != 0U;
  }

  [[nodiscard]] const std::vector<MemoVisitState>& visit_states()
      const noexcept {
    return visit_states_;
  }

  [[nodiscard]] std::size_t cached_miniball_count() const {
    const std::size_t source_count = static_cast<std::size_t>(
        std::count_if(
            cached_miniballs_.begin(),
            cached_miniballs_.end(),
            [](const std::optional<ExactFacetMiniballResult>& value) {
              return value.has_value();
            }));
    return checked_size_sum(source_count, successor_miniballs_.size());
  }

 private:
  [[nodiscard]] std::uint64_t fingerprint(
      const ExactDirectSparseFacetKey& key) const noexcept {
    std::uint64_t value = UINT64_C(1469598103934665603);
    value ^= static_cast<std::uint64_t>(key.point_count);
    value *= UINT64_C(1099511628211);
    for (std::size_t index = 0U; index < key.point_count; ++index) {
      value ^= static_cast<std::uint64_t>(key.point_ids[index]);
      value *= UINT64_C(1099511628211);
    }
    return value & config_.memo_fingerprint_mask;
  }

  [[nodiscard]] MemoSearchResult memo_search(
      const ExactDirectSparseFacetKey& key,
      bool count_work) {
    const std::uint64_t key_fingerprint = fingerprint(key);
    std::size_t slot_index = static_cast<std::size_t>(
        key_fingerprint % static_cast<std::uint64_t>(memo_slots_.size()));
    for (std::size_t visit = 0U; visit < memo_slots_.size(); ++visit) {
      if (count_work) {
        checked_increment_counter(
            result_.counters.memo_slot_visit_count);
      }
      const MemoSlot& slot = memo_slots_[slot_index];
      if (!slot.occupied) {
        return {slot_index, 0U, false};
      }
      if (slot.fingerprint == key_fingerprint) {
        if (count_work) {
          checked_increment_counter(
              result_.counters.memo_full_key_comparison_count);
        }
        if (result_.nodes[slot.node_index].facet_key == key) {
          return {slot_index, slot.node_index, true};
        }
        if (count_work) {
          checked_increment_counter(
              result_.counters.equal_fingerprint_distinct_key_count);
        }
      }
      slot_index = (slot_index + 1U) % memo_slots_.size();
    }
    throw std::logic_error(
        "a below-half-load closure memo table has no empty terminator");
  }

  [[nodiscard]] std::size_t create_node(
      const ExactDirectSparseFacetKey& key,
      std::size_t slot_index,
      ExactDirectSparseFacetDescentNodeKind kind,
      MemoVisitState state,
      bool created_as_successor) {
    const std::size_t node_index = result_.nodes.size();
    ExactDirectSparseFacetDescentNode node;
    node.node_index = node_index;
    node.facet_key = key;
    node.terminal_node_index = node_index;
    node.kind = kind;
    node.full_miniball_not_persisted = true;
    result_.nodes.push_back(std::move(node));
    cached_miniballs_.emplace_back();
    visit_states_.push_back(state);
    node_created_as_successor_.push_back(
        created_as_successor ? 1U : 0U);
    memo_slots_[slot_index] = {fingerprint(key), node_index, true};
    return node_index;
  }

  [[nodiscard]] bool try_increment_cache_counter(
      std::size_t& counter) noexcept {
    // The cache callback ABI is allocation-free and noexcept.  Keep a sticky
    // failure bit and report a miss on overflow; process_root checks the bit
    // immediately after the step core returns, before accepting its result.
    if (!try_increment_size(counter)) {
      cache_counter_overflow_ = true;
      return false;
    }
    return true;
  }

  void require_cache_counter_integrity() const {
    if (cache_counter_overflow_) {
      throw std::overflow_error(
          "an allocation-free closure cache-work counter overflows size_t");
    }
  }

  [[nodiscard]] const ExactFacetMiniballResult*
  find_cached_miniball_noexcept(
      const ExactDirectSparseFacetKey& key) noexcept {
    const std::uint64_t key_fingerprint = fingerprint(key);
    std::size_t slot_index = static_cast<std::size_t>(
        key_fingerprint % static_cast<std::uint64_t>(memo_slots_.size()));
    for (std::size_t visit = 0U; visit < memo_slots_.size(); ++visit) {
      if (!try_increment_cache_counter(
              result_.counters.memo_slot_visit_count)) {
        return nullptr;
      }
      const MemoSlot& slot = memo_slots_[slot_index];
      if (!slot.occupied) {
        break;
      }
      if (slot.fingerprint == key_fingerprint) {
        if (!try_increment_cache_counter(
                result_.counters.memo_full_key_comparison_count)) {
          return nullptr;
        }
        if (result_.nodes[slot.node_index].facet_key == key) {
          if (cached_miniballs_[slot.node_index].has_value()) {
            return &*cached_miniballs_[slot.node_index];
          }
          break;
        }
        if (!try_increment_cache_counter(
                result_.counters.equal_fingerprint_distinct_key_count)) {
          return nullptr;
        }
      }
      slot_index = (slot_index + 1U) % memo_slots_.size();
    }

    slot_index = static_cast<std::size_t>(
        key_fingerprint %
        static_cast<std::uint64_t>(successor_miniball_slots_.size()));
    for (std::size_t visit = 0U;
         visit < successor_miniball_slots_.size();
         ++visit) {
      if (!try_increment_cache_counter(
              result_.counters.memo_slot_visit_count)) {
        return nullptr;
      }
      const SuccessorMiniballSlot& slot =
          successor_miniball_slots_[slot_index];
      if (!slot.occupied) {
        return nullptr;
      }
      if (slot.fingerprint == key_fingerprint) {
        if (!try_increment_cache_counter(
                result_.counters.memo_full_key_comparison_count)) {
          return nullptr;
        }
        const CachedSuccessorMiniball& cached =
            successor_miniballs_[slot.cache_index];
        if (cached.facet_key == key) {
          return &cached.miniball;
        }
        if (!try_increment_cache_counter(
                result_.counters.equal_fingerprint_distinct_key_count)) {
          return nullptr;
        }
      }
      slot_index =
          (slot_index + 1U) % successor_miniball_slots_.size();
    }
    return nullptr;
  }

  [[nodiscard]] const ExactFacetMiniballResult* find_cached_miniball(
      const ExactDirectSparseFacetKey& key) {
    const ExactFacetMiniballResult* miniball =
        find_cached_miniball_noexcept(key);
    require_cache_counter_integrity();
    return miniball;
  }

  [[nodiscard]] static const ExactFacetMiniballResult*
  find_cached_miniball_callback(
      const void* context,
      const ExactDirectSparseFacetKey& key) noexcept {
    auto* self = const_cast<ClosureBuilder*>(
        static_cast<const ClosureBuilder*>(context));
    return self->find_cached_miniball_noexcept(key);
  }

  void set_node_geometry_from_cache(std::size_t node_index) {
    const ExactFacetMiniballResult* miniball =
        find_cached_miniball(result_.nodes[node_index].facet_key);
    if (miniball == nullptr) {
      return;
    }
    ExactDirectSparseFacetDescentNode& node = result_.nodes[node_index];
    node.exact_center = miniball->center;
    node.exact_squared_level = miniball->squared_radius;
    node.exact_center_and_level_present = true;
  }

  void cache_new_successor_miniball(ExactFacetMiniballResult miniball) {
    ExactDirectSparseFacetKey key;
    if (miniball.facet_point_ids.empty() ||
        miniball.facet_point_ids.size() > key.point_ids.size()) {
      throw std::logic_error(
          "a newly certified successor miniball has an invalid full key");
    }
    key.point_count = miniball.facet_point_ids.size();
    for (std::size_t index = 0U;
         index < miniball.facet_point_ids.size();
         ++index) {
      key.point_ids[index] = miniball.facet_point_ids[index];
    }
    if (find_cached_miniball(key) != nullptr) {
      throw std::logic_error(
          "a certified successor miniball was rebuilt for one full key");
    }

    const std::uint64_t key_fingerprint = fingerprint(key);
    std::size_t slot_index = static_cast<std::size_t>(
        key_fingerprint %
        static_cast<std::uint64_t>(successor_miniball_slots_.size()));
    for (std::size_t visit = 0U;
         visit < successor_miniball_slots_.size();
         ++visit) {
      SuccessorMiniballSlot& slot = successor_miniball_slots_[slot_index];
      if (!slot.occupied) {
        const std::size_t cache_index = successor_miniballs_.size();
        successor_miniballs_.push_back({key, std::move(miniball)});
        slot = {key_fingerprint, cache_index, true};
        return;
      }
      if (slot.fingerprint == key_fingerprint &&
          successor_miniballs_[slot.cache_index].facet_key == key) {
        throw std::logic_error(
            "a certified successor miniball was rebuilt for one full key");
      }
      slot_index =
          (slot_index + 1U) % successor_miniball_slots_.size();
    }
    throw std::logic_error(
        "the bounded successor-miniball memo has no empty slot");
  }

  [[nodiscard]] bool attach_successor_miniball(
      std::size_t target_index,
      const ExactDirectSparseFacetDescentStepWitness& witness) {
    const ExactFacetMiniballResult* miniball =
        find_cached_miniball(witness.successor_facet_key);
    if (miniball == nullptr) {
      return false;
    }
    set_node_geometry_from_cache(target_index);
    const ExactDirectSparseFacetDescentNode& target =
        result_.nodes[target_index];
    return target.facet_key == witness.successor_facet_key &&
           target.exact_center ==
               std::optional<exact::ExactCenter3>{witness.successor_center} &&
           target.exact_squared_level ==
               std::optional<exact::ExactLevel>{
                   witness.successor_facet_squared_level} &&
           miniball->center == witness.successor_center &&
           miniball->squared_radius ==
               witness.successor_facet_squared_level;
  }

  void accumulate_step(
      const ExactDirectSparseFacetDescentStepResult& step) {
    add_step_counters(result_.counters.aggregate_step_counters, step.counters);
    checked_add_counter(
        result_.counters.source_miniball_build_count,
        step.counters.source_miniball_build_count);
    checked_add_counter(
        result_.counters.source_miniball_reuse_count,
        step.counters.source_miniball_reuse_count);
    checked_add_counter(
        result_.counters.successor_miniball_build_count,
        step.counters.successor_miniball_build_count);
    checked_add_counter(
        result_.counters.successor_miniball_reuse_count,
        step.counters.successor_miniball_reuse_count);
  }

  void add_strict_edge(
      std::size_t source_index,
      std::size_t target_index,
      const ExactDirectSparseFacetDescentStepWitness& witness) {
    ExactDirectSparseFacetDescentNode& source = result_.nodes[source_index];
    const ExactDirectSparseFacetDescentNode& target =
        result_.nodes[target_index];
    if (source.outgoing_edge_index.has_value() ||
        !source.exact_center_and_level_present ||
        !target.exact_center_and_level_present ||
        source.exact_center !=
            std::optional<exact::ExactCenter3>{witness.source_center} ||
        source.exact_squared_level !=
            std::optional<exact::ExactLevel>{
                witness.source_facet_squared_level}) {
      throw std::logic_error(
          "a strict closure edge does not match its source miniball seam");
    }
    ExactDirectSparseFacetDescentEdge edge;
    edge.edge_index = result_.edges.size();
    edge.source_node_index = source_index;
    edge.target_node_index = target_index;
    edge.strict_step_witness = witness;
    edge.source_and_target_keys_match_nodes =
        source.facet_key == witness.source_facet_key &&
        target.facet_key == witness.successor_facet_key;
    edge.target_center_and_level_match_node =
        target.exact_center ==
            std::optional<exact::ExactCenter3>{witness.successor_center} &&
        target.exact_squared_level ==
            std::optional<exact::ExactLevel>{
                witness.successor_facet_squared_level};
    edge.strict_level_decrease_certified =
        witness.successor_facet_squared_level <
        witness.source_facet_squared_level;
    edge.same_closed_batch_level_certified =
        witness.source_facet_squared_level <= closed_batch_squared_level_;
    if (!edge.source_and_target_keys_match_nodes ||
        !edge.target_center_and_level_match_node ||
        !edge.strict_level_decrease_certified ||
        !edge.same_closed_batch_level_certified) {
      throw std::logic_error(
          "a strict closure edge failed its exact seam precommit");
    }
    source.outgoing_edge_index = edge.edge_index;
    source.diagnostic_strict_step_witness.reset();
    result_.edges.push_back(std::move(edge));
  }

  void make_graph_budget_terminal(std::size_t node_index) noexcept {
    ExactDirectSparseFacetDescentNode& node = result_.nodes[node_index];
    node.kind = ExactDirectSparseFacetDescentNodeKind::graph_budget_terminal;
    node.step_evaluated = false;
    node.local_step_projection_certified = false;
    node.closure_disposition =
        ExactDirectSparseFacetDescentClosureDisposition::budget_exhausted;
    node.terminal_node_index = node_index;
    node.full_miniball_not_persisted = true;
  }

  void make_step_budget_terminal(
      std::size_t node_index,
      const ExactDirectSparseFacetDescentStepResult& step) {
    ExactDirectSparseFacetDescentNode& node = result_.nodes[node_index];
    node.closure_disposition =
        ExactDirectSparseFacetDescentClosureDisposition::budget_exhausted;
    node.terminal_node_index = node_index;
    node.diagnostic_strict_step_witness = step.strict_step_witness;
    if (step.strict_step_witness.has_value()) {
      checked_increment_counter(
          result_.counters.diagnostic_strict_witness_without_edge_count);
    }
  }

  void close_path_from_terminal(std::size_t terminal_index) noexcept {
    const ExactDirectSparseFacetDescentClosureDisposition disposition =
        result_.nodes[terminal_index].closure_disposition;
    for (auto iterator = path_.rbegin(); iterator != path_.rend(); ++iterator) {
      ExactDirectSparseFacetDescentNode& node = result_.nodes[*iterator];
      node.terminal_node_index = terminal_index;
      node.closure_disposition = disposition;
      node.terminal_pointer_certified = true;
      visit_states_[*iterator] = MemoVisitState::closed;
    }
  }

  void close_path_to_closed_target(std::size_t target_index) noexcept {
    const std::size_t terminal_index =
        result_.nodes[target_index].terminal_node_index;
    const ExactDirectSparseFacetDescentClosureDisposition disposition =
        result_.nodes[target_index].closure_disposition;
    for (auto iterator = path_.rbegin(); iterator != path_.rend(); ++iterator) {
      ExactDirectSparseFacetDescentNode& node = result_.nodes[*iterator];
      node.terminal_node_index = terminal_index;
      node.closure_disposition = disposition;
      node.terminal_pointer_certified = true;
      visit_states_[*iterator] = MemoVisitState::closed;
    }
  }

  void poison_cycle(
      const ExactDirectSparseFacetKey& source_key,
      const ExactDirectSparseFacetKey& target_key,
      ExactDirectSparseFacetDescentStepDecision local_decision,
      bool visiting_cycle,
      bool geometry_mismatch) noexcept {
    result_.contradiction_witness.emplace(
        ExactDirectSparseFacetDescentContradictionWitness{
            source_key,
            target_key,
            local_decision,
            visiting_cycle,
            geometry_mismatch});
  }

  const spatial::MortonLbvhIndex& index_;
  const spatial::CanonicalPointCloud& cloud_;
  const exact::ExactLevel& closed_batch_squared_level_;
  const ExactDirectSparseFacetWitness& locator_query_witness_;
  const ExactDirectSparsePositiveFacetLocator& locator_;
  const ExactDirectSparseFacetDescentClosureBudget& budget_;
  const ExactDirectSparseFacetDescentClosureConfig& config_;
  spatial::LbvhTraversalOrder traversal_order_;
  ExactDirectSparseFacetDescentClosureResult& result_;
  std::vector<MemoSlot> memo_slots_;
  std::vector<std::optional<ExactFacetMiniballResult>> cached_miniballs_;
  std::vector<SuccessorMiniballSlot> successor_miniball_slots_;
  std::vector<CachedSuccessorMiniball> successor_miniballs_;
  std::vector<MemoVisitState> visit_states_;
  std::vector<std::uint8_t> node_created_as_successor_;
  std::vector<std::size_t> path_;
  bool cache_counter_overflow_{false};
};

void canonicalize_graph(
    ExactDirectSparseFacetDescentClosureResult& result) {
  const std::size_t node_count = result.nodes.size();
  std::vector<std::size_t> node_order(node_count);
  for (std::size_t index = 0U; index < node_count; ++index) {
    node_order[index] = index;
  }
  std::sort(
      node_order.begin(),
      node_order.end(),
      [&result](std::size_t left, std::size_t right) {
        return facet_key_less(
            result.nodes[left].facet_key,
            result.nodes[right].facet_key);
      });
  std::vector<std::size_t> node_remap(node_count);
  std::vector<ExactDirectSparseFacetDescentNode> canonical_nodes;
  canonical_nodes.reserve(node_count);
  for (std::size_t new_index = 0U;
       new_index < node_order.size();
       ++new_index) {
    node_remap[node_order[new_index]] = new_index;
    canonical_nodes.push_back(std::move(result.nodes[node_order[new_index]]));
  }
  for (std::size_t node_index = 0U;
       node_index < canonical_nodes.size();
       ++node_index) {
    ExactDirectSparseFacetDescentNode& node = canonical_nodes[node_index];
    node.node_index = node_index;
    node.terminal_node_index = node_remap[node.terminal_node_index];
  }

  for (ExactDirectSparseFacetDescentEdge& edge : result.edges) {
    edge.source_node_index = node_remap[edge.source_node_index];
    edge.target_node_index = node_remap[edge.target_node_index];
  }
  std::sort(
      result.edges.begin(),
      result.edges.end(),
      [](const ExactDirectSparseFacetDescentEdge& left,
         const ExactDirectSparseFacetDescentEdge& right) {
        if (left.source_node_index != right.source_node_index) {
          return left.source_node_index < right.source_node_index;
        }
        return left.target_node_index < right.target_node_index;
      });
  for (ExactDirectSparseFacetDescentNode& node : canonical_nodes) {
    node.outgoing_edge_index.reset();
  }
  for (std::size_t edge_index = 0U;
       edge_index < result.edges.size();
       ++edge_index) {
    ExactDirectSparseFacetDescentEdge& edge = result.edges[edge_index];
    edge.edge_index = edge_index;
    canonical_nodes[edge.source_node_index].outgoing_edge_index = edge_index;
  }

  for (ExactDirectSparseFacetDescentSeedProjection& seed :
       result.seed_projections) {
    seed.root_node_index = node_remap[seed.root_node_index];
    seed.terminal_node_index = node_remap[seed.terminal_node_index];
  }
  std::sort(
      result.seed_projections.begin(),
      result.seed_projections.end(),
      [](const ExactDirectSparseFacetDescentSeedProjection& left,
         const ExactDirectSparseFacetDescentSeedProjection& right) {
        return left.seed_index < right.seed_index;
      });
  result.nodes = std::move(canonical_nodes);
}

void recompute_graph_counters_and_facts(
    ExactDirectSparseFacetDescentClosureResult& result,
    std::size_t cached_miniball_count) {
  result.counters.interned_node_count = result.nodes.size();
  result.counters.strict_edge_count = result.edges.size();
  result.counters.processed_seed_reference_count =
      result.seed_projections.size();
  result.counters.distinct_cached_miniball_count = cached_miniball_count;
  result.counters.terminal_node_count = 0U;
  result.counters.relative_positive_terminal_count = 0U;
  result.counters.unresolved_terminal_count = 0U;
  result.counters.budget_terminal_count = 0U;
  for (const ExactDirectSparseFacetDescentNode& node : result.nodes) {
    if (node.outgoing_edge_index.has_value()) {
      continue;
    }
    checked_increment_counter(result.counters.terminal_node_count);
    switch (node.closure_disposition) {
      case ExactDirectSparseFacetDescentClosureDisposition::relative_positive:
        checked_increment_counter(
            result.counters.relative_positive_terminal_count);
        break;
      case ExactDirectSparseFacetDescentClosureDisposition::unresolved:
        checked_increment_counter(
            result.counters.unresolved_terminal_count);
        break;
      case ExactDirectSparseFacetDescentClosureDisposition::budget_exhausted:
        checked_increment_counter(result.counters.budget_terminal_count);
        break;
      default:
        break;
    }
  }
  result.all_seed_references_processed =
      result.seed_projections.size() ==
      result.counters.input_seed_reference_count;
  result.every_distinct_evaluated_key_called_step_core_once =
      result.counters.evaluated_step_source_count ==
          result.counters.aggregate_step_counters.source_locator_probe_count &&
      result.counters.evaluated_step_source_count <= result.nodes.size();
  const std::size_t cached_miniball_build_count = checked_size_sum(
      result.counters.source_miniball_build_count,
      result.counters.successor_miniball_build_count);
  result.cached_miniballs_reused_at_exact_seams =
      result.counters.source_miniball_build_count ==
          result.counters.aggregate_step_counters
              .source_miniball_build_count &&
      result.counters.source_miniball_reuse_count ==
          result.counters.aggregate_step_counters
              .source_miniball_reuse_count &&
      result.counters.successor_miniball_build_count ==
          result.counters.aggregate_step_counters
              .successor_miniball_build_count &&
      result.counters.successor_miniball_reuse_count ==
          result.counters.aggregate_step_counters
              .successor_miniball_reuse_count &&
      cached_miniball_build_count ==
          result.counters.distinct_cached_miniball_count;
  const std::size_t graph_cardinality = checked_size_sum(
      result.edges.size(), result.counters.terminal_node_count);
  result.strict_functional_graph_certified =
      graph_cardinality == result.nodes.size() &&
      (result.nodes.empty() || result.edges.size() < result.nodes.size());
  result.exact_level_acyclicity_certified =
      std::all_of(
          result.edges.begin(),
          result.edges.end(),
          [](const ExactDirectSparseFacetDescentEdge& edge) {
            return edge.strict_level_decrease_certified;
          });
  result.edge_node_terminal_identity_certified = graph_shape_well_formed(result);
}

void clear_scientific_graph_on_contradiction(
    ExactDirectSparseFacetDescentClosureResult& result) {
  result.nodes.clear();
  result.edges.clear();
  result.seed_projections.clear();
  result.counters.interned_node_count = 0U;
  result.counters.strict_edge_count = 0U;
  result.counters.processed_seed_reference_count = 0U;
  result.counters.terminal_node_count = 0U;
  result.counters.relative_positive_terminal_count = 0U;
  result.counters.unresolved_terminal_count = 0U;
  result.counters.budget_terminal_count = 0U;
  result.all_seed_references_processed = false;
  result.strict_functional_graph_certified = false;
  result.exact_level_acyclicity_certified = false;
  result.edge_node_terminal_identity_certified = false;
}

}  // namespace

bool ExactDirectSparseFacetDescentClosureResult::
certified_complete_relative_positive_closure() const noexcept {
  return common_closed_scope(*this) && budget_preflight_satisfied &&
         input_shape_certified &&
         disposition ==
             ExactDirectSparseFacetDescentClosureDisposition::
                 relative_positive &&
         (decision == ExactDirectSparseFacetDescentClosureDecision::
                          complete_empty_seed_set ||
          decision == ExactDirectSparseFacetDescentClosureDecision::
                          complete_all_seeds_relative_positive) &&
         all_seed_references_processed &&
         every_distinct_evaluated_key_called_step_core_once &&
         cached_miniballs_reused_at_exact_seams &&
         strict_functional_graph_certified &&
         exact_level_acyclicity_certified &&
         edge_node_terminal_identity_certified &&
         counters.unresolved_terminal_count == 0U &&
         counters.budget_terminal_count == 0U &&
         !contradiction_witness.has_value() &&
         graph_shape_well_formed(*this);
}

bool ExactDirectSparseFacetDescentClosureResult::
certified_complete_with_unresolved_terminals() const noexcept {
  return common_closed_scope(*this) && budget_preflight_satisfied &&
         input_shape_certified &&
         disposition ==
             ExactDirectSparseFacetDescentClosureDisposition::unresolved &&
         decision == ExactDirectSparseFacetDescentClosureDecision::
                         complete_all_seeds_with_unresolved_terminals &&
         all_seed_references_processed &&
         every_distinct_evaluated_key_called_step_core_once &&
         cached_miniballs_reused_at_exact_seams &&
         strict_functional_graph_certified &&
         exact_level_acyclicity_certified &&
         edge_node_terminal_identity_certified &&
         counters.unresolved_terminal_count != 0U &&
         counters.budget_terminal_count == 0U &&
         !contradiction_witness.has_value() &&
         graph_shape_well_formed(*this);
}

bool ExactDirectSparseFacetDescentClosureResult::
certified_budget_exhaustion() const noexcept {
  if (!common_closed_scope(*this) ||
      disposition !=
          ExactDirectSparseFacetDescentClosureDisposition::budget_exhausted ||
      contradiction_witness.has_value()) {
    return false;
  }
  if (decision == ExactDirectSparseFacetDescentClosureDecision::
                      no_closure_preflight_budget_exhausted) {
    return !budget_preflight_satisfied && nodes.empty() && edges.empty() &&
           seed_projections.empty();
  }
  const bool runtime_budget =
      decision == ExactDirectSparseFacetDescentClosureDecision::
                      certified_prefix_node_budget_exhausted ||
      decision == ExactDirectSparseFacetDescentClosureDecision::
                      certified_prefix_step_call_budget_exhausted ||
      decision == ExactDirectSparseFacetDescentClosureDecision::
                      certified_prefix_step_budget_exhausted;
  return runtime_budget && budget_preflight_satisfied &&
         input_shape_certified &&
         every_distinct_evaluated_key_called_step_core_once &&
         cached_miniballs_reused_at_exact_seams &&
         strict_functional_graph_certified &&
         exact_level_acyclicity_certified &&
         edge_node_terminal_identity_certified &&
         counters.budget_terminal_count != 0U &&
         graph_shape_well_formed(*this);
}

bool ExactDirectSparseFacetDescentClosureResult::
certified_fail_closed_contradiction() const noexcept {
  return common_closed_scope(*this) && budget_preflight_satisfied &&
         input_shape_certified &&
         disposition ==
             ExactDirectSparseFacetDescentClosureDisposition::contradiction &&
         (decision == ExactDirectSparseFacetDescentClosureDecision::
                          contradiction_certified_local_step ||
          decision == ExactDirectSparseFacetDescentClosureDecision::
                          contradiction_cycle_or_incompatible_shared_target) &&
         contradiction_witness.has_value() && nodes.empty() && edges.empty() &&
         seed_projections.empty();
}

bool ExactDirectSparseFacetDescentClosureResult::
certified_partial_refinement_outcome() const noexcept {
  return certified_complete_relative_positive_closure() ||
         certified_complete_with_unresolved_terminals() ||
         certified_budget_exhaustion() ||
         certified_fail_closed_contradiction();
}

namespace {

struct CanonicalClosureSeedInput {
  std::span<const ExactDirectSparseFacetKey> distinct_keys;
  std::span<const ExactDirectSparseFacetDescentClosureSeed>
      ordered_seed_records;
  bool implicit_distinct_seed_identities{false};

  [[nodiscard]] std::size_t seed_count() const noexcept {
    return implicit_distinct_seed_identities ? distinct_keys.size()
                                             : ordered_seed_records.size();
  }

  [[nodiscard]] std::size_t seed_index(std::size_t index) const noexcept {
    return implicit_distinct_seed_identities
               ? index
               : ordered_seed_records[index].seed_index;
  }

  [[nodiscard]] const ExactDirectSparseFacetKey& seed_key(
      std::size_t index) const noexcept {
    return implicit_distinct_seed_identities
               ? distinct_keys[index]
               : ordered_seed_records[index].source_facet_key;
  }
};

void finish_closure_snapshot(
    const ExactDirectSparsePositiveFacetLocator& locator,
    ExactDirectSparseFacetDescentClosureResult& result) {
  checked_increment_counter(result.counters.locator_snapshot_check_count);
  if (locator.snapshot_stamp() != result.locator_snapshot_stamp) {
    throw std::runtime_error(
        "the positive-facet locator changed before closure publication");
  }
  result.common_locator_snapshot_certified = true;
  result.every_memo_fingerprint_candidate_compared_by_full_key = true;
  result.no_half_edge_published = true;
  result.no_top_k_partition_or_shell_persisted = true;
}

void finish_closure_preflight_budget_exhaustion(
    const ExactDirectSparsePositiveFacetLocator& locator,
    ExactDirectSparseFacetDescentClosureResult& result) {
  result.disposition =
      ExactDirectSparseFacetDescentClosureDisposition::budget_exhausted;
  result.decision = ExactDirectSparseFacetDescentClosureDecision::
      no_closure_preflight_budget_exhausted;
  result.every_distinct_evaluated_key_called_step_core_once = true;
  result.cached_miniballs_reused_at_exact_seams = true;
  result.strict_functional_graph_certified = true;
  result.exact_level_acyclicity_certified = true;
  result.edge_node_terminal_identity_certified = true;
  finish_closure_snapshot(locator, result);
}

[[nodiscard]] ExactDirectSparseFacetDescentClosureResult
initialize_closure_build_result(
    std::size_t input_seed_reference_count,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentClosureBudget& budget,
    const ExactDirectSparseFacetDescentClosureConfig& config,
    spatial::LbvhTraversalOrder traversal_order) {
  ExactDirectSparseFacetDescentClosureResult result;
  result.config = config;
  result.requested_budget = budget;
  result.traversal_order = traversal_order;
  result.closed_batch_squared_level = closed_batch_squared_level;
  result.locator_query_witness = locator_query_witness;
  result.locator_snapshot_stamp = locator.snapshot_stamp();
  result.counters.locator_snapshot_check_count = 1U;
  result.counters.input_seed_reference_count = input_seed_reference_count;
  result.required_memo_slot_count =
      required_memo_slot_count(budget.maximum_node_count);
  result.trusted_authorities_certified = true;
  result.budget_preflight_completed = true;
  initialize_closed_scope(result);
  return result;
}

[[nodiscard]] ExactDirectSparseFacetDescentClosureResult
build_closure_from_canonical_seed_input(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const CanonicalClosureSeedInput& input,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentClosureBudget& budget,
    const ExactDirectSparseFacetDescentClosureConfig& config,
    spatial::LbvhTraversalOrder traversal_order,
    ExactDirectSparseFacetDescentClosureResult result) {
  result.counters.distinct_seed_key_count = input.distinct_keys.size();
  result.counters.duplicate_seed_key_reference_count =
      input.seed_count() - input.distinct_keys.size();

  if (input.distinct_keys.size() > budget.maximum_node_count) {
    finish_closure_preflight_budget_exhaustion(locator, result);
    return result;
  }
  result.budget_preflight_satisfied = true;

  if (input.seed_count() == 0U) {
    result.all_seed_references_processed = true;
    result.every_distinct_evaluated_key_called_step_core_once = true;
    result.cached_miniballs_reused_at_exact_seams = true;
    result.strict_functional_graph_certified = true;
    result.exact_level_acyclicity_certified = true;
    result.edge_node_terminal_identity_certified = true;
    result.disposition =
        ExactDirectSparseFacetDescentClosureDisposition::relative_positive;
    result.decision = ExactDirectSparseFacetDescentClosureDecision::
        complete_empty_seed_set;
    finish_closure_snapshot(locator, result);
    return result;
  }

  ClosureBuilder builder(
      index,
      cloud,
      closed_batch_squared_level,
      locator_query_witness,
      locator,
      budget,
      config,
      traversal_order,
      result);
  builder.intern_seed_roots(input.distinct_keys);

  PathBuildStatus global_status = PathBuildStatus::complete;
  for (const ExactDirectSparseFacetKey& key : input.distinct_keys) {
    global_status = builder.process_root(key);
    if (global_status != PathBuildStatus::complete) {
      break;
    }
  }

  if (global_status == PathBuildStatus::contradiction) {
    const bool local_step_contradiction =
        result.contradiction_witness.has_value() &&
        result.contradiction_witness->local_step_decision !=
            ExactDirectSparseFacetDescentStepDecision::not_certified &&
        !result.contradiction_witness->visiting_cycle_detected &&
        !result.contradiction_witness->shared_target_geometry_mismatch;
    clear_scientific_graph_on_contradiction(result);
    result.disposition =
        ExactDirectSparseFacetDescentClosureDisposition::contradiction;
    result.decision = local_step_contradiction
                          ? ExactDirectSparseFacetDescentClosureDecision::
                                contradiction_certified_local_step
                          : ExactDirectSparseFacetDescentClosureDecision::
                                contradiction_cycle_or_incompatible_shared_target;
    finish_closure_snapshot(locator, result);
    return result;
  }
  if (global_status != PathBuildStatus::complete) {
    builder.close_unseen_seed_roots_as_budget_terminals();
  }

  std::vector<std::size_t> reference_count_per_node(result.nodes.size(), 0U);
  for (std::size_t input_index = 0U;
       input_index < input.seed_count();
       ++input_index) {
    const ExactDirectSparseFacetKey& seed_key = input.seed_key(input_index);
    const MemoSearchResult root = builder.find_node(seed_key);
    if (!root.found ||
        builder.visit_states()[root.node_index] != MemoVisitState::closed) {
      continue;
    }
    const bool reused = builder.node_created_as_successor(root.node_index) ||
                        reference_count_per_node[root.node_index] != 0U;
    if (reused) {
      checked_increment_counter(result.counters.memoized_seed_reuse_count);
    }
    checked_increment_counter(reference_count_per_node[root.node_index]);
    const ExactDirectSparseFacetDescentNode& root_node =
        result.nodes[root.node_index];
    result.seed_projections.push_back(
        {input.seed_index(input_index),
         seed_key,
         root.node_index,
         root_node.terminal_node_index,
         root_node.closure_disposition,
         reused});
  }

  canonicalize_graph(result);
  recompute_graph_counters_and_facts(
      result, builder.cached_miniball_count());

  switch (global_status) {
    case PathBuildStatus::node_budget_exhausted:
      result.disposition =
          ExactDirectSparseFacetDescentClosureDisposition::budget_exhausted;
      result.decision = ExactDirectSparseFacetDescentClosureDecision::
          certified_prefix_node_budget_exhausted;
      break;
    case PathBuildStatus::step_call_budget_exhausted:
      result.disposition =
          ExactDirectSparseFacetDescentClosureDisposition::budget_exhausted;
      result.decision = ExactDirectSparseFacetDescentClosureDecision::
          certified_prefix_step_call_budget_exhausted;
      break;
    case PathBuildStatus::step_budget_exhausted:
      result.disposition =
          ExactDirectSparseFacetDescentClosureDisposition::budget_exhausted;
      result.decision = ExactDirectSparseFacetDescentClosureDecision::
          certified_prefix_step_budget_exhausted;
      break;
    case PathBuildStatus::complete:
      if (result.counters.unresolved_terminal_count == 0U) {
        result.disposition =
            ExactDirectSparseFacetDescentClosureDisposition::relative_positive;
        result.decision = ExactDirectSparseFacetDescentClosureDecision::
            complete_all_seeds_relative_positive;
      } else {
        result.disposition =
            ExactDirectSparseFacetDescentClosureDisposition::unresolved;
        result.decision = ExactDirectSparseFacetDescentClosureDecision::
            complete_all_seeds_with_unresolved_terminals;
      }
      break;
    case PathBuildStatus::contradiction:
      throw std::logic_error("a contradiction escaped its fail-closed path");
  }

  finish_closure_snapshot(locator, result);
  return result;
}

void require_closure_build_authorities(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentClosureBudget& budget,
    spatial::LbvhTraversalOrder traversal_order) {
  require_valid_traversal_order(traversal_order);
  require_budget_within_confidence(budget);
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "a direct sparse descent closure requires a matching LBVH authority");
  }
  if (!locator.certified_positive_locator()) {
    throw std::invalid_argument(
        "a direct sparse descent closure requires a certified locator");
  }
  if (locator_query_witness.external_authority_id == 0U ||
      locator_query_witness.external_authority_id !=
          locator.config().external_authority_id ||
      locator_query_witness.replay_token == 0U) {
    throw std::invalid_argument(
        "a direct sparse descent closure requires one matching locator witness");
  }
}

}  // namespace

ExactDirectSparseFacetDescentClosureResult
build_exact_direct_sparse_facet_descent_closure(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const ExactDirectSparseFacetDescentClosureSeed> seeds,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentClosureBudget& budget,
    const ExactDirectSparseFacetDescentClosureConfig& config,
    spatial::LbvhTraversalOrder traversal_order) {
  require_closure_build_authorities(
      index,
      cloud,
      locator_query_witness,
      locator,
      budget,
      traversal_order);
  ExactDirectSparseFacetDescentClosureResult result =
      initialize_closure_build_result(
          seeds.size(),
          closed_batch_squared_level,
          locator_query_witness,
          locator,
          budget,
          config,
          traversal_order);

  if (seeds.size() > budget.maximum_seed_count ||
      result.required_memo_slot_count > budget.maximum_memo_slot_count) {
    finish_closure_preflight_budget_exhaustion(locator, result);
    return result;
  }
  std::vector<ExactDirectSparseFacetDescentClosureSeed> ordered_seeds(
      seeds.begin(), seeds.end());
  std::sort(
      ordered_seeds.begin(),
      ordered_seeds.end(),
      [](const ExactDirectSparseFacetDescentClosureSeed& left,
         const ExactDirectSparseFacetDescentClosureSeed& right) {
        return left.seed_index < right.seed_index;
      });
  for (std::size_t index_in_order = 0U;
       index_in_order < ordered_seeds.size();
       ++index_in_order) {
    if (ordered_seeds[index_in_order].seed_index != index_in_order ||
        !valid_facet_key(
            cloud, ordered_seeds[index_in_order].source_facet_key)) {
      throw std::invalid_argument(
          "closure seeds require contiguous identities and canonical full keys");
    }
    if (index_in_order == 0U) {
      result.common_facet_cardinality =
          ordered_seeds[index_in_order].source_facet_key.point_count;
    } else if (ordered_seeds[index_in_order].source_facet_key.point_count !=
               result.common_facet_cardinality) {
      throw std::invalid_argument(
          "all direct sparse descent-closure seeds need one fixed order");
    }
  }
  result.input_shape_certified = true;

  std::vector<ExactDirectSparseFacetKey> distinct_keys;
  distinct_keys.reserve(ordered_seeds.size());
  for (const ExactDirectSparseFacetDescentClosureSeed& seed : ordered_seeds) {
    distinct_keys.push_back(seed.source_facet_key);
  }
  std::sort(distinct_keys.begin(), distinct_keys.end(), facet_key_less);
  distinct_keys.erase(
      std::unique(distinct_keys.begin(), distinct_keys.end()),
      distinct_keys.end());
  return build_closure_from_canonical_seed_input(
      index,
      cloud,
      CanonicalClosureSeedInput{
          std::span<const ExactDirectSparseFacetKey>{distinct_keys},
          std::span<const ExactDirectSparseFacetDescentClosureSeed>{
              ordered_seeds},
          false},
      closed_batch_squared_level,
      locator_query_witness,
      locator,
      budget,
      config,
      traversal_order,
      std::move(result));
}

ExactDirectSparseFacetDescentClosureResult
build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const ExactDirectSparseFacetKey> canonical_distinct_keys,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentClosureBudget& budget,
    const ExactDirectSparseFacetDescentClosureConfig& config,
    spatial::LbvhTraversalOrder traversal_order) {
  require_closure_build_authorities(
      index,
      cloud,
      locator_query_witness,
      locator,
      budget,
      traversal_order);
  ExactDirectSparseFacetDescentClosureResult result =
      initialize_closure_build_result(
          canonical_distinct_keys.size(),
          closed_batch_squared_level,
          locator_query_witness,
          locator,
          budget,
          config,
          traversal_order);

  if (canonical_distinct_keys.size() > budget.maximum_seed_count ||
      result.required_memo_slot_count > budget.maximum_memo_slot_count) {
    finish_closure_preflight_budget_exhaustion(locator, result);
    return result;
  }
  for (std::size_t key_index = 0U;
       key_index < canonical_distinct_keys.size();
       ++key_index) {
    const ExactDirectSparseFacetKey& key =
        canonical_distinct_keys[key_index];
    if (!valid_facet_key(cloud, key)) {
      throw std::invalid_argument(
          "canonical distinct closure keys require valid complete facet keys");
    }
    if (key_index == 0U) {
      result.common_facet_cardinality = key.point_count;
      continue;
    }
    if (key.point_count != result.common_facet_cardinality) {
      throw std::invalid_argument(
          "all canonical distinct closure keys need one fixed order");
    }
    if (!facet_key_less(canonical_distinct_keys[key_index - 1U], key)) {
      throw std::invalid_argument(
          "canonical distinct closure keys must be strictly increasing");
    }
  }
  result.input_shape_certified = true;

  return build_closure_from_canonical_seed_input(
      index,
      cloud,
      CanonicalClosureSeedInput{
          canonical_distinct_keys,
          std::span<const ExactDirectSparseFacetDescentClosureSeed>{},
          true},
      closed_batch_squared_level,
      locator_query_witness,
      locator,
      budget,
      config,
      traversal_order,
      std::move(result));
}

ExactDirectSparseFacetDescentClosureVerification
verify_exact_direct_sparse_facet_descent_closure(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const ExactDirectSparseFacetDescentClosureSeed> seeds,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentClosureBudget& budget,
    const ExactDirectSparseFacetDescentClosureConfig& config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseFacetDescentClosureResult& observed) {
  ExactDirectSparseFacetDescentClosureVerification verification;
  verification.trusted_inputs_certified =
      index.validated_for(cloud) && locator.certified_positive_locator() &&
      locator_query_witness.external_authority_id != 0U &&
      locator_query_witness.external_authority_id ==
          locator.config().external_authority_id &&
      locator_query_witness.replay_token != 0U;
  if (!verification.trusted_inputs_certified) {
    return verification;
  }
  verification.observed_storage_within_budget =
      observed.nodes.size() <= budget.maximum_node_count &&
      observed.edges.size() <= budget.maximum_node_count &&
      observed.seed_projections.size() <= budget.maximum_seed_count;
  if (!verification.observed_storage_within_budget) {
    return verification;
  }
  verification.locator_snapshot_matches_observed_build =
      locator.snapshot_stamp() == observed.locator_snapshot_stamp;
  if (!verification.locator_snapshot_matches_observed_build) {
    return verification;
  }

  const ExactDirectSparseFacetDescentClosureResult expected =
      build_exact_direct_sparse_facet_descent_closure(
          index,
          cloud,
          seeds,
          closed_batch_squared_level,
          locator_query_witness,
          locator,
          budget,
          config,
          traversal_order);
  const bool contradiction_freshly_replayed =
      observed.certified_fail_closed_contradiction() &&
      expected.certified_fail_closed_contradiction();
  verification.observed_outcome_well_formed =
      observed.certified_partial_refinement_outcome();
  verification.seeds_freshly_canonicalized =
      observed.common_facet_cardinality == expected.common_facet_cardinality &&
      observed.counters.input_seed_reference_count ==
          expected.counters.input_seed_reference_count &&
      observed.counters.distinct_seed_key_count ==
          expected.counters.distinct_seed_key_count &&
      observed.counters.duplicate_seed_key_reference_count ==
          expected.counters.duplicate_seed_key_reference_count;
  verification.memoized_graph_freshly_replayed =
      observed.nodes == expected.nodes && observed.edges == expected.edges &&
      observed.seed_projections == expected.seed_projections;
  verification.local_step_projections_freshly_replayed =
      observed.counters.aggregate_step_counters ==
          expected.counters.aggregate_step_counters &&
      observed.counters.evaluated_step_source_count ==
          expected.counters.evaluated_step_source_count;
  verification.strict_edges_and_seams_freshly_replayed =
      observed.edges == expected.edges &&
      (contradiction_freshly_replayed ||
       (observed.exact_level_acyclicity_certified &&
        expected.exact_level_acyclicity_certified));
  verification.functional_forest_cardinality_certified =
      graph_shape_well_formed(observed) ==
          graph_shape_well_formed(expected) &&
      (observed.certified_fail_closed_contradiction() ||
       graph_shape_well_formed(observed));
  verification.terminal_dispositions_freshly_propagated =
      observed.seed_projections == expected.seed_projections &&
      observed.counters.relative_positive_terminal_count ==
          expected.counters.relative_positive_terminal_count &&
      observed.counters.unresolved_terminal_count ==
          expected.counters.unresolved_terminal_count &&
      observed.counters.budget_terminal_count ==
          expected.counters.budget_terminal_count;
  verification.no_duplicate_top_k_or_miniball_work_certified =
      (contradiction_freshly_replayed ||
       (observed.cached_miniballs_reused_at_exact_seams &&
        expected.cached_miniballs_reused_at_exact_seams)) &&
      observed.counters.source_miniball_build_count ==
          expected.counters.source_miniball_build_count &&
      observed.counters.source_miniball_reuse_count ==
          expected.counters.source_miniball_reuse_count &&
      observed.counters.successor_miniball_build_count ==
          expected.counters.successor_miniball_build_count &&
      observed.counters.successor_miniball_reuse_count ==
          expected.counters.successor_miniball_reuse_count;
  verification.no_locator_mutation_or_batch_commit =
      !observed.locator_state_mutated && !observed.locator_batch_committed &&
      !expected.locator_state_mutated && !expected.locator_batch_committed;
  verification.external_binding_authority_replayed = false;
  verification.no_isolation_singleton_or_attachment_invented =
      !observed.missing_facet_means_isolated &&
      !observed.singleton_component_created &&
      !observed.hierarchy_attachment_published &&
      !expected.missing_facet_means_isolated &&
      !expected.singleton_component_created &&
      !expected.hierarchy_attachment_published;
  verification.no_forbidden_global_structure_materialized =
      !observed.forbidden_global_structure_materialized &&
      !expected.forbidden_global_structure_materialized &&
      observed.no_top_k_partition_or_shell_persisted &&
      expected.no_top_k_partition_or_shell_persisted;
  verification.fresh_replay_certified = observed == expected;
  verification.result_certified =
      verification.observed_storage_within_budget &&
      verification.observed_outcome_well_formed &&
      verification.seeds_freshly_canonicalized &&
      verification.memoized_graph_freshly_replayed &&
      verification.local_step_projections_freshly_replayed &&
      verification.strict_edges_and_seams_freshly_replayed &&
      verification.functional_forest_cardinality_certified &&
      verification.terminal_dispositions_freshly_propagated &&
      verification.no_duplicate_top_k_or_miniball_work_certified &&
      verification.no_locator_mutation_or_batch_commit &&
      !verification.external_binding_authority_replayed &&
      verification.no_isolation_singleton_or_attachment_invented &&
      verification.no_forbidden_global_structure_materialized &&
      verification.fresh_replay_certified &&
      expected.certified_partial_refinement_outcome();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
