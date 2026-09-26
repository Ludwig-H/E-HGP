#pragma once

// Exact, portable cache for independent S2 tiles. One representative
// produces a FixedWitnessTrace via filter<true,true>. Other pairs may test
// that immutable trace concurrently. A cache result contains DEAD lanes
// only; surviving lanes restart filter<true> from its root and ZERO count.
// Never reuse the representative's rejected mask without these re-tests.
#include "witness_filter.hpp"

namespace mhgp9::gpu {

// Structural boundary for externally supplied traces. The node array is
// already a validated immutable index. An internal trace from the producer
// above has this property by construction and need not pay O(K^2) per pair.
// Rank intervals, not coordinate boxes, decide disjointness: witness boxes
// can touch while their site sets are disjoint. Different lanes may overlap.
MHGP9_HD inline bool validate_witness_trace(const FlatNode* nodes, u32 node_count,
                                           const FixedWitnessTrace& trace) {
  if (trace.size > witness_trace_capacity || (node_count != 0 && nodes == nullptr)) return false;
  for (unsigned i = 0; i < trace.size; ++i) {
    if (trace.nodes[i] >= node_count || trace.lanes[i] == 0 || (trace.lanes[i] & ~6U) != 0) return false;
    const FlatNode& a = nodes[trace.nodes[i]];
    if (a.first >= a.last) return false;
    for (unsigned j = 0; j < i; ++j) {
      if ((trace.lanes[i] & trace.lanes[j]) == 0) continue;
      const FlatNode& b = nodes[trace.nodes[j]];
      if (a.first < b.last && b.first < a.last) return false;
    }
  }
  return true;
}

// Internal certified-trace entry. Preconditions are those of filter<true>
// (u18 singleton boxes, K1..10, mask subset of 6, same validated index),
// plus a producer trace or successful validate_witness_trace. The cache
// retests strict node admission for the NEW endpoints. In particular
// Hmin>0 excludes either new endpoint automatically. Bounds/products are
// the same exact i64/i128 expressions as the historical search (<2^80).
// node_tests counts geometric node tests only, not trace validation.
MHGP9_HD inline u8 cached_witness_rejections(const FlatNode* nodes, const FlatBox& a, const FlatBox& b,
                                            unsigned kmax, u8 lane_mask, const FixedWitnessTrace& trace,
                                            std::uint64_t& node_tests) {
  const u8 available = kmax >= 3 ? 6 : kmax == 2 ? 2 : 0;
  const u8 lanes = static_cast<u8>(lane_mask & available);
  if (lanes == 0 || trace.size == 0) return 0;
  const PairBounds pair = prepare_pair(a.low, b.low);
  const unsigned threshold[2] = {kmax - 1, kmax >= 3 ? kmax - 2 : 0U};
  unsigned count[2] = {0, 0};
  for (unsigned i = 0; i < trace.size; ++i) {
    const u8 open = static_cast<u8>(trace.lanes[i] & lanes);
    if (open == 0) continue;
    const FlatNode& node = nodes[trace.nodes[i]];
    ++node_tests;
    const Bounds4 h = pair_h(pair, node.box);
    if (h.minimum4 <= 0) continue;
    const Xi xi = pair_xi(pair, node.box);
    const i128 h_squared = static_cast<i128>(h.minimum4) * h.minimum4;
    const i128 xi16 = static_cast<i128>(16) * xi.high;
    for (unsigned lane = 0; lane < 2; ++lane) {
      const u8 bit = static_cast<u8>(2U << lane);
      if ((open & bit) == 0 || count[lane] == threshold[lane]) continue;
      const i128 alpha = lane == 0 ? 3 : 2;
      if (alpha * h_squared <= xi16) continue;
      const unsigned room = threshold[lane] - count[lane];
      const u32 population = node.last - node.first;
      count[lane] += room < population ? room : population;
    }
  }
  u8 rejected = 0;
  if ((lanes & 2U) != 0 && count[0] == threshold[0]) rejected = static_cast<u8>(rejected | 2U);
  if ((lanes & 4U) != 0 && count[1] == threshold[1]) rejected = static_cast<u8>(rejected | 4U);
  return rejected;
}

}  // namespace mhgp9::gpu
