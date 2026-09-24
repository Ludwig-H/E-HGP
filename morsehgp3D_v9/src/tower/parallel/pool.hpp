// MorseHGP3D v6 — parallelisme MESURE, jamais declare.
//
// Deux primitives, toutes deux a tirage dynamique et a fusion en ORDRE
// D'INDEX (jamais en ordre d'achevement) : la sortie est bit-identique au
// sequentiel quel que soit le nombre de fils. Chaque primitive RETOURNE le
// nombre d'ouvriers reellement engages (fils crees pour l'appel, ou
// participants d'un pool persistant, appelant compris) : c'est cette valeur,
// et elle seule, que les compteurs publient (mutant `parallel-one-worker` : un
// seul fil quel que soit le budget — la metadonnee mesuree le trahit).
//
// v9 E2 (pool persistant de la tour) : un TaskPool cree une fois par
// construction de tour sert les primitives appelees par SON fil proprietaire
// (PoolScope, pointeur thread_local) ; toute autre invocation — depuis un
// ouvrier du pool, depuis la part du proprietaire pendant un travail, depuis
// un fil tiers (coureurs de la phase A) — garde ses propres fils, comme
// avant. Meme partition du travail, memes indices d'ouvrier : sortie
// bit-identique.
#pragma once

#include <algorithm>
#include <atomic>
#include <cfenv>
#include <chrono>
#include <memory>
#include <cstddef>
#include <exception>
#include <mutex>
#include <system_error>
#include <thread>
#include <vector>

#include "../../common/raw_vector.hpp"
#include "../core/mutants.hpp"
#include "../core/types.hpp"

