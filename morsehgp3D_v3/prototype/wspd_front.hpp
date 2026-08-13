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
