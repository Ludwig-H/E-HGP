// Reclamation saturante ; equipe dimensionnee par le nombre de tranches (8 octobre 2026) : chaque ouvrier engage est
// reveille par son propre semaphore et acquitte par un compteur atomique ; le dernier libere l'appelant. Une seule
// tranche s'execute dans l'appelant sans reveil. Aucune file asynchrone, aucune allocation par travail. Remplace
// l'epoque booleenne et le reveil de TOUS les ouvriers sous un mutex commun, candidat (hypothese, a mesurer en
// paire) a la penalite de largeur des petits nuages (session C, receipts/g4_mesc_20261008 : 15,6 ms vers 150 sites
// a 48 fils contre 6,8 ms a 4 fils).
#include "sched/sched.hpp"

#include <algorithm>
#include <system_error>

namespace mhgp12::sched {
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

Pool::Pool(u32 workers) : wake_(new Wake[workers]) {
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
  stop_.store(true, std::memory_order_release);
  for (std::size_t w = 0; w < workers_.size(); ++w) wake_[w].go.release();
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
  for (;;) {
    wake_[worker - 1].go.acquire();  // un jeton par invocation qui engage cet ouvrier
    if (stop_.load(std::memory_order_acquire)) return;
    Job& job = *current_;
    run_chunks(job, worker);
    if (job.remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) done_.release();
  }
}

Outcome Pool::parallel_for(u64 n, u64 grain, void* context, Body body) noexcept {
  if (active_.test_and_set(std::memory_order_acquire)) return fail(Reason::pool_busy);
  const ActiveCall active(active_);
  if (grain == 0 || body == nullptr) return fail(Reason::parameter_out_of_range);
  if (n == 0) return {};
  const u64 chunks = (n - 1) / grain + 1;  // n >= 1 : aucun debordement
  if (chunks == 1) return invoke(body, context, 0, n, 0);  // une tranche [0, n) : dans l'appelant, sans reveil
  const u32 team = static_cast<u32>(std::min<u64>(workers_.size(), chunks - 1));
  Job job;
  job.n = n;
  job.grain = grain;
  job.context = context;
  job.body = body;
  job.remaining.store(team, std::memory_order_relaxed);
  if (team != 0) {
    current_ = &job;
    for (u32 w = 0; w < team; ++w) wake_[w].go.release();  // publie current_ et job (release)
  }
  run_chunks(job, 0);
  if (team != 0) {
    done_.acquire();  // le dernier ouvrier engage a acquitte : toutes les cases de job.outcomes sont ecrites
    current_ = nullptr;
  }
  Outcome outcome;
  for (u32 worker = 0; worker <= team; ++worker) outcome = merge(outcome, job.outcomes[worker]);
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

}  // namespace mhgp12::sched
