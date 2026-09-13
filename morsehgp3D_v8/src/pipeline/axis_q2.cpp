#include "axis_q2.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace mhgp8 {
namespace {

[[nodiscard]] bool contains(Range range, std::size_t id) {
  return range.first <= id && id < range.last;
}

[[nodiscard]] bool contains(const Box3& box, const Point3& point) {
  return box.low.x <= point.x && point.x <= box.high.x &&
         box.low.y <= point.y && point.y <= box.high.y &&
         box.low.z <= point.z && point.z <= box.high.z;
}

[[nodiscard]] bool contains(const Box3& outer, const Box3& inner) {
  return contains(outer, inner.low) && contains(outer, inner.high);
}

[[nodiscard]] bool disjoint(const Box3& left, const Box3& right) {
  return left.high.x < right.low.x || right.high.x < left.low.x ||
         left.high.y < right.low.y || right.high.y < left.low.y ||
         left.high.z < right.low.z || right.high.z < left.low.z;
}

void set_coordinate(Point3& point, std::size_t axis, std::uint16_t value) {
  switch (axis) {
    case 0: point.x = value; return;
    case 1: point.y = value; return;
    case 2: point.z = value; return;
    default: throw std::logic_error("mhgp8 axis filter has an invalid axis");
  }
}

[[nodiscard]] u64 pair_count(std::size_t a_size, std::size_t b_size) {
  static_assert(sizeof(std::size_t) <= sizeof(u64));
  const auto a = static_cast<u64>(a_size);
  const auto b = static_cast<u64>(b_size);
  if (b != 0 && a > std::numeric_limits<u64>::max() / b) {
    throw std::overflow_error("mhgp8 axis candidate cardinal exceeds u64");
  }
  return a * b;
}

class BoxIndex final {
 public:
  BoxIndex(std::span<const Point3> points, std::vector<std::size_t>& order,
           AxisQ2Work& work) : points_(points), order_(order), work_(work) {
    if (order_.empty()) {
      throw std::logic_error("mhgp8 axis index requires a nonempty factor");
    }
    static_cast<void>(build({0, order_.size()}, 0));
  }

  template <class Consumer>
  void query(const Box3& box, Consumer&& consume) const {
    query_node(0, box, consume);
  }

 private:
  static constexpr std::size_t absent = std::numeric_limits<std::size_t>::max();
  struct Node {
    Range range;
    Box3 box;
    std::size_t left{absent};
    std::size_t right{absent};
  };

  std::span<const Point3> points_;
  std::vector<std::size_t>& order_;
  AxisQ2Work& work_;
  std::vector<Node> nodes_;

  [[nodiscard]] std::size_t build(Range range, u64 depth) {
    const auto& first = points_[order_[range.first]];
    Box3 box{first, first};
    for (std::size_t index = range.first; index < range.last; ++index) {
      counter_add(work_.tree_point_visits);
      const auto& point = points_[order_[index]];
      box.low = {std::min(box.low.x, point.x), std::min(box.low.y, point.y),
                 std::min(box.low.z, point.z)};
      box.high = {std::max(box.high.x, point.x), std::max(box.high.y, point.y),
                  std::max(box.high.z, point.z)};
    }
    const auto node_id = nodes_.size();
    nodes_.push_back(Node{range, box});
    counter_add(work_.tree_nodes);
    work_.max_tree_depth = std::max(work_.max_tree_depth, depth);
    if (range.size() == 1) {
      return node_id;
    }
    std::size_t axis = 0;
    for (std::size_t candidate = 1; candidate < 3; ++candidate) {
      if (box.high[candidate] - box.low[candidate] > box.high[axis] - box.low[axis]) {
        axis = candidate;
      }
    }
    const unsigned middle =
        (static_cast<unsigned>(box.low[axis]) + box.high[axis]) / 2;
    const auto begin = order_.begin() + static_cast<std::ptrdiff_t>(range.first);
    const auto end = order_.begin() + static_cast<std::ptrdiff_t>(range.last);
    const auto cut = std::partition(begin, end, [&](std::size_t id) {
      counter_add(work_.tree_point_visits);
      return points_[id][axis] <= middle;
    });
    const auto split = static_cast<std::size_t>(cut - order_.begin());
    if (split == range.first || split == range.last) {
      throw std::logic_error("mhgp8 axis midpoint split failed on distinct sites");
    }
    // Every split halves a positive u16 coordinate extent. A path has at most
    // 48 splits; this is an input-width proof, not a truncation of queries.
    const auto left = build({range.first, split}, depth + 1);
    const auto right = build({split, range.last}, depth + 1);
    nodes_[node_id].left = left;
    nodes_[node_id].right = right;
    return node_id;
  }

