// MorseHGP3D v4 — COMPTE DE TEMOINS UNIVERSELS q2 D'UN COUPLE DE NŒUDS.
//
// Minore `|P ∩ W_2(a,b)|` uniformément sur TOUTES les paires (a,b) du
// rectangle `A×B` (témoins du cœur, hors A∪B — théorème de disjonction,
// MATHEMATIQUES.md § 2.5). Deux autorités, essayées dans cet ordre :
//
//   1. BOULE-CŒUR (spindle_q2.hpp) : une seule descente de boule, crédits par
//      sous-arbres entiers, early-exit à `h`. Rapide, plus lâche.
//   2. DESCENTE `Hmin` : si la boule n'atteint pas `h`, le compte est REPRIS
//      de zéro avec l'autorité `Hmin` exacte (région ⊇ boule : reprendre
//      évite tout double comptage). `Hmin > 0` crédite le sous-arbre,
//      `Hmax⁴ ≤ 0` l'élague, feuille → H exact continu sur A×B.
//
// Les témoins de A∪B sont exclus par soustraction d'intervalles (A et B sont
// des plages disjointes de positions uniques dans l'ordre de Morton). Les
// multiplicités comptent : deux PointId à la même position font deux témoins.
// Fail-open : tout crédit est prouvé pour toute paire du rectangle continu,
// donc a fortiori pour les paires réelles.
#pragma once

#include "spindle_q2.hpp"

namespace mhgp4 {

struct NodeRange {
  i32 first, last;
};

inline NodeRange range_of(const CloudIndex& ix, NodeRef v) {
  if (v >= 0) return NodeRange{ix.nodes[(size_t)v].first, ix.nodes[(size_t)v].last};
  return NodeRange{-1 - v, -1 - v};
}

// Poids des positions uniques de [f,l] ∩ [r.first, r.last].
inline u64 overlap_weight(const CloudIndex& ix, i32 f, i32 l, const NodeRange& r) {
  const i32 lo = std::max(f, r.first);
  const i32 hi = std::min(l, r.last);
  if (lo > hi) return 0;
  return ix.range_weight(lo, hi);
}

struct Q2CountOpts {
  bool use_core_ball = true;
  bool use_hmin = true;
  bool mutant_ceil_distance = false;
};

namespace detail_q2c {

// Poids du nœud z privé de A et B.
inline u64 credit_weight(const CloudIndex& ix, NodeRef z, const NodeRange& ra,
                         const NodeRange& rb) {
  const NodeRange rz = range_of(ix, z);
  const u64 w = ix.range_weight(rz.first, rz.last);
  return w - overlap_weight(ix, rz.first, rz.last, ra) -
         overlap_weight(ix, rz.first, rz.last, rb);
}

inline bool point_in_ball(const CloudIndex& ix, i32 u, const CoreBall& cb) {
  const P3& p = ix.upos[(size_t)u];
  const i64 c[3] = {p.x, p.y, p.z};
  i128 d2 = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 v = 4 * c[i] - cb.center4[i];
    d2 += (i128)v * v;
  }
  return d2 < (i128)cb.radius4 * cb.radius4;
}

}  // namespace detail_q2c

// Compte (minorant) de témoins universels, écrêté à `h`.
inline u64 count_universal_witnesses_q2(const CloudIndex& ix, NodeRef a, NodeRef b,
                                        u64 h, const Q2CountOpts& opts) {
  if (ix.nodes.empty() && ix.unique_count() < 3) return 0;
  const AxisBox boxA = box_of_node(ix, a);
  const AxisBox boxB = box_of_node(ix, b);
  const NodeRange ra = range_of(ix, a);
  const NodeRange rb = range_of(ix, b);
  const NodeRef root = ix.nodes.empty() ? (NodeRef)(-1) : 0;

  // Phase 1 : boule-cœur.
  u64 ball_count = 0;
  if (opts.use_core_ball) {
    const CoreBall cb = core_ball_q2(boxA, boxB, opts.mutant_ceil_distance);
    if (cb.radius4 > 0) {
      u64& count = ball_count;
      std::vector<NodeRef> stack{root};
      while (!stack.empty() && count < h) {
        const NodeRef z = stack.back();
        stack.pop_back();
        const AxisBox bz = box_of_node(ix, z);
        const int side = node_vs_ball(bz, cb);
        if (side < 0) continue;
        if (side > 0) {
          count += detail_q2c::credit_weight(ix, z, ra, rb);
          continue;
        }
        if (z < 0) {
          const i32 u = -1 - z;
          const bool in_ab = (u >= ra.first && u <= ra.last) ||
                             (u >= rb.first && u <= rb.last);
          if (!in_ab && detail_q2c::point_in_ball(ix, u, cb))
            count += ix.range_weight(u, u);
          continue;
        }
        stack.push_back(ix.nodes[(size_t)z].left);
        stack.push_back(ix.nodes[(size_t)z].right);
      }
      if (count >= h) return count;
    }
  }

  // Phase 2 : descente Hmin, compte repris de zéro (région ⊇ boule, donc le
  // compte Hmin majore le compte boule : aucun double comptage possible).
  if (!opts.use_hmin) return ball_count;
  u64 count = 0;
  std::vector<NodeRef> stack{root};
  while (!stack.empty() && count < h) {
    const NodeRef z = stack.back();
    stack.pop_back();
    const AxisBox bz = box_of_node(ix, z);
    if (hmax4_boxes(boxA, boxB, bz) <= 0) continue;
    if (hmin_boxes(boxA, boxB, bz) > 0) {
      count += detail_q2c::credit_weight(ix, z, ra, rb);
      continue;
    }
    if (z < 0) continue;  // feuille : Hmin est deja exact sur A×B×{z}
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  return count;
}

// Juge ponctuel : nombre de points STRICTEMENT intérieurs à la boule
// diamétrale de (a,b), écrêté à `h`. C'est l'équivalence exacte du fuseau
// q2 ; il sert de vérité pour les portes fail-open. `a`, `b` sont des index
// de positions uniques.
inline u64 true_interior_count_q2(const CloudIndex& ix, i32 ua, i32 ub, u64 h) {
  const P3& pa = ix.upos[(size_t)ua];
  const P3& pb = ix.upos[(size_t)ub];
  AxisBox A{}, B{};
  const i64 ca[3] = {pa.x, pa.y, pa.z};
  const i64 cb2[3] = {pb.x, pb.y, pb.z};
  for (int i = 0; i < 3; ++i) {
    A.lo[i] = A.hi[i] = ca[i];
    B.lo[i] = B.hi[i] = cb2[i];
  }
  u64 count = 0;
  if (ix.nodes.empty()) return 0;
  std::vector<NodeRef> stack{0};
  while (!stack.empty() && count < h) {
    const NodeRef z = stack.back();
    stack.pop_back();
    const AxisBox bz = box_of_node(ix, z);
    if (hmax4_boxes(A, B, bz) <= 0) continue;
    if (z < 0) {
      const i32 u = -1 - z;
      if (u == ua || u == ub) continue;
      if (h_point(pa, pb, ix.upos[(size_t)u]) > 0) count += ix.range_weight(u, u);
      continue;
    }
    if (hmin_boxes(A, B, bz) > 0) {
      const NodeRange rz = range_of(ix, z);
      u64 w = ix.range_weight(rz.first, rz.last);
      if (ua >= rz.first && ua <= rz.last) w -= ix.range_weight(ua, ua);
      if (ub >= rz.first && ub <= rz.last) w -= ix.range_weight(ub, ub);
      count += w;
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  return count;
}

}  // namespace mhgp4
