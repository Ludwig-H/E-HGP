// AcuteOwnerBoxGateway — classifier `A x B x C` exact, SANS jamais expanser une
// paire.
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=diagnostic_counter_only,
//         public_status=not_claimed.
//
// Specification : audits/AUDIT_CONTRE_RECEPTION_PORTEUR_AIGU_207B542_20260815.md
// et audits/AUDIT_SUIVI_PORTEUR_AIGU_GATEWAY_JUNG_207B542_20260815.md.
//
// ---------------------------------------------------------------------------
// LE PROBLEME QU'IL RESOUT
//
// L'elagage precedent descendait l'octree POUR CHAQUE ANCRE. Il rendait le coût
// par ancre logarithmique, mais il fallait d'abord materialiser les ancres — et
// `two_lines` en a `Theta(n^2)`. Aucune constante ne repare un mauvais
// quantificateur.
//
// Ici la recursion porte sur les TROIS boites a la fois. Un bloc `A x B x C`
// certifie mort n'expanse AUCUNE paire ; un bloc certifie entierement porteur
// est emis SYMBOLIQUEMENT, avec sa masse `|A| |B| |C|`, sans enumerer un seul
// triplet. Seuls les blocs indecis descendent, et seules leurs feuilles sont
// testees point a point.
//
// ---------------------------------------------------------------------------
// LES TROIS QUANTITES, ET POURQUOI ELLES SONT SEPARABLES PAR AXE
//
// Avec `D = |a-b|^2`, `E = |a-x|^2`, `X = |b-x|^2` et `Phi = (a-x).(b-x)`, un
// `x` est porteur aigu de l'arete `(a,b)` exactement quand
//
//     E <= D,        X <= D,        Phi > 0
//
// — c'est `H < 0` du fuseau, puisque `H = -Phi`. En posant
//
//     Delta_E = D - E,     Delta_X = D - X,
//
// la condition devient `Delta_E >= 0`, `Delta_X >= 0`, `Phi > 0`.
//
// LE POINT CLE : ces trois quantites sont des SOMMES DE TERMES PAR AXE.
//
//     Phi     = somme_k (a_k - x_k)(b_k - x_k)
//     Delta_E = somme_k [ (a_k - b_k)^2 - (a_k - x_k)^2 ]
//     Delta_X = somme_k [ (a_k - b_k)^2 - (b_k - x_k)^2 ]
//
// L'extremum sur un produit de trois AABB est donc la somme des extrema sur les
// trois produits d'INTERVALLES. C'est exact — pas une relaxation — et cela
// preserve la correlation par `a_k`, ce qui est precisement ce que des bornes
// independantes sur `D` et `E` detruiraient.
//
// Chaque extremum d'axe se calcule en `O(1)` :
//
//   `Phi_max`     : convexe en `x`, affine en `a` et `b` -> HUIT coins (24 au
//                   total, le compte annonce par l'audit).
//   `Phi_min`     : le minimum en `x` est a `(a+b)/2` ecrete dans `X_k`, donc
//                   quatre candidats par axe (12 au total — meme compte).
//   `Delta_E,max` : `(a-b)^2` maximal veut `b` au coin le plus LOIN de `a` ;
//                   `(a-x)^2` minimal veut `x = clamp(a, X_k)`. `Delta_E` etant
//                   AFFINE en `a` — il vaut `(x-b)(2a-b-x)` — les extrema en
//                   `a` sont aux coins de `A_k`.
//   `Delta_E,min` : `(a-b)^2` minimal veut `b = clamp(a, B_k)` ; `(a-x)^2`
//                   maximal veut `x` au coin le plus LOIN.
//   `Delta_X`     : identique par la symetrie `a <-> b`.
//
// Les six formules ont ete verifiees contre l'enumeration exhaustive de tous les
// entiers de six mille triplets d'intervalles : ZERO ecart. Ma premiere version
// de `Delta_E,min` prenait `b` a un coin au lieu du clamp, et elle est morte la.
//
// ---------------------------------------------------------------------------
// CE QUE LE CLASSIFIEUR NE PEUT PAS DECIDER
//
// `MIXED` reste un SURENSEMBLE : les extrema des trois contraintes peuvent etre
// atteints par trois triplets differents, donc « aucune contrainte n'est
// uniformement violee » n'implique pas « un triplet les satisfait toutes ».
// C'est un classifieur fail-open, et c'est le contrat.
//
// L'owner CANONIQUE — longueur maximale puis `EdgeKey` minimale — ne peut pas se
// decider sur des boites, puisqu'il depend des `PointId`. `ALL_STRICT` exige
// donc `Delta_E,min > 0` et `Delta_X,min > 0` STRICTEMENT : sans egalite
// possible, l'owner faible et l'owner canonique coincident sur tout le bloc.
#ifndef MHGP3V_PROTOTYPE_ACUTE_OWNER_GATEWAY_HPP
#define MHGP3V_PROTOTYPE_ACUTE_OWNER_GATEWAY_HPP

