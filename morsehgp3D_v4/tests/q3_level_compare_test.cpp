// MorseHGP3D v4 — ORDRE EXACT DES NIVEAUX q3 : U192 contre l'oracle 384 bits.
//
// Contre-audit 489c617 § 2 : la foret exige `num1·den2 ? num2·den1` sans
// overflow ; chaque produit croise < 2^171 (num < 2^101, den < 2^70), donc
// 192 bits non signes avec marge — i128 ne suffit pas. Ce test :
//   - recolte les niveaux de TOUS les triangles aigus de trois nuages
//     (uniform n=32, equilateral maximal M=65535, presque-rectangle aigu) ;
//   - compare toutes les paires par `compare_level` (U192) CONTRE l'oracle
//     obigint 384 bits (produits croises en OBig — autre arithmetique) ;
//   - verifie l'antisymetrie et la canonicite (egalite de niveau ⟺ egalite
//     des fractions reduites) ;
//   - PLANCHER : au moins un PLATEAU (deux triangles distincts de meme
//     niveau — l'equilateral maximal en fournit par symetrie) : c'est le cas
//     qui imposera les macro-lots a la foret ;
//   - fixture de largeur gravee : deux fractions dont les produits croises
//     ne different QUE dans le mot haut (bits >= 128) ;
//   - mutant --inject=level-trunc-hi (mot haut tronque) : tue par la fixture
//     de largeur, prouvant que les comparaisons traversent les bits hauts.
// Codes : 0 accord total ; 1 desaccord ; 3 plancher/invariant ; 4 mutant tue.
#include <cstdio>
#include <cstring>
#include <vector>

#include "../oracle/obigint.hpp"
#include "../src/cloud/families.hpp"
#include "../src/events/q3_event.hpp"

namespace {

using namespace mhgp4;
using OB = mhgp4_oracle::OBig<6>;

OB ob(i128 v) { return OB::from_i128(v); }

int g_fail = 0;
void fail(const char* what) {
  std::fprintf(stderr, "DESACCORD niveau : %s\n", what);
  ++g_fail;
}

std::vector<P3> equilateral_max_cloud() {
  const i64 M = 65535;
  return {{0, 0, 0}, {M, M, 0}, {M, 0, M}, {0, M, M}, {M, M, M},
          {30000, 20000, 10000}};
}

std::vector<P3> near_right_cloud() {
  return {{0, 0, 0}, {40000, 0, 0}, {20000, 20001, 0}, {20000, 10000, 1000},
          {1000, 19000, 2000}};
}

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp4;
  bool mutant = false;
  for (int i = 1; i < argc; ++i)
    if (std::strcmp(argv[i], "--inject=level-trunc-hi") == 0) mutant = true;

  // 1. Recolte des niveaux de tous les triangles aigus (le niveau
  // D·E·X/(4G) est symetrique : produit des trois aretes carrees sur 16
  // fois l'aire carree — l'etiquetage du triangle est indifferent).
  std::vector<std::vector<P3>> clouds;
  clouds.push_back(make_family_cloud(CloudFamily::kUniform, 32,
                                     cloud_family_default_coord(CloudFamily::kUniform, 32), 3));
  clouds.push_back(equilateral_max_cloud());
  clouds.push_back(near_right_cloud());
  std::vector<Q3Level> levels;
  for (const std::vector<P3>& pts : clouds)
    for (size_t i = 0; i < pts.size(); ++i)
      for (size_t j = i + 1; j < pts.size(); ++j)
        for (size_t k = j + 1; k < pts.size(); ++k) {
          const P3 &a = pts[i], &b = pts[j], &x = pts[k];
          const bool acute = p3_dot(p3_sub(b, a), p3_sub(x, a)) > 0 &&
                             p3_dot(p3_sub(a, b), p3_sub(x, b)) > 0 &&
                             p3_dot(p3_sub(a, x), p3_sub(b, x)) > 0;
          if (!acute) continue;
          levels.push_back(q3_exact_level(a, b, x));
        }
  if (levels.size() < 200) {
    std::fprintf(stderr, "PLANCHER : %zu niveaux recoltes (< 200)\n",
                 levels.size());
    return 3;
  }

  // 2. Toutes les paires contre l'oracle 384 bits + antisymetrie +
  // canonicite ; comptage des plateaux (paires de niveau egal).
  u64 pairs = 0, plateaux = 0;
  for (size_t i = 0; i < levels.size(); ++i)
    for (size_t j = i + 1; j < levels.size(); ++j) {
      const Q3Level &x = levels[i], &y = levels[j];
      const int got = compare_level(x, y, mutant);
      const int back = compare_level(y, x, mutant);
      const int want =
          cmp(ob(x.num) * ob(y.den), ob(y.num) * ob(x.den));
      if (got != want) fail("produit croise");
      if (back != -got) fail("antisymetrie");
      if (want == 0) {
        ++plateaux;
        // Fractions canoniques : meme rationnel => memes (num, den).
        if (x.num != y.num || x.den != y.den) fail("canonicite");
      }
      ++pairs;
    }
  if (mhgp4_oracle::overflow_seen()) {
    std::fprintf(stderr, "REFUS numeric_failure : debordement obigint\n");
    return 3;
  }

  // 3. Fixture de largeur gravee : produits croises egaux sur les 128 bits
  // bas, differents seulement dans le mot haut (bits >= 128). num < 2^101 et
  // den < 2^70 sont respectes ; ces fractions ne sont pas reduites, l'ordre
  // ne l'exige pas.
  {
    const Q3Level x{(i128)1 << 100, (i128)1 << 69};
    const Q3Level y{((i128)1 << 100) + ((i128)1 << 71), (i128)1 << 69};
    const int got = compare_level(x, y, mutant);
    if (got != -1) fail("fixture mot haut (x < y attendu)");
  }

  std::printf(
      "q3_level_compare : %zu niveaux, %llu paires, %llu plateaux, "
      "desaccords=%d\n",
      levels.size(), (unsigned long long)pairs, (unsigned long long)plateaux,
      g_fail);
  if (mutant) {
    if (g_fail > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  if (g_fail > 0) return 1;
  if (plateaux == 0) {
    std::fprintf(stderr,
                 "PLANCHER : aucun plateau de niveau — la porte macro-lot "
                 "n'est pas exercee\n");
    return 3;
  }
  return 0;
}
