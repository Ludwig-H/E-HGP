// MorseHGP3D v6 — pilote en ligne de commande du pipeline (src/pipeline/run.hpp).
// Codes : 0 complete_regular, 2 refus avant calcul, 3 invariant viole.
// Binaire PRODUIT : compile sans MHGP6_TESTING — aucun mutant n'y existe.
// PARSING EXACT DE TOUTES LES OPTIONS (docs/PROVENANCE.md, dette v5 fermee) :
// suffixe, vide, signe explicite ou debordement ⟹ code 2, jamais un atoi
// tolerant qui selectionnerait silencieusement un autre regime.
#include <sys/resource.h>

#include <cstdio>
#include <cstring>
#include <limits>
#include <string>

#include "../src/cloud/families.hpp"
#include "../src/core/parse.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp6;

namespace {

bool parse_int_range(const char* text, long long lo, long long hi, long long* out) {
  i64 v = 0;
  if (!parse_i64_exact(text, &v)) return false;
  if ((long long)v < lo || (long long)v > hi) return false;
  *out = (long long)v;
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  bool ok = true;
  long long n = 8000, coord = 0, seed = 3;
  RunOptions opt;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto val = [&](const char* prefix) -> const char* {
      const size_t l = std::strlen(prefix);
      return arg.compare(0, l, prefix) == 0 ? arg.c_str() + l : nullptr;
    };
    long long v = 0;
    i64 vi = 0;
    if (const char* s = val("--family=")) {
      ok = parse_cloud_family(s, &family) && ok;
    } else if (const char* s = val("--n=")) {
      ok = parse_int_range(s, 2, std::numeric_limits<int>::max(), &n) && ok;
    } else if (const char* s = val("--coord=")) {
      ok = parse_int_range(s, 1, 65536, &coord) && ok;
    } else if (const char* s = val("--seed=")) {
      ok = parse_i64_exact(s, &vi) && ok;
      seed = (long long)vi;
    } else if (const char* s = val("--s=")) {
      ok = parse_i64_exact(s, &opt.s) && ok;
    } else if (const char* s = val("--smax=")) {
      ok = parse_int_range(s, 2, 11, &v) && ok;
      opt.smax = (u64)v;
    } else if (const char* s = val("--threads=")) {
      ok = parse_int_range(s, 1, 1024, &v) && ok;
      opt.threads = (int)v;
    } else if (const char* s = val("--fold-inflight=")) {
      ok = parse_int_range(s, 1, kFoldInflightMax, &v) && ok;
      opt.fold_inflight = (int)v;
    } else if (const char* s = val("--pretest-query-min=")) {
      ok = parse_int_range(s, 0, std::numeric_limits<long long>::max(), &v) && ok;
      opt.pretest_query_min_points = (size_t)v;
    } else if (const char* s = val("--cell-min-sites=")) {
      ok = parse_int_range(s, 0, std::numeric_limits<long long>::max(), &v) && ok;
      opt.cell_grid_min_sites = (size_t)v;
    } else if (const char* s = val("--shell-cap=")) {
      ok = parse_int_range(s, 4, 12, &v) && ok;
      opt.shell_cap = (size_t)v;
    } else if (arg == "--sonde-e6") {
      opt.e6_probe = true;
    } else if (arg == "--digest") {
      opt.digest = true;
    } else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      ok = false;
    }
  }
  if (!ok || opt.s < kSeparationProfileMin) {
    std::fprintf(stderr, "REFUS : arguments (parsing exact ; profil s >= 8, smax dans [2, 11])\n");
    return 2;
  }
  if (coord <= 0) coord = cloud_family_default_coord(family, (int)n);
  const std::vector<InputPoint> in = make_family_input(family, (int)n, (int)coord, seed);
  if (in.empty()) {
    std::fprintf(stderr, "REFUS : famille vide (parametres hors contrat de la famille)\n");
    return 2;
  }
  const RunResult rr = run_pipeline(in, opt);
  if (rr.status != PipelineStatus::kCompleteRegular) {
    std::fprintf(stderr, "REFUS %s\n", rr.message.c_str());
    return status_exit_code(rr.status);
  }
  print_run(stdout, cloud_family_name(family), (int)n, (int)coord, seed, opt, rr);
  // Pic de residence MESURE (RSS max du processus), jamais un pic annonce.
  struct rusage ru;
  if (getrusage(RUSAGE_SELF, &ru) == 0) std::printf("rss_max_kb=%ld\n", ru.ru_maxrss);
  return 0;
}
