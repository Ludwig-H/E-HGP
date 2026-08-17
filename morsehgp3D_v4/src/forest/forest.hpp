// MorseHGP3D v4 — NOYAU DE FORET : union-find a MACRO-LOTS de niveaux
// semantiquement egaux (MATHEMATIQUES.md § 5).
//
// L'evenement de foret porte le K-simplexe σ = S ∪ I entier (support
// d'arite q, d = K+1-q interieurs) et son niveau exact promu en Q4Level.
// build_forest unionne les K+1 facettes de chaque evenement (chemin sur
// toutes — equivalent clique pour la connectivite, conforme a la Def. 29).
//
// INVARIANT DES RAYONS DE NAISSANCE (theoreme, § 5.2) : une facette non
// active σ∖{z} (z interieur) nait AU niveau ρ(σ) ; si elle etait facette
// d'un simplexe de niveau STRICTEMENT inferieur, son rayon de naissance
// serait < ρ(σ) — contradiction. Un bras non actif deja vu dans un lot
// anterieur refuterait donc la coherence des niveaux du flux :
// `attach_violations` le MESURE (porte : 0), au lieu de le supposer.
//
// MACRO-LOT (contrat grave) : groupe maximal d'evenements de niveau
// semantiquement egal (`compare_exact_level` U320, JAMAIS l'egalite de
// representation) ; racines gelees avant le lot, toutes les unions du lot
// ensemble, puis UN nœud de dendrogramme par racine finale ayant absorbe
// plusieurs composantes pre-lot — aucune chronologie binaire artificielle.
#pragma once

#include <algorithm>
#include <map>
#include <vector>

#include "../events/q4_event.hpp"

