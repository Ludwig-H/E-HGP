// MorseHGP3D v5 — lane q4 PAR LOTS (docs/GPU.md, livraison 4d, etage hote) :
// le contrat de la future lane q4 device, ecrit et prouve sur CPU, sur le
// modele de q3_lane_batched.hpp :
//   1. build_q4_batch : pour un rectangle vivant, tableaux plats — par ancre
//      survivante (histogramme, W_4) : sites affines SoA, positions, indices
//      de la lentille, indices de a et b ; par seed aigu de la lentille :
//      cœur SeedQ4D (J < 0 = mort d'office), face Q3FormD, l_ax, l_bx ;
//   2. un EXECUTEUR rend, par seed, le verdict du cœur et ses six compteurs,
//      puis pour chaque seed vivant et chaque completion y de la lentille
//      l'etage atteint (Q4Stage) et, si emission, la liste ordonnee des
//      paires (seed, y) — ici scan_q4_batch_host (boucles plates sur
//      q4_seed_core_shaped, q4_completion_stage_shaped, q4_depth_shaped) ;
//   3. emit_q4_batch : l'hote recalcule la forme i128 des paires emises et
//      pousse les candidats (cle, niveau) dans l'ordre de la production.
// tests/q4_lane_batched_gate.cpp : egalite post-RLE des candidats q4 et de
// tous les compteurs de la lane avec generate.hpp. Mutant de porte
// `q4-batched-emit-deep` : les candidats profonds sont emis (code 4).
// CONTRAT DE SYNCHRONISME de l'executeur `scan` : il est SYNCHRONE — a son
// retour, les verdicts (et pour q4 les emissions et etages) sont complets,
// l'appelant les lit puis vide le lot. Un futur recouvrement asynchrone
// exigerait des lots possedes ou double-bufferises et un handle de
// completion explicite ; ce n'est pas le contrat actuel.
#pragma once

#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "../core/mutants.hpp"
#include "../lanes/edge_cover.hpp"
#include "../lanes/q3.hpp"
#include "../lanes/q4.hpp"
#include "../pipeline/float_filter.hpp"
#include "../pipeline/generate.hpp"
#include "../spindle/spindle.hpp"
#include "q3_lane_batched.hpp"
#include "q4_completion_shaped.hpp"
#include "q4_core_shaped.hpp"

