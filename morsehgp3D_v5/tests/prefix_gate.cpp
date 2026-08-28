// MorseHGP3D v5 — PORTE DU PREFIXE EXACT (docs/ECHELLE.md § 3.3) : les ordres
// K <= smax − 1 d'un run a `smax` sont l'objet complet restreint — memes
// digests par ordre (digest_forest[K]), memes cardinalites par ordre — que le
// run a smax = 11. Theoreme : la mort par profondeur |I_B| >= h_q = smax − q + 1
// ne tue que des boules dont tous les evenements ont K = q + d − 1 >= smax.
// Planchers : >= --min-events evenements a l'ordre smax − 1 (jamais vert par
// vacuite). Codes : 0 conforme ; 1 desaccord ; 2 arguments ; 3 plancher.
// Mutant `anchor-kill-h-minus-one` : les tests d'ancre tuent a h − 1, h depend
// de smax, donc le prefixe est brise (code 4 attendu).
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  int n = 1200;
  u64 smax = 6;
  u64 min_events = 1000;
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--family=", 0) == 0) { if (!parse_cloud_family(a.c_str() + 9, &family)) return 2; }
    else if (a.rfind("--n=", 0) == 0) n = std::atoi(a.c_str() + 4);
    else if (a.rfind("--smax=", 0) == 0) smax = (u64)std::atoll(a.c_str() + 7);
    else if (a.rfind("--min-events=", 0) == 0) min_events = (u64)std::atoll(a.c_str() + 13);
    else if (a.rfind("--inject=", 0) == 0) inject = a.substr(9);
    else return 2;
  }
  if (smax < 3 || smax > 10) return 2;
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool mutant = MHGP5_MUTANT("anchor-kill-h-minus-one");
  const std::vector<InputPoint> in = make_family_input(family, n, cloud_family_default_coord(family, n), 3);
  RunOptions full, pre;
  full.threads = pre.threads = 4;
  full.digest = pre.digest = true;
  full.smax = 11;
  pre.smax = smax;
  const RunResult rf = run_pipeline(in, full);
  const RunResult rp = run_pipeline(in, pre);
  if (rf.status != PipelineStatus::kCompleteRegular || rp.status != PipelineStatus::kCompleteRegular) {
    std::printf("REFUS : statuts %d / %d\n", (int)rf.status, (int)rp.status);
    return 2;
  }
  if (rp.kmax_eff != smax - 1) { std::printf("DESACCORD : kmax_eff=%llu attendu %llu\n", (unsigned long long)rp.kmax_eff, (unsigned long long)(smax - 1)); return 1; }
  u64 mism = 0;
  for (u64 K = 1; K <= rp.kmax_eff; ++K) {
    const bool same = rf.digest_forest[K] == rp.digest_forest[K] && rf.cards[K].events == rp.cards[K].events &&
                      rf.cards[K].facets == rp.cards[K].facets && rf.cards[K].deltas == rp.cards[K].deltas;
    if (!same) ++mism;
    std::printf("K=%llu evenements=%llu/%llu facettes=%llu/%llu deltas=%llu/%llu digest %s\n", (unsigned long long)K,
                (unsigned long long)rf.cards[K].events, (unsigned long long)rp.cards[K].events, (unsigned long long)rf.cards[K].facets,
                (unsigned long long)rp.cards[K].facets, (unsigned long long)rf.cards[K].deltas, (unsigned long long)rp.cards[K].deltas,
                same ? "egal" : "DIFFERENT");
  }
  const u64 top_events = rp.cards[rp.kmax_eff].events;
  std::printf("prefix_gate famille=%s n=%d smax=%llu ordres=%llu desaccords=%llu evenements_ordre_max=%llu boules=%llu/%llu\n",
              cloud_family_name(family), n, (unsigned long long)smax, (unsigned long long)rp.kmax_eff, (unsigned long long)mism,
              (unsigned long long)top_events, (unsigned long long)rf.expand.unique_balls, (unsigned long long)rp.expand.unique_balls);
  if (top_events < min_events) { std::printf("PLANCHER : %llu evenements a l'ordre %llu < %llu\n", (unsigned long long)top_events, (unsigned long long)rp.kmax_eff, (unsigned long long)min_events); return 3; }
  if (mutant) {
    if (mism) return 4;
    std::printf("MUTANT NON TUE\n");
    return 1;
  }
  if (mism) return 1;
  std::printf("prefix_gate OK\n");
  return 0;
}
