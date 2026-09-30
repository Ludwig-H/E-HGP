// Variante bornee de audit_continu_20260929/pool_head/pool_throw.cpp (seule difference : l'attente de l'ouvrier sur
// `release` est bornee a 2 s, et l'on note s'il a vu `release`). Sur un pool correct, la sonde d'origine s'interbloque
// par construction : l'ouvrier n'y sort que lorsque l'appelant, dans son catch, pose `release`, alors que parallel_for
// doit attendre la sortie de l'ouvrier avant de relancer. Ici : l'ouvrier sort seul apres 2 s ; il ne doit jamais voir
// `release` (pose apres le retour de parallel_for) tant qu'il est dans le rappel.
// Code 0 : l'appelant n'a repris la main qu'apres la sortie de l'ouvrier ; 1 : sortie anticipee (ou ASan).
#include <atomic>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include "sched/pool.hpp"

int main() {
  std::atomic<bool> entered{false}, release{false}, saw_release{false};
  mhgp10::sched::Pool pool(2);
  try {
    pool.parallel_for(100, 1, [&](mhgp10::u64, mhgp10::u64, unsigned worker) {
      if (worker == 0) {
        while (!entered.load()) std::this_thread::yield();
        throw std::runtime_error("deliberate caller exception");
      }
      entered.store(true);
      const auto t0 = std::chrono::steady_clock::now();
      while (!release.load() && std::chrono::steady_clock::now() - t0 < std::chrono::seconds(2)) std::this_thread::yield();
      if (release.load()) saw_release.store(true);
    });
  } catch (const std::exception&) {
    std::puts("caller caught callback exception");
    release.store(true);
  }
  const bool ok = !saw_release.load();
  std::printf("%s\n", ok ? "caller_resumed_after_worker_left" : "caller_resumed_while_worker_inside");
  return ok ? 0 : 1;
}
