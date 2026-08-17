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
                                     NodeRef b, u64 h, const WitnessCountOpts& opts);

namespace detail_wc {
// Descente Hmin exacte de la lane q2, reprise du delegue historique.
inline u64 q2_hmin_phase(const CloudIndex& ix, NodeRef a, NodeRef b, u64 h) {
  Q2CountOpts o;
  o.use_core_ball = false;
  return count_universal_witnesses_q2(ix, a, b, h, o);
}
}  // namespace detail_wc

inline u64 count_universal_witnesses(Lane q, const CloudIndex& ix, NodeRef a,
                                     NodeRef b, u64 h, const WitnessCountOpts& opts) {
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
  if (q == Lane::kQ2) return detail_wc::q2_hmin_phase(ix, a, b, h);
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

// DESCENTE FUSIONNEE (etape 3 de l'audit du 17 aout) : une seule pile de
// nœuds temoins, un masque de lanes OUVERTES par sous-arbre, trois
// compteurs. Autorites par nœud :
//   - elagage commun `Hmax` minimax (W_q ⊆ W_2 : vaut pour les trois lanes) ;
//   - q2 : credit de sous-arbre par `Hmin` exact (subsume la boule q2 : un
//     sous-arbre inclus dans la boule a Hmin > 0) ;
//   - q3/q4 : credit de sous-arbre par leur boule-cœur max(R_dec, R_coup) ;
//   - feuille (si `with_corners`) : UNE evaluation (H, Xi) par coin distinct
//     sert les deux lanes q3/q4 a la fois.
// Aucun compte n'est repris de zero : chaque sous-arbre est attribue une
// seule fois par lane (le bit de lane se ferme au credit). Fail-open.
struct FusedCounts {
  u64 c[3] = {0, 0, 0};
  u64 nodes_visited = 0;
  u64 corner_evals = 0;
};

inline FusedCounts count_universal_witnesses_234(const CloudIndex& ix, NodeRef a,
                                                 NodeRef b, const u64 h[3],
                                                 u8 mask_in, bool with_corners,
                                                 bool mutant_ceil = false) {
  FusedCounts fc;
  const AxisBox boxA = box_of_node(ix, a);
  const AxisBox boxB = box_of_node(ix, b);
  const NodeRange ra = range_of(ix, a);
  const NodeRange rb = range_of(ix, b);
  CoreBall balls[3];
  if (mask_in & 0b010) balls[1] = core_ball(Lane::kQ3, boxA, boxB, mutant_ceil);
  if (mask_in & 0b100) balls[2] = core_ball(Lane::kQ4, boxA, boxB, mutant_ceil);

  struct Entry {
    NodeRef z;
    u8 open;
  };
  const NodeRef root = ix.nodes.empty() ? (NodeRef)(-1) : 0;
  std::vector<Entry> stack{{root, mask_in}};
  const auto counting = [&]() {
    u8 m = 0;
    for (int li = 0; li < 3; ++li)
      if ((mask_in & (1u << li)) && fc.c[li] < h[li]) m |= (u8)(1u << li);
    return m;
  };
  while (!stack.empty()) {
    const u8 still = counting();
    if (!still) break;
    Entry e = stack.back();
    stack.pop_back();
    e.open &= still;
    if (!e.open) continue;
    ++fc.nodes_visited;
    const AxisBox bz = box_of_node(ix, e.z);
    if (hmax4_boxes(boxA, boxB, bz) <= 0) continue;  // aucun temoin possible
    // Credits de sous-arbre.
    if ((e.open & 0b001) && hmin_boxes(boxA, boxB, bz) > 0) {
      fc.c[0] += detail_q2c::credit_weight(ix, e.z, ra, rb);
      e.open &= (u8)~0b001;
    }
    for (int li = 1; li < 3; ++li)
      if ((e.open & (1u << li)) && balls[li].radius4 > 0) {
        const int side = node_vs_ball(bz, balls[li]);
        if (side > 0) {
          fc.c[li] += detail_q2c::credit_weight(ix, e.z, ra, rb);
          e.open &= (u8)~(1u << li);
        } else if (side < 0 && !with_corners) {
          // Sans autorite de feuille, la boule est la seule voie q3/q4 :
          // un sous-arbre disjoint de la boule n'apportera rien a la lane.
          e.open &= (u8)~(1u << li);
        }
      }
    if (!e.open) continue;
    if (e.z < 0) {
      const i32 u = -1 - e.z;
      const bool in_ab =
          (u >= ra.first && u <= ra.last) || (u >= rb.first && u <= rb.last);
      if (in_ab || !(e.open & 0b110) || !with_corners) continue;
      // (H, Xi) une fois par coin distinct, pour q3 et q4 a la fois.
      const P3& z = ix.upos[(size_t)u];
      bool all3 = (e.open & 0b010) != 0, all4 = (e.open & 0b100) != 0;
      P3 pa{}, pb{};
      for (int ia = 0; ia < 8 && (all3 || all4); ++ia) {
        if (((ia & 1) && boxA.hi[0] == boxA.lo[0]) ||
            ((ia & 2) && boxA.hi[1] == boxA.lo[1]) ||
            ((ia & 4) && boxA.hi[2] == boxA.lo[2]))
          continue;
        pa.x = (ia & 1) ? boxA.hi[0] : boxA.lo[0];
        pa.y = (ia & 2) ? boxA.hi[1] : boxA.lo[1];
        pa.z = (ia & 4) ? boxA.hi[2] : boxA.lo[2];
        for (int ib = 0; ib < 8 && (all3 || all4); ++ib) {
          if (((ib & 1) && boxB.hi[0] == boxB.lo[0]) ||
              ((ib & 2) && boxB.hi[1] == boxB.lo[1]) ||
              ((ib & 4) && boxB.hi[2] == boxB.lo[2]))
            continue;
          pb.x = (ib & 1) ? boxB.hi[0] : boxB.lo[0];
          pb.y = (ib & 2) ? boxB.hi[1] : boxB.lo[1];
          pb.z = (ib & 4) ? boxB.hi[2] : boxB.lo[2];
          ++fc.corner_evals;
          const P3 w = p3_sub(z, pa);
          const P3 d = p3_sub(pb, pa);
          const i64 hh = p3_dot(d, w) - p3_norm2(w);
          if (hh <= 0) {
            all3 = all4 = false;
            break;
          }
          const P3 c = p3_cross(d, w);
          const i128 xi = (i128)c.x * c.x + (i128)c.y * c.y + (i128)c.z * c.z;
          const i128 h2 = (i128)hh * hh;
          if (3 * h2 <= xi) all3 = false;
          if (2 * h2 <= xi) all4 = false;
        }
      }
      const u64 w = ix.range_weight(u, u);
      if (all3 && (e.open & 0b010)) fc.c[1] += w;
      if (all4 && (e.open & 0b100)) fc.c[2] += w;
      continue;
    }
    stack.push_back(Entry{ix.nodes[(size_t)e.z].left, e.open});
    stack.push_back(Entry{ix.nodes[(size_t)e.z].right, e.open});
  }
  for (int li = 0; li < 3; ++li) fc.c[li] = std::min(fc.c[li], h[li]);
  return fc;
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
