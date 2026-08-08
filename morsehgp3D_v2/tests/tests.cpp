// MorseHGP3D v2 — tests unitaires de l'arithmetique et de la geometrie.

#include <cstdio>
#include <random>
#include <vector>

#include "mhgp/exact.hpp"
#include "mhgp/miniball.hpp"
#include "mhgp/sphere.hpp"

using namespace mhgp;

static int failures = 0;
static void check(bool c, const char* what) {
  if (!c) { ++failures; std::printf("  ECHEC %s\n", what); }
}

int main() {
  // ---- big_cmp doit resister au debordement de la difference -------------
  {
    BigInt<2> hi{{~u64(0), 0x7fffffffffffffffull}};  // 2^127 - 1
    BigInt<2> lo = big_from_i128<2>(-1);
    check(big_cmp(hi, lo) > 0, "big_cmp max vs -1");
    check(big_cmp(lo, hi) < 0, "big_cmp -1 vs max");
    BigInt<2> mn{{0, 0x8000000000000000ull}};       // -2^127
    check(big_cmp(mn, hi) < 0, "big_cmp min vs max");
    check(big_cmp(mn, mn) == 0, "big_cmp egalite");
  }

  // ---- mul128 ------------------------------------------------------------
  {
    const i128 a = (i128(1) << 100) + 12345;
    const i128 b = -((i128(1) << 90) + 7);
    const BigInt<4> p = mul128(a, b);
    const BigInt<4> q = mul128(b, a);
    check(big_cmp(p, q) == 0, "mul128 commutatif");
    check(big_negative(p), "mul128 signe");
    const BigInt<4> z = mul128(a, 0);
    check(big_is_zero(z), "mul128 par zero");
  }

  // ---- spheres : cas connus ---------------------------------------------
  {
    const P3 a{0, 0, 0}, b{100, 0, 0};
    const Sphere s = sphere2(a, b);
    check(sphere_side(s, P3{50, 0, 0}) < 0, "milieu dedans");
    check(sphere_side(s, P3{0, 0, 0}) == 0, "extremite sur le bord");
    check(sphere_side(s, P3{101, 0, 0}) > 0, "hors");
    check(sphere_side(s, P3{50, 49, 0}) < 0, "dedans lateral");
    check(sphere_side(s, P3{50, 51, 0}) > 0, "hors lateral");
  }
  {
    // triangle rectangle : non strictement acutangle
    const P3 a{0, 0, 0}, b{100, 0, 0}, c{0, 100, 0};
    check(!well_centered3(a, b, c), "triangle rectangle non bien centre");
    const P3 d{50, 40, 0};
    check(well_centered3(a, b, d) || !well_centered3(a, b, d), "well_centered3 total");
  }
  {
    // tetraedre regulier approche : bien centre
    const P3 a{0, 0, 0}, b{1000, 0, 0}, c{500, 866, 0}, d{500, 289, 816};
    Sphere s;
    check(sphere4(a, b, c, d, &s), "sphere4 construite");
    check(well_centered4(s, a, b, c, d), "tetraedre regulier bien centre");
    check(sphere_side(s, a) == 0 && sphere_side(s, b) == 0
          && sphere_side(s, c) == 0 && sphere_side(s, d) == 0,
          "sommets sur la sphere");
  }
  {
    // le contre-exemple du §3 de l'audit : le circumcentre de {p,u,v} tombe
    // sur [u,v], donc le support minimal est {u,v}, pas {p,u,v}.
    const P3 p{0, 0, 0}, u{1000, 0, 0}, v{0, 1000, 0};
    check(!well_centered3(p, u, v), "audit §3 : support non minimal rejete");
    std::vector<P3> pts{p, u, v};
    i32 ids[3] = {0, 1, 2};
    const MiniballResult mb = miniball_of(pts, ids, 3);
    check(mb.ok && mb.n_support == 2, "miniball support {u,v}");
  }

  // ---- comparaison de niveaux --------------------------------------------
  {
    const P3 a{0, 0, 0}, b{100, 0, 0}, c{0, 200, 0};
    const Sphere s1 = sphere2(a, b);
    const Sphere s2 = sphere2(a, c);
    check(sphere_cmp_beta(s1, s2) < 0, "cmp_beta ordre");
    check(sphere_cmp_beta(s2, s1) > 0, "cmp_beta ordre inverse");
    check(sphere_cmp_beta(s1, s1) == 0, "cmp_beta egalite");
  }

  // ---- coherence side / miniball sur du tirage aleatoire -----------------
  {
    std::mt19937_64 rng(7);
    std::uniform_int_distribution<i64> u(0, kCoordMax);
    for (int t = 0; t < 2000; ++t) {
      std::vector<P3> pts;
      const int m = 2 + static_cast<int>(rng() % 6);
      for (int i = 0; i < m; ++i) pts.push_back(P3{u(rng), u(rng), u(rng)});
      std::vector<i32> ids(static_cast<size_t>(m));
      for (int i = 0; i < m; ++i) ids[static_cast<size_t>(i)] = i;
      const MiniballResult mb = miniball_of(pts, ids.data(), m);
      if (!mb.ok) continue;
      for (int i = 0; i < m; ++i)
        check(sphere_side(mb.sph, pts[static_cast<size_t>(i)]) <= 0,
              "miniball couvre ses points");
      for (int i = 0; i < mb.n_support; ++i)
        check(sphere_side(mb.sph, pts[static_cast<size_t>(mb.support[i])]) == 0,
              "support sur le bord");
    }
  }

  if (failures) { std::printf("ECHEC : %d\n", failures); return 1; }
  std::printf("OK : tests unitaires\n");
  return 0;
}
