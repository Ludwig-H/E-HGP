// MorseHGP3D v4 — RENDU § 9.1 : F_K^render, multiplicites d'incidence,
// niveaux de naissance des facettes (MATHEMATIQUES.md § 5.6).
//
// Contrat grave (§ 2.0 et audit « naissances, croissances et rendu ») :
//
//   F_K^render = TOUTES les facettes distinctes de tous les evenements —
//   les attachements nes au lot en sont membres pleins (le carre K=3 :
//   quatre triangles, tous attachements ; un rendu active-only serait
//   VIDE alors qu'un K-polyedre vient de naitre) ;
//
//   S_tau somme la contribution de CHAQUE K-simplexe incident :
//   Delta S_tau = mult_B(tau) * psi(r_B) pour chaque boule B, ou
//   mult_B(tau) = #{ T ⊆ U_B : |T| = K+1-|I_B|, c ∈ conv(T),
//   tau facette de I_B ∪ T }. Une compression par arbre couvrant est
//   exacte pour F_K^conn mais FAUSSE pour le § 9.1 — le rendu conserve
//   donc l'objet symbolique (facette -> (lot, multiplicite)) dont tout
//   psi decroissant se deduit ; le flux de plateaux enumere deja chaque
//   simplexe une fois, mult_B(tau) est l'agregation exacte.
//
//   Le niveau de naissance d'une facette ne se resume PAS au bit actif :
//   table FacetKey -> rho(facette)² par MINIBOULE EXACTE de ses <= 10
//   points — ne jamais supposer qu'une facette est elle-meme un evenement
//   de Gabriel de l'ordre inferieur.
//
// La connectivite (forest.hpp) et le rendu restent deux consommateurs
// distincts du meme flux d'evenements.
#pragma once

#include <algorithm>
#include <map>
#include <vector>

#include "../events/q2_event.hpp"
#include "forest.hpp"

