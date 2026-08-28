// MorseHGP3D v5 — EXECUTEUR DEVICE de la lane q3 par lots (docs/GPU.md,
// livraison 4) : remplit le contrat de scan_q3_batch_host avec k_scan
// (q3_scan_kernel.cuh) — un lot (rectangle) = un transfert des tableaux SoA
// et des seeds, un lancement, un rapatriement des verdicts. Un executeur par
// fil d'execution (son propre flux CUDA) ; les tampons device croissent et
// sont reutilises. Compile par nvcc seulement. Toute erreur CUDA est un refus
// (exception std::runtime_error) — jamais un verdict invente.
#pragma once
#include <chrono>

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

// Evenement CUDA sous RAII (jamais fuite sur exception).
struct CudaEvent {
  cudaEvent_t e{};
  CudaEvent() { cuda_check(cudaEventCreate(&e), "event"); }
  ~CudaEvent() { cudaEventDestroy(e); }
  CudaEvent(const CudaEvent&) = delete;
  CudaEvent& operator=(const CudaEvent&) = delete;
  void record(cudaStream_t s) { cuda_check(cudaEventRecord(e, s), "record"); }
  static float ms_between(const CudaEvent& a, const CudaEvent& b) {
    float ms = 0;
    cuda_check(cudaEventElapsedTime(&ms, a.e, b.e), "elapsed");
    return ms;
  }
};

class Q3DeviceExecutor {
 public:
  Q3DeviceExecutor() { cuda_check(cudaStreamCreate(&stream_), "stream"); }
  ~Q3DeviceExecutor() {
    release();
    cudaStreamDestroy(stream_);
  }
  Q3DeviceExecutor(const Q3DeviceExecutor&) = delete;
  Q3DeviceExecutor& operator=(const Q3DeviceExecutor&) = delete;

  double kernel_ms_total = 0;  // mur kernel (evenements sur le flux de l'executeur)
  double h2d_ms_total = 0, d2h_ms_total = 0;  // murs transferts, separes (audit : jamais un cumul indistinct)
  double wall_ms_total = 0;                    // mur HOTE de scan() entier (transferts + kernel + synchronisations)
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
    const auto t_wall = std::chrono::steady_clock::now();
    reserve(ns, nj, na);
    static_assert(sizeof(Q3BatchSeed) == sizeof(SeedJob), "Q3BatchSeed et SeedJob doivent avoir le meme layout");
    static_assert(sizeof(Q3BatchAnchor) == sizeof(AnchorRange), "layout des ancres");
    static_assert(sizeof(Q3BatchVerdict) == sizeof(SeedOut), "layout des verdicts");
    CudaEvent e_h0, e_h1, e_k1, e_d1;
    e_h0.record(stream_);
    up(d_u0_, b->u0.data(), ns); up(d_u1_, b->u1.data(), ns); up(d_u2_, b->u2.data(), ns); up(d_q_, b->q.data(), ns);
    up(d_jobs_, reinterpret_cast<const SeedJob*>(b->seeds.data()), nj);
    up(d_anchors_, reinterpret_cast<const AnchorRange*>(b->anchors.data()), na);
    e_h1.record(stream_);
    const unsigned threads = 256, wpb = threads / 32;
    const unsigned blocks = (unsigned)((nj + wpb - 1) / wpb);
    k_scan<<<blocks, threads, 0, stream_>>>(d_jobs_, (unsigned)nj, d_anchors_, d_u0_, d_u1_, d_u2_, d_q_, h3, false, nonstrict,
                                            d_out_);
    cuda_check(cudaGetLastError(), "lancement k_scan");
    e_k1.record(stream_);
    cuda_check(cudaMemcpyAsync(b->verdicts.data(), d_out_, nj * sizeof(SeedOut), cudaMemcpyDeviceToHost, stream_), "verdicts");
    e_d1.record(stream_);
    cuda_check(cudaStreamSynchronize(stream_), "synchronisation");
    h2d_ms_total += CudaEvent::ms_between(e_h0, e_h1);
    kernel_ms_total += CudaEvent::ms_between(e_h1, e_k1);
    d2h_ms_total += CudaEvent::ms_between(e_k1, e_d1);
    wall_ms_total += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t_wall).count();
    ++launches;
  }

 private:
  cudaStream_t stream_{};
  i64 *d_u0_ = nullptr, *d_u1_ = nullptr, *d_u2_ = nullptr, *d_q_ = nullptr;
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
      SeedJob* jobs = nullptr;
      SeedOut* out = nullptr;
      AnchorRange* anchors = nullptr;
      if (gs) { u0 = t.alloc<i64>(cs); u1 = t.alloc<i64>(cs); u2 = t.alloc<i64>(cs); q = t.alloc<i64>(cs); }
      if (gj) { jobs = t.alloc<SeedJob>(cj); out = t.alloc<SeedOut>(cj); }
      if (ga) { anchors = t.alloc<AnchorRange>(ca); }
      // Tout est alloue : echange.
      if (gs) { swap_in(&d_u0_, u0); swap_in(&d_u1_, u1); swap_in(&d_u2_, u2); swap_in(&d_q_, q); cap_sites_ = cs; }
      if (gj) { swap_in(&d_jobs_, jobs); swap_in(&d_out_, out); cap_jobs_ = cj; }
      if (ga) { swap_in(&d_anchors_, anchors); cap_anchors_ = ca; }
      t.commit();
    } catch (...) {
      broken_ = true;
      throw;
    }
  }
  void release() {
    for (void* p : {(void*)d_u0_, (void*)d_u1_, (void*)d_u2_, (void*)d_q_, (void*)d_jobs_, (void*)d_anchors_, (void*)d_out_})
      if (p) cudaFree(p);
  }
};

