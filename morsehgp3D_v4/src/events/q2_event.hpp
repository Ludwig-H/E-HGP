// MorseHGP3D v4 — EVENEMENT q2 TRANSACTIONNEL.
//
// Le fuseau W_2(a,b) EST la boule diametrale ouverte (H = d·w - |w|² > 0
// ⟺ |z-m| < D/2) : le census q2 est une requete |2z-(a+b)|² <= D² (cover
// d'arete a coefficient 1), interieur strict <, coquille ==. Les deux
// extremites a et b sont exactement sur la sphere (support, exclues).
//
// Une boule est une boule : |2z-(a+b)|² - D² = 4(|z|² - (a+b)·z + a·b),
// donc la forme reduite est directement PRIMITIVE :
//   (A, B, C) = (1, -(a+b), a·b)   — A = 1, aucun pgcd a payer.
// Niveau public (rayon au carre) : D²/4, fraction canonique par le meme
// reducteur que q3. Largeurs u16 : tout tient en i64/i128 confortablement.
//
// Profil K_max <= 10 : h_2 = s_max - 1 = 10, profondeur survivante <= 9 —
// l'evenement q2 porte jusqu'a NEUF interieurs (K = d + 1).
#pragma once

#include "q3_event.hpp"

namespace mhgp4 {

inline Q3BallKey q2_ball_key(const P3& a, const P3& b) {
  Q3BallKey k;
  k.a = 1;
  k.b[0] = -(i128)(a.x + b.x);
  k.b[1] = -(i128)(a.y + b.y);
  k.b[2] = -(i128)(a.z + b.z);
  k.c = (i128)a.x * b.x + (i128)a.y * b.y + (i128)a.z * b.z;
  return k;  // primitive par construction (A = 1)
}

inline Q3Level q2_exact_level(i64 D2) {
  return q3_level_reduce(Q3Level{(i128)D2, 4});
}

struct Q2Event {
  EdgeKey support;  // l'arete est son propre owner
  Q3BallKey ball;
  Q3Level level;
  u8 depth = 0;
  std::array<PointId, 9> interior{};

  bool operator==(const Q2Event& o) const {
    return support.lo == o.support.lo && support.hi == o.support.hi &&
           ball == o.ball && level.num == o.level.num &&
           level.den == o.level.den && depth == o.depth && interior == o.interior;
  }
  bool operator<(const Q2Event& o) const {
    if (support.lo != o.support.lo) return support.lo < o.support.lo;
    if (support.hi != o.support.hi) return support.hi < o.support.hi;
    if (!(ball == o.ball)) return ball < o.ball;
    if (level.num != o.level.num) return level.num < o.level.num;
    if (level.den != o.level.den) return level.den < o.level.den;
    if (depth != o.depth) return depth < o.depth;
    return interior < o.interior;
  }
};

}  // namespace mhgp4
