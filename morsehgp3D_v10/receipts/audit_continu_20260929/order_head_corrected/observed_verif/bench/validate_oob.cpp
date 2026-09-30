// Mutant hors depot : dendrogramme dont un defaut precoce (enfant = soi, rank_order) precede un CSR tardif hors bornes
// (child_off[w + 1] enorme). La version serie s'arrete au premier defaut ; la version parallele (v4) lit-elle hors
// bornes dans une tranche ulterieure ?   validate_oob NOEUDS FILS [serie|parallele]
#include <cstdio>
#include <string>
#include <vector>

#include "points/dendrogram.hpp"
#include "sched/pool.hpp"

using namespace mhgp10;

int main(int argc, char** argv) {
  const u32 n = argc > 1 ? u32(std::stoul(argv[1])) : 200000;
  const unsigned threads = argc > 2 ? unsigned(std::stoul(argv[2])) : 4;
  const bool par = argc > 3 && std::string(argv[3]) == "parallele";
  // peigne : noeud v (v >= 1) a pour enfant v - 1 ; racine n - 1 ; un point par noeud
  PointDendrogram d;
  d.level.resize(n);
  for (u32 i = 0; i < n; ++i) d.level[i] = 1.0 + i;
  d.node_rank.resize(n);
  d.parent.resize(n);
  d.child_off.assign(n + 1, 0);
  for (u32 v = 0; v < n; ++v) {
    d.node_rank[v] = v;
    d.parent[v] = v + 1 < n ? v + 1 : kNone;
    d.child_off[v + 1] = v;  // enfants de v : [v - 1, v) pour v >= 1
  }
  d.child_val.resize(n - 1);
  for (u32 v = 1; v < n; ++v) d.child_val[v - 1] = v - 1;
  d.point_node.resize(n);
  d.point_rank.resize(n);
  d.point_weight.assign(n, 1);
  for (u32 x = 0; x < n; ++x) d.point_node[x] = x, d.point_rank[x] = x;
  const Outcome ok0 = validate(d);
  std::printf("intact : %s\n", std::string(reason_name(ok0.reason)).c_str());
  d.child_val[0] = 1;                       // noeud 1 : enfant = soi -> rank_order (defaut precoce)
  const u32 w = n - n / 8;                  // noeud tardif, autre tranche
  d.child_off[w] = 0xFFFFFF00u;             // plage de w : [enorme, enorme + 1) : lecture hors bornes si w est visite
  d.child_off[w + 1] = 0xFFFFFF01u;
  sched::Pool pool(threads);
  std::fflush(stdout);
  const Outcome r = par ? validate(d, &pool) : validate(d);
  std::printf("%s : %s\n", par ? "parallele" : "serie", std::string(reason_name(r.reason)).c_str());
  return 0;
}
