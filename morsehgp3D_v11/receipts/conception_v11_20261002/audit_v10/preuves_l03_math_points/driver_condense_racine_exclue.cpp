// Audit L03 : rejeu, sur la tete publiee (head.cpp, afb081774), du contre-exemple de l'auditeur continu avec racine
// globale EXCLUE. A : 3 points admis aux rayons carres 1, 4, 9 ; B : 2 points a 16 ; A et B fusionnent en R a 25 ;
// C : 2 points a 100 ; R et C fusionnent en G a 1600. mcs = 2, z = 1 et 2, allow_single_cluster = false.
// Condensation de HDBSCAN (cohortes) : stabilite(A) = 11/15, A + B = 5/6 < stabilite(R) = 7/8 : selection R, C.
#include <cstdio>
#include "head/head.hpp"
using namespace mhgp10;
int main() {
  PointDendrogram d;
  d.level = {1, 4, 9, 16, 25, 100, 1600};
  d.node_rank = {0, 3, 4, 5, 6};          // A, B, R, C, G
  d.child_off = {0, 0, 0, 2, 2, 4};
  d.child_val = {0, 1, 2, 3};
  d.parent = {2, 2, 4, 4, kNone};
  d.point_node = {0, 0, 0, 1, 1, 3, 3};
  d.point_rank = {0, 1, 2, 3, 3, 5, 5};
  d.point_weight = {1, 1, 1, 1, 1, 1, 1};
  const Outcome v = validate(d);
  std::printf("validate ok=%d\n", int(v.ok()));
  for (int zi = 1; zi <= 2; ++zi) {
    ClusterParams p;
    p.min_cluster_size = 2;
    p.z = double(zi);
    p.allow_single_cluster = false;
    const Clustering c = cluster(d, p);
    std::printf("z=%d allow_single=0 clusters_condenses=%zu\n", zi, c.tree.parent.size());
    for (size_t i = 0; i < c.tree.parent.size(); ++i)
      std::printf("  cluster %zu parent=%d naissance_lambda=%.6f stabilite=%.6f masse=%llu\n", i,
                  c.tree.parent[i] == kNone ? -1 : int(c.tree.parent[i]), c.tree.birth[i], c.tree.stability[i],
                  (unsigned long long)c.tree.mass[i]);
    std::printf("  etiquettes :");
    for (int l : c.label) std::printf(" %d", l);
    std::printf("\n");
  }
  return 0;
}
