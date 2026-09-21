#include "q34_witness_search.hpp"
#include "q34_pair_bounds.hpp"

#include "../pipeline/q2_joint_bounds.hpp"
#include "../spindle/predicates.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace mhgp8 {
namespace {

struct Frame {
  std::size_t node{};
  std::uint8_t mask{};
};

constexpr std::size_t stack_capacity =
    3 * std::numeric_limits<std::uint16_t>::digits + 1;

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
  // With M=65535, center4 in [0,4M], |delta|<=4M, sum<=48M^2<2^38.
  return result;
}

template <bool Exclusion, bool Affine>
std::uint8_t filter_impl(
    const Q2CensusIndex& index, const Box3& a, const Box3& b,
    std::uint8_t kmax, std::uint8_t lane_mask, Q34WitnessSearchWork& work,
    Q34WitnessBoundsWork* bounds_work) {
  static_assert(!Affine || Exclusion);
  if (kmax == 0 || kmax > 10 || (lane_mask & ~6U) != 0)
    throw std::invalid_argument("mhgp8 q34 witness search requires K1..10 and a subset of mask6");
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
    throw std::logic_error("mhgp8 q34 witness index has no root");
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
      throw std::logic_error("mhgp8 q34 witness index exceeds its proven u16 DFS depth");
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
        return prepared.h_bounds(node.box);
      } else return prepared.bounds(node.box);
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
          return prepared.xi_bounds(node.box);
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
      // <=27M^4<2^69 and 16Xi_high<=192M^4<2^72. All wider
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
          // All products fit the same <2^72 bound as admission. Exclusion
          // removes ONLY this frame's lane, never remaining or its count.
          if (alpha * (static_cast<i128>(h.maximum4) * h.maximum4) <=
              static_cast<i128>(16) * xi.low) {
            counter_add(lane == 0 ? bounds_work->q3_excluded_nodes
                                  : bounds_work->q4_excluded_nodes);
            frame.mask &= static_cast<std::uint8_t>(~bit);
            remaining &= static_cast<std::uint8_t>(~bit);
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
    if (admitted_mask != 0) counter_add(work.admitted_nodes);
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
      throw std::invalid_argument("mhgp8 q34 witness search requires a valid bounds mode");
  }
}

}  // namespace mhgp8
