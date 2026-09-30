// Sonde courte indépendante : validation série/parallèle du même objet public.
// Lie uniquement la bibliothèque déjà construite ; aucun moteur reconstruit.
#include <cstdio>
#include <string>
#include "points/dendrogram.hpp"
#include "sched/pool.hpp"
using namespace mhgp10;
int main(int argc, char** argv) {
  const u32 n = 65536;
  const bool parallel = argc > 1 && std::string(argv[1]) == "parallel";
  const bool corrupt = argc > 2 && std::string(argv[2]) == "corrupt";
  PointDendrogram d;
  d.level.resize(n); d.node_rank.resize(n); d.parent.resize(n);
  d.child_off.resize(n + 1); d.child_val.resize(n - 1);
  d.point_node.resize(n); d.point_rank.resize(n); d.point_weight.assign(n, 1);
  d.child_off[0] = 0;
  for (u32 v = 0; v < n; ++v) {
    d.level[v] = 1.0 + v; d.node_rank[v] = v;
    d.parent[v] = v + 1 < n ? v + 1 : kNone;
    d.child_off[v + 1] = v;
    d.point_node[v] = v; d.point_rank[v] = v;
    if (v > 0) d.child_val[v - 1] = v - 1;
  }
  if (corrupt) {
    d.child_val[0] = 1;
    const u32 w = n - n / 8;
    d.child_off[w] = 0xFFFFFF00u;
    d.child_off[w + 1] = 0xFFFFFF01u;
  }
  sched::Pool pool(4);
  std::printf("n=%u chunks=16 w=57344 slice14=[57344,61440) mode=%s object=%s\n",
              n, parallel ? "parallel" : "serial", corrupt ? "corrupt" : "valid");
  std::fflush(stdout);
  const Outcome r = validate(d, parallel ? &pool : nullptr);
  std::printf("reason=%s\n", std::string(reason_name(r.reason)).c_str());
  return r.ok() ? 0 : 1;
}
