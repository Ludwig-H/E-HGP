// MorseHGP3D v5 — TEMOIN DEVICE (docs/GPU.md, livraison 3). Compile par nvcc
// seulement (option CMake MHGP5_ENABLE_CUDA, sm_120, -fmad=false) ; sans
// __CUDACC__, le programme rend 2 (refus : aucun device compile).
//
// Deux lots, generes sur l'hote de facon deterministe, executes sur le
// device, rapatries et compares BIT A BIT aux resultats de l'hote :
//   1. arithmetique DI128 (add, sub, mul_i64_i64, mul_di128_i64, cmp, shl1)
//      sur 1 << 18 tirages + bords ;
//   2. scan q3 en forme de kernel (src/gpu/q3_scan_shaped.hpp) : pour chaque
//      ancre survivante d'un petit nuage (uniform n=400 et eight_clusters
//      n=400), les sites affines (SoA) et les seeds ; un warp par seed sur le
//      device (les 32 fils balaient les sites, reduction par __ballot_sync
//      pour compter les interieurs, verdict mort/vivant par seed) ; le device
//      rend aussi les compteurs de certification par seed. L'hote compare au
//      scan shaped CPU, lui-meme prouve egal a la lane de production
//      (mhgp5_q3_scan_shaped_gate).
// Codes : 0 conforme, 1 desaccord device/hote, 2 refus (pas de device, erreur
// CUDA), 3 plancher, 4 mutant tue (--inject=witness-no-warp-correction).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <random>
#include <string>
#include <vector>

#include "../cloud/families.hpp"
#include "../core/dint.hpp"
#include "../core/mutants.hpp"
#include "../lanes/edge_cover.hpp"
#include "../lanes/q3.hpp"
#include "../pipeline/float_filter.hpp"
#include "../pipeline/generate.hpp"
#include "q3_scan_kernel.cuh"
#include "q3_scan_shaped.hpp"

using namespace mhgp5;
using namespace mhgp5::gpu;

namespace {

struct ArithCase {
  i64 a, b;
  DI128 x, y;
};
struct ArithOut {
  DI128 sum, dif, mul64, mulx, shl;
  int cmp;
};

#if defined(__CUDACC__)
#define CUDA_OK(call)                                                                                   \
  do {                                                                                                  \
    cudaError_t e_ = (call);                                                                            \
    if (e_ != cudaSuccess) {                                                                            \
      std::fprintf(stderr, "REFUS cuda : %s (%s:%d)\n", cudaGetErrorString(e_), __FILE__, __LINE__);   \
      return 2;                                                                                         \
    }                                                                                                   \
  } while (0)

__global__ void k_arith(const ArithCase* in, ArithOut* out, unsigned n) {
  const unsigned i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= n) return;
  const ArithCase c = in[i];
  ArithOut o;
  o.sum = di_add(c.x, c.y);
  o.dif = di_sub(c.x, c.y);
  o.mul64 = di_mul_i64_i64(c.a, c.b);
  o.mulx = di_mul_di128_i64(c.x, c.b);
  o.shl = di_shl1(c.x);
  o.cmp = di_cmp(c.x, c.y);
  out[i] = o;
}

#endif

}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool m_warp = MHGP5_MUTANT("witness-no-warp-correction");
#if !defined(__CUDACC__)
  (void)m_warp;
  std::fprintf(stderr, "REFUS : temoin device compile sans nvcc (aucun device)\n");
  return 2;
