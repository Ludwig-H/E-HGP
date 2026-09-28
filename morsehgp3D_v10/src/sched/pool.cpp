#include "sched/pool.hpp"

#include <algorithm>

namespace mhgp10::sched {

namespace {
thread_local bool tls_in_region = false;
}

bool in_parallel_region() { return tls_in_region; }

Pool::Pool(unsigned threads) {
  if (threads == 0) threads = std::max(1u, std::thread::hardware_concurrency());
  workers_.reserve(threads - 1);
  for (unsigned i = 1; i < threads; ++i) workers_.emplace_back([this, i] { worker_loop(i); });
}

Pool::~Pool() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stop_ = true;
  }
  wake_.notify_all();
  for (auto& t : workers_) t.join();
}

void Pool::run_chunks(unsigned id) {
  const bool was = tls_in_region;
  tls_in_region = true;
  for (;;) {
    const u64 begin = next_.fetch_add(grain_, std::memory_order_relaxed);
    if (begin >= n_) break;
    const u64 end = std::min(n_, begin + grain_);
    (*job_)(begin, end, id);
  }
  tls_in_region = was;
}

void Pool::worker_loop(unsigned id) {
  u64 seen = 0;
  for (;;) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      wake_.wait(lock, [&] { return stop_ || generation_ != seen; });
      if (stop_) return;
      seen = generation_;
      ++active_;
    }
    run_chunks(id);
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (--active_ == 0) done_.notify_all();
    }
  }
}

void Pool::parallel_for(u64 n, u64 grain, const std::function<void(u64, u64, unsigned)>& body) {
  if (n == 0) return;
  if (grain == 0) grain = 1;
  if (tls_in_region || workers_.empty()) {
    if (tls_in_region) nested_.fetch_add(1, std::memory_order_relaxed);
    for (u64 b = 0; b < n; b += grain) body(b, std::min(n, b + grain), 0);
    return;
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    job_ = &body;
    n_ = n;
    grain_ = grain;
    next_.store(0, std::memory_order_relaxed);
    ++generation_;
  }
  wake_.notify_all();
  run_chunks(0);
  std::unique_lock<std::mutex> lock(mutex_);
  done_.wait(lock, [&] { return active_ == 0; });
  job_ = nullptr;
}

}  // namespace mhgp10::sched
