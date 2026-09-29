#include <atomic>
#include <cstdio>
#include <stdexcept>
#include "sched/pool.hpp"

int main() {
  std::atomic<bool> entered{false}, release{false};
  mhgp10::sched::Pool pool(2);
  try {
    pool.parallel_for(100, 1, [&](mhgp10::u64, mhgp10::u64, unsigned worker) {
      if (worker == 0) {
        while (!entered.load()) std::this_thread::yield();
        throw std::runtime_error("deliberate caller exception");
      }
      entered.store(true);
      while (!release.load()) std::this_thread::yield();
    });
  } catch (const std::exception&) {
    std::puts("caller caught callback exception");
    release.store(true);
  }
}
