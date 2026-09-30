// Exporteur de l'experience frontiere (Fondations A, hors produit) : nuage u32le + ordre K -> catalogue critique
// (servant K, ou --kmax-catalogue >= K) puis tour FULL construite deux fois, entree core puis entree cover, avec
// TowerParams::ball_nodes = true. Ecrit un JSON deterministe (identique octet pour octet quel que soit le nombre de
// fils) ; aucune source du moteur n'est modifiee, seuls ses en-tetes publics et libmhgp10_core.a sont utilises.
//
//   export_frontier IN.u32le --k=K --out=FICHIER [--threads=W] [--kmax-catalogue=KC] [--all-orders]
//
//   --threads=W          fils du Pool (defaut 1 : machine partagee) ; la sortie n'en depend pas ;
//   --kmax-catalogue=KC  ordre servi par le catalogue (defaut K ; K <= KC <= 12) ; sert aux controles
//                        d'independance vis-a-vis de Kmax (les rangs changent, les niveaux exacts non) ;
//   --all-orders         construit et exporte les ordres 1..K (avec les images verticales `lower`) ; sinon
//                        TowerParams::only_order = K et seul l'ordre K est exporte.
//
// Sortie standard : une ligne JSON (statut, comptes, temps muraux). Le fichier --out suit le schema
// mhgp10_frontier_export_v1, decrit dans README_FONDATIONS_A.md :
//   levels : table des niveaux exacts (rayons CARRES) num/den ; lv 0 = niveau nul, lv r >= 1 = rang r - 1 du
//            catalogue (lv d'un noeud = OrderForest::rank, lv d'une boule = Catalogue::rank + 1) ;
//   sites  : [PointId, x, y, z] par site (indice = rang de Morton) ;
//   balls  : ordre canonique du catalogue ; centre exact absolu [n0, n1, n2, den] ; I, U tries par indice de site ;
//   orders : par ordre, noeuds [lv, parent, naissance, enfants], entrees core (D_K entier, noeud) et cover
//            (lv de alpha_K^2, noeud), ball_node (composante du centre de chaque boule de population >= K a son
//            propre niveau ; null a K = 1, ou les temoins sont les sites au niveau nul).
// Codes : 0 conforme ; 2 refus (arguments, entree, refus du moteur) ; 3 invariant viole (moteur, ou controle de
// coherence de cet exporteur : les deux constructions doivent donner la meme foret, les indices doivent etre
// dans leurs bornes, les dates des noeuds ne doivent pas depasser les dates d'entree).
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "tower/tower.hpp"

#ifndef MHGP10_FRONTIER_ENGINE_COMMIT
#define MHGP10_FRONTIER_ENGINE_COMMIT "inconnu"
#endif

using namespace mhgp10;

