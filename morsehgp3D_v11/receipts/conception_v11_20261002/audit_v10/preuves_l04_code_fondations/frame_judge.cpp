// L04 audit : juge d'echantillon de SiteTree sur une trame LiDAR entiere (n ~ 4e4), force brute exacte.
// Centres : spheres minimales de paires, de triangles NON obtus et de tetraedres a centre interieur (centre dans
// l'enveloppe convexe : le cas d'emploi de la tour), tires parmi des voisins proches ET parmi des sites lointains.
#include <algorithm>
#include <cstdio>
#include <random>
#include <vector>

#include "cloud/site_tree.hpp"

using namespace mhgp10;

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::vector<u32> raw;
  u32 buf[3];
  while (std::fread(buf, 4, 3, f) == 3) raw.insert(raw.end(), buf, buf + 3);
  std::fclose(f);
  const u32 n = u32(raw.size() / 3);
  std::vector<u32> x(n), y(n), z(n), pid(n);
  for (u32 i = 0; i < n; ++i) {
    x[i] = raw[3 * i];
    y[i] = raw[3 * i + 1];
    z[i] = raw[3 * i + 2];
    pid[i] = i;
  }
  auto r = prepare_cloud(x, y, z, pid, 18);
  if (!r.ok()) return 2;
  const Cloud& c = r.value();
  SiteTree tree(c);
  auto P = [&](u32 s) { return geom::P3{i64(c.x[s]), i64(c.y[s]), i64(c.z[s])}; };
  std::mt19937_64 g(2026);
  u64 q2 = 0, q3 = 0, q4 = 0, bad_ball = 0, bad_near = 0, shells_gt = 0, bad_kth = 0, bad_within = 0, kth = 0;
  std::vector<std::pair<i128, u32>> want, got;
  std::vector<u32> wi, wu, gi, gu;
  const u32 S = c.sites();
  for (int t = 0; t < 30000; ++t) {
    u32 s[4];
    s[0] = u32(g() % S);
    const bool local = (t % 4) != 0;  // 3 sur 4 : voisins dans l'ordre de Morton ; 1 sur 4 : sites quelconques
    for (int a = 1; a < 4; ++a) s[a] = local ? u32((s[0] + 1 + g() % 40) % S) : u32(g() % S);
    const geom::P3 A = P(s[0]);
    geom::Center ctr{{0, 0, 0}, 1};
    const u64 ar = g() % 10; const int arity = ar < 1 ? 2 : (ar < 3 ? 3 : 4);
    if (arity == 2) {
      geom::center2(A, P(s[1]), ctr);
      ++q2;
    } else if (arity == 3) {
      const geom::P3 B = P(s[1]), C = P(s[2]);
      if (geom::dot(geom::sub(B, A), geom::sub(C, A)) < 0 || geom::dot(geom::sub(A, B), geom::sub(C, B)) < 0 ||
          geom::dot(geom::sub(A, C), geom::sub(B, C)) < 0)
        continue;
      if (!geom::center3(A, B, C, ctr)) continue;
      ++q3;
    } else {
      const geom::P3 B = P(s[1]), C = P(s[2]), D = P(s[3]);
      if (!geom::center4(A, B, C, D, ctr)) continue;
      const geom::P3* tt[4] = {&A, &B, &C, &D};
      if (!geom::strictly_inside_tetra(tt, A, ctr)) continue;
      ++q4;
    }
    want.clear();
    wi.clear();
    wu.clear();
    for (u32 zz = 0; zz < S; ++zz) {
      const i128 key = geom::side_key(ctr, A, P(zz));
      want.push_back({key, zz});
      if (key < 0) wi.push_back(zz);
      else if (key == 0) wu.push_back(zz);
    }
    tree.closed_ball(A, ctr, gi, gu);
    bad_ball += !(gi == wi && gu == wu);
    shells_gt += wu.size() > size_t(arity);
    const u32 count = 1 + u32(g() % 12);
    std::partial_sort(want.begin(), want.begin() + count, want.end());
    tree.nearest(A, ctr, count, got);
    bool same = got.size() == count;
    for (u32 i = 0; same && i < count; ++i) same = got[i] == want[i];
    bad_near += !same;
  }
  // requetes entieres : D_k et boule fermee entiere
  std::vector<u64> d2(S);
  std::vector<u32> gw;
  for (int t = 0; t < 600; ++t) {
    const u32 s = u32(g() % S);
    const i64 qx = c.x[s] + i64(g() % 7) - 3, qy = c.y[s] + i64(g() % 7) - 3, qz = c.z[s] + i64(g() % 7) - 3;
    for (u32 zz = 0; zz < S; ++zz) {
      const i64 dx = i64(c.x[zz]) - qx, dy = i64(c.y[zz]) - qy, dz = i64(c.z[zz]) - qz;
      d2[zz] = u64(dx * dx + dy * dy + dz * dz);
    }
    std::vector<u64> sorted = d2;
    std::partial_sort(sorted.begin(), sorted.begin() + 12, sorted.end());
    for (u64 k : {1ull, 2ull, 5ull, 10ull, 12ull}) {
      bad_kth += tree.kth_distance(qx, qy, qz, k) != sorted[k - 1];
      ++kth;
    }
    const u64 r2 = sorted[9];
    tree.within(qx, qy, qz, r2, gw);
    u32 cnt = 0;
    bool ok = true;
    for (u32 zz = 0; zz < S; ++zz)
      if (d2[zz] <= r2) {
        ok &= cnt < gw.size() && gw[cnt] == zz;
        ++cnt;
      }
    bad_within += !(ok && cnt == gw.size());
  }
  std::printf("sites=%u ; spheres jugees : q2=%llu q3=%llu q4=%llu (coquilles etendues %llu)\n", S, (unsigned long long)q2,
              (unsigned long long)q3, (unsigned long long)q4, (unsigned long long)shells_gt);
  std::printf("closed_ball faux=%llu nearest faux=%llu kth_distance faux=%llu/%llu within faux=%llu/600\n",
              (unsigned long long)bad_ball, (unsigned long long)bad_near, (unsigned long long)bad_kth, (unsigned long long)kth,
              (unsigned long long)bad_within);
  const bool floor_ok = q2 >= 500 && q3 >= 200 && q4 >= 20;
  if (!floor_ok) std::printf("PLANCHER non atteint\n");
  return (bad_ball || bad_near || bad_kth || bad_within) ? 1 : (floor_ok ? 0 : 3);
}
