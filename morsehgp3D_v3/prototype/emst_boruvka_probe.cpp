// MorseHGP3D v3 — LE PROBE + JUGE + PORTES DE LA LANE k=1 (EMST exact par
// Boruvka sur LBVH, PROPOSITION §6.1) : « l'ordre un est exactement le
// single linkage, au niveau distance_squared/4. La route candidate calcule
// un EMST par Boruvka exact sur points u16, trie ses aretes par niveau
// rationnel et rejoue chaque lot d'egalite atomiquement. La reception
// compare, apres chaque coupe stricte et fermee, les partitions canoniques
// de PointId a l'oracle EMST CPU. »
//
// LE JUGE (--oracle 1, n <= 2048) est INDEPENDANT : Prim O(n^2) en i64/i128
// reecrit ICI (structure inspiree de structural_scale_check.cpp mais sans
// rien partager : ni le LBVH, ni le DSU, ni les helpers du sujet). La
// comparaison N'EST PAS l'egalite des ensembles d'aretes (l'EMST n'est pas
// unique sous ex aequo) : c'est (a) l'egalite du MULTIENSEMBLE des n-1
// niveaux d^2 et (b) l'egalite des PARTITIONS canoniques de PointId apres
// CHAQUE coupe stricte ET fermee, a chaque niveau distinct (union des
// niveaux du juge et du sujet), chaque lot d'egalite applique ATOMIQUEMENT.
//
// LES QUATRE FIXTURES GRAVEES (feuilles 2, oracle force ; verification
// numerique hors bande en Python avant gravure — voir le rapport de
// session) : tie-lattice (grille 4x4x1, 24 aretes ex aequo, jeu canonique
// force), colocated-batch (3 colocalises, lot de niveau 0 atomique),
// u16-extremes (les 8 coins du cube {0,65535}^3, aucun debordement),
// two-clusters (une unique arete pont, assertion directe de la plus proche
// paire inter-amas).
//
// LE REPLAY CANONIQUE : quand n <= 4096, le sujet est rejoue sur le MEME
// nuage avec d'autres tailles de feuilles — la sortie canonique (aretes ET
// niveaux, sequences exactes) est INDEPENDANTE de la forme de l'arbre ;
// c'est la porte qui mord le departage par ordre de decouverte.
//
// L'EQUIVARIANCE (--permute 1, n <= 4096) : nuage renverse + melange LCG
// grave — memes multiensembles de niveaux et memes partitions apres
// renommage (les ARETES peuvent differer legitimement sous ex aequo).
//
// Codes de sortie EXACTS : 0 OK ; 1 desaccords du juge / invariant viole /
// contrat de cardinalite ; 2 campagne refusee avant calcul ; 3 plancher ou
// nuage non genere ; 4 mutant tue. Aucun crash par signal.
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/emst_boruvka.hpp"
#include "prototype/morton_lbvh.hpp"

