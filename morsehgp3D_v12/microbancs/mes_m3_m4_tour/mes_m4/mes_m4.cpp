// mhgp12_mes_m4 : microbanc MES-M4 (LEV-FOREST-D-F1), hors produit, sans dependance a la v11.
// Foret d'un ordre SANS LOTS, a partir des naissances, des cellules et des graines videes par mhgp12_vidage :
//   1. numerotation canonique des naissances : (rang, centre exact) ; a l'ordre 1, les sites par (x, y, z) ;
//   2. noyau (D-F1, CONCEPTION_TOUR § 4.1 ; port de preuves_tour/noyau_v11.cpp) : union-find par TAILLE sur des
//      evenements binaires (rang, deux operandes = sommets courants, plus petite feuille, survivant), cellules par rang
//      croissant ; attache de chaque perdant, sommet par jonction ; aucune allocation dans la boucle ;
//   3. contraction des plateaux (LEM-T4 = lemme P de MATHEMATIQUES § 10.3) : par groupe de rang, classes d'evenements
//      lies (l'un a pour operande le sommet produit par l'autre) = multifusions N-aires ; numerotation des fusions par
//      (rang, plus petite naissance) ; parents ; enfants en CSR tries ;
//   4. juge : IDENTITE avec la foret publiee par la v11 (noeuds, parents, rangs, cles de naissance, enfants, racine) ;
//      plus, sur les ordres consecutifs, l'image de chaque naissance par LEM-T6 contre les verticales de la v11.
// Modes :
//   mhgp12_mes_m4 --porte [cas]                  hypergraphes aleatoires a rangs repetes contre un Kruskal par lots
//                                                (semantique v10), temoins de plateau ; code 0 conforme, 1 ecart
//   mhgp12_mes_m4 <dossier> [--repetitions R] [--fils-contraction T]
//                                                banc sur un vidage ; JSON sur la sortie standard ; T > 1 : contraction
//                                                parallele par tranches alignees sur les rangs, identite exigee
// Mutant : MHGP12_MUTANT_SANS_CONTRACTION (chaque evenement binaire devient un noeud) ; la porte et le banc doivent le
// tuer.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <barrier>
#include <functional>
#include <thread>

#include "../common/format.hpp"

