// Ordre de grandeur local (hors G4) : value-init serie d'un catalogue de B BallData
// (224 o) et liberation de 192 plages totalisant P presentations (112 o).
#include <array>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <sys/resource.h>
#include <vector>
using i128 = __int128;
struct Key { i128 a; i128 b[3]; i128 c; };
struct Level { std::uint64_t num[3]; i128 den; };
struct BallData { Key key; Level level; std::uint8_t arity = 0; std::uint8_t ni = 0, ns = 0;
  std::int32_t in[9] = {}; std::int32_t sh[12] = {}; };
struct Presentation { std::array<i128, 5> key; std::uint8_t arity = 0; std::array<std::uint32_t, 4> support{};
  std::uint32_t depth = 0, shell = 0; };
static double cpu() { timespec t; clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t); return t.tv_sec * 1e3 + t.tv_nsec / 1e6; }
static double wall() { timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1e3 + t.tv_nsec / 1e6; }
static long minflt() { rusage r; getrusage(RUSAGE_SELF, &r); return r.ru_minflt; }
int main() {
  static_assert(sizeof(BallData) == 224 && sizeof(Presentation) == 112);
  const std::size_t B = 4383302, P = 4383304, R = 192;  // 08/000100/K10 (probe_12)
  // plages de presentations (touchees), puis liberation
  std::vector<std::vector<Presentation>> ranges(R);
  for (std::size_t r = 0; r < R; ++r) { ranges[r].reserve(P / R + 1); ranges[r].resize(P / R + (r < P % R)); }
  double c0 = cpu(), w0 = wall(); long f0 = minflt();
  std::vector<BallData> balls(B);
  double c1 = cpu(), w1 = wall(); long f1 = minflt();
  { auto tmp = std::move(ranges); }
  double c2 = cpu(), w2 = wall();
  volatile std::uint8_t sink = balls[B / 2].arity; (void)sink;
  { auto tmp = std::move(balls); }
  double c3 = cpu(), w3 = wall();
  std::printf("{\"bytes_catalogue\":%zu,\"init_cpu_ms\":%.1f,\"init_wall_ms\":%.1f,\"init_minflt\":%ld,"
              "\"free_ranges_cpu_ms\":%.1f,\"free_ranges_wall_ms\":%.1f,\"free_catalogue_cpu_ms\":%.1f,\"free_catalogue_wall_ms\":%.1f}\n",
              B * sizeof(BallData), c1 - c0, w1 - w0, f1 - f0, c2 - c1, w2 - w1, c3 - c2, w3 - w2);
}
