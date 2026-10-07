// Microbanc d'opportunite (audit L05) : etage des triplets de la feuille v10 sur les candidats REELS (trame 02).
//   A : sequence du HEAD : droite des centres contre la boite (lemme Z, i64 + i128), puis aigu, centre q3 (i128),
//       centre dans la boite (i128) ;
//   D : deux phases par blocs SoA : phase 1 en double sans branchement (droite avec borne d'erreur, aigu exact en
//       double, centre dans la boite avec borne d'erreur), phase 2 : repli exact (A) pour les indecis seulement.
// Decisions comparees element par element : (touche, juge). Ce n'est pas du code produit.
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
struct RecT { int a[3], b[3], c[3], lo[3], hi[3]; };

static inline bool center_in_box(const P3& a, const geom::Center& c, const Box& Q) {  // generator.cpp:96
  const i64 av[3] = {a.x, a.y, a.z};
  for (int i = 0; i < 3; ++i) {
    const i128 v = (i128(av[i]) * c.D + c.N[i]) * (i128(1) << kT);
    if (v < i128(Q.lo[i]) * c.D || v >= i128(Q.hi[i]) * c.D) return false;
  }
  return true;
}
static inline int center_line_meets(const P3& A, i64 a2, const P3& B, i64 b2, const P3& Dp, i64 d2, const Box& Q) {  // generator.cpp:122
  const i64 u[3] = {A.x - B.x, A.y - B.y, A.z - B.z};
  const i64 v[3] = {A.x - Dp.x, A.y - Dp.y, A.z - Dp.z};
  const i64 c01 = u[0] * v[1] - v[0] * u[1], c02 = u[0] * v[2] - v[0] * u[2], c12 = u[1] * v[2] - v[1] * u[2];
  if (c01 == 0 && c02 == 0 && c12 == 0) return -1;
  i64 p0 = 2 * (a2 - b2), p1 = 2 * (a2 - d2), h[3];
  for (int k = 0; k < 3; ++k) {
    const i64 s = Q.lo[k] + Q.hi[k];
    p0 -= 2 * s * u[k];
    p1 -= 2 * s * v[k];
    h[k] = Q.hi[k] - Q.lo[k];
  }
  const i64 a01 = c01 < 0 ? -c01 : c01, a02 = c02 < 0 ? -c02 : c02, a12 = c12 < 0 ? -c12 : c12;
  const i128 r[3] = {2 * (i128(h[1]) * a01 + i128(h[2]) * a02), 2 * (i128(h[0]) * a01 + i128(h[2]) * a12),
                     2 * (i128(h[0]) * a02 + i128(h[1]) * a12)};
  for (int k = 0; k < 3; ++k) {
    if (u[k] == 0 && v[k] == 0) continue;
    i128 l = i128(v[k]) * p0 - i128(u[k]) * p1;
    if (l < 0) l = -l;
    if (l > r[k]) return 0;
  }
  return 1;
}
// A : rend 0 alignes, 1 droite hors boite, 2 touche sans juge, 3 touche et juge.
static inline int variant_a(const RecT& r) {
  const P3 a{r.a[0], r.a[1], r.a[2]}, b{r.b[0], r.b[1], r.b[2]}, e{r.c[0], r.c[1], r.c[2]};
  const P3 A{a.x << kT, a.y << kT, a.z << kT}, Bs{b.x << kT, b.y << kT, b.z << kT}, E{e.x << kT, e.y << kT, e.z << kT};
  const Box Q{{r.lo[0], r.lo[1], r.lo[2]}, {r.hi[0], r.hi[1], r.hi[2]}};
  const int line = center_line_meets(A, geom::dot(A, A), Bs, geom::dot(Bs, Bs), E, geom::dot(E, E), Q);
  if (line < 0) return 0;
  if (line == 0) return 1;
  if (!geom::acute(a, b, e)) return 2;
  geom::Center ctr;
  geom::center3(a, b, e, ctr);
  return center_in_box(a, ctr, Q) ? 3 : 2;
}

