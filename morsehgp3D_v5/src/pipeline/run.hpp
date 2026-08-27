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

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <exception>
#include <thread>
#include <mutex>
#include <deque>
#include <condition_variable>
#include <vector>

#include "../tree/cloud_index.hpp"
#include "digest.hpp"
#include "expand.hpp"
#include "generate.hpp"

namespace mhgp5 {

struct RunOptions {
  i64 s = 8;
  u64 smax = 11;
  int threads = 1;
  size_t shell_cap = 12;
  bool digest = false;
  // Ordres K dont la reduction (etage B, sequentielle par ordre) peut etre en
  // vol simultanement ; la publication reste dans l'ordre des K et la sortie
  // bit-identique ; residence bornee a fold_inflight + 1 ordres.
  int fold_inflight = 2;
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
  std::string digest_balls, digest_all;
  std::vector<std::string> digest_forest;  // indexee par K
  double t_index_ms = 0, t_gen_ms = 0, t_rle_ms = 0, t_prefilter_ms = 0, t_census_ms = 0, t_expand_ms = 0,
         t_fold_ms = 0, t_count_ms = 0, t_digest_ms = 0;
  double t_fold_sort_ms = 0, t_fold_intern_ms = 0, t_fold_merge_ms = 0, t_fold_reduce_ms = 0;
  double t_fold_wall_ms = 0;  // mur des etages A et B (premier ordre expanse -> derniere publication)
  double t_total_ms = 0;  // temps MUR de run_pipeline
  u64 fold_workers = 0, rle_workers = 0;
};

namespace run_detail {
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
inline constexpr size_t kShellCapProfile = 12;

inline bool validate_run_options(const std::vector<InputPoint>& in, const RunOptions& opt, std::string* why) {
  if (in.size() < 2) { *why = "invalid_input : moins de deux points"; return false; }
  if (opt.s < 1) { *why = "invalid_input : separation s < 1"; return false; }
  if (opt.smax < 2 || opt.smax > kSmaxProfile) { *why = "invalid_input : smax hors du profil [2, 11]"; return false; }
  if (opt.threads < 1) { *why = "invalid_input : threads < 1"; return false; }
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
  go.q3_override = opt.q3_override;
  go.q4_override = opt.q4_override;
  rr.backend_override = (bool)opt.q3_override || (bool)opt.q4_override;
  go.s = opt.s;
  go.smax = rr.smax_eff;
  go.threads = opt.threads;
  generate_candidates(ix, go, &cands, &rr.gen);
  rr.t_gen_ms = ms(t_g);
  if (rr.gen.invariant_jneg) {
    rr.status = PipelineStatus::kInvariantViolated;
    rr.message = "invariant : seed q4 aigu avec J < 0 (inatteignable par theoreme, MATHEMATIQUES § 10) : " +
                 std::to_string(rr.gen.invariant_jneg) + " occurrence(s)";
    return rr;
  }
  const auto t_r = std::chrono::steady_clock::now();
  rr.emitted = cands.size();
  rr.rle_workers = rle_candidates(&cands, opt.threads);
  rr.t_rle_ms = ms(t_r);
  rr.expand.unique_balls = cands.size();

  const auto t_p = std::chrono::steady_clock::now();
  std::vector<Survivor> surv;
  prefilter_balls(ix, cands, rr.smax_eff, opt.threads, &surv, &rr.expand);
  rr.t_prefilter_ms = ms(t_p);
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
  struct Stage {
    u64 K = 0;
    std::vector<ForestEvent> events;
    FoldPrepared prep;
  };
  struct BSlot {
    std::thread t;
    u64 K = 0;
    std::exception_ptr exc;
    PipelineStatus status = PipelineStatus::kCompleteRegular;
    std::string message;
  };
  const int inflight = std::max(1, opt.fold_inflight);
  std::deque<std::unique_ptr<BSlot>> slots;
  std::mutex pub_mutex;
  std::condition_variable pub_cv;
  u64 next_publish = 1;
  bool pub_failed = false;
  // CONTRAT D'EXCEPTION : une exception levee par `on_forest` (ou par l'etage
  // B) est capturee dans son fil, TOUS les fils sont joints (le joiner RAII
  // libere d'abord les ordres en attente de publication), puis elle est
  // relancee par run_pipeline dans le fil appelant — jamais std::terminate.
  // Le joiner agit aussi sur toute sortie anticipee du fil principal
  // (exception ou refus de l'etage A compris).
  struct BJoiner {
    std::deque<std::unique_ptr<BSlot>>& s;
    std::mutex& m;
    std::condition_variable& cv;
    bool& failed;
    ~BJoiner() {
      {
        std::lock_guard<std::mutex> lk(m);
        failed = true;
      }
      cv.notify_all();
      for (auto& b : s)
        if (b->t.joinable()) b->t.join();
    }
  } bjoiner{slots, pub_mutex, pub_cv, pub_failed};
  // Le fil principal n'ecrit que dans ses propres cumuls ; les cumuls des
  // fils d'arriere-plan (t_fold_*, digests, cartes, totaux) ne sont ecrits
  // que sous le verrou de publication et lus apres le dernier join.
  double t_prepare_total_ms = 0;
  // Joint le plus ancien ordre en vol et propage son verdict (le premier
  // defaut dans l'ordre des K fait foi ; les ordres suivants sont joints par
  // le joiner sans publier).
  const auto reap_front = [&]() -> bool {
    std::unique_ptr<BSlot> b = std::move(slots.front());
    slots.pop_front();
    b->t.join();
    if (b->exc) std::rethrow_exception(b->exc);
    if (b->status != PipelineStatus::kCompleteRegular) {
      rr.status = b->status;
      rr.message = b->message;
      return false;
    }
    return true;
  };
  const auto t_fold_wall = std::chrono::steady_clock::now();
  for (u64 K = 1; K <= rr.kmax_eff; ++K) {
    auto st = std::make_unique<Stage>();
    st->K = K;
    const auto t_k = std::chrono::steady_clock::now();
    expand_events_k(ix, balls, K, rr.kmax_eff, opt.threads, &st->events, &rr.expand);
    rr.t_expand_ms += ms(t_k);
    if (st->events.size() != kc[K].events) {
      rr.status = PipelineStatus::kInvariantViolated;
      rr.message = "invariant : comptage par K != expansion (K=" + std::to_string(K) + ")";
      return rr;
    }
    const auto t_f = std::chrono::steady_clock::now();
    st->prep = prepare_fold(st->events, opt.threads);
    t_prepare_total_ms += ms(t_f);
    if (!st->prep.r.refusal.empty()) {  // impossible apres la garde amont ; traite en invariant
      rr.status = PipelineStatus::kInvariantViolated;
      rr.message = "invariant : refus de fold apres la garde de capacite (K=" + std::to_string(K) + ")";
      return rr;
    }
    if ((int)slots.size() >= inflight && !reap_front()) return rr;
    auto slot = std::make_unique<BSlot>();
    slot->K = K;
    BSlot* sp = slot.get();
    slot->t = std::thread([&rr, &opt, &dg_all, &pub_mutex, &pub_cv, &next_publish, &pub_failed, sp, st = std::move(st)]() mutable {
      try {
        const u64 K = st->K;
        const auto t_r = std::chrono::steady_clock::now();
        ForestResult r = reduce_fold(std::move(st->prep));
        const double t_fold_local = run_detail::ms(t_r);
        std::string dg;
        double t_dg = 0;
        if (opt.digest) {
          const auto t_d = std::chrono::steady_clock::now();
          dg = digest_forest_v4((u32)K, r);
          t_dg = run_detail::ms(t_d);
        }
        std::unique_lock<std::mutex> lk(pub_mutex);
        pub_cv.wait(lk, [&] { return pub_failed || next_publish == K; });
        if (pub_failed) return;
        rr.t_fold_ms += t_fold_local;
        rr.t_fold_sort_ms += r.t_sort_ms;
        rr.t_fold_intern_ms += r.t_intern_ms;
        rr.t_fold_merge_ms += r.t_merge_ms;
        rr.t_fold_reduce_ms += r.t_reduce_ms + r.t_partition_ms;
        rr.fold_workers = std::max(rr.fold_workers, r.workers);
        if (r.attach_violations || r.birth_violations || r.partition_violations) {
          sp->status = PipelineStatus::kInvariantViolated;
          sp->message = "invariant : violations de roles ou de partition (K=" + std::to_string(K) + ")";
          pub_failed = true;
          lk.unlock();
          pub_cv.notify_all();
          return;
        }
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
        if (opt.on_forest) opt.on_forest(K, st->events, r);
        next_publish = K + 1;
        lk.unlock();
        pub_cv.notify_all();
        // Liberation : evenements et resultat de cet ordre.
        st.reset();
      } catch (...) {
        sp->exc = std::current_exception();
        {
          std::lock_guard<std::mutex> lk(pub_mutex);
          pub_failed = true;
        }
        pub_cv.notify_all();
      }
    });
    slots.push_back(std::move(slot));
  }
  while (!slots.empty())
    if (!reap_front()) return rr;
  rr.t_fold_wall_ms = ms(t_fold_wall);
  rr.t_fold_ms += t_prepare_total_ms;
  std::vector<BallData>().swap(balls);
  if (opt.digest) rr.digest_all = dg_all.hex();
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
               "tues_profondeur=%llu/%llu/%llu ancres_w4=%llu ancres_w3=%llu ancres_secteurs=%llu/%llu seeds_core_tues=%llu seeds_corde_tues=%llu float_cert=%llu/%llu repli=%llu "
               "jung=%llu/%llu/%llu\n",
               (unsigned long long)gs.rect_alive[0], (unsigned long long)gs.rect_alive[1], (unsigned long long)gs.rect_alive[2],
               (unsigned long long)gs.anchors[0], (unsigned long long)gs.anchors[1], (unsigned long long)gs.anchors[2],
               (unsigned long long)gs.candidates[0], (unsigned long long)gs.candidates[1], (unsigned long long)gs.candidates[2],
               (unsigned long long)gs.depth_killed[0], (unsigned long long)gs.depth_killed[1], (unsigned long long)gs.depth_killed[2],
               (unsigned long long)gs.anchors_killed_w4, (unsigned long long)gs.anchors_killed_w3, (unsigned long long)gs.anchors_killed_sectors[1], (unsigned long long)gs.anchors_killed_sectors[2], (unsigned long long)gs.seeds_killed_core, (unsigned long long)gs.seeds_killed_chord,
               (unsigned long long)gs.float_cert_neg, (unsigned long long)gs.float_cert_pos, (unsigned long long)gs.float_fallback,
               (unsigned long long)gs.jung_cert_kill, (unsigned long long)gs.jung_cert_skip, (unsigned long long)gs.jung_fallback);
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
  std::fprintf(out, "temps_fold_mur_ms=%.1f (etages A et B, %d ordre(s) en vol)\n", rr.t_fold_wall_ms, std::max(1, opt.fold_inflight));
  for (u64 K = 1; K <= rr.kmax_eff; ++K)
    std::fprintf(out, "cardinalites K=%llu evenements=%llu facettes=%llu deltas=%llu attachements=%llu fusions=%llu noeuds=%llu\n",
                 (unsigned long long)K, (unsigned long long)rr.cards[K].events, (unsigned long long)rr.cards[K].facets,
                 (unsigned long long)rr.cards[K].deltas, (unsigned long long)rr.cards[K].attachments,
                 (unsigned long long)rr.cards[K].fusions, (unsigned long long)rr.cards[K].nodes);
  if (opt.digest) {
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
