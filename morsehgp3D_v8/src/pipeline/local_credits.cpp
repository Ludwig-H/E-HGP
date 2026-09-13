#include "local_credits.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <utility>

#include "spindle/predicates.hpp"
#include "tube_credits.hpp"

namespace mhgp8 {
namespace {

[[nodiscard]] bool contains(Range range, std::size_t id) {
  return range.first <= id && id < range.last;
}

[[nodiscard]] Box3 bounds(std::span<const Point3> points, Range range) {
  Box3 result{points[range.first], points[range.first]};
  for (std::size_t id = range.first + 1; id < range.last; ++id) {
    const auto& p = points[id];
    result.low = {std::min(result.low.x, p.x), std::min(result.low.y, p.y),
                  std::min(result.low.z, p.z)};
    result.high = {std::max(result.high.x, p.x), std::max(result.high.y, p.y),
                   std::max(result.high.z, p.z)};
  }
  return result;
}

[[nodiscard]] i64 diameter_squared(const Box3& box) {
  i64 value = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 width = static_cast<i64>(box.high[axis]) - box.low[axis];
    value += width * width;
  }
  return value;
}

[[nodiscard]] u64 pair_count(std::size_t a, std::size_t b) {
  if (b != 0 && a > std::numeric_limits<u64>::max() / b) {
    throw std::overflow_error("mhgp8 candidate cardinal exceeds u64");
  }
  return static_cast<u64>(a) * static_cast<u64>(b);
}

[[nodiscard]] std::vector<std::uint8_t> pool_credits(
    const PreparedRectangle& rectangle, Range range, const Box3& own,
    const Box3& opposite, Lane lane, std::uint8_t need, Work& work) {
  // Proposal only: the h+1 best projections include alternatives for the
  // anchor itself. Failing to find a witness never removes a candidate.
  struct Proposal { i64 score; std::size_t id; };
  std::array<i64, 3> direction{};
  for (std::size_t axis = 0; axis < 3; ++axis) {
    direction[axis] = static_cast<i64>(opposite.low[axis]) + opposite.high[axis]
                      - own.low[axis] - own.high[axis];
  }
  std::vector<Proposal> pool;
  const std::size_t capacity = std::min(range.size(), std::size_t{need} + 1);
  pool.reserve(capacity);
  for (std::size_t id = range.first; id < range.last; ++id) {
    i64 score = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      score += direction[axis] * rectangle.points()[id][axis];
    }
    auto position = pool.begin();
    while (position != pool.end()) {
      counter_add(work.pool_selection_tests);
      if (score > position->score || (score == position->score && id < position->id)) {
        break;
      }
      ++position;
    }
    if (position != pool.end() || pool.size() < capacity) {
      pool.insert(position, Proposal{score, id});
      if (pool.size() > capacity) {
        pool.pop_back();
      }
    }
  }
  counter_add(work.pool_selected, pool.size());
  std::vector<std::uint8_t> credits(range.size(), 0);
  for (std::size_t id = range.first; id < range.last; ++id) {
    auto& value = credits[id - range.first];
    for (const auto& proposal : pool) {
      if (proposal.id != id && universal_witness(
              lane, rectangle.points()[id], opposite,
              rectangle.points()[proposal.id], work.predicates)) {
        ++value;
        if (value == need) {
          break;
        }
      }
    }
  }
  return credits;
}

class DualTree {
 public:
  DualTree(std::span<const Point3> points, Range range, const Box3& opposite,
           Lane lane, std::uint8_t need, Work& work)
      : points_(points), range_(range), opposite_(opposite), lane_(lane),
        need_(need), work_(work), order_(range.size()) {
    std::iota(order_.begin(), order_.end(), range.first);
    // Do not reserve 2*n speculatively or store an anchor x witness matrix.
    build({0, order_.size()}, 0);
  }

  [[nodiscard]] std::vector<std::uint8_t> run() {
    visit(0, 0, 0);
    std::vector<std::uint8_t> values(range_.size(), 0);
    extract(0, values);
    return values;
  }

 private:
  static constexpr std::size_t absent = std::numeric_limits<std::size_t>::max();
  struct Node {
    Range range;
    Box3 box;
    std::size_t left{absent};
    std::size_t right{absent};
    std::uint8_t minimum{};
    std::uint8_t lazy{};
    [[nodiscard]] bool leaf() const noexcept { return left == absent; }
  };
  std::span<const Point3> points_;
  Range range_;
  const Box3& opposite_;
  Lane lane_;
  std::uint8_t need_;
  Work& work_;
  std::vector<std::size_t> order_;
  std::vector<Node> nodes_;

