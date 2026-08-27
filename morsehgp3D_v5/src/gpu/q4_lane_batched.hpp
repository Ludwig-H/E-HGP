// MorseHGP3D v5 — lane q4 PAR LOTS (docs/GPU.md, livraison 4d, etage hote) :
// le contrat de la future lane q4 device, ecrit et prouve sur CPU, sur le
// modele de q3_lane_batched.hpp :
//   1. build_q4_batch : pour un rectangle vivant, tableaux plats — par ancre
//      survivante (histogramme, W_4) : sites affines SoA, positions, indices
//      de la lentille, indices de a et b ; par seed aigu de la lentille :
//      cœur SeedQ4D (J < 0 = mort d'office), face Q3FormD, l_ax, l_bx ;
//   2. un EXECUTEUR rend, par seed, le verdict du cœur et ses six compteurs,
//      puis pour chaque seed vivant et chaque completion y de la lentille
//      l'etage atteint (Q4Stage) et, si emission, la liste ordonnee des
//      paires (seed, y) — ici scan_q4_batch_host (boucles plates sur
//      q4_seed_core_shaped, q4_completion_stage_shaped, q4_depth_shaped) ;
//   3. emit_q4_batch : l'hote recalcule la forme i128 des paires emises et
//      pousse les candidats (cle, niveau) dans l'ordre de la production.
// tests/q4_lane_batched_gate.cpp : egalite post-RLE des candidats q4 et de
// tous les compteurs de la lane avec generate.hpp. Mutant de porte
// `q4-batched-emit-deep` : les candidats profonds sont emis (code 4).
#pragma once

#include <limits>
#include <vector>

#include "../core/mutants.hpp"
#include "../lanes/edge_cover.hpp"
#include "../lanes/q3.hpp"
#include "../lanes/q4.hpp"
#include "../pipeline/float_filter.hpp"
#include "../pipeline/generate.hpp"
#include "../spindle/spindle.hpp"
#include "q3_lane_batched.hpp"
#include "q4_completion_shaped.hpp"
#include "q4_core_shaped.hpp"

