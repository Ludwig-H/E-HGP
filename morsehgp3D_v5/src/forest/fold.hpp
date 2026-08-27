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
#include <atomic>
#include <array>
#include <string>
#include <vector>

#include <chrono>

#include "../core/mutants.hpp"
#include "../lanes/keys.hpp"
#include "../lanes/level.hpp"
#include "../parallel/pool.hpp"
#include "../parallel/sort.hpp"

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
  u64 workers = 0;  // ouvriers reellement crees (max sur les phases paralleles)
  double t_sort_ms = 0, t_intern_ms = 0, t_merge_ms = 0, t_reduce_ms = 0, t_partition_ms = 0;
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

// Garde de capacite transactionnelle, decidable AVANT toute allocation sur
// les seuls comptes (evenements <= UINT32_MAX ; Σ(q+d) <= INT32_MAX pour les
// fid de l'union-find i32 ; lots <= evenements < UINT32_MAX, sentinelle des
// tables a epoque).
inline bool fold_capacity_ok(u64 events, u64 incidences, std::string* why) {
  if (events >= (u64)UINT32_MAX / 11) { *why = "resource_exhausted/requires_tiling : evenements >= (2^32-1)/11 (positions d'incidence u32)"; return false; }
  if (incidences > (u64)INT32_MAX) { *why = "resource_exhausted/requires_tiling : incidences > 2^31-1"; return false; }
  return true;
}

// `order` : permutation des evenements triee par niveau exact (calculee par
// l'appelant, qui peut la partager avec le rendu).
inline std::vector<u32> sort_events_by_level(const std::vector<ForestEvent>& events, int threads = 1,
                                             u64* workers = nullptr) {
  std::vector<u32> order(events.size());
  for (size_t i = 0; i < order.size(); ++i) order[i] = (u32)i;
  // Tri STABLE (les ex aequo d'un lot gardent l'ordre d'entree : l'ordre des
  // unions et l'emission des deltas en dependent) — parallele, identique a
  // std::stable_sort par contrat (porte mhgp5_parallel_sort_gate).
  const size_t w = parallel_stable_sort(
      order.begin(), order.end(),
      [&](u32 x, u32 y) { return compare_exact_level(events[x].level, events[y].level) < 0; }, threads);
  if (workers) *workers = std::max(*workers, (u64)w);
  return order;
}

// PARTITIONNEMENT PAR EMPREINTE (parallelisation du fold, session G4 du
// 27 aout : le fold sequentiel pesait 115 s sur 219 a uniform 50k) :
// 64 partitions FIXES par les six bits hauts de l'empreinte — independantes du
// nombre de fils, donc les pools et les tid ne dependent que de l'ordre des
// evenements ; les fid finaux viennent du tri global des cles uniques et ne
// dependent de rien d'autre : la sortie est bit-identique a 1 fil et a N fils.
inline constexpr int kFoldPartitionBits = 6;
inline constexpr size_t kFoldPartitions = (size_t)1 << kFoldPartitionBits;

// ETAT PREPARE d'un fold : tout ce qui est parallele (tri des evenements,
// lots, internement partitionne, fusion, remap). La reduction (union-find a
// lots, deltas, partition finale) est sequentielle et vit dans
// `reduce_fold`, pour pouvoir etre PIPELINEE avec la preparation de l'ordre
// suivant (run.hpp). Non copiable ; `events` est non possede : l'appelant le
// garde vivant jusqu'a la fin de la reduction.
struct FoldPrepared {
  const std::vector<ForestEvent>* events = nullptr;
  ForestResult r;  // refusal, workers, chronos de preparation, facets
  std::vector<u32> order;
  std::vector<std::pair<size_t, size_t>> batches;
  std::vector<FacetKey> keys;
  std::vector<u32> ev_fid;
  bool mutants[6] = {false, false, false, false, false, false};  // binary, repr, attach_pre, drop_nonmerge, canon_root, no_detector
};

