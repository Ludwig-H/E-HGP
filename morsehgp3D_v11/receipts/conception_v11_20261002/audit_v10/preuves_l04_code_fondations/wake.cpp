// L04 audit : cout fixe d'un parallel_for (reveil des ouvriers par condition_variable + mutex unique).
// Mesures : latence par appel d'un travail vide, commutations de contexte volontaires par appel (getrusage),
// et latence de PRISE EN CHARGE (delai avant que le dernier ouvrier ait commence), pour P fils.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <sys/resource.h>
#include <thread>
#include <vector>

#include "sched/pool.hpp"

using namespace mhgp10;
using Clock = std::chrono::steady_clock;

static long nvcsw() {
  rusage r{};
  getrusage(RUSAGE_SELF, &r);
  return r.ru_nvcsw;
}

int main(int argc, char** argv) {
  const unsigned P = argc > 1 ? unsigned(std::atoi(argv[1])) : 8;
  const int reps = argc > 2 ? std::atoi(argv[2]) : 20000;
  sched::Pool pool(P);
  std::atomic<u64> sink{0};
  // (1) travail vide d'une tranche : cout pur de publication + fermeture
  for (int warm = 0; warm < 2; ++warm) {
    const long c0 = nvcsw();
    const auto t0 = Clock::now();
    for (int r = 0; r < reps; ++r) pool.parallel_for(1, 1, [&](u64, u64, unsigned) { sink.fetch_add(1, std::memory_order_relaxed); });
    const double us = std::chrono::duration<double, std::micro>(Clock::now() - t0).count() / reps;
    if (warm) std::printf("P=%u vide_1_tranche : %.2f us/appel, %.2f commutations volontaires/appel\n", P, us, double(nvcsw() - c0) / reps);
  }
  // (2) travail de P tranches de ~20 us chacune (calcul) : temps ideal = 20 us ; mesure le surcout de reveil
  auto spin = [](double us) {
    const auto t = Clock::now();
    volatile u64 x = 0;
    while (std::chrono::duration<double, std::micro>(Clock::now() - t).count() < us) x = x + 1;
  };
  for (double work_us : {20.0, 200.0, 2000.0}) {
    const int r2 = work_us > 500 ? 300 : 3000;
    std::vector<double> lat;
    std::vector<unsigned> used;
    const long c0 = nvcsw();
    const auto t0 = Clock::now();
    for (int r = 0; r < r2; ++r) {
      std::atomic<unsigned> mask{0};
      pool.parallel_for(P, 1, [&](u64, u64, unsigned wk) {
        mask.fetch_or(1u << wk, std::memory_order_relaxed);
        spin(work_us);
      });
      used.push_back(unsigned(__builtin_popcount(mask.load())));
    }
    const double us = std::chrono::duration<double, std::micro>(Clock::now() - t0).count() / r2;
    double mean_used = 0;
    for (unsigned u : used) mean_used += u;
    mean_used /= double(used.size());
    std::printf("P=%u %u tranches de %.0f us : %.1f us/appel (ideal %.0f), fils distincts employes en moyenne %.2f, %.2f commutations/appel\n",
                P, P, work_us, us, work_us, mean_used, double(nvcsw() - c0) / r2);
  }
  return int(sink.load() == 0);
}
