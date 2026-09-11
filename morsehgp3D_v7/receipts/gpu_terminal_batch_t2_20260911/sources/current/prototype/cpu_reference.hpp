#pragma once
#include <span>
#include "source/morsehgp3D_v7/src/pipeline/census.hpp"
namespace mhgp7::gpu_intruder_private {
struct CpuStats { u64 intruder_queries=0, intruder_nodes=0, interior_ranges=0, intruder_power_tests=0; };
struct CpuReference {
  const CloudIndex& ix;
  using FullBallStats=CpuStats;
  static void add(u64& value) { ++value; }
  i32 intruder_work(const BallKey& key, std::span<const i32> selected,
      FullBallStats& work, std::vector<NodeRef>& scratch) const {
    add(work.intruder_queries);
    const auto member = [&](i32 u) { return std::binary_search(selected.begin(), selected.end(), u); };
    const census_detail::AxisBounds bounds(key);
    scratch.clear(); scratch.push_back(ix.root());
    while (!scratch.empty()) {
      const auto node = scratch.back(); scratch.pop_back(); add(work.intruder_nodes);
      i128 lo, hi; bounds.bounds(ix.box_of(node), &lo, &hi);
      if (lo >= 0) continue;
      if (hi < 0) {
        add(work.interior_ranges);
        const auto range = ix.range_of(node);
        for (i32 u = range.first; u <= range.last; ++u) if (!member(u)) return u;
      } else if (is_leaf(node)) {
        const i32 u = leaf_index(node);
        if (!member(u)) {
          add(work.intruder_power_tests);
          if (key.power(ix.upos[u]) < 0) return u;
        }
      } else {
        scratch.push_back(ix.nodes[node].right); scratch.push_back(ix.nodes[node].left);
      }
    }
    return -1;
  }
};
}
