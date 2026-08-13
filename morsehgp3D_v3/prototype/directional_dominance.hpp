// MorseHGP3D v3 — LES 432 SOUS-CONES ENTIERS, ET LE CUTOFF DE HAUTEUR.
//
// Specification : audits/AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md
// sections 2 et 3, et la banque de sous-cones de
// audits/AUDIT_REPONSES_CLAUDE_CHAMBRES_NIVEAUX_CUTTING_20260812.md section 1.2.
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=proposition_math_non_recue,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// LA CONSTRUCTION
//
// Le groupe octaedrique d'ordre 48 ramene toute direction non nulle a la chambre
// canonique x >= y >= z >= 0. Dans cette chambre, les rayons entiers
// v_ij = (3,i,j), 0 <= j <= i <= 3, decoupent neuf triangles geodesiques :
//
//   U_ij = (v_ij, v_{i+1,j}, v_{i+1,j+1})   pour 0 <= i <= 2, 0 <= j <= i   (six)
//   D_ij = (v_ij, v_{i,j+1}, v_{i+1,j+1})   pour 1 <= i <= 2, 0 <= j < i    (trois)
//
// La verification finie des vingt-sept aretes donne min cos^2(gamma) = 9/11 pour
// deux rayons d'un meme sous-cone. Les 48 chambres donnent donc 48*9 = 432
// sous-cones. L'attribution est HALF-OPEN : toute egalite va au plus petit
// identifiant.
//
// ---------------------------------------------------------------------------
// LE CUTOFF DE HAUTEUR — POURQUOI IL SUPPRIME LA BOUCLE SUR LES PAIRES
//
// Poser tau(v) = max(|v_x|,|v_y|,|v_z|). Dans la chambre canonique c'est v_x, et
// ||v||^2 <= 3 tau(v)^2. Si d = b-a et s = z-a sont dans le MEME sous-cone et
//
//     tau(d) >= 3 tau(s),
//
// alors ||s||^2/||d||^2 <= 3 tau(s)^2 / tau(d)^2 <= 1/3 < 9/25, et le cutoff
// radial q4 `25 r^2 <= 9 D^2` de la section 2.2 s'applique : `z` est un temoin
// universel q4 pour l'ancre `ab` prise comme ARETE MAXIMALE.
//
// La consequence est structurelle. Pour un couple (ancre, sous-cone), il suffit
// de connaitre les huitieme, neuvieme et dixieme plus petites hauteurs. Toute
// cible dont la hauteur atteint trois fois le seuil correspondant est fermee
// SANS etre examinee individuellement : la decision porte sur un intervalle de
// hauteurs, pas sur une paire. C'est la difference exacte avec les six
// ordonnances precedentes.
//
// ---------------------------------------------------------------------------
// CE QUE LE CUTOFF NE DIT PAS
//
// Il ferme la CANDIDATURE de `ab` comme arete maximale d'un support d'arite q.
// Il ne dit pas que `ab` ne peut pas etre une arete non maximale d'un autre
// support : ce support sera traite par sa propre arete maximale canonique.
// Une cellule contenant moins de `h` temoins a un seuil INFINI et toutes ses
// cibles restent au residuel.
#pragma once

#include <cstdint>

#include "mhgp/mhgp.hpp"