namespace mhgp5 {

struct Q4BatchAnchor {
  u32 begin = 0, count = 0;            // sites
  u32 lens_begin = 0, lens_count = 0;  // indices (dans lens_sites) des sites de la lentille
  u32 skip_a = 0, skip_b = 0;          // indices de a et b dans les sites (UINT32_MAX si absents)
  i64 D2 = 0;
  P3 a, b;
  PointId ida = 0, idb = 0;
};
struct Q4BatchSeed {
  SeedQ4D core;
  Q3FormD face;
  u32 anchor = 0;
  u32 x_site = 0;  // index de x dans les sites
  i64 l_ax = 0, l_bx = 0;
  u8 jneg = 0;     // J < 0 : mort d'office (compte seeds_killed_core)
};
struct Q4SeedVerdict {
  u32 dead = 0;
  Q4CoreCounters c;
};
struct Q4Emit {
  u32 seed = 0, y_site = 0;
};
struct Q4StageCounts {
  u64 completions = 0, lens = 0, owner = 0, once = 0, i64_ = 0, face = 0, det = 0, center = 0, deep = 0, emit = 0;
};

struct Q4Batch {
  u64 pairs_estimate = 0;  // somme sur les ancres de (seeds x sites de lentille) : borne des paires du lot
  std::vector<i64> u0, u1, u2, q;
  std::vector<i64> px, py, pz;
  std::vector<PointId> pid;
  std::vector<u32> site_index;  // wire G1 : indice de position unique par site (4 o), parallele a u0..pid
  std::vector<u32> lens_sites;
  std::vector<Q4BatchAnchor> anchors;
  std::vector<Q4BatchSeed> seeds;
  std::vector<Q4SeedVerdict> verdicts;  // executeur
  std::vector<Q4Emit> emits;            // executeur, ordre (seed, y)
  Q4StageCounts stages;                 // executeur
  void clear() {
    pairs_estimate = 0;
    u0.clear(); u1.clear(); u2.clear(); q.clear();
    px.clear(); py.clear(); pz.clear(); pid.clear(); site_index.clear();
    lens_sites.clear(); anchors.clear(); seeds.clear(); verdicts.clear(); emits.clear();
    stages = Q4StageCounts{};
  }
};

// VUE et CONTRAT STRUCTUREL d'un lot q4 (verifies avant tout scan et avant
// toute emission ; fail-closed) : sites < 2^32, huit SoA de meme taille,
// indices de lentille dans les sites, tranches d'ancres (sites et lentille)
// dans les tableaux, ancre de chaque seed valide et x_site dans la tranche,
// skip_a/skip_b dans la tranche ou UINT32_MAX ; apres scan : un verdict par
// seed, emissions ordonnees (seed croissant, y croissant dans la lentille),
// distinctes, issues de seeds vivants, y_site dans la tranche.
struct Q4BatchView {
  size_t n_index = 0;         // wire G1 (0 = absent)
  bool geom_declared = false;  // la geometrie residente est-elle declaree ? (distinct d'une borne nulle)
  size_t n_geom_points = 0;    // points de la geometrie residente (0 EST une borne valide)
  const u32* site_index = nullptr;
  size_t n_sites = 0, n_lens = 0, n_anchors = 0, n_seeds = 0, n_verdicts = 0, n_emits = 0;
  size_t soa_sizes[7] = {0, 0, 0, 0, 0, 0, 0};  // u1,u2,q,px,py,pz,pid
  const u32* lens_sites = nullptr;
  const Q4BatchAnchor* anchors = nullptr;
  const Q4BatchSeed* seeds = nullptr;
  const Q4SeedVerdict* verdicts = nullptr;
  const Q4Emit* emits = nullptr;
};
inline Q4BatchView q4_batch_view(const Q4Batch& b) {
  Q4BatchView v;
  v.n_sites = b.u0.size(); v.n_lens = b.lens_sites.size(); v.n_anchors = b.anchors.size(); v.n_seeds = b.seeds.size();
  v.n_verdicts = b.verdicts.size(); v.n_emits = b.emits.size(); v.n_index = b.site_index.size(); v.site_index = b.site_index.data();
  const size_t sz[7] = {b.u1.size(), b.u2.size(), b.q.size(), b.px.size(), b.py.size(), b.pz.size(), b.pid.size()};
  for (int i = 0; i < 7; ++i) v.soa_sizes[i] = sz[i];
  v.lens_sites = b.lens_sites.data(); v.anchors = b.anchors.data(); v.seeds = b.seeds.data();
  v.verdicts = b.verdicts.data(); v.emits = b.emits.data();
  return v;
}
inline bool validate_q4_batch_view(const Q4BatchView& v, std::string* why) {
  if (v.n_sites > (size_t)UINT32_MAX) { *why = "lot q4 : plus de 2^32 - 1 sites"; return false; }
  for (int i = 0; i < 7; ++i)
    if (v.soa_sizes[i] != v.n_sites) { *why = "lot q4 : tailles SoA differentes"; return false; }
  if (v.n_index != 0 && v.n_index != v.n_sites) { *why = "lot q4 : wire par indices de taille differente des sites"; return false; }
  // FAIL-CLOSED du wire G1 (audit du 28 aout) : chaque index est verifie EN
  // VALEUR contre la geometrie residente declaree — un index egal au nombre de
  // points, ou UINT32_MAX, lirait hors des tableaux du device.
  if (v.n_index != 0 && v.geom_declared) {
    if (!v.site_index) { *why = "lot q4 : wire par indices sans tableau d'indices"; return false; }
    for (size_t i = 0; i < v.n_index; ++i)
      if ((size_t)v.site_index[i] >= v.n_geom_points) { *why = "lot q4 : index de site hors de la geometrie residente"; return false; }
  }
  if (v.n_lens > (size_t)UINT32_MAX || v.n_anchors > (size_t)UINT32_MAX || v.n_seeds > (size_t)UINT32_MAX) {
    *why = "lot q4 : plus de 2^32 - 1 indices de lentille, ancres ou seeds"; return false;
  }
  for (size_t i = 0; i < v.n_lens; ++i)
    if (v.lens_sites[i] >= v.n_sites) { *why = "lot q4 : indice de lentille hors des sites"; return false; }
  for (size_t a = 0; a < v.n_anchors; ++a) {
    const Q4BatchAnchor& an = v.anchors[a];
    if ((u64)an.begin + an.count > v.n_sites) { *why = "lot q4 : tranche d'ancre hors des sites"; return false; }
    if ((u64)an.lens_begin + an.lens_count > v.n_lens) { *why = "lot q4 : tranche de lentille hors du tableau"; return false; }
    if (an.skip_a != UINT32_MAX && an.skip_a >= an.count) { *why = "lot q4 : skip_a hors de la tranche"; return false; }
    if (an.skip_b != UINT32_MAX && an.skip_b >= an.count) { *why = "lot q4 : skip_b hors de la tranche"; return false; }
    for (u32 li = an.lens_begin; li < an.lens_begin + an.lens_count; ++li)
      if (v.lens_sites[li] >= an.count) { *why = "lot q4 : indice de lentille hors de la tranche de son ancre"; return false; }
  }
  for (size_t i = 0; i < v.n_seeds; ++i) {
    const Q4BatchSeed& sd = v.seeds[i];
    if (sd.anchor >= v.n_anchors) { *why = "lot q4 : ancre de seed invalide"; return false; }
    if (sd.x_site >= v.anchors[sd.anchor].count) { *why = "lot q4 : x_site hors de la tranche"; return false; }
  }
  return true;
}
// Meme convention que q3 : `geom_points < 0` = geometrie ABSENTE (tailles
// seules), `>= 0` = geometrie DECLAREE de cette taille (valeurs bornees).
inline bool validate_q4_batch(const Q4Batch& b, std::string* why, long long geom_points = -1) {
  Q4BatchView v = q4_batch_view(b);
  v.geom_declared = geom_points >= 0;
  v.n_geom_points = geom_points >= 0 ? (size_t)geom_points : 0;
  return validate_q4_batch_view(v, why);
}
// Apres le scan.
// `emit_eq` : en nominal, le nombre d'emissions doit valoir st.emit (le mutant
// q4-batched-emit-deep, qui emet aussi les profonds, le desactive).
inline bool validate_q4_results_view(const Q4BatchView& v, const Q4StageCounts& st, std::string* why, bool emit_eq = true) {
  if (v.n_verdicts != v.n_seeds) { *why = "lot q4 : un verdict par seed attendu apres le scan"; return false; }
  if ((v.n_verdicts && !v.verdicts) || (v.n_emits && !v.emits) || (v.n_seeds && !v.seeds) || (v.n_anchors && !v.anchors) ||
      (v.n_lens && !v.lens_sites)) { *why = "lot q4 : pointeur nul avec un compte non nul"; return false; }
  const u64 parts[9] = {st.lens, st.owner, st.once, st.i64_, st.face, st.det, st.center, st.deep, st.emit};
  u64 sum = 0;
  for (const u64 x : parts) {
    if (sum > UINT64_MAX - x) { *why = "lot q4 : somme des etages debordante"; return false; }
    sum += x;
  }
  if (sum != st.completions) { *why = "lot q4 : la somme des etages ne vaut pas le nombre de completions"; return false; }
  if (emit_eq && st.emit != v.n_emits) { *why = "lot q4 : nombre d'emissions different de l'etage emit"; return false; }
  for (size_t e = 0; e < v.n_emits; ++e) {
    const Q4Emit& em = v.emits[e];
    if (em.seed >= v.n_seeds) { *why = "lot q4 : emission hors des seeds"; return false; }
    if (v.verdicts[em.seed].dead) { *why = "lot q4 : emission d'un seed mort"; return false; }
    const Q4BatchSeed& sd = v.seeds[em.seed];
    const Q4BatchAnchor& an = v.anchors[sd.anchor];
    if (em.y_site >= an.count) { *why = "lot q4 : y_site hors de la tranche"; return false; }
    if (em.y_site == sd.x_site || em.y_site == an.skip_a || em.y_site == an.skip_b) {
      *why = "lot q4 : y_site egal a x, a ou b"; return false;
    }
    bool in_lens = false;
    for (u32 li = an.lens_begin; li < an.lens_begin + an.lens_count && !in_lens; ++li) in_lens = v.lens_sites[li] == em.y_site;
    if (!in_lens) { *why = "lot q4 : y_site hors de la lentille de son ancre"; return false; }
    if (e > 0) {
      const Q4Emit& pr = v.emits[e - 1];
      if (pr.seed > em.seed || (pr.seed == em.seed && pr.y_site >= em.y_site)) {
        *why = "lot q4 : emissions non ordonnees ou non distinctes"; return false;
      }
    }
  }
  return true;
}
inline bool validate_q4_results(const Q4Batch& b, std::string* why, bool emit_eq = true) {
  return validate_q4_results_view(q4_batch_view(b), b.stages, why, emit_eq);
}

// Etage 1 : formation du lot (aucun verdict), AJOUT au lot courant ; `flush`
// apres chaque ancre des que le lot atteint `threshold` seeds. Une ancre sans
// seed n'est PAS materialisee (aucune completion possible). Compte anchors,
// anchors_killed_hist, anchors_killed_w4, seeds comme la lane de production.
template <class Flush>
inline void build_q4_batch(const CloudIndex& ix, const AliveRect& ar, const u64 h_of[3], bool float_on, i64 cover_coef,
                           bool depth_nonstrict, bool core_nonstrict, bool no_canonical,
                           generate_detail::AnchorScratch& sc, Q4Batch* bdev, std::vector<BallCandidate>* lo,
                           GenerateStats* ls, const BatchLimits& lim, BatchStats* bs, Flush&& flush) {
  using namespace generate_detail;
  corner_histograms(ix, Lane::kQ4, ar.r, &sc.ha, &sc.hb);
  const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
  rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), cover_coef, &sc.handles, &sc.cover_nodes);
  sc.handle_points = 0;
  for (const NodeRef h : sc.handles) { const NodeRange r = ix.range_of(h); sc.handle_points += (u64)(r.last - r.first + 1); }
  const bool pretest_by_query = sc.handle_points >= lim.pretest_query_min_points;
  if (pretest_by_query) rect_diametral_candidates(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), &sc.query, &sc.cover_nodes);
  const u64 need = h_of[2] - ar.core;
  for (i32 ua = ra.first; ua <= ra.last; ++ua)
    for (i32 ub = rb.first; ub <= rb.last; ++ub) {
      ++ls->anchors[2];
      if (sc.ha[(size_t)(ua - ra.first)] + sc.hb[(size_t)(ub - rb.first)] >= need) {
        ++ls->anchors_killed_hist[2];
        continue;
      }
      const P3& pa = ix.upos[(size_t)ua];
      const P3& pb = ix.upos[(size_t)ub];
      const i64 D2 = p3_norm2(p3_sub(pb, pa));
      if (D2 == 0) continue;
      if (pretest_by_query) {
        const int k = anchor_kill_from_candidates(sc.query, ix.upos, ua, ub, pa, pb, D2, Lane::kQ4, 8, h_of[2]);
        if (k == 1) { ++ls->anchors_killed_w4; continue; }
        if (k == 2) { ++ls->anchors_killed_sectors[2]; continue; }
      }
      anchor_cover_from_handles(ix, sc.handles, pa, pb, D2, cover_coef, &sc.cover, &sc.visits, &sc.cover_tmp);
      const bool ignore_threshold = MHGP5_MUTANT("route-ignore-threshold");
      if (sc.cover.size() < lim.device_min_sites && !ignore_threshold) {
        // Routage hote : lane de production par ancre, emission immediate (pretests deja faits si requete).
        const u64 seeds_before_h = ls->seeds[1];
        process_anchor_q4(ix, sc, ua, ub, pa, pb, D2, h_of[2], float_on, depth_nonstrict, core_nonstrict, no_canonical, lo, ls,
                          pretest_by_query ? AnchorPretests::kAlreadyApplied : AnchorPretests::kApply);
        ++bs->anchors_host;
        bs->seeds_host += ls->seeds[1] - seeds_before_h;
        continue;
      }
      if (!pretest_by_query) {
        u64 n4 = 0;
        for (const CoverPoint& cz : sc.cover) {
          if (cz.u == ua || cz.u == ub) continue;
          if (in_spindle(Lane::kQ4, pa, pb, ix.upos[(size_t)cz.u]) && ++n4 >= h_of[2]) break;
        }
        if (n4 >= h_of[2]) {
          ++ls->anchors_killed_w4;
          continue;
        }
        u64 wmin = 0;
        if (anchor_sector_kill(sc.cover, ix.upos, ua, ub, pa, pb, D2, 8, h_of[2], &wmin)) {
          ++ls->anchors_killed_sectors[2];
          continue;
        }
      }
      // Grille de cellules (theoreme 10.5) — le MEME etage que la production
      // (anchor_grid_stage), une seule fois par ancre : ancre entiere ou corde
      // de chaque seed (les seeds tues ne sont jamais materialises).
      if (anchor_grid_stage(ix, sc, ua, ub, pa, pb, D2, Lane::kQ4, h_of[2], float_on, ls)) continue;
      const i64 d4[3] = {pb.x - pa.x, pb.y - pa.y, pb.z - pa.z};
      const auto cell_dead_seed = [&](const P3& px) {
        if (!sc.grid.built) return false;
        const Q3Form f3s = q3_form(pa, pb, px);
        const P3 nrm = p3_cross(p3_sub(pb, pa), p3_sub(px, pa));
        return seed_chord_cell_dead(sc.grid, f3s, d4, nrm, D2, p3_norm2(p3_sub(px, pa)), p3_norm2(p3_sub(px, pb)));
      };
      // Seeds aigus de la lentille D'ABORD (sans materialiser) : une ancre
      // sans seed n'entre jamais dans le lot (rien a scanner ni a completer).
      const size_t nc = sc.cover.size();
      thread_local std::vector<u32> lens_idx;  // indices (dans le cover) des sites de la lentille
      lens_idx.clear();
      for (size_t i = 0; i < nc; ++i) {
        const P3& p = ix.upos[(size_t)sc.cover[i].u];
        if (p3_norm2(p3_sub(p, pa)) <= D2 && p3_norm2(p3_sub(p, pb)) <= D2) lens_idx.push_back((u32)i);
      }
      size_t nseeds = 0, nseeds_acute = 0;  // nseeds : seeds vivants apres la grille (materialises) ; acute : comptes comme en production
      for (const u32 li : lens_idx) {
        const i32 ux = sc.cover[li].u;
        if (ux == ua || ux == ub) continue;
        if (!is_acute_seed(pa, pb, ix.upos[(size_t)ux], D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(ux))) continue;
        ++nseeds_acute;
        if (!cell_dead_seed(ix.upos[(size_t)ux])) ++nseeds;
      }
      // PREFLIGHT (avant toute ecriture) : sites, seeds, paires (arithmetique
      // verifiee) ; ancre trop grande pour un lot -> corps de production ;
      // sinon vidage AVANT l'ajout qui depasserait un seuil.
      const u64 anchor_pairs_pre = (u64)nseeds * (u64)lens_idx.size();
      const bool pairs_overflow = nseeds != 0 && anchor_pairs_pre / nseeds != lens_idx.size();
      const bool oversized = nc > lim.sites || nseeds > lim.seeds || pairs_overflow || anchor_pairs_pre > lim.pairs ||
                             nc > (size_t)UINT32_MAX || lens_idx.size() > (size_t)UINT32_MAX;
      if (oversized) {
        const u64 seeds_before_h = ls->seeds[1];
        process_anchor_q4(ix, sc, ua, ub, pa, pb, D2, h_of[2], float_on, depth_nonstrict, core_nonstrict, no_canonical, lo, ls,
                          AnchorPretests::kAlreadyAppliedWithGrid);  // W_4, secteurs ET grille deja appliques ci-dessus : rien n'est reconstruit
        ++bs->anchors_host;
        ++bs->anchors_oversized;
        bs->seeds_host += ls->seeds[1] - seeds_before_h;
        continue;
      }
      ls->seeds[1] += nseeds_acute;
      ls->seeds_killed_cells[2] += nseeds_acute - nseeds;
      if (nseeds == 0) {
        ++bs->anchors_host;  // rien a scanner ni a completer : comptee cote hote, jamais materialisee
        continue;
      }
      Q4Batch* b = bdev;
      if (b->seeds.size() + nseeds > lim.seeds || b->u0.size() + nc > lim.sites ||
          b->pairs_estimate > lim.pairs - anchor_pairs_pre)
        flush();
      sc.fill_affine_sites(ix, pa, pb, D2);
      const size_t seeds_before = b->seeds.size();
      Q4BatchAnchor an;
      an.begin = (u32)b->u0.size();
      an.count = (u32)nc;
      an.skip_a = an.skip_b = std::numeric_limits<u32>::max();
      an.D2 = D2; an.a = pa; an.b = pb; an.ida = ix.point_id(ua); an.idb = ix.point_id(ub);
      an.lens_begin = (u32)b->lens_sites.size();
      for (size_t i = 0; i < nc; ++i) {
        const CoverPoint& cz = sc.cover[i];
        const P3& p = ix.upos[(size_t)cz.u];
        b->u0.push_back(sc.su0[i]); b->u1.push_back(sc.su1[i]); b->u2.push_back(sc.su2[i]); b->q.push_back(sc.sq[i]);
        b->px.push_back(p.x); b->py.push_back(p.y); b->pz.push_back(p.z);
        b->pid.push_back(ix.point_id(cz.u));
        b->site_index.push_back((u32)cz.u);  // wire G1
        if (cz.u == ua) an.skip_a = (u32)i;
        if (cz.u == ub) an.skip_b = (u32)i;
      }
      for (const u32 li : lens_idx) b->lens_sites.push_back(li);
      an.lens_count = (u32)b->lens_sites.size() - an.lens_begin;
      const u32 aidx = (u32)b->anchors.size();
      b->anchors.push_back(an);
      for (const u32 xs : lens_idx) {
        const CoverPoint& cx = sc.cover[xs];
        if (cx.u == ua || cx.u == ub) continue;
        const P3& px = ix.upos[(size_t)cx.u];
        if (!is_acute_seed(pa, pb, px, D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(cx.u))) continue;
        if (cell_dead_seed(px)) continue;  // deja compte dans seeds_killed_cells (precomptage)
        Q4BatchSeed sd;
        sd.anchor = aidx;
        sd.x_site = xs;
        sd.l_ax = p3_norm2(p3_sub(px, pa));
        sd.l_bx = p3_norm2(p3_sub(px, pb));
        const Q3Form f3s = q3_form(pa, pb, px);
        sd.face = q3_form_d(pa, pb, px);
        const P3 nrm = p3_cross(p3_sub(pb, pa), p3_sub(px, pa));
        const i128 Jb = (i128)D2 * (3 * f3s.g - 2 * (i128)sd.l_ax * sd.l_bx);
        sd.jneg = Jb < 0 ? 1 : 0;
        if (Jb < 0) ++ls->invariant_jneg;  // inatteignable par theoreme : violation d'invariant signalee
        const AffineSeed as(f3s, pa, pb, sc, float_on);
        sd.core.aff.G = di_from_i128(as.G);
        sd.core.aff.N0 = di_from_i128(as.N0); sd.core.aff.N1 = di_from_i128(as.N1); sd.core.aff.N2 = di_from_i128(as.N2);
        sd.core.aff.Gd = as.Gd; sd.core.aff.Nd0 = as.Nd0; sd.core.aff.Nd1 = as.Nd1; sd.core.aff.Nd2 = as.Nd2;
        sd.core.aff.bound = as.bound;
        sd.core.n0 = nrm.x; sd.core.n1 = nrm.y; sd.core.n2 = nrm.z;
        sd.core.J = di_from_i128(Jb < 0 ? (i128)0 : Jb);
        const double Jd = (double)Jb;
        sd.core.Jlo = Jd * (1.0 - kJungGuard);
        sd.core.Jhi = Jd * (1.0 + kJungGuard);
        sd.core.skip_x = xs;
        b->seeds.push_back(sd);
      }
      const u64 anchor_seeds = (u64)(b->seeds.size() - seeds_before);
      const u64 anchor_pairs = anchor_seeds * (u64)an.lens_count;  // == anchor_pairs_pre (verifie sans debordement)
      if (anchor_pairs != anchor_pairs_pre) throw std::logic_error("lot q4 : precomptage des paires incoherent");
      b->pairs_estimate += anchor_pairs;
      bs->max_anchor_seeds = std::max(bs->max_anchor_seeds, anchor_seeds);
      bs->max_anchor_sites = std::max(bs->max_anchor_sites, (u64)nc);
      bs->max_anchor_pairs = std::max(bs->max_anchor_pairs, anchor_pairs);
      ++bs->anchors_device;
      bs->seeds_device += anchor_seeds;
      if (b->seeds.size() >= lim.seeds || b->u0.size() >= lim.sites || b->pairs_estimate >= lim.pairs) flush();
    }
}