namespace {

using i64 = long long;
using i128 = __int128;

// ---------------------------------------------------------------------------
// LE DSU DES COUPES ET DU JUGE — independant du DSU du sujet (representation
// volontairement differente : union par RANG, reecriture complete du chemin).
// ---------------------------------------------------------------------------
struct CutDsu {
  std::vector<int> parent;
  std::vector<int> rank_;
  void init(int n) {
    parent.resize((std::size_t)n);
    rank_.assign((std::size_t)n, 0);
    for (int i = 0; i < n; ++i) parent[(std::size_t)i] = i;
  }
  int find(int x) {
    int r = x;
    while (parent[(std::size_t)r] != r) r = parent[(std::size_t)r];
    while (parent[(std::size_t)x] != r) {
      const int next = parent[(std::size_t)x];
      parent[(std::size_t)x] = r;
      x = next;
    }
    return r;
  }
  void unite(int a, int b) {
    const int ra = find(a), rb = find(b);
    if (ra == rb) return;
    if (rank_[(std::size_t)ra] < rank_[(std::size_t)rb]) parent[(std::size_t)ra] = rb;
    else if (rank_[(std::size_t)rb] < rank_[(std::size_t)ra]) parent[(std::size_t)rb] = ra;
    else {
      parent[(std::size_t)rb] = ra;
      ++rank_[(std::size_t)ra];
    }
  }
};

struct EdgeLvl {
  i64 lvl;
  int a, b;
};

// La representation CANONIQUE d'une partition : rep[i] = plus petit PointId
// de la composante de i.
void reps_of(CutDsu* dsu, int n, std::vector<int>* out, std::vector<int>* scratch) {
  scratch->assign((std::size_t)n, n);
  out->resize((std::size_t)n);
  for (int i = 0; i < n; ++i) {
    const int r = dsu->find(i);
    if (i < (*scratch)[(std::size_t)r]) (*scratch)[(std::size_t)r] = i;
  }
  for (int i = 0; i < n; ++i) (*out)[(std::size_t)i] = (*scratch)[(std::size_t)dsu->find(i)];
}

// La partition du graphe des aretes de niveau < cut (stricte) ou <= cut
// (fermee) — pour les assertions de fixtures (une coupe isolee).
std::vector<int> cut_reps(int n, const std::vector<EdgeLvl>& edges, i64 cut, bool closed) {
  CutDsu dsu;
  dsu.init(n);
  for (const EdgeLvl& e : edges)
    if (e.lvl < cut || (closed && e.lvl == cut)) dsu.unite(e.a, e.b);
  std::vector<int> reps, scratch;
  reps_of(&dsu, n, &reps, &scratch);
  return reps;
}

int component_count(const std::vector<int>& reps) {
  int count = 0;
  for (std::size_t i = 0; i < reps.size(); ++i)
    if (reps[i] == (int)i) ++count;
  return count;
}

// ---------------------------------------------------------------------------
// LA COMPARAISON SINGLE-LINKAGE : multiensemble des niveaux, puis partitions
// canoniques apres chaque coupe STRICTE et FERMEE a chaque niveau distinct
// (union des niveaux des deux cotes), lots d'egalite appliques ATOMIQUEMENT.
// Retourne une chaine vide si tout coincide, sinon le premier desaccord.
// ---------------------------------------------------------------------------
std::string compare_single_linkage(int n, std::vector<EdgeLvl> lhs, std::vector<EdgeLvl> rhs,
                                   const char* lhs_name, const char* rhs_name) {
  char buf[224];
  const auto by_lvl = [](const EdgeLvl& x, const EdgeLvl& y) {
    if (x.lvl != y.lvl) return x.lvl < y.lvl;
    if (x.a != y.a) return x.a < y.a;
    return x.b < y.b;
  };
  std::sort(lhs.begin(), lhs.end(), by_lvl);
  std::sort(rhs.begin(), rhs.end(), by_lvl);
  if (lhs.size() != rhs.size()) {
    std::snprintf(buf, sizeof buf, "%zu aretes (%s) contre %zu (%s)", lhs.size(), lhs_name,
                  rhs.size(), rhs_name);
    return buf;
  }
  for (std::size_t k = 0; k < lhs.size(); ++k)
    if (lhs[k].lvl != rhs[k].lvl) {
      std::snprintf(buf, sizeof buf,
                    "multiensembles de niveaux divergents au rang %zu : %lld (%s) contre"
                    " %lld (%s)", k, lhs[k].lvl, lhs_name, rhs[k].lvl, rhs_name);
      return buf;
    }
  std::vector<i64> cuts;
  cuts.reserve(lhs.size() + rhs.size());
  for (const EdgeLvl& e : lhs) cuts.push_back(e.lvl);
  for (const EdgeLvl& e : rhs) cuts.push_back(e.lvl);
  std::sort(cuts.begin(), cuts.end());
  cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());
  CutDsu da, db;
  da.init(n);
  db.init(n);
  std::vector<int> ra, rb, scratch;
  std::size_t pa = 0, pb = 0;
  for (const i64 c : cuts) {
    // COUPE STRICTE au niveau c : tous les lots < c sont deja appliques.
    reps_of(&da, n, &ra, &scratch);
    reps_of(&db, n, &rb, &scratch);
    for (int i = 0; i < n; ++i)
      if (ra[(std::size_t)i] != rb[(std::size_t)i]) {
        std::snprintf(buf, sizeof buf,
                      "partitions divergentes a la coupe STRICTE au niveau %lld : PointId %d"
                      " a le representant %d (%s) contre %d (%s)", c, i, ra[(std::size_t)i],
                      lhs_name, rb[(std::size_t)i], rhs_name);
        return buf;
      }
    // LE LOT D'EGALITE AU NIVEAU c, applique ATOMIQUEMENT de chaque cote.
    while (pa < lhs.size() && lhs[pa].lvl == c) {
      da.unite(lhs[pa].a, lhs[pa].b);
      ++pa;
    }
    while (pb < rhs.size() && rhs[pb].lvl == c) {
      db.unite(rhs[pb].a, rhs[pb].b);
      ++pb;
    }
    // COUPE FERMEE au niveau c.
    reps_of(&da, n, &ra, &scratch);
    reps_of(&db, n, &rb, &scratch);
    for (int i = 0; i < n; ++i)
      if (ra[(std::size_t)i] != rb[(std::size_t)i]) {
        std::snprintf(buf, sizeof buf,
                      "partitions divergentes a la coupe FERMEE au niveau %lld : PointId %d"
                      " a le representant %d (%s) contre %d (%s)", c, i, ra[(std::size_t)i],
                      lhs_name, rb[(std::size_t)i], rhs_name);
        return buf;
      }
  }
  return {};
}

