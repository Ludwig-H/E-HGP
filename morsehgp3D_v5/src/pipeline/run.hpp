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
#include <string>
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
  // Appele pour chaque K croissant, AVANT liberation du resultat.
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
  std::string digest_balls, digest_all;
  std::vector<std::string> digest_forest;  // indexee par K
  double t_index_ms = 0, t_gen_ms = 0, t_rle_ms = 0, t_prefilter_ms = 0, t_census_ms = 0, t_expand_ms = 0,
         t_fold_ms = 0;
};

namespace run_detail {
inline double ms(std::chrono::steady_clock::time_point t0) {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}
}  // namespace run_detail

// Profil : K_max <= 10 ⟺ smax <= 11 (les tampons d'interieurs et les tableaux
// par K sont dimensionnes pour ce profil ; au-dela, refus explicite).
inline constexpr u64 kSmaxProfile = 11;

// GARDES DE BIBLIOTHEQUE (pas seulement de CLI) : toute option hors profil et
// toute entree qui ne definit pas l'objet sont refusees AVANT tout calcul,
// avec le statut contractuel — jamais un debordement.
inline bool validate_run_options(const std::vector<InputPoint>& in, const RunOptions& opt, std::string* why) {
  if (in.size() < 2) { *why = "invalid_input : moins de deux points"; return false; }
  if (opt.s < 1) { *why = "invalid_input : separation s < 1"; return false; }
  if (opt.smax < 2 || opt.smax > kSmaxProfile) { *why = "invalid_input : smax hors du profil [2, 11]"; return false; }
  if (opt.threads < 1) { *why = "invalid_input : threads < 1"; return false; }
  if (opt.shell_cap < 4) { *why = "invalid_input : plafond de coquille < 4 (un support q4 a quatre points)"; return false; }
  return true;
}

inline RunResult run_pipeline(const std::vector<InputPoint>& in, const RunOptions& opt) {
  using run_detail::ms;
  RunResult rr;
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
  go.s = opt.s;
  go.smax = rr.smax_eff;
  go.threads = opt.threads;
  generate_candidates(ix, go, &cands, &rr.gen);
  rr.t_gen_ms = ms(t_g);
  const auto t_r = std::chrono::steady_clock::now();
  rr.emitted = cands.size();
  rle_candidates(&cands);
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
  const std::vector<KCount> kc = count_events_by_k(ix, balls, rr.kmax_eff);
  for (u64 K = 1; K <= rr.kmax_eff; ++K) {
    std::string why;
    if (!fold_capacity_ok(kc[K].events, kc[K].incidences, &why)) {
      rr.status = PipelineStatus::kResourceExhausted;
      rr.message = "fold K=" + std::to_string(K) + " : " + why;
      return rr;
    }
  }
  rr.t_expand_ms += ms(t_e);

  if (opt.digest) rr.digest_balls = digest_balls_v4(cands);
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
  std::vector<ForestEvent> events;
  for (u64 K = 1; K <= rr.kmax_eff; ++K) {
    const auto t_k = std::chrono::steady_clock::now();
    expand_events_k(ix, balls, K, rr.kmax_eff, opt.threads, &events, &rr.expand);
    rr.t_expand_ms += ms(t_k);
    if (events.size() != kc[K].events) {
      rr.status = PipelineStatus::kInvariantViolated;
      rr.message = "invariant : comptage par K != expansion (K=" + std::to_string(K) + ")";
      return rr;
    }
    const auto t_f = std::chrono::steady_clock::now();
    ForestResult r = build_forest(events);
    rr.t_fold_ms += ms(t_f);
    if (!r.refusal.empty()) {  // impossible apres la garde amont ; traite en invariant
      rr.status = PipelineStatus::kInvariantViolated;
      rr.message = "invariant : refus de fold apres la garde de capacite (K=" + std::to_string(K) + ")";
      return rr;
    }
    if (r.attach_violations || r.birth_violations || r.partition_violations) {
      rr.status = PipelineStatus::kInvariantViolated;
      rr.message = "invariant : violations de roles ou de partition (K=" + std::to_string(K) + ")";
      return rr;
    }
    rr.cards[K] = KCardinalities{events.size(), r.facets, r.deltas.size(), r.new_attachments, r.fusions, r.nodes};
    rr.total_events += events.size();
    rr.total_facets += r.facets;
    rr.total_fusions += r.fusions;
    rr.total_deltas += r.deltas.size();
    rr.total_nodes += r.nodes;
    if (opt.digest) {
      rr.digest_forest[K] = digest_forest_v4((u32)K, r);
      dg_all.add(rr.digest_forest[K]);
    }
    if (opt.on_forest) opt.on_forest(K, events, r);
    std::vector<ForestEvent>().swap(events);
  }
  std::vector<BallData>().swap(balls);
  if (opt.digest) rr.digest_all = dg_all.hex();
  return rr;
}

// Impression standard des compteurs (lignes parsees par les campagnes).
inline void print_run(std::FILE* out, const char* family, int n, int coord, long long seed, const RunOptions& opt,
                      const RunResult& rr) {
  const GenerateStats& gs = rr.gen;
  const ExpandStats& es = rr.expand;
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
               "tues_profondeur=%llu/%llu/%llu ancres_w4=%llu seeds_core_tues=%llu float_cert=%llu/%llu repli=%llu "
               "jung=%llu/%llu/%llu\n",
               (unsigned long long)gs.rect_alive[0], (unsigned long long)gs.rect_alive[1], (unsigned long long)gs.rect_alive[2],
               (unsigned long long)gs.anchors[0], (unsigned long long)gs.anchors[1], (unsigned long long)gs.anchors[2],
               (unsigned long long)gs.candidates[0], (unsigned long long)gs.candidates[1], (unsigned long long)gs.candidates[2],
               (unsigned long long)gs.depth_killed[0], (unsigned long long)gs.depth_killed[1], (unsigned long long)gs.depth_killed[2],
               (unsigned long long)gs.anchors_killed_w4, (unsigned long long)gs.seeds_killed_core,
               (unsigned long long)gs.float_cert_neg, (unsigned long long)gs.float_cert_pos, (unsigned long long)gs.float_fallback,
               (unsigned long long)gs.jung_cert_kill, (unsigned long long)gs.jung_cert_skip, (unsigned long long)gs.jung_fallback);
  std::fprintf(out, "ouvriers wspd=%llu/%llu/%llu rects=%llu/%llu/%llu prefiltre=%llu census=%llu expansion=%llu\n",
               (unsigned long long)gs.workers_wspd[0], (unsigned long long)gs.workers_wspd[1], (unsigned long long)gs.workers_wspd[2],
               (unsigned long long)gs.workers_rects[0], (unsigned long long)gs.workers_rects[1], (unsigned long long)gs.workers_rects[2],
               (unsigned long long)es.workers_prefilter, (unsigned long long)es.workers_census, (unsigned long long)es.workers_expand);
  std::fprintf(out,
               "temps_ms index=%.1f gen=%.1f (wspd %.1f/%.1f/%.1f rects %.1f/%.1f/%.1f) rle=%.1f prefiltre=%.1f "
               "census=%.1f expansion=%.1f fold=%.1f\n",
               rr.t_index_ms, rr.t_gen_ms, gs.t_wspd_ms[0], gs.t_wspd_ms[1], gs.t_wspd_ms[2], gs.t_rects_ms[0],
               gs.t_rects_ms[1], gs.t_rects_ms[2], rr.t_rle_ms, rr.t_prefilter_ms, rr.t_census_ms, rr.t_expand_ms,
               rr.t_fold_ms);
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
