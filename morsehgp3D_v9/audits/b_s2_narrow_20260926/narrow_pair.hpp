#pragma once

// Isolated audit prototype; not part of the product or its qualification.
#include "../../src/gpu/witness_filter.hpp"

namespace mhgp9::audit_b::s2_narrow {
using namespace mhgp9::gpu;

struct CrossExtrema { i64 low[3], high[3]; };
struct Decision { u8 excluded{}, admitted{}; bool narrow{}; };
struct Work { std::uint64_t visits{}, eligible{}, fallback{}, h_rejected{}; };

inline CrossExtrema cross_extrema(const PairBounds& p, const FlatBox& z) {
  CrossExtrema c{};
  for (int axis = 0; axis < 3; ++axis) {
    const int j = (axis + 1) % 3, k = (axis + 2) % 3;
    const i64 first = p.difference[j], second = -p.difference[k];
    c.low[axis] = p.cross_constant[axis] + first * static_cast<i64>(first < 0 ? z.high[k] : z.low[k]) +
                  second * static_cast<i64>(second < 0 ? z.high[j] : z.low[j]);
    c.high[axis] = p.cross_constant[axis] + first * static_cast<i64>(first < 0 ? z.low[k] : z.high[k]) +
                   second * static_cast<i64>(second < 0 ? z.low[j] : z.high[j]);
  }
  return c;
}

// Never abs(INT64_MIN), and never square before this gate. Its extra checks
// make the numeric helper closed even on fabricated Bounds4/extrema inputs.
inline bool narrow_safe(const Bounds4& h, const CrossExtrema& c) {
  constexpr i64 h_limit = i64{1} << 30;
  constexpr i64 c_limit = i64{1} << 28;
  if (h.maximum4 <= 0 || h.maximum4 > h_limit || h.minimum4 > h.maximum4) return false;
  for (int axis = 0; axis < 3; ++axis)
    if (c.low[axis] < -c_limit || c.high[axis] > c_limit || c.low[axis] > c.high[axis]) return false;
  return true;
}

template<class Wide> inline Decision decide_as(const Bounds4& h, const CrossExtrema& c) {
  Wide lo = 0, hi = 0;
  for (int axis = 0; axis < 3; ++axis) {
    const Wide l = static_cast<Wide>(c.low[axis]) * c.low[axis];
    const Wide r = static_cast<Wide>(c.high[axis]) * c.high[axis];
    lo += c.low[axis] <= 0 && c.high[axis] >= 0 ? Wide{0} : (l < r ? l : r);
    hi += l < r ? r : l;
  }
  const Wide hmax2 = static_cast<Wide>(h.maximum4) * h.maximum4;
  const Wide hmin2 = h.minimum4 > 0 ? static_cast<Wide>(h.minimum4) * h.minimum4 : Wide{0};
  Decision d;
  for (unsigned lane = 0; lane < 2; ++lane) {
    const u8 bit = static_cast<u8>(2U << lane);
    const Wide alpha = lane == 0 ? 3 : 2;
    if (alpha * hmax2 <= 16 * lo) d.excluded = static_cast<u8>(d.excluded | bit);
    else if (h.minimum4 > 0 && alpha * hmin2 > 16 * hi)
      d.admitted = static_cast<u8>(d.admitted | bit);
  }
  return d;
}

inline Decision decide(const PairBounds& p, const FlatBox& z, const Bounds4& h) {
  const auto c = cross_extrema(p, z);
  const bool narrow = narrow_safe(h, c);
  auto d = narrow ? decide_as<i64>(h, c) : decide_as<i128>(h, c);
  d.narrow = narrow;
  return d;
}

// Literal traversal twin of gpu::filter<true>. Only the numeric comparison
// dispatch changes; population, strictness, K, masks and DFS order do not.
inline u8 filter_pairs(const FlatNode* nodes, const FlatBox& a, const FlatBox& b,
                       unsigned kmax, u8 lane_mask, Work& work) {
  const u8 available = kmax >= 3 ? 6 : kmax == 2 ? 2 : 0;
  u8 remaining = static_cast<u8>(lane_mask & available);
  if (remaining == 0) return 0;
  const auto pair = prepare_pair(a.low, b.low);
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
    ++work.visits;
    const Bounds4 h = pair_h(pair, node.box);
    if (h.maximum4 <= 0) { ++work.h_rejected; continue; }
    const auto d = decide(pair, node.box, h);
    ++(d.narrow ? work.eligible : work.fallback);
    for (unsigned lane = 0; lane < 2; ++lane) {
      const u8 bit = static_cast<u8>(2U << lane);
      if ((mask & bit) == 0) continue;
      if ((d.excluded & bit) != 0) { mask = static_cast<u8>(mask & ~bit); continue; }
      if ((d.admitted & bit) == 0) continue;
      const u32 population = node.last - node.first;
      const unsigned room = threshold[lane] - count[lane];
      count[lane] += room < population ? room : population;
      mask = static_cast<u8>(mask & ~bit);
      if (count[lane] == threshold[lane]) remaining = static_cast<u8>(remaining & ~bit);
    }
    if (mask == 0 || node.left == absent32) continue;
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
}  // namespace mhgp9::audit_b::s2_narrow
