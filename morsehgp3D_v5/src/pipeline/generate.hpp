// MorseHGP3D v5 — generation des boules candidates : les trois lanes.
//
// Pour chaque lane q ∈ {2,3,4} :
//   1. descente WSPD ternaire (`alive_rectangles`) : une paire de nœuds est
//      MORTE si son cœur compte >= h_q temoins universels (sans descente),
//      TERMINALE si separee (instruite), SCINDEE sinon (plus grand diametre) ;
//   2. par rectangle vivant : histogrammes h_a/h_b (autorite 8 coins, exacte),
//      ancres survivantes (h_coeur + h_a + h_b < h_q — theoreme de
//      disjonction, fail-open) ;
//   3. par ancre : cover (coef 3), puis
//        q2 : la boule diametrale, toujours emise (la profondeur se decide au
//             census) ;
//        q3 : seeds aigus canoniques ; filtre de profondeur A LA GENERATION
//             (#{z ∈ cover : P(z) < 0} minore |I_B| — atteindre h_3 tue
//             AVANT l'emission ; etage flottant certifie, repli affine exact) ;
//        q4 : ancre tuee par le compte W_4 exact ; seed aigu ; cœur universel
//             du seed (Jung : P < 0 et 2P² > JB² ⟹ interieur de TOUTE sphere
//             admissible, compte a h_4) ; completions sur la lentille :
//             owner 6 aretes, exact-once du seed, prefiltres du bien-centrage
//             (etage i64, puissance de face), Cramer, centre strict ; filtre
//             de profondeur a la generation.
// Toutes les decisions sont entieres ; le flottant n'est qu'un filtre certifie
// (pipeline/float_filter.hpp). Sorties : un multiensemble de BallCandidate,
// INDEPENDANT du decoupage parallele (chaque ouvrier a son brouillon et son
// vecteur ; fusion en ordre d'ouvrier, puis tri stable + RLE canonisent).
//
// Completude (docs/MATHEMATIQUES.md § 2) : une boule pertinente pour K <= K_max
// a |I_B| <= K_max + 1 − q ; ses temoins de fuseau sont sous h_q : aucun filtre
// ci-dessus ne perd un plateau pertinent. Toute optimisation ici change le
// COUT de decouverte, jamais l'objet — c'est la porte de conformite v4 qui le
// grave (digest_balls).
#pragma once

#include <chrono>
#include <limits>
#include <vector>

#include "../lanes/edge_cover.hpp"
#include "../lanes/q2.hpp"
#include "../lanes/q3.hpp"
#include "../lanes/q4.hpp"
#include "../parallel/pool.hpp"
#include "../spindle/witness_count.hpp"
#include "../wspd/wavefront.hpp"
#include "candidates.hpp"
#include "float_filter.hpp"