// ---------------------------------------------------------------------------
// LE JUGE : Prim O(n^2) en i64/i128, REECRIT ici — aucune primitive du sujet.
// Le choix d'arete du juge sous ex aequo est libre (premier minimum) : seul
// le couple (multiensemble, partitions) est contractuel.
// ---------------------------------------------------------------------------
std::vector<EdgeLvl> judge_prim(const std::vector<mhgp::P3>& pts) {
  const int n = (int)pts.size();
  const auto squared = [&pts](int a, int b) -> i128 {
    const i128 dx = pts[(std::size_t)a].x - pts[(std::size_t)b].x;
    const i128 dy = pts[(std::size_t)a].y - pts[(std::size_t)b].y;
    const i128 dz = pts[(std::size_t)a].z - pts[(std::size_t)b].z;
    return dx * dx + dy * dy + dz * dz;
  };
  std::vector<unsigned char> in_tree((std::size_t)n, 0);
  std::vector<i128> best((std::size_t)n, 0);
  std::vector<int> from((std::size_t)n, 0);
  for (int v = 1; v < n; ++v) best[(std::size_t)v] = squared(0, v);
  in_tree[0] = 1;
  std::vector<EdgeLvl> edges;
  edges.reserve((std::size_t)(n - 1));
  for (int step = 1; step < n; ++step) {
    int chosen = -1;
    for (int v = 0; v < n; ++v)
      if (!in_tree[(std::size_t)v] &&
          (chosen < 0 || best[(std::size_t)v] < best[(std::size_t)chosen]))
        chosen = v;
    in_tree[(std::size_t)chosen] = 1;
    const int u = from[(std::size_t)chosen];
    // u16 : d^2 <= 3*65535^2 < 2^34 — la reduction i128 -> i64 est exacte.
    edges.push_back(EdgeLvl{(i64)best[(std::size_t)chosen], u < chosen ? u : chosen,
                            u < chosen ? chosen : u});
    for (int v = 0; v < n; ++v)
      if (!in_tree[(std::size_t)v]) {
        const i128 d = squared(chosen, v);
        if (d < best[(std::size_t)v]) {
          best[(std::size_t)v] = d;
          from[(std::size_t)v] = chosen;
        }
      }
  }
  return edges;
}

// ---------------------------------------------------------------------------
// Les invariants d'arbre couvrant du sujet (mordent skip-last-round).
// ---------------------------------------------------------------------------
std::string check_spanning(int n, const mhgp3v::EmstResult& res) {
  char buf[192];
  if (res.levels4.size() != res.edges.size()) {
    std::snprintf(buf, sizeof buf, "%zu niveaux pour %zu aretes", res.levels4.size(),
                  res.edges.size());
    return buf;
  }
  if ((int)res.edges.size() != n - 1) {
    std::snprintf(buf, sizeof buf,
                  "%zu aretes publiees, n-1=%d attendues (arbre couvrant viole)",
                  res.edges.size(), n - 1);
    return buf;
  }
  CutDsu dsu;
  dsu.init(n);
  for (std::size_t k = 0; k < res.edges.size(); ++k) {
    const int a = res.edges[k][0], b = res.edges[k][1];
    if (a < 0 || b >= n || a >= b) {
      std::snprintf(buf, sizeof buf, "arete %zu invalide : (%d,%d)", k, a, b);
      return buf;
    }
    if (res.levels4[k] < 0) {
      std::snprintf(buf, sizeof buf, "niveau negatif %lld a l'arete %zu", res.levels4[k], k);
      return buf;
    }
    dsu.unite(a, b);
  }
  int comps = 0;
  for (int i = 0; i < n; ++i)
    if (dsu.find(i) == i) ++comps;
  if (comps != 1) {
    std::snprintf(buf, sizeof buf, "les aretes publiees laissent %d composantes", comps);
    return buf;
  }
  return {};
}

std::vector<EdgeLvl> to_edge_lvls(const mhgp3v::EmstResult& res) {
  std::vector<EdgeLvl> out;
  out.reserve(res.edges.size());
  for (std::size_t k = 0; k < res.edges.size(); ++k)
    out.push_back(EdgeLvl{res.levels4[k], res.edges[k][0], res.edges[k][1]});
  return out;
}

