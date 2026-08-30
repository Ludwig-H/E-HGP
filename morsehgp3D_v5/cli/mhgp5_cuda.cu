// MorseHGP3D v5 — pilote CUDA du pipeline : identique a cli/mhgp5.cpp, avec
// `--gpu` qui remplace les lanes q3 et q4 par leurs executeurs device
// (src/gpu/q3_lane_device.cuh, q4_lane_device.cuh). Sans --gpu : lanes CPU
// (le meme binaire sert de temoin CPU dans un banc apparie). Codes : 0
// complete_regular, 2 refus avant calcul (dont : aucun device), 3 invariant
// viole. Compile sans MHGP5_TESTING — aucun mutant.
#include <sys/resource.h>

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

#include "../src/cloud/families.hpp"
#include "../src/core/parse.hpp"
#include "../src/gpu/q3_lane_device.cuh"
#include "../src/gpu/q4_lane_device.cuh"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  bool ok = true;
  int n = 8000, coord = 0;
  long long seed = 3;
  RunOptions opt;
  bool gpu = false;
  size_t gpu_min_sites = 1;  // 1 = toute ancre au device ; N = seulement les covers d'au moins N sites
  int gpu_executors = 4;  // pool persistant (G0), 1..8
  bool gpu_wire_index = false;  // G1 : covers par indices u32 + geometrie residente (q3)
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto val = [&](const char* prefix) -> const char* {
      const size_t l = std::strlen(prefix);
      return arg.compare(0, l, prefix) == 0 ? arg.c_str() + l : nullptr;
    };
    if (const char* v = val("--family=")) ok = parse_cloud_family(v, &family) && ok;
    else if (const char* v = val("--n=")) n = std::atoi(v);
    else if (const char* v = val("--coord=")) coord = std::atoi(v);
    else if (const char* v = val("--seed=")) seed = std::atoll(v);
    else if (const char* v = val("--s=")) {
      i64 parsed = 0;
      if (!parse_i64_exact(v, &parsed)) ok = false;
      else opt.s = parsed;
    }
    else if (const char* v = val("--smax=")) opt.smax = (u64)std::atoll(v);
    else if (const char* v = val("--threads=")) opt.threads = std::atoi(v);
    else if (const char* v = val("--shell-cap=")) opt.shell_cap = (size_t)std::atoll(v);
    else if (arg == "--digest") opt.digest = true;
    else if (arg == "--gpu") gpu = true;
    else if (const char* v = val("--gpu-wire=")) {
      if (std::string(v) == "index") gpu_wire_index = true;
      else if (std::string(v) == "soa") gpu_wire_index = false;
      else ok = false;
    } else if (const char* v = val("--gpu-executors=")) {
      gpu_executors = std::atoi(v);
      if (gpu_executors < 1 || gpu_executors > gpu::kGpuExecutorsMax) ok = false;
    } else if (const char* v = val("--gpu-min-sites=")) {
      const long long m = std::atoll(v);
      if (m < 1) ok = false;  // refus explicite (jamais une conversion silencieuse en SIZE_MAX)
      else gpu_min_sites = (size_t)m;
    }
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      ok = false;
    }
  }
  if (gpu_min_sites < 1) ok = false;
  if (!ok || n < 2 || opt.s < kSeparationProfileMin || opt.smax < 2 || opt.smax > 11) {
    std::fprintf(stderr, "REFUS : arguments (profil s >= 8, K_max <= 10 ⟺ smax <= 11)\n");
    return 2;
  }
  double kernel_ms = 0;
  u64 launches = 0;
  BatchStats bs3, bs4;
  gpu::DeviceExecutorStats sg3, sg4;
  if (gpu) {
    int ndev = 0;
    if (cudaGetDeviceCount(&ndev) != cudaSuccess || ndev < 1) {
      std::fprintf(stderr, "REFUS : aucun device CUDA visible\n");
      return 2;
    }
    BatchLimits lim;
    lim.device_min_sites = gpu_min_sites;
    lim.gpu_executors = gpu_executors;
    lim.wire_index = gpu_wire_index;
    opt.q3_override = [&, lim](const CloudIndex& ix, const GenerateOptions& g, std::vector<BallCandidate>* out, GenerateStats* st) {
      gpu::generate_q3_device(ix, g, out, st, &kernel_ms, &launches, lim, &bs3, &sg3);
    };
    opt.q4_override = [&, lim](const CloudIndex& ix, const GenerateOptions& g, std::vector<BallCandidate>* out, GenerateStats* st) {
      gpu::generate_q4_device(ix, g, out, st, &kernel_ms, &launches, lim, &bs4, &sg4);
    };
  }
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const std::vector<InputPoint> in = make_family_input(family, n, coord, seed);
  RunResult rr;
  try {
    rr = run_pipeline(in, opt);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "REFUS : %s\n", e.what());
    return 2;
  }
  if (rr.status != PipelineStatus::kCompleteRegular) {
    std::fprintf(stderr, "REFUS %s\n", rr.message.c_str());
    return status_exit_code(rr.status);
  }
  print_run(stdout, cloud_family_name(family), n, coord, seed, opt, rr);
  std::printf("gpu=%d kernel_ms=%.1f lancements=%llu min_sites=%zu routage_q3=%llu/%llu ancres (seeds %llu/%llu) routage_q4=%llu/%llu ancres (seeds %llu/%llu)\n",
              gpu ? 1 : 0, kernel_ms, (unsigned long long)launches, gpu_min_sites, (unsigned long long)bs3.anchors_device,
              (unsigned long long)bs3.anchors_host, (unsigned long long)bs3.seeds_device, (unsigned long long)bs3.seeds_host,
              (unsigned long long)bs4.anchors_device, (unsigned long long)bs4.anchors_host, (unsigned long long)bs4.seeds_device,
              (unsigned long long)bs4.seeds_host);
  // INSTRUMENT RECEVABLE (audit du 28 aout 2026) : deux lignes par lane device.
  // `lane_wall_ms` est le MUR de la lane (rects q3/q4 de temps_ms : phase des
  // rectangles, assemblage hote compris, mesure par le fil appelant) ;
  // `executor_ms_sum` et toutes les durees entre parentheses sont des SOMMES
  // de temps-executeur sur des fils concurrents — elles ne se soustraient pas
  // au mur de lane et n'en sont pas une fraction. Etapes device par evenements
  // CUDA (h2d, k1, d2h1, h2d2, k2, d2h2, h2d3, k3, d2h3) ; hote en steady_clock
  // (reserve, enfilement, attente dans les synchronisations existantes,
  // boucles hote1/2/3, reste non classe). `kernel_ms` de la ligne gpu= est
  // desormais la somme des kernels seuls (q3 : k1 ; q4 : k1 + k2 + k3).
  // Lots : p50/p95 par classe log2 (`p50<2^k` : la valeur de rang median est
  // < 2^k et >= 2^(k-1) si k >= 1), max exact. `flux_pic` = pic de scan()
  // simultanes (compteur atomique). Octets = cumul des copies enfilees.
  if (gpu) {
    const GenerateStats& gs = rr.gen;
    std::printf("gpu_q3_etapes lane_wall_ms=%.1f executor_ms_sum=%.1f (h2d=%.1f k1=%.1f d2h1=%.1f reserve=%.1f enfilement=%.1f attente=%.1f reste=%.1f)"
                " h2d_octets=%llu d2h_octets=%llu lots=%llu lancements=%llu flux_pic=%u"
                " index_lots=%llu soa_lots=%llu octets_sites_index=%llu octets_sites_soa=%llu"
                " lots_seeds p50<2^%d p95<2^%d max=%llu lots_sites p50<2^%d p95<2^%d max=%llu\n",
                gs.t_rects_ms[1], sg3.executor_ms_sum, sg3.h2d_ms, sg3.k1_ms, sg3.d2h1_ms, sg3.reserve_ms, sg3.issue_ms, sg3.wait_ms, sg3.rest_ms(),
                (unsigned long long)sg3.h2d_bytes, (unsigned long long)sg3.d2h_bytes, (unsigned long long)sg3.lots, (unsigned long long)sg3.launches,
                (unsigned)sg3.peak_concurrent,
                (unsigned long long)sg3.index_lots, (unsigned long long)sg3.soa_lots, (unsigned long long)sg3.site_index_bytes,
                (unsigned long long)sg3.site_soa_bytes,
                bs3.lot_seeds.quantile_class(0.50), bs3.lot_seeds.quantile_class(0.95), (unsigned long long)bs3.max_lot_seeds,
                bs3.lot_sites.quantile_class(0.50), bs3.lot_sites.quantile_class(0.95), (unsigned long long)bs3.max_lot_sites);
    std::printf("gpu_q4_etapes lane_wall_ms=%.1f executor_ms_sum=%.1f (h2d=%.1f k1=%.1f d2h1=%.1f hote1=%.1f h2d2=%.1f k2=%.1f d2h2=%.1f hote2=%.1f"
                " h2d3=%.1f k3=%.1f d2h3=%.1f hote3=%.1f reserve=%.1f enfilement=%.1f attente=%.1f reste=%.1f)"
                " h2d_octets=%llu d2h_octets=%llu lots=%llu lancements=%llu flux_pic=%u"
                " index_lots=%llu soa_lots=%llu octets_sites_index=%llu octets_sites_soa=%llu"
                " lots_seeds p50<2^%d p95<2^%d max=%llu lots_sites p50<2^%d p95<2^%d max=%llu lots_paires p50<2^%d p95<2^%d max=%llu\n",
                gs.t_rects_ms[2], sg4.executor_ms_sum, sg4.h2d_ms, sg4.k1_ms, sg4.d2h1_ms, sg4.host1_ms, sg4.h2d2_ms, sg4.k2_ms, sg4.d2h2_ms, sg4.host2_ms,
                sg4.h2d3_ms, sg4.k3_ms, sg4.d2h3_ms, sg4.host3_ms, sg4.reserve_ms, sg4.issue_ms, sg4.wait_ms, sg4.rest_ms(),
                (unsigned long long)sg4.h2d_bytes, (unsigned long long)sg4.d2h_bytes, (unsigned long long)sg4.lots, (unsigned long long)sg4.launches,
                (unsigned)sg4.peak_concurrent,
                (unsigned long long)sg4.index_lots, (unsigned long long)sg4.soa_lots, (unsigned long long)sg4.site_index_bytes,
                (unsigned long long)sg4.site_soa_bytes,
                bs4.lot_seeds.quantile_class(0.50), bs4.lot_seeds.quantile_class(0.95), (unsigned long long)bs4.max_lot_seeds,
                bs4.lot_sites.quantile_class(0.50), bs4.lot_sites.quantile_class(0.95), (unsigned long long)bs4.max_lot_sites,
                bs4.lot_pairs.quantile_class(0.50), bs4.lot_pairs.quantile_class(0.95), (unsigned long long)bs4.max_lot_pairs);
    // Cycle de vie des executeurs (creation, flux, evenements, allocations, destruction) : HORS executor_ms_sum,
    // cumule sur le processus (les executeurs thread_local des deux lanes), somme sur les fils — jamais un mur.
    // PREUVE DE BRANCHE (audit du 28 aout) : le pilote REFUSE (code 3, invariant)
    // un run dont le wire execute ne correspond pas au wire demande — imprimer
    // `wire=index` ne suffit pas, il faut que tous les lots aient pris cette
    // branche et n'aient televerse aucun octet SoA de site.
    for (int li = 0; li < 2; ++li) {
      const gpu::DeviceExecutorStats& sg = li == 0 ? sg3 : sg4;
      if (sg.lots == 0) continue;
      const bool ok_branch = gpu_wire_index ? (sg.index_lots == sg.lots && sg.soa_lots == 0 && sg.site_soa_bytes == 0)
                                            : (sg.soa_lots == sg.lots && sg.index_lots == 0 && sg.site_index_bytes == 0);
      if (!ok_branch) {
        std::fprintf(stderr, "invariant : lane q%d, wire demande %s mais index_lots=%llu soa_lots=%llu sur %llu lots\n", li + 3,
                     gpu_wire_index ? "index" : "soa", (unsigned long long)sg.index_lots, (unsigned long long)sg.soa_lots,
                     (unsigned long long)sg.lots);
        return 3;
      }
    }
    const gpu::ExecutorLifecycle& lc = gpu::ExecutorLifecycle::global();
    std::printf("gpu_cycle_de_vie executeurs_crees=%llu cycle_de_vie_ms_sum=%.1f pool_executeurs=%d wire=%s\n", (unsigned long long)lc.created.load(),
                (double)lc.lifecycle_ns.load() / 1e6, gpu_executors, gpu_wire_index ? "index" : "soa");
  }
  // Pic de residence MESURE (RSS max du processus), jamais un pic annonce.
  struct rusage ru;
  if (getrusage(RUSAGE_SELF, &ru) == 0) std::printf("rss_max_kb=%ld\n", ru.ru_maxrss);
  return 0;
}
