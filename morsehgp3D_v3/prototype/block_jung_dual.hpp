// MorseHGP3D v3 — `BlockJungDual64` : UN GROUPE FERME UN RECTANGLE ENTIER.
//
// Specification : section 5.9 de
// audits/AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md.
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=certificat_exact_fail_open,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// L'ABI QUI MANQUAIT
//
// `jung_dual.hpp` decide si un groupe de temoins couvre UNE paire. Pour un
// rectangle il fallait jusqu'ici developper les `PairId`, ce que toute
// l'architecture interdit. Cette primitive comble exactement ce trou : a base
// et poids FIXES, elle decide le rectangle entier en soixante-quatre tests.
//
// Avec `L = somme w`, `Z = somme w z` et `Q = somme w ||z||^2` :
//
//   A0 = -L (a.b) + (a+b).Z - Q
//   C0 = L (a x b) - a x Z - Z x b
//   q4 : A0 > 0 et 2 A0^2 > ||C0||^2
//   q3 : A0 > 0 et 3 A0^2 > ||C0||^2
//
// ---------------------------------------------------------------------------
// POURQUOI C'EST LA MEME CHOSE QUE LE DUAL, ET POURQUOI 64 COINS SUFFISENT
//
// L'identite avec la forme `A/P/R` de `jung_dual.hpp` se verifie a la main. En
// developpant `||a+b-2z||^2 = ||a+b||^2 - 4(a+b).z + 4||z||^2` :
//
//   A = L D - somme w ||a+b-2z||^2 = -4L(a.b) + 4(a+b).Z - 4Q = 4 A0.
//
// Et avec `P = L(a+b) - 2Z` et `d = b-a`, l'identite de Lagrange donne
// `R = D||P||^2 - (P.d)^2 = ||P x d||^2`. Or
// `(a+b) x (b-a) = 2 (a x b)`, donc
// `P x d = 2[L(a x b) - a x Z - Z x b] = 2 C0`, d'ou `R = 4 ||C0||^2`.
// Le test `A^2 > 2R` devient `16 A0^2 > 8 ||C0||^2`, c'est-a-dire
// `2 A0^2 > ||C0||^2`. Les deux ecritures decident donc la meme chose — et la
// porte l'exige sur des boites reduites a des points.
//
// LE POINT STRUCTUREL EST AILLEURS. A `b` fixe, `A0` est affine en `a` — le
// terme `-L(a.b)` et le terme `(a+b).Z` le sont tous deux — et `C0` aussi,
// puisque `L(a x b)` et `-a x Z` sont lineaires en `a`. Symetriquement a `a`
// fixe. Or la lane est le cone de Lorentz STRICT `{A0>0, 2A0^2 > ||C0||^2}`,
// qui est convexe. Le meme argument en deux temps que `SOC64` s'applique donc :
//
//   1. a chaque coin `b_j` fixe, les huit coins `a_i` passent, donc tout `A`
//      passe par convexite en `a` ;
//   2. a `a` quelconque dans `A`, les huit coins `b_j` passent, donc tout `B`
//      passe par convexite en `b`.
//
// Passer les `8x8 = 64` couples de coins est donc NECESSAIRE ET SUFFISANT pour
// tout le produit AABB continu `A x B`. Ce n'est pas une relaxation : c'est une
// equivalence, parce que `(A0,C0)` est vraiment affine en chaque argument — a
// la difference de `SOC64`, ou `e` et `t` partagent le meme `z`.
//
// CE QUI RESTE INTERDIT. Reproposer des poids DIFFERENTS a chaque coin puis
// transferer leur union au rectangle : la base et ses poids doivent etre
// communs a toute la tuile, et reproposes dans ses enfants apres un split. Un
// echec ne vaut jamais `NONE` — une autre ponderation peut reussir.
//
// ---------------------------------------------------------------------------
// LARGEURS PROUVEES, PROFIL u16
//
// Avec `U = 65535` et `1 <= L <= 65535` :
//   |Z_i|   <= L U          = 4,29e9      (33 bits)
//   Q       <= 3 L U^2      = 8,45e14     (50 bits)
//   |A0|    <= 3 L U^2      = 8,45e14     (50 bits)
//   |C0_i|  <= 2 L U^2      = 5,63e14     (49 bits)
//   A0^2    <= 7,14e29                    (100 bits)
//   ||C0||^2 <= 9,52e29                   (100 bits)
// `i128` suffit. Le preflight de `L` somme en type large AVANT tout rejet, et
// toute somme est elargie avant `a+b`, avant chaque produit scalaire ou
// vectoriel, et avant `||z||^2`.
#pragma once

