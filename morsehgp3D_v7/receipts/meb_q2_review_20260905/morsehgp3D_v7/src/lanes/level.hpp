// MorseHGP3D v6 — niveaux exacts (rayon AU CARRE, jamais un rayon).
//
//   Rational128 : fraction i128 canonique (q2 : D²/4 ; q3 : D·E·X/(4G)) ;
//   ExactLevel  : representant commun a toutes les lanes — num en U192 et den
//                 en i128, NON reduits (q4 : |N'|², det²). `operator==` n'est
//                 qu'une egalite de REPRESENTATION ; la seule egalite
//                 semantique est `same_exact_level` (produits croises U320).
//                 Deux boules distinctes de meme rayon portent des couples
//                 differents : jamais un groupement par `==` ni par hash.
#pragma once

#include "../core/intmath.hpp"
#include "../core/wide.hpp"

namespace mhgp7 {

struct Rational128 {
  i128 num;
  i128 den;  // > 0
};

inline Rational128 rational_reduce(Rational128 l) {
  const u128 g = ugcd128(uabs128(l.num), uabs128(l.den));
  if (g > 1) {
    l.num /= (i128)g;
    l.den /= (i128)g;
  }
  return l;
}

// Ordre exact de deux fractions positives (den > 0) : produits croises < 2^171.
inline int compare_rational(const Rational128& x, const Rational128& y) {
  return cmp_u192(mul_128x128_192((u128)x.num, (u128)y.den), mul_128x128_192((u128)y.num, (u128)x.den));
}

struct ExactLevel {
  u64 num[3];  // U192, non reduit
  i128 den;    // > 0, non reduit
  bool operator==(const ExactLevel& o) const {
    return num[0] == o.num[0] && num[1] == o.num[1] && num[2] == o.num[2] && den == o.den;
  }
  bool operator!=(const ExactLevel& o) const { return !(*this == o); }
  // Ordre de REPRESENTATION (tri deterministe), pas l'ordre semantique.
  bool operator<(const ExactLevel& o) const {
    for (int i = 2; i >= 0; --i)
      if (num[i] != o.num[i]) return num[i] < o.num[i];
    return den < o.den;
  }
};

MHGP7_HD inline int compare_exact_level(const ExactLevel& x, const ExactLevel& y) {
  const U192 nx{{x.num[0], x.num[1], x.num[2]}};
  const U192 ny{{y.num[0], y.num[1], y.num[2]}};
  return cmp_u320(mul_192x128_320(nx, (u128)y.den), mul_192x128_320(ny, (u128)x.den));
}

MHGP7_HD inline bool same_exact_level(const ExactLevel& x, const ExactLevel& y) {
  return compare_exact_level(x, y) == 0;
}

inline ExactLevel promote_level(const Rational128& l) {
  ExactLevel r{{0, 0, 0}, l.den};
  const u128 n = (u128)l.num;
  r.num[0] = (u64)n;
  r.num[1] = (u64)(n >> 64);
  return r;
}

// Plus petite representation (ordre de representation) — le representant
// canonique d'un niveau au RLE : arite minimale d'abord (par l'appelant),
// puis cet ordre.
inline bool level_repr_less(const ExactLevel& x, const ExactLevel& y) { return x < y; }

}  // namespace mhgp7
