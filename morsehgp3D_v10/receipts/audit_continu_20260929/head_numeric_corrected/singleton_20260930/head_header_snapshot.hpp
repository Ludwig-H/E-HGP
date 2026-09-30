// Tete de clustering : condensation HDBSCAN exacte (Campello, Moulavi, Sander 2013) sur un PointDendrogram,
// selection par exces de masse (EOM) ou par feuilles, etiquettes.
//
// Semantique (celle de scikit-learn/hdbscan) : en descendant les niveaux depuis la racine, une composante
// dont au moins deux enfants pesent >= mcs se scinde (le cluster meurt, chaque gros enfant ouvre un
// cluster) ; les enfants legers perdent leurs points au niveau de la scission ; si aucun enfant n'est
// gros, le cluster meurt et TOUS ses points restants sortent a ce niveau. Un point attache directement
// sort a son propre niveau d'entree. Stabilite = somme des w (lambda_sortie - lambda_naissance).
// Echelle lambda = r^(-z) = niveau^(-z/2) (z = 1 : HDBSCAN). La racine nait a lambda = 0.
// allow_single_cluster (regle de scikit-learn, _do_labelling avec cluster_selection_epsilon = 0) : quand la racine
// est le seul cluster retenu, un point ne lui appartient que s'il sort au plus haut lambda des lignes de la racine
// dans le condense (points qui sortent de la racine, naissances de ses enfants) ; les autres points sont du bruit.
//
// Preconditions de condense et cluster : validate(d).ok() et validate(p).ok(). Les passes de selection et
// d'etiquetage sont lineaires : le condense cree ses parents avant leurs enfants, l'etat d'un ancetre se propage
// donc du parent a l'enfant en une passe, sans remontee par cluster ni par point.
#pragma once

#include <string>
#include <vector>

#include "points/dendrogram.hpp"

namespace mhgp10 {

enum class Selection : u8 { eom, leaf };

// Borne publiee de z : pour tout niveau L de [1/4, 2^64] (rayons carres non nuls de coordonnees entieres), lambda =
// L^(-z/2) est un double normal de [2^-512, 2^16] : ni debordement ni sous-flux, et des stabilites finies (au plus
// 2^80 pour une masse totale < 2^64). Le niveau 0 garde lambda = +inf par convention (lambda_of).
inline constexpr double kMaxScaleExponent = 16.0;

struct ClusterParams {
  u64 min_cluster_size = 5;        // en masse (somme des multiplicites), >= 1
  double z = 1.0;                  // exposant de l'echelle lambda, 0 < z <= kMaxScaleExponent
  Selection selection = Selection::eom;
  bool allow_single_cluster = false;
};

// Domaine des parametres de tete : min_cluster_size >= 1, z fini et 0 < z <= kMaxScaleExponent, selection eom ou
// leaf. Hors domaine : parameter_out_of_range (invalid_input).
Outcome validate(const ClusterParams& p);

// Configurations de tete d'un fichier, une par ligne « mcs z eom|leaf 0|1 » (format des bancs). Tout le fichier est
// lu et chaque configuration validee avant usage : un champ mal forme, un jeton inconnu, un reste non lu ou une
// configuration hors domaine refuse le fichier entier (parameter_out_of_range) ; jamais un prefixe silencieux.
Outcome read_configs(const std::string& path, std::vector<ClusterParams>& out);

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
  // Travail discret (diagnostic de cout, pas un chrono) : pas d'un cluster vers son parent faits par la selection et
  // l'etiquetage (listes d'enfants, desactivation EOM, etiquette du premier ancetre retenu). Au plus 3 (m - 1) pour
  // m clusters condenses, independamment de la profondeur et du nombre de points.
  u64 ancestor_steps = 0;
};

CondensedTree condense(const PointDendrogram& d, const ClusterParams& p);
Clustering cluster(const PointDendrogram& d, const ClusterParams& p);

}  // namespace mhgp10
