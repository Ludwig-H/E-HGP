#pragma once

// Explicit AUDIT adaptation of src/wspd/front.cpp at
// ba11e3abfe686d8078c13bacfc066e372d6bfc10, SHA256
// 1af797b322e28f0b3e5bf1dc5014c2397ea2acfbe486a5d273a30aa716985f89.
// The distance/filter/emission primitives below retain the pinned source
// bodies. One whole-task step replaces its stack loop; no filter is paused.
// This is a SERIAL partition experiment, not a scheduler or a census API.

#include "wspd/front.hpp"
#include "spindle/predicates.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <stdexcept>
#include <vector>

namespace mhgp8::audit_tasks {

struct Task {
  std::size_t a{};
  std::size_t b{};
  std::uint8_t mask{};
  u64 depth{};
};

struct JobStats {
  Task seed;  // Untreated task, with inherited orientation/mask/absolute depth.
  u64 seed_mass{};  // choose(|A|,2) if diagonal, otherwise |A|*|B|.
  u64 product_visits{};
  u64 witness_descent_steps{};
  u64 rectangles{};
  u64 pair_mass{};  // Emitted rectangle masses once each, not once per lane.
  double elapsed_ms{};  // Traversal and synchronous callbacks; stack is reused.
};

struct PartitionResult {
  WspdFrontResult front;
  WspdFrontWork prefix;
  std::vector<JobStats> jobs;
  std::size_t maximum_ready{};  // FIFO pending tasks, distinct from DFS stacks.
  std::size_t task_bytes{sizeof(Task)};  // Layout only, not any memory peak.
  double prefix_ms{};
  double total_ms{};
};

namespace detail {
using Clock = std::chrono::steady_clock;
inline double milliseconds(Clock::time_point a, Clock::time_point b) {
  return std::chrono::duration<double, std::milli>(b - a).count();
}

inline u64 product(std::size_t left, std::size_t right) {
  const auto value = static_cast<i128>(left) * right;
  if (value > std::numeric_limits<u64>::max()) {
    throw std::overflow_error("mhgp8 WSPD pair mass exceeds u64");
  }
  return static_cast<u64>(value);
}

inline i64 diagonal2(const Box3& box) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 delta = static_cast<i64>(box.high[axis]) - box.low[axis];
    result += delta * delta;
  }
  return result;
}

inline i64 gap2(const Box3& a, const Box3& b) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 delta = std::max<i64>({0, static_cast<i64>(a.low[axis]) - b.high[axis],
                                      static_cast<i64>(b.low[axis]) - a.high[axis]});
    result += delta * delta;
  }
  return result;
}

inline bool contains(Range range, std::size_t rank) {
  return range.first <= rank && rank < range.last;
}


inline u64 choose_two(std::size_t n) {
  return n < 2 ? 0 : n % 2 == 0 ? product(n / 2, n - 1) : product(n, (n - 1) / 2);
}

inline u64 emitted_mass(const WspdFrontWork& work) {
  u64 result = 0;
  for (const auto value : work.size_class_pair_mass) counter_add(result, value);
  return result;
}

struct Children {
  std::array<Task, 3> tasks{};
  std::size_t size{};
};

// Internal-only execution seam: run_partitioned creates every Task itself.
// Do not interpret arbitrary node integers as an adoptable index certificate.
class Stepper {
 public:
  Stepper(const Q2CensusIndex& index, unsigned kmax, unsigned separation,
          WspdFrontMode mode, const WspdRectangleConsumer& consumer, std::uint8_t requested_mask)
      : nodes_(index.spatial_nodes()), order_(index.spatial_order()),
        points_(index.cloud().points()), kmax_(kmax), separation_(separation),
        mode_(mode), consumer_(consumer) {
    result_.total_unordered_pairs = choose_two(points_.size());
    for (unsigned lane = 0; lane < 3; ++lane) {
      if (lane < kmax_ && (requested_mask & (1U << lane)) != 0) {
        thresholds_[lane] = kmax_ - lane;
        result_.active_lane_mask |= static_cast<std::uint8_t>(1U << lane);
      }
    }
  }

  [[nodiscard]] Task root() const { return {0, 0, result_.active_lane_mask, 0}; }
  [[nodiscard]] const WspdFrontWork& work() const { return result_.work; }
  [[nodiscard]] u64 seed_mass(const Task& task) const {
    const auto a = nodes_[task.a].range.size();
    return task.a == task.b ? choose_two(a) : product(a, nodes_[task.b].range.size());
  }
  void note_stack(std::size_t size) {
    result_.work.max_stack_size = std::max(result_.work.max_stack_size, static_cast<u64>(size));
  }

