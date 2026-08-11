// MorseHGP3D v3 — LE PRUNE CONVEXE CERTIFIE DES CELLULES DE CENTRES
// (reponse pont/q4 du 11 aout, §6).
//
// CONDITION NECESSAIRE : tout support propre de la branche beta < Q_{q,C}
// possede par la cellule C est inclus dans A_{q,C}, et son centre appartient
// a conv(A_{q,C}). Si closure(C) et conv(A_{q,C}) sont STRICTEMENT separes,
// la cellule ne possede aucun support de cette branche : sa masse
// combinadique vaut exactement zero pour le quotient — etiquette « branche
// beta<Q vide », l'autre branche etant normalized_h0_inert, JAMAIS
// « no_support ».
//
// FAIL-CLOSED : une proposition de plan peut venir de n'importe ou (axes,
// direction du barycentre, flottant, GJK) — seule DECIDE la verification
// entiere exacte : tous les points de A_C STRICTEMENT d'un cote du plan,
// les huit coins FERMES de C de l'autre. Un simple contact n'est pas une
// separation (le centre d'un vrai support peut etre SUR la face). Sans
// certificat, la cellule est conservee.
//
// Largeurs : direction du barycentre nu = 2*somme(A) - 2*m*centre_box, avec
// m <= 100000 et coordonnees < 2^17 : |nu| < 2^35, produits scalaires
// < 3*2^52 — tout tient en i64 ; l'accumulation du barycentre en i64 aussi.
#pragma once

#include <cstdint>
#include <vector>

#include "mhgp/mhgp.hpp"

namespace mhgp3v {

struct CellPruneMutants {
  bool contact_as_separation = false;   // >= au lieu de > : un contact prune a
                                        // tort un centre de frontiere
  bool prune_from_cloud = false;        // teste conv(X) au lieu de conv(A_C) :
                                        // perd le prune des pics lointains
};

// Le verdict et sa provenance (axe ou plan general), pour les recus.
struct CellPruneVerdict {
  bool separated = false;
  bool by_axis = false;
  mhgp::i64 direction[3] = {0, 0, 0};   // la normale entiere certifiee
};

// La VERIFICATION exacte d'une direction candidate nu (entiere) : rend vrai
// ssi min_{x in A} <nu,x> depasse STRICTEMENT max_{coin y} <nu,y>. Les coins
// fermes sont enumerables par les extremes par axe : max <nu,y> sur la boite
// = somme par axe de max(nu_d*lo_d, nu_d*hi_d).
template <class PointAt>
inline bool cell_prune_certify(const mhgp::i64* box_lo, const mhgp::i64* box_hi,
                               const PointAt& point_at, std::size_t count,
                               const mhgp::i64* direction, bool contact_mutant) {
  if (direction[0] == 0 && direction[1] == 0 && direction[2] == 0) return false;
  mhgp::i64 corner_max = 0;
  for (int d = 0; d < 3; ++d) {
    const mhgp::i64 at_lo = direction[d] * box_lo[d];
    const mhgp::i64 at_hi = direction[d] * box_hi[d];
    corner_max += at_lo > at_hi ? at_lo : at_hi;
  }
  mhgp::i64 points_min = 0;
  bool first = true;
  for (std::size_t i = 0; i < count; ++i) {
    const mhgp::P3 p = point_at(i);
    const mhgp::i64 dot = direction[0] * (mhgp::i64)p.x + direction[1] * (mhgp::i64)p.y +
                          direction[2] * (mhgp::i64)p.z;
    if (first || dot < points_min) points_min = dot;
    first = false;
  }
  if (first) return false;   // A vide : aucun certificat (jamais atteint, m >= t_q)
  return contact_mutant ? points_min >= corner_max : points_min > corner_max;
}

// LE PRUNE COMPLET : essaie les six directions d'axe (le v1 deja recu a
// 50 k), puis la direction du barycentre de A_C — la proposition naturelle
// pour une nappe sous ou sur la cellule, y compris inclinee. Chaque
// candidate passe par la MEME verification exacte ; sans certificat, la
// cellule est conservee.
template <class PointAt>
inline CellPruneVerdict cell_prune(const mhgp::i64* box_lo, const mhgp::i64* box_hi,
                                   const PointAt& point_at, std::size_t count,
                                   CellPruneMutants mutants = {}) {
  CellPruneVerdict verdict;
  if (count == 0) return verdict;
  // 1. LES AXES : nu = +/- e_d.
  for (int d = 0; d < 3 && !verdict.separated; ++d) {
    for (int sign = -1; sign <= 1 && !verdict.separated; sign += 2) {
      mhgp::i64 direction[3] = {0, 0, 0};
      direction[d] = sign;
      if (cell_prune_certify(box_lo, box_hi, point_at, count, direction,
                             mutants.contact_as_separation)) {
        verdict.separated = true;
        verdict.by_axis = true;
        for (int a = 0; a < 3; ++a) verdict.direction[a] = direction[a];
      }
    }
  }
  if (verdict.separated) return verdict;
  // 2. LE BARYCENTRE : nu = 2*somme(A) - 2*m*centre_box, pointant de la boite
  // vers le nuage local — exact, sans division.
  mhgp::i64 sum[3] = {0, 0, 0};
  for (std::size_t i = 0; i < count; ++i) {
    const mhgp::P3 p = point_at(i);
    sum[0] += (mhgp::i64)p.x;
    sum[1] += (mhgp::i64)p.y;
    sum[2] += (mhgp::i64)p.z;
  }
  mhgp::i64 direction[3];
  for (int d = 0; d < 3; ++d)
    direction[d] = 2 * sum[d] - (mhgp::i64)count * (box_lo[d] + box_hi[d]);
  if (cell_prune_certify(box_lo, box_hi, point_at, count, direction,
                         mutants.contact_as_separation)) {
    verdict.separated = true;
    verdict.by_axis = false;
    for (int a = 0; a < 3; ++a) verdict.direction[a] = direction[a];
  }
  return verdict;
}

}  // namespace mhgp3v
