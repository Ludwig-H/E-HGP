// Auditeur C / piste D3 : cout de halo d'un pavage par tuiles, sur trames LiDAR.
// Usage : d3_tiles <fichier.u32le>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

using P = std::array<std::int64_t, 3>;
static std::vector<P> read(const char* path) {
  std::ifstream in(path, std::ios::binary);
  std::vector<unsigned char> b((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::vector<P> p(b.size() / 12);
  for (std::size_t i = 0; i < p.size(); ++i)
    for (int a = 0; a < 3; ++a) {
      std::uint32_t v = 0;
      for (int k = 0; k < 4; ++k) v |= std::uint32_t(b[(3 * i + a) * 4 + k]) << (8 * k);
      p[i][a] = v;
    }
  return p;
}

struct Grid {  // grille creuse de pas s (mm)
  std::int64_t s;
  std::unordered_map<std::uint64_t, std::vector<std::uint32_t>> cells;
  static std::uint64_t key(std::int64_t x, std::int64_t y, std::int64_t z) {
    return (std::uint64_t(x + 4096) << 42) | (std::uint64_t(y + 4096) << 21) | std::uint64_t(z + 4096);
  }
  Grid(const std::vector<P>& p, std::int64_t s_) : s(s_) {
    for (std::uint32_t i = 0; i < p.size(); ++i) cells[key(p[i][0] / s, p[i][1] / s, p[i][2] / s)].push_back(i);
  }
  // nombre de sites dans la boite [lo,hi] (fermee)
  std::uint64_t count_box(const std::vector<P>& p, const P& lo, const P& hi) const {
    std::uint64_t c = 0;
    for (std::int64_t x = std::max<std::int64_t>(0, lo[0]) / s; x <= hi[0] / s; ++x)
      for (std::int64_t y = std::max<std::int64_t>(0, lo[1]) / s; y <= hi[1] / s; ++y)
        for (std::int64_t z = std::max<std::int64_t>(0, lo[2]) / s; z <= hi[2] / s; ++z) {
          auto it = cells.find(key(x, y, z));
          if (it == cells.end()) continue;
          for (auto i : it->second) {
            bool in = true;
            for (int a = 0; a < 3; ++a) in = in && p[i][a] >= lo[a] && p[i][a] <= hi[a];
            c += in;
          }
        }
    return c;
  }
};

static void quant(std::vector<std::uint64_t> v, const char* name) {
  std::sort(v.begin(), v.end());
  auto q = [&](double f) { return v.empty() ? 0ull : (unsigned long long)v[std::min(v.size() - 1, (std::size_t)(f * v.size()))]; };
  std::printf("\"%s\":{\"p50\":%llu,\"p90\":%llu,\"p99\":%llu,\"max\":%llu}", name, q(0.5), q(0.9), q(0.99),
              v.empty() ? 0ull : (unsigned long long)v.back());
}

int main(int argc, char** argv) {
  const auto p = read(argv[1]);
  const std::size_t n = p.size();
  P lo{1 << 30, 1 << 30, 1 << 30}, hi{0, 0, 0};
  for (auto& x : p) for (int a = 0; a < 3; ++a) { lo[a] = std::min(lo[a], x[a]); hi[a] = std::max(hi[a], x[a]); }
  std::printf("{\"file\":\"%s\",\"n\":%zu,\"bbox_m\":[%.1f,%.1f,%.1f],", argv[1], n, (hi[0] - lo[0]) / 1e3,
              (hi[1] - lo[1]) / 1e3, (hi[2] - lo[2]) / 1e3);
  // 1) voisins a distance <= 2h par site (h = 0.25, 0.5 m)
  std::printf("\"neighbors_within_2h\":{");
  bool first = true;
  for (std::int64_t h : {125, 250, 500}) {
    const std::int64_t R = 2 * h;
    Grid g(p, R);
    std::vector<std::uint64_t> cnt(n, 0);
    for (std::size_t i = 0; i < n; ++i) {
      const auto cx = p[i][0] / R, cy = p[i][1] / R, cz = p[i][2] / R;
      std::uint64_t c = 0;
      for (int dx = -1; dx <= 1; ++dx) for (int dy = -1; dy <= 1; ++dy) for (int dz = -1; dz <= 1; ++dz) {
        auto it = g.cells.find(Grid::key(cx + dx, cy + dy, cz + dz));
        if (it == g.cells.end()) continue;
        for (auto j : it->second) {
          std::int64_t d2 = 0;
          for (int a = 0; a < 3; ++a) d2 += (p[i][a] - p[j][a]) * (p[i][a] - p[j][a]);
          c += (j != i && d2 <= R * R);
        }
      }
      cnt[i] = c;
    }
    double mean = 0; for (auto c : cnt) mean += c; mean /= n;
    std::printf("%s\"h%lld\":{\"mean\":%.1f,", first ? "" : ",", (long long)h, mean);
    quant(cnt, "q");
    std::printf("}");
    first = false;
  }
  std::printf("},");
  // 2) pavage kd (feuilles <= m sites), propriete par site, halo 2h autour de la boite des sites de la feuille
  //    et pavage par grille cubique de cote L, propriete par centre, halo h autour du cube.
  std::printf("\"kd_site_owner\":[");
  first = true;
  std::vector<std::uint32_t> ids(n);
  for (std::uint32_t i = 0; i < n; ++i) ids[i] = i;
  for (std::size_t m : {128, 512, 2048}) {
    std::vector<std::pair<std::size_t, std::size_t>> leaves;
    std::vector<std::uint32_t> v = ids;
    std::vector<std::pair<std::size_t, std::size_t>> st{{0, n}};
    while (!st.empty()) {
      auto [a, b] = st.back(); st.pop_back();
      if (b - a <= m) { leaves.push_back({a, b}); continue; }
      P l{1 << 30, 1 << 30, 1 << 30}, h{0, 0, 0};
      for (auto k = a; k < b; ++k) for (int c = 0; c < 3; ++c) { l[c] = std::min(l[c], p[v[k]][c]); h[c] = std::max(h[c], p[v[k]][c]); }
      int ax = 0; for (int c = 1; c < 3; ++c) if (h[c] - l[c] > h[ax] - l[ax]) ax = c;
      const auto mid = a + (b - a) / 2;
      std::nth_element(v.begin() + a, v.begin() + mid, v.begin() + b, [&](auto x, auto y) { return p[x][ax] < p[y][ax]; });
      st.push_back({a, mid}); st.push_back({mid, b});
    }
    for (std::int64_t h : {250, 500, 1000, 2000}) {
      Grid g(p, std::max<std::int64_t>(2 * h, 500));
      std::vector<std::uint64_t> ext;
      std::uint64_t sum = 0;
      for (auto [a, b] : leaves) {
        P l{1 << 30, 1 << 30, 1 << 30}, hh{0, 0, 0};
        for (auto k = a; k < b; ++k) for (int c = 0; c < 3; ++c) { l[c] = std::min(l[c], p[v[k]][c]); hh[c] = std::max(hh[c], p[v[k]][c]); }
        for (int c = 0; c < 3; ++c) { l[c] -= 2 * h; hh[c] += 2 * h; }
        const auto e = g.count_box(p, l, hh);
        ext.push_back(e); sum += e;
      }
      std::printf("%s{\"m\":%zu,\"h_mm\":%lld,\"tiles\":%zu,\"dup\":%.2f,", first ? "" : ",", m, (long long)h, leaves.size(),
                  double(sum) / n);
      quant(ext, "ext");
      std::printf("}");
      first = false;
    }
  }
  std::printf("],\"grid_center_owner\":[");
  first = true;
  for (std::int64_t L : {1000, 2000, 4000}) {
    for (std::int64_t h : {250, 500, 1000}) {
      // cellules de cote L a distance <= h d'au moins un site
      Grid gL(p, L);
      Grid g(p, std::max<std::int64_t>(h, 500));
      std::unordered_map<std::uint64_t, int> tiles;
      const std::int64_t reach = (h + L - 1) / L;
      for (auto& [k, vv] : gL.cells) {
        (void)vv;
        const std::int64_t x = std::int64_t(k >> 42) - 4096, y = std::int64_t((k >> 21) & ((1u << 21) - 1)) - 4096,
                           z = std::int64_t(k & ((1u << 21) - 1)) - 4096;
        for (auto dx = -reach; dx <= reach; ++dx) for (auto dy = -reach; dy <= reach; ++dy) for (auto dz = -reach; dz <= reach; ++dz)
          tiles[Grid::key(x + dx, y + dy, z + dz)] = 1;
      }
      std::vector<std::uint64_t> ext, own;
      std::uint64_t sum = 0, nonempty = 0;
      for (auto& [k, one] : tiles) {
        (void)one;
        const std::int64_t x = std::int64_t(k >> 42) - 4096, y = std::int64_t((k >> 21) & ((1u << 21) - 1)) - 4096,
                           z = std::int64_t(k & ((1u << 21) - 1)) - 4096;
        P l{x * L - h, y * L - h, z * L - h}, hh{(x + 1) * L - 1 + h, (y + 1) * L - 1 + h, (z + 1) * L - 1 + h};
        const auto e = g.count_box(p, l, hh);
        if (e == 0) continue;
        ++nonempty; ext.push_back(e); sum += e;
      }
      std::printf("%s{\"L_mm\":%lld,\"h_mm\":%lld,\"tiles\":%llu,\"dup\":%.2f,", first ? "" : ",", (long long)L, (long long)h,
                  (unsigned long long)nonempty, double(sum) / n);
      quant(ext, "ext");
      std::printf("}");
      first = false;
    }
  }
  std::printf("]}\n");
  return 0;
}
