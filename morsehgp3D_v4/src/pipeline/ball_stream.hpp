// MorseHGP3D v4 — FLUX DE BOULES : les trois lanes WSPD comme generateurs,
// puis sort/RLE par BallKey et UN census par cle (l'ABI SpherePlateau de
// l'audit bloquant, version echelle).
//
// COMPLETUDE SOUS LES SEUILS h_q (derive_v4) : un plateau pertinent pour la
// foret (∃ σ = I_B ∪ T, |σ| <= K_max+1 = 11) dont le support minimal est
// d'arite q verifie |T| >= q donc |I_B| <= 11 - 1 - (q - 1) - ... plus
// simplement : |I_B| <= K_max + 1 - q ; ses temoins de fuseau
// W_q(a,b) ⊆ I_B sont donc au plus K_max + 1 - q - 1 < h_q = s_max - q + 1
// (s_max = K_max + 1). L'ancre du support minimal SURVIT toujours aux
// filtres h_coeur/h_a/h_b : les seuils du profil sont exactement calibres
// pour ne perdre aucun plateau pertinent. (Carathéodory garantit un support
// minimal d'arite 2, 3 ou 4 dans U_B — audit § 2.)
//
// CENSUS PAR CLE : la forme primitive (A, B, C) donne le predicat UNIFORME
// P(z) = A·|z|² + B·z + C (< 0 interieur strict, = 0 coquille), lanes
// confondues. Largeurs u16 : A < 2^68, |B_i| < 2^86, |C| < 2^104 ;
// A|z|² < 2^102, |B·z| < 2^105 → i128. Descente d'arbre separable par axe
// (parabole convexe, minimum de reseau aux entiers voisins du sommet).
//
// NIVEAU CANONIQUE PAR BOULE : au RLE, le representant de niveau retenu est
// celui du generateur d'ARITE MINIMALE (puis plus petite representation) —
// q2/q3 donnent des fractions canoniques, q4 un representant (|N'|², det²) ;
// sujet et juge appliquent la meme regle, les representants coincident.
#pragma once

#include <algorithm>
#include <vector>

#include "../events/acute_seed.hpp"
#include "../events/edge_cover.hpp"
#include "../events/q2_event.hpp"
#include "../events/q4_event.hpp"
#include "../events/witness_count.hpp"
#include "../wspd/wavefront.hpp"

