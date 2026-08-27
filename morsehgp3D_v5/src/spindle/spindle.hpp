// MorseHGP3D v5 — fuseaux de mort W_q(a,b) emboites : tests ponctuels, bornes
// de bloc, boule-cœur par arite, autorites aux coins.
//
// Avec w = z-a, d = b-a, H = d·w - |w|², Xi = |d×w|² :
//   W_2 : H > 0 ;   W_3 : H > 0 et 3H² > Xi ;   W_4 : H > 0 et 2H² > Xi.
// W_4 ⊂ W_3 ⊂ W_2 ; (H, Xi) ne dependent pas de l'arite : une evaluation sert
// les trois lanes. z ∈ W_q(a,b) ⟹ z tue tout support d'arite q d'ancre (a,b)
// (minorant de temoins, fail-open ; la reciproque est fausse en q3/q4).
// q2 est EXACT : W_2 = boule diametrale ouverte, H = (z-a)·(b-z).
//
// Bornes de bloc sur Box(A)×Box(B)×Box(Z) :
//   Hmin EXACT — H est separable par axe, bilineaire en (a_i,b_i) (minimum a
//   un coin) et concave en z_i (minimum a un bout) ; Hmin > 0 ⟹ tout point de
//   Z est temoin universel du rectangle (credit de sous-arbre, q2) ;
//   Hmax⁴ — majorant minimax `min_ab max_z H` (echelle quatre) : <= 0 elague
//   le sous-arbre pour les trois lanes, jamais utilise pour crediter.
//
// Boule-cœur d'arite q : B°(m, R) ⊆ W_q(a,b) pour tout (a,b) des deux boules
// circonscrites aux boites, avec κ_2 = 1/2, κ_3 = 1/(2√3), κ_4 = sin 15° ;
// rayon quadruple = max(4R_dec, 4R_coup), constantes point-fixe SOUS-approchees
// a preuves compilables, arithmetique dirigee (distance minoree, rayons
// majores, divisions plancher, comparaison stricte). Le mutant
// `core-ball-ceil-distance` majore la distance : fausses morts, tue par le juge.
//
// Autorites aux coins : ALL aux 8×8 coins DISTINCTS de Box(A)×Box(B) est un
// certificat SUFFISANT de temoin universel (sens consomme) ; ALL aux 8 coins
// de Box(T) pour {s}×Box(T) est EXACT (cone convexe en t, audit v4 17 aout).
#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdlib>

#include "../core/intmath.hpp"
#include "../core/mutants.hpp"
#include "../tree/cloud_index.hpp"

