// Audit L07 : la tete a cohortes change-t-elle la STRUCTURE de l'arbre condense, ou seulement des stabilites, et
// lesquelles ? Nuages aleatoires, entree core ; compare la tete publiee (V) et la tete a cohortes (C, meme dendrogramme).
#include <cstdio>
#include <cstring>
#include <random>
#include <set>
#include <vector>

#include "l07_heads.hpp"
#include "tower/tower.hpp"

using namespace mhgp10;

int main() {
  std::mt19937_64 rng(20261002);
  sched::Pool pool(1);
  unsigned long long cases = 0, same_structure = 0, clusters = 0, leaf_changed = 0, internal_changed = 0,
                     lam_later = 0, lam_earlier = 0, points = 0, cluster_of_point_changed = 0;
  for (int t = 0; t < 4000; ++t) {
    const int n = 12 + int(rng() % 40);
    const int grid = (t % 2) ? 1000 : 24;
    std::set<std::array<u32, 3>> pts;
    while (int(pts.size()) < n) pts.insert({u32(rng() % grid), u32(rng() % grid), u32(rng() % grid)});
    std::vector<u32> x, y, z, pid;
    for (const auto& p : pts) {
      x.push_back(p[0]);
      y.push_back(p[1]);
      z.push_back(p[2]);
      pid.push_back(u32(pid.size()));
    }
    auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
    if (!prepared.ok()) continue;
    const Cloud& cloud = prepared.value();
    SiteTree tree(cloud);
    for (int k : {2, 3, 5}) {
      CatalogueParams cp;
      cp.kmax = k;
      auto cat = build_catalogue(cloud, cp, pool);
      if (!cat.ok()) continue;
      TowerParams tp;
      tp.kmax = k;
      tp.only_order = k;
      auto tw = build_tower(cloud, tree, cat.value(), tp, pool);
      if (!tw.ok() || int(tw.value().orders.size()) < k) continue;
      const PointDendrogram d = point_dendrogram(cat.value(), tw.value().orders[k - 1], cloud);
      if (!validate(d).ok()) continue;
      for (u64 mcs : {3ull, 5ull, 8ull}) {
        ClusterParams p;
        p.min_cluster_size = mcs;
        p.z = 1;
        const CondensedTree V = condense(d, p);
        const CondensedTree C = l07::condense_x(d, p, true);
        ++cases;
        const bool same = V.parent == C.parent && V.mass == C.mass &&
                          std::memcmp(V.birth.data(), C.birth.data(), V.birth.size() * sizeof(double)) == 0;
        same_structure += same;
        if (!same) continue;
        std::vector<unsigned char> has_kid(V.parent.size(), 0);
        for (size_t c = 1; c < V.parent.size(); ++c) has_kid[V.parent[c]] = 1;
        for (size_t c = 0; c < V.parent.size(); ++c) {
          ++clusters;
          if (V.stability[c] != C.stability[c]) (has_kid[c] ? internal_changed : leaf_changed)++;
        }
        for (u32 q = 0; q < d.points(); ++q) {
          ++points;
          cluster_of_point_changed += V.point_cluster[q] != C.point_cluster[q];
          lam_later += C.point_lambda[q] > V.point_lambda[q];
          lam_earlier += C.point_lambda[q] < V.point_lambda[q];
        }
      }
    }
  }
  std::printf("cas %llu ; meme structure (parents, masses, naissances bit pour bit) %llu ; clusters %llu ; stabilite "
              "changee : feuilles %llu, internes %llu ; points %llu : cluster de sortie change %llu, lambda de sortie "
              "plus petit sous cohortes %llu, plus grand %llu\n",
              cases, same_structure, clusters, leaf_changed, internal_changed, points, cluster_of_point_changed,
              lam_earlier, lam_later);
  return 0;
}
