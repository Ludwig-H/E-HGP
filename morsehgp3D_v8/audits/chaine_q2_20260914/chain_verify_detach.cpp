// Auditeur B (15 sept. 2026) — vérificateur du détachement intérieur du census q2 (tranche 16).
// (A) Détachement récursif mono-fil : une continuation avance par budget ; à chaque pause on tente
//     detach_pending() ; chaque enfant est traité de la même façon (récursivement) ; la réunion des
//     émissions de toute la lignée doit égaler la référence série et la force brute ; les compteurs
//     géométriques et masses sommés sur la lignée doivent égaler la référence ; identités de détachement
//     (Σ detached_frames = Σ imported_frames ; candidates de l'enfant = population de son B ; donneur
//     diminué d'autant) ; aucun doublon.
// (B) run_q2_anchor_parallel : workers ∈ {1,2,4,8} × quantum ∈ {1,256} × file ∈ {1,8} : réunion des slots
//     = référence, sum = référence sur les compteurs discrets, aucun doublon entre slots ; vivacité :
//     exception depuis un slot → propagée, jamais de blocage (chien de garde 600 s).
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include "pipeline/q2_census_parallel.hpp"
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
static std::atomic<long long> g_started{0}; static std::atomic<bool> g_in{false};
static long long now_ms() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
struct Guard { Guard() { g_started = now_ms(); g_in = true; } ~Guard() { g_in = false; } };
struct Sums { u64 cand = 0, acc = 0, rej = 0, visits = 0, bound = 0, point = 0, tasks = 0, supports = 0, shell = 0, sib = 0, phase = 0; };
static void add_sums(Sums& s, const Q2CensusResult& c, const Q2SiblingWork& sw, const Q2OrderWork& ow) { s.cand += c.candidate_pairs; s.acc += c.accepted_pairs; s.rej += c.rejected_pairs; s.visits += c.work.count_node_visits; s.bound += c.work.count_bound_tests; s.point += c.work.count_point_tests; s.tasks += c.work.query_tasks; s.supports += c.work.payload_supports; s.shell += c.work.payload_shell_sites; s.sib += sw.rejected_pairs; s.phase += ow.phase_switches; }
static bool same_sums(const Sums& a, const Sums& b, std::string& bad) { auto ck = [&](const char* n, u64 x, u64 y) { if (x != y && bad.empty()) bad = std::string(n) + "(" + std::to_string(x) + "!=" + std::to_string(y) + ")"; };
  ck("cand", a.cand, b.cand); ck("acc", a.acc, b.acc); ck("rej", a.rej, b.rej); ck("visits", a.visits, b.visits); ck("bound", a.bound, b.bound); ck("point", a.point, b.point); ck("tasks", a.tasks, b.tasks); ck("supports", a.supports, b.supports); ck("shell", a.shell, b.shell); ck("sib", a.sib, b.sib); ck("phase", a.phase, b.phase); return bad.empty(); }
