// MorseHGP3D v4 — FIXTURE GRAVEE : l'arrondi du rayon de la boule-cœur.
//
// a=(0,0,0), b=(3,1,0) : `‖ab‖² = 10` n'est pas un carré parfait, donc
// `floor_sqrt(40) = 6` et `ceil_sqrt(40) = 7` divergent en unités doublées.
// z_shell=(2,2,0) est EXACTEMENT sur la sphère diamétrale
// (`‖z−m‖² = 5/2 = ‖ab‖²/4`) : la coquille ne tue jamais.
// z_int=(1,0,0) est strictement intérieur.
//
// Avec `h = 2` : le compte vrai de témoins stricts vaut 1 → l'ancre est
// VIVANTE. La boule-cœur saine (floor) ne crédite que z_int. Le mutant
// `radius-ceil` (distance majorée au lieu de minorée) gonfle le rayon,
// crédite la coquille et publie une FAUSSE MORT : la porte le tue (code 4).
//
// Codes : 0 = comportement sain vérifié ; 3 = invariant violé ;
// 4 = mutant tué (mode --inject=radius-ceil).
#include <cstdio>
#include <cstring>

#include "../src/events/q2_witness_count.hpp"

int main(int argc, char** argv) {
  using namespace mhgp4;
  const bool mutant = argc > 1 && std::strcmp(argv[1], "--inject=radius-ceil") == 0;

  const std::vector<P3> pts = {{0, 0, 0}, {3, 1, 0}, {2, 2, 0}, {1, 0, 0}};
  const CloudIndex ix = build_cloud_index(pts);
  if (ix.unique_count() != 4) {
    std::fprintf(stderr, "fixture : 4 positions uniques attendues\n");
    return 3;
  }
  // Retrouver les index uniques de a et b par leurs coordonnees.
  i32 ua = -1, ub = -1;
  for (i32 u = 0; u < 4; ++u) {
    const P3& p = ix.upos[(size_t)u];
    if (p.x == 0 && p.y == 0 && p.z == 0) ua = u;
    if (p.x == 3 && p.y == 1 && p.z == 0) ub = u;
  }
  if (ua < 0 || ub < 0) return 3;

  const u64 h = 2;
  if (true_interior_count_q2(ix, ua, ub, h) != 1) {
    std::fprintf(stderr, "juge : compte interieur vrai != 1\n");
    return 3;
  }

  Q2CountOpts opts;
  opts.use_hmin = false;  // isoler l'autorite boule-cœur
  opts.mutant_ceil_distance = mutant;
  const u64 counted =
      count_universal_witnesses_q2(ix, (NodeRef)(-1 - ua), (NodeRef)(-1 - ub), h, opts);

  if (!mutant) {
    if (counted >= h) {
      std::fprintf(stderr, "FAUSSE MORT : la boule saine credite %llu temoins\n",
                   (unsigned long long)counted);
      return 3;
    }
    std::printf("q2_fixture OK : ancre vivante, %llu credit(s) sur h=2\n",
                (unsigned long long)counted);
    return 0;
  }
  if (counted >= h) {
    std::printf("MUTANT TUE : radius-ceil publie une fausse mort (%llu credits)\n",
                (unsigned long long)counted);
    return 4;
  }
  std::fprintf(stderr, "PORTE INEFFICACE : le mutant n'a pas ete discrimine\n");
  return 3;
}
