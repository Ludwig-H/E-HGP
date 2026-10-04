// MorseHGP3D v6 — lane q3 : circumboule d'un triangle strictement aigu.
//
// Ancre (a,b) = arete maximale canonique (longueur carree maximale, ex aequo
// par plus petite EdgeKey). Porteur x dans lentille(ab) ∖ boule diametrale :
// |x-a|² <= D², |x-b|² <= D² (arete maximale faible, departagee par EdgeKey)
// et V² > D² avec V = 2x-a-b (acuite ⟺ positivite, STRICT : l'egalite est le
// triangle rectangle, dont le support retombe a l'arite 2).
//
// Forme de Gram (puissance sans centre) : d = b-a, u = x-a, D = d·d, E = u·u,
// F = d·u, G = DE - F² > 0, W = E(D-F) d + D(E-F) u ;
// P(z) = G|z-a|² - (z-a)·W : < 0 interieur strict, = 0 coquille.
// Largeurs u16 : D, E < 3·2^32 ; G < 9·2^64 ; |W_i| < 9·2^82 ; G|v|² < 2^103 ;
// |v·W| < 2^105 — i128.
// BallForm : (A, B, C) = (G, -(2G a + W), G|a|² + W·a) ; |B_i| < 2^86, |C| < 2^104.
// Niveau (rayon carre) : D·E·X/(4G), X = |b-x|² = D+E-2F ; D·E·X < 2^104.
// Profondeur par descente exacte de l'arbre : P separable par axe et convexe
// (minimum de reseau par axe aux entiers voisins du sommet). Elagage STRICT
// `mn > 0` : un nœud a mn == 0 peut porter une coquille (mutant `q3-prune-ge`).
#pragma once

#include <array>
#include <vector>

#include "../core/mutants.hpp"
#include "../tree/cloud_index.hpp"
#include "keys.hpp"
#include "level.hpp"