namespace mhgp9::tower {

// Vecteur a initialisation par defaut (src/common/raw_vector.hpp) : tampons
// dont chaque case est ecrite avant toute lecture (v9 E1).
using mhgp9::DefaultInitAllocator;
using mhgp9::RawVector;
using mhgp9::poison_unwritten;

inline size_t planned_workers(size_t items, int threads) {
  if (threads <= 1 || items <= 1) return 1;
  if (MHGP9_MUTANT("parallel-one-worker")) return 1;
  return std::min((size_t)threads, items);
}

// CONTRAT D'EXCEPTION (commun aux deux primitives) : si `fn` leve dans un
// ouvrier, la PREMIERE exception est capturee, l'arret des nouveaux travaux
// est demande, TOUS les fils sont joints, puis l'exception est relancee dans
// le fil appelant — jamais std::terminate. Les travaux deja commences dans
// d'autres ouvriers se terminent ; leurs resultats sont abandonnes par
// l'appelant avec l'exception.
namespace parallel_detail {
// Admission is delayed until every thread exists. A partially created sort
// cannot enter a barrier whose missing participants would never arrive.
#if defined(MHGP9_TESTING)
inline thread_local size_t launch_fail_after = (size_t)-1;
inline std::atomic<size_t> launch_started{0};
inline std::atomic<size_t> launch_active{0};
// Pool thread `join_delay_index` sleeps join_delay_ms after each wake-up,
// before trying to join the job (gate of the late-join rule).
inline std::atomic<size_t> join_delay_index{0};
inline std::atomic<unsigned> join_delay_ms{0};
#endif
#if defined(MHGP9_POOL_MUTANT_JOIN_CLOSED)
#define MHGP9_POOL_JOIN_CLOSED true  // mutant: a late thread joins a closed job
#else
#define MHGP9_POOL_JOIN_CLOSED false
#endif
// Threads created by the per-call path of run_threads, process-wide (the
// persistent pool's own threads are not counted here). A tower publishes the
// delta over its build: exact when it is the process's only client of these
// helpers during the build, as in the chain.
inline std::atomic<u64> spawned_threads{0};

struct JoinThreads {
  std::vector<std::thread>& threads;
  ~JoinThreads() {
    for (auto& th : threads)
      if (th.joinable()) th.join();
  }
};

// Persistent pool (v9 E2): participants() - 1 threads created ONCE, plus the
// owner thread, which runs its own share as worker 0. run(count, fn) is for
// the helpers' pull loops, where ONE call drains the whole shared work: it
// calls fn(0) on the owner and fn(t) at most once on each pool thread t in
// [1, count) that JOINS the job while it is open; once fn(0) has returned the
// owner closes the job (no thread joins any more) and returns when every
// joined thread has left it. A thread woken after the close skips the job
// without reading its fields, so the fields are never rewritten while a
// thread may still read them, and the end of a job never waits for a thread
// that the scheduler has not run yet (a thread created per call must run
// once before it can be joined). Contracts:
//  - admission: the threads wait until EVERY thread exists; a creation
//    failure cancels them (they exit without reading any job), joins them
//    and rethrows, so a partially created pool is never published and no
//    job ever waits for a participant that does not exist;
//  - exceptions: fn is the helpers' capturing wrapper; an exception that
//    still escapes (on the owner or a thread) is kept, the others finish,
//    and the first one is rethrown on the owner after the join;
//  - floating point: each job carries the owner's floating-point
//    environment, installed by every thread before fn, as a thread created
//    for the call would inherit it (the certified level filter reads the
//    rounding mode);
//  - nesting: while its own share runs, the owner has no current pool, and
//    the pool threads never have one: a helper called from inside a job
//    creates its own threads (no queue on a busy pool, no deadlock).
class TaskPool;
inline thread_local TaskPool* current_pool = nullptr;

class TaskPool {
 public:
  explicit TaskPool(size_t participants) {
    const size_t count = participants > 1 ? participants - 1 : 0;
    threads_.reserve(count);
    try {
      for (size_t t = 0; t < count; ++t) {
#if defined(MHGP9_TESTING)
        if (t == launch_fail_after) {
          while (launch_started.load() < t) std::this_thread::yield();
          throw std::system_error(std::make_error_code(std::errc::resource_unavailable_try_again));
        }
#endif
        threads_.emplace_back([this, t] { worker(t + 1); });
      }
    } catch (...) {
#if defined(MHGP9_POOL_MUTANT_ADMIT_PARTIAL)
      if (!threads_.empty()) {  // mutant: the partial pool is published with the threads it has
        admission_.store(1);
        admission_.notify_all();
        return;
      }
#endif
      admission_.store(2);
      admission_.notify_all();
      for (auto& th : threads_) th.join();
      threads_.clear();
      throw;
    }
    admission_.store(1);
    admission_.notify_all();
  }
  TaskPool(const TaskPool&) = delete;
  TaskPool& operator=(const TaskPool&) = delete;
  ~TaskPool() {
    stopping_.store(true, std::memory_order_release);
    generation_.fetch_add(1, std::memory_order_release);
    generation_.notify_all();
    for (auto& th : threads_) th.join();
  }
  size_t participants() const { return threads_.size() + 1; }
  size_t threads() const { return threads_.size(); }
  u64 jobs() const { return jobs_; }  // read by the owner only
  bool owned_by_this_thread() const { return current_pool == this; }

  template <typename Fn>
  void run(size_t count, Fn& fn) {
#if defined(MHGP9_POOL_MUTANT_NESTED_QUEUE)
    // mutant: a submission while a job runs waits for the pool (a queue)
    for (bool busy = busy_.load(); busy; busy = busy_.load()) busy_.wait(true);
    busy_.store(true);
#endif
    job_invoke_ = [](void* context, size_t t) { (*static_cast<Fn*>(context))(t); };
    job_context_ = &fn;
    job_count_ = count;
    std::fegetenv(&job_env_);
    ++jobs_;
    const u32 generation = generation_.load(std::memory_order_relaxed) + 1;  // the owner alone writes it
    state_.store((static_cast<u64>(generation) << 32) | kOpen, std::memory_order_release);
    generation_.store(generation, std::memory_order_release);
    generation_.notify_all();
    std::exception_ptr own;
    {
#if !defined(MHGP9_POOL_MUTANT_NESTED_QUEUE)
      struct NoPool {  // the owner's share: nested helpers use their own threads
        TaskPool* saved = current_pool;
        NoPool() { current_pool = nullptr; }
        ~NoPool() { current_pool = saved; }
      } no_pool;
#endif
      try { fn(0); } catch (...) { own = std::current_exception(); }
    }
#if !defined(MHGP9_POOL_MUTANT_RETURN_BEFORE_JOIN)
    // Close the job, then wait for the threads that joined it.
    for (u64 left = state_.fetch_and(~kOpen, std::memory_order_acq_rel) & ~kOpen; (left & kActive) != 0;
         left = state_.load(std::memory_order_acquire))
      state_.wait(left, std::memory_order_acquire);
#endif
#if defined(MHGP9_POOL_MUTANT_NESTED_QUEUE)
    busy_.store(false);
    busy_.notify_all();
#endif
    if (own) std::rethrow_exception(own);
    if (worker_error_) {
      std::exception_ptr error;
      error.swap(worker_error_);
      std::rethrow_exception(error);
    }
  }

