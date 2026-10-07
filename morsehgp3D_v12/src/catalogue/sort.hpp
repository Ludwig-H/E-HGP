// Ordre canonique des boules du catalogue (fin d'etage) : niveau exact, puis S* compare par la liste triee des
// POSITIONS de ses sites (ordre lexicographique des coordonnees, une liste plus courte et prefixe de l'autre passe
// avant), jamais par les rangs de Morton (ARCHITECTURE.md, paragraphe 1, regle 5 ; CST-0113). Tri d'une permutation
// par blocs puis fusions par co-rangs, sur le Pool ; port explicite de src/catalogue/sort_indices.cpp et
// sort_level_key.hpp de la v11 (ac081a06f) : cles F3 des niveaux (64 bits de tete de N et D, quotient en binaire64),
// decision F4 seulement hors de la marge prouvee c = 1 - 2^-40, sinon comparaison exacte (doctrine flottante F1 a F4,
// ARCHITECTURE.md de la v11, paragraphe 4). Aucune decision en flottant : la permutation est celle du tri exact.
#pragma once

#include "catalogue/internal.hpp"

namespace mhgp12::catalogue_detail {

// Cle F3 d'un niveau. Les entiers de Level ont au plus 8s+12 <= 276 bits : ldexp exact, quotient normal et fini.
double level_key(const num::Level& level) noexcept;
// -1/+1 : ordre CERTAIN des niveaux exacts ; 0 : repli exact (cles egales ou voisines).
int level_key_order(double a, double b) noexcept;

// Comparaison des S* de deux boules par positions (-1, 0, 1).
int compare_support_positions(const Cloud& cloud, const u32 (&a)[4], const u32 (&b)[4]) noexcept;

// Permutation des boules dans l'ordre canonique ; keys : cles F3 deja calculees. Deux tableaux de N u32 pendant le
// tri (un rendu). N < kNone.
[[nodiscard]] Result<Buffer<u32>> sort_balls(const Cloud& cloud, std::span<const BallRecord> records,
                                             std::span<const num::Level> levels, std::span<const double> keys,
                                             MemoryBudget& budget, sched::Pool& pool) noexcept;

}  // namespace mhgp12::catalogue_detail
