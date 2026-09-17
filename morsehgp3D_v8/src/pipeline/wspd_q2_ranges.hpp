#pragma once

#include "wspd_q2_parallel.hpp"

namespace mhgp8 {

struct WspdQ2RangeOptions {
  std::size_t jobs_per_worker = 16;
  std::size_t queue_capacity = 8;
  std::size_t anchor_grain = 64;
};

struct Q2RangeWork {
  u64 initial_ranges{}, completed_ranges{}, received_ranges{};
  u64 initial_anchors{}, completed_anchors{}, initial_pairs{}, completed_pairs{};
  u64 initial_shared_ranges{}, initial_pool_ranges{}, initial_passthrough_ranges{};
  u64 donations{}, donated_anchors{}, donated_pairs{}, donations_after_seeds_exhausted{};
  u64 shared_donations{}, pool_donations{}, passthrough_donations{};
  u64 offer_checks{}, offer_busy{}, offer_full{}, offer_no_waiter{};
  u64 waits{}, wakes{}, max_queue_size{}, max_active_tasks{};
};

struct Q2RangeWorkerWork { Q2RangeWork work; };

struct WspdQ2RangeResult {
  WspdQ2ParallelResult pipeline;
  Q2RangeWork work;
  std::vector<Q2RangeWorkerWork> workers;
  u64 range_task_bytes{};
  // Maxima of distinct registered Pool parents. Registration occurs after
  // full construction, withdrawal at destructor entry before members die.
  // Bytes include each parent object plus its retained vector capacities,
  // once, not once per reference. These measured intervals exclude creation
  // and destruction tails, shared_ptr control blocks, temporary construction
  // storage, index/cloud, engines and process RSS.
  u64 max_live_pool_parents{}, max_live_pool_bytes{};
};

// Explicit alternative to Coarse, not a changed default and not HGP FULL.
// SharedBlocks/Individual are fixed. One persistent team owns Coarse front
// seeds and a preallocated VALUE queue of UNSTARTED anchor ranges. Each
// worker reuses its private census engine and buffers. There are no owned
// census continuations, B-branch donation, or new index/coordinate copies.
//
// A shared range carries spatial anchor ranks and its original global B.
// A filtered Pool range carries positions in one immutable parent's grouped
// A permutation and one B prefix. The parent owns the index before its
// noncopyable/nonmovable plan; it is prepared ONCE per selected rectangle.
// No Pool credits seed census counts. A Pool plan filtering no pair is
// released before processing, and its spatial anchors retain Shared census.
// Never interpret a Pool prefix as a global B node or prepare it per chunk.
//
// An eligible range has more than anchor_grain unstarted anchors. With W>1
// it polls once at entry, then after every anchor_grain completed anchors,
// only while eligible. Each successful offer transfers the last floor(m/2)
// of its m remaining anchors. Child construction precedes narrowing of the
// donor; publication and narrowing do not allocate or throw. Busy/full queue
// or no idle worker slot means local continuation, never waiting for space.
// Grain is scheduling granularity, not an input/search/output limit. An
// entire anchor (including all selected Pool pairs and callbacks) is atomic
// with respect to this scheduler; no cancellation latency bound is claimed.
//
// Seeds and queued ranges share one termination condition: every seed has
// been CLAIMED, queue empty, active count zero. A seed stays active through
// its callbacks and local ranges; those ranges add no nested active count.
// All W workers start if any seeds exist, even W>seeds. With no seeds none
// start; W1 executes inline without offers. Callback slots are serial but
// may run concurrently across slots. Views expire on callback return; the
// owned index survives reset of caller pointers. All threads join on success
// or error; earlier emissions are not rolled back. Do not recursively enter
// the same dispatcher from a callback; a new independent call is supported.
//
// Rectangle candidate mass/descriptors and Pool bands/preparation are charged
// once by the parent. Consuming workers charge actual roots, geometry,
// selected Pool anchors/pairs and payloads. Only GLOBAL candidate mass is a
// partition: per-worker candidates need not cover its accepted/rejected work.
// Geometry and old integer counters/logical maxima equal the Coarse path.
// All Q2RangeWork fields sum over workers except the two maxima. On success:
// initial_ranges=shared+pool+passthrough initial ranges;
// completed_ranges=initial_ranges+donations; received_ranges=donations;
// donations=shared_donations+pool_donations+passthrough_donations;
// initial_anchors=completed_anchors; initial_pairs=completed_pairs=candidates;
// offer_checks=offer_busy+offer_full+offer_no_waiter+donations; waits=wakes.
// Donated anchor/pair masses count transfers (the same future work can move
// repeatedly); they are not additional candidates or executed census work.
// donations_after_seeds_exhausted means claimed seeds, not completed seeds.
//
// pipeline.queue_storage_bytes is actual allocated range-value capacity.
// At most Q+W DISTINCT Pool parents are live; a parent may be shared by many
// queued ranges. pipeline.pool_work.plan_peak_bytes is the old per-plan
// vector-capacity maximum, and pool_peak_bytes_sum remains the sum of each
// creator worker's maximum: NOT a simultaneous shared-plan upper bound.
// Dispatch sidecars remain zero (no front DFS donation in this entry).
//
// total_ms encloses seeds/queue, team, all work, joining/reduction/destruction,
// not preprocessing. Worker times include waiting. Pool selected_total_ms
// is preparation plus active local range-processing intervals, including
// offers/callbacks, NOT an enclosing asynchronous parent wall interval.
// These sums cannot be subtracted from total wall time. All old APIs retain
// their previous clock semantics and defaults.
// front_proposals only widens the front's rejection heuristic
// (WspdFrontProposals); the census still starts every count from zero.
[[nodiscard]] WspdQ2RangeResult run_wspd_q2_census_ranges(
    Q2CensusIndexPtr index, unsigned kmax, unsigned separation_s,
    WspdFrontMode front_mode, std::span<const Q2CensusConsumer> consumers,
    WspdQ2RangeOptions options = {},
    Q2SiblingMode sibling_mode = Q2SiblingMode::Disabled,
    Q2WitnessOrder witness_order = Q2WitnessOrder::GlobalDfs,
    std::size_t pool_min_factor = 0, WspdFrontProposals front_proposals = {});

}  // namespace mhgp8
