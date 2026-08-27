// MorseHGP3D v5 — EXECUTEUR DEVICE de la lane q4 par lots (docs/GPU.md,
// livraison 5b) : remplit le contrat de scan_q4_batch_host avec les trois
// kernels de q4_kernels.cuh. Un executeur par fil (flux propre, tampons
// croissants). Entre les kernels, l'hote calcule les offsets de paires des
// seeds vivants, compte les etages et compacte les paires candidates ; les
// verdicts, etages et profondeurs rapatries sont exactement ceux que
// scan_q4_batch_host produirait (porte tests/q4_lane_device_gate.cu). Toute
// erreur CUDA est une exception, jamais un verdict.
#pragma once

#include <mutex>
#include <stdexcept>
#include <string>

#include "q3_lane_device.cuh"
#include "q4_kernels.cuh"
#include "q4_lane_batched.hpp"

namespace mhgp5 {
namespace gpu {

#if defined(__CUDACC__)
class Q4DeviceExecutor {
 public:
  Q4DeviceExecutor() { cuda_check(cudaStreamCreate(&stream_), "stream"); }
  ~Q4DeviceExecutor() {
    release();
    cudaStreamDestroy(stream_);
  }
  Q4DeviceExecutor(const Q4DeviceExecutor&) = delete;
  Q4DeviceExecutor& operator=(const Q4DeviceExecutor&) = delete;
  double kernel_ms_total = 0;
  u64 launches = 0;

