#include <cstdio>
#include <stdexcept>
#include <thread>
#include "sched/pool.hpp"
// Exception levee dans un fil OUVRIER (id != 0) : attendu std::terminate sur le code publie.
int main() {
  mhgp10::sched::Pool pool(2);
  try {
    pool.parallel_for(1000, 1, [&](mhgp10::u64, mhgp10::u64, unsigned worker) {
      if (worker != 0) throw std::runtime_error("deliberate worker exception");
      std::this_thread::yield();
    });
    std::puts("no exception reached the caller");
  } catch (const std::exception&) {
    std::puts("caller caught");
  }
}
