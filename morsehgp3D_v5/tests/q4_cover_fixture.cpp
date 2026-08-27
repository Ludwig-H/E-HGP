// MorseHGP3D v5 — FIXTURE DIFFERENTIELLE DU COVER q4 (audit bloquant 87e915bd).
//
// Sur `eight_clusters n=1200` (graine 3, emprise par defaut), la boule de cle
// primitive (2712, -198919, -939434, -201167, 88336155) a une profondeur
// globale EXACTE de 8 = h_4 (smax = 11) : elle est morte pour la foret, mais
// le cover d'ancre au coefficient 3 n'en voit que 7 interieurs — le huitieme
// verifie 3D² = 1215 < |2z−a−b|² = 1237 <= 4D² = 1620. La v4 (coefficient 3)
// EMET donc ce candidat et le prefiltre exact le tue ensuite ; un cover au
// coefficient 4 le tue avant l'emission et change `digest_balls` sans changer
// la foret. Contrat v5 (conformite stricte) : la cle est PRESENTE post-RLE
// (arite 4), ABSENTE des survivantes du prefiltre, et le census global lui
// compte exactement 8 interieurs. Mutant `q4-cover-coef4` : cle absente
// post-RLE -> code 4.
// Codes : 0 conforme, 3 contrat viole, 4 mutant tue.
#include <cstdio>
#include <cstring>
#include <string>

#include "../src/cloud/families.hpp"
#include "../src/pipeline/expand.hpp"
#include "../src/pipeline/generate.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const int n = 1200;
  const CloudIndex ix = build_cloud_index(make_family_input(CloudFamily::kEightClusters, n, 0, 3));
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  const BallKey target{2712, {-198919, -939434, -201167}, 88336155};
  GenerateOptions go;
  go.threads = 8;
  std::vector<BallCandidate> cands;
  GenerateStats gs;
  generate_candidates(ix, go, &cands, &gs);
  rle_candidates(&cands);
  const BallCandidate* found = nullptr;
  for (const BallCandidate& c : cands)
    if (c.key == target) { found = &c; break; }
  u64 depth = 0;
  const bool deep = ball_depth_at_least(ix, target, 8, &depth);
  std::vector<i32> in, sh;
  ball_census(ix, target, 64, 64, &in, &sh);
  std::printf("q4_cover_fixture candidats=%zu cle_presente=%d arite=%d profondeur>=8=%d census_interieurs=%zu coquille=%zu\n",
              cands.size(), found != nullptr, found ? (int)found->arity : -1, (int)deep, in.size(), sh.size());
  int bad = 0;
  if (!(in.size() == 8 && deep)) { std::fprintf(stderr, "profondeur globale != 8\n"); ++bad; }
  if (!found || found->arity != 4) { std::fprintf(stderr, "cle absente post-RLE ou arite != 4\n"); ++bad; }
  if (found) {
    std::vector<Survivor> surv;
    ExpandStats es;
    prefilter_balls(ix, cands, 11, 8, &surv, &es);
    bool survives = false;
    for (const Survivor& s : surv)
      if (cands[s.idx].key == target) survives = true;
    if (survives) { std::fprintf(stderr, "la cle survit au prefiltre\n"); ++bad; }
  }
  if (!inject.empty()) {
    if (bad) { std::fprintf(stderr, "MUTANT TUE : %s\n", inject.c_str()); return 4; }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s survivant\n", inject.c_str());
    return 3;
  }
  if (bad) return 3;
  std::printf("q4_cover_fixture OK\n");
  return 0;
}
