// Clustering hierarchique depuis la tour : nuage u32le -> catalogue (K) -> ordre K de la tour -> hierarchie de
// points C n X -> condensation HDBSCAN exacte -> etiquettes (i32 par point d'entree, -1 = bruit).
//
//   mhgp10_cluster IN.u32le OUT.i32le --k=K --mcs=M [--z=Z] [--selection=eom|leaf] [--allow-single]
//                  [--threads=W] [--tree=FILE] [--configs=FILE]
// --configs : une configuration de tete par ligne « mcs z eom|leaf 0|1 » ; la i-eme ecrit OUT.i (i = 0, 1, ...),
// toutes sur la meme construction de la tour.
// --k-list=1,2,5 : un seul catalogue (a l'ordre maximal), puis chaque ordre de la liste ; les sorties deviennent
// OUT.k<K>.<i> (les ordres sont independants a catalogue donne).
// --tree : exporte la hierarchie de points (niveaux, parents, attaches) pour les tetes Python de developpement.
// --entry=core|cover : entree des points par leur propre rayon K-NN (coeurs, C n X, defaut) ou par premiere
// couverture (amas discrets), voir TowerParams ; coverE (E = 1..9) : boule couvrante de poids >= K + E (cover1 :
// entree a alpha_{K+1}, comme HGP-old). --entry=core,cover,cover1 : plusieurs entrees sur le meme catalogue (ordre
// max(K) + max(E)) ; les sorties deviennent OUT.<entree>.k<K>.<i> (et l'arbre TREE.<entree>.k<K>). --label=vote (entree cover) : ecrit en
// plus OUT[...].vote, ou chaque point recoit l'amas retenu qui le couvre par sa boule de plus bas niveau (-1 si
// aucun). --cover-extra=E (entree cover) : boule couvrante de poids >= K + E (E = 1 : entree a alpha_{K+1}, comme
// HGP-old) ; le catalogue est construit a l'ordre max(K) + E.
// Codes : 0 conforme, 2 refus, 3 invariant viole.
#include <cctype>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "head/head.hpp"
#include "tower/tower.hpp"

using namespace mhgp10;

