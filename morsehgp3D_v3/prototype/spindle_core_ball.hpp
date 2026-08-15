// MorseHGP3D v3 — LE CŒUR D'UN RECTANGLE COMME BOULE INSCRITE, EN FORME CLOSE.
//
// Specification : audits/NOTE_CLAUDE_COEUR_BOULE_FORME_CLOSE_20260815.md.
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=diagnostic_counter_only,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// POURQUOI UNE BOULE PLUTOT QU'UNE BOITE
//
// `soc64_rect.hpp` et `combined_prefilter_probe.cpp` relaxent les nœuds par
// leurs AABB, puis testent UN SITE A LA FOIS. C'est le mauvais probleme : la
// question n'est pas « ce site est-il temoin » mais « COMBIEN de temoins », et
// un comptage dans une region est ce qu'un index spatial sait faire sans
// enumerer. Ce fichier rend la region.
//
// Les deux relaxations sont INCOMPARABLES — une AABB n'est incluse ni dans sa
// boule englobante ni l'inverse — donc ceci ne remplace pas `corner64` : ca le
// complete. La boule elague et credite en bloc, `corner64` decide les feuilles
// de frontiere.
//
// ---------------------------------------------------------------------------
// LES TROIS FUSEAUX SONT DES LIEUX ANGULAIRES
//
// Avec e = z-a, t = b-z, H = e.t, Xi = |e x t|^2 = |e|^2 |t|^2 - H^2 :
//
//   W_2 : H > 0        <=> cos(e,t) > 0        <=> angle(e,t) < 90°
//   W_3 : 3H^2 > Xi    <=> 4H^2 > |e|^2 |t|^2  <=> cos(e,t) > 1/2
//   W_4 : 2H^2 > Xi    <=> 3H^2 > |e|^2 |t|^2  <=> cos(e,t) > 1/sqrt(3)
//
// L'angle du triangle EN z vaut 180° - angle(e,t). Donc
// W_q(a,b) = { z : angle(a,z,b) > theta_q } avec theta = 90°, 120°, 125,264°.
//
// ---------------------------------------------------------------------------
// LA BOULE INSCRITE EST TANGENTE, DONC OPTIMALE
//
// Soit L = |ab| et m le milieu de [a,b]. Un point a distance perpendiculaire
// `rho` de m voit [a,b] sous 2 arctan((L/2)/rho) ; egaler a theta_q donne le
// rayon equatorial du fuseau, rho = (L/2) cot(theta_q/2) = kappa_q L :
//
//   kappa_2 = 1/2                          = 0,500000
//   kappa_3 = 1/(2 sqrt(3)) = sqrt(3)/6    = 0,288675
//   kappa_4 = (sqrt(6)-sqrt(2))/4 = sin15° = 0,258819
//
// La forme q4 se verifie directement sur l'axe equatorial : avec a=(0,0,0),
// b=(L,0,0), z=(L/2,rho,0), la condition `3H^2 > |e|^2|t|^2` se reduit a
// `4 rho^2 < L^2 (2 - sqrt(3))`, donc kappa_4 = sqrt(2-sqrt(3))/2, qui vaut
// sin15° apres rationalisation. Meme calcul en q3 : `12 rho^2 < L^2`.
//
// B(m, kappa_q L) est incluse dans W_q(a,b) et TANGENTE interieurement : dans
// le plan meridien la lentille est D((0,k),R) inter D((0,-k),R) avec
// R = L/(2 sin theta_q) et k = sqrt(R^2 - L^2/4) ; un point a distance
// kappa_q L de l'origine est a distance au plus kappa_q L + k du centre
// (0,-k), et ce majorant vaut EXACTEMENT R. L'inclusion passe a la rotation,
// les deux solides etant de revolution autour de (ab). Aucun rayon plus grand
// ne convient : la fixture `tangence` du probe l'exige des DEUX cotes.
//
// ---------------------------------------------------------------------------
// LE CŒUR D'UN RECTANGLE, EN UNE SOUSTRACTION
//
// Pour A de boule englobante (c_A, r_A), B de (c_B, r_B), d = |c_B - c_A| :
// le milieu m_ab s'ecarte de m = (c_A+c_B)/2 d'au plus (r_A+r_B)/2, et
// |ab| >= d - r_A - r_B. D'ou
//
//   B(m, R_q) incluse dans W_q(a,b) pour TOUT (a,b) de A x B,
//   R_q = kappa_q (d - r_A - r_B) - (r_A + r_B)/2.
//
// Aucun coin, aucune enumeration, aucun produit de coordonnees : R_q ne
// combine que des distances.
//
// ---------------------------------------------------------------------------
// UNITES ENTIERES, ET POURQUOI LE QUADRUPLE
//
// `Sphere` du probe porte des coordonnees DOUBLEES : c2 = lo + hi = 2c, et
// r2 = 2r majore. En posant d2 = |c2_B - c2_A| = 2d, S2 = r2_A + r2_B = 2S et
// L2 = d2 - S2 = 2(d - r_A - r_B), il vient
//
//   R4 := 4 R_q = 2 kappa_q L2 - S2,
//
// et le centre en coordonnees QUADRUPLEES est l'entier c2_A + c2_B. Un point
// `z` du nuage y devient `4z`. Tout est entier, rien n'est divise.
//
// `2 kappa_q` vaut 1 en q2 — exact — et est IRRATIONNEL en q3 et q4. On le
// minore par un rationnel, et le plancher de la division entiere minore encore :
// R4 est donc toujours SOUS-ESTIME, ce qui garde la boule a l'interieur du
// fuseau. Une sur-estimation, elle, certifierait de faux temoins.
//
// LARGEURS. Coordonnees dans [0,65535] : |c2| <= 131070, le centre quadruple
// tient dans [0, 262140], une difference dans [-524280, 524280], son carre sous
// 2,8e11 et la somme des trois sous 8,3e11 — i64 suffirait. `i128` est utilise
// partout par uniformite avec le reste du dossier, et parce que le numerateur
// rationnel multiplie L2 avant division : 577350 * 262140 = 1,5e11, largement
// sous i64 aussi. Rien ici ne peut deborder.
#pragma once

