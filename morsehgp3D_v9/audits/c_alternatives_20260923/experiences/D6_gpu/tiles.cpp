// Statistiques de tuiles (D3/D6) : points par tuile cubique de cote T et par tuile elargie d'un halo h <= T.
// Sert a dimensionner une execution GPU par tuile en memoire partagee. Entree u32le (mm).
#include <cstdio>
#include <cstdint>
#include <vector>
#include <map>
#include <array>
#include <algorithm>
#include <tuple>
int main(int argc, char** argv) {
  for (int f = 1; f < argc; ++f) {
    FILE* fp = std::fopen(argv[f], "rb");
    std::vector<std::array<int64_t,3>> P;
    uint32_t t[3];
    while (std::fread(t, 4, 3, fp) == 3) P.push_back({t[0], t[1], t[2]});
    std::fclose(fp);
    const size_t n = P.size();
    std::printf("%s n=%zu\n", argv[f], n);
    const int64_t cfg[][2] = {{1000,1000},{2000,2000}};
    for (auto& c : cfg) {
      const int64_t T = c[0], h = c[1];
      std::map<std::tuple<int64_t,int64_t,int64_t>, std::vector<size_t>> tiles;
      for (size_t i = 0; i < n; ++i) tiles[{P[i][0]/T, P[i][1]/T, P[i][2]/T}].push_back(i);
      std::vector<size_t> core, tot;
      for (auto& [key, ids] : tiles) {
        auto [tx,ty,tz] = key;
        const int64_t lo[3] = {tx*T - h, ty*T - h, tz*T - h}, hi[3] = {(tx+1)*T + h, (ty+1)*T + h, (tz+1)*T + h};
        size_t s = 0;
        const int R = (int)((h + T - 1) / T); for (int dx=-R; dx<=R; ++dx) for (int dy=-R; dy<=R; ++dy) for (int dz=-R; dz<=R; ++dz) {
          auto it = tiles.find({tx+dx, ty+dy, tz+dz});
          if (it == tiles.end()) continue;
          for (size_t i : it->second) {
            bool in = true;
            for (int a = 0; a < 3; ++a) in = in && P[i][a] >= lo[a] && P[i][a] < hi[a];
            s += in;
          }
        }
        core.push_back(ids.size()); tot.push_back(s);
      }
      auto q = [](std::vector<size_t> v, double fr) { std::sort(v.begin(), v.end()); return v[std::min(v.size()-1, (size_t)(fr*v.size()))]; };
      double dup = 0; size_t big = 0, bigpts = 0, big8 = 0, big8pts = 0;
      for (size_t i = 0; i < tot.size(); ++i) {
        dup += tot[i];
        if (tot[i] > 2048) { ++big; bigpts += core[i]; }
        if (tot[i] > 8192) { ++big8; big8pts += core[i]; }
      }
      std::printf("  T=%lldmm h=%lldmm tuiles=%zu coeur p50/p90/p99/max=%zu/%zu/%zu/%zu  tuile+halo p50/p90/p99/max=%zu/%zu/%zu/%zu  "
                  "duplication=%.2f  tuiles>2048: %zu (%.1f%% des sites)  >8192: %zu (%.1f%% des sites)\n",
                  (long long)T, (long long)h, core.size(), q(core,.5), q(core,.9), q(core,.99), *std::max_element(core.begin(), core.end()),
                  q(tot,.5), q(tot,.9), q(tot,.99), *std::max_element(tot.begin(), tot.end()), dup / n,
                  big, 100.0*bigpts/n, big8, 100.0*big8pts/n);
    }
  }
}
