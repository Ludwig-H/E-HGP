// Oracle exact independant de la v9 (hors produit) : frequence, sur LiDAR 1 mm,
// des presentations q3 du catalogue (triangle strictement aigu, r <= h,
// p + 3 <= Kmax + 1) dont la plus longue arete est EX AEQUO.
// Dans ce cas l'arete proprietaire v9 est choisie par edge_key sur les IDs
// (wspd_q34.cpp:722-726, q4_local.cpp:27-35) : la propriete depend alors de
// l'ordre des IDs, pas seulement de P n B.
// Usage : tie_oracle file.u32le h_mm Kmax
#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

using i64 = std::int64_t;
using i128 = __int128;
struct P { i64 x, y, z; };

static std::vector<P> read_u32le(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  const std::vector<unsigned char> b((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::vector<P> p(b.size() / 12);
  for (std::size_t i = 0; i < p.size(); ++i) {
    i64 c[3];
    for (int a = 0; a < 3; ++a) {
      std::uint32_t v = 0;
      for (int k = 0; k < 4; ++k) v |= std::uint32_t(b[(3 * i + a) * 4 + k]) << (8 * k);
      c[a] = v;
    }
    p[i] = {c[0], c[1], c[2]};
  }
  return p;
}
static i64 d2(const P& a, const P& b) {
  const i64 dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
  return dx * dx + dy * dy + dz * dz;
}

int main(int argc, char** argv) {
  if (argc < 4) { std::fprintf(stderr, "usage: tie_oracle file h_mm Kmax\n"); return 2; }
  const auto pts = read_u32le(argv[1]);
  const i64 h = std::stoll(argv[2]);
  const int kmax = std::stoi(argv[3]);
  const std::size_t n = pts.size();
  const i64 cell = 2 * h, R2 = 4 * h * h;
  std::unordered_map<std::uint64_t, std::vector<std::uint32_t>> grid;
  auto key = [&](i64 gx, i64 gy, i64 gz) {
    return (std::uint64_t(gx) << 42) | (std::uint64_t(gy) << 21) | std::uint64_t(gz);
  };
  for (std::uint32_t i = 0; i < n; ++i)
    grid[key(pts[i].x / cell, pts[i].y / cell, pts[i].z / cell)].push_back(i);
  std::uint64_t acute_small = 0, tied = 0, tied_cat = 0, tied_cat_split = 0;
  std::uint64_t cat_q3 = 0;
  std::vector<std::uint32_t> nb;
  for (std::uint32_t i = 0; i < n; ++i) {
    nb.clear();
    const auto& a = pts[i];
    const i64 gx = a.x / cell, gy = a.y / cell, gz = a.z / cell;
    for (i64 dx = -1; dx <= 1; ++dx)
      for (i64 dy = -1; dy <= 1; ++dy)
        for (i64 dz = -1; dz <= 1; ++dz) {
          if (gx + dx < 0 || gy + dy < 0 || gz + dz < 0) continue;
          auto it = grid.find(key(gx + dx, gy + dy, gz + dz));
          if (it == grid.end()) continue;
          for (auto j : it->second)
            if (j != i && d2(a, pts[j]) <= R2) nb.push_back(j);
        }
    for (std::size_t s = 0; s < nb.size(); ++s) {
      const auto j = nb[s];
      if (j < i) continue;
      for (std::size_t t = 0; t < nb.size(); ++t) {
        const auto k = nb[t];
        if (k <= j) continue;
        const P& b = pts[j];
        const P& x = pts[k];
        const i64 ab = d2(a, b), ax = d2(a, x), bx = d2(b, x);
        if (bx > R2) continue;
        // strictly acute: each side^2 < sum of the two others
        if (!(ab < ax + bx && ax < ab + bx && bx < ab + ax)) continue;
        const i64 m = std::max(ab, std::max(ax, bx));
        const int ties = (ab == m) + (ax == m) + (bx == m);
        // exact circumcenter relative to a: c-a = N/den
        const i64 ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z;
        const i64 vx = x.x - a.x, vy = x.y - a.y, vz = x.z - a.z;
        const i128 wx = (i128)uy * vz - (i128)uz * vy, wy = (i128)uz * vx - (i128)ux * vz,
                   wz = (i128)ux * vy - (i128)uy * vx;
        const i128 uu = ux * ux + uy * uy + uz * uz, vv = vx * vx + vy * vy + vz * vz;
        // v x w and w x u
        const i128 vwx = vy * wz - vz * wy, vwy = vz * wx - vx * wz, vwz = vx * wy - vy * wx;
        const i128 wux = wy * uz - wz * uy, wuy = wz * ux - wx * uz, wuz = wx * uy - wy * ux;
        const i128 Nx = uu * vwx + vv * wux, Ny = uu * vwy + vv * wuy, Nz = uu * vwz + vv * wuz;
        const i128 den = 2 * (wx * wx + wy * wy + wz * wz);
        if (den == 0) continue;
        // r^2 = |N|^2/den^2 <= h^2
        const i128 NN = Nx * Nx + Ny * Ny + Nz * Nz;
        if (NN > (i128)h * h * den * den) continue;
        ++acute_small;
        if (ties >= 2) ++tied;
        // census on the full neighbour list of a (every site of the closed ball
        // is within 2r <= 2h of a)
        int p = 0, u = 3;
        for (auto z : nb) {
          if (z == j || z == k) continue;
          const i64 zx = pts[z].x - a.x, zy = pts[z].y - a.y, zz = pts[z].z - a.z;
          const i128 lhs = (i128)(zx * zx + zy * zy + zz * zz) * den - 2 * (zx * Nx + zy * Ny + zz * Nz);
          if (lhs < 0) ++p; else if (lhs == 0) ++u;
          if (p + 3 > kmax + 1) break;
        }
        if (p + 3 > kmax + 1) continue;
        ++cat_q3;
        if (ties >= 2) {
          ++tied_cat;
          // tied longest edges share a vertex (the apex); ownership endpoint
          // a_min (smallest id) differs between the tied edges iff the apex is
          // not the smallest id of the triple.
          std::uint32_t apex;
          if (ab == m && ax == m) apex = i; else if (ab == m && bx == m) apex = j; else apex = k;
          if (apex != i) ++tied_cat_split;
          if (tied_cat <= 3)
            std::printf("exemple: ids %u %u %u ab2=%lld ax2=%lld bx2=%lld p=%d u=%d\n", i, j, k,
                        (long long)ab, (long long)ax, (long long)bx, p, u);
        }
      }
    }
  }
  std::printf("{\"file\":\"%s\",\"h_mm\":%lld,\"kmax\":%d,\"n\":%zu,\"acute_r_le_h\":%llu,"
              "\"acute_r_le_h_tied\":%llu,\"q3_cat_r_le_h\":%llu,\"q3_cat_tied\":%llu,"
              "\"q3_cat_tied_amin_split\":%llu}\n",
              argv[1], (long long)h, kmax, n, (unsigned long long)acute_small, (unsigned long long)tied,
              (unsigned long long)cat_q3, (unsigned long long)tied_cat, (unsigned long long)tied_cat_split);
  return 0;
}
