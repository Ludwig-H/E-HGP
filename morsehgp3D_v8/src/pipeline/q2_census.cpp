#include "q2_census.hpp"
#include "../spindle/q2_prepared_bounds.hpp"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace mhgp8 {
namespace {

using Clock = std::chrono::steady_clock;
constexpr std::size_t absent = std::numeric_limits<std::size_t>::max();

[[nodiscard]] double milliseconds(Clock::time_point begin, Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

[[nodiscard]] Box3 joined(const Box3& a, const Box3& b) {
  return {{std::min(a.low.x, b.low.x), std::min(a.low.y, b.low.y),
           std::min(a.low.z, b.low.z)},
          {std::max(a.high.x, b.high.x), std::max(a.high.y, b.high.y),
           std::max(a.high.z, b.high.z)}};
}

[[nodiscard]] i64 squared_diagonal(const Box3& box) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 delta = static_cast<i64>(box.high[axis]) - box.low[axis];
    result += delta * delta;
  }
  return result;
}

[[nodiscard]] Q2BallKey ball_key(const Point3& a, const Point3& b) {
  Q2BallKey key;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    key.center_twice[axis] = static_cast<std::uint32_t>(a[axis]) + b[axis];
    const i64 difference = static_cast<i64>(a[axis]) - b[axis];
    key.diameter_squared += static_cast<u64>(difference * difference);
  }
  return key;
}

// Coordinates are promoted BEFORE subtraction or multiplication. With
// d=65535, -12*d^2 <= 4H <= 3*d^2; all sums below fit comfortably in i64.
// The key uses the doubled center and four times the squared radius, so
// half-integral centers require neither rounding nor division.
[[nodiscard]] i64 point_power4(const Q2BallKey& key, const Point3& point) {
  i64 value = static_cast<i64>(key.diameter_squared);
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 delta = 2 * static_cast<i64>(point[axis]) - key.center_twice[axis];
    value -= delta * delta;
  }
  return value;
}

using PowerBounds = Q2Bounds;

// The inexpensive singleton-pair path: exact min/max distances from the
// doubled center to the doubled Z box. Pairwise never pays the product-box
// routine used to share several different queries.
[[nodiscard]] PowerBounds pair_bounds(const Q2BallKey& key, const Box3& z) {
  i64 nearest_squared = 0;
  i64 farthest_squared = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 center = key.center_twice[axis];
    const i64 low = 2 * static_cast<i64>(z.low[axis]);
    const i64 high = 2 * static_cast<i64>(z.high[axis]);
    const i64 nearest = center < low ? low - center : center > high ? center - high : 0;
    const i64 low_delta = low - center;
    const i64 high_delta = high - center;
    nearest_squared += nearest * nearest;
    farthest_squared += std::max(low_delta * low_delta, high_delta * high_delta);
  }
  const auto radius4 = static_cast<i64>(key.diameter_squared);
  return {radius4 - farthest_squared, radius4 - nearest_squared};
}

}  // namespace

Q2CensusIndex::Q2CensusIndex(CloudPtr cloud) : cloud_(std::move(cloud)) {
  if (!cloud_) {
    throw std::invalid_argument("mhgp8 q2 index requires an immutable cloud");
  }
  order_.resize(cloud_->points().size());
  std::iota(order_.begin(), order_.end(), std::size_t{0});
  if (order_.empty()) {
    throw std::logic_error("mhgp8 certified rectangle has no sites");
  }
  static_cast<void>(build({0, order_.size()}, 0));
}

