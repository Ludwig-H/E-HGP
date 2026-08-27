// MorseHGP3D v5 — SONDE (jamais un claim) : test de SEED q4 par morceaux de
// corde. Pour un seed (a,b,x), les centres des boules q4 admissibles forment
// la CORDE c_μ − m = v3 + μ n/(2G), |μ| <= μ* = sqrt(J/2), n = (b−a)×(x−a)
// (lemme 4.1 de l'analyse) ; un site z est interieur a la boule c_μ ssi
// P(z) − μ B(z) < 0, P = L/4 (kernel affine du seed), B = n·(z−a) — AFFINE en
// μ. Le cœur de seed de Jung de production (P < 0 et 2P² > J B²) est
// « z universel sur toute la corde » (K = 1). Ici : la corde est coupee en K
// morceaux [μ_i, μ_{i+1}] (bornes ENTIERES sur-approximantes ±μ̂, μ̂ =
// isqrt(J/2) + 1 : une sur-approximation de la corde reste une condition
// suffisante) ; z est universel sur un morceau ssi P − μ_i B < 0 et
// P − μ_{i+1} B < 0 ; le seed est MORT si chaque morceau compte >= h4
// temoins — alors toute completion y (dont le centre est sur la corde) a >= h4
// interieurs et est tuee par le filtre de profondeur : objet inchange.
// Mesure : seeds vivants apres le cœur de production, tues par K = 2 / 4 / 8,
// completions evitees, FAUX POSITIFS (seed tue par morceaux ayant une boule
// emise par la production : doit etre 0, code 1 sinon).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/lanes/edge_cover.hpp"
#include "../src/lanes/q3.hpp"
#include "../src/lanes/q4.hpp"
#include "../src/lanes/sector_kill.hpp"
#include "../src/pipeline/generate.hpp"
#include "../src/spindle/spindle.hpp"

using namespace mhgp5;

