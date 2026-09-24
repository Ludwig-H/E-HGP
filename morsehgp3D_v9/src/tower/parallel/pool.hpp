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
