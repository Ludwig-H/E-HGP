#pragma once

#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace morsehgp3d::hierarchy {

using K1NodeId = std::uint64_t;

enum class K1CutClosure {
  strict,
  closed,
};

enum class K1CutEdgeSource {
  complete_graph,
  selected_emst,
};

struct ExactEmstEdge {
  spatial::PointId u{};
  spatial::PointId v{};
  exact::ExactLevel squared_length{};
  exact::ExactLevel merge_level{};

  friend bool operator==(const ExactEmstEdge&, const ExactEmstEdge&) = default;
};

// Leaves have no children, a singleton point_ids coverage and level zero.
// Every non-leaf is the canonical multifusion of all distinct components
// incident in one connected component of the strict pre-batch quotient.
struct K1HierarchyNode {
  K1NodeId id{};
  exact::ExactLevel level{};
  std::vector<K1NodeId> children;
  std::vector<spatial::PointId> point_ids;

  [[nodiscard]] bool is_leaf() const noexcept { return children.empty(); }

  friend bool operator==(const K1HierarchyNode&, const K1HierarchyNode&) = default;
};

using K1CutComponent = std::vector<spatial::PointId>;
using K1Cut = std::vector<K1CutComponent>;

struct K1Multifusion {
  K1NodeId node_id{};
  std::vector<K1CutComponent> child_components;
  K1CutComponent merged_component;

  [[nodiscard]] std::size_t arity() const noexcept {
    return child_components.size();
  }

  friend bool operator==(const K1Multifusion&, const K1Multifusion&) = default;
};

struct K1EqualLevelBatch {
  exact::ExactLevel squared_length{};
  exact::ExactLevel level{};
  std::size_t candidate_edge_count{};
  std::vector<ExactEmstEdge> complete_edges;
  std::vector<ExactEmstEdge> selected_edges;
  std::vector<K1NodeId> merge_node_ids;
  std::vector<K1Multifusion> multifusions;
  std::size_t pre_batch_component_count{};
  std::size_t post_batch_component_count{};

  friend bool operator==(const K1EqualLevelBatch&, const K1EqualLevelBatch&) = default;
};

struct ExactEmstCounters {
  std::size_t point_count{};
  std::size_t distance_evaluations{};
  std::size_t complete_edge_count{};
  std::size_t distinct_edge_weight_count{};
  std::size_t equal_weight_batch_count{};
  std::size_t max_equal_weight_batch_size{};
  std::size_t selected_edge_count{};
  std::size_t redundant_edge_count{};
  std::size_t merge_batch_count{};
  std::size_t merge_event_count{};
  std::size_t multifusion_count{};
  std::size_t max_merge_arity{};
  std::size_t replay_level_count{};

  friend bool operator==(const ExactEmstCounters&, const ExactEmstCounters&) = default;
};

struct K1EmstResult {
  std::size_t point_count{};
  std::size_t complete_graph_edge_count{};
  exact::ExactLevel total_squared_weight{};
  exact::ExactLevel total_hgp_weight{};
  std::vector<ExactEmstEdge> complete_edges;
  std::vector<ExactEmstEdge> emst_edges;
  std::vector<K1HierarchyNode> nodes;
  std::vector<K1EqualLevelBatch> equal_level_batches;
  K1NodeId root_node_id{};
  ExactEmstCounters counters{};

  // A strict cut activates leaves only for level > 0 and replays edges with
  // merge_level < level. A closed cut activates leaves for level >= 0 and
  // also includes the entire equality batch at level. Components and their
  // point identifiers are returned in canonical increasing order.
  [[nodiscard]] K1Cut cut(
      const exact::ExactLevel& level,
      K1CutClosure closure = K1CutClosure::closed,
      K1CutEdgeSource edge_source = K1CutEdgeSource::selected_emst) const;
};

// Exact reference/production anchor for order k=1. The implementation forms
// every edge of the complete Euclidean graph, computes its dyadic squared
// length exactly, orders edges by exact comparison then canonical endpoint
// IDs, and contracts equal-length batches from a frozen pre-batch state.
[[nodiscard]] K1EmstResult build_exact_complete_graph_emst(
    const spatial::CanonicalPointCloud& cloud);

}  // namespace morsehgp3d::hierarchy
