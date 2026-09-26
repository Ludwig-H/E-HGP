#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <vector>
#include <type_traits>
#include "tower/forest/ball_data.hpp"
using mhgp9::tower::BallData;
static double tcpu() { timespec t; clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t); return t.tv_sec * 1e3 + t.tv_nsec / 1e6; }
static double pcpu() { timespec t; clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t); return t.tv_sec * 1e3 + t.tv_nsec / 1e6; }
int main(int argc, char** argv) {
  const size_t n = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 1306696;
  const int reps = argc > 2 ? std::atoi(argv[2]) : 3;
  std::printf("sizeof(BallData)=%zu trivially_default=%d n=%zu bytes=%.1f MB\n", sizeof(BallData),
              (int)std::is_trivially_default_constructible_v<BallData>, n, n * sizeof(BallData) / 1e6);
  for (int r = 0; r < reps; ++r) {
    auto w0 = std::chrono::steady_clock::now(); double c0 = tcpu();
    std::vector<BallData> balls(n);
    double c1 = tcpu(); auto w1 = std::chrono::steady_clock::now();
    volatile int sink = balls[n / 2].arity; (void)sink;
    double c2 = tcpu(); auto w2 = std::chrono::steady_clock::now();
    { std::vector<BallData>().swap(balls); }
    double c3 = tcpu(); auto w3 = std::chrono::steady_clock::now();
    std::printf("serial value-init: thread-cpu %.1f ms wall %.1f ms | free: cpu %.1f wall %.1f\n", c1 - c0,
                std::chrono::duration<double, std::milli>(w1 - w0).count(), c3 - c2,
                std::chrono::duration<double, std::milli>(w3 - w2).count());
  }
  // parallel first touch (8 threads here), for scale only
  for (int r = 0; r < reps; ++r) {
    const int T = 8;
    auto w0 = std::chrono::steady_clock::now(); double p0 = pcpu();
    BallData* p = static_cast<BallData*>(std::malloc(n * sizeof(BallData)));
    std::vector<std::thread> th;
    for (int t = 0; t < T; ++t) th.emplace_back([=] { for (size_t i = n * t / T; i < n * (t + 1) / T; ++i) new (p + i) BallData(); });
    for (auto& x : th) x.join();
    double p1 = pcpu(); auto w1 = std::chrono::steady_clock::now();
    std::free(p);
    std::printf("parallel(8) first-touch init: process-cpu %.1f ms wall %.1f ms\n", p1 - p0, std::chrono::duration<double, std::milli>(w1 - w0).count());
  }
}
