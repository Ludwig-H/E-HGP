#pragma once

#include "wspd_q2_parallel.hpp"

namespace mhgp9::gen {

struct WspdQ2BatchOptions {
  std::size_t jobs_per_worker = 16;
  std::size_t lanes = 16;
  std::size_t quantum = 1;
};

struct Q2BatchWork {
  u64 enqueued{}, completed{}, completed_accepted{}, completed_rejected{};
  u64 batch_passes{}, lane_visits{}, transitions{};
  u64 entry_steps{}, witness_steps{}, admission_steps{}, payload_steps{};
  u64 sibling_due{}, entry_after_credit{}, entry_inside_deferred{};
  u64 key_preparations{}, sibling_bound_tests{}, sibling_rejected{}, sibling_rejected_after_credit{};
  u64 full_drains{}, seed_flushes{}, pool_flushes{}, max_active{};
};

struct Q2BatchWorkerWork {
  Q2BatchWork work;
  u64 state_capacity{}, state_storage_bytes{};
};

struct WspdQ2BatchResult {
  WspdQ2ParallelResult pipeline;
  Q2BatchWork work;
  std::vector<Q2BatchWorkerWork> workers;
  u64 state_bytes{}, state_capacity_sum{}, state_storage_bytes_sum{};
};

// Explicit Coarse-only q2 entry; old APIs/defaults are unchanged. SharedBlocks
// and Individual are fixed. Actual workers=min(requested workers, seeds),
// as in Coarse, and W1 executes inline. There is no range/branch donation,
// continuation dispatcher or GPU execution, and this is not an HGP FULL tower.
//
// Each worker allocates ONE reusable vector of singleton states and shares
// its existing engine's payload buffers. States contain the exact pair key,
// anchor rank, original B, cursor/count/phase, due sibling and Entry/Witness/
// Emit stage. Their index is owned once by the batch; no node/context pointer
// borrowed from a returning stack frame survives. There is no B stack or
// state allocation per query; shared payload vectors may still grow. Only
// private shared_task descendants may import an
// inherited prefix: the public API accepts no forgeable witness credit.
//
// A singleton is submitted BEFORE its query entry is charged. Entry pays
// query_tasks and its due sibling certificate once, Witness follows the SAME
// original Z ordering and decisions, and Emit uses the already computed key
// to collect every interior/shell ID and invoke the callback atomically.
// Parent roots/descriptors/candidate masses and query-split counters are not
// replayed. Reordering independent pairs changes neither geometry nor global
// work; it may change callback order, which was already unspecified.
//
// Full storage drives round-robin passes until a slot is free; no input is
// dropped. Each pass advances every currently active lane by up to quantum
// transitions. Lanes/quantum regulate scheduling, not search/output size.
// Seed completion flushes every pending singleton BEFORE the seed is counted
// completed. Before every Pool-selected rectangle the batch is flushed, and
// that whole Pool route, including Shared passthrough, remains synchronous.
// Thus no Pool permutation or borrowed parent enters the batch, and old Pool
// clocks retain their synchronous meaning. No flush occurs merely because
// one non-Pool rectangle ended: tiny rectangles can fill a useful batch.
//
// The current synchronous singleton path ALREADY has no allocation per
// query and no Z stack. This entry tests interleaved memory accesses and
// singleton-specialized control, not a removed historical 49-frame (u16) cost.
// The whole collect+callback remains atomic; no bound on wall time, shell
// size or cancellation latency follows. An exception discards unfinished
// private obligations only as part of a FAILED whole call, never a successful
// partial result. Earlier callbacks remain visible; all workers are joined.
// Per-slot callbacks are serial; different slots may run concurrently and
// require disjoint/synchronized state. Support spans expire at callback exit.
//
// Work sums across workers except max_active, which is a maximum. On success
// enqueued=completed=completed_accepted+completed_rejected=entry_steps;
// payload_steps=completed_accepted; transitions=entry_steps+witness_steps+
// admission_steps+payload_steps. admission_steps=completed_accepted. A witness
// step is one topological/geometric action or phase switch; admission is a
// separate transition. lane_visits counts positive dispatches, not empty
// slot inspections. batch_passes counts passes over the current dense active
// prefix, even when a previous lane finishes and compacts that prefix.
// Seed/pool flush counters count calls, including empty no-ops; full_drains
// counts submissions which first found all lanes occupied.
// key_preparations=enqueued: no key is rebuilt during witnesses or emission.
// sibling_due counts actual proposals in Saturating mode only; the three
// sibling_* result counters describe SINGLETON certificates, not group tests.
//
// pipeline.queue_storage_bytes and dispatch sidecars remain zero. New state
// storage reports actual vector capacities; bytes exclude vector/batch object
// metadata, shared index/cloud, payload buffers, thread stacks and RSS.
// pipeline.total_ms includes seed partition, allocations, launches, all work,
// joins/reduction/destruction, not input/index preparation. Worker intervals
// are active Coarse intervals, not waits in a shared queue. Payload and Pool
// sums cannot be subtracted from wall time. No speed/complexity claim follows
// from an unchanged geometry count or a bounded per-worker lane buffer.
// front_proposals only widens the front's rejection heuristic
// (WspdFrontProposals); the census still starts every count from zero.
[[nodiscard]] WspdQ2BatchResult run_wspd_q2_census_batched(
    Q2CensusIndexPtr index, unsigned kmax, unsigned separation_s,
    WspdFrontMode front_mode, std::span<const Q2CensusConsumer> consumers,
    WspdQ2BatchOptions options = {},
    Q2SiblingMode sibling_mode = Q2SiblingMode::Disabled,
    Q2WitnessOrder witness_order = Q2WitnessOrder::GlobalDfs,
    std::size_t pool_min_factor = 0, WspdFrontProposals front_proposals = {});

}  // namespace mhgp9::gen
