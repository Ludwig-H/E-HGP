// MorseHGP3D v3 — `MidballBlockDepth` : LE PREDICAT q2 EXACT, SUR UN BLOC.
//
// Specification : section 4 et reponse Q5 de
// audits/AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md.
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=certificat_exact_fail_open,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// POURQUOI CE FICHIER EXISTE
//
// Un support q2 `{a,b}` ne possede qu'UN centre — le milieu — et donc qu'UNE
// boule, celle de diametre `ab`. Il n'y a aucun continuum a quantifier : le
// domaine des centres q2 est un point. Le certificat central en place teste
// pourtant une relaxation par intervalle de score, donc il est seulement
// SUFFISANT la ou l'exact est disponible.
//
// Le predicat exact est celui de Thales :
//
//   H(a,b,z) = (z-a).(b-z) > 0   <=>   z est STRICTEMENT dans la boule de
//                                      diametre ab
//
// ---------------------------------------------------------------------------
// CE QUI REND LE BLOC EXACT, ET NON UNE RELAXATION
//
// La somme se separe par axe :
//
//   H(a,b,z) = somme_i (z_i - a_i)(b_i - z_i)
//
// Chaque terme ne depend QUE de l'axe `i`. Le minimum sur le produit de boites
// est donc la SOMME des minima par axe — pas une borne, la valeur exacte.
//
// Sur un axe, `f(a,b,z) = (z-a)(b-z)` est :
//   - lineaire en `a`, de pente `-(b-z)` ;
//   - lineaire en `b`, de pente `(z-a)` ;
//   - CONCAVE en `z`, puisque `f = -z^2 + z(a+b) - ab`.
//
// Le minimum d'une fonction lineaire comme d'une fonction concave sur un
// intervalle est atteint a une EXTREMITE. Les huit sommets de l'axe suffisent
// donc exactement pour le minimum.
//
// Le maximum, lui, NE se reduit PAS aux sommets : une fonction concave atteint
// son maximum a l'interieur. Il faut le point stationnaire `z* = (a+b)/2`
// rabattu sur l'intervalle. Confondre les deux est le mutant
// `midball-max-aux-coins`, et il produirait de faux `NONE`.
//
// ---------------------------------------------------------------------------
// CE QUE LE VERDICT SIGNIFIE, ET CE QU'IL NE SIGNIFIE PAS
//
// `ALL`  : tout `PointId` de `C` est interieur a la boule diametrale de toute
//          paire de `A x B`. Sa population distincte est creditable en q2
//          AVANT tout fill, et avant toute descente.
// `NONE` : aucun point de l'enveloppe continue de `C` ne l'est. C'est un
//          elagage sur, jamais une fermeture.
// `MIXED`: le bloc doit etre scinde ou descendu.
//
// Ce verdict ferme la seule lane EVENEMENTIELLE q2. Ajouter la population de la
// boule diametrale a une lane superieure est le mutant `midball-prune-q3q4`,
// que les deux fixtures u16 de la section 2 de l'audit tuent : un q2 profond
// laisse subsister un q3 et un q4 de rang trois et quatre.
//
// ---------------------------------------------------------------------------
// LARGEURS PROUVEES, PROFIL u16
//
// Avec `U = 65535`, chaque facteur est dans `[-U, U]`, donc chaque terme est
// dans `[-U^2, U^2]` et la somme des trois dans `[-3U^2, 3U^2]`, soit
// `1,29e10 < 2^34`. `i64` suffit tres largement ; aucun `i128` n'est forme.
#pragma once

#include <cstdint>

#include "mhgp/mhgp.hpp"
#include "spindle_cone.hpp"