namespace {
inline i128 isqrt_floor128(i128 v) {  // v >= 0, v < 2^120
  if (v <= 0) return 0;
  long double x = std::sqrt((long double)v);
  i128 r = (i128)x;
  while (r * r > v) --r;
  while ((r + 1) * (r + 1) <= v) ++r;
  return r;
}
struct BallKeyLess {
  bool operator()(const BallKey& x, const BallKey& y) const { return x < y; }
};
}  // namespace

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kEightClusters;
  int n = 2000, coord = 0;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--family=", 0) == 0) { if (!parse_cloud_family(a.c_str() + 9, &family)) return 2; }
    else if (a.rfind("--n=", 0) == 0) n = std::atoi(a.c_str() + 4);
    else if (a.rfind("--coord=", 0) == 0) coord = std::atoi(a.c_str() + 8);
    else return 2;
  }
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const CloudIndex ix = build_cloud_index(make_family_input(family, n, coord, 3));
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  const u64 smax = 11;
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax), lane_h(Lane::kQ3, smax), lane_h(Lane::kQ4, smax)};
  const u64 h4 = h_of[2];
  const bool float_on = float_filter_runtime_enabled();
  std::vector<AliveRect> alive;
  u64 visited = 0, workers = 0;
  generate_detail::alive_rectangles(ix, 8, h_of, 2, 1, &alive, &visited, &workers);
  generate_detail::AnchorScratch sc;
  u64 anchors_alive = 0, seeds = 0, seeds_alive_k1 = 0, killed_k[4] = {0, 0, 0, 0}, compl_alive = 0, compl_avoided_k[4] = {0, 0, 0, 0};
  u64 wrong = 0, emitted_total = 0;
  const int Ks[4] = {1, 2, 4, 8};
  std::vector<i128> P, B;
  for (const AliveRect& ar : alive) {
    generate_detail::corner_histograms(ix, Lane::kQ4, ar.r, &sc.ha, &sc.hb);
    const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
    rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes);
    const u64 need = h_of[2] - ar.core;
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        if (sc.ha[(size_t)(ua - ra.first)] + sc.hb[(size_t)(ub - rb.first)] >= need) continue;
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        anchor_cover_from_handles(ix, sc.handles, pa, pb, D2, 3, &sc.cover, &sc.visits, &sc.cover_tmp);
        // Tests d'ancre de production (W_4 puis secteurs) : on ne sonde que les ancres vivantes.
        {
          u64 n4 = 0;
          for (const CoverPoint& cz : sc.cover) {
            if (cz.u == ua || cz.u == ub) continue;
            if (in_spindle(Lane::kQ4, pa, pb, ix.upos[(size_t)cz.u]) && ++n4 >= h4) break;
          }
          if (n4 >= h4) continue;
          u64 wmin = 0;
          if (anchor_sector_kill(sc.cover, ix.upos, ua, ub, pa, pb, D2, 8, h4, &wmin)) continue;
        }
        ++anchors_alive;
        // Emissions de production pour cette ancre (pretests deja appliques).
        std::vector<BallCandidate> lo;
        GenerateStats ls;
        sc.affine_filled = false;
        generate_detail::process_anchor_q4(ix, sc, ua, ub, pa, pb, D2, h4, float_on, false, false, false, &lo, &ls,
                                           generate_detail::AnchorPretests::kAlreadyApplied);
        std::set<BallKey, BallKeyLess> emitted;
        for (const BallCandidate& c : lo) emitted.insert(c.key);
        emitted_total += lo.size();
        // Lentille et seeds (comme la production).
        std::vector<i32> lens;
        for (const CoverPoint& cz : sc.cover) {
          const P3& pz = ix.upos[(size_t)cz.u];
          if (p3_norm2(p3_sub(pz, pa)) <= D2 && p3_norm2(p3_sub(pz, pb)) <= D2) lens.push_back(cz.u);
        }
        for (const i32 ux : lens) {
          if (ux == ua || ux == ub) continue;
          const P3& px = ix.upos[(size_t)ux];
          if (!is_acute_seed(pa, pb, px, D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(ux))) continue;
          ++seeds;
          const i64 l_ax = p3_norm2(p3_sub(px, pa)), l_bx = p3_norm2(p3_sub(px, pb));
          const Q3Form f3 = q3_form(pa, pb, px);
          const P3 nrm = p3_cross(p3_sub(pb, pa), p3_sub(px, pa));
          const i128 J = (i128)D2 * (3 * f3.g - 2 * (i128)l_ax * l_bx);
          if (J < 0) return 5;  // inatteignable par theoreme
          // P et B exacts par site (hors a, b, x).
          P.clear(); B.clear();
          for (const CoverPoint& cz : sc.cover) {
            if (cz.u == ua || cz.u == ub || cz.u == ux) continue;
            const P3& pz = ix.upos[(size_t)cz.u];
            P.push_back(q3_power(f3, pz));  // = L/4
            B.push_back((i128)p3_dot(nrm, p3_sub(pz, pa)));
          }
          // K = 1 : cœur de production (exact) — z universel sur toute la corde ssi P < 0 et 2P² > J B².
          u64 core = 0;
          for (size_t i = 0; i < P.size(); ++i)
            if (P[i] < 0 && cmp_2p2_jb2(P[i], J, (i64)B[i]) > 0) ++core;
          const bool dead1 = core >= h4;
          if (!dead1) ++seeds_alive_k1;
          // Morceaux : bornes entieres sur-approximantes ; on scale par 8 pour des bornes en huitiemes de μ̂.
          const i128 mu_hat = isqrt_floor128(J / 2) + 1;  // >= sqrt(J/2)
          u64 dead_k[4] = {dead1 ? 1u : 0u, 0, 0, 0};
          for (int ki = 1; ki < 4; ++ki) {
            const int K = Ks[ki];
            // morceau i : μ in [μ̂(2i/K − 1), μ̂(2(i+1)/K − 1)] ; on teste P·K − (2i − K) μ̂ B < 0 aux deux bornes (scale K).
            bool all = true;
            for (int i = 0; i < K && all; ++i) {
              u64 cnt = 0;
              for (size_t s = 0; s < P.size(); ++s) {
                const i128 lo_b = (i128)K * P[s] - (i128)(2 * i - K) * mu_hat * B[s];
                const i128 hi_b = (i128)K * P[s] - (i128)(2 * (i + 1) - K) * mu_hat * B[s];
                if (lo_b < 0 && hi_b < 0 && ++cnt >= h4) break;
              }
              if (cnt < h4) all = false;
            }
            dead_k[ki] = all ? 1u : 0u;
          }
          // Completions de ce seed (y de la lentille, y != x, a, b) : travail evite si mort par morceaux.
          u64 ncompl = 0;
          for (const i32 uy : lens) if (uy != ux && uy != ua && uy != ub) ++ncompl;
          if (!dead1) compl_alive += ncompl;
          for (int ki = 1; ki < 4; ++ki) {
            if (dead_k[ki] && !dead1) { ++killed_k[ki]; compl_avoided_k[ki] += ncompl; }
            if (dead_k[ki]) {
              // Faux positif ? aucune completion (x, y) de ce seed ne doit avoir ete emise.
              for (const i32 uy : lens) {
                if (uy == ux || uy == ua || uy == ub) continue;
                const Q4Form f4 = q4_form(pa, pb, px, ix.upos[(size_t)uy]);
                if (f4.det == 0) continue;
                if (emitted.count(ball_key_reduce(q4_ball_form(f4)))) { ++wrong; break; }
              }
            }
          }
        }
      }
  }
  std::printf("q4_chord_probe famille=%s n=%d ancres_vivantes=%llu seeds=%llu vivants_apres_cœur=%llu (%.1f %%) completions_vivantes=%llu emis=%llu\n",
              cloud_family_name(family), n, (unsigned long long)anchors_alive, (unsigned long long)seeds, (unsigned long long)seeds_alive_k1,
              seeds ? 100.0 * (double)seeds_alive_k1 / (double)seeds : 0.0, (unsigned long long)compl_alive, (unsigned long long)emitted_total);
  for (int ki = 1; ki < 4; ++ki)
    std::printf("  K=%d : seeds vivants tues %llu (%.1f %% des vivants), completions evitees %llu (%.1f %% des completions vivantes)\n", Ks[ki],
                (unsigned long long)killed_k[ki], seeds_alive_k1 ? 100.0 * (double)killed_k[ki] / (double)seeds_alive_k1 : 0.0,
                (unsigned long long)compl_avoided_k[ki], compl_alive ? 100.0 * (double)compl_avoided_k[ki] / (double)compl_alive : 0.0);
  std::printf("  FAUX POSITIFS (seed tue par morceaux avec une boule emise) = %llu\n", (unsigned long long)wrong);
  return wrong ? 1 : 0;
}
