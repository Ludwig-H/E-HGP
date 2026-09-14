// Auditeur B (14 sept. 2026) — pilote du prototype : modes ref (bibliotheque
// constructeur, inchangee), copy (copie sans propagation ni lentille), audit
// (lentille), propagate (temoins herites), verify_ref / verify_propagate
// (force brute exacte sur tous les sites : chaque paire rejetee doit avoir
// >= h_q temoins W_q distincts).
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <utility>
#include "wspd/front.hpp"
#include "pipeline/q2_census.hpp"
#include "front_fixtures.hpp"
namespace mhgp8 { struct LensStats; WspdFrontResult run_wspd_front_lens(const Q2CensusIndex&, unsigned, unsigned, WspdFrontMode, const WspdRectangleConsumer&, LensStats*, bool, bool, bool); }
namespace mhgp8 { struct LensStats {
  u64 lens_tests{}, lens_empty{}, lens_empty_skipped_searches{}, lens_nonempty_searches{};
  u64 lens_empty_but_rejected{}, lens_nonempty_no_rejection{}, lens_nonempty_full_rejection{}, lens_nonempty_partial_rejection{};
  u64 lens_empty_leaf_pairs{}, lens_candidates{};
  u64 inherited_nonzero_products{}, rejections_by_inheritance{}, duplicate_proposals{}; }; }