int main(int argc, char** argv) {
  if (argc < 5) { std::fprintf(stderr, "usage: family|adv:name n kmax s [seed] [max_pairs]\n"); return 2; }
  std::thread([] { for (;;) { std::this_thread::sleep_for(std::chrono::seconds(5)); if (g_in && now_ms() - g_started > 600000) { std::fprintf(stderr, "WATCHDOG: call exceeded 600 s\n"); std::abort(); } } }).detach();
  const std::string fam = argv[1]; const std::size_t n = std::strtoull(argv[2], nullptr, 10);
  const unsigned kmax = std::atoi(argv[3]); const unsigned seed = argc > 5 ? std::atoi(argv[5]) : 3; const std::size_t max_pairs = argc > 6 ? std::strtoull(argv[6], nullptr, 10) : 200;
  std::vector<Point3> pts; if (fam.rfind("adv:", 0) == 0) pts = adversarial(fam.substr(4), seed); else pts = bench::make_front_fixture(n, fam, seed).points;
  auto cloud = prepare_cloud(pts); auto index = make_q2_cloud_index(cloud);
  const auto P = index->cloud().points(); const std::size_t N = P.size(); const auto nodes = index->spatial_nodes(); const auto order = index->spatial_order();
  // couples (ancre, B) : nœuds B de taille >= 2 (un détachement exige au moins deux cadres) et <= 128
  std::vector<std::pair<std::size_t, std::size_t>> pairs; u64 lcg = 0x9E3779B97F4A7C15ULL + seed;
  for (std::size_t id = 0; id < nodes.size() && pairs.size() < max_pairs; ++id) {
    const auto& b = nodes[id]; if (b.range.size() < 2 || b.range.size() > 128) continue;
    std::vector<std::size_t> anchors; if (b.range.first > 0) anchors.push_back(b.range.first - 1); if (b.range.last < N) anchors.push_back(b.range.last);
    lcg = lcg * 6364136223846793005ULL + 1442695040888963407ULL; const std::size_t r = (std::size_t)((lcg >> 33) % N); if (r < b.range.first || r >= b.range.last) anchors.push_back(r);
    for (auto a : anchors) if (pairs.size() < max_pairs) pairs.push_back({a, id});
  }
  auto truth_of = [&](std::size_t i, std::size_t j, std::vector<std::size_t>& in, std::vector<std::size_t>& sh) { in.clear(); sh.clear(); for (std::size_t k = 0; k < N; ++k) { i64 h = 0; for (int ax=0;ax<3;++ax) h += ((i64)P[k][ax]-P[i][ax])*((i64)P[j][ax]-P[k][ax]); if (h > 0) in.push_back(k); else if (h == 0) sh.push_back(k); } };
  auto collect = [](std::map<Key, Emitted>& m, u64& dups, const Q2Support& sp) { Key k{std::min(sp.a_id, sp.b_id), std::max(sp.a_id, sp.b_id)}; if (m.count(k)) ++dups; Emitted e; e.interior.assign(sp.interior.begin(), sp.interior.end()); e.shell.assign(sp.shell.begin(), sp.shell.end()); e.key = sp.key; std::sort(e.interior.begin(), e.interior.end()); std::sort(e.shell.begin(), e.shell.end()); m[k] = e; };
  struct Opt { Q2SiblingMode sb; Q2WitnessOrder o; const char* name; };
  const Opt opts[] = {{Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, "global"}, {Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, "sib+compl"}, {Q2SiblingMode::Disabled, Q2WitnessOrder::ComplementFirst, "compl"}};
  u64 lineages = 0, detached_total = 0, mismatch = 0, sum_breaks = 0, ident_breaks = 0, par_runs = 0, par_mismatch = 0, par_sum_breaks = 0, par_dups = 0, live_runs = 0, live_exc = 0, alive_total = 0, ref_mismatch = 0;
  std::vector<std::size_t> in, sh;
  for (const auto& [rank, bnode] : pairs) {
    const auto a_id = order[rank]; const auto& b = nodes[bnode];
    std::map<Key, Emitted> truth;
    for (auto pos = b.range.first; pos < b.range.last; ++pos) { const auto b_id = order[pos]; truth_of(a_id, b_id, in, sh); if (in.size() < kmax) { Emitted e; e.interior = in; e.shell = sh; for (int ax=0;ax<3;++ax) { e.key.center_twice[ax] = (std::uint32_t)P[a_id][ax] + P[b_id][ax]; i64 d = (i64)P[a_id][ax]-P[b_id][ax]; e.key.diameter_squared += (u64)(d*d); } truth[{std::min(a_id,b_id), std::max(a_id,b_id)}] = e; } }
    alive_total += truth.size();
    for (const auto& op : opts) {
      std::map<Key, Emitted> ref; u64 rdups = 0;
      auto rres = run_q2_anchor_reference(index, rank, bnode, kmax, [&](const Q2Support& sp) { collect(ref, rdups, sp); }, op.sb, op.o);
      Sums rs; add_sums(rs, rres.census, rres.sibling_work, rres.order_work);
      if (!(ref == truth) || rdups) ++ref_mismatch;
      // (A) lignée par détachement récursif, budgets 1 et 5
      for (auto budget : std::vector<std::size_t>{1, 5}) {
        ++lineages; std::map<Key, Emitted> got; u64 gdups = 0; Q2CensusConsumer consumer = [&](const Q2Support& sp) { collect(got, gdups, sp); };
        std::deque<std::unique_ptr<Q2CensusContinuation>> work; work.push_back(make_q2_census_continuation(index, rank, bnode, kmax, op.sb, op.o));
        Sums ls; u64 detached = 0, imported = 0, transferred = 0; bool ident_ok = true; unsigned guard = 0;
        while (!work.empty()) {
          auto cont = std::move(work.front()); work.pop_front();
          bool done = false;
          while (!done) {
            if (++guard > 50000000U) { std::printf("GUARD\n"); break; }
            { Guard g; done = cont->advance(budget, consumer); }
            if (!done) {
              const auto before = cont->snapshot();
              auto child = cont->detach_pending();
              if (child) {
                const auto after = cont->snapshot(); const auto cs = child->snapshot();
                // identités : candidates de l'enfant = décrément du donneur ; enfant imported_frames = 1 ; lignée conservée
                const u64 m = cs.census.candidate_pairs;
                if (before.census.candidate_pairs != after.census.candidate_pairs + m || cs.resume_work.max_pending_tasks != 1 || after.detach_work.detached_frames != before.detach_work.detached_frames + 1 || after.detach_work.transferred_pairs != before.detach_work.transferred_pairs + m) ident_ok = false;
                ++detached; transferred += m; work.push_back(std::move(child));
              }
            }
          }
          const auto fin = cont->snapshot(); add_sums(ls, fin.census, fin.sibling_work, fin.order_work); imported += fin.detach_work.imported_frames;
        }
        detached_total += detached;
        std::string bad; const bool sums_ok = same_sums(ls, rs, bad);
        if (!(got == ref) || gdups) { ++mismatch; std::printf("MISMATCH rank=%zu b=%zu opt=%s budget=%zu got=%zu ref=%zu dups=%llu\n", rank, bnode, op.name, budget, got.size(), ref.size(), (unsigned long long)gdups); }
        if (!sums_ok) { ++sum_breaks; std::printf("SUMS rank=%zu b=%zu opt=%s budget=%zu %s detached=%llu\n", rank, bnode, op.name, budget, bad.c_str(), (unsigned long long)detached); }
        if (!ident_ok || detached != imported) { ++ident_breaks; std::printf("IDENT rank=%zu b=%zu opt=%s detached=%llu imported=%llu\n", rank, bnode, op.name, (unsigned long long)detached, (unsigned long long)imported); }
      }
      // (B) run_q2_anchor_parallel
      for (auto W : std::vector<std::size_t>{1, 2, 4, 8}) for (auto Q : std::vector<std::size_t>{1, 256}) for (auto C : std::vector<std::size_t>{1, 8}) {
        ++par_runs; std::vector<std::map<Key, Emitted>> slots(W); std::vector<Q2CensusConsumer> consumers;
        for (std::size_t w = 0; w < W; ++w) consumers.push_back([&slots, w](const Q2Support& sp) { Key k{std::min(sp.a_id, sp.b_id), std::max(sp.a_id, sp.b_id)}; Emitted e; e.interior.assign(sp.interior.begin(), sp.interior.end()); e.shell.assign(sp.shell.begin(), sp.shell.end()); e.key = sp.key; std::sort(e.interior.begin(), e.interior.end()); std::sort(e.shell.begin(), e.shell.end()); auto& m = slots[w]; if (m.count(k)) e.interior.push_back(~std::size_t{0}); m[k] = e; });
        Q2CensusParallelOptions o; o.workers = W; o.quantum = Q; o.queue_capacity = C;
        Q2CensusParallelResult pres; { Guard g; pres = run_q2_anchor_parallel(index, rank, bnode, kmax, o, consumers, op.sb, op.o); }
        std::map<Key, Emitted> merged; u64 dups = 0; for (auto& m : slots) for (auto& [k, e] : m) { if (merged.count(k)) ++dups; merged[k] = e; }
        Sums ps; add_sums(ps, pres.sum.census, pres.sum.sibling_work, pres.sum.order_work); std::string bad;
        if (!(merged == ref) || dups) { ++par_mismatch; par_dups += dups; std::printf("PAR MISMATCH rank=%zu b=%zu opt=%s W=%zu Q=%zu C=%zu got=%zu ref=%zu dups=%llu\n", rank, bnode, op.name, W, Q, C, merged.size(), ref.size(), (unsigned long long)dups); }
        if (!same_sums(ps, rs, bad)) { ++par_sum_breaks; std::printf("PAR SUMS rank=%zu b=%zu opt=%s W=%zu Q=%zu C=%zu %s\n", rank, bnode, op.name, W, Q, C, bad.c_str()); }
      }
      // vivacité : tout slot lève à son premier support (W=8, quantum 1, file 1) : si la référence a au moins un
      // support, une exception doit sortir (une seule fois) et l'appel doit rendre la main sans blocage.
      if (!ref.empty()) { ++live_runs; const std::size_t W = 8; std::vector<Q2CensusConsumer> consumers;
        for (std::size_t w = 0; w < W; ++w) consumers.push_back([](const Q2Support&) { throw std::runtime_error("audit: slot failure"); });
        Q2CensusParallelOptions o; o.workers = W; o.quantum = 1; o.queue_capacity = 1; Guard g;
        try { auto r = run_q2_anchor_parallel(index, rank, bnode, kmax, o, consumers, op.sb, op.o); static_cast<void>(r); std::printf("LIVENESS: no exception rank=%zu b=%zu\n", rank, bnode); } catch (const std::runtime_error&) { ++live_exc; } }
    }
  }
  std::printf("SUMMARY family=%s n=%zu kmax=%u pairs=%zu lineages=%llu detached=%llu alive=%llu ref_mismatch=%llu mismatch=%llu sum_breaks=%llu ident_breaks=%llu par_runs=%llu par_mismatch=%llu par_sum_breaks=%llu par_dups=%llu liveness_runs=%llu liveness_exceptions=%llu\n",
    fam.c_str(), N, kmax, pairs.size(), (unsigned long long)lineages, (unsigned long long)detached_total, (unsigned long long)alive_total, (unsigned long long)ref_mismatch, (unsigned long long)mismatch, (unsigned long long)sum_breaks, (unsigned long long)ident_breaks, (unsigned long long)par_runs, (unsigned long long)par_mismatch, (unsigned long long)par_sum_breaks, (unsigned long long)par_dups, (unsigned long long)live_runs, (unsigned long long)live_exc);
  return (mismatch == 0 && ref_mismatch == 0 && sum_breaks == 0 && ident_breaks == 0 && par_mismatch == 0 && par_sum_breaks == 0 && live_runs == live_exc) ? 0 : 1;
}
