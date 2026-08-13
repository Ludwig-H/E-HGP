// MorseHGP3D v3 — GROUPES CONIQUES DE TROIS TEMOINS.
//
// Specification : audits/AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md
// section 4.
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=proposition_math_non_recue,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// POURQUOI UN GROUPE VOIT CE QU'AUCUN POINT NE VOIT
//
// Un temoin PONCTUEL doit etre interieur a TOUTE sphere admissible. Sur une
// famille en amas, l'ancre inter-amas n'a souvent aucun site unique qui tienne
// cette contrainte : le milieu est vide, et chaque site n'est interieur que
// pour une partie du pinceau. Le certificat ponctuel n'y ferme donc rien.
//
// Un GROUPE de trois sites peut pourtant couvrir tout le pinceau, chaque sphere
// etant servie par un membre different. C'est la difference exacte, et c'est
// pourquoi ce certificat vise le mur des amas.
//
// ---------------------------------------------------------------------------
// LE THEOREME
//
// Pour une paire (a,b), poser d = b-a et s_i = z_i-a. Les centres des spheres
// passant par a et b s'ecrivent c = (a+b)/2 + t avec t.d = 0. En prenant `a`
// pour origine, la puissance de z_i vaut
//
//   ||z_i-c||^2 - ||a-c||^2 = ||s_i||^2 - d.s_i - 2 s_i.t,
//
// donc z_i est STRICTEMENT INTERIEUR si et seulement si
//
//   mu_i(t) = d.s_i - ||s_i||^2 + 2 s_i.t > 0.
//
// Supposons (H1) d dans le cone POSITIF de s_1,s_2,s_3, et (H2) d.s_i > ||s_i||^2
// pour chacun des trois. Ecrivons d = somme alpha_i s_i avec alpha_i >= 0 ; alors
// A = somme alpha_i > 0 puisque d != 0, et lambda_i = alpha_i/A donne
// somme lambda_i s_i = d/A. Par consequent
//
//   somme lambda_i mu_i(t) = somme lambda_i (d.s_i - ||s_i||^2) + 2 (d/A).t
//                          = somme lambda_i (d.s_i - ||s_i||^2)  > 0,
//
// car t.d = 0 annule le terme en `t`. UNE combinaison convexe des `mu_i` est
// donc strictement positive pour TOUT centre admissible, donc au moins un
// `mu_i` l'est. Fin de preuve.
//
// Huit groupes DEUX A DEUX DISJOINTS donnent alors huit interieurs distincts
// dans chaque sphere admissible — les identites varient d'une sphere a l'autre,
// mais le COMPTE ne varie pas, et c'est le compte que la lane exige. Neuf
// groupes ferment q3, dix ferment q2.
//
// LA DISJONCTION EST ESSENTIELLE : deux groupes partageant un `PointId`
// pourraient etre servis par ce meme point et ne donner qu'un seul interieur.
//
// ---------------------------------------------------------------------------
// LARGEURS, PROFIL u16
//
//   |d.s_i|, ||s_i||^2  <= 3 * 65535^2 = 1,29e10          (i64)
//   det(s_1,s_2,s_3)    <= 6 * 65535^3 = 1,69e15          (i64)
//   det de Cramer       <= meme borne                      (i64)
//
// Aucun produit ne depasse i64 sur ce domaine. Un triple de rang deficient
// (determinant nul) n'est JAMAIS accepte : il reste fail-open.
#pragma once

#include <cstdint>

#include "mhgp/mhgp.hpp"

namespace mhgp3v {
namespace conic {

using mhgp::i64;
using mhgp::P3;

enum class GroupMutant {
  kNone,
  kLinearSpan,     // remplace le cone positif par le seul espace lineaire
  kAcceptEquality, // accepte d.s_i == ||s_i||^2, ou la puissance est nulle
  kReusePointId,   // autorise deux groupes a partager un PointId
  kAxisOnly,       // ne teste que l'axe de la cellule au lieu du cone
  kSingularOk,     // accepte un triple de determinant nul
};

inline const char* group_mutant_name(GroupMutant m) {
  switch (m) {
    case GroupMutant::kNone: return "none";
    case GroupMutant::kLinearSpan: return "groupe-espace-lineaire";
    case GroupMutant::kAcceptEquality: return "groupe-egalite-puissance";
    case GroupMutant::kReusePointId: return "groupe-pointid-reutilise";
    case GroupMutant::kAxisOnly: return "groupe-axe-seul";
    case GroupMutant::kSingularOk: return "groupe-determinant-nul";
  }
  return "?";
}

inline i64 dot3(const i64 u[3], const i64 v[3]) {
  return u[0] * v[0] + u[1] * v[1] + u[2] * v[2];
}

inline i64 det3(const i64 c0[3], const i64 c1[3], const i64 c2[3]) {
  return c0[0] * (c1[1] * c2[2] - c1[2] * c2[1]) -
         c1[0] * (c0[1] * c2[2] - c0[2] * c2[1]) +
         c2[0] * (c0[1] * c1[2] - c0[2] * c1[1]);
}

// (H2) seule : chaque membre a une puissance strictement negative au centre du
// segment. Elle est necessaire, jamais suffisante.
inline bool member_admissible(const i64 d[3], const i64 s[3], GroupMutant mu) {
  const i64 lhs = dot3(d, s);
  const i64 rhs = dot3(s, s);
  // A l'egalite la puissance est NULLE au centre diametral : le site est sur la
  // sphere, pas dedans. Le mutant l'accepte et fabrique un interieur qui n'en
  // est pas un.
  return (mu == GroupMutant::kAcceptEquality) ? (lhs >= rhs) : (lhs > rhs);
}

// (H1) : `d` appartient au cone POSITIF de (s1,s2,s3). Par Cramer, avec
// D = det(s1,s2,s3) non nul, les coefficients sont alpha_i = D_i / D et la
// condition est que les trois `D_i` aient le signe de `D`.
//
// Le mutant `kLinearSpan` ne teste que l'inversibilite : `d` est alors dans
// l'ESPACE engendre, pas dans le cone, et la combinaison peut avoir des poids
// negatifs — la preuve tombe entierement.
inline bool cone_contains(const i64 s1[3], const i64 s2[3], const i64 s3[3],
                          const i64 d[3], GroupMutant mu) {
  const i64 det = det3(s1, s2, s3);
  if (det == 0) return mu == GroupMutant::kSingularOk;  // fail-open par defaut
  if (mu == GroupMutant::kLinearSpan) return true;
  const i64 d1 = det3(d, s2, s3);
  const i64 d2 = det3(s1, d, s3);
  const i64 d3 = det3(s1, s2, d);
  if (det > 0) return d1 >= 0 && d2 >= 0 && d3 >= 0;
  return d1 <= 0 && d2 <= 0 && d3 <= 0;
}

// Le certificat complet d'un groupe pour l'ancre orientee (a,b).
inline bool group_covers(const i64 d[3], const i64 s1[3], const i64 s2[3], const i64 s3[3],
                         GroupMutant mu = GroupMutant::kNone) {
  if (!member_admissible(d, s1, mu)) return false;
  if (!member_admissible(d, s2, mu)) return false;
  if (!member_admissible(d, s3, mu)) return false;
  if (mu == GroupMutant::kAxisOnly) return true;  // le cone n'est jamais teste
  return cone_contains(s1, s2, s3, d, mu);
}

}  // namespace conic
}  // namespace mhgp3v
