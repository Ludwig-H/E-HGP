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
};

inline const char* midball_mutant_name(MidballMutant m) {
  switch (m) {
    case MidballMutant::kNone: return "none";
    case MidballMutant::kMaxAuxCoins: return "midball-max-aux-coins";
    case MidballMutant::kAcceptEquality: return "midball-accept-equality";
    case MidballMutant::kDropAxis: return "midball-drop-axis";
    case MidballMutant::kMinAuMilieu: return "midball-min-au-milieu";
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

}  // namespace midball
}  // namespace mhgp3v
