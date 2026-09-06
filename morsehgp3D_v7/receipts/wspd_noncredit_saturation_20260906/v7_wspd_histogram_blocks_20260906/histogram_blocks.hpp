// Private fixed-anchor prototype. No producer source or public API changed.
// Authority: audits/receipts_block_histograms_20260906/README.md
// SHA256 137514adbb09d243ea049000e31a0a7614d715245c0c6286753ffa6f2aa5db76.
// Original-factor positions are counted, not multiplicities. No saturation.
#pragma once
#include <algorithm>
#include <array>
#include <vector>
#include "src/core/inline_stack.hpp"
#include "src/spindle/spindle.hpp"
#include "src/wspd/wavefront.hpp"

namespace mhgp7::histogram_blocks_private {
struct Work {
  u64 logical_pairs = 0, node_visits = 0;
  u64 hmax_tests = 0, hmax_rejected_nodes = 0, hmax_rejected_pairs = 0;
  u64 hmin_tests = 0, ximax_tests = 0;
  u64 credited_blocks = 0, credited_positions = 0;
  u64 scalar_tests = 0, scalar_true = 0, diagonal_leaves = 0;
  u64 scalar_factors = 0, block_factors = 0, singleton_factors = 0;
};
struct Interval { i64 lo, hi; };
inline Interval product(Interval a, Interval b) {
  const std::array<i64, 4> values{a.lo * b.lo, a.lo * b.hi, a.hi * b.lo, a.hi * b.hi};
  const auto bounds = std::minmax_element(values.begin(), values.end());
  return {*bounds.first, *bounds.second};
}
inline AxisBox point_box(const P3& a) { return {{a.x, a.y, a.z}, {a.x, a.y, a.z}}; }

// Each bound is i64 under u16; convert BEFORE squaring (Xi can exceed u64).
inline i128 cross_upper(const P3& a, const AxisBox& t, const AxisBox& z) {
  const std::array<i64, 3> v{a.x, a.y, a.z};
  std::array<Interval, 3> d{}, w{};
  for (unsigned i = 0; i < 3; ++i) {
    d[i] = {t.lo[i] - v[i], t.hi[i] - v[i]};
    w[i] = {z.lo[i] - v[i], z.hi[i] - v[i]};
  }
  i128 result = 0;
  for (unsigned i = 0; i < 3; ++i) {
    const unsigned j = (i + 1) % 3, k = (i + 2) % 3;
    const Interval first = product(d[j], w[k]), second = product(d[k], w[j]);
    const i64 lo = first.lo - second.hi, hi = first.hi - second.lo;
    const i128 magnitude = std::max(lo < 0 ? -lo : lo, hi < 0 ? -hi : hi);
    result += magnitude * magnitude;
  }
  return result;
}
inline bool certifies(Lane lane, const P3& a, const AxisBox& t, const AxisBox& z, Work& work) {
  ++work.hmin_tests;
  const i64 minimum = hmin_boxes(point_box(a), t, z);
  if (minimum <= 0) return false;
  if (lane == Lane::kQ2) return true;
  ++work.ximax_tests;
  const i128 h = minimum;
  return (lane == Lane::kQ3 ? 3 : 2) * h * h > cross_upper(a, t, z);
}

// Factor/anchor are trusted references into the immutable checked index.
// Work is per rectangle call, not an unchecked lifetime-global accumulator.
inline u64 fixed_anchor(const CloudIndex& ix, Lane lane, NodeRef factor,
                        const AxisBox& opposite, i32 anchor, Work& work) {
  const P3& a = ix.upos[static_cast<std::size_t>(anchor)];
  const AxisBox singleton = point_box(a);
  InlineStack<NodeRef, 64> stack;
  stack.push_back(factor);
  u64 result = 0;
  while (!stack.empty()) {
    const NodeRef node = stack.back();
    stack.pop_back();
    ++work.node_visits;
    if (is_leaf(node)) {
      const i32 z = leaf_index(node);
      if (z == anchor) { ++work.diagonal_leaves; continue; }
      ++work.scalar_tests;
      if (universal_over_corners(lane, a, opposite, ix.upos[static_cast<std::size_t>(z)])) {
        ++result;
        ++work.scalar_true;
      }
      continue;
    }
    const NodeRange range = ix.range_of(node);
    const bool diagonal = range.first <= anchor && anchor <= range.last;
    const u64 positions = static_cast<u64>(range.last - range.first + 1);
    const AxisBox box = ix.box_of(node);
    ++work.hmax_tests;
    // Valid because the anchor box is a SINGLE POINT, not a box of anchors.
    if (hmax4_boxes(singleton, opposite, box) <= 0) {
      ++work.hmax_rejected_nodes;
      work.hmax_rejected_pairs += positions - (diagonal ? 1u : 0u);
      continue;
    }
    // A strict uniformly credited box cannot contain z=a. Skip that futile
    // certificate rather than subtracting a diagonal from an invalid credit.
    if (!diagonal && certifies(lane, a, opposite, box, work)) {
      result += positions;
      ++work.credited_blocks;
      work.credited_positions += positions;
      continue;
    }
    const auto& children = ix.nodes[static_cast<std::size_t>(node)];
    stack.push_back(children.right);
    stack.push_back(children.left);
  }
  return result;
}

inline void factor_histogram(const CloudIndex& ix, Lane lane, NodeRef factor,
                             const AxisBox& opposite, std::vector<u64>& output,
                             std::size_t scalar_below, Work& work) {
  const NodeRange range = ix.range_of(factor);
  const u64 size = static_cast<u64>(range.last - range.first + 1);
  output.assign(static_cast<std::size_t>(size), 0);
  work.logical_pairs += size * (size - 1);
  if (size == 1) { ++work.singleton_factors; return; }
  if (size <= scalar_below) {
    ++work.scalar_factors;
    for (i32 a = range.first; a <= range.last; ++a)
      for (i32 z = range.first; z <= range.last; ++z) {
        if (a == z) continue;
        ++work.scalar_tests;
        if (universal_over_corners(lane, ix.upos[static_cast<std::size_t>(a)], opposite,
                                   ix.upos[static_cast<std::size_t>(z)])) {
          ++output[static_cast<std::size_t>(a - range.first)];
          ++work.scalar_true;
        }
      }
    return;
  }
  ++work.block_factors;
  for (i32 a = range.first; a <= range.last; ++a)
    output[static_cast<std::size_t>(a - range.first)] = fixed_anchor(ix, lane, factor, opposite, a, work);
}

// 8 is an UNMEASURED dispatch candidate, never a completeness/resource cap.
// 0 forces blocks for non-singletons; SIZE_MAX gives the scalar reference path.
inline Work corner_histograms(const CloudIndex& ix, Lane lane, const WspdRect& rectangle,
                              std::vector<u64>& ha, std::vector<u64>& hb,
                              std::size_t scalar_below = 8) {
  Work work;
  factor_histogram(ix, lane, rectangle.a, ix.box_of(rectangle.b), ha, scalar_below, work);
  factor_histogram(ix, lane, rectangle.b, ix.box_of(rectangle.a), hb, scalar_below, work);
  return work;
}
}  // namespace mhgp7::histogram_blocks_private
