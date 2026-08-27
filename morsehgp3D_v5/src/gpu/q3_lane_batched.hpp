// MorseHGP3D v5 — lane q3 PAR LOTS (docs/GPU.md, livraison 4, etage hote).
// Le contrat de la future lane device, ecrit et prouve sur CPU :
//   1. build_q3_batch : pour un rectangle vivant, l'hote forme les tableaux
//      plats (sites affines de chaque ancre survivante en SoA, seeds aigus
//      avec leur forme affine, leur candidat pret a emettre) — aucune
//      decision de profondeur n'est prise ici ;
//   2. un EXECUTEUR de scan rend, par seed, le verdict mort/vivant et les
//      compteurs de certification — ici `scan_q3_batch_host` (boucle plate
//      sur q3_scan_seed_shaped) ; le device (k_scan warp-par-seed de
//      device_witness.cu) remplira le meme contrat ;
//   3. emit_q3_batch : l'hote emet les candidats des seeds vivants DANS
//      L'ORDRE des seeds (ancres en (ua, ub), seeds en ordre de cover), donc
//      dans l'ordre exact de la lane de production.
// tests/q3_lane_batched_gate.cpp exige l'egalite VECTEUR A VECTEUR (ordre
// compris) des candidats q3 et l'egalite des compteurs avec generate.hpp.
// Mutant `q3-batched-emit-dead` : les seeds morts sont emis (tue en code 4).
#pragma once

#include <limits>
#include <vector>

#include "../core/mutants.hpp"
#include "../lanes/edge_cover.hpp"
#include "../lanes/q3.hpp"
#include "../pipeline/float_filter.hpp"
#include "../pipeline/generate.hpp"
#include "q3_scan_shaped.hpp"

namespace mhgp5 {

struct Q3BatchAnchor {
  u32 begin = 0, count = 0;  // tranche des sites dans les tableaux SoA
};
struct Q3BatchSeed {
  SeedAffineD seed;
  u32 anchor = 0;
};
struct Q3BatchVerdict {
  u32 dead = 0, cert_neg = 0, cert_pos = 0, fallback = 0;
};

// Un lot = un rectangle vivant (les vecteurs sont reutilises entre rectangles).
struct Q3Batch {
  std::vector<i64> u0, u1, u2, q;
  std::vector<double> u0d, u1d, u2d, qd;
  std::vector<Q3BatchAnchor> anchors;
  std::vector<Q3BatchSeed> seeds;
  std::vector<BallCandidate> emit_if_alive;  // un par seed
  std::vector<Q3BatchVerdict> verdicts;      // rempli par l'executeur
  void clear() {
    u0.clear(); u1.clear(); u2.clear(); q.clear();
    u0d.clear(); u1d.clear(); u2d.clear(); qd.clear();
    anchors.clear(); seeds.clear(); emit_if_alive.clear(); verdicts.clear();
  }
};

// Etage 1 : formation du lot (aucun verdict). Compte anchors/anchors_killed_hist/seeds
// comme la lane de production.
inline void build_q3_batch(const CloudIndex& ix, const AliveRect& ar, const u64 h_of[3], bool float_on,
                           generate_detail::AnchorScratch& sc, Q3Batch* b, GenerateStats* ls) {
  using namespace generate_detail;
  b->clear();
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
      u32 aidx = std::numeric_limits<u32>::max();
      for (const CoverPoint& cp : sc.cover) {
        if (cp.u == ua || cp.u == ub) continue;
        const P3& px = ix.upos[(size_t)cp.u];
        if (!is_acute_seed(pa, pb, px, D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(cp.u))) continue;
        ++ls->seeds[0];
        if (!sc.affine_filled) {
          // Sites de l'ancre : remplis au premier seed, comme en production, puis
          // copies dans le lot (i64 et doubles exactes).
          sc.fill_affine_sites(ix, pa, pb, D2);
          aidx = (u32)b->anchors.size();
          const u32 begin = (u32)b->u0.size();
          const size_t nc = sc.cover.size();
          b->anchors.push_back(Q3BatchAnchor{begin, (u32)nc});
          for (size_t i = 0; i < nc; ++i) {
            b->u0.push_back(sc.su0[i]); b->u1.push_back(sc.su1[i]); b->u2.push_back(sc.su2[i]); b->q.push_back(sc.sq[i]);
            b->u0d.push_back((double)sc.su0[i]); b->u1d.push_back((double)sc.su1[i]);
            b->u2d.push_back((double)sc.su2[i]); b->qd.push_back((double)sc.sq[i]);
          }
        }
        const Q3Form f3 = q3_form(pa, pb, px);
        const AffineSeed as(f3, pa, pb, sc, float_on);
        Q3BatchSeed s;
        s.seed.G = di_from_i128(as.G);
        s.seed.N0 = di_from_i128(as.N0);
        s.seed.N1 = di_from_i128(as.N1);
        s.seed.N2 = di_from_i128(as.N2);
        s.seed.Gd = as.Gd; s.seed.Nd0 = as.Nd0; s.seed.Nd1 = as.Nd1; s.seed.Nd2 = as.Nd2; s.seed.bound = as.bound;
        s.anchor = aidx;
        b->seeds.push_back(s);
        b->emit_if_alive.push_back(BallCandidate{q3_ball_key(f3), promote_level(q3_exact_level(pa, pb, px)), 3});
      }
    }
}

