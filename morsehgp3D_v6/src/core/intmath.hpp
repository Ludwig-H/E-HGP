// MorseHGP3D v6 — arithmetique entiere dirigee : racines plancher/plafond,
// valeur absolue et pgcd 128 bits. Toute direction d'arrondi est nommee par
// la fonction ; aucun `isqrt(x)+1` deguise en plafond.
#pragma once

#include <cmath>

#include "device.hpp"
#include "types.hpp"

namespace mhgp6 {

// floor(sqrt(x)) exact sur i64 >= 0 (x <= 0 donne 0).
inline i64 floor_sqrt(i64 x) {
  if (x <= 0) return 0;
  i64 r = (i64)std::sqrt((double)x);
  while (r > 0 && (i128)r * r > x) --r;
  while ((i128)(r + 1) * (r + 1) <= x) ++r;
  return r;
}

// VRAI plafond : floor_sqrt(x), plus un si le carre est strictement en dessous.
inline i64 ceil_sqrt(i64 x) {
  const i64 r = floor_sqrt(x);
  return ((i128)r * r < x) ? r + 1 : r;
}

MHGP6_HD inline u128 uabs128(i128 v) { return v < 0 ? (u128)(-(v + 1)) + 1 : (u128)v; }

inline u64 ugcd64(u64 x, u64 y) {
  while (y) {
    const u64 t = x % y;
    x = y;
    y = t;
  }
  return x;
}

// Pgcd hybride : Euclide u128 (division logicielle) seulement jusqu'a passer
// sous 64 bits, puis Euclide materiel. Le tout-binaire de Stein a ete mesure
// pire en v4 (~bits iterations 128 bits).
inline u128 ugcd128(u128 x, u128 y) {
  if (!x) return y;
  if (!y) return x;
  while ((y >> 64) != 0) {
    const u128 t = x % y;
    x = y;
    y = t;
    if (!y) return x;
  }
  const u64 y64 = (u64)y;
  return ugcd64(y64, (u64)(x % y64));
}

// Plancher rationnel exact de num/den (den != 0).
inline i128 floor_div128(i128 num, i128 den) {
  i128 q = num / den;
  if (num % den != 0 && ((num < 0) != (den < 0))) --q;
  return q;
}

}  // namespace mhgp6