namespace mhgp3v {
namespace dominance {

using mhgp::i64;
using mhgp::P3;

inline constexpr int kChambers = 48;
inline constexpr int kSubcones = 9;
inline constexpr int kCells = kChambers * kSubcones;  // 432
// Seuils de temoins a smax=11 : dix pour q2, neuf pour q3, huit pour q4.
inline constexpr int kNeed[3] = {10, 9, 8};
inline constexpr int kMaxNeed = 10;

enum class DomMutant {
  kNone,
  kHMinusOne,      // seuil a h-1 temoins : ferme sans en avoir assez
  kTargetAsWitness,  // compte la cible elle-meme comme temoin
  kBoundaryClosed,   // attribution de frontiere fermee au lieu de half-open
  kNeighbourCell,    // emploie le seuil de la cellule voisine
  kFactorTwo,        // remplace le facteur trois du cutoff par deux
};

inline const char* dom_mutant_name(DomMutant m) {
  switch (m) {
    case DomMutant::kNone: return "none";
    case DomMutant::kHMinusOne: return "dom-h-moins-un";
    case DomMutant::kTargetAsWitness: return "dom-cible-temoin";
    case DomMutant::kBoundaryClosed: return "dom-frontiere-fermee";
    case DomMutant::kNeighbourCell: return "dom-cellule-voisine";
    case DomMutant::kFactorTwo: return "dom-facteur-deux";
  }
  return "?";
}

// Hauteur : norme infinie. Dans une chambre fixee c'est une forme LINEAIRE des
// coordonnees, donc tau(b-a) = tau_j(b) - tau_j(a) : c'est ce qui rend la
// selection top-h orthogonale et la fermeture decidable par intervalle.
inline i64 height(i64 x, i64 y, i64 z) {
  const i64 ax = x < 0 ? -x : x;
  const i64 ay = y < 0 ? -y : y;
  const i64 az = z < 0 ? -z : z;
  i64 m = ax;
  if (ay > m) m = ay;
  if (az > m) m = az;
  return m;
}

// Les six permutations, dans l'ordre lexicographique de leurs indices : c'est
// l'ordre canonique qui tranche les ex aequo.
inline constexpr int kPerm[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2},
                                    {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};

// ---------------------------------------------------------------------------
// CLASSIFICATION D'UNE DIRECTION NON NULLE DANS L'UN DES 432 SOUS-CONES.
// Rend -1 pour le vecteur nul, qui releve du preflight de duplicats.
//
// Toutes les decisions sont ENTIERES : aucune division, aucune racine, aucun
// flottant. Les egalites vont au plus petit identifiant.
// ---------------------------------------------------------------------------
inline int cell_of(i64 sx, i64 sy, i64 sz, DomMutant mu = DomMutant::kNone) {
  const i64 s[3] = {sx, sy, sz};
  i64 a[3];
  for (int k = 0; k < 3; ++k) a[k] = s[k] < 0 ? -s[k] : s[k];
  if (a[0] == 0 && a[1] == 0 && a[2] == 0) return -1;

  // Permutation qui trie les magnitudes en ordre DECROISSANT, ex aequo tranches
  // par l'indice croissant. Le mutant de frontiere inverse ce tie-break, ce qui
  // envoie les directions de bord dans la cellule voisine.
  int pi = -1;
  for (int p = 0; p < 6; ++p) {
    const int* q = kPerm[p];
    const bool ordered = (mu == DomMutant::kBoundaryClosed)
                             ? (a[q[0]] >= a[q[1]] && a[q[1]] >= a[q[2]])
                             : (a[q[0]] > a[q[1]] ||
                                (a[q[0]] == a[q[1]] && q[0] < q[1])) &&
                                   (a[q[1]] > a[q[2]] ||
                                    (a[q[1]] == a[q[2]] && q[1] < q[2]));
    if (ordered) { pi = p; break; }
  }
  if (pi < 0) pi = 0;
  const int* q = kPerm[pi];
  const i64 w0 = a[q[0]], w1 = a[q[1]], w2 = a[q[2]];

  // Signes : bit k = 1 lorsque la composante d'origine `k` est strictement
  // negative. Le zero est positif, ce qui ferme le demi-espace du bas.
  int sgn = 0;
  for (int k = 0; k < 3; ++k)
    if (s[k] < 0) sgn |= (1 << k);
  const int chamber = pi * 8 + sgn;

  // Sous-cone : i = floor(3 w1 / w0), j = floor(3 w2 / w0), sans division.
  int i = 0, j = 0;
  while (i < 3 && (i64)(i + 1) * w0 <= 3 * w1) ++i;
  while (j < 3 && (i64)(j + 1) * w0 <= 3 * w2) ++j;
  if (i > 2) i = 2;
  if (j > i) j = i;
  if (j > 2) j = 2;

  // Diagonale du petit carre : U si 3(w1-w2) >= (i-j) w0, sinon D. L'egalite va
  // a U, qui porte le plus petit identifiant.
  const bool up = (3 * (w1 - w2) >= (i64)(i - j) * w0);
  static constexpr int kBase[3][3] = {{0, -1, -1}, {1, 3, -1}, {4, 6, 8}};
  int sub = kBase[i][j];
  if (sub < 0) sub = 0;
  if (j < i && !up) sub += 1;  // la variante D suit immediatement son U
  return chamber * kSubcones + sub;
}

inline int cell_of(const P3& from, const P3& to, DomMutant mu = DomMutant::kNone) {
  return cell_of((i64)to.x - (i64)from.x, (i64)to.y - (i64)from.y, (i64)to.z - (i64)from.z, mu);
}

// ---------------------------------------------------------------------------
// LE CUTOFF — ET POURQUOI LE FACTEUR TROIS EST TROP GROS DANS HUIT CAS SUR NEUF
//
// La forme publiee de la section 2.4 est `tau(d) >= 3 tau(s)`. Son facteur trois
// vient de `||v||^2 <= 3 tau(v)^2`, qui est l'inegalite de la CHAMBRE canonique
// `x >= y >= z >= 0`, saturee seulement sur la diagonale `x=y=z`. Or les
// sous-cones sont BIEN plus etroits qu'une chambre.
//
// Le cutoff radial exact de la section 2.2 est `25 r^2 <= 9 D^2` en q4, et celui
// de la section 2.3 est `64 r^2 <= 25 D^2` en q3. Pour un sous-cone engendre par
// les rayons `v = (3,i,j)`, la fonction convexe `||v||^2 / v_x^2` atteint son
// maximum sur un SOMMET du triangle, donc
//
//     ||s||^2 <= kappa_j^2 tau(s)^2,   kappa_j^2 = max_sommets (9+i^2+j^2)/9,
//
// tandis que `||d||^2 >= tau(d)^2` pour tout vecteur. Il suffit donc de
//
//     q4 :  25 (9+i^2+j^2) tau(s)^2 <= 81 tau(d)^2,
//     q3 :  64 (9+i^2+j^2) tau(s)^2 <= 225 tau(d)^2.
//
// Tout est entier et la constante ne depend que du sous-cone, pas de la chambre.
// Sur le sous-cone axial `U_00`, de sommets (3,0,0),(3,1,0),(3,1,1), le maximum
// vaut 11 et le seuil devient `tau(d) >= 1,843 tau(s)` au lieu de `3` : c'est un
// facteur 1,63 en rayon, donc environ 4,3 en volume de cibles certifiees. Sur le
// sous-cone le plus large, `U_22`, le maximum vaut 27 et le seuil reste 2,887 —
// a peine mieux que 3. Le gain est donc tres inegal, et c'est precisement
// pourquoi une constante universelle est le mauvais choix.
//
// La q3 est en outre PLUS PERMISSIVE que la q4 : son cutoff radial autorise
// `r/D <= 5/8` contre `3/5`. Les employer separement est exact ; employer le
// seul facteur trois pour les trois lanes ne l'etait pas faux, seulement
// inutilement pauvre.
// ---------------------------------------------------------------------------

// `9 + i^2 + j^2` maximal sur les trois sommets, dans l'ordre des neuf
// identifiants de sous-cone. Sommets : U_ij = (v_ij, v_{i+1,j}, v_{i+1,j+1}),
// D_ij = (v_ij, v_{i,j+1}, v_{i+1,j+1}).
//   0 : U00 (3,0,0)(3,1,0)(3,1,1)  -> max(9,10,11)   = 11
//   1 : U10 (3,1,0)(3,2,0)(3,2,1)  -> max(10,13,14)  = 14
//   2 : D10 (3,1,0)(3,1,1)(3,2,1)  -> max(10,11,14)  = 14
//   3 : U11 (3,1,1)(3,2,1)(3,2,2)  -> max(11,14,17)  = 17
//   4 : U20 (3,2,0)(3,3,0)(3,3,1)  -> max(13,18,19)  = 19
//   5 : D20 (3,2,0)(3,2,1)(3,3,1)  -> max(13,14,19)  = 19
//   6 : U21 (3,2,1)(3,3,1)(3,3,2)  -> max(14,19,22)  = 22
//   7 : D21 (3,2,1)(3,2,2)(3,3,2)  -> max(14,17,22)  = 22
//   8 : U22 (3,2,2)(3,3,2)(3,3,3)  -> max(17,22,27)  = 27
inline constexpr int kKappaNum[kSubcones] = {11, 14, 14, 17, 19, 19, 22, 22, 27};

// Lane 0 = q2 et lane 2 = q4 emploient le cutoff q4, plus fort ; lane 1 = q3
// emploie le sien, plus permissif. Les seuils de temoins restent 10/9/8.
// Marge du certificat, en unites de son propre test. Elle vaut zero exactement
// a la frontiere. Le controle echantillonne tire les fermetures de PLUS PETITE
// marge : ce sont les seules ou un mutant peut differer, et un tirage uniforme
// sur des dizaines de millions de fermetures ne les rencontre jamais.
inline mhgp::i128 cutoff_margin(int cell, int lane, i64 tau_d, i64 tau_h) {
  if (tau_h < 0 || cell < 0) return -1;
  const int kappa = kKappaNum[cell % kSubcones];
  const mhgp::i128 lhs = (mhgp::i128)tau_d * (mhgp::i128)tau_d;
  const mhgp::i128 rhs = (mhgp::i128)tau_h * (mhgp::i128)tau_h * (mhgp::i128)kappa;
  return (lane == 1) ? (225 * lhs - 64 * rhs) : (81 * lhs - 25 * rhs);
}

inline bool cutoff_closes(int cell, int lane, i64 tau_d, i64 tau_h,
                          DomMutant mu = DomMutant::kNone) {
  if (tau_h < 0 || cell < 0) return false;  // cellule sous-pleine : seuil infini
  const int kappa = kKappaNum[cell % kSubcones];
  // MUTANT : le facteur universel deux, qui ne derive d'aucun cutoff radial.
  if (mu == DomMutant::kFactorTwo) return tau_d >= 2 * tau_h;
  const mhgp::i128 lhs = (mhgp::i128)tau_d * (mhgp::i128)tau_d;
  const mhgp::i128 rhs = (mhgp::i128)tau_h * (mhgp::i128)tau_h * (mhgp::i128)kappa;
  return (lane == 1) ? (225 * lhs >= 64 * rhs) : (81 * lhs >= 25 * rhs);
}

}  // namespace dominance
}  // namespace mhgp3v
