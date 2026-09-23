#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>
#include "tower/forest/ball_data.hpp"
using mhgp9::tower::BallData;
int main(int argc, char** argv) {
  const std::size_t n = std::strtoull(argv[1], nullptr, 10);
  using C = std::chrono::steady_clock;
  for (int rep = 0; rep < 2; ++rep) {
    auto t0 = C::now();
    double a, f;
    {
      std::vector<BallData> v(n);            // chemin de la chaine (tower_chain.cpp:450)
      auto t1 = C::now();
      a = std::chrono::duration<double, std::milli>(t1 - t0).count();
      volatile auto s = v[n / 2].n_shell; (void)s;
      t0 = C::now();
    }
    f = std::chrono::duration<double, std::milli>(C::now() - t0).count();
    std::printf("sizeof=%zu n=%zu bytes=%zu value_init_ms=%.1f free_ms=%.1f\n", sizeof(BallData), n, n * sizeof(BallData), a, f);
  }
}
