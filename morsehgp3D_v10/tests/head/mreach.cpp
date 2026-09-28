#include "mreach.hpp"

#include <algorithm>
#include <numeric>

namespace mhgp10 {

std::vector<u64> core_distances2(const SiteTree& tree, u64 k, sched::Pool& pool) {
  const Cloud& c = tree.cloud();
  std::vector<u64> core(c.sites());
  pool.parallel_for(c.sites(), 256, [&](u64 b, u64 e, unsigned) {
    for (u64 s = b; s < e; ++s) core[s] = tree.kth_distance(c.x[s], c.y[s], c.z[s], k);
  });
  return core;
}

namespace {

struct Edge {
  u64 w;
  u32 a, b;
};

// Prim exact O(n^2) : cle (poids, sommet) pour un choix deterministe.
std::vector<Edge> prim_mst(const Cloud& c, const std::vector<u64>& core, sched::Pool& pool) {
  const u32 n = c.sites();
  std::vector<Edge> mst;
  if (n <= 1) return mst;
  mst.reserve(n - 1);
  std::vector<u64> best(n, ~u64{0});
  std::vector<u32> from(n, kNone);
  std::vector<unsigned char> done(n, 0);
  const unsigned T = pool.size();
  std::vector<std::pair<u64, u32>> local(T);
  u32 cur = 0;
  done[0] = 1;
  for (u32 it = 1; it < n; ++it) {
    const i64 cx = c.x[cur], cy = c.y[cur], cz = c.z[cur];
    const u64 cc = core[cur];
    for (auto& l : local) l = {~u64{0}, kNone};
    pool.parallel_for(n, 2048, [&](u64 b, u64 e, unsigned t) {
      std::pair<u64, u32> mine = local[t];
      for (u64 v = b; v < e; ++v) {
        if (done[v]) continue;
        const i64 dx = i64(c.x[v]) - cx, dy = i64(c.y[v]) - cy, dz = i64(c.z[v]) - cz;
        u64 m = static_cast<u64>(dx * dx + dy * dy + dz * dz);
        m = std::max(m, std::max(cc, core[v]));
        if (m < best[v] || (m == best[v] && cur < from[v])) {
          best[v] = m;
          from[v] = cur;
        }
        if (best[v] < mine.first || (best[v] == mine.first && v < mine.second)) mine = {best[v], u32(v)};
      }
      local[t] = mine;
    });
    std::pair<u64, u32> pick{~u64{0}, kNone};
    for (auto& l : local)
      if (l.first < pick.first || (l.first == pick.first && l.second < pick.second)) pick = l;
    const u32 v = pick.second;
    done[v] = 1;
    mst.push_back({best[v], std::min(v, from[v]), std::max(v, from[v])});
    cur = v;
  }
  return mst;
}

u32 find(std::vector<u32>& p, u32 x) {
  while (p[x] != x) {
    p[x] = p[p[x]];
    x = p[x];
  }
  return x;
}

}  // namespace

PointDendrogram mreach_dendrogram(const SiteTree& tree, u64 k, sched::Pool& pool) {
  const Cloud& c = tree.cloud();
  const u32 n = c.sites();
  const std::vector<u64> core = core_distances2(tree, k, pool);
  std::vector<Edge> mst = prim_mst(c, core, pool);
  std::sort(mst.begin(), mst.end(), [](const Edge& x, const Edge& y) {
    if (x.w != y.w) return x.w < y.w;
    if (x.a != y.a) return x.a < y.a;
    return x.b < y.b;
  });
  // Table des niveaux : valeurs entieres exactes distinctes.
  std::vector<u64> vals(core.begin(), core.end());
  for (const Edge& e : mst) vals.push_back(e.w);
  std::sort(vals.begin(), vals.end());
  vals.erase(std::unique(vals.begin(), vals.end()), vals.end());
  auto rank_of = [&](u64 v) { return static_cast<u32>(std::lower_bound(vals.begin(), vals.end(), v) - vals.begin()); };

  PointDendrogram d;
  d.level.assign(vals.begin(), vals.end());
  d.node_rank.resize(n);
  d.parent.assign(n, kNone);
  d.child_off.assign(n + 1, 0);
  d.point_node.resize(n);
  d.point_rank.resize(n);
  d.point_weight.resize(n);
  for (u32 s = 0; s < n; ++s) {
    d.node_rank[s] = rank_of(core[s]);
    d.point_node[s] = s;
    d.point_rank[s] = d.node_rank[s];
    d.point_weight[s] = c.w[s];
  }
  // Kruskal par plateaux : dsu sur les sites, racine courante -> noeud.
  std::vector<u32> dsu(n), node_of(n);
  std::iota(dsu.begin(), dsu.end(), 0u);
  std::iota(node_of.begin(), node_of.end(), 0u);
  std::vector<u32> group;  // racines touchees dans le plateau
  for (u64 i = 0; i < mst.size();) {
    u64 j = i;
    while (j < mst.size() && mst[j].w == mst[i].w) ++j;
    const u32 rk = rank_of(mst[i].w);
    // anciennes racines (avant le plateau) de chaque extremite
    std::vector<std::pair<u32, u32>> pairs;
    pairs.reserve(j - i);
    for (u64 t = i; t < j; ++t) pairs.push_back({find(dsu, mst[t].a), find(dsu, mst[t].b)});
    // union dans le plateau
    for (auto& [ra, rb] : pairs) {
      const u32 x = find(dsu, ra), y = find(dsu, rb);
      if (x != y) dsu[std::max(x, y)] = std::min(x, y);
    }
    // regrouper les anciennes racines par nouvelle racine
    std::vector<std::pair<u32, u32>> members;  // (nouvelle racine, ancienne racine)
    for (auto& [ra, rb] : pairs) {
      members.push_back({find(dsu, ra), ra});
      members.push_back({find(dsu, rb), rb});
    }
    std::sort(members.begin(), members.end());
    members.erase(std::unique(members.begin(), members.end()), members.end());
    for (u64 a = 0; a < members.size();) {
      u64 b = a;
      while (b < members.size() && members[b].first == members[a].first) ++b;
      const u32 node = d.nodes();
      d.node_rank.push_back(rk);
      d.parent.push_back(kNone);
      std::vector<u32> kids;
      for (u64 t = a; t < b; ++t) kids.push_back(node_of[members[t].second]);
      std::sort(kids.begin(), kids.end());
      for (u32 kid : kids) {
        d.child_val.push_back(kid);
        d.parent[kid] = node;
      }
      d.child_off.push_back(static_cast<u32>(d.child_val.size()));
      node_of[members[a].first] = node;
      a = b;
    }
    i = j;
  }
  return d;
}

}  // namespace mhgp10
