// Verificateur adverse (hors depot) du levier ordre_tete : tete v2/v4 (point_dendrogram, validate, condense, cluster)
// de la bibliotheque liee contre les copies extraites mecaniquement de a10605a06 (ref_base.inc).
//  - bits de tous les champs, Pool de 1, 2, 3, 5, 8, 13 fils et pool nul ;
//  - grille (mcs, z) adverse, selection eom et leaf, allow_single_cluster ;
//  - validate : dendrogrammes corrompus (mutants), meme raison que la reference serie.
//   verif_tete IN.u32le K cover|core [mcs:z ...]
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "head/head.hpp"
#include "tower/tower.hpp"

#include "ref_base.inc"

using namespace mhgp10;

template <class T>
static bool same_bits(const std::vector<T>& a, const std::vector<T>& b) {
  return a.size() == b.size() && (a.empty() || std::memcmp(a.data(), b.data(), a.size() * sizeof(T)) == 0);
}
static bool same_pd(const PointDendrogram& r, const PointDendrogram& l) {
  return same_bits(r.level, l.level) && same_bits(r.node_rank, l.node_rank) && same_bits(r.child_off, l.child_off) &&
         same_bits(r.child_val, l.child_val) && same_bits(r.parent, l.parent) && same_bits(r.point_node, l.point_node) &&
         same_bits(r.point_rank, l.point_rank) && same_bits(r.point_weight, l.point_weight);
}
static bool same_ct(const CondensedTree& r, const CondensedTree& l) {
  return same_bits(r.parent, l.parent) && same_bits(r.birth, l.birth) && same_bits(r.stability, l.stability) &&
         same_bits(r.mass, l.mass) && same_bits(r.point_cluster, l.point_cluster) &&
         same_bits(r.point_lambda, l.point_lambda) && same_bits(r.node_cluster, l.node_cluster);
}
static bool same_cl(const Clustering& r, const Clustering& l) {
  return same_bits(r.label, l.label) && same_bits(r.selected, l.selected) && same_bits(r.cluster_label, l.cluster_label) &&
         same_ct(r.tree, l.tree);
}

