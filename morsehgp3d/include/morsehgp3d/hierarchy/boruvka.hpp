#pragma once

#include "morsehgp3d/hierarchy/emst.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace morsehgp3d::hierarchy {

// The minimum is selected against labels frozen at the beginning of the
// round. component_label is the least PointId in that frozen component and
// source_point_id identifies the endpoint that issued the point-to-LBVH
// query. outgoing_edge is oriented canonically by endpoint identifier.
struct K1BoruvkaComponentMinimum {
  spatial::PointId component_label{};
  spatial::PointId source_point_id{};
  ExactEmstEdge outgoing_edge{};

  friend bool operator==(
      const K1BoruvkaComponentMinimum&,
      const K1BoruvkaComponentMinimum&) = default;
};

struct K1BoruvkaRound {
  std::size_t round_index{};
  std::size_t pre_round_component_count{};
  std::vector<K1BoruvkaComponentMinimum> component_minima;
  // Duplicate minima chosen from both endpoints are stored only once. The
  // retained edges are ordered by (squared_length, u, v).
  std::vector<ExactEmstEdge> accepted_edges;
  std::size_t post_round_component_count{};
  std::size_t uniform_lbvh_node_count{};
  std::size_t mixed_lbvh_node_count{};

  friend bool operator==(const K1BoruvkaRound&, const K1BoruvkaRound&) =
      default;
};

// Pure CPU combinatorial reduction of one already-certified exact Boruvka
// decision. The input labels are the least PointId of every frozen component;
// component_minima keeps one witness per component. Only accepted_edges is
// deduplicated. The post-round labels again use the least PointId.
struct K1BoruvkaRoundContraction {
  std::size_t pre_round_component_count{};
  std::vector<ExactEmstEdge> accepted_edges;
  std::vector<spatial::PointId> post_round_component_labels;
  std::size_t post_round_component_count{};

  friend bool operator==(
      const K1BoruvkaRoundContraction&,
      const K1BoruvkaRoundContraction&) = default;
};

// Validates the canonical frozen partition, recomputes the exact geometry of
// every already-certified minimum, sorts/deduplicates by
// (squared_length, u, v), rejects cycles, enforces the Boruvka halving bound
// and returns canonical labels. This reducer does not decide kappa-minimality.
[[nodiscard]] K1BoruvkaRoundContraction contract_exact_k1_boruvka_round(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> frozen_component_labels,
    std::span<const K1BoruvkaComponentMinimum> component_minima);

struct K1ExactBoruvkaCounters {
  std::size_t point_count{};
  std::size_t lbvh_node_count{};
  std::size_t round_count{};
  std::size_t theoretical_max_round_count{};
  std::size_t frozen_component_label_count{};
  std::size_t component_minimum_count{};
  std::size_t accepted_edge_count{};
  std::size_t component_contraction_count{};
  std::size_t point_query_count{};
  std::size_t node_visit_count{};
  std::size_t internal_node_expansion_count{};
  std::size_t exact_aabb_bound_evaluation_count{};
  std::size_t exact_point_distance_evaluation_count{};
  std::size_t uniform_lbvh_node_tag_count{};
  std::size_t mixed_lbvh_node_tag_count{};
  std::size_t uniform_component_prune_count{};
  std::size_t strict_aabb_prune_count{};
  std::size_t final_component_count{};

  friend bool operator==(
      const K1ExactBoruvkaCounters&,
      const K1ExactBoruvkaCounters&) = default;
};

// This is deliberately not K1EmstResult: no complete graph, hierarchy,
// public status or persistent coverage is materialized. The accepted witness
// can be passed separately to build_compact_k1_forest(point_count, emst_edges).
struct K1ExactBoruvkaResult {
  static constexpr const char* proof_basis =
      "boruvka_exact_cut_minima_lbvh_v1";

  std::size_t point_count{};
  std::vector<K1BoruvkaRound> rounds;
  // Canonical output order is always (squared_length, u, v), independently
  // of the round in which an edge was accepted.
  std::vector<ExactEmstEdge> emst_edges;
  exact::ExactLevel total_squared_weight{};
  exact::ExactLevel total_hgp_weight{};
  K1ExactBoruvkaCounters counters{};
  // Local statement only: the guarded replay below certified the Boruvka
  // witness for this cloud and index. This is not a MorseHGP3D public status.
  bool emst_witness_certified{false};
};

struct K1BoruvkaVerification {
  bool index_identity_certified{false};
  bool round_count_bound_certified{false};
  bool round_replay_certified{false};
  bool component_minima_certified{false};
  bool accepted_edges_certified{false};
  bool canonical_contractions_certified{false};
  bool spanning_tree_certified{false};
  bool exact_weights_certified{false};
  bool emst_witness_certified{false};
  std::size_t replayed_round_count{};
  std::size_t replayed_component_minimum_count{};
};

// Exact CPU Boruvka producer over one already-built Morton LBVH. Every round
// freezes component labels, tags every LBVH node uniform or mixed, performs
// exact point-to-tree searches, prunes only on strict exact AABB separation,
// and contracts through the least PointId. The caller retains ownership of
// the unique spatial index.
[[nodiscard]] K1ExactBoruvkaResult build_exact_lbvh_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud);

// Replays labels and rounds from the same immutable cloud/index and checks the
// complete witness rather than trusting result flags or counters.
[[nodiscard]] K1BoruvkaVerification verify_exact_lbvh_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const K1ExactBoruvkaResult& result);

}  // namespace morsehgp3d::hierarchy