// VALIDATION STRUCTURELLE (P0 de l'audit 9762daaf), distincte de la garde de
// capacite et AVANT toute allocation ou tri : q in [2, 11], d <= 9, q + d <= 11,
// un meme K = q + d - 1 dans l'appel, identifiants distincts (support et
// interieur), active_mask borne aux q bits. Un evenement hors contrat est un
// refus `invalid_input`, jamais une ecriture hors des dix cases de FacetKey.
inline bool fold_event_ok(const ForestEvent& ev, int K) {
  if (ev.q < 2 || ev.q > 11 || ev.d > 9 || (int)ev.q + (int)ev.d > 11) return false;
  if ((int)ev.q + (int)ev.d - 1 != K) return false;
  if ((u32)ev.active_mask >= (1u << ev.q)) return false;
  PointId ids[20];
  int n = 0;
  for (int t = 0; t < (int)ev.q; ++t) ids[n++] = ev.support[t];
  for (int t = 0; t < (int)ev.d; ++t) ids[n++] = ev.interior[t];
  for (int a = 0; a < n; ++a)
    for (int b = a + 1; b < n; ++b)
      if (ids[a] == ids[b]) return false;
  return true;
}

inline bool validate_fold_events(const std::vector<ForestEvent>& events, int threads, std::string* why) {
  // BALAYAGE SEQUENTIEL : aucune allocation ni fil avant que le contrat ne
  // soit verifie (la revendication « avant toute allocation » est ainsi
  // vraie ; cout mesure : ~60 ns par evenement). `threads` reste dans la
  // signature pour les appelants ; il n'est pas utilise ici.
  (void)threads;
  if (events.empty()) return true;
  const int K = (int)events[0].q + (int)events[0].d - 1;
  for (size_t i = 0; i < events.size(); ++i) {
    const ForestEvent& ev = events[i];
    const bool level_ok = ev.level.den > 0;  // contrat ExactLevel : den > 0
    if (fold_event_ok(ev, K) && level_ok) continue;
    *why = "invalid_input : evenement " + std::to_string(i) + " hors contrat (q=" + std::to_string((int)ev.q) +
           ", d=" + std::to_string((int)ev.d) + ", K attendu=" + std::to_string(K) +
           (level_ok ? "" : ", niveau den <= 0") +
           ") : q in [2,11], d <= 9, q+d <= 11, K constant, identifiants distincts, masque < 2^q, den > 0";
    return false;
  }
  return true;
}

