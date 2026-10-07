// Audit L07 : affiche, pour un petit nuage reel, l'arbre condense de la tete publiee (V) et de la tete a cohortes sur
// dendrogramme normalise (N) : parent, masse, lambda de naissance, stabilite, retenu ; puis les etiquettes.
//   l07_show IN.u32le K core|cover MCS Z SINGLE
#include <cstdio>
#include <string>
#include <vector>

#include "l07_heads.hpp"
#include "tower/tower.hpp"

using namespace mhgp10;

static void show(const char* name, const Clustering& c, const Cloud& cloud, u32 n) {
  std::printf("%s :\n", name);
  std::vector<unsigned char> sel(c.tree.parent.size(), 0);
  for (u32 s : c.selected) sel[s] = 1;
  for (u32 i = 0; i < c.tree.parent.size(); ++i)
    std::printf("  cluster %u parent %lld masse %llu naissance %.17g stabilite %.17g retenu %d\n", i,
                c.tree.parent[i] == kNone ? -1LL : (long long)c.tree.parent[i], (unsigned long long)c.tree.mass[i],
                c.tree.birth[i], c.tree.stability[i], int(sel[i]));
  std::vector<i32> out(n, -1);
  std::vector<double> lam(n, 0);
  for (u32 s = 0; s < cloud.sites(); ++s)
    for (PointId p : cloud.ids.row(s)) {
      out[idx(p)] = c.label[s];
      lam[idx(p)] = c.tree.point_lambda[s];
    }
  std::printf("  etiquettes");
  for (i32 l : out) std::printf(" %d", l);
  std::printf("\n  lambda de sortie");
  for (double l : lam) std::printf(" %.6g", l);
  std::printf("\n");
}

int main(int argc, char** argv) {
  if (argc < 7) return 2;
  std::vector<u32> x, y, z, pid;
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  u32 buf[3];
  while (std::fread(buf, 4, 3, f) == 3) {
    x.push_back(buf[0]);
    y.push_back(buf[1]);
    z.push_back(buf[2]);
    pid.push_back(u32(pid.size()));
  }
  std::fclose(f);
  const int k = std::stoi(argv[2]);
  const std::string entry = argv[3];
  ClusterParams p;
  p.min_cluster_size = std::stoull(argv[4]);
  p.z = std::stod(argv[5]);
  p.allow_single_cluster = std::stoi(argv[6]) != 0;
  auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
  if (!prepared.ok()) return 2;
  const Cloud& cloud = prepared.value();
  sched::Pool pool(1);
  SiteTree tree(cloud);
  CatalogueParams cp;
  cp.kmax = k;
  auto cat = build_catalogue(cloud, cp, pool);
  if (!cat.ok()) return 2;
  TowerParams tp;
  tp.kmax = k;
  tp.only_order = k;
  tp.entry = entry == "core" ? PointEntry::core : PointEntry::cover;
  auto tw = build_tower(cloud, tree, cat.value(), tp, pool);
  if (!tw.ok() || int(tw.value().orders.size()) < k) return 2;
  const PointDendrogram d = point_dendrogram(cat.value(), tw.value().orders[k - 1], cloud);
  if (!validate(d).ok()) return 3;
  const Clustering V = cluster(d, p);
  const PointDendrogram dn = l07::normalize(d);
  const Clustering N = l07::select_x(l07::condense_x(dn, p, true), dn.points(), p);
  show("tete publiee", V, cloud, u32(x.size()));
  show("tete a cohortes", N, cloud, u32(x.size()));
  return 0;
}
