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
//
// Construction (tous les ordres ensemble, par etages ; parallelisme par l'unique sched::Pool, sorties identiques
// quel que soit le nombre de fils) :
//   L  atlas des cellules (boule, K) de chaque fenetre, naissances et jonctions de tous les ordres, en une passe
//      parallele par morceaux fixes de boules (coquilles regulieres analytiques ; coquilles etendues : quotient
//      local calcule une fois par cellule) ;
//   H  semis H_K : populations des naissances regulieres de K sites (table plate, egalite verifiee sur la cle) ;
//   G  descentes de tous les representants de tous les ordres ; la descente est une fonction pure de (F, K), le
//      memo par cellule n'est qu'un cache. MEB proposee en double puis certifiee en exact (sinon Welzl exact) ;
//      recensement (I, U) lu au catalogue quand la MEB certifiee a pour support canonique une boule du catalogue
//      (juge d'echantillon : 1 boule sur 32 recensee aussi par l'arbre), sinon boule fermee par l'arbre ;
//   T  Kruskal par plateaux, un ordre par tache ;
//   P  attaches C n X (une requete des kmax plus proches par site, puis une descente par ordre) ;
//   V  verticales : naissance reguliere = representant I u U \ {U[m-1]} de la jonction de la meme boule a K - 1,
//      deja resolu ; ancetres par pointeurs de saut (Myers) au lieu de la remontee parent par parent.
// Tout filtre flottant a une marge prouvee et un repli exact ; aucune decision n'est prise en flottant.
#pragma once

#include <vector>

#include "catalogue/catalogue.hpp"
#include "cloud/site_tree.hpp"
#include "points/dendrogram.hpp"