namespace mhgp4 {

// Incidences d'une facette : (index de macro-lot, multiplicite), lots
// croissants. Les lots et leurs niveaux canoniques sont IDENTIQUES a ceux
// de build_forest (meme tri stable, meme egalite semantique U320).
struct FacetIncidences {
  FacetKey facet;
  std::vector<std::pair<u64, u64>> per_batch;
};

struct RenderResult {
  std::vector<FacetIncidences> facets;  // F_K^render, triees par FacetKey
  std::vector<Q4Level> batch_levels;    // representant canonique par lot
  u64 incidences = 0;                   // Σ multiplicites
};

// Agregation exacte des incidences. MUTANTS : `active_only` (seules les
// facettes actives entrent au rendu — l'erreur que la porte du carre K=3
// tue : rendu vide a la naissance) ; `collapse_mult` (multiplicite ecrasee
// a 1 par lot — la signature d'une compression par arbre couvrant, tuee
// par la porte de multiplicite du carre K=2).
inline RenderResult build_render(std::vector<ForestEvent> events,
                                 bool mutant_active_only = false,
                                 bool mutant_collapse_mult = false) {
  RenderResult r;
  std::stable_sort(events.begin(), events.end(),
                   [](const ForestEvent& x, const ForestEvent& y) {
                     return compare_exact_level(x.level, y.level) < 0;
                   });
  // Agregation par TRI (fold sort/reduce) : un enregistrement (facette,
  // lot) par incidence, tries puis reduits par plages — l'ancienne
  // map<FacetKey, map<u64, u64>> portait la pente ×2,8 du fold.
  struct RRec {
    FacetKey f;
    u64 batch;
  };
  std::vector<RRec> recs;
  {
    size_t total = 0;
    for (const ForestEvent& ev : events) total += (size_t)ev.q + ev.d;
    recs.reserve(total);
  }
  size_t e0 = 0;
  u64 batch = 0;
  while (e0 < events.size()) {
    size_t e1 = e0 + 1;
    while (e1 < events.size() &&
           same_exact_level(events[e1].level, events[e0].level))
      ++e1;
    for (size_t e = e0; e < e1; ++e) {
      const ForestEvent& ev = events[e];
      for (int s = 0; s < (int)ev.q; ++s) {
        if (mutant_active_only && !((ev.active_mask >> s) & 1u)) continue;
        recs.push_back({detail_forest::facet_minus(ev, s, -1), batch});
      }
      // Les retraits d'interieur sont toujours des attachements : le mutant
      // active-only les exclut aussi.
      if (!mutant_active_only)
        for (int z = 0; z < (int)ev.d; ++z)
          recs.push_back({detail_forest::facet_minus(ev, -1, z), batch});
    }
    r.batch_levels.push_back(events[e0].level);
    ++batch;
    e0 = e1;
  }
  std::stable_sort(recs.begin(), recs.end(), [](const RRec& x, const RRec& y) {
    if (!(x.f == y.f)) return x.f < y.f;
    return x.batch < y.batch;
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
      if (mutant_collapse_mult) m = 1;
      fi.per_batch.push_back({bt, m});
      r.incidences += m;
    }
    r.facets.push_back(std::move(fi));
    i = j;
  }
  return r;
}

namespace detail_render {

// Contenance FERMEE : P(z) = A|z|² + B·z + C <= 0 pour tous les points
// (coquille permise — la miniboule porte son support sur la coquille).
// Largeurs identiques au predicat de census (< 2^106, i128).
inline bool ball_contains_all(const Q3BallKey& key, const P3* p, int n) {
  for (int i = 0; i < n; ++i) {
    const i128 pw = key.a * p3_norm2(p[i]) + key.b[0] * p[i].x +
                    key.b[1] * p[i].y + key.b[2] * p[i].z + key.c;
    if (pw > 0) return false;
  }
  return true;
}

}  // namespace detail_render

// MINIBOULE EXACTE de k <= 10 points distincts : niveau rho² en Q4Level.
//
// Theoreme utilise : la miniboule a un support de 2 a 4 points dont elle
// est la boule circonscrite (diametrale pour une paire), et toute boule
// candidate CONTENANT tous les points a un niveau >= au sien — le MINIMUM
// sur les candidates contenantes suffit, sans test de convexite. Les
// candidates enumerees couvrent toujours la miniboule :
//   - paires : boules diametrales (toutes) ;
//   - triplets : STRICTEMENT aigus seulement (precondition de q3_form) —
//     un support rectangle a sa circonscrite egale a la diametrale de
//     l'hypotenuse (couverte par la paire), un obtus n'est jamais support ;
//   - quadruplets : non coplanaires (det != 0) — un support coplanaire
//     cocirculaire est couvert par un triplet aigu ou une paire.
// Rend false si k < 2 (une facette K=1 est un point : rho = 0, hors table).
inline bool facet_birth_level(const P3* p, int k, Q4Level* out) {
  bool have = false;
  Q4Level best{};
  const auto consider = [&](const Q3BallKey& key, const Q4Level& lvl) {
    if (!detail_render::ball_contains_all(key, p, k)) return;
    if (!have || compare_exact_level(lvl, best) < 0) {
      best = lvl;
      have = true;
    }
  };
  for (int i = 0; i < k; ++i)
    for (int j = i + 1; j < k; ++j) {
      const i64 D2 = p3_norm2(p3_sub(p[j], p[i]));
      consider(q2_ball_key(p[i], p[j]), promote_q3_level(q2_exact_level(D2)));
    }
  for (int i = 0; i < k; ++i)
    for (int j = i + 1; j < k; ++j)
      for (int l = j + 1; l < k; ++l) {
        // Arete maximale (a,b), sommet x ; strictement aigu exige
        // |2x - a - b|² > |ab|² (le meme predicat que le juge du probe).
        const int vs[3] = {i, j, l};
        int bu = 0, bv = 1;
        i64 bl2 = -1;
        for (int s0 = 0; s0 < 3; ++s0)
          for (int s1 = s0 + 1; s1 < 3; ++s1) {
            const i64 l2 = p3_norm2(p3_sub(p[vs[s1]], p[vs[s0]]));
            if (l2 > bl2) {
              bl2 = l2;
              bu = s0;
              bv = s1;
            }
          }
        const P3 &pa = p[vs[bu]], &pb = p[vs[bv]];
        int vx = -1;
        for (int s0 = 0; s0 < 3; ++s0)
          if (s0 != bu && s0 != bv) vx = s0;
        const P3& px = p[vs[vx]];
        const P3 vv{2 * px.x - pa.x - pb.x, 2 * px.y - pa.y - pb.y,
                    2 * px.z - pa.z - pb.z};
        if (p3_norm2(vv) <= bl2) continue;
        consider(q3_ball_key(q3_form(pa, pb, px)),
                 promote_q3_level(q3_exact_level(pa, pb, px)));
      }
  for (int i = 0; i < k; ++i)
    for (int j = i + 1; j < k; ++j)
      for (int l = j + 1; l < k; ++l)
        for (int m = l + 1; m < k; ++m) {
          const Q4Form f4 = q4_form(p[i], p[j], p[l], p[m]);
          if (f4.det == 0) continue;
          consider(q3_ball_key_reduce(q4_ball_form(f4)), q4_level_raw(f4));
        }
  if (have) *out = best;
  return have;
}

}  // namespace mhgp4
