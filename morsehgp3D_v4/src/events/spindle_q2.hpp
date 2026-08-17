// MorseHGP3D v4 — FUSEAU q2 : bornes exactes de bloc et boule-cœur.
//
// Le fuseau q2 est EXACT : `W_2(a,b) = B°((a+b)/2, ‖ab‖/2)`, la boule
// diamétrale ouverte, et `z ∈ W_2 ⟺ H(z;a,b) = (z-a)·(b-z) > 0`
// (MATHEMATIQUES.md § 2.4). Un témoin STRICTEMENT intérieur tue ; le shell
// (H = 0) ne tue jamais — toutes les comparaisons de crédit sont strictes.
//
// Deux certificats de bloc, tous deux fail-open :
//
//  1. `Hmin` EXACT sur `Box(A) × Box(B) × Box(Z)` : H se développe en
//     `Σ_i [ z_i(a_i+b_i) - a_i b_i - z_i² ]`, séparable par axe ; le crochet
//     est bilinéaire en (a_i, b_i) — minimum à l'un des quatre coins — et
//     concave en z_i — minimum à l'une des deux extrémités. Huit combinaisons
//     par axe, minimum EXACT sur le produit continu des boîtes (theoreme_v3,
//     PROPOSITION § 6bis.3). `Hmin > 0` ⟹ tout point de Z est témoin
//     universel du rectangle entier.
//
//  2. BOULE-CŒUR : `B°(m, R_dec)` avec `m = (c_A+c_B)/2` et, pour des
//     extrémités contenues dans deux boules de rayons r_A, r_B à distance d :
//     `R_dec,2 = κ_2(d-r) - r/2 = d/2 - r` avec `r = r_A + r_B` et κ_2 = 1/2
//     (theoreme_v3 corrigé, PROPOSITION § 6bis.3ter). Cette boule est incluse
//     dans `W_2(a,b)` pour TOUT (a,b) des deux boules circonscrites aux
//     boîtes serrées. Toute l'arithmétique est DIRIGÉE : distance des centres
//     minorée (`floor_sqrt`), rayons majorés (`ceil_sqrt` VRAI plafond),
//     comparaison stricte `< R`. Le mutant `radius-ceil` (majorer la
//     distance) fabrique de fausses morts : la porte du probe le tue.
//
// Largeurs : deltas de boîtes en unités quadruplées < 2^19 ; carrés < 2^40 ;
// tout tient en i64, les produits croisés en i128 par prudence.
#pragma once

#include <algorithm>

#include "../tree/radix_tree.hpp"

