// MorseHGP3D v5 — le pipeline complet, en bibliotheque : nuage -> foret HGP.
//
//   index -> generation (trois lanes) -> RLE -> prefiltre de profondeur ->
//   census -> expansion des plateaux -> folds par K, STREAMES.
//
// Chaque fold est construit, signe, compte puis LIBERE avant le suivant :
// la residence est bornee par UN fold plus les evenements (jamais dix forets
// residentes — la faute de fond de la v4). Statuts transactionnels :
//   complete_regular | unsupported_degeneracy | resource_exhausted |
//   invalid_input | invariant_violated.
// Le consommateur recoit chaque ForestResult par callback (`on_forest`) et
// decide ce qu'il en garde ; la signature au format v4 est calculee ici pour
// la porte de conformite.
#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "../core/mutants.hpp"
#include "../tree/cloud_index.hpp"
#include "digest.hpp"
#include "expand.hpp"
#include "generate.hpp"

namespace mhgp5 {

// Phases de l'etage B d'un ordre K, OBSERVEES par `RunOptions::on_fold_phase`
// (instrument, jamais une garde) :
//   kStageABegin   fil principal : expansion + preparation de l'ordre K commencent ;
//   kStageAFailed  fil principal : l'etage A de K a produit un defaut (invariant
//                  ou refus) — les ordres en vol vont etre draines, pas annules ;
//   kReduceBegin   fil B de K : reduction (+ digest) commencee ;
//   kReduceEnd     fil B de K : reduction terminee, en attente de son tour ;
//   kReduceFailed  fil B de K : exception de reduction/digest CONSERVEE dans le
//                  slot, en attente de son tour (aucun ordre inferieur annule) ;
//   kPublished     fil B de K : publication terminee (callback compris) ;
//   kNotPublished  fil B de K : pas de publication (defaut decide a son tour,
//                  ou annule par un defaut d'ordre inferieur).
enum class FoldPhase : u8 { kStageABegin, kStageAFailed, kReduceBegin, kReduceEnd, kReduceFailed, kPublished, kNotPublished };

// Domaine de `RunOptions::fold_inflight` : [1, kFoldInflightMax]. Toute autre
// valeur est REFUSEE (invalid_input) avant tout calcul, jamais ramenee en
// silence a une valeur valide (une option hors profil n'est pas une mesure).
inline constexpr int kFoldInflightMax = 16;

// Profil produit v5 : la generation q3/q4 n'est autorisee qu'a partir de la
// separation entiere 8 — `kSeparationProfileMin`, defini avec sa borne de marge
// dans src/wspd/wavefront.hpp. Les primitives WSPD et les fixtures test-only
// peuvent explorer plus bas, mais SEULEMENT par l'opt-in explicite
// `allow_subprofile_separation` d'`alive_rectangles`, et une telle mesure n'est
// jamais un point de fonctionnement recevable.

struct RunOptions {
  i64 s = 8;
  u64 smax = 11;
  int threads = 1;
  size_t shell_cap = 12;
  bool digest = false;
  // Diagnostic opt-in : signe le multiensemble trie AVANT RLE. Le digest
  // normal reste seul par defaut, afin de ne pas ajouter un second hachage du
  // catalogue brut aux contrats de temps historiques.
  bool diagnostic_raw_candidates_digest = false;
  // Ordres K dont l'etage B (reduction sequentielle par ordre, signature,
  // publication) peut etre en vol simultanement, dans [1, kFoldInflightMax] ;
  // la publication reste dans l'ordre des K et la sortie bit-identique ;
  // residence bornee a fold_inflight + 1 ordres. Le pic reellement atteint est
  // MESURE (`RunResult::peak_fold_inflight`), jamais declare.
  int fold_inflight = 2;
  // Observateur des phases du fold (vide : rien). Appele HORS de tout verrou,
  // depuis le fil principal (kStageA*) ou depuis le fil B de l'ordre concerne
  // (les autres phases), donc potentiellement depuis plusieurs fils a la fois :
  // l'observateur se synchronise lui-meme. Il peut bloquer (les portes s'en
  // servent pour forcer un entrelacement) ; une exception qu'il leve est
  // traitee comme une exception de l'ordre observe (propagee apres jonction).
  std::function<void(u64 K, FoldPhase phase)> on_fold_phase;
  size_t pretest_query_min_points = kPretestQueryMinPoints;  // 0 = requete toujours ; SIZE_MAX = cover toujours
  size_t cell_grid_min_sites = kCellGridMinSites;  // grille de cellules (generate.hpp) ; SIZE_MAX : jamais (mesure contrefactuelle)
  // Filtre d'enveloppe q3/q4 experimental, opt-in. Il compacte le cover
  // historique apres les handles ; aucun claim tant que l'appariement ON/OFF
  // du catalogue brut jusqu'aux forets n'est pas recu.
  bool cover_envelope_filter = false;
  u32 postsep_refine_levels = 0;  // raffinement post-separation, L in [0, 3] ; 0 = desactive. q2 jamais raffinee.
  // Appele pour chaque K croissant, AVANT liberation du resultat, depuis le
  // fil d'arriere-plan de cet ordre (un seul a la fois, dans l'ordre des K,
  // sous le verrou de publication) — PROVISOIRE jusqu'au statut terminal.
  std::function<void(u64 K, const std::vector<ForestEvent>& events, const ForestResult& r)> on_forest;
  // Executeurs de lane externes (device) — vides : lanes integrees. BACKEND
  // EXPERIMENTAL, NON AUTORITAIRE : un resultat obtenu avec un executeur
  // externe est marque `backend_override` (imprime `backend=override_experimental`)
  // et ne vaut que par son egalite prouvee avec le chemin integre (portes,
  // campagnes appariees) — jamais par lui-meme.
  LaneOverride q3_override, q4_override;
};

struct KCardinalities {
  u64 events = 0, facets = 0, deltas = 0, attachments = 0, fusions = 0, nodes = 0;
};

struct RunResult {
  bool backend_override = false;  // un executeur de lane externe a ete employe (non autoritaire)
  PipelineStatus status = PipelineStatus::kCompleteRegular;
  std::string message;
  u64 smax_eff = 0, kmax_eff = 0;
  size_t emitted = 0;
  GenerateStats gen;
  ExpandStats expand;
  std::vector<KCardinalities> cards;  // indexee par K
  u64 total_facets = 0, total_fusions = 0, total_deltas = 0, total_nodes = 0, total_events = 0;
  std::string digest_raw_candidates, digest_balls, digest_all;
  std::vector<std::string> digest_forest;  // indexee par K
  double t_index_ms = 0, t_gen_ms = 0, t_rle_ms = 0, t_prefilter_ms = 0, t_census_ms = 0, t_expand_ms = 0,
         t_fold_ms = 0, t_count_ms = 0, t_digest_ms = 0;
  double t_fold_sort_ms = 0, t_fold_intern_ms = 0, t_fold_merge_ms = 0, t_fold_reduce_ms = 0;
  double t_fold_wall_ms = 0;  // mur des etages A et B (premier ordre expanse -> derniere publication)
  double rss_mb[6] = {0, 0, 0, 0, 0, 0};  // paliers : apres generation, rle, prefiltre, census, max pendant le fold, fin
  double t_total_ms = 0;  // temps MUR de run_pipeline
  u64 fold_workers = 0, rle_workers = 0;
  // Pic MESURE d'ordres simultanement en vol dans l'etage B (du demarrage du
  // fil de reduction a la fin de sa publication ou de son abandon) ; toujours
  // <= fold_inflight, et == 1 si fold_inflight == 1.
  u64 peak_fold_inflight = 0;
};

namespace run_detail {

// Resident set courant en Mo (Linux : /proc/self/statm ; 0 ailleurs) — un
// instrument de lecture des paliers memoire, jamais une garde.
inline double rss_mb_now() {
  std::FILE* f = std::fopen("/proc/self/statm", "r");
  if (!f) return 0.0;
  unsigned long long size = 0, resident = 0;
  const int got = std::fscanf(f, "%llu %llu", &size, &resident);
  std::fclose(f);
  if (got != 2) return 0.0;
  return (double)resident * 4096.0 / (1024.0 * 1024.0);
}

inline double ms(std::chrono::steady_clock::time_point t0) {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}
}  // namespace run_detail

// Profil : K_max <= 10 ⟺ smax <= 11 (les tampons d'interieurs et les tableaux
// par K sont dimensionnes pour ce profil ; au-dela, refus explicite).
inline constexpr u64 kSmaxProfile = 11;

// CONTRAT DE PAYLOAD (arbitrage V3 de l'auditeur, 27 aout 2026) — version de
// representation declaree, jamais cachee dans une graine :
//   payload_kind      = forets horizontales par ordre K (pas la tour : aucune
//                       application verticale entre ordres n'est livree) ;
//   par K             = niveaux de lots (`batch_levels`), deltas avec parents,
//                       naissances et representant de sortie (`deltas`),
//                       partition finale dense (`facet_keys` strictement
//                       croissantes + `final_canon_fid`) ;
//   retention         = toutes les facettes (F_K^render), jamais un prefixe ;
//   authority         = `RunResult::status` terminal ; les callbacks sont
//                       PROVISOIRES jusqu'a ce statut ;
//   canonical digest  = format `mhgp4-digest-v1` (conformite v4).
inline constexpr const char* kForestPayloadVersion = "mhgp5-forests-horizontal-v1";

// GARDES DE BIBLIOTHEQUE (pas seulement de CLI) : toute option hors profil et
// toute entree qui ne definit pas l'objet sont refusees AVANT tout calcul,
// avec le statut contractuel — jamais un debordement.
// Profil du plafond de coquille : 4 <= shell_cap <= 12 (arbitrage V2 ; une
// coquille effectivement > 12 est resource_exhausted, jamais tronquee).
inline constexpr size_t kShellCapProfile = kBallShellMax;  // = tableau inline de BallData (expand.hpp)

inline bool validate_run_options(const std::vector<InputPoint>& in, const RunOptions& opt, std::string* why) {
  if (in.size() < 2) { *why = "invalid_input : moins de deux points"; return false; }
  if (opt.s < kSeparationProfileMin) { *why = "invalid_input : separation s < 8"; return false; }
  if (opt.smax < 2 || opt.smax > kSmaxProfile) { *why = "invalid_input : smax hors du profil [2, 11]"; return false; }
  if (opt.threads < 1) { *why = "invalid_input : threads < 1"; return false; }
  if (opt.postsep_refine_levels > 3) {
    *why = "invalid_input : postsep_refine_levels hors du domaine [0, 3]";
    return false;
  }
  if (opt.postsep_refine_levels > 0 && (opt.q3_override || opt.q4_override)) {
    *why = "invalid_input : raffinement post-separation non propage aux overrides q3/q4";
    return false;
  }
  if (opt.fold_inflight < 1 || opt.fold_inflight > kFoldInflightMax) {
    *why = "invalid_input : fold_inflight hors du domaine [1, " + std::to_string(kFoldInflightMax) + "]";
    return false;
  }
  if (opt.shell_cap < 4 || opt.shell_cap > kShellCapProfile) {
    *why = "invalid_input : plafond de coquille hors du profil [4, 12] (l'enumeration des plateaux indexe 2^|coquille|)";
    return false;
  }
  return true;
}

inline RunResult run_pipeline(const std::vector<InputPoint>& in, const RunOptions& opt) {
  using run_detail::ms;
  RunResult rr;
  const auto t_all = std::chrono::steady_clock::now();
  if (!validate_run_options(in, opt, &rr.message)) {
    rr.status = PipelineStatus::kInvalidInput;
    return rr;
  }
  const auto t_ix = std::chrono::steady_clock::now();
  const CloudIndex ix = build_cloud_index(in);
  rr.t_index_ms = ms(t_ix);
  if (!ix.valid) {
    rr.status = PipelineStatus::kInvalidInput;
    rr.message = "invalid_input : coordonnee hors profil u16 ou PointId duplique";
    return rr;
  }
  if (ix.has_duplicate_positions()) {
    rr.status = PipelineStatus::kUnsupportedDegeneracy;
    rr.message = "unsupported_degeneracy : positions dupliquees";
    return rr;
  }
  rr.smax_eff = std::min<u64>(opt.smax, in.size());
  rr.kmax_eff = rr.smax_eff - 1;

  const auto t_g = std::chrono::steady_clock::now();
  std::vector<BallCandidate> cands;
  GenerateOptions go;
  go.pretest_query_min_points = opt.pretest_query_min_points;
  go.cell_grid_min_sites = opt.cell_grid_min_sites;
  go.cover_envelope_filter = opt.cover_envelope_filter;
  go.postsep_refine_levels = opt.postsep_refine_levels;
  go.q3_override = opt.q3_override;
  go.q4_override = opt.q4_override;
  rr.backend_override = (bool)opt.q3_override || (bool)opt.q4_override;
  go.s = opt.s;
  go.smax = rr.smax_eff;
  go.threads = opt.threads;
  generate_candidates(ix, go, &cands, &rr.gen);
  rr.t_gen_ms = ms(t_g);
  rr.rss_mb[0] = run_detail::rss_mb_now();
  for (int q = 0; q < 3; ++q) {
    const u128 accounted = (u128)rr.gen.postsep_emitted_mass[q] + rr.gen.postsep_killed_mass[q];
    if (accounted != rr.gen.postsep_base_mass[q]) ++rr.gen.postsep_ledger_violations;
    if (rr.gen.rect_alive[q] != rr.gen.postsep_emitted_rects[q]) ++rr.gen.postsep_ledger_violations;
    if (opt.postsep_refine_levels == 0 &&
        rr.gen.postsep_parent_rects[q] != rr.gen.postsep_emitted_rects[q])
      ++rr.gen.postsep_ledger_violations;
  }
  if (rr.gen.postsep_killed_mass[0] != 0 || rr.gen.postsep_emitted_mass[0] != rr.gen.postsep_base_mass[0] ||
      rr.gen.postsep_parent_rects[0] != rr.gen.postsep_emitted_rects[0] ||
      rr.gen.postsep_subrects[0] != 0 || rr.gen.postsep_core_evals[0] != 0 ||
      rr.gen.postsep_core_nodes[0] != 0 || rr.gen.postsep_corner_evals[0] != 0 ||
      rr.gen.postsep_rollbacks[0] != 0)
    ++rr.gen.postsep_ledger_violations;
  if (rr.gen.postsep_core_regressions[0] || rr.gen.postsep_core_regressions[1] || rr.gen.postsep_core_regressions[2])
    ++rr.gen.postsep_ledger_violations;
  if (rr.gen.postsep_ledger_violations) {
    rr.status = PipelineStatus::kInvariantViolated;
    rr.message = "invariant : grand-livre, structure q2 ou monotonie du raffinement post-separation viole";
    return rr;
  }
  if (rr.gen.invariant_jneg) {
    rr.status = PipelineStatus::kInvariantViolated;
    rr.message = "invariant : seed q4 aigu avec J < 0 (inatteignable par theoreme, MATHEMATIQUES § 10) : " +
                 std::to_string(rr.gen.invariant_jneg) + " occurrence(s)";
    return rr;
  }
  const auto t_r = std::chrono::steady_clock::now();
  rr.emitted = cands.size();
  rr.rle_workers = sort_candidates(&cands, opt.threads);
  const double t_sort_candidates_ms = ms(t_r);
  if (opt.diagnostic_raw_candidates_digest) {
    const auto t_d = std::chrono::steady_clock::now();
    rr.digest_raw_candidates = digest_raw_candidates_v5(cands);
    rr.t_digest_ms += ms(t_d);
  }
  const auto t_u = std::chrono::steady_clock::now();
  deduplicate_candidates(&cands);
  rr.t_rle_ms = t_sort_candidates_ms + ms(t_u);
  rr.rss_mb[1] = run_detail::rss_mb_now();
  rr.expand.unique_balls = cands.size();

  const auto t_p = std::chrono::steady_clock::now();
  std::vector<Survivor> surv;
  prefilter_balls(ix, cands, rr.smax_eff, opt.threads, &surv, &rr.expand);
  rr.t_prefilter_ms = ms(t_p);
  rr.rss_mb[2] = run_detail::rss_mb_now();
  const auto t_c = std::chrono::steady_clock::now();
  std::vector<BallData> balls;
  const PipelineStatus cs = census_balls(ix, cands, surv, rr.smax_eff, opt.shell_cap, opt.threads, &balls, &rr.expand);
  if (cs != PipelineStatus::kCompleteRegular) {
    rr.status = cs;
    rr.message = cs == PipelineStatus::kResourceExhausted
                     ? "resource_exhausted : coquille au-dela du plafond (jamais de troncature)"
                     : "invariant : census contredit la passe count-only";
    return rr;
  }
  std::vector<Survivor>().swap(surv);
  rr.t_census_ms = ms(t_c);
  rr.rss_mb[3] = run_detail::rss_mb_now();

  // GARDES DE CAPACITE DE TOUS LES ORDRES AVANT TOUTE PUBLICATION (contrat
  // transactionnel) : un refus ne suit jamais un callback. Les comptes par K
  // sont etablis sans materialiser les evenements.
  const auto t_e = std::chrono::steady_clock::now();
  const std::vector<KCount> kc = count_events_by_k(ix, balls, rr.kmax_eff, opt.threads);
  for (u64 K = 1; K <= rr.kmax_eff; ++K) {
    std::string why;
    if (!fold_capacity_ok(kc[K].events, kc[K].incidences, &why)) {
      rr.status = PipelineStatus::kResourceExhausted;
      rr.message = "fold K=" + std::to_string(K) + " : " + why;
      return rr;
    }
  }
  rr.t_count_ms += ms(t_e);

  const auto t_d0 = std::chrono::steady_clock::now();
  if (opt.digest) rr.digest_balls = digest_balls_v4(cands);
  rr.t_digest_ms += ms(t_d0);
  std::vector<BallCandidate>().swap(cands);
  DigestAll dg_all;
  rr.digest_forest.assign(rr.kmax_eff + 1, std::string());
  rr.cards.assign(rr.kmax_eff + 1, KCardinalities{});
  rr.expand.events_by_k.assign(rr.kmax_eff + 1, 0);
  // STREAMING PAR K : les evenements d'un ordre sont materialises, foldes,
  // signes, publies (callback) puis liberes ; les boules censusees restent le
  // seul objet amont resident. Les callbacks sont PROVISOIRES jusqu'au statut
  // terminal : seule une violation d'invariant (un defaut, jamais un refus de
  // capacite) peut encore invalider la sortie apres un callback.
  // PIPELINE A DEUX ETAGES : etage A (expansion, tri, internement, fusion —
  // parallele, `threads` ouvriers) pour l'ordre K+1 pendant que l'etage B
  // (reduction sequentielle, signature, publication, liberation) des ordres
  // precedents s'execute en arriere-plan. ETAGE B CONCURRENT PAR ORDRE :
  // jusqu'a `fold_inflight` reductions en vol (ordres distincts,
  // independantes — un fil chacune), mais une PUBLICATION strictement dans
  // l'ordre des K sous un verrou (digest chaine, cartes, totaux, callback,
  // l'un apres l'autre) : la sortie est bit-identique au pipeline sequentiel
  // (chaque reduce est deterministe et ne depend que de son ordre), la
  // residence est bornee a fold_inflight + 1 ordres. Les callbacks
  // s'executent depuis le fil de l'ordre publie, jamais deux a la fois.
  //
  // SURETE DES FILS (P1 de l'audit du 28 aout 2026) :
  //   1. le slot d'un ordre est POSSEDE par `slots` AVANT le demarrage de son
  //      fil : aucune fenetre ou un std::thread joignable serait detruit
  //      (std::terminate) par l'echec d'une insertion ;
  //   2. AUCUN `return rr` ni relance d'exception apres le premier lancement
  //      ne precede la JONCTION EXPLICITE de tous les fils (`drain`) : le
  //      joiner RAII ne suffit pas, car la valeur de retour est copiee ou
  //      deplacee AVANT la destruction des locales (sans NRVO, `rr` serait
  //      deplace pendant qu'un fil y ecrit) ; le joiner ne reste qu'un filet
  //      de securite (annulation + jonction) pour un chemin d'exception que le
  //      drain lui-meme n'aurait pas couvert ;
  //   3. PREMIER DEFAUT DANS L'ORDRE DES K : chaque fil B conserve son verdict
  //      (exception de reduction/digest/callback, ou statut d'invariant) dans
  //      SON slot et ne le rend decisif qu'a SON tour de publication
  //      (`next_publish == K`) — une exception d'un K superieur n'annule
  //      jamais la publication d'un K inferieur ; un defaut d'etage A a l'ordre
  //      K (fil principal) est lui aussi un defaut d'ordre K : les ordres en vol
  //      (tous < K) sont DRAINES (publies ou decides a leur tour), et seul le
  //      plus petit ordre en defaut fait foi (un defaut B inferieur l'emporte
  //      sur le defaut A superieur). `pub_failed` n'est pose que par le slot
  //      dont c'est le tour (ou par le filet RAII) : les ordres superieurs
  //      s'arretent alors sans publier (`kNotPublished`).
  struct Stage {
    u64 K = 0;
    std::vector<ForestEvent> events;
    FoldPrepared prep;
  };
  struct BSlot {
    std::thread t;
    u64 K = 0;
    bool decided = false;  // son tour est venu : `exc`/`status` font foi (sinon : annule, sans autorite)
    std::exception_ptr exc;
    PipelineStatus status = PipelineStatus::kCompleteRegular;
    std::string message;
  };
  const int inflight = opt.fold_inflight;  // valide dans [1, kFoldInflightMax] par validate_run_options
  std::deque<std::unique_ptr<BSlot>> slots;
  std::mutex pub_mutex;
  std::condition_variable pub_cv;
  u64 next_publish = 1;
  // Pose sous `pub_mutex` (jamais de reveil perdu), mais ATOMIQUE pour que le
  // fil principal puisse le lire sans prendre le verrou de publication, tenu
  // pendant tout un callback (qui peut bloquer longtemps : les portes s'en servent).
  std::atomic<bool> pub_failed{false};
  std::atomic<u64> b_inflight{0}, b_peak{0};  // ordres en vol dans l'etage B ; pic MESURE
  // Filet RAII (annulation + notification + jonction), NORMALEMENT INACTIF :
  // tous les chemins de sortie post-lancement drainent explicitement avant de
  // retourner ou de relancer. Il ne joue que si le drain lui-meme est
  // interrompu par une exception (jonction impossible) — jamais std::terminate.
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
  // Le fil principal n'ecrit que dans ses propres cumuls ; les cumuls des
  // fils d'arriere-plan (t_fold_*, digests, cartes, totaux) ne sont ecrits
  // que sous le verrou de publication et lus apres le dernier join.
  double t_prepare_total_ms = 0;
  // Verdict global : le premier defaut dans l'ordre des K (B decide a son tour,
  // ou A du fil principal — toujours d'ordre superieur aux slots en vol).
  std::exception_ptr first_exc;
  PipelineStatus first_status = PipelineStatus::kCompleteRegular;
  std::string first_message;
  bool have_first = false;
  // Joint le plus ancien ordre en vol et RETIENT son verdict s'il est le premier
  // (les slots sont en ordre de K ; un slot non decide a ete annule par un
  // defaut inferieur deja retenu et n'a aucune autorite). Ne relance rien :
  // toute relance attend la jonction de TOUS les fils.
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
  std::exception_ptr main_exc;  // exception du fil principal (etage A ou lancement d'un fil)
  PipelineStatus a_status = PipelineStatus::kCompleteRegular;  // defaut d'etage A (ordre courant)
  std::string a_message;
  try {
    for (u64 K = 1; K <= rr.kmax_eff; ++K) {
      if (pub_failed.load()) break;  // un defaut inferieur est decide : ne plus lancer d'ordre (lecture sans verrou)
      phase(K, FoldPhase::kStageABegin);
      auto st = std::make_unique<Stage>();
      st->K = K;
      const auto t_k = std::chrono::steady_clock::now();
      expand_events_k(ix, balls, K, rr.kmax_eff, opt.threads, &st->events, &rr.expand);
      rr.t_expand_ms += ms(t_k);
      bool count_mismatch = st->events.size() != kc[K].events;
      if (MHGP5_MUTANT("fold-inject-a-failure-k2") && K == 2) count_mismatch = true;  // echec d'etage A injecte a K=2
      if (count_mismatch) {
        a_status = PipelineStatus::kInvariantViolated;
        a_message = "invariant : comptage par K != expansion (K=" + std::to_string(K) + ")";
        phase(K, FoldPhase::kStageAFailed);
        break;
      }
      const auto t_f = std::chrono::steady_clock::now();
      st->prep = prepare_fold(st->events, opt.threads);
      t_prepare_total_ms += ms(t_f);
      if (!st->prep.r.refusal.empty()) {  // impossible apres la garde amont ; traite en invariant
        a_status = PipelineStatus::kInvariantViolated;
        a_message = "invariant : refus de fold apres la garde de capacite (K=" + std::to_string(K) + ")";
        phase(K, FoldPhase::kStageAFailed);
        break;
      }
      if ((int)slots.size() >= inflight && !reap_front()) break;
      // Le slot appartient au conteneur AVANT le demarrage du fil (surete 1).
      slots.push_back(std::make_unique<BSlot>());
      BSlot* sp = slots.back().get();
      sp->K = K;
      sp->t = std::thread([&rr, &opt, &dg_all, &pub_mutex, &pub_cv, &next_publish, &pub_failed, &b_inflight, &b_peak, sp,
                           st = std::move(st)]() mutable {
        const u64 K = st->K;
        // Comptage des ordres en vol (demarrage -> fin de publication/abandon).
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
          r = reduce_fold(std::move(st->prep));
          t_fold_local = run_detail::ms(t_r);
          if (MHGP5_MUTANT("fold-inject-b-exception-k3") && K == 3)
            throw std::runtime_error("mutant fold-inject-b-exception-k3 : exception de reduction (K=3)");
          if (opt.digest) {
            const auto t_d = std::chrono::steady_clock::now();
            dg = digest_forest_v4((u32)K, r);
            t_dg = run_detail::ms(t_d);
          }
        } catch (...) {
          sp->exc = std::current_exception();
        }
        observe(sp->exc ? FoldPhase::kReduceFailed : FoldPhase::kReduceEnd);
        // SON TOUR : seul l'ordre `next_publish` decide ; un defaut conserve
        // ici n'a annule personne jusqu'a present.
        std::unique_lock<std::mutex> lk(pub_mutex);
        pub_cv.wait(lk, [&] { return pub_failed.load() || next_publish == K; });
        if (pub_failed.load()) {  // un ordre INFERIEUR a decide un defaut : abandon sans autorite
          lk.unlock();
          observe(FoldPhase::kNotPublished);
          return;
        }
        sp->decided = true;
        if (!sp->exc && (r.attach_violations || r.birth_violations || r.partition_violations)) {
          sp->status = PipelineStatus::kInvariantViolated;
          // Le formatage alloue : un bad_alloc ici ne doit ni quitter le fil ni masquer le statut (audit du 28 aout).
          try {
            sp->message = "invariant : violations de roles ou de partition (K=" + std::to_string(K) + ")";
          } catch (...) {
            sp->message.clear();  // le statut fait foi ; le fil principal fournit un message statique
          }
        }
        if (sp->exc || sp->status != PipelineStatus::kCompleteRegular) {
          pub_failed.store(true);  // premier defaut dans l'ordre des K : les ordres superieurs n'ont plus de tour
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
          rr.cards[K] = KCardinalities{st->events.size(), r.facets, r.deltas.size(), r.new_attachments, r.fusions, r.nodes};
          rr.total_events += st->events.size();
          rr.total_facets += r.facets;
          rr.total_fusions += r.fusions;
          rr.total_deltas += r.deltas.size();
          rr.total_nodes += r.nodes;
          if (opt.digest) {
            rr.digest_forest[K] = dg;
            dg_all.add(dg);
            rr.t_digest_ms += t_dg;
          }
          // Mutant `prefix-tamper-batch-levels` : altere les niveaux de lots du dernier ordre d'un PREFIXE (jamais
          // signes par le digest v4) — tue par la porte de prefixe etendue (niveaux de tous les lots).
          if (MHGP5_MUTANT("prefix-tamper-batch-levels") && rr.kmax_eff < 10 && K == rr.kmax_eff && !r.batch_levels.empty())
            r.batch_levels.push_back(r.batch_levels.back());
          if (opt.on_forest) opt.on_forest(K, st->events, r);
          rr.rss_mb[4] = std::max(rr.rss_mb[4], run_detail::rss_mb_now());
        } catch (...) {  // exception du callback (ou d'une publication) : defaut de CET ordre, a son tour
          if (!sp->exc) sp->exc = std::current_exception();  // jamais ecraser une exception anterieure de l'observateur
          pub_failed.store(true);
          lk.unlock();
          pub_cv.notify_all();
          observe(FoldPhase::kNotPublished);
          return;
        }
        // P0 (audit du 28 aout) : l'observateur terminal `kPublished` est appele AVANT l'ouverture irreversible du
        // tour K+1 — hors verrou (contrat de on_fold_phase), etat libere d'abord ; s'il leve, la faute est celle de
        // CET ordre (pub_failed, K+1 jamais publie), exactement comme une exception de callback.
        lk.unlock();
        st.reset();  // liberation : evenements et resultat de cet ordre
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
      });
    }
  } catch (...) {
    main_exc = std::current_exception();
  }
  // JONCTION EXPLICITE de tous les fils AVANT tout retour ou relance (surete 2),
  // puis ARBITRAGE (surete 3) : le plus petit ordre en defaut fait foi — un
  // defaut B des ordres en vol (tous inferieurs a l'ordre courant du fil
  // principal), sinon le defaut A ou l'exception du fil principal.
  drain();
  if (have_first) {
    if (first_exc) std::rethrow_exception(first_exc);
    rr.status = first_status;
    rr.message = first_message;
    return rr;
  }
  if (main_exc) std::rethrow_exception(main_exc);
  if (a_status != PipelineStatus::kCompleteRegular) {
    rr.status = a_status;
    rr.message = a_message;
    return rr;
  }
  rr.t_fold_wall_ms = ms(t_fold_wall);
  rr.t_fold_ms += t_prepare_total_ms;
  std::vector<BallData>().swap(balls);
  if (opt.digest) rr.digest_all = dg_all.hex();
  rr.rss_mb[5] = run_detail::rss_mb_now();
  rr.t_total_ms = ms(t_all);
  return rr;
}