std::size_t Q2CensusIndex::build(Range range, u64 depth) {
  const auto points = cloud_->points();
  Box3 box = singleton_box(points[order_[range.first]]);
  for (std::size_t position = range.first; position < range.last; ++position) {
    counter_add(work_.point_visits);
    box = joined(box, singleton_box(points[order_[position]]));
  }
  const auto node_id = nodes_.size();
  nodes_.push_back(Node{range, box});
  counter_add(work_.nodes);
  work_.max_depth = std::max(work_.max_depth, depth);
  if (range.size() == 1) {
    nodes_[node_id].escape = nodes_.size();
    counter_add(work_.escape_links);
    if (nodes_[node_id].escape != node_id + 1) {
      throw std::logic_error("mhgp8 q2 leaf escape must follow its preorder node");
    }
    return node_id;
  }
  std::size_t axis = 0;
  for (std::size_t other = 1; other < 3; ++other) {
    if (box.high[other] - box.low[other] > box.high[axis] - box.low[axis]) {
      axis = other;
    }
  }
  const unsigned midpoint = (static_cast<unsigned>(box.low[axis]) + box.high[axis]) / 2;
  const auto begin = order_.begin() + static_cast<std::ptrdiff_t>(range.first);
  const auto end = order_.begin() + static_cast<std::ptrdiff_t>(range.last);
  const auto cut = std::partition(begin, end, [&](std::size_t id) {
    counter_add(work_.point_visits);
    return points[id][axis] <= midpoint;
  });
  const auto split = static_cast<std::size_t>(cut - order_.begin());
  if (split == range.first || split == range.last) {
    throw std::logic_error("mhgp8 q2 index failed to split distinct u16 sites");
  }
  // Midpoint splits halve one positive coordinate extent. At most 48 such
  // splits occur on a u16 path; no artificial depth or visit limit is used.
  const auto left = build({range.first, split}, depth + 1);
  const auto right = build({split, range.last}, depth + 1);
  nodes_[node_id].left = left;
  nodes_[node_id].right = right;
  nodes_[node_id].escape = nodes_.size();
  counter_add(work_.escape_links);
  // Validate the DFS continuation when linking children, without another
  // point traversal: left follows parent, its escape is right, and the two
  // child populations partition the parent. The right escape leaves parent.
  // By induction every escape either advances to the next population range
  // or to the final index sentinel; it cannot skip/revisit a witness prefix.
  if (left != node_id + 1 || nodes_[left].escape != right ||
      nodes_[right].escape != nodes_[node_id].escape ||
      nodes_[left].range.first != range.first ||
      nodes_[left].range.last != nodes_[right].range.first ||
      nodes_[right].range.last != range.last) {
    throw std::logic_error("mhgp8 q2 index children or DFS escapes do not partition their parent");
  }
  return node_id;
}

Q2CensusIndexPtr make_q2_census_index(RectanglePtr rectangle) {
  if (!rectangle) {
    throw std::invalid_argument("mhgp8 q2 index adapter requires an owned rectangle");
  }
  return make_q2_cloud_index(rectangle->cloud_ptr());
}

Q2CensusIndexPtr make_q2_cloud_index(CloudPtr cloud) {
  return Q2CensusIndexPtr(new Q2CensusIndex(std::move(cloud)));
}

std::size_t Q2CensusIndex::retained_bytes() const {
  constexpr auto maximum = std::numeric_limits<std::size_t>::max();
  if (order_.capacity() > maximum / sizeof(std::size_t) ||
      nodes_.capacity() > maximum / sizeof(Node)) {
    throw std::overflow_error("mhgp8 q2 index capacity byte count overflow");
  }
  const auto orders = order_.capacity() * sizeof(std::size_t);
  const auto nodes = nodes_.capacity() * sizeof(Node);
  if (orders > maximum - nodes) {
    throw std::overflow_error("mhgp8 q2 index retained byte count overflow");
  }
  return orders + nodes;
}

struct Q2CensusEngine {
  struct QueryNode {
    Range range;
    Box3 box;
    std::size_t left{absent};
    std::size_t right{absent};
  };
  const Q2CensusIndex& index;
  const AxisQ2Plan& plan;
  const Q2CensusConsumer& consumer;
  std::span<const Point3> points;
  std::span<const std::size_t> b_order;
  unsigned threshold;
  Q2CensusResult result;
  std::vector<QueryNode> queries;
  std::vector<std::size_t> interior;
  std::vector<std::size_t> shell;

