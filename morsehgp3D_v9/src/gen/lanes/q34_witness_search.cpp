#include "q34_witness_search.hpp"
#include "q34_pair_bounds.hpp"

#include "../pipeline/q2_joint_bounds.hpp"
#include "../spindle/predicates.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace mhgp9::gen {
// Internal adapter: Z always belongs to the immutable certified index.
// Public entry points validate A/B and standalone bound queries remain checked.
struct Q34WitnessSearchBoundsAccess {
  static Q2Bounds h(const Q2JointPreparedBounds& prepared, const Box3& z) {
    return prepared.bounds_unchecked(z);
  }
  static Q2Bounds h(const PreparedPairCitronBounds& prepared, const Box3& z) {
    return prepared.h_bounds_unchecked(z);
  }
  static Q34XiBounds xi(const PreparedPairCitronBounds& prepared, const Box3& z) {
    return prepared.xi_bounds_unchecked(z);
  }
};
namespace {

struct Frame {
  std::size_t node{};
  std::uint8_t mask{};
};

constexpr std::size_t stack_capacity = index_stack_frames;

[[nodiscard]] i64 midpoint_distance16(const std::array<i64, 3>& center4,
                                      const Box3& box) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 delta = std::max<i64>({0,
        4 * static_cast<i64>(box.low[axis]) - center4[axis],
        center4[axis] - 4 * static_cast<i64>(box.high[axis])});
    result += delta * delta;
  }
  // Sixteen times the squared distance from the product-box midpoint.
  // With M=262143, center4 in [0,4M], |delta|<=4M, sum<=48M^2<2^42.
  return result;
}

