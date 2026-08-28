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
#include "../src/pipeline/digest.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

namespace {
// Signature des EVENEMENTS canoniques (q, d, masque, support, interieurs, niveau) et des NIVEAUX DE LOTS d'un
// ordre — ce que le digest v4 (facettes, partition, deltas) ne couvre pas (audit du passage a l'echelle, P2).
struct KSig {
  std::string events, batch_levels;
  u64 nevents = 0, nbatches = 0;
};
KSig sign_forest(const std::vector<ForestEvent>& events, const ForestResult& r) {
  KSig k;
  digest_detail::Writer we;
  we.tag("mhgp5-prefix-gate:events");
  we.u64v((u64)events.size());
  for (const ForestEvent& e : events) {
    we.u8v(e.q);
    we.u8v(e.d);
    we.u32v(e.active_mask);
    for (int t = 0; t < (int)e.q; ++t) we.u32v(e.support[(size_t)t]);
    for (int t = 0; t < (int)e.d; ++t) we.u32v(e.interior[(size_t)t]);
    we.level(e.level);
  }
  k.events = we.hex();
  digest_detail::Writer wb;
  wb.tag("mhgp5-prefix-gate:batch_levels");
  wb.u64v((u64)r.batch_levels.size());
  for (const ExactLevel& l : r.batch_levels) wb.level(l);
  k.batch_levels = wb.hex();
  k.nevents = events.size();
  k.nbatches = r.batch_levels.size();
  return k;
}
}  // namespace

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  int n = 1200, coord = 0;
  u64 smax = 6;
  u64 min_events = 1000, min_plateau_batches = 0;
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--family=", 0) == 0) { if (!parse_cloud_family(a.c_str() + 9, &family)) return 2; }
    else if (a.rfind("--n=", 0) == 0) n = std::atoi(a.c_str() + 4);
    else if (a.rfind("--coord=", 0) == 0) coord = std::atoi(a.c_str() + 8);
    else if (a.rfind("--min-plateau-batches=", 0) == 0) min_plateau_batches = (u64)std::atoll(a.c_str() + 22);
    else if (a.rfind("--smax=", 0) == 0) smax = (u64)std::atoll(a.c_str() + 7);
    else if (a.rfind("--min-events=", 0) == 0) min_events = (u64)std::atoll(a.c_str() + 13);
    else if (a.rfind("--inject=", 0) == 0) inject = a.substr(9);
    else return 2;
  }
  if (smax < 3 || smax > 10) return 2;
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool mutant = MHGP5_MUTANT("anchor-kill-h-minus-one");
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const std::vector<InputPoint> in = make_family_input(family, n, coord, 3);
  RunOptions full, pre;
  full.threads = pre.threads = 4;
  full.digest = pre.digest = true;
  full.smax = 11;
  pre.smax = smax;
  std::vector<KSig> sf(12), sp(12);
  full.on_forest = [&](u64 K, const std::vector<ForestEvent>& ev, const ForestResult& r) { sf[K] = sign_forest(ev, r); };
  pre.on_forest = [&](u64 K, const std::vector<ForestEvent>& ev, const ForestResult& r) { sp[K] = sign_forest(ev, r); };
  const RunResult rf = run_pipeline(in, full);
  const RunResult rp = run_pipeline(in, pre);
  if (rf.status != PipelineStatus::kCompleteRegular || rp.status != PipelineStatus::kCompleteRegular) {
    std::printf("REFUS : statuts %d / %d\n", (int)rf.status, (int)rp.status);
    return 2;
  }
  if (rp.kmax_eff != smax - 1) { std::printf("DESACCORD : kmax_eff=%llu attendu %llu\n", (unsigned long long)rp.kmax_eff, (unsigned long long)(smax - 1)); return 1; }
  u64 mism = 0;
  for (u64 K = 1; K <= rp.kmax_eff; ++K) {
    const bool same_digest = rf.digest_forest[K] == rp.digest_forest[K] && rf.cards[K].events == rp.cards[K].events &&
                             rf.cards[K].facets == rp.cards[K].facets && rf.cards[K].deltas == rp.cards[K].deltas;
    const bool same_events = sf[K].events == sp[K].events && !sf[K].events.empty();
    const bool same_batches = sf[K].batch_levels == sp[K].batch_levels && !sf[K].batch_levels.empty();
    const bool same = same_digest && same_events && same_batches;
    if (!same) ++mism;
    std::printf("K=%llu evenements=%llu/%llu facettes=%llu/%llu deltas=%llu/%llu lots=%llu/%llu digest %s evenements %s niveaux_de_lots %s\n",
                (unsigned long long)K, (unsigned long long)rf.cards[K].events, (unsigned long long)rp.cards[K].events,
                (unsigned long long)rf.cards[K].facets, (unsigned long long)rp.cards[K].facets, (unsigned long long)rf.cards[K].deltas,
                (unsigned long long)rp.cards[K].deltas, (unsigned long long)sf[K].nbatches, (unsigned long long)sp[K].nbatches,
                same_digest ? "egal" : "DIFFERENT", same_events ? "egaux" : "DIFFERENTS", same_batches ? "egaux" : "DIFFERENTS");
  }
  const u64 top_events = rp.cards[rp.kmax_eff].events;
  // Plateaux non triviaux (lots de plusieurs evenements) a l'ordre maximal du prefixe : plancher optionnel.
  const u64 plateau_batches = sp[rp.kmax_eff].nevents > sp[rp.kmax_eff].nbatches ? sp[rp.kmax_eff].nevents - sp[rp.kmax_eff].nbatches : 0;
  if (plateau_batches < min_plateau_batches) {
    std::printf("PLANCHER : %llu evenements en exces sur les lots (plateaux) < %llu\n", (unsigned long long)plateau_batches, (unsigned long long)min_plateau_batches);
    return 3;
  }
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