  template <class Consumer>
  void query_node(std::size_t node_id, const Box3& box, Consumer& consume) const {
    counter_add(work_.query_nodes);
    const auto& node = nodes_[node_id];
    if (disjoint(box, node.box)) {
      counter_add(work_.disjoint_nodes);
      return;
    }
    if (contains(box, node.box)) {
      counter_add(work_.contained_nodes);
      consume(node.range);
      return;
    }
    if (node.left == absent) {
      throw std::logic_error("mhgp8 axis point box is neither contained nor disjoint");
    }
    query_node(node.left, box, consume);
    query_node(node.right, box, consume);
  }
};

}  // namespace

AxisQ2Plan::AxisQ2Plan(RectanglePtr rectangle) : rectangle_(std::move(rectangle)) {
  if (!rectangle_) {
    throw std::invalid_argument("mhgp8 axis filter requires an owned rectangle");
  }
  const auto& r = *rectangle_;
  const auto a = r.a_range();
  const auto b = r.b_range();
  const auto points = r.points();
  total_ = pair_count(a.size(), b.size());
  need_ = static_cast<std::uint8_t>(r.threshold(Lane::Q2) - r.core_credit(Lane::Q2));
  if (need_ == 0) {
    return;
  }

  constexpr auto maximum = std::numeric_limits<std::uint16_t>::max();
  const Box3 universe{{0, 0, 0}, {maximum, maximum, maximum}};
  anchor_bounds_.assign(a.size(), universe);
  std::vector<std::size_t> order(a.size());
  std::iota(order.begin(), order.end(), a.first);
  for (std::size_t axis = 0; axis < 3; ++axis) {
    std::array<std::size_t, 2> other{};
    std::size_t offset = 0;
    for (std::size_t candidate = 0; candidate < 3; ++candidate) {
      if (candidate != axis) {
        other[offset++] = candidate;
      }
    }
    counter_add(work_.sort_passes);
    counter_add(work_.sorted_sites, order.size());
    std::sort(order.begin(), order.end(), [&](std::size_t left, std::size_t right) {
      counter_add(work_.sort_comparisons);
      for (const auto coordinate : other) {
        if (points[left][coordinate] != points[right][coordinate]) {
          return points[left][coordinate] < points[right][coordinate];
        }
      }
      if (points[left][axis] != points[right][axis]) {
        return points[left][axis] < points[right][axis];
      }
      return left < right;
    });
    const auto same_column = [&](std::size_t left, std::size_t right) {
      return points[left][other[0]] == points[right][other[0]] &&
             points[left][other[1]] == points[right][other[1]];
    };
    std::size_t begin = 0;
    while (begin < order.size()) {
      std::size_t end = begin + 1;
      while (end < order.size() && same_column(order[begin], order[end])) {
        ++end;
      }
      counter_add(work_.columns);
      for (std::size_t index = begin; index < end; ++index) {
        auto& slab = anchor_bounds_[order[index] - a.first];
        // Within a column the varying coordinates are strictly increasing:
        // equality would duplicate all three coordinates, rejected by r.
        // Every one of these need sites alone satisfies H>0 if b lies
        // STRICTLY beyond the kth coordinate. Equality remains a candidate.
        if (index - begin >= need_) {
          const auto bound = points[order[index - need_]][axis];
          if (bound > slab.low[axis]) {
            set_coordinate(slab.low, axis, bound);
            counter_add(work_.slab_bound_updates);
          }
        }
        if (end - index > need_) {
          const auto bound = points[order[index + need_]][axis];
          if (bound < slab.high[axis]) {
            set_coordinate(slab.high, axis, bound);
            counter_add(work_.slab_bound_updates);
          }
        }
      }
      begin = end;
    }
  }

  // Each of the six exclusions certifies need witnesses independently. The
  // closed box intersects their complements. This conservative variant does
  // not add axes, although exact coordinate columns only intersect at the
  // excluded anchor and thus have disjoint witness populations. The core
  // remains disjoint from A and B by the rectangle's own certification.
  bool needs_index = false;
  for (const auto& slab : anchor_bounds_) {
    if (slab.low != universe.low || slab.high != universe.high) {
      counter_add(work_.constrained_anchors);
    }
    if (!contains(slab, r.b_box()) && !disjoint(slab, r.b_box())) {
      needs_index = true;
    }
  }
  b_order_.resize(b.size());
  std::iota(b_order_.begin(), b_order_.end(), b.first);
  std::unique_ptr<BoxIndex> index;
  if (needs_index) {
    index = std::make_unique<BoxIndex>(points, b_order_, work_);
  }
  for (std::size_t a_id = a.first; a_id < a.last; ++a_id) {
    const auto& slab = anchor_bounds_[a_id - a.first];
    const auto emit = [&](Range range) {
      blocks_.push_back(AxisQ2Block{a_id, range});
      counter_add(work_.emitted_blocks);
      counter_add(candidates_, range.size());
    };
    if (contains(slab, r.b_box())) {
      // Generic data with no repeated transverse coordinates takes this path:
      // one descriptor per anchor, without visiting the B tree or its sites.
      counter_add(work_.whole_factor_accepts);
      emit({0, b_order_.size()});
    } else if (disjoint(slab, r.b_box())) {
      counter_add(work_.whole_factor_rejects);
    } else {
      if (!index) {
        throw std::logic_error("mhgp8 axis query has no prepared index");
      }
      index->query(slab, emit);
    }
  }
  // Only range descriptors survive. Index nodes and their borrowed point span
  // are local to construction. D emitted fragments cost O(D) memory; neither
  // the number of visited nodes nor D has a global linear guarantee here.
}

