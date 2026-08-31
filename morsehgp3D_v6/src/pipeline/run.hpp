// MorseHGP3D v6 — le pipeline complet, en bibliotheque : nuage -> foret HGP.
//
//   index -> generation (front fusionne, trois lanes) -> RLE -> prefiltre de
//   profondeur -> census -> expansion des plateaux -> folds par K, STREAMES.
//
// Chaque fold est construit, signe, compte puis LIBERE avant le suivant :
// la residence est bornee par `fold_inflight + 1` ordres (jamais dix forets
// residentes). Statuts transactionnels :
//   complete_regular | unsupported_degeneracy | resource_exhausted |
//   invalid_input | invariant_violated.
// Le consommateur recoit chaque ForestResult par callback (`on_forest`),
// PROVISOIRE jusqu'au statut terminal ; la signature au format v4 est
// calculee ici pour la porte de conformite v5 ≡ v6.
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

namespace mhgp6 {

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
  // Diagnostic opt-in : signe le multiensemble trie AVANT RLE.
  bool diagnostic_raw_candidates_digest = false;
  int fold_inflight = 2;
  // Sonde E6 opt-in (--sonde-e6) : lecture seule, objet inchange.
  bool e6_probe = false;
  // Experimentation E3/G16 par bras (kOff = production).
  E3G16Mode e3_mode = E3G16Mode::kOff;
  std::function<void(u64 K, FoldPhase phase)> on_fold_phase;
  size_t pretest_query_min_points = 512;
  size_t cell_grid_min_sites = kCellGridMinSites;
  std::function<void(u64 K, const std::vector<ForestEvent>& events, const ForestResult& r)> on_forest;
};

struct KCardinalities {
  u64 events = 0, facets = 0, deltas = 0, attachments = 0, fusions = 0, nodes = 0;
};

struct RunResult {
  PipelineStatus status = PipelineStatus::kCompleteRegular;
  std::string message;
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
  double rss_mb[6] = {0, 0, 0, 0, 0, 0};
  double t_total_ms = 0;
  u64 fold_workers = 0, rle_workers = 0;
  u64 peak_fold_inflight = 0;
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

inline double ms(std::chrono::steady_clock::time_point t0) {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}
}  // namespace run_detail

inline constexpr u64 kSmaxProfile = 11;

// CONTRAT DE PAYLOAD (arbitrage V3, inchange en doctrine) : forets
// horizontales par ordre K, retention de toutes les facettes, autorite au
// statut terminal, digest canonique au format `mhgp4-digest-v1`.
inline constexpr const char* kForestPayloadVersion = "mhgp6-forests-horizontal-v1";

inline constexpr size_t kShellCapProfile = kBallShellMax;

