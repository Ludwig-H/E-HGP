// MorseHGP3D v6 — entiers non signes larges : U192 et U320.
//
// Les niveaux exacts sont des rayons AU CARRE en fraction : q2/q3 tiennent en
// i128 (num < 2^101, den < 2^70), q4 non (|N'|² < 2^146, det² < 2^114). L'ordre
// des niveaux se decide par produits croises : q3/q3 < 2^171 (U192),
// q4/q4 < 2^260 (U320). Preconditions PROUVEES sur le profil u16 et testees
// contre l'oracle 384 bits (`mhgp6_level_cmp`). Le mutant `level-trunc-hi`
// tronque le mot haut : la porte qui le tue prouve que les comparaisons
// traversent les bits hauts.
#pragma once

#include "device.hpp"
#include "mutants.hpp"
#include "types.hpp"

namespace mhgp6 {

struct U192 {
  u64 w[3];  // lo, mid, hi
};

struct U320 {
  u64 w[5];
};

// Produit 128×128 -> 192. Precondition : le produit tient dans 192 bits.
MHGP6_HD inline U192 mul_128x128_192(u128 x, u128 y) {
  const u64 x0 = (u64)x, x1 = (u64)(x >> 64);
  const u64 y0 = (u64)y, y1 = (u64)(y >> 64);
  const u128 p00 = (u128)x0 * y0;
  const u128 p01 = (u128)x0 * y1;
  const u128 p10 = (u128)x1 * y0;
  const u128 p11 = (u128)x1 * y1;
  U192 r;
  r.w[0] = (u64)p00;
  const u128 mid = (p00 >> 64) + (u64)p01 + (u64)p10;
  r.w[1] = (u64)mid;
  r.w[2] = (u64)((mid >> 64) + (p01 >> 64) + (p10 >> 64) + p11);
  if (MHGP6_MUTANT("level-trunc-hi")) r.w[2] = 0;
  return r;
}

// Produit (U192) × (u128) -> U320. Precondition : produit < 2^320.
MHGP6_HD inline U320 mul_192x128_320(const U192& n, u128 d) {
  const u64 d0 = (u64)d, d1 = (u64)(d >> 64);
  const u128 p00 = (u128)n.w[0] * d0;
  const u128 p01 = (u128)n.w[0] * d1;
  const u128 p10 = (u128)n.w[1] * d0;
  const u128 p11 = (u128)n.w[1] * d1;
  const u128 p20 = (u128)n.w[2] * d0;
  const u128 p21 = (u128)n.w[2] * d1;
  U320 r;
  r.w[0] = (u64)p00;
  u128 acc = (p00 >> 64) + (u64)p01 + (u64)p10;
  r.w[1] = (u64)acc;
  acc = (acc >> 64) + (p01 >> 64) + (p10 >> 64) + (u64)p11 + (u64)p20;
  r.w[2] = (u64)acc;
  acc = (acc >> 64) + (p11 >> 64) + (p20 >> 64) + (u64)p21;
  r.w[3] = (u64)acc;
  r.w[4] = (u64)((acc >> 64) + (p21 >> 64));
  if (MHGP6_MUTANT("level-trunc-hi")) r.w[4] = 0;
  return r;
}

MHGP6_HD inline int cmp_u192(const U192& a, const U192& b) {
  for (int i = 2; i >= 0; --i)
    if (a.w[i] != b.w[i]) return a.w[i] < b.w[i] ? -1 : 1;
  return 0;
}

MHGP6_HD inline int cmp_u320(const U320& a, const U320& b) {
  for (int i = 4; i >= 0; --i)
    if (a.w[i] != b.w[i]) return a.w[i] < b.w[i] ? -1 : 1;
  return 0;
}

// Somme de trois carres 128 bits en U192 (|N'|² : somme < 3·2^144 < 2^192).
MHGP6_HD inline U192 sum_of_three_squares_192(u128 a, u128 b, u128 c) {
  U192 r{{0, 0, 0}};
  const u128 vals[3] = {a, b, c};
  for (int i = 0; i < 3; ++i) {
    const U192 sq = mul_128x128_192(vals[i], vals[i]);
    u128 acc = (u128)r.w[0] + sq.w[0];
    r.w[0] = (u64)acc;
    acc = (u128)r.w[1] + sq.w[1] + (acc >> 64);
    r.w[1] = (u64)acc;
    r.w[2] += sq.w[2] + (u64)(acc >> 64);
  }
  return r;
}

}  // namespace mhgp6
