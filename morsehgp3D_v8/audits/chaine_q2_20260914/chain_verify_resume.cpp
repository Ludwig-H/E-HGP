// Auditeur B (14 sept. 2026) — vérificateur des continuations de census q2 (q2_census_resume.hpp).
// Pour des couples (ancre, nœud B) tirés de l'index, trois réglages d'options et des budgets de
// transitions 1, 3 et 1000 : la continuation avancée jusqu'à Done doit émettre exactement le
// multiensemble de supports de la référence série run_q2_anchor_reference, qui doit lui-même être
// la vérité de force brute (paires (a,b), b ∈ B, à moins de Kmax intérieurs stricts, avec intérieurs,
// coquille complète et clé) ; les compteurs géométriques discrets finaux doivent égaler ceux de la
// référence ; un « transfert » exécute chaque pas dans un fil neuf joint avant le suivant ; après
// Done, un pas supplémentaire rend true sans émission. Les pauses après crédit, en phase différée et
// pendant l'émission sont comptées pour attester les cas positifs de suspension.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include "pipeline/q2_census_resume.hpp"
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
int main(int argc, char** argv) {
  if (argc < 5) { std::fprintf(stderr, "usage: family|adv:name n kmax s [seed] [max_pairs]\n"); return 2; }
  const std::string fam = argv[1]; const std::size_t n = std::strtoull(argv[2], nullptr, 10);
  const unsigned kmax = std::atoi(argv[3]); const unsigned s = std::atoi(argv[4]); const unsigned seed = argc > 5 ? std::atoi(argv[5]) : 3;
  const std::size_t max_pairs = argc > 6 ? std::strtoull(argv[6], nullptr, 10) : 400;
  static_cast<void>(s);
  std::vector<Point3> pts;
  if (fam.rfind("adv:", 0) == 0) pts = adversarial(fam.substr(4), seed); else pts = bench::make_front_fixture(n, fam, seed).points;
  auto cloud = prepare_cloud(pts); auto index = make_q2_cloud_index(cloud);
  const auto P = index->cloud().points(); const std::size_t N = P.size();
  const auto nodes = index->spatial_nodes(); const auto order = index->spatial_order();
  // couples (ancre, B) : tous les nœuds B de taille <= 64, ancres = premier rang hors B avant, après et un rang pseudo-aléatoire hors B
  std::vector<std::pair<std::size_t, std::size_t>> pairs; u64 lcg = 0x9E3779B97F4A7C15ULL + seed;
  for (std::size_t id = 0; id < nodes.size() && pairs.size() < max_pairs; ++id) {
    const auto& b = nodes[id]; if (b.range.size() > 64) continue;
    std::vector<std::size_t> anchors;
    if (b.range.first > 0) anchors.push_back(b.range.first - 1);
    if (b.range.last < N) anchors.push_back(b.range.last);
    lcg = lcg * 6364136223846793005ULL + 1442695040888963407ULL; const std::size_t r = (std::size_t)((lcg >> 33) % N);
    if (r < b.range.first || r >= b.range.last) anchors.push_back(r);
    for (auto a : anchors) if (pairs.size() < max_pairs) pairs.push_back({a, id});
  }
  auto truth_of = [&](std::size_t i, std::size_t j, std::vector<std::size_t>& in, std::vector<std::size_t>& sh) {
    in.clear(); sh.clear();
    for (std::size_t k = 0; k < N; ++k) { i64 h = 0; for (int ax=0;ax<3;++ax) h += ((i64)P[k][ax]-P[i][ax])*((i64)P[j][ax]-P[k][ax]); if (h > 0) in.push_back(k); else if (h == 0) sh.push_back(k); }
  };
  struct Opt { Q2SiblingMode sb; Q2WitnessOrder o; const char* name; };
  const Opt opts[] = {{Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, "global"}, {Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, "sib+compl"}, {Q2SiblingMode::Disabled, Q2WitnessOrder::ComplementFirst, "compl"}};
  const std::vector<std::size_t> budgets = {1, 3, 1000};
  u64 continuations = 0, mismatch = 0, ref_mismatch = 0, counter_breaks = 0, pauses = 0, pauses_after_credit = 0, pauses_inside_deferred = 0, pauses_during_emission = 0, max_pending = 0, transfers = 0, after_done_emissions = 0, alive_total = 0;
  auto collect = [](std::map<Key, Emitted>& m, u64& dups, const Q2Support& sp) { Key k{std::min(sp.a_id, sp.b_id), std::max(sp.a_id, sp.b_id)}; if (m.count(k)) ++dups; Emitted e; e.interior.assign(sp.interior.begin(), sp.interior.end()); e.shell.assign(sp.shell.begin(), sp.shell.end()); e.key = sp.key; std::sort(e.interior.begin(), e.interior.end()); std::sort(e.shell.begin(), e.shell.end()); m[k] = e; };
  std::vector<std::size_t> in, sh;
  for (const auto& [rank, bnode] : pairs) {
    const auto a_id = order[rank]; const auto& b = nodes[bnode];
    // vérité
    std::map<Key, Emitted> truth;
    for (auto pos = b.range.first; pos < b.range.last; ++pos) { const auto b_id = order[pos]; truth_of(a_id, b_id, in, sh); if (in.size() < kmax) { Emitted e; e.interior = in; e.shell = sh; for (int ax=0;ax<3;++ax) { e.key.center_twice[ax] = (std::uint32_t)P[a_id][ax] + P[b_id][ax]; i64 d = (i64)P[a_id][ax]-P[b_id][ax]; e.key.diameter_squared += (u64)(d*d); } truth[{std::min(a_id,b_id), std::max(a_id,b_id)}] = e; } }
    alive_total += truth.size();
    for (const auto& op : opts) {
      std::map<Key, Emitted> ref; u64 rdups = 0;
      auto rres = run_q2_anchor_reference(index, rank, bnode, kmax, [&](const Q2Support& sp) { collect(ref, rdups, sp); }, op.sb, op.o);
      if (!(ref == truth) || rdups) { ++ref_mismatch; std::printf("REF MISMATCH rank=%zu b=%zu opt=%s\n", rank, bnode, op.name); }
      for (auto budget : budgets) for (int transfer = 0; transfer < 2; ++transfer) {
        if (transfer && budget != 3) continue;  // transfert de fil testé au budget 3
        ++continuations; if (transfer) ++transfers;
        auto cont = make_q2_census_continuation(index, rank, bnode, kmax, op.sb, op.o);
        std::map<Key, Emitted> got; u64 gdups = 0;
        Q2CensusConsumer consumer = [&](const Q2Support& sp) { collect(got, gdups, sp); };
        bool done = false; unsigned guard = 0;
        while (!done) {
          if (++guard > 20000000U) { std::printf("GUARD rank=%zu b=%zu\n", rank, bnode); break; }
          if (transfer) { std::thread t([&] { done = cont->advance(budget, consumer); }); t.join(); }
          else done = cont->advance(budget, consumer);
        }
        // après Done : un pas de plus rend true sans émission
        const auto before = got.size(); const bool again = cont->advance(1, consumer); if (!again || got.size() != before) ++after_done_emissions;
        const auto snap = cont->snapshot(); const auto pend = cont->pending();
        const bool ok = (got == ref) && gdups == 0 && snap.status == Q2CensusContinuationStatus::Done && pend.task_count == 0;
        const auto& cw = snap.census.work; const auto& rw = rres.census.work;
        const bool counters_ok = snap.census.candidate_pairs == rres.census.candidate_pairs && snap.census.accepted_pairs == rres.census.accepted_pairs && snap.census.rejected_pairs == rres.census.rejected_pairs
          && cw.count_node_visits == rw.count_node_visits && cw.count_bound_tests == rw.count_bound_tests && cw.count_point_tests == rw.count_point_tests && cw.query_tasks == rw.query_tasks && cw.payload_supports == rw.payload_supports && cw.payload_shell_sites == rw.payload_shell_sites
          && snap.sibling_work.rejected_pairs == rres.sibling_work.rejected_pairs && snap.order_work.phase_switches == rres.order_work.phase_switches;
        if (!ok) { ++mismatch; std::printf("MISMATCH rank=%zu b=%zu opt=%s budget=%zu transfer=%d got=%zu ref=%zu dups=%llu status=%d pending=%zu\n", rank, bnode, op.name, budget, transfer, got.size(), ref.size(), (unsigned long long)gdups, (int)snap.status, pend.task_count); }
        if (!counters_ok) { ++counter_breaks; std::printf("COUNTERS rank=%zu b=%zu opt=%s budget=%zu cand=%llu/%llu acc=%llu/%llu visits=%llu/%llu tasks=%llu/%llu\n", rank, bnode, op.name, budget, (unsigned long long)snap.census.candidate_pairs, (unsigned long long)rres.census.candidate_pairs, (unsigned long long)snap.census.accepted_pairs, (unsigned long long)rres.census.accepted_pairs, (unsigned long long)cw.count_node_visits, (unsigned long long)rw.count_node_visits, (unsigned long long)cw.query_tasks, (unsigned long long)rw.query_tasks); }
        const auto& z = snap.resume_work;
        pauses += z.pauses; pauses_after_credit += z.pauses_after_credit; pauses_inside_deferred += z.pauses_inside_deferred; pauses_during_emission += z.pauses_during_emission; max_pending = std::max<u64>(max_pending, z.max_pending_tasks);
      }
    }
  }
  std::printf("SUMMARY family=%s n=%zu kmax=%u pairs=%zu continuations=%llu transfers=%llu alive=%llu ref_mismatch=%llu mismatch=%llu counter_breaks=%llu after_done_emissions=%llu pauses=%llu pauses_after_credit=%llu pauses_inside_deferred=%llu pauses_during_emission=%llu max_pending=%llu\n",
    fam.c_str(), N, kmax, pairs.size(), (unsigned long long)continuations, (unsigned long long)transfers, (unsigned long long)alive_total, (unsigned long long)ref_mismatch, (unsigned long long)mismatch, (unsigned long long)counter_breaks, (unsigned long long)after_done_emissions, (unsigned long long)pauses, (unsigned long long)pauses_after_credit, (unsigned long long)pauses_inside_deferred, (unsigned long long)pauses_during_emission, (unsigned long long)max_pending);
  return (mismatch == 0 && ref_mismatch == 0 && counter_breaks == 0 && after_done_emissions == 0) ? 0 : 1;
}
