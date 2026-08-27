// MorseHGP3D v5 — KERNEL du scan q3 (docs/GPU.md, livraison 4) : un warp par
// seed, transcription directe de q3_scan_seed_shaped (q3_scan_shaped.hpp).
// Partage par le temoin device (device_witness.cu) et la lane q3 device
// (q3_lane_device.cuh). Compile par nvcc seulement (__CUDACC__).
//
// Contrat (identique au scalaire) : verdict mort ssi >= h3 interieurs ;
// compteurs de certification (cert_neg, cert_pos, fallback) comptes jusqu'au
// site du h3-ieme interieur inclus — la correction intra-warp retire la
// contribution des sites au-dela dans le dernier warp-pas. `nonstrict` :
// L == 0 compte comme interieur (mutant CPU genfilter-nonstrict, pour garder
// l'egalite sous mutant) ; `no_warp_correction` : mutant du temoin.
#pragma once

#include "../core/dint.hpp"
#include "../core/types.hpp"
#include "q3_scan_shaped.hpp"

namespace mhgp5 {
namespace gpu {

struct SeedJob {
  SeedAffineD seed;
  unsigned anchor;  // index de l'ancre (sites)
};
struct AnchorRange {
  unsigned begin, count;  // dans les tableaux SoA concatenes
};
struct SeedOut {
  unsigned dead, cert_neg, cert_pos, fallback;
};

#if defined(__CUDACC__)
// Un warp par seed : les 32 fils balaient les sites de l'ancre par pas de 32 ;
// compte des interieurs par __ballot_sync + __popc, arret quand le compte du
// warp atteint h3 (meme objet que la sortie anticipee scalaire : le verdict
// « >= h3 interieurs » ne depend pas de l'ordre).

__global__ void k_scan(const SeedJob* jobs, unsigned njobs, const AnchorRange* anchors, const i64* u0, const i64* u1,
                       const i64* u2, const i64* q, const double* u0d, const double* u1d, const double* u2d,
                       const double* qd, unsigned h3, bool no_warp_correction, bool nonstrict, SeedOut* out) {
  const unsigned warp = (blockIdx.x * blockDim.x + threadIdx.x) >> 5;
  const unsigned lane = threadIdx.x & 31;
  if (warp >= njobs) return;
  const SeedJob job = jobs[warp];
  const AnchorRange ar = anchors[job.anchor];
  unsigned depth = 0, cn = 0, cp = 0, cf = 0;
  bool dead = false;
  for (unsigned base = 0; base < ar.count && !dead; base += 32) {
    const unsigned i = base + lane;
    bool interior = false;
    unsigned my_n = 0, my_p = 0, my_f = 0;
    if (i < ar.count) {
      const unsigned g = ar.begin + i;
      const double t = fma(job.seed.Nd2, u2d[g], fma(job.seed.Nd1, u1d[g], job.seed.Nd0 * u0d[g]));
      const double lh = fma(job.seed.Gd, qd[g], -(t + t));
      if (lh < -job.seed.bound) {
        my_n = 1;
        interior = true;
      } else if (lh > job.seed.bound) {
        my_p = 1;
      } else {
        my_f = 1;
        const DI128 L = q3_l_exact_shaped(job.seed, u0[g], u1[g], u2[g], q[g]);
        const int sg = di_sign(L);
        interior = sg < 0 || (nonstrict && sg == 0);
      }
    }
    const unsigned mask_int = __ballot_sync(0xffffffffu, interior);
    cn += __popc(__ballot_sync(0xffffffffu, my_n != 0));
    cp += __popc(__ballot_sync(0xffffffffu, my_p != 0));
    cf += __popc(__ballot_sync(0xffffffffu, my_f != 0));
    // Compte ordonne : le nombre d'interieurs AVANT d'atteindre h3, comme le
    // scalaire (les compteurs de certification s'arretent au meme site).
    const unsigned before = depth;
    const unsigned add = __popc(mask_int);
    if (before + add >= h3) {
      // Sites au-dela du h3-ieme interieur : leurs compteurs ne sont pas vus
      // par le scalaire (sortie anticipee). Retirer leur contribution.
      // MUTANT witness-no-warp-correction (drapeau hote) : ne rien retirer —
      // les compteurs des seeds morts different alors du scalaire (code 4).
      if (no_warp_correction) {
        dead = true;
        depth = h3;
        break;
      }
      const unsigned need = h3 - before;
      unsigned m = mask_int, kth = 0;
      for (unsigned k = 0; k < need; ++k) {
        kth = __ffs(m) - 1;
        m &= m - 1;
      }
      const unsigned keep = (kth == 31) ? 0xffffffffu : ((1u << (kth + 1)) - 1u);
      cn -= __popc(__ballot_sync(0xffffffffu, my_n != 0) & ~keep);
      cp -= __popc(__ballot_sync(0xffffffffu, my_p != 0) & ~keep);
      cf -= __popc(__ballot_sync(0xffffffffu, my_f != 0) & ~keep);
      dead = true;
      depth = h3;
    } else {
      depth += add;
    }
  }
  if (lane == 0) out[warp] = SeedOut{dead ? 1u : 0u, cn, cp, cf};
}
#endif  // __CUDACC__

}  // namespace gpu
}  // namespace mhgp5
