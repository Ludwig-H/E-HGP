// Informatif : debordement du compteur de tranches sans exception (grain proche de 2^63), ancien et nouveau pool.
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>
#include "sched/pool.hpp"
int main() {
  mhgp10::sched::Pool pool(8);
  long long dup = 0, runs = 0;
  for (int it = 0; it < 300; ++it) {
    std::atomic<unsigned> zero{0};
    pool.parallel_for(10, mhgp10::u64{1} << 63, [&](mhgp10::u64 b, mhgp10::u64, unsigned) {
      if (b == 0) zero.fetch_add(1);
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    });
    dup += zero.load() > 1;
    ++runs;
  }
  std::printf("wrap_grain executions ou la tranche 0 passe deux fois ou plus : %lld / %lld\n", dup, runs);  // tranche lente : les ouvriers entrent
  return 0;
}
