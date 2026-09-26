#pragma once

// MorseHGP3D v9 (23 septembre 2026) — portable copy (host and CUDA device)
// of the exact q3/q4 witness filter of lanes/q34_witness_search.cpp
// (filter_impl with Exclusion bounds: the affine pair specialization and the
// general box one used for WSPD rectangles). Same integer arithmetic, same
// DFS order, same credits; no work ledger except node visits. Optional
// fixed witness traces do not change traversal or credits. A flat node
// array replaces the index spans.
//
// Exactness is judged, not assumed: the host compilation of this header is
// compared query by query with filter_q34_witnesses on real rectangles and
// pairs (tests/gpu/witness_filter_port_gate.cpp), and the CUDA probe compares
// every device mask with the CPU mask of the same query.
//
// Bounds (M=262143, 18 bits), as in the source: 4H in [-12M^2, 3M^2] fits
// i64; Xi components <= 2M^2 (i64), Xi <= 12M^4 < 2^76 (i128); every
// comparison product is < 2^80 and promoted to i128 before multiplication.

#include <cstdint>

#if defined(__CUDACC__)
#define MHGP9_HD __host__ __device__
#else
#define MHGP9_HD
#endif

namespace mhgp9::gpu {

using i64 = long long;
__extension__ typedef signed __int128 i128;  // as in gen/core/types.hpp (-Wpedantic)
using u32 = std::uint32_t;
using u8 = std::uint8_t;

inline constexpr u32 absent32 = 0xffffffffU;
// index_stack_frames of the generator (3 * 18 splits + 1): a proved bound.
inline constexpr int stack_frames = 55;
// Returned instead of a lane mask when the proved stack bound is violated.
inline constexpr u8 stack_failure = 0xff;

struct FlatBox {
  std::int32_t low[3];
  std::int32_t high[3];
};

// Q2SpatialNode without the escape link; left == absent32 marks a leaf.
struct FlatNode {
  FlatBox box;
  u32 left, right;
  u32 first, last;  // spatial rank range [first, last)
};

// A traced query admits an antichain per lane. Every recorded node pays
// at least one effective credit, so size <= (K-1)+(K-2) <= 17 for K3..10
// (K1: no active lane, K2: at most one node).
// The arrays are split to keep the host/device transfer at 88 bytes rather
// than 17 padded pairs. The trace belongs to the SAME immutable FlatNode
// array as its producer; no pointer, rank permutation or partial count is
// carried to another query. Only entries below size are initialized/read.
inline constexpr unsigned witness_trace_capacity = 17;
struct FixedWitnessTrace {
  u32 nodes[witness_trace_capacity];
  u8 lanes[witness_trace_capacity];
  u8 size = 0;
};
static_assert(sizeof(FixedWitnessTrace) == 88);

struct Bounds4 {
  i64 minimum4, maximum4;
};
struct Xi {
  i128 low, high;
};

MHGP9_HD inline i64 max_i64(i64 a, i64 b) { return a < b ? b : a; }
MHGP9_HD inline i64 min_i64(i64 a, i64 b) { return b < a ? b : a; }
MHGP9_HD inline i128 max_i128(i128 a, i128 b) { return a < b ? b : a; }
MHGP9_HD inline i128 min_i128(i128 a, i128 b) { return b < a ? b : a; }

// PreparedPairCitronBounds (lanes/q34_pair_bounds.hpp), fixed endpoints.
struct PairBounds {
  i64 difference[3], center_twice[3], cross_constant[3];
  i64 diameter_squared;
};

MHGP9_HD inline PairBounds prepare_pair(const std::int32_t a[3], const std::int32_t b[3]) {
  PairBounds p{};
  for (int axis = 0; axis < 3; ++axis) {
    p.difference[axis] = static_cast<i64>(b[axis]) - a[axis];
    p.center_twice[axis] = static_cast<i64>(a[axis]) + b[axis];
    p.diameter_squared += p.difference[axis] * p.difference[axis];
  }
  for (int axis = 0; axis < 3; ++axis) {
    const int j = (axis + 1) % 3, k = (axis + 2) % 3;
    p.cross_constant[axis] = p.difference[k] * static_cast<i64>(a[j]) - p.difference[j] * static_cast<i64>(a[k]);
  }
  return p;
}

MHGP9_HD inline Bounds4 pair_h(const PairBounds& p, const FlatBox& z) {
  Bounds4 r{p.diameter_squared, p.diameter_squared};
  for (int axis = 0; axis < 3; ++axis) {
    const i64 low = 2 * static_cast<i64>(z.low[axis]) - p.center_twice[axis];
    const i64 high = 2 * static_cast<i64>(z.high[axis]) - p.center_twice[axis];
    const i64 low_squared = low * low, high_squared = high * high;
    r.minimum4 -= max_i64(low_squared, high_squared);
    r.maximum4 -= low > 0 ? low_squared : high < 0 ? high_squared : 0;
  }
  return r;
}

MHGP9_HD inline Xi pair_xi(const PairBounds& p, const FlatBox& z) {
  Xi r{0, 0};
  for (int axis = 0; axis < 3; ++axis) {
    const int j = (axis + 1) % 3, k = (axis + 2) % 3;
    const i64 first = p.difference[j], second = -p.difference[k];
    const i64 low = p.cross_constant[axis] + first * static_cast<i64>(first < 0 ? z.high[k] : z.low[k]) +
                    second * static_cast<i64>(second < 0 ? z.high[j] : z.low[j]);
    const i64 high = p.cross_constant[axis] + first * static_cast<i64>(first < 0 ? z.low[k] : z.high[k]) +
                     second * static_cast<i64>(second < 0 ? z.low[j] : z.high[j]);
    const i128 left = static_cast<i128>(low) * low;
    const i128 right = static_cast<i128>(high) * high;
    r.low += low <= 0 && high >= 0 ? i128{0} : min_i128(left, right);
    r.high += max_i128(left, right);
  }
  return r;
}

// Q2JointPreparedBounds (pipeline/q2_joint_bounds.hpp), two boxes.
struct JointBounds {
  i64 center_twice[3][4], distance_squared[3][4];
};

MHGP9_HD inline JointBounds prepare_joint(const FlatBox& a, const FlatBox& b) {
  JointBounds p{};
  for (int axis = 0; axis < 3; ++axis) {
    const i64 a_ends[2] = {a.low[axis], a.high[axis]};
    const i64 b_ends[2] = {b.low[axis], b.high[axis]};
    for (int ai = 0; ai < 2; ++ai)
      for (int bi = 0; bi < 2; ++bi) {
        const i64 difference = b_ends[bi] - a_ends[ai];
        p.center_twice[axis][2 * ai + bi] = a_ends[ai] + b_ends[bi];
        p.distance_squared[axis][2 * ai + bi] = difference * difference;
      }
  }
  return p;
}

MHGP9_HD inline Bounds4 joint_h(const JointBounds& p, const FlatBox& z) {
  Bounds4 r{0, 0};
  for (int axis = 0; axis < 3; ++axis) {
    const i64 low_twice = 2 * static_cast<i64>(z.low[axis]);
    const i64 high_twice = 2 * static_cast<i64>(z.high[axis]);
    i64 minimum4 = 0, maximum4 = 0;
    for (int c = 0; c < 4; ++c) {
      const i64 center = p.center_twice[axis][c];
      const i64 distance = p.distance_squared[axis][c];
      const i64 low_delta = low_twice - center;
      const i64 high_delta = high_twice - center;
      const i64 low_squared = low_delta * low_delta;
      const i64 high_squared = high_delta * high_delta;
      const i64 nearest_squared = low_delta > 0 ? low_squared : high_delta < 0 ? high_squared : 0;
      const i64 lo = distance - max_i64(low_squared, high_squared);
      const i64 hi = distance - nearest_squared;
      minimum4 = c == 0 ? lo : min_i64(minimum4, lo);
      maximum4 = c == 0 ? hi : max_i64(maximum4, hi);
    }
    r.minimum4 += minimum4;
    r.maximum4 += maximum4;
  }
  return r;
}

// spindle_detail::xi_bounds (spindle/predicates.hpp), interval arithmetic.
struct Interval {
  i64 low, high;
};
MHGP9_HD inline Interval interval_subtract(Interval l, Interval r) { return {l.low - r.high, l.high - r.low}; }
MHGP9_HD inline Interval interval_multiply(Interval l, Interval r) {
  const i64 p0 = l.low * r.low, p1 = l.low * r.high, p2 = l.high * r.low, p3 = l.high * r.high;
  return {min_i64(min_i64(p0, p1), min_i64(p2, p3)), max_i64(max_i64(p0, p1), max_i64(p2, p3))};
}

MHGP9_HD inline Xi box_xi(const FlatBox& a, const FlatBox& b, const FlatBox& z) {
  Interval u[3], w[3];
  for (int axis = 0; axis < 3; ++axis) {
    u[axis] = interval_subtract({z.low[axis], z.high[axis]}, {a.low[axis], a.high[axis]});
    w[axis] = interval_subtract({b.low[axis], b.high[axis]}, {z.low[axis], z.high[axis]});
  }
  Xi r{0, 0};
  for (int axis = 0; axis < 3; ++axis) {
    const int j = (axis + 1) % 3, k = (axis + 2) % 3;
    const Interval c = interval_subtract(interval_multiply(u[j], w[k]), interval_multiply(u[k], w[j]));
    const i128 left = static_cast<i128>(c.low) * c.low;
    const i128 right = static_cast<i128>(c.high) * c.high;
    r.low += c.low <= 0 && c.high >= 0 ? i128{0} : min_i128(left, right);
    r.high += max_i128(left, right);
  }
  return r;
}

MHGP9_HD inline i64 midpoint_distance16(const i64 center4[3], const FlatBox& box) {
  i64 result = 0;
  for (int axis = 0; axis < 3; ++axis) {
    const i64 below = 4 * static_cast<i64>(box.low[axis]) - center4[axis];
    const i64 above = center4[axis] - 4 * static_cast<i64>(box.high[axis]);
    const i64 delta = max_i64(max_i64(below, above), 0);
    result += delta * delta;
  }
  return result;
}

// filter_impl<true, Affine> of lanes/q34_witness_search.cpp. Inputs come from
// a validated index: K in 1..10, lane_mask a subset of 6, valid boxes (and
// singleton boxes when Affine). Returns the surviving lanes, or
// stack_failure if the proved DFS bound is violated.
template <bool Affine, bool Trace = false>
MHGP9_HD inline u8 filter(const FlatNode* nodes, const FlatBox& a, const FlatBox& b, unsigned kmax,
                          u8 lane_mask, std::uint64_t& visits, FixedWitnessTrace* trace = nullptr) {
  if constexpr (Trace) {
    if (trace == nullptr) return stack_failure;
    trace->size = 0;
  }
  const u8 available = kmax >= 3 ? 6 : kmax == 2 ? 2 : 0;
  u8 remaining = static_cast<u8>(lane_mask & available);
  if (remaining == 0) return 0;
  PairBounds pair{};
  JointBounds joint{};
  if constexpr (Affine) pair = prepare_pair(a.low, b.low);
  else joint = prepare_joint(a, b);
  i64 center4[3];
  for (int axis = 0; axis < 3; ++axis)
    center4[axis] = static_cast<i64>(a.low[axis]) + a.high[axis] + b.low[axis] + b.high[axis];
  const unsigned threshold[2] = {kmax - 1, kmax >= 3 ? kmax - 2 : 0U};
  unsigned count[2] = {0, 0};
  u32 stack_node[stack_frames];
  u8 stack_mask[stack_frames];
  int size = 0;
  stack_node[size] = 0;
  stack_mask[size++] = remaining;
  while (size != 0 && remaining != 0) {
    --size;
    u8 mask = static_cast<u8>(stack_mask[size] & remaining);
    if (mask == 0) continue;
    const FlatNode& node = nodes[stack_node[size]];
    const bool leaf = node.left == absent32;
    ++visits;
    const Bounds4 h = Affine ? pair_h(pair, node.box) : joint_h(joint, node.box);
    if (h.maximum4 <= 0) continue;
    const Xi xi = Affine ? pair_xi(pair, node.box) : box_xi(a, b, node.box);
    const i128 xi16 = static_cast<i128>(16) * xi.high;
    const i128 h4_squared = h.minimum4 > 0 ? static_cast<i128>(h.minimum4) * h.minimum4 : i128{0};
    u8 admitted_mask = 0;
    for (unsigned lane = 0; lane < 2; ++lane) {
      const u8 bit = static_cast<u8>(2U << lane);
      if ((mask & bit) == 0) continue;
      const i128 alpha = lane == 0 ? 3 : 2;
      // Exclusion: alpha*Hmax4^2 <= 16*Xi_low refutes a strict witness.
      if (alpha * (static_cast<i128>(h.maximum4) * h.maximum4) <= static_cast<i128>(16) * xi.low) {
        mask = static_cast<u8>(mask & ~bit);
        continue;
      }
      if (h.minimum4 <= 0) continue;
      if (alpha * h4_squared <= xi16) continue;
      const u32 population = node.last - node.first;
      const unsigned room = threshold[lane] - count[lane];
      count[lane] += room < population ? room : population;
      if constexpr (Trace) admitted_mask = static_cast<u8>(admitted_mask | bit);
      mask = static_cast<u8>(mask & ~bit);
      if (count[lane] == threshold[lane]) remaining = static_cast<u8>(remaining & ~bit);
    }
    if constexpr (Trace) {
      if (admitted_mask != 0) {
        // Fail closed if an invalid index/query breaks the proved bound;
        // never turn a truncated trace into a successful filter result.
        if (trace->size == witness_trace_capacity) return stack_failure;
        trace->nodes[trace->size] = stack_node[size];
        trace->lanes[trace->size++] = admitted_mask;
      }
    }
    if (mask == 0 || leaf) continue;
    if (size + 2 > stack_frames) return stack_failure;
    const bool left_first =
        midpoint_distance16(center4, nodes[node.left].box) <= midpoint_distance16(center4, nodes[node.right].box);
    stack_node[size] = left_first ? node.right : node.left;
    stack_mask[size++] = mask;
    stack_node[size] = left_first ? node.left : node.right;
    stack_mask[size++] = mask;
  }
  return remaining;
}

// filter_q34_witnesses dispatch in Affine bounds mode: singleton boxes use
// the affine pair specialization, any other box pair the general one.
MHGP9_HD inline u8 filter_boxes(const FlatNode* nodes, const FlatBox& a, const FlatBox& b, unsigned kmax,
                                u8 lane_mask, std::uint64_t& visits) {
  bool singleton = true;
  for (int axis = 0; axis < 3; ++axis)
    singleton = singleton && a.low[axis] == a.high[axis] && b.low[axis] == b.high[axis];
  return singleton ? filter<true>(nodes, a, b, kmax, lane_mask, visits)
                   : filter<false>(nodes, a, b, kmax, lane_mask, visits);
}

}  // namespace mhgp9::gpu