namespace mhgp5 {

enum class Lane : int { kQ2 = 2, kQ3 = 3, kQ4 = 4 };
inline constexpr Lane kLanes[3] = {Lane::kQ2, Lane::kQ3, Lane::kQ4};
inline constexpr int lane_index(Lane q) { return (int)q - 2; }
inline constexpr int lane_arity(Lane q) { return (int)q; }

// Seuil de mort de la lane : h_q = s_max - q + 1 (0 si la lane n'existe pas).
inline constexpr u64 lane_h(Lane q, u64 smax) {
  return smax >= (u64)(int)q ? smax - (u64)(int)q + 1 : 0;
}

// H ponctuel : (z-a)·(b-z). i64 sous u16.
MHGP5_HD inline i64 h_point(const P3& a, const P3& b, const P3& z) {
  return p3_dot(p3_sub(z, a), p3_sub(b, z));
}

// Test ponctuel exact d'appartenance a W_q(a,b). i128 : Xi < 2^72, 3H² < 2^74.
MHGP5_HD inline bool in_spindle(Lane q, const P3& a, const P3& b, const P3& z) {
  const P3 w = p3_sub(z, a);
  const P3 d = p3_sub(b, a);
  const i64 h = p3_dot(d, w) - p3_norm2(w);
  if (h <= 0) return false;
  if (q == Lane::kQ2) return true;
  const P3 c = p3_cross(d, w);
  const i128 xi = (i128)c.x * c.x + (i128)c.y * c.y + (i128)c.z * c.z;
  const i128 h2 = (i128)h * h;
  return (q == Lane::kQ3) ? (3 * h2 > xi) : (2 * h2 > xi);
}

namespace spindle_detail {
inline i64 axis_term(i64 a, i64 b, i64 z) { return z * (a + b) - a * b - z * z; }
}  // namespace spindle_detail

// Minimum EXACT de H sur le produit continu des trois boites.
inline i64 hmin_boxes(const AxisBox& A, const AxisBox& B, const AxisBox& Z) {
  i64 total = 0;
  for (int i = 0; i < 3; ++i) {
    i64 m = INT64_MAX;
    for (const i64 a : {A.lo[i], A.hi[i]})
      for (const i64 b : {B.lo[i], B.hi[i]})
        for (const i64 z : {Z.lo[i], Z.hi[i]}) m = std::min(m, spindle_detail::axis_term(a, b, z));
    total += m;
  }
  return total;
}

// Majorant minimax d'elagage, echelle quatre : <= 0 ⟹ aucun temoin dans Z.
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

// Constantes point-fixe SOUS-approchees de 2·κ_q, echelle D = 2^30.
inline constexpr i64 kSpindleD = 1ll << 30;
inline constexpr i64 kA2 = kSpindleD;   // 2κ_2 = 1
inline constexpr i64 kA3 = 619000000;   // < floor(2^30/√3) = 619925131
inline constexpr i64 kA4 = 555000000;   // < floor(2 sin15° · 2^30)
static_assert((i128)3 * kA3 * kA3 < (i128)kSpindleD * kSpindleD, "kA3 sous-approche 2^30/sqrt(3)");
static_assert((i128)2 * kSpindleD * kSpindleD - (i128)kA4 * kA4 > 0, "kA4 : X positif");
static_assert(((i128)2 * kSpindleD * kSpindleD - (i128)kA4 * kA4) *
                      ((i128)2 * kSpindleD * kSpindleD - (i128)kA4 * kA4) >
                  (i128)3 * kSpindleD * kSpindleD * kSpindleD * kSpindleD,
              "kA4 sous-approche 2 sin(15°) 2^30");
// Sur-approximations de (4κ_q²+1), echelle E = 2^20 (terme SOUSTRAIT).
inline constexpr i64 kCoupE = 1ll << 20;
inline constexpr i64 kC2 = 2 * kCoupE;
inline constexpr i64 kC3 = (4 * kCoupE + 2) / 3;
inline constexpr i64 kC4 = 1329545;  // > (3-√3)·2^20
static_assert(3 * kC3 >= 4 * kCoupE, "kC3 majore 4/3");
static_assert((i128)(3 * kCoupE - kC4) * (3 * kCoupE - kC4) < (i128)3 * kCoupE * kCoupE, "kC4 majore 3-sqrt(3)");

struct CoreBall {
  i64 center4[3];   // 4m, entier
  i64 radius4 = 0;  // 4R sous-approche ; 0 = pas de boule
};

// Boule-cœur d'arite q : 4R = max(A_q(d2u - r2u)/D - r2u, A_q d2u/D - ceil_sqrt(2 C_q S2/E)).
inline CoreBall core_ball(Lane q, const AxisBox& A, const AxisBox& B) {
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
  const i64 d2u = MHGP5_MUTANT("core-ball-ceil-distance") ? ceil_sqrt(d2q) : floor_sqrt(d2q);
  const i64 ra2u = ceil_sqrt(w2a);
  const i64 rb2u = ceil_sqrt(w2b);
  const i64 r2u = ra2u + rb2u;
  const i64 aq = (q == Lane::kQ2) ? kA2 : (q == Lane::kQ3) ? kA3 : kA4;
  const i64 cq = (q == Lane::kQ2) ? kC2 : (q == Lane::kQ3) ? kC3 : kC4;
  i64 r4 = 0;
  const i64 gap = d2u - r2u;
  if (gap > 0) r4 = (i64)(((i128)aq * gap) / kSpindleD) - r2u;
  const i64 s2 = ra2u * ra2u + rb2u * rb2u;
  const i64 sub2 = (2 * cq * s2 + kCoupE - 1) / kCoupE;  // ceil
  const i64 coup = (i64)(((i128)aq * d2u) / kSpindleD) - ceil_sqrt(sub2);
  r4 = std::max(r4, coup);
  cb.radius4 = std::max<i64>(0, r4);
  return cb;
}

// Position d'une boite par rapport a la boule (unites quadruplees, strict) :
// +1 entierement STRICTEMENT interieure ; -1 disjointe ; 0 mixte.
inline int box_vs_ball(const AxisBox& box, const CoreBall& cb) {
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
  if (far2 < r2) return +1;
  if (near2 >= r2) return -1;
  return 0;
}

inline bool point_in_ball(const P3& p, const CoreBall& cb) {
  const i64 c[3] = {p.x, p.y, p.z};
  i128 d2 = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 v = 4 * c[i] - cb.center4[i];
    d2 += (i128)v * v;
  }
  return d2 < (i128)cb.radius4 * cb.radius4;
}