// Etage 2 (executeur hote) : cœurs puis completions, boucles plates.
inline void scan_q4_batch_host(Q4Batch* b, u32 h4, bool core_nonstrict, bool depth_nonstrict, bool no_canonical) {
  std::string why;
  if (!validate_q4_batch(*b, &why)) throw std::invalid_argument(why);
  b->verdicts.assign(b->seeds.size(), Q4SeedVerdict{});
  b->emits.clear();
  b->stages = Q4StageCounts{};
  for (size_t si = 0; si < b->seeds.size(); ++si) {
    const Q4BatchSeed& s = b->seeds[si];
    const Q4BatchAnchor& an = b->anchors[s.anchor];
    Q4SeedVerdict& v = b->verdicts[si];
    if (s.jneg) {
      v.dead = 1;
      continue;
    }
    const AnchorSitesSoA sites{b->u0.data() + an.begin, b->u1.data() + an.begin, b->u2.data() + an.begin,
                               b->q.data() + an.begin, an.count};
    v.dead = q4_seed_core_shaped(s.core, sites, an.skip_a, an.skip_b, h4, core_nonstrict, &v.c, MHGP5_MUTANT("chord-nonstrict")) ? 1u : 0u;
    if (v.dead) continue;
    const AnchorPositionsSoA pos{b->px.data() + an.begin, b->py.data() + an.begin, b->pz.data() + an.begin, an.count};
    const P3 x{b->px[an.begin + s.x_site], b->py[an.begin + s.x_site], b->pz[an.begin + s.x_site]};
    const PointId idx = b->pid[an.begin + s.x_site];
    for (u32 li = an.lens_begin; li < an.lens_begin + an.lens_count; ++li) {
      const u32 ys = b->lens_sites[li];
      if (ys == s.x_site || ys == an.skip_a || ys == an.skip_b) continue;
      ++b->stages.completions;
      const P3 y{b->px[an.begin + ys], b->py[an.begin + ys], b->pz[an.begin + ys]};
      Q4FormD f4{};
      Q4Stage st = q4_completion_stage_shaped(an.a, an.b, x, y, an.ida, an.idb, idx, b->pid[an.begin + ys], an.D2, s.l_ax,
                                              s.l_bx, s.face, no_canonical, false, &f4);
      if (st == Q4Stage::kEmit && q4_depth_shaped(f4, pos, h4, depth_nonstrict)) st = Q4Stage::kDeep;
      switch (st) {
        case Q4Stage::kRejLens: ++b->stages.lens; break;
        case Q4Stage::kRejOwner: ++b->stages.owner; break;
        case Q4Stage::kRejOnce: ++b->stages.once; break;
        case Q4Stage::kRejI64: ++b->stages.i64_; break;
        case Q4Stage::kRejFacePower: ++b->stages.face; break;
        case Q4Stage::kRejDet: ++b->stages.det; break;
        case Q4Stage::kRejCenter: ++b->stages.center; break;
        case Q4Stage::kDeep:
          ++b->stages.deep;
          if (MHGP5_MUTANT("q4-batched-emit-deep")) b->emits.push_back(Q4Emit{(u32)si, ys});
          break;
        case Q4Stage::kEmit:
          ++b->stages.emit;
          b->emits.push_back(Q4Emit{(u32)si, ys});
          break;
        default:
          throw std::logic_error("lot q4 : etage inconnu");
      }
    }
  }
}

