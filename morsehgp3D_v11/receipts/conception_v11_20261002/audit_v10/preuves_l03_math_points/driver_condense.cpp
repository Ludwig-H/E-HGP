// Audit L03 : rejeu du defaut de condensation des departs de points sur la tete publiee (head.cpp, afb081774).
// Dendrogramme abstrait valide (validate() le confirme) : A porte 3 points admis aux rayons carres 1, 4, 9 ;
// B porte 2 points admis a 16 ; A et B fusionnent en R a 25. mcs = 2, z = 1, racine selectionnable.
#include <cstdio>
#include "head/head.hpp"
using namespace mhgp10;
int main() {
  PointDendrogram d;
  d.level = {1, 4, 9, 16, 25};
  d.node_rank = {0, 3, 4};            // A, B, R
  d.child_off = {0, 0, 0, 2};
  d.child_val = {0, 1};
  d.parent = {2, 2, kNone};
  d.point_node = {0, 0, 0, 1, 1};
  d.point_rank = {0, 1, 2, 3, 3};
  d.point_weight = {1, 1, 1, 1, 1};
  const Outcome v = validate(d);
  std::printf("validate ok=%d\n", int(v.ok()));
  for (int single = 0; single < 2; ++single) {
    ClusterParams p;
    p.min_cluster_size = 2;
    p.z = 1.0;
    p.allow_single_cluster = single != 0;
    const Clustering c = cluster(d, p);
    std::printf("allow_single=%d clusters_condenses=%zu\n", single, c.tree.parent.size());
    for (size_t i = 0; i < c.tree.parent.size(); ++i)
      std::printf("  cluster %zu parent=%d naissance_lambda=%.6f stabilite=%.6f masse=%llu\n", i,
                  c.tree.parent[i] == kNone ? -1 : int(c.tree.parent[i]), c.tree.birth[i], c.tree.stability[i],
                  (unsigned long long)c.tree.mass[i]);
    std::printf("  lambda de sortie des points :");
    for (double l : c.tree.point_lambda) std::printf(" %.6f", l);
    std::printf("\n  etiquettes :");
    for (int l : c.label) std::printf(" %d", l);
    std::printf("\n");
  }
  return 0;
}
