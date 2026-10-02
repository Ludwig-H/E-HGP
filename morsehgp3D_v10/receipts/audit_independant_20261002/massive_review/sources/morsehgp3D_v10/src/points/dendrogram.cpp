#include "points/dendrogram.hpp"

namespace mhgp10 {

Outcome validate(const PointDendrogram& d) {
  const u32 n = d.nodes();
  MHGP10_CHECK(d.child_off.size() == u64(n) + 1 && d.child_off[0] == 0, csr_bounds);
  MHGP10_CHECK(d.child_off[n] == d.child_val.size() && d.parent.size() == n, csr_bounds);
  for (u64 i = 1; i < d.level.size(); ++i) MHGP10_CHECK(d.level[i - 1] < d.level[i], rank_order);
  u32 roots = 0;
  for (u32 v = 0; v < n; ++v) {
    MHGP10_CHECK(d.child_off[v] <= d.child_off[v + 1], csr_bounds);
    MHGP10_CHECK(d.node_rank[v] < d.level.size(), csr_bounds);
    for (u32 j = d.child_off[v]; j < d.child_off[v + 1]; ++j) {
      const u32 c = d.child_val[j];
      MHGP10_CHECK(c < v, rank_order);  // enfants crees avant
      MHGP10_CHECK(d.parent[c] == v, csr_bounds);
      MHGP10_CHECK(d.node_rank[c] <= d.node_rank[v], rank_order);
    }
    if (d.parent[v] == kNone) ++roots;
  }
  MHGP10_CHECK(roots == 1, root_count);
  MHGP10_CHECK(d.point_rank.size() == d.points() && d.point_weight.size() == d.points(), csr_bounds);
  for (u32 x = 0; x < d.points(); ++x) {
    const u32 v = d.point_node[x];
    MHGP10_CHECK(v < n && d.point_weight[x] >= 1, csr_bounds);
    MHGP10_CHECK(d.point_rank[x] >= d.node_rank[v], rank_order);
    if (d.parent[v] != kNone) MHGP10_CHECK(d.point_rank[x] <= d.node_rank[d.parent[v]], rank_order);
  }
  return Outcome{};
}

}  // namespace mhgp10