// Lane q3 complete avec l'executeur device : un executeur par fil (thread_local).
// MURS PAR ETAPE d'un executeur device (cumul sur les fils ; instrument de
// mesure, jamais un claim) : la somme des etapes est le mur de scan(), a
// comparer au mur de la lane (rects) qui contient en plus l'assemblage hote.
struct DeviceStageMs {
  double h2d = 0, k1 = 0, d2h1 = 0, host1 = 0, k2 = 0, d2h2 = 0, host2 = 0, k3 = 0, d2h3 = 0, wall = 0;
  void add(const DeviceStageMs& o) {
    h2d += o.h2d; k1 += o.k1; d2h1 += o.d2h1; host1 += o.host1; k2 += o.k2; d2h2 += o.d2h2; host2 += o.host2; k3 += o.k3; d2h3 += o.d2h3; wall += o.wall;
  }
};

inline void generate_q3_device(const CloudIndex& ix, const GenerateOptions& opt, std::vector<BallCandidate>* out,
                               GenerateStats* st, double* kernel_ms, u64* launches, BatchLimits lim = BatchLimits{},
                               BatchStats* bs = nullptr, DeviceStageMs* stages = nullptr) {
  std::mutex mu;
  generate_q3_batched_with(ix, opt, out, st, [&](Q3Batch* b, u32 h3, bool nonstrict) {
    thread_local Q3DeviceExecutor ex;
    const double before_ms = ex.kernel_ms_total, before_h2d = ex.h2d_ms_total, before_d2h = ex.d2h_ms_total, before_wall = ex.wall_ms_total;
    const u64 before_l = ex.launches;
    ex.scan(b, h3, nonstrict);
    std::lock_guard<std::mutex> lk(mu);
    *kernel_ms += ex.kernel_ms_total - before_ms;
    *launches += ex.launches - before_l;
    if (stages) {
      DeviceStageMs d;
      d.h2d = ex.h2d_ms_total - before_h2d;
      d.k1 = ex.kernel_ms_total - before_ms;
      d.d2h1 = ex.d2h_ms_total - before_d2h;
      d.wall = ex.wall_ms_total - before_wall;
      stages->add(d);
    }
  }, lim, bs);
}
#endif  // __CUDACC__

}  // namespace gpu
}  // namespace mhgp5