namespace mhgp5 {

struct Q4BatchAnchor {
  u32 begin = 0, count = 0;            // sites
  u32 lens_begin = 0, lens_count = 0;  // indices (dans lens_sites) des sites de la lentille
  u32 skip_a = 0, skip_b = 0;          // indices de a et b dans les sites (UINT32_MAX si absents)
  i64 D2 = 0;
  P3 a, b;
  PointId ida = 0, idb = 0;
};
struct Q4BatchSeed {
  SeedQ4D core;
  Q3FormD face;
  u32 anchor = 0;
  u32 x_site = 0;  // index de x dans les sites
  i64 l_ax = 0, l_bx = 0;
  u8 jneg = 0;     // J < 0 : mort d'office (compte seeds_killed_core)
};
struct Q4SeedVerdict {
  u32 dead = 0;
  Q4CoreCounters c;
};
struct Q4Emit {
  u32 seed = 0, y_site = 0;
};
struct Q4StageCounts {
  u64 completions = 0, lens = 0, owner = 0, once = 0, i64_ = 0, face = 0, det = 0, center = 0, deep = 0, emit = 0;
};

struct Q4Batch {
  std::vector<i64> u0, u1, u2, q;
  std::vector<double> u0d, u1d, u2d, qd;
  std::vector<i64> px, py, pz;
  std::vector<PointId> pid;
  std::vector<u32> lens_sites;
  std::vector<Q4BatchAnchor> anchors;
  std::vector<Q4BatchSeed> seeds;
  std::vector<Q4SeedVerdict> verdicts;  // executeur
  std::vector<Q4Emit> emits;            // executeur, ordre (seed, y)
  Q4StageCounts stages;                 // executeur
  void clear() {
    u0.clear(); u1.clear(); u2.clear(); q.clear();
    u0d.clear(); u1d.clear(); u2d.clear(); qd.clear();
    px.clear(); py.clear(); pz.clear(); pid.clear();
    lens_sites.clear(); anchors.clear(); seeds.clear(); verdicts.clear(); emits.clear();
    stages = Q4StageCounts{};
  }
};

// Etage 1 : formation du lot (aucun verdict). Compte anchors, anchors_killed_hist,
// anchors_killed_w4, seeds comme la lane de production.
inline void build_q4_batch(const CloudIndex& ix, const AliveRect& ar, const u64 h_of[3], bool float_on, i64 cover_coef,
                           generate_detail::AnchorScratch& sc, Q4Batch* b, GenerateStats* ls) {
  using namespace generate_detail;
  corner_histograms(ix, Lane::kQ4, ar.r, &sc.ha, &sc.hb);
  const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
  rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), cover_coef, &sc.handles, &sc.cover_nodes);
  const u64 need = h_of[2] - ar.core;
  for (i32 ua = ra.first; ua <= ra.last; ++ua)
    for (i32 ub = rb.first; ub <= rb.last; ++ub) {
      ++ls->anchors[2];
      if (sc.ha[(size_t)(ua - ra.first)] + sc.hb[(size_t)(ub - rb.first)] >= need) {
        ++ls->anchors_killed_hist[2];
        continue;
      }
      const P3& pa = ix.upos[(size_t)ua];
      const P3& pb = ix.upos[(size_t)ub];
      const i64 D2 = p3_norm2(p3_sub(pb, pa));
      if (D2 == 0) continue;
      anchor_cover_from_handles(ix, sc.handles, pa, pb, D2, cover_coef, &sc.cover, &sc.visits, &sc.cover_tmp);
      {
        u64 n4 = 0;
        for (const CoverPoint& cz : sc.cover) {
          if (cz.u == ua || cz.u == ub) continue;
          if (in_spindle(Lane::kQ4, pa, pb, ix.upos[(size_t)cz.u]) && ++n4 >= h_of[2]) break;
        }
        if (n4 >= h_of[2]) {
          ++ls->anchors_killed_w4;
          continue;
        }
      }
      // Sites (toujours remplis : la lentille et les positions servent aux
      // completions meme sans seed a scanner).
      sc.fill_affine_sites(ix, pa, pb, D2);
      const size_t nc = sc.cover.size();
      Q4BatchAnchor an;
      an.begin = (u32)b->u0.size();
      an.count = (u32)nc;
      an.skip_a = an.skip_b = std::numeric_limits<u32>::max();
      an.D2 = D2; an.a = pa; an.b = pb; an.ida = ix.point_id(ua); an.idb = ix.point_id(ub);
      an.lens_begin = (u32)b->lens_sites.size();
      for (size_t i = 0; i < nc; ++i) {
        const CoverPoint& cz = sc.cover[i];
        const P3& p = ix.upos[(size_t)cz.u];
        b->u0.push_back(sc.su0[i]); b->u1.push_back(sc.su1[i]); b->u2.push_back(sc.su2[i]); b->q.push_back(sc.sq[i]);
        b->u0d.push_back((double)sc.su0[i]); b->u1d.push_back((double)sc.su1[i]);
        b->u2d.push_back((double)sc.su2[i]); b->qd.push_back((double)sc.sq[i]);
        b->px.push_back(p.x); b->py.push_back(p.y); b->pz.push_back(p.z);
        b->pid.push_back(ix.point_id(cz.u));
        if (cz.u == ua) an.skip_a = (u32)i;
        if (cz.u == ub) an.skip_b = (u32)i;
        if (p3_norm2(p3_sub(p, pa)) <= D2 && p3_norm2(p3_sub(p, pb)) <= D2) b->lens_sites.push_back((u32)i);
      }
      an.lens_count = (u32)b->lens_sites.size() - an.lens_begin;
      const u32 aidx = (u32)b->anchors.size();
      b->anchors.push_back(an);
      for (u32 li = an.lens_begin; li < an.lens_begin + an.lens_count; ++li) {
        const u32 xs = b->lens_sites[li];
        const CoverPoint& cx = sc.cover[xs];
        if (cx.u == ua || cx.u == ub) continue;
        const P3& px = ix.upos[(size_t)cx.u];
        if (!is_acute_seed(pa, pb, px, D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(cx.u))) continue;
        ++ls->seeds[1];
        Q4BatchSeed sd;
        sd.anchor = aidx;
        sd.x_site = xs;
        sd.l_ax = p3_norm2(p3_sub(px, pa));
        sd.l_bx = p3_norm2(p3_sub(px, pb));
        const Q3Form f3s = q3_form(pa, pb, px);
        sd.face = q3_form_d(pa, pb, px);
        const P3 nrm = p3_cross(p3_sub(pb, pa), p3_sub(px, pa));
        const i128 Jb = (i128)D2 * (3 * f3s.g - 2 * (i128)sd.l_ax * sd.l_bx);
        sd.jneg = Jb < 0 ? 1 : 0;
        const AffineSeed as(f3s, pa, pb, sc, float_on);
        sd.core.aff.G = di_from_i128(as.G);
        sd.core.aff.N0 = di_from_i128(as.N0); sd.core.aff.N1 = di_from_i128(as.N1); sd.core.aff.N2 = di_from_i128(as.N2);
        sd.core.aff.Gd = as.Gd; sd.core.aff.Nd0 = as.Nd0; sd.core.aff.Nd1 = as.Nd1; sd.core.aff.Nd2 = as.Nd2;
        sd.core.aff.bound = as.bound;
        sd.core.n0 = nrm.x; sd.core.n1 = nrm.y; sd.core.n2 = nrm.z;
        sd.core.J = di_from_i128(Jb < 0 ? (i128)0 : Jb);
        const double Jd = (double)Jb;
        sd.core.Jlo = Jd * (1.0 - kJungGuard);
        sd.core.Jhi = Jd * (1.0 + kJungGuard);
        sd.core.skip_x = xs;
        b->seeds.push_back(sd);
      }
    }
}

