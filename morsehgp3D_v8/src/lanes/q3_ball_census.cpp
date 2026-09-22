#include "q3_ball_census.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace mhgp8 {
namespace {

struct PowerBounds {
  i128 minimum{};
  i128 maximum{};
};

class PreparedPower final {
 public:
  explicit PreparedPower(const ExactBall& ball) : ball_(ball), coefficients_(ball.coefficients()) {
    const i128 denominator = 2 * coefficients_[0];
    constexpr i128 maximum_coordinate = coordinate_limit;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const i128 numerator = -coefficients_[axis + 1];
      i128 floor = numerator / denominator;
      const i128 remainder = numerator % denominator;
      if (remainder < 0) --floor;  // C++ division truncates toward zero.
      const i128 ceil = floor + (remainder != 0 ? 1 : 0);
      // Clamp before narrowing, even though positive support centres lie
      // inside the coordinate cube. This avoids relying on support-arity metadata.
      floor_[axis] = static_cast<i64>(std::clamp<i128>(floor, 0, maximum_coordinate));
      ceil_[axis] = static_cast<i64>(std::clamp<i128>(ceil, 0, maximum_coordinate));
    }
  }

  [[nodiscard]] PowerBounds bounds(const Q2SpatialNode& node) const {
    if (node.left == Q2SpatialNode::absent) {
      // The immutable index certifies a singleton box at every leaf.
      const auto power = ball_.power(node.box.low);
      return {power, power};
    }
    PowerBounds result{coefficients_[4], coefficients_[4]};
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const i64 low = node.box.low[axis], high = node.box.high[axis];
      const auto term = [&](i64 coordinate) {
        return (coefficients_[0] * static_cast<i128>(coordinate) + coefficients_[axis + 1]) * coordinate;
      };
      const auto at_floor = term(std::clamp(floor_[axis], low, high));
      const auto at_ceil = term(std::clamp(ceil_[axis], low, high));
      result.minimum += std::min(at_floor, at_ceil);
      result.maximum += std::max(term(low), term(high));
    }
    // For M=262143 (18 bits) the dominating q3 coefficients obey A<=12M^4,
    // |Bi|<=60M^5, |C|<=144M^6. Thus |A*t^2+Bi*t|<=72M^6 and
    // every partial or complete bound <=360M^6<2^117. Existing q2/q4
    // factory bounds are smaller. 2A and -Bi are also safe; no Bi^2 or
    // rational cross multiplication is used. Promotions precede products.
    return result;
  }

 private:
  const ExactBall& ball_;
  const std::array<i128, 5>& coefficients_;
  std::array<i64, 3> floor_{};
  std::array<i64, 3> ceil_{};
};

struct Frame {
  std::size_t node{};
  PowerBounds bounds;
};

constexpr std::size_t stack_capacity = index_stack_frames;

}  // namespace

Q3BallCensusResult census_q3_ball(
    const Q2CensusIndex& index, const ExactBall& ball, std::size_t threshold,
    std::vector<std::size_t>& shell, Q3BallCensusWork& work) {
  if (threshold == 0)
    throw std::invalid_argument("mhgp8 q3 ball census requires a positive threshold");
  if (ball.coefficients()[0] <= 0)
    throw std::logic_error("mhgp8 exact ball lost its positive leading coefficient");
  const auto nodes = index.spatial_nodes();
  if (nodes.empty())
    throw std::logic_error("mhgp8 q3 census index has no root");

  shell.clear();
  counter_add(work.queries);
  const PreparedPower prepared(ball);
  counter_add(work.preparations);
  counter_add(work.vertex_axes, 3);
  std::array<Frame, stack_capacity> stack{};
  work.stack_storage_bytes = std::max(work.stack_storage_bytes, static_cast<u64>(sizeof(stack)));
  std::size_t size = 0;
  const auto push = [&](Frame frame, u64& peak) {
    // build() halves the chosen positive integer extent at each split.
    // Each coordinate allows at most coordinate_bits such reductions:
    // depth<=max_index_depth (54) and a binary DFS has <=index_stack_frames
    // (55) pending frames. This is not a visit budget.
    if (size == stack.size())
      throw std::logic_error("mhgp8 q3 census index exceeds its proven DFS depth");
    stack[size++] = frame;
    peak = std::max(peak, static_cast<u64>(size));
  };
  const auto prepare = [&](std::size_t node_id, bool payload) {
    const auto& node = nodes[node_id];
    if (payload) {
      counter_add(work.shell_bounds_prepared);
      counter_add(node.left == Q2SpatialNode::absent ? work.shell_point_tests : work.shell_box_bound_tests);
    } else {
      counter_add(work.count_bounds_prepared);
      counter_add(node.left == Q2SpatialNode::absent ? work.count_point_tests : work.count_box_bound_tests);
    }
    return Frame{node_id, prepared.bounds(node)};
  };

  std::size_t depth = 0;
  push(prepare(0, false), work.peak_count_stack);
  while (size != 0 && depth < threshold) {
    const Frame frame = stack[--size];
    const auto& node = nodes[frame.node];
    counter_add(work.count_node_visits);
    if (frame.bounds.minimum >= 0) {
      counter_add(work.count_nonnegative_nodes);
      counter_add(work.count_nonnegative_sites, static_cast<u64>(node.range.size()));
      continue;
    }
    if (frame.bounds.maximum < 0) {
      counter_add(work.count_inside_nodes);
      const auto credit = std::min(threshold - depth, node.range.size());
      depth += credit;
      counter_add(work.count_inside_sites, static_cast<u64>(credit));
      continue;
    }
    // A leaf has equal min/max and was classified by one of the branches.
    if (node.left == Q2SpatialNode::absent)
      throw std::logic_error("mhgp8 q3 census leaf has inconsistent power bounds");
    counter_add(work.count_split_nodes);
    const auto left = prepare(node.left, false);
    const auto right = prepare(node.right, false);
    const bool left_first = left.bounds.minimum <= right.bounds.minimum;
    push(left_first ? right : left, work.peak_count_stack);
    push(left_first ? left : right, work.peak_count_stack);
  }
  counter_add(work.count_prepared_unvisited, static_cast<u64>(size));
  if (depth == threshold) {
    counter_add(work.count_saturations);
    counter_add(work.rejected_queries);
    return {false, threshold};
  }

  // A separate pass is essential: the depth pass excluded all contacts via
  // min>=0, and may have admitted interior blocks without visiting leaves.
  const auto order = index.spatial_order();
  push(prepare(0, true), work.peak_shell_stack);
  while (size != 0) {
    const Frame frame = stack[--size];
    const auto& node = nodes[frame.node];
    counter_add(work.shell_node_visits);
    if (frame.bounds.minimum > 0 || frame.bounds.maximum < 0) {
      counter_add(work.shell_excluded_nodes);
      continue;
    }
    if (node.left == Q2SpatialNode::absent) {
      shell.push_back(order[node.range.first]);
      counter_add(work.shell_ids);
      continue;
    }
    counter_add(work.shell_split_nodes);
    push(prepare(node.right, true), work.peak_shell_stack);
    push(prepare(node.left, true), work.peak_shell_stack);
  }
  counter_add(work.accepted_queries);
  return {true, depth};
}

}  // namespace mhgp8
