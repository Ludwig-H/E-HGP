#pragma once
// SCRATCH PROTOTYPE: exact dead-lane certificate for an owner edge ab.
#include "pipeline/q2_census.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <queue>
#include <vector>
#include <cstdlib>

namespace mhgp9::gen {

struct DeadEdgeProverWork {
  u64 edges{}, cells{}, outside_cells{}, deep_cells{}, failed_cells{}, pops{}, pushes{}, credited{};
  u64 q3_proved{}, q4_proved{}, q3_failed{}, q4_failed{};
};

class DeadEdgeProver {
 public:
  explicit DeadEdgeProver(const Q2CensusIndex& index, unsigned max_depth = 8, unsigned min_depth = 2)
      : index_(index), max_depth_(max_depth), min_depth_(min_depth) {}

  // Returns true iff every ball through a,b with center c, |c-m|^2 <= L^2/ratio
  // (ratio 12 for q3, 8 for q4), has at least `threshold` sites strictly inside.
  // Linear mode: forms of the given sites (excluding a, b) precomputed once.
  template <class Ranges, class Order>
  void load_linear(std::size_t a, std::size_t b, const Ranges& ranges, const Order& order) {
    setup(a, b);
    const auto points = index_.cloud().points();
    forms_.clear();
    for (const auto range : ranges)
      for (auto rank = range.first; rank < range.last; ++rank) {
        const auto id = order[rank];
        if (id == a || id == b) continue;
        Vec w{};
        for (std::size_t i = 0; i < 3; ++i) w[i] = 2 * static_cast<i64>(points[id][i]) - mid2_[i];
        forms_.push_back({kScale * (dot(w, w) - d2_), -2 * dot(w, A_), -2 * dot(w, B_)});
      }
    linear_ = true;
  }
  bool prove_loaded(unsigned ratio, std::size_t threshold, DeadEdgeProverWork& work) {
    if (threshold == 0) return false;
    ratio_ = ratio; threshold_ = threshold;
    return cell(-2 * kScale, 2 * kScale, -2 * kScale, 2 * kScale, 0, work);
  }
  bool prove(std::size_t a, std::size_t b, unsigned ratio, std::size_t threshold, DeadEdgeProverWork& work) {
    if (threshold == 0) return false;
    linear_ = false;
    setup(a, b);
    ratio_ = ratio; threshold_ = threshold;
    return cell(-2 * kScale, 2 * kScale, -2 * kScale, 2 * kScale, 0, work);
  }

 private:
  using Vec = std::array<i64, 3>;
  static constexpr unsigned kScaleBits = 20;
  static constexpr i64 kScale = i64{1} << kScaleBits;
  static i64 dot(const Vec& x, const Vec& y) { return x[0]*y[0] + x[1]*y[1] + x[2]*y[2]; }

  void setup(std::size_t a_id, std::size_t b_id) {
    const auto points = index_.cloud().points();
    const auto a = points[a_id], b = points[b_id];
    std::size_t main_axis = 0;
    for (std::size_t i = 0; i < 3; ++i) {
      v_[i] = static_cast<i64>(b[i]) - a[i];
      mid2_[i] = static_cast<i64>(a[i]) + b[i];
      if (std::llabs(v_[i]) > std::llabs(v_[main_axis])) main_axis = i;
    }
    d2_ = dot(v_, v_);
    const std::size_t ai = (main_axis + 1) % 3, aj = (main_axis + 2) % 3;
    const i64 h = std::llabs(v_[main_axis]), sign = v_[main_axis] > 0 ? 1 : -1;
    A_ = {0, 0, 0}; B_ = {0, 0, 0};
    A_[ai] = h; A_[main_axis] = -sign * v_[ai];
    B_[aj] = h; B_[main_axis] = -sign * v_[aj];
  }

  struct Cell { i64 l, r, bo, t; };

  // Lower bound of |alpha A + beta B|^2 over the cell (per-axis nearest to 0).
  i128 norm_lower(const Cell& c) const {
    i128 norm = 0;
    for (std::size_t i = 0; i < 3; ++i) {
      const i64 x = A_[i], y = B_[i];
      const i64 lo = x * (x < 0 ? c.r : c.l) + y * (y < 0 ? c.t : c.bo);
      const i64 hi = x * (x < 0 ? c.l : c.r) + y * (y < 0 ? c.bo : c.t);
      const i128 nearest = lo > 0 ? lo : (hi < 0 ? hi : 0);
      norm += nearest * nearest;
    }
    return norm;
  }