// Etage 3 : compteurs et emission ordonnee (forme i128 recalculee par l'hote).
inline void emit_q4_batch(const Q4Batch& b, std::vector<BallCandidate>* lo, GenerateStats* ls) {
  std::string why;
  if (!validate_q4_batch(b, &why) || !validate_q4_results(b, &why, !MHGP5_MUTANT("q4-batched-emit-deep")))
    throw std::invalid_argument(why);
  for (const Q4SeedVerdict& v : b.verdicts) {
    if (v.dead) { if (v.c.dead_by_chord) ++ls->seeds_killed_chord; else ++ls->seeds_killed_core; }
    ls->float_cert_pos += v.c.cert_pos; ls->q4_cert[0] += v.c.cert_pos;
    ls->float_cert_neg += v.c.cert_neg; ls->q4_cert[1] += v.c.cert_neg;
    ls->jung_cert_kill += v.c.jung_kill; ls->q4_cert[2] += v.c.jung_kill;
    ls->jung_cert_skip += v.c.jung_skip; ls->q4_cert[3] += v.c.jung_skip;
    ls->jung_fallback += v.c.jung_fallback; ls->q4_cert[4] += v.c.jung_fallback;
    ls->float_fallback += v.c.float_fallback; ls->q4_cert[5] += v.c.float_fallback;
  }
  ls->q4_completions += b.stages.completions;
  ls->q4_rej_lens += b.stages.lens;
  ls->q4_rej_owner += b.stages.owner;
  ls->q4_rej_once += b.stages.once;
  ls->q4_rej_i64 += b.stages.i64_;
  ls->q4_rej_face_power += b.stages.face;
  ls->q4_rej_det += b.stages.det;
  ls->q4_rej_center += b.stages.center;
  ls->depth_killed[2] += b.stages.deep;
  for (const Q4Emit& e : b.emits) {
    const Q4BatchSeed& s = b.seeds[e.seed];
    const Q4BatchAnchor& an = b.anchors[s.anchor];
    const P3 x{b.px[an.begin + s.x_site], b.py[an.begin + s.x_site], b.pz[an.begin + s.x_site]};
    const P3 y{b.px[an.begin + e.y_site], b.py[an.begin + e.y_site], b.pz[an.begin + e.y_site]};
    const Q4Form f4 = q4_form(an.a, an.b, x, y);
    lo->push_back(BallCandidate{ball_key_reduce(q4_ball_form(f4)), q4_level_raw(f4), 4});
    ++ls->candidates[2];
  }
}

