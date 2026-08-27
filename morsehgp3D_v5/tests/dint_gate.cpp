// MorseHGP3D v5 — PORTE de l'arithmetique 128 bits portable (src/core/dint.hpp)
// et des formes device (src/lanes/device_forms.hpp), preparation du port GPU
// SANS GPU : tout est prouve egal a __int128 et aux formes de production sur
// le CPU.
//
//   (a) DI128 contre __int128 materiel sur >= 200 000 tirages deterministes
//       (mt19937_64, graine gravee) : add/sub/neg/shl1 (semantique modulaire
//       2^128 declaree, reference = u128 repliee), cmp/sign/is_zero/abs/
//       fits_i64 sur TOUTE la plage i128 ; div_by_4_exact sur des multiples
//       de 4 ; mul_i64_i64 sur toute la plage i64 (et largeurs melangees) ;
//       mul_di128_i64 sous la precondition |resultat| < 2^126 ; mulhi portable
//       (limbes 32 bits) contre mulhi u128 ; du_mul_128x128_192 contre
//       wide.hpp::mul_128x128_192 ; di_sum_of_three_squares_192 contre
//       wide.hpp::sum_of_three_squares_192. Plus les BORDS : 0, ±1,
//       INT64_MIN/MAX, ±2^63, ±2^64, ±2^126, ±(2^127-1), -2^127 (toutes les
//       paires ; mul_di128_i64 sur (bord i128, bord i64) des que le produit
//       tient dans i128 — c'est le contrat exact).
//   (b) formes device contre formes de production sur TOUS les triangles
//       (trois affectations arete/apex) et TOUS les tetraedres (deux ordres)
//       des nuages : uniform n=40 et eight_clusters n=32 sur TOUTE la grille
//       (coord 65536), et les fixtures u16 EXTREMES de tests/q3_oracle.cpp
//       (coordonnees recopiees) : equilateral a M=65535, presque-rectangle
//       aigu, sa translation au bord de la grille, grande cosphere R=20000,
//       petite cosphere, tetraedre, grille large generique ; plus les
//       tetraedres graves de q4_oracle (coquille a puissances nulles, 13
//       points). q3_form_d ≡ q3_form (G, W), q3_power_d ≡ q3_power sur tous
//       les points, q4_form_d ≡ q4_form (det, N'), q4_power_d ≡ q4_power,
//       q4_center_strictly_inside_d ≡ q4_center_strictly_inside (det > 0),
//       q4_level_d ≡ q4_level_raw (num U192, den), in_spindle_d ≡ in_spindle
//       (trois arites, toutes les paires ordonnees, tous les z).
//   (c) mutant `dint-mulhi-dropped` (src/core/dint.hpp : mot haut du produit
//       64×64 ignore sur host) : tue par (a) et (b), code 4.
//
// Codes : 0 accord total et planchers atteints ; 2 refus (argument ou mutant
// inconnu, n invalide) ; 3 desaccord (une FIXTURE A GRAVER, jamais a cacher),
// plancher ou invariant viole, mutant non discrimine ; 4 mutant tue.
// Planchers : --min-cases (tirages de (a)), --min-triangles (formes q3),
// --min-tetra (tetraedres), --min-inside (bien centres), --min-spindle.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/core/dint.hpp"
#include "../src/core/mutants.hpp"
#include "../src/lanes/device_forms.hpp"
#include "../src/lanes/q4.hpp"
#include "../src/spindle/spindle.hpp"

using namespace mhgp5;

