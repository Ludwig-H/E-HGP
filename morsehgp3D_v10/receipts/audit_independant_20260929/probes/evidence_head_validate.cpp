#include "head/head.hpp"

#include <cmath>
#include <cstdio>
#include <limits>
#include <string>

using namespace mhgp10;

PointDendrogram single() {
  PointDendrogram d;
  d.level = {1.0};
  d.node_rank = {0};
  d.child_off = {0, 0};
  d.parent = {kNone};
  d.point_node = {0};
  d.point_rank = {0};
  d.point_weight = {1};
  return d;
}

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  const std::string mode = argv[1];
  auto d = single();
  ClusterParams p;
  p.min_cluster_size = 1;
  p.allow_single_cluster = true;
  if (mode == "rank") {
    d.point_rank[0] = 1;  // hors de level ; la racine n'a pas de borne parentale
  } else if (mode == "missing-edge") {
    d.level = {1.0, 4.0};
    d.node_rank = {0, 1};
    d.parent = {1, kNone};
    d.child_off = {0, 0, 0};  // l'enfant 0 est absent du CSR de son parent 1
  } else if (mode == "nan-level") {
    d.level[0] = std::numeric_limits<double>::quiet_NaN();
  } else if (mode == "nan-z") {
    d.level[0] = 4.0;
    p.z = std::numeric_limits<double>::quiet_NaN();
  } else if (mode == "negative-z") {
    d.level[0] = 4.0;
    p.z = -1.0;
  } else {
    return 2;
  }
  const auto v = validate(d);
  std::printf("mode=%s validate=%s\n", mode.c_str(), std::string(status_name(v.status())).c_str());
  std::fflush(stdout);
  if (!v.ok()) return 3;
  const auto cl = cluster(d, p);
  std::printf("label=%d stability=%g finite=%d\n", cl.label[0], cl.tree.stability[0],
              int(std::isfinite(cl.tree.stability[0])));
}
