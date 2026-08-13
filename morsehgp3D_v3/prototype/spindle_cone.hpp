// MorseHGP3D v3 — LE CONE CIBLE EXACT D'UN TEMOIN UNIVERSEL.
//
// Specification : audits/AUDIT_REPONSES_MUR_AMAS_CENSUS_SPINDLE_20260812.md,
// sections 3 et 7 (reponses Q2 et Q6 du contre-audit).
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=proposition_math_non_recue,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// L'OBJET
//
// Pour une paire distincte (a,b), le temoin `z` est UNIVERSEL a la lane q
// lorsqu'il est interieur a TOUTE boule admissible d'arite q de cette ancre.
// Les domaines ouverts sont les spindles du contre-audit, ecrits ici dans la
// representation (H,R) :
//
//   H(a,b,z) = (b-z) . (z-a),      R(a,b,z) = |(b-a) x (z-a)|^2
//
//   q2 : H > 0                     (interieur de la boule diametrale)
//   q3 : H > 0 et 3 H^2 > R
//   q4 : H > 0 et 2 H^2 > R
//
// L'identite avec la representation historique (g,Q) du lemme du noeud spindle
// est exacte sur les entiers : avec u = z-a, w = b-z, d = b-a et U = 2z-a-b,
//   g = D2 - |U|^2 = 4 (u.w) = 4 H,        Q = D2 |U|^2 - (U.d)^2 = 4 R,
// donc {g>0, 3g^2>4Q} = {H>0, 3H^2>R} et {g>0, g^2>2Q} = {H>0, 2H^2>R}.
// Les deux representations sont implementees ici et la porte exige leur accord
// exact : une faute commune ne peut pas se compenser entre elles.
//
// ---------------------------------------------------------------------------
// LE RESULTAT NOUVEAU : A `a` ET `z` FIXES, LA CIBLE VIT DANS UN CONE CONVEXE
//
// Poser e = z-a et t = b-z. Alors H = t.e et, par Lagrange,
//   R = |t x e|^2 = |t|^2 |e|^2 - H^2 = E2 X2 - H^2.
// En decomposant t = alpha u + v avec u = e/|e| et v orthogonal a u, il vient
// H = alpha |e| et R = |e|^2 |v|^2, donc
//
//   C2(a,z) = { b : alpha > 0 }                  demi-espace ouvert
//   C3(a,z) = { b : alpha > 0, |v| < sqrt(3) alpha }   demi-angle 60 degres
//   C4(a,z) = { b : alpha > 0, |v| < sqrt(2) alpha }   demi-angle arctan(sqrt2)
//
// Ce sont trois cones de Lorentz OUVERTS et CONVEXES, d'apex `z` et d'axe
// `z-a`, embouites C4 < C3 < C2. Deux consequences portent tout le reste :
//
//   1. une AABB fermee de cibles est incluse dans C_q SI ET SEULEMENT SI ses
//      huit coins y sont strictement. C'est une equivalence, pas une
//      suffisance : la boite est l'enveloppe convexe de ses coins et C_q est
//      convexe ouvert. Un temoin est donc credite pour TOUTE la masse
//      partenaire d'un noeud en huit tests ponctuels ;
//   2. l'apex `z` n'appartient pas au cone ouvert (H=0), donc une boite qui
//      contient `z` echoue automatiquement : le temoin ne peut jamais se
//      compter lui-meme, ni compter `b=z`.
//
// La forme sans produit vectoriel est preferee sur les coins : avec
// E2 = |z-a|^2 et X2 = |b-z|^2,
//   q3 <=> H > 0 et 4 H^2 > E2 X2,        q4 <=> H > 0 et 3 H^2 > E2 X2.
//
// LE DEMI-ANGLE SE MESURE DEPUIS LE TEMOIN, PAS DEPUIS L'ANCRE. La lecture
// « tout voisin de `a` dans un cone de 54,74 degres autour de la direction
// `b-a` est certifie » est FAUSSE a distance finie : elle demande encore une
// borne radiale. Aucune valeur trigonometrique n'entre ici ; seuls les tests
// entiers `H>0`, `4H^2>E2 X2` et `3H^2>E2 X2` decident.
//
// ---------------------------------------------------------------------------
// BORNES ET LARGEUR
//
// Profil u16 : coordonnees dans [0,65535], donc |e_i|,|t_i| <= 65535.
//   |H|      <= 3 * 65535^2 = 1,29e10  < 2^34            (i64)
//   E2, X2   <= 1,29e10                                   (i64)
//   E2 * X2  <= 1,66e20                                   -> 68 bits, i128
//   4 H^2    <= 6,6e20                                    -> 70 bits, i128
//   |(t x e)_j| <= 2 * 65535^2 = 8,6e9, donc R <= 2,2e20  -> i128
// Les carres et produits sont donc PROMUS AVANT multiplication. Un mutant qui
// reste en i64 doit mourir.
#pragma once

