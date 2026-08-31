// MorseHGP3D v6 — TEST DE SEED q4 PAR MORCEAUX DE CORDE.
//
// Lemme (analyse du 27 aout 2026, § 4) : pour un seed (a,b,x), les centres des
// boules q4 admissibles (tetraedres abxy bien centres dont ab est l'arete la
// plus longue, rayon <= D·sqrt(3/8) par Jung) forment une CORDE du plan
// bissecteur : c_μ − m = v3 + μ·n/(2G), n = (b−a)×(x−a), |μ| <= μ* = sqrt(J/2),
// J = D²(3G − 2 l_ax l_bx) > 0. Un site z est strictement interieur a la boule
// de centre c_μ ssi P(z) − μ·B(z) < 0, avec P = L/4 (L = kernel affine du seed,
// L = G q − 2 u·N) et B = n·(z − a) — AFFINE en μ. Le cœur de seed de Jung de
// production (P < 0 et 2P² > J B²) dit « z interieur pour tout μ de la corde »
// (temoin universel de la corde, K = 1).
// Ici la corde est coupee en K = 4 morceaux a bornes ENTIERES sur-approximantes
// ±μ̂, μ̂ = isqrt(J/2) + 1 >= μ* : sommets μ_j = (2j − 4)·μ̂/4, j = 0..4, soit,
// apres mise a l'echelle par 4, v_j(z) = 4P − (2j − 4)·μ̂·B = L − (2j − 4)·μ̂·B.
// z est temoin du morceau i ssi v_i < 0 et v_{i+1} < 0 (minimum d'une forme
// affine aux extremites). THEOREME (suffisance) : si chaque morceau compte
// >= h4 temoins, toute boule admissible du seed contient >= h4 sites
// strictement interieurs, donc toute completion y est tuee par le filtre de
// profondeur : le seed est MORT et l'objet est inchange. Les morceaux etant
// plus longs que la corde exacte (μ̂ > μ*), le test n'est pas plus fort que le
// cœur K = 1 aux extremites : les deux sont CUMULES (mort ssi fcount >= h4 ou
// min_i cnt_i >= h4).
// Arithmetique : filtre flottant certifie (L ∈ [lh − E, lh + E] comme le cœur ;
// produit (2j−4)·μ̂·B en double avec garde relative kChordGuard) puis repli
// EXACT en __int128 (|L| < 2^105, μ̂ < 2^54, |B| < 2^54 : |v_j| < 2^110).
// Mutant `chord-nonstrict` : v_j <= 0 compte comme temoin.
#pragma once

#include <cmath>

#include "../core/device.hpp"
#include "../core/mutants.hpp"
#include "../core/types.hpp"

namespace mhgp6 {

inline constexpr double kChordGuard = 0x1p-40;
inline constexpr int kChordPieces = 4;

// Racine carree entiere par defaut (floor), v >= 0, v < 2^120.
MHGP6_HD inline i128 isqrt128_floor(i128 v) {
  if (v <= 0) return 0;
  i128 r = (i128)std::sqrt((double)v);
  while (r > 0 && r * r > v) --r;
  while ((r + 1) * (r + 1) <= v) ++r;
  return r;
}

struct ChordPieces {
  i128 mu_hat = 0;      // borne entiere >= μ*
  double mu_hat_d = 0;  // (double)mu_hat
  u32 cnt[kChordPieces] = {0, 0, 0, 0};
  bool nonstrict = false;

  MHGP6_HD void init(i128 J, bool nonstrict_mutant) {
    mu_hat = isqrt128_floor(J / 2) + 1;
    mu_hat_d = (double)mu_hat;
    for (int i = 0; i < kChordPieces; ++i) cnt[i] = 0;
    nonstrict = nonstrict_mutant;
  }
  // Signe certifie de v_j = L − c·μ̂·B : rend -1 (< 0 certifie), +1 (>= 0 certifie), 0 (indecidable en flottant).
  MHGP6_HD int certified_sign(double lh, double E, int c, i64 B) const {
    const double t = (double)c * mu_hat_d * (double)B;
    const double at = t < 0 ? -t : t;
    const double tmin = t - at * kChordGuard, tmax = t + at * kChordGuard;
    if (lh + E < tmin) return -1;
    if (lh - E >= tmax) return 1;
    return 0;
  }
  // Mise a jour des comptes pour un site (L : valeur exacte paresseuse).
  template <class ExactL>
  MHGP6_HD void update(double lh, double E, i64 B, ExactL&& exact_l) {
    bool neg[kChordPieces + 1];
    bool have_exact = false;
    i128 Lx = 0;
    for (int j = 0; j <= kChordPieces; ++j) {
      const int c = 2 * j - kChordPieces;
      const int s = certified_sign(lh, E, c, B);
      if (s != 0) {
        neg[j] = s < 0;
      } else {
        if (!have_exact) { Lx = exact_l(); have_exact = true; }
        const i128 v = Lx - (i128)c * mu_hat * (i128)B;
        neg[j] = nonstrict ? (v <= 0) : (v < 0);
      }
    }
    for (int i = 0; i < kChordPieces; ++i)
      if (neg[i] && neg[i + 1]) ++cnt[i];
  }
  MHGP6_HD bool dead(u64 h) const {
    u32 mn = cnt[0];
    for (int i = 1; i < kChordPieces; ++i) mn = cnt[i] < mn ? cnt[i] : mn;
    return (u64)mn >= h;
  }
};

}  // namespace mhgp6
