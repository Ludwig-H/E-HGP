// MorseHGP3D v4 — FUSEAUX W_q EMBOITES : tests ponctuels, boule-cœur par
// arite, autorite 64 coins.
//
// Avec `w = z-a`, `d = b-a`, `H = d·w - ‖w‖²`, `Xi = ‖d×w‖²` :
//   W_2 : H > 0 ;   W_3 : H > 0 et 3H² > Xi ;   W_4 : H > 0 et 2H² > Xi.
// Emboîtement W_4 ⊂ W_3 ⊂ W_2 ; (H, Xi) ne dependent pas de l'arite : une
// evaluation sert les trois lanes. Statut : formes reprises de la v3
// (PROPOSITION § 6bis.3) — PISTE a re-deriver par l'auditeur (Q6), gardee en
// attendant par le juge fail-open des probes.
//
// BOULE-CŒUR par arite : B°(m, κ_q(d-r) - r/2) ⊆ W_q(a,b) pour tout (a,b)
// des deux boules circonscrites, κ_2 = 1/2, κ_3 = 1/(2√3), κ_4 = sin 15°.
// Realisation point-fixe D = 2^30, `A_q <= floor(2 κ_q D)` SOUS-approche
// (les constantes ci-dessous sont volontairement en retrait du floor exact,
// avec preuves compilables ; le serrage au floor exact viendra avec sa
// preuve). Arithmetique dirigee : distance minoree, rayons majores, division
// plancher, comparaison stricte.
//
// AUTORITE 64 COINS (feuille temoin) : pour z fixe, le predicat W_q evalue
// aux 8×8 coins de Box(A)×Box(B). Claim v3 (`corner64_all_lane`) : ALL aux
// coins decide exactement l'enveloppe AABB continue, par convexite separee
// en a et b — PISTE (Q7 auditeur). La v4 ne consomme que le sens ALL
// (suffisant pour crediter un temoin universel) et le juge surveille.
#pragma once

#include "spindle_q2.hpp"

