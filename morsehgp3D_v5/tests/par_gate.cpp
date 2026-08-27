// MorseHGP3D v5 — porte du parallelisme : sortie BIT-IDENTIQUE 1 fil / N fils
// (digests balls et all), et ouvriers MESURES (jamais declares).
//   0 : digests identiques ET au moins un etage a cree >= 2 ouvriers a N fils
//       ET aucun etage n'en a cree plus que N ;
//   3 : porte inefficace (aucun ouvrier cree a N fils : le parallelisme
//       n'est pas exerce) ;
//   4 : mutant tue — `par-drop-shard` / `par-drop-ball-chunk` (digests
//       differents) ou `parallel-one-worker` (digests identiques mais un seul
//       ouvrier mesure partout : la metadonnee trahit le mutant).
#include <cstdio>
#include <cstring>
#include <string>

#include "../src/cloud/families.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  int n = 400, threads = 8;
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--family=", 0) == 0) { if (!parse_cloud_family(arg.c_str() + 9, &family)) return 2; }
    else if (arg.rfind("--n=", 0) == 0) n = std::atoi(arg.c_str() + 4);
    else if (arg.rfind("--threads=", 0) == 0) threads = std::atoi(arg.c_str() + 10);
    else if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else return 2;
  }
  if (n < 2 || threads < 2) return 2;
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const int coord = cloud_family_default_coord(family, n);
  const std::vector<InputPoint> in = make_family_input(family, n, coord, 3);
  RunOptions o1;
  o1.threads = 1;
  o1.digest = true;
  RunOptions oN = o1;
  oN.threads = threads;
  const RunResult r1 = run_pipeline(in, o1);
  const RunResult rN = run_pipeline(in, oN);
  if (r1.status != PipelineStatus::kCompleteRegular || rN.status != PipelineStatus::kCompleteRegular) return 2;
  const u64 w[] = {rN.gen.workers_wspd[0], rN.gen.workers_wspd[1], rN.gen.workers_wspd[2], rN.gen.workers_rects[0],
                   rN.gen.workers_rects[1], rN.gen.workers_rects[2], rN.expand.workers_prefilter,
                   rN.expand.workers_census, rN.expand.workers_expand};
  u64 wmax = 0;
  for (const u64 x : w) wmax = std::max(wmax, x);
  const bool same = r1.digest_balls == rN.digest_balls && r1.digest_all == rN.digest_all;
  std::printf("par_gate famille=%s n=%d fils=%d digests=%s ouvriers_max=%llu (wspd %llu/%llu/%llu rects %llu/%llu/%llu "
              "prefiltre %llu census %llu expansion %llu)\n",
              cloud_family_name(family), n, threads, same ? "identiques" : "DIFFERENTS", (unsigned long long)wmax,
              (unsigned long long)w[0], (unsigned long long)w[1], (unsigned long long)w[2], (unsigned long long)w[3],
              (unsigned long long)w[4], (unsigned long long)w[5], (unsigned long long)w[6], (unsigned long long)w[7],
              (unsigned long long)w[8]);
  if (wmax > (u64)threads) {
    std::fprintf(stderr, "INVARIANT : plus d'ouvriers crees (%llu) que demandes (%d)\n", (unsigned long long)wmax, threads);
    return 3;
  }
  if (!inject.empty()) {
    if (!same) {
      std::fprintf(stderr, "MUTANT TUE : %s (digests differents)\n", inject.c_str());
      return 4;
    }
    if (inject == "parallel-one-worker" && wmax <= 1) {
      std::fprintf(stderr, "MUTANT TUE : %s (un seul ouvrier mesure)\n", inject.c_str());
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s survivant\n", inject.c_str());
    return 3;
  }
  if (!same) {
    std::fprintf(stderr, "INVARIANT : la sortie depend du nombre de fils\n");
    return 3;
  }
  if (wmax < 2) {
    std::fprintf(stderr, "PORTE INEFFICACE : aucun parallelisme exerce\n");
    return 3;
  }
  std::printf("par_gate OK\n");
  return 0;
}
