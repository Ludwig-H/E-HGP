// MorseHGP3D v5 — EXECUTEUR DEVICE de la lane q4 par lots (docs/GPU.md,
// livraison 5b) : remplit le contrat de scan_q4_batch_host avec les trois
// kernels de q4_kernels.cuh. Un executeur par fil (flux propre, tampons
// croissants). Entre les kernels, l'hote calcule les offsets de paires des
// seeds vivants, compte les etages et compacte les paires candidates ; les
// verdicts, etages et profondeurs rapatries sont exactement ceux que
// scan_q4_batch_host produirait (porte tests/q4_lane_device_gate.cu). Toute
// erreur CUDA est une exception, jamais un verdict.
#pragma once
#include <chrono>

#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "q3_lane_device.cuh"
#include "q4_kernels.cuh"
#include "q4_lane_batched.hpp"

namespace mhgp5 {
namespace gpu {

#if defined(__CUDACC__)
// INSTRUMENT (voir q3_lane_device.cuh) : evenements par etape sur le flux —
// H2D initial | K1 | D2H verdicts || H2D vivants/offsets | K2 | D2H etages ||
// H2D candidates | K3 | D2H profondeurs — recoltes apres les synchronisations
// qui existaient au pin 82f613d3 (K1, K2, K3) ; la barriere "sync H2D" de
// 63deda74 est RETIREE et la "sync fin" (flux deja vide) aussi. Les temps hote
// (boucle des vivants, compaction, emissions) et les reservations sont des
// steady_clock entre deux points hote. Sommes sur les fils, jamais un mur.
class Q4DeviceExecutor {
 public:
  Q4DeviceExecutor() { cuda_check(cudaStreamCreate(&stream_), "stream"); }
  ~Q4DeviceExecutor() {
    release();
    cudaStreamDestroy(stream_);
  }
  Q4DeviceExecutor(const Q4DeviceExecutor&) = delete;
  Q4DeviceExecutor& operator=(const Q4DeviceExecutor&) = delete;
  DeviceExecutorStats total;  // cumul de cet executeur (un fil)
  inline static ConcurrencyGauge gauge;  // partage par tous les executeurs q4 du processus
  bool chord_nonstrict_ = MHGP5_MUTANT("chord-nonstrict");  // drapeau hote passe au kernel

