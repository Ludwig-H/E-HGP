#pragma once

#include "lanes/edge_cover.hpp"
#include "lanes/q34_seed.hpp"

namespace mhgp9::gen {

struct Q34CoverSeedWork {
  Q34SeedWork seed;
  u64 site_reads{};  // One fused q3/q4 scan of the cover, not two scans.
  u64 q3_shell_sort_comparisons{}, q4_shell_sort_comparisons{};
  u64 peak_buffer_bytes{};  // Actual simultaneous vector capacities, not RSS.
};

struct Q34EdgeWork {
  u64 node_visits{}, bound_tests{}, point_tests{};
  // Rejections here are INTERNAL boxes only; nonacute/foreign-owner leaves
  // are included in point_tests. Unlike cover.work(), this is no site partition.
  u64 rejected_nodes{}, split_nodes{}, rejected_sites{};
  u64 acute_seeds{}, owner_tests{}, owner_rejections{}, seeds{};
  // Counter sums over seeds; capacities and family.max_group are MAXIMA.
  Q34CoverSeedWork covered;
};

// New positive-candidate contract, NOT a replacement for run_q4_family.
// At arbitrary roots the sweep counts only covered sites (a lower bound).
// Only after positive-support AND longest-edge certification is the entire
// closed ball inside the cover: emitted depths/shells are then GLOBAL/exact.
// Input x must be a strictly acute face vertex. Ownership, canonical q4 seed,
// thresholds and callback lifetime match run_q34_seed_candidates. Spatial
// ranks are converted to original IDs; both emitted shell parts are sorted.
// A q3 rejection never suppresses q4. K=0/null callback/cover are invalid.
// Work O(m log(1+m)), memory O(m), for this ONE seed, m=cover.site_count().
// Besides events, q3 and constant q4 shells need original-ID sorting: the
// O(m+e log(1+e)) bound of the original-ID whole-cloud scan does not apply.
[[nodiscard]] Q34CoverSeedWork run_q34_cover_seed_candidates(
    Q34EdgeCoverPtr cover, std::size_t x_id, std::size_t kmax,
    const Q34SeedConsumer& consumer);

// All acute longest-edge-owned seeds of ONE supplied edge, generated through
// exact index-box bounds (lens and exterior of its diameter ball). The same
// immutable cover is reused; no cloud/index/cover preparation per seed.
// No q2/q3 accepted-support gate is used. Seeds are streamed, not stored.
// O(V + sum_seed(m log(1+m))) plus callbacks; V<=2n-1. This does NOT bound
// seed count or the global WSPD pipeline subquadratically. Sequential for now.
// Exceptions leave prior callback outputs with the caller. Ownership of the
// cover keeps index/cloud alive throughout callbacks, including nested calls.
[[nodiscard]] Q34EdgeWork run_q34_edge_candidates(
    Q34EdgeCoverPtr cover, std::size_t kmax, const Q34SeedConsumer& consumer);

}  // namespace mhgp9::gen
