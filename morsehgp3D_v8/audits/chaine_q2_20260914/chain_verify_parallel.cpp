// Auditeur B (14 sept. 2026) — vérificateur de la chaîne parallèle front + census q2
// (run_wspd_q2_census_parallel) : pour W ∈ {1,2,3,4,8} fils et deux tailles de lots, les supports
// émis par tous les slots, réunis, doivent être exactement ceux de la force brute (chaque paire
// à moins de Kmax intérieurs stricts, une seule fois, avec intérieurs, coquille complète et clé),
// identiques d'un W à l'autre (condensé canonique indépendant de l'ordre), sans doublon entre slots ;
// les compteurs discrets (rectangles, candidates, admises, rejetées) doivent égaler ceux du chemin
// série run_wspd_q2_census avec les mêmes options.
// Avec -DMHGP8_AUDIT_DONATE (redistribution dynamique, WspdQ2Schedule), chaque appel parallèle est
// répété pour les ordonnancements Coarse, Donate{64,64} et Donate{1,1} (file minimale, adversarial),
// Avec -DMHGP8_AUDIT_COOP (tranche 17, équipe coopérative front + census à continuations), chaque combinaison
// SharedBlocks/Individual est aussi exécutée par run_wspd_q2_census_cooperative pour W ∈ {1,2,3,4,8} et quatre
// réglages (min_b_size 1 ou 64, quantum 1 ou 256, file 1 ou 8) : mêmes vérifications (force brute, condensé,
// compteurs globaux du pipeline égaux au chemin série), plus les identités de continuation/don annoncées.
// Avec -DMHGP8_AUDIT_RANGES (tranche 18, plages d'ancres et Pool partagé), chaque combinaison SharedBlocks/Individual
// est aussi exécutée par run_wspd_q2_census_ranges pour W ∈ {1,2,3,4,8} et quatre réglages (grain 1 ou 64, file 1 ou 8) :
// force brute, condensé, compteurs globaux égaux au chemin série, identités de plages et de dons, vivacité.
// Avec -DMHGP8_AUDIT_BATCHED (tranche 19, petits census entrelacés), chaque combinaison SharedBlocks/Individual est
// aussi exécutée par run_wspd_q2_census_batched pour W ∈ {1,2,3,4,8} et quatre réglages (lots 1, 4, 16, 64 ; quantum
// 1 ou 8) : force brute, condensé, compteurs globaux égaux au chemin série, identités de lot, vivacité.
// un chien de garde signale tout appel dépassant 600 s (perte de réveil = blocage), et un scénario de
// vivacité lève une exception depuis un slot pendant que d'autres workers peuvent dormir sur la file :
// l'appel doit rendre la main en propageant l'exception (jamais de blocage, jamais de worker détaché).
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "pipeline/wspd_q2_census.hpp"
#include "pipeline/wspd_q2_parallel.hpp"
#ifdef MHGP8_AUDIT_COOP
#include "pipeline/wspd_q2_cooperative.hpp"
#endif
#ifdef MHGP8_AUDIT_RANGES
#include "pipeline/wspd_q2_ranges.hpp"
#endif
#ifdef MHGP8_AUDIT_BATCHED
#include "pipeline/wspd_q2_batched.hpp"
#endif
#include "front_fixtures.hpp"
using namespace mhgp8;
struct Emitted { std::vector<std::size_t> interior, shell; Q2BallKey key;
  bool operator==(const Emitted& o) const { return interior == o.interior && shell == o.shell && key == o.key; } };
