// L04 audit : SiteTree::closed_ball / nearest avec un centre rationnel LOIN de la boite des sites (u18).
// Trois sites presque alignes : a = (0,0,0), b = (100000,1,0), c = (200000,0,0). Centre circonscrit
// (100000, -(10^10 - 1)/2, 0), rayon ~ 5e9 : hors de l'enveloppe convexe (triangle obtus). Les sites (100000 + p, 0, q)
// avec p^2 + q^2 = 10^10 sont EXACTEMENT sur la sphere. Juge : force brute avec geom::side_key (exact, i128).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

#include "cloud/site_tree.hpp"

using namespace mhgp10;

int main() {
  std::vector<u32> x, y, z, pid;
  auto add = [&](u32 a, u32 b, u32 c) {
    x.push_back(a);
    y.push_back(b);
    z.push_back(c);
    pid.push_back(u32(pid.size()));
  };
  add(0, 0, 0);
  add(100000, 1, 0);
  add(200000, 0, 0);
  // points entiers du cercle p^2 + q^2 = 10^10, q > 0
  u32 on_sphere = 0, near_sphere = 0;
  for (long long p = -100000; p <= 100000; ++p) {
    const long long q2 = 10000000000LL - p * p;
    long long q = static_cast<long long>(std::sqrt(static_cast<double>(q2)));  // racine entiere, corrigee ci-dessous
    while (q * q > q2) --q;
    while ((q + 1) * (q + 1) <= q2) ++q;
    if (q > 0 && q * q == q2) {
      add(u32(100000 + p), 0, u32(q));
      ++on_sphere;
    }
    // sites a moins de 4096 (en distance carree) de la sphere, de part et d'autre : e = p^2 + q^2 - 10^10
    for (long long qq : {q, q + 1}) {
      const long long e = p * p + qq * qq - 10000000000LL;
      if (qq > 0 && qq <= 100000 && e != 0 && e > -4096 && e < 4096) {
        add(u32(100000 + p), 0, u32(qq));
        ++near_sphere;
      }
    }
  }
  // trois sites pres du point de la boite le plus proche du centre : distances carrees au centre
  // y0^2 + {0, 2116, 4050} ; en double (ulp 4096 a 2,5e19) les deux derniers sont intervertis.
  add(100000, 0, 0);
  add(100046, 0, 0);
  add(100045, 0, 45);
  std::mt19937_64 g(5);
  for (int i = 0; i < 3000; ++i) add(u32(g() % 200001), u32(g() % 3), u32(g() % 100001));
  auto r = prepare_cloud(x, y, z, pid, 18);
  if (!r.ok()) return 2;
  const Cloud& c = r.value();
  SiteTree tree(c);
  auto P = [&](u32 s) { return geom::P3{i64(c.x[s]), i64(c.y[s]), i64(c.z[s])}; };
  const geom::P3 A{0, 0, 0}, B{100000, 1, 0}, C{200000, 0, 0};
  geom::Center ctr{};
  if (!geom::center3(A, B, C, ctr)) return 2;
  std::vector<u32> wi, wu, gi, gu;
  std::vector<std::pair<i128, u32>> want, got;
  for (u32 s = 0; s < c.sites(); ++s) {
    const i128 k = geom::side_key(ctr, A, P(s));
    want.push_back({k, s});
    if (k < 0) wi.push_back(s);
    else if (k == 0) wu.push_back(s);
  }
  std::sort(want.begin(), want.end());
  tree.closed_ball(A, ctr, gi, gu);
  std::vector<u32> lost_shell, false_interior, lost_interior;
  for (u32 s : wu)
    if (!std::binary_search(gu.begin(), gu.end(), s)) lost_shell.push_back(s);
  for (u32 s : gi)
    if (!std::binary_search(wi.begin(), wi.end(), s)) false_interior.push_back(s);
  for (u32 s : wi)
    if (!std::binary_search(gi.begin(), gi.end(), s)) lost_interior.push_back(s);
  std::printf("sites %u sur_la_sphere_construits %u proches_de_la_sphere %u\n", c.sites(), on_sphere + 3, near_sphere);
  std::printf("exact : interieur %zu coquille %zu | closed_ball : interieur %zu coquille %zu\n", wi.size(), wu.size(),
              gi.size(), gu.size());
  std::printf("coquille perdue %zu ; faux interieurs %zu ; interieurs perdus %zu\n", lost_shell.size(),
              false_interior.size(), lost_interior.size());
  u32 bad_nearest = 0;
  for (u32 count : {1u, 2u, 3u, 7u, 12u, 30u}) {
    tree.nearest(A, ctr, count, got);
    bool same = got.size() == std::min<size_t>(count, want.size());
    for (size_t i = 0; same && i < got.size(); ++i) same = got[i] == want[i];
    if (!same) ++bad_nearest;
    std::printf("nearest count=%u : %s\n", count, same ? "conforme" : "FAUX");
  }
  const bool wrong = !lost_shell.empty() || !false_interior.empty() || !lost_interior.empty() || bad_nearest;
  std::printf("%s\n", wrong ? "far_center_FAUX" : "far_center_ok");
  return wrong ? 1 : 0;
}