// Etage 2 (executeur hote) : un verdict par seed, boucle plate — la forme que
// le kernel transcrit (un warp par seed).
inline void scan_q3_batch_host(Q3Batch* b, u32 h3, bool nonstrict) {
  b->verdicts.resize(b->seeds.size());
  for (size_t i = 0; i < b->seeds.size(); ++i) {
    const Q3BatchSeed& s = b->seeds[i];
    const Q3BatchAnchor& a = b->anchors[s.anchor];
    const AnchorSitesSoA sites{b->u0.data() + a.begin, b->u1.data() + a.begin, b->u2.data() + a.begin,
                               b->q.data() + a.begin,  b->u0d.data() + a.begin, b->u1d.data() + a.begin,
                               b->u2d.data() + a.begin, b->qd.data() + a.begin, a.count};
    Q3BatchVerdict v;
    v.dead = q3_scan_seed_shaped(s.seed, sites, h3, std::numeric_limits<u32>::max(), &v.cert_neg, &v.cert_pos,
                                 &v.fallback, nonstrict) ? 1u : 0u;
    b->verdicts[i] = v;
  }
}

// Etage 3 : emission ordonnee et compteurs.
inline void emit_q3_batch(const Q3Batch& b, std::vector<BallCandidate>* lo, GenerateStats* ls) {
  const bool emit_dead = MHGP5_MUTANT("q3-batched-emit-dead");
  for (size_t i = 0; i < b.seeds.size(); ++i) {
    const Q3BatchVerdict& v = b.verdicts[i];
    ls->float_cert_neg += v.cert_neg;
    ls->float_cert_pos += v.cert_pos;
    ls->float_fallback += v.fallback;
    ls->q3_cert[0] += v.cert_neg;
    ls->q3_cert[1] += v.cert_pos;
    ls->q3_cert[2] += v.fallback;
    if (v.dead && !emit_dead) {
      ++ls->depth_killed[1];
      continue;
    }
    lo->push_back(b.emit_if_alive[i]);
    ++ls->candidates[1];
  }
}

// Lane q3 complete par lots (executeur hote) : meme signature d'effet que la
// lane q3 de generate_candidates (candidats ajoutes a `out`, stats cumulees).
inline void generate_q3_batched(const CloudIndex& ix, const GenerateOptions& opt, std::vector<BallCandidate>* out,
                                GenerateStats* st) {
  using namespace generate_detail;
  const bool float_on = float_filter_runtime_enabled();
  const bool nonstrict = MHGP5_MUTANT("genfilter-nonstrict");
  const u64 h_of[3] = {lane_h(Lane::kQ2, opt.smax), lane_h(Lane::kQ3, opt.smax), lane_h(Lane::kQ4, opt.smax)};
  std::vector<AliveRect> alive;
  const auto t0 = std::chrono::steady_clock::now();
  alive_rectangles(ix, opt.s, h_of, 1, opt.threads, &alive, &st->rect_visited[1], &st->workers_wspd[1]);
  st->t_wspd_ms[1] += ms_since(t0);
  st->rect_alive[1] = alive.size();
  const auto t1 = std::chrono::steady_clock::now();
  run_lane(alive, opt.threads, 1, out, st, [&](const AliveRect& ar, AnchorScratch& sc, std::vector<BallCandidate>* lo, GenerateStats* ls) {
    // Un brouillon de lot par fil d'execution (reutilise entre rectangles).
    thread_local Q3Batch tl;
    build_q3_batch(ix, ar, h_of, float_on, sc, &tl, ls);
    scan_q3_batch_host(&tl, (u32)h_of[1], nonstrict);
    emit_q3_batch(tl, lo, ls);
  });
  st->t_rects_ms[1] += ms_since(t1);
}

}  // namespace mhgp5
