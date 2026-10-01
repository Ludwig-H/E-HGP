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
// Selection par feuilles sans aucune scission : la racine est la seule feuille du condense, allow_single_cluster
// l'admet, elle est retenue avec la meme regle. scikit-learn 1.9.1 n'y retient aucun cluster (_get_clusters,
// _tree.pyx : la racine choisie faute de feuille y est ecrasee quand cluster_selection_epsilon = 0) : divergence
// volontaire, gravee (head_gate, test_head_frontier.py).
//
// Domaine numerique conjoint (d, p), verifie avant toute condensation (constat H3 ; contre-audits
// CONTRE_AUDIT_TETE_BANCS et CONTRE_AUDIT_TETE_NUMERIQUE_20260929). La condensation garde vivante une composante si
// elle est la racine ou si sa masse atteint mcs (la masse croit vers la racine) ; une composante legere sort avec
// son premier ancetre vivant, au lambda de la fusion de celui-ci.
//   (1) aucune fusion (composante d'au moins deux enfants) au niveau 0 (-0 compris) ;
//   (2) aucune composante nee au niveau 0 (-0 compris) gardee vivante, la vie se lisant sur la masse de tout le
//       sous-arbre, pas sur les seuls points attaches. Le niveau 0 n'a pas de lambda : lambda(0) = +inf n'entre
//       jamais dans un calcul. Une composante legere nee a 0 sort au lambda de sa premiere fusion positive
//       (drop_subtree) ; une composante vivante nee a 0 (racine, ou masse >= mcs) aurait une stabilite infinie ;
//   (3) lambda = niveau^(-z/2) fini et normal pour tout niveau consomme : fusions des composantes vivantes, entrees
//       des points qui leur sont attaches ;
//   (4) M * lambda_max < 2^1000, M = masse totale (u64 exact, au plus (2^32 - 1)^2), lambda_max = plus grand lambda
//       consomme. Une stabilite et le meilleur EOM sont des sommes de durees positives de points disjoints, donc au
//       plus M * lambda_max ; la marge de 2^24 sous DBL_MAX absorbe les arrondis des sommes.
// Hors domaine : numeric_domain (unsupported_degeneracy), avant tout calcul flottant, sorties vides.
// Producteur u18 (tour : niveaux = rayons carres en unites de grille, poids unitaires, M = n < 2^32) : a K >= 2
// sans multiplicite, tout niveau est dans [1/4, 3 * 2^36] (boule bien centree du catalogue : au moins deux sites
// distincts sur sa sphere, donc rayon >= 1/2, et centre dans l'enveloppe de son support, donc rayon <= diametre du
// nuage ; D_K entier dans [1, 3 * 2^36]). (1) et (2) sont donc vides, lambda est un double normal de [2^-304, 2^16]
// pour 0 < z <= 16 et M * lambda_max < 2^48 : toujours dans le domaine. A K = 1, les sites naissent au niveau 0
// avec une masse d'au plus 1 : dans le domaine si mcs >= 2 et au moins deux sites ; a mcs = 1, refuse des qu'un site
// porte son point (entree core : toujours ; auparavant, stabilites infinies).
//
// condense et cluster verifient eux-memes validate(d), validate(p) et ce domaine : sur refus, outcome porte la raison
// et toutes les sorties sont vides ; un appelant lit outcome avant tout acces aux etiquettes ou au condense (les deux
// CLI de tete : mhgp10_cluster, temoin mreach). Les passes de selection et d'etiquetage sont lineaires : le condense
// cree ses parents avant leurs enfants, l'etat d'un ancetre se propage donc du parent a l'enfant en une passe, sans
// remontee par cluster ni par point.
//
// Environnement flottant (decision du raccord R2, etape tete) : la tete calcule dans le mode d'arrondi du fil appelant ;
// contrat du produit : FE_TONEAREST (les CLI ne changent jamais le mode ; porte mhgp10_regression_dendrogram_rounding).
// Aucune garde de mode n'est posee : hors FE_TONEAREST, l'issue, la structure du condense et la selection par feuilles
// de cette porte restent celles du plus proche, les doubles (lambda, stabilites) peuvent differer de quelques ulps et
// la selection EOM n'est pas garantie. Les regles (3) et (4) restent des bornes sures sous tout mode (marge de 2^24
// sous DBL_MAX), mais leur decision exacte au bord se lit dans le mode de l'appelant.
#pragma once