template <bool Exclusion, bool Affine>
std::uint8_t filter_impl(
    const Q2CensusIndex& index, const Box3& a, const Box3& b,
    std::uint8_t kmax, std::uint8_t lane_mask, Q34WitnessSearchWork& work,
    Q34WitnessBoundsWork* bounds_work, std::vector<Q34WitnessNode>* trace = nullptr) {
  static_assert(!Affine || Exclusion);
  if (kmax == 0 || kmax > 10 || (lane_mask & ~6U) != 0)
    throw std::invalid_argument("mhgp9 gen q34 witness search requires K1..10 and a subset of mask6");
  require_valid_box(a);
  require_valid_box(b);

  const std::uint8_t available = kmax >= 3 ? 6 : kmax == 2 ? 2 : 0;
  std::uint8_t remaining = lane_mask & available;
  counter_add(work.queries);
  counter_add(work.q3_queries, (remaining & 2U) != 0 ? 1 : 0);
  counter_add(work.q4_queries, (remaining & 4U) != 0 ? 1 : 0);
  if constexpr (Exclusion) counter_add(bounds_work->queries);
  if (remaining == 0) return 0;

  // Dispatch once before the DFS. The fixed-endpoint specialization NEVER
  // constructs an unused general preparation alongside the affine one.
  const auto prepared = [&] {
    if constexpr (Affine) return PreparedPairCitronBounds(a.low, b.low);
    else return Q2JointPreparedBounds(a, b);
  }();
  counter_add(work.prepared_bounds);
  if constexpr (Exclusion) {
    if constexpr (Affine) counter_add(bounds_work->pair_preparations);
    else counter_add(bounds_work->general_preparations);
  }
  const auto nodes = index.spatial_nodes();
  if (nodes.empty())
    throw std::logic_error("mhgp9 gen q34 witness index has no root");
  std::array<i64, 3> center4{};
  for (std::size_t axis = 0; axis < 3; ++axis)
    center4[axis] = static_cast<i64>(a.low[axis]) + a.high[axis] +
                    b.low[axis] + b.high[axis];

  const std::array<unsigned, 2> threshold{
      static_cast<unsigned>(kmax) - 1,
      kmax >= 3 ? static_cast<unsigned>(kmax) - 2 : 0};
  std::array<unsigned, 2> count{};
  std::array<Frame, stack_capacity> stack{};
  work.stack_storage_bytes = std::max(work.stack_storage_bytes,
                                      static_cast<u64>(sizeof(stack)));
  std::size_t size = 0;
  const auto push = [&](Frame frame) {
    // An immutable, validated index cannot exceed this bound. Fail rather
    // than silently dropping a frontier if its structural contract changes.
    if (size == stack.size())
      throw std::logic_error("mhgp9 gen q34 witness index exceeds its proven u16 DFS depth");
    stack[size++] = frame;
    work.peak_stack = std::max(work.peak_stack, static_cast<u64>(size));
  };
  push({0, remaining});
  while (size != 0 && remaining != 0) {
    Frame frame = stack[--size];
    frame.mask &= remaining;
    if (frame.mask == 0) {
      counter_add(work.pending_nodes_skipped);
      continue;
    }
    const auto& node = nodes[frame.node];
    const bool leaf = node.left == Q2SpatialNode::absent;
    counter_add(work.node_visits);
    counter_add(work.point_tests, leaf ? 1 : 0);
    counter_add(work.h_bound_tests);
    const auto h = [&] {
      if constexpr (Affine) {
        counter_add(bounds_work->affine_h_tests);
        return Q34WitnessSearchBoundsAccess::h(prepared, node.box);
      } else return Q34WitnessSearchBoundsAccess::h(prepared, node.box);
    }();
    if (h.maximum4 <= 0) {
      counter_add(work.h_excluded_nodes);
      continue;
    }

    const auto input_mask = frame.mask;
    std::uint8_t admitted_mask = 0;
    if (Exclusion || h.minimum4 > 0) {
      counter_add(work.xi_bound_tests);
      if constexpr (Exclusion) {
        if (h.minimum4 <= 0) counter_add(bounds_work->xi_on_nonpositive_minimum);
      }
      const auto xi = [&] {
        if constexpr (Affine) {
          counter_add(bounds_work->affine_xi_tests);
          return Q34WitnessSearchBoundsAccess::xi(prepared, node.box);
        } else {
          const auto general = spindle_detail::xi_bounds(a, b, node.box);
          return Q34XiBounds{general.low, general.high};
        }
      }();
      const i128 xi16 = static_cast<i128>(16) * xi.high;
      const i128 h4_squared = h.minimum4 > 0
          ? static_cast<i128>(h.minimum4) * h.minimum4 : 0;
      // Q2JointPreparedBounds encloses exactly 4H on A x B x Z.
      // Positive 4H<=3M^2 and Xi_high<=12M^4, so alpha*(4H)^2
      // <=27M^4<2^77 and 16Xi_high<=192M^4<2^80 (M=262143). All wider
      // products are promoted BEFORE multiplication. The comparison is
      // strict; equal roots or tangent witnesses never enter the counts.
      for (unsigned lane = 0; lane < 2; ++lane) {
        const auto bit = static_cast<std::uint8_t>(2U << lane);
        if ((frame.mask & bit) == 0) continue;
        const i128 alpha = lane == 0 ? 3 : 2;
        if constexpr (Exclusion) {
          counter_add(lane == 0 ? bounds_work->q3_exclusion_tests
                                : bounds_work->q4_exclusion_tests);
          // Hmax4 is positive after the early H exclusion. For every
          // H>0 point, alpha*(4H)^2 <= alpha*Hmax4^2 <=16*Xi_low
          // refutes a STRICT witness. Equality therefore excludes safely.
          // All products fit the same <2^80 bound as admission. Exclusion
          // removes ONLY this frame's lane, never remaining or its count.
          if (alpha * (static_cast<i128>(h.maximum4) * h.maximum4) <=
              static_cast<i128>(16) * xi.low) {
            counter_add(lane == 0 ? bounds_work->q3_excluded_nodes
                                  : bounds_work->q4_excluded_nodes);
            frame.mask &= static_cast<std::uint8_t>(~bit);
            continue;
          }
        }
        if (h.minimum4 <= 0) continue;
        counter_add(lane == 0 ? work.q3_lane_tests : work.q4_lane_tests);
        if (alpha * h4_squared <= xi16) continue;
        counter_add(lane == 0 ? work.q3_admitted_nodes : work.q4_admitted_nodes);
        const auto credit = std::min<std::size_t>(threshold[lane] - count[lane], node.range.size());
        // credit<=threshold<=9 before narrowing; no size_t/u64 sum with
        // the full population can overflow just to reach a small threshold.
        count[lane] += static_cast<unsigned>(credit);
        counter_add(lane == 0 ? work.q3_credits : work.q4_credits, static_cast<u64>(credit));
        admitted_mask |= bit;
        frame.mask &= static_cast<std::uint8_t>(~bit);
        if (count[lane] == threshold[lane]) {
          remaining &= static_cast<std::uint8_t>(~bit);
          counter_add(lane == 0 ? work.q3_rejected : work.q4_rejected);
        }
      }
    }
    if (admitted_mask != 0) {
      counter_add(work.admitted_nodes);
      if (trace != nullptr) trace->push_back({frame.node, admitted_mask});
    }
    if (frame.mask == 0) {
      if constexpr (Exclusion) {
        if (admitted_mask == input_mask) counter_add(work.fully_admitted_nodes);
        else if (admitted_mask == 0) counter_add(bounds_work->fully_excluded_nodes);
        else counter_add(bounds_work->mixed_terminal_nodes);
      } else counter_add(work.fully_admitted_nodes);
      continue;
    }
    if (leaf) {
      counter_add(work.leaf_remainders);
      continue;
    }

    counter_add(work.split_nodes);
    counter_add(work.midpoint_box_tests, 2);
    const bool left_first = midpoint_distance16(center4, nodes[node.left].box) <=
                            midpoint_distance16(center4, nodes[node.right].box);
    push({left_first ? node.right : node.left, frame.mask});
    push({left_first ? node.left : node.right, frame.mask});
  }
  return remaining;
}

}  // namespace

