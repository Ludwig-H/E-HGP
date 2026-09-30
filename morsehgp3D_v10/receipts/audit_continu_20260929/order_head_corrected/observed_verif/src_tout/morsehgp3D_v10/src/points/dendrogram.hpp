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
#include "sched/pool.hpp"

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

// Invariants structurels : CSR, parents coherents avec les enfants, niveaux croissants le long des
// aretes, une seule racine, attaches dans l'intervalle de vie, noeuds crees avant leurs parents
// (identifiants croissants vers la racine).
// pool (facultatif) : controles en parallele, meme premier defaut (meme ordre des controles) qu'en serie.
Outcome validate(const PointDendrogram& d, sched::Pool* pool = nullptr);

}  // namespace mhgp10