 private:
  std::vector<std::thread> threads_;
  std::atomic<u32> admission_{0};  // 0 waiting, 1 admitted, 2 cancelled
  std::atomic<u32> generation_{0};  // wake-up word: the generation of the last job
  // Job state: generation (high 32 bits), open bit, threads inside (low 31
  // bits). A thread joins by compare-and-swap while the job of ITS generation
  // is open, and leaves by decrement; the owner closes it by clearing kOpen.
  static constexpr u64 kOpen = u64{1} << 31, kActive = kOpen - 1;
  std::atomic<u64> state_{0};
  // Job fields: written by the owner before the state's release, read by a
  // thread only after joining (acquire), never rewritten before the job is
  // closed and every joined thread has left.
  void (*job_invoke_)(void*, size_t) = nullptr;
  void* job_context_ = nullptr;
  size_t job_count_ = 0;
  std::fenv_t job_env_{};
  std::atomic<bool> stopping_{false};
  u64 jobs_ = 0;
  std::mutex error_mu_;
  std::exception_ptr worker_error_;
#if defined(MHGP9_POOL_MUTANT_NESTED_QUEUE)
  std::atomic<bool> busy_{false};
#endif

  void worker(size_t index) {
#if defined(MHGP9_TESTING)
    launch_active.fetch_add(1);
    launch_started.fetch_add(1);
    struct ActiveGuard {
      ~ActiveGuard() { launch_active.fetch_sub(1); }
    } active_guard;
#endif
    admission_.wait(0);
    if (admission_.load() != 1) return;
#if defined(MHGP9_POOL_MUTANT_NESTED_QUEUE)
    current_pool = this;  // mutant: nested helpers of a job submit to this pool
#endif
    u32 seen = 0;
    for (;;) {
      generation_.wait(seen, std::memory_order_acquire);
      seen = generation_.load(std::memory_order_acquire);
      if (stopping_.load(std::memory_order_acquire)) return;
#if defined(MHGP9_TESTING)
      if (index == join_delay_index) std::this_thread::sleep_for(std::chrono::milliseconds(join_delay_ms.load()));
#endif
      // Join the job of generation `seen` while it is open; a closed or
      // superseded job is skipped without reading its fields.
      bool joined = false;
      for (u64 state = state_.load(std::memory_order_acquire);
           (state >> 32) == seen && ((state & kOpen) != 0 || MHGP9_POOL_JOIN_CLOSED);)
        if (state_.compare_exchange_weak(state, state + 1, std::memory_order_acq_rel, std::memory_order_acquire)) {
          joined = true;
          break;
        }
      if (!joined) continue;
      if (index < job_count_) {
#if !defined(MHGP9_POOL_MUTANT_STALE_FENV)
        std::fesetenv(&job_env_);
#endif
        try {
          job_invoke_(job_context_, index);
        } catch (...) {
          std::lock_guard<std::mutex> lock(error_mu_);
          if (!worker_error_) worker_error_ = std::current_exception();
        }
      }
      const u64 left = state_.fetch_sub(1, std::memory_order_acq_rel) - 1;
      if ((left & (kOpen | kActive)) == 0) state_.notify_all();  // the last one out of a closed job
    }
  }
};

// Installs `pool` as the current pool of THIS thread for the scope (nullptr:
// none), restoring the previous one on exit.
class PoolScope {
 public:
  explicit PoolScope(TaskPool* pool) : saved_(current_pool) { current_pool = pool; }
  PoolScope(const PoolScope&) = delete;
  PoolScope& operator=(const PoolScope&) = delete;
  ~PoolScope() { current_pool = saved_; }
 private:
  TaskPool* saved_;
};

template <typename Fn>
inline void run_threads(size_t count, Fn&& fn) {
#if !defined(MHGP9_POOL_MUTANT_BYPASS)
  if (TaskPool* pool = current_pool; pool && count <= pool->participants()) {
    pool->run(count, fn);
    return;
  }
#endif
  std::atomic<unsigned> admission{0};  // 0 waiting, 1 admitted, 2 cancelled
  std::vector<std::thread> pool;
  pool.reserve(count);
  JoinThreads joined{pool};
  try {
    for (size_t t = 0; t < count; ++t) {
#if defined(MHGP9_TESTING)
      if (t == launch_fail_after) {
        while (launch_started.load() < t) std::this_thread::yield();
        throw std::system_error(std::make_error_code(std::errc::resource_unavailable_try_again));
      }
#endif
      pool.emplace_back([&, t] {
#if defined(MHGP9_TESTING)
        launch_active.fetch_add(1);
        launch_started.fetch_add(1);
        struct ActiveGuard {
          ~ActiveGuard() { launch_active.fetch_sub(1); }
        } active_guard;
#endif
        admission.wait(0);
        if (admission.load() == 1) fn(t);
      });
      spawned_threads.fetch_add(1, std::memory_order_relaxed);
    }
  } catch (...) {
    admission.store(MHGP9_MUTANT("parallel-admit-partial-launch") ? 1u : 2u);
    admission.notify_all();
    throw;
  }
  admission.store(1);
  admission.notify_all();
}

struct FirstException {
  std::atomic<bool> stop{false};
  std::atomic<bool> armed{false};
  std::exception_ptr first;
  std::mutex mu;
  void capture() {
    std::lock_guard<std::mutex> lk(mu);
    if (!armed.load()) {
      first = std::current_exception();
      armed.store(true);
    }
    stop.store(true);
  }
  void rethrow_if_any() {
    if (armed.load()) std::rethrow_exception(first);
  }
};
}  // namespace parallel_detail

// Decoupe [0, n) en tranches contigues (≈ 8 par ouvrier), executees par
// tirage dynamique ; `fn(b, e, worker)` traite la tranche [b, e). Retourne
// le nombre d'ouvriers engages (1 = sequentiel, aucun fil ; sur le pool
// courant du fil appelant, l'appelant est l'ouvrier 0).
template <typename Fn>
inline size_t parallel_ranges(size_t n, int threads, Fn&& fn) {
  const size_t T = planned_workers(n, threads);
  if (T <= 1) {
    if (n > 0) fn((size_t)0, n, (size_t)0);
    return n > 0 ? 1 : 0;
  }
  const size_t chunk = std::max<size_t>(1, (n + 8 * T - 1) / (8 * T));
  const size_t nchunks = (n + chunk - 1) / chunk;
  std::atomic<size_t> next{0};
  parallel_detail::FirstException fx;
  parallel_detail::run_threads(T, [&](size_t t) {
      try {
        for (;;) {
          if (fx.stop.load()) break;
          const size_t c = next.fetch_add(1);
          if (c >= nchunks) break;
          fn(c * chunk, std::min(n, (c + 1) * chunk), t);
        }
      } catch (...) {
        fx.capture();
      }
    });
  fx.rethrow_if_any();
  return T;
}

// Meme contrat, une tache par item : `fn(i, worker)`.
template <typename Fn>
inline size_t parallel_items(size_t n, int threads, Fn&& fn) {
  const size_t T = planned_workers(n, threads);
  if (T <= 1) {
    for (size_t i = 0; i < n; ++i) fn(i, (size_t)0);
    return n > 0 ? 1 : 0;
  }
  std::atomic<size_t> next{0};
  parallel_detail::FirstException fx;
  parallel_detail::run_threads(T, [&](size_t t) {
      try {
        for (;;) {
          if (fx.stop.load()) break;
          const size_t i = next.fetch_add(1);
          if (i >= n) break;
          fn(i, t);
        }
      } catch (...) {
        fx.capture();
      }
    });
  fx.rethrow_if_any();
  return T;
}

// Tri parallele pour un ordre STRICT et TOTAL `less` (aucun ex aequo) : le
// resultat est donc l'unique permutation triee, bit-identique au tri
// sequentiel quel que soit le nombre de fils. Tri par echantillonnage, sans
// fusion serielle : des separateurs pris a des positions pseudo-aleatoires
// (deterministes, jamais une grille periodique qu'une entree structuree
// pourrait aligner) coupent l'ordre en seaux [s_{b-1}, s_b) ; chaque tranche de l'entree
// repartit ses elements dans les seaux (en parallele), puis chaque seau est
// recopie a sa place et trie (en parallele). Retourne le nombre d'ouvriers
// crees au plus large. Pic : un second tampon de n elements.
template <typename T, typename A, typename Less>
inline size_t parallel_sort(std::vector<T, A>& values, int threads, Less less) {
  const size_t n = values.size();
  const size_t workers = planned_workers(n / 4096, threads);
  if (workers <= 1) {
    std::sort(values.begin(), values.end(), less);
    return n > 0 ? 1 : 0;
  }
  const size_t wanted = 4 * workers;
  const size_t samples = std::min(n, 32 * wanted);
  std::vector<T> sample;
  sample.reserve(samples);
  u64 state = 0x9e3779b97f4a7c15ull ^ static_cast<u64>(n);
  for (size_t i = 0; i < samples; ++i) {
    state += 0x9e3779b97f4a7c15ull;  // splitmix64: deterministic positions
    u64 z = state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull; z = (z ^ (z >> 27)) * 0x94d049bb133111ebull; z ^= z >> 31;
    sample.push_back(values[static_cast<size_t>(z % n)]);
  }
  std::sort(sample.begin(), sample.end(), less);
  std::vector<T> splitters;  // strictly increasing
  for (size_t b = 1; b < wanted; ++b) {
    const T& candidate = sample[(b * sample.size()) / wanted];
    if (splitters.empty() || less(splitters.back(), candidate)) splitters.push_back(candidate);
  }
  const size_t buckets = splitters.size() + 1;
  const size_t chunks = 4 * workers;
  RawVector<u32> bucket_of(n);  // every slot written by its chunk below
  std::vector<size_t> counts(chunks * buckets, 0);
  size_t created = parallel_items(chunks, threads, [&](size_t c, size_t) {
    for (size_t i = n * c / chunks; i < n * (c + 1) / chunks; ++i) {
      const size_t b = static_cast<size_t>(
          std::upper_bound(splitters.begin(), splitters.end(), values[i], less) - splitters.begin());
      bucket_of[i] = static_cast<u32>(b);
      ++counts[c * buckets + b];
    }
  });
  // Bucket-major offsets: bucket b occupies [first[b], first[b+1]).
  std::vector<size_t> offset(chunks * buckets), first(buckets + 1);
  size_t at = 0;
  for (size_t b = 0; b < buckets; ++b) {
    first[b] = at;
    for (size_t c = 0; c < chunks; ++c) { offset[c * buckets + b] = at; at += counts[c * buckets + b]; }
  }
  first[buckets] = n;
  auto scattered = std::make_unique_for_overwrite<T[]>(n);
  created = std::max(created, parallel_items(chunks, threads, [&](size_t c, size_t) {
    size_t* cursor = offset.data() + c * buckets;
    for (size_t i = n * c / chunks; i < n * (c + 1) / chunks; ++i) scattered[cursor[bucket_of[i]]++] = values[i];
  }));
  created = std::max(created, parallel_items(buckets, threads, [&](size_t b, size_t) {
    std::copy(scattered.get() + first[b], scattered.get() + first[b + 1], values.begin() + first[b]);
#if defined(MHGP9_PARALLEL_SORT_MUTANT_UNSORTED_BUCKET)
    if (b == buckets / 2) return;  // mutant: one bucket left unsorted
#endif
    std::sort(values.begin() + first[b], values.begin() + first[b + 1], less);
  }));
  return created;
}

}  // namespace mhgp9::tower