int main(int argc, char** argv) {
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::fseek(f, 0, SEEK_END);
  size_t n = size_t(std::ftell(f)) / sizeof(RecT);
  std::fseek(f, 0, SEEK_SET);
  const int reps = argc > 2 ? std::atoi(argv[2]) : 3;
  if (argc > 3) n = std::min<size_t>(n, std::strtoull(argv[3], nullptr, 10));
  std::vector<RecT> v(n);
  if (std::fread(v.data(), sizeof(RecT), n, f) != n) return 2;
  std::fclose(f);
  std::vector<unsigned char> ra(n), rd(n);
  double best_a = 1e300, best_g = 1e300, best_1 = 1e300, best_2 = 1e300;
  unsigned long long cnt_a[4] = {}, fb_line = 0, fb_box = 0;
  constexpr size_t B = 1024;
  alignas(64) static double S[15][B];   // A', B', E' (repere local de boite, unites 2^-T), h ; puis a, b, e non mis a l'echelle relatifs a a
  alignas(64) static double o_line[B], o_judge[B];
  for (int rep = 0; rep < reps; ++rep) {
    unsigned long long c[4] = {};
    unsigned long long t0 = __rdtsc();
    for (size_t i = 0; i < n; ++i) { const int x = variant_a(v[i]); ra[i] = static_cast<unsigned char>(x); ++c[x]; }
    best_a = std::min(best_a, double(__rdtsc() - t0) / double(n));
    for (int k = 0; k < 4; ++k) cnt_a[k] = c[k];
    unsigned long long tg = 0, t1 = 0, t2 = 0;
    fb_line = fb_box = 0;
    for (size_t b0 = 0; b0 < n; b0 += B) {
      const size_t m = std::min(B, n - b0);
      t0 = __rdtsc();
      for (size_t i = 0; i < m; ++i) {
        const RecT& r = v[b0 + i];
        for (int k = 0; k < 3; ++k) {
          S[k][i] = double((i64(r.a[k]) << kT) - r.lo[k]);
          S[3 + k][i] = double((i64(r.b[k]) << kT) - r.lo[k]);
          S[6 + k][i] = double((i64(r.c[k]) << kT) - r.lo[k]);
          S[9 + k][i] = double(r.hi[k] - r.lo[k]);
        }
      }
      tg += __rdtsc() - t0;
      t0 = __rdtsc();
      for (size_t i = 0; i < m; ++i) {
        // --- droite des centres (lemme Z) en repere local (lo = 0, lo + hi = h) ; entiers < 2^25 exacts en double
        const double ax = S[0][i], ay = S[1][i], az = S[2][i], bx = S[3][i], by = S[4][i], bz = S[5][i], ex = S[6][i], ey = S[7][i], ez = S[8][i];
        const double h0 = S[9][i], h1 = S[10][i], h2 = S[11][i];
        const double u0 = ax - bx, u1 = ay - by, u2 = az - bz, v0 = ax - ex, v1 = ay - ey, v2 = az - ez;
        const double c01 = u0 * v1 - v0 * u1, c02 = u0 * v2 - v0 * u2, c12 = u1 * v2 - v1 * u2;  // exacts (< 2^51)
        const double a2 = ax * ax + ay * ay + az * az, b2 = bx * bx + by * by + bz * bz, e2 = ex * ex + ey * ey + ez * ez;  // exacts
        const double p0 = 2 * (a2 - b2) - 2 * (h0 * u0 + h1 * u1 + h2 * u2), p1 = 2 * (a2 - e2) - 2 * (h0 * v0 + h1 * v1 + h2 * v2);
        const double a01 = std::fabs(c01), a02 = std::fabs(c02), a12 = std::fabs(c12);
        const double r0 = 2 * (h1 * a01 + h2 * a02), r1 = 2 * (h0 * a01 + h2 * a12), r2 = 2 * (h0 * a02 + h1 * a12);
        const double q00 = v0 * p0, q01 = u0 * p1, q10 = v1 * p0, q11 = u1 * p1, q20 = v2 * p0, q21 = u2 * p1;
        const double l0 = std::fabs(q00 - q01), l1 = std::fabs(q10 - q11), l2 = std::fabs(q20 - q21);
        const double e0 = 0x1p-48 * (std::fabs(q00) + std::fabs(q01) + r0), e1 = 0x1p-48 * (std::fabs(q10) + std::fabs(q11) + r1),
                     e2b = 0x1p-48 * (std::fabs(q20) + std::fabs(q21) + r2);
        // marge par axe : r - l (axe inactif u_k = v_k = 0 : l = 0 <= r, marge >= 0, erreur nulle)
        const double m0 = r0 - l0, m1 = r1 - l1, m2 = r2 - l2;
        const double miss = std::max(std::max(-m0 - e0, -m1 - e1), -m2 - e2b);        // > 0 : manque certain
        const double hitm = std::min(std::min(m0 - e0, m1 - e1), m2 - e2b);           // > 0 : touche certaine (stricte)
        const double col = (c01 == 0) & (c02 == 0) & (c12 == 0);
        // code : 0 alignes, 1 manque certain, 2 touche certaine, 3 indecis
        o_line[i] = col != 0 ? 0.0 : (miss > 0 ? 1.0 : (hitm > 0 ? 2.0 : 3.0));
        // --- aigu (exact en double : produits scalaires < 2^39)
        const double U0 = (bx - ax) * 0x1p-6, U1 = (by - ay) * 0x1p-6, U2 = (bz - az) * 0x1p-6;
        const double V0 = (ex - ax) * 0x1p-6, V1 = (ey - ay) * 0x1p-6, V2 = (ez - az) * 0x1p-6;
        const double uu = U0 * U0 + U1 * U1 + U2 * U2, vv = V0 * V0 + V1 * V1 + V2 * V2, uv = U0 * V0 + U1 * V1 + U2 * V2;
        o_judge[i] = (uv > 0) & (uu - uv > 0) & (vv - uv > 0);
      }
      t1 += __rdtsc() - t0;
      t0 = __rdtsc();
      for (size_t i = 0; i < m; ++i) {
        int res;
        const int lc = int(o_line[i]);
        if (lc == 3) { ++fb_line; res = variant_a(v[b0 + i]); }
        else if (lc == 0) res = 0;
        else if (lc == 1) res = 1;
        else if (o_judge[i] == 0) res = 2;
        else {
          // centre q3 et boite en double avec borne d'erreur (scalaire, 32 % des candidats)
          const double ax = S[0][i], ay = S[1][i], az = S[2][i], h0 = S[9][i], h1 = S[10][i], h2 = S[11][i];
          const double U0 = (S[3][i] - ax) * 0x1p-6, U1 = (S[4][i] - ay) * 0x1p-6, U2 = (S[5][i] - az) * 0x1p-6;
          const double V0 = (S[6][i] - ax) * 0x1p-6, V1 = (S[7][i] - ay) * 0x1p-6, V2 = (S[8][i] - az) * 0x1p-6;
          const double uu = U0 * U0 + U1 * U1 + U2 * U2, vv = V0 * V0 + V1 * V1 + V2 * V2;
          const double w0 = U1 * V2 - U2 * V1, w1 = U2 * V0 - U0 * V2, w2 = U0 * V1 - U1 * V0;   // exacts (< 2^38)
          const double t0x = uu * V0 - vv * U0, t0y = uu * V1 - vv * U1, t0z = uu * V2 - vv * U2;
          const double N0 = t0y * w2 - t0z * w1, N1 = t0z * w0 - t0x * w2, N2 = t0x * w1 - t0y * w0;
          const double D = 2 * (w0 * w0 + w1 * w1 + w2 * w2);
          const double tb = uu * (std::fabs(V0) + std::fabs(V1) + std::fabs(V2)) + vv * (std::fabs(U0) + std::fabs(U1) + std::fabs(U2));
          const double wb = std::fabs(w0) + std::fabs(w1) + std::fabs(w2);
          const double eN = 0x1p-47 * tb * wb;
          const double g0 = 64 * N0 + ax * D, g1 = 64 * N1 + ay * D, g2 = 64 * N2 + az * D;
          const double eg0 = 64 * eN + 0x1p-49 * (std::fabs(64 * N0) + std::fabs(ax) * D), eg1 = 64 * eN + 0x1p-49 * (std::fabs(64 * N1) + std::fabs(ay) * D),
                       eg2 = 64 * eN + 0x1p-49 * (std::fabs(64 * N2) + std::fabs(az) * D);
          const double k0 = h0 * D - g0, k1 = h1 * D - g1, k2 = h2 * D - g2;
          const double ek0 = eg0 + 0x1p-50 * h0 * D, ek1 = eg1 + 0x1p-50 * h1 * D, ek2 = eg2 + 0x1p-50 * h2 * D;
          const double out = std::max(std::max(std::max(-g0 - eg0, -g1 - eg1), -g2 - eg2), std::max(std::max(-k0 - ek0, -k1 - ek1), -k2 - ek2));
          const double in = std::min(std::min(std::min(g0 - eg0, g1 - eg1), g2 - eg2), std::min(std::min(k0 - ek0, k1 - ek1), k2 - ek2));
          if (out > 0) res = 2;
          else if (in > 0) res = 3;
          else { ++fb_box; res = variant_a(v[b0 + i]); }
        }
        rd[b0 + i] = static_cast<unsigned char>(res);
      }
      t2 += __rdtsc() - t0;
    }
    best_g = std::min(best_g, double(tg) / double(n));
    best_1 = std::min(best_1, double(t1) / double(n));
    best_2 = std::min(best_2, double(t2) / double(n));
  }
  unsigned long long diff = 0;
  for (size_t i = 0; i < n; ++i) diff += ra[i] != rd[i];
  std::printf("triplets %zu | A HEAD %.1f cyc/triplet (alignes %llu, manque %llu, touche sans juge %llu, juge %llu)\n", n, best_a, cnt_a[0], cnt_a[1], cnt_a[2], cnt_a[3]);
  std::printf("D : rassemblement SoA %.1f cyc | phase 1 (droite + aigu, double, sans branchement) %.1f cyc | phase 2 (centre/boite double scalaire sur aigus touches + replis exacts) %.1f cyc | total hors rassemblement %.1f cyc/triplet\n",
              best_g, best_1, best_2, best_1 + best_2);
  std::printf("replis exacts : droite %llu (%.4f %%), boite %llu (%.4f %%) | desaccords A/D %llu\n", fb_line, 100.0 * double(fb_line) / double(n), fb_box,
              100.0 * double(fb_box) / double(n), diff);
  return diff ? 1 : 0;
}