inline bool validate_run_options(const std::vector<InputPoint>& in, const RunOptions& opt, std::string* why) {
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
  go.s = opt.s;
  go.smax = rr.smax_eff;
  go.threads = opt.threads;
  go.e6_probe = opt.e6_probe;
  go.e3_mode = opt.e3_mode;
  generate_candidates(ix, go, &cands, &rr.gen);
  rr.t_gen_ms = ms(t_g);
  rr.rss_mb[0] = run_detail::rss_mb_now();
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
      return rr;
    }
  }
  if (rr.gen.invariant_jneg) {
    rr.status = PipelineStatus::kInvariantViolated;
    rr.message = "invariant : seed q4 aigu avec J < 0 (inatteignable par theoreme, MATHEMATIQUES § 2) : " +
                 std::to_string(rr.gen.invariant_jneg) + " occurrence(s)";
    invalidate_provisional(&rr);
    return rr;
  }
  const auto t_r = std::chrono::steady_clock::now();
  rr.emitted = cands.size();
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
  rr.rss_mb[1] = run_detail::rss_mb_now();
  rr.expand.unique_balls = cands.size();

  if (!candidates_capacity_ok(cands.size())) {
    rr.status = PipelineStatus::kResourceExhausted;
    rr.message = "resource_exhausted : plus de 2^32-1 boules uniques (indices u32 du prefiltre)";
    invalidate_provisional(&rr);
    return rr;
  }
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
    invalidate_provisional(&rr);
    return rr;
  }
  rr.t_census_ms = ms(t_c);  // arrete AVANT le digest post-prefiltre (audit du 31 aout)
  if (opt.digest) {
    const auto t_dp = std::chrono::steady_clock::now();
    rr.digest_postprefilter = digest_postprefilter_v6(cands, surv);
    rr.t_digest_ms += ms(t_dp);
  }
  std::vector<Survivor>().swap(surv);
  rr.rss_mb[3] = run_detail::rss_mb_now();

  // GARDES DE CAPACITE DE TOUS LES ORDRES AVANT TOUTE PUBLICATION.
  const auto t_e = std::chrono::steady_clock::now();
  const std::vector<KCount> kc = count_events_by_k(ix, balls, rr.kmax_eff, opt.threads);
  for (u64 K = 1; K <= rr.kmax_eff; ++K) {
    std::string why;
    if (!fold_capacity_ok(kc[K].events, kc[K].incidences, &why)) {
      rr.status = PipelineStatus::kResourceExhausted;
      rr.message = "fold K=" + std::to_string(K) + " : " + why;
      invalidate_provisional(&rr);
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
  // STREAMING PAR K, PIPELINE A DEUX ETAGES — transcription v5 (suretes 1-3 de
  // l'audit du 28 aout 2026 conservees : possession du slot avant demarrage,
  // jonction explicite avant tout retour, premier defaut dans l'ordre des K).
  struct Stage {
    u64 K = 0;
    std::vector<ForestEvent> events;
    FoldPrepared prep;
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
      expand_events_k(ix, balls, K, rr.kmax_eff, opt.threads, &st->events, &rr.expand);
      rr.t_expand_ms += ms(t_k);
      bool count_mismatch = st->events.size() != kc[K].events;
      if (MHGP6_MUTANT("fold-inject-a-failure-k2") && K == 2) count_mismatch = true;
      if (count_mismatch) {
        a_status = PipelineStatus::kInvariantViolated;
        a_message = "invariant : comptage par K != expansion (K=" + std::to_string(K) + ")";
        phase(K, FoldPhase::kStageAFailed);
        break;
      }
      const auto t_f = std::chrono::steady_clock::now();
      st->prep = prepare_fold(st->events, opt.threads);
      t_prepare_total_ms += ms(t_f);
      if (!st->prep.r.refusal.empty()) {
        a_status = PipelineStatus::kInvariantViolated;
        a_message = "invariant : refus de fold apres la garde de capacite (K=" + std::to_string(K) + ")";
        phase(K, FoldPhase::kStageAFailed);
        break;
      }
      if ((int)slots.size() >= inflight && !reap_front()) break;
      slots.push_back(std::make_unique<BSlot>());
      BSlot* sp = slots.back().get();
      sp->K = K;
      sp->t = std::thread([&rr, &opt, &dg_all, &pub_mutex, &pub_cv, &next_publish, &pub_failed, &b_inflight, &b_peak,
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
          r = reduce_fold(std::move(st->prep));
          t_fold_local = run_detail::ms(t_r);
          if (MHGP6_MUTANT("fold-inject-b-exception-k3") && K == 3)
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
        std::unique_lock<std::mutex> lk(pub_mutex);
        pub_cv.wait(lk, [&] { return pub_failed.load() || next_publish == K; });
        if (pub_failed.load()) {
          lk.unlock();
          observe(FoldPhase::kNotPublished);
          return;
        }
        sp->decided = true;
        if (!sp->exc && (r.attach_violations || r.birth_violations || r.partition_violations)) {
          sp->status = PipelineStatus::kInvariantViolated;
          try {
            sp->message = "invariant : violations de roles ou de partition (K=" + std::to_string(K) + ")";
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
              KCardinalities{st->events.size(), r.facets, r.deltas.size(), r.new_attachments, r.fusions, r.nodes};
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
          if (MHGP6_MUTANT("prefix-tamper-batch-levels") && rr.kmax_eff < 10 && K == rr.kmax_eff &&
              !r.batch_levels.empty())
            r.batch_levels.push_back(r.batch_levels.back());
          if (opt.on_forest) opt.on_forest(K, st->events, r);
          rr.rss_mb[4] = std::max(rr.rss_mb[4], run_detail::rss_mb_now());
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
      });
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
    return rr;
  }
  if (main_exc) std::rethrow_exception(main_exc);
  if (a_status != PipelineStatus::kCompleteRegular) {
    rr.status = a_status;
    rr.message = a_message;
    invalidate_provisional(&rr);
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

// Impression standard des compteurs (lignes parsees par les campagnes v6).
inline void print_run(std::FILE* out, const char* family, int n, int coord, long long seed, const RunOptions& opt,
                      const RunResult& rr) {
  const GenerateStats& gs = rr.gen;
  const ExpandStats& es = rr.expand;
  std::fprintf(out, "payload=%s authority=status_terminal callbacks=provisional vertical_maps=none\n",
               kForestPayloadVersion);
  std::fprintf(out, "backend=cpu_reference\n");
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
  std::fprintf(out, "temps_fold_mur_ms=%.1f (etages A et B, fold_inflight=%d, pic_mesure_en_vol=%llu)\n",
               rr.t_fold_wall_ms, opt.fold_inflight, (unsigned long long)rr.peak_fold_inflight);
  std::fprintf(out,
               "rss_mb apres_generation=%.0f apres_rle=%.0f apres_prefiltre=%.0f apres_census=%.0f max_fold=%.0f "
               "fin=%.0f\n",
               rr.rss_mb[0], rr.rss_mb[1], rr.rss_mb[2], rr.rss_mb[3], rr.rss_mb[4], rr.rss_mb[5]);
  for (u64 K = 1; K <= rr.kmax_eff; ++K)
    std::fprintf(
        out, "cardinalites K=%llu evenements=%llu facettes=%llu deltas=%llu attachements=%llu fusions=%llu noeuds=%llu\n",
        (unsigned long long)K, (unsigned long long)rr.cards[K].events, (unsigned long long)rr.cards[K].facets,
        (unsigned long long)rr.cards[K].deltas, (unsigned long long)rr.cards[K].attachments,
        (unsigned long long)rr.cards[K].fusions, (unsigned long long)rr.cards[K].nodes);
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

}  // namespace mhgp6
