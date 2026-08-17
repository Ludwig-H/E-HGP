// MorseHGP3D v4 — EVENEMENT q3 TRANSACTIONNEL : BallForm, BallKey, niveau.
//
// Formes exactes recues par l'audit du 17 aout (reponse census, § 5.3) :
//   ExactCenter = (2G·a + W, 2G)                    (centre rationnel)
//   ExactLevel  = D·E·X / (4G),  X = |b-x|² = D+E-2F  (RAYON AU CARRE — le
//                 niveau public est un carre, jamais un rayon)
//   BallForm    = (A = G, B = -(2G·a + W), C = G·|a|² + W·a)
// reduite par pgcd et signe A > 0 pour former la BallKey (cinq coefficients
// primitifs, formee AVANT tout census — jamais un champ issu du census).
//
// Largeurs sous u16 : G < 2^68 ; |W_i| < 2^85 ; |B_i| <= 2G·2^16 + |W_i|
// < 2^86 ; |C| <= G·3·2^32 + 3·2^85·2^16 < 2^104 ; D·E·X < 2^104 — i128.
//
// INVARIANT (regime regulier, sites distincts, coquilles refusees) : deux
// supports q3 distincts ne partagent jamais une BallKey — un partage
// exigerait un quatrieme point cospherique, donc un extra-shell deja refuse.
// La porte `ballkeys_uniques == evenements` le grave.
#pragma once

#include "q3_instruction.hpp"

namespace mhgp4 {

struct Q3BallKey {
  i128 a;      // > 0 apres normalisation
  i128 b[3];
  i128 c;
  bool operator==(const Q3BallKey& o) const {
    return a == o.a && b[0] == o.b[0] && b[1] == o.b[1] && b[2] == o.b[2] &&
           c == o.c;
  }
  bool operator<(const Q3BallKey& o) const {
    if (a != o.a) return a < o.a;
    for (int i = 0; i < 3; ++i)
      if (b[i] != o.b[i]) return b[i] < o.b[i];
    return c < o.c;
  }
};

struct Q3Level {
  i128 num;  // D·E·X / pgcd
  i128 den;  // 4G / pgcd, > 0
};

namespace detail_ev {

inline u128 uabs(i128 v) { return v < 0 ? (u128)(-(v + 1)) + 1 : (u128)v; }

inline u128 ugcd(u128 x, u128 y) {
  while (y) {
    const u128 t = x % y;
    x = y;
    y = t;
  }
  return x;
}

}  // namespace detail_ev

// BallForm reduite pgcd/signe. Precondition : f issue d'un triangle
// strictement aigu d'arete maximale (G > 0).
inline Q3BallKey q3_ball_key(const Q3Form& f) {
  Q3BallKey k;
  k.a = f.g;
  const i64 ax[3] = {f.a.x, f.a.y, f.a.z};
  i128 wa = 0;
  for (int i = 0; i < 3; ++i) {
    k.b[i] = -(2 * f.g * ax[i] + f.w[i]);
    wa += f.w[i] * ax[i];
  }
  k.c = f.g * ((i128)ax[0] * ax[0] + (i128)ax[1] * ax[1] + (i128)ax[2] * ax[2]) + wa;
  u128 g = detail_ev::uabs(k.a);
  for (int i = 0; i < 3; ++i) g = detail_ev::ugcd(g, detail_ev::uabs(k.b[i]));
  g = detail_ev::ugcd(g, detail_ev::uabs(k.c));
  if (g > 1) {
    k.a /= (i128)g;
    for (int i = 0; i < 3; ++i) k.b[i] /= (i128)g;
    k.c /= (i128)g;
  }
  return k;  // signe : A = G > 0 par construction
}

// Niveau public exact (rayon au carre), fraction canonique den > 0.
inline Q3Level q3_exact_level(const P3& a, const P3& b, const P3& x) {
  const i64 D = p3_norm2(p3_sub(b, a));
  const i64 E = p3_norm2(p3_sub(x, a));
  const i64 F = p3_dot(p3_sub(b, a), p3_sub(x, a));
  const i64 X = D + E - 2 * F;
  Q3Level l;
  l.num = (i128)D * E * X;
  l.den = 4 * ((i128)D * E - (i128)F * F);
  const u128 g = detail_ev::ugcd(detail_ev::uabs(l.num), detail_ev::uabs(l.den));
  if (g > 1) {
    l.num /= (i128)g;
    l.den /= (i128)g;
  }
  return l;
}

}  // namespace mhgp4
