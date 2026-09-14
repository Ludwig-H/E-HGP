#include "q2_census.hpp"
#include "q2_census_resume.hpp"
#include "wspd_q2_census.hpp"
#include "wspd_q2_parallel.hpp"
#include "q2_joint_bounds.hpp"
#include "q2_node_pool.hpp"
#include "../spindle/q2_prepared_bounds.hpp"
#include "../parallel/joined_workers.hpp"
#include "../parallel/work_reduction.hpp"

#include <algorithm>
#include <atomic>
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
  struct OrderContext {
    std::size_t deferred;
    std::size_t escape;
    std::size_t anchor_rank;
  };
  const Q2CensusIndex& index;
  const Q2CensusConsumer& consumer;
  std::span<const Point3> points;
  std::span<const std::size_t> b_order;
  unsigned threshold;
  Q2CensusResult result;
  Q2SiblingWork sibling_work;
  Q2OrderWork order_work;
  Q2JointWork joint_work;
  std::vector<QueryNode> queries;
  std::span<const Q2SpatialNode> shared_queries;
  std::vector<std::size_t> interior;
  std::vector<std::size_t> shell;

  Q2CensusEngine(const Q2CensusIndex& input_index, const AxisQ2Plan& input_plan,
                 const Q2CensusConsumer& input_consumer)
      : index(input_index), consumer(input_consumer),
        points(input_index.cloud().points()), b_order(input_plan.b_order()),
        threshold(input_plan.rectangle().kmax()) {
    result.candidate_pairs = input_plan.candidate_pairs();
    result.work.input_descriptors = static_cast<u64>(input_plan.blocks().size());
  }

  Q2CensusEngine(const Q2CensusIndex& input_index, unsigned kmax,
                 const Q2CensusConsumer& input_consumer)
      : index(input_index), consumer(input_consumer), points(input_index.cloud().points()),
        b_order(input_index.spatial_order()), threshold(kmax),
        shared_queries(input_index.spatial_nodes()) {}

  // Owned continuations use the same trusted-box authority as shared_task.
  // These helpers add no new operation to the historical execution paths.
  [[nodiscard]] static Q2PreparedBounds inert_resume_bounds() { return {}; }
  [[nodiscard]] Q2Bounds resume_bounds(const Q2PreparedBounds& prepared, std::size_t node) const {
    return prepared.bounds_unchecked(index.nodes_[node].box);
  }

  [[nodiscard]] QueryNode query_node(std::size_t id) const {
    if (!shared_queries.empty()) {
      const auto& node = shared_queries[id];
      return {node.range, node.box, node.left, node.right};
    }
    return queries[id];
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

  template<bool sibling_certificate = false, bool complement_first = false>
  void shared_task(std::size_t a_id, std::size_t query, unsigned count,
                    std::size_t cursor, std::size_t sibling = absent,
                    const OrderContext* context = nullptr, bool inside_deferred = false) {
    counter_add(result.work.query_tasks);
    const auto b = query_node(query);
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
    if constexpr (sibling_certificate) {
      if (sibling != absent) {
        counter_add(sibling_work.proposals);
        const auto& witness = index.nodes_[sibling];
        if (witness.range.size() < threshold) {
          counter_add(sibling_work.cardinality_skips);
        } else {
          counter_add(sibling_work.bound_tests);
          const auto bounds = singleton ? pair_bounds(key, witness.box)
                                        : prepared.bounds_unchecked(witness.box);
          // This population is an independent saturation certificate, NOT
          // a new disjoint credit. It may overlap the already consumed Z
          // prefix. Neither count nor cursor is ever modified by this test.
          if (bounds.minimum4 > 0) {
            counter_add(sibling_work.rejected_tasks);
            counter_add(sibling_work.rejected_pairs, static_cast<u64>(b.range.size()));
            if (count != 0) counter_add(sibling_work.rejected_after_credit);
            reject(b.range);
            return;
          }
        }
      }
    }
    while (true) {
      if constexpr (complement_first) {
        // context is created by this integrated root and borrowed throughout
        // its synchronous descendants. It never changes to the current B.
        if (inside_deferred) {
          if (cursor == context->escape) break;
        } else if (cursor == index.nodes_.size()) {
          inside_deferred = true;
          cursor = context->deferred;
          counter_add(order_work.phase_switches);
          counter_add(result.work.cursor_advances);
        }
      } else if (cursor == index.nodes_.size()) {
        break;
      }
      if (cursor >= index.nodes_.size()) {
        throw std::logic_error("mhgp8 q2 witness cursor exceeds the immutable index");
      }
      const auto& z = index.nodes_[cursor];
      if constexpr (complement_first) {
        if (!inside_deferred && cursor == context->deferred) {
          // Defer, do not consume: every site remains available in phase two.
          cursor = context->escape;
          counter_add(order_work.deferred_skips);
          counter_add(result.work.cursor_advances);
          continue;
        }
        const bool contains_anchor = z.range.first <= context->anchor_rank &&
                                     context->anchor_rank < z.range.last;
        const bool contains_deferred = !inside_deferred && cursor < context->deferred &&
                                       context->deferred < z.escape;
        if (contains_anchor || contains_deferred) {
          // These topological exclusions precede ALL geometric decisions.
          // Consuming an ancestor first could include a deferred population,
          // even if a later, narrower query made that ancestor uniform.
          if (z.left == absent) {
            // Only the anchor can be a leaf here: H(a,b,a)=0 for every b.
            cursor = z.escape;
            consume_witnesses(1);
            counter_add(order_work.anchor_skips);
          } else {
            cursor = z.left;
            counter_add(order_work.structural_splits);
          }
          counter_add(result.work.cursor_advances);
          continue;
        }
      }
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
        shared_task<sibling_certificate, complement_first>(a_id, b.left, count, cursor,
                                                          b.right, context, inside_deferred);
        shared_task<sibling_certificate, complement_first>(a_id, b.right, count, cursor,
                                                          b.left, context, inside_deferred);
        return;
      }
    }
    accept(a_id, b.range, count);
  }

  template<bool sibling_certificate, bool complement_first, bool split_only_a = false>
  void joint_task(std::size_t a_node, std::size_t b_node, unsigned count,
                  std::size_t cursor, std::size_t deferred, std::size_t escape,
                  bool inside_deferred, u64 depth) {
    counter_add(joint_work.tasks);
    joint_work.max_depth = std::max(joint_work.max_depth, depth);
    const auto& a = index.nodes_[a_node];
    const auto& b = index.nodes_[b_node];
    const auto wide_mass = static_cast<i128>(a.range.size()) * b.range.size();
    if (wide_mass > std::numeric_limits<u64>::max())
      throw std::overflow_error("mhgp8 joint query mass exceeds u64");
    const auto mass = static_cast<u64>(wide_mass);
    if (a.left == absent) {
      counter_add(joint_work.singleton_handoffs);
      counter_add(joint_work.handoff_pair_mass, mass);
      if (count != 0) counter_add(joint_work.handoffs_after_credit);
      const OrderContext context{deferred, escape, a.range.first};
      // The prefix is already resolved uniformly. Removing this anchor
      // only removes a known-zero contribution; never restart the count.
      shared_task<sibling_certificate, complement_first>(index.order_[a.range.first],
          b_node, count, cursor, absent, &context, inside_deferred);
      return;
    }
    const Q2JointPreparedBounds prepared(a.box, b.box);
    const auto a_diagonal = squared_diagonal(a.box);
    const auto b_diagonal = squared_diagonal(b.box);
    while (true) {
      if constexpr (complement_first) {
        if (inside_deferred) {
          if (cursor == escape) break;
        } else if (cursor == index.nodes_.size()) {
          inside_deferred = true;
          cursor = deferred;
          counter_add(joint_work.phase_switches);
          counter_add(joint_work.cursor_advances);
        }
      } else if (cursor == index.nodes_.size()) {
        break;
      }
      if (cursor >= index.nodes_.size())
        throw std::logic_error("mhgp8 joint witness cursor exceeds the immutable index");
      const auto& z = index.nodes_[cursor];
      if constexpr (complement_first) {
        if (!inside_deferred && cursor == deferred) {
          cursor = escape;
          counter_add(joint_work.deferred_skips);
          counter_add(joint_work.cursor_advances);
          continue;
        }
        if (!inside_deferred && cursor < deferred && deferred < z.escape) {
          cursor = z.left;
          counter_add(joint_work.structural_splits);
          counter_add(joint_work.cursor_advances);
          continue;
        }
      }
      counter_add(joint_work.bound_tests);
      const auto bounds = prepared.bounds_unchecked(z.box);
      if (bounds.minimum4 > 0) {
        counter_add(joint_work.credit_events);
        counter_add(joint_work.credited_pair_mass, mass);
        counter_add(joint_work.consumed_witness_sites, static_cast<u64>(z.range.size()));
        add_count(count, z.range.size());
        cursor = z.escape;
        counter_add(joint_work.cursor_advances);
        if (count == threshold) {
          counter_add(joint_work.rejected_pairs, mass);
          counter_add(result.rejected_pairs, mass);
          counter_add(result.work.uniform_rejected_pairs, mass);
          return;
        }
      } else if (bounds.maximum4 <= 0) {
        counter_add(joint_work.consumed_witness_sites, static_cast<u64>(z.range.size()));
        cursor = z.escape;
        counter_add(joint_work.cursor_advances);
      } else if (z.left != absent &&
                 squared_diagonal(z.box) > std::max(a_diagonal, b_diagonal)) {
        cursor = z.left;
        counter_add(joint_work.witness_splits);
        counter_add(joint_work.cursor_advances);
      } else {
        if (count != 0) counter_add(joint_work.splits_after_credit);
        if (split_only_a || b.left == absent || a_diagonal >= b_diagonal) {
          counter_add(joint_work.splits_a);
          joint_task<sibling_certificate, complement_first, split_only_a>(a.left, b_node, count, cursor,
              deferred, escape, inside_deferred, depth + 1);
          joint_task<sibling_certificate, complement_first, split_only_a>(a.right, b_node, count, cursor,
              deferred, escape, inside_deferred, depth + 1);
        } else {
          counter_add(joint_work.splits_b);
          joint_task<sibling_certificate, complement_first, split_only_a>(a_node, b.left, count, cursor,
              deferred, escape, inside_deferred, depth + 1);
          joint_task<sibling_certificate, complement_first, split_only_a>(a_node, b.right, count, cursor,
              deferred, escape, inside_deferred, depth + 1);
        }
        return;
      }
    }
    counter_add(joint_work.accepted_pairs, mass);
    for (auto rank = a.range.first; rank < a.range.last; ++rank)
      accept(index.order_[rank], b.range, count);
  }

  template<bool sibling_certificate, bool complement_first>
  void joint_root(std::size_t a, std::size_t b, Q2AnchorMode anchor_mode) {
    if (anchor_mode == Q2AnchorMode::SharedAnchors)
      joint_task<sibling_certificate, complement_first, true>(a, b, 0, 0, b, index.nodes_[b].escape, false, 0);
    else
      joint_task<sibling_certificate, complement_first, false>(a, b, 0, 0, b, index.nodes_[b].escape, false, 0);
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

  void pair_task(std::size_t a_id, std::size_t position) {
    counter_add(result.work.query_tasks);
    root_start(0);
    unsigned count = 0;
    const auto key = ball_key(points[a_id], points[b_order[position]]);
    count_pair(0, key, count);
    const Range singleton{position, position + 1};
    if (count == threshold) reject(singleton);
    else accept(a_id, singleton, count);
  }

  // Explicit product adaptation of the audited fbbecc01 Pool/pairs bridge.
  // The local B permutation only names pair queries; Z never changes. The
  // global query-node view is NOT usable with this temporary permutation.
  // Keep this route synchronous and pair-only until a separate query-tree
  // contract is implemented. Restoring the view also covers callback failure.
  [[nodiscard]] bool terminal_pool(std::size_t a_node, std::size_t b_node, Q2PoolWork& work) {
    const auto started = Clock::now();
    const auto a = index.nodes_[a_node].range;
    const auto b = index.nodes_[b_node].range;
    counter_add(work.selected_rectangles);
    counter_add(work.original_selected_anchors, static_cast<u64>(a.size()));
    counter_add(work.factor_sites, static_cast<u64>(a.size()));
    counter_add(work.factor_sites, static_cast<u64>(b.size()));
    {
      const detail::Q2NodePoolPlan plan(index, a_node, b_node, threshold);
      work.preparation_ms += milliseconds(started, Clock::now());
      counter_add(work.selected_pairs, plan.total_pairs());
      counter_add(work.residual_pairs, plan.candidate_pairs());
      counter_add(work.filtered_pairs, plan.total_pairs() - plan.candidate_pairs());
      const auto& pw = plan.work();
      counter_add(work.selection_tests, pw.selection_tests);
      counter_add(work.witness_attempts, pw.witness_attempts);
      counter_add(work.universal_queries, pw.predicates.universal_queries);
      counter_add(work.q2_axis_terms, pw.predicates.q2_axis_terms);
      counter_add(work.pool_selected, pw.pool_selected);
      counter_add(work.pool_insertions, pw.pool_insertions);
      counter_add(work.pool_shifted_entries, pw.pool_shifted_entries);
      counter_add(work.prefix_class_visits, pw.prefix_class_visits);
      counter_add(work.factor_read_visits, pw.selection_point_visits);
      counter_add(work.factor_read_visits, pw.certification_anchor_visits);
      counter_add(work.grouping_visits, pw.group_credit_visits);
      counter_add(work.grouping_visits, pw.group_scatter_visits);
      work.plan_peak_bytes = std::max(work.plan_peak_bytes, static_cast<u64>(plan.retained_bytes()));
      if (plan.candidate_pairs() == plan.total_pairs()) {
        // No reduction must not replace a useful Shared/sibling/joint route
        // by the full pair expansion (notably the parallel-row fixture).
        counter_add(work.passthrough_rectangles);
        counter_add(work.passthrough_pairs, plan.total_pairs());
        counter_add(work.passthrough_anchors, static_cast<u64>(a.size()));
        return false;
      }
      counter_add(result.candidate_pairs, plan.candidate_pairs());
      const auto saved_order = b_order;
      b_order = plan.b_order();
      try {
        for (unsigned credit = 0; credit < threshold; ++credit) {
          const auto group = plan.a_groups()[credit];
          const auto prefix = plan.prefix_for_credit(credit);
          if (group.size() == 0 || prefix == 0) continue;
          counter_add(work.bands);
          for (auto position = group.first; position < group.last; ++position) {
            counter_add(work.selected_anchors);
            const auto a_id = index.order_[plan.a_ranks()[position]];
            for (std::size_t j = 0; j < prefix; ++j) {
              counter_add(work.pair_roots);
              pair_task(a_id, j);  // Count zero, all of the original global Z.
            }
          }
        }
      } catch (...) {
        b_order = saved_order;
        throw;
      }
      b_order = saved_order;
    }
    return true;
  }

  void run(const AxisQ2Plan& plan, Q2CensusMode mode) {
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
          pair_task(block.a_id, position);
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
    engine.run(plan, mode);
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

namespace {

void consume_wspd_rectangle(Q2CensusEngine& engine, WspdQ2CensusResult& result,
    std::span<const Q2SpatialNode> nodes, std::span<const std::size_t> order,
    const WspdRectangle& rectangle, Q2CensusMode census_mode,
    Q2SiblingMode sibling_mode, Q2WitnessOrder witness_order,
    Q2AnchorMode anchor_mode, std::size_t pool_min_factor) {
  if (rectangle.lane_mask != 1) {
    throw std::logic_error("mhgp8 integrated census received a non-q2 lane");
  }
  auto a_node = rectangle.a_node;
  auto b_node = rectangle.b_node;
  if (nodes[a_node].range.size() > nodes[b_node].range.size()) std::swap(a_node, b_node);
  const auto a = nodes[a_node].range;
  const auto b = nodes[b_node].range;
  counter_add(result.input_rectangles);
  counter_add(result.anchor_queries, static_cast<u64>(a.size()));
  counter_add(engine.result.work.input_descriptors);
  const auto mass = static_cast<i128>(a.size()) * b.size();
  if (mass > std::numeric_limits<u64>::max()) {
    throw std::overflow_error("mhgp8 integrated q2 pair mass exceeds u64");
  }
  const auto consume_unfiltered = [&] {
    counter_add(engine.result.candidate_pairs, static_cast<u64>(mass));
    if (anchor_mode != Q2AnchorMode::Individual) {
      counter_add(engine.joint_work.root_products);
      engine.root_start(0);
      if (witness_order == Q2WitnessOrder::ComplementFirst) {
        if (sibling_mode == Q2SiblingMode::Saturating)
          engine.joint_root<true, true>(a_node, b_node, anchor_mode);
        else
          engine.joint_root<false, true>(a_node, b_node, anchor_mode);
      } else {
        if (sibling_mode == Q2SiblingMode::Saturating)
          engine.joint_root<true, false>(a_node, b_node, anchor_mode);
        else
          engine.joint_root<false, false>(a_node, b_node, anchor_mode);
      }
      return;
    }
    for (auto rank = a.first; rank < a.last; ++rank) {
      const auto a_id = order[rank];
      if (census_mode == Q2CensusMode::SharedBlocks) {
        engine.root_start(0);
        if (witness_order == Q2WitnessOrder::ComplementFirst) {
          const Q2CensusEngine::OrderContext context{b_node, nodes[b_node].escape, rank};
          if (sibling_mode == Q2SiblingMode::Saturating) {
            engine.shared_task<true, true>(a_id, b_node, 0, 0, absent, &context);
          } else {
            engine.shared_task<false, true>(a_id, b_node, 0, 0, absent, &context);
          }
        } else if (sibling_mode == Q2SiblingMode::Saturating) {
          engine.shared_task<true>(a_id, b_node, 0, 0);
        } else {
          engine.shared_task(a_id, b_node, 0, 0);
        }
      } else {
        for (auto position = b.first; position < b.last; ++position) engine.pair_task(a_id, position);
      }
    }
  };
  if (pool_min_factor != 0 && b.size() >= pool_min_factor) {
    const auto pool_started = Clock::now();
    if (!engine.terminal_pool(a_node, b_node, result.pool_work)) consume_unfiltered();
    result.pool_work.selected_total_ms += milliseconds(pool_started, Clock::now());
  } else {
    consume_unfiltered();
  }
}

}  // namespace

namespace {

void validate_integrated_modes(Q2CensusMode census_mode, const Q2CensusConsumer& consumer,
    Q2SiblingMode sibling_mode, Q2WitnessOrder witness_order, Q2AnchorMode anchor_mode) {
  if (census_mode != Q2CensusMode::Pairwise && census_mode != Q2CensusMode::SharedBlocks) {
    throw std::invalid_argument("mhgp8 integrated q2 census mode is invalid");
  }
  if ((sibling_mode != Q2SiblingMode::Disabled && sibling_mode != Q2SiblingMode::Saturating) ||
      (sibling_mode == Q2SiblingMode::Saturating && census_mode != Q2CensusMode::SharedBlocks)) {
    throw std::invalid_argument("mhgp8 sibling certificate requires a valid SharedBlocks mode");
  }
  if ((witness_order != Q2WitnessOrder::GlobalDfs && witness_order != Q2WitnessOrder::ComplementFirst) ||
      (witness_order == Q2WitnessOrder::ComplementFirst && census_mode != Q2CensusMode::SharedBlocks)) {
    throw std::invalid_argument("mhgp8 q2 witness reordering requires a valid SharedBlocks mode");
  }
  if (!consumer) {
    throw std::invalid_argument("mhgp8 integrated q2 census requires a payload consumer");
  }
  if ((anchor_mode != Q2AnchorMode::Individual && anchor_mode != Q2AnchorMode::SharedProduct &&
       anchor_mode != Q2AnchorMode::SharedAnchors) ||
      (anchor_mode != Q2AnchorMode::Individual && census_mode != Q2CensusMode::SharedBlocks)) {
    throw std::invalid_argument("mhgp8 joint q2 census requires a valid SharedBlocks mode");
  }
}

}  // namespace

WspdQ2CensusResult run_wspd_q2_census(
    const Q2CensusIndex& index, unsigned kmax, unsigned separation_s,
    WspdFrontMode front_mode, Q2CensusMode census_mode,
    const Q2CensusConsumer& consumer, Q2SiblingMode sibling_mode,
    Q2WitnessOrder witness_order, Q2AnchorMode anchor_mode, std::size_t pool_min_factor) {
  const auto started = Clock::now();
  validate_integrated_modes(census_mode, consumer, sibling_mode, witness_order, anchor_mode);
  WspdQ2CensusResult result;
  {
    Q2CensusEngine engine(index, kmax, consumer);
    const auto nodes = index.spatial_nodes();
    const auto order = index.spatial_order();
    // The producer validates K/s/mode before its first callback and emits
    // handles from this exact index. No arbitrary externally forged handles
    // are adopted, and no global or factor validation is repeated here.
    result.front = run_wspd_front(index, kmax, separation_s, front_mode,
        [&](const WspdRectangle& rectangle) {
          consume_wspd_rectangle(engine, result, nodes, order, rectangle, census_mode,
                                 sibling_mode, witness_order, anchor_mode, pool_min_factor);
        }, 1);
    result.census = engine.result;
    result.sibling_work = engine.sibling_work;
    result.order_work = engine.order_work;
    result.joint_work = engine.joint_work;
    const auto& pool = result.pool_work;
    const auto front_mass = result.front.work.residual_pair_mass[0];
    if (pool.selected_pairs > front_mass || pool.filtered_pairs > pool.selected_pairs ||
        pool.residual_pairs != pool.selected_pairs - pool.filtered_pairs ||
        pool.passthrough_pairs > pool.residual_pairs ||
        pool.pair_roots != pool.residual_pairs - pool.passthrough_pairs ||
        pool.pair_roots > result.census.candidate_pairs)
      throw std::logic_error("mhgp8 Pool census lost its selected partition");
    if (anchor_mode != Q2AnchorMode::Individual) {
      const auto candidates = result.census.candidate_pairs - pool.pair_roots;
      const auto& joint = result.joint_work;
      if (joint.rejected_pairs > candidates ||
          joint.accepted_pairs > candidates - joint.rejected_pairs ||
          joint.handoff_pair_mass != candidates - joint.rejected_pairs - joint.accepted_pairs)
        throw std::logic_error("mhgp8 joint q2 census lost its terminal mass partition");
    }
    if (result.census.candidate_pairs != front_mass - pool.filtered_pairs ||
        result.input_rectangles != result.front.work.emitted_rectangles ||
        result.census.accepted_pairs > result.census.candidate_pairs ||
        result.census.rejected_pairs != result.census.candidate_pairs - result.census.accepted_pairs) {
      throw std::logic_error("mhgp8 integrated q2 census lost the residual partition");
    }
  }
  result.total_ms = milliseconds(started, Clock::now());
  result.census.total_ms = result.total_ms;
  result.census.count_ms = result.total_ms - result.census.payload_ms;
  return result;
}

WspdQ2ParallelResult run_wspd_q2_census_parallel(
    Q2CensusIndexPtr index, unsigned kmax, unsigned separation_s,
    WspdFrontMode front_mode, Q2CensusMode census_mode,
    std::span<const Q2CensusConsumer> consumers, std::size_t jobs_per_worker,
    Q2SiblingMode sibling_mode, Q2WitnessOrder witness_order,
    Q2AnchorMode anchor_mode, std::size_t pool_min_factor, WspdQ2Schedule schedule) {
  const auto started = Clock::now();
  if (!index || consumers.empty() || jobs_per_worker == 0) {
    throw std::invalid_argument("mhgp8 parallel q2 requires index, workers and positive job granularity");
  }
  if ((schedule.mode != WspdQ2ScheduleMode::Coarse &&
       schedule.mode != WspdQ2ScheduleMode::Donate) ||
      schedule.queue_capacity == 0 || schedule.donation_interval == 0) {
    throw std::invalid_argument("mhgp8 parallel q2 invalid front schedule");
  }
  if (consumers.size() > std::numeric_limits<std::size_t>::max() / jobs_per_worker) {
    throw std::overflow_error("mhgp8 parallel q2 target job count overflow");
  }
  for (const auto& consumer : consumers) {
    validate_integrated_modes(census_mode, consumer, sibling_mode, witness_order, anchor_mode);
  }
  WspdQ2ParallelResult result;
  result.requested_workers = static_cast<u64>(consumers.size());
  result.target_jobs = static_cast<u64>(consumers.size() * jobs_per_worker);
  {
    // Copy callbacks before any worker starts; retaining the exact index
    // makes reset of the caller's shared_ptr harmless during a callback.
    const std::vector<Q2CensusConsumer> callbacks(consumers.begin(), consumers.end());
    const auto partition_started = Clock::now();
    const auto plan = make_wspd_front_jobs(index, kmax, separation_s, front_mode,
                                         static_cast<std::size_t>(result.target_jobs), 1);
    result.partition_ms = milliseconds(partition_started, Clock::now());
    result.front = plan->prefix_result();
    result.prefix_product_visits = result.front.work.product_visits;
    result.jobs = static_cast<u64>(plan->job_count());
    result.terminal_jobs = static_cast<u64>(plan->terminal_job_count());
    result.job_storage_bytes = static_cast<u64>(plan->retained_bytes());
    const auto worker_count = std::min(callbacks.size(), plan->job_count());
    result.started_workers = static_cast<u64>(worker_count);
    const auto dispatch = schedule.mode == WspdQ2ScheduleMode::Donate && worker_count != 0
        ? plan->make_dispatch(schedule.queue_capacity, schedule.donation_interval, worker_count)
        : nullptr;
    if (dispatch) result.queue_storage_bytes = static_cast<u64>(dispatch->retained_bytes());
    struct alignas(64) WorkerState {
      WspdQ2CensusResult result;
      Q2ParallelWorkerStats stats;
    };
    std::vector<WorkerState> states(worker_count);
    std::atomic<std::size_t> next{0};
    const auto nodes = index->spatial_nodes();
    const auto order = index->spatial_order();
    parallel_detail::run_joined_workers(worker_count,
        [&](std::size_t worker, const std::atomic<bool>& cancel) {
          const auto worker_started = Clock::now();
          auto& state = states[worker];
          {
            Q2CensusEngine engine(*index, kmax, callbacks[worker]);
            const WspdRectangleConsumer receiver = [&](const WspdRectangle& rectangle) {
              consume_wspd_rectangle(engine, state.result, nodes, order, rectangle,
                                     census_mode, sibling_mode, witness_order,
                                     anchor_mode, pool_min_factor);
            };
            if (dispatch) {
              const auto part = dispatch->run_worker(receiver, cancel);
              parallel_detail::merge_work(state.result.front.work, part.front.work);
              state.stats.dispatch_work = part.work;
              state.stats.jobs = part.work.seeds_completed;
            } else {
              while (!cancel.load(std::memory_order_relaxed)) {
                auto job = next.load(std::memory_order_relaxed);
                while (job < plan->job_count() &&
                       !next.compare_exchange_weak(job, job + 1, std::memory_order_relaxed)) {}
                if (job == plan->job_count()) break;
                const auto part = plan->run_job(job, receiver);
                parallel_detail::merge_work(state.result.front.work, part.work);
                counter_add(state.stats.jobs);  // Completed, not merely claimed.
              }
            }
            state.result.census = engine.result;
            state.result.sibling_work = engine.sibling_work;
            state.result.order_work = engine.order_work;
            state.result.joint_work = engine.joint_work;
          }  // Include destruction of this worker's private payload buffers.
          state.stats.front_products = state.result.front.work.product_visits;
          state.stats.input_rectangles = state.result.input_rectangles;
          state.stats.count_node_visits = state.result.census.work.count_node_visits;
          state.stats.supports = state.result.census.accepted_pairs;
          state.stats.pool_peak_bytes = state.result.pool_work.plan_peak_bytes;
          state.stats.payload_ms = state.result.census.payload_ms;
          state.stats.elapsed_ms = milliseconds(worker_started, Clock::now());
        }, parallel_detail::ThreadLauncher{}, [&]() noexcept {
          if (dispatch) dispatch->cancel();
        });
    // No worker is alive during reduction, even on a callback/launch failure.
    result.workers.reserve(states.size());
    for (const auto& state : states) {
      parallel_detail::merge_work(result.front.work, state.result.front.work);
      parallel_detail::merge_work(result.census_work, state.result.census.work);
      parallel_detail::merge_work(result.sibling_work, state.result.sibling_work);
      parallel_detail::merge_work(result.order_work, state.result.order_work);
      parallel_detail::merge_work(result.joint_work, state.result.joint_work);
      parallel_detail::merge_work(result.pool_work, state.result.pool_work);
      parallel_detail::merge_work(result.dispatch_work, state.stats.dispatch_work);
      counter_add(result.input_rectangles, state.result.input_rectangles);
      counter_add(result.anchor_queries, state.result.anchor_queries);
      counter_add(result.candidate_pairs, state.result.census.candidate_pairs);
      counter_add(result.accepted_pairs, state.result.census.accepted_pairs);
      counter_add(result.rejected_pairs, state.result.census.rejected_pairs);
      counter_add(result.completed_jobs, state.stats.jobs);
      counter_add(result.pool_peak_bytes_sum, state.stats.pool_peak_bytes);
      result.worker_ms_sum += state.stats.elapsed_ms;
      result.payload_ms_sum += state.stats.payload_ms;
      result.workers.push_back(state.stats);
    }
  }  // Plan, callback copies and per-worker accumulation buffers are dead.
  const auto front_mass = result.front.work.residual_pair_mass[0];
  const auto& pool = result.pool_work;
  if (schedule.mode == WspdQ2ScheduleMode::Donate &&
      (result.dispatch_work.seeds_started != result.jobs ||
       result.dispatch_work.seeds_completed != result.jobs ||
       result.dispatch_work.donations != result.dispatch_work.stolen_started ||
       result.dispatch_work.donations != result.dispatch_work.stolen_completed)) {
    throw std::logic_error("mhgp8 parallel q2 lost its donated work partition");
  }
  if (result.completed_jobs != result.jobs || result.front.active_lane_mask != 1 ||
      result.front.work.rejected_pair_mass[0] > result.front.total_unordered_pairs ||
      front_mass != result.front.total_unordered_pairs - result.front.work.rejected_pair_mass[0] ||
      pool.selected_pairs > front_mass || pool.filtered_pairs > pool.selected_pairs ||
      pool.residual_pairs != pool.selected_pairs - pool.filtered_pairs ||
      pool.passthrough_pairs > pool.residual_pairs ||
      pool.pair_roots != pool.residual_pairs - pool.passthrough_pairs ||
      pool.pair_roots > result.candidate_pairs ||
      result.candidate_pairs != front_mass - pool.filtered_pairs ||
      result.input_rectangles != result.front.work.emitted_rectangles ||
      result.census_work.input_descriptors != result.input_rectangles ||
      result.accepted_pairs > result.candidate_pairs ||
      result.rejected_pairs != result.candidate_pairs - result.accepted_pairs ||
      result.census_work.payload_supports != result.accepted_pairs) {
    throw std::logic_error("mhgp8 parallel q2 lost its job or residual partition");
  }
  if (anchor_mode != Q2AnchorMode::Individual) {
    const auto candidates = result.candidate_pairs - pool.pair_roots;
    const auto& joint = result.joint_work;
    if (joint.rejected_pairs > candidates || joint.accepted_pairs > candidates - joint.rejected_pairs ||
        joint.handoff_pair_mass != candidates - joint.rejected_pairs - joint.accepted_pairs)
      throw std::logic_error("mhgp8 parallel joint census lost its terminal partition");
  }
  result.total_ms = milliseconds(started, Clock::now());
  return result;
}

namespace {

void validate_anchor_resume(const Q2CensusIndexPtr& index, std::size_t anchor_rank,
                            std::size_t b_node, unsigned kmax,
                            Q2SiblingMode sibling, Q2WitnessOrder order) {
  if (!index || kmax == 0 || kmax > 10 ||
      (sibling != Q2SiblingMode::Disabled && sibling != Q2SiblingMode::Saturating) ||
      (order != Q2WitnessOrder::GlobalDfs && order != Q2WitnessOrder::ComplementFirst)) {
    throw std::invalid_argument("mhgp8 q2 anchor requires an owned index, K1..10 and valid modes");
  }
  const auto nodes = index->spatial_nodes();
  if (anchor_rank >= index->spatial_order().size() || b_node >= nodes.size()) {
    throw std::invalid_argument("mhgp8 q2 anchor requires a global rank and node in this index");
  }
  const auto b = nodes[b_node].range;
  if (b.size() == 0 || (b.first <= anchor_rank && anchor_rank < b.last)) {
    throw std::invalid_argument("mhgp8 q2 anchor must be disjoint from its nonempty B node");
  }
}

void initialize_anchor_result(Q2CensusEngine& engine, std::size_t b_node) {
  engine.result.candidate_pairs = static_cast<u64>(engine.index.spatial_nodes()[b_node].range.size());
  engine.result.work.input_descriptors = 1;
  engine.root_start(0);
}

void check_completed_anchor(const Q2CensusResult& result) {
  if (result.accepted_pairs > result.candidate_pairs ||
      result.rejected_pairs != result.candidate_pairs - result.accepted_pairs ||
      result.work.payload_supports != result.accepted_pairs) {
    throw std::logic_error("mhgp8 q2 anchor did not complete its candidate and payload partition");
  }
}

// This is an actual exclusive acquisition, not a racy observation followed
// by an unprotected read. The public contract still requires single-owner
// access and synchronization when handing the continuation to another thread.
class ExclusiveResumeCall {
 public:
  explicit ExclusiveResumeCall(std::atomic_flag& flag) : flag_(flag) {
    if (flag_.test_and_set(std::memory_order_acquire))
      throw std::logic_error("mhgp8 q2 continuation call overlaps another call");
  }
  ~ExclusiveResumeCall() { flag_.clear(std::memory_order_release); }
  ExclusiveResumeCall(const ExclusiveResumeCall&) = delete;
  ExclusiveResumeCall& operator=(const ExclusiveResumeCall&) = delete;

 private:
  std::atomic_flag& flag_;
};

std::size_t resume_bytes(std::size_t capacity, std::size_t width) {
  if (capacity > std::numeric_limits<std::size_t>::max() / width)
    throw std::overflow_error("mhgp8 q2 continuation retained bytes exceed size_t");
  return capacity * width;
}

std::size_t resume_byte_sum(std::size_t a, std::size_t b) {
  if (b > std::numeric_limits<std::size_t>::max() - a)
    throw std::overflow_error("mhgp8 q2 continuation retained byte sum exceeds size_t");
  return a + b;
}

}  // namespace

Q2CensusAnchorResult run_q2_anchor_reference(
    Q2CensusIndexPtr index, std::size_t anchor_rank, std::size_t b_node,
    unsigned kmax, const Q2CensusConsumer& consumer,
    Q2SiblingMode sibling_mode, Q2WitnessOrder witness_order) {
  const auto started = Clock::now();
  validate_anchor_resume(index, anchor_rank, b_node, kmax, sibling_mode, witness_order);
  if (!consumer) throw std::invalid_argument("mhgp8 q2 anchor requires a payload consumer");
  Q2CensusAnchorResult result;
  {
    Q2CensusEngine engine(*index, kmax, consumer);
    initialize_anchor_result(engine, b_node);
    const auto a_id = index->spatial_order()[anchor_rank];
    const Q2CensusEngine::OrderContext context{b_node, index->spatial_nodes()[b_node].escape, anchor_rank};
    if (witness_order == Q2WitnessOrder::ComplementFirst) {
      if (sibling_mode == Q2SiblingMode::Saturating)
        engine.shared_task<true, true>(a_id, b_node, 0, 0, absent, &context);
      else
        engine.shared_task<false, true>(a_id, b_node, 0, 0, absent, &context);
    } else {
      if (sibling_mode == Q2SiblingMode::Saturating)
        engine.shared_task<true, false>(a_id, b_node, 0, 0);
      else
        engine.shared_task<false, false>(a_id, b_node, 0, 0);
    }
    check_completed_anchor(engine.result);
    result = {engine.result, engine.sibling_work, engine.order_work};
  }
  result.census.total_ms = milliseconds(started, Clock::now());
  result.census.count_ms = result.census.total_ms - result.census.payload_ms;
  return result;
}

struct Q2CensusContinuation::Impl {
  struct Frame {
    std::size_t query;
    std::size_t cursor;
    std::size_t sibling;
    unsigned count;
    bool inside_deferred;
    Q2CensusResumeStage stage = Q2CensusResumeStage::Entry;
    Q2PreparedBounds prepared = Q2CensusEngine::inert_resume_bounds();
    Q2BallKey key;
    i64 b_diagonal{};
    std::size_t emit_next{};

    Frame(std::size_t input_query, std::size_t input_cursor, std::size_t input_sibling,
          unsigned input_count, bool input_phase)
        : query(input_query), cursor(input_cursor), sibling(input_sibling),
          count(input_count), inside_deferred(input_phase) {}
  };

  Q2CensusIndexPtr owner;
  const std::size_t anchor_rank;
  const std::size_t anchor_id;
  const std::size_t original_b;
  const std::size_t original_escape;
  const Q2SiblingMode sibling_mode;
  const Q2WitnessOrder witness_order;
  // The existing engine borrows only this owned, unused sentinel. User
  // callbacks are passed directly to emit() and NEVER stored in the object.
  const Q2CensusConsumer unused_consumer = [](const Q2Support&) {
    throw std::logic_error("mhgp8 q2 continuation used its inactive callback");
  };
  Q2CensusEngine engine;
  std::vector<Frame> stack;
  Q2CensusResumeWork resume_work;
  Q2CensusContinuationStatus status = Q2CensusContinuationStatus::Ready;
  mutable std::atomic_flag busy = ATOMIC_FLAG_INIT;

  Impl(Q2CensusIndexPtr index, std::size_t a_rank, std::size_t b_node,
       unsigned kmax, Q2SiblingMode sibling, Q2WitnessOrder order)
      : owner(std::move(index)), anchor_rank(a_rank), anchor_id(owner->spatial_order()[a_rank]),
        original_b(b_node), original_escape(owner->spatial_nodes()[b_node].escape),
        sibling_mode(sibling), witness_order(order), engine(*owner, kmax, unused_consumer) {
    initialize_anchor_result(engine, original_b);
    // Only B splits. Its u16 tree depth is <=48, so a single-root DFS has
    // at most49 pending frames. This reserves storage, never caps searches.
    stack.reserve(49);
    stack.emplace_back(original_b, 0, absent, 0, false);
    resume_work.max_pending_tasks = 1;
  }

  void enter() {
    counter_add(resume_work.entry_steps);
    auto& frame = stack.back();
    counter_add(engine.result.work.query_tasks);
    const auto& b = owner->spatial_nodes()[frame.query];
    const bool singleton = b.range.size() == 1;
    if (singleton) {
      frame.key = ball_key(engine.points[anchor_id], engine.points[engine.b_order[b.range.first]]);
    } else {
      frame.b_diagonal = squared_diagonal(b.box);
      frame.prepared = Q2PreparedBounds(engine.points[anchor_id], b.box);
    }
    frame.stage = Q2CensusResumeStage::Witness;
    if (sibling_mode == Q2SiblingMode::Saturating && frame.sibling != absent) {
      auto& work = engine.sibling_work;
      counter_add(work.proposals);
      const auto& witness = owner->spatial_nodes()[frame.sibling];
      if (witness.range.size() < engine.threshold) {
        counter_add(work.cardinality_skips);
      } else {
        counter_add(work.bound_tests);
        const auto bounds = singleton ? pair_bounds(frame.key, witness.box)
                                      : engine.resume_bounds(frame.prepared, frame.sibling);
        if (bounds.minimum4 > 0) {
          counter_add(work.rejected_tasks);
          counter_add(work.rejected_pairs, static_cast<u64>(b.range.size()));
          if (frame.count != 0) counter_add(work.rejected_after_credit);
          engine.reject(b.range);
          stack.pop_back();
        }
      }
    }
  }

  void admit() {
    counter_add(resume_work.admission_steps);
    auto& frame = stack.back();
    if (frame.count >= engine.threshold)
      throw std::logic_error("mhgp8 q2 continuation admitted a saturated query");
    const auto range = owner->spatial_nodes()[frame.query].range;
    counter_add(engine.result.accepted_pairs, static_cast<u64>(range.size()));
    if (range.size() > 1)
      counter_add(engine.result.work.uniform_accepted_pairs, static_cast<u64>(range.size()));
    frame.emit_next = range.first;
    frame.stage = Q2CensusResumeStage::Emit;
  }

  void witness() {
    auto& frame = stack.back();
    const auto nodes = owner->spatial_nodes();
    const bool complement = witness_order == Q2WitnessOrder::ComplementFirst;
    if ((!complement && frame.cursor == nodes.size()) ||
        (complement && frame.inside_deferred && frame.cursor == original_escape)) {
      admit();
      return;
    }
    counter_add(resume_work.witness_steps);
    auto& work = engine.result.work;
    auto& order = engine.order_work;
    if (complement && !frame.inside_deferred && frame.cursor == nodes.size()) {
      frame.inside_deferred = true;
      frame.cursor = original_b;
      counter_add(order.phase_switches);
      counter_add(work.cursor_advances);
      return;  // Phase transition is paid once, before its first Z decision.
    }
    if (frame.cursor >= nodes.size())
      throw std::logic_error("mhgp8 q2 continuation cursor exceeds its immutable index");
    const auto& z = nodes[frame.cursor];
    if (complement) {
      if (!frame.inside_deferred && frame.cursor == original_b) {
        frame.cursor = original_escape;
        counter_add(order.deferred_skips);
        counter_add(work.cursor_advances);
        return;
      }
      const bool contains_anchor = z.range.first <= anchor_rank && anchor_rank < z.range.last;
      const bool contains_deferred = !frame.inside_deferred && frame.cursor < original_b &&
                                     original_b < z.escape;
      if (contains_anchor || contains_deferred) {
        if (z.left == absent) {
          frame.cursor = z.escape;
          engine.consume_witnesses(1);
          counter_add(order.anchor_skips);
        } else {
          frame.cursor = z.left;
          counter_add(order.structural_splits);
        }
        counter_add(work.cursor_advances);
        return;
      }
    }
    const auto& b = nodes[frame.query];
    const bool singleton = b.range.size() == 1;
    counter_add(work.count_node_visits);
    PowerBounds bounds;
    if (singleton && z.left == absent) {
      counter_add(work.count_point_tests);
      const auto value = point_power4(frame.key, engine.points[owner->spatial_order()[z.range.first]]);
      bounds = {value, value};
    } else {
      counter_add(work.count_bound_tests);
      bounds = singleton ? pair_bounds(frame.key, z.box) : engine.resume_bounds(frame.prepared, frame.cursor);
    }
    if (bounds.minimum4 > 0) {
      engine.consume_witnesses(z.range.size());
      if (!singleton) counter_add(work.uniform_credited_pairs, static_cast<u64>(b.range.size()));
      engine.add_count(frame.count, z.range.size());
      frame.cursor = z.escape;
      counter_add(work.cursor_advances);
      if (frame.count == engine.threshold) {
        engine.reject(b.range);
        stack.pop_back();
      }
    } else if (bounds.maximum4 <= 0) {
      engine.consume_witnesses(z.range.size());
      frame.cursor = z.escape;
      counter_add(work.cursor_advances);
    } else if (singleton || (z.left != absent && squared_diagonal(z.box) > frame.b_diagonal)) {
      if (z.left == absent)
        throw std::logic_error("mhgp8 q2 singleton continuation power cannot be uncertain");
      counter_add(work.witness_splits);
      frame.cursor = z.left;
      counter_add(work.cursor_advances);
    } else {
      if (b.left == absent)
        throw std::logic_error("mhgp8 q2 continuation query has no children");
      counter_add(work.query_splits);
      if (frame.count > 0) counter_add(work.shared_splits_after_credit);
      counter_add(work.cursor_reuses, 2);
      const auto count = frame.count;
      const auto cursor = frame.cursor;
      const auto phase = frame.inside_deferred;
      // Discard the completed parent, push right then left. Children inherit
      // the SAME count/cursor/phase and the immutable original-B context.
      stack.pop_back();
      stack.emplace_back(b.right, cursor, b.left, count, phase);
      stack.emplace_back(b.left, cursor, b.right, count, phase);
      resume_work.max_pending_tasks = std::max(resume_work.max_pending_tasks, static_cast<u64>(stack.size()));
    }
  }

  void emit(const Q2CensusConsumer& consumer) {
    counter_add(resume_work.payload_steps);
    auto& frame = stack.back();
    const auto started = Clock::now();
    try {
      const auto b_id = engine.b_order[frame.emit_next];
      const auto key = ball_key(engine.points[anchor_id], engine.points[b_id]);
      engine.interior.clear();
      engine.shell.clear();
      engine.collect(0, key);
      if (engine.interior.size() != frame.count)
        throw std::logic_error("mhgp8 q2 continuation count and global interior IDs disagree");
      counter_add(engine.result.work.payload_interior_sites, static_cast<u64>(engine.interior.size()));
      counter_add(engine.result.work.payload_shell_sites, static_cast<u64>(engine.shell.size()));
      counter_add(engine.result.work.payload_supports);
      consumer(Q2Support{anchor_id, b_id, key, engine.interior, engine.shell});
      ++frame.emit_next;  // Only after this atomic emission succeeds.
      if (frame.emit_next == owner->spatial_nodes()[frame.query].range.last) stack.pop_back();
    } catch (...) {
      engine.result.payload_ms += milliseconds(started, Clock::now());
      throw;
    }
    engine.result.payload_ms += milliseconds(started, Clock::now());
  }

  void step(const Q2CensusConsumer& consumer) {
    switch (stack.back().stage) {
      case Q2CensusResumeStage::Entry: enter(); break;
      case Q2CensusResumeStage::Witness: witness(); break;
      case Q2CensusResumeStage::Emit: emit(consumer); break;
      case Q2CensusResumeStage::None:
        throw std::logic_error("mhgp8 q2 continuation has an empty active frame");
    }
  }

  bool advance(std::size_t budget, const Q2CensusConsumer& consumer) {
    if (budget == 0 || !consumer)
      throw std::invalid_argument("mhgp8 q2 continuation requires positive budget and a consumer");
    const ExclusiveResumeCall lock(busy);
    if (status == Q2CensusContinuationStatus::Failed)
      throw std::logic_error("mhgp8 q2 failed continuation cannot be resumed");
    if (status == Q2CensusContinuationStatus::Done) return true;
    const auto started = Clock::now();
    const auto record_time = [&] {
      engine.result.total_ms += milliseconds(started, Clock::now());
      engine.result.count_ms = engine.result.total_ms - engine.result.payload_ms;
    };
    try {
      counter_add(resume_work.advance_calls);
      while (budget != 0 && !stack.empty()) {
        counter_add(resume_work.transitions);
        step(consumer);
        --budget;
      }
      if (stack.empty()) {
        check_completed_anchor(engine.result);
        status = Q2CensusContinuationStatus::Done;
      } else {
        counter_add(resume_work.pauses);
        const auto& frame = stack.back();
        if (frame.count != 0) counter_add(resume_work.pauses_after_credit);
        if (frame.inside_deferred) counter_add(resume_work.pauses_inside_deferred);
        if (frame.stage == Q2CensusResumeStage::Emit) counter_add(resume_work.pauses_during_emission);
      }
    } catch (...) {
      status = Q2CensusContinuationStatus::Failed;
      record_time();
      throw;
    }
    record_time();
    return status == Q2CensusContinuationStatus::Done;
  }
};

Q2CensusContinuation::Q2CensusContinuation(std::unique_ptr<Impl> implementation)
    : implementation_(std::move(implementation)) {}
Q2CensusContinuation::~Q2CensusContinuation() = default;

std::unique_ptr<Q2CensusContinuation> make_q2_census_continuation(
    Q2CensusIndexPtr index, std::size_t anchor_rank, std::size_t b_node,
    unsigned kmax, Q2SiblingMode sibling_mode, Q2WitnessOrder witness_order) {
  validate_anchor_resume(index, anchor_rank, b_node, kmax, sibling_mode, witness_order);
  auto implementation = std::make_unique<Q2CensusContinuation::Impl>(
      std::move(index), anchor_rank, b_node, kmax, sibling_mode, witness_order);
  return std::unique_ptr<Q2CensusContinuation>(new Q2CensusContinuation(std::move(implementation)));
}

bool Q2CensusContinuation::advance(std::size_t budget, const Q2CensusConsumer& consumer) {
  return implementation_->advance(budget, consumer);
}

Q2CensusResumeSnapshot Q2CensusContinuation::snapshot() const {
  const auto& state = *implementation_;
  const ExclusiveResumeCall lock(state.busy);
  return {state.engine.result, state.engine.sibling_work, state.engine.order_work,
          state.resume_work, state.status};
}

Q2CensusResumePending Q2CensusContinuation::pending() const {
  const auto& state = *implementation_;
  const ExclusiveResumeCall lock(state.busy);
  Q2CensusResumePending result;
  result.task_count = state.stack.size();
  result.original_b_node = state.original_b;
  if (!state.stack.empty()) {
    const auto& frame = state.stack.back();
    result.stage = frame.stage;
    result.query_node = frame.query;
    result.cursor = frame.cursor;
    result.sibling_node = frame.sibling;
    result.acquired_count = frame.count;
    result.inside_deferred = frame.inside_deferred;
    if (frame.stage == Q2CensusResumeStage::Emit) {
      result.emit_next = frame.emit_next;
      result.emit_end = state.owner->spatial_nodes()[frame.query].range.last;
    }
  }
  return result;
}

Q2CensusResumeMemory Q2CensusContinuation::memory() const {
  const auto& state = *implementation_;
  const ExclusiveResumeCall lock(state.busy);
  Q2CensusResumeMemory result;
  result.stack_capacity = state.stack.capacity();
  result.stack_bytes = resume_bytes(result.stack_capacity, sizeof(Impl::Frame));
  result.interior_capacity = state.engine.interior.capacity();
  result.shell_capacity = state.engine.shell.capacity();
  result.payload_bytes = resume_byte_sum(resume_bytes(result.interior_capacity, sizeof(std::size_t)),
                                        resume_bytes(result.shell_capacity, sizeof(std::size_t)));
  result.retained_bytes = resume_byte_sum(result.stack_bytes, result.payload_bytes);
  return result;
}

const Q2CensusIndex& Q2CensusContinuation::index() const noexcept { return *implementation_->owner; }

}  // namespace mhgp8
