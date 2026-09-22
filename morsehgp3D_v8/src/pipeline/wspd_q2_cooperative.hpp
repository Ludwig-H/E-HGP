#pragma once

#include "q2_census_resume.hpp"
#include "wspd_q2_parallel.hpp"

namespace mhgp8 {

struct WspdQ2CooperativeOptions {
  std::size_t jobs_per_worker = 16;
  std::size_t queue_capacity = 8;
  std::size_t quantum = 256;
  std::size_t min_b_size = 64;
};

struct Q2CooperativeWork {
  u64 continued_anchors{}, continued_pairs{}, completed_pairs{};
  u64 fragments_started{}, completed_fragments{}, donations{};
  // Publication after every initial front seed has been claimed; some seeds
  // may still be running. This does not mean all front callbacks have ended.
  u64 donations_after_seeds_exhausted{};
  u64 offer_checks{}, offer_busy{}, offer_full{}, offer_no_waiter{}, offer_no_sibling{};
  u64 waits{}, wakes{}, max_queue_size{}, max_active_tasks{}, max_fragment_bytes{};
  Q2CensusResumeWork resume_work;
  Q2CensusDetachWork detach_work;
};

struct Q2CooperativeWorkerWork {
  Q2CooperativeWork work;
};

struct WspdQ2CooperativeResult {
  WspdQ2ParallelResult pipeline;
  Q2CooperativeWork work;
  std::vector<Q2CooperativeWorkerWork> workers;
};

// The complete q2-only front/census stream, NOT q3/q4 or an HGP FULL tower.
// Uses the existing Coarse front jobs exactly once and ONE persistent worker
// team. There is no front-DFS donation in this entry point. SharedBlocks and
// Individual are fixed, not options. The smaller rectangle factor supplies
// anchors; only unfiltered roots with B.size() >= min_b_size use continuations.
// Smaller roots and ALL Pool-selected rectangles (including no-filter Pool
// passthroughs) retain the existing synchronous route. Positive min_b_size is
// a method-selection threshold, not a limit on input/search/output size.
// Consequently 0 < pool_min_factor <= min_b_size disables continuations:
// every otherwise eligible B is handled by that synchronous Pool route.
//
// The census queue has priority over unclaimed front seeds. A worker owning
// a seed remains active throughout its synchronous callbacks and local anchor
// continuations; those roots do not count as a second active task. Queue
// entries are detached, unvisited B siblings with their OWN continuation,
// buffers and immutable original-B context. Only that exact index is shared.
// No old witness/root credit or front parent is replayed; no index is copied.
//
// After each unfinished quantum, a worker may offer one sibling if another
// worker slot is idle. Demand includes slots not yet started. Full/busy queue
// or no demand means continuing locally; no producer waits for queue space.
// At most queue_capacity + started_workers continuations remain live; the
// transient child is created while a free queue slot is held and fits that
// same bound. This excludes synchronous Pool/engine buffers, front seeds,
// index, callback/result objects, thread stacks and allocator metadata.
// A continuation currently reserves index_stack_frames (55) frames; quantum bounds transitions,
// not time: an entire payload collection/callback is one atomic transition.
// Pool, small census roots and front jobs are still synchronous and may delay
// cancellation. No claim of bounded wall-clock cancellation latency is made.
//
// consumers.size() is positive. All requested workers start when front jobs
// exist, even if there are fewer seeds, so idle slots can service the census
// queue. With no jobs, no workers start. A single worker executes inline and
// never polls offers. Each callback slot is serial; different slots may run
// concurrently and require disjoint or synchronized caller state. Callbacks
// are copied before launches; support spans last only until callback return.
// The owned index survives caller pointer reset. All workers join on success
// or failure; earlier callbacks are not rolled back and errors return no
// completed result. Invalid options/callbacks fail before any emission.
//
// Geometry and global masses in pipeline equal the Coarse synchronous path.
// Continuations do NOT charge candidate mass or input_descriptors again:
// the rectangle already owns those entries. Historical work stays where it
// was paid, so an individual worker's accepted/rejected mass need not be a
// partition of that worker's candidate mass. Only the global partition holds.
// pipeline.dispatch_work and each front dispatch sidecar stay zero; here
// pipeline.queue_storage_bytes measures the census unique_ptr queue instead.
// pipeline workers/jobs still count completed initial front seeds only.
//
// All work sums over workers except max_queue_size, max_active_tasks,
// max_fragment_bytes and resume_work.max_pending_tasks, which take maxima.
// continued_pairs sums INITIAL root masses; completed_pairs sums remaining
// candidate mass of EVERY completed root/child. At successful completion:
// continued_pairs=completed_pairs, fragments_started=completed_fragments=
// continued_anchors+donations and donations=detached_frames=imported_frames.
// offer_checks=offer_busy+offer_full+offer_no_waiter+detach_work.attempts;
// attempts=donations+offer_no_sibling. waits/wakes count CV predicate calls
// and returns, not internal spurious wakeups. max_active_tasks counts active
// seeds plus dequeued children, never nested root continuations separately.
//
// pipeline.total_ms encloses preparation of seeds/queue, launches, work,
// joins, reduction and private destruction (not input/index preprocessing).
// Worker elapsed intervals INCLUDE their waits; their sum and payload_ms_sum
// are not subtractable from wall time. Pool clocks remain synchronous sums.
// No new clock here isolates continuation allocation or detachment cost.
// front_proposals only widens the front's rejection heuristic
// (WspdFrontProposals); the census still starts every count from zero.
[[nodiscard]] WspdQ2CooperativeResult run_wspd_q2_census_cooperative(
    Q2CensusIndexPtr index, unsigned kmax, unsigned separation_s,
    WspdFrontMode front_mode, std::span<const Q2CensusConsumer> consumers,
    WspdQ2CooperativeOptions options = {},
    Q2SiblingMode sibling_mode = Q2SiblingMode::Disabled,
    Q2WitnessOrder witness_order = Q2WitnessOrder::GlobalDfs,
    std::size_t pool_min_factor = 0, WspdFrontProposals front_proposals = {});

}  // namespace mhgp8
