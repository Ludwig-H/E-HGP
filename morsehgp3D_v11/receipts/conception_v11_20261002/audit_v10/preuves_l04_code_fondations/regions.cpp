// L04 audit : appels systeme futex par region parallele (compte par strace -f -c), corps triviaux.
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include "sched/pool.hpp"
using namespace mhgp10;
int main(int argc, char** argv) {
  const unsigned P = argc > 1 ? unsigned(std::atoi(argv[1])) : 48;
  const int regions = argc > 2 ? std::atoi(argv[2]) : 2000;
  sched::Pool pool(P);
  std::atomic<unsigned long long> sink{0};
  for (int r = 0; r < regions; ++r)
    pool.parallel_for(P, 1, [&](u64 b, u64, unsigned) {
      unsigned long long x = b;
      for (int i = 0; i < 2000; ++i) x = x * 6364136223846793005ull + 1442695040888963407ull;  // ~2 us de calcul
      sink.fetch_add(x, std::memory_order_relaxed);
    });
  std::printf("P=%u regions=%d\n", P, regions);
  return int(sink.load() == 1);
}
