// Aides des portes des etages T, M, V et R (hors produit) : entrees synthetiques possedees, nuage sur une droite,
// foret de reference par Kruskal par lots (semantique de la v10 : racines lues avant les unions du plateau, une fusion
// N-aire par groupe ; port de kruskal_lots de MES-M4 et de preuves_tour/foret_check.py), comparaison champ par champ.
#pragma once

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <numeric>
#include <vector>

#include "sched/sched.hpp"
#include "test.hpp"
#include "tower/tower.hpp"

namespace mhgp12::tower_test {

using tower::ForestInput;
using tower::OrderForest;
using tower::TowerForests;

inline std::vector<LevelRank> ranks(std::initializer_list<u32> values) {
  std::vector<LevelRank> out;
  for (u32 v : values) out.push_back(make_id<LevelRank>(v));
  return out;
}

// Entree possedee d'un ordre.
struct OrderData {
  Order k = 1;
  std::vector<u32> birth_key, targets;
  std::vector<LevelRank> birth_rank, cell_rank;
  std::vector<BallIdx> cell_ball;
  std::vector<u64> rep_offsets{0};

  ForestInput view() const;
  // Naissances 0 .. n - 1 de rang `rank` (cles = indices).
  void births(u32 n, u32 rank = 0) {
    for (u32 i = 0; i < n; ++i) {
      birth_key.push_back(static_cast<u32>(birth_key.size()));
      birth_rank.push_back(make_id<LevelRank>(rank));
    }
  }
  // Cellule de boule `ball` (par defaut : la suivante) et de rang `rank`, cibles donnees.
  void cell(u32 rank, std::vector<u32> cell_targets, u32 ball = kNone) {
    cell_ball.push_back(make_id<BallIdx>(ball != kNone ? ball : static_cast<u32>(cell_ball.size())));
    cell_rank.push_back(make_id<LevelRank>(rank));
    targets.insert(targets.end(), cell_targets.begin(), cell_targets.end());
    rep_offsets.push_back(targets.size());
  }
};

// Vue aux accesseurs de ResolvedOrder de l'etage G (INTERFACE.md de G, paragraphe 3) : toutes les portes passent par
// l'adaptateur tower::forest_input, celui de la chaine du produit.
struct ResolvedView {
  const OrderData& d;
  Order order() const noexcept { return d.k; }
  std::span<const u32> birth_keys() const noexcept { return d.birth_key; }
  std::span<const LevelRank> birth_ranks() const noexcept { return d.birth_rank; }
  std::span<const BallIdx> cell_balls() const noexcept { return d.cell_ball; }
  std::span<const LevelRank> cell_ranks() const noexcept { return d.cell_rank; }
  std::span<const u64> cell_offsets() const noexcept { return d.rep_offsets; }
  std::span<const u32> targets() const noexcept { return d.targets; }
};
inline ForestInput OrderData::view() const { return tower::forest_input(ResolvedView{*this}); }

// Nuage de n sites (i, 0, 0) : ordre de Morton et ordre (x, y, z) sont l'ordre des indices.
inline Cloud line_cloud(u32 n, MemoryBudget& budget) {
  std::vector<u32> x(n), y(n, 0), z(n, 0);
  std::vector<PointId> ids(n);
  for (u32 i = 0; i < n; ++i) {
    x[i] = i;
    ids[i] = make_id<PointId>(i);
  }
  auto cloud = prepare_cloud(x, y, z, ids, CoordWidth(), budget);
  if (!cloud.ok()) std::terminate();
  return std::move(cloud).take();
}

inline std::unique_ptr<sched::Pool> pool_of(u32 workers) {
  auto made = sched::make_pool(sched::PoolParams{workers});
  if (!made.ok()) std::terminate();
  return std::move(made).take();
}

// Foret de reference d'un ordre a naissances de rang nul ou donne, feuilles = indices d'entree (ordre canonique de
// line_cloud a k = 1). Une cible << cellule >> est remplacee par la feuille de la premiere cible de cette cellule
// (meme composante a la coupe ouverte : la cellule cible est de rang strictement inferieur).
struct Reference {
  std::vector<u32> rank, parent;
  std::vector<std::vector<u32>> kids;
};

inline u32 reference_leaf(const OrderData& d, u32 target) {
  while (target_is_cell(target)) target = d.targets[d.rep_offsets[target_index(target)]];
  return target;
}

inline Reference kruskal_lots(const OrderData& d) {
  const u32 nb = static_cast<u32>(d.birth_key.size());
  Reference out;
  for (const LevelRank r : d.birth_rank) out.rank.push_back(idx(r));
  out.parent.assign(nb, kNone);
  out.kids.assign(nb, {});
  std::vector<u32> dsu(nb), top(nb);
  std::iota(dsu.begin(), dsu.end(), 0u);
  std::iota(top.begin(), top.end(), 0u);
  auto find = [&dsu](u32 x) {
    while (dsu[x] != x) x = dsu[x] = dsu[dsu[x]];
    return x;
  };
  const u64 nc = d.cell_ball.size();
  for (u64 i = 0; i < nc;) {
    const u32 rk = idx(d.cell_rank[i]);
    u64 j = i;
    while (j < nc && idx(d.cell_rank[j]) == rk) ++j;
    std::vector<std::vector<u32>> pre;
    for (u64 t = i; t < j; ++t) {
      std::vector<u32> roots;
      for (u64 r = d.rep_offsets[t]; r < d.rep_offsets[t + 1]; ++r) roots.push_back(find(reference_leaf(d, d.targets[r])));
      pre.push_back(roots);
    }
    for (const auto& roots : pre)
      for (std::size_t r = 1; r < roots.size(); ++r) {
        const u32 x = find(roots[0]), y = find(roots[r]);
        if (x != y) dsu[std::max(x, y)] = std::min(x, y);
      }
    std::vector<std::pair<u32, u32>> members;
    for (const auto& roots : pre)
      for (u32 r : roots) members.push_back({find(r), r});
    std::sort(members.begin(), members.end());
    members.erase(std::unique(members.begin(), members.end()), members.end());
    for (std::size_t a = 0; a < members.size();) {
      std::size_t b = a;
      while (b < members.size() && members[b].first == members[a].first) ++b;
      if (b - a >= 2) {
        const u32 node = static_cast<u32>(out.rank.size());
        out.rank.push_back(rk);
        out.parent.push_back(kNone);
        std::vector<u32> children;
        for (std::size_t t = a; t < b; ++t) children.push_back(top[members[t].second]);
        std::sort(children.begin(), children.end());
        for (u32 ch : children) out.parent[ch] = node;
        out.kids.push_back(children);
        top[members[a].first] = node;
      }
      a = b;
    }
    i = j;
  }
  return out;
}

// Egalite champ par champ du registre d'un ordre et de la reference.
inline bool same_forest(const OrderForest& f, const Reference& ref) {
  if (f.nodes() != ref.rank.size()) return false;
  for (u32 v = 0; v < f.nodes(); ++v) {
    if (f.rank[v] != ref.rank[v] || f.parent[v] != ref.parent[v]) return false;
    const u64 b = f.children.off[v], e = f.children.off[u64{v} + 1];
    if (e - b != ref.kids[v].size() || !std::equal(ref.kids[v].begin(), ref.kids[v].end(), f.children.val.data() + b))
      return false;
  }
  return true;
}

// Egalite de deux registres (tous les tableaux publies).
template <class T>
bool same_buffer(const Buffer<T>& a, const Buffer<T>& b) {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}
inline bool same_registry(const OrderForest& a, const OrderForest& b) {
  return a.k == b.k && a.births == b.births && a.root == b.root && same_buffer(a.birth_key, b.birth_key) &&
         same_buffer(a.birth_node, b.birth_node) && same_buffer(a.rank, b.rank) && same_buffer(a.parent, b.parent) &&
         same_buffer(a.minleaf, b.minleaf) && same_buffer(a.children.off, b.children.off) &&
         same_buffer(a.children.val, b.children.val) && same_buffer(a.lower, b.lower) &&
         same_buffer(a.cell_node, b.cell_node) && same_buffer(a.event_cell, b.event_cell) &&
         same_buffer(a.retained_cell, b.retained_cell) && same_buffer(a.retained_ball, b.retained_ball) &&
         same_buffer(a.retained_rank, b.retained_rank) && same_buffer(a.branches.off, b.branches.off) &&
         same_buffer(a.branches.val, b.branches.val);
}

}  // namespace mhgp12::tower_test
