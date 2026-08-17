// MorseHGP3D v4 — ORDRE SEMANTIQUE DES NIVEAUX q4 : U320 contre l'oracle.
//
// Audit « lemme prefixe et niveau » § 2 : le representant du niveau q4 est
// NON reduit — l'egalite de representation (operator==) n'est pas une
// egalite de niveau. Ce test :
//   - recolte les niveaux de TOUS les supports q4 (centre strictement
//     interieur) de trois nuages, dont la GRANDE COSPHERE : ses tetraedres
//     cospheriques partagent la meme boule avec des (|N'|², det²)
//     DIFFERENTS — le plateau semantique a representations distinctes est
//     garanti par construction, c'est exactement le cas que les macro-lots
//     doivent grouper et que operator== raterait ;
//   - juge toutes les paires par `compare_exact_level` (U320) CONTRE
//     l'oracle obigint 384 bits (produits croises en OBig) ;
//   - verifie l'antisymetrie, et la coherence de la promotion q3 :
//     compare_level (U192) et compare_exact_level (U320 apres promotion)
//     doivent rendre le meme verdict sur les niveaux q3 recoltes ;
//   - PLANCHERS : >= 1 paire a meme niveau semantique ET representations
//     differentes ; >= 200 niveaux q4 ;
//   - fixture de largeur gravee : produits croises egaux sur 256 bits,
//     differents au seul mot w[4] ; mutant --inject=level320-trunc-hi tue.
// Codes : 0 accord ; 1 desaccord ; 3 plancher/invariant ; 4 mutant tue.
#include <cstdio>
#include <cstring>
#include <set>
#include <vector>

#include "../oracle/obigint.hpp"
#include "../src/cloud/families.hpp"
#include "../src/events/q4_event.hpp"

