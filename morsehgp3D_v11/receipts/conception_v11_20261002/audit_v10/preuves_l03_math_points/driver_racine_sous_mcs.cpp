// Audit L03 : rejeu du controle de l'auditeur independant (2 octobre) sur la tete publiee (afb081774).
// Trois points dans un seul noeud, mcs = 5, allow_single_cluster = true : un cluster de trois points est-il rendu ?
#include <cstdio>
#include "head/head.hpp"
using namespace mhgp10;
int main() {
  PointDendrogram d;
  d.level = {1};
  d.node_rank = {0};
  d.child_off = {0, 0};
  d.child_val = {};
  d.parent = {kNone};
  d.point_node = {0, 0, 0};
  d.point_rank = {0, 0, 0};
  d.point_weight = {1, 1, 1};
  const Outcome v = validate(d);
  std::printf("validate ok=%d\n", int(v.ok()));
  for (int single = 0; single < 2; ++single) {
    ClusterParams p;
    p.min_cluster_size = 5;
    p.z = 1.0;
    p.allow_single_cluster = single != 0;
    const Clustering c = cluster(d, p);
    std::printf("mcs=5 allow_single=%d etiquettes :", single);
    for (int l : c.label) std::printf(" %d", l);
    std::printf("\n");
  }
  return 0;
}
