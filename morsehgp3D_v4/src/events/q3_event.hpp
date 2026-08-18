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
//
// ENREGISTREMENT UNIQUE (audit cible 5964214, § 1) : l'evenement n'est pas
// quatre vecteurs paralleles, c'est UN record Q3Event{support, owner, ball,
// level, depth, interior}. Les champs support/owner/ball/level sont formes
// AVANT le census (candidat) ; le census n'ajoute que depth et la liste
// triee des interieurs. CONTRAT DE CAPACITE (audit § 2) : le profil courant
// est K_max <= 10, soit s_max <= 11, h_3 <= 9, profondeur survivante <= 8 —
// les tampons d'interieurs sont dimensionnes a 8 et tout chemin qui accepte
// un s_max plus grand doit REFUSER (code 2), jamais tronquer.
#pragma once

#include <array>
#include <utility>

#include "../gpu/device_compat.hpp"
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

inline u64 ugcd64(u64 x, u64 y) {
  while (y) {
    const u64 t = x % y;
    x = y;
    y = t;
  }
  return x;
}

// Pgcd HYBRIDE : la cle est formee pour chaque CANDIDAT avant le census, le
// pgcd est donc sur le chemin chaud. Euclide u128 (appel __umodti3 couteux)
// SEULEMENT jusqu'a passer sous 64 bits — un ou deux modulos suffisent, la
// taille d'Euclide decroit geometriquement — puis Euclide 64 bits materiel.
// (Le tout-binaire de Stein a ete mesure PIRE : ~bits iterations 128 bits.)
inline u128 ugcd(u128 x, u128 y) {
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

}  // namespace detail_ev

// CONTRAT CAUSAL EN DEUX TEMPS (audit cible 5964214, § 1, concilie avec le
// cout — mesure : ~300 candidats meurent au census pour 1 publie) :
//   1. la FORME BRUTE (cinq coefficients, niveau non reduit) est une
//      fonction pure du support, formee AVANT le census dans le candidat ;
//   2. la CANONISATION pgcd/signe est une fonction pure de cette forme
//      pre-census — sa signature ne recoit RIEN du census — appliquee a la
//      publication, comme le tri des InteriorIds.
// Aucun champ de la cle ne peut donc provenir du census, et le pgcd n'est
// paye que par les survivants.
struct Q3BallForm {
  i128 a;  // = G > 0
  i128 b[3];
  i128 c;
};

inline Q3BallForm q3_ball_form(const Q3Form& f) {
  Q3BallForm r;
  r.a = f.g;
  const i64 ax[3] = {f.a.x, f.a.y, f.a.z};
  i128 wa = 0;
  for (int i = 0; i < 3; ++i) {
    r.b[i] = -(2 * f.g * ax[i] + f.w[i]);
    wa += f.w[i] * ax[i];
  }
  r.c = f.g * ((i128)ax[0] * ax[0] + (i128)ax[1] * ax[1] + (i128)ax[2] * ax[2]) + wa;
  return r;
}

// Canonisation pgcd/signe — fonction PURE de la forme pre-census.
inline Q3BallKey q3_ball_key_reduce(const Q3BallForm& raw) {
  Q3BallKey k{raw.a, {raw.b[0], raw.b[1], raw.b[2]}, raw.c};
  // Sortie anticipee : des que le pgcd courant vaut 1, la cle est primitive.
  u128 g = detail_ev::uabs(k.a);
  for (int i = 0; i < 3 && g != 1; ++i)
    g = detail_ev::ugcd(g, detail_ev::uabs(k.b[i]));
  if (g != 1) g = detail_ev::ugcd(g, detail_ev::uabs(k.c));
  if (g > 1) {
    k.a /= (i128)g;
    for (int i = 0; i < 3; ++i) k.b[i] /= (i128)g;
    k.c /= (i128)g;
  }
  return k;  // signe : A = G > 0 par construction
}

// Precondition : f issue d'un triangle strictement aigu d'arete maximale
// (G > 0).
inline Q3BallKey q3_ball_key(const Q3Form& f) {
  return q3_ball_key_reduce(q3_ball_form(f));
}

