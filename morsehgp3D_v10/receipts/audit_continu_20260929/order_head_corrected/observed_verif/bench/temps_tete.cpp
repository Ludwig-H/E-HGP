// Rejeu relatif (verificateur adverse) des temps de la tete : copies de a10605a06 (ref_base.inc) contre la
// bibliotheque liee, serie (pool nul : H1/H2) et Pool de P fils (v4), dans le meme processus, ordre tourne a chaque
// repetition. Medianes et minima.   temps_tete IN.u32le K cover|core REPS P [mcs z]
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "head/head.hpp"
#include "tower/tower.hpp"

#include "ref_base.inc"

using namespace mhgp10;
using clk = std::chrono::steady_clock;

int main(int argc, char** argv) {
  if (argc < 6) return 2;
  const int K = std::stoi(argv[2]);
  const bool cover = std::string(argv[3]) == "cover";
  const int reps = std::stoi(argv[4]);
  const unsigned P = unsigned(std::stoul(argv[5]));
  ClusterParams cp;
  cp.min_cluster_size = argc > 6 ? std::stoull(argv[6]) : 200;
  cp.z = argc > 7 ? std::stod(argv[7]) : 3;
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::vector<u32> raw;
  u32 buf[3];
  while (std::fread(buf, 4, 3, f) == 3) raw.insert(raw.end(), buf, buf + 3);
  std::fclose(f);
  const u32 n = static_cast<u32>(raw.size() / 3);
  std::vector<u32> x(n), y(n), z(n), pid(n);
  for (u32 i = 0; i < n; ++i) x[i] = raw[3 * i], y[i] = raw[3 * i + 1], z[i] = raw[3 * i + 2], pid[i] = i;
  auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
  if (!prepared.ok()) return 2;
  const Cloud& cloud = prepared.value();
  sched::Pool pool(P);
  SiteTree tree(cloud);
  CatalogueParams catp;
  catp.kmax = K;
  auto cat = build_catalogue(cloud, catp, pool);
  if (!cat.ok()) return 2;
  TowerParams tp;
  tp.kmax = K;
  tp.only_order = K;
  tp.entry = cover ? PointEntry::cover : PointEntry::core;
  auto tw = build_tower(cloud, tree, cat.value(), tp, pool);
  if (!tw.ok()) return 2;
  const OrderForest& forest = tw.value().orders[K - 1];
  // variantes : 0 reference, 1 bibliotheque serie, 2 bibliotheque Pool
  std::vector<double> t_pd[3], t_va[3], t_cd[3], t_all[3];
  auto sec = [](clk::time_point a, clk::time_point b) { return std::chrono::duration<double>(b - a).count(); };
  double sink = 0;
  for (int rep = 0; rep < reps; ++rep)
    for (int k = 0; k < 3; ++k) {
      const int v = (k + rep) % 3;
      sched::Pool* pp = v == 2 ? &pool : nullptr;
      const auto a0 = clk::now();
#ifdef SANS_POOL  // bibliotheque v2 (A1 + H1 + H2, serie) : la variante « pool » rejoue la serie
      (void)pp;
      const PointDendrogram d = v == 0 ? ref_point_dendrogram(cat.value(), forest, cloud)
                                       : point_dendrogram(cat.value(), forest, cloud);
      const auto a1 = clk::now();
      const Outcome o = v == 0 ? ref_validate(d) : validate(d);
      const auto a2 = clk::now();
      const Clustering c = v == 0 ? ref_cluster(d, cp) : cluster(d, cp);
#else
      const PointDendrogram d = v == 0 ? ref_point_dendrogram(cat.value(), forest, cloud)
                                       : point_dendrogram(cat.value(), forest, cloud, pp);
      const auto a1 = clk::now();
      const Outcome o = v == 0 ? ref_validate(d) : validate(d, pp);
      const auto a2 = clk::now();
      const Clustering c = v == 0 ? ref_cluster(d, cp) : cluster(d, cp, pp);
#endif
      const auto a3 = clk::now();
      sink += o.ok() + c.selected.size() + c.tree.stability.size();
      t_pd[v].push_back(sec(a0, a1));
      t_va[v].push_back(sec(a1, a2));
      t_cd[v].push_back(sec(a2, a3));
      t_all[v].push_back(sec(a0, a3));
    }
  auto med = [](std::vector<double> w) {
    std::sort(w.begin(), w.end());
    return w[w.size() / 2];
  };
  auto mn = [](const std::vector<double>& w) { return *std::min_element(w.begin(), w.end()); };
  const char* nm[3] = {"reference", "serie", "pool"};
  std::printf("{\"K\":%d,\"entree\":\"%s\",\"P\":%u,\"reps\":%d,\"noeuds\":%zu", K, cover ? "cover" : "core", pool.size(),
              reps, forest.rank.size());
  for (int v = 0; v < 3; ++v)
    std::printf(",\"%s\":{\"pd\":%.2f,\"validate\":%.2f,\"cluster\":%.2f,\"tete\":%.2f,\"tete_min\":%.2f}", nm[v],
                1e3 * med(t_pd[v]), 1e3 * med(t_va[v]), 1e3 * med(t_cd[v]), 1e3 * med(t_all[v]), 1e3 * mn(t_all[v]));
  std::printf(",\"rapports_medianes\":{\"pd_serie\":%.2f,\"cluster_serie\":%.2f,\"tete_serie\":%.2f,\"pd_pool\":%.2f,"
              "\"cluster_pool\":%.2f,\"tete_pool\":%.2f},\"sink\":%.0f}\n",
              med(t_pd[0]) / med(t_pd[1]), med(t_cd[0]) / med(t_cd[1]), med(t_all[0]) / med(t_all[1]),
              med(t_pd[0]) / med(t_pd[2]), med(t_cd[0]) / med(t_cd[2]), med(t_all[0]) / med(t_all[2]), sink);
  return 0;
}