AxisQ2Plan make_axis_q2_plan(RectanglePtr rectangle) {
  return AxisQ2Plan(std::move(rectangle));
}

AxisQ2Plan::AxisQ2Plan(AxisQ2Plan&& other) noexcept
    : rectangle_(std::move(other.rectangle_)), need_(std::exchange(other.need_, 0)),
      anchor_bounds_(std::move(other.anchor_bounds_)),
      b_order_(std::move(other.b_order_)), blocks_(std::move(other.blocks_)),
      work_(std::exchange(other.work_, AxisQ2Work{})),
      total_(std::exchange(other.total_, 0)), candidates_(std::exchange(other.candidates_, 0)) {
  other.anchor_bounds_.clear();
  other.b_order_.clear();
  other.blocks_.clear();
}

bool AxisQ2Plan::keeps(std::size_t a_id, std::size_t b_id) const {
  if (!rectangle_) {
    throw std::logic_error("mhgp8 axis plan was moved from");
  }
  const auto a = rectangle_->a_range();
  const auto b = rectangle_->b_range();
  if (!contains(a, a_id) || !contains(b, b_id)) {
    throw std::invalid_argument("mhgp8 axis pair IDs do not belong to this rectangle");
  }
  return need_ != 0 && contains(anchor_bounds_[a_id - a.first], rectangle_->points()[b_id]);
}

}  // namespace mhgp8