  Q2CensusEngine(const Q2CensusIndex& input_index, const AxisQ2Plan& input_plan,
                 const Q2CensusConsumer& input_consumer)
      : index(input_index), plan(input_plan), consumer(input_consumer),
        points(input_index.cloud().points()), b_order(input_plan.b_order()),
        threshold(input_plan.rectangle().kmax()) {
    result.candidate_pairs = plan.candidate_pairs();
    result.work.input_descriptors = static_cast<u64>(plan.blocks().size());
  }

  [[nodiscard]] std::size_t build_queries(Range range, u64 depth) {
    const auto id = queries.size();
    queries.push_back(QueryNode{range, {}});
    counter_add(result.work.query_build_nodes);
    result.work.query_build_max_depth = std::max(result.work.query_build_max_depth, depth);
    if (range.size() == 1) {
      counter_add(result.work.query_build_point_visits);
      queries[id].box = singleton_box(points[b_order[range.first]]);
      return id;
    }
    // Preserve the axis permutation and its descriptor ranges. Construct
    // boxes bottom-up in O(|B|), without rescanning B for every anchor.
    const auto split = range.first + range.size() / 2;
    const auto left = build_queries({range.first, split}, depth + 1);
    const auto right = build_queries({split, range.last}, depth + 1);
    queries[id].left = left;
    queries[id].right = right;
    queries[id].box = joined(queries[left].box, queries[right].box);
    return id;
  }

  void root_start(unsigned acquired) {
    counter_add(result.work.count_root_starts);
    if (acquired != 0) {
      counter_add(result.work.frontier_restarts);
      throw std::logic_error("mhgp8 q2 census cannot restart its root after a credit");
    }
  }

  void consume_witnesses(std::size_t population) {
    counter_add(result.work.consumed_witness_sites, static_cast<u64>(population));
  }

  void add_count(unsigned& count, std::size_t population) const {
    // Count stays <=Kmax<=10 for the certified owner. Compare before narrowing
    // even if the global witness population is many millions of sites.
    const auto remaining = static_cast<std::size_t>(threshold - count);
    count += static_cast<unsigned>(std::min(remaining, population));
  }

  void count_pair(std::size_t witness, const Q2BallKey& key, unsigned& count) {
    if (count == threshold) {
      return;
    }
    counter_add(result.work.count_node_visits);
    const auto& z = index.nodes_[witness];
    if (z.left == absent) {
      counter_add(result.work.count_point_tests);
      consume_witnesses(1);
      if (point_power4(key, points[index.order_[z.range.first]]) > 0) {
        add_count(count, 1);
      }
      return;
    }
    counter_add(result.work.count_bound_tests);
    const auto bounds = pair_bounds(key, z.box);
    if (bounds.minimum4 > 0) {
      consume_witnesses(z.range.size());
      add_count(count, z.range.size());
    } else if (bounds.maximum4 <= 0) {
      consume_witnesses(z.range.size());
    } else {
      counter_add(result.work.witness_splits);
      count_pair(z.left, key, count);
      count_pair(z.right, key, count);
    }
  }

  void collect(std::size_t witness, const Q2BallKey& key) {
    counter_add(result.work.payload_node_visits);
    const auto& z = index.nodes_[witness];
    if (z.left == absent) {
      counter_add(result.work.payload_point_tests);
      const auto id = index.order_[z.range.first];
      const auto value = point_power4(key, points[id]);
      if (value > 0) {
        interior.push_back(id);
      } else if (value == 0) {
        shell.push_back(id);
      }
      return;
    }
    counter_add(result.work.payload_bound_tests);
    const auto bounds = pair_bounds(key, z.box);
    if (bounds.maximum4 < 0) {
      return;
    }
    if (bounds.minimum4 > 0 || (bounds.minimum4 == 0 && bounds.maximum4 == 0)) {
      auto& destination = bounds.minimum4 > 0 ? interior : shell;
      const auto begin = index.order_.begin() + static_cast<std::ptrdiff_t>(z.range.first);
      const auto end = index.order_.begin() + static_cast<std::ptrdiff_t>(z.range.last);
      destination.insert(destination.end(), begin, end);
      return;
    }
    // Equality cannot be discarded in this second pass: every shell ID is
    // needed, including endpoints and non-support boundary sites.
    collect(z.left, key);
    collect(z.right, key);
  }