namespace {

using namespace mhgp4;
using OB = mhgp4_oracle::OBig<6>;

OB ob(i128 v) { return OB::from_i128(v); }

OB ob_words3(const u64 w[3]) {
  OB r;
  r.w[0] = w[0];
  r.w[1] = w[1];
  r.w[2] = w[2];
  return r;
}

int g_fail = 0;
void fail(const char* what) {
  std::fprintf(stderr, "DESACCORD niveau q4 : %s\n", what);
  ++g_fail;
}

std::vector<P3> cosphere_cloud(i64 cx, const int pat[3], const P3& inner) {
  std::vector<P3> pts;
  int perm[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
  std::set<long long> seen;
  for (auto& pr : perm)
    for (int sx = -1; sx <= 1; sx += 2)
      for (int sy = -1; sy <= 1; sy += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
          const i64 x = cx + sx * pat[pr[0]];
          const i64 y = cx + sy * pat[pr[1]];
          const i64 z = cx + sz * pat[pr[2]];
          if (x < 0 || y < 0 || z < 0) continue;
          const long long key = (x << 34) | (y << 17) | z;
          if (!seen.insert(key).second) continue;
          pts.push_back(P3{x, y, z});
        }
  pts.push_back(P3{cx, cx, cx});
  pts.push_back(inner);
  return pts;
}

std::vector<P3> equilateral_max_cloud() {
  const i64 M = 65535;
  return {{0, 0, 0}, {M, M, 0}, {M, 0, M}, {0, M, M}, {M, M, M},
          {30000, 20000, 10000}};
}

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp4;
  bool mutant = false;
  for (int i = 1; i < argc; ++i)
    if (std::strcmp(argv[i], "--inject=level320-trunc-hi") == 0) mutant = true;

  const int big_pat[3] = {12000, 16000, 0};
  std::vector<std::vector<P3>> clouds;
  clouds.push_back(make_family_cloud(CloudFamily::kUniform, 22,
                                     cloud_family_default_coord(CloudFamily::kUniform, 22), 3));
  clouds.push_back(equilateral_max_cloud());
  clouds.push_back(cosphere_cloud(32768, big_pat, P3{30000, 30000, 30000}));

  // 1. Recolte : niveaux de tous les supports q4 (centre strictement
  // interieur), toutes aretes maximales confondues (le niveau est un
  // invariant de la boule, l'etiquetage est indifferent).
  std::vector<Q4Level> levels;
  for (const std::vector<P3>& pts : clouds) {
    const size_t m = pts.size();
    for (size_t i = 0; i < m; ++i)
      for (size_t j = i + 1; j < m; ++j)
        for (size_t k = j + 1; k < m; ++k)
          for (size_t l = k + 1; l < m; ++l) {
            const Q4Form f4 = q4_form(pts[i], pts[j], pts[k], pts[l]);
            if (f4.det == 0) continue;
            if (!q4_center_strictly_inside(f4, pts[i], pts[j], pts[k], pts[l]))
              continue;
            levels.push_back(q4_level_raw(f4));
          }
  }
  if (levels.size() < 200) {
    std::fprintf(stderr, "PLANCHER : %zu niveaux q4 (< 200)\n", levels.size());
    return 3;
  }

  // 2. Toutes les paires contre l'oracle 384 bits ; antisymetrie ; compte
  // des plateaux semantiques a representations differentes.
  u64 pairs = 0, semantic_plateaus_diff_repr = 0;
  for (size_t i = 0; i < levels.size(); ++i)
    for (size_t j = i + 1; j < levels.size(); ++j) {
      const Q4Level &x = levels[i], &y = levels[j];
      const int got = compare_exact_level(x, y, mutant);
      const int back = compare_exact_level(y, x, mutant);
      const int want = cmp(ob_words3(x.num) * ob(y.den), ob_words3(y.num) * ob(x.den));
      if (got != want) fail("produit croise");
      if (back != -got) fail("antisymetrie");
      if (want == 0 && !same_level_representation(x, y))
        ++semantic_plateaus_diff_repr;
      ++pairs;
    }
  if (mhgp4_oracle::overflow_seen()) {
    std::fprintf(stderr, "REFUS numeric_failure : debordement obigint\n");
    return 3;
  }

  // 3. Coherence de la promotion q3 : U192 et U320 rendent le meme verdict
  // sur des niveaux q3 exacts (tous les triangles aigus d'un petit nuage).
  {
    const std::vector<P3>& pts = clouds[0];
    std::vector<Q3Level> l3;
    for (size_t i = 0; i < pts.size(); ++i)
      for (size_t j = i + 1; j < pts.size(); ++j)
        for (size_t k = j + 1; k < pts.size(); ++k) {
          const P3 &a = pts[i], &b = pts[j], &x = pts[k];
          const bool acute = p3_dot(p3_sub(b, a), p3_sub(x, a)) > 0 &&
                             p3_dot(p3_sub(a, b), p3_sub(x, b)) > 0 &&
                             p3_dot(p3_sub(a, x), p3_sub(b, x)) > 0;
          if (acute) l3.push_back(q3_exact_level(a, b, x));
        }
    for (size_t i = 0; i < l3.size(); ++i)
      for (size_t j = i + 1; j < l3.size(); ++j) {
        const int v192 = compare_level(l3[i], l3[j]);
        const int v320 = compare_exact_level(promote_q3_level(l3[i]),
                                             promote_q3_level(l3[j]), mutant);
        if (v192 != v320) fail("promotion q3");
      }
  }

  // 4. Fixture de largeur gravee : produits croises egaux sur les 256 bits
  // bas, differents au seul mot w[4] (num < 2^146 et den < 2^114
  // respectes ; representants non reduits, l'ordre ne l'exige pas).
  {
    Q4Level x{{0, 0, 0}, (i128)1 << 113};
    x.num[2] = (u64)1 << 17;  // num = 2^145
    Q4Level y{{0, 0, 0}, (i128)1 << 113};
    y.num[2] = ((u64)1 << 17) | ((u64)1 << 15);  // num = 2^145 + 2^143
    const int got = compare_exact_level(x, y, mutant);
    if (got != -1) fail("fixture mot w[4] (x < y attendu)");
  }

  std::printf(
      "q4_level_compare : %zu niveaux, %llu paires, "
      "plateaux_semantiques_repr_differentes=%llu, desaccords=%d\n",
      levels.size(), (unsigned long long)pairs,
      (unsigned long long)semantic_plateaus_diff_repr, g_fail);
  if (mutant) {
    if (g_fail > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  if (g_fail > 0) return 1;
  if (semantic_plateaus_diff_repr == 0) {
    std::fprintf(stderr,
                 "PLANCHER : aucun plateau semantique a representations "
                 "differentes — le contrat macro-lot n'est pas exerce\n");
    return 3;
  }
  return 0;
}
