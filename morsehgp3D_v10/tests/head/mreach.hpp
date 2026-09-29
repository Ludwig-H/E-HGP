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

// Hierarchie mreach ; les points de la hierarchie sont les SITES (poids = multiplicite). alpha (1 ou 2, parametre
// alpha de scikit-learn) : mreach = max(core(a), core(b), |a - b| / alpha), publie a l'echelle alpha^2 en carre,
// max(alpha^2 core2(a), alpha^2 core2(b), |a - b|^2), entier exact ; une homothetie ne change pas l'EOM.
PointDendrogram mreach_dendrogram(const SiteTree& tree, u64 k, sched::Pool& pool, u64 alpha = 1);

}  // namespace mhgp10
