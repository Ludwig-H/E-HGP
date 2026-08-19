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
#include <array>
#include <chrono>
#include <map>
#include <string>
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
// absorbe `absorbed` composantes pre-lot (>= 2). VUE DERIVEE des seuls
// deltas a >= 2 parents — le payload hierarchique complet est ComponentDelta.
struct ForestNode {
  u64 batch = 0;      // index du macro-lot (niveaux croissants)
  u64 absorbed = 0;   // composantes pre-lot fusionnees dans ce nœud
};

// TRANSITION COMPLETE d'une composante dans un macro-lot (audits
// « naissances et croissances ») :
//   parents vide      -> NAISSANCE (la composante apparait entiere) ;
//   1 parent          -> CROISSANCE (le polyedre absorbe des facettes) ;
//   >= 2 parents      -> (MULTI)FUSION, eventuellement avec facettes nees.
// `output` et `parents` : identifiants canoniques deterministes = plus
// petite FacetKey de la composante (post-lot / pre-lot). `born` : les
// facettes nees exactement dans ce lot (attachment ∧ !existed ∧ !active),
// membres pleins de F_K^render. Le niveau EXACT est conserve (representant
// canonique par boule — comparable en representation).
struct ComponentDelta {
  u64 batch = 0;
  Q4Level level{};
  FacetKey output;
  std::vector<FacetKey> parents;  // tries
  std::vector<FacetKey> born;     // triees
  bool operator==(const ComponentDelta& o) const {
    return batch == o.batch && level == o.level && output == o.output &&
           parents == o.parents && born == o.born;
  }
  bool operator<(const ComponentDelta& o) const {
    if (batch != o.batch) return batch < o.batch;
    if (!(output == o.output)) return output < o.output;
    if (parents != o.parents) return parents < o.parents;
    if (born != o.born) return born < o.born;
    return level < o.level;
  }
};

struct ForestResult {
  u64 facets = 0;             // sommets du K-graphe effectivement vus
  u64 fusions = 0;            // unions effectives (toutes facettes)
  u64 attach_violations = 0;  // attachement DEJA vu dans un lot anterieur (: 0)
  u64 birth_violations = 0;   // facette active ET attachement au meme lot (: 0)
  u64 new_attachments = 0;    // facettes nees au lot, hors enfants du nœud
  u64 batches = 0;            // macro-lots traites
  std::vector<ForestNode> nodes;
  std::vector<ComponentDelta> deltas;  // le payload hierarchique complet
  std::vector<Q4Level> batch_levels;   // niveau exact de chaque lot
  std::vector<u64> batch_of_event;  // lot de chaque evenement (ordre trie)
  // Partition FINALE canonique (facette -> plus petite facette de sa
  // composante) : O(facettes), toujours remplie. Les instantanes par lot
  // (parametre `snapshots`) restent reserves aux petits n — ils coutent
  // O(lots × facettes).
  std::map<FacetKey, FacetKey> final_partition;
  // FOLD COMPACT (reponse d'audit « fold compact » § 1.1) : la
  // representation DENSE est la source de verite — facet_keys[fid]
  // strictement croissante, final_canon_fid[fid] = plus petit fid de la
  // composante (le canonique est facet_keys[final_canon_fid[fid]]) ;
  // final_partition n'est plus qu'une VUE de compatibilite, remplie sur
  // demande (fill_legacy_partition). partition_violations compte les
  // invariants structurels (tri strict, canon <= fid, idempotence) :
  // toujours 0, un ecart est traite comme un invariant de roles.
  std::vector<FacetKey> facet_keys;
  std::vector<u32> final_canon_fid;
  u64 partition_violations = 0;
  // GARDE DE CAPACITE (contre-audit 5d274a1 § 7) : non vide = entree
  // REFUSEE avant allocation (resource_exhausted / requires_tiling),
  // jamais une troncature — tous les autres champs sont alors vides.
  std::string refusal;
  // Chronos GROSSIERS (§ 1.4 — jamais un steady_clock par lot ; la
  // separation roles/unions/deltas n'est volontairement pas mesuree).
  double t_batching_ms = 0, t_intern_ms = 0, t_reduce_ms = 0,
         t_partition_ms = 0;
  // Sous-postes de l'internement (trois marques par fold, jamais par
  // lot) : balayage streaming, tri des cles UNIQUES, retour des
  // identifiants temporaires vers les fid. Le tri porte exactement sur
  // la sortie — c'est le plancher incompressible tant que la partition
  // dense publie des facettes triees.
  double t_intern_scan_ms = 0, t_intern_sort_ms = 0, t_intern_remap_ms = 0;
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

// COMPARATEUR D'INTERNEMENT : dans une foret K donnee, toutes les
// facettes ont k = K, et les entrees au-dela de k sont NULLES par
// construction (`facet_minus` part d'une cle value-initialisee et
// remplit exactement k mots). Comparer `k` puis les k premiers mots est
// donc STRICTEMENT l'ordre de `FacetKey::operator<` (qui compare les dix
// mots), sans parcourir les mots morts — a K = 1 ou 2, l'ordre generique
// paie 10 comparaisons pour 1 ou 2 utiles.
inline bool facet_less_k(const FacetKey& x, const FacetKey& y) {
  if (x.k != y.k) return x.k < y.k;
  for (u8 i = 0; i < x.k; ++i)
    if (x.p[i] != y.p[i]) return x.p[i] < y.p[i];
  return false;
}

inline bool facet_equal_k(const FacetKey& x, const FacetKey& y) {
  if (x.k != y.k) return false;
  for (u8 i = 0; i < x.k; ++i)
    if (x.p[i] != y.p[i]) return false;
  return true;
}

// Empreinte 64 bits de la facette (splitmix64 sur k et les k mots). Elle
// ne sert QU'A l'adressage de la table d'internement : l'appartenance est
// toujours tranchee par une comparaison EXACTE de cle, et l'ordre des fid
// vient du tri final des cles uniques — aucune sortie ne depend du
// hachage.
inline u64 facet_fingerprint(const FacetKey& f) {
  // Polynome a multiplicateur impair (deux operations par mot, l'ordre
  // des points compte) puis finalisation splitmix : les bits hauts, qui
  // servent d'etiquette de case, dependent de TOUS les mots. Le cout par
  // incidence est le poste chaud — une ronde de melange complete par mot
  // couterait cinq fois plus pour aucune propriete utile ici, la
  // comparaison de cle restant l'autorite.
  u64 h = 0x9E3779B97F4A7C15ull ^ (u64)f.k;
  for (u8 i = 0; i < f.k; ++i)
    h = (h + (u64)f.p[i]) * 0x9E3779B97F4A7C15ull;
  h ^= h >> 31;
  h *= 0xBF58476D1CE4E5B9ull;
  h ^= h >> 27;
  h *= 0x94D049BB133111EBull;
  h ^= h >> 31;
  return h;
}

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

// TABLE D'INTERNEMENT EN STREAMING (reponse d'audit « fold compact »
// § 3, contrat d'echelle) : adressage ouvert, sondage lineaire, charge
// <= 1/2, capacite en puissance de deux. Chaque case tient sur 64 bits
// (empreinte tronquee a 32 bits << 32 | tid+1 ; 0 = vide) : l'empreinte
// evite de toucher le pool sur collision, la comparaison EXACTE de cle
// tranche l'appartenance. AUCUN tableau de records (facette, evenement,
// slot) n'est materialise — c'etait 52 octets par incidence, double par
// le tampon de fusion de `stable_sort`.
//
// INDEPENDANCE AU HACHAGE (invariant public) : les tid sont attribues
// dans l'ordre de RENCONTRE, mais les fid publies viennent du tri des
// cles UNIQUES — l'ordre `fid croissant <=> FacetKey croissante`, dont
// depend le canonique min-fid, ne doit rien a l'empreinte ni au
// sondage.
struct FacetIntern {
  static constexpr u64 kTag = 0xFFFFFFFF00000000ull;
  std::vector<u64> table;                      // cases
  std::vector<std::pair<FacetKey, u32>> pool;  // tid -> (cle, tid)
  size_t mask = 0;
  // MUTANT : `no_verify` decide l'appartenance sur huit bits d'empreinte
  // SANS comparer la cle.
  bool no_verify = false;

