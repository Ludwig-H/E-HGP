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

#include "../gpu/device_compat.hpp"
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

// ATTENTION (contrat grave par l'audit « lemme prefixe et niveau ») :
// operator== et operator< sont des comparaisons de REPRESENTATION (champs
// bruts) — legitimes pour l'identite de record et le multiensemble du juge,
// JAMAIS pour les macro-lots de la foret. Deux boules distinctes de meme
// rayon portent des couples non reduits differents : la seule egalite
// semantique autorisee est `same_exact_level` (produit croise U320).
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

inline bool same_level_representation(const Q4Level& x, const Q4Level& y) {
  return x == y;
}

// |N'|² par trois carres 128×128 -> 192 additionnes (aucun debordement :
// somme < 3·2^144 < 2^192).
MHGP4_HD inline Q4Level q4_level_raw(const Q4Form& f) {
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

// ---- ORDRE SEMANTIQUE DES NIVEAUX (audit « lemme prefixe et niveau »,
// § 2) : N1/D1 <=> N2/D2 ssi N1·D2 <=> N2·D1 ; chaque produit q4/q4 est
// < 2^146 × 2^114 = 2^260 — CINQ mots (U320). Le meme comparateur recoit
// q3 apres promotion (num i128 < 2^101 -> trois mots, den < 2^70).
struct U320 {
  u64 w[5];
};

// Produit (U192 en trois mots) × (u128) -> U320. Precondition prouvee :
// produit < 2^260 < 2^320. `mutant_trunc_hi` tronque le mot w[4] — la
// porte qui le tue prouve que les comparaisons traversent les bits >= 256.
MHGP4_HD inline U320 mul_192_128_to_320(const u64 n[3], u128 d,
                               bool mutant_trunc_hi = false) {
  const u64 d0 = (u64)d, d1 = (u64)(d >> 64);
  const u128 p00 = (u128)n[0] * d0;
  const u128 p01 = (u128)n[0] * d1;
  const u128 p10 = (u128)n[1] * d0;
  const u128 p11 = (u128)n[1] * d1;
  const u128 p20 = (u128)n[2] * d0;
  const u128 p21 = (u128)n[2] * d1;
  U320 r;
  r.w[0] = (u64)p00;
  u128 acc = (p00 >> 64) + (u64)p01 + (u64)p10;
  r.w[1] = (u64)acc;
  acc = (acc >> 64) + (p01 >> 64) + (p10 >> 64) + (u64)p11 + (u64)p20;
  r.w[2] = (u64)acc;
  acc = (acc >> 64) + (p11 >> 64) + (p20 >> 64) + (u64)p21;
  r.w[3] = (u64)acc;
  r.w[4] = (u64)((acc >> 64) + (p21 >> 64));
  if (mutant_trunc_hi) r.w[4] = 0;  // MUTANT
  return r;
}

MHGP4_HD inline int cmp_u320(const U320& a, const U320& b) {
  for (int i = 4; i >= 0; --i)
    if (a.w[i] != b.w[i]) return a.w[i] < b.w[i] ? -1 : 1;
  return 0;
}

// -1 / 0 / +1 : l'ORDRE EXACT des niveaux (rayons carres), et la SEULE
// egalite semantique autorisee pour les macro-lots de la foret.
MHGP4_HD inline int compare_exact_level(const Q4Level& x, const Q4Level& y,
                               bool mutant_trunc_hi = false) {
  return cmp_u320(mul_192_128_to_320(x.num, (u128)y.den, mutant_trunc_hi),
                  mul_192_128_to_320(y.num, (u128)x.den, mutant_trunc_hi));
}

MHGP4_HD inline bool same_exact_level(const Q4Level& x, const Q4Level& y) {
  return compare_exact_level(x, y) == 0;
}

// Promotion d'un niveau q3 (fraction i128 canonique) vers le representant
// commun : le comparateur U320 ordonne alors q3 et q4 ensemble.
MHGP4_HD inline Q4Level promote_q3_level(const Q3Level& l) {
  Q4Level r{{0, 0, 0}, l.den};
  const u128 n = (u128)l.num;  // num > 0 (niveau public)
  r.num[0] = (u64)n;
  r.num[1] = (u64)(n >> 64);
  return r;
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
