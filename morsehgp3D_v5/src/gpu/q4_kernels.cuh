// MorseHGP3D v5 — KERNELS q4 (docs/GPU.md, livraison 5b) : transcription des
// fonctions « en forme de kernel » de q4_core_shaped.hpp et
// q4_completion_shaped.hpp. Compile par nvcc seulement.
//   k_q4_core     : un WARP par seed — les 32 fils balaient les sites de
//                   l'ancre, six compteurs par ballot, sortie anticipee a h4
//                   reproduite (correction intra-warp : les compteurs des
//                   sites au-dela du h4-ieme temoin sont retires) ;
//   k_q4_complete : un BLOC par seed vivant — les fils parcourent la
//                   lentille de l'ancre et ecrivent l'etage atteint de chaque
//                   paire (seed, y) (Q4Stage, avant profondeur) ;
//   k_q4_depth    : un WARP par paire candidate (etage kEmit) — profondeur
//                   sur les positions de l'ancre par ballot, sortie anticipee.
// L'hote compacte les paires candidates entre k_q4_complete et k_q4_depth et
// recalcule cle et niveau des emissions (q4_lane_batched.hpp).
#pragma once

#include "../core/dint.hpp"
#include "../core/types.hpp"
#include "q4_completion_shaped.hpp"
#include "q3_scan_kernel.cuh"
#include "q4_core_shaped.hpp"
#include "q4_lane_batched.hpp"

