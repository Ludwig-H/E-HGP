// Isolated API witness: no geometry, cloud, catalogue, tower or GCP.
#include "head/head.hpp"
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {
struct Fixture {
  const char* name;
  std::vector<mhgp10::u32> rank, weight;
};

void numbers(const std::vector<mhgp10::u32>& values) {
  std::cout << '[';
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i) std::cout << ',';
    std::cout << values[i];
  }
  std::cout << ']';
}
}

int main() {
  using namespace mhgp10;
  const std::vector<Fixture> fixtures{{"under_root", {0, 1, 2, 3, 3, 5, 5}, {1, 1, 1, 1, 1, 1, 1}}};
  std::cout << std::setprecision(17);
  unsigned rows = 0;
  for (const auto& fixture : fixtures) {
    PointDendrogram d;
    d.level = {1, 4, 9, 16, 25, 100, 1600};
    d.node_rank = {0, 3, 4, 5, 6};
    d.child_off = {0, 0, 0, 2, 2, 4};
    d.child_val = {0, 1, 2, 3};
    d.parent = {2, 2, 4, 4, kNone};
    d.point_node = {0, 0, 0, 1, 1, 3, 3};
    d.point_rank = fixture.rank;
    d.point_weight = fixture.weight;
    const Outcome valid = validate(d);
    if (!valid.ok()) {
      std::cerr << "fixture invalid: " << fixture.name << ' ' << reason_name(valid.reason) << '\n';
      return 2;
    }
    for (unsigned mcs : {1u, 2u}) {
      for (unsigned z : {1u, 2u}) {
        ClusterParams p;
        p.min_cluster_size = mcs;
        p.z = z;
        p.allow_single_cluster = false;
        p.selection = Selection::eom;
        const auto out = cluster(d, p);
        std::cout << "{\"case\":\"" << fixture.name << "\",\"mcs\":" << mcs
                  << ",\"z\":" << z << ",\"allow_single\":false,\"api_valid\":true,\"selected\":";
        numbers(out.selected);
        std::cout << ",\"labels\":[";
        for (std::size_t x = 0; x < out.label.size(); ++x) {
          if (x) std::cout << ',';
          std::cout << out.label[x];
        }
        std::cout << "],\"point_cluster\":";
        numbers(out.tree.point_cluster);
        std::cout << ",\"point_lambda\":[";
        for (std::size_t x = 0; x < out.tree.point_lambda.size(); ++x) {
          if (x) std::cout << ',';
          std::cout << out.tree.point_lambda[x];
        }
        std::cout << "],\"clusters\":[";
        for (std::size_t c = 0; c < out.tree.parent.size(); ++c) {
          if (c) std::cout << ',';
          std::cout << "{\"id\":" << c << ",\"parent\":";
          if (out.tree.parent[c] == kNone) std::cout << "null";
          else std::cout << out.tree.parent[c];
          std::cout << ",\"birth\":" << out.tree.birth[c]
                    << ",\"mass\":" << out.tree.mass[c]
                    << ",\"stability\":" << out.tree.stability[c] << '}';
        }
        std::cout << "]}\n";
        ++rows;
      }
    }
  }
  if (rows != 4) {
    std::cerr << "nonvacuity row count\n";
    return 3;
  }
  return 0;
}

