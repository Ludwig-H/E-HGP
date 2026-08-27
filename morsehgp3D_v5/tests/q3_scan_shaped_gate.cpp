// MorseHGP3D v5 — porte du scan q3 « en forme de kernel » (src/gpu/q3_scan_shaped.hpp)
// contre la logique de la lane q3 de production (generate.hpp) : pour chaque
// rectangle vivant de la lane q3 et chaque ancre survivante, les sites affines
// et les seeds aigus sont formes comme en production, puis (a) le scan de
// production (kernel affine i128, filtre flottant) et (b) le scan shaped
// (DI128, memes doubles) doivent rendre le MEME verdict mort/vivant par seed,
// les MEMES compteurs de certification (cert_neg, cert_pos, replis) et le
// MEME multiensemble de candidats q3 emis. Planchers : --min-seeds,
// --min-fallback (le repli exact doit etre exerce). Mutant DE PORTE
// `q3-shaped-strict-flip` : le scan shaped compte L == 0 comme interieur —
// tue par la porte (code 4) si des sites a L = 0 existent (planchers).
// Codes : 0, 2, 3, 4.
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/gpu/q3_scan_shaped.hpp"
#include "../src/lanes/edge_cover.hpp"
#include "../src/lanes/q3.hpp"
#include "../src/pipeline/float_filter.hpp"
#include "../src/pipeline/generate.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  int n = 400, coord = 0;
  u64 min_seeds = 1000, min_fallback = 10;
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--family=", 0) == 0) { if (!parse_cloud_family(arg.c_str() + 9, &family)) return 2; }
    else if (arg.rfind("--n=", 0) == 0) n = std::atoi(arg.c_str() + 4);
    else if (arg.rfind("--coord=", 0) == 0) coord = std::atoi(arg.c_str() + 8);
    else if (arg.rfind("--min-seeds=", 0) == 0) min_seeds = (u64)std::atoll(arg.c_str() + 12);
    else if (arg.rfind("--min-fallback=", 0) == 0) min_fallback = (u64)std::atoll(arg.c_str() + 15);
    else if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool m_flip = MHGP5_MUTANT("q3-shaped-strict-flip");
  const CloudIndex ix = build_cloud_index(make_family_input(family, n, coord, 3));
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  const u64 smax = 11;
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax), lane_h(Lane::kQ3, smax), lane_h(Lane::kQ4, smax)};
  const bool float_on = float_filter_runtime_enabled();
  std::vector<AliveRect> alive;
  u64 visited = 0, workers = 0;
  generate_detail::alive_rectangles(ix, 8, h_of, 1, 1, &alive, &visited, &workers);
  u64 seeds = 0, mismatches = 0, fallback_total = 0, zero_sites = 0, dead_prod = 0, dead_shaped = 0;
  std::vector<u64> ha, hb;
  std::vector<NodeRef> handles;
  std::vector<CoverPoint> cover, tmp;
  u64 cnodes = 0, visits = 0;
  std::vector<i64> su0, su1, su2, sq;
  std::vector<double> du0, du1, du2, dq;
  std::vector<BallCandidate> prod, shaped;
  for (const AliveRect& ar : alive) {
    generate_detail::corner_histograms(ix, Lane::kQ3, ar.r, &ha, &hb);
    const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
    rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &handles, &cnodes);
    const u64 need = h_of[1] - ar.core;
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        if (ha[(size_t)(ua - ra.first)] + hb[(size_t)(ub - rb.first)] >= need) continue;
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        anchor_cover_from_handles(ix, handles, pa, pb, D2, 3, &cover, &visits, &tmp);
        // Sites affines (comme AnchorScratch::fill_affine_sites).
        const size_t nc = cover.size();
        su0.resize(nc); su1.resize(nc); su2.resize(nc); sq.resize(nc);
        du0.resize(nc); du1.resize(nc); du2.resize(nc); dq.resize(nc);
        i64 qmax = 1, umax = 1;
        const i64 sx = pa.x + pb.x, sy = pa.y + pb.y, sz = pa.z + pb.z;
        for (size_t i = 0; i < nc; ++i) {
          const P3& pz = ix.upos[(size_t)cover[i].u];
          const i64 u0 = 2 * pz.x - sx, u1 = 2 * pz.y - sy, u2 = 2 * pz.z - sz;
          const i64 qz = u0 * u0 + u1 * u1 + u2 * u2 - D2;
          su0[i] = u0; su1[i] = u1; su2[i] = u2; sq[i] = qz;
          du0[i] = (double)u0; du1[i] = (double)u1; du2[i] = (double)u2; dq[i] = (double)qz;
          qmax = std::max(qmax, qz < 0 ? -qz : qz);
          umax = std::max({umax, u0 < 0 ? -u0 : u0, u1 < 0 ? -u1 : u1, u2 < 0 ? -u2 : u2});
        }
        const AnchorSitesSoA sites{su0.data(), su1.data(), su2.data(), sq.data(), du0.data(), du1.data(), du2.data(), dq.data(), (u32)nc};
        for (size_t ix_c = 0; ix_c < nc; ++ix_c) {
          const CoverPoint& cp = cover[ix_c];
          if (cp.u == ua || cp.u == ub) continue;
          const P3& px = ix.upos[(size_t)cp.u];
          if (!is_acute_seed(pa, pb, px, D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(cp.u))) continue;
          ++seeds;
          const Q3Form f3 = q3_form(pa, pb, px);
          // (a) production : kernel affine i128 (copie de la lane q3).
          const i128 N0 = f3.w[0] - f3.g * (i128)(pb.x - pa.x);
          const i128 N1 = f3.w[1] - f3.g * (i128)(pb.y - pa.y);
          const i128 N2 = f3.w[2] - f3.g * (i128)(pb.z - pa.z);
          const double Gd = (double)f3.g, Nd0 = (double)N0, Nd1 = (double)N1, Nd2 = (double)N2;
          const double bound = float_on ? affine_l_bound(Gd, Nd0, Nd1, Nd2, (double)qmax, (double)umax) : std::numeric_limits<double>::infinity();
          u32 pn = 0, pp = 0, pf = 0;
          u64 depth = 0;
          bool dead_p = false;
          for (size_t iz = 0; iz < nc; ++iz) {
            // La lane de production ne saute PAS le carrier : son L vaut 0 (sur la sphere) et n'est jamais < 0.
            const double lh = affine_l_hat(Gd, Nd0, Nd1, Nd2, du0[iz], du1[iz], du2[iz], dq[iz]);
            bool interior;
            if (lh < -bound) { ++pn; interior = true; }
            else if (lh > bound) { ++pp; interior = false; }
            else {
              ++pf;
              const i128 L = f3.g * (i128)sq[iz] - 2 * ((i128)su0[iz] * N0 + (i128)su1[iz] * N1 + (i128)su2[iz] * N2);
              if (L == 0) ++zero_sites;
              interior = L < 0;
            }
            if (interior && ++depth >= h_of[1]) { dead_p = true; break; }
          }
          // (b) shaped : DI128 ; skip = aucun (meme comportement que la production : le carrier rend L = 0).
          SeedAffineD sd;
          sd.G = di_from_i128(f3.g);
          sd.N0 = di_from_i128(N0); sd.N1 = di_from_i128(N1); sd.N2 = di_from_i128(N2);
          sd.Gd = Gd; sd.Nd0 = Nd0; sd.Nd1 = Nd1; sd.Nd2 = Nd2; sd.bound = bound;
          u32 sn = 0, sp = 0, sf = 0;
          const bool dead_s = q3_scan_seed_shaped(sd, sites, (u32)h_of[1], std::numeric_limits<u32>::max(), &sn, &sp, &sf, m_flip);
          fallback_total += pf;
          if (dead_p) ++dead_prod;
          if (dead_s) ++dead_shaped;
          if (dead_p != dead_s || (!dead_p && (pn != sn || pp != sp || pf != sf))) ++mismatches;
          if (!dead_p) prod.push_back(BallCandidate{q3_ball_key(f3), promote_level(q3_exact_level(pa, pb, px)), 3});
          if (!dead_s) shaped.push_back(BallCandidate{q3_ball_key(f3), promote_level(q3_exact_level(pa, pb, px)), 3});
        }
      }
  }
  rle_candidates(&prod);
  rle_candidates(&shaped);
  bool same = prod.size() == shaped.size();
  for (size_t i = 0; same && i < prod.size(); ++i) same = prod[i].key == shaped[i].key && prod[i].level == shaped[i].level;
  std::printf("q3_scan_shaped famille=%s n=%d rectangles=%zu seeds=%llu morts_prod=%llu morts_shaped=%llu replis=%llu sites_L0=%llu "
              "candidats=%zu/%zu desaccords=%llu\n",
              cloud_family_name(family), n, alive.size(), (unsigned long long)seeds, (unsigned long long)dead_prod,
              (unsigned long long)dead_shaped, (unsigned long long)fallback_total, (unsigned long long)zero_sites, prod.size(),
              shaped.size(), (unsigned long long)mismatches);
  const bool killed = mismatches != 0 || !same;
  if (!inject.empty()) {
    if (killed) { std::fprintf(stderr, "MUTANT TUE : %s\n", inject.c_str()); return 4; }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s survivant\n", inject.c_str());
    return 3;
  }
  if (killed) return 1;
  if (seeds < min_seeds || fallback_total < min_fallback) { std::fprintf(stderr, "PLANCHER : seeds=%llu replis=%llu\n", (unsigned long long)seeds, (unsigned long long)fallback_total); return 3; }
  std::printf("q3_scan_shaped_gate OK\n");
  return 0;
}
