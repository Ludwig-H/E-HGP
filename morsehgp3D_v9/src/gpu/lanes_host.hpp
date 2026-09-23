#pragma once

// MorseHGP3D v9 (23 septembre 2026) — S4a: host run of gpu/lanes.hpp
// (HostGroup), the reference of the device call (same slabs, same deferral
// rules, same records) and the CPU implementation of the chain lever
// q34_batch_q3. Edges are processed in parallel by blocks; the blocks are
// committed in edge order, so the arena reservation (and a deferral when it
// is full) is deterministic whatever the number of threads.

#include "filter_runner.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>

namespace mhgp9::gpu {

inline LanesOutput run_lanes_batch_host(const LanesInput& input, std::size_t workers) {
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
  const std::size_t arena = input.arena_capacity == 0 ? default_arena_capacity(edges) : input.arena_capacity;
  out.capacity = capacity;
  out.record_capacity = record_capacity;
  out.status.assign(edges, 0);
  out.record_begin.assign(edges, 0);
  out.record_count.assign(edges, 0);
  const LanesIndex index{CertificateIndex{input.index.nodes, input.escapes, static_cast<u32>(input.index.node_count),
                                          input.index.rank_points},
                         input.rank_ids};
  constexpr std::size_t grain = 64;
  const std::size_t blocks = (edges + grain - 1) / grain;
  const std::size_t threads = std::max<std::size_t>(1, std::min(workers, blocks));
  std::atomic<std::size_t> next{0};
  std::mutex mutex;
  std::condition_variable turn;
  std::size_t committed = 0;  // blocks committed, in order
  bool failed = false;
  std::exception_ptr failure;
  unsigned long long reserved = 0;
  const auto worker = [&] {
    std::vector<u32> ranges(2 * static_cast<std::size_t>(capacity)), ranks(capacity), seeds(capacity),
        scratch(capacity);
    std::vector<std::int32_t> points(3 * static_cast<std::size_t>(capacity));
    std::vector<LaneRecord> slab_records(record_capacity);
    const LanesSlab slab{ranges.data(), points.data(), ranks.data(), seeds.data(), scratch.data(),
                         slab_records.data(), capacity, record_capacity};
    std::vector<CertificateStatus> status;
    std::vector<EdgeQ3Work> local;
    std::vector<u32> counts;
    std::vector<LaneRecord> records;
    for (;;) {
      const std::size_t block = next.fetch_add(1);
      if (block >= blocks) return;
      const std::size_t first = block * grain, last = std::min(edges, first + grain);
      status.assign(last - first, CertificateStatus::decided);
      local.assign(last - first, EdgeQ3Work{});
      counts.assign(last - first, 0);
      records.clear();
      try {
        for (std::size_t i = first; i < last; ++i) {
          u32 count = 0;
          status[i - first] = q3_lane(HostGroup{}, index, input.edge_a[i], input.edge_b[i], input.index.kmax, slab,
                                      count, local[i - first]);
          if (status[i - first] != CertificateStatus::decided) continue;
          counts[i - first] = count;
          for (u32 r = 0; r < count; ++r) {
            records.push_back(slab_records[r]);
            records.back().edge = static_cast<u32>(i);
          }
        }
      } catch (...) {
        const std::lock_guard<std::mutex> lock(mutex);
        if (!failure) failure = std::current_exception();
        failed = true;
        turn.notify_all();
        return;
      }
      std::unique_lock<std::mutex> lock(mutex);
      turn.wait(lock, [&] { return committed == block || failed; });
      if (failed) return;
      std::size_t cursor = 0;
      for (std::size_t i = first; i < last; ++i) {
        auto s = status[i - first];
        const u32 count = counts[i - first];
        if (s == CertificateStatus::decided) {
          // The device's reservation: the counter always advances.
          reserved += count;
          if (reserved > arena) {
            s = CertificateStatus::deferred;
          } else {
            out.record_begin[i] = static_cast<u32>(out.records.size());
            out.record_count[i] = count;
            out.records.insert(out.records.end(), records.begin() + static_cast<std::ptrdiff_t>(cursor),
                               records.begin() + static_cast<std::ptrdiff_t>(cursor + count));
            add_q3_edge(out.work, local[i - first]);
          }
          cursor += count;
        }
        out.status[i] = static_cast<u8>(s);
        if (s == CertificateStatus::deferred) ++out.deferred;
        else if (s == CertificateStatus::fault) ++out.faults;
      }
      ++committed;
      turn.notify_all();
    }
  };
  std::vector<std::thread> pool;
  pool.reserve(threads);
  try {
    for (std::size_t t = 1; t < threads; ++t) pool.emplace_back(worker);
  } catch (...) {
    {
      const std::lock_guard<std::mutex> lock(mutex);
      failed = true;
      turn.notify_all();
    }
    for (auto& thread : pool) thread.join();
    throw;
  }
  worker();
  for (auto& thread : pool) thread.join();
  if (failure) std::rethrow_exception(failure);
  const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
  out.kernel_ms = ms;
  out.total_ms = ms;
  return out;
}

}  // namespace mhgp9::gpu
