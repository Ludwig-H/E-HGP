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

void Pool::run_chunks(Job& job, unsigned id) {
  const bool was = tls_in_region;
  tls_in_region = true;
  for (;;) {
    const u64 begin = job.next.fetch_add(job.grain, std::memory_order_relaxed);
    if (begin >= job.n) break;
    (*job.body)(begin, std::min(job.n, begin + job.grain), id);
  }
  tls_in_region = was;
}

void Pool::worker_loop(unsigned id) {
  u64 seen = 0;
  for (;;) {
    Job* job;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      wake_.wait(lock, [&] { return stop_ || (current_ != nullptr && generation_ != seen); });
      if (stop_) return;
      seen = generation_;
      job = current_;
      ++job->users;
    }
    run_chunks(*job, id);
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (--job->users == 0) done_.notify_all();
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
  // Un ouvrier en retard ne voit jamais les champs d'un autre travail : il capture le descripteur sous le verrou, et
  // l'appelant ferme la capture avant d'attendre ceux qui l'ont pris (regression du 29 septembre 2026 : un ouvrier
  // inscrit apres la fin d'un travail lisait le compteur du suivant et en executait des tranches deux fois).
  Job job;
  job.body = &body;
  job.n = n;
  job.grain = grain;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    current_ = &job;
    ++generation_;
  }
  wake_.notify_all();
  run_chunks(job, 0);
  std::unique_lock<std::mutex> lock(mutex_);
  current_ = nullptr;
  done_.wait(lock, [&] { return job.users == 0; });
}

}  // namespace mhgp10::sched