#else
  int ndev = 0;
  if (cudaGetDeviceCount(&ndev) != cudaSuccess || ndev < 1) {
    std::fprintf(stderr, "REFUS : aucun device CUDA visible\n");
    return 2;
  }
  cudaDeviceProp prop{};
  CUDA_OK(cudaGetDeviceProperties(&prop, 0));
  std::printf("device=%s sm=%d.%d\n", prop.name, prop.major, prop.minor);
  int bad = 0;
  // ---- Lot 1 : arithmetique.
  {
    std::mt19937_64 rng(0x5eed0d1a7ull);
    const unsigned n = 1u << 18;
    std::vector<ArithCase> in(n);
    const i64 bords[] = {0, 1, -1, INT64_MAX, INT64_MIN, (i64)1 << 40, -((i64)1 << 40), 65535, -65535};
    // Tous les cas sont DANS le contrat exact de dint.hpp (docs : |x·b| < 2^127
    // pour mul_di128_i64) : mode A, x = produit de deux |·| < 2^31 (|x| < 2^62)
    // et b sur tout i64 ; mode B, x = produit de deux |·| < 2^39 (|x| < 2^78) et
    // |b| < 2^40. L'oracle hote est l'__int128 natif, sans debordement.
    const auto rnd = [&](int bits) -> i64 { return (i64)(rng() % ((u64)1 << bits)) - ((i64)1 << (bits - 1)); };
    for (unsigned i = 0; i < n; ++i) {
      ArithCase c;
      if (i < 81) {
        c.a = bords[i / 9];
        c.b = bords[i % 9];
      } else {
        const int mode = (int)(rng() % 3);
        c.a = mode == 0 ? (i64)rng() : rnd(36);
        c.b = mode == 1 ? (i64)rng() : rnd(36);
      }
      if (i & 1) {  // mode A
        c.x = di_mul_i64_i64(rnd(31), rnd(31));
        c.y = di_mul_i64_i64(rnd(31), rnd(31));
        if (i >= 81 && (rng() & 1)) c.b = (i64)rng();
      } else {  // mode B
        c.x = di_mul_i64_i64(rnd(39), rnd(39));
        c.y = di_mul_i64_i64(rnd(39), rnd(39));
        if (i >= 81) c.b = rnd(40);
        else if (c.b == INT64_MAX || c.b == INT64_MIN) c.b = rnd(40);
      }
      in[i] = c;
    }
    ArithCase* d_in = nullptr;
    ArithOut* d_out = nullptr;
    CUDA_OK(cudaMalloc(&d_in, n * sizeof(ArithCase)));
    CUDA_OK(cudaMalloc(&d_out, n * sizeof(ArithOut)));
    CUDA_OK(cudaMemcpy(d_in, in.data(), n * sizeof(ArithCase), cudaMemcpyHostToDevice));
    k_arith<<<(n + 255) / 256, 256>>>(d_in, d_out, n);
    CUDA_OK(cudaGetLastError());
    CUDA_OK(cudaDeviceSynchronize());
    std::vector<ArithOut> out(n);
    CUDA_OK(cudaMemcpy(out.data(), d_out, n * sizeof(ArithOut), cudaMemcpyDeviceToHost));
    cudaFree(d_in);
    cudaFree(d_out);
    unsigned mism = 0;
    for (unsigned i = 0; i < n; ++i) {
      const ArithCase& c = in[i];
      const ArithOut& o = out[i];
      const i128 X = di_to_i128(c.x), Y = di_to_i128(c.y);
      const bool ok = di_to_i128(o.sum) == X + Y && di_to_i128(o.dif) == X - Y &&
                      di_to_i128(o.mul64) == (i128)c.a * (i128)c.b && di_to_i128(o.shl) == X * 2 &&
                      o.cmp == (X < Y ? -1 : X > Y ? 1 : 0) && di_to_i128(o.mulx) == X * (i128)c.b;
      if (!ok) ++mism;
    }
    std::printf("arith cas=%u desaccords=%u\n", n, mism);
    if (mism) ++bad;
  }
  // ---- Lot 2 : scan q3 par warp.
  for (const CloudFamily family : {CloudFamily::kUniform, CloudFamily::kEightClusters}) {
    const CloudIndex ix = build_cloud_index(make_family_input(family, 400, 0, 3));
    if (!ix.valid) return 2;
    const u64 smax = 11;
    const u64 h_of[3] = {lane_h(Lane::kQ2, smax), lane_h(Lane::kQ3, smax), lane_h(Lane::kQ4, smax)};
    const bool float_on = float_filter_runtime_enabled();
    std::vector<AliveRect> alive;
    u64 visited = 0, workers = 0;
    generate_detail::alive_rectangles(ix, 8, h_of, 1, 1, &alive, &visited, &workers);
    std::vector<i64> U0, U1, U2, Q;
    std::vector<double> F0, F1, F2, FQ;
    std::vector<AnchorRange> ranges;
    std::vector<SeedJob> jobs;
    std::vector<SeedOut> host;
    std::vector<u64> ha, hb;
    std::vector<NodeRef> handles;
    std::vector<CoverPoint> cover, tmp;
    u64 cnodes = 0, visits = 0;
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
          const unsigned begin = (unsigned)U0.size();
          i64 qmax = 1, umax = 1;
          const i64 sx = pa.x + pb.x, sy = pa.y + pb.y, sz = pa.z + pb.z;
          for (const CoverPoint& cp : cover) {
            const P3& pz = ix.upos[(size_t)cp.u];
            const i64 u0 = 2 * pz.x - sx, u1 = 2 * pz.y - sy, u2 = 2 * pz.z - sz;
            const i64 qz = u0 * u0 + u1 * u1 + u2 * u2 - D2;
            U0.push_back(u0); U1.push_back(u1); U2.push_back(u2); Q.push_back(qz);
            F0.push_back((double)u0); F1.push_back((double)u1); F2.push_back((double)u2); FQ.push_back((double)qz);
            qmax = std::max(qmax, qz < 0 ? -qz : qz);
            umax = std::max({umax, u0 < 0 ? -u0 : u0, u1 < 0 ? -u1 : u1, u2 < 0 ? -u2 : u2});
          }
          const unsigned aidx = (unsigned)ranges.size();
          ranges.push_back(AnchorRange{begin, (unsigned)cover.size()});
          const AnchorSitesSoA sites{U0.data() + begin, U1.data() + begin, U2.data() + begin, Q.data() + begin,
                                     F0.data() + begin, F1.data() + begin, F2.data() + begin, FQ.data() + begin,
                                     (u32)cover.size()};
          for (const CoverPoint& cp : cover) {
            if (cp.u == ua || cp.u == ub) continue;
            const P3& px = ix.upos[(size_t)cp.u];
            if (!is_acute_seed(pa, pb, px, D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(cp.u))) continue;
            const Q3Form f3 = q3_form(pa, pb, px);
            const i128 N0 = f3.w[0] - f3.g * (i128)(pb.x - pa.x);
            const i128 N1 = f3.w[1] - f3.g * (i128)(pb.y - pa.y);
            const i128 N2 = f3.w[2] - f3.g * (i128)(pb.z - pa.z);
            SeedAffineD sd;
            sd.G = di_from_i128(f3.g);
            sd.N0 = di_from_i128(N0); sd.N1 = di_from_i128(N1); sd.N2 = di_from_i128(N2);
            sd.Gd = (double)f3.g; sd.Nd0 = (double)N0; sd.Nd1 = (double)N1; sd.Nd2 = (double)N2;
            sd.bound = float_on ? affine_l_bound(sd.Gd, sd.Nd0, sd.Nd1, sd.Nd2, (double)qmax, (double)umax)
                                : std::numeric_limits<double>::infinity();
            u32 cn = 0, cp2 = 0, cf = 0;
            const bool dead = q3_scan_seed_shaped(sd, sites, (u32)h_of[1], std::numeric_limits<u32>::max(), &cn, &cp2, &cf);
            jobs.push_back(SeedJob{sd, aidx});
            host.push_back(SeedOut{dead ? 1u : 0u, cn, cp2, cf});
          }
        }
    }
    // Les pointeurs de `sites` ci-dessus pointaient dans des vecteurs qui ont pu se reallouer : les
    // resultats hote ont ete calcules AVANT toute reallocation ulterieure (sites construit apres push).
    const unsigned njobs = (unsigned)jobs.size();
    if (njobs < 1000) {
      std::fprintf(stderr, "PLANCHER : %u seeds\n", njobs);
      return 3;
    }
    SeedJob* d_jobs; AnchorRange* d_ranges; i64 *d_u0, *d_u1, *d_u2, *d_q; double *d_d0, *d_d1, *d_d2, *d_dq; SeedOut* d_out;
    const size_t ns = U0.size();
    CUDA_OK(cudaMalloc(&d_jobs, njobs * sizeof(SeedJob)));
    CUDA_OK(cudaMalloc(&d_ranges, ranges.size() * sizeof(AnchorRange)));
    CUDA_OK(cudaMalloc(&d_u0, ns * sizeof(i64))); CUDA_OK(cudaMalloc(&d_u1, ns * sizeof(i64)));
    CUDA_OK(cudaMalloc(&d_u2, ns * sizeof(i64))); CUDA_OK(cudaMalloc(&d_q, ns * sizeof(i64)));
    CUDA_OK(cudaMalloc(&d_d0, ns * sizeof(double))); CUDA_OK(cudaMalloc(&d_d1, ns * sizeof(double)));
    CUDA_OK(cudaMalloc(&d_d2, ns * sizeof(double))); CUDA_OK(cudaMalloc(&d_dq, ns * sizeof(double)));
    CUDA_OK(cudaMalloc(&d_out, njobs * sizeof(SeedOut)));
    CUDA_OK(cudaMemcpy(d_jobs, jobs.data(), njobs * sizeof(SeedJob), cudaMemcpyHostToDevice));
    CUDA_OK(cudaMemcpy(d_ranges, ranges.data(), ranges.size() * sizeof(AnchorRange), cudaMemcpyHostToDevice));
    CUDA_OK(cudaMemcpy(d_u0, U0.data(), ns * sizeof(i64), cudaMemcpyHostToDevice));
    CUDA_OK(cudaMemcpy(d_u1, U1.data(), ns * sizeof(i64), cudaMemcpyHostToDevice));
    CUDA_OK(cudaMemcpy(d_u2, U2.data(), ns * sizeof(i64), cudaMemcpyHostToDevice));
    CUDA_OK(cudaMemcpy(d_q, Q.data(), ns * sizeof(i64), cudaMemcpyHostToDevice));
    CUDA_OK(cudaMemcpy(d_d0, F0.data(), ns * sizeof(double), cudaMemcpyHostToDevice));
    CUDA_OK(cudaMemcpy(d_d1, F1.data(), ns * sizeof(double), cudaMemcpyHostToDevice));
    CUDA_OK(cudaMemcpy(d_d2, F2.data(), ns * sizeof(double), cudaMemcpyHostToDevice));
    CUDA_OK(cudaMemcpy(d_dq, FQ.data(), ns * sizeof(double), cudaMemcpyHostToDevice));
    const unsigned threads = 256, warps_per_block = threads / 32;
    cudaEvent_t e0, e1;
    cudaEventCreate(&e0); cudaEventCreate(&e1);
    cudaEventRecord(e0);
    k_scan<<<(njobs + warps_per_block - 1) / warps_per_block, threads>>>(d_jobs, njobs, d_ranges, d_u0, d_u1, d_u2, d_q,
                                                                          d_d0, d_d1, d_d2, d_dq, (unsigned)h_of[1], m_warp, false, d_out);
    CUDA_OK(cudaGetLastError());
    cudaEventRecord(e1);
    CUDA_OK(cudaDeviceSynchronize());
    float ms = 0;
    cudaEventElapsedTime(&ms, e0, e1);
    std::vector<SeedOut> dev(njobs);
    CUDA_OK(cudaMemcpy(dev.data(), d_out, njobs * sizeof(SeedOut), cudaMemcpyDeviceToHost));
    cudaFree(d_jobs); cudaFree(d_ranges); cudaFree(d_u0); cudaFree(d_u1); cudaFree(d_u2); cudaFree(d_q);
    cudaFree(d_d0); cudaFree(d_d1); cudaFree(d_d2); cudaFree(d_dq); cudaFree(d_out);
    unsigned mism = 0, dead = 0;
    for (unsigned i = 0; i < njobs; ++i) {
      // Verdict ET compteurs de certification pour TOUTES les seeds, mortes
      // comprises (la correction intra-warp de k_scan est ainsi exercee).
      if (dev[i].dead != host[i].dead || dev[i].cert_neg != host[i].cert_neg || dev[i].cert_pos != host[i].cert_pos ||
          dev[i].fallback != host[i].fallback)
        ++mism;
      dead += host[i].dead;
    }
    std::printf("scan famille=%s ancres=%zu seeds=%u sites=%zu morts=%u desaccords=%u kernel_ms=%.3f\n", cloud_family_name(family),
                ranges.size(), njobs, ns, dead, mism, ms);
    if (mism) ++bad;
  }
  if (bad) {
    std::fprintf(stderr, "DESACCORD device/hote\n");
    return m_warp ? 4 : 1;
  }
  if (m_warp) {
    std::fprintf(stderr, "MUTANT NON TUE\n");
    return 1;
  }
  std::printf("device_witness OK\n");
  return 0;
#endif
}
