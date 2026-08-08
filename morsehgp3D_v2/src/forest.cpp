// MorseHGP3D v2 — foret de fusion d'un ordre k a partir du catalogue.
//
// Naissances : spheres critiques de rang ferme k.
// Fusions    : spheres critiques de rang ferme k+1 ; chacune porte |U| bras
//              F_u = S \ {u}, u ∈ U, de niveau strictement inferieur.
// Chaque bras est resolu par descente (DESIGN.md §4) jusqu'a un minimum, puis
// un balayage par niveau croissant avec DSU par lots produit la foret.

#include "mhgp/mhgp.hpp"
#include "mhgp/miniball.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <vector>
#include <unordered_map>
#include <vector>

namespace mhgp {
namespace {

struct DSU {
  std::vector<i32> parent;
  std::vector<i32> node;  // noeud courant de la foret pour cette classe
  i32 find(i32 a) {
    while (parent[static_cast<size_t>(a)] != a) {
      parent[static_cast<size_t>(a)] = parent[static_cast<size_t>(parent[static_cast<size_t>(a)])];
      a = parent[static_cast<size_t>(a)];
    }
    return a;
  }
};

// L'identite d'un minimum est son ENSEMBLE de points, pas un hash : un hash
// 64 bits n'est pas une preuve d'egalite d'ensembles
// (WARNING_AUDIT_IMPLEMENTATION_2 §4). On indexe donc par la cle complete.
using Key = std::vector<i32>;

}  // namespace

Forest build_forest(const std::vector<P3>& pts, const Catalogue& cat, int k) {
  Forest F;
  F.order = k;

  // ---- index des minima (spheres de rang exactement k) -------------------
  std::map<Key, i32> min_index;
  std::vector<std::vector<i32>> min_members;
  std::vector<double> min_beta;
  for (size_t i = 0; i < cat.spheres.size(); ++i) {
    const CriticalSphere& s = cat.spheres[i];
    if (s.rank != k) continue;
    std::vector<i32> mem(cat.members.begin() + s.members_begin,
                         cat.members.begin() + s.members_begin + s.rank);
    std::sort(mem.begin(), mem.end());
    if (min_index.find(mem) != min_index.end()) continue;
    min_index.emplace(mem, static_cast<i32>(min_members.size()));
    min_beta.push_back(s.beta);
    min_members.push_back(std::move(mem));
  }

  const i32 nmin = static_cast<i32>(min_members.size());
  DSU dsu;
  dsu.parent.resize(static_cast<size_t>(nmin));
  dsu.node.assign(static_cast<size_t>(nmin), -1);
  for (i32 i = 0; i < nmin; ++i) dsu.parent[static_cast<size_t>(i)] = i;

  // ---- descente d'un ensemble de k points vers un minimum ----------------
  auto descend = [&](std::vector<i32> Fset) -> i32 {
    for (int step = 0; step < 4096; ++step) {
      std::sort(Fset.begin(), Fset.end());
      auto it = min_index.find(Fset);
      if (it != min_index.end()) return it->second;
      const MiniballResult mb = miniball_of(pts, Fset.data(), static_cast<int>(Fset.size()));
      if (!mb.ok) return -1;
      // cherche un intrus strictement interieur
      i32 intruder = -1;
      for (size_t z = 0; z < pts.size(); ++z) {
        if (std::binary_search(Fset.begin(), Fset.end(), static_cast<i32>(z))) continue;
        if (sphere_side(mb.sph, pts[z]) < 0) { intruder = static_cast<i32>(z); break; }
      }
      if (intruder < 0) return -1;  // pas de minimum atteint (degenerescence)
      // remplace un point du support par l'intrus : beta decroit strictement
      const i32 victim = mb.support[0];
      Fset.erase(std::find(Fset.begin(), Fset.end(), victim));
      Fset.push_back(intruder);
    }
    return -1;
  };

  // ---- evenements tries par niveau ---------------------------------------
  struct Event {
    const Sphere* sph;
    i32 idx;      // index de la sphere dans le catalogue
    int kind;     // 0 naissance, 1 fusion
    double beta;  // publication seulement, jamais une decision
  };
  std::vector<Event> events;
  events.reserve(cat.spheres.size());
  for (size_t i = 0; i < cat.spheres.size(); ++i) {
    const CriticalSphere& s = cat.spheres[i];
    if (s.rank == k) events.push_back({&s.sph, static_cast<i32>(i), 0, s.beta});
    else if (s.rank == k + 1) events.push_back({&s.sph, static_cast<i32>(i), 1, s.beta});
  }
  // Tri par comparaison EXACTE des niveaux rationnels : les doubles ne servent
  // qu'a la publication (WARNING_AUDIT_IMPLEMENTATION_2 §4).
  std::sort(events.begin(), events.end(), [&](const Event& a, const Event& b) {
    const int c = sphere_cmp_beta(*a.sph, *b.sph);
    if (c != 0) return c < 0;
    return a.kind < b.kind;
  });

  auto new_node = [&](double beta, int kind) {
    ForestNode n;
    n.beta = beta;
    n.kind = kind;
    F.nodes.push_back(n);
    return static_cast<i32>(F.nodes.size()) - 1;
  };

  // naissances : un noeud par minimum, cree lors du balayage
  std::vector<u8> born(static_cast<size_t>(nmin), 0);

  size_t e = 0;
  while (e < events.size()) {
    // lot de meme niveau (comparaison exacte a l'interieur d'une plage
    // flottante identique)
    size_t j = e;
    while (j < events.size() && sphere_cmp_beta(*events[j].sph, *events[e].sph) == 0) ++j;

    // 1. naissances du lot
    for (size_t t = e; t < j; ++t) {
      if (events[t].kind != 0) continue;
      const CriticalSphere& s = cat.spheres[static_cast<size_t>(events[t].idx)];
      std::vector<i32> mem(cat.members.begin() + s.members_begin,
                           cat.members.begin() + s.members_begin + s.rank);
      std::sort(mem.begin(), mem.end());
      auto it = min_index.find(mem);
      if (it == min_index.end()) continue;
      const i32 m = it->second;
      if (born[static_cast<size_t>(m)]) continue;
      born[static_cast<size_t>(m)] = 1;
      const i32 nd = new_node(s.beta, 0);
      F.nodes[static_cast<size_t>(nd)].source = events[t].idx;
      dsu.node[static_cast<size_t>(dsu.find(m))] = nd;
      ++F.births;
    }

    // 2. fusions du lot : on resout tous les bras AVANT toute union, puis on
    //    contracte le lot en composantes connexes. Appliquer les unions
    //    evenement par evenement produirait une chaine de fusions binaires de
    //    meme niveau au lieu d'une multifusion
    //    (WARNING_AUDIT_IMPLEMENTATION_2 §4, DEFINITION_HGP_3D §6).
    std::vector<std::vector<i32>> hyper;  // une hyperarete par evenement
    std::vector<i32> hyper_source;        // sphere de rang k+1 qui l'a produite
    for (size_t t = e; t < j; ++t) {
      if (events[t].kind != 1) continue;
      const CriticalSphere& s = cat.spheres[static_cast<size_t>(events[t].idx)];
      std::vector<i32> S(cat.members.begin() + s.members_begin,
                         cat.members.begin() + s.members_begin + s.rank);
      std::vector<i32> roots;
      bool broken = false;
      for (int u = 0; u < s.n_support; ++u) {
        std::vector<i32> arm;
        arm.reserve(S.size());
        for (i32 v : S) if (v != s.support[u]) arm.push_back(v);
        const i32 mn = descend(std::move(arm));
        if (mn < 0) { ++F.unresolved_arms; broken = true; continue; }
        roots.push_back(dsu.find(mn));
      }
      // Censure ATOMIQUE : un evenement dont un bras n'est pas resolu ne doit
      // pas produire d'hyperarete partielle, et il retire son autorite a toute
      // la foret (WARNING_AUDIT_IMPLEMENTATION_2 §4).
      if (broken) {
        ++F.censored_events;
        F.authoritative = false;
        continue;
      }
      std::sort(roots.begin(), roots.end());
      roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
      if (roots.size() >= 2) {
        hyper.push_back(std::move(roots));
        hyper_source.push_back(events[t].idx);
      }
    }

    // 3. composantes connexes du lot, puis une multifusion par composante
    if (!hyper.empty()) {
      std::map<i32, i32> local;   // racine DSU -> index local
      std::vector<i32> back;
      for (const auto& h : hyper)
        for (i32 r : h)
          if (local.emplace(r, static_cast<i32>(back.size())).second) back.push_back(r);
      std::vector<i32> par(back.size());
      for (size_t i = 0; i < par.size(); ++i) par[i] = static_cast<i32>(i);
      std::function<i32(i32)> find = [&](i32 a) {
        while (par[static_cast<size_t>(a)] != a) {
          par[static_cast<size_t>(a)] = par[static_cast<size_t>(par[static_cast<size_t>(a)])];
          a = par[static_cast<size_t>(a)];
        }
        return a;
      };
      for (const auto& h : hyper) {
        const i32 a = find(local[h[0]]);
        for (size_t t = 1; t < h.size(); ++t) {
          const i32 b = find(local[h[t]]);
          if (a != b) par[static_cast<size_t>(b)] = a;
        }
      }
      std::map<i32, std::vector<i32>> groups;
      for (size_t i = 0; i < back.size(); ++i)
        groups[find(static_cast<i32>(i))].push_back(back[i]);

      // Sphere source de chaque composante : la plus petite (par index) des
      // spheres de rang k+1 du lot qui y contribuent. Toutes les spheres du lot
      // ont le MEME niveau exact, donc ce choix ne fixe qu'un representant --
      // mais il donne au noeud un acces exact a son propre niveau, que `beta`
      // ne peut pas fournir (arrondi d'un ULP aux niveaux critiques).
      std::map<i32, i32> group_source;
      for (size_t hi = 0; hi < hyper.size(); ++hi) {
        const i32 rep = find(local[hyper[hi][0]]);
        auto it = group_source.find(rep);
        if (it == group_source.end()) group_source.emplace(rep, hyper_source[hi]);
        else it->second = std::min(it->second, hyper_source[hi]);
      }

      for (auto& g : groups) {
        std::vector<i32>& live = g.second;
        std::sort(live.begin(), live.end());
        live.erase(std::unique(live.begin(), live.end()), live.end());
        if (live.size() < 2) continue;
        const i32 src = group_source.at(g.first);
        const i32 nd = new_node(cat.spheres[static_cast<size_t>(src)].beta, 1);
        F.nodes[static_cast<size_t>(nd)].source = src;
        for (i32 r : live) {
          const i32 child = dsu.node[static_cast<size_t>(r)];
          if (child >= 0) {
            F.nodes[static_cast<size_t>(child)].parent = nd;
            F.nodes[static_cast<size_t>(child)].next_sibling =
                F.nodes[static_cast<size_t>(nd)].first_child;
            F.nodes[static_cast<size_t>(nd)].first_child = child;
            ++F.nodes[static_cast<size_t>(nd)].n_children;
          }
        }
        const i32 keep = live[0];
        for (size_t t = 1; t < live.size(); ++t)
          dsu.parent[static_cast<size_t>(live[t])] = keep;
        dsu.node[static_cast<size_t>(dsu.find(keep))] = nd;
        ++F.merge_events;
        F.killed += static_cast<i64>(live.size()) - 1;
      }
    }
    e = j;
  }

  for (size_t i = 0; i < F.nodes.size(); ++i)
    if (F.nodes[i].parent < 0) F.roots.push_back(static_cast<i32>(i));
  return F;
}

}  // namespace mhgp