using Key = std::pair<std::size_t, std::size_t>;
static std::vector<Point3> adversarial(const std::string& name, unsigned seed) {
  std::vector<Point3> p; std::set<std::tuple<int,int,int>> seen;
  auto add = [&](int x, int y, int z) { if (x<0||y<0||z<0||x>65535||y>65535||z>65535) return; if (seen.insert({x,y,z}).second) p.push_back({(std::uint16_t)x,(std::uint16_t)y,(std::uint16_t)z}); };
  if (name == "grid5") { for (int x=0;x<5;++x) for (int y=0;y<5;++y) for (int z=0;z<5;++z) add(100+7*x,100+7*y,100+7*z); }
  else if (name == "cospherical") { for (int a=-13;a<=13;++a) for (int b=-13;b<=13;++b) for (int c=-13;c<=13;++c) if (a*a+b*b+c*c==169) add(1000+a,1000+b,1000+c);
    for (int a=-5;a<=5;++a) for (int b=-5;b<=5;++b) for (int c=-5;c<=5;++c) if (a*a+b*b+c*c==25) add(1000+a,1000+b,1000+c);
    add(1000,1000,1000); add(1000,1000,1001); add(1300,1000,1000); add(700,1000,1000); }
  else if (name == "collinear") { for (int i=0;i<40;++i) add(10+37*i, 500, 500); for (int i=0;i<10;++i) add(200+13*i, 501, 500); add(0,0,0); add(65535,65535,65535); }
  else if (name == "cube_corners") { for (int c=0;c<8;++c) add(2000+((c&1)?300:0), 2000+((c&2)?300:0), 2000+((c&4)?300:0)); add(2150,2150,2150); add(2150,2150,2151); add(2000,2150,2150); add(60000,60000,60000); add(60000,60000,60001); add(60001,60000,60000); }
  else if (name == "extremes") { add(0,0,0); add(65535,0,0); add(0,65535,0); add(0,0,65535); add(65535,65535,65535); add(32767,32768,32767); add(32768,32767,32768); add(1,1,1); add(65534,65534,65534); add(40000,20000,60000); add(40001,20000,60000); add(40000,20001,60000); }
  else if (name == "halfint") { std::srand(seed); for (int i=0;i<60;++i) add(1001+2*(std::rand()%30), 2001+2*(std::rand()%30), 3001+2*(std::rand()%30)); }
  else if (name == "dense_ball") { std::srand(seed); for (int i=0;i<80;++i) { int a=std::rand()%21-10, b=std::rand()%21-10, c=std::rand()%21-10; if (a*a+b*b+c*c<=100) add(5000+a,5000+b,5000+c); } add(5000,5000,5000); add(5010,5000,5000); add(4990,5000,5000); }
  return p;
}
static std::string canon(const std::map<Key, Emitted>& m) {
  // condensé canonique : FNV-1a 64 sur la séquence triée (paire, intérieurs, coquille, clé)
  u64 h = 1469598103934665603ULL; auto mix = [&](u64 v) { for (int i = 0; i < 8; ++i) { h ^= (v >> (8*i)) & 0xff; h *= 1099511628211ULL; } };
  for (const auto& [k, e] : m) { mix(k.first); mix(k.second); mix(e.interior.size()); for (auto v : e.interior) mix(v); mix(e.shell.size()); for (auto v : e.shell) mix(v); mix(e.key.diameter_squared); for (int ax=0;ax<3;++ax) mix(e.key.center_twice[ax]); }
  char buf[32]; std::snprintf(buf, sizeof buf, "%016llx", (unsigned long long)h); return buf;
}
static std::atomic<long long> g_call_started_ms{0};
static std::atomic<bool> g_in_call{false};
static long long now_ms() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
struct CallGuard { CallGuard() { g_call_started_ms = now_ms(); g_in_call = true; } ~CallGuard() { g_in_call = false; } };
int main(int argc, char** argv) {
  std::thread watchdog([] { for (;;) { std::this_thread::sleep_for(std::chrono::seconds(5)); if (g_in_call && now_ms() - g_call_started_ms > 600000) { std::fprintf(stderr, "WATCHDOG: parallel call exceeded 600 s (lost wakeup or hang)\n"); std::fflush(stderr); std::abort(); } } });
  watchdog.detach();
  if (argc < 5) { std::fprintf(stderr, "usage: family|adv:name n kmax s [seed] [scale]\n  scale : pas de force brute (grands nuages) ; une combinaison, W ∈ {1,2,4,8}, J = 16, identité des condensés et des compteurs seulement\n"); return 2; }
  const std::string fam = argv[1]; const std::size_t n = std::strtoull(argv[2], nullptr, 10);
  const unsigned kmax = std::atoi(argv[3]); const unsigned s = std::atoi(argv[4]); const unsigned seed = argc > 5 ? std::atoi(argv[5]) : 3;
  const bool scale = argc > 6 && std::string(argv[6]) == "scale";
  std::vector<Point3> pts;
  if (fam.rfind("adv:", 0) == 0) pts = adversarial(fam.substr(4), seed); else pts = bench::make_front_fixture(n, fam, seed).points;
  auto cloud = prepare_cloud(pts); auto index = make_q2_cloud_index(cloud);
  const auto P = index->cloud().points(); const std::size_t N = P.size();
  auto truth_of = [&](std::size_t i, std::size_t j, std::vector<std::size_t>& in, std::vector<std::size_t>& sh) {
    in.clear(); sh.clear();
    for (std::size_t k = 0; k < N; ++k) { i64 h = 0; for (int ax=0;ax<3;++ax) h += ((i64)P[k][ax]-P[i][ax])*((i64)P[j][ax]-P[k][ax]); if (h > 0) in.push_back(k); else if (h == 0) sh.push_back(k); }
  };
  struct Combo { WspdFrontMode f; Q2CensusMode c; Q2SiblingMode sb; Q2WitnessOrder o; Q2AnchorMode am; std::size_t pool; const char* name; };
  const std::vector<Combo> all_combos = {
    {WspdFrontMode::MidpointSamples, Q2CensusMode::SharedBlocks, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, Q2AnchorMode::Individual, 64, "samples/shared/sib+compl/pool64"},
    {WspdFrontMode::MidpointSamples, Q2CensusMode::SharedBlocks, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, Q2AnchorMode::Individual, 2, "samples/shared/sib+compl/pool2"},
    {WspdFrontMode::MidpointSamples, Q2CensusMode::Pairwise, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, Q2AnchorMode::Individual, 0, "samples/pairwise"},
    {WspdFrontMode::Pure, Q2CensusMode::SharedBlocks, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, Q2AnchorMode::Individual, 0, "pure/shared/sib+compl"},
    {WspdFrontMode::MidpointSamples, Q2CensusMode::SharedBlocks, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, Q2AnchorMode::SharedAnchors, 64, "samples/joint-anchors/sib+compl/pool64"},
  };
  const std::vector<Combo> combos = scale ? std::vector<Combo>{all_combos[0]} : all_combos;  // échelle : Pool 64, frère, Complement
  const std::vector<std::size_t> workers_list = scale ? std::vector<std::size_t>{1, 2, 4, 8} : std::vector<std::size_t>{1, 2, 3, 4, 8};
  const std::vector<std::size_t> jobs_list = scale ? std::vector<std::size_t>{16} : std::vector<std::size_t>{1, 16};
#ifdef MHGP8_AUDIT_DONATE
  struct Sched { WspdQ2Schedule s; const char* name; };
  const std::vector<Sched> schedules = {
    {WspdQ2Schedule{WspdQ2ScheduleMode::Coarse, 64, 64}, "coarse"},
    {WspdQ2Schedule{WspdQ2ScheduleMode::Donate, 64, 64}, "donate64/64"},
    {WspdQ2Schedule{WspdQ2ScheduleMode::Donate, 1, 1}, "donate1/1"},
  };
#else
  struct Sched { int s; const char* name; };
  const std::vector<Sched> schedules = {{0, "coarse"}};
#endif
  u64 total_mismatch = 0, total_checked = 0, total_alive = 0, total_runs = 0, cross_slot_dups = 0, digest_breaks = 0, counter_breaks = 0;
  u64 donations_total = 0, stolen_total = 0;
  u64 bat_runs = 0, bat_mismatch = 0, bat_digest_breaks = 0, bat_counter_breaks = 0, bat_ident_breaks = 0, bat_enqueued = 0, bat_live_runs = 0, bat_live_exc = 0;
  u64 rng_runs = 0, rng_mismatch = 0, rng_digest_breaks = 0, rng_counter_breaks = 0, rng_ident_breaks = 0, rng_donations = 0, rng_live_runs = 0, rng_live_exc = 0;
  u64 coop_runs = 0, coop_mismatch = 0, coop_digest_breaks = 0, coop_counter_breaks = 0, coop_ident_breaks = 0, coop_continued = 0, coop_donations = 0, coop_live_runs = 0, coop_live_exc = 0;
  // vérité (une fois)
  std::map<Key, Emitted> truth;
  if (!scale) { std::vector<std::size_t> in, sh; for (std::size_t i = 0; i < N; ++i) for (std::size_t j = i + 1; j < N; ++j) { ++total_checked; truth_of(i, j, in, sh); if (in.size() < kmax) { Emitted e; e.interior = in; e.shell = sh; for (int ax=0;ax<3;++ax) { e.key.center_twice[ax] = (std::uint32_t)P[i][ax] + P[j][ax]; i64 d = (i64)P[i][ax]-P[j][ax]; e.key.diameter_squared += (u64)(d*d); } truth[{i, j}] = e; } } }
  total_alive = truth.size();
  for (const auto& cb : combos) {
    // référence série
    std::map<Key, Emitted> serial; u64 sdup = 0;
    auto sconsumer = [&](const Q2Support& sp) { Key k{std::min(sp.a_id, sp.b_id), std::max(sp.a_id, sp.b_id)}; if (serial.count(k)) ++sdup; Emitted e; e.interior.assign(sp.interior.begin(), sp.interior.end()); e.shell.assign(sp.shell.begin(), sp.shell.end()); e.key = sp.key; std::sort(e.interior.begin(), e.interior.end()); std::sort(e.shell.begin(), e.shell.end()); serial[k] = e; };
    auto sres = run_wspd_q2_census(*index, kmax, s, cb.f, cb.c, sconsumer, cb.sb, cb.o, cb.am, cb.pool);
    const std::string sdigest = canon(serial);
    if (scale) truth = serial;  // en mode échelle, la référence est le chemin série (pas de force brute)
    const bool serial_ok = (serial == truth) && sdup == 0;
    if (!serial_ok) ++total_mismatch;
    std::printf("%-40s serial: emitted=%zu alive=%zu dup=%llu ok=%s digest=%s cand=%llu acc=%llu rej=%llu rects=%llu serial_total_ms=%.1f\n", cb.name, serial.size(), truth.size(), (unsigned long long)sdup, serial_ok ? "yes" : "NO", sdigest.c_str(), (unsigned long long)sres.census.candidate_pairs, (unsigned long long)sres.census.accepted_pairs, (unsigned long long)sres.census.rejected_pairs, (unsigned long long)sres.input_rectangles, sres.total_ms);
    for (auto W : workers_list) for (auto J : jobs_list) for (const auto& sched : schedules) {
      ++total_runs;
      std::vector<std::map<Key, Emitted>> slots(W);
      std::vector<Q2CensusConsumer> consumers;
      for (std::size_t w = 0; w < W; ++w) consumers.push_back([&slots, w](const Q2Support& sp) { Key k{std::min(sp.a_id, sp.b_id), std::max(sp.a_id, sp.b_id)}; Emitted e; e.interior.assign(sp.interior.begin(), sp.interior.end()); e.shell.assign(sp.shell.begin(), sp.shell.end()); e.key = sp.key; std::sort(e.interior.begin(), e.interior.end()); std::sort(e.shell.begin(), e.shell.end()); auto& m = slots[w]; if (m.count(k)) { /* doublon intra-slot */ e.interior.push_back(~std::size_t{0}); } m[k] = e; });
#ifdef MHGP8_AUDIT_DONATE
      CallGuard guard;
      auto pres = run_wspd_q2_census_parallel(index, kmax, s, cb.f, cb.c, consumers, J, cb.sb, cb.o, cb.am, cb.pool, sched.s);
#else
      static_cast<void>(sched);
      auto pres = run_wspd_q2_census_parallel(index, kmax, s, cb.f, cb.c, consumers, J, cb.sb, cb.o, cb.am, cb.pool);
#endif
      std::map<Key, Emitted> merged; u64 dups = 0;
      for (auto& m : slots) for (auto& [k, e] : m) { if (merged.count(k)) ++dups; merged[k] = e; }
      const std::string pdigest = canon(merged);
      const bool ok = (merged == truth) && dups == 0;
      const bool digest_ok = pdigest == sdigest;
      // Conservation par somme de tous les compteurs de travail discrets (doctrine « décisions géométriques
      // indépendantes de l'ordonnancement ») : census, frère, Pool et front, comparés au chemin série.
      std::string first_bad;
      auto same = [&](const char* name, u64 a, u64 b) { if (a != b && first_bad.empty()) first_bad = std::string(name) + "(" + std::to_string(a) + "!=" + std::to_string(b) + ")"; };
      same("candidate_pairs", pres.candidate_pairs, sres.census.candidate_pairs); same("accepted_pairs", pres.accepted_pairs, sres.census.accepted_pairs);
      same("rejected_pairs", pres.rejected_pairs, sres.census.rejected_pairs); same("input_rectangles", pres.input_rectangles, sres.input_rectangles);
      same("anchor_queries", pres.anchor_queries, sres.anchor_queries);
      const auto& pw = pres.census_work; const auto& sw = sres.census.work;
      same("count_node_visits", pw.count_node_visits, sw.count_node_visits); same("count_bound_tests", pw.count_bound_tests, sw.count_bound_tests);
      same("count_point_tests", pw.count_point_tests, sw.count_point_tests); same("query_tasks", pw.query_tasks, sw.query_tasks);
      same("count_root_starts", pw.count_root_starts, sw.count_root_starts); same("uniform_rejected_pairs", pw.uniform_rejected_pairs, sw.uniform_rejected_pairs);
      same("payload_supports", pw.payload_supports, sw.payload_supports); same("payload_shell_sites", pw.payload_shell_sites, sw.payload_shell_sites);
      same("sibling_rejected", pres.sibling_work.rejected_pairs, sres.sibling_work.rejected_pairs);
      same("order_phase_switches", pres.order_work.phase_switches, sres.order_work.phase_switches);
      same("pool_selected_rectangles", pres.pool_work.selected_rectangles, sres.pool_work.selected_rectangles); same("pool_filtered_pairs", pres.pool_work.filtered_pairs, sres.pool_work.filtered_pairs);
      same("pool_pair_roots", pres.pool_work.pair_roots, sres.pool_work.pair_roots); same("pool_passthrough_rectangles", pres.pool_work.passthrough_rectangles, sres.pool_work.passthrough_rectangles);
      same("front_product_visits", pres.front.work.product_visits, sres.front.work.product_visits); same("front_witness_searches", pres.front.work.witness_searches, sres.front.work.witness_searches);
      same("front_emitted_rectangles", pres.front.work.emitted_rectangles, sres.front.work.emitted_rectangles); same("front_residual_q2", pres.front.work.residual_pair_mass[0], sres.front.work.residual_pair_mass[0]);
      const bool counters_ok = first_bad.empty();
      if (!ok) ++total_mismatch;
      if (dups) cross_slot_dups += dups;
      if (!digest_ok) ++digest_breaks;
      if (!counters_ok) ++counter_breaks;
      // Déséquilibre : temps du worker le plus chargé rapporté à la moyenne, et sa part des visites de comptage.
      double max_ms = 0, sum_ms = 0; u64 max_visits = 0, sum_visits = 0, max_jobs = 0;
      for (const auto& wk : pres.workers) { max_ms = std::max(max_ms, wk.elapsed_ms); sum_ms += wk.elapsed_ms; max_visits = std::max(max_visits, wk.count_node_visits); sum_visits += wk.count_node_visits; max_jobs = std::max(max_jobs, wk.jobs); }
      const double mean_ms = pres.workers.empty() ? 0 : sum_ms / pres.workers.size();
#ifdef MHGP8_AUDIT_DONATE
      const auto& dw = pres.dispatch_work;
      std::printf("  sched=%s donations=%llu stolen_completed=%llu offer_full=%llu offer_busy=%llu max_queue=%llu\n", sched.name, (unsigned long long)dw.donations, (unsigned long long)dw.stolen_completed, (unsigned long long)dw.offer_full, (unsigned long long)dw.offer_busy, (unsigned long long)dw.max_queue_size);
      donations_total += dw.donations; stolen_total += dw.stolen_completed;
#endif
      std::printf("  W=%zu J=%-2zu started=%llu jobs=%llu completed=%llu terminal=%llu emitted=%zu dups=%llu ok=%s digest_eq=%s counters_eq=%s cand=%llu acc=%llu rej=%llu total_ms=%.1f worker_max_ms=%.1f worker_mean_ms=%.1f imbalance=%.2f max_visit_share=%.3f max_jobs=%llu\n", W, J, (unsigned long long)pres.started_workers, (unsigned long long)pres.jobs, (unsigned long long)pres.completed_jobs, (unsigned long long)pres.terminal_jobs, merged.size(), (unsigned long long)dups, ok ? "yes" : "NO", digest_ok ? "yes" : "NO", counters_ok ? "yes" : first_bad.c_str(), (unsigned long long)pres.candidate_pairs, (unsigned long long)pres.accepted_pairs, (unsigned long long)pres.rejected_pairs, pres.total_ms, max_ms, mean_ms, mean_ms > 0 ? max_ms / mean_ms : 0.0, sum_visits ? (double)max_visits / sum_visits : 0.0, (unsigned long long)max_jobs);
    }
#ifdef MHGP8_AUDIT_BATCHED
    // ---- petits census entrelacés (SharedBlocks / Individual seulement)
    if (cb.c == Q2CensusMode::SharedBlocks && cb.am == Q2AnchorMode::Individual) {
      struct BatOpt { std::size_t lanes, quantum; const char* name; };
      const std::vector<BatOpt> bat_opts = scale ? std::vector<BatOpt>{{16, 1, "l16/q1"}, {1, 1, "l1/q1"}} : std::vector<BatOpt>{{16, 1, "l16/q1"}, {1, 1, "l1/q1"}, {64, 8, "l64/q8"}, {4, 1, "l4/q1"}};
      for (auto W : workers_list) for (const auto& bo : bat_opts) {
        ++bat_runs;
        std::vector<std::map<Key, Emitted>> slots(W); std::vector<Q2CensusConsumer> consumers;
        for (std::size_t w = 0; w < W; ++w) consumers.push_back([&slots, w](const Q2Support& sp) { Key k{std::min(sp.a_id, sp.b_id), std::max(sp.a_id, sp.b_id)}; Emitted e; e.interior.assign(sp.interior.begin(), sp.interior.end()); e.shell.assign(sp.shell.begin(), sp.shell.end()); e.key = sp.key; std::sort(e.interior.begin(), e.interior.end()); std::sort(e.shell.begin(), e.shell.end()); auto& m = slots[w]; if (m.count(k)) e.interior.push_back(~std::size_t{0}); m[k] = e; });
        WspdQ2BatchOptions o; o.jobs_per_worker = 16; o.lanes = bo.lanes; o.quantum = bo.quantum;
        WspdQ2BatchResult bres; { CallGuard guard; bres = run_wspd_q2_census_batched(index, kmax, s, cb.f, consumers, o, cb.sb, cb.o, cb.pool); }
        const auto& pres = bres.pipeline;
        std::map<Key, Emitted> merged; u64 dups = 0; for (auto& m : slots) for (auto& [k, e] : m) { if (merged.count(k)) ++dups; merged[k] = e; }
        const std::string pdigest = canon(merged);
        const bool ok = (merged == truth) && dups == 0; const bool digest_ok = pdigest == sdigest;
        std::string first_bad;
        auto same = [&](const char* name, u64 a, u64 b) { if (a != b && first_bad.empty()) first_bad = std::string(name) + "(" + std::to_string(a) + "!=" + std::to_string(b) + ")"; };
        same("candidate_pairs", pres.candidate_pairs, sres.census.candidate_pairs); same("accepted_pairs", pres.accepted_pairs, sres.census.accepted_pairs);
        same("rejected_pairs", pres.rejected_pairs, sres.census.rejected_pairs); same("input_rectangles", pres.input_rectangles, sres.input_rectangles);
        const auto& pw = pres.census_work; const auto& sw = sres.census.work;
        same("count_node_visits", pw.count_node_visits, sw.count_node_visits); same("count_bound_tests", pw.count_bound_tests, sw.count_bound_tests);
        same("count_point_tests", pw.count_point_tests, sw.count_point_tests); same("uniform_rejected_pairs", pw.uniform_rejected_pairs, sw.uniform_rejected_pairs);
        same("query_tasks", pw.query_tasks, sw.query_tasks);
        same("payload_supports", pw.payload_supports, sw.payload_supports); same("payload_shell_sites", pw.payload_shell_sites, sw.payload_shell_sites);
        same("sibling_rejected", pres.sibling_work.rejected_pairs, sres.sibling_work.rejected_pairs); same("order_phase_switches", pres.order_work.phase_switches, sres.order_work.phase_switches);
        same("pool_filtered_pairs", pres.pool_work.filtered_pairs, sres.pool_work.filtered_pairs); same("pool_pair_roots", pres.pool_work.pair_roots, sres.pool_work.pair_roots);
        same("front_emitted_rectangles", pres.front.work.emitted_rectangles, sres.front.work.emitted_rectangles); same("front_residual_q2", pres.front.work.residual_pair_mass[0], sres.front.work.residual_pair_mass[0]);
        const bool counters_ok = first_bad.empty();
        const auto& bw = bres.work;
        const bool ident_ok = bw.enqueued == bw.completed && bw.completed == bw.completed_accepted + bw.completed_rejected && bw.completed == bw.entry_steps
          && bw.payload_steps == bw.completed_accepted && bw.admission_steps == bw.completed_accepted
          && bw.transitions == bw.entry_steps + bw.witness_steps + bw.admission_steps + bw.payload_steps && bw.key_preparations == bw.enqueued;
        if (!ok) ++bat_mismatch;
        if (!digest_ok) ++bat_digest_breaks;
        if (!counters_ok) ++bat_counter_breaks;
        if (!ident_ok) ++bat_ident_breaks;
        bat_enqueued += bw.enqueued;
        std::printf("  batched W=%zu %s: emitted=%zu dups=%llu ok=%s digest_eq=%s counters_eq=%s ident=%s enqueued=%llu passes=%llu entry_after_credit=%llu entry_inside_deferred=%llu sibling_due=%llu total_ms=%.1f\n", W, bo.name, merged.size(), (unsigned long long)dups, ok ? "yes" : "NO", digest_ok ? "yes" : "NO", counters_ok ? "yes" : first_bad.c_str(), ident_ok ? "yes" : "NO", (unsigned long long)bw.enqueued, (unsigned long long)bw.batch_passes, (unsigned long long)bw.entry_after_credit, (unsigned long long)bw.entry_inside_deferred, (unsigned long long)bw.sibling_due, pres.total_ms);
      }
      if (!serial.empty() && !scale) { ++bat_live_runs; const std::size_t W = 8; std::vector<Q2CensusConsumer> consumers;
        for (std::size_t w = 0; w < W; ++w) consumers.push_back([](const Q2Support&) { throw std::runtime_error("audit: slot failure"); });
        WspdQ2BatchOptions o; o.lanes = 1; o.quantum = 1; CallGuard guard;
        try { auto r = run_wspd_q2_census_batched(index, kmax, s, cb.f, consumers, o, cb.sb, cb.o, cb.pool); static_cast<void>(r); std::printf("  BATCHED LIVENESS: no exception\n"); } catch (const std::runtime_error&) { ++bat_live_exc; } }
    }
#endif
#ifdef MHGP8_AUDIT_RANGES
    // ---- plages d'ancres et Pool partagé (SharedBlocks / Individual seulement)
    if (cb.c == Q2CensusMode::SharedBlocks && cb.am == Q2AnchorMode::Individual) {
      struct RngOpt { std::size_t grain, queue; const char* name; };
      const std::vector<RngOpt> rng_opts = scale ? std::vector<RngOpt>{{64, 8, "g64/f8"}, {1, 8, "g1/f8"}} : std::vector<RngOpt>{{64, 8, "g64/f8"}, {1, 8, "g1/f8"}, {1, 1, "g1/f1"}, {4, 1, "g4/f1"}};
      for (auto W : workers_list) for (const auto& ro : rng_opts) {
        ++rng_runs;
        std::vector<std::map<Key, Emitted>> slots(W); std::vector<Q2CensusConsumer> consumers;
        for (std::size_t w = 0; w < W; ++w) consumers.push_back([&slots, w](const Q2Support& sp) { Key k{std::min(sp.a_id, sp.b_id), std::max(sp.a_id, sp.b_id)}; Emitted e; e.interior.assign(sp.interior.begin(), sp.interior.end()); e.shell.assign(sp.shell.begin(), sp.shell.end()); e.key = sp.key; std::sort(e.interior.begin(), e.interior.end()); std::sort(e.shell.begin(), e.shell.end()); auto& m = slots[w]; if (m.count(k)) e.interior.push_back(~std::size_t{0}); m[k] = e; });
        WspdQ2RangeOptions o; o.jobs_per_worker = 16; o.queue_capacity = ro.queue; o.anchor_grain = ro.grain;
        WspdQ2RangeResult rres; { CallGuard guard; rres = run_wspd_q2_census_ranges(index, kmax, s, cb.f, consumers, o, cb.sb, cb.o, cb.pool); }
        const auto& pres = rres.pipeline;
        std::map<Key, Emitted> merged; u64 dups = 0; for (auto& m : slots) for (auto& [k, e] : m) { if (merged.count(k)) ++dups; merged[k] = e; }
        const std::string pdigest = canon(merged);
        const bool ok = (merged == truth) && dups == 0; const bool digest_ok = pdigest == sdigest;
        std::string first_bad;
        auto same = [&](const char* name, u64 a, u64 b) { if (a != b && first_bad.empty()) first_bad = std::string(name) + "(" + std::to_string(a) + "!=" + std::to_string(b) + ")"; };
        same("candidate_pairs", pres.candidate_pairs, sres.census.candidate_pairs); same("accepted_pairs", pres.accepted_pairs, sres.census.accepted_pairs);
        same("rejected_pairs", pres.rejected_pairs, sres.census.rejected_pairs); same("input_rectangles", pres.input_rectangles, sres.input_rectangles);
        const auto& pw = pres.census_work; const auto& sw = sres.census.work;
        same("count_node_visits", pw.count_node_visits, sw.count_node_visits); same("count_bound_tests", pw.count_bound_tests, sw.count_bound_tests);
        same("count_point_tests", pw.count_point_tests, sw.count_point_tests); same("uniform_rejected_pairs", pw.uniform_rejected_pairs, sw.uniform_rejected_pairs);
        same("payload_supports", pw.payload_supports, sw.payload_supports); same("payload_shell_sites", pw.payload_shell_sites, sw.payload_shell_sites);
        same("sibling_rejected", pres.sibling_work.rejected_pairs, sres.sibling_work.rejected_pairs); same("order_phase_switches", pres.order_work.phase_switches, sres.order_work.phase_switches);
        same("pool_filtered_pairs", pres.pool_work.filtered_pairs, sres.pool_work.filtered_pairs); same("pool_pair_roots", pres.pool_work.pair_roots, sres.pool_work.pair_roots);
        same("pool_selected_rectangles", pres.pool_work.selected_rectangles, sres.pool_work.selected_rectangles);
        same("front_emitted_rectangles", pres.front.work.emitted_rectangles, sres.front.work.emitted_rectangles); same("front_residual_q2", pres.front.work.residual_pair_mass[0], sres.front.work.residual_pair_mass[0]);
        const bool counters_ok = first_bad.empty();
        const auto& rw = rres.work;
        const bool ident_ok = rw.initial_ranges == rw.initial_shared_ranges + rw.initial_pool_ranges + rw.initial_passthrough_ranges
          && rw.completed_ranges == rw.initial_ranges + rw.donations && rw.received_ranges == rw.donations
          && rw.donations == rw.shared_donations + rw.pool_donations + rw.passthrough_donations
          && rw.initial_anchors == rw.completed_anchors && rw.initial_pairs == rw.completed_pairs && rw.initial_pairs == pres.candidate_pairs
          && rw.offer_checks == rw.offer_busy + rw.offer_full + rw.offer_no_waiter + rw.donations && rw.waits == rw.wakes;
        if (!ok) ++rng_mismatch;
        if (!digest_ok) ++rng_digest_breaks;
        if (!counters_ok) ++rng_counter_breaks;
        if (!ident_ok) ++rng_ident_breaks;
        rng_donations += rw.donations;
        std::printf("  ranges W=%zu %s: emitted=%zu dups=%llu ok=%s digest_eq=%s counters_eq=%s ident=%s ranges=%llu donations=%llu pool_parents_max=%llu total_ms=%.1f\n", W, ro.name, merged.size(), (unsigned long long)dups, ok ? "yes" : "NO", digest_ok ? "yes" : "NO", counters_ok ? "yes" : first_bad.c_str(), ident_ok ? "yes" : "NO", (unsigned long long)rw.initial_ranges, (unsigned long long)rw.donations, (unsigned long long)rres.max_live_pool_parents, pres.total_ms);
      }
      if (!serial.empty() && !scale) { ++rng_live_runs; const std::size_t W = 8; std::vector<Q2CensusConsumer> consumers;
        for (std::size_t w = 0; w < W; ++w) consumers.push_back([](const Q2Support&) { throw std::runtime_error("audit: slot failure"); });
        WspdQ2RangeOptions o; o.queue_capacity = 1; o.anchor_grain = 1; CallGuard guard;
        try { auto r = run_wspd_q2_census_ranges(index, kmax, s, cb.f, consumers, o, cb.sb, cb.o, cb.pool); static_cast<void>(r); std::printf("  RANGES LIVENESS: no exception\n"); } catch (const std::runtime_error&) { ++rng_live_exc; } }
    }
#endif
#ifdef MHGP8_AUDIT_COOP
    // ---- chaîne coopérative (SharedBlocks / Individual seulement, comme l'entrée l'impose)
    if (cb.c == Q2CensusMode::SharedBlocks && cb.am == Q2AnchorMode::Individual) {
      struct CoopOpt { std::size_t minb, quantum, queue; const char* name; };
      const std::vector<CoopOpt> coop_opts = scale ? std::vector<CoopOpt>{{64, 256, 8, "b64/q256/f8"}, {1, 256, 8, "b1/q256/f8"}, {1, 1, 8, "b1/q1/f8"}} : std::vector<CoopOpt>{{64, 256, 8, "b64/q256/f8"}, {1, 256, 8, "b1/q256/f8"}, {1, 1, 1, "b1/q1/f1"}, {1, 1, 8, "b1/q1/f8"}};
      for (auto W : workers_list) for (const auto& co : coop_opts) {
        ++coop_runs;
        std::vector<std::map<Key, Emitted>> slots(W); std::vector<Q2CensusConsumer> consumers;
        for (std::size_t w = 0; w < W; ++w) consumers.push_back([&slots, w](const Q2Support& sp) { Key k{std::min(sp.a_id, sp.b_id), std::max(sp.a_id, sp.b_id)}; Emitted e; e.interior.assign(sp.interior.begin(), sp.interior.end()); e.shell.assign(sp.shell.begin(), sp.shell.end()); e.key = sp.key; std::sort(e.interior.begin(), e.interior.end()); std::sort(e.shell.begin(), e.shell.end()); auto& m = slots[w]; if (m.count(k)) e.interior.push_back(~std::size_t{0}); m[k] = e; });
        WspdQ2CooperativeOptions o; o.jobs_per_worker = 16; o.queue_capacity = co.queue; o.quantum = co.quantum; o.min_b_size = co.minb;
        WspdQ2CooperativeResult cres; { CallGuard guard; cres = run_wspd_q2_census_cooperative(index, kmax, s, cb.f, consumers, o, cb.sb, cb.o, cb.pool); }
        const auto& pres = cres.pipeline;
        std::map<Key, Emitted> merged; u64 dups = 0; for (auto& m : slots) for (auto& [k, e] : m) { if (merged.count(k)) ++dups; merged[k] = e; }
        const std::string pdigest = canon(merged);
        const bool ok = (merged == truth) && dups == 0; const bool digest_ok = pdigest == sdigest;
        std::string first_bad;
        auto same = [&](const char* name, u64 a, u64 b) { if (a != b && first_bad.empty()) first_bad = std::string(name) + "(" + std::to_string(a) + "!=" + std::to_string(b) + ")"; };
        same("candidate_pairs", pres.candidate_pairs, sres.census.candidate_pairs); same("accepted_pairs", pres.accepted_pairs, sres.census.accepted_pairs);
        same("rejected_pairs", pres.rejected_pairs, sres.census.rejected_pairs); same("input_rectangles", pres.input_rectangles, sres.input_rectangles);
        const auto& pw = pres.census_work; const auto& sw = sres.census.work;
        same("count_node_visits", pw.count_node_visits, sw.count_node_visits); same("count_bound_tests", pw.count_bound_tests, sw.count_bound_tests);
        same("count_point_tests", pw.count_point_tests, sw.count_point_tests); same("uniform_rejected_pairs", pw.uniform_rejected_pairs, sw.uniform_rejected_pairs);
        same("payload_supports", pw.payload_supports, sw.payload_supports); same("payload_shell_sites", pw.payload_shell_sites, sw.payload_shell_sites);
        same("sibling_rejected", pres.sibling_work.rejected_pairs, sres.sibling_work.rejected_pairs); same("order_phase_switches", pres.order_work.phase_switches, sres.order_work.phase_switches);
        same("pool_filtered_pairs", pres.pool_work.filtered_pairs, sres.pool_work.filtered_pairs); same("pool_pair_roots", pres.pool_work.pair_roots, sres.pool_work.pair_roots);
        same("front_emitted_rectangles", pres.front.work.emitted_rectangles, sres.front.work.emitted_rectangles); same("front_residual_q2", pres.front.work.residual_pair_mass[0], sres.front.work.residual_pair_mass[0]);
        const bool counters_ok = first_bad.empty();
        const auto& cw = cres.work;
        const bool ident_ok = cw.continued_pairs == cw.completed_pairs && cw.fragments_started == cw.completed_fragments && cw.fragments_started == cw.continued_anchors + cw.donations
          && cw.donations == cw.detach_work.detached_frames && cw.detach_work.detached_frames == cw.detach_work.imported_frames
          && cw.offer_checks == cw.offer_busy + cw.offer_full + cw.offer_no_waiter + cw.detach_work.attempts && cw.detach_work.attempts == cw.donations + cw.offer_no_sibling;
        if (!ok) ++coop_mismatch;
        if (!digest_ok) ++coop_digest_breaks;
        if (!counters_ok) ++coop_counter_breaks;
        if (!ident_ok) ++coop_ident_breaks;
        coop_continued += cw.continued_anchors; coop_donations += cw.donations;
        std::printf("  coop W=%zu %s: emitted=%zu dups=%llu ok=%s digest_eq=%s counters_eq=%s ident=%s continued_anchors=%llu donations=%llu total_ms=%.1f\n", W, co.name, merged.size(), (unsigned long long)dups, ok ? "yes" : "NO", digest_ok ? "yes" : "NO", counters_ok ? "yes" : first_bad.c_str(), ident_ok ? "yes" : "NO", (unsigned long long)cw.continued_anchors, (unsigned long long)cw.donations, pres.total_ms);
      }
      // vivacité coopérative : tout slot lève à son premier support (W=8, b1/q1/f1)
      if (!serial.empty() && !scale) { ++coop_live_runs; const std::size_t W = 8; std::vector<Q2CensusConsumer> consumers;
        for (std::size_t w = 0; w < W; ++w) consumers.push_back([](const Q2Support&) { throw std::runtime_error("audit: slot failure"); });
        WspdQ2CooperativeOptions o; o.queue_capacity = 1; o.quantum = 1; o.min_b_size = 1; CallGuard guard;
        try { auto r = run_wspd_q2_census_cooperative(index, kmax, s, cb.f, consumers, o, cb.sb, cb.o, cb.pool); static_cast<void>(r); std::printf("  COOP LIVENESS: no exception\n"); } catch (const std::runtime_error&) { ++coop_live_exc; } }
    }
#endif
  }
#ifdef MHGP8_AUDIT_DONATE
  // Vivacité : exception levée par le dernier slot après cinq supports, pendant que les autres workers
  // peuvent dormir sur la file (Donate{1,1} maximise les attentes). L'appel doit propager l'exception.
  u64 liveness_runs = 0, liveness_exceptions = 0, liveness_no_throw = 0;
  if (!scale) {
    const auto& cb = all_combos[0];
    for (const auto& sched : schedules) for (auto W : std::vector<std::size_t>{2, 8}) for (int rep = 0; rep < 10; ++rep) {
      std::vector<std::atomic<unsigned>> seen(W);
      std::vector<Q2CensusConsumer> consumers;
      for (std::size_t w = 0; w < W; ++w) consumers.push_back([&seen, w, W](const Q2Support&) { if (w == W - 1 && ++seen[w] == 5) throw std::runtime_error("audit: slot failure"); else if (w != W - 1) ++seen[w]; });
      ++liveness_runs; CallGuard guard;
      try { auto r = run_wspd_q2_census_parallel(index, kmax, s, cb.f, cb.c, consumers, 1, cb.sb, cb.o, cb.am, cb.pool, sched.s); static_cast<void>(r); ++liveness_no_throw; }
      catch (const std::runtime_error&) { ++liveness_exceptions; }
    }
  }
  std::printf("  liveness: runs=%llu exceptions=%llu no_throw=%llu (no hang: watchdog silent)\n", (unsigned long long)liveness_runs, (unsigned long long)liveness_exceptions, (unsigned long long)liveness_no_throw);
#endif
  std::printf("SUMMARY family=%s n=%zu kmax=%u s=%u combos=%zu parallel_runs=%llu checked=%llu alive=%llu mismatch=%llu cross_slot_dups=%llu digest_breaks=%llu counter_breaks=%llu donations=%llu stolen=%llu coop_runs=%llu coop_mismatch=%llu coop_digest_breaks=%llu coop_counter_breaks=%llu coop_ident_breaks=%llu coop_continued=%llu coop_donations=%llu coop_liveness_runs=%llu coop_liveness_exceptions=%llu rng_runs=%llu rng_mismatch=%llu rng_digest_breaks=%llu rng_counter_breaks=%llu rng_ident_breaks=%llu rng_donations=%llu rng_liveness_runs=%llu rng_liveness_exceptions=%llu bat_runs=%llu bat_mismatch=%llu bat_digest_breaks=%llu bat_counter_breaks=%llu bat_ident_breaks=%llu bat_enqueued=%llu bat_liveness_runs=%llu bat_liveness_exceptions=%llu\n", fam.c_str(), N, kmax, s, combos.size(), (unsigned long long)total_runs, (unsigned long long)total_checked, (unsigned long long)total_alive, (unsigned long long)total_mismatch, (unsigned long long)cross_slot_dups, (unsigned long long)digest_breaks, (unsigned long long)counter_breaks, (unsigned long long)donations_total, (unsigned long long)stolen_total,
    (unsigned long long)coop_runs, (unsigned long long)coop_mismatch, (unsigned long long)coop_digest_breaks, (unsigned long long)coop_counter_breaks, (unsigned long long)coop_ident_breaks, (unsigned long long)coop_continued, (unsigned long long)coop_donations, (unsigned long long)coop_live_runs, (unsigned long long)coop_live_exc,
    (unsigned long long)rng_runs, (unsigned long long)rng_mismatch, (unsigned long long)rng_digest_breaks, (unsigned long long)rng_counter_breaks, (unsigned long long)rng_ident_breaks, (unsigned long long)rng_donations, (unsigned long long)rng_live_runs, (unsigned long long)rng_live_exc,
    (unsigned long long)bat_runs, (unsigned long long)bat_mismatch, (unsigned long long)bat_digest_breaks, (unsigned long long)bat_counter_breaks, (unsigned long long)bat_ident_breaks, (unsigned long long)bat_enqueued, (unsigned long long)bat_live_runs, (unsigned long long)bat_live_exc);
  return (total_mismatch == 0 && cross_slot_dups == 0 && digest_breaks == 0 && counter_breaks == 0 && coop_mismatch == 0 && coop_digest_breaks == 0 && coop_counter_breaks == 0 && coop_ident_breaks == 0 && coop_live_runs == coop_live_exc && rng_mismatch == 0 && rng_digest_breaks == 0 && rng_counter_breaks == 0 && rng_ident_breaks == 0 && rng_live_runs == rng_live_exc && bat_mismatch == 0 && bat_digest_breaks == 0 && bat_counter_breaks == 0 && bat_ident_breaks == 0 && bat_live_runs == bat_live_exc) ? 0 : 1;
}