#include <cstdint>

#include "mhgp/mhgp.hpp"

namespace mhgp3v {
namespace corebl {

using mhgp::i128;
using mhgp::i64;

// ---------------------------------------------------------------------------
// MUTANTS. Chacun casse UNE decision, dans le predicat et non dans le probe.
// ---------------------------------------------------------------------------
enum class BallMutant {
  kNone,
  kKappaQ3Sur,     // remplace 1/sqrt(3) par 3/5 : SUR-estime, la boule deborde
  kKappaQ4Sur,     // remplace (sqrt6-sqrt2)/2 par 21/40 : idem en q4
  kOublieDerive,   // oublie le terme -S2 : le deplacement du milieu ignore
  // ARRONDIR VERS LE HAUT D'UNE UNITE N'EST PAS UN MUTANT, ET C'EST UN
  // RESULTAT. Avec le plancher `f = floor(vrai)` et l'inegalite STRICTE, les
  // distances admises sont `far <= f - 1`. Avec `ceil = f + 1` elles deviennent
  // `far <= f`, et `f <= vrai` : tout point admis reste dans le fuseau. La
  // faute est donc absorbee, et l'ecrire comme mutant aurait produit une porte
  // vacue — exactement le defaut que le contre-audit du 15 aout a trouve
  // ailleurs. Il faut deux unites pour sortir, d'ou le mutant ci-dessous.
  kRayonPlusDeux,  // sur-estime le rayon de deux unites quadruplees : le
                   // premier entier strictement au-dela du vrai rayon devient
                   // admissible, et la fixture `tangence` le refute
  kInclusLarge,    // « boite dans la boule » avec <= sur le carre du rayon
                   // alors que le fuseau est OUVERT : la frontiere entre
  // « BOITE HORS DE LA BOULE » AVEC `>=` N'EST PAS UN MUTANT NON PLUS, ET
  // C'EST LE MEME GENRE DE RESULTAT. Une boite dont le point le plus proche est
  // a distance EXACTEMENT R4 a tous ses points a distance >= R4 ; or
  // `ball_contains_point` exige `< R4`. Aucun de ses points ne serait compte,
  // donc l'elaguer ne perd rien : les deux ecritures decident pareil. Le
  // mesurer a coute une porte qui ne mordait sur aucun nuage, et la vraie faute
  // du primitif d'elagage est ailleurs — elle est ci-dessous.
  kDisjointCentre, // elague sur la distance au CENTRE de la boite au lieu de son
                   // point le plus proche : une boite a cheval sur la sphere,
                   // centre au-dela, est coupee avec ses temoins dedans
};

inline const char* ball_mutant_name(BallMutant m) {
  switch (m) {
    case BallMutant::kNone: return "none";
    case BallMutant::kKappaQ3Sur: return "boule-kappa3-sur";
    case BallMutant::kKappaQ4Sur: return "boule-kappa4-sur";
    case BallMutant::kOublieDerive: return "boule-oublie-derive";
    case BallMutant::kRayonPlusDeux: return "boule-rayon-plus-deux";
    case BallMutant::kInclusLarge: return "boule-inclus-large";
    case BallMutant::kDisjointCentre: return "boule-disjoint-centre";
  }
  return "?";
}

// ---------------------------------------------------------------------------
// `2 kappa_q`, MINORE PAR UN RATIONNEL.
//
//   q2 : 1                exact
//   q3 : 1/sqrt(3)      = 0,5773502692...  minore par 577350/1000000
//   q4 : (sqrt6-sqrt2)/2 = 0,5176380902...  minore par 517638/1000000
//
// Les deux troncatures sont strictement inferieures a la valeur reelle : la
// sixieme decimale de q3 est un 2 tronque, celle de q4 un 0 suivi d'un 9.
// ---------------------------------------------------------------------------
struct Kappa2 { i64 num, den; };

MHGP_HD inline Kappa2 deux_kappa(int q, BallMutant mu = BallMutant::kNone) {
  if (q == 2) return Kappa2{1, 1};
  if (q == 3) {
    if (mu == BallMutant::kKappaQ3Sur) return Kappa2{3, 5};  // 0,6 > 0,57735
    return Kappa2{577350, 1000000};
  }
  if (mu == BallMutant::kKappaQ4Sur) return Kappa2{21, 40};  // 0,525 > 0,51764
  return Kappa2{517638, 1000000};
}

// ---------------------------------------------------------------------------
// RAYON DU CŒUR, EN UNITES QUADRUPLEES. Rend `R4 = 2 kappa_q L2 - S2`, ou une
// valeur <= 0 quand le cœur est vide — ce qui n'est pas une erreur mais le
// regime normal des rectangles insuffisamment separes.
//
// `d2` doit etre un MINORANT entier de la distance doublee des centres, `rA2`
// et `rB2` des MAJORANTS des rayons doubles : c'est ce que produisent
// `isqrt_floor` et `sphere_of` du probe. Les trois conservatismes vont dans le
// meme sens.
// ---------------------------------------------------------------------------
MHGP_HD inline i64 core_ball_radius4(i64 d2, i64 rA2, i64 rB2, int q,
                                     BallMutant mu = BallMutant::kNone) {
  const i64 S2 = rA2 + rB2;
  const i64 L2 = d2 - S2;
  if (L2 <= 0) return 0;
  const Kappa2 k = deux_kappa(q, mu);
  const i128 prod = (i128)k.num * (i128)L2;
  i64 terme = (i64)(prod / k.den);  // plancher : minore encore, donc sur
  // MUTANT : deux unites suffisent a franchir le vrai rayon, une seule non.
  if (mu == BallMutant::kRayonPlusDeux) terme += 2;
  if (mu == BallMutant::kOublieDerive) return terme;  // MUTANT : derive ignoree
  return terme - S2;
}

// ---------------------------------------------------------------------------
// LES DEUX PRIMITIVES SPHERE-BOITE, EXACTES EN ENTIERS.
//
// `M` est le centre en coordonnees QUADRUPLEES (c2_A + c2_B), `R4` le rayon
// quadruple. La boite est donnee dans les coordonnees d'origine du nuage : elle
// est multipliee par quatre ici, ce qui est exact.
//
// C'est ce couple qui remplace la descente par predicat : un sous-arbre
// entierement dans la boule est credite par sa POPULATION, en O(1), et un
// sous-arbre disjoint est coupe. Le coût suit la SURFACE de la sphere dans
// l'arbre, pas son volume.
// ---------------------------------------------------------------------------
MHGP_HD inline bool ball_contains_box(const i64 M[3], i64 R4,
                                      const i64 lo[3], const i64 hi[3],
                                      BallMutant mu = BallMutant::kNone) {
  if (R4 <= 0) return false;
  i128 far = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 a = 4 * lo[i] - M[i];
    const i64 b = 4 * hi[i] - M[i];
    const i64 na = a < 0 ? -a : a;
    const i64 nb = b < 0 ? -b : b;
    const i64 w = na > nb ? na : nb;
    far += (i128)w * (i128)w;
  }
  const i128 r2 = (i128)R4 * (i128)R4;
  // Le fuseau est OUVERT et la boule lui est TANGENTE : un point a distance
  // exactement `R4` est sur la frontiere, donc hors du fuseau. L'inegalite est
  // stricte, et le mutant `kInclusLarge` la relache.
  return (mu == BallMutant::kInclusLarge) ? (far <= r2) : (far < r2);
}

MHGP_HD inline bool ball_disjoint_box(const i64 M[3], i64 R4,
                                      const i64 lo[3], const i64 hi[3],
                                      BallMutant mu = BallMutant::kNone) {
  if (R4 <= 0) return true;
  i128 near = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 a = 4 * lo[i], b = 4 * hi[i];
    i64 g = 0;
    if (mu == BallMutant::kDisjointCentre) {
      // MUTANT : distance au centre de la boite. Elle MAJORE la distance au
      // point le plus proche, donc elague des boites encore utiles.
      const i64 c = (a + b) / 2;
      g = M[i] > c ? M[i] - c : c - M[i];
    } else if (M[i] < a) {
      g = a - M[i];
    } else if (M[i] > b) {
      g = M[i] - b;
    }
    near += (i128)g * (i128)g;
  }
  const i128 r2 = (i128)R4 * (i128)R4;
  return near > r2;
}

MHGP_HD inline bool ball_contains_point(const i64 M[3], i64 R4, const i64 p[3],
                                        BallMutant mu = BallMutant::kNone) {
  const i64 lo[3] = {p[0], p[1], p[2]};
  return ball_contains_box(M, R4, lo, lo, mu);
}

}  // namespace corebl
}  // namespace mhgp3v