#include <cstdint>

#include "mhgp/mhgp.hpp"

// MHGP_HD vient de mhgp/exact.hpp : host seul en C++, host+device sous nvcc.

namespace mhgp3v {
namespace cone {

using mhgp::i128;
using mhgp::i64;
using mhgp::P3;

// Valeurs de lane, ordonnees par force croissante. `kLaneNone` signifie que le
// point n'est meme pas dans le demi-espace q2.
enum : int { kLaneNone = 0, kLaneQ2 = 2, kLaneQ3 = 3, kLaneQ4 = 4 };

// Masque de refutation : bit q positionne = AUCUN point de la boite n'est dans
// C_q. Il est fail-open (il peut rester nul a tort, jamais se lever a tort).
enum : unsigned { kNoneQ2 = 1u << 2, kNoneQ3 = 1u << 3, kNoneQ4 = 1u << 4 };

// ---------------------------------------------------------------------------
// MUTANTS DU CLASSIFIEUR. Chacun casse UNE decision exacte et doit etre tue
// par la porte correspondante. Ils sont ici, et non dans le probe, parce que
// la faute doit vivre dans le predicat lui-meme.
// ---------------------------------------------------------------------------
enum class ConeMutant {
  kNone,
  kNoPositivity,    // oublie H > 0 : le cone oppose est accepte
  kAcceptEquality,  // >= au lieu de > : la frontiere fermee entre dans le cone
  kSwapCoeff,       // confond les coefficients 3 et 4 des lanes q3/q4
  kCenterOnly,      // ne teste que le centre de la boite au lieu des huit coins
  kSevenCorners,    // omet le huitieme coin
  kNarrowI64,       // ne promeut pas avant multiplication
  kNoneUsesRmax,    // refutation par un MAJORANT de R : peut refuter a tort
  kDuplicateSlot,   // credite deux fois le meme slot de banque
  kIgnoreInherited, // recredite un temoin deja credite chez un ancetre
};

inline const char* cone_mutant_name(ConeMutant m) {
  switch (m) {
    case ConeMutant::kNone: return "none";
    case ConeMutant::kNoPositivity: return "cone-no-positivity";
    case ConeMutant::kAcceptEquality: return "cone-accept-equality";
    case ConeMutant::kSwapCoeff: return "cone-swap-coeff";
    case ConeMutant::kCenterOnly: return "cone-center-only";
    case ConeMutant::kSevenCorners: return "cone-seven-corners";
    case ConeMutant::kNarrowI64: return "cone-narrow-i64";
    case ConeMutant::kNoneUsesRmax: return "cone-none-uses-rmax";
    case ConeMutant::kDuplicateSlot: return "cone-duplicate-slot";
    case ConeMutant::kIgnoreInherited: return "cone-ignore-inherited";
  }
  return "?";
}

// ---------------------------------------------------------------------------
// Le temoin precalcule : son apex `z`, son axe `e = z-a` et |e|^2.
// ---------------------------------------------------------------------------
struct Witness {
  i64 z[3] = {0, 0, 0};
  i64 e[3] = {0, 0, 0};
  i64 e2 = 0;
  int id = -1;  // PointId, pour l'unicite des credits
};

MHGP_HD inline Witness make_witness(i64 ax, i64 ay, i64 az, i64 zx, i64 zy, i64 zz, int id) {
  Witness w{};
  w.z[0] = zx; w.z[1] = zy; w.z[2] = zz;
  w.e[0] = zx - ax; w.e[1] = zy - ay; w.e[2] = zz - az;
  w.e2 = w.e[0] * w.e[0] + w.e[1] * w.e[1] + w.e[2] * w.e[2];
  w.id = id;
  return w;
}

inline Witness make_witness(const P3& a, const P3& z, int id) {
  return make_witness((i64)a.x, (i64)a.y, (i64)a.z, (i64)z.x, (i64)z.y, (i64)z.z, id);
}

// ---------------------------------------------------------------------------
// PREDICAT PONCTUEL, REPRESENTATION (H, E2 X2). C'est l'autorite du chemin
// produit : elle n'emploie aucun produit vectoriel.
//
// Rend la lane la PLUS FORTE satisfaite par la cible `b` : kLaneNone, kLaneQ2,
// kLaneQ3 ou kLaneQ4. L'emboitement C4 < C3 < C2 rend ce seul entier
// suffisant.
// ---------------------------------------------------------------------------
MHGP_HD inline int lane_of_target(const Witness& w, i64 bx, i64 by, i64 bz,
                                  ConeMutant mu = ConeMutant::kNone) {
  const i64 t[3] = {bx - w.z[0], by - w.z[1], bz - w.z[2]};
  const i64 h = t[0] * w.e[0] + t[1] * w.e[1] + t[2] * w.e[2];
  // Le mutant kNoPositivity omet ce test : comme seul H^2 intervient ensuite,
  // il accepte alors le cone OPPOSE, c'est-a-dire des `b` situes du cote de
  // `a` — exactement la faute que la positivite interdit.
  if (mu != ConeMutant::kNoPositivity) {
    const bool positive = (mu == ConeMutant::kAcceptEquality) ? (h >= 0) : (h > 0);
    if (!positive) return kLaneNone;
  }
  const i64 x2 = t[0] * t[0] + t[1] * t[1] + t[2] * t[2];

  // Coefficients : q3 exige 4 H^2 > E2 X2, q4 exige 3 H^2 > E2 X2.
  int c3 = 4, c4 = 3;
  if (mu == ConeMutant::kSwapCoeff) { c3 = 3; c4 = 4; }

  if (mu == ConeMutant::kNarrowI64) {
    // MUTANT : tout en i64. Le produit E2*X2 deborde des que les distances
    // depassent quelques milliers d'unites ; la decision devient aleatoire.
    const i64 hh = h * h;
    const i64 ex = w.e2 * x2;
    if ((i64)c4 * hh > ex) return kLaneQ4;
    if ((i64)c3 * hh > ex) return kLaneQ3;
    return kLaneQ2;
  }

  const i128 hh = (i128)h * (i128)h;
  const i128 ex = (i128)w.e2 * (i128)x2;
  if (mu == ConeMutant::kAcceptEquality) {
    if ((i128)c4 * hh >= ex) return kLaneQ4;
    if ((i128)c3 * hh >= ex) return kLaneQ3;
    return kLaneQ2;
  }
  if ((i128)c4 * hh > ex) return kLaneQ4;
  if ((i128)c3 * hh > ex) return kLaneQ3;
  return kLaneQ2;
}

// ---------------------------------------------------------------------------
// PREDICAT PONCTUEL, REPRESENTATION HISTORIQUE (g, Q). Il ne partage aucune
// quantite intermediaire avec la fonction ci-dessus : ni H, ni E2, ni X2. Son
// role est d'etre le second temoin arithmetique du selftest ; il n'est jamais
// sur le chemin produit.
//
//   d = b-a, D2 = d.d, U = 2z-a-b, g = D2 - U.U, Q = D2 (U.U) - (U.d)^2
//   q2 : g > 0      q3 : g > 0 et 3 g^2 > 4 Q      q4 : g > 0 et g^2 > 2 Q
// ---------------------------------------------------------------------------
MHGP_HD inline int lane_of_target_gq(i64 ax, i64 ay, i64 az, i64 zx, i64 zy, i64 zz,
                                     i64 bx, i64 by, i64 bz) {
  const i64 d[3] = {bx - ax, by - ay, bz - az};
  const i64 u[3] = {2 * zx - ax - bx, 2 * zy - ay - by, 2 * zz - az - bz};
  const i64 d2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
  const i64 uu = u[0] * u[0] + u[1] * u[1] + u[2] * u[2];
  const i64 ud = u[0] * d[0] + u[1] * d[1] + u[2] * d[2];
  const i64 g = d2 - uu;
  if (g <= 0) return kLaneNone;
  const i128 q = (i128)d2 * (i128)uu - (i128)ud * (i128)ud;
  const i128 gg = (i128)g * (i128)g;
  if (gg > 2 * q) return kLaneQ4;
  if (3 * gg > 4 * q) return kLaneQ3;
  return kLaneQ2;
}

// ---------------------------------------------------------------------------
// PREDICAT PONCTUEL, REPRESENTATION (H, R). Troisieme ecriture, celle qui sert
// la porte NONE : elle materialise le produit vectoriel.
// ---------------------------------------------------------------------------
MHGP_HD inline int lane_of_target_hr(const Witness& w, i64 bx, i64 by, i64 bz) {
  const i64 t[3] = {bx - w.z[0], by - w.z[1], bz - w.z[2]};
  const i64 h = t[0] * w.e[0] + t[1] * w.e[1] + t[2] * w.e[2];
  if (h <= 0) return kLaneNone;
  const i64 c[3] = {t[1] * w.e[2] - t[2] * w.e[1], t[2] * w.e[0] - t[0] * w.e[2],
                    t[0] * w.e[1] - t[1] * w.e[0]};
  const i128 r = (i128)c[0] * (i128)c[0] + (i128)c[1] * (i128)c[1] + (i128)c[2] * (i128)c[2];
  const i128 hh = (i128)h * (i128)h;
  if (2 * hh > r) return kLaneQ4;
  if (3 * hh > r) return kLaneQ3;
  return kLaneQ2;
}

// ---------------------------------------------------------------------------
// LA BOITE CIBLE.
// ---------------------------------------------------------------------------
struct Box {
  i64 lo[3] = {0, 0, 0};
  i64 hi[3] = {0, 0, 0};
};

MHGP_HD inline void box_corner(const Box& box, int k, i64* out) {
  out[0] = (k & 1) ? box.hi[0] : box.lo[0];
  out[1] = (k & 2) ? box.hi[1] : box.lo[1];
  out[2] = (k & 4) ? box.hi[2] : box.lo[2];
}

// ---------------------------------------------------------------------------
// PORTE `ALL` PAR HUIT COINS — UNE EQUIVALENCE.
//
// Rend la lane la plus forte q telle que la boite ENTIERE est incluse dans
// C_q. Le minimum des lanes des huit coins repond exactement, parce que C_q
// est convexe ouvert et que la boite est l'enveloppe convexe de ses coins.
//
// Un mutant qui ne teste que le centre, ou qui omet un coin, doit mourir : la
// fixture `B = {12} x [8,12] x {10}` avec a=(10,10,10) et z=(11,10,10) a son
// centre strictement dans C4 et deux coins hors de C3.
// ---------------------------------------------------------------------------
// Le parametre `floor` est une SORTIE ANTICIPEE, pas un relachement : des que
// le minimum courant tombe sous `floor`, la reponse exacte ne peut plus
// atteindre `floor` et le classifieur rend une valeur strictement inferieure.
// Toute reponse >= floor reste donc la lane ALL exacte. L'appelant ne demande
// jamais autre chose : un temoin deja credite q3 n'a d'interet qu'en q4.
//
// Le centre est teste EN PREMIER : la boite est incluse dans le cone convexe
// seulement si son centre y est, donc `lane(centre)` majore la reponse. Un
// seul predicat elimine ainsi la grande majorite des couples (temoin, noeud).
MHGP_HD inline int all_lane_of_box(const Witness& w, const Box& box,
                                   ConeMutant mu = ConeMutant::kNone, int floor = kLaneQ2,
                                   long long* evals = nullptr) {
  if (mu == ConeMutant::kCenterOnly) {
    // MUTANT : le centre entier de la boite (arrondi vers le bas) decide seul.
    if (evals != nullptr) ++*evals;
    return lane_of_target(w, (box.lo[0] + box.hi[0]) / 2, (box.lo[1] + box.hi[1]) / 2,
                          (box.lo[2] + box.hi[2]) / 2);
  }
  const int centre = lane_of_target(w, (box.lo[0] + box.hi[0]) / 2, (box.lo[1] + box.hi[1]) / 2,
                                    (box.lo[2] + box.hi[2]) / 2, mu);
  if (evals != nullptr) ++*evals;
  if (centre < floor) return centre;
  const int corners = (mu == ConeMutant::kSevenCorners) ? 7 : 8;
  int best = kLaneQ4;
  for (int k = 0; k < corners; ++k) {
    i64 c[3];
    box_corner(box, k, c);
    const int lane = lane_of_target(w, c[0], c[1], c[2], mu);
    if (evals != nullptr) ++*evals;
    if (lane < best) best = lane;
    if (best < floor) return best;
  }
  return best;
}

// ---------------------------------------------------------------------------
// PORTE `NONE` FAIL-OPEN.
//
// Elle certifie qu'AUCUN point de la boite n'est dans C_q, sans jamais
// descendre. `H` est affine en `b` : son maximum sur la boite est exact. Les
// trois composantes de `t x e` sont elles aussi affines en `b` : l'intervalle
// exact de chacune donne sa distance a zero, et la somme des carres de ces
// trois distances est un MINORANT de R malgre les correlations.
//
//   Hmax <= 0                     => aucun point n'est meme dans C2 ;
//   c * max(Hmax,0)^2 <= Rlb      => aucun point n'est dans C_q, c = 3 (q3),
//                                    c = 2 (q4).
//
// Pour tout `b` de la boite avec H(b) > 0 on a 0 < H(b) <= Hmax et
// R(b) >= Rlb, donc c H(b)^2 <= c Hmax^2 <= Rlb <= R(b) : le predicat est
// faux. L'egalite reste hors du cone ouvert, le rejet est donc sur.
//
// Ce minorant peut repondre UNKNOWN a tort — jamais NONE a tort. Le mutant
// `kNoneUsesRmax` remplace Rlb par un MAJORANT et refute alors des boites
// vivantes : il doit mourir.
// ---------------------------------------------------------------------------
MHGP_HD inline unsigned none_mask_of_box(const Witness& w, const Box& box,
                                         ConeMutant mu = ConeMutant::kNone) {
  // Maximum exact de H = (b - z) . e sur la boite : chaque axe est independant.
  i64 hmax = 0;
  for (int d = 0; d < 3; ++d) {
    const i64 lo = (box.lo[d] - w.z[d]) * w.e[d];
    const i64 hi = (box.hi[d] - w.z[d]) * w.e[d];
    hmax += (lo > hi) ? lo : hi;
  }
  if (hmax <= 0) return kNoneQ2 | kNoneQ3 | kNoneQ4;

  // Intervalle exact de chaque composante de t x e, t = b - z.
  // (t x e)_0 = t1 e2 - t2 e1, etc. Chaque terme est monotone en une seule
  // coordonnee de `b`, donc l'intervalle du produit est exact.
  i128 rlb = 0;
  i128 rub = 0;
  for (int j = 0; j < 3; ++j) {
    const int p = (j + 1) % 3;
    const int q = (j + 2) % 3;
    // terme 1 : (b_p - z_p) * e_q ; terme 2 : -(b_q - z_q) * e_p
    const i64 a1 = (box.lo[p] - w.z[p]) * w.e[q];
    const i64 b1 = (box.hi[p] - w.z[p]) * w.e[q];
    const i64 a2 = -((box.lo[q] - w.z[q]) * w.e[p]);
    const i64 b2 = -((box.hi[q] - w.z[q]) * w.e[p]);
    const i64 lo = ((a1 < b1) ? a1 : b1) + ((a2 < b2) ? a2 : b2);
    const i64 hi = ((a1 > b1) ? a1 : b1) + ((a2 > b2) ? a2 : b2);
    i64 dist = 0;
    if (lo > 0) dist = lo;
    else if (hi < 0) dist = -hi;
    rlb += (i128)dist * (i128)dist;
    const i64 far = ((lo < 0 ? -lo : lo) > (hi < 0 ? -hi : hi)) ? (lo < 0 ? -lo : lo)
                                                               : (hi < 0 ? -hi : hi);
    rub += (i128)far * (i128)far;
  }
  const i128 bound = (mu == ConeMutant::kNoneUsesRmax) ? rub : rlb;
  const i128 hh = (i128)hmax * (i128)hmax;
  unsigned mask = 0;
  if (3 * hh <= bound) mask |= kNoneQ3;
  if (2 * hh <= bound) mask |= kNoneQ4;
  // Une boite refutee en q3 l'est a fortiori en q4 : C4 est incluse dans C3.
  if ((mask & kNoneQ3) != 0) mask |= kNoneQ4;
  return mask;
}

}  // namespace cone
}  // namespace mhgp3v
