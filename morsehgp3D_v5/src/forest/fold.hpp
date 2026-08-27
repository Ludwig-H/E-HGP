// MorseHGP3D v5 — fold d'un ordre K : de la liste d'evenements a la foret.
//
// Objet (docs/MATHEMATIQUES.md § 5) : pour K fixe, les sommets sont les
// facettes (K-uplets tries de PointId), chaque evenement σ = T ∪ I (|T| = q,
// |I| = d, K = q + d − 1, niveau exact) unionne ses K+1 facettes. La foret
// est le K-MST elague (Theoreme 5), rendue en macro-lots :
//   - LOTS : plages de niveaux SEMANTIQUEMENT egaux (`same_exact_level`,
//     jamais l'egalite de representation — mutant `repr-ties`) ; un lot
//     n'est jamais scinde en chronologie binaire (mutant `binary-ties`) ;
//   - ROLES : une facette σ∖{v} est ACTIVE (nee strictement avant) ou un
//     ATTACHEMENT (nait au niveau) ; les enfants d'un nœud sont les racines
//     pre-lot des ACTIVES seulement (mutant `attach-prebatch`) ;
//   - DELTAS : par racine post-lot touchee, parents = canoniques pre-lot
//     distincts, born = facettes nees (attachement ∧ ¬active) ; emis des que
//     parents != 1 ou born non vide : naissance, croissance, (multi)fusion
//     (mutant `drop-nonmerge` : fusions seules) ;
//   - CANONIQUE = plus petite FacetKey de la composante = min-fid, les fid
//     etant attribues en ordre de FacetKey croissante (mutant
//     `canonical-is-uf-root`) ;
//   - INVARIANTS mesures (toujours 0) : attachement deja vu dans un lot
//     anterieur, facette active et attachement au meme lot, partition
//     dense non triee / non idempotente.
//
// Internement en streaming : table d'adressage ouvert dimensionnee UNE fois
// sur le majorant des incidences (jamais de rehachage), appartenance par
// comparaison EXACTE de cle, puis tri des cles UNIQUES seules — le tri final
// est la seule autorite d'ordre, l'empreinte n'entre dans aucune sortie.
//
// Garde de capacite transactionnelle : evenements <= UINT32_MAX,
// Σ(q+d) <= INT32_MAX, lots < UINT32_MAX — refus AVANT toute allocation,
// jamais une troncature.
#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "../core/mutants.hpp"
#include "../lanes/keys.hpp"
#include "../lanes/level.hpp"

