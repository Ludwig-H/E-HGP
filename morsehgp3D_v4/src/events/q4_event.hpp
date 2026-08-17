// MorseHGP3D v4 — EVENEMENT q4 TRANSACTIONNEL.
//
// Une boule est une boule : la forme developpee de P4 donne
//   A = det, B = -2(det·a + N'), C = det·|a|² + 2N'·a
// (A > 0 par canonisation d'orientation), largeurs A < 2^57, |B_i| < 2^74,
// |C| < 2^90 — le MEME gabarit a cinq coefficients que la Q3BallKey, reduit
// par le meme pgcd/signe. Contrat causal en deux temps identique a q3 :
// forme brute dans le candidat, canonisation pure a la publication.
//
// NIVEAU q4 (MATHEMATIQUES.md § 4.5, question Q12) : R² = |N'|²/det² a un
// numerateur < 3·2^144 < 2^146 — HORS i128. Representant v4 (option b de
// Q12) : num en trois mots U192 (via mul_level_192, precondition 2^146 <
// 2^192 respectee), den = det² < 2^114 en i128, NON reduits ; egalite et
// ordre par produits croises (U320, a venir avec la foret). L'identite de
// boule reste portee par la BallKey primitive, jamais par le niveau.
#pragma once

#include "q3_event.hpp"
#include "q4_instruction.hpp"

namespace mhgp4 {

// Forme brute de la boule q4 (candidat, avant census).
inline Q3BallForm q4_ball_form(const Q4Form& f) {
  Q3BallForm r;
  r.a = f.det;
  const i64 ax[3] = {f.a.x, f.a.y, f.a.z};
  i128 na = 0;
  for (int i = 0; i < 3; ++i) {
    r.b[i] = -2 * (f.det * ax[i] + f.np[i]);
    na += f.np[i] * ax[i];
  }
  r.c = f.det * ((i128)ax[0] * ax[0] + (i128)ax[1] * ax[1] + (i128)ax[2] * ax[2]) +
        2 * na;
  return r;
}

struct Q4Level {
  u64 num[3];  // |N'|² en U192 (lo, mid, hi), non reduit
  i128 den;    // det² < 2^114, non reduit
  bool operator==(const Q4Level& o) const {
    return num[0] == o.num[0] && num[1] == o.num[1] && num[2] == o.num[2] &&
           den == o.den;
  }
  bool operator<(const Q4Level& o) const {
    for (int i = 2; i >= 0; --i)
      if (num[i] != o.num[i]) return num[i] < o.num[i];
    return den < o.den;
  }
};

// |N'|² par trois carres 128×128 -> 192 additionnes (aucun debordement :
// somme < 3·2^144 < 2^192).
inline Q4Level q4_level_raw(const Q4Form& f) {
  Q4Level l{{0, 0, 0}, f.det * f.det};
  for (int i = 0; i < 3; ++i) {
    const u128 m = detail_ev::uabs(f.np[i]);
    const U192 sq = mul_level_192(m, m);
    u128 acc = (u128)l.num[0] + sq.w[0];
    l.num[0] = (u64)acc;
    acc = (u128)l.num[1] + sq.w[1] + (acc >> 64);
    l.num[1] = (u64)acc;
    l.num[2] += sq.w[2] + (u64)(acc >> 64);
  }
  return l;
}

// L'evenement q4 complet : meme discipline que Q3Event (profil K_max <= 10,
// h_4 <= 8, profondeur survivante <= 7 — interior dimensionne a 8).
struct Q4Event {
  SupportKey4 support;
  EdgeKey owner;
  Q3BallKey ball;
  Q4Level level;
  u8 depth = 0;
  std::array<PointId, 8> interior{};

  bool operator==(const Q4Event& o) const {
    return support == o.support && owner.lo == o.owner.lo &&
           owner.hi == o.owner.hi && ball == o.ball && level == o.level &&
           depth == o.depth && interior == o.interior;
  }
  bool operator<(const Q4Event& o) const {
    if (!(support == o.support)) return support < o.support;
    if (owner.lo != o.owner.lo) return owner.lo < o.owner.lo;
    if (owner.hi != o.owner.hi) return owner.hi < o.owner.hi;
    if (!(ball == o.ball)) return ball < o.ball;
    if (!(level == o.level)) return level < o.level;
    if (depth != o.depth) return depth < o.depth;
    return interior < o.interior;
  }
};

}  // namespace mhgp4
