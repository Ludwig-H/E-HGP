// MorseHGP3D v6 — comptage de temoins universels du cœur d'un rectangle.
//
// Minore |P ∩ W_q(a,b)| uniformement sur toutes les paires (a,b) de A×B par
// les temoins HORS A∪B (theoreme de disjonction : h_coeur, h_a, h_b sont
// disjoints). DESCENTE FUSIONNEE : une pile de nœuds, un masque de lanes
// ouvertes par sous-arbre, trois compteurs ecretes a h_q. Autorites :
//   - elagage commun Hmax⁴ (W_q ⊆ W_2) ;
//   - q2 : credit de sous-arbre par Hmin exact ;
//   - q3/q4 : credit de sous-arbre par boule-cœur ;
//   - feuille : (H, Xi) une fois par coin distinct pour q3 et q4.
// Masques de lanes obligatoires : un sous-arbre credite pour la lane q ne
// l'est plus pour elle en dessous (mutant `witness-no-lane-mask` : doubles
// credits, fausses morts, tue par le juge). Fail-open partout.
#pragma once

#include <vector>

#include "spindle.hpp"
#include "../core/inline_stack.hpp"

namespace mhgp7 {

struct FusedCounts {
  u64 c[3] = {0, 0, 0};
  u64 nodes_visited = 0;
  u64 corner_evals = 0;
};

namespace witness_detail {

inline u64 overlap_weight(const CloudIndex& ix, i32 f, i32 l, const NodeRange& r) {
  const i32 lo = std::max(f, r.first);
  const i32 hi = std::min(l, r.last);
  if (lo > hi) return 0;
  return ix.range_weight(lo, hi);
}

// Poids du nœud z prive de A et de B.
inline u64 credit_weight(const CloudIndex& ix, NodeRef z, const NodeRange& ra, const NodeRange& rb) {
  const NodeRange rz = ix.range_of(z);
  return ix.range_weight(rz.first, rz.last) - overlap_weight(ix, rz.first, rz.last, ra) -
         overlap_weight(ix, rz.first, rz.last, rb);
}

inline bool in_range(i32 u, const NodeRange& r) { return u >= r.first && u <= r.last; }

}  // namespace witness_detail

// Compte fusionne des trois lanes ; `mask_in` = lanes demandees (bit 0 = q2,
// 1 = q3, 2 = q4) ; `h[3]` seuils par lane ; `with_corners` active l'autorite
// de feuille q3/q4.
inline FusedCounts count_universal_witnesses(const CloudIndex& ix, NodeRef a, NodeRef b, const u64 h[3],
                                             u8 mask_in, bool with_corners) {
  FusedCounts fc;
  const AxisBox boxA = ix.box_of(a);
  const AxisBox boxB = ix.box_of(b);
  const NodeRange ra = ix.range_of(a);
  const NodeRange rb = ix.range_of(b);
  const bool no_mask = MHGP7_MUTANT("witness-no-lane-mask");
  CoreBall balls[3];
  if (mask_in & 0b010) balls[1] = core_ball(Lane::kQ3, boxA, boxB);
  if (mask_in & 0b100) balls[2] = core_ball(Lane::kQ4, boxA, boxB);

  struct Entry {
    NodeRef z;
    u8 open;
  };
  u8 mask_eff = mask_in;
  if (!with_corners) {  // sans autorite de feuille, une boule nulle ne credite rien
    if ((mask_eff & 0b010) && balls[1].radius4 == 0) mask_eff &= (u8)~0b010;
    if ((mask_eff & 0b100) && balls[2].radius4 == 0) mask_eff &= (u8)~0b100;
  }
  InlineStack<Entry, 64> stack;
  stack.push_back(Entry{ix.root(), mask_eff});
  const auto counting = [&]() {
    u8 m = 0;
    for (int li = 0; li < 3; ++li)
      if ((mask_eff & (1u << li)) && fc.c[li] < h[li]) m |= (u8)(1u << li);
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
    const AxisBox bz = ix.box_of(e.z);
    if (hmax4_boxes(boxA, boxB, bz) <= 0) continue;
    if ((e.open & 0b001) && hmin_boxes(boxA, boxB, bz) > 0) {
      fc.c[0] += witness_detail::credit_weight(ix, e.z, ra, rb);
      if (!no_mask) e.open &= (u8)~0b001;
    }
    for (int li = 1; li < 3; ++li)
      if ((e.open & (1u << li)) && balls[li].radius4 > 0) {
        const int side = box_vs_ball(bz, balls[li]);
        if (side > 0) {
          fc.c[li] += witness_detail::credit_weight(ix, e.z, ra, rb);
          if (!no_mask) e.open &= (u8)~(1u << li);
        } else if (side < 0 && !with_corners) {
          e.open &= (u8)~(1u << li);
        }
      }
    if (!e.open) continue;
    if (is_leaf(e.z)) {
      const i32 u = leaf_index(e.z);
      if (witness_detail::in_range(u, ra) || witness_detail::in_range(u, rb) || !(e.open & 0b110) ||
          !with_corners)
        continue;
      bool all3 = (e.open & 0b010) != 0, all4 = (e.open & 0b100) != 0;
      corner64_universal_34(boxA, boxB, ix.upos[(size_t)u], &all3, &all4, &fc.corner_evals);
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

// Collecte des index de positions uniques certifies temoins universels du
// cœur d'un rectangle VIVANT, pour la lane q (boule-cœur + 64 coins aux
// feuilles), hors A∪B, au plus `cap`. Precondition : compte < h_q.
inline u64 collect_universal_ids(Lane q, const CloudIndex& ix, NodeRef a, NodeRef b, u64 cap, i32* out) {
  const AxisBox boxA = ix.box_of(a);
  const AxisBox boxB = ix.box_of(b);
  const NodeRange ra = ix.range_of(a);
  const NodeRange rb = ix.range_of(b);
  const CoreBall cb = core_ball(q, boxA, boxB);
  u64 count = 0;
  const auto push_range = [&](i32 f, i32 l) {
    for (i32 u = f; u <= l && count < cap; ++u) {
      if (witness_detail::in_range(u, ra) || witness_detail::in_range(u, rb)) continue;
      out[count++] = u;
    }
  };
  std::vector<NodeRef> stack{ix.root()};
  while (!stack.empty() && count < cap) {
    const NodeRef z = stack.back();
    stack.pop_back();
    const AxisBox bz = ix.box_of(z);
    if (hmax4_boxes(boxA, boxB, bz) <= 0) continue;
    if (cb.radius4 > 0 && box_vs_ball(bz, cb) > 0) {
      const NodeRange rz = ix.range_of(z);
      push_range(rz.first, rz.last);
      continue;
    }
    if (is_leaf(z)) {
      const i32 u = leaf_index(z);
      if (!witness_detail::in_range(u, ra) && !witness_detail::in_range(u, rb) &&
          corner64_universal(q, boxA, boxB, ix.upos[(size_t)u]))
        out[count++] = u;
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  return count;
}

// JUGE ponctuel : |P ∩ W_q(a,b)| exact, ecrete a h, par test in_spindle sous
// elagage Hmax⁴ (a, b index de positions uniques).
inline u64 true_spindle_count(Lane q, const CloudIndex& ix, i32 ua, i32 ub, u64 h) {
  const P3& pa = ix.upos[(size_t)ua];
  const P3& pb = ix.upos[(size_t)ub];
  const AxisBox A = ix.box_of(leaf_ref(ua)), B = ix.box_of(leaf_ref(ub));
  u64 count = 0;
  std::vector<NodeRef> stack{ix.root()};
  while (!stack.empty() && count < h) {
    const NodeRef z = stack.back();
    stack.pop_back();
    const AxisBox bz = ix.box_of(z);
    if (hmax4_boxes(A, B, bz) <= 0) continue;
    if (is_leaf(z)) {
      const i32 u = leaf_index(z);
      if (u == ua || u == ub) continue;
      if (in_spindle(q, pa, pb, ix.upos[(size_t)u])) count += ix.range_weight(u, u);
      continue;
    }
    if (q == Lane::kQ2 && hmin_boxes(A, B, bz) > 0) {
      const NodeRange rz = ix.range_of(z);
      u64 w = ix.range_weight(rz.first, rz.last);
      if (witness_detail::in_range(ua, rz)) w -= ix.range_weight(ua, ua);
      if (witness_detail::in_range(ub, rz)) w -= ix.range_weight(ub, ub);
      count += w;
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  return count;
}

}  // namespace mhgp7

