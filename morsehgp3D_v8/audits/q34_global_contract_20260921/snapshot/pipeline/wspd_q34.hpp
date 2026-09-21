#pragma once

#include "lanes/q4_local.hpp"
#include "lanes/q4_window.hpp"
#include "wspd/front.hpp"

#include <functional>
#include <vector>

namespace mhgp8 {

enum class WspdQ4Backend { Local28, Window30 };

struct WspdQ34Options {
  WspdFrontMode front_mode{WspdFrontMode::MidpointSamples};
  std::uint8_t requested_lane_mask{6};  // Exactly 2=q3, 4=q4 or 6=both.
  WspdQ4Backend q4_backend{WspdQ4Backend::Local28};
  Q4LocalOptions local{};
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

struct WspdQ34ParallelResult {
  WspdQ34Result pipeline;
  WspdQ34ParallelWork parallel;
  std::vector<WspdQ34WorkerWork> workers;
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

}  // namespace mhgp8
