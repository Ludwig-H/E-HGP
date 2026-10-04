// Reclamation saturante et barriere d'epoque ; aucune file asynchrone, aucune allocation par travail.
#include "sched/sched.hpp"

#include <algorithm>
#include <system_error>

namespace mhgp11::sched {
namespace {

class ActiveCall {
 public:
  explicit ActiveCall(std::atomic_flag& flag) noexcept : flag_(flag) {}
  ~ActiveCall() { flag_.clear(std::memory_order_release); }
  ActiveCall(const ActiveCall&) = delete;
  ActiveCall& operator=(const ActiveCall&) = delete;

 private:
  std::atomic_flag& flag_;
};

Outcome invoke(Pool::Body body, void* context, u64 begin, u64 end, u32 worker) noexcept {
  try {
    return body(context, begin, end, worker);
  } catch (const std::bad_alloc&) {
    return fail(Reason::memory_budget);
  } catch (...) {
    return fail(Reason::task_exception);
  }
}

}  // namespace

Pool::Pool(u32 workers) {
  workers_.reserve(workers - 1);
  try {
    for (u32 worker = 1; worker < workers; ++worker)
      workers_.emplace_back([this, worker] { worker_loop(worker); });
  } catch (...) {
    shutdown();  // jamais de std::thread joignable detruit, meme si la creation du suivant echoue
    throw;
  }
}

Pool::~Pool() { shutdown(); }

void Pool::shutdown() noexcept {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stop_ = true;
  }
  wake_.notify_all();
  for (auto& worker : workers_) worker.join();
}

void Pool::run_chunks(Job& job, u32 worker) noexcept {
  Outcome outcome;
  u64 begin = job.next.load(std::memory_order_relaxed);
  while (begin < job.n) {
    // 0<=begin<end<=n, meme pour n=UINT64_MAX ; aucun fetch_add susceptible de reboucler.
    const u64 end = begin + std::min(job.grain, job.n - begin);
    if (!job.next.compare_exchange_weak(begin, end, std::memory_order_relaxed)) continue;
    outcome = merge(outcome, invoke(job.body, job.context, begin, end, worker));
    begin = job.next.load(std::memory_order_relaxed);
  }
  job.outcomes[worker] = outcome;
}

void Pool::worker_loop(u32 worker) {
  bool seen = false;
  for (;;) {
    Job* job;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      wake_.wait(lock, [&] { return stop_ || (current_ != nullptr && epoch_ != seen); });
      if (stop_) return;
      seen = epoch_;
      job = current_;
    }
    run_chunks(*job, worker);
    {
      std::lock_guard<std::mutex> lock(mutex_);
      --job->remaining;
      if (job->remaining == 0) done_.notify_all();
    }
  }
}

Outcome Pool::parallel_for(u64 n, u64 grain, void* context, Body body) noexcept {
  if (active_.test_and_set(std::memory_order_acquire)) return fail(Reason::pool_busy);
  const ActiveCall active(active_);
  if (grain == 0 || body == nullptr) return fail(Reason::parameter_out_of_range);
  if (n == 0) return {};
  Job job;
  job.n = n;
  job.grain = grain;
  job.context = context;
  job.body = body;
  job.remaining = static_cast<u32>(workers_.size());
  {
    std::lock_guard<std::mutex> lock(mutex_);
    current_ = &job;
    epoch_ = !epoch_;
  }
  wake_.notify_all();
  run_chunks(job, 0);
  {
    std::unique_lock<std::mutex> lock(mutex_);
    done_.wait(lock, [&] { return job.remaining == 0; });
    current_ = nullptr;
  }
  Outcome outcome;
  for (u32 worker = 0; worker < size(); ++worker) outcome = merge(outcome, job.outcomes[worker]);
  return outcome;
}

Result<std::unique_ptr<Pool>> make_pool(PoolParams params) noexcept {
  if (params.workers < 1 || params.workers > kMaxWorkers) return fail(Reason::parameter_out_of_range);
  try {
    return std::unique_ptr<Pool>(new Pool(params.workers));
  } catch (const std::bad_alloc&) {
    return fail(Reason::session_overhead);
  } catch (const std::system_error&) {
    return fail(Reason::session_overhead);
  }
}

}  // namespace mhgp11::sched