namespace mhgp5 {

// Compteurs de la generation (jamais une autorite).
struct GenerateStats {
  u64 rect_alive[3] = {0, 0, 0};
  u64 rect_visited[3] = {0, 0, 0};
  u64 anchors[3] = {0, 0, 0};
  u64 anchors_killed_hist[3] = {0, 0, 0};
  u64 anchors_killed_w4 = 0;
  u64 candidates[3] = {0, 0, 0};
  u64 depth_killed[3] = {0, 0, 0};
  u64 seeds[2] = {0, 0};             // q3, q4
  u64 seeds_killed_core = 0;         // q4 : cœur de seed
  u64 q4_completions = 0, q4_rej_lens = 0, q4_rej_owner = 0, q4_rej_once = 0, q4_rej_i64 = 0,
      q4_rej_face_power = 0, q4_rej_det = 0, q4_rej_center = 0;
  u64 float_cert_neg = 0, float_cert_pos = 0, float_fallback = 0;
  u64 jung_cert_kill = 0, jung_cert_skip = 0, jung_fallback = 0;
  u64 workers_wspd[3] = {0, 0, 0};
  u64 workers_rects[3] = {0, 0, 0};
  double t_wspd_ms[3] = {0, 0, 0};
  double t_rects_ms[3] = {0, 0, 0};
  void add_from(const GenerateStats& o) {
    for (int i = 0; i < 3; ++i) {
      anchors[i] += o.anchors[i];
      anchors_killed_hist[i] += o.anchors_killed_hist[i];
      candidates[i] += o.candidates[i];
      depth_killed[i] += o.depth_killed[i];
    }
    anchors_killed_w4 += o.anchors_killed_w4;
    seeds[0] += o.seeds[0];
    seeds[1] += o.seeds[1];
    seeds_killed_core += o.seeds_killed_core;
    q4_completions += o.q4_completions;
    q4_rej_lens += o.q4_rej_lens;
    q4_rej_owner += o.q4_rej_owner;
    q4_rej_once += o.q4_rej_once;
    q4_rej_i64 += o.q4_rej_i64;
    q4_rej_face_power += o.q4_rej_face_power;
    q4_rej_det += o.q4_rej_det;
    q4_rej_center += o.q4_rej_center;
    float_cert_neg += o.float_cert_neg;
    float_cert_pos += o.float_cert_pos;
    float_fallback += o.float_fallback;
    jung_cert_kill += o.jung_cert_kill;
    jung_cert_skip += o.jung_cert_skip;
    jung_fallback += o.jung_fallback;
  }
};

struct AliveRect {
  WspdRect r;
  u64 core;  // h_coeur de la lane
};

namespace generate_detail {

inline double ms_since(std::chrono::steady_clock::time_point t0) {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

// Descente WSPD ternaire d'une lane, parallele par tranches ORDONNEES de la
// vague : chaque tranche ecrit ses tampons, la concatenation se fait en ordre
// de tranche — sortie bit-identique au sequentiel.
inline void alive_rectangles(const CloudIndex& ix, i64 s, const u64 h_of[3], int lane_idx, int threads,
                             std::vector<AliveRect>* out, u64* visited, u64* workers) {
  out->clear();
  if (ix.nodes.empty()) return;
  const u8 mask = (u8)(1u << lane_idx);
  const u64 h = h_of[lane_idx];
  std::vector<WspdRect> wave, next;
  wave.reserve(ix.nodes.size());
  for (const RadixNode& n : ix.nodes) wave.push_back(WspdRect{n.left, n.right});
  std::vector<std::vector<AliveRect>> lout;
  std::vector<std::vector<WspdRect>> lnext;
  std::vector<u64> lvis;
  while (!wave.empty()) {
    const size_t nw = wave.size();
    const int t_eff = nw < 256 ? 1 : threads;  // les premieres vagues sont minuscules
    const size_t T = planned_workers(nw, t_eff);
    const size_t chunk = std::max<size_t>(1, (nw + 8 * T - 1) / (8 * T));
    const size_t nchunks = (nw + chunk - 1) / chunk;
    lout.assign(nchunks, {});
    lnext.assign(nchunks, {});
    lvis.assign(nchunks, 0);
    const size_t created = parallel_items(nchunks, (int)T, [&](size_t c, size_t) {
      const size_t b0 = c * chunk, b1 = std::min(nw, b0 + chunk);
      for (size_t i = b0; i < b1; ++i) {
        const WspdRect& r = wave[i];
        ++lvis[c];
        const FusedCounts fc = count_universal_witnesses(ix, r.a, r.b, h_of, mask, false);
        if (fc.c[lane_idx] >= h) continue;  // MORTE sans descente
        const AxisBox va = ix.box_of(r.a), vb = ix.box_of(r.b);
        if (wspd_detail::separated(va, vb, s, 1)) {
          const FusedCounts ff = count_universal_witnesses(ix, r.a, r.b, h_of, mask, true);
          if (ff.c[lane_idx] < h) lout[c].push_back(AliveRect{r, ff.c[lane_idx]});
          continue;
        }
        const i64 w2a = wspd_detail::box_w2(va), w2b = wspd_detail::box_w2(vb);
        const bool split_a = (r.a >= 0) && (r.b < 0 || w2a >= w2b);
        const NodeRef keep = split_a ? r.b : r.a;
        const RadixNode& n = ix.nodes[(size_t)(split_a ? r.a : r.b)];
        lnext[c].push_back(split_a ? WspdRect{n.left, keep} : WspdRect{keep, n.left});
        lnext[c].push_back(split_a ? WspdRect{n.right, keep} : WspdRect{keep, n.right});
      }
    });
    *workers = std::max(*workers, (u64)created);
    next.clear();
    for (size_t c = 0; c < nchunks; ++c) {
      out->insert(out->end(), lout[c].begin(), lout[c].end());
      next.insert(next.end(), lnext[c].begin(), lnext[c].end());
      *visited += lvis[c];
    }
    wave.swap(next);
  }
}

// Histogrammes h_a/h_b (autorite 8 coins, exacte) d'un rectangle, pour une lane.
inline void corner_histograms(const CloudIndex& ix, Lane lane, const WspdRect& r, std::vector<u64>* ha,
                              std::vector<u64>* hb) {
  const NodeRange ra = ix.range_of(r.a), rb = ix.range_of(r.b);
  const AxisBox boxA = ix.box_of(r.a), boxB = ix.box_of(r.b);
  const int na = ra.last - ra.first + 1, nb = rb.last - rb.first + 1;
  ha->assign((size_t)na, 0);
  hb->assign((size_t)nb, 0);
  for (int ia = 0; ia < na; ++ia)
    for (int iz = 0; iz < na; ++iz)
      if (iz != ia && universal_over_corners(lane, ix.upos[(size_t)(ra.first + ia)], boxB, ix.upos[(size_t)(ra.first + iz)]))
        ++(*ha)[(size_t)ia];
  for (int ib = 0; ib < nb; ++ib)
    for (int iz = 0; iz < nb; ++iz)
      if (iz != ib && universal_over_corners(lane, ix.upos[(size_t)(rb.first + ib)], boxA, ix.upos[(size_t)(rb.first + iz)]))
        ++(*hb)[(size_t)ib];
}

// Brouillon par ouvrier : cover de l'ancre, lentille, sites affines.
struct AnchorScratch {
  std::vector<u64> ha, hb;
  std::vector<NodeRef> handles;
  std::vector<CoverPoint> cover, cover_tmp, lens;
  u64 cover_nodes = 0, visits = 0;
  // Sites affines de l'ancre : u = 2z−a−b, q = |u|²−D² (entiers < 2^36, exacts
  // en binaire64) ; remplis PARESSEUSEMENT au premier seed.
  std::vector<i64> su0, su1, su2, sq;
  double qmax_d = 1.0, umax_d = 1.0;
  bool affine_filled = false;
  void fill_affine_sites(const CloudIndex& ix, const P3& pa, const P3& pb, i64 D2) {
    const size_t nc = cover.size();
    su0.resize(nc); su1.resize(nc); su2.resize(nc); sq.resize(nc);
    i64 qmax = 1, umax = 1;
    const i64 sx = pa.x + pb.x, sy = pa.y + pb.y, sz = pa.z + pb.z;
    for (size_t i = 0; i < nc; ++i) {
      const P3& pz = ix.upos[(size_t)cover[i].u];
      const i64 u0 = 2 * pz.x - sx, u1 = 2 * pz.y - sy, u2 = 2 * pz.z - sz;
      const i64 qz = u0 * u0 + u1 * u1 + u2 * u2 - D2;
      su0[i] = u0; su1[i] = u1; su2[i] = u2; sq[i] = qz;
      qmax = std::max(qmax, qz < 0 ? -qz : qz);
      umax = std::max({umax, u0 < 0 ? -u0 : u0, u1 < 0 ? -u1 : u1, u2 < 0 ? -u2 : u2});
    }
    qmax_d = (double)qmax;
    umax_d = (double)umax;
    affine_filled = true;
  }
};

// Kernel affine d'un seed sur les sites de l'ancre : N = W − G·d, borne E.
struct AffineSeed {
  i128 N0, N1, N2;
  double Gd, Nd0, Nd1, Nd2, bound;
  i128 G;
  AffineSeed(const Q3Form& f3, const P3& pa, const P3& pb, const AnchorScratch& sc, bool float_on)
      : N0(f3.w[0] - f3.g * (i128)(pb.x - pa.x)),
        N1(f3.w[1] - f3.g * (i128)(pb.y - pa.y)),
        N2(f3.w[2] - f3.g * (i128)(pb.z - pa.z)),
        Gd((double)f3.g), Nd0((double)N0), Nd1((double)N1), Nd2((double)N2),
        bound(float_on ? affine_l_bound(Gd, Nd0, Nd1, Nd2, sc.qmax_d, sc.umax_d)
                       : std::numeric_limits<double>::infinity()),
        G(f3.g) {}
  double l_hat(const AnchorScratch& sc, size_t iz) const {
    return affine_l_hat(Gd, Nd0, Nd1, Nd2, (double)sc.su0[iz], (double)sc.su1[iz], (double)sc.su2[iz],
                        (double)sc.sq[iz]);
  }
  // L = 4P exact (i128, |L| < 2^105).
  i128 l_exact(const AnchorScratch& sc, size_t iz) const {
    return G * (i128)sc.sq[iz] - 2 * ((i128)sc.su0[iz] * N0 + (i128)sc.su1[iz] * N1 + (i128)sc.su2[iz] * N2);
  }
};

}  // namespace generate_detail

struct GenerateOptions {
  i64 s = 8;
  u64 smax = 11;
  int threads = 1;
};

// Execute une lane sur ses rectangles vivants : chaque ouvrier a son brouillon,
// son vecteur d'emissions et ses compteurs ; fusion en ordre d'ouvrier.
// Mutant `par-drop-shard` : la fusion oublie le premier ouvrier.
template <typename Body>
inline void run_lane(const std::vector<AliveRect>& alive, int threads, int lane_idx, std::vector<BallCandidate>* out,
                     GenerateStats* st, Body&& body) {
  const size_t nrect = alive.size();
  const size_t T = planned_workers(nrect, threads);
  std::vector<std::vector<BallCandidate>> louts(T);
  std::vector<GenerateStats> lst(T);
  std::vector<generate_detail::AnchorScratch> lsc(T);
  const size_t created = parallel_items(nrect, (int)T, [&](size_t i, size_t t) { body(alive[i], lsc[t], &louts[t], &lst[t]); });
  st->workers_rects[lane_idx] = std::max(st->workers_rects[lane_idx], (u64)created);
  const bool drop = MHGP5_MUTANT("par-drop-shard");
  for (size_t t = 0; t < T; ++t) {
    if (drop && t == 0 && T > 1) continue;
    out->insert(out->end(), louts[t].begin(), louts[t].end());
    st->add_from(lst[t]);
  }
}

inline void generate_candidates(const CloudIndex& ix, const GenerateOptions& opt, std::vector<BallCandidate>* out,
                                GenerateStats* st) {
  using namespace generate_detail;
  out->clear();
  const bool float_on = float_filter_runtime_enabled();
  const bool genfilter_nonstrict = MHGP5_MUTANT("genfilter-nonstrict");
  const bool seed_core_nonstrict = MHGP5_MUTANT("q4-seed-core-nonstrict");
  const bool q4_from_q3_live = MHGP5_MUTANT("q4-seeds-from-q3-live");
  const bool no_canonical = MHGP5_MUTANT("q4-no-canonical");
  // MUTANT q4-cover-coef4 : cover au coefficient 4 — sans effet sur l'objet,
  // mais il tue des candidats profonds avant l'emission et change
  // `digest_balls` (fixture q4_cover_fixture : la boule differentielle).
  const i64 q4_cover_coef = MHGP5_MUTANT("q4-cover-coef4") ? 4 : 3;
  const u64 h_of[3] = {lane_h(Lane::kQ2, opt.smax), lane_h(Lane::kQ3, opt.smax), lane_h(Lane::kQ4, opt.smax)};
  const auto pid = [&](i32 u) { return ix.point_id(u); };
  std::vector<AliveRect> alive;

  // ---- q2.
  {
    const auto t0 = std::chrono::steady_clock::now();
    alive_rectangles(ix, opt.s, h_of, 0, opt.threads, &alive, &st->rect_visited[0], &st->workers_wspd[0]);
    st->t_wspd_ms[0] += ms_since(t0);
    st->rect_alive[0] = alive.size();
    const auto t1 = std::chrono::steady_clock::now();
    run_lane(alive, opt.threads, 0, out, st, [&](const AliveRect& ar, AnchorScratch& sc, std::vector<BallCandidate>* lo, GenerateStats* ls) {
      corner_histograms(ix, Lane::kQ2, ar.r, &sc.ha, &sc.hb);
      const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
      const u64 need = h_of[0] - ar.core;
      for (i32 ua = ra.first; ua <= ra.last; ++ua)
        for (i32 ub = rb.first; ub <= rb.last; ++ub) {
          ++ls->anchors[0];
          if (sc.ha[(size_t)(ua - ra.first)] + sc.hb[(size_t)(ub - rb.first)] >= need) {
            ++ls->anchors_killed_hist[0];
            continue;
          }
          const P3& pa = ix.upos[(size_t)ua];
          const P3& pb = ix.upos[(size_t)ub];
          const i64 D2 = p3_norm2(p3_sub(pb, pa));
          if (D2 == 0) continue;
          lo->push_back(BallCandidate{q2_ball_key(pa, pb), promote_level(q2_exact_level(D2)), 2});
          ++ls->candidates[0];
        }
    });
    st->t_rects_ms[0] += ms_since(t1);
  }

  // ---- q3.
  {
    const auto t0 = std::chrono::steady_clock::now();
    alive_rectangles(ix, opt.s, h_of, 1, opt.threads, &alive, &st->rect_visited[1], &st->workers_wspd[1]);
    st->t_wspd_ms[1] += ms_since(t0);
    st->rect_alive[1] = alive.size();
    const auto t1 = std::chrono::steady_clock::now();
    run_lane(alive, opt.threads, 1, out, st, [&](const AliveRect& ar, AnchorScratch& sc, std::vector<BallCandidate>* lo, GenerateStats* ls) {
      corner_histograms(ix, Lane::kQ3, ar.r, &sc.ha, &sc.hb);
      const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
      rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes);
      const u64 need = h_of[1] - ar.core;
      for (i32 ua = ra.first; ua <= ra.last; ++ua)
        for (i32 ub = rb.first; ub <= rb.last; ++ub) {
          ++ls->anchors[1];
          if (sc.ha[(size_t)(ua - ra.first)] + sc.hb[(size_t)(ub - rb.first)] >= need) {
            ++ls->anchors_killed_hist[1];
            continue;
          }
          const P3& pa = ix.upos[(size_t)ua];
          const P3& pb = ix.upos[(size_t)ub];
          const i64 D2 = p3_norm2(p3_sub(pb, pa));
          if (D2 == 0) continue;
          anchor_cover_from_handles(ix, sc.handles, pa, pb, D2, 3, &sc.cover, &sc.visits, &sc.cover_tmp);
          sc.affine_filled = false;
          for (const CoverPoint& cp : sc.cover) {
            if (cp.u == ua || cp.u == ub) continue;
            const P3& px = ix.upos[(size_t)cp.u];
            if (!is_acute_seed(pa, pb, px, D2, pid(ua), pid(ub), pid(cp.u))) continue;
            ++ls->seeds[0];
            if (!sc.affine_filled) sc.fill_affine_sites(ix, pa, pb, D2);
            const Q3Form f3 = q3_form(pa, pb, px);
            const AffineSeed seed(f3, pa, pb, sc, float_on);
            // Filtre de profondeur a la generation : minorant certifie.
            u64 depth = 0;
            bool deep = false;
            for (size_t iz = 0; iz < sc.cover.size() && !deep; ++iz) {
              const double lh = seed.l_hat(sc, iz);
              bool interior;
              if (lh < -seed.bound) {
                ++ls->float_cert_neg;
                interior = true;
              } else if (lh > seed.bound) {
                ++ls->float_cert_pos;
                interior = false;
              } else {
                ++ls->float_fallback;
                const i128 L = seed.l_exact(sc, iz);
                interior = L < 0 || (genfilter_nonstrict && L <= 0);
              }
              if (interior && ++depth >= h_of[1]) deep = true;
            }
            if (deep) {
              ++ls->depth_killed[1];
              continue;
            }
            lo->push_back(BallCandidate{q3_ball_key(f3), promote_level(q3_exact_level(pa, pb, px)), 3});
            ++ls->candidates[1];
          }
        }
    });
    st->t_rects_ms[1] += ms_since(t1);
  }

