// MorseHGP3D v5 — SONDE de profil par etage de la lane q4 (jamais un claim) :
// compilee avec -DMHGP5_PROFILE_Q4, elle rejoue la boucle des rectangles de
// la lane q4 de production (un fil) et cumule le temps des covers d'ancre
// (anchor_cover_from_handles), des tests d'ancre (W_4, secteurs), des scans
// de cœur/corde par seed et des completions. Ratios dans un meme run seulement.
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/lanes/edge_cover.hpp"
#include "../src/lanes/sector_kill.hpp"
#include "../src/pipeline/generate.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kEightClusters;
  int n = 4000, coord = 0;
  size_t pretest_min = kPretestQueryMinPoints;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--family=", 0) == 0) { if (!parse_cloud_family(a.c_str() + 9, &family)) return 2; }
    else if (a.rfind("--n=", 0) == 0) n = std::atoi(a.c_str() + 4);
    else if (a.rfind("--coord=", 0) == 0) coord = std::atoi(a.c_str() + 8);
    else if (a.rfind("--pretest-query-min=", 0) == 0) pretest_min = (size_t)std::atoll(a.c_str() + 20);
    else return 2;
  }
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const CloudIndex ix = build_cloud_index(make_family_input(family, n, coord, 3));
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  const u64 smax = 11;
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax), lane_h(Lane::kQ3, smax), lane_h(Lane::kQ4, smax)};
  const bool float_on = float_filter_runtime_enabled();
  std::vector<AliveRect> alive;
  u64 visited = 0, workers = 0;
  generate_detail::alive_rectangles(ix, 8, h_of, 2, 1, &alive, &visited, &workers);
  generate_detail::AnchorScratch sc;
  GenerateStats ls;
  std::vector<BallCandidate> lo;
  u64 t_hist = 0, t_handles = 0, t_cover = 0, t_body = 0, t_query = 0, anchors = 0;
  const auto now = [] { return std::chrono::steady_clock::now(); };
  const auto ns = [](auto t0) { return (u64)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count(); };
  const auto t_all = now();
  for (const AliveRect& ar : alive) {
    auto t0 = now();
    generate_detail::corner_histograms(ix, Lane::kQ4, ar.r, &sc.ha, &sc.hb);
    t_hist += ns(t0);
    const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
    t0 = now();
    rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes);
    t_handles += ns(t0);
    sc.handle_points = 0;
    for (const NodeRef h : sc.handles) { const NodeRange r = ix.range_of(h); sc.handle_points += (u64)(r.last - r.first + 1); }
    const bool by_query = sc.handle_points >= pretest_min;
    const u64 need = h_of[2] - ar.core;
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        ++ls.anchors[2];
        if (sc.ha[(size_t)(ua - ra.first)] + sc.hb[(size_t)(ub - rb.first)] >= need) { ++ls.anchors_killed_hist[2]; continue; }
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        ++anchors;
        if (by_query) {
          t0 = now();
          const int k = anchor_kill_from_query(ix, ua, ub, pa, pb, D2, Lane::kQ4, 8, h_of[2], &sc.query);
          t_query += ns(t0);
          if (k == 1) { ++ls.anchors_killed_w4; continue; }
          if (k == 2) { ++ls.anchors_killed_sectors[2]; continue; }
        }
        t0 = now();
        anchor_cover_from_handles(ix, sc.handles, pa, pb, D2, 3, &sc.cover, &sc.visits, &sc.cover_tmp);
        t_cover += ns(t0);
        t0 = now();
        generate_detail::process_anchor_q4(ix, sc, ua, ub, pa, pb, D2, h_of[2], float_on, false, false, false, &lo, &ls,
                                           by_query ? generate_detail::AnchorPretests::kAlreadyApplied : generate_detail::AnchorPretests::kApply);
        t_body += ns(t0);
      }
  }
  const double total = (double)ns(t_all) / 1e6;
  const auto ms = [](u64 x) { return (double)x / 1e6; };
  std::printf("q4_stage_probe famille=%s n=%d rectangles=%zu ancres_post_hist=%llu candidats=%llu seeds=%llu core_tues=%llu corde_tues=%llu completions=%llu profonds=%llu\n",
              cloud_family_name(family), n, alive.size(), (unsigned long long)anchors, (unsigned long long)lo.size(),
              (unsigned long long)ls.seeds[1], (unsigned long long)ls.seeds_killed_core, (unsigned long long)ls.seeds_killed_chord,
              (unsigned long long)ls.q4_completions, (unsigned long long)ls.depth_killed[2]);
  std::printf("profil (ms, 1 fil, ratios seulement ; pretest_query_min=%zu) : total=%.0f histogrammes=%.0f handles=%.0f requetes_pretest=%.0f covers=%.0f corps=%.0f dont tests_ancre=%.0f cœur+corde=%.0f completions=%.0f ; tuees W4=%llu secteurs=%llu\n",
              pretest_min, total, ms(t_hist), ms(t_handles), ms(t_query), ms(t_cover), ms(t_body), ms(ls.prof_q4_anchor_ns), ms(ls.prof_q4_core_ns), ms(ls.prof_q4_compl_ns),
              (unsigned long long)ls.anchors_killed_w4, (unsigned long long)ls.anchors_killed_sectors[2]);
  return 0;
}
