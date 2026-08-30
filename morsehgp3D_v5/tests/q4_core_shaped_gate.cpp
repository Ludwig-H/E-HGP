// MorseHGP3D v5 — porte du cœur de seed q4 « en forme de kernel »
// (src/gpu/q4_core_shaped.hpp) contre la logique de la lane q4 de production
// (generate.hpp) : pour chaque rectangle vivant de la lane q4, chaque ancre
// survivante (histogramme, W_4) et chaque seed aigu de la lentille avec J >= 0,
// (a) le cœur de seed de production (kernel affine i128, intervalle de Jung,
// replis exacts) et (b) le cœur shaped (DI128, memes doubles) doivent rendre
// le MEME verdict mort/vivant et les MEMES six compteurs. Planchers :
// --min-seeds, --min-jung-fallback, --min-float-fallback, --min-jung-skip.
// Mutant de porte `q4-shaped-jung-skip-kills` (un non-temoin certifie compte
// comme temoin) : code 4. Codes : 0, 2, 3, 4.
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/gpu/q4_core_shaped.hpp"
#include "../src/lanes/edge_cover.hpp"
#include "../src/lanes/q3.hpp"
#include "../src/lanes/q4.hpp"
#include "../src/spindle/spindle.hpp"
#include "../src/pipeline/float_filter.hpp"
#include "../src/pipeline/generate.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  int n = 400, coord = 0;
  u64 min_seeds = 1000, min_jf = 10, min_ff = 10, min_js = 10, min_chord = 0;
  // --ordre-corde : le VERDICT d'un seed ne doit pas dependre de l'ordre des
  // sites. Le scan est rejoue corde active dans l'ordre naturel puis dans
  // l'ordre INVERSE ; un desaccord signe une couture d'ordre (le `continue`
  // d'un site certifie P > 0 qui saute la constatation de mort par corde).
  bool ordre_corde = false;
  u64 chord_dead = 0, ordre_mism = 0;
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--family=", 0) == 0) { if (!parse_cloud_family(arg.c_str() + 9, &family)) return 2; }
    else if (arg.rfind("--n=", 0) == 0) n = std::atoi(arg.c_str() + 4);
    else if (arg.rfind("--coord=", 0) == 0) coord = std::atoi(arg.c_str() + 8);
    else if (arg.rfind("--min-seeds=", 0) == 0) min_seeds = (u64)std::atoll(arg.c_str() + 12);
    else if (arg.rfind("--min-jung-fallback=", 0) == 0) min_jf = (u64)std::atoll(arg.c_str() + 20);
    else if (arg.rfind("--min-float-fallback=", 0) == 0) min_ff = (u64)std::atoll(arg.c_str() + 21);
    else if (arg.rfind("--min-jung-skip=", 0) == 0) min_js = (u64)std::atoll(arg.c_str() + 16);
    else if (arg.rfind("--min-corde=", 0) == 0) min_chord = (u64)std::atoll(arg.c_str() + 12);
    else if (arg == "--ordre-corde") ordre_corde = true;
    else if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool m_skip_kills = MHGP5_MUTANT("q4-shaped-jung-skip-kills");
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
  u64 seeds = 0, mism = 0, dead_n = 0;
  Q4CoreCounters tot;
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
        {
          u64 n4 = 0;
          for (const CoverPoint& cz : sc.cover) {
            if (cz.u == ua || cz.u == ub) continue;
            if (in_spindle(Lane::kQ4, pa, pb, ix.upos[(size_t)cz.u]) && ++n4 >= h_of[2]) break;
          }
          if (n4 >= h_of[2]) continue;
        }
        sc.affine_filled = false;
        sc.lens.clear();
        for (const CoverPoint& cz : sc.cover) {
          const P3& pz = ix.upos[(size_t)cz.u];
          if (p3_norm2(p3_sub(pz, pa)) <= D2 && p3_norm2(p3_sub(pz, pb)) <= D2) sc.lens.push_back(cz);
        }
        u32 skip_a = std::numeric_limits<u32>::max(), skip_b = skip_a;
        for (size_t i = 0; i < sc.cover.size(); ++i) {
          if (sc.cover[i].u == ua) skip_a = (u32)i;
          if (sc.cover[i].u == ub) skip_b = (u32)i;
        }
        for (const CoverPoint& cx : sc.lens) {
          if (cx.u == ua || cx.u == ub) continue;
          const P3& px = ix.upos[(size_t)cx.u];
          if (!is_acute_seed(pa, pb, px, D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(cx.u))) continue;
          const i64 l_ax = p3_norm2(p3_sub(px, pa));
          const i64 l_bx = p3_norm2(p3_sub(px, pb));
          const Q3Form f3s = q3_form(pa, pb, px);
          const P3 nrm = p3_cross(p3_sub(pb, pa), p3_sub(px, pa));
          const i128 Jb = (i128)D2 * (3 * f3s.g - 2 * (i128)l_ax * l_bx);
          if (Jb < 0) continue;
          ++seeds;
          if (!sc.affine_filled) sc.fill_affine_sites(ix, pa, pb, D2);
          // (a) production.
          const generate_detail::AffineSeed seed(f3s, pa, pb, sc, float_on);
          const double Jd = (double)Jb;
          const double Jlo = Jd * (1.0 - kJungGuard), Jhi = Jd * (1.0 + kJungGuard);
          Q4CoreCounters pc;
          u64 fcount = 0;
          bool pdead = false;
          for (size_t iz = 0; iz < sc.cover.size(); ++iz) {
            const CoverPoint& cz = sc.cover[iz];
            if (cz.u == ua || cz.u == ub || cz.u == cx.u) continue;
            const double lh = seed.l_hat(sc, iz);
            if (lh > seed.bound) { ++pc.cert_pos; continue; }
            if (lh < -seed.bound) {
              ++pc.cert_neg;
              const P3& pz = ix.upos[(size_t)cz.u];
              const i64 Bz = p3_dot(nrm, p3_sub(pz, f3s.a));
              const int js = jung_interval_sign(lh, seed.bound, Jlo, Jhi, Bz);
              if (js != 0) {
                if (js > 0) { ++pc.jung_kill; if (++fcount >= h_of[2]) { pdead = true; break; } }
                else ++pc.jung_skip;
                continue;
              }
              ++pc.jung_fallback;
              const i128 Pz = seed.l_exact(sc, iz) / 4;
              if (cmp_2p2_jb2(Pz, Jb, Bz) > 0 && ++fcount >= h_of[2]) { pdead = true; break; }
              continue;
            }
            ++pc.float_fallback;
            const i128 Pz = seed.l_exact(sc, iz) / 4;
            if (Pz >= 0) continue;
            const P3& pz = ix.upos[(size_t)cz.u];
            const i64 Bz = p3_dot(nrm, p3_sub(pz, f3s.a));
            if (cmp_2p2_jb2(Pz, Jb, Bz) > 0 && ++fcount >= h_of[2]) { pdead = true; break; }
          }
          // (b) shaped.
          SeedQ4D sd;
          sd.aff.G = di_from_i128(seed.G);
          sd.aff.N0 = di_from_i128(seed.N0); sd.aff.N1 = di_from_i128(seed.N1); sd.aff.N2 = di_from_i128(seed.N2);
          sd.aff.Gd = seed.Gd; sd.aff.Nd0 = seed.Nd0; sd.aff.Nd1 = seed.Nd1; sd.aff.Nd2 = seed.Nd2; sd.aff.bound = seed.bound;
          sd.n0 = nrm.x; sd.n1 = nrm.y; sd.n2 = nrm.z;
          sd.J = di_from_i128(Jb);
          sd.Jlo = Jlo; sd.Jhi = Jhi;
          sd.skip_x = std::numeric_limits<u32>::max();
          for (size_t i = 0; i < sc.cover.size(); ++i)
            if (sc.cover[i].u == cx.u) sd.skip_x = (u32)i;
          const AnchorSitesSoA sites{sc.su0.data(), sc.su1.data(), sc.su2.data(), sc.sq.data(), (u32)sc.cover.size()};
          if (ordre_corde) {
            const u32 nn = (u32)sc.cover.size();
            Q4CoreCounters c1, c2;
            const bool d1 = q4_seed_core_shaped(sd, sites, skip_a, skip_b, (u32)h_of[2], false, &c1, false, true);
            std::vector<i64> r0(nn), r1(nn), r2(nn);
            std::vector<i64> rq(nn);
            for (u32 i = 0; i < nn; ++i) {
              r0[i] = sc.su0[nn - 1 - i]; r1[i] = sc.su1[nn - 1 - i];
              r2[i] = sc.su2[nn - 1 - i]; rq[i] = sc.sq[nn - 1 - i];
            }
            const auto rev = [&](u32 j) { return j == std::numeric_limits<u32>::max() ? j : nn - 1 - j; };
            SeedQ4D sdr = sd;
            sdr.skip_x = rev(sd.skip_x);
            const AnchorSitesSoA sr{r0.data(), r1.data(), r2.data(), rq.data(), nn};
            const bool d2 = q4_seed_core_shaped(sdr, sr, rev(skip_a), rev(skip_b), (u32)h_of[2], false, &c2, false, true);
            chord_dead += (c1.dead_by_chord != 0) + (c2.dead_by_chord != 0);
            if (d1 != d2) ++ordre_mism;
          }
          Q4CoreCounters hc;
          bool hdead = q4_seed_core_shaped(sd, sites, skip_a, skip_b, (u32)h_of[2], false, &hc, false, /*chord_on=*/false);
          if (m_skip_kills && hc.jung_skip > 0) {  // MUTANT de porte : les non-temoins certifies tuent
            hc.jung_kill += hc.jung_skip;
            hc.jung_skip = 0;
            hdead = hdead || hc.jung_kill >= h_of[2];
          }
          if (hdead != pdead || hc.cert_pos != pc.cert_pos || hc.cert_neg != pc.cert_neg || hc.jung_kill != pc.jung_kill ||
              hc.jung_skip != pc.jung_skip || hc.jung_fallback != pc.jung_fallback || hc.float_fallback != pc.float_fallback)
            ++mism;
          dead_n += pdead;
          tot.cert_pos += pc.cert_pos; tot.cert_neg += pc.cert_neg; tot.jung_kill += pc.jung_kill;
          tot.jung_skip += pc.jung_skip; tot.jung_fallback += pc.jung_fallback; tot.float_fallback += pc.float_fallback;
        }
      }
  }
  std::printf("q4_core_shaped famille=%s n=%d rectangles=%zu seeds=%llu morts=%llu cert_pos=%u cert_neg=%u jung_kill=%u "
              "jung_skip=%u jung_fallback=%u float_fallback=%u desaccords=%llu\n",
              cloud_family_name(family), n, alive.size(), (unsigned long long)seeds, (unsigned long long)dead_n, tot.cert_pos,
              tot.cert_neg, tot.jung_kill, tot.jung_skip, tot.jung_fallback, tot.float_fallback, (unsigned long long)mism);
  if (ordre_corde)
    std::printf("ordre_corde morts_corde=%llu desaccords_ordre=%llu\n", (unsigned long long)chord_dead,
                (unsigned long long)ordre_mism);
  if (seeds < min_seeds || tot.jung_fallback < min_jf || tot.float_fallback < min_ff || tot.jung_skip < min_js ||
      (ordre_corde && chord_dead < min_chord)) {
    std::printf("PLANCHER\n");
    return 3;
  }
  if (ordre_corde) {
    // Le mutant `chord-dead-skip-positive` rend le verdict DEPENDANT de l'ordre.
    const bool m_dead = MHGP5_MUTANT("chord-dead-skip-positive");
    if (ordre_mism) return m_dead ? 4 : 1;
    if (m_dead) { std::printf("MUTANT NON TUE\n"); return 1; }
  }
  if (mism) return m_skip_kills ? 4 : 1;
  if (m_skip_kills) { std::printf("MUTANT NON TUE\n"); return 1; }
  std::printf("q4_core_shaped OK\n");
  return 0;
}