  // `d` (facultatif) recoit les mesures DE CE LOT (ajoutees) ; `total` les cumule toujours.
  void scan(Q4Batch* b, u32 h4, bool core_nonstrict, bool depth_nonstrict, bool no_canonical, DeviceExecutorStats* d = nullptr) {
    if (broken_) throw std::runtime_error("cuda : executeur q4 inutilisable apres une erreur d'allocation");
    std::string why;
    if (!validate_q4_batch(*b, &why)) throw std::invalid_argument(why);
    const size_t ns = b->u0.size(), nsd = b->seeds.size(), na = b->anchors.size(), nl = b->lens_sites.size();
    b->verdicts.assign(nsd, Q4SeedVerdict{});
    b->emits.clear();
    b->stages = Q4StageCounts{};
    DeviceExecutorStats m;
    m.lots = 1;
    if (nsd == 0) {
      total.add(m);
      if (d) d->add(m);
      return;
    }
    const ConcurrencyGauge::Scope active(gauge);
    const auto t_scan = std::chrono::steady_clock::now();
    auto tm = t_scan;
    const auto tick = [&](double* acc) { const auto now = std::chrono::steady_clock::now(); *acc += std::chrono::duration<double, std::milli>(now - tm).count(); tm = now; };
    const auto sync = [&](const char* what) {
      const auto t_w = std::chrono::steady_clock::now();
      cuda_check(cudaStreamSynchronize(stream_), what);
      m.wait_ms += ms_host_since(t_w);
    };
    reserve_sites(ns, nl);
    reserve_seeds(nsd, na);
    tick(&m.reserve_ms);
    ev_[0].record(stream_);
    m.h2d_bytes += up(d_u0_, b->u0.data(), ns) + up(d_u1_, b->u1.data(), ns) + up(d_u2_, b->u2.data(), ns) + up(d_q_, b->q.data(), ns);
    m.h2d_bytes += up(d_px_, b->px.data(), ns) + up(d_py_, b->py.data(), ns) + up(d_pz_, b->pz.data(), ns) + up(d_pid_, b->pid.data(), ns);
    if (nl) m.h2d_bytes += up(d_lens_, b->lens_sites.data(), nl);
    m.h2d_bytes += up(d_seeds_, b->seeds.data(), nsd);
    m.h2d_bytes += up(d_anchors_, b->anchors.data(), na);
    const Q4SitesDev S{d_u0_, d_u1_, d_u2_, d_q_, d_px_, d_py_, d_pz_, d_pid_, d_lens_};
    ev_[1].record(stream_);
    // K1 : cœurs.
    {
      const unsigned threads = 256, wpb = threads / 32;
      k_q4_core<<<(unsigned)((nsd + wpb - 1) / wpb), threads, 0, stream_>>>(d_seeds_, (unsigned)nsd, d_anchors_, S, h4,
                                                                           core_nonstrict, chord_nonstrict_, d_verdicts_);
      cuda_check(cudaGetLastError(), "lancement k_q4_core");
      m.launches += 1;
    }
    ev_[2].record(stream_);
    cuda_check(cudaMemcpyAsync(b->verdicts.data(), d_verdicts_, nsd * sizeof(Q4SeedVerdict), cudaMemcpyDeviceToHost, stream_), "verdicts");
    m.d2h_bytes += nsd * sizeof(Q4SeedVerdict);
    ev_[3].record(stream_);
    tick(&m.issue_ms);
    sync("sync K1");  // synchronisation du contrat (deja presente au pin)
    m.h2d_ms += CudaEvent::ms_between(ev_[0], ev_[1]);
    m.k1_ms += CudaEvent::ms_between(ev_[1], ev_[2]);
    m.d2h1_ms += CudaEvent::ms_between(ev_[2], ev_[3]);
    tm = std::chrono::steady_clock::now();
    // Hote : seeds vivants et offsets de paires (lens_count de leur ancre).
    alive_.clear();
    pair_off_.clear();
    // Comptage des paires en u64, refuse au-dela du domaine u32 des kernels
    // (offsets, allocations, copies) — jamais un wrap.
    u64 total_pairs = 0;
    for (size_t si = 0; si < nsd; ++si) {
      if (b->verdicts[si].dead) continue;
      if (total_pairs > (u64)UINT32_MAX) break;
      alive_.push_back((u32)si);
      pair_off_.push_back((u32)total_pairs);
      total_pairs += b->anchors[b->seeds[si].anchor].lens_count;
    }
    if (total_pairs > (u64)UINT32_MAX) throw std::length_error("lot q4 : plus de 2^32 - 1 paires (seed, y) — reduire le seuil de lot");
    tick(&m.host1_ms);
    if (!alive_.empty() && total_pairs > 0) {
      reserve_pairs(alive_.size(), total_pairs);
      tick(&m.reserve_ms);
      ev_[4].record(stream_);
      m.h2d_bytes += up(d_alive_, alive_.data(), alive_.size());
      m.h2d_bytes += up(d_pair_off_, pair_off_.data(), pair_off_.size());
      ev_[5].record(stream_);
      k_q4_complete<<<(unsigned)alive_.size(), 128, 0, stream_>>>(d_seeds_, d_anchors_, S, d_alive_, d_pair_off_, no_canonical, d_stage_);
      cuda_check(cudaGetLastError(), "lancement k_q4_complete");
      m.launches += 1;
      ev_[6].record(stream_);
      stage_.resize(total_pairs);
      cuda_check(cudaMemcpyAsync(stage_.data(), d_stage_, total_pairs, cudaMemcpyDeviceToHost, stream_), "etages");
      m.d2h_bytes += total_pairs;
      ev_[7].record(stream_);
      tick(&m.issue_ms);
      sync("sync K2");  // deja presente au pin
      m.h2d2_ms += CudaEvent::ms_between(ev_[4], ev_[5]);
      m.k2_ms += CudaEvent::ms_between(ev_[5], ev_[6]);
      m.d2h2_ms += CudaEvent::ms_between(ev_[6], ev_[7]);
      tm = std::chrono::steady_clock::now();
      // Hote : compteurs d'etages et compaction des candidates, dans l'ordre
      // (seed vivant, lentille) = ordre de la production.
      cand_.clear();
      for (size_t ai = 0; ai < alive_.size(); ++ai) {
        const Q4BatchSeed& sd = b->seeds[alive_[ai]];
        const Q4BatchAnchor& an = b->anchors[sd.anchor];
        for (u32 li = 0; li < an.lens_count; ++li) {
          const u8 st = stage_[pair_off_[ai] + li];
          if (st == 255) continue;
          ++b->stages.completions;
          switch ((Q4Stage)st) {
            case Q4Stage::kRejLens: ++b->stages.lens; break;
            case Q4Stage::kRejOwner: ++b->stages.owner; break;
            case Q4Stage::kRejOnce: ++b->stages.once; break;
            case Q4Stage::kRejI64: ++b->stages.i64_; break;
            case Q4Stage::kRejFacePower: ++b->stages.face; break;
            case Q4Stage::kRejDet: ++b->stages.det; break;
            case Q4Stage::kRejCenter: ++b->stages.center; break;
            case Q4Stage::kDeep: throw std::logic_error("lot q4 : etage profond avant le kernel de profondeur");
            case Q4Stage::kEmit: cand_.push_back(Q4Emit{alive_[ai], b->lens_sites[an.lens_begin + li]}); break;
            default: throw std::logic_error("lot q4 : etage inconnu rendu par le device");
          }
        }
      }
      tick(&m.host2_ms);
      if (!cand_.empty()) {
        reserve_cand(cand_.size());
        tick(&m.reserve_ms);
        ev_[8].record(stream_);
        m.h2d_bytes += up(d_cand_, cand_.data(), cand_.size());
        ev_[9].record(stream_);
        const unsigned threads = 256, wpb = threads / 32;
        k_q4_depth<<<(unsigned)((cand_.size() + wpb - 1) / wpb), threads, 0, stream_>>>(d_seeds_, d_anchors_, S, d_cand_,
                                                                                        (unsigned)cand_.size(), h4,
                                                                                        depth_nonstrict, d_deep_);
        cuda_check(cudaGetLastError(), "lancement k_q4_depth");
        m.launches += 1;
        ev_[10].record(stream_);
        deep_.resize(cand_.size());
        cuda_check(cudaMemcpyAsync(deep_.data(), d_deep_, cand_.size(), cudaMemcpyDeviceToHost, stream_), "profondeurs");
        m.d2h_bytes += cand_.size();
        ev_[11].record(stream_);
        tick(&m.issue_ms);
        sync("sync K3");  // deja presente au pin
        m.h2d3_ms += CudaEvent::ms_between(ev_[8], ev_[9]);
        m.k3_ms += CudaEvent::ms_between(ev_[9], ev_[10]);
        m.d2h3_ms += CudaEvent::ms_between(ev_[10], ev_[11]);
        tm = std::chrono::steady_clock::now();
        const bool emit_deep = MHGP5_MUTANT("q4-batched-emit-deep");
        for (size_t c = 0; c < cand_.size(); ++c) {
          if (deep_[c]) {
            ++b->stages.deep;
            if (emit_deep) b->emits.push_back(cand_[c]);
          } else {
            ++b->stages.emit;
            b->emits.push_back(cand_[c]);
          }
        }
        tick(&m.host3_ms);
      }
    }
    // Aucune synchronisation finale : chaque operation enfilee precede une des
    // synchronisations ci-dessus, le flux est vide ici.
    m.executor_ms_sum += ms_host_since(t_scan);
    total.add(m);
    if (d) d->add(m);
  }

