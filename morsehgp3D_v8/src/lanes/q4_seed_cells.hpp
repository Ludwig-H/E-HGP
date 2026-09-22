#pragma once

#include "lanes/q4_local.hpp"

namespace mhgp8 {

enum class Q4SeedCellMode { Individual, LiveOnly, Joined };

struct Q4SeedCellOptions {
  Q4SeedCellMode mode{Q4SeedCellMode::Individual};
  // Positive memory grain, not a search quota. Every surviving spatial
  // subtree is processed; no seed, incidence, event or shell is truncated.
  std::size_t block_sites{64};
};

struct Q4SeedCellWork {
  // SUM: valid, active, non-Individual edge calls only.
  u64 queries{};
  u64 live_preparations{}, live_node_visits{}, live_child_reads{}, live_leaves{};
  u64 whole_atlas_skips{}, live_skipped_nodes{};
  u64 antichain_node_visits{}, antichain_splits{}, blocks{}, block_sites{};
  u64 cache_entries_initialized{}, cache_hits{}, cache_misses{}, invalid_cache_hits{};
  u64 family_preparations{}, family_cache_hits{}, form_preparations{};
  u64 product_visits{}, product_seed_rejections{};
  u64 positive_products{}, negative_products{}, uncertain_products{}, zero_bound_products{};
  u64 block_bound_tests{}, singleton_bound_tests{}, spatial_tests_reused{};
  u64 x_splits{}, cell_splits{}, terminal_pairs{};
  // MAX: all seven fields below. Capacity peaks include simultaneous old
  // and replacement cache storage during explicit cache growth. Additional
  // fixed product-stack backing is included, not just its logical fill.
  u64 max_block_sites{}, peak_cache_bytes{}, peak_live_bytes{}, peak_product_stack{};
  u64 product_stack_bytes{}, peak_auxiliary_bytes{}, peak_total_buffer_bytes{};
  bool operator==(const Q4SeedCellWork&) const = default;
};

// Explicit traversal choice for q4 ONLY. The historical signature/default
// stays unchanged. Individual validates these extra options, then delegates
// directly to that entry and leaves the supplied extra ledger UNCHANGED.
// K1/K2 are inactive after full validation and also leave it unchanged.
// Invalid arguments change neither ledger nor emit callbacks.
//
// Both non-Individual modes first test atlas.work().leaf_cells==0 in O(1):
// a dead atlas needs no summary or seed generation, and its construction
// memory/work is still paid. LiveOnly otherwise prepares exact counts of live
// atlas leaves once, pruning branches with none. Joined also streams disjoint spatial cache blocks, then descends
// products of seed nodes and CLOSED atlas cells. Strict positive/negative
// bounds alone discard products; zero/contact remains. A terminal incidence
// calls the existing fragment sweep DIRECTLY, never restarts at the root.
// All Positive-domain geometry still uses every original lens completion.
// No q3 consequence, witness removal, inherited census credit or FULL claim.
//
// Parents (atlas, geometry, cover, original index/cloud) remain immutable and
// owned for the whole synchronous call. Live summary/cache/buffers are private
// to it. Each seed belongs to one cache block, including cached rejections;
// at most one family is prepared per seed/arête. Callbacks borrow their spans
// exactly as in q4_local. Failure propagates with prior emissions intact and
// possibly partial extra work, but without a successful returned work report.
//
// Existing returned generator fields count ACTUAL tests. In Joined, repeated
// spatial tests/rejected_sites are node/site INCIDENCES, not unique sites;
// edge.seeds counts distinct valid seeds actually tested, while
// sweep.seed_queries=family_preparations<=edge.seeds. In LiveOnly they stay
// equal. Joined navigation lives in this extra ledger: the old sweep's
// query_visits/line_tests/line_skips and seed_owner_tests remain zero.
// Its leaf_queries and all subsequent sweep/payload counts retain their exact
// meanings. Old peak_live_buffer_bytes includes the new buffers in these modes.
//
// Completed non-Individual calls: queries = live_preparations + whole_atlas_skips.
// live_node_visits/live_child_reads count only nonempty atlases whose summary
// was actually constructed. Whole-atlas skips are a LiveOnly control gain,
// not a reduction attributed to Joined's product bounds.
//
// Joined: product_visits = live_skipped_nodes + product_seed_rejections +
// positive_products + negative_products + uncertain_products;
// uncertain_products = x_splits + cell_splits + terminal_pairs;
// terminal_pairs = family_preparations + family_cache_hits = sweep.leaf_queries.
// cache_misses = returned point_tests; cache_hits include invalid_cache_hits.
// returned node_visits = bound_tests + point_tests, and returned split_nodes
// = antichain_splits + x_splits. Unlike the Individual tree traversal,
// bound_tests need NOT equal rejected_nodes+split_nodes: one paid spatial
// test can be carried through several center splits, or require no X split.
// A carried spatial certificate avoids repeating its test after a C split,
// counted in spatial_tests_reused, not as a newly executed bound.
//
// Work includes atlas preparation, live summary, block/cache preparation,
// every product test, repeated spatial filtering, scans/sorts and callbacks.
// Stack<=187 descriptors follows the index height max_index_depth (54) and cell depth44;
// it is NOT a work bound. No subquadratic guarantee over all edges/regimes.
// Memory excludes shared point/index/cover storage, object/control metadata,
// standard-library sort stack and callback allocations; it is not RSS.
[[nodiscard]] Q4LocalEdgeWork run_q4_local_edge_candidates(
    Q34EdgeCoverPtr cover, std::size_t kmax, Q4LocalOptions local_options,
    const Q34SeedConsumer& consumer, Q4SeedCellOptions seed_options,
    Q4SeedCellWork& work);
// Same traversal from an atlas already built for this edge (shared with the
// q3 seed certificates of the same edge); Individual delegates to the local
// atlas-based entry. Work reports the supplied atlas and its geometry.
[[nodiscard]] Q4LocalEdgeWork run_q4_local_edge_candidates(
    Q4LocalAtlasPtr atlas, const Q34SeedConsumer& consumer,
    Q4SeedCellOptions seed_options, Q4SeedCellWork& work);

}  // namespace mhgp8
