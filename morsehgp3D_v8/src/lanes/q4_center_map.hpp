#pragma once

#include "lanes/q34_collective.hpp"
#include "lanes/q4_positive_domain.hpp"

#include <memory>

namespace mhgp8 {

enum class Q4CenterDomainMode { Disk, Positive };
struct Q4CenterMapOptions {
  Q4CenterDomainMode domain{Q4CenterDomainMode::Positive};
  unsigned max_depth{7};
  std::size_t node_budget{4096};
};

// Depth <=44 is the arithmetic domain of this certificate, not a search
// truncation. Exhausted depth/nodes mean UNKNOWN and exact fallback.
void validate_q4_center_map_options(Q4CenterMapOptions options);

struct Q4CenterMapWork {
  Q4PositiveDomainWork domain;
  u64 preparations{}, prepared_forms{}, projection_points{}, hull_sort_comparisons{};
  u64 hull_orientation_tests{}, hull_vertices{}, facets{};
  u64 queries{}, seed_owner_tests{}, seed_owner_rejections{};
  u64 rejected_queries{}, unknown_queries{};
  u64 query_visits{}, line_tests{}, line_skips{};
  u64 cells_created{}, cells_evaluated{}, disk_tests{}, facet_tests{};
  u64 outside_cells{}, deep_cells{}, witness_tests{}, inside_credits{}, outside_witnesses{};
  u64 splits{}, compressed_deep{}, compressed_outside{};
  u64 pending_ids_copied{}, pending_lists_created{}, node_storage_growths{};
  u64 depth_stops{}, budget_stops{}, empty_stops{};
  u64 max_depth{}, peak_pending_bytes{}, peak_retained_bytes{};
};

// One exclusive mutable certificate cache for one immutable pool/edge and
// one K. Never share queries concurrently. No candidate/output is produced.
// Global OUT/DEEP states are independent of the queried line; a line miss is
// only a local query result, never cached as global OUT. Children not visited
// remain unknown. Saturated DEEP means >=K-2, NOT >=K-1 or a larger K.
// Query order may change refinement/work/rejections, never the exact fallback.
// An allocation/arithmetic failure during a query poisons this cache; previous
// decisions remain valid but it cannot be resumed. Invalid arguments do not.
class Q4CenterMap final {
 public:
  [[nodiscard]] static std::unique_ptr<Q4CenterMap> make(
      Q34WitnessPoolPtr pool, std::size_t kmax, Q4CenterMapOptions options);
  ~Q4CenterMap();
  Q4CenterMap(const Q4CenterMap&) = delete;
  Q4CenterMap& operator=(const Q4CenterMap&) = delete;
  Q4CenterMap(Q4CenterMap&&) = delete;
  Q4CenterMap& operator=(Q4CenterMap&&) = delete;
  [[nodiscard]] bool reject_seed(std::size_t x_id);
  [[nodiscard]] const Q4CenterMapWork& work() const noexcept;
  [[nodiscard]] const Q34WitnessPoolPtr& pool() const noexcept;
  // Dynamic capacities only; excludes fixed context/domain objects, allocator
  // metadata/transients, owned-but-shared cloud/index/cover/pool and callbacks.
  [[nodiscard]] std::size_t retained_bytes() const;
  // Peak dynamic capacities during the most recent successful query, including
  // its entry state, before/after splits. For coupling with live caller scratch.
  [[nodiscard]] std::size_t last_query_peak_bytes() const noexcept;

 private:
  struct Impl;
  explicit Q4CenterMap(std::unique_ptr<Impl> impl);
  std::unique_ptr<Impl> impl_;
};

struct Q34MappedSeedWork {
  Q34CollectiveSeedWork collective;
  Q4CenterMapWork map;
  u64 q3_only_after_map{}, both_rejected_by_map{};
  u64 peak_live_buffer_bytes{};  // Map+workspace+fallback at the same seed.
};
struct Q34MappedEdgeWork {
  Q34CollectiveEdgeWork collective;
  Q4CenterMapWork map;
  u64 q3_only_after_map{}, both_rejected_by_map{};
  u64 peak_live_buffer_bytes{};
};

// Explicit composition: the requested pool filter runs first. Only q4 still
// alive may query/refine the shared map. q3 remains independent, all fallback
// counts start at zero. Map construction is deferred to the first live q4;
// budget0 bypasses it entirely. Same synchronous candidate-view contract.
[[nodiscard]] Q34MappedSeedWork run_q34_mapped_seed_candidates(
    Q34WitnessPoolPtr pool, std::size_t x_id, std::size_t kmax,
    Q34PoolOptions filter, Q4CenterMapOptions map, const Q34SeedConsumer& consumer);
[[nodiscard]] Q34MappedEdgeWork run_q34_mapped_edge_candidates(
    Q34WitnessPoolPtr pool, std::size_t kmax,
    Q34PoolOptions filter, Q4CenterMapOptions map, const Q34SeedConsumer& consumer);

}  // namespace mhgp8
