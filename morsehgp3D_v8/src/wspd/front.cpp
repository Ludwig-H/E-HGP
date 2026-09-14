#include "wspd/front.hpp"

#include "spindle/predicates.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <vector>

namespace mhgp8 {
namespace {

u64 product(std::size_t left, std::size_t right) {
  const auto value = static_cast<i128>(left) * right;
  if (value > std::numeric_limits<u64>::max()) {
    throw std::overflow_error("mhgp8 WSPD pair mass exceeds u64");
  }
  return static_cast<u64>(value);
}

i64 diagonal2(const Box3& box) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 delta = static_cast<i64>(box.high[axis]) - box.low[axis];
    result += delta * delta;
  }
  return result;
}

i64 gap2(const Box3& a, const Box3& b) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 delta = std::max<i64>({0, static_cast<i64>(a.low[axis]) - b.high[axis],
                                      static_cast<i64>(b.low[axis]) - a.high[axis]});
    result += delta * delta;
  }
  return result;
}

bool contains(Range range, std::size_t rank) {
  return range.first <= rank && rank < range.last;
}

struct Task {
  std::size_t a;
  std::size_t b;
  std::uint8_t mask;
  u64 depth;
};

class Front {
 public:
  Front(const Q2CensusIndex& index, unsigned kmax, unsigned separation,
        WspdFrontMode mode, const WspdRectangleConsumer& consumer, std::uint8_t requested_mask)
      : nodes_(index.spatial_nodes()), order_(index.spatial_order()),
        points_(index.cloud().points()), kmax_(kmax), separation_(separation),
        mode_(mode), consumer_(consumer) {
    const auto n = points_.size();
    // Divide before multiplying, so even a representable choose(n,2)
    // does not require the larger ordered-pair count to fit u64.
    result_.total_unordered_pairs = n % 2 == 0 ? product(n / 2, n - 1)
                                               : product(n, (n - 1) / 2);
    for (unsigned lane = 0; lane < 3; ++lane) {
      if (lane < kmax_ && (requested_mask & (1U << lane)) != 0) {
        thresholds_[lane] = kmax_ - lane;
        result_.active_lane_mask |= static_cast<std::uint8_t>(1U << lane);
      }
    }
    // A reservation, not an exploration cap. The u16 product path has
    // at most 96 coordinate halvings; no result is truncated if it grows.
    stack_.reserve(97);
  }

  WspdFrontResult run() {
    push({0, 0, result_.active_lane_mask, 0});
    while (!stack_.empty()) {
      const auto task = stack_.back();
      stack_.pop_back();
      counter_add(result_.work.product_visits);
      result_.work.max_product_depth = std::max(result_.work.max_product_depth, task.depth);
      const auto& a = nodes_[task.a];
      const auto& b = nodes_[task.b];
      if (task.a == task.b) {
        if (a.left == Q2SpatialNode::absent) {
          counter_add(result_.work.diagonal_leaves);
        } else {
          counter_add(result_.work.diagonal_splits);
          // LCA decomposition: LL, LR and RR partition unordered pairs.
          push({a.right, a.right, task.mask, task.depth + 1});
          push({a.left, a.right, task.mask, task.depth + 1});
          push({a.left, a.left, task.mask, task.depth + 1});
        }
        continue;
      }
      auto mask = task.mask;
      if (mode_ == WspdFrontMode::MidpointSamples) mask = filter(a, b, mask);
      const auto mass = product(a.range.size(), b.range.size());
      for (unsigned lane = 0; lane < 3; ++lane) {
        const auto bit = static_cast<std::uint8_t>(1U << lane);
        if ((task.mask & bit) != 0 && (mask & bit) == 0) {
          counter_add(result_.work.rejected_pair_mass[lane], mass);
        }
      }
      if (mask == 0) {
        counter_add(result_.work.fully_rejected_products);
        continue;
      }
      counter_add(result_.work.separation_tests);
      const auto da = diagonal2(a.box);
      const auto db = diagonal2(b.box);
      if (static_cast<i128>(gap2(a.box, b.box)) >=
          static_cast<i128>(separation_) * separation_ * std::max(da, db)) {
        emit(task.a, task.b, mask, mass);
        continue;
      }
      counter_add(result_.work.disjoint_splits);
      const bool split_a = a.left != Q2SpatialNode::absent &&
                          (b.left == Q2SpatialNode::absent || da >= db);
      const auto& split = split_a ? a : b;
      if (split.left == Q2SpatialNode::absent) {
        throw std::logic_error("mhgp8 WSPD distinct singleton boxes must be separated");
      }
      push({split_a ? split.right : task.a, split_a ? task.b : split.right, mask, task.depth + 1});
      push({split_a ? split.left : task.a, split_a ? task.b : split.left, mask, task.depth + 1});
    }
    for (unsigned lane = 0; lane < 3; ++lane) {
      const auto expected = (result_.active_lane_mask & (1U << lane)) != 0
                                ? result_.total_unordered_pairs : 0;
      auto accounted = result_.work.rejected_pair_mass[lane];
      counter_add(accounted, result_.work.residual_pair_mass[lane]);
      if (accounted != expected) throw std::logic_error("mhgp8 WSPD lane mass ledger failed");
    }
    return result_;
  }

 private:
  void push(Task task) {
    stack_.push_back(task);
    result_.work.max_stack_size = std::max(result_.work.max_stack_size,
                                          static_cast<u64>(stack_.size()));
  }

