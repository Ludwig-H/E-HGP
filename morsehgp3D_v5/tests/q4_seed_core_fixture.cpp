// MorseHGP3D v5 — FIXTURE-CŒUR du seed q4 (v4, audit « axial arbre et cœur de
// seed » § 4.2) : la frontiere du cœur seed-local n'est PAS comptee.
// Six points sur le cercle R² = 25 du plan z = 10 (centre (15,10)) — aucune
// paire antipodale — et y = (15,10,16) : le seed (a,b,x) = (11,7,10),
// (20,10,10), (15,15,10) est strictement aigu ; c1..c3 sont cocirculaires au
// seed (P = 0, B = 0 : sur toute sphere du faisceau, cas d'egalite
// 2P² = J·B²) ; la sphere du tetraedre (a,b,x,y) a R² = 3721/144 et ses c_i
// sont des points de COQUILLE (aucun interieur strict). Regle stricte : aucun
// temoin, la cle est emise a smax = 6. Mutant `q4-seed-core-nonstrict` : les
// cocirculaires comptent (0 >= 0), tout seed porte par le cercle meurt, et
// l'exact-once comme le centre hors tetraedre interdisent toute emission de
// secours : la cle disparait -> code 4.
#include <cstdio>
#include <string>

#include "../src/pipeline/census.hpp"
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
  const std::vector<P3> fx = {{11, 7, 10}, {20, 10, 10}, {15, 15, 10}, {11, 13, 10}, {18, 14, 10}, {12, 14, 10}, {15, 10, 16}};
  const CloudIndex ix = build_cloud_index(fx);
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  const Q4Form f4 = q4_form(fx[0], fx[1], fx[2], fx[6]);
  const BallKey key = ball_key_reduce(q4_ball_form(f4));
  // R² = |N'|²/det² = 3721/144 : verification gravee.
  const ExactLevel lvl = q4_level_raw(f4);
  const bool level_ok = same_exact_level(lvl, promote_level(Rational128{3721, 144}));
  std::vector<BallCandidate> cands;
  GenerateStats gs;
  GenerateOptions go;
  go.smax = 6;
  go.threads = 1;
  generate_candidates(ix, go, &cands, &gs);
  rle_candidates(&cands);
  bool present = false;
  for (const BallCandidate& c : cands)
    if (c.key == key) present = true;
  std::vector<i32> in, sh;
  ball_census(ix, key, 8, 8, &in, &sh);
  std::printf("q4_seed_core_fixture niveau_3721_144=%d cle_presente=%d interieurs=%zu coquille=%zu seeds_tues=%llu\n",
              (int)level_ok, (int)present, in.size(), sh.size(), (unsigned long long)gs.seeds_killed_core);
  int bad = 0;
  if (!level_ok || !in.empty() || sh.size() != 7) ++bad;
  if (!present) ++bad;
  if (!inject.empty()) {
    if (bad) { std::fprintf(stderr, "MUTANT TUE : %s\n", inject.c_str()); return 4; }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s survivant\n", inject.c_str());
    return 3;
  }
  if (bad) return 3;
  std::printf("q4_seed_core_fixture OK\n");
  return 0;
}