template <class Scan>
inline void generate_q4_batched_with(const CloudIndex& ix, const GenerateOptions& opt, std::vector<BallCandidate>* out,
                                     GenerateStats* st, Scan&& scan, BatchLimits lim = BatchLimits{},
                                     BatchStats* batch_stats = nullptr) {
  using namespace generate_detail;
  if (lim.seeds < 1 || lim.sites < 1 || lim.pairs < 1) throw std::invalid_argument("seuils de lot < 1");
  const bool float_on = float_filter_runtime_enabled();
  const bool depth_nonstrict = MHGP5_MUTANT("genfilter-nonstrict");
  const bool core_nonstrict = MHGP5_MUTANT("q4-seed-core-nonstrict");
  const bool no_canonical = MHGP5_MUTANT("q4-no-canonical");
  const i64 cover_coef = MHGP5_MUTANT("q4-cover-coef4") ? 4 : 3;
  const u64 h_of[3] = {lane_h(Lane::kQ2, opt.smax), lane_h(Lane::kQ3, opt.smax), lane_h(Lane::kQ4, opt.smax)};
  std::vector<AliveRect> alive;
  const auto t0 = std::chrono::steady_clock::now();
  alive_rectangles(ix, opt.s, h_of, 2, opt.threads, &alive, &st->rect_visited[2], &st->workers_wspd[2]);
  st->t_wspd_ms[2] += ms_since(t0);
  st->rect_alive[2] = alive.size();
  const auto t1 = std::chrono::steady_clock::now();
  const size_t nrect = alive.size();
  const size_t T = planned_workers(nrect, opt.threads);
  std::vector<std::vector<BallCandidate>> louts(T);
  std::vector<GenerateStats> lst(T);
  std::vector<AnchorScratch> lsc(T);
  for (AnchorScratch& x : lsc) x.cell_min_sites = opt.cell_grid_min_sites;  // une seule autorite (GenerateOptions)
  std::vector<Q4Batch> lb(T);
  std::vector<BatchStats> lbs(T);
  const auto flush = [&](size_t t) {
    Q4Batch& b = lb[t];
    if (b.seeds.empty() && b.anchors.empty()) return;
    lbs[t].max_lot_seeds = std::max(lbs[t].max_lot_seeds, (u64)b.seeds.size());
    lbs[t].max_lot_sites = std::max(lbs[t].max_lot_sites, (u64)b.u0.size());
    lbs[t].max_lot_pairs = std::max(lbs[t].max_lot_pairs, b.pairs_estimate);
    lbs[t].lot_seeds.add((u64)b.seeds.size());
    lbs[t].lot_sites.add((u64)b.u0.size());
    lbs[t].lot_pairs.add(b.pairs_estimate);
    ++lbs[t].flushes;
    scan(&b, (u32)h_of[2], core_nonstrict, depth_nonstrict, no_canonical);
    emit_q4_batch(b, &louts[t], &lst[t]);
    b.clear();
  };
  const size_t created = parallel_items(nrect, (int)T, [&](size_t i, size_t t) {
    build_q4_batch(ix, alive[i], h_of, float_on, cover_coef, depth_nonstrict, core_nonstrict, no_canonical, lsc[t], &lb[t],
                   &louts[t], &lst[t], lim, &lbs[t], [&] { flush(t); });
  });
  for (size_t t = 0; t < T; ++t) flush(t);
  st->workers_rects[2] = std::max(st->workers_rects[2], (u64)created);
  const bool drop = MHGP5_MUTANT("par-drop-shard");
  for (size_t t = 0; t < T; ++t) {
    if (drop && t == 0 && T > 1) continue;
    out->insert(out->end(), louts[t].begin(), louts[t].end());
    st->add_from(lst[t]);
    if (batch_stats) batch_stats->add_from(lbs[t]);
  }
  st->t_rects_ms[2] += ms_since(t1);
}

inline void generate_q4_batched(const CloudIndex& ix, const GenerateOptions& opt, std::vector<BallCandidate>* out,
                                GenerateStats* st, BatchLimits lim = BatchLimits{}, BatchStats* bs = nullptr) {
  generate_q4_batched_with(ix, opt, out, st, [](Q4Batch* b, u32 h4, bool cn, bool dn, bool nc) {
    scan_q4_batch_host(b, h4, cn, dn, nc);
  }, lim, bs);
}

}  // namespace mhgp5
