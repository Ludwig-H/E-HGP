#include "points/dendrogram.hpp"

#include <algorithm>
#include <atomic>
#include <vector>

namespace mhgp10 {

namespace {

// Premier defaut d'une plage [b, e) (indice, raison) ; Reason::none si aucun.
struct Fault {
  u64 at = ~u64(0);
  Reason why = Reason::none;
};

}  // namespace

Outcome validate(const PointDendrogram& d, sched::Pool* pool) {
  if (pool != nullptr && pool->size() > 1 && !sched::in_parallel_region()) {
    const u32 n = d.nodes();
    MHGP10_CHECK(d.child_off.size() == u64(n) + 1 && d.child_off[0] == 0, csr_bounds);
    MHGP10_CHECK(d.child_off[n] == d.child_val.size() && d.parent.size() == n, csr_bounds);
    // meme ordre que la version serie : niveaux, puis noeuds (par indice), racine unique, puis points (par indice)
    const u64 P = pool->size();
    auto run = [&](u64 total, auto&& check) {  // premier defaut sur [0, total), tranches paralleles
      const u64 chunks = std::min<u64>(P * 8, std::max<u64>(1, total / 4096));
      std::vector<Fault> f(chunks);
      pool->parallel_for(chunks, 1, [&](u64 c0, u64 c1, unsigned) {
        for (u64 c = c0; c < c1; ++c)
          for (u64 i = total * c / chunks; i < total * (c + 1) / chunks; ++i) {
            const Reason r = check(i);
            if (r != Reason::none) {
              f[c] = Fault{i, r};
              break;
            }
          }
      });
      for (const Fault& x : f)
        if (x.why != Reason::none) return x;
      return Fault{};
    };
    const u64 nl = d.level.size();
    Fault f = run(nl > 0 ? nl - 1 : 0, [&](u64 i) { return d.level[i] < d.level[i + 1] ? Reason::none : Reason::rank_order; });
    if (f.why != Reason::none) return fail(f.why);
    std::atomic<u64> roots{0};
    f = run(n, [&](u64 v) {
      if (!(d.child_off[v] <= d.child_off[v + 1])) return Reason::csr_bounds;
      if (!(d.node_rank[v] < d.level.size())) return Reason::csr_bounds;
      for (u32 j = d.child_off[v]; j < d.child_off[v + 1]; ++j) {
        const u32 c = d.child_val[j];
        if (!(c < v)) return Reason::rank_order;
        if (!(d.parent[c] == v)) return Reason::csr_bounds;
        if (!(d.node_rank[c] <= d.node_rank[v])) return Reason::rank_order;
      }
      if (d.parent[v] == kNone) roots.fetch_add(1, std::memory_order_relaxed);
      return Reason::none;
    });
    if (f.why != Reason::none) return fail(f.why);
    MHGP10_CHECK(roots.load() == 1, root_count);
    MHGP10_CHECK(d.point_rank.size() == d.points() && d.point_weight.size() == d.points(), csr_bounds);
    f = run(d.points(), [&](u64 x) {
      const u32 v = d.point_node[x];
      if (!(v < n && d.point_weight[x] >= 1)) return Reason::csr_bounds;
      if (!(d.point_rank[x] >= d.node_rank[v])) return Reason::rank_order;
      if (d.parent[v] != kNone && !(d.point_rank[x] <= d.node_rank[d.parent[v]])) return Reason::rank_order;
      return Reason::none;
    });
    if (f.why != Reason::none) return fail(f.why);
    return Outcome{};
  }
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
