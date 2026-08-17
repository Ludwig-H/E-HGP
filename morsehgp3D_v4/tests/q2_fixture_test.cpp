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

#include "../src/events/spindle.hpp"
#include "../src/events/q2_witness_count.hpp"

// Fixture discriminante du rayon q4 (audit du 17 aout, § 6) :
// z est dans le cœur EXACT q4 (`2H² > Xi`) mais echoue au test conservateur
// `15U < 4L` (rayon D/√15 < D·sin 15°). Le test exact est
// `Y = 2L−U, Y > 0 et Y² > 3L²`.
static int check_q4_radius_fixture() {
  using namespace mhgp4;
  const P3 a{10000, 10000, 0}, b{20000, 10000, 0}, z{15000, 12585, 0};
  if (!in_spindle(Lane::kQ4, a, b, z)) {
    std::fprintf(stderr, "fixture rayon q4 : z devrait etre dans W_4\n");
    return 3;
  }
  const P3 u{2 * z.x - a.x - b.x, 2 * z.y - a.y - b.y, 2 * z.z - a.z - b.z};
  const i64 U = p3_norm2(u);
  const i64 L = p3_norm2(p3_sub(b, a));
  if (15 * U < 4 * L) {
    std::fprintf(stderr, "fixture rayon q4 : 15U<4L devrait echouer ici\n");
    return 3;
  }
  const i64 Y = 2 * L - U;
  if (!(Y > 0 && (i128)Y * Y > (i128)3 * L * L)) {
    std::fprintf(stderr, "fixture rayon q4 : le test exact Y devrait accepter\n");
    return 3;
  }
  return 0;
}

int main(int argc, char** argv) {
  using namespace mhgp4;
  const bool mutant = argc > 1 && std::strcmp(argv[1], "--inject=radius-ceil") == 0;
  if (!mutant) {
    const int rc = check_q4_radius_fixture();
    if (rc != 0) return rc;
  }

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