namespace {

int g_disagree = 0;
int g_invariant = 0;

void hex128(char* out, i128 v) {
  const u128 m = uabs128(v);
  std::snprintf(out, 40, "%s0x%016llx%016llx", v < 0 ? "-" : "", (unsigned long long)(m >> 64),
                (unsigned long long)(u64)m);
}

void disagree(const char* what, i128 a = 0, i128 b = 0) {
  if (g_disagree < 40) {
    char ha[40], hb[40];
    hex128(ha, a);
    hex128(hb, b);
    std::fprintf(stderr, "DESACCORD dint : %s a=%s b=%s\n", what, ha, hb);
  }
  ++g_disagree;
}

void disagree_geom(const char* what, const P3* p, int k) {
  if (g_disagree < 40) {
    std::fprintf(stderr, "DESACCORD forme device : %s sur", what);
    for (int i = 0; i < k; ++i)
      std::fprintf(stderr, " (%lld,%lld,%lld)", (long long)p[i].x, (long long)p[i].y, (long long)p[i].z);
    std::fprintf(stderr, "\n");
  }
  ++g_disagree;
}

void invariant(const char* what) {
  if (g_invariant < 20) std::fprintf(stderr, "INVARIANT dint : %s\n", what);
  ++g_invariant;
}

// ---- Tirages deterministes -------------------------------------------------------
struct Rng {
  std::mt19937_64 g;
  explicit Rng(u64 seed) : g(seed) {}
  u64 u() { return g(); }
  u128 u128_full() { return ((u128)u() << 64) | u(); }
  i128 i128_full() { return (i128)u128_full(); }
  // Magnitude de `bits` bits exactement (1 <= bits <= 127), signe aleatoire.
  i128 i128_bits(int bits) {
    u128 m = u128_full();
    if (bits < 128) m &= (((u128)1) << bits) - 1;
    m |= ((u128)1) << (bits - 1);
    const i128 v = (i128)m;
    return (u() & 1) ? -v : v;
  }
  i64 i64_bits(int bits) {  // 1 <= bits <= 63
    u64 m = u() & ((1ull << bits) - 1);
    m |= 1ull << (bits - 1);
    const i64 v = (i64)m;
    return (u() & 1) ? -v : v;
  }
  u128 u128_bits(int bits) {  // 1 <= bits <= 128
    u128 m = u128_full();
    if (bits < 128) m &= (((u128)1) << bits) - 1;
    m |= ((u128)1) << (bits - 1);
    return m;
  }
};

i128 wrap_add(i128 a, i128 b) { return (i128)((u128)a + (u128)b); }
i128 wrap_sub(i128 a, i128 b) { return (i128)((u128)a - (u128)b); }
i128 wrap_neg(i128 a) { return (i128)(0 - (u128)a); }
i128 wrap_shl1(i128 a) { return (i128)((u128)a << 1); }
int sign_of(i128 v) { return v < 0 ? -1 : (v > 0 ? 1 : 0); }
int cmp_of(i128 a, i128 b) { return a < b ? -1 : (a > b ? 1 : 0); }

bool same_u192(const U192& a, const U192& b) { return a.w[0] == b.w[0] && a.w[1] == b.w[1] && a.w[2] == b.w[2]; }

// ---- (a) unaires / binaires sur une paire i128 ------------------------------------
void check_pair(i128 a, i128 b) {
  const DI128 da = di_from_i128(a), db = di_from_i128(b);
  if (di_to_i128(da) != a) disagree("aller-retour from_i128/to_i128", a);
  if (di_to_i128(di_add(da, db)) != wrap_add(a, b)) disagree("add", a, b);
  if (di_to_i128(di_sub(da, db)) != wrap_sub(a, b)) disagree("sub", a, b);
  if (di_to_i128(di_neg(da)) != wrap_neg(a)) disagree("neg", a);
  if (di_cmp(da, db) != cmp_of(a, b) || di_cmp(db, da) != cmp_of(b, a)) disagree("cmp", a, b);
  if (di_sign(da) != sign_of(a)) disagree("sign", a);
  if (di_is_zero(da) != (a == 0)) disagree("is_zero", a);
  if (di_is_neg(da) != (a < 0)) disagree("is_neg", a);
  if (di_eq(da, db) != (a == b)) disagree("eq", a, b);
  if (di_to_i128(di_shl1(da)) != wrap_shl1(a)) disagree("shl1", a);
  const i128 a4 = (i128)((u128)a & ~(u128)3);
  if (di_to_i128(di_div_by_4_exact(di_from_i128(a4))) != a4 / 4) disagree("div_by_4_exact", a4);
  if (du_to_u128(di_abs(da)) != uabs128(a)) disagree("abs", a);
  const bool fits = a >= (i128)INT64_MIN && a <= (i128)INT64_MAX;
  if (di_fits_i64(da) != fits) disagree("fits_i64", a);
  if (fits && di_to_i64_unchecked(da) != (i64)a) disagree("to_i64_unchecked", a);
}

void check_mul_i64(i64 x, i64 y) {
  const i128 want = (i128)x * y;
  if (di_to_i128(di_mul_i64_i64(x, y)) != want) disagree("mul_i64_i64", x, y);
  if (di_to_i128(di_mul_i64_i64(y, x)) != want) disagree("mul_i64_i64 (commute)", y, x);
}

// Rend false si le produit ne tient pas dans i128 (cas ecarte, pas un desaccord).
bool check_mul_di128_i64(i128 a, i64 v) {
  i128 want = 0;
  if (__builtin_mul_overflow(a, (i128)v, &want)) return false;
  if (di_to_i128(di_mul_di128_i64(di_from_i128(a), v)) != want) disagree("mul_di128_i64", a, v);
  return true;
}

// ---- Fixtures gravees (coordonnees exactes recopiees de tests/q3_oracle.cpp
// et tests/q4_oracle.cpp) -------------------------------------------------------------
std::vector<P3> cosphere_cloud(i64 cx, const i64 pat[3], const P3& inner) {
  const int perm[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
  std::vector<P3> pts;
  std::set<long long> seen;
  for (const auto& pr : perm)
    for (int sx = -1; sx <= 1; sx += 2)
      for (int sy = -1; sy <= 1; sy += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
          const i64 x = cx + sx * pat[pr[0]], y = cx + sy * pat[pr[1]], z = cx + sz * pat[pr[2]];
          if (x < 0 || y < 0 || z < 0) continue;
          const long long key = (x << 34) | (y << 17) | z;
          if (!seen.insert(key).second) continue;
          pts.push_back(P3{x, y, z});
        }
  pts.push_back(P3{cx, cx, cx});
  pts.push_back(inner);
  return pts;
}

std::vector<P3> tetra_cloud() { return {{0, 0, 0}, {2, 2, 0}, {2, 0, 2}, {0, 2, 2}, {9, 9, 9}, {1, 9, 3}}; }

std::vector<P3> equilateral_max_cloud() {
  const i64 M = 65535;
  return {{0, 0, 0}, {M, M, 0}, {M, 0, M}, {0, M, M}, {M, M, M}, {30000, 20000, 10000}};
}

std::vector<P3> near_right_cloud(const P3& t) {
  return {{t.x + 0, t.y + 0, t.z + 0},
          {t.x + 40000, t.y + 0, t.z + 0},
          {t.x + 20000, t.y + 20001, t.z + 0},
          {t.x + 20000, t.y + 10000, t.z + 1000},
          {t.x + 1000, t.y + 19000, t.z + 2000}};
}

std::vector<P3> wide_generic_cloud() {
  return {{12345, 54321, 6789}, {65535, 1, 32768},    {777, 65000, 4242},  {40001, 20002, 60003}, {31337, 3, 65534},
          {2, 44444, 22222},    {55555, 55555, 5},     {9999, 9, 49999},    {60000, 30001, 15000}, {23456, 65535, 34567}};
}

// Tetraedres graves de q4_oracle : coquille a puissances equatoriales NULLES
// (ab diametre : frontiere du bien-centrage) et tetraedre de la fixture 13 points.
std::vector<P3> q4_engraved_cloud() {
  return {{0, 0, 0}, {4, 0, 0}, {2, 2, 0}, {2, 0, 2}, {100, 300, 300}, {300, 300, 300}, {200, 160, 400}, {200, 160, 200}};
}

struct Args {
  bool ok = true;
  u64 cases = 200000;
  u64 min_cases = 200000, min_triangles = 1, min_tetra = 1, min_inside = 1, min_spindle = 1;
  int n_uniform = 40, n_clusters = 32, coord = 65536;
  long long seed = 3;
  std::string inject;
};

Args parse(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto val = [&](const char* prefix) -> const char* {
      const size_t l = std::strlen(prefix);
      return arg.compare(0, l, prefix) == 0 ? arg.c_str() + l : nullptr;
    };
    if (const char* v = val("--cases=")) a.cases = (u64)std::atoll(v);
    else if (const char* v = val("--min-cases=")) a.min_cases = (u64)std::atoll(v);
    else if (const char* v = val("--min-triangles=")) a.min_triangles = (u64)std::atoll(v);
    else if (const char* v = val("--min-tetra=")) a.min_tetra = (u64)std::atoll(v);
    else if (const char* v = val("--min-inside=")) a.min_inside = (u64)std::atoll(v);
    else if (const char* v = val("--min-spindle=")) a.min_spindle = (u64)std::atoll(v);
    else if (const char* v = val("--n-uniform=")) a.n_uniform = std::atoi(v);
    else if (const char* v = val("--n-clusters=")) a.n_clusters = std::atoi(v);
    else if (const char* v = val("--coord=")) a.coord = std::atoi(v);
    else if (const char* v = val("--seed=")) a.seed = std::atoll(v);
    else if (const char* v = val("--inject=")) a.inject = v;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      a.ok = false;
    }
  }
  return a;
}