namespace mhgp5 {

struct ForestEvent {
  u8 q = 0;               // |T| <= 11
  u8 d = 0;               // |I| <= 9
  u16 active_mask = 0;    // bit t : σ∖{T[t]} active
  PointId support[11] = {};
  PointId interior[9] = {};
  ExactLevel level{};
};

struct ComponentDelta {
  u64 batch = 0;
  ExactLevel level{};
  FacetKey output;
  std::vector<FacetKey> parents;  // tries
  std::vector<FacetKey> born;     // triees
};

struct ForestResult {
  std::string refusal;                 // non vide = refus AVANT allocation
  u64 facets = 0, fusions = 0, batches = 0, new_attachments = 0;
  u64 attach_violations = 0, birth_violations = 0, partition_violations = 0;
  u64 nodes = 0;                       // deltas a >= 2 parents (vue derivee)
  std::vector<FacetKey> facet_keys;    // fid -> FacetKey, strictement croissante
  std::vector<u32> final_canon_fid;    // fid -> plus petit fid de sa composante
  std::vector<ComponentDelta> deltas;  // le payload hierarchique complet
  std::vector<ExactLevel> batch_levels;
  double t_intern_ms = 0, t_reduce_ms = 0;
};

namespace fold_detail {

inline FacetKey facet_minus(const ForestEvent& e, int drop_support, int drop_interior) {
  FacetKey f;
  for (int t = 0; t < (int)e.q; ++t)
    if (t != drop_support) f.p[f.k++] = e.support[t];
  for (int t = 0; t < (int)e.d; ++t)
    if (t != drop_interior) f.p[f.k++] = e.interior[t];
  for (u8 t = 1; t < f.k; ++t) {  // insertion (k <= 10)
    const PointId v = f.p[t];
    u8 w = t;
    for (; w > 0 && f.p[w - 1] > v; --w) f.p[w] = f.p[w - 1];
    f.p[w] = v;
  }
  return f;
}

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

// Empreinte d'adressage seulement : l'appartenance est tranchee par la cle.
inline u64 facet_fingerprint(const FacetKey& f) {
  u64 h = 0x9E3779B97F4A7C15ull ^ (u64)f.k;
  for (u8 i = 0; i < f.k; ++i) h = (h + (u64)f.p[i]) * 0x9E3779B97F4A7C15ull;
  h ^= h >> 31;
  h *= 0xBF58476D1CE4E5B9ull;
  h ^= h >> 27;
  h *= 0x94D049BB133111EBull;
  h ^= h >> 31;
  return h;
}

struct FacetIntern {
  static constexpr u64 kTag = 0xFFFFFFFF00000000ull;
  std::vector<u64> table;                      // empreinte<<32 | tid+1 ; 0 = vide
  std::vector<std::pair<FacetKey, u32>> pool;  // tid -> (cle, tid)
  size_t mask = 0;
  explicit FacetIntern(size_t incidences) {
    size_t cap = 1024;
    while (cap < incidences * 2 + 2) cap <<= 1;
    table.assign(cap, 0);
    mask = cap - 1;
    pool.reserve(incidences);
  }
  void prefetch(u64 h) const {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(&table[(size_t)h & mask], 1, 1);
#else
    (void)h;
#endif
  }
  u32 intern_hashed(const FacetKey& f, u64 h) {
    const u64 tag = h & kTag;
    size_t i = (size_t)h & mask;
    while (table[i] != 0) {
      const u32 tid = (u32)(table[i] & 0xFFFFFFFFull) - 1;
      if ((table[i] & kTag) == tag && facet_equal_k(pool[(size_t)tid].first, f)) return tid;
      i = (i + 1) & mask;
    }
    const u32 tid = (u32)pool.size();
    pool.push_back({f, tid});
    table[i] = tag | (u64)(tid + 1);
    return tid;
  }
};

struct UnionFind {
  std::vector<i32> parent;
  explicit UnionFind(size_t n) : parent(n) {
    for (size_t i = 0; i < n; ++i) parent[i] = (i32)i;
  }
  i32 find(i32 v) {
    while (parent[(size_t)v] != v) {
      parent[(size_t)v] = parent[(size_t)parent[(size_t)v]];
      v = parent[(size_t)v];
    }
    return v;
  }
};

}  // namespace fold_detail

// `order` : permutation des evenements triee par niveau exact (calculee par
// l'appelant, qui peut la partager avec le rendu).
inline std::vector<u32> sort_events_by_level(const std::vector<ForestEvent>& events) {
  std::vector<u32> order(events.size());
  for (size_t i = 0; i < order.size(); ++i) order[i] = (u32)i;
  std::stable_sort(order.begin(), order.end(),
                   [&](u32 x, u32 y) { return compare_exact_level(events[x].level, events[y].level) < 0; });
  return order;
}

inline ForestResult build_forest(const std::vector<ForestEvent>& events) {
  using namespace fold_detail;
  ForestResult r;
  const bool m_binary = MHGP5_MUTANT("binary-ties");
  const bool m_repr = MHGP5_MUTANT("repr-ties");
  const bool m_attach_pre = MHGP5_MUTANT("attach-prebatch");
  const bool m_drop_nonmerge = MHGP5_MUTANT("drop-nonmerge");
  const bool m_canon_root = MHGP5_MUTANT("canonical-is-uf-root");
  const bool m_no_detector = MHGP5_MUTANT("attach-detector-disabled");

  // Garde de capacite AVANT toute allocation.
  u64 total_recs = 0;
  for (const ForestEvent& ev : events) total_recs += (u64)ev.q + ev.d;
  if ((u64)events.size() > (u64)UINT32_MAX || total_recs > (u64)INT32_MAX || (u64)events.size() >= (u64)UINT32_MAX) {
    r.refusal = "resource_exhausted/requires_tiling";
    return r;
  }
  const std::vector<u32> order = sort_events_by_level(events);
  const auto evt = [&](size_t i) -> const ForestEvent& { return events[(size_t)order[i]]; };

  // Lots.
  std::vector<std::pair<size_t, size_t>> batches;
  for (size_t b0 = 0; b0 < events.size();) {
    size_t b1 = b0 + 1;
    while (b1 < events.size() && !m_binary) {
      const bool same = m_repr ? (evt(b1).level == evt(b0).level) : same_exact_level(evt(b1).level, evt(b0).level);
      if (!same) break;
      ++b1;
    }
    batches.push_back({b0, b1});
    b0 = b1;
  }

  // Internement en streaming, bloc de 48 avec prefetch.
  std::vector<FacetKey> keys;
  std::vector<u32> ev_fid(events.size() * 11);
  {
    FacetIntern in((size_t)total_recs);
    constexpr size_t kBlock = 48;
    struct Pending { FacetKey f; u64 h; size_t pos; };
    std::array<Pending, kBlock> pend{};
    size_t np = 0;
    const auto flush = [&]() {
      for (size_t i = 0; i < np; ++i) ev_fid[pend[i].pos] = in.intern_hashed(pend[i].f, pend[i].h);
      np = 0;
    };
    const auto push = [&](const FacetKey& f, size_t pos) {
      Pending& p = pend[np];
      p.f = f;
      p.h = facet_fingerprint(f);
      p.pos = pos;
      in.prefetch(p.h);
      if (++np == kBlock) flush();
    };
    for (size_t e = 0; e < events.size(); ++e) {
      const ForestEvent& ev = evt(e);
      for (int s = 0; s < (int)ev.q; ++s) push(facet_minus(ev, s, -1), e * 11 + (size_t)s);
      for (int z = 0; z < (int)ev.d; ++z) push(facet_minus(ev, -1, z), e * 11 + (size_t)(ev.q + z));
    }
    flush();
    std::vector<u64>().swap(in.table);  // liberee AVANT le tri
    std::sort(in.pool.begin(), in.pool.end(),
              [](const std::pair<FacetKey, u32>& x, const std::pair<FacetKey, u32>& y) {
                return facet_less_k(x.first, y.first);
              });
    const size_t np2 = in.pool.size();
    keys.resize(np2);
    std::vector<u32> rank(np2);
    for (size_t i = 0; i < np2; ++i) {
      keys[i] = in.pool[i].first;
      rank[(size_t)in.pool[i].second] = (u32)i;
    }
    std::vector<std::pair<FacetKey, u32>>().swap(in.pool);
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

  UnionFind uf(nfid);
  std::vector<u32> canon_fid(nfid);
  for (size_t i = 0; i < nfid; ++i) canon_fid[i] = (u32)i;
  const auto unite_canon = [&](i32 a, i32 b) {
    const i32 ra = uf.find(a), rb = uf.find(b);
    if (ra == rb) return false;
    const u32 mn = m_canon_root ? canon_fid[(size_t)ra] : std::min(canon_fid[(size_t)ra], canon_fid[(size_t)rb]);
    uf.parent[(size_t)rb] = ra;
    canon_fid[(size_t)ra] = mn;
    return true;
  };
  constexpr u8 kActive = 1, kAttach = 2;
  std::vector<u32> role_epoch(nfid, UINT32_MAX);
  std::vector<u8> role_bits(nfid, 0), seen(nfid, 0);
  std::vector<u32> pre_epoch(nfid, UINT32_MAX), post_epoch(nfid, UINT32_MAX);
  std::vector<u32> pre_canon_fid(nfid), post_slot(nfid);
  std::vector<u32> touched;
  std::vector<i32> pre_list, post_list;
  std::vector<ComponentDelta> scratch;
  for (size_t b = 0; b < batches.size(); ++b) {
    const size_t e0 = batches[b].first, e1 = batches[b].second;
    touched.clear();
    const auto touch = [&](u32 fid, u8 bit) {
      if (role_epoch[fid] != (u32)b) {
        role_epoch[fid] = (u32)b;
        role_bits[fid] = 0;
        touched.push_back(fid);
      }
      role_bits[fid] |= bit;
    };
    for (size_t e = e0; e < e1; ++e) {
      const ForestEvent& ev = evt(e);
      for (int s = 0; s < (int)ev.q; ++s) touch(ev_fid[e * 11 + (size_t)s], ((ev.active_mask >> s) & 1u) ? kActive : kAttach);
      for (int z = 0; z < (int)ev.d; ++z) touch(ev_fid[e * 11 + (size_t)(ev.q + z)], kAttach);
    }
    for (const u32 fid : touched) {
      const bool active = role_bits[fid] & kActive;
      const bool attach = role_bits[fid] & kAttach;
      if (!m_no_detector) {
        if (attach && seen[fid]) ++r.attach_violations;
        if (attach && active) ++r.birth_violations;
      }
      if (attach && !active) ++r.new_attachments;
    }
    pre_list.clear();
    for (const u32 fid : touched)
      if (m_attach_pre || (role_bits[fid] & kActive)) {
        const i32 pr = uf.find((i32)fid);
        if (pre_epoch[(size_t)pr] != (u32)b) {
          pre_epoch[(size_t)pr] = (u32)b;
          pre_canon_fid[(size_t)pr] = canon_fid[(size_t)pr];
          pre_list.push_back(pr);
        }
      }
    std::sort(pre_list.begin(), pre_list.end());
    for (size_t e = e0; e < e1; ++e) {
      const ForestEvent& ev = evt(e);
      i32 first = -1;
      for (int s = 0; s < (int)ev.q; ++s) {
        const i32 v = (i32)ev_fid[e * 11 + (size_t)s];
        if (first < 0) first = v;
        else if (unite_canon(first, v)) ++r.fusions;
      }
      for (int z = 0; z < (int)ev.d; ++z)
        if (unite_canon(first, (i32)ev_fid[e * 11 + (size_t)(ev.q + z)])) ++r.fusions;
    }
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
    for (const i32 pr : pre_list) post_of(uf.find(pr)).parents.push_back(keys[pre_canon_fid[(size_t)pr]]);
    for (const u32 fid : touched)
      if ((role_bits[fid] & kAttach) && !(role_bits[fid] & kActive)) post_of(uf.find((i32)fid)).born.push_back(keys[fid]);
    std::sort(post_list.begin(), post_list.end());
    for (const i32 rt : post_list) {
      ComponentDelta& cd = scratch[post_slot[(size_t)rt]];
      std::sort(cd.parents.begin(), cd.parents.end());
      std::sort(cd.born.begin(), cd.born.end());
      if (cd.parents.size() >= 2) ++r.nodes;
      if (cd.parents.size() == 1 && cd.born.empty()) continue;  // continuation
      if (m_drop_nonmerge && cd.parents.size() < 2) continue;
      cd.batch = (u64)b;
      cd.level = evt(e0).level;
      cd.output = keys[canon_fid[(size_t)uf.find(rt)]];
      r.deltas.push_back(cd);
    }
    r.batch_levels.push_back(evt(e0).level);
    for (const u32 fid : touched) seen[fid] = 1;
    ++r.batches;
  }
  r.final_canon_fid.resize(nfid);
  for (size_t fid = 0; fid < nfid; ++fid) {
    const u32 c = canon_fid[(size_t)uf.find((i32)fid)];
    r.final_canon_fid[fid] = c;
    if (c > (u32)fid || r.final_canon_fid[(size_t)c] != c) ++r.partition_violations;
  }
  for (size_t fid = 1; fid < nfid; ++fid)
    if (!(keys[fid - 1] < keys[fid])) ++r.partition_violations;
  r.facet_keys = std::move(keys);
  return r;
}

}  // namespace mhgp5
