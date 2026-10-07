// L04 audit : part de la memoire reellement comptee par MemoryBudget (Buffer<T>) dans une construction complete.
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "tower/tower.hpp"

using namespace mhgp10;

static unsigned long long hwm_kib() {
  FILE* f = std::fopen("/proc/self/status", "r");
  if (!f) return 0;
  char line[256];
  unsigned long long v = 0;
  while (std::fgets(line, sizeof line, f))
    if (std::strncmp(line, "VmHWM:", 6) == 0) v = std::strtoull(line + 6, nullptr, 10);
  std::fclose(f);
  return v;
}

int main(int argc, char** argv) {
  if (argc < 4) return 2;
  const int kmax = std::atoi(argv[2]);
  const unsigned threads = unsigned(std::atoi(argv[3]));
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
  using clk = std::chrono::steady_clock;
  auto ms = [](clk::time_point a, clk::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); };
  const unsigned long long hwm0 = hwm_kib();
  const auto t0 = clk::now();
  auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
  const auto t1 = clk::now();
  if (!prepared.ok()) return 2;
  const Cloud& cloud = prepared.value();
  const u64 counted_cloud = default_budget().peak();
  sched::Pool pool(threads);
  const auto t2 = clk::now();
  SiteTree tree(cloud);
  const auto t3 = clk::now();
  CatalogueParams cp;
  cp.kmax = kmax;
  auto cat = build_catalogue(cloud, cp, pool);
  const auto t4 = clk::now();
  if (!cat.ok()) return 3;
  const u64 counted_cat = default_budget().peak();
  const unsigned long long hwm_cat = hwm_kib();
  TowerParams tp;
  tp.kmax = kmax;
  tp.points = false;
  auto tw = build_tower(cloud, tree, cat.value(), tp, pool);
  const auto t5 = clk::now();
  if (!tw.ok()) return 3;
  const unsigned long long hwm_tow = hwm_kib();
  std::printf("n=%u sites=%u K=%d threads=%u balls=%u\n", n, cloud.sites(), kmax, pool.size(), cat.value().balls());
  std::printf("temps_ms : prepare_cloud=%.2f pool=%.2f site_tree=%.2f catalogue=%.1f tour=%.1f\n", ms(t0, t1), ms(t1, t2), ms(t2, t3),
              ms(t3, t4), ms(t4, t5));
  std::printf("octets COMPTES par MemoryBudget (pic) : apres nuage=%llu, apres catalogue=%llu, apres tour=%llu ; en usage a la fin=%llu\n",
              (unsigned long long)counted_cloud, (unsigned long long)counted_cat, (unsigned long long)default_budget().peak(),
              (unsigned long long)default_budget().used());
  std::printf("memoire resident REELLE (VmHWM, kio) : avant=%llu apres catalogue=%llu apres tour=%llu\n", hwm0, hwm_cat, hwm_tow);
  std::printf("part comptee = %.2f %% du pic resident ajoute\n",
              100.0 * double(default_budget().peak()) / (1024.0 * double(hwm_tow - hwm0)));
  return 0;
}
