#pragma once

#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t binary64_lbvh_top_k_schema_version = 1U;
inline constexpr std::string_view binary64_lbvh_top_k_backend =
    "cuda_binary64_lbvh_top_k";
inline constexpr std::string_view binary64_lbvh_top_k_profile =
    "hgp_reduced_surrogate";
inline constexpr std::string_view binary64_lbvh_top_k_mode =
    "stackless_aabb_branch_and_bound";

namespace detail {
class Phase14Binary64LbvhTopKHostState;
}  // namespace detail

// Operational evidence for one complete all-point query.  The fixed binary64
// recipe and strict directed AABB prune qualify only this neighbor transcript;
// they do not certify the Morse hierarchy or exact-rational geometry.
struct Binary64LbvhTopKAudit {
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t maximum_order{};
  std::size_t seed_window_radius{};
  std::size_t neighbor_record_count{};
  std::size_t seed_distance_evaluation_count{};
  std::size_t traversal_leaf_distance_evaluation_count{};
  std::size_t node_visit_count{};
  std::size_t strict_aabb_prune_count{};
  std::size_t seed_covered_subtree_skip_count{};
  std::size_t invalid_aabb_bound_descent_count{};
  std::size_t maximum_node_visit_count_per_query{};
  std::size_t median_node_visit_count_per_query{};
  std::size_t p95_node_visit_count_per_query{};
  std::size_t p99_node_visit_count_per_query{};
  std::size_t full_tree_query_count{};
  std::size_t completed_query_count{};
  std::size_t failed_query_count{};
  std::size_t persistent_input_device_byte_capacity{};
  std::size_t transient_output_device_byte_capacity{};
  std::size_t kernel_block_count{};
  std::size_t kernel_thread_count{};
  std::uint64_t allocation_nanoseconds{};
  std::uint64_t kernel_nanoseconds{};
  std::uint64_t device_to_host_nanoseconds{};
  std::uint64_t launcher_wall_nanoseconds{};
  std::uint64_t source_snapshot_epoch{};
  int cuda_device{-1};
  int multiprocessor_count{};
  std::string device_name;
  bool traversal_lease_adopted{};
  bool fixed_round_to_nearest_distance_recipe_requested{};
  bool directed_round_down_aabb_recipe_requested{};
  bool strict_prune_requested{};
  bool stackless_postorder_traversal_requested{};
  bool complete_query_coverage{};
  bool no_candidate_truncation{};
  bool source_morton_permutation_validated{};
  bool host_transcript_structure_validated{};
  bool higher_order_delaunay_mosaic_materialized{};
  bool global_pair_matrix_materialized{};
  bool exact_morse_hierarchy_claimed{};
  bool public_status_claimed{};

  friend bool operator==(
      const Binary64LbvhTopKAudit&,
      const Binary64LbvhTopKAudit&) = default;
};

struct Binary64LbvhTopKResult {
  std::uint32_t schema_version{binary64_lbvh_top_k_schema_version};
  // Row-major by certified Morton position, then lexicographic
  // (squared_distance, PointId).
  std::vector<spatial::PointId> source_point_ids_by_morton_position;
  std::vector<spatial::PointId> neighbor_point_ids;
  std::vector<double> squared_distances;
  Binary64LbvhTopKAudit audit;

  [[nodiscard]] bool validated_complete_binary64_transcript() const noexcept;
};

inline constexpr std::uint32_t
    binary64_lbvh_csr_neighbor_rank_schema_version = 1U;
inline constexpr std::size_t
    binary64_lbvh_csr_neighbor_rank_maximum = 256U;

