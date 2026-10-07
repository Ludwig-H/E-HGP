// Microbanc d'opportunite (audit L05), suite : filtre flottant des quadruplets en deux phases, sur lots SoA.
// Phase 1 (sans branchement, vectorisable) : classe chaque candidat en rejet certain (0) ou survivant/indecis (1).
// Phase 2 : les survivants passent par l'exact (barycentriques i128, boite). Meme borne d'erreur que micro_quads C.
// Le rassemblement SoA (u, v, s en double) est chronometre a part : dans une feuille reelle il se fait par gather.
#include <x86intrin.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>
typedef __int128 i128;
typedef long long i64;
constexpr int kT = 6;
struct RecQ { int a[3], b[3], c[3], d[3], lo[3], hi[3]; };
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
  for (int i = 0; i < 3; ++i) {
    const i64 u = r.b[i] - r.a[i], v = r.c[i] - r.a[i], s = r.d[i] - r.a[i];
    const i128 N = y.l1 * u + y.l2 * v + y.l3 * s;
    const i128 val = (i128(r.a[i]) * y.D + N) * (i128(1) << kT);
    if (val < i128(r.lo[i]) * y.D || val >= i128(r.hi[i]) * y.D) return false;
  }
  return true;
}
int main(int argc, char** argv) {
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::fseek(f, 0, SEEK_END);
  const size_t n = size_t(std::ftell(f)) / sizeof(RecQ);
  std::fseek(f, 0, SEEK_SET);
  std::vector<RecQ> v(n);
  if (std::fread(v.data(), sizeof(RecQ), n, f) != n) return 2;
  std::fclose(f);
  const int reps = argc > 2 ? std::atoi(argv[2]) : 5;
  constexpr size_t B = 1024;
  alignas(64) static double A[9][B];
  alignas(64) static double cls[B];
  double best_g = 1e300, best_1 = 1e300, best_2 = 1e300;
  unsigned long long surv = 0, judged = 0;
  for (int rep = 0; rep < reps; ++rep) {
    unsigned long long tg = 0, t1 = 0, t2 = 0;
    surv = judged = 0;
    for (size_t b0 = 0; b0 < n; b0 += B) {
      const size_t m = std::min(B, n - b0);
      unsigned long long t0 = __rdtsc();
      for (size_t i = 0; i < m; ++i)
        for (int k = 0; k < 3; ++k) {
          A[k][i] = v[b0 + i].b[k] - v[b0 + i].a[k];
          A[3 + k][i] = v[b0 + i].c[k] - v[b0 + i].a[k];
          A[6 + k][i] = v[b0 + i].d[k] - v[b0 + i].a[k];
        }
      tg += __rdtsc() - t0;
      const double *__restrict ux = A[0], *__restrict uy = A[1], *__restrict uz = A[2], *__restrict vx = A[3], *__restrict vy = A[4],
                   *__restrict vz = A[5], *__restrict sx = A[6], *__restrict sy = A[7], *__restrict sz = A[8];
      double* __restrict c = cls;
      t0 = __rdtsc();
      for (size_t i = 0; i < m; ++i) {
        const double uu = ux[i] * ux[i] + uy[i] * uy[i] + uz[i] * uz[i], vv = vx[i] * vx[i] + vy[i] * vy[i] + vz[i] * vz[i],
                     ss = sx[i] * sx[i] + sy[i] * sy[i] + sz[i] * sz[i];
        const double uv = ux[i] * vx[i] + uy[i] * vy[i] + uz[i] * vz[i], us = ux[i] * sx[i] + uy[i] * sy[i] + uz[i] * sz[i],
                     vs = vx[i] * sx[i] + vy[i] * sy[i] + vz[i] * sz[i];
        const double M = std::max(uu, std::max(vv, ss));
        const double eps = 0x1p-44 * M * M * M;
        const double m_vvss = vv * ss - vs * vs, m_uvss = uv * ss - vs * us, m_uvvs = uv * vs - vv * us;
        const double d3 = uu * m_vvss - uv * m_uvss + us * m_uvvs;
        const double l1 = uu * m_vvss - uv * (vv * ss - vs * ss) + us * (vv * vs - vv * ss);
        const double l2 = uu * (vv * ss - vs * ss) - uu * m_uvss + us * (uv * ss - vv * us);
        const double l3 = uu * (vv * ss - vs * vv) - uv * (uv * ss - vv * us) + uu * m_uvvs;
        const double l0 = 2 * d3 - l1 - l2 - l3;
        const double mn = std::min(std::min(l1, l2), std::min(l3, l0));
        // marge du rejet certain : > 0 ssi det certainement > 0 et une barycentrique certainement < 0
        c[i] = std::min(d3 - eps, -eps - mn);
      }
      t1 += __rdtsc() - t0;
      t0 = __rdtsc();
      for (size_t i = 0; i < m; ++i)
        if (!(c[i] > 0)) {
          ++surv;
          Bary y{};
          if (!bary_exact(v[b0 + i], y)) continue;
          if (!(y.l1 > 0 && y.l2 > 0 && y.l3 > 0 && y.D - y.l1 - y.l2 - y.l3 > 0)) continue;
          judged += box_from_bary(v[b0 + i], y);
        }
      t2 += __rdtsc() - t0;
    }
    best_g = std::min(best_g, double(tg) / double(n));
    best_1 = std::min(best_1, double(t1) / double(n));
    best_2 = std::min(best_2, double(t2) / double(n));
  }
  std::printf("quadruplets %zu | rassemblement SoA %.1f cyc | phase 1 (filtre, sans branchement) %.1f cyc | phase 2 (exact sur %llu survivants, %.2f %%) %.1f cyc amortis | total hors rassemblement %.1f cyc/quadruplet | juges %llu\n",
              n, best_g, best_1, surv, 100.0 * double(surv) / double(n), best_2, best_1 + best_2, judged);
  return 0;
}
