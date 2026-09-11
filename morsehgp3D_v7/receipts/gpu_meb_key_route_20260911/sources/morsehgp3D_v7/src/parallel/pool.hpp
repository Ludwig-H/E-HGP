// MorseHGP3D v6 — parallelisme MESURE, jamais declare.
//
// Deux primitives, toutes deux a tirage dynamique et a fusion en ORDRE
// D'INDEX (jamais en ordre d'achevement) : la sortie est bit-identique au
// sequentiel quel que soit le nombre de fils. Chaque primitive RETOURNE le
// nombre d'ouvriers reellement crees : c'est cette valeur, et elle seule,
// que les compteurs publient (mutant `parallel-one-worker` : un seul fil
// cree quel que soit le budget — la metadonnee mesuree le trahit).
#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <mutex>
#include <system_error>
#include <thread>
#include <vector>

#include "../core/mutants.hpp"
#include "../core/types.hpp"

namespace mhgp7 {

inline size_t planned_workers(size_t items, int threads) {
  if (threads <= 1 || items <= 1) return 1;
  if (MHGP7_MUTANT("parallel-one-worker")) return 1;
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
#if defined(MHGP7_TESTING)
inline thread_local size_t launch_fail_after = (size_t)-1;
inline std::atomic<size_t> launch_started{0};
inline std::atomic<size_t> launch_active{0};
#endif

struct JoinThreads {
  std::vector<std::thread>& threads;
  ~JoinThreads() {
    for (auto& th : threads)
      if (th.joinable()) th.join();
  }
};

template <typename Fn>
inline void run_threads(size_t count, Fn&& fn) {
  std::atomic<unsigned> admission{0};  // 0 waiting, 1 admitted, 2 cancelled
  std::vector<std::thread> pool;
  pool.reserve(count);
  JoinThreads joined{pool};
  try {
    for (size_t t = 0; t < count; ++t) {
#if defined(MHGP7_TESTING)
      if (t == launch_fail_after) {
        while (launch_started.load() < t) std::this_thread::yield();
        throw std::system_error(std::make_error_code(std::errc::resource_unavailable_try_again));
      }
#endif
      pool.emplace_back([&, t] {
#if defined(MHGP7_TESTING)
        launch_active.fetch_add(1);
        launch_started.fetch_add(1);
        struct ActiveGuard {
          ~ActiveGuard() { launch_active.fetch_sub(1); }
        } active_guard;
#endif
        admission.wait(0);
        if (admission.load() == 1) fn(t);
      });
    }
  } catch (...) {
    admission.store(MHGP7_MUTANT("parallel-admit-partial-launch") ? 1u : 2u);
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
// le nombre d'ouvriers crees (1 = sequentiel, aucun fil).
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

}  // namespace mhgp7