inline FoldPrepared prepare_fold(const std::vector<ForestEvent>& events, int threads = 1) {
  using namespace fold_detail;
  FoldPrepared fp;
  fp.events = &events;
  ForestResult& r = fp.r;
  const bool m_binary = MHGP5_MUTANT("binary-ties");
  const bool m_repr = MHGP5_MUTANT("repr-ties");
  fp.mutants[0] = m_binary;
  fp.mutants[1] = m_repr;
  fp.mutants[2] = MHGP5_MUTANT("attach-prebatch");
  fp.mutants[3] = MHGP5_MUTANT("drop-nonmerge");
  fp.mutants[4] = MHGP5_MUTANT("canonical-is-uf-root");
  fp.mutants[5] = MHGP5_MUTANT("attach-detector-disabled");
  auto tmark = std::chrono::steady_clock::now();
  const auto mark = [&](double* out) {
    const auto now = std::chrono::steady_clock::now();
    *out += std::chrono::duration<double, std::milli>(now - tmark).count();
    tmark = now;
  };
  // Validation structurelle puis garde de capacite, AVANT toute allocation.
  if (!validate_fold_events(events, threads, &r.refusal)) return fp;
  u64 total_recs = 0;
  for (const ForestEvent& ev : events) total_recs += (u64)ev.q + ev.d;
  if (!fold_capacity_ok((u64)events.size(), total_recs, &r.refusal)) return fp;
  fp.order = sort_events_by_level(events, threads, &r.workers);
  const std::vector<u32>& order = fp.order;
  const auto evt = [&](size_t i) -> const ForestEvent& { return events[(size_t)order[i]]; };
  mark(&r.t_sort_ms);

  // Lots.
  std::vector<std::pair<size_t, size_t>>& batches = fp.batches;
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

  // ---- Internement partitionne.
  // Passe 1 (parallele par tranche d'evenements) : empreintes et positions,
  // comptees par partition. Passe 2 : diffusion en ordre (partition, tranche).
  // Passe 3 (parallele par partition) : table privee, tid locaux, ev_fid =
  // partition << 26 | tid. Passe 4 (parallele par partition) : tri des cles
  // uniques. Passe 5 : fusion k-aire des 64 listes triees -> fid globaux.
  // Passe 6 (parallele par tranche) : remap des ev_fid.
  const size_t ne = events.size();
  std::vector<FacetKey>& keys = fp.keys;
  fp.ev_fid.assign(ne * 11, 0);
  std::vector<u32>& ev_fid = fp.ev_fid;
  std::vector<u8> ev_part(ne * 11, 0);  // partition de chaque enregistrement (temporaire de prepare_fold)
  {
    struct Rec {
      u64 h;
      u32 pos;
    };
    const size_t T = planned_workers(ne, threads);
    const size_t chunk = T <= 1 ? std::max<size_t>(ne, 1) : std::max<size_t>(1, (ne + 8 * T - 1) / (8 * T));
    const size_t nchunks = ne == 0 ? 0 : (ne + chunk - 1) / chunk;
    std::vector<std::vector<Rec>> crec(nchunks);
    std::vector<std::vector<u32>> ccount(nchunks, std::vector<u32>(kFoldPartitions, 0));
    size_t created = parallel_items(nchunks, (int)T, [&](size_t c, size_t) {
      const size_t e0 = c * chunk, e1 = std::min(ne, e0 + chunk);
      std::vector<Rec>& out = crec[c];
      std::vector<u32>& cnt = ccount[c];
      size_t inc = 0;
      for (size_t e = e0; e < e1; ++e) inc += (size_t)evt(e).q + evt(e).d;
      out.reserve(inc);
      for (size_t e = e0; e < e1; ++e) {
        const ForestEvent& ev = evt(e);
        for (int s = 0; s < (int)ev.q; ++s) {
          const u64 h = facet_fingerprint(facet_minus(ev, s, -1));
          out.push_back(Rec{h, (u32)(e * 11 + (size_t)s)});
          ++cnt[(size_t)(h >> (64 - kFoldPartitionBits))];
        }
        for (int z = 0; z < (int)ev.d; ++z) {
          const u64 h = facet_fingerprint(facet_minus(ev, -1, z));
          out.push_back(Rec{h, (u32)(e * 11 + (size_t)(ev.q + z))});
          ++cnt[(size_t)(h >> (64 - kFoldPartitionBits))];
        }
      }
    });
    r.workers = std::max(r.workers, (u64)created);
    // Offsets : base par partition, puis par (partition, tranche).
    std::vector<size_t> pbase(kFoldPartitions + 1, 0);
    for (size_t p = 0; p < kFoldPartitions; ++p) {
      size_t tot = 0;
      for (size_t c = 0; c < nchunks; ++c) tot += ccount[c][p];
      pbase[p + 1] = pbase[p] + tot;
    }
    std::vector<std::vector<size_t>> coff(nchunks, std::vector<size_t>(kFoldPartitions, 0));
    for (size_t p = 0; p < kFoldPartitions; ++p) {
      size_t off = pbase[p];
      for (size_t c = 0; c < nchunks; ++c) {
        coff[c][p] = off;
        off += ccount[c][p];
      }
    }
    std::vector<Rec> parts((size_t)total_recs);
    created = parallel_items(nchunks, (int)T, [&](size_t c, size_t) {
      std::vector<size_t> off = coff[c];
      for (const Rec& rc : crec[c]) parts[off[(size_t)(rc.h >> (64 - kFoldPartitionBits))]++] = rc;
      std::vector<Rec>().swap(crec[c]);
    });
    r.workers = std::max(r.workers, (u64)created);
    // Passe 3 : internement par partition (table privee), tid locaux.
    std::vector<std::vector<std::pair<FacetKey, u32>>> pools(kFoldPartitions);
    const size_t TP = planned_workers(kFoldPartitions, threads);
    created = parallel_items(kFoldPartitions, (int)TP, [&](size_t p, size_t) {
      const size_t b = pbase[p], e = pbase[p + 1];
      if (b == e) return;
      FacetIntern in(e - b);
      for (size_t i = b; i < e; ++i) {
        const Rec& rc = parts[i];
        const size_t ev_i = rc.pos / 11, slot = rc.pos % 11;
        const ForestEvent& ev = evt(ev_i);
        const FacetKey f = slot < ev.q ? facet_minus(ev, (int)slot, -1) : facet_minus(ev, -1, (int)(slot - ev.q));
        // Temporaire {partition, tid} en deux tableaux : injectif sans borne
        // sur le nombre de facettes par partition (P1 de l'audit 9762daaf).
        ev_fid[rc.pos] = in.intern_hashed(f, rc.h);
        ev_part[rc.pos] = (u8)p;
      }
      std::vector<u64>().swap(in.table);
      pools[p].swap(in.pool);
      // Passe 4 : tri des cles uniques de la partition ; second = tid.
      std::sort(pools[p].begin(), pools[p].end(),
                [](const std::pair<FacetKey, u32>& x, const std::pair<FacetKey, u32>& y) {
                  return facet_less_k(x.first, y.first);
                });
    });
    r.workers = std::max(r.workers, (u64)created);
    std::vector<Rec>().swap(parts);
    mark(&r.t_intern_ms);
    // Passe 5 : fusion k-aire des partitions triees -> keys globales et
    // g_of[p][tid] = fid global. Les cles sont distinctes entre partitions
    // (empreintes differentes) : aucune egalite a departager. PARALLELE PAR
    // RANGS DE VALEURS : des separateurs pris dans la plus grosse partition
    // decoupent l'ordre total en R rangs ; chaque rang fusionne (tas) les 64
    // sous-listes [lower_bound(sep_t), lower_bound(sep_{t+1})) — les bornes
    // par partition et les offsets de sortie sont connus avant la fusion.
    size_t total_unique = 0, biggest = 0;
    for (size_t p = 0; p < kFoldPartitions; ++p) {
      total_unique += pools[p].size();
      if (pools[p].size() > pools[biggest].size()) biggest = p;
    }
    keys.resize(total_unique);
    std::vector<std::vector<u32>> g_of(kFoldPartitions);
    for (size_t p = 0; p < kFoldPartitions; ++p) g_of[p].resize(pools[p].size());
    {
      const size_t R = std::max<size_t>(1, std::min<size_t>(planned_workers(total_unique, threads) * 2, pools[biggest].size()));
      // Bornes lo[t][p] : debut du rang t dans la partition p (lo[R][p] = fin).
      std::vector<std::vector<size_t>> lo(R + 1, std::vector<size_t>(kFoldPartitions, 0));
      for (size_t p = 0; p < kFoldPartitions; ++p) lo[R][p] = pools[p].size();
      for (size_t t = 1; t < R; ++t) {
        const FacetKey& sep = pools[biggest][t * pools[biggest].size() / R].first;
        for (size_t p = 0; p < kFoldPartitions; ++p) {
          const auto it = std::lower_bound(pools[p].begin(), pools[p].end(), sep,
                                           [](const std::pair<FacetKey, u32>& a, const FacetKey& k) {
                                             return facet_less_k(a.first, k);
                                           });
          lo[t][p] = (size_t)(it - pools[p].begin());
        }
      }
      std::vector<size_t> out_off(R + 1, 0);
      for (size_t t = 0; t < R; ++t) {
        size_t n = 0;
        for (size_t p = 0; p < kFoldPartitions; ++p) n += lo[t + 1][p] - lo[t][p];
        out_off[t + 1] = out_off[t] + n;
      }
      const size_t created_m = parallel_items(R, threads, [&](size_t t, size_t) {
        std::vector<size_t> cur(kFoldPartitions);
        for (size_t p = 0; p < kFoldPartitions; ++p) cur[p] = lo[t][p];
        std::vector<u32> heap;
        const auto less_p = [&](u32 a, u32 b) {
          return facet_less_k(pools[b][cur[b]].first, pools[a][cur[a]].first);  // min-heap
        };
        for (u32 p = 0; p < (u32)kFoldPartitions; ++p)
          if (cur[p] < lo[t + 1][p]) heap.push_back(p);
        std::make_heap(heap.begin(), heap.end(), less_p);
        for (size_t g = out_off[t]; g < out_off[t + 1]; ++g) {
          std::pop_heap(heap.begin(), heap.end(), less_p);
          const u32 p = heap.back();
          heap.pop_back();
          keys[g] = pools[p][cur[p]].first;
          g_of[p][(size_t)pools[p][cur[p]].second] = (u32)g;
          if (++cur[p] < lo[t + 1][p]) {
            heap.push_back(p);
            std::push_heap(heap.begin(), heap.end(), less_p);
          }
        }
      });
      r.workers = std::max(r.workers, (u64)created_m);
    }
    for (size_t p = 0; p < kFoldPartitions; ++p) std::vector<std::pair<FacetKey, u32>>().swap(pools[p]);
    // Passe 6 : remap parallele.
    created = parallel_ranges(ne * 11, threads, [&](size_t b, size_t e, size_t) {
      for (size_t i = b; i < e; ++i) {
        const size_t ev_i = i / 11, slot = i % 11;
        const ForestEvent& ev = evt(ev_i);
        if (slot >= (size_t)ev.q + ev.d) continue;
        ev_fid[i] = g_of[(size_t)ev_part[i]][(size_t)ev_fid[i]];
      }
    });
    r.workers = std::max(r.workers, (u64)created);
    mark(&r.t_merge_ms);
  }
  r.facets = keys.size();
  return fp;
}

