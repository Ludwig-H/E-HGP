#pragma once

// MorseHGP3D v9 (23 septembre 2026) — S4a: host run of gpu/lanes.hpp
// (HostGroup), the reference of the device call (same slabs, same deferral
// rules, same records) and the CPU implementation of the chain lever
// q34_batch_q3.
//
// v9 S4b tasks (24 septembre 2026, lanes plan step 2): the host twin runs
// the SAME three steps as the device (gpu/lanes_tasks.hpp) — P per edge,
// T per (edge, seed range) task, C per edge (replay, arena rule in edge
// order, gather in (phase, task) order, ledger of the decided edges) — over
// windows of edges (bounded host memory: the covers of one window at a
// time). Its output (records, begins, counts, statuses, ledgers) is a
// function of the input only: the same for every thread count, window size
// and task order, and byte-identical to the device call and to the
// one-task-per-edge run (B = single_task_budget).

#include "filter_runner.hpp"
#include "lanes_tasks.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace mhgp9::gpu {

namespace lanes_host_detail {

// A barrier that a failure opens for good: every waiter returns false and
// leaves (auditor A: never a waiter left behind, never a terminating thread).
class PhaseBarrier {
 public:
  explicit PhaseBarrier(std::size_t count) : count_(count) {}
  bool arrive_and_wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (failed_) return false;
    const std::size_t generation = generation_;
    if (++waiting_ == count_) {
      waiting_ = 0;
      ++generation_;
      turn_.notify_all();
      return true;
    }
    turn_.wait(lock, [&] { return generation_ != generation || failed_; });
    return !failed_;
  }
  void fail() {
    const std::lock_guard<std::mutex> lock(mutex_);
    failed_ = true;
    turn_.notify_all();
  }
  bool failed() {
    const std::lock_guard<std::mutex> lock(mutex_);
    return failed_;
  }

 private:
  std::mutex mutex_;
  std::condition_variable turn_;
  std::size_t count_, waiting_ = 0, generation_ = 0;
  bool failed_ = false;
};

}  // namespace lanes_host_detail

inline constexpr std::size_t lanes_host_window = 16384;  // edges whose covers live at once on the host