  // For a node box and the four cell corners: LB = max over corners of the
  // min over the box of scale*L, UB = max over corners of the max over the box.
  void node_bounds(const Box3& box, const Cell& c, i64& lb, i64& ub) const {
    lb = std::numeric_limits<i64>::min(); ub = std::numeric_limits<i64>::min();
    for (unsigned corner = 0; corner < 4; ++corner) {
      const i64 alpha = (corner & 1U) ? c.r : c.l;
      const i64 beta = (corner & 2U) ? c.t : c.bo;
      i64 minimum = -kScale * d2_, maximum = minimum;
      for (std::size_t axis = 0; axis < 3; ++axis) {
        const i64 low = 2 * static_cast<i64>(box.low[axis]) - mid2_[axis];
        const i64 high = 2 * static_cast<i64>(box.high[axis]) - mid2_[axis];
        const i64 target = A_[axis] * alpha + B_[axis] * beta;
        const auto value = [&](i64 w) { return kScale * w * w - 2 * w * target; };
        const i64 at_low = value(low), at_high = value(high);
        maximum += std::max(at_low, at_high);
        if (target < kScale * low) minimum += at_low;
        else if (target > kScale * high) minimum += at_high;
        else {
          const i128 square = static_cast<i128>(target) * target;
          const i64 quotient = static_cast<i64>(square >> kScaleBits);
          minimum -= quotient + ((square & (kScale - 1)) != 0 ? 1 : 0);
        }
      }
      lb = std::max(lb, minimum); ub = std::max(ub, maximum);
    }
  }

  // Exact count (capped at threshold) of sites strictly inside for every
  // center of the closed cell.
  // Min over the box of scale*L at ONE center (alpha,beta) of the cell.
  i64 box_min_at(const Box3& box, i64 alpha, i64 beta) const {
    i64 minimum = -kScale * d2_;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const i64 low = 2 * static_cast<i64>(box.low[axis]) - mid2_[axis];
      const i64 high = 2 * static_cast<i64>(box.high[axis]) - mid2_[axis];
      const i64 target = A_[axis] * alpha + B_[axis] * beta;
      if (target < kScale * low) minimum += kScale * low * low - 2 * low * target;
      else if (target > kScale * high) minimum += kScale * high * high - 2 * high * target;
      else {
        const i128 square = static_cast<i128>(target) * target;
        const i64 quotient = static_cast<i64>(square >> kScaleBits);
        minimum -= quotient + ((square & (kScale - 1)) != 0 ? 1 : 0);
      }
    }
    return minimum;
  }
  // Exact max over the closed cell of scale*L for one site.
  i64 site_max(const Point3& p, const Cell& c) const {
    Vec w{};
    for (std::size_t i = 0; i < 3; ++i) w[i] = 2 * static_cast<i64>(p[i]) - mid2_[i];
    const i64 k0 = dot(w, w) - d2_, gx = -2 * dot(w, A_), gy = -2 * dot(w, B_);
    return kScale * k0 + gx * (gx < 0 ? c.l : c.r) + gy * (gy < 0 ? c.bo : c.t);
  }

  std::size_t uniform_count_center(const Cell& c, DeadEdgeProverWork& work) {
    const auto nodes = index_.spatial_nodes();
    const auto points = index_.cloud().points();
    const auto order = index_.spatial_order();
    const i64 ca = c.l + (c.r - c.l) / 2, cb = c.bo + (c.t - c.bo) / 2;
    using Item = ItemStore;
    heap_.clear();
    auto push = [&](std::size_t id) {
      const i64 lb = box_min_at(nodes[id].box, ca, cb);
      if (lb < 0) { heap_.push_back({lb, 0, id}); std::push_heap(heap_.begin(), heap_.end(), std::greater<Item>{}); ++work.pushes; }
    };
    push(0);
    std::size_t credit = 0;
    while (!heap_.empty()) {
      std::pop_heap(heap_.begin(), heap_.end(), std::greater<Item>{});
      const Item it = heap_.back(); heap_.pop_back(); ++work.pops;
      const auto& node = nodes[it.node];
      if (node.left == Q2SpatialNode::absent) {
        if (site_max(points[order[node.range.first]], c) < 0) {
          ++credit; ++work.credited;
          if (credit >= threshold_) return credit;
        }
        continue;
      }
      push(node.left); push(node.right);
    }
    return credit;
  }