namespace mhgp4 {

struct BallCandidate {
  Q3BallKey key;
  Q4Level level;
  u8 arity;  // arite du generateur (2, 3, 4)
};

inline bool ball_candidate_less(const BallCandidate& x, const BallCandidate& y) {
  if (!(x.key == y.key)) return x.key < y.key;
  if (x.arity != y.arity) return x.arity < y.arity;
  return x.level < y.level;  // representation : depart deterministe
}

// Statistiques du flux (compteurs, jamais une autorite).
struct BallStreamStats {
  u64 rect_alive[3] = {0, 0, 0};
  u64 anchors[3] = {0, 0, 0};
  u64 candidates[3] = {0, 0, 0};
  u64 unique_balls = 0;
  u64 census_interior = 0;
  u64 census_shell = 0;
  u64 balls_dead_depth = 0;  // |I_B| > K_max - 1 : aucun K <= K_max
};

namespace detail_bs {

// Vague WSPD ternaire d'une lane : rectangles vivants (h_coeur < h).
struct AliveRect {
  WspdRect r;
  u64 core;
};

inline void wspd_alive(const CloudIndex& ix, i64 s, const u64 h_of[3], int lane_idx,
                       u8 mask, u64 h, std::vector<AliveRect>* out) {
  out->clear();
  if (ix.nodes.empty()) return;
  std::vector<WspdRect> wave, next;
  for (const RadixNode& n : ix.nodes) wave.push_back(WspdRect{n.left, n.right});
  while (!wave.empty()) {
    next.clear();
    for (const WspdRect& r : wave) {
      const FusedCounts fc = count_universal_witnesses_234(ix, r.a, r.b, h_of, mask, false);
      if (fc.c[lane_idx] >= h) continue;
      i64 ba[3], bb[3];
      const auto va = detail::node_view(ix, r.a, ba);
      const auto vb = detail::node_view(ix, r.b, bb);
      if (detail::separated(va, vb, s, 1)) {
        const FusedCounts ff = count_universal_witnesses_234(ix, r.a, r.b, h_of, mask, true);
        if (ff.c[lane_idx] < h) out->push_back(AliveRect{r, ff.c[lane_idx]});
        continue;
      }
      const i64 w2a = detail::box_w2(va);
      const i64 w2b = detail::box_w2(vb);
      const bool split_a = (r.a >= 0) && (r.b < 0 || w2a >= w2b);
      const NodeRef keep = split_a ? r.b : r.a;
      const RadixNode& n = ix.nodes[(size_t)(split_a ? r.a : r.b)];
      next.push_back(split_a ? WspdRect{n.left, keep} : WspdRect{keep, n.left});
      next.push_back(split_a ? WspdRect{n.right, keep} : WspdRect{keep, n.right});
    }
    wave.swap(next);
  }
}

// Histogrammes h_a/h_b a 8 coins d'un rectangle, pour une lane.
inline void corner_histograms(const CloudIndex& ix, Lane lane, const AliveRect& ar,
                              std::vector<u64>* ha, std::vector<u64>* hb) {
  const NodeRange ra = range_of(ix, ar.r.a);
  const NodeRange rb = range_of(ix, ar.r.b);
  const AxisBox boxA = box_of_node(ix, ar.r.a);
  const AxisBox boxB = box_of_node(ix, ar.r.b);
  const int na = ra.last - ra.first + 1;
  const int nb = rb.last - rb.first + 1;
  ha->assign((size_t)na, 0);
  hb->assign((size_t)nb, 0);
  for (int ia = 0; ia < na; ++ia)
    for (int iz = 0; iz < na; ++iz) {
      if (iz == ia) continue;
      if (universal_over_corners(lane, ix.upos[(size_t)(ra.first + ia)], boxB,
                                 ix.upos[(size_t)(ra.first + iz)]))
        ++(*ha)[(size_t)ia];
    }
  for (int ib = 0; ib < nb; ++ib)
    for (int iz = 0; iz < nb; ++iz) {
      if (iz == ib) continue;
      if (universal_over_corners(lane, ix.upos[(size_t)(rb.first + ib)], boxA,
                                 ix.upos[(size_t)(rb.first + iz)]))
        ++(*hb)[(size_t)ib];
    }
}

}  // namespace detail_bs

// Collecte les boules candidates des trois lanes (generateurs seulement —
// AUCUN census ici : il se fait une fois par cle unique, en aval).
inline void collect_candidate_balls(const CloudIndex& ix, i64 s, u64 smax_eff,
                                    std::vector<BallCandidate>* out,
                                    BallStreamStats* st) {
  out->clear();
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax_eff), lane_h(Lane::kQ3, smax_eff),
                       lane_h(Lane::kQ4, smax_eff)};
  std::vector<detail_bs::AliveRect> alive;
  std::vector<u64> ha, hb;
  std::vector<CoverPoint> cover;
  std::vector<NodeRef> handles;
  u64 cover_nodes = 0, visits = 0;
  // ---- q2.
  detail_bs::wspd_alive(ix, s, h_of, 0, 0b001, h_of[0], &alive);
  st->rect_alive[0] = alive.size();
  for (const auto& ar : alive) {
    detail_bs::corner_histograms(ix, Lane::kQ2, ar, &ha, &hb);
    const NodeRange ra = range_of(ix, ar.r.a);
    const NodeRange rb = range_of(ix, ar.r.b);
    const u64 need = h_of[0] - ar.core;
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        ++st->anchors[0];
        if (ha[(size_t)(ua - ra.first)] + hb[(size_t)(ub - rb.first)] >= need)
          continue;
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        out->push_back(BallCandidate{q2_ball_key(pa, pb),
                                     promote_q3_level(q2_exact_level(D2)), 2});
        ++st->candidates[0];
      }
  }
  // ---- q3.
  detail_bs::wspd_alive(ix, s, h_of, 1, 0b010, h_of[1], &alive);
  st->rect_alive[1] = alive.size();
  for (const auto& ar : alive) {
    detail_bs::corner_histograms(ix, Lane::kQ3, ar, &ha, &hb);
    const NodeRange ra = range_of(ix, ar.r.a);
    const NodeRange rb = range_of(ix, ar.r.b);
    const AxisBox boxA = box_of_node(ix, ar.r.a);
    const AxisBox boxB = box_of_node(ix, ar.r.b);
    rect_cover_handles(ix, boxA, boxB, 3, false, &handles, &cover_nodes);
    const u64 need = h_of[1] - ar.core;
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        ++st->anchors[1];
        if (ha[(size_t)(ua - ra.first)] + hb[(size_t)(ub - rb.first)] >= need)
          continue;
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        anchor_cover_from_handles(ix, handles, pa, pb, D2, 3, &cover, &visits);
        for (const CoverPoint& cp : cover) {
          if (cp.u == ua || cp.u == ub) continue;
          const P3& px = ix.upos[(size_t)cp.u];
          if (!is_acute_seed(pa, pb, px, D2, ix.bucket_ids[ix.bucket_start[(size_t)ua]],
                             ix.bucket_ids[ix.bucket_start[(size_t)ub]],
                             ix.bucket_ids[ix.bucket_start[(size_t)cp.u]]))
            continue;
          const Q3Form f3 = q3_form(pa, pb, px);
          out->push_back(BallCandidate{q3_ball_key(f3),
                                       promote_q3_level(q3_exact_level(pa, pb, px)),
                                       3});
          ++st->candidates[1];
        }
      }
  }
  // ---- q4.
  detail_bs::wspd_alive(ix, s, h_of, 2, 0b100, h_of[2], &alive);
  st->rect_alive[2] = alive.size();
  for (const auto& ar : alive) {
    detail_bs::corner_histograms(ix, Lane::kQ4, ar, &ha, &hb);
    const NodeRange ra = range_of(ix, ar.r.a);
    const NodeRange rb = range_of(ix, ar.r.b);
    const AxisBox boxA = box_of_node(ix, ar.r.a);
    const AxisBox boxB = box_of_node(ix, ar.r.b);
    rect_cover_handles(ix, boxA, boxB, 3, false, &handles, &cover_nodes);
    const u64 need = h_of[2] - ar.core;
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        ++st->anchors[2];
        if (ha[(size_t)(ua - ra.first)] + hb[(size_t)(ub - rb.first)] >= need)
          continue;
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        anchor_cover_from_handles(ix, handles, pa, pb, D2, 3, &cover, &visits);
        const auto pid = [&](i32 u) { return ix.bucket_ids[ix.bucket_start[(size_t)u]]; };
        for (const CoverPoint& cx : cover) {
          if (cx.u == ua || cx.u == ub) continue;
          const P3& px = ix.upos[(size_t)cx.u];
          if (!is_acute_seed(pa, pb, px, D2, pid(ua), pid(ub), pid(cx.u))) continue;
          const i64 l_ax = p3_norm2(p3_sub(px, pa));
          const i64 l_bx = p3_norm2(p3_sub(px, pb));
          for (const CoverPoint& cy : cover) {
            const i32 uy = cy.u;
            if (uy == cx.u || uy == ua || uy == ub) continue;
            const P3& py = ix.upos[(size_t)uy];
            const i64 l_ay = p3_norm2(p3_sub(py, pa));
            const i64 l_by = p3_norm2(p3_sub(py, pb));
            if (l_ay > D2 || l_by > D2) continue;
            const i64 l_xy = p3_norm2(p3_sub(py, px));
            if (l_xy > D2) continue;
            if (!tetra_owned_by(D2, l_ax, l_ay, l_bx, l_by, l_xy, pid(ua),
                                pid(ub), pid(cx.u), pid(uy)))
              continue;
            // Exact-once du seed (le RLE dedupliquerait de toute facon ;
            // ceci borne le flux de candidats).
            const P3 vy{2 * py.x - pa.x - pb.x, 2 * py.y - pa.y - pb.y,
                        2 * py.z - pa.z - pb.z};
            if (p3_norm2(vy) > D2 && pid(uy) < pid(cx.u)) continue;
            const Q4Form f4 = q4_form(pa, pb, px, py);
            if (f4.det == 0) continue;
            if (!q4_center_strictly_inside(f4, pa, pb, px, py)) continue;
            out->push_back(BallCandidate{q3_ball_key_reduce(q4_ball_form(f4)),
                                         q4_level_raw(f4), 4});
            ++st->candidates[2];
          }
        }
      }
  }
}

