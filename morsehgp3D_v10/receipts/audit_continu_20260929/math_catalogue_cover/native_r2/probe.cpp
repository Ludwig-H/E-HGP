// Audit-only exporter, linked against an existing pinned archive. No engine rebuild.
#include <fstream>
#include <iostream>
#include <vector>
#include "tower/tower.hpp"

using namespace mhgp10;

template <class Range> void array(const Range& a) {
  std::cout << '[';
  bool comma = false;
  for (auto v : a) { if (comma) std::cout << ','; comma = true; std::cout << v; }
  std::cout << ']';
}

int main(int argc, char** argv) {
  if (argc != 5) return 2;
  std::ifstream in(argv[1]);
  std::vector<u32> xs, ys, zs, ids;
  u32 x, y, z;
  while (in >> x >> y >> z) { xs.push_back(x); ys.push_back(y); zs.push_back(z); ids.push_back(u32(ids.size())); }
  auto cloud_result = prepare_cloud(xs, ys, zs, ids);
  if (!cloud_result.ok()) return 2;
  const auto& cloud = cloud_result.value();
  sched::Pool pool(unsigned(std::stoul(argv[3])));
  SiteTree tree(cloud);
  CatalogueParams cp; cp.kmax = std::stoi(argv[2]);
  auto cat_result = build_catalogue(cloud, cp, pool);
  if (!cat_result.ok()) { std::cerr << reason_name(cat_result.outcome().reason); return 2; }
  const auto& cat = cat_result.value();
  TowerParams tp; tp.kmax = cp.kmax; tp.entry = PointEntry::cover; tp.ball_nodes = std::stoi(argv[4]) != 0;
  auto tower_result = build_tower(cloud, tree, cat, tp, pool);
  if (!tower_result.ok()) { std::cerr << reason_name(tower_result.outcome().reason); return 3; }
  std::cout << "{\"points\":[";
  for (u32 i = 0; i < cloud.sites(); ++i) {
    if (i) std::cout << ',';
    std::cout << '[' << cloud.x[i] << ',' << cloud.y[i] << ',' << cloud.z[i] << ']';
  }
  std::cout << "],\"levels\":[";
  for (std::size_t i = 0; i < cat.level.size(); ++i) {
    if (i) std::cout << ',';
    std::cout << "[\"" << arith::to_string(cat.level[i].num) << "\",\""
              << arith::to_string(cat.level[i].den) << "\"]";
  }
  std::cout << "],\"balls\":[";
  for (u32 b = 0; b < cat.balls(); ++b) {
    if (b) std::cout << ',';
    std::cout << "{\"rank\":" << cat.rank[b] << ",\"q\":" << unsigned(cat.qmin[b])
              << ",\"p\":" << cat.p[b] << ",\"u\":" << cat.u[b] << ",\"S\":";
    array(cat.support[b]); std::cout << ",\"I\":"; array(cat.interior(b));
    std::cout << ",\"U\":"; array(cat.shell(b)); std::cout << '}';
  }
  std::cout << "],\"orders\":[";
  bool comma = false;
  for (const auto& order : tower_result.value().orders) {
    if (comma) std::cout << ',';
    comma = true;
    std::cout << "{\"k\":" << order.k << ",\"rank\":"; array(order.rank);
    std::cout << ",\"parent\":"; array(order.parent);
    std::cout << ",\"birth\":"; array(order.birth);
    std::cout << ",\"point_node\":"; array(order.point_node);
    std::cout << ",\"point_cat_rank\":"; array(order.point_cat_rank);
    std::cout << ",\"point_level\":"; array(order.point_level);
    std::cout << ",\"ball_node\":"; array(order.ball_node);
    std::cout << '}';
  }
  std::cout << "]}\n";
}