namespace mhgp3v {
namespace midball {

using mhgp::i64;

using cone::Box;

enum class MidballVerdict { kAll, kNone, kMixed };

// ---------------------------------------------------------------------------
// MUTANTS. Chacun casse une decision exacte de la primitive.
// ---------------------------------------------------------------------------
enum class MidballMutant {
  kNone,
  kMaxAuxCoins,     // maximum aux seuls sommets : la concavite est ignoree
  kAcceptEquality,  // `>=` au lieu de `>` : le shell entre dans l'interieur
  kDropAxis,        // oublie le troisieme axe : la separation devient fausse
  kMinAuMilieu,     // minimum au centre de la boite au lieu des extremites
  kSwapCoeffHC,     // confond les coefficients 2 (q4) et 3 (q3) de `(H,C)`
  kCorrelationIgnoree,  // borne `C_i` par un seul produit : la soustraction saute
};

inline const char* midball_mutant_name(MidballMutant m) {
  switch (m) {
    case MidballMutant::kNone: return "none";
    case MidballMutant::kMaxAuxCoins: return "midball-max-aux-coins";
    case MidballMutant::kAcceptEquality: return "midball-accept-equality";
    case MidballMutant::kDropAxis: return "midball-drop-axis";
    case MidballMutant::kMinAuMilieu: return "midball-min-au-milieu";
    case MidballMutant::kSwapCoeffHC: return "hc-swap-coeff";
    case MidballMutant::kCorrelationIgnoree: return "hc-correlation-ignoree";
  }
  return "?";
}

// Predicat ponctuel exact. `H > 0` vaut interieur strict, `H == 0` le shell.
MHGP_HD inline i64 midball_h(const i64 a[3], const i64 b[3], const i64 z[3]) {
  i64 h = 0;
  for (int i = 0; i < 3; ++i) h += (z[i] - a[i]) * (b[i] - z[i]);
  return h;
}

// Minimum EXACT du terme d'axe sur `[al,ah] x [bl,bh] x [zl,zh]`.
// Lineaire en `a` et `b`, concave en `z` : les huit sommets suffisent.
MHGP_HD inline i64 axis_min(i64 al, i64 ah, i64 bl, i64 bh, i64 zl, i64 zh,
                            MidballMutant mu) {
  if (mu == MidballMutant::kMinAuMilieu) {
    const i64 a = (al + ah) / 2, b = (bl + bh) / 2, z = (zl + zh) / 2;
    return (z - a) * (b - z);
  }
  i64 best = 0;
  bool first = true;
  for (int ia = 0; ia < 2; ++ia) {
    const i64 a = ia ? ah : al;
    for (int ib = 0; ib < 2; ++ib) {
      const i64 b = ib ? bh : bl;
      for (int iz = 0; iz < 2; ++iz) {
        const i64 z = iz ? zh : zl;
        const i64 v = (z - a) * (b - z);
        if (first || v < best) { best = v; first = false; }
      }
    }
  }
  return best;
}

// Maximum EXACT du terme d'axe. En `z` la fonction est CONCAVE : son maximum
// est au point stationnaire `(a+b)/2` rabattu sur `[zl,zh]`, jamais aux seules
// extremites. En `a` et `b` elle reste lineaire, donc les quatre sommets
// endpoint suffisent.
MHGP_HD inline i64 axis_max(i64 al, i64 ah, i64 bl, i64 bh, i64 zl, i64 zh,
                            MidballMutant mu) {
  i64 best = 0;
  bool first = true;
  for (int ia = 0; ia < 2; ++ia) {
    const i64 a = ia ? ah : al;
    for (int ib = 0; ib < 2; ++ib) {
      const i64 b = ib ? bh : bl;
      // Le stationnaire exact vaut `(a+b)/2`. Sous u16 la somme peut etre
      // impaire : on essaie les deux entiers encadrants, ce qui donne le
      // maximum exact sur les ENTIERS de l'intervalle, et le rabattement donne
      // le maximum continu lorsque le stationnaire sort de la boite.
      i64 cands[4] = {zl, zh, (a + b) / 2, (a + b) / 2 + 1};
      const int ncand = (mu == MidballMutant::kMaxAuxCoins) ? 2 : 4;
      for (int k = 0; k < ncand; ++k) {
        i64 z = cands[k];
        if (z < zl) z = zl;
        if (z > zh) z = zh;
        const i64 v = (z - a) * (b - z);
        if (first || v > best) { best = v; first = false; }
      }
    }
  }
  return best;
}

// Verdict de bloc. `ALL` credite, `NONE` elague, `MIXED` scinde ou descend.
MHGP_HD inline MidballVerdict midball_block(const Box& A, const Box& B, const Box& C,
                                            MidballMutant mu = MidballMutant::kNone,
                                            i64* out_min = nullptr,
                                            i64* out_max = nullptr) {
  const int naxes = (mu == MidballMutant::kDropAxis) ? 2 : 3;
  i64 hmin = 0, hmax = 0;
  for (int i = 0; i < naxes; ++i) {
    hmin += axis_min(A.lo[i], A.hi[i], B.lo[i], B.hi[i], C.lo[i], C.hi[i], mu);
    hmax += axis_max(A.lo[i], A.hi[i], B.lo[i], B.hi[i], C.lo[i], C.hi[i], mu);
  }
  if (out_min != nullptr) *out_min = hmin;
  if (out_max != nullptr) *out_max = hmax;
  const bool tout = (mu == MidballMutant::kAcceptEquality) ? (hmin >= 0) : (hmin > 0);
  if (tout) return MidballVerdict::kAll;
  // L'egalite est SHELL, jamais un interieur : un maximum nul ne credite rien
  // mais ne prouve pas non plus l'exterieur strict de tout le bloc.
  if (hmax <= 0) return MidballVerdict::kNone;
  return MidballVerdict::kMixed;
}

// ---------------------------------------------------------------------------
// `HCBlockDepth` : LA MEME IDEE, POUSSEE AUX LANES q3 ET q4.
//
// Le predicat universel s'ecrit aussi dans la representation `(H,C)`, avec
// `e = z-a`, `t = b-z`, `H = t.e` et `C = t x e` :
//
//   q2 : H > 0
//   q3 : H > 0  et  3 H^2 > ||C||^2
//   q4 : H > 0  et  2 H^2 > ||C||^2
//
// `H` est deja separable par axe, donc son minimum sur le bloc est EXACT.
// `||C||^2` ne l'est pas, mais chaque composante l'est presque : avec
// `(p,q)` la permutation cyclique de `i`,
//
//   C_i = t_p e_q - t_q e_p
//
// et les deux produits ne partagent que les coordonnees `z_p` et `z_q`. Borner
// chaque produit par son intervalle, puis soustraire, perd donc seulement cette
// correlation — ce qui ne peut creer que des `MIXED`, jamais un faux `ALL`.
//
// CE QUE CELA GAGNE SUR LE CERTIFICAT CENTRAL. Le masque central q4 teste
// `209 s <= 56 D`, c'est-a-dire le PIRE CAS DIRECTIONNEL `2-sqrt(3)` : il jette
// le terme `(U.d)^2` et suppose `C` maximal. Le present classifieur le calcule.
// Sur l'axe, ou `C` est nul, le central renonce alors que le vrai cœur atteint
// toute la boule diametrale.
//
// COUT. Trois minima d'axe pour `H`, puis douze produits d'intervalles pour les
// trois composantes de `C` : une trentaine de multiplications `i64`, contre
// soixante-quatre triplets pour `SOC64`. C'est un PREFILTRE, pas un remplacant :
// son echec reste `MIXED` et n'interdit pas a un certificat plus fin de fermer.
//
// LARGEURS. Sous u16, `|t_i|` et `|e_i|` sont bornes par `U = 65535`, donc
// `|C_i| <= 2 U^2 < 2^33` et `||C||^2 <= 3*(2U^2)^2 < 2^68` : `i128` est exige
// pour la comparaison, alors que `H` tient dans `i64`.

// Intervalle du produit de deux intervalles independants.
MHGP_HD inline void prod_interval(i64 xl, i64 xh, i64 yl, i64 yh, i64* lo, i64* hi) {
  const i64 c[4] = {xl * yl, xl * yh, xh * yl, xh * yh};
  i64 mn = c[0], mx = c[0];
  for (int k = 1; k < 4; ++k) {
    if (c[k] < mn) mn = c[k];
    if (c[k] > mx) mx = c[k];
  }
  *lo = mn;
  *hi = mx;
}

// Lane la plus forte que le bloc ferme UNIFORMEMENT. `kLaneNone` signifie
// seulement « ce classifieur ne conclut pas » : ce n'est jamais une refutation.
MHGP_HD inline int hc_lane_block(const Box& A, const Box& B, const Box& C,
                                 MidballMutant mu = MidballMutant::kNone,
                                 mhgp::i128* out_c2 = nullptr) {
  i64 hmin = 0, hmax = 0;
  const auto v = midball_block(A, B, C, mu, &hmin, &hmax);
  if (v != MidballVerdict::kAll) return cone::kLaneNone;   // meme `H > 0` echoue

  mhgp::i128 c2 = 0;
  for (int i = 0; i < 3; ++i) {
    const int p = (i + 1) % 3, q = (i + 2) % 3;
    // `t_p = b_p - z_p` et `e_q = z_q - a_q`, chacun sur son propre axe.
    const i64 tpl = B.lo[p] - C.hi[p], tph = B.hi[p] - C.lo[p];
    const i64 eql = C.lo[q] - A.hi[q], eqh = C.hi[q] - A.lo[q];
    const i64 tql = B.lo[q] - C.hi[q], tqh = B.hi[q] - C.lo[q];
    const i64 epl = C.lo[p] - A.hi[p], eph = C.hi[p] - A.lo[p];
    i64 u1l, u1h, u2l, u2h;
    prod_interval(tpl, tph, eql, eqh, &u1l, &u1h);
    prod_interval(tql, tqh, epl, eph, &u2l, &u2h);
    // La correlation entre les deux termes est perdue ici, et seulement ici.
    // MUTANT `hc-correlation-ignoree` : garder le seul premier produit revient
    // a oublier la soustraction, donc a SOUS-estimer `||C||^2` et a promettre
    // des `ALL` que la geometrie refute.
    const bool saute = (mu == MidballMutant::kCorrelationIgnoree);
    const i64 cil = saute ? u1l : (u1l - u2h);
    const i64 cih = saute ? u1h : (u1h - u2l);
    const i64 a1 = cil < 0 ? -cil : cil;
    const i64 a2 = cih < 0 ? -cih : cih;
    const i64 mx = a1 > a2 ? a1 : a2;
    c2 += (mhgp::i128)mx * (mhgp::i128)mx;
  }
  if (out_c2 != nullptr) *out_c2 = c2;
  const mhgp::i128 hh = (mhgp::i128)hmin * (mhgp::i128)hmin;
  i64 k4 = 2, k3 = 3;
  if (mu == MidballMutant::kSwapCoeffHC) { k4 = 3; k3 = 2; }
  if (mu == MidballMutant::kAcceptEquality) {
    if ((mhgp::i128)k4 * hh >= c2) return cone::kLaneQ4;
    if ((mhgp::i128)k3 * hh >= c2) return cone::kLaneQ3;
    return cone::kLaneQ2;
  }
  if ((mhgp::i128)k4 * hh > c2) return cone::kLaneQ4;
  if ((mhgp::i128)k3 * hh > c2) return cone::kLaneQ3;
  return cone::kLaneQ2;
}

}  // namespace midball
}  // namespace mhgp3v