namespace mhgp10 {

// Compteurs de travail de la resolution. Ils dependent de l'ordre d'arrivee des fils (memo partage) : ils
// decrivent le travail, pas l'objet, et ne sont jamais dans le dump. A un fil, ils sont deterministes.
struct ResolveCounters {
  u64 resolves = 0;      // appels de resolve
  u64 steps = 0;         // pas de descente
  u64 seed_hits = 0;     // arrets sur un semis H_K
  u64 birth_hits = 0;    // arrets sur la cellule de naissance d'une boule
  u64 memo_hits = 0;     // arrets sur le memo d'une cellule
  u64 meb = 0;           // MEB exactes
  u64 knn_queries = 0;   // requetes kNN (descente et attaches)
  u64 knn_jumps = 0;     // sauts K-NN
  u64 closed_balls = 0;  // boules fermees
  u64 lookups = 0;       // consultations du catalogue par support
  u64 local_calls = 0;   // structures locales calculees pendant les descentes
  u64 reused = 0;        // descentes evitees (verticale d'une naissance reguliere : representant deja resolu)
  u64 census_cat = 0;    // recensements lus au catalogue (MEB de support certifie, boule du catalogue)
  u64 level_exact = 0;   // gardes I3 tranchees en exact (filtre flottant non concluant)
  u64 jump_exact = 0;    // sauts K-NN tranches en exact (ecart des distances approchees insuffisant)
  void add(const ResolveCounters& o) {
    level_exact += o.level_exact;
    jump_exact += o.jump_exact;
    reused += o.reused;
    census_cat += o.census_cat;
    resolves += o.resolves;
    steps += o.steps;
    seed_hits += o.seed_hits;
    birth_hits += o.birth_hits;
    memo_hits += o.memo_hits;
    meb += o.meb;
    knn_queries += o.knn_queries;
    knn_jumps += o.knn_jumps;
    closed_balls += o.closed_balls;
    lookups += o.lookups;
    local_calls += o.local_calls;
  }
};

// Travail d'un ordre (la tour est construite par etages, tous ordres ensemble : les temps sont par etage, dans
// TowerStats ; ici, la duree des taches sequentielles propres a l'ordre).
struct OrderStats {
  double t_kruskal = 0;   // Kruskal par plateaux de cet ordre (tache sequentielle)
  double t_vertical = 0;  // fusions de la carte verticale K -> K - 1 (tache sequentielle)
  u64 local_cells = 0;    // cellules (boule, K) de l'atlas
  u64 walk_steps = 0;     // pas de remontee vers un ancetre (attaches et verticales)
  ResolveCounters join, point, vertical;  // resolutions des jonctions, des attaches et des verticales
};

// Temps de mur par etage (secondes).
struct TowerStats {
  double t_prepare = 0;   // coordonnees, index des supports
  double t_local = 0;     // atlas des cellules, structures locales, naissances et jonctions
  double t_seeds = 0;     // index des semis H_K
  double t_resolve = 0;   // resolution des representants des jonctions
  double t_kruskal = 0;   // Kruskal par plateaux (ordres en parallele)
  double t_points = 0;    // attaches C n X
  double t_vertical = 0;  // cartes verticales
  u64 meb_fallbacks = 0;  // MEB dont la proposition flottante n'a pas ete certifiee (repli Welzl exact)
};

struct OrderForest {
  int k = 0;
  // noeuds : naissances puis fusions, dans l'ordre de creation (rang croissant)
  std::vector<u32> rank;       // 0 = niveau nul, sinon rang du catalogue + 1
  std::vector<u32> parent;     // kNone pour la racine
  std::vector<u32> child_off;  // CSR des enfants (fusions N-aires)
  std::vector<u32> child_val;
  std::vector<u32> birth;      // naissance : boule (ou site a K = 1) ; kNone pour une fusion
  std::vector<u32> lower;      // image verticale dans l'ordre K - 1 (noeud vivant au niveau de creation) ; vide a K = 1
  std::vector<u32> point_node; // par site : composante d'entree (voir PointEntry)
  std::vector<u64> point_level;// entree core : D_K(x) (entier exact)
  std::vector<u32> point_cat_rank;  // entree cover : rang de noeud (rang du catalogue + 1) du niveau alpha_K(x)^2
  std::vector<u32> ball_node;       // entree cover, sur demande (TowerParams::ball_nodes) : composante de
                                    // L_K(niveau de b) qui contient le centre de la boule b (kNone si le poids de b
                                    // est < K) ; relation de couverture des points
  u64 descents = 0, descent_steps = 0, memo_hits = 0, joins = 0, births = 0, merges = 0;
  OrderStats stats;
};

struct Tower {
  int kmax = 0;
  std::vector<OrderForest> orders;  // orders[k - 1]
  TowerStats stats;
};

// Entree des points dans la hierarchie d'ordre K (docs/CLUSTERING_DEPUIS_LA_TOUR_20260929.md) :
//   core  : x entre a D_K(x) = d_K(x)^2 (x compris) dans la composante de L_K qui le contient (semantique des
//           coeurs, C n X) ;
//   cover : x entre a alpha_K(x)^2, alpha_K(x) = rayon de la plus petite boule fermee contenant x et au moins K - 1
//           autres sites, dans la composante de cette boule (amas discrets, theoreme 2 de la these :
//           d(x, C) <= r). d_K(x) / 2 <= alpha_K(x) <= d_K(x). A K = 1, alpha = 0 : les deux coincident.
enum class PointEntry : u8 { core, cover };

struct TowerParams {
  int kmax = 5;
  bool points = true;  // attaches des points
  PointEntry entry = PointEntry::core;
  int cover_extra = 0;      // entree cover : boule couvrante de poids >= K + cover_extra (1 : entree a
                            // alpha_{K+1}(x), semantique de HGP-old) ; le catalogue doit servir K + cover_extra
  bool ball_nodes = false;  // entree cover : publier aussi ball_node pour toutes les boules (une resolution par
                            // boule de poids >= K) ; sinon seules les premieres boules couvrantes sont resolues
  int only_order = 0;  // > 0 : ne construire que cet ordre (les ordres sont independants)
  bool verticals = true;  // cartes K -> K - 1 (ignorees si only_order > 0)
};

Result<Tower> build_tower(const Cloud& cloud, const SiteTree& tree, const Catalogue& cat, const TowerParams& params,
                          sched::Pool& pool);

// Hierarchie de points C n X de l'ordre K (points = sites), prete pour la tete de clustering.
PointDendrogram point_dendrogram(const Catalogue& cat, const OrderForest& forest, const Cloud& cloud);

}  // namespace mhgp10
