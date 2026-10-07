// Controle autonome de la fonction l09_rejects (certificat de moments) : cas attendu positif et controle de surete
// par echantillonnage (coins + points entiers de la boite fermee).
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>
#include <array>
using i64 = std::int64_t;
__extension__ typedef __int128 i128;
struct L09Group { i128 W = 0, M[3] = {0, 0, 0}, V = 0; };
inline bool l09_rejects(const L09Group& g, const i64 a[3], const i64 h[3], i64 theta) {
  if (theta < 0) return false;
  i128 a2 = 0, sig = g.V, r2 = 0;
  for (int i = 0; i < 3; ++i) {
    a2 += i128(a[i]) * a[i];
    const i128 coef = -2 * (g.M[i] - g.W * a[i]);
    if (coef > 0) sig += coef * h[i];
    const i128 d0 = i128(a[i]) * a[i], d1 = i128(a[i] - h[i]) * (a[i] - h[i]);
    r2 += d0 > d1 ? d0 : d1;
  }
  sig -= g.W * a2;
  if (r2 == 0) return false;
  return sig + i128(theta) * r2 < 0;
}
int main() {
  std::mt19937_64 rng(20261002);
  // 1. cas attendu : 10 sites serres autour du centre d'une petite boite, ancre lointaine -> rejet jusqu'a theta = 8
  {
    const i64 h[3] = {8, 8, 8};
    L09Group g;
    for (int i = 0; i < 10; ++i) {
      const i64 z[3] = {4 + (i % 3) - 1, 4 + ((i / 3) % 3) - 1, 4};
      g.W += 1;
      for (int k = 0; k < 3; ++k) { g.M[k] += z[k]; g.V += i128(z[k]) * z[k]; }
    }
    const i64 a[3] = {1000, 2000, -500};
    int best = -1;
    for (int th = 0; th < 12; ++th) if (l09_rejects(g, a, h, th)) best = th;
    std::printf("cas serre : plus grand theta rejete = %d (attendu 8 ou 9 ; 9 exige p > 9 = 10 sites)\n", best);
  }
  // 2. surete : configurations aleatoires ; si rejet(theta), alors pour tout c echantillonne p_Z(c) > theta
  long long cases = 0, fired = 0, bad = 0;
  for (int it = 0; it < 400000; ++it) {
    const int m = 3 + int(rng() % 14);
    i64 h[3];
    for (int k = 0; k < 3; ++k) h[k] = 1 + i64(rng() % 64);
    std::vector<std::array<i64, 3>> z(m);
    const i64 spread = 20 + i64(rng() % 400);
    L09Group g;
    for (int i = 0; i < m; ++i) {
      for (int k = 0; k < 3; ++k) z[i][k] = i64(rng() % (2 * spread)) - spread + h[k] / 2;
      g.W += 1;
      for (int k = 0; k < 3; ++k) { g.M[k] += z[i][k]; g.V += i128(z[i][k]) * z[i][k]; }
    }
    i64 a[3];
    const i64 far = spread * (1 + i64(rng() % 6));
    for (int k = 0; k < 3; ++k) a[k] = i64(rng() % (2 * far + 1)) - far + h[k] / 2;
    for (int th = 0; th < m; ++th) {
      ++cases;
      if (!l09_rejects(g, a, h, th)) continue;
      ++fired;
      // echantillons : 8 coins + 40 points entiers
      for (int s = 0; s < 48; ++s) {
        i64 c[3];
        for (int k = 0; k < 3; ++k) c[k] = s < 8 ? ((s >> k) & 1 ? h[k] : 0) : i64(rng() % (h[k] + 1));
        i128 da = 0;
        for (int k = 0; k < 3; ++k) da += i128(a[k] - c[k]) * (a[k] - c[k]);
        int p = 0;
        for (int i = 0; i < m; ++i) {
          i128 dz = 0;
          for (int k = 0; k < 3; ++k) dz += i128(z[i][k] - c[k]) * (z[i][k] - c[k]);
          p += dz < da;
        }
        if (p <= th) ++bad;
      }
    }
  }
  std::printf("surete : cas=%lld rejets=%lld violations=%lld\n", cases, fired, bad);
  return bad ? 1 : 0;
}
