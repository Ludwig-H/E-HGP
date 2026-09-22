#pragma once

#include "q2_census_resume.hpp"

#include <span>
#include <vector>

namespace mhgp9::gen {

struct Q2CensusParallelOptions {
  std::size_t workers = 1;
  std::size_t quantum = 256;
  std::size_t queue_capacity = 8;
};

struct Q2CensusScheduleWork {
  u64 offer_checks{}, offer_busy{}, offer_full{}, offer_no_sibling{};
  u64 donations{}, fragments_started{}, fragments_completed{}, waits{}, wakes{};
  u64 max_queue_size{}, max_active_fragments{};
};

struct Q2CensusWorkerResult {
  Q2CensusResumeSnapshot sum;
  Q2CensusScheduleWork schedule;
  // Maximum per-fragment vector capacity observed at creation and each pause,
  // not simultaneous RSS. Index, thread stacks and metadata are excluded.
  u64 max_fragment_bytes{};
};

struct Q2CensusParallelResult {
  Q2CensusResumeSnapshot sum;
  Q2CensusScheduleWork schedule;
  std::vector<Q2CensusWorkerResult> workers;
  u64 queue_storage_bytes{};
  u64 max_fragment_bytes{};
  // Encloses root creation, all launches, work, joins, reduction and private
  // continuation destruction. The returned result storage remains alive.
  double total_ms{};
};

// ONE anchor against one B node. Uses the exact same search as the recursive
// reference; only unvisited pending B siblings can be donated. Not the WSPD
// front, Pool, joint census or a FULL tower. Options change scheduling, never
// the amount of search. A positive quantum does not bound callback duration.
//
// Exactly options.workers nonempty callbacks, one private slot per worker.
// A slot is never called concurrently with itself; distinct slots can run
// concurrently and must not share mutable state without synchronization.
// Callback spans are borrowed only until return. All threads are joined on
// success and failure, including partial launch failure. Earlier emissions
// are NOT rolled back on failure. No completed result is returned on error.
//
// Queue space is preallocated; try-lock/full means continue locally. A donor
// is detached only AFTER acquiring a free queue slot. No search waits for
// space, no obligation is dropped, and no parent is replayed. The queue owns
// the child's full continuation (currently 6272 stack bytes plus metadata),
// not a compact GPU descriptor. At most queue_capacity + workers objects
// remain live, except the one being constructed while a free slot is held;
// that construction still fits this same bound. No per-anchor bulk allocation.
//
// sum.census clocks are SUMS of active fragment intervals, NOT wall time.
// Schedule counters and resume pauses/peaks are order-dependent. Geometric
// counters, support payloads and four step totals add to the mono reference.
[[nodiscard]] Q2CensusParallelResult run_q2_anchor_parallel(
    Q2CensusIndexPtr index, std::size_t anchor_rank, std::size_t b_node,
    unsigned kmax, Q2CensusParallelOptions options,
    std::span<const Q2CensusConsumer> consumers,
    Q2SiblingMode sibling_mode = Q2SiblingMode::Disabled,
    Q2WitnessOrder witness_order = Q2WitnessOrder::GlobalDfs);

}  // namespace mhgp9::gen
