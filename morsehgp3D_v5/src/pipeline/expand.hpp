// MorseHGP3D v5 — du candidat a l'evenement : prefiltre, census, expansion.
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

#include <vector>

#include "../forest/fold.hpp"
#include "../forest/plateau.hpp"
#include "../parallel/pool.hpp"
#include "candidates.hpp"
#include "census.hpp"

namespace mhgp5 {

struct Survivor {
  u32 idx;
  u64 depth;
};

struct BallData {
  BallKey key;
  ExactLevel level;
  u8 arity;
  std::vector<i32> interior, shell;
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

inline void prefilter_balls(const CloudIndex& ix, const std::vector<BallCandidate>& cands, u64 smax, int threads,
                            std::vector<Survivor>* survivors, ExpandStats* st) {
  const bool m_minus_one = MHGP5_MUTANT("depth-threshold-minus-one");
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
  const bool m_skip_full = MHGP5_MUTANT("skip-full-census");
  const bool m_drop_chunk = MHGP5_MUTANT("par-drop-ball-chunk");
  size_t nchunks = 0;
  std::vector<std::vector<BallData>> lb;
  std::vector<int> lrc;
  {
    size_t dummy = 0;
    expand_detail::chunked(survivors.size(), threads, &dummy, [&](size_t, size_t, size_t) {});
    lb.assign(dummy, {});
    lrc.assign(dummy, 0);
  }
  const size_t created = expand_detail::chunked(survivors.size(), threads, &nchunks, [&](size_t c, size_t b, size_t e) {
    for (size_t i = b; i < e; ++i) {
      const Survivor& sv = survivors[i];
      const BallCandidate& bc = cands[sv.idx];
      BallData bd;
      bd.key = bc.key;
      bd.level = bc.level;
      bd.arity = bc.arity;
      if (m_skip_full) {
        lb[c].push_back(std::move(bd));
        continue;
      }
      const CensusStatus cs = ball_census(ix, bc.key, (size_t)(smax - bc.arity), shell_cap, &bd.interior, &bd.shell);
      if (cs == CensusStatus::kShellOverflow) { lrc[c] = 2; return; }
      if (cs == CensusStatus::kInteriorOverflow || bd.interior.size() != (size_t)sv.depth) { lrc[c] = 3; return; }
      lb[c].push_back(std::move(bd));
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
      st->census_interior += bd.interior.size();
      st->census_shell += bd.shell.size();
      balls->push_back(std::move(bd));
    }
  }
  return PipelineStatus::kCompleteRegular;
}

// Expansion des plateaux en evenements par K (1..kmax), en PointId externes.
// PRECONDITION (V1 tranche par l'auditeur) : positions distinctes — l'index a
// ete accepte par run_pipeline ; `kmax <= 10` (profil).
inline void expand_events(const CloudIndex& ix, const std::vector<BallData>& balls, u64 kmax, int threads,
                          std::vector<std::vector<ForestEvent>>* ev_k, ExpandStats* st) {
  const bool m_dense = MHGP5_MUTANT("dense-pointid");
  size_t nchunks = 0;
  std::vector<std::vector<std::vector<ForestEvent>>> lev;
  {
    size_t dummy = 0;
    expand_detail::chunked(balls.size(), threads, &dummy, [&](size_t, size_t, size_t) {});
    lev.assign(dummy, std::vector<std::vector<ForestEvent>>((size_t)kmax + 1));
  }
  const size_t created = expand_detail::chunked(balls.size(), threads, &nchunks, [&](size_t c, size_t b, size_t e) {
    std::vector<PlateauEvent> pevents;
    for (size_t bi = b; bi < e; ++bi) {
      const BallData& bd = balls[bi];
      pevents.clear();
      expand_plateau(ball_center(bd.key), ix.upos, bd.interior, bd.shell, (size_t)(kmax + 1), &pevents);
      for (const PlateauEvent& pe : pevents) {
        const size_t K = pe.tpart.size() + pe.ipart.size() - 1;
        if (K < 1 || K > (size_t)kmax) continue;
        ForestEvent ev;
        ev.q = (u8)pe.tpart.size();
        ev.d = (u8)pe.ipart.size();
        ev.active_mask = pe.active_mask;
        for (size_t t = 0; t < pe.tpart.size(); ++t)
          ev.support[t] = m_dense ? (PointId)pe.tpart[t] : ix.point_id(pe.tpart[t]);
        for (size_t t = 0; t < pe.ipart.size(); ++t)
          ev.interior[t] = m_dense ? (PointId)pe.ipart[t] : ix.point_id(pe.ipart[t]);
        ev.level = bd.level;
        lev[c][K].push_back(ev);
      }
    }
  });
  st->workers_expand = std::max(st->workers_expand, (u64)created);
  ev_k->assign((size_t)kmax + 1, {});
  st->events_by_k.assign((size_t)kmax + 1, 0);
  for (size_t c = 0; c < nchunks; ++c)
    for (size_t K = 1; K <= (size_t)kmax; ++K)
      (*ev_k)[K].insert((*ev_k)[K].end(), lev[c][K].begin(), lev[c][K].end());
  for (size_t K = 1; K <= (size_t)kmax; ++K) st->events_by_k[K] = (*ev_k)[K].size();
}

}  // namespace mhgp5
