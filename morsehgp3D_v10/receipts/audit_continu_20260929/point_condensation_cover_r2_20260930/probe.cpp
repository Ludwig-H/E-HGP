// New head-only executions on a controlled translation of a HISTORICAL cover export.
#include "head/head.hpp"
#include "materialized.hpp"
#include <iomanip>
#include <iostream>
#include <vector>

void unsigneds(const std::vector<mhgp10::u32>& values) {
  std::cout << '[';
  for (std::size_t i=0;i<values.size();++i) {
    if (i) std::cout << ',';
    std::cout << values[i];
  }
  std::cout << ']';
}
int main() {
  using namespace mhgp10;
  auto d=historical_cover();
  const auto valid=validate(d);
  if (!valid.ok()) {
    std::cerr << "materialized API invalid: " << reason_name(valid.reason) << '\n';
    return 2;
  }
  std::cout << std::setprecision(17);
  unsigned rows=0;
  for (unsigned mcs : {1u,2u,6u}) for (unsigned z : {1u,2u}) for (bool single : {false,true}) {
    ClusterParams p;
    p.min_cluster_size=mcs;p.z=z;p.allow_single_cluster=single;
    auto out=cluster(d,p);
    std::cout << "{\"case\":\"historical_cover\",\"K\":3,\"point_ids\":[3,4,5,1,0,2],"
              << "\"api_valid\":true,\"mcs\":" << mcs << ",\"z\":" << z
              << ",\"allow_single\":" << (single?"true":"false") << ",\"selected\":";
    unsigneds(out.selected);
    std::cout << ",\"labels\":[";
    for (std::size_t i=0;i<out.label.size();++i) {
      if (i) std::cout << ',';
      std::cout << out.label[i];
    }
    std::cout << "],\"point_cluster\":";
    unsigneds(out.tree.point_cluster);
    std::cout << ",\"point_lambda\":[";
    for (std::size_t i=0;i<out.tree.point_lambda.size();++i) {
      if (i) std::cout << ',';
      std::cout << out.tree.point_lambda[i];
    }
    std::cout << "],\"clusters\":[";
    for (std::size_t c=0;c<out.tree.parent.size();++c) {
      if (c) std::cout << ',';
      std::cout << "{\"id\":" << c << ",\"parent\":";
      if (out.tree.parent[c]==kNone) std::cout << "null";
      else std::cout << out.tree.parent[c];
      std::cout << ",\"birth\":" << out.tree.birth[c] << ",\"mass\":" << out.tree.mass[c]
                << ",\"stability\":" << out.tree.stability[c] << '}';
    }
    std::cout << "]}\n";
    ++rows;
  }
  if (rows!=12) {std::cerr<<"row floor\n";return 3;}
  return 0;
}