  std::size_t build(Range range, u64 depth) {
    Box3 box{points_[order_[range.first]], points_[order_[range.first]]};
    for (std::size_t i = range.first; i < range.last; ++i) {
      counter_add(work_.tree_point_visits);
      const auto& p = points_[order_[i]];
      box.low = {std::min(box.low.x, p.x), std::min(box.low.y, p.y),
                 std::min(box.low.z, p.z)};
      box.high = {std::max(box.high.x, p.x), std::max(box.high.y, p.y),
                  std::max(box.high.z, p.z)};
    }
    const std::size_t index = nodes_.size();
    nodes_.push_back(Node{range, box});
    counter_add(work_.tree_nodes);
    work_.max_tree_depth = std::max(work_.max_tree_depth, depth);
    if (range.size() == 1) {
      return index;
    }
    std::size_t axis = 0;
    for (std::size_t candidate = 1; candidate < 3; ++candidate) {
      if (box.high[candidate] - box.low[candidate] > box.high[axis] - box.low[axis]) {
        axis = candidate;
      }
    }
    const unsigned middle = (static_cast<unsigned>(box.low[axis]) + box.high[axis]) / 2;
    const auto first = order_.begin() + static_cast<std::ptrdiff_t>(range.first);
    const auto last = order_.begin() + static_cast<std::ptrdiff_t>(range.last);
    const auto cut = std::partition(first, last, [&](std::size_t id) {
      counter_add(work_.tree_point_visits);
      return points_[id][axis] <= middle;
    });
    const auto split = static_cast<std::size_t>(cut - order_.begin());
    if (split == range.first || split == range.last) {
      throw std::logic_error("mhgp8 distinct points failed midpoint split");
    }
    // u16 midpoint splits halve a positive coordinate span: depth <=48.
    // This is an input representation bound, not a run-time truncation.
    const auto left = build({range.first, split}, depth + 1);
    const auto right = build({split, range.last}, depth + 1);
    nodes_[index].left = left;
    nodes_[index].right = right;
    return index;
  }

  void add(std::size_t index, std::size_t amount) {
    auto& node = nodes_[index];
    const auto increment = std::min<std::size_t>(need_, amount);
    node.minimum = static_cast<std::uint8_t>(
        std::min<std::size_t>(need_, node.minimum + increment));
    node.lazy = static_cast<std::uint8_t>(
        std::min<std::size_t>(need_, node.lazy + increment));
  }

  void push(std::size_t index) {
    auto& node = nodes_[index];
    if (!node.leaf() && node.lazy != 0) {
      add(node.left, node.lazy);
      add(node.right, node.lazy);
      node.lazy = 0;
    }
  }

  void visit(std::size_t anchors, std::size_t witnesses, u64 depth) {
    counter_add(work_.dual_tasks);
    work_.max_task_depth = std::max(work_.max_task_depth, depth);
    auto& a = nodes_[anchors];
    const auto& z = nodes_[witnesses];
    if (a.minimum == need_) {
      counter_add(work_.saturated_tasks);
      return;
    }
    if (a.leaf() && z.leaf()) {
      counter_add(work_.leaf_pairs);
      const auto aid = order_[a.range.first];
      const auto zid = order_[z.range.first];
      if (aid != zid && universal_witness(lane_, points_[aid], opposite_,
                                          points_[zid], work_.predicates)) {
        add(anchors, 1);
      }
      return;
    }
    const auto decision = classify_witness_block(
        lane_, a.box, opposite_, z.box, work_.predicates);
    if (decision == BlockDecision::Credit) {
      // H>0 excludes any shared site between these boxes. Every site in Z
      // is therefore a distinct non-self witness of every anchor in U.
      add(anchors, z.range.size());
      counter_add(work_.credited_blocks);
      return;
    }
    if (decision == BlockDecision::NoCredit) {
      counter_add(work_.noncredit_blocks);
      return;
    }
    if (!a.leaf() && (z.leaf() || a.range.size() >= z.range.size())) {
      push(anchors);
      visit(a.left, witnesses, depth + 1);
      visit(a.right, witnesses, depth + 1);
      a.minimum = std::min(nodes_[a.left].minimum, nodes_[a.right].minimum);
    } else {
      visit(anchors, z.left, depth + 1);
      visit(anchors, z.right, depth + 1);
    }
  }

  void extract(std::size_t index, std::vector<std::uint8_t>& values) {
    const auto& node = nodes_[index];
    if (node.leaf()) {
      values[order_[node.range.first] - range_.first] = node.minimum;
      return;
    }
    push(index);
    extract(node.left, values);
    extract(node.right, values);
  }
};

