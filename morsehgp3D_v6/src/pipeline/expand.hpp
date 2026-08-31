// MorseHGP3D v6 — du candidat a l'evenement : prefiltre, census, expansion.
//
//   prefilter_balls : passe count-only par arite minimale — mort a
//     |I_B| >= h_qmin = smax + 1 − q_min (10/9/8 au profil smax = 11) ;
//     mutant `depth-threshold-minus-one` ;
//   census_balls    : I_B et U_B COMPLETS des survivantes ; recoupement
//     passe 1 == passe 2 (invariant) ; coquille > plafond = resource_exhausted ;
//     mutant `shell-cap-before-depth` (une seule passe : le mauvais statut) ;
//   expand_events   : plateaux → evenements par K, en PointId externes —
//     LA frontiere d'identite (GeometryIndex → PointId ici et seulement ici,
//     par le representant du bucket ; mutant `dense-pointid` : cast du rang).
// Toutes les phases sont paralleles par tranches d'index avec fusion en
// ordre de tranche : sortie bit-identique au sequentiel (mutant
// `par-drop-ball-chunk` : une tranche de census oubliee).
#pragma once

#include <algorithm>
#include <limits>
#include <span>
#include <utility>
#include <vector>

#include "../forest/fold.hpp"
#include "../forest/plateau.hpp"
#include "../parallel/pool.hpp"
#include "candidates.hpp"
#include "census.hpp"

