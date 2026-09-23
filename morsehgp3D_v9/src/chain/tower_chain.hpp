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
  // MEB propose par Welzl en double et verifie exactement dans la tour
  // (anchor_meb_proposed), meme objet. Contrat v9 : active par defaut.
  bool tower_meb_proposal = true;
  // Ordonnancement q3/q4 (meme objet) : jobs du front prepares en scindant
  // d'abord le produit le plus massif, puis reclames par masse decroissante ;
  // grain fin (64 jobs par fil au lieu de 16). Contrat v9 : actives par
  // defaut, publies et epingles (reçu G4 R8 : fils q34 affames a W48).
  bool q34_jobs_by_mass = true;
  bool q34_fine_jobs = true;
  // Tour statique : phase A de chaque ordre lancee des que sa phase 0 est
  // faite (phase 0 par K decroissant), meme objet. Contrat v9 : actif par
  // defaut, publie et epingle.
  bool tower_overlap_static = true;
  // Ordonnancement q2 (meme objet) : plan de jobs du front prepare par masse
  // decroissante et 64 jobs par fil (le plus long job faisait tout q2).
  bool q2_jobs_by_mass = true;
  // v9 S2 : filtre temoin q3/q4 par lots (gen::run_wspd_q34_batched). Le
  // front ne fait que collecter ses rectangles, un appel decide tous les
  // rectangles puis toutes les paires sans cache, les ouvriers traitent les
  // seules paires survivantes. Meme objet (memes decisions que le moteur).
  // q34_gpu_filter execute cet appel sur le GPU (build MHGP9_ENABLE_CUDA) et
  // exige q34_batch_filter ; sans GPU la chaine refuse explicitement.
  // Desactives par defaut.
  bool q34_batch_filter = false;
  bool q34_gpu_filter = false;
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

enum class EulerStatus { kNotCheckable, kHolds, kFails };
const char* euler_status_name(EulerStatus status);

struct CatalogueStats {
  std::uint64_t q2_presentations = 0, q3_presentations = 0, q4_presentations = 0;
  std::uint64_t unique_keys = 0, balls = 0, extra_shell_balls = 0;
  std::uint64_t shell_over_cap = 0, max_shell = 0, max_interior = 0;
  std::uint64_t census_nodes = 0, census_leaf_tests = 0;
  std::array<std::uint64_t, 5> balls_by_qmin{};
  std::array<std::uint64_t, 17> balls_by_shell{};  // index = taille de coquille (16 = 16 et plus)
  std::uint64_t bytes = 0;  // capacite du catalogue BallData
  // Invariant d'Euler par ordre (note C, preuve par le nerf de B, sans
  // position generale) : pour K <= euler_checkable_max_k = min(Kmax-2, n),
  // n*[K=1] + somme des contributions des boules du catalogue vaut 1 si le
  // catalogue est complet. Contribution d'une boule : coefficient de t^{K-1}
  // dans t^p * somme_{T : centre dans conv(T)} (t-1)^{|T|-1}, T parcourant les
  // sous-coquilles (ShellTable) d'une coquille etendue, T = U seule pour une
  // coquille reguliere (support positif). Condition NECESSAIRE seulement : une
  // somme juste ne certifie pas chaque cle. Kmax < 3 : rien de verifiable.
  std::array<std::int64_t, 11> euler_by_k{};  // indice K = 1..10
  unsigned euler_checkable_max_k = 0;
  EulerStatus euler_status = EulerStatus::kNotCheckable;
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
  // Global-index DFS already counted by the generator (audit B, ledger of
  // hidden q3/q4 visits): witness searches of rectangles and pairs, q3 seed
  // search per open q3 edge, q4 positive domain, cover decomposition and
  // LiveOnly seed traversal, plus the repeated active-site sweeps of q4.
  std::uint64_t q34_input_rectangles, witness_rect_queries, witness_rect_node_visits, witness_pair_queries,
      witness_pair_node_visits, q3_edge_queries, q3_seed_node_visits, q3_seed_point_tests, q3_seed_bound_tests,
      q4_geometry_preparations, q4_domain_node_visits, q4_cover_decomposition_node_visits, q4_seed_node_visits,
      q4_seed_cell_queries, q4_sweep_active_sites;
};

// Occupation mesuree des ouvriers q3/q4 (jamais comparee entre executions) :
// fils demarres, murs extremes de leurs boucles, CPU de fil et attente sur la
// file de taches, sommes sur les fils ; taches consommees et attentes.
struct Q34Occupancy {
  std::uint64_t started_workers = 0, jobs = 0, tasks_published = 0, tasks_consumed = 0, task_waits = 0;
  double wall_max_ms = 0, wall_min_ms = 0, cpu_sum_s = 0, wait_sum_s = 0;
  double job_sum_s = 0, max_job_ms = 0;  // mur dans les jobs du front : somme, plus long job
};

// Phases mesurees du chemin q3/q4 par lots (ms, jamais comparees).
struct Q34BatchTimes {
  bool used = false;
  std::string backend;  // "cpu" ou le nom de l'appareil
  double front_ms = 0, filter_ms = 0, edges_ms = 0;
  double device_ms = 0;  // passe GPU mesuree par evenements (0 sur CPU)
  std::uint64_t rectangles = 0, survivors = 0;
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
  tower::FullBallTimes tower_times;  // chronos par phase de la tour (mesures)
  Q34Occupancy q34_occupancy;
  Q34BatchTimes q34_batch;
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
