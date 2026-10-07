// L04 audit : ordre de grandeur d'un tri par base des couples (cle de Morton 54 bits, PointId) contre le tri indirect de
// prepare_cloud ; meme ordre total (cle, PointId). Minimum sur 25 repetitions, 1 fil. Indicatif (machine chargee).
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <numeric>
#include <vector>

#include "cloud/cloud.hpp"

using namespace mhgp10;
using clk = std::chrono::steady_clock;

struct KV {
  u64 key;
  u32 id;
};

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
  std::vector<u64> key(n);
  for (u32 i = 0; i < n; ++i) key[i] = morton3(x[i], y[i], z[i]);
  std::vector<u32> order(n), ref(n);
  double t_std = 1e300, t_radix = 1e300;
  for (int r = 0; r < 25; ++r) {
    const auto t0 = clk::now();
    std::iota(order.begin(), order.end(), 0u);
    std::sort(order.begin(), order.end(), [&](u32 a, u32 b) { return key[a] != key[b] ? key[a] < key[b] : pid[a] < pid[b]; });
    t_std = std::min(t_std, std::chrono::duration<double, std::milli>(clk::now() - t0).count());
  }
  ref = order;
  std::vector<KV> a(n), b(n);
  std::vector<u32> got(n);
  for (int r = 0; r < 25; ++r) {
    const auto t0 = clk::now();
    // entree presentee par PointId croissant (ici pid == rang) : un tri par base stable sur la cle donne l'ordre (cle, PointId)
    for (u32 i = 0; i < n; ++i) a[i] = KV{key[i], i};
    for (int pass = 0; pass < 5; ++pass) {  // 5 passes de 11 bits : 55 bits >= 54 bits de cle (18 bits par axe)
      u32 count[2048] = {0};
      const int sh = 11 * pass;
      for (u32 i = 0; i < n; ++i) ++count[(a[i].key >> sh) & 2047];
      u32 sum = 0;
      for (u32& c : count) {
        const u32 t = c;
        c = sum;
        sum += t;
      }
      for (u32 i = 0; i < n; ++i) b[count[(a[i].key >> sh) & 2047]++] = a[i];
      a.swap(b);
    }
    for (u32 i = 0; i < n; ++i) got[i] = a[i].id;
    t_radix = std::min(t_radix, std::chrono::duration<double, std::milli>(clk::now() - t0).count());
  }
  std::printf("n=%u : tri indirect std::sort %.3f ms ; tri par base (5 passes de 11 bits) %.3f ms ; rapport %.1f ; meme permutation : %s\n", n,
              t_std, t_radix, t_std / t_radix, got == ref ? "oui" : "NON");
  return got == ref ? 0 : 1;
}