std::uint8_t filter_q34_witnesses(
    const Q2CensusIndex& index, Point3 a, Point3 b,
    std::uint8_t kmax, std::uint8_t lane_mask, Q34WitnessSearchWork& work,
    Q34WitnessBoundsWork& bounds_work, std::vector<Q34WitnessNode>& trace) {
  trace.clear();
  const Box3 box_a{a, a}, box_b{b, b};
  return filter_impl<true, true>(index, box_a, box_b, kmax, lane_mask, work, &bounds_work, &trace);
}

std::uint8_t q34_cached_witness_rejections(
    const Q2CensusIndex& index, Point3 a, Point3 b, std::uint8_t kmax, std::uint8_t lane_mask,
    std::span<const Q34WitnessNode> cached, Q34WitnessCacheWork& work) {
  if (kmax == 0 || kmax > 10 || (lane_mask & ~6U) != 0)
    throw std::invalid_argument("mhgp9 gen q34 witness cache requires K1..10 and a subset of mask6");
  const std::uint8_t available = kmax >= 3 ? 6 : kmax == 2 ? 2 : 0;
  const std::uint8_t lanes = lane_mask & available;
  counter_add(work.queries);
  if (lanes == 0 || cached.empty()) return 0;
  const PreparedPairCitronBounds prepared(a, b);  // validates both points
  const auto nodes = index.spatial_nodes();
  // Per lane, the cached nodes must be pairwise disjoint (one traced call
  // admits an antichain per lane): a forged or duplicated span is refused,
  // never double-credited.
  for (std::size_t i = 0; i < cached.size(); ++i) {
    if (cached[i].node >= nodes.size() || (cached[i].lanes & ~6U) != 0)
      throw std::invalid_argument("mhgp9 gen q34 witness cache entry outside the index");
    for (std::size_t j = 0; j < i; ++j) {
      if ((cached[i].lanes & cached[j].lanes) == 0) continue;
      const auto x = nodes[cached[i].node].range, y = nodes[cached[j].node].range;
      if (x.first < y.last && y.first < x.last)
        throw std::invalid_argument("mhgp9 gen q34 witness cache nodes overlap on a lane");
    }
  }
  const std::array<unsigned, 2> threshold{static_cast<unsigned>(kmax) - 1,
                                          kmax >= 3 ? static_cast<unsigned>(kmax) - 2 : 0};
  std::array<unsigned, 2> count{};
  for (const auto& entry : cached) {
    const std::uint8_t open = entry.lanes & lanes;
    if (open == 0) continue;
    const auto& node = nodes[entry.node];
    counter_add(work.node_tests);
    const auto h = Q34WitnessSearchBoundsAccess::h(prepared, node.box);
    if (h.minimum4 <= 0) continue;  // also excludes a and b themselves
    const auto xi = Q34WitnessSearchBoundsAccess::xi(prepared, node.box);
    // Same exact admission as the search, written independently of its
    // text (the v8 mutation sites stay unique): alpha*(4Hmin)^2 > 16*Xi_high
    // with alpha = 3 (q3) or 2 (q4); products <2^80 (see filter_impl).
    const i128 h_min_squared = static_cast<i128>(h.minimum4) * h.minimum4;
    const i128 sixteen_xi = 16 * xi.high;
    for (unsigned lane = 0; lane < 2; ++lane) {
      const auto bit = static_cast<std::uint8_t>(2U << lane);
      if ((open & bit) == 0 || count[lane] >= threshold[lane]) continue;
      const i128 weight = 3 - static_cast<i128>(lane);
      if (!(weight * h_min_squared > sixteen_xi)) continue;
      count[lane] += static_cast<unsigned>(std::min<std::size_t>(threshold[lane] - count[lane], node.range.size()));
    }
  }
  std::uint8_t rejected = 0;
  if ((lanes & 2U) != 0 && count[0] >= threshold[0]) { rejected |= 2U; counter_add(work.q3_rejections); }
  if ((lanes & 4U) != 0 && count[1] >= threshold[1]) { rejected |= 4U; counter_add(work.q4_rejections); }
  if (rejected == lanes) counter_add(work.full_rejections);
  return rejected;
}

