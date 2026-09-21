#pragma once

#include "lanes/q4_local_partition.hpp"
#include "lanes/q34_seed.hpp"

namespace mhgp8 {

struct Q4LocalOptions {
  Q4CenterDomainMode domain{Q4CenterDomainMode::Positive};
  unsigned max_depth{7};
  std::size_t node_budget{4096};
  // Intermediate fragment tests only. An unfinished terminal frontier is
  // classified completely ONCE before it is shared by any seed queries.
  u64 z_test_budget{512};
  std::size_t leaf_sites{32};
  bool clip_events{true};
};

struct Q4LocalAtlasWork {
  Q4LocalPartitionWork partition;
  Q4LocalGeometryQueryWork domain;
  u64 cells_created{}, outside_cells{}, deep_cells{}, leaf_cells{}, splits{};
  u64 depth_stops{}, node_stops{}, small_stops{}, active_sites_sum{}, active_blocks_sum{};
  u64 terminal_refinements{}, terminal_deep_cells{};
  u64 max_depth{}, peak_fragment_bytes{}, peak_build_bytes{}, retained_bytes{};
};

class Q4LocalAtlas;
using Q4LocalAtlasPtr = std::shared_ptr<const Q4LocalAtlas>;

// Immutable after factory success. One edge/index owner, exact leaf fragments,
// no compressed lower bound reused as an exact count. Construction budgets
// end center refinement ONLY. Intermediate Z budgets retain all unresolved
// blocks; terminal classification then finishes once, with its full cost paid.
// Independent calls may share this atlas, never their mutable work/buffers.
class Q4LocalAtlas final {
 public:
  [[nodiscard]] static Q4LocalAtlasPtr make(
      Q34EdgeCoverPtr cover, std::size_t kmax, Q4LocalOptions options);
  ~Q4LocalAtlas();
  Q4LocalAtlas(const Q4LocalAtlas&) = delete;
  Q4LocalAtlas& operator=(const Q4LocalAtlas&) = delete;
  Q4LocalAtlas(Q4LocalAtlas&&) = delete;
  Q4LocalAtlas& operator=(Q4LocalAtlas&&) = delete;
  [[nodiscard]] const Q4LocalGeometryPtr& geometry() const noexcept;
  [[nodiscard]] const Q4LocalAtlasWork& work() const noexcept;
  [[nodiscard]] std::size_t kmax() const noexcept;
  [[nodiscard]] const Q4LocalOptions& options() const noexcept;
  [[nodiscard]] std::size_t retained_bytes() const;
 private:
  struct Impl;
  explicit Q4LocalAtlas(std::unique_ptr<Impl> impl);
  std::unique_ptr<Impl> impl_;
  friend struct Q4LocalEngine;
  friend struct Q4SeedCellEngine;
};

struct Q4LocalSweepWork {
  u64 seed_queries{}, seed_owner_tests{}, seed_owner_rejections{};
  u64 query_visits{}, line_tests{}, line_skips{}, leaf_queries{};
  u64 reference_points{}, reference_side_tests{}, active_blocks{}, active_sites{};
  u64 root_locations{}, clipped_events{}, clipped_inside{}, kept_events{};
  u64 constant_inside{}, constant_outside{}, constant_shell_ids{}, entries{}, exits{};
  u64 sort_comparisons{}, shell_sort_comparisons{}, group_comparisons{}, groups{};
  u64 boundary_skips{}, boundary_skipped_ids{}, depth_rejections{}, depth_skipped_ids{};
  u64 presentations{}, owner_tests{}, owner_rejections{};
  u64 positive_tests{}, positive_rejections{}, canonical_tests{}, canonical_rejections{};
  u64 emitted{}, shell_ids{}, groups_without_support{}, unexamined_after_emit{};
  u64 max_group{}, peak_buffer_bytes{};
};

struct Q4LocalEdgeWork {
  Q4LocalAtlasWork atlas;
  Q4LocalGeometryWork geometry;
  Q4LocalSweepWork sweep;
  u64 node_visits{}, bound_tests{}, point_tests{}, rejected_nodes{}, split_nodes{};
  u64 rejected_sites{}, acute_seeds{}, owner_tests{}, owner_rejections{}, seeds{};
  u64 peak_live_buffer_bytes{};
};

// Q4 ONLY, not q3 or FULL. One valid acute seed, same owner/canonical and
// candidate-view contract as q34_cover; positive owner certification is
// required before a covered depth/shell is promoted to global. A callback
// exception leaves prior outputs emitted, no partial successful work report.
[[nodiscard]] Q4LocalSweepWork run_q4_local_seed_candidates(
    Q4LocalAtlasPtr atlas, std::size_t x_id, const Q34SeedConsumer& consumer);

// Builds one atlas and streams every acute owner seed of this supplied edge.
// No pool26 prefix, no q2/q3 acceptance gate. Each local sweep reads all active
// sites; clipping removes outside events from the sort, not from the count.
// Counts are unsaturated because exits later decrease them. No general bound
// on sum(seed/leaf active sites), all edges, output size or G4 is implied.
[[nodiscard]] Q4LocalEdgeWork run_q4_local_edge_candidates(
    Q34EdgeCoverPtr cover, std::size_t kmax, Q4LocalOptions options,
    const Q34SeedConsumer& consumer);

}  // namespace mhgp8
