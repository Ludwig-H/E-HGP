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
  long long seed = 3;
  int lift_cap = 0;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--family=", 0) == 0) { if (!parse_cloud_family(a.c_str() + 9, &family)) return 2; }
    else if (a.rfind("--n=", 0) == 0) n = std::atoi(a.c_str() + 4);
    else if (a.rfind("--coord=", 0) == 0) coord = std::atoi(a.c_str() + 8);
    else if (a.rfind("--pretest-query-min=", 0) == 0) pretest_min = (size_t)std::atoll(a.c_str() + 20);
    else if (a.rfind("--seed=", 0) == 0) seed = std::atoll(a.c_str() + 7);
    // Contrefactuel de mesure : borne le saut de canopee de `terrain`, qui vaut
    // sinon coord/8 et grandit avec le nuage (docs/TERRAIN_CANOPEE.md). Ce
    // n'est PAS un regime de production.
    else if (a.rfind("--canopy-lift-cap=", 0) == 0) lift_cap = std::atoi(a.c_str() + 18);
    else return 2;
  }
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const CloudIndex ix = build_cloud_index(make_family_input(family, n, coord, seed, lift_cap));
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
  u64 query_rectangles = 0, query_candidates = 0, pretest_anchors = 0, pretest_sites = 0;
  const auto now = [] { return std::chrono::steady_clock::now(); };
  const auto ns = [](auto t0) { return (u64)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count(); };
  const u64 cpu0 = mhgp5::generate_detail::mhgp5_thread_cpu_ns();
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
    if (by_query) {
      t0 = now();
      rect_diametral_candidates(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), &sc.query, &sc.cover_nodes);
      t_query += ns(t0);
      ++query_rectangles;
      query_candidates += sc.query.size();
    }
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
          ++pretest_anchors;
          pretest_sites += sc.query.size();
          t0 = now();
          const int k = anchor_kill_from_candidates(sc.query, ix.upos, ua, ub, pa, pb, D2, Lane::kQ4, 8, h_of[2]);
          t_query += ns(t0);
          if (k == 1) { ++ls.anchors_killed_w4; continue; }
          if (k == 2) { ++ls.anchors_killed_sectors[2]; continue; }
        }
        t0 = now();
        const u64 cover_visits_before = sc.visits;
        anchor_cover_from_handles(ix, sc.handles, pa, pb, D2, 3, &sc.cover, &sc.visits, &sc.cover_tmp);
        t_cover += ns(t0);
        ++ls.q4_covers_built;
        ls.q4_cover_visits += sc.visits - cover_visits_before;
        ls.q4_cover_sites += sc.cover.size();
        t0 = now();
        generate_detail::process_anchor_q4(ix, sc, ua, ub, pa, pb, D2, h_of[2], float_on, false, false, false, &lo, &ls,
                                           by_query ? generate_detail::AnchorPretests::kAlreadyApplied : generate_detail::AnchorPretests::kApply);
        t_body += ns(t0);
      }
  }
  const double total = (double)ns(t_all) / 1e6;
  const double cpu_ms = (double)(mhgp5::generate_detail::mhgp5_thread_cpu_ns() - cpu0) / 1e6;
  const auto ms = [](u64 x) { return (double)x / 1e6; };
  const u128 core_partition = (u128)ls.q4_cert[0] + ls.q4_cert[1] + ls.q4_cert[5];
  const u128 depth_partition = (u128)ls.depth_killed[2] + ls.candidates[2];
  const u128 completion_partition = (u128)ls.q4_rej_lens + ls.q4_rej_owner + ls.q4_rej_once + ls.q4_rej_i64 +
                                    ls.q4_rej_face_power + ls.q4_rej_det + ls.q4_rej_center + ls.depth_killed[2] +
                                    ls.candidates[2];
  const bool core_ok = core_partition == ls.q4_core_site_tests;
  const bool depth_ok = depth_partition == ls.q4_depth_entries;
  const bool completions_ok = completion_partition == ls.q4_completions;
  const bool query_nonempty = pretest_min != 0 || (query_rectangles > 0 && query_candidates > 0);
  const bool nonempty = query_nonempty && ls.q4_covers_built > 0 && ls.q4_cover_visits > 0 && ls.q4_cover_sites > 0 &&
                        ls.q4_core_site_tests > 0 && ls.q4_depth_entries > 0 && ls.q4_power_tests > 0 &&
                        ls.q4_completions > 0;
  std::printf("parametres graine=%lld canopy_lift_cap=%d\n", seed, lift_cap);
  std::printf("q4_stage_probe famille=%s n=%d rectangles=%zu ancres_post_hist=%llu candidats=%llu seeds=%llu core_tues=%llu corde_tues=%llu completions=%llu profonds=%llu\n",
              cloud_family_name(family), n, alive.size(), (unsigned long long)anchors, (unsigned long long)lo.size(),
              (unsigned long long)ls.seeds[1], (unsigned long long)ls.seeds_killed_core, (unsigned long long)ls.seeds_killed_chord,
              (unsigned long long)ls.q4_completions, (unsigned long long)ls.depth_killed[2]);
  std::printf("profil (ms, 1 fil, ratios seulement ; pretest_query_min=%zu) : total=%.0f histogrammes=%.0f handles=%.0f requetes_pretest=%.0f rectangles_requete=%llu candidats_requete=%llu covers=%.0f corps=%.0f dont tests_ancre=%.0f cœur+corde=%.0f completions=%.0f ; tuees W4=%llu secteurs=%llu\n",
              pretest_min, total, ms(t_hist), ms(t_handles), ms(t_query), (unsigned long long)query_rectangles,
              (unsigned long long)query_candidates, ms(t_cover), ms(t_body), ms(ls.prof_q4_anchor_ns), ms(ls.prof_q4_core_ns), ms(ls.prof_q4_compl_ns),
              (unsigned long long)ls.anchors_killed_w4, (unsigned long long)ls.anchors_killed_sectors[2]);
  std::printf("masses_q4 covers=%llu visites_points_cover=%llu sites_retenus=%llu tests_cœur=%llu entrees_profondeur=%llu tests_q4_power=%llu\n",
              (unsigned long long)ls.q4_covers_built, (unsigned long long)ls.q4_cover_visits,
              (unsigned long long)ls.q4_cover_sites, (unsigned long long)ls.q4_core_site_tests,
              (unsigned long long)ls.q4_depth_entries, (unsigned long long)ls.q4_power_tests);
  std::printf("etages_q4 pretest_ancres=%llu pretest_sites=%llu lentille_balayee=%llu lentille_gardee=%llu grille_sites=%llu grille_ancres=%llu grille_sites_baties=%llu tests_aigus=%llu corde_cellules=%llu affine_remplis=%llu affine_sites=%llu w4_prescan=%llu secteur_sites=%llu\n",
              (unsigned long long)pretest_anchors, (unsigned long long)pretest_sites,
              (unsigned long long)ls.q4_lens_scanned, (unsigned long long)ls.q4_lens_kept,
              (unsigned long long)ls.q4_grid_scan_sites, (unsigned long long)ls.q4_grid_anchors,
              (unsigned long long)ls.q4_grid_built_sites, (unsigned long long)ls.q4_acute_tests,
              (unsigned long long)ls.q4_chord_cell_tests, (unsigned long long)ls.q4_affine_fills,
              (unsigned long long)ls.q4_affine_sites, (unsigned long long)ls.q4_w4_prescan_tests,
              (unsigned long long)ls.q4_sector_sites);
  std::printf("temps_q4 lentille_grille=%.0f dont_grille=%.0f boucle_seeds=%.0f cœur=%.0f completions=%.0f dont_profondeur=%.0f ; grilles_tentees=%llu grilles_baties=%llu\n",
              ms(ls.prof_q4_lens_ns), ms(ls.prof_q4_grid_ns), ms(ls.prof_q4_seedloop_ns), ms(ls.prof_q4_core_ns),
              ms(ls.prof_q4_compl_ns), ms(ls.prof_q4_depth_ns), (unsigned long long)ls.grids_attempted[2],
              (unsigned long long)ls.grids_built[2]);
  std::printf("rejets_q4 lentille=%llu owner=%llu once=%llu i64=%llu face_power=%llu det=%llu centre=%llu profonds=%llu emis=%llu\n",
              (unsigned long long)ls.q4_rej_lens, (unsigned long long)ls.q4_rej_owner, (unsigned long long)ls.q4_rej_once,
              (unsigned long long)ls.q4_rej_i64, (unsigned long long)ls.q4_rej_face_power, (unsigned long long)ls.q4_rej_det,
              (unsigned long long)ls.q4_rej_center, (unsigned long long)ls.depth_killed[2], (unsigned long long)ls.candidates[2]);
  std::printf("seeds_q4 seeds=%llu cellules_tues=%llu cœur_tues=%llu corde_tues=%llu survivants=%llu ancres_covers=%llu ancres_w4=%llu ancres_secteurs=%llu ancres_cellules=%llu\n",
              (unsigned long long)ls.seeds[1], (unsigned long long)ls.seeds_killed_cells[2],
              (unsigned long long)ls.seeds_killed_core, (unsigned long long)ls.seeds_killed_chord,
              (unsigned long long)(ls.seeds[1] - ls.seeds_killed_cells[2] - ls.seeds_killed_core - ls.seeds_killed_chord),
              (unsigned long long)ls.q4_covers_built, (unsigned long long)ls.anchors_killed_w4,
              (unsigned long long)ls.anchors_killed_sectors[2], (unsigned long long)ls.anchors_killed_cells[2]);
  std::printf("certs_q4 cert_pos=%llu cert_neg=%llu jung_kill=%llu jung_skip=%llu jung_repli=%llu repli_flottant=%llu\n",
              (unsigned long long)ls.q4_cert[0], (unsigned long long)ls.q4_cert[1], (unsigned long long)ls.q4_cert[2],
              (unsigned long long)ls.q4_cert[3], (unsigned long long)ls.q4_cert[4], (unsigned long long)ls.q4_cert[5]);
  std::printf("horloge_q4 mur_ms=%.0f cpu_processus_ms=%.0f (machine partagee : mur/cpu mesure la contention)\n", total, cpu_ms);
  std::printf("identites_q4 cœur=%s profondeur=%s completions=%s non_vacuite=%s\n", core_ok ? "OK" : "ECHEC",
              depth_ok ? "OK" : "ECHEC", completions_ok ? "OK" : "ECHEC", nonempty ? "OK" : "ECHEC");
  if (!nonempty) return 3;
  return core_ok && depth_ok && completions_ok ? 0 : 1;
}