  void accept(std::size_t a_id, Range range, unsigned count) {
    if (count >= threshold) {
      throw std::logic_error("mhgp8 q2 accepted a saturated support");
    }
    counter_add(result.accepted_pairs, static_cast<u64>(range.size()));
    if (range.size() > 1) {
      counter_add(result.work.uniform_accepted_pairs, static_cast<u64>(range.size()));
    }
    const auto started = Clock::now();
    for (std::size_t position = range.first; position < range.last; ++position) {
      const auto b_id = b_order[position];
      const auto key = ball_key(points[a_id], points[b_id]);
      interior.clear();
      shell.clear();
      collect(0, key);
      if (interior.size() != count) {
        throw std::logic_error("mhgp8 q2 count and second-pass interior IDs disagree");
      }
      counter_add(result.work.payload_interior_sites, static_cast<u64>(interior.size()));
      counter_add(result.work.payload_shell_sites, static_cast<u64>(shell.size()));
      counter_add(result.work.payload_supports);
      consumer(Q2Support{a_id, b_id, key, interior, shell});
    }
    result.payload_ms += milliseconds(started, Clock::now());
  }

  void reject(Range range) {
    counter_add(result.rejected_pairs, static_cast<u64>(range.size()));
    if (range.size() > 1) {
      counter_add(result.work.uniform_rejected_pairs, static_cast<u64>(range.size()));
    }
  }

  void shared_task(std::size_t a_id, std::size_t query, unsigned count,
                    std::size_t cursor) {
    counter_add(result.work.query_tasks);
    const auto& b = queries[query];
    const bool singleton = b.range.size() == 1;
    const auto key = singleton ? ball_key(points[a_id], points[b_order[b.range.first]])
                               : Q2BallKey{};
    const auto b_diagonal = singleton ? 0 : squared_diagonal(b.box);
    // Prepare six endpoint constants once for this query task, not once for
    // every Z box. The 48-byte value has no heap storage or point views; the
    // existing singleton-pair path does not compute these constants.
    Q2PreparedBounds prepared;
    if (!singleton) {
      prepared = Q2PreparedBounds(points[a_id], b.box);
    }
    while (cursor != index.nodes_.size()) {
      if (cursor >= index.nodes_.size()) {
        throw std::logic_error("mhgp8 q2 witness cursor exceeds the immutable index");
      }
      const auto& z = index.nodes_[cursor];
      counter_add(result.work.count_node_visits);
      PowerBounds bounds;
      if (singleton && z.left == absent) {
        counter_add(result.work.count_point_tests);
        const auto value = point_power4(key, points[index.order_[z.range.first]]);
        bounds = {value, value};
      } else {
        counter_add(result.work.count_bound_tests);
        bounds = singleton ? pair_bounds(key, z.box) : prepared.bounds_unchecked(z.box);
      }
      if (bounds.minimum4 > 0) {
        consume_witnesses(z.range.size());
        if (!singleton) {
          counter_add(result.work.uniform_credited_pairs, static_cast<u64>(b.range.size()));
        }
        add_count(count, z.range.size());
        cursor = z.escape;
        counter_add(result.work.cursor_advances);
        if (count == threshold) {
          reject(b.range);
          return;
        }
      } else if (bounds.maximum4 <= 0) {
        consume_witnesses(z.range.size());
        cursor = z.escape;
        counter_add(result.work.cursor_advances);
      } else if (singleton ||
                 (z.left != absent && squared_diagonal(z.box) > b_diagonal)) {
        if (z.left == absent) {
          throw std::logic_error("mhgp8 q2 singleton power cannot be uncertain");
        }
        counter_add(result.work.witness_splits);
        cursor = z.left;
        counter_add(result.work.cursor_advances);
      } else {
        if (b.left == absent) {
          throw std::logic_error("mhgp8 q2 shared task has no query children");
        }
        counter_add(result.work.query_splits);
        if (count > 0) {
          counter_add(result.work.shared_splits_after_credit);
        }
        counter_add(result.work.cursor_reuses, 2);
        // A fixed Z DFS order makes one preorder cursor represent the entire
        // unconsumed suffix. Both query children inherit that cursor and the
        // exact acquired count. No frontier list, allocation, root restart,
        // or copy of a long continuation is needed. Query tasks may later be
        // scheduled independently, but their internal Z order must stay fixed.
        shared_task(a_id, b.left, count, cursor);
        shared_task(a_id, b.right, count, cursor);
        return;
      }
    }
    accept(a_id, b.range, count);
  }

