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

// ---------------------------------------------------------------------------
// LE LEDGER DE VIVACITE `W_4`, AU NIVEAU BLOC.
//
// Le gateway aigu seul est exact mais enumere un objet CUBIQUE : un triangle
// aigu quelconque n'est pas une source q4. Il faut la CONJONCTION — l'ancre doit
// aussi etre `W_4`-vivante, c'est-a-dire que son fuseau contienne moins de
// `r4 = 8` points.
//
// `W_4(a,b) = { z : H > 0 et 3H^2 > |e|^2 |t|^2 }` avec `e = z-a`, `t = b-z`.
// Noter que `H = -Phi` : le meme produit scalaire sert aux deux clauses, avec le
// signe oppose. Un temoin q2 est un non-porteur, et reciproquement.
//
// DEUX TESTS DE BLOC, ET UNE MONOTONIE QUI EVITE DE TOUT RECALCULER.
//
//   `bloc_tout_w4`   : TOUT `z` de `Z` est dans `W_4(a,b)` pour TOUTE paire du
//                      rectangle -> on credite `|Z|` d'un coup.
//   `bloc_aucun_w2`  : AUCUN `z` de `Z` n'est meme dans `W_2` -> on jette `Z`.
//                      C'est `H_max <= 0`, soit `Phi_min >= 0` : le meme
//                      extremum que le gateway aigu calcule deja.
//
// MONOTONIE. Raffiner `A` ou `B` AFFAIBLIT le « pour toute paire », donc
// `L4_open` ne peut que CROITRE. Le credit acquis est donc definitif, et seule
// la frontiere indecise se re-teste chez les enfants. C'est ce qui rend le
// ledger heritable au lieu d'etre recalcule a chaque nœud.
//
// `bloc_tout_w4` enumere les coins : `H` est concave en `z` et affine en `a` et
// `b`, donc son MINIMUM sur le produit est en un sommet ; et la lane est decidee
// exactement par le meme argument en trois temps que `block_lane`.
inline bool bloc_tout_w4(const BoxI& A, const BoxI& B, const BoxI& Z, long long* ev) {
  i64 ca[8][3], cb[8][3], cz[8][3];
  auto coins = [](const Interval ax[3], i64 out[8][3]) {
    int nx = 0;
    for (int i = 0; i < (ax[0].lo == ax[0].hi ? 1 : 2); ++i)
      for (int j = 0; j < (ax[1].lo == ax[1].hi ? 1 : 2); ++j)
        for (int k = 0; k < (ax[2].lo == ax[2].hi ? 1 : 2); ++k) {
          out[nx][0] = i ? ax[0].hi : ax[0].lo;
          out[nx][1] = j ? ax[1].hi : ax[1].lo;
          out[nx][2] = k ? ax[2].hi : ax[2].lo;
          ++nx;
        }
    return nx;
  };
  const int na = coins(A.ax, ca), nb = coins(B.ax, cb), nz = coins(Z.ax, cz);
  for (int iz = 0; iz < nz; ++iz)
    for (int ia = 0; ia < na; ++ia) {
      const i64 e[3] = {cz[iz][0] - ca[ia][0], cz[iz][1] - ca[ia][1], cz[iz][2] - ca[ia][2]};
      const i64 e2 = e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
      if (e2 == 0) return false;  // `z` confondu avec une ancre : jamais temoin
      for (int ib = 0; ib < nb; ++ib) {
        if (ev) ++*ev;
        const i64 t[3] = {cb[ib][0] - cz[iz][0], cb[ib][1] - cz[iz][1], cb[ib][2] - cz[iz][2]};
        const i64 h = e[0] * t[0] + e[1] * t[1] + e[2] * t[2];
        if (h <= 0) return false;
        const i64 t2 = t[0] * t[0] + t[1] * t[1] + t[2] * t[2];
        // q4 : `3 H^2 > |e|^2 |t|^2`. STRICTE : l'egalite est la frontiere.
        if (!((i128)3 * (i128)h * (i128)h > (i128)e2 * (i128)t2)) return false;
      }
    }
  return true;
}

// `H_max <= 0` sur tout le bloc : aucun `z` n'est dans `W_2`, donc a fortiori
// pas dans `W_4`. `H = -Phi`, donc c'est `Phi_min >= 0`.
inline bool bloc_aucun_w2(const Extrema& e) { return e.phi_min >= 0; }

// Le verdict conjoint que l'audit `1d9425d` specifie en section 3.3.
enum class VerdictConjoint {
  kDeadW4,          // `L4_open >= r4` : toutes les paires du bloc sont mortes
  kDeadNoCarrier,   // le gateway aigu tue : aucun porteur possible
  kActiveAll,       // `U4_closed < r4` ET `ALL_STRICT` : bloc entierement actif
  kMixed
};

inline const char* verdict_conjoint_nom(VerdictConjoint v) {
  switch (v) {
    case VerdictConjoint::kDeadW4: return "DEAD_W4";
    case VerdictConjoint::kDeadNoCarrier: return "DEAD_NO_CARRIER";
    case VerdictConjoint::kActiveAll: return "ACTIVE_ALL";
    case VerdictConjoint::kMixed: return "MIXED";
  }
  return "?";
}

// `L4_open` : IDs universellement interieurs a `W_4` pour TOUTE paire du bloc.
// `U4_closed` : `L4_open` plus la frontiere encore indecise — le majorant.
//
// LA CONJONCTION NE SE SEPARE PAS, et c'est le point de la section 3.1 : « il
// existe une paire vivante » et « il existe une paire portant un carrier »
// peuvent etre realisees par des paires DIFFERENTES. `ACTIVE_ALL` exige donc les
// deux pour TOUTES les paires du bloc a la fois, pas leur simple coexistence.
inline VerdictConjoint classifie_conjoint(const Extrema& e, long long L4_open,
                                          long long U4_closed, int r4,
                                          GwMutant mu = GwMutant::kNone) {
  if (L4_open >= r4) return VerdictConjoint::kDeadW4;
  const Verdict v = classifie(e, mu);
  if (v == Verdict::kDeadPhi || v == Verdict::kDeadOwnerE || v == Verdict::kDeadOwnerX)
    return VerdictConjoint::kDeadNoCarrier;
  if (v == Verdict::kAllStrict && U4_closed < r4) return VerdictConjoint::kActiveAll;
  return VerdictConjoint::kMixed;
}

}  // namespace acute
}  // namespace mhgp3v

#endif  // MHGP3V_PROTOTYPE_ACUTE_OWNER_GATEWAY_HPP