  // DIMENSIONNEMENT UNE FOIS POUR TOUTES a partir du nombre d'incidences
  // (majorant EXACT du nombre de facettes uniques : l'internement
  // dedoublonne, jamais n'ajoute) : la table ne rehache JAMAIS et le
  // pool ne se realloue jamais. Le rehachage par doublement coutait deux
  // fois le nombre de facettes en sondages aleatoires supplementaires —
  // mesure du 18 aout — pour une capacite finale identique. `reserve` ne
  // touche pas les pages : le RSS suit les facettes reellement
  // internees, pas le majorant.
  explicit FacetIntern(size_t incidences) {
    size_t cap = 1024;
    while (cap < incidences * 2 + 2) cap <<= 1;
    table.assign(cap, 0);
    mask = cap - 1;
    pool.reserve(incidences);
  }

  // Prefetch de la case visee : la table depasse largement les caches
  // (134 Mo a n=8000, K=10), donc chaque sondage est un defaut de cache
  // ET de TLB. Le pipeline logiciel de l'appelant precharge un bloc de
  // cases pendant qu'il calcule les cles suivantes.
  void prefetch(u64 h) const {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(&table[(size_t)h & mask], 1, 1);
#else
    (void)h;
#endif
  }

  u32 intern(const FacetKey& f) { return intern_hashed(f, facet_fingerprint(f)); }

  u32 intern_hashed(const FacetKey& f, u64 h) {
    const u64 tag = h & kTag;
    size_t i = (size_t)h & mask;
    while (table[i] != 0) {
      const u32 tid = (u32)(table[i] & 0xFFFFFFFFull) - 1;
      const bool hit =
          no_verify ? (((table[i] ^ tag) & (0xFFull << 32)) == 0)
                    : ((table[i] & kTag) == tag &&
                       facet_equal_k(pool[(size_t)tid].first, f));
      if (hit) return tid;
      i = (i + 1) & mask;
    }
    const u32 tid = (u32)pool.size();
    pool.push_back({f, tid});
    table[i] = tag | (u64)(tid + 1);
    return tid;
  }
};

}  // namespace detail_forest

