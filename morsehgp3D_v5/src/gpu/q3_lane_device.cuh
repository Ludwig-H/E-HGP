// MorseHGP3D v5 — EXECUTEUR DEVICE de la lane q3 par lots (docs/GPU.md,
// livraison 4) : remplit le contrat de scan_q3_batch_host avec k_scan
// (q3_scan_kernel.cuh) — un lot (rectangle) = un transfert des tableaux SoA
// et des seeds, un lancement, un rapatriement des verdicts. Un executeur par
// fil d'execution (son propre flux CUDA) ; les tampons device croissent et
// sont reutilises. Compile par nvcc seulement. Toute erreur CUDA est un refus
// (exception std::runtime_error) — jamais un verdict invente.
#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "q3_lane_batched.hpp"
#include "q3_scan_kernel.cuh"

namespace mhgp5 {
namespace gpu {

#if defined(__CUDACC__)
inline void cuda_check(cudaError_t e, const char* what) {
  if (e != cudaSuccess) throw std::runtime_error(std::string("cuda : ") + what + " : " + cudaGetErrorString(e));
}

class Q3DeviceExecutor {
 public:
  Q3DeviceExecutor() { cuda_check(cudaStreamCreate(&stream_), "stream"); }
  ~Q3DeviceExecutor() {
    release();
    cudaStreamDestroy(stream_);
  }
  Q3DeviceExecutor(const Q3DeviceExecutor&) = delete;
  Q3DeviceExecutor& operator=(const Q3DeviceExecutor&) = delete;

  double kernel_ms_total = 0;
  u64 launches = 0;  // kernels lances (q3 : un par lot)
  u64 lots = 0;      // lots scannes

  void scan(Q3Batch* b, u32 h3, bool nonstrict) {
    if (broken_) throw std::runtime_error("cuda : executeur q3 inutilisable apres une erreur d'allocation");
    std::string why;
    if (!validate_q3_batch(*b, &why)) throw std::invalid_argument(why);
    const size_t ns = b->u0.size(), nj = b->seeds.size(), na = b->anchors.size();
    b->verdicts.resize(nj);
    ++lots;
    if (nj == 0) return;
    reserve(ns, nj, na);
    static_assert(sizeof(Q3BatchSeed) == sizeof(SeedJob), "Q3BatchSeed et SeedJob doivent avoir le meme layout");
    static_assert(sizeof(Q3BatchAnchor) == sizeof(AnchorRange), "layout des ancres");
    static_assert(sizeof(Q3BatchVerdict) == sizeof(SeedOut), "layout des verdicts");
    up(d_u0_, b->u0.data(), ns); up(d_u1_, b->u1.data(), ns); up(d_u2_, b->u2.data(), ns); up(d_q_, b->q.data(), ns);
    up(d_d0_, b->u0d.data(), ns); up(d_d1_, b->u1d.data(), ns); up(d_d2_, b->u2d.data(), ns); up(d_dq_, b->qd.data(), ns);
    up(d_jobs_, reinterpret_cast<const SeedJob*>(b->seeds.data()), nj);
    up(d_anchors_, reinterpret_cast<const AnchorRange*>(b->anchors.data()), na);
    const unsigned threads = 256, wpb = threads / 32;
    const unsigned blocks = (unsigned)((nj + wpb - 1) / wpb);
    cudaEvent_t e0, e1;
    cuda_check(cudaEventCreate(&e0), "event"); cuda_check(cudaEventCreate(&e1), "event");
    cuda_check(cudaEventRecord(e0, stream_), "record");
    k_scan<<<blocks, threads, 0, stream_>>>(d_jobs_, (unsigned)nj, d_anchors_, d_u0_, d_u1_, d_u2_, d_q_, d_d0_, d_d1_, d_d2_,
                                            d_dq_, h3, false, nonstrict, d_out_);
    cuda_check(cudaGetLastError(), "lancement k_scan");
    cuda_check(cudaEventRecord(e1, stream_), "record");
    cuda_check(cudaMemcpyAsync(b->verdicts.data(), d_out_, nj * sizeof(SeedOut), cudaMemcpyDeviceToHost, stream_), "verdicts");
    cuda_check(cudaStreamSynchronize(stream_), "synchronisation");
    float ms = 0;
    cudaEventElapsedTime(&ms, e0, e1);
    cudaEventDestroy(e0); cudaEventDestroy(e1);
    kernel_ms_total += ms;
    ++launches;
  }

 private:
  cudaStream_t stream_{};
  i64 *d_u0_ = nullptr, *d_u1_ = nullptr, *d_u2_ = nullptr, *d_q_ = nullptr;
  double *d_d0_ = nullptr, *d_d1_ = nullptr, *d_d2_ = nullptr, *d_dq_ = nullptr;
  SeedJob* d_jobs_ = nullptr;
  AnchorRange* d_anchors_ = nullptr;
  SeedOut* d_out_ = nullptr;
  size_t cap_sites_ = 0, cap_jobs_ = 0, cap_anchors_ = 0;

