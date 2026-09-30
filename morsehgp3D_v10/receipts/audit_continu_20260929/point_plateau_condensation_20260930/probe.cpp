#include "head/head.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

template<class T> void arr(const std::vector<T>& x) {
  std::cout << '[';
  for (std::size_t i=0; i<x.size(); ++i) { if(i) std::cout << ','; std::cout << x[i]; }
  std::cout << ']';
}

mhgp10::PointDendrogram fixture(bool flat) {
  using namespace mhgp10;
  PointDendrogram d;
  d.level={1,25};
  if(flat) {
    d.node_rank={0,0,0,1}; d.parent={3,3,3,kNone};
    d.child_off={0,0,0,0,3}; d.child_val={0,1,2};
  } else {
    d.node_rank={0,0,0,1,1}; d.parent={3,3,4,4,kNone};
    d.child_off={0,0,0,0,2,4}; d.child_val={0,1,3,2};
  }
  d.point_node={0,0,0,1,1,1,2,2,2,2,2,2};
  d.point_rank.assign(12,0); d.point_weight.assign(12,1);
  return d;
}

mhgp10::u32 lca(const mhgp10::PointDendrogram& d, mhgp10::u32 u, mhgp10::u32 v) {
  while(u!=v) { if(u<v) u=d.parent[u]; else v=d.parent[v]; }
  return u;
}

int main() {
  using namespace mhgp10;
  std::cout << std::setprecision(17);
  unsigned rows=0;
  for(bool flat : {false,true}) {
    const auto d=fixture(flat);
    const auto valid=validate(d);
    if(!valid.ok()) { std::cerr << "invalid " << reason_name(valid.reason) << '\n'; return 2; }
    for(unsigned z : {1u,2u}) {
      ClusterParams p; p.min_cluster_size=5; p.z=z;
      p.selection=Selection::eom; p.allow_single_cluster=false;
      const auto out=cluster(d,p);
      std::cout << "{\"case\":\"" << (flat?"flat":"factored") << "\",\"z\":" << z
                << ",\"mcs\":5,\"allow_single\":false,\"api_valid\":true,\"node_rank\":";
      arr(d.node_rank); std::cout << ",\"parent\":"; arr(d.parent);
      std::cout << ",\"child_off\":"; arr(d.child_off);
      std::cout << ",\"child_val\":"; arr(d.child_val);
      std::cout << ",\"point_node\":"; arr(d.point_node);
      std::cout << ",\"point_rank\":"; arr(d.point_rank);
      std::cout << ",\"point_weight\":"; arr(d.point_weight);
      std::cout << ",\"matrix_beta\":[";
      for(u32 i=0;i<d.points();++i) {
        if(i) std::cout << ',';
        std::cout << '[';
        for(u32 j=0;j<d.points();++j) {
          if(j) std::cout << ',';
          if(i==j) std::cout << 0;
          else {
            const u32 w=lca(d,d.point_node[i],d.point_node[j]);
            const u32 rank=std::max({d.point_rank[i],d.point_rank[j],d.node_rank[w]});
            std::cout << d.level[rank];
          }
        }
        std::cout << ']';
      }
      std::cout << "],\"selected\":"; arr(out.selected);
      std::cout << ",\"labels\":"; arr(out.label);
      std::cout << ",\"point_cluster\":"; arr(out.tree.point_cluster);
      std::cout << ",\"point_lambda\":"; arr(out.tree.point_lambda);
      std::cout << ",\"clusters\":[";
      for(u32 c=0;c<out.tree.parent.size();++c) {
        if(c) std::cout << ',';
        std::cout << "{\"id\":" << c << ",\"parent\":";
        if(out.tree.parent[c]==kNone) std::cout << "null"; else std::cout << out.tree.parent[c];
        std::cout << ",\"birth\":" << out.tree.birth[c] << ",\"mass\":" << out.tree.mass[c]
                  << ",\"stability\":" << out.tree.stability[c] << '}';
      }
      std::cout << "]}\n"; ++rows;
    }
  }
  if(rows!=4) { std::cerr << "rows\n"; return 3; }
}
