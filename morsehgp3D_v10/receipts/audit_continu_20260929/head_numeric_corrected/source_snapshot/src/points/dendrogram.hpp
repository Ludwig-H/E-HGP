// Contrat entre producteurs de hierarchies de points (tour C n X, temoin d'atteignabilite mutuelle) et la tete.
//
// Arbre N-aire enracine sur des COMPOSANTES ; les points y sont attaches. Une composante est creee a un
// niveau (rang dans `level`, niveaux = rayons carres croissants) soit par naissance (aucun enfant), soit
// par multifusion de ses enfants (plateau atomique : jamais binarise). Un point x est attache a la
// composante qui le contient a son niveau d'entree ; point_rank[x] >= node_rank[point_node[x]] et, si la
// composante a un parent, point_rank[x] <= node_rank[parent] (egalite : naissance et fusion dans le
// meme plateau, duree de vie nulle).
#pragma once

#include <vector>

#include "core/status.hpp"

namespace mhgp10 {

struct PointDendrogram {
  std::vector<double> level;          // rang -> niveau (rayon carre), strictement croissant
  std::vector<u32> node_rank;         // composante -> rang de creation
  std::vector<u32> child_off;         // CSR des enfants absorbes a la creation (tailles nodes + 1)
  std::vector<u32> child_val;
  std::vector<u32> parent;            // composante -> parent (kNone pour la racine)
  std::vector<u32> point_node;        // point -> composante d'attache
  std::vector<u32> point_rank;        // point -> rang d'entree
  std::vector<u32> point_weight;      // point -> multiplicite (>= 1)

  u32 nodes() const { return static_cast<u32>(node_rank.size()); }
  u32 points() const { return static_cast<u32>(point_node.size()); }
};

// Invariants structurels, verifies en temps et memoire lineaires, sans lecture hors bornes meme sur une entree
// quelconque : tailles et CSR bornes (child_off croissant) ; niveaux finis, >= 0, strictement croissants ;
// parents bornes, une seule racine, equivalence parents <-> CSR (toute non-racine apparait exactement une fois,
// dans la liste de son parent) ; niveaux croissants le long des aretes ; noeuds crees avant leurs parents
// (identifiants croissants vers la racine) ; attaches bornees (point_rank < level.size()) dans l'intervalle de vie.
// Un dendrogramme valide est sur pour la tete (head.hpp).
Outcome validate(const PointDendrogram& d);

}  // namespace mhgp10
