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
#include "q4_core_shaped.hpp"
#include "q4_lane_batched.hpp"

namespace mhgp5 {
namespace gpu {

#if defined(__CUDACC__)
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

__global__ void k_q4_core(const Q4BatchSeed* seeds, unsigned nseeds, const Q4BatchAnchor* anchors, Q4SitesDev S,
                          unsigned h4, bool nonstrict, Q4SeedVerdict* out) {
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
  unsigned fcount = 0;
  bool dead = false;
  for (unsigned base = 0; base < an.count && !dead; base += 32) {
    const unsigned i = base + lane;
    // Classification du site de la lane : un bit par compteur, un bit temoin.
    unsigned my_pos = 0, my_neg = 0, my_kill = 0, my_skip = 0, my_jf = 0, my_ff = 0, my_wit = 0;
    if (i < an.count && i != an.skip_a && i != an.skip_b && i != sd.core.skip_x) {
      const unsigned g = an.begin + i;
      const double lh = q3_l_hat_shaped(sd.core.aff, (double)S.u0[g], (double)S.u1[g], (double)S.u2[g], (double)S.q[g]);
      if (lh > sd.core.aff.bound) {
        my_pos = 1;
      } else {
        const i64 nu = sd.core.n0 * S.u0[g] + sd.core.n1 * S.u1[g] + sd.core.n2 * S.u2[g];
        const i64 Bz = nu / 2;
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
            const DI128 P = di_div_by_4_exact(q3_l_exact_shaped(sd.core.aff, S.u0[g], S.u1[g], S.u2[g], S.q[g]));
            const int cm = cmp_2p2_jb2(di_to_i128_hd(P), J, Bz);
            my_wit = (nonstrict ? (cm >= 0) : (cm > 0)) ? 1 : 0;
          }
        } else {
          my_ff = 1;
          const DI128 P = di_div_by_4_exact(q3_l_exact_shaped(sd.core.aff, S.u0[g], S.u1[g], S.u2[g], S.q[g]));
          const int sg = di_sign(P);
          if (!(nonstrict ? (sg > 0) : (sg >= 0))) {
            const int cm = cmp_2p2_jb2(di_to_i128_hd(P), J, Bz);
            my_wit = (nonstrict ? (cm >= 0) : (cm > 0)) ? 1 : 0;
          }
        }
      }
    }
    const unsigned wit = __ballot_sync(0xffffffffu, my_wit != 0);
    unsigned keep = 0xffffffffu;
    const unsigned add = __popc(wit);
    if (fcount + add >= h4) {
      keep = keep_mask_upto(wit, h4 - fcount);
      dead = true;
      fcount = h4;
    } else {
      fcount += add;
    }
    v.c.cert_pos += __popc(__ballot_sync(0xffffffffu, my_pos != 0) & keep);
    v.c.cert_neg += __popc(__ballot_sync(0xffffffffu, my_neg != 0) & keep);
    v.c.jung_kill += __popc(__ballot_sync(0xffffffffu, my_kill != 0) & keep);
    v.c.jung_skip += __popc(__ballot_sync(0xffffffffu, my_skip != 0) & keep);
    v.c.jung_fallback += __popc(__ballot_sync(0xffffffffu, my_jf != 0) & keep);
    v.c.float_fallback += __popc(__ballot_sync(0xffffffffu, my_ff != 0) & keep);
  }
  v.dead = dead ? 1u : 0u;
  if (lane == 0) out[warp] = v;
}

// Un bloc par seed VIVANT (alive[blockIdx.x] = index du seed) ; les fils
// parcourent la lentille ; stage[pair_off[blockIdx.x] + li] = etage.
__global__ void k_q4_complete(const Q4BatchSeed* seeds, const Q4BatchAnchor* anchors, Q4SitesDev S, const u32* alive,
                              const u32* pair_off, bool no_canonical, u8* stage) {
  const Q4BatchSeed sd = seeds[alive[blockIdx.x]];
  const Q4BatchAnchor an = anchors[sd.anchor];
  const unsigned gx = an.begin + sd.x_site;
  const P3 x{S.px[gx], S.py[gx], S.pz[gx]};
  const PointId idx = S.pid[gx];
  const u32 base = pair_off[blockIdx.x];
  for (unsigned li = threadIdx.x; li < an.lens_count; li += blockDim.x) {
    const u32 ys = S.lens_sites[an.lens_begin + li];
    u8 st = 255;  // paire ignoree (x, a ou b)
    if (ys != sd.x_site && ys != an.skip_a && ys != an.skip_b) {
      const unsigned gy = an.begin + ys;
      const P3 y{S.px[gy], S.py[gy], S.pz[gy]};
      Q4FormD f4;
      st = (u8)q4_completion_stage_shaped(an.a, an.b, x, y, an.ida, an.idb, idx, S.pid[gy], an.D2, sd.l_ax, sd.l_bx,
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
  const P3 x{S.px[gx], S.py[gx], S.pz[gx]};
  const P3 y{S.px[gy], S.py[gy], S.pz[gy]};
  const Q4FormD f4 = q4_form_d(an.a, an.b, x, y);
  unsigned depth = 0;
  bool is_deep = false;
  for (unsigned base = 0; base < an.count && !is_deep; base += 32) {
    const unsigned i = base + lane;
    bool in = false;
    if (i < an.count) {
      const unsigned g = an.begin + i;
      const P3 pz{S.px[g], S.py[g], S.pz[g]};
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
