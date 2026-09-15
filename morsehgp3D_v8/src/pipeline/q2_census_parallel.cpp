#include "q2_census_parallel.hpp"

#include "parallel/joined_workers.hpp"
#include "parallel/work_reduction.hpp"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace mhgp8 {
namespace {
using Clock = std::chrono::steady_clock;

void merge(Q2CensusResumeSnapshot& out, const Q2CensusResumeSnapshot& v) {
  counter_add(out.census.candidate_pairs, v.census.candidate_pairs);
  counter_add(out.census.accepted_pairs, v.census.accepted_pairs);
  counter_add(out.census.rejected_pairs, v.census.rejected_pairs);
  parallel_detail::merge_work(out.census.work, v.census.work);
  parallel_detail::merge_work(out.sibling_work, v.sibling_work);
  parallel_detail::merge_work(out.order_work, v.order_work);
  out.census.query_index_ms += v.census.query_index_ms;
  out.census.count_ms += v.census.count_ms;
  out.census.payload_ms += v.census.payload_ms;
  out.census.total_ms += v.census.total_ms;
#define MHGP8_RESUME_SUM(field) counter_add(out.resume_work.field, v.resume_work.field)
  MHGP8_RESUME_SUM(advance_calls); MHGP8_RESUME_SUM(transitions);
  MHGP8_RESUME_SUM(entry_steps); MHGP8_RESUME_SUM(witness_steps);
  MHGP8_RESUME_SUM(admission_steps); MHGP8_RESUME_SUM(payload_steps);
  MHGP8_RESUME_SUM(pauses); MHGP8_RESUME_SUM(pauses_after_credit);
  MHGP8_RESUME_SUM(pauses_inside_deferred); MHGP8_RESUME_SUM(pauses_during_emission);
#undef MHGP8_RESUME_SUM
  out.resume_work.max_pending_tasks = std::max(out.resume_work.max_pending_tasks,
                                              v.resume_work.max_pending_tasks);
#define MHGP8_DETACH_SUM(field) counter_add(out.detach_work.field, v.detach_work.field)
  MHGP8_DETACH_SUM(attempts); MHGP8_DETACH_SUM(detached_frames);
  MHGP8_DETACH_SUM(imported_frames); MHGP8_DETACH_SUM(transferred_pairs);
  MHGP8_DETACH_SUM(moved_frames);
#undef MHGP8_DETACH_SUM
}

void merge(Q2CensusScheduleWork& out, const Q2CensusScheduleWork& v) {
#define MHGP8_SCHEDULE_SUM(field) counter_add(out.field, v.field)
  MHGP8_SCHEDULE_SUM(offer_checks); MHGP8_SCHEDULE_SUM(offer_busy);
  MHGP8_SCHEDULE_SUM(offer_full); MHGP8_SCHEDULE_SUM(offer_no_sibling);
  MHGP8_SCHEDULE_SUM(donations); MHGP8_SCHEDULE_SUM(fragments_started);
  MHGP8_SCHEDULE_SUM(fragments_completed); MHGP8_SCHEDULE_SUM(waits);
  MHGP8_SCHEDULE_SUM(wakes);
#undef MHGP8_SCHEDULE_SUM
  out.max_queue_size = std::max(out.max_queue_size, v.max_queue_size);
  out.max_active_fragments = std::max(out.max_active_fragments, v.max_active_fragments);
}

class AnchorDispatcher {
 public:
  AnchorDispatcher(std::unique_ptr<Q2CensusContinuation> root, std::size_t capacity)
      : queue_(capacity) { queue_[0] = std::move(root); size_ = 1; }

  std::unique_ptr<Q2CensusContinuation> take(Q2CensusScheduleWork& work) {
    std::unique_lock lock(mutex_);
    for (;;) {
      if (cancelled_) return {};
      if (size_ != 0) {
        counter_add(work.fragments_started);
        work.max_queue_size = std::max<u64>(work.max_queue_size, size_);
        ++active_;
        work.max_active_fragments = std::max<u64>(work.max_active_fragments, active_);
        return std::move(queue_[--size_]);
      }
      if (active_ == 0) return {};
      counter_add(work.waits);
      changed_.wait(lock, [&] { return cancelled_ || size_ != 0 || active_ == 0; });
      counter_add(work.wakes);
    }
  }

  void offer(Q2CensusContinuation& donor, Q2CensusScheduleWork& work) {
    counter_add(work.offer_checks);
    std::unique_lock lock(mutex_, std::try_to_lock);
    if (!lock.owns_lock()) { counter_add(work.offer_busy); return; }
    if (cancelled_) return;
    if (size_ == queue_.size()) { counter_add(work.offer_full); return; }
    // A throwing detach leaves the donor intact. Once detached, move into a
    // preallocated empty slot is noexcept; later accounting failure cancels
    // the whole invocation, with the obligation still owned by the queue.
    auto child = donor.detach_pending();
    if (!child) { counter_add(work.offer_no_sibling); return; }
    queue_[size_++] = std::move(child);
    changed_.notify_one();
    counter_add(work.donations);
    work.max_queue_size = std::max<u64>(work.max_queue_size, size_);
  }

