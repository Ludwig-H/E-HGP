// L04 audit : parallel_for(n, grain) avec un grain >= 2^63 : le compteur atomique `next` reboucle modulo 2^64.
#include <atomic>
#include <cstdio>

#include "sched/pool.hpp"

using namespace mhgp10;

int main() {
  int worst = 0;
  for (int rep = 0; rep < 2000; ++rep) {
    sched::Pool pool(2);
    std::atomic<int> runs{0};
    pool.parallel_for(2, u64{1} << 63, [&](u64 b, u64 e, unsigned) {
      if (b == 0 && e == 2) runs.fetch_add(1);
      for (volatile int i = 0; i < 20000; i = i + 1) {}
    });
    worst = std::max(worst, runs.load());
  }
  std::printf("executions de la tranche [0, 2) dans un meme appel (max sur 2000 essais) : %d\n", worst);
  return worst > 1 ? 1 : 0;
}
