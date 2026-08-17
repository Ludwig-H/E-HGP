// MorseHGP3D v4 — SELECTION AXIALE q4 (MATHEMATIQUES.md § 4.6).
//
// A seed aigu (a,b,x) fixe, les spheres par le cercle circonscrit forment
// le faisceau Phi(z; mu) = P3(z) - mu·pi(z), P3 = forme de Gram q3 (tete
// G > 0), pi(z) = n·(z-a). Classification par site : A = P3(z) (< 2^106),
// B = pi(z) (< 2^55). Borne de rang : une completion vivante est dans les
// premiers groupes de son cote (mu croissant cote positif, decroissant
// cote negatif) tant que p + predecesseurs_stricts <= h_4 - 1. Le filtre
// est un MINORANT fail-open : il reduit les candidats, jamais l'autorite —
// census, owner, arite et exact-once restent inchanges sur chaque candidat,
// et le juge brut verifie que les records ne bougent pas.
#pragma once

#include <algorithm>
#include <vector>

#include "q3_event.hpp"
#include "q4_instruction.hpp"

namespace mhgp4 {

namespace detail_ax {

// Signe exact de A1·B2 - A2·B1 (|A| < 2^106, |B| < 2^55 : produits
// < 2^161, magnitudes via mul_level_192, signes traites a part).
inline int cmp_cross_128x64(i128 A1, i64 B2, i128 A2, i64 B1) {
  const auto sgn_prod = [](i128 u, i64 v) {
    if (u == 0 || v == 0) return 0;
    return ((u < 0) != (v < 0)) ? -1 : 1;
  };
  const int s1 = sgn_prod(A1, B2);
  const int s2 = sgn_prod(A2, B1);
  if (s1 != s2) return s1 < s2 ? -1 : 1;
  if (s1 == 0) return 0;
  const U192 m1 = mul_level_192(detail_ev::uabs(A1), (u128)(B2 < 0 ? -B2 : B2));
  const U192 m2 = mul_level_192(detail_ev::uabs(A2), (u128)(B1 < 0 ? -B1 : B1));
  const int c = cmp_u192(m1, m2);
  return s1 > 0 ? c : -c;
}

}  // namespace detail_ax

struct AxialSite {
  i32 u;
  i128 A;  // P3(z)
  i64 B;   // pi(z) = n·(z-a)
};

// mu_1 < mu_2 au sein d'un MEME cote (B1·B2 > 0) : A1·B2 < A2·B1.
inline bool axial_mu_less(const AxialSite& s1, const AxialSite& s2) {
  return detail_ax::cmp_cross_128x64(s1.A, s2.B, s2.A, s1.B) < 0;
}
inline bool axial_mu_equal(const AxialSite& s1, const AxialSite& s2) {
  return detail_ax::cmp_cross_128x64(s1.A, s2.B, s2.A, s1.B) == 0;
}

// Coupe de rang d'un cote trie (mu croissant cote positif, decroissant
// cote negatif) : garde les sites tant que le nombre de PREDECESSEURS
// STRICTS (groupes de mu distincts) plus `p` reste <= h - 1. `slack`
// retranche des groupes (mutant axial-rank-cut : slack = 1, la porte qui
// le tue prouve que la borne est serree au groupe pres).
inline void axial_rank_cut(const std::vector<AxialSite>& sorted_side, u64 p,
                           u64 h, u64 slack, std::vector<i32>* out) {
  u64 preds = 0;
  for (size_t t = 0; t < sorted_side.size(); ++t) {
    if (t > 0 && !axial_mu_equal(sorted_side[t], sorted_side[t - 1]))
      preds = t;  // rang du premier element du groupe courant
    if (p + preds + slack > h - 1) break;
    out->push_back(sorted_side[t].u);
  }
}

}  // namespace mhgp4
