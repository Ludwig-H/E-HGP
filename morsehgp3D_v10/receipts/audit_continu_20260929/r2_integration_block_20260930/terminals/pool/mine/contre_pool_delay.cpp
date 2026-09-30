// Variante de diagnostic de contre_pool.cpp (audit independant) : le fil qui leve attend D ms apres la barriere
// avant de lever (D = argv[1]). La barriere de 4 fils est remplacee par une barriere a attente bornee (2 s) qui
// compte les expirations au lieu de s'interbloquer. Montre que la sonde d'origine suppose la capture en moins de
// 5 ms : au-dela, les trois autres fils reclament (legitimement, avant la capture) une tranche qui attend un
// quatrieme fil sorti du travail.
#include "sched/pool.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <thread>

using namespace mhgp10;

struct TimedBarrier {  // barriere a phases, attente bornee
  std::atomic<unsigned> arrived{0}, phase{0};
  std::atomic<unsigned> timeouts{0};
  unsigned n;
  explicit TimedBarrier(unsigned k) : n(k) {}
  void arrive_and_wait() {
    const unsigned ph = phase.load();
    if (arrived.fetch_add(1) + 1 == n) {
      arrived.store(0);
      phase.fetch_add(1);
      return;
    }
    const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (phase.load() == ph) {
      if (std::chrono::steady_clock::now() > limit) { timeouts.fetch_add(1); return; }
      std::this_thread::yield();
    }
  }
};

int main(int argc, char** argv) {
  const int delay_ms = argc > 1 ? std::atoi(argv[1]) : 0;
  sched::Pool pool(4);
  unsigned total_timeouts = 0;
  for (unsigned thrower : {0u, 1u}) {
    TimedBarrier started(4);
    bool caught = false;
    try {
      pool.parallel_for(64, 1, [&](u64, u64, unsigned id) {
        started.arrive_and_wait();
        if (id == thrower) {
          std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
          throw std::bad_alloc();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      });
    } catch (const std::bad_alloc&) {
      caught = true;
    }
    std::printf("delai %d ms lanceur %u : relancee %d, attentes de barriere expirees %u\n", delay_ms, thrower, caught,
                started.timeouts.load());
    total_timeouts += started.timeouts.load();
  }
  return total_timeouts ? 1 : 0;
}
