// MorseHGP3D v5 — parallelisme MESURE, jamais declare.
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
#include <thread>
#include <vector>

#include "../core/mutants.hpp"
#include "../core/types.hpp"

namespace mhgp5 {

inline size_t planned_workers(size_t items, int threads) {
  if (threads <= 1 || items <= 1) return 1;
  if (MHGP5_MUTANT("parallel-one-worker")) return 1;
  return std::min((size_t)threads, items);
}

// CONTRAT D'EXCEPTION (commun aux deux primitives) : si `fn` leve dans un
// ouvrier, la PREMIERE exception est capturee, l'arret des nouveaux travaux
// est demande, TOUS les fils sont joints, puis l'exception est relancee dans
// le fil appelant — jamais std::terminate. Les travaux deja commences dans
// d'autres ouvriers se terminent ; leurs resultats sont abandonnes par
// l'appelant avec l'exception.
namespace parallel_detail {
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
  std::vector<std::thread> pool;
  pool.reserve(T);
  for (size_t t = 0; t < T; ++t)
    pool.emplace_back([&, t] {
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
  for (auto& th : pool) th.join();
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
  std::vector<std::thread> pool;
  pool.reserve(T);
  for (size_t t = 0; t < T; ++t)
    pool.emplace_back([&, t] {
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
  for (auto& th : pool) th.join();
  fx.rethrow_if_any();
  return T;
}

}  // namespace mhgp5
