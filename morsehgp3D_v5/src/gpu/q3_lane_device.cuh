// MorseHGP3D v5 — EXECUTEUR DEVICE de la lane q3 par lots (docs/GPU.md,
// livraison 4) : remplit le contrat de scan_q3_batch_host avec k_scan
// (q3_scan_kernel.cuh) — un lot (rectangle) = un transfert des tableaux SoA
// et des seeds, un lancement, un rapatriement des verdicts. Un executeur par
// fil d'execution (son propre flux CUDA) ; les tampons device croissent et
// sont reutilises. Compile par nvcc seulement. Toute erreur CUDA est un refus
// (exception std::runtime_error) — jamais un verdict invente.
//
// INSTRUMENT RECEVABLE (audit AUDIT_RENDEMENT_GPU_MULTICPU_20260828, § reception
// de 63deda74) : toutes les durees de DeviceExecutorStats sont des SOMMES sur
// les appels scan() de fils CONCURRENTS (`executor_ms_sum`) — aucune n'est un
// mur et aucune ne se soustrait au mur de lane (t_rects_ms, imprime
// `lane_wall_ms` par la CLI). Les etapes device (H2D, kernels, D2H) sont
// mesurees par EVENEMENTS CUDA sur le flux de l'executeur et recoltees apres
// les synchronisations qui existaient deja ; l'instrument n'ajoute aucune
// synchronisation. Les temps hote (reservations, boucles) sont des
// steady_clock entre deux points hote.
#pragma once
#include <chrono>

#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "device_stats.hpp"
#include "executor_pool.hpp"
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
  // Valide seulement apres une synchronisation qui couvre les deux evenements.
  static float ms_between(const CudaEvent& a, const CudaEvent& b) {
    float ms = 0;
    cuda_check(cudaEventElapsedTime(&ms, a.e, b.e), "elapsed");
    return ms;
  }
};

