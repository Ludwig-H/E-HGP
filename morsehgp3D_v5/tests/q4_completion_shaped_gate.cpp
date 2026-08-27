// MorseHGP3D v5 — porte des completions q4 « en forme de kernel »
// (src/gpu/q4_completion_shaped.hpp) contre la lane q4 de production : pour
// chaque seed VIVANT (cœur de seed de production ET shaped, qui doivent
// concorder) et chaque completion y de la lentille, l'etage atteint doit etre
// le meme (lentille, owner, once, i64, puissance de face, det, centre,
// profondeur, emission) ; sur les emissions, la forme de Cramer (det, N')
// doit etre identique entre Q4Form (i128) et Q4FormD (DI128). Planchers :
// --min-emit, --min-deep, --min-center. Mutant de porte `q4-shaped-once-flip`
// (exact-once inverse) : code 4. Codes : 0, 2, 3, 4.
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/gpu/q4_completion_shaped.hpp"
#include "../src/gpu/q4_core_shaped.hpp"
#include "../src/lanes/edge_cover.hpp"
#include "../src/lanes/q3.hpp"
#include "../src/lanes/q4.hpp"
#include "../src/pipeline/float_filter.hpp"
#include "../src/pipeline/generate.hpp"
#include "../src/spindle/spindle.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  int n = 400, coord = 0;
  u64 min_emit = 1000, min_deep = 100, min_center = 100;
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--family=", 0) == 0) { if (!parse_cloud_family(arg.c_str() + 9, &family)) return 2; }
    else if (arg.rfind("--n=", 0) == 0) n = std::atoi(arg.c_str() + 4);
    else if (arg.rfind("--coord=", 0) == 0) coord = std::atoi(arg.c_str() + 8);
    else if (arg.rfind("--min-emit=", 0) == 0) min_emit = (u64)std::atoll(arg.c_str() + 11);
    else if (arg.rfind("--min-deep=", 0) == 0) min_deep = (u64)std::atoll(arg.c_str() + 11);
    else if (arg.rfind("--min-center=", 0) == 0) min_center = (u64)std::atoll(arg.c_str() + 13);
    else if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool m_once = MHGP5_MUTANT("q4-shaped-once-flip");
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
  std::vector<double> du0, du1, du2, dq;
  std::vector<i64> px_, py_, pz_;
  u64 seeds_alive = 0, core_mism = 0, pairs = 0, mism = 0;
  u64 stages[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  const auto pid = [&](i32 u) { return ix.point_id(u); };
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
        const size_t nc = sc.cover.size();
        px_.resize(nc); py_.resize(nc); pz_.resize(nc);
        u32 skip_a = std::numeric_limits<u32>::max(), skip_b = skip_a;
        for (size_t i = 0; i < nc; ++i) {
          const P3& p = ix.upos[(size_t)sc.cover[i].u];
          px_[i] = p.x; py_[i] = p.y; pz_[i] = p.z;
          if (sc.cover[i].u == ua) skip_a = (u32)i;
          if (sc.cover[i].u == ub) skip_b = (u32)i;
        }
        const AnchorPositionsSoA pos{px_.data(), py_.data(), pz_.data(), (u32)nc};
        for (const CoverPoint& cx : sc.lens) {
          if (cx.u == ua || cx.u == ub) continue;
          const P3& px = ix.upos[(size_t)cx.u];
          if (!is_acute_seed(pa, pb, px, D2, pid(ua), pid(ub), pid(cx.u))) continue;
          const i64 l_ax = p3_norm2(p3_sub(px, pa));
          const i64 l_bx = p3_norm2(p3_sub(px, pb));
          const Q3Form f3s = q3_form(pa, pb, px);
          const Q3FormD f3d = q3_form_d(pa, pb, px);
          const P3 nrm = p3_cross(p3_sub(pb, pa), p3_sub(px, pa));
          const i128 Jb = (i128)D2 * (3 * f3s.g - 2 * (i128)l_ax * l_bx);
          bool pdead = Jb < 0;
          if (!pdead) {
            if (!sc.affine_filled) {
              sc.fill_affine_sites(ix, pa, pb, D2);
              du0.resize(nc); du1.resize(nc); du2.resize(nc); dq.resize(nc);
              for (size_t i = 0; i < nc; ++i) {
                du0[i] = (double)sc.su0[i]; du1[i] = (double)sc.su1[i]; du2[i] = (double)sc.su2[i]; dq[i] = (double)sc.sq[i];
              }
            }
            const generate_detail::AffineSeed seed(f3s, pa, pb, sc, float_on);
            const double Jd = (double)Jb;
            const double Jlo = Jd * (1.0 - kJungGuard), Jhi = Jd * (1.0 + kJungGuard);
            u64 fcount = 0;
            for (size_t iz = 0; iz < nc && !pdead; ++iz) {
              const CoverPoint& cz = sc.cover[iz];
              if (cz.u == ua || cz.u == ub || cz.u == cx.u) continue;
              const double lh = seed.l_hat(sc, iz);
              if (lh > seed.bound) continue;
              const P3& pz = ix.upos[(size_t)cz.u];
              const i64 Bz = p3_dot(nrm, p3_sub(pz, f3s.a));
              if (lh < -seed.bound) {
                const int js = jung_interval_sign(lh, seed.bound, Jlo, Jhi, Bz);
                if (js != 0) { if (js > 0 && ++fcount >= h_of[2]) pdead = true; continue; }
                const i128 Pz = seed.l_exact(sc, iz) / 4;
                if (cmp_2p2_jb2(Pz, Jb, Bz) > 0 && ++fcount >= h_of[2]) pdead = true;
                continue;
              }
              const i128 Pz = seed.l_exact(sc, iz) / 4;
              if (Pz >= 0) continue;
              if (cmp_2p2_jb2(Pz, Jb, Bz) > 0 && ++fcount >= h_of[2]) pdead = true;
            }
            // Cœur shaped : doit concorder (deja prouve par q4_core_shaped_gate).
            SeedQ4D sd;
            sd.aff.G = di_from_i128(seed.G);
            sd.aff.N0 = di_from_i128(seed.N0); sd.aff.N1 = di_from_i128(seed.N1); sd.aff.N2 = di_from_i128(seed.N2);
            sd.aff.Gd = seed.Gd; sd.aff.Nd0 = seed.Nd0; sd.aff.Nd1 = seed.Nd1; sd.aff.Nd2 = seed.Nd2; sd.aff.bound = seed.bound;
            sd.n0 = nrm.x; sd.n1 = nrm.y; sd.n2 = nrm.z;
            sd.J = di_from_i128(Jb); sd.Jlo = Jlo; sd.Jhi = Jhi;
            sd.skip_x = std::numeric_limits<u32>::max();
            for (size_t i = 0; i < nc; ++i)
              if (sc.cover[i].u == cx.u) sd.skip_x = (u32)i;
            const AnchorSitesSoA sites{sc.su0.data(), sc.su1.data(), sc.su2.data(), sc.sq.data(),
                                       du0.data(), du1.data(), du2.data(), dq.data(), (u32)nc};
            Q4CoreCounters hc;
            if (q4_seed_core_shaped(sd, sites, skip_a, skip_b, (u32)h_of[2], false, &hc) != pdead) ++core_mism;
          }
          if (pdead) continue;
          ++seeds_alive;
          for (const CoverPoint& cy : sc.lens) {
            const i32 uy = cy.u;
            if (uy == cx.u || uy == ua || uy == ub) continue;
            ++pairs;
            const P3& py = ix.upos[(size_t)uy];
            // (a) production.
            Q4Stage ps = Q4Stage::kEmit;
            Q4Form f4p;
            bool have_p = false;
            const i64 l_ay = p3_norm2(p3_sub(py, pa));
            const i64 l_by = p3_norm2(p3_sub(py, pb));
            const i64 l_xy = p3_norm2(p3_sub(py, px));
            if (l_ay > D2 || l_by > D2 || l_xy > D2) ps = Q4Stage::kRejLens;
            else if (!tetra_owned_by(D2, l_ax, l_ay, l_bx, l_by, l_xy, pid(ua), pid(ub), pid(cx.u), pid(uy))) ps = Q4Stage::kRejOwner;
            else {
              const P3 vy{2 * py.x - pa.x - pb.x, 2 * py.y - pa.y - pb.y, 2 * py.z - pa.z - pb.z};
              if (p3_norm2(vy) > D2 && pid(uy) < pid(cx.u)) ps = Q4Stage::kRejOnce;
              else if (!q4_i64_prefilter(D2, l_ax, l_bx, l_ay, l_by, l_xy)) ps = Q4Stage::kRejI64;
              else if (!q4_face_power_prefilter(f3s, py)) ps = Q4Stage::kRejFacePower;
              else {
                f4p = q4_form(pa, pb, px, py);
                have_p = true;
                if (f4p.det == 0) ps = Q4Stage::kRejDet;
                else if (!q4_center_strictly_inside(f4p, pa, pb, px, py)) ps = Q4Stage::kRejCenter;
                else {
                  u64 depth = 0;
                  bool deep = false;
                  for (const CoverPoint& cz : sc.cover)
                    if (q4_power(f4p, ix.upos[(size_t)cz.u]) < 0 && ++depth >= h_of[2]) { deep = true; break; }
                  ps = deep ? Q4Stage::kDeep : Q4Stage::kEmit;
                }
              }
            }
            // (b) shaped.
            Q4FormD f4d{};
            Q4Stage hs = q4_completion_stage_shaped(pa, pb, px, py, pid(ua), pid(ub), pid(cx.u), pid(uy), D2, l_ax, l_bx,
                                                    f3d, false, m_once, &f4d);
            if (hs == Q4Stage::kEmit && q4_depth_shaped(f4d, pos, (u32)h_of[2], false)) hs = Q4Stage::kDeep;
            bool ok = hs == ps;
            if (ok && have_p && (ps == Q4Stage::kEmit || ps == Q4Stage::kDeep || ps == Q4Stage::kRejCenter))
              ok = di_to_i128(f4d.det) == f4p.det && di_to_i128(f4d.np[0]) == f4p.np[0] &&
                   di_to_i128(f4d.np[1]) == f4p.np[1] && di_to_i128(f4d.np[2]) == f4p.np[2];
            if (!ok) ++mism;
            ++stages[(int)ps];
          }
        }
      }
  }
  std::printf("q4_completion_shaped famille=%s n=%d seeds_vivants=%llu paires=%llu lens=%llu owner=%llu once=%llu i64=%llu "
              "face=%llu det=%llu centre=%llu profond=%llu emis=%llu desaccords_coeur=%llu desaccords=%llu\n",
              cloud_family_name(family), n, (unsigned long long)seeds_alive, (unsigned long long)pairs,
              (unsigned long long)stages[0], (unsigned long long)stages[1], (unsigned long long)stages[2],
              (unsigned long long)stages[3], (unsigned long long)stages[4], (unsigned long long)stages[5],
              (unsigned long long)stages[6], (unsigned long long)stages[7], (unsigned long long)stages[8],
              (unsigned long long)core_mism, (unsigned long long)mism);
  if (stages[8] < min_emit || stages[7] < min_deep || stages[6] < min_center) {
    std::printf("PLANCHER\n");
    return 3;
  }
  if (core_mism) return 1;
  if (mism) return m_once ? 4 : 1;
  if (m_once) { std::printf("MUTANT NON TUE\n"); return 1; }
  std::printf("q4_completion_shaped OK\n");
  return 0;
}