  // ---- q4.
  {
    const auto t0 = std::chrono::steady_clock::now();
    // MUTANT q4-seeds-from-q3-live : la source q4 branchee sur la lane q3
    // (perd les ancres q3-mortes / q4-vivantes — fixture bloquante).
    alive_rectangles(ix, opt.s, h_of, q4_from_q3_live ? 1 : 2, opt.threads, &alive, &st->rect_visited[2],
                     &st->workers_wspd[2]);
    st->t_wspd_ms[2] += ms_since(t0);
    st->rect_alive[2] = alive.size();
    const auto t1 = std::chrono::steady_clock::now();
    run_lane(alive, opt.threads, 2, out, st, [&](const AliveRect& ar, AnchorScratch& sc, std::vector<BallCandidate>* lo, GenerateStats* ls) {
      corner_histograms(ix, Lane::kQ4, ar.r, &sc.ha, &sc.hb);
      const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
      rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), q4_cover_coef, &sc.handles, &sc.cover_nodes);
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
          // Cover au coefficient 3 (la lentille des sommets). Les interieurs
          // et la coquille d'une boule q4 vivent dans le coefficient 4 (Jung),
          // mais ici le cover n'est qu'un SOUS-ENSEMBLE pour des minorants
          // fail-open (W_4, cœur de seed, profondeur a la generation) : le
          // census exact passe par l'arbre entier. Le coefficient 3 est celui
          // de la v4 ; un coefficient 4 tuerait quelques boules profondes de
          // plus avant l'emission (23 sur 1,4 M a uniform n=8000) sans changer
          // l'objet — mais changerait `digest_balls`, qui compte les candidats
          // profonds (docs/PROVENANCE.md, conformite).
          anchor_cover_from_handles(ix, sc.handles, pa, pb, D2, q4_cover_coef, &sc.cover, &sc.visits, &sc.cover_tmp);
          // Compte W_4 exact par ancre : n4 >= h_4 tue l'ancre entiere.
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
          sc.affine_filled = false;
          sc.lens.clear();
          for (const CoverPoint& cz : sc.cover) {
            const P3& pz = ix.upos[(size_t)cz.u];
            if (p3_norm2(p3_sub(pz, pa)) <= D2 && p3_norm2(p3_sub(pz, pb)) <= D2) sc.lens.push_back(cz);
          }
          for (const CoverPoint& cx : sc.lens) {
            if (cx.u == ua || cx.u == ub) continue;
            const P3& px = ix.upos[(size_t)cx.u];
            if (!is_acute_seed(pa, pb, px, D2, pid(ua), pid(ub), pid(cx.u))) continue;
            ++ls->seeds[1];
            const i64 l_ax = p3_norm2(p3_sub(px, pa));
            const i64 l_bx = p3_norm2(p3_sub(px, pb));
            const Q3Form f3s = q3_form(pa, pb, px);
            const P3 nrm = p3_cross(p3_sub(pb, pa), p3_sub(px, pa));
            // Cœur universel du seed (Jung) : J = D²(3G − 2 l_ax l_bx).
            const i128 Jb = (i128)D2 * (3 * f3s.g - 2 * (i128)l_ax * l_bx);
            bool dead = Jb < 0;
            if (!dead) {
              if (!sc.affine_filled) sc.fill_affine_sites(ix, pa, pb, D2);
              const AffineSeed seed(f3s, pa, pb, sc, float_on);
              const double Jd = (double)Jb;
              const double Jlo = Jd * (1.0 - kJungGuard), Jhi = Jd * (1.0 + kJungGuard);
              u64 fcount = 0;
              for (size_t iz = 0; iz < sc.cover.size(); ++iz) {
                const CoverPoint& cz = sc.cover[iz];
                if (cz.u == ua || cz.u == ub || cz.u == cx.u) continue;
                const double lh = seed.l_hat(sc, iz);
                if (lh > seed.bound) {
                  ++ls->float_cert_pos;
                  continue;  // P > 0 certifie : jamais temoin
                }
                if (lh < -seed.bound) {
                  ++ls->float_cert_neg;
                  const P3& pz = ix.upos[(size_t)cz.u];
                  const i64 Bz = p3_dot(nrm, p3_sub(pz, f3s.a));
                  const int js = jung_interval_sign(lh, seed.bound, Jlo, Jhi, Bz);
                  if (js != 0) {
                    if (js > 0) {
                      ++ls->jung_cert_kill;
                      if (++fcount >= h_of[2]) break;
                    } else {
                      ++ls->jung_cert_skip;
                    }
                    continue;
                  }
                  ++ls->jung_fallback;
                  const i128 Pz = seed.l_exact(sc, iz) / 4;
                  const int c = cmp_2p2_jb2(Pz, Jb, Bz);
                  if ((seed_core_nonstrict ? (c >= 0) : (c > 0)) && ++fcount >= h_of[2]) break;
                  continue;
                }
                ++ls->float_fallback;
                const i128 Pz = seed.l_exact(sc, iz) / 4;
                if (seed_core_nonstrict ? (Pz > 0) : (Pz >= 0)) continue;
                const P3& pz = ix.upos[(size_t)cz.u];
                const i64 Bz = p3_dot(nrm, p3_sub(pz, f3s.a));
                const int c = cmp_2p2_jb2(Pz, Jb, Bz);
                if ((seed_core_nonstrict ? (c >= 0) : (c > 0)) && ++fcount >= h_of[2]) break;
              }
              dead = fcount >= h_of[2];
            }
            if (dead) {
              ++ls->seeds_killed_core;
              continue;
            }
            // Completions y sur la lentille.
            for (const CoverPoint& cy : sc.lens) {
              const i32 uy = cy.u;
              if (uy == cx.u || uy == ua || uy == ub) continue;
              ++ls->q4_completions;
              const P3& py = ix.upos[(size_t)uy];
              const i64 l_ay = p3_norm2(p3_sub(py, pa));
              const i64 l_by = p3_norm2(p3_sub(py, pb));
              const i64 l_xy = p3_norm2(p3_sub(py, px));
              if (l_ay > D2 || l_by > D2 || l_xy > D2) { ++ls->q4_rej_lens; continue; }
              if (!tetra_owned_by(D2, l_ax, l_ay, l_bx, l_by, l_xy, pid(ua), pid(ub), pid(cx.u), pid(uy))) {
                ++ls->q4_rej_owner;
                continue;
              }
              // Exact-once du seed : le carrier est le plus petit PointId
              // parmi les faces incidentes aigues (mutant q4-no-canonical :
              // toutes les faces emettent ; le RLE masque, la porte compte).
              const P3 vy{2 * py.x - pa.x - pb.x, 2 * py.y - pa.y - pb.y, 2 * py.z - pa.z - pb.z};
              if (!no_canonical && p3_norm2(vy) > D2 && pid(uy) < pid(cx.u)) { ++ls->q4_rej_once; continue; }
              // Prefiltres du bien-centrage : etage i64 puis puissance de face.
              if (!q4_i64_prefilter(D2, l_ax, l_bx, l_ay, l_by, l_xy)) { ++ls->q4_rej_i64; continue; }
              if (!q4_face_power_prefilter(f3s, py)) { ++ls->q4_rej_face_power; continue; }
              const Q4Form f4 = q4_form(pa, pb, px, py);
              if (f4.det == 0) { ++ls->q4_rej_det; continue; }
              if (!q4_center_strictly_inside(f4, pa, pb, px, py)) { ++ls->q4_rej_center; continue; }
              // Filtre de profondeur a la generation.
              u64 depth = 0;
              bool deep = false;
              for (const CoverPoint& cz : sc.cover) {
                const i128 pw = q4_power(f4, ix.upos[(size_t)cz.u]);
                if ((pw < 0 || (genfilter_nonstrict && pw <= 0)) && ++depth >= h_of[2]) { deep = true; break; }
              }
              if (deep) { ++ls->depth_killed[2]; continue; }
              lo->push_back(BallCandidate{ball_key_reduce(q4_ball_form(f4)), q4_level_raw(f4), 4});
              ++ls->candidates[2];
            }
          }
        }
    });
    st->t_rects_ms[2] += ms_since(t1);
  }
}

}  // namespace mhgp5