struct Counters {
  u64 cases = 0, border_cases = 0, mul_cross64 = 0, mul_di_cases = 0;
  u64 triangles = 0, q3_points = 0, tetra = 0, coplanar = 0, q4_points = 0, inside_true = 0, inside_false = 0;
  u64 spindle = 0, spindle_true[3] = {0, 0, 0};
};

// ---- (b) un nuage entier ------------------------------------------------------------
void judge_cloud(const std::vector<P3>& pts, Counters* ct) {
  const size_t n = pts.size();
  // q3 : tous les triangles, trois affectations (arete, apex).
  for (size_t p = 0; p < n; ++p)
    for (size_t q = p + 1; q < n; ++q)
      for (size_t r = q + 1; r < n; ++r) {
        const size_t e0[3] = {p, p, q}, e1[3] = {q, r, r}, ap[3] = {r, q, p};
        for (int t = 0; t < 3; ++t) {
          const P3 tri[3] = {pts[e0[t]], pts[e1[t]], pts[ap[t]]};
          const Q3Form f = q3_form(tri[0], tri[1], tri[2]);
          const Q3FormD fd = q3_form_d(tri[0], tri[1], tri[2]);
          ++ct->triangles;
          if (di_to_i128(fd.g) != f.g) {
            disagree_geom("q3_form G", tri, 3);
            continue;
          }
          bool wok = fd.a == f.a;
          for (int i = 0; i < 3; ++i) wok = wok && di_to_i128(fd.w[i]) == f.w[i];
          if (!wok) {
            disagree_geom("q3_form W", tri, 3);
            continue;
          }
          bool pok = true;
          for (size_t z = 0; z < n; ++z) {
            const i128 pw = q3_power(f, pts[z]);
            const DI128 pd = q3_power_d(fd, pts[z]);
            if (di_to_i128(pd) != pw || di_sign(pd) != sign_of(pw)) pok = false;
            ++ct->q3_points;
          }
          if (!pok) disagree_geom("q3_power", tri, 3);
        }
      }
  // q4 : tous les tetraedres, deux ordres (le second inverse l'orientation).
  for (size_t p = 0; p < n; ++p)
    for (size_t q = p + 1; q < n; ++q)
      for (size_t r = q + 1; r < n; ++r)
        for (size_t s = r + 1; s < n; ++s) {
          ++ct->tetra;
          for (int o = 0; o < 2; ++o) {
            const P3 tet[4] = {pts[p], o == 0 ? pts[q] : pts[r], o == 0 ? pts[r] : pts[q], pts[s]};
            const Q4Form f = q4_form(tet[0], tet[1], tet[2], tet[3]);
            const Q4FormD fd = q4_form_d(tet[0], tet[1], tet[2], tet[3]);
            if (di_to_i128(fd.det) != f.det) {
              disagree_geom("q4_form det", tet, 4);
              continue;
            }
            bool nok = fd.a == f.a;
            for (int i = 0; i < 3; ++i) nok = nok && di_to_i128(fd.np[i]) == f.np[i];
            if (!nok) {
              disagree_geom("q4_form N'", tet, 4);
              continue;
            }
            bool pok = true;
            for (size_t z = 0; z < n; ++z) {
              const i128 pw = q4_power(f, pts[z]);
              const DI128 pd = q4_power_d(fd, pts[z]);
              if (di_to_i128(pd) != pw || di_sign(pd) != sign_of(pw)) pok = false;
              ++ct->q4_points;
            }
            if (!pok) disagree_geom("q4_power", tet, 4);
            if (f.det == 0) {
              if (o == 0) ++ct->coplanar;
              continue;
            }
            const bool in_s = q4_center_strictly_inside(f, tet[0], tet[1], tet[2], tet[3]);
            const bool in_d = q4_center_strictly_inside_d(fd, tet[0], tet[1], tet[2], tet[3]);
            if (in_s != in_d) disagree_geom("q4_center_strictly_inside", tet, 4);
            if (o == 0) {
              if (in_s) ++ct->inside_true;
              else ++ct->inside_false;
            }
            const ExactLevel lv = q4_level_raw(f);
            U192 num{{0, 0, 0}};
            DI128 den = di_zero();
            if (!q4_level_d(fd, &num, &den)) {
              disagree_geom("q4_level_d : det hors i64 (precondition u16 violee)", tet, 4);
              continue;
            }
            const U192 ns{{lv.num[0], lv.num[1], lv.num[2]}};
            if (!same_u192(num, ns) || di_to_i128(den) != lv.den) disagree_geom("q4_level (num U192 / den det²)", tet, 4);
          }
        }
  // fuseaux : toutes les paires ordonnees (a,b), tous les z, trois arites.
  for (size_t ia = 0; ia < n; ++ia)
    for (size_t ib = 0; ib < n; ++ib) {
      if (ia == ib) continue;
      for (size_t iz = 0; iz < n; ++iz)
        for (const Lane q : kLanes) {
          const bool ws = in_spindle(q, pts[ia], pts[ib], pts[iz]);
          const bool wd = in_spindle_d(lane_arity(q), pts[ia], pts[ib], pts[iz]);
          ++ct->spindle;
          if (ws != wd) {
            const P3 trip[3] = {pts[ia], pts[ib], pts[iz]};
            disagree_geom("in_spindle", trip, 3);
          } else if (ws) {
            ++ct->spindle_true[lane_index(q)];
          }
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
  const Args a = parse(argc, argv);
  if (!a.ok || a.cases < 1 || a.n_uniform < 4 || a.n_clusters < 4 || a.coord < 4 || a.coord > 65536) {
    std::fprintf(stderr, "REFUS : arguments invalides\n");
    return 2;
  }
  if (!a.inject.empty() && !mutants_enable(a.inject)) {
    std::fprintf(stderr, "REFUS : mutant inconnu %s\n", a.inject.c_str());
    return 2;
  }
  const bool mutant = !a.inject.empty();
  Counters ct;

  // ---- (a) tirages ----------------------------------------------------------------
  {
    Rng rng(0xd1a7c0de5eed0001ull);  // graine GRAVEE : aucune horloge
    for (u64 c = 0; c < a.cases; ++c) {
      // Toute la plage i128 (un tirage sur deux a largeur controlee).
      const bool wide = (c & 1) == 0;
      const i128 x = wide ? rng.i128_full() : rng.i128_bits(1 + (int)(rng.u() % 127));
      const i128 y = wide ? rng.i128_full() : rng.i128_bits(1 + (int)(rng.u() % 127));
      check_pair(x, y);
      // i64 × i64 : toute la plage puis largeurs melangees.
      const i64 p = (i64)rng.u(), q = (i64)rng.u();
      check_mul_i64(p, q);
      if (uabs128((i128)p * q) >= (((u128)1) << 64)) ++ct.mul_cross64;
      const i64 p2 = rng.i64_bits(1 + (int)(rng.u() % 63)), q2 = rng.i64_bits(1 + (int)(rng.u() % 63));
      check_mul_i64(p2, q2);
      if (uabs128((i128)p2 * q2) >= (((u128)1) << 64)) ++ct.mul_cross64;
      // DI128 × i64 sous la precondition : |a| < 2^ab, |v| < 2^vb, ab + vb <= 125.
      const int vb = 1 + (int)(rng.u() % 63);
      const int ab = 1 + (int)(rng.u() % (u64)(125 - vb));
      const i128 av = rng.i128_bits(ab);
      const i64 vv = rng.i64_bits(vb);
      if (uabs128(av) * uabs128(vv) >= (((u128)1) << 126)) invariant("generateur : produit >= 2^126");
      if (!check_mul_di128_i64(av, vv)) invariant("generateur : produit hors i128");
      ++ct.mul_di_cases;
      // mulhi portable (limbes 32) contre mulhi u128.
      const u64 mx = rng.u(), my = rng.u();
      const u64 want_hi = (u64)(((u128)mx * my) >> 64);
      if (di_mulhi_u64_portable(mx, my) != want_hi) disagree("mulhi portable", (i128)mx, (i128)my);
      if (di_mulhi_u64(mx, my) != want_hi) disagree("mulhi (selection host)", (i128)mx, (i128)my);
      // 128×128 -> 192 en limbes contre wide.hpp (produit < 2^192).
      const int xb = 1 + (int)(rng.u() % 128);
      const int yb_max = 192 - xb > 128 ? 128 : 192 - xb;
      const int yb = 1 + (int)(rng.u() % (u64)yb_max);
      const u128 ux = rng.u128_bits(xb), uy = rng.u128_bits(yb);
      if (!same_u192(du_mul_128x128_192(du_from_u128(ux), du_from_u128(uy)), mul_128x128_192(ux, uy)))
        disagree("du_mul_128x128_192", (i128)ux, (i128)uy);
      // |a|² + |b|² + |c|² (|.| < 2^95) contre wide.hpp.
      const i128 s0 = rng.i128_bits(1 + (int)(rng.u() % 95)), s1 = rng.i128_bits(1 + (int)(rng.u() % 95)),
                 s2 = rng.i128_bits(1 + (int)(rng.u() % 95));
      if (!same_u192(di_sum_of_three_squares_192(di_from_i128(s0), di_from_i128(s1), di_from_i128(s2)),
                     sum_of_three_squares_192(uabs128(s0), uabs128(s1), uabs128(s2))))
        disagree("di_sum_of_three_squares_192", s0, s1);
      ++ct.cases;
    }
  }

  // ---- (a') bords -------------------------------------------------------------------
  {
    const i128 imax = (i128)((((u128)1) << 127) - 1);
    const i128 imin = -imax - 1;
    const i128 b126 = (i128)1 << 126;
    const i128 b64 = (i128)1 << 64;
    const i128 b63 = (i128)1 << 63;
    const i128 borders[] = {0,    1,     -1,    (i128)INT64_MIN, (i128)INT64_MAX, b63,  -b63, b64, -b64,
                            b126, -b126, imax, -imax,           imin,            imin + 1};
    for (const i128 x : borders)
      for (const i128 y : borders) {
        check_pair(x, y);
        ++ct.border_cases;
      }
    const i64 b64s[] = {0, 1, -1, 2, -2, INT64_MIN, INT64_MAX, INT64_MIN + 1, -INT64_MAX, 1ll << 32, -(1ll << 32),
                        (1ll << 32) - 1, (1ll << 62), -(1ll << 62)};
    for (const i64 x : b64s)
      for (const i64 y : b64s) {
        check_mul_i64(x, y);
        ++ct.border_cases;
      }
    for (const i128 x : borders)
      for (const i64 v : b64s)
        if (check_mul_di128_i64(x, v)) ++ct.border_cases;  // seuls les produits qui tiennent (contrat exact)
    // Bords explicites du contrat de mul_di128_i64 : ±2^126 × ±1, 2^63 × 2^63,
    // (2^127-1) × ±1, -2^127 × 1, (2^63-1) × 2^63 (< 2^127).
    if (di_to_i128(di_mul_di128_i64(di_from_i128(b126), -1)) != -b126) disagree("2^126 * -1");
    if (di_to_i128(di_mul_di128_i64(di_from_i128(-b126), -1)) != b126) disagree("-2^126 * -1");
    if (di_to_i128(di_mul_di128_i64(di_from_i128(b63), INT64_MIN)) != -b126) disagree("2^63 * -2^63");
    if (di_to_i128(di_mul_di128_i64(di_from_i128(-b63), INT64_MIN)) != b126) disagree("-2^63 * -2^63");
    if (di_to_i128(di_mul_di128_i64(di_from_i128(imax), -1)) != -imax) disagree("(2^127-1) * -1");
    if (di_to_i128(di_mul_di128_i64(di_from_i128(imin), 1)) != imin) disagree("-2^127 * 1");
    if (di_to_i128(di_mul_di128_i64(di_from_i128(imin + 1), -1)) != imax) disagree("(-2^127+1) * -1");
    if (di_to_i128(di_mul_di128_i64(di_from_i128(b64 - 1), INT64_MAX)) != (b64 - 1) * INT64_MAX)
      disagree("(2^64-1) * (2^63-1)");
    ct.border_cases += 8;
  }

  // ---- (b) formes ---------------------------------------------------------------------
  {
    const i64 small_pat[3] = {3, 4, 0};
    const i64 big_pat[3] = {12000, 16000, 0};
    struct Cloud {
      const char* name;
      std::vector<P3> pts;
    };
    const std::vector<Cloud> clouds = {
        {"uniform", make_family_cloud(CloudFamily::kUniform, a.n_uniform, a.coord, a.seed)},
        {"eight_clusters", make_family_cloud(CloudFamily::kEightClusters, a.n_clusters, a.coord, a.seed)},
        {"equilateral_max", equilateral_max_cloud()},
        {"near_right", near_right_cloud(P3{0, 0, 0})},
        {"near_right_bord", near_right_cloud(P3{25535, 45534, 63535})},
        {"grande_cosphere", cosphere_cloud(32768, big_pat, P3{30000, 30000, 30000})},
        {"petite_cosphere", cosphere_cloud(4, small_pat, P3{1, 1, 1})},
        {"tetraedre", tetra_cloud()},
        {"grille_large", wide_generic_cloud()},
        {"q4_graves", q4_engraved_cloud()},
    };
    for (const Cloud& c : clouds) {
      if (c.pts.size() < 4) {
        std::fprintf(stderr, "REFUS : nuage %s de moins de 4 points\n", c.name);
        return 2;
      }
      for (const P3& p : c.pts)
        if (!p3_in_profile(p)) {
          std::fprintf(stderr, "REFUS : nuage %s hors profil u16\n", c.name);
          return 2;
        }
      judge_cloud(c.pts, &ct);
    }
  }

  std::printf(
      "dint_gate : cas=%llu bords=%llu mul_croisant_64=%llu mul_di128_i64=%llu | formes q3=%llu (points %llu) "
      "tetraedres=%llu (coplanaires %llu, points %llu, bien_centres %llu, non %llu) fuseaux=%llu (W2 %llu, W3 %llu, "
      "W4 %llu) desaccords=%d invariants=%d\n",
      (unsigned long long)ct.cases, (unsigned long long)ct.border_cases, (unsigned long long)ct.mul_cross64,
      (unsigned long long)ct.mul_di_cases, (unsigned long long)ct.triangles, (unsigned long long)ct.q3_points,
      (unsigned long long)ct.tetra, (unsigned long long)ct.coplanar, (unsigned long long)ct.q4_points,
      (unsigned long long)ct.inside_true, (unsigned long long)ct.inside_false, (unsigned long long)ct.spindle,
      (unsigned long long)ct.spindle_true[0], (unsigned long long)ct.spindle_true[1],
      (unsigned long long)ct.spindle_true[2], g_disagree, g_invariant);

  if (mutant) {
    if (g_disagree > 0) {
      std::printf("MUTANT TUE : %s\n", a.inject.c_str());
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s non discrimine\n", a.inject.c_str());
    return 3;
  }
  if (g_disagree > 0) {
    std::fprintf(stderr, "DESACCORD : %d cas — chaque cas est une fixture a graver, jamais a cacher\n", g_disagree);
    return 3;
  }
  if (g_invariant > 0) return 3;
  if (ct.cases < a.min_cases || ct.triangles < a.min_triangles || ct.tetra < a.min_tetra ||
      ct.inside_true < a.min_inside || ct.spindle < a.min_spindle) {
    std::fprintf(stderr,
                 "PLANCHER : cas=%llu (>= %llu), formes_q3=%llu (>= %llu), tetraedres=%llu (>= %llu), "
                 "bien_centres=%llu (>= %llu), fuseaux=%llu (>= %llu)\n",
                 (unsigned long long)ct.cases, (unsigned long long)a.min_cases, (unsigned long long)ct.triangles,
                 (unsigned long long)a.min_triangles, (unsigned long long)ct.tetra, (unsigned long long)a.min_tetra,
                 (unsigned long long)ct.inside_true, (unsigned long long)a.min_inside,
                 (unsigned long long)ct.spindle, (unsigned long long)a.min_spindle);
    return 3;
  }
  std::printf("dint_gate OK\n");
  return 0;
}
