// MorseHGP3D v6 — le pipeline complet, en bibliotheque : nuage -> foret HGP.
//
//   index -> generation (front fusionne, trois lanes) -> RLE -> prefiltre de
//   profondeur -> census -> expansion des plateaux -> folds par K, STREAMES.
//
// Chaque fold est construit, signe, compte puis LIBERE avant le suivant :
// la residence stationnaire est bornee par `fold_inflight + 1` ordres (jamais
// dix forets residentes), le transitoire de fusion des shards par
// `fold_inflight + 2` — le facteur du budget partiel. Statuts transactionnels :
//   complete_regular | unsupported_degeneracy | resource_exhausted |
//   invalid_input | invariant_violated.
// Le consommateur recoit chaque ForestResult par callback (`on_forest`),
// PROVISOIRE jusqu'au statut terminal ; la signature au format v4 est
// calculee ici pour la porte de conformite v5 ≡ v6.
//
// EPUISEMENT MEMOIRE : `run_pipeline` enrobe le corps du pipeline et convertit
// un `std::bad_alloc` en refus transactionnel `resource_exhausted` qui NOMME
// l'etage atteint (curseur `RunResult::stage_reached`, avance aux memes points
// que les `rss_mb`), sans jamais publier un prefixe de payload. Ce n'est pas
// une garantie anti-OOM : voir le commentaire de l'enrobage.
//
// GRAND-LIVRE GLOBAL DU FRONT FUSIONNE (remplace le ledger postsep v5, la v6
// n'ayant pas de raffinement post-separation) : par lane,
//   masses emises + masses tuees == C(n,2) − Σ C(mult_u, 2)
// — verifie ici avant toute publication, violation = invariant_violated.
#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <deque>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "../core/mutants.hpp"
#include "../core/caps.hpp"
#include "../tree/cloud_index.hpp"
#include "../forest/silent_incidence.hpp"
#include "digest.hpp"
#include "expand.hpp"
#include "generate.hpp"

