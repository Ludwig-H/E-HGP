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

enum class BoxDecision { Accept, Reject, Refine };

class BoxIndex final {
 public:
  BoxIndex(std::span<const Point3> points, std::vector<std::size_t>& order,
           AxisQ2Work& work, std::span<const std::uint8_t> credits = {},
           std::size_t b_first = 0)
      : points_(points), order_(order), work_(work), credits_(credits), b_first_(b_first) {
    if (order_.empty()) {
      throw std::logic_error("mhgp8 axis index requires a nonempty factor");
    }
    static_cast<void>(build({0, order_.size()}, 0));
  }

  template <class Consumer>
  void query(const Box3& box, Consumer&& consume) const {
    query_node(0, box, consume);
  }

  template <class Classifier, class Consumer>
  void query_refined(Classifier&& classify, Consumer&& consume) const {
    // The factor root was already classified before deciding to build B.
    // Reuse that result, while still counting its actual query visit.
    counter_add(work_.query_nodes);
    if (nodes_[0].left == absent) {
      throw std::logic_error("mhgp8 axis point root cannot need refinement");
    }
    query_classified_node(nodes_[0].left, classify, consume);
    query_classified_node(nodes_[0].right, classify, consume);
  }

 private:
  static constexpr std::size_t absent = std::numeric_limits<std::size_t>::max();
  struct Node {
    Range range;
    Box3 box;
    std::size_t left{absent};
    std::size_t right{absent};
    std::uint8_t min_credit{};
    std::uint8_t max_credit{};
  };

  std::span<const Point3> points_;
  std::vector<std::size_t>& order_;
  AxisQ2Work& work_;
  std::span<const std::uint8_t> credits_;
  std::size_t b_first_{};
  std::vector<Node> nodes_;

  [[nodiscard]] std::size_t build(Range range, u64 depth) {
    const auto& first = points_[order_[range.first]];
    Box3 box{first, first};
    std::uint8_t min_credit = std::numeric_limits<std::uint8_t>::max();
    std::uint8_t max_credit = 0;
    for (std::size_t index = range.first; index < range.last; ++index) {
      counter_add(work_.tree_point_visits);
      const auto& point = points_[order_[index]];
      box.low = {std::min(box.low.x, point.x), std::min(box.low.y, point.y),
                 std::min(box.low.z, point.z)};
      box.high = {std::max(box.high.x, point.x), std::max(box.high.y, point.y),
                  std::max(box.high.z, point.z)};
      if (!credits_.empty()) {
        counter_add(work_.restriction_credit_visits);
        const auto credit = credits_[order_[index] - b_first_];
        min_credit = std::min(min_credit, credit);
        max_credit = std::max(max_credit, credit);
      }
    }
    const auto node_id = nodes_.size();
    nodes_.push_back(Node{range, box});
    if (!credits_.empty()) {
      nodes_.back().min_credit = min_credit;
      nodes_.back().max_credit = max_credit;
    }
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

  template <class Classifier, class Consumer>
  void query_classified_node(std::size_t node_id, Classifier& classify,
                             Consumer& consume) const {
    counter_add(work_.query_nodes);
    const auto& node = nodes_[node_id];
    const auto decision = classify(node.box, node.min_credit, node.max_credit);
    if (decision == BoxDecision::Reject) {
      counter_add(work_.disjoint_nodes);
      return;
    }
    if (decision == BoxDecision::Accept) {
      counter_add(work_.contained_nodes);
      consume(node.range);
      return;
    }
    if (node.left == absent) {
      throw std::logic_error("mhgp8 axis point counts must have exact bounds");
    }
    query_classified_node(node.left, classify, consume);
    query_classified_node(node.right, classify, consume);
  }
};

}  // namespace