namespace mhgp5 {
namespace gpu {

#if defined(__CUDACC__)
// Sites d'un lot q4 : wire SoA (u0..q, px..pz, pid copies par ancre : 60 o/site)
// ou wire G1 par INDICES (`idx` non nul : 4 o/site + geometrie et PointId
// RESIDENTS) ; les accesseurs reconstruisent u = 2z − (a+b) et q = |u|² − D²
// en i64 exacts — les memes entiers que fill_affine_sites sur l'hote.
struct Q4SitesDev {
  const i64* u0;
  const i64* u1;
  const i64* u2;
  const i64* q;
  const i64* px;
  const i64* py;
  const i64* pz;
  const PointId* pid;
  const u32* lens_sites;
  const unsigned* idx = nullptr;  // wire G1
  GeomDev geom{nullptr, nullptr, nullptr};
  const PointId* gpid = nullptr;  // PointId par position residente
  __device__ __forceinline__ i64 PX(unsigned g) const { return idx ? (i64)geom.px[idx[g]] : px[g]; }
  __device__ __forceinline__ i64 PY(unsigned g) const { return idx ? (i64)geom.py[idx[g]] : py[g]; }
  __device__ __forceinline__ i64 PZ(unsigned g) const { return idx ? (i64)geom.pz[idx[g]] : pz[g]; }
  __device__ __forceinline__ PointId PID(unsigned g) const { return idx ? gpid[idx[g]] : pid[g]; }
  __device__ __forceinline__ i64 U0(unsigned g, const Q4BatchAnchor& an) const { return idx ? 2 * PX(g) - (an.a.x + an.b.x) : u0[g]; }
  __device__ __forceinline__ i64 U1(unsigned g, const Q4BatchAnchor& an) const { return idx ? 2 * PY(g) - (an.a.y + an.b.y) : u1[g]; }
  __device__ __forceinline__ i64 U2(unsigned g, const Q4BatchAnchor& an) const { return idx ? 2 * PZ(g) - (an.a.z + an.b.z) : u2[g]; }
  __device__ __forceinline__ i64 Q(unsigned g, const Q4BatchAnchor& an) const {
    if (!idx) return q[g];
    const i64 a0 = U0(g, an), a1 = U1(g, an), a2 = U2(g, an);
    return a0 * a0 + a1 * a1 + a2 * a2 - an.D2;
  }
};

__device__ inline unsigned keep_mask_upto(unsigned witness_mask, unsigned need) {
  // Masque des lanes <= la `need`-ieme lane temoin (need >= 1).
  unsigned m = witness_mask, kth = 0;
  for (unsigned k = 0; k < need; ++k) {
    kth = __ffs(m) - 1;
    m &= m - 1;
  }
  return (kth == 31) ? 0xffffffffu : ((1u << (kth + 1)) - 1u);
}

// Corde (chord_kill.hpp) sur device : chaque lane evalue ses temoignages par
// morceau ; les comptes par morceau sont des ballots ; la mort (fcount >= h4
// OU tous les morceaux >= h4) est detectee a la premiere lane ou elle survient
// (prefixes de popcount), et les compteurs des lanes suivantes sont retires —
// meme sequence que le scalaire q4_seed_core_shaped.
__global__ void k_q4_core(const Q4BatchSeed* seeds, unsigned nseeds, const Q4BatchAnchor* anchors, Q4SitesDev S,
                          unsigned h4, bool nonstrict, bool chord_nonstrict, Q4SeedVerdict* out) {
  const unsigned warp = (blockIdx.x * blockDim.x + threadIdx.x) >> 5;
  const unsigned lane = threadIdx.x & 31;
  if (warp >= nseeds) return;
  const Q4BatchSeed sd = seeds[warp];
  const Q4BatchAnchor an = anchors[sd.anchor];
  Q4SeedVerdict v;
  if (sd.jneg) {
    v.dead = 1;
    if (lane == 0) out[warp] = v;
    return;
  }
  const i128 J = di_to_i128_hd(sd.core.J);
  ChordPieces chord;
  chord.init(J, chord_nonstrict);
  unsigned fcount = 0;
  unsigned piece_cnt[kChordPieces] = {0, 0, 0, 0};
  bool dead = false, dead_chord = false;
  for (unsigned base = 0; base < an.count && !dead; base += 32) {
    const unsigned i = base + lane;
    // Classification du site de la lane : un bit par compteur, un bit temoin, un bit par morceau.
    unsigned my_pos = 0, my_neg = 0, my_kill = 0, my_skip = 0, my_jf = 0, my_ff = 0, my_wit = 0;
    unsigned my_piece = 0;  // bit k : temoin du morceau k
    if (i < an.count && i != an.skip_a && i != an.skip_b && i != sd.core.skip_x) {
      const unsigned g = an.begin + i;
      const double lh = q3_l_hat_shaped(sd.core.aff, (double)S.U0(g, an), (double)S.U1(g, an), (double)S.U2(g, an), (double)S.Q(g, an));
      if (lh > sd.core.aff.bound) {
        my_pos = 1;
      } else {
        const i64 nu = sd.core.n0 * S.U0(g, an) + sd.core.n1 * S.U1(g, an) + sd.core.n2 * S.U2(g, an);
        const i64 Bz = nu / 2;
        {
          ChordPieces local;
          local.init(J, chord_nonstrict);
          local.update(lh, sd.core.aff.bound, Bz, [&]() {
            return di_to_i128_hd(q3_l_exact_shaped(sd.core.aff, S.U0(g, an), S.U1(g, an), S.U2(g, an), S.Q(g, an)));
          });
          for (int k = 0; k < kChordPieces; ++k) if (local.cnt[k]) my_piece |= 1u << k;
        }
        if (lh < -sd.core.aff.bound) {
          my_neg = 1;
          const int js = jung_interval_sign_shaped(lh, sd.core.aff.bound, sd.core.Jlo, sd.core.Jhi, Bz);
          if (js > 0) {
            my_kill = 1;
            my_wit = 1;
          } else if (js < 0) {
            my_skip = 1;
          } else {
            my_jf = 1;
            const DI128 P = di_div_by_4_exact(q3_l_exact_shaped(sd.core.aff, S.U0(g, an), S.U1(g, an), S.U2(g, an), S.Q(g, an)));
            const int cm = cmp_2p2_jb2(di_to_i128_hd(P), J, Bz);
            my_wit = (nonstrict ? (cm >= 0) : (cm > 0)) ? 1 : 0;
          }
        } else {
          my_ff = 1;
          const DI128 P = di_div_by_4_exact(q3_l_exact_shaped(sd.core.aff, S.U0(g, an), S.U1(g, an), S.U2(g, an), S.Q(g, an)));
          const int sg = di_sign(P);
          if (!(nonstrict ? (sg > 0) : (sg >= 0))) {
            const int cm = cmp_2p2_jb2(di_to_i128_hd(P), J, Bz);
            my_wit = (nonstrict ? (cm >= 0) : (cm > 0)) ? 1 : 0;
          }
        }
      }
    }
    const unsigned wit = __ballot_sync(0xffffffffu, my_wit != 0);
    unsigned pm[kChordPieces];
    for (int k = 0; k < kChordPieces; ++k) pm[k] = __ballot_sync(0xffffffffu, (my_piece >> k) & 1u);
    // Premiere lane ou la mort survient : par le cœur (h4-ieme temoin) ou par la corde (chaque morceau atteint h4).
    unsigned death_lane = 32;  // 32 = pas de mort dans ce pas
    const unsigned add = __popc(wit);
    if (fcount + add >= h4) {
      const unsigned m = keep_mask_upto(wit, h4 - fcount);
      death_lane = 31 - __clz(m);  // lane du h4-ieme temoin
    }
    {
      // Corde : lane ou le DERNIER morceau atteint h4 (max sur k de la lane du (h4 − piece_cnt[k])-ieme temoin du morceau k).
      unsigned dl = 0;
      bool all = true;
      for (int k = 0; k < kChordPieces && all; ++k) {
        const unsigned need = h4 > piece_cnt[k] ? h4 - piece_cnt[k] : 0;
        if (need == 0) continue;
        if ((unsigned)__popc(pm[k]) < need) { all = false; break; }
        const unsigned mk = keep_mask_upto(pm[k], need);
        const unsigned lk = 31 - __clz(mk);
        if (lk > dl) dl = lk;
      }
      if (all && dl < death_lane) { death_lane = dl; dead_chord = true; }
      else if (all && dl == death_lane) { dead_chord = false; }  // egalite : le cœur est constate d'abord (meme ordre que le scalaire : test du cœur avant la corde)
    }
    unsigned keep = 0xffffffffu;
    if (death_lane < 32) {
      keep = (death_lane == 31) ? 0xffffffffu : ((1u << (death_lane + 1)) - 1u);
      dead = true;
    }
    fcount += __popc(wit & keep);
    for (int k = 0; k < kChordPieces; ++k) piece_cnt[k] += __popc(pm[k] & keep);
    v.c.cert_pos += __popc(__ballot_sync(0xffffffffu, my_pos != 0) & keep);
    v.c.cert_neg += __popc(__ballot_sync(0xffffffffu, my_neg != 0) & keep);
    v.c.jung_kill += __popc(__ballot_sync(0xffffffffu, my_kill != 0) & keep);
    v.c.jung_skip += __popc(__ballot_sync(0xffffffffu, my_skip != 0) & keep);
    v.c.jung_fallback += __popc(__ballot_sync(0xffffffffu, my_jf != 0) & keep);
    v.c.float_fallback += __popc(__ballot_sync(0xffffffffu, my_ff != 0) & keep);
  }
  v.dead = dead ? 1u : 0u;
  v.c.dead_by_chord = (dead && dead_chord) ? 1u : 0u;
  if (lane == 0) out[warp] = v;
}

// Un bloc par seed VIVANT (alive[blockIdx.x] = index du seed) ; les fils
// parcourent la lentille ; stage[pair_off[blockIdx.x] + li] = etage.
__global__ void k_q4_complete(const Q4BatchSeed* seeds, const Q4BatchAnchor* anchors, Q4SitesDev S, const u32* alive,
                              const u32* pair_off, bool no_canonical, u8* stage) {
  const Q4BatchSeed sd = seeds[alive[blockIdx.x]];
  const Q4BatchAnchor an = anchors[sd.anchor];
  const unsigned gx = an.begin + sd.x_site;
  const P3 x{S.PX(gx), S.PY(gx), S.PZ(gx)};
  const PointId idx = S.PID(gx);
  const u32 base = pair_off[blockIdx.x];
  for (unsigned li = threadIdx.x; li < an.lens_count; li += blockDim.x) {
    const u32 ys = S.lens_sites[an.lens_begin + li];
    u8 st = 255;  // paire ignoree (x, a ou b)
    if (ys != sd.x_site && ys != an.skip_a && ys != an.skip_b) {
      const unsigned gy = an.begin + ys;
      const P3 y{S.PX(gy), S.PY(gy), S.PZ(gy)};
      Q4FormD f4;
      st = (u8)q4_completion_stage_shaped(an.a, an.b, x, y, an.ida, an.idb, idx, S.PID(gy), an.D2, sd.l_ax, sd.l_bx,
                                          sd.face, no_canonical, false, &f4);
    }
    stage[base + li] = st;
  }
}

// Un warp par paire candidate : deep[c] = 1 si >= h4 sites de puissance < 0.
__global__ void k_q4_depth(const Q4BatchSeed* seeds, const Q4BatchAnchor* anchors, Q4SitesDev S, const Q4Emit* cand,
                           unsigned ncand, unsigned h4, bool nonstrict, u8* deep) {
  const unsigned warp = (blockIdx.x * blockDim.x + threadIdx.x) >> 5;
  const unsigned lane = threadIdx.x & 31;
  if (warp >= ncand) return;
  const Q4Emit e = cand[warp];
  const Q4BatchSeed sd = seeds[e.seed];
  const Q4BatchAnchor an = anchors[sd.anchor];
  const unsigned gx = an.begin + sd.x_site, gy = an.begin + e.y_site;
  const P3 x{S.PX(gx), S.PY(gx), S.PZ(gx)};
  const P3 y{S.PX(gy), S.PY(gy), S.PZ(gy)};
  const Q4FormD f4 = q4_form_d(an.a, an.b, x, y);
  unsigned depth = 0;
  bool is_deep = false;
  for (unsigned base = 0; base < an.count && !is_deep; base += 32) {
    const unsigned i = base + lane;
    bool in = false;
    if (i < an.count) {
      const unsigned g = an.begin + i;
      const P3 pz{S.PX(g), S.PY(g), S.PZ(g)};
      const int sg = di_sign(q4_power_d(f4, pz));
      in = sg < 0 || (nonstrict && sg == 0);
    }
    depth += __popc(__ballot_sync(0xffffffffu, in));
    if (depth >= h4) is_deep = true;
  }
  if (lane == 0) deep[warp] = is_deep ? 1u : 0u;
}
#endif  // __CUDACC__

}  // namespace gpu
}  // namespace mhgp5