#include <cstdint>

#include "mhgp/mhgp.hpp"
#include "spindle_cone.hpp"

namespace mhgp3v {
namespace bjd {

using mhgp::i128;
using mhgp::i64;

using cone::Box;
using cone::box_corner;
using cone::kLaneNone;
using cone::kLaneQ2;
using cone::kLaneQ3;
using cone::kLaneQ4;

constexpr i64 kMaxPoids = 65535;   // borne de `L`, prouvee ci-dessus

// ---------------------------------------------------------------------------
// MUTANTS. Chacun casse une decision exacte de la primitive de bloc.
// ---------------------------------------------------------------------------
enum class BjdMutant {
  kNone,
  kNoPositivity,    // oublie A0 > 0
  kAcceptEquality,  // >= au lieu de > : la frontiere entre dans le cone
  kSwapCoeff,       // confond les coefficients 2 (q4) et 3 (q3)
  kCornersDiagonal, // 8 couples diagonaux au lieu des 64 : l'equivalence tombe
  kCentreOnly,      // ne teste que le couple de centres
  kCrossSigne,      // inverse un signe du produit vectoriel de `C0`
};

inline const char* bjd_mutant_name(BjdMutant m) {
  switch (m) {
    case BjdMutant::kNone: return "none";
    case BjdMutant::kNoPositivity: return "bjd-no-positivity";
    case BjdMutant::kAcceptEquality: return "bjd-accept-equality";
    case BjdMutant::kSwapCoeff: return "bjd-swap-coeff";
    case BjdMutant::kCornersDiagonal: return "bjd-corners-diagonal";
    case BjdMutant::kCentreOnly: return "bjd-centre-only";
    case BjdMutant::kCrossSigne: return "bjd-cross-signe";
  }
  return "?";
}

// Precalcul de la base : `L`, `Z` et `Q` ne dependent que du groupe, jamais du
// rectangle. C'est ce qui rend la primitive utilisable sur une tuile entiere.
struct Base {
  i64 L = 0;
  i64 Z[3] = {0, 0, 0};
  i128 Q = 0;
  int n = 0;
  bool valide = false;
};

// Le preflight de `L` somme en type large AVANT tout rejet : une banque de
// poids epuisee rend une base INVALIDE, jamais une couverture par defaut.
inline Base make_base(const i64 z[][3], const i64 w[], int k) {
  Base b{};
  if (k <= 0 || k > 8) return b;
  i128 somme = 0;
  for (int i = 0; i < k; ++i) {
    if (w[i] <= 0) return b;
    somme += (i128)w[i];
  }
  if (somme > (i128)kMaxPoids) return b;
  b.L = (i64)somme;
  for (int i = 0; i < k; ++i) {
    for (int j = 0; j < 3; ++j) b.Z[j] += w[i] * z[i][j];
    b.Q += (i128)w[i] * ((i128)z[i][0] * z[i][0] + (i128)z[i][1] * z[i][1] +
                         (i128)z[i][2] * z[i][2]);
  }
  b.n = k;
  b.valide = true;
  return b;
}

// Lane fermee par la base pour le couple ponctuel `(a,b)`.
MHGP_HD inline int bjd_lane_point(const Base& base, const i64 a[3], const i64 b[3],
                                  BjdMutant mu = BjdMutant::kNone) {
  if (!base.valide) return kLaneNone;
  const i128 L = (i128)base.L;
  const i128 ab = (i128)a[0] * b[0] + (i128)a[1] * b[1] + (i128)a[2] * b[2];
  i128 apbZ = 0;
  for (int j = 0; j < 3; ++j) apbZ += (i128)(a[j] + b[j]) * (i128)base.Z[j];
  const i128 a0 = -L * ab + apbZ - base.Q;

  if (mu != BjdMutant::kNoPositivity) {
    const bool positif = (mu == BjdMutant::kAcceptEquality) ? (a0 >= 0) : (a0 > 0);
    if (!positif) return kLaneNone;
  }

  // `C0 = L (a x b) - a x Z - Z x b`, composante par composante.
  const int sg = (mu == BjdMutant::kCrossSigne) ? -1 : 1;
  i128 c0[3];
  for (int j = 0; j < 3; ++j) {
    const int p = (j + 1) % 3, q = (j + 2) % 3;
    const i128 axb = (i128)a[p] * b[q] - (i128)a[q] * b[p];
    const i128 axZ = (i128)a[p] * base.Z[q] - (i128)a[q] * base.Z[p];
    const i128 Zxb = (i128)base.Z[p] * b[q] - (i128)base.Z[q] * b[p];
    c0[j] = L * axb - axZ - (i128)sg * Zxb;
  }
  const i128 nc = c0[0] * c0[0] + c0[1] * c0[1] + c0[2] * c0[2];
  const i128 aa = a0 * a0;
  i128 c4 = 2, c3 = 3;
  if (mu == BjdMutant::kSwapCoeff) { c4 = 3; c3 = 2; }
  if (mu == BjdMutant::kAcceptEquality) {
    if (c4 * aa >= nc) return kLaneQ4;
    if (c3 * aa >= nc) return kLaneQ3;
    return kLaneQ2;
  }
  if (c4 * aa > nc) return kLaneQ4;
  if (c3 * aa > nc) return kLaneQ3;
  return kLaneQ2;
}

// ---------------------------------------------------------------------------
// LES SOIXANTE-QUATRE COINS. Rend la lane la plus forte que la base ferme sur
// TOUT le produit AABB continu `A x B`. C'est une EQUIVALENCE, pas une
// suffisance : `(A0,C0)` est affine en chaque argument et le cone est convexe.
//
// `floor` est une sortie anticipee. Comme pour `SOC64`, une valeur rendue sous
// le seuil n'est pas une lane exacte : elle vaut `kLaneNone` seulement quand ce
// minimum est atteint, sinon la sortie est marquee par le retour negatif.
// ---------------------------------------------------------------------------
constexpr int kUnknownBelowFloor = -1;

MHGP_HD inline int bjd_lane_box(const Base& base, const Box& A, const Box& B,
                                BjdMutant mu = BjdMutant::kNone, int floor = kLaneQ2,
                                long long* couples = nullptr) {
  if (!base.valide) return kLaneNone;
  i64 ac[3], bc[3];
  for (int j = 0; j < 3; ++j) {
    ac[j] = (A.lo[j] + A.hi[j]) / 2;
    bc[j] = (B.lo[j] + B.hi[j]) / 2;
  }
  const int centre = bjd_lane_point(base, ac, bc, mu);
  if (couples != nullptr) ++*couples;
  if (mu == BjdMutant::kCentreOnly) return centre;
  if (centre < floor)
    return (centre == kLaneNone) ? kLaneNone : kUnknownBelowFloor;

  int best = kLaneQ4;
  if (mu == BjdMutant::kCornersDiagonal) {
    for (int k = 0; k < 8; ++k) {
      i64 pa[3], pb[3];
      box_corner(A, k, pa);
      box_corner(B, k, pb);
      const int lane = bjd_lane_point(base, pa, pb, mu);
      if (couples != nullptr) ++*couples;
      if (lane < best) best = lane;
      if (best < floor) return (best == kLaneNone) ? kLaneNone : kUnknownBelowFloor;
    }
    return best;
  }
  for (int i = 0; i < 8; ++i) {
    i64 pa[3];
    box_corner(A, i, pa);
    for (int j = 0; j < 8; ++j) {
      i64 pb[3];
      box_corner(B, j, pb);
      const int lane = bjd_lane_point(base, pa, pb, mu);
      if (couples != nullptr) ++*couples;
      if (lane < best) best = lane;
      if (best < floor) return (best == kLaneNone) ? kLaneNone : kUnknownBelowFloor;
    }
  }
  return best;
}

}  // namespace bjd
}  // namespace mhgp3v
