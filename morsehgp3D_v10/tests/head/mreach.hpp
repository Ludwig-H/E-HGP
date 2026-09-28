// Temoin : hierarchie de points d'atteignabilite mutuelle (l'objet d'HDBSCAN), niveaux entiers exacts.
//
// core2(s) = D_K(s) (K-ieme distance carree ponderee, le site compte avec sa multiplicite : convention
// scikit-learn min_samples = K) ; mreach2(a, b) = max(core2(a), core2(b), |a - b|^2). Arbre couvrant
// minimal exact (Prim en O(n^2), parallele), puis Kruskal par plateaux : chaque site est une composante
// nee a core2(s) ; les aretes de meme poids forment une multifusion N-aire.
#pragma once

#include "cloud/site_tree.hpp"
#include "points/dendrogram.hpp"
#include "sched/pool.hpp"

namespace mhgp10 {

// Distance-coeur carree de chaque site (K >= 1).
std::vector<u64> core_distances2(const SiteTree& tree, u64 k, sched::Pool& pool);

// Hierarchie mreach ; les points de la hierarchie sont les SITES (poids = multiplicite).
PointDendrogram mreach_dendrogram(const SiteTree& tree, u64 k, sched::Pool& pool);

}  // namespace mhgp10