#include <algorithm>

namespace mhgp3v {
namespace acute {

using i64 = long long;
using i128 = __int128;

struct Interval {
  i64 lo, hi;
};

struct BoxI {
  Interval ax[3];
};

enum class Verdict {
  kDeadPhi,      // aucun `x` du bloc n'est aigu pour aucune ancre du bloc
  kDeadOwnerE,   // `|ax| > |ab|` partout : `(a,b)` n'est jamais maximale
  kDeadOwnerX,   // `|bx| > |ab|` partout
  kAllStrict,    // TOUT triplet du bloc est porteur, owner canonique compris
  kMixed         // indecis : il faut descendre
};

inline const char* verdict_nom(Verdict v) {
  switch (v) {
    case Verdict::kDeadPhi: return "DEAD_PHI";
    case Verdict::kDeadOwnerE: return "DEAD_OWNER_E";
    case Verdict::kDeadOwnerX: return "DEAD_OWNER_X";
    case Verdict::kAllStrict: return "ALL_STRICT";
    case Verdict::kMixed: return "MIXED";
  }
  return "?";
}

inline i64 clamp_i(i64 v, const Interval& I) {
  return v < I.lo ? I.lo : (v > I.hi ? I.hi : v);
}

// Coin de `I` le plus ELOIGNE de `v` — celui qui maximise `(v - .)^2`.
inline i64 coin_loin(i64 v, const Interval& I) {
  const i64 dl = v - I.lo, dh = v - I.hi;
  return (dl < 0 ? -dl : dl) >= (dh < 0 ? -dh : dh) ? I.lo : I.hi;
}

// ---- `Phi` par axe : `(a-x)(b-x)`.
inline i128 phi_max_axe(const Interval& A, const Interval& B, const Interval& X) {
  i128 m = 0;
  bool premier = true;
  const i64 as[2] = {A.lo, A.hi}, bs[2] = {B.lo, B.hi}, xs[2] = {X.lo, X.hi};
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      for (int k = 0; k < 2; ++k) {
        const i128 v = (i128)(as[i] - xs[k]) * (i128)(bs[j] - xs[k]);
        if (premier || v > m) { m = v; premier = false; }
      }
  return m;
}

inline i128 phi_min_axe(const Interval& A, const Interval& B, const Interval& X) {
  i128 m = 0;
  bool premier = true;
  const i64 as[2] = {A.lo, A.hi}, bs[2] = {B.lo, B.hi};
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j) {
      // Convexe en `x` : le minimum est au sommet `(a+b)/2`, ecrete dans `X`.
      // Les entiers imposent d'essayer les deux voisins du demi-entier.
      const i64 s = as[i] + bs[j];
      i64 cand[4] = {X.lo, X.hi, clamp_i(s / 2, X), clamp_i((s + 1) / 2, X)};
      for (int t = 0; t < 4; ++t) {
        const i128 v = (i128)(as[i] - cand[t]) * (i128)(bs[j] - cand[t]);
        if (premier || v < m) { m = v; premier = false; }
      }
    }
  return m;
}

// ---- `Delta_E = (a-b)^2 - (a-x)^2` par axe. AFFINE en `a`, donc coins de `A`.
inline i128 delta_max_axe(const Interval& A, const Interval& B, const Interval& X) {
  i128 m = 0;
  bool premier = true;
  const i64 as[2] = {A.lo, A.hi};
  for (int i = 0; i < 2; ++i) {
    const i64 a = as[i];
    const i64 b = coin_loin(a, B);   // maximise `(a-b)^2`
    const i64 x = clamp_i(a, X);     // minimise `(a-x)^2`
    const i128 v = (i128)(a - b) * (i128)(a - b) - (i128)(a - x) * (i128)(a - x);
    if (premier || v > m) { m = v; premier = false; }
  }
  return m;
}

inline i128 delta_min_axe(const Interval& A, const Interval& B, const Interval& X) {
  i128 m = 0;
  bool premier = true;
  const i64 as[2] = {A.lo, A.hi};
  for (int i = 0; i < 2; ++i) {
    const i64 a = as[i];
    const i64 b = clamp_i(a, B);     // minimise `(a-b)^2`
    const i64 x = coin_loin(a, X);   // maximise `(a-x)^2`
    const i128 v = (i128)(a - b) * (i128)(a - b) - (i128)(a - x) * (i128)(a - x);
    if (premier || v < m) { m = v; premier = false; }
  }
  return m;
}

struct Extrema {
  i128 phi_max, phi_min, dE_max, dE_min, dX_max, dX_min;
};

inline Extrema extrema(const BoxI& A, const BoxI& B, const BoxI& C) {
  Extrema e{0, 0, 0, 0, 0, 0};
  for (int k = 0; k < 3; ++k) {
    e.phi_max += phi_max_axe(A.ax[k], B.ax[k], C.ax[k]);
    e.phi_min += phi_min_axe(A.ax[k], B.ax[k], C.ax[k]);
    e.dE_max += delta_max_axe(A.ax[k], B.ax[k], C.ax[k]);
    e.dE_min += delta_min_axe(A.ax[k], B.ax[k], C.ax[k]);
    // `Delta_X` est `Delta_E` avec `a` et `b` echanges.
    e.dX_max += delta_max_axe(B.ax[k], A.ax[k], C.ax[k]);
    e.dX_min += delta_min_axe(B.ax[k], A.ax[k], C.ax[k]);
  }
  return e;
}

// Mutants, pour que les portes ne soient pas vertes par vacuite.
enum class GwMutant {
  kNone,
  kPhiLarge,      // `Phi_max < 0` au lieu de `<= 0` : declare MIXED des blocs morts
  kDeadLarge,     // `Delta_max <= 0` : tue des blocs qui portent
  kAllStrictLache // `ALL_STRICT` sans la stricte sur les `Delta` : owner faux
};

inline Verdict classifie(const Extrema& e, GwMutant mu = GwMutant::kNone) {
  // ---- LES TROIS CAUSES DE MORT, chacune exacte sur l'enveloppe continue.
  //
  // `Phi > 0` est requis, donc `Phi_max <= 0` tue le bloc. La LARGE est la
  // bonne : `Phi = 0` est l'angle droit, qui n'est pas un porteur.
  if (mu == GwMutant::kPhiLarge ? e.phi_max < 0 : e.phi_max <= 0)
    return Verdict::kDeadPhi;
  // `Delta >= 0` est requis, donc `Delta_max < 0` tue. La STRICTE est la bonne :
  // `Delta = 0` est `|ax| = |ab|`, encore dans la lentille fermee.
  if (mu == GwMutant::kDeadLarge ? e.dE_max <= 0 : e.dE_max < 0)
    return Verdict::kDeadOwnerE;
  if (mu == GwMutant::kDeadLarge ? e.dX_max <= 0 : e.dX_max < 0)
    return Verdict::kDeadOwnerX;
  // ---- LE BLOC ENTIEREMENT PORTEUR.
  //
  // Les `Delta` STRICTEMENT positifs interdisent toute egalite de longueur,
  // donc aucun tie-break `EdgeKey` n'est requis : l'owner faible et l'owner
  // canonique coincident sur tout le bloc. Sans cette stricte, un bloc
  // contenant un triangle equilateral serait emis avec trois owners.
  const bool delta_ok = (mu == GwMutant::kAllStrictLache)
                            ? (e.dE_min >= 0 && e.dX_min >= 0)
                            : (e.dE_min > 0 && e.dX_min > 0);
  if (e.phi_min > 0 && delta_ok) return Verdict::kAllStrict;
  return Verdict::kMixed;
}

}  // namespace acute
}  // namespace mhgp3v

#endif  // MHGP3V_PROTOTYPE_ACUTE_OWNER_GATEWAY_HPP
