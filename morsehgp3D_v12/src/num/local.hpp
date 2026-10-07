// Predicats en repere de feuille (docs/CONTRAT_NUMERIQUE.md, paragraphes 2 et 3), ecrits dans num pour que le
// catalogue (tranche T1) les emploie : leurs arguments appartiennent au repere donne (NUM-COUVERTURE), controle ici,
// et la voie suit le palier de ce repere, uniforme pour toute la feuille.
#pragma once

#include "num/frame.hpp"

namespace mhgp12::num {

// Distance du reservoir avant G1 (CST-0208) : sum_j (2 x_j - lo_j - hi_j)^2, quatre fois le carre de l'ecart du site au
// centre de la boite fermee [lo, hi] ; le filtrage G1 d'un enfant s'evalue dans le repere du PARENT (CST-0112), qui
// contient la boite de l'enfant et les sites de la liste parente. Avec |2x_j - lo_j - hi_j| < 2M, la somme est < 12 M^2 :
// budget 2s+4, natif i64 jusqu'a s = 29 ; le seuil s = 30 du seul G1 ne couvre pas cette preparation (temoin de
// l'auditeur : s = 30, boite [0,1]^3, site (2^30-1)^3, 3(2^31-3)^2 > 2^63). Voie : i64 aux paliers etroit et moyen
// (52 bits au plus), i128 au palier large (70 bits au plus).
// Refus parameter_out_of_range si le site ou la boite sortent du repere, ou si lo > hi sur un axe.
[[nodiscard]] Result<u128> reservoir_distance(const Frame& frame, const std::array<u32, 3>& site,
                                              const std::array<u64, 3>& lo, const std::array<u64, 3>& hi,
                                              LaneCount* lanes = nullptr) noexcept;

}  // namespace mhgp12::num
