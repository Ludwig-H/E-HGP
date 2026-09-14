// Auditeur B (14 sept. 2026) : vérité terrain sur les paires q2 survivantes des crédits locaux
// (Pool ou DualBlocks) appliqués aux rectangles terminaux à gros facteurs du front v8 (da366f7f).
// Pour chaque survivante, compte exact des sites strictement intérieurs à sa boule diamétrale
// contre TOUS les sites (force brute, arrêt à Kmax) : combien sont réellement retenues (< Kmax) ?
// Contrôle de sûreté : aucune paire rejetée par les crédits ne doit avoir moins de Kmax intérieurs
// (vérifié par échantillon déterministe des rejetées, pas exhaustivement).
#include <algorithm>
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
  if (argc < 6) { std::fprintf(stderr, "usage: n family kmax s threshold [pool|dual] [sample_rejected]\n"); return 2; }
  const std::size_t n = std::strtoull(argv[1], nullptr, 10); const std::string fam = argv[2];
  const unsigned kmax = std::atoi(argv[3]); const unsigned s = std::atoi(argv[4]); const std::size_t thr = std::strtoull(argv[5], nullptr, 10);
  const std::string strat = argc > 6 ? argv[6] : "pool"; const u64 sample_rejected = argc > 7 ? std::strtoull(argv[7], nullptr, 10) : 20000;
  auto fx = bench::make_front_fixture(n, fam, 3);
  auto cloud = prepare_cloud(fx.points);
  auto index = make_q2_cloud_index(cloud);
  const auto nodes = index->spatial_nodes(); const auto order = index->spatial_order(); const auto pts = index->cloud().points();
  const std::size_t N = pts.size();
  // compte strict intérieur, arrêté à cap
  auto interior_capped = [&](std::size_t i, std::size_t j, unsigned cap) {
    unsigned c = 0;
    for (std::size_t k = 0; k < N; ++k) {
      i64 h = 0; for (int ax=0;ax<3;++ax) h += ((i64)pts[k][ax]-pts[i][ax])*((i64)pts[j][ax]-pts[k][ax]);
      if (h > 0 && ++c >= cap) return c;
    }
    return c;
  };
  const Strategy st = strat == "dual" ? Strategy::DualBlocks : Strategy::Pool;
  u64 rects = 0, big = 0, big_mass = 0, survivors = 0, alive = 0, rejected_sampled = 0, rejected_unsound = 0;
  std::vector<u64> hist(kmax + 1, 0); // hist[c] = survivantes à c intérieurs (c<kmax), hist[kmax] = mortes
  std::vector<u64> alive_per_rect; alive_per_rect.reserve(64);
  double credit_ms = 0, truth_ms = 0; u64 truth_steps = 0;
  u64 lcg = 0x9E3779B97F4A7C15ULL;
  auto consumer = [&](const WspdRectangle& r) {
    ++rects;
    if (!(r.lane_mask & 1U)) return;
    const auto& A = nodes[r.a_node]; const auto& B = nodes[r.b_node];
    if (std::max(A.range.size(), B.range.size()) < thr) return;
    ++big; big_mass += (u64)A.range.size() * B.range.size();
    const auto t0 = Clock::now();
    RectangleInput in; in.points.reserve(A.range.size() + B.range.size());
    for (auto k = A.range.first; k < A.range.last; ++k) in.points.push_back(pts[order[k]]);
    for (auto k = B.range.first; k < B.range.last; ++k) in.points.push_back(pts[order[k]]);
    in.a = {0, A.range.size()}; in.b = {A.range.size(), in.points.size()};
    auto rect = prepare_rectangle(in, kmax, s);
    auto plan = make_credit_plan(rect, Lane::Q2, st);
    credit_ms += ms(t0, Clock::now());
    const auto t1 = Clock::now();
    u64 alive_here = 0;
    plan.for_each_candidate([&](std::size_t la, std::size_t lb) {
      const std::size_t gi = order[A.range.first + la], gj = order[B.range.first + (lb - A.range.size())];
      ++survivors;
      const unsigned c = interior_capped(gi, gj, kmax);
      ++hist[c]; if (c < kmax) { ++alive; ++alive_here; }
    });
    alive_per_rect.push_back(alive_here);
    // échantillon déterministe de paires rejetées : sûreté (toutes doivent avoir >= kmax intérieurs)
    const u64 mass = (u64)A.range.size() * B.range.size();
    const u64 want = std::min<u64>(sample_rejected, mass);
    for (u64 t = 0; t < want; ++t) {
      lcg = lcg * 6364136223846793005ULL + 1442695040888963407ULL;
      const std::size_t la = (std::size_t)((lcg >> 33) % A.range.size());
      lcg = lcg * 6364136223846793005ULL + 1442695040888963407ULL;
      const std::size_t lb = (std::size_t)((lcg >> 33) % B.range.size());
      if (plan.keeps(la, A.range.size() + lb)) continue;
      ++rejected_sampled;
      const unsigned c = interior_capped(order[A.range.first + la], order[B.range.first + lb], kmax);
      if (c < kmax) ++rejected_unsound;
    }
    truth_ms += ms(t1, Clock::now());
  };
  const auto f0 = Clock::now();
  auto res = run_wspd_front(*index, kmax, s, WspdFrontMode::MidpointSamples, consumer);
  const auto f1 = Clock::now();
  static_cast<void>(res); static_cast<void>(truth_steps);
  std::sort(alive_per_rect.begin(), alive_per_rect.end());
  std::printf("family=%s n=%zu kmax=%u s=%u strategy=%s threshold=%zu rects=%llu big=%llu big_mass=%llu survivors=%llu alive=%llu dead=%llu rejected_sampled=%llu rejected_unsound=%llu front_total_ms=%.1f credit_ms=%.1f truth_ms=%.1f\n",
    fam.c_str(), n, kmax, s, strat.c_str(), thr, (unsigned long long)rects, (unsigned long long)big, (unsigned long long)big_mass, (unsigned long long)survivors, (unsigned long long)alive, (unsigned long long)(survivors - alive),
    (unsigned long long)rejected_sampled, (unsigned long long)rejected_unsound, ms(f0,f1), credit_ms, truth_ms);
  std::printf("  hist_interior:"); for (unsigned c = 0; c <= kmax; ++c) std::printf(" %u:%llu", c, (unsigned long long)hist[c]); std::printf("\n");
  std::printf("  alive_per_big_rect(sorted):"); for (auto v : alive_per_rect) std::printf(" %llu", (unsigned long long)v); std::printf("\n");
  return rejected_unsound == 0 ? 0 : 3;
}
