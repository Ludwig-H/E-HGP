// MorseHGP3D v5 — pilote en ligne de commande du pipeline (src/pipeline/run.hpp).
// Codes : 0 complete_regular, 2 refus avant calcul, 3 invariant viole.
// Binaire PRODUIT : compile sans MHGP5_TESTING — aucun mutant n'y existe.
#include <sys/resource.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "../src/cloud/families.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  bool ok = true;
  int n = 8000, coord = 0;
  long long seed = 3;
  RunOptions opt;
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
    else if (const char* v = val("--fold-inflight=")) opt.fold_inflight = std::atoi(v);
    else if (const char* v = val("--cell-min-sites=")) opt.cell_grid_min_sites = (size_t)std::atoll(v);
    else if (const char* v = val("--postsep=")) {  // raffinement post-separation : L in [0, 3], refus hors domaine
      if (v[0] < '0' || v[0] > '3' || v[1] != '\0') ok = false;
      else opt.postsep_refine_levels = (u32)(v[0] - '0');
    }
    else if (const char* v = val("--shell-cap=")) opt.shell_cap = (size_t)std::atoll(v);
    else if (arg == "--digest") opt.digest = true;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      ok = false;
    }
  }
  if (!ok || n < 2 || opt.s < 1 || opt.smax < 2 || opt.smax > 11) {
    std::fprintf(stderr, "REFUS : arguments (profil K_max <= 10 ⟺ smax <= 11)\n");
    return 2;
  }
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const std::vector<InputPoint> in = make_family_input(family, n, coord, seed);
  const RunResult rr = run_pipeline(in, opt);
  if (rr.status != PipelineStatus::kCompleteRegular) {
    std::fprintf(stderr, "REFUS %s\n", rr.message.c_str());
    return status_exit_code(rr.status);
  }
  print_run(stdout, cloud_family_name(family), n, coord, seed, opt, rr);
  // Pic de residence MESURE (RSS max du processus), jamais un pic annonce.
  struct rusage ru;
  if (getrusage(RUSAGE_SELF, &ru) == 0) std::printf("rss_max_kb=%ld\n", ru.ru_maxrss);
  return 0;
}