int main(int argc, char** argv) {
  if (argc < 3) return 2;
  int k = 2;
  unsigned threads = 0;
  ClusterParams cp;
  std::string tree_out, configs;
  struct EntrySpec {
    PointEntry entry;
    int extra;        // entree cover : boule couvrante de poids >= K + extra
    std::string tag;  // suffixe des sorties quand plusieurs entrees sont demandees
  };
  std::vector<EntrySpec> entries;
  bool vote = false;
  int cover_extra = -1;  // --cover-extra : applique aux jetons « cover » sans suffixe
  std::vector<int> klist;
  for (int i = 3; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--k=", 0) == 0) k = std::stoi(a.substr(4));
    else if (a.rfind("--mcs=", 0) == 0) cp.min_cluster_size = std::stoull(a.substr(6));
    else if (a.rfind("--z=", 0) == 0) cp.z = std::stod(a.substr(4));
    else if (a == "--selection=leaf") cp.selection = Selection::leaf;
    else if (a == "--selection=eom") cp.selection = Selection::eom;
    else if (a == "--allow-single") cp.allow_single_cluster = true;
    else if (a.rfind("--threads=", 0) == 0) threads = unsigned(std::stoul(a.substr(10)));
    else if (a.rfind("--tree=", 0) == 0) tree_out = a.substr(7);
    else if (a.rfind("--configs=", 0) == 0) configs = a.substr(10);
    else if (a.rfind("--entry=", 0) == 0) {
      entries.clear();
      std::string v = a.substr(8);
      size_t pos = 0;
      while (pos <= v.size()) {
        const size_t e = v.find(',', pos);
        const std::string tok = v.substr(pos, e == std::string::npos ? std::string::npos : e - pos);
        if (tok == "core") entries.push_back({PointEntry::core, 0, tok});
        else if (tok.rfind("cover", 0) == 0 && tok.size() <= 6 && (tok.size() == 5 || std::isdigit(tok[5])))
          entries.push_back({PointEntry::cover, tok.size() == 5 ? 0 : tok[5] - '0', tok});
        else {
          std::fprintf(stderr, "entree inconnue %s\n", tok.c_str());
          return 2;
        }
        if (e == std::string::npos) break;
        pos = e + 1;
      }
    }
    else if (a == "--label=vote") vote = true;
    else if (a.rfind("--cover-extra=", 0) == 0) cover_extra = std::stoi(a.substr(14));
    else if (a == "--label=tree") vote = false;
    else if (a.rfind("--k-list=", 0) == 0) {
      std::string v = a.substr(9);
      size_t pos = 0;
      while (pos < v.size()) {
        const size_t e = v.find(',', pos);
        klist.push_back(std::stoi(v.substr(pos, e == std::string::npos ? std::string::npos : e - pos)));
        if (e == std::string::npos) break;
        pos = e + 1;
      }
    }
    else {
      std::fprintf(stderr, "option inconnue %s\n", a.c_str());
      return 2;
    }
  }
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::vector<u32> raw;
  u32 buf[3];
  while (std::fread(buf, 4, 3, f) == 3) raw.insert(raw.end(), buf, buf + 3);
  std::fclose(f);
  const u32 n = static_cast<u32>(raw.size() / 3);
  std::vector<u32> x(n), y(n), z(n), pid(n);
  for (u32 i = 0; i < n; ++i) {
    x[i] = raw[3 * i];
    y[i] = raw[3 * i + 1];
    z[i] = raw[3 * i + 2];
    pid[i] = i;
  }
  using clk = std::chrono::steady_clock;
  const auto t0 = clk::now();
  auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
  if (!prepared.ok()) return 2;
  const Cloud& cloud = prepared.value();
  sched::Pool pool(threads);
  SiteTree tree(cloud);
  if (klist.empty()) klist.push_back(k);
  if (entries.empty()) entries.push_back({PointEntry::core, 0, "core"});
  if (cover_extra >= 0)
    for (EntrySpec& es : entries)
      if (es.entry == PointEntry::cover && es.tag == "cover") es.extra = cover_extra;
  int max_extra = 0;
  for (const EntrySpec& es : entries) max_extra = std::max(max_extra, es.extra);
  int kmax = 0;
  for (int kk : klist) kmax = std::max(kmax, kk);
  CatalogueParams catp;
  catp.kmax = kmax + max_extra;  // l'entree cover a K + extra lit des boules de poids K + extra
  auto cat = build_catalogue(cloud, catp, pool);
  if (!cat.ok()) {
    std::printf("{\"status\":\"%s\",\"reason\":\"%s\"}\n", std::string(status_name(cat.outcome().status())).c_str(),
                std::string(reason_name(cat.outcome().reason)).c_str());
    return 2;
  }
  const auto t1 = clk::now();
  // configurations de tete
  std::vector<ClusterParams> list;
  if (configs.empty()) {
    list.push_back(cp);
  } else {
    FILE* c = std::fopen(configs.c_str(), "r");
    if (!c) return 2;
    unsigned long long m;
    double zz;
    char sel[16];
    int single;
    while (std::fscanf(c, "%llu %lf %15s %d", &m, &zz, sel, &single) == 4) {
      ClusterParams q;
      q.min_cluster_size = m;
      q.z = zz;
      q.selection = std::string(sel) == "leaf" ? Selection::leaf : Selection::eom;
      q.allow_single_cluster = single != 0;
      list.push_back(q);
    }
    std::fclose(c);
  }
  const bool multi = klist.size() > 1 || !configs.empty() || entries.size() > 1;
  double tower_s = 0, head_s = 0;
  size_t clusters = 0;
  for (int kk : klist)
  for (const EntrySpec& es : entries) {
    const PointEntry entry = es.entry;
    const std::string tag = entries.size() > 1 ? "." + es.tag : "";
    const auto a0 = clk::now();
    TowerParams tp;
    tp.kmax = kmax;
    tp.only_order = kk;
    tp.entry = entry;
    tp.cover_extra = es.extra;
    tp.ball_nodes = vote && entry == PointEntry::cover;  // la relation de couverture complete ne sert qu'au vote
    auto tw = build_tower(cloud, tree, cat.value(), tp, pool);
    if (!tw.ok()) {
      std::printf("{\"status\":\"%s\",\"reason\":\"%s\",\"k\":%d}\n", std::string(status_name(tw.outcome().status())).c_str(),
                  std::string(reason_name(tw.outcome().reason)).c_str(), kk);
      return tw.outcome().status() == Status::invariant_violated ? 3 : 2;
    }
    const auto a1 = clk::now();
    const OrderForest& forest = tw.value().orders[kk - 1];
    const PointDendrogram d = point_dendrogram(cat.value(), forest, cloud, &pool);
    const Outcome v = validate(d, &pool);
    if (!v.ok()) {
      std::printf("{\"status\":\"invariant_violated\",\"reason\":\"%s\"}\n", std::string(reason_name(v.reason)).c_str());
      return 3;
    }
    // vote de couverture : boules couvrantes de chaque site, par niveau croissant (ordre canonique du catalogue)
    std::vector<u64> cov_off;
    std::vector<u32> cov_ball;
    if (vote && !forest.ball_node.empty()) {
      const Catalogue& C = cat.value();
      cov_off.assign(u64(cloud.sites()) + 1, 0);
      for (u32 b = 0; b < C.balls(); ++b)
        if (forest.ball_node[b] != kNone)
          for (u64 q = C.pop_off[b]; q < C.pop_off[b + 1]; ++q) ++cov_off[C.pop[q] + 1];
      for (u32 s = 0; s < cloud.sites(); ++s) cov_off[s + 1] += cov_off[s];
      cov_ball.resize(cov_off[cloud.sites()]);
      std::vector<u64> fillc(cov_off.begin(), cov_off.end() - 1);
      for (u32 b = 0; b < C.balls(); ++b)
        if (forest.ball_node[b] != kNone)
          for (u64 q = C.pop_off[b]; q < C.pop_off[b + 1]; ++q) cov_ball[fillc[C.pop[q]]++] = b;
    }
    for (size_t i = 0; i < list.size(); ++i) {
      const Clustering cl = cluster(d, list[i], &pool);
      clusters = cl.selected.size();
      std::vector<i32> lab_vote;
      if (!cov_off.empty()) {
        lab_vote.assign(cloud.sites(), -1);
        for (u32 s = 0; s < cloud.sites(); ++s)
          for (u64 q = cov_off[s]; q < cov_off[s + 1] && lab_vote[s] < 0; ++q) {
            const u32 c = cl.tree.node_cluster[forest.ball_node[cov_ball[q]]];
            if (c != kNone) lab_vote[s] = cl.cluster_label[c];
          }
      }
      const std::string path = !multi ? std::string(argv[2])
                                      : std::string(argv[2]) + tag + ".k" + std::to_string(kk) + "." + std::to_string(i);
      for (int variant = 0; variant < (lab_vote.empty() ? 1 : 2); ++variant) {
        const std::vector<i32>& lab = variant == 0 ? cl.label : lab_vote;
        std::vector<i32> out(n, -1);
        for (u32 s = 0; s < cloud.sites(); ++s)
          for (PointId p : cloud.ids.row(s)) out[idx(p)] = lab[s];
        FILE* o = std::fopen((path + (variant == 0 ? "" : ".vote")).c_str(), "wb");
        if (!o) return 2;
        std::fwrite(out.data(), 4, n, o);
        std::fclose(o);
      }
    }
    if (!tree_out.empty()) {
      FILE* t = std::fopen((tree_out + (multi ? tag + ".k" + std::to_string(kk) : std::string())).c_str(), "w");
      if (!t) return 2;
      std::fprintf(t, "levels %zu\n", d.level.size());
      for (double lv : d.level) std::fprintf(t, "%.17g\n", lv);
      std::fprintf(t, "nodes %u\n", d.nodes());
      for (u32 v2 = 0; v2 < d.nodes(); ++v2)
        std::fprintf(t, "%u %lld\n", d.node_rank[v2], d.parent[v2] == kNone ? -1LL : (long long)d.parent[v2]);
      std::fprintf(t, "points %u\n", n);
      for (u32 s = 0; s < cloud.sites(); ++s)
        for (PointId p : cloud.ids.row(s)) std::fprintf(t, "%u %u %u %u\n", idx(p), d.point_node[s], d.point_rank[s], 1u);
      std::fclose(t);
    }
    const auto a2 = clk::now();
    tower_s += std::chrono::duration<double>(a1 - a0).count();
    head_s += std::chrono::duration<double>(a2 - a1).count();
  }
  auto sec = [](auto a, auto b) { return std::chrono::duration<double>(b - a).count(); };
  std::printf("{\"status\":\"ok\",\"n\":%u,\"kmax\":%d,\"balls\":%u,\"clusters\":%zu,\"catalogue_s\":%.3f,\"tower_s\":%.3f,"
              "\"head_s\":%.3f}\n",
              n, kmax, cat.value().balls(), clusters, sec(t0, t1), tower_s, head_s);
  return 0;
}