inline ForestResult reduce_fold(FoldPrepared&& fp) {
  using namespace fold_detail;
  ForestResult r = std::move(fp.r);
  if (!r.refusal.empty()) return r;
  const std::vector<ForestEvent>& events = *fp.events;
  const std::vector<u32>& order = fp.order;
  const auto evt = [&](size_t i) -> const ForestEvent& { return events[(size_t)order[i]]; };
  const std::vector<std::pair<size_t, size_t>>& batches = fp.batches;
  std::vector<FacetKey>& keys = fp.keys;
  std::vector<u32>& ev_fid = fp.ev_fid;
  const bool m_attach_pre = fp.mutants[2], m_drop_nonmerge = fp.mutants[3], m_canon_root = fp.mutants[4],
             m_no_detector = fp.mutants[5];
  auto tmark = std::chrono::steady_clock::now();
  const auto mark = [&](double* out) {
    const auto now = std::chrono::steady_clock::now();
    *out += std::chrono::duration<double, std::milli>(now - tmark).count();
    tmark = now;
  };
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
  mark(&r.t_reduce_ms);
  r.final_canon_fid.resize(nfid);
  for (size_t fid = 0; fid < nfid; ++fid) {
    const u32 c = canon_fid[(size_t)uf.find((i32)fid)];
    r.final_canon_fid[fid] = c;
    if (c > (u32)fid || r.final_canon_fid[(size_t)c] != c) ++r.partition_violations;
  }
  for (size_t fid = 1; fid < nfid; ++fid)
    if (!(keys[fid - 1] < keys[fid])) ++r.partition_violations;
  std::vector<u32>().swap(ev_fid);
  r.facet_keys = std::move(keys);
  mark(&r.t_partition_ms);
  return r;
}

// Le fold complet, sequentiel de bout en bout du point de vue de l'appelant.
inline ForestResult build_forest(const std::vector<ForestEvent>& events, int threads = 1) {
  return reduce_fold(prepare_fold(events, threads));
}

}  // namespace mhgp5
