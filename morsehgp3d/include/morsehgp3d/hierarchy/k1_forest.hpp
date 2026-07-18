#pragma once

#include "morsehgp3d/hierarchy/emst.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace morsehgp3d::hierarchy {

// Leaves are implicit: node identifiers [0, point_count) are singleton
// PointIds. Only multifusion nodes occupy the compact merge-node arena.
struct K1CompactMergeNode {
  K1NodeId id{};
  std::size_t level_index{};
  std::size_t child_offset{};
  std::size_t child_count{};
  spatial::PointId minimum_point_id{};
  std::size_t coverage_size{};

  friend bool operator==(const K1CompactMergeNode&, const K1CompactMergeNode&) =
      default;
};

// Offsets address the selected-edge and merge-node arenas, respectively.
// The component counts describe the frozen strict pre-batch state and the
// closed post-batch state.
struct K1CompactEqualLevelBatch {
  std::size_t level_index{};
  std::size_t selected_edge_offset{};
  std::size_t selected_edge_count{};
  std::size_t merge_node_offset{};
  std::size_t merge_node_count{};
  std::size_t pre_batch_component_count{};
  std::size_t post_batch_component_count{};

  friend bool operator==(
      const K1CompactEqualLevelBatch&,
      const K1CompactEqualLevelBatch&) = default;
};

struct K1CompactForestCounters {
  std::size_t point_count{};
  std::size_t input_edge_count{};
  std::size_t selected_edge_count{};
  std::size_t batched_selected_edge_count{};
  std::size_t distinct_level_count{};
  std::size_t equal_level_batch_count{};
  std::size_t merge_node_count{};
  std::size_t batched_merge_node_count{};
  std::size_t child_reference_count{};
  std::size_t total_node_count{};
  std::size_t merge_event_count{};
  std::size_t multifusion_count{};
  std::size_t max_merge_arity{};
  std::size_t root_coverage_size{};
  std::size_t stored_coverage_point_id_count{};
  // Logical entries in the five persistent arenas. This structural O(n)
  // count deliberately excludes allocator metadata and exact-integer limbs.
  std::size_t linear_storage_entry_count{};
  std::size_t linear_storage_entry_limit{};

  friend bool operator==(
      const K1CompactForestCounters&,
      const K1CompactForestCounters&) = default;
};

struct K1CompactForest {
  std::size_t point_count{};
  std::vector<exact::ExactLevel> levels;
  std::vector<ExactEmstEdge> selected_edges;
  std::vector<K1CompactMergeNode> merge_nodes;
  std::vector<K1NodeId> child_ids;
  std::vector<K1CompactEqualLevelBatch> equal_level_batches;
  K1NodeId root_node_id{};
  exact::ExactLevel total_squared_weight{};
  exact::ExactLevel total_hgp_weight{};
  K1CompactForestCounters counters{};

  [[nodiscard]] std::size_t node_count() const noexcept;
  [[nodiscard]] bool is_leaf(K1NodeId node_id) const;
  [[nodiscard]] exact::ExactLevel node_level(K1NodeId node_id) const;
  [[nodiscard]] std::span<const K1NodeId> children(K1NodeId node_id) const;

  // Leaves are born at level zero. Strict cuts therefore omit all leaves at
  // zero, whereas closed cuts include them and their complete equality batch.
  [[nodiscard]] K1Cut cut(
      const exact::ExactLevel& level,
      K1CutClosure closure = K1CutClosure::closed) const;
  [[nodiscard]] K1Cut cut_strict(const exact::ExactLevel& level) const;
  [[nodiscard]] K1Cut cut_closed(const exact::ExactLevel& level) const;

  // Coverage vectors are intentionally absent from persistent storage and are
  // reconstructed only for oracle/debug consumers. Materializing every node
  // can allocate Theta(n^2) output and is outside the scalable path.
  [[nodiscard]] K1HierarchyNode materialize_node(K1NodeId node_id) const;
  [[nodiscard]] K1Multifusion materialize_multifusion(K1NodeId node_id) const;
  [[nodiscard]] std::vector<K1HierarchyNode> materialize_nodes() const;
  [[nodiscard]] std::vector<K1Multifusion> materialize_multifusions() const;
};

// Builds the canonical compact k=1 hierarchy from an exact EMST that has
// already been certified by a geometric producer. Input edge order and
// endpoint orientation are ignored. This adapter validates only the spanning
// tree, exact weights and length-to-level relation, frozen equality batches,
// CSR offsets and linear storage identities; point geometry and minimality are
// deliberately outside this input contract.
[[nodiscard]] K1CompactForest build_compact_k1_forest(
    std::size_t point_count,
    std::span<const ExactEmstEdge> certified_emst_edges);

[[nodiscard]] K1CompactForest build_compact_k1_forest(
    const K1EmstResult& emst);

}  // namespace morsehgp3d::hierarchy
