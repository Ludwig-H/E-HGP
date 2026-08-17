// MorseHGP3D v4 — INSTRUCTION q3 : de l'ancre survivante a l'evenement.
//
// Pour une ancre (a,b) d'arete maximale canonique, le porteur x d'un support
// q3 vit dans `lentille(ab) ∖ boule diametrale` :
//   - lentille : `|x-a|² <= D²` et `|x-b|² <= D²` (l'arete reste maximale
//     faible), egalite permise puis departagee par EdgeKey ;
//   - acuite ⟺ positivite (theoreme v3 § 5.1, recu : sous arete maximale
//     faible, seuls l'angle en x est a tester et il se lit sur `V² > D²`
//     avec `V = 2x-a-b` ; l'egalite est le triangle rectangle, dont le
//     support retombe a l'arite 2 — STRICT) ;
//   - owner canonique : l'arete maximale par longueur carree PUIS plus
//     petite EdgeKey=(min PointId, max PointId) — une egalite de longueur
//     avec ax ou bx dont l'EdgeKey est plus petite retire l'ancre (a,b).
//
// L'evenement est la circum-boule ambiante du triangle aigu. Puissance
// exacte sans centre (forme de Gram, v3 § 5.3, verifiee ici en largeurs) :
//   d = b-a, u = x-a, D = d·d, E = u·u, F = d·u, G = DE - F²  (> 0 ssi
//   affinement independant), W = E(D-F)·d + D(E-F)·u ;
//   P(z) = G·‖z-a‖² - (z-a)·W  :  P < 0 interieur strict, P = 0 shell,
//   P > 0 exterieur.
// Largeurs sous u16 : D,E < 3·2^32 ; F ≤ √(DE) < 3·2^32 ; G < 9·2^64 ;
// |W_i| ≤ E·D·|d_i| + D·E·|u_i| < 2·9·2^64·2^17 = 9·2^82 ;
// G·‖v‖² < 9·2^64·3·2^34 < 2^103 ; |v·W| < 3·2^17·9·2^82 < 2^105 → i128.
// La profondeur se compte par descente sur l'arbre : P est separable par
// axe et convexe (minimum de reseau par axe aux entiers voisins du sommet
// de la parabole, maximum aux extremites) — exact sur les points du reseau,
// aucun flottant.
#pragma once

#include "../tree/radix_tree.hpp"
#include "q2_witness_count.hpp"

namespace mhgp4 {

struct Q3Form {
  i128 g = 0;        // G > 0
  i128 w[3] = {0, 0, 0};
  P3 a;              // origine de la forme
};

// Construit la forme de Gram du triangle {a,b,x}. Precondition : triangle
// strictement aigu d'arete maximale (a,b) (G > 0 garanti).
inline Q3Form q3_form(const P3& a, const P3& b, const P3& x) {
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

// P(z) exact. i128 : voir les largeurs en tete.
inline i128 q3_power(const Q3Form& f, const P3& z) {
  const P3 v = p3_sub(z, f.a);
  return f.g * p3_norm2(v) - (f.w[0] * v.x + f.w[1] * v.y + f.w[2] * v.z);
}

namespace detail_q3 {

// Contribution d'un axe : g·t² - w·t pour t = z_i - a_i.
inline i128 axis_val(const Q3Form& f, int i, i64 t) {
  return f.g * ((i128)t * t) - f.w[i] * t;
}

// Minimum de RESEAU par axe sur [lo, hi] (parabole convexe : le minimum
// entier est a l'un des deux entiers voisins du sommet w/(2g), clippes).
inline i128 axis_min(const Q3Form& f, int i, i64 lo, i64 hi) {
  // sommet continu : t* = w_i / (2g) ; plancher rationnel par division i128.
  const i128 num = f.w[i];
  const i128 den = 2 * f.g;
  i128 q = num / den;
  if (num % den != 0 && ((num < 0) != (den < 0))) --q;  // vrai plancher
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

}  // namespace detail_q3

// Profondeur (points du nuage STRICTEMENT interieurs a la circum-boule),
// ecretee a `cap`, support {ua, ub, ux} exclu. Descente exacte : un nœud
// dont le minimum de reseau de P est >= 0 est elague ; un nœud dont le
// maximum est < 0 est credite en bloc.
inline u64 q3_ball_depth(const CloudIndex& ix, const Q3Form& f, i32 ua, i32 ub,
                         i32 ux, u64 cap, u64* shell_extra = nullptr) {
  if (ix.nodes.empty()) return 0;
  u64 count = 0;
  std::vector<NodeRef> stack{0};
  while (!stack.empty() && count < cap) {
    const NodeRef z = stack.back();
    stack.pop_back();
    const AxisBox bz = box_of_node(ix, z);
    i128 mn = 0, mx = 0;
    for (int i = 0; i < 3; ++i) {
      const i64 lo = bz.lo[i] - (i == 0 ? f.a.x : i == 1 ? f.a.y : f.a.z);
      const i64 hi = bz.hi[i] - (i == 0 ? f.a.x : i == 1 ? f.a.y : f.a.z);
      mn += detail_q3::axis_min(f, i, lo, hi);
      mx += detail_q3::axis_max(f, i, lo, hi);
    }
    // Elagage STRICT : `mn > 0` seulement. Un nœud a `mn == 0` peut porter
    // des points de coquille (P = 0) que le refus transactionnel des
    // extra-shell doit voir — il descend.
    if (mn > 0) continue;
    const auto skip_support = [&](i32 u) { return u == ua || u == ub || u == ux; };
    if (mx < 0 && z >= 0) {
      // Tout le sous-arbre est strictement interieur.
      const NodeRange r = range_of(ix, z);
      u64 w = ix.range_weight(r.first, r.last);
      for (const i32 u : {ua, ub, ux})
        if (u >= r.first && u <= r.last) w -= ix.range_weight(u, u);
      count += w;
      continue;
    }
    if (z < 0) {
      const i32 u = -1 - z;
      if (skip_support(u)) continue;
      const i128 p = q3_power(f, ix.upos[(size_t)u]);
      if (p < 0) count += ix.range_weight(u, u);
      else if (p == 0 && shell_extra) ++*shell_extra;
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  return count;
}

// EdgeKey canonique et comparaison owner : longueur carree maximale d'abord,
// puis PLUS PETITE EdgeKey (min PointId, max PointId).
struct EdgeKey {
  PointId lo, hi;
};
inline EdgeKey edge_key(PointId x, PointId y) {
  return x < y ? EdgeKey{x, y} : EdgeKey{y, x};
}
inline bool edge_key_less(const EdgeKey& p, const EdgeKey& q) {
  return p.lo != q.lo ? p.lo < q.lo : p.hi < q.hi;
}

// (a,b) reste-t-elle l'arete owner du triangle {a,b,x} ? Longueurs carrees
// l_ab, l_ax, l_bx ; l'owner maximise la longueur puis minimise l'EdgeKey.
inline bool anchor_owns_q3(i64 l_ab, i64 l_ax, i64 l_bx, PointId ida, PointId idb,
                           PointId idx) {
  const EdgeKey e_ab = edge_key(ida, idb);
  if (l_ax > l_ab || (l_ax == l_ab && edge_key_less(edge_key(ida, idx), e_ab)))
    return false;
  if (l_bx > l_ab || (l_bx == l_ab && edge_key_less(edge_key(idb, idx), e_ab)))
    return false;
  return true;
}

}  // namespace mhgp4