namespace mhgp6 {

struct Survivor {
  u32 idx;
  u64 depth;
};

// Listes de census INLINE et bornees par theoreme et profil : interieur
// <= smax − arite <= 9 (K_max <= 10 <=> smax <= 11, refus explicite au-dela),
// coquille <= kBallShellMax = plafond du profil (run.hpp). Aucune allocation
// par boule (deux std::vector par boule = ~400 o et deux mallocs par boule,
// poste mesure a 16 000 : +2,6 Go de census pour 6,5 M boules).
inline constexpr size_t kBallInteriorMax = 9;
inline constexpr size_t kBallShellMax = 12;

struct BallData {
  BallKey key;
  ExactLevel level;
  u8 arity = 0;
  u8 n_interior = 0, n_shell = 0;
  i32 interior_ids[kBallInteriorMax] = {};
  i32 shell_ids[kBallShellMax] = {};
  std::span<const i32> interior() const { return std::span<const i32>(interior_ids, (size_t)n_interior); }
  std::span<const i32> shell() const { return std::span<const i32>(shell_ids, (size_t)n_shell); }
};

struct ExpandStats {
  u64 unique_balls = 0, dead_depth = 0, survivors = 0, census_interior = 0, census_shell = 0;
  std::vector<u64> events_by_k;  // indexee par K, dimensionnee kmax + 1
  u64 workers_prefilter = 0, workers_census = 0, workers_expand = 0;
  DepthStats depth;
};

enum class PipelineStatus { kCompleteRegular, kUnsupportedDegeneracy, kResourceExhausted, kInvalidInput, kInvariantViolated };

namespace expand_detail {
// Tranches d'index contigues (≈ 8 par ouvrier) executees par tirage dynamique,
// resultats par TRANCHE (fusion en ordre de tranche par l'appelant).
template <typename Fn>
inline size_t chunked(size_t n, int threads, size_t* nchunks_out, Fn&& fn) {
  const size_t T = planned_workers(n, threads);
  const size_t chunk = T <= 1 ? std::max<size_t>(n, 1) : std::max<size_t>(1, (n + 8 * T - 1) / (8 * T));
  const size_t nchunks = n == 0 ? 0 : (n + chunk - 1) / chunk;
  *nchunks_out = nchunks;
  return parallel_items(nchunks, (int)T, [&](size_t c, size_t) { fn(c, c * chunk, std::min(n, (c + 1) * chunk)); });
}
}  // namespace expand_detail

// Plafond TESTABLE des indices du prefiltre (Survivor::idx u32). Le refus
// contractuel (resource_exhausted) vit dans run_pipeline AVANT l'appel ; la
// garde interne reste une defense (exception = faute d'appelant, jamais la
// voie produit).
inline bool candidates_capacity_ok(size_t n_candidates) {
  return n_candidates <= (size_t)std::numeric_limits<u32>::max();
}

inline void prefilter_balls(const CloudIndex& ix, const std::vector<BallCandidate>& cands, u64 smax, int threads,
                            std::vector<Survivor>* survivors, ExpandStats* st) {
  if (!candidates_capacity_ok(cands.size()))
    throw std::length_error("prefilter_balls : plus de 2^32-1 candidats (garde interne ; le pipeline refuse avant)");
  const bool m_minus_one = MHGP6_MUTANT("depth-threshold-minus-one");
  size_t nchunks = 0;
  std::vector<std::vector<Survivor>> lsv;
  std::vector<DepthStats> lds;
  // Dimensionne d'abord (nchunks inconnu avant l'appel) : deux passes de
  // decoupage identiques.
  {
    size_t dummy = 0;
    expand_detail::chunked(cands.size(), threads, &dummy, [&](size_t, size_t, size_t) {});
    lsv.assign(dummy, {});
    lds.assign(dummy, {});
  }
  u64 dead = 0;
  std::vector<u64> ldead(lsv.size(), 0);
  const size_t created = expand_detail::chunked(cands.size(), threads, &nchunks, [&](size_t c, size_t b, size_t e) {
    for (size_t i = b; i < e; ++i) {
      const BallCandidate& bc = cands[i];
      u64 h = smax + 1 - (u64)bc.arity;
      if (m_minus_one) --h;
      u64 depth = 0;
      if (ball_depth_at_least(ix, bc.key, h, &depth, &lds[c])) {
        ++ldead[c];
        continue;
      }
      lsv[c].push_back(Survivor{(u32)i, depth});
    }
  });
  st->workers_prefilter = std::max(st->workers_prefilter, (u64)created);
  survivors->clear();
  for (size_t c = 0; c < nchunks; ++c) {
    survivors->insert(survivors->end(), lsv[c].begin(), lsv[c].end());
    dead += ldead[c];
    st->depth.nodes += lds[c].nodes;
    st->depth.leaf_tests += lds[c].leaf_tests;
    st->depth.range_add_mass += lds[c].range_add_mass;
  }
  st->dead_depth = dead;
  st->survivors = survivors->size();
}

inline PipelineStatus census_balls(const CloudIndex& ix, const std::vector<BallCandidate>& cands,
                                   const std::vector<Survivor>& survivors, u64 smax, size_t shell_cap, int threads,
                                   std::vector<BallData>* balls, ExpandStats* st) {
  const bool m_skip_full = MHGP6_MUTANT("skip-full-census");
  const bool m_drop_chunk = MHGP6_MUTANT("par-drop-ball-chunk");
  size_t nchunks = 0;
  std::vector<std::vector<BallData>> lb;
  std::vector<int> lrc;
  {
    size_t dummy = 0;
    expand_detail::chunked(survivors.size(), threads, &dummy, [&](size_t, size_t, size_t) {});
    lb.assign(dummy, {});
    lrc.assign(dummy, 0);
  }
  if (shell_cap > kBallShellMax) return PipelineStatus::kInvalidInput;  // garde du profil (run.hpp la refuse en amont)
  const size_t created = expand_detail::chunked(survivors.size(), threads, &nchunks, [&](size_t c, size_t b, size_t e) {
    std::vector<i32> in, sh;  // tampons de l'ouvrier, copies dans les tableaux inline
    for (size_t i = b; i < e; ++i) {
      const Survivor& sv = survivors[i];
      const BallCandidate& bc = cands[sv.idx];
      BallData bd;
      bd.key = bc.key;
      bd.level = bc.level;
      bd.arity = bc.arity;
      if (m_skip_full) {
        lb[c].push_back(bd);
        continue;
      }
      const CensusStatus cs = ball_census(ix, bc.key, (size_t)(smax - bc.arity), shell_cap, &in, &sh);
      if (cs == CensusStatus::kShellOverflow) { lrc[c] = 2; return; }
      if (cs == CensusStatus::kInteriorOverflow || in.size() != (size_t)sv.depth) { lrc[c] = 3; return; }
      if (in.size() > kBallInteriorMax || sh.size() > kBallShellMax) { lrc[c] = 3; return; }  // inatteignable : bornes du profil
      bd.n_interior = (u8)in.size();
      bd.n_shell = (u8)sh.size();
      std::copy(in.begin(), in.end(), bd.interior_ids);
      std::copy(sh.begin(), sh.end(), bd.shell_ids);
      lb[c].push_back(bd);
    }
  });
  st->workers_census = std::max(st->workers_census, (u64)created);
  for (size_t c = 0; c < nchunks; ++c)
    if (lrc[c] != 0) return lrc[c] == 2 ? PipelineStatus::kResourceExhausted : PipelineStatus::kInvariantViolated;
  balls->clear();
  balls->reserve(survivors.size());
  for (size_t c = 0; c < nchunks; ++c) {
    if (m_drop_chunk && nchunks > 1 && c == 0) continue;
    for (BallData& bd : lb[c]) {
      st->census_interior += bd.n_interior;
      st->census_shell += bd.n_shell;
      balls->push_back(bd);
    }
  }
  return PipelineStatus::kCompleteRegular;
}

// EXPANSION PAR ORDRE K (residence reelle par K, audit 87e915bd P1) : les
// boules censusees sont le seul objet amont resident ; les evenements d'un
// ordre sont materialises pour CE K seulement, puis liberes par l'appelant.
// Regime regulier (|U_B| = arite du support minimal) : la boule donne UN
// evenement, d'ordre K = |I_B| + |U_B| − 1, connu sans expansion. Plateau
// (|U_B| > arite minimale) : expansion complete, filtree sur K.
// PRECONDITIONS (V1 tranchee par l'auditeur) : positions distinctes ;
// kmax <= 10 (profil) ; la conversion GeometryIndex -> PointId a lieu ICI et
// seulement ici (mutant `dense-pointid` : cast du rang).

// Comptage des evenements par K sans materialisation (pour les gardes de
// capacite AVANT toute publication) : (evenements, incidences Σ(q+d)) par K.
struct KCount {
  u64 events = 0, incidences = 0;
};

inline std::vector<KCount> count_events_by_k(const CloudIndex& ix, const std::vector<BallData>& balls, u64 kmax,
                                             int threads = 1) {
  size_t nchunks = 0;
  std::vector<std::vector<KCount>> lc;
  {
    size_t dummy = 0;
    expand_detail::chunked(balls.size(), threads, &dummy, [&](size_t, size_t, size_t) {});
    lc.assign(dummy, std::vector<KCount>((size_t)kmax + 1));
  }
  expand_detail::chunked(balls.size(), threads, &nchunks, [&](size_t c, size_t b, size_t e) {
    std::vector<PlateauEvent> pevents;
    std::vector<KCount>& out = lc[c];
    for (size_t bi = b; bi < e; ++bi) {
      const BallData& bd = balls[bi];
      const size_t q = bd.n_shell, d = bd.n_interior;
      if (q == (size_t)bd.arity) {  // regulier : un evenement, ordre q + d − 1
        const size_t K = q + d - 1;
        if (K >= 1 && K <= (size_t)kmax) {
          ++out[K].events;
          out[K].incidences += q + d;
        }
        continue;
      }
      pevents.clear();
      expand_plateau(ball_center(bd.key), ix.upos, bd.interior(), bd.shell(), (size_t)(kmax + 1), &pevents);
      for (const PlateauEvent& pe : pevents) {
        const size_t K = pe.tpart.size() + pe.ipart.size() - 1;
        if (K < 1 || K > (size_t)kmax) continue;
        ++out[K].events;
        out[K].incidences += pe.tpart.size() + pe.ipart.size();
      }
    }
  });
  std::vector<KCount> out((size_t)kmax + 1);
  for (size_t c = 0; c < nchunks; ++c)
    for (size_t K = 0; K <= (size_t)kmax; ++K) {
      out[K].events += lc[c][K].events;
      out[K].incidences += lc[c][K].incidences;
    }
  return out;
}

namespace expand_detail {
inline ForestEvent make_event(const CloudIndex& ix, const BallData& bd, std::span<const i32> tpart,
                              std::span<const i32> ipart, u16 active_mask, bool dense) {
  ForestEvent ev;
  ev.q = (u8)tpart.size();
  ev.d = (u8)ipart.size();
  ev.active_mask = active_mask;
  for (size_t t = 0; t < tpart.size(); ++t) ev.support[t] = dense ? (PointId)tpart[t] : ix.point_id(tpart[t]);
  for (size_t t = 0; t < ipart.size(); ++t) ev.interior[t] = dense ? (PointId)ipart[t] : ix.point_id(ipart[t]);
  ev.level = bd.level;
  return ev;
}
}  // namespace expand_detail

// Evenements de l'ordre K seulement, en PointId externes, ordre des tranches
// conserve (bit-identique au sequentiel).
inline void expand_events_k(const CloudIndex& ix, const std::vector<BallData>& balls, u64 K, u64 kmax, int threads,
                            std::vector<ForestEvent>* out, ExpandStats* st) {
  const bool m_dense = MHGP6_MUTANT("dense-pointid");
  size_t nchunks = 0;
  std::vector<std::vector<ForestEvent>> lev;
  {
    size_t dummy = 0;
    expand_detail::chunked(balls.size(), threads, &dummy, [&](size_t, size_t, size_t) {});
    lev.assign(dummy, {});
  }
  const size_t created = expand_detail::chunked(balls.size(), threads, &nchunks, [&](size_t c, size_t b, size_t e) {
    std::vector<PlateauEvent> pevents;
    for (size_t bi = b; bi < e; ++bi) {
      const BallData& bd = balls[bi];
      const size_t q = bd.n_shell, d = bd.n_interior;
      if (q == (size_t)bd.arity) {
        if (q + d - 1 != (size_t)K) continue;
        // Regime regulier : tous les retraits de support sont actifs (le
        // support minimal retrecit la boule), les retraits d'interieur sont
        // des attachements — la meme regle que l'expansion de plateau.
        // MEME ordre de support que l'expansion de plateau (trie par index
        // unique) : l'ordre des unions d'un lot en depend, donc l'ordre
        // d'emission des deltas et le digest.
        std::vector<i32> tsorted(bd.shell().begin(), bd.shell().end());
        std::sort(tsorted.begin(), tsorted.end());
        lev[c].push_back(expand_detail::make_event(ix, bd, tsorted, bd.interior(), (u16)((1u << q) - 1u), m_dense));
        continue;
      }
      pevents.clear();
      expand_plateau(ball_center(bd.key), ix.upos, bd.interior(), bd.shell(), (size_t)(kmax + 1), &pevents);
      for (const PlateauEvent& pe : pevents) {
        if (pe.tpart.size() + pe.ipart.size() - 1 != (size_t)K) continue;
        lev[c].push_back(expand_detail::make_event(ix, bd, pe.tpart, pe.ipart, pe.active_mask, m_dense));
      }
    }
  });
  // Mutant `prefix-tamper-event-order` : sur le dernier ordre d'un PREFIXE (kmax < 10), echange interior[0] et
  // interior[1] des evenements a d >= 2 — l'ordre des interieurs n'entre ni dans les facettes (ensembles), ni dans
  // le choix de la racine (support[0]), ni dans le digest v4 ; seule la signature des evenements canoniques de la
  // porte de prefixe le voit.
  if (MHGP6_MUTANT("prefix-tamper-event-order") && kmax < 10 && K == kmax)
    for (std::vector<ForestEvent>& v : lev)
      for (ForestEvent& e : v)
        if (e.d >= 2) std::swap(e.interior[0], e.interior[1]);

  st->workers_expand = std::max(st->workers_expand, (u64)created);
  out->clear();
  for (size_t c = 0; c < nchunks; ++c) out->insert(out->end(), lev[c].begin(), lev[c].end());
  if (st->events_by_k.size() < (size_t)kmax + 1) st->events_by_k.assign((size_t)kmax + 1, 0);
  st->events_by_k[K] = out->size();
}

}  // namespace mhgp6