inline double ms_host_since(std::chrono::steady_clock::time_point t0) {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

// GEOMETRIE RESIDENTE (G1) : positions uniques du CloudIndex televersees UNE
// fois par lane (3 x i32 par position : 50 k -> 600 Ko, 10 M -> 120 Mo), lues
// par les kernels a wire par indices ; partagee en lecture seule par les
// executeurs du pool. RAII, jamais de fuite sur exception.
struct GpuGeometry {
  int *d_px = nullptr, *d_py = nullptr, *d_pz = nullptr;
  PointId* d_pid = nullptr;  // PointId par position (lane q4)
  size_t count = 0;
  u64 bytes = 0;
  explicit GpuGeometry(const CloudIndex& ix) {
    count = ix.upos.size();
    std::vector<int> hx(count), hy(count), hz(count);
    std::vector<PointId> hp(count);
    for (size_t i = 0; i < count; ++i) { hx[i] = (int)ix.upos[i].x; hy[i] = (int)ix.upos[i].y; hz[i] = (int)ix.upos[i].z; hp[i] = ix.point_id((i32)i); }
    try {
      cuda_check(cudaMalloc(&d_px, count * sizeof(int)), "cudaMalloc geometrie x");
      cuda_check(cudaMalloc(&d_py, count * sizeof(int)), "cudaMalloc geometrie y");
      cuda_check(cudaMalloc(&d_pz, count * sizeof(int)), "cudaMalloc geometrie z");
      cuda_check(cudaMemcpy(d_px, hx.data(), count * sizeof(int), cudaMemcpyHostToDevice), "geometrie x");
      cuda_check(cudaMemcpy(d_py, hy.data(), count * sizeof(int), cudaMemcpyHostToDevice), "geometrie y");
      cuda_check(cudaMemcpy(d_pz, hz.data(), count * sizeof(int), cudaMemcpyHostToDevice), "geometrie z");
      cuda_check(cudaMalloc(&d_pid, count * sizeof(PointId)), "cudaMalloc geometrie pid");
      cuda_check(cudaMemcpy(d_pid, hp.data(), count * sizeof(PointId), cudaMemcpyHostToDevice), "geometrie pid");
    } catch (...) {
      release();
      throw;
    }
    bytes = 3 * count * sizeof(int) + count * sizeof(PointId);
  }
  ~GpuGeometry() { release(); }
  GpuGeometry(const GpuGeometry&) = delete;
  GpuGeometry& operator=(const GpuGeometry&) = delete;
  GeomDev dev() const { return GeomDev{d_px, d_py, d_pz}; }
 private:
  void release() {
    if (d_px) cudaFree(d_px);
    if (d_py) cudaFree(d_py);
    if (d_pz) cudaFree(d_pz);
    if (d_pid) cudaFree(d_pid);
    d_px = d_py = d_pz = nullptr;
    d_pid = nullptr;
  }
};

class Q3DeviceExecutor {
 public:
  Q3DeviceExecutor() { cuda_check(cudaStreamCreate(&stream_), "stream"); }
  ExecutorLifecycle::Scope lifecycle_;  // construction -> destruction, cumule dans ExecutorLifecycle::global()
  ~Q3DeviceExecutor() {
    release();
    cudaStreamDestroy(stream_);
  }
  Q3DeviceExecutor(const Q3DeviceExecutor&) = delete;
  Q3DeviceExecutor& operator=(const Q3DeviceExecutor&) = delete;

  DeviceExecutorStats total;  // cumul de cet executeur (un fil)
  inline static ConcurrencyGauge gauge;  // partage par tous les executeurs q3 du processus

  // `d` (facultatif) recoit les mesures DE CE LOT (ajoutees) ; `total` les cumule toujours.
  // `geom` (facultatif) + `wire_index` : chemin G1 (indices u32 + geometrie residente) ; sinon wire SoA.
  void scan(Q3Batch* b, u32 h3, bool nonstrict, DeviceExecutorStats* d = nullptr, const GpuGeometry* geom = nullptr,
            bool wire_index = false) {
    if (broken_) throw std::runtime_error("cuda : executeur q3 inutilisable apres une erreur d'allocation");
    std::string why;
    // Wire par indices : les VALEURS des indices sont bornees par la geometrie
    // residente AVANT tout lancement (fail-closed).
    if (!validate_q3_batch(*b, &why, wire_index && geom ? (long long)geom->count : -1)) throw std::invalid_argument(why);
    const size_t ns = b->u0.size(), nj = b->seeds.size(), na = b->anchors.size();
    b->verdicts.resize(nj);
    DeviceExecutorStats m;
    m.lots = 1;
    if (nj == 0) {
      total.add(m);
      if (d) d->add(m);
      return;
    }
    const ConcurrencyGauge::Scope active(gauge);
    const auto t_scan = std::chrono::steady_clock::now();
    {
      const auto t_r = std::chrono::steady_clock::now();
      reserve(ns, nj, na);
      m.reserve_ms += ms_host_since(t_r);
    }
    static_assert(sizeof(Q3BatchSeed) == sizeof(SeedJob), "Q3BatchSeed et SeedJob doivent avoir le meme layout");
    static_assert(sizeof(Q3BatchAnchor) == sizeof(AnchorRange), "layout des ancres");
    static_assert(sizeof(Q3BatchVerdict) == sizeof(SeedOut), "layout des verdicts");
    static_assert(sizeof(Q3BatchAnchorGeom) == sizeof(AnchorGeom), "layout de la geometrie d'ancre");
    // MUTANT `wire-index-force-soa` : le wire index demande retombe
    // silencieusement sur SoA — verdicts et digests resteraient verts ; seuls
    // les compteurs de branche le voient.
    const bool use_idx = wire_index && !MHGP5_MUTANT("wire-index-force-soa") && geom && b->site_index.size() == ns && b->ageom.size() == na;
    if (wire_index && !use_idx && !MHGP5_MUTANT("wire-index-force-soa"))
      throw std::invalid_argument("lot q3 : wire par indices demande sans indices/geometrie complets");
    if (use_idx) { m.index_lots = 1; m.site_index_bytes += ns * sizeof(u32) + na * sizeof(Q3BatchAnchorGeom); }
    else { m.soa_lots = 1; m.site_soa_bytes += ns * 4 * sizeof(i64); }
    const auto t_issue = std::chrono::steady_clock::now();
    ev_h0_.record(stream_);
    if (use_idx) {
      m.h2d_bytes += up(d_idx_, b->site_index.data(), ns);
      m.h2d_bytes += up(d_ageom_, reinterpret_cast<const AnchorGeom*>(b->ageom.data()), na);
    } else {
      m.h2d_bytes += up(d_u0_, b->u0.data(), ns) + up(d_u1_, b->u1.data(), ns) + up(d_u2_, b->u2.data(), ns) + up(d_q_, b->q.data(), ns);
    }
    m.h2d_bytes += up(d_jobs_, reinterpret_cast<const SeedJob*>(b->seeds.data()), nj);
    m.h2d_bytes += up(d_anchors_, reinterpret_cast<const AnchorRange*>(b->anchors.data()), na);
    ev_h1_.record(stream_);
    const unsigned threads = 256, wpb = threads / 32;
    const unsigned blocks = (unsigned)((nj + wpb - 1) / wpb);
    if (use_idx)
      k_scan_idx<<<blocks, threads, 0, stream_>>>(d_jobs_, (unsigned)nj, d_anchors_, d_idx_, geom->dev(), d_ageom_, h3, false, nonstrict, d_out_);
    else
      k_scan<<<blocks, threads, 0, stream_>>>(d_jobs_, (unsigned)nj, d_anchors_, d_u0_, d_u1_, d_u2_, d_q_, h3, false, nonstrict, d_out_);
    cuda_check(cudaGetLastError(), "lancement k_scan");
    m.launches += 1;
    ev_k1_.record(stream_);
    cuda_check(cudaMemcpyAsync(b->verdicts.data(), d_out_, nj * sizeof(SeedOut), cudaMemcpyDeviceToHost, stream_), "verdicts");
    m.d2h_bytes += nj * sizeof(SeedOut);
    ev_d1_.record(stream_);
    m.issue_ms += ms_host_since(t_issue);
    {
      const auto t_w = std::chrono::steady_clock::now();
      cuda_check(cudaStreamSynchronize(stream_), "synchronisation");  // la synchronisation du contrat (deja presente)
      m.wait_ms += ms_host_since(t_w);
    }
    // Recolte des evenements APRES la synchronisation existante.
    m.h2d_ms += CudaEvent::ms_between(ev_h0_, ev_h1_);
    m.k1_ms += CudaEvent::ms_between(ev_h1_, ev_k1_);
    m.d2h1_ms += CudaEvent::ms_between(ev_k1_, ev_d1_);
    m.executor_ms_sum += ms_host_since(t_scan);
    total.add(m);
    if (d) d->add(m);
  }

 private:
  cudaStream_t stream_{};
  CudaEvent ev_h0_, ev_h1_, ev_k1_, ev_d1_;  // crees une fois par executeur (pas par lot)
  i64 *d_u0_ = nullptr, *d_u1_ = nullptr, *d_u2_ = nullptr, *d_q_ = nullptr;
  unsigned* d_idx_ = nullptr;      // wire G1 (capacite cap_sites_)
  AnchorGeom* d_ageom_ = nullptr;  // wire G1 (capacite cap_anchors_)
  SeedJob* d_jobs_ = nullptr;
  AnchorRange* d_anchors_ = nullptr;
  SeedOut* d_out_ = nullptr;
  size_t cap_sites_ = 0, cap_jobs_ = 0, cap_anchors_ = 0;

  // Copie H2D asynchrone ; rend les octets enfiles.
  template <class T>
  u64 up(T* dst, const T* src, size_t n) {
    cuda_check(cudaMemcpyAsync(dst, src, n * sizeof(T), cudaMemcpyHostToDevice, stream_), "transfert");
    return (u64)n * sizeof(T);
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
      unsigned* idx = nullptr;
      AnchorGeom* ageom = nullptr;
      SeedJob* jobs = nullptr;
      SeedOut* out = nullptr;
      AnchorRange* anchors = nullptr;
      if (gs) { u0 = t.alloc<i64>(cs); u1 = t.alloc<i64>(cs); u2 = t.alloc<i64>(cs); q = t.alloc<i64>(cs); idx = t.alloc<unsigned>(cs); }
      if (gj) { jobs = t.alloc<SeedJob>(cj); out = t.alloc<SeedOut>(cj); }
      if (ga) { anchors = t.alloc<AnchorRange>(ca); ageom = t.alloc<AnchorGeom>(ca); }
      // Tout est alloue : echange.
      if (gs) { swap_in(&d_u0_, u0); swap_in(&d_u1_, u1); swap_in(&d_u2_, u2); swap_in(&d_q_, q); swap_in(&d_idx_, idx); cap_sites_ = cs; }
      if (gj) { swap_in(&d_jobs_, jobs); swap_in(&d_out_, out); cap_jobs_ = cj; }
      if (ga) { swap_in(&d_anchors_, anchors); swap_in(&d_ageom_, ageom); cap_anchors_ = ca; }
      t.commit();
    } catch (...) {
      broken_ = true;
      throw;
    }
  }
  void release() {
    for (void* p : {(void*)d_u0_, (void*)d_u1_, (void*)d_u2_, (void*)d_q_, (void*)d_idx_, (void*)d_ageom_, (void*)d_jobs_, (void*)d_anchors_, (void*)d_out_})
      if (p) cudaFree(p);
  }
};