  template <class T>
  void up(T* dst, const T* src, size_t n) {
    cuda_check(cudaMemcpyAsync(dst, src, n * sizeof(T), cudaMemcpyHostToDevice, stream_), "transfert");
  }
  bool broken_ = false;
  // RESERVE TRANSACTIONNELLE : les nouveaux tampons sont alloues dans des
  // temporaires ; en cas d'echec, ils sont liberes, l'instance devient
  // inutilisable (broken_) et l'exception est relancee — ni capacite
  // mensongere ni pointeurs partiels.
  struct Tmp {
    std::vector<void*> ptrs;
    ~Tmp() {
      for (void* p : ptrs) cudaFree(p);
    }
    template <class T>
    T* alloc(size_t n) {
      T* p = nullptr;
      cuda_check(cudaMalloc(&p, n * sizeof(T)), "cudaMalloc");
      ptrs.push_back(p);
      return p;
    }
    void commit() { ptrs.clear(); }
  };
  template <class T>
  static void swap_in(T** dst, T* fresh) {
    if (*dst) cudaFree(*dst);
    *dst = fresh;
  }
  void reserve(size_t ns, size_t nj, size_t na) {
    try {
      Tmp t;
      const bool gs = ns > cap_sites_, gj = nj > cap_jobs_, ga = na > cap_anchors_;
      const size_t cs = gs ? ns + ns / 2 : 0, cj = gj ? nj + nj / 2 : 0, ca = ga ? na + na / 2 : 0;
      i64 *u0 = nullptr, *u1 = nullptr, *u2 = nullptr, *q = nullptr;
      double *d0 = nullptr, *d1 = nullptr, *d2 = nullptr, *dq = nullptr;
      SeedJob* jobs = nullptr;
      SeedOut* out = nullptr;
      AnchorRange* anchors = nullptr;
      if (gs) { u0 = t.alloc<i64>(cs); u1 = t.alloc<i64>(cs); u2 = t.alloc<i64>(cs); q = t.alloc<i64>(cs);
                d0 = t.alloc<double>(cs); d1 = t.alloc<double>(cs); d2 = t.alloc<double>(cs); dq = t.alloc<double>(cs); }
      if (gj) { jobs = t.alloc<SeedJob>(cj); out = t.alloc<SeedOut>(cj); }
      if (ga) { anchors = t.alloc<AnchorRange>(ca); }
      // Tout est alloue : echange.
      if (gs) { swap_in(&d_u0_, u0); swap_in(&d_u1_, u1); swap_in(&d_u2_, u2); swap_in(&d_q_, q);
                swap_in(&d_d0_, d0); swap_in(&d_d1_, d1); swap_in(&d_d2_, d2); swap_in(&d_dq_, dq); cap_sites_ = cs; }
      if (gj) { swap_in(&d_jobs_, jobs); swap_in(&d_out_, out); cap_jobs_ = cj; }
      if (ga) { swap_in(&d_anchors_, anchors); cap_anchors_ = ca; }
      t.commit();
    } catch (...) {
      broken_ = true;
      throw;
    }
  }
  void release() {
    for (void* p : {(void*)d_u0_, (void*)d_u1_, (void*)d_u2_, (void*)d_q_, (void*)d_d0_, (void*)d_d1_, (void*)d_d2_,
                    (void*)d_dq_, (void*)d_jobs_, (void*)d_anchors_, (void*)d_out_})
      if (p) cudaFree(p);
  }
};

// Lane q3 complete avec l'executeur device : un executeur par fil (thread_local).
inline void generate_q3_device(const CloudIndex& ix, const GenerateOptions& opt, std::vector<BallCandidate>* out,
                               GenerateStats* st, double* kernel_ms, u64* launches, size_t seeds_per_launch = kSeedsPerLaunch,
                               BatchStats* bs = nullptr) {
  std::mutex mu;
  generate_q3_batched_with(ix, opt, out, st, [&](Q3Batch* b, u32 h3, bool nonstrict) {
    thread_local Q3DeviceExecutor ex;
    const double before_ms = ex.kernel_ms_total;
    const u64 before_l = ex.launches;
    ex.scan(b, h3, nonstrict);
    std::lock_guard<std::mutex> lk(mu);
    *kernel_ms += ex.kernel_ms_total - before_ms;
    *launches += ex.launches - before_l;
  }, seeds_per_launch, bs);
}
#endif  // __CUDACC__

}  // namespace gpu
}  // namespace mhgp5
