// MorseHGP3D v3 — CREDITS CELLULAIRES : LE GROUPE SANS ENUMERATION DE TRIPLES.
//
// Specification : audits/AUDIT_REPONSE_GATE_TROIS_VOIES_20260813.md, section
// « Deblocage mathematique prioritaire : credits cellulaires sans triples ».
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=proposition_math_non_recue,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// POURQUOI LES TRIPLES ETAIENT LE MAUVAIS OBJET
//
// Le theoreme de groupe vaut pour un ensemble fini `G` de taille QUELCONQUE :
// si la direction cible appartient a son cone positif et si chaque membre
// satisfait strictement la puissance (H2), alors `G` fournit un interieur a
// toute sphere admissible. Caratheodory borne a trois un sous-groupe pour une
// direction FIXEE ; il n'impose nullement d'enumerer ces sous-groupes pour
// couvrir une cellule entiere. Mon premier sujet formait `C(m,3)` triples : il
// payait une combinatoire que la geometrie ne demande pas.
//
// ---------------------------------------------------------------------------
// L'EVENEMENT D'ACTIVATION, EXACT ET ENTIER
//
// Soit une cellule simpliciale `C = cone(r_0,r_1,r_2)` dont les trois rayons
// sont sur la section de hauteur `T`. Pour un site relatif `s = z-a`, poser
//
//     m_C(s) = min_{0<=j<3} r_j . s.
//
// Toute direction `d` de `C` s'ecrit `d = somme beta_j r_j` avec `beta_j >= 0`,
// et la hauteur etant LINEAIRE sur la chambre, `tau(d) = T somme beta_j`. Donc
//
//     d . s = somme beta_j (r_j . s) >= (somme beta_j) m_C(s) = tau(d) m_C(s)/T.
//
// Si `m_C(s) > 0`, la condition (H2) `d.s > ||s||^2` est donc acquise sur TOUT
// le suffixe de hauteur des que
//
//     tau(d) m_C(s) > T ||s||^2,
//
// c'est-a-dire des que `tau(d) >= X_s = floor(T ||s||^2 / m_C(s)) + 1`.
//
// `X_s` est l'evenement d'activation du site pour la cellule. Il est exact,
// entier, et ne depend pas de la cible : c'est ce qui rend la decision
// DECIDABLE PAR INTERVALLE DE HAUTEUR, comme le cutoff de dominance.
//
// ---------------------------------------------------------------------------
// LE CREDIT
//
// Un credit de la cellule `C` est un sous-ensemble `G` du pool actif tel que
// `C` soit incluse dans `cone(G)`, ce qui equivaut a ce que les TROIS rayons
// `r_j` y appartiennent. Son seuil est `X_G = max_{s in G} X_s`. Pour toute
// cible `b` de `C` avec `tau(b-a) >= X_G`, les deux hypotheses du theoreme sont
// reunies, donc `G` fournit un interieur.
//
// Des credits DEUX A DEUX DISJOINTS donnent des interieurs distincts. Les
// trier par `X_G` croissant et lire le `h`-ieme donne le seuil de fermeture de
// la lane qui exige `h` temoins. Un echec du glouton reste `UNKNOWN` : aucun
// packing maximal n'est requis pour la surete.
//
// ---------------------------------------------------------------------------
// AUCUNE DIVISION DANS L'AUTORITE
//
// `r_j` appartient a `cone(G)` si et seulement si, dans la carte projective
// `w.u = 1` avec `w = r_0+r_1+r_2`, le projete de `r_j` est dans l'enveloppe
// convexe des projetes de `G`. Les denominateurs `w.s` y sont strictement
// positifs sur le pool actif, donc toute orientation projective a le SIGNE du
// determinant `det(s_i, s_j, s_k)`. L'enveloppe se calcule donc par marche de
// Jarvis avec ce seul predicat entier.
//
// Largeurs u16 : `|r_j| <= 3*sqrt(3) < 6`, `|s| <= 65535*sqrt(3)`, donc
// `|r_j . s| <= 3*3*65535 = 5,9e5`, `T ||s||^2 <= 3*3*65535^2 = 3,9e10`, et
// `det(s_i,s_j,s_k) <= 6*65535^3 = 1,7e15`. Tout tient dans `i64`.
#pragma once

#include <cstdint>

#include "mhgp/mhgp.hpp"
#include "prototype/directional_dominance.hpp"