// The three steps over windows of `window` edges (>= 1). run_lanes_batch_host
// is this with lanes_host_window; the gates vary the window.
inline LanesOutput run_lanes_tasks_host(const LanesInput& input, std::size_t workers, std::size_t window) {
  LanesOutput out;
  out.error = validate_lanes_input(input);
  if (!out.error.empty()) {
    out.error_kind = BatchError::input_guard;
    return out;
  }
  const auto start = std::chrono::steady_clock::now();
  out.available = true;
  out.device = "cpu";
  const std::size_t edges = input.edge_count;
  const u32 capacity = input.capacity == 0 ? default_lanes_capacity : input.capacity;
  const u32 record_capacity = input.record_capacity == 0 ? default_record_capacity : input.record_capacity;
  const u32 event_capacity = input.event_capacity == 0 ? default_event_capacity : input.event_capacity;
  const std::size_t arena = input.arena_capacity == 0 ? default_arena_capacity(edges) : input.arena_capacity;
  const u64 budget = input.task_budget == 0 ? default_task_budget : input.task_budget;
  const u64 staging_capacity =
      input.staging_capacity == 0 ? default_staging_capacity(edges) : input.staging_capacity;
  const u64 cover_capacity = input.cover_capacity;  // 0: no limit on the host
  out.capacity = capacity;
  out.record_capacity = record_capacity;
  out.status.assign(edges, 0);
  out.record_begin.assign(edges, 0);
  out.record_count.assign(edges, 0);
  const LanesIndex index{CertificateIndex{input.index.nodes, input.escapes, static_cast<u32>(input.index.node_count),
                                          input.index.rank_points},
                         input.rank_ids};
  if (edges == 0) return out;  // no slab: nothing to decide
  if (window == 0) window = 1;
  const std::size_t windows = (edges + window - 1) / window;
  const std::size_t threads = std::max<std::size_t>(1, std::min(workers, edges));

  // Per-window state (shared, written in disjoint parts or by thread 0
  // between barriers).
  std::vector<LanesPlan> plans;
  std::vector<EdgeQ3Work> plan_work;
  std::vector<u32> cover_owner;         // thread whose cover store holds the edge
  std::vector<LanesTaskWork> slots;     // the edge's tasks' work
  std::vector<std::size_t> task_first;  // window-local, size n + 1
  std::vector<LanesTask> tasks;
  std::vector<LanesTaskRecords> scan;   // per task: its records, then their inclusive scan (C)
  std::vector<u32> task_owner;          // thread whose staging holds the task's records
  std::vector<u8> pre_status;           // after the replay, before the arena rule
  std::vector<u32> counts;
  std::atomic<std::size_t> next_edge{0}, next_task{0}, next_gather{0};
  // Per-thread stores and results.
  std::vector<std::vector<std::int32_t>> cover_points(threads);
  std::vector<std::vector<u32>> cover_ranks(threads), cover_seeds(threads);
  std::vector<RawVector<LaneRecord>> staging(threads);
  std::vector<Q3Work> work3(threads);
  std::vector<Q4Work> work4(threads);
  std::vector<u64> cover_total(threads, 0), staged_total(threads, 0), task_steps(threads, 0);
  u64 tasks_total = 0;
  unsigned long long reserved = 0;
  std::size_t window_first = 0, window_count = 0;
  double plan_ms = 0, task_ms = 0, compact_ms = 0;
  std::exception_ptr failure;
  std::mutex failure_mutex;
  lanes_host_detail::PhaseBarrier barrier(threads);
  const auto fail = [&] {
    {
      const std::lock_guard<std::mutex> lock(failure_mutex);
      if (!failure) failure = std::current_exception();
    }
    barrier.fail();
  };
  const auto worker = [&](std::size_t self) noexcept {
    try {
      std::vector<u32> ranges(2 * static_cast<std::size_t>(capacity)), scratch(capacity);
      std::vector<LaneRecord> slab_records(record_capacity);
      std::vector<u32> positions(event_capacity), bits(event_capacity), list(event_capacity);
      const Q4Slab q4{positions.data(), bits.data(), list.data(), event_capacity};
      const LanesSlab walk{ranges.data(), nullptr, nullptr, nullptr, scratch.data(), nullptr, capacity, 0};
      std::chrono::steady_clock::time_point mark{};  // thread 0: start of the current phase
      const auto lap = [&](double& to) {
        const auto now = std::chrono::steady_clock::now();
        to += std::chrono::duration<double, std::milli>(now - mark).count();
        mark = now;
      };
      for (std::size_t w = 0; w < windows; ++w) {
        // Thread 0 opens the window; every thread empties its own stores
        // (the last barrier of the previous window ended every reader).
        if (self == 0) {
          window_first = w * window;
          window_count = std::min(edges, window_first + window) - window_first;
          plans.assign(window_count, LanesPlan{});
          plan_work.assign(window_count, EdgeQ3Work{});
          cover_owner.assign(window_count, 0);
          slots.assign(window_count, LanesTaskWork{});
          next_edge.store(0);
          next_task.store(0);
          next_gather.store(0);
        }
        cover_points[self].clear();
        cover_ranks[self].clear();
        cover_seeds[self].clear();
        staging[self].clear();
        if (!barrier.arrive_and_wait()) return;
        if (self == 0) mark = std::chrono::steady_clock::now();
        // ---- P: one per edge of the window.
        for (;;) {
          const std::size_t i = next_edge.fetch_add(1);
          if (i >= window_count) break;
          const std::size_t e = window_first + i;
          const u8 lanes = input.edge_lanes == nullptr ? u8{2} : input.edge_lanes[e];
          u32 range_count = 0, sites = 0;
          EdgeQ3Work& local = plan_work[i];
          LanesPlan& plan = plans[i];
          plan.lanes = lanes;
          const auto status = lanes_plan_cover(HostGroup{}, index, input.edge_a[e], input.edge_b[e], lanes,
                                               input.index.kmax, walk, range_count, sites, local);
          if (status == CertificateStatus::decided) {
            const u32 cover = sites;  // reserved whole (the device's arena rule), kept sites below
            auto& points = cover_points[self];
            auto& ranks = cover_ranks[self];
            auto& seeds = cover_seeds[self];
            const std::size_t offset = ranks.size();
            points.resize(3 * (offset + sites));
            ranks.resize(offset + sites);
            seeds.resize(offset + sites);
            const LanesSlab placed{ranges.data(), points.data() + 3 * offset, ranks.data() + offset,
                                   seeds.data() + offset, scratch.data(), nullptr, capacity, 0};
            plan.seeds = lanes_plan_order(HostGroup{}, index, input.edge_a[e], input.edge_b[e], lanes, placed,
                                          range_count, sites, local);
            plan.offset = offset;
            plan.sites = sites;
            plan.tasks = lanes_task_count(sites, plan.seeds, budget);
            cover_owner[i] = static_cast<u32>(self);
            cover_total[self] += cover;
          }
          plan.status = static_cast<u8>(status);
        }
        if (!barrier.arrive_and_wait()) return;
        if (self == 0) {
          // The task table of the window, in (edge, range) order.
          task_first.assign(window_count + 1, 0);
          for (std::size_t i = 0; i < window_count; ++i) task_first[i + 1] = task_first[i] + plans[i].tasks;
          tasks.assign(task_first[window_count], LanesTask{});
          scan.assign(tasks.size(), LanesTaskRecords{0, 0});
          task_owner.assign(tasks.size(), 0);
          for (std::size_t i = 0; i < window_count; ++i) {
            const u64 per = lanes_seeds_per_task(plans[i].sites, budget);
            for (u32 k = 0; k < plans[i].tasks; ++k) {
              LanesTask& t = tasks[task_first[i] + k];
              t.edge = static_cast<u32>(i);
              t.first = static_cast<u32>(k * per);
              t.last = static_cast<u32>(std::min<u64>(plans[i].seeds, k * per + per));
            }
          }
          tasks_total += tasks.size();
          pre_status.assign(window_count, 0);
          counts.assign(window_count, 0);
          lap(plan_ms);
        }
        if (!barrier.arrive_and_wait()) return;
        // ---- T: one per task.
        for (;;) {
          std::size_t t = next_task.fetch_add(1);
          if (t >= tasks.size()) break;
#if defined(MHGP9_LANES_TASKS_MUTANT_COMPLETION_ORDER)
          t = tasks.size() - 1 - t;  // mutant emulation: the tasks complete in reverse order
#endif
          LanesTask& task = tasks[t];
          const std::size_t i = task.edge, e = window_first + i;
          const LanesPlan& plan = plans[i];
          const u32 owner = cover_owner[i];
          const LanesSlab slab{nullptr,
                               cover_points[owner].data() + 3 * plan.offset,
                               cover_ranks[owner].data() + plan.offset,
                               cover_seeds[owner].data() + plan.offset,
                               nullptr,
                               slab_records.data(),
                               capacity,
                               record_capacity};
          Q4Work w4{};
          const u64 steps = lanes_task(HostGroup{}, index, input.edge_a[e], input.edge_b[e], plan.lanes,
                                       input.index.kmax, slab, plan.sites, task.first, task.last, q4, task, w4,
                                       slots[i]);
          task_steps[self] = std::max<u64>(task_steps[self], steps);
          lanes_task_publish(task, t, slots[i], scan[t]);
          if (task.fail_phase == 0) {
            const u32 n = task.q3 + task.q4;
            task.begin = staging[self].size();
            task_owner[t] = static_cast<u32>(self);
            staging[self].insert(staging[self].end(), slab_records.begin(), slab_records.begin() + n);
            staged_total[self] += n;
          }
        }
        if (!barrier.arrive_and_wait()) return;
        // ---- C (step v2): the inclusive scan of the tasks' records (thread
        // 0), the replay per edge in O(1) (parallel), the arena rule in edge
        // order (thread 0), then the ledger per edge and the gather per task
        // (parallel), with the device's functions.
        if (self == 0) {
          lap(task_ms);
          for (std::size_t t = 1; t < scan.size(); ++t) scan[t] = LanesRecordsSum{}(scan[t - 1], scan[t]);
        }
        if (!barrier.arrive_and_wait()) return;
        for (;;) {
          const std::size_t i = next_gather.fetch_add(1);
          if (i >= window_count) break;
          auto status = static_cast<CertificateStatus>(plans[i].status);
          u32 count = 0;
          if (status == CertificateStatus::decided)
            status = lanes_replay_scan(tasks.data(), scan.data(), task_first[i], plans[i].tasks, slots[i].fail_first,
                                       record_capacity, count);
          pre_status[i] = static_cast<u8>(status);
          counts[i] = count;
        }
        if (!barrier.arrive_and_wait()) return;
        if (self == 0) {
          std::size_t begin = out.records.size();
          for (std::size_t i = 0; i < window_count; ++i) {
            const std::size_t e = window_first + i;
            auto s = static_cast<CertificateStatus>(pre_status[i]);
            if (s == CertificateStatus::decided) {
              // The device's rule: the exclusive prefix of the counts (the
              // counter always advances).
              if (!lanes_edge_kept(pre_status[i], reserved, counts[i], arena)) {
                s = CertificateStatus::deferred;
              } else {
                out.record_begin[e] = static_cast<u32>(begin);
                out.record_count[e] = counts[i];
                begin += counts[i];
              }
              reserved += counts[i];
            }
            out.status[e] = static_cast<u8>(s);
            if (s == CertificateStatus::deferred) ++out.deferred;
            else if (s == CertificateStatus::fault) ++out.faults;
          }
          const std::size_t from = out.records.size();
          out.records.resize(begin);
          poison_unwritten(out.records, from);
          next_gather.store(0);
          next_task.store(0);
        }
        if (!barrier.arrive_and_wait()) return;
        for (;;) {
          const std::size_t i = next_gather.fetch_add(1);
          if (i >= window_count) break;
#if defined(MHGP9_LANES_TASKS_MUTANT_DEFERRED_COUNTERS)
          const bool counted = plans[i].status == static_cast<u8>(CertificateStatus::decided);  // mutant
#else
          const bool counted = out.status[window_first + i] == static_cast<u8>(CertificateStatus::decided);
#endif
          if (counted) {
            EdgeQ3Work e3{};
            Q4Work e4{};
            lanes_edge_work(plan_work[i], slots[i], plans[i].lanes, e3, e4);
            add_q3_edge(work3[self], e3);
            add_q4(work4[self], e4);
          }
        }
        for (;;) {
          const std::size_t t = next_task.fetch_add(1);
          if (t >= tasks.size()) break;
          const LanesTask& task = tasks[t];
          const std::size_t i = task.edge, e = window_first + i;
          if (out.status[e] != static_cast<u8>(CertificateStatus::decided) || task.q3 + task.q4 == 0) continue;
          u64 to3 = 0, to4 = 0;
          lanes_task_destinations(scan.data(), task, task_first[i], plans[i].tasks, t, out.record_begin[e], to3, to4);
          const LaneRecord* from = staging[task_owner[t]].data() + task.begin;
          for (u32 r = 0; r < task.q3 + task.q4; ++r) {
            LaneRecord& to = out.records[r < task.q3 ? to3 + r : to4 + (r - task.q3)];
            to = from[r];
            to.edge = static_cast<u32>(e);
          }
        }
        if (!barrier.arrive_and_wait()) return;
        if (self == 0) lap(compact_ms);
      }
    } catch (...) {
      fail();
    }
  };
  std::vector<std::thread> pool;
  pool.reserve(threads);
  for (std::size_t t = 1; t < threads; ++t) {
    try {
      pool.emplace_back(worker, t);
    } catch (...) {
      fail();  // the calling thread still runs (and fails fast), then joins
      break;
    }
  }
  worker(0);
  for (auto& thread : pool) thread.join();
  if (failure) std::rethrow_exception(failure);
  if (barrier.failed()) throw std::logic_error("mhgp9 lanes host twin: a phase barrier failed without a cause");
  u64 covers = 0, staged = 0;
  for (std::size_t t = 0; t < threads; ++t) {
    add_q3(out.work, work3[t]);
    add_q4(out.work4, work4[t]);
    covers += cover_total[t];
    staged += staged_total[t];
    out.max_task_steps = std::max<u64>(out.max_task_steps, task_steps[t]);
  }
  out.tasks = tasks_total;
  // The device's arenas (lanes_tasks.hpp): an overflow is a refusal of the
  // whole call, decided on totals that do not depend on the order.
  const auto refuse = [&](const char* why) {
    LanesOutput refused;
    refused.error = why;
    refused.error_kind = BatchError::capacity;
    return refused;
  };
  if (cover_capacity != 0 && covers > cover_capacity) return refuse("lanes cover arena exceeded");
  if (tasks_total > 0xffffffffULL) return refuse("lanes task table exceeds 2^32 tasks");
  if (staged > staging_capacity) {
    // As the device (review before R18): a staging overflow defers every
    // non-faulty edge to the CPU tail, with no record and no ledger; it
    // never refuses the call.
    for (std::size_t e = 0; e < out.status.size(); ++e) {
      if (out.status[e] != static_cast<u8>(CertificateStatus::fault))
        out.status[e] = static_cast<u8>(CertificateStatus::deferred);
      out.record_begin[e] = 0;
      out.record_count[e] = 0;
    }
    out.records.clear();
    out.work = Q3Work{};
    out.work4 = Q4Work{};
    out.deferred = out.faults = 0;
    for (const auto status : out.status)
      if (status == static_cast<u8>(CertificateStatus::deferred)) ++out.deferred;
      else ++out.faults;
  }
  const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
  out.kernel_ms = ms;
  out.total_ms = ms;
  out.plan_ms = plan_ms;
  out.task_ms = task_ms;
  out.compact_ms = compact_ms;
  return out;
}

inline LanesOutput run_lanes_batch_host(const LanesInput& input, std::size_t workers) {
  return run_lanes_tasks_host(input, workers, lanes_host_window);
}

}  // namespace mhgp9::gpu