mhgp::P3 pt(int x, int y, int z) {
  mhgp::P3 p{};
  p.x = (mhgp::i64)x;
  p.y = (mhgp::i64)y;
  p.z = (mhgp::i64)z;
  return p;
}

}  // namespace

int main(int argc, char** argv) {
  int n = 512, coord = 0, leaf_size = 8, oracle = 0, permute = 0;
  i64 seed = 20260810;
  i64 min_rounds = 0, min_distinct_levels = 0, min_tie_batches = 0;
  mhgp3v::CloudFamily family = mhgp3v::CloudFamily::kTerrain;
  std::string fixture_name;
  mhgp3v::EmstInjections injections;
  const auto integer = [](const char* text, i64* value) {
    const char* last = text + strlen(text);
    unsigned long long magnitude = 0;
    const auto r = std::from_chars(text, last, magnitude);
    if (text == last || r.ec != std::errc{} || r.ptr != last) return false;
    if (magnitude > 1000000000000ULL) return false;
    *value = (i64)magnitude;
    return true;
  };
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--family")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --family\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "uniform")) family = mhgp3v::CloudFamily::kUniform;
      else if (!strcmp(argv[i], "terrain")) family = mhgp3v::CloudFamily::kTerrain;
      else if (!strcmp(argv[i], "scanline_single_pass"))
        family = mhgp3v::CloudFamily::kScanlineSinglePass;
      else if (!strcmp(argv[i], "scanline_overlap_multiecho"))
        family = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
      else { std::printf("ECHEC : famille inconnue %s\n", argv[i]); return 2; }
      continue;
    }
    if (!strcmp(argv[i], "--fixture")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --fixture\n"); return 2; }
      fixture_name = argv[++i];
      continue;
    }
    if (!strcmp(argv[i], "--inject")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --inject\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "reconnect-wrong-components"))
        injections.reconnect_wrong_components = true;
      else if (!strcmp(argv[i], "level-off-by-one")) injections.level_off_by_one = true;
      else if (!strcmp(argv[i], "skip-last-round")) injections.skip_last_round = true;
      else if (!strcmp(argv[i], "purity-stale")) injections.purity_stale = true;
      else if (!strcmp(argv[i], "tie-nondeterministic")) injections.tie_nondeterministic = true;
      else { std::printf("ECHEC : injection inconnue %s\n", argv[i]); return 2; }
      continue;
    }
    i64 value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    if (!has) { std::printf("ECHEC : argument %s sans valeur\n", argv[i]); return 2; }
    if (!strcmp(argv[i], "--points")) n = (int)value;
    else if (!strcmp(argv[i], "--coord")) coord = (int)value;
    else if (!strcmp(argv[i], "--seed")) seed = value;
    else if (!strcmp(argv[i], "--leaf-size")) leaf_size = (int)value;
    else if (!strcmp(argv[i], "--oracle")) oracle = (int)value;
    else if (!strcmp(argv[i], "--permute")) permute = (int)value;
    else if (!strcmp(argv[i], "--min-rounds")) min_rounds = value;
    else if (!strcmp(argv[i], "--min-distinct-levels")) min_distinct_levels = value;
    else if (!strcmp(argv[i], "--min-tie-batches")) min_tie_batches = value;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    ++i;
  }
  if (n < 4 || n > 100000 || coord < 0 || coord > 65536 || leaf_size < 2 ||
      leaf_size > 256 || oracle < 0 || oracle > 1 || permute < 0 || permute > 1) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }

  // -------------------------------------------------------------------------
  // Le nuage : fixture gravee (feuilles 2, oracle force) ou famille partagee
  // avec les DEUX gardes de cardinalite du generateur.
  // -------------------------------------------------------------------------
  const bool is_fixture = !fixture_name.empty();
  std::vector<mhgp::P3> pts;
  if (is_fixture) {
    if (fixture_name == "tie-lattice") {
      // GRILLE 4x4x1 au pas 100 : 24 aretes de grille toutes a d^2 = 10000
      // (ex aequo massifs, plusieurs EMST valides). Verifie hors bande : 15
      // aretes EMST au seul niveau 10000 ; la coupe stricte a 10000 rend 16
      // singletons, la fermee UNE composante ; UN lot d'egalite.
      for (int j = 0; j < 4; ++j)
        for (int i = 0; i < 4; ++i) pts.push_back(pt(1000 + 100 * i, 1000 + 100 * j, 500));
    } else if (fixture_name == "colocated-batch") {
      // 6 points dont 3 COLOCALISES : lot atomique de niveau 0. Verifie hors
      // bande : niveaux {0,0,100,400,2500} ; la coupe fermee a 0 fusionne
      // exactement {0,1,2}, la stricte laisse 6 singletons.
      pts = {pt(30000, 30000, 30000), pt(30000, 30000, 30000), pt(30000, 30000, 30000),
             pt(30010, 30000, 30000), pt(30030, 30000, 30000), pt(30000, 30050, 30000)};
    } else if (fixture_name == "u16-extremes") {
      // LES 8 COINS du cube {0,65535}^3 : 7 aretes au niveau 65535^2 =
      // 4294836225 ; la diagonale d'espace 3*65535^2 < 2^34 est testee par le
      // juge sans deborder. Verifie hors bande.
      for (int c = 0; c < 8; ++c)
        pts.push_back(pt(65535 * (c & 1), 65535 * ((c >> 1) & 1), 65535 * ((c >> 2) & 1)));
    } else if (fixture_name == "two-clusters") {
      // DEUX AMAS relies par une UNIQUE arete pont (3,4) au niveau
      // 58970^2+58960^2+58925^2 = 10425898125 ; toutes les distances de la
      // fixture sont deux a deux distinctes (aucun ex aequo). Verifie hors
      // bande : la coupe stricte sous le pont rend exactement 2 composantes.
      pts = {pt(1000, 1000, 1000),    pt(1080, 1000, 1000),  pt(1000, 1090, 1000),
             pt(1030, 1040, 1075),    pt(60000, 60000, 60000), pt(60063, 60000, 60000),
             pt(60000, 60077, 60000), pt(60041, 60040, 60061)};
    } else {
      std::printf("ECHEC : fixture inconnue %s\n", fixture_name.c_str());
      return 2;
    }
    n = (int)pts.size();
    leaf_size = 2;
    oracle = 1;
    std::printf("provenance : --fixture %s (%d points graves, feuilles <= %d, oracle"
                " force)\n", fixture_name.c_str(), n, leaf_size);
  } else {
    if (coord == 0) coord = mhgp3v::cloud_family_default_coord(family, n);
    pts = mhgp3v::make_family_cloud(family, n, coord, seed);
    if ((int)pts.size() < n) { std::printf("ECHEC : nuage non genere\n"); return 3; }
    if ((int)pts.size() != n) {
      std::printf("ECHEC : contrat de cardinalite viole — %zu points rendus pour %d"
                  " demandes\n", pts.size(), n);
      return 1;
    }
    std::printf("provenance : --points %d --coord %d --seed %lld --family %s"
                " --leaf-size %d\n", n, coord, seed, mhgp3v::cloud_family_name(family),
                leaf_size);
  }
  if (oracle == 1 && n > 2048) {
    std::printf("ECHEC : l'oracle Prim exige n <= 2048\n");
    return 2;
  }
  if (permute == 1 && n > 4096) {
    std::printf("ECHEC : l'equivariance exige n <= 4096\n");
    return 2;
  }

  const auto fail = [&injections](const char* what, const char* detail) {
    if (injections.any()) {
      std::printf("mutant tue par %s : %s\n", what, detail);
      return 4;
    }
    std::printf("ECHEC %s : %s\n", what, detail);
    return 1;
  };

  // -------------------------------------------------------------------------
  // LE SUJET : LBVH partage + Boruvka exact (chronometres separement).
  // -------------------------------------------------------------------------
  mhgp3v::MortonLbvh tree;
  const auto t0 = std::chrono::steady_clock::now();
  tree.build(pts, leaf_size);
  const auto t1 = std::chrono::steady_clock::now();
  const mhgp3v::EmstResult subject = mhgp3v::emst_boruvka(tree, injections);
  const auto t2 = std::chrono::steady_clock::now();
  std::printf("temps      : lbvh %.3f s, boruvka %.3f s (mono-thread)\n",
              std::chrono::duration<double>(t1 - t0).count(),
              std::chrono::duration<double>(t2 - t1).count());
  if (!subject.ok) return fail("le coeur Boruvka", subject.refusal);
  {
    const std::string what = check_spanning(n, subject);
    if (!what.empty()) return fail("l'assertion d'arbre couvrant", what.c_str());
  }

  // Les statistiques de niveaux : distincts et lots d'egalite (mult >= 2).
  i64 distinct_levels = 0, tie_batches = 0, max_level = 0;
  {
    std::vector<i64> sorted_levels = subject.levels4;
    std::sort(sorted_levels.begin(), sorted_levels.end());
    for (std::size_t k = 0; k < sorted_levels.size();) {
      std::size_t j = k;
      while (j < sorted_levels.size() && sorted_levels[j] == sorted_levels[k]) ++j;
      ++distinct_levels;
      if (j - k >= 2) ++tie_batches;
      k = j;
    }
    if (!sorted_levels.empty()) max_level = sorted_levels.back();
  }
  std::printf("compteurs  : rondes=%lld visites-noeuds=%lld tests-distance=%lld"
              " high-water=%lld\n", subject.rounds, subject.node_visits,
              subject.distance_tests, subject.stack_high_water);
  std::printf("niveaux    : %lld distincts, %lld lots d'egalite (multiplicite >= 2),"
              " max=%lld\n", distinct_levels, tie_batches, max_level);

  const std::vector<EdgeLvl> subject_edges = to_edge_lvls(subject);

  // -------------------------------------------------------------------------
  // LES ASSERTIONS DE FIXTURES (gravees, verifiees numeriquement hors bande).
  // -------------------------------------------------------------------------
  if (is_fixture) {
    char buf[224];
    if (fixture_name == "tie-lattice") {
      for (const EdgeLvl& e : subject_edges)
        if (e.lvl != 10000) {
          std::snprintf(buf, sizeof buf, "arete (%d,%d) au niveau %lld au lieu de 10000",
                        e.a, e.b, e.lvl);
          return fail("la fixture tie-lattice", buf);
        }
      if (distinct_levels != 1 || tie_batches != 1) {
        std::snprintf(buf, sizeof buf,
                      "%lld niveaux distincts et %lld lots au lieu de 1 et 1",
                      distinct_levels, tie_batches);
        return fail("la fixture tie-lattice", buf);
      }
      // Le lot d'egalite est ATOMIQUE : stricte a 10000 = 16 singletons,
      // fermee a 10000 = UNE composante.
      const int strict_comps = component_count(cut_reps(n, subject_edges, 10000, false));
      const int closed_comps = component_count(cut_reps(n, subject_edges, 10000, true));
      if (strict_comps != 16 || closed_comps != 1) {
        std::snprintf(buf, sizeof buf,
                      "coupes a 10000 : %d composantes strictes et %d fermees au lieu de"
                      " 16 et 1", strict_comps, closed_comps);
        return fail("la fixture tie-lattice", buf);
      }
      // LE JEU CANONIQUE FORCE (derive hors bande de la regle (d^2, min id,
      // max id) seule — independant de la forme de l'arbre) : c'est la porte
      // qui mord le departage par ordre de decouverte.
      static const int kCanonical[15][2] = {
          {0, 1}, {0, 4}, {1, 2}, {1, 5}, {2, 3}, {2, 6}, {3, 7}, {4, 8}, {5, 9},
          {6, 10}, {7, 11}, {8, 12}, {9, 13}, {10, 14}, {11, 15}};
      std::vector<std::array<int, 2>> got = subject.edges;
      std::sort(got.begin(), got.end());
      for (int k = 0; k < 15; ++k)
        if (got[(std::size_t)k][0] != kCanonical[k][0] ||
            got[(std::size_t)k][1] != kCanonical[k][1]) {
          std::snprintf(buf, sizeof buf,
                        "arete canonique %d : (%d,%d) au lieu de (%d,%d)", k,
                        got[(std::size_t)k][0], got[(std::size_t)k][1], kCanonical[k][0],
                        kCanonical[k][1]);
          return fail("le jeu canonique de tie-lattice", buf);
        }
    } else if (fixture_name == "colocated-batch") {
      static const i64 kLevels[5] = {0, 0, 100, 400, 2500};
      std::vector<i64> sorted_levels = subject.levels4;
      std::sort(sorted_levels.begin(), sorted_levels.end());
      for (int k = 0; k < 5; ++k)
        if (sorted_levels[(std::size_t)k] != kLevels[k]) {
          std::snprintf(buf, sizeof buf, "niveau %d : %lld au lieu de %lld", k,
                        sorted_levels[(std::size_t)k], kLevels[k]);
          return fail("la fixture colocated-batch", buf);
        }
      const std::vector<int> strict0 = cut_reps(n, subject_edges, 0, false);
      const std::vector<int> closed0 = cut_reps(n, subject_edges, 0, true);
      if (component_count(strict0) != 6) {
        std::snprintf(buf, sizeof buf, "la coupe stricte a 0 rend %d composantes au lieu"
                      " de 6 singletons", component_count(strict0));
        return fail("la fixture colocated-batch", buf);
      }
      if (component_count(closed0) != 4 || closed0[0] != 0 || closed0[1] != 0 ||
          closed0[2] != 0 || closed0[3] != 3 || closed0[4] != 4 || closed0[5] != 5) {
        std::snprintf(buf, sizeof buf,
                      "la coupe fermee a 0 ne fusionne pas exactement les colocalises"
                      " {0,1,2} (%d composantes)", component_count(closed0));
        return fail("la fixture colocated-batch", buf);
      }
    } else if (fixture_name == "u16-extremes") {
      for (const EdgeLvl& e : subject_edges)
        if (e.lvl != 4294836225LL) {
          std::snprintf(buf, sizeof buf,
                        "arete (%d,%d) au niveau %lld au lieu de 65535^2 = 4294836225",
                        e.a, e.b, e.lvl);
          return fail("la fixture u16-extremes", buf);
        }
    } else if (fixture_name == "two-clusters") {
      // L'ASSERTION DIRECTE : le pont est LA plus proche paire inter-amas,
      // recalculee ici par force brute i128 (amas = ids 0..3 contre 4..7).
      i128 best_d = 0;
      int best_a = -1, best_b = -1;
      bool unique = true;
      for (int a = 0; a < 4; ++a)
        for (int b = 4; b < 8; ++b) {
          const i128 dx = pts[(std::size_t)a].x - pts[(std::size_t)b].x;
          const i128 dy = pts[(std::size_t)a].y - pts[(std::size_t)b].y;
          const i128 dz = pts[(std::size_t)a].z - pts[(std::size_t)b].z;
          const i128 d = dx * dx + dy * dy + dz * dz;
          if (best_a < 0 || d < best_d) {
            best_d = d;
            best_a = a;
            best_b = b;
            unique = true;
          } else if (d == best_d) {
            unique = false;
          }
        }
      if (!unique || best_a != 3 || best_b != 4 || (i64)best_d != 10425898125LL) {
        std::snprintf(buf, sizeof buf,
                      "la plus proche paire inter-amas devrait etre (3,4) a 10425898125,"
                      " unique — force brute : (%d,%d)", best_a, best_b);
        return fail("la fixture two-clusters", buf);
      }
      int bridges = 0;
      bool bridge_exact = false;
      for (const EdgeLvl& e : subject_edges)
        if ((e.a < 4) != (e.b < 4)) {
          ++bridges;
          bridge_exact = (e.a == 3 && e.b == 4 && e.lvl == 10425898125LL);
        }
      if (bridges != 1 || !bridge_exact)
        return fail("la fixture two-clusters",
                    "l'arete pont n'est pas exactement (3,4) au niveau 10425898125");
      const int below = component_count(cut_reps(n, subject_edges, 10425898125LL, false));
      const int at = component_count(cut_reps(n, subject_edges, 10425898125LL, true));
      if (below != 2 || at != 1) {
        std::snprintf(buf, sizeof buf,
                      "coupes au pont : %d composantes strictes et %d fermees au lieu de"
                      " 2 et 1", below, at);
        return fail("la fixture two-clusters", buf);
      }
    }
  }

  // -------------------------------------------------------------------------
  // LE JUGE (--oracle 1) : Prim independant, multiensemble + partitions
  // canoniques a chaque coupe stricte et fermee. Il passe AVANT le replay :
  // une arete manquee (purity-stale) meurt d'abord par le juge.
  // -------------------------------------------------------------------------
  if (oracle == 1) {
    const auto tj0 = std::chrono::steady_clock::now();
    const std::vector<EdgeLvl> judge = judge_prim(pts);
    const auto tj1 = std::chrono::steady_clock::now();
    std::printf("temps juge : %.3f s (Prim O(n^2) independant)\n",
                std::chrono::duration<double>(tj1 - tj0).count());
    const std::string what = compare_single_linkage(n, subject_edges, judge, "sujet", "juge");
    if (!what.empty()) return fail("le juge Prim", what.c_str());
  }

  // -------------------------------------------------------------------------
  // LE REPLAY CANONIQUE (n <= 4096) : le meme nuage, d'autres tailles de
  // feuilles — la sortie exacte (sequences d'aretes ET de niveaux) doit etre
  // IDENTIQUE : le canonique ne depend pas de la forme de l'arbre.
  // -------------------------------------------------------------------------
  if (n <= 4096) {
    const int alts[2] = {leaf_size == 2 ? 5 : 2, leaf_size == 8 ? 11 : 8};
    for (const int alt : alts) {
      mhgp3v::MortonLbvh tree_alt;
      tree_alt.build(pts, alt);
      const mhgp3v::EmstResult replay = mhgp3v::emst_boruvka(tree_alt, injections);
      if (!replay.ok) return fail("le replay canonique", replay.refusal);
      if (replay.edges != subject.edges || replay.levels4 != subject.levels4) {
        char buf[128];
        std::snprintf(buf, sizeof buf,
                      "sortie divergente entre feuilles %d et %d — le departage n'est pas"
                      " canonique", leaf_size, alt);
        return fail("le replay canonique", buf);
      }
    }
  }

  // -------------------------------------------------------------------------
  // L'EQUIVARIANCE (--permute 1) : nuage renverse + melange LCG grave —
  // memes multiensembles de niveaux, memes partitions apres renommage. Les
  // ARETES peuvent differer legitimement sous ex aequo : elles ne sont pas
  // comparees ici.
  // -------------------------------------------------------------------------
  if (permute == 1) {
    std::vector<std::vector<int>> sigmas;
    {
      std::vector<int> reversed_sigma((std::size_t)n);
      for (int i = 0; i < n; ++i) reversed_sigma[(std::size_t)i] = n - 1 - i;
      sigmas.push_back(std::move(reversed_sigma));
      std::vector<int> shuffled((std::size_t)n);
      for (int i = 0; i < n; ++i) shuffled[(std::size_t)i] = i;
      unsigned long long lcg = 0x9E3779B97F4A7C15ULL ^ (unsigned long long)seed;
      for (int i = n - 1; i > 0; --i) {
        lcg = lcg * 6364136223846793005ULL + 1442695040888963407ULL;
        const int j = (int)(lcg % (unsigned long long)(i + 1));
        std::swap(shuffled[(std::size_t)i], shuffled[(std::size_t)j]);
      }
      sigmas.push_back(std::move(shuffled));
    }
    for (const std::vector<int>& sigma : sigmas) {
      std::vector<mhgp::P3> permuted((std::size_t)n);
      for (int i = 0; i < n; ++i)
        permuted[(std::size_t)i] = pts[(std::size_t)sigma[(std::size_t)i]];
      mhgp3v::MortonLbvh tree2;
      tree2.build(permuted, leaf_size);
      const mhgp3v::EmstResult run2 = mhgp3v::emst_boruvka(tree2, injections);
      if (!run2.ok) return fail("l'equivariance", run2.refusal);
      const std::string span = check_spanning(n, run2);
      if (!span.empty()) return fail("l'equivariance (arbre couvrant permute)", span.c_str());
      // Renommage vers les PointId ORIGINAUX : le point permute j est
      // l'original sigma[j].
      std::vector<EdgeLvl> renamed;
      renamed.reserve(run2.edges.size());
      for (std::size_t k = 0; k < run2.edges.size(); ++k) {
        const int a = sigma[(std::size_t)run2.edges[k][0]];
        const int b = sigma[(std::size_t)run2.edges[k][1]];
        renamed.push_back(EdgeLvl{run2.levels4[k], a < b ? a : b, a < b ? b : a});
      }
      const std::string what =
          compare_single_linkage(n, subject_edges, renamed, "sujet", "sujet permute");
      if (!what.empty()) return fail("l'equivariance", what.c_str());
    }
  }

  // -------------------------------------------------------------------------
  // LES PLANCHERS (code 3) — jamais evalues sous injection : un mutant ne
  // meurt que par les portes de verite, pas par un plancher.
  // -------------------------------------------------------------------------
  if (!injections.any()) {
    if (subject.rounds < min_rounds) {
      std::printf("ECHEC : plancher viole — %lld rondes < %lld exigees\n", subject.rounds,
                  min_rounds);
      return 3;
    }
    if (distinct_levels < min_distinct_levels) {
      std::printf("ECHEC : plancher viole — %lld niveaux distincts < %lld exiges\n",
                  distinct_levels, min_distinct_levels);
      return 3;
    }
    if (tie_batches < min_tie_batches) {
      std::printf("ECHEC : plancher viole — %lld lots d'egalite < %lld exiges\n",
                  tie_batches, min_tie_batches);
      return 3;
    }
  }

  if (injections.any()) {
    std::printf("MUTANT SURVIVANT : l'injection n'a pas ete detectee\n");
    return 0;
  }
  std::printf("OK : lane k=1 — EMST Boruvka exact sur LBVH, %d points, %zu aretes,"
              " juge=%s, equivariance=%s\n", n, subject.edges.size(),
              oracle == 1 ? "oui" : "non", permute == 1 ? "oui" : "non");
  return 0;
}