AxisQ2Plan::AxisQ2Plan(RectanglePtr rectangle, AxisQ2Mode mode,
                     const CreditPlan* restriction)
    : rectangle_(std::move(rectangle)), mode_(mode),
      has_restriction_(restriction != nullptr) {
  if (!rectangle_) {
    throw std::invalid_argument("mhgp8 axis filter requires an owned rectangle");
  }
  if (mode_ != AxisQ2Mode::Independent && mode_ != AxisQ2Mode::Additive) {
    throw std::invalid_argument("mhgp8 axis filter mode is invalid");
  }
  // Validate even for a rectangle already killed by its core. A moved-from
  // restriction throws through its checked owner accessor, before any alias
  // or credit could be consumed.
  if (restriction != nullptr &&
      (&restriction->rectangle() != rectangle_.get() || restriction->lane() != Lane::Q2)) {
    throw std::invalid_argument("mhgp8 axis restriction must be q2 on the same owner");
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
  if (restriction != nullptr) {
    const auto ca = restriction->a_credits();
    const auto cb = restriction->b_credits();
    if (ca.size() != a.size() || cb.size() != b.size()) {
      throw std::invalid_argument("mhgp8 axis restriction credit sizes disagree");
    }
    restriction_a_.assign(ca.begin(), ca.end());
    restriction_b_.assign(cb.begin(), cb.end());
    counter_add(work_.restriction_credit_copies, restriction_a_.size());
    counter_add(work_.restriction_credit_copies, restriction_b_.size());
  }

  constexpr auto maximum = std::numeric_limits<std::uint16_t>::max();
  const Box3 universe{{0, 0, 0}, {maximum, maximum, maximum}};
  anchor_bounds_.assign(a.size(), universe);
  if (mode_ == AxisQ2Mode::Additive) {
    column_windows_.resize(a.size());
  }
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
        if (mode_ == AxisQ2Mode::Additive) {
          // The window includes the anchor but excludes it from both searches.
          // Subtractions are bounded by the existing column; no index+need
          // expression can overflow near size_t's upper boundary.
          const auto before = std::min<std::size_t>(need_, index - begin);
          const auto after = std::min<std::size_t>(need_, end - index - 1);
          column_windows_[order[index] - a.first][axis] =
              ColumnWindow{index - before, index, index + 1 + after};
        }
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
    if (mode_ == AxisQ2Mode::Additive) {
      // Preserve the same initial order of the next sort as Independent while
      // retaining only IDs, never a duplicate coordinate buffer.
      column_orders_[axis] = order;
    }
  }

  // These slabs preserve the Independent certificate and its statistics.
  // Additive additionally uses the retained column ranks below: the exact
  // columns meet only at the excluded anchor, so their witnesses are disjoint.
  // The core remains outside A and B by the rectangle's own certification.
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

  if (mode_ == AxisQ2Mode::Additive || has_restriction_) {
    std::uint8_t min_credit = 0;
    std::uint8_t max_credit = 0;
    if (has_restriction_) {
      min_credit = std::numeric_limits<std::uint8_t>::max();
      for (const auto credit : restriction_b_) {
        counter_add(work_.restriction_credit_visits);
        min_credit = std::min(min_credit, credit);
        max_credit = std::max(max_credit, credit);
      }
    }
    const auto classify = [&](std::size_t a_index, const Box3& box,
                              std::uint8_t min_cb, std::uint8_t max_cb) {
      bool restriction_accepts = true;
      if (has_restriction_) {
        counter_add(work_.restriction_bound_queries);
        const unsigned ca = restriction_a_[a_index];
        // Local A/B populations are disjoint from each other, but may overlap
        // the axial witnesses. Intersect the two residuals; never add axes
        // to ca+cb. Integer promotion bounds this sum by 2*255, not u8.
        if (ca + min_cb >= need_) {
          counter_add(work_.restriction_pruned_nodes);
          return BoxDecision::Reject;
        }
        restriction_accepts = ca + max_cb < need_;
      }
      // The h-th-neighbour slab already certifies h witnesses on one axis.
      // Its constant-cost exclusion avoids rank searches without changing
      // the additive residual: a rejected slab implies sum >= need.
      if (disjoint(anchor_bounds_[a_index], box)) {
        counter_add(work_.axis_slab_rejects);
        return BoxDecision::Reject;
      }
      unsigned axis_min = 0;
      unsigned axis_max = 0;
      if (mode_ == AxisQ2Mode::Additive) {
        const auto bounds = axis_bounds(a_index, box, &work_);
        axis_min = bounds.first;
        axis_max = bounds.second;
      } else {
        counter_add(work_.axis_bound_queries);
        axis_max = contains(anchor_bounds_[a_index], box) ? 0 : need_;
      }
      if (axis_min >= need_) {
        counter_add(work_.axis_pruned_nodes);
        return BoxDecision::Reject;
      }
      return axis_max < need_ && restriction_accepts ? BoxDecision::Accept
                                                   : BoxDecision::Refine;
    };

    std::vector<BoxDecision> roots;
    roots.reserve(a.size());
    needs_index = false;
    for (std::size_t a_index = 0; a_index < a.size(); ++a_index) {
      const auto decision = classify(a_index, r.b_box(), min_credit, max_credit);
      roots.push_back(decision);
      needs_index = needs_index || decision == BoxDecision::Refine;
    }
    std::unique_ptr<BoxIndex> index;
    if (needs_index) {
      index = std::make_unique<BoxIndex>(points, b_order_, work_, restriction_b_, b.first);
    }
    for (std::size_t a_id = a.first; a_id < a.last; ++a_id) {
      const auto a_index = a_id - a.first;
      const auto emit = [&](Range range) {
        counter_add(candidates_, range.size());
        if (mode_ == AxisQ2Mode::Additive && !blocks_.empty() &&
            blocks_.back().a_id == a_id && blocks_.back().b.last == range.first) {
          blocks_.back().b.last = range.last;
          counter_add(work_.coalesced_blocks);
          return;
        }
        blocks_.push_back(AxisQ2Block{a_id, range});
        counter_add(work_.emitted_blocks);
      };
      if (roots[a_index] == BoxDecision::Accept) {
        counter_add(work_.whole_factor_accepts);
        emit({0, b_order_.size()});
      } else if (roots[a_index] == BoxDecision::Reject) {
        counter_add(work_.whole_factor_rejects);
      } else {
        if (!index) {
          throw std::logic_error("mhgp8 axis query has no prepared index");
        }
        const auto classify_anchor = [&](const Box3& box, std::uint8_t low,
                                         std::uint8_t high) {
          return classify(a_index, box, low, high);
        };
        index->query_refined(classify_anchor, emit);
      }
    }
    // Queries emit disjoint index ranges. Coalescing changes only adjacent
    // descriptors, not their IDs or cardinality. J visits and D retained
    // fragments still have no global subquadratic guarantee on general data.
    return;
  }

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

AxisQ2Plan make_axis_q2_plan(RectanglePtr rectangle, AxisQ2Mode mode,
                           const CreditPlan* restriction) {
  return AxisQ2Plan(std::move(rectangle), mode, restriction);
}

AxisQ2Plan::AxisQ2Plan(AxisQ2Plan&& other) noexcept
    : rectangle_(std::move(other.rectangle_)), need_(std::exchange(other.need_, 0)),
      mode_(std::exchange(other.mode_, AxisQ2Mode::Independent)),
      has_restriction_(std::exchange(other.has_restriction_, false)),
      anchor_bounds_(std::move(other.anchor_bounds_)),
      column_orders_(std::move(other.column_orders_)),
      column_windows_(std::move(other.column_windows_)),
      restriction_a_(std::move(other.restriction_a_)),
      restriction_b_(std::move(other.restriction_b_)),
      b_order_(std::move(other.b_order_)), blocks_(std::move(other.blocks_)),
      work_(std::exchange(other.work_, AxisQ2Work{})),
      total_(std::exchange(other.total_, 0)), candidates_(std::exchange(other.candidates_, 0)) {
  other.anchor_bounds_.clear();
  for (auto& order : other.column_orders_) {
    order.clear();
  }
  other.column_windows_.clear();
  other.restriction_a_.clear();
  other.restriction_b_.clear();
  other.b_order_.clear();
  other.blocks_.clear();
}

unsigned AxisQ2Plan::axis_count(std::size_t a_index, std::size_t axis,
                               std::uint16_t value, AxisQ2Work* work) const {
  if (work != nullptr) {
    counter_add(work->axis_count_queries);
  }
  const auto& window = column_windows_[a_index][axis];
  const auto& order = column_orders_[axis];
  const auto points = rectangle_->points();
  const auto coordinate = points[order[window.rank]][axis];
  if (value == coordinate) {
    return 0;
  }
  const bool before = value < coordinate;
  std::size_t first = before ? window.first : window.rank + 1;
  std::size_t last = before ? window.rank : window.last;
  // Strict count: before a, upper_bound(value) excludes equality with b;
  // after a, lower_bound(value) does likewise. Each searched window has at
  // most need <= 255 sites, giving O(log(need+1)) comparisons, not O(log|A|).
  while (first < last) {
    const auto middle = first + (last - first) / 2;
    if (work != nullptr) {
      counter_add(work->axis_rank_comparisons);
    }
    const auto candidate = points[order[middle]][axis];
    if (before ? candidate <= value : candidate < value) {
      first = middle + 1;
    } else {
      last = middle;
    }
  }
  const auto count = before ? window.rank - first : first - (window.rank + 1);
  return static_cast<unsigned>(count);  // count <= need <= 255
}

std::pair<unsigned, unsigned> AxisQ2Plan::axis_bounds(
    std::size_t a_index, const Box3& box, AxisQ2Work* work) const {
  if (work != nullptr) {
    counter_add(work->axis_bound_queries);
  }
  const auto& anchor = rectangle_->points()[rectangle_->a_range().first + a_index];
  unsigned minimum = 0;
  unsigned maximum = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto& window = column_windows_[a_index][axis];
    if (window.first == window.rank && window.last == window.rank + 1) {
      continue;  // An exact singleton column supplies no witness at any t.
    }
    const auto low = axis_count(a_index, axis, box.low[axis], work);
    const auto high = box.low[axis] == box.high[axis]
                          ? low : axis_count(a_index, axis, box.high[axis], work);
    const auto lower = box.low[axis] <= anchor[axis] && anchor[axis] <= box.high[axis]
                           ? 0U : std::min(low, high);
    // Coordinate columns meet only at the excluded anchor. Their certified
    // witness IDs are disjoint; saturating their sum preserves both tests.
    // Each addition is at most 2*need <= 510, safely inside unsigned.
    minimum = std::min<unsigned>(need_, minimum + lower);
    maximum = std::min<unsigned>(need_, maximum + std::max(low, high));
    if (minimum == need_) {
      return {need_, need_};
    }
  }
  return {minimum, maximum};
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
  if (need_ == 0) {
    return false;
  }
  if (has_restriction_ &&
      static_cast<unsigned>(restriction_a_[a_id - a.first]) + restriction_b_[b_id - b.first]
          >= need_) {
    return false;
  }
  const auto& point = rectangle_->points()[b_id];
  if (mode_ == AxisQ2Mode::Additive) {
    return axis_bounds(a_id - a.first, Box3{point, point}, nullptr).first < need_;
  }
  return contains(anchor_bounds_[a_id - a.first], point);
}

}  // namespace mhgp8
