// MorseHGP3D v5 — PORTE : la corde doit voir les sites certifies P > 0.
//
// Le morceau i de la corde teste v_j = L - (2j-4) mu_hat B aux deux extremites.
// Pour un morceau EXTERIEUR (c = 2j-4 != 0), v_j peut etre STRICTEMENT NEGATIF
// alors meme que L > 0, des que c mu_hat B > L. Sauter un site certifie P > 0
// avant `ChordPieces::update` perd donc de vrais temoins de corde.
//
// Le defaut est FAIL-OPEN : il ne cree aucune fausse mort et ne change pas
// l objet — `digest_all` est identique dans les deux etats. Il ne peut donc pas
// etre tue par une porte de digest. Il se voit sur le COMPTEUR : le mutant
// `chord-skip-positive` fait BAISSER `seeds_killed_chord`. La porte est donc un
// PLANCHER, l idiome du depot contre le vert-par-vacuite, place stictement entre
// les deux valeurs mesurees.
//
// Codes : 0 conforme ; 2 refus avant calcul ; 4 plancher viole (mutant tue).
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/core/mutants.hpp"
#include "../src/pipeline/generate.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kTerrain;
  int n = 2000, coord = 0;
  long long seed = 3;
  u64 min_corde = 0, min_seeds = 0;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--family=", 0) == 0) { if (!parse_cloud_family(a.c_str() + 9, &family)) return 2; }
    else if (a.rfind("--n=", 0) == 0) n = std::atoi(a.c_str() + 4);
    else if (a.rfind("--seed=", 0) == 0) seed = std::atoll(a.c_str() + 7);
    else if (a.rfind("--min-corde=", 0) == 0) min_corde = (u64)std::atoll(a.c_str() + 12);
    else if (a.rfind("--min-seeds=", 0) == 0) min_seeds = (u64)std::atoll(a.c_str() + 12);
    else if (a.rfind("--inject=", 0) == 0) { if (!mutants_enable(a.c_str() + 9)) return 2; }
    else return 2;
  }
  if (n < 4) return 2;
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const CloudIndex ix = build_cloud_index(make_family_input(family, n, coord, seed));
  if (!ix.valid || ix.has_duplicate_positions()) return 2;

  GenerateOptions opt;
  opt.s = 8;
  opt.smax = 11;
  opt.threads = 1;
  std::vector<BallCandidate> out;
  GenerateStats st;
  generate_candidates(ix, opt, &out, &st);

  std::printf("chord_positive famille=%s n=%d seeds_q4=%llu corde_tues=%llu coeur_tues=%llu candidats_q4=%llu\n",
              cloud_family_name(family), n, (unsigned long long)st.seeds[1],
              (unsigned long long)st.seeds_killed_chord, (unsigned long long)st.seeds_killed_core,
              (unsigned long long)st.candidates[2]);

  // PLANCHER DE NON-VACUITE : sans seeds, le plancher de corde serait vert par
  // vacuite. Les deux planchers sont donc exiges ensemble.
  if (st.seeds[1] < min_seeds) {
    std::printf("REFUS : plancher de seeds q4 (%llu < %llu)\n",
                (unsigned long long)st.seeds[1], (unsigned long long)min_seeds);
    return 4;
  }
  if (st.seeds_killed_chord < min_corde) {
    std::printf("MUTANT TUE : la corde a perdu des temoins P > 0 (%llu < %llu)\n",
                (unsigned long long)st.seeds_killed_chord, (unsigned long long)min_corde);
    return 4;
  }
  return 0;
}
