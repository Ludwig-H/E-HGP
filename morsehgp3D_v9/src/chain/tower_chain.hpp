// MorseHGP3D v9 — chaine de bout en bout : generateur exact (mhgp9::gen, port
// v8) -> catalogue canonique de boules -> tour HGP FULL (mhgp9::tower, port v7).
//
// Frontiere du chronometre du contrat (synthese d'ouverture § 9, hypothese 1) :
// du nuage prepare en memoire (sites 18 bits distincts) a la tour complete en
// memoire. La lecture du fichier est mesuree a part par l'appelant.
//
// Le catalogue n'est pas une autorite : il recoupe deux implementations
// independantes et refuse toute divergence (statut kInvariantViolated) :
//   - la cle v8 (ExactBall) est recalculee depuis le support avec les formules
//     de la tour v7 ;
//   - le compte d'interieurs et la taille de coquille emis par le generateur
//     sont recalcules par un census exact sur l'index de la tour ;
//   - q_min recalcule sur la coquille (quotient local) doit egaler la plus
//     petite arite presentee (sinon une voie du generateur est incomplete).
// Une coquille de plus de 12 sites est un REFUS DE DOMAINE explicite
// (kUnsupportedDegeneracy), jamais une troncature (contre-audit A § 1).
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "core/types.hpp"  // mhgp9::gen::Point3
#include "../tower/forest/full_ball_tower.hpp"

