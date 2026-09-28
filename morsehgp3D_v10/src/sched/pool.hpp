// Ordonnanceur unique de la v10 : un Pool par Session, aucun fil ailleurs.
//
// Regles de determinisme : chaque tache ecrit a des positions fixees par son ordinal (jamais par
// l'ordre d'arrivee) ; les concatenations passent par des prefixes ; pas de parallelisme imbrique
// (un parallel_for appele depuis un ouvrier s'execute en serie et le signale).
#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "core/types.hpp"

namespace mhgp10::sched {

class Pool {
 public:
  // threads = 0 : std::thread::hardware_concurrency().
  explicit Pool(unsigned threads = 0);
  ~Pool();
  Pool(const Pool&) = delete;
  Pool& operator=(const Pool&) = delete;

  unsigned size() const { return static_cast<unsigned>(workers_.size()) + 1; }

  // Execute body(begin, end, worker) sur [0, n) par tranches de `grain` ; le fil appelant participe.
  // Les tranches sont distribuees dynamiquement, mais body ne doit ecrire qu'en fonction de [begin, end).
  void parallel_for(u64 n, u64 grain, const std::function<void(u64, u64, unsigned)>& body);

  // Nombre d'appels imbriques executes en serie depuis la creation (0 attendu).
  u64 nested_calls() const { return nested_.load(std::memory_order_relaxed); }

 private:
  void worker_loop(unsigned id);
  void run_chunks(unsigned id);

  std::vector<std::thread> workers_;
  std::mutex mutex_;
  std::condition_variable wake_;
  std::condition_variable done_;
  const std::function<void(u64, u64, unsigned)>* job_ = nullptr;
  u64 n_ = 0, grain_ = 1;
  std::atomic<u64> next_{0};
  unsigned active_ = 0;
  u64 generation_ = 0;
  bool stop_ = false;
  std::atomic<u64> nested_{0};
};

// Vrai dans un fil ouvrier du Pool (ou dans l'appelant pendant un parallel_for).
bool in_parallel_region();

}  // namespace mhgp10::sched