namespace mhgp12 {
namespace {
namespace d = ::mhgp12::dump;
using d::u32;
using d::u64;
using d::u8;
using Clock = std::chrono::steady_clock;
constexpr u32 kNone = d::kNone;
constexpr u32 kEv = 0x80000000u;  // operande : bit de poids fort = evenement, sinon feuille (naissance)
// Domaine exact du codage des operandes (constat CST-0212 de l'auditeur) : le bit 31 porte le genre, il reste 31 bits
// utiles. Une feuille d'indice 2^31 se confondrait avec l'evenement 0. D'ou, par ordre : au plus 2^31 - 1 naissances
// (feuilles 0..2^31-2), donc au plus 2^31 - 2 evenements (une union de moins que de naissances) et au plus
// 2 (2^31 - 1) - 1 = 2^32 - 3 noeuds, sous la sentinelle kNone = 2^32 - 1. Au-dela : refus explicite, decide sur le
// seul nombre de naissances, AVANT toute allocation (porte a la borne dans --porte). Choix retenu plutot qu'un genre
// separe ou des indices sur 64 bits : evenement de 20 octets inchange, aucun cout mesurable ; la v11 refuse deja
// 2^31 naissances et plus (forest_capacities, tower_capacity). Donnees : au plus 979 350 naissances par ordre.
constexpr u64 kMaxBirths = (u64{1} << 31) - 1;
constexpr bool operand_domain(u64 births) { return births <= kMaxBirths; }
static_assert(operand_domain(kMaxBirths) && !operand_domain(kMaxBirths + 1), "CST-0212 : borne a 2^31 - 1");
static_assert(2 * kMaxBirths - 1 < u64{kNone}, "CST-0212 : identifiants de noeuds sous la sentinelle");

#ifdef MHGP12_MUTANT_SANS_CONTRACTION
constexpr bool kMutantSansContraction = true;
#else
constexpr bool kMutantSansContraction = false;
#endif

double seconds(Clock::time_point t) { return std::chrono::duration<double>(Clock::now() - t).count(); }

// ---- Entree d'un ordre ----------------------------------------------------------------------------------------------
struct OrderInput {
  u32 nb = 0;                    // naissances, numerotees canoniquement (feuilles 0..nb-1)
  std::vector<u32> leaf_rank;    // rang de chaque naissance (ordre canonique)
  std::vector<u32> jrank;        // rang de chaque jonction (cellule), croissant
  std::vector<u64> joff;         // CSR des representants par jonction
  std::vector<u32> rep;          // feuille (naissance canonique) de chaque representant
};

// ---- Noyau D-F1 -----------------------------------------------------------------------------------------------------
struct Cell {
  u32 up, size, last, minleaf;  // 16 octets
};
struct Event {
  u32 rank, a, b, minleaf, surv;  // 20 octets
};
static_assert(sizeof(Event) == 20, "evenement de 20 octets");

struct Kernel {
  std::vector<Cell> cell;
  std::vector<Event> ev;
  std::vector<u32> att_parent, att_rank, jtop;
  u64 find_steps = 0, finds = 0;
};

// Rend false hors du domaine des operandes (aucune allocation) ou si un evenement deborderait (impossible dans le
// domaine : au plus nb - 1 unions).
bool kernel(const OrderInput& in, Kernel& k) {
  if (!operand_domain(in.nb)) return false;
  const u32 nb = in.nb;
  const u64 nj = in.jrank.size();
  k.cell.resize(nb);
  for (u32 i = 0; i < nb; ++i) k.cell[i] = Cell{i, 1, kNone, i};
  k.ev.clear();
  k.ev.reserve(nb ? nb - 1 : 0);
  k.att_parent.assign(nb, kNone);
  k.att_rank.assign(nb, 0);
  k.jtop.resize(nj);
  Cell* c = k.cell.data();
  auto find = [c](u32 x) {
    while (c[x].up != x) {
      c[x].up = c[c[x].up].up;  // demi-compression
      x = c[x].up;
    }
    return x;
  };
  const u32* rep = in.rep.data();
  const u64 nr = in.rep.size();
  for (u64 t = 0; t < nj; ++t) {
    const u64 b0 = in.joff[t], e0 = in.joff[t + 1];
    for (u64 r = b0 + 16; r < e0 + 16 && r < nr; ++r) __builtin_prefetch(&c[rep[r]]);
    const u32 rk = in.jrank[t];
    u32 x = find(rep[b0]);
    for (u64 r = b0 + 1; r < e0; ++r) {
      const u32 y = find(rep[r]);
      if (x == y) continue;
      const u32 e = static_cast<u32>(k.ev.size());
      if (e >= kEv) return false;
      const u32 a = c[x].last == kNone ? x : (c[x].last | kEv), b = c[y].last == kNone ? y : (c[y].last | kEv);
      const u32 ml = std::min(c[x].minleaf, c[y].minleaf);
      u32 s = x, l = y;
      if (c[x].size < c[y].size) s = y, l = x;  // union par TAILLE
      k.ev.push_back(Event{rk, a, b, ml, s});
      c[l].up = s;
      c[s].size += c[l].size;
      c[s].minleaf = ml;
      c[s].last = e;
      k.att_parent[l] = s;
      k.att_rank[l] = rk;
      x = s;
    }
    k.jtop[t] = c[x].last == kNone ? x : (c[x].last | kEv);
  }
  return true;
}

// ---- Contraction des plateaux (LEM-T4) et numerotation canonique -----------------------------------------------------
struct Forest {
  u32 nb = 0;
  std::vector<u32> rank, parent;   // par noeud ; feuilles 0..nb-1 puis fusions
  std::vector<u64> child_off;      // CSR des enfants, dans l'ordre des noeuds
  std::vector<u32> children;
  std::vector<u32> nid;            // evenement -> noeud
  u64 groups = 0, groups_multi = 0, max_group = 0;
};

void contract(const OrderInput& in, const Kernel& k, Forest& f) {
  const u32 nb = in.nb;
  const u32 ne = static_cast<u32>(k.ev.size());
  f.nb = nb;
  f.nid.assign(ne, kNone);
  f.rank.assign(in.leaf_rank.begin(), in.leaf_rank.end());
  f.groups = f.groups_multi = f.max_group = 0;
  std::vector<u32> loc;           // union-find local au groupe de rang
  std::vector<std::array<u32, 2>> keys;  // (plus petite feuille, racine locale) des classes du groupe
  std::vector<u32> class_min;
  u32 next = nb;
  for (u32 g0 = 0; g0 < ne;) {
    u32 g1 = g0 + 1;
    while (g1 < ne && k.ev[g1].rank == k.ev[g0].rank) ++g1;
    const u32 n = g1 - g0;
    ++f.groups;
    f.groups_multi += n >= 2;
    f.max_group = std::max<u64>(f.max_group, n);
    if (kMutantSansContraction || n == 1) {
      // Un evenement seul est une classe ; le mutant binarise toutes les multifusions (un noeud par evenement).
      std::vector<std::array<u32, 2>> single;
      if (n == 1) {
        f.nid[g0] = next++;
        f.rank.push_back(k.ev[g0].rank);
      } else {
        for (u32 e = g0; e < g1; ++e) single.push_back({k.ev[e].minleaf, e});
        std::sort(single.begin(), single.end());
        for (const auto& s : single) {
          f.nid[s[1]] = next++;
          f.rank.push_back(k.ev[s[1]].rank);
        }
      }
      g0 = g1;
      continue;
    }
    loc.resize(n);
    std::iota(loc.begin(), loc.end(), 0u);
    auto find = [&loc](u32 x) {
      while (loc[x] != x) x = loc[x] = loc[loc[x]];
      return x;
    };
    for (u32 e = g0; e < g1; ++e)
      for (u32 op : {k.ev[e].a, k.ev[e].b})
        if ((op & kEv) && (op & ~kEv) >= g0) {  // operande produit dans ce groupe de rang : evenements lies
          const u32 x = find((op & ~kEv) - g0), y = find(e - g0);
          if (x != y) loc[x] = y;
        }
    class_min.assign(n, kNone);
    for (u32 e = g0; e < g1; ++e) {
      const u32 r = find(e - g0);
      class_min[r] = std::min(class_min[r], k.ev[e].minleaf);
    }
    keys.clear();
    for (u32 i = 0; i < n; ++i)
      if (find(i) == i) keys.push_back({class_min[i], i});
    std::sort(keys.begin(), keys.end());  // (rang fixe, plus petite naissance) : cle unique (sous-arbres disjoints)
    for (const auto& key : keys) {
      class_min[key[1]] = next++;  // reemploi : racine locale -> noeud
      f.rank.push_back(k.ev[g0].rank);
    }
    for (u32 e = g0; e < g1; ++e) f.nid[e] = class_min[find(e - g0)];
    g0 = g1;
  }
  const u32 nn = next;
  f.parent.assign(nn, kNone);
  std::vector<u32> count(nn + 1, 0);
  for (u32 e = 0; e < ne; ++e)
    for (u32 op : {k.ev[e].a, k.ev[e].b}) {
      u32 child;
      if (!(op & kEv)) child = op;
      else if (kMutantSansContraction || k.ev[op & ~kEv].rank != k.ev[e].rank) child = f.nid[op & ~kEv];
      else continue;  // operande interne au plateau
      if (kMutantSansContraction && (op & kEv) && f.nid[op & ~kEv] == f.nid[e]) continue;
      f.parent[child] = f.nid[e];
      ++count[f.nid[e] + 1];
    }
  f.child_off.assign(nn + 1, 0);
  for (u32 v = 0; v < nn; ++v) f.child_off[v + 1] = f.child_off[v] + count[v + 1];
  f.children.assign(f.child_off[nn], kNone);
  std::vector<u64> fill(f.child_off.begin(), f.child_off.end() - 1);
  for (u32 v = 0; v < nn; ++v)
    if (f.parent[v] != kNone) f.children[fill[f.parent[v]]++] = v;  // v croissant : listes deja triees
}

// ---- Contraction parallele (M de ARCHITECTURE § 4.3) -----------------------------------------------------------------
// Equipe persistante de T fils (le pilote est le fil 0) ; les phases sont separees par une barriere. Tranches
// d'evenements alignees sur les changements de rang (une tranche ne coupe jamais un groupe de rang) : les classes, les
// noeuds qu'elles creent, leurs parents et leurs enfants sont ecrits par la seule tranche qui les contient (ecritures
// disjointes, aucun atomique) ; deux sommes prefixes sur les tranches (noeuds, puis enfants) par le fil 0. Sortie
// identique a contract() (controle dans le banc).
class Team {
 public:
  explicit Team(u32 threads) : size_(threads), start_(threads), sync_(threads) {
    for (u32 w = 1; w < threads; ++w)
      workers_.emplace_back([this, w]() {
        for (;;) {
          start_.arrive_and_wait();
          if (quit_) return;
          job_(w);
        }
      });
  }
  ~Team() {
    quit_ = true;
    start_.arrive_and_wait();
    for (auto& t : workers_) t.join();
  }
  Team(const Team&) = delete;
  Team& operator=(const Team&) = delete;
  u32 size() const { return size_; }
  void sync() { sync_.arrive_and_wait(); }
  void run(const std::function<void(u32)>& job) {
    job_ = job;
    start_.arrive_and_wait();
    job_(0);
  }