namespace mhgp3v {
namespace credits {

using mhgp::i64;
using mhgp::P3;

inline constexpr int kRayHeight = 3;  // les rayons (3,i,j) sont a hauteur trois

enum class CreditMutant {
  kNone,
  kActivationOff,    // oublie le `+1` : la frontiere d'activation entre a tort
  kOneRayOnly,       // ne fait passer qu'un rayon au lieu des trois
  kShareIds,         // autorise deux credits a partager un `PointId`
  kIgnorePositive,   // n'exige pas `m_C(s) > 0`
};

inline const char* credit_mutant_name(CreditMutant m) {
  switch (m) {
    case CreditMutant::kNone: return "none";
    case CreditMutant::kActivationOff: return "credit-activation-frontiere";
    case CreditMutant::kOneRayOnly: return "credit-un-seul-rayon";
    case CreditMutant::kShareIds: return "credit-ids-partages";
    case CreditMutant::kIgnorePositive: return "credit-sans-positivite";
  }
  return "?";
}

// Les trois rayons canoniques de chaque sous-cone, dans l'ordre des neuf
// identifiants. Ce sont les sommets des triangles `U_ij` et `D_ij`.
inline constexpr int kCanonicalRays[dominance::kSubcones][3][3] = {
    {{3, 0, 0}, {3, 1, 0}, {3, 1, 1}},  // U00
    {{3, 1, 0}, {3, 2, 0}, {3, 2, 1}},  // U10
    {{3, 1, 0}, {3, 1, 1}, {3, 2, 1}},  // D10
    {{3, 1, 1}, {3, 2, 1}, {3, 2, 2}},  // U11
    {{3, 2, 0}, {3, 3, 0}, {3, 3, 1}},  // U20
    {{3, 2, 0}, {3, 2, 1}, {3, 3, 1}},  // D20
    {{3, 2, 1}, {3, 3, 1}, {3, 3, 2}},  // U21
    {{3, 2, 1}, {3, 2, 2}, {3, 3, 2}},  // D21
    {{3, 2, 2}, {3, 3, 2}, {3, 3, 3}},  // U22
};

// Ramene les trois rayons d'une cellule dans le repere d'origine. La chambre
// vaut `pi*8 + sgn` : `pi` indexe la permutation qui trie les magnitudes en
// ordre decroissant, `sgn` porte un bit par composante negative. Un vecteur
// canonique `v` se relit donc `orig[q[k]] = +/- v[k]`.
inline void cell_rays(int cell, i64 out[3][3]) {
  const int chamber = cell / dominance::kSubcones;
  const int sub = cell % dominance::kSubcones;
  const int pi = chamber / 8, sgn = chamber % 8;
  const int* q = dominance::kPerm[pi];
  for (int j = 0; j < 3; ++j)
    for (int k = 0; k < 3; ++k) {
      const i64 v = kCanonicalRays[sub][j][k];
      out[j][q[k]] = ((sgn >> q[k]) & 1) ? -v : v;
    }
}

inline i64 dot3(const i64 a[3], const i64 b[3]) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline i64 det3(const i64 a[3], const i64 b[3], const i64 c[3]) {
  return a[0] * (b[1] * c[2] - b[2] * c[1]) - b[0] * (a[1] * c[2] - a[2] * c[1]) +
         c[0] * (a[1] * b[2] - a[2] * b[1]);
}

// `m_C(s)` : le minimum des trois produits scalaires avec les rayons.
inline i64 cell_margin(const i64 rays[3][3], const i64 s[3]) {
  i64 m = dot3(rays[0], s);
  for (int j = 1; j < 3; ++j) {
    const i64 v = dot3(rays[j], s);
    if (v < m) m = v;
  }
  return m;
}

// EVENEMENT D'ACTIVATION EXACT. Rend `-1` lorsque le site ne s'active jamais.
// Le mutant oublie le `+1` : a `tau(d) = floor(T||s||^2/m)` l'inegalite (H2)
// est une EGALITE au mieux, donc la puissance peut etre nulle et le site sur la
// sphere plutot que dedans.
inline i64 activation_height(const i64 rays[3][3], const i64 s[3],
                             CreditMutant mu = CreditMutant::kNone) {
  const i64 m = cell_margin(rays, s);
  if (m <= 0) return (mu == CreditMutant::kIgnorePositive) ? 1 : -1;
  const i64 num = (i64)kRayHeight * dot3(s, s);
  const i64 q = num / m;
  return (mu == CreditMutant::kActivationOff) ? q : q + 1;
}

}  // namespace credits
}  // namespace mhgp3v