// Etage 2 (executeur hote) : cœurs puis completions, boucles plates.
inline void scan_q4_batch_host(Q4Batch* b, u32 h4, bool core_nonstrict, bool depth_nonstrict, bool no_canonical) {
  b->verdicts.assign(b->seeds.size(), Q4SeedVerdict{});
  b->emits.clear();
  b->stages = Q4StageCounts{};
  for (size_t si = 0; si < b->seeds.size(); ++si) {
    const Q4BatchSeed& s = b->seeds[si];
    const Q4BatchAnchor& an = b->anchors[s.anchor];
    Q4SeedVerdict& v = b->verdicts[si];
    if (s.jneg) {
      v.dead = 1;
      continue;
    }
    const AnchorSitesSoA sites{b->u0.data() + an.begin, b->u1.data() + an.begin, b->u2.data() + an.begin,
                               b->q.data() + an.begin,  b->u0d.data() + an.begin, b->u1d.data() + an.begin,
                               b->u2d.data() + an.begin, b->qd.data() + an.begin, an.count};
    v.dead = q4_seed_core_shaped(s.core, sites, an.skip_a, an.skip_b, h4, core_nonstrict, &v.c) ? 1u : 0u;
    if (v.dead) continue;
    const AnchorPositionsSoA pos{b->px.data() + an.begin, b->py.data() + an.begin, b->pz.data() + an.begin, an.count};
    const P3 x{b->px[an.begin + s.x_site], b->py[an.begin + s.x_site], b->pz[an.begin + s.x_site]};
    const PointId idx = b->pid[an.begin + s.x_site];
    for (u32 li = an.lens_begin; li < an.lens_begin + an.lens_count; ++li) {
      const u32 ys = b->lens_sites[li];
      if (ys == s.x_site || ys == an.skip_a || ys == an.skip_b) continue;
      ++b->stages.completions;
      const P3 y{b->px[an.begin + ys], b->py[an.begin + ys], b->pz[an.begin + ys]};
      Q4FormD f4{};
      Q4Stage st = q4_completion_stage_shaped(an.a, an.b, x, y, an.ida, an.idb, idx, b->pid[an.begin + ys], an.D2, s.l_ax,
                                              s.l_bx, s.face, no_canonical, false, &f4);
      if (st == Q4Stage::kEmit && q4_depth_shaped(f4, pos, h4, depth_nonstrict)) st = Q4Stage::kDeep;
      switch (st) {
        case Q4Stage::kRejLens: ++b->stages.lens; break;
        case Q4Stage::kRejOwner: ++b->stages.owner; break;
        case Q4Stage::kRejOnce: ++b->stages.once; break;
        case Q4Stage::kRejI64: ++b->stages.i64_; break;
        case Q4Stage::kRejFacePower: ++b->stages.face; break;
        case Q4Stage::kRejDet: ++b->stages.det; break;
        case Q4Stage::kRejCenter: ++b->stages.center; break;
        case Q4Stage::kDeep:
          ++b->stages.deep;
          if (MHGP5_MUTANT("q4-batched-emit-deep")) b->emits.push_back(Q4Emit{(u32)si, ys});
          break;
        case Q4Stage::kEmit:
          ++b->stages.emit;
          b->emits.push_back(Q4Emit{(u32)si, ys});
          break;
      }
    }
  }
}