  std::size_t uniform_count(const Cell& c, DeadEdgeProverWork& work) {
    if (linear_) {
      std::size_t credit = 0;
      for (const auto& f : forms_) {
        ++work.pops;
        if (f.k + f.x * (f.x < 0 ? c.l : c.r) + f.y * (f.y < 0 ? c.bo : c.t) < 0 && ++credit >= threshold_) return credit;
      }
      return credit;
    }
    static const bool center_mode = std::getenv("PV_CENTER") != nullptr;
    if (center_mode) return uniform_count_center(c, work);
    const auto nodes = index_.spatial_nodes();
    using Item = ItemStore;
    heap_.clear();
    auto push = [&](std::size_t id) {
      i64 lb, ub; node_bounds(nodes[id].box, c, lb, ub);
      if (lb < 0) { heap_.push_back({lb, ub, id}); std::push_heap(heap_.begin(), heap_.end(), std::greater<Item>{}); ++work.pushes; }
    };
    push(0);
    std::size_t credit = 0;
    while (!heap_.empty()) {
      std::pop_heap(heap_.begin(), heap_.end(), std::greater<Item>{});
      const Item it = heap_.back(); heap_.pop_back(); ++work.pops;
      const auto& node = nodes[it.node];
      if (it.ub < 0) {
        credit += node.range.size(); work.credited += node.range.size();
        if (credit >= threshold_) return credit;
        continue;
      }
      if (node.left == Q2SpatialNode::absent) continue;  // leaf with ub>=0: not uniformly inside
      push(node.left); push(node.right);
    }
    return credit;
  }

  // Exact count (capped) of sites strictly inside the ball at the cell's
  // lower-left corner (a center of the closed cell).
  std::size_t point_depth(const Cell& c, DeadEdgeProverWork& work) {
    if (linear_) {
      std::size_t count = 0;
      for (const auto& f : forms_) {
        ++work.pops;
        if (f.k + f.x * c.l + f.y * c.bo < 0 && ++count >= threshold_) return count;
      }
      return count;
    }
    const Cell p{c.l, c.l, c.bo, c.bo};
    const auto nodes = index_.spatial_nodes();
    std::size_t count = 0;
    stack_.clear(); stack_.push_back(0);
    while (!stack_.empty()) {
      const auto id = stack_.back(); stack_.pop_back(); ++work.pops;
      i64 lb, ub; node_bounds(nodes[id].box, p, lb, ub);
      if (lb >= 0) continue;
      if (ub < 0) { count += nodes[id].range.size(); if (count >= threshold_) return count; continue; }
      if (nodes[id].left == Q2SpatialNode::absent) continue;
      stack_.push_back(nodes[id].right); stack_.push_back(nodes[id].left);
    }
    return count;
  }

  bool cell(i64 l, i64 r, i64 bo, i64 t, unsigned depth, DeadEdgeProverWork& work) {
    ++work.cells;
    const Cell c{l, r, bo, t};
    if (static_cast<i128>(ratio_ / 4) * norm_lower(c) > static_cast<i128>(d2_) * kScale * kScale) {
      // ratio/4: q3 (12) -> 3|C|^2 > |v|^2 ; q4 (8) -> 2|C|^2 > |v|^2
      ++work.outside_cells; return true;
    }
    if (depth >= min_depth_ && uniform_count(c, work) >= threshold_) { ++work.deep_cells; return true; }
    if (depth >= min_depth_ && point_depth(c, work) < threshold_) { ++work.failed_cells; return false; }
    if (depth == max_depth_) { ++work.failed_cells; return false; }
    const i64 mx = l + (r - l) / 2, my = bo + (t - bo) / 2;
    return cell(l, mx, bo, my, depth + 1, work) && cell(mx, r, bo, my, depth + 1, work) &&
           cell(l, mx, my, t, depth + 1, work) && cell(mx, r, my, t, depth + 1, work);
  }

  const Q2CensusIndex& index_;
  unsigned max_depth_;
  unsigned min_depth_;
  std::vector<std::size_t> stack_;
  struct Form { i64 k, x, y; };
  std::vector<Form> forms_;
  bool linear_{false};
  Vec v_{}, mid2_{}, A_{}, B_{};
  i64 d2_{};
  unsigned ratio_{};
  std::size_t threshold_{};
  struct ItemStore { i64 lb, ub; std::size_t node; bool operator>(const ItemStore& o) const { return lb > o.lb; } };
  std::vector<ItemStore> heap_;
};

}  // namespace mhgp9::gen
