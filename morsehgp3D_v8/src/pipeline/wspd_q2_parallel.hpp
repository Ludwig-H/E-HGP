#pragma once

#include "wspd_q2_census.hpp"

#include <span>
#include <vector>

namespace mhgp8 {

enum class WspdQ2ScheduleMode { Coarse, Donate };

struct WspdQ2Schedule {
  WspdQ2ScheduleMode mode = WspdQ2ScheduleMode::Coarse;
  std::size_t queue_capacity = 64;
  std::size_t donation_interval = 64;
};

struct Q2ParallelWorkerStats {
  u64 jobs{}, front_products{}, input_rectangles{}, count_node_visits{}, supports{};
  u64 pool_peak_bytes{};
  double elapsed_ms{}, payload_ms{};
  // jobs counts completed initial seeds only; donations are separate.
  WspdFrontDispatchWork dispatch_work{};
};

// Integer work is summed over the prefix and workers; geometric decisions
// do not depend on scheduling. Front stack/depth are logical DFS maxima,
// not physical parallel memory. Pool times are SUMS of worker intervals.
struct WspdQ2ParallelResult {
  WspdFrontResult front;
  Q2CensusWork census_work;
  Q2SiblingWork sibling_work;
  Q2OrderWork order_work;
  Q2JointWork joint_work;
  Q2PoolWork pool_work;
  u64 input_rectangles{}, anchor_queries{};
  u64 candidate_pairs{}, accepted_pairs{}, rejected_pairs{};
  u64 requested_workers{}, started_workers{}, target_jobs{}, jobs{}, completed_jobs{};
  u64 terminal_jobs{}, prefix_product_visits{}, job_storage_bytes{};
  WspdFrontDispatchWork dispatch_work{};
  u64 queue_storage_bytes{};
  // Sum of per-worker maxima: an upper bound, NOT simultaneous peak RSS.
  u64 pool_peak_bytes_sum{};
  double partition_ms{}, worker_ms_sum{}, payload_ms_sum{}, total_ms{};
  std::vector<Q2ParallelWorkerStats> workers;
};

// Front jobs: each original parent is evaluated once before its
// children are distributed. Every worker owns one reusable census engine
// and its payload buffers. No complete WSPD or support catalogue is stored.
// Consumers are copied before work starts; slot i is invoked only by worker
// i, serially within that slot. Different slots may run concurrently, so
// mutable captured state must be disjoint or synchronized by the caller.
// Each support view lasts only for its callback; order across workers is
// unspecified. Equal balls retain all original support incidences.
//
// This call owns an index reference until all workers have joined. Pool
// plans and order contexts stay synchronous inside a worker; no borrowed
// plan/context escapes into a job queue. Callback exceptions stop taking
// new jobs; already running jobs may finish before all workers join and
// the exception is rethrown. Earlier emissions are not rolled back.
// Empty/invalid options fail before emission. No detached worker survives
// a success, callback failure or thread-launch failure.
//
// consumers.size() is the requested worker count (>0); jobs_per_worker is
// positive, not an exploration cap. Actual workers <= jobs. One worker
// executes inline. Wall time includes partition, launches, joins, reductions
// and destruction of private engines/contexts, not caller preprocessing.
// Never subtract payload_ms_sum or worker_ms_sum from total_ms.
// Donate additionally shares unvisited DFS products through a bounded
// queue. A full/busy queue never blocks its producer: traversal continues
// locally. It does not divide an already running census callback. Queue
// capacity and donation interval must be positive, also in Coarse mode.
// front_proposals only widens the front's rejection heuristic
// (WspdFrontProposals); the census still starts every count from zero.
[[nodiscard]] WspdQ2ParallelResult run_wspd_q2_census_parallel(
    Q2CensusIndexPtr index, unsigned kmax, unsigned separation_s,
    WspdFrontMode front_mode, Q2CensusMode census_mode,
    std::span<const Q2CensusConsumer> consumers, std::size_t jobs_per_worker = 16,
    Q2SiblingMode sibling_mode = Q2SiblingMode::Disabled,
    Q2WitnessOrder witness_order = Q2WitnessOrder::GlobalDfs,
    Q2AnchorMode anchor_mode = Q2AnchorMode::Individual,
    std::size_t pool_min_factor = 0, WspdQ2Schedule schedule = {},
    WspdFrontProposals front_proposals = {});

}  // namespace mhgp8
