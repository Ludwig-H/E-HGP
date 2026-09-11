// MorseHGP3D v6 — PORTE PILOTE STUB (C5 local) : le pipeline COMPLET avec la
// route serie C (RunOptions::prefilter_census_override, kernels en stub
// hote) produit un OBJET IDENTIQUE a la route CPU de production — digest_all,
// digest_balls, digest_postprefilter (survivants + profondeurs !), les dix
// digests de foret, cartes et totaux — sur deux familles x deux tailles.
// Preuve C++ hote de bout en bout, jamais un recu device ni de performance.
//
// Refus TRANSACTIONNEL du run entier (§ 5.5 recus) exerce par mutants :
//   gpu-stack-shallow   : la pile coupee fait refuser le RUN (invariant),
//                         la route CPU reste verte — divergence = TUE ;
//   gpu-census-nonstrict: coquilles comptees interieures => census contredit
//                         la passe count-only => refus invariant = TUE.
// Codes : 0 ; 1 desaccord ; 2 refus ; 3 plancher ; 4 mutant tue.
#define MHGP7_FAKE_DEVICE 1
#include "cuda_stub.hpp"

#include <cstdio>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/gpu/pilot.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp7;

namespace {
int failures = 0;
void expect(bool ok, const char* what) {
  if (!ok) {
    std::printf("ECHEC : %s\n", what);
    ++failures;
  } else {
    std::printf("ok : %s\n", what);
  }
}
}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--inject=", 0) == 0) inject = a.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  u32 mut = 0;
  if (MHGP7_MUTANT("gpu-stack-shallow")) mut |= gpu::kMutStackShallow;
  if (MHGP7_MUTANT("gpu-census-nonstrict")) mut |= gpu::kMutCensusNonstrict;
  if (MHGP7_MUTANT("gpu-skip-ball-write")) mut |= gpu::kMutSkipBallWrite;
  if (MHGP7_MUTANT("gpu-nshell-overdomain")) mut |= gpu::kMutNshellOverdomain;
  if (MHGP7_MUTANT("gpu-skip-count-write")) mut |= gpu::kMutSkipCountWrite;
  const bool mutant = mut != 0;

  // ---- TRANSACTIONNALITE DE LA ROUTE, appelee DIRECTEMENT (bc5812dc : le
  // RunResult vide ne distingue pas le commit local de l'invalidation de
  // l'enveloppe) : sorties et statistiques CANARISEES, faute tardive forcee
  // par drapeau — identite avant/apres exigee ; et le cas nb=0 PUBLIE des
  // sorties vides.
  if (!mutant) {
    const std::vector<InputPoint> in400 =
        make_family_input(CloudFamily::kUniform, 400, cloud_family_default_coord(CloudFamily::kUniform, 400), 3);
    const CloudIndex ix = build_cloud_index(in400);
    GenerateOptions go;
    go.s = 8;
    go.smax = 11;
    go.threads = 4;
    std::vector<BallCandidate> cands;
    GenerateStats gs;
    generate_candidates(ix, go, &cands, &gs);
    sort_candidates(&cands, 4);
    deduplicate_candidates(&cands);
    const auto canary_surv = std::vector<Survivor>{Survivor{123u, 9u}};
    std::vector<BallData> canary_balls(1);
    canary_balls[0].arity = 7;
    // Faute TARDIVE forcee (drapeau nshell hors domaine, i%4096==3 — bien
    // apres le debut de la reconstruction) : les sorties canarisees doivent
    // rester IDENTIQUES et les stats ne pas bouger.
    std::vector<Survivor> sv = canary_surv;
    std::vector<BallData> bd = canary_balls;
    ExpandStats st{};
    st.census_interior = 777;
    const std::string err =
        gpu::stub_prefilter_census_route(ix, cands, 11, 12, &sv, &bd, &st, gpu::kMutNshellOverdomain);
    expect(!err.empty(), "route directe : la faute tardive forcee refuse");
    expect(sv.size() == 1 && sv[0].idx == 123u && sv[0].depth == 9u && bd.size() == 1 && bd[0].arity == 7 &&
               st.census_interior == 777,
           "route directe : sorties et stats CANARISEES identiques apres refus (commit local transactionnel)");
    // Cas VIDE : publication de sorties vides, jamais les canaris conserves.
    std::vector<Survivor> sv2 = canary_surv;
    std::vector<BallData> bd2 = canary_balls;
    ExpandStats st2{};
    const std::vector<BallCandidate> none;
    const std::string err2 = gpu::stub_prefilter_census_route(ix, none, 11, 12, &sv2, &bd2, &st2, 0);
    expect(err2.empty() && sv2.empty() && bd2.empty(), "route directe : nb=0 publie des sorties VIDES");
  }

  u64 compared = 0;
  for (const CloudFamily fam : {CloudFamily::kUniform, CloudFamily::kEightClusters}) {
    for (const int n : {400, 2000}) {
      const std::vector<InputPoint> in = make_family_input(fam, n, cloud_family_default_coord(fam, n), 3);
      RunOptions cpu;
      cpu.s = 8;
      cpu.smax = 11;
      cpu.threads = 4;
      cpu.digest = true;
      const RunResult a = run_pipeline(in, cpu);
      if (a.status != PipelineStatus::kCompleteRegular) return 2;
      RunOptions dev = cpu;
      dev.prefilter_census_override = [&](const CloudIndex& ix, const std::vector<BallCandidate>& cands,
                                          u64 smax_eff, size_t shell_cap, std::vector<Survivor>* sv,
                                          std::vector<BallData>* bd, ExpandStats* st) {
        return gpu::stub_prefilter_census_route(ix, cands, smax_eff, shell_cap, sv, bd, st, mut);
      };
      const RunResult b = run_pipeline(in, dev);
      if (mutant) {
        // Le mutant doit provoquer le REFUS TRANSACTIONNEL du run entier
        // (invariant), la route CPU restant verte, TOUT provisoire vide —
        // sur AU MOINS une configuration (la pile courte peut ne mordre
        // qu'aux arbres plus profonds : on continue, jamais un verdict
        // premature).
        const bool refused = b.status == PipelineStatus::kInvariantViolated &&
                             b.message.find("route serie C") != std::string::npos;
        const bool clean = b.digest_all.empty() && b.digest_forest.empty() && b.cards.empty();
        if (refused && clean) {
          std::printf("mutant %s TUE : refus transactionnel du run entier (%s %d)\n", inject.c_str(),
                      cloud_family_name(fam), n);
          return 4;
        }
        continue;
      }
      bool same = b.status == PipelineStatus::kCompleteRegular;
      same = same && a.digest_all == b.digest_all && a.digest_balls == b.digest_balls &&
             a.digest_postprefilter == b.digest_postprefilter && a.digest_forest == b.digest_forest;
      same = same && a.cards == b.cards && a.total_events == b.total_events &&
             a.total_facets == b.total_facets && a.total_fusions == b.total_fusions &&
             a.total_deltas == b.total_deltas && a.total_nodes == b.total_nodes;
      same = same && a.emitted == b.emitted && a.expand.survivors == b.expand.survivors;
      char what[128];
      std::snprintf(what, sizeof(what), "objet identique CPU vs route serie C : %s n=%d",
                    cloud_family_name(fam), n);
      expect(same, what);
      ++compared;
    }
  }
  if (mutant) {
    std::printf("MUTANT NON TUE (aucune configuration n'a refuse)\n");
    return 1;
  }
  if (failures) return 1;
  if (compared < 4) {
    std::printf("PLANCHER : %llu comparaisons\n", (unsigned long long)compared);
    return 3;
  }
  return 0;
}

