// MorseHGP3D v3 — LA LANE k=1 DU CONTRAT hgp_reduced_normalized_h0_v3 :
// L'EMST EXACT PAR BORUVKA SUR LE LBVH PARTAGE (PROPOSITION §6.1 : « l'ordre
// un est exactement le single linkage, au niveau distance_squared/4 »).
//
// Le coeur ne publie AUCUN flottant : chaque arete porte levels4 = 4*niveau
// = d^2 EXACT en i64 (profil u16 : d^2 <= 3*65535^2 < 3*2^32 — l'i64 est
// large, aucun debordement possible). Le niveau rationnel du single linkage
// est levels4/4, jamais materialise.
//
// LA RONDE DE BORUVKA : pour chaque composante DSU, la plus courte arete
// SORTANTE par requetes best-first sur le MortonLbvh partage
// (prototype/morton_lbvh.hpp — le coeur le CONSOMME, il ne reecrit ni la
// cle Morton ni l'arbre), avec :
//   (a) l'elagage par la meilleure distance COURANTE de la composante,
//       STRICT (bd > best seulement) : un noeud qui peut encore contenir un
//       candidat ex aequo n'est JAMAIS elague — le canonique reste
//       decidable ;
//   (b) le saut des sous-arbres PURS de la meme composante — purete par
//       noeud recalculee A CHAQUE RONDE en O(noeuds), bottom-up : la
//       construction du LBVH pousse le pere avant ses fils, donc tout fils
//       porte un indice STRICTEMENT superieur et une passe descendante sur
//       les indices decroissants visite fils avant pere ;
//   (c) le departage DETERMINISTE des ex aequo par la paire canonique :
//       (d^2, min id, max id) lexicographique minimal — la sortie est
//       rejouable, INDEPENDANTE de la forme de l'arbre et de l'ordre de
//       parcours (le probe le grave par un replay a plusieurs tailles de
//       feuilles).
// DSU : union par taille + compression de chemin. Les fusions se font en
// FIN de ronde (lots dedupliques par l'etat du DSU), jamais pendant les
// requetes ; une ronde sterile est un refus ferme, jamais une boucle
// infinie.
//
// AUCUNE E/S ICI, AUCUN main : le coeur est consomme par
// emst_boruvka_probe.cpp et, ensuite, par la lane k=1 du pipeline. Les
// injections vivent ici parce qu'elles mutent l'algorithme lui-meme ; le
// sujet nominal est EmstInjections{} (tout faux).
#pragma once

#include <algorithm>
#include <array>
#include <vector>

#include "prototype/morton_lbvh.hpp"

