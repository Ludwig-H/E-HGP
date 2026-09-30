// Sonde d'audit : publier les representants des naissances, sans refaire la descente du produit.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "tower/tower.hpp"
using namespace mhgp10;

int main(int argc, char** argv) {
  if (argc != 4) return 2;
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::vector<u32> x, y, z, pid;
  u32 p[3];
  while (std::fread(p, 4, 3, f) == 3) {
    x.push_back(p[0]); y.push_back(p[1]); z.push_back(p[2]); pid.push_back(u32(pid.size()));
  }
  std::fclose(f);
  auto cloud = prepare_cloud(x, y, z, pid, kCoordinateBits);
  if (!cloud.ok()) return 2;
  sched::Pool pool(unsigned(std::atoi(argv[3])));
  SiteTree tree(cloud.value());
  CatalogueParams cp;
  cp.kmax = std::atoi(argv[2]);
  auto cat = build_catalogue(cloud.value(), cp, pool);
  if (!cat.ok()) return 2;
  for (u32 s = 0; s < cloud.value().sites(); ++s)
    std::printf("site %u %u %u %u\n", s, cloud.value().x[s], cloud.value().y[s], cloud.value().z[s]);
  for (PointEntry mode : {PointEntry::core, PointEntry::cover}) {
    TowerParams tp;
    tp.kmax = cp.kmax;
    tp.entry = mode;
    tp.ball_nodes = true;
    auto tw = build_tower(cloud.value(), tree, cat.value(), tp, pool);
    if (!tw.ok()) {
      std::printf("error %s %s %u\n", mode == PointEntry::core ? "core" : "cover",
                  std::string(reason_name(tw.outcome().reason)).c_str(), unsigned(tw.outcome().order));
      return 3;
    }
    std::printf("mode %s\n", mode == PointEntry::core ? "core" : "cover");
    auto level = [&](u32 r) {
      if (r == 0) { std::printf(" 0 1"); return; }
      const auto& lv = cat.value().level[r - 1];
      std::printf(" %s %s", arith::to_string(lv.num).c_str(), arith::to_string(lv.den).c_str());
    };
    for (const auto& ord : tw.value().orders) {
      for (u32 v = 0; v < ord.rank.size(); ++v) {
        std::printf("node %d %u %lld", ord.k, v, ord.parent[v] == kNone ? -1LL : (long long)ord.parent[v]);
        level(ord.rank[v]);
        std::printf(" %lld", ord.lower.empty() ? -1LL : (long long)ord.lower[v]);
        if (ord.birth[v] != kNone) {
          if (ord.k == 1) std::printf(" %u", ord.birth[v]);
          else {
            const auto I = cat.value().interior(ord.birth[v]);
            const auto U = cat.value().shell(ord.birth[v]);
            for (u32 s : I) std::printf(" %u", s);
            for (u32 i = 0; i < u32(ord.k) - I.size(); ++i) std::printf(" %u", U[i]);
          }
        }
        std::puts("");
      }
      for (u32 s = 0; s < cloud.value().sites(); ++s) {
        std::printf("point %d %u %u", ord.k, s, ord.point_node[s]);
        if (!ord.point_cat_rank.empty()) level(ord.point_cat_rank[s]);
        else std::printf(" %llu 1", (unsigned long long)ord.point_level[s]);
        std::puts("");
      }
      for (u32 b = 0; b < ord.ball_node.size(); ++b) {
        if (ord.ball_node[b] == kNone) continue;
        std::printf("ball %d %u %u", ord.k, b, ord.ball_node[b]);
        level(cat.value().rank[b] + 1);
        for (u32 s : cat.value().interior(b)) std::printf(" %u", s);
        for (u32 s : cat.value().shell(b)) std::printf(" %u", s);
        std::puts("");
      }
    }
  }
}
