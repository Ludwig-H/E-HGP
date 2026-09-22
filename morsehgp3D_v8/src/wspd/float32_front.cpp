#include "wspd/float32_front.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <vector>

namespace mhgp8 {
namespace {
using float32_predicate_detail::as_double;
using float32_predicate_detail::count;

std::uint64_t mass(std::size_t a, std::size_t b) {
  static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
  if (a != 0 && b > std::numeric_limits<std::uint64_t>::max() / a)
    throw std::overflow_error("mhgp8 float32 front pair mass exceeds u64");
  return static_cast<std::uint64_t>(a) * static_cast<std::uint64_t>(b);
}
double diameter(const Float32Box3& box) {
  double value = 0;
  for (std::size_t i = 0; i != 3; ++i) {
    const auto width = as_double(box.high()[i]) - as_double(box.low()[i]);
    value += width * width;
  }
  return value;  // Split heuristic ONLY, never a separation certificate.
}
double distance(const std::array<double, 3>& p, const Float32Box3& box) {
  double value = 0;
  for (std::size_t i = 0; i != 3; ++i) {
    const auto d = std::max({0.0, as_double(box.low()[i]) - p[i], p[i] - as_double(box.high()[i])});
    value += d * d;
  }
  return value;  // Proposal order ONLY.
}
struct Task { std::size_t a{}, b{}, depth{}; };

bool reject(const Float32CloudIndex& index, const Float32IndexNode& a,
            const Float32IndexNode& b, std::size_t threshold, Float32FrontWork& work) {
  const auto n = index.points().size();
  // At most n-2 sites can be strict witnesses for any edge. This avoids
  // allocating or proposing K sites when the caller supplies huge K.
  if (threshold > n - 2) return false;
  count(work.witness_searches);
  std::array<double, 3> middle{};
  for (std::size_t i = 0; i != 3; ++i)
    middle[i] = (as_double(a.box.low()[i]) + as_double(a.box.high()[i]) +
                 as_double(b.box.low()[i]) + as_double(b.box.high()[i])) * 0.25;
  auto node_id = std::size_t{0};
  while (!index.nodes()[node_id].leaf()) {
    count(work.witness_descent_steps);
    count(work.witness_box_tests, 2);
    const auto& node = index.nodes()[node_id];
    node_id = distance(middle, index.nodes()[node.left].box) <=
                      distance(middle, index.nodes()[node.right].box) ? node.left : node.right;
  }
  const auto pivot = index.nodes()[node_id].first;
  const auto width = std::min(n, threshold);
  const auto first = std::min(pivot > width / 2 ? pivot - width / 2 : std::size_t{0}, n - width);
  std::size_t credits = 0;
  for (std::size_t rank = first; rank != first + width; ++rank) {
    count(work.proposed_sites);
    if ((a.first <= rank && rank < a.last) || (b.first <= rank && rank < b.last)) {
      count(work.proposals_in_factors);
      continue;
    }
    const auto& z = index.points()[index.permutation()[rank]];
    const auto box = Float32Box3::from_corners(z.bits(), z.bits());
    if (float32_q3_citron_boxes(a.box, b.box, box, work.geometry) < 0) {
      count(work.witness_credits);
      ++credits;
      if (credits == threshold) return true;
    }
  }
  return false;
}
}  // namespace

void run_float32_front(Float32IndexPtr index, std::size_t kmax, std::uint32_t s,
                       Float32FrontOptions options, const Float32FrontConsumer& consumer,
                       Float32FrontWork& work) {
  if (!index || !consumer || kmax == 0 || s == 0 ||
      (options.witnesses != Float32FrontWitnessMode::Disabled &&
       options.witnesses != Float32FrontWitnessMode::MidpointSamples))
    throw std::invalid_argument("mhgp8 float32 front invalid arguments");
  const Float32FrontConsumer emit = consumer;
  const auto n = index->points().size();
  const auto pairs = n < 2 ? 0 : (n % 2 == 0 ? mass(n / 2, n - 1) : mass(n, (n - 1) / 2));
  count(work.queries);
  count(work.total_unordered_pairs, pairs);
  if (kmax == 1 || n < 2) return;

  std::vector<Task> stack;
  // Median index depth is <=ceil(log2 n). Every push still uses the vector's
  // normal growth: this reserve is an allocation hint, never a search bound.
  stack.reserve(2 * index->max_depth() + 3);
  const auto push = [&](Task task) {
    stack.push_back(task);
    work.max_stack_size = std::max(work.max_stack_size, static_cast<std::uint64_t>(stack.size()));
    work.peak_stack_bytes = std::max(work.peak_stack_bytes, mass(stack.capacity(), sizeof(Task)));
  };
  push({0, 0, 0});
  const auto nodes = index->nodes();
  while (!stack.empty()) {
    const auto task = stack.back();
    stack.pop_back();
    count(work.product_visits);
    work.max_product_depth = std::max(work.max_product_depth, static_cast<std::uint64_t>(task.depth));
    const auto& a = nodes[task.a];
    const auto& b = nodes[task.b];
    if (task.a == task.b) {
      if (a.leaf()) {
        count(work.diagonal_leaves);
        continue;
      }
      count(work.diagonal_splits);
      push({a.right, a.right, task.depth + 1});
      push({a.left, a.right, task.depth + 1});
      push({a.left, a.left, task.depth + 1});
      continue;
    }
    if (options.witnesses == Float32FrontWitnessMode::MidpointSamples &&
        reject(*index, a, b, kmax - 1, work)) {
      count(work.rejected_products);
      count(work.rejected_pairs, mass(a.last - a.first, b.last - b.first));
      continue;
    }
    if (float32_boxes_separated(a.box, b.box, s, work.geometry)) {
      const auto product = mass(a.last - a.first, b.last - b.first);
      count(work.emitted_rectangles);
      count(work.residual_pairs, product);
      count(work.emitted_factor_sites, static_cast<std::uint64_t>(a.last - a.first));
      count(work.emitted_factor_sites, static_cast<std::uint64_t>(b.last - b.first));
      if (a.leaf() && b.leaf()) count(work.leaf_pair_rectangles);
      emit({task.a, task.b, product});
      continue;
    }
    if (a.leaf() && b.leaf())
      throw std::logic_error("mhgp8 two distinct float32 sites failed separation");
    count(work.disjoint_splits);
    if (!a.leaf() && (b.leaf() || diameter(a.box) >= diameter(b.box))) {
      push({a.right, task.b, task.depth + 1});
      push({a.left, task.b, task.depth + 1});
    } else {
      push({task.a, b.right, task.depth + 1});
      push({task.a, b.left, task.depth + 1});
    }
  }
}
}  // namespace mhgp8
