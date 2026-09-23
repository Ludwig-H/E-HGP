#pragma once

#include "lanes/q4_local.hpp"
#include "lanes/q4_seed_cells.hpp"
#include "lanes/q4_window.hpp"
#include "lanes/q34_witness_search.hpp"
#include "lanes/q3_ball_census.hpp"
#include "wspd/front.hpp"

#include <functional>
#include <vector>

namespace mhgp9::gen {

enum class WspdQ4Backend { Local28, Window30 };
enum class WspdQ34WitnessMode { Disabled, Pair, RectanglePair };
enum class WspdQ3CensusMode { ScalarCover, GlobalBoxes };

struct WspdQ34Options {
  WspdFrontMode front_mode{WspdFrontMode::MidpointSamples};
  std::uint8_t requested_lane_mask{6};  // Exactly 2=q3, 4=q4 or 6=both.
  WspdQ4Backend q4_backend{WspdQ4Backend::Local28};
  Q4LocalOptions local{};
  WspdQ34WitnessMode witness_mode{WspdQ34WitnessMode::Disabled};
  WspdQ3CensusMode q3_census_mode{WspdQ3CensusMode::ScalarCover};
  Q34WitnessBoundsMode witness_bounds_mode{Q34WitnessBoundsMode::Legacy};
  Q4SeedCellOptions q4_seed_cells{};
  // Explicit option: when both lanes are active on Local28 (K>=3), build the
  // q4 center atlas BEFORE the q3 lane and reject without census every q3
  // seed whose circumcenter lies in a cell certified with >=K-1 cover sites
  // strictly inside all its balls; a root certificate skips the q3 lane.
  // Exact (the cover holds every site inside such balls; certificates hold
  // on closed cells); the q4 sweep then reuses the same atlas. Off keeps the
  // historical order and every counter bit-identical.
  bool q3_atlas_consultation{false};
  // v9 option (requires q3_atlas_consultation): a q3 seed whose centre lies in
  // an EXACT atlas leaf is censused from that leaf (certified count + frontier
  // sites) instead of the global index. Same depth and shell; off keeps the
  // v8 path and every historical counter.
  bool q3_leaf_census{false};
  // Parallel entry only: every surviving residual rectangle is published to
  // the team's bounded task queue, by ranges of a-ranks when its pair mass
  // exceeds this grain, so that any idle worker expands it; the discovering
  // worker expands inline only what the full queue refuses. Whole edges are
  // never split. Zero disables sharing (historical Coarse jobs only).
  std::size_t parallel_task_pairs{256};
  // Pending tasks admitted before publishers fall back to inline expansion
  // (bounded memory, never a search or output quota).
  std::size_t parallel_queue_capacity{4096};
};

struct WspdQ3AtlasWork {
  u64 edges_with_atlas{}, root_lane_skips{}, locations{}, outside_domain{}, rejections{};
  // v9: q3 census started from the exact atlas leaf containing the centre
  // (certified count + the leaf frontier only) instead of the global index.
  u64 leaf_censuses{}, leaf_point_tests{}, leaf_rejections{}, lower_bound_fallbacks{};
  bool operator==(const WspdQ3AtlasWork&) const = default;
};

struct WspdQ34WitnessWork {
  // Input union mass, then whole-product and individual-pair rejections.
  // Lane masses overlap; they must NEVER be summed as a union mass.
  u64 input_pair_mass{}, rejected_rectangles{}, rectangle_pair_mass{};
  u64 rectangle_q3_pairs{}, rectangle_q4_pairs{};
  u64 rejected_pairs{}, pair_q3_pairs{}, pair_q4_pairs{};
  Q34WitnessSearchWork rectangles, pairs;
  Q34WitnessBoundsWork rectangles_bounds, pairs_bounds;
  bool operator==(const WspdQ34WitnessWork&) const = default;
};

struct WspdQ3Work {
  u64 edge_queries{};
  u64 seed_node_visits{}, seed_bound_tests{}, seed_point_tests{};
  u64 seed_rejected_nodes{}, seed_split_nodes{}, seed_rejected_sites{};
  u64 acute_seeds{}, owner_tests{}, owner_rejections{}, seeds{}, ball_builds{};
  u64 census_range_visits{}, census_point_tests{};
  u64 census_inside_sites{}, census_outside_sites{}, census_shell_sites{};
  u64 depth_rejections{}, early_unread_sites{};
  u64 shell_sort_comparisons{}, shell_ids{}, emitted{}, peak_shell_bytes{};
  bool operator==(const WspdQ3Work&) const = default;
};

struct WspdQ34Work {
  u64 input_rectangles{}, expanded_pairs{}, q3_edges{}, q4_edges{}, both_edges{};
  u64 cover_builds{}, cover_sites{}, max_cover_sites{}, peak_cover_bytes{};
  u64 q3_emitted{}, q4_emitted{}, payload_shell_ids{};
  // Cover + reused q3 shell + the current q4 edge's own capacity peak.
  // Excludes front stack, shared cloud/index, fixed objects, allocator
  // transients and all storage/work added by the consumer. Not RSS.
  u64 peak_edge_buffer_bytes{};
  Q34EdgeCoverWork cover;
  WspdQ3Work q3;
  // Only the selected backend runs. Work is summed across edges, except
  // capacities, maxima and retained_bytes fields, which combine by MAX.
  Q4LocalEdgeWork local;
  Q4WindowEdgeWork window;
  WspdQ34WitnessWork witness;
  Q3BallCensusWork q3_blocks;
  Q4SeedCellWork q4_seed_cells;
  WspdQ3AtlasWork q3_atlas;
};

struct WspdQ34Result {
  WspdFrontResult front;
  WspdQ34Work work;
};

// Global q3/q4 CANDIDATE STREAM over this immutable cloud/index, not a
// deduplicated ball catalogue, interior-ID payload, HGP forest or FULL tower.
// The WSPD covers every active lane independently. Each residual rectangle
// is explicitly expanded once; its original-ID edge builds ONE cover shared
// by q3 and q4. No accepted q2 or q3 gate gives access to another lane, and
// no front witness credit seeds a census. The q2-only front proposal options
// are deliberately not exposed here: the qualified multilanes use defaults.
//
// q3-only: exact box-generated acute owner seeds, then a saturating covered
// census at K-1, collecting/sorting the complete shell on acceptance. Its
// buffer is private and reused between seeds AND edges. There is no hidden
// q4 sweep in this q3 branch. q4: unchanged Local28 by default, Window30 only
// when explicitly selected, at depth<K-2 with their canonical support rules.
// Positive/owner certification transfers every published depth/shell to the
// full cloud. Different presentations/edges may still emit the same ball.
//
// K must be in 1..10, s positive, mask2/4/6, valid enums/local options and
// nonnull index/consumer. Validation precedes any output, including when no
// requested lane is available. K1 (or K2 with q4-only) returns an empty stream
// and zero work, with total_unordered_pairs metadata still checked for u64.
// This bound on K is the existing front contract, not a search/output quota.
//
// Sequential callbacks borrow candidate/spans for the callback duration only.
// The index is owned throughout even if the caller resets its handle. Calls
// have independent mutable engines; nested calls require no shared state.
// Allocation/counter/callback exceptions propagate; earlier emissions remain
// with the caller and no successful partial ledger is returned.
//
// Total work includes the front, sum_rect |A||B| explicitly expanded edges,
// cover preparation per edge, seed generation, q3 sum_seed cover reads/shell
// sorts, and the selected q4 backend. cover_sites is their paid Fcover sum,
// not a distinct-site count. No global subquadratic or performance guarantee
// follows. No complete edge/face list, cloud copy or per-edge index is built.
//
// Explicit witness modes first search the SAME index for strict universal
// citron witnesses, independently at K-1/K-2. RectanglePair searches products
// before expansion; both modes search remaining pairs BEFORE building covers.
// Partial counts are discarded, never used to seed another search or census.
// expanded_pairs counts pairs entered after whole-product filtering;
// q3_edges/q4_edges/both_edges and cover_builds count only surviving lanes.
// The separate witness ledger pays every search and partitions input masses.
// GlobalBoxes is a separate explicit q3 option: census on the global index,
// saturating strict interior first, complete shell only on acceptance. No
// witness credit seeds it. q3.census_* and early_unread_sites then stay zero;
// q3_blocks reports the actual work, including precomputed unvisited bounds.
// Exclusion adds a certified lower-Xi exclusion of WITNESS nodes; Affine
// additionally specializes H/Xi when both endpoint boxes are singletons.
// Such exclusions remove only local DFS lanes and supply ZERO credits:
// they never reject an edge/lane without its separate saturation proof.
// Legacy preserves the previous traversal; the new bounds ledgers stay zero.
// No witness-state inheritance between rectangles/pairs is introduced here.
// Explicit Local28-only LiveOnly/Joined seed-cell traversals keep every live
// incidence/contact and use private bounded caches, not accepted-q3 gates.
// Their additional work is separate; Individual keeps the historical path.
// Non-Individual + Window30, zero block_sites and invalid modes are rejected
// before output, including inactive lanes. This does not split an edge across
// workers: only the traversal within its synchronous call changes.
[[nodiscard]] WspdQ34Result run_wspd_q34_candidates(
    Q2CensusIndexPtr index, unsigned kmax, unsigned separation_s,
    WspdQ34Options options, const Q34SeedConsumer& consumer);

using WspdQ34ParallelConsumer =
    std::function<void(std::size_t, const Q34SeedCandidate&)>;

struct WspdQ34WorkerWork {
  u64 jobs{}, front_products{}, input_rectangles{}, expanded_pairs{};
  u64 q3_emitted{}, q4_emitted{}, peak_edge_buffer_bytes{};
  bool operator==(const WspdQ34WorkerWork&) const = default;
};

struct WspdQ34ParallelWork {
  u64 requested_workers{}, started_workers{}, target_jobs{}, jobs{};
  u64 completed_jobs{}, terminal_jobs{}, prefix_product_visits{};
  u64 job_storage_bytes{}, worker_state_bytes{}, callback_storage_bytes{};
  // SUM of private worker peaks, an upper bound, not a simultaneous peak.
  // Fixed engines/front stacks, callback target allocations, shared cloud,
  // allocator/thread metadata and caller-owned payload/RSS are excluded.
  u64 edge_buffer_bytes_sum{};
  bool operator==(const WspdQ34ParallelWork&) const = default;
};

// Rectangle-range task sharing between the joined workers (SUM fields, plus
// maxima). published = ranges handed to the queue; consumed = ranges taken
// from it (by any worker, the publisher included); task_pairs = their pair
// mass; peak_queue = simultaneously pending tasks; waits = idle waits.
struct WspdQ34TaskWork {
  u64 split_rectangles{}, published{}, consumed{}, task_pairs{}, peak_queue{}, waits{}, refused{};
  bool operator==(const WspdQ34TaskWork&) const = default;
};

// Measured, never logical: wall time of the slot's loop, CPU time of its
// thread (CLOCK_THREAD_CPUTIME_ID) and wall time spent waiting on the task
// queue. Nanoseconds; they vary between runs and are never compared.
struct WspdQ34WorkerTiming {
  u64 wall_ns{}, cpu_ns{}, wait_ns{};
};

struct WspdQ34ParallelResult {
  WspdQ34Result pipeline;
  WspdQ34ParallelWork parallel;
  std::vector<WspdQ34WorkerWork> workers;
  WspdQ34TaskWork tasks;
  std::vector<u64> worker_tasks;  // Ranges consumed per started slot.
  std::vector<WspdQ34WorkerTiming> worker_timings;
};

// Explicit Coarse front-job entry; the old mono entry/default is unchanged.
// One joined team, one engine/reused shell per started worker, one immutable
// index and job plan. A rectangle/edge/local28/window30 call is never split.
// jobs_per_worker controls initial job granularity, never search/output size.
// W1 follows the same jobs path. started_workers=min(requested_workers,jobs).
// Inactive lanes return metadata only, no job plan or workers; both positive
// worker_count/jobs_per_worker and their product are still validated first.
//
// Each started slot receives its own COPY of the callback before any worker
// starts. Callbacks within one slot are sequential; distinct slots may call
// concurrently and order is unspecified. Captured references/shared objects
// still need caller synchronization. The borrowed-view lifetime is unchanged.
// Index/job ownership extends through every join, even after caller reset.
// Allocation, callback, counter or launch failure cancels acquisition of new
// jobs, lets already running jobs finish, joins all started threads, then
// propagates. Partial emissions remain; no successful partial result exists.
//
// Front prefix + jobs and additive geometric counters equal the mono work.
// Reductions use SUM/MAX exactly as above; shell capacity and its coupled
// edge-buffer maximum may differ with worker assignment and are NOT logical
// work invariants. Orchestration/capacity sums are reported separately.
[[nodiscard]] WspdQ34ParallelResult run_wspd_q34_parallel(
    Q2CensusIndexPtr index, unsigned kmax, unsigned separation_s,
    WspdQ34Options options, std::size_t worker_count,
    const WspdQ34ParallelConsumer& consumer, std::size_t jobs_per_worker = 16);

}  // namespace mhgp9::gen