std::uint8_t filter_q34_witnesses(
    const Q2CensusIndex& index, const Box3& a, const Box3& b,
    std::uint8_t kmax, std::uint8_t lane_mask, Q34WitnessSearchWork& work) {
  // No new counters or exclusion tests exist in this specialization.
  return filter_impl<false, false>(index, a, b, kmax, lane_mask, work, nullptr);
}

std::uint8_t filter_q34_witnesses(
    const Q2CensusIndex& index, const Box3& a, const Box3& b,
    std::uint8_t kmax, std::uint8_t lane_mask, Q34WitnessSearchWork& work,
    Q34WitnessBoundsMode mode, Q34WitnessBoundsWork& bounds_work) {
  switch (mode) {
    case Q34WitnessBoundsMode::Legacy:
      return filter_q34_witnesses(index, a, b, kmax, lane_mask, work);
    case Q34WitnessBoundsMode::Exclusion:
      return filter_impl<true, false>(index, a, b, kmax, lane_mask, work, &bounds_work);
    case Q34WitnessBoundsMode::Affine:
      if (a.low == a.high && b.low == b.high)
        return filter_impl<true, true>(index, a, b, kmax, lane_mask, work, &bounds_work);
      return filter_impl<true, false>(index, a, b, kmax, lane_mask, work, &bounds_work);
    default:
      throw std::invalid_argument("mhgp9 gen q34 witness search requires a valid bounds mode");
  }
}

}  // namespace mhgp9::gen
