// MorseHGP3D v3 — `JungDual` : UN GROUPE DE TEMOINS FERME UNE PAIRE.
//
// Specification : section 5 de
// audits/AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md, forme
// entiere de la condition minimax de la section 9.1 de
// audits/AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md.
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=certificat_exact_fail_open,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// LE SAUT CONCEPTUEL, EN UNE PHRASE
//
// Le certificat central demande a UN temoin d'etre interieur a TOUTE sphere
// admissible de la paire. C'est une exigence tres forte, et c'est exactement
// pourquoi il echoue sur les longues aretes inter-amas : une telle arete
// possede toujours une sphere presque vide, qui bombe vers le vide entre les
// amas, et aucun temoin unique ne la rencontre.
//
// `JungDual` demande beaucoup moins : que pour toute sphere admissible, AU
// MOINS UN membre du groupe soit interieur. Le groupe couvre alors la paire et
// vaut un credit. Huit groupes a identites disjointes ferment q4, neuf ferment
// q3. Un temoin unique est le cas particulier `k=1`, et le predicat s'y reduit
// exactement a l'ecriture historique `(g,Q)` de `spindle_cone.hpp` — ce que la
// porte verifie.
//
// ---------------------------------------------------------------------------
// LA FORME ENTIERE
//
// Les centres des spheres admissibles vivent dans le disque de Jung du plan
// mediateur : `c = m + w` avec `m=(a+b)/2`, `w` orthogonal a `h=(b-a)/2` et
// `||w|| <= kappa ||h||`, ou `kappa^2 = 1/2` pour q4 et `1/3` pour q3.
//
// La marge interieure d'un temoin `z` est affine en `w` :
//   Phi_z(w) = ||h||^2 - ||m-z||^2 - 2 w . (m-z).
// Le groupe couvre la paire si et seulement si le maximum sur le disque du
// minimum sur le groupe reste strictement positif. Par minimax, cela equivaut a
// l'existence de poids `lambda_z >= 0` de somme un tels que, avec
// `alpha = ||h||^2 - somme lambda_z ||m-z||^2` et `p = m - somme lambda_z z` :
//
//   q4 : alpha > 0 et alpha^2 > 2 (||h||^2 ||p||^2 - (p.h)^2)
//   q3 : alpha > 0 et 3 alpha^2 > 4 (||h||^2 ||p||^2 - (p.h)^2)
//
// Ecrit avec des poids ENTIERS positifs `w_z` de somme `W`, toutes les
// divisions disparaissent. En posant `d = b-a` et `D = ||d||^2` :
//
//   A = W D - somme w_z ||a+b-2z||^2
//   P = W (a+b) - 2 somme w_z z
//   R = D ||P||^2 - (P.d)^2
//   q4 : A > 0 et   A^2 > 2 R
//   q3 : A > 0 et 3 A^2 > 4 R
//
// L'identite se verifie en substituant `lambda_z = w_z/W` : `alpha = A/(4W)`,
// `p = P/(2W)` et le membre de droite vaut `R/(16 W^2)`. Les facteurs `16 W^2`
// se simplifient des deux cotes.
//
// ---------------------------------------------------------------------------
// LARGEURS PROUVEES, PROFIL u16
//
// Avec `W <= 65535` :
//   D                <= 3*65535^2         = 1,29e10   (34 bits)
//   ||a+b-2z||^2     <= 3*(2*65535)^2     = 5,15e10   (36 bits)
//   |A|              <= W * 5,15e10       = 3,38e15   (52 bits)
//   |P_i|            <= 2 W * 65535       = 8,59e9    (34 bits)
//   ||P||^2          <= 3*(8,59e9)^2      = 2,21e20   (68 bits)
//   D ||P||^2        <= 2,86e30                       (102 bits)
//   (P.d)^2          <= (3*8,59e9*65535)^2 = 2,85e30  (102 bits)
//   A^2              <= 1,14e31                       (104 bits)
// Tout tient dans `i128` signe (borne 1,70e38), et rien n'est jamais divise.
#pragma once

#include <cstdint>

#include "mhgp/mhgp.hpp"
#include "spindle_cone.hpp"

