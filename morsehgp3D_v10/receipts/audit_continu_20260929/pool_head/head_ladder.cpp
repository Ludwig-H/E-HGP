#include <chrono>
#include <cstdio>
#include "head/head.hpp"

int main() {
  using namespace mhgp10;
  for (u32 n : {2000u, 4000u, 8000u}) {
    PointDendrogram d;
    const u32 nn = 2*n-1;
    d.level.resize(n);
    for (u32 r=0;r<n;++r) d.level[r]=r+1;
    d.node_rank.assign(nn,0);
    d.parent.assign(nn,kNone);
    d.child_off.assign(nn+1,0);
    d.point_node.resize(n);
    d.point_rank.assign(n,0);
    d.point_weight.assign(n,1);
    for(u32 x=0;x<n;++x) d.point_node[x]=x;
    for(u32 v=n;v<nn;++v) {
      u32 a = v==n ? 0 : v-1;
      u32 b = v-n+1;
      d.node_rank[v]=v-n+1;
      d.parent[a]=v;
      d.parent[b]=v;
      d.child_val.push_back(a);
      d.child_val.push_back(b);
      d.child_off[v+1]=d.child_val.size();
    }
    if(!validate(d).ok()) return 2;
    ClusterParams p;
    p.min_cluster_size=1;
    p.selection=Selection::leaf;
    double best=1e9;
    u64 unselected_internal_ancestor_steps=(u64(n-2)*(n-1))/2;
    for(int rep=0;rep<3;++rep) {
      const auto start=std::chrono::steady_clock::now();
      auto result=cluster(d,p);
      const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
      best=std::min(best,elapsed);
      if(result.selected.size()!=n) return 3;
    }
    std::printf("n=%u internal_ancestor_steps=%llu best_s=%.9f\n",n,
                static_cast<unsigned long long>(unselected_internal_ancestor_steps),best);
  }
}