// Census EXACT d'une boule par sa forme primitive : interieurs stricts et
// coquille complete. Descente separable par axe (parabole convexe par axe,
// minimum de reseau aux entiers voisins du sommet, maxima aux bornes).
// Retourne false si |I_B| depasse `interior_cap` (boule sans K <= K_max) OU
// si |U_B| depasse `shell_cap` (a traiter en resource_exhausted par
// l'appelant — jamais une troncature silencieuse).
// PRE-FILTRE DE PROFONDEUR (reçu flux reels : 98 % des boules uniques
// meurent en |I_B| > cap APRES avoir paye leur census complet — le census
// sortait tot mais feuille par feuille). Descente COMPTANTE : une boite
// entierement STRICTEMENT interieure (max P < 0 sur la boite) est avalee
// en O(1) par son compte de positions uniques — une boule profonde meurt
// en quelques visites, sans allocation. EXACT : un point de coquille
// (P == 0) n'est jamais compte (une feuille a mn == mx ; a 0 elle est
// coquille et passe), donc le filtre tue exactement les boules que le
// census aurait tuees. MUTANT nonstrict : les boites a max P <= 0
// comptees interieures — des boules a coquille meurent a tort, le juge
// le voit (evenements manquants).
inline bool ball_depth_exceeds(const CloudIndex& ix, const Q3BallKey& k,
                               size_t cap, bool mutant_nonstrict = false) {
  if (ix.nodes.empty()) return false;
  const auto axis_val = [&](int i, i64 t) { return k.a * ((i128)t * t) + k.b[i] * t; };
  const auto axis_min = [&](int i, i64 lo, i64 hi) {
    const i128 num = -k.b[i];
    const i128 den = 2 * k.a;
    i128 q = num / den;
    if (num % den != 0 && ((num < 0) != (den < 0))) --q;
    const i64 t1 = (i64)q;
    i128 best = 0;
    bool first = true;
    for (const i64 cand : {t1, t1 + 1, lo, hi}) {
      const i64 c = std::min(std::max(cand, lo), hi);
      const i128 v = axis_val(i, c);
      if (first || v < best) best = v, first = false;
    }
    return best;
  };
  const auto axis_max = [&](int i, i64 lo, i64 hi) {
    return std::max(axis_val(i, lo), axis_val(i, hi));
  };
  size_t count = 0;
  std::vector<NodeRef> stack{0};
  while (!stack.empty()) {
    const NodeRef z = stack.back();
    stack.pop_back();
    const AxisBox bz = box_of_node(ix, z);
    i128 mn = k.c, mx = k.c;
    for (int i = 0; i < 3; ++i) {
      mn += axis_min(i, bz.lo[i], bz.hi[i]);
      mx += axis_max(i, bz.lo[i], bz.hi[i]);
    }
    if (mn > 0) continue;
    if (mx < 0 || (mutant_nonstrict && mx <= 0)) {
      count += (z < 0)
                   ? 1
                   : (size_t)(ix.nodes[(size_t)z].last -
                              ix.nodes[(size_t)z].first + 1);
      if (count > cap) return true;
      continue;
    }
    if (z < 0) continue;  // feuille mn <= 0 <= mx : exactement la coquille
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  return false;
}

inline bool ball_census(const CloudIndex& ix, const Q3BallKey& k,
                        size_t interior_cap, size_t shell_cap,
                        std::vector<i32>* interior, std::vector<i32>* shell,
                        bool* shell_overflow) {
  interior->clear();
  shell->clear();
  *shell_overflow = false;
  if (ix.nodes.empty()) return true;
  const auto axis_val = [&](int i, i64 t) { return k.a * ((i128)t * t) + k.b[i] * t; };
  const auto axis_min = [&](int i, i64 lo, i64 hi) {
    const i128 num = -k.b[i];
    const i128 den = 2 * k.a;
    i128 q = num / den;
    if (num % den != 0 && ((num < 0) != (den < 0))) --q;
    const i64 t1 = (i64)q;
    i128 best = 0;
    bool first = true;
    for (const i64 cand : {t1, t1 + 1, lo, hi}) {
      const i64 c = std::min(std::max(cand, lo), hi);
      const i128 v = axis_val(i, c);
      if (first || v < best) best = v, first = false;
    }
    return best;
  };
  const auto axis_max = [&](int i, i64 lo, i64 hi) {
    return std::max(axis_val(i, lo), axis_val(i, hi));
  };
  std::vector<NodeRef> stack{0};
  while (!stack.empty()) {
    const NodeRef z = stack.back();
    stack.pop_back();
    const AxisBox bz = box_of_node(ix, z);
    i128 mn = k.c, mx = k.c;
    for (int i = 0; i < 3; ++i) {
      mn += axis_min(i, bz.lo[i], bz.hi[i]);
      mx += axis_max(i, bz.lo[i], bz.hi[i]);
    }
    if (mn > 0) continue;  // strict : mn == 0 descend (coquilles a voir)
    if (z < 0) {
      const i32 u = -1 - z;
      const P3& p = ix.upos[(size_t)u];
      const i128 pw = k.a * p3_norm2(p) + k.b[0] * p.x + k.b[1] * p.y +
                      k.b[2] * p.z + k.c;
      if (pw < 0) {
        interior->push_back(u);
        if (interior->size() > interior_cap) return false;
      } else if (pw == 0) {
        shell->push_back(u);
        if (shell->size() > shell_cap) {
          *shell_overflow = true;
          return false;
        }
      }
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  return true;
}

}  // namespace mhgp4
