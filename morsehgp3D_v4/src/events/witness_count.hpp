// MorseHGP3D v4 — COMPTE DE TEMOINS UNIVERSELS PAR ARITE (q2/q3/q4).
//
// Generalisation trois-lanes de q2_witness_count.hpp. Autorites par lane :
//
//   toutes  : boule-cœur d'arite (spindle.hpp), descente avec early-exit ;
//   q2      : + descente Hmin exacte (credits de sous-arbres entiers) ;
//   q3/q4   : + autorite 64 coins aux FEUILLES (pas de credit de sous-arbre
//             en v1 : le majorant de Xi par bloc n'est pas encore recu).
//
// L'elagage de la descente est commun aux trois lanes : W_q ⊆ W_2, donc la
// borne minimax sur H elague pour toutes les arites. Fail-open partout : un
// credit est prouve pour toute paire du rectangle continu.
#pragma once

#include "q2_witness_count.hpp"
#include "spindle.hpp"

namespace mhgp4 {

struct WitnessCountOpts {
  bool use_core_ball = true;
  bool use_descent = true;
  bool mutant_ceil_distance = false;
};

inline u64 count_universal_witnesses(Lane q, const CloudIndex& ix, NodeRef a,
                                     NodeRef b, u64 h, const WitnessCountOpts& opts) {
  if (q == Lane::kQ2) {
    Q2CountOpts o;
    o.use_core_ball = opts.use_core_ball;
    o.use_hmin = opts.use_descent;
    o.mutant_ceil_distance = opts.mutant_ceil_distance;
    return count_universal_witnesses_q2(ix, a, b, h, o);
  }
  const AxisBox boxA = box_of_node(ix, a);
  const AxisBox boxB = box_of_node(ix, b);
  const NodeRange ra = range_of(ix, a);
  const NodeRange rb = range_of(ix, b);
  const NodeRef root = ix.nodes.empty() ? (NodeRef)(-1) : 0;

  u64 ball_count = 0;
  if (opts.use_core_ball) {
    const CoreBall cb = core_ball(q, boxA, boxB, opts.mutant_ceil_distance);
    if (cb.radius4 > 0) {
      std::vector<NodeRef> stack{root};
      while (!stack.empty() && ball_count < h) {
        const NodeRef z = stack.back();
        stack.pop_back();
        const AxisBox bz = box_of_node(ix, z);
        const int side = node_vs_ball(bz, cb);
        if (side < 0) continue;
        if (side > 0) {
          ball_count += detail_q2c::credit_weight(ix, z, ra, rb);
          continue;
        }
        if (z < 0) {
          const i32 u = -1 - z;
          const bool in_ab =
              (u >= ra.first && u <= ra.last) || (u >= rb.first && u <= rb.last);
          if (!in_ab && detail_q2c::point_in_ball(ix, u, cb))
            ball_count += ix.range_weight(u, u);
          continue;
        }
        stack.push_back(ix.nodes[(size_t)z].left);
        stack.push_back(ix.nodes[(size_t)z].right);
      }
      if (ball_count >= h) return ball_count;
    }
  }

  if (!opts.use_descent) return ball_count;
  // Descente : elagage H commun, credit feuille par 64 coins. Compte repris
  // de zero (la region 64-coins contient la boule-cœur).
  u64 count = 0;
  std::vector<NodeRef> stack{root};
  while (!stack.empty() && count < h) {
    const NodeRef z = stack.back();
    stack.pop_back();
    const AxisBox bz = box_of_node(ix, z);
    if (hmax4_boxes(boxA, boxB, bz) <= 0) continue;
    if (z < 0) {
      const i32 u = -1 - z;
      const bool in_ab =
          (u >= ra.first && u <= ra.last) || (u >= rb.first && u <= rb.last);
      if (!in_ab && corner64_universal(q, boxA, boxB, ix.upos[(size_t)u]))
        count += ix.range_weight(u, u);
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  return count;
}

// Juge ponctuel trois-lanes : |P ∩ W_q(a,b)| exact, ecrete a h, par test
// `in_spindle` point a point sous elagage H (W_q ⊆ W_2).
inline u64 true_spindle_count(Lane q, const CloudIndex& ix, i32 ua, i32 ub, u64 h) {
  if (q == Lane::kQ2) return true_interior_count_q2(ix, ua, ub, h);
  const P3& pa = ix.upos[(size_t)ua];
  const P3& pb = ix.upos[(size_t)ub];
  AxisBox A{}, B{};
  const i64 ca[3] = {pa.x, pa.y, pa.z};
  const i64 cbp[3] = {pb.x, pb.y, pb.z};
  for (int i = 0; i < 3; ++i) {
    A.lo[i] = A.hi[i] = ca[i];
    B.lo[i] = B.hi[i] = cbp[i];
  }
  if (ix.nodes.empty()) return 0;
  u64 count = 0;
  std::vector<NodeRef> stack{0};
  while (!stack.empty() && count < h) {
    const NodeRef z = stack.back();
    stack.pop_back();
    const AxisBox bz = box_of_node(ix, z);
    if (hmax4_boxes(A, B, bz) <= 0) continue;
    if (z < 0) {
      const i32 u = -1 - z;
      if (u == ua || u == ub) continue;
      if (in_spindle(q, pa, pb, ix.upos[(size_t)u])) count += ix.range_weight(u, u);
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  return count;
}

}  // namespace mhgp4
