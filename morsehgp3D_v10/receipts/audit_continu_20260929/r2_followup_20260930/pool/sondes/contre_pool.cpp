#include "sched/pool.hpp"

#include <array>
#include <atomic>
#include <barrier>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace mhgp10;

int main() {
  sched::Pool pool(4);
  unsigned verified = 0;
  for (unsigned thrower : {0u, 1u}) {
    for (bool nested : {false, true}) {
      std::barrier started(4);
      std::atomic<unsigned> active{0};
      bool caught = false;
      try {
        pool.parallel_for(64, 1, [&](u64, u64, unsigned id) {
          active.fetch_add(1);
          started.arrive_and_wait();
          if (id == thrower) {
            active.fetch_sub(1);
            if (nested) {
              pool.parallel_for(1, 1, [](u64, u64, unsigned) { throw std::bad_alloc(); });
            } else {
              throw std::bad_alloc();
            }
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
          active.fetch_sub(1);
        });
      } catch (const std::bad_alloc&) {
        caught = true;
      }
      if (!caught || active.load() != 0 || sched::in_parallel_region()) return 1;
      std::array<std::atomic<unsigned>, 64> visits{};
      pool.parallel_for(64, 3, [&](u64 b, u64 e, unsigned) {
        for (u64 i = b; i < e; ++i) visits[i].fetch_add(1);
      });
      for (auto& count : visits) if (count.load() != 1) return 2;
      if (sched::in_parallel_region()) return 3;
      ++verified;
    }
  }
  std::cout << "PASS " << verified << " exceptions appelant/ouvrier, directes/imbriquees; quiescence, TLS, reutilisation\n";
}
