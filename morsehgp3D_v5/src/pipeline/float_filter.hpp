// MorseHGP3D v5 — etage flottant CERTIFIE du signe de la puissance q3, forme
// affine par ancre, et etage d'intervalles de Jung.
//
// Le flottant n'existe ici que comme FILTRE a repli exact : une decision
// certifiee est prouvee (borne d'erreur), sinon le chemin exact tranche.
//
// Forme affine (identite gravee par la porte `mhgp5_q3_affine`) : pour une
// ancre (a,b) et un site z, u = 2z−a−b et q = |u|²−D² sont des entiers
// < 2^36, EXACTS en binaire64 ; pour un seed x, N = W − G·d (|N| < 2^87),
// G < 2^68 ; alors L(z) = G·q − 2·u·N = 4·P(z) (P = puissance de Gram) et
// P = L/4 exactement (divisibilite gravee). Sequence FIGEE :
//   t  = fma(N2,u2, fma(N1,u1, N0·u0)) ;   L^ = fma(G, q, −(t+t)).
// Erreur : seules les conversions de G et N (ulp) et cinq arrondis
// contribuent, toutes RELATIVES : |L^ − L| <= ~8u·(G|q| + 2Σ|N_i u_i|),
// u = 2^-53 ; majoree UNE fois par seed par E = 2^-48·(G·qmax + 2|N|_1·umax)
// (marge ×4, arrondis de E absorbes). Decision : L^ < −E ⟹ L < 0 certifie ;
// L^ > +E ⟹ L > 0 certifie ; sinon repli affine exact i128 (|L| < 2^105).
//
// Intervalles de Jung : pour un site certifie P < 0 (P ∈ [(L^−E)/4, (L^+E)/4])
// et J >= 0, si [2P²] et [J][B²] se separent, temoin certifie (2P² > JB²
// strict) ou non-temoin certifie, sans exact ; sinon repli `cmp_2p2_jb2`.
// Erreurs relatives < 8u par cote ; garde 2^-40 = 2^13 u. Les egalites
// tombent TOUJOURS dans le repli. Mutant `jung-swap-bounds` : le kill teste
// le mauvais bout de l'intervalle.
//
// Conditions d'execution : binaire64, arrondi au plus proche, aucune
// reassociation. Sous __FAST_MATH__ le filtre est coupe a la compilation ;
// un mode d'arrondi != FE_TONEAREST le coupe a l'execution (borne = +inf ⟹
// repli exact integral ; la correction ne depend jamais du filtre).
#pragma once

#include <cfenv>
#include <cmath>
#include <limits>

#include "../core/mutants.hpp"
#include "../core/wide.hpp"

namespace mhgp5 {

inline double affine_l_hat(double gd, double nd0, double nd1, double nd2, double u0, double u1, double u2,
                           double q) {
  const double t = std::fma(nd2, u2, std::fma(nd1, u1, nd0 * u0));
  return std::fma(gd, q, -(t + t));
}

inline double affine_l_bound(double gd, double nd0, double nd1, double nd2, double qmax, double umax) {
  return 0x1p-48 * std::fma(gd, qmax, 2.0 * (std::fabs(nd0) + std::fabs(nd1) + std::fabs(nd2)) * umax);
}

inline constexpr double kJungGuard = 0x1p-40;

// +1 temoin certifie (2P² > JB²), -1 non-temoin certifie, 0 repli exact.
// Preconditions : lh < -e (P < 0 certifie), jlo <= J <= jhi, J >= 0.
inline int jung_interval_sign(double lh, double e, double jlo, double jhi, i64 b) {
  const double bd = (double)b;
  const double b2 = bd * bd;
  const double pu = (lh + e) * 0.25;  // borne sup de P (la plus pres de 0)
  const double pl = (lh - e) * 0.25;  // borne inf de P
  const double pk = MHGP5_MUTANT("jung-swap-bounds") ? pl : pu;
  const double lhs_min = 2.0 * (pk * pk) * (1.0 - kJungGuard);
  const double rhs_max = jhi * (b2 * (1.0 + kJungGuard));
  if (lhs_min > rhs_max) return 1;
  const double lhs_max = 2.0 * (pl * pl) * (1.0 + kJungGuard);
  const double rhs_min = jlo * (b2 * (1.0 - kJungGuard));
  if (lhs_max < rhs_min) return -1;
  return 0;
}

#if defined(__FAST_MATH__)
inline constexpr bool kFloatFilterCompileEnabled = false;
#else
inline constexpr bool kFloatFilterCompileEnabled = true;
#endif

inline bool float_filter_runtime_enabled() {
  return kFloatFilterCompileEnabled && std::fegetround() == FE_TONEAREST;
}

// Comparaison EXACTE 2·P² <=> J·B² (cœur de seed de Jung). Preconditions :
// P <= 0, J >= 0 ; produits < 2^210 (U320).
MHGP5_HD inline int cmp_2p2_jb2(i128 P, i128 J, i64 B) {
  const u128 ap = (u128)(-P);
  const U192 pw{{(u64)ap, (u64)(ap >> 64), 0}};
  U320 lhs = mul_192x128_320(pw, ap);
  u64 carry = 0;
  for (int i = 0; i < 5; ++i) {
    const u64 nc = lhs.w[i] >> 63;
    lhs.w[i] = (lhs.w[i] << 1) | carry;
    carry = nc;
  }
  const u128 ju = (u128)J;
  const U192 jw{{(u64)ju, (u64)(ju >> 64), 0}};
  const u128 b2 = (u128)((i128)B * B);
  return cmp_u320(lhs, mul_192x128_320(jw, b2));
}

}  // namespace mhgp5