[[nodiscard]] std::vector<Range> group(std::span<const std::uint8_t> credits,
                                      Range original, std::uint8_t need,
                                      std::vector<std::size_t>& order) {
  std::vector<Range> groups(std::size_t{need} + 1);
  for (const auto credit : credits) {
    ++groups[credit].last;
  }
  std::size_t offset = 0;
  for (auto& range : groups) {
    const auto count = range.last;
    range = {offset, offset + count};
    offset += count;
  }
  auto cursors = groups;
  order.resize(credits.size());
  for (std::size_t i = 0; i < credits.size(); ++i) {
    order[cursors[credits[i]].first++] = original.first + i;
  }
  return groups;
}

}  // namespace

RectanglePtr prepare_rectangle(const RectangleInput& input, unsigned kmax,
                               unsigned separation_s) {
  // A move does not revoke pointers previously taken by the caller. Certify
  // only a private copy, then share this owner between immutable plans.
  auto result = std::shared_ptr<PreparedRectangle>(new PreparedRectangle());
  result->points_ = input.points;
  result->a_ = input.a;
  result->b_ = input.b;
  result->kmax_ = kmax;
  result->separation_s_ = separation_s;
  auto core_candidates = input.core_candidates;
  if (kmax == 0 || kmax > 10 || separation_s == 0) {
    throw std::invalid_argument("mhgp8 P0 requires Kmax in [1,10] and s>0");
  }
  const auto valid_range = [&result](Range range) {
    return range.first < range.last && range.last <= result->points_.size();
  };
  if (!valid_range(result->a_) || !valid_range(result->b_) ||
      (result->a_.first < result->b_.last && result->b_.first < result->a_.last)) {
    throw std::invalid_argument("mhgp8 requires nonempty disjoint in-range factors");
  }
  result->box_a_ = bounds(result->points_, result->a_);
  result->box_b_ = bounds(result->points_, result->b_);
  i64 gap2 = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto& a = result->box_a_;
    const auto& b = result->box_b_;
    const i64 gap = std::max({i64{0}, static_cast<i64>(a.low[axis]) - b.high[axis],
                             static_cast<i64>(b.low[axis]) - a.high[axis]});
    gap2 += gap * gap;
  }
  const auto diameter2 = std::max(diameter_squared(result->box_a_),
                                  diameter_squared(result->box_b_));
  if (static_cast<i128>(gap2) < static_cast<i128>(separation_s) * separation_s * diameter2) {
    throw std::invalid_argument("mhgp8 rectangle fails declared box separation");
  }
  std::vector<u64> keys;
  keys.reserve(result->points_.size());
  for (const auto& point : result->points_) {
    counter_add(result->work_.validation_points);
    keys.push_back((static_cast<u64>(point.x) << 32) |
                   (static_cast<u64>(point.y) << 16) | point.z);
  }
  std::sort(keys.begin(), keys.end(), [&result](u64 a, u64 b) {
    counter_add(result->work_.uniqueness_comparisons);
    return a < b;
  });
  if (std::adjacent_find(keys.begin(), keys.end()) != keys.end()) {
    throw std::invalid_argument("mhgp8 P0 requires distinct u16 sites");
  }
  std::sort(core_candidates.begin(), core_candidates.end());
  if (std::adjacent_find(core_candidates.begin(), core_candidates.end()) !=
      core_candidates.end()) {
    throw std::invalid_argument("mhgp8 core proposals contain duplicate IDs");
  }
  for (const auto id : core_candidates) {
    if (id >= result->points_.size() || contains(result->a_, id) || contains(result->b_, id)) {
      throw std::invalid_argument("mhgp8 core proposals must be outside both factors");
    }
  }
  for (const auto lane : {Lane::Q2, Lane::Q3, Lane::Q4}) {
    auto& credit = result->core_[arity(lane) - 2];
    for (const auto id : core_candidates) {
      if (credit == result->threshold(lane)) {
        break;
      }
      if (box_witness(lane, result->box_a_, result->box_b_, result->points_[id],
                       result->work_.predicates)) {
        ++credit;
      }
    }
  }
  return result;
}

std::uint8_t PreparedRectangle::threshold(Lane lane) const {
  const auto q = arity(lane);
  return q > kmax_ + 1 ? 0 : static_cast<std::uint8_t>(kmax_ + 2 - q);
}

std::uint8_t PreparedRectangle::core_credit(Lane lane) const {
  return core_[arity(lane) - 2];
}