// Etage 3 : compteurs et emission ordonnee (forme i128 recalculee par l'hote).
inline void emit_q4_batch(const Q4Batch& b, std::vector<BallCandidate>* lo, GenerateStats* ls) {
  for (const Q4SeedVerdict& v : b.verdicts) {
    if (v.dead) ++ls->seeds_killed_core;
    ls->float_cert_pos += v.c.cert_pos; ls->q4_cert[0] += v.c.cert_pos;
    ls->float_cert_neg += v.c.cert_neg; ls->q4_cert[1] += v.c.cert_neg;
    ls->jung_cert_kill += v.c.jung_kill; ls->q4_cert[2] += v.c.jung_kill;
    ls->jung_cert_skip += v.c.jung_skip; ls->q4_cert[3] += v.c.jung_skip;
    ls->jung_fallback += v.c.jung_fallback; ls->q4_cert[4] += v.c.jung_fallback;
    ls->float_fallback += v.c.float_fallback; ls->q4_cert[5] += v.c.float_fallback;
  }
  ls->q4_completions += b.stages.completions;
  ls->q4_rej_lens += b.stages.lens;
  ls->q4_rej_owner += b.stages.owner;
  ls->q4_rej_once += b.stages.once;
  ls->q4_rej_i64 += b.stages.i64_;
  ls->q4_rej_face_power += b.stages.face;
  ls->q4_rej_det += b.stages.det;
  ls->q4_rej_center += b.stages.center;
  ls->depth_killed[2] += b.stages.deep;
  for (const Q4Emit& e : b.emits) {
    const Q4BatchSeed& s = b.seeds[e.seed];
    const Q4BatchAnchor& an = b.anchors[s.anchor];
    const P3 x{b.px[an.begin + s.x_site], b.py[an.begin + s.x_site], b.pz[an.begin + s.x_site]};
    const P3 y{b.px[an.begin + e.y_site], b.py[an.begin + e.y_site], b.pz[an.begin + e.y_site]};
    const Q4Form f4 = q4_form(an.a, an.b, x, y);
    lo->push_back(BallCandidate{ball_key_reduce(q4_ball_form(f4)), q4_level_raw(f4), 4});
    ++ls->candidates[2];
  }
}

template <class Scan>
inline void generate_q4_batched_with(const CloudIndex& ix, const GenerateOptions& opt, std::vector<BallCandidate>* out,
                                     GenerateStats* st, Scan&& scan, size_t seeds_per_launch = kSeedsPerLaunch) {
  using namespace generate_detail;
  const bool float_on = float_filter_runtime_enabled();
  const bool depth_nonstrict = MHGP5_MUTANT("genfilter-nonstrict");
  const bool core_nonstrict = MHGP5_MUTANT("q4-seed-core-nonstrict");
  const bool no_canonical = MHGP5_MUTANT("q4-no-canonical");
  const i64 cover_coef = MHGP5_MUTANT("q4-cover-coef4") ? 4 : 3;
  const u64 h_of[3] = {lane_h(Lane::kQ2, opt.smax), lane_h(Lane::kQ3, opt.smax), lane_h(Lane::kQ4, opt.smax)};
  std::vector<AliveRect> alive;
  const auto t0 = std::chrono::steady_clock::now();
  alive_rectangles(ix, opt.s, h_of, 2, opt.threads, &alive, &st->rect_visited[2], &st->workers_wspd[2]);
  st->t_wspd_ms[2] += ms_since(t0);
  st->rect_alive[2] = alive.size();
  const auto t1 = std::chrono::steady_clock::now();
  const size_t nrect = alive.size();
  const size_t T = planned_workers(nrect, opt.threads);
  std::vector<std::vector<BallCandidate>> louts(T);
  std::vector<GenerateStats> lst(T);
  std::vector<AnchorScratch> lsc(T);
  std::vector<Q4Batch> lb(T);
  const auto flush = [&](size_t t) {
    if (lb[t].seeds.empty() && lb[t].anchors.empty()) return;
    scan(&lb[t], (u32)h_of[2], core_nonstrict, depth_nonstrict, no_canonical);
    emit_q4_batch(lb[t], &louts[t], &lst[t]);
    lb[t].clear();
  };
  const size_t created = parallel_items(nrect, (int)T, [&](size_t i, size_t t) {
    build_q4_batch(ix, alive[i], h_of, float_on, cover_coef, lsc[t], &lb[t], &lst[t]);
    if (lb[t].seeds.size() >= seeds_per_launch) flush(t);
  });
  for (size_t t = 0; t < T; ++t) flush(t);
  st->workers_rects[2] = std::max(st->workers_rects[2], (u64)created);
  const bool drop = MHGP5_MUTANT("par-drop-shard");
  for (size_t t = 0; t < T; ++t) {
    if (drop && t == 0 && T > 1) continue;
    out->insert(out->end(), louts[t].begin(), louts[t].end());
    st->add_from(lst[t]);
  }
  st->t_rects_ms[2] += ms_since(t1);
}

inline void generate_q4_batched(const CloudIndex& ix, const GenerateOptions& opt, std::vector<BallCandidate>* out,
                                GenerateStats* st, size_t seeds_per_launch = kSeedsPerLaunch) {
  generate_q4_batched_with(ix, opt, out, st, [](Q4Batch* b, u32 h4, bool cn, bool dn, bool nc) {
    scan_q4_batch_host(b, h4, cn, dn, nc);
  }, seeds_per_launch);
}

}  // namespace mhgp5
