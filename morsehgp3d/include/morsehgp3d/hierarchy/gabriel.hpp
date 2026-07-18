#pragma once

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/hierarchy/emst.hpp"

#include <cstddef>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

// Every returned pair decision is exact.  This status is deliberately
// separate from K1PairCatalogStatus: an exactly detected unsupported
// degeneracy is still an exact decision, but cannot support a public exact
// catalogue claim.
enum class K1PairDecisionStatus {
  exact,
};

enum class K1PairCatalogStatus {
  supported,
  unsupported_degeneracy,
};

enum class K1PairSphereClassification {
  // The global closed ball consists exactly of the two support endpoints.
  rank_two_critical,
  // Its open interior is empty, but at least one additional site lies on the
  // shell.  This remains a useful Gabriel diagnostic and is unsupported by
  // the first public general-position contract.
  extra_shell_degeneracy,
  // At least one ambient point is strictly inside the diametric ball.
  interior_blocked,
};

[[nodiscard]] std::string_view to_string(K1PairDecisionStatus status);
[[nodiscard]] std::string_view to_string(K1PairCatalogStatus status);
[[nodiscard]] std::string_view to_string(
    K1PairSphereClassification classification);

struct K1PairSphereRecord {
  // Stable index in K1PairSphereCatalog::pairs.  Pairs are enumerated
  // lexicographically by canonical PointId, independently of the EMST path.
  std::size_t pair_index{};
  spatial::PointId u{};
  spatial::PointId v{};
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_length{};
  exact::ExactLevel level{};
  std::vector<spatial::PointId> interior_ids;
  std::vector<spatial::PointId> shell_ids;
  std::size_t exterior_count{};
  std::size_t closed_rank{};
  K1PairDecisionStatus decision_status{K1PairDecisionStatus::exact};
  K1PairSphereClassification classification{
      K1PairSphereClassification::interior_blocked};

  [[nodiscard]] bool is_rank_two_critical() const noexcept {
    return classification == K1PairSphereClassification::rank_two_critical;
  }

  [[nodiscard]] bool is_gabriel_diagnostic() const noexcept {
    return classification != K1PairSphereClassification::interior_blocked;
  }

  [[nodiscard]] ExactEmstEdge edge() const;

  friend bool operator==(const K1PairSphereRecord&, const K1PairSphereRecord&) =
      default;
};

struct K1PairCatalogCounters {
  std::size_t point_count{};
  std::size_t pair_count{};
  std::size_t support_analysis_count{};
  std::size_t support_predicate_decision_count{};
  std::size_t closed_ball_query_count{};
  std::size_t exact_point_distance_evaluation_count{};
  std::size_t rank_two_critical_count{};
  std::size_t extra_shell_degeneracy_count{};
  std::size_t interior_blocked_count{};
  std::size_t gabriel_diagnostic_count{};

  friend bool operator==(
      const K1PairCatalogCounters&, const K1PairCatalogCounters&) = default;
};

struct K1PairSphereCatalog {
  std::size_t point_count{};
  K1PairDecisionStatus decision_status{K1PairDecisionStatus::exact};
  K1PairCatalogStatus catalog_status{K1PairCatalogStatus::supported};
  std::vector<K1PairSphereRecord> pairs;
  std::vector<std::size_t> rank_two_pair_indices;
  std::vector<std::size_t> gabriel_diagnostic_pair_indices;
  // All three edge vectors are sorted by exact squared length, then endpoint
  // IDs.  all_pair_edges is the independently enumerated complete graph.
  std::vector<ExactEmstEdge> all_pair_edges;
  std::vector<ExactEmstEdge> rank_two_edges;
  std::vector<ExactEmstEdge> gabriel_diagnostic_edges;
  K1PairCatalogCounters counters{};

  [[nodiscard]] bool exact_decisions_complete() const noexcept {
    return point_count != 0U &&
           decision_status == K1PairDecisionStatus::exact &&
           counters.point_count == point_count &&
           counters.pair_count == pairs.size();
  }

  friend bool operator==(
      const K1PairSphereCatalog&, const K1PairSphereCatalog&) = default;
};

// Exhaustive reference catalogue.  It intentionally performs an independent
// canonical pair enumeration, an exact two-point support analysis, then one
// complete global closed-ball query per pair.
[[nodiscard]] K1PairSphereCatalog build_exact_k1_pair_sphere_catalog(
    const spatial::CanonicalPointCloud& cloud);

enum class K1RankTwoCutEdgeSource {
  rank_two_graph,
  selected_witness_tree,
  gabriel_diagnostic_graph,
};