// SupportKey q3 : triplet TRIE de vrais PointId externes (u32 — un id
// au-dessus du bit 31 reste ordonne correctement, contrairement a un i32).
struct SupportKey3 {
  PointId p[3];
  bool operator==(const SupportKey3& o) const {
    return p[0] == o.p[0] && p[1] == o.p[1] && p[2] == o.p[2];
  }
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

// L'evenement q3 complet. `interior` porte exactement `depth` PointId tries
// croissants (profil K_max <= 10 : depth <= h_3 - 1 <= 8). L'ordre total et
// l'egalite portent sur TOUS les champs : deux modes de calcul apparies
// doivent produire le meme multiensemble de records, pas le meme compte.
struct Q3Event {
  SupportKey3 support;
  EdgeKey owner;
  Q3BallKey ball;
  Q3Level level;
  u8 depth = 0;
  std::array<PointId, 8> interior{};

  bool operator==(const Q3Event& o) const {
    return support == o.support && owner.lo == o.owner.lo &&
           owner.hi == o.owner.hi && ball == o.ball && level.num == o.level.num &&
           level.den == o.level.den && depth == o.depth && interior == o.interior;
  }
  bool operator<(const Q3Event& o) const {
    if (!(support == o.support)) return support < o.support;
    if (owner.lo != o.owner.lo) return owner.lo < o.owner.lo;
    if (owner.hi != o.owner.hi) return owner.hi < o.owner.hi;
    if (!(ball == o.ball)) return ball < o.ball;
    if (level.num != o.level.num) return level.num < o.level.num;
    if (level.den != o.level.den) return level.den < o.level.den;
    if (depth != o.depth) return depth < o.depth;
    return interior < o.interior;
  }
};

// Niveau brut (rayon au carre, fraction non reduite ; num > 0 et den > 0
// des la formation — l'invariant de positivite se teste sur le brut).
inline Q3Level q3_level_raw(const P3& a, const P3& b, const P3& x) {
  const i64 D = p3_norm2(p3_sub(b, a));
  const i64 E = p3_norm2(p3_sub(x, a));
  const i64 F = p3_dot(p3_sub(b, a), p3_sub(x, a));
  const i64 X = D + E - 2 * F;
  Q3Level l;
  l.num = (i128)D * E * X;
  l.den = 4 * ((i128)D * E - (i128)F * F);
  return l;
}

// Canonisation — fonction PURE du niveau brut pre-census.
inline Q3Level q3_level_reduce(Q3Level l) {
  const u128 g = detail_ev::ugcd(detail_ev::uabs(l.num), detail_ev::uabs(l.den));
  if (g > 1) {
    l.num /= (i128)g;
    l.den /= (i128)g;
  }
  return l;
}

// Niveau public exact (rayon au carre), fraction canonique den > 0.
inline Q3Level q3_exact_level(const P3& a, const P3& b, const P3& x) {
  return q3_level_reduce(q3_level_raw(a, b, x));
}

// ---- ORDRE EXACT DES NIVEAUX (contre-audit 489c617, § 2). La foret exige
// `num1·den2 ? num2·den1` ; sous u16, num < 2^101 (D·E·X < 27·2^96) et
// den = 4G < 2^70 (G < 9·2^64), donc chaque produit croise est < 2^171 :
// un entier NON SIGNE de 192 bits suffit avec marge nette, i128 ne suffit
// pas. L'egalite de niveaux se lit directement sur les couples reduits.
struct U192 {
  u64 w[3];  // lo, mid, hi
};

// Produit 128×128 -> 192 bits. PRECONDITION (prouvee ci-dessus, testee
// contre l'oracle 384 bits) : le produit tient dans 192 bits.
// `mutant_trunc_hi` : tronque le mot haut — la porte qui le tue prouve que
// les comparaisons traversent reellement les bits >= 128.
MHGP4_HD inline U192 mul_level_192(u128 x, u128 y, bool mutant_trunc_hi = false) {
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
  if (mutant_trunc_hi) r.w[2] = 0;  // MUTANT
  return r;
}

MHGP4_HD inline int cmp_u192(const U192& a, const U192& b) {
  for (int i = 2; i >= 0; --i)
    if (a.w[i] != b.w[i]) return a.w[i] < b.w[i] ? -1 : 1;
  return 0;
}

// -1 / 0 / +1 selon x <=> y (niveaux positifs, den > 0). Les evenements de
// meme niveau (0) devront etre traites dans UN MEME MACRO-LOT par la foret :
// racines gelees avant le lot, toutes les multifusions du plateau ensemble,
// aucune chronologie binaire artificielle (contrat grave, foret a venir).
inline int compare_level(const Q3Level& x, const Q3Level& y,
                         bool mutant_trunc_hi = false) {
  return cmp_u192(mul_level_192((u128)x.num, (u128)y.den, mutant_trunc_hi),
                  mul_level_192((u128)y.num, (u128)x.den, mutant_trunc_hi));
}

}  // namespace mhgp4