  void release() {
    std::lock_guard lock(mutex_);
    --active_;
    if (active_ == 0) changed_.notify_all();
  }

  void cancel() noexcept {
    std::lock_guard lock(mutex_);
    cancelled_ = true;
    changed_.notify_all();
  }

  u64 storage_bytes() const { return queue_.capacity() * sizeof(queue_[0]); }

 private:
  std::vector<std::unique_ptr<Q2CensusContinuation>> queue_;
  std::size_t size_{}, active_{};
  bool cancelled_{};
  std::mutex mutex_;
  std::condition_variable changed_;
};
}  // namespace

Q2CensusParallelResult run_q2_anchor_parallel(
    Q2CensusIndexPtr index, std::size_t anchor_rank, std::size_t b_node,
    unsigned kmax, Q2CensusParallelOptions options,
    std::span<const Q2CensusConsumer> consumers,
    Q2SiblingMode sibling_mode, Q2WitnessOrder witness_order) {
  const auto started = Clock::now();
  if (options.workers == 0 || options.quantum == 0 || options.queue_capacity == 0 ||
      consumers.size() != options.workers)
    throw std::invalid_argument("mhgp8 anchor parallel needs positive W/quantum/queue and W callbacks");
  for (const auto& consumer : consumers)
    if (!consumer) throw std::invalid_argument("mhgp8 empty parallel anchor callback");
  auto root = make_q2_census_continuation(std::move(index), anchor_rank, b_node,
                                        kmax, sibling_mode, witness_order);
  const auto candidates = root->snapshot().census.candidate_pairs;
  Q2CensusParallelResult result;
  result.workers.resize(options.workers);
  {
    AnchorDispatcher dispatcher(std::move(root), options.queue_capacity);
    result.queue_storage_bytes = dispatcher.storage_bytes();
    parallel_detail::run_joined_workers(options.workers,
        [&](std::size_t slot, const std::atomic<bool>& cancelled) {
          auto& worker = result.workers[slot];
          while (!cancelled.load(std::memory_order_relaxed)) {
            auto fragment = dispatcher.take(worker.schedule);
            if (!fragment) break;
            try {
              bool done = false;
              do {
                worker.max_fragment_bytes = std::max<u64>(worker.max_fragment_bytes,
                                                           fragment->memory().retained_bytes);
                if (cancelled.load(std::memory_order_relaxed)) break;
                done = fragment->advance(options.quantum, consumers[slot]);
                if (!done && options.workers > 1) dispatcher.offer(*fragment, worker.schedule);
              } while (!done);
              worker.max_fragment_bytes = std::max<u64>(worker.max_fragment_bytes,
                                                         fragment->memory().retained_bytes);
              if (done) {
                merge(worker.sum, fragment->snapshot());
                counter_add(worker.schedule.fragments_completed);
              }
              fragment.reset();
              dispatcher.release();
            } catch (...) {
              dispatcher.cancel();
              fragment.reset();
              dispatcher.release();
              throw;
            }
          }
        }, parallel_detail::ThreadLauncher{}, [&]() noexcept { dispatcher.cancel(); });
  }
  for (auto& worker : result.workers) {
    worker.sum.status = Q2CensusContinuationStatus::Done;
    merge(result.sum, worker.sum);
    merge(result.schedule, worker.schedule);
    result.max_fragment_bytes = std::max(result.max_fragment_bytes, worker.max_fragment_bytes);
  }
  const auto& c = result.sum.census;
  const auto& s = result.schedule;
  const auto& d = result.sum.detach_work;
  if (c.candidate_pairs != candidates || c.accepted_pairs > candidates ||
      c.rejected_pairs != candidates - c.accepted_pairs ||
      c.work.payload_supports != c.accepted_pairs || c.work.count_root_starts != 1 ||
      c.work.input_descriptors != 1 || d.detached_frames != d.imported_frames ||
      d.detached_frames != s.donations || s.fragments_started != s.fragments_completed ||
      s.fragments_completed == 0 || s.fragments_completed - 1 != s.donations)
    throw std::logic_error("mhgp8 incomplete parallel anchor obligation ledger");
  result.sum.status = Q2CensusContinuationStatus::Done;
  result.total_ms = std::chrono::duration<double, std::milli>(Clock::now() - started).count();
  return result;
}

}  // namespace mhgp8
