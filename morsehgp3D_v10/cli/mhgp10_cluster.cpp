// Clustering hierarchique depuis la tour : nuage u32le -> catalogue (K) -> ordre K de la tour -> hierarchie de
// points C n X -> condensation HDBSCAN exacte -> etiquettes (i32 par point d'entree, -1 = bruit).
//
//   mhgp10_cluster IN.u32le OUT.i32le --k=K --mcs=M [--z=Z] [--selection=eom|leaf] [--allow-single]
//                  [--threads=W] [--tree=FILE] [--configs=FILE]
// --configs : une configuration de tete par ligne « mcs z eom|leaf 0|1 » ; la i-eme ecrit OUT.i (i = 0, 1, ...),
// toutes sur la meme construction de la tour.
// --tree : exporte la hierarchie de points (niveaux, parents, attaches) pour les tetes Python de developpement.
// Codes : 0 conforme, 2 refus, 3 invariant viole.
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "head/head.hpp"
#include "tower/tower.hpp"

using namespace mhgp10;

int main(int argc, char** argv) {
  if (argc < 3) return 2;
  int k = 2;
  unsigned threads = 0;
  ClusterParams cp;
  std::string tree_out, configs;
  for (int i = 3; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--k=", 0) == 0) k = std::stoi(a.substr(4));
    else if (a.rfind("--mcs=", 0) == 0) cp.min_cluster_size = std::stoull(a.substr(6));
    else if (a.rfind("--z=", 0) == 0) cp.z = std::stod(a.substr(4));
    else if (a == "--selection=leaf") cp.selection = Selection::leaf;
    else if (a == "--selection=eom") cp.selection = Selection::eom;
    else if (a == "--allow-single") cp.allow_single_cluster = true;
    else if (a.rfind("--threads=", 0) == 0) threads = unsigned(std::stoul(a.substr(10)));
    else if (a.rfind("--tree=", 0) == 0) tree_out = a.substr(7);
    else if (a.rfind("--configs=", 0) == 0) configs = a.substr(10);
    else {
      std::fprintf(stderr, "option inconnue %s\n", a.c_str());
      return 2;
    }
  }
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::vector<u32> raw;
  u32 buf[3];
  while (std::fread(buf, 4, 3, f) == 3) raw.insert(raw.end(), buf, buf + 3);
  std::fclose(f);
  const u32 n = static_cast<u32>(raw.size() / 3);
  std::vector<u32> x(n), y(n), z(n), pid(n);
  for (u32 i = 0; i < n; ++i) {
    x[i] = raw[3 * i];
    y[i] = raw[3 * i + 1];
    z[i] = raw[3 * i + 2];
    pid[i] = i;
  }
  using clk = std::chrono::steady_clock;
  const auto t0 = clk::now();
  auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
  if (!prepared.ok()) return 2;
  const Cloud& cloud = prepared.value();
  sched::Pool pool(threads);
  SiteTree tree(cloud);
  CatalogueParams catp;
  catp.kmax = k;
  auto cat = build_catalogue(cloud, catp, pool);
  if (!cat.ok()) {
    std::printf("{\"status\":\"%s\",\"reason\":\"%s\"}\n", std::string(status_name(cat.outcome().status())).c_str(),
                std::string(reason_name(cat.outcome().reason)).c_str());
    return 2;
  }
  const auto t1 = clk::now();
  TowerParams tp;
  tp.kmax = k;
  tp.only_order = k;
  auto tw = build_tower(cloud, tree, cat.value(), tp, pool);
  if (!tw.ok()) {
    std::printf("{\"status\":\"%s\",\"reason\":\"%s\"}\n", std::string(status_name(tw.outcome().status())).c_str(),
                std::string(reason_name(tw.outcome().reason)).c_str());
    return tw.outcome().status() == Status::invariant_violated ? 3 : 2;
  }
  const auto t2 = clk::now();
  const OrderForest& forest = tw.value().orders[k - 1];
  const PointDendrogram d = point_dendrogram(cat.value(), forest, cloud);
  const Outcome v = validate(d);
  if (!v.ok()) {
    std::printf("{\"status\":\"invariant_violated\",\"reason\":\"%s\"}\n", std::string(reason_name(v.reason)).c_str());
    return 3;
  }
  std::vector<ClusterParams> list;
  std::vector<std::string> paths;
  if (configs.empty()) {
    list.push_back(cp);
    paths.push_back(argv[2]);
  } else {
    FILE* c = std::fopen(configs.c_str(), "r");
    if (!c) return 2;
    unsigned long long m;
    double zz;
    char sel[16];
    int single;
    while (std::fscanf(c, "%llu %lf %15s %d", &m, &zz, sel, &single) == 4) {
      ClusterParams q;
      q.min_cluster_size = m;
      q.z = zz;
      q.selection = std::string(sel) == "leaf" ? Selection::leaf : Selection::eom;
      q.allow_single_cluster = single != 0;
      list.push_back(q);
      paths.push_back(std::string(argv[2]) + "." + std::to_string(list.size() - 1));
    }
    std::fclose(c);
  }
  size_t clusters = 0;
  for (size_t i = 0; i < list.size(); ++i) {
    const Clustering cl = cluster(d, list[i]);
    clusters = cl.selected.size();
    std::vector<i32> out(n, -1);
    for (u32 s = 0; s < cloud.sites(); ++s)
      for (PointId p : cloud.ids.row(s)) out[idx(p)] = cl.label[s];
    FILE* o = std::fopen(paths[i].c_str(), "wb");
    if (!o) return 2;
    std::fwrite(out.data(), 4, n, o);
    std::fclose(o);
  }
  const auto t3 = clk::now();
  if (!tree_out.empty()) {
    // format texte : levels L / l <valeur> ; nodes N / v <rang> <parent> ; points P / p <site> <noeud> <rang> <poids>
    FILE* t = std::fopen(tree_out.c_str(), "w");
    if (!t) return 2;
    std::fprintf(t, "levels %zu\n", d.level.size());
    for (double lv : d.level) std::fprintf(t, "%.17g\n", lv);
    std::fprintf(t, "nodes %u\n", d.nodes());
    for (u32 v2 = 0; v2 < d.nodes(); ++v2)
      std::fprintf(t, "%u %lld\n", d.node_rank[v2], d.parent[v2] == kNone ? -1LL : (long long)d.parent[v2]);
    std::fprintf(t, "points %u\n", n);
    for (u32 s = 0; s < cloud.sites(); ++s)
      for (PointId p : cloud.ids.row(s)) std::fprintf(t, "%u %u %u %u\n", idx(p), d.point_node[s], d.point_rank[s], 1u);
    std::fclose(t);
  }
  auto sec = [](auto a, auto b) { return std::chrono::duration<double>(b - a).count(); };
  std::printf("{\"status\":\"ok\",\"n\":%u,\"k\":%d,\"balls\":%u,\"clusters\":%zu,\"catalogue_s\":%.3f,\"tower_s\":%.3f,"
              "\"head_s\":%.3f}\n",
              n, k, cat.value().balls(), clusters, sec(t0, t1), sec(t1, t2), sec(t2, t3));
  return 0;
}
