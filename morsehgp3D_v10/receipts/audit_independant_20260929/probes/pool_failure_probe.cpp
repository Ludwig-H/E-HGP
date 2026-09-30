// Sonde d'audit : une allocation qui echoue dans un ouvrier doit pouvoir fermer l'appel proprement.
#include <atomic>
#include <cstdio>
#include <new>
#include <thread>
#include "sched/pool.hpp"

int main() {
  mhgp10::sched::Pool pool(2);
  std::atomic<bool> worker_seen{false};
  try {
    pool.parallel_for(64, 1, [&](mhgp10::u64, mhgp10::u64, unsigned worker) {
      if (worker != 0) {
        worker_seen.store(true, std::memory_order_release);
        throw std::bad_alloc();
      }
      while (!worker_seen.load(std::memory_order_acquire)) std::this_thread::yield();
    });
  } catch (const std::bad_alloc&) {
    std::puts("allocation_failure_propagated_after_join");
    return 0;
  }
  std::puts("unexpected_no_failure");
  return 1;
}
