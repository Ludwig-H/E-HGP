// Banc hors depot : temps des etapes de la tete (dendrogramme de points, validation, condensation + EOM) sur une trame.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "head/head.hpp"
#include "tower/tower.hpp"

using namespace mhgp10;
using clk = std::chrono::steady_clock;

int main(int argc, char** argv) {
  const int K = argc > 2 ? std::stoi(argv[2]) : 5;
  const unsigned threads = argc > 3 ? unsigned(std::stoul(argv[3])) : 8;
  FILE* f = std::fopen(argv[1], "rb");
  std::vector<u32> raw;
  u32 buf[3];
  while (std::fread(buf, 4, 3, f) == 3) raw.insert(raw.end(), buf, buf + 3);
  std::fclose(f);
  const u32 n = static_cast<u32>(raw.size() / 3);
  std::vector<u32> x(n), y(n), z(n), pid(n);
  for (u32 i = 0; i < n; ++i) x[i] = raw[3 * i], y[i] = raw[3 * i + 1], z[i] = raw[3 * i + 2], pid[i] = i;
  auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
  const Cloud& cloud = prepared.value();
  sched::Pool pool(threads);
  SiteTree tree(cloud);
  CatalogueParams catp;
  catp.kmax = K;
  auto cat = build_catalogue(cloud, catp, pool);
  TowerParams tp;
  tp.kmax = K;
  tp.only_order = K;
  tp.entry = PointEntry::cover;
  auto tw = build_tower(cloud, tree, cat.value(), tp, pool);
  const OrderForest& forest = tw.value().orders[K - 1];
  ClusterParams cp;
  cp.min_cluster_size = 200;
  cp.z = 3;
  double best[3] = {1e9, 1e9, 1e9};
  size_t clusters = 0, nodes = 0;
  for (int rep = 0; rep < 5; ++rep) {
    const auto a0 = clk::now();
    const PointDendrogram d = point_dendrogram(cat.value(), forest, cloud);
    const auto a1 = clk::now();
    const Outcome v = validate(d);
    const auto a2 = clk::now();
    const Clustering cl = cluster(d, cp);
    const auto a3 = clk::now();
    if (!v.ok()) return 3;
    clusters = cl.selected.size();
    nodes = d.nodes();
    best[0] = std::min(best[0], std::chrono::duration<double>(a1 - a0).count());
    best[1] = std::min(best[1], std::chrono::duration<double>(a2 - a1).count());
    best[2] = std::min(best[2], std::chrono::duration<double>(a3 - a2).count());
  }
  std::printf("{\"n\":%u,\"K\":%d,\"nodes\":%zu,\"clusters\":%zu,\"point_dendrogram_s\":%.4f,\"validate_s\":%.4f,"
              "\"cluster_s\":%.4f}\n", n, K, nodes, clusters, best[0], best[1], best[2]);
  return 0;
}