// ---- BACKEND FIGE DE COMPARAISON (reponse d'audit « fold compact »
// § 2.1) : copie STRICTE du fold d'avant la representation dense, ne
// sert QUE la porte --fold-compact-gate (egalite des paires de
// partition, des nœuds, des deltas et des niveaux entre backends). A
// SUPPRIMER apres requalification du fold compact. Ne pas optimiser,
// ne pas corriger : il est le temoin.
// Construit la foret K a partir d'evenements DEJA filtres pour ce K
// (q + d = K + 1). Les evenements sont tries ici par niveau exact.
// `snapshots` (optionnel) : partition canonique apres CHAQUE macro-lot
// (facette -> plus petite facette de sa composante), pour le juge.
inline ForestResult build_forest_legacy(
    std::vector<ForestEvent> events, bool mutant_binary_ties = false,
    bool mutant_repr_equality = false,
    std::vector<std::map<FacetKey, FacetKey>>* snapshots = nullptr,
    bool mutant_attach_prebatch = false,
    bool mutant_drop_nonmerge = false) {
  ForestResult r;
  std::stable_sort(events.begin(), events.end(),
                   [](const ForestEvent& x, const ForestEvent& y) {
                     return compare_exact_level(x.level, y.level) < 0;
                   });
  // LOTS d'abord (independants des unions) : plages de niveaux
  // semantiquement egaux. MUTANTS : ties binaires (lot force a un seul
  // evenement) ou egalite de REPRESENTATION (le piege grave par l'audit :
  // deux representants differents du meme rationnel brisent le lot a tort).
  r.batch_of_event.assign(events.size(), 0);
  std::vector<std::pair<size_t, size_t>> batch_bounds;
  {
    size_t b0 = 0;
    while (b0 < events.size()) {
      size_t b1 = b0 + 1;
      while (b1 < events.size()) {
        const bool same =
            mutant_repr_equality
                ? same_level_representation(events[b1].level, events[b0].level)
                : same_exact_level(events[b1].level, events[b0].level);
        if (mutant_binary_ties || !same) break;
        ++b1;
      }
      for (size_t e = b0; e < b1; ++e)
        r.batch_of_event[e] = batch_bounds.size();
      batch_bounds.push_back({b0, b1});
      b0 = b1;
    }
  }
  // INTERNEMENT GLOBAL PAR TRI (fold sort/reduce : les std::map chauds —
  // id_of global et roles par lot — portaient la pente ×2,8/doublement du
  // fold ; un tri unique des (facette, evenement, slot) les remplace).
  // Les fid sont attribues en ordre de FacetKey croissante ; first_batch
  // rend `existed` = « vue dans un lot STRICTEMENT anterieur » en O(1).
  struct FRec {
    FacetKey f;
    u32 e;
    u8 slot;  // < q : retrait de support ; >= q : retrait d'interieur
  };
  std::vector<FRec> recs;
  {
    size_t total = 0;
    for (const ForestEvent& ev : events) total += (size_t)ev.q + ev.d;
    recs.reserve(total);
  }
  for (size_t e = 0; e < events.size(); ++e) {
    const ForestEvent& ev = events[e];
    for (int s = 0; s < (int)ev.q; ++s)
      recs.push_back({detail_forest::facet_minus(ev, s, -1), (u32)e, (u8)s});
    for (int z = 0; z < (int)ev.d; ++z)
      recs.push_back(
          {detail_forest::facet_minus(ev, -1, z), (u32)e, (u8)(ev.q + z)});
  }
  std::stable_sort(recs.begin(), recs.end(),
                   [](const FRec& x, const FRec& y) { return x.f < y.f; });
  std::vector<FacetKey> keys;          // fid -> FacetKey (ordre croissant)
  std::vector<u32> first_batch;        // fid -> premier lot d'apparition
  std::vector<u32> ev_fid(events.size() * 11);  // (e, slot) -> fid
  for (size_t i = 0; i < recs.size(); ++i) {
    if (i == 0 || !(recs[i].f == recs[i - 1].f)) {
      keys.push_back(recs[i].f);
      first_batch.push_back(UINT32_MAX);
    }
    const u32 fid = (u32)keys.size() - 1;
    ev_fid[(size_t)recs[i].e * 11 + recs[i].slot] = fid;
    first_batch.back() =
        std::min(first_batch.back(), (u32)r.batch_of_event[recs[i].e]);
  }
  const size_t nfid = keys.size();
  r.facets = nfid;
  detail_forest::UnionFind uf;
  for (size_t i = 0; i < nfid; ++i) uf.add();
  // Identifiant canonique de composante : plus petite FacetKey, maintenu
  // incrementalement (creation + union).
  std::vector<FacetKey> canon_of = keys;
  const auto unite_canon = [&](i32 a, i32 b) {
    const i32 ra = uf.find(a), rb = uf.find(b);
    if (ra == rb) return false;
    const FacetKey mn = std::min(canon_of[(size_t)ra], canon_of[(size_t)rb]);
    uf.unite(ra, rb);
    canon_of[(size_t)uf.find(ra)] = mn;
    return true;
  };
  // Roles par lot en tableaux a EPOQUE (jamais une map par facette).
  constexpr u8 kActive = 1, kAttach = 2;
  std::vector<u32> role_epoch(nfid, UINT32_MAX);
  std::vector<u8> role_bits(nfid, 0);
  std::vector<u32> touched_fids;
  for (size_t b = 0; b < batch_bounds.size(); ++b) {
    const size_t e0 = batch_bounds[b].first, e1 = batch_bounds[b].second;
    // ROLES agreges par facette sur TOUT le lot (audit « facettes nees
    // dans le lot ») : une facette n'est un enfant du nœud que si elle est
    // ACTIVE (nee strictement avant le niveau) ou PREEXISTANTE ; les
    // attachements nes au lot restent dans la fermeture union-find mais
    // jamais dans `absorbed`.
    touched_fids.clear();
    const auto touch = [&](u32 fid, u8 bit) {
      if (role_epoch[fid] != (u32)b) {
        role_epoch[fid] = (u32)b;
        role_bits[fid] = 0;
        touched_fids.push_back(fid);
      }
      role_bits[fid] |= bit;
    };
    for (size_t e = e0; e < e1; ++e) {
      const ForestEvent& ev = events[e];
      for (int s = 0; s < (int)ev.q; ++s)
        touch(ev_fid[e * 11 + (size_t)s],
              ((ev.active_mask >> s) & 1u) ? kActive : kAttach);
      for (int z = 0; z < (int)ev.d; ++z)
        touch(ev_fid[e * 11 + (size_t)(ev.q + z)], kAttach);
    }
    for (const u32 fid : touched_fids) {
      const bool existed = first_batch[fid] < (u32)b;
      const bool active = role_bits[fid] & kActive;
      const bool attach = role_bits[fid] & kAttach;
      // Invariants (audit § 3) : un attachement deja vu, ou une facette a
      // la fois active et attachement au meme niveau, refutent la
      // coherence des rayons de naissance du flux.
      if (attach && existed) ++r.attach_violations;
      if (attach && active) ++r.birth_violations;
      if (attach && !existed && !active) ++r.new_attachments;
    }
    // Racines PRE-LOT : actives (leur rayon de naissance est strictement
    // inferieur, meme jamais rencontrees) OU preexistantes. MUTANT
    // attach-prebatch : l'ancienne convention fausse (tout le lot).
    std::map<i32, u64> prebatch_roots;
    for (const u32 fid : touched_fids)
      if (mutant_attach_prebatch || (role_bits[fid] & kActive) ||
          first_batch[fid] < (u32)b)
        prebatch_roots[uf.find((i32)fid)] = 0;
    // Canoniques PRE-LOT des racines gelees (avant toute union du lot).
    std::map<i32, FacetKey> pre_canon;
    for (const auto& pr : prebatch_roots)
      pre_canon.emplace(pr.first, canon_of[(size_t)pr.first]);
    // Unions du lot : chemin sur les K+1 facettes de chaque evenement.
    for (size_t e = e0; e < e1; ++e) {
      const ForestEvent& ev = events[e];
      i32 first = -1;
      for (int s = 0; s < (int)ev.q; ++s) {
        const i32 v = (i32)ev_fid[e * 11 + (size_t)s];
        if (first < 0) first = v;
        else if (unite_canon(first, v)) ++r.fusions;
      }
      for (int z = 0; z < (int)ev.d; ++z) {
        const i32 v = (i32)ev_fid[e * 11 + (size_t)(ev.q + z)];
        if (unite_canon(first, v)) ++r.fusions;
      }
    }
    // DELTAS du lot (le payload complet) : par racine post-lot touchee,
    // parents = canoniques pre-lot distincts, born = facettes nees
    // (attachment ∧ !existed ∧ !active) de cette composante. Emission des
    // que parents != 1 ou born non vide : naissance (0 parent), croissance
    // (1 parent + nees), (multi)fusion (>= 2). MUTANT drop-nonmerge :
    // l'ancienne sortie squelette (fusions seules).
    std::map<i32, ComponentDelta> touched;
    for (const auto& pr : prebatch_roots) {
      ComponentDelta& cd = touched[uf.find(pr.first)];
      cd.parents.push_back(pre_canon[pr.first]);
    }
    for (const u32 fid : touched_fids)
      if ((role_bits[fid] & kAttach) && !(role_bits[fid] & kActive) &&
          first_batch[fid] >= (u32)b)
        touched[uf.find((i32)fid)].born.push_back(keys[fid]);
    // Nœuds (vue derivee) : >= 2 parents.
    for (auto& tc : touched) {
      ComponentDelta& cd = tc.second;
      std::sort(cd.parents.begin(), cd.parents.end());
      std::sort(cd.born.begin(), cd.born.end());
      if (cd.parents.size() >= 2)
        r.nodes.push_back(ForestNode{(u64)b, cd.parents.size()});
      if (cd.parents.size() == 1 && cd.born.empty()) continue;  // continuation
      if (mutant_drop_nonmerge && cd.parents.size() < 2) continue;
      cd.batch = (u64)b;
      cd.level = events[e0].level;
      cd.output = canon_of[(size_t)uf.find(tc.first)];
      r.deltas.push_back(cd);
    }
    r.batch_levels.push_back(events[e0].level);
    if (snapshots) {
      // Instantane (petits n seulement) : les facettes DEJA VUES jusqu'au
      // lot courant inclus (first_batch <= b), en ordre de FacetKey — le
      // canonique du bloc est le min, deja maintenu dans canon_of.
      std::map<FacetKey, FacetKey> part;
      for (size_t fid = 0; fid < nfid; ++fid)
        if (first_batch[fid] <= (u32)b)
          part[keys[fid]] = canon_of[(size_t)uf.find((i32)fid)];
      snapshots->push_back(std::move(part));
    }
    ++r.batches;
  }
  for (size_t fid = 0; fid < nfid; ++fid)
    r.final_partition.emplace_hint(r.final_partition.end(), keys[fid],
                                   canon_of[(size_t)uf.find((i32)fid)]);
  return r;
}


