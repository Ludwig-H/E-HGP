// MorseHGP3D v7 — du candidat a l'evenement : prefiltre, census, expansion.
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
// Prefiltre et expansion fusionnent leurs tranches en ordre. Le census ecrit
// directement dans une destination privee de taille connue, une case par
// survivante ; seul un census entierement valide publie ce tableau par swap.
// La residence BallData n'a plus de shards ni de copie de fusion, a un fil
// comme a plusieurs. L'ordre logique reste celui des survivantes.
#pragma once

#include <algorithm>
#include <array>
#include <limits>
#include <span>
#include <utility>
#include <vector>

#include "../forest/fold.hpp"
#include "../forest/plateau.hpp"
#include "../parallel/pool.hpp"
#include "candidates.hpp"
#include "census.hpp"

namespace mhgp7 {

struct Survivor {
  u32 idx;
  u64 depth;
};

// Listes de census INLINE et bornees par theoreme et profil : interieur
// <= smax − arite <= 9 (K_max <= 10 <=> smax <= 11, refus explicite au-dela),
// coquille <= kBallShellMax = plafond du profil (run.hpp). Aucune allocation
// par boule (deux std::vector par boule = ~400 o et deux mallocs par boule,
// poste mesure a 16 000 : +2,6 Go de census pour 6,5 M boules). Palier P4 : la
// PILE DE DESCENTE des deux passes est hissee au meme niveau (une par tranche
// au lieu d'une par boule) — voir census.hpp et `DepthStats::owned_stacks`.
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
  // V_census — DEUX composantes declarees (audit du 31 aout : « ce compteur
  // n'est pas prefiltre + census ») : `depth` = traversees de
  // `ball_depth_at_least` (prefiltre count-only : nœuds, tests de feuille,
  // masse de range-add) ; `census` = traversees de `ball_census` (passe 2 :
  // nœuds, tests de puissance en feuille ; range_add_mass = 0 par nature).
  DepthStats depth;
  DepthStats census;
  // Nom historique conserve pour les consommateurs, semantique v7 versionnee
  // ci-dessous : pic des CAPACITES BallData possedees par le census direct.
  // Il comprend la destination privee et tout shadow sous mutant, AVANT leur
  // destruction. Ce n'est ni le RSS, ni un compteur independent de
  // l'allocateur. L'ancienne fusion v6 echantillonnait apres liberation et
  // omettait le pic entre copie et liberation (notamment a une tranche).
  u64 census_merge_peak_bytes = 0;
};

inline constexpr const char* kCensusStorageVersion = "mhgp7-census-direct-v1";

enum class PipelineStatus { kCompleteRegular, kUnsupportedDegeneracy, kResourceExhausted, kInvalidInput, kInvariantViolated };

