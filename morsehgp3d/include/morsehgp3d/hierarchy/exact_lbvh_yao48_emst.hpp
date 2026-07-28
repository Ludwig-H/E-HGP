#pragma once

#include "morsehgp3d/hierarchy/emst.hpp"
#include "morsehgp3d/hierarchy/yao48_emst.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t exact_lbvh_yao48_emst_schema_version = 1U;
inline constexpr std::string_view exact_lbvh_yao48_emst_backend =
    "reference_cpu";
inline constexpr std::string_view exact_lbvh_yao48_emst_profile =
    "hgp_reduced";
inline constexpr std::string_view exact_lbvh_yao48_emst_mode =
    "exact_search_prototype";
inline constexpr std::string_view exact_lbvh_yao48_emst_public_status =
    "not_claimed";
inline constexpr std::string_view exact_lbvh_yao48_emst_proof_basis =
    "single_reverse_postorder_morton_lbvh_traversal_per_source_"
    "exact_closed_yao48_cone_aabb_intersection_mask_strict_exact_aabb_"
    "distance_pruning_half_open_leaf_partition_canonical_edge_key_v1";

struct ExactLbvhYao48EmstConfig {
  // A bounded Morton neighbourhood only initializes exact incumbents.  Every
  // point outside it remains covered by the complete LBVH traversal.
  std::size_t morton_seed_window_radius{32U};

  friend bool operator==(
      const ExactLbvhYao48EmstConfig&,
      const ExactLbvhYao48EmstConfig&) = default;
};

// Compact GPU-shaped transcript.  The exact squared length is authenticated
// by the immutable cloud and is materialized once after endpoint deduplication.
// Records are emitted in (source_point_id, cone_index) order.
struct ExactLbvhYao48DirectedCandidate {
  spatial::PointId source_point_id{};
  std::size_t cone_index{};
  spatial::PointId target_point_id{};

  friend bool operator==(
      const ExactLbvhYao48DirectedCandidate&,
      const ExactLbvhYao48DirectedCandidate&) = default;
};

struct ExactLbvhYao48EmstCounters {
  std::size_t point_count{};
  std::size_t lbvh_node_count{};
  std::size_t morton_seed_window_radius{};
  std::size_t seed_distance_evaluation_count{};
  std::size_t node_visit_count{};
  std::size_t exact_aabb_lower_bound_evaluation_count{};
  std::size_t closed_cone_aabb_intersection_test_count{};
  std::size_t cone_mask_empty_count{};
  std::size_t strict_aabb_prune_count{};
  std::size_t equal_bound_descent_count{};
  std::size_t strict_pruned_leaf_count{};
  std::size_t seeded_subtree_skip_count{};
  std::size_t seeded_subtree_skipped_leaf_count{};
  std::size_t leaf_distance_evaluation_count{};
  std::size_t directed_cone_classification_count{};
  std::size_t unique_candidate_distance_evaluation_count{};
  std::size_t completed_source_count{};
  std::size_t maximum_node_visit_count_per_source{};
  std::size_t maximum_leaf_distance_evaluation_count_per_source{};
  std::size_t certified_node_coverage_count{};
  std::size_t certified_leaf_coverage_count{};
  std::size_t directed_candidate_count{};
  std::size_t empty_point_cone_count{};
  std::size_t unique_candidate_edge_count{};
  std::size_t selected_edge_count{};
  std::size_t redundant_candidate_edge_count{};
  std::size_t final_component_count{};
  // Fixed C++ record envelopes only; dynamic exact-integer limbs and allocator
  // overhead are intentionally not presented as measured retained bytes.
  std::size_t fixed_output_record_byte_count{};
  bool complete_source_coverage{};
  bool no_candidate_truncation{};
  bool morton_permutation_validated{};
  bool higher_order_delaunay_mosaic_materialized{};
  bool global_pair_matrix_materialized{};
  bool public_status_claimed{};

  friend bool operator==(
      const ExactLbvhYao48EmstCounters&,
      const ExactLbvhYao48EmstCounters&) = default;
};

// Exact CPU search prototype over the one shared Morton LBVH.  There is one
// stackless reverse-postorder traversal per source, not one traversal per
// cone.  A node is skipped by geometry only when its exact AABB lower bound is
// strictly larger than the incumbent of every closed Yao cone whose closure
// can intersect that AABB.  Equality therefore always descends.  Leaves are
// assigned to the oracle's half-open partition, preserving its exact tie
// semantics.  The algorithm never forms Delaunay, a pair matrix, a
// higher-order cell, a coface arena or a 48*n incumbent table.
struct ExactLbvhYao48EmstResult {
  static constexpr std::string_view backend =
      exact_lbvh_yao48_emst_backend;
  static constexpr std::string_view profile =
      exact_lbvh_yao48_emst_profile;
  static constexpr std::string_view mode = exact_lbvh_yao48_emst_mode;
  static constexpr std::string_view public_status =
      exact_lbvh_yao48_emst_public_status;
  static constexpr std::string_view proof_basis =
      exact_lbvh_yao48_emst_proof_basis;

  std::uint32_t schema_version{exact_lbvh_yao48_emst_schema_version};
  std::size_t point_count{};
  ExactLbvhYao48EmstConfig config{};
  std::vector<ExactLbvhYao48DirectedCandidate> directed_candidates;
  std::vector<ExactEmstEdge> unique_candidate_edges;
  std::vector<ExactEmstEdge> emst_edges;
  exact::ExactLevel total_squared_weight{};
  exact::ExactLevel total_hgp_weight{};
  ExactLbvhYao48EmstCounters counters{};

  // Local structure and accounting only.  Geometry is authenticated by the
  // bounded fresh comparison below.
  [[nodiscard]] bool locally_structurally_valid() const;

  friend bool operator==(
      const ExactLbvhYao48EmstResult&,
      const ExactLbvhYao48EmstResult&) = default;
};

[[nodiscard]] ExactLbvhYao48EmstResult build_exact_lbvh_yao48_emst(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    ExactLbvhYao48EmstConfig config = {});

struct ExactLbvhYao48EmstVerification {
  bool index_identity_certified{false};
  bool input_within_reference_bound{false};
  bool result_structurally_valid{false};
  bool operational_replay_exact{false};
  bool directed_candidates_match_reference{false};
  bool unique_candidate_edges_match_reference{false};
  bool canonical_kruskal_matches_reference{false};
  bool exact_totals_match_reference{false};
  bool canonical_emst_matches_exact_lbvh_boruvka{false};
  bool full_bounded_geometry_transcript_exact{false};

  [[nodiscard]] bool transcript_certified() const noexcept {
    return index_identity_certified && input_within_reference_bound &&
           result_structurally_valid && operational_replay_exact &&
           directed_candidates_match_reference &&
           unique_candidate_edges_match_reference &&
           canonical_kruskal_matches_reference &&
           exact_totals_match_reference &&
           canonical_emst_matches_exact_lbvh_boruvka &&
           full_bounded_geometry_transcript_exact;
  }

  friend bool operator==(
      const ExactLbvhYao48EmstVerification&,
      const ExactLbvhYao48EmstVerification&) = default;
};

// Fresh bounded qualification.  It compares the complete directed candidate,
// deduplicated edge, canonical Kruskal and exact-weight transcript with the
// independent O(n^2) Yao oracle, and independently compares the final tree
// with exact LBVH Boruvka.  It refuses inputs beyond the oracle guard.
[[nodiscard]] ExactLbvhYao48EmstVerification
verify_exact_lbvh_yao48_emst_bounded(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactLbvhYao48EmstResult& result);

}  // namespace morsehgp3d::hierarchy
