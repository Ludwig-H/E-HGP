// Microbanc d'opportunite (audit L05) : etage des quadruplets de la feuille v10, sur les candidats REELS vides par le
// generateur (trame 02). Trois variantes, memes decisions exigees :
//   A : sequence du HEAD : center4 (i128) -> centre dans la boite (i128) -> interieur strict (4 orientations i128) ;
//   B : exacte reordonnee : interieur strict par barycentriques entieres (Gram, i128) d'abord, centre et boite ensuite ;
//   C : filtre flottant (double, borne d'erreur a priori) sur les barycentriques, repli exact B si indecis.
// Ce n'est pas du code produit : il sert a chiffrer un levier. Compilation : g++ -O3 -std=c++20 [-march=x86-64-v3].
#include <x86intrin.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "arith/geometry.hpp"

using namespace mhgp10;
using geom::P3;
constexpr int kT = 6;
struct Box { i64 lo[3], hi[3]; };
struct RecQ { int a[3], b[3], c[3], d[3], lo[3], hi[3]; };

// copie de generator.cpp:96-103 (HEAD afb081774)
static inline bool center_in_box(const P3& a, const geom::Center& c, const Box& Q) {
  const i64 av[3] = {a.x, a.y, a.z};
  for (int i = 0; i < 3; ++i) {
    const i128 v = (i128(av[i]) * c.D + c.N[i]) * (i128(1) << kT);
    if (v < i128(Q.lo[i]) * c.D || v >= i128(Q.hi[i]) * c.D) return false;
  }
  return true;
}

// A : HEAD. Rend 0 degenere, 1 hors boite, 2 dans la boite non interieur, 3 juge.
static inline int variant_a(const RecQ& r) {
  const P3 a{r.a[0], r.a[1], r.a[2]}, b{r.b[0], r.b[1], r.b[2]}, e{r.c[0], r.c[1], r.c[2]}, f{r.d[0], r.d[1], r.d[2]};
  const Box Q{{r.lo[0], r.lo[1], r.lo[2]}, {r.hi[0], r.hi[1], r.hi[2]}};
  geom::Center ctr;
  if (!geom::center4(a, b, e, f, ctr)) return 0;
  if (!center_in_box(a, ctr, Q)) return 1;
  const P3* t4[4] = {&a, &b, &e, &f};
  if (!geom::strictly_inside_tetra(t4, a, ctr)) return 2;
  return 3;
}