namespace expand_detail {
struct ChunkPlan {
  size_t workers = 1;
  size_t chunk = 1;
  size_t count = 0;
};

// Plan pur : aucune allocation, aucun lancement de fil pour dimensionner les
// tableaux par tranche. Les bornes restent celles du chemin v6.
inline ChunkPlan chunk_plan(size_t n, int threads) {
  const size_t workers = planned_workers(n, threads);
  const size_t chunks_per_team = 8 * workers;
  const size_t chunk = workers <= 1 ? std::max<size_t>(n, 1)
      : std::max<size_t>(1, n / chunks_per_team + (n % chunks_per_team != 0));
  return {workers, chunk, n / chunk + (n % chunk != 0)};
}

// Tranches d'index contigues (≈ 8 par ouvrier) executees par tirage dynamique,
// resultats par TRANCHE (fusion en ordre de tranche par l'appelant).
template <typename Fn>
inline size_t chunked(size_t n, int threads, size_t* nchunks_out, Fn&& fn) {
  const ChunkPlan p = chunk_plan(n, threads);
  *nchunks_out = p.count;
  return parallel_items(p.count, (int)p.workers, [&](size_t c, size_t) {
    const size_t begin = c * p.chunk;
    fn(c, begin, begin + std::min(p.chunk, n - begin));
  });
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
  const bool m_minus_one = MHGP7_MUTANT("depth-threshold-minus-one");
  const bool m_keep_chunks = MHGP7_MUTANT("keep-ball-chunks");
  size_t nchunks = expand_detail::chunk_plan(cands.size(), threads).count;
  std::vector<std::vector<Survivor>> lsv(nchunks);
  std::vector<DepthStats> lds(nchunks);
  u64 dead = 0;
  std::vector<u64> ldead(lsv.size(), 0);
  const size_t created = expand_detail::chunked(cands.size(), threads, &nchunks, [&](size_t c, size_t b, size_t e) {
    // Pile de descente HISSEE au niveau de l'ouvrier (palier P4), meme motif
    // que les tampons `in`/`sh` du census : videe a chaque boule, sa capacite
    // survit — plus une allocation par boule mais une par tranche.
    std::vector<NodeRef> stack;
    for (size_t i = b; i < e; ++i) {
      const BallCandidate& bc = cands[i];
      u64 h = smax + 1 - (u64)bc.arity;
      if (m_minus_one) --h;
      u64 depth = 0;
      if (ball_depth_at_least(ix, bc.key, h, &depth, &lds[c], &stack)) {
        ++ldead[c];
        continue;
      }
      lsv[c].push_back(Survivor{(u32)i, depth});
    }
  });
  st->workers_prefilter = std::max(st->workers_prefilter, (u64)created);
  survivors->clear();
  size_t n_surv = 0;
  for (size_t c = 0; c < nchunks; ++c) n_surv += lsv[c].size();
  survivors->reserve(n_surv);  // total connu AVANT la fusion : aucune reallocation geometrique
  for (size_t c = 0; c < nchunks; ++c) {
    survivors->insert(survivors->end(), lsv[c].begin(), lsv[c].end());
    dead += ldead[c];
    st->depth.nodes += lds[c].nodes;
    st->depth.leaf_tests += lds[c].leaf_tests;
    st->depth.range_add_mass += lds[c].range_add_mass;
    st->depth.owned_stacks += lds[c].owned_stacks;
    // Liberation de la tranche DES qu'elle est consommee (le vecteur vide
    // rend la page au repartiteur ; clear() la garderait). La fusion reste
    // en ORDRE DE TRANCHE : sortie bit-identique au sequentiel.
    // Mutant `keep-ball-chunks` : la liberation est retiree aux TROIS etages —
    // la fusion reporte de nouveau DEUX FOIS la residence de l'etage. Aucun
    // digest, aucune cardinalite, aucun plancher ne peut le voir (l'objet est
    // inchange) : seul le PLAFOND d'increment de pic du census le tue.
    if (!m_keep_chunks) std::vector<Survivor>().swap(lsv[c]);
  }
  st->dead_depth = dead;
  st->survivors = survivors->size();
}

inline PipelineStatus census_balls(const CloudIndex& ix, const std::vector<BallCandidate>& cands,
                                   const std::vector<Survivor>& survivors, u64 smax, size_t shell_cap, int threads,
                                   std::vector<BallData>* balls, ExpandStats* st) {
  const bool m_skip_full = MHGP7_MUTANT("skip-full-census");
  const bool m_drop_chunk = MHGP7_MUTANT("par-drop-ball-chunk");
  const bool m_keep_chunks = MHGP7_MUTANT("keep-ball-chunks");
  const bool m_offset = MHGP7_MUTANT("census-direct-offset");
  const bool m_publish_prefix = MHGP7_MUTANT("census-direct-publish-prefix");
  // Une sortie reutilisee devient vide meme en cas de refus ou d'exception.
  // Aucune donnee partielle ne transite par le vecteur public.
  std::vector<BallData>().swap(*balls);
  st->census_merge_peak_bytes = 0;
  st->census_interior = 0;
  st->census_shell = 0;
  if (shell_cap > kBallShellMax) return PipelineStatus::kInvalidInput;  // garde du profil (run.hpp la refuse en amont)
  size_t nchunks = expand_detail::chunk_plan(survivors.size(), threads).count;
  std::vector<int> lrc(nchunks, 0);
  std::vector<DepthStats> lcs(nchunks);
  std::vector<BallData> staged(survivors.size());
  st->census_merge_peak_bytes = (u64)staged.capacity() * (u64)sizeof(BallData);
  const size_t created = expand_detail::chunked(survivors.size(), threads, &nchunks, [&](size_t c, size_t b, size_t e) {
    std::vector<i32> in, sh;    // tampons de l'ouvrier, copies dans les tableaux inline
    std::vector<NodeRef> stack;  // pile de descente hissee (palier P4), meme motif
    for (size_t i = b; i < e; ++i) {
      const Survivor& sv = survivors[i];
      const BallCandidate& bc = cands[sv.idx];
      BallData bd;
      bd.key = bc.key;
      bd.level = bc.level;
      bd.arity = bc.arity;
      // Rotation bijective sous mutant : aucune ecriture concurrente sur une
      // meme case, mais l'ordre des survivantes est intentionnellement perdu.
      const size_t dst = m_offset ? (i + 1 == survivors.size() ? 0 : i + 1) : i;
      if (m_skip_full) {
        staged[dst] = bd;
        continue;
      }
      const CensusStatus cs =
          ball_census(ix, bc.key, (size_t)(smax - bc.arity), shell_cap, &in, &sh, &lcs[c], &stack);
      if (cs == CensusStatus::kShellOverflow) { lrc[c] = 2; return; }
      if (cs == CensusStatus::kInteriorOverflow || in.size() != (size_t)sv.depth) { lrc[c] = 3; return; }
      if (in.size() > kBallInteriorMax || sh.size() > kBallShellMax) { lrc[c] = 3; return; }  // inatteignable : bornes du profil
      bd.n_interior = (u8)in.size();
      bd.n_shell = (u8)sh.size();
      std::copy(in.begin(), in.end(), bd.interior_ids);
      std::copy(sh.begin(), sh.end(), bd.shell_ids);
      staged[dst] = bd;
    }
  });
  st->workers_census = std::max(st->workers_census, (u64)created);
  for (size_t c = 0; c < nchunks; ++c) {
    st->census.nodes += lcs[c].nodes;
    st->census.leaf_tests += lcs[c].leaf_tests;
    st->census.owned_stacks += lcs[c].owned_stacks;
  }
  for (size_t c = 0; c < nchunks; ++c) {
    if (lrc[c] != 0) {
      if (m_publish_prefix) balls->swap(staged);
      return lrc[c] == 2 ? PipelineStatus::kResourceExhausted : PipelineStatus::kInvariantViolated;
    }
  }
  // Dent historique : une copie de payload reellement materialisee et lue,
  // sans toucher a l'objet. Le pic est releve PENDANT la coexistence.
  std::vector<BallData> shadow;
  if (m_keep_chunks) {
    shadow = staged;
    st->census_merge_peak_bytes += (u64)shadow.capacity() * (u64)sizeof(BallData);
  }
  const auto& counted = m_keep_chunks ? shadow : staged;
  const size_t dropped = m_drop_chunk && nchunks > 1
      ? expand_detail::chunk_plan(survivors.size(), threads).chunk : 0;
  for (size_t i = dropped; i < counted.size(); ++i) {
    st->census_interior += counted[i].n_interior;
    st->census_shell += counted[i].n_shell;
  }
  // Conserve la dent d'omission : supprimer exactement la premiere tranche,
  // jamais laisser une case nulle qui modifierait la classe d'echec aval.
  if (dropped) staged.erase(staged.begin(), staged.begin() + (std::ptrdiff_t)dropped);
  balls->swap(staged);
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
  size_t nchunks = expand_detail::chunk_plan(balls.size(), threads).count;
  std::vector<std::vector<KCount>> lc(nchunks, std::vector<KCount>((size_t)kmax + 1));
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
  const bool m_dense = MHGP7_MUTANT("dense-pointid");
  const bool m_keep_chunks = MHGP7_MUTANT("keep-ball-chunks");
  size_t nchunks = expand_detail::chunk_plan(balls.size(), threads).count;
  std::vector<std::vector<ForestEvent>> lev(nchunks);
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
        std::array<i32, kBallShellMax> tsorted{};
        std::copy(bd.shell().begin(), bd.shell().end(), tsorted.begin());
        std::sort(tsorted.begin(), tsorted.begin() + (std::ptrdiff_t)q);
        lev[c].push_back(expand_detail::make_event(ix, bd, std::span<const i32>(tsorted.data(), q),
                                                  bd.interior(), (u16)((1u << q) - 1u), m_dense));
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
  if (MHGP7_MUTANT("prefix-tamper-event-order") && kmax < 10 && K == kmax)
    for (std::vector<ForestEvent>& v : lev)
      for (ForestEvent& e : v)
        if (e.d >= 2) std::swap(e.interior[0], e.interior[1]);

  st->workers_expand = std::max(st->workers_expand, (u64)created);
  out->clear();
  size_t n_ev = 0;
  for (size_t c = 0; c < nchunks; ++c) n_ev += lev[c].size();
  out->reserve(n_ev);  // total de l'ordre K connu AVANT la fusion (= kc[K].events, revalide par l'appelant)
  for (size_t c = 0; c < nchunks; ++c) {
    out->insert(out->end(), lev[c].begin(), lev[c].end());
    if (!m_keep_chunks) std::vector<ForestEvent>().swap(lev[c]);  // meme motif : la tranche meurt des sa consommation
  }
  if (st->events_by_k.size() < (size_t)kmax + 1) st->events_by_k.assign((size_t)kmax + 1, 0);
  st->events_by_k[K] = out->size();
}

}  // namespace mhgp7