// Offline-only audit for the exact prefix of the fixed binary64
// (squared_distance, PointId) key.  The query traverses the complete certified
// Morton LBVH and writes ranks only for arcs already present in a supplied
// CSR graph.  Ranks greater than maximum_rank are censored to
// overflow_rank=maximum_rank+1; this is a proposal diagnostic, not a proof of
// exact Euclidean or Morse geometry.
struct Binary64LbvhCsrNeighborRankAudit {
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t directed_csr_arc_count{};
  std::size_t maximum_rank{};
  std::size_t overflow_rank{};
  std::size_t seed_window_radius{};
  std::size_t query_batch_size{};
  std::size_t query_batch_count{};
  std::size_t ranked_arc_count{};
  std::size_t overflow_arc_count{};
  std::size_t seed_distance_evaluation_count{};
  std::size_t traversal_leaf_distance_evaluation_count{};
  std::size_t node_visit_count{};
  std::size_t strict_aabb_prune_count{};
  std::size_t aabb_equality_descent_count{};
  std::size_t seed_covered_subtree_skip_count{};
  std::size_t invalid_aabb_bound_descent_count{};
  std::size_t maximum_node_visit_count_per_query{};
  std::size_t full_tree_query_count{};
  std::size_t completed_query_count{};
  std::size_t failed_query_count{};
  std::size_t persistent_input_device_byte_capacity{};
  std::size_t transient_csr_device_byte_capacity{};
  std::size_t maximum_batch_audit_device_byte_capacity{};
  std::size_t kernel_block_count_maximum{};
  std::size_t kernel_thread_count_maximum{};
  std::size_t kernel_local_size_bytes_per_thread{};
  std::uint64_t allocation_and_input_copy_nanoseconds{};
  std::uint64_t kernel_nanoseconds{};
  std::uint64_t device_to_host_nanoseconds{};
  std::uint64_t launcher_wall_nanoseconds{};
  std::uint64_t source_snapshot_epoch{};
  int cuda_device{-1};
  int multiprocessor_count{};
  std::string device_name;
  bool traversal_lease_adopted{};
  bool fixed_round_to_nearest_distance_recipe_requested{};
  bool directed_round_down_aabb_recipe_requested{};
  bool strict_prune_requested{};
  bool equality_descends_requested{};
  bool stackless_postorder_traversal_requested{};
  bool complete_query_coverage{};
  bool no_search_work_cap{};
  bool exact_binary64_prefix_complete{};
  bool ranks_above_maximum_censored{};
  bool source_mapping_permutation_validated{};
  bool csr_structure_validated{};
  bool host_rank_transcript_validated{};
  bool n_by_maximum_rank_table_materialized{};
  bool higher_order_delaunay_mosaic_materialized{};
  bool global_pair_matrix_materialized{};
  bool exact_morse_hierarchy_claimed{};
  bool public_status_claimed{};

  friend bool operator==(
      const Binary64LbvhCsrNeighborRankAudit&,
      const Binary64LbvhCsrNeighborRankAudit&) = default;
};

struct Binary64LbvhCsrNeighborRankResult {
  std::uint32_t schema_version{
      binary64_lbvh_csr_neighbor_rank_schema_version};
  // Aligned with the caller-supplied directed CSR adjacency array.  A value
  // in [1, maximum_rank] is the exact rank under the fixed binary64 key;
  // overflow_rank means only that the exact rank is greater than maximum_rank.
  std::vector<std::uint16_t> directed_csr_ranks;
  Binary64LbvhCsrNeighborRankAudit audit;

  [[nodiscard]] bool validated_complete_binary64_rank_prefix() const noexcept;
};

// Consumes the unique resident traversal lease.  Calls are synchronous and
// serialized.  The seed window supplies only an initial upper bound; every
// omitted subtree must be strictly pruned by a directed AABB lower bound, so
// the implementation has no candidate or traversal cap.
class Binary64LbvhTopKContext final {
 public:
  Binary64LbvhTopKContext(
      const spatial::CanonicalPointCloud& cloud,
      MortonLbvhDeviceTraversalLease&& traversal_lease);
  ~Binary64LbvhTopKContext() noexcept;

  Binary64LbvhTopKContext(Binary64LbvhTopKContext&&) noexcept;
  Binary64LbvhTopKContext& operator=(
      Binary64LbvhTopKContext&&) noexcept;

  Binary64LbvhTopKContext(const Binary64LbvhTopKContext&) = delete;
  Binary64LbvhTopKContext& operator=(
      const Binary64LbvhTopKContext&) = delete;

  [[nodiscard]] Binary64LbvhTopKResult query_all(
      const spatial::CanonicalPointCloud& cloud,
      std::size_t maximum_order,
      std::size_t seed_window_radius);

  // Diagnostic-only compact query.  source_point_id_by_canonical_id maps the
  // canonical cloud namespace back to the CSR namespace.  CSR rows and each
  // row's targets must be strictly increasing.  The implementation batches
  // only per-query audit records; it never materializes an n-by-M transcript.
  [[nodiscard]] Binary64LbvhCsrNeighborRankResult query_csr_neighbor_ranks(
      const spatial::CanonicalPointCloud& cloud,
      std::span<const std::uint64_t> csr_offsets,
      std::span<const spatial::PointId> csr_neighbors,
      std::span<const spatial::PointId> source_point_id_by_canonical_id,
      std::size_t maximum_rank,
      std::size_t seed_window_radius,
      std::size_t query_batch_size);

  [[nodiscard]] std::size_t point_count() const noexcept;
  [[nodiscard]] std::size_t certified_node_count() const noexcept;

 private:
  void require_matching_cloud(
      const spatial::CanonicalPointCloud& cloud) const;

  std::unique_ptr<detail::Phase14Binary64LbvhTopKHostState> host_;
};

}  // namespace morsehgp3d::gpu