// Barycentriques du centre circonscrit par le systeme de Gram : c - a = (l1 u + l2 v + l3 s) / D, D = 2 det G > 0.
// Bornes u18 : entrees de Gram < 3 2^36 ; mineurs 2x2 < 2^76.2 ; l_i, det < 9 2^112 < 2^116 : i128.
struct Bary { i128 l1, l2, l3, D; };
static inline bool bary_exact(const RecQ& r, Bary& o) {
  const i64 ux = r.b[0] - r.a[0], uy = r.b[1] - r.a[1], uz = r.b[2] - r.a[2];
  const i64 vx = r.c[0] - r.a[0], vy = r.c[1] - r.a[1], vz = r.c[2] - r.a[2];
  const i64 sx = r.d[0] - r.a[0], sy = r.d[1] - r.a[1], sz = r.d[2] - r.a[2];
  const i128 uu = ux * ux + uy * uy + uz * uz, vv = vx * vx + vy * vy + vz * vz, ss = sx * sx + sy * sy + sz * sz;
  const i128 uv = ux * vx + uy * vy + uz * vz, us = ux * sx + uy * sy + uz * sz, vs = vx * sx + vy * sy + vz * sz;
  const i128 m_vvss = vv * ss - vs * vs, m_uvss = uv * ss - vs * us, m_uvvs = uv * vs - vv * us;
  const i128 d3 = uu * m_vvss - uv * m_uvss + us * m_uvvs;
  if (d3 == 0) return false;
  o.l1 = uu * m_vvss - uv * (vv * ss - vs * ss) + us * (vv * vs - vv * ss);
  o.l2 = uu * (vv * ss - vs * ss) - uu * m_uvss + us * (uv * ss - vv * us);
  o.l3 = uu * (vv * ss - vs * vv) - uv * (uv * ss - vv * us) + uu * m_uvvs;
  o.D = 2 * d3;
  return true;
}
static inline bool box_from_bary(const RecQ& r, const Bary& y) {
  // centre = a + (l1 u + l2 v + l3 s) / D ; |N| < 3 2^116 2^18 : depasse i128 en pire cas u18 -> garde ci-dessous
  for (int i = 0; i < 3; ++i) {
    const i64 u = r.b[i] - r.a[i], v = r.c[i] - r.a[i], s = r.d[i] - r.a[i];
    const i128 N = y.l1 * u + y.l2 * v + y.l3 * s;
    const i128 val = (i128(r.a[i]) * y.D + N) * (i128(1) << kT);
    if (val < i128(r.lo[i]) * y.D || val >= i128(r.hi[i]) * y.D) return false;
  }
  return true;
}
// B : exacte, interieur d'abord. Rend 0 degenere, 2 non interieur, 1 interieur hors boite, 3 juge.
static inline int variant_b(const RecQ& r) {
  Bary y{};
  if (!bary_exact(r, y)) return 0;
  if (!(y.l1 > 0 && y.l2 > 0 && y.l3 > 0 && y.D - y.l1 - y.l2 - y.l3 > 0)) return 2;
  return box_from_bary(r, y) ? 3 : 1;
}
// C : filtre double. Entrees de Gram exactes en double (< 2^53). Chaque l_i et det est une somme de 3 produits de
// trois entrees : erreur absolue <= 32 u M^3 (u = 2^-53, M = max des entrees) ; quatrieme barycentrique : <= 160 u M^3.
// On prend eps = 2^-44 M^3 pour toutes (marge > 3). Decision certaine si |valeur| > eps.
static unsigned long long g_fallback = 0;
static inline int variant_c(const RecQ& r) {
  const double ux = r.b[0] - r.a[0], uy = r.b[1] - r.a[1], uz = r.b[2] - r.a[2];
  const double vx = r.c[0] - r.a[0], vy = r.c[1] - r.a[1], vz = r.c[2] - r.a[2];
  const double sx = r.d[0] - r.a[0], sy = r.d[1] - r.a[1], sz = r.d[2] - r.a[2];
  const double uu = ux * ux + uy * uy + uz * uz, vv = vx * vx + vy * vy + vz * vz, ss = sx * sx + sy * sy + sz * sz;
  const double uv = ux * vx + uy * vy + uz * vz, us = ux * sx + uy * sy + uz * sz, vs = vx * sx + vy * sy + vz * sz;
  const double M = std::max(uu, std::max(vv, ss));
  const double eps = 0x1p-44 * M * M * M;
  const double m_vvss = vv * ss - vs * vs, m_uvss = uv * ss - vs * us, m_uvvs = uv * vs - vv * us;
  const double d3 = uu * m_vvss - uv * m_uvss + us * m_uvvs;
  const double l1 = uu * m_vvss - uv * (vv * ss - vs * ss) + us * (vv * vs - vv * ss);
  const double l2 = uu * (vv * ss - vs * ss) - uu * m_uvss + us * (uv * ss - vv * us);
  const double l3 = uu * (vv * ss - vs * vv) - uv * (uv * ss - vv * us) + uu * m_uvvs;
  const double l0 = 2 * d3 - l1 - l2 - l3;
  // rejet certain : une barycentrique certainement <= 0 (det > 0 pour un tetraedre non degenere)
  if (d3 > eps && (l1 < -eps || l2 < -eps || l3 < -eps || l0 < -eps)) return 2;
  if (!(d3 > eps && l1 > eps && l2 > eps && l3 > eps && l0 > eps)) {
    ++g_fallback;
    return variant_b(r);
  }
  Bary y{};
  if (!bary_exact(r, y)) return 0;
  return box_from_bary(r, y) ? 3 : 1;
}

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::fseek(f, 0, SEEK_END);
  const size_t n = size_t(std::ftell(f)) / sizeof(RecQ);
  std::fseek(f, 0, SEEK_SET);
  std::vector<RecQ> v(n);
  if (std::fread(v.data(), sizeof(RecQ), n, f) != n) return 2;
  std::fclose(f);
  const int reps = argc > 2 ? std::atoi(argv[2]) : 5;
  unsigned long long cnt[3][4] = {};
  double best[3] = {1e300, 1e300, 1e300};
  i64 maxext = 0;
  for (size_t i = 0; i < n; ++i)
    for (int k = 0; k < 3; ++k) {
      const i64 lo = std::min(std::min(v[i].a[k], v[i].b[k]), std::min(v[i].c[k], v[i].d[k]));
      const i64 hi = std::max(std::max(v[i].a[k], v[i].b[k]), std::max(v[i].c[k], v[i].d[k]));
      maxext = std::max(maxext, hi - lo);
    }
  for (int rep = 0; rep < reps; ++rep) {
    for (int var = 0; var < 3; ++var) {
      unsigned long long c[4] = {};
      g_fallback = 0;
      const unsigned long long t0 = __rdtsc();
      if (var == 0) for (size_t i = 0; i < n; ++i) ++c[variant_a(v[i])];
      else if (var == 1) for (size_t i = 0; i < n; ++i) ++c[variant_b(v[i])];
      else for (size_t i = 0; i < n; ++i) ++c[variant_c(v[i])];
      const double cyc = double(__rdtsc() - t0) / double(n);
      best[var] = std::min(best[var], cyc);
      for (int k = 0; k < 4; ++k) cnt[var][k] = c[k];
    }
  }
  std::printf("quadruplets %zu, etendue max des 4 sites %lld unites\n", n, (long long)maxext);
  const char* nm[3] = {"A HEAD (centre, boite, interieur ; i128)", "B exacte reordonnee (interieur d'abord)", "C filtre double + repli exact"};
  for (int var = 0; var < 3; ++var)
    std::printf("%-44s : %7.1f cycles/quadruplet (min de %d) | degeneres %llu, classe1 %llu, classe2 %llu, juges %llu\n", nm[var], best[var], reps,
                cnt[var][0], cnt[var][1], cnt[var][2], cnt[var][3]);
  std::printf("replis exacts de C : %llu (%.4f %%)\n", g_fallback, 100.0 * double(g_fallback) / double(n));
  // accord des decisions : les juges (classe 3) doivent etre les memes, element par element
  unsigned long long diff_ab = 0, diff_ac = 0;
  for (size_t i = 0; i < n; ++i) {
    const bool ja = variant_a(v[i]) == 3;
    diff_ab += ja != (variant_b(v[i]) == 3);
    diff_ac += ja != (variant_c(v[i]) == 3);
  }
  std::printf("desaccords sur la decision de juger : A/B %llu, A/C %llu\n", diff_ab, diff_ac);
  return (diff_ab || diff_ac) ? 1 : 0;
}