namespace mhgp4 {

enum class Lane : int { kQ2 = 2, kQ3 = 3, kQ4 = 4 };

inline u64 lane_h(Lane q, u64 smax) { return smax - (u64)(int)q + 1; }

// Test ponctuel exact d'appartenance a W_q(a,b). i128 : Xi < 2^72, 3H² < 2^74.
inline bool in_spindle(Lane q, const P3& a, const P3& b, const P3& z) {
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

// Constantes point-fixe SOUS-approchees de 2·κ_q, echelle D = 2^30.
inline constexpr i64 kSpindleD = 1ll << 30;
inline constexpr i64 kA2 = kSpindleD;      // 2·κ_2 = 1 exact
inline constexpr i64 kA3 = 619000000;      // < floor(2^30/√3) = 619925131
inline constexpr i64 kA4 = 555000000;      // < floor(2·sin15°·2^30)
// Preuves compilables (PROPOSITION § 6bis.3ter) : A_3 <= 2κ_3 D ⟺ 3A_3² <= D².
static_assert((i128)3 * kA3 * kA3 < (i128)kSpindleD * kSpindleD,
              "kA3 doit sous-approcher 2^30/sqrt(3)");
// A_4 <= 2κ_4 D ⟺ A_4² <= (2-√3)D² ⟺ X = 2D²-A_4² verifie X>0 et X² >= 3D⁴.
static_assert((i128)2 * kSpindleD * kSpindleD - (i128)kA4 * kA4 > 0,
              "kA4 : X positif");
static_assert(((i128)2 * kSpindleD * kSpindleD - (i128)kA4 * kA4) *
                      ((i128)2 * kSpindleD * kSpindleD - (i128)kA4 * kA4) >
                  (i128)3 * kSpindleD * kSpindleD * kSpindleD * kSpindleD,
              "kA4 doit sous-approcher 2*sin(15 degres)*2^30");

// Sur-approximations entieres de (4κ_q²+1), echelle E = 2^20 (terme
// SOUSTRAIT de R_coup : majorer). q2 : exactement 2E ; q3 : 4/3 ;
// q4 : 3-√3 ≈ 1,26795.
inline constexpr i64 kCoupE = 1ll << 20;
inline constexpr i64 kC2 = 2 * kCoupE;
inline constexpr i64 kC3 = (4 * kCoupE + 2) / 3;  // ceil(4E/3)
inline constexpr i64 kC4 = 1329545;               // > (3-√3)·2^20 = 1329541,06...
static_assert(3 * kC3 >= 4 * kCoupE, "kC3 doit majorer 4/3");
// kC4 > (3-√3)E ⟺ (3E-kC4)² < 3E² (les deux membres positifs).
static_assert((i128)(3 * kCoupE - kC4) * (3 * kCoupE - kC4) <
                  (i128)3 * kCoupE * kCoupE,
              "kC4 doit majorer (3-sqrt(3))*2^20");

// Boule-cœur d'arite q, rayon quadruple = max des deux bornes v3 re-derivees
// (derive_v4 : identite du parallelogramme |p|²+|w|²=(|u'|²+|v'|²)/2 puis
// Cauchy-Schwarz sur 2κ_q|w|+|p|) :
//   4R_dec  = A_q·(d2u - r2u)/D - r2u                (deplacement + perte)
//   4R_coup = A_q·d2u/D - ceil_sqrt(2·C_q·S2/E)      (borne couplee, S2 =
//             somme des carres des diagonales — plus forte en cas equilibre)
// Divisions PLANCHER, distance minoree, rayons majores, clamp a zero.
// Redonne core_ball_q2 pour q = 2 sur la branche R_dec.
inline CoreBall core_ball(Lane q, const AxisBox& A, const AxisBox& B,
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
  const i64 ra2u = ceil_sqrt(w2a);
  const i64 rb2u = ceil_sqrt(w2b);
  const i64 r2u = ra2u + rb2u;
  const i64 aq = (q == Lane::kQ2) ? kA2 : (q == Lane::kQ3) ? kA3 : kA4;
  const i64 cq = (q == Lane::kQ2) ? kC2 : (q == Lane::kQ3) ? kC3 : kC4;
  i64 r4 = 0;
  const i64 gap = d2u - r2u;
  if (gap > 0)
    r4 = (i64)(((i128)aq * gap) / kSpindleD) - r2u;
  // Borne couplee : S2 = (2r_A)² + (2r_B)² majore ; 2·C_q·S2/E < 2^38.
  const i64 s2 = ra2u * ra2u + rb2u * rb2u;
  const i64 sub2 = (2 * cq * s2 + kCoupE - 1) / kCoupE;  // ceil
  const i64 coup = (i64)(((i128)aq * d2u) / kSpindleD) - ceil_sqrt(sub2);
  r4 = std::max(r4, coup);
  cb.radius4 = std::max<i64>(0, r4);
  return cb;
}

// Autorite 64 coins : W_q evalue aux coins DISTINCTS de Box(A)×Box(B) pour un
// temoin ponctuel z. ALL ⟹ z universel sur l'enveloppe continue (sens
// suffisant seulement). Une boite plate n'a pas huit coins : seuls les coins
// distincts sont enumeres (lecon v3 : terrain est quasi-surfacique).
inline bool corner64_universal(Lane q, const AxisBox& A, const AxisBox& B,
                               const P3& z) {
  P3 a{}, b{};
  for (int ia = 0; ia < 8; ++ia) {
    a.x = (ia & 1) ? A.hi[0] : A.lo[0];
    a.y = (ia & 2) ? A.hi[1] : A.lo[1];
    a.z = (ia & 4) ? A.hi[2] : A.lo[2];
    if (((ia & 1) && A.hi[0] == A.lo[0]) || ((ia & 2) && A.hi[1] == A.lo[1]) ||
        ((ia & 4) && A.hi[2] == A.lo[2]))
      continue;  // coin duplique d'une boite plate
    for (int ib = 0; ib < 8; ++ib) {
      b.x = (ib & 1) ? B.hi[0] : B.lo[0];
      b.y = (ib & 2) ? B.hi[1] : B.lo[1];
      b.z = (ib & 4) ? B.hi[2] : B.lo[2];
      if (((ib & 1) && B.hi[0] == B.lo[0]) || ((ib & 2) && B.hi[1] == B.lo[1]) ||
          ((ib & 4) && B.hi[2] == B.lo[2]))
        continue;
      if (!in_spindle(q, a, b, z)) return false;
    }
  }
  return true;
}

}  // namespace mhgp4
