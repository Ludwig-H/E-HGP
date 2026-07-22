#include "morsehgp3d/spatial/lbvh.hpp"

#include "exact_query.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::spatial {
namespace {

struct BudgetedNodeQueueEntry {
  exact::ExactLevel lower_bound;
  std::size_t node_index;
};

struct BudgetedNodeQueueCompare {
  LbvhTraversalOrder order;

  [[nodiscard]] bool operator()(
      const BudgetedNodeQueueEntry& left,
      const BudgetedNodeQueueEntry& right) const {
    if (left.lower_bound != right.lower_bound) {
      if (order == LbvhTraversalOrder::near_first) {
        return left.lower_bound > right.lower_bound;
      }
      return left.lower_bound < right.lower_bound;
    }
    if (order == LbvhTraversalOrder::near_first) {
      return left.node_index > right.node_index;
    }
    return left.node_index < right.node_index;
  }
};

struct BudgetedWorstNeighborFirst {
  [[nodiscard]] bool operator()(
      const ExactNeighbor& left,
      const ExactNeighbor& right) const {
    return detail::exact_neighbor_less(left, right);
  }
};

void require_budgeted_traversal_order(
    LbvhTraversalOrder traversal_order) {
  switch (traversal_order) {
    case LbvhTraversalOrder::near_first:
    case LbvhTraversalOrder::far_first:
      return;
  }
  throw std::invalid_argument("an LBVH traversal order is invalid");
}

class ProvisionalCutoffShell {
 public:
  ProvisionalCutoffShell(
      std::size_t capacity,
      std::size_t maximum_possible_size,
      ExactLbvhTopKAudit& audit)
      : capacity_(capacity), audit_(audit) {
    retained_ids_.reserve(std::min(capacity, maximum_possible_size));
  }

  void rebuild(
      const std::vector<ExactNeighbor>& best_neighbors,
      const exact::ExactLevel& cutoff) {
    retained_ids_.clear();
    overflowed_ = false;
    for (const ExactNeighbor& neighbor : best_neighbors) {
      if (neighbor.squared_distance == cutoff) {
        observe(neighbor.point_id);
      }
    }
  }

  void observe(PointId point_id) {
    if (retained_ids_.size() < capacity_) {
      retained_ids_.push_back(point_id);
      audit_.peak_retained_cutoff_shell_entry_count = std::max(
          audit_.peak_retained_cutoff_shell_entry_count,
          retained_ids_.size());
      return;
    }
    if (!overflowed_) {
      overflowed_ = true;
      ++audit_.provisional_cutoff_shell_overflow_count;
    }
  }

  [[nodiscard]] bool overflowed() const noexcept {
    return overflowed_;
  }

  [[nodiscard]] std::vector<PointId> take_complete_ids() {
    if (overflowed_) {
      throw std::logic_error(
          "an overflowing provisional shell is not a scientific result");
    }
    return std::move(retained_ids_);
  }

 private:
  std::size_t capacity_;
  ExactLbvhTopKAudit& audit_;
  std::vector<PointId> retained_ids_;
  bool overflowed_{false};
};

}  // namespace