namespace mhgp9 {

enum class ChainStatus {
  kComplete,               // tour complete relative au catalogue recoupe
  kUnsupportedDegeneracy,  // coquille > 12 : hors du domaine du constructeur
  kInvalidInput,
  kResourceExhausted,
  kInvariantViolated,
};
const char* chain_status_name(ChainStatus status);

struct ChainOptions {
  unsigned kmax = 5;
  unsigned separation_s = 8;   // jamais moins de 8 (consigne utilisateur)
  std::size_t workers = 1;     // generateur et census du catalogue
  // Resolution geometrique de la tour : 0 = voie temporelle sequentielle de la
  // v7 ; T > 0 = voie statique sur T fils (meme objet, condenses identiques) ;
  // -1 (defaut) = autant de fils que `workers` s'il y en a plus d'un, sinon 0.
  int tower_static_threads = -1;
  bool run_tower = true;       // false : s'arreter au catalogue (mesure de l'amont)
  bool keep_catalogue = false; // publier le catalogue recoupe (portes, juges)
  // Atlas q4 saturant (option v8 de la reprise u18, desactivee par defaut en
  // v8) : arret d'une cellule des que son compte certifie atteint K-1,
  // certificat terminal sans fragment. Contrat v9 : ACTIVE par defaut dans la
  // chaine ; la sonde publie la valeur et le plan G4 l'epingle.
  bool atlas_saturate_deep = true;
  // Census q3 depuis la feuille exacte de l'atlas (levier de l'auditeur A,
  // meme objet). Contrat v9 : active par defaut, publie et epingle de meme.
  bool q3_leaf_census = true;
  // Certificat de voie morte q3/q4 par arete (lanes/q34_dead_lanes.hpp), meme
  // objet. Contrat v9 : active par defaut, publie et epingle.
  bool q34_dead_lanes = true;
  // Cache des noeuds temoins du filtre de paires (meme extremite a), meme
  // objet. Contrat v9 : active par defaut, publie et epingle.
  bool q34_witness_cache = true;
  // Noyau diametral du certificat de voie morte (exige q34_dead_lanes), meme
  // objet. Contrat v9 : active par defaut, publie et epingle.
  bool q34_dead_core = true;
};

// Temps de mur en millisecondes, CPU du processus en secondes.
struct ChainTimes {
  double prepare_ms = 0, gen_index_ms = 0, q2_ms = 0, q34_ms = 0;
  double merge_ms = 0, tower_index_ms = 0, census_ms = 0, tower_ms = 0, total_ms = 0;
  // Verification digest of the published tower, measured after total_ms
  // (not part of the chain's construction time).
  double digest_ms = 0;
  double cpu_s = 0;
};

struct CatalogueStats {
  std::uint64_t q2_presentations = 0, q3_presentations = 0, q4_presentations = 0;
  std::uint64_t unique_keys = 0, balls = 0, extra_shell_balls = 0;
  std::uint64_t shell_over_cap = 0, max_shell = 0, max_interior = 0;
  std::uint64_t census_nodes = 0, census_leaf_tests = 0;
  std::array<std::uint64_t, 5> balls_by_qmin{};
  std::array<std::uint64_t, 17> balls_by_shell{};  // index = taille de coquille (16 = 16 et plus)
  std::uint64_t bytes = 0;  // capacite du catalogue BallData
};

// Registre du generateur q3/q4 (copie scalaire des compteurs v8, sommes sur
// tous les workers ; masses de natures differentes, a ne jamais additionner).
struct GeneratorLedger {
  std::uint64_t expanded_pairs, cover_builds, cover_sites, cover_node_visits, q3_edges, q4_edges, both_edges, witness_input_pair_mass, witness_rejected_rectangles, witness_rejected_pairs;
  std::uint64_t q3_seeds, q3_ball_builds, q3_depth_rejections, q3_census_bounds, q3_census_point_tests, q3_atlas_edges, q3_atlas_locations, q3_atlas_rejections, q3_atlas_outside_domain;
  std::uint64_t atlas_cells, atlas_leaf_cells, atlas_deep_cells, atlas_outside_cells, atlas_splits, atlas_node_visits, atlas_block_bounds, atlas_point_tests, atlas_ids_copied;
  std::uint64_t q4_seeds, q4_live_leaves, q4_whole_atlas_skips, q4_sweep_events;
  std::uint64_t q3_leaf_censuses, q3_leaf_point_tests, q3_leaf_rejections, q3_lower_bound_fallbacks;
  std::uint64_t dead_loads, dead_form_sites, dead_cells, dead_outside_cells, dead_deep_cells, dead_failed_cells,
      dead_uniform_tests, dead_point_tests, dead_q3_proved, dead_q3_open, dead_q4_proved, dead_q4_open;
  std::uint64_t witness_cache_queries, witness_cache_node_tests, witness_cache_rejected_pairs;
  std::uint64_t core_builds, core_sites, core_closed_edges, dead_core_loads, dead_core_form_sites, dead_core_cells,
      dead_core_uniform_tests, dead_core_point_tests, dead_core_q3_proved, dead_core_q3_open, dead_core_q4_proved,
      dead_core_q4_open;
  std::uint64_t core_cover_node_visits, core_cover_bound_tests, core_cover_point_tests, dead_core_outside_cells,
      dead_core_deep_cells, dead_core_failed_cells;
};

struct OrderSummary {
  unsigned k = 0;
  std::uint64_t nodes = 0, births = 0, merges = 0, contributions = 0, parents = 0;
};

struct ChainResult {
  ChainStatus status = ChainStatus::kInvalidInput;
  std::string reason = "chain_uninitialized";
  unsigned kmax_effective = 0;
  int tower_static_threads = 0;  // voie de resolution effective de la tour
  std::uint64_t sites = 0;
  ChainTimes times;
  CatalogueStats catalogue;
  // Registres du generateur (copies scalaires utiles au grand-livre).
  std::uint64_t q2_front_rectangles = 0, q2_candidate_pairs = 0, q2_accepted_pairs = 0;
  std::uint64_t q34_expanded_pairs = 0, q34_cover_builds = 0, q3_emitted = 0, q4_emitted = 0;
  GeneratorLedger ledger{};
  tower::FullBallStats tower_stats;
  std::vector<OrderSummary> orders;
  // Condense FNV-1a 64 d'un encodage canonique de toute la tour (tous ordres,
  // noeuds, parents, contributions, populations en PointId, verticales).
  std::uint64_t tower_digest = 0;
  // Tour complete si status == kComplete et run_tower : proprietaire du
  // resultat (le catalogue et l'index sont liberes avant publication).
  tower::FullBallTowerResult tower;
  // Catalogue recoupe (indices geometriques de l'index de la tour), si demande.
  std::vector<tower::BallData> catalogue_balls;
  // Key ranges of the parallel presentation sort (1 below 4 096 presentations).
  std::size_t presentation_ranges = 0;
};

// points[i] a l'identite i (PointId = rang d'entree). Sites distincts requis.
ChainResult run_tower_chain(std::span<const gen::Point3> points, const ChainOptions& options);

// Condense canonique d'une tour publiee (independant de l'ordre du catalogue).
std::uint64_t tower_digest(const tower::FullBallTowerResult& tower);

}  // namespace mhgp9