 private:
  u32 size_;
  std::barrier<> start_, sync_;
  std::vector<std::thread> workers_;
  std::function<void(u32)> job_;
  bool quit_ = false;
};

struct ParallelScratch {
  std::vector<u32> slice_lo, slice_hi;     // tranches d'evenements
  std::vector<u32> slice_classes, slice_node0, slice_children;
  std::vector<u64> slice_child0;
  std::vector<std::vector<u32>> loc, class_min;
  std::vector<std::vector<std::array<u32, 2>>> keys;
  std::vector<u32> child_count;
};

void contract_parallel(const OrderInput& in, const Kernel& k, Forest& f, Team& team, ParallelScratch& w) {
  const u32 nb = in.nb;
  const u32 ne = static_cast<u32>(k.ev.size());
  const u32 T = team.size();
  f.nb = nb;
  f.nid.assign(ne, kNone);
  w.slice_lo.assign(T, 0);
  w.slice_hi.assign(T, 0);
  for (u32 t = 0; t < T; ++t) {  // debuts alignes sur un changement de rang
    u32 b = static_cast<u32>(u64{ne} * t / T);
    while (b > 0 && b < ne && k.ev[b].rank == k.ev[b - 1].rank) ++b;
    w.slice_lo[t] = b;
  }
  for (u32 t = 0; t < T; ++t) w.slice_hi[t] = t + 1 < T ? w.slice_lo[t + 1] : ne;
  w.slice_classes.assign(T, 0);
  w.slice_node0.assign(T, 0);
  w.slice_children.assign(T, 0);
  w.slice_child0.assign(T, 0);
  w.loc.resize(T);
  w.class_min.resize(T);
  w.keys.resize(T);
  u32 total_nodes = 0;
  team.run([&](u32 t) {
    const u32 lo = w.slice_lo[t], hi = std::max(w.slice_lo[t], w.slice_hi[t]);
    auto& loc = w.loc[t];
    auto& cmin = w.class_min[t];
    auto& keys = w.keys[t];
    loc.resize(hi - lo);
    std::iota(loc.begin(), loc.end(), 0u);
    auto find = [&loc](u32 x) {
      while (loc[x] != x) x = loc[x] = loc[loc[x]];
      return x;
    };
    // Phase 1 : classes d'evenements lies, cles (rang, plus petite naissance) par groupe de rang.
    for (u32 e = lo; e < hi; ++e)
      for (u32 op : {k.ev[e].a, k.ev[e].b})
        if ((op & kEv) && k.ev[op & ~kEv].rank == k.ev[e].rank) {
          const u32 x = find((op & ~kEv) - lo), y = find(e - lo);
          if (x != y) loc[x] = y;
        }
    cmin.assign(hi - lo, kNone);
    for (u32 e = lo; e < hi; ++e) {
      const u32 r = find(e - lo);
      cmin[r] = std::min(cmin[r], k.ev[e].minleaf);
    }
    keys.clear();
    for (u32 g0 = lo; g0 < hi;) {  // classes d'un groupe de rang, triees par plus petite naissance
      u32 g1 = g0 + 1;
      while (g1 < hi && k.ev[g1].rank == k.ev[g0].rank) ++g1;
      const std::size_t first = keys.size();
      for (u32 e = g0; e < g1; ++e)
        if (find(e - lo) == e - lo) keys.push_back({cmin[e - lo], e - lo});
      std::sort(keys.begin() + first, keys.end());
      g0 = g1;
    }
    w.slice_classes[t] = static_cast<u32>(keys.size());
    team.sync();
    if (t == 0) {
      u32 next = nb;
      for (u32 s = 0; s < T; ++s) {
        w.slice_node0[s] = next;
        next += w.slice_classes[s];
      }
      total_nodes = next;
      f.rank.assign(next, 0);
      f.parent.assign(next, kNone);
      w.child_count.assign(next, 0);
      for (u32 v = 0; v < nb; ++v) f.rank[v] = in.leaf_rank[v];
    }
    team.sync();
    // Phase 2a : identifiants et rangs des noeuds de la tranche, noeud de chaque evenement.
    u32 node = w.slice_node0[t];
    for (const auto& key : keys) {
      cmin[key[1]] = node;
      f.rank[node] = k.ev[lo + key[1]].rank;
      ++node;
    }
    for (u32 e = lo; e < hi; ++e) f.nid[e] = cmin[find(e - lo)];
    team.sync();  // tous les nid sont ecrits : un operande d'une tranche anterieure est lisible
    // Phase 2b : parents (un seul ecrivain par enfant) et nombres d'enfants (noeuds de la tranche seulement).
    u32 children = 0;
    for (u32 e = lo; e < hi; ++e) {
      const u32 id = f.nid[e];
      for (u32 op : {k.ev[e].a, k.ev[e].b}) {
        if (!(op & kEv)) {
          f.parent[op] = id;
        } else if (k.ev[op & ~kEv].rank != k.ev[e].rank) {
          f.parent[f.nid[op & ~kEv]] = id;
        } else {
          continue;
        }
        ++w.child_count[id];
        ++children;
      }
    }
    w.slice_children[t] = children;
    team.sync();
    if (t == 0) {
      u64 at = 0;
      for (u32 s = 0; s < T; ++s) {
        w.slice_child0[s] = at;
        at += w.slice_children[s];
      }
      f.child_off.assign(u64{total_nodes} + 1, 0);
      f.children.assign(at, kNone);
      f.child_off[total_nodes] = at;
    }
    team.sync();
    // Phase 3 : decalages et enfants des noeuds de la tranche, tries.
    u64 at = w.slice_child0[t];
    const u32 n0 = w.slice_node0[t], n1 = n0 + w.slice_classes[t];
    for (u32 v = n0; v < n1; ++v) {
      f.child_off[v] = at;
      at += w.child_count[v];
    }
    if (t == 0)
      for (u32 v = 0; v < nb; ++v) f.child_off[v] = 0;
    for (u32 v = n0; v < n1; ++v) w.child_count[v] = 0;  // curseurs de remplissage
    for (u32 e = lo; e < hi; ++e) {
      const u32 id = f.nid[e];
      for (u32 op : {k.ev[e].a, k.ev[e].b}) {
        u32 child;
        if (!(op & kEv)) child = op;
        else if (k.ev[op & ~kEv].rank != k.ev[e].rank) child = f.nid[op & ~kEv];
        else continue;
        f.children[f.child_off[id] + w.child_count[id]++] = child;
      }
    }
    for (u32 v = n0; v < n1; ++v)
      std::sort(f.children.begin() + f.child_off[v], f.children.begin() + f.child_off[v] + w.child_count[v]);
    team.sync();
  });
  f.groups = f.groups_multi = f.max_group = 0;  // diagnostics de groupes : voie sequentielle seulement
}

// ---- Numerotation canonique des naissances ---------------------------------------------------------------------------
struct Births {
  std::vector<u32> order;      // position canonique -> indice dans BIRTHS
  std::vector<u32> canonical;  // indice dans BIRTHS -> position canonique
};

void number_births(const d::BirthRec* births, const d::CenterRec* centers, u32 nb, Births& out) {
  out.order.resize(nb);
  std::iota(out.order.begin(), out.order.end(), 0u);
  // Les naissances sont videes par cle croissante ; le catalogue range ses boules par niveau croissant, donc les rangs
  // sont croissants et chaque cohorte de meme rang est contigue (verifie). Seules les cohortes de plus d'une
  // naissance sont triees par centre exact.
  for (u32 lo = 0; lo < nb;) {
    u32 hi = lo + 1;
    while (hi < nb && births[hi].rank == births[lo].rank) ++hi;
    if (hi < nb && births[hi].rank < births[lo].rank) throw std::runtime_error("naissances : rangs non croissants");
    if (hi - lo > 1)
      std::sort(out.order.begin() + lo, out.order.begin() + hi, [&](u32 a, u32 b) {
        const int s = d::compare_centers(centers[a], centers[b]);
        if (s == 0) throw std::runtime_error("naissances : deux centres egaux au meme rang");
        return s < 0;
      });
    lo = hi;
  }
  out.canonical.resize(nb);
  for (u32 i = 0; i < nb; ++i) out.canonical[out.order[i]] = i;
}

// ---- Reference de porte : Kruskal par lots (semantique v10, preuves_tour/foret_check.py) ----------------------------
void kruskal_lots(const OrderInput& in, std::vector<u32>& rank, std::vector<u32>& parent,
                  std::vector<std::vector<u32>>& kids) {
  const u32 nb = in.nb;
  rank.assign(in.leaf_rank.begin(), in.leaf_rank.end());
  parent.assign(nb, kNone);
  kids.assign(nb, {});
  std::vector<u32> dsu(nb), top(nb);
  std::iota(dsu.begin(), dsu.end(), 0u);
  std::iota(top.begin(), top.end(), 0u);
  auto find = [&dsu](u32 x) {
    while (dsu[x] != x) x = dsu[x] = dsu[dsu[x]];
    return x;
  };
  const u64 nj = in.jrank.size();
  for (u64 i = 0; i < nj;) {
    const u32 rk = in.jrank[i];
    u64 j = i;
    while (j < nj && in.jrank[j] == rk) ++j;
    std::vector<std::vector<u32>> pre;
    for (u64 t = i; t < j; ++t) {
      std::vector<u32> roots;
      for (u64 r = in.joff[t]; r < in.joff[t + 1]; ++r) roots.push_back(find(in.rep[r]));
      pre.push_back(roots);
    }
    for (const auto& roots : pre)
      for (std::size_t r = 1; r < roots.size(); ++r) {
        const u32 x = find(roots[0]), y = find(roots[r]);
        if (x != y) dsu[std::max(x, y)] = std::min(x, y);
      }
    std::vector<std::pair<u32, u32>> members;
    for (const auto& roots : pre)
      for (u32 r : roots) members.push_back({find(r), r});
    std::sort(members.begin(), members.end());
    members.erase(std::unique(members.begin(), members.end()), members.end());
    for (std::size_t a = 0; a < members.size();) {
      std::size_t b = a;
      while (b < members.size() && members[b].first == members[a].first) ++b;
      if (b - a >= 2) {
        const u32 node = static_cast<u32>(rank.size());
        rank.push_back(rk);
        parent.push_back(kNone);
        std::vector<u32> children;
        for (std::size_t t = a; t < b; ++t) children.push_back(top[members[t].second]);
        std::sort(children.begin(), children.end());
        for (u32 ch : children) parent[ch] = node;
        kids.push_back(children);
        top[members[a].first] = node;
      }
      a = b;
    }
    i = j;
  }
}

bool same_as_reference(const Forest& f, const std::vector<u32>& rank, const std::vector<u32>& parent,
                       const std::vector<std::vector<u32>>& kids) {
  if (f.rank != rank || f.parent != parent) return false;
  for (u32 v = 0; v < rank.size(); ++v) {
    const std::vector<u32> mine(f.children.begin() + f.child_off[v], f.children.begin() + f.child_off[v + 1]);
    if (mine != kids[v]) return false;
  }
  return true;
}

int run_gate(int cases) {
  std::mt19937_64 rng(20261007);
  int failures = 0;
  u64 nodes = 0, nary = 0, plateaus = 0, events = 0;
  auto check = [&](const OrderInput& in, const char* name) {
    Kernel k;
    if (!kernel(in, k)) return false;
    Forest f;
    contract(in, k, f);
    std::vector<u32> rank, parent;
    std::vector<std::vector<u32>> kids;
    kruskal_lots(in, rank, parent, kids);
    const bool ok = same_as_reference(f, rank, parent, kids);
    if (!ok && name != nullptr) std::cout << "{\"temoin\":\"" << name << "\",\"conforme\":false}\n";
    nodes += rank.size();
    events += k.ev.size();
    for (const auto& c : kids) nary += c.size() >= 3;
    return ok;
  };
  // CST-0212 : porte de refus a la borne du codage des operandes. 2^31 - 1 naissances : dans le domaine ; 2^31 et
  // 2^32 - 1 : refus du noyau, sans aucune allocation (le tableau des cellules reste vide).
  int bound_failures = 0;
  {
    bound_failures += !operand_domain(kMaxBirths);
    bound_failures += operand_domain(kMaxBirths + 1);
    for (u64 births : {kMaxBirths + 1, u64{kNone}}) {
      OrderInput huge;
      huge.nb = static_cast<u32>(births);
      huge.joff = {0};
      Kernel kk;
      const bool accepted = kernel(huge, kk);
      bound_failures += accepted || !kk.cell.empty() || !kk.ev.empty();
    }
    std::cout << "{\"temoin\":\"borne_operandes_cst0212\",\"maximum\":" << kMaxBirths
              << ",\"conforme\":" << (bound_failures == 0 ? "true" : "false") << "}\n";
  }
  failures += bound_failures;
  // Temoins graves : plateau ternaire par une cellule (WIT-SIX, triangle equilateral), plateau chaine par deux
  // cellules de meme rang, deux plateaux disjoints de meme rang, fusion a rang superieur.
  {
    OrderInput t;
    t.nb = 3;
    t.leaf_rank = {0, 0, 0};
    t.jrank = {1};
    t.joff = {0, 3};
    t.rep = {0, 1, 2};
    failures += !check(t, "plateau_ternaire_une_cellule");
    t.jrank = {1, 1};
    t.joff = {0, 2, 4};
    t.rep = {0, 1, 1, 2};
    failures += !check(t, "plateau_chaine_deux_cellules");
    t.nb = 5;
    t.leaf_rank = {0, 0, 0, 0, 0};
    t.jrank = {1, 1, 2};
    t.joff = {0, 2, 4, 6};
    t.rep = {0, 1, 2, 3, 3, 4};
    failures += !check(t, "plateaux_disjoints_puis_fusion");
  }
  const int named = failures;
  for (int it = 0; it < cases; ++it) {
    OrderInput in;
    in.nb = 2 + static_cast<u32>(rng() % 39);
    in.leaf_rank.assign(in.nb, 0);
    const u32 nranks = std::array<u32, 6>{1, 2, 3, 5, 12, 40}[rng() % 6];
    const u32 njoins = 1 + static_cast<u32>(rng() % (3 * in.nb));
    std::vector<std::pair<u32, std::vector<u32>>> joins;
    for (u32 j = 0; j < njoins; ++j) {
      const u32 q = std::array<u32, 6>{2, 2, 3, 3, 4, 5}[rng() % 6];
      std::vector<u32> reps;
      for (u32 r = 0; r < q; ++r) reps.push_back(static_cast<u32>(rng() % in.nb));
      joins.push_back({1 + static_cast<u32>(rng() % nranks), reps});
    }
    std::stable_sort(joins.begin(), joins.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    in.joff.push_back(0);
    for (const auto& [rk, reps] : joins) {
      in.jrank.push_back(rk);
      in.rep.insert(in.rep.end(), reps.begin(), reps.end());
      in.joff.push_back(in.rep.size());
    }
    for (std::size_t j = 1; j < joins.size(); ++j) plateaus += joins[j].first == joins[j - 1].first;
    failures += !check(in, nullptr);
  }
  std::cout << "{\"porte\":\"mes_m4\",\"mutant_sans_contraction\":" << (kMutantSansContraction ? "true" : "false")
            << ",\"temoins_ecarts\":" << named << ",\"cas_aleatoires\":" << cases << ",\"ecarts\":" << failures
            << ",\"noeuds\":" << nodes << ",\"fusions_nary_3plus\":" << nary << ",\"voisins_meme_rang\":" << plateaus
            << ",\"evenements\":" << events << "}\n";
  return failures == 0 ? 0 : 1;
}

// ---- Banc sur un vidage ---------------------------------------------------------------------------------------------
struct PreviousOrder {  // ordre k-1, pour LEM-T6
  bool valid = false;
  std::vector<u32> ball_jtop_node;   // boule -> noeud du sommet laisse par sa jonction (kNone sinon)
  std::vector<u32> ball_birth_node;  // boule -> noeud de naissance (kNone sinon)
  std::vector<u32> rank, parent;
};

int run_bench(const std::string& dir, int repetitions, u32 threads) {
  const d::Reader cat(dir + "/cat.bin");
  const u32 kmax = cat.header().kmax;
  const u32 balls = static_cast<u32>(cat.get<d::BallRec>("BALLS").second);
  std::cout << "{\"phase\":\"entree\",\"trame\":\"" << cat.frame() << "\",\"K\":" << kmax << ",\"repetitions\":"
            << repetitions << ",\"mutant_sans_contraction\":" << (kMutantSansContraction ? "true" : "false") << "}\n"
            << std::flush;
  int code = 0;
  PreviousOrder prev;
  for (u32 k = 1; k <= kmax; ++k) {
    const d::Reader order(dir + "/ordre_" + std::to_string(k) + ".bin");
    const d::Reader forest(dir + "/foret_" + std::to_string(k) + ".bin");
    // Pointeurs et comptes nommes (pas de liaisons structurees capturees par les lambdas : GCC 11).
    const auto births_s = order.get<d::BirthRec>("BIRTHS");
    const auto centers_s = order.get<d::CenterRec>("BCENTER");
    const auto cells_s = order.get<d::CellRec>("CELLS");
    const auto cell_off_s = order.get<u64>("CELLOFF");
    const auto seeds_s = order.get<d::SeedRec>("SEEDS");
    const auto nodes_s = forest.get<d::NodeRec>("FNODES");
    const auto edges_s = forest.get<u32>("FEDGES");
    const auto meta_s = forest.get<u64>("FMETA");
    const d::BirthRec* births = births_s.first;
    const d::CenterRec* centers = centers_s.first;
    const d::CellRec* cells = cells_s.first;
    const u64* cell_off = cell_off_s.first;
    const d::SeedRec* seeds = seeds_s.first;
    const d::NodeRec* nodes = nodes_s.first;
    const u32* edges = edges_s.first;
    const u64* meta = meta_s.first;
    const u64 nb64 = births_s.second, nc = centers_s.second, ncell = cells_s.second, nco = cell_off_s.second;
    const u64 ns = seeds_s.second, nn = nodes_s.second, ne = edges_s.second, nm = meta_s.second;
    if (nc != nb64 || nco != ncell + 1 || cell_off[ncell] != ns || nm != 3)
      throw std::runtime_error("vidage incoherent a l'ordre " + std::to_string(k));
    if (!operand_domain(nb64)) {  // CST-0212 : refus explicite, avant tout calcul
      std::cout << "{\"phase\":\"refus\",\"k\":" << k << ",\"raison\":\"domaine_operandes_31_bits\","
                << "\"naissances\":" << nb64 << ",\"maximum\":" << kMaxBirths << "}\n";
      return 2;
    }
    const u32 nb = static_cast<u32>(nb64);
    // 1. Naissances canoniques.
    Births b;
    const double t_births = [&]() {
      double best = 1e300;
      for (int r = 0; r < repetitions; ++r) {
        const auto t0 = Clock::now();
        number_births(births, centers, nb, b);
        best = std::min(best, seconds(t0));
      }
      return best;
    }();
    u64 birth_bad = 0;
    for (u32 i = 0; i < nb; ++i) birth_bad += births[i].v11_node != b.canonical[i];
    // Cle de naissance -> feuille canonique (dense sur les cles).
    u32 max_key = 0;
    for (u32 i = 0; i < nb; ++i) max_key = std::max(max_key, births[i].key);
    std::vector<u32> leaf_of_key(u64{max_key} + 1, kNone);
    for (u32 i = 0; i < nb; ++i) leaf_of_key[births[i].key] = b.canonical[i];
    // 2. Entree du noyau : jonctions par rang croissant, representants = feuilles canoniques des graines.
    OrderInput in;
    in.nb = nb;
    in.leaf_rank.resize(nb);
    for (u32 i = 0; i < nb; ++i) in.leaf_rank[b.canonical[i]] = births[i].rank;
    in.jrank.resize(ncell);
    in.joff.assign(cell_off, cell_off + ncell + 1);
    in.rep.resize(ns);
    u64 seed_bad = 0;
    for (u64 j = 0; j < ncell; ++j) {
      in.jrank[j] = cells[j].rank;
      if (j > 0 && cells[j].rank < cells[j - 1].rank) throw std::runtime_error("cellules : rangs non croissants");
    }
    for (u64 t = 0; t < ns; ++t) {
      const u32 key = seeds[t].key;
      const u32 leaf = key <= max_key ? leaf_of_key[key] : kNone;
      if (leaf == kNone) throw std::runtime_error("graine sans naissance");
      in.rep[t] = leaf;
      seed_bad += leaf != seeds[t].node;
    }
    // Graine de rang strictement inferieur a celui de sa cellule (date de la descente).
    u64 date_bad = 0;
    for (u64 j = 0; j < ncell; ++j)
      for (u64 t = cell_off[j]; t < cell_off[j + 1]; ++t) date_bad += in.leaf_rank[in.rep[t]] >= cells[j].rank;
    // 3. Noyau et contraction, chronometres a un fil (minimum de R passes).
    Kernel kern;
    Forest f;
    double t_kernel = 1e300, t_contract = 1e300, t_parallel = 0;
    bool kernel_ok = true;
    for (int r = 0; r < repetitions; ++r) {
      auto t0 = Clock::now();
      kernel_ok = kernel(in, kern);
      t_kernel = std::min(t_kernel, seconds(t0));
      t0 = Clock::now();
      contract(in, kern, f);
      t_contract = std::min(t_contract, seconds(t0));
    }
    // Contraction parallele (si --fils-contraction T > 1) : sortie identique exigee, minimum de R passes.
    bool parallel_same = true;
    if (threads > 1 && kernel_ok) {
      static std::unique_ptr<Team> team;
      if (!team || team->size() != threads) team = std::make_unique<Team>(threads);
      ParallelScratch scratch;
      Forest g;
      t_parallel = 1e300;
      for (int r = 0; r < repetitions; ++r) {
        const auto t0 = Clock::now();
        contract_parallel(in, kern, g, *team, scratch);
        t_parallel = std::min(t_parallel, seconds(t0));
      }
      parallel_same = g.rank == f.rank && g.parent == f.parent && g.child_off == f.child_off &&
                      g.children == f.children && g.nid == f.nid;
      if (!parallel_same) code = 1;
    }
    const bool one_root = kernel_ok && kern.ev.size() + 1 == nb;  // refus root_count sinon
    // 4. Identite avec la foret de la v11.
    const u32 nodes_v11 = static_cast<u32>(nn);
    u64 node_bad = 0, edge_bad = 0;
    bool shape = f.rank.size() == nodes_v11 && meta[0] == nb && f.children.size() == ne;
    u32 root = kNone;
    for (u32 v = 0; v < f.rank.size(); ++v)
      if (f.parent[v] == kNone) root = (root == kNone ? v : kNone - 1);
    shape = shape && root == meta[1];
    if (f.rank.size() == nodes_v11) {
      for (u32 v = 0; v < nodes_v11; ++v) {
        const u32 key = v < nb ? births[b.order[v]].key : kNone;
        const bool same = nodes[v].rank == f.rank[v] && nodes[v].parent == f.parent[v] && nodes[v].birth_key == key &&
                          nodes[v].child_count == f.child_off[v + 1] - f.child_off[v] &&
                          nodes[v].child_begin == f.child_off[v];
        node_bad += !same;
      }
      if (f.children.size() == ne)
        for (u64 i = 0; i < ne; ++i) edge_bad += edges[i] != f.children[i];
      else edge_bad = 1;
    }
    const bool identical = shape && one_root && node_bad == 0 && edge_bad == 0 && birth_bad == 0 && seed_bad == 0 &&
                           date_bad == 0;
    if (!identical) code = 1;
    // 5. LEM-T6 : image d'une naissance de l'ordre k depuis le sommet laisse par la jonction de la meme boule a
    //    l'ordre k-1 (remontee d'un cran si le parent a le rang de la boule), contre les verticales de la v11.
    u64 t6_checked = 0, t6_bad = 0, t6_birth_lower = 0;
    if (k >= 2 && prev.valid && forest.has("FLOWER")) {
      const auto lower_s = forest.get<u32>("FLOWER");
      const u32* lower = lower_s.first;
      const u64 nl = lower_s.second;
      if (nl != nn) throw std::runtime_error("FLOWER");
      for (u32 i = 0; i < nb; ++i) {
        const u32 ball = births[i].key;
        u32 image = kNone;
        if (ball < prev.ball_jtop_node.size() && prev.ball_jtop_node[ball] != kNone) {
          image = prev.ball_jtop_node[ball];
          const u32 up = prev.parent[image];
          if (up != kNone && prev.rank[up] == births[i].rank) image = up;
        } else if (ball < prev.ball_birth_node.size() && prev.ball_birth_node[ball] != kNone) {
          image = prev.ball_birth_node[ball];
          ++t6_birth_lower;
        }
        ++t6_checked;
        t6_bad += image != lower[b.canonical[i]];
      }
      if (t6_bad != 0) code = 1;
    }
    // Etat de l'ordre k pour LEM-T6 a l'ordre k+1 (cles de boules seulement : ordres >= 1 vers >= 2).
    prev = PreviousOrder{};
    if (identical) {
      prev.valid = true;
      prev.rank = f.rank;
      prev.parent = f.parent;
      prev.ball_jtop_node.assign(balls, kNone);
      prev.ball_birth_node.assign(balls, kNone);
      for (u64 j = 0; j < ncell; ++j) {
        const u32 top = kern.jtop[j];
        prev.ball_jtop_node[cells[j].ball] = (top & kEv) ? f.nid[top & ~kEv] : top;
      }
      if (k >= 2)
        for (u32 i = 0; i < nb; ++i) prev.ball_birth_node[births[i].key] = b.canonical[i];
    }
    std::cout << "{\"phase\":\"ordre\",\"k\":" << k << ",\"naissances\":" << nb << ",\"cellules\":" << ncell
              << ",\"representants\":" << ns << ",\"evenements\":" << kern.ev.size() << ",\"noeuds\":" << f.rank.size()
              << ",\"noeuds_v11\":" << nodes_v11 << ",\"fusions\":" << f.rank.size() - nb
              << ",\"groupes_de_rang\":" << f.groups << ",\"groupes_multiples\":" << f.groups_multi
              << ",\"plus_grand_groupe\":" << f.max_group << ",\"racine_unique\":" << (one_root ? "true" : "false")
              << ",\"identite\":{\"naissances_ecarts\":" << birth_bad << ",\"graines_ecarts\":" << seed_bad
              << ",\"dates_ecarts\":" << date_bad << ",\"noeuds_ecarts\":" << node_bad
              << ",\"enfants_ecarts\":" << edge_bad << ",\"forme\":" << (shape ? "true" : "false")
              << ",\"identiques\":" << (identical ? "true" : "false") << "},\"lem_t6\":{\"naissances_jugees\":"
              << t6_checked << ",\"ecarts\":" << t6_bad << ",\"images_par_naissance_basse\":" << t6_birth_lower
              << "},\"temps_un_fil_s\":{\"naissances\":" << t_births << ",\"noyau\":" << t_kernel
              << ",\"contraction\":" << t_contract << "},\"contraction_parallele\":{\"fils\":" << threads
              << ",\"secondes\":" << t_parallel << ",\"identique\":" << (parallel_same ? "true" : "false") << "}}\n"
              << std::flush;
  }
  std::cout << "{\"phase\":\"fin\",\"code\":" << code << "}\n";
  return code;
}

}  // namespace
}  // namespace mhgp12

int main(int argc, char** argv) {
  try {
    if (argc >= 2 && std::string(argv[1]) == "--porte")
      return mhgp12::run_gate(argc >= 3 ? std::max(1, std::atoi(argv[2])) : 6000);
    if (argc < 2) {
      std::cerr << "usage : mhgp12_mes_m4 --porte [cas] | <dossier> [--repetitions R] [--fils-contraction T]\n";
      return 2;
    }
    int repetitions = 5;
    mhgp12::u32 threads = 1;
    for (int i = 2; i < argc; ++i) {
      const std::string opt = argv[i];
      if (opt == "--repetitions" && i + 1 < argc) repetitions = std::max(1, std::atoi(argv[++i]));
      else if (opt == "--fils-contraction" && i + 1 < argc)
        threads = static_cast<mhgp12::u32>(std::clamp(std::atoi(argv[++i]), 1, 256));
      else {
        std::cerr << "option inconnue : " << opt << "\n";
        return 2;
      }
    }
    return mhgp12::run_bench(argv[1], repetitions, threads);
  } catch (const std::exception& e) {
    std::cout << "{\"phase\":\"exception\",\"message\":\"" << e.what() << "\"}\n";
    return 3;
  }
}
