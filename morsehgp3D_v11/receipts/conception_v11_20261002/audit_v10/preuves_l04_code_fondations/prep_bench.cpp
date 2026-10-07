// L04 audit : decomposition du cout de preparation (prepare_cloud + SiteTree), minimum sur R repetitions (machine partagee).
// Les etapes de prepare_cloud sont rejouees a l'identique (memes algorithmes que src/cloud/cloud.cpp) pour les chronometrer une a une.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <vector>

#include "cloud/site_tree.hpp"

using namespace mhgp10;
using clk = std::chrono::steady_clock;

template <class F>
double best_ms(int reps, F&& f) {
  double best = 1e300;
  for (int r = 0; r < reps; ++r) {
    const auto t0 = clk::now();
    f();
    best = std::min(best, std::chrono::duration<double, std::milli>(clk::now() - t0).count());
  }
  return best;
}

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::vector<u32> raw;
  u32 buf[3];
  while (std::fread(buf, 4, 3, f) == 3) raw.insert(raw.end(), buf, buf + 3);
  std::fclose(f);
  const u32 n = u32(raw.size() / 3);
  std::vector<u32> x(n), y(n), z(n), pid(n);
  for (u32 i = 0; i < n; ++i) {
    x[i] = raw[3 * i];
    y[i] = raw[3 * i + 1];
    z[i] = raw[3 * i + 2];
    pid[i] = i;
  }
  const int R = 25;
  u64 sink = 0;
  const double t_total = best_ms(R, [&] {
    auto r = prepare_cloud(x, y, z, pid, kCoordinateBits);
    sink += r.value().sites();
  });
  const double t_dup = best_ms(R, [&] {
    std::vector<u32> sorted(pid.begin(), pid.end());
    std::sort(sorted.begin(), sorted.end());
    sink += std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end();
  });
  std::vector<u64> key(n);
  const double t_key = best_ms(R, [&] {
    for (u64 i = 0; i < n; ++i) key[i] = morton3(x[i], y[i], z[i]);
  });
  std::vector<u32> order(n);
  const double t_sort = best_ms(R, [&] {
    std::iota(order.begin(), order.end(), 0u);
    std::sort(order.begin(), order.end(), [&](u32 a, u32 b) {
      if (key[a] != key[b]) return key[a] < key[b];
      return pid[a] < pid[b];
    });
  });
  // variante de reference : tri de paires (cle, indice) contigues, meme ordre total (pid == indice ici)
  std::vector<std::pair<u64, u32>> kv(n);
  const double t_sort_pairs = best_ms(R, [&] {
    for (u32 i = 0; i < n; ++i) kv[i] = {key[i], pid[i]};
    std::sort(kv.begin(), kv.end());
  });
  auto r = prepare_cloud(x, y, z, pid, kCoordinateBits);
  const Cloud& c = r.value();
  const double t_tree = best_ms(R, [&] {
    SiteTree tree(c);
    sink += tree.cloud().sites();
  });
  std::printf("n=%u sites=%u (min sur %d repetitions, 1 fil)\n", n, c.sites(), R);
  std::printf("prepare_cloud total            : %7.3f ms\n", t_total);
  std::printf("  controle des PointId (tri)   : %7.3f ms\n", t_dup);
  std::printf("  cles de Morton               : %7.3f ms\n", t_key);
  std::printf("  tri indirect (cle, PointId)  : %7.3f ms\n", t_sort);
  std::printf("  [variante] tri de paires     : %7.3f ms\n", t_sort_pairs);
  std::printf("SiteTree (construction)        : %7.3f ms\n", t_tree);
  std::printf("preparation totale             : %7.3f ms\n", t_total + t_tree);
  return int(sink == 0);
}
