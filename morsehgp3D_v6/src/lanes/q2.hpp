// MorseHGP3D v6 — lane q2 : la boule diametrale.
//
// W_2(a,b) EST la boule diametrale ouverte : |2z-(a+b)|² < D² interieur strict,
// = D² coquille, a et b sur la sphere (support, exclus). Forme primitive
// directe : |2z-(a+b)|² - D² = 4(|z|² - (a+b)·z + a·b), donc
// (A, B, C) = (1, -(a+b), a·b) — aucun pgcd a payer. Niveau : D²/4.
#pragma once

#include <array>

#include "keys.hpp"
#include "level.hpp"

namespace mhgp6 {

MHGP6_HD inline BallKey q2_ball_key(const P3& a, const P3& b) {
  BallKey k;
  k.a = 1;
  k.b[0] = -(i128)(a.x + b.x);
  k.b[1] = -(i128)(a.y + b.y);
  k.b[2] = -(i128)(a.z + b.z);
  k.c = (i128)a.x * b.x + (i128)a.y * b.y + (i128)a.z * b.z;
  return k;
}

inline Rational128 q2_exact_level(i64 D2) { return rational_reduce(Rational128{(i128)D2, 4}); }

struct Q2Event {
  EdgeKey support;
  BallKey ball;
  Rational128 level;
  u8 depth = 0;
  std::array<PointId, 9> interior{};
};

}  // namespace mhgp6
