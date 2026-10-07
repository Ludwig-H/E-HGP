// Microbanc d'opportunite (audit L05) : recensement d'une sphere candidate sur la liste de la feuille (juges REELS).
//   A : HEAD : geom::side en i128 site par site, sortie anticipee des que p > theta ;
//   D : marge en double sans branchement sur tous les sites (borne d'erreur a priori), repli exact pour les indecis
//       hors generateurs (un generateur est sur la coquille par construction).
// Poids 1 (trame sans doublon). Resultat compare : (sortie anticipee) sinon (p, taille de coquille).
#include <x86intrin.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "arith/geometry.hpp"
using namespace mhgp10;
using geom::P3;

static inline double to_double(i128 v) { return static_cast<double>(v); }

int main(int argc, char** argv) {
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::fseek(f, 0, SEEK_END);
  const size_t bytes = size_t(std::ftell(f));
  std::fseek(f, 0, SEEK_SET);
  std::vector<int> raw(bytes / 4);
  if (std::fread(raw.data(), 4, raw.size(), f) != raw.size()) return 2;
  std::fclose(f);
  const int reps = argc > 2 ? std::atoi(argv[2]) : 3;
  // index des juges et centres exacts (hors chronometre)
  struct J { size_t off; int m, q, theta, anchor, g[4]; geom::Center c; };
  std::vector<J> js;
  for (size_t p = 0; p + 8 <= raw.size();) {
    J j;
    j.off = p + 8; j.m = raw[p]; j.q = raw[p + 1]; j.theta = raw[p + 2]; j.anchor = raw[p + 3];
    for (int k = 0; k < 4; ++k) j.g[k] = raw[p + 4 + k];
    auto pt = [&](int t) { return P3{raw[j.off + 3 * t], raw[j.off + 3 * t + 1], raw[j.off + 3 * t + 2]}; };
    if (j.q == 2) geom::center2(pt(j.g[0]), pt(j.g[1]), j.c);
    else if (j.q == 3) geom::center3(pt(j.g[0]), pt(j.g[1]), pt(j.g[2]), j.c);
    else geom::center4(pt(j.g[0]), pt(j.g[1]), pt(j.g[2]), pt(j.g[3]), j.c);
    js.push_back(j);
    p = j.off + 3 * size_t(j.m);
  }
  if (argc > 4) {
    const size_t off = std::strtoull(argv[3], nullptr, 10), w = std::strtoull(argv[4], nullptr, 10);
    if (off + w <= js.size()) js = std::vector<J>(js.begin() + off, js.begin() + off + w);
  }
  const size_t n = js.size();
  std::vector<double> dx_all(raw.size()), cd(4 * n);
  for (size_t i = 0; i < n; ++i) {
    const J& j = js[i];
    const int* c = raw.data() + j.off;
    for (int t = 0; t < j.m; ++t)
      for (int k = 0; k < 3; ++k) dx_all[j.off + 3 * t + k] = double(c[3 * t + k] - c[3 * j.anchor + k]);
    for (int k = 0; k < 3; ++k) cd[4 * i + k] = to_double(j.c.N[k]);
    cd[4 * i + 3] = to_double(j.c.D);
  }
  std::vector<int> ra(n), rd(n);
  unsigned long long sites_a = 0, sites_d = 0, fb = 0, exits = 0;
  double best_a = 1e300, best_d = 1e300;
  for (int rep = 0; rep < reps; ++rep) {
    sites_a = sites_d = fb = exits = 0;
    unsigned long long t0 = __rdtsc();
    for (size_t i = 0; i < n; ++i) {
      const J& j = js[i];
      const int* c = raw.data() + j.off;
      const P3 a{c[3 * j.anchor], c[3 * j.anchor + 1], c[3 * j.anchor + 2]};
      int p = 0, sh = 0, t = 0;
      bool out = false;
      for (; t < j.m; ++t) {
        const int s = geom::side(j.c, a, P3{c[3 * t], c[3 * t + 1], c[3 * t + 2]});
        if (s < 0) { if (++p > j.theta) { out = true; ++t; break; } }
        else if (s == 0) ++sh;
      }
      sites_a += t;
      exits += out;
      ra[i] = out ? -1 : p * 1024 + sh;
    }
    best_a = std::min(best_a, double(__rdtsc() - t0));
    t0 = __rdtsc();
    for (size_t i = 0; i < n; ++i) {
      const J& j = js[i];
      const int* c = raw.data() + j.off;
      const int m = j.m;
      const double* xs = dx_all.data() + j.off;  // differences a l'ancre, en double, preparees hors chronometre (SoA par feuille en vrai)
      const double N0 = cd[4 * i], N1 = cd[4 * i + 1], N2 = cd[4 * i + 2], D = cd[4 * i + 3];
      double pin = 0, unc = 0;
      for (int t = 0; t < m; ++t) {
        const double dx = xs[3 * t], dy = xs[3 * t + 1], dz = xs[3 * t + 2];
        const double lhs = D * (dx * dx + dy * dy + dz * dz);
        const double r0 = N0 * dx, r1 = N1 * dy, r2 = N2 * dz;
        const double rhs = 2 * (r0 + r1 + r2);
        const double eps = 0x1p-49 * (lhs + 2 * (std::fabs(r0) + std::fabs(r1) + std::fabs(r2)));
        const double mg = lhs - rhs;
        pin += mg < -eps;
        unc += !(std::fabs(mg) > eps);
      }
      sites_d += m;
      int p = int(pin), sh = j.q;
      if (int(unc) != j.q) {
        // des indecis hors generateurs (coincidence cospherique ou quasi) : passe exacte
        const P3 a{c[3 * j.anchor], c[3 * j.anchor + 1], c[3 * j.anchor + 2]};
        sh = 0;
        for (int t = 0; t < m; ++t) {
          const double dx = xs[3 * t], dy = xs[3 * t + 1], dz = xs[3 * t + 2];
          const double lhs = D * (dx * dx + dy * dy + dz * dz);
          const double r0 = N0 * dx, r1 = N1 * dy, r2 = N2 * dz;
          const double eps = 0x1p-49 * (lhs + 2 * (std::fabs(r0) + std::fabs(r1) + std::fabs(r2)));
          if (std::fabs(lhs - 2 * (r0 + r1 + r2)) > eps) continue;
          if (t == j.g[0] || t == j.g[1] || (j.q > 2 && t == j.g[2]) || (j.q > 3 && t == j.g[3])) { ++sh; continue; }
          ++fb;
          const int s = geom::side(j.c, a, P3{c[3 * t], c[3 * t + 1], c[3 * t + 2]});
          if (s < 0) ++p; else if (s == 0) ++sh;
        }
      }
      rd[i] = p > j.theta ? -1 : p * 1024 + sh;
    }
    best_d = std::min(best_d, double(__rdtsc() - t0));
  }
  unsigned long long diff = 0;
  for (size_t i = 0; i < n; ++i) diff += ra[i] != rd[i];
  std::printf("juges %zu | A HEAD : %.1f cyc/juge, %.1f cyc/site visite (%.1f sites/juge, sorties anticipees %llu)\n", n, best_a / double(n), best_a / double(sites_a),
              double(sites_a) / double(n), exits);
  std::printf("D double + repli : %.1f cyc/juge, %.1f cyc/site (%.1f sites/juge), replis exacts %llu (%.4f par juge) | desaccords %llu\n", best_d / double(n),
              best_d / double(sites_d), double(sites_d) / double(n), fb, double(fb) / double(n), diff);
  return diff ? 1 : 0;
}