namespace mhgp4 {

// Facette = K-uplet trie de PointId (K <= 10).
struct FacetKey {
  u8 k = 0;
  std::array<PointId, 10> p{};
  bool operator<(const FacetKey& o) const {
    if (k != o.k) return k < o.k;
    return p < o.p;
  }
  bool operator==(const FacetKey& o) const { return k == o.k && p == o.p; }
};

// Evenement de foret : σ = part T (q points de coquille portant le centre,
// c ∈ conv(T)) ∪ interieurs I (d points), K = q + d - 1, niveau exact
// promu (rayon au carre). Sous position generale T est le support minimal
// (q <= 4) ; sur un PLATEAU spherique (§ 5.3bis) T peut monter a 11 —
// les bras de retrait d'un point de T peuvent naitre plus tot ou au meme
// niveau, ceux de retrait d'un interieur naissent AU niveau (invariant des
// rayons de naissance).
struct ForestEvent {
  u8 q = 0;  // |T| <= 11
  u8 d = 0;  // |I| <= 9
  // Bit t : la facette σ∖{T[t]} est ACTIVE (nee STRICTEMENT avant le
  // niveau — sa miniboule retrecit : c ∉ conv(T∖{v})). Un bit a 0 = la
  // facette garde la meme boule et nait AU niveau (attachement). Les
  // retraits d'interieur sont toujours des attachements. Regime regulier
  // (T = support minimal) : tous les bits a 1.
  u16 active_mask = 0;
  PointId support[11] = {};
  PointId interior[9] = {};
  Q4Level level{};
};

// Fusion enregistree : un nœud de dendrogramme par racine de lot ayant
// absorbe `absorbed` composantes pre-lot (>= 2).
struct ForestNode {
  u64 batch = 0;      // index du macro-lot (niveaux croissants)
  u64 absorbed = 0;   // composantes pre-lot fusionnees dans ce nœud
};

struct ForestResult {
  u64 facets = 0;             // sommets du K-graphe effectivement vus
  u64 fusions = 0;            // unions effectives (toutes facettes)
  u64 attach_violations = 0;  // attachement DEJA vu dans un lot anterieur (: 0)
  u64 birth_violations = 0;   // facette active ET attachement au meme lot (: 0)
  u64 new_attachments = 0;    // facettes nees au lot, hors enfants du nœud
  u64 batches = 0;            // macro-lots traites
  std::vector<ForestNode> nodes;
  std::vector<u64> batch_of_event;  // lot de chaque evenement (ordre trie)
  // Partition FINALE canonique (facette -> plus petite facette de sa
  // composante) : O(facettes), toujours remplie. Les instantanes par lot
  // (parametre `snapshots`) restent reserves aux petits n — ils coutent
  // O(lots × facettes).
  std::map<FacetKey, FacetKey> final_partition;
};

namespace detail_forest {

struct UnionFind {
  std::vector<i32> parent;
  i32 add() {
    parent.push_back((i32)parent.size());
    return (i32)parent.size() - 1;
  }
  i32 find(i32 v) {
    while (parent[(size_t)v] != v) {
      parent[(size_t)v] = parent[(size_t)parent[(size_t)v]];
      v = parent[(size_t)v];
    }
    return v;
  }
  bool unite(i32 a, i32 b) {
    a = find(a);
    b = find(b);
    if (a == b) return false;
    parent[(size_t)b] = a;
    return true;
  }
};

inline FacetKey facet_minus(const ForestEvent& e, int drop_support,
                            int drop_interior) {
  FacetKey f;
  for (int t = 0; t < (int)e.q; ++t)
    if (t != drop_support) f.p[f.k++] = e.support[t];
  for (int t = 0; t < (int)e.d; ++t)
    if (t != drop_interior) f.p[f.k++] = e.interior[t];
  // Tri par insertion (k <= 10) : evite le faux positif -Warray-bounds de
  // GCC 13 sur les sous-intervalles de tableau fixe.
  for (u8 t = 1; t < f.k; ++t) {
    const PointId v = f.p[t];
    u8 w = t;
    for (; w > 0 && f.p[w - 1] > v; --w) f.p[w] = f.p[w - 1];
    f.p[w] = v;
  }
  return f;
}

}  // namespace detail_forest

// Construit la foret K a partir d'evenements DEJA filtres pour ce K
// (q + d = K + 1). Les evenements sont tries ici par niveau exact.
// `snapshots` (optionnel) : partition canonique apres CHAQUE macro-lot
// (facette -> plus petite facette de sa composante), pour le juge.
inline ForestResult build_forest(
    std::vector<ForestEvent> events, bool mutant_binary_ties = false,
    bool mutant_repr_equality = false,
    std::vector<std::map<FacetKey, FacetKey>>* snapshots = nullptr,
    bool mutant_attach_prebatch = false) {
  ForestResult r;
  std::stable_sort(events.begin(), events.end(),
                   [](const ForestEvent& x, const ForestEvent& y) {
                     return compare_exact_level(x.level, y.level) < 0;
                   });
  detail_forest::UnionFind uf;
  std::map<FacetKey, i32> id_of;
  const auto facet_id = [&](const FacetKey& f) {
    const auto it = id_of.find(f);
    if (it != id_of.end()) return it->second;
    const i32 v = uf.add();
    id_of.emplace(f, v);
    ++r.facets;
    return v;
  };
  r.batch_of_event.assign(events.size(), 0);
  size_t e0 = 0;
  while (e0 < events.size()) {
    // Macro-lot [e0, e1) : niveaux semantiquement egaux. MUTANTS : ties
    // binaires (lot force a un seul evenement) ou egalite de
    // REPRESENTATION (le piege grave par l'audit : deux representants
    // differents du meme rationnel brisent le lot a tort).
    size_t e1 = e0 + 1;
    while (e1 < events.size()) {
      const bool same =
          mutant_repr_equality
              ? same_level_representation(events[e1].level, events[e0].level)
              : same_exact_level(events[e1].level, events[e0].level);
      if (mutant_binary_ties || !same) break;
      ++e1;
    }
    // ROLES agreges par facette sur TOUT le lot, AVANT toute creation d'ID
    // (audit « facettes nees dans le lot ») : une facette n'est un enfant
    // du nœud que si elle est ACTIVE (nee strictement avant le niveau) ou
    // PREEXISTANTE ; les attachements nes au lot restent dans la fermeture
    // union-find mais jamais dans `absorbed`.
    struct Role {
      bool existed = false, active = false, attach = false;
      i32 id = -1;
    };
    std::map<FacetKey, Role> roles;
    for (size_t e = e0; e < e1; ++e) {
      r.batch_of_event[e] = r.batches;
      const ForestEvent& ev = events[e];
      for (int s = 0; s < (int)ev.q; ++s) {
        Role& ro = roles[detail_forest::facet_minus(ev, s, -1)];
        if ((ev.active_mask >> s) & 1u) ro.active = true;
        else ro.attach = true;
      }
      for (int z = 0; z < (int)ev.d; ++z)
        roles[detail_forest::facet_minus(ev, -1, z)].attach = true;
    }
    for (auto& kv : roles) {
      kv.second.existed = id_of.find(kv.first) != id_of.end();
      // Invariants (audit § 3) : un attachement deja vu, ou une facette a
      // la fois active et attachement au meme niveau, refutent la
      // coherence des rayons de naissance du flux.
      if (kv.second.attach && kv.second.existed) ++r.attach_violations;
      if (kv.second.attach && kv.second.active) ++r.birth_violations;
      if (kv.second.attach && !kv.second.existed && !kv.second.active)
        ++r.new_attachments;
    }
    for (auto& kv : roles) kv.second.id = facet_id(kv.first);
    // Racines PRE-LOT : actives (leur rayon de naissance est strictement
    // inferieur, meme jamais rencontrees) OU preexistantes. MUTANT
    // attach-prebatch : l'ancienne convention fausse (tout le lot).
    std::map<i32, u64> prebatch_roots;
    for (const auto& kv : roles)
      if (mutant_attach_prebatch || kv.second.active || kv.second.existed)
        prebatch_roots[uf.find(kv.second.id)] = 0;
    // Unions du lot : chemin sur les K+1 facettes de chaque evenement.
    for (size_t e = e0; e < e1; ++e) {
      const ForestEvent& ev = events[e];
      i32 first = -1;
      for (int s = 0; s < (int)ev.q; ++s) {
        const i32 v = roles[detail_forest::facet_minus(ev, s, -1)].id;
        if (first < 0) first = v;
        else if (uf.unite(first, v)) ++r.fusions;
      }
      for (int z = 0; z < (int)ev.d; ++z) {
        const i32 v = roles[detail_forest::facet_minus(ev, -1, z)].id;
        if (uf.unite(first, v)) ++r.fusions;
      }
    }
    // Nœuds du lot : une racine finale ayant absorbe >= 2 racines pre-lot
    // donne UN nœud (jamais une chaine binaire).
    std::map<i32, u64> absorbed;  // racine post-lot -> composantes pre-lot
    for (const auto& pr : prebatch_roots)
      ++absorbed[uf.find(pr.first)];
    for (const auto& ab : absorbed)
      if (ab.second >= 2) r.nodes.push_back(ForestNode{r.batches, ab.second});
    if (snapshots) {
      std::map<FacetKey, FacetKey> part;
      std::map<i32, FacetKey> canon;
      for (const auto& kv : id_of) {
        const i32 root = uf.find(kv.second);
        if (canon.find(root) == canon.end()) canon.emplace(root, kv.first);
      }
      for (const auto& kv : id_of) part[kv.first] = canon[uf.find(kv.second)];
      snapshots->push_back(std::move(part));
    }
    ++r.batches;
    e0 = e1;
  }
  {
    std::map<i32, FacetKey> canon;
    for (const auto& kv : id_of) {
      const i32 root = uf.find(kv.second);
      if (canon.find(root) == canon.end()) canon.emplace(root, kv.first);
    }
    for (const auto& kv : id_of)
      r.final_partition[kv.first] = canon[uf.find(kv.second)];
  }
  return r;
}

}  // namespace mhgp4