ExactBudgetedLbvhTopKResult::ExactBudgetedLbvhTopKResult(
    ExactLbvhTopKStatus status,
    ExactLbvhTopKStopReason stop_reason,
    ExactLbvhTopKBudget requested_budget,
    ExactLbvhTopKAudit audit,
    std::optional<TopKPartition> partition)
    : status_(status),
      stop_reason_(stop_reason),
      requested_budget_(requested_budget),
      audit_(audit),
      partition_(std::move(partition)) {
  if (audit_.node_visit_count >
          requested_budget_.maximum_node_visit_count ||
      audit_.internal_node_expansion_count >
          requested_budget_.maximum_internal_node_expansion_count ||
      audit_.exact_aabb_bound_evaluation_count >
          requested_budget_.maximum_exact_aabb_bound_evaluation_count ||
      audit_.exact_point_distance_evaluation_count >
          requested_budget_.maximum_exact_point_distance_evaluation_count ||
      audit_.peak_frontier_entry_count >
          requested_budget_.maximum_frontier_entry_count ||
      audit_.peak_best_neighbor_entry_count >
          requested_budget_.maximum_best_neighbor_entry_count ||
      audit_.peak_retained_cutoff_shell_entry_count >
          requested_budget_.maximum_cutoff_shell_entry_count) {
    throw std::logic_error("a budgeted LBVH top-k audit exceeds its budget");
  }
  switch (status_) {
    case ExactLbvhTopKStatus::not_certified:
      throw std::logic_error(
          "a constructed budgeted LBVH top-k result is not certified");
    case ExactLbvhTopKStatus::complete:
      if (stop_reason_ != ExactLbvhTopKStopReason::none ||
          !audit_.traversal_complete || !partition_.has_value() ||
          !partition_->shell_complete()) {
        throw std::logic_error(
            "a complete budgeted LBVH top-k result lacks its exact partition");
      }
      {
        const SpatialQueryCounters& counters =
            partition_->query_counters();
        if (audit_.node_visit_count != counters.node_visit_count ||
            audit_.internal_node_expansion_count !=
                counters.internal_node_expansion_count ||
            audit_.exact_aabb_bound_evaluation_count !=
                counters.exact_aabb_bound_evaluation_count ||
            audit_.exact_point_distance_evaluation_count !=
                counters.exact_point_distance_evaluation_count ||
            audit_.peak_best_neighbor_entry_count !=
                partition_->requested_rank()) {
          throw std::logic_error(
              "a complete budgeted LBVH top-k audit disagrees with its partition");
        }
      }
      return;
    case ExactLbvhTopKStatus::budget_exhausted:
      if (stop_reason_ == ExactLbvhTopKStopReason::none ||
          partition_.has_value()) {
        throw std::logic_error(
            "an exhausted budgeted LBVH top-k result publishes a partition");
      }
      if ((stop_reason_ ==
               ExactLbvhTopKStopReason::cutoff_shell_entry_limit) !=
          audit_.traversal_complete) {
        throw std::logic_error(
            "budgeted LBVH top-k exhaustion has inconsistent traversal state");
      }
      return;
  }
  throw std::logic_error("a budgeted LBVH top-k status is invalid");
}

ExactBudgetedLbvhTopKResult::ExactBudgetedLbvhTopKResult(
    ExactBudgetedLbvhTopKResult&& other) noexcept
    : status_(std::exchange(
          other.status_, ExactLbvhTopKStatus::not_certified)),
      stop_reason_(std::exchange(
          other.stop_reason_, ExactLbvhTopKStopReason::none)),
      requested_budget_(std::exchange(
          other.requested_budget_, ExactLbvhTopKBudget{})),
      audit_(std::exchange(other.audit_, ExactLbvhTopKAudit{})),
      partition_(std::move(other.partition_)) {
  other.partition_.reset();
}

ExactBudgetedLbvhTopKResult& ExactBudgetedLbvhTopKResult::operator=(
    ExactBudgetedLbvhTopKResult&& other) noexcept {
  if (this != &other) {
    status_ = std::exchange(
        other.status_, ExactLbvhTopKStatus::not_certified);
    stop_reason_ = std::exchange(
        other.stop_reason_, ExactLbvhTopKStopReason::none);
    requested_budget_ = std::exchange(
        other.requested_budget_, ExactLbvhTopKBudget{});
    audit_ = std::exchange(other.audit_, ExactLbvhTopKAudit{});
    partition_ = std::move(other.partition_);
    other.partition_.reset();
  }
  return *this;
}

const TopKPartition& ExactBudgetedLbvhTopKResult::partition() const & {
  if (!complete() || !partition_.has_value()) {
    throw std::logic_error(
        "an exhausted budgeted LBVH top-k result has no scientific partition");
  }
  return *partition_;
}