namespace mhgp7 {

// Phases de l'etage B d'un ordre K, OBSERVEES par `RunOptions::on_fold_phase`
// (instrument, jamais une garde).
enum class FoldPhase : u8 {
  kStageABegin,
  kStageAFailed,
  kReduceBegin,
  kReduceEnd,
  kReduceFailed,
  kPublished,
  kNotPublished
};

inline constexpr int kFoldInflightMax = 16;

struct RunOptions {
  i64 s = 8;
  u64 smax = 11;
  int threads = 1;
  size_t shell_cap = 12;
  bool digest = false;
  // Separate scientific object, opt-in while the normalized H0 contract is
  // being qualified. The v6 compatibility route stays explicitly available.
  bool complete_silent_incidence = false;
  SilentIncidenceLimits silent_limits;
  // Plafond d'emission des candidats bruts (caps.hpp) — transmis a la
  // generation ; abaissable, jamais au-dela du structurel.
  u64 max_raw_candidates = kMaxRawCandidates;
  // Budget PARTIEL DE TAMPONS DECLARES, optionnel (octets ; 0 = desactive).
  // Contrat exact : CAP DE CARDINALITE A OVERSHOOT BORNE sur l'emission
  // (arret cooperatif AVANT la fusion globale et le tri — les shards locaux
  // materialisent jusqu'a l'observation du drapeau), et refus
  // resource_exhausted des tampons NOMMES (tri x2, prefiltre/census
  // conservatif S <= C sur les TAILLES exactes avec BallData x2, evenements
  // du fold x (inflight+2)) — PROXY DE PAYLOAD LOGIQUE NOMME, pas une
  // promesse anti-OOM globale.
  u64 memory_budget_bytes = 0;
#if defined(MHGP7_TESTING)
  // Caps abaissables en test (0 = structurel) : branches de refus du front
  // fusionne et instant PRE-insertion exerces a petit n.
  u64 wave_tasks_cap_for_tests = 0;
  u64 alive_rects_cap_for_tests = 0;
#endif
  // Diagnostic opt-in : signe le multiensemble trie AVANT RLE.
  bool diagnostic_raw_candidates_digest = false;
  int fold_inflight = 2;
  // Joindre B(K) AVANT de preparer A(K+1) (§ 5.10). fold_inflight=1 seul
  // n'isole PAS B : la preparation suivante peut co-tourner. Avec threads=1,
  // B est appele inline et ses callbacks s'executent sur le thread appelant.
  // Sinon B conserve son thread. Objet, ordre de publication et autorite
  // provisoire des callbacks sont inchanges ; declarer cet ordonnancement
  // dans toute mesure (le mode avec recouvrement reste le defaut).
  bool fold_join_before_next_k = false;
  // ROUTE DE STOCKAGE des deltas (palier KeyCSR) : classic (defaut) | csr —
  // meme objet, meme digest ; AUCUNE route de repli (un echec csr est un refus
  // transactionnel, csr_fallback est mesure et vaut 0 par construction).
  ForestLayout forest_layout = ForestLayout::kClassic;
  // Sonde E6 opt-in (--sonde-e6) : lecture seule, objet inchange.
  bool e6_probe = false;
  // Experimentation E3/G16 par bras (kOff = production).
  E3G16Mode e3_mode = E3G16Mode::kOff;
  std::function<void(u64 K, FoldPhase phase)> on_fold_phase;
  size_t pretest_query_min_points = 512;
  size_t cell_grid_min_sites = kCellGridMinSites;
  std::function<void(u64 K, const std::vector<ForestEvent>& events, const ForestResult& r)> on_forest;
  // COUTURE SERIE C (C5, docs/GPU.md) : substitution du PREFILTRE+CENSUS par
  // un executeur externe (GPU — ou son stub hote pour la preuve locale).
  // Recoit l'index, les candidats uniques, smax_eff, shell_cap ; REMPLIT
  // survivants (ordre des candidats) et BallData (ordre des survivants,
  // n_int == depth croise comme la route CPU) et les stats de census
  // fournies par la route. Rend "" ou un message de REFUS transactionnel
  // (mappe resource_exhausted — lots et prefixes jetes, jamais un prefixe
  // publie). nullptr = route CPU de production, STRICTEMENT inchangee.
  // L'OBJET aval est le meme ; la porte pilote prouve les digests egaux.
  std::function<std::string(const CloudIndex&, const std::vector<BallCandidate>&, u64 smax_eff,
                            size_t shell_cap, std::vector<Survivor>*, std::vector<BallData>*, ExpandStats*)>
      prefilter_census_override;
};

struct KCardinalities {
  u64 events = 0, facets = 0, deltas = 0, attachments = 0, fusions = 0, nodes = 0;
  bool operator==(const KCardinalities&) const = default;  // comparaison large de la fenetre (d)
};

// CURSEUR D'ETAGE du pipeline (alerte G4 du 2 septembre : un `std::bad_alloc`
// generique ne nomme pas l'etage qui deborde, et les RSS par etage ne sont
// imprimes qu'apres SUCCES). Le curseur nomme l'etage EN COURS : il est
// avance aux MEMES points que les mesures `rss_mb`, chacune prise a la FIN
// d'un etage — donc a l'instant ou l'etage suivant commence. Il n'est JAMAIS
// reconstruit apres coup. Seule exception : `entree -> generation`, avance
// juste avant `generate_candidates` (l'entree n'a pas de mesure rss). La
// couture serie C (prefilter_census_override) est UN SEUL aller-retour : le
// curseur y reste `prefiltre`, les deux etages n'y sont pas separables.
enum : u8 {
  kRunStageEntree = 0,       // validation des options, index du nuage
  kRunStageGeneration = 1,   // front fusionne, trois lanes  (clos par rss_mb[0])
  kRunStageRle = 2,          // tri + deduplication des candidats (rss_mb[1])
  kRunStagePrefiltre = 3,    // prefiltre de profondeur (rss_mb[2])
  kRunStageCensus = 4,       // census I_B / U_B + plateaux (rss_mb[3])
  kRunStageFold = 5,         // comptage, expansion, folds par K (rss_mb[4])
  kRunStagePublication = 6,  // digest global, liberations, retour (rss_mb[5])
  kRunStageCount = 7
};

inline const char* run_stage_name(u8 stage) {
  switch (stage) {
    case kRunStageEntree:
      return "entree";
    case kRunStageGeneration:
      return "generation";
    case kRunStageRle:
      return "rle";
    case kRunStagePrefiltre:
      return "prefiltre";
    case kRunStageCensus:
      return "census";
    case kRunStageFold:
      return "fold";
    case kRunStagePublication:
      return "publication";
    default:
      return "inconnu";
  }
}

struct RunResult {
  PipelineStatus status = PipelineStatus::kCompleteRegular;
  std::string message;
  // Etage ATTEINT (en cours) au moment du retour : diagnostic, jamais un
  // payload — il survit a un refus avec le statut, le message, les chronos et
  // les RSS d'etage, et rien d'autre.
  u8 stage_reached = kRunStageEntree;
  u64 smax_eff = 0, kmax_eff = 0;
  size_t emitted = 0;
  GenerateStats gen;
  ExpandStats expand;
  std::vector<KCardinalities> cards;  // indexee par K
  u64 total_facets = 0, total_fusions = 0, total_deltas = 0, total_nodes = 0, total_events = 0;
  // digest_balls = digest_candidates_v5_compat (tag v4, candidats uniques
  // post-RLE : diagnostic differentiel de generation, PAS l'objet) ;
  // digest_postprefilter = mhgp6-digest-v1:postprefilter-candidates (les
  // survivants du prefiltre exact : non-regression interne v6).
  std::string digest_raw_candidates, digest_balls, digest_postprefilter, digest_all;
  std::vector<std::string> digest_forest;  // indexee par K
  double t_index_ms = 0, t_gen_ms = 0, t_rle_ms = 0, t_prefilter_ms = 0, t_census_ms = 0, t_expand_ms = 0,
         t_fold_ms = 0, t_count_ms = 0, t_digest_ms = 0;
  double t_fold_sort_ms = 0, t_fold_intern_ms = 0, t_fold_merge_ms = 0, t_fold_reduce_ms = 0;
  double t_fold_wall_ms = 0;
  double t_silent_ms = 0;
  std::vector<SilentIncidenceStats> silent_stats;  // diagnostic work paid, per K
  double rss_mb[6] = {0, 0, 0, 0, 0, 0};
  // PIC HISTORIQUE de residence (VmHWM de /proc/self/status), releve aux MEMES
  // frontieres d'etage que `rss_mb` — releve, jamais une garde. Les `rss_mb`
  // sont des instantanes : ils manquent structurellement tout pic NE et MORT
  // entre deux jalons (fusion des shards, tri des candidats, transitoire du
  // census). VmHWM est monotone (le noyau ne le redescend jamais) et domine le
  // resident : c'est ce que la porte de RESIDENCE verifie. RESERVE MESUREE le
  // 2 septembre, et non supposee : la lecture de VmHWM peut RETARDER de
  // quelques centaines de kilo-octets sur celle de statm (compteurs de RSS par
  // tache synchronises par lots), donc `hwm_mb[j] >= rss_mb[j]` ne vaut qu'a
  // cette tolerance pres — la porte la rend explicite et publie le retard.
  // hwm_mb[2] n'est releve que sur la route CPU (comme rss_mb[2] : la couture
  // serie C ne separe pas prefiltre et census).
  double hwm_mb[6] = {0, 0, 0, 0, 0, 0};
  // COMPTEURS DE FORME supposes par les conceptions et jamais mesures :
  //  - `plateau_balls` : boules dont la coquille depasse l'arite du candidat
  //    (n_shell != arity), donc celles qui passent par expand_plateau ;
  //    `census_balls` est le denominateur de la fraction.
  //  - `sum_parents_by_k` : Sigma|parents| des deltas emis a l'ordre K. La
  //    valeur EXISTAIT deja, enfouie dans la signature de stockage
  //    (`stockage_foret ... cles_parents=`) ; elle est ici un compteur de
  //    premier rang, avec son TOTAL (qui, lui, n'existait nulle part).
  u64 plateau_balls = 0, census_balls = 0;
  std::vector<u64> sum_parents_by_k;  // indexe par K (0 inutilise)
  u64 sum_parents_total = 0;
  double t_total_ms = 0;
  u64 fold_workers = 0, rle_workers = 0;
  // DIAGNOSTIC pur (jamais un payload, jamais invalide, AUCUNE autorite de
  // seuil) : capacite observee du vecteur de candidats, saisie AVANT le tri
  // et conservee a travers le RLE.
  u64 diag_candidates_capacity = 0;
  u64 peak_fold_inflight = 0;  // cycle de vie des workers B (reduction + digest + attente de publication + callback)
  // STOCKAGE DES FORETS (palier KeyCSR) : signe par K a la publication, PROVISOIRE
  // (efface par invalidate_provisional). csr_fallback est MESURE : aucune route de repli n'existe.
  struct ForestStorageStats {
    u8 kind = 0;  // ForestStorageKind construit
    u64 deltas = 0, keys_parents = 0, keys_born = 0;
    u64 meta_size = 0, meta_capacity = 0, offsets_size = 0, offsets_capacity = 0;
    u64 parents_size = 0, parents_capacity = 0, born_size = 0, born_capacity = 0;
    u64 csr_capacity_growths = 0, bytes_owned = 0;  // growths : csr seulement (classic non instrumente = 0)
    u64 parents_off_back = 0, born_off_back = 0;    // csr : *_off.back() LUS (temoin independant des tailles d'arene)
    bool bytes_exact = false;  // classic : borne inferieure (vecteurs internes non parcourus)
  };
  ForestLayout forest_layout = ForestLayout::kClassic;  // demande
  u64 csr_fallback = 0, forest_storage_conformes = 0;
  std::vector<ForestStorageStats> forest_storage;  // indexe par K (0 inutilise)
#ifdef MHGP7_PROFILE_REDUCE
  // § 5.10 : records draines sans I/O D'IMPRESSION du profil (l'impression
  // dans le worker serialisait la publication et contaminait le mur) —
  // imprimes par print_run APRES le retour de run_pipeline. Le worker
  // execute toujours le callback et lit /proc/self/statm sous pub_mutex :
  // ne jamais ecrire « aucune I/O dans les workers ».
  std::vector<ReduceProfile> fold_profiles;  // indexes par K (0 inutilise)
  u64 peak_reduce_active = 0;                // pic STRICTEMENT autour de reduce_fold (chevauchement B×B prouve)
  std::chrono::steady_clock::time_point fold_epoch{};  // origine des intervalles debut/fin
  bool profile_join = false;                 // fold_join du run, signe dans la sortie
#endif
};

namespace run_detail {

inline double rss_mb_now() {
  std::FILE* f = std::fopen("/proc/self/statm", "r");
  if (!f) return 0.0;
  unsigned long long size = 0, resident = 0;
  const int got = std::fscanf(f, "%llu %llu", &size, &resident);
  std::fclose(f);
  if (got != 2) return 0.0;
  return (double)resident * 4096.0 / (1024.0 * 1024.0);
}

// PIC HISTORIQUE du RSS : VmHWM de /proc/self/status, en Mo. Contrairement a
// /proc/self/statm (resident INSTANTANE), le noyau ne le redescend jamais : il
// capture les pics qui naissent et meurent entre deux jalons. 0 si la ligne est
// absente (noyau sans VmHWM) — la porte de residence traite 0 comme un plancher
// viole, jamais comme un vert.
// Mutant `hwm-instant-rss` : rend l'instantane a la place du pic — le defaut
// historique que ces six jalons avaient exactement (tue par la porte de
// residence : hwm cesse d'etre non decroissant des que le fold se libere).
inline double vm_hwm_mb_now() {
  if (MHGP7_MUTANT("hwm-instant-rss")) return rss_mb_now();
  std::FILE* f = std::fopen("/proc/self/status", "r");
  if (!f) return 0.0;
  char line[256];
  double mb = 0.0;
  while (std::fgets(line, sizeof line, f)) {
    if (std::strncmp(line, "VmHWM:", 6) != 0) continue;
    unsigned long long kb = 0;
    if (std::sscanf(line + 6, "%llu", &kb) == 1) mb = (double)kb / 1024.0;
    break;
  }
  std::fclose(f);
  return mb;
}

// Noms des SIX frontieres d'etage ou `rss_mb` et `hwm_mb` sont releves —
// source unique, partagee par print_run et par la porte de residence (une
// route qui ajoute un jalon change ce tableau et rien d'autre).
inline constexpr const char* kResidenceStageLabel[6] = {"apres_generation", "apres_rle", "apres_prefiltre",
                                                        "apres_census",     "max_fold",  "fin"};

inline double ms(std::chrono::steady_clock::time_point t0) {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}
}  // namespace run_detail

inline constexpr u64 kSmaxProfile = 11;

// CONTRAT DE PAYLOAD (arbitrage V3, inchange en doctrine) : forets
// horizontales par ordre K, retention de toutes les facettes, autorite au
// statut terminal, digest canonique au format `mhgp4-digest-v1`.
inline constexpr const char* kForestPayloadVersion = "mhgp7-forests-horizontal-v1";

inline constexpr size_t kShellCapProfile = kBallShellMax;

inline bool validate_run_options(const std::vector<InputPoint>& in, const RunOptions& opt, std::string* why) {
  if (opt.max_raw_candidates == 0 || opt.max_raw_candidates > kMaxRawCandidates) {
    *why = "invalid_input : max_raw_candidates hors de [1, 2^32-1]";
    return false;
  }
  if (in.size() < 2) {
    *why = "invalid_input : moins de deux points";
    return false;
  }
  if (opt.s < kSeparationProfileMin) {
    *why = "invalid_input : separation s < 8";
    return false;
  }
  if (opt.smax < 2 || opt.smax > kSmaxProfile) {
    *why = "invalid_input : smax hors du profil [2, 11]";
    return false;
  }
  if (opt.threads < 1) {
    *why = "invalid_input : threads < 1";
    return false;
  }
  if (opt.fold_inflight < 1 || opt.fold_inflight > kFoldInflightMax) {
    *why = "invalid_input : fold_inflight hors du domaine [1, " + std::to_string(kFoldInflightMax) + "]";
    return false;
  }
  if (opt.shell_cap < 4 || opt.shell_cap > kShellCapProfile) {
    *why = "invalid_input : plafond de coquille hors du profil [4, 12]";
    return false;
  }
  return true;
}

// CAP EFFECTIF d'emission : structurel, ET derive du budget partiel quand
// il est declare (division, jamais un produit non controle) — HELPER UNIQUE
// employe par l'execution ET par la signature CLI du budget.
inline u64 effective_raw_cap(const RunOptions& opt) {
  u64 cap = opt.max_raw_candidates;
  if (opt.memory_budget_bytes != 0) {
    const u64 from_budget = opt.memory_budget_bytes / (u64)sizeof(BallCandidate);
    if (from_budget < cap) cap = from_budget == 0 ? 1 : from_budget;
  }
  return cap;
}

// CAP DE FUSION BUDGETAIRE (§ 5.9, 5e contre-lecture) : plus grande emission
// E que la garde 2E de la fusion globale accepte sous le budget declare —
// budget / (2 x 144), la MEME division que `fits_budget(E, sizeof, 2,
// budget)` executee ; publie par la signature CLI sous le nom
// `cap_fusion_budgetaire` (PAS « effectif » : un cap brut demande plus bas
// peut borner le run avant cette garde ; 0 = pas de budget).
inline u64 budget_fusion_cap(const RunOptions& opt) {
  if (opt.memory_budget_bytes == 0) return 0;
  return opt.memory_budget_bytes / (2 * (u64)sizeof(BallCandidate));
}

// ROUTINE TERMINALE COMMUNE (P1 audit du 31 aout) : sur TOUT retour non
// complet, les champs provisoires sont vides — digests, forets, cartes,
// totaux. Aucun defaut ne laisse une foret K1 ou un digest raw visibles.
inline void invalidate_provisional(RunResult* rr) {
  rr->digest_raw_candidates.clear();
  rr->digest_balls.clear();
  rr->digest_postprefilter.clear();
  rr->digest_all.clear();
  rr->digest_forest.clear();
  rr->cards.clear();
  rr->total_events = rr->total_facets = rr->total_fusions = rr->total_deltas = rr->total_nodes = 0;
  rr->forest_storage.clear();  // signatures de stockage : provisoires comme les cartes
  // Sigma|parents| DECRIT le payload (cardinalite des deltas publies) : efface
  // comme les cartes. `plateau_balls` / `census_balls` decrivent le census, au
  // meme titre que `expand` et les `rss_mb` — diagnostic conserve.
  // Mutant `provisional-keep-sum-parents` : ne l'efface plus. Un refus a K=2
  // laisserait alors visible le Sigma|parents| de la foret K=1 — un PREFIXE
  // EXACT de payload publie, ce que la doctrine interdit (tue par la
  // projection `provisoires_vides` de selftest.cpp et bad_alloc_gate.cpp).
  if (!MHGP7_MUTANT("provisional-keep-sum-parents")) {
    rr->sum_parents_by_k.clear();
    rr->sum_parents_total = 0;
  }
  rr->csr_fallback = 0;
  rr->forest_storage_conformes = 0;
#ifdef MHGP7_PROFILE_REDUCE
  // § 5.10 (contre-lecture 7ec81064) : une panne B ne rouvre jamais un canal
  // provisoire par les records de profil — vides sur tout retour non complet.
  rr->fold_profiles.clear();
  rr->peak_reduce_active = 0;
#endif
}

namespace run_detail {

// CORPS du pipeline. Ecrit dans `rr` au fil de l'eau (curseur d'etage compris)
// et peut PROPAGER une exception : c'est l'enrobage `run_pipeline` qui decide
// du sort d'un `std::bad_alloc`. Toute autre exception continue de sortir
// telle quelle (first_exc / main_exc), comportement inchange.
inline void run_pipeline_into(const std::vector<InputPoint>& in, const RunOptions& opt, RunResult& rr) {
  using run_detail::ms;
  const auto t_all = std::chrono::steady_clock::now();
  if ((u64)in.size() > kMaxTreePositions) {
    rr.status = PipelineStatus::kResourceExhausted;
    rr.message = "resource_exhausted : plus de 2^30-1 positions d'entree (arbre radix a indices i32, recherche de Karras)";
    return;
  }
  if (!validate_run_options(in, opt, &rr.message)) {
    rr.status = PipelineStatus::kInvalidInput;
    return;
  }
  if (opt.memory_budget_bytes != 0 && opt.memory_budget_bytes < (u64)sizeof(BallCandidate)) {
    rr.status = PipelineStatus::kResourceExhausted;
    rr.message = "resource_exhausted : budget partiel inferieur au cout d'un seul candidat (" +
                 std::to_string(sizeof(BallCandidate)) + " octets)";
    return;
  }
  const auto t_ix = std::chrono::steady_clock::now();
  const CloudIndex ix = build_cloud_index(in);
  rr.t_index_ms = ms(t_ix);
  if (!ix.valid) {
    rr.status = PipelineStatus::kInvalidInput;
    rr.message = "invalid_input : coordonnee hors profil u16 ou PointId duplique";
    return;
  }
  if (ix.has_duplicate_positions()) {
    rr.status = PipelineStatus::kUnsupportedDegeneracy;
    rr.message = "unsupported_degeneracy : positions dupliquees";
    return;
  }
  rr.smax_eff = std::min<u64>(opt.smax, in.size());
  rr.kmax_eff = rr.smax_eff - 1;

  const auto t_g = std::chrono::steady_clock::now();
  std::vector<BallCandidate> cands;
  GenerateOptions go;
  go.pretest_query_min_points = opt.pretest_query_min_points;
  go.cell_grid_min_sites = opt.cell_grid_min_sites;
  go.s = opt.s;
  go.smax = rr.smax_eff;
  go.threads = opt.threads;
  go.e6_probe = opt.e6_probe;
  go.e3_mode = opt.e3_mode;
  // CAP EFFECTIF d'emission par le HELPER PARTAGE (la signature CLI du
  // budget imprime la MEME valeur que celle executee).
  go.max_raw_candidates = effective_raw_cap(opt);
  go.memory_budget_bytes = opt.memory_budget_bytes;  // garde 2E de la fusion globale
#if defined(MHGP7_TESTING)
  go.wave_tasks_cap_for_tests = opt.wave_tasks_cap_for_tests;
  go.alive_rects_cap_for_tests = opt.alive_rects_cap_for_tests;
#endif
  rr.stage_reached = kRunStageGeneration;
  generate_candidates(ix, go, &cands, &rr.gen);
  rr.t_gen_ms = ms(t_g);
  // REFUS DE CAPACITE DE LA GENERATION (caps.hpp) : mappe AVANT le
  // grand-livre (un flux volontairement arrete n'a pas a fermer les
  // identites de masse).
  if (rr.gen.cap_refus != kCapRefusNone) {
    rr.status = PipelineStatus::kResourceExhausted;
    switch (rr.gen.cap_refus) {
      case kCapRefusRawCandidates:
        rr.message = "resource_exhausted : candidats bruts au-dela du plafond declare (arret avant fusion globale et tri)";
        break;
      case kCapRefusWaveTasks:
        rr.message = "resource_exhausted : taches de vague au-dela du plafond declare (front fusionne)";
        break;
      case kCapRefusAliveRects:
        rr.message = "resource_exhausted : rectangles vivants au-dela du plafond declare (front fusionne)";
        break;
      case kCapRefusFusionBudget:
        rr.message = "resource_exhausted : fusion globale des candidats hors budget partiel declare (payload logique nomme 2E : shards + sortie)";
        break;
      default:  // code inconnu = faute d'implementation, jamais confondu avec un refus nomme (§ 5.9)
        rr.status = PipelineStatus::kInvariantViolated;
        rr.message = "invariant : code de refus de capacite inconnu (" + std::to_string(rr.gen.cap_refus) + ")";
        break;
    }
    invalidate_provisional(&rr);
    return;
  }
  rr.rss_mb[0] = run_detail::rss_mb_now();
  rr.hwm_mb[0] = run_detail::vm_hwm_mb_now();
  rr.stage_reached = kRunStageRle;
  // GRAND-LIVRE GLOBAL : par lane du profil, emis + tues == masse attendue.
  {
    const u128 expected = expected_pair_mass(ix);
    u64 violations = 0;
    for (int q = 0; q < 3; ++q) {
      const u128 accounted = rr.gen.ledger_emitted_mass[q] + rr.gen.ledger_killed_mass[q];
      if (accounted != expected) ++violations;
    }
    if (violations) {
      rr.status = PipelineStatus::kInvariantViolated;
      rr.message = "invariant : grand-livre des masses de paires du front fusionne viole (" +
                   std::to_string(violations) + " lane(s))";
      invalidate_provisional(&rr);  // finaliseur LITTERAL sur toute sortie non complete (audit du 31 aout)
      return;
    }
  }
  if (rr.gen.invariant_jneg) {
    rr.status = PipelineStatus::kInvariantViolated;
    rr.message = "invariant : seed q4 aigu avec J < 0 (inatteignable par theoreme, MATHEMATIQUES § 2) : " +
                 std::to_string(rr.gen.invariant_jneg) + " occurrence(s)";
    invalidate_provisional(&rr);
    return;
  }
  const auto t_r = std::chrono::steady_clock::now();
  rr.emitted = cands.size();
  // Capacite OBSERVEE, exposee en DIAGNOSTIC seulement (aucune autorite de
  // seuil : le budget partiel est un proxy de PAYLOAD LOGIQUE, ses gardes
  // portent sur les tailles — C++20 ne promet que capacity() >= exact).
  rr.diag_candidates_capacity = (u64)cands.capacity();
  if (opt.memory_budget_bytes != 0 &&
      !fits_budget((u64)cands.size(), (u64)sizeof(BallCandidate), 2, opt.memory_budget_bytes)) {
    rr.status = PipelineStatus::kResourceExhausted;
    rr.message = "resource_exhausted : tri des candidats hors budget partiel declare (tampon de fusion 2 x " +
                 std::to_string(sizeof(BallCandidate)) + " octets x " + std::to_string(cands.size()) + ")";
    invalidate_provisional(&rr);
    return;
  }
  rr.rle_workers = sort_candidates(&cands, opt.threads);
  const double t_sort_candidates_ms = ms(t_r);
  if (opt.diagnostic_raw_candidates_digest) {
    const auto t_d = std::chrono::steady_clock::now();
    rr.digest_raw_candidates = digest_raw_candidates_v6(cands);
    rr.t_digest_ms += ms(t_d);
  }
  const auto t_u = std::chrono::steady_clock::now();
  deduplicate_candidates(&cands);
  rr.t_rle_ms = t_sort_candidates_ms + ms(t_u);
  // Mutant `drop-stage-milestone` : le jalon apres_rle n'est plus releve DU
  // TOUT (ni instantane, ni pic) — le defaut qu'une route future (serie C,
  // GPU) introduirait par accident. Sans plancher de JALONS JUGES, la porte
  // de residence saute simplement le jalon et reste verte : c'est ce que ce
  // mutant interdit.
  if (!MHGP7_MUTANT("drop-stage-milestone")) {
    rr.rss_mb[1] = run_detail::rss_mb_now();
    rr.hwm_mb[1] = run_detail::vm_hwm_mb_now();
  }
  rr.stage_reached = kRunStagePrefiltre;
  rr.expand.unique_balls = cands.size();

  if (!candidates_capacity_ok(cands.size())) {
    rr.status = PipelineStatus::kResourceExhausted;
    rr.message = "resource_exhausted : plus de 2^32-1 boules uniques (indices u32 du prefiltre)";
    invalidate_provisional(&rr);
    return;
  }
  const auto t_p = std::chrono::steady_clock::now();
  std::vector<Survivor> surv;
  if (opt.memory_budget_bytes != 0 &&
      !fits_budget((u64)cands.size(),
                   (u64)sizeof(BallCandidate) + (u64)sizeof(Survivor) + 2 * (u64)sizeof(BallData), 1,
                   opt.memory_budget_bytes)) {
    rr.status = PipelineStatus::kResourceExhausted;
    rr.message = "resource_exhausted : prefiltre/census hors budget partiel declare (borne conservative "
                 "S <= C sur la TAILLE post-RLE — proxy de payload logique, coexistence BallData x2)";
    invalidate_provisional(&rr);
    return;
  }
  std::vector<BallData> balls;
  if (opt.prefilter_census_override) {
    // Route serie C : le temps combine tombe dans t_census_ms (t_prefilter
    // reste 0 — semantique documentee de la couture, les deux etages sont un
    // seul aller-retour device).
    const std::string err =
        opt.prefilter_census_override(ix, cands, rr.smax_eff, opt.shell_cap, &surv, &balls, &rr.expand);
    if (!err.empty()) {
      // § 5.11 : la CLASSE du refus est preservee — un invalid_input du wire
      // n'est jamais requalifie silencieusement en resource_exhausted.
      const bool inv = err.rfind("invariant", 0) == 0;
      const bool bad_in = err.rfind("invalid_input", 0) == 0;
      rr.status = inv ? PipelineStatus::kInvariantViolated
                      : bad_in ? PipelineStatus::kInvalidInput : PipelineStatus::kResourceExhausted;
      rr.message = std::string(inv ? "invariant" : bad_in ? "invalid_input" : "resource_exhausted") +
                   " : route serie C — " + err;
      invalidate_provisional(&rr);
      return;
    }
    rr.t_census_ms = ms(t_p);
  } else {
    prefilter_balls(ix, cands, rr.smax_eff, opt.threads, &surv, &rr.expand);
    rr.t_prefilter_ms = ms(t_p);
    rr.rss_mb[2] = run_detail::rss_mb_now();
    rr.hwm_mb[2] = run_detail::vm_hwm_mb_now();
    rr.stage_reached = kRunStageCensus;
    const auto t_c = std::chrono::steady_clock::now();
    // Panne d'allocation INJECTEE a l'etage census : l'enrobage doit la
    // convertir en refus transactionnel NOMMANT l'etage, jamais laisser
    // l'exception terminer le processus (code 134).
    if (MHGP7_MUTANT("caps-throw-bad-alloc-census")) throw std::bad_alloc();
    const PipelineStatus cs =
        census_balls(ix, cands, surv, rr.smax_eff, opt.shell_cap, opt.threads, &balls, &rr.expand);
    if (cs != PipelineStatus::kCompleteRegular) {
      rr.status = cs;
      rr.message = cs == PipelineStatus::kResourceExhausted
                       ? "resource_exhausted : coquille au-dela du plafond (jamais de troncature)"
                       : "invariant : census contredit la passe count-only";
      invalidate_provisional(&rr);
      return;
    }
    rr.t_census_ms = ms(t_c);  // arrete AVANT le digest post-prefiltre (audit du 31 aout)
  }
  if (opt.digest) {
    const auto t_dp = std::chrono::steady_clock::now();
    rr.digest_postprefilter = digest_postprefilter_v6(cands, surv);
    rr.t_digest_ms += ms(t_dp);
  }
  std::vector<Survivor>().swap(surv);
  // FRACTION DE BOULES A PLATEAU (compteur neuf) : une boule dont la coquille
  // depasse l'arite du candidat (n_shell != arity) est cospherique et passe
  // par expand_plateau — c'est elle qui fait exploser le nombre d'evenements
  // d'un ordre. Un simple balayage O(#boules) des BallData deja resident :
  // aucune structure ajoutee, aucune influence sur l'objet.
  rr.census_balls = (u64)balls.size();
  rr.plateau_balls = 0;
  for (const BallData& bd : balls)
    if ((u64)bd.n_shell != (u64)bd.arity) ++rr.plateau_balls;
  rr.rss_mb[3] = run_detail::rss_mb_now();
  rr.hwm_mb[3] = run_detail::vm_hwm_mb_now();
  if (opt.complete_silent_incidence && rr.plateau_balls != 0) {
    rr.status = PipelineStatus::kUnsupportedDegeneracy;
    rr.message = "unsupported_degeneracy : incidence completion requires no rank-relevant extra-shell";
    invalidate_provisional(&rr);
    return;
  }
  rr.stage_reached = kRunStageFold;

  // GARDES DE CAPACITE DE TOUS LES ORDRES AVANT TOUTE PUBLICATION.
  const auto t_e = std::chrono::steady_clock::now();
  const std::vector<KCount> kc = count_events_by_k(ix, balls, rr.kmax_eff, opt.threads);
  for (u64 K = 1; K <= rr.kmax_eff; ++K) {
    std::string why;
    if (!fold_capacity_ok(kc[K].events, kc[K].incidences, &why)) {
      rr.status = PipelineStatus::kResourceExhausted;
      rr.message = "fold K=" + std::to_string(K) + " : " + why;
      invalidate_provisional(&rr);
      return;
    }
    if (opt.memory_budget_bytes != 0 &&
        !fits_budget(kc[K].events, (u64)sizeof(ForestEvent), (u64)(opt.fold_inflight + 2),
                     opt.memory_budget_bytes)) {
      rr.status = PipelineStatus::kResourceExhausted;
      rr.message = "fold K=" + std::to_string(K) +
                   " : resource_exhausted : evenements hors budget partiel declare (" +
                   std::to_string(kc[K].events) + " x " + std::to_string(sizeof(ForestEvent)) +
                   " octets x (inflight+2, coexistence des shards lev et de la fusion comptee) — "
                   "tampon NOMME, les autres structures du fold ne sont pas comptees)";
      invalidate_provisional(&rr);
      return;
    }
  }
  rr.t_count_ms += ms(t_e);

  const auto t_d0 = std::chrono::steady_clock::now();
  if (opt.digest) rr.digest_balls = digest_balls_v4(cands);
  rr.t_digest_ms += ms(t_d0);
  std::vector<BallCandidate>().swap(cands);
  DigestAll dg_all;
  rr.digest_forest.assign(rr.kmax_eff + 1, std::string());
#ifdef MHGP7_PROFILE_REDUCE
  rr.fold_profiles.assign(rr.kmax_eff + 1, ReduceProfile{});
  rr.profile_join = opt.fold_join_before_next_k;
#endif
  rr.cards.assign(rr.kmax_eff + 1, KCardinalities{});
  rr.silent_stats.assign(rr.kmax_eff + 1, SilentIncidenceStats{});
  rr.forest_layout = opt.forest_layout;
  rr.forest_storage.assign(rr.kmax_eff + 1, RunResult::ForestStorageStats{});
  rr.sum_parents_by_k.assign(rr.kmax_eff + 1, 0);
  rr.sum_parents_total = 0;
  rr.expand.events_by_k.assign(rr.kmax_eff + 1, 0);
  // STREAMING PAR K, PIPELINE A DEUX ETAGES — transcription v5 (suretes 1-3 de
  // l'audit du 28 aout 2026 conservees : possession du slot avant demarrage,
  // jonction explicite avant tout retour, premier defaut dans l'ordre des K).
  struct Stage {
    u64 K = 0;
    std::vector<ForestEvent> events;
    FoldPrepared prep;
#ifdef MHGP7_PROFILE_REDUCE
    std::chrono::steady_clock::time_point a_begin{}, a_end{};  // intervalle de l'etage A (expansion + preparation)
#endif
  };
  struct BSlot {
    std::thread t;
    u64 K = 0;
    bool decided = false;
    std::exception_ptr exc;
    PipelineStatus status = PipelineStatus::kCompleteRegular;
    std::string message;
  };
  const int inflight = opt.fold_inflight;
  std::deque<std::unique_ptr<BSlot>> slots;
  std::mutex pub_mutex;
  std::condition_variable pub_cv;
  u64 next_publish = 1;
  std::atomic<bool> pub_failed{false};
  std::atomic<u64> b_inflight{0}, b_peak{0};
  struct BJoiner {
    std::deque<std::unique_ptr<BSlot>>& s;
    std::mutex& m;
    std::condition_variable& cv;
    std::atomic<bool>& failed;
    ~BJoiner() {
      {
        std::lock_guard<std::mutex> lk(m);
        failed.store(true);
      }
      cv.notify_all();
      for (auto& b : s)
        if (b->t.joinable()) b->t.join();
    }
  } bjoiner{slots, pub_mutex, pub_cv, pub_failed};
  const auto phase = [&](u64 K, FoldPhase p) {
    if (opt.on_fold_phase) opt.on_fold_phase(K, p);
  };
  double t_prepare_total_ms = 0;
  std::exception_ptr first_exc;
  PipelineStatus first_status = PipelineStatus::kCompleteRegular;
  std::string first_message;
  bool have_first = false;
  const auto reap_front = [&]() -> bool {
    std::unique_ptr<BSlot> b = std::move(slots.front());
    slots.pop_front();
    if (b->t.joinable()) b->t.join();
    if (!have_first && b->decided && (b->exc || b->status != PipelineStatus::kCompleteRegular)) {
      have_first = true;
      first_exc = b->exc;
      first_status = b->status;
      first_message = b->message.empty() && b->status != PipelineStatus::kCompleteRegular
                          ? std::string("invariant : violations de roles ou de partition (message non formate)")
                          : b->message;
    }
    return !have_first;
  };
  const auto drain = [&]() {
    while (!slots.empty()) (void)reap_front();
    rr.peak_fold_inflight = b_peak.load();
  };
  const auto t_fold_wall = std::chrono::steady_clock::now();
#ifdef MHGP7_PROFILE_REDUCE
  rr.fold_epoch = t_fold_wall;
  std::atomic<u64> reduce_active{0}, reduce_peak{0};
#endif
  std::exception_ptr main_exc;
  PipelineStatus a_status = PipelineStatus::kCompleteRegular;
  std::string a_message;
  try {
    for (u64 K = 1; K <= rr.kmax_eff; ++K) {
      if (pub_failed.load()) break;
      phase(K, FoldPhase::kStageABegin);
      auto st = std::make_unique<Stage>();
      st->K = K;
      const auto t_k = std::chrono::steady_clock::now();
#ifdef MHGP7_PROFILE_REDUCE
      st->a_begin = t_k;
#endif
      expand_events_k(ix, balls, K, rr.kmax_eff, opt.threads, &st->events, &rr.expand);
      rr.t_expand_ms += ms(t_k);
      bool count_mismatch = st->events.size() != kc[K].events;
      if (MHGP7_MUTANT("fold-inject-a-failure-k2") && K == 2) count_mismatch = true;
      if (count_mismatch) {
        a_status = PipelineStatus::kInvariantViolated;
        a_message = "invariant : comptage par K != expansion (K=" + std::to_string(K) + ")";
        phase(K, FoldPhase::kStageAFailed);
        break;
      }
      if (opt.complete_silent_incidence && K >= 2) {
        const auto t_silent = std::chrono::steady_clock::now();
        SilentIncidenceResult added = build_silent_cofaces(ix, st->events, opt.silent_limits);
        rr.t_silent_ms += ms(t_silent);
        rr.silent_stats[K] = added.stats;
        if (added.status != SilentIncidenceStatus::kComplete) {
          switch (added.status) {
            case SilentIncidenceStatus::kUnsupportedDegeneracy:
              a_status = PipelineStatus::kUnsupportedDegeneracy; break;
            case SilentIncidenceStatus::kResourceExhausted:
              a_status = PipelineStatus::kResourceExhausted; break;
            case SilentIncidenceStatus::kInvalidInput:
              a_status = PipelineStatus::kInvalidInput; break;
            default:
              a_status = PipelineStatus::kInvariantViolated; break;
          }
          a_message = "silent incidence K=" + std::to_string(K) + " : " + added.reason;
          phase(K, FoldPhase::kStageAFailed);
          break;
        }
        const u64 count = static_cast<u64>(st->events.size()) + added.events.size();
        std::string why;
        if (!fold_capacity_ok(count, count * (K + 1), &why) ||
            (opt.memory_budget_bytes && !fits_budget(count, sizeof(ForestEvent),
              static_cast<u64>(opt.fold_inflight + 3), opt.memory_budget_bytes))) {
          a_status = PipelineStatus::kResourceExhausted;
          a_message = "silent incidence fold capacity K=" + std::to_string(K) + " : " + why;
          phase(K, FoldPhase::kStageAFailed);
          break;
        }
        st->events.insert(st->events.end(), added.events.begin(), added.events.end());
        rr.expand.events_by_k[K] += added.events.size();
      }
      const auto t_f = std::chrono::steady_clock::now();
      st->prep = prepare_fold(st->events, opt.threads, opt.forest_layout, opt.complete_silent_incidence);
      t_prepare_total_ms += ms(t_f);
#ifdef MHGP7_PROFILE_REDUCE
      st->a_end = std::chrono::steady_clock::now();
#endif
      if (!st->prep.r.refusal.empty()) {
        a_status = PipelineStatus::kInvariantViolated;
        a_message = "invariant : refus de fold apres la garde de capacite (K=" + std::to_string(K) + ")";
        phase(K, FoldPhase::kStageAFailed);
        break;
      }
      if ((int)slots.size() >= inflight && !reap_front()) break;
      const bool join_b_now = opt.fold_join_before_next_k;  // § 5.10 : B(K) joint avant A(K+1)
      slots.push_back(std::make_unique<BSlot>());
      BSlot* sp = slots.back().get();
      sp->K = K;
      auto fold_b = [&rr, &opt, &dg_all, &pub_mutex, &pub_cv, &next_publish, &pub_failed, &b_inflight, &b_peak,
#ifdef MHGP7_PROFILE_REDUCE
                           &reduce_active, &reduce_peak,
#endif
                           sp, st = std::move(st)]() mutable {
        const u64 K = st->K;
        struct Inflight {
          std::atomic<u64>& cur;
          std::atomic<u64>& peak;
          Inflight(std::atomic<u64>& c, std::atomic<u64>& p) : cur(c), peak(p) {
            const u64 now = cur.fetch_add(1) + 1;
            u64 seen = peak.load();
            while (seen < now && !peak.compare_exchange_weak(seen, now)) {
            }
          }
          ~Inflight() { cur.fetch_sub(1); }
        } inflight_guard{b_inflight, b_peak};
        const auto observe = [&](FoldPhase p) {
          if (!opt.on_fold_phase) return;
          try {
            opt.on_fold_phase(K, p);
          } catch (...) {
            if (!sp->exc) sp->exc = std::current_exception();
          }
        };
        ForestResult r;
        std::string dg;
        double t_fold_local = 0, t_dg = 0;
        try {
          observe(FoldPhase::kReduceBegin);
          const auto t_r = std::chrono::steady_clock::now();
#ifdef MHGP7_PROFILE_REDUCE
          {  // pic STRICTEMENT autour de reduce_fold (§ 5.10 : l'ancien
             // pic_inflight couvrait digest, attente de publication,
             // callback et I/O — il ne prouvait aucun chevauchement B×B) ;
             // RAII : une exception de la reduction ne fuit pas le compteur.
            struct ReduceScope {
              std::atomic<u64>& active;
              explicit ReduceScope(std::atomic<u64>& a, std::atomic<u64>& peak) : active(a) {
                const u64 nowv = active.fetch_add(1) + 1;
                u64 seen = peak.load();
                while (seen < nowv && !peak.compare_exchange_weak(seen, nowv)) {
                }
              }
              ~ReduceScope() { active.fetch_sub(1); }
            } reduce_scope{reduce_active, reduce_peak};
            r = reduce_fold(std::move(st->prep));
          }
#else
          r = reduce_fold(std::move(st->prep));
#endif
          t_fold_local = run_detail::ms(t_r);
          if (MHGP7_MUTANT("fold-inject-b-exception-k3") && K == 3)
            throw std::runtime_error("mutant fold-inject-b-exception-k3 : exception de reduction (K=3)");
          // Panne d'allocation INJECTEE dans un WORKER de l'etage B, au
          // PREMIER ordre (K=1) : aucun ordre n'a encore ete publie, donc la
          // scene exige zero callback. Le worker la capture (sp->exc), le fil
          // principal la relance, l'enrobage la convertit en refus « fold ».
          if (MHGP7_MUTANT("caps-throw-bad-alloc-fold") && K == 1) throw std::bad_alloc();
          // Aucune vue avant validation du stockage (le payload csr est de toute
          // facon vide en echec) : pas de digest d'un fold refuse.
          if (opt.digest && r.storage_violations == 0 && r.storage_message.empty()) {
            const auto t_d = std::chrono::steady_clock::now();
            dg = digest_forest_v4((u32)K, r);
            t_dg = run_detail::ms(t_d);
          }
        } catch (...) {
          sp->exc = std::current_exception();
        }
        observe(sp->exc ? FoldPhase::kReduceFailed : FoldPhase::kReduceEnd);
        std::unique_lock<std::mutex> lk(pub_mutex);
        pub_cv.wait(lk, [&] { return pub_failed.load() || next_publish == K; });
        if (pub_failed.load()) {
          lk.unlock();
          observe(FoldPhase::kNotPublished);
          return;
        }
        sp->decided = true;
        if (!sp->exc && (r.attach_violations || r.birth_violations || r.partition_violations || r.storage_violations)) {
          sp->status = PipelineStatus::kInvariantViolated;
          try {
            sp->message = "invariant : violations de roles, de partition ou de stockage (K=" + std::to_string(K) + ")" +
                          (r.storage_violations ? " : " + r.storage_message : std::string());
          } catch (...) {
            sp->message.clear();
          }
        } else if (!sp->exc && !r.storage_message.empty()) {
          // Capacite du stockage csr (gardes max_size/plafond/octets) : refus
          // transactionnel resource_exhausted, jamais un repli vers le classique.
          sp->status = PipelineStatus::kResourceExhausted;
          try {
            sp->message = "fold K=" + std::to_string(K) + " : " + r.storage_message;
          } catch (...) {
            sp->message.clear();
          }
        }
        if (sp->exc || sp->status != PipelineStatus::kCompleteRegular) {
          pub_failed.store(true);
          lk.unlock();
          pub_cv.notify_all();
          observe(FoldPhase::kNotPublished);
          return;
        }
        try {
          rr.t_fold_ms += t_fold_local;
          rr.t_fold_sort_ms += r.t_sort_ms;
          rr.t_fold_intern_ms += r.t_intern_ms;
          rr.t_fold_merge_ms += r.t_merge_ms;
          rr.t_fold_reduce_ms += r.t_reduce_ms + r.t_partition_ms;
          rr.fold_workers = std::max(rr.fold_workers, r.workers);
          rr.cards[K] =
              KCardinalities{st->events.size(), r.facets, r.delta_count(), r.new_attachments, r.fusions, r.nodes};
          rr.total_events += st->events.size();
          rr.total_facets += r.facets;
          rr.total_fusions += r.fusions;
          rr.total_deltas += r.delta_count();
          rr.total_nodes += r.nodes;
          {
            // SIGNATURE DE STOCKAGE par K (lectures size()/capacity() seulement,
            // aucun parcours) ; conformite = stockage CONSTRUIT == layout DEMANDE.
            RunResult::ForestStorageStats& s = rr.forest_storage[K];
            const bool is_csr = r.storage_kind == ForestStorageKind::kCsrFacetKeysV1;
            s.kind = (u8)r.storage_kind;
            s.deltas = r.delta_count();
            s.keys_parents = r.keys_parents;
            s.keys_born = r.keys_born;
            rr.sum_parents_by_k[K] = r.keys_parents;  // compteur de premier rang, plus seulement un champ de stockage
            rr.sum_parents_total += r.keys_parents;
            s.meta_size = r.delta_meta.size();
            s.meta_capacity = r.delta_meta.capacity();
            s.offsets_size = r.parents_off.size() + r.born_off.size();
            s.offsets_capacity = r.parents_off.capacity() + r.born_off.capacity();
            s.parents_size = r.parents_keys.size();
            s.parents_capacity = r.parents_keys.capacity();
            s.born_size = r.born_keys.size();
            s.born_capacity = r.born_keys.capacity();
            s.csr_capacity_growths = r.csr_capacity_growths;
            s.parents_off_back = r.parents_off.empty() ? 0 : r.parents_off.back();
            s.born_off_back = r.born_off.empty() ? 0 : r.born_off.back();
            s.bytes_exact = is_csr;
            s.bytes_owned = is_csr ? s.meta_capacity * sizeof(DeltaMeta) + s.offsets_capacity * sizeof(u32) +
                                         (s.parents_capacity + s.born_capacity) * sizeof(FacetKey)
                                   : r.deltas.capacity() * sizeof(ComponentDelta) +
                                         (r.keys_parents + r.keys_born) * sizeof(FacetKey);
            const bool conforme = (opt.forest_layout == ForestLayout::kCsr) == is_csr;
            if (conforme) ++rr.forest_storage_conformes;
            else ++rr.csr_fallback;
          }
          if (opt.digest) {
            rr.digest_forest[K] = dg;
            dg_all.add(dg);
            rr.t_digest_ms += t_dg;
          }
          if (MHGP7_MUTANT("prefix-tamper-batch-levels") && rr.kmax_eff < 10 && K == rr.kmax_eff &&
              !r.batch_levels.empty())
            r.batch_levels.push_back(r.batch_levels.back());
          if (opt.on_forest) opt.on_forest(K, st->events, r);
#ifdef MHGP7_PROFILE_REDUCE
          // § 5.10 (2e contre-lecture) : AUCUNE I/O ici — l'impression dans
          // le worker, sous le verrou de publication et avant l'arret du mur
          // du fold, serialisait la publication et contaminait le scheduler.
          // Le record est DRAINE (copie) et imprime par print_run apres le
          // retour de run_pipeline.
          rr.fold_profiles[K] = r.profile;
          rr.fold_profiles[K].a_begin = st->a_begin;
          rr.fold_profiles[K].a_end = st->a_end;
          rr.fold_profiles[K].duree_digest_foret_k_ms = t_dg;
          rr.peak_reduce_active = reduce_peak.load();
#endif
          rr.rss_mb[4] = std::max(rr.rss_mb[4], run_detail::rss_mb_now());
          rr.hwm_mb[4] = std::max(rr.hwm_mb[4], run_detail::vm_hwm_mb_now());
        } catch (...) {
          if (!sp->exc) sp->exc = std::current_exception();
          pub_failed.store(true);
          lk.unlock();
          pub_cv.notify_all();
          observe(FoldPhase::kNotPublished);
          return;
        }
        lk.unlock();
        st.reset();
        observe(FoldPhase::kPublished);
        lk.lock();
        if (sp->exc) {
          pub_failed.store(true);
          lk.unlock();
          pub_cv.notify_all();
          return;
        }
        next_publish = K + 1;
        lk.unlock();
        pub_cv.notify_all();
      };
      // Genuine mono execution: with join-before-next-K no earlier B slot
      // can still own next_publish, so this identical body cannot wait for
      // a predecessor that only the current thread could finish.
      if (opt.threads == 1 && join_b_now) fold_b();
      else sp->t = std::thread(std::move(fold_b));
      if (join_b_now && !reap_front()) break;  // § 5.10 : B(K) au bout avant A(K+1)
    }
  } catch (...) {
    main_exc = std::current_exception();
  }
  drain();
  if (have_first) {
    if (first_exc) std::rethrow_exception(first_exc);
    rr.status = first_status;
    rr.message = first_message;
    invalidate_provisional(&rr);
    return;
  }
  if (main_exc) std::rethrow_exception(main_exc);
  if (a_status != PipelineStatus::kCompleteRegular) {
    rr.status = a_status;
    rr.message = a_message;
    invalidate_provisional(&rr);
    return;
  }
  rr.stage_reached = kRunStagePublication;
  rr.t_fold_wall_ms = ms(t_fold_wall);
  rr.t_fold_ms += t_prepare_total_ms;
  std::vector<BallData>().swap(balls);
  if (opt.digest) rr.digest_all = dg_all.hex();
  rr.rss_mb[5] = run_detail::rss_mb_now();
  rr.hwm_mb[5] = run_detail::vm_hwm_mb_now();
  rr.t_total_ms = ms(t_all);
}

}  // namespace run_detail

// ENROBAGE TRANSACTIONNEL DE L'EPUISEMENT MEMOIRE (alerte G4 du 2 septembre).
// `std::bad_alloc` est la SEULE exception capturee ici : elle devient un refus
// dont le TEXTE ne porte jamais la sous-chaine `bad_alloc` — le validateur de
// campagne (gcp-migration/validate_v6_campaign.py) tient les classes d'issue de
// la phase frontiere pour mutuellement exclusives : un code 2 doit etre un refus
// type SANS diagnostic d'allocation brute, un code 134 est le bad_alloc NON
// capture (abort). Changer ce texte casserait cette exclusivite. Le refus
// NOMME l'etage atteint et grave les RSS d'etage — une donnee, la ou un abort
// (code 134) n'en est pas une. Toute autre
// exception se propage exactement comme avant (fold-inject-b-exception-k3
// termine toujours par signal : sa porte est inchangee).
// AUCUN PREFIXE DE PAYLOAD N'EST PUBLIE : `invalidate_provisional` vide
// digests, forets, cartes, totaux et signatures de stockage ; les seuls
// survivants sont le statut, le message, l'etage, les chronos et les RSS
// d'etage (diagnostic, jamais l'objet), comme sur tout autre refus.
// PORTEE EXACTE DU « AUCUN PREFIXE PUBLIE » (retour auditeur) : les callbacks
// deja appeles sont PROVISOIRES jusqu'au statut terminal, et l'invalidation
// interne ne reprend aucun effet EXTERNE deja produit chez l'appelant. La
// porte ne prouve donc l'absence de publication que pour une panne ANTERIEURE
// au premier callback (scene K=1) ; au-dela, le contrat est celui, historique,
// des provisoires — l'appelant jette ce qu'il a recu quand le statut n'est pas
// complete_regular.
// CE QUE CETTE CAPTURE NE PROMET PAS : ce n'est pas une garantie anti-OOM.
// L'OOM killer du noyau frappe hors de toute portee C++, `RLIMIT_AS` borne
// l'espace virtuel et non le RSS, et un allocateur qui rendrait un pointeur
// invalide au lieu de lever ne passe pas par ici.
inline RunResult run_pipeline(const std::vector<InputPoint>& in, const RunOptions& opt) {
  RunResult rr;
  const auto t_all = std::chrono::steady_clock::now();
  try {
    // La PROVISION du message est SOUS LA GARDE (retour auditeur du 2
    // septembre) : placee avant le `try`, son propre echec d'allocation
    // s'echappait de l'enrobage et rendait un abort — exactement le cas que
    // cet enrobage doit convertir. Le mutant caps-throw-bad-alloc-provision
    // injecte la panne A CET INSTANT, avant le corps du pipeline.
    if (MHGP7_MUTANT("caps-throw-bad-alloc-provision")) throw std::bad_alloc();
    rr.message.reserve(256);
    run_detail::run_pipeline_into(in, opt, rr);
  } catch (const std::bad_alloc&) {
    rr.status = PipelineStatus::kResourceExhausted;
    char buf[256];
    std::snprintf(buf, sizeof buf,
                  "resource_exhausted : allocation impossible a l'etage %s (rss_mb apres_generation=%.0f apres_rle=%.0f "
                  "apres_prefiltre=%.0f apres_census=%.0f max_fold=%.0f)",
                  run_stage_name(rr.stage_reached), rr.rss_mb[0], rr.rss_mb[1], rr.rss_mb[2], rr.rss_mb[3],
                  rr.rss_mb[4]);
    rr.message.clear();
    try {
      rr.message.assign(buf);
    } catch (...) {
      // Tas encore epuise : le statut et l'etage font foi, jamais un message
      // menteur (le champ reste vide).
    }
    invalidate_provisional(&rr);
    rr.t_total_ms = run_detail::ms(t_all);
  } catch (const std::system_error& error) {
    // Thread creation and OS resource failures are terminal, never an abort
    // caused by destroying a partially created team of joinable threads.
    const auto code = error.code();
    const bool resource = code == std::errc::resource_unavailable_try_again ||
                          code == std::errc::not_enough_memory ||
                          code == std::errc::too_many_files_open ||
                          code == std::errc::too_many_files_open_in_system;
    rr.status = resource ? PipelineStatus::kResourceExhausted : PipelineStatus::kInvariantViolated;
    rr.message.clear();
    try {
      rr.message = resource ? "resource_exhausted : OS/thread resource unavailable"
                            : "invariant : unexpected OS/thread failure";
    } catch (...) {
    }
    invalidate_provisional(&rr);
    rr.t_total_ms = run_detail::ms(t_all);
  }
  if (rr.status != PipelineStatus::kCompleteRegular) rr.t_total_ms = run_detail::ms(t_all);
  return rr;
}

// Impression standard des compteurs (lignes parsees par les campagnes v6).
inline void print_run(std::FILE* out, const char* family, int n, int coord, long long seed, const RunOptions& opt,
                      const RunResult& rr) {
  const GenerateStats& gs = rr.gen;
  const ExpandStats& es = rr.expand;
  std::fprintf(out, "payload=%s authority=status_terminal callbacks=provisional vertical_maps=none\n",
               kForestPayloadVersion);
  // Ligne SEPAREE (la ligne payload= est gravee textuellement par les
  // validateurs) : route DEMANDEE (forest_layout), stockage CONSTRUIT
  // (forest_storage_kind = kind commun des K publies, `mixte` s'ils
  // different, `aucun` sans ordre publie — jamais derive de la demande), repli
  // mesure (0 par construction), ordres publies et conformes.
  const char* kind_construit = "aucun";
  {
    bool any = false, mixte = false;
    u8 k0 = 0;
    for (u64 K = 1; K < (u64)rr.forest_storage.size() && K <= rr.kmax_eff; ++K) {
      const u8 k = rr.forest_storage[K].kind;
      if (!any) { k0 = k; any = true; }
      else if (k != k0) mixte = true;
    }
    if (any) kind_construit = mixte ? "mixte" : forest_storage_kind_name((ForestStorageKind)k0);
  }
  std::fprintf(out, "forest_layout=%s forest_storage_kind=%s csr_fallback=%llu ordres_publies=%llu ordres_storage_conformes=%llu\n",
               forest_layout_name(opt.forest_layout), kind_construit,
               (unsigned long long)rr.csr_fallback, (unsigned long long)rr.kmax_eff,
               (unsigned long long)rr.forest_storage_conformes);
  std::fprintf(out, "backend=cpu_reference\n");
  std::fprintf(out, "forest_semantics=%s public_status=not_claimed require_exact=false\n",
               opt.complete_silent_incidence ? "normalized_horizontal_h0_candidate" : "verified_events_only");
  std::fprintf(out, "census_storage=%s census_payload_peak_scope=owned_balldata_capacities\n",
               kCensusStorageVersion);
  if (opt.complete_silent_incidence) {
    std::fprintf(out, "silent_limits core_records=%llu chain_steps=%llu cofaces=%llu query_nodes=%llu meb_supports=%llu\n",
                 (unsigned long long)opt.silent_limits.max_core_records,
                 (unsigned long long)opt.silent_limits.max_chain_steps,
                 (unsigned long long)opt.silent_limits.max_added_cofaces,
                 (unsigned long long)opt.silent_limits.max_query_nodes,
                 (unsigned long long)opt.silent_limits.max_meb_supports);
    std::fprintf(out, "silent_incidence_ms=%.3f regularity=rank_window_and_local_descent vertical_maps=none\n", rr.t_silent_ms);
    for (u64 k = 2; k < rr.silent_stats.size(); ++k) {
      const auto& s = rr.silent_stats[k];
      std::fprintf(out, "silent_K%llu core=%llu with_two_intruders=%llu steps=%llu added=%llu max_chain=%llu query_nodes=%llu meb_supports=%llu\n",
                   (unsigned long long)k, (unsigned long long)s.core_facets,
                   (unsigned long long)s.facets_with_two_intruders, (unsigned long long)s.chain_steps,
                   (unsigned long long)s.added_cofaces, (unsigned long long)s.max_chain_length,
                   (unsigned long long)s.query_nodes, (unsigned long long)s.meb_supports);
    }
  }
  if (rr.smax_eff == 11)
    std::fprintf(out, "tower_scope=profile_complete_k10 smax_requested=%llu smax_effective=%llu\n",
                 (unsigned long long)opt.smax, (unsigned long long)rr.smax_eff);
  else
    std::fprintf(out,
                 "tower_scope=prefix_k%llu smax_requested=%llu smax_effective=%llu (K = 1..%llu, prefixe exact de "
                 "l'objet complet)\n",
                 (unsigned long long)(rr.smax_eff - 1), (unsigned long long)opt.smax, (unsigned long long)rr.smax_eff,
                 (unsigned long long)(rr.smax_eff - 1));
  std::fprintf(out,
               "famille=%s n=%d coord=%d s=%lld smax=%llu seed=%lld threads=%d emis=%zu boules_uniques=%llu "
               "mortes_profondeur=%llu survivantes=%llu census_int=%llu census_shell=%llu evenements=%llu "
               "facettes=%llu fusions=%llu deltas=%llu noeuds=%llu\n",
               family, n, coord, (long long)opt.s, (unsigned long long)rr.smax_eff, seed, opt.threads, rr.emitted,
               (unsigned long long)es.unique_balls, (unsigned long long)es.dead_depth, (unsigned long long)es.survivors,
               (unsigned long long)es.census_interior, (unsigned long long)es.census_shell,
               (unsigned long long)rr.total_events, (unsigned long long)rr.total_facets,
               (unsigned long long)rr.total_fusions, (unsigned long long)rr.total_deltas,
               (unsigned long long)rr.total_nodes);
  std::fprintf(out,
               "generation rect_alive=%llu/%llu/%llu rect_visites_fusionnes=%llu ancres=%llu/%llu/%llu "
               "candidats=%llu/%llu/%llu tues_profondeur=%llu/%llu/%llu ancres_w4=%llu ancres_w3=%llu "
               "ancres_secteurs=%llu/%llu ancres_cellules=%llu/%llu seeds_cellules=%llu/%llu grilles=%llu/%llu "
               "seeds=%llu/%llu completions_q4=%llu seeds_core_tues=%llu seeds_corde_tues=%llu "
               "float_cert=%llu/%llu repli=%llu ancres_hist=%llu/%llu/%llu hist_lignes=%llu/%llu/%llu "
               "hist_seuil=%llu/%llu/%llu hist_survivants=%llu/%llu/%llu jung=%llu/%llu/%llu\n",
               (unsigned long long)gs.rect_alive[0], (unsigned long long)gs.rect_alive[1],
               (unsigned long long)gs.rect_alive[2], (unsigned long long)gs.rect_visited_fused,
               (unsigned long long)gs.anchors[0], (unsigned long long)gs.anchors[1], (unsigned long long)gs.anchors[2],
               (unsigned long long)gs.candidates[0], (unsigned long long)gs.candidates[1],
               (unsigned long long)gs.candidates[2], (unsigned long long)gs.depth_killed[0],
               (unsigned long long)gs.depth_killed[1], (unsigned long long)gs.depth_killed[2],
               (unsigned long long)gs.anchors_killed_w4, (unsigned long long)gs.anchors_killed_w3,
               (unsigned long long)gs.anchors_killed_sectors[1], (unsigned long long)gs.anchors_killed_sectors[2],
               (unsigned long long)gs.anchors_killed_cells[1], (unsigned long long)gs.anchors_killed_cells[2],
               (unsigned long long)gs.seeds_killed_cells[1], (unsigned long long)gs.seeds_killed_cells[2],
               (unsigned long long)gs.grids_built[1], (unsigned long long)gs.grids_built[2],
               (unsigned long long)gs.seeds[0], (unsigned long long)gs.seeds[1], (unsigned long long)gs.q4_completions,
               (unsigned long long)gs.seeds_killed_core, (unsigned long long)gs.seeds_killed_chord,
               (unsigned long long)gs.float_cert_neg, (unsigned long long)gs.float_cert_pos,
               (unsigned long long)gs.float_fallback, (unsigned long long)gs.anchors_killed_hist[0],
               (unsigned long long)gs.anchors_killed_hist[1], (unsigned long long)gs.anchors_killed_hist[2],
               (unsigned long long)gs.hist_killed_rows[0], (unsigned long long)gs.hist_killed_rows[1],
               (unsigned long long)gs.hist_killed_rows[2], (unsigned long long)gs.hist_killed_thresh[0],
               (unsigned long long)gs.hist_killed_thresh[1], (unsigned long long)gs.hist_killed_thresh[2],
               (unsigned long long)gs.hist_survivors[0], (unsigned long long)gs.hist_survivors[1],
               (unsigned long long)gs.hist_survivors[2], (unsigned long long)gs.jung_cert_kill,
               (unsigned long long)gs.jung_cert_skip, (unsigned long long)gs.jung_fallback);
  // GRAND-LIVRE DU SWEEP DE CORDE (docs/GRAND_LIVRE.md : W_sweep scinde).
  std::fprintf(out,
               "sweep tests_coeur=%llu tests_prof_q3=%llu tests_passe2=%llu tri_comparaisons=%llu seeds_passe2=%llu racines_corde=%llu groupes=%llu racines_hors_corde=%llu temoins_constants=%llu "
               "rejets=lens:%llu/owner:%llu/once:%llu/i64:%llu/face:%llu/det:%llu/centre:%llu\n",
               (unsigned long long)gs.q4_core_site_tests, (unsigned long long)gs.q3_depth_site_tests,
               (unsigned long long)gs.sweep_pass2_site_tests, (unsigned long long)gs.sweep_root_comparisons,
               (unsigned long long)gs.sweep_pass2_seeds, (unsigned long long)gs.sweep_roots_onchord,
               (unsigned long long)gs.sweep_root_groups,
               (unsigned long long)gs.sweep_roots_offchord, (unsigned long long)gs.sweep_const_interior,
               (unsigned long long)gs.q4_rej_lens, (unsigned long long)gs.q4_rej_owner,
               (unsigned long long)gs.q4_rej_once, (unsigned long long)gs.q4_rej_i64,
               (unsigned long long)gs.q4_rej_face_power, (unsigned long long)gs.q4_rej_det,
               (unsigned long long)gs.q4_rej_center);
  // GRAND-LIVRE GLOBAL DES PAIRES (u128 imprime en deux u64 haut/bas si
  // necessaire ; sous u16 et n <= 2^32 la masse tient en u64).
  std::fprintf(out,
               "vwspd nœuds_temoins=%llu coins=%llu h_rect=%llu/%llu/%llu h_scan=%llu/%llu/%llu "
               "m_anchor=%llu/%llu/%llu entrees_ancres=%llu/%llu/%llu "
               "iters_coeur=%llu iters_passe2=%llu\n",
               (unsigned long long)gs.wspd_witness_nodes, (unsigned long long)gs.wspd_corner_evals,
               (unsigned long long)gs.h_rect[0], (unsigned long long)gs.h_rect[1], (unsigned long long)gs.h_rect[2],
               (unsigned long long)gs.h_scan[0], (unsigned long long)gs.h_scan[1], (unsigned long long)gs.h_scan[2],
               (unsigned long long)gs.m_anchor[0], (unsigned long long)gs.m_anchor[1],
               (unsigned long long)gs.m_anchor[2], (unsigned long long)gs.anchor_entries[0],
               (unsigned long long)gs.anchor_entries[1], (unsigned long long)gs.anchor_entries[2],
               (unsigned long long)gs.q4_core_iters, (unsigned long long)gs.q4_pass2_iters);
  const auto print_octaves = [&](const char* name, const u64 v[16]) {
    std::fprintf(out, "%s", name);
    for (int i = 0; i < 16; ++i) std::fprintf(out, "%llu%s", (unsigned long long)v[i], i == 15 ? "" : ",");
  };
  print_octaves("octaves_q4 ancres=", gs.q4_anchors_by_octave);
  print_octaves(" seeds=", gs.q4_seeds_by_octave);
  print_octaves(" w1=", gs.q4_w1_by_octave);
  std::fprintf(out, " (octave = log2 de la taille du cover de l'ancre)\n");
  // Les quatre issues d'un seed q4 par octave : identite fermante par octave
  // seeds[o] == cellules[o] + coeur[o] + corde[o] + passe2[o] (validateur).
  print_octaves("octaves_q4_seeds cellules=", gs.q4_seedcells_by_octave);
  print_octaves(" coeur=", gs.q4_seedcore_by_octave);
  print_octaves(" corde=", gs.q4_seedchord_by_octave);
  print_octaves(" passe2=", gs.q4_seedpass2_by_octave);
  std::fprintf(out, "\n");
  if (opt.e3_mode != E3G16Mode::kOff) {
    // Le bras ACTIF est imprime meme s'il n'a construit aucune grille
    // (exigence du quatrieme tour), avec les monnaies d'attribution.
    const char* bras = opt.e3_mode == E3G16Mode::kG8Lourdes      ? "g8_lourdes"
                       : opt.e3_mode == E3G16Mode::kG16Politique ? "g16_politique"
                       : opt.e3_mode == E3G16Mode::kG16NearM     ? "g16_nearm"
                       : opt.e3_mode == E3G16Mode::kG16Ratio     ? "g16_ratio"
                                                                 : "g16_leve";
    std::fprintf(out,
                 "e3_g16 bras=%s grilles16=%llu grilles8_lourdes=%llu scan_politique=%llu "
                 "scan_politique_saute=%llu cellules_consultees=g8:%llu/g16:%llu\n",
                 bras, (unsigned long long)gs.e6_grids16_built, (unsigned long long)gs.e3_g8_heavy_built,
                 (unsigned long long)gs.policy_scan_sites, (unsigned long long)gs.policy_scan_skipped_sites,
                 (unsigned long long)gs.cells_consulted_g8, (unsigned long long)gs.cells_consulted_g16);
  }
  if (gs.e6_sondes || gs.e6_sans_grille) {
    std::fprintf(out,
                 "sonde_e6 coeur_cellules=%llu,%llu,%llu,%llu,%llu sans_grille=%llu "
                 "raisons=cover:%llu/ratio:%llu/nearm:%llu/refus:%llu sondes=%llu "
                 "(min des temoins des cellules de corde des seeds tuees par coeur : 0 | <h/2 | >=h/2 | h-1 | hors domaine)\n",
                 (unsigned long long)gs.e6_coeur_cellules[0], (unsigned long long)gs.e6_coeur_cellules[1],
                 (unsigned long long)gs.e6_coeur_cellules[2], (unsigned long long)gs.e6_coeur_cellules[3],
                 (unsigned long long)gs.e6_coeur_cellules[4], (unsigned long long)gs.e6_sans_grille,
                 (unsigned long long)gs.e6_sans_grille_raison[1], (unsigned long long)gs.e6_sans_grille_raison[2],
                 (unsigned long long)gs.e6_sans_grille_raison[3], (unsigned long long)gs.e6_sans_grille_raison[4],
                 (unsigned long long)gs.e6_sondes);
  }
  std::fprintf(out,
               "vcensus prefiltre_nœuds=%llu prefiltre_feuilles=%llu range_add=%llu census_nœuds=%llu "
               "census_feuilles=%llu\n",
               (unsigned long long)es.depth.nodes, (unsigned long long)es.depth.leaf_tests,
               (unsigned long long)es.depth.range_add_mass, (unsigned long long)es.census.nodes,
               (unsigned long long)es.census.leaf_tests);
  std::fprintf(out, "p_factor=%llu/%llu/%llu (evaluations d'auto-produits des histogrammes)\n",
               (unsigned long long)gs.p_factor[0], (unsigned long long)gs.p_factor[1],
               (unsigned long long)gs.p_factor[2]);
  std::fprintf(out, "ledger_paires emis=%llu/%llu/%llu tues=%llu/%llu/%llu\n",
               (unsigned long long)gs.ledger_emitted_mass[0], (unsigned long long)gs.ledger_emitted_mass[1],
               (unsigned long long)gs.ledger_emitted_mass[2], (unsigned long long)gs.ledger_killed_mass[0],
               (unsigned long long)gs.ledger_killed_mass[1], (unsigned long long)gs.ledger_killed_mass[2]);
  std::fprintf(out, "ouvriers wspd=%llu rects=%llu rle=%llu prefiltre=%llu census=%llu expansion=%llu fold=%llu\n",
               (unsigned long long)gs.workers_wspd, (unsigned long long)gs.workers_rects,
               (unsigned long long)rr.rle_workers, (unsigned long long)es.workers_prefilter,
               (unsigned long long)es.workers_census, (unsigned long long)es.workers_expand,
               (unsigned long long)rr.fold_workers);
  std::fprintf(out,
               "temps_ms index=%.1f gen=%.1f (wspd %.1f rects %.1f) rle=%.1f prefiltre=%.1f "
               "census=%.1f comptage=%.1f expansion=%.1f fold=%.1f (tri %.1f intern %.1f fusion %.1f reduce %.1f) "
               "digest=%.1f\n",
               rr.t_index_ms, rr.t_gen_ms, gs.t_wspd_ms, gs.t_rects_ms, rr.t_rle_ms, rr.t_prefilter_ms, rr.t_census_ms,
               rr.t_count_ms, rr.t_expand_ms, rr.t_fold_ms, rr.t_fold_sort_ms, rr.t_fold_intern_ms, rr.t_fold_merge_ms,
               rr.t_fold_reduce_ms, rr.t_digest_ms);
  std::fprintf(out,
               "temps_mur_ms=%.1f (etages A et B du fold pipelines : fold+digest ci-dessus sont des cumuls par etage, "
               "pas le mur)\n",
               rr.t_total_ms);
  std::fprintf(out, "temps_fold_mur_ms=%.1f (etages A et B, fold_inflight=%d, fold_join=%d, pic_mesure_en_vol=%llu)\n",
               rr.t_fold_wall_ms, opt.fold_inflight, opt.fold_join_before_next_k ? 1 : 0,
               (unsigned long long)rr.peak_fold_inflight);
#ifdef MHGP7_PROFILE_REDUCE
  // DRAIN DU PROFIL (§ 5.10, 2e contre-lecture) : impression APRES le retour
  // de run_pipeline — plus aucune I/O dans les workers ni sous le verrou de
  // publication. Bornes du residuel = MEMES bornes que les fenetres
  // (end - begin, jamais t_reduce + t_partition — ceux-ci restent des
  // compteurs separes) ; pic_reduce_actif = chevauchement B×B STRICT autour
  // de reduce_fold (l'ancien pic est le cycle de vie des workers) ; les
  // intervalles A par K rendent la concurrence A/B LISIBLE dans la trace.
  // Le seul mur de debit reste celui d'un Release NON instrumente.
  std::fprintf(out,
               "profil_kind=reduce_v2%s fold_join=%d inflight_demande=%d pic_workers_b=%llu pic_reduce_actif=%llu layout=%s\n",
#ifdef MHGP7_PROFILE_LIVENESS
               "+liveness",
#else
               "",
#endif
               rr.profile_join ? 1 : 0, opt.fold_inflight, (unsigned long long)rr.peak_fold_inflight,
               (unsigned long long)rr.peak_reduce_active, forest_layout_name(opt.forest_layout));
  for (u64 K = 1; K < (u64)rr.fold_profiles.size(); ++K) {
    const ReduceProfile& pf = rr.fold_profiles[K];
    const auto rel = [&](std::chrono::steady_clock::time_point tp) {
      return std::chrono::duration<double, std::milli>(tp - rr.fold_epoch).count();
    };
    const double mur = std::chrono::duration<double, std::milli>(pf.end - pf.begin).count();
    // BORNES HONNETES (contre-lecture 2142c798) : cette fenetre couvre le
    // CORPS INTERNE de reduce_fold (apres le deplacement initial, avant ses
    // destructeurs) — les liberations de FoldPrepared/Stage, le digest, la
    // publication, le callback et la sonde RSS sont HORS fenetre. Le claim
    // est le recouvrement A/REDUCTION, jamais l'etage B complet.
    std::fprintf(out,
                 "profil_reduce K=%llu init=%.3f touch=%.3f pre=%.3f unite=%.3f post_remplissage=%.3f "
                 "materialisation_tri_copie=%.3f liveness=%.3f partition=%.3f liberation=%.3f "
                 "somme=%.3f mur_reduce_interne=%.3f residuel=%.3f reduce_interne_debut=%.3f "
                 "reduce_interne_fin=%.3f a_debut=%.3f a_fin=%.3f duree_digest_foret_k_ms=%.3f\n",
                 (unsigned long long)K, pf.init_ms, pf.touch_ms, pf.pre_ms, pf.unite_ms,
                 pf.post_remplissage_ms, pf.materialisation_tri_copie_ms, pf.liveness_ms, pf.partition_ms,
                 pf.liberation_ms, pf.somme(), mur, mur - pf.somme(), rel(pf.begin), rel(pf.end),
                 rel(pf.a_begin), rel(pf.a_end), pf.duree_digest_foret_k_ms);
    // Noms HONNETES (contre-lecture 7724e730) : alloc_empreintes inclut les
    // allocations de preparation, offsets_diffusion les offsets, intern_tri
    // toute la passe d'internement exact PUIS le tri local — pas de
    // separation artificielle qui perturberait elle-meme le profil.
    // fusion_et_lib_parts / remap_et_lib_pools : la liberation de `parts`
    // tombe dans la fenetre fusion et celle de `pools` dans la fenetre remap
    // (contre-lecture 01bd14a9 : nommer plutot que separer artificiellement).
    std::fprintf(out,
                 "profil_intern K=%llu alloc_empreintes=%.3f offsets_diffusion=%.3f intern_tri=%.3f "
                 "fusion_et_lib_parts=%.3f remap_et_lib_pools=%.3f\n",
                 (unsigned long long)K, pf.intern_empreintes_ms, pf.intern_diffusion_ms, pf.intern_tri_ms,
                 pf.intern_fusion_ms, pf.intern_remap_ms);
#ifdef MHGP7_PROFILE_LIVENESS
    std::fprintf(out, "profil_vivantes K=%llu pic_intra_lot=%llu frontiere_max=%llu moyenne_frontiere_pct=%.1f\n",
                 (unsigned long long)K, (unsigned long long)pf.live_peak_intra,
                 (unsigned long long)pf.live_frontier_max, pf.live_frontier_mean_pct);
#endif
  }
#endif
  std::fprintf(out,
               "rss_mb apres_generation=%.0f apres_rle=%.0f apres_prefiltre=%.0f apres_census=%.0f max_fold=%.0f "
               "fin=%.0f (telemetrie active : DEUX lectures par jalon — /proc/self/statm (instantane, ~57 us) "
               "et /proc/self/status (VmHWM, ~109 us) — dont une paire par K sous le verrou de publication ; "
               "a desarmer ou signer pour un run de debit)\n",
               rr.rss_mb[0], rr.rss_mb[1], rr.rss_mb[2], rr.rss_mb[3], rr.rss_mb[4], rr.rss_mb[5]);
  // Ligne ADJACENTE (la ligne rss_mb ci-dessus est gravee textuellement dans
  // les recus, elle ne bouge pas) : curseur d'etage du run. Au succes il vaut
  // toujours `publication` ; sur un refus, c'est l'etage que le message NOMME.
  std::fprintf(out, "etage_atteint=%s (curseur d'etage ; un refus le nomme dans son message)\n",
               run_stage_name(rr.stage_reached));
  // Ligne PROPRE, ADJACENTE (la ligne rss_mb ci-dessus reste bit-identique aux
  // recus immuables) : PIC HISTORIQUE de residence a chaque frontiere d'etage.
  // LECTURE EXACTE (rectifiee le 2 septembre par les relecteurs) : hwm_mb[j]
  // est le maximum HISTORIQUE depuis le debut du processus, pas le pic de
  // l'etage j. La seule quantite imputable a l'intervalle ]j-1, j] est
  // l'INCREMENT hwm_mb[j] - hwm_mb[j-1] (et hwm_mb[0] pour le premier) ; la
  // difference hwm_mb[j] - rss_mb[j] n'est qu'un MAJORANT GLOBAL, et elle
  // reste grande sur un etage qui n'alloue rien (le maximum d'un etage
  // anterieur y est simplement perime). SECONDE RECTIFICATION : VmHWM n'est
  // pas strictement monotone — /proc/pid/status publie max(mm->hiwater_rss,
  // resident courant) et hiwater_rss n'est rafraichi qu'a certains points, donc
  // deux lectures successives peuvent decroitre de quelques centaines de kio
  // (MESURE : 0,758 Mo sur le pipeline a smax=6, 0,18 Mo en sonde dediee).
  // C'est la porte de residence qui juge les increments, jamais cette ligne. MESURE DE RESIDENCE, jamais une preuve
  // de correction ni une garde. apres_prefiltre=0 signale la couture serie C
  // (le jalon n'existe pas), jamais un pic nul.
  std::fprintf(out,
               "residence_hwm_mb apres_generation=%.0f apres_rle=%.0f apres_prefiltre=%.0f apres_census=%.0f "
               "max_fold=%.0f fin=%.0f (VmHWM de /proc/self/status, maximum HISTORIQUE depuis le debut du "
               "processus — non decroissant a quelques centaines de kio pres, VmHWM valant max(hiwater "
               "enregistre, resident courant) ; l'ecart avec le rss_mb du meme jalon est un majorant global, "
               "pas une mesure d'etage)\n",
               rr.hwm_mb[0], rr.hwm_mb[1], rr.hwm_mb[2], rr.hwm_mb[3], rr.hwm_mb[4], rr.hwm_mb[5]);
  {
    // INCREMENTS : hwm_mb[j] - hwm_mb[j-1], seule quantite imputable a
    // l'intervalle. Un jalon non releve (route serie C, mutant
    // `drop-stage-milestone`) vaut 0 et reporte sa masse sur le suivant.
    double prev = 0.0;
    std::fprintf(out, "residence_increment_mb");
    for (int j = 0; j < 6; ++j) {
      const double h = rr.hwm_mb[j];
      const double inc = h > prev ? h - prev : 0.0;
      std::fprintf(out, " %s=%.0f", run_detail::kResidenceStageLabel[j], inc);
      prev = std::max(prev, h);
    }
    std::fprintf(out, " (increments du pic historique : ce que CHAQUE etage a ajoute au maximum)\n");
  }
  {
    const double pct = rr.census_balls ? 100.0 * (double)rr.plateau_balls / (double)rr.census_balls : 0.0;
    std::fprintf(out, "residence_compteurs boules_census=%llu boules_plateau=%llu plateau_pct=%.3f somme_parents_total=%llu somme_parents_par_K=",
                 (unsigned long long)rr.census_balls, (unsigned long long)rr.plateau_balls, pct,
                 (unsigned long long)rr.sum_parents_total);
    for (u64 K = 1; K < (u64)rr.sum_parents_by_k.size(); ++K)
      std::fprintf(out, "%s%llu", K == 1 ? "" : ",", (unsigned long long)rr.sum_parents_by_k[K]);
    std::fprintf(out, "%s\n", rr.sum_parents_by_k.size() > 1 ? "" : "(aucun ordre publie)");
  }
  for (u64 K = 1; K <= rr.kmax_eff; ++K)
    std::fprintf(
        out, "cardinalites K=%llu evenements=%llu facettes=%llu deltas=%llu attachements=%llu fusions=%llu noeuds=%llu\n",
        (unsigned long long)K, (unsigned long long)rr.cards[K].events, (unsigned long long)rr.cards[K].facets,
        (unsigned long long)rr.cards[K].deltas, (unsigned long long)rr.cards[K].attachments,
        (unsigned long long)rr.cards[K].fusions, (unsigned long long)rr.cards[K].nodes);
  // SIGNATURE DE STOCKAGE par K (palier KeyCSR) : offset_dernier_* = valeur LUE
  // de parents_off.back() / born_off.back() en csr (temoin independant : le juge
  // exige offset_dernier_* == cles_*), cles_* en classic ; csr_capacity_growths
  // n'est instrumente qu'en csr (0 = non instrumente en classic, incomparable).
  for (u64 K = 1; K < (u64)rr.forest_storage.size() && K <= rr.kmax_eff; ++K) {
    const RunResult::ForestStorageStats& s = rr.forest_storage[K];
    const bool is_csr = s.kind == (u8)ForestStorageKind::kCsrFacetKeysV1;
    std::fprintf(out,
                 "stockage_foret K=%llu kind=%s deltas=%llu cles_parents=%llu cles_nes=%llu meta=%llu/%llu offsets=%llu/%llu "
                 "parents=%llu/%llu nes=%llu/%llu csr_capacity_growths=%llu octets_possedes=%llu exact=%d "
                 "offset_dernier_parents=%llu offset_dernier_nes=%llu\n",
                 (unsigned long long)K, forest_storage_kind_name((ForestStorageKind)s.kind), (unsigned long long)s.deltas,
                 (unsigned long long)s.keys_parents, (unsigned long long)s.keys_born, (unsigned long long)s.meta_size,
                 (unsigned long long)s.meta_capacity, (unsigned long long)s.offsets_size,
                 (unsigned long long)s.offsets_capacity, (unsigned long long)s.parents_size,
                 (unsigned long long)s.parents_capacity, (unsigned long long)s.born_size,
                 (unsigned long long)s.born_capacity, (unsigned long long)s.csr_capacity_growths,
                 (unsigned long long)s.bytes_owned, s.bytes_exact ? 1 : 0,
                 (unsigned long long)(is_csr ? s.parents_off_back : s.keys_parents),
                 (unsigned long long)(is_csr ? s.born_off_back : s.keys_born));
  }
  if (opt.digest) {
    if (!rr.digest_raw_candidates.empty())
      std::fprintf(out, "digest_raw_candidates=%s\n", rr.digest_raw_candidates.c_str());
    std::fprintf(out, "digest_candidates_v5_compat=%s\n", rr.digest_balls.c_str());
    std::fprintf(out, "digest_postprefilter=%s\n", rr.digest_postprefilter.c_str());
    for (u64 K = 1; K <= rr.kmax_eff; ++K)
      std::fprintf(out, "digest_forest_K%llu=%s\n", (unsigned long long)K, rr.digest_forest[K].c_str());
    std::fprintf(out, "digest_all=%s\n", rr.digest_all.c_str());
  }
}

// Code de sortie transactionnel d'un statut.
inline int status_exit_code(PipelineStatus s) {
  switch (s) {
    case PipelineStatus::kCompleteRegular:
      return 0;
    case PipelineStatus::kInvariantViolated:
      return 3;
    default:
      return 2;
  }
}

}  // namespace mhgp7
