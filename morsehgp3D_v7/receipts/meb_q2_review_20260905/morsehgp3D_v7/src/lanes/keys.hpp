// MorseHGP3D v6 — cles combinatoires et geometriques partagees par les lanes.
//
//   EdgeKey      : arete (min PointId, max PointId) — l'ANCRE canonique ;
//   SupportKey3/4 : support trie ;
//   BallKey      : UNE boule est UNE boule — forme (A, B[3], C) de
//                  P(z) = A|z|² + B·z + C, primitive (pgcd) et A > 0, quelle
//                  que soit la lane generatrice (q2 : A = 1 ; q3 : A = G ;
//                  q4 : A = det). P < 0 interieur strict, P = 0 coquille.
//                  Largeurs sous u16 : A < 2^68, |B_i| < 2^86, |C| < 2^104 ;
//                  A|z|² < 2^102, |B·z| < 2^105 — i128 ;
//   FacetKey     : K-uplet trie de PointId (K <= 10), sommet du K-graphe.
//
// Contrat causal : la forme brute est une fonction pure du support, formee
// AVANT tout census ; la canonisation pgcd/signe est une fonction pure de la
// forme brute, appliquee a la publication. Aucun champ d'une cle ne provient
// du census.
#pragma once

#include <algorithm>
#include <array>
#include <utility>

#include "../core/device.hpp"
#include "../core/intmath.hpp"
#include "../core/types.hpp"

namespace mhgp7 {

struct EdgeKey {
  PointId lo = 0, hi = 0;
  MHGP7_HD bool operator==(const EdgeKey& o) const { return lo == o.lo && hi == o.hi; }
  MHGP7_HD bool operator<(const EdgeKey& o) const { return lo != o.lo ? lo < o.lo : hi < o.hi; }
};
// Appele par anchor_owns_q3 (MHGP7_HD) : host + device.
MHGP7_HD inline EdgeKey edge_key(PointId x, PointId y) { return x < y ? EdgeKey{x, y} : EdgeKey{y, x}; }

struct SupportKey3 {
  PointId p[3];
  bool operator==(const SupportKey3& o) const { return p[0] == o.p[0] && p[1] == o.p[1] && p[2] == o.p[2]; }
  bool operator<(const SupportKey3& o) const {
    for (int i = 0; i < 3; ++i)
      if (p[i] != o.p[i]) return p[i] < o.p[i];
    return false;
  }
};
inline SupportKey3 support_key3(PointId a, PointId b, PointId x) {
  SupportKey3 k{{a, b, x}};
  if (k.p[0] > k.p[1]) std::swap(k.p[0], k.p[1]);
  if (k.p[1] > k.p[2]) std::swap(k.p[1], k.p[2]);
  if (k.p[0] > k.p[1]) std::swap(k.p[0], k.p[1]);
  return k;
}

struct SupportKey4 {
  PointId p[4];
  bool operator==(const SupportKey4& o) const {
    return p[0] == o.p[0] && p[1] == o.p[1] && p[2] == o.p[2] && p[3] == o.p[3];
  }
  bool operator<(const SupportKey4& o) const {
    for (int i = 0; i < 4; ++i)
      if (p[i] != o.p[i]) return p[i] < o.p[i];
    return false;
  }
};
inline SupportKey4 support_key4(PointId a, PointId b, PointId x, PointId y) {
  SupportKey4 k{{a, b, x, y}};
  for (int i = 1; i < 4; ++i)
    for (int j = i; j > 0 && k.p[j - 1] > k.p[j]; --j) std::swap(k.p[j - 1], k.p[j]);
  return k;
}

// Forme brute d'une boule (avant canonisation).
struct BallForm {
  i128 a;
  i128 b[3];
  i128 c;
};

struct BallKey {
  i128 a;  // > 0
  i128 b[3];
  i128 c;
  bool operator==(const BallKey& o) const {
    return a == o.a && b[0] == o.b[0] && b[1] == o.b[1] && b[2] == o.b[2] && c == o.c;
  }
  bool operator!=(const BallKey& o) const { return !(*this == o); }
  bool operator<(const BallKey& o) const {
    if (a != o.a) return a < o.a;
    for (int i = 0; i < 3; ++i)
      if (b[i] != o.b[i]) return b[i] < o.b[i];
    return c < o.c;
  }
  // Puissance exacte du point z : < 0 interieur strict, = 0 coquille. i128.
  MHGP7_HD i128 power(const P3& z) const {
    return a * p3_norm2(z) + b[0] * z.x + b[1] * z.y + b[2] * z.z + c;
  }
};

// Canonisation pgcd/signe — fonction PURE de la forme brute. Precondition
// a > 0. Sortie anticipee des que le pgcd courant vaut 1.
inline BallKey ball_key_reduce(const BallForm& raw) {
  BallKey k{raw.a, {raw.b[0], raw.b[1], raw.b[2]}, raw.c};
  u128 g = uabs128(k.a);
  for (int i = 0; i < 3 && g != 1; ++i) g = ugcd128(g, uabs128(k.b[i]));
  if (g != 1) g = ugcd128(g, uabs128(k.c));
  if (g > 1) {
    k.a /= (i128)g;
    for (int i = 0; i < 3; ++i) k.b[i] /= (i128)g;
    k.c /= (i128)g;
  }
  return k;
}

inline constexpr int kFacetMaxK = 10;

struct FacetKey {
  u8 k = 0;
  std::array<PointId, kFacetMaxK> p{};
  bool operator<(const FacetKey& o) const { return k != o.k ? k < o.k : p < o.p; }
  bool operator==(const FacetKey& o) const { return k == o.k && p == o.p; }
  bool operator!=(const FacetKey& o) const { return !(*this == o); }
};

}  // namespace mhgp7