  // Children are returned in visit order LL,LR,RR or left,right. The FIFO
  // appends them in this order; a local DFS pushes them in the reverse order.
  [[nodiscard]] Children step(const Task& task) {
    counter_add(result_.work.product_visits);
    result_.work.max_product_depth = std::max(result_.work.max_product_depth, task.depth);
    const auto& a = nodes_[task.a];
    const auto& b = nodes_[task.b];
    if (task.a == task.b) {
      if (a.left == Q2SpatialNode::absent) {
        counter_add(result_.work.diagonal_leaves);
        return {};
      }
      counter_add(result_.work.diagonal_splits);
      return {{{Task{a.left, a.left, task.mask, task.depth + 1},
                Task{a.left, a.right, task.mask, task.depth + 1},
                Task{a.right, a.right, task.mask, task.depth + 1}}}, 3};
    }
    auto mask = task.mask;
    if (mode_ == WspdFrontMode::MidpointSamples) mask = filter(a, b, mask);
    const auto mass = product(a.range.size(), b.range.size());
    for (unsigned lane = 0; lane < 3; ++lane) {
      const auto bit = static_cast<std::uint8_t>(1U << lane);
      if ((task.mask & bit) != 0 && (mask & bit) == 0)
        counter_add(result_.work.rejected_pair_mass[lane], mass);
    }
    if (mask == 0) {
      counter_add(result_.work.fully_rejected_products);
      return {};
    }
    counter_add(result_.work.separation_tests);
    const auto da = diagonal2(a.box);
    const auto db = diagonal2(b.box);
    if (static_cast<i128>(gap2(a.box, b.box)) >=
        static_cast<i128>(separation_) * separation_ * std::max(da, db)) {
      emit(task.a, task.b, mask, mass);
      return {};
    }
    counter_add(result_.work.disjoint_splits);
    const bool split_a = a.left != Q2SpatialNode::absent &&
                        (b.left == Q2SpatialNode::absent || da >= db);
    const auto& split = split_a ? a : b;
    if (split.left == Q2SpatialNode::absent)
      throw std::logic_error("audit WSPD distinct singleton boxes must be separated");
    return {{{Task{split_a ? split.left : task.a, split_a ? task.b : split.left, mask, task.depth + 1},
              Task{split_a ? split.right : task.a, split_a ? task.b : split.right, mask, task.depth + 1},
              Task{}}}, 2};
  }

  [[nodiscard]] WspdFrontResult finish() const {
    for (unsigned lane = 0; lane < 3; ++lane) {
      const auto expected = (result_.active_lane_mask & (1U << lane)) != 0
                                ? result_.total_unordered_pairs : 0;
      auto accounted = result_.work.rejected_pair_mass[lane];
      counter_add(accounted, result_.work.residual_pair_mass[lane]);
      if (accounted != expected) throw std::logic_error("audit WSPD lane mass ledger failed");
    }
    return result_;
  }

 private:
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
};
}  // namespace detail

// The same immutable index and consumer are borrowed through every whole
// synchronous step. Exceptions propagate; previously emitted rectangles are
// not rolled back. Jobs are processed serially and contain no census state.
// FIFO preparation can change emission order; compare labelled multisets,
// not callback order (width=1 retains the original DFS order).
// width is a target count of unprocessed seeds, never an exploration cap.
// A diagonal step can overshoot the target by one seed. If traversal ends
// before reaching width, the prefix owns all work and jobs may be empty.
// front.work.max_stack_size records only the maximum LOCAL job DFS stack;
// prefix.max_stack_size is zero and maximum_ready describes the FIFO instead.
[[nodiscard]] inline PartitionResult run_partitioned(
    const Q2CensusIndex& index, unsigned kmax, unsigned separation_s,
    WspdFrontMode mode, const WspdRectangleConsumer& consumer,
    std::uint8_t requested_lane_mask, std::size_t width) {
  const auto started = detail::Clock::now();
  if (kmax == 0 || kmax > 10 || separation_s == 0 || width == 0 || !consumer ||
      (mode != WspdFrontMode::Pure && mode != WspdFrontMode::MidpointSamples))
    throw std::invalid_argument("audit WSPD requires K1..10, positive s/width, mode and consumer");
  const unsigned available = (1U << std::min(kmax, 3U)) - 1;
  if (requested_lane_mask == 0 || requested_lane_mask > 7 ||
      (requested_lane_mask & available) == 0)
    throw std::invalid_argument("audit WSPD mask must intersect available lanes");
  PartitionResult output;
  {
    detail::Stepper stepper(index, kmax, separation_s, mode, consumer, requested_lane_mask);
    std::deque<Task> ready;
    ready.push_back(stepper.root());
    output.maximum_ready = 1;
    while (!ready.empty() && ready.size() < width) {
      const auto task = ready.front();
      ready.pop_front();
      const auto children = stepper.step(task);
      for (std::size_t i = 0; i < children.size; ++i) ready.push_back(children.tasks[i]);
      output.maximum_ready = std::max(output.maximum_ready, ready.size());
    }
    output.prefix = stepper.work();
    output.prefix_ms = detail::milliseconds(started, detail::Clock::now());
    output.jobs.reserve(ready.size());
    std::vector<Task> stack;
    if (!ready.empty()) stack.reserve(97);  // Worker-local storage, not a cap.
    while (!ready.empty()) {
      const auto seed = ready.front();
      ready.pop_front();
      JobStats job;
      job.seed = seed;
      job.seed_mass = stepper.seed_mass(seed);
      const auto before = stepper.work();
      const auto before_mass = detail::emitted_mass(before);
      const auto job_started = detail::Clock::now();
      stack.push_back(seed);
      stepper.note_stack(stack.size());
      while (!stack.empty()) {
        const auto task = stack.back();
        stack.pop_back();
        const auto children = stepper.step(task);
        for (auto i = children.size; i > 0; --i) {
          stack.push_back(children.tasks[i - 1]);
          stepper.note_stack(stack.size());
        }
      }
      job.elapsed_ms = detail::milliseconds(job_started, detail::Clock::now());
      const auto& after = stepper.work();
      job.product_visits = after.product_visits - before.product_visits;
      job.witness_descent_steps = after.witness_descent_steps - before.witness_descent_steps;
      job.rectangles = after.emitted_rectangles - before.emitted_rectangles;
      job.pair_mass = detail::emitted_mass(after) - before_mass;
      output.jobs.push_back(job);
    }
    output.front = stepper.finish();
  }
  output.total_ms = detail::milliseconds(started, detail::Clock::now());
  return output;
}

}  // namespace mhgp8::audit_tasks