  i64 midpoint_distance4(const std::array<i64, 3>& center4, const Box3& box) {
    counter_add(result_.work.witness_box_distance_tests);
    i64 result = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto delta = std::max<i64>({0, 4 * static_cast<i64>(box.low[axis]) - center4[axis],
                                         center4[axis] - 4 * static_cast<i64>(box.high[axis])});
      result += delta * delta;
    }
    return result;  // 16 times squared distance; <=48*65535^2 fits i64.
  }

  std::uint8_t filter(const Q2SpatialNode& a, const Q2SpatialNode& b, std::uint8_t mask) {
    unsigned needed = kmax_;
    for (unsigned lane = 0; lane < 3; ++lane) {
      if ((mask & (1U << lane)) != 0) needed = std::min(needed, thresholds_[lane]);
    }
    // A strict universal witness cannot belong to A or B: choose that
    // endpoint and H is zero. If too few exterior sites exist, skip search.
    if (points_.size() - a.range.size() - b.range.size() < needed) return mask;
    counter_add(result_.work.witness_searches);
    std::array<i64, 3> center4{};
    for (std::size_t axis = 0; axis < 3; ++axis) {
      center4[axis] = static_cast<i64>(a.box.low[axis]) + a.box.high[axis] +
                     b.box.low[axis] + b.box.high[axis];
    }
    std::size_t node = 0;
    while (nodes_[node].left != Q2SpatialNode::absent) {
      counter_add(result_.work.witness_descent_steps);
      const auto left = nodes_[node].left;
      const auto right = nodes_[node].right;
      // Heuristic one-path proposal only, not an exact nearest-neighbor
      // query. Ties choose left; no hidden search of the other subtree.
      const auto left_distance = midpoint_distance4(center4, nodes_[left].box);
      const auto right_distance = midpoint_distance4(center4, nodes_[right].box);
      node = left_distance <= right_distance ? left : right;
    }
    const auto count = std::min<std::size_t>(kmax_, order_.size());
    const auto pivot = nodes_[node].range.first;
    const auto first = std::min(pivot > count / 2 ? pivot - count / 2 : 0,
                                order_.size() - count);
    const auto last = first + count;
    std::array<unsigned, 3> credits{};
    for (auto rank = first; rank < last && mask != 0; ++rank) {
      counter_add(result_.work.proposed_sites);
      if (contains(a.range, rank) || contains(b.range, rank)) {
        counter_add(result_.work.proposals_in_factors);
        continue;
      }
      const auto z = singleton_box(points_[order_[rank]]);
      counter_add(result_.work.h_bound_tests);
      const auto h = spindle_detail::h_minimum(a.box, b.box, z);
      if (h <= 0) continue;
      i128 xi = 0;
      if ((mask & 6U) != 0) {
        counter_add(result_.work.xi_bound_tests);
        xi = spindle_detail::xi_bounds(a.box, b.box, z).high;
      }
      const auto h2 = spindle_detail::square(h);
      for (unsigned lane = 0; lane < 3; ++lane) {
        const auto bit = static_cast<std::uint8_t>(1U << lane);
        if ((mask & bit) != 0 && (lane == 0 || (lane == 1 ? 3 : 2) * h2 > xi)) {
          counter_add(result_.work.witness_lane_credits);
          if (++credits[lane] == thresholds_[lane]) mask &= static_cast<std::uint8_t>(~bit);
        }
      }
    }
    return mask;
  }

  void emit(std::size_t a_id, std::size_t b_id, std::uint8_t mask, u64 mass) {
    const auto na = nodes_[a_id].range.size();
    const auto nb = nodes_[b_id].range.size();
    const auto maximum = std::max(na, nb);
    auto& work = result_.work;
    counter_add(work.emitted_rectangles);
    counter_add(work.emitted_factor_sites, static_cast<u64>(na));
    counter_add(work.emitted_factor_sites, static_cast<u64>(nb));
    work.max_factor_size = std::max(work.max_factor_size, static_cast<u64>(maximum));
    const unsigned bin = maximum == 1 ? 0 : maximum < 8 ? 1 : maximum < 64 ? 2
                                                       : maximum < 1024 ? 3 : 4;
    counter_add(work.size_class_rectangles[bin]);
    counter_add(work.size_class_pair_mass[bin], mass);
    if (maximum == 1) counter_add(work.leaf_pair_rectangles);
    for (unsigned lane = 0; lane < 3; ++lane) {
      if ((mask & (1U << lane)) != 0) {
        counter_add(work.lane_rectangles[lane]);
        counter_add(work.residual_pair_mass[lane], mass);
      }
    }
    consumer_(WspdRectangle{a_id, b_id, mask});
  }

  std::span<const Q2SpatialNode> nodes_;
  std::span<const std::size_t> order_;
  std::span<const Point3> points_;
  unsigned kmax_;
  unsigned separation_;
  WspdFrontMode mode_;
  const WspdRectangleConsumer& consumer_;
  std::array<unsigned, 3> thresholds_{};
  WspdFrontResult result_;
  std::vector<Task> stack_;
};

}  // namespace

WspdFrontResult run_wspd_front(const Q2CensusIndex& index, unsigned kmax,
                               unsigned separation_s, WspdFrontMode mode,
                               const WspdRectangleConsumer& consumer, std::uint8_t requested_lane_mask) {
  if (kmax == 0 || kmax > 10 || separation_s == 0 ||
      (mode != WspdFrontMode::Pure && mode != WspdFrontMode::MidpointSamples) || !consumer) {
    throw std::invalid_argument("mhgp8 WSPD requires Kmax1..10, positive s, valid mode and consumer");
  }
  const unsigned available = (1U << std::min(kmax, 3U)) - 1;
  if (requested_lane_mask == 0 || requested_lane_mask > 7 ||
      (requested_lane_mask & available) == 0) {
    throw std::invalid_argument("mhgp8 WSPD requires a mask in 1..7 intersecting available lanes");
  }
  return Front(index, kmax, separation_s, mode, consumer, requested_lane_mask).run();
}

}  // namespace mhgp8
