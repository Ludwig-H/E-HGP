// MorseHGP3D v2 — API publique.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "mhgp/exact.hpp"
#include "mhgp/sphere.hpp"

namespace mhgp {

inline constexpr int kMaxRank = 32;   // borne dure sur s_max supportee
inline constexpr int kMaxSupport = 4;

// ---------------------------------------------------------------------------
// Une sphere critique : boule fermee dont le contenu est exactement I ∪ U et
// qui est la plus petite boule contenant ce contenu.
// ---------------------------------------------------------------------------
struct CriticalSphere {
  i32 support[kMaxSupport];  // U, trie
  i32 n_support = 0;
  i32 rank = 0;              // s = |I| + |U|
  i32 members_begin = 0;     // offset dans le pool de membres (I ∪ U, trie)
  Sphere sph{};
  double beta = 0.0;
};

struct Catalogue {
  std::vector<CriticalSphere> spheres;
  std::vector<i32> members;   // pool concatene
  // diagnostics
  std::vector<i32> neighbourhood_size;   // |W_p| final par point
  std::vector<i32> growth_rounds;        // nombre de doublements par point
  std::vector<u8> certified;             // majoration a priori atteinte
  i64 candidate_pairs = 0;
  i64 candidate_triples = 0;
  i64 candidate_quads = 0;
  // Sorties du domaine declare (DESIGN.md §6.4). Une coquille cospherique n'est
  // pas representable : la sphere est rejetee, donc la fusion qu'elle porte
  // n'existe dans aucune foret. Le rejet DOIT etre publie, sinon le resultat est
  // silencieusement incomplet -- c'est ce que l'oracle O2 a mis au jour.
  i64 degenerate_shells = 0;
  i64 shell_anomalies = 0;
};

// ---------------------------------------------------------------------------
// Foret de fusion d'un ordre k.
// ---------------------------------------------------------------------------
// `source` est l'index, dans le catalogue, de la sphere critique qui a cree le
// noeud : la sphere de rang k pour une naissance, et pour une multifusion la
// plus petite (par index) des spheres de rang k+1 du lot qui participent a
// cette composante. Dans les deux cas le NIVEAU de la sphere source est
// exactement le niveau du noeud : `source` est donc le seul acces exact au
// niveau, `beta` n'en etant que la projection flottante pour l'affichage. Un
// consommateur qui doit decider « ce noeud est-il au niveau a ? » compare les
// rationnels de `cat.spheres[source].sph`, jamais `beta` (l'ecart d'un ULP se
// produit precisement aux niveaux critiques, ou la decision compte).
struct ForestNode {
  double beta = 0.0;
  i32 parent = -1;
  i32 first_child = -1;
  i32 next_sibling = -1;
  i32 n_children = 0;
  i32 kind = 0;       // 0 = naissance, 1 = multifusion
  i32 source = -1;    // index dans le catalogue ; jamais -1 dans une foret publiee
};

struct Forest {
  i32 order = 0;
  std::vector<ForestNode> nodes;
  std::vector<i32> roots;
  i64 births = 0;
  i64 merge_events = 0;
  i64 killed = 0;
  i64 unresolved_arms = 0;   // bras dont la descente n'aboutit pas
  i64 censored_events = 0;   // evenements censures atomiquement
  bool authoritative = true; // faux des qu'un evenement est censure
};

// ---------------------------------------------------------------------------
// Entree / options
// ---------------------------------------------------------------------------
struct Options {
  int k_max = 10;         // K
  bool all_orders = true; // calculer la tour k = 1..K
  int seed_neighbours = 0;   // 0 = auto (4 * s_max)
  int max_growth_rounds = 24;
  int cone_directions = 42;  // recouvrement de S^2 pour la certification
  int threads = 0;           // 0 = auto
  bool verbose = false;
  bool exhaustive_neighbourhood = false;  // diagnostic : W_p = X
};

struct Timings {
  double quantize_ms = 0;
  double grid_ms = 0;
  double catalogue_ms = 0;
  double forest_ms = 0;
  double total_ms = 0;
};

struct Result {
  Catalogue catalogue;
  std::vector<Forest> forests;  // indexe par k-1
  Timings timings;
  i32 n = 0;
  i32 k_max = 0;
  i32 uncertified_points = 0;
  bool forests_suppressed = false;  // aucune foret publiee
  bool out_of_declared_domain = false;  // coquille cospherique rencontree
  std::vector<i32> censored_orders;
};

// Quantifie un nuage flottant sur la grille entiere [0, 2^kCoordBits).
std::vector<P3> quantize(const std::vector<double>& xyz, double* scale_out,
                         double* origin_out);

// Construit le catalogue des spheres critiques de rang ferme <= s_max.
Catalogue build_catalogue(const std::vector<P3>& pts, int s_max,
                          const Options& opt);

// Construit la foret de fusion de l'ordre k a partir du catalogue.
Forest build_forest(const std::vector<P3>& pts, const Catalogue& cat, int k);

Result run(const std::vector<P3>& pts, const Options& opt);

}  // namespace mhgp