namespace {

constexpr const char* kSchema = "mhgp10_frontier_export_v1";

using Clock = std::chrono::steady_clock;
double seconds_since(Clock::time_point t0) { return std::chrono::duration<double>(Clock::now() - t0).count(); }

// Refus : une ligne JSON sur la sortie standard, code 2 (ou 3 pour un invariant viole).
int report(const char* status, const std::string& reason, int code) {
  std::printf("{\"status\":\"%s\",\"reason\":\"%s\"}\n", status, reason.c_str());
  return code;
}

int engine_failure(const Outcome& o) {
  const bool inv = o.status() == Status::invariant_violated;
  return report(std::string(status_name(o.status())).c_str(),
                std::string(reason_name(o.reason)) + (o.order ? " order=" + std::to_string(o.order) : ""),
                inv ? 3 : 2);
}

// Entier decimal strict (chiffres seulement, sans signe ni blanc) ; false si invalide ou trop grand.
bool parse_uint(const std::string& s, unsigned long& out) {
  if (s.empty() || s.size() > 9) return false;
  for (char ch : s)
    if (ch < '0' || ch > '9') return false;
  out = std::strtoul(s.c_str(), nullptr, 10);
  return true;
}

std::string str128(i128 v) { return arith::to_string(arith::Wide<2>::from_i128(v)); }

// Ecriture tamponnee dans un FILE* ; l'etat d'erreur est lu une fois a la fin.
struct Out {
  FILE* f = nullptr;
  void s(const char* t) { std::fputs(t, f); }
  void s(const std::string& t) { std::fputs(t.c_str(), f); }
  void u(unsigned long long v) { std::fprintf(f, "%llu", v); }
  void idx(u32 v) {  // indice ou -1 pour kNone
    if (v == kNone) s("-1");
    else u(v);
  }
  void q(const std::string& t) {  // chaine sans echappement (chiffres et signe seulement)
    std::fputc('"', f);
    s(t);
    std::fputc('"', f);
  }
  template <class Range>
  void arr(const Range& r) {
    std::fputc('[', f);
    bool first = true;
    for (auto v : r) {
      if (!first) std::fputc(',', f);
      first = false;
      idx(static_cast<u32>(v));
    }
    std::fputc(']', f);
  }
};

bool same_forest(const OrderForest& a, const OrderForest& b) {
  return a.k == b.k && a.rank == b.rank && a.parent == b.parent && a.child_off == b.child_off &&
         a.child_val == b.child_val && a.birth == b.birth && a.lower == b.lower;
}

// Controles de bornes et de dates d'un ordre (invariants de l'export, independants du moteur). Rend un message vide
// si tout est conforme.
std::string check_order(const Catalogue& cat, const OrderForest& core, const OrderForest& cover, u32 sites) {
  const u32 nn = static_cast<u32>(core.rank.size());
  const u32 nlv = static_cast<u32>(cat.level.size());
  if (core.parent.size() != nn || core.birth.size() != nn || core.child_off.size() != u64(nn) + 1 ||
      core.child_val.size() != core.child_off[nn])
    return "tailles de la foret";
  u32 roots = 0;
  for (u32 v = 0; v < nn; ++v) {
    if (core.rank[v] > nlv) return "lv de noeud hors table";
    const u32 p = core.parent[v];
    if (p == kNone) ++roots;
    else if (p >= nn || core.rank[p] < core.rank[v]) return "parent invalide";
    const u32 nk = core.child_off[v + 1] - core.child_off[v];
    if ((core.birth[v] == kNone) != (nk >= 2)) return "naissance/fusion incoherente";
    for (u32 j = core.child_off[v]; j < core.child_off[v + 1]; ++j) {
      const u32 c = core.child_val[j];
      if (c >= nn || core.parent[c] != v) return "enfant invalide";
    }
    if (core.birth[v] != kNone) {
      if (core.k == 1 ? core.birth[v] >= sites : core.birth[v] >= cat.balls()) return "naissance hors bornes";
      if (core.k >= 2 && core.rank[v] != cat.rank[core.birth[v]] + 1) return "niveau de naissance";
    }
  }
  if (roots != 1) return "nombre de racines";
  if (core.point_node.size() != sites || core.point_level.size() != sites || cover.point_node.size() != sites)
    return "tailles des entrees";
  for (u32 x = 0; x < sites; ++x) {
    const u32 v = core.point_node[x];
    if (v >= nn) return "noeud core hors bornes";
    // date du noeud core <= D_K(x) (entier exact)
    if (core.rank[v] > 0) {
      geom::Level d;
      d.num = arith::I192::from_u128(core.point_level[x]);
      d.den = arith::I128w::from_u128(1);
      if (geom::compare(cat.level[core.rank[v] - 1], d) > 0) return "noeud core posterieur a D_K";
    }
    const u32 w = cover.point_node[x];
    if (w >= nn) return "noeud cover hors bornes";
    if (core.k >= 2) {
      if (cover.point_cat_rank.size() != sites) return "taille des niveaux cover";
      const u32 r = cover.point_cat_rank[x];
      if (r == 0 || r > nlv || core.rank[w] > r) return "niveau cover";
    } else if (core.rank[w] != 0 || core.point_level[x] != 0) {
      return "entree K = 1 non nulle";
    }
  }
  if (core.k >= 2) {
    if (cover.ball_node.size() != cat.balls()) return "taille de ball_node";
    for (u32 b = 0; b < cat.balls(); ++b) {
      const u32 v = cover.ball_node[b];
      const bool covering = u64(cat.p[b]) + cat.u[b] >= u64(core.k) &&
                            cat.pop_off[b + 1] - cat.pop_off[b] >= u64(core.k);
      if (covering != (v != kNone)) return "ball_node : population";
      if (v != kNone && (v >= nn || core.rank[v] > cat.rank[b] + 1)) return "ball_node hors bornes ou posterieur";
    }
  } else if (!cover.ball_node.empty()) {
    return "ball_node non vide a K = 1";
  }
  return {};
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr,
                 "usage: export_frontier IN.u32le --k=K --out=FICHIER [--threads=W] [--kmax-catalogue=KC] "
                 "[--all-orders]\n");
    return 2;
  }
  unsigned long K = 0, KC = 0, threads = 1;
  std::string out_path;
  bool all_orders = false;
  for (int i = 2; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--k=", 0) == 0) {
      if (!parse_uint(a.substr(4), K)) return report("invalid_input", "k", 2);
    } else if (a.rfind("--kmax-catalogue=", 0) == 0) {
      if (!parse_uint(a.substr(17), KC)) return report("invalid_input", "kmax-catalogue", 2);
    } else if (a.rfind("--threads=", 0) == 0) {
      if (!parse_uint(a.substr(10), threads) || threads == 0 || threads > 256)
        return report("invalid_input", "threads", 2);
    } else if (a.rfind("--out=", 0) == 0) {
      out_path = a.substr(6);
    } else if (a == "--all-orders") {
      all_orders = true;
    } else {
      return report("invalid_input", "option inconnue " + a, 2);
    }
  }
  if (K < 1 || K > static_cast<unsigned long>(kMaxOrder)) return report("invalid_input", "k_out_of_range", 2);
  if (KC == 0) KC = K;
  if (KC < K || KC > static_cast<unsigned long>(kMaxCatalogueOrder))
    return report("invalid_input", "kmax_out_of_range", 2);
  if (out_path.empty()) return report("invalid_input", "--out manquant", 2);

  // Lecture stricte : triplets u32 petit-boutistes, taille multiple de 12 octets, non vide.
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return report("invalid_input", "entree illisible", 2);
  std::vector<unsigned char> bytes;
  {
    unsigned char buf[1 << 16];
    size_t got;
    while ((got = std::fread(buf, 1, sizeof buf, f)) > 0) bytes.insert(bytes.end(), buf, buf + got);
    const bool err = std::ferror(f) != 0;
    std::fclose(f);
    if (err) return report("invalid_input", "lecture", 2);
  }
  if (bytes.empty() || bytes.size() % 12 != 0) return report("invalid_input", "taille non multiple de 12", 2);
  const u64 n64 = bytes.size() / 12;
  if (n64 >= kNone) return report("invalid_input", "index_overflow_u32", 2);
  const u32 n = static_cast<u32>(n64);
  auto le32 = [&](u64 off) {
    return u32(bytes[off]) | (u32(bytes[off + 1]) << 8) | (u32(bytes[off + 2]) << 16) | (u32(bytes[off + 3]) << 24);
  };
  std::vector<u32> x(n), y(n), z(n), pid(n);
  for (u32 i = 0; i < n; ++i) {
    x[i] = le32(12 * u64(i));
    y[i] = le32(12 * u64(i) + 4);
    z[i] = le32(12 * u64(i) + 8);
    pid[i] = i;
  }
  bytes.clear();
  bytes.shrink_to_fit();

  auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
  if (!prepared.ok()) return engine_failure(prepared.outcome());
  const Cloud& cloud = prepared.value();
  const u32 sites = cloud.sites();
  if (K > sites) return report("invalid_input", "k_out_of_range (K > sites)", 2);
  sched::Pool pool(static_cast<unsigned>(threads));
  SiteTree tree(cloud);

  const auto t0 = Clock::now();
  CatalogueParams cp;
  cp.kmax = static_cast<int>(KC);
  auto built = build_catalogue(cloud, cp, pool);
  if (!built.ok()) return engine_failure(built.outcome());
  const Catalogue& cat = built.value();
  const double t_cat = seconds_since(t0);

  TowerParams tp;
  tp.kmax = static_cast<int>(K);
  tp.points = true;
  tp.ball_nodes = true;
  tp.cover_extra = 0;
  tp.only_order = all_orders ? 0 : static_cast<int>(K);
  tp.verticals = all_orders;
  auto t1 = Clock::now();
  tp.entry = PointEntry::core;
  auto tw_core = build_tower(cloud, tree, cat, tp, pool);
  if (!tw_core.ok()) return engine_failure(tw_core.outcome());
  const double t_core = seconds_since(t1);
  t1 = Clock::now();
  tp.entry = PointEntry::cover;
  auto tw_cover = build_tower(cloud, tree, cat, tp, pool);
  if (!tw_cover.ok()) return engine_failure(tw_cover.outcome());
  const double t_cover = seconds_since(t1);
  const Tower& core = tw_core.value();
  const Tower& cover = tw_cover.value();

  const u32 kfirst = all_orders ? 1 : static_cast<u32>(K);
  if (core.orders.size() < K || cover.orders.size() < K) return report("invariant_violated", "ordres absents", 3);
  u64 nodes_total = 0;
  for (u32 k = kfirst; k <= K; ++k) {
    const OrderForest& a = core.orders[k - 1];
    const OrderForest& b = cover.orders[k - 1];
    if (a.k != static_cast<int>(k)) return report("invariant_violated", "ordre " + std::to_string(k) + " absent", 3);
    if (!same_forest(a, b))
      return report("invariant_violated", "forets core/cover differentes a k=" + std::to_string(k), 3);
    const std::string err = check_order(cat, a, b, sites);
    if (!err.empty()) return report("invariant_violated", err + " a k=" + std::to_string(k), 3);
    nodes_total += a.rank.size();
  }

  // ---- ecriture
  t1 = Clock::now();
  Out o;
  o.f = std::fopen(out_path.c_str(), "w");
  if (!o.f) return report("invalid_input", "sortie non inscriptible", 2);
  o.s("{\"schema\":");
  o.q(kSchema);
  o.s(",\"engine_commit\":");
  o.q(MHGP10_FRONTIER_ENGINE_COMMIT);
  o.s(",\"K\":");
  o.u(K);
  o.s(",\"kmax_catalogue\":");
  o.u(KC);
  o.s(",\"orders_built\":");
  o.q(all_orders ? "all_orders" : "only_order");
  o.s(",\"coordinate_bits\":");
  o.u(kCoordinateBits);
  o.s(",\"n_points\":");
  o.u(n);
  o.s(",\"n_sites\":");
  o.u(sites);
  o.s(",\n\"levels\":[[\"0\",\"1\"]");
  for (u32 r = 0; r < cat.level.size(); ++r) {
    o.s(",\n[");
    o.q(arith::to_string(cat.level[r].num));
    o.s(",");
    o.q(arith::to_string(cat.level[r].den));
    o.s("]");
  }
  o.s("],\n\"sites\":[");
  for (u32 s = 0; s < sites; ++s) {
    const auto ids = cloud.ids.row(s);
    if (ids.size() != 1) {  // multiplicites refusees par la tour : jamais atteint
      std::fclose(o.f);
      return report("invariant_violated", "site a plusieurs points", 3);
    }
    o.s(s ? ",\n[" : "\n[");
    o.u(idx(ids[0]));
    o.s(",");
    o.u(cloud.x[s]);
    o.s(",");
    o.u(cloud.y[s]);
    o.s(",");
    o.u(cloud.z[s]);
    o.s("]");
  }
  o.s("],\n\"balls\":[");
  for (u32 b = 0; b < cat.balls(); ++b) {
    o.s(b ? ",\n{\"lv\":" : "\n{\"lv\":");
    o.u(u64(cat.rank[b]) + 1);
    o.s(",\"q\":");
    o.u(cat.qmin[b]);
    o.s(",\"p\":");
    o.u(cat.p[b]);
    o.s(",\"u\":");
    o.u(cat.u[b]);
    o.s(",\"flags\":");
    o.u(cat.flags[b]);
    o.s(",\"S\":[");
    for (u32 j = 0; j < cat.qmin[b]; ++j) {
      if (j) o.s(",");
      o.u(cat.support[b][j]);
    }
    // centre absolu : (ancre * D + N) / D, ancre = premier site du support (i128 suffit : |.| < 2^101 en u18)
    const geom::Center c = ball_center(cloud, cat, b);
    const u32 a = cat.support[b][0];
    const i128 ax = i128(cloud.x[a]), ay = i128(cloud.y[a]), az = i128(cloud.z[a]);
    o.s("],\"c\":[");
    o.q(str128(ax * c.D + c.N[0]));
    o.s(",");
    o.q(str128(ay * c.D + c.N[1]));
    o.s(",");
    o.q(str128(az * c.D + c.N[2]));
    o.s(",");
    o.q(str128(c.D));
    o.s("],\"I\":");
    o.arr(cat.interior(b));
    o.s(",\"U\":");
    o.arr(cat.shell(b));
    o.s("}");
  }
  o.s("],\n\"orders\":[");
  for (u32 k = kfirst; k <= K; ++k) {
    const OrderForest& a = core.orders[k - 1];
    const OrderForest& b = cover.orders[k - 1];
    const u32 nn = static_cast<u32>(a.rank.size());
    o.s(k > kfirst ? ",\n{\"k\":" : "\n{\"k\":");
    o.u(k);
    o.s(",\"nodes\":[");
    for (u32 v = 0; v < nn; ++v) {
      o.s(v ? ",\n[" : "\n[");
      o.u(a.rank[v]);
      o.s(",");
      o.idx(a.parent[v]);
      o.s(",");
      o.idx(a.birth[v]);
      o.s(",[");
      for (u32 j = a.child_off[v]; j < a.child_off[v + 1]; ++j) {
        if (j > a.child_off[v]) o.s(",");
        o.u(a.child_val[j]);
      }
      o.s("]]");
    }
    o.s("],\n\"lower\":");
    if (a.lower.empty()) o.s("null");
    else o.arr(a.lower);
    o.s(",\n\"core_level\":[");
    for (u32 s = 0; s < sites; ++s) {
      if (s) o.s(",");
      o.u(a.point_level[s]);
    }
    o.s("],\n\"core_node\":");
    o.arr(a.point_node);
    o.s(",\n\"cover_lv\":[");
    for (u32 s = 0; s < sites; ++s) {
      if (s) o.s(",");
      o.u(k >= 2 ? b.point_cat_rank[s] : 0);
    }
    o.s("],\n\"cover_node\":");
    o.arr(b.point_node);
    o.s(",\n\"ball_node\":");
    if (k >= 2) o.arr(b.ball_node);
    else o.s("null");
    o.s("}");
  }
  o.s("]}\n");
  const bool werr = std::ferror(o.f) != 0;
  const bool cerr = std::fclose(o.f) != 0;
  if (werr || cerr) return report("invalid_input", "ecriture de la sortie", 2);
  const double t_write = seconds_since(t1);

  std::printf("{\"status\":\"ok\",\"schema\":\"%s\",\"K\":%lu,\"kmax_catalogue\":%lu,\"orders_built\":\"%s\","
              "\"n_points\":%u,\"n_sites\":%u,\"balls\":%u,\"levels\":%zu,\"nodes\":%llu,\"threads\":%u,"
              "\"catalogue_s\":%.4f,\"tower_core_s\":%.4f,\"tower_cover_s\":%.4f,\"write_s\":%.4f}\n",
              kSchema, K, KC, all_orders ? "all_orders" : "only_order", n, sites, cat.balls(), cat.level.size(),
              static_cast<unsigned long long>(nodes_total), pool.size(), t_cat, t_core, t_cover, t_write);
  return 0;
}
