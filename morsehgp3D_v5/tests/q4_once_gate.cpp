// MorseHGP3D v5 — porte de l'EXACT-ONCE du seed q4 (lemme du prefixe
// ternaire : un tetraedre bien centre est emis une fois, depuis le carrier de
// plus petit PointId parmi ses faces aigues incidentes a l'arete maximale).
// Sur un nuage sans quadruplets cospheriques (uniform, emprise 50000, petit n),
// chaque boule q4 a UN tetraedre : le nombre d'emissions de la lane q4 doit
// egaler le nombre de cles d'arite 4 post-RLE. Mutant `q4-no-canonical`
// (toutes les faces aigues emettent) : emissions > cles -> code 4.
// Codes : 0, 3 (plancher ou doublons nominaux), 4.
#include <cstdio>
#include <string>

#include "../src/cloud/families.hpp"
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
  const CloudIndex ix = build_cloud_index(make_family_input(CloudFamily::kUniform, 150, 50000, 7));
  if (!ix.valid) return 2;
  std::vector<BallCandidate> cands;
  GenerateStats gs;
  GenerateOptions go;
  go.threads = 4;
  generate_candidates(ix, go, &cands, &gs);
  const u64 emitted_q4 = gs.candidates[2];
  rle_candidates(&cands);
  u64 keys_q4 = 0, keys_q4_from_lower = 0;
  for (const BallCandidate& c : cands) {
    if (c.arity == 4) ++keys_q4;
  }
  (void)keys_q4_from_lower;
  std::printf("q4_once_gate emissions_q4=%llu cles_arite4=%llu\n", (unsigned long long)emitted_q4, (unsigned long long)keys_q4);
  const bool dup = emitted_q4 > keys_q4;
  if (!inject.empty()) {
    if (dup) { std::fprintf(stderr, "MUTANT TUE : %s\n", inject.c_str()); return 4; }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s survivant\n", inject.c_str());
    return 3;
  }
  if (dup || keys_q4 < 200) return 3;
  std::printf("q4_once_gate OK\n");
  return 0;
}