ExactBudgetedLbvhTopKResult lbvh_top_k_budgeted(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    std::size_t requested_rank,
    const ExclusionSet& exclusions,
    ExactLbvhTopKBudget budget,
    LbvhTraversalOrder traversal_order) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the Morton LBVH belongs to a different canonical point namespace");
  }
  if (!exclusions.validated_for(cloud)) {
    throw std::invalid_argument(
        "the exclusion set belongs to a different canonical point namespace");
  }
  require_budgeted_traversal_order(traversal_order);
  const std::size_t eligible_point_count =
      cloud.size() - exclusions.ids().size();
  if (requested_rank == 0U || requested_rank > eligible_point_count) {
    throw std::out_of_range(
        "the requested rank is outside the eligible point set");
  }
  const exact::ExactRational3 canonical_query =
      detail::validated_query(query);

  ExactLbvhTopKAudit audit;
  const auto exhausted = [&budget, &audit](
                             ExactLbvhTopKStopReason stop_reason) {
    return ExactBudgetedLbvhTopKResult{
        ExactLbvhTopKStatus::budget_exhausted,
        stop_reason,
        budget,
        audit,
        std::nullopt};
  };

  // Check every allocation needed to seed the query before performing it.
  if (requested_rank > budget.maximum_best_neighbor_entry_count) {
    return exhausted(ExactLbvhTopKStopReason::best_neighbor_entry_limit);
  }
  if (budget.maximum_frontier_entry_count == 0U) {
    return exhausted(ExactLbvhTopKStopReason::frontier_entry_limit);
  }
  if (budget.maximum_exact_aabb_bound_evaluation_count == 0U) {
    return exhausted(
        ExactLbvhTopKStopReason::exact_aabb_bound_evaluation_limit);
  }

  std::vector<BudgetedNodeQueueEntry> frontier_storage;
  frontier_storage.reserve(std::min(
      budget.maximum_frontier_entry_count, index.nodes_.size()));
  std::priority_queue<
      BudgetedNodeQueueEntry,
      std::vector<BudgetedNodeQueueEntry>,
      BudgetedNodeQueueCompare>
      nodes_to_visit{
          BudgetedNodeQueueCompare{traversal_order},
          std::move(frontier_storage)};
  std::vector<ExactNeighbor> best_neighbors;
  best_neighbors.reserve(requested_rank);
  ProvisionalCutoffShell cutoff_shell{
      budget.maximum_cutoff_shell_entry_count,
      eligible_point_count,
      audit};

  SpatialQueryCounters counters;
  counters.method = SpatialQueryMethod::morton_lbvh;
  counters.excluded_point_count = exclusions.ids().size();

  exact::ExactLevel root_bound = index.minimum_squared_distance_to_node(
      cloud, index.root_index_, canonical_query);
  ++counters.exact_aabb_bound_evaluation_count;
  ++audit.exact_aabb_bound_evaluation_count;
  nodes_to_visit.push(
      BudgetedNodeQueueEntry{std::move(root_bound), index.root_index_});
  audit.peak_frontier_entry_count = 1U;

  const BudgetedWorstNeighborFirst best_compare;
  while (!nodes_to_visit.empty()) {
    if (audit.node_visit_count >= budget.maximum_node_visit_count) {
      return exhausted(ExactLbvhTopKStopReason::node_visit_limit);
    }
    BudgetedNodeQueueEntry entry = nodes_to_visit.top();
    nodes_to_visit.pop();
    ++counters.node_visit_count;
    ++audit.node_visit_count;

    if (best_neighbors.size() == requested_rank &&
        entry.lower_bound > best_neighbors.front().squared_distance) {
      detail::record_strict_pruning_margin(
          counters,
          entry.lower_bound,
          best_neighbors.front().squared_distance);
      ++counters.pruned_subtree_count;
      counters.pruned_eligible_point_count +=
          index.eligible_count_in_node(
              index.nodes_[entry.node_index], exclusions);
      continue;
    }

    const MortonLbvhIndex::Node& node = index.nodes_[entry.node_index];
    if (node.is_leaf()) {
      const PointId point_id = index.leaves_[node.leaf_begin].point_id;
      if (exclusions.contains(point_id)) {
        continue;
      }
      if (audit.exact_point_distance_evaluation_count >=
          budget.maximum_exact_point_distance_evaluation_count) {
        return exhausted(
            ExactLbvhTopKStopReason::exact_point_distance_evaluation_limit);
      }
      ExactNeighbor neighbor{
          point_id,
          detail::exact_squared_distance(
              canonical_query, cloud.point(point_id))};
      ++counters.exact_point_distance_evaluation_count;
      ++audit.exact_point_distance_evaluation_count;

      if (best_neighbors.size() < requested_rank) {
        best_neighbors.push_back(std::move(neighbor));
        audit.peak_best_neighbor_entry_count = std::max(
            audit.peak_best_neighbor_entry_count,
            best_neighbors.size());
        std::push_heap(
            best_neighbors.begin(), best_neighbors.end(), best_compare);
        if (best_neighbors.size() == requested_rank) {
          cutoff_shell.rebuild(
              best_neighbors, best_neighbors.front().squared_distance);
        }
        continue;
      }

      const exact::ExactLevel previous_cutoff =
          best_neighbors.front().squared_distance;
      const bool neighbor_is_on_previous_cutoff =
          neighbor.squared_distance == previous_cutoff;
      if (detail::exact_neighbor_less(neighbor, best_neighbors.front())) {
        std::pop_heap(
            best_neighbors.begin(), best_neighbors.end(), best_compare);
        best_neighbors.pop_back();
        best_neighbors.push_back(std::move(neighbor));
        std::push_heap(
            best_neighbors.begin(), best_neighbors.end(), best_compare);
      }
      if (best_neighbors.front().squared_distance < previous_cutoff) {
        ++audit.provisional_cutoff_decrease_count;
        cutoff_shell.rebuild(
            best_neighbors, best_neighbors.front().squared_distance);
      } else if (neighbor_is_on_previous_cutoff) {
        cutoff_shell.observe(point_id);
      }
      continue;
    }

    if (audit.internal_node_expansion_count >=
        budget.maximum_internal_node_expansion_count) {
      return exhausted(
          ExactLbvhTopKStopReason::internal_node_expansion_limit);
    }
    const std::size_t frontier_size = nodes_to_visit.size();
    if (frontier_size > budget.maximum_frontier_entry_count ||
        budget.maximum_frontier_entry_count - frontier_size < 2U) {
      return exhausted(ExactLbvhTopKStopReason::frontier_entry_limit);
    }
    if (audit.exact_aabb_bound_evaluation_count >
            budget.maximum_exact_aabb_bound_evaluation_count ||
        budget.maximum_exact_aabb_bound_evaluation_count -
                audit.exact_aabb_bound_evaluation_count <
            2U) {
      return exhausted(
          ExactLbvhTopKStopReason::exact_aabb_bound_evaluation_limit);
    }
    ++counters.internal_node_expansion_count;
    ++audit.internal_node_expansion_count;
    for (const std::size_t child : {node.left_child, node.right_child}) {
      exact::ExactLevel child_bound = index.minimum_squared_distance_to_node(
          cloud, child, canonical_query);
      ++counters.exact_aabb_bound_evaluation_count;
      ++audit.exact_aabb_bound_evaluation_count;
      nodes_to_visit.push(
          BudgetedNodeQueueEntry{std::move(child_bound), child});
      audit.peak_frontier_entry_count = std::max(
          audit.peak_frontier_entry_count, nodes_to_visit.size());
    }
  }

  audit.traversal_complete = true;
  if (cutoff_shell.overflowed()) {
    return exhausted(
        ExactLbvhTopKStopReason::cutoff_shell_entry_limit);
  }
  if (best_neighbors.size() != requested_rank ||
      counters.exact_point_distance_evaluation_count +
              counters.pruned_eligible_point_count !=
          eligible_point_count) {
    throw std::logic_error(
        "a complete budgeted LBVH traversal does not close on eligible leaves");
  }

  const exact::ExactLevel cutoff =
      best_neighbors.front().squared_distance;
  std::vector<ExactNeighbor> strict_below;
  strict_below.reserve(requested_rank - 1U);
  for (ExactNeighbor& neighbor : best_neighbors) {
    if (neighbor.squared_distance < cutoff) {
      strict_below.push_back(std::move(neighbor));
    }
  }
  std::sort(
      strict_below.begin(), strict_below.end(), detail::exact_neighbor_less);
  std::vector<PointId> cutoff_shell_ids =
      cutoff_shell.take_complete_ids();
  std::sort(cutoff_shell_ids.begin(), cutoff_shell_ids.end());

  std::vector<PointId> canonical_choice_ids;
  canonical_choice_ids.reserve(requested_rank);
  for (const ExactNeighbor& neighbor : strict_below) {
    canonical_choice_ids.push_back(neighbor.point_id);
  }
  const std::size_t shell_choice_count =
      requested_rank - strict_below.size();
  if (cutoff_shell_ids.size() < shell_choice_count) {
    throw std::logic_error(
        "a complete budgeted LBVH shell does not straddle the requested rank");
  }
  for (std::size_t index = 0U; index < shell_choice_count; ++index) {
    canonical_choice_ids.push_back(cutoff_shell_ids[index]);
  }
  std::sort(canonical_choice_ids.begin(), canonical_choice_ids.end());

  TopKPartition partition{
      cloud,
      requested_rank,
      cutoff,
      std::move(strict_below),
      std::move(cutoff_shell_ids),
      std::move(canonical_choice_ids),
      eligible_point_count,
      std::move(counters)};
  return ExactBudgetedLbvhTopKResult{
      ExactLbvhTopKStatus::complete,
      ExactLbvhTopKStopReason::none,
      budget,
      audit,
      std::optional<TopKPartition>{std::move(partition)}};
}

}  // namespace morsehgp3d::spatial
