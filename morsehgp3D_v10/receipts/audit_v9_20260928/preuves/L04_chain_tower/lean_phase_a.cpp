// Synthetic proxy (NOT the engine): lean sequential phase A of one order K,
// union-find by lots of equal rank, with the same action semantics as
// full_ball_tower.hpp order_lot (birth / continuation / multi-merge, parents
// = pre-lot roots, anchors installed after the lot). Sizes of ng00 K5:
// V = 789 886 blocks, E = 1 350 172 representatives, births ~ 43 %.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <random>
#include <vector>
using u32 = uint32_t; using u64 = uint64_t;
int main(int argc, char** argv) {
  const u32 V = argc > 1 ? atoi(argv[1]) : 789886;
  const double birth_frac = 0.43, grouped_frac = argc > 3 ? atof(argv[3]) : 0.10;
  const int mode = argc > 2 ? atoi(argv[2]) : 0;  // 0 uniform earlier target, 1 recent (window 4096)
  std::mt19937_64 rng(3);
  // blocks in rank order; rank increases except inside lots
  std::vector<u32> rank(V), deg(V), begin(V + 1);
  std::vector<u32> targets; targets.reserve(V * 3);
  u32 r = 1;
  std::uniform_real_distribution<double> U(0, 1);
  for (u32 b = 0; b < V; ++b) {
    if (b == 0 || U(rng) > grouped_frac) ++r;
    rank[b] = r;
  }
  // first block with each rank
  std::vector<u32> lot_first(V);
  for (u32 b = 0; b < V; ++b) lot_first[b] = (b && rank[b] == rank[b - 1]) ? lot_first[b - 1] : b;
  u64 E = 0;
  for (u32 b = 0; b < V; ++b) {
    begin[b] = targets.size();
    const u32 lf = lot_first[b];
    if (lf == 0 || U(rng) < birth_frac) continue;  // birth: no representative
    const int q = 2 + (int)(U(rng) * 3);  // 2..4 representatives
    for (int i = 0; i < q; ++i) {
      u32 t;
      if (mode == 0) t = (u32)(U(rng) * lf);
      else { u32 w = std::min<u32>(lf, 4096); t = lf - 1 - (u32)(U(rng) * w); }
      targets.push_back(t); ++E;
    }
  }
  begin[V] = targets.size();
  // ---- the lean phase A
  auto t0 = std::chrono::steady_clock::now();
  std::vector<u32> anchor(V, ~0u), uf;  // uf over nodes: parent pointers (path halving)
  std::vector<u32> node_parent_begin{0}, node_parents; std::vector<u32> node_rank;
  uf.reserve(V); node_rank.reserve(V); node_parents.reserve(E);
  auto find = [&](u32 x) { while (uf[x] != x) { uf[x] = uf[uf[x]]; x = uf[x]; } return x; };
  std::vector<u32> roots_buf, lot_roots_off, gid; std::vector<std::pair<u32,u32>> owner;
  u64 births = 0, merges = 0, conts = 0;
  for (u32 b = 0; b < V;) {
    u32 e = b + 1; while (e < V && rank[e] == rank[b]) ++e;
    if (e == b + 1) {  // singleton lot (fast path)
      roots_buf.clear();
      for (u32 i = begin[b]; i < begin[b + 1]; ++i) roots_buf.push_back(find(anchor[targets[i]]));
      std::sort(roots_buf.begin(), roots_buf.end());
      roots_buf.erase(std::unique(roots_buf.begin(), roots_buf.end()), roots_buf.end());
      u32 tgt;
      if (roots_buf.size() == 1) { tgt = roots_buf[0]; ++conts; }
      else {
        tgt = uf.size(); uf.push_back(tgt); node_rank.push_back(rank[b]);
        for (u32 p : roots_buf) { uf[p] = tgt; node_parents.push_back(p); }
        node_parent_begin.push_back(node_parents.size());
        roots_buf.empty() ? ++births : ++merges;
      }
      anchor[b] = tgt; b = e; continue;
    }
    // grouped lot: roots per block, group blocks sharing a root (small DSU)
    owner.clear(); gid.assign(e - b, 0); std::iota(gid.begin(), gid.end(), 0);
    auto gf = [&](u32 x) { while (gid[x] != x) { gid[x] = gid[gid[x]]; x = gid[x]; } return x; };
    for (u32 j = b; j < e; ++j)
      for (u32 i = begin[j]; i < begin[j + 1]; ++i) owner.push_back({find(anchor[targets[i]]), j - b});
    std::sort(owner.begin(), owner.end());
    for (size_t i = 1; i < owner.size(); ++i) if (owner[i].first == owner[i - 1].first) {
      u32 a = gf(owner[i].second), c = gf(owner[i - 1].second); if (a != c) gid[std::max(a, c)] = std::min(a, c);
    }
    // per group: parents = distinct roots
    std::vector<u32> tgt_of_group(e - b, ~0u);
    // collect roots per group in owner order (owner sorted by root)
    std::vector<std::vector<u32>> gp;  // rare path, allocations acceptable in proxy
    gp.resize(e - b);
    for (size_t i = 0; i < owner.size(); ++i) { u32 g = gf(owner[i].second); if (gp[g].empty() || gp[g].back() != owner[i].first) gp[g].push_back(owner[i].first); }
    for (u32 j = 0; j < e - b; ++j) {
      u32 g = gf(j); if (tgt_of_group[g] != ~0u) { anchor[b + j] = tgt_of_group[g]; continue; }
      auto& ps = gp[g]; std::sort(ps.begin(), ps.end()); ps.erase(std::unique(ps.begin(), ps.end()), ps.end());
      u32 tgt;
      if (ps.size() == 1) { tgt = ps[0]; ++conts; }
      else { tgt = uf.size(); uf.push_back(tgt); node_rank.push_back(rank[b]);
        for (u32 p : ps) { uf[p] = tgt; node_parents.push_back(p); }
        node_parent_begin.push_back(node_parents.size()); ps.empty() ? ++births : ++merges; }
      tgt_of_group[g] = tgt; anchor[b + j] = tgt;
    }
    b = e;
  }
  auto t1 = std::chrono::steady_clock::now();
  printf("V=%u E=%llu mode=%d nodes=%zu births=%llu merges=%llu cont_or_inert=%llu phaseA_ms=%.3f\n", V,
         (unsigned long long)E, mode, uf.size(), (unsigned long long)births, (unsigned long long)merges,
         (unsigned long long)conts, std::chrono::duration<double, std::milli>(t1 - t0).count());
}