struct K1RankTwoEqualLevelBatch {
  exact::ExactLevel squared_length{};
  exact::ExactLevel level{};
  std::vector<ExactEmstEdge> rank_two_edges;
  std::vector<ExactEmstEdge> selected_witness_edges;
  std::vector<K1NodeId> merge_node_ids;
  std::vector<K1Multifusion> multifusions;
  std::size_t pre_batch_component_count{};
  std::size_t post_batch_component_count{};

  friend bool operator==(
      const K1RankTwoEqualLevelBatch&,
      const K1RankTwoEqualLevelBatch&) = default;
};

struct K1RankTwoReductionCounters {
  std::size_t point_count{};
  std::size_t rank_two_edge_count{};
  std::size_t gabriel_diagnostic_edge_count{};
  std::size_t distinct_rank_two_level_count{};
  std::size_t equal_level_batch_count{};
  std::size_t max_equal_level_batch_size{};
  std::size_t selected_witness_edge_count{};
  std::size_t redundant_rank_two_edge_count{};
  std::size_t merge_batch_count{};
  std::size_t merge_event_count{};
  std::size_t multifusion_count{};
  std::size_t max_merge_arity{};
  std::size_t replay_level_count{};

  friend bool operator==(
      const K1RankTwoReductionCounters&,
      const K1RankTwoReductionCounters&) = default;
};

struct K1RankTwoReductionResult {
  std::size_t point_count{};
  std::vector<ExactEmstEdge> rank_two_edges;
  std::vector<ExactEmstEdge> gabriel_diagnostic_edges;
  std::vector<ExactEmstEdge> selected_witness_edges;
  exact::ExactLevel total_selected_squared_weight{};
  exact::ExactLevel total_selected_hgp_weight{};
  std::vector<K1HierarchyNode> nodes;
  std::vector<K1RankTwoEqualLevelBatch> equal_level_batches;
  K1NodeId root_node_id{};
  K1RankTwoReductionCounters counters{};

  [[nodiscard]] K1Cut cut(
      const exact::ExactLevel& level,
      K1CutClosure closure = K1CutClosure::closed,
      K1RankTwoCutEdgeSource edge_source =
          K1RankTwoCutEdgeSource::rank_two_graph) const;

  friend bool operator==(
      const K1RankTwoReductionResult&, const K1RankTwoReductionResult&) =
      default;
};

// Contracts only supported rank-two edges.  Every equality batch is formed
// from a frozen strict pre-batch DSU state; the selected edges are deterministic
// witnesses and never define the semantic multifusion.
[[nodiscard]] K1RankTwoReductionResult build_exact_k1_rank_two_reduction(
    const K1PairSphereCatalog& catalog);

struct K1ExactAnchorCertificate {
  bool exact_pair_decisions_complete{};
  bool pair_universe_matches_emst{};
  std::size_t comparison_level_count{};
  bool strict_cuts_match{};
  bool closed_cuts_match{};
  bool multifusions_match{};
  bool selected_tree_edges_match{};
  bool selected_tree_squared_weight_matches{};
  bool selected_tree_hgp_weight_matches{};
  bool selected_tree_hierarchy_matches{};
  bool all_selected_witness_edges_are_rank_two{};
  // Semantic aggregate. selected_tree_edges_match is deliberately excluded:
  // equal-weight batches may admit several valid witness trees.
  bool anchor_equivalence_certified{};

  friend bool operator==(
      const K1ExactAnchorCertificate&, const K1ExactAnchorCertificate&) =
      default;
};

struct K1ExactAnchorResult {
  K1EmstResult emst;
  K1PairSphereCatalog pair_catalog;
  K1RankTwoReductionResult rank_two_reduction;
  K1ExactAnchorCertificate certificate{};

  // Local readiness is not the project public_status.  In particular, the
  // global catalogue, proof basis and later phase gates remain external.
  [[nodiscard]] bool locally_supported() const noexcept {
    return pair_catalog.catalog_status == K1PairCatalogStatus::supported &&
           certificate.anchor_equivalence_certified;
  }
};

// Builds the two independent exact paths and checks them at the union of all
// exact levels.  A mismatch is preserved as a fail-closed certificate instead
// of being converted into a public exact claim.  An extra-shell degeneracy is
// likewise recorded without preventing either exact diagnostic path.
[[nodiscard]] K1ExactAnchorResult build_exact_k1_anchor(
    const spatial::CanonicalPointCloud& cloud);

}  // namespace morsehgp3d::hierarchy
