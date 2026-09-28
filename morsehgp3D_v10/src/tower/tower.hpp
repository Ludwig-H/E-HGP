// Tour FULL par ordre K a partir du catalogue critique, et hierarchie de points C n X.
//
// Semantique (validee contre l'oracle Gamma_k par reference/hgp10_ref.py, degenerescences comprises) :
// pour une boule b (interieur I de poids p, coquille U de m sites, centre c) et un ordre K avec
// p < K <= p + m, soit t = K - p et A_t les sous-ensembles de U de taille t separables (c hors de conv(A),
// theoreme de Gordan) :
//   - A_t vide : NAISSANCE d'une composante au niveau de b ;
//   - sinon les morceaux locaux (composantes de A ~ A' ssi c hors de conv(A u A')) sont joints au niveau
//     de b ; un representant I u A par morceau est rattache a une naissance par DESCENTE.
// Descente d'une K-partie F : MEB exacte de F, puis (i) >= K sites strictement interieurs : saut aux K plus
// proches du centre ; (ii) sinon, si la sphere est une boule du catalogue dont la fenetre contient K : la
// cellule (b, K) (resolue une fois, memo) ; (iii) sinon un representant du premier morceau. Le niveau
// decroit strictement a chaque pas (sinon invariant_violated).
// K = 1 : les sites naissent au niveau 0. Multiplicites > 1 : refus explicite (semantique ponderee a venir).
#pragma once

#include <vector>

#include "catalogue/catalogue.hpp"
#include "cloud/site_tree.hpp"
#include "points/dendrogram.hpp"

namespace mhgp10 {

struct OrderForest {
  int k = 0;
  // noeuds : naissances puis fusions, dans l'ordre de creation (rang croissant)
  std::vector<u32> rank;       // 0 = niveau nul, sinon rang du catalogue + 1
  std::vector<u32> parent;     // kNone pour la racine
  std::vector<u32> child_off;  // CSR des enfants (fusions N-aires)
  std::vector<u32> child_val;
  std::vector<u32> birth;      // naissance : boule (ou site a K = 1) ; kNone pour une fusion
  std::vector<u32> point_node; // par site : composante qui le contient a son niveau d'entree D_K(x)
  std::vector<u64> point_level;// par site : D_K(x) (entier exact)
  u64 descents = 0, descent_steps = 0, memo_hits = 0, joins = 0, births = 0, merges = 0;
};

struct Tower {
  int kmax = 0;
  std::vector<OrderForest> orders;  // orders[k - 1]
};

struct TowerParams {
  int kmax = 5;
  bool points = true;  // attaches C n X
};

Result<Tower> build_tower(const Cloud& cloud, const SiteTree& tree, const Catalogue& cat, const TowerParams& params,
                          sched::Pool& pool);

// Hierarchie de points C n X de l'ordre K (points = sites), prete pour la tete de clustering.
PointDendrogram point_dendrogram(const Catalogue& cat, const OrderForest& forest, const Cloud& cloud);

}  // namespace mhgp10