// Construit la foret K a partir d'evenements DEJA filtres pour ce K
// (q + d = K + 1). Les evenements sont tries ici par niveau exact.
// `snapshots` (optionnel) : partition canonique apres CHAQUE macro-lot
// (facette -> plus petite facette de sa composante), pour le juge.
inline ForestResult build_forest(
    const std::vector<ForestEvent>& events, bool mutant_binary_ties = false,
    bool mutant_repr_equality = false,
    std::vector<std::map<FacetKey, FacetKey>>* snapshots = nullptr,
    bool mutant_attach_prebatch = false,
    bool mutant_drop_nonmerge = false, bool mutant_canon_root = false,
    bool fill_legacy_partition = true, u64 cap_base_events = 0,
    u64 cap_base_recs = 0, u64 cap_base_batches = 0,
    int mutant_capacity = 0, int mutant_intern = 0, int intern_mode = 0,
    int mutant_detector = 0) {
  // Mutants du DETECTEUR (reponse d'audit `95061c1` § 2) : 1 = detecteur
  // desactive ; 2 = `seen` marque AVANT le controle du lot.
  const bool mutant_no_detector = mutant_detector == 1;
  const bool mutant_seen_before = mutant_detector == 2;
  ForestResult r;
  // GARDE DE CAPACITE TRANSACTIONNELLE (contre-audit 5d274a1 § 7) : les
  // index LOCAUX du fold sont volontairement etroits (FRec::e en u32,
  // fid compatible i32 pour l'union-find, epoques u32 a SENTINELLE
  // UINT32_MAX) — c'est le bon choix pour les futures tuiles, mais toute
  // entree qui ne tient pas doit etre REFUSEE avant les casts et les
  // allocations, jamais tronquee. Majorants VERIFIABLES A L'ENTREE :
  // nfid <= Σ(q_e + d_e) (l'internement dedoublonne, jamais n'ajoute) et
  // lots <= evenements ; le lot b doit rester STRICTEMENT sous la
  // sentinelle UINT32_MAX des tableaux a epoque. Les bases cap_base_*
  // (fictives, portes seulement) approchent les limites sans allocation
  // geante ; les trois mutants degradent chacun UNE comparaison.
  u64 total_recs = 0;
  {
    for (const ForestEvent& ev : events) total_recs += (u64)ev.q + ev.d;
    const u64 nev = cap_base_events + (u64)events.size();
    const u64 nrec = cap_base_recs + total_recs;
    const u64 nbat = cap_base_batches + (u64)events.size();
    // MUTANTS 1 et 2 : le compte passe par u32 AVANT la comparaison
    // (enroulement) — la verification elle-meme reste la meme.
    const u64 nev_eff = mutant_capacity == 1 ? (u64)(u32)nev : nev;
    const u64 nrec_eff = mutant_capacity == 2 ? (u64)(u32)nrec : nrec;
    const bool ok_events = nev_eff <= (u64)UINT32_MAX;
    const bool ok_fid = nrec_eff <= (u64)INT32_MAX;
    const bool ok_batches = mutant_capacity == 3
                                ? nbat <= (u64)UINT32_MAX  // MUTANT : == ok
                                : nbat < (u64)UINT32_MAX;
    if (!ok_events || !ok_fid || !ok_batches) {
      r.refusal = "resource_exhausted/requires_tiling";
      return r;
    }
  }
  auto tf = std::chrono::steady_clock::now();
  const auto mark = [&](double* out) {
    const auto now = std::chrono::steady_clock::now();
    *out += std::chrono::duration<double, std::milli>(now - tf).count();
    tf = now;
  };
  // PERMUTATION COMPACTE plutot qu'une copie des enregistrements
  // (reponse d'audit `95061c1` § 3, etape prealable) : `build_forest`
  // recevait ses evenements PAR VALEUR pour les trier, donc chaque fold
  // concurrent payait une copie complete de `ForestEvent` (~130 octets
  // par evenement, ~400 Mo cumules a n=8000). Un tableau d'indices u32
  // suffit : le contrat public est intact — `batch_of_event` et `ev_fid`
  // restent indexes par la POSITION DANS L'ORDRE TRIE, exactement comme
  // avant.
  std::vector<u32> order(events.size());
  for (size_t i = 0; i < order.size(); ++i) order[i] = (u32)i;
  std::stable_sort(order.begin(), order.end(), [&](u32 x, u32 y) {
    return compare_exact_level(events[x].level, events[y].level) < 0;
  });
  const auto evt = [&](size_t i) -> const ForestEvent& {
    return events[(size_t)order[i]];
  };
  // LOTS d'abord (independants des unions) : plages de niveaux
  // semantiquement egaux. MUTANTS : ties binaires (lot force a un seul
  // evenement) ou egalite de REPRESENTATION (le piege grave par l'audit :
  // deux representants differents du meme rationnel brisent le lot a tort).
  r.batch_of_event.assign(events.size(), 0);
  std::vector<std::pair<size_t, size_t>> batch_bounds;
  {
    size_t b0 = 0;
    while (b0 < events.size()) {
      size_t b1 = b0 + 1;
      while (b1 < events.size()) {
        const bool same =
            mutant_repr_equality
                ? same_level_representation(evt(b1).level, evt(b0).level)
                : same_exact_level(evt(b1).level, evt(b0).level);
        if (mutant_binary_ties || !same) break;
        ++b1;
      }
      for (size_t e = b0; e < b1; ++e)
        r.batch_of_event[e] = batch_bounds.size();
      batch_bounds.push_back({b0, b1});
      b0 = b1;
    }
  }
  mark(&r.t_batching_ms);
  // INTERNEMENT EN STREAMING (reponse d'audit « fold compact » § 3 —
  // remplace le tri global des (facette, evenement, slot)). Le tableau
  // de records pesait 52 octets par incidence et `stable_sort` en
  // reclamait autant en tampon de fusion : a n=8000 c'etait 26,65 M
  // records pour 19,47 M facettes uniques (facteur de dedoublonnage
  // 1,37 seulement) — on payait 2 x 52 octets de trafic memoire par
  // incidence pour trier 1,37 fois plus d'elements que la sortie n'en
  // contient. Ici chaque facette est internee A LA VOLEE (table
  // d'adressage ouvert, comparaison EXACTE de cle), puis les cles
  // UNIQUES — et elles seules — sont triees : le tri porte exactement
  // sur la sortie, et aucun tampon de records n'existe.
  //
  // INVARIANT PUBLIC INCHANGE ET INDEPENDANT DU HACHAGE : les fid sont
  // attribues en ordre de FacetKey croissante (le tri final est la
  // seule autorite d'ordre — l'empreinte et le sondage n'entrent dans
  // aucune sortie). MUTANTS : 1 = fid en ordre de rencontre (le tri
  // saute) ; 2 = appartenance sur huit bits d'empreinte sans
  // comparaison de cle.
  //
  // L'internement ne porte PLUS le premier lot de chaque facette
  // (reponse d'audit `95061c1` § 2) : sur un flux sans
  // `attach_violations`, `S_b(f) => A_b(f)`, donc `active ∨ existed =
  // active` et `attach ∧ ¬active ∧ ¬existed = attach ∧ ¬active` — la
  // semantique n'a jamais eu besoin de cette donnee. Elle est remplacee
  // par un bit `seen` EN LIGNE, mis a jour APRES le lot, qui ne sert
  // qu'au detecteur. Gain concret : un u32 par facette unique
  // (74 MiB touches a n=8000) et une ecriture ALEATOIRE de moins par
  // sondage reussi du chemin chaud.
  std::vector<FacetKey> keys;          // fid -> FacetKey (ordre croissant)
  std::vector<u32> ev_fid(events.size() * 11);  // (e, slot) -> fid
  if (intern_mode == 1) {
    // MODE 1 — INTERNEMENT PAR TRI GLOBAL (l'historique, conserve comme
    // COMPARANDE mesurable dans le MEME processus). Il materialise un
    // record de 52 octets par incidence et `stable_sort` en reclame
    // autant en tampon de fusion ; son acces est sequentiel, celui du
    // mode 0 est aleatoire — sur un conteneur ou le fold varie de ±40 %
    // d'un processus a l'autre, seule une alternance intra-processus
    // (`--fold-intern-bench`) permet de les departager. Les deux modes
    // doivent produire des sorties IDENTIQUES (porte a trois backends).
    struct FRec {
      FacetKey f;
      u32 e;
      u8 slot;
    };
    std::vector<FRec> recs;
    recs.reserve((size_t)total_recs);
    for (size_t e = 0; e < events.size(); ++e) {
      const ForestEvent& ev = evt(e);
      for (int s = 0; s < (int)ev.q; ++s)
        recs.push_back({detail_forest::facet_minus(ev, s, -1), (u32)e, (u8)s});
      for (int z = 0; z < (int)ev.d; ++z)
        recs.push_back(
            {detail_forest::facet_minus(ev, -1, z), (u32)e, (u8)(ev.q + z)});
    }
    std::stable_sort(recs.begin(), recs.end(),
                     [](const FRec& x, const FRec& y) { return x.f < y.f; });
    mark(&r.t_intern_scan_ms);
    for (size_t i = 0; i < recs.size(); ++i) {
      if (i == 0 || !(recs[i].f == recs[i - 1].f)) keys.push_back(recs[i].f);
      ev_fid[(size_t)recs[i].e * 11 + recs[i].slot] = (u32)keys.size() - 1;
    }
    mark(&r.t_intern_remap_ms);
  } else {
    detail_forest::FacetIntern in((size_t)total_recs);
    in.no_verify = mutant_intern == 2;
    // PIPELINE LOGICIEL : un bloc de 48 incidences calcule d'abord ses
    // cles et ses empreintes (tout tient en L1) en PRECHARGEANT les
    // cases visees, puis sonde. Sans lui le balayage est une chaine de
    // defauts de cache et de TLB dependants ; la capacite de la table
    // etant figee a l'entree, l'adresse prechargee est celle qui sera
    // sondee. La semantique est inchangee : meme suite d'appels, meme
    // ordre.
    {
      constexpr size_t kBlock = 48;
      struct Pending {
        FacetKey f;
        u64 h;
        size_t pos;  // e*11+slot : jamais un u32 (e va jusqu'a UINT32_MAX)
      };
      std::array<Pending, kBlock> pend{};
      size_t np = 0;
      const auto flush = [&]() {
        for (size_t i = 0; i < np; ++i)
          ev_fid[pend[i].pos] = in.intern_hashed(pend[i].f, pend[i].h);
        np = 0;
      };
      const auto push = [&](const FacetKey& f, size_t pos) {
        Pending& p = pend[np];
        p.f = f;
        p.h = detail_forest::facet_fingerprint(f);
        p.pos = pos;
        in.prefetch(p.h);
        if (++np == kBlock) flush();
      };
      for (size_t e = 0; e < events.size(); ++e) {
        const ForestEvent& ev = evt(e);
        for (int s = 0; s < (int)ev.q; ++s)
          push(detail_forest::facet_minus(ev, s, -1), e * 11 + (size_t)s);
        for (int z = 0; z < (int)ev.d; ++z)
          push(detail_forest::facet_minus(ev, -1, z),
               e * 11 + (size_t)(ev.q + z));
      }
      flush();
    }
    mark(&r.t_intern_scan_ms);
    // Table liberee AVANT le tri : le pic memoire du fold ne cumule
    // jamais l'index d'internement et la sortie.
    std::vector<u64>().swap(in.table);
    if (mutant_intern != 1)
      std::sort(in.pool.begin(), in.pool.end(),
                [](const std::pair<FacetKey, u32>& x,
                   const std::pair<FacetKey, u32>& y) {
                  return detail_forest::facet_less_k(x.first, y.first);
                });
    mark(&r.t_intern_sort_ms);
    const size_t np = in.pool.size();
    keys.resize(np);
    std::vector<u32> rank(np);
    for (size_t i = 0; i < np; ++i) {
      keys[i] = in.pool[i].first;
      rank[(size_t)in.pool[i].second] = (u32)i;
    }
    std::vector<std::pair<FacetKey, u32>>().swap(in.pool);
    // Retour des identifiants temporaires vers les fid publies.
    for (size_t e = 0; e < events.size(); ++e) {
      const ForestEvent& ev = evt(e);
      for (int t = 0; t < (int)ev.q + (int)ev.d; ++t) {
        u32& slot = ev_fid[e * 11 + (size_t)t];
        slot = rank[(size_t)slot];
      }
    }
  }
  const size_t nfid = keys.size();
  r.facets = nfid;
  mark(&r.t_intern_remap_ms);
  r.t_intern_ms =
      r.t_intern_scan_ms + r.t_intern_sort_ms + r.t_intern_remap_ms;
  detail_forest::UnionFind uf;
  for (size_t i = 0; i < nfid; ++i) uf.add();
  // CANONIQUE PAR MIN-FID (fold compact § 1.1) : les fid sont attribues
  // en ordre de FacetKey croissante, donc fid1 < fid2 <=> keys[fid1] <
  // keys[fid2] — le canonique (plus petite FacetKey de la composante)
  // est keys[min fid] et l'union compare deux u32, jamais deux
  // FacetKey. MUTANT canonical-is-uf-root : le canonique suit la racine
  // union-find au lieu du minimum.
  std::vector<u32> canon_fid(nfid);
  for (size_t i = 0; i < nfid; ++i) canon_fid[i] = (u32)i;
  const auto unite_canon = [&](i32 a, i32 b) {
    const i32 ra = uf.find(a), rb = uf.find(b);
    if (ra == rb) return false;
    const u32 mn = mutant_canon_root
                       ? canon_fid[(size_t)ra]  // MUTANT : biais racine
                       : std::min(canon_fid[(size_t)ra],
                                  canon_fid[(size_t)rb]);
    uf.unite(ra, rb);
    canon_fid[(size_t)uf.find(ra)] = mn;
    return true;
  };
  // Roles par lot en tableaux a EPOQUE (jamais une map par facette).
  constexpr u8 kActive = 1, kAttach = 2;
  std::vector<u32> role_epoch(nfid, UINT32_MAX);
  std::vector<u8> role_bits(nfid, 0);
  std::vector<u32> touched_fids;
  // RACINES pre-lot et post-lot en TABLEAUX A EPOQUE (§ 1.2 : plus
  // aucune map par lot). Les listes sont TRIEES avant materialisation
  // pour conserver l'ordre observable historique (racines croissantes) ;
  // le brouillon de deltas est REUTILISE entre les lots (capacites
  // conservees — les seules allocations restantes sont celles des
  // deltas effectivement EMIS, CSR en reserve si la mesure l'exige).
  // Bit `seen` : facette rencontree dans un lot STRICTEMENT anterieur.
  // Instrument du detecteur, jamais une entree du calcul.
  std::vector<u8> seen(nfid, 0);
  std::vector<u32> pre_epoch(nfid, UINT32_MAX), post_epoch(nfid, UINT32_MAX);
  std::vector<u32> pre_canon_fid(nfid), post_slot(nfid);
  std::vector<i32> pre_list, post_list;
  std::vector<ComponentDelta> scratch;
  for (size_t b = 0; b < batch_bounds.size(); ++b) {
    const size_t e0 = batch_bounds[b].first, e1 = batch_bounds[b].second;
    // ROLES agreges par facette sur TOUT le lot (audit « facettes nees
    // dans le lot ») : une facette n'est un enfant du nœud que si elle est
    // ACTIVE (nee strictement avant le niveau) ou PREEXISTANTE ; les
    // attachements nes au lot restent dans la fermeture union-find mais
    // jamais dans `absorbed`.
    touched_fids.clear();
    const auto touch = [&](u32 fid, u8 bit) {
      if (role_epoch[fid] != (u32)b) {
        role_epoch[fid] = (u32)b;
        role_bits[fid] = 0;
        touched_fids.push_back(fid);
      }
      role_bits[fid] |= bit;
    };
    for (size_t e = e0; e < e1; ++e) {
      const ForestEvent& ev = evt(e);
      for (int s = 0; s < (int)ev.q; ++s)
        touch(ev_fid[e * 11 + (size_t)s],
              ((ev.active_mask >> s) & 1u) ? kActive : kAttach);
      for (int z = 0; z < (int)ev.d; ++z)
        touch(ev_fid[e * 11 + (size_t)(ev.q + z)], kAttach);
    }
    // DETECTEUR SEPARE DE LA SEMANTIQUE (reponse d'audit `95061c1` § 2).
    // `seen` n'entre dans AUCUNE decision du fold : il ne sert qu'aux
    // deux compteurs, et il est mis a jour APRES le lot (le lot courant
    // n'est pas « anterieur »). MUTANTS : `attach-detector-disabled`
    // (le detecteur ne compte plus rien — la fixture de flux incoherent
    // le tue) ; `seen-before-check` (mise a jour AVANT le controle —
    // tous les premiers attachements sont alors signales a tort, et les
    // familles geometriques regulieres le tuent).
    if (mutant_seen_before)
      for (const u32 fid : touched_fids) seen[fid] = 1;
    for (const u32 fid : touched_fids) {
      const bool active = role_bits[fid] & kActive;
      const bool attach = role_bits[fid] & kAttach;
      // Invariants (audit § 3) : un attachement deja vu, ou une facette a
      // la fois active et attachement au meme niveau, refutent la
      // coherence des rayons de naissance du flux.
      if (!mutant_no_detector) {
        if (attach && seen[fid]) ++r.attach_violations;
        if (attach && active) ++r.birth_violations;
      }
      if (attach && !active) ++r.new_attachments;
    }
    // Racines PRE-LOT : les ACTIVES, point (leur rayon de naissance est
    // strictement inferieur, meme jamais rencontrees). Le terme
    // « preexistante » etait redondant : sans `attach_violations`,
    // `S_b(f) => A_b(f)`. MUTANT attach-prebatch : l'ancienne convention
    // fausse (tout le lot).
    pre_list.clear();
    for (const u32 fid : touched_fids)
      if (mutant_attach_prebatch || (role_bits[fid] & kActive)) {
        const i32 pr = uf.find((i32)fid);
        if (pre_epoch[(size_t)pr] != (u32)b) {
          pre_epoch[(size_t)pr] = (u32)b;
          // Canonique PRE-LOT gele AVANT toute union du lot.
          pre_canon_fid[(size_t)pr] = canon_fid[(size_t)pr];
          pre_list.push_back(pr);
        }
      }
    std::sort(pre_list.begin(), pre_list.end());
    // Unions du lot : chemin sur les K+1 facettes de chaque evenement.
    for (size_t e = e0; e < e1; ++e) {
      const ForestEvent& ev = evt(e);
      i32 first = -1;
      for (int s = 0; s < (int)ev.q; ++s) {
        const i32 v = (i32)ev_fid[e * 11 + (size_t)s];
        if (first < 0) first = v;
        else if (unite_canon(first, v)) ++r.fusions;
      }
      for (int z = 0; z < (int)ev.d; ++z) {
        const i32 v = (i32)ev_fid[e * 11 + (size_t)(ev.q + z)];
        if (unite_canon(first, v)) ++r.fusions;
      }
    }
    // DELTAS du lot (le payload complet) : par racine post-lot touchee,
    // parents = canoniques pre-lot distincts, born = facettes nees
    // (attachment ∧ !existed ∧ !active) de cette composante. Emission des
    // que parents != 1 ou born non vide : naissance (0 parent), croissance
    // (1 parent + nees), (multi)fusion (>= 2). MUTANT drop-nonmerge :
    // l'ancienne sortie squelette (fusions seules).
    post_list.clear();
    const auto post_of = [&](i32 rt) -> ComponentDelta& {
      if (post_epoch[(size_t)rt] != (u32)b) {
        post_epoch[(size_t)rt] = (u32)b;
        post_slot[(size_t)rt] = (u32)post_list.size();
        post_list.push_back(rt);
        if (scratch.size() < post_list.size()) scratch.emplace_back();
        ComponentDelta& cd = scratch[post_list.size() - 1];
        cd.parents.clear();
        cd.born.clear();
        return cd;
      }
      return scratch[post_slot[(size_t)rt]];
    };
    for (const i32 pr : pre_list)
      post_of(uf.find(pr)).parents.push_back(keys[pre_canon_fid[(size_t)pr]]);
    for (const u32 fid : touched_fids)
      if ((role_bits[fid] & kAttach) && !(role_bits[fid] & kActive))
        post_of(uf.find((i32)fid)).born.push_back(keys[fid]);
    // Emission en ordre de racine croissante (l'ordre observable
    // historique de la map) ; nœuds (vue derivee) : >= 2 parents.
    std::sort(post_list.begin(), post_list.end());
    for (const i32 rt : post_list) {
      ComponentDelta& cd = scratch[post_slot[(size_t)rt]];
      std::sort(cd.parents.begin(), cd.parents.end());
      std::sort(cd.born.begin(), cd.born.end());
      if (cd.parents.size() >= 2)
        r.nodes.push_back(ForestNode{(u64)b, cd.parents.size()});
      if (cd.parents.size() == 1 && cd.born.empty()) continue;  // continuation
      if (mutant_drop_nonmerge && cd.parents.size() < 2) continue;
      cd.batch = (u64)b;
      cd.level = evt(e0).level;
      cd.output = keys[canon_fid[(size_t)uf.find(rt)]];
      r.deltas.push_back(cd);
    }
    r.batch_levels.push_back(evt(e0).level);
    // FIN DE LOT : le lot courant devient « anterieur ». `seen` ne
    // gouverne rien avant ce point.
    if (!mutant_seen_before)
      for (const u32 fid : touched_fids) seen[fid] = 1;
    if (snapshots) {
      // Instantane (petits n seulement) : les facettes DEJA VUES jusqu'au
      // lot courant INCLUS — `seen` vient d'etre mis a jour — en ordre de
      // FacetKey ; le canonique du bloc est le min, deja maintenu.
      std::map<FacetKey, FacetKey> part;
      for (size_t fid = 0; fid < nfid; ++fid)
        if (seen[fid])
          part[keys[fid]] = keys[canon_fid[(size_t)uf.find((i32)fid)]];
      snapshots->push_back(std::move(part));
    }
    ++r.batches;
  }
  mark(&r.t_reduce_ms);
  // PARTITION FINALE DENSE + invariants structurels PERMANENTS
  // (§ 2.2 : cles strictement croissantes, canon <= fid, idempotence).
  r.final_canon_fid.resize(nfid);
  for (size_t fid = 0; fid < nfid; ++fid) {
    const u32 c = canon_fid[(size_t)uf.find((i32)fid)];
    r.final_canon_fid[fid] = c;
    if (c > (u32)fid || r.final_canon_fid[(size_t)c] != c)
      ++r.partition_violations;
  }
  for (size_t fid = 1; fid < nfid; ++fid)
    if (!(keys[fid - 1] < keys[fid])) ++r.partition_violations;
  if (fill_legacy_partition)
    for (size_t fid = 0; fid < nfid; ++fid)
      r.final_partition.emplace_hint(
          r.final_partition.end(), keys[fid],
          keys[(size_t)r.final_canon_fid[fid]]);
  r.facet_keys = std::move(keys);
  mark(&r.t_partition_ms);
  return r;
}

}  // namespace mhgp4
