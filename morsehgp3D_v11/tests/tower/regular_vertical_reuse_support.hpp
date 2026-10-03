// Harnais : les images et le travail structurel sont fixes, pas les terminaux ni le travail des memos.
#pragma once
#include "census_reuse_support.hpp"
#include "tower/regular_vertical_seeds.hpp"

namespace regular_vertical_test {
using namespace census_reuse_test;
inline u64 retained_bytes(const OrderForest& f) {
  return f.node_capacity() * sizeof(ForestNode) + f.edge_capacity() * sizeof(NodeIdx) +
         f.lookup_reserved_bytes() + (f.order() == 1 ? 0 : f.node_capacity() * sizeof(NodeIdx));
}
inline ForestLedger fixed_work(ForestLedger value) {
  value.descent = {};
  value.vertical_descents = value.vertical_reuses = value.ancestor_find_steps = 0;
  return value;
}
inline u64 regular_births(const FullDomain& domain, const OrderForest& forest) {
  if (forest.order() == 1) return 0;
  u64 count = 0;
  for (u32 i = 0; i < forest.births(); ++i) {
    const auto& b = domain.catalogue().balls_data()[forest.nodes()[i].birth_key];
    count += b.m == b.qmin;
  }
  return count;
}
inline BallIdx ball_at(const FullDomain& domain, u32 p, u32 m, u8 q, i64 n, i64 d) {
  const auto balls = domain.catalogue().balls_data();
  for (u32 i = 0; i < balls.size(); ++i) {
    const auto& b = balls[i];
    if (b.p == p && b.m == m && b.qmin == q && level_is(domain.catalogue().levels()[idx(b.rank)],n,d))
      return BallIdx{i};
  }
  throw std::runtime_error("boule analytique de reemploi absente");
}
inline NodeIdx birth_at(const OrderForest& forest, BallIdx ball) {
  for (u32 i = 0; i < forest.births(); ++i)
    if (forest.nodes()[i].birth_key == idx(ball)) return NodeIdx{i};
  throw std::runtime_error("naissance analytique de reemploi absente");
}
inline FullParams reuse_params(u32 q, unsigned flags) {
  FullParams p;
  p.memo_capacity = (flags & 1u) != 0 ? 8 : 0;
  p.regular_batch_capacity = q; p.descent_lanes = q == 0 ? 1 : 4;
  p.lane_memo_capacity = q == 0 ? 0 : p.memo_capacity;
  p.parallel_verticals = q > 1;
  p.reuse_census_workspace = (flags & 2u) != 0;
  p.dense_birth_lookup = (flags & 4u) != 0;
  p.reuse_regular_verticals = true;
  return p;
}
}  // namespace regular_vertical_test
