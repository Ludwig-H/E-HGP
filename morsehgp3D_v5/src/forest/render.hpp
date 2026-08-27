// MorseHGP3D v5 — rendu § 9.1 du manuscrit : F_K^render, multiplicites
// d'incidence, niveaux de naissance des facettes.
//
// Contrat (docs/MATHEMATIQUES.md, rendu) :
//   F_K^render = TOUTES les facettes distinctes de tous les evenements — les
//   attachements nes au lot en sont membres pleins (le carre cocyclique en
//   K=3 : quatre triangles, tous attachements ; un rendu « actives seulement »
//   serait VIDE alors qu'un K-polyedre vient de naitre — mutant
//   `render-active-only`) ;
//   S_tau somme la contribution de CHAQUE K-simplexe incident : pour une
//   boule B, mult_B(tau) = #{T ⊆ U_B : |T| = K+1-|I_B|, c ∈ conv(T), tau
//   facette de I_B ∪ T} ; une compression par arbre couvrant est exacte pour
//   la connectivite mais FAUSSE ici (mutant `render-collapse-mult`) ;
//   le niveau de NAISSANCE d'une facette est le rayon carre de la miniboule
//   EXACTE de ses <= 10 points — jamais le niveau de sa premiere incidence
//   (mutant `birth-from-events`).
// Le rendu conserve l'objet symbolique facette -> (lot, multiplicite) ; tout
// psi decroissant (S_tau, T_x, m_tau, votes de la Prop. 7) s'en deduit en aval.
#pragma once

#include <algorithm>
#include <vector>

#include "../core/mutants.hpp"
#include "../lanes/q2.hpp"
#include "../lanes/q3.hpp"
#include "../lanes/q4.hpp"
#include "fold.hpp"

namespace mhgp5 {

struct FacetIncidences {
  FacetKey facet;
  std::vector<std::pair<u64, u64>> per_batch;  // (lot, multiplicite), lots croissants
};

struct RenderResult {
  std::vector<FacetIncidences> facets;  // F_K^render, triees par FacetKey
  std::vector<ExactLevel> batch_levels;
  u64 incidences = 0;
};

// Agregation exacte des incidences par tri (facette, lot) puis reduction par
// plages. Les lots sont IDENTIQUES a ceux de build_forest (meme tri stable,
// meme egalite semantique).
inline RenderResult build_render(const std::vector<ForestEvent>& events) {
  RenderResult r;
  const bool m_active = MHGP5_MUTANT("render-active-only");
  const bool m_collapse = MHGP5_MUTANT("render-collapse-mult");
  const std::vector<u32> order = sort_events_by_level(events);
  struct Rec {
    FacetKey f;
    u64 batch;
  };
  std::vector<Rec> recs;
  {
    size_t total = 0;
    for (const ForestEvent& ev : events) total += (size_t)ev.q + ev.d;
    recs.reserve(total);
  }
  size_t e0 = 0;
  u64 batch = 0;
  while (e0 < events.size()) {
    size_t e1 = e0 + 1;
    while (e1 < events.size() && same_exact_level(events[order[e1]].level, events[order[e0]].level)) ++e1;
    for (size_t e = e0; e < e1; ++e) {
      const ForestEvent& ev = events[order[e]];
      for (int s = 0; s < (int)ev.q; ++s) {
        if (m_active && !((ev.active_mask >> s) & 1u)) continue;
        recs.push_back({fold_detail::facet_minus(ev, s, -1), batch});
      }
      if (!m_active)
        for (int z = 0; z < (int)ev.d; ++z) recs.push_back({fold_detail::facet_minus(ev, -1, z), batch});
    }
    r.batch_levels.push_back(events[order[e0]].level);
    ++batch;
    e0 = e1;
  }
  std::stable_sort(recs.begin(), recs.end(), [](const Rec& x, const Rec& y) {
    return x.f != y.f ? x.f < y.f : x.batch < y.batch;
  });
  for (size_t i = 0; i < recs.size();) {
    FacetIncidences fi;
    fi.facet = recs[i].f;
    size_t j = i;
    while (j < recs.size() && recs[j].f == fi.facet) {
      const u64 bt = recs[j].batch;
      u64 m = 0;
      while (j < recs.size() && recs[j].f == fi.facet && recs[j].batch == bt) {
        ++m;
        ++j;
      }
      if (m_collapse) m = 1;
      fi.per_batch.push_back({bt, m});
      r.incidences += m;
    }
    r.facets.push_back(std::move(fi));
    i = j;
  }
  return r;
}

namespace render_detail {
// Contenance FERMEE : P(z) <= 0 pour tous les points (coquille permise).
inline bool ball_contains_all(const BallKey& key, const P3* p, int n) {
  for (int i = 0; i < n; ++i)
    if (key.power(p[i]) > 0) return false;
  return true;
}
}  // namespace render_detail

// MINIBOULE EXACTE de k <= 10 points distincts : rho² en ExactLevel.
// Theoreme : la miniboule a un support de 2 a 4 points dont elle est la boule
// circonscrite, et toute candidate CONTENANTE a un niveau >= au sien : le
// minimum sur les candidates contenantes suffit. Candidates : paires
// (diametrales), triplets STRICTEMENT aigus, quadruplets non coplanaires.
// Rend false si k < 2.
inline bool facet_birth_level(const P3* p, int k, ExactLevel* out) {
  bool have = false;
  ExactLevel best{};
  const auto consider = [&](const BallKey& key, const ExactLevel& lvl) {
    if (!render_detail::ball_contains_all(key, p, k)) return;
    if (!have || compare_exact_level(lvl, best) < 0) {
      best = lvl;
      have = true;
    }
  };
  for (int i = 0; i < k; ++i)
    for (int j = i + 1; j < k; ++j) {
      const i64 D2 = p3_norm2(p3_sub(p[j], p[i]));
      consider(q2_ball_key(p[i], p[j]), promote_level(q2_exact_level(D2)));
    }
  for (int i = 0; i < k; ++i)
    for (int j = i + 1; j < k; ++j)
      for (int l = j + 1; l < k; ++l) {
        const int vs[3] = {i, j, l};
        int bu = 0, bv = 1;
        i64 bl2 = -1;
        for (int s0 = 0; s0 < 3; ++s0)
          for (int s1 = s0 + 1; s1 < 3; ++s1) {
            const i64 l2 = p3_norm2(p3_sub(p[vs[s1]], p[vs[s0]]));
            if (l2 > bl2) { bl2 = l2; bu = s0; bv = s1; }
          }
        const P3 &pa = p[vs[bu]], &pb = p[vs[bv]];
        const P3& px = p[vs[3 - bu - bv]];
        const P3 vv{2 * px.x - pa.x - pb.x, 2 * px.y - pa.y - pb.y, 2 * px.z - pa.z - pb.z};
        if (p3_norm2(vv) <= bl2) continue;  // pas strictement aigu
        consider(q3_ball_key(q3_form(pa, pb, px)), promote_level(q3_exact_level(pa, pb, px)));
      }
  for (int i = 0; i < k; ++i)
    for (int j = i + 1; j < k; ++j)
      for (int l = j + 1; l < k; ++l)
        for (int m = l + 1; m < k; ++m) {
          const Q4Form f4 = q4_form(p[i], p[j], p[l], p[m]);
          if (f4.det == 0) continue;
          consider(ball_key_reduce(q4_ball_form(f4)), q4_level_raw(f4));
        }
  if (have) *out = best;
  return have;
}

}  // namespace mhgp5