namespace mhgp3v {
namespace jdual {

using mhgp::i128;
using mhgp::i64;

using cone::kLaneNone;
using cone::kLaneQ2;
using cone::kLaneQ3;
using cone::kLaneQ4;

constexpr int kMaxGroup = 3;   // Helly dans le plan : une base de trois suffit

// ---------------------------------------------------------------------------
// MUTANTS. Chacun casse une decision exacte du certificat dual.
// ---------------------------------------------------------------------------
enum class DualMutant {
  kNone,
  kNoPositivity,   // oublie A > 0 : le demi-espace oppose est accepte
  kAcceptEquality, // >= au lieu de > : la frontiere entre dans le certificat
  kSwapCoeff,      // confond (1,2) de q4 et (3,4) de q3
  kNarrowI64,      // ne promeut pas : D ||P||^2 atteint 102 bits et enroule
  kIgnoreWeights,  // force tous les poids a un : le minimax n'est plus atteint
};

inline const char* dual_mutant_name(DualMutant m) {
  switch (m) {
    case DualMutant::kNone: return "none";
    case DualMutant::kNoPositivity: return "dual-no-positivity";
    case DualMutant::kAcceptEquality: return "dual-accept-equality";
    case DualMutant::kSwapCoeff: return "dual-swap-coeff";
    case DualMutant::kNarrowI64: return "dual-narrow-i64";
    case DualMutant::kIgnoreWeights: return "dual-ignore-weights";
  }
  return "?";
}

// ---------------------------------------------------------------------------
// LE PREDICAT. Rend la lane la plus forte que le groupe ferme pour la paire.
//
// `z[k][3]` sont les temoins, `w[k]` leurs poids entiers strictement positifs.
// Aucun poids nul : un temoin de poids nul n'est pas dans le groupe, et le
// laisser fausserait le compte d'identites consommees.
// ---------------------------------------------------------------------------
MHGP_HD inline int dual_lane(const i64 a[3], const i64 b[3], const i64 z[][3],
                             const i64 w[], int k, DualMutant mu = DualMutant::kNone) {
  if (k <= 0 || k > kMaxGroup) return kLaneNone;
  i64 sw = 0;
  for (int i = 0; i < k; ++i) {
    const i64 wi = (mu == DualMutant::kIgnoreWeights) ? 1 : w[i];
    if (wi <= 0) return kLaneNone;
    sw += wi;
  }
  i64 d[3];
  for (int j = 0; j < 3; ++j) d[j] = b[j] - a[j];
  const i64 dd = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];

  // A = W D - somme w_z ||a+b-2z||^2, forme en 128 bits : W D atteint 52 bits
  // et chaque terme 36 bits, mais leur somme ponderee peut approcher 52 bits.
  i128 aa = (i128)sw * (i128)dd;
  i128 p[3] = {0, 0, 0};
  for (int j = 0; j < 3; ++j) p[j] = (i128)sw * (i128)(a[j] + b[j]);
  for (int i = 0; i < k; ++i) {
    const i64 wi = (mu == DualMutant::kIgnoreWeights) ? 1 : w[i];
    i64 u[3];
    for (int j = 0; j < 3; ++j) u[j] = a[j] + b[j] - 2 * z[i][j];
    const i64 uu = u[0] * u[0] + u[1] * u[1] + u[2] * u[2];
    aa -= (i128)wi * (i128)uu;
    for (int j = 0; j < 3; ++j) p[j] -= (i128)2 * (i128)wi * (i128)z[i][j];
  }
  if (mu != DualMutant::kNoPositivity) {
    const bool positif = (mu == DualMutant::kAcceptEquality) ? (aa >= 0) : (aa > 0);
    if (!positif) return kLaneNone;
  }

  if (mu == DualMutant::kNarrowI64) {
    // MUTANT : `D ||P||^2` atteint 102 bits ; en i64 il enroule et le signe
    // de la comparaison devient arbitraire.
    const i64 pn = (i64)((p[0] * p[0] + p[1] * p[1] + p[2] * p[2]));
    const i64 pd = (i64)(p[0] * d[0] + p[1] * d[1] + p[2] * d[2]);
    const i64 rr = dd * pn - pd * pd;
    const i64 a2 = (i64)aa * (i64)aa;
    if (a2 > 2 * rr) return kLaneQ4;
    if (3 * a2 > 4 * rr) return kLaneQ3;
    return kLaneQ2;
  }

  const i128 pn = p[0] * p[0] + p[1] * p[1] + p[2] * p[2];
  const i128 pd = p[0] * d[0] + p[1] * d[1] + p[2] * d[2];
  const i128 rr = (i128)dd * pn - pd * pd;
  const i128 a2 = aa * aa;
  int c4n = 1, c4d = 2, c3n = 3, c3d = 4;
  if (mu == DualMutant::kSwapCoeff) { c4n = 3; c4d = 4; c3n = 1; c3d = 2; }
  if (mu == DualMutant::kAcceptEquality) {
    if ((i128)c4n * a2 >= (i128)c4d * rr) return kLaneQ4;
    if ((i128)c3n * a2 >= (i128)c3d * rr) return kLaneQ3;
    return kLaneQ2;
  }
  if ((i128)c4n * a2 > (i128)c4d * rr) return kLaneQ4;
  if ((i128)c3n * a2 > (i128)c3d * rr) return kLaneQ3;
  return kLaneQ2;
}

// Cas `k=1`, poids un : il DOIT coincider avec l'ecriture historique `(g,Q)`.
// C'est le juge interne le moins cher et le plus severe du fichier : si les
// deux divergent sur un seul triple, l'une des deux algebres est fausse.
MHGP_HD inline int dual_lane_single(const i64 a[3], const i64 b[3], const i64 zz[3],
                                    DualMutant mu = DualMutant::kNone) {
  i64 z[1][3] = {{zz[0], zz[1], zz[2]}};
  const i64 w[1] = {1};
  return dual_lane(a, b, z, w, 1, mu);
}

}  // namespace jdual
}  // namespace mhgp3v