using namespace mhgp8;
using Clock = std::chrono::steady_clock;
static double ms(Clock::time_point a, Clock::time_point b){ return std::chrono::duration<double,std::milli>(b-a).count(); }
int main(int argc, char** argv) {
  if (argc < 6) { std::fprintf(stderr, "usage: n family kmax s seed [mode: audit|skip|ref]\n"); return 2; }
  const std::size_t n = std::strtoull(argv[1], nullptr, 10); const std::string fam = argv[2];
  const unsigned kmax = std::atoi(argv[3]); const unsigned s = std::atoi(argv[4]); const u64 seed = std::strtoull(argv[5], nullptr, 10);
  const std::string mode = argc > 6 ? argv[6] : "audit";
  auto fx = bench::make_front_fixture(n, fam, seed);
  auto cloud = prepare_cloud(fx.points);
  auto index = make_q2_cloud_index(cloud);
  u64 count = 0;
  const bool verify = mode == "verify_ref" || mode == "verify_propagate";
  std::vector<WspdRectangle> rects;
  auto consumer = [&](const WspdRectangle& r) { ++count; if (verify) rects.push_back(r); };
  LensStats st;
  const auto t0 = Clock::now();
  WspdFrontResult r;
  if (mode == "ref" || mode == "verify_ref") r = run_wspd_front(*index, kmax, s, WspdFrontMode::MidpointSamples, consumer);
  else r = run_wspd_front_lens(*index, kmax, s, WspdFrontMode::MidpointSamples, consumer, &st, mode == "skip", mode == "propagate" || mode == "verify_propagate", mode == "audit" || mode == "skip");
  const auto t1 = Clock::now();
  const auto& w = r.work;
  std::printf("mode=%s family=%s n=%zu kmax=%u s=%u seed=%llu front_ms=%.1f visits=%llu searches=%llu rejected_full=%llu emitted=%llu steps=%llu credits=%llu residual_q2=%llu residual_q3=%llu residual_q4=%llu\n",
    mode.c_str(), fam.c_str(), n, kmax, s, (unsigned long long)seed, ms(t0,t1), (unsigned long long)w.product_visits, (unsigned long long)w.witness_searches,
    (unsigned long long)w.fully_rejected_products, (unsigned long long)w.emitted_rectangles, (unsigned long long)w.witness_descent_steps,
    (unsigned long long)w.witness_lane_credits, (unsigned long long)w.residual_pair_mass[0], (unsigned long long)w.residual_pair_mass[1], (unsigned long long)w.residual_pair_mass[2]);
  if (verify) {
    // Soundness: every pair NOT in the residual of lane q must have >= h_q exact witnesses (H>0 and W3/W4) among all other sites.
    const auto pts = index->cloud().points(); const auto order = index->spatial_order(); const auto nodes = index->spatial_nodes();
    const std::size_t nn = pts.size();
    if (nn > 3000) { std::fprintf(stderr, "verify requires n <= 3000\n"); return 2; }
    std::vector<std::uint8_t> alive(nn * nn, 0);  // alive[i*n+j] bitmask over lanes, i<j original ids
    for (const auto& rc : rects) {
      const auto& A = nodes[rc.a_node]; const auto& B = nodes[rc.b_node];
      for (auto ra = A.range.first; ra < A.range.last; ++ra) for (auto rb = B.range.first; rb < B.range.last; ++rb) {
        auto i = order[ra], j = order[rb]; if (i > j) std::swap(i, j);
        alive[i * nn + j] |= rc.lane_mask;
      }
    }
    const unsigned thr[3] = {kmax, kmax >= 1 ? kmax - 1 : 0, kmax >= 2 ? kmax - 2 : 0};
    u64 unsound = 0, checked = 0, rejected_pairs[3] = {0,0,0};
    for (std::size_t i = 0; i < nn; ++i) for (std::size_t j = i + 1; j < nn; ++j) {
      ++checked;
      unsigned cnt[3] = {0,0,0};
      for (std::size_t k = 0; k < nn; ++k) {
        if (k == i || k == j) continue;
        i64 u[3], w[3], h = 0;
        for (int ax = 0; ax < 3; ++ax) { u[ax] = (i64)pts[k][ax] - pts[i][ax]; w[ax] = (i64)pts[j][ax] - pts[k][ax]; h += u[ax]*w[ax]; }
        if (h <= 0) continue;
        ++cnt[0];
        i128 xi = 0; for (int ax = 0; ax < 3; ++ax) { int jx=(ax+1)%3, kx=(ax+2)%3; i64 c = u[jx]*w[kx]-u[kx]*w[jx]; xi += (i128)c*c; }
        const i128 h2 = (i128)h*h;
        if (3*h2 > xi) ++cnt[1];
        if (2*h2 > xi) ++cnt[2];
      }
      for (unsigned lane = 0; lane < 3; ++lane) {
        if (lane + 2 > kmax + 1) continue;  // inactive lane
        if ((alive[i * nn + j] & (1U << lane)) == 0) { ++rejected_pairs[lane]; if (cnt[lane] < thr[lane]) ++unsound; }
      }
    }
    std::printf("  verify: pairs=%llu rejected(q2/q3/q4)=%llu/%llu/%llu UNSOUND=%llu\n", (unsigned long long)checked, (unsigned long long)rejected_pairs[0], (unsigned long long)rejected_pairs[1], (unsigned long long)rejected_pairs[2], (unsigned long long)unsound);
  }
  if (mode == "propagate" || mode == "verify_propagate")
    std::printf("  inherited_nonzero_products=%llu rejections_by_inheritance=%llu duplicate_proposals=%llu\n", (unsigned long long)st.inherited_nonzero_products, (unsigned long long)st.rejections_by_inheritance, (unsigned long long)st.duplicate_proposals);
  if (mode == "audit" || mode == "skip")
    std::printf("  lens_tests=%llu lens_empty=%llu (%.1f%%) lens_empty_leaf_pairs=%llu empty_but_rejected=%llu nonempty_searches=%llu nonempty_no_rejection=%llu nonempty_partial=%llu nonempty_full=%llu skipped=%llu candidates=%llu\n",
      (unsigned long long)st.lens_tests, (unsigned long long)st.lens_empty, 100.0*st.lens_empty/std::max<u64>(1,st.lens_tests), (unsigned long long)st.lens_empty_leaf_pairs,
      (unsigned long long)st.lens_empty_but_rejected, (unsigned long long)st.lens_nonempty_searches, (unsigned long long)st.lens_nonempty_no_rejection,
      (unsigned long long)st.lens_nonempty_partial_rejection, (unsigned long long)st.lens_nonempty_full_rejection, (unsigned long long)st.lens_empty_skipped_searches, (unsigned long long)st.lens_candidates);
  return 0;
}