namespace spindle_detail {
// Coin `bits` de la boite, ou false si ce coin duplique un coin deja
// enumere (boite plate sur cet axe).
inline bool corner(const AxisBox& B, int bits, P3* out) {
  if (((bits & 1) && B.hi[0] == B.lo[0]) || ((bits & 2) && B.hi[1] == B.lo[1]) ||
      ((bits & 4) && B.hi[2] == B.lo[2]))
    return false;
  out->x = (bits & 1) ? B.hi[0] : B.lo[0];
  out->y = (bits & 2) ? B.hi[1] : B.lo[1];
  out->z = (bits & 4) ? B.hi[2] : B.lo[2];
  return true;
}
}  // namespace spindle_detail

// ALL aux coins DISTINCTS de Box(A)×Box(B) ⟹ z temoin universel (suffisant).
inline bool corner64_universal(Lane q, const AxisBox& A, const AxisBox& B, const P3& z) {
  P3 a{}, b{};
  for (int ia = 0; ia < 8; ++ia) {
    if (!spindle_detail::corner(A, ia, &a)) continue;
    for (int ib = 0; ib < 8; ++ib) {
      if (!spindle_detail::corner(B, ib, &b)) continue;
      if (!in_spindle(q, a, b, z)) return false;
    }
  }
  return true;
}

// Une evaluation (H, Xi) par coin pour q3 ET q4 a la fois.
inline void corner64_universal_34(const AxisBox& A, const AxisBox& B, const P3& z, bool* all3, bool* all4,
                                  u64* evals) {
  P3 a{}, b{};
  for (int ia = 0; ia < 8 && (*all3 || *all4); ++ia) {
    if (!spindle_detail::corner(A, ia, &a)) continue;
    for (int ib = 0; ib < 8 && (*all3 || *all4); ++ib) {
      if (!spindle_detail::corner(B, ib, &b)) continue;
      ++*evals;
      const P3 w = p3_sub(z, a);
      const P3 d = p3_sub(b, a);
      const i64 hh = p3_dot(d, w) - p3_norm2(w);
      if (hh <= 0) {
        *all3 = *all4 = false;
        return;
      }
      const P3 c = p3_cross(d, w);
      const i128 xi = (i128)c.x * c.x + (i128)c.y * c.y + (i128)c.z * c.z;
      const i128 h2 = (i128)hh * hh;
      if (3 * h2 <= xi) *all3 = false;
      if (2 * h2 <= xi) *all4 = false;
    }
  }
}

// h_a : z temoin universel de {s}×Box(T) ssi W_q(s,t) aux coins distincts de T (EXACT).
inline bool universal_over_corners(Lane q, const P3& s, const AxisBox& T, const P3& z) {
  P3 t{};
  for (int it = 0; it < 8; ++it) {
    if (!spindle_detail::corner(T, it, &t)) continue;
    if (!in_spindle(q, s, t, z)) return false;
  }
  return true;
}

}  // namespace mhgp5