// Lane q3 complete avec l'executeur device : un executeur par fil (thread_local).
// `kernel_ms` recoit la somme des durees d'evenements des KERNELS SEULS (q3 :
// k_scan) sur tous les fils ; `stages` (facultatif) recoit le cumul complet
// (DeviceExecutorStats) — sommes de temps-executeur, jamais un mur.
inline void generate_q3_device(const CloudIndex& ix, const GenerateOptions& opt, std::vector<BallCandidate>* out,
                               GenerateStats* st, double* kernel_ms, u64* launches, BatchLimits lim = BatchLimits{},
                               BatchStats* bs = nullptr, DeviceExecutorStats* stages = nullptr) {
  std::mutex mu;
  Q3DeviceExecutor::gauge.reset_peak();
  // G0 : pool BORNE et PERSISTANT (lim.gpu_executors executeurs crees une fois pour la lane, file avec
  // contre-pression ; les producteurs CPU attendent leur lot, donc l'ordre d'emission par ouvrier est inchange).
  ExecutorPool<Q3DeviceExecutor> pool(lim.gpu_executors);
  // G1 : geometrie residente televersee UNE fois par lane quand le wire par indices est demande.
  std::unique_ptr<GpuGeometry> geom;
  if (lim.wire_index) {
    geom = std::make_unique<GpuGeometry>(ix);
    if (stages) stages->h2d_bytes += geom->bytes;
  }
  generate_q3_batched_with(ix, opt, out, st, [&](Q3Batch* b, u32 h3, bool nonstrict) {
    DeviceExecutorStats d;
    pool.submit_and_wait([&](Q3DeviceExecutor& ex) { ex.scan(b, h3, nonstrict, &d, geom.get(), lim.wire_index); });
    std::lock_guard<std::mutex> lk(mu);
    *kernel_ms += d.kernel_ms();
    *launches += d.launches;
    if (stages) stages->add(d);
  }, lim, bs);
  if (stages) {
    const u32 p = Q3DeviceExecutor::gauge.read_peak();
    if (p > stages->peak_concurrent) stages->peak_concurrent = p;
  }
}
#endif  // __CUDACC__

}  // namespace gpu
}  // namespace mhgp5
