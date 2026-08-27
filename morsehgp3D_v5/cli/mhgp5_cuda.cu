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
    else if (const char* v = val("--s=")) opt.s = std::atoll(v);
    else if (const char* v = val("--smax=")) opt.smax = (u64)std::atoll(v);
    else if (const char* v = val("--threads=")) opt.threads = std::atoi(v);
    else if (const char* v = val("--shell-cap=")) opt.shell_cap = (size_t)std::atoll(v);
    else if (arg == "--digest") opt.digest = true;
    else if (arg == "--gpu") gpu = true;
    else if (const char* v = val("--gpu-min-sites=")) {
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
  if (!ok || n < 2 || opt.s < 1 || opt.smax < 2 || opt.smax > 11) {
    std::fprintf(stderr, "REFUS : arguments (profil K_max <= 10 ⟺ smax <= 11)\n");
    return 2;
  }
  double kernel_ms = 0;
  u64 launches = 0;
  BatchStats bs3, bs4;
  if (gpu) {
    int ndev = 0;
    if (cudaGetDeviceCount(&ndev) != cudaSuccess || ndev < 1) {
      std::fprintf(stderr, "REFUS : aucun device CUDA visible\n");
      return 2;
    }
    BatchLimits lim;
    lim.device_min_sites = gpu_min_sites;
    opt.q3_override = [&, lim](const CloudIndex& ix, const GenerateOptions& g, std::vector<BallCandidate>* out, GenerateStats* st) {
      gpu::generate_q3_device(ix, g, out, st, &kernel_ms, &launches, lim, &bs3);
    };
    opt.q4_override = [&, lim](const CloudIndex& ix, const GenerateOptions& g, std::vector<BallCandidate>* out, GenerateStats* st) {
      gpu::generate_q4_device(ix, g, out, st, &kernel_ms, &launches, lim, &bs4);
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
  // Pic de residence MESURE (RSS max du processus), jamais un pic annonce.
  struct rusage ru;
  if (getrusage(RUSAGE_SELF, &ru) == 0) std::printf("rss_max_kb=%ld\n", ru.ru_maxrss);
  return 0;
}
