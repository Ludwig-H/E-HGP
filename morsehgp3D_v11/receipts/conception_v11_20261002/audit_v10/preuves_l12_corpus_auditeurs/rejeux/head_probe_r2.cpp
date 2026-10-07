// Sonde L12 : la tete du raccord R2 prive (head.cpp a513aebd) conserve-t-elle le defaut des departs de points ?
#include <cstdio>
#include "head/head.hpp"
using namespace mhgp10;
int main() {
  PointDendrogram d;
  d.level = {1, 4, 9, 16, 25};
  d.node_rank = {0, 3, 4};
  d.child_off = {0, 0, 0, 2};
  d.child_val = {0, 1};
  d.parent = {2, 2, kNone};
  d.point_node = {0, 0, 0, 1, 1};
  d.point_rank = {0, 1, 2, 3, 3};
  d.point_weight = {1, 1, 1, 1, 1};
  ClusterParams p;
  p.min_cluster_size = 2;
  p.z = 1.0;
  p.allow_single_cluster = true;
  auto t = condense(d, p);
  for (size_t i = 0; i < t.stability.size(); ++i) std::printf("cluster %zu stability=%.17g\n", i, t.stability[i]);
  std::printf("attendu correct stab(A)=%.17g ; defaut=%.17g\n", 11.0 / 15.0, 37.0 / 30.0);
}