void CreditPlan::initialize(RectanglePtr rectangle, Lane lane, Strategy strategy) {
  require_valid_lane(lane);
  if (!rectangle || (strategy != Strategy::Pool && strategy != Strategy::DualBlocks &&
                     strategy != Strategy::Tubes)) {
    throw std::invalid_argument("mhgp8 requires an owned rectangle and valid strategy");
  }
  rectangle_ = std::move(rectangle);
  lane_ = lane;
  strategy_ = strategy;
  threshold_ = rectangle_->threshold(lane);
  core_ = rectangle_->core_credit(lane);
  total_ = pair_count(rectangle_->a_range().size(), rectangle_->b_range().size());
  a_.resize(rectangle_->a_range().size(), 0);
  b_.resize(rectangle_->b_range().size(), 0);
}

CreditPlan make_credit_plan(RectanglePtr rectangle, Lane lane, Strategy strategy) {
  CreditPlan plan;
  plan.initialize(std::move(rectangle), lane, strategy);
  const auto& r = *plan.rectangle_;
  const auto need = static_cast<std::uint8_t>(plan.threshold_ - plan.core_);
  if (need == 0) {
    return plan;
  }
  if (strategy == Strategy::Pool) {
    plan.a_ = pool_credits(r, r.a_range(), r.a_box(), r.b_box(), lane, need, plan.work_);
    plan.b_ = pool_credits(r, r.b_range(), r.b_box(), r.a_box(), lane, need, plan.work_);
  } else if (strategy == Strategy::DualBlocks) {
    plan.a_ = DualTree(r.points(), r.a_range(), r.b_box(), lane, need, plan.work_).run();
    plan.b_ = DualTree(r.points(), r.b_range(), r.a_box(), lane, need, plan.work_).run();
  } else {
    plan.a_ = tube_detail::credits(r, r.a_range(), r.a_box(), r.b_box(), lane, need, plan.work_);
    plan.b_ = tube_detail::credits(r, r.b_range(), r.b_box(), r.a_box(), lane, need, plan.work_);
  }
  plan.group_residual();
  return plan;
}

void CreditPlan::group_residual() {
  const auto& r = *rectangle_;
  const auto need = static_cast<std::uint8_t>(threshold_ - core_);
  if (need == 0) {
    return;
  }
  const auto a_groups = group(a_, r.a_range(), need, a_order_);
  const auto b_groups = group(b_, r.b_range(), need, b_order_);
  for (std::size_t a = 0; a < need; ++a) {
    if (a_groups[a].size() == 0) {
      continue;
    }
    for (std::size_t b = 0; a + b < need; ++b) {
      if (b_groups[b].size() != 0) {
        blocks_.push_back({a_groups[a], b_groups[b]});
        counter_add(candidates_, pair_count(a_groups[a].size(), b_groups[b].size()));
      }
    }
  }
}

CreditBatch make_credit_batch(RectanglePtr rectangle, Strategy strategy) {
  if (strategy != Strategy::Tubes) {
    return CreditBatch({make_credit_plan(rectangle, Lane::Q2, strategy),
                        make_credit_plan(rectangle, Lane::Q3, strategy),
                        make_credit_plan(rectangle, Lane::Q4, strategy)}, Work{});
  }
  std::array<CreditPlan, 3> plans{CreditPlan{}, CreditPlan{}, CreditPlan{}};
  Work preparation_work;
  std::optional<tube_detail::PreparedTubes> a_tubes;
  std::optional<tube_detail::PreparedTubes> b_tubes;
  for (const auto lane : {Lane::Q2, Lane::Q3, Lane::Q4}) {
    auto& plan = plans[arity(lane) - 2];
    plan.initialize(rectangle, lane, strategy);
    const auto need = static_cast<std::uint8_t>(plan.threshold_ - plan.core_);
    if (need == 0) {
      continue;
    }
    if (!a_tubes) {
      a_tubes.emplace(*rectangle, rectangle->a_range(), rectangle->a_box(),
                      rectangle->b_box(), preparation_work);
      b_tubes.emplace(*rectangle, rectangle->b_range(), rectangle->b_box(),
                      rectangle->a_box(), preparation_work);
    }
    plan.a_ = tube_detail::credits(*a_tubes, lane, need, plan.work_);
    plan.b_ = tube_detail::credits(*b_tubes, lane, need, plan.work_);
    plan.group_residual();
  }
  return CreditBatch(std::move(plans), preparation_work);
}

bool CreditPlan::keeps(std::size_t a_id, std::size_t b_id) const {
  const auto a = rectangle().a_range();
  const auto b = rectangle().b_range();
  if (!contains(a, a_id) || !contains(b, b_id)) {
    throw std::invalid_argument("mhgp8 pair IDs do not belong to this ordered rectangle");
  }
  return core_ + a_[a_id - a.first] + b_[b_id - b.first] < threshold_;
}

}  // namespace mhgp8
