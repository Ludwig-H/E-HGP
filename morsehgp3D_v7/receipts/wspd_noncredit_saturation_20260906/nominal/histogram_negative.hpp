// Private extension of histogram_blocks.hpp 1da88985, which stays unchanged.
// Mathematical authority: audits/DIALOGUE_COURANT.md, non-credit section,
// SHA912c54c3914dc84170ca6ee5ee3e8abb5b07047afc1859cb6ba053c253d25a0f.
// Reject only histogram CONTRIBUTIONS, never an anchor or a rectangle.
#pragma once
#include <limits>
#include "../v7_wspd_histogram_blocks_20260906/histogram_blocks.hpp"

namespace mhgp7::histogram_negative_private {
namespace positive = histogram_blocks_private;
struct Work {
  positive::Work base;
  u64 negative_tests = 0, ximin_tests = 0;
  u64 nonpositive_m4_nodes = 0, angular_rejected_nodes = 0;
  u64 negative_rejected_pairs = 0;
  u64 saturated_anchors = 0, saturation_unvisited_pairs = 0;
  u64 clipped_credit_blocks = 0;
};

// Linear cross-product intervals at FIXED integer a,b. Coordinate differences
// and interval endpoints fit i64 under u16. Squaring is performed in i128.
inline i128 cross_lower(const P3& a, const P3& b, const AxisBox& z) {
  const std::array<i64, 3> av{a.x, a.y, a.z}, d{b.x - a.x, b.y - a.y, b.z - a.z};
  std::array<positive::Interval, 3> v{};
  for (unsigned i = 0; i < 3; ++i) v[i] = {z.lo[i] - av[i], z.hi[i] - av[i]};
  i128 sum = 0;
  for (unsigned i = 0; i < 3; ++i) {
    const unsigned j = (i + 1) % 3, k = (i + 2) % 3;
    const auto left = positive::product({d[j], d[j]}, v[k]);
    const auto right = positive::product({d[k], d[k]}, v[j]);
    const i64 lo = left.lo - right.hi, hi = left.hi - right.lo;
    const i128 distance = lo > 0 ? lo : hi < 0 ? -hi : 0;
    sum += distance * distance;
  }
  return sum;
}

inline bool rejects_fixed_pair(Lane lane, const P3& a, const P3& b0,
                               const AxisBox& z, Work& work) {
  if (lane == Lane::kQ2) return false;  // This delta leaves the q2 path alone.
  ++work.negative_tests;
  const i64 m4 = hmax4_boxes(positive::point_box(a), positive::point_box(b0), z);
  if (m4 <= 0) { ++work.nonpositive_m4_nodes; return true; }
  ++work.ximin_tests;
  const i128 m = m4;  // BEFORE square: t*M4^2 may exceed u64 (but <2^73).
  if ((lane == Lane::kQ3 ? 3 : 2) * m * m <= 16 * cross_lower(a, b0, z)) {
    ++work.angular_rejected_nodes;
    return true;
  }
  return false;
}

// Integer floor-centre lies in the opposite box. It need not be a real site:
// passing every box corner implies passing this point by separate convexity.
// Failing this point refutes ONLY the universal-over-corners contribution.
inline P3 box_representative(const AxisBox& box) {
  return {(box.lo[0] + box.hi[0]) / 2, (box.lo[1] + box.hi[1]) / 2,
          (box.lo[2] + box.hi[2]) / 2};
}

// Literal private traversal port; sole extra decision after failed positive
// credit is the fixed-b0 non-credit certificate. MAX keeps exact full counts;
// finite need clips successes only. No operation quota is introduced.
inline u64 fixed_anchor(const CloudIndex& ix, Lane lane, NodeRef factor,
                        const AxisBox& opposite, const P3& b0, i32 anchor, Work& work,
                        u64 need = std::numeric_limits<u64>::max()) {
  const P3& a = ix.upos[static_cast<std::size_t>(anchor)];
  const AxisBox singleton = positive::point_box(a);
  InlineStack<NodeRef, 64> stack;
  stack.push_back(factor);
  u64 result = 0;
  const auto factor_range = ix.range_of(factor);
  u64 remaining = static_cast<u64>(factor_range.last - factor_range.first);
  while (!stack.empty() && result < need) {
    const NodeRef node = stack.back(); stack.pop_back();
    ++work.base.node_visits;
    if (is_leaf(node)) {
      const i32 z = leaf_index(node);
      if (z == anchor) { ++work.base.diagonal_leaves; continue; }
      --remaining;
      ++work.base.scalar_tests;
      if (universal_over_corners(lane, a, opposite, ix.upos[static_cast<std::size_t>(z)])) {
        ++result; ++work.base.scalar_true;
      }
      continue;
    }
    const NodeRange range = ix.range_of(node);
    const bool diagonal = range.first <= anchor && anchor <= range.last;
    const u64 positions = static_cast<u64>(range.last - range.first + 1);
    const AxisBox box = ix.box_of(node);
    ++work.base.hmax_tests;
    if (hmax4_boxes(singleton, opposite, box) <= 0) {
      ++work.base.hmax_rejected_nodes;
      work.base.hmax_rejected_pairs += positions - (diagonal ? 1u : 0u);
      remaining -= positions - (diagonal ? 1u : 0u);
      continue;
    }
    if (!diagonal && positive::certifies(lane, a, opposite, box, work.base)) {
      if (positions > need - result) ++work.clipped_credit_blocks;
      result += std::min(positions, need - result);
      remaining -= positions;
      ++work.base.credited_blocks; work.base.credited_positions += positions;
      continue;
    }
    if (rejects_fixed_pair(lane, a, b0, box, work)) {
      work.negative_rejected_pairs += positions - (diagonal ? 1u : 0u);
      remaining -= positions - (diagonal ? 1u : 0u);
      continue;
    }
    const auto& children = ix.nodes[static_cast<std::size_t>(node)];
    stack.push_back(children.right); stack.push_back(children.left);
  }
  if (result == need) {
    ++work.saturated_anchors;
    work.saturation_unvisited_pairs += remaining;
  }
  return result;
}
inline void factor_histogram(const CloudIndex& ix, Lane lane, NodeRef factor,
                             const AxisBox& opposite, std::vector<u64>& output,
                             std::size_t scalar_below, Work& work, u64 need) {
  const NodeRange range = ix.range_of(factor);
  const u64 size = static_cast<u64>(range.last - range.first + 1);
  if (need == std::numeric_limits<u64>::max() && (size <= scalar_below || size == 1)) {
    positive::factor_histogram(ix, lane, factor, opposite, output, scalar_below, work.base);
    return;
  }
  output.assign(static_cast<std::size_t>(size), 0);
  work.base.logical_pairs += size * (size - 1);
  if (size == 1) {
    ++work.base.singleton_factors;
    if (need == 0) ++work.saturated_anchors;
    return;
  }
  if (size <= scalar_below) {
    ++work.base.scalar_factors;
    for (i32 a = range.first; a <= range.last; ++a) {
      u64 remaining = size - 1;
      u64& value = output[static_cast<std::size_t>(a - range.first)];
      for (i32 z = range.first; z <= range.last && value < need; ++z) {
        if (z == a) continue;
        --remaining; ++work.base.scalar_tests;
        if (universal_over_corners(lane, ix.upos[static_cast<std::size_t>(a)], opposite,
                                   ix.upos[static_cast<std::size_t>(z)])) {
          ++value; ++work.base.scalar_true;
        }
      }
      if (value == need) { ++work.saturated_anchors; work.saturation_unvisited_pairs += remaining; }
    }
    return;
  }
  ++work.base.block_factors;
  const P3 b0 = box_representative(opposite);
  for (i32 a = range.first; a <= range.last; ++a)
    output[static_cast<std::size_t>(a - range.first)] = fixed_anchor(ix, lane, factor, opposite, b0, a, work, need);
}
inline Work corner_histograms(const CloudIndex& ix, Lane lane, const WspdRect& r,
                              std::vector<u64>& ha, std::vector<u64>& hb,
                              std::size_t scalar_below = 8,
                              u64 need = std::numeric_limits<u64>::max()) {
  Work work;
  factor_histogram(ix, lane, r.a, ix.box_of(r.b), ha, scalar_below, work, need);
  factor_histogram(ix, lane, r.b, ix.box_of(r.a), hb, scalar_below, work, need);
  return work;
}
}  // namespace mhgp7::histogram_negative_private
