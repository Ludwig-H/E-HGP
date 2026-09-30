// Peigne valide : chaque feuille porte deux points ; chaque scission a deux
// enfants lourds. Compte exactement les remontées de cluster_label (head.cpp).
#include "head/head.hpp"

#include <cstdio>

using namespace mhgp10;

PointDendrogram comb(u32 q) {
  PointDendrogram d;
  const u32 n = 2 * q + 1;
  d.node_rank.resize(n);
  d.parent.assign(n, kNone);
  d.child_off.assign(n + 1, 0);
  d.level.push_back(1.0);
  for (u32 i = 1; i <= q; ++i) d.level.push_back(double(i + 1));
  for (u32 leaf = 0; leaf <= q; ++leaf)
    for (u32 j = 0; j < 2; ++j) {
      d.point_node.push_back(leaf);
      d.point_rank.push_back(0);
      d.point_weight.push_back(1);
    }
  for (u32 v = 0; v < n; ++v) {
    if (v > q) {
      const u32 i = v - q - 1;
      const u32 left = i == 0 ? 0 : v - 1, right = i + 1;
      d.node_rank[v] = i + 1;
      d.child_val.push_back(left);
      d.child_val.push_back(right);
      d.parent[left] = d.parent[right] = v;
    }
    d.child_off[v + 1] = static_cast<u32>(d.child_val.size());
  }
  return d;
}

int main() {
  for (u32 q : {64u, 128u, 256u}) {
    auto d = comb(q);
    if (!validate(d).ok()) return 2;
    ClusterParams p;
    p.min_cluster_size = 2;
    p.selection = Selection::leaf;
    const auto c = cluster(d, p);
    u64 climbs = 0;
    for (u32 start = 0; start < c.tree.parent.size(); ++start)
      for (u32 a = start; a != kNone; a = c.tree.parent[a]) {
        ++climbs;
        // Avec sélection en feuilles, les clusters sélectionnés sont ceux qui
        // étiquettent deux points ; tous les clusters internes remontent à EOF.
        bool selected = false;
        for (u32 s : c.selected) selected |= s == a;
        if (selected) break;
      }
    std::printf("q=%u points=%u clusters=%zu selected=%zu cluster_label_ancestor_visits=%llu\n", q,
                d.points(), c.tree.parent.size(), c.selected.size(), (unsigned long long)climbs);
  }
}