  void cover(std::size_t a_id, Range selected, std::size_t query) {
    counter_add(result.work.query_cover_visits);
    const auto& node = queries[query];
    if (selected.last <= node.range.first || node.range.last <= selected.first) {
      return;
    }
    if (selected.first <= node.range.first && node.range.last <= selected.last) {
      root_start(0);
      shared_task(a_id, query, 0, 0);
      return;
    }
    if (node.left == absent) {
      throw std::logic_error("mhgp8 q2 descriptor partially intersects a singleton range");
    }
    cover(a_id, selected, node.left);
    cover(a_id, selected, node.right);
  }

  void run(Q2CensusMode mode) {
    if (result.candidate_pairs == 0) {
      return;
    }
    if (mode == Q2CensusMode::SharedBlocks) {
      const auto started = Clock::now();
      static_cast<void>(build_queries({0, b_order.size()}, 0));
      result.query_index_ms = milliseconds(started, Clock::now());
      for (const auto& block : plan.blocks()) {
        cover(block.a_id, block.b, 0);
      }
    } else {
      for (const auto& block : plan.blocks()) {
        for (std::size_t position = block.b.first; position < block.b.last; ++position) {
          counter_add(result.work.query_tasks);
          root_start(0);
          unsigned count = 0;
          const auto key = ball_key(points[block.a_id], points[b_order[position]]);
          count_pair(0, key, count);
          const Range singleton{position, position + 1};
          if (count == threshold) {
            reject(singleton);
          } else {
            accept(block.a_id, singleton, count);
          }
        }
      }
    }
  }
};

Q2CensusResult run_q2_census(const Q2CensusIndex& index, const AxisQ2Plan& plan,
                            Q2CensusMode mode, const Q2CensusConsumer& consumer) {
  const auto started = Clock::now();
  if (&index.cloud() != &plan.rectangle().cloud()) {
    throw std::invalid_argument("mhgp8 q2 index and plan require the same cloud");
  }
  if (mode != Q2CensusMode::Pairwise && mode != Q2CensusMode::SharedBlocks) {
    throw std::invalid_argument("mhgp8 q2 census mode is invalid");
  }
  if (!consumer) {
    throw std::invalid_argument("mhgp8 q2 census requires a synchronous payload consumer");
  }
  Q2CensusResult result;
  {
    Q2CensusEngine engine(index, plan, consumer);
    engine.run(mode);
    if (engine.result.accepted_pairs > engine.result.candidate_pairs ||
        engine.result.rejected_pairs != engine.result.candidate_pairs - engine.result.accepted_pairs) {
      throw std::logic_error("mhgp8 q2 census did not partition the candidate residual");
    }
    result = engine.result;
  }
  // Include destruction of the temporary B tree and payload buffers
  // in the enclosing interval, rather than reporting only active searches.
  result.total_ms = milliseconds(started, Clock::now());
  result.count_ms = result.total_ms - result.query_index_ms - result.payload_ms;
  return result;
}

}  // namespace mhgp8
