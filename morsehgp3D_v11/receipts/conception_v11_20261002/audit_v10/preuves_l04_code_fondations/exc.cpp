// L04 audit : comportement du Pool quand le corps leve une exception (std::bad_alloc d'un std::vector, par exemple).
//   exc worker : l'exception part d'un fil ouvrier ; exc caller : du fil appelant ; exc none : temoin.
#include <atomic>
#include <cstdio>
#include <cstring>
#include <new>
#include <thread>
#include <chrono>

#include "sched/pool.hpp"

using namespace mhgp10;

int main(int argc, char** argv) {
  const char* mode = argc > 1 ? argv[1] : "none";
  sched::Pool pool(4);
  std::atomic<int> done{0};
  try {
    pool.parallel_for(4000, 1, [&](u64 b, u64, unsigned wk) {
      std::this_thread::sleep_for(std::chrono::microseconds(50));
      if (!std::strcmp(mode, "worker") && wk != 0 && b > 100) throw std::bad_alloc();
      if (!std::strcmp(mode, "caller") && wk == 0 && b > 100) throw std::bad_alloc();
      done.fetch_add(1);
    });
  } catch (const std::bad_alloc&) {
    std::printf("exception rattrapee par l'appelant apres %d tranches ; in_parallel_region()=%d\n", done.load(),
                int(sched::in_parallel_region()));
    // l'appelant croit le Pool utilisable : second travail
    std::atomic<int> second{0};
    pool.parallel_for(1000, 1, [&](u64, u64, unsigned) { second.fetch_add(1); });
    std::printf("second travail : %d tranches sur 1000, appels imbriques comptes = %llu\n", second.load(),
                (unsigned long long)pool.nested_calls());
    return 5;
  }
  std::printf("termine sans exception : %d tranches\n", done.load());
  return 0;
}