int main(int argc, char** argv) {
  if (argc < 4) return 2;
  std::setvbuf(stdout, nullptr, _IOLBF, 0);
  const int K = std::stoi(argv[2]);
  const bool cover = std::string(argv[3]) == "cover";
  std::vector<std::pair<u64, double>> grid;
  for (int i = 4; i < argc; ++i) {
    const std::string a = argv[i];
    const auto c = a.find(':');
    grid.push_back({std::stoull(a.substr(0, c)), std::stod(a.substr(c + 1))});
  }
  if (grid.empty()) grid = {{1, 3}, {2, 1}, {5, 3}, {89, 3}, {200, 3}, {1000, 6}, {1000000000ull, 3}, {50, 0.5}};
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::vector<u32> raw;
  u32 buf[3];
  while (std::fread(buf, 4, 3, f) == 3) raw.insert(raw.end(), buf, buf + 3);
  std::fclose(f);
  const u32 n = static_cast<u32>(raw.size() / 3);
  std::vector<u32> x(n), y(n), z(n), pid(n);
  for (u32 i = 0; i < n; ++i) x[i] = raw[3 * i], y[i] = raw[3 * i + 1], z[i] = raw[3 * i + 2], pid[i] = i;
  auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
  if (!prepared.ok()) return 2;
  const Cloud& cloud = prepared.value();
  sched::Pool build_pool(4);
  SiteTree tree(cloud);
  CatalogueParams catp;
  catp.kmax = K;
  auto cat = build_catalogue(cloud, catp, build_pool);
  if (!cat.ok()) {
    std::printf("{\"catalogue\":\"refus\"}\n");
    return 2;
  }
  TowerParams tp;
  tp.kmax = K;
  tp.only_order = K;
  tp.entry = cover ? PointEntry::cover : PointEntry::core;
  auto tw = build_tower(cloud, tree, cat.value(), tp, build_pool);
  if (!tw.ok()) {
    std::printf("{\"tour\":\"refus\"}\n");
    return 2;
  }
  const OrderForest& forest = tw.value().orders[K - 1];
  const PointDendrogram dr = ref_point_dendrogram(cat.value(), forest, cloud);
  const Outcome vr = ref_validate(dr);
  int bad = 0;
  const unsigned ths[] = {1, 2, 3, 5, 8, 13};
  std::vector<std::unique_ptr<sched::Pool>> pools;
  for (unsigned t : ths) pools.emplace_back(new sched::Pool(t));
  // point_dendrogram et validate
  {
    const PointDendrogram d0 = point_dendrogram(cat.value(), forest, cloud, nullptr);
    bool ok = same_pd(dr, d0) && validate(d0).reason == vr.reason;
    for (auto& p : pools) {
      const PointDendrogram dl = point_dendrogram(cat.value(), forest, cloud, p.get());
      ok = ok && same_pd(dr, dl) && validate(dl, p.get()).reason == vr.reason;
    }
    std::printf("{\"K\":%d,\"entree\":\"%s\",\"noeuds\":%u,\"points\":%u,\"niveaux\":%zu,\"ref_valide\":%s,"
                "\"pd_identique\":%s}\n",
                K, cover ? "cover" : "core", dr.nodes(), dr.points(), dr.level.size(), vr.ok() ? "true" : "false",
                ok ? "true" : "false");
    bad += !ok;
  }
  // condensation et selection
  for (const auto& [mcs, zz] : grid) {
    for (int sel = 0; sel < 2; ++sel)
      for (int single = 0; single < 2; ++single) {
        ClusterParams cp;
        cp.min_cluster_size = mcs;
        cp.z = zz;
        cp.selection = sel ? Selection::leaf : Selection::eom;
        cp.allow_single_cluster = single != 0;
        const Clustering cr = ref_cluster(dr, cp);
        bool ok = same_cl(cr, cluster(dr, cp, nullptr));
        for (auto& p : pools) ok = ok && same_cl(cr, cluster(dr, cp, p.get()));
        if ((sel == 0 && single == 0) || !ok)
          std::printf("  mcs=%llu z=%g sel=%s single=%d clusters_condenses=%zu retenus=%zu identique=%s\n",
                      (unsigned long long)mcs, zz, sel ? "leaf" : "eom", single, cr.tree.parent.size(), cr.selected.size(),
                      ok ? "true" : "false");
        bad += !ok;
      }
  }
  // validate : mutants (raison de la reference serie contre validate serie et paralleles)
  struct Mut {
    const char* name;
    void (*apply)(PointDendrogram&);
  };
  static const Mut muts[] = {
      {"niveau_egal_milieu", [](PointDendrogram& d) { if (d.level.size() > 2) d.level[d.level.size() / 2] = d.level[d.level.size() / 2 - 1]; }},
      {"niveau_dernier", [](PointDendrogram& d) { if (d.level.size() > 1) d.level.back() = d.level.front(); }},
      {"csr_decroissant", [](PointDendrogram& d) {
         for (u32 v = d.nodes() / 2; v < d.nodes(); ++v)
           if (d.child_off[v + 1] > d.child_off[v] + 1) { d.child_off[v + 1] = d.child_off[v] - 1 + (d.child_off[v] == 0); return; }
       }},
      {"enfant_soi", [](PointDendrogram& d) {
         for (u32 v = d.nodes() / 3; v < d.nodes(); ++v)
           if (d.child_off[v + 1] > d.child_off[v]) { d.child_val[d.child_off[v]] = v; return; }
       }},
      {"parent_faux", [](PointDendrogram& d) {
         for (u32 v = d.nodes() / 3; v < d.nodes(); ++v)
           if (d.child_off[v + 1] > d.child_off[v]) { d.parent[d.child_val[d.child_off[v]]] = v == d.nodes() - 1 ? 0 : v + 1; return; }
       }},
      {"deux_racines", [](PointDendrogram& d) {
         for (u32 v = 2 * d.nodes() / 3; v < d.nodes(); ++v)
           if (d.child_off[v + 1] > d.child_off[v]) { d.parent[d.child_val[d.child_off[v]]] = kNone; return; }
       }},
      {"rang_enfant", [](PointDendrogram& d) {
         for (u32 v = d.nodes() / 2; v < d.nodes(); ++v)
           if (d.child_off[v + 1] > d.child_off[v]) { d.node_rank[d.child_val[d.child_off[v]]] = d.node_rank[v] + 1; return; }
       }},
      {"rang_hors", [](PointDendrogram& d) { d.node_rank[d.nodes() / 2] = u32(d.level.size()); }},
      {"point_hors", [](PointDendrogram& d) { d.point_node[d.points() / 2] = d.nodes(); }},
      {"point_poids0", [](PointDendrogram& d) { d.point_weight[d.points() / 3] = 0; }},
      {"point_tot", [](PointDendrogram& d) {
         for (u32 x = d.points() / 2; x < d.points(); ++x)
           if (d.node_rank[d.point_node[x]] > 0) { d.point_rank[x] = d.node_rank[d.point_node[x]] - 1; return; }
       }},
      {"point_tard", [](PointDendrogram& d) {
         for (u32 x = d.points() / 2; x < d.points(); ++x)
           if (d.parent[d.point_node[x]] != kNone) { d.point_rank[x] = d.node_rank[d.parent[d.point_node[x]]] + 1; return; }
       }},
      {"deux_defauts_noeuds", [](PointDendrogram& d) {  // csr_bounds tardif (rang hors), rank_order precoce (enfant soi)
         d.node_rank[d.nodes() - 2] = u32(d.level.size());
         for (u32 v = 1; v < d.nodes(); ++v)
           if (d.child_off[v + 1] > d.child_off[v]) { d.child_val[d.child_off[v]] = v; return; }
       }},
      {"niveau_et_noeud", [](PointDendrogram& d) {
         d.node_rank[1] = u32(d.level.size());
         if (d.level.size() > 2) d.level[d.level.size() - 1] = d.level[d.level.size() - 2];
       }},
      {"points_deux", [](PointDendrogram& d) {  // poids nul tardif, rang tot precoce
         d.point_weight[d.points() - 1] = 0;
         for (u32 x = 0; x < d.points(); ++x)
           if (d.node_rank[d.point_node[x]] > 0) { d.point_rank[x] = d.node_rank[d.point_node[x]] - 1; return; }
       }},
  };
  for (const Mut& m : muts) {
    if (dr.nodes() < 4 || dr.points() < 4 || dr.level.size() < 4) break;  // mutants sur dendrogrammes non triviaux
    PointDendrogram d = dr;
    m.apply(d);
    const Outcome r = ref_validate(d);
    bool ok = validate(d).reason == r.reason;
    for (auto& p : pools) ok = ok && validate(d, p.get()).reason == r.reason;
    std::printf("  mutant %-20s ref=%-12s identique=%s\n", m.name, std::string(reason_name(r.reason)).c_str(),
                ok ? "true" : "false");
    bad += !ok;
  }
  std::printf("ECARTS %d\n", bad);
  return bad ? 1 : 0;
}