#include <string>
#include <vector>

#include "points/dendrogram.hpp"

namespace mhgp10 {

enum class Selection : u8 { eom, leaf };

// Borne publiee de z (domaine des parametres). Le domaine numerique conjoint ci-dessus decide seul de la finitude.
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

// Domaine complet d'un appel de la tete : validate(d), validate(p), puis domaine numerique conjoint (regles 1 a 4
// ci-dessus ; numeric_domain). Lineaire, un pow par rang de niveau consomme.
Outcome validate(const PointDendrogram& d, const ClusterParams& p);

// Syntaxe d'un z (option --z des deux CLI de tete, fichiers de configurations ; lecteur unique de z depuis le raccord
// R2) : decimal strict, chiffres [. chiffres] [e|E [+|-] chiffres] ou . chiffres [...], sans signe, sans blanc, jeton
// consomme en entier. false sinon (les CLI mettent alors z a NaN). Le domaine est ensuite juge par
// validate(ClusterParams) : 1e400 (z infini) et 1e-400 (z nul) sont syntaxiquement admis, puis refuses.
bool parse_z(const std::string& token, double& z);

// Configurations de tete d'un fichier, une par ligne « mcs z eom|leaf 0|1 » (format des bancs). Lecteur unique de
// --configs pour mhgp10_cluster et le temoin mreach (raccord R2, prealable P3 : cli::parse_head_configs retire). Tout le
// fichier est lu et chaque configuration validee avant usage. Lignes separees par \n (\r\n admis), jetons par espaces
// ou tabulations, lignes blanches ignorees ; mcs en chiffres decimaux sans signe ni depassement de u64, z selon
// parse_z, drapeau 0 ou 1 exactement. Refus du fichier entier, jamais un prefixe silencieux ; out est alors vide :
//   - input_unreadable : ouverture impossible (fichier absent), erreur ou lecture incomplete (dossier compris) ;
//   - parameter_out_of_range : jeton mal forme ou inconnu, nombre de jetons different de 4 sur une ligne,
//     configuration hors domaine (validate(ClusterParams)), fichier sans configuration ou de plus de 1 Mio (2^20
//     octets admis, lecture arretee des le depassement) ;
//   - memory_budget : allocation impossible pendant la lecture.
Outcome read_configs(const std::string& path, std::vector<ClusterParams>& out);

struct CondensedTree {
  Outcome outcome;                 // refus (validate(d), validate(p), numeric_domain) : toutes les tables vides
  std::vector<u32> parent;         // cluster -> cluster parent (kNone : racine)
  std::vector<double> birth;       // lambda de naissance
  std::vector<double> stability;
  std::vector<u64> mass;           // masse a la naissance
  std::vector<u32> point_cluster;  // point -> cluster d'ou il sort
  std::vector<double> point_lambda;
  std::vector<u32> node_cluster;   // noeud du dendrogramme -> cluster condense qui le contient (vote de couverture)
};

struct Clustering {
  Outcome outcome;                 // celui du condense ; refus : toutes les sorties vides
  std::vector<i32> label;          // par point du dendrogramme ; -1 = bruit
  std::vector<u32> selected;       // clusters retenus
  std::vector<i32> cluster_label;  // cluster condense -> etiquette de son premier ancetre retenu (lui compris), -1 sinon
  CondensedTree tree;
  // Diagnostic de cout seulement (la porte causale est mhgp10_head_complexity, en temps) : pas d'un cluster vers son
  // parent faits par la selection et l'etiquetage. Au plus 3 (m - 1) pour m clusters condenses.
  u64 ancestor_steps = 0;
};

CondensedTree condense(const PointDendrogram& d, const ClusterParams& p);
Clustering cluster(const PointDendrogram& d, const ClusterParams& p);

}  // namespace mhgp10
