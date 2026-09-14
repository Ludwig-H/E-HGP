// Auditeur B : crédits locaux Pool/DualBlocks appliqués aux rectangles TERMINAUX du premier front v8
// (da366f7f), par classe de taille de facteur ; résidu par voie avant census.
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "wspd/front.hpp"
#include "pipeline/q2_census.hpp"
#include "pipeline/local_credits.hpp"
#include "front_fixtures.hpp"
using namespace mhgp8;
using Clock = std::chrono::steady_clock;
static double ms(Clock::time_point a, Clock::time_point b){ return std::chrono::duration<double,std::milli>(b-a).count(); }
int main(int argc, char** argv) {
  if (argc < 6) { std::fprintf(stderr, "usage: n family kmax s threshold [pool|dual] [pure|samples]\n"); return 2; }
  const std::size_t n = std::strtoull(argv[1], nullptr, 10); const std::string fam = argv[2];
  const unsigned kmax = std::atoi(argv[3]); const unsigned s = std::atoi(argv[4]); const std::size_t thr = std::strtoull(argv[5], nullptr, 10);
  const std::string strat = argc > 6 ? argv[6] : "pool"; const std::string fmode = argc > 7 ? argv[7] : "samples";
  auto fx = bench::make_front_fixture(n, fam, 3);
  auto cloud = prepare_cloud(fx.points);
  auto index = make_q2_cloud_index(cloud);
  const auto nodes = index->spatial_nodes(); const auto order = index->spatial_order(); const auto pts = index->cloud().points();
  u64 rects = 0, big = 0, pairs_total[3] = {0,0,0}, pairs_after[3] = {0,0,0}, pairs_big[3] = {0,0,0}, big_after[3] = {0,0,0};
  double credit_ms = 0; u64 predicate_tests = 0;
  const Strategy st = strat == "dual" ? Strategy::DualBlocks : Strategy::Pool;
  auto consumer = [&](const WspdRectangle& r) {
    ++rects;
    const auto& A = nodes[r.a_node]; const auto& B = nodes[r.b_node];
    const u64 mass = (u64)A.range.size() * B.range.size();
    for (unsigned lane = 0; lane < 3; ++lane) if (r.lane_mask & (1U << lane)) pairs_total[lane] += mass;
    if (std::max(A.range.size(), B.range.size()) < thr) { for (unsigned lane = 0; lane < 3; ++lane) if (r.lane_mask & (1U << lane)) pairs_after[lane] += mass; return; }
    ++big;
    const auto t0 = Clock::now();
    RectangleInput in; in.points.reserve(A.range.size() + B.range.size());
    for (auto k = A.range.first; k < A.range.last; ++k) in.points.push_back(pts[order[k]]);
    for (auto k = B.range.first; k < B.range.last; ++k) in.points.push_back(pts[order[k]]);
    in.a = {0, A.range.size()}; in.b = {A.range.size(), in.points.size()};
    auto rect = prepare_rectangle(in, kmax, s);
    for (unsigned lane = 0; lane < 3; ++lane) {
      if (!(r.lane_mask & (1U << lane))) continue;
      const Lane L = lane == 0 ? Lane::Q2 : lane == 1 ? Lane::Q3 : Lane::Q4;
      auto plan = make_credit_plan(rect, L, st);
      pairs_big[lane] += mass; big_after[lane] += plan.candidate_pairs(); pairs_after[lane] += plan.candidate_pairs();
      predicate_tests += plan.work().predicates.universal_queries + plan.work().predicates.block_bound_tests;
    }
    credit_ms += ms(t0, Clock::now());
  };
  const auto f0 = Clock::now();
  auto res = run_wspd_front(*index, kmax, s, fmode == "pure" ? WspdFrontMode::Pure : WspdFrontMode::MidpointSamples, consumer);
  const auto f1 = Clock::now();
  std::printf("family=%s n=%zu kmax=%u s=%u front=%s strategy=%s threshold=%zu rects=%llu big=%llu front_total_ms=%.1f credit_ms=%.1f predicate_tests=%llu\n",
    fam.c_str(), n, kmax, s, fmode.c_str(), strat.c_str(), thr, (unsigned long long)rects, (unsigned long long)big, ms(f0,f1), credit_ms, (unsigned long long)predicate_tests);
  for (unsigned lane = 0; lane < 3; ++lane)
    std::printf("  q%u residual_front=%llu residual_after_credits=%llu (x%.4f) | big_rects_pairs=%llu -> %llu (x%.4f)\n", lane + 2,
      (unsigned long long)pairs_total[lane], (unsigned long long)pairs_after[lane], pairs_total[lane] ? (double)pairs_after[lane]/pairs_total[lane] : 0.0,
      (unsigned long long)pairs_big[lane], (unsigned long long)big_after[lane], pairs_big[lane] ? (double)big_after[lane]/pairs_big[lane] : 0.0);
  static_cast<void>(res);
  return 0;
}
