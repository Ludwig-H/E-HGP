// MorseHGP3D v5 — STATISTIQUES DE L'INSTRUMENT DEVICE (hote pur, sans CUDA :
// verifiable par une porte CPU, tests/gpu_instrument_gate.cpp). Reponse a
// audits/AUDIT_RENDEMENT_GPU_MULTICPU_20260828.md (§ reception de 63deda74) :
//   - toutes les durees sont des SOMMES de temps-executeur sur les appels
//     scan() de fils CONCURRENTS (`executor_ms_sum`) : aucune n'est un mur,
//     aucune ne se soustrait au mur de lane (t_rects_ms, imprime lane_wall_ms) ;
//   - les etapes device (h2d, k1, d2h1, h2d2, k2, d2h2, h2d3, k3, d2h3) sont
//     des durees d'EVENEMENTS CUDA sur le flux de l'executeur, recoltees apres
//     les synchronisations qui existaient deja ; les temps hote (reserve,
//     enfilement, attente, boucles hote1/2/3) sont des steady_clock entre deux
//     points hote ;
//   - octets H2D/D2H = sizeof exact des copies enfilees ; lots et kernels
//     comptes ; pic de scan() simultanes par compteur atomique (ConcurrencyGauge).
#pragma once

#include <atomic>
#include <chrono>

#include "../core/mutants.hpp"
#include "../core/types.hpp"

namespace mhgp5 {
namespace gpu {

// STATISTIQUES D'EXECUTEUR DEVICE, cumulees par lane (q3 : h2d/k1/d2h1 ;
// q4 : toutes). Champs `*_ms` = sommes de temps-executeur sur les fils.
struct DeviceExecutorStats {
  // Evenements CUDA (chronologie du flux de l'executeur), en ms :
  double h2d_ms = 0;   // copies H2D initiales (sites SoA, seeds, ancres ; q4 : + positions, ids, lentille)
  double k1_ms = 0;    // kernel 1 (q3 : k_scan ; q4 : k_q4_core)
  double d2h1_ms = 0;  // retour des verdicts
  double h2d2_ms = 0;  // q4 : copie des vivants et offsets (avant K2)
  double k2_ms = 0;    // q4 : k_q4_complete
  double d2h2_ms = 0;  // q4 : retour des etages (un octet par paire)
  double h2d3_ms = 0;  // q4 : copie des candidates (avant K3)
  double k3_ms = 0;    // q4 : k_q4_depth
  double d2h3_ms = 0;  // q4 : retour des profondeurs
  // Hote (steady_clock entre deux points hote), en ms :
  double reserve_ms = 0;  // reservations device transactionnelles (cudaMalloc/cudaFree), toutes etapes
  double issue_ms = 0;    // enfilement hote des copies/kernels (appels CUDA asynchrones : driver, contention entre fils)
  double wait_ms = 0;     // attentes hote dans cudaStreamSynchronize (les seules qui existaient deja)
  double host1_ms = 0;    // q4 : boucle des seeds vivants / offsets de paires
  double host2_ms = 0;    // q4 : comptage des etages et compaction des candidates
  double host3_ms = 0;    // q4 : emissions apres K3
  double executor_ms_sum = 0;  // duree hote de scan() entier — SOMME sur les fils, JAMAIS un mur
  // Octets transferes (cumul), lots, kernels, pic de scan() simultanes.
  u64 h2d_bytes = 0, d2h_bytes = 0;
  u64 lots = 0, launches = 0;
  u32 peak_concurrent = 0;
  void add(const DeviceExecutorStats& o) {
    h2d_ms += o.h2d_ms; k1_ms += o.k1_ms; d2h1_ms += o.d2h1_ms; h2d2_ms += o.h2d2_ms; k2_ms += o.k2_ms; d2h2_ms += o.d2h2_ms;
    h2d3_ms += o.h2d3_ms; k3_ms += o.k3_ms; d2h3_ms += o.d2h3_ms;
    reserve_ms += o.reserve_ms; issue_ms += o.issue_ms; wait_ms += o.wait_ms; host1_ms += o.host1_ms; host2_ms += o.host2_ms; host3_ms += o.host3_ms;
    executor_ms_sum += o.executor_ms_sum;
    h2d_bytes += o.h2d_bytes; d2h_bytes += o.d2h_bytes; lots += o.lots; launches += o.launches;
    if (o.peak_concurrent > peak_concurrent) peak_concurrent = o.peak_concurrent;
  }
  double kernel_ms() const { return k1_ms + k2_ms + k3_ms; }  // kernels seuls (homogene q3/q4)
  // Reste hote non classe de scan() (executor_ms_sum moins reservations, enfilement, attentes et boucles hote) :
  // en theorie quelques appels et recoltes d'evenements ; borne a zero si les horloges se croisent.
  double rest_ms() const {
    const double r = executor_ms_sum - (reserve_ms + issue_ms + wait_ms + host1_ms + host2_ms + host3_ms);
    return r > 0 ? r : 0;
  }
};

// PIC DE scan() SIMULTANES : compteur atomique incremente a l'entree de
// scan(), decremente a la sortie (RAII, donc aussi sur exception) ; le pic est
// remis a zero au debut de chaque lane (generate_*_device).
struct ConcurrencyGauge {
  std::atomic<u32> active{0}, peak{0};
  struct Scope {
    ConcurrencyGauge& g;
    explicit Scope(ConcurrencyGauge& gg) : g(gg) {
      const u32 cur = g.active.fetch_add(1, std::memory_order_acq_rel) + 1;
      if (MHGP5_MUTANT("gauge-no-peak")) return;  // mutant : le pic n'est jamais releve (vert-par-vacuite)
      u32 p = g.peak.load(std::memory_order_relaxed);
      while (cur > p && !g.peak.compare_exchange_weak(p, cur, std::memory_order_acq_rel)) {}
    }
    ~Scope() { g.active.fetch_sub(1, std::memory_order_acq_rel); }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
  };
  void reset_peak() { peak.store(active.load(std::memory_order_acquire), std::memory_order_release); }
  u32 read_peak() const { return peak.load(std::memory_order_acquire); }
};

// CYCLE DE VIE DES EXECUTEURS (audit rendement, P0 « le cycle de vie CUDA
// echappe a la decomposition ») : nombre d'executeurs construits et duree
// construction -> destruction (flux, evenements, allocations, destruction),
// cumules sur le processus ; ces durees sont HORS executor_ms_sum et, comme
// lui, des sommes sur les fils — jamais un mur. Les executeurs thread_local
// sont detruits a la fin de leur fil, donc avant la lecture par la CLI.
struct ExecutorLifecycle {
  std::atomic<u64> created{0};
  std::atomic<u64> lifecycle_ns{0};
  static ExecutorLifecycle& global() {
    static ExecutorLifecycle g;
    return g;
  }
  struct Scope {
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    Scope() { ExecutorLifecycle::global().created.fetch_add(1, std::memory_order_relaxed); }
    ~Scope() {
      const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count();
      ExecutorLifecycle::global().lifecycle_ns.fetch_add((u64)(ns > 0 ? ns : 0), std::memory_order_relaxed);
    }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
  };
};

}  // namespace gpu
}  // namespace mhgp5
