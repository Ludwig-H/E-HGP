// Copie instrumentee de point_dendrogram (tower.cpp) : temps par phase. Hors depot.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "tower/tower.hpp"

using namespace mhgp10;
using clk = std::chrono::steady_clock;
static double sec(clk::time_point a, clk::time_point b) { return std::chrono::duration<double>(b - a).count(); }

int main(int argc, char** argv) {
  const int K = std::stoi(argv[2]);
  FILE* fp = std::fopen(argv[1], "rb");
  std::vector<u32> raw;
  u32 buf[3];
  while (std::fread(buf, 4, 3, fp) == 3) raw.insert(raw.end(), buf, buf + 3);
  std::fclose(fp);
  const u32 n = static_cast<u32>(raw.size() / 3);
  std::vector<u32> x(n), y(n), z(n), pid(n);
  for (u32 i = 0; i < n; ++i) x[i] = raw[3 * i], y[i] = raw[3 * i + 1], z[i] = raw[3 * i + 2], pid[i] = i;
  auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
  const Cloud& cloud = prepared.value();
  sched::Pool pool(8);
  SiteTree tree(cloud);
  CatalogueParams catp;
  catp.kmax = K;
  auto catr = build_catalogue(cloud, catp, pool);
  const Catalogue& cat = catr.value();
  TowerParams tp;
  tp.kmax = K;
  tp.only_order = K;
  tp.entry = PointEntry::cover;
  auto tw = build_tower(cloud, tree, cat, tp, pool);
  const OrderForest& f = tw.value().orders[K - 1];
  for (int rep = 0; rep < 3; ++rep) {
    auto t0 = clk::now();
    struct Key { bool from_cat; u32 rank; u64 e; };
    auto level_of = [&](const Key& k) -> geom::Level {
      if (k.from_cat) {
        if (k.rank == 0) return geom::Level{arith::I192{}, arith::I128w::from_u128(1)};
        return cat.level[k.rank - 1];
      }
      geom::Level L;
      L.num = arith::I192::from_u128(k.e);
      L.den = arith::I128w::from_u128(1);
      return L;
    };
    std::vector<Key> keys;
    for (u32 r : f.rank) keys.push_back({true, r, 0});
    for (u32 r : f.point_cat_rank) keys.push_back({true, r, 0});
    auto t1 = clk::now();
    struct Item { double x; u32 i; };
    std::vector<Item> items(keys.size());
    for (u32 i = 0; i < items.size(); ++i) items[i] = {level_of(keys[i]).approx(), i};
    auto t2 = clk::now();
    auto near = [](double a, double b) { return a == b || std::abs(a - b) <= 1e-9 * std::max(a, b); };
    u64 exact = 0;
    std::sort(items.begin(), items.end(), [&](const Item& a, const Item& b) {
      if (!near(a.x, b.x)) return a.x < b.x;
      ++exact;
      return geom::compare(level_of(keys[a.i]), level_of(keys[b.i])) < 0;
    });
    auto t3 = clk::now();
    std::vector<u32> merged(keys.size());
    std::vector<double> level;
    u64 exact2 = 0;
    for (u32 i = 0; i < items.size(); ++i) {
      const double xx = items[i].x;
      bool distinct = false;
      if (i > 0) {
        if (!near(items[i - 1].x, xx)) distinct = true;
        else { ++exact2; distinct = geom::compare(level_of(keys[items[i - 1].i]), level_of(keys[items[i].i])) != 0; }
      }
      if (i == 0 || (distinct && xx > level.back())) level.push_back(xx);
      merged[items[i].i] = static_cast<u32>(level.size() - 1);
    }
    auto t4 = clk::now();
    std::printf("{\"K\":%d,\"keys\":%zu,\"keys_s\":%.4f,\"approx_s\":%.4f,\"sort_s\":%.4f,\"merge_s\":%.4f,\"exact_sort\":%llu,\"exact_merge\":%llu,\"levels\":%zu}\n",
                K, keys.size(), sec(t0, t1), sec(t1, t2), sec(t2, t3), sec(t3, t4), (unsigned long long)exact, (unsigned long long)exact2, level.size());
  }
  return 0;
}
