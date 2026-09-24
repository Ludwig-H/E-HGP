// Audit-only reproducer for local E2 commit b37b49504; not a product gate.
// Build from the E2 worktree root:
// g++ -std=c++20 -O0 -Wall -Wextra -Werror -pthread -I . SOURCE -o /tmp/mhgp9_pool_e2_probe
#include <barrier>
#include <cstdio>
#include <stdexcept>

#include "morsehgp3D_v9/src/tower/parallel/pool.hpp"

int main() {
  namespace pd = mhgp9::tower::parallel_detail;
  pd::TaskPool pool(2);
  pd::PoolScope scope(&pool);
  std::barrier both_ready(2);
  auto both_throw = [&](std::size_t i) {
    both_ready.arrive_and_wait();
    throw std::runtime_error(i == 0 ? "owner_A" : "worker_B");
  };
  try {
    pd::run_threads(2, both_throw);
  } catch (const std::exception& e) {
    std::printf("first=%s\n", e.what());
  }
  auto clean = [](std::size_t) {};
  try {
    pd::run_threads(2, clean);
    std::puts("second=success");
  } catch (const std::exception& e) {
    std::printf("second=%s\n", e.what());
    return 1;  // a successful next generation must not inherit an old error
  }
  return 0;
}