// Impression standard des compteurs (lignes parsees par les campagnes).
inline void print_run(std::FILE* out, const char* family, int n, int coord, long long seed, const RunOptions& opt,
                      const RunResult& rr) {
  const GenerateStats& gs = rr.gen;
  const ExpandStats& es = rr.expand;
  std::fprintf(out, "payload=%s authority=status_terminal callbacks=provisional vertical_maps=none\n",
               kForestPayloadVersion);
  std::fprintf(out, "backend=%s\n", rr.backend_override ? "override_experimental (executeur de lane externe : non autoritaire)" : "cpu_reference");
  // PROFIL NOMME (docs/ECHELLE.md § 1, § 3.3) : l'objet complet (smax = 11, K = 1..10) ou un PREFIXE exact de la tour
  // (smax = s : K = 1..s-1, memes digests par ordre que l'objet complet) — jamais confondus dans un recu.
  // PORTEE DE LA TOUR (docs/ECHELLE.md § 1, § 3.3 ; audit du passage a l'echelle P2) : `tower_scope` distinct du
  // profil normatif `quantized_u16_input_only` ; « complet » = complet DANS le profil K <= 10, jamais une tour illimitee ;
  // un prefixe (smax = s : K = 1..s-1) a les memes digests par ordre que l'objet complet.
  if (rr.smax_eff == 11) std::fprintf(out, "tower_scope=profile_complete_k10 smax_requested=%llu smax_effective=%llu\n",
                                      (unsigned long long)opt.smax, (unsigned long long)rr.smax_eff);
  else std::fprintf(out, "tower_scope=prefix_k%llu smax_requested=%llu smax_effective=%llu (K = 1..%llu, prefixe exact de l'objet complet)\n",
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
               "generation rect_alive=%llu/%llu/%llu ancres=%llu/%llu/%llu candidats=%llu/%llu/%llu "
               "tues_profondeur=%llu/%llu/%llu ancres_w4=%llu ancres_w3=%llu ancres_secteurs=%llu/%llu ancres_cellules=%llu/%llu seeds_cellules=%llu/%llu grilles=%llu/%llu seeds=%llu/%llu completions_q4=%llu seeds_core_tues=%llu seeds_corde_tues=%llu float_cert=%llu/%llu repli=%llu "
               "ancres_hist=%llu/%llu/%llu hist_lignes=%llu/%llu/%llu hist_seuil=%llu/%llu/%llu hist_survivants=%llu/%llu/%llu jung=%llu/%llu/%llu\n",
               (unsigned long long)gs.rect_alive[0], (unsigned long long)gs.rect_alive[1], (unsigned long long)gs.rect_alive[2],
               (unsigned long long)gs.anchors[0], (unsigned long long)gs.anchors[1], (unsigned long long)gs.anchors[2],
               (unsigned long long)gs.candidates[0], (unsigned long long)gs.candidates[1], (unsigned long long)gs.candidates[2],
               (unsigned long long)gs.depth_killed[0], (unsigned long long)gs.depth_killed[1], (unsigned long long)gs.depth_killed[2],
               (unsigned long long)gs.anchors_killed_w4, (unsigned long long)gs.anchors_killed_w3, (unsigned long long)gs.anchors_killed_sectors[1], (unsigned long long)gs.anchors_killed_sectors[2],
               (unsigned long long)gs.anchors_killed_cells[1], (unsigned long long)gs.anchors_killed_cells[2],
               (unsigned long long)gs.seeds_killed_cells[1], (unsigned long long)gs.seeds_killed_cells[2],
               (unsigned long long)gs.grids_built[1], (unsigned long long)gs.grids_built[2],
               (unsigned long long)gs.seeds[0], (unsigned long long)gs.seeds[1], (unsigned long long)gs.q4_completions, (unsigned long long)gs.seeds_killed_core, (unsigned long long)gs.seeds_killed_chord,
               (unsigned long long)gs.float_cert_neg, (unsigned long long)gs.float_cert_pos, (unsigned long long)gs.float_fallback,
               (unsigned long long)gs.anchors_killed_hist[0], (unsigned long long)gs.anchors_killed_hist[1],
               (unsigned long long)gs.anchors_killed_hist[2],
               (unsigned long long)gs.hist_killed_rows[0], (unsigned long long)gs.hist_killed_rows[1],
               (unsigned long long)gs.hist_killed_rows[2],
               (unsigned long long)gs.hist_killed_thresh[0], (unsigned long long)gs.hist_killed_thresh[1],
               (unsigned long long)gs.hist_killed_thresh[2],
               (unsigned long long)gs.hist_survivors[0], (unsigned long long)gs.hist_survivors[1],
               (unsigned long long)gs.hist_survivors[2],
               (unsigned long long)gs.jung_cert_kill, (unsigned long long)gs.jung_cert_skip, (unsigned long long)gs.jung_fallback);
  std::fprintf(out,
               "enveloppe_cover active=%d "
               "q3_cover=%llu/%llu/%llu/%llu q3_query=%llu/%llu/%llu/%llu "
               "q4_cover=%llu/%llu/%llu/%llu q4_query=%llu/%llu/%llu/%llu "
               "(ancres/sites_avant/sites_apres/tests_transverses)\n",
               opt.cover_envelope_filter ? 1 : 0,
               (unsigned long long)gs.edge_envelope_anchors[1][0],
               (unsigned long long)gs.edge_envelope_sites_before[1][0],
               (unsigned long long)gs.edge_envelope_sites_after[1][0],
               (unsigned long long)gs.edge_envelope_cross_tests[1][0],
               (unsigned long long)gs.edge_envelope_anchors[1][1],
               (unsigned long long)gs.edge_envelope_sites_before[1][1],
               (unsigned long long)gs.edge_envelope_sites_after[1][1],
               (unsigned long long)gs.edge_envelope_cross_tests[1][1],
               (unsigned long long)gs.edge_envelope_anchors[2][0],
               (unsigned long long)gs.edge_envelope_sites_before[2][0],
               (unsigned long long)gs.edge_envelope_sites_after[2][0],
               (unsigned long long)gs.edge_envelope_cross_tests[2][0],
               (unsigned long long)gs.edge_envelope_anchors[2][1],
               (unsigned long long)gs.edge_envelope_sites_before[2][1],
               (unsigned long long)gs.edge_envelope_sites_after[2][1],
               (unsigned long long)gs.edge_envelope_cross_tests[2][1]);
#if defined(MHGP5_PROFILE_Q4)
  std::fprintf(out,
               "profil_enveloppe q3_tests_profondeur=%llu q4_covers=%llu q4_visites_handles=%llu "
               "q4_sites_cover_historiques=%llu q4_tests_coeur=%llu q4_completions=%llu "
               "q4_entrees_profondeur=%llu q4_tests_puissance=%llu\n",
               (unsigned long long)(gs.q3_cert[0] + gs.q3_cert[1] + gs.q3_cert[2]),
               (unsigned long long)gs.q4_covers_built, (unsigned long long)gs.q4_cover_visits,
               (unsigned long long)gs.q4_cover_sites, (unsigned long long)gs.q4_core_site_tests,
               (unsigned long long)gs.q4_completions, (unsigned long long)gs.q4_depth_entries,
               (unsigned long long)gs.q4_power_tests);
#endif
  // GRAND-LIVRE DU RAFFINEMENT POST-SEPARATION : identite exacte par lane.
  std::fprintf(out, "postsep L=%u parents=%llu/%llu/%llu produits=%llu/%llu/%llu base=%llu/%llu/%llu emis=%llu/%llu/%llu tues=%llu/%llu/%llu etats=%llu/%llu/%llu comptages=%llu/%llu/%llu nœuds=%llu/%llu/%llu coins=%llu/%llu/%llu rollbacks=%llu/%llu/%llu regressions=%llu/%llu/%llu\n",
               opt.postsep_refine_levels, (unsigned long long)gs.postsep_parent_rects[0],
               (unsigned long long)gs.postsep_parent_rects[1], (unsigned long long)gs.postsep_parent_rects[2],
               (unsigned long long)gs.postsep_emitted_rects[0], (unsigned long long)gs.postsep_emitted_rects[1],
               (unsigned long long)gs.postsep_emitted_rects[2],
               (unsigned long long)gs.postsep_base_mass[0], (unsigned long long)gs.postsep_base_mass[1],
               (unsigned long long)gs.postsep_base_mass[2], (unsigned long long)gs.postsep_emitted_mass[0],
               (unsigned long long)gs.postsep_emitted_mass[1], (unsigned long long)gs.postsep_emitted_mass[2],
               (unsigned long long)gs.postsep_killed_mass[0], (unsigned long long)gs.postsep_killed_mass[1],
               (unsigned long long)gs.postsep_killed_mass[2], (unsigned long long)gs.postsep_subrects[0],
               (unsigned long long)gs.postsep_subrects[1], (unsigned long long)gs.postsep_subrects[2],
               (unsigned long long)gs.postsep_core_evals[0], (unsigned long long)gs.postsep_core_evals[1],
               (unsigned long long)gs.postsep_core_evals[2], (unsigned long long)gs.postsep_core_nodes[0],
               (unsigned long long)gs.postsep_core_nodes[1], (unsigned long long)gs.postsep_core_nodes[2],
               (unsigned long long)gs.postsep_corner_evals[0], (unsigned long long)gs.postsep_corner_evals[1],
               (unsigned long long)gs.postsep_corner_evals[2], (unsigned long long)gs.postsep_rollbacks[0],
               (unsigned long long)gs.postsep_rollbacks[1], (unsigned long long)gs.postsep_rollbacks[2],
               (unsigned long long)gs.postsep_core_regressions[0], (unsigned long long)gs.postsep_core_regressions[1],
               (unsigned long long)gs.postsep_core_regressions[2]);
  std::fprintf(out, "ouvriers wspd=%llu/%llu/%llu rects=%llu/%llu/%llu rle=%llu prefiltre=%llu census=%llu expansion=%llu fold=%llu\n",
               (unsigned long long)gs.workers_wspd[0], (unsigned long long)gs.workers_wspd[1], (unsigned long long)gs.workers_wspd[2],
               (unsigned long long)gs.workers_rects[0], (unsigned long long)gs.workers_rects[1], (unsigned long long)gs.workers_rects[2],
               (unsigned long long)rr.rle_workers, (unsigned long long)es.workers_prefilter, (unsigned long long)es.workers_census,
               (unsigned long long)es.workers_expand, (unsigned long long)rr.fold_workers);
  std::fprintf(out,
               "temps_ms index=%.1f gen=%.1f (wspd %.1f/%.1f/%.1f rects %.1f/%.1f/%.1f) rle=%.1f prefiltre=%.1f "
               "census=%.1f comptage=%.1f expansion=%.1f fold=%.1f (tri %.1f intern %.1f fusion %.1f reduce %.1f) digest=%.1f\n",
               rr.t_index_ms, rr.t_gen_ms, gs.t_wspd_ms[0], gs.t_wspd_ms[1], gs.t_wspd_ms[2], gs.t_rects_ms[0],
               gs.t_rects_ms[1], gs.t_rects_ms[2], rr.t_rle_ms, rr.t_prefilter_ms, rr.t_census_ms, rr.t_count_ms,
               rr.t_expand_ms, rr.t_fold_ms, rr.t_fold_sort_ms, rr.t_fold_intern_ms, rr.t_fold_merge_ms,
               rr.t_fold_reduce_ms, rr.t_digest_ms);
  std::fprintf(out, "temps_mur_ms=%.1f (etages A et B du fold pipelines : fold+digest ci-dessus sont des cumuls par etage, pas le mur)\n",
               rr.t_total_ms);
  std::fprintf(out, "temps_fold_mur_ms=%.1f (etages A et B, fold_inflight=%d, pic_mesure_en_vol=%llu)\n", rr.t_fold_wall_ms,
               opt.fold_inflight, (unsigned long long)rr.peak_fold_inflight);
  std::fprintf(out, "rss_mb apres_generation=%.0f apres_rle=%.0f apres_prefiltre=%.0f apres_census=%.0f max_fold=%.0f fin=%.0f\n",
               rr.rss_mb[0], rr.rss_mb[1], rr.rss_mb[2], rr.rss_mb[3], rr.rss_mb[4], rr.rss_mb[5]);
  for (u64 K = 1; K <= rr.kmax_eff; ++K)
    std::fprintf(out, "cardinalites K=%llu evenements=%llu facettes=%llu deltas=%llu attachements=%llu fusions=%llu noeuds=%llu\n",
                 (unsigned long long)K, (unsigned long long)rr.cards[K].events, (unsigned long long)rr.cards[K].facets,
                 (unsigned long long)rr.cards[K].deltas, (unsigned long long)rr.cards[K].attachments,
                 (unsigned long long)rr.cards[K].fusions, (unsigned long long)rr.cards[K].nodes);
  if (opt.digest) {
    if (!rr.digest_raw_candidates.empty())
      std::fprintf(out, "digest_raw_candidates=%s\n", rr.digest_raw_candidates.c_str());
    std::fprintf(out, "digest_balls=%s\n", rr.digest_balls.c_str());
    for (u64 K = 1; K <= rr.kmax_eff; ++K)
      std::fprintf(out, "digest_forest_K%llu=%s\n", (unsigned long long)K, rr.digest_forest[K].c_str());
    std::fprintf(out, "digest_all=%s\n", rr.digest_all.c_str());
  }
}

// Code de sortie transactionnel d'un statut.
inline int status_exit_code(PipelineStatus s) {
  switch (s) {
    case PipelineStatus::kCompleteRegular: return 0;
    case PipelineStatus::kInvariantViolated: return 3;
    default: return 2;
  }
}

}  // namespace mhgp5