namespace mhgp7 {

struct Q3Form {
  i128 g = 0;  // G > 0
  i128 w[3] = {0, 0, 0};
  P3 a;
};

MHGP7_HD inline Q3Form q3_form(const P3& a, const P3& b, const P3& x) {
  const P3 d = p3_sub(b, a);
  const P3 u = p3_sub(x, a);
  const i64 D = p3_norm2(d);
  const i64 E = p3_norm2(u);
  const i64 F = p3_dot(d, u);
  Q3Form f;
  f.a = a;
  f.g = (i128)D * E - (i128)F * F;
  const i128 c1 = (i128)E * (D - F);
  const i128 c2 = (i128)D * (E - F);
  f.w[0] = c1 * d.x + c2 * u.x;
  f.w[1] = c1 * d.y + c2 * u.y;
  f.w[2] = c1 * d.z + c2 * u.z;
  return f;
}

MHGP7_HD inline i128 q3_power(const Q3Form& f, const P3& z) {
  const P3 v = p3_sub(z, f.a);
  return f.g * p3_norm2(v) - (f.w[0] * v.x + f.w[1] * v.y + f.w[2] * v.z);
}

MHGP7_HD inline BallForm q3_ball_form(const Q3Form& f) {
  BallForm r;
  r.a = f.g;
  const i64 ax[3] = {f.a.x, f.a.y, f.a.z};
  i128 wa = 0;
  for (int i = 0; i < 3; ++i) {
    r.b[i] = -(2 * f.g * ax[i] + f.w[i]);
    wa += f.w[i] * ax[i];
  }
  r.c = f.g * ((i128)ax[0] * ax[0] + (i128)ax[1] * ax[1] + (i128)ax[2] * ax[2]) + wa;
  return r;
}

inline BallKey q3_ball_key(const Q3Form& f) { return ball_key_reduce(q3_ball_form(f)); }

// Niveau brut (num > 0, den > 0 des la formation) puis canonique.
MHGP7_HD inline Rational128 q3_level_raw(const P3& a, const P3& b, const P3& x) {
  const i64 D = p3_norm2(p3_sub(b, a));
  const i64 E = p3_norm2(p3_sub(x, a));
  const i64 F = p3_dot(p3_sub(b, a), p3_sub(x, a));
  const i64 X = D + E - 2 * F;
  Rational128 l;
  l.num = (i128)D * E * X;
  l.den = 4 * ((i128)D * E - (i128)F * F);
  if (MHGP7_MUTANT("q3-level-4g")) l.den = ((i128)D * E - (i128)F * F);  // MUTANT : denominateur G au lieu de 4G
  return l;
}
inline Rational128 q3_exact_level(const P3& a, const P3& b, const P3& x) { return rational_reduce(q3_level_raw(a, b, x)); }

// (a,b) reste-t-elle l'arete OWNER du triangle {a,b,x} ?
MHGP7_HD inline bool anchor_owns_q3(i64 l_ab, i64 l_ax, i64 l_bx, PointId ida, PointId idb, PointId idx) {
  const EdgeKey e_ab = edge_key(ida, idb);
  if (l_ax > l_ab || (l_ax == l_ab && edge_key(ida, idx) < e_ab)) return false;
  if (l_bx > l_ab || (l_bx == l_ab && edge_key(idb, idx) < e_ab)) return false;
  return true;
}

// Le porteur x est-il un SEED AIGU canonique de l'ancre (a,b) ? (lentille,
// hors boule diametrale ⟺ acuite stricte, owner). Partage par q3 et q4.
MHGP7_HD inline bool is_acute_seed(const P3& pa, const P3& pb, const P3& px, i64 D2, PointId ida, PointId idb,
                                   PointId idx) {
  const i64 l_ax = p3_norm2(p3_sub(px, pa));
  const i64 l_bx = p3_norm2(p3_sub(px, pb));
  if (l_ax > D2 || l_bx > D2) return false;
  const P3 v{2 * px.x - pa.x - pb.x, 2 * px.y - pa.y - pb.y, 2 * px.z - pa.z - pb.z};
  if (p3_norm2(v) <= D2) return false;
  return anchor_owns_q3(D2, l_ax, l_bx, ida, idb, idx);
}

struct AcuteSeed {
  EdgeKey owner_edge;
  PointId a, b, carrier;
  i32 ua, ub, ux;
  Q3Form face_form;
};

namespace q3_detail {
inline i128 axis_val(const Q3Form& f, int i, i64 t) { return f.g * ((i128)t * t) - f.w[i] * t; }
inline i128 axis_min(const Q3Form& f, int i, i64 lo, i64 hi) {
  const i128 num = f.w[i];
  const i128 den = 2 * f.g;
  i128 q = num / den;
  if (num % den != 0 && ((num < 0) != (den < 0))) --q;
  const i64 t1 = (i64)q;
  i128 best = 0;
  bool first = true;
  for (const i64 cand : {t1, t1 + 1, lo, hi}) {
    const i64 c = std::min(std::max(cand, lo), hi);
    const i128 v = axis_val(f, i, c);
    if (first || v < best) { best = v; first = false; }
  }
  return best;
}
inline i128 axis_max(const Q3Form& f, int i, i64 lo, i64 hi) {
  return std::max(axis_val(f, i, lo), axis_val(f, i, hi));
}
}  // namespace q3_detail

// Profondeur (points STRICTEMENT interieurs, support exclu), ecretee a `cap`.
// `interior_out` (tampon de 8) collecte les index uniques interieurs ;
// `shell_extra` compte les coquilles rencontrees.
inline u64 q3_ball_depth(const CloudIndex& ix, const Q3Form& f, i32 ua, i32 ub, i32 ux, u64 cap,
                         u64* shell_extra = nullptr, i32* interior_out = nullptr, u8* interior_n = nullptr) {
  if (ix.unique_count() == 0) return 0;
  const bool prune_ge = MHGP7_MUTANT("q3-prune-ge");
  u64 count = 0;
  std::vector<NodeRef> stack{ix.root()};
  const auto skip_support = [&](i32 u) { return u == ua || u == ub || u == ux; };
  const auto record = [&](i32 u) {
    if (interior_out && interior_n && *interior_n < 8) interior_out[(*interior_n)++] = u;
  };
  const i64 ao[3] = {f.a.x, f.a.y, f.a.z};
  while (!stack.empty() && count < cap) {
    const NodeRef z = stack.back();
    stack.pop_back();
    const AxisBox bz = ix.box_of(z);
    i128 mn = 0, mx = 0;
    for (int i = 0; i < 3; ++i) {
      mn += q3_detail::axis_min(f, i, bz.lo[i] - ao[i], bz.hi[i] - ao[i]);
      mx += q3_detail::axis_max(f, i, bz.lo[i] - ao[i], bz.hi[i] - ao[i]);
    }
    if (prune_ge ? (mn >= 0) : (mn > 0)) continue;
    if (mx < 0 && !is_leaf(z)) {
      const NodeRange r = ix.range_of(z);
      if (interior_out) {
        for (i32 u = r.first; u <= r.last; ++u) {
          if (skip_support(u)) continue;
          record(u);
          count += ix.range_weight(u, u);
        }
      } else {
        u64 w = ix.range_weight(r.first, r.last);
        for (const i32 u : {ua, ub, ux})
          if (u >= r.first && u <= r.last) w -= ix.range_weight(u, u);
        count += w;
      }
      continue;
    }
    if (is_leaf(z)) {
      const i32 u = leaf_index(z);
      if (skip_support(u)) continue;
      const i128 p = q3_power(f, ix.upos[(size_t)u]);
      if (p < 0) {
        record(u);
        count += ix.range_weight(u, u);
      } else if (p == 0 && shell_extra) {
        ++*shell_extra;
      }
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  return count;
}

struct Q3Event {
  SupportKey3 support;
  EdgeKey owner;
  BallKey ball;
  Rational128 level;
  u8 depth = 0;
  std::array<PointId, 8> interior{};
};

}  // namespace mhgp7