namespace mhgp3v {

// LES MUTANTS DE LA LANE k=1 (portes --inject du probe, code 4 exige) :
//   - reconnect_wrong_components : a la premiere ronde, les deux premieres
//     entrees de fusion aux extremites disjointes ECHANGENT leurs cibles —
//     les niveaux publies sont conserves (le multiensemble tient), mais les
//     composantes reliees sont fausses : la partition du juge diverge.
//   - level_off_by_one : chaque arete publie d^2+1 — le multiensemble des
//     niveaux diverge du juge.
//   - skip_last_round : Boruvka s'arrete une ronde trop tot (les aretes de
//     la derniere ronde sont retranchees) — moins de n-1 aretes, l'assertion
//     d'arbre couvrant du probe le tue.
//   - purity_stale : apres la premiere ronde, un noeud interne HERITE du
//     verdict de purete de son fils GAUCHE sans re-examiner le droit — la
//     table n'est pas correctement recalculee apres les fusions : des
//     sous-arbres ETRANGERS sont sautes, une arete sortante minimale peut
//     etre manquee (arete plus longue, voire ronde sterile). NOTE DE
//     CONCEPTION : la staleness temporelle litterale (reutiliser la table de
//     la ronde precedente) est PROUVABLEMENT fail-slow-mais-correcte dans ce
//     design a fusions de fin de ronde — la purete ne peut que croitre (un
//     noeud pur de C reste pur de find(C)) ; le mutant realise donc l'EFFET
//     decrit (sous-arbre etranger saute) par la voie qui mord vraiment.
//   - tie_nondeterministic : le departage des ex aequo garde le PREMIER
//     candidat decouvert (ordre de parcours) au lieu de la paire canonique —
//     la sortie depend de la forme de l'arbre ; la fixture tie-lattice du
//     probe grave le jeu d'aretes canonique force et le replay multi-feuilles
//     compare les sorties exactes.
struct EmstInjections {
  bool reconnect_wrong_components = false;
  bool level_off_by_one = false;
  bool skip_last_round = false;
  bool purity_stale = false;
  bool tie_nondeterministic = false;
  bool any() const {
    return reconnect_wrong_components || level_off_by_one || skip_last_round ||
           purity_stale || tie_nondeterministic;
  }
};

struct EmstResult {
  std::vector<std::array<int, 2>> edges;  // n-1 aretes, id min d'abord
  std::vector<long long> levels4;         // 4*niveau = d^2 exact, parallele a edges
  // LES COMPTEURS DE FAISABILITE (mesure 50k du probe, mono-thread) :
  long long rounds = 0;            // rondes de Boruvka
  long long node_visits = 0;       // noeuds LBVH poppes (toutes requetes)
  long long distance_tests = 0;    // paires evaluees en d^2 dans les feuilles
  long long stack_high_water = 0;  // profondeur maximale de la pile best-first
  bool ok = false;
  const char* refusal = nullptr;
};

// Le DSU DU SUJET : union par taille + compression de chemin (le juge du
// probe reecrit le sien — representation volontairement differente).
struct EmstDsu {
  std::vector<int> parent;
  std::vector<int> size_;
  void init(int n) {
    parent.resize((std::size_t)n);
    size_.assign((std::size_t)n, 1);
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
  bool unite(int a, int b) {
    int ra = find(a), rb = find(b);
    if (ra == rb) return false;
    if (size_[(std::size_t)ra] < size_[(std::size_t)rb]) std::swap(ra, rb);
    parent[(std::size_t)rb] = ra;
    size_[(std::size_t)ra] += size_[(std::size_t)rb];
    return true;
  }
};

// d^2 exact en i64 — u16 : |delta| <= 65535, somme < 3*2^32 << 2^62.
inline long long emst_dist2(const mhgp::P3& a, const mhgp::P3& b) {
  const long long dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
  return dx * dx + dy * dy + dz * dz;
}

// Le minorant EXACT de d^2 entre un point et la boite AABB u16 d'un noeud.
inline long long emst_box_d2(const LbvhNode& node, const mhgp::P3& p) {
  const long long c[3] = {p.x, p.y, p.z};
  long long s = 0;
  for (int d = 0; d < 3; ++d) {
    long long delta = 0;
    if (c[d] < node.lo[d]) delta = node.lo[d] - c[d];
    else if (c[d] > node.hi[d]) delta = c[d] - node.hi[d];
    s += delta * delta;
  }
  return s;
}

inline EmstResult emst_boruvka(const MortonLbvh& tree,
                               const EmstInjections& inject = EmstInjections{}) {
  EmstResult res;
  if (tree.points == nullptr || tree.order.empty() || tree.nodes.empty()) {
    res.refusal = "arbre vide ou points absents";
    return res;
  }
  const std::vector<mhgp::P3>& pts = *tree.points;
  const int n = (int)tree.order.size();
  if ((std::size_t)n != pts.size()) {
    res.refusal = "l'ordre du LBVH ne couvre pas le nuage";
    return res;
  }
  if (n == 1) {
    res.ok = true;
    res.rounds = 0;
    return res;
  }

  EmstDsu dsu;
  dsu.init(n);
  int components = n;
  std::vector<int> comp((std::size_t)n);
  std::vector<int> node_comp(tree.nodes.size(), -1);  // -1 = impur

  // La meilleure arete sortante COURANTE de chaque composante, indexee par
  // racine DSU. d2 < 0 = vide ; (a, b) toujours (min id, max id).
  struct Cand {
    long long d2 = -1;
    int a = -1, b = -1;
  };
  std::vector<Cand> best((std::size_t)n);
  struct Frame {
    int node;
    long long bd;
  };
  std::vector<Frame> stack;
  stack.reserve(128);

  res.edges.reserve((std::size_t)(n - 1));
  res.levels4.reserve((std::size_t)(n - 1));
  std::size_t last_round_begin = 0;

  while (components > 1) {
    last_round_begin = res.edges.size();

    // 1. LES ETIQUETTES DE COMPOSANTE de la ronde (racines DSU courantes).
    for (int i = 0; i < n; ++i) comp[(std::size_t)i] = dsu.find(i);

    // 2. LA PURETE PAR NOEUD, recalculee A CHAQUE RONDE en O(noeuds) :
    // fils avant pere (indices decroissants). Feuille : un seul comp sur la
    // plage. Interne : pur ssi les deux fils sont purs du MEME comp.
    for (int v = (int)tree.nodes.size() - 1; v >= 0; --v) {
      const LbvhNode& node = tree.nodes[(std::size_t)v];
      if (node.left < 0) {
        int c = comp[(std::size_t)tree.order[(std::size_t)node.begin]];
        for (int t = node.begin + 1; t < node.end; ++t)
          if (comp[(std::size_t)tree.order[(std::size_t)t]] != c) {
            c = -1;
            break;
          }
        node_comp[(std::size_t)v] = c;
      } else {
        const int lc = node_comp[(std::size_t)node.left];
        const int rc = node_comp[(std::size_t)node.right];
        if (inject.purity_stale && res.rounds > 0 && lc >= 0)
          node_comp[(std::size_t)v] = lc;  // MUTANT : le droit n'est pas re-examine
        else
          node_comp[(std::size_t)v] = (lc >= 0 && lc == rc) ? lc : -1;
      }
    }

    // 3. LES REQUETES BEST-FIRST : pour chaque point, la plus courte arete
    // sortante de SA composante ; l'elagage partage la meilleure candidate
    // courante de toute la composante (elle se resserre au fil des points).
    for (int i = 0; i < n; ++i) best[(std::size_t)i].d2 = -1;
    for (int i = 0; i < n; ++i) {
      const mhgp::P3& p = pts[(std::size_t)i];
      const int root = comp[(std::size_t)i];
      Cand& b = best[(std::size_t)root];
      stack.clear();
      stack.push_back(Frame{0, emst_box_d2(tree.nodes[0], p)});
      while (!stack.empty()) {
        if ((long long)stack.size() > res.stack_high_water)
          res.stack_high_water = (long long)stack.size();
        const Frame f = stack.back();
        stack.pop_back();
        ++res.node_visits;
        if (node_comp[(std::size_t)f.node] == root) continue;  // (b) sous-arbre PUR
        // (a) elagage STRICT : bd == best reste examine (ex aequo canonique).
        if (b.d2 >= 0 && f.bd > b.d2) continue;
        const LbvhNode& node = tree.nodes[(std::size_t)f.node];
        if (node.left < 0) {
          for (int t = node.begin; t < node.end; ++t) {
            const int j = tree.order[(std::size_t)t];
            if (comp[(std::size_t)j] == root) continue;
            ++res.distance_tests;
            const long long dd = emst_dist2(p, pts[(std::size_t)j]);
            const int lo = i < j ? i : j;
            const int hi = i < j ? j : i;
            if (b.d2 < 0 || dd < b.d2 ||
                (dd == b.d2 && !inject.tie_nondeterministic &&
                 (lo < b.a || (lo == b.a && hi < b.b)))) {
              // (c) canonique — le MUTANT garde le premier decouvert.
              b.d2 = dd;
              b.a = lo;
              b.b = hi;
            }
          }
        } else {
          const long long bl = emst_box_d2(tree.nodes[(std::size_t)node.left], p);
          const long long br = emst_box_d2(tree.nodes[(std::size_t)node.right], p);
          // Le fils le plus proche AU SOMMET de la pile (poppe en premier).
          if (bl <= br) {
            stack.push_back(Frame{node.right, br});
            stack.push_back(Frame{node.left, bl});
          } else {
            stack.push_back(Frame{node.left, bl});
            stack.push_back(Frame{node.right, br});
          }
        }
      }
    }

    // 4. LES FUSIONS DE FIN DE RONDE, racines croissantes : deterministe et
    // dedupliquee par l'etat du DSU (un lot mutuel ne publie qu'une arete).
    struct MergeEntry {
      long long d2;
      int a, b;
    };
    std::vector<MergeEntry> entries;
    entries.reserve((std::size_t)components);
    for (int r = 0; r < n; ++r)
      if (comp[(std::size_t)r] == r && best[(std::size_t)r].d2 >= 0)
        entries.push_back(
            MergeEntry{best[(std::size_t)r].d2, best[(std::size_t)r].a, best[(std::size_t)r].b});
    if (inject.reconnect_wrong_components && res.rounds == 0 && entries.size() >= 2) {
      // MUTANT : les deux premieres entrees aux quatre extremites disjointes
      // echangent leurs cibles — memes niveaux publies, mauvaises composantes.
      for (std::size_t x = 0; x + 1 < entries.size(); ++x) {
        bool crossed = false;
        for (std::size_t y = x + 1; y < entries.size(); ++y) {
          if (entries[x].a == entries[y].a || entries[x].a == entries[y].b ||
              entries[x].b == entries[y].a || entries[x].b == entries[y].b)
            continue;
          std::swap(entries[x].b, entries[y].b);
          crossed = true;
          break;
        }
        if (crossed) break;
      }
    }
    bool merged_any = false;
    for (const MergeEntry& e : entries) {
      if (!dsu.unite(e.a, e.b)) continue;
      const int lo = e.a < e.b ? e.a : e.b;
      const int hi = e.a < e.b ? e.b : e.a;
      res.edges.push_back({lo, hi});
      res.levels4.push_back(e.d2 + (inject.level_off_by_one ? 1 : 0));
      --components;
      merged_any = true;
    }
    ++res.rounds;
    if (!merged_any) {
      // JAMAIS DE BOUCLE INFINIE : une ronde sans fusion est un refus ferme
      // (elle ne peut survenir que sous mutant ou defaut du coeur).
      res.refusal = "ronde sterile : aucune fusion realisee";
      return res;
    }
  }

  if (inject.skip_last_round && res.rounds > 0) {
    // MUTANT : les aretes de la DERNIERE ronde sont retranchees — l'arbre ne
    // couvre plus, l'assertion du probe doit le tuer.
    res.edges.resize(last_round_begin);
    res.levels4.resize(last_round_begin);
  }

  res.ok = true;
  return res;
}

}  // namespace mhgp3v