 private:
  cudaStream_t stream_{};
  CudaEvent ev_[12];  // crees une fois par executeur : (h2d, k1, d2h) x trois etapes
  i64 *d_u0_ = nullptr, *d_u1_ = nullptr, *d_u2_ = nullptr, *d_q_ = nullptr, *d_px_ = nullptr, *d_py_ = nullptr, *d_pz_ = nullptr;
  PointId* d_pid_ = nullptr;
  u32* d_lens_ = nullptr;
  Q4BatchSeed* d_seeds_ = nullptr;
  Q4BatchAnchor* d_anchors_ = nullptr;
  Q4SeedVerdict* d_verdicts_ = nullptr;
  u32 *d_alive_ = nullptr, *d_pair_off_ = nullptr;
  u8 *d_stage_ = nullptr, *d_deep_ = nullptr;
  Q4Emit* d_cand_ = nullptr;
  size_t cap_sites_ = 0, cap_lens_ = 0, cap_seeds_ = 0, cap_anchors_ = 0, cap_alive_ = 0, cap_pairs_ = 0, cap_cand_ = 0;
  std::vector<u32> alive_, pair_off_;
  std::vector<u8> stage_, deep_;
  std::vector<Q4Emit> cand_;

  // Copie H2D asynchrone ; rend les octets enfiles.
  template <class T>
  u64 up(T* dst, const T* src, size_t n) {
    cuda_check(cudaMemcpyAsync(dst, src, n * sizeof(T), cudaMemcpyHostToDevice, stream_), "transfert");
    return (u64)n * sizeof(T);
  }
  bool broken_ = false;
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
  // RESERVES TRANSACTIONNELLES (allouer dans des temporaires, echanger apres
  // reussite complete ; sinon broken_ et relance).
  void reserve_sites(size_t ns, size_t nl) {
    try {
      Tmp t;
      const bool gs = ns > cap_sites_, gl = nl > cap_lens_;
      const size_t cs = gs ? ns + ns / 2 : 0, cl = gl ? nl + nl / 2 : 0;
      i64 *u0 = nullptr, *u1 = nullptr, *u2 = nullptr, *q = nullptr, *px = nullptr, *py = nullptr, *pz = nullptr;
      PointId* pid = nullptr;
      u32* lens = nullptr;
      if (gs) { u0 = t.alloc<i64>(cs); u1 = t.alloc<i64>(cs); u2 = t.alloc<i64>(cs); q = t.alloc<i64>(cs);
                px = t.alloc<i64>(cs); py = t.alloc<i64>(cs); pz = t.alloc<i64>(cs); pid = t.alloc<PointId>(cs); }
      if (gl) lens = t.alloc<u32>(cl);
      if (gs) { swap_in(&d_u0_, u0); swap_in(&d_u1_, u1); swap_in(&d_u2_, u2); swap_in(&d_q_, q);
                swap_in(&d_px_, px); swap_in(&d_py_, py); swap_in(&d_pz_, pz); swap_in(&d_pid_, pid); cap_sites_ = cs; }
      if (gl) { swap_in(&d_lens_, lens); cap_lens_ = cl; }
      t.commit();
    } catch (...) { broken_ = true; throw; }
  }
  void reserve_seeds(size_t nsd, size_t na) {
    try {
      Tmp t;
      const bool gs = nsd > cap_seeds_, ga = na > cap_anchors_;
      const size_t cs = gs ? nsd + nsd / 2 : 0, ca = ga ? na + na / 2 : 0;
      Q4BatchSeed* seeds = nullptr;
      Q4SeedVerdict* verdicts = nullptr;
      Q4BatchAnchor* anchors = nullptr;
      if (gs) { seeds = t.alloc<Q4BatchSeed>(cs); verdicts = t.alloc<Q4SeedVerdict>(cs); }
      if (ga) anchors = t.alloc<Q4BatchAnchor>(ca);
      if (gs) { swap_in(&d_seeds_, seeds); swap_in(&d_verdicts_, verdicts); cap_seeds_ = cs; }
      if (ga) { swap_in(&d_anchors_, anchors); cap_anchors_ = ca; }
      t.commit();
    } catch (...) { broken_ = true; throw; }
  }
  void reserve_pairs(size_t nalive, size_t npairs) {
    try {
      Tmp t;
      const bool gv = nalive > cap_alive_, gp = npairs > cap_pairs_;
      const size_t cv = gv ? nalive + nalive / 2 : 0, cp = gp ? npairs + npairs / 2 : 0;
      u32 *alive = nullptr, *off = nullptr;
      u8* stage = nullptr;
      if (gv) { alive = t.alloc<u32>(cv); off = t.alloc<u32>(cv); }
      if (gp) stage = t.alloc<u8>(cp);
      if (gv) { swap_in(&d_alive_, alive); swap_in(&d_pair_off_, off); cap_alive_ = cv; }
      if (gp) { swap_in(&d_stage_, stage); cap_pairs_ = cp; }
      t.commit();
    } catch (...) { broken_ = true; throw; }
  }
  void reserve_cand(size_t nc) {
    try {
      Tmp t;
      if (nc <= cap_cand_) return;
      const size_t cc = nc + nc / 2;
      Q4Emit* cand = t.alloc<Q4Emit>(cc);
      u8* deep = t.alloc<u8>(cc);
      swap_in(&d_cand_, cand); swap_in(&d_deep_, deep); cap_cand_ = cc;
      t.commit();
    } catch (...) { broken_ = true; throw; }
  }
  void release() {
    for (void* p : {(void*)d_u0_, (void*)d_u1_, (void*)d_u2_, (void*)d_q_, (void*)d_px_, (void*)d_py_, (void*)d_pz_, (void*)d_pid_, (void*)d_lens_, (void*)d_seeds_,
                    (void*)d_anchors_, (void*)d_verdicts_, (void*)d_alive_, (void*)d_pair_off_, (void*)d_stage_,
                    (void*)d_deep_, (void*)d_cand_})
      if (p) cudaFree(p);
  }
};

// `kernel_ms` : somme des durees d'evenements des KERNELS SEULS (K1 + K2 + K3)
// sur tous les fils — homogene avec q3 (avant 63deda74 il couvrait tout le lot
// apres H2D, boucles hote comprises) ; `stages` : cumul complet, sommes de
// temps-executeur, jamais un mur.
inline void generate_q4_device(const CloudIndex& ix, const GenerateOptions& opt, std::vector<BallCandidate>* out,
                               GenerateStats* st, double* kernel_ms, u64* launches, BatchLimits lim = BatchLimits{},
                               BatchStats* bs = nullptr, DeviceExecutorStats* stages = nullptr) {
  std::mutex mu;
  Q4DeviceExecutor::gauge.reset_peak();
  generate_q4_batched_with(ix, opt, out, st, [&](Q4Batch* b, u32 h4, bool cn, bool dn, bool nc) {
    thread_local Q4DeviceExecutor ex;
    DeviceExecutorStats d;
    ex.scan(b, h4, cn, dn, nc, &d);
    std::lock_guard<std::mutex> lk(mu);
    *kernel_ms += d.kernel_ms();
    *launches += d.launches;
    if (stages) stages->add(d);
  }, lim, bs);
  if (stages) {
    const u32 p = Q4DeviceExecutor::gauge.read_peak();
    if (p > stages->peak_concurrent) stages->peak_concurrent = p;
  }
}
#endif  // __CUDACC__

}  // namespace gpu
}  // namespace mhgp5
