// MorseHGP3D v3 — PARTITION WSPD ENTIERE, CONSTRUITE AVANT TOUTE GEOMETRIE.
//
// Objet (audit `AUDIT_DEBLOCAGE_WSPD_PREFIX_CARRIERS`, section 4) : construire
// la partition en rectangles bien separes SANS appeler `H`, puis classifier
// exactement une fois chaque `RectId` TERMINAL. Le probe precedent appelait le
// classifieur sur les rectangles internes PUIS sur les terminaux, payant ainsi
// presque deux fois la traversee et melant construction du front et couverture
// geometrique.
//
// SEPARATION ENTIERE EN NORME INFINIE. Le predicat flottant `d - rA - rB >=
// s max(rA,rB)` alterait les `RectId` et le determinisme CPU/device. Sa forme
// entiere, toutes quantites DOUBLEES pour eviter les demis :
//
//     c2_i = lo_i + hi_i          (double du centre)
//     r2   = max_i (hi_i - lo_i)  (double du rayon L-infini)
//     d2   = max_i |c2A_i - c2B_i|
//
// et l'arret exact a `s = 2` est
//
//     d2 >= r2A + r2B    et    d2 - r2A - r2B >= 2 max(r2A, r2B).
//
// Tout tient en `i64` sous u16. La WSPD ne decide AUCUNE geometrie
// scientifique : un rectangle separe mais non ferme est simplement transmis.
#pragma once

#include <algorithm>

namespace mhgp3v {

struct WspdBox {
  long long lo[3], hi[3];
};

// ---- SEPARATION EUCLIDIENNE, EN CARRES ENTIERS.
//
// La variante L-infini ci-dessous est exacte mais ne parle pas la meme langue
// que les certificats, qui sont euclidiens : la boule circonscrite a un cube de
// demi-cote `r` a un rayon `r sqrt(3)`, donc la separation euclidienne
// effective pouvait valoir jusqu'a `sqrt(3)` fois moins que le `s` annonce.
//
// La forme euclidienne s'ecrit sans aucune racine. Avec `w_i = hi_i - lo_i`, le
// DIAMETRE carre d'une boite vaut `W2 = sum_i w_i^2`, donc `2r = sqrt(W2)`. Avec
// les centres DOUBLES `c2_i = lo_i + hi_i`, la distance carree vaut
// `D2 = sum_i (c2A_i - c2B_i)^2 = (2d)^2`. La condition
//
//     d - r_A - r_B >= s max(r_A, r_B)
//
// est IMPLIQUEE par `d >= (s+2) max(r_A,r_B)`, puisque `r_A + r_B <= 2 max`.
// En doublant et en elevant au carre, avec `s = p/q` rationnel :
//
//     q^2 D2 >= (p + 2q)^2 max(W2_A, W2_B).
//
// Tout est entier. Sous u16, `D2 <= 3 x 131070^2 < 5,2e10` et le membre droit
// reste sous `2^47` pour `p/q <= 32` : `i64` suffit, aucun deux-limbes.
//
// Ce test peut PERDRE des separations quand les rayons different beaucoup — il
// majore `r_A + r_B` par `2 max` —, mais il ne peut jamais en INVENTER. Une
// separation manquee ne fait que transmettre un residu ; une separation
// inventee casserait la borne.
inline long long wspd_w2(const WspdBox& b) {
  long long s = 0;
  for (int i = 0; i < 3; ++i) { const long long w = b.hi[i] - b.lo[i]; s += w * w; }
  return s;
}

inline bool wspd_separated_euclid(const WspdBox& a, const WspdBox& b,
                                  long long p, long long q) {
  long long d2 = 0;
  for (int i = 0; i < 3; ++i) {
    const long long u = (a.lo[i] + a.hi[i]) - (b.lo[i] + b.hi[i]);
    d2 += u * u;
  }
  const long long r2 = std::max(wspd_w2(a), wspd_w2(b));
  const long long lhs = q * q;
  const long long k = p + 2 * q;
  // `q^2 D2 >= (p+2q)^2 R2`, en __int128 pour ne rien supposer sur les bornes.
  return (__int128)lhs * d2 >= (__int128)k * k * r2;
}

inline long long wspd_r2(const WspdBox& b) {
  long long r = 0;
  for (int i = 0; i < 3; ++i) r = std::max(r, b.hi[i] - b.lo[i]);
  return r;
}

// `s` est un ENTIER : l'arret exact est `d2 - r2A - r2B >= s * max(r2A,r2B)`.
inline bool wspd_separated(const WspdBox& a, const WspdBox& b, long long s) {
  const long long ra = wspd_r2(a), rb = wspd_r2(b);
  long long d2 = 0;
  for (int i = 0; i < 3; ++i) {
    const long long u = (a.lo[i] + a.hi[i]) - (b.lo[i] + b.hi[i]);
    d2 = std::max(d2, u < 0 ? -u : u);
  }
  if (d2 < ra + rb) return false;
  return d2 - ra - rb >= s * std::max(ra, rb);
}

}  // namespace mhgp3v
