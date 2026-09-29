// Tete de clustering : condensation HDBSCAN exacte (Campello, Moulavi, Sander 2013) sur un PointDendrogram,
// selection par exces de masse (EOM) ou par feuilles, etiquettes.
//
// Semantique (celle de scikit-learn/hdbscan) : en descendant les niveaux depuis la racine, une composante
// dont au moins deux enfants pesent >= mcs se scinde (le cluster meurt, chaque gros enfant ouvre un
// cluster) ; les enfants legers perdent leurs points au niveau de la scission ; si aucun enfant n'est
// gros, le cluster meurt et TOUS ses points restants sortent a ce niveau. Un point attache directement
// sort a son propre niveau d'entree. Stabilite = somme des w (lambda_sortie - lambda_naissance).
// Echelle lambda = r^(-z) = niveau^(-z/2) (z = 1 : HDBSCAN). La racine nait a lambda = 0.
#pragma once

#include <vector>

#include "points/dendrogram.hpp"

namespace mhgp10 {

enum class Selection : u8 { eom, leaf };

struct ClusterParams {
  u64 min_cluster_size = 5;        // en masse (somme des multiplicites)
  double z = 1.0;                  // exposant de l'echelle lambda
  Selection selection = Selection::eom;
  bool allow_single_cluster = false;
};

struct CondensedTree {
  std::vector<u32> parent;         // cluster -> cluster parent (kNone : racine)
  std::vector<double> birth;       // lambda de naissance
  std::vector<double> stability;
  std::vector<u64> mass;           // masse a la naissance
  std::vector<u32> point_cluster;  // point -> cluster d'ou il sort
  std::vector<double> point_lambda;
  std::vector<u32> node_cluster;   // noeud du dendrogramme -> cluster condense qui le contient (vote de couverture)
};

struct Clustering {
  std::vector<i32> label;          // par point du dendrogramme ; -1 = bruit
  std::vector<u32> selected;       // clusters retenus
  std::vector<i32> cluster_label;  // cluster condense -> etiquette de son premier ancetre retenu (lui compris), -1 sinon
  CondensedTree tree;
};

CondensedTree condense(const PointDendrogram& d, const ClusterParams& p);
Clustering cluster(const PointDendrogram& d, const ClusterParams& p);

}  // namespace mhgp10