namespace mhgp4 {

struct AxisBox {
  i64 lo[3], hi[3];
};

inline AxisBox box_of_node(const CloudIndex& ix, NodeRef v) {
  AxisBox b{};
  if (v >= 0) {
    for (int i = 0; i < 3; ++i) {
      b.lo[i] = ix.nodes[(size_t)v].tlo[i];
      b.hi[i] = ix.nodes[(size_t)v].thi[i];
    }
  } else {
    const P3& p = ix.upos[(size_t)(-1 - v)];
    const i64 c[3] = {p.x, p.y, p.z};
    for (int i = 0; i < 3; ++i) b.lo[i] = b.hi[i] = c[i];
  }
  return b;
}

// H ponctuel : (z-a)·(b-z). i64 sous u16 (|H| < 3·2^34).
inline i64 h_point(const P3& a, const P3& b, const P3& z) {
  const P3 w = p3_sub(z, a);
  const P3 t = p3_sub(b, z);
  return p3_dot(w, t);
}

namespace detail_q2 {

// Contribution d'un axe : f(a,b,z) = z(a+b) - ab - z².
inline i64 axis_term(i64 a, i64 b, i64 z) { return z * (a + b) - a * b - z * z; }

}  // namespace detail_q2

// Minimum EXACT de H sur le produit continu des trois boîtes.
inline i64 hmin_boxes(const AxisBox& A, const AxisBox& B, const AxisBox& Z) {
  i64 total = 0;
  for (int i = 0; i < 3; ++i) {
    i64 m = INT64_MAX;
    for (const i64 a : {A.lo[i], A.hi[i]})
      for (const i64 b : {B.lo[i], B.hi[i]})
        for (const i64 z : {Z.lo[i], Z.hi[i]})
          m = std::min(m, detail_q2::axis_term(a, b, z));
    total += m;
  }
  return total;
}

// Borne MINIMAX d'élagage (échelle QUATRE). Un z est témoin universel ssi
// `min_{a,b} H(z) > 0` ; le sous-arbre Z s'élague quand
// `max_z min_{a,b} H <= 0`. Élaguer sur `max_{a,b,z} H` serait beaucoup trop
// lâche — c'est en gros l'union des boules diamétrales du rectangle (leçon
// v3, PROPOSITION § 6bis.4). Majorant employé, par l'inégalité minimax et la
// séparabilité par axe : `max_z min_ab H <= min_ab max_z H = Σ_i min sur les
// quatre coins (a_i, b_i) de [ (b-a)² - (y-s)² ]` avec `s = a+b`,
// `y = clip(s, [2zl, 2zh])` (max_z par axe, échelle quatre). Sûr, jamais
// utilisé pour créditer.
inline i64 hmax4_boxes(const AxisBox& A, const AxisBox& B, const AxisBox& Z) {
  i64 total = 0;
  for (int i = 0; i < 3; ++i) {
    i64 m = INT64_MAX;
    for (const i64 a : {A.lo[i], A.hi[i]})
      for (const i64 b : {B.lo[i], B.hi[i]}) {
        const i64 s = a + b;
        const i64 y = std::clamp(s, 2 * Z.lo[i], 2 * Z.hi[i]);
        m = std::min(m, (b - a) * (b - a) - (y - s) * (y - s));
      }
    total += m;
  }
  return total;
}

// floor(sqrt(x)) exact sur i64 >= 0.
inline i64 floor_sqrt(i64 x) {
  if (x <= 0) return 0;
  i64 r = (i64)__builtin_sqrt((double)x);
  while (r > 0 && (i128)r * r > x) --r;
  while ((i128)(r + 1) * (r + 1) <= x) ++r;
  return r;
}

// VRAI plafond : r = floor_sqrt(x) ; si r² < x alors r+1. Jamais isqrt(x)+1.
inline i64 ceil_sqrt(i64 x) {
  const i64 r = floor_sqrt(x);
  return ((i128)r * r < x) ? r + 1 : r;
}

// Boule-cœur q2 d'un couple de nœuds : centre 4m = c2_A + c2_B (unités
// quadruplées, entier) et rayon sous-approché 4R = d2u_lo − 2(rA2u_hi +
// rB2u_hi) où d2u = 2d (distance des centres doublés minorée) et
// r?2u = diagonale de boîte majorée (= 2r). Rendu : rayon quadruplé (>0 si
// la boule est non vide), ou 0.
struct CoreBall {
  i64 center4[3];  // 4m, entier
  i64 radius4 = 0;  // 4R sous-approché ; 0 = pas de boule
};

// `mutant_ceil_distance` : majore la distance au lieu de la minorer — le
// défaut d'arrondi que la porte du probe doit tuer.
inline CoreBall core_ball_q2(const AxisBox& A, const AxisBox& B,
                             bool mutant_ceil_distance = false) {
  CoreBall cb{};
  i64 d2q = 0, w2a = 0, w2b = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 ca2 = A.lo[i] + A.hi[i];
    const i64 cb2 = B.lo[i] + B.hi[i];
    cb.center4[i] = ca2 + cb2;
    const i64 u = cb2 - ca2;
    d2q += u * u;
    const i64 wa = A.hi[i] - A.lo[i];
    const i64 wb = B.hi[i] - B.lo[i];
    w2a += wa * wa;
    w2b += wb * wb;
  }
  const i64 d2u = mutant_ceil_distance ? ceil_sqrt(d2q) : floor_sqrt(d2q);
  const i64 r4 = d2u - 2 * (ceil_sqrt(w2a) + ceil_sqrt(w2b));
  cb.radius4 = std::max<i64>(0, r4);
  return cb;
}

// Position du nœud par rapport à la boule (unités quadruplées, strict) :
//  +1 : boîte entièrement STRICTEMENT intérieure ; -1 : disjointe ; 0 : mixte.
inline int node_vs_ball(const AxisBox& box, const CoreBall& cb) {
  i128 far2 = 0, near2 = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 lo4 = 4 * box.lo[i] - cb.center4[i];
    const i64 hi4 = 4 * box.hi[i] - cb.center4[i];
    const i64 far1 = std::max(std::llabs(lo4), std::llabs(hi4));
    far2 += (i128)far1 * far1;
    i64 near1 = 0;
    if (lo4 > 0) near1 = lo4;
    else if (hi4 < 0) near1 = -hi4;
    near2 += (i128)near1 * near1;
  }
  const i128 r2 = (i128)cb.radius4 * cb.radius4;
  if (far2 < r2) return +1;   // strictement intérieur (coquille exclue)
  if (near2 >= r2) return -1;  // aucun point strictement intérieur
  return 0;
}

}  // namespace mhgp4