  void scan(Q4Batch* b, u32 h4, bool core_nonstrict, bool depth_nonstrict, bool no_canonical) {
    const size_t ns = b->u0.size(), nsd = b->seeds.size(), na = b->anchors.size(), nl = b->lens_sites.size();
    b->verdicts.assign(nsd, Q4SeedVerdict{});
    b->emits.clear();
    b->stages = Q4StageCounts{};
    if (nsd == 0) return;
    reserve_sites(ns, nl);
    reserve_seeds(nsd, na);
    up(d_u0_, b->u0.data(), ns); up(d_u1_, b->u1.data(), ns); up(d_u2_, b->u2.data(), ns); up(d_q_, b->q.data(), ns);
    up(d_d0_, b->u0d.data(), ns); up(d_d1_, b->u1d.data(), ns); up(d_d2_, b->u2d.data(), ns); up(d_dq_, b->qd.data(), ns);
    up(d_px_, b->px.data(), ns); up(d_py_, b->py.data(), ns); up(d_pz_, b->pz.data(), ns); up(d_pid_, b->pid.data(), ns);
    if (nl) up(d_lens_, b->lens_sites.data(), nl);
    up(d_seeds_, b->seeds.data(), nsd);
    up(d_anchors_, b->anchors.data(), na);
    const Q4SitesDev S{d_u0_, d_u1_, d_u2_, d_q_, d_d0_, d_d1_, d_d2_, d_dq_, d_px_, d_py_, d_pz_, d_pid_, d_lens_};
    cudaEvent_t e0, e1;
    cuda_check(cudaEventCreate(&e0), "event"); cuda_check(cudaEventCreate(&e1), "event");
    cuda_check(cudaEventRecord(e0, stream_), "record");
    // K1 : cœurs.
    {
      const unsigned threads = 256, wpb = threads / 32;
      k_q4_core<<<(unsigned)((nsd + wpb - 1) / wpb), threads, 0, stream_>>>(d_seeds_, (unsigned)nsd, d_anchors_, S, h4,
                                                                           core_nonstrict, d_verdicts_);
      cuda_check(cudaGetLastError(), "lancement k_q4_core");
      ++launches;
    }
    cuda_check(cudaMemcpyAsync(b->verdicts.data(), d_verdicts_, nsd * sizeof(Q4SeedVerdict), cudaMemcpyDeviceToHost, stream_), "verdicts");
    cuda_check(cudaStreamSynchronize(stream_), "sync K1");
    // Hote : seeds vivants et offsets de paires (lens_count de leur ancre).
    alive_.clear();
    pair_off_.clear();
    u32 total_pairs = 0;
    for (size_t si = 0; si < nsd; ++si) {
      if (b->verdicts[si].dead) continue;
      alive_.push_back((u32)si);
      pair_off_.push_back(total_pairs);
      total_pairs += b->anchors[b->seeds[si].anchor].lens_count;
    }
    if (!alive_.empty() && total_pairs > 0) {
      reserve_pairs(alive_.size(), total_pairs);
      up(d_alive_, alive_.data(), alive_.size());
      up(d_pair_off_, pair_off_.data(), pair_off_.size());
      k_q4_complete<<<(unsigned)alive_.size(), 128, 0, stream_>>>(d_seeds_, d_anchors_, S, d_alive_, d_pair_off_, no_canonical, d_stage_);
      cuda_check(cudaGetLastError(), "lancement k_q4_complete");
      ++launches;
      stage_.resize(total_pairs);
      cuda_check(cudaMemcpyAsync(stage_.data(), d_stage_, total_pairs, cudaMemcpyDeviceToHost, stream_), "etages");
      cuda_check(cudaStreamSynchronize(stream_), "sync K2");
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
            case Q4Stage::kDeep: break;  // jamais avant K3
            case Q4Stage::kEmit: cand_.push_back(Q4Emit{alive_[ai], b->lens_sites[an.lens_begin + li]}); break;
          }
        }
      }
      if (!cand_.empty()) {
        reserve_cand(cand_.size());
        up(d_cand_, cand_.data(), cand_.size());
        const unsigned threads = 256, wpb = threads / 32;
        k_q4_depth<<<(unsigned)((cand_.size() + wpb - 1) / wpb), threads, 0, stream_>>>(d_seeds_, d_anchors_, S, d_cand_,
                                                                                        (unsigned)cand_.size(), h4,
                                                                                        depth_nonstrict, d_deep_);
        cuda_check(cudaGetLastError(), "lancement k_q4_depth");
        ++launches;
        deep_.resize(cand_.size());
        cuda_check(cudaMemcpyAsync(deep_.data(), d_deep_, cand_.size(), cudaMemcpyDeviceToHost, stream_), "profondeurs");
        cuda_check(cudaStreamSynchronize(stream_), "sync K3");
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
      }
    }
    cuda_check(cudaEventRecord(e1, stream_), "record");
    cuda_check(cudaStreamSynchronize(stream_), "sync fin");
    float ms = 0;
    cudaEventElapsedTime(&ms, e0, e1);
    cudaEventDestroy(e0); cudaEventDestroy(e1);
    kernel_ms_total += ms;
  }

 private:
  cudaStream_t stream_{};
  i64 *d_u0_ = nullptr, *d_u1_ = nullptr, *d_u2_ = nullptr, *d_q_ = nullptr, *d_px_ = nullptr, *d_py_ = nullptr, *d_pz_ = nullptr;
  double *d_d0_ = nullptr, *d_d1_ = nullptr, *d_d2_ = nullptr, *d_dq_ = nullptr;
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

  template <class T>
  void up(T* dst, const T* src, size_t n) {
    cuda_check(cudaMemcpyAsync(dst, src, n * sizeof(T), cudaMemcpyHostToDevice, stream_), "transfert");
  }
  template <class T>
  static void grow(T** p, size_t n) {
    if (*p) cudaFree(*p);
    cuda_check(cudaMalloc(p, n * sizeof(T)), "cudaMalloc");
  }
  void reserve_sites(size_t ns, size_t nl) {
    if (ns > cap_sites_) {
      cap_sites_ = ns + ns / 2;
      grow(&d_u0_, cap_sites_); grow(&d_u1_, cap_sites_); grow(&d_u2_, cap_sites_); grow(&d_q_, cap_sites_);
      grow(&d_d0_, cap_sites_); grow(&d_d1_, cap_sites_); grow(&d_d2_, cap_sites_); grow(&d_dq_, cap_sites_);
      grow(&d_px_, cap_sites_); grow(&d_py_, cap_sites_); grow(&d_pz_, cap_sites_); grow(&d_pid_, cap_sites_);
    }
    if (nl > cap_lens_) {
      cap_lens_ = nl + nl / 2;
      grow(&d_lens_, cap_lens_);
    }
  }
  void reserve_seeds(size_t nsd, size_t na) {
    if (nsd > cap_seeds_) {
      cap_seeds_ = nsd + nsd / 2;
      grow(&d_seeds_, cap_seeds_);
      grow(&d_verdicts_, cap_seeds_);
    }
    if (na > cap_anchors_) {
      cap_anchors_ = na + na / 2;
      grow(&d_anchors_, cap_anchors_);
    }
  }
  void reserve_pairs(size_t nalive, size_t npairs) {
    if (nalive > cap_alive_) {
      cap_alive_ = nalive + nalive / 2;
      grow(&d_alive_, cap_alive_);
      grow(&d_pair_off_, cap_alive_);
    }
    if (npairs > cap_pairs_) {
      cap_pairs_ = npairs + npairs / 2;
      grow(&d_stage_, cap_pairs_);
    }
  }
  void reserve_cand(size_t nc) {
    if (nc > cap_cand_) {
      cap_cand_ = nc + nc / 2;
      grow(&d_cand_, cap_cand_);
      grow(&d_deep_, cap_cand_);
    }
  }
  void release() {
    for (void* p : {(void*)d_u0_, (void*)d_u1_, (void*)d_u2_, (void*)d_q_, (void*)d_d0_, (void*)d_d1_, (void*)d_d2_,
                    (void*)d_dq_, (void*)d_px_, (void*)d_py_, (void*)d_pz_, (void*)d_pid_, (void*)d_lens_, (void*)d_seeds_,
                    (void*)d_anchors_, (void*)d_verdicts_, (void*)d_alive_, (void*)d_pair_off_, (void*)d_stage_,
                    (void*)d_deep_, (void*)d_cand_})
      if (p) cudaFree(p);
  }
};

inline void generate_q4_device(const CloudIndex& ix, const GenerateOptions& opt, std::vector<BallCandidate>* out,
                               GenerateStats* st, double* kernel_ms, u64* launches, size_t seeds_per_launch = kSeedsPerLaunch) {
  std::mutex mu;
  generate_q4_batched_with(ix, opt, out, st, [&](Q4Batch* b, u32 h4, bool cn, bool dn, bool nc) {
    thread_local Q4DeviceExecutor ex;
    const double before_ms = ex.kernel_ms_total;
    const u64 before_l = ex.launches;
    ex.scan(b, h4, cn, dn, nc);
    std::lock_guard<std::mutex> lk(mu);
    *kernel_ms += ex.kernel_ms_total - before_ms;
    *launches += ex.launches - before_l;
  }, seeds_per_launch);
}
#endif  // __CUDACC__

}  // namespace gpu
}  // namespace mhgp5
